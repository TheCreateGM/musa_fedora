#include "gguf.h"
#include "model.h"
#include "musa_backend.h"
#include "tokenizer.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <memory>
#include <random>
#include <string>
#include <vector>

#ifdef _OPENMP
#include <omp.h>
#endif

namespace {

struct Args {
    std::string model;
    std::string backend = "gpu";
    std::string prompt = "Once upon a time";
    std::string system = "You are a helpful assistant.";
    std::string dump_logits;
    int tokens = 32;
    int compare_steps = 8;
    int threads = 0;
    float temperature = 0.f;
    unsigned seed = 1;
    bool chat = false;
    bool ids = false;
    bool bench = false;
};

void usage() {
    std::fputs(
        "usage: musa-infer --model FILE [options]\n"
        "  --backend gpu|cpu|compare   execution backend (default gpu)\n"
        "  --prompt TEXT               prompt text (default: 'Once upon a time')\n"
        "  --chat                      wrap the prompt in the TinyLlama/Zephyr ChatML template\n"
        "  --system TEXT               system message for --chat\n"
        "  --tokens N                  tokens to generate (default 32)\n"
        "  --compare-steps N           steps compared by --backend compare (default 8)\n"
        "  --temperature T             sampling temperature, 0 = greedy (default 0)\n"
        "  --seed N                    sampling seed (default 1)\n"
        "  --threads N                 CPU reference threads (default: all)\n"
        "  --ids                       print prompt token ids\n"
        "  --dump-logits FILE          write the first-step logits as float32\n"
        "  --bench                     report prompt and generation throughput\n",
        stderr);
}

Args parse(int argc, char** argv) {
    Args a;
    for (int i = 1; i < argc; ++i) {
        std::string k = argv[i];
        auto next = [&]() -> std::string {
            if (i + 1 >= argc) { std::fprintf(stderr, "musa-infer: %s needs a value\n", k.c_str()); std::exit(2); }
            return argv[++i];
        };
        if (k == "--model") a.model = next();
        else if (k == "--backend") a.backend = next();
        else if (k == "--prompt") a.prompt = next();
        else if (k == "--system") a.system = next();
        else if (k == "--tokens") a.tokens = std::stoi(next());
        else if (k == "--compare-steps") a.compare_steps = std::stoi(next());
        else if (k == "--temperature") a.temperature = std::stof(next());
        else if (k == "--seed") a.seed = static_cast<unsigned>(std::stoul(next()));
        else if (k == "--threads") a.threads = std::stoi(next());
        else if (k == "--dump-logits") a.dump_logits = next();
        else if (k == "--chat") a.chat = true;
        else if (k == "--ids") a.ids = true;
        else if (k == "--bench") a.bench = true;
        else if (k == "-h" || k == "--help") { usage(); std::exit(0); }
        else { std::fprintf(stderr, "musa-infer: unknown option %s\n", k.c_str()); usage(); std::exit(2); }
    }
    if (a.model.empty()) { usage(); std::exit(2); }
    if (a.backend != "gpu" && a.backend != "cpu" && a.backend != "compare") {
        std::fprintf(stderr, "musa-infer: --backend must be gpu, cpu or compare\n"); std::exit(2);
    }
    return a;
}

int argmax(const std::vector<float>& v) {
    return static_cast<int>(std::max_element(v.begin(), v.end()) - v.begin());
}

int sample(const std::vector<float>& logits, float temperature, std::mt19937& rng) {
    if (temperature <= 0.f) return argmax(logits);
    std::vector<double> p(logits.size());
    double max_logit = *std::max_element(logits.begin(), logits.end());
    double sum = 0.0;
    for (size_t i = 0; i < logits.size(); ++i) {
        p[i] = std::exp((logits[i] - max_logit) / temperature);
        sum += p[i];
    }
    std::uniform_real_distribution<double> dist(0.0, sum);
    double coin = dist(rng), acc = 0.0;
    for (size_t i = 0; i < p.size(); ++i) {
        acc += p[i];
        if (coin < acc) return static_cast<int>(i);
    }
    return static_cast<int>(p.size() - 1);
}

void print_piece(std::string& pending) {
    for (unsigned char c : pending) {
        if (c == '\n' || c == '\t' || c >= 0x20) std::fputc(c, stdout);
    }
    pending.clear();
}

std::string chat_prompt(const Args& a) {
    return "<|system|>\n" + a.system + "</s>\n<|user|>\n" + a.prompt + "</s>\n<|assistant|>\n";
}

struct CompareStats {
    double max_abs = 0.0;
    double sum_abs = 0.0;
    size_t values = 0;
    int top1_same = 0;
    int top1_total = 0;
};

void compare_logits(const std::vector<float>& a, const std::vector<float>& b, CompareStats& s) {
    if (a.size() != b.size()) throw std::runtime_error("backend logits have different sizes");
    for (size_t i = 0; i < a.size(); ++i) {
        const double d = std::fabs(static_cast<double>(a[i]) - static_cast<double>(b[i]));
        s.max_abs = std::max(s.max_abs, d);
        s.sum_abs += d;
        ++s.values;
    }
    ++s.top1_total;
    if (argmax(a) == argmax(b)) ++s.top1_same;
}

double seconds_since(std::chrono::steady_clock::time_point t0) {
    return std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
}

} // namespace

int main(int argc, char** argv) {
    Args args;
    try { args = parse(argc, argv); } catch (const std::exception& e) { std::fprintf(stderr, "musa-infer: %s\n", e.what()); return 2; }
#ifdef _OPENMP
    if (args.threads > 0) omp_set_num_threads(args.threads);
#endif

    try {
        Model model(args.model);
        const ModelConfig& c = model.config();
        Tokenizer tokenizer;
        tokenizer.build(model.file());

        std::printf("MUSA_INFER_MODEL: %s\n", args.model.c_str());
        std::printf("MUSA_INFER_CONFIG: arch=llama dim=%d layers=%d heads=%d kv_heads=%d head_dim=%d hidden=%d vocab=%d ctx=%d rope_dim=%d theta=%.0f eps=%.6g\n",
                    c.dim, c.n_layers, c.n_heads, c.n_kv_heads, c.head_dim, c.hidden_dim, c.vocab_size,
                    c.seq_len, c.rope_dim, c.rope_theta, c.rms_eps);
        std::printf("MUSA_INFER_TOKENIZER: vocab=%d bos=%d eos=%d\n", tokenizer.vocab_size(), tokenizer.bos_id(), tokenizer.eos_id());

        std::string text = args.chat ? chat_prompt(args) : args.prompt;
        std::vector<int> prompt_ids = args.chat ? tokenizer.encode_chat(text, true, false)
                                                : tokenizer.encode(text, true, false);
        if (prompt_ids.size() >= static_cast<size_t>(c.seq_len)) {
            std::fprintf(stderr, "musa-infer: prompt (%zu tokens) does not fit the %d-token context\n", prompt_ids.size(), c.seq_len);
            return 2;
        }
        std::printf("MUSA_INFER_PROMPT_TOKENS: %zu\n", prompt_ids.size());
        if (args.ids) {
            std::printf("MUSA_INFER_PROMPT_IDS:");
            for (int id : prompt_ids) std::printf(" %d", id);
            std::printf("\n");
            std::printf("MUSA_INFER_PROMPT_PIECES:");
            for (int id : prompt_ids) {
                std::string escaped;
                for (unsigned char ch : tokenizer.piece(id)) {
                    if (ch == '\n') escaped += "\\n";
                    else if (ch == '\t') escaped += "\\t";
                    else if (ch < 0x20 || ch == 0x7f) { char b[8]; std::snprintf(b, sizeof(b), "\\x%02X", ch); escaped += b; }
                    else escaped += static_cast<char>(ch);
                }
                std::printf(" [%s]", escaped.c_str());
            }
            std::printf("\n");
        }

        std::string error;
        std::unique_ptr<MusaEngine> engine;
        if (args.backend != "cpu") {
            std::string engine_error;
            engine.reset(new MusaEngine(model, engine_error));
            if (!engine->valid()) {
                std::fprintf(stderr, "musa-infer: MUSA backend unavailable: %s\n", engine_error.c_str());
                return 3;
            }
            std::printf("MUSA_INFER_DEVICE: %s\n", engine->device_name().c_str());
            std::printf("MUSA_INFER_WEIGHTS: %.1f MiB\n", static_cast<double>(engine->weight_bytes()) / (1024.0 * 1024.0));
        }

        if (args.backend == "compare") {
            CompareStats stats;
            const int steps = std::min(args.compare_steps, c.seq_len - static_cast<int>(prompt_ids.size()) - 1);
            std::vector<int> generated;
            std::string rendered;
            int next_token = prompt_ids.back();
            int pos = 0;
            for (size_t i = 0; i < prompt_ids.size(); ++i) {
                next_token = prompt_ids[i];
                const std::vector<float>& host = model.cpu_forward(next_token, pos);
                std::vector<float> cpu_logits = host;
                const std::vector<float>& gpu_logits = engine->forward(next_token, pos, error);
                if (!error.empty()) { std::fprintf(stderr, "musa-infer: %s\n", error.c_str()); return 4; }
                compare_logits(cpu_logits, gpu_logits, stats);
                ++pos;
            }
            for (int s = 0; s < steps; ++s) {
                const std::vector<float>& host = model.cpu_forward(next_token, pos);
                std::vector<float> cpu_logits = host;
                const std::vector<float>& gpu_logits = engine->forward(next_token, pos, error);
                if (!error.empty()) { std::fprintf(stderr, "musa-infer: %s\n", error.c_str()); return 4; }
                compare_logits(cpu_logits, gpu_logits, stats);
                next_token = argmax(gpu_logits);
                generated.push_back(next_token);
                rendered += tokenizer.decode(next_token);
                ++pos;
            }
            std::printf("MUSA_INFER_COMPARE: steps=%d values=%zu max_abs=%.6g mean_abs=%.6g top1_same=%d/%d\n",
                        stats.top1_total, stats.values, stats.max_abs, stats.sum_abs / std::max<size_t>(1, stats.values),
                        stats.top1_same, stats.top1_total);
            std::printf("MUSA_INFER_GENERATED: %s\n", rendered.c_str());
            const bool pass = stats.max_abs <= 0.05 && stats.top1_same == stats.top1_total;
            std::printf("MUSA_INFER_COMPARE_STATUS: %s\n", pass ? "PASS" : "FAIL");
            return pass ? 0 : 5;
        }

        const bool use_gpu = args.backend == "gpu";
        std::vector<float> logits;
        std::mt19937 rng(args.seed);

        auto step = [&](int token, int pos) -> const std::vector<float>& {
            if (use_gpu) {
                const std::vector<float>& out = engine->forward(token, pos, error);
                if (!error.empty()) throw std::runtime_error(error);
                return out;
            }
            return model.cpu_forward(token, pos);
        };

        auto prompt_start = std::chrono::steady_clock::now();
        int pos = 0;
        for (int id : prompt_ids) {
            logits = step(id, pos);
            ++pos;
        }
        const double prompt_seconds = seconds_since(prompt_start);

        {
            const int top = argmax(logits);
            std::printf("MUSA_INFER_TOP1: id=%d piece=%s\n", top, tokenizer.piece(top).c_str());
        }
        if (!args.dump_logits.empty()) {
            std::ofstream out(args.dump_logits, std::ios::binary);
            if (!out) throw std::runtime_error("cannot open " + args.dump_logits);
            out.write(reinterpret_cast<const char*>(logits.data()), static_cast<std::streamsize>(logits.size() * sizeof(float)));
            std::printf("MUSA_INFER_LOGITS: wrote %zu floats to %s\n", logits.size(), args.dump_logits.c_str());
        }

        std::vector<int> generated;
        std::string pending, complete;
        auto gen_start = std::chrono::steady_clock::now();
        for (int n = 0; n < args.tokens; ++n) {
            int token = sample(logits, args.temperature, rng);
            if (token == tokenizer.eos_id()) break;
            generated.push_back(token);
            std::string piece = tokenizer.decode(token);
            pending += piece;
            complete += piece;
            print_piece(pending);
            std::fflush(stdout);
            if (pos + 1 >= c.seq_len) break;
            logits = step(token, pos);
            ++pos;
        }
        const double gen_seconds = seconds_since(gen_start);
        std::fputc('\n', stdout);

        std::printf("MUSA_INFER_GENERATED: %s\n", complete.c_str());
        std::printf("MUSA_INFER_SPEED: prompt=%.3fs (%.2f tok/s) generate=%.3fs (%.2f tok/s) tokens=%zu\n",
                    prompt_seconds, prompt_ids.size() / std::max(1e-9, prompt_seconds),
                    gen_seconds, generated.size() / std::max(1e-9, gen_seconds), generated.size());
        if (args.bench) {
            std::printf("MUSA_INFER_BENCH: prompt_tokens=%zu generated=%zu prompt_tok_s=%.2f gen_tok_s=%.2f\n",
                        prompt_ids.size(), generated.size(),
                        prompt_ids.size() / std::max(1e-9, prompt_seconds),
                        generated.size() / std::max(1e-9, gen_seconds));
        }
        return 0;
    } catch (const std::exception& e) {
        std::fprintf(stderr, "musa-infer: %s\n", e.what());
        return 1;
    }
}

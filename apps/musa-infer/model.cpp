#include "model.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>

namespace {

struct Q8Block {
    uint16_t d;
    int8_t qs[32];
} __attribute__((packed));

} // namespace

float half_to_float(uint16_t h) {
    uint32_t sign = (h >> 15) & 1u;
    uint32_t exponent = (h >> 10) & 0x1fu;
    uint32_t mantissa = h & 0x3ffu;
    float value;
    if (exponent == 0) {
        value = std::ldexp(static_cast<float>(mantissa), -24);
    } else if (exponent == 31) {
        value = mantissa == 0 ? INFINITY : NAN;
    } else {
        value = std::ldexp(static_cast<float>(mantissa | 0x400u), static_cast<int>(exponent) - 25);
    }
    return sign ? -value : value;
}

Model::Model(const std::string& path) : file_(GgufFile::open(path)) {
    const std::string arch = file_.get_string("general.architecture", "llama");
    if (arch != "llama")
        throw std::runtime_error("unsupported architecture '" + arch + "' (musa-infer implements llama-family GGUFs)");
    auto key = [&](const std::string& suffix) { return arch + "." + suffix; };

    cfg_.dim = static_cast<int>(file_.get_int(key("embedding_length"), 0));
    cfg_.n_layers = static_cast<int>(file_.get_int(key("block_count"), 0));
    cfg_.n_heads = static_cast<int>(file_.get_int(key("attention.head_count"), 0));
    cfg_.n_kv_heads = static_cast<int>(file_.get_int(key("attention.head_count_kv"), cfg_.n_heads));
    cfg_.hidden_dim = static_cast<int>(file_.get_int(key("feed_forward_length"), 0));
    cfg_.vocab_size = static_cast<int>(file_.get_int(key("vocab_size"), 0));
    const int context = static_cast<int>(file_.get_int(key("context_length"), 2048));
    cfg_.seq_len = std::max(1, std::min(context, 2048));
    cfg_.rms_eps = static_cast<float>(file_.get_float(key("attention.layer_norm_rms_epsilon"), 1e-5));
    cfg_.rope_theta = static_cast<float>(file_.get_float(key("rope.freq_base"), 10000.0));

    if (cfg_.dim <= 0 || cfg_.n_layers <= 0 || cfg_.n_heads <= 0 || cfg_.n_kv_heads <= 0 ||
        cfg_.hidden_dim <= 0 || cfg_.dim % cfg_.n_heads != 0 || cfg_.n_heads % cfg_.n_kv_heads != 0)
        throw std::runtime_error("model metadata is incomplete or inconsistent");
    cfg_.head_dim = cfg_.dim / cfg_.n_heads;
    cfg_.rope_dim = static_cast<int>(file_.get_int(key("rope.dimension_count"), cfg_.head_dim));
    if (cfg_.rope_dim <= 0 || cfg_.rope_dim > cfg_.head_dim || (cfg_.rope_dim & 1) != 0)
        throw std::runtime_error("unsupported rope.dimension_count");

    if (cfg_.vocab_size <= 0) {
        cfg_.vocab_size = static_cast<int>(file_.get_strings("tokenizer.ggml.tokens").size());
    }
    cfg_.has_output_weight = file_.has_tensor("output.weight");

    const GgufTensor& emb = file_.tensor("token_embd.weight");
    if (emb.dims.size() != 2 || static_cast<int>(emb.dims[0]) != cfg_.dim ||
        static_cast<int>(emb.dims[1]) != cfg_.vocab_size)
        throw std::runtime_error("token_embd.weight shape does not match the model metadata");
    if (emb.type != GgufType::Q8_0)
        throw std::runtime_error(std::string("this build supports Q8_0 token embeddings, model has ") + gguf_type_name(emb.type));

    for (int l = 0; l < cfg_.n_layers; ++l) {
        const std::string p = "blk." + std::to_string(l) + ".";
        for (const char* t : {"attn_q.weight", "attn_k.weight", "attn_v.weight", "attn_output.weight",
                             "ffn_gate.weight", "ffn_up.weight", "ffn_down.weight", "attn_norm.weight", "ffn_norm.weight"}) {
            if (!file_.has_tensor(p + t)) throw std::runtime_error("model is missing tensor " + p + t);
        }
        const GgufTensor& qw = file_.tensor(p + "attn_q.weight");
        if (qw.type != GgufType::Q8_0)
            throw std::runtime_error("this build supports Q8_0 projection weights; " + p + "attn_q.weight is " + gguf_type_name(qw.type));
    }

    const int dim = cfg_.dim, hidden = cfg_.hidden_dim, kv = cfg_.kv_dim();
    x_.assign(dim, 0.f);
    xb_.assign(dim, 0.f);
    xb2_.assign(dim, 0.f);
    q_.assign(dim, 0.f);
    k_.assign(kv, 0.f);
    v_.assign(kv, 0.f);
    att_.assign(static_cast<size_t>(cfg_.n_heads) * cfg_.seq_len, 0.f);
    hb_.assign(hidden, 0.f);
    hb2_.assign(hidden, 0.f);
    logits_.assign(cfg_.vocab_size, 0.f);
    kcache_.assign(static_cast<size_t>(cfg_.n_layers) * cfg_.seq_len * kv, 0.f);
    vcache_.assign(static_cast<size_t>(cfg_.n_layers) * cfg_.seq_len * kv, 0.f);
}

void Model::matmul_cpu(float* out, const float* x, const GgufTensor& w, int rows) const {
    const int cols = static_cast<int>(w.dims[0]);
    const uint8_t* base = static_cast<const uint8_t*>(file_.tensor_data(w));
    uint64_t vpb = 0, bpb = 0;
    gguf_block_layout(w.type, vpb, bpb);
    const uint64_t row_bytes = static_cast<uint64_t>(cols) / vpb * bpb;

    #pragma omp parallel for schedule(static)
    for (int r = 0; r < rows; ++r) {
        const uint8_t* row = base + static_cast<uint64_t>(r) * row_bytes;
        float acc = 0.f;
        if (w.type == GgufType::Q8_0) {
            const int blocks = cols / 32;
            for (int b = 0; b < blocks; ++b) {
                const Q8Block* blk = reinterpret_cast<const Q8Block*>(row + static_cast<size_t>(b) * 34);
                const float d = half_to_float(blk->d);
                const float* xb = x + b * 32;
                for (int i = 0; i < 32; ++i) acc += d * static_cast<float>(blk->qs[i]) * xb[i];
            }
        } else if (w.type == GgufType::F32) {
            const float* wp = reinterpret_cast<const float*>(row);
            for (int c = 0; c < cols; ++c) acc += wp[c] * x[c];
        } else if (w.type == GgufType::F16) {
            const uint16_t* wp = reinterpret_cast<const uint16_t*>(row);
            for (int c = 0; c < cols; ++c) acc += half_to_float(wp[c]) * x[c];
        } else {
            acc = NAN;
        }
        out[r] = acc;
    }
}

void Model::rmsnorm_cpu(float* out, const float* x, const GgufTensor& w) const {
    const int n = cfg_.dim;
    double sum = 0.0;
    for (int i = 0; i < n; ++i) sum += static_cast<double>(x[i]) * x[i];
    const float scale = 1.0f / std::sqrt(static_cast<float>(sum / n) + cfg_.rms_eps);
    const uint8_t* base = static_cast<const uint8_t*>(file_.tensor_data(w));
    if (w.type == GgufType::F32) {
        const float* wp = reinterpret_cast<const float*>(base);
        for (int i = 0; i < n; ++i) out[i] = x[i] * scale * wp[i];
    } else if (w.type == GgufType::F16) {
        const uint16_t* wp = reinterpret_cast<const uint16_t*>(base);
        for (int i = 0; i < n; ++i) out[i] = x[i] * scale * half_to_float(wp[i]);
    } else {
        throw std::runtime_error("unsupported norm tensor type " + std::string(gguf_type_name(w.type)));
    }
}

const std::vector<float>& Model::cpu_forward(int token, int pos) {
    const ModelConfig& c = cfg_;
    const int dim = c.dim, kv = c.kv_dim(), hd = c.head_dim;
    const int kv_mul = c.n_heads / c.n_kv_heads;
    if (pos < 0 || pos >= c.seq_len) throw std::runtime_error("position is outside the model context");

    const GgufTensor& emb = file_.tensor("token_embd.weight");
    const uint8_t* emb_base = static_cast<const uint8_t*>(file_.tensor_data(emb));
    const uint64_t emb_row = static_cast<uint64_t>(dim) / 32 * 34;
    {
        const uint8_t* row = emb_base + static_cast<uint64_t>(token) * emb_row;
        for (int b = 0; b < dim / 32; ++b) {
            const Q8Block* blk = reinterpret_cast<const Q8Block*>(row + static_cast<size_t>(b) * 34);
            const float d = half_to_float(blk->d);
            for (int i = 0; i < 32; ++i) x_[b * 32 + i] = d * static_cast<float>(blk->qs[i]);
        }
    }

    for (int l = 0; l < c.n_layers; ++l) {
        const std::string p = "blk." + std::to_string(l) + ".";
        rmsnorm_cpu(xb_.data(), x_.data(), file_.tensor(p + "attn_norm.weight"));

        matmul_cpu(q_.data(), xb_.data(), file_.tensor(p + "attn_q.weight"), dim);
        matmul_cpu(k_.data(), xb_.data(), file_.tensor(p + "attn_k.weight"), kv);
        matmul_cpu(v_.data(), xb_.data(), file_.tensor(p + "attn_v.weight"), kv);

        for (int i = 0; i < c.rope_dim; i += 2) {
            const float freq = std::pow(c.rope_theta, -static_cast<float>(i) / static_cast<float>(c.rope_dim));
            const float angle = static_cast<float>(pos) * freq;
            const float cs = std::cos(angle), sn = std::sin(angle);
            for (int h = 0; h < c.n_heads; ++h) {
                float* vec = q_.data() + h * hd + i;
                const float a = vec[0], b = vec[1];
                vec[0] = a * cs - b * sn;
                vec[1] = a * sn + b * cs;
            }
            for (int h = 0; h < c.n_kv_heads; ++h) {
                float* vec = k_.data() + h * hd + i;
                const float a = vec[0], b = vec[1];
                vec[0] = a * cs - b * sn;
                vec[1] = a * sn + b * cs;
            }
        }

        float* kc = kcache_.data() + (static_cast<size_t>(l) * c.seq_len + pos) * kv;
        float* vc = vcache_.data() + (static_cast<size_t>(l) * c.seq_len + pos) * kv;
        std::memcpy(kc, k_.data(), sizeof(float) * kv);
        std::memcpy(vc, v_.data(), sizeof(float) * kv);

        const float scale = 1.0f / std::sqrt(static_cast<float>(hd));
        for (int h = 0; h < c.n_heads; ++h) {
            const float* qv = q_.data() + h * hd;
            const float* kb = kcache_.data() + static_cast<size_t>(l) * c.seq_len * kv + (h / kv_mul) * hd;
            float* scores = att_.data() + h * c.seq_len;
            float max_score = -INFINITY;
            for (int t = 0; t <= pos; ++t) {
                const float* ktv = kb + static_cast<size_t>(t) * kv;
                float dot = 0.f;
                for (int i = 0; i < hd; ++i) dot += qv[i] * ktv[i];
                scores[t] = dot * scale;
                max_score = std::max(max_score, scores[t]);
            }
            float sum = 0.f;
            for (int t = 0; t <= pos; ++t) {
                scores[t] = std::exp(scores[t] - max_score);
                sum += scores[t];
            }
            for (int t = 0; t <= pos; ++t) scores[t] /= sum;
            for (int i = 0; i < hd; ++i) xb_[h * hd + i] = 0.f;
            const float* vb = vcache_.data() + static_cast<size_t>(l) * c.seq_len * kv + (h / kv_mul) * hd;
            for (int t = 0; t <= pos; ++t) {
                const float a = scores[t];
                const float* vtv = vb + static_cast<size_t>(t) * kv;
                for (int i = 0; i < hd; ++i) xb_[h * hd + i] += a * vtv[i];
            }
        }

        matmul_cpu(xb2_.data(), xb_.data(), file_.tensor(p + "attn_output.weight"), dim);
        for (int i = 0; i < dim; ++i) x_[i] += xb2_[i];

        rmsnorm_cpu(xb_.data(), x_.data(), file_.tensor(p + "ffn_norm.weight"));
        matmul_cpu(hb_.data(), xb_.data(), file_.tensor(p + "ffn_gate.weight"), c.hidden_dim);
        matmul_cpu(hb2_.data(), xb_.data(), file_.tensor(p + "ffn_up.weight"), c.hidden_dim);
        for (int i = 0; i < c.hidden_dim; ++i) {
            const float v = hb_[i];
            hb_[i] = v / (1.0f + std::exp(-v)) * hb2_[i];
        }
        matmul_cpu(xb_.data(), hb_.data(), file_.tensor(p + "ffn_down.weight"), dim);
        for (int i = 0; i < dim; ++i) x_[i] += xb_[i];
    }

    rmsnorm_cpu(x_.data(), x_.data(), file_.tensor("output_norm.weight"));
    const GgufTensor& out_w = c.has_output_weight ? file_.tensor("output.weight") : emb;
    matmul_cpu(logits_.data(), x_.data(), out_w, c.vocab_size);
    return logits_;
}

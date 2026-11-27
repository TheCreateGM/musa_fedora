#include "tokenizer.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <stdexcept>

namespace {

const char* SPACE_MARK = "\xE2\x96\x81"; // U+2581 LOWER ONE EIGHTH BLOCK

std::string normalize_piece(std::string s) {
    size_t pos;
    while ((pos = s.find(SPACE_MARK)) != std::string::npos) s.replace(pos, 3, " ");
    return s;
}

} // namespace

void Tokenizer::build(const GgufFile& file) {
    const auto& raw = file.get_strings("tokenizer.ggml.tokens");
    if (raw.empty()) throw std::runtime_error("model has no tokenizer.ggml.tokens");
    vocab_.resize(raw.size());
    for (size_t i = 0; i < raw.size(); ++i) vocab_[i] = normalize_piece(raw[i]);

    scores_.assign(vocab_.size(), 0.0f);
    if (file.meta().count("tokenizer.ggml.scores")) {
        const auto& s = file.get_floats("tokenizer.ggml.scores");
        if (s.size() != vocab_.size()) throw std::runtime_error("tokenizer scores do not match the vocabulary size");
        for (size_t i = 0; i < s.size(); ++i) scores_[i] = static_cast<float>(s[i]);
    }

    bos_id_ = static_cast<int>(file.get_int("tokenizer.ggml.bos_token_id", 1));
    eos_id_ = static_cast<int>(file.get_int("tokenizer.ggml.eos_token_id", 2));
    if (bos_id_ < 0 || bos_id_ >= vocab_size() || eos_id_ < 0 || eos_id_ >= vocab_size())
        throw std::runtime_error("model BOS/EOS id is outside the vocabulary");

    sorted_.resize(vocab_.size());
    for (size_t i = 0; i < vocab_.size(); ++i) sorted_[i] = static_cast<int>(i);
    std::sort(sorted_.begin(), sorted_.end(), [&](int a, int b) { return vocab_[a] < vocab_[b]; });

    max_token_length_ = 0;
    for (const auto& v : vocab_) max_token_length_ = std::max<unsigned>(max_token_length_, static_cast<unsigned>(v.size()));

    for (int b = 0; b < 256; ++b) {
        char name[16];
        std::snprintf(name, sizeof(name), "<0x%02X>", b);
        int id = find(name);
        byte_id_[b] = id >= 0 ? id : b + 3; // Llama keeps byte tokens after <unk>/<s>/</s>
    }

    for (const auto& v : vocab_) {
        if (v.size() >= 4 && v.rfind("<|", 0) == 0 && v.compare(v.size() - 2, 2, "|>") == 0)
            specials_.push_back(v);
    }
    std::sort(specials_.begin(), specials_.end(), [](const std::string& a, const std::string& b) {
        return a.size() > b.size();
    });
}

int Tokenizer::find(const std::string& piece) const {
    auto it = std::lower_bound(sorted_.begin(), sorted_.end(), piece, [&](int idx, const std::string& key) {
        return vocab_[idx] < key;
    });
    if (it == sorted_.end() || vocab_[*it] != piece) return -1;
    return *it;
}

std::string Tokenizer::decode_piece(const std::string& piece) const {
    unsigned value = 0;
    if (piece.size() == 6 && std::sscanf(piece.c_str(), "<0x%02X>", &value) == 1 && value < 256)
        return std::string(1, static_cast<char>(value));
    return piece;
}

std::string Tokenizer::decode(int token) const {
    if (token < 0 || token >= vocab_size()) throw std::runtime_error("token id out of range");
    return decode_piece(vocab_[token]);
}

std::vector<int> Tokenizer::encode_plain(const std::string& text) const {
    std::vector<int> tokens;
    if (text.empty()) return tokens;

    // SentencePiece add_dummy_prefix. The GGUF piece for U+2581 is " ", so the
    // dummy prefix is the token whose text is a single space.
    int dummy = find(" ");
    if (dummy >= 0) tokens.push_back(dummy);

    std::string buffer;
    for (size_t i = 0; i < text.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(text[i]);
        if ((c & 0xC0) != 0x80) buffer.clear();
        buffer.push_back(static_cast<char>(c));
        bool more = i + 1 < text.size() && (static_cast<unsigned char>(text[i + 1]) & 0xC0) == 0x80 && buffer.size() < 4;
        if (more) continue;

        int id = find(buffer);
        if (id >= 0) {
            tokens.push_back(id);
        } else {
            for (unsigned char b : buffer) tokens.push_back(byte_id_[b]);
        }
        buffer.clear();
    }

    while (tokens.size() >= 2) {
        float best_score = -1e30f;
        int best_id = -1;
        size_t best_idx = 0;
        for (size_t i = 0; i + 1 < tokens.size(); ++i) {
            std::string joined = vocab_[tokens[i]] + vocab_[tokens[i + 1]];
            int id = find(joined);
            if (id >= 0 && scores_[id] > best_score) {
                best_score = scores_[id];
                best_id = id;
                best_idx = i;
            }
        }
        if (best_id < 0) break;
        tokens[best_idx] = best_id;
        tokens.erase(tokens.begin() + static_cast<std::ptrdiff_t>(best_idx) + 1);
    }
    return tokens;
}

std::vector<int> Tokenizer::encode(const std::string& text, bool bos, bool eos) const {
    std::vector<int> out;
    if (bos) out.push_back(bos_id_);
    auto body = encode_plain(text);
    out.insert(out.end(), body.begin(), body.end());
    if (eos) out.push_back(eos_id_);
    return out;
}

std::vector<int> Tokenizer::encode_chat(const std::string& text, bool bos, bool eos) const {
    std::vector<int> out;
    if (bos) out.push_back(bos_id_);

    size_t pos = 0;
    bool first = true;
    while (pos < text.size()) {
        const std::string* hit = nullptr;
        for (const auto& s : specials_) {
            if (text.compare(pos, s.size(), s) == 0) { hit = &s; break; }
        }
        if (hit) {
            out.push_back(find(*hit));
            pos += hit->size();
            first = false;
            continue;
        }
        size_t next = text.size();
        for (const auto& s : specials_) {
            size_t p = text.find(s, pos);
            if (p != std::string::npos) next = std::min(next, p);
        }
        std::string chunk = text.substr(pos, next - pos);
        auto ids = encode_plain(chunk);
        if (!first && !ids.empty() && ids.front() == find(" ")) ids.erase(ids.begin());
        out.insert(out.end(), ids.begin(), ids.end());
        first = false;
        pos = next;
    }

    if (eos) out.push_back(eos_id_);
    return out;
}

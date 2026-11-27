// SentencePiece-BPE tokenizer for Llama-family GGUFs.
//
// The merge algorithm is adapted from llama2.c (MIT, Andrej Karpathy), which
// implements the same SentencePiece BPE that Llama-2 and TinyLlama use. GGUF
// stores the SentencePiece pieces with U+2581 ("▁") for the space marker; this
// reader rewrites them to a plain space, the representation the adapted
// algorithm expects. Special tokens written as <|name|> are matched literally
// and injected by id instead of being fed through BPE.
#pragma once

#include "gguf.h"

#include <cstdint>
#include <string>
#include <vector>

class Tokenizer {
public:
    void build(const GgufFile& file);

    int vocab_size() const { return static_cast<int>(vocab_.size()); }
    int bos_id() const { return bos_id_; }
    int eos_id() const { return eos_id_; }
    const std::string& piece(int id) const { return vocab_[id]; }

    // Returns -1 when the exact piece is not in the vocabulary.
    int find(const std::string& piece) const;

    // SentencePiece BPE. `bos`/`eos` add the model's BOS/EOS ids when true.
    std::vector<int> encode(const std::string& text, bool bos, bool eos) const;
    // Like encode(), but exact <|special|> strings are looked up and injected
    // as single ids; unknown text runs through BPE.
    std::vector<int> encode_chat(const std::string& text, bool bos, bool eos) const;

    // Bytes for one token; handles SentencePiece byte-fallback pieces such as
    // <0x0A>. The caller must not assume the result is printable.
    std::string decode(int token) const;

private:
    std::string decode_piece(const std::string& piece) const;
    std::vector<int> encode_plain(const std::string& text) const;

    std::vector<std::string> vocab_;
    std::vector<float> scores_;
    std::vector<int> sorted_;            // vocab indices sorted by piece
    std::vector<std::string> specials_;  // <|...|> tokens present in vocab
    int byte_id_[256] = {};
    unsigned max_token_length_ = 0;
    int bos_id_ = 1;
    int eos_id_ = 2;
};

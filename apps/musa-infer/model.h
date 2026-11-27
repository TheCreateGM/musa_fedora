// Llama-family model configuration, weight access and a CPU reference
// forward pass for musa-infer.
#pragma once

#include "gguf.h"

#include <cstdint>
#include <string>
#include <vector>

struct ModelConfig {
    int dim = 0;
    int n_layers = 0;
    int n_heads = 0;
    int n_kv_heads = 0;
    int hidden_dim = 0;
    int vocab_size = 0;
    int seq_len = 0;
    int head_dim = 0;
    int rope_dim = 0;
    float rms_eps = 1e-5f;
    float rope_theta = 10000.0f;
    bool has_output_weight = false;
    int kv_dim() const { return n_kv_heads * head_dim; }
};

class Model {
public:
    explicit Model(const std::string& path);

    const GgufFile& file() const { return file_; }
    const ModelConfig& config() const { return cfg_; }
    const GgufTensor& tensor(const std::string& name) const { return file_.tensor(name); }
    bool has_tensor(const std::string& name) const { return file_.has_tensor(name); }

    // CPU reference: returns the logits for one token at `pos`. Resets the KV
    // cache when pos == 0.
    const std::vector<float>& cpu_forward(int token, int pos);

private:
    void rmsnorm_cpu(float* out, const float* x, const GgufTensor& w) const;
    void matmul_cpu(float* out, const float* x, const GgufTensor& w, int rows) const;

    GgufFile file_;
    ModelConfig cfg_;
    std::vector<float> x_, xb_, xb2_, q_, k_, v_, att_, hb_, hb2_, logits_;
    std::vector<float> kcache_, vcache_;
};

// IEEE binary16 to float, host build.
float half_to_float(uint16_t h);

#include "musa_backend.h"

#include "model.h"

#include <stdexcept>

namespace {

int tensor_type(const GgufTensor& t) {
    switch (t.type) {
    case GgufType::F32: return MUSA_TYPE_F32;
    case GgufType::F16: return MUSA_TYPE_F16;
    case GgufType::Q8_0: return MUSA_TYPE_Q8_0;
    default: throw std::runtime_error(std::string("unsupported tensor type ") + gguf_type_name(t.type) + " in " + t.name);
    }
}

} // namespace

MusaEngine::MusaEngine(const Model& model, std::string& error) {
    const ModelConfig& c = model.config();

    // Fixed slot order: embedding, final norm, optional output projection, then
    // nine tensors per layer (see musa_layout.h).
    std::vector<std::string> names;
    names.reserve(static_cast<size_t>(MUSA_LAYER_BASE) + static_cast<size_t>(c.n_layers) * MUSA_LAYER_TENSORS);
    names.push_back("token_embd.weight");
    names.push_back("output_norm.weight");
    if (c.has_output_weight) names.push_back("output.weight");
    for (int l = 0; l < c.n_layers; ++l) {
        const std::string p = "blk." + std::to_string(l) + ".";
        for (const char* t : {"attn_norm.weight", "ffn_norm.weight", "attn_q.weight", "attn_k.weight",
                             "attn_v.weight", "attn_output.weight", "ffn_gate.weight", "ffn_up.weight", "ffn_down.weight"}) {
            names.push_back(p + t);
        }
    }

    std::vector<MusaTensor> tensors;
    tensors.reserve(names.size());
    try {
        for (const auto& name : names) {
            const GgufTensor& t = model.tensor(name);
            tensors.push_back(MusaTensor{name.c_str(), model.file().tensor_data(t), static_cast<size_t>(t.nbytes), tensor_type(t)});
        }
    } catch (const std::exception& e) {
        error = e.what();
        return;
    }

    MusaConfig cfg {};
    cfg.dim = c.dim;
    cfg.n_layers = c.n_layers;
    cfg.n_heads = c.n_heads;
    cfg.n_kv_heads = c.n_kv_heads;
    cfg.hidden_dim = c.hidden_dim;
    cfg.vocab_size = c.vocab_size;
    cfg.seq_len = c.seq_len;
    cfg.head_dim = c.head_dim;
    cfg.rope_dim = c.rope_dim;
    cfg.rms_eps = c.rms_eps;
    cfg.rope_theta = c.rope_theta;
    cfg.has_output_weight = c.has_output_weight ? 1 : 0;

    char buffer[512] = {};
    impl_ = musa_engine_create(tensors.data(), static_cast<int>(tensors.size()), &cfg, buffer, sizeof(buffer));
    if (!impl_) {
        error = buffer[0] ? buffer : "MUSA backend initialisation failed";
        return;
    }
    device_ = musa_engine_device(impl_);
    weight_bytes_ = musa_engine_weight_bytes(impl_);
    host_logits_.assign(static_cast<size_t>(c.vocab_size), 0.f);
}

MusaEngine::~MusaEngine() {
    if (impl_) musa_engine_destroy(impl_);
}

const std::vector<float>& MusaEngine::forward(int token, int pos, std::string& error) {
    char buffer[512] = {};
    if (musa_engine_forward(impl_, token, pos, host_logits_.data(), buffer, sizeof(buffer)) != 0) {
        error = buffer[0] ? buffer : "MUSA forward failed";
    } else {
        error.clear();
    }
    return host_logits_;
}

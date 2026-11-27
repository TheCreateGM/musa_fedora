// POD layout shared by the C++ host wrapper and the mcc-compiled backend.
// This header must stay free of C++ standard-library types: the MUSA SDK's
// clang cannot parse the newer libstdc++ headers on current Fedora.
#pragma once

#include <cstddef>
#include <cstdint>

enum {
    MUSA_TYPE_F32 = 0,
    MUSA_TYPE_F16 = 1,
    MUSA_TYPE_Q8_0 = 2,

    MUSA_EMB = 0,
    MUSA_NORM = 1,
    MUSA_OUT = 2,
    MUSA_LAYER_BASE = 3,
    MUSA_LAYER_TENSORS = 9,
};

// Layer-local order inside MUSA_LAYER_TENSORS.
enum {
    MUSA_ATTN_NORM = 0,
    MUSA_FFN_NORM = 1,
    MUSA_ATTN_Q = 2,
    MUSA_ATTN_K = 3,
    MUSA_ATTN_V = 4,
    MUSA_ATTN_OUT = 5,
    MUSA_FFN_GATE = 6,
    MUSA_FFN_UP = 7,
    MUSA_FFN_DOWN = 8,
};

struct MusaTensor {
    const char* name;
    const void* data;
    size_t nbytes;
    int type;
};

struct MusaConfig {
    int dim;
    int n_layers;
    int n_heads;
    int n_kv_heads;
    int hidden_dim;
    int vocab_size;
    int seq_len;
    int head_dim;
    int rope_dim;
    float rms_eps;
    float rope_theta;
    int has_output_weight;
};

// MUSA execution backend for musa-infer.
//
// musa_backend.cu is compiled by mcc through the project's CUDA-compatible
// nvcc wrapper and runs on the real MTT S30 (mp_10). It is deliberately a pure
// C interface: the host-side C++ (STL) code stays in musa_host.cpp, compiled by
// the system g++, because the MUSA SDK's clang cannot parse the newer libstdc++
// headers shipped with current Fedora.
#pragma once

#include "musa_layout.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

extern "C" {
typedef struct MusaEngineC MusaEngineC;
MusaEngineC* musa_engine_create(const MusaTensor* tensors, int count, const MusaConfig* cfg,
                                char* error, size_t error_len);
int musa_engine_forward(MusaEngineC* engine, int token, int pos, float* logits,
                        char* error, size_t error_len);
void musa_engine_destroy(MusaEngineC* engine);
const char* musa_engine_device(MusaEngineC* engine);
size_t musa_engine_weight_bytes(MusaEngineC* engine);
}

class Model;

class MusaEngine {
public:
    MusaEngine(const Model& model, std::string& error);
    ~MusaEngine();
    MusaEngine(const MusaEngine&) = delete;
    MusaEngine& operator=(const MusaEngine&) = delete;

    bool valid() const { return impl_ != nullptr; }
    const std::string& device_name() const { return device_; }
    size_t weight_bytes() const { return weight_bytes_; }

    const std::vector<float>& forward(int token, int pos, std::string& error);

private:
    MusaEngineC* impl_ = nullptr;
    std::vector<float> host_logits_;
    std::string device_;
    size_t weight_bytes_ = 0;
};

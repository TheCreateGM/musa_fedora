// GGUF v3 reader for musa-infer.
//
// This is a focused reader: it parses the metadata and tensor table that the
// Llama-family GGUFs used by this tool need, and leaves tensor payloads in the
// memory-mapped file. It does not decompress or repack weights.
#pragma once

#include <cstdint>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

enum class GgufType : uint32_t {
    F32 = 0, F16 = 1, Q4_0 = 2, Q4_1 = 3, Q5_0 = 6, Q5_1 = 7, Q8_0 = 8,
    Q8_1 = 9, Q2_K = 10, Q3_K = 11, Q4_K = 12, Q5_K = 13, Q6_K = 14,
    Q8_K = 15, IQ2_XXS = 16, IQ2_XS = 17, IQ3_XXS = 18, IQ1_S = 19,
    IQ4_NL = 20, IQ3_S = 21, IQ2_S = 22, IQ4_XS = 23, I8 = 24, I16 = 25,
    I32 = 26, I64 = 27, F64 = 28, IQ1_M = 29, BF16 = 30,
};

struct GgufTensor {
    std::string name;
    std::vector<uint64_t> dims;   // dims[0] is the fastest-varying axis
    GgufType type = GgufType::F32;
    uint64_t offset = 0;          // from the start of the data section
    uint64_t nbytes = 0;
};

struct GgufValue {
    uint32_t type = 0;
    std::string s;
    int64_t i = 0;
    double f = 0.0;
    std::vector<std::string> sa;
    std::vector<int64_t> ia;
    std::vector<double> fa;
};

class GgufFile {
public:
    static GgufFile open(const std::string& path);

    int version() const { return version_; }
    uint64_t alignment() const { return alignment_; }
    const uint8_t* base() const { return base_; }
    size_t size() const { return size_; }
    const std::vector<GgufTensor>& tensors() const { return tensors_; }
    const std::map<std::string, GgufValue>& meta() const { return meta_; }

    const GgufTensor& tensor(const std::string& name) const;
    bool has_tensor(const std::string& name) const;
    const void* tensor_data(const GgufTensor& t) const;

    // Typed accessors that throw with the key name on a missing or wrong-type key.
    int64_t get_int(const std::string& key, int64_t fallback) const;
    double get_float(const std::string& key, double fallback) const;
    std::string get_string(const std::string& key, const std::string& fallback) const;
    const std::vector<std::string>& get_strings(const std::string& key) const;
    const std::vector<int64_t>& get_ints(const std::string& key) const;
    const std::vector<double>& get_floats(const std::string& key) const;

private:
    const uint8_t* base_ = nullptr;
    size_t size_ = 0;
    int version_ = 0;
    uint64_t alignment_ = 32;
    uint64_t data_start_ = 0;
    std::map<std::string, GgufValue> meta_;
    std::vector<GgufTensor> tensors_;
    std::map<std::string, size_t> index_;
};

// Bytes per element for the types this tool can consume. Returns 0 for a type
// it cannot consume, so callers can reject the model with a clear error.
size_t gguf_type_size(GgufType t);
// Number of logical values in a tensor.
uint64_t gguf_nelements(const GgufTensor& t);
// Number of values stored per block and bytes per block for quantized types.
void gguf_block_layout(GgufType t, uint64_t& values_per_block, uint64_t& bytes_per_block);
const char* gguf_type_name(GgufType t);

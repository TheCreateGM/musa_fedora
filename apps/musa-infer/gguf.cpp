#include "gguf.h"

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

namespace {

constexpr uint32_t GGUF_MAGIC = 0x46554747u; // "GGUF" little-endian

[[noreturn]] void fail(const std::string& what) { throw std::runtime_error(what); }

class Reader {
public:
    Reader(const uint8_t* base, size_t size, size_t pos, std::string path)
        : base_(base), size_(size), pos_(pos), path_(std::move(path)) {}

    size_t pos() const { return pos_; }

    void need(size_t n) const {
        if (n > size_ - pos_) fail(path_ + ": truncated GGUF at offset " + std::to_string(pos_));
    }

    template <typename T> T scalar() {
        need(sizeof(T));
        T v;
        std::memcpy(&v, base_ + pos_, sizeof(T));
        pos_ += sizeof(T);
        return v;
    }

    std::string str() {
        uint64_t n = scalar<uint64_t>();
        if (n > (1ull << 32)) fail(path_ + ": implausible GGUF string length");
        need(static_cast<size_t>(n));
        std::string s(reinterpret_cast<const char*>(base_ + pos_), static_cast<size_t>(n));
        pos_ += static_cast<size_t>(n);
        return s;
    }

private:
    const uint8_t* base_;
    size_t size_;
    size_t pos_;
    std::string path_;
};

void read_value(Reader& r, uint32_t type, GgufValue& v);

void read_array(Reader& r, uint32_t elem_type, uint64_t count, GgufValue& v) {
    v.type = 9;
    if (count > (1ull << 28)) fail("implausible GGUF array length");
    for (uint64_t i = 0; i < count; ++i) {
        GgufValue e;
        read_value(r, elem_type, e);
        switch (elem_type) {
        case 8: v.sa.push_back(e.s); break;
        case 0: case 1: case 2: case 3: case 4: case 5: case 10: case 11:
            v.ia.push_back(e.i); break;
        case 6: case 12: v.fa.push_back(e.f); break;
        default: fail("unsupported GGUF array element type " + std::to_string(elem_type));
        }
    }
}

void read_value(Reader& r, uint32_t type, GgufValue& v) {
    v.type = type;
    switch (type) {
    case 0: v.i = r.scalar<uint8_t>(); break;
    case 1: v.i = r.scalar<int8_t>(); break;
    case 2: v.i = r.scalar<uint16_t>(); break;
    case 3: v.i = r.scalar<int16_t>(); break;
    case 4: v.i = r.scalar<uint32_t>(); break;
    case 5: v.i = r.scalar<int32_t>(); break;
    case 6: v.f = r.scalar<float>(); break;
    case 7: v.i = r.scalar<uint8_t>() != 0; break;
    case 8: v.s = r.str(); break;
    case 9: {
        uint32_t elem_type = r.scalar<uint32_t>();
        uint64_t count = r.scalar<uint64_t>();
        read_array(r, elem_type, count, v);
        break;
    }
    case 10: v.i = static_cast<int64_t>(r.scalar<uint64_t>()); break;
    case 11: v.i = r.scalar<int64_t>(); break;
    case 12: v.f = r.scalar<double>(); break;
    default: fail("unsupported GGUF metadata type " + std::to_string(type));
    }
}

uint64_t align_up(uint64_t v, uint64_t a) { return (v + a - 1) / a * a; }

} // namespace

GgufFile GgufFile::open(const std::string& path) {
    int fd = ::open(path.c_str(), O_RDONLY | O_CLOEXEC);
    if (fd < 0) fail("open " + path + ": " + std::strerror(errno));

    struct stat st {};
    if (::fstat(fd, &st) != 0) {
        ::close(fd);
        fail("fstat " + path + ": " + std::strerror(errno));
    }
    if (static_cast<uint64_t>(st.st_size) < 32) {
        ::close(fd);
        fail(path + ": file is too small to be a GGUF file");
    }

    void* map = ::mmap(nullptr, static_cast<size_t>(st.st_size), PROT_READ, MAP_PRIVATE, fd, 0);
    ::close(fd);
    if (map == MAP_FAILED) fail("mmap " + path + ": " + std::strerror(errno));

    GgufFile f;
    f.base_ = static_cast<const uint8_t*>(map);
    f.size_ = static_cast<size_t>(st.st_size);

    Reader r(f.base_, f.size_, 0, path);
    uint32_t magic = r.scalar<uint32_t>();
    if (magic != GGUF_MAGIC) fail(path + ": bad GGUF magic");
    f.version_ = static_cast<int>(r.scalar<uint32_t>());
    if (f.version_ != 3) fail(path + ": unsupported GGUF version " + std::to_string(f.version_) + " (need 3)");
    uint64_t n_tensors = r.scalar<uint64_t>();
    uint64_t n_kv = r.scalar<uint64_t>();
    if (n_tensors > 100000 || n_kv > 1000000) fail(path + ": implausible GGUF counts");

    for (uint64_t i = 0; i < n_kv; ++i) {
        std::string key = r.str();
        uint32_t type = r.scalar<uint32_t>();
        GgufValue v;
        read_value(r, type, v);
        f.meta_[key] = std::move(v);
    }

    auto it = f.meta_.find("general.alignment");
    if (it != f.meta_.end() && it->second.i > 0) f.alignment_ = static_cast<uint64_t>(it->second.i);
    if (f.alignment_ == 0 || (f.alignment_ & (f.alignment_ - 1)) != 0)
        fail(path + ": general.alignment is not a power of two");

    f.tensors_.reserve(static_cast<size_t>(n_tensors));
    for (uint64_t i = 0; i < n_tensors; ++i) {
        GgufTensor t;
        t.name = r.str();
        uint32_t n_dims = r.scalar<uint32_t>();
        if (n_dims == 0 || n_dims > 4) fail(path + ": tensor " + t.name + " has " + std::to_string(n_dims) + " dims");
        t.dims.resize(n_dims);
        for (uint32_t d = 0; d < n_dims; ++d) t.dims[d] = r.scalar<uint64_t>();
        t.type = static_cast<GgufType>(r.scalar<uint32_t>());
        t.offset = r.scalar<uint64_t>();

        uint64_t vpb = 0, bpb = 0;
        gguf_block_layout(t.type, vpb, bpb);
        if (vpb == 0) fail(path + ": tensor " + t.name + " has unsupported type " + gguf_type_name(t.type));
        if (t.dims[0] % vpb != 0) fail(path + ": tensor " + t.name + " width is not a multiple of its block size");
        uint64_t rows = 1;
        for (size_t d = 1; d < t.dims.size(); ++d) rows *= t.dims[d];
        t.nbytes = rows * (t.dims[0] / vpb) * bpb;
        f.index_[t.name] = f.tensors_.size();
        f.tensors_.push_back(std::move(t));
    }

    f.data_start_ = align_up(r.pos(), f.alignment_);
    for (const auto& t : f.tensors_) {
        if (f.data_start_ + t.offset > f.size_ || t.nbytes > f.size_ - (f.data_start_ + t.offset))
            fail(path + ": tensor " + t.name + " runs past the end of the file");
    }
    return f;
}

const GgufTensor& GgufFile::tensor(const std::string& name) const {
    auto it = index_.find(name);
    if (it == index_.end()) fail("GGUF has no tensor named " + name);
    return tensors_[it->second];
}

bool GgufFile::has_tensor(const std::string& name) const { return index_.count(name) != 0; }

const void* GgufFile::tensor_data(const GgufTensor& t) const {
    return base_ + data_start_ + t.offset;
}

int64_t GgufFile::get_int(const std::string& key, int64_t fallback) const {
    auto it = meta_.find(key);
    if (it == meta_.end()) return fallback;
    const GgufValue& v = it->second;
    switch (v.type) {
    case 0: case 1: case 2: case 3: case 4: case 5: case 7: case 10: case 11: return v.i;
    case 6: case 12: return static_cast<int64_t>(v.f);
    default: fail("GGUF key " + key + " is not an integer");
    }
}

double GgufFile::get_float(const std::string& key, double fallback) const {
    auto it = meta_.find(key);
    if (it == meta_.end()) return fallback;
    const GgufValue& v = it->second;
    switch (v.type) {
    case 6: case 12: return v.f;
    case 0: case 1: case 2: case 3: case 4: case 5: case 10: case 11: return static_cast<double>(v.i);
    default: fail("GGUF key " + key + " is not a number");
    }
}

std::string GgufFile::get_string(const std::string& key, const std::string& fallback) const {
    auto it = meta_.find(key);
    if (it == meta_.end()) return fallback;
    if (it->second.type != 8) fail("GGUF key " + key + " is not a string");
    return it->second.s;
}

const std::vector<std::string>& GgufFile::get_strings(const std::string& key) const {
    auto it = meta_.find(key);
    if (it == meta_.end() || it->second.type != 9) fail("GGUF key " + key + " is not a string array");
    return it->second.sa;
}

const std::vector<int64_t>& GgufFile::get_ints(const std::string& key) const {
    auto it = meta_.find(key);
    if (it == meta_.end() || it->second.type != 9) fail("GGUF key " + key + " is not an integer array");
    return it->second.ia;
}

const std::vector<double>& GgufFile::get_floats(const std::string& key) const {
    auto it = meta_.find(key);
    if (it == meta_.end() || it->second.type != 9) fail("GGUF key " + key + " is not a float array");
    return it->second.fa;
}

size_t gguf_type_size(GgufType t) {
    switch (t) {
    case GgufType::F32: return 4;
    case GgufType::F16: return 2;
    case GgufType::BF16: return 2;
    case GgufType::F64: return 8;
    case GgufType::I8: return 1;
    case GgufType::I16: return 2;
    case GgufType::I32: return 4;
    case GgufType::I64: return 8;
    default: return 0;
    }
}

uint64_t gguf_nelements(const GgufTensor& t) {
    uint64_t n = 1;
    for (uint64_t d : t.dims) n *= d;
    return n;
}

void gguf_block_layout(GgufType t, uint64_t& vpb, uint64_t& bpb) {
    vpb = 0;
    bpb = 0;
    switch (t) {
    case GgufType::F32: vpb = 1; bpb = 4; break;
    case GgufType::F16: vpb = 1; bpb = 2; break;
    case GgufType::BF16: vpb = 1; bpb = 2; break;
    case GgufType::F64: vpb = 1; bpb = 8; break;
    case GgufType::I8: vpb = 1; bpb = 1; break;
    case GgufType::I16: vpb = 1; bpb = 2; break;
    case GgufType::I32: vpb = 1; bpb = 4; break;
    case GgufType::I64: vpb = 1; bpb = 8; break;
    case GgufType::Q4_0: vpb = 32; bpb = 18; break;
    case GgufType::Q4_1: vpb = 32; bpb = 20; break;
    case GgufType::Q5_0: vpb = 32; bpb = 22; break;
    case GgufType::Q5_1: vpb = 32; bpb = 24; break;
    case GgufType::Q8_0: vpb = 32; bpb = 34; break;
    case GgufType::Q8_1: vpb = 32; bpb = 36; break;
    case GgufType::Q2_K: vpb = 256; bpb = 84; break;
    case GgufType::Q3_K: vpb = 256; bpb = 110; break;
    case GgufType::Q4_K: vpb = 256; bpb = 144; break;
    case GgufType::Q5_K: vpb = 256; bpb = 176; break;
    case GgufType::Q6_K: vpb = 256; bpb = 210; break;
    case GgufType::Q8_K: vpb = 256; bpb = 292; break;
    default: break;
    }
}

const char* gguf_type_name(GgufType t) {
    switch (t) {
    case GgufType::F32: return "F32";
    case GgufType::F16: return "F16";
    case GgufType::BF16: return "BF16";
    case GgufType::F64: return "F64";
    case GgufType::Q4_0: return "Q4_0";
    case GgufType::Q4_1: return "Q4_1";
    case GgufType::Q5_0: return "Q5_0";
    case GgufType::Q5_1: return "Q5_1";
    case GgufType::Q8_0: return "Q8_0";
    case GgufType::Q8_1: return "Q8_1";
    case GgufType::Q2_K: return "Q2_K";
    case GgufType::Q3_K: return "Q3_K";
    case GgufType::Q4_K: return "Q4_K";
    case GgufType::Q5_K: return "Q5_K";
    case GgufType::Q6_K: return "Q6_K";
    case GgufType::Q8_K: return "Q8_K";
    case GgufType::I8: return "I8";
    case GgufType::I16: return "I16";
    case GgufType::I32: return "I32";
    case GgufType::I64: return "I64";
    default: return "UNKNOWN";
    }
}

// MUSA kernels and C ABI for musa-infer. Compiled by mcc (mp_10).
//
// The MTT S30 has 8 SMs, so launch overhead matters. Q/K/V projections share
// one launch, the FFN gate/up/SwiGLU pass shares one launch, and the two
// residual projections accumulate in place; a decode step is 201 launches
// instead of ~330. Driver release 3.1.0 cannot replay a captured graph of this
// workload (the submission thread segfaults inside libmusa), so the backend
// uses plain launches.
#include "musa_layout.h"

#include <cuda_fp16.h>
#include <cuda_runtime.h>

#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstring>

namespace {

struct Q8Block {
    unsigned short d;
    signed char qs[32];
} __attribute__((packed));

__device__ __forceinline__ float half_bits_to_float(unsigned short h) {
    return __half2float(__ushort_as_half(h));
}

__device__ float warp_sum(float acc) {
    for (int off = 16; off > 0; off >>= 1) acc += __shfl_down_sync(0xffffffffu, acc, off);
    return acc;
}

// Q8_0 payloads are repacked on the device into 32-byte payloads and a packed
// fp16 scale array. In the on-disk layout every 34-byte block puts the 2-byte
// scale in its own cache sector, which costs more bandwidth than the payload;
// separating the arrays makes payload reads exactly one sector per block and
// scales one sector per 16 blocks.
// Four independent accumulator chains per lane keep four payload/scalar loads
// in flight; one accumulator per lane leaves the warp with too few outstanding
// requests to reach the S30's streaming bandwidth. The row total is valid in
// lane 0 after warp_sum in the caller.
__device__ __forceinline__ float q8_row_dot(const unsigned char* __restrict__ qs,
                                            const unsigned short* __restrict__ dsc,
                                            const float* __restrict__ x, int blocks, int lane) {
    const signed char* q = reinterpret_cast<const signed char*>(qs);
    float a0 = 0.f, a1 = 0.f, a2 = 0.f, a3 = 0.f;
    int b = 0;
    for (; b + 3 < blocks; b += 4) {
        a0 += half_bits_to_float(dsc[b + 0]) * static_cast<float>(q[(b + 0) * 32 + lane]) * x[(b + 0) * 32 + lane];
        a1 += half_bits_to_float(dsc[b + 1]) * static_cast<float>(q[(b + 1) * 32 + lane]) * x[(b + 1) * 32 + lane];
        a2 += half_bits_to_float(dsc[b + 2]) * static_cast<float>(q[(b + 2) * 32 + lane]) * x[(b + 2) * 32 + lane];
        a3 += half_bits_to_float(dsc[b + 3]) * static_cast<float>(q[(b + 3) * 32 + lane]) * x[(b + 3) * 32 + lane];
    }
    for (; b < blocks; ++b) {
        a0 += half_bits_to_float(dsc[b]) * static_cast<float>(q[b * 32 + lane]) * x[b * 32 + lane];
    }
    return (a0 + a1) + (a2 + a3);
}

__global__ void rmsnorm_kernel(float* out, const float* x, const float* w, int n, float eps) {
    __shared__ float red[256];
    const int tid = threadIdx.x;
    float sum = 0.f;
    for (int i = tid; i < n; i += blockDim.x) {
        const float v = x[i];
        sum += v * v;
    }
    red[tid] = sum;
    __syncthreads();
    for (int off = blockDim.x / 2; off > 0; off >>= 1) {
        if (tid < off) red[tid] += red[tid + off];
        __syncthreads();
    }
    const float scale = rsqrtf(red[0] / static_cast<float>(n) + eps);
    for (int i = tid; i < n; i += blockDim.x) out[i] = x[i] * scale * w[i];
}

// One warp per output row, reading the repacked payloads described above.
__global__ void gemv_q8_kernel(float* out, const float* x, const unsigned char* qs,
                               const unsigned short* dsc, int rows, int cols) {
    const int lane = threadIdx.x & 31;
    const int warp = (blockIdx.x * blockDim.x + threadIdx.x) >> 5;
    if (warp >= rows) return;
    const int blocks = cols / 32;
    const size_t row = static_cast<size_t>(warp) * blocks;
    const float acc = warp_sum(q8_row_dot(qs + row * 32, dsc + row, x, blocks, lane));
    if (lane == 0) out[warp] = acc;
}

// Q, K and V projections in one launch: rows [0, dim) go to Q, the next kv
// rows to K, the last kv rows to V.
__global__ void gemv_q8_qkv_kernel(float* qout, float* kout, float* vout, const float* x,
                                   const unsigned char* qq, const unsigned short* dq,
                                   const unsigned char* qk, const unsigned short* dk,
                                   const unsigned char* qv, const unsigned short* dv,
                                   int dim, int kv, int cols) {
    const int lane = threadIdx.x & 31;
    const int warp = (blockIdx.x * blockDim.x + threadIdx.x) >> 5;
    if (warp >= dim + 2 * kv) return;
    const unsigned char* qs;
    const unsigned short* dsc;
    float* out;
    int row;
    if (warp < dim) { qs = qq; dsc = dq; out = qout; row = warp; }
    else if (warp < dim + kv) { qs = qk; dsc = dk; out = kout; row = warp - dim; }
    else { qs = qv; dsc = dv; out = vout; row = warp - dim - kv; }
    const int blocks = cols / 32;
    const size_t off = static_cast<size_t>(row) * blocks;
    const float acc = warp_sum(q8_row_dot(qs + off * 32, dsc + off, x, blocks, lane));
    if (lane == 0) out[row] = acc;
}

// Accumulating projection: dst[row] += dot(W[row], x). Used for attn_output and
// ffn_down, which both feed a residual add.
__global__ void gemv_q8_add_kernel(float* dst, const float* x, const unsigned char* qs,
                                   const unsigned short* dsc, int rows, int cols) {
    const int lane = threadIdx.x & 31;
    const int warp = (blockIdx.x * blockDim.x + threadIdx.x) >> 5;
    if (warp >= rows) return;
    const int blocks = cols / 32;
    const size_t row = static_cast<size_t>(warp) * blocks;
    const float acc = warp_sum(q8_row_dot(qs + row * 32, dsc + row, x, blocks, lane));
    if (lane == 0) dst[warp] += acc;
}

// FFN gate and up projections for one row, followed by SwiGLU in the same
// launch: out[row] = silu(gate[row]) * up[row].
__global__ void ffn_swiglu_kernel(float* out, const float* x,
                                  const unsigned char* qg, const unsigned short* dg,
                                  const unsigned char* qu, const unsigned short* du,
                                  int rows, int cols) {
    const int lane = threadIdx.x & 31;
    const int warp = (blockIdx.x * blockDim.x + threadIdx.x) >> 5;
    if (warp >= rows) return;
    const int blocks = cols / 32;
    const size_t off = static_cast<size_t>(warp) * blocks;
    const float ag = warp_sum(q8_row_dot(qg + off * 32, dg + off, x, blocks, lane));
    const float au = warp_sum(q8_row_dot(qu + off * 32, du + off, x, blocks, lane));
    if (lane == 0) {
        const float g = ag;
        out[warp] = (g / (1.f + expf(-g))) * au;
    }
}

__global__ void embed_row_kernel(float* out, const unsigned char* qs, const unsigned short* dsc,
                                 int token, int dim) {
    const int blocks = dim / 32;
    const unsigned char* row = qs + static_cast<size_t>(token) * blocks * 32;
    const unsigned short* drow = dsc + static_cast<size_t>(token) * blocks;
    for (int i = threadIdx.x; i < dim; i += blockDim.x) {
        const int b = i / 32, j = i % 32;
        out[i] = half_bits_to_float(drow[b]) * static_cast<float>(static_cast<signed char>(row[b * 32 + j]));
    }
}

// Split the 34-byte on-disk blocks into a 32-byte payload array and a packed
// fp16 scale array. One thread handles four payload bytes, so a warp reads and
// writes contiguous 128-byte spans.
__global__ void repack_q8_kernel(const unsigned char* __restrict__ src, unsigned char* __restrict__ qs,
                                 unsigned short* __restrict__ dsc, int blocks) {
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    const int chunks = blocks * 8;
    if (i < chunks) {
        const int b = i >> 3, c = (i & 7) * 4;
        const unsigned char* s = src + static_cast<size_t>(b) * 34 + 2 + c;
        unsigned char* d = qs + static_cast<size_t>(b) * 32 + c;
        d[0] = s[0]; d[1] = s[1]; d[2] = s[2]; d[3] = s[3];
    } else if (i < chunks + blocks) {
        const int b = i - chunks;
        dsc[b] = *reinterpret_cast<const unsigned short*>(src + static_cast<size_t>(b) * 34);
    }
}

__global__ void rope_kernel(float* q, float* k, int pos, int n_heads, int n_kv, int hd, int rope_dim, float theta) {
    const int pairs = rope_dim / 2;
    const int total = (n_heads + n_kv) * pairs;
    const int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= total) return;
    const int head = idx / pairs;
    const int i = (idx % pairs) * 2;
    const float freq = powf(theta, -static_cast<float>(i) / static_cast<float>(rope_dim));
    const float angle = static_cast<float>(pos) * freq;
    const float c = cosf(angle), s = sinf(angle);
    float* vec = head < n_heads ? q + static_cast<size_t>(head) * hd + i
                                 : k + static_cast<size_t>(head - n_heads) * hd + i;
    const float a = vec[0], b = vec[1];
    vec[0] = a * c - b * s;
    vec[1] = a * s + b * c;
}

__global__ void store_kv_kernel(float* kc, float* vc, const float* k, const float* v,
                                int pos, int layer, int max_seq, int kv_dim) {
    float* dst_k = kc + (static_cast<size_t>(layer) * max_seq + pos) * kv_dim;
    float* dst_v = vc + (static_cast<size_t>(layer) * max_seq + pos) * kv_dim;
    for (int i = threadIdx.x; i < kv_dim; i += blockDim.x) {
        dst_k[i] = k[i];
        dst_v[i] = v[i];
    }
}

__global__ void attention_kernel(float* out, const float* q, const float* kc, const float* vc,
                                 int pos, int kv_mul, int hd, int layer, int max_seq, int kv_dim) {
    __shared__ float scores[2048];
    const int seqlen = pos + 1;
    const int h = blockIdx.x;
    const int tid = threadIdx.x;
    const float* qv = q + static_cast<size_t>(h) * hd;
    const float* kb = kc + static_cast<size_t>(layer) * max_seq * kv_dim + static_cast<size_t>(h / kv_mul) * hd;
    for (int t = tid; t < seqlen; t += blockDim.x) {
        const float* kt = kb + static_cast<size_t>(t) * kv_dim;
        float dot = 0.f;
        for (int i = 0; i < hd; ++i) dot += qv[i] * kt[i];
        scores[t] = dot * rsqrtf(static_cast<float>(hd));
    }
    __syncthreads();
    if (tid == 0) {
        float max_score = -1e30f;
        for (int t = 0; t < seqlen; ++t) max_score = fmaxf(max_score, scores[t]);
        float sum = 0.f;
        for (int t = 0; t < seqlen; ++t) { scores[t] = expf(scores[t] - max_score); sum += scores[t]; }
        const float inv = 1.f / sum;
        for (int t = 0; t < seqlen; ++t) scores[t] *= inv;
    }
    __syncthreads();
    const float* vb = vc + static_cast<size_t>(layer) * max_seq * kv_dim + static_cast<size_t>(h / kv_mul) * hd;
    for (int d = tid; d < hd; d += blockDim.x) {
        float acc = 0.f;
        for (int t = 0; t < seqlen; ++t) acc += scores[t] * vb[static_cast<size_t>(t) * kv_dim + d];
        out[static_cast<size_t>(h) * hd + d] = acc;
    }
}

int div_up(int a, int b) { return (a + b - 1) / b; }

} // namespace

struct MusaEngineC {
    MusaConfig cfg;
    float** weights;          // Q8 tensors: repacked 32-byte payloads; others: raw data
    unsigned short** scales;  // Q8 tensors: packed fp16 block scales
    int wcount;
    float* x;
    float* xb;
    float* q;
    float* k;
    float* v;
    float* hb;
    float* logits;
    float* kcache;
    float* vcache;
    cudaStream_t stream;
    char device[256];
    size_t weight_bytes;
};

namespace {

void set_error(char* buffer, size_t len, const char* fmt, ...) {
    if (!buffer || len == 0) return;
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buffer, len, fmt, ap);
    va_end(ap);
}

bool alloc_f32(float** ptr, size_t count, char* error, size_t error_len) {
    if (cudaMalloc(reinterpret_cast<void**>(ptr), count * sizeof(float)) != cudaSuccess) {
        set_error(error, error_len, "cudaMalloc failed: %s", cudaGetErrorString(cudaGetLastError()));
        return false;
    }
    return true;
}

void launch_sequence(MusaEngineC* e, int token, int pos) {
    const MusaConfig& c = e->cfg;
    const int dim = c.dim, kv = c.n_kv_heads * c.head_dim, hd = c.head_dim;
    const int kv_mul = c.n_heads / c.n_kv_heads;
    cudaStream_t s = e->stream;

    embed_row_kernel<<<1, 256, 0, s>>>(e->x, reinterpret_cast<const unsigned char*>(e->weights[MUSA_EMB]),
                                       e->scales[MUSA_EMB], token, dim);

    for (int l = 0; l < c.n_layers; ++l) {
        float** w = e->weights + MUSA_LAYER_BASE + l * MUSA_LAYER_TENSORS;
        rmsnorm_kernel<<<1, 256, 0, s>>>(e->xb, e->x, w[MUSA_ATTN_NORM], dim, c.rms_eps);
        unsigned short** d = e->scales + MUSA_LAYER_BASE + l * MUSA_LAYER_TENSORS;
        gemv_q8_qkv_kernel<<<div_up(dim + 2 * kv, 8), 256, 0, s>>>(
            e->q, e->k, e->v, e->xb,
            reinterpret_cast<const unsigned char*>(w[MUSA_ATTN_Q]), d[MUSA_ATTN_Q],
            reinterpret_cast<const unsigned char*>(w[MUSA_ATTN_K]), d[MUSA_ATTN_K],
            reinterpret_cast<const unsigned char*>(w[MUSA_ATTN_V]), d[MUSA_ATTN_V], dim, kv, dim);
        rope_kernel<<<div_up((c.n_heads + c.n_kv_heads) * (c.rope_dim / 2), 128), 128, 0, s>>>(
            e->q, e->k, pos, c.n_heads, c.n_kv_heads, hd, c.rope_dim, c.rope_theta);
        store_kv_kernel<<<1, 256, 0, s>>>(e->kcache, e->vcache, e->k, e->v, pos, l, c.seq_len, kv);
        attention_kernel<<<c.n_heads, 128, 0, s>>>(e->xb, e->q, e->kcache, e->vcache, pos, kv_mul, hd,
                                                    l, c.seq_len, kv);
        gemv_q8_add_kernel<<<div_up(dim, 8), 256, 0, s>>>(
            e->x, e->xb, reinterpret_cast<const unsigned char*>(w[MUSA_ATTN_OUT]), d[MUSA_ATTN_OUT], dim, dim);

        rmsnorm_kernel<<<1, 256, 0, s>>>(e->xb, e->x, w[MUSA_FFN_NORM], dim, c.rms_eps);
        ffn_swiglu_kernel<<<div_up(c.hidden_dim, 8), 256, 0, s>>>(
            e->hb, e->xb, reinterpret_cast<const unsigned char*>(w[MUSA_FFN_GATE]), d[MUSA_FFN_GATE],
            reinterpret_cast<const unsigned char*>(w[MUSA_FFN_UP]), d[MUSA_FFN_UP], c.hidden_dim, dim);
        gemv_q8_add_kernel<<<div_up(dim, 8), 256, 0, s>>>(
            e->x, e->hb, reinterpret_cast<const unsigned char*>(w[MUSA_FFN_DOWN]), d[MUSA_FFN_DOWN], dim, c.hidden_dim);
    }

    rmsnorm_kernel<<<1, 256, 0, s>>>(e->x, e->x, e->weights[MUSA_NORM], dim, c.rms_eps);
    const int out_slot = c.has_output_weight ? MUSA_OUT : MUSA_EMB;
    gemv_q8_kernel<<<div_up(c.vocab_size, 8), 256, 0, s>>>(
        e->logits, e->x, reinterpret_cast<const unsigned char*>(e->weights[out_slot]), e->scales[out_slot],
        c.vocab_size, dim);
}

} // namespace

extern "C" {

void musa_engine_destroy(MusaEngineC* engine);

MusaEngineC* musa_engine_create(const MusaTensor* tensors, int count, const MusaConfig* cfg,
                                char* error, size_t error_len) {
    if (!tensors || !cfg || count <= 0) { set_error(error, error_len, "invalid backend arguments"); return nullptr; }
    const int expected = MUSA_LAYER_BASE + cfg->n_layers * MUSA_LAYER_TENSORS;
    if (count != expected) { set_error(error, error_len, "expected %d tensors, got %d", expected, count); return nullptr; }

    MusaEngineC* e = static_cast<MusaEngineC*>(calloc(1, sizeof(MusaEngineC)));
    if (!e) { set_error(error, error_len, "out of memory"); return nullptr; }
    e->cfg = *cfg;
    e->wcount = count;
    e->weights = static_cast<float**>(calloc(static_cast<size_t>(count), sizeof(float*)));
    e->scales = static_cast<unsigned short**>(calloc(static_cast<size_t>(count), sizeof(unsigned short*)));
    if (!e->weights || !e->scales) { set_error(error, error_len, "out of memory"); musa_engine_destroy(e); return nullptr; }

    cudaDeviceProp prop {};
    if (cudaGetDeviceProperties(&prop, 0) != cudaSuccess) {
        set_error(error, error_len, "cudaGetDeviceProperties failed: %s", cudaGetErrorString(cudaGetLastError()));
        musa_engine_destroy(e);
        return nullptr;
    }
    strncpy(e->device, prop.name, sizeof(e->device) - 1);

    for (int i = 0; i < count; ++i) {
        const MusaTensor& t = tensors[i];
        if (!t.data) continue; // optional output.weight
        if (t.type != MUSA_TYPE_F32 && t.type != MUSA_TYPE_F16 && t.type != MUSA_TYPE_Q8_0) {
            set_error(error, error_len, "unsupported tensor type %d in %s", t.type, t.name ? t.name : "?");
            musa_engine_destroy(e);
            return nullptr;
        }
        e->weight_bytes += t.nbytes;
        if (t.type != MUSA_TYPE_Q8_0) {
            float* device_ptr = nullptr;
            if (cudaMalloc(reinterpret_cast<void**>(&device_ptr), t.nbytes) != cudaSuccess) {
                set_error(error, error_len, "cudaMalloc of %s failed: %s", t.name ? t.name : "?",
                          cudaGetErrorString(cudaGetLastError()));
                musa_engine_destroy(e);
                return nullptr;
            }
            if (cudaMemcpy(device_ptr, t.data, t.nbytes, cudaMemcpyHostToDevice) != cudaSuccess) {
                cudaFree(device_ptr);
                set_error(error, error_len, "upload of %s failed: %s", t.name ? t.name : "?",
                          cudaGetErrorString(cudaGetLastError()));
                musa_engine_destroy(e);
                return nullptr;
            }
            e->weights[i] = device_ptr;
            continue;
        }

        if (t.nbytes % 34 != 0) {
            set_error(error, error_len, "%s is not a whole number of Q8_0 blocks", t.name ? t.name : "?");
            musa_engine_destroy(e);
            return nullptr;
        }
        const int blocks = static_cast<int>(t.nbytes / 34);
        void* raw = nullptr;
        unsigned char* payload = nullptr;
        unsigned short* scale = nullptr;
        if (cudaMalloc(&raw, t.nbytes) != cudaSuccess) {
            set_error(error, error_len, "cudaMalloc of %s failed: %s", t.name ? t.name : "?",
                      cudaGetErrorString(cudaGetLastError()));
            musa_engine_destroy(e);
            return nullptr;
        }
        if (cudaMemcpy(raw, t.data, t.nbytes, cudaMemcpyHostToDevice) != cudaSuccess) {
            cudaFree(raw);
            set_error(error, error_len, "upload of %s failed: %s", t.name ? t.name : "?",
                      cudaGetErrorString(cudaGetLastError()));
            musa_engine_destroy(e);
            return nullptr;
        }
        if (cudaMalloc(reinterpret_cast<void**>(&payload), static_cast<size_t>(blocks) * 32) != cudaSuccess ||
            cudaMalloc(reinterpret_cast<void**>(&scale), static_cast<size_t>(blocks) * 2) != cudaSuccess) {
            if (payload) cudaFree(payload);
            if (scale) cudaFree(scale);
            cudaFree(raw);
            set_error(error, error_len, "cudaMalloc of the repacked %s failed: %s", t.name ? t.name : "?",
                      cudaGetErrorString(cudaGetLastError()));
            musa_engine_destroy(e);
            return nullptr;
        }
        repack_q8_kernel<<<div_up(blocks * 9, 256), 256>>>(
            static_cast<const unsigned char*>(raw), payload, scale, blocks);
        cudaError_t repack_err = cudaDeviceSynchronize();
        cudaFree(raw);
        if (repack_err != cudaSuccess) {
            cudaFree(payload);
            cudaFree(scale);
            set_error(error, error_len, "repack of %s failed: %s", t.name ? t.name : "?",
                      cudaGetErrorString(repack_err));
            musa_engine_destroy(e);
            return nullptr;
        }
        e->weights[i] = reinterpret_cast<float*>(payload);
        e->scales[i] = scale;
    }

    const size_t kv = static_cast<size_t>(cfg->n_kv_heads) * cfg->head_dim;
    const size_t cache = static_cast<size_t>(cfg->n_layers) * cfg->seq_len * kv;
    if (!alloc_f32(&e->x, cfg->dim, error, error_len) || !alloc_f32(&e->xb, cfg->dim, error, error_len) ||
        !alloc_f32(&e->q, cfg->dim, error, error_len) || !alloc_f32(&e->k, kv, error, error_len) ||
        !alloc_f32(&e->v, kv, error, error_len) || !alloc_f32(&e->hb, cfg->hidden_dim, error, error_len) ||
        !alloc_f32(&e->logits, cfg->vocab_size, error, error_len) ||
        !alloc_f32(&e->kcache, cache, error, error_len) || !alloc_f32(&e->vcache, cache, error, error_len)) {
        musa_engine_destroy(e);
        return nullptr;
    }
    if (cudaStreamCreate(&e->stream) != cudaSuccess) {
        set_error(error, error_len, "cudaStreamCreate failed: %s", cudaGetErrorString(cudaGetLastError()));
        musa_engine_destroy(e);
        return nullptr;
    }
    return e;
}

int musa_engine_forward(MusaEngineC* e, int token, int pos, float* logits, char* error, size_t error_len) {
    if (!e || !logits) { set_error(error, error_len, "backend is not initialised"); return -1; }
    const MusaConfig& c = e->cfg;
    if (token < 0 || token >= c.vocab_size || pos < 0 || pos >= c.seq_len) {
        set_error(error, error_len, "token or position out of range");
        return -1;
    }

    launch_sequence(e, token, pos);
    cudaError_t err = cudaStreamSynchronize(e->stream);
    if (err != cudaSuccess) { set_error(error, error_len, "kernel execution failed: %s", cudaGetErrorString(err)); return -1; }
    err = cudaMemcpy(logits, e->logits, sizeof(float) * c.vocab_size, cudaMemcpyDeviceToHost);
    if (err != cudaSuccess) { set_error(error, error_len, "logits copy failed: %s", cudaGetErrorString(err)); return -1; }
    return 0;
}

void musa_engine_destroy(MusaEngineC* e) {
    if (!e) return;
    if (e->stream) cudaStreamDestroy(e->stream);
    if (e->weights) {
        for (int i = 0; i < e->wcount; ++i) {
            if (e->weights[i]) cudaFree(e->weights[i]);
        }
        free(e->weights);
    }
    if (e->scales) {
        for (int i = 0; i < e->wcount; ++i) {
            if (e->scales[i]) cudaFree(e->scales[i]);
        }
        free(e->scales);
    }
    cudaFree(e->x); cudaFree(e->xb); cudaFree(e->q); cudaFree(e->k); cudaFree(e->v);
    cudaFree(e->hb); cudaFree(e->logits); cudaFree(e->kcache); cudaFree(e->vcache);
    free(e);
}

const char* musa_engine_device(MusaEngineC* e) { return e ? e->device : ""; }
size_t musa_engine_weight_bytes(MusaEngineC* e) { return e ? e->weight_bytes : 0; }

} // extern "C"

/*
 * Additional MUSA compute checks for the MTT S30 (mp_10).
 *
 * The live device reports warpSize 128, sharedMemPerBlock 28 KiB,
 * maxThreadsPerBlock 1024, and totalConstMem 8192. These kernels stay
 * inside those limits. They do not use cooperative launch.
 *
 * Build and run with scripts/test-compute.sh. Link libmusart 1.5.4.
 * The host prints PASS only after it checks each result.
 */
#include <musa_runtime.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define CHECK(call)                                                          \
    do {                                                                     \
        musaError_t e_ = (call);                                             \
        if (e_ != musaSuccess) {                                             \
            fprintf(stderr, "%s:%d %s -> %d (%s)\n", __FILE__, __LINE__,     \
                    #call, (int)e_, musaGetErrorString(e_));                 \
            return EXIT_FAILURE;                                            \
        }                                                                    \
    } while (0)

enum { BLOCK = 128, SAXPY_N = 1 << 20, REDUCE_N = 1 << 16, ATOMIC_N = 100000 };

__global__ void saxpy(float a, const float *x, float *y, int n)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n)
        y[i] = a * x[i] + y[i];
}

__global__ void reduce_block(const float *in, float *partial, int n)
{
    __shared__ float tile[BLOCK];
    int tid = threadIdx.x;
    int i = blockIdx.x * blockDim.x + tid;
    tile[tid] = (i < n) ? in[i] : 0.0f;
    __syncthreads();
    for (int stride = blockDim.x >> 1; stride > 0; stride >>= 1) {
        if (tid < stride)
            tile[tid] += tile[tid + stride];
        __syncthreads();
    }
    if (tid == 0)
        partial[blockIdx.x] = tile[0];
}

__global__ void atomic_count(int *counter, int n)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n)
        atomicAdd(counter, 1);
}

__constant__ float cscale[4];

__global__ void scale_const(float *out, int n)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n)
        out[i] = cscale[i & 3] * (float)i;
}

__global__ void kmath(float *out)
{
    if (threadIdx.x == 0 && blockIdx.x == 0) {
        out[0] = sinf(0.0f);
        out[1] = cosf(0.0f);
        out[2] = expf(0.0f);
        out[3] = sqrtf(4.0f);
    }
}

__global__ void fill2d(int *out, int width, int height)
{
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;
    if (x < width && y < height)
        out[y * width + x] = y * width + x;
}

static int near_eq(float got, float expect, float tol)
{
    float d = got - expect;
    if (d < 0.0f)
        d = -d;
    return d <= tol;
}

int main(void)
{
    int devices = 0;
    size_t free_b = 0, total_b = 0;
    int failed = 0;

    CHECK(musaGetDeviceCount(&devices));
    if (devices < 1) {
        fprintf(stderr, "no MUSA device\n");
        return EXIT_FAILURE;
    }
    CHECK(musaMemGetInfo(&free_b, &total_b));
    printf("devices=%d mem_free=%zu mem_total=%zu\n", devices, free_b, total_b);

    /* SAXPY: y = 2*x + y, x[i] = i, y[i] = 1 -> expect 2*i+1 */
    {
        const int n = SAXPY_N;
        const size_t bytes = (size_t)n * sizeof(float);
        float *hx = (float *)malloc(bytes);
        float *hy = (float *)malloc(bytes);
        float *dx = NULL, *dy = NULL;
        int i, mismatch = 0;
        if (!hx || !hy)
            return EXIT_FAILURE;
        for (i = 0; i < n; i++) {
            hx[i] = (float)i;
            hy[i] = 1.0f;
        }
        CHECK(musaMalloc((void **)&dx, bytes));
        CHECK(musaMalloc((void **)&dy, bytes));
        CHECK(musaMemcpy(dx, hx, bytes, musaMemcpyHostToDevice));
        CHECK(musaMemcpy(dy, hy, bytes, musaMemcpyHostToDevice));
        saxpy<<<(n + BLOCK - 1) / BLOCK, BLOCK>>>(2.0f, dx, dy, n);
        CHECK(musaGetLastError());
        CHECK(musaDeviceSynchronize());
        CHECK(musaMemcpy(hy, dy, bytes, musaMemcpyDeviceToHost));
        for (i = 0; i < n; i++) {
            if (hy[i] != 2.0f * (float)i + 1.0f) {
                fprintf(stderr, "saxpy mismatch at %d: %f\n", i, hy[i]);
                mismatch = 1;
                break;
            }
        }
        printf("COMPUTE saxpy %s\n", mismatch ? "FAIL" : "PASS");
        failed += mismatch;
        CHECK(musaFree(dx));
        CHECK(musaFree(dy));
        free(hx);
        free(hy);
    }

    /* Shared-memory reduction of REDUCE_N ones. */
    {
        const int n = REDUCE_N;
        const int blocks = (n + BLOCK - 1) / BLOCK;
        const size_t in_bytes = (size_t)n * sizeof(float);
        const size_t part_bytes = (size_t)blocks * sizeof(float);
        float *hin = (float *)malloc(in_bytes);
        float *hpart = (float *)malloc(part_bytes);
        float *din = NULL, *dpart = NULL;
        double sum = 0.0;
        int i, mismatch = 0;
        if (!hin || !hpart)
            return EXIT_FAILURE;
        for (i = 0; i < n; i++)
            hin[i] = 1.0f;
        CHECK(musaMalloc((void **)&din, in_bytes));
        CHECK(musaMalloc((void **)&dpart, part_bytes));
        CHECK(musaMemcpy(din, hin, in_bytes, musaMemcpyHostToDevice));
        reduce_block<<<blocks, BLOCK>>>(din, dpart, n);
        CHECK(musaGetLastError());
        CHECK(musaDeviceSynchronize());
        CHECK(musaMemcpy(hpart, dpart, part_bytes, musaMemcpyDeviceToHost));
        for (i = 0; i < blocks; i++)
            sum += (double)hpart[i];
        if (sum != (double)n) {
            fprintf(stderr, "reduce sum %.1f != %d\n", sum, n);
            mismatch = 1;
        }
        printf("COMPUTE shared_reduce %s\n", mismatch ? "FAIL" : "PASS");
        failed += mismatch;
        CHECK(musaFree(din));
        CHECK(musaFree(dpart));
        free(hin);
        free(hpart);
    }

    /* Integer atomicAdd. One device-wide counter. */
    {
        const int n = ATOMIC_N;
        int *h = (int *)malloc(sizeof(int));
        int *d = NULL;
        int mismatch = 0;
        if (!h)
            return EXIT_FAILURE;
        *h = 0;
        CHECK(musaMalloc((void **)&d, sizeof(int)));
        CHECK(musaMemcpy(d, h, sizeof(int), musaMemcpyHostToDevice));
        atomic_count<<<(n + BLOCK - 1) / BLOCK, BLOCK>>>(d, n);
        CHECK(musaGetLastError());
        CHECK(musaDeviceSynchronize());
        CHECK(musaMemcpy(h, d, sizeof(int), musaMemcpyDeviceToHost));
        if (*h != n) {
            fprintf(stderr, "atomic count %d != %d\n", *h, n);
            mismatch = 1;
        }
        printf("COMPUTE atomic_add %s\n", mismatch ? "FAIL" : "PASS");
        failed += mismatch;
        CHECK(musaFree(d));
        free(h);
    }

    /* __constant__ memory, 16 bytes, inside the 8192-byte S30 limit. */
    {
        const int n = 1024;
        const float host_scale[4] = {1.0f, 2.0f, 3.0f, 4.0f};
        const size_t bytes = (size_t)n * sizeof(float);
        float *h = (float *)malloc(bytes);
        float *d = NULL;
        int i, mismatch = 0;
        if (!h)
            return EXIT_FAILURE;
        CHECK(musaMemcpyToSymbol(cscale, host_scale, sizeof(host_scale), 0,
                                 musaMemcpyHostToDevice));
        CHECK(musaMalloc((void **)&d, bytes));
        scale_const<<<(n + BLOCK - 1) / BLOCK, BLOCK>>>(d, n);
        CHECK(musaGetLastError());
        CHECK(musaDeviceSynchronize());
        CHECK(musaMemcpy(h, d, bytes, musaMemcpyDeviceToHost));
        for (i = 0; i < n; i++) {
            float expect = host_scale[i & 3] * (float)i;
            if (h[i] != expect) {
                fprintf(stderr, "const mismatch at %d: %f != %f\n", i, h[i], expect);
                mismatch = 1;
                break;
            }
        }
        printf("COMPUTE constant_mem %s\n", mismatch ? "FAIL" : "PASS");
        failed += mismatch;
        CHECK(musaFree(d));
        free(h);
    }

    /* Device libm: exact values at 0 and a perfect square. */
    {
        float h[4] = {-1.0f, -1.0f, -1.0f, -1.0f};
        float *d = NULL;
        int mismatch = 0;
        CHECK(musaMalloc((void **)&d, sizeof(h)));
        kmath<<<1, BLOCK>>>(d);
        CHECK(musaGetLastError());
        CHECK(musaDeviceSynchronize());
        CHECK(musaMemcpy(h, d, sizeof(h), musaMemcpyDeviceToHost));
        if (!near_eq(h[0], 0.0f, 1e-6f) || !near_eq(h[1], 1.0f, 1e-6f) ||
            !near_eq(h[2], 1.0f, 1e-6f) || !near_eq(h[3], 2.0f, 1e-6f)) {
            fprintf(stderr, "math got %f %f %f %f\n", h[0], h[1], h[2], h[3]);
            mismatch = 1;
        }
        printf("COMPUTE device_math %s\n", mismatch ? "FAIL" : "PASS");
        failed += mismatch;
        CHECK(musaFree(d));
    }

    /* Two-dimensional launch. */
    {
        const int width = 64, height = 32, n = width * height;
        const size_t bytes = (size_t)n * sizeof(int);
        int *h = (int *)malloc(bytes);
        int *d = NULL;
        dim3 block(16, 8, 1);
        dim3 grid((width + block.x - 1) / block.x, (height + block.y - 1) / block.y, 1);
        int i, mismatch = 0;
        if (!h)
            return EXIT_FAILURE;
        CHECK(musaMalloc((void **)&d, bytes));
        fill2d<<<grid, block>>>(d, width, height);
        CHECK(musaGetLastError());
        CHECK(musaDeviceSynchronize());
        CHECK(musaMemcpy(h, d, bytes, musaMemcpyDeviceToHost));
        for (i = 0; i < n; i++) {
            if (h[i] != i) {
                fprintf(stderr, "fill2d mismatch at %d: %d\n", i, h[i]);
                mismatch = 1;
                break;
            }
        }
        printf("COMPUTE grid2d %s\n", mismatch ? "FAIL" : "PASS");
        failed += mismatch;
        CHECK(musaFree(d));
        free(h);
    }

    /* Device-to-device copy. */
    {
        const int n = 4096;
        const size_t bytes = (size_t)n * sizeof(float);
        float *h = (float *)malloc(bytes);
        float *src = NULL, *dst = NULL;
        int i, mismatch = 0;
        if (!h)
            return EXIT_FAILURE;
        for (i = 0; i < n; i++)
            h[i] = (float)(i + 3);
        CHECK(musaMalloc((void **)&src, bytes));
        CHECK(musaMalloc((void **)&dst, bytes));
        CHECK(musaMemcpy(src, h, bytes, musaMemcpyHostToDevice));
        CHECK(musaMemset(dst, 0x7f, bytes));
        CHECK(musaMemcpy(dst, src, bytes, musaMemcpyDeviceToDevice));
        CHECK(musaMemcpy(h, dst, bytes, musaMemcpyDeviceToHost));
        for (i = 0; i < n; i++) {
            if (h[i] != (float)(i + 3)) {
                fprintf(stderr, "d2d mismatch at %d: %f\n", i, h[i]);
                mismatch = 1;
                break;
            }
        }
        printf("COMPUTE d2d_copy %s\n", mismatch ? "FAIL" : "PASS");
        failed += mismatch;
        CHECK(musaFree(src));
        CHECK(musaFree(dst));
        free(h);
    }

    if (failed) {
        printf("COMPUTE_TEST: FAIL (%d)\n", failed);
        return EXIT_FAILURE;
    }
    printf("COMPUTE_TEST: PASS\n");
    return EXIT_SUCCESS;
}

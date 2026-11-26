// Unified memory + half precision, plain CUDA source.
#include <cuda_runtime.h>
#include <cuda_fp16.h>
#include <stdio.h>
#include <math.h>
#define N 4096
__global__ void hadd(const __half *a, const __half *b, __half *c, int n)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n) c[i] = __hadd(a[i], b[i]);
}
__global__ void dot2(const __half2 *a, const __half2 *b, float *out, int n)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n) {
        __half2 p = __hmul2(a[i], b[i]);
        float2 f = __half22float2(p);
        atomicAdd(out, f.x + f.y);
    }
}
int main(void)
{
    __half *a, *b, *c;
    float *s;
    if (cudaMallocManaged(&a, N * sizeof(__half))) { printf("FP16_FAIL um\n"); return 1; }
    cudaMallocManaged(&b, N * sizeof(__half));
    cudaMallocManaged(&c, N * sizeof(__half));
    cudaMallocManaged(&s, sizeof(float));
    *s = 0.0f;
    for (int i = 0; i < N; i++) { a[i] = __float2half(1.0f); b[i] = __float2half(0.5f); }
    hadd<<<N / 128, 128>>>(a, b, c, N);
    if (cudaDeviceSynchronize()) { printf("FP16_FAIL sync\n"); return 1; }
    float v = __half2float(c[100]);
    __half2 *h2a = (__half2 *)a, *h2b = (__half2 *)b;
    dot2<<<(N / 2) / 128, 128>>>(h2a, h2b, s, N / 2);
    cudaDeviceSynchronize();
    printf("FP16 single=%.4f dot=%.2f (expect 1.5 and %.2f)\n", v, *s, N * 0.5);
    int ok = fabsf(v - 1.5f) < 1e-3f && fabsf(*s - N * 0.5f) < 1.0f;
    cudaFree(a); cudaFree(b); cudaFree(c); cudaFree(s);
    printf(ok ? "FP16_STATUS: PASS\n" : "FP16_STATUS: FAIL\n");
    return ok ? 0 : 1;
}

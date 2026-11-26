// Verbatim CUDA source: builds with nvcc on NVIDIA and with musa-nvcc here.
#include <cuda_runtime.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define N (1 << 22)
#define BLOCK 128

__global__ void vadd(const float *a, const float *b, float *c, int n)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n) c[i] = a[i] + b[i];
}

static const char *err(cudaError_t e) { return cudaGetErrorString(e); }

int main(void)
{
    int dev = 0, count = 0;
    if (cudaGetDeviceCount(&count) != cudaSuccess || count < 1) {
        printf("VADD_FAIL no device\n"); return 1;
    }
    cudaSetDevice(dev);
    cudaDeviceProp prop;
    if (cudaGetDeviceProperties(&prop, dev) != cudaSuccess) {
        printf("VADD_FAIL props\n"); return 1;
    }
    printf("DEVP name=%s SMs=%d cc=%d.%d warp=%d shared=%zu\n",
           prop.name, prop.multiProcessorCount, prop.major, prop.minor,
           prop.warpSize, prop.sharedMemPerBlock);

    size_t sz = (size_t)N * sizeof(float);
    float *h = (float *)malloc(sz), *a = NULL, *b = NULL, *c = NULL;
    for (int i = 0; i < N; i++) h[i] = (float)i;
    cudaError_t e;
    e = cudaMalloc(&a, sz); if (e) { printf("VADD_FAIL %s\n", err(e)); return 1; }
    cudaMalloc(&b, sz); cudaMalloc(&c, sz);
    cudaMemcpy(a, h, sz, cudaMemcpyHostToDevice);
    cudaMemcpy(b, h, sz, cudaMemcpyHostToDevice);
    vadd<<<(N + BLOCK - 1) / BLOCK, BLOCK>>>(a, b, c, N);
    e = cudaGetLastError(); if (e) { printf("VADD_FAIL launch %s\n", err(e)); return 1; }
    cudaDeviceSynchronize();
    float *out = (float *)malloc(sz);
    cudaMemcpy(out, c, sz, cudaMemcpyDeviceToHost);
    int bad = 0;
    for (int i = 0; i < N; i++) if (fabsf(out[i] - 2.0f * (float)i) > 1e-3f) bad++;
    printf("VADD bad=%d of %d\n", bad, N);
    cudaFree(a); cudaFree(b); cudaFree(c); free(h); free(out);
    printf(bad == 0 ? "VADD_STATUS: PASS\n" : "VADD_STATUS: FAIL\n");
    return bad == 0 ? 0 : 1;
}

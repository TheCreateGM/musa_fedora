// Streams, events, pinned memory and async copies, plain CUDA source.
#include <cuda_runtime.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#define N (1 << 20)
__global__ void scale(float *x, float k, int n)
{ int i = blockIdx.x * blockDim.x + threadIdx.x; if (i < n) x[i] *= k; }
int main(void)
{
    float *h = NULL, *d1 = NULL, *d2 = NULL;
    cudaError_t e = cudaMallocHost((void **)&h, N * sizeof(float));
    if (e) { printf("STREAM_FAIL pinned %d\n", (int)e); return 1; }
    cudaMalloc(&d1, N * sizeof(float)); cudaMalloc(&d2, N * sizeof(float));
    for (int i = 0; i < N; i++) h[i] = 1.0f;
    cudaStream_t s[2]; cudaEvent_t t0, t1;
    for (int i = 0; i < 2; i++) {
        if ((e = cudaStreamCreate(&s[i]))) { printf("STREAM_FAIL create %d\n", (int)e); return 1; }
    }
    cudaEventCreate(&t0); cudaEventCreate(&t1);
    cudaEventRecord(t0, s[0]);
    cudaMemcpyAsync(d1, h, N * sizeof(float), cudaMemcpyHostToDevice, s[0]);
    scale<<<N / 128, 128, 0, s[0]>>>(d1, 2.0f, N);
    cudaMemcpyAsync(d2, d1, N * sizeof(float), cudaMemcpyDeviceToDevice, s[0]);
    cudaEventRecord(t1, s[0]);
    cudaEventSynchronize(t1);
    float ms = 0; cudaEventElapsedTime(&ms, t0, t1);
    cudaStreamSynchronize(s[0]);
    float probe = 0;
    cudaMemcpyAsync(h, d2, sizeof(float), cudaMemcpyDeviceToHost, s[0]);
    cudaStreamSynchronize(s[0]);
    probe = h[0];
    printf("STREAM elapsed=%.3fms probe=%.1f mem_info: ", ms, probe);
    size_t fr = 0, tot = 0;
    cudaMemGetInfo(&fr, &tot);
    printf("free=%zuMiB total=%zuMiB\n", fr >> 20, tot >> 20);
    cudaStreamDestroy(s[0]); cudaStreamDestroy(s[1]);
    cudaEventDestroy(t0); cudaEventDestroy(t1);
    cudaFreeHost(h); cudaFree(d1); cudaFree(d2);
    printf(probe == 2.0f ? "STREAM_STATUS: PASS\n" : "STREAM_STATUS: FAIL\n");
    return probe == 2.0f ? 0 : 1;
}

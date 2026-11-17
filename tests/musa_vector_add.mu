#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <musa_runtime.h>

__global__ void vectorAdd(const float *a, const float *b, float *c, int n) {
    int i = blockDim.x * blockIdx.x + threadIdx.x;
    if (i < n) {
        c[i] = a[i] + b[i];
    }
}

int main(void) {
    const int n = 1 << 20;
    const size_t bytes = (size_t)n * sizeof(float);
    float *host_a = NULL;
    float *host_b = NULL;
    float *host_c = NULL;
    float *device_a = NULL;
    float *device_b = NULL;
    float *device_c = NULL;
    musaError_t error = musaSuccess;
    int result = 1;
    const int threads = 256;
    const int blocks = (n + threads - 1) / threads;

    host_a = (float *)malloc(bytes);
    host_b = (float *)malloc(bytes);
    host_c = (float *)malloc(bytes);
    if (!host_a || !host_b || !host_c) {
        fprintf(stderr, "host allocation failed\n");
        goto cleanup;
    }

    for (int i = 0; i < n; ++i) {
        host_a[i] = 1.0f;
        host_b[i] = 2.0f;
        host_c[i] = 0.0f;
    }

#define MUSA_CHECK(call)                                                        \
    do {                                                                        \
        error = (call);                                                         \
        if (error != musaSuccess) {                                             \
            fprintf(stderr, "%s failed with MUSA error %d\n", #call, (int)error); \
            goto cleanup;                                                       \
        }                                                                       \
    } while (0)

    MUSA_CHECK(musaMalloc((void **)&device_a, bytes));
    MUSA_CHECK(musaMalloc((void **)&device_b, bytes));
    MUSA_CHECK(musaMalloc((void **)&device_c, bytes));
    MUSA_CHECK(musaMemcpy(device_a, host_a, bytes, musaMemcpyHostToDevice));
    MUSA_CHECK(musaMemcpy(device_b, host_b, bytes, musaMemcpyHostToDevice));

    vectorAdd<<<blocks, threads>>>(device_a, device_b, device_c, n);
    MUSA_CHECK(musaGetLastError());
    MUSA_CHECK(musaDeviceSynchronize());
    MUSA_CHECK(musaMemcpy(host_c, device_c, bytes, musaMemcpyDeviceToHost));

    for (int i = 0; i < n; ++i) {
        if (fabsf(host_c[i] - 3.0f) > 1.0e-5f) {
            fprintf(stderr, "incorrect result at index %d: %f\n", i, host_c[i]);
            result = 2;
            goto cleanup;
        }
    }

    printf("MUSA_VECTOR_ADD_OK elements=%d\n", n);
    result = 0;

cleanup:
    if (device_c) {
        musaError_t free_error = musaFree(device_c);
        if (free_error != musaSuccess) {
            fprintf(stderr, "musaFree(device_c) failed with error %d\n", (int)free_error);
            result = 3;
        }
    }
    if (device_b) {
        musaError_t free_error = musaFree(device_b);
        if (free_error != musaSuccess) {
            fprintf(stderr, "musaFree(device_b) failed with error %d\n", (int)free_error);
            result = 3;
        }
    }
    if (device_a) {
        musaError_t free_error = musaFree(device_a);
        if (free_error != musaSuccess) {
            fprintf(stderr, "musaFree(device_a) failed with error %d\n", (int)free_error);
            result = 3;
        }
    }
    free(host_c);
    free(host_b);
    free(host_a);
    return result;
}

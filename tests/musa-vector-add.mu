/*
 * MUSA vector-add hardware test.
 *
 * Build (mp_10 is the S30 / SuDi architecture):
 *   mcc -O2 --offload-arch=mp_10 -I/usr/local/musa-5.2/include \
 *       -L/usr/local/musa/lib -lmusart -Wl,-rpath,/usr/local/musa/lib \
 *       -o vector_add musa-vector-add.mu
 *
 * Prints PASS only when the kernel result is verified on the host.
 */
#include <musa_runtime.h>
#include <stdio.h>
#include <stdlib.h>

#define N (1u << 20)
#define CHECK(call)                                                          \
    do {                                                                     \
        musaError_t e_ = (call);                                             \
        if (e_ != musaSuccess) {                                             \
            fprintf(stderr, "%s:%d %s -> %d (%s)\n", __FILE__, __LINE__,     \
                    #call, (int)e_, musaGetErrorString(e_));                 \
            exit(EXIT_FAILURE);                                              \
        }                                                                    \
    } while (0)

__global__ void vadd(const float *a, const float *b, float *c, unsigned n)
{
    unsigned i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n)
        c[i] = a[i] + b[i];
}

int main(void)
{
    const size_t bytes = (size_t)N * sizeof(float);
    float *ha = (float *)malloc(bytes), *hb = (float *)malloc(bytes), *hc = (float *)malloc(bytes);
    float *da = NULL, *db = NULL, *dc = NULL;
    int dev = -1;
    CHECK(musaGetDeviceCount(&dev));
    if (dev < 1) {
        fprintf(stderr, "no MUSA device\n");
        return EXIT_FAILURE;
    }
    printf("MUSA devices: %d\n", dev);

    for (unsigned i = 0; i < N; i++) {
        ha[i] = (float)i;
        hb[i] = 2.0f * (float)i;
    }

    CHECK(musaMalloc((void **)&da, bytes));
    CHECK(musaMalloc((void **)&db, bytes));
    CHECK(musaMalloc((void **)&dc, bytes));
    CHECK(musaMemcpy(da, ha, bytes, musaMemcpyHostToDevice));
    CHECK(musaMemcpy(db, hb, bytes, musaMemcpyHostToDevice));

    vadd<<<N / 256, 256>>>(da, db, dc, N);
    CHECK(musaGetLastError());
    CHECK(musaDeviceSynchronize());
    CHECK(musaMemcpy(hc, dc, bytes, musaMemcpyDeviceToHost));

    for (unsigned i = 0; i < N; i++) {
        if (hc[i] != 3.0f * (float)i) {
            fprintf(stderr, "MISMATCH at %u: %f != %f\n", i, hc[i],
                    3.0f * (float)i);
            return EXIT_FAILURE;
        }
    }
    printf("PASS: %u float adds verified on S30\n", N);

    CHECK(musaFree(da));
    CHECK(musaFree(db));
    CHECK(musaFree(dc));
    free(ha); free(hb); free(hc);
    return EXIT_SUCCESS;
}

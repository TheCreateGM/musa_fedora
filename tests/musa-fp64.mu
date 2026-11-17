/*
 * Double-precision probe for the MTT S30.
 *
 * mp_10 is a first-generation MUSA target. This file is separate so a
 * compiler rejection does not hide the float compute tests. The host
 * accepts the result only when 0.1 + 0.2 is within 1e-12 of 0.3.
 */
#include <musa_runtime.h>
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

__global__ void add_f64(double *out)
{
    if (threadIdx.x == 0 && blockIdx.x == 0)
        out[0] = 0.1 + 0.2;
}

int main(void)
{
    double h = 0.0;
    double *d = NULL;
    double err;
    int devices = 0;

    CHECK(musaGetDeviceCount(&devices));
    if (devices < 1) {
        fprintf(stderr, "no MUSA device\n");
        return EXIT_FAILURE;
    }
    CHECK(musaMalloc((void **)&d, sizeof(double)));
    add_f64<<<1, 128>>>(d);
    CHECK(musaGetLastError());
    CHECK(musaDeviceSynchronize());
    CHECK(musaMemcpy(&h, d, sizeof(h), musaMemcpyDeviceToHost));
    CHECK(musaFree(d));
    err = h - 0.3;
    if (err < 0.0)
        err = -err;
    printf("fp64_result=%.17g abs_err=%.3e\n", h, err);
    if (err > 1e-12) {
        printf("FP64_TEST: FAIL\n");
        return EXIT_FAILURE;
    }
    printf("FP64_TEST: PASS\n");
    return EXIT_SUCCESS;
}

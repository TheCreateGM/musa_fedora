/*
 * musa-alloc-latency.mu — time musaMalloc()/musaFree() pairs on the S30.
 *
 * The S30 is host-memory backed: every buffer object inside the driver's SVM
 * heap goes through OSAllocUserPages() at order 6 (256 KiB, the
 * GeneralSVMHeapPageSize default).  Under host memory pressure that request
 * has to trigger direct reclaim and compaction, so allocation latency - not
 * bandwidth - is what a caller feels first.
 *
 * This probe times a fixed number of large alloc/free pairs and reports
 * percentiles plus the worst case, so a change to the driver's allocation
 * path can be measured instead of guessed at.
 *
 * Build:  mcc -march=mp_10 -o musa-alloc-latency musa-alloc-latency.mu
 * Exit:   0 on success.
 */
#include <musa_runtime.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define CHECK(call)                                                           \
    do {                                                                      \
        musaError_t e_ = (call);                                              \
        if (e_ != musaSuccess) {                                              \
            fprintf(stderr, "FAIL %s:%d %s -> %d (%s)\n", __FILE__, __LINE__, \
                    #call, (int)e_, musaGetErrorString(e_));                  \
            exit(1);                                                          \
        }                                                                     \
    } while (0)

static double now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1e3 + (double)ts.tv_nsec / 1e6;
}

static int cmp_double(const void *a, const void *b)
{
    double x = *(const double *)a, y = *(const double *)b;
    return (x > y) - (x < y);
}

int main(int argc, char **argv)
{
    size_t mib = 32;
    int rounds = 60;
    if (argc > 1) mib = (size_t)strtoul(argv[1], NULL, 0);
    if (argc > 2) rounds = atoi(argv[2]);
    if (mib == 0 || rounds < 5) {
        fprintf(stderr, "usage: %s [MiB=32] [rounds=60]\n", argv[0]);
        return 2;
    }

    size_t bytes = mib << 20;
    double *ms = (double *)calloc((size_t)rounds, sizeof(double));
    if (!ms) return 1;

    int devices = 0;
    CHECK(musaGetDeviceCount(&devices));
    if (devices < 1) { fprintf(stderr, "FAIL no MUSA device\n"); return 1; }
    CHECK(musaSetDevice(0));

    size_t free_b = 0, total_b = 0;
    CHECK(musaMemGetInfo(&free_b, &total_b));
    printf("buffer=%zu MiB rounds=%d mem_free=%zu MiB mem_total=%zu MiB\n",
           mib, rounds, free_b >> 20, total_b >> 20);

    /* Warm up once so the first-touch cost is not counted as a sample. */
    {
        void *p = NULL;
        CHECK(musaMalloc(&p, bytes));
        CHECK(musaFree(p));
    }

    double t_all = now_ms();
    for (int i = 0; i < rounds; i++) {
        void *p = NULL;
        double t0 = now_ms();
        CHECK(musaMalloc(&p, bytes));
        /* Touch one word per 256 KiB heap page so the sample includes the
         * kernel's mapping work, not just the bookkeeping. */
        CHECK(musaMemset(p, i & 0xff, bytes));
        double t1 = now_ms();
        ms[i] = t1 - t0;
        CHECK(musaFree(p));
    }
    double total = now_ms() - t_all;

    double *sorted = (double *)calloc((size_t)rounds, sizeof(double));
    if (!sorted) return 1;
    memcpy(sorted, ms, (size_t)rounds * sizeof(double));
    qsort(sorted, (size_t)rounds, sizeof(double), cmp_double);

    double sum = 0;
    for (int i = 0; i < rounds; i++) sum += ms[i];
    printf("alloc+memset p50=%.3f ms p95=%.3f ms max=%.3f ms mean=%.3f ms\n",
           sorted[rounds / 2], sorted[(int)(rounds * 0.95)], sorted[rounds - 1],
           sum / rounds);
    printf("total=%.1f ms\n", total);
    printf("ALLOC_LATENCY: PASS\n");

    free(sorted);
    free(ms);
    return 0;
}

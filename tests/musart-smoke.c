/*
 * musart-smoke.c — minimal MUSA runtime API smoke test (host side only).
 *
 * Exercises, in order: musaGetDeviceCount, musaGetDeviceProperties,
 * musaMalloc, musaMallocHost, musaMemcpy (H2D and D2H), musaMemset and
 * musaFree. It does NOT launch a device kernel; kernel launch is covered by
 * the mcc-compiled tests. Every result is printed and the exit code reflects
 * the furthest API stage reached.
 *
 * MUSA has no musaInit(); the runtime initializes lazily on the first API.
 *
 * The host API must be compiled against the *runtime* headers that match the
 * installed library: the MTT S30 uses MUSA runtime 1.5.4, whose musaDeviceProp
 * layout differs from the 5.2.0 SDK headers (728 vs 1032 bytes). Compile and
 * link against the same version to keep the device-property ABI correct.
 *
 * Build:  gcc -O2 tests/musart-smoke.c -I/usr/local/musa/include \
 *             -L/usr/local/musa/lib -lmusart -Wl,-rpath,/usr/local/musa/lib \
 *             -o build/musart-smoke
 * Run:    LD_LIBRARY_PATH=/usr/local/musa/lib build/musart-smoke
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <musa_runtime.h>

#define MUSART_STAGE_RC(name) static int musart_rc_##name(musaError_t e)
MUSART_STAGE_RC(count) { return 20; }
MUSART_STAGE_RC(props) { return 30; }
MUSART_STAGE_RC(alloc) { return 40; }
MUSART_STAGE_RC(xfer)  { return 50; }
MUSART_STAGE_RC(zero)  { return 60; }
MUSART_STAGE_RC(release) { return 70; }
MUSART_STAGE_RC(stream) { return 80; }
MUSART_STAGE_RC(event)  { return 85; }
MUSART_STAGE_RC(aio)    { return 90; }

#define TRY(call, name)                                                      \
	do {                                                                 \
		musaError_t e__ = (call);                                    \
		if (e__ != musaSuccess) {                                    \
			fprintf(stderr, "FAIL[%s] %s -> %s (%d)\n", #name,   \
				#call, musaGetErrorString(e__), (int)e__);   \
			return musart_rc_##name(e__);                        \
		}                                                            \
	} while (0)

int main(void)
{
	int devcount = -1;
	struct musaDeviceProp prop;
	memset(&prop, 0, sizeof(prop));

	TRY(musaGetDeviceCount(&devcount), count);
	printf("device_count=%d\n", devcount);
	if (devcount < 1) {
		fprintf(stderr, "FAIL: no MUSA devices reported\n");
		return 21;
	}

	TRY(musaGetDeviceProperties(&prop, 0), props);
	printf("name=%s\n", prop.name);
	printf("totalGlobalMem=%zu\n", (size_t)prop.totalGlobalMem);
	printf("sharedMemoryPerBlock=%zu\n", (size_t)prop.sharedMemPerBlock);
	printf("multiProcessorCount=%d\n", prop.multiProcessorCount);
	printf("major=%d minor=%d\n", prop.major, prop.minor);
	printf("clockRate=%d\n", prop.clockRate);
	printf("memoryClockRate=%d\n", prop.memoryClockRate);
	printf("memoryBusWidth=%d\n", prop.memoryBusWidth);

	float *d = NULL, *h = NULL;
	const size_t n = 1024, bytes = n * sizeof(float);

	TRY(musaMalloc((void **)&d, bytes), alloc);
	TRY(musaMallocHost((void **)&h, bytes), alloc);
	for (size_t i = 0; i < n; i++)
		h[i] = (float)i;

	TRY(musaMemcpy(d, h, bytes, musaMemcpyHostToDevice), xfer);
	TRY(musaMemset(d, 0, bytes), zero);

	memset(h, 0, bytes);
	TRY(musaMemcpy(h, d, bytes, musaMemcpyDeviceToHost), xfer);

	int zeros = 1;
	for (size_t i = 0; i < n; i++)
		if (h[i] != 0.0f) { zeros = 0; break; }
	printf("d2h_memset_verify=%s\n", zeros ? "PASS" : "FAIL");

	TRY(musaFreeHost(h), release);
	TRY(musaFree(d), release);

	/* Stream + event path: async copy and event timing on the device. */
	musaStream_t stream = NULL;
	musaEvent_t ev_start = NULL, ev_stop = NULL;
	float elapsed_ms = -1.0f;
	float *da = NULL, *ha = NULL;
	const size_t n2 = 4096, b2 = n2 * sizeof(float);

	TRY(musaStreamCreate(&stream), stream);
	TRY(musaEventCreate(&ev_start), event);
	TRY(musaEventCreate(&ev_stop), event);
	TRY(musaMalloc((void **)&da, b2), alloc);
	TRY(musaMallocHost((void **)&ha, b2), alloc);
	for (size_t i = 0; i < n2; i++)
		ha[i] = (float)(i + 1);

	TRY(musaEventRecord(ev_start, stream), event);
	TRY(musaMemcpyAsync(da, ha, b2, musaMemcpyHostToDevice, stream), aio);
	TRY(musaMemcpyAsync(ha, da, b2, musaMemcpyDeviceToHost, stream), aio);
	TRY(musaEventRecord(ev_stop, stream), event);
	TRY(musaEventSynchronize(ev_stop), event);
	TRY(musaStreamSynchronize(stream), stream);
	TRY(musaEventElapsedTime(&elapsed_ms, ev_start, ev_stop), event);

	int roundtrip = 1;
	for (size_t i = 0; i < n2; i++)
		if (ha[i] != (float)(i + 1)) { roundtrip = 0; break; }
	printf("async_roundtrip=%s\n", roundtrip ? "PASS" : "FAIL");
	printf("event_elapsed_ms=%.4f\n", elapsed_ms);

	TRY(musaFreeHost(ha), release);
	TRY(musaFree(da), release);
	TRY(musaEventDestroy(ev_stop), event);
	TRY(musaEventDestroy(ev_start), event);
	TRY(musaStreamDestroy(stream), stream);

	if (!roundtrip) {
		fprintf(stderr, "FAIL: async round-trip data mismatch\n");
		return 91;
	}

	if (!zeros) {
		fprintf(stderr, "FAIL: D2H data did not match memset pattern\n");
		return 51;
	}
	printf("SMOKE_TEST: PASS (no kernel launch)\n");
	return 0;
}

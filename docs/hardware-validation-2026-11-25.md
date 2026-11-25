# Hardware validation — MTT S30, 2026-11-25

Host: Fedora Linux 45, kernel `7.3.4-300.fc45.x86_64`, 7.4 GiB RAM, 20 CPUs.
Boot started 2026-11-24 23:59; `mtgpu` loaded by udev with no blacklist
(compute mode, see [compute-mode-enabled-2026-11-24](compute-mode-enabled-2026-11-24.md)).

## Driver init (journal, this boot)

```
mtgpu 0000:01:00.0: MTGPU Firmware image 'mthreads/mtfw-gen1.bin' loaded
mtgpu 0000:01:00.0: mtgpu watchdog enabled
[drm] Initialized mtgpu 2.0.0 for 0000:01:00.0 on minor 0
```

No `BUG`, `Oops`, general-protection, or invalid-opcode entry was logged in
this boot. The `irq:NNN affinity has changed` messages are the known noise
documented in the IRQ affinity notes.

## Verification run

Command (run in a cgroup with a 3 GiB hard cap and no swap):

```sh
systemd-run --user --scope -p MemoryMax=3G -p MemorySwapMax=0 -p TasksMax=256 \
    scripts/verify-musa.sh --skip-memcpy
```

Result: **13 passed, 0 failed, 2 skipped** (exit 0).

* PASS PCI binding (`1ed5:0102` on `mtgpu`), `/dev/mtgpu.0`, render node `renderD129`
* PASS IOMMU groups (13), `libdrm_mtgpu.so.1` (packaged 3.1.0)
* PASS `mthreads-gmi` reports the MTT S30 (driver 3.1.0)
* PASS `musaInfo` (runtime 1.5.4)
* PASS `musart-smoke` on the GPU (d2h memset, async round trip)
* PASS `mcc` builds `mp_10` vector-add; 1,048,576 float adds verified on the S30
* PASS compute suite: saxpy, shared reduce, atomics, constant memory, device math, 2D grid, D2D copy, FP64
* PASS Vulkan device enumeration and host-visible buffer map
* SKIP `muMemcpyTest` (skipped by `--skip-memcpy`; see finding 1)
* SKIP Moore Threads OpenCL (no MT ICD installed)

## Findings

1. **`muMemcpyTest` needs more than 2 GiB of host RSS.** A first run under a
   2 GiB cap was OOM-killed inside its cgroup (`oom-kill` on `muMemcpyTest`,
   anon-rss ≈ 2.0 GiB), and the wrapper did not complete. The test is not
   run in the 3 GiB verification above. Run it only when at least ~4 GiB is
   free, in a capped scope, and record the result as a separate log.
2. **Parallel builds exhausted host RAM.** On 2026-11-25 02:28 many parallel
   `cc1` compiler processes triggered a global OOM. The kernel killed
   `deepai-code` and a browser content process; `mtgpu` was not involved.
   Build and test jobs on this host should run under a memory cap
   (`systemd-run --user --scope -p MemoryMax=… `) or with a bounded `-j`.
3. Git: both `musa-fedora` and `musa-data` are up to date with origin, and
   `git push --dry-run` succeeds with the current GitHub login.
4. COPR: `copr-cli` is authenticated and lists the `axogm` projects. The chroot
   state from the 2026-11-23 verification is unchanged in this session.

## Driver fix: system/SVM heap page size (mtgpu-dkms 3.1.0-15)

The 3.1.0-14 driver allocated every host-backed buffer object in units of
`PVRSRV_APPHINT_GENERALSVMHEAPPAGESIZE = 0x40000` — **256 KiB, order 6**. On
this 7.4 GiB host that request failed under pressure: `musaMallocHost()`
returned `musaErrorOutOfMemory` and `scripts/s30-perf.sh` aborted, after the
kernel logged `page allocation failure: order:6`.

3.1.0-15 sets the heap page to `0x4000` (16 KiB, order 2), the size the driver
already uses for its non-4K heap. The host rebooted onto the patched module
(`/usr/src/mtgpu-3.1.0/inc/config_kernel.h` carries `0x4000`) and the S30 bound
it. See [the corrected root cause](root-cause-order6-warn-alloc-2026-11-24.md);
the earlier 16 MiB reading of order 6 was wrong.

## Post-fix hardware runs

All three ran on the physical S30 in memory-capped scopes
(`MemoryMax=3G MemorySwapMax=0`) with no kernel fault logged.

### Allocation latency

```
buffer=32 MiB rounds=60 mem_free=3775 MiB mem_total=4069 MiB
alloc+memset p50=0.064 ms p95=0.074 ms max=0.085 ms mean=0.065 ms
total=131.1 ms
ALLOC_LATENCY: PASS
```

No `page allocation failure` accompanied the run, and
`journalctl -k -b | grep -c 'page allocation failure\|warn_alloc'` is **0**.

### Performance benchmark

```
pinned   H2D=   6.4 GB/s  D2H=   6.4 GB/s
pageable H2D=   5.1 GB/s  D2H=   5.5 GB/s
2000 no-op launches in 5.58 ms -> 2.8 us/launch
blockDim=1024 grid=  8192     1.934 ms     52.0 GB/s
pipelined (2 streams, pinned, async) =   17.202 ms per pass
speedup=1.11x
PERF_STATUS: PASS (1 skipped)
```

The skip is `musaDeviceSetCacheConfig(PreferShared)`, which `mp_10` reports as
`musaErrorNotSupported`. The benchmark's kernel section reports `none (no new
mtgpu fault, allocation failure or Oops)`.

### VA-API (musa-runtime-va 1.5.4-7)

`scripts/hardware/test-vaapi.sh` reports **14 passed, 0 failed, 0 skipped**:
`vaInitialize 1.24` on `renderD129`, the Moore Threads VPU driver string,
20 profiles, and an `h264_vaapi` encode that produced a 425 503-byte stream
decoding cleanly. Details and the advertised-capability caveat are in
[VA-API](vaapi.md).

## Isolated container install and GPU run

`scripts/test-container-install.sh --skip-memcpy` installed the seven locally
built RPMs (dkms 3.1.0-15, firmware 3.1.0-5, runtime 1.5.4-7, runtime-va
1.5.4-7, runtime-display 1.5.4-7, sdk 5.2.0-7, toolkit 3.1.0-5) into a
throwaway Fedora 45 container with RPM scriptlets disabled, so the host's driver
and package database were untouched. Only `/dev/mtgpu.0` and the S30 render
node were passed through.

```
CONTAINER_SUMMARY: 13 passed, 0 failed
musaInfo: MTT S30, 3.97 GB, 8 SMs
muMemcpyTest: 1073741824 bytes, SUCCESS
musart-smoke: PASS (d2h_memset_verify PASS, async_roundtrip PASS)
mcc --offload-arch=mp_10: PASS: 1048576 float adds verified on S30
TEST_CONTAINER_INSTALL_STATUS: SUCCESS
```

The container also installed `musa-runtime-display` because the test passes
every RPM explicitly; the host still does not have it. This is the isolated
install gate, not a desktop test.

## COPR publication

The same package set was submitted to both COPR projects. All 12 builds (10
initial plus toolkit 3.1.0-5 in both projects) succeeded across 9 chroots, and
`scripts/verify-copr-chroots.sh` reports **20 passed, 0 failed**: every
chroot's published `repodata` carries all seven artifacts at the expected
versions. Build IDs and the submission workaround are recorded in
[COPR verification 2026-11-25](copr-verification-2026-11-25.md).

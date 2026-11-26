# Hardware validation — MTT S30, 2026-11-26 boot

Host: Fedora Linux 45, kernel `7.3.4-300.fc45.x86_64`, 7.4 GiB RAM, 20 CPUs.
Fresh boot started **2026-11-26 00:07:06 UTC**. This is the first validation
session on a boot after the full 2026-11-25 stack (dkms 3.1.0-15 with the
16 KiB SVM heap page, runtime 1.5.4-7, toolkit 3.1.0-5 pinned set) reached the
published state, and the first on a reboot that proves compute-mode boot
persistence end-to-end.

## Driver init (journal, this boot)

```
mtgpu 0000:01:00.0: MTGPU Firmware image 'mthreads/mtfw-gen1.bin' loaded
mtgpu 0000:01:00.0: mtgpu watchdog enabled
[drm] Initialized mtgpu 2.0.0 for 0000:01:00.0 on minor 0
```

`lspci -k` shows `Kernel driver in use: mtgpu` on `1ed5:0102` with **no
blacklist active** — the device bound automatically at this cold boot. That is
the compute-mode persistence claim re-proven on a second independent boot.
At the start of this session the boot journal contained **zero**
`BUG/Oops/general protection/invalid opcode` entries for `mtgpu` (the
recurring `irq:NNN affinity has changed` lines are the documented noise).

## Verification runs (all on this boot, capped scopes)

Every GPU job ran under
`systemd-run --user --scope -p MemoryMax=… -p MemorySwapMax=0` per the
parallel-build OOM finding from 2026-11-25.

### Full suite — `scripts/verify-musa.sh --skip-memcpy`

Result: **13 passed, 0 failed, 2 skipped** (log: `.logs/verify-2026-11-26-boot.log`).

* PASS PCI binding, `/dev/mtgpu.0`, `renderD129`, IOMMU groups, packaged
  `libdrm_mtgpu.so.1` (3.1.0)
* PASS `mthreads-gmi` (S30, driver 3.1.0), `musaInfo` (runtime 1.5.4)
* PASS `musart-smoke` (d2h memset verify, async round trip, event 0.119 ms)
* PASS `mcc` `mp_10` vector-add: 1,048,576 float adds verified on the S30
* PASS compute suite: saxpy, shared reduce, atomics, constant memory, device
  math, 2D grid, D2D copy, FP64 (abs err 5.551e-17)
* PASS Vulkan headless: device enumeration (`vendor=0x1ed5 id=0x0102`),
  `vkCreateDevice=0`, host-visible buffer map
* SKIP `muMemcpyTest` — needs ≥4 GiB host RSS; this boot has ~3.4 GiB
  available (finding 1 of the 2026-11-25 record stands)
* SKIP Moore Threads OpenCL — no MT ICD; the archived 2.1.1 ICD remains
  quarantined after its historical kernel Oops

### Performance — `scripts/s30-perf.sh` (MemoryMax=3G)

`PERF_STATUS: PASS (1 skipped)` (log: `.logs/s30-perf-2026-11-26-boot.log`):

```
pinned   H2D=   6.3 GB/s  D2H=   6.5 GB/s
pageable H2D=   5.0 GB/s  D2H=   5.6 GB/s
2000 no-op launches in 5.47 ms -> 2.7 us/launch
blockDim=1024 grid=  8192     1.936 ms     52.0 GB/s
pipelined (2 streams, pinned, async): 17.179 ms vs 19.164 ms serial -> 1.12x
persistent-launch saving            =   96 us over 64-chunk split
```

The single skip is `musaDeviceSetCacheConfig(PreferShared)` →
`musaErrorNotSupported (801)`: `mp_10` exposes no shared/L1 carveout knob.
Benchmark-internal kernel-log section: **none (no new mtgpu fault, allocation
failure or Oops)**. Cross-boot reproducibility against the 2026-11-25 runs:

| Metric | 11-25 a.m. | 11-25 p.m. | 11-26 (this boot) |
|---|---|---|---|
| pinned H2D / D2H | 6.4 / 6.4 | 6.4 / 6.5 | 6.3 / 6.5 GB/s |
| launch overhead | 2.8 µs | 2.7 µs | 2.7 µs |
| triad best | 52.0 GB/s | 51.8 GB/s | 52.0 GB/s |
| 2-stream pipeline | 1.11× | 1.13× | 1.12× |

Numbers are stable across three boots on the 3.1.0-15 heap-page fix; the
variance is within measurement noise.

### VA-API — `scripts/hardware/test-vaapi.sh` (MemoryMax=2G)

**14 passed, 0 failed, 0 skipped**: `vaInitialize 1.24` on `renderD129`,
Moore Threads VPU driver string, 20 profiles (incl. AV1 encode entrypoints
advertised), `h264_vaapi` encode producing a 425 503-byte stream that decodes
cleanly; kernel log during the run shows only `[mtvpu] Core 2 FW Version
ad3595b` and no fault.

### Environment — `scripts/check-musa-env.sh`

`MUSA_ENV_STATUS: PASS`. The two WARNs are the documented unmanaged legacy
`/etc/profile.d/musa.sh` 5.2-prefix snippet, which the packaged
`zz-musa-fedora.sh` deliberately overrides by sort order (see commit
`408a387`); quarantine remains an operator option, not a requirement.

### COPR — `scripts/verify-copr-chroots.sh`

**20 passed, 0 failed** across both projects and all 9 chroots
(fedora-43/44/45/eln/rawhide). Published set confirmed at
`mtgpu-dkms 3.1.0-16`, `musa-toolkit 3.1.0-6`, runtime family 1.5.4-7,
sdk 5.2.0-7, firmware 3.1.0-5; every latest build `succeeded`.

## Host package state and the -16/-6 decision

The host runs the pinned verified set
(`mtgpu-dkms-3.1.0-15`, `mtgpu-firmware-3.1.0-5`, `musa-runtime-1.5.4-7`,
`musa-runtime-va-1.5.4-7`, `musa-sdk-5.2.0-7`, `musa-toolkit-3.1.0-5`) with
`mtgpu` loaded. Per the 2026-11-25 decision (docs/objtool-fallback-2026-11-25.md),
the `3.1.0-16` / `3.1.0-6` pair is **not** installed over the live module:
dkms 3.1.0-16 changes only the objtool fallback wrapper (build robustness on
rawhide binutils drift; byte-identical ORC output on this kernel), and the
toolkit pins the exact set. Install during a window without a loaded `mtgpu`.

## Boot-verify re-arm

`musa-boot-verify.service` had self-removed after its previous run (by
design). Re-armed this session with
`pkexec scripts/install-boot-verify.sh install`; it now shows `enabled` and
will write `14/0/0`-style compute-mode evidence to
`/var/log/musa-fedora/compute-mode-boot-<timestamp>.log` at the next reboot.

## Post-session journal re-check

After all GPU work above, `journalctl -k -b | grep -ciE
'mtgpu.*(BUG|Oops|general protection|invalid opcode|null pointer)'` → **0**;
`page allocation failure` → **0**.

## Verdict

The full stack reproduces clean on an independent cold boot with automatic
driver binding (compute mode), 13/0/2 suite, stable perf numbers, 14/0/0
VA-API, PASS environment gate, and 20/20 COPR repository checks, with zero
kernel faults attributable to mtgpu throughout.

# Controlled load test with `musa-runtime` 1.5.4-4 — 2026-11-23 07:38 UTC

## Purpose

The first controlled load on 2026-11-23 ran against `musa-runtime` 1.5.4-3,
which still shipped the vendor Mesa DRI drivers; GNOME Shell crashed during
that run. `musa-runtime` 1.5.4-4 moved every DRI/GBM/Xorg driver into the
optional `musa-runtime-display` subpackage
([display-driver split](display-driver-subpackage-2026-11-23.md)), so the
crash condition should be gone from a compute-only install. This run repeats
the identical controlled load with 1.5.4-4 installed to verify that the crash
fix actually works on the hardware. Until this run, the boot-time block
stayed in place; the result below is the evidence behind
[`compute-mode`](compute-mode.md).

## Host state at run time

| Item | Value |
|---|---|
| Boot | installed Fedora 45 from disk, kernel `7.3.4-300.fc45.x86_64` |
| Boot time | 2026-11-23 07:32:05 UTC |
| Package set | `mtgpu-dkms` 3.1.0-11, `mtgpu-firmware` 3.1.0-4, `musa-runtime` **1.5.4-4**, `musa-sdk` 5.2.0-6, `musa-toolkit` 3.1.0-3 |
| `musa-runtime-display` | not installed |
| Vendor Mesa DRI/GBM/Xorg files | absent (checked `/usr/lib64/dri`, `/usr/lib64/gbm`, `/usr/lib64/xorg/modules/drivers`) |
| `mtgpu` before load | not loaded, blacklisted at boot (`/etc/modprobe.d/blacklist-mtgpu.conf`) |
| Desktop | GNOME on Intel i915, GDM active |
| IOMMU | `intel_iommu=on iommu=pt` on the kernel command line |

## The run

`scripts/hardware/load-and-test.sh` (one `insmod`, headless parameters
`display=none disable_fbdev=1 mtgpu_drm_major=2`, no rmmod, no unbind/rebind,
blacklist untouched). Log:
`~/.musa-work/load-test-20261123T073849Z/load-and-test.log`.

The run began with the script's own insmod and the first four steps. The five
remaining steps were interrupted by the previous session; they were built
with the reproducible builder `scripts/hardware/build-test-binaries.sh`
(commit `436e6bc`) into the same log directory and executed with exactly the
environment `load-and-test.sh` uses, appending to the same log file:

```
env LD_LIBRARY_PATH=/usr/local/musa/lib timeout 120 ./musart-smoke
env LD_LIBRARY_PATH=/usr/local/musa/lib timeout 120 ./musa-vector-add
env LD_LIBRARY_PATH=/usr/local/musa/lib timeout 120 ./musa-compute
env LD_LIBRARY_PATH=/usr/local/musa/lib timeout 120 ./musa-fp64
env VK_DRIVER_FILES=/etc/vulkan/icd.d/musaicdconf.json \
    VK_ICD_FILENAMES=/etc/vulkan/icd.d/musaicdconf.json \
    LD_LIBRARY_PATH=/usr/local/musa/lib timeout 60 ./vk-enum
```

## Results

Every functional step passed. **No kernel fault appeared at any point in this
boot** (`journalctl -k -b` contains no `Oops:` / `BUG:` / NULL dereference).
GNOME Shell and GDM survived the load and the whole test — the crash that
ended the 1.5.4-3 run did not reproduce, confirming the root cause was the
packaged Mesa DRI drivers and that 1.5.4-4 fixes it.

| Step | Result | Evidence (log) |
|---|---|---|
| `insmod` (headless args) | rc=0 | `insmod rc=0`, `initstate=live` |
| S30 render node | created | `/dev/dri/renderD130` |
| GNOME after load | survived | `gnome-shell after load: 1 process(es)` |
| `mthreads-gmi -q` | rc=0 | reports MTT S30, 4096 MiB |
| `musaInfo` (1.5.4) | rc=0 | one device: MTT S30, 8 SM, 3.97 GB |
| `muMemcpyTest` | rc=0 | vendor device-copy test SUCCESS |
| `musart-smoke` | rc=0 | `d2h_memset_verify=PASS`, `async_roundtrip=PASS`, `event_elapsed_ms=0.2438`, `SMOKE_TEST: PASS` |
| `musa-vector-add` (mp_10) | rc=0 | `PASS: 1048576 float adds verified on S30` |
| `musa-compute` (mp_10) | rc=0 | saxpy, shared_reduce, atomic_add, constant_mem, device_math, grid2d, d2d_copy all PASS |
| `musa-fp64` (mp_10) | rc=0 | `abs_err=5.551e-17`, `FP64_TEST: PASS` |
| `vk-enum` headless Vulkan | rc=0 | `device0 vendor=0x1ed5 id=0x0102 name=MTT S30`, buffer/alloc/map PASS |
| Desktop at end | survived | `gnome-shell: 2 process(es); gdm: active` |
| Kernel faults this boot | 0 | `mtgpu faults in this boot: 0` |

Device properties as reported by `musaInfo` (identical to the first run):
MTT S30, `multiProcessorCount=8`, `maxThreadsPerMultiProcessor=6144`,
`warpSize=128`, 1300 MHz core / 4250 MHz memory, 256-bit bus, 3.97 GB.

## Interpretation

* The **driver** (mtgpu 3.1.0), the **1.5.4 runtime**, the 5.2.0 `mcc`-built
  `mp_10` kernels and the Vulkan ICD all work on the installed kernel, and
  they work **while the desktop runs on i915** — the two devices coexist when
  the vendor Mesa display drivers are absent.
* The mutter crash is confirmed as a packaging problem (vendor DRI drivers in
  the base runtime), not a driver bug. `musa-runtime` 1.5.4-4 + no
  `musa-runtime-display` is a stable compute-only configuration.
* This is the evidence required by
  [`compute-mode.md`](compute-mode.md) to remove the boot-time block.

## What this run did not test

* Display output from the S30's own connectors (`display=mt`) — never
  exercised; the desktop stays on i915.
* A **reboot with the block removed** (compute mode at boot). The headless
  load parameters are identical to the ones used here, but the boot-time path
  itself (initramfs, plymouth/gdm ordering with the module present) has not
  been observed yet.
* Moore Threads OpenCL (no MT ICD in the 1.5.4 runtime; the archived 2.1.1
  ICD stays quarantined).

## State after the run

`mtgpu` remains loaded (the scripts never unload; a reboot returns to the
blacklisted, unbound state because the boot-time block is still in place at
the time of this run). Nothing in `/etc/modprobe.d`, the initramfs, or any
boot configuration was changed by this run.

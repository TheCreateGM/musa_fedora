# Hardware validation — 2026-11-23 controlled load test (installed system)

## Scope and provenance

This record captures the only **current-generation** load test of the MTT S30
on the installed Fedora 45 system (kernel `7.3.4-300.fc45.x86_64`). It was run
from the 2026-11-23 Live USB session at `02:09:33Z`–`02:09:46Z` against the
installed root, using `scripts/hardware/load-and-test.sh`. The script's full
log and all test binaries are preserved under:

```
/home/axogm/.musa-work/load-test-20261123T020927Z/
  load-and-test.log   (10927 bytes)
  musa-compute, musa-fp64, musart-smoke, musa-vector-add, vk-enum
```

**This is evidence about the installed system, not the Live USB kernel
`7.2.11-300.fc45.x86_64`.** The S30 in the Live USB boot is still unbound.

The load followed the safety contract of the (still unapproved)
`docs/maintenance-plan-mtgpu-load-2026-11-20.md`: blacklist untouched, one
`insmod` with headless parameters, no `rmmod`, no PCI unbind/rebind. It did
**not** modify any system file. A reboot restores the known-good blacklisted
state.

## Pre-flight evidence

| Item | Value |
|---|---|
| Module | `/lib/modules/7.3.4-300.fc45.x86_64/extra/mtgpu.ko.xz` |
| Module sha256 | `8389751507f413952168a010c24e31d4a45ace6d1fe480dc91f9158f870fb8ca` |
| Vendor core sha256 | `d0fdaceab37bce7387d376f8a87a2e9dbf210eeccf4f4f7c67f8dee83f4e8200` (matches pin) |
| Kernel cmdline | `... intel_iommu=on iommu=pt` (unchanged; security not disabled) |
| Blacklist | `blacklist mtgpu` (still active) |
| Load args | `display=none disable_fbdev=1 mtgpu_drm_major=2` |
| `insmod` result | `rc=0`, no kernel fault (`BUG:`/`Oops:`) in this boot |

The driver bound successfully: `mtgpu-drm-driver ... bound mtgpu.4.auto`,
firmware `mthreads/mtfw-gen1.bin` and shader `musa.sh.1.0.0.0` loaded,
`[drm] Initialized mtgpu 2.0.0 ... on minor 2`, and device nodes
`/dev/mtgpu.0` + `/dev/dri/renderD130` appeared.

## Results

| Step | Result | Notes |
|---|---|---|
| Firmware + device init | PASS | No kernel fault, `initstate=live` |
| `muMemcpyTest` (vendor, 1 GiB) | **PASS** | `Detected 1 MUSA Capable device(s)`, `device 0 SUCCESS` |
| `musart-smoke` (host runtime API) | **PASS** | `device_count=1`, `name=MTT S30`, 4,267,532,288 B VRAM, 8 SMs, memset and async round-trip verified |
| vector-add `mp_10` (mcc 5.2.0) | **PASS** | 1,048,576 float adds verified on S30 |
| compute suite | **PASS** | saxpy, shared_reduce, atomic_add, constant_mem, device_math, grid2d, d2d_copy |
| fp64 | **PASS** | `abs_err=5.551e-17` |
| Vulkan headless device/buffer/map | **PASS** | vendor `0x1ed5` device `0x0102`, buffer alloc + bind + map |
| `mthreads-gmi -q` | **FAIL** (rc=2) | `Error: failed to initialize mtml.` |
| `musaInfo` | **FAIL** (rc=134) | `mtgpu_query_marketing_name failed (-22)`, libdrm-mt assertion |
| GNOME Shell | **crashed** | `gnome-shell` count went 2 → 0; GDM active |

Overall script result: `rc=1` (two informational-tool failures; every
functional compute/GPU step passed).

## Root-cause findings

### 1. Both tool failures are userspace/tooling issues, not compute-path faults

The failing steps are informational queries; the MUSA compute path itself is
proven working by the six passing steps above, including real kernel launches.

- **`musaInfo`**: the script invoked `$MUSA_SDK/bin/musaInfo`, i.e. the **5.2.0**
  tool, which links `libmusart.so.5`. The verified stack uses the **1.5.4**
  runtime. The 1.5.4 tool at `/usr/local/musa/bin/musaInfo` links
  `libmusart.so.1.5`/`libmusa.so.1.5`. This mismatch explains the `EINVAL`
  device-info ioctl and the `libdrm-mt` refcount assertion. Fixed in
  `scripts/hardware/load-and-test.sh` by selecting the runtime's own tool.
- **`mthreads-gmi`**: it loads `libdrm_mtgpu.so` at runtime. On this host,
  `/usr/lib64/libdrm_mtgpu.so.1` was redirected to `libmtperf_target.so`
  instead of the packaged `libdrm_mtgpu.so.1.0.0`. This is a pre-existing,
  documented local difference (`docs/current-state-2026-11-20.md`), left
  untouched. It is the most likely cause of `failed to initialize mtml`; the
  correct fix is to restore the packaged symlink, not to change the driver.

### 2. The GNOME crash is a Mesa/GBM device-selection issue

The kernel log shows `comm = gnome-shell, ... force destroy app memory
context` and prior boot records show `libmutter-cogl-51.so` and
`libsrv_um_MUSA.so` in the GNOME coredump. When `mtgpu` registers a DRM card,
Mesa enumerates it and loads the vendor MUSA DRI drivers from
`/usr/lib64/dri/` (`musa_dri.so`, `kms_swrast_musa_dri.so`, `mtgpu_dri.so`),
and GNOME Shell segfaults in its KMS thread.

`display=none` is a **valid** value: although `modinfo` lists only `<dummy>,
<mt>`, disassembly of the pinned vendor core object shows the
`mtgpu_display_is_none()` check compares against the string `none`, and
`mtgpu_drm_dummy_connector.c` reports that connector disconnected. The crash
is therefore not caused by an invalid module parameter; it is caused by the
Mesa DRI drivers being visible system-wide. The blacklist remains the correct
default host state until a display isolation fix is reviewed and approved.

`mtgpu_drm_major` is a DDK generation selector (`1` = ddk1.0, `2` = ddk2.0),
**not** a DRM minor override; the `on minor 2` line is coincidental.

## What this does and does not establish

Established: with the pinned 3.1.0 driver, 1.5.4 runtime and 5.2.0 `mcc`
compiler on the installed Fedora 45 kernel, the S30 executes vendor memcpy,
host runtime API, `mp_10` kernels, a seven-kernel compute suite, fp64 and
Vulkan buffer/memory operations. No kernel fault was observed in this boot.

Not established: display output on the S30, OpenCL (absent and previously
faulty), framework integration, or desktop stability while `mtgpu` is loaded.
The blacklist is still active at boot; the Live USB S30 remains unbound. Do
not generalize these results to the Live USB kernel or to a boot-time driver
load without a reviewed, approved display-isolation plan.

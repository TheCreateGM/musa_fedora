# Controlled load test on the MTT S30 with `musa-runtime` 1.5.4-5 and `mtgpu-dkms` 3.1.0-12

**Date:** 2026-11-24 01:43-01:44 UTC
**Kernel:** `7.3.4-300.fc45.x86_64` (booted 00:46:06 UTC)
**Purpose:** re-validate the stack after the 1.5.4-5 environment fix and the
3.1.0-12 packaging change, and re-check the S30 compute path on real hardware.

## Host state at load time

| Item | Value |
|------|-------|
| OS | Fedora Linux 45 (Workstation), x86_64 |
| GPU | MTT S30 `0000:01:00.0` (`1ed5:0102`), audio `1ed5:01ff` |
| Packages | `musa-runtime` 1.5.4-5, `musa-sdk` 5.2.0-6, `musa-toolkit` 3.1.0-3, `mtgpu-dkms` 3.1.0-12, `mtgpu-firmware` 3.1.0-4 |
| `musa-runtime-display` | **not installed** (no vendor DRI/GBM/Xorg files) |
| Desktop | GNOME on the Intel i915, GDM active, 2 `gnome-shell` processes |
| IOMMU | `intel_iommu=on iommu=pt` on the cmdline |
| Boot-time block | `/etc/modprobe.d/blacklist-mtgpu.conf` present, not changed by this run |
| Module before load | not loaded, no `mtgpu` lines in this boot journal |

Artifacts:

* module: `/lib/modules/7.3.4-300.fc45.x86_64/extra/mtgpu.ko.xz`,
  sha256 `8389751507f413952168a010c24e31d4a45ace6d1fe480dc91f9158f870fb8ca`
* vendor core: `/usr/src/mtgpu-3.1.0/objs/x86_64/mtgpu_core.o_binary`,
  sha256 `d0fdaceab37bce7387d376f8a87a2e9dbf210eeccf4f4f7c67f8dee83f4e8200`
* evidence: `~/.musa-work/load-test-20261124T014358Z/load-and-test.log` and
  `kernel-journal.log` in the same directory

## The run

```sh
scripts/hardware/load-and-test.sh
```

One `insmod` with the documented headless parameters, no `rmmod`, no PCI
unbind/rebind, blacklist untouched:

```
insmod mtgpu.ko.xz display=none disable_fbdev=1 mtgpu_drm_major=2   # rc=0
initstate=live
/dev/mtgpu.0 present
S30 render node: /dev/dri/renderD130
```

GNOME survived the load (`gnome-shell` 2 processes before and after).

## Results

| Step | Result | Evidence |
|------|--------|----------|
| `insmod` + init | PASS | `insmod rc=0`, `initstate=live` |
| device nodes | PASS | `/dev/mtgpu.0`, `/dev/dri/renderD130` |
| `mthreads-gmi -q` | **FAIL** (rc=2) | `Error: failed to initialize mtml.` - see `docs/root-cause-mthreads-gmi-drm-query-2026-11-24.md` |
| `musaInfo` (1.5.4) | PASS | MTT S30, 8 SMs, 1300 MHz, 3.97 GB, 3.69 GB free (93%) |
| `muMemcpyTest` | PASS | 1,073,741,824 bytes copied, `====device 0 SUCCESS===================` |
| `musart-smoke` | PASS | `d2h_memset_verify=PASS`, `async_roundtrip=PASS`, `SMOKE_TEST: PASS` |
| vector-add (`mp_10`) | PASS | `PASS: 1048576 float adds verified on S30` |
| compute suite | PASS | saxpy, shared_reduce, atomic_add, constant_mem, device_math, grid2d, d2d_copy, `COMPUTE_TEST: PASS` |
| FP64 | PASS | `abs_err=5.551e-17` |
| Vulkan headless | PASS | `device0 vendor=0x1ed5 id=0x0102 name=MTT S30`, buffer + host-visible map |
| OpenCL | SKIP | no Moore Threads ICD in the 1.5.4 runtime (by policy) |

`scripts/verify-musa.sh` on the same live stack: **12 passed, 1 failed,
1 skipped**. The single failure is `mthreads-gmi`; every compute, memory,
compiler and Vulkan layer passes.

`MUSA_S30_TEST_APPROVED=1 scripts/test-gpu.sh` also ran against the same
module: the fail-closed journal gate found no `Invalid cmd_type` in this
boot and the smoke test printed `MUSA_VECTOR_ADD_OK elements=1048576` and
`GPU_TEST_STATUS: PASS`.

Notes on the run:

* `load-and-test.sh` ended with `overall rc=1` because the `mthreads-gmi`
  step exited 2. All other steps returned 0.
* The log line `test binaries staged: 0/5` was a cosmetic bug in the
  staging summary (it listed names in the wrong directory) and has been
  fixed; the five binaries were present and ran. The duplicated log lines
  in this evidence file are the old `tee` + redirection bug, also fixed
  after the run. Neither affected the test results.
* The host had a separate packaging defect: a 5.2.0
  `libmtperf_target.so` (SONAME `libdrm_mtgpu.so.1`) had hijacked
  `/usr/lib64/libdrm_mtgpu.so.1`. It was quarantined during this session
  and the packaged link restored (`rpm -V musa-runtime` clean). See
  `docs/root-cause-drm-soname-hijack-2026-11-24.md`. It is not the cause of
  the `mthreads-gmi` failure (that tool loads no libdrm).

## Kernel health

`journalctl -k -b` after the run contains **no** `Oops:`, `BUG: kernel`,
`general protection fault`, kernel NULL dereference, or `Invalid cmd_type`.
The module logs only its normal init lines, the cosmetic `event_report`
`/var/log/mtgpu` warnings (vendor core, documented 2026-11-23), and IRQ
affinity messages. GDM stayed active and GNOME Shell did not crash.

## State after the test

`mtgpu` is still loaded (the scripts never unload it). Immediately after this
run the boot-time blacklist was unchanged. **Subsequent change (02:08 UTC):**
`scripts/compute-mode.sh enable` was executed and the host is now
compute-active on a normal boot; the blacklist was moved to
`/var/backups/musa-fedora-compute-mode/20261124T020848Z/` and the initramfs was
regenerated. See
[`compute-mode-enabled-2026-11-24.md`](compute-mode-enabled-2026-11-24.md).

A second `verify-musa.sh` run at 02:05 UTC, after the `libdrm_mtgpu.so.1` link
was restored, reported **13 passed / 1 failed / 1 skipped**; the DRM-library
check passes now, and the only failure is still `mthreads-gmi`.

## Not covered

Display output from the S30 connectors (`display=mt`), DisplayPort hotplug,
video encode/decode, multi-GPU, OpenCL, and framework integrations
(`torch_musa`, etc.) remain untested. The 02:08 compute-mode enable is recorded
separately; the one post-reboot observation still outstanding is udev loading
`mtgpu` before GDM with no vendor DRI files installed.
# Current State — updated 2026-11-17

> Earlier snapshot. The canonical current-state record is
> [current-state-2026-11-17.md](current-state-2026-11-17.md) (and the project
> [README](../README.md)); where the two differ, that record governs.

Fedora Linux 45 Workstation, x86_64, kernel `7.3.3-300.fc45.x86_64` (64-bit),
Intel Core i5-13500 with Intel UHD Graphics 770 (i915 display), plus a **Moore
Threads MTT S30** (`1ed5:0102`, 4 GiB, 2 cores) on `0000:01:00.0`.

## Live host summary

- MTT S30 PCI: `0000:01:00.0`, vendor `1ed5`, device `0102`. **Verified by PCI**
- Kernel module: `mtgpu` loaded, `initstate=live`; module file is
  `/lib/modules/7.3.3-300.fc45.x86_64/extra/mtgpu.ko` and reports kernel
  vermagic `7.3.3-300.fc45.x86_64`. **Verified loaded**
- `/dev/mtgpu.0` and S30 DRM render node `/dev/dri/renderD129` present.
  **Verified**
- IOMMU: `intel_iommu=on iommu=pt` on active kernel command line; S30 is in
  IOMMU group 11. **Verified**
- Firmware installed: `/lib/firmware/mthreads/` (`mtfw-*`, `mtlog-*`, `mtvpu-*`).
  **Verified**
- MUSA runtime 1.5.4 installed at `/usr/local/musa/`
  (`musa_runtime_version` reports 1.5.4). **Verified**
- MUSA 5.2.0 SDK at `/usr/local/musa-5.2/` (mcc, mtxas, headers, libmusart).
  **Verified**
- Display: `i915` drives the desktop. **Verified**
- `mthreads-gmi` reports driver 3.1.0, S30, 4096 MiB; MUSA runtime 1.5.4
  `musaInfo` reports one device; `muMemcpyTest` succeeds. **Verified**
- `scripts/verify-musa.sh` on 2026-11-17: **10 passed, 0 failed**. It compiled
  with `mcc` 5.2.0 and the `mp_10` target, ran against runtime 1.5.4, and
  validated all 1,048,576 vector-add results on the S30. **Real GPU execution
  verified for this test only.**

## Remaining validation and packaging work

| Layer | State | Evidence / limitation |
|---|---|---|
| PCI detection and driver binding | **Verified** | `1ed5:0102`, live mtgpu, S30 render node |
| Runtime discovery and memory/copy operations | **Verified** | runtime 1.5.4 `musaInfo` + `muMemcpyTest` |
| GPU kernel execution | **Verified for smoke test** | 1,048,576-element vector-add passed; other workloads/features not covered |
| Local RPM integrity | **Blocked** | `musa-sdk-5.2.0-1.fc45.x86_64.rpm` fails `rpm -K` (header and payload digests) |
| Fedora/Copr package represents working host stack | **No** | current specs/builds package 5.2.0; host uses driver 3.1.0 + runtime 1.5.4 |
| Clean chroot install / dependency resolution | **Not validated** | successful Copr builds establish compile/package build only |

The runtime block described in earlier records is no longer current: IOMMU is
now enabled and the 1.5.4 runtime test succeeds. Do not install the current
5.2.0 DKMS package over the active module. Packaging must be reconciled with the
working 3.1.0/1.5.4 stack, rebuilt, checked for RPM integrity and tested in
isolated chroots before publication as a usable S30 stack.

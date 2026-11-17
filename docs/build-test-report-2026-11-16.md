# MUSA on Fedora — Historical Build & Test Report (2026-11-16)

> The initial host/runtime assessment below is retained as historical evidence
> and was superseded by the 2026-11-17 recheck at the end of this report.
> Copr build results remain relevant for build status only; they do not establish
> a working S30 install.

## Historical Summary
- RPM packaging: all 5 packages build locally and on COPR for all 5 Fedora x86_64 chroots.
- GPU detection: S30 present, mtgpu module live, /dev/mtgpu.0 present, IOMMU enabled.
- Driver init: module live but no mtgpu DRM render node exposed → compute interface unavailable.
- MUSA runtime: both 1.5.4 and 5.2.0 musaInfo fail ("CreatePlatform failed").
- GPU execution: NOT verified.

## COPR project
- Project: https://copr.fedorainfracloud.org/coprs/axogm/musa-mtt-s30/
- Chroots (all x86_64): fedora-43, 44, 45, rawhide, eln
- Builds (all succeeded):
  - libdrm-mtgpu 5.2.0-1 (build 11334175)
  - libmusa 5.2.0-1 (build 11334177)
  - musa-sdk 5.2.0-1 (build 11334270)
  - mtgpu-dkms 5.2.0-1 (build 11334274)
  - musa-toolkit 5.2.0-1 (build 11334275)

## Local build verification
- rpmbuild -ba for all 5 specs: exit 0.
- Payloads verified via rpm2cpio (libdrm-mtgpu 4 files, libmusa 7, musa-sdk ~4859, mtgpu-dkms 491, musa-toolkit 2).
- Host dependency check: libdrm 2.4.134, dkms 3.4.4, gcc 16.2.1, kernel-devel 7.3.3, glibc 2.44, make 4.4.1, elfutils-libelf-devel 0.197, openssl-devel 4.0.3 — all satisfy spec Requires.

## GPU detection (verified)
- lspci: 01:00.0 VGA 1ed5:0102 MTT S30, kernel driver in use: mtgpu
- /sys/module/mtgpu/initstate: live
- /dev/mtgpu.0 present
- /sys/kernel/iommu_groups: 13 groups (intel_iommu=on iommu=pt on cmdline)
- mtgpu aliases include pci:v00001ED5d00000102 (S30)

## Driver init (verified partial)
- Module loaded, device bound, /dev/mtgpu.0 present.
- NO mtgpu DRM render node: /sys/class/drm has only card1 (i915, Intel UHD 770). No renderD* bound to mtgpu.
- Therefore the compute/DRM interface the MUSA runtime needs is not exposed.

## MUSA runtime (verified FAIL)
- 1.5.4 musaInfo: "CreatePlatform failed! invalid argument(1)"
- 5.2.0 musaInfo: "CreatePlatform failed! no MUSA-capable device is detected (100)"
- mthreads-gmi: installed at /usr/local/musa/bin; not re-run for this report.

## GPU execution (NOT VERIFIED)
- Blocked by: no mtgpu render node + runtime cannot CreatePlatform.
- Underlying blockers (vendor-level, not fixable in this repo):
  1. Vendor core object mt-rm.o (containing rmapi_alloc) lacks IBT/ENDBR64 → unsafe to load on Fedora 7.x IBT-enabled kernels; cannot rebuild a safe module.
  2. MUSA SDK 5.2.0 catalog does not list S30 as a target; S30 not vendor-qualified on Fedora 45 + Intel.
  3. mtgpu DRM render node not created by the loaded 5.2.0 server driver on this host.

## Known limitations
- mtgpu-dkms RPM ships source only (vendor core object intentionally omitted — not IBT-safe).
- musa-sdk RPM is ~309MB (prebuilt vendor toolchain).
- No verified GPU execution on MTT S30 with this stack.
- Display output uses i915; S30 is compute-only here.

## Recheck — 2026-11-17 (Fedora 45 / kernel 7.3.3)

The sections above are the historical 2026-11-16 report and are superseded for
live-host runtime status by this recheck. No package installation, module
load/unload/rebind, or system configuration change was performed.

- PCI / binding: S30 `1ed5:0102` at `0000:01:00.0`; mtgpu module `live`;
  `/dev/mtgpu.0` and `/dev/dri/renderD129` present.
- IOMMU: `intel_iommu=on iommu=pt`; S30 in IOMMU group 11.
- Driver telemetry: `mthreads-gmi` reports driver 3.1.0, MTT S30, 4096 MiB.
- Runtime: MUSA 1.5.4 `musaInfo` succeeded and detected one device;
  `muMemcpyTest` succeeded.
- Compiler: MUSA 5.2.0 `mcc`, target `mp_10`.
- GPU execution: `scripts/verify-musa.sh` completed **11 passed, 0 failed,
  0 skipped** (including the host runtime API `tests/musart-smoke.c`);
  vector-add output was `PASS: 1048576 float adds verified on S30`.
  This verifies the tested kernel and runtime path only, not general workload
  compatibility.

### Packaging release gate (not passed)

- Copr project `axogm/musa-mtt-s30` has Fedora 43, 44, 45, ELN and Rawhide
  x86_64 chroots configured. The five recorded build IDs (11334175,
  11334177, 11334270, 11334274, 11334275) all report `succeeded` on Copr.
- These successful builds are for version 5.2.0 and do not demonstrate a
  working S30 install. The live validated stack is driver 3.1.0 + runtime
  1.5.4, with the 5.2.0 SDK compiler only.
- Local `packaging/rpm/RPMS/x86_64/musa-sdk-5.2.0-1.fc45.x86_64.rpm` fails
  `rpm -K` with bad header and payload digests, and rpm query reports it is not
  a valid RPM. The staged local build artifacts therefore are not releasable.
- No clean-chroot installation/dependency test was performed. Do not install
  the existing 5.2.0 DKMS package over the working module.
- Package publishing is intentionally held until the package source/version
  matches the verified stack and all RPM integrity/install checks pass.
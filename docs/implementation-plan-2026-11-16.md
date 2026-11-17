# MUSA on Fedora — MTT S30 — Implementation Plan (2026-11-16)

> Historical planning snapshot (2026-11-16). Its "still open" items were
> subsequently resolved; see the
> [2026-11-17 current state](current-state-2026-11-17.md) and the project
> [README](../README.md). Retained as a dated record.

Host: Fedora 45 Workstation, x86_64, kernel 7.3.3-300.fc45.x86_64
GPU: Moore Threads MTT S30 (`1ed5:0102`) at 0000:01:00.0

## 1. Verified evidence (already on host)

- S30 PCI `1ed5:0102` present and bound to `mtgpu` (3.1.0) — verified
- `/dev/mtgpu.0` present — verified
- IOMMU enabled (`intel_iommu=on iommu=pt`); 13 groups — verified
- Firmware installed in `/lib/firmware/mthreads/` — verified
- MUSA runtime 1.5.4 installed at `/usr/local/musa/` — verified
- MUSA 5.2.0 SDK at `/usr/local/musa-5.2/` (mcc/headers) — verified
- `mcc --offload-arch=mp_10` compiles `tests/musa-vector-add.mu` — verified
- mtgpu kernel module for kernel 7.3.3 links (5,504 objtool/warning lines, not
  load-approved in the earlier pass; current boot entry uses the vendor
  driver 3.1.0 as live module — initstate=live)

## 2. Still open (not claimed as verified)

- `musaInfo` / muInit: runtime platform creation returns
  `MUSA_ERROR_INVALID_VALUE` ("Unable to create hal platform") — blocked
- muMemcpyTest / vector-add execution on GPU — blocked on platform
- Real GPU kernel execution — blocked on runtime platform

## 3. Deliverables / success criteria

| Deliverable | Method | Status |
|---|---|---|
| Reproducible kernel module build for running kernel | `scripts/build-kernel-driver.sh` (isolated) | Done |
| RPM specs cleanly built | `rpmbuild -bs` (5 specs) | To do |
| Copr packages for all supported Fedoras + archs | `copr` CLI, all fedora chroots | To do |
| Monitor every Copr build, fix & rebuild | `copr watch-build` / `list-builds`, tail logs | To do |
| Clean-chroot install + dependency resolution | mock/dnf in disposable chroot | To do |
| Driver load test (current host) | `insmod` of locally built `mtgpu.ko` (safety gate first) | To do |
| Real GPU execution | gated `verify-musa.sh` + `test-gpu.sh` with MUSA_S30_TEST_APPROVED=1 | To do (honest limits) |
| Documentation + build logs + troubleshooting | new/updated docs | To do |

## 4. Build matrix (supported Fedora chroots)

- x86_64: fedora-44-x86_64, fedora-45-x86_64, fedora-eln-x86_64,
  fedora-rawhide-x86_64
- aarch64: fedora-44-aarch64, fedora-45-aarch64, fedora-eln-aarch64,
  fedora-rawhide-aarch64

Packages: `musa-sdk` (x86_64 only), `libmusa` (x86_64 only),
`libdrm-mtgpu` (x86_64 only), `mtgpu-dkms` (noarch), `musa-toolkit` (noarch).

## 5. Safety rules

- No kernel command line / no IBT, CET, or firmware changes.
- No overwriting of the live vendor module flow; the S30 remains bound to the
  vendor `mtgpu` 3.1.0 driver already loaded.
- Any system-level change (install, module load) requires explicit user
  confirmation via pkexec, following the project's script-gated style.
- All logs under `docs/` and project `.logs/` organized by phase.

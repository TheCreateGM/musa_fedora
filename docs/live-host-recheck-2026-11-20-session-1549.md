# MUSA on Fedora — live host recheck (2026-11-20, 15:49 +08:00)

This is a read-only host recheck. It supersedes the earlier same-day report
for facts re-measured here. No packages, kernel modules, firmware, boot
settings, or system configuration were changed. The MTT S30 driver was not
loaded, and no GPU workload or MUSA runtime query was run.

## Host and hardware

- Fedora Linux 45 Workstation, x86_64; running kernel
  `7.3.4-300.fc45.x86_64` (GCC 16.2.1).
- Intel UHD Graphics 770 at `0000:00:02.0` is bound to `i915` and drives the
  Wayland desktop.
- Moore Threads MTT S30 at `0000:01:00.0`, PCI `1ed5:0102`, and HDMI/DP audio
  function `0000:01:00.1`, PCI `1ed5:01ff`, are both unbound. The S30 is in
  IOMMU group 11. `/dev/dri/renderD128` belongs to Intel; there is no S30 render
  node or `/dev/mtgpu*` node.
- `mtgpu` is not loaded. `/etc/modprobe.d/blacklist-mtgpu.conf` still contains
  `blacklist mtgpu`. The headless option file uses
  `display=none disable_fbdev=1 mtgpu_drm_major=2` and does not set
  `rm_feature_level=1`.
- Secure Boot is disabled. The kernel command line includes
  `intel_iommu=on iommu=pt`; it does not include `ibt=off`.
- Installed packages: `mtgpu-dkms-3.1.0-11.fc45`,
  `mtgpu-firmware-3.1.0-2.fc45`, `musa-runtime-1.5.4-3.fc45`,
  `musa-sdk-5.2.0-6.fc45`, and `musa-toolkit-3.1.0-1.fc45`. DKMS reports
  mtgpu 3.1.0 installed for kernels 7.2.15, 7.3.3, and 7.3.4.
- Installed firmware includes MUSA and MTGPU firmware. Presence of firmware,
  a PCI ID in module metadata, or an installed package does not show that the
  driver initialized the GPU.

## Checks run in this recheck

| Check | Result |
|---|---|
| Kernel driver compile-only build | `scripts/build-kernel-driver.sh 7.3.4-300.fc45.x86_64` succeeded in an isolated source tree under `musa-data/research/builds/`; it did not install or load the module. The resulting module has matching 7.3.4 vermagic and S30 PCI aliases. SHA-256: `f0ce05f16aeaeaa47200be31c4004b79b024c298e3b19e9f99f1905ea550a947`. |
| Driver build warnings | 5,502 compiler/objtool warning lines, including vendor-core objtool warnings about naked returns and IBT/ENDBR. The proprietary core is opaque; a successful compile does not prove it safe to load. |
| MUSA compiler and headers | `MUSA_PATH=/usr/local/musa-5.2 ./scripts/test-toolchain.sh` passed. It produced a host executable containing MUSA fat-binary sections, compiled an explicit `mp_10` target, and syntax-checked runtime headers. The executables were not run; `GPU_RUNTIME_STATUS: NOT TESTED`. |
| Read-only hardware readiness check | `scripts/test/check-hardware.sh` found the S30 PCI function unbound, no MTT device nodes, and reported `GPU_TEST_STATUS: NOT RUN`; exit 3 is the expected blocked result. |
| Hardware validation ladder, no-load mode | `scripts/test/s30-hardware-validation.sh` passed environment and PCI identity checks, then reported `VALIDATION_STATUS: BLOCKED (driver not loaded)`; exit 3. The `--load` mode was not run. |
| RPM spec syntax | `rpmspec -P` succeeded for all five checked-in RPM specs. No package build/installation transaction was performed in this recheck. |
| Current-boot kernel journal | Contains S30 PCI enumeration, but no `mtgpu` probe or driver initialization messages. |

The temporary hardware-gate log directory created under `/tmp` by this recheck
was removed afterward. The isolated driver build and log remain under
`/home/axogm/Documents/project/musa-data/research/builds/`, the designated
research/build-data tree.

## MUSA component and compatibility boundary

The installed MUSA 5.2.0 compiler can generate `mp_10` code, which is the
architecture target the project's existing official-source research
associates with the S30. This is compile evidence only. The locally staged
vendor stack is mtgpu 3.1.0 plus runtime 1.5.4; the repository documents that
the separate 5.2.0 runtime is not ABI-compatible with that driver. The current
public support-matrix assessment in
[`vendor-sdk-availability.md`](vendor-sdk-availability.md) does not qualify
Intel + Fedora 45 + S30. Vendor-source research and downloaded reference
artifacts belong under `musa-data`; no new upstream source was downloaded in
this recheck.

Historical S30 workload results recorded elsewhere in this repository are
not results from this boot. No current runtime discovery, allocation, kernel
execution, Vulkan/OpenCL, display, video, or framework test is claimed here.
The prior GNOME/KMS conflict, kernel faults, and rebind fault remain relevant
safety evidence. The maintenance/load plan remains explicitly unapproved;
the blacklist was not altered and no physical driver-load attempt was made.

## Status

**Buildable and compiler-checkable; physical S30 path not validated in this
boot.** The successful isolated module build, matching PCI alias, firmware,
and `mp_10` compile are not enough to claim a functional or vendor-qualified
MUSA stack. Do not remove the blacklist, load/unload/rebind the module, change
boot settings, or run GPU workloads without review and explicit authorization
of the existing recovery plan.

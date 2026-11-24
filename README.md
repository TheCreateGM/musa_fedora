# MUSA on Fedora — MTT S30

MUSA driver/runtime packaging and test sources for the **Moore Threads MTT S30**
(`1ed5:0102`) on Fedora. The host is Fedora 45 / kernel `7.3.4-300.fc45.x86_64`.
Since **2026-11-24** the host runs with **compute mode enabled**: the boot-time
`blacklist mtgpu` has been removed (with a full backup and a `disable` path), so
udev binds the S30 on a normal boot. The change is recorded in
[compute-mode-enabled-2026-11-24](docs/compute-mode-enabled-2026-11-24.md); the
boot-time block remains available for hosts that want the S30 unbound by
default (`scripts/compute-mode.sh disable`). See the
[2026-11-24 hardware run](docs/hardware-validation-2026-11-24-load-test.md),
[current S30 suite record](docs/hardware-validation-2026-11-23-suite-verify.md),
[full host status](docs/current-state-2026-11-20.md), and the
[Live USB validation/build report](docs/session-2026-11-23-live-usb-validation.md).
For the repair of the installed system's disk boot (GDM crash caused by the
old base package shipping vendor Mesa DRI drivers), see
[the 2026-11-23 boot fix](docs/boot-fix-2026-11-23.md).

### Updates — 2026-11-24 (session)

- **`musa-runtime` 1.5.4-5 installed and validated.** It ships
  `/etc/profile.d/zz-musa-fedora.sh`, which puts the 1.5.4 runtime before the
  5.2.0 compiler prefix in login shells; the unowned legacy
  `/etc/profile.d/musa.sh` had resolved `musaInfo` to the incompatible 5.2
  binary. `scripts/check-musa-env.sh` now reports `MUSA_ENV_STATUS: PASS`.
- **Clean-boot hardware re-validation:** `verify-musa.sh` reported
  **13 passed, 1 failed, 1 skipped** on the live S30. The single failure is the
  vendor `mthreads-gmi` telemetry tool (DRM ioctl skew, root cause recorded);
  `musaInfo`, `muMemcpyTest`, the runtime API, `mp_10` vector-add, the compute
  suite, FP64 and headless Vulkan all pass.
- **Compute mode enabled** (02:08 UTC): blacklist moved to
  `/var/backups/musa-fedora-compute-mode/20261124T020848Z/`, initramfs
  regenerated; udev resolves the S30 to `kmod load mtgpu` and GNOME survived
  the live driver with no vendor DRI files installed. Post-reboot checks are
  listed in the [compute-mode record](docs/compute-mode-enabled-2026-11-24.md).
- **COPR:** 9/9 chroots, 20/20 checks pass; both projects publish
  `musa-runtime` 1.5.4-5, `musa-sdk` 5.2.0-6, `musa-toolkit` 3.1.0-3,
  `mtgpu-dkms` 3.1.0-12 and `mtgpu-firmware` 3.1.0-4.

A controlled load test on the installed system on 2026-11-23 ran the full MUSA
workload successfully on the S30; see the
[2026-11-23 load-test evidence](docs/hardware-validation-2026-11-23-load-test.md).
That test did not change the boot-time blacklist. To avoid the Mesa/GNOME
crash seen there, the vendor display drivers now live in an opt-in
`musa-runtime-display` subpackage; see the
[display-driver split](docs/display-driver-subpackage-2026-11-23.md).
The controlled load has since been repeated with `musa-runtime` 1.5.4-4 (the
version without the Mesa DRI drivers): **every functional step passed, GNOME
survived the load, and no kernel fault was logged** — see
[the 1.5.4-4 load-test evidence](docs/hardware-validation-2026-11-23-runtime154-load-test.md).

### Updates — 2026-11-23 (session)

- **The installed system boots from disk again.** The GDM crash fix
  ([boot fix](docs/boot-fix-2026-11-23.md)) took effect: the host now boots
  `/dev/nvme0n1p7` on kernel `7.3.4-300.fc45.x86_64`, GNOME runs on Intel
  `i915`, and `mtgpu` stays unloaded with the boot-time block in place.
- **All 9 configured COPR chroots verified**, for both `axogm/musa-fedora` and
  `axogm/musa-mtt-s30` (11 checks including both projects' build status): every package build reports `succeeded` and every
  chroot's published repository advertises `musa-runtime 1.5.4-4`,
  `musa-runtime-display 1.5.4-4`, `musa-sdk 5.2.0-6`, `musa-toolkit 3.1.0-3`,
  `mtgpu-dkms 3.1.0-12` and `mtgpu-firmware 3.1.0-4`. Re-run it with
  [`scripts/verify-copr-chroots.sh`](scripts/verify-copr-chroots.sh); see
  [the verification record](docs/copr-verification-2026-11-23.md).
- **The driver still builds from the repository source** for the running
  kernel (`7.3.4-300.fc45.x86_64`, vermagic verified, S30 PCI alias
  `pci:v00001ED5d00000102` present) and its module aliases are identical to
  the installed module — reproducible, not a one-off binary.
- **New:** [`scripts/compute-mode.sh`](scripts/compute-mode.sh) enables the
  S30 for compute on a normal boot (removing the boot-time block) with
  preflight guards, backups and a restore path, and
  [`docs/compute-mode.md`](docs/compute-mode.md) explains the workflow, the
  evidence behind it and the limits.
- **Resolved:** the controlled load test was repeated on 2026-11-23 07:38 UTC
  with `musa-runtime 1.5.4-4` installed. All steps passed (telemetry,
  `musaInfo`, `muMemcpyTest`, runtime API, `mp_10` vector-add/compute/fp64,
  headless Vulkan), GNOME/GDM survived the load, and no kernel fault was
  logged. This verifies that the Mesa/GNOME crash was caused by the packaged
  vendor DRI drivers and that 1.5.4-4 fixes it; see
  [the 1.5.4-4 load-test evidence](docs/hardware-validation-2026-11-23-runtime154-load-test.md).
  The boot-time block was still in place during that run; removing it
  permanently via `compute-mode.sh enable` remains a separate change requiring
  explicit approval. The subsequent full-suite record documents two earlier
  userspace aborts from the incompatible SDK `libmusart.so.5` ABI; corrected
  1.5.4-linked tests passed without an Oops/BUG. The smoke-test script now
  checks both executable SONAMEs before device initialization and blocks after
  `Invalid cmd_type` entries in the current boot journal; do not rerun GPU
  workloads until a reviewed recovery plan and clean boot.

The project targets the vendor's own S30 stack — **mtgpu driver 3.1.0** plus
**MUSA runtime 1.5.4** from `musa_3.1.0-pc_x86.deb` — and codes kernels with the
**MUSA 5.2.0 `mcc`** compiler for the `mp_10` (SuDi) architecture.

> This file records what has actually been verified on the hardware. Anything
> not listed as verified below is *not* claimed to work.

## Status

**Current host (2026-11-24):** compute mode is enabled. `/etc/modprobe.d` no
longer blacklists `mtgpu`, the regenerated initramfs carries no blacklist, and
udev matches the S30's PCI modalias to `kmod load mtgpu`. The module is live
with the documented headless parameters (`display=none disable_fbdev=1
mtgpu_drm_major=2`), `/dev/mtgpu.0` and an `mtgpu` DRM render node exist, and
GNOME runs on Intel `i915`. The vendor Mesa DRI drivers are not installed
(`musa-runtime-display` is optional), which removes the 2026-11-23 GDM crash
condition. The previous boot-time block is preserved in
`/var/backups/musa-fedora-compute-mode/20261124T020848Z/` and can be restored
with `scripts/compute-mode.sh disable`.

On this boot `scripts/verify-musa.sh` reports **13 passed, 1 failed, 1 skipped**;
the failure is the vendor `mthreads-gmi` telemetry tool, not the compute path
(see [its root cause](docs/root-cause-mthreads-gmi-drm-query-2026-11-24.md)).
The table below records the 2026-11-17, 2026-11-23 and 2026-11-24 hardware runs.

| Layer | State | Evidence |
|---|---|---|
| PCI enumeration | **verified** | `lspci -nn` → `1ed5:0102` |
| `mtgpu` driver / device nodes | **verified** | module live; S30 render node `/dev/dri/renderD130`; `/dev/mtgpu.0` |
| IOMMU | **verified** | `intel_iommu=on iommu=pt`; S30 assigned to IOMMU group 11 |
| Device telemetry | **verified via MUSA** | `musaInfo` reports MTT S30, 8 SMs, 3.97 GB; the separate `mthreads-gmi` 2.0.0 tool currently fails on a DRM ioctl skew (documented) |
| MUSA runtime discovery | **verified** | MUSA runtime 1.5.4 `musaInfo` reports one MTT S30 |
| GPU memcpy test | **verified** | vendor `muMemcpyTest` exited successfully |
| MUSA runtime host API | **verified** | `tests/musart-smoke.c` (alloc/copy/memset/streams/events) passed on the S30 |
| Compiler target | **verified** | MUSA 5.2.0 `mcc` compiled `mp_10` vector-add |
| GPU kernel execution | **verified** | vector-add completed; all 1,048,576 results matched |
| MUSA compute operations | **verified** | SAXPY, shared reduction, atomics, constant memory, device math, 2D launch, D2D copy, and FP64 passed |
| Headless Vulkan | **verified, limited** | S30 device/queue, buffer allocation, and host-visible memory map passed; no rendering test |
| Moore Threads OpenCL | **unavailable** | No MT ICD in the default loader; archived 2.1.1 ICD compute probe caused a kernel Oops (do not use) |
| Copr package builds | **verified 2026-11-23** | 9/9 chroots across both projects carry the current package set and every build reports `succeeded` — see [COPR verification](docs/copr-verification-2026-11-23.md) and `scripts/verify-copr-chroots.sh`; build success does not qualify driver loading |
| Driver rebuild from source | **verified 2026-11-23** | `scripts/build-kernel-driver.sh` rebuilds `mtgpu` for `7.3.4-300.fc45.x86_64`; vermagic matches and the module aliases equal the installed module's |
| Historical Fedora container + GPU test | **verified** | the 2026-11-17 five-RPM set installed in a throwaway Fedora 45 container and its packaged runtime and `mcc` ran on the S30: `13 passed, 0 failed` |
| Current Fedora 45 package install + compiler | **compile-only passed** | current Copr RPMs installed in a disposable container with scriptlets disabled; `rpm -V`, `dnf check`, and the packaged compiler/header smoke check passed; no GPU runtime query was made |

Historical verification command on 2026-11-17: `scripts/verify-musa.sh` — **13 passed,
0 failed, 1 skipped**. It must not be read as a current-host result. The skip is Moore Threads OpenCL: the loader found
MVGAL and Intel platforms, but no MT OpenCL ICD. A separate experiment with
an archived MobiMaliang 2.1.1 ICD triggered a kernel Oops during compute and
must not be repeated; see [the failure record](docs/current-state-2026-11-17.md).
The recorded tests used the installed
1.5.4 runtime and 5.2.0 compiler. They validate device discovery, copy and
host runtime APIs, eight compute operations, one `mp_10` vector-add, and a
headless Vulkan device/buffer/memory path. They do not validate display
rendering, video, `torch_musa`, or other frameworks, and they do not make the
separate 5.2.0 runtime suitable for S30 execution.

## Repository layout

```
driver-s30-3.1.0/     S30 driver source and compatibility/build files
scripts/              build, install, IOMMU, verification helpers
tests/                hardware tests (MUSA + raw ioctl ABI probes)
docs/                 verified analysis and compatibility notes
packaging/            RPM/DKMS packaging
```

Vendor packages, debs, extracted sources and research live outside this tree in
`../musa-data/`.

## Build the driver

```sh
scripts/build-kernel-driver.sh          # out-of-tree build for the running kernel
```

## Verify on the host

```sh
scripts/verify-musa.sh
```

It checks PCI/driver binding, the S30 render node, IOMMU groups, `mthreads-gmi`,
`musaInfo`, `muMemcpyTest`, the host runtime API (`tests/musart-smoke.c`), the
`mp_10` vector-add and compute suite, headless Vulkan device/buffer/memory use,
and Moore Threads OpenCL platform availability. It creates a temporary build
directory and does not load/unload modules or change system configuration, but
it does execute GPU workloads. Its recorded result above predates the OpenCL
kernel Oops. Do not rerun it in the boot that logged the Oops; resume GPU
validation only after recovery and a clean boot, with a reviewed test plan.

For an interactive shell, source the project environment first:

```sh
source scripts/musa-env.sh
```

The compiler and device-code headers come from MUSA 5.2.0, while the supported
S30 runtime and host API headers come from MUSA 1.5.4. The script puts the
1.5.4 runtime libraries before the SDK libraries because the system
`/etc/profile.d/musa.sh` currently places the separate 5.2.0 runtime first;
an unqualified `musaInfo` under that default environment aborted during this
recheck. Do not use the 5.2.0 runtime for S30 execution.

## Known limitations and packaging status

* The verified host uses the vendor mtgpu 3.1.0 stack and MUSA runtime 1.5.4;
  MUSA 5.2.0 is used only for its compiler/headers. The 5.2 runtime is not
  compatible with this 3.1.0 driver ABI; do not substitute it for runtime 1.5.4.
* The host-installed set at the 2026-11-23 controlled load was
  `mtgpu-dkms` 3.1.0-11, `mtgpu-firmware` 3.1.0-4, `musa-runtime` 1.5.4-4,
  `musa-sdk` 5.2.0-6, and `musa-toolkit` 3.1.0-3. COPR publishes
  `mtgpu-dkms` 3.1.0-12 (tmpfiles event-report directories; see the
  [COPR verification](docs/copr-verification-2026-11-23.md)) and
  `musa-runtime` 1.5.4-5. Both were installed on 2026-11-24 during a boot in
  which `mtgpu` was not loaded (a live-module replacement is refused by the
  package helper); the host now runs `musa-runtime` 1.5.4-5,
  `mtgpu-dkms` 3.1.0-12 and compute mode, with the boot-time blacklist no
  longer active (see
  [compute-mode-enabled-2026-11-24](docs/compute-mode-enabled-2026-11-24.md)).
  Newer package builds are not evidence of runtime compatibility.
* A Fedora 45 container test and packaged compiler/runtime test on the physical
  S30 passed on 2026-11-17, with scriptlets suppressed. This is historical
  evidence from a prior loaded-driver state, not a test of the current boot.
  The SDK packaging fixes included shipping `clang-offload-bundler` and
  requiring `gcc-c++` for the C++ linker/headers.
* The installed 1.5.4 runtime ships its own `musa_runtime.h` at
  `/usr/local/musa/include`; compile host runtime-API code against those
  headers, not the 5.2.0 SDK headers. `sizeof(struct musaDeviceProp)` is 728
  bytes in the 1.5.4 headers versus 1032 bytes in the 5.2.0 headers, so mixing
  them corrupts the device-properties ABI. Use the 5.2.0 SDK for the `mcc`
  compiler and device-code headers, and keep host code within the API exported
  by `libmusart.so.1.5.4`.
* Display output is not configured on the S30; i915 drives the desktop.
* The archived MobiMaliang 2.1.1 OpenCL ICD is unsafe with the active driver:
  its compute probe caused a kernel Oops in `mtgpu`. Do not install or run it.
* Historical unbind/rebind of this vendor module triggered a kernel NULL
  dereference. Do not unbind/rebind or replace the active module as part of
  routine tests. See `docs/root-cause-rebind-2026-11-16.md`.

## Safety

* The recorded tests are read-only apart from temporary files and do not install
  packages, change boot settings, or load/unload/rebind kernel modules.
* Since 2026-11-24 the host runs compute mode: the S30 is bound on a normal
  boot and remains bound after reboot. The previous boot-time block and the
  whole `/etc/modprobe.d` set are backed up under
  `/var/backups/musa-fedora-compute-mode/`; `scripts/compute-mode.sh disable`
  restores the unbound state. Do not unload the module or unbind/rebind the
  PCI device as part of routine tests (historical rebind fault:
  `docs/root-cause-rebind-2026-11-16.md`). The controlled-load runbook remains
  the safe path for a host that still blacklists the driver.
* The older boot that logged an `mtgpu` kernel fault during the archived OpenCL
  experiment also must not be used for GPU workloads. The reviewed procedure to
  resume hardware validation after a clean reboot is in
  [docs/clean-boot-resume-plan-2026-11-17.md](docs/clean-boot-resume-plan-2026-11-17.md);
  `scripts/test-gpu.sh` now refuses to run in a boot that already logged an
  `mtgpu` fault.
* Do not install a new DKMS package over the active driver as part of routine
  testing. The isolated package test suppresses scriptlets and does not build,
  load, or replace a kernel module.
* Reference packages, extracts, and system backups are kept under
  `/home/axogm/Documents/project/musa-data/`.

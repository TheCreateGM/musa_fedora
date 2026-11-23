# MUSA on Fedora — MTT S30

MUSA driver/runtime packaging and test sources for the **Moore Threads MTT S30**
(`1ed5:0102`) on Fedora. The latest live host inventory is Fedora 45 / kernel
`7.3.4-300.fc45.x86_64`; it found the S30 unbound and MUSA GPU tests blocked.
See the [latest live recheck](docs/live-host-recheck-2026-11-20-session-1549.md),
the [full host status](docs/current-state-2026-11-20.md), and the
[2026-11-23 Live USB validation/build report](docs/session-2026-11-23-live-usb-validation.md).
For the repair of the installed system's disk boot (GDM crash caused by an
auto-loaded `mtgpu`), see [the 2026-11-23 boot fix](docs/boot-fix-2026-11-23.md).

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
  `axogm/musa-mtt-s30`: every package build reports `succeeded` and every
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
  permanently via `compute-mode.sh enable` remains a separate, explicitly
  approved change.

The project targets the vendor's own S30 stack — **mtgpu driver 3.1.0** plus
**MUSA runtime 1.5.4** from `musa_3.1.0-pc_x86.deb` — and codes kernels with the
**MUSA 5.2.0 `mcc`** compiler for the `mp_10` (SuDi) architecture.

> This file records what has actually been verified on the hardware. Anything
> not listed as verified below is *not* claimed to work.

## Status

**Current host:** on a normal boot the S30 is present but unbound.
`/etc/modprobe.d/blacklist-mtgpu.conf` intentionally blocks the driver because
prior driver loads caused GNOME/KMS crashes; prior vendor-core faults also make
ad-hoc load/rebind unsafe. In the 2026-11-23 controlled test boot the module
was loaded once with headless parameters and remains loaded (the scripts never
unload it); a reboot restores the blacklisted, unbound state. The read-only
hardware check and gated GPU test report NOT RUN/BLOCKED on a normal boot. In the
2026-11-20 recheck, the isolated 7.3.4 driver build and MUSA 5.2.0 `mp_10`
compile/header checks passed, but no driver was installed or loaded and no GPU
workload ran. In a separate controlled load on 2026-11-23, the driver was
loaded once with headless parameters on the installed kernel and all functional
GPU steps passed (memcpy, runtime API, `mp_10` kernels, compute, fp64, Vulkan);
two informational tools failed and GNOME Shell crashed. That result is
documented in the
[2026-11-23 load-test evidence](docs/hardware-validation-2026-11-23-load-test.md).
It is not the state of a normal boot, where the S30 stays unbound. The table
below records both the 2026-11-17 and 2026-11-23 hardware runs.

| Layer | State | Evidence |
|---|---|---|
| PCI enumeration | **verified** | `lspci -nn` → `1ed5:0102` |
| `mtgpu` driver / device nodes | **verified** | module live; S30 render node `/dev/dri/renderD129`; `/dev/mtgpu.0` |
| IOMMU | **verified** | `intel_iommu=on iommu=pt`; S30 assigned to IOMMU group 11 |
| Device telemetry | **verified** | `mthreads-gmi` reports Driver Version 3.1.0, MTT S30, 4096 MiB |
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
  `musa-sdk` 5.2.0-6, and `musa-toolkit` 3.1.0-3. COPR now publishes
  `mtgpu-dkms` 3.1.0-12 (tmpfiles event-report directories; see the
  [COPR verification](docs/copr-verification-2026-11-23.md)), but it is not
  installed in this boot: the module is loaded and the package helper refuses
  transactions that would replace a live module. The boot-time blacklist is
  still in place for normal boots; the controlled test deliberately loaded the
  driver once. Newer package builds are not evidence of runtime compatibility.
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
* Current host state (2026-11-20): the S30 is unbound and `mtgpu` is
  blacklisted to protect the Intel-driven GNOME desktop after prior KMS
  crashes. No current-boot GPU test ran. See
  [the live inventory](docs/current-state-2026-11-20.md). Do not remove the
  blacklist or load/unbind/rebind the driver without a reviewed recovery plan;
  the draft plan is
  [docs/maintenance-plan-mtgpu-load-2026-11-20.md](docs/maintenance-plan-mtgpu-load-2026-11-20.md)
  and is not approved for execution yet.
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

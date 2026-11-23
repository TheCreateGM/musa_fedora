# MUSA on Fedora — MTT S30

MUSA driver/runtime packaging and test sources for the **Moore Threads MTT S30**
(`1ed5:0102`) on Fedora. The latest live host inventory is Fedora 45 / kernel
`7.3.4-300.fc45.x86_64`; it found the S30 unbound and MUSA GPU tests blocked.
See the [latest live recheck](docs/live-host-recheck-2026-11-20-session-1549.md),
the [full host status](docs/current-state-2026-11-20.md), and the
[2026-11-23 Live USB validation/build report](docs/session-2026-11-23-live-usb-validation.md).

The project targets the vendor's own S30 stack — **mtgpu driver 3.1.0** plus
**MUSA runtime 1.5.4** from `musa_3.1.0-pc_x86.deb` — and codes kernels with the
**MUSA 5.2.0 `mcc`** compiler for the `mp_10` (SuDi) architecture.

> This file records what has actually been verified on the hardware. Anything
> not listed as verified below is *not* claimed to work.

## Status

**Current host:** S30 is present but unbound. `/etc/modprobe.d/blacklist-mtgpu.conf`
intentionally blocks the driver because prior driver loads caused GNOME/KMS
crashes; prior vendor-core faults also make ad-hoc load/rebind unsafe. The
read-only hardware check and gated GPU test both report NOT RUN/BLOCKED. In the
2026-11-20 recheck, the isolated 7.3.4 driver build and MUSA 5.2.0 `mp_10`
compile/header checks passed, but no driver was installed or loaded and no GPU
workload ran. The following hardware rows describe only recorded, historical
2026-11-17 results, not the current boot state.

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
| Copr package builds | **see live status** | configured targets are Fedora 44/45, ELN, and Rawhide x86_64; the second project adds Fedora 43; build success does not qualify driver loading — see [build status](docs/current-state-2026-11-20.md) |
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
* The host-installed package set is `mtgpu-dkms` 3.1.0-11,
  `mtgpu-firmware` 3.1.0-2,
  `musa-runtime` 1.5.4-3, `musa-sdk` 5.2.0-6, and `musa-toolkit` 3.1.0-1.
  No package transaction was performed during the latest recheck. The
  `mtgpu` module remains blacklisted and unloaded; the maintenance plan for
  physical loading is still unapproved. Newer firmware/toolkit package builds
  are not evidence of runtime compatibility.
  Current spec releases, Copr build states, target chroots, and known compiler
  warnings are recorded in the [live status](docs/current-state-2026-11-20.md);
  successful builds do not imply current GPU functionality.
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

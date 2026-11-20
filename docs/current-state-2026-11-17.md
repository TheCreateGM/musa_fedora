# MUSA on Fedora — Historical state (2026-11-17)

This report records the live host check and Copr builds completed on
2026-11-17. It is superseded for current host status by
[`current-state-2026-11-20.md`](current-state-2026-11-20.md): the S30 is now
unbound and `mtgpu` is intentionally blacklisted after display/driver faults.
The hardware tests and Copr results below are retained as dated evidence only;
they do not describe current GPU availability or establish untested features.

## Host and hardware

- Fedora Linux 45 Workstation, x86_64; running kernel
  `7.3.3-300.fc45.x86_64`.
- Intel Core i5-13500 (20 logical CPUs) and Intel UHD Graphics 770 (`i915`),
  which drives the desktop.
- Moore Threads MTT S30 at `0000:01:00.0`, PCI ID `1ed5:0102`, with 4 GiB
  reported by the vendor management utility.
- `mtgpu` is loaded and reports `initstate=live`; `mthreads-gmi` reports
  driver version 3.1.0. `/dev/mtgpu.0` and the S30 DRM render node
  `/dev/dri/renderD129` are present.
- IOMMU groups are present and the S30 is assigned to a group. The host's
  active command line enables Intel IOMMU passthrough.
- RPM queries show the Fedora set installed: mtgpu-dkms 3.1.0-5,
  mtgpu-firmware 3.1.0-2, musa-runtime 1.5.4-3, musa-sdk 5.2.0-6, and
  musa-toolkit 3.1.0-1. The active module is at
  /lib/modules/7.3.3-300.fc45.x86_64/extra/mtgpu.ko.xz; DKMS reports 3.1.0
  installed. This recheck did not change packages, load/unload/replace/rebind
  a module, or change boot settings.

## Verified MUSA path on the physical S30

Command: `bash scripts/verify-musa.sh`.

| Check | Result |
|---|---|
| PCI identity, loaded driver, device node, render node | Passed |
| IOMMU groups | Passed |
| `mthreads-gmi` device/driver report | Passed; MTT S30, driver 3.1.0, 4096 MiB |
| MUSA 1.5.4 `musaInfo` | Passed; one device detected |
| Vendor `muMemcpyTest` | Passed |
| MUSA runtime host API (`tests/musart-smoke.c`) | Passed on the S30; alloc/free, H2D+D2H copies, memset, streams, events |
| MUSA 5.2.0 `mcc`, `mp_10` vector-add | Passed; 1,048,576 results verified on the S30 |
| MUSA compute suite | Passed; SAXPY, shared reduction, atomics, constant memory, device math, 2D grid, D2D copy, FP64 |
| Headless Vulkan | Passed; S30 device/queue creation, buffer allocation, host-visible map/readback |
| Moore Threads OpenCL | No MT platform/ICD detected; only MVGAL and Intel platforms reported |
| Overall | **13 passed, 0 failed, 1 skipped** |

The working combination exercised was the installed MUSA 1.5.4 runtime with
the MUSA 5.2.0 compiler targeting `mp_10`. The compute suite exercised the
operations listed above. The Vulkan probe stopped at device/queue creation and
buffer allocation/mapping; it did not render or present a frame. The OpenCL
check found no Moore Threads platform in the default system loader, so that
item was skipped. A separate experiment with an archived MobiMaliang 2.1.1
OpenCL ICD did enumerate a platform inconsistently, then triggered a kernel
Oops in `mtgpu` during an OpenCL compute request. The failure is detailed
below. Do not rerun that ICD or OpenCL compute probe. Other libraries,
frameworks, video, audio, display output, and general workload support remain
unvalidated.
The desktop remains on Intel graphics.

The unowned `/etc/profile.d/musa.sh` sets `LD_LIBRARY_PATH` to
`/usr/local/musa-5.2/lib`. During this recheck, launching the installed
1.5.4 `musaInfo` with that default environment aborted in its driver-library
initialization. Pinning `/usr/local/musa/lib` (runtime 1.5.4) first made
`musaInfo` succeed, including when the SDK library directory remained later
in the search path. `source scripts/musa-env.sh` now sets that safe ordering
for interactive use. No system profile or loader configuration was changed.
Do not replace the validated 1.5.4 runtime with the separate 5.2.0 runtime.

## Fedora RPM and Copr results

Copr project: [`axogm/musa-fedora`](https://copr.fedorainfracloud.org/coprs/axogm/musa-fedora/).
Its configured x86_64 chroots are Fedora 44, Fedora 45, Fedora ELN, and Fedora
Rawhide. The following current package builds all completed successfully in
all four chroots:

| Package | Version-release | Copr build |
|---|---:|---:|
| `mtgpu-dkms` | `3.1.0-5` | [11339362](https://copr.fedorainfracloud.org/coprs/build/11339362) |
| `mtgpu-firmware` | `3.1.0-2` | [11339153](https://copr.fedorainfracloud.org/coprs/build/11339153) |
| `musa-runtime` | `1.5.4-3` | [11339379](https://copr.fedorainfracloud.org/coprs/build/11339379) |
| `musa-sdk` | `5.2.0-6` | [11339529](https://copr.fedorainfracloud.org/coprs/build/11339529) |
| `musa-toolkit` | `3.1.0-1` | [11339164](https://copr.fedorainfracloud.org/coprs/build/11339164) |

The `mtgpu-dkms` build validates compilation and packaging against the kernel
development packages in each build chroot. Fedora 44 and 45 used kernel 7.3
headers; ELN and Rawhide used kernel 7.4 release-candidate headers.
The release 5 source selects the renamed
`drm/clients/drm_fbdev_helper.h` header for Linux 7.4 and newer. This follows
the upstream [DRM fbdev-helper move and header rename](https://git.zx2c4.com/linux-rng/commit/include/drm?id=6ccc5b5fc1d365a362055d4ae22547febcb80a34).

The runtime release 3 removes an obsolete `mesa-libglapi` package requirement:
the vendor runtime ships `libglapi_musa.so.0`, and Fedora 44+ does not provide
the old package name. SDK release 4 first declared a C++ header dependency for
`mcc`; the current release 6 ships the `clang-offload-bundler` link stage and
requires `gcc-c++`, which is what provides the `libstdc++.so` linker symlink
(see the container section below). SDK build 11339529 (release 6) succeeded in
all four chroots.

The rebuild history is retained under the ignored `.logs/copr-2026-11-17/`
directory. Build 11339303 (release 3) failed because `modinfo` was absent from
build roots and the source lacked Linux 7.4 DRM reset guards. Build 11339330
(release 4) passed Fedora 44/45 but exposed the 7.4 fbdev-helper header rename
on ELN/Rawhide. Release 5 fixed the header path and passed all four chroots.
The release 5 build also reports non-fatal objtool warnings from the vendor
core object; a successful package build is not a substitute for the physical
GPU test above.

The Fedora 45 RPMs for the five packages passed `rpm -K --nosignature` digest
verification. Copr's OpenPGP signatures were not independently trusted in the
container (`NOKEY`); the digest checks passed.

### Isolated container install and real-GPU test

`scripts/test-container-install.sh` installs the locally built RPM set into a
throwaway Fedora 45 container with only the S30 device nodes passed through, and
then exercises the packaged runtime and compiler against the physical MTT S30.
It suppresses RPM scriptlets (`tsflags=noscripts`), so no kernel module is
built, loaded, or replaced, and it does not touch the host package database.

The current run of the five-RPM set (`mtgpu-dkms` 3.1.0-5, `mtgpu-firmware`
3.1.0-2, `musa-runtime` 1.5.4-3, `musa-sdk` 5.2.0-6, `musa-toolkit` 3.1.0-1)
reports `CONTAINER_SUMMARY: 13 passed, 0 failed` and
`TEST_CONTAINER_INSTALL_STATUS: SUCCESS`. The 13 checks cover: the RPM set
installs and resolves its declared dependencies; the packaged files are present
(`libmusart.so.1.5.4`, `musaInfo`, `mthreads-gmi`, `mcc`, and both
`musa_runtime.h` headers); the C++ standard headers and the packaged `clang++`
resolve `<cmath>`; `musaInfo` discovers the passed-through S30; the vendor
`muMemcpyTest` runs; `tests/musart-smoke.c` builds and runs; and the packaged
`mcc` builds an `mp_10` vector-add that runs on the S30 and verifies 1,048,576
results.

Two real packaging defects were found and fixed through this gate:

- `clang-offload-bundler` was missing from the SDK payload, so every `mcc` link
  failed with `Executable "clang-offload-bundler" doesn't exist!`. Release 5
  ships it (together with `clang-offload-wrapper`).
- The SDK declared `libstdc++-devel`, but Fedora ships the unversioned
  `libstdc++.so` linker symlink only in `gcc-c++`; linking failed with
  `cannot find -lstdc++`. Release 6 requires `gcc-c++` instead, which pulls the
  C++ headers transitively.

Staging the SDK tarball needs roughly 3.5 GiB, so the staging and packaging
scripts must run with `TMPDIR=/var/tmp` here: `/tmp` is a small tmpfs and the
default staging directory exceeds its per-user quota.

## Known limits and safety

- The host hardware check exercised the already-running driver/runtime stack;
  it did not install or replace the kernel driver.
- A successful Copr build does not establish that DKMS can load on every
  supported kernel or that every device feature works.
- The 5.2.0 runtime is not substituted for runtime 1.5.4 in this verified S30
  path. Moore Threads' [public SDK catalog](https://developer.mthreads.com/resources/musa-sdk?tab=overview)
  currently lists the 5.2.0 Linux package for MTT S5000; it does not qualify
  Fedora or the MTT S30 for that release.
- Do not unbind/rebind or replace the active module as a routine test. A
  previous rebind attempt caused a kernel fault; see
  [`root-cause-rebind-2026-11-16.md`](root-cause-rebind-2026-11-16.md).
- The five RPMs are installed on the host, as verified with RPM queries;
  this recheck did not run an RPM transaction. Do not update or remove DKMS
  packages while the live mtgpu module is loaded. Back up the on-disk module
  before any later package transaction and retain the current recovery plan.
- The physical-host end-to-end test passed using the existing 1.5.4 runtime
  and 5.2.0 compiler, and the isolated container install plus real-GPU rerun of
  the current five-RPM set (SDK 5.2.0-6) passes end to end: 13 checks, 0
  failures. The container gate exercises the packaged `mcc` on the S30 as a
  fully installed set; it does not validate display rendering/presentation,
  video, or frameworks.

### Follow-up revalidation on 2026-11-17

- `scripts/test-toolchain.sh` passed its compile-only checks for the staged
  official 5.2.0 compiler, including explicit `mp_10` generation and runtime
  header syntax. It did not execute the generated binary.
- `scripts/build-kernel-driver.sh 7.3.3-300.fc45.x86_64` linked a fresh
  27 MiB module in
  `/home/axogm/Documents/project/musa-data/research/builds/mtgpu-7.3.3-300.fc45.x86_64-670BVq/`.
  Vermagic and the S30 PCI alias matched. The build emitted 5,502 compiler/
  objtool warning lines; the artifact was not installed or loaded and is not
  approved as a replacement for the live module.
- A fresh `scripts/verify-musa.sh` run passed 13 checks with 0 failures and
  skipped only Moore Threads OpenCL because no MT OpenCL ICD is installed.
- Sourcing `scripts/musa-env.sh` placed `/usr/local/musa/lib` (runtime 1.5.4)
  before the SDK libraries; `musaInfo` then detected the S30. No global
  profile, loader, boot, package, or kernel-module configuration was changed.

### Legacy OpenCL experiment and kernel Oops

After the verification run above, an isolated experiment used the archived
MobiMaliang 2.1.1 desktop ICD with the active `mtgpu` 3.1.0 module. The ICD
platform enumeration was inconsistent. Its compute smoke test triggered a
kernel Oops at `2026-11-17 16:18:38 +08:00` in process `opencl-s30-smok`, in
`PVRSRVRGXFlushComputeDataKM` while handling a DRM ioctl; the fault was a
supervisor write to address `0x1` in `mutex_lock`. The trace is archived at
`/home/axogm/Documents/project/musa-data/research/builds/opencl-mobi-2.1.1-on-mtgpu-3.1.0-2026-11-17/kernel-oops.log`.

The test process was stopped. Afterwards the module remained loaded and bound,
and `mthreads-gmi` plus the MUSA 1.5.4 `musaInfo` query still detected the S30.
Those status queries do not establish that the driver is unaffected. The
exact userspace call in flight is unknown; an old userspace/current-driver ABI
mismatch is plausible but unproven. Do not install the archived ICD or repeat
the compute probe. No module reload, unbind/rebind, package, boot, or system
OpenCL configuration change was made. No more GPU tests were run after the
Oops in this boot. See the archived experiment report in the same research
directory.

The kernel journal for this boot records a `BUG: kernel NULL pointer
dereference` (`Oops: 0002 [#1]`) at 16:18:38 reached via
`PVRSRVRGXFlushComputeDataKM` / `PVRSRVBridgeRGXFlushComputeData` /
`BridgedDispatchKM` in `mtgpu`, preceded at 16:18:36 by a trace through
`PhysmemNewLocalRamBackedPMR` / `PVRSRVBridgePhysmemNewRamBackedPMR`. After the
fault, `mtgpu 0000:01:00.0: irq:NNN affinity has changed.` messages continued.
The pre-reboot host snapshot is at
`musa-data/host-backups/pre-clean-boot-reboot-20261117-162913/` (live module
SHA-256 `21a3764b…01adad5`).

### Resume plan and gate

Resuming GPU validation requires a clean boot. The reviewed procedure is in
[clean-boot-resume-plan-2026-11-17.md](clean-boot-resume-plan-2026-11-17.md):
reboot the default `7.3.3-300.fc45` kernel (no boot-argument changes), run the
read-only pre-flight, then the gated smoke test and the full suite. The gate
[`scripts/test-gpu.sh`](../scripts/test-gpu.sh) now also reports
`GPU_TEST_STATUS: NOT RUN` with `this boot logged an mtgpu kernel fault` when
the running boot already logged an `mtgpu` `BUG`/`Oops`, so the suite cannot be
launched from a degraded boot by accident. The reboot is low risk for the
desktop (i915 drives display) and the S30 VGA function autoloads `mtgpu` via
`modules.alias` (`pci:v00001ED5d00000102…mtgpu`).

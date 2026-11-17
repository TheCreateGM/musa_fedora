# MUSA on Fedora — Current state (2026-11-17)

This report supersedes the 2026-11-13 host snapshots and the packaging
assessment in `docs/current-state-2026-11-16.md`. It records the live host
check and Copr builds completed on 2026-11-17. It does not claim features that
were not exercised.

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
- No MUSA or MTGPU RPM is installed on the host. This session did not install,
  load, unload, replace, or rebind any kernel module or change boot settings.

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
| Overall | **11 passed, 0 failed, 0 skipped** |

The working combination exercised was the installed MUSA 1.5.4 runtime with
the MUSA 5.2.0 compiler targeting `mp_10`. This verifies device discovery,
the vendor copy test, the host runtime API, and this vector-add only. It does
not validate other libraries, frameworks, graphics, video, audio, or general
workload support.
The desktop remains on Intel graphics.

The shell currently defaults `LD_LIBRARY_PATH` to `/usr/local/musa-5.2/lib`.
The successful verifier deliberately selects `/usr/local/musa/lib` (runtime
1.5.4). A direct probe using the separate 5.2.0 runtime failed on this host;
do not replace the validated 1.5.4 runtime with it.

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
- Host installation remains untested and is not needed to reproduce the
  container package test. Do not install the RPM stack over the working host
  environment without a separate recovery plan.
- The physical-host end-to-end test passed using the existing 1.5.4 runtime
  and 5.2.0 compiler, and the isolated container install plus real-GPU rerun of
  the current five-RPM set (SDK 5.2.0-6) passes end to end: 13 checks, 0
  failures. The container gate exercises the packaged `mcc` on the S30 as a
  fully installed set; it still does not validate graphics, video, or
  frameworks.

# MUSA on Fedora — MTT S30

Reproducible MUSA environment for the **Moore Threads MTT S30**
(`1ed5:0102`, 4 GiB, 2 cores) on **Fedora Linux 45 / kernel 7.3.3**.

The project targets the vendor's own S30 stack — **mtgpu driver 3.1.0** plus
**MUSA runtime 1.5.4** from `musa_3.1.0-pc_x86.deb` — and codes kernels with the
**MUSA 5.2.0 `mcc`** compiler for the `mp_10` (SuDi) architecture.

> This file records what has actually been verified on the hardware. Anything
> not listed as verified below is *not* claimed to work.

## Status

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
| Fedora RPM builds | **built** | all five packages build locally and pass `rpm -K --nosignature`; Copr publishes them for Fedora 44, Fedora 45, ELN, and Rawhide x86_64 — see [current status](docs/current-state-2026-11-17.md) |
| Isolated RPM install + GPU test | **verified** | the five-RPM set installs in a throwaway Fedora 45 container (scriptlets suppressed) and the packaged runtime and `mcc` run on the S30: `13 passed, 0 failed` |

Verification command on 2026-11-17: `scripts/verify-musa.sh` — **11 passed,
0 failed, 0 skipped**. The tests used the installed 1.5.4 runtime and 5.2.0
compiler. This establishes device discovery, the vendor copy test, the host
runtime API (allocation, copies, memset, streams, events), and one `mp_10`
vector-add kernel on this host. It does not validate OpenCL, graphics, video,
`torch_musa`, or other frameworks, and it does not make the separate 5.2.0 RPM
stack suitable for installation.

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
`musaInfo`, `muMemcpyTest`, the host runtime API (`tests/musart-smoke.c`), and
compiles/runs the `mp_10` vector-add. It creates a temporary build directory and
does not load/unload modules or change system configuration. The successful result recorded above is for this host and stack;
repeat it after driver, runtime, or kernel changes.

## Known limitations and packaging status

* The verified host uses the vendor mtgpu 3.1.0 stack and MUSA runtime 1.5.4;
  MUSA 5.2.0 is used only for its compiler/headers. The 5.2 runtime is not
  compatible with this 3.1.0 driver ABI; do not substitute it for runtime 1.5.4.
* The current package set is `mtgpu-dkms` 3.1.0-5, `mtgpu-firmware` 3.1.0-2,
  `musa-runtime` 1.5.4-3, `musa-sdk` 5.2.0-6, and `musa-toolkit` 3.1.0-1. All
  five build locally and pass `rpm -K --nosignature` digest verification. Copr
  publishes them for Fedora 44, Fedora 45, ELN, and Rawhide x86_64; see
  [current status](docs/current-state-2026-11-17.md) for the build numbers.
* The set was installed in an isolated Fedora 45 container with scriptlets
  suppressed and only the S30 device nodes passed through. The packaged
  `musaInfo`, the vendor `muMemcpyTest`, the runtime API smoke test, and the
  packaged `mcc` `mp_10` vector-add all run on the physical S30:
  `13 passed, 0 failed`. Two packaging defects were fixed to get there — the
  SDK now ships `clang-offload-bundler` (release 5) and requires `gcc-c++`,
  which provides the `libstdc++.so` linker symlink (release 6).
* The installed 1.5.4 runtime ships its own `musa_runtime.h` at
  `/usr/local/musa/include`; compile host runtime-API code against those
  headers, not the 5.2.0 SDK headers. `sizeof(struct musaDeviceProp)` is 728
  bytes in the 1.5.4 headers versus 1032 bytes in the 5.2.0 headers, so mixing
  them corrupts the device-properties ABI. Use the 5.2.0 SDK for the `mcc`
  compiler and device-code headers, and keep host code within the API exported
  by `libmusart.so.1.5.4`.
* Display output is not configured on the S30; i915 drives the desktop.
* Historical unbind/rebind of this vendor module triggered a kernel NULL
  dereference. Do not unbind/rebind or replace the active module as part of
  routine tests. See `docs/root-cause-rebind-2026-11-16.md`.

## Safety

* The recorded tests are read-only apart from temporary files and do not install
  packages, change boot settings, or load/unload/rebind kernel modules.
* Do not install a new DKMS package over the active driver as part of routine
  testing. The isolated package test suppresses scriptlets and does not build,
  load, or replace a kernel module.
* Reference packages, extracts, and system backups are kept under
  `/home/axogm/Documents/project/musa-data/`.

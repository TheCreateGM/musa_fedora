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
| Compiler target | **verified** | MUSA 5.2.0 `mcc` compiled `mp_10` vector-add |
| GPU kernel execution | **verified** | vector-add completed; all 1,048,576 results matched |
| Fedora RPM/Copr builds | **built** | current package releases succeeded in Fedora 44, Fedora 45, ELN, and Rawhide chroots; see [current status](docs/current-state-2026-11-17.md) |
| Isolated RPM install | **partially verified** | Fedora 45 installed the Copr packages and runtime discovery passed; the latest SDK dependency fix still needs an isolated reinstall/compile run |

Verification command on 2026-11-17: `scripts/verify-musa.sh` — **10 passed, 0 failed**.
The execution test used the installed 1.5.4 runtime and 5.2.0 compiler. This
establishes a working vector-add path on this host; it does not validate other
MUSA features or make the separate 5.2.0 RPM stack suitable for installation.

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
`musaInfo`, `muMemcpyTest`, and compiles/runs the `mp_10` vector-add. It creates
a temporary build directory and does not load/unload modules or change system
configuration. The successful result recorded above is for this host and stack;
repeat it after driver, runtime, or kernel changes.

## Known limitations and packaging status

* The verified host uses the vendor mtgpu 3.1.0 stack and MUSA runtime 1.5.4;
  MUSA 5.2.0 is used only for its compiler/headers. The 5.2 runtime is not
  compatible with this 3.1.0 driver ABI; do not substitute it for runtime 1.5.4.
* The current Copr package set is `mtgpu-dkms` 3.1.0-5,
  `mtgpu-firmware` 3.1.0-2, `musa-runtime` 1.5.4-3, `musa-sdk` 5.2.0-4, and
  `musa-toolkit` 3.1.0-1. All five builds succeeded in Fedora 44, Fedora 45,
  ELN, and Rawhide chroots. The runtime and SDK specs now avoid a Fedora 45
  dependency failure and declare the standard C++ headers needed by `mcc`.
* The package set was installed in an isolated Fedora 45 container with
  scriptlets suppressed. Packaged `musaInfo` detected the passed-through S30.
  SDK release 3 lacked the C++ header dependency, so the container compile
  failed; release 4 fixes the dependency and builds in Copr, but has not yet
  been reinstalled and rerun in the container.
* The 3.1.0 runtime headers are incomplete for compiling; use the 5.2.0 SDK
  compiler/headers while linking to the installed 1.5.4 runtime. Keep test code
  within the API exported by `libmusart.so.1.5.4`.
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

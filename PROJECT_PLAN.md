# MUSA on Fedora 45 — MTT S30 Project Plan

## Objective

Build a reproducible MUSA environment for the MTT S30 while keeping
production code in `/home/axogm/Documents/project/musa-fedora` and reference
packages, vendor source, SDK extracts, dependencies, and research outputs in
`/home/axogm/Documents/project/musa-data`.

Hardware checks recorded before the later OpenCL kernel Oops verified an MUSA
path on the MTT S30: driver `mtgpu` 3.1.0 was loaded and live, runtime 1.5.4
detected the device and passed the vendor copy test, and the 5.2.0 `mcc`
compiler produced `mp_10` vector-add and compute tests that ran on the S30. A
headless Vulkan device/buffer/memory test also passed. The system has no Moore
Threads OpenCL ICD, and an archived 2.1.1 ICD compute probe triggered the
Oops. These checks validate only the exercised operations; they do not
establish display rendering, video, framework, or general library support.
PCI discovery or a successful compile alone is still not hardware validation.

## Latest live host status — 2026-11-20

Fedora 45 runs kernel `7.3.4-300.fc45.x86_64`. The MTT S30 is visible on PCI
at `0000:01:00.0` (`1ed5:0102`). Root cause analysis of driver loading faults
was completed: setting `rm_feature_level=1` activated an incomplete vendor
Resource Manager path in `mtgpu_core.o_binary` causing an immediate NULL pointer
dereference in `gpu_phys_heap_alloc_mem` (CR2: 0x128). Removing `rm_feature_level=1`
and preserving `rm_feature_level=0` restores the verified PVR memory model.
`mtgpu-dkms` was updated to `3.1.0-9` (Copr build [11365750](https://copr.fedorainfracloud.org/coprs/build/11365750),
succeeded on Fedora 44, 45, ELN, Rawhide x86_64) shipping the corrected
`90-mtgpu-headless.conf` (`display=none disable_fbdev=1 mtgpu_drm_major=2`)
and removing an unsupported claim that a GNU IBT property note establishes
safe runtime behavior. The host still has release 7 installed and the S30
remains unbound/blacklisted; releases 8 and 9 were not installed on the host.
`mtgpu-firmware` was later re-released as `3.1.0-4` (Copr builds
[11365858](https://copr.fedorainfracloud.org/coprs/build/11365858) primary and
[11365859](https://copr.fedorainfracloud.org/coprs/build/11365859) second
project): `/usr/lib/firmware` usr-merge layout, `x86_64`-only packaging, and
no strip pass on GPU ELF blobs — the published payload is byte-identical to
the installed 3.1.0-2 blobs. `musa-toolkit` was re-released as `3.1.0-3`
(Copr builds [11365857](https://copr.fedorainfracloud.org/coprs/build/11365857)
and [11365856](https://copr.fedorainfracloud.org/coprs/build/11365856)) with
an honest, historical-results-only description. Both RPMs verified
`digests signatures OK` against the project Copr key.
The later [same-day live recheck](docs/live-host-recheck-2026-11-20-session-1549.md)
confirms the currently installed DKMS package is 3.1.0-11 (not release 7),
but the S30 remains unbound and blacklisted. A fresh isolated build for the
running 7.3.4 kernel and MUSA `mp_10` compile/header checks passed; the module
was not installed or loaded, and no runtime or GPU workload was run. The
physical-load plan remains unapproved. See also
[`docs/current-state-2026-11-20.md`](docs/current-state-2026-11-20.md) and
[`docs/root-cause-rm-feature-level-2026-11-20.md`](docs/root-cause-rm-feature-level-2026-11-20.md).

## Historical host status — 2026-11-17 (superseded)

- Fedora Linux 45 Workstation, x86_64, kernel `7.3.3-300.fc45.x86_64`.
- Intel Core i5-13500 and Intel UHD Graphics 770 (`i915`), which drives the
  desktop.
- MTT S30 at `0000:01:00.0`, PCI `1ed5:0102`, 4 GiB; vendor audio function
  `1ed5:01ff`. The desktop stays on Intel `i915`; the S30 does not drive
  display output, though a headless Vulkan device/memory path passed.
- Historical 2026-11-17: `mtgpu` driver 3.1.0 was loaded and live (`initstate=live`); module
  `/lib/modules/7.3.3-300.fc45.x86_64/extra/mtgpu.ko`, vermagic
  `7.3.3-300.fc45.x86_64`. `/dev/mtgpu.0` and the S30 DRM render node
  `/dev/dri/renderD129` are present.
- IOMMU enabled: `intel_iommu=on iommu=pt`, 13 IOMMU groups, S30 in group 11.
- Firmware installed under `/lib/firmware/mthreads/`.
- MUSA runtime 1.5.4 at `/usr/local/musa/` (`libmusart.so.1.5.4`); MUSA 5.2.0
  SDK (compiler `mcc`, headers) at `/usr/local/musa-5.2/`.
- `mthreads-gmi` reports driver version 3.1.0, MTT S30, 4096 MiB. MUSA 1.5.4
  `musaInfo` detects one device; vendor `muMemcpyTest` passes.
- MUSA 5.2.0 `mcc` built `tests/musa-vector-add.mu` for `--offload-arch=mp_10`
  and it ran on the S30: `PASS: 1048576 float adds verified on S30`.
- `tests/musart-smoke.c` (host runtime API: device count, properties,
  alloc/free, H2D+D2H copies, memset, streams, events) passed on the S30.
- The recorded `scripts/verify-musa.sh` run: **13 passed, 0 failed, 1
  skipped** (no Moore Threads OpenCL ICD); it completed before a later legacy
  OpenCL ICD experiment triggered an `mtgpu` kernel Oops. Do not run more GPU
  tests in this boot; see `docs/current-state-2026-11-17.md`.
- MUSA compute checks on `mp_10`: SAXPY, shared reduction, integer atomics,
  constant memory, device math, 2D grid, device-to-device copy, and FP64 passed.
- Headless Vulkan S30 device creation, buffer allocation, and host-visible
  mapping passed; no rendering or display output was tested.
- Supported combination: driver `mtgpu` 3.1.0 + MUSA runtime 1.5.4 + MUSA 5.2.0
  compiler targeting `mp_10`. The separate 5.2.0 *runtime* is not compatible
  with the 3.1.0 driver ABI and must not be substituted for 1.5.4.

See [current-state-2026-11-17.md](docs/current-state-2026-11-17.md) for the
full record.

The current boot logged an `mtgpu` kernel fault (a NULL-pointer dereference in
the PVR/DDK compute bridge, `PVRSRVRGXFlushComputeDataKM`) during the archived
OpenCL experiment, so no further GPU workload runs in this boot. The reviewed
procedure to resume hardware validation after a clean reboot is in
[clean-boot-resume-plan-2026-11-17.md](docs/clean-boot-resume-plan-2026-11-17.md).
The GPU gate `scripts/test-gpu.sh` now also refuses to run when the current
boot already logged an `mtgpu` fault.

## Historical live host status — 2026-11-13 (superseded)

- Fedora Linux 45 Workstation, x86_64, kernel `7.3.3-300.fc45.x86_64`.
- Intel Core i5-13500 and Intel UHD Graphics 770 (`i915`), which remains the
  active display GPU.
- MTT S30 at `0000:01:00.0`, PCI `1ed5:0102`, visible but unbound.
- The unowned, boot-loaded `mtgpu.ko` failed initialization with
  `Missing ENDBR: rmapi_alloc`; it remains resident in `initstate=coming`,
  taint `OE`. An unowned module file is currently present in the 7.3.3 tree;
  its hash differs from the file quarantined on 2026-11-12 and its origin is
  unknown. Do not query or operate the GPU or unload the module.
- `/dev/mtgpu.0` and an S30-associated DRM render node are absent.
- QEMU/KVM and `/dev/kvm` are available, but the S30 functions have no IOMMU
  group in this boot; safe PCI passthrough to a VM is not available without
  a separate host configuration change.
- The only other installed kernels are Fedora 44/45 `7.2.15`; their configs
  also enable kernel IBT. No installed kernel is within the 4.19–5.15 range in
  the official S30 desktop driver v2.5.0 notes.
- Secure Boot is disabled; SELinux is enforcing. The active command line has
  no `ibt=off` or `noibt`; this boot faulted with IBT active.

See [the live host report](docs/live-host-recheck-2026-11-13.md) for the
historical hashes, logs, and build/test evidence. The
[2026-11-12 report](docs/live-host-recheck-2026-11-12.md) documents the
earlier quarantine action.

## Historical evidence and technical findings

| Layer | Evidence | Status |
|---|---|---|
| PCI/device discovery | S30 enumerates as `1ed5:0102`; `mtgpu` 3.1.0 live | Verified |
| Official kernel source | Separate MUSA 5.2.0 DKMS source includes S30 PCI ID | Source recognition only; not the source used by the active 3.1.0 build |
| Active S30 driver source | Project `src/` and `inc/` identify as package version 3.1.0 | DKMS build is installed but not loaded; the S30 is unbound and the module is blacklisted |
| Legacy S30 desktop bundle | Official 2.1.1 source recognizes S30 ID `1ed5:0102` | Historical; DKMS build fails against 7.3.3 and its bundled core has no IBT notes/ENDBR |
| Fedora kernel port | Isolated 3.1.0 `mtgpu.ko` links for kernel 7.3.3 | Historical build artifact had 5,504 compiler/objtool warning lines; the vendor-loaded module is live |
| MUSA compiler | Product filing maps S30 to SuDi; architecture docs map SuDi to `MP_10`; 5.2.0 compiler emits `mp_10` | Verified on the S30: the `mp_10` vector-add ran |
| Runtime | MUSA 1.5.4 `musaInfo`, `muMemcpyTest`, `musart-smoke` | Verified |
| Physical GPU | Runtime API, vector-add, eight compute operations, headless Vulkan device/buffer/memory | Verified for these tests only |
| SDK support | Official 5.2.0 catalog lists S5000 packages, not S30; S30 docs do not list Fedora | Fedora 45 + S30 not vendor-qualified |

Official source, packages, package hashes, and compiler extracts are under
`musa-data/research/official-musa-5.2.0/`. The official source PCI table
recognizes S30 but does not prove runtime compatibility. The product filing
identifies S30 as SuDi, the MUSA architecture guide maps SuDi to `MP_10`, and
the compiler manual lists `mp_10` as an offload target. This supports `mp_10`
as the S30 code-generation target. The separate 5.2.0 runtime is not compatible
with the working 3.1.0 driver ABI; the verified runtime on this host is 1.5.4.
Evidence is recorded in
`musa-data/research/official-musa-5.2.0/s30-mp10-target-evidence-2026-11-12.md`.

## Completed work

1. Inspected Fedora, kernel, PCI devices, graphics stack, development headers,
   security state, local SDK, and module trees.
2. Staged official driver source and SDK packages under `musa-data` and
   recorded hashes and package provenance.
3. Ported the S30 3.1.0 kernel source far enough to compile/link on kernel 7.3.3.
4. Made the build script copy the source to `musa-data`, verify the pinned
   vendor-core hash, and keep all generated artifacts there. The 2026-11-13
   build linked for 7.3.3 but remains unsafe to load.
5. Re-ran official MUSA 5.2.0 compile-only checks; the default bundle,
   explicit `mp_10`, and runtime-header syntax check passed.
6. Ran the read-only host inventory and the gated GPU script during the
   unbound-driver investigation; neither opened the GPU node.
7. Backed up and reversibly quarantined the then-present unowned module file
   on 2026-11-12. A different unowned module is present in the running tree
   as of 2026-11-13; its origin is unknown.
8. Audited the archived official 2.1.1 desktop package and performed an
   isolated 7.3.3 build probe; the legacy source fails before link and remains
   research-only.
9. Brought up the live S30 stack: vendor `mtgpu` 3.1.0 loaded and live, runtime
   1.5.4 `musaInfo` and `muMemcpyTest` pass, and the `mcc` `mp_10` vector-add
   ran on the GPU.
10. Added `tests/musart-smoke.c` and showed the host runtime API passes on the
   S30; the expanded `scripts/verify-musa.sh` reports 13 passed, 0 failed,
   and 1 skip for the unavailable Moore Threads OpenCL ICD. A subsequent
   archived ICD compute probe triggered a kernel Oops; that probe is disabled
   and no more GPU tests were run in this boot.
11. Built five Fedora RPM specs (`mtgpu-dkms` 3.1.0-5, `mtgpu-firmware`
    3.1.0-2, `musa-runtime` 1.5.4-3, `musa-sdk` 5.2.0-6 compiler-only,
    `musa-toolkit` 3.1.0-1) targeting the working stack.
12. Installed the five-RPM set in an isolated Fedora 45 container with the S30
    device nodes passed through and scriptlets suppressed; `scripts/test-container-install.sh`
    reports 13 passed, 0 failed, with the packaged runtime and `mcc` running on
    the physical S30. Two packaging defects were fixed (SDK release 5 ships
    `clang-offload-bundler`; release 6 requires `gcc-c++`). Copr build 11339529
    publishes SDK 5.2.0-6 in all four chroots.
13. Updated `mtgpu-dkms` through releases 6, 7, and 8. Release 6 added headless
    VPU layout compatibility in `inc/s30_vpu_compat.h`. Release 7 fixed buildroot
    udev macro expansion via `BuildRequires: systemd-rpm-macros` (Copr build
    11365567). Release 8 eliminated the fatal `rm_feature_level=1` parameter from
    `90-mtgpu-headless.conf`, restoring the stable PVR memory model (Copr build
    11365657 succeeded across Fedora 44, 45, ELN, Rawhide x86_64).
14. Release 9 removed the unsupported IBT/CET load-safety claim; Copr build
    11365750 succeeded across Fedora 44, 45, ELN, and Rawhide x86_64. The full
    logs still contain 5,486 objtool warning lines in each chroot. The Fedora
    45 RPM passed signature/digest checks and a dependency-resolving container
    install with scriptlets disabled. The host remains on release 7 and the
    GPU stays blacklisted/unbound.
15. Rebuilt the current five-package set in the separately configured
    `musa-mtt-s30` Copr project to cover its Fedora 43 target. Builds 11365759
    through 11365763 succeeded across Fedora 43, 44, 45, ELN, and Rawhide
    x86_64. Fedora 43 logs were inspected; the DKMS build still emits 5,486
    objtool warnings. See the [current validation report](docs/current-state-2026-11-20.md).
16. Downloaded the Fedora 45 RPMs from the primary Copr project and
    independently verified all five signatures/digests against its key. DNF
    installed the set in a disposable Fedora 45 container with scriptlets
    disabled; `rpm -V` and `dnf check` passed. The installed compiler passed
    the compile-only smoke test, including explicit `mp_10` output. No runtime
    or GPU call was made.

17. Closed the CUDA-compatibility packaging gaps in session 4 (2026-11-27):
    `musify-text` (export-derived CUDA↔MUSA porting tool, `/usr/bin`), CMake
    discovery (`/usr/lib64/cmake/musa`: `find_package(MUSA)` config + module,
    `musa_add_executable`/`musa_add_library` pinned to mcc 5.2.0 + libmusart
    1.5.4 + mp_10), and the generated `musa_rt154_names.h` runtime-ABI pin.
    Published as `musa-cuda-compat-1.0.0-2` on both COPR projects; hardware +
    container gates 16/0/0 and 16/0; local signing key rotated
    (`BFAB85E6884C116DAA985DF0C2B44B18E3841DC2`) after the old key proved
    passphrase-protected against headless signing. Record:
    `docs/hardware-validation-2026-11-27-session4.md`.

## Remaining work

Updated 2026-11-27 — most items listed here historically are now done: the
six-package RPM set builds on COPR for fedora-43/44/45/eln/rawhide
(`scripts/verify-copr-chroots.sh` now **24/24**: the map gained the
`fedora-43` chroot that had been enabled in `musa-fedora` on 2026-11-26 but
was never asserted, and the gate cross-checks its map against the COPR API),
the isolated container install test
passes (13/0), VA-API decode works (14/0/0), compute mode is enabled across
reboots, and LLM inference runs on the S30 via `apps/musa-infer` (see
[docs/hardware-validation-2026-11-27-reverify.md](docs/hardware-validation-2026-11-27-reverify.md)).
Later the same day the two open CUDA-compatibility packaging items were
closed in `musa-cuda-compat-1.0.0-2`: `musify-text` and CMake discovery
(`find_package(MUSA)`, `musa_add_executable`), both hardware-verified
(`tests/cuda-compat` **16/0/0** host and **16/0** container; COPR builds
11397615/11397626), and the local signing key was rotated to an genuinely
unprotected batch key — see
[docs/hardware-validation-2026-11-27-session4.md](docs/hardware-validation-2026-11-27-session4.md).
The items below are the ones that remain open.

### 1. RPM OpenPGP signing — done 2026-11-27

The local build tooling produces signed artifacts and the trust setup is
documented in [docs/rpm-signing.md](docs/rpm-signing.md). Closed on
2026-11-27: all **8/8** local-build RPMs (the six-package set plus
`musa-runtime-display` and `musa-cuda-compat`) were signed with the local
build-signing key `763A7D1470E1672E236F290EFBA85A80C8D8BC89`, and that public
key was then imported into the system RPM keyring through `pkexec rpm --import`,
so plain `rpm -K` now reports `digests signatures OK` on the whole set. The
import is reversible (`pkexec rpm -e gpg-pubkey-763a7d1470e1672e236f290efba85a80c8d8bc89`),
and packages published through Copr keep Copr's own build-service signature.
Standing requirement: re-run `scripts/rpm-signing.sh sign` after every local
rebuild, because a new package is unsigned until signed.

### 2. Broader feature and framework support (vendor-dependent)

Verified on hardware: device discovery, host runtime APIs, eight `mp_10`
compute operations, headless Vulkan, VA-API decode, and the self-contained
`musa-infer` LLM path. Still absent, and blocked on the vendor rather than this
repository:

* No Moore Threads OpenCL ICD safe with the 3.1.0 driver (the archived 2.1.1
  ICD compute probe oopsed the kernel and must not be rerun).
* No `mublas`/`mudnn` for `mp_10`; `torch_musa` issue #134 excludes the S30, so
  generic frameworks cannot target this GPU. `apps/musa-infer` is the working
  alternative, not a framework replacement.
* S30 display output is not configured; i915 drives the desktop.
* The `MKT_NAME` query sub-command dispatch lives inside the proprietary
  `objs/x86_64/mtgpu_core.o_binary`; it cannot be added from open source
  (evidence: `musa-data/research/video-decode-codec-enum-mismatch-2026-11-26.md`
  §4.3c).

### 3. Vendor qualification

The supported S30 runtime/driver/firmware matrix and Fedora/Intel
qualification still need Moore Threads' input; a support-request draft is in
[docs/vendor-support-request-draft.md](docs/vendor-support-request-draft.md).
The 5.2.0 runtime remains incompatible with the 3.1.0 driver ABI and must not
replace 1.5.4.

### 4. Kernel driver safety

The active module is the vendor 3.1.0 driver and is live. Do not unbind/rebind
or replace it as a routine test: a historical rebind caused a kernel fault.
Do not disable IBT/CET.

## Safe commands

```sh
# GPU test suite: do not run in the boot that logged the OpenCL kernel Oops.
# Recorded pre-Oops result: 13 passed, 0 failed, 1 OpenCL skip.
./scripts/verify-musa.sh

# Compile-only MUSA compiler/header check; does not run the executable
./scripts/test-toolchain.sh

# Fresh kernel build under musa-data; never installs or loads the module
./scripts/build-kernel-driver.sh "$(uname -r)"

# Read-only hardware-test gate; must report NOT RUN while any blocker exists
./scripts/test-gpu.sh

# Read-only preview or exact-hash recovery action
bash scripts/quarantine-unvalidated-mtgpu.sh --plan
pkexec /usr/bin/bash /home/axogm/Documents/project/musa-fedora/scripts/quarantine-unvalidated-mtgpu.sh --restore
```

Do not run `make clean` in the production tree; the Makefile refuses broad
cleanup when local staging exists. Do not install the Ubuntu DEB packages,
load the fresh or archived driver artifacts, disable IBT/CET, or alter the
proprietary core object.

## Project layout

- `src/`, `inc/`, and `Makefile`: production kernel-driver port and build.
- `scripts/`: isolated build, read-only inventory, compile-only checks, gated
  GPU test, and exact-hash quarantine/restore script.
- `tests/`: non-invasive hardware probe and MUSA compile-only source.
- `docs/`: compatibility status, build evidence, safety limits, and recovery.
- `musa-data/research/`: vendor packages/source, SDK extracts, logs, and
  generated build outputs.

# MUSA on Fedora 45 — MTT S30 Project Plan

## Objective

Build a reproducible MUSA environment for the MTT S30 while keeping
production code in `/home/axogm/Documents/project/musa-fedora` and reference
packages, vendor source, SDK extracts, dependencies, and research outputs in
`/home/axogm/Documents/project/musa-data`.

The project is not a functional MUSA stack yet. PCI discovery and compiler
output are not hardware validation.

## Live host status — 2026-11-13

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

See [the live host report](docs/live-host-recheck-2026-11-13.md) for current
hashes, logs, and build/test evidence. The
[2026-11-12 report](docs/live-host-recheck-2026-11-12.md) documents the
earlier quarantine action.

## Evidence so far

| Layer | Evidence | Status |
|---|---|---|
| PCI/device discovery | S30 enumerates as `1ed5:0102` | Verified |
| Official kernel source | Separate MUSA 5.2.0 DKMS source includes S30 PCI ID | Source recognition only; not the source used by the active 3.1.0 S30 build |
| Active S30 driver source | Project `src/` and `inc/` identify as package version 3.1.0 | Isolated build links; not safe to load |
| Legacy S30 desktop bundle | Official 2.1.1 source recognizes S30 ID `1ed5:0102` | DKMS build fails against 7.3.3; bundled core has no IBT notes/ENDBR and objtool reports 362 `!ENDBR` relocations |
| Fedora kernel port | Fresh isolated 3.1.0 `mtgpu.ko` links for kernel 7.3.3 | 5,504 compiler/objtool warning lines; not safe to load |
| MUSA compiler | Official product filing maps S30 to SuDi; MUSA architecture docs map SuDi to `MP_10`; staged 5.2.0 compiler emits `mp_10` | Architecture target strongly supported; 5.2.0 runtime and S30 execution unverified |
| Runtime | No current runtime query because the failed module is resident | Not validated |
| Physical GPU | No S30 initialization, allocation, transfer, or kernel execution | Not tested |
| SDK support | Current official 5.2.0 catalog lists S5000 packages, not S30; S30 docs do not list Fedora | Fedora 45 + S30 not qualified |

Official source, packages, package hashes, and compiler extracts are under
`musa-data/research/official-musa-5.2.0/`. The official source PCI table
recognizes S30 but does not prove runtime compatibility. The product filing
identifies S30 as SuDi, the MUSA architecture guide maps SuDi to `MP_10`, and
the compiler manual lists `mp_10` as an offload target. This supports `mp_10`
as the S30 code-generation target, while the current 5.2.0 S30 runtime and
driver compatibility remain unverified. Evidence is recorded in
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
6. Ran read-only host inventory and the gated GPU script. Both report the
   S30 unbound and the failed module; neither opened the GPU node.
7. Backed up and reversibly quarantined the then-present unowned module file
   on 2026-11-12. A different unowned module is present in the running tree
   as of 2026-11-13; its origin is unknown.
8. Audited the archived official 2.1.1 desktop package and performed an
   isolated 7.3.3 build probe; the legacy source fails before link and remains
   research-only.

## Blocking work

### 1. Kernel driver safety

The latest isolated 3.1.0 build for 7.3.3 reports 5,504 compiler/objtool
warning lines. The boot-loaded module has already faulted under kernel IBT.
Official S30 desktop-driver release notes document DKMS kernels only through 5.15;
the running 7.3.3 kernel is outside that range. Obtain an IBT-safe vendor core
object or complete source and a supported kernel/OS matrix, then review and
resolve the warnings without disabling IBT/CET or rewriting vendor metadata.
Do not load the current artifact.

### 2. S30 SDK and runtime support

The official SDK catalog does not list S30 as a 5.2.0 package target. The
official product and architecture documents support `mp_10` as the S30 code-
generation target, but do not establish that the 5.2.0 runtime, driver,
firmware, or libraries work with S30. Obtain the supported S30 runtime/driver
version matrix and Fedora/Intel qualification from Moore Threads.

### 3. Runtime and device operation

After the driver passes static and load-safety review, validate in a
disposable environment or controlled maintenance window: driver discovery,
firmware initialization, device-node permissions, VRAM allocation, DMA,
stream synchronization, and a vendor-compiled GPU kernel. A VM without S30
PCI passthrough cannot validate physical GPU behavior.

### 4. Packaging and deployment

Only after the stack passes hardware tests, build Fedora-native runtime
packaging plus install, removal, signing, DKMS/kernel-update, and recovery
procedures. The current quarantine/restore tool is a narrow safety action; it
is not an MUSA installer.

## Safe commands

```sh
# Read-only inventory; exits 3 while driver state is unqualified
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

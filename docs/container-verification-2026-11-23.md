# Isolated container verification of the current package set (2026-11-23)

Independent reproduction of the "disposable environment" gate on the current
release set, on the installed Fedora 45 host, against the physical MTT S30.

**No host kernel module, firmware, package database, modprobe file, initramfs or
boot configuration was changed.** The host `mtgpu` module was already live from
the earlier approved controlled load (07:39 UTC) and stayed untouched: the
container test passes only `/dev/mtgpu.0` and the S30 render node into a
throwaway Fedora 45 container and suppresses every RPM scriptlet, so nothing is
built, loaded or replaced on the host.

## Why this gate exists

Package build success in Copr only proves the specs compile. It does not prove
that the produced RPMs actually contain a working stack: a spec can pass every
BRP check while shipping a runtime that cannot talk to the driver ABI (the
documented `libmusart.so.5` versus `libmusart.so.1.5` failure) or a compiler
that cannot link. This gate installs the real RPMs into a disposable root and
executes the packaged runtime and compiler there, which is the strongest check
available before touching the installed system.

## Provenance of the tested artefacts

The tested set is the output of the checked-in specs at the current releases,
built locally into `packaging/rpm/local-build/RPMS`. Its NEVRAs are identical to
what Copr publishes, so the same stack is exercised as the one users install:

```
mtgpu-dkms-3.1.0-12.fc45.noarch.rpm
mtgpu-firmware-3.1.0-4.fc45.x86_64.rpm
musa-runtime-1.5.4-4.fc45.x86_64.rpm
musa-runtime-display-1.5.4-4.fc45.x86_64.rpm
musa-sdk-5.2.0-6.fc45.x86_64.rpm
musa-toolkit-3.1.0-3.fc45.x86_64.rpm
```

The display subpackage was included deliberately: the container has no KMS
session, so the vendor Mesa DRI files cannot collide with anything there.

## Command

```sh
./scripts/test-container-install.sh --keep-image
```

## Dependency resolution (part of the result)

Installing with `--setopt=tsflags=noscripts` still resolved every declared
dependency from the Fedora 45 repositories, including `kernel-devel
7.3.4-300.fc45`, `gcc-c++ 16.2.1-3.fc45`, `binutils` and `dkms 3.4.4-1.fc45`.
This confirms the packages' `Requires:` are correct and satisfiable on the
target release; the DKMS scriptlet then had nothing to do because scriptlets
were suppressed.

## Results

`TEST_CONTAINER_INSTALL_STATUS: SUCCESS`, `CONTAINER_SUMMARY: 13 passed, 0 failed`.
No failure was tolerated or skipped (`--skip-memcpy` was not used).

| Step | Result |
|---|---|
| RPM set installs, dependencies resolve | PASS |
| Package payload present (`libmusart.so.1.5.4`, `musaInfo`, `mthreads-gmi`, `mcc`, both `musa_runtime.h` copies) | PASS (9/9 files) |
| Packaged `clang++` resolves `<cmath>` (host side of `mcc`) | PASS |
| Packaged runtime `musaInfo` | PASS — `MTT S30`, 4096 MiB, 1300 MHz, warp 128, free 3.69 GB (93 %) |
| Vendor `muMemcpyTest` | PASS — `Testing 1073741824 bytes`, `device 0 SUCCESS` |
| Host runtime API `musart-smoke` (alloc / copy / memset / streams / events) | PASS — `d2h_memset_verify=PASS`, `async_roundtrip=PASS` |
| Packaged `mcc` builds an explicit `mp_10` kernel | PASS |
| That kernel runs on the S30 | PASS — `PASS: 1048576 float adds verified on S30` |

`SMOKE_TEST: PASS (no kernel launch)` for the host-API binary and the vector-add
kernel execution together show packaged **memory management** and packaged
**kernel execution** both work from inside the container, using only the
packaged compiler, packaged headers and packaged runtime.

## Post-run host health

- Kernel journal since the run: no `Oops:`, `BUG:`, general-protection fault,
  NULL dereference, or new `Invalid cmd_type`. The only `mtgpu` lines are the
  cosmetic IRQ-affinity messages.
- GNOME Shell survived (2 processes), `gdm` active, GPU back to 0 % and 46 °C
  with no processes after the run.
- Host memory stayed within the 7 GB budget; no OOM or swap thrash.

## Scope limits

- One container, one Fedora major (45), x86_64 only; no Fedora 43/44/ELN/Rawhide
  container run was made.
- The container reuses the **host's live module**; it does not test the DKMS
  build/load path, which is a separate gate.
- No display, Vulkan, OpenCL, video or framework coverage here; those are
  covered (or excluded) by `scripts/verify-musa.sh`.
- The gate is a packaging check, not a hardware qualification: it reuses the
  driver state of the running boot.

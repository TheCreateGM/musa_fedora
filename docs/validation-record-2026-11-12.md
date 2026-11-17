# Validation record — 2026-11-12

> Historical snapshot (2026-11-12). Superseded by the
> [2026-11-17 current state](current-state-2026-11-17.md) and the project
> [README](../README.md); retained as a dated record.

## Later live recheck — kernel 7.3.3

The later recheck is authoritative for the current host; the 7.2.15 inventory
below is an earlier pass from the same date. Full evidence is in
[live-host-recheck-2026-11-12.md](live-host-recheck-2026-11-12.md).

- Host: Fedora 45, kernel `7.3.3-300.fc45.x86_64`; S30 `1ed5:0102` visible
  but unbound; Intel UHD 770 (`i915`) remains the active display GPU.
- The boot-loaded, unowned `mtgpu.ko` failed at `rmapi_alloc` with an IBT
  control-protection fault. The resident module is stuck at
  `initstate=coming`, taint `OE`. It was not unloaded or queried.
- Through `pkexec`, the exact module file was backed up under
  `musa-data/research/system-backups/` with its SHA-256 verified, removed
  from the running kernel's module tree, and `depmod` completed. The current
  in-memory module still requires a reboot to clear. The initramfs and boot
  entries were not modified.
- A fresh isolated 7.3.3 build linked a 29,712,192-byte module with matching
  vermagic and the S30 PCI alias. Objtool reported 7,451 warning lines,
  including 1,837 ENDBR-related warnings and four missing-`int3` warnings.
  The result was not installed or loaded.
- Staged official MUSA 5.2.0 compile-only checks passed, including explicit
  `mp_10` and runtime-header syntax. No generated executable was run.
- The read-only inventory and gated GPU test both exited 3 after quarantine.
  The test gate stopped before runtime query, application compilation, or
  GPU access. No physical GPU test ran.

The build and post-quarantine transcripts are under
`/home/axogm/Documents/project/musa-data/research/musa-fedora-live-validation-2026-11-12/`.
The unowned boot-loaded module's origin remains unverified; its hash does not
match the preserved research builds.

## Compile-only and inventory recheck after verifier cleanup

The read-only inventory helper now compiles `tests/hw-probe.c` into a temporary
directory and removes that directory on exit; it no longer leaves an ignored
binary in the production tree. After removing the stale generated probe, the
helper was rerun and confirmed not to recreate `tests/hw-probe`. It exited 3
as expected because the S30 is unbound and `mtgpu` remains in `coming`; the
GPU test stayed `NOT RUN`. The transcript is
`/home/axogm/Documents/project/musa-data/research/musa-fedora-validation-2026-11-12/verify-musa-recheck-after-verifier-fix.log`.

The staged official MUSA 5.2.0 compiler check was rerun independently and
exited 0: it emitted the default `mp_21`/`mp_22`/`mp_31`/`mp_32` bundle,
explicitly emitted `mp_10`, and passed the runtime-header syntax check. This
remains compile-only evidence; the executable was not run and the S30 target
mapping remains unconfirmed. Its transcript is
`/home/axogm/Documents/project/musa-data/research/musa-fedora-validation-2026-11-12/toolchain-recheck-after-verifier-fix.log`.

The sourced staging environment helper was aligned with these canonical
package extracts and now preserves any existing `LD_LIBRARY_PATH`. Running
the compile-only check through that environment also exited 0; the transcript
is `/home/axogm/Documents/project/musa-data/research/musa-fedora-validation-2026-11-12/musa-env-staging-toolchain-check.log`.
The helper changes only the current shell environment and does not install or
qualify the SDK for S30 runtime use.

The compile-only script now sources the same helper automatically when it
selects the staged package set. A fresh direct invocation through that shared
path also exited 0; its transcript is
`/home/axogm/Documents/project/musa-data/research/musa-fedora-validation-2026-11-12/toolchain-recheck-through-shared-env.log`.
The exact sourced command documented in `docs/SDK-SETUP.md` was then run in a
fresh shell and also exited 0; its transcript is
`/home/axogm/Documents/project/musa-data/research/musa-fedora-validation-2026-11-12/toolchain-recheck-documented-env-command.log`.

## Earlier 7.2.15 validation pass (historical)

This record separates compile-time evidence from hardware evidence. No
installation, module load, reboot, GPU ioctl, or GPU kernel execution was
performed in this validation pass.

## Host and device

- Fedora 45 x86_64, kernel `7.2.15-300.fc45.x86_64`.
- Moore Threads MTT S30 at `0000:01:00.0`, PCI ID `1ed5:0102`; currently
  unbound, with no `/dev/mtgpu.0` and no S30-associated DRM render node.
- No MTT kernel module is loaded. The active command line still has
  `ibt=off`; restoring IBT requires a reboot after the persistent boot entries
  have been corrected. See [IBT recovery status](ibt-off-recovery.md).
- Four Moore Threads firmware files are present in `/usr/lib/firmware/mthreads`.
  Their presence does not establish driver or GPU operation.

## Build and compiler checks

| Check | Result | Scope |
|---|---|---|
| `scripts/build-kernel-driver.sh` with a fresh source copy and regenerated conftest | Exit 0; module linked at 29,783,016 bytes with 7.2.15 vermagic and S30 PCI aliases | Compile/link only |
| Objtool during that fresh build | 7,460 warnings, including 1,837 ENDBR-related and four missing-`int3` warnings | Blocks load approval |
| Staged official MUSA 5.2.0 compiler | Compile/link pass for default `mp_21`, `mp_22`, `mp_31`, `mp_32` bundle and explicit `mp_10` bundle; runtime header syntax check passed | Compile-only; official architecture docs support S30 → SuDi → `MP_10`; hardware execution untested |
| Installed `/usr/local/musa-5.2/bin/musaInfo` | API error 100: “no MUSA-capable device is detected” | Runtime discovery failed |
| `scripts/verify-musa.sh` | Exit 3; device visible but unbound, no node, no qualified driver | Read-only inventory |
| `scripts/test-gpu.sh` | Exit 3; test not run because IBT is off, no full driver is bound, and no render node exists | No GPU test |

The build script copies only source inputs, removes stale generated conftest
output in the copy, and builds under
`/home/axogm/Documents/project/musa-data/research/builds/`. Its complete
output is recorded in
`/home/axogm/Documents/project/musa-data/research/musa-fedora-validation-2026-11-12/project-build-script-clean-tree-7.2.15.log`;
the detailed Kbuild log and module are beside the build copy. An earlier
manual clean build is also retained in the validation directory. The
compile-only toolchain, verifier, gated hardware test, and runtime-query logs
are kept there as well.

## Hardware test status

No S30 driver initialization, MUSA device query on a bound driver, VRAM
allocation, DMA, kernel execution, display, or audio test is verified. The
project-built module is not approved for installation or loading because the
opaque vendor core produces thousands of IBT/objtool warnings. Do not disable
IBT/CET, alter vendor core metadata, or use PCI enumeration, a device node, or
successful compilation as GPU-test evidence.

## Deployment status

No system files, boot settings, kernel modules, libraries, firmware, or package
state were changed in this pass. There is no supported install or removal
procedure yet. The official S30 source-table entries do not establish Fedora
compatibility; see [the compatibility assessment](compatibility.md) and
[vendor package research](vendor-sdk-availability.md).

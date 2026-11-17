# Implementation report

> Historical report. The host snapshots recorded below (2026-11-13 and earlier)
> are superseded by the [2026-11-17 current state](current-state-2026-11-17.md)
> and the project [README](../README.md); retained as a dated record.

## Latest live host recheck (2026-11-13)

The host runs Fedora 45 kernel `7.3.3-300.fc45.x86_64`. The S30 (`1ed5:0102`)
is visible but unbound; Intel UHD 770 (`i915`) is the active display GPU. A
boot-loaded, unowned `mtgpu.ko` failed with an IBT control-protection fault
at `rmapi_alloc`; the resident module remains `initstate=coming`, taint `OE`.
An unowned file is currently present in the running kernel's module tree, with
a different hash from the one recorded as quarantined on 2026-11-12. Its
origin is unknown. No MUSA runtime query or GPU test was run.

A fresh isolated 7.3.3 module build linked with matching vermagic and an S30
PCI alias, but logged 5,504 compiler/objtool warning lines. It is not
installed or approved to load. The staged official MUSA 5.2.0 compiler passed
compile-only checks; no executable ran. No functional MUSA environment for
the MTT S30 has been completed. See the
[live host report](live-host-recheck-2026-11-13.md).

## Earlier result (2026-11-11)

A functional MUSA environment for the MTT S30 had not been completed in this
earlier inventory. Its then-current host snapshot and module event are
historical; see the latest recheck above for present state.

## Evidence from earlier validation passes

- The read-only verifier completed and exited 3. It found the S30 visible but
  unbound, with no MTT-specific node, and reported GPU testing as NOT RUN.
- Official Mthreads 5.2.0 DKMS source, package artifacts, package-index
  evidence, and related research are staged under
  `/home/axogm/Documents/project/musa-data/research/official-musa-5.2.0/`.
  The source PCI table recognizes S30 ID `0x0102`.
- A production-tree compile/link build was completed for kernel
  `7.3.3-300.fc45.x86_64` during an earlier host snapshot. The generated
  `mtgpu.ko` has matching
  vermagic and an S30 PCI alias, but the build emits more than 7,000 objtool
  warnings, including `!ENDBR` relocations and missing-`int3` warnings from
  the opaque vendor core. The build command did not install or load it.
- An earlier fresh `mtgpu.ko` under `musa-data/research/builds` has
  `7.2.15-300.fc45.x86_64` vermagic and is not load-approved. The latest
  fresh 7.3.3 artifact is separately described in the live host report; it is
  also not approved for loading. Stale root build outputs are archived under
  `musa-data/research/archived-project-build-artifacts-2026-11-12/`.
- The first current-tree build attempt exposed an absolute/relative target
  mismatch in the Makefile core-object rule. The rule was corrected to create
  the relative Kbuild object symlink explicitly; the subsequent build
  completed. Full output is archived under
  `musa-data/research/kernel-port-fedora-7.3.3-build-2026-11-11/`.
- The staged official MUSA 5.2.0 compiler passed a fresh compile-only check for
  both its default `mp_21`–`mp_32` fat binary and an explicit `mp_10`
  binary. Moore Threads' filing identifies S30 as SuDi; official MUSA
  architecture documentation maps SuDi to MP_10. This supports MP_10 as the
  S30 code-generation target, but does not establish 5.2.0 runtime or hardware
  compatibility. Neither executable was run. Source chain:
  `musa-data/research/official-musa-5.2.0/s30-mp10-target-evidence-2026-11-12.md`.
- A separate, unowned `/usr/local/musa-5.2` tree reports version 5.2.0. Its
  `musaInfo` query returned error 100, “no MUSA-capable device is detected.”
  No GPU execution was attempted.
- The 2026-11-10 kernel 7.2.15 boot was documented with incomplete module
  initialization and kernel IBT/ENDBR faults.
- No system packages, modules, firmware, boot configuration, or loader
  configuration were changed during this inventory.

## Not completed

No driver initialization, runtime device discovery, firmware interaction,
VRAM allocation, DMA transfer, physical GPU kernel execution, display test, or
reboot-persistence test is verified. Official product and MUSA architecture
documents support `mp_10` as the S30 code-generation target, but public vendor
guidance reviewed so far does not qualify the 5.2.0 runtime or Fedora 45 +
Intel + S30.

## Historical unexpected module event (2026-11-11)

At approximately 13:49 on 2026-11-11, a later read-only check found
`/lib/modules/7.3.3-300.fc45.x86_64/extra/mtgpu.ko` loaded with
`initstate=coming`. Its SHA-256 matches the project-tree compile-only build.
The kernel journal reported `Missing ENDBR: rmapi_alloc`; the S30 remained
unbound and `/dev/mtgpu.0` absent. This session issued no module installation
or load command, so the system change's actor and mechanism are unknown. No
device test, module removal, or further driver interaction was attempted.

## Evidence and safety boundary

- Current host check: `./scripts/verify-musa.sh` (exit 3; GPU test not run).
- Historical 7.2.15 build evidence and source basis:
  [driver-porting.md](driver-porting.md).
- Official package/source research:
  [vendor-sdk-availability.md](vendor-sdk-availability.md).
- Detailed current host facts: [system-inventory.md](system-inventory.md).

No GPU device was opened and no module installation/load command was issued
in this session. The later system-level module event above is untrusted and
must not be treated as a successful test. Do not interact with it or run
device tests until an independently reviewed recovery plan is established.

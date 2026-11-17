# Live host recheck — 2026-11-13

This report records a read-only host recheck and isolated compile/build
validation. It supersedes the present-tense module-file and quarantine status
in the [2026-11-12 report](live-host-recheck-2026-11-12.md); earlier actions
and test results in that report remain historical.

## Host and device state

| Item | Observation |
|---|---|
| OS/kernel | Fedora Linux 45 Workstation, x86_64; `7.3.3-300.fc45.x86_64` |
| Kernel IBT | Enabled; no `ibt=off` or `noibt` command-line override |
| MTT S30 | `0000:01:00.0`, PCI `1ed5:0102`, visible but unbound |
| Active display | Intel UHD Graphics 770 (`i915`) |
| MTT device nodes | `/dev/mtgpu.0` absent; no S30-associated DRM render node |
| MUSA driver state | `mtgpu` loaded in `initstate=coming`, taint `OE`; not usable |

The current unowned file
`/lib/modules/7.3.3-300.fc45.x86_64/extra/mtgpu.ko` is 29,974,352 bytes,
modified 2026-11-12 16:59 local time, and has SHA-256
`958232c0785d2e9588cdd67ff4b17e10f9c57d31c5f3d116b3b95dd28b4cc2e6`.
It is not owned by an RPM. This differs from the module hash recorded as
quarantined on 2026-11-12; the current file's origin is unknown. The boot
journal records `Missing ENDBR: rmapi_alloc` and a kernel BUG in
`arch/x86/kernel/cet.c`; the S30 remains unbound. Do not open the GPU device,
query the MUSA runtime, load another module, or try to unload the failed
module.

No system file, module, boot setting, or device state was changed by this
recheck. No `pkexec`, reboot, module load/unload, or GPU-device operation was
performed.

## Isolated driver build

`scripts/build-kernel-driver.sh "$(uname -r)"` copies the active S30 driver
source (`PACKAGE_VERSION` 3.1.0), project `Makefile`, and feature-probe helper
into a fresh build directory under `musa-data/research/builds/`. It uses the
vendor core object from the existing 3.1.0 research build, pinned to SHA-256
`d0fdaceab37bce7387d376f8a87a2e9dbf210eeccf4f4f7c67f8dee83f4e8200`.
The build completed for kernel `7.3.3-300.fc45.x86_64`; the resulting module
has matching vermagic and the S30 PCI alias. It was not installed or loaded.

- Artifact: `/home/axogm/Documents/project/musa-data/research/builds/mtgpu-7.3.3-300.fc45.x86_64-fu40fr/source/mtgpu.ko`
- SHA-256: `f9d70da28c9b17dc19c21bdcf65865353b2c8b2a09791e4a0384d5f56157fe71`
- Build log: `/home/axogm/Documents/project/musa-data/research/builds/mtgpu-7.3.3-300.fc45.x86_64-fu40fr/build.log`
- Compiler/objtool warning lines: 5,504

The warnings and the earlier `rmapi_alloc` IBT fault mean this artifact is
not approved for loading. A successful link and PCI alias are not evidence
of driver initialization or GPU execution.

## MUSA compiler and test gates

The staged official MUSA 5.2.0 compiler passed the compile-only vector-add
check, explicit `mp_10` code generation, and runtime-header syntax check.
Neither executable was run. The S30-to-SuDi-to-`MP_10` evidence supports the
compiler target only; S30 compatibility of this runtime/driver combination
remains unverified.

The read-only inventory and gated GPU test both exited 3. The inventory
reported the S30 unbound and `mtgpu` incomplete; the test gate stopped before
runtime discovery or GPU access. No physical GPU test ran.

The official package/source support boundary and required vendor
confirmations remain as recorded in
[vendor availability](vendor-sdk-availability.md). In particular, current
public documentation does not qualify Fedora 45 + Intel + S30 or establish
that MUSA 5.2.0 runtime supports S30.

# Feature boundary and limitations

The current state is recorded in the
[live host recheck](live-host-recheck-2026-11-13.md).

## No S30 compute stack is usable on this host

The S30 is visible at PCI ID `1ed5:0102` but remains unbound on Fedora 45
kernel `7.3.3-300.fc45.x86_64`. The Intel UHD 770 (`i915`) is the active
display GPU. `/dev/mtgpu.0` and an S30-associated DRM render node are absent.
Moore Threads lists OpenCL parallel compute as an S30 hardware capability;
that specification does not establish a working OpenCL or MUSA stack on this
Fedora installation. The vendor feature claims and project validation state
are separated in [supported features](supported-features.md).

An unowned `mtgpu.ko` loaded during boot, then hit a kernel IBT control-
protection fault at `rmapi_alloc`. That module remains resident in
`initstate=coming`, with taint `OE`. An unowned on-disk `mtgpu.ko` is
currently present and its origin is unknown. Do not issue GPU calls, query
the runtime, or try to unload it.

The project-authored `mtgpu_bind.ko` in the inactive
`7.2.15-300.fc45.x86_64` tree only bound the PCI function and created a
character device. Its default ioctl returned `ENOTTY`; it was not a MUSA GPU
driver. Its exact module file and global alias have been hash-verified and
quarantined under `musa-data` so that the alias cannot load it when that
kernel is selected. The files remain recoverable but are not approved for use.

## Driver build boundary

A fresh isolated build links for kernel 7.3.3 and includes an S30 PCI alias.
The latest build logged 5,504 compiler/objtool warning lines. The artifact is
not installed or load-approved. Do not disable IBT/CET or alter vendor-core
metadata to bypass these warnings.

## Capability matrix

| Capability | Status |
|---|---|
| S30 PCI identity `1ed5:0102` | Verified |
| Official source contains S30 device ID | Verified by source inspection |
| S30 PCI driver binding | Unbound |
| Incomplete project PCI binder | Reversibly quarantined from 7.2.15-300; its global alias is removed |
| Fresh kernel 7.3.3 driver build | Linked; not load-approved |
| Vendor-qualified Fedora 45 + S30 stack | Not established |
| MUSA compiler/header check | Passed; compile-only |
| S30 compiler code-generation target | Official evidence chain maps S30 → SuDi → `MP_10`; 5.2.0 runtime/device compatibility remains unverified |
| Driver initialization | Failed in boot-loaded module; current module remains stuck in `coming` |
| Runtime device discovery | Not tested in the latest pass |
| S30 VRAM, DMA, GPU execution | Not tested |
| `torch_musa` on S30 | Not tested or established |

## Required before hardware testing

1. Obtain a vendor IBT-safe core object or complete source and review its
   kernel interfaces and memory-management behavior.
2. Resolve the objtool warnings without disabling IBT/CET or rewriting vendor
   metadata.
3. Obtain version-specific confirmation that MUSA 5.2.0's `mp_10` toolchain,
   runtime, and driver support the S30, or identify a supported matching SDK.
4. Verify that runtime and driver combination on a supported OS/kernel before
   adapting it to Fedora.
5. Prepare a separately reviewed recovery plan, then test device discovery,
   memory transfer, and a vendor-compiled GPU kernel on the physical card.

Until those gates pass, do not install or load the driver or claim a
functional MUSA environment.

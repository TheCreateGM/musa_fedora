# Verified project features

This table lists only evidence-backed facts. It does not claim a functional
MUSA environment or SDK conformance. The current host baseline is the
[2026-11-13 live recheck](live-host-recheck-2026-11-13.md).

| Area | Status | Evidence |
|---|---|---|
| S30 PCI identity | Verified, `1ed5:0102` | Live sysfs inventory |
| Official driver source recognizes S30 ID | Verified by source inspection | Staged MUSA 5.2.0 DKMS source |
| Read-only hardware inventory | Passed as an inventory; exits 3 for unqualified driver state | `scripts/verify-musa.sh`; no device node opened |
| Kernel driver build | Compile/link passed for kernel 7.3.3; not safe to load | Fresh isolated 3.1.0 artifact has matching vermagic and S30 alias, with 5,504 compiler/objtool warning lines |
| Boot-loaded module | Failed initialization; not bound | Kernel log records `Missing ENDBR: rmapi_alloc`; `initstate=coming`, taint `OE` |
| On-disk failed module | Unowned, present; origin unknown | Current hash differs from the module quarantined on 2026-11-12 |
| Legacy project PCI binder | Reversibly quarantined | Incomplete `mtgpu_bind.ko` and its global alias backed up; no module was loaded |
| MUSA 5.2.0 compiler and headers | Compile-only verified | Default fat binary, explicit `mp_10`, and runtime-header syntax check passed |
| S30 code-generation target | Official architecture evidence supports `mp_10`; hardware execution unverified | Company filing maps S30 to SuDi; MUSA 4.3 architecture guide maps SuDi to `MP_10`; MCC manual lists `mp_10`; staged MUSA 5.2.0 compiler emits it |
| Runtime device discovery | Not tested in the latest pass | Runtime query was withheld while the loaded module remained stuck |
| S30 VRAM, DMA, GPU execution | Not tested | No functioning, reviewed driver/runtime |

## Vendor-listed S30 hardware capabilities

These are Moore Threads product specifications, not measurements or
validation on this Fedora host. The vendor identifies the S30 as first-
generation MUSA hardware and lists OpenCL parallel computing, OpenGL/OpenGL
ES/Vulkan graphics, and hardware video encode/decode. The product page lists
H.264/H.265/AV1 encoding and H.264/H.265/AV1/VP8/VP9/AVS/AVS2/MPEG4/MPEG2
decoding. The 2 GB and 4 GB S30 variants differ in display limits: 4K@30Hz
and 4K@60Hz respectively. See the [S30 product specification](https://docs.mthreads.com/s30/s30-doc-online/introduction/).

The vendor's S30 product page does not name an MCC target directly; the
architecture-level mapping comes from the company filing and MUSA 4.3
architecture/compiler manuals. This does not prove that the current MUSA 5.2.0
runtime or driver supports S30. The SDK catalog and driver support boundary
remain documented in [vendor availability](vendor-sdk-availability.md).

The official SDK catalog does not list S30 as a MUSA 5.2.0 Linux package
target. A successful compiler run or source PCI table is not evidence of
hardware support. The earlier CPU emulator and CPU-only tests are archived
under `/home/axogm/Documents/project/musa-data/research/`; they do not
execute MUSA device code.

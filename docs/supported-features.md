# Verified project features

This table lists only evidence-backed facts. The current host baseline is the
[current state](current-state-2026-11-17.md).

| Area | Status | Evidence |
|---|---|---|
| S30 PCI identity | Verified, `1ed5:0102` | Live sysfs inventory |
| S30 PCI driver binding | Verified; `mtgpu` 3.1.0 loaded, `initstate=live` | `/dev/mtgpu.0` and `/dev/dri/renderD129` present |
| IOMMU | Verified | `intel_iommu=on iommu=pt`; S30 in IOMMU group 11 |
| Device telemetry | Verified | `mthreads-gmi` reports driver 3.1.0, MTT S30, 4096 MiB |
| MUSA 1.5.4 runtime discovery | Verified | `musaInfo` detects one device |
| Vendor copy test | Verified | `muMemcpyTest` passes |
| Host runtime API | Verified | `tests/musart-smoke.c` passes on the S30 |
| S30 code-generation target | Verified on hardware | MUSA 5.2.0 `mcc` `mp_10` vector-add ran; 1,048,576 adds verified |
| End-to-end check | Verified | `scripts/verify-musa.sh`: 11 passed, 0 failed |
| OpenCL, graphics, video, `torch_musa` | Not tested or claimed | Not exercised on this host |
| Vendor-qualified Fedora 45 + S30 stack | Not established | Public catalog lists S5000, not S30 or Fedora |

The staged official MUSA 5.2.0 DKMS source also recognizes the S30 PCI ID by
source inspection. That is source-level recognition only.

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
target. A successful compiler run or source PCI table is not by itself
evidence of hardware support. The earlier CPU emulator and CPU-only tests are
archived under `/home/axogm/Documents/project/musa-data/research/`; they do not
execute MUSA device code.

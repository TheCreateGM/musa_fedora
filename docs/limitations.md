# Feature boundary and limitations

This page states what has actually been exercised on the MTT S30 under this
project and what has not. The latest live evidence is
[hardware validation 2026-11-26](hardware-validation-2026-11-26.md) — an
independent cold boot that reproduced 13/0/2, PERF PASS, VA-API 14/0/0, env
PASS, and COPR 20/20 with zero mtgpu kernel faults — following
[hardware validation 2026-11-25](hardware-validation-2026-11-25.md); measured
figures are in [performance tuning](performance-tuning.md).

## Current live state

Fedora Linux 45, kernel `7.3.4-300.fc45.x86_64`, Intel UHD 770 driving the
desktop (`i915`). The S30 is bound to the out-of-tree `mtgpu` 3.1.0 driver on a
normal boot through udev, with compute mode enabled. Installed stack:

| Package | Version |
|---|---|
| `mtgpu-dkms` | 3.1.0-15 on the host; 3.1.0-16 published (objtool fallback, [record](objtool-fallback-2026-11-25.md)) |
| `mtgpu-firmware` | 3.1.0-5 |
| `musa-runtime` | 1.5.4-8 |
| `musa-runtime-va` | 1.5.4-8 |
| `musa-sdk` | 5.2.0-7 |
| `musa-toolkit` | 3.1.0-5 on the host; 3.1.0-6 published (README payload refresh) |

`scripts/verify-musa.sh --skip-memcpy` reports **15 passed, 0 failed, 2
skipped**. The skips are the vendor `muMemcpyTest` (it needs more host RAM than
the capped test scope allows) and Moore Threads OpenCL (no MT ICD installed).
Without `--skip-memcpy` the suite reports 16 passed, 0 failed, 1 skipped when
enough host memory is free. The count rose from 13 to 15 on 2026-11-26 when the
SONAME-hygiene and VA-driver-path guards were added.

## Verified

- PCI identity `1ed5:0102`, driver binding, `/dev/mtgpu.0`, render node
  (`renderD129` on this boot), IOMMU enabled.
- MUSA runtime 1.5.4 device discovery (`musaInfo`), host runtime API
  (`musart-smoke`), memory allocation and copy.
- MUSA 5.2.0 `mcc` targeting `mp_10`: vector-add, SAXPY, shared reduction,
  atomics, constant memory, device math, 2D launch, D2D copy, FP64; all results
  host-checked.
- Headless Vulkan device, queue, buffer and host-visible map/readback.
- H.264 **encode** through VA-API on the S30 VPU, producing a decodable
  bitstream (14/0/0 in `scripts/hardware/test-vaapi.sh`).
- Host-backed allocation after the 3.1.0-15 heap-page fix: no `warn_alloc`,
  p50 allocation latency 0.064 ms at 32 MiB.

## Not verified or not supported

- **Hardware video decode** is advertised by the driver (`VAEntrypointVLD`
  profiles for H.264, HEVC, VP9, AV1, VC-1, MPEG-2, JPEG, VP8) but **does
  not work on this host**. It was exercised end to end on 2026-11-26 with
  `scripts/hardware/test-video-decode.sh`: the packaged driver build cannot
  allocate its VPU working buffers (`mos_bo_alloc DEC_MV` →
  `M3d::CreateGpuMemory -9`), the older build rejects the picture
  (`VpuApiDecPic: slices number was wrong!`), and the MUSA-native MUVID
  route in `musa-runtime` 1.5.4 rejects the H.264 codec id outright
  (`MUSA_ERROR_NOT_SUPPORTED`). The S30 encodes video in hardware
  (`h264_vaapi`) but does not decode it here.

  The hardware decoder itself **is** present: the upstream Video Codec SDK
  library reports `h264_420 supported=1 mtdECs=1` for this GPU. What blocks
  it is the userspace chain above the driver — that library needs the MUSA
  5.x driver API, and the MUSA 5.x API issues `MTGPU_QUERY_CMD_MKT_NAME`
  (sub-command 12), which the only S30 kernel driver available (mtgpu
  3.1.0) does not implement. **One ingredient would unblock MUSA-native
  hardware decode: a Moore Threads S30 kernel driver that implements
  `MTGPU_QUERY_CMD_MKT_NAME`.** Full analysis with evidence:
  [root-cause-video-decode-2026-11-26](root-cause-video-decode-2026-11-26.md).
- **Moore Threads OpenCL** is unavailable: no MT ICD is installed, and the
  archived 2.1.1 ICD caused a kernel Oops in `PVRSRVRGXFlushComputeDataKM`.
  Do not install or retry it.
- **Display output / rendering / OpenGL** through the S30 is not used or
  claimed. The desktop stays on `i915`. `musa-runtime-display` exists as an
  explicit opt-in but the vendor Mesa driver crashes GNOME Shell on this host;
  do not install it next to a desktop session.
- **`torch_musa` and other frameworks** are not tested or claimed.
- **Hardware decode, transcoding, multi-stream VA** and VP9/HEVC/AV1 encode
  were not exercised.
- **Vendor qualification**: Fedora 45 + Intel + S30 is not a vendor-qualified
  combination. The public MUSA 5.2.0 catalog lists S5000, not S30, and not
  Fedora.

The historically exercised combination is driver `mtgpu` 3.1.0 + MUSA runtime
1.5.4 + MUSA 5.2.0 compiler targeting `mp_10`. The separate 5.2.0 *runtime* is
not ABI-compatible with the 3.1.0 driver and must not be substituted for 1.5.4.

## Driver build boundary

`mtgpu-dkms` 3.1.0-16 (and -15) build on Fedora 43, 44, 45, ELN and Rawhide
x86_64 and carry the S30 PCI alias. Since 3.1.0-16, a kernel-devel tree
whose prebuilt objtool cannot start does not silently lose ORC metadata: the
build compiles a working objtool from the kernel sources instead
([objtool-fallback-2026-11-25](objtool-fallback-2026-11-25.md)). The vendor
core is an opaque prebuilt object; every
build emits objtool warnings from it, including missing-ENDBR relocations and
RETPOLINE naked returns. Do not disable IBT/CET or rewrite vendor-core metadata
to silence them.

A DKMS build regenerates the driver's `conftest` probes against the running
kernel. Do not run those probes under a tight process limit: `conftest` forks
many short-lived compile checks, and a low `TasksMax`/pid cap makes them fail
with `EAGAIN`, which is then misread as "kernel API absent" and produces a
broken build. Leave several hundred processes of headroom.

The project-authored `mtgpu_bind.ko` in the inactive 7.2.15 tree only bound the
PCI function and created a character device; its default ioctl returned
`ENOTTY`. It was not a MUSA GPU driver and is quarantined under `musa-data`.

## Capability matrix

| Capability | Status |
|---|---|
| S30 PCI identity `1ed5:0102` | Verified |
| S30 binding to `mtgpu` 3.1.0 on normal boot | Verified since compute mode enabled 2026-11-24 |
| `/dev/mtgpu.0` and render node | Verified |
| IOMMU groups | Verified; IOMMU enabled |
| MUSA runtime 1.5.4 device discovery | Verified; one device |
| Vendor `muMemcpyTest` | Verified historically; skipped in capped runs (host-RAM footprint) |
| Host runtime API (`musart-smoke`) | Verified |
| `mcc` `mp_10` vector-add and compute suite | Verified on the S30 |
| Headless Vulkan device/queue/buffer/map | Verified; display rendering not tested |
| VA-API initialisation and H.264 encode | Verified; hardware decode exercised and failed (2026-11-26) |
| MUSA-native MUVID hardware decode | Unavailable; the shipped runtime rejects the H.264 codec id, and the upstream library that supports it needs a newer S30 kernel driver |
| Host-backed allocation after 3.1.0-15 | Verified; zero `warn_alloc` |
| Moore Threads OpenCL | Unavailable; archived ICD caused a kernel Oops |
| OpenGL, display output, `torch_musa`, other frameworks | Not tested or claimed |
| Vendor-qualified Fedora 45 + S30 stack | Not established |

## Remaining work

Broader MUSA coverage (OpenGL, `torch_musa`, other frameworks), working
hardware video decode (blocked on the vendor driver/library defects analysed
in [root-cause-video-decode-2026-11-26](root-cause-video-decode-2026-11-26.md)),
Moore Threads OpenCL (needs a driver-compatible ICD, not the archived one), and
vendor qualification. Basic device bring-up, compute, compiler, VA encode and
the host-memory fix are done. S30 performance evidence and tuning are in
[performance-tuning.md](performance-tuning.md).

## Safety

Do not unbind/rebind or replace the active module as a routine test: a
historical rebind caused a kernel fault (see
[root-cause-rebind-2026-11-16.md](root-cause-rebind-2026-11-16.md)). Do not
install a DKMS/RPM stack over the live module without a separate recovery plan.
Do not disable IBT/CET.

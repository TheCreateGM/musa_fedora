# VA-API on the MTT S30 (musa-runtime-va)

The S30's video processing unit is driven by the vendor VA-API driver
`mtgpu_drv_video*.so`. It is packaged separately from the compute runtime and
from the Mesa display drivers:

| Package | Payload | Who loads it |
|---|---|---|
| `musa-runtime-va` | `/usr/lib64/dri/mtgpu_drv_video*.so`, `musa_drv_video.so` | libva only |
| `musa-runtime-display` | Mesa DRI/GBM/Xorg drivers | Mesa/GNOME (opt-in; crashes mutter on this host) |

Keeping the VA driver out of `musa-runtime-display` means hardware video works
on a compute-only host without installing the Mesa payload that crashes GNOME
Shell. See [the display-driver split](display-driver-subpackage-2026-11-23.md).

## Verified on the physical S30 (2026-11-25)

Host: Fedora 45, kernel `7.3.4-300.fc45.x86_64`, `mtgpu` 3.1.0-15,
`musa-runtime-va` 1.5.4-7, libva 2.24.1, render node `/dev/dri/renderD129`.

```sh
scripts/hardware/test-vaapi.sh
```

Result: **14 passed, 0 failed, 0 skipped** (`VAAPI_STATUS: PASS`).

```
PROBE_OK vaInitialize 1.24 on /dev/dri/renderD129
VENDOR: Mthreads VPU driver for MT(R) MTT S30 Graphics - 0.0.1.378a2099 (378a2099)
PROFILES: 20
```

The enumerated profiles are, exactly as `vaprobe` reported them:

| Profile | Entrypoints |
|---|---|
| MPEG-2 Simple/Main | VLD |
| H.264 Baseline/Main/High | VLD |
| VC-1 Simple/Main | VLD + EncSlice + EncSliceLP |
| VC-1 Advanced | VLD |
| JPEG Baseline | VLD |
| VP8 0.3 | VLD |
| HEVC Main12 | VLD + EncSlice + EncSliceLP |
| VP9 Profile0 | VLD |
| VP9 Profile3 | VLD + EncSlice + EncSliceLP |
| HEVC Main422_10 | VLD + EncSlice + EncSliceLP |
| HEVC Main444 / Main444_12 | VLD |
| AV1 Profile0/Profile1 | VLD + EncSlice + EncSliceLP |
| AV1 Profile0/Profile1 (second build) | VLD + EncSlice + EncSliceLP |
| one unnamed profile (id 36) | VLD + EncSlice + EncSliceLP |

An actual encode round trip was then executed: `ffmpeg` used `h264_vaapi` on
the S30 to encode a 60-frame 640×480 synthetic source at 2 Mbit/s. The result
was a 425 503-byte bitstream that decoded cleanly with the software decoder, so
the encode path produced a valid stream rather than merely initialising.

**This produced an instructive discrepancy:** the driver does not advertise an
H.264 encode entrypoint in `vaQueryConfigEntrypoints`, yet ffmpeg's
`h264_vaapi` successfully created the config and encoded the stream. The
advertised entrypoint list is therefore not a complete capability list on this
driver build; a capability must be exercised, not inferred from enumeration.
The reverse also holds: the VLD profiles are advertised, but hardware decode was
not exercised here.

The kernel log for the run contained only the expected `[mtvpu] Core 2 FW
Version` line and no `BUG`, `Oops`, general-protection or allocation failure.

## What this does and does not establish

Verified:

- VA-API initialisation against the S30 render node;
- the advertised profile/entrypoint set;
- H.264 hardware encode producing a decodable bitstream;
- no kernel fault during the run;
- the driver is picked up by libva from the packaged path.

Not verified:

- hardware **decode** does not work on this host. It was exercised end to
  end on 2026-11-26 with `scripts/hardware/test-video-decode.sh` and fails
  inside the vendor driver, not in the test. The newest driver build
  cannot allocate its VPU working buffers (`mos_bo_alloc DEC_MV` →
  `M3d::CreateGpuMemory -9`); the older build rejects the picture
  (`VpuApiDecPic: slices number was wrong!`). The full analysis, including
  the libva 2.24 API breakage any client hits, is in
  [root-cause-video-decode-2026-11-26](root-cause-video-decode-2026-11-26.md).
- VP9/HEVC/AV1 encode, transcoding, and multi-stream sessions were not
  exercised.
- Display/GL paths are out of scope; this subpackage contains no Mesa driver.

## Hardware decode status (2026-11-26)

```
$ scripts/hardware/test-video-decode.sh
```

Two independent decode routes are driven by that script; neither completes
a picture on this host:

```
== VA-API route (libva VAEntrypointVLD) ==
VADECODE_FAIL render -> 2 (resource allocation failed)
Failed M3d::CreateGpuMemory -9
[LIBVA]:CRITICAL - mos_gem_bo_alloc_internal:198: fail to create m3d bo
[LIBVA]:CRITICAL - VpuApiDecSeqInit:1549: Error mos_bo_alloc DEC_MV
[LIBVA]:CRITICAL - DdiDecode_EndPicture:2008: Seq init failed.
--- older-build ---
VADECODE_FAIL render -> 18 (invalid parameter)
[LIBVA]:CRITICAL - VpuApiDecPic:1708: slices number was wrong! please check.
[LIBVA]:CRITICAL - DdiDecode_EndPicture:1916: Dec frame failed.

== MUSA MUVID route (libvideo_musa.so) ==
MUVID_FAIL muvidCreateVideoParser -> 801
```

The MUSA-native route fails earlier still, for a different reason: the
runtime in `musa-runtime` 1.5.4 accepts only codec ids 0..3 and answers
`MUSA_ERROR_NOT_SUPPORTED` (801) for the id 4 that the vendor Video Codec
SDK headers use for H.264.

The MUSA-native route fails earlier still, for a different reason: the
runtime in `musa-runtime` 1.5.4 accepts only codec ids 0..3 and answers
`MUSA_ERROR_NOT_SUPPORTED` (801) for the id 4 that the vendor Video Codec
SDK headers use for H.264.

The upstream Video Codec SDK library (`libvideo_musa.so.13.0`) *does*
implement H.264 — dispatching on codec ids 0..3, 4, 8, 10 and 11 — and
reports this GPU as `h264_420 supported=1 mtdECs=1`, i.e. **the hardware
decode engine is present and correctly advertised**. It cannot be used
here because it belongs to the MUSA 5.x generation, and the MUSA 5.x driver
API is rejected by the installed mtgpu 3.1.0 kernel driver
(`MTGPU_QUERY_CMD_MKT_NAME`, sub-command 12 → `muInit` fails with -22).

Practical consequence: **on Fedora the S30 can encode video in hardware but
cannot decode it in hardware.** `h264_vaapi` encode is the working half,
and the missing piece is a newer S30 kernel driver rather than missing
hardware.

## Requirements

- the `mtgpu` kernel driver loaded and an S30 render node present;
- `libva` (`libva.so.2`, `libva-drm.so.2`);
- `ffmpeg` built with `h264_vaapi` for the encode check. Fedora's
  `ffmpeg-free` may lack it; the test SKIPs encode in that case rather than
  failing.

## Troubleshooting

- `vaInitialize` fails: confirm `musa-runtime-va` is installed and
  `/usr/lib64/dri/mtgpu_drv_video.so` exists.
- wrong device: set `VA_RENDER_NODE` (the script auto-detects the `mtgpu`
  render node).
- `LIBVA_DRIVER_NAME`: the test pins `mtgpu` for determinism, but libva
  normally probes the driver automatically.

## See also

- [Display-driver subpackage](display-driver-subpackage-2026-11-23.md)
- [Hardware validation 2026-11-25](hardware-validation-2026-11-25.md)
- [Supported features](supported-features.md)

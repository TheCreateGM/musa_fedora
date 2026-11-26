# Root cause: hardware video **decode** on the MTT S30 (2026-11-26)

**Date:** 2026-11-26
**Host:** Fedora 45 Workstation, kernel `7.3.4-300.fc45.x86_64`
**GPU:** Moore Threads MTT S30 `0000:01:00.0` (`1ed5:0102`), bound to the
live `mtgpu` 3.1.0 module
**Packages:** `mtgpu-dkms` 3.1.0-15, `mtgpu-firmware` 3.1.0-5,
`musa-runtime` 1.5.4-7, `musa-runtime-va` 1.5.4-7, `musa-sdk` 5.2.0-7,
libva 2.24.1

Scope: read-only analysis, two new user-space probes, and one reversible
host change (quarantining an unowned library). **No module load/unload, no
PCI unbind/rebind, no boot or blacklist change.** No kernel fault was
produced at any point.

## Summary

Hardware video **decode** does not work on this host, on either of the two
routes the vendor stack offers, and the causes are now known:

| Route | Entry point | Result |
|---|---|---|
| VA-API | `mtgpu_drv_video.so`, `VAEntrypointVLD` | **vendor driver defect** — see §2 |
| MUSA MUVID | `libvideo_musa.so` (`musa-runtime` 1.5.4) | **runtime implements only codec ids 0..3**; H.264 rejected with 801 — see §3 |
| MUSA MUVID (upstream) | `libvideo_musa.so.13.0` (Video Codec SDK) | implements H.264 (`mtdECs=1`), but its MUSA 5.x pairing is blocked by the 3.1.0 kernel driver — see §3.3–3.5 |

Encode, by contrast, is unaffected: `h264_vaapi` produces a valid
H.264 stream on the S30 (`docs/vaapi.md`).

The single blocking prerequisite for the MUSA-native route is now
identified: a Moore Threads S30 **kernel driver that implements
`MTGPU_QUERY_CMD_MKT_NAME`** (sub-command 12). That would unblock the
MUSA 5.x userspace, which in turn unblocks the Video Codec SDK 13.0 MUVID
library — and that library *does* implement H.264 hardware decode
(`mtdECs=1`, `h264_420 supported=1`). The hardware decoder is present in
this S30 and is advertised correctly; only the userspace chain above it is
too old.

Along the way three **upstream/libva** defect classes were found that break
any client, and one **packaging** defect was found and fixed.

## 1. libva 2.24 changed the `vaCreateBuffer()` prototype

This is an upstream change in libva 2.24 and it breaks every client that was
written against the libva 1.x signature.

`va/va.h` in libva 2.24.1:

```c
VAStatus vaCreateBuffer(
    VADisplay dpy,
    VAContextID context,      /* NEW in 2.24 */
    VABufferType type,
    unsigned int size,
    unsigned int num_elements,
    void *data,
    VABufferID *buf_id
);
```

libva 1.x had `VABufferType type` in that slot. The first probe used the
1.x signature and the driver rejected it in a way that points at the wrong
thing entirely:

```
[LIBVA]:CRITICAL - DdiMedia_GetContextFromContextID:82: Invalid context: 0x1
[LIBVA]:CRITICAL - DdiMedia_CreateBuffer:2405: nullptr ctxPtr
```

`0x1` is the *buffer type* the caller passed (`VABufferTypeSliceDataBuffer`
was mistakenly given the value 1), read by the driver as the context id.
`va/va.c` in 2.24.1 confirms the new argument:

```c
VAStatus vaCreateBuffer(
    VADisplay dpy,
    VAContextID context,    /* in */
    ...
    vaStatus = ctx->vtable->vaCreateBuffer(ctx, context, type, size, num_elements, data, buf_id);
```

Two further 2.24 changes affect decode clients:

* the decode bitstream buffer type is `VASliceDataBufferType = 5`, not 1
  (`va/va.h`, `typedef enum { VAPictureParameterBufferType = 0, ... }`);
* `vaQueryConfigAttributes()` and `vaGetConfigAttributes()` now take
  `VAProfile *` / `VAEntrypoint *` instead of the values.

Probe: `tests/vadecode.c` uses the 2.24 signatures and the correct buffer
type, and gets past all three.

## 2. The S30's VA video driver cannot complete a decode

With the API fixed, the stream is accepted and the driver fails in two
different places depending on which of the two shipped driver builds is
loaded. `mtgpu_drv_video.so` is a 10 KiB shim that `dlopen()`s one of two
implementations and picks the newest `__vaDriverInit_*` entry point it
finds:

```
$ strings /usr/lib64/dri/mtgpu_drv_video.so | grep __vaDriverInit
__vaDriverInit_1_4
__vaDriverInit_1_5
__vaDriverInit_1_7
__vaDriverInit_1_12
__vaDriverInit_1_18
__vaDriverInit_1_20
```

### 2a. The packaged build (`mtgpu_drv_video_2.so`, `__vaDriverInit_1_20`)

```
Fail to create m3d bo
[LIBVA]:CRITICAL - mos_gem_bo_alloc_internal:198: fail to create m3d bo
[LIBVA]:CRITICAL - VpuApiDecSeqInit:1549: Error mos_bo_alloc DEC_MV
[LIBVA]:CRITICAL - DdiDecode_EndPicture:2008: Seq init failed.
```

The decoder initialises its VPU working memory (motion vectors, FBC
tables, task and work buffers) through the MT graphics abstraction, and the
allocation of the first of them, `DEC_MV`, fails with `M3d::CreateGpuMemory
-9` before any ioctl reaches the kernel — the kernel log for the run
contains no allocation error at all. This is a userspace defect in the
vendor driver build, not a kernel one.

### 2b. The older build (`mtgpu_drv_video_1.so`)

Swapping in the older implementation gets past the buffer allocation and
fails later, in the VPU picture submission:

```
[LIBVA]:CRITICAL - VpuApiDecPic:1708: slices number was wrong! please check.
[LIBVA]:CRITICAL - DdiDecode_EndPicture:1916: Dec frame failed.
```

The picture is rejected before the hardware is touched.

The stream is a well-formed single access unit — the S30's own encoder
produced it:

```
$ python3 -c '...'      # Annex-B NAL walk
single.h264: 11232 bytes, 4 NALs
   nal types: {7: 1, 8: 1, 6: 1, 5: 1}      # SPS, PPS, SEI, IDR
```

so this is not a malformed-input artefact. The same failure occurs for
Baseline, Main and High profiles and for two different resolutions, and it
occurs with `num_elements` 1, 2 and 4, i.e. it does not depend on how the
slice count is presented.

### 2c. The hardcoded fallback device node (not hit here, but a real hazard)

The driver has a fallback for the case where the libva wrapper does not
hand it a file descriptor:

```
696561 nullptr pDRMState
696573 nullptr pDevicefd
696585 /dev/dri/renderD128
6965a0 DDI:LIBVA Wrapper doesn't pass file descriptor for graphics adaptor, trying to open the graphics...
696608 DDI: Still failed to open the graphic adaptor, return failure
696646 Unable to get device FD
```

The disassembly of the check is unambiguous:

```asm
bdd07: cmpq $0x0,0x68(%rbx)   ; ctx->drm_state
bdd0c: je   be558              ; -> "nullptr pDRMState"
bdd1e: mov  0x68(%rbx),%rbp    ; rbp = drm_state
bdd2b: mov  0x0(%rbp),%r9d     ; r9d = drm_state->fd
bdd2f: test %r9d,%r9d
bdd32: jle  be4f0              ; fd <= 0 -> open("/dev/dri/renderD128")
```

Those offsets match Fedora's libva 2.24.1 exactly — verified against the
installed headers:

```
sizeof(VADriverContext)=480
offsetof drm_state=104 (0x68)
offsetof(struct drm_state, fd)=0
```

so the driver is *not* misreading the ABI, and on this host the fallback is
never taken (the log contains no "LIBVA Wrapper doesn't pass file
descriptor" line). It remains a latent hazard: the node is hardcoded, and on
a host where the S30 is not `renderD128` — as here, where `renderD128` is
the Intel UHD 770 — a driver that did fall back would allocate from the
wrong GPU.

### 2d. Conclusion for the VA route

VA-API hardware decode is **advertised but not implemented** in the shipped
S30 driver: the newest build cannot allocate its VPU working buffers and
the older build rejects the picture. The earlier project record said
hardware decode was "not exercised"; it has now been exercised, and it
fails.

## 3. The MUSA-native MUVID route: the runtime is too old for H.264

`musa-runtime` 1.5.4 ships `libvideo_musa.so.1.2.10`, which exposes the
MUVID hardware video API (`muvidCreateVideoParser`, `muvidCreateDecoder`,
`muvidDecodePicture`, ...). This is the MUSA-native decoder and the route a
MUSA application should use.

### 3.1 MUVID is a libva client with a Debian default path

`libvideo_musa.so` opens the VA driver itself. Its built-in default search
path is the Debian multiarch location `/usr/lib/x86_64-linux-gnu/dri`,
which does not exist on Fedora, where `musa-runtime-va` installs the driver
into `/usr/lib64/dri`. A MUVID application therefore finds no VA driver
unless `LIBVA_DRIVERS_PATH` is set. **This is a packaging gap and it is now
fixed**: `musa-runtime` 1.5.4-8 exports `LIBVA_DRIVERS_PATH=/usr/lib64/dri`
from `/etc/profile.d/zz-musa-fedora.sh`.

### 3.2 The shipped runtime implements only codec ids 0..3

`muvidCreateVideoParser` begins with

```asm
c85c4: cmpl   $0x3,(%rsi)       ; pParams->CodecType
c85c7: ja     c8680              ; -> "pParams->CodecType: {}, not support!
                                     exit with MUSA_ERROR_NOT_SUPPORTED"
```

so the runtime accepts only codec ids 0..3. Both copies of the vendor
headers number the codecs identically, and **H.264 is id 4**:

| id | enum | id | enum |
|---|---|---|---|
| 0 | MPEG1 | 6 | H264_SVC |
| 1 | MPEG2 | 7 | H264_MVC |
| 2 | MPEG4 | 8 | HEVC |
| 3 | VC1 | 9 | VP8 |
| 4 | **H264** | 10 | VP9 |
| 5 | JPEG | 11 | AV1 |

The numbering is therefore not the problem — the shipping runtime simply
had not implemented anything beyond id 3 and rejects H.264 as unsupported.
The ids it does implement are rejected on this device, because the S30's VPU
decodes H.264/HEVC/VP9/AV1, not the MPEG family or VC-1:

```
parser  codec MPEG1 (0) -> 101     decoder codec MPEG1 (0) -> 101
parser  codec MPEG2 (1) -> 101     decoder codec MPEG2 (1) -> 101
parser  codec MPEG4 (2) -> 101     decoder codec MPEG4 (2) -> 101
parser  codec VC1   (3) -> 101     decoder codec VC1   (3) -> 101
parser  codec H264  (4) -> 801     decoder codec H264  (4) -> 801
parser  codec HEVC  (8) -> 801     decoder codec HEVC  (8) -> 801
parser  codec VP9  (10) -> 801     decoder codec VP9  (10) -> 801
parser  codec AV1  (11) -> 801     decoder codec AV1  (11) -> 801
```

(`101` = `MUSA_ERROR_INVALID_DEVICE`, `801` = `MUSA_ERROR_NOT_SUPPORTED`;
the same split appears in `muvidCreateDecoder`, so it is the runtime's
codec handling and not the parser alone.)

### 3.3 The upstream Video Codec SDK 13.0 library *does* implement H.264

The research tree carries the upstream Video Codec SDK libraries
(`mt-codec-headers/libs/x86_64/libvideo_musa.so.13.0`). Its
`muvidCreateVideoParser` dispatches on the codec id with an explicit switch
that includes precisely the ids the shipped runtime is missing:

```asm
b8bf9: test %eax,%eax        ; id 0  -> MPEG1
b8c49: cmp  $0x1,%eax        ; id 1  -> MPEG2
b8c52: cmp  $0x2,%eax        ; id 2  -> MPEG4
b8c5b: cmp  $0x3,%eax        ; id 3  -> VC1
b8c64: cmp  $0x4,%eax        ; id 4  -> H264
b8c6d: cmp  $0x8,%eax        ; id 8  -> HEVC
b8c76: cmp  $0xa,%eax        ; id 10 -> VP9
b8c7f: cmp  $0xb,%eax        ; id 11 -> AV1
```

Driving the same probe against that library, from an isolated directory
with nothing on the host modified, takes the MUSA-native route past the
codec check and shows the hardware decoder is present:

```
LIBRARY /tmp/muvid13/libvideo_musa.so.1 (MUVID_LIB)
[info] load m_musa_functions: 0x222ab9c0, m_load_result: 0
[info] Query iGpu: 1
[info] libva: Trying to open /usr/lib64/dri/mtgpu_drv_video.so
[info] libva: Found init function __vaDriverInit_1_20
[info] libva: va_openDriver() returns 0
[warning] decoder caps query CodecType input:4 normalized:4
[warning] change codecType frome :4 to:4, abiKind:1
CAPS h264_420 supported=1 mtdECs=1 min=16x16 max=8192x8192 max_mb=262144 output_format_mask=0x1
MUVID_FAIL muvidCreateVideoParser -> 101
```

`mtdECs=1` — one MT decode engine — and `h264_420 supported=1`: **the
hardware H.264 decoder is present and correctly advertised**. The parser
creation then fails with `MUSA_ERROR_INVALID_DEVICE`.

### 3.4 Why the upstream library cannot be used here either

The upstream 13.0 library belongs to the MUSA 5.x generation. Paired with
the MUSA 5.2.0 driver API it aborts immediately, because that API issues
the `MTGPU_QUERY_CMD_MKT_NAME` query (sub-command 12) that the installed
mtgpu 3.1.0 kernel driver does not implement:

```
$ MUVID_MUSA_LIB=.../libmusa.so.5.2.0 ... probe stream.h264
mtgpu_device_initialize: mtgpu_query_marketing_name failed (-22)
Aborted
```

Paired with the packaged MUSA 1.5.4 runtime it loads (`m_load_result: 0`)
and gets as far as the parser, but the parser creation fails with 101. The
library and the runtime must be used as a pair: the upstream 13.0 headers
and the upstream 13.0 library agree on the struct layouts, while the
1.5.4-era runtime predates them. The library does version-adapt
("abiKind:1"), but the MUSA entry points it wants are absent from the
1.5.4 runtime:

```
muDevicePrimaryCtxRelease   muDevicePrimaryCtxReset  muDevicePrimaryCtxSetFlags
muGraphicsGLRegisterImage   muGraphicsSubResourceGetMappedArray
muLinkAddData  muLinkComplete  muLinkCreate  muLinkDestroy  muModuleGetGlobal
```

### 3.5 The blocking chain

```
mtgpu kernel driver 3.1.0   (no cmd 12 / MKT_NAME)
        |
        |  needs a driver that implements MTGPU_QUERY_CMD_MKT_NAME
        v
MUSA 5.x userspace (libmusa.so.5.2.0)
        |
        |  provides the API the Video Codec SDK 13.0 expects
        v
libvideo_musa.so.13.0  (muvidCreateVideoParser implements H.264)
        |
        v
MUSA-native H.264 hardware decode
```

Every arrow is broken at the top on this host, because the only Moore
Threads S30 kernel driver available is 3.1.0. **This is the single
prerequisite that would unblock MUSA-native hardware decode**: a Moore
Threads S30 kernel driver that implements `MTGPU_QUERY_CMD_MKT_NAME`.
Together it would also unblock the MUSA 5.x userspace and thereby the
Video Codec SDK 13.0 MUVID library with its H.264 support.

### 3.6 A second, smaller SONAME mismatch

The Moore Threads Video Codec SDK loader
(`tests/vendor/mt-codec-headers/include/ffmtcodec/internal/load_common.h`)
asks for `MUSA_LIBNAME "libmusa.so.1"`, but the packaged runtime provides
the SONAME `libmusa.so.1.5`. With the 5.2.0 orphan removed that name
resolves to nothing, so the loader fails outright:

```
Cannot load libmusa.so.1
```

`tests/muvid-decode.c` therefore resolves the libraries itself and tries
`libmusa.so.1.5`, `libmusa.so.1`, `libmusa.so` in turn. Applications built
against the Video Codec SDK need the same treatment, or a runtime that
also provides the `libmusa.so.1` SONAME.

## 4. Packaging defect found and fixed: the unowned 5.2.0 driver API

While loading the MUVID library, the probe hit a failure that looked
familiar:

```
mtgpu_device_initialize: mtgpu_query_marketing_name failed (-22)
.../libdrm-mt/mtgpu/mtgpu_internal.h:162: update_references:
    Assertion `atomic_read(dst) > 0' failed.
```

This is the same `MTGPU_QUERY_CMD_MKT_NAME` (sub-command 12) rejection that
[`root-cause-invalid-cmd-type-2026-11-23.md`](root-cause-invalid-cmd-type-2026-11-23.md)
and
[`root-cause-mthreads-gmi-drm-query-2026-11-24.md`](root-cause-mthreads-gmi-drm-query-2026-11-24.md)
describe — but the trigger is different and it is a *packaging* problem:

```
$ ls -la /usr/lib64/libmusa.so*
lrwxrwxrwx  /usr/lib64/libmusa.so      -> libmusa.so.1
lrwxrwxrwx  /usr/lib64/libmusa.so.1    -> libmusa.so.5.2.0
lrwxrwxrwx  /usr/lib64/libmusa.so.5    -> libmusa.so.5.2.0
-rwxrwxr-x  /usr/lib64/libmusa.so.5.2.0

$ rpm -qf /usr/lib64/libmusa.so.1
file /usr/lib64/libmusa.so.1 is not owned by any package
```

The 5.2.0 **driver API** left over from the SDK install is the only
provider of the SONAME `libmusa.so.1` on the host. Anything that asks for
that name — the Moore Threads Video Codec SDK loader does exactly that,
`#define MUSA_LIBNAME "libmusa.so.1"` — gets the 5.2.0 library, whose UAPI
is too new for the 3.1.0 kernel driver.

The packaged stack itself is not affected, because it is versioned
consistently:

| File | SONAME | Owner |
|---|---|---|
| `/usr/local/musa/lib/libmusa.so.1.5.4` | `libmusa.so.1.5` | `musa-runtime` |
| `/usr/local/musa/lib/libmusa.so.1.5` → `libmusa.so.1.5.4` | — | `musa-runtime` |
| `/usr/local/musa/lib/libmusa.so` → `libmusa.so.1.5` | — | `musa-runtime` |
| `/usr/local/musa/lib/libmusart.so.1.5.4` | `libmusart.so.1.5` (needs `libmusa.so.1.5`) | `musa-runtime` |
| `/usr/lib64/libmusa.so.1` | `libmusa.so.1` | **nobody** (5.2.0 leftover, `axogm:axogm`, Nov 2025) |

So `dlopen("libmusa.so")` — the unversioned name — resolved to the
*packaged* library all along, and only the explicitly versioned
`libmusa.so.1` was hijacked by the orphan.

**Fix.** The four unowned files were *moved* (never deleted) to
`/var/lib/musa-fedora/quarantine/musa-5.2-driver-api/` and `ldconfig` was
re-run. The operation is reproducible and reversible with the packaged
script:

```sh
pkexec scripts/quarantine-5.2-driver-api.sh            # quarantine
pkexec scripts/quarantine-5.2-driver-api.sh --restore  # put them back
```

After the quarantine the MUSA driver API initialises cleanly against the
3.1.0 driver:

```
LIBRARY libmusa.so.1.5
DEVICE index=0 context=ok
```

and the recurring `Invalid cmd_type: 1 or cmd: 12` journal noise stops.
(The only entries left in the boot journal are the two from 00:58, before
the quarantine, and one from the deliberate 5.2.0-driver-API experiment of
§3.4, which reproduces the same rejection on purpose.)

**Prevention.** `scripts/verify-musa.sh` gained a "MUSA runtime library
(SONAME hygiene)" section that names any foreign provider of the SONAME
`libmusa.so.1`, the same way the existing check names a foreign provider of
`libdrm_mtgpu.so.1`.

## 5. What was verified to still work

After every change in this document:

* `scripts/verify-musa.sh --skip-memcpy` → **13 passed, 0 failed, 2
  skipped** (unchanged from before the session);
* `mthreads-gmi` still reports the S30 (`Driver Version 3.1.0`);
* `h264_vaapi` encode still produces a valid stream;
* the kernel log contains **zero** `BUG`/`Oops`/general-protection/
  invalid-opcode entries and **zero** `page allocation failure` entries
  for the whole session.

## Evidence index

| Claim | Command |
|---|---|
| libva 2.24 `vaCreateBuffer` has a context argument | `grep -A8 'VAStatus vaCreateBuffer' /usr/include/va/va.h` (from `libva-devel-2.24.1-2.fc45.x86_64`) |
| decode slice-data buffer type is 5 | `sed -n '2044,2058p' va/va.h` |
| VA driver build fails to allocate DEC_MV | `scripts/hardware/test-video-decode.sh` → "VA-API route" |
| older VA driver build rejects the picture | same script, "older-build" row |
| stream is a valid single access unit | Annex-B NAL walk, §2b |
| driver fallback node is hardcoded | `strings -t x mtgpu_drv_video_2.so`, disassembly at `bdd07` |
| driver ABI offsets match libva 2.24 | `offsetof` probe against the installed headers |
| MUVID accepts only codec ids 0..3 | disassembly at `c85c4` in `libvideo_musa.so.1.2.10` |
| upstream MUVID 13.0 implements H.264/HEVC/VP9/AV1 | disassembly of `muvidCreateVideoParser` in `libvideo_musa.so.13.0` (`cmp $0x4`, `$0x8`, `$0xa`, `$0xb`) |
| the S30 does expose an MT decode engine | `muvidGetDecoderCaps` → `h264_420 supported=1 mtdECs=1` |
| the upstream library pairs with MUSA 5.x | `MUVID_MUSA_LIB=<libmusa.so.5.2.0>` → `mtgpu_query_marketing_name failed (-22)`, abort |
| the 3.1.0 kernel driver lacks cmd 12 | kernel log `Invalid cmd_type: 1 or cmd: 12`; `enum` of strings inside `mtgpu_core.o_binary` |
| MUVID is a libva client with a Debian default path | `MTDECODE_LEVEL=1 MTDECODE_PATH=…` log |
| MUVID logger knobs | `strings libvideo_musa.so.1.2.10 \| grep MTDECODE` |
| the 5.2.0 driver API is unowned | `rpm -qf /usr/lib64/libmusa.so.1` |
| the packaged runtime is versioned consistently | `readelf -d` on the three libraries |
| quarantine is reversible | `scripts/quarantine-5.2-driver-api.sh --restore` |
| no regression in the packaged stack | `scripts/verify-musa.sh --skip-memcpy` |
| no kernel fault anywhere in the session | `journalctl -k -b \| grep -ciE 'BUG\|Oops\|general protection\|invalid opcode'` |

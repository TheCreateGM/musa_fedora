# Hardware validation — MTT S30, 2026-11-26 video-decode session

Same boot as [hardware-validation-2026-11-26](hardware-validation-2026-11-26.md)
(kernel `7.3.4-300.fc45.x86_64`, boot at 00:07 UTC, `mtgpu` auto-bound to the
S30 at `1ed5:0102`). This session's subject is **hardware video decode** and a
**driver-API SONAME** packaging defect.

All GPU work ran under
`systemd-run --user --scope -p MemoryMax=3G -p MemorySwapMax=0`.

## 1. Baseline re-verification — `scripts/verify-musa.sh --skip-memcpy`

**15 passed, 0 failed, 2 skipped** (exit 0). The count is 13 + the two new
guards added in this session:

```
== MUSA runtime library (SONAME hygiene) ==
  PASS  no foreign provider of SONAME libmusa.so.1 in the linker path
        packaged runtime provides libmusa.so.1.5 (never libmusa.so.1)
  PASS  VA driver present at /usr/lib64/dri (MUVID needs LIBVA_DRIVERS_PATH)
        the packaged profile.d sets LIBVA_DRIVERS_PATH=/usr/lib64/dri
```

The two skips are unchanged and expected: `muMemcpyTest` needs more host
RAM than the capped scope allows, and the Moore Threads OpenCL ICD is
absent from this runtime.

## 2. The packaging defect and its fix

The investigation of the MUVID library turned up a **non-packaged** library
that was the only provider of the SONAME `libmusa.so.1`:

```
$ ls -la /usr/lib64/libmusa.so*
lrwxrwxrwx  axogm axogm   28 Nov 11 05:50 /usr/lib64/libmusa.so     -> libmusa.so.1
lrwxrwxrwx  axogm axogm   16 Nov 11 05:50 /usr/lib64/libmusa.so.1   -> libmusa.so.5.2.0
lrwxrwxrwx  axogm axogm   16 Nov 10 05:37 /usr/lib64/libmusa.so.5   -> libmusa.so.5.2.0
-rwxrwxr-x  axogm axogm 17781704 Nov 11 05:50 /usr/lib64/libmusa.so.5.2.0

$ rpm -qf /usr/lib64/libmusa.so.1
file /usr/lib64/libmusa.so.1 is not owned by any package
```

Any consumer of the name `libmusa.so.1` — the Moore Threads Video Codec SDK
loader uses exactly that name — therefore loaded the **5.2.0** driver API,
whose UAPI is newer than the 3.1.0 kernel driver. The failure was visible in
userspace and in the kernel:

```
mtgpu_device_initialize: mtgpu_query_marketing_name failed (-22)
.../mtgpu_internal.h:162: update_references: Assertion `atomic_read(dst) > 0' failed.

journalctl -k -b:
Nov 26 00:58:17 fedora kernel: mtgpu 0000:01:00.0: Invalid cmd_type: 1 or cmd: 12
Nov 26 00:58:58 fedora kernel: mtgpu 0000:01:00.0: Invalid cmd_type: 1 or cmd: 12
```

**Fix applied.** The four files were *moved* (never deleted) and `ldconfig`
re-run:

```
$ pkexec scripts/quarantine-5.2-driver-api.sh
mv -v '/usr/lib64/libmusa.so' ...

$ ldconfig -p | grep libmusa
	libmusa.so.3 (libc6,x86-64) => /usr/local/musa/lib/libmusa.so.3
	libmusa.so.1.5 (libc6,x86-64) => /usr/local/musa/lib/libmusa.so.1.5
	libmusa.so (libc6,x86-64) => /usr/local/musa/lib/libmusa.so
```

After the quarantine, `musaInfo` and the probe both initialise the driver
API cleanly, and **no new `Invalid cmd_type` line has appeared in the
journal** — the two entries above are both from 00:58, before the fix at
01:06.

A third entry exists at 01:20:54. It was produced deliberately, by the
isolated experiment that pairs the upstream MUVID 13.0 library with the
MUSA **5.2.0** driver API (§3c): that API issues
`MTGPU_QUERY_CMD_MKT_NAME` (sub-command 12), so the 3.1.0 kernel driver
rejects it and logs exactly this line. It is the expected behaviour of a
deliberately mismatched pairing, not a regression of the packaged stack.
No entry appeared at all during the final verification runs.

The remediation is reproducible and reversible:

```sh
pkexec scripts/quarantine-5.2-driver-api.sh            # quarantine
pkexec scripts/quarantine-5.2-driver-api.sh --restore  # put them back
```

**Packaging change.** `musa-runtime` 1.5.4-8 now exports
`LIBVA_DRIVERS_PATH=/usr/lib64/dri` from
`/etc/profile.d/zz-musa-fedora.sh`, because `libvideo_musa.so` (the MUVID
API) is a libva client whose built-in default driver path is the Debian
`/usr/lib/x86_64-linux-gnu/dri`.

**Prevention.** `scripts/verify-musa.sh` gained the "MUSA runtime library
(SONAME hygiene)" section shown in §1, which names any foreign provider of
`libmusa.so.1` in the linker path.

## 3. Hardware video decode — `scripts/hardware/test-video-decode.sh`

Run with the host's own S30 encoder to produce the test bitstream, so the
stream is known-good by construction:

```
== Test bitstream ==
  PASS  encoded a 320x240 H.264 stream with the S30 (h264_vaapi)
        6441 bytes
```

The stream is a single valid access unit (Annex-B NAL walk: SPS, PPS, SEI,
IDR — one each).

### 3a. VA-API route — **fails inside the vendor driver**

```
== VA-API route (libva VAEntrypointVLD) ==
  PASS  built tests/vadecode.c
        --- packaged ---
        VADECODE_FAIL render -> 2 (resource allocation failed)
        Failed M3d::CreateGpuMemory -9
        [LIBVA]:CRITICAL - mos_gem_bo_alloc_internal:198: fail to create m3d bo
        [LIBVA]:CRITICAL - VpuApiDecSeqInit:1549: Error mos_bo_alloc DEC_MV
        [LIBVA]:CRITICAL - DdiDecode_EndPicture:2008: Seq init failed.
        VA-API decode did not complete with the packaged driver (rc=3)
        --- older-build ---
        VADECODE_FAIL render -> 18 (invalid parameter)
        [LIBVA]:CRITICAL - DdiMediaUtil_LoadRogue2D:1666:
                   "/usr/lib/x86_64-linux-gnu/musa/librogue2d_api_MUSA.so" not exist
        [LIBVA]:CRITICAL - VpuApiDecPic:1708: slices number was wrong! please check.
        [LIBVA]:CRITICAL - DdiDecode_EndPicture:1916: Dec frame failed.
        VA-API decode did not complete with the older-build driver (rc=3)
```

Both failures are in userspace, before any decode ioctl reaches the kernel.

### 3b. MUSA-native MUVID route — **fails earlier, on the codec id**

```
== MUSA MUVID route (libvideo_musa.so) ==
  PASS  built tests/muvid-decode.c
        NOTE muvidGetDecoderCaps failed; continuing without the capability query
        MUVID_FAIL muvidCreateVideoParser -> 801
        LIBRARY libmusa.so.1.5
        LIBRARY libvideo_musa.so.1.2
        DEVICE index=0 context=ok
        CAPS h264_420 supported=1 mtdECs=0 min=0x0 max=0x0 max_mb=0 output_format_mask=0x0
        NOTE libvideo_musa.so has no muvidCtxLockCreate; proceeding without a context lock
        MUSA MUVID decode did not complete (rc=2)
```

`801` is `MUSA_ERROR_NOT_SUPPORTED`. The runtime accepts only codec ids
0..3 and the vendor headers number H.264 as 4:

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

(`101` = `MUSA_ERROR_INVALID_DEVICE`.)

### 3c. MUSA MUVID with the upstream Video Codec SDK library

The upstream Video Codec SDK library implements the H.264 codec id that the
shipped runtime rejects:

```
== MUSA MUVID route with the upstream Video Codec SDK library ==
        MUVID_FAIL muvidCreateVideoParser -> 101
        LIBRARY /usr/local/musa/lib/libmusa.so.1.5.4 (MUVID_MUSA_LIB)
        LIBRARY /tmp/test-vdecode.35sSiP/upstream/libvideo_musa.so.1 (MUVID_LIB)
        DEVICE index=0 context=ok
        CAPS h264_420 supported=1 mtdECs=1 min=16x16 max=8192x8192 max_mb=262144 output_format_mask=0x1
        upstream MUVID reports an MT decode engine but could not decode
        it is a MUSA 5.x library: it needs a newer S30 kernel driver
```

`mtdECs=1`: the MT decode engine is present and advertised for this GPU.
The library's own `muvidCreateVideoParser` dispatches on codec ids 0..3,
4, 8, 10 and 11, so it *does* implement H.264. What it cannot do is
initialise against mtgpu 3.1.0, because it is a MUSA 5.x library and the
MUSA 5.x driver API is rejected by that kernel driver:

```
$ MUVID_MUSA_LIB=<libmusa.so.5.2.0> ... probe stream.h264
mtgpu_device_initialize: mtgpu_query_marketing_name failed (-22)
Aborted
```

### 3d. Kernel log for the whole session

```
== Kernel log for this run ==
  PASS  kernel log carries no fault
```

Session-wide counters over the whole boot:

```
$ journalctl -k -b | grep -ciE 'BUG:|Oops|general protection|invalid opcode'
0
$ journalctl -k -b | grep -ci 'page allocation failure'
0
$ journalctl -k -b | grep -c 'Invalid cmd_type'
3
```

All three `Invalid cmd_type` entries are accounted for: two from 00:58
(before the SONAME quarantine at 01:06, from the 5.2.0 driver API being
picked up) and one from 01:20 (the deliberate 5.2.0-API experiment of
§3c). None comes from the packaged stack.

## 4. What still works

* `scripts/verify-musa.sh --skip-memcpy` → **15 passed, 0 failed, 2 skipped**
* `mthreads-gmi` → MTT S30, driver 3.1.0
* `h264_vaapi` encode → valid single-access-unit H.264 (6 441 bytes)
* `mcc` `mp_10` vector-add → 1 048 576 results verified
* headless Vulkan → device, buffer, host-visible map all PASS
* **zero** kernel faults, **zero** page-allocation failures

## 5. Package upgrade and post-upgrade re-verification

`musa-runtime` and `musa-runtime-va` were upgraded from 1.5.4-7 to
1.5.4-8, the release that exports `LIBVA_DRIVERS_PATH=/usr/lib64/dri` so
MUVID can find the packaged VA driver. The only payload difference is the
profile snippet; see [COPR verification](copr-verification-2026-11-26.md).

```
$ rpm -q musa-runtime musa-runtime-va
musa-runtime-1.5.4-8.fc45.x86_64
musa-runtime-va-1.5.4-8.fc45.x86_64
$ rpm -V musa-runtime musa-runtime-va     # clean
$ dnf check                                 # clean
```

| Check | Result |
|---|---|
| `scripts/verify-musa.sh --skip-memcpy` | **15 passed, 0 failed, 2 skipped** |
| `scripts/hardware/test-vaapi.sh` | **14 passed, 0 failed, 0 skipped** |
| `scripts/check-musa-env.sh` | `MUSA_ENV_STATUS: PASS` |
| login shell `LIBVA_DRIVERS_PATH` | `/usr/lib64/dri` (was unset before) |

The fix in action, from a login shell:

```
$ bash -lc 'echo $LIBVA_DRIVERS_PATH'
/usr/lib64/dri
$ ls /usr/lib64/dri/mtgpu_drv_video*.so
mtgpu_drv_video_1.so  mtgpu_drv_video_2.so  mtgpu_drv_video.so
```

The MUVID probe in that shell now loads the VA driver successfully; the
remaining failure is the codec-id limitation of the 1.5.4 runtime
(`MUSA_ERROR_NOT_SUPPORTED`), which is a separate, already-diagnosed issue.

## Verdict

Hardware video **decode** on the MTT S30 is **not working**, on any of the
three routes tried, for three fully diagnosed reasons: a defect in the
shipped VA driver build, a MUVID runtime that predates H.264 support, and
a kernel driver that cannot initialise the MUSA 5.x userspace the newer
MUVID library requires.

The hardware itself is not the limitation: the upstream Video Codec SDK
library reports `h264_420 supported=1 mtdECs=1` for this GPU — one MT
decode engine, correctly advertised. The single missing ingredient is a
Moore Threads S30 **kernel driver that implements
`MTGPU_QUERY_CMD_MKT_NAME`** (sub-command 12); with it, the MUSA 5.x
userspace and therefore the H.264-capable MUVID library would both become
usable. Hardware **encode** remains working.

The session also fixed a genuine packaging defect — an unowned 5.2.0
library was shadowing the packaged driver API for the SONAME `libmusa.so.1`
— and added a permanent guard against its return.

Full analysis: [root-cause-video-decode-2026-11-26](root-cause-video-decode-2026-11-26.md).

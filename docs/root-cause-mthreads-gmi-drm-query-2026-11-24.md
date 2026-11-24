# mthreads-gmi 2.0.0 fails to initialize mtml on the 3.1.0 driver

**Date:** 2026-11-24 (boot 2026-11-24 00:46 UTC, module loaded 01:44 UTC)
**Host:** Fedora 45, kernel `7.3.4-300.fc45.x86_64`, MTT S30 `0000:01:00.0`
**Packages:** `mtgpu-dkms` 3.1.0-12, `mtgpu-firmware` 3.1.0-4,
`musa-runtime` 1.5.4-5, `musa-sdk` 5.2.0-6, `musa-toolkit` 3.1.0-3

## Symptom

The module is live (`/sys/module/mtgpu/initstate` = `live`, `/dev/mtgpu.0`
and `/dev/dri/renderD130` present) and every compute test passes, but the
vendor telemetry tool aborts during initialization:

```
$ mthreads-gmi -q
Error: failed to initialize mtml.
$ echo $?
2
```

`mthreads-gmi` is version 2.0.0 (`--version`), installed by `musa-runtime`
at `/usr/local/musa/bin/mthreads-gmi` (also `/usr/bin/mthreads-gmi`).

## Evidence: the failing operation is a DRM query ioctl

```
strace -f -e trace=openat,ioctl -o /tmp/gmi.strace mthreads-gmi
```

Relevant sequence (fd 3 = `/dev/mtgpu.0`, fd 4 = `/dev/dri/renderD130`):

| fd | device | ioctl | argument size | result |
|----|--------|-------|---------------|--------|
| 3 | `/dev/mtgpu.0` | `_IOC(RW, 0x4d, 0xae, 0x0c)` | 12 B | 0 |
| 3 | `/dev/mtgpu.0` | `_IOC(RW, 0x4d, 0xaf, 0x08)` (x3) | 8 B | 0 |
| 4 | `/dev/dri/renderD130` | `_IOC(RW, 0x64, 0x8b, 0x10)` | 16 B | 0 |
| 4 | `/dev/dri/renderD130` | `_IOC(RW, 0x64, 0x8c, 0x148)` | 328 B | 0 |
| 4 | `/dev/dri/renderD130` | `_IOC(RW, 0x64, 0x8a, 0x28)` | 40 B | **-1 EINVAL** |

The tool opens the management node and the render node successfully, reads
`/sys/module/mtgpu/parameters/*`, and then the last query in its
initialization sequence is rejected with `EINVAL`. It never reaches the
point where it prints device telemetry.

Two negative results are important:

* **`libdrm_mtgpu` is not involved.** The trace contains no `openat` for
  `libdrm_mtgpu.so*`; the tool talks to the two device nodes directly. The
  earlier hypothesis in `docs/hardware-validation-2026-11-23-load-test.md`
  ("mthreads-gmi loads libdrm_mtgpu at runtime, so the redirected symlink is
  the likely cause") is therefore **disproven**. The symlink has since been
  repaired (`rpm -V musa-runtime` clean, see
  `docs/root-cause-drm-soname-hijack-2026-11-24.md`) and the tool still fails
  identically.
* **The failure is not a permission or open() problem.** Both device opens
  succeed as user `axogm`, and the management-node ioctls return 0.

## Driver side: the command is outside the legacy ioctl table

`pvr_drm_ioctls` parsed from the installed module
(`/lib/modules/$(uname -r)/extra/mtgpu.ko.xz`, 128 entries of 24 bytes)
contains only:

```
index 0x40  cmd 0xc0206440
index 0x41  cmd 0x40206441
index 0x42  cmd 0x00006442
index 0x43  cmd 0xc0306443
index 0x44  cmd 0x80086444
index 0x45  cmd 0x40046445
index 0x4f  cmd 0xc020644f   (DRM_IOCTL_MTGPU_CMD bridge)
```

The render node is the DDK 2.0 driver (`[drm] Initialized mtgpu 2.0.0 ...
on minor 2`, `mtgpu_drm_major=2`, the module default). Commands `0x8a`-
`0x8c` are direct vendor commands resolved by the DDK 2.0 path in the
opaque vendor core (`objs/x86_64/mtgpu_core.o_binary`, sha256
`d0fdaceab37bce7387d376f8a87a2e9dbf210eeccf4f4f7c67f8dee83f4e8200`), not
by the legacy table shown above. `0x8b` and `0x8c` succeed; `0x8a` is
answered with `EINVAL`. We do not patch the vendor blob or fake the query.

## Impact

Telemetry only. On the same live stack, in the same boot:

* `musaInfo` (1.5.4 runtime) - PASS, MTT S30, 8 SMs, 3.97 GB, 93% free
* `muMemcpyTest` (1 GiB) - PASS
* `musart-smoke` (alloc/memcpy/memset/streams/events) - PASS
* `mp_10` vector-add (1,048,576 adds) - PASS
* compute suite (saxpy, shared reduce, atomics, constant memory, libm,
  2D grid, D2D copy) - PASS
* FP64 (`abs_err=5.551e-17`) - PASS
* headless Vulkan (device/buffer/host-visible map) - PASS

So the MUSA compute path, memory management, `mcc` output and the Vulkan ICD
are unaffected. Only driver/tool telemetry is unavailable.

## What was ruled out

| Hypothesis | Result |
|------------|--------|
| Redirected `libdrm_mtgpu.so.1` symlink | Not loaded by this tool; repaired link still fails |
| PATH / wrong `mthreads-gmi` | Only one binary exists; `--version` = 2.0.0 |
| Missing privileges | Device opens and mgmt ioctls succeed as the normal user |
| Module not live | `initstate=live`, render node present, compute passes |
| Load arguments (`display=none`, `disable_fbdev=1`, `mtgpu_drm_major=2`) | Same arguments as the 2026-11-23 runs that were reported as passing |

Earlier reports (`docs/hardware-validation-2026-11-23-runtime154-load-test.md`,
`docs/hardware-validation-2026-11-23-suite-verify.md`) record
`mthreads-gmi -q` returning 0 and reporting the card. That result could not
be reproduced on 2026-11-24 with the current module and tool; the ioctl
evidence above is what the current stack actually does. Treat
`mthreads-gmi` as **not working** until a vendor update or a documented
workaround is verified on hardware.

## Workaround / alternative telemetry

Until the vendor mismatch is resolved, read the same facts from stable
interfaces:

```sh
lspci -nnk -d 1ed5:0102          # device, subsystem, bound driver
cat /sys/class/drm/card*/device/uevent
cat /sys/module/mtgpu/parameters/mtgpu_drm_major
LD_LIBRARY_PATH=/usr/local/musa/lib /usr/local/musa/bin/musaInfo
```

`scripts/verify-musa.sh` reports this as one FAIL (12 passed / 1 failed /
1 skipped) and points here; it does not hide the failure.

## Reproduce

```sh
# with the module live
strace -f -e trace=openat,ioctl -o /tmp/gmi.strace mthreads-gmi
grep -E "renderD|0x8a|0x8b|0x8c" /tmp/gmi.strace
```

Do not retry the failed 5.2.0 runtime (`libmusart.so.5`) against this driver;
that is a different, already-documented ABI failure
(`docs/root-cause-invalid-cmd-type-2026-11-23.md`).
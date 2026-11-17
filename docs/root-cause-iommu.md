# Root cause: MUSA runtime returns 801 on the MTT S30 (ENOTSUPPORTED)

> Historical root-cause record (2026-11-16). The IOMMU fix below has since been
> applied (`intel_iommu=on iommu=pt`); the reported 801 symptom no longer occurs
> — see the [2026-11-17 current state](current-state-2026-11-17.md). Retained as
> a dated analysis.

Date: 2026-11-16
Host: Fedora 45, kernel 7.3.3-300.fc45.x86_64, MTT S30 (`1ed5:0102`)

## Symptom

Every MUSA runtime entry point fails immediately:

```
$ musaGetDeviceCount -> 801 (operation not supported)
$ muMemcpyTest       -> Driver API error = 0801
$ musaInfo           -> error: 'operation not supported'(801)
```

`mthreads-gmi` works and reports the card, so the kernel driver and its raw
ioctls are healthy — the failure is above the driver.

## What was ruled out

1. **PCI/driver binding** — `mtgpu` is live, `/dev/mtgpu.0` exists, `card3` and
   `renderD130` are present.
2. **Kernel ABI mismatch (3.1.0 vs 5.2.0)** — real. The 3.1.0 UAPI stops at
   `MTGPU_QUERY_CMD_MISC_INFO (11)`; 5.2.0 inserts `MKT_NAME (12)` and
   `MEM_INFO_V2 (13)`. The 5.2 runtime asks for 12 and the kernel logs
   `Invalid cmd_type: 1 or cmd: 12`. That is why the **5.2 runtime cannot be
   used** with this driver. The 1.5.4 runtime does not send those commands.
3. **Permissions** — running the probe as root reproduces the failure.
4. **Missing card node / `drmModeGetResources(-1)`** — real but secondary; an
   LD_PRELOAD interposer that supplies the card fd does not change the result.
5. **MVGAL virtual device interference** — `mvgal` was unloaded; no change.
6. **`vm.max_map_count`** — already `2147483647` as the vendor package requires.
7. **Library mismatch** — `libmusa.so.1.5.4` and `libmusart.so.1.5.4` are
   byte-identical to the official 3.1.0 package; `libdrm_mtgpu` is statically
   linked into `libmusa`, so `/usr/lib64`'s 5.2 copy is not used.

## The actual cause

With `MUSA_LOG=0xffff`:

```
mu-ini pid:... numa awareness is unsupported on the current system
mu-ERR pid:... There is no guarantee that the device has capability to access
               system memoryplease enable IOMMU or decrease sysram to 512G
mu-ERR pid:... Unable to initialize device, id: 0
mu-ERR pid:... Unable to create hal platform
muInit ret=801(MUSA_ERROR_NOT_SUPPORTED)
```

Disassembly confirms the branch. In `Hal::M3d::Device::Init()`
(`libmusa.so.1.5.4`, `0x3894e0`):

```
call   Hal::M3d::Device::InitProps()
test   %al,%al ; je fail
mov    0x1b54(%rbp),%edx      ; system-memory-access capability
test   %edx,%edx
jne    ok
...        ; prints the "enable IOMMU or decrease sysram to 512G" message
```

The capability is derived from the kernel IOMMU state, which the driver reports
through `MTGPU_QUERY_CMD_PLATFORM_INFO`:

```
platform_info: mtlink=0 iommu_enable=0 vps=0 platform_type=0 dca=1
```

`iommu_enable=0` is correct: this host really has no IOMMU groups.

```
$ cat /proc/cmdline
... ro rootflags=subvol=root console=ttyS0,115200n8 i915.enable_psr=0
$ ls /sys/kernel/iommu_groups | wc -l
0
$ zgrep INTEL_IOMMU_DEFAULT_ON /boot/config-$(uname -r)
# CONFIG_INTEL_IOMMU_DEFAULT_ON is not set
```

The hardware and firmware support VT-d (an ACPI DMAR table is present and both
DMAR units are parsed), but the kernel was built without default-on IOMMU and
boots without `intel_iommu=on`, so no IOMMU domain exists for the GPU. The glue
implements this check faithfully:

```c
/* src/pvr/osfunc.c */
IMG_BOOL OSIsIOMMUOn(struct bus_type *psBusType) { return iommu_present(psBusType); }
```

The ported replacement (`mt_iommu_present`) is semantically identical, so this is
not a porting regression — it is the real kernel state.

## Fix

Boot the kernel with the IOMMU enabled:

```
intel_iommu=on iommu=pt
```

Applied with `scripts/enable-iommu.sh` (grubby, BLS). `iommu=pt` gives devices
identity domains, which both satisfies `iommu_present()` and keeps DMA behaviour
close to the current configuration. A reboot is required.

## Secondary defect found: vendor-core crash from the hwmon add-on

Earlier work added `src/mtgpu/mtgpu_hwmon.c`, which is **not** vendor code. Its
`vram_used_show()` / `dev_vram_used_show()` call the closed vendor core:

```c
struct mtgpu_device *mtdev = dev_get_drvdata(dev);
mtgpu_get_global_mem_stats(mtdev, &stats);
```

At boot this produced a kernel NULL dereference:

```
BUG: kernel NULL pointer dereference, address: 0000000000000040
RIP: OSGetDeviceNodeFromDrm+0x9/0x60 [mtgpu]
Call Trace:
 mtgpu_get_global_mem_stats+0x130/0x1b0 [mtgpu]
 vram_used_show+0x6b/0xc0 [mtgpu]
```

`OSGetDeviceNodeFromDrm()` dereferences `drm_device->dev_private`; the add-on
passes a `struct mtgpu_device *` obtained from a PCI device, not a DRM device,
so the lookup faults. The oops killed the user's `mvgald` daemon (PID 1293) and
tainted the kernel (`U O E`). The same path is reachable from the raw
`MTGPU_QUERY_CMD_MEM_INFO` ioctl.

The hwmon add-on must be removed from the build (it is not needed by MUSA).

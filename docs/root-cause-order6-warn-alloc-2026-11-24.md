# Root cause: `page allocation failure: order:6` from the mtgpu driver

**Date:** 2026-11-24 (analysis), resolved 2026-11-25 in `mtgpu-dkms` 3.1.0-15
**Host:** Fedora 45, kernel `7.3.4-300.fc45.x86_64`, driver `mtgpu` 3.1.0
**Severity:** non-fatal but real; it hard-failed `musaMallocHost()` on this host
**Status:** fixed in the packaged driver (heap page size 256 KiB → 16 KiB) and
re-verified on the physical MTT S30.

## Symptom

While running `tests/musa-perf.mu` (the MTT S30 performance benchmark) the
kernel logged a page-allocation warning:

```
musa-perf: page allocation failure: order:6, mode:0x140cc0(GFP_USER|__GFP_COMP),
            nodemask=(null),cpuset=user.slice,mems_allowed=0
```

Full stack for one occurrence:

```
warn_alloc+0x171/0x1c0
__alloc_pages_slowpath.constprop.0+0x3c7/0xc60
__alloc_frozen_pages_noprof+0x3e9/0x470
alloc_pages_mpol+0xb8/0x190
alloc_pages_noprof+0x49/0xe0
 ? OSAllocMem+0xaa/0x180 [mtgpu]
 ? OSAllocMem+0xaa/0x180 [mtgpu]
 OSAllocUserPages+0x30/0x50 [mtgpu]
 AllocSystemPages+0x143/0x660 [mtgpu]
 PhysmemCreateSystemMemPMR+0x185/0x3c0 [mtgpu]
 PhysmemNewLocalRamBackedPMR+0xad/0x130 [mtgpu]
 PhysHeapCreatePMR+0x35/0x40 [mtgpu]
 PhysmemNewRamBackedPMR_direct+0x226/0x610 [mtgpu]
 PhysmemNewRamBackedPMR+0x98/0x170 [mtgpu]
 mtgpu_bo_create_ioctl+0x198/0x4d0 [mtgpu]
 mtgpu_ioctl+0x1c4/0x210 [mtgpu]
```

On 2026-11-25 the same allocation path failed outright instead of only warning:
`musaMallocHost()` returned `musaErrorOutOfMemory`, and `scripts/s30-perf.sh`
aborted before producing a `PERF_STATUS`.

## Root cause

**An order-6 allocation is 64 pages, i.e. 256 KiB — not 16 MiB.** The earlier
revision of this document said 16 MiB, which was wrong by a factor of 64 and
sent the investigation in the wrong direction. The correct arithmetic:

```
order 6  = 2^6 pages = 64 pages
64 × 4 KiB = 256 KiB
```

The S30 has no dedicated VRAM on this board: the driver backs buffer objects
with **host RAM**. For this system-memory path the allocation unit is the
driver's system/SVM heap page size, defined in `inc/config_kernel.h` as:

```c
#define PVRSRV_APPHINT_GENERALSVMHEAPPAGESIZE 0x40000   /* 256 KiB, order 6 */
```

That exactly matches the failing order. The request therefore was not driven by
the size of any user buffer; it was the driver's own 256 KiB heap page. On this
7.4 GiB host the memory cgroup and fragmentation frequently left no order-6
block: at the time of the first failure `/proc/buddyinfo` showed **0 free
order-6 blocks** and thousands of order-2 blocks. `GFP_USER|__GFP_COMP` made
each request even harder by asking the kernel to compact.

`OSAllocUserPages()` retries the same order up to five times and has **no
lower-order fallback**, so a fragmented host hard-fails the request.

## Resolution (driver 3.1.0-15)

`PVRSRV_APPHINT_GENERALSVMHEAPPAGESIZE` is now **`0x4000`** (16 KiB, order 2):

```c
#define PVRSRV_APPHINT_GENERALSVMHEAPPAGESIZE 0x4000    /* 16 KiB, order 2 */
```

This is the same page size the vendor driver already uses for its non-4K heap
(`GeneralNon4KHeapPageSize 0x4000`), so it is an existing, exercised driver
path rather than a new invention. The module was rebuilt through DKMS, the host
rebooted, and the S30 bound the patched module; `/usr/src/mtgpu-3.1.0/inc/config_kernel.h`
and the packaged `mtgpu-dkms-3.1.0.tar.gz` both contain `0x4000`.

Verified on the physical S30 on 2026-11-25:

| Check | Result |
|---|---|
| `journalctl -k -b \| grep -c 'page allocation failure\|warn_alloc'` | **0** |
| `tests/musa-alloc-latency.mu` (32 MiB, 60 rounds) | `p50=0.064 ms p95=0.074 ms max=0.085 ms`, `ALLOC_LATENCY: PASS` |
| `scripts/s30-perf.sh` | `PERF_STATUS: PASS (1 skipped)`, no allocation failure, 6.4 GB/s pinned H2D |
| `scripts/verify-musa.sh --skip-memcpy` | 13 passed, 0 failed, 2 skipped |

The page-size change does not reduce GPU memory bandwidth: the benchmark's
pinned H2D/D2H still measures 6.4/6.4 GB/s and the triad kernel 52.0 GB/s,
inside the ranges recorded before the change.

## Why this is a driver fix and not a host tuning workaround

Shrinking the benchmark's working set and freeing staging buffers between
stages (the earlier mitigation) only reduced the number of order-6 requests; it
did not make them satisfiable. The allocating unit belonged to the driver, so
the durable fix is in the driver's configured heap page size. The change keeps
the retry behaviour and the allocation path identical; it only makes each
request an order-2 allocation that this host can satisfy reliably.

## Guidance

- **Size GPU working sets against host free memory, not against the 4 GiB
  device.** On this host the device memory is also host memory.
- **Watch `MemAvailable` before a large GPU job.** `scripts/s30-perf-profile.sh`
  reports it and flags the low case.
- **Do not treat `warn_alloc` as cosmetic.** It is recoverable once, but with no
  lower-order fallback a persistent shortfall becomes `musaErrorOutOfMemory`
  inside an application.
- If a future vendor drop restores a larger SVM heap page, re-check
  `/proc/buddyinfo` for the matching order before running large workloads.

## What was *not* done

- No boot-time reservation or `vm.*` tuning was applied. The fix is the driver's
  allocation unit, which is portable to any host.
- `GeneralNon4KHeapPageSize` was left at `0x4000`; it is a different heap and
  was already the value the driver used.
- The driver was **not** blacklisted and the module was not unloaded/reloaded as
  an experiment; the change took effect through the normal reboot with compute
  mode enabled.

## See also

- [Performance tuning](performance-tuning.md)
- [Limitations](limitations.md)
- [2026-11-25 hardware validation](hardware-validation-2026-11-25.md)

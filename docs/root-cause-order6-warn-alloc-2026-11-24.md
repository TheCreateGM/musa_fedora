# Root cause: `page allocation failure: order:6` from the mtgpu driver

**Date:** 2026-11-24
**Host:** Fedora 45, kernel `7.3.4-300.fc45.x86_64`, driver `mtgpu` 3.1.0
**Severity:** non-fatal (`warn_alloc`), but a real signal
**Status:** root-caused and worked around in the benchmark; the underlying
driver behaviour is unchanged.

## Symptom

While running `tests/musa-perf.mu` (the MTT S30 performance benchmark) the
kernel logs a page-allocation warning:

```
musa-perf: page allocation failure: order:6, mode:0x140cc0(GFP_USER|__GFP_COMP),
            nodemask=(null),cpuset=user.slice,mems_allowed=0
```

Occurrences on this boot (all six attributed to the benchmark process by its
`Comm:` field):

```
journalctl -k -b | grep -c "order:6"
6
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

## Root cause

`order:6` is a **4096-page (16 MiB) physically contiguous** host allocation.

The S30 has no dedicated VRAM on this board: the driver backs every buffer
object with **page-backed host RAM**. The chain above shows exactly that:

`mtgpu_bo_create_ioctl` → `PhysmemNewRamBackedPMR` → `PhysmemNewLocalRamBackedPMR`
→ `PhysmemCreateSystemMemPMR` → `AllocSystemPages` → `OSAllocUserPages` →
`OSAllocMem` → `alloc_pages`.

So every `musaMalloc()`, every pinned `musaMallocHost()` allocation, and every
DMA staging buffer is a host allocation through this path. `mode:
0x140cc0(GFP_USER|__GFP_COMP)` confirms it: `__GFP_COMP` asks the kernel to
**compaction** to satisfy the order-6 request, i.e. the driver *knows* it needs
a large contiguous range.

The allocation fails because the host cannot produce a 16 MiB contiguous range
at that moment. Three host conditions combine on this machine:

1. `MemTotal` is only 7.4 GiB, and `MemAvailable` was ~2–3 GiB.
2. `SwapTotal` is 7.4 GiB with ~3.5 GiB in use, so the kernel is actively
   reclaiming (`pgsteal_kswapd` in the tens of millions of pages).
3. `user.slice` carries `memory.min = 262144000` (250 MiB), which pins a
   protected floor inside the cgroup the benchmark runs in, so reclaim cannot
   free the whole shortfall.

`cpuset=user.slice` in the message is the cgroup, and `mems_allowed=0` means
the allocation was refused **because the cgroup's memory limit could not be
raised**, not because the machine was out of memory in general.

The warning is `warn_alloc`, not a page fault or an OOM kill. The kernel
recovers and the allocation is eventually satisfied by the driver's retry
logic: `OSAllocUserPages` retries the same order up to 5 times before it
would hard-fail. There is **no lower-order fallback**, so a host that cannot
produce 16 MiB contiguous after 5 retries hard-fails the allocation instead of
degrading gracefully.

## Why the benchmark triggered it

The driver allocates a **16 MiB contiguous** region for the PMR regardless of
how small the user's request is. The trigger is therefore the *total live
footprint* of page-backed buffers, not the size of any single user buffer.

This was measured directly during this work:

| Configuration | Result |
|---|---|
| 64 MiB per buffer (earlier revision) | `order:6` warnings, run still passed |
| 32 MiB per buffer, all stage buffers alive at once | `order:6` warnings (2026-11-24 09:29:34) |
| 32 MiB per buffer, each stage's staging freed before the next stage allocates | **no warnings** |

The last row is the fix that is in the tree: `tests/musa-perf.mu` now calls
`musaFreeHost()` on the MEMBW staging buffers before the OVERLAP section
allocates its own, so the total live page-backed footprint stays flat instead
of growing with each section.

Verified after the change — four consecutive runs, no `order:6`:

```
$ for i in 1 2 3 4; do scripts/s30-perf.sh --no-build 2>&1 \
      | grep -E "speedup=|PERF_STATUS|allocation failure"; done
        speedup=1.13x
PERF_STATUS: PASS (1 skipped)
        speedup=1.14x
PERF_STATUS: PASS (1 skipped)
        speedup=1.11x
PERF_STATUS: PASS (1 skipped)
        speedup=1.14x
PERF_STATUS: PASS (1 skipped)
```

## Guidance

- **Size GPU working sets against host free memory, not against the 4 GiB
  device.** On this host the device memory is *also* host memory.
- **Free a stage's staging buffers before allocating the next stage's.** This
  is the single change that removed the warnings here.
- **Keep the per-allocation size ≤ 32 MiB** in benchmarks and applications
  running under a `user.slice` with a `memory.min` floor.
- **Watch `MemAvailable` before a large GPU job**, not just before a large
  *system* job. `scripts/s30-perf-profile.sh` reports this.

## What was *not* done

- No module parameter was changed. `mtgpu_page_size` and
  `GeneralNon4KHeapPageSize` were considered; neither affects the order of the
  driver's *host* allocation, and changing them requires a module reload,
  which this project deliberately does not do as a routine experiment — see
  [the rebind fault](root-cause-rebind-2026-11-16.md).
- No boot-time reservation or `vm.*` tuning was applied. The fix belongs in
  the allocating application, not in the host's memory configuration.
- The driver was **not** blacklisted. The warning is not a driver fault: the
  allocation was refused by the cgroup, and the driver recovered every time.

## See also

- [Performance tuning](performance-tuning.md)
- [Limitations](limitations.md)

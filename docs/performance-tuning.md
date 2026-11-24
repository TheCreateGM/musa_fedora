# MTT S30 performance tuning

All numbers on this page were measured on the real MTT S30 in this repository
with `scripts/s30-perf.sh` (kernel `7.3.4-300.fc45.x86_64`, driver `mtgpu`
3.1.0, MUSA runtime 1.5.4, MUSA 5.2.0 `mcc` targeting `mp_10`). Reproduce
any figure with:

```bash
./scripts/s30-perf.sh              # build + run, reports PERF_STATUS
./scripts/s30-perf-profile.sh      # read-only driver/host tuning report
```

Every figure below comes from a run whose outputs were verified against a
host-side reference, so a "fast" number cannot come from a kernel that did
less work. Nothing here changes system configuration.

## The platform in numbers

| Property | Measured value |
|---|---|
| PCI identity | `1ed5:0102` at `0000:01:00.0`, bound to `mtgpu` |
| Device memory | 4267532288 bytes (4.0 GiB), ~3.96e9 free at idle |
| Multiprocessors | 8 |
| Core clock | 1300 MHz |
| Memory clock / bus | 4250 MHz / 256 bit |
| Theoretical peak memory bandwidth | **272.0 GB/s** |
| Warp size | **128** (mp_10) |
| Concurrency capability flags | `GpuOverlap=1`, `ConcurrentKernels=1`, `AsyncEngineCount=1` |
| PCIe link | **3.0 ×8** (`8.0 GT/s` × `8` lanes) |
| PCIe ceiling | ~7.9 GB/s per direction |

## The bottleneck: the PCIe link

The single most important fact for tuning on this host is that the S30 sits
behind a **PCIe 3.0 ×8** link:

```
theoretical device bandwidth   272.0 GB/s
PCIe 3.0 x8 ceiling              ~7.9 GB/s per direction
measured H2D / D2H               ~6.3 / ~6.0 GB/s   (about 81% of the link)
```

Measured host↔device transfer already reaches ~81% of what the link can ever
carry, while being roughly **42× below the GPU's own memory bandwidth**. Any
optimization that reduces traffic across PCIe is worth far more than one that
makes the kernels marginally faster.

Practical consequence: **keep data on the device between phases.** A kernel
that reads its input from device memory runs at up to ~53 GB/s (measured, see
below); the same bytes pulled over PCIe run at ~6.3 GB/s.

## Measured optimizations

### 1. Pinned staging memory — +12% to +29%

`musaMallocHost()` staging beats plain `malloc()` buffers for every transfer.

| Direction | Pinned | Pageable | Gain |
|---|---|---|---|
| Host→Device | 6.3–6.5 GB/s | 4.9–5.1 GB/s | **1.25×–1.29×** |
| Device→Host | 6.0–6.5 GB/s | 5.3–5.7 GB/s | **1.11×–1.16×** |

Use `musaMallocHost()`/`musaHostRegister()` for any buffer that is copied
more than once per job, and reuse those buffers instead of re-pinning.

### 2. Fewer, larger kernel launches — ~90–170 µs per batch

Measured launch overhead: **2.7–3.0 µs per kernel launch**.

Splitting a fixed body of work across 64 launches instead of one measurably
costs GPU time:

| Launch form | Time |
|---|---|
| one launch, 32768 blocks | **2.14–2.21 ms** |
| same work, 64 launches of 512 blocks | 2.30–2.31 ms |
| one persistent launch (32 blocks, grid-stride) | 2.28–2.32 ms |

Fusing 64 launches into one saved **88–168 µs**, matching what the 2.7–3.0
µs/launch figure predicts (≈167–191 µs). The error term shows the saving is
genuine launch overhead rather than scheduling noise.

Guidance:

- Fuse per-element work into a single kernel per logical stage.
- Never launch one small kernel per item from a host loop — at ~3 µs each a
  10 000-item loop burns ~30 ms of pure launch overhead.
- A persistent/grid-stride kernel is *not* automatically faster here: with
  only 32 blocks on 8 multiprocessors it matched, not beat, the full grid.
  Use persistence to amortize *repeated* work, and give it enough blocks to
  fill the machine.

### 3. Block size: prefer 1024 — up to +8%

Triad throughput (2 reads + 1 write, effective traffic) by `blockDim`:

| blockDim | grid | Effective bandwidth |
|---|---|---|
| 128 | 65536 | 48.5–49.6 GB/s |
| 256 | 32768 | 48.8–50.6 GB/s |
| 512 | 16384 | 49.1–50.5 GB/s |
| **1024** | 8192 | **51.9–53.1 GB/s** |

`blockDim=1024` was best in every run, ~8% above 128. Because mp_10 has a
**warp size of 128**, keep block sizes as multiples of 128; a block of 64
threads occupies below one warp.

Note the realistic ceiling: a naive global-memory triad reaches ~53 GB/s, about
**19% of the 272 GB/s** theoretical peak. Raising that needs better data
reuse (tiling into shared memory), not launch tuning.

### 4. Double-buffered streams — 1.11× to 1.14×

A two-stream copy/compute pipeline (pinned, `musaMemcpyAsync`) versus an
explicit serial copy/flop/copy per chunk:

| Run | Serial | Pipelined | Speedup |
|---|---|---|---|
| run 1 | 19.36 ms | 17.09 ms | **1.13×** |
| run 2 | 19.41 ms | 17.09 ms | **1.14×** |
| run 3 | 18.92 ms | 17.05 ms | **1.11×** |
| run 4 | 19.52 ms | 17.07 ms | **1.14×** |

**Both variants must use the same staging model for this comparison to mean
anything.** An earlier revision of this benchmark staged the serial path
through pageable `malloc()` memory and the pipelined path through pinned
memory, so it was really measuring pageable-vs-pinned and reported a
meaningless 0.98×–1.13× spread. With both paths pinned and the timing taken as
the best of 10 passes, the pipeline's advantage is a reproducible **11–14%**.

The gain is bounded: the working set crosses PCIe once per direction, so there
is little compute to hide behind the copy. **Overlap is worth doing, but expect
a small win, not a transformation.**

### 5. What does *not* work

- **Cache carveout tuning is unavailable.**
  `musaDeviceSetCacheConfig(musaFuncCachePreferShared/L1/Equal)` returns
  `musaErrorNotSupported` (**801**) on the S30. The runtime exposes no
  shared/L1 split knob, so cache configuration is not a tuning lever on this
  device. `scripts/s30-perf.sh` reports this as an honest `SKIP`.
- **Moore Threads OpenCL** is absent from this install, and the archived
  2.1.1 ICD caused a kernel `Oops` — see `docs/limitations.md`. Do not
  install it.

## Driver parameters (read-only inspection)

`scripts/s30-perf-profile.sh` reports the module parameters that matter. The
ones that are load-time only and therefore **cannot** be changed without a
module reload are shown for understanding, not as live knobs:

| Parameter | Value on this host | Meaning |
|---|---|---|
| `rm_feature_level` | `0` | **Must stay 0.** `1` activated an incomplete RM path and oopsed in `gpu_phys_heap_alloc_mem` — see `docs/root-cause-rm-feature-level-2026-11-20.md` |
| `display` | `none` | Headless compute; the desktop stays on Intel `i915` |
| `disable_fbdev` | `Y` | No fbdev console contention with compute |
| `enable_gpu_dvfs` | `Y` | GPU may clock up under load |
| `mtgpu_pstate_mode` | `1` | Power-state policy |
| `mtgpu_sched_mode` | `2` | Scheduler mode; `mtgpu_sched_host_policy=1`, `mtgpu_sched_early_submit=Y` |
| `mtgpu_vram_size_total` | `17179869184` | **Not the device size.** It is a module default of 16 GiB; the real S30 is 4 GiB and the runtime reports 4267532288 bytes |
| `GeneralNon4KHeapPageSize` | `0x4000` | 16 KiB (order 2) heap page size — unrelated to the order-6 warnings below |

These are load-time module parameters. Changing one requires a module reload
or reboot; **do not reload `mtgpu` as a routine experiment** — a historical
rebind caused a kernel fault (`docs/root-cause-rebind-2026-11-16.md`).

`scripts/s30-perf-profile.sh --print-dropin` prints, but does **not install**,
a modprobe.d proposal for hosts that want to set the compute values
explicitly. Review it before saving anything to `/etc/modprobe.d/`.

## Host memory matters to the GPU

The driver creates buffer objects through `PhysmemNewRamBackedPMR()`
(`OSAllocUserPages()`, `GFP_USER|__GFP_COMP`), so large `musaMalloc()` and
pinned allocations consume **host** memory as well as device memory.

On 2026-11-24 a 64 MiB-per-buffer benchmark produced `page allocation failure:
order:6` warnings under a `user.slice` memory cgroup with 256 MiB
`memory.min`, while `MemAvailable` was ~2 GiB and swap was in use. Reducing
the working set to 32 MiB and the warnings stopped, and the run completed.
`warn_alloc` is non-fatal — the run still passed — but it is a real signal:

- size GPU working sets against **host** free memory, not just the 4 GiB device;
- `scripts/s30-perf-profile.sh` reports `MemAvailable` and flags the low case;
- the retry policy has no lower-order fallback (`OSAllocUserPages` retries the
  same order up to 5 times), so fragmented host memory can hard-fail an
  allocation.

The full root cause, including the exact kernel call chain and a repeatable
reproduction recipe, is in
[order:6 warn_alloc](root-cause-order6-warn-alloc-2026-11-24.md).

The benchmark now also **releases each stage's staging buffers before the
next stage allocates its own** (`musaFreeHost()` between the MEMBW and OVERLAP
sections). Keeping both sets alive at once re-introduced the order:6 warning
even at 32 MiB per buffer, because the driver's 16 MiB contiguous allocation
is as sensitive to the *total* live footprint as to the per-buffer size.

## Logging noise that is *not* a fault

`mtgpu 0000:01:00.0: irq:N affinity has changed.` is emitted by `irqbalance`
migrating the driver's MSI vectors (10–60 s cadence, ~200–470 lines per boot).
It is cosmetic and does not affect benchmark results. It is intentionally
**not** suppressed by banning the device from `irqbalance` — see
`docs/root-cause-irq-affinity-2026-11-23.md` for why that option was rejected.

## Applied optimization summary

What this project actually recommends, in order of measured effect:

1. **Minimize PCIe traffic** — at ~6.3 GB/s versus 272 GB/s internal, this
   dominates everything else.
2. **Pin staging buffers** with `musaMallocHost()` — measured +12% to +29%.
3. **Fuse launches** — ~3 µs each; batching saved 88–168 µs per 64-launch batch.
4. **Use `blockDim` multiples of 128, preferring 1024** — up to +8%.
5. **Double-buffer with streams** — 1.11×–1.14× measured, worthwhile but modest.
6. **Do not bother with cache carveout** — unsupported (801) on mp_10.

## Known limitations

- These are single-GPU, single-run-of-record measurements on one host; figures
  vary by a few percent run to run. Re-run `scripts/s30-perf.sh` before
  treating any value as a regression.
- Only the exercised operations are validated. Display output, video encode/
  decode, OpenGL, `torch_musa`, and Moore Threads OpenCL are **not** claimed
  — see [`limitations.md`](limitations.md).
- Emulated or container-based package tests never prove hardware behaviour;
  every number above came from the physical S30.

See also: [`supported-features.md`](supported-features.md),
[`compatibility.md`](compatibility.md), [`troubleshooting.md`](troubleshooting.md).

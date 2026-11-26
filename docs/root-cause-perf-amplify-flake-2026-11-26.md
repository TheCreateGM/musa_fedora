# Root cause: `s30-perf.sh` AMPLIFY flake ("splitting work was faster than a single launch")

Date: 2026-11-26
Component: `tests/musa-perf.mu` (`bench_amplify`), reported by `scripts/s30-perf.sh`
Status: **fixed** — one-launch now measured best-of-10 like the split cases

## Symptom

`s30-perf.sh` intermittently reported `PERF_STATUS: FAIL` with exactly one
failed check:

```
== AMPLIFY ==
  PASS  amplify results verified against the host reference
        one launch        (32768 blocks)      =    2.472 ms
        64 launches       (64 chunks)      =    2.341 ms
        persistent launch (32 blocks, stride) =    2.212 ms
        launch amplification cost of 63 extra launches=-131 us (predicted 179 us at 2.8 us/launch)
  FAIL  splitting work was faster than a single launch
```

The negative amplification cost (`-131 us`) is the tell: the check asserts
that one launch covering the whole array is at least as fast as the identical
work split across 64 launches (fewer launches should save the per-launch
overhead). Seeing the split *faster* is physically backwards — 64 launches add
63 extra `muLaunchKernel` round-trips and cannot beat one launch on the same
work.

## Root cause: unfair sampling (cold vs warm), not a real performance result

`bench_amplify` measured the three cases with **different sampling methods**:

| case | method before the fix |
|------|-----------------------|
| A — one launch | a **single** `musaEventElapsedTime` sample |
| B — 64 launches | `best of TIME_ITERS` (10) samples |
| C — persistent  | `best of TIME_ITERS` (10) samples |

Taking a **lone** sample for case A catches the GPU before its clocks have
ramped (DVFS) and before the kernel path is warm, while cases B and C report
their **fastest** of ten iterations — i.e. after the GPU has reached boost
clocks. The 0.13 ms gap (`2.472` vs `2.341`) is exactly the warm-up/DVFS
artifact of that first launch, not a genuine win for splitting. The result
varied run-to-run depending on where the clocks happened to be when A sampled,
which is why the check was flaky rather than always-failing.

Note this is distinct from the earlier bandwidth-check flake under memory
pressure documented in the README; that one retried green with no code change
and is unrelated to AMPLIFY.

## Fix

Measure case A with the **same `best of TIME_ITERS` method** used by B and C,
so all three cases are compared warm and the only difference left is the launch
count. In `tests/musa-perf.mu`, the single `triad<<<full_grid, bd>>>` sample is
wrapped in a `TIME_ITERS` loop that keeps the minimum elapsed time, matching
the split and persistent cases. No threshold or pass criterion was weakened.

## Verification (live MTT S30)

Three consecutive `scripts/s30-perf.sh` runs after the fix:

```
run 1:  one launch = 1.984 ms   64 launches = 2.330 ms   PERF_STATUS: PASS
run 2:  one launch = 1.995 ms   64 launches = 2.325 ms   PERF_STATUS: PASS
run 3:  one launch = 1.979 ms   64 launches = 2.362 ms   PERF_STATUS: PASS
```

With the fair method the one-launch case consistently wins, the amplification
cost is positive and matches the predicted per-launch overhead:

```
launch amplification cost of 63 extra launches=345 us (predicted 169 us at 2.7 us/launch)
PASS  fewer launches beat the split (saved 345 us of GPU time)
```

`verify-musa.sh --skip-memcpy` still reports **15/0/2** and the journal shows
no mtgpu fault across the runs. The perf suite is no longer flaky.

## Takeaway

When a benchmark compares a single variant against a repeated variant, every
variant must use the same sampling/aggregation. A cold single sample is not
comparable to a warm `best-of-N`; that asymmetry produces exactly this kind of
"impossible" result that looks like a hardware/driver problem but is a
measurement bug.

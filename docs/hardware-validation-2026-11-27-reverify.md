# Hardware validation — 2026-11-27 (full-gate re-verify after the musa-infer session)

Host: Fedora 45 Workstation, kernel `7.3.4-300.fc45.x86_64`, MTT S30
`1ed5:0102` bound to `mtgpu` on a normal boot (compute mode, no blacklist).
PCIe Gen3 x8, LPDDR4 4266 Mbps (Samsung), 4096 MiB total / 266 MiB used at
session start. Installed set: `mtgpu-dkms 3.1.0-16`, `mtgpu-firmware 3.1.0-5`,
`musa-runtime 1.5.4-8`, `musa-runtime-va 1.5.4-8`, `musa-sdk 5.2.0-7`,
`musa-toolkit 3.1.0-6`, `musa-cuda-compat 1.0.0-1`, `mvgal 0.7.48-1`.
COPR authenticated as `axogm`; GitHub authenticated as `nnsupermax`.

## Results (this boot)

| Suite | Result |
| --- | --- |
| `scripts/verify-musa.sh --skip-memcpy` | **15/0/2** (musart-smoke, vector-add 1M floats, saxpy/shared-reduce/atomics/constant-mem/libm/2D-grid/D2D/fp64 PASS; Vulkan headless device+memory PASS; OpenCL SKIP — no MT ICD) |
| `scripts/s30-perf.sh` | **PASS** (best triad 51.7 GB/s at blockDim 1024; two-stream pipeline **1.11×** with verified output; 1 CACHE SKIP — `PreferShared` carveout not exposed on mp_10) |
| `scripts/hardware/test-vaapi.sh` | **14/0/0** (encoded stream decodes cleanly, no kernel log entries during the run) |
| `scripts/check-musa-env.sh` | **PASS** (musaInfo → 1.5.4 runtime, mcc → 5.2.0, LD_LIBRARY_PATH ordering correct) |
| `tests/musa-infer/run.sh` | **PASS** — all four checks (compare status, GPU tokenizer/decode, chat template) |
| `apps/musa-infer` fresh run | TinyLlama-1.1B-Chat Q8_0: prompt 12.59 tok/s, **generate 12.50 tok/s** over 16 tokens; top-1 and greedy text `, World!` unchanged |
| `scripts/verify-copr-chroots.sh` | **20/20** — as this run reported it. Correction added later the same day: that score covered only four chroots for `musa-fedora`, because the script's map predated the `fedora-43` chroot enabled there on 2026-11-26. The map now lists all five chroots of both projects, is cross-checked against the COPR API per run, and re-reports **24/24** including `fedora-43` artifacts in `musa-fedora` (see [compatibility](compatibility.md)). |
| `scripts/test-container-install.sh --skip-memcpy` (isolated podman) | **13/0** — RPM set installs in a clean Fedora 45 container; packaged runtime discovers the S30; packaged `mcc` mp_10 vector-add runs on the physical S30 |

## Journal

Kernel messages matching `mtgpu.*(fault|error|oops|warn|fail)` in this boot: 6,
all known-benign — the unsigned out-of-tree module taint line, the AER status
clear notice, and the three `event_report` mkdir failures plus `xid_record`
file-create notice already root-caused in
[root-cause-mtgpu-event-report-mkdir-2026-11-23](root-cause-mtgpu-event-report-mkdir-2026-11-23.md).
No BUG/Oops/general-protection/null-pointer and no page-allocation failure.

## Verdict

Every gate that passed on 2026-11-26 reproduces on this boot with the current
installed set (`3.1.0-16` / `1.5.4-8` / `5.2.0-7` / `1.0.0-1`), including the
new `musa-infer` LLM workload at its documented performance. Nothing required a
blacklist, module replacement, or boot-config change. Both git repositories are
in sync with their remotes; both COPR repos publish succeeded builds for all
supported x86_64 chroots.

Remaining items are unchanged and vendor-dependent: no Moore Threads OpenCL ICD
safe for this driver (the archived 2.1.1 ICD oopsed the kernel and must not be
rerun), no `mublas`/`mudnn` for `mp_10` (frameworks cannot target the S30 —
`torch_musa` issue #134), S30 display output not configured (i915 drives the
desktop), and the `MKT_NAME` query sub-command lives inside the vendor binary
object and cannot be added from open source (see
`musa-data/research/video-decode-codec-enum-mismatch-2026-11-26.md` §4.3c).

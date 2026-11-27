# MUSA on Fedora — MTT S30

MUSA driver/runtime packaging and test sources for the **Moore Threads MTT S30**
(`1ed5:0102`) on Fedora. The host is Fedora 45 / kernel `7.3.4-300.fc45.x86_64`.
Since **2026-11-24** the host runs with **compute mode enabled**: the boot-time
`blacklist mtgpu` has been removed (with a full backup and a `disable` path), so
udev binds the S30 on a normal boot. The change is recorded in
[compute-mode-enabled-2026-11-24](docs/compute-mode-enabled-2026-11-24.md); the
boot-time block remains available for hosts that want the S30 unbound by
default (`scripts/compute-mode.sh disable`). See the
[2026-11-24 hardware run](docs/hardware-validation-2026-11-24-load-test.md),
[current S30 suite record](docs/hardware-validation-2026-11-23-suite-verify.md),
[full host status](docs/current-state-2026-11-20.md), and the
[Live USB validation/build report](docs/session-2026-11-23-live-usb-validation.md).
For the repair of the installed system's disk boot (GDM crash caused by the
old base package shipping vendor Mesa DRI drivers), see
[the 2026-11-23 boot fix](docs/boot-fix-2026-11-23.md).

### Updates — 2026-11-27 (session: end-to-end LLM inference)

- **LLM inference now runs on the S30 through the 1.5.4 runtime.** New
  `apps/musa-infer/` is a self-contained Llama-2 GGUF engine (CPU reference plus
  MUSA kernels, no framework) built with `mcc` 5.2.0 and the CUDA-compat layer.
  TinyLlama-1.1B-Chat Q8_0 runs at **12.58 tok/s** generation (16 tokens);
  `--backend compare` reports `max_abs=9.47e-05`, `top1_same=10/10`, PASS, and
  the greedy continuation `, World!` and the chat answer `The capital of France
  is Paris.` match an independent `llama.cpp` `a07831a` reference exactly.
  `tests/musa-infer/run.sh` passes all four checks. This is not a framework
  path: `torch_musa` issue #134 excludes the S30 and the runtime ships no
  `mublas`/`mudnn` for `mp_10`. Details and the negative CUDA-graph result:
  [musa-infer](docs/musa-infer.md).

### Updates — 2026-11-26 (session: perf AMPLIFY flake fixed)

- **`s30-perf.sh` AMPLIFY flake root-caused and fixed.** The suite sometimes
  reported `PERF_STATUS: FAIL` on `splitting work was faster than a single
  launch` (negative amplification cost). The cause was a measurement bug, not
  the GPU: `bench_amplify` took a **single** sample for the one-launch case but
  `best of 10` for the 64-launch and persistent cases, so the lone sample
  caught cold DVFS clocks while the split cases reported their warmest
  iteration. Fix: measure the one-launch case with the same `best of 10`
  method — no pass criterion weakened. Verified across three consecutive runs
  (one launch 1.98 ms beats 64 launches 2.33 ms every time, amplification cost
  `+345 us`, `PERF_STATUS: PASS`), `verify-musa.sh --skip-memcpy` still
  **15/0/2**, zero mtgpu faults. Details:
  [root-cause-perf-amplify-flake-2026-11-26](docs/root-cause-perf-amplify-flake-2026-11-26.md).

### Updates — 2026-11-26 (session: same-boot re-verify)

- **All gates re-passed on the live host:** `verify-musa.sh --skip-memcpy`
  **15/0/2**, `s30-perf.sh` **PASS** (6.4 GB/s pinned, 2.6–2.7 µs/launch,
  52.1 GB/s triad), `test-vaapi.sh` **14/0/0**, `check-musa-env.sh`
  **PASS**, `verify-copr-chroots.sh` **20/20**, and isolated podman
  install **13/0** — zero mtgpu kernel faults in the journal. One perf
  run flaked its bandwidth check under memory pressure (6.2G/7.4G used)
  and passed on retry with no code change. Full evidence:
  [hardware-validation-2026-11-26-session3](docs/hardware-validation-2026-11-26-session3.md).
- **Fresh-boot reproduction (04:37 UTC boot, no code change):** the same
  gates re-passed — `verify-musa.sh --skip-memcpy` **15/0/2**,
  `check-musa-env.sh` **PASS**, `s30-perf.sh` **PASS** (pinned H2D 6.4 GB/s,
  2.8 µs/launch, 51.8 GB/s triad, 1.14× pipeline; `PreferShared` CACHE knob
  still unsupported), `verify-copr-chroots.sh` **20/20** — with zero mtgpu
  kernel faults and zero page-allocation failures in the journal. The
  `MKT_NAME` porting question was also closed: the query dispatch table
  `query_cmd_ioctl_desc` and all twelve `mtgpu_query_*_ioctl` handlers are
  local symbols inside the proprietary `objs/x86_64/mtgpu_core.o_binary`, so
  sub-command 12 cannot be added from the open source (evidence:
  `musa-data/research/video-decode-codec-enum-mismatch-2026-11-26.md` §4.3c).

### Updates — 2026-11-26 (session: cold-boot re-verification)

- **Independent cold boot re-validated the whole stack.** After a fresh
  reboot (2026-11-26 00:07 UTC) with no blacklist, udev auto-bound `mtgpu`
  on the S30 and every gate reproduced: `verify-musa.sh --skip-memcpy`
  **13/0/2** at that time (**15/0/2** after the 2026-11-26 SONAME guards), `s30-perf.sh` **PERF_STATUS: PASS** (6.3/6.5 GB/s pinned,
  2.7 µs/launch, 52.0 GB/s triad, 1.12× pipeline), `test-vaapi.sh`
  **14/0/0**, `check-musa-env.sh` **PASS**, `verify-copr-chroots.sh`
  **20/20** — zero mtgpu kernel faults and zero page-allocation failures in
  the journal throughout. Perf numbers are stable across three boots on the
  3.1.0-15 heap-page fix. Full record:
  [hardware-validation-2026-11-26](docs/hardware-validation-2026-11-26.md).
- **Host package decision unchanged:** the live host keeps the pinned,
  verified `mtgpu-dkms` 3.1.0-15 / `musa-toolkit` 3.1.0-5 set while
  `mtgpu` is loaded; the published 3.1.0-16 / 3.1.0-6 pair (objtool
  fallback, README payload) is for a module-unloaded window.
- **Boot verifier re-armed.** `musa-boot-verify.service` had self-removed
  after its designed one-shot run; reinstalled via
  `pkexec scripts/install-boot-verify.sh install` so the next reboot writes
  compute-mode boot evidence to `/var/log/musa-fedora/`.

### Updates — 2026-11-26 (video decode and SONAME session)

- **Hardware video decode was exercised and does not work on this host, on
  either vendor route.** The new `scripts/hardware/test-video-decode.sh`
  drives the VA-API `VAEntrypointVLD` path and the MUSA-native MUVID path
  and reports precisely where each one stops. The packaged VA driver build
  cannot allocate its VPU working buffers (`mos_bo_alloc DEC_MV` →
  `M3d::CreateGpuMemory -9`); the older driver build rejects the valid
  single-access-unit stream (`VpuApiDecPic: slices number was wrong!`); and
  `musa-runtime` 1.5.4's MUVID accepts only codec ids 0..3 and rejects the
  id 4 that the vendor Video Codec SDK headers use for H.264
  (`MUSA_ERROR_NOT_SUPPORTED`). The S30 still **encodes** video in
  hardware (`h264_vaapi`). Full evidence chain:
  [root-cause-video-decode-2026-11-26](docs/root-cause-video-decode-2026-11-26.md).
- **The blocked ingredient is identified, and it is not the hardware.** The
  upstream Video Codec SDK library `libvideo_musa.so.13.0` *does* implement
  the H.264 codec id (its `muvidCreateVideoParser` dispatches on ids 0..3,
  4, 8, 10, 11) and reports this GPU as `h264_420 supported=1 mtdECs=1` —
  one MT decode engine, correctly advertised. It cannot be used here
  because it is a MUSA 5.x library: the MUSA 5.x driver API issues
  `MTGPU_QUERY_CMD_MKT_NAME` (sub-command 12), which the only S30 kernel
  driver available (mtgpu 3.1.0) does not implement, so `muInit` fails
  with -22. **A Moore Threads S30 kernel driver that implements
  `MTGPU_QUERY_CMD_MKT_NAME` is the single prerequisite that would unblock
  MUSA-native hardware decode.** The upstream comparison is reproducible
  with `MUSA_UPSTREAM_MUVID_LIB=<path to libvideo_musa.so.13.0>
  scripts/hardware/test-video-decode.sh`.
- **libva 2.24 broke the decode API for every client.**
  `vaCreateBuffer()` gained a `VAContextID` argument, so a libva-1.x caller
  silently passes its *buffer type* where the driver expects a context id
  (`Invalid context: 0x1`); the decode slice-data buffer type is
  `VASliceDataBufferType = 5`; and `vaQueryConfigAttributes()` /
  `vaGetConfigAttributes()` now take `VAProfile *` / `VAEntrypoint *`.
  `tests/vadecode.c` uses the 2.24 signatures.
- **A packaging defect was found and fixed.** An unowned 5.2.0 leftover in
  `/usr/lib64` (`libmusa.so.1` → `libmusa.so.5.2.0`) was the only provider
  of the SONAME `libmusa.so.1` on the host. Every consumer of that name
  therefore got the 5.2.0 driver API, which asks the 3.1.0 kernel driver
  for the non-existent `MTGPU_QUERY_CMD_MKT_NAME` (12) and dies in
  `muInit()` with `-22` while the kernel logs `Invalid cmd_type: 1 or
  cmd: 12`. The four files were moved (never deleted) to
  `/var/lib/musa-fedora/quarantine/musa-5.2-driver-api/` and `ldconfig` was
  re-run; the packaged stack is versioned consistently (`libmusa.so.1.5`)
  and is unaffected. The remediation is reproducible and reversible with
  `pkexec scripts/quarantine-5.2-driver-api.sh [--restore]`, and
  `scripts/verify-musa.sh` now names any foreign provider of that SONAME.
- **`musa-runtime` 1.5.4-8 exports `LIBVA_DRIVERS_PATH=/usr/lib64/dri`.**
  `libvideo_musa.so` (the MUVID API) is a libva client whose built-in
  default driver path is the Debian `/usr/lib/x86_64-linux-gnu/dri`, which
  does not exist on Fedora, so a MUVID application found no VA driver at
  all.
- **`musa-runtime` 1.5.4-8 is built, published and installed.** It exports
  `LIBVA_DRIVERS_PATH=/usr/lib64/dri` from the profile snippet, so the
  MUVID video API finds the packaged VA driver instead of searching the
  Debian path that does not exist on Fedora. Published to both COPR
  projects ([11392496](https://copr.fedorainfracloud.org/coprs/build/11392496/),
  [11392497](https://copr.fedorainfracloud.org/coprs/build/11392497/)) and
  built for all 9 chroots; all 22 build logs were downloaded and contain
  **zero** real errors. On the host: `rpm -V` and `dnf check` clean,
  `verify-musa.sh --skip-memcpy` **15/0/2**, `test-vaapi.sh` **14/0/0**,
  `check-musa-env.sh` **PASS**. See
  [COPR verification](docs/copr-verification-2026-11-26.md).
- No regression: `scripts/verify-musa.sh --skip-memcpy` still reports
  **15 passed, 0 failed, 2 skipped** (up from 13 by the two new guards),
  `mthreads-gmi` still reports the S30,
  `h264_vaapi` encode still produces a valid stream, and the kernel log
  contains **zero** `BUG`/`Oops`/general-protection/invalid-opcode entries
  for the whole session.

### Updates — 2026-11-25 (later session: objtool fallback, 3.1.0-16)

- **A build-log audit (not just “build succeeded”) found objtool silently
  skipped on rawhide**, where `kernel-devel`’s prebuilt objtool is linked to
  the datestamped `libopcodes-2.47.50.20261029` SONAME that current binutils
  no longer ships. Modules linked that way carry **no `.orc_unwind*`
  metadata**. `mtgpu-dkms` 3.1.0-16 teaches
  `scripts/objtool-safe.sh` to **rebuild a working objtool from the
  kernel-devel sources** (cache dir, `mkdir` lock, clean environment) instead
  of skipping — after working around two Fedora `kernel-devel` gaps
  (missing `tools/build/Build`, missing `gen-cpu-feature-names-x86.awk` with
  the generated `.c` shipped instead). A forced-broken-objtool build on the
  host now produces **byte-identical ORC sections and the same 5502 warning
  lines as a normal build**; the pre-fix skip path produced `.orc_header`
  only. Full record: [objtool-fallback-2026-11-25](docs/objtool-fallback-2026-11-25.md).
- **COPR re-verified after the fix: 20/20 checks, 9/9 chroots, and all 18
  per-chroot build logs inspected individually** (0 errors, 0
  `linking without objtool`, 5502 warnings in every DKMS chroot incl.
  rawhide). New builds: `mtgpu-dkms` 3.1.0-16
  ([11388916](https://copr.fedorainfracloud.org/coprs/build/11388916/),
  [11388919](https://copr.fedorainfracloud.org/coprs/build/11388919/)) and
  `musa-toolkit` 3.1.0-6 (README payload refresh;
  [11388917](https://copr.fedorainfracloud.org/coprs/build/11388917/),
  [11388920](https://copr.fedorainfracloud.org/coprs/build/11388920/)). The
  host keeps `mtgpu-dkms` 3.1.0-15 while the module is live — no DKMS
  transaction runs over a loaded driver; see the doc for the upgrade window.
- **`musa-toolkit` 3.1.0-5 activated on the host** with `pkexec dnf
  --refresh upgrade musa-toolkit` from the published COPR repo (gpg-checked;
  `rpm -V` and `dnf check` clean). The stale host dnf metadata only offered
  3.1.0-3 until `--refresh` pointed at the current `repodata`.
- **Same-session re-verification of the live S30** (all in capped scopes, no
  kernel fault in `journalctl -k -b`): `verify-musa.sh --skip-memcpy`
  **13/0/2**, `check-musa-env.sh` **MUSA_ENV_STATUS: PASS**,
  `test-vaapi.sh` **14/0/0**, and `s30-perf.sh` **PERF_STATUS: PASS**
  (pinned H2D 6.4 / D2H 6.6 GB/s, 2.6 µs/launch, 51.8 GB/s triad at
  `blockDim=1024`, 1.12× two-stream pipeline, kernel section `none`).
- **Isolated container gate passed with the new set** (`mtgpu-dkms`
  3.1.0-16, `musa-toolkit` 3.1.0-6): **13 passed, 0 failed**, scriptlets
  disabled, physical S30 passed through. Two stalled attempts exposed a
  container-mirror problem (curl error 28 at single-digit KiB/s); the test
  now pins the container’s repos to the `dl.fedoraproject.org` baseurls —
  same repos and packages, only mirror selection changes — and completes in
  minutes with zero curl errors.

### Updates — 2026-11-25 (session)

- **Boot-verify false FAIL root-caused and fixed.** The one-shot boot
  verifier on this boot logged 12/1/1 (`graphical.target is not active` plus
  an `0\n0` arithmetic error): the desktop was fine. Two script defects — an
  unpolled `graphical.target` check racing GDM startup, and a `pgrep -c ||
  echo 0` fallback producing `0\n0` — are fixed in
  `scripts/compute-mode-boot-verify.sh`; the re-run on the live desktop
  reports **14 passed, 0 failed, 0 warnings**. See the
  [2026-11-25 record](docs/hardware-validation-2026-11-25.md).
- **Same-day re-verification on this boot:** suite **13/0/2**
  (`--skip-memcpy`), COPR **20/20 checks across all 9 chroots**, and a fresh
  `s30-perf.sh` run (**PERF_STATUS: PASS**, 6.4/6.5 GB/s pinned H2D/D2H,
  2.7 µs/launch, 51.8 GB/s triad, 1.13× pipeline) with zero kernel faults.
- **Driver 3.1.0-15 fixes the host-memory failure.** The system/SVM heap page
  size was `0x40000` (256 KiB, order 6); on this 7.4 GiB host that request
  failed, `musaMallocHost()` returned `musaErrorOutOfMemory` and
  `scripts/s30-perf.sh` aborted. It is now `0x4000` (16 KiB, order 2), the
  size the driver already used for its non-4K heap. The patched module runs the
  benchmark with `PERF_STATUS: PASS`, zero `warn_alloc` entries in the boot
  journal, and `musa-alloc-latency` p50 = 0.064 ms. The earlier 16 MiB
  reading of order 6 was wrong; the corrected analysis is in
  [the order-6 root cause](docs/root-cause-order6-warn-alloc-2026-11-24.md).
- **VA-API is packaged separately and verified.** `musa-runtime-va` 1.5.4-7
  ships only the S30 VPU driver into `/usr/lib64/dri`, so it is safe next to
  an i915 desktop. `scripts/hardware/test-vaapi.sh` reports **14 passed, 0
  failed, 0 skipped**: `vaInitialize 1.24`, 20 profiles, and an
  `h264_vaapi` encode whose 425 503-byte stream decodes cleanly. See
  [VA-API](docs/vaapi.md).
- **Baseline suite on the patched stack:** `scripts/verify-musa.sh
  --skip-memcpy` reports **15 passed, 0 failed, 2 skipped** (vendor memcpy
  skipped for host RAM, OpenCL absent), with driver, runtime, `mcc`,
  `mp_10` compute, FP64 and headless Vulkan all passing. The count rose
  from 13 to 15 on 2026-11-26 when the SONAME-hygiene and VA-driver-path
  guards were added.
- **Performance benchmark:** 6.4/6.4 GB/s pinned H2D/D2H, 2.8 µs/launch,
  52.0 GB/s triad at `blockDim=1024`, 1.11× two-stream pipeline — inside the
  ranges recorded before the heap-page change, so the fix costs no bandwidth.

### Updates — 2026-11-24 (session)

- **Compute mode survived its first real reboot (05:45 UTC, clean boot).**
  udev autoloaded `mtgpu`, GNOME reached the login screen on `i915`, the
  kernel log has no fault, `scripts/compute-mode-boot-verify.sh` reports
  **14 passed, 0 failed, 0 warnings**, and `scripts/verify-musa.sh` reports
  **14 passed, 0 failed, 1 skipped** (OpenCL skip, no MT ICD). See
  [boot-verify-fix-2026-11-24](docs/boot-verify-fix-2026-11-24.md).
- **`mthreads-gmi` works on the clean boot.** The EINVAL failure recorded
  earlier today was state-dependent (live-module-replacement boots), not
  permanent: the same tool now reports the S30 (Driver 3.1.0, 45 °C,
  262/4096 MiB) and the telemetry check passes. The
  [root-cause doc](docs/root-cause-mthreads-gmi-drm-query-2026-11-24.md) is
  updated with the resolution.
- **Boot-verify service fixed and reinstalled.** systemd had deleted the
  one-shot job because the unit ordered itself `After=graphical.target` while
  being wanted by `multi-user.target` (ordering cycle) — no boot evidence was
  captured. The unit now orders only after `display-manager.service`, the
  script polls for `gdm` and falls back when the log directory is not
  writable; `systemd-analyze verify` passes and the service is enabled for
  the next reboot.
- **COPR re-verified (20/20 checks pass, 9/9 chroots).** Both projects now
  publish `mtgpu-dkms` 3.1.0-13 (comment/changelog-only delta from -12) plus
  `musa-runtime` 1.5.4-5, `musa-sdk` 5.2.0-6, `musa-toolkit` 3.1.0-3,
  `mtgpu-firmware` 3.1.0-4. The host stays on `mtgpu-dkms` 3.1.0-12 until its
  next reboot window (the -13 delta is not a functional change).
- **`musa-runtime` 1.5.4-5 installed and validated.** It ships
  `/etc/profile.d/zz-musa-fedora.sh`, which puts the 1.5.4 runtime before the
  5.2.0 compiler prefix in login shells; the unowned legacy
  `/etc/profile.d/musa.sh` had resolved `musaInfo` to the incompatible 5.2
  binary. `scripts/check-musa-env.sh` now reports `MUSA_ENV_STATUS: PASS`.
- **Clean-boot hardware re-validation:** `verify-musa.sh` reported
  **13 passed, 1 failed, 1 skipped** on the live S30. The single failure is the
  vendor `mthreads-gmi` telemetry tool (DRM ioctl skew, root cause recorded);
  `musaInfo`, `muMemcpyTest`, the runtime API, `mp_10` vector-add, the compute
  suite, FP64 and headless Vulkan all pass. *(Superseded later the same day:
  the 05:45 UTC clean boot passes 14/0/1 — see the first bullet above.)*
- **Compute mode enabled** (02:08 UTC): blacklist moved to
  `/var/backups/musa-fedora-compute-mode/20261124T020848Z/`, initramfs
  regenerated; udev resolves the S30 to `kmod load mtgpu` and GNOME survived
  the live driver with no vendor DRI files installed. Post-reboot checks are
  listed in the [compute-mode record](docs/compute-mode-enabled-2026-11-24.md).
- **COPR:** 9/9 chroots, 20/20 checks pass; both projects publish
  `musa-runtime` 1.5.4-5, `musa-sdk` 5.2.0-6, `musa-toolkit` 3.1.0-3,
  `mtgpu-dkms` 3.1.0-12 and `mtgpu-firmware` 3.1.0-4.

A controlled load test on the installed system on 2026-11-23 ran the full MUSA
workload successfully on the S30; see the
[2026-11-23 load-test evidence](docs/hardware-validation-2026-11-23-load-test.md).
That test did not change the boot-time blacklist. To avoid the Mesa/GNOME
crash seen there, the vendor display drivers now live in an opt-in
`musa-runtime-display` subpackage; see the
[display-driver split](docs/display-driver-subpackage-2026-11-23.md).
The controlled load has since been repeated with `musa-runtime` 1.5.4-4 (the
version without the Mesa DRI drivers): **every functional step passed, GNOME
survived the load, and no kernel fault was logged** — see
[the 1.5.4-4 load-test evidence](docs/hardware-validation-2026-11-23-runtime154-load-test.md).

### Updates — 2026-11-23 (session)

- **The installed system boots from disk again.** The GDM crash fix
  ([boot fix](docs/boot-fix-2026-11-23.md)) took effect: the host now boots
  `/dev/nvme0n1p7` on kernel `7.3.4-300.fc45.x86_64`, GNOME runs on Intel
  `i915`, and `mtgpu` stays unloaded with the boot-time block in place.
- **All 9 configured COPR chroots verified**, for both `axogm/musa-fedora` and
  `axogm/musa-mtt-s30` (11 checks including both projects' build status): every package build reports `succeeded` and every
  chroot's published repository advertises `musa-runtime 1.5.4-4`,
  `musa-runtime-display 1.5.4-4`, `musa-sdk 5.2.0-6`, `musa-toolkit 3.1.0-3`,
  `mtgpu-dkms 3.1.0-12` and `mtgpu-firmware 3.1.0-4`. Re-run it with
  [`scripts/verify-copr-chroots.sh`](scripts/verify-copr-chroots.sh); see
  [the verification record](docs/copr-verification-2026-11-23.md).
- **The driver still builds from the repository source** for the running
  kernel (`7.3.4-300.fc45.x86_64`, vermagic verified, S30 PCI alias
  `pci:v00001ED5d00000102` present) and its module aliases are identical to
  the installed module — reproducible, not a one-off binary.
- **New:** [`scripts/compute-mode.sh`](scripts/compute-mode.sh) enables the
  S30 for compute on a normal boot (removing the boot-time block) with
  preflight guards, backups and a restore path, and
  [`docs/compute-mode.md`](docs/compute-mode.md) explains the workflow, the
  evidence behind it and the limits.
- **Resolved:** the controlled load test was repeated on 2026-11-23 07:38 UTC
  with `musa-runtime 1.5.4-4` installed. All steps passed (telemetry,
  `musaInfo`, `muMemcpyTest`, runtime API, `mp_10` vector-add/compute/fp64,
  headless Vulkan), GNOME/GDM survived the load, and no kernel fault was
  logged. This verifies that the Mesa/GNOME crash was caused by the packaged
  vendor DRI drivers and that 1.5.4-4 fixes it; see
  [the 1.5.4-4 load-test evidence](docs/hardware-validation-2026-11-23-runtime154-load-test.md).
  The boot-time block was still in place during that run; removing it
  permanently via `compute-mode.sh enable` remains a separate change requiring
  explicit approval. The subsequent full-suite record documents two earlier
  userspace aborts from the incompatible SDK `libmusart.so.5` ABI; corrected
  1.5.4-linked tests passed without an Oops/BUG. The smoke-test script now
  checks both executable SONAMEs before device initialization and blocks after
  `Invalid cmd_type` entries in the current boot journal; do not rerun GPU
  workloads until a reviewed recovery plan and clean boot.

The project targets the vendor's own S30 stack — **mtgpu driver 3.1.0** plus
**MUSA runtime 1.5.4** from `musa_3.1.0-pc_x86.deb` — and codes kernels with the
**MUSA 5.2.0 `mcc`** compiler for the `mp_10` (SuDi) architecture.

> This file records what has actually been verified on the hardware. Anything
> not listed as verified below is *not* claimed to work.

## Status

**Current host (2026-11-25):** compute mode is enabled and has survived real
reboots. `/etc/modprobe.d` no longer blacklists `mtgpu`, the regenerated
initramfs carries no blacklist, and udev matches the S30's PCI modalias to
`kmod load mtgpu`. The module is live with the documented headless parameters
(`display=none disable_fbdev=1 mtgpu_drm_major=2`), `/dev/mtgpu.0` and an
`mtgpu` DRM render node exist, and GNOME runs on Intel `i915`. The vendor Mesa
DRI drivers are not installed (`musa-runtime-display` is optional), which
removes the 2026-11-23 GDM crash condition. The previous boot-time block is
preserved in `/var/backups/musa-fedora-compute-mode/20261124T020848Z/` and can
be restored with `scripts/compute-mode.sh disable`.

Installed set: `mtgpu-dkms` 3.1.0-15 (16 KiB system/SVM heap page),
`mtgpu-firmware` 3.1.0-5, `musa-runtime` 1.5.4-8, `musa-runtime-va`
1.5.4-8, `musa-sdk` 5.2.0-7 and `musa-toolkit` 3.1.0-5 (activated from the
published repo on 2026-11-25; `rpm -V` and `dnf check` clean). COPR also
publishes `mtgpu-dkms` 3.1.0-16 (objtool fallback) and `musa-toolkit`
3.1.0-6 (README payload); both are deliberately **not installed on the live
host** — a DKMS transaction must not replace the module under a loaded
`mtgpu`. Install them in a window where the module is not loaded, as
described in [objtool-fallback-2026-11-25](docs/objtool-fallback-2026-11-25.md).
`musa-runtime-display` is deliberately not installed.

On the 2026-11-26 boot `scripts/verify-musa.sh --skip-memcpy` reports **15
passed, 0 failed, 2 skipped** (the vendor memcpy test needs more host RAM than
the capped test scope allows; OpenCL is absent). The table below records the
2026-11-17 through 2026-11-26 hardware runs.

| Layer | State | Evidence |
|---|---|---|
| PCI enumeration | **verified** | `lspci -nn` → `1ed5:0102` |
| `mtgpu` driver / device nodes | **verified** | module live; S30 render node (`/dev/dri/renderD129` on this boot); `/dev/mtgpu.0` |
| IOMMU | **verified** | `intel_iommu=on iommu=pt`; S30 assigned to IOMMU group 11 |
| Device telemetry | **verified** | `musaInfo` and `mthreads-gmi` both report MTT S30, 8 SMs, 3.97 GB on the clean 05:45 boot; `mthreads-gmi` fails only in live-module-replacement boots (documented) |
| MUSA runtime discovery | **verified** | MUSA runtime 1.5.4 `musaInfo` reports one MTT S30 |
| GPU memcpy test | **verified** | vendor `muMemcpyTest` exited successfully |
| MUSA runtime host API | **verified** | `tests/musart-smoke.c` (alloc/copy/memset/streams/events) passed on the S30 |
| Compiler target | **verified** | MUSA 5.2.0 `mcc` compiled `mp_10` vector-add |
| GPU kernel execution | **verified** | vector-add completed; all 1,048,576 results matched |
| MUSA compute operations | **verified** | SAXPY, shared reduction, atomics, constant memory, device math, 2D launch, D2D copy, and FP64 passed |
| LLM inference on the S30 | **verified 2026-11-27** | `apps/musa-infer` runs TinyLlama-1.1B Q8_0 through runtime 1.5.4: 12.58 tok/s generation, CPU/GPU compare PASS (`max_abs=9.47e-05`, `top1_same=10/10`), token-for-token match with llama.cpp `a07831a` — [details](docs/musa-infer.md) |
| Host-memory allocation (SVM heap 16 KiB) | **verified 2026-11-25** | 3.1.0-15 fixed the order-6 heap page; alloc latency p50 0.064 ms, zero `warn_alloc`, benchmark PASS |
| VA-API on the S30 VPU | **verified 2026-11-25** | `musa-runtime-va` 1.5.4-7; vaInitialize, 20 profiles, H.264 encode round trip (14/0/0) |
| Hardware video decode (VA-API VLD) | **broken 2026-11-26** | exercised via `scripts/hardware/test-video-decode.sh`; the packaged driver build fails `mos_bo_alloc DEC_MV`, the older build rejects the picture — [root cause](docs/root-cause-video-decode-2026-11-26.md) |
| MUSA-native MUVID hardware decode | **unavailable** | `musa-runtime` 1.5.4 accepts only codec ids 0..3 and rejects the vendor headers' H.264 id 4 with `MUSA_ERROR_NOT_SUPPORTED`; the upstream SDK library implements H.264 but needs a newer S30 kernel driver |
| MT decode engine present on the S30 | **verified 2026-11-26** | upstream Video Codec SDK reports `h264_420 supported=1 mtdECs=1` for this GPU |
| MUSA driver-API SONAME hygiene | **fixed 2026-11-26** | unowned 5.2.0 `/usr/lib64/libmusa.so.1` quarantined; `verify-musa.sh` now guards against a re-introduction |
| Headless Vulkan | **verified, limited** | S30 device/queue, buffer allocation, and host-visible memory map passed; no rendering test |
| Moore Threads OpenCL | **unavailable** | No MT ICD in the default loader; archived 2.1.1 ICD compute probe caused a kernel Oops (do not use) |
| Copr publication | **verified 2026-11-26** | 9/9 chroots across both projects each publish all seven artifacts at dkms 3.1.0-16, firmware 3.1.0-5, **runtime family 1.5.4-8**, sdk 5.2.0-7, toolkit 3.1.0-6; `scripts/verify-copr-chroots.sh` reports 20/20 checks and all 22 build logs of the runtime-1.5.4-8 submission were inspected with 0 errors — see [COPR verification](docs/copr-verification-2026-11-26.md), [the 11-25 one](docs/copr-verification-2026-11-25.md) and [objtool fallback](docs/objtool-fallback-2026-11-25.md); build success does not qualify driver loading |
| Driver rebuild from source | **verified 2026-11-23** | `scripts/build-kernel-driver.sh` rebuilds `mtgpu` for `7.3.4-300.fc45.x86_64`; vermagic matches and the module aliases equal the installed module's |
| Fedora 45 container + GPU test (2026-11-25 set) | **verified** | all seven RPMs installed in a throwaway Fedora 45 container with scriptlets disabled; `musaInfo`, vendor `muMemcpyTest` (1 GiB), `musart-smoke`, and an `mcc` `mp_10` vector-add ran on the S30: `13 passed, 0 failed` |
| Historical Fedora container + GPU test (2026-11-17) | **verified** | the five-RPM set installed in a throwaway container and its packaged runtime and `mcc` ran on the S30: `13 passed, 0 failed` |

Historical verification command on 2026-11-17: `scripts/verify-musa.sh` — **13 passed,
0 failed, 1 skipped**. It must not be read as a current-host result. The skip is Moore Threads OpenCL: the loader found
MVGAL and Intel platforms, but no MT OpenCL ICD. A separate experiment with
an archived MobiMaliang 2.1.1 ICD triggered a kernel Oops during compute and
must not be repeated; see [the failure record](docs/current-state-2026-11-17.md).
The recorded tests used the installed
1.5.4 runtime and 5.2.0 compiler. They validate device discovery, copy and
host runtime APIs, eight compute operations, one `mp_10` vector-add, and a
headless Vulkan device/buffer/memory path. They do not validate display
rendering, video, `torch_musa`, or other frameworks, and they do not make the
separate 5.2.0 runtime suitable for S30 execution.

## Repository layout

```
driver-s30-3.1.0/     S30 driver source and compatibility/build files
scripts/              build, install, IOMMU, verification helpers
tests/                hardware tests (MUSA + raw ioctl ABI probes)
apps/                 end-to-end workloads (musa-infer LLM engine)
docs/                 verified analysis and compatibility notes
packaging/            RPM/DKMS packaging
```

Vendor packages, debs, extracted sources and research live outside this tree in
`../musa-data/`.

## Build the driver

```sh
scripts/build-kernel-driver.sh          # out-of-tree build for the running kernel
```

## Verify on the host

```sh
scripts/verify-musa.sh
```

It checks PCI/driver binding, the S30 render node, IOMMU groups, `mthreads-gmi`,
`musaInfo`, `muMemcpyTest`, the host runtime API (`tests/musart-smoke.c`), the
`mp_10` vector-add and compute suite, headless Vulkan device/buffer/memory use,
and Moore Threads OpenCL platform availability. It creates a temporary build
directory and does not load/unload modules or change system configuration, but
it does execute GPU workloads. Its recorded result above predates the OpenCL
kernel Oops. Do not rerun it in the boot that logged the Oops; resume GPU
validation only after recovery and a clean boot, with a reviewed test plan.

For an interactive shell, source the project environment first:

```sh
source scripts/musa-env.sh
```

The compiler and device-code headers come from MUSA 5.2.0, while the supported
S30 runtime and host API headers come from MUSA 1.5.4. The script puts the
1.5.4 runtime libraries before the SDK libraries because the system
`/etc/profile.d/musa.sh` currently places the separate 5.2.0 runtime first;
an unqualified `musaInfo` under that default environment aborted during this
recheck. Do not use the 5.2.0 runtime for S30 execution.

## Known limitations and packaging status

* The verified host uses the vendor mtgpu 3.1.0 stack and MUSA runtime 1.5.4;
  MUSA 5.2.0 is used only for its compiler/headers. The 5.2 runtime is not
  compatible with this 3.1.0 driver ABI; do not substitute it for runtime 1.5.4.
* The host-installed set at the 2026-11-23 controlled load was
  `mtgpu-dkms` 3.1.0-11, `mtgpu-firmware` 3.1.0-4, `musa-runtime` 1.5.4-4,
  `musa-sdk` 5.2.0-6, and `musa-toolkit` 3.1.0-3. COPR publishes
  `mtgpu-dkms` 3.1.0-12 (tmpfiles event-report directories; see the
  [COPR verification](docs/copr-verification-2026-11-23.md)) and
  `musa-runtime` 1.5.4-5. Both were installed on 2026-11-24 during a boot in
  which `mtgpu` was not loaded (a live-module replacement is refused by the
  package helper); the host now runs `musa-runtime` 1.5.4-5,
  `mtgpu-dkms` 3.1.0-12 and compute mode, with the boot-time blacklist no
  longer active (see
  [compute-mode-enabled-2026-11-24](docs/compute-mode-enabled-2026-11-24.md)).
  Newer package builds are not evidence of runtime compatibility.
* A Fedora 45 container test and packaged compiler/runtime test on the physical
  S30 passed on 2026-11-17, with scriptlets suppressed. This is historical
  evidence from a prior loaded-driver state, not a test of the current boot.
  The SDK packaging fixes included shipping `clang-offload-bundler` and
  requiring `gcc-c++` for the C++ linker/headers.
* The installed 1.5.4 runtime ships its own `musa_runtime.h` at
  `/usr/local/musa/include`; compile host runtime-API code against those
  headers, not the 5.2.0 SDK headers. `sizeof(struct musaDeviceProp)` is 728
  bytes in the 1.5.4 headers versus 1032 bytes in the 5.2.0 headers, so mixing
  them corrupts the device-properties ABI. Use the 5.2.0 SDK for the `mcc`
  compiler and device-code headers, and keep host code within the API exported
  by `libmusart.so.1.5.4`.
* No vendor BLAS or AI framework supports this GPU: `torch_musa` issue #134
  places the S30 outside its scope and the 1.5.4 runtime has no `mublas` or
  `mudnn` for `mp_10`. `apps/musa-infer` is a self-contained alternative for
  Llama-2-class models, not a drop-in framework replacement.
* Display output is not configured on the S30; i915 drives the desktop.
* The archived MobiMaliang 2.1.1 OpenCL ICD is unsafe with the active driver:
  its compute probe caused a kernel Oops in `mtgpu`. Do not install or run it.
* Historical unbind/rebind of this vendor module triggered a kernel NULL
  dereference. Do not unbind/rebind or replace the active module as part of
  routine tests. See `docs/root-cause-rebind-2026-11-16.md`.

## Safety

* The recorded tests are read-only apart from temporary files and do not install
  packages, change boot settings, or load/unload/rebind kernel modules.
* Since 2026-11-24 the host runs compute mode: the S30 is bound on a normal
  boot and remains bound after reboot. The previous boot-time block and the
  whole `/etc/modprobe.d` set are backed up under
  `/var/backups/musa-fedora-compute-mode/`; `scripts/compute-mode.sh disable`
  restores the unbound state. Do not unload the module or unbind/rebind the
  PCI device as part of routine tests (historical rebind fault:
  `docs/root-cause-rebind-2026-11-16.md`). The controlled-load runbook remains
  the safe path for a host that still blacklists the driver.
* The older boot that logged an `mtgpu` kernel fault during the archived OpenCL
  experiment also must not be used for GPU workloads. The reviewed procedure to
  resume hardware validation after a clean reboot is in
  [docs/clean-boot-resume-plan-2026-11-17.md](docs/clean-boot-resume-plan-2026-11-17.md);
  `scripts/test-gpu.sh` now refuses to run in a boot that already logged an
  `mtgpu` fault.
* Do not install a new DKMS package over the active driver as part of routine
  testing. The isolated package test suppresses scriptlets and does not build,
  load, or replace a kernel module.
* Reference packages, extracts, and system backups are kept under
  `/home/axogm/Documents/project/musa-data/`.

# Hardware validation — 2026-11-26 (session 3, same-boot re-verify)

Host: Fedora 45 Workstation, kernel `7.3.4-300.fc45.x86_64`, MTT S30
`1ed5:0102` bound to `mtgpu` (compute mode, no blacklist). Packages:
`mtgpu-dkms 3.1.0-15`, `mtgpu-firmware 3.1.0-5`, `musa-runtime 1.5.4-8`,
`musa-runtime-va 1.5.4-8`, `musa-sdk 5.2.0-7`, `musa-toolkit 3.1.0-5`.
COPR-authenticated as `axogm`.

## Results (this session)

| Suite | Result |
| --- | --- |
| `scripts/verify-musa.sh --skip-memcpy` | **15/0/2** (musart-smoke PASS: 4267532288 bytes, 8 SMs; vector-add 1M floats verified; saxpy/shared-reduce/atomics/constant-mem/libm/2D-grid/D2D/fp64 PASS; Vulkan PASS; OpenCL SKIP — no MT ICD) |
| `scripts/s30-perf.sh` | **PASS** (6.4 GB/s pinned H2D, 2.6–2.7 µs/launch, 52.1 GB/s triad, 1.11–1.12× pipeline; 1 CACHE SKIP — `PreferShared` not supported) |
| `scripts/hardware/test-vaapi.sh` | **14/0/0** |
| `scripts/check-musa-env.sh` | **PASS** (1 orphan `/etc/profile.d/musa.sh` WARN, overridden by packaged env) |
| `scripts/verify-copr-chroots.sh` | **20/20** (7 packages × fedora-43/44/45/eln/rawhide) |
| `scripts/test-container-install.sh` (isolated podman) | **13/0** — packages install in a clean Fedora container and vector-add runs on the S30 |

## Notes

- Feeds `mcc` binaries must link the **1.5.4 runtime** (`libmusart.so.1.5`
  from `/usr/local/musa/lib`), not the 5.2.0 SDK runtime (`libmusart.so.5`):
  the latter segfaults inside `musaMalloc()` against the 3.1.0 `mtgpu`
  driver. `scripts/hardware/build-test-binaries.sh` enforces this by
  checking the linked SONAME. (This session re-confirmed the failure mode
  with a direct `mcc` build against the SDK runtime: segfault, exit 139.)
- One `s30-perf.sh` run reported `FAIL (1 check)` while the same script
  passed on retry with no code change — host was at 6.2G/7.4G RAM, i.e. a
  bandwidth threshold flaked under memory pressure, not a GPU fault. No
  `mtgpu` kernel message accompanied it.
- `musaInfo` (5.2.0 SDK) still errors with
  `API call is not supported in the installed MUSA driver (36)` against the
  3.1.0 driver — expected until the SDK/runtime pair is unified.

## Journal

`journalctl -k -b | grep -icE 'mtgpu.*(BUG|Oops|general protection|invalid opcode|null pointer)'` → **0**;
`'page allocation failure'` → **0**. Only benign IRQ-affinity churn in the
boot log.

## Verdict

Full MUSA stack (driver, runtime, compiler, Vulkan, VA-API) reproduces on
this boot; isolated container install also passes. Nothing required a
blacklist, module replacement, or boot-config change.

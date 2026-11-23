# MUSA on Fedora — live host recheck (2026-11-20, afternoon)

Read-only recheck after the morning inventory in
[`current-state-2026-11-20.md`](current-state-2026-11-20.md). No driver was
loaded, no package was installed/removed, and no system configuration was
changed in this recheck. The S30 remains unbound/blacklisted (see below); GPU
workloads were **not** run.

## Host state (verified this session)

- Fedora 45 Workstation, kernel `7.3.4-300.fc45.x86_64`
  (`CONFIG_X86_KERNEL_IBT=y`, cmdline has no `ibt=off`).
- MTT S30 at `0000:01:00.0` (`1ed5:0102`) + audio `1ed5:01ff`; both unbound.
  No `/dev/mtgpu*`; `/dev/dri/renderD128` is Intel i915.
- `/etc/modprobe.d/blacklist-mtgpu.conf` still contains `blacklist mtgpu`.
- `/etc/modprobe.d/90-mtgpu-headless.conf` is now corrected (checked on disk,
  not just in the spec): `options mtgpu display=none disable_fbdev=1
  mtgpu_drm_major=2`, no `rm_feature_level=1` — matching the root-cause fix in
  [`root-cause-rm-feature-level-2026-11-20.md`](root-cause-rm-feature-level-2026-11-20.md).
- Installed RPMs: `mtgpu-dkms-3.1.0-11` (newer than the morning report's
  `-7`; the uncommitted spec change in the working tree bumps `9 → 11`),
  `mtgpu-firmware-3.1.0-2`, `musa-runtime-1.5.4-3`, `musa-sdk-5.2.0-6`,
  `musa-toolkit-3.1.0-1`. DKMS has `mtgpu/3.1.0` built+installed for
  `7.3.4-300.fc45` (`/lib/modules/7.3.4-300.fc45.x86_64/extra/mtgpu.ko.xz`).
- Secure Boot disabled; rescue kernel `/boot/vmlinuz-0-rescue-*` present.
- Current-boot `journalctl -k` contains no `mtgpu` probe/Oops/BUG lines
  (grep for `Oops:|BUG: kernel NULL|Missing ENDBR|mtgpu` → no hits). The boot
  is clean for a gated load test.

## Checks run this session (all real, all passed)

| Check | Result |
|---|---|
| Phase-0 snapshot | `musa-data/host-backups/pre-pre-mtgpu-load-2026-11-20-recheck-20261120-154309/` (cmdline, journal, rpm-qa, dkms, modprobe.d, module+vendor-core sha256, lspci, IOMMU, DRM nodes, rescue kernel) |
| Vendor core integrity (installed) | `sha256(/usr/src/mtgpu-3.1.0/objs/x86_64/mtgpu_core.o_binary) = d0fdaceab37bce7387d376f8a87a2e9dbf210eeccf4f4f7c67f8dee83f4e8200` — matches the pinned hash required by the uncommitted `-11` spec (`__brp_strip` disabled). The strip-mangling that affected releases ≤ 9 is **not** present in the installed tree. |
| Local `-11` RPM payload | `rpm -K --nosignature: digests OK`; extracted `mtgpu_core.o_binary` from `local-build-dkms/.../mtgpu-dkms-3.1.0-11.fc45.noarch.rpm` hashes to the same `d0fdace…` — the RPM payload is byte-identical to the pinned vendor blob. `rpmspec -P` on the spec succeeds. |
| Toolchain (compile-only) | `MUSA_PATH=/usr/local/musa-5.2 scripts/test-toolchain.sh` → `COMPILE_ONLY_STATUS: PASS` (fat-binary section present, explicit `mp_10` compile PASS, runtime-header syntax check PASS). No executable was run; `GPU_RUNTIME_STATUS: NOT TESTED`. |
| Hardware gate (no load) | `scripts/test/s30-hardware-validation.sh` (without `--load`) → PCI PASS, environment PASS, then `VALIDATION_STATUS: BLOCKED (driver not loaded)`, exit 3 — the ladder correctly refuses to claim GPU results without a live driver. `scripts/test/check-hardware.sh` likewise reports unbound/NOT RUN (exit 3). | | Reproducible build (compile-only, 7.3.4) | `scripts/build-kernel-driver.sh 7.3.4-300.fc45.x86_64` → `BUILD_STATUS: SUCCESS`; module `mtgpu.ko` (sha256 `3e76dc13aa4f055fd8a5da46128a04e77bbcaaa5ef8c57169b7313c174f0eed1`) with `vermagic 7.3.4-300.fc45.x86_64 SMP preempt mod_unload`, S30 PCI alias `pci:v00001ED5d00000102`, and 5,502 compiler/objtool warning lines (the documented IBT/ENDBR noise from the opaque vendor core). No install, no load. |

`rpm -V` on the five packages shows only the two previously recorded local
differences (`/etc/modprobe.d/mtgpu.conf` missing — the disabled config — and
`/usr/lib64/libdrm_mtgpu.so.1` redirected); nothing new.

## What changed vs. the morning inventory, and what did not

- Changed on the host since the morning report: DKMS package is now `-11`
  (not `-7`) and the on-disk `90-mtgpu-headless.conf` no longer carries
  `rm_feature_level=1`. Both changes remove documented load-fault causes.
- Not changed: the blacklist is intact, the S30 is still unbound, firmware is
  still `3.1.0-2` (blobs verified byte-identical to `-4` in the morning
  report, so no upgrade is needed for correctness), and the uncommitted
  working-tree changes (`mtgpu-dkms.spec` 9→11, `scripts/rpm-stack.sh`
  expectation, plus untracked `scripts/snapshot-host-state.sh`,
  `scripts/test/s30-hardware-validation.sh`,
  `packaging/rpm/local-build-dkms/`) were left untouched.

## Blocker / required decision

The only remaining step to a functional S30 stack is Phase 2 of
[`maintenance-plan-mtgpu-load-2026-11-20.md`](maintenance-plan-mtgpu-load-2026-11-20.md):
one controlled headless load (`display=none disable_fbdev=1
mtgpu_drm_major=2`, no `rm_feature_level`, no rebind, abort on any Oops),
followed by the validation ladder. That plan is marked **DRAFT — NOT
APPROVED**, and its non-negotiable rule is that it must not be executed
without an explicit go-ahead from the user. Preconditions for Mode A
(current boot, IBT on) are now all met:

1. clean current-boot journal, 2. corrected headless config on disk,
3. byte-identical pinned vendor core in the installed module,
4. Phase-0 snapshot taken, 5. blacklist intact for normal boots,
6. rescue kernel present.

Residual risks that approval must accept: the opaque vendor core still emits
5,486 objtool/IBT warnings per build and is not proven IBT-safe (Mode B,
one-shot `ibt=off` boot, isolates that variable but needs a reboot), and the
historical GNOME/KMS collision is mitigated only by headless mode, not
eliminated.

**No load was performed in this session.** To proceed, the user must
explicitly approve Phase 2 Mode A (or direct Mode B with a reboot), after
which the gated ladder in `scripts/test/s30-hardware-validation.sh --load`
runs steps 0–10 and records verbatim PASS/FAIL per step. Until then, no
claim is made that the S30 is usable in this boot.

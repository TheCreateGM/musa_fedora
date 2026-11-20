# Maintenance plan draft — mtgpu 3.1.0 load test on the S30 (2026-11-20)

> **STATUS: DRAFT — NOT APPROVED. Do not execute this plan without an
> explicit go-ahead from the user.** It exists to satisfy the "required next
> step" in [`current-state-2026-11-20.md`](current-state-2026-11-20.md):
> a reviewed maintenance/recovery plan that addresses the documented
> GNOME/KMS conflict, IBT/objtool warnings, opaque vendor-core faults, and
> the historical rebind failure before any hardware attempt.

## Findings this plan builds on

1. **`rm_feature_level=1` is the confirmed load-fault root cause**
   ([root-cause-rm-feature-level-2026-11-20.md](root-cause-rm-feature-level-2026-11-20.md)):
   it activates an incomplete vendor Resource Manager path in
   `mtgpu_core.o_binary` and NULL-dereferences in `gpu_phys_heap_alloc_mem`.
   The verified PVR memory model needs `rm_feature_level=0` (the default).
   *Host fact:* the installed release-7 config
   `/etc/modprobe.d/90-mtgpu-headless.conf` still carries `rm_feature_level=1`;
   it is currently inert only because `/etc/modprobe.d/blacklist-mtgpu.conf`
   blocks the module. It must be neutralized before any load attempt.
2. **The 2026-11-17 verified session loaded the module live while GNOME ran**
   — after headless mode (`display=none disable_fbdev=1 mtgpu_drm_major=2`)
   stopped the card1/KMS collision. The later faults were (a) the OpenCL-ICD
   compute probe Oops (`PVRSRVRGXFlushComputeDataKM`) and (b) the
   `rm_feature_level=1` load fault. Neither was a display collision.
3. **IBT/objtool warnings (5,486 lines per build)** include missing-ENDBR
   relocations (e.g. `rmapi_alloc`) and RETPOLINE naked returns. The 2026-11-13
   boot faulted on `Missing ENDBR: rmapi_alloc` with IBT active. The module is
   *not* proven safe under IBT; a test boot with `ibt=off` isolates that
   variable from the load test.
4. **PCI unbind/rebind after a fault is unsafe** (documented rebind NULL
   dereference). The plan never unbinds/rebinds, and performs at most one
   load per boot.
5. **Driver 5.2.0 runtime is not ABI-compatible with driver 3.1.0.** Only
   runtime 1.5.4 may be exercised.

## Non-negotiable safety rules

- The Intel/i915 display and GNOME session must keep working; the S30 must not
  drive output.
- Default boot entries are never modified. The blacklist stays in place for
  every boot the plan does not explicitly run.
- No module unload, no `rmmod`, no PCI rebind, no second module load in the
  same boot.
- Any kernel Oops/BUG in `journalctl -k` immediately ends testing for that
  boot: no further GPU work, capture logs, plan recovery.
- `scripts/test-gpu.sh` gate stays in force: it refuses to run in a boot that
  logged an `mtgpu` fault.
- `rpm -V` and package digests are checked after any package change.

## Phase 0 — Pre-flight snapshot (read-only, safe any time)

Record into
`/home/axogm/Documents/project/musa-data/host-backups/pre-mtgpu-load-<timestamp>/`:

- `journalctl -k -b` full export, `journalctl -b --since boot` summary
- `rpm -qa`, `dkms status`, `cat /proc/cmdline`, `cat /etc/modprobe.d/*.conf`
- `sha256sum` of the on-disk mtgpu module for the running kernel
- `lspci -nnk -s 01:00.0`, IOMMU group listing, `/etc/selinux/config`
- Confirm the rescue kernel exists (`/boot/vmlinuz-0-rescue-*`) and record the
  GRUB default entry; do not change it.

Exit criteria: snapshot complete; both 2026-11-17 backups still present.

## Phase 1 — Host config/package reconciliation (elevated, reversible)

Two approved-change options, in preference order:

- **Option 1 (recommended):** install the verified Copr release set on the
  host with `pkexec dnf upgrade ...`: `mtgpu-dkms 3.1.0-9` (removes the
  unsupported IBT claim, keeps `rm_feature_level` out of
  `90-mtgpu-headless.conf`), `mtgpu-firmware 3.1.0-4` (usr-merge path; blobs
  byte-identical to installed — verified 2026-11-20), `musa-toolkit 3.1.0-3`,
  keep `musa-runtime 1.5.4-3` and `musa-sdk 5.2.0-6`. DKMS scriptlets build
  the module for kernel 7.3.4 as they did for 7.3.3.
- **Option 2 (minimal change):** leave packages as installed; edit
  `/etc/modprobe.d/90-mtgpu-headless.conf` to drop `rm_feature_level=1`
  (backup the file to the Phase 0 snapshot first).

Either way, the blacklist file is untouched. After the change, run
`scripts/rpm-stack.sh` (Option 1) or diff the config against the snapshot
(Option 2) and record the result. Revert = reinstall release 7 firmware/SDK
packages or restore the backed-up config file.

## Phase 2 — Load test (one controlled attempt, gated)

Choose the boot mode by variable isolation:

- **Mode A — current GNOME boot, headless load (closest to verified 11-17
  state):** use the existing
  `scripts/load-driver-manual.sh` (already updated to
  `display=none disable_fbdev=1 mtgpu_drm_major=2`, no
  `rm_feature_level=1`). Accepts the documented residual risk that GNOME runs
  during the load; historically survived with these args.
- **Mode B — IBT isolated:** one-time boot with `ibt=off` added via
  `grub-editenv` one-shot (`grub2-reboot` with a command line, or an edited
  one-time entry), then load as Mode A. Requires Secure Boot to remain off
  (verified disabled). Tests the ENDBR-warning risk explicitly.

Not permitted: `rm_feature_level=1` in any load, second load attempts after a
failed load, any unbind/rebind.

### Test sequence after a successful load (stop at first anomaly)

1. `journalctl -k` — confirm no faults; record module init messages.
2. `/dev/mtgpu.0` and a new `/dev/dri/renderD*` node exist; GNOME still active.
3. `mthreads-gmi -q` — driver version, device identity, memory.
4. MUSA 1.5.4 `musaInfo` (project env: `source scripts/musa-env.sh`).
5. Vendor `muMemcpyTest`.
6. `tests/musart-smoke` (host runtime API).
7. `mcc` rebuild of `tests/musa-vector-add.mu` (`mp_10`) and run.
8. `tests/musa-compute.mu` compute suite.
9. Headless Vulkan `tests/vulkan-enumerate`.
10. Record every result verbatim in a new dated status doc. Do **not** claim
    display/video/OpenCL/framework support.

If a fault appears at any step: stop, `journalctl -k > evidence file`, leave
the module loaded (do not unload), and go to Phase 3.

## Phase 3 — Recovery paths (in escalation order)

1. **Mutter/GNOME crash only:** the shell restarts; switch VT (Ctrl+Alt+F3)
   or use the running agent terminal; export `journalctl -b` evidence;
   blacklist already protects future boots — no further action.
2. **Module load failed but no Oops:** system continues normally; record
   failure; no unload/rebind this boot.
3. **Kernel Oops during load or test:** capture full journal; do not start new
   GPU work; if the desktop is unusable, reboot from a VT or from the agent
   (`pkexec systemctl reboot`). Note `kernel.sysrq=16` (sync-only) limits
   magic SysRq; a clean reboot command is the primary path.
4. **Unbootable system (not expected — display is i915, blacklist intact for
   normal boots):** boot the rescue kernel from GRUB; `dnf history rollback`
   or restore Phase 0 backups (`/etc/modprobe.d`, firmware). The rescue
   kernel's existence is verified in Phase 0.
5. After any recovery, verify: i915 display works, `lspci -k` shows S30
   unbound, blacklist file unchanged, then write the incident into a new dated
   status doc.

## Acceptance criteria to declare the maintenance successful

- `mtgpu` loaded once with headless args and no `rm_feature_level=1`, zero
  kernel faults in `journalctl -k` for the whole boot.
- All Phase 2 steps 1–9 completed with recorded output.
- GNOME/i915 unaffected; post-boot blacklist and configs unchanged.
- New dated status doc records exact commands and outputs; README/PROJECT_PLAN
  tables updated; both repos pushed.

## Explicit non-goals

- No display output from the S30, no Vulkan rendering, no OpenCL (the 2.1.1
  ICD remains quarantined), no `torch_musa`, no MUSA 5.2.0 runtime use.
- No IBT-safe-module claim; if Mode B is not run, IBT risk stays documented
  and unresolved.

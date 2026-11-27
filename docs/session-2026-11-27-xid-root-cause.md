# Session 2026-11-27 (session 6): live re-verify + XID root cause proven

Read-only live re-verification of the validated MTT S30 MUSA stack, plus the
definitive root-cause proof for the one open diagnostic item (the vendor XID
record file).  No host kernel, bootloader, initramfs, firmware, driver binary,
or system-critical component was modified.

## Live re-verification (real MTT S30, this boot)

| Check | Result |
|---|---|
| `scripts/check-musa-env.sh` | **PASS** (musaInfo→1.5.4 runtime, mcc→5.2.0, LD_LIBRARY_PATH ordered) |
| `scripts/verify-copr-chroots.sh` | **24 passed, 0 failed** — musa-fedora + musa-mtt-s30, 8 packages × 5 chroots (f43/f44/f45/eln/rawhide x86_64), every latest build `succeeded` and every chroot repo advertises the expected RPMs |
| `scripts/verify-musa.sh` | **16 passed, 0 failed, 1 skipped** (OpenCL ICD absent — expected) |

`verify-musa.sh` on-device evidence this boot: `mthreads-gmi` reports the MTT
S30 (driver 3.1.0); `muMemcpyTest` PASS; `musart-smoke` PASS (`name=MTT S30`,
`totalGlobalMem=4267532288`, `multiProcessorCount=8`, async round-trip PASS);
`mcc` `mp_10` vector-add **1,048,576 float adds verified on S30**; compute
features saxpy/shared-reduce/atomics/constant-mem/device-math/2D-grid/D2D all
PASS; **fp64 PASS** (abs err 5.55e-17); Vulkan device+buffer+host-map PASS
(vendor `0x1ed5` id `0x0102`).  Installed stack: `mtgpu-dkms 3.1.0-16`,
`mtgpu-firmware 3.1.0-5`, `musa-runtime 1.5.4-8`, `musa-runtime-va 1.5.4-8`,
`musa-sdk 5.2.0-7`, `musa-toolkit 3.1.0-6`, `musa-cuda-compat 1.0.0-2`.

## XID record-file root cause — PROVEN

The vendor XID on-disk record file has never been written (every boot logs
`Failed to create directory: /var/log/mtgpu, ret = -2` … `xid_record file
create fd failed`).  Prior notes attributed this to the module loading in the
initramfs / a missing parent directory.  **Both theories are wrong.**  Direct
kernel probes establish the true cause:

1. **`mtgpu_drv_mkdir` is find-or-fail.**  It calls
   `start_creating_path(AT_FDCWD, path, LOOKUP_DIRECTORY)` — no
   `LOOKUP_CREATE`.  Per `fs/namei.c`, a missing target without `LOOKUP_CREATE`
   returns `-ENOENT` and creates nothing.  `tests/xid-sem-probe` measures this
   exactly (target-absent → `-ENOENT`, nothing created).

2. **The mkdir runs in a kworker that sees the initramfs root, not the real
   root.**  `tests/xid-ctx-probe` runs the identical call from both contexts
   and prints each task's root superblock:

   ```
   process (kmod):     fs_root=/ sb_dev=0x26 root_ino=256 -> EEXIST (found, SUCCESS)
   kworker (workqueue): fs_root=/ sb_dev=0x4  root_ino=1   -> ENOENT  (not found, FAIL)
   ```

   Same mount namespace, same displayed `/`, **different superblocks**:
   `sb_dev=0x26` is the real btrfs root; `sb_dev=0x4` is the initramfs
   `rootfs` tmpfs.  `/var/log/mtgpu` lives on btrfs, so the kworker cannot see
   it.  The mkdir is dispatched through `drv_mkdir_work_func` (a workqueue),
   and `switch-root` does not update `init_fs.root` used by kthreads.

Reproduced stably across four probe runs.  This is a **vendor driver design
flaw** (workqueue path resolution against `init_fs` + find-or-fail mkdir).  It
does **not** affect GPU compute/inference/stability (faults still reach
`dmesg`/`journalctl`); only the vendor's on-disk convenience record is absent.
Persistent on-disk records require a vendor driver change.  Full analysis:
[root-cause-mtgpu-xid-initramfs-2026-11-27](root-cause-mtgpu-xid-initramfs-2026-11-27.md).

## Changes this session

| Change | Evidence / safety |
|---|---|
| `tests/xid-sem-probe/` — decisive `start_creating_path` create-semantics probe | read-only + throw-away `/var/tmp/xidsem`; proves find-or-fail |
| `tests/xid-ctx-probe/` — process-vs-kworker A/B probe with superblock identity | read-only lookups; proves the init_fs root cause |
| `docs/root-cause-mtgpu-xid-initramfs-2026-11-27.md` | rewritten to the proven cause (supersedes the wrong theories) |
| `docs/limitations.md` | added the proven XID limitation |
| `packaging/rpm/SPECS/mtgpu-dkms.spec` 3.1.0-18 | comments corrected to stop overclaiming `omit_drivers`; changelog records the proven cause |

`omit_drivers+=" mtgpu "` is retained as initramfs hygiene only (smaller
initramfs, driver loads in the real root via udev), and its shipped comment now
says so.  No `MUSA_S30_TEST_APPROVED` workload was run beyond the read-only
`verify-musa.sh` suite; no module reload that would disturb audio/DRM, no
reboot, and no initramfs/bootloader change was made.

## Remaining

Persistent on-disk XID records need a **vendor driver fix** (mkdir/open in
process context against the real root, or `LOOKUP_CREATE` + `vfs_mkdir`).  This
is outside safe packaging scope for a non-critical logging feature and is
documented as a known vendor limitation, not a project defect.

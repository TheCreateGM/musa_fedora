# Root cause (PROVEN): mtgpu XID record file is never written

Host: Fedora Linux 45 Workstation, kernel `7.3.4-300.fc45.x86_64`,
Moore Threads MTT S30 (`0000:01:00.0`, PCI `1ed5:0102`), `mtgpu` 3.1.0,
MUSA runtime 1.5.4-8.  Status: **root cause proven by direct A/B kernel
probe** (2026-11-27).  This document supersedes the earlier "parent directory
missing / initramfs `omit_drivers`" theory in this file's history, which was
**incorrect**.

## Symptom

Every boot the kernel logs:

```
mtgpu 0000:01:00.0: Failed to create directory: /var/log/mtgpu, ret = -2
mtgpu 0000:01:00.0: Failed to create directory: /var/log/mtgpu/event_report, ret = -2
mtgpu 0000:01:00.0: Failed to create directory: /var/log/mtgpu/event_report/0b336cab-1977-ccbe-4e9a-a18ef71961cc, ret = -2
mtgpu 0000:01:00.0: xid_record file create fd failed
```

and the on-disk `xid_records` file is never opened for writing.  `-2` is
`-ENOENT`.

## The two facts that produce the failure

### Fact 1 — `mtgpu_drv_mkdir()` is *find-or-fail*; it cannot create a missing directory

The vendor `mtgpu_drv_mkdir(path, mode)` calls the kernel's
`start_creating_path(AT_FDCWD, path, &parent, LOOKUP_DIRECTORY)` — i.e. with
**only** `LOOKUP_DIRECTORY`, *not* `LOOKUP_CREATE`.  Per the kernel's own
`fs/namei.c :: lookup_one_qstr_excl()`:

> "Will return `-ENOENT` if name isn't found and `LOOKUP_CREATE` wasn't passed."

Measured with `tests/xid-sem-probe/xidsem.ko` (self-cleaning, only touches
`/var/tmp/xidsem`):

| case | flags | result |
|---|---|---|
| target present | `LOOKUP_DIRECTORY` | `OK` / `EEXIST` → driver treats as success |
| **target absent, parent present** | `LOOKUP_DIRECTORY` (driver's flags) | **`-ENOENT`, and nothing is created** |
| target absent, parent present | `LOOKUP_DIRECTORY\|LOOKUP_CREATE` | returns negative dentry for the caller to `vfs_mkdir` |

So the driver only ever *finds* the directory (then `EEXIST` → success) or
*fails*.  It never creates one.  The driver logs `ret = -2` for anything that
is not `-EEXIST`, so a `-ENOENT` is reported verbatim as "Failed to create
directory".

### Fact 2 — the directory creation runs in a *kworker* that sees the initramfs root, not the real root

The mkdir is dispatched through `drv_mkdir_work_func()` (a workqueue), not run
in the module-init process.  Kernel threads resolve `AT_FDCWD`/absolute paths
against `current->fs` = `init_fs`, whose root is the **initramfs `rootfs`
superblock**, not the real root filesystem.  `switch-root` updates PID 1's root
to the real disk but does **not** update `init_fs.root` used by kthreads.

Measured with `tests/xid-ctx-probe/xidctx.ko`, which runs the *identical*
`start_creating_path(AT_FDCWD, "/var/log/mtgpu", LOOKUP_DIRECTORY)` call from
both contexts and prints each task's `fs->root` superblock identity.  Reproduced
stably across four runs:

```
XIDCTX process: comm=kmod    fs_root=/ sb_dev=0x26 root_ino=256  -> err=-17 (EEXIST: found, driver=SUCCESS)
XIDCTX kworker: comm=kworker/X:Y fs_root=/ sb_dev=0x4  root_ino=1   -> err=-2 (ENOENT: not found, driver=FAIL)
```

* same mount namespace (`mnt_ns` identical for both),
* same displayed `fs_root=/`,
* **different superblocks**: process `sb_dev=0x26` is the real btrfs root
  (subvol root inode 256); kworker `sb_dev=0x4` is the initramfs `rootfs`
  tmpfs (root inode 1).

`/var/log/mtgpu` exists on the real btrfs (`sb_dev=0x26`) but not in the
initramfs `rootfs` (`sb_dev=0x4`), so the kworker gets `-ENOENT`.  Combined with
Fact 1 (can't create), the driver fails all three mkdirs and never opens
`xid_records`.

The device UUID `0b336cab-1977-ccbe-4e9a-a18ef71961cc` is **stable across
boots** (same in boots 0/-1/-2), so the tree is fully predictable.

## Why the earlier theories were wrong

* **"omit_drivers / module in initramfs"**: moving `mtgpu` out of the initramfs
  (`/etc/dracut.conf.d/99-musa-mtgpu.conf`) did **not** change the outcome —
  the 2026-11-27 06:54 boot still logged the same `-ENOENT`.  The failure is
  not *where the module is loaded* but *which superblock the kworker resolves
  against* (`init_fs` = initramfs rootfs regardless).
* **"parent directory missing"**: the probe shows `-ENOENT` here means
  "target not found and no `LOOKUP_CREATE`", not "parent missing".  `/var/log`
  and `/var/log/mtgpu` are present on the real root; the kworker simply looks
  in the wrong superblock.

## What this is / is not

* This is a **vendor driver design flaw**: the opaque `mtgpu_core` blob runs its
  event-report/XID file setup from a workqueue that resolves paths against
  `init_fs` (initramfs rootfs) instead of the real root, and its
  `mtgpu_drv_mkdir` cannot create directories.  Fixing persistent on-disk XID
  records requires a **vendor driver change** (run the mkdir/open in process
  context against the real root, or resolve via `kern_path` on the init
  namespace root, or pass `LOOKUP_CREATE` and `vfs_mkdir`).
* It is **not** a GPU functionality, compute, stability, or MUSA-stack defect.
  GPU faults/XIDs are still reported to `dmesg`/`journalctl`; only the vendor's
  *on-disk* convenience record file is missing.  The MUSA stack is fully
  validated on this host (see `hardware-validation-*`).

## Safe mitigations in this repo

* `packaging/rpm/SPECS/mtgpu-dkms.spec` ships
  `/usr/lib/tmpfiles.d/mtgpu.conf` that pre-creates the **complete** stable
  tree on the real root, including the per-device UUID directory and the empty
  `xid_records` file.  This closes the "driver cannot create the UUID dir"
  gap and guarantees a complete tree for any process-context access or a future
  vendor fix.  It does **not** by itself make the kworker see the real root.
* `tests/xid-sem-probe/` and `tests/xid-ctx-probe/` are the reproducible
  diagnostics that prove Facts 1 and 2 above.  Build with `make`, load with
  `pkexec insmod ./<name>.ko`, read `dmesg | grep XIDSEM|XIDCTX`, unload with
  `pkexec rmmod`.  They only perform lookups / throw-away `/var/tmp` scratch.

## Fix that would actually work (not implemented — vendor/kernel change)

Making the kworker resolve against the real root is the only way to get
persistent on-disk records.  Options, all outside safe packaging scope for a
non-critical logging feature:

1. Vendor: run `mtgpu_drv_mkdir`/`filp_open` in a process context (or
   `kthread_use_mm` / resolve via `kern_path` against the init-namespace root)
   and use `LOOKUP_CREATE` + `vfs_mkdir`.
2. Kernel: have `switch-root` update `init_fs.root` (not done upstream).
3. Bind the real `/var/log/mtgpu` into the initramfs `rootfs` — infeasible from
   userspace after switch-root (that rootfs is orphaned).

Per the project's safety policy (no unverified initramfs/bootloader changes for
a non-critical feature; VM-first), none of 1–3 was implemented.  No initramfs,
bootloader, kernel, firmware, or driver binary was modified.

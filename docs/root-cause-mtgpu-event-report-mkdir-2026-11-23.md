# Root cause: `mtgpu` event_report mkdir returns -ENOENT (2026-11-23)

## Symptom

Every load of the `mtgpu` module on the MTT S30 host logs:

```text
mtgpu 0000:01:00.0: Failed to create directory: /var/log/mtgpu, ret = -2
mtgpu 0000:01:00.0: Failed to create directory: /var/log/mtgpu/event_report, ret = -2
mtgpu 0000:01:00.0: Failed to create directory: /var/log/mtgpu/event_report/0b336cab-1977-ccbe-4e9a-a18ef71961cc, ret = -2
mtgpu 0000:01:00.0: xid_record file create fd failed
```

Observed at the 2026-11-23 07:39 controlled load (`journalctl -k -b`) and
earlier. `-2` is `-ENOENT`.

## Why the existing directory does not explain it

The directories **already exist** on the host:

```text
drwxr-xr-x root root /var/log/mtgpu
drwxrwxrwx root root /var/log/mtgpu/event_report
drw----r-x root root /var/log/mtgpu/event_report/0b336cab-1977-ccbe-4e9a-a18ef71961cc
-rw-r--r-- root root /var/log/mtgpu/event_report/0b336cab-.../xid_records
```

The `xid_records` file was written by the driver itself on 2026-11-19, with
the same UUID that appears in the 2026-11-23 message. So the driver created
this tree before, and the UUID is a stable per-board identifier.

`mkdir()` on an existing name returns `-EEXIST` (-17), never `-ENOENT` (-2).
The kernel path the driver uses confirms this (see below): with
`LOOKUP_EXCL` an existing name yields `-EEXIST`. The observed `-2` therefore
means the driver's lookup **did not observe an existing entry** where it
looked — the failure is not "the directory is missing".

## The code path (verified from the shipped artefacts)

The message is emitted by the vendor core. Resolving the relocation that
points at the format string
(`.rodata.str1.8+0xdc70`, `"Failed to create directory: %s, ret = %d"`)
identifies exactly one referencing function:

```text
$ objdump -dr --disassemble=mtgpu_event_report_persist_init \
      driver-s30-3.1.0/objs/x86_64/mtgpu_core.o_binary
 85fb9: call os_snprintf                       # build "<uuid>" from the PCI device
 85fd9: call os_snprintf                       # build "/var/log/mtgpu/event_report/<uuid>"
 86004: call mtgpu_drv_mkdir                   # create the tree
 86009: test %eax,%eax
 8600b: jne  860e8                             # on error: log and CONTINUE
 860e8: ...
 860f7: .rodata.str1.8+0xdc70                 # "Failed to create directory: %s, ret = %d"
 860fb: call _os_dev_err
 86100: jmp 86011                              # -> event_report_create_fd() anyway
```

Two facts follow:

1. The warning is **logged and ignored** by the driver. Execution continues
   into `event_report_create_fd()`, which then fails with
   `xid_record file create fd failed`. Nothing else in the driver depends on
   the result.
2. The only lost functionality is the **vendor XID event log file** on disk.
   No GPU feature depends on it.

The directory-creation half is open source and is shipped in this tree:

```c
/* driver-s30-3.1.0/src/common/os-interface.c */
void *os_path_create(void)                                   /* kzalloc(64)        */
struct dentry *os_kern_path_create(int dfd, const char *name,
                struct path *path, unsigned int lookup_flags)
{
        return start_creating_path(dfd, name, path, lookup_flags);
}
int os_security_path_mkdir(struct path *path, struct dentry *dentry, umode_t mode)
{
        return security_path_mkdir(path, dentry, mode);
}
int os_vfs_mkdir(struct path *path, struct dentry *dentry, umode_t mode)
{
        return vfs_mkdir(mnt_idmap(path->mnt), path->dentry->d_inode, dentry, mode);
}
```

`mtgpu_drv_mkdir()` (vendor core) wires them together; the sequence is the
kernel's own `mkdir` sequence, and the running kernel exports exactly these
symbols:

```text
$ grep -E 'security_path_mkdir|start_creating_path|vfs_mkdir' \
      /usr/src/kernels/7.3.4-300.fc45.x86_64/Module.symvers
0x00000000	start_creating_path	vmlinux	EXPORT_SYMBOL
0x00000000	vfs_mkdir	vmlinux	EXPORT_SYMBOL
0x00000000	security_path_mkdir	vmlinux	EXPORT_SYMBOL
```

Kernel side (Fedora 7.x carries the dentry-based mkdir series):

```c
/* fs/namei.c */
static struct dentry *filename_create(int dfd, struct filename *name,
                                       struct path *path, unsigned int lookup_flags)
{
        ...
        create_flags = LOOKUP_CREATE | LOOKUP_EXCL;
        ...
        dentry = start_dirop(path->dentry, &last, reval_flag | create_flags);
        ...
}
/* fs/namei.c:1822 */
if (d_is_positive(dentry) && (flags & LOOKUP_EXCL)) {   /* -> -EEXIST */
```

so an **existing** target returns `-EEXIST` (-17), and the only `-ENOENT`
sources left in the sequence are:

* `may_create_dentry()` — `if (IS_DEADDIR(dir)) return -ENOENT;`
* `security_inode_mkdir()` / LSM — SELinux SID computation failure
  (`security_compute_create()` can return `-ENOENT`).

## SELinux is not the cause

Checked on the live host (`SELinux: Enforcing`):

* No AVC denial exists for the load window
  (`ausearch -m avc -ts 11/23/2026 07:30:00` → empty). A SELinux denial
  would have been recorded; the `-ENOENT` from SID computation is *not* an
  AVC event, so this is not proof on its own.
* A userspace `mkdir /var/log/mtgpu_selinux_probe` as root **succeeds** on the
  same directory, so SELinux policy permits creating entries under
  `var_log_t`.
* The affected directories are correctly labelled
  (`system_u:object_r:var_log_t:s0`), i.e. they are not in a state that
  would make a transition fail.

So the most likely remaining sources are the `IS_DEADDIR(parent)` case in
`may_create_dentry()` or a path-resolution detail in the vendored
`security_path_mkdir()` variant.

## Consequence for packaging

`mtgpu-dkms` 3.1.0-12 ships a `tmpfiles.d` entry and a `%post` that create
`/var/log/mtgpu` and `/var/log/mtgpu/event_report`. That guarantees the
directories exist **on fresh installs** (where the driver genuinely needs
them), which is necessary but **not sufficient**: on this host the
directories already existed and the same `-2` was logged. The 3.1.0-12
changelog says exactly this, and the warning is cosmetic.

Do **not** delete `/var/log/mtgpu/event_report` as an ad-hoc fix. Removing
the stale tree would at best change the errno from `-2` to `-17`
(`-EEXIST`), and at worst lose the vendor's recorded XID history from
2026-11-19.

## Test that would settle it (gated)

The only way to observe the driver's mkdir on a *fresh* tree is to load the
module again, which is deliberately gated in this project:

1. In a reviewed boot (not the current one), move
   `/var/log/mtgpu` aside:
   `mv /var/log/mtgpu /var/log/mtgpu.stale-2026-11-19`.
2. Perform the approved controlled load
   (`scripts/hardware/load-and-test.sh`), which performs exactly one
   `insmod` with `display=none disable_fbdev=1 mtgpu_drm_major=2`.
3. Compare the kernel log:
   * directories created successfully → the failure is specific to the
     pre-existing/stale tree;
   * still `-2` → the failure is in the vendored `security_path_mkdir()`
     path selection and is independent of the tree state.

Until that controlled load is approved, this remains an open, documented
defect in the vendor core.

## Independent live re-verification (same boot, 2026-11-23)

While investigating, the full suite was re-run against the already loaded
driver to confirm the warning is cosmetic and that nothing regressed
(`scripts/verify-musa.sh`, log kept at `/tmp/verify-2026-11-23b.log`):

```text
PASS  MTT S30 present on PCI (1ed5:0102)      mtgpu bound, render node renderD130
PASS  mtgpu module loaded (init state: live)   /dev/mtgpu.0 present
PASS  IOMMU groups present (13 groups)
PASS  mthreads-gmi reports the MTT S30         Driver Version 3.1.0, 4096 MiB
PASS  musaInfo succeeded                       runtime 1.5.4, one device
PASS  muMemcpyTest succeeded
PASS  musart-smoke passed on the GPU            totalGlobalMem=4267532288, 8 SMs
PASS  mcc built tests/musa-vector-add.mu      1,048,576 float adds verified
PASS  saxpy, shared reduce, atomics, constant memory, libm, 2D grid, D2D, fp64
PASS  MTT S30 Vulkan device, buffer, and host-visible map
SKIP  no Moore Threads OpenCL ICD in this runtime
Summary: 13 passed, 0 failed, 1 skipped
```

`journalctl -k -b` after the run contains no `Oops`, `BUG:`, call trace,
segfault or GPU fault. The only `mtgpu` messages in the entire boot are the
IRQ-affinity notices, the two `pixel format with alpha exposed but blend
mode not setup` warnings from the DRM planes (expected in the headless
`display=none` configuration), the four event_report messages analysed
above, and one `force destroy app memory context` notice when `musaInfo`
exited.

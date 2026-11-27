# Root cause: the mtgpu XID record file is never written (2026-11-27)

Host: Fedora Linux 45 Workstation, kernel `7.3.4-300.fc45.x86_64`,
Moore Threads MTT S30 (`0000:01:00.0`, PCI `1ed5:0102`), `mtgpu` 3.1.0
(verified working), MUSA runtime 1.5.4-8.

## Symptom

Every boot since 2026-11-19 the kernel logs:

```
mtgpu 0000:01:00.0: Failed to create directory: /var/log/mtgpu, ret = -2
mtgpu 0000:01:00.0: Failed to create directory: /var/log/mtgpu/event_report, ret = -2
mtgpu 0000:01:00.0: Failed to create directory: /var/log/mtgpu/event_report/0b336cab-1977-ccbe-4e9a-a18ef71961cc, ret = -2
mtgpu 0000:01:00.0: xid_record file create fd failed
```

This is not cosmetic. The second message means the driver never opened its
XID record file, so **no GPU fault record is ever written for the lifetime of
the machine**. The file on disk is unchanged since it was created on
2026-11-19:

```
$ stat -c '%s bytes, mtime=%y %n' /var/log/mtgpu/event_report/*/xid_records
32 bytes, mtime=2026-11-19 00:20:54.396037207 +0000
```

Its content is only the file header (`XID` + version + the device UUID), not
a single fault record.

Earlier project notes dismissed this as "cosmetic"
(`mtgpu-dkms` release 12). That was wrong: the failure disables the GPU's
fault-reporting path entirely.

## Root cause

**The mtgpu module is loaded inside the initramfs, where `/var/log` does not
exist, so the driver's event-report directory creation fails with `-ENOENT`
and the XID record file is never opened.**

### Step 1 - what the driver actually does

The vendor core (`objs/x86_64/mtgpu_core.o_binary`) is an opaque pre-built
object, so the code was recovered from the *linked* module
(`/lib/modules/7.3.4-300.fc45.x86_64/extra/mtgpu.ko`), where the relocations
are resolved:

* `mtgpu_drv_mkdir(path, mode)` calls
  `start_creating_path(AT_FDCWD, path, &ctx, LOOKUP_DIRECTORY)`.
  This is the kernel's "path creation" API. The driver treats a `-EEXIST`
  return as success (`PTR_ERR(ret) + EEXIST == 0` -> return 0) and reports
  any other error verbatim.
* `start_creating_path()` returns `-EEXIST` when the target exists and
  `-ENOENT` when the **parent directory** does not exist.
* `event_report_create_fd()` then opens
  `/var/log/mtgpu/event_report/<uuid>/xid_records` with
  `filp_open(path, O_RDWR | O_CREAT | O_APPEND, 0644)`.

The flags and the mode were read out of the linked module's `.rodata`
(`os_value[]` entries 122/126/127 = `O_RDWR`/`O_CREAT`/`O_APPEND`, and the
literal `0644`), not guessed.

### Step 2 - the kernel behaviour, measured

`tests/xid-path-probe/` is a small out-of-tree module that calls the same
`start_creating_path()` with the driver's flags. It only performs lookups;
it creates, modifies and removes nothing.

```
$ pkexec insmod ./xidprobe.ko ; dmesg | grep XIDPROBE2
XIDPROBE2 driver-flags flags=0x00000002 path=/var/log/mtgpu/            -> err=-17
XIDPROBE2 driver-flags flags=0x00000002 path=/var/log/mtgpu             -> err=-17
XIDPROBE2 driver-flags flags=0x00000002 path=/var/log/mtgpu/event_report -> err=-17
XIDPROBE2 driver-flags flags=0x00000002 path=/var/log/mtgpu/event_report/0b336cab-... -> err=-17
XIDPROBE2 driver-flags flags=0x00000002 path=/nonexistent-parent-xyz/foo -> err=-2
```

`-17` is `-EEXIST` (target exists -> the driver treats it as success),
`-2` is `-ENOENT` (parent missing -> the driver reports the failure).
The driver's `ret = -2` therefore means **the parent directory did not
exist**.

### Step 3 - where the driver is initialised

`mtgpu.ko` is part of the initramfs:

```
$ lsinitrd /boot/initramfs-7.3.4-300.fc45.x86_64.img | grep mtgpu
usr/lib/modules/7.3.4-300.fc45.x86_64/extra/mtgpu.ko.xz
```

dracut's hostonly mode includes it automatically because the MTT S30 is
present in the host. The initramfs has **no `/var/log`** (it has
`/var/lib/dracut/hooks/...` only), and the initramfs's own systemd runs
`systemd-udev-trigger.service` (coldplug) at 00:11:49-00:11:52, which is
where the module is loaded:

```
00:11:49 systemd[1]: Starting systemd-udev-trigger.service - Coldplug All udev Devices...
00:11:49 systemd[1]: Reached target initrd-usr-fs.target - Initrd /usr File System.
00:11:52 kernel: mtgpu: loading out-of-tree module taints kernel.
00:11:52 kernel: mtgpu 0000:01:00.0: Matched GPU device id is: 0x0102
00:11:56 kernel: mtgpu 0000:01:00.0: Failed to create directory: /var/log/mtgpu, ret = -2
```

Because the module is already loaded by then, it is not re-initialised after
`switch_root`; the only chance the driver ever gets to create its log
directories is the one inside the initramfs, where `/var/log` is missing.
All three directory creations fail with `-ENOENT`, the event-report
initialisation aborts, and the XID file open is skipped.

### Step 4 - why it worked on 2026-11-19

On that day the driver did create the per-device directory
`/var/log/mtgpu/event_report/0b336cab-...` (birth timestamp 2026-11-19
00:20:54) with mode `0605` - the mode the driver itself uses
(`S_IRUSR|S_IWUSR|S_IROTH|S_IXOTH`, i.e. no owner execute bit). It also
created `xid_records` there. On later boots the module is loaded in the
initramfs first, so the driver finds `/var/log` missing and never gets that
far.

## Fix

Keep the module out of the initramfs so the driver initialises in the real
root, where `/var/log` exists:

```
# /etc/dracut.conf.d/99-musa-mtgpu.conf
omit_drivers+=" mtgpu "
```

`omit_drivers` is a documented dracut.conf knob ("Specify a space-separated
list of kernel modules not to add to the initramfs"). The module is still
loaded automatically in the real root - udev coldplugs the PCI device and
the `pci:v00001ED5d00000102` alias of the mtgpu module matches - so:

* no blacklist is used,
* the device stays fully enabled,
* no kernel, bootloader or firmware change is involved,
* the initramfs only gets smaller (72,215,456 -> 69,663,503 bytes).

The package ships this file (`mtgpu-dkms` release 3.1.0-17) and rebuilds the
initramfs from `%post`.

## Verification

See `docs/hardware-validation-2026-11-27-xid-fix.md` for the post-reboot
evidence.

## What was ruled out

| Hypothesis | Result |
|---|---|
| The `xid_records` file is stale and blocks `O_APPEND` | Ruled out: `O_CREAT\|O_APPEND` opens an existing file successfully (measured as root) |
| The per-device directory mode `0605` (no owner execute bit) blocks the lookup | Ruled out: root opens files inside such a directory (measured); the driver runs as root |
| SELinux denies the operation | Ruled out: no AVC for `mtgpu`, `var_log_t` or `event_report` in the boot journal; SELinux is enforcing but the driver's own `security_path_mkdir()` call reported no denial |
| The driver's mount namespace hides `/var/log` | Ruled out: the real root's `/var/log/mtgpu` is visible in `systemd-udevd`'s private mount namespace |
| The driver is unloaded/rebound on purpose | Not the case: `mtgpu` is loaded once, by the initramfs |

# Compute mode enabled on the installed host (2026-11-24)

**Host:** Fedora Linux 45 Workstation, kernel `7.3.4-300.fc45.x86_64`
**GPU:** MTT S30 `0000:01:00.0` (`1ed5:0102`), IOMMU group 11
**Packages:** `musa-runtime` 1.5.4-5, `musa-sdk` 5.2.0-6,
`musa-toolkit` 3.1.0-3, `mtgpu-dkms` 3.1.0-12, `mtgpu-firmware` 3.1.0-4;
`musa-runtime-display` **not installed** (no vendor Mesa DRI/GBM/Xorg files)

## What changed

The host no longer relies on `/etc/modprobe.d/blacklist-mtgpu.conf` to keep
`mtgpu` unloaded on a normal boot. `scripts/compute-mode.sh enable` was run at
**2026-11-24 02:08 UTC** so udev auto-binds the S30 at boot and the GPU is
usable without a manual one-shot load.

The reason the boot-time block existed was the Mesa/GNOME KMS crash caused by
the vendor DRI drivers (`docs/boot-fix-2026-11-23.md`). Since
`musa-runtime` 1.5.4-4 those drivers live in the opt-in
`musa-runtime-display` subpackage (`docs/display-driver-subpackage-2026-11-23.md`),
which is not installed here, so the crash condition is gone.

## Preflight (passed, nothing changed before this point)

```
[ OK ] musa-runtime-display is not installed
[ OK ] module present: /lib/modules/7.3.4-300.fc45.x86_64/extra/mtgpu.ko.xz
[ OK ] initramfs tool: dracut
       S30 1ed5:0102 present at 0000:01:00.0
```

## Changes applied

| Item | Result |
|---|---|
| Backup | `/var/backups/musa-fedora-compute-mode/20261124T020816Z/` (old `blacklist-mtgpu.conf` + `.moved`, `90-mtgpu-headless.conf`, every `/etc/modprobe.d/*.conf`, previous initramfs) |
| `/etc/modprobe.d/blacklist-mtgpu.conf` | moved into the backup directory (not deleted) |
| `/etc/modprobe.d/90-mtgpu-headless.conf` | kept: `options mtgpu display=none disable_fbdev=1 mtgpu_drm_major=2` |
| initramfs | regenerated with dracut for `7.3.4-300.fc45.x86_64` (no `mtgpu` blacklist inside) |
| module load | already live (`initstate=live`); the script did not reload it |

The script restores every moved file automatically if initramfs regeneration
fails. It never unloads the module and never unbinds the PCI device.

## Verification performed on the live host

```sh
$ ls /etc/modprobe.d/ | grep mtgpu
90-mtgpu-headless.conf
90-mtgpu-headless.conf.rpmnew
blacklist-mtgpu.conf.bak
mtgpu.conf
mtgpu.conf.bak-20261112-153847
mtgpu.conf.disabled-by-bootfix
# (no active blacklist-mtgpu.conf)

$ pkexec lsinitrd -f /etc/modprobe.d/blacklist-mtgpu.conf /boot/initramfs-$(uname -r).img
# (empty: the regenerated initramfs carries no blacklist for mtgpu)

$ modprobe -c | grep -i '^blacklist mtgpu'
# (no match)

$ udevadm test --action=add /sys/bus/pci/devices/0000:01:00.0 2>&1 | grep -E 'kmod load|mtgpu'
/usr/lib/udev/rules.d/80-drivers.rules:5 RUN{builtin}+="kmod load"
  mtgpu
  DRIVER=mtgpu
  RUN{builtin} : kmod load
```

That last line is the boot autoload path: the PCI modalias of `0000:01:00.0`
resolves to `mtgpu`, and udev's `kmod load` builtin is what loads it (the same
path observed in the 2026-11-23 boot, where `mtgpu` loaded automatically at
00:51:53 when the blacklist was inactive).

Live state after enabling:

```
musa-runtime           1.5.4-5.fc45
musa-runtime-display   not installed
mtgpu                  initstate=live
/dev/mtgpu.0           present, mode 0666 + uaccess tag
/dev/dri/renderD130    bound to mtgpu
card2                  -> 0000:01:00.0 (Virtual-1, compute-only DRM device)
GNOME                  2 gnome-shell processes alive before and after
```

`scripts/verify-musa.sh` was then re-run on this stack: **13 passed, 1 failed,
1 skipped**. The only failure is the vendor telemetry tool
(`mthreads-gmi`, `docs/root-cause-mthreads-gmi-drm-query-2026-11-24.md`);
`musaInfo`, `muMemcpyTest`, the runtime API smoke test, the `mp_10`
vector-add, the eight-kernel compute suite, FP64 and headless Vulkan all pass.

## What is verified and what still needs one reboot

**Verified now (no reboot):** the blacklist is gone from `/etc/modprobe.d`
*and* from the live initramfs, udev resolves the S30 to `kmod load mtgpu`, the
module is live and the full MUSA workload passes with GNOME running.

**Still to confirm on the next boot:** that udev loads `mtgpu` before GDM and
that GNOME still reaches the login screen. The two ingredients are already
independently verified — automatic udev load of `mtgpu` (2026-11-23 boot) and
coexistence of a live `mtgpu` with GNOME when the vendor DRI files are absent
(the 2026-11-23 and 2026-11-24 controlled loads) — but the exact
"autoload + no-DRI package" boot has not been observed yet in this session.

After the next reboot, check:

```sh
lspci -k -s 01:00.0                  # expect: Kernel driver in use: mtgpu
cat /sys/module/mtgpu/initstate      # expect: live
ls -l /dev/mtgpu.0 /dev/dri/renderD* # expect /dev/mtgpu.0 and an mtgpu render node
systemctl is-active gdm              # expect: active
source scripts/musa-env.sh
scripts/verify-musa.sh               # expect 13 passed / 1 failed (gmi) / 1 skipped
```

If GNOME fails to start, the display still works on `i915`; log in on a
console (or boot the rescue entry) and restore the previous boot state:

```sh
pkexec scripts/compute-mode.sh disable
pkexec systemctl reboot
```

`disable` restores the blacklist from the backup, regenerates the initramfs,
and leaves the module loaded until the reboot.

## Recovery inputs

| Item | Path |
|---|---|
| Full pre-change backup | `/var/backups/musa-fedora-compute-mode/20261124T020816Z/` |
| Old blacklist (original path copy) | `.../blacklist-mtgpu.conf` |
| Old blacklist (moved file) | `.../blacklist-mtgpu.conf.moved` |
| Previous initramfs | `.../initramfs-7.3.4-300.fc45.x86_64.img` |
| modprobe config snapshot | `.../modprobe.d-all/` |
| Rescue kernel | `/boot/vmlinuz-0-rescue-*` (present) |

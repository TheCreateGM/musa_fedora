# compute-mode.sh: Fedora initramfs regeneration fix (2026-11-23)

## Scope

This records a defect in [`scripts/compute-mode.sh`](../scripts/compute-mode.sh)
and the fix. The fix was exercised with `status` and `--dry-run` on the live
Fedora 45 host while `mtgpu` was loaded. **No persistent host change was made**:
the boot-time blacklist, `/etc/modprobe.d`, the initramfs and the loaded module
were left untouched. A real `enable`/`disable` still requires a reviewed
reboot plan; see [`compute-mode.md`](compute-mode.md).

## Defect

`compute-mode.sh` is the helper that enables the MTT S30 for compute on a
normal boot by removing the boot-time `mtgpu` blacklist. Both `enable` and
`disable` regenerated the initramfs with:

```sh
if command -v update-initramfs >/dev/null 2>&1; then
    update-initramfs -u -k "$KVER"
fi
```

`update-initramfs` is Debian/Ubuntu tooling. Fedora uses `dracut`, so on
Fedora the condition was false and the block was silently skipped. The
blacklist file was moved, but the existing
`/boot/initramfs-$KVER.img` could still carry the old modprobe state. The
script then reported success. That is the dangerous failure mode:

- `enable` could look successful while the next boot still blocked `mtgpu`,
  or could leave `/etc/modprobe.d` and the initramfs inconsistent.
- `disable` could restore `/etc/modprobe.d/blacklist-mtgpu.conf` but leave an
  initramfs that no longer carries the block.
- The failure was a warning at most; it never stopped the operation.

The earlier documentation claimed the initramfs was regenerated. On Fedora
that was not true with the code above.

## Fix

The script now:

1. Detects `dracut` first, then `update-initramfs` as a fallback.
2. **Fails loudly** (`return 1`/preflight failure) when neither tool exists.
   It no longer continues after silently skipping initramfs regeneration.
3. Backs up `/boot/initramfs-$KVER.img` into the timestamped backup directory
   before overwriting it.
4. Uses `dracut --force --kver "$KVER"` on Fedora, writing the tool log to
   `/tmp/compute-mode-initramfs.log` and printing the tail on failure.
5. In `enable`, if regeneration fails, restores every blacklist file that was
   moved and stops **before** loading the module. The boot-time block is
   therefore preserved.
6. In `disable`, if regeneration fails, leaves the restored blacklist in place
   but returns non-zero and tells the operator not to reboot until the
   initramfs is repaired manually.
7. Adds `--dry-run` to `enable`/`disable`: it performs all read-only
   preflight checks and prints the exact plan without requiring root and
   without changing a file, module, or initramfs.
8. Fixes the DRI/GBM/Xorg detection so `status` no longer reports the
   `musa-runtime-display` crash condition when no vendor driver exists.

`status` now also reports which initramfs tool is available.

## Verification performed

Host: Fedora 45, kernel `7.3.4-300.fc45.x86_64`; `mtgpu` 3.1.0 loaded in this
boot by the separately approved controlled load test; `musa-runtime` 1.5.4-4
installed; no MUSA DRI/GBM/Xorg driver present.

```text
$ bash -n scripts/compute-mode.sh
syntax OK

$ scripts/compute-mode.sh status
  ...
  Display drivers (must be absent for a compute-only host)
    [ OK ] no vendor Mesa DRI/GBM/Xorg driver installed
  ...
  [ OK ] module for running kernel: /lib/modules/7.3.4-300.fc45.x86_64/extra/mtgpu.ko.xz
  [ OK ] module loaded (initstate=live)
  ...
  [ OK ] initramfs tool: dracut
  [ OK ] S30 1ed5:0102 present at 0000:01:00.0

$ scripts/compute-mode.sh enable --dry-run
  [ OK ] musa-runtime-display is not installed
  [ OK ] module present: ...
  [ OK ] initramfs tool: dracut
DRY-RUN: the preflight passed; no file, module, or initramfs was changed.
  would back up /etc/modprobe.d/blacklist-mtgpu.conf and /etc/modprobe.d/*.conf
  would ensure /etc/modprobe.d/90-mtgpu-headless.conf sets display=none
  would move any file that blacklists mtgpu out of /etc/modprobe.d
  would run: dracut (regenerate /boot/initramfs-7.3.4-300.fc45.x86_64.img)
  would run: modprobe mtgpu
rc=0

$ scripts/compute-mode.sh disable --dry-run
DRY-RUN: the preflight passed; no file or initramfs was changed.
  would restore /etc/modprobe.d/blacklist-mtgpu.conf from the newest backup (or write a fresh one)
  would run: dracut (regenerate /boot/initramfs-7.3.4-300.fc45.x86_64.img)
  would NOT unload mtgpu; a reboot is required
rc=0
```

The missing-tool path was simulated by running the script with a `PATH` that
contained every command it needs **except** `dracut` and `update-initramfs`:

```text
[ OK ] musa-runtime-display is not installed
[ OK ] module present: ...
[ERR ] neither dracut nor update-initramfs is available

[ERR ] preflight failed — nothing was changed
rc=1
```

No `/var/backups/musa-fedora-compute-mode` directory was created by the
dry-runs or the failure simulation, which confirms they were read-only.

## What this does and does not establish

Established: the Fedora tool detection is correct, the failure path is loud
and non-destructive, the dry-run plans the right commands, and the DRI status
check is accurate on a compute-only host.

Not established: a real `enable` followed by a reboot. That remains gated on
an explicit decision because it changes `/etc/modprobe.d`, regenerates
`/boot/initramfs-7.3.4-300.fc45.x86_64.img`, and loads `mtgpu` at boot. The
recovery path is `scripts/compute-mode.sh disable` (or restoring the backup
directory printed by `enable`) and rebooting. See
[`compute-mode.md`](compute-mode.md) for the full workflow and
[`runbook-controlled-load-2026-11-23.md`](runbook-controlled-load-2026-11-23.md)
for the already-executed one-shot load.

# Runbook — approved controlled load and test (2026-11-23)

## Why this must be run from the installed system

The current Live USB session runs kernel `7.2.11-300.fc45.x86_64`, which has no
`mtgpu` module. The pinned module exists only for the installed kernels
`7.3.4`, `7.3.3` and `7.2.15`. Loading a module built for another kernel is
not possible (vermagic mismatch). Therefore the approved single load must be
run after booting the installed Fedora 45 system from disk (kernel `7.3.4`).

## Preconditions

1. Remove the Live USB pendrive and boot the installed Fedora 45 system.
   In GRUB choose `Fedora Linux (7.3.4-300.fc45.x86_64) 45 (Workstation Edition)`.
2. Log in to the GNOME session normally. The boot-time blacklist is still
   active, so the S30 is unbound and the desktop runs on Intel i915.
3. Confirm the environment from a terminal:

```sh
uname -r                                  # 7.3.4-300.fc45.x86_64
cat /etc/modprobe.d/blacklist-mtgpu.conf  # blacklist mtgpu
ls /sys/module/mtgpu                      # must fail (module not loaded)
pgrep -c gnome-shell                      # desktop is running
```

## Run exactly one controlled load

```sh
cd /home/axogm/Documents/project/musa-fedora
scripts/hardware/load-and-test.sh
```

The script is read-only with respect to configuration: it never touches the
blacklist, never unloads the module, never unbinds/rebinds PCI, and performs
one `insmod` with `display=none disable_fbdev=1 mtgpu_drm_major=2`. It writes
all output to `~/.musa-work/load-test-<UTC>/load-and-test.log`, so evidence
survives a desktop crash.

Expected outcome based on the first run:

- all functional steps pass again (memcpy, runtime API, `mp_10` vector-add,
  compute suite, fp64, Vulkan);
- `musaInfo` should now pass with the corrected 1.5.4 tool;
- `mthreads-gmi` will still fail until the redirected
  `/usr/lib64/libdrm_mtgpu.so.1` symlink is restored;
- GNOME Shell will likely crash again on the 1.5.4-3 runtime, because the
  vendor Mesa DRI drivers are still installed. That is expected and is what
  the new `musa-runtime-display` split fixes.

If a kernel fault (`Oops:`, `BUG:`, NULL dereference) appears, the script stops
with exit code 3. Do not retry.

## After the run

1. Reboot the installed system. The blacklist restores the known-good,
   display-only state; no manual cleanup is needed.
2. Copy the new log directory out for analysis if needed:

```sh
ls -la ~/.musa-work/load-test-*/
tail -60 ~/.musa-work/load-test-*/load-and-test.log
```

A clean result requires no kernel fault and `musaInfo` passing. Desktop
stability during the load is a separate fix that depends on installing the
new `musa-runtime` 1.5.4-4 **without** `musa-runtime-display`.

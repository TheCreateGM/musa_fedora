# Compute mode — running MUSA on the S30 from a normal boot

## The problem this solves

The S30 is a PCIe GPU (`1ed5:0102`) that the `mtgpu` driver binds as a DRM
device. Two independent facts had to be separated:

1. **The driver works.** With the pinned vendor stack (`mtgpu` 3.1.0 kernel
   driver + MUSA 1.5.4 runtime + MUSA 5.2.0 `mcc` compiler), the S30 executes
   memcpy, host runtime API calls, `mp_10` kernels, an eight-kernel compute
   suite, FP64 and headless Vulkan buffer operations. See
   [`hardware-validation-2026-11-23-load-test.md`](hardware-validation-2026-11-23-load-test.md).
2. **The desktop broke when the driver was present.** Mesa picked up the vendor
   MUSA DRI drivers (`/usr/lib64/dri/musa_dri.so`, `mtgpu_dri.so`,
   `kms_swrast_musa_dri.so`, …) for the S30 DRM card and GNOME Shell / mutter
   segfaulted in its KMS thread. GDM gave up after repeated display failures
   and no login screen appeared. See
   [`boot-fix-2026-11-23.md`](boot-fix-2026-11-23.md).

The boot-time `blacklist mtgpu` was the blunt instrument used to keep the
desktop alive. It works, but it also keeps the GPU useless on every normal
boot, which is not a solution to the problem.

## The root cause fix (packaging)

The crash was **not** a driver bug: the vendor display drivers were shipped in
the base `musa-runtime` RPM, so every install — including a compute-only one
— put them where Mesa could find them. `musa-runtime` 1.5.4-4 therefore moves
all DRI/GBM/Xorg drivers into a new optional subpackage:

- `musa-runtime` — compute runtime, headers, `libdrm_mtgpu`, Vulkan ICD,
  `mthreads-gmi`, linker/sysctl/modprobe configuration. **No DRM/GBM/Xorg
  drivers.**
- `musa-runtime-display` — `/usr/lib64/dri/`, `/usr/lib64/gbm/`,
  `/usr/lib64/xorg/modules/drivers/`. Pulled in by nothing; install it only
  when the S30's own DP/HDMI outputs are to be used.

A compute-only install therefore cannot trigger the mutter crash, and the
boot-time block is no longer needed.

## Enabling compute at boot

[`scripts/compute-mode.sh`](../scripts/compute-mode.sh) implements it:

```sh
scripts/compute-mode.sh status     # read-only report
scripts/compute-mode.sh enable     # enable at boot (needs root/pkexec)
scripts/compute-mode.sh disable    # restore the boot-time block
```

`enable`:

1. **Preflight.** Refuses to continue and changes nothing if
   - `musa-runtime-display` is installed (the crash condition is still
     present),
   - the `mtgpu` module is missing for the running kernel,
   - the S30 is not on the PCI bus,
   - `musa-runtime` / `mtgpu-dkms` are not installed.
2. **Backs up** `/etc/modprobe.d/blacklist-mtgpu.conf`, the headless option
   file and every `modprobe.d` file into
   `/var/backups/musa-fedora-compute-mode/<UTC timestamp>/`.
3. **Writes** `/etc/modprobe.d/90-mtgpu-headless.conf` when it is missing
   (`display=none disable_fbdev=1 mtgpu_drm_major=2`). `display=none` makes
   the driver register a compute-only DRM device: no KMS connectors, no
   fbdev, nothing that can fight i915 for the console.
4. **Moves the blacklist aside** (never deletes it) and removes any other
   `*.conf` in `/etc/modprobe.d` that still blacklists `mtgpu`.
5. **Regenerates the initramfs**, because the running system's initramfs also
   carries the blacklist — without this the module would still be blocked at
   boot even though `/etc/modprobe.d` no longer blocks it.
6. **Loads the driver once** with `modprobe mtgpu` and reports the resulting
   device nodes. The module is *not* unloaded afterwards; a live removal of
   the driver is not safe.

`disable` restores the blacklist file and regenerates the initramfs. It does
not unload the module — reboot for that.

## Verifying after the change

```sh
scripts/compute-mode.sh status
ls -l /dev/mtgpu.* /dev/dri/renderD*
/usr/local/musa/bin/musaInfo                 # expects: 1 MUSA capable device
/usr/local/musa/bin/muMemcpyTest             # expects: device 0 SUCCESS
mthreads-gmi -q                             # expects: MTT S30, 4096 MiB
```

The functional GPU checks are the same ones used in the controlled load test:
`tests/musart-smoke`, `tests/musa-vector-add.mu` (`mp_10`),
`tests/musa-compute.mu`, `tests/musa-fp64.mu` and the Vulkan device/buffer/map
probe, driven by `scripts/hardware/load-and-test.sh`.

## Verifying that compute mode survives a reboot

The `enable` path is verified live (blacklist gone from `/etc/modprobe.d`
*and* from the regenerated initramfs, udev resolving the S30 to
`kmod load mtgpu`, the full MUSA workload passing with GNOME alive). The one
fact that can only be observed after a reboot is that **udev loads `mtgpu`
before GDM and GNOME still reaches the login screen**.

`scripts/compute-mode-boot-verify.sh` captures exactly that. It is read-only
(no `modprobe`, no `insmod`/`rmmod`, no unbind/rebind, no config change) and
writes its evidence to `/var/log/musa-fedora/compute-mode-boot-<UTC>.log`:

```sh
# interactive run (writes to ~/.musa-work by default output dir override)
scripts/compute-mode-boot-verify.sh --out-dir ~/.musa-work/boot-verify --wait 0
```

It proves: udev autoload + `initstate=live`, the S30 bound to `mtgpu` with a
render node, GDM/`graphical.target`/`gnome-shell` still up, no blacklist in
`/etc/modprobe.d` or `modprobe -c`, and **no `mtgpu` BUG/Oops/NULL-dereference
and no `Invalid cmd_type` in the current boot's kernel log**.

To capture the evidence automatically on the next boot, a one-shot systemd
unit runs the script at `graphical.target` and then disables and removes
itself:

```sh
pkexec scripts/install-boot-verify.sh install     # arm the one-shot
# ... reboot ...
cat /var/log/musa-fedora/compute-mode-boot-latest.log
pkexec scripts/install-boot-verify.sh status       # confirm it fired and left
```

The unit is skipped (`ConditionPathExists`) while a boot-time block is
present, and `install` refuses to arm it if `mtgpu` is still blacklisted.
`install-boot-verify.sh uninstall [--purge-logs]` removes it.

If the post-boot log shows FAIL, or GNOME does not reach the login screen, the
display stays on `i915`; log in on a console and restore the previous state
with `pkexec scripts/compute-mode.sh disable && pkexec systemctl reboot`.

## Safety notes

- The driver is never unloaded and the PCI device is never unbound or
  rebound; a reboot is the only way back to a clean state.
- Every file that is touched is copied into a timestamped backup directory
  first, and `disable` restores it.
- Nothing in this workflow disables a security mechanism: IBT/Secure Boot,
  SELinux (still enforcing) and the IOMMU configuration
  (`intel_iommu=on iommu=pt`) are untouched.
- `install musa-runtime-display` on a host whose desktop runs on that same
  GPU restores the crash condition. `compute-mode.sh enable` refuses to run
  in that case.

## Known limitations

- The display path (`display=mt`, i.e. driving the S30's own DP/HDMI
  outputs) is **not** validated on this host. The desktop is driven by Intel
  i915, and the S30's connectors are never exercised.
- Moore Threads OpenCL is **not** available: the 1.5.4 runtime ships no MT
  OpenCL ICD, and the archived 2.1.1 ICD triggered a kernel Oops during
  compute ([`current-state-2026-11-17.md`](current-state-2026-11-17.md)).
- The MUSA 5.2.0 *runtime* is ABI-incompatible with the 3.1.0 driver. Only
  the 1.5.4 runtime is packaged; the 5.2.0 `mcc` compiler is fine, because its
  output runs against the 1.5.4 runtime.

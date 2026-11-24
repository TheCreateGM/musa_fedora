# Session 2026-11-24: runtime 1.5.4-5 environment fix, COPR state, hardware re-validation

## 1. Environment fix (`musa-runtime` 1.5.4-5)

The host carried an unowned legacy `/etc/profile.d/musa.sh` that exported
`MUSA_HOME=/usr/local/musa-5.2` and put `/usr/local/musa-5.2/bin` and
`/usr/local/musa-5.2/lib` **ahead** of the 1.5.4 runtime. Login shells then
resolved `musaInfo` to the 5.2.0 tool, which links `libmusart.so.5` and
aborts against the 3.1.0 driver (`Invalid cmd_type`, NULL dereference path):
the already-documented ABI mismatch.

`musa-runtime` release 5 adds `/etc/profile.d/zz-musa-fedora.sh`
(`%config(noreplace)`), which sorts after the legacy file and:

* reorders `PATH` and `LD_LIBRARY_PATH` so `/usr/local/musa` comes before
  `/usr/local/musa-5.2`;
* exports `MUSA_RUNTIME_HOME=/usr/local/musa` and keeps
  `MUSA_HOME=/usr/local/musa-5.2` for the compiler;
* removes the helper variables/function it used, so it is idempotent.

Verified on the host with `bash -lc` + `scripts/check-musa-env.sh`:

```
MUSA_ENV_STATUS: PASS
musaInfo  -> /usr/local/musa/bin/musaInfo  (libmusart.so.1.5)
mcc       -> /usr/local/musa-5.2/bin/mcc
LD_LIBRARY_PATH=/usr/local/musa/lib:/usr/local/musa-5.2/lib
/etc/profile.d/zz-musa-fedora.sh owned by musa-runtime-1.5.4-5
```

The check still warns about the unmanaged `musa.sh`, which the packaged
script overrides; `--quarantine-orphans --plan` is available for an explicit
cleanup and was not applied.

## 2. COPR

`musa-runtime` 1.5.4-5 (and the `musa-runtime-display` subpackage) built and
published in both projects; `scripts/verify-copr-chroots.sh` then reported
**passed: 20, failed: 0** across all nine chroots:

| Project | Chroots |
|---------|---------|
| `axogm/musa-fedora` | fedora-44, fedora-45, eln, rawhide (x86_64) |
| `axogm/musa-mtt-s30` | fedora-43, fedora-44, fedora-45, eln, rawhide (x86_64) |

Latest published set: `musa-runtime` 1.5.4-5, `musa-runtime-display` 1.5.4-5,
`musa-sdk` 5.2.0-6, `musa-toolkit` 3.1.0-3, `mtgpu-dkms` 3.1.0-12,
`mtgpu-firmware` 3.1.0-4.

## 3. Hardware re-validation

A single controlled `insmod` (documented headless arguments) followed by the
gated suite on the live MTT S30. Full record:
[`hardware-validation-2026-11-24-load-test.md`](hardware-validation-2026-11-24-load-test.md).
Result: `verify-musa.sh` **12 passed / 1 failed / 1 skipped** at 01:43 UTC;
the failure is the vendor telemetry tool, not the compute path. A second run at
02:05 UTC, after the `libdrm_mtgpu.so.1` link was restored, reported
**13 passed / 1 failed / 1 skipped** (the DRM-library check now passes); the
remaining failure is still `mthreads-gmi`. No kernel fault, GNOME/GDM survived.
Evidence under `~/.musa-work/load-test-20261124T014358Z/`.

## 4. Two findings from the hardware run

1. **`mthreads-gmi` 2.0.0 cannot initialize mtml.** The tool opens
   `/dev/mtgpu.0` and `/dev/dri/renderD130` and gets `EINVAL` from DRM ioctl
   `0x64/0x8a` (40-byte argument) while `0x8b`/`0x8c` succeed. Root cause and
   evidence:
   [`root-cause-mthreads-gmi-drm-query-2026-11-24.md`](root-cause-mthreads-gmi-drm-query-2026-11-24.md).
   Telemetry is unavailable; compute, memory, `mcc` and Vulkan are unaffected.
2. **A 5.2.0 perf shim hijacked `libdrm_mtgpu.so.1`.** An unowned
   `/usr/lib64/libmtperf_target.so` (from `libdrm-mtgpu_5.2.0_amd64.deb`)
   advertises the same SONAME as the packaged 3.1.0 library, so `ldconfig`
   repointed the link. Quarantined to
   `~/.musa-work/quarantine/2026-11-24-5.2-drm-shim/`; link restored,
   `rpm -V musa-runtime` clean. Details:
   [`root-cause-drm-soname-hijack-2026-11-24.md`](root-cause-drm-soname-hijack-2026-11-24.md).

## 5. Script changes

* `scripts/verify-musa.sh`: new **DRM userspace library** check (link target +
  foreign-SONAME scan), and the `mthreads-gmi` failure now prints the error
  and points at the root-cause document.
* `scripts/hardware/load-and-test.sh`: fixed duplicated log lines
  (`exec >>` + `tee`) and the wrong staging count; console progress now goes
  to fd 3, the log file is written once.
* `scripts/check-musa-env.sh`: new login-environment validator
  (`--quarantine-orphans --plan` for the legacy `musa.sh`).

## 6. State at end of the controlled load

The module was left loaded for evidence collection and, at that point, the
boot-time blacklist was unchanged, so a reboot would have restored the safe
display-only state. Section 7 records the subsequent decision to enable
compute mode permanently on this host.

## 7. Compute mode enabled (02:08 UTC)

After the hardware run and the package/COPR checks, `scripts/compute-mode.sh
enable` was executed. Preflight passed (no `musa-runtime-display`, module
present for the running kernel, S30 on the bus, dracut available), the whole
`/etc/modprobe.d` set and the previous initramfs were backed up to
`/var/backups/musa-fedora-compute-mode/20261124T020848Z/`, the blacklist was
moved into the backup, and the initramfs was regenerated. The module was
already live, so it was not reloaded.

Verification (no reboot):

* no `blacklist mtgpu` in `/etc/modprobe.d` or in the regenerated initramfs;
* `modprobe -c` has no `blacklist mtgpu`;
* `udevadm test --action=add /sys/bus/pci/devices/0000:01:00.0` resolves the
device to `kmod load` with `DRIVER=mtgpu`;
* the full MUSA suite still passes with GNOME alive (section 3);
* the S30 DRM card is `card2` with a `Virtual-1` connector; `/dev/mtgpu.0`
  and `/dev/dri/renderD130` are present.

The one thing that still needs a reboot to observe is udev loading `mtgpu`
before GDM with no vendor DRI files present. Both halves are independently
verified (udev autoload happened on the 2026-11-23 boot; the live module with
no DRI files coexists with GNOME in the controlled loads), and the exact
post-reboot checks plus the `disable` rollback are in
[`compute-mode-enabled-2026-11-24.md`](compute-mode-enabled-2026-11-24.md).

At the end of this session the host is left **compute-active**: `mtgpu` live,
blacklist removed from `/etc/modprobe.d` and the initramfs, full backup on
disk.
# Driver loading and recovery status

## Current host state — 2026-11-20

The MTT S30 (`1ed5:0102`, `0000:01:00.0`) is present but unbound on Fedora 45
kernel `7.3.4-300.fc45.x86_64`. `/sys/module/mtgpu` and `/dev/mtgpu.0` are
absent. `/etc/modprobe.d/blacklist-mtgpu.conf` intentionally blacklists the
module because prior driver loads caused DRM/KMS conflicts and GNOME crashes.
The Intel UHD 770 (`i915`) drives the desktop. No S30 render node exists;
`renderD129` belongs to virtual MVGAL. IOMMU remains enabled
(`intel_iommu=on iommu=pt`). See the
[2026-11-20 live inventory](current-state-2026-11-20.md).

The runtime and compute checks below were recorded on 2026-11-17 while the
module was already loaded. They are historical and are not current-host tests.
Do not remove the blacklist or load/unbind/rebind the driver without a reviewed
display rollback and vendor-core recovery plan.

Historical tests recorded on 2026-11-17 showed that `muMemcpyTest`, MUSA
1.5.4 `musaInfo`, the host runtime API (`tests/musart-smoke.c`), an `mp_10`
vector-add and compute suite, and a limited headless Vulkan buffer/memory probe
passed while the S30 driver was live. The recorded
`scripts/verify-musa.sh` result was **13 passed, 0 failed, 1 skipped**; Moore
Threads OpenCL was absent. None of these tests was repeated in the current
unbound state.

A separate archived OpenCL ICD compute probe caused a kernel Oops in `mtgpu`.
The previous boot's module remained loaded and bound after the Oops; that is not
the current state. Do not rerun the archived ICD probe; see the
[2026-11-17 incident record](current-state-2026-11-17.md).

Because loading the module has caused display and kernel faults, do **not**
install over it, unbind, or rebind the device. An unbind/rebind can fault inside
`mtgpu_pstate_notifier_call_chain` (see
[root-cause-rebind-2026-11-16.md](root-cause-rebind-2026-11-16.md)).

## Historical quarantine and recovery

The 2026-11-12 quarantine action targeted a then-unbound host and a
hash-pinned module file. It is historical and does not authorize restoring or
loading that file. The script refuses to overwrite a different file at the
target; its `--plan` mode is read-only.

The exact command and inverse remain in the
[2026-11-12 historical report](live-host-recheck-2026-11-12.md). Do not
remove the current blacklist or load/unbind/rebind/replace the module without
a reviewed display rollback and vendor-core recovery plan.

## Incomplete binder in an older kernel tree

The inactive `7.2.15-300.fc45.x86_64` tree contains an unowned,
project-authored `mtgpu_bind.ko` version 1.2.0. Its source only binds the S30
PCI function and creates a character device; its ioctl handler returns
`ENOTTY` until another module replaces it. It is not a MUSA GPU driver. An
unowned `/etc/modprobe.d/musa.conf` aliases S30 PCI IDs to it, so booting that
kernel could load the incomplete binder. The current default remains 7.3.3,
which has no matching on-disk module. The binder and global alias have been
quarantined under `musa-data`; `lspci` no longer resolves an MTGPU candidate.

The
[`quarantine-experimental-mtgpu-binder.sh`](../scripts/quarantine-experimental-mtgpu-binder.sh)
has backed up both exact files under `musa-data` and removed them reversibly.
Its `--plan` mode is read-only. The inverse is:

```sh
pkexec /usr/bin/bash /home/axogm/Documents/project/musa-fedora/scripts/quarantine-experimental-mtgpu-binder.sh --restore
```

The quarantine did not change any initramfs or boot entry and did not remove
the separate `sgpu_km` modules. Restoring the binder reactivates its alias and
may autoload it when kernel 7.2.15-300 is booted.

## Load approval boundary

A 3.1.0 DKMS module is installed for the current kernel, but it is not loaded;
the S30 remains unbound and the module blacklist is active.
The isolated fresh 7.3.3 build in `musa-data` emitted 5,504 compiler/objtool
warning lines and is not approved for installation as a replacement. Do not
install over the live module, and do not disable IBT/CET, patch vermagic,
alter vendor-core metadata, or load the 7.2.15 binder/compute modules on
kernel 7.3.3. An unbind/rebind of the live device is not approved.

The gated [`test-gpu.sh`](../scripts/test-gpu.sh) currently blocks because the
S30 is unbound, no S30 render node exists, and explicit test approval is absent.
It does not load or unload modules. A successful compile, PCI ID, device node,
or firmware file is not by itself a hardware test.

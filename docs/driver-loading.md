# Driver loading and recovery status

## Current host state — 2026-11-17

The MTT S30 (`1ed5:0102`, `0000:01:00.0`) is bound to the vendor `mtgpu`
3.1.0 driver on Fedora 45 kernel `7.3.3-300.fc45.x86_64`. The module is loaded
and live (`/sys/module/mtgpu/initstate=live`) from
`/lib/modules/7.3.3-300.fc45.x86_64/extra/mtgpu.ko`. Both `/dev/mtgpu.0` and
the S30 DRM render node `/dev/dri/renderD129` are present. IOMMU is enabled
(`intel_iommu=on iommu=pt`); the S30 is in IOMMU group 11. The Intel UHD 770
(`i915`) remains the active display GPU; the S30 is compute-only here.

The vendor `muMemcpyTest` passes, MUSA 1.5.4 `musaInfo` detects one device,
the host runtime API (`tests/musart-smoke.c`) passes, and a 5.2.0 `mcc`
`mp_10` vector-add ran on the S30. `scripts/verify-musa.sh` reports **11
passed, 0 failed, 0 skipped**.

Because the module is live, do **not** install over it, unbind, or rebind the
device. An unbind/rebind can fault inside
`mtgpu_pstate_notifier_call_chain` (see
[root-cause-rebind-2026-11-16.md](root-cause-rebind-2026-11-16.md)).

## Historical quarantine and recovery

The 2026-11-12 quarantine action targeted a then-unbound host and a
hash-pinned module file. That host state is historical and is superseded by
the live state above; do not apply that restore action against the current
live stack. The script refuses to overwrite a different file at the target,
but the `--restore` action is unnecessary and not approved here. Its `--plan`
mode is read-only.

The exact command and inverse remain in the
[2026-11-12 historical report](live-host-recheck-2026-11-12.md). Do not
remove or replace the currently loaded module during this boot.

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

A 3.1.0 module is now installed and live on this host; the stack is running.
The isolated fresh 7.3.3 build in `musa-data` emitted 5,504 compiler/objtool
warning lines and is not approved for installation as a replacement. Do not
install over the live module, and do not disable IBT/CET, patch vermagic,
alter vendor-core metadata, or load the 7.2.15 binder/compute modules on
kernel 7.3.3. An unbind/rebind of the live device is not approved.

The gated [`test-gpu.sh`](../scripts/test-gpu.sh) blocks until a full driver
is bound, initialized, has an S30 render node, and receives explicit test
approval. It does not load or unload modules. A successful compile, PCI ID,
device node, or firmware file is not by itself a hardware test.

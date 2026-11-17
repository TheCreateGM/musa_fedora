# Driver loading and recovery status

## Current host state — 2026-11-13

The MTT S30 (`1ed5:0102`) is visible and unbound on Fedora 45 kernel
`7.3.3-300.fc45.x86_64`. The Intel UHD 770 (`i915`) remains the active
display GPU. No `/dev/mtgpu.0` node or S30-associated DRM render node exists.

An unowned `mtgpu.ko` was loaded at boot and failed with `Missing ENDBR:
rmapi_alloc`, followed by a kernel control-protection fault. The resident
module remains in `initstate=coming`, with taint `OE`; the kernel is tainted.
An unowned module file is currently present in the 7.3.3 tree, but its hash
differs from the file recorded as quarantined on 2026-11-12. Its origin is
unknown. Do not access the GPU, query the runtime, or try to unload the
module.

The [latest live report](live-host-recheck-2026-11-13.md) has the current
hash and state. The earlier quarantine and its restore command are documented
in the [2026-11-12 report](live-host-recheck-2026-11-12.md); do not apply
that restore action based on the historical state alone.

## Historical quarantine and recovery

The 2026-11-12 quarantine action targeted a different, hash-pinned module.
The current file has a different hash, so do not run the historical restore
command against the current host state. The script refuses to overwrite a
different file at the target, but the `--restore` action is unnecessary and
not approved here. Its `--plan` mode is read-only.

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

The latest fresh 7.3.3 build linked, but emitted 5,504 compiler/objtool
warning lines. The build is not approved for installation or load.
Do not disable IBT/CET, patch vermagic, alter vendor-core metadata, or load
the 7.2.15 binder/compute modules on kernel 7.3.3.

The gated [`test-gpu.sh`](../scripts/test-gpu.sh) blocks until a full driver
is bound, initialized, has an S30 render node, and receives explicit test
approval. It does not load or unload modules. A successful compile, PCI ID,
device node, or firmware file is not a hardware test.

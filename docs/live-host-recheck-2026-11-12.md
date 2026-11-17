# Live host recheck — 2026-11-12

This report supersedes earlier present-tense host descriptions recorded while
kernel `7.2.15-300.fc45.x86_64` was running. The live host is Fedora 45 on
kernel `7.3.3-300.fc45.x86_64`.

## Hardware and system state

| Item | Live observation |
|---|---|
| OS | Fedora Linux 45 Workstation, x86_64 |
| Kernel | `7.3.3-300.fc45.x86_64`; matching kernel-devel tree is installed |
| Other installed kernels | `7.2.15-200.fc44`, `7.2.15-300.fc45`, and the running `7.3.3-300.fc45`; all installed configs set `CONFIG_X86_KERNEL_IBT=y` |
| CPU | Intel Core i5-13500; Intel UHD Graphics 770 is driven by `i915` |
| MTT card | MTT S30 at `0000:01:00.0`, vendor/device `1ed5:0102` |
| PCIe link and BARs | Negotiated 8 GT/s x8; 32 MiB non-prefetchable BAR0 and 4 GiB 64-bit prefetchable BAR2 mapped above 4 GiB; Resizable BAR capability reports 4 GiB current size |
| S30 PCI driver | Unbound; no sysfs `driver` symlink |
| MTT audio function | `0000:01:00.1`, unbound |
| DRM | No render node is associated with the S30; Intel has `/dev/dri/renderD128` |
| MTT device node | `/dev/mtgpu.0` is absent |
| Secure Boot | Disabled |
| SELinux | Enforcing |
| VM passthrough | QEMU/KVM is installed and `/dev/kvm` exists, but neither S30 PCI function has an IOMMU group; safe S30 passthrough is unavailable in the current boot |
| Kernel command line | No `ibt=off` or `noibt` override; the kernel IBT control-protection fault below occurred in this boot |

## Firmware files on the host

The four files in `/usr/lib/firmware/mthreads/` are not owned by any Fedora
RPM. Their SHA-256 hashes match the files with the same names in the archived
official `musa_2.1.1-Ubuntu-dev_amd64.deb` package exactly:

| Firmware file | SHA-256 |
|---|---|
| `musa.fw.1.0.0.0` | `ada58e43ed82f0314060ea8a24654549aa10000d18e2aefbb635821808f69440` |
| `musa.fw.1.1.0.0` | `9b9c6dbb57ab5102230279e64618520fd9abdac02155213610043ea599243b38` |
| `musa.fw.2.0.0.0` | `b8f1ea7aa7a391308e5386904e32ba612c11714dd209a6c33125f948646f14f3` |
| `musa.sh.1.0.0.0` | `332b9c1b79cba9efa3094e810777daf35c7ed6cb0e1b1af419594a4ebe4398d9` |

This ties the files to the older Ubuntu 20.04 desktop bundle, not to an
RPM-owned Fedora driver installation. They were not modified. Their presence
does not remedy the resident module's IBT fault or qualify a current MUSA
driver/runtime combination.

## Unsafe module already present in this boot

At boot, `/lib/modules/7.3.3-300.fc45.x86_64/extra/mtgpu.ko` was loaded. It
has no RPM owner, and DKMS lists no MTGPU package. The module remains in
`initstate=coming`; its sysfs taint is `OE` and `/proc/sys/kernel/tainted`
contains `12480`. The S30 is still unbound. No `insmod`, `modprobe`, `rmmod`,
or GPU-device operation was issued by this project; the on-disk copy was
later moved to a hash-verified backup without touching the resident module.

The boot journal records `Missing ENDBR: rmapi_alloc`, followed by a kernel
control-protection exception and `kernel BUG at arch/x86/kernel/cet.c:133!`
during `mtgpu_driver_init`. The active Intel display is provided by `i915`.
The current initramfs contains no `mtgpu.ko`, so the failing file is supplied
from the root filesystem's kernel module tree.

The 7.3.3 module file is now absent from that kernel's module tree, and a
fresh read confirms there is no `driver` symlink on the S30 PCI function. The
only other installed kernels are 7.2.15, and their configs also enable kernel
IBT. None of the installed kernels is within the 4.19–5.15 range in the
official S30 desktop driver v2.5.0 notes; see
[vendor availability](vendor-sdk-availability.md). A reboot would clear the
resident failed module but would not provide an installed, vendor-supported
kernel for hardware validation.

## Additional module-tree and boot-entry audit (2026-11-12)

A read-only scan found these unowned files outside the running kernel tree:

| Kernel tree | Artifact | SHA-256 | Observation |
|---|---|---|---|
| `7.2.15-300.fc45.x86_64` | `extra/mtgpu_bind.ko` | `65415c54b87d5d1f2ab4bb3f9315fa71c3f3e7709fbb5bc98fa216527df5fa85` | Project-authored version 1.2.0 PCI/character-device binder; incomplete and not load-approved |
| `7.2.15-300.fc45.x86_64` | `extra/sgpu_km.ko` | `c23b451215b33e8ce5b7da9dfd618af17eac48b6a9785e1301414f133f3b9c26` | Unowned MooreThreads-labeled module; not loaded or validated |
| `7.2.13-200.fc44.x86_64` | `extra/mtgpu_stub.ko` | `4bf18bcecd014fd455f58ba8cb6c947db14a754be3bc7e93b08f5a4c5cf41b35` | Stub-labeled module in a tree with no installed matching kernel RPM or kernel image |
| `7.2.13-200.fc44.x86_64` | `extra/sgpu_km.ko` | `38fde5cae1247570f0289987aa605ce1a8ff5ca9788ce9c87ccc2f69535c8861` | Unowned MooreThreads-labeled module in the same orphaned tree; not loaded or validated |

`/etc/modprobe.d/musa.conf` is also unowned and maps both S30 PCI IDs to
`mtgpu_bind`; its hash is
`abead3eeb640844a67e87cabdfdf67024074f82de82d7f2302192b22c6203f75`. It
causes `lspci` to list `mtgpu_bind` as a candidate even though that module is
absent from the running 7.3.3 tree. The 7.2.15-300 tree does contain this
project-authored binder, so selecting that kernel could load it through the
alias. Its source only creates a PCI binding and character device; its
default ioctl handler returns `ENOTTY`. It is not a MUSA GPU driver and is
not approved for use.

An elevated, read-only `lsinitrd` scan found no `mtgpu`, `mtgpu_bind`,
`mtgpu_stub`, or `sgpu_km` module path in any of the three installed kernels'
initramfs images. `grubby` reports 7.3.3 as the default kernel. It also lists
a stale custom entry titled `Fedora 7.2.13 MTT S30 TEST (no blacklist, no
rhgb)` pointing to `/boot/vmlinuz-7.2.13-200.fc44.x86_64`; that kernel image
and its initramfs are absent. The boot entry was not changed.

The reversible
[`quarantine-experimental-mtgpu-binder.sh`](../scripts/quarantine-experimental-mtgpu-binder.sh)
backed up the exact project binder and stale modprobe file under
`musa-data/research/system-backups/experimental-mtgpu-binder-7.2.15-300.fc45.x86_64-2026-11-12/`
and removed only those two files through `pkexec`. Both backup hashes match
the originals; `depmod` completed. The binder and alias are now absent from
the 7.2.15-300 tree, and `lspci` no longer reports an MTGPU driver candidate.
No module was loaded or unloaded, and no initramfs or boot entry was changed.
The exact inverse is:

```sh
pkexec /usr/bin/bash /home/axogm/Documents/project/musa-fedora/scripts/quarantine-experimental-mtgpu-binder.sh --restore
```

The `sgpu_km` files and stale 7.2.13 boot entry are left untouched pending
provenance review.

The loaded file is 29,696,368 bytes with SHA-256
`b59c446472d7f1b7946efb4d559054b2c2124fa3e4c2a5fa1eab3355ec2f8ddb` and
`build_version=3df6cad-dirty`. Its hash does not match the preserved driver
build artifacts under `musa-data/research`; its origin is unverified. Do not
open a GPU node, query the runtime, load another MTT module, or attempt to
unload this module in the current boot.

## Current isolated build and compiler evidence

The fresh build script copied source into
`/home/axogm/Documents/project/musa-data/research/builds/mtgpu-7.3.3-300.fc45.x86_64-ZEbZhc/`
and linked `mtgpu.ko` for the running kernel. The artifact is 29,712,192
bytes, has matching 7.3.3 vermagic, and carries the S30 PCI alias. This is
compile/link evidence only. Objtool emitted 7,451 warning lines, including
1,837 ENDBR-related warnings and four `missing int3 after ret` warnings. The
artifact remains uninstalled and unloaded.

The staged, official MUSA 5.2.0 compiler passed a compile-only vector-add
check and an explicit `mp_10` build. Runtime headers passed a syntax check.
The generated programs were not executed, and this pass made no MUSA runtime
query. Moore Threads' product filing identifies the S30 as SuDi, and the
official MUSA architecture guide maps SuDi to `MP_10`; the compiler manual
lists `mp_10` as an offload target. This supports `mp_10` as the S30
code-generation target, but does not establish that the MUSA 5.2.0 runtime
supports or executes on S30.

The full read-only inventory exited 3 and reported `mtgpu=coming`, the S30
unbound, and no MTT device node. The gated GPU script also exited 3 before
compiling or running a GPU executable. It reported no S30 binding, incomplete
module initialization, and no S30 render node. No hardware test ran.

Detailed transcripts and build logs are kept only under
`/home/axogm/Documents/project/musa-data/research/musa-fedora-live-validation-2026-11-12/`
and the corresponding fresh build directory. They are research evidence, not
production artifacts.

## Vendor support boundary

The current official [MUSA SDK catalog](https://developer.mthreads.com/resources/musa-sdk?tab=overview)
lists its 5.2.0 Linux DEB and RPM downloads for MTT S5000; it does not list
S30 as a 5.2.0 package target. Moore Threads describes S30 as first-generation
MUSA in its [S30 product page](https://www.mthreads.com/product/S30), and its
[S30 product documentation](https://docs.mthreads.com/s30/s30-doc-online/introduction/)
lists several operating systems but not Fedora. The staged 5.2.0 driver
source recognizes PCI device ID `0x0102`; that source-table entry does not
establish a working Fedora driver or 5.2.0 runtime support for S30.

The official product/architecture/compiler source chain supports the S30
code-generation target `mp_10`; it is not a hardware compatibility result.
See the [dated target evidence record](../../musa-data/research/official-musa-5.2.0/s30-mp10-target-evidence-2026-11-12.md).

## Reversible next-boot recovery action

The prepared [quarantine script](../scripts/quarantine-unvalidated-mtgpu.sh)
accepts only the exact unowned module hash recorded above. Its `--plan` mode
is read-only. `--quarantine` requires `pkexec`, verifies that the S30 is
unbound and the failed module is still in `coming`, saves a hash-verified
copy under `musa-data/research/system-backups/`, removes only the on-disk
copy from this kernel's `extra/` directory, and reruns `depmod`. It does not
unload the module resident in this boot, alter the initramfs, change a boot
entry, or access the GPU.

At 11:17 local time on 2026-11-12, the quarantine action completed through
`pkexec`. It saved the module at
`/home/axogm/Documents/project/musa-data/research/system-backups/mtgpu-unvalidated-7.3.3-300.fc45.x86_64-2026-11-12/mtgpu.ko`;
the backup hash matches the expected SHA-256. The `.ko` file is now absent
from the running kernel's module tree, and `depmod` completed. The custom
modprobe alias still resolves to `mtgpu_bind`, which is absent from the
running kernel tree; it no longer resolves to `mtgpu`. The resident module
still reports `initstate=coming` and taint `OE`. It was not unloaded and the
host was not rebooted.

After the action, the read-only inventory and gated GPU script both exited 3.
The inventory confirmed the module file is absent, the S30 remains unbound,
and no `/dev/mtgpu.0` node exists. The GPU script blocked before any runtime
query or application build. Their transcripts are under the live-validation
directory listed above.

The exact inverse is:

```sh
pkexec /usr/bin/bash /home/axogm/Documents/project/musa-fedora/scripts/quarantine-unvalidated-mtgpu.sh --restore
```

That restores the verified module file, reapplies the Fedora SELinux label,
and regenerates the module indexes. A reboot is still required to clear the
failed module initialization from memory. The quarantine action prevents this
specific file from being found through the current kernel's module indexes
after reboot; it does not make the driver safe or install MUSA support.

No hardware validation is approved until Moore Threads supplies an IBT-safe
driver core or complete source, the kernel safety warnings are resolved, and
a separate recovery-reviewed test plan is established. QEMU/KVM is installed,
but the S30 has no IOMMU group in this boot, so VM PCI passthrough is not
currently available. No VM was created or used for device testing.

# Live host inventory and verification record

Latest live recheck: 2026-11-20. Fedora is not a vendor-qualified S30 OS.
The complete current record is
[here](current-state-2026-11-20.md). The 2026-11-17 loaded-driver report below
is retained as historical hardware-test evidence only.

## Latest live state — 2026-11-20

| Item | Observed |
|---|---|
| Running kernel | `7.3.4-300.fc45.x86_64` |
| Active command line | `intel_iommu=on iommu=pt`; no `ibt=off` or `noibt` override |
| Accelerator | Moore Threads MTT S30, PCI `1ed5:0102`, BDF `0000:01:00.0`; audio function `1ed5:01ff` |
| S30 PCI binding | Unbound; no PCI `driver` symlink |
| Driver/module | DKMS 3.1.0 installed for this kernel; `mtgpu` not loaded; `/etc/modprobe.d/blacklist-mtgpu.conf` intentionally blocks automatic loading after prior GNOME/KMS crashes |
| IOMMU | Enabled by kernel command line; device groups were not re-enumerated in this snapshot |
| MTT device nodes | `/dev/mtgpu.0` absent; no DRM render node is associated with the S30 (`renderD129` is virtual MVGAL) |
| Active display | Intel UHD Graphics 770 (`i915`) |
| Firmware/userspace | Firmware and MUSA runtime/SDK RPMs installed; no runtime API was called |
| GPU tests | Not run; read-only hardware checker and gated GPU test both report NOT RUN/BLOCKED |
| Security | Secure Boot disabled; kernel IBT enabled |

No package, boot, module, blacklist, firmware, or display configuration was
changed. The prior 2026-11-17 table follows as a dated historical state.

## Historical live state — 2026-11-17 (superseded)

| Item | Observed |
|---|---|
| Running kernel | `7.3.3-300.fc45.x86_64` |
| Active command line | `intel_iommu=on iommu=pt`; no `ibt=off` or `noibt` override |
| Accelerator | Moore Threads MTT S30, PCI `1ed5:0102`, BDF `0000:01:00.0`, 4 GiB; audio function `1ed5:01ff` |
| S30 PCI binding | Bound to vendor `mtgpu` 3.1.0; `initstate=live` |
| Installed driver module | `/lib/modules/7.3.3-300.fc45.x86_64/extra/mtgpu.ko` (vermagic `7.3.3-300.fc45.x86_64`) |
| IOMMU | Enabled; 13 IOMMU groups; S30 in group 11 |
| MTT device nodes | `/dev/mtgpu.0` present; S30 DRM render node `/dev/dri/renderD129` present |
| Active display | Intel UHD Graphics 770 (`i915`); S30 does not drive display output; its headless Vulkan device/buffer/memory path was exercised |
| Firmware | Installed under `/lib/firmware/mthreads/` |
| Runtime | MUSA runtime 1.5.4 at `/usr/local/musa/` (`libmusart.so.1.5.4`); SDK 5.2.0 compiler/headers at `/usr/local/musa-5.2/` |
| Telemetry | `mthreads-gmi` reported driver 3.1.0, MTT S30, 4096 MiB (2026-11-17 snapshot); **as of 2026-11-24 the tool fails with `EINVAL` on DRM ioctl `0x64/0x8a`** ([root cause](root-cause-mthreads-gmi-drm-query-2026-11-24.md)). MUSA 1.5.4 `musaInfo` still detects one device |
| Hardware tests | `muMemcpyTest`, host runtime API, vector-add, eight `mp_10` compute checks, and headless Vulkan device/buffer/memory probe passed before the later OpenCL Oops; Moore Threads OpenCL ICD absent |
| Verifier | Recorded pre-Oops `scripts/verify-musa.sh`: **13 passed, 0 failed, 1 skipped**; no further GPU tests were run in that boot |
| Security and boot | Secure Boot disabled; SELinux enforcing |

The supported combination on this host is driver `mtgpu` 3.1.0 + MUSA runtime
1.5.4 + MUSA 5.2.0 compiler targeting `mp_10`. The separate 5.2.0 *runtime*
is not compatible with the 3.1.0 driver ABI and must not be substituted for
1.5.4.

Full provider detail is in [current-state-2026-11-17.md](current-state-2026-11-17.md).

## Historical snapshot — 2026-11-13 (superseded)

On 2026-11-13 the S30 was unbound, the read-only verifier and GPU test gate
both exited 3, and no MUSA runtime query or GPU device operation was
performed. The verifier reported that the S30 was unbound and `mtgpu` remained
in `coming`; the gate stopped before runtime discovery or GPU access. That
unbound/`coming` state is historical and is superseded by the live table
above. See the [2026-11-13 live host report](live-host-recheck-2026-11-13.md).

The 2026-11-12 compiler output and isolated build logs are under
`/home/axogm/Documents/project/musa-data/research/musa-fedora-live-validation-2026-11-12/`
and its dated build directory. The 2026-11-12 build artifact remains
uninstalled and unloaded; it is not the module now live on this host. See the
[historical live host report](live-host-recheck-2026-11-12.md) and
[compatibility assessment](compatibility.md).

## Earlier snapshots and events (superseded)

The following entries record earlier checks and incidents. Their host-state
statements are historical and are superseded by the latest live table above.

## Hardware and operating system

| Item | Observed |
|---|---|
| OS | Fedora Linux 45 Workstation, x86_64 |
| Running kernel | `7.3.3-300.fc45.x86_64` |
| Installed kernel development tree | Matching `kernel-devel` and headers for the running kernel are present |
| Accelerator | Moore Threads MTT S30, PCI `1ed5:0102`, BDF `0000:01:00.0` |
| PCIe resources | 8 GT/s x8; 4 GiB prefetchable BAR allocated above 4 GiB; Resizable BAR reports 4 GiB current size |
| S30 binding | Unbound; PCI sysfs has no `driver` symlink |
| S30 device node | `/dev/mtgpu.0` is absent |
| Active display GPU | Intel UHD Graphics 770 (`8086:4680`), driver `i915` |
| Moore Threads audio function | PCI `1ed5:01ff`; unbound |
| Secure Boot | Disabled; unchanged |
| Installed MUSA packages | No RPM-owned package matching MUSA, MThreads, or MTGPU was found |
| MUSA firmware files | Four unowned files under `/usr/lib/firmware/mthreads/`; exact matches for files in the archived official 2.1.1 Ubuntu DEB, not evidence of a working current driver |

At the initial inventory, the running kernel's `/lib/modules/7.3.3-300.fc45.x86_64/extra/` contained
`mvgal` and its backend modules, including `mvgal_mtt`; it does not contain
`mtgpu`, `mtgpu_bind`, `sgpu_km`, or `sgpu_xx`. The running module list includes
`mvgal` and `mvgal_ntsync`, not an MUSA driver. `mvgal` creates a virtual DRM
device; the boot log says it skipped the S30 because no native driver is bound.
This is not MUSA device support.

Older installed kernels include Fedora 44 `7.2.15-200.fc44` and Fedora 45
`7.2.15-300.fc45`. All installed kernel configs set
`CONFIG_X86_KERNEL_IBT=y`, and none lies in the 4.19–5.15 DKMS range documented
for official S30 desktop driver v2.5.0. Their presence does not make their
modules compatible with the current running kernel.

## Local SDK-looking content

`/usr/local/musa-5.2` contains MUSA 5.2.0-looking compiler and runtime files,
but is not RPM-owned and was not installed or validated by this project. Its
`mcc` path resolves to `clang-14`; `mcc_version` reports version 5.2.0. Files
include `musaInfo`, `libmusart.so.5.2.0`, and `libmtrtc.so.5.2.0`. The
environment sets `MUSA_HOME=/usr/local/musa-5.2` and a matching library path.
Neither compiler output nor these files establish a compatible S30 runtime.
The 2026-11-12 `musaInfo` query returned error 100, “no MUSA-capable device is
detected.”

Separately, official MUSA 5.2.0 packages and package-index evidence are staged
under `/home/axogm/Documents/project/musa-data/research/official-musa-5.2.0/`.
They were not installed from that directory. See
[vendor-sdk-availability.md](vendor-sdk-availability.md).

## Project artifacts and driver build evidence

On 2026-11-11, the production tree was compile/linked against the running
`7.3.3-300.fc45.x86_64` kernel. The resulting root `mtgpu.ko` has matching
vermagic and an alias for PCI ID `1ed5:0102`, but objtool emitted more than
7,000 warnings, including `!ENDBR` relocations and missing-`int3` warnings
from the opaque vendor core object. This build artifact is not installed and
must not be loaded. See [driver-porting.md](driver-porting.md) and the
archived build log under `musa-data/research/`.

The earlier 2026-11-10 production-tree build targeted kernel
`7.2.15-300.fc45.x86_64`; its result is historical and must not be used on
the current kernel.

An earlier 2026-11-10 boot with kernel 7.2.15 was documented with `mtgpu` and
`sgpu_km` failing to finish initialization after kernel IBT/ENDBR control
protection faults. Those modules are not loaded in the current boot, and the
current boot's journal does not show those faults. That historical failure
does not establish the cause or status of the present unbound device.

## Read-only verification

`./scripts/verify-musa.sh` completed its read-only checks and exited 3:

- The S30 is visible on PCI and unbound.
- No MTT-specific device node exists.
- No MUSA driver module was loaded or installed at the time of this initial
  check.
- GPU tests were not run.

The check does not load modules, open device nodes, issue ioctls, install files,
or change system configuration. No system files, boot settings, kernel
modules, firmware, or libraries were changed for this inventory.

## Subsequent unexpected module event (2026-11-11 13:49)

A later readiness check found
`/lib/modules/7.3.3-300.fc45.x86_64/extra/mtgpu.ko` and an `mtgpu` entry in
`/proc/modules` with `/sys/module/mtgpu/initstate=coming`. The PCI device
remained unbound and `/dev/mtgpu.0` remained absent. The system module file's
SHA-256 (`b59c446472d7f1b7946efb4d559054b2c2124fa3e4c2a5fa1eab3355ec2f8ddb`)
exactly matches the project-tree compile-only artifact. The kernel
journal reported `Missing ENDBR: rmapi_alloc` while loading that module and an
`OE+` module taint. No install or load command was issued in this session; the
actor and mechanism that placed and loaded the file are unknown. No module
removal, device access, or GPU test was attempted. Treat the live module as
unsafe and do not interact with it.

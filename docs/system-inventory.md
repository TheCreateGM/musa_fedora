# Live host inventory and verification record

Latest live recheck: 2026-11-13. This describes observed host state; it is
not vendor qualification or evidence of GPU execution. The complete current
record is [here](live-host-recheck-2026-11-13.md). Earlier host snapshots
below are historical.

## Latest live state

| Item | Observed |
|---|---|
| Running kernel | `7.3.3-300.fc45.x86_64` |
| Active command line | No `ibt=off` or `noibt` override; this boot logged an IBT control-protection fault |
| S30 PCI binding | Unbound; PCI ID `1ed5:0102` at `0000:01:00.0` |
| Loaded MTT modules | `mtgpu` remains loaded in `initstate=coming`, taint `OE`; the module was not unloaded |
| Module file | An unowned `mtgpu.ko` is present in the running tree; its hash differs from the quarantined file recorded on 2026-11-12 and its origin is unknown |
| Backup/quarantine | The 2026-11-12 quarantine action is historical; see the latest recheck before any recovery action |
| Active display | Intel UHD Graphics 770 (`i915`) |
| MTT device nodes | `/dev/mtgpu.0` absent; no S30-associated DRM render node |
| Compiler check | Staged official 5.2.0 compile-only check passed for the default bundle and explicit `mp_10`; official product/architecture documentation maps S30 → SuDi → `MP_10`, but runtime execution is unverified |
| Fresh module build | Isolated 7.3.3 module linked with matching vermagic and S30 alias; latest build logged 5,504 compiler/objtool warning lines |
| GPU test | Blocked before runtime or application build; no physical GPU operation was attempted |
| Security and boot | Secure Boot disabled; SELinux enforcing; initramfs and boot entries unchanged during this validation |

On 2026-11-13 the read-only verifier and GPU test gate both exited 3. The
verifier reported that the S30 is unbound and `mtgpu` remains in `coming`;
the gate stopped before runtime discovery or GPU access. A different
unowned `mtgpu.ko` is now present in the current kernel tree than the one
quarantined on 2026-11-12. No MUSA runtime query or GPU device operation was
performed. The earlier post-quarantine snapshot below is historical.

The 2026-11-12 compiler output and isolated build logs are under
`/home/axogm/Documents/project/musa-data/research/musa-fedora-live-validation-2026-11-12/`
and its dated build directory.
The 2026-11-12 build artifact remains uninstalled and unloaded. The module that failed
during boot has an unverified origin and does not match those preserved build
artifacts by hash. See the
[historical live host report](live-host-recheck-2026-11-12.md) and
[compatibility assessment](compatibility.md).

## Historical snapshots and events (superseded)

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

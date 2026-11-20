# MUSA on Fedora — live host status (2026-11-20)

This report records a read-only host inventory on 2026-11-20. It supersedes
claims that the S30 driver is currently loaded or that the GPU is presently
available for workloads. The earlier 2026-11-17 hardware tests remain historical
results; they were not repeated during this inventory.

## Host and devices

- Fedora Linux 45 Workstation, x86_64; kernel `7.3.4-300.fc45.x86_64`.
- Intel Core i5-13500; Intel UHD 770 (`i915`) is bound to PCI `0000:00:02.0`
and drives the current desktop session.
- Moore Threads MTT S30 is enumerated at `0000:01:00.0`, PCI ID `1ed5:0102`;
its HDMI/DP audio function is `0000:01:00.1`, ID `1ed5:01ff`.
- The S30 functions are **unbound**. `/sys/module/mtgpu` and `/dev/mtgpu.0`
are absent. No DRM render node belongs to the S30; `/dev/dri/renderD128` is
Intel, and `renderD129` belongs to the virtual MVGAL device.
- IOMMU remains enabled (`intel_iommu=on iommu=pt`); Secure Boot is disabled.
- Installed packages are `mtgpu-dkms-3.1.0-5.fc45`,
`mtgpu-firmware-3.1.0-2.fc45`, `musa-runtime-1.5.4-3.fc45`,
`musa-sdk-5.2.0-6.fc45`, and `musa-toolkit-3.1.0-1.fc45`. DKMS has a module
for the current kernel, but installation alone does not mean that it is loaded
or usable.
- Proprietary MUSA firmware files are present and the firmware package owns
them. The module metadata requests additional `mtfw-*` and `mtvpu-*` images;
the presence of firmware does not establish driver initialization.

## Why no driver/GPU test was attempted

`/etc/modprobe.d/blacklist-mtgpu.conf` contains `blacklist mtgpu`. Its comments
record the reason: the vendor driver registers a DRM card that collided with
the Intel display setup and caused GNOME Shell/Mutter KMS crashes. This safety
configuration was left unchanged. Historical project records also document a
kernel NULL dereference in vendor code during an OpenCL compute experiment and
a separate NULL dereference on PCI unbind/rebind. The vendor core is opaque,
and the built module emits extensive objtool/IBT warnings. Do not remove the
blacklist, load the module, or unbind/rebind the device as an ad-hoc test.

The read-only checker `scripts/test/check-hardware.sh` reported the S30 visible
and unbound, no MTT-specific nodes, and GPU test not run (exit 3). The gated
`scripts/test-gpu.sh` also reported `GPU_TEST_STATUS: NOT RUN` (exit 3): the
S30 is not bound to the full `mtgpu` module, has no associated render node, and
explicit test approval is absent. These are expected safety blocks, not
successful GPU tests. No MUSA runtime API, compiler-executed kernel, Vulkan,
OpenCL, driver load, package transaction, or GPU workload was run in this
inventory.

The current boot's kernel journal includes PCI enumeration and MVGAL's message
that it skipped the S30 because no native driver is bound. It contains no
`mtgpu` probe, firmware-load, or GPU-workload messages. Direct `dmesg` access is
restricted (`kernel.dmesg_restrict=1`); the journal was read through
`journalctl -k`. Coredump records from Nov 16–19 include prior MUSA/GNOME
failures, but they do not establish a current-boot crash. The current-boot
journal was not found to contain an `mtgpu` Oops.

## Driver, firmware, MUSA and upstream compatibility

The installed sources and Fedora packages are real Moore Threads components,
not placeholders. The S30 PCI ID appears in the vendor-derived `mtgpu` module
metadata and the firmware/runtime/compiler RPMs contain the proprietary MUSA
payloads. However, this host is presently blocked before driver probing by the
intentional blacklist and display stability history. The public vendor support
matrix reviewed in [`vendor-sdk-availability.md`](vendor-sdk-availability.md)
does not qualify this Intel + Fedora + S30 combination; MUSA 5.2.0 runtime is
not interchangeable with the driver-3.1.0-compatible 1.5.4 runtime.

Prior real-device tests on 2026-11-17 verified a limited set of operations
while the driver was already live: runtime discovery and copy tests, the MUSA
host API smoke test, an `mp_10` vector-add with 1,048,576 checked results,
selected compute operations, and a headless Vulkan buffer/memory test. Those
results are explicitly historical, do not authorize loading the driver now,
and do not establish other features or general workload compatibility. The
known OpenCL Oops and rebind fault remain safety blockers.

## RPM and Copr status

`rpmspec -P` succeeds for all five checked-in RPM specs. Existing ignored local
RPMs pass `rpm -K --nosignature`, but their NEVRAs are older than current specs;
those local files are not evidence that the checked-in current releases were
rebuilt locally in this session. The Copr API reports these latest intended
package builds as succeeded:

| Package | Version-release | Copr build | Result/chroots |
|---|---:|---:|---|
| `mtgpu-dkms` | `3.1.0-5` | [11339362](https://copr.fedorainfracloud.org/coprs/build/11339362) | succeeded: Fedora 44, 45, ELN, Rawhide x86_64 |
| `mtgpu-firmware` | `3.1.0-2` | [11339153](https://copr.fedorainfracloud.org/coprs/build/11339153) | succeeded: Fedora 44, 45, ELN, Rawhide x86_64 |
| `musa-runtime` | `1.5.4-3` | [11339379](https://copr.fedorainfracloud.org/coprs/build/11339379) | succeeded: Fedora 44, 45, ELN, Rawhide x86_64 |
| `musa-sdk` | `5.2.0-6` | [11339529](https://copr.fedorainfracloud.org/coprs/build/11339529) | succeeded: Fedora 44, 45, ELN, Rawhide x86_64 |
| `musa-toolkit` | `3.1.0-1` | [11339164](https://copr.fedorainfracloud.org/coprs/build/11339164) | succeeded: Fedora 44, 45, ELN, Rawhide x86_64 |

The configured Fedora chroots are x86_64 only. These are Copr build/package
results, not tests of installing or loading the driver or running the packages
on S30 hardware. `mock` is not installed on this host, so no local clean-chroot
build was run. Existing archived Copr build logs are under ignored
`.logs/copr-2026-11-17/`; they were not altered. No Copr builds were submitted
or modified during this inventory.

## Repository and host-change boundaries

Both repositories were inspected before edits. `musa-fedora` is on `main`,
tracks `origin/main`, and had a pre-existing user edit in
`scripts/load-driver-manual.sh`; it is preserved and must not be included in the
status-documentation commit. `musa-data` is on `main`, tracks `origin/main`, and
was clean before the dated inventory report was added. No host packages,
services, kernel modules, modprobe files, firmware, boot settings, or display
configuration were changed. No claim is made here that the current boot has a
working MUSA GPU path.

## Required next step

The next hardware attempt needs a separately reviewed maintenance/recovery
plan that addresses the documented GNOME/KMS conflict, IBT/objtool warnings,
opaque vendor-core faults, and the historical rebind failure. It must preserve
Intel display operation and a bootable rollback path. Until those issues are
resolved and an approved module can be safely probed on hardware, S30 GPU tests
must remain blocked. Consult Moore Threads for a supported Fedora/Intel/S30
driver/runtime/firmware combination or a corrected driver/core before
proceeding.

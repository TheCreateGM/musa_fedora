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
are absent. `/dev/dri/renderD128` is Intel; no other DRM render node is
currently present.
- IOMMU remains enabled (`intel_iommu=on iommu=pt`); Secure Boot is disabled.
- Installed packages are `mtgpu-dkms-3.1.0-7.fc45`,
`mtgpu-firmware-3.1.0-2.fc45`, `musa-runtime-1.5.4-3.fc45`,
`musa-sdk-5.2.0-6.fc45`, and `musa-toolkit-3.1.0-1.fc45`. DKMS has a module
for the current kernel, but installation alone does not mean that it is loaded
or usable.
- The installed release-7 config `/etc/modprobe.d/90-mtgpu-headless.conf`
still includes `rm_feature_level=1`, which the 3.1.0-8 package removes after
the documented kernel fault. Releases 8 and 9 were built in Copr but were not
installed on this host; the blacklist below keeps the old option inactive.
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
| `mtgpu-dkms` | `3.1.0-9` | [11365750](https://copr.fedorainfracloud.org/coprs/build/11365750) | succeeded: Fedora 44, 45, ELN, Rawhide x86_64 |
| `mtgpu-firmware` | `3.1.0-2` | [11339153](https://copr.fedorainfracloud.org/coprs/build/11339153) | succeeded: Fedora 44, 45, ELN, Rawhide x86_64 |
| `musa-runtime` | `1.5.4-3` | [11339379](https://copr.fedorainfracloud.org/coprs/build/11339379) | succeeded: Fedora 44, 45, ELN, Rawhide x86_64 |
| `musa-sdk` | `5.2.0-6` | [11339529](https://copr.fedorainfracloud.org/coprs/build/11339529) | succeeded: Fedora 44, 45, ELN, Rawhide x86_64 |
| `musa-toolkit` | `3.1.0-1` | [11339164](https://copr.fedorainfracloud.org/coprs/build/11339164) | succeeded: Fedora 44, 45, ELN, Rawhide x86_64 |

Firmware and toolkit were re-released later on 2026-11-20:

| Package | Version-release | Copr build | Result/chroots |
|---|---:|---:|---|
| `musa-toolkit` | `3.1.0-2` (intermediate) | [11365810](https://copr.fedorainfracloud.org/coprs/build/11365810) | succeeded: Fedora 44, 45, ELN, Rawhide x86_64 |
| `mtgpu-firmware` | `3.1.0-3` (intermediate) | [11365821](https://copr.fedorainfracloud.org/coprs/build/11365821) | succeeded: Fedora 44, 45, ELN, Rawhide x86_64 |
| `musa-toolkit` | `3.1.0-3` | [11365857](https://copr.fedorainfracloud.org/coprs/build/11365857) | succeeded: Fedora 44, 45, ELN, Rawhide x86_64 |
| `mtgpu-firmware` | `3.1.0-4` | [11365858](https://copr.fedorainfracloud.org/coprs/build/11365858) | succeeded: Fedora 44, 45, ELN, Rawhide x86_64 |

The firmware release 3.1.0-4 moves the payload from `/lib/firmware/mthreads` to
`/usr/lib/firmware/mthreads` (matching Fedora's linux-firmware usr-merge
layout and letting the buildroot `brp-linkdupes` pass process `/usr`), makes
the package `ExclusiveArch: x86_64` instead of `noarch` (the blobs are GPU ISA
ELF images, not host-arch-independent files), and disables the host `strip`
BRP passes so the opaque GPU ELF firmware bytes stay untouched. The toolkit
release 3.1.0-3 keeps a non-empty buildroot `/usr` for the `brp-linkdupes`
pass and rewords its description as historically exercised rather than
currently hardware-validated.

The second configured project, `axogm/musa-mtt-s30`, was rebuilt to cover its
additional Fedora 43 chroot. All four builds succeeded across Fedora 43, 44,
45, ELN, and Rawhide x86_64:

- [`musa-toolkit` 3.1.0-2 (intermediate), build 11365809](https://copr.fedorainfracloud.org/coprs/build/11365809)
- [`mtgpu-firmware` 3.1.0-3 (intermediate), build 11365820](https://copr.fedorainfracloud.org/coprs/build/11365820)
- [`musa-toolkit` 3.1.0-3, build 11365856](https://copr.fedorainfracloud.org/coprs/build/11365856)
- [`mtgpu-firmware` 3.1.0-4, build 11365859](https://copr.fedorainfracloud.org/coprs/build/11365859)

## Firmware/toolkit build log and package verification (2026-11-20)

The final builds were inspected in full:

- The `mtgpu-firmware` 3.1.0-4 `builder-live.log` for Fedora 45 and Rawhide and
  the `musa-mtt-s30` Fedora 43 build 11365859 each show
  `Wrote: /builddir/build/RPMS/mtgpu-firmware-3.1.0-<dist>.x86_64.rpm` with
  child return code 0; the only case-insensitive `error` hits are dnf package
  names (`libgpg-error`). The `brp-linkdupes` pass scanned the buildroot
  `/usr` tree and linked 2 duplicate files (~1.6 MiB saved); no strip pass ran
  on the firmware blobs.
- The `musa-toolkit` 3.1.0-3 `builder-live.log` for Fedora 45 shows
  `Wrote: /builddir/build/RPMS/musa-toolkit-3.1.0-3.fc45.x86_64.rpm`; no fatal
  errors. The second project's Fedora 43 build 11365856 succeeded the same way.
- The published Fedora 45 RPMs `mtgpu-firmware-3.1.0-4.fc45.x86_64.rpm` and
  `musa-toolkit-3.1.0-3.fc45.x86_64.rpm` (the metapackage now defaults to
  `x86_64`, not `noarch`) were downloaded from the Copr repository and
  verified with a private `rpmkeys` database (no system keyring change):
  `digests signatures OK` against the project Copr key
  `0F9E1B00A8990F19AB661745B21968F295ECB7E4`.
- The firmware payload is byte-identical to the installed 3.1.0-2 payload:
  SHA-256 comparison of every file under `mthreads/` matched the host
  `/lib/firmware/mthreads` tree, confirming the strip-skip change did not
  alter any blob.
- `rpmspec -P` passed for both updated specs, and both SRPMs built locally
  (`mtgpu-firmware-3.1.0-4.fc45.src.rpm`, `musa-toolkit-3.1.0-3.fc45.src.rpm`).
  One duplicate Copr submission of the firmware SRPM (build 11366194) was made
  while re-verifying the workflow and was canceled; the same SRPM had already
  succeeded as 11365858.
- `scripts/rpm-stack.sh` now expects `mtgpu-firmware` 3.1.0-4 and
  `musa-toolkit` 3.1.0-3. `scripts/load-driver-manual.sh` loads with
  `display=none disable_fbdev=1 mtgpu_drm_major=2` (no `rm_feature_level=1`).

Prior intermediate builds for `mtgpu-dkms`:
- Release 6 failed across chroots due to missing `systemd-rpm-macros` during `%files` macro expansion for `70-mtgpu.rules`.
- Release 7 ([11365567](https://copr.fedorainfracloud.org/coprs/build/11365567)) resolved buildroot dependencies and succeeded across all 4 chroots.
- Release 8 ([11365657](https://copr.fedorainfracloud.org/coprs/build/11365657)) removed fatal `rm_feature_level=1` from `90-mtgpu-headless.conf` (see [`root-cause-rm-feature-level-2026-11-20.md`](root-cause-rm-feature-level-2026-11-20.md)).
- Release 9 ([11365750](https://copr.fedorainfracloud.org/coprs/build/11365750)) removed the unsupported claim that an IBT property note makes the opaque vendor core safe to load on IBT/CET kernels.

The second configured project, `axogm/musa-mtt-s30`, was also rebuilt to cover
its additional Fedora 43 chroot. All five builds succeeded across Fedora 43,
44, 45, ELN, and Rawhide x86_64:

- [`mtgpu-firmware`, build 11365759](https://copr.fedorainfracloud.org/coprs/build/11365759)
- [`mtgpu-dkms` 3.1.0-9, build 11365760](https://copr.fedorainfracloud.org/coprs/build/11365760)
- [`musa-runtime`, build 11365761](https://copr.fedorainfracloud.org/coprs/build/11365761)
- [`musa-sdk`, build 11365762](https://copr.fedorainfracloud.org/coprs/build/11365762)
- [`musa-toolkit`, build 11365763](https://copr.fedorainfracloud.org/coprs/build/11365763)

All five Fedora 43 builder logs were inspected. They show successful RPM
results and no fatal build errors; the DKMS log again contains 5,486 objtool
warning lines.

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

## Follow-on validation after the inventory

- `MUSA_PATH=/usr/local/musa-5.2 scripts/test-toolchain.sh` passed on the host
  and inside a Fedora 45 container after installing the Copr package set. Both
  runs emitted a MUSA fat-binary section, produced an explicit `mp_10` target,
  and syntax-checked the runtime headers. Neither run executed the program or
  queried the runtime.
- All five actual Fedora 45 Copr RPMs (DKMS 3.1.0-9, firmware 3.1.0-2,
  runtime 1.5.4-3, SDK 5.2.0-6, and toolkit 3.1.0-1) passed signature and
  digest verification against the project's Copr key
  (`0F9E1B00A8990F19AB661745B21968F295ECB7E4`). DNF installed the set into a
  disposable Fedora 45 container with RPM scriptlets disabled; `rpm -V` and
  `dnf check` passed. DNF's signature checks were disabled for that transaction;
  the five project RPMs were checked separately. The packaged driver config
  omits `rm_feature_level=1`. DKMS was not run in the container.
- Host `rpm -V` was clean for `mtgpu-dkms` 3.1.0-7, firmware, SDK, and
  toolkit. `musa-runtime` reports its disabled `/etc/modprobe.d/mtgpu.conf`
  missing and `/usr/lib64/libdrm_mtgpu.so.1` redirected to
  `libmtperf_target.so`; those local differences were left untouched.
- `scripts/test/check-hardware.sh` again reported PCI identity `1ed5:0102`,
  unbound driver, no MTT device nodes, and `GPU_TEST_STATUS: NOT RUN` (exit 3).
  The current-boot kernel journal still has no `mtgpu` probe/workload entries;
  no GPU, runtime, OpenCL, or Vulkan test was attempted. No GPU-related system
  service is active, and `systemctl --failed` reported no failed units.
- The complete Fedora 44/45/ELN/Rawhide logs for Copr build 11365750 each show
  a successful module compile and RPM result, with 5,486 objtool warning lines
  per chroot. The warnings include missing-ENDBR relocations and RETPOLINE
  naked returns. Build 11365657 shows the same warnings. These successful
  builds do not establish that the module is safe to load.
- The Fedora 45 package set installed with scriptlets disabled; package
  verification and `dnf check` were clean. The in-container compiler check
  emitted the MUSA fat-binary section, produced explicit `mp_10` output, and
  syntax-checked runtime headers. No executable or runtime query was run.

## Required next step

A draft maintenance/recovery plan is now checked in as
[`maintenance-plan-mtgpu-load-2026-11-20.md`](maintenance-plan-mtgpu-load-2026-11-20.md).
It is **not approved and not executed**; it requires explicit review before
any load attempt. It addresses the documented GNOME/KMS conflict (headless
mode + intact blacklist for normal boots), the IBT/objtool warnings (optional
one-shot `ibt=off` boot mode, Mode B), the opaque vendor-core faults (no
`rm_feature_level=1`, single load per boot, no unbind/rebind, abort on any
Oops), and preserves an Intel display plus a bootable rescue rollback path.
Until the plan is approved and executed, S30 GPU tests remain blocked.
Consult Moore Threads for a supported Fedora/Intel/S30 driver/runtime/firmware
combination or a corrected driver/core before proceeding.

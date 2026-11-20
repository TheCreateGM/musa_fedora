# Historical clean-boot resume plan — MTT S30 (2026-11-17)

> **Do not follow this plan on the current host.** It predates the intentional
> `/etc/modprobe.d/blacklist-mtgpu.conf` safety block and the later documented
> GNOME/KMS crashes during driver loading. The current S30 is unbound and the
> module must not be loaded or unbound/rebound ad hoc. See
> [`current-state-2026-11-20.md`](current-state-2026-11-20.md) and
> [`driver-loading.md`](driver-loading.md). A new attempt requires a reviewed
> display rollback and a vendor-core/IBT recovery plan.

This dated plan is retained as historical context for the 2026-11-17 OpenCL
kernel fault only. Its reboot and GPU-test steps are not approval for current
hardware validation.

## Why a reboot is required

The current boot (started 2026-11-17 14:16) logged an `mtgpu` kernel fault at
16:18. The archived trace shows the driver reached a NULL-pointer
dereference from the PVR/DDK compute bridge:

```
16:18:38 kernel: BUG: kernel NULL pointer dereference, address: 0000000000000001
16:18:38 kernel: Oops: 0002 [#1] SMP NOPTI
16:18:38 kernel:  PVRSRVRGXFlushComputeDataKM+0x86/0x1c0 [mtgpu]
16:18:38 kernel:  PVRSRVBridgeRGXFlushComputeData+0x62/0xb0 [mtgpu]
16:18:38 kernel:  BridgedDispatchKM+0x1df/0x580 [mtgpu]
16:18:38 kernel:  PVRSRV_BridgeDispatchKM+0xeb/0x1a0 [mtgpu]
```

An earlier trace in the same window came from
`PhysmemNewLocalRamBackedPMR` / `PVRSRVBridgePhysmemNewRamBackedPMR`.
After the fault the module stayed bound and `initstate=live`, but the driver
state is unknown and `mtgpu ... irq:NNN affinity has changed.` messages kept
appearing. Per project safety rules, no further GPU workload runs in a boot
that logged an `mtgpu` fault. The gate
[`scripts/test-gpu.sh`](../scripts/test-gpu.sh) now enforces this: it reports
`GPU_TEST_STATUS: NOT RUN` with `this boot logged an mtgpu kernel fault`.

## Pre-reboot snapshot (already captured)

Read-only host state was captured before this plan to
`/home/axogm/Documents/project/musa-data/host-backups/pre-clean-boot-reboot-20261117-162913/`:

- `BASELINE.txt`, `proc-cmdline.txt`, `rpm-versions.txt`, `dkms-status.txt`
- `mtgpu.modinfo.txt`, `on-disk-module.sha256.txt` (live module:
  `21a3764bac0598118fdb5961f7d91adb7118cbb56d130f9d83faaaace01adad5`)
- `lspci-1ed5.txt`, `iommu-s30.txt` (S30 in IOMMU group 11)
- `installed-configs.txt`, `kernel-journal-mtgpu.txt`

A prior full backup exists at
`/home/axogm/Documents/project/musa-data/host-backups/pre-rpm-deploy-20261117-153237/`
(working module, firmware tarball, `/usr/local/musa` tarball, boot backup).

## Safety assessment

The reboot is low risk for the existing Fedora installation:

- The desktop is driven by Intel UHD 770 (`i915`); the S30 does not drive
  display output. If `mtgpu` fails to initialize after reboot, the desktop is
  unaffected.
- A rescue kernel (`vmlinuz-0-rescue-*`) and two older kernels
  (`7.2.15-300.fc45`, `7.2.15-200.fc44`) are installed and selectable from the
  GRUB menu.
- No boot argument, kernel, package, or loader configuration is changed by the
  reboot. The active command line stays
  `... i915.enable_psr=0 intel_iommu=on iommu=pt` with IBT enabled.
- `/lib/modules/7.3.3-300.fc45.x86_64/modules.alias` contains
  `pci:v00001ED5d00000102...mtgpu`, so the S30 VGA function will autoload
  `mtgpu` on the next boot.

## Step 1 — Reboot (user action)

No configuration changes. Reboot into the default kernel `7.3.3-300.fc45`:

```sh
systemctl reboot
```

Do not add or remove kernel arguments, do not select `ibt=off`, and do not
boot a different kernel unless Step 3 abort/recovery is required.

## Step 2 — Post-boot pre-flight (read-only)

Run these after the reboot and confirm each result before any GPU workload:

```sh
# a. Fresh boot, expected default kernel, unchanged command line
uptime; uname -r; cat /proc/cmdline

# b. No mtgpu fault in this boot (must print nothing)
journalctl -k -b 0 --no-pager | grep -E 'BUG: kernel NULL|Oops:|general protection fault' | grep mtgpu

# c. Driver loaded and live, device + render nodes present
cat /sys/module/mtgpu/initstate
ls -l /dev/mtgpu.0 /dev/dri/renderD*

# d. On-disk module unchanged since the snapshot
sha256sum /lib/modules/$(uname -r)/extra/mtgpu.ko.xz

# e. Gated status: must be NOT RUN only for the approval flag, not for a fault
bash scripts/test-gpu.sh
```

Expected: kernel `7.3.3-300.fc45.x86_64`; (b) empty; (c) `live` plus
`/dev/mtgpu.0` and the S30 render node; (d) the same SHA-256 as the snapshot;
(e) `BLOCKED: set MUSA_S30_TEST_APPROVED=1 ...` and **not** the
`this boot logged an mtgpu kernel fault` line.

## Step 3 — Reviewed GPU validation

Only after Step 2 passes, run the gated smoke test once, then the full suite:

```sh
# Gate + minimal vector-add (S30 discovery, one compile+run)
MUSA_S30_TEST_APPROVED=1 bash scripts/test-gpu.sh

# Full recorded suite (real MUSA compute + Vulkan device/memory)
source scripts/musa-env.sh
bash scripts/verify-musa.sh
```

Scope: device discovery, `mthreads-gmi`, `musaInfo`, `muMemcpyTest`, the host
runtime API, the `mp_10` vector-add, the eight compute operations, and the
headless Vulkan device/buffer/memory probe. Expected record: **13 passed,
0 failed, 1 skipped** (the skip is Moore Threads OpenCL; no MT ICD is
installed).

Do **not** install or run the archived MobiMaliang 2.1.1 OpenCL ICD, and do not
run any OpenCL compute probe: that path triggered the fault. Do not
unbind/rebind the device, and do not load or install a replacement module.

## Abort criteria

Stop and record evidence (do not retry in the same boot) if any of these
happens:

- Step 2 (b) finds an `mtgpu` kernel fault — the fresh boot is already
  degraded; do not run Step 3.
- `bash scripts/test-gpu.sh` reports the fault blocker, or
  `musaInfo` / `muMemcpyTest` / the vector-add fails.
- A new `BUG`/`Oops`/`general protection fault` appears in
  `journalctl -k -b 0` during Step 3.
- `/sys/module/mtgpu/initstate` is not `live`, or the render node is missing.

Bias to aborting: a successful status query does not establish driver health.

## Recovery

- **Driver does not come up after reboot (S30 unbound, no `/dev/mtgpu.0`):**
  the desktop is unaffected. Reboot to the rescue entry or the older
  `7.2.15-300.fc45` kernel from the GRUB menu, capture
  `journalctl -k -b -1 | grep mtgpu`, and stop. No unbind/rebind of a live
  module. If autoload must be suppressed for a later boot, review a reversible
  `modprobe` blacklist change first; do not edit it blindly.
- **Kernel fault recurs in Step 3:** reboot, then stop GPU testing and treat
  the driver as degraded until the fault is root-caused statically.
- **GRUB/boot change needed:** boot entries and the GRUB environment were
  backed up to
  `musa-data/host-backups/boot-config-2026-11-12.tar`; see
  [IBT-off recovery](ibt-off-recovery.md) for the restore commands. Do not
  disable IBT/CET as a workaround.

## Do not

- Do not disable IBT/CET or add `ibt=off`.
- Do not unbind/rebind or replace the live module; a historical rebind faulted
  in `mtgpu_pstate_notifier_call_chain`
  ([root cause](root-cause-rebind-2026-11-16.md)).
- Do not install a DKMS/RPM stack over the live module without its own
  recovery plan.
- Do not install or run the archived 2.1.1 OpenCL ICD.

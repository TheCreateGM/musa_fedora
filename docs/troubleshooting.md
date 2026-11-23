# Troubleshooting

Current facts and logs are in the
[live state](current-state-2026-11-20.md). Historical hardware tests and the
OpenCL incident are in the [2026-11-17 record](current-state-2026-11-17.md).

## Recorded host verification

The command below performs real GPU work. The recorded run is historical for
the current host; do not execute it while the S30 is unbound or the driver is
blacklisted. The current boot has the module blacklisted and the S30 unbound;
do not remove the blacklist as an ad-hoc recovery step.

```sh
./scripts/verify-musa.sh
```

The historical verifier run completed with **13 passed, 0 failed, 1 skipped**
before a separate archived OpenCL ICD compute probe triggered a kernel Oops in
`mtgpu`. The verifier runs real MUSA compute and Vulkan device/memory checks;
it is not read-only with respect to GPU execution. Do not run GPU tests in a
boot where the driver faulted. The current inventory finds the S30 unbound and
`mtgpu` blacklisted after documented GNOME/KMS crashes. See the [failure record](current-state-2026-11-17.md),
[current inventory](current-state-2026-11-20.md), and the
[reviewed clean-boot resume plan](clean-boot-resume-plan-2026-11-17.md).
The gate `scripts/test-gpu.sh` currently reports NOT RUN because the S30 is
unbound and has no S30-associated render node.

## Driver and module state

The latest inventory finds the vendor `mtgpu` 3.1.0 module not loaded on
kernel `7.3.4-300.fc45.x86_64`; the S30 is unbound and
`/etc/modprobe.d/blacklist-mtgpu.conf` remains active to protect the Intel
desktop. Do not remove the blacklist or load, unbind, or rebind the device:
a prior load caused GNOME/KMS crashes, and a rebind can fault inside
`mtgpu_pstate_notifier_call_chain` (see
[root-cause-rebind-2026-11-16.md](root-cause-rebind-2026-11-16.md)). Do not
install a new module over the live one in this session, and do not disable
IBT/CET or patch vermagic.

Earlier boots also recorded a module failing with `Missing ENDBR: rmapi_alloc`
and an IBT control-protection fault while `initstate=coming`; these are
additional reasons not to load an unreviewed module.

## Build and compiler checks

A DKMS 3.1.0 module is installed for the current kernel, but it is blocked
from loading and is not hardware-verified in the current boot. The isolated
fresh S30 3.1.0 kernel build in `musa-data` for 7.3.3 emitted 5,504
compiler/objtool warning lines and is not approved for installation. The MUSA
5.2.0 compiler and `mp_10` vector-add were previously tested while the module
was live; this is historical evidence only. Build logs are under
`/home/axogm/Documents/project/musa-data/research/musa-fedora-live-validation-2026-11-12/`
and `musa-data/research/builds/`.

## Vendor event-report logging warning (cosmetic)

At every `mtgpu` load the kernel log may show:

```
mtgpu 0000:01:00.0: Failed to create directory: /var/log/mtgpu, ret = -2
mtgpu 0000:01:00.0: Failed to create directory: /var/log/mtgpu/event_report, ret = -2
mtgpu 0000:01:00.0: Failed to create directory: /var/log/mtgpu/event_report/<uuid>, ret = -2
mtgpu 0000:01:00.0: xid_record file create fd failed
```

Recorded 2026-11-23 with the directories already present (stale 2026-11-19
state; the `event_report/<uuid>` subdirectory carries a non-searchable mode,
`drw----r-x`). The directory and file creation logic lives in the opaque
vendor core blob `objs/x86_64/mtgpu_core.o_binary` (confirmed by `strings`),
so it cannot be patched from the source tree. `mtgpu-dkms` 3.1.0-12 ships a
tmpfiles.d entry that guarantees `/var/log/mtgpu{,/event_report}` exist on
fresh installs, which is necessary but not sufficient to silence the
warning. It is cosmetic: the complete GPU workload (memcpy, runtime API,
`mp_10` kernels, compute suite, FP64, Vulkan) passed with the warning
present. Do not delete the stale `event_report` directory as an ad-hoc fix;
its effect can only be verified across a controlled driver load in a
reviewed boot.

The code path is identified at function level in
[`root-cause-mtgpu-event-report-mkdir-2026-11-23.md`](root-cause-mtgpu-event-report-mkdir-2026-11-23.md):
the message comes from `mtgpu_event_report_persist_init`, the driver logs
the error and continues, and only the vendor XID log file is lost. That
document also shows why `-2` cannot mean "the directory is missing": the
kernel returns `-EEXIST` for an existing name, so the driver's lookup did
not observe the host's existing tree.

## Archived experiments

The CPU emulator and CPU-only experiments under
`/home/axogm/Documents/project/musa-data/research/` are not GPU evidence.
Do not use archived or stale module artifacts for the running kernel.

# Troubleshooting

Current facts and logs are in the
[current state](current-state-2026-11-17.md).

## Recorded host verification

The command below performs real GPU work. The recorded run is historical for
the current boot; do not execute it again in the boot that logged the Oops.
Resume validation after recovery and a clean boot, with the test scope reviewed.

```sh
./scripts/verify-musa.sh
```

The recorded verifier run completed with **13 passed, 0 failed, 1 skipped**
before a separate archived OpenCL ICD compute probe triggered a kernel Oops in
`mtgpu`. The verifier runs real MUSA compute and Vulkan device/memory checks;
it is not read-only with respect to GPU execution. Do not run GPU tests again
in the boot that logged the Oops. The S30 module remained loaded and bound,
but that status does not establish driver health. See the [failure record](current-state-2026-11-17.md)
and the [reviewed clean-boot resume plan](clean-boot-resume-plan-2026-11-17.md).
The gate `scripts/test-gpu.sh` refuses to run when the current boot already
logged an `mtgpu` fault.

## Driver and module state

The vendor `mtgpu` 3.1.0 driver is loaded and live
(`/sys/module/mtgpu/initstate=live`) on kernel `7.3.3-300.fc45.x86_64`, with
`/dev/mtgpu.0` and the S30 render node `/dev/dri/renderD129` present. Do not
unbind or rebind the live device: a rebind can fault inside
`mtgpu_pstate_notifier_call_chain` (see
[root-cause-rebind-2026-11-16.md](root-cause-rebind-2026-11-16.md)). Do not
install a new module over the live one in this session, and do not disable
IBT/CET or patch vermagic.

Earlier boots recorded a module failing with `Missing ENDBR: rmapi_alloc` and
an IBT control-protection fault while `initstate=coming`; that state is
historical and is superseded by the live state above.

## Build and compiler checks

A 3.1.0 module is installed and live, so the stack runs. The isolated fresh
S30 3.1.0 kernel build in `musa-data` for 7.3.3 emitted 5,504 compiler/objtool
warning lines and is not approved for installation as a replacement. The
MUSA 5.2.0 compiler built `tests/musa-vector-add.mu` for `mp_10`, which ran on
the S30. Build logs are under
`/home/axogm/Documents/project/musa-data/research/musa-fedora-live-validation-2026-11-12/`
and `musa-data/research/builds/`.

## Archived experiments

The CPU emulator and CPU-only experiments under
`/home/axogm/Documents/project/musa-data/research/` are not GPU evidence.
Do not use archived or stale module artifacts for the running kernel.

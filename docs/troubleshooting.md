# Troubleshooting

Current facts and logs are in the
[current state](current-state-2026-11-17.md).

## Read-only host check

Run:

```sh
./scripts/verify-musa.sh
```

On this host the S30 is bound and the `mtgpu` 3.1.0 module is live, so the
verifier reports **11 passed, 0 failed, 0 skipped** and exits 0. A non-zero
exit status (previously 3) indicates the S30 is unbound or an MTGPU module is
not fully initialized. The verifier reads PCI/sysfs and device-node metadata;
it does not open the GPU node or initialize the MUSA runtime.

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

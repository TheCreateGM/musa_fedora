# Troubleshooting

Current facts and logs are in the
[live host recheck](live-host-recheck-2026-11-13.md).

## Read-only host check

Run:

```sh
./scripts/verify-musa.sh
```

Exit status 3 is expected while the S30 is unbound or any MTGPU module is not
fully initialized. The verifier reads PCI/sysfs and device-node metadata; it
does not open the GPU node or initialize the MUSA runtime.

## Current module failure

The boot-loaded, unowned module reported `Missing ENDBR: rmapi_alloc` and a
kernel IBT control-protection fault. It remains resident in
`initstate=coming`; do not query the runtime, open a GPU node, or try to
unload it in this boot. An unowned module file is currently present in the running kernel tree, with
a different hash from the file quarantined on 2026-11-12. Its origin is
unknown. Do not query the runtime, open a GPU node, unload the module, or
restore the historical backup. The quarantine script's `--plan` mode is
read-only.

## Build and compiler checks

The latest isolated S30 3.1.0 kernel build for 7.3.3 linked, but reported
5,504 compiler/objtool warning lines. Do not install or load the artifact.
The staged
MUSA 5.2.0 compiler passes compile-only checks; its output was not run and
does not prove S30 compatibility. Build logs are under
`/home/axogm/Documents/project/musa-data/research/musa-fedora-live-validation-2026-11-12/`
and `musa-data/research/builds/`.

## Archived experiments

The CPU emulator and CPU-only experiments under
`/home/axogm/Documents/project/musa-data/research/` are not GPU evidence.
Do not use archived or stale module artifacts for the running kernel.

# Userspace / runtime reconciliation

Reviewed 2026-11-17. Current host state is recorded in the
[current state](current-state-2026-11-17.md). The host runs kernel 7.3.3 with
the vendor `mtgpu` 3.1.0 driver live and the MTT S30 bound. MUSA runtime 1.5.4
at `/usr/local/musa/` detects the S30 and passes the vendor copy test; the
host runtime API (`tests/musart-smoke.c`) passes; and a 5.2.0 `mcc` `mp_10`
vector-add ran on the S30. The Intel UHD 770 remains the display GPU.

## Verified runtime

The supported userspace runtime on this host is MUSA 1.5.4 at
`/usr/local/musa/` (`libmusart.so.1.5.4`). It ships its own
`musa_runtime.h` under `/usr/local/musa/include`; compile host code against
those headers, not the 5.2.0 SDK headers. `/dev/mtgpu.0` and the S30 render
node `/dev/dri/renderD129` are present.

## Host-local SDK tree

`/usr/local/musa-5.2` exists and is not RPM-owned. `mcc_version` and
`musa_runtime_version` report 5.2.0; `bin/mcc` resolves to the local clang
toolchain. The tree includes `musaInfo`, `libmusart.so.5.2.0`, and
`libmtrtc.so.5.2.0`. The host environment sets
`MUSA_HOME=/usr/local/musa-5.2` and adds its library directory to
`LD_LIBRARY_PATH`.

The 5.2.0 *compiler* is the one in use (it built the `mp_10` vector-add
kernel), but the 5.2.0 *runtime* is not compatible with the 3.1.0 driver ABI
(`sizeof(struct musaDeviceProp)` is 728 bytes in 1.5.4 vs 1032 in 5.2.0) and
must not be substituted for 1.5.4.

## Official 5.2.0 research packages

Official package files, package-index metadata, and prior compile/link
artifacts are staged under
`/home/axogm/Documents/project/musa-data/research/official-musa-5.2.0/`.
They have not been installed from that directory. The extracted package
layout differs from the host-local `/usr/local/musa-5.2` tree; the package
hash evidence and exact paths are in
`docs/vendor-sdk-availability.md`.

The compile-only test linked a default x86-64 executable with MUSA offload
bundles for `mp_21`, `mp_22`, `mp_31`, and `mp_32`, and separately compiled
an explicit `mp_10` executable. Moore Threads' product filing identifies
S30 as SuDi, and its MUSA architecture guide maps SuDi to `MP_10`. That
`mp_10` target is now confirmed on hardware: `tests/musa-vector-add.mu` built
for `mp_10` and ran on the S30 (`PASS: 1048576 float adds verified on S30`).
See the
[target evidence record](../../musa-data/research/official-musa-5.2.0/s30-mp10-target-evidence-2026-11-12.md).

## Safety status

The current host runs kernel `7.3.3-300.fc45.x86_64` with the `mtgpu` 3.1.0
module live and the S30 bound. Do not install over the live module, unbind or
rebind the device, replace files under `/usr/local`, or alter system library
paths on a running stack. The unvalidated installer, DKMS hooks, udev rule,
and blob-rewrite helper are archived under
`musa-data/research/disabled-deployment-drafts-2026-11-11/`; do not execute
them. No installation or rollback procedure is approved.

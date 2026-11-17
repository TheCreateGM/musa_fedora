# Userspace / runtime reconciliation

Reviewed 2026-11-12. Current host state is recorded in the
[live recheck](live-host-recheck-2026-11-13.md). The host runs kernel 7.3.3;
the S30 is unbound and the boot-loaded `mtgpu` module is stuck in
`initstate=coming` after an IBT control-protection fault. No runtime query was
performed in the latest pass. An unowned `mtgpu.ko` with a different hash
from the file quarantined on 2026-11-12 is currently present in the running
kernel tree; its origin is unknown.

## Host-local SDK tree

`/usr/local/musa-5.2` exists and is not RPM-owned. `mcc_version` and
`musa_runtime_version` report 5.2.0; `bin/mcc` resolves to the local clang
toolchain. The tree includes `musaInfo`, `libmusart.so.5.2.0`, and
`libmtrtc.so.5.2.0`. These observations do not authenticate the install as the
staged package set or establish compatibility with the S30.

The host environment sets `MUSA_HOME=/usr/local/musa-5.2` and adds its library
directory to `LD_LIBRARY_PATH`. The S30 is currently unbound and
`/dev/mtgpu.0` is absent. Do not open a GPU node or run `musaInfo` or GPU
programs while the failed module remains resident.

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
S30 as SuDi, and its MUSA architecture guide maps SuDi to `MP_10`; this
supports `mp_10` as the S30 code-generation target. Neither executable was
run, and the MUSA 5.2.0 runtime, driver, and S30 device compatibility are
unverified. See the
[target evidence record](../../musa-data/research/official-musa-5.2.0/s30-mp10-target-evidence-2026-11-12.md).

## Safety status

The latest host check found kernel `7.3.3-300.fc45.x86_64`, with the full
`mtgpu` module stuck in `coming` and the PCI function unbound. Do not install
staged packages into Fedora, alter system library paths, replace files under
`/usr/local`, or perform GPU runtime calls. The unvalidated installer, DKMS
hooks, udev rule, and blob-rewrite helper are archived under
`musa-data/research/disabled-deployment-drafts-2026-11-11/`; do not execute
them. No installation or rollback procedure is approved.

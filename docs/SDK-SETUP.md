# MUSA SDK setup status

No SDK from the research package set has been installed. A separate,
unowned `/usr/local/musa-5.2` tree exists, but its S30 runtime compatibility
is not established. The current host and test evidence are in the
[live recheck](live-host-recheck-2026-11-13.md).

## Staged vendor packages

Official MUSA 5.2.0 packages and package-index hashes are staged under
`/home/axogm/Documents/project/musa-data/research/official-musa-5.2.0/`.
They were extracted for inspection and compile-only checks; they were not
installed from that directory. The official [SDK catalog](https://developer.mthreads.com/resources/musa-sdk?tab=overview)
currently lists 5.2.0 Linux DEB/RPM packages for MTT S5000, not S30.

## Compile-only validation

The staged compiler produced a host executable with the vendor fat-binary
sections and default target tags `mp_21`, `mp_22`, `mp_31`, and `mp_32`. An
explicit `--offload-arch=mp_10` build and a runtime-header syntax check also
passed. Moore Threads' product filing identifies S30 as SuDi architecture;
its MUSA architecture guide maps SuDi to `MP_10`, and the MCC manual lists
`mp_10` as an offload target. This official architecture chain supports
`mp_10` as the S30 code-generation target. The 5.2.0 executable was not run,
so S30 runtime and device compatibility remain unverified. See the
[target evidence record](../../musa-data/research/official-musa-5.2.0/s30-mp10-target-evidence-2026-11-12.md)
for sources and version limits.

Run the compile-only check with:

```sh
./scripts/test-toolchain.sh
```

It uses staged packages under `musa-data` when available. It does not query
the runtime or access a GPU.

For a shell-local SDK environment backed by the same staged extracts, source:

```sh
MUSA_USE_STAGING=1 source ./scripts/musa-env.sh
./scripts/test-toolchain.sh
```

This only exports compiler, header, and library search paths in the current
shell; it does not install packages or edit loader configuration. The helper
preserves an existing `LD_LIBRARY_PATH` and excludes the link-only `libmusa`
stub from runtime search paths. Do not run `musaInfo` or a GPU executable
while the driver remains unqualified and stuck in initialization.

## Host safety status

The S30 is unbound. A boot-loaded MTGPU module faulted at `rmapi_alloc` with
an IBT control-protection exception and remains in `initstate=coming`. Its
on-disk file was hash-verified, backed up, and quarantined from the running
kernel tree; the current in-memory module remains until reboot. The gated GPU
test exits before runtime access.

Do not install Ubuntu packages directly, load unreviewed kernel modules,
disable IBT/CET, or treat compiler output as proof of device support. The
driver build and Fedora runtime compatibility remain unresolved.

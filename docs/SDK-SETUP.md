# MUSA SDK setup status

A MUSA SDK is in place on this host. The supported runtime is MUSA 1.5.4 at
`/usr/local/musa/` (`libmusart.so.1.5.4`), and the MUSA 5.2.0 compiler and
headers are at `/usr/local/musa-5.2/`. The host and test evidence are in the
[current state](current-state-2026-11-17.md). No SDK from the staged research
package set has been installed into Fedora; the running `/usr/local` trees are
separate from those staged packages.

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
`mp_10` as the S30 code-generation target, and it is now confirmed on
hardware: `tests/musa-vector-add.mu` built for `mp_10` and ran on the S30
(`PASS: 1048576 float adds verified on S30`). See the
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
stub from runtime search paths.

## Host safety status

The MTT S30 is bound and the vendor `mtgpu` 3.1.0 driver is live
(`initstate=live`) with `/dev/mtgpu.0` present. MUSA runtime 1.5.4 detects
the device and the hardware tests above pass. The separate 5.2.0 *runtime* is
not compatible with the 3.1.0 driver ABI (`sizeof(struct musaDeviceProp)` is
728 bytes in 1.5.4 vs 1032 in 5.2.0) and must not be substituted for 1.5.4.

Do not install Ubuntu packages directly, install over the live module,
unbind/rebind the device, or disable IBT/CET. The five Fedora RPM specs
(`mtgpu-dkms`, `mtgpu-firmware`, `musa-runtime`, `musa-sdk`, `musa-toolkit`)
describe the intended packaging but are not the running stack.

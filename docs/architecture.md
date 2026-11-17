# Current project architecture

The active `src/`, `inc/`, and `Makefile` tree identifies as the S30 driver
package version 3.1.0. The separate `driver-mtgpu-5.2.0/` tree is research
and build work for the MUSA 5.2.0 server driver; it is not the source used by
the current isolated S30 build. The authoritative current host snapshot is
the [current state](current-state-2026-11-17.md): kernel 7.3.3, `mtgpu` 3.1.0
loaded and live (`initstate=live`) with the MTT S30 bound at `0000:01:00.0`
(`1ed5:0102`), IOMMU enabled (`intel_iommu=on iommu=pt`, S30 in group 11),
`/dev/mtgpu.0` and the S30 render node `/dev/dri/renderD129` present, with the
Intel `i915` GPU still driving the desktop. The earlier
[2026-11-13 live recheck](live-host-recheck-2026-11-13.md) (unbound S30,
`initstate=coming`, no device nodes) is now historical.

## Production tree

| Path | Role |
|---|---|
| `src/`, `inc/`, `Makefile` | S30 driver package 3.1.0 sources and Fedora compatibility port |
| `driver-mtgpu-5.2.0/` | Separate MUSA 5.2.0 server-driver source/build experiment |
| `/home/axogm/Documents/project/musa-data/research/` | Vendor core objects, staged packages, build outputs, and other research data |
| <code>scripts/verify-musa.sh</code> | Read-only readiness report; never loads modules or runs GPU work |
| <code>scripts/test/check-hardware.sh</code> | Read-only PCI, module, and device-node check |
| <code>scripts/test/run-gpu-tests.sh</code> | Explicitly reports GPU tests as NOT RUN until a usable stack exists |
| <code>scripts/test-toolchain.sh</code> | Compile-only MUSA compiler and header check; never initializes the runtime |
| <code>scripts/test-gpu.sh</code> | Requires the full S30 driver, an associated render node, IBT, and explicit test approval |
| <code>scripts/quarantine-unvalidated-mtgpu.sh</code> | Reversibly quarantines only the recorded failed module file for kernel 7.3.3; does not unload the resident module |
| <code>scripts/quarantine-experimental-mtgpu-binder.sh</code> | Reversibly quarantines the incomplete project binder and stale alias in the inactive 7.2.15-300 tree |
| <code>tests/hw-probe.c</code> | Read-only sysfs inventory of S30 identity and binding |
| <code>tests/report-module-trees.sh</code> | Reports installed module files and vermagic across kernel trees |
| <code>docs/driver-porting.md</code> | Reproducible compile-only build result and runtime validation boundary |

The reproducible build script creates a fresh source copy and keeps generated
module output under `musa-data`; it does not install or load a module:

<pre><code>./scripts/build-kernel-driver.sh "$(uname -r)"</code></pre>

The latest clean 3.1.0 build was produced for kernel 7.3.3 in a fresh
directory under `musa-data/research/builds`. It has matching vermagic and an
S30 alias but carries 5,504 compiler/objtool warning lines. A 3.1.0 module is
installed at
`/lib/modules/7.3.3-300.fc45.x86_64/extra/mtgpu.ko` and is loaded and live on
this host; the isolated build artifact in `musa-data` is not what is
installed. Review of the vendor core, driver initialization, and GPU
execution is partly validated: the S30 is bound and live, and the four
hardware-verified tests below pass.
See
[driver-porting.md](driver-porting.md) for results and safety gates.

The five Fedora RPM specs (`mtgpu-dkms`, `mtgpu-firmware`, `musa-runtime`,
`musa-sdk`, `musa-toolkit`) describe the intended packaging, but the running
stack is currently unowned. Do not install over the live module or unbind the
device. The two quarantine scripts are recovery tools for known unvalidated
artifacts; they do not install MUSA support.

## Research archive

An earlier host CPU emulator, its `.mu` translator, generated binaries, and
CPU-only tests are preserved under
<code>/home/axogm/Documents/project/musa-data/research/legacy-cpu-model-2026-11/</code>.
They were removed from production targets because they did not communicate
with the MTT S30 and could not validate MUSA behavior. The archive is historical
research material, not an SDK or an installation source.

The pristine official Mthreads source, SDK packages, and compatibility
experiments remain under <code>musa-data/research/official-musa-5.2.0/</code>.
The Fedora kernel port remains unreviewed for runtime safety; see
[driver-porting.md](driver-porting.md).

Residual CPU-emulator code, generated binaries, unsafe legacy module helpers,
and their build/test output were moved out of the production tree on
2026-11-06. They are preserved under the corresponding
<code>musa-data/research/*/project-residual-2026-11-06/</code> and
<code>workspace-output-2026-11-06/</code> directories. Do not use them as a
MUSA implementation or hardware-test evidence. The latest experimental copy
of the official driver source is under
<code>musa-data/research/official-musa-5.2.0/project-tree-attempt-2026-11-06/</code>;
it is unreviewed and does not build into a usable module.

## Implementation boundary

A hardware-verified path now exists for device discovery, the vendor copy
test, the host MUSA runtime API (`tests/musart-smoke.c`), and one `mp_10`
vector-add kernel (`scripts/verify-musa.sh`: 11 passed, 0 failed, 0 skipped).
Everything beyond those four checks — OpenCL, OpenGL/Vulkan, video,
`torch_musa`, and general frameworks — still needs separately validated
kernel-driver, runtime, compiler, and device-execution layers. Fedora is not a
vendor-qualified S30 OS. Installation over the live module and unbind/rebind
remain unapproved until those layers have been examined for safety.

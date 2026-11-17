# Official driver porting record

## Current active source/build (2026-11-17)

The active project `Makefile`, `inc/`, and `src/` identify as S30 driver
package version 3.1.0. The separate `driver-mtgpu-5.2.0/` source tree is not
used by this build. `scripts/build-kernel-driver.sh` copies the active source
and feature-probe helper into a fresh directory under `musa-data`, and pins
the vendor core object to SHA-256
`d0fdaceab37bce7387d376f8a87a2e9dbf210eeccf4f4f7c67f8dee83f4e8200`.

The 2026-11-13 isolated build linked for kernel 7.3.3 with matching vermagic
and the S30 PCI alias, emitting 5,504 compiler/objtool warning lines; that
isolated artifact is not approved for installation as a replacement. A 3.1.0
module is now installed at
`/lib/modules/7.3.3-300.fc45.x86_64/extra/mtgpu.ko` and is loaded and live on
this host, with the S30 bound and the expanded compute/Vulkan tests passing (see the
[current state](current-state-2026-11-17.md)). Do not install over the live
module or unbind/rebind the device; the opaque vendor core still carries
objtool/ENDBR warnings, so do not disable IBT/CET.

## Previous current-kernel build (2026-11-12)

The current host runs kernel `7.3.3-300.fc45.x86_64`. The build script copied
the current production `Makefile`, `inc/`, and `src/` into a fresh directory
under `musa-data/research/builds/mtgpu-7.3.3-300.fc45.x86_64-ZEbZhc/`, removed
stale generated conftest output there, regenerated conftest for 7.3.3, and
linked `mtgpu.ko` successfully. The artifact is 29,712,192 bytes, has
matching 7.3.3 vermagic, and advertises the S30 PCI alias. Its SHA-256 is
`aa2daf0882bf2f44e9d03b10d0aeed8b806ca246d6055ed37bfe7daa3a08dea7`.

Objtool reported 7,451 warning lines, including 1,837 ENDBR-related warnings
and four `missing int3 after ret` warnings in the vendor core. The build is
not load-approved. It was not installed or loaded. Full build output is in
`musa-data/research/musa-fedora-live-validation-2026-11-12/kernel-build-7.3.3.log`;
the detailed Kbuild log is beside the artifact.

Static inspection of the staged core archive found a concrete IBT defect:
412 of its 413 ELF members advertise IBT, but `mt-rm.o`, which contains
`rmapi_alloc`, does not. Disassembly shows `rmapi_alloc` starts with `push
%rbp` and has no `ENDBR64`. The 7.3.3 warning total consists of 4,852
rethunk naked-return warnings, 1,837 ENDBR relocation warnings, 565
retpoline indirect-call warnings, 193 retpoline indirect-jump warnings, and
four missing-`int3` warnings. See the
[reproducible core analysis](/home/axogm/Documents/project/musa-data/research/builds/mtgpu-7.3.3-300.fc45.x86_64-ZEbZhc/ibt-core-analysis.md).
The old boot-loaded module's origin remains unknown, so the matching
`rmapi_alloc` fault name does not prove that it used this exact archive.

The boot-loaded `/lib/modules/7.3.3-300.fc45.x86_64/extra/mtgpu.ko` was an
unowned file with `build_version=3df6cad-dirty` and SHA-256
`b59c446472d7f1b7946efb4d559054b2c2124fa3e4c2a5fa1eab3355ec2f8ddb`. It
does not match the fresh artifact or preserved build artifacts. The boot log
records a Missing ENDBR fault at `rmapi_alloc`; its resident module remains
stuck in `initstate=coming`. The exact disk file was backed up under
`musa-data/research/system-backups/` and removed from the module tree. Its
provenance is unverified; do not load or test it.

The current SDK compile-only, gated GPU, and read-only inventory results are
in [the live host report](live-host-recheck-2026-11-12.md).

## Historical 5.2.0 package source research

The package and experiments below concern the separate official MUSA
5.2.0-server source tree. They do not identify the active S30 3.1.0 source
used by the current isolated build above.

- Vendor package: <code>mthreads-dkms_5.2.0_amd64.deb</code>.
- SHA-256: <code>e3f684b1f7582fa0b399a234b945ad4539c565a46399f8294db91368fa32c62b</code>.
- Package, pristine re-extraction, experimental source, patches, and logs are kept under <code>/home/axogm/Documents/project/musa-data/research/official-musa-5.2.0/</code>.
- The package source PCI table includes <code>1ed5:0102</code> (MTT S30 2-core).
- The complete, experimental source delta is <code>patches/kernel-7.2-experimental-unreviewed.patch</code>. It was dry-run-applied to a pristine package extraction and includes only source/config changes; generated object files and conftest output are excluded.

To repeat the isolated build on this host, apply that patch to a fresh extraction of the package's <code>data.tar.zst</code>, then run from the extracted driver source directory:

<pre><code>make ARCH=x86_64 KERNELVER="$(uname -r)"</code></pre>

The reproduction used Fedora kernel <code>7.2.15-300.fc45.x86_64</code> as it
was installed during the 2026-11-10 investigation. Its command output is saved
as <code>/home/axogm/Documents/project/musa-data/research/official-musa-5.2.0/build-fedora45-reproduction.log</code>.
The unmodified vendor package build exits 2 and produces no
<code>mtgpu.ko</code>; this is distinct from the in-tree compatibility port
below.

## In-tree compile-only port record (2026-11-10)

The production tree builds the mtgpu module against the target kernel:

<pre><code>make ARCH=x86_64 KERNELVER=7.2.15-300.fc45.x86_64 -j2 all</code></pre>

Result: exit 0. A clean rebuild on 2026-11-10 produced <code>mtgpu.ko</code> (29,740,696 bytes, production tree root) with vermagic <code>7.2.15-300.fc45.x86_64 SMP preempt mod_unload</code>, license Dual MIT/GPL, and aliases for PCI ID <code>1ed5:0102</code>. The complete clean-and-build log is <code>/home/axogm/Documents/project/musa-data/research/project-build-logs-2026-11-10/kernel-port-clean-rebuild-2026-11-10.log</code>. No module was installed or loaded.

The output includes the vendor-supplied prebuilt core object. Build success does not establish that its behavior is safe or that the GPU initializes.

### Source and Makefile basis

- Ported sources copied from the reviewed research tree <code>musa-data/research/official-musa-5.2.0/port-experiment-02-clean-repro/usr/src/mtgpu-5.2.0-server</code>: 27 <code>src/*.c</code> files, 5 <code>inc/</code> headers, the new <code>inc/mtgpu/mtgpu_kernel_compat.h</code>, and the conftest script. The unreviewed patch <code>patches/kernel-7.2-experimental-unreviewed.patch</code> was NOT applied to production.
- The production Makefile was kept verbatim except two hunks: <code>CONFTEST_H := $(OBJ_DIR)/inc/conftest.h</code>, and <code>ccflags-y += -include $(OBJ_DIR)/inc/mtgpu/mtgpu_kernel_compat.h</code> after the conftest include.
- conftest was regenerated against the target kernel; the resulting <code>conftest.h</code> is byte-identical to the research tree's (including <code>OS_DRM_CONNECTOR_MODE_VALID_HAS_ENUM_RETURN_AND_CONST_MODE</code> and <code>OS_IOMMU_PAGING_DOMAIN_ALLOCATION_IS_AVAILABLE</code>).

### Iterations

1. <code>kernel-port-iter-01.log</code> — production baseline: fails in <code>src/common/os-interface-drm.c</code> (struct_mutex removed, DisplayPort delay and syncobj signatures changed) and the mode_valid callback.
2. <code>kernel-port-iter-02.log</code> — all ~60 C units compile; linking stops because the top-level <code>if_changed</code> helper is unavailable outside kbuild. Creating the intended symlink to the unchanged vendor core blob resolves this build step.
3. <code>kernel-port-iter-03.log</code> — modpost fails because a misplaced preprocessor guard excludes required <code>os_*</code> helpers.
4. <code>kernel-port-iter-04.log</code> — moving the guard restores those helpers; compile and link succeed.
5. <code>kernel-port-final.log</code> — historical clean rebuild succeeded. Historical port logs are archived under <code>/home/axogm/Documents/project/musa-data/research/project-build-logs-2026-11-10/</code>.

### Semantic decisions

- The IOMMU fix only corrects preprocessor structure to match the vendor's original layout; no code inside either branch was changed, preserving identity-domain ownership and reference-counting semantics.
- The remaining API fixes (mempool, VMA PFN unmap, dma-fence/dma-resv, procfs/VFS, timer, eventfd, pci_resize_resource, access_ok, mode_valid enum signature) come from the reviewed research sources, previously checked against ground truth in <code>/usr/src/kernels/7.2.15-300.fc45.x86_64/include/</code>. No compile-only casts or blind type puns were added in production.
- Non-fatal warnings remain and are recorded in the final log: <code>-Wdiscarded-qualifiers</code> at <code>os-interface.c:1320</code> (pre-existing), <code>-Wundef</code> for JPU_LOG_ENABLE, unused <code>*_value_str</code> helpers, objtool rethunk/retpoline/ENDBR/int3 warnings from prebuilt core-blob code, and BTF skipped (no vmlinux). Zero errors.

## Current-kernel build (isolated copy, 2026-11-11)

At the time of this build investigation, the host ran
<code>7.3.3-300.fc45.x86_64</code>. A copy of the current
production source was built outside the production tree at
<code>/home/axogm/Documents/project/musa-data/research/kernel-port-fedora-7.3.3-build-2026-11-11/</code>.
The first attempt exposed the Linux 7.3 rename from
<code>dma_fence_enable_sw_signaling()</code> to
<code>dma_fence_enable_signaling()</code>. The production compatibility header
now maps the former name to the latter for kernels >= 7.3.0. The isolated copy
was rebuilt with that exact header; <code>make ARCH=x86_64 KERNELVER=7.3.3-300.fc45.x86_64 -j2 all</code>
completed with exit 0 and linked <code>mtgpu.ko</code> with matching 7.3.3
vermagic.

The build emitted thousands of objtool warnings, including proprietary-core
relocations to functions without ENDBR, rethunk warnings, and missing int3
after returns. That kernel has <code>CONFIG_X86_KERNEL_IBT=y</code>.
These warnings block any claim of load safety. The build command did not
install or load the module. A later, unexpected system-level event loaded an
identical module and produced a Missing ENDBR fault; see
<code>docs/driver-loading.md</code>. The full build log is retained in the
research directory. This result proves compilation/linking only.

### Boundary

The 2026-11-10 boot with kernel 7.2.15 was separately reported with loaded
<code>mtgpu</code>/<code>sgpu_km</code> remaining in
<code>initstate=coming</code> after Missing ENDBR control-protection faults.
That is historical. The later 7.3.3 event also reported a Missing ENDBR
fault while the S30 remained unbound. Do not disable IBT/CET or
modify the proprietary core blob's IBT metadata. The experimental blob rewrite
helper is archived under
<code>musa-data/research/disabled-deployment-drafts-2026-11-11/</code> and must
not be used.

## Production-tree compile-only build (2026-11-11)

Command:

<pre><code>make ARCH=x86_64 KERNELVER="$(uname -r)" -j1 all</code></pre>

The first attempt stopped before compiling because `mtgpu-objs` names
`objs/x86_64/mtgpu.core.o` relatively while its Makefile rule declared an
absolute target and used a Kbuild-only `if_changed` helper from the outer
make. The rule now uses the matching relative target and a direct `ln -sf`
recipe. The retry completed with exit 0.

The generated project-root module is 29 MiB, has vermagic
<code>7.3.3-300.fc45.x86_64 SMP preempt mod_unload</code>, and advertises PCI
aliases for the S30 (`1ed5:0102`). Objtool emitted over 7,000 warnings,
including `!ENDBR` data relocations and missing-`int3`-after-return warnings
in the proprietary core object. The kernel config has
<code>CONFIG_X86_KERNEL_IBT=y</code>. Consequently, this is compile/link
evidence only: the module was not installed, signed, or loaded, and it is not
approved for a hardware test. The complete output is archived at
<code>/home/axogm/Documents/project/musa-data/research/kernel-port-fedora-7.3.3-build-2026-11-11/build-production-tree-2026-11-11.log</code>.

## Acceptance gates

A successful module build alone is not evidence that the S30 initializes or
executes MUSA code; driver loading, device initialization, VRAM/DMA access,
toolkit compilation, GPU execution, and recovery behavior each require
separate evidence. On this host those layers have now been separately
exercised for the four verified checks (see the
[current state](current-state-2026-11-17.md)). Any replacement artifact must
still build against the intended kernel, pass static review of the affected
memory-management and synchronization paths, and be exercised in a disposable
test environment before it is installed over the live module.

## Earlier manual clean isolated rebuild (2026-11-12)

To avoid reusing generated objects from earlier builds, the active production
sources were copied to
`/home/axogm/Documents/project/musa-data/research/musa-fedora-validation-2026-11-12/clean-build-7.2.15/`.
`make clean` and `make ARCH=x86_64 KERNELVER=7.2.15-300.fc45.x86_64 -j1 all`
both completed successfully. The resulting `mtgpu.ko` is 29,786,248 bytes,
has matching 7.2.15 vermagic, and advertises the S30 PCI ID. This confirms only
that the port compiles and links against the running kernel.

The clean build emitted 7,451 objtool warnings: 1,837 ENDBR-related warnings
and four missing-`int3`-after-return warnings, in addition to other objtool
warnings. These come from the opaque vendor core linked into the module. The
module was not installed, signed, loaded, or tested on the S30. The complete
build log is
`/home/axogm/Documents/project/musa-data/research/musa-fedora-validation-2026-11-12/clean-build-7.2.15-build.log`.
The compiler, readiness, gated hardware-test, and `musaInfo` logs are archived
beside it.

## Fresh-build-script verification (2026-11-12)

The build script was verified from the production tree with a separate fresh
source copy and regenerated conftest. It linked a 29,783,016-byte module for
`7.2.15-300.fc45.x86_64`, with matching vermagic and S30 aliases. The complete
script output is
`/home/axogm/Documents/project/musa-data/research/musa-fedora-validation-2026-11-12/project-build-script-clean-tree-7.2.15.log`;
the module and detailed build log are under
`/home/axogm/Documents/project/musa-data/research/builds/mtgpu-7.2.15-300.fc45.x86_64-yfPP4w/`.
Objtool reported 7,460 warnings, including 1,837 ENDBR-related warnings and
four missing-`int3`-after-return warnings. This is compile/link evidence only;
the output was not installed, signed, loaded, or tested on the S30.

The Makefile reads the adapted conftest helper from
`musa-data/research/official-musa-5.2.0/port-experiment-02-clean-repro/`.
The pristine package helper lacks several Linux API probes needed by this
Fedora compatibility port; the generated probe header is created afresh in
the isolated build directory.

## Legacy S30 desktop driver build probe (2026-11-12)

The `mtgpu-1.0.0` source bundled with the official
`musa_2.1.1-Ubuntu-dev_amd64.deb` package was copied to
`/home/axogm/Documents/project/musa-data/research/builds/mtgpu-mobi-2.1.1-7.3.3-2026-11-12/`
and built with:

<pre><code>make ARCH=x86_64 KERNELVER=7.3.3-300.fc45.x86_64 -j2</code></pre>

The build exited 2 before linking. Compile errors include changed
DisplayPort helper signatures, `vm_flags` becoming read-only, `proc_ops`,
removed PCI aperture/error-reporting and IOMMU helpers, and a changed
`pci_resize_resource()` signature. No `mtgpu.ko` was produced, installed, or
loaded. The full log is `build.log` in that research directory. This is a
separate old desktop driver line; the production port continues to use the
newer MUSA 5.2.0 source and its separate compile-only evidence above.

The bundled legacy `mtgpu_core.o_binary` is also unsafe on the current IBT
kernel. Static inspection found no GNU property notes or `ENDBR64`; objtool's
dry-run scan reported 362 relocations to `!ENDBR` targets, 2,592 rethunk
naked-return findings, and 238 retpoline indirect-call findings. The complete
analysis and exact artifact hash are in the
[legacy core IBT report](/home/axogm/Documents/project/musa-data/research/official-mobi-maliang-2.1.1/legacy-core-ibt-analysis-2026-11-12.md).
Porting its visible Linux API calls would not resolve the binary core's IBT
defects.

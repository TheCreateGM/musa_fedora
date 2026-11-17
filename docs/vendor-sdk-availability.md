# MUSA driver and SDK availability

Current live host state is recorded in the
[2026-11-13 recheck](live-host-recheck-2026-11-13.md). Vendor/package research
below was reviewed 2026-11-12. The staged official Mthreads 5.2.0 source recognizes the
S30 PCI ID. This is source-level recognition, not runtime qualification.
The current public SDK catalogue lists MUSA 5.2.0 Linux packages for MTT
S5000 and does not list S30 among its product filters or package targets.
Older catalog entries list selected S3000/S4000/S80 and CPU/OS combinations,
but none lists S30. These package listings do not qualify Fedora 45 + S30 or
establish MUSA 5.2.0 runtime/driver support for S30. The architecture target
evidence is assessed separately below. See the
[official MUSA SDK catalogue](https://developer.mthreads.com/resources/musa-sdk?tab=overview).

The official [Linux desktop driver support matrix](https://docs.mthreads.com/driver-linux-pc/driver-linux-pc-doc-online/introduction/)
does list S30 for selected Phytium, Zhaoxin, Hygon, and Loongson CPU plus
Kylin, UOS, or NFS OS combinations. Its Intel row lists Ubuntu with S80 only;
it does not list Intel + S30 or Fedora. The same page says desktop driver
packages are integrated into supported Kylin/UOS releases or available by
request from Moore Threads support. This host's Intel + Fedora 45 + S30 setup
is therefore outside the currently documented desktop support combinations.
The official version 2.5.0 release notes for Phytium, Loongson, and the
Zhaoxin/Hygon platforms document DKMS support for kernels 4.19 through 5.15.
Those release notes list S30 on their supported desktop platforms; they do not
qualify this Intel/Fedora host or its 7.3.3 kernel. See the official
[Phytium](https://docs.mthreads.com/driver-linux-pc/driver-linux-pc-doc-online/releasenotes/arm_releasenote/),
[Loongson](https://docs.mthreads.com/driver-linux-pc/driver-linux-pc-doc-online/releasenotes/loongarch_releasenote/), and
[Zhaoxin/Hygon](https://docs.mthreads.com/driver-linux-pc/driver-linux-pc-doc-online/releasenotes/x86_releasenote/)
release notes.

## Older official S30 desktop driver package study

The local reference archive contains the Moore Threads
[`MobiMaliangSDK`](https://github.com/MooreThreads/MobiMaliangSDK) repository
and its `musa_2.1.1-Ubuntu-dev_amd64.deb` package. Its source is versioned as
`mtgpu-1.0.0` and includes PCI IDs `1ed5:0102` and `1ed5:0103`; this confirms
that the older desktop driver knew about the two S30 variants. The package
README specifies Ubuntu 20.04. It supplies OpenCL and graphics libraries, but
not the MUSA 5.2 runtime used by the staged toolkit. The package also includes
an opaque 2,742,960-byte kernel core object. A dry-run IBT analysis with the
Fedora 7.2.15 objtool reports relocations to functions without ENDBR. This
older package therefore does not resolve the current Fedora/IBT blocker. It
was extracted only under
`/home/axogm/Documents/project/musa-data/research/official-mobi-maliang-2.1.1/`;
it was not installed or loaded.

The archived package inventory contains Mesa/OpenCL graphics libraries but no
`mcc`, `musaInfo`, or `libmusart`; it is not a complete MUSA compute SDK. The
official repository's [installation README](https://github.com/MooreThreads/MobiMaliangSDK/blob/main/README.md)
explicitly starts from Ubuntu 20.04 and installs that DEB. The official
[S30 installation guide](https://docs.mthreads.com/s30/s30-doc-online/install_guide/)
describes UOS/Kylin images and platform-specific BIOS requirements, but gives
no Fedora installation path.

The MobiMaliangSDK installation guide also documents a companion PyTorch
container at `registry.mthreads.com/mcconline/musa-pytorch-release-public`.
Its registry configuration, captured by digest under
`/home/axogm/Documents/project/musa-data/research/official-mobi-maliang-2.1.1/oci-musa-pytorch-release-public/`,
records Ubuntu 20.04, MUSA Toolkit RC1.4.0, muDNN RTM2.1.1, and Python 3.8
wheels for PyTorch `2.0.0a0+gitc263bd4` and `torch_musa` 1.0.0. This is
useful evidence of a historical integrated user-space stack, separate from
the host driver package. The image was created in 2023; only its registry
metadata was inspected, not its multi-gigabyte layers. The image history does
not state that it supports S30, and it does not establish compatibility with
Fedora 45, kernel 7.3.3, or the current 5.2.0 SDK. No image or GPU execution
was performed.

The four host files in `/usr/lib/firmware/mthreads/` are unowned by Fedora
RPMs. Each byte-for-byte matches the corresponding firmware file in the
archived official 2.1.1 package: `musa.fw.1.0.0.0`, `musa.fw.1.1.0.0`,
`musa.fw.2.0.0.0`, and `musa.sh.1.0.0.0`. This establishes provenance to that
archive only; it does not show that the installed files or any matching driver
have been validated on the current kernel or by the 5.2.0 runtime. They were
left untouched.

An isolated build of the archived `mtgpu-1.0.0` DKMS source against the live
7.3.3 headers exited 2 before module linking. The first compile pass reports
kernel API breaks in DisplayPort helpers, PCI IRQ/aperture/resize helpers,
read-only `vm_flags`, procfs operations, timer and IOMMU APIs. The build log
and copied source are under
`/home/axogm/Documents/project/musa-data/research/builds/mtgpu-mobi-2.1.1-7.3.3-2026-11-12/`.
No module was installed or loaded. This test rules out the unmodified old
desktop package as a buildable 7.3.3 driver; it does not test GPU behavior.

This distinction matters: the older graphics/OpenCL package, the S30 PCI
table, and the newer MUSA 5.2 compiler/runtime are different compatibility
claims. The reviewed public package catalogue still does not identify S30 as
a target for the current MUSA SDK. The official
[`torch_musa` issue #134](https://github.com/MooreThreads/torch_musa/issues/134)
also records a maintainer response that S30 is outside that framework's
support scope.

## Historical host state (2026-11-12)

The [2026-11-12 host report](live-host-recheck-2026-11-12.md) records the
earlier state; the current live report is linked at the top of this document.
Fedora 45 runs kernel <code>7.3.3-300.fc45.x86_64</code>; the S30 is visible
but unbound, and Intel UHD 770 (<code>i915</code>) is the active display GPU.
An unowned <code>mtgpu.ko</code> failed during boot with an IBT
control-protection fault at <code>rmapi_alloc</code>. The loaded module remains
<code>initstate=coming</code>, taint <code>OE</code>. Its exact disk copy was
backed up under <code>musa-data</code> and removed from the current kernel's
module tree; the resident module remains until reboot. The S30 has no
<code>/dev/mtgpu.0</code> node or associated DRM render node.

A fresh isolated 7.3.3 driver build linked but emitted 7,451 objtool warning
lines and is not approved to load. The staged official compiler passed
compile-only checks. The current read-only inventory and gated GPU test exit
3; no runtime query or hardware test was run.

## Official source and S30 ID

The official Ubuntu 22.04 package index at <code>https://dl.mthreads.com/repo/repository/ubuntu2204/pool/jammy/amd64/</code> responded during this review. The 5.2.0 package <code>mthreads-dkms_5.2.0_amd64.deb</code> was downloaded and extracted under:

<code>/home/axogm/Documents/project/musa-data/research/official-musa-5.2.0/</code>

Its DKMS source contains <code>DEVICE_ID_MTT_S30_2_Core (0x0102)</code> and a matching <code>mtgpu</code> PCI table entry. This exactly matches the installed card’s device ID. This is meaningful driver source, unlike the project-local binder and the archived third-party <code>dixyes/mtgpu-drv</code> project.

## S30 compiler-target evidence

Moore Threads' 2025 [IPO prospectus](https://static.cninfo.com.cn/finalpage/2025-11-28/1224831699.PDF)
identifies the S30 as a SuDi architecture product. The official [MUSA 4.3
hardware architecture guide](https://docs.mthreads.com/musa-sdk/version-4.3.x/programming_guide/Chapter02/)
maps SuDi to <code>MP_10</code>, and the [MCC user
manual](https://docs.mthreads.com/musa-sdk/version-4.3.x/api/mcc_um.zh-CN/)
lists <code>mp_10</code> as an offload target. The 5.2.0 DKMS source also
routes S30 PCI ID <code>0x0102</code> through <code>sudi_drvdata</code> in
<code>src/mtgpu/mtgpu_drv.c</code>. Together, these official sources support
<code>mp_10</code> as the S30 code-generation target. The staged official MUSA
5.2.0 compiler also accepts <code>--offload-arch=mp_10</code> and emits the
compile-only vector-add executable. Neither the 5.2.0 code nor any runtime
was executed on the S30. This architecture mapping does not establish that
the 5.2.0 driver, firmware, runtime, or libraries support S30. A dated source
and version assessment is in
<code>musa-data/research/official-musa-5.2.0/s30-mp10-target-evidence-2026-11-12.md</code>.

Moore Threads’ public SDK 5.2.0 installation guide specifies Ubuntu 22.04 and kernel 5.15.0-105-generic for its documented server path. The desktop matrix likewise does not qualify Fedora 45 or Intel + S30. The source’s S30 ID therefore establishes device recognition in that driver source, not Fedora compatibility or a working MUSA stack. The vendor’s <code>torch_musa</code> issue tracker also states that S30 is outside that project’s support scope.

## Isolated official userspace packages

The official 5.2.0 compiler, assembler, runtime, development headers, toolkit configuration, compute library, and MTGPU DRM library were downloaded from the same Ubuntu 22.04 package repository and extracted only under <code>musa-data/research/official-musa-5.2.0/packages/</code>. None were installed on Fedora. SHA-256 values for every staged package match the official <code>Packages.gz</code> index stored in the adjacent <code>apt-metadata/</code> directory.

| Package | SHA-256 |
|---|---|
| <code>mtcc-5-2_5.2.0_amd64.deb</code> | <code>22a37d7af78ea5bd25e72b7d9549bb02a2512c852617c2e87ecf6a27b5d242d1</code> |
| <code>mtxas-5-2_5.2.0_amd64.deb</code> | <code>75ac91d51724ac0269fd4e7252ec22a84545f0585acd66f88aea2b27d4315fb9</code> |
| <code>musa-musart-5-2_5.2.0_amd64.deb</code> | <code>ba3e3a4f52b68602382f81542625f544ed65cc0fe3f9cb64e655b1167ac34a5f</code> |
| <code>musa-musart-dev-5-2_5.2.0_amd64.deb</code> | <code>7e235c9efc2d85adcc752870cdc955af77c3591ed767ec6775d942ef21408bdf</code> |
| <code>libmthreads-compute_5.2.0_amd64.deb</code> | <code>254e61c367e10692cfb228e11ea4e3c3401be38b5ca71f372e6042c6d7436924</code> |
| <code>libdrm-mtgpu_5.2.0_amd64.deb</code> | <code>8a5c07b5789721ba6c20b5128a5f06e71f4dee03d866edc6102df2be58bb40c9</code> |
| <code>musa-toolkit-5-2-config-common_5.2.0_amd64.deb</code> | <code>919fc728192df8cb1f12f99f91e38091bc7fc8ae3d664002e4010d4fe93216bd</code> |
| <code>musa-toolkit-5-2_5.2.0_amd64.deb</code> | <code>f6a29a25f1d18c4595ba5bca6ffe942109acfa861b07ac1f236a54bbb7abb38d</code> |
| <code>musa-compiler-5-2_5.2.0_amd64.deb</code> | <code>6d39e1e4a53e8fb8d5bcde6a56b1cdc0aeac594d21c717e9fec0b9d1d2ef6657</code> |

Using the extracted 5.2.0 toolchain, a vector-add program was compiled and linked as an x86-64 executable. <code>readelf</code> confirmed <code>.mtFatBinSegment</code>, and its embedded offload bundles identify <code>mp_21</code>, <code>mp_22</code>, <code>mp_31</code>, and <code>mp_32</code>. This verifies only that the vendor compiler and host linker produce an executable; it does not identify which target, if any, is the S30 target. The extracted compiler header mentions <code>mp_31</code>, and the DKMS table recognizes the S30 PCI ID, but the reviewed vendor materials do not map the S30 model to any of these targets. The test executable was not run.

The installed-tree <code>musaInfo</code> was queried during the 2026-11-12
check and reported <code>no MUSA-capable device is detected</code> (API error
100). The staged official compiler passed compile-only checks, including the
default bundle and explicit <code>mp_10</code> build, but no generated
executable or GPU kernel was run.

## Isolated build result

The official 5.2.0 DKMS source was built outside the production tree against
kernel <code>7.2.15-300.fc45.x86_64</code> during the 2026-11-10 investigation.
No module was installed or loaded from that isolated build. The unmodified
package failed on kernel API differences; research-only changes let compilation
proceed farther, but the driver core still failed on additional
incompatibilities. This is historical and does not describe the current
7.3.3 build.

The unmodified vendor-package build log is at <code>/home/axogm/Documents/project/musa-data/research/official-musa-5.2.0/build-fedora45-initial.log</code>. It records kernel API incompatibilities including changed mempool, procfs/VFS, timer, fence, eventfd, PCI, IOMMU, and access-check interfaces. This failure applies to the isolated, unmodified package build. The in-tree port first completed a compile/link build for kernel 7.2.15. On
2026-11-11, a separate copy of the current production source was built for
kernel 7.3.3 after adding a compatibility alias for the renamed
<code>dma_fence_enable_signaling</code> API. That build completed, but emitted
extensive objtool warnings, including <code>!ENDBR</code> relocations in the
prebuilt vendor object. Its output and log are under
<code>/home/axogm/Documents/project/musa-data/research/kernel-port-fedora-7.3.3-build-2026-11-11/</code>.
The build is not installed, loaded, or runtime-validated.

On 2026-11-11, the production tree was also built against the running 7.3.3
kernel. The initial attempt exposed an absolute/relative target mismatch for
the vendor-core object; the Makefile rule was corrected, and the retry
completed. It emitted over 7,000 objtool warnings, including
<code>!ENDBR</code> relocations and missing-<code>int3</code>-after-return
warnings. The module has matching vermagic and an S30 PCI alias, but was not
installed or loaded. Its full log is archived alongside the isolated build.

Research patches and the extracted vendor source remain under <code>musa-data</code>. They are not production patches and have not been reviewed for runtime or memory-safety correctness.

## Compatibility decision

The source recognizes the S30 PCI ID, and compile/link builds exist for
kernels 7.2.15 and 7.3.3. The fresh 7.3.3 build reports 1,837 ENDBR-related
warnings and four missing-<code>int3</code> warnings. A separate boot-loaded
module produced an actual IBT control-protection fault. No artifact is
load-approved. Official architecture materials support <code>mp_10</code> as
the S30 code-generation target, but no hardware result confirms execution and
the public documentation does not qualify Fedora 45 + Intel + S30.

Static inspection of the staged official 5.2.0 core archive found mixed IBT
metadata: 412 of 413 ELF members advertise IBT, while `mt-rm.o`—the member
containing `rmapi_alloc`—does not, and that function entry has no `ENDBR64`.
No source definition for `rmapi_alloc` was found in the staged official
driver/source trees. This does not prove that the separate historical
boot-loaded module used the same archive; its origin is unverified. See the
[core-object analysis](/home/axogm/Documents/project/musa-data/research/builds/mtgpu-7.3.3-300.fc45.x86_64-ZEbZhc/ibt-core-analysis.md)
for the archive hash, exact objtool counts, and reproduction commands.

The earlier 7.2.15 boot with `ibt=off` and its persistent boot-entry cleanup
are historical. The current 7.3.3 boot has IBT enabled; the driver faulted
under that protection. Do not disable IBT/CET, alter vendor-core metadata,
load an unreviewed module, install Ubuntu packages directly, or run GPU tests
against the current resident module.

The missing vendor confirmations are collected in the local
[support request draft](vendor-support-request-draft.md). It has not been sent.

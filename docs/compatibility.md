# Moore Threads compatibility assessment

The latest live host evidence is in the
[2026-11-13 report](live-host-recheck-2026-11-13.md). Vendor-support research
below was reviewed on 2026-11-12.

## Current host status

- Fedora Linux 45 Workstation, x86_64, kernel `7.3.3-300.fc45.x86_64`.
- The MTT S30 is visible at `0000:01:00.0`, PCI ID `1ed5:0102`, and remains
  unbound. The Intel UHD Graphics 770 (`i915`) is the active display GPU.
- An unowned `mtgpu.ko` loaded during boot and failed in module initialization
  with a kernel IBT control-protection fault at `rmapi_alloc`. The resident
  module remains `initstate=coming`, taint `OE`; do not access or unload it.
- A different unowned module file is currently present in the 7.3.3 module
  tree than the one quarantined on 2026-11-12. Its origin is unknown.
- `/dev/mtgpu.0` is absent, and no DRM render node is associated with the S30.

## Vendor support boundary

The current [official MUSA SDK catalog](https://developer.mthreads.com/resources/musa-sdk?tab=overview)
lists the 5.2.0 Linux packages for MTT S5000; it does not list S30 as a 5.2.0
package target. The [S30 product documentation](https://docs.mthreads.com/s30/s30-doc-online/introduction/)
lists UOS, Kylin, Fangde, Ubuntu, and Windows environments, not Fedora. It
describes the S30 as first-generation MUSA. The staged 5.2.0 DKMS source
contains the S30 PCI ID, which confirms device recognition in that source
table only.

The older official MobiMaliang 2.1.1 desktop package also recognizes S30, but
its bundled `mtgpu_core.o_binary` has no IBT notes or `ENDBR64`; objtool
reports 362 `!ENDBR` relocations, and its unmodified source does not compile
against kernel 7.3.3. It is not a safe substitute. See the
[legacy driver/core assessment](driver-porting.md#legacy-s30-desktop-driver-build-probe-2026-11-12).

Moore Threads' [2025 product filing](https://static.cninfo.com.cn/finalpage/2025-11-28/1224831699.PDF)
identifies S30 as SuDi architecture. The official [MUSA 4.3 architecture
guide](https://docs.mthreads.com/musa-sdk/version-4.3.x/programming_guide/Chapter02/)
maps SuDi to `MP_10`, and its [MCC user
manual](https://docs.mthreads.com/musa-sdk/version-4.3.x/api/mcc_um.zh-CN/)
lists `mp_10` as an offload target. This supports `mp_10` as the S30
code-generation target. The staged 5.2.0 compiler also emits `mp_10`, but
neither executable was run; 5.2.0 runtime and device compatibility remain
unverified. See the [target evidence record](../../musa-data/research/official-musa-5.2.0/s30-mp10-target-evidence-2026-11-12.md).

Framework support is separate from the kernel driver and runtime. A working
`torch_musa` or MUSA SDK compiler does not prove the S30 driver works on
Fedora.

## Build and runtime evidence

- A fresh isolated build linked for kernel 7.3.3, with matching vermagic and
  an S30 PCI alias. The latest S30 3.1.0 build logged 5,504
  compiler/objtool warning lines. The artifact is not installed or
  load-approved.
- The staged official MUSA 5.2.0 compiler passed compile-only checks for its
  default `mp_21`–`mp_32` fat binary and explicit `mp_10` output. Runtime
  headers passed syntax checking. Neither executable ran.
- The read-only verifier and gated GPU script both exited 3. The S30 is
  unbound and the resident module is not fully initialized. No runtime query,
  VRAM allocation, DMA transfer, GPU kernel, display, or audio test ran in
  this validation pass.
- An earlier `musaInfo` query reported error 100, “no MUSA-capable device is
  detected.” That older observation is not a current runtime test.

## Compatibility conclusion

The S30 PCI ID is recognized by staged vendor source, and the SDK compiler
can produce MUSA device-code bundles. Fedora 45 + kernel 7.3.3 + MTT S30 is
not qualified: the opaque core fails IBT initialization, objtool reports
extensive safety warnings, version-specific S30 runtime support is
unestablished, and no physical GPU test has passed. Do not disable IBT/CET,
modify vendor-core metadata, or load another MTGPU module. A vendor-provided
IBT-safe core or complete source and an S30-qualified driver/runtime matrix
are needed before hardware validation.

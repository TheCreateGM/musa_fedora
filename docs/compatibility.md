# Moore Threads compatibility assessment

The latest live host evidence is in the
[current state](current-state-2026-11-20.md). Vendor-support research below was
reviewed on 2026-11-12.

## Historical host status — 2026-11-17 (superseded)

The current host is described in the 2026-11-20 report linked above. The
loaded-driver and hardware results in this section are historical.

- Fedora Linux 45 Workstation, x86_64, kernel `7.3.3-300.fc45.x86_64`.
- The MTT S30 is at `0000:01:00.0`, PCI ID `1ed5:0102`, bound to the vendor
  `mtgpu` 3.1.0 driver (`initstate=live`). `/dev/mtgpu.0` and the S30 DRM
  render node `/dev/dri/renderD129` are present.
- IOMMU is enabled (`intel_iommu=on iommu=pt`); the S30 is in IOMMU group 11.
- The Intel UHD Graphics 770 (`i915`) remains the active display GPU; the S30
  does not drive display output. A limited headless Vulkan device/buffer/memory
  path passes.
- MUSA runtime 1.5.4 detects the S30 (`musaInfo`), the vendor `muMemcpyTest`
  passes, the host runtime API passes (`tests/musart-smoke.c`), the 5.2.0
  `mcc` `mp_10` vector-add and compute suite ran on the S30, and a headless
  Vulkan device/buffer/memory probe passed before the later OpenCL kernel Oops
  recorded in the [current state](current-state-2026-11-17.md).

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
lists `mp_10` as an offload target. This is now confirmed on hardware: the
5.2.0 compiler built `tests/musa-vector-add.mu` for `--offload-arch=mp_10`
and it ran on the S30. The supported combination on this host is driver
`mtgpu` 3.1.0 + MUSA runtime 1.5.4 + MUSA 5.2.0 compiler (`mp_10`). The
separate 5.2.0 *runtime* is not compatible with the 3.1.0 driver ABI
(`sizeof(struct musaDeviceProp)` is 728 bytes in 1.5.4 vs 1032 in 5.2.0) and
must not be substituted for 1.5.4. See the [target evidence
record](../../musa-data/research/official-musa-5.2.0/s30-mp10-target-evidence-2026-11-12.md).

Framework support is separate from the kernel driver and runtime. A working
`torch_musa` or MUSA SDK compiler does not prove the S30 driver works on
Fedora.

## Build and runtime evidence

- A fresh isolated build linked for kernel 7.3.3, with matching vermagic and
  an S30 PCI alias. The latest S30 3.1.0 build logged 5,504
  compiler/objtool warning lines; the resulting module is installed and live
  (`initstate=live`).
- The official MUSA 5.2.0 compiler passed compile-only checks for its default
  `mp_21`–`mp_32` fat binary and explicit `mp_10` output, and went on to
  build `tests/musa-vector-add.mu` for `mp_10`, which ran on the S30
  (`PASS: 1048576 float adds verified on S30`).
- The verifier (`scripts/verify-musa.sh`) reports **13 passed, 0 failed, 1
  skipped**. The skip is Moore Threads OpenCL; the loader sees MVGAL and Intel
  platforms, but no MT ICD. The eight `mp_10` compute checks and the limited
  headless Vulkan buffer/memory probe passed on the S30 before the separate
  archived OpenCL ICD compute probe caused an `mtgpu` kernel Oops. Do not rerun
  that probe; see the failure record in the current state.
- The older observation that `musaInfo` returned error 100, “no MUSA-capable
  device is detected,” applied while the S30 was unbound; it is no longer the
  current state.

## Compatibility conclusion

The S30 PCI ID is recognized by vendor source, the vendor `mtgpu` 3.1.0
driver is live on Fedora 45 + kernel 7.3.3, and a hardware-verified MUSA path
exists (device discovery, vendor copy test, host runtime API, eight compute
operations, vector-add, and a limited headless Vulkan path) from before the
archived OpenCL experiment caused a kernel Oops. Moore Threads OpenCL is
unavailable in the default loader, and the archived ICD must not be used. This
does not validate display rendering, OpenGL, video, `torch_musa`, or general frameworks.
Fedora is not a
vendor-qualified S30 OS, so the stack is verified on this host but not
vendor-qualified. Do not disable IBT/CET, modify vendor-core metadata, or load
another MTGPU module. Broader features and an S30-qualified driver/runtime
matrix are needed before extending the claim.

## 2026-11-26 validation update (Fedora 45 / kernel 7.3.4)

- Host: Fedora 45, kernel `7.3.4-300.fc45.x86_64`, MTT S30 `[1ed5:0102]` bound to `mtgpu` 3.1.0, IOMMU enabled.
- Validated package set on host: `mtgpu-dkms-3.1.0-15.fc45`, `mtgpu-firmware-3.1.0-5.fc45`, `musa-runtime-1.5.4-8.fc45`, `musa-runtime-va-1.5.4-8.fc45`, `musa-sdk-5.2.0-7.fc45`, `musa-toolkit-3.1.0-5.fc45`.
- Live verification: `scripts/check-musa-env.sh` PASS; `scripts/verify-musa.sh --skip-memcpy` **15 passed, 0 failed, 2 skipped** (muMemcpyTest skipped by request; no Moore Threads OpenCL ICD).
- Container install + real-GPU passthrough (podman, disposable Fedora 45 container, host packages untouched): `scripts/test-container-install.sh --skip-memcpy` **13 passed, 0 failed**, `TEST_CONTAINER_INSTALL_STATUS: SUCCESS`.
- COPR `axogm/musa-fedora` enabled chroots: `fedora-43/44/45/rawhide/eln-x86_64`. Validated local-spec packages built successfully for fedora-43-x86_64: mtgpu-firmware 11392834, mtgpu-dkms 11392833, musa-runtime 11392835, musa-toolkit 11392838, musa-sdk 11392839.
- Legacy COPR packages `libmusa` and `libdrm-mtgpu` (5.2.0-1 upload sources, no latest builds) are **not compatible** with the validated 3.1.0+1.5.4 stack and must not be built/installed into it.
- Scope boundary unchanged: verified MUSA compute/runtime/compiler/VAAPI paths on this host; no claim of vendor-qualified Fedora support, OpenCL MT ICD, display rendering, or general framework compatibility without evidence.

## 2026-11-27 second-pass re-verify (Fedora 45 / kernel 7.3.4)

Same boot as [hardware-validation-2026-11-27-reverify](hardware-validation-2026-11-27-reverify.md); every gate was re-run from scratch in a later session of that boot and reproduced its result.

- Validated package set **currently** on the host: `mtgpu-dkms-3.1.0-16.fc45`, `mtgpu-firmware-3.1.0-5.fc45`, `musa-runtime-1.5.4-8.fc45`, `musa-runtime-va-1.5.4-8.fc45`, `musa-sdk-5.2.0-7.fc45`, `musa-toolkit-3.1.0-6.fc45`, `musa-cuda-compat-1.0.0-1.fc45`, `mvgal-0.7.48-1.fc45` (`rpm -qa`). The 3.1.0-15 / 3.1.0-5 figures above were correct for 2026-11-26 and are kept as history.
- `scripts/compute-mode-boot-verify.sh --wait 5` **14 passed / 0 failed / 0 warnings** — udev loaded `mtgpu` on its own (`initstate=live`), `0000:01:00.0` is bound, `/dev/mtgpu.0` and `renderD129` exist, gdm/graphical.target/gnome-shell are live with the driver up, no blacklist anywhere, no `mtgpu` BUG/Oops/NULL-deref and no `Invalid cmd_type` entry this boot, `musaInfo` reports the S30.
- `scripts/check-musa-env.sh` **PASS**; `scripts/verify-musa.sh --skip-memcpy` **15/0/2**; `scripts/s30-perf.sh` **PASS** (best triad 52.0 GB/s at blockDim 1024, two-stream pipeline 1.14x with verified output, 1 cache-config SKIP); `scripts/hardware/test-vaapi.sh` **14/0/0**; `tests/cuda-compat/run.sh` **7/7**; `tests/musa-infer/run.sh` **PASS**; `scripts/test-container-install.sh --skip-memcpy` **13/0**; `scripts/verify-copr-chroots.sh` **24/24** (see next bullet); `scripts/test-toolchain.sh` compile-only **PASS**.
- The COPR gate itself was fixed: its chroot map covered only four of the five chroots enabled in `musa-fedora` (`fedora-43` was added on 2026-11-26 after the map was written), so an earlier **20/20** did not actually assert `fedora-43` there. The map now lists all five chroots for both projects, the gate compares the map against the COPR API's `chroot_repos` on every run so future drift fails instead of shrinking coverage, and the re-run reports **24/24** — all seven artifacts at the expected versions are published for `fedora-43-x86_64` in `musa-fedora` as well.
- `apps/musa-infer --bench` on TinyLlama-1.1B-Chat Q8_0: prompt 12.59 tok/s, generate **12.50 tok/s** over 16 tokens, which matches the figure recorded earlier the same day.
- Journal: **0** `mtgpu` fault/oops/bug entries for the whole boot after all of the above.
- The two transient `musa-cuda-compat` COPR failures (11393017, 11393009) were root-caused in their `builder-live.log.gz`: `%build` died with `make: command not found`. The spec fix landed in `f8a5c98` (`BuildRequires: make gcc musa-runtime = 1.5.4`); builds 11393082/11393083 supersede them and succeeded in all five x86_64 chroots.
- Local RPM signing closed: `signed packages 0/8` → `8/8`, and after a `pkexec rpm --import` of the project key a plain `rpm -K` reports `digests signatures OK` for all eight. See [rpm-signing](rpm-signing.md).

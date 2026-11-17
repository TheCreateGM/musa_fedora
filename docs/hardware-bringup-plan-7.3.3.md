# MTT S30 hardware validation status

Hardware bring-up has succeeded on this host. The vendor `mtgpu` 3.1.0 driver
is loaded and live (`initstate=live`) with the MTT S30 bound at
`0000:01:00.0` (`1ed5:0102`), and a hardware-verified MUSA path exists
(device discovery, vendor copy test, host runtime API, eight compute
operations, vector-add, and a limited headless Vulkan path). The `mtgpu` build still contains an opaque vendor core
object with unresolved objtool/ENDBR warnings, so do not disable IBT/CET,
install over the live module, or unbind/rebind the device.

The earlier proposed `ibt=off` and module-load procedure was unsafe and has
been removed. The scripts with those names now refuse to modify boot settings
or load modules. The revert helper only removes a persistent `ibt=off`
argument from the explicitly selected kernel entry, after verifying the entry
and rechecking the result.

## Live host state (2026-11-17)

See the [current state](current-state-2026-11-17.md). The host runs Fedora 45
kernel `7.3.3-300.fc45.x86_64`; the S30 is bound to `mtgpu` 3.1.0 and
`/dev/mtgpu.0` plus the render node `/dev/dri/renderD129` are present. IOMMU
is enabled (`intel_iommu=on iommu=pt`, S30 in group 11). Intel UHD 770
(`i915`) remains the active display GPU; the S30 does not drive display output.

MUSA runtime 1.5.4 detects the S30, the vendor `muMemcpyTest` passes, the host
runtime API (`tests/musart-smoke.c`) passes, and a 5.2.0 `mcc` `mp_10`
vector-add and the `mp_10` compute suite ran on the S30, and a limited
headless Vulkan probe passed. `scripts/verify-musa.sh` reports **13 passed,
0 failed, 1 skipped** before a later archived OpenCL ICD compute probe caused
an `mtgpu` kernel Oops; the Moore Threads OpenCL ICD is absent from the default
loader. Do not rerun the archived probe. See the [current state](current-state-2026-11-17.md).

The isolated fresh S30 3.1.0 build for 7.3.3 emitted 5,504 compiler/objtool
warning lines and is not approved for installation as a replacement for the
live module. The hardware tests cover only the listed compute operations and
headless Vulkan device/buffer/memory path before the Oops; Moore Threads
OpenCL is unavailable, and the archived ICD probe is unsafe. Display
rendering, OpenGL, video, `torch_musa`, and VM passthrough remain untested.

## Historical host state (2026-11-13, superseded)

The 2026-11-13 recheck recorded a boot-loaded, unowned `mtgpu` module that
faulted under kernel IBT at `rmapi_alloc` and remained in `initstate=coming`,
with the S30 unbound. That state is historical and has been superseded by the
live state above.

See [driver loading](driver-loading.md), [driver porting](driver-porting.md),
and [IBT recovery status](ibt-off-recovery.md) for the background and the
remaining safety boundaries.

# Feature boundary and limitations

The current state is recorded in
[current-state-2026-11-17.md](current-state-2026-11-17.md).

## What is verified today

The S30 compute stack is live on this host. The vendor `mtgpu` 3.1.0 driver is
loaded and reports `initstate=live`; `/dev/mtgpu.0` and the S30 DRM render node
`/dev/dri/renderD129` are present. IOMMU is enabled
(`intel_iommu=on iommu=pt`). MUSA runtime 1.5.4 `musaInfo` detects one device,
the vendor `muMemcpyTest` passes, the host runtime-API test
(`tests/musart-smoke.c`) passes, and a MUSA 5.2.0 `mcc` `mp_10` vector-add ran
on the S30 with all 1,048,576 results verified. The `mp_10` compute suite
passed SAXPY, shared reduction, atomics, constant memory, device math, 2D
launch, D2D copy, and FP64 checks. A headless Vulkan probe created the S30
device and queue, allocated a buffer, and mapped/read back host-visible memory.
The recorded `scripts/verify-musa.sh` run reports 13 passed, 0 failed, 1
skipped; the skip is Moore Threads OpenCL because no MT ICD is present in the
default loader configuration. A later experiment with an archived MobiMaliang
2.1.1 ICD triggered a kernel Oops in `PVRSRVRGXFlushComputeDataKM` during an
OpenCL compute request. Do not install or rerun that ICD/probe; see the
[current state and failure record](current-state-2026-11-17.md).

The desktop still runs on Intel UHD Graphics 770 (`i915`); the S30 does not
drive display output here. Only a headless Vulkan device/buffer/memory path
was exercised.

## What is not verified or supported

The listed MUSA compute operations and limited headless Vulkan
device/buffer/memory path passed before the OpenCL Oops described above.
Moore Threads OpenCL is unavailable: no MT ICD is installed in the default
loader, and the archived 2.1.1 ICD caused a kernel Oops on this driver. OpenGL,
display rendering/presentation, hardware video encode/decode, `torch_musa`,
and other frameworks or libraries are not tested or claimed. Moore Threads
lists OpenCL parallel compute and graphics as S30 capabilities; that
specification does not establish those features on this Fedora installation.
The vendor feature claims and project validation state are separated in
[supported features](supported-features.md).

The supported combination on this host is driver `mtgpu` 3.1.0 + MUSA runtime
1.5.4 + MUSA 5.2.0 compiler targeting `mp_10`. The separate 5.2.0 *runtime* is
not compatible with the 3.1.0 driver ABI and must not be substituted for 1.5.4.
Fedora 45 + Intel + S30 is not vendor-qualified: the public SDK catalog lists
S5000 for MUSA 5.2.0 and does not list S30 or Fedora.

## Driver build boundary

A fresh isolated build links for kernel 7.3.3 and includes an S30 PCI alias.
Historical isolated builds logged thousands of compiler/objtool warning lines
from the opaque vendor core; such artifacts are not load-approved. The live
driver on this host is the vendor-loaded 3.1.0 module. Do not disable IBT/CET
or alter vendor-core metadata.

The project-authored `mtgpu_bind.ko` in the inactive `7.2.15-300.fc45.x86_64`
tree only bound the PCI function and created a character device; its default
ioctl returned `ENOTTY`. It was not a MUSA GPU driver. Its exact module file
and global alias were hash-verified and quarantined under `musa-data`.

## Capability matrix

| Capability | Status |
|---|---|
| S30 PCI identity `1ed5:0102` | Verified |
| S30 PCI driver binding (`mtgpu` 3.1.0) | Verified; module live |
| `/dev/mtgpu.0` and S30 render node `/dev/dri/renderD129` | Verified |
| IOMMU groups | Verified; S30 in IOMMU group 11 |
| MUSA runtime 1.5.4 device discovery (`musaInfo`) | Verified; one device |
| Vendor `muMemcpyTest` | Verified |
| Host runtime API (`musart-smoke`) | Verified |
| MUSA 5.2.0 `mcc` `mp_10` vector-add | Verified; ran on the S30 |
| MUSA compute suite: SAXPY, reduction, atomics, constant, math, 2D grid, D2D, FP64 | Verified; all host-checked results passed |
| Headless Vulkan device, queue, buffer, host-visible map/readback | Verified; display output and rendering not tested |
| Moore Threads OpenCL platform | Unavailable; no system MT ICD; archived ICD compute probe caused kernel Oops |
| Fedora RPM set install + packaged `mcc` on the S30 | Verified; isolated Fedora 45 container, 13 checks passed |
| OpenGL, display rendering, video, `torch_musa`, other frameworks | Not tested or established |
| Vendor-qualified Fedora 45 + S30 stack | Not established |
| Incomplete project PCI binder | Reversibly quarantined from 7.2.15-300; its global alias is removed |

## Remaining work

Packaging is reconciled with the verified 3.1.0/1.5.4 stack: the five RPMs
build locally and pass `rpm -K --nosignature`, Copr publishes them, and the set
installs in an isolated Fedora 45 container whose packaged runtime and `mcc`
run on the physical S30 (`13 passed, 0 failed`); see
[current-state-2026-11-17.md](current-state-2026-11-17.md). Broader MUSA
features, frameworks, and a vendor Fedora/Intel qualification are still
outstanding.

## Safety

Do not unbind/rebind or replace the active module as a routine test: a
historical rebind caused a kernel fault (see
[root-cause-rebind-2026-11-16.md](root-cause-rebind-2026-11-16.md)). Do not
install a DKMS/RPM stack over the live module without a separate recovery
plan. Do not disable IBT/CET.

# mtgpu kernel driver (Moore Threads MTT S30) — 3.1.0

DKMS source for the Moore Threads `mtgpu` kernel driver, validated against the
MTT S30 (`1ed5:0102`) on Fedora Linux.

## Contents

- `Makefile`, `conftest` — kernel build system with per-kernel feature probing.
- `inc/` — driver headers, including the Fedora compatibility shims
  (`arch_compat.h`, `iommu_compat.h`, `pfn_compat.h`, `workqueue_compat.h`,
  `config_kernel.h`, `s30_runtime_compat.h`, `s30_vpu_compat.h`).
- `src/` — driver sources. `src/mtgpu/mtgpu_ob_conf_fixup.c` plus the
  `os-interface.c` hook complete the object-bookkeeping fixup required by the
  vendor core on Fedora kernels.
- `objs/x86_64/mtgpu_core.o_binary` — vendor core object (required to link the
  loadable module).

## Build & install

DKMS builds the module automatically against the running kernel:

```
dkms add    -m mtgpu -v 3.1.0
dkms build  -m mtgpu -v 3.1.0
dkms install -m mtgpu -v 3.1.0
```

On Secure Boot systems, enroll the DKMS MOK before loading:

```
mokutil --import /etc/dkms/mok.pub   # or the project-supplied MOK
```

## Result

The built `mtgpu.ko` carries the GNU property note `x86 feature: IBT` and is
safe to load on Fedora kernels with IBT/CET enabled. It binds the S30
(`pci:v00001ED5d00000102`) and exposes `/dev/mtgpu.0` and a DRM render node.

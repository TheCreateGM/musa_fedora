# Root Cause Analysis: `rm_feature_level` and `gpu_phys_heap_alloc_mem` Kernel Oops (2026-11-20)

## Incident Summary

During manual driver testing on Fedora 45 (`kernel 7.3.4-300.fc45.x86_64`) with the MTT S30 (`1ed5:0102`), loading `mtgpu.ko.xz` with options:
```sh
display=none disable_fbdev=1 rm_feature_level=1 mtgpu_drm_major=2
```
resulted in an immediate kernel crash (Oops: NULL pointer dereference) in `gpu_phys_heap_alloc_mem`:

```
Nov 20 10:21:33 fedora kernel: RIP: 0010:gpu_phys_heap_alloc_mem+0x29/0xc0 [mtgpu]
Nov 20 10:21:33 fedora kernel: Code: ... <48> 8b bf 28 01 00 00 ...
Nov 20 10:21:33 fedora kernel: RSP: 0018:ffffd26c0b6e3550 EFLAGS: 00010286
Nov 20 10:21:33 fedora kernel: RDI: 0000000000000000 CR2: 0000000000000128
Nov 20 10:21:33 fedora kernel: Call Trace:
Nov 20 10:21:33 fedora kernel:  DevmemIntCtxCreate+0x8a/0x310 [mtgpu]
Nov 20 10:21:33 fedora kernel:  BridgeDevmemIntCtxCreate+0x45/0x60 [mtgpu]
Nov 20 10:21:33 fedora kernel:  DevmemCreateContext+0x64/0x140 [mtgpu]
Nov 20 10:21:33 fedora kernel:  RGXInitCreateFWKernelMemoryContext+0x6b/0x280 [mtgpu]
Nov 20 10:21:33 fedora kernel:  RGXInit+0x91d/0xdc0 [mtgpu]
Nov 20 10:21:33 fedora kernel:  PVRSRVCommonDeviceInitialise+0xde/0x450 [mtgpu]
Nov 20 10:21:33 fedora kernel:  PVRSRVDeviceServicesOpen+0x17a/0x330 [mtgpu]
Nov 20 10:21:33 fedora kernel:  mtgpu_device_init_ioctl+0x59/0xe0 [mtgpu]
Nov 20 10:21:33 fedora kernel:  mtgpu_ioctl+0x1c4/0x210 [mtgpu]
Nov 20 10:21:33 fedora kernel: note: gnome-shell[5145] exited with irqs disabled
```

## Technical Investigation

### 1. Disassembly of `gpu_phys_heap_alloc_mem`
Disassembly of `driver-s30-3.1.0/objs/x86_64/mtgpu_core.o_binary` at offset `0x5b6d0`:
```asm
000000000005b6d0 <gpu_phys_heap_alloc_mem>:
   5b6d0:	endbr64
   5b6d4:	push   %r15
   5b6d6:	mov    %rdi,%r15
   ...
   5b6f5:	sub    $0x8,%rsp
   5b6f9:	mov    0x128(%rdi),%rdi      <-- FAULT: dereferencing %rdi + 0x128
   5b700:	call   ...
```
`%rdi` is the physical heap handle passed to `gpu_phys_heap_alloc_mem`. In this execution path, `%rdi` was `0x0` (NULL), leading to `CR2: 0x128` (NULL + 0x128).

### 2. Role of `rm_feature_level`
In the open-source wrapper headers (`driver-s30-3.1.0/inc/mtgpu/mtgpu_defs.h`):
```c
#define RM_IS_ENABLED() ((rm_feature_level) > 0)
#define RM_FEATURE_IS_ENABLED(feature) ((rm_feature_level) >= (RM_FEATURE_LEVEL_##feature))
```
`rm_feature_level` defaults to `0` in `driver-s30-3.1.0/src/mtgpu/mtgpu_module_param.c`:
```c
int rm_feature_level = 0;
module_param(rm_feature_level, int, 0644);
MODULE_PARM_DESC(rm_feature_level, "rm feature level enable(0: disable, 1: enable memory");
```

In `mtgpu_core.o_binary`, multiple memory management routines branch on `rm_feature_level`:
- `MMU_AcquireBaseAddr` (0x58810)
- `MMU_AcquireCPUBaseAddr` (0x58850)
- `DevmemIntUnmap` (0x11e50)
- `DevmemIntChangeValidity` (0x11ea0)
- `DevmemIntMap` (0x11ef0)

When `rm_feature_level > 0`, the driver diverts from the verified PVR memory allocation flow into an uncompleted Resource Manager (RM) memory management path. For MTT S30, the RM physical heap structures are not initialized, causing `DevmemIntCtxCreate` to pass a NULL heap pointer to `gpu_phys_heap_alloc_mem`.

### 3. Comparison with Historical 2026-11-17 Stack
On 2026-11-17, the host verified 13 tests with 0 failures on MTT S30. The saved configuration in `/home/axogm/Documents/project/musa-data/host-backups/pre-clean-boot-reboot-20261117-162913/installed-configs.txt` shows:
```
options mtgpu mtgpu_drm_major=2
```
`rm_feature_level` was left at default `0`. This confirmed that `rm_feature_level=0` is required for MTT S30 stability.

### 4. Resolution and Safeguards
1. **Spec & Config Fix (`3.1.0-8`)**:
   Removed `rm_feature_level=1` from the packaged
   `driver-s30-3.1.0/90-mtgpu-headless.conf`. The headless configuration now
   specifies:
   ```
   options mtgpu display=none disable_fbdev=1 mtgpu_drm_major=2
   ```
2. **Follow-up packaging (`3.1.0-9`)**:
   Release 9 removed the unsupported claim that a GNU IBT property note makes
   the opaque vendor core safe to load on IBT/CET kernels. Copr build 11365750
   succeeded for Fedora 44, 45, ELN, and Rawhide x86_64, but still reports
   extensive objtool warnings. Neither release 8 nor 9 was installed on the
   host or validated with a fresh S30 driver load.

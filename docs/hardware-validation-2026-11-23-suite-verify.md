# End-to-end suite verification on the live S30 (2026-11-23, post-08:39 UTC)

Independent re-validation of the full stack on the installed Fedora 45 host
after the earlier approved controlled load in this same boot. This document
records the exact commands and results. **No module was loaded or unloaded, no
boot or modprobe configuration was changed, and no PCI rebinding was
performed** by this run: the `mtgpu` module had been loaded at 07:39 UTC by the
approved runbook load (`docs/runbook-controlled-load-2026-11-23.md`) and the
scripts only ran userspace test binaries against the already-live driver.

## Environment

- Fedora Linux 45 (Workstation), kernel `7.3.4-300.fc45.x86_64` (`CONFIG_X86_KERNEL_IBT=y`, IBT not disabled)
- MTT S30 `0000:01:00.0` (`1ed5:0102`), bound to `mtgpu` 3.1.0 (`b0e8209-dirty` build), `/dev/mtgpu.0`, render node `renderD129`
- Installed packages: `musa-runtime 1.5.4-4`, `musa-sdk 5.2.0-6`, `musa-toolkit 3.1.0-3`, `mtgpu-dkms 3.1.0-11` (module live from the approved load), `mtgpu-firmware 3.1.0-4`
- Boot kernel journal before the run: clean (no Oops/BUG/NULL dereference; the only `mtgpu` log line is the cosmetic IRQ-affinity message)

## Commands and results (all PASS)

### 1. Reproducible test-binary build (isolated, compile+link only)

```sh
./scripts/hardware/build-test-binaries.sh
```

- `BUILD_STATUS: SUCCESS` — `musart-smoke`, `musa-vector-add`, `musa-compute`,
  `musa-fp64`, `vk-enum` built into `~/.musa-work/test-bin`.
- The new `check_runtime_link()` guard verified every binary's `DT_NEEDED`
  contains exactly `libmusart.so.1.5` (the SONAME of the installed 1.5.4
  runtime) and rejects `libmusart.so.5`: `OK (links libmusart.so.1.5)` on each
  MUSA binary. This fail-closed guard prevents the known 5.2.0-SDK-runtime ABI
  abort in `musaMalloc()` against the 3.1.0 driver. (Guard authored in the
  shared tree by peer work; this run is its positive-path validation.)

### 2. Compile-only toolchain check (no runtime, no device)

```sh
./scripts/test-toolchain.sh
```

- `COMPILE_ONLY_STATUS: PASS` — `mcc` 5.2.0, vector-add host+device compile
  OK, fat-binary sections present, explicit `mp_10` compile **PASS**, runtime
  header syntax check passed.

### 3. Gated GPU smoke test (real hardware, against the live driver)

```sh
MUSA_S30_TEST_APPROVED=1 ./scripts/test-gpu.sh
```

- All gates passed first (IBT on, clean boot journal, S30 at the expected BDF
  bound to the full `mtgpu` module). The approval flag is the documented
  gate from `docs/clean-boot-resume-plan-2026-11-17.md`.
- Runtime 1.5.4 `musaInfo` reported the MTT S30: 4096 MiB, 1300 MHz,
  warp 128, computeMode 0, free 3.69 GB (93%).
- `MUSA_VECTOR_ADD_OK elements=1048576` — vector-add, copy, allocation and
  synchronization all ran on the GPU. `GPU_TEST_STATUS: PASS`.

### 4. Full end-to-end verification suite (real hardware)

```sh
./scripts/verify-musa.sh
```

**Summary: 13 passed, 0 failed, 1 skipped.** Passed layers: PCI enumeration,
module/device nodes, `mthreads-gmi` telemetry, 1.5.4 `musaInfo`, vendor
`muMemcpyTest`, host runtime API (`musart-smoke`: alloc/memcpy/memset/streams/
events, `d2h_memset_verify=PASS`, `async_roundtrip=PASS`), `mcc` mp_10 build,
vector-add (1,048,576 float adds verified), compute suite (saxpy, shared
reduce, atomics, constant memory, device math/libm, 2D grid, D2D copy),
FP64 (`abs_err=5.551e-17`), headless Vulkan (device/queue, buffer,
host-visible map). The single SKIP is the Moore Threads OpenCL ICD, which is
absent by policy after the documented archived-ICD kernel Oops.

## Post-run system health

- Kernel journal since 08:39 UTC: no `Oops:`, `BUG:`, NULL dereference or
  general-protection fault; `mtgpu` only logs the cosmetic IRQ-affinity message.
- GNOME Shell survived (`pgrep -c gnome-shell` → 2, GDM active).
- `mthreads-gmi` 2.0.0 answered: Driver Version 3.1.0, MTT S30, 44 C,
  262 MiB / 4096 MiB in use, no processes.

## COPR state re-verified the same session

```sh
./scripts/verify-copr-chroots.sh
```

**passed: 11, failed: 0.** Both projects (`axogm/musa-fedora`,
`axogm/musa-mtt-s30`) report `succeeded` for the latest build of every package
and every configured chroot's published repository carries all six expected
artifacts: `musa-runtime 1.5.4-4`, `musa-runtime-display 1.5.4-4`,
`musa-sdk 5.2.0-6`, `musa-toolkit 3.1.0-3`, `mtgpu-dkms 3.1.0-12`,
`mtgpu-firmware 3.1.0-4`. Chroots: fedora-43/44/45/rawhide/eln x86_64 per
project (4 chroots in `musa-fedora`, 5 in `musa-mtt-s30`).

## Scope limits

This run validates discovery, memory management, kernel execution and SDK
compatibility on the live stack. It does **not** qualify display rendering,
display-port hotplug, video encode/decode, multi-GPU, OpenCL, or framework
integration (`torch_musa`, etc.), and it does not change the boot-time
blacklist: a normal reboot still returns to the safe unbound state. Permanent
enablement remains the explicit-approval `scripts/compute-mode.sh enable`
workflow in `docs/compute-mode.md`.
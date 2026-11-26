# CUDA compatibility notes (MTT S30 / MUSA Fedora)

This document describes **technically feasible** CUDA-compatibility paths for the validated MTT S30 MUSA stack. It does not claim binary CUDA compatibility or NVIDIA drop-in support.

## Validated baseline

- Host: Fedora 45, kernel 7.3.4-300.fc45.x86_64, MTT S30 `[1ed5:0102]`, `mtgpu` 3.1.0.
- Runtime: MUSA 1.5.4 (`/usr/local/musa`, `libmusart.so.1.5`).
- Compiler/SDK: MUSA 5.2.0 (`/usr/local/musa-5.2`, `mcc`, `mp_10` target for S30/SuDi).
- Verification: `verify-musa.sh --skip-memcpy` 15/0/2; container install+GPU passthrough 13/0.

## What exists

- **MUSA API**: CUDA-like runtime/driver APIs (`musa_runtime.h`, `musa.h`, device/driver types, streams, events, memory, kernels). MUSA programs compile with `mcc --offload-arch=mp_10`.
- **MUSA SDK clang/cuda wrappers**: MUSA 5.2.0 includes clang CUDA wrapper headers under `/usr/local/musa-5.2/lib/clang/14.0.0/include` (e.g. `__clang_cuda_*`), used by the MUSA toolchain; this is not a guarantee of arbitrary CUDA source compatibility.
- **Pkgconfig helpers (repo)**: `packaging/pkgconfig/musart.pc`, `packaging/pkgconfig/musa.pc` provide discovery for the validated 1.5.4 stack.

## Feasible compatibility paths

1. **Port CUDA source to MUSA**: use Moore Threads musify/porting tools where available, or manually map CUDA APIs/types to MUSA equivalents. Validate each port on S30 (`mp_10`).
2. **Build against MUSA headers/libs**: compile `.mu`/MUSA C++ with `mcc`, link `-lmusart` (and driver libs as needed) against `/usr/local/musa`.
3. **CMake/pkgconfig discovery**: use provided pkgconfig files; for CMake, add a Find/Config module that resolves MUSA_HOME and `mp_10` arch (future work).
4. **Framework ports**: use MUSA-ported frameworks from `musa-data` (e.g. `torch_musa`) only after building/testing against the validated runtime/compiler and S30 hardware.

## Not provided / not claimed

- No drop-in NVIDIA `libcuda.so`, `libcudart.so`, `nvcc`, or CUDA driver compatibility layer is installed.
- No claim that arbitrary CUDA binaries/libraries run unmodified.
- No claim of CUDA/OpenCL/ROCm framework compatibility without build+runtime evidence on S30.

## Safety

- Do not install legacy 5.2.0 `libmusa`/driver-API packages into the validated 3.1.0+1.5.4 stack.
- Verify ABI compatibility before substituting runtimes: 1.5.4 and 5.2.0 runtime ABIs differ.

MUSA Fedora Cuda Compatibility

A CUDA source-compatibility layer for the Moore Threads MUSA runtime on
Fedora, targeting the MTT S30 (mp_10).

WHAT IT IS
  A source-compatibility layer, not an emulator. CUDA source compiles with
  mcc for the real device and library calls land on real MUSA 1.5.4 runtime
  symbols. Only the subset of the CUDA API that runtime exports is mapped.

WHAT YOU GET
  /usr/local/cuda/bin/nvcc            nvcc-compatible wrapper around mcc
  /usr/local/cuda/include/cuda*.h     CUDA headers mapping onto MUSA
  /usr/local/cuda/lib64/libcudart.so  cudaX -> musaX jump shim
  /usr/local/cuda/lib64/libcuda.so    cuX/MUX -> muX jump shim
  /usr/bin/musify-text                CUDA<->MUSA source porting tool
  /usr/lib64/cmake/musa/              find_package(MUSA) config + module
  /usr/lib64/musa-cuda-compat/include/musa_rt154_names.h
                                      runtime-ABI pin for MUSA-named source

PORTING WITH musify-text
  musify-text --rename-ext --strict -o port src/kernel.cu
  musify-text -d m2c --inplace ported-file.mu
  Its name table is loaded from the generated maps, so every rewrite it
  performs is a rename the 1.5.4 runtime actually exports; anything it cannot
  map is reported, never invented. Use the --strict flag to fail when
  CUDA-only tokens survive, and heed the pin note: sources that call
  musaGetDeviceProperties (or any renamed entry point) need
  '-include musa_rt154_names.h' passed to mcc.

CMAKE
  find_package(MUSA REQUIRED)         package mode, no hints needed once the
                                      RPM is installed (/usr/lib64/cmake/musa)
  musa_add_executable(app src/kernel.mu)
  musa_add_library(app_shared src/kern.mu)
  The module searches MUSA_HOME/musa-runtime roots, pins the offload target
  to mp_10 for the S30, links ONLY the 1.5.4 runtime, and force-includes
  musa_rt154_names.h so MUSA-named callers hit the exported symbol names.

BUILD A PROGRAM
  nvcc -O2 -o app app.cu
  cmake -DCMAKE_CUDA_COMPILER=/usr/local/cuda/bin/nvcc ..
  Both work unchanged. One device is supported, so every -arch and -gencode
  spelling maps to mp_10.

WHAT IS NOT SUPPORTED
  Anything absent from the 1.5.4 runtime: memory pools, device graphs beyond
  the legacy entry points, cudaMallocAsync, 5.x-only entry points, and the
  VectorAdd-in-header helpers the runtime never shipped. Those fail at
  compile or link time. Check the audit output of gen-cuda-compat.py.

WHY THE 1.5.4 RUNTIME
  It is the runtime validated with the mtgpu 3.1.0 kernel driver on this GPU.
  The MUSA 5.x runtime issues an ioctl (MKT_NAME) that the 3.1.0 driver does
  not implement, so muInit fails with -22.

REGENERATING THE MAPS
  On a host with the vendor SDK installed:
    make -C /usr/lib64/musa-cuda-compat/.. regen   (or in this directory)
    make regen
  That reads the SDK headers, emits only mappings whose target the runtime
  exports, and rebuilds the shims. Review the diff before committing.

FILES
  include/musa_cuda_map_runtime.h   generated cudaX -> musaX map
  include/musa_cuda_map_driver.h    generated cuX/MUX -> muX map
  include/musa_cuda_undef_*.h       header renames undone before inclusion
  include/musa_rt154_names.h        renames undone AFTER inclusion (MUSA names)
  bin/musify-text                   source porting tool (stdlib Python)
  cmake/FindMUSA.cmake              module-mode discovery + mcc build helpers
  cmake/MUSAConfig*.cmake           package-mode discovery
  src/cudart_jumps.S                generated shim source
  run.sh                            hardware test suite (tests/cuda-compat)

LICENCE
  MIT for this layer. MUSA headers and libraries remain vendor proprietary.

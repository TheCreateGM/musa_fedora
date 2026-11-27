# CUDA source compatibility on the MTT S30

Status: **mapped and hardware-verified for the subset the MUSA 1.5.4 runtime
exports.** `musa-cuda-compat` 1.0.0 lets unmodified CUDA source build and run
on an MTT S30 through the MUSA compiler and runtime. Nothing is emulated.
Since release `1.0.0-2` the package also ships **`musify-text`** (CUDA↔MUSA
source porting) and **CMake discovery** (`find_package(MUSA)`,
`musa_add_executable`), both verified on hardware.

- Hardware: MTT S30 `[1ed5:0102]`, `mtgpu` 3.1.0, MUSA runtime 1.5.4.
- Verified: `tests/cuda-compat/run.sh` (**16/16** on the S30, including the
  musify and CMake gates) and
  `tests/cuda-compat/container-test.sh` (**16/16** in a disposable container
  with device passthrough, installed-RPM layout). See
  [hardware-validation-2026-11-26-cuda-compat](hardware-validation-2026-11-26-cuda-compat.md)
  and the 2026-11-27 session-4 logs.
- Published: COPR `axogm/musa-fedora`
  ([11397615](https://copr.fedorainfracloud.org/coprs/build/11397615/) for
  1.0.0-2; [11393082](https://copr.fedorainfracloud.org/coprs/build/11393082/)
  for 1.0.0-1) and `axogm/musa-mtt-s30`
  ([11397626](https://copr.fedorainfracloud.org/coprs/build/11397626/);
  [11393083](https://copr.fedorainfracloud.org/coprs/build/11393083/)), all
  five x86_64 chroots; `scripts/verify-copr-chroots.sh` asserts the 1.0.0-2
  artifact set in every chroot of both projects (24/24, 2026-11-27).

## Why a generated layer, not a hand-written one

MUSA is a rename of the CUDA API: `cudaMalloc` → `musaMalloc`,
`cuLaunchKernel` → `muLaunchKernel`, `CUdeviceptr` → `MUdeviceptr`. Two facts
make a naive text substitution wrong on this stack:

1. **The SDK headers are newer than the runtime.** The 5.2.0 headers rename
   entry points to versioned symbols (`musaGetDeviceProperties` →
   `musaGetDeviceProperties_v2`) that the 1.5.4 runtime does **not** export.
   Nine of those renames were found here; using the headers unchanged makes
   every caller fail to link. Even worse, `muGetProcAddress_v2` and the five
   `muGraph*KernelNode*_v2` names have **different prototypes**, so aliasing
   them to the legacy symbol would compile and then corrupt arguments.
2. **Many CUDA 11 entry points were never shipped in 1.5.4.** 251 runtime and
   293 driver functions the headers declare have no symbol in the runtime
   library.

So the maps are *generated* from the library's own export list
(`packaging/cuda-compat/gen-cuda-compat.py`), and a name is only mapped when
its target symbol exists. The versioned renames are undone in a header that is
included before the vendor headers, and the runtime-era prototype is restored
where the ABI differs, so a caller expecting the newer ABI gets a compile
error instead of a silent argument mismatch.

For MUSA-named source (what `musify-text` produces) the same undo runs
*after* the vendor headers via `musa_rt154_names.h`: it `#undef`s the nine
renamed entry points and re-declares the runtime-ABI prototypes inside
`extern "C"`. The CMake module force-includes it, so
`musaGetDeviceProperties` links against the plain symbol the 1.5.4 runtime
exports instead of the never-shipped `_v2`.

Regenerate on a host with the SDK installed:

```sh
sudo dnf install -y musa-cuda-compat-devel   # not required, the sources are in the repo
make -C packaging/cuda-compat regen          # needs /usr/local/musa{,-5.2}
```

## What works on this GPU

| Area | Verified |
|---|---|
| `__global__` kernels, `<<<grid, block>>>`, `mp_10` codegen | yes |
| Device/context/properties queries (`musaDeviceProp`: 8 SMs, warp 128) | yes |
| `cudaMalloc`, `cudaMemcpy*`, `cudaMemset`, `cudaFree` | yes |
| Streams, events, `cudaEventElapsedTime` | yes |
| Pinned host memory (`cudaMallocHost`) | yes |
| Unified memory (`cudaMallocManaged`) | yes |
| `__half` / `__half2` arithmetic and `half2float` | yes |
| Driver API: `cuInit`, contexts, `cuMemAlloc`, HtoD/DtoH, attributes | yes |
| `cudaMemGetInfo`, `cuMemGetInfo` | yes |
| CMake `LANGUAGES CUDA`, plain Makefiles, Meson via `CUDACXX` | yes |
| `libcudart` / `libcuda` link names for pre-linked objects | yes |
| `musify-text` c2m/m2c (strict, idempotent, byte-identical round trip) | yes |
| musified `.mu` built by `mcc`, run on S30, linked `libmusart.so.1.5` | yes |
| `find_package(MUSA)` module + config mode, `musa_add_executable`/`_library` | yes |

## What does not work

| Gap | Reason |
|---|---|
| `cudaMallocAsync`, `cudaMallocFromPool*`, `cudaMemPool*` | runtime 1.5.4 exports no symbol |
| `cudaExternal*Memory` / `*Semaphore` | not exported |
| `cudaGraph*` beyond legacy entry points, `cuGraphAddKernelNode_v2` | not exported (init/launch works) |
| `cudaDeviceGetDevResource`, `cudaDevSmResourceSplit` | 5.x only |
| Device-side `printf`/`malloc` from kernels | no device runtime in MUSA; `cudadevrt` is a link-name stub |
| MR, cooperative groups beyond header-only helpers | untested here |
| `nvrtc` runtime compilation | not shipped by the vendor |

Anything listed above fails at compile or link time. Nothing degrades
silently.

### CUDA 13 driver-discovery limitation (observed 2026-11-27)

The compatibility shims have **not** been validated with CUDA 13 applications.
On Fedora 45, Ollama's bundled CUDA 13 `libggml-cuda.so` called
`cudaGetDeviceCount`, entered the system `libcuda.so.1` shim's
`cuGetExportTable` forwarding path, and segfaulted inside vendor
`libmusa.so.1.5.4`. The kernel logged no `mtgpu` fault; Ollama subsequently
enumerated the S30 through its separate Vulkan backend, which is not evidence
of successful inference. Do not infer general pre-linked CUDA binary
compatibility from the CUDA 11 compatibility tests. Do not repeat this CUDA 13
probe or change the system shim/service without a reviewed ABI fix. Details:
[2026-11-27 live baseline](live-baseline-2026-11-27-session5.md).

### Wording worth keeping straight

* The `cudadevrt` library is **empty**. MUSA bundles device code inside `mcc`
  and has no equivalent library, so the name exists only because CMake's CUDA
  support always links it. Do not assume device-side `malloc` works because a
  binary links.
* `CUDART_VERSION`/`CUDA_VERSION` report **11000**. That is the source API
  level of these headers, not the CUDA version MUSA derives from.
* `cudaGetDeviceProperties` reports `cc 1.0`. That is the MUSA value
  (`mp_10`), *not* `sm_10`; CMake needs `-DCMAKE_CUDA_ARCHITECTURES=52` (or
  any value) because every gencode spelling collapses to `mp_10`, but
  `sm_10` itself is rejected by CMake's architecture validation.
* `libcudart.so.11.0` and `libcuda.so.1` are jump shims, not reimplementations:
  each entry point is a `jmp` to the MUSA symbol.

## Usage

```sh
nvcc -O2 -o app app.cu                       # bin/nvcc is on PATH via /usr/local/cuda
cmake -B build -DCMAKE_CUDA_COMPILER=/usr/local/cuda/bin/nvcc ..
make -C build && ./build/app
```

Include paths: `-I/usr/local/cuda/include`. Library paths: `-L/usr/local/cuda/lib64`.

Source-porting path (no compat headers in the final build):

```sh
musify-text --rename-ext --strict -o port src/kernel.cu   # kernel.cu -> kernel.mu
mcc -O2 --offload-arch=mp_10 -I/usr/local/musa-5.2/include \
    -include /usr/lib64/musa-cuda-compat/include/musa_rt154_names.h \
    port/kernel.mu -L/usr/local/musa/lib -lmusart -lmusa -o app
```

CMake discovery path (MUSA-named projects):

```cmake
find_package(MUSA REQUIRED)          # resolves from /usr/lib64/cmake/musa
musa_add_executable(app src/kernel.mu)   # mcc + mp_10 + pinned 1.5.4 link
```

## Why the 1.5.4 runtime and not 5.x

The 5.x runtime asks the kernel driver for `MTGPU_QUERY_CMD_MKT_NAME`, which
the only S30 driver (mtgpu 3.1.0) does not implement, so `muInit` fails with
`-22`. This layer is therefore generated against 1.5.4 and pinned to it with
`Requires: musa-runtime = 1.5.4`. If a future driver implements that ioctl,
regenerate against 5.x — the generator is runtime-agnostic.

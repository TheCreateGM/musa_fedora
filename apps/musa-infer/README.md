# musa-infer

A self-contained Llama-2-architecture GGUF inference engine with a CPU reference
backend and a MUSA backend, used as a real end-to-end compute workload on the
MTT S30. It is deliberately independent of `torch_musa`, `vllm-musa` and other
frameworks: it links the MUSA 1.5.4 runtime directly through the CUDA-compatible
layer in this repository and runs its own kernels on `mp_10`.

## Verified configuration (2026-11-27)

- Fedora 45 Workstation, kernel `7.3.4-300.fc45.x86_64`, x86_64
- Moore Threads MTT S30 (`1ed5:0102`), `mp_10`, 8 SMs, 4069 MiB VRAM
- `musa-runtime` 1.5.4-8, `musa-sdk` 5.2.0-7, `musa-cuda-compat` 1.0.0-1
- `mcc` 5.2.0 (`/usr/local/cuda/bin/nvcc` wrapper), system `g++` 16, OpenMP

## Model

TinyLlama-1.1B-Chat-v1.0, Q8_0, 1.1 GiB:

- URL: <https://huggingface.co/TheBloke/TinyLlama-1.1B-Chat-v1.0-GGUF/resolve/main/tinyllama-1.1b-chat-v1.0.Q8_0.gguf>
- SHA256: `a4c9bb1dbaa372f6381a035fa5c02ef087aaa1ff1f843a56a22328114f03fc59`
- License: Apache-2.0 (TinyLlama)

## Build

```sh
make -C apps/musa-infer -j3
```

The Makefile expects the runtime at `/usr/local/musa` and the CUDA-compat
toolchain at `/usr/local/cuda`; override `MUSA_RT`/`MUSA_CUDA_DIR` if needed.
The `.cu` file is compiled by `mcc` but uses only a C ABI because the MUSA SDK
clang cannot parse Fedora's libstdc++ headers; all STL host code stays in
`musa_host.cpp` and is compiled by `g++`.

## Run

```sh
M=../musa-data/models/tinyllama-1.1b-chat-q8_0/tinyllama-1.1b-chat-v1.0.Q8_0.gguf
apps/musa-infer/musa-infer --model "$M" --backend gpu   --prompt 'Hello' --tokens 16
apps/musa-infer/musa-infer --model "$M" --backend cpu   --prompt 'Hello' --tokens 16
apps/musa-infer/musa-infer --model "$M" --backend compare --prompt 'Hello' --compare-steps 8
apps/musa-infer/musa-infer --model "$M" --backend gpu --chat \
  --prompt 'What is the capital of France?' --tokens 10
```

Useful options: `--ids` (prompt token ids), `--temperature`, `--seed`,
`--threads`, `--dump-logits`, `--bench`. The program
prints machine-readable `MUSA_INFER_*` lines; the model file is memory-mapped
and never copied into RAM in full.

## Verified results

Run on 2026-11-27 with the configuration above:

| Check | Result |
|---|---|
| CPU/GPU logits (2 prompt tokens + 8 steps, 320000 values) | `max_abs=9.47e-05`, `mean_abs=5.98e-06`, `top1_same=10/10`, `MUSA_INFER_COMPARE_STATUS: PASS` |
| `Hello`, GPU | prompt ids `1 15043`, top-1 `29892` (`,`), generated `, World!` |
| Independent llama.cpp `a07831a` reference | ids `1 15043`, top-1 `29892` logit `10.461020`, `, World!` |
| Chat, GPU | `The capital of France is Paris.` |
| Throughput, 16 generated tokens | 12.58 tok/s GPU, 12.33 tok/s CPU (OpenMP, 20 threads) |

The CPU and GPU paths use different arithmetic (the GPU keeps weights quantised
and converts fp16 scales in-kernel), so `compare` checks numerical agreement
rather than bit equality. The top-1 token agrees on every step.

## Design

- `gguf.cpp`: read-only `mmap` of GGUF v3, metadata map and tensor table, typed
  accessors, Q4/Q5/Q8/K-quant block layouts.
- `tokenizer.cpp`: SentencePiece BPE with the U+2581 space marker, byte fallback
  (`<0xNN>`) and ChatML/Zephyr special tokens, adapted from `llama2.c` (MIT).
- `model.cpp`: CPU reference forward (Llama-2 architecture), OpenMP, dequantise
  in the matmul loop.
- `musa_backend.cu`: pure C ABI; MUSA kernels for RMSNorm, Q8_0 GEMV, RoPE,
  KV-cache store, causal attention, SwiGLU and residual adds.
- `musa_host.cpp`: C++ wrapper that maps GGUF tensors to a fixed slot layout,
  converts unsupported types, and calls the C ABI.
- `main.cpp`: CLI, sampling, comparison and benchmark output.

Decode is launch-bound on an 8-SM part, so the backend fuses work: Q/K/V share
one launch, the FFN gate/up/SwiGLU share one launch, and the two residual
projections accumulate in place. A token is 201 launches instead of ~330.
Q8_0 blocks are repacked once on the device into a 32-byte payload array plus a
packed fp16 scale array; the on-disk 34-byte stride puts each 2-byte scale in
its own cache sector and costs more bandwidth than the payload. fp16 scales are
converted with the hardware `cvt` instruction.

CUDA graph capture/replay was implemented and tested: capture, instantiation and
launch of a 333-node graph all report success, but the 3.1.0 driver then
segfaults in `Musa::UniversalManager::CmdSubmission` from `libmusa.so.1.5`.
The backend therefore uses plain launches; the graph path was removed rather
than shipped. See `docs/musa-infer.md` for the backtrace.

## Limitations

- Single sequence, batch 1; no batched prefill GEMM, so prompt processing runs
  at the same per-token rate as decode.
- 2048-token context, F32 KV cache, Llama-2 architecture only (TinyLlama).
- Tokenisation and sampling run on the CPU; logits are copied to the host every token.
- No `mublas`/`mudnn` on the S30; this engine exists precisely because those
  vendor libraries are not available for this GPU and runtime.

## Attribution and license

The tokenizer and the reference forward structure are adapted from
[llama2.c](https://github.com/karpathy/llama2.c) (MIT). The GGUF reader, MUSA
kernels and host glue were written for this repository. [llama.cpp](https://github.com/ggml-org/llama.cpp)
(MIT) was used only as an external correctness reference. The code in this
directory is MIT-licensed; see `LICENSE`.

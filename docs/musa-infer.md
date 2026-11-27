# `musa-infer`: end-to-end LLM inference on the MTT S30

Date: 2026-11-27. Host: Fedora 45, kernel `7.3.4-300.fc45.x86_64`, MTT S30
(`1ed5:0102`, `mp_10`, 8 SMs, 4069 MiB), `musa-runtime` 1.5.4-8, `musa-sdk`
5.2.0-7, `musa-cuda-compat` 1.0.0-1, `mcc` 5.2.0, `g++` 16.1.1.

This documents the evidence behind `apps/musa-infer/README.md`. Every number
below comes from a command run on the S30 host; no result is extrapolated.

## Why another inference engine

`torch_musa` issue #134 states that the MTT S30 is outside the supported scope,
and the 1.5.4 runtime ships no `mublas` or `mudnn` for `mp_10`. The generic
frameworks therefore cannot provide a working LLM path on this GPU. `musa-infer`
is a small, auditable workload that exercises the validated 1.5.4 runtime
directly: allocation, kernel launch, fp16 conversion, shared memory, warp
shuffles, streams and D2H copies. It is not a framework replacement.

## Model and reference implementation

- TinyLlama-1.1B-Chat-v1.0 Q8_0, 1.1 GiB, Apache-2.0.
- SHA256 `a4c9bb1dbaa372f6381a035fa5c02ef087aaa1ff1f843a56a22328114f03fc59`
  (`sha256sum` on the downloaded file).
- Independent reference: `ggml-org/llama.cpp` at `a07831a`, CPU build in
  `/tmp/llama.cpp.probe/build-cpu`, plus a small probe (`/tmp/llama_ref.cpp`)
  that prints raw token ids and the first-step logits without the interactive
  chat loop.

The reference probe printed:

```text
REF_TOKENS: 1 15043
REF_TOP1: id=29892 logit=10.461020
REF_GENERATED_IDS: 29892 2787 29991 13
REF_TEXT: , World!
```

`musa-infer --backend gpu --prompt Hello --ids` printed the same prompt ids
(`1 15043`) and the same top-1 (`id=29892 piece=,`), and greedy decoding
produced `, World!` (ids `29892 2787 29991 13`). The chat prompt is identical
to llama.cpp's: `<|system|>\nYou are a helpful assistant.</s>\n<|user|>\n…</s>\n<|assistant|>\n`,
and both produce `The capital of France is Paris.` for the France question.

## Numerical agreement

```text
$ apps/musa-infer/musa-infer --model "$M" --backend compare --prompt Hello --compare-steps 8
MUSA_INFER_COMPARE: steps=10 values=320000 max_abs=9.47118e-05 mean_abs=5.97617e-06 top1_same=10/10
MUSA_INFER_COMPARE_STATUS: PASS
```

Ten comparisons are the two prompt tokens plus the eight decode steps
(`--compare-steps`); `--tokens` does not affect the comparison loop.

The two backends do not use identical arithmetic: the CPU reference dequantises
Q8_0 blocks to float in its matmul loop, while the GPU kernel keeps the payload
as int8 and converts the fp16 scale with a hardware `cvt`. A `max_abs` around
`1e-4` on logits of magnitude ~10 is expected and the argmax agrees on all
ten comparisons.

## Throughput

`--prompt Hello --tokens 16`, same boot, back to back:

```text
GPU: prompt=0.158s (12.62 tok/s) generate=1.272s (12.58 tok/s) tokens=16
CPU: prompt=0.216s ( 9.27 tok/s) generate=1.297s (12.33 tok/s) tokens=16
```

Both paths are memory-bound on the 1.1 GiB of Q8_0 weights and land at roughly
the same rate. The GPU is not faster than a 20-thread CPU for batch-1 decode on
this 8-SM part; the useful result is that the GPU performs the full forward pass
correctly through the 1.5.4 runtime, not that it beats the CPU. A batched
prefill path (not implemented) is the obvious next step for prompt latency.

## Launch fusion and layout

Recording a profile is not available on this driver, so the design was driven
by measured deltas:

| Change | GPU generate |
|---|---|
| Initial kernel set (~330 launches/token) | 7.45 tok/s |
| Fuse Q/K/V, FFN gate/up/SwiGLU, accumulate residuals (201 launches) | 8.71 tok/s |
| Hardware fp16→fp32 scale conversion | 9.85 tok/s |
| Split Q8 payload and scale arrays (32-byte aligned payloads) | 12.54 tok/s |

The split layout is the important one. In the GGUF layout each 34-byte block
is `fp16 scale + 32 int8`; a warp reading 32 payload bytes therefore touches two
sectors, and the scale load touches a third for two useful bytes. Repacking on
the device into `payload[blocks][32]` and `scale[blocks]` makes payload reads
one aligned sector and amortises 16 scales per sector. The repack runs once at
model load (device memory, ~1.1 GiB, freed after upload).

Streaming read bandwidth measured with a separate float4 kernel on the S30 is
57.6 GB/s (23.07 GB in 400.6 ms). The GEMV path reaches ~14 GB/s effective on
1.1 GiB per token, so there is still headroom; further work would quantise the
activation to int8 and use `dp4a`, as llama.cpp does, instead of converting
weights to fp32 for every multiply.

## CUDA graph experiment (negative result)

MUSA 1.5.4 exports the graph API (`musaStreamBeginCapture`, `musaGraphInstantiate`,
`musaGraphLaunch`) and a minimal two-kernel graph works. Capturing the real
decode sequence also succeeds: 333 nodes, instantiate `cudaSuccess`, launch
`cudaSuccess`. The first synchronisation after launch then segfaults:

```text
Thread 4 received signal SIGSEGV
#0 Musa::UniversalManager::CmdSubmission(Musa::MusaSubmission const*, Musa::GraphCommand*, ...)
#1 Musa::GraphCommand::Execute()
#2 Musa::GraphCommand::Submit()
#3 Musa::Stream::AsyncSubmit()
   from /usr/local/musa/lib/libmusa.so.1.5
```

The same crash occurs with `cudaStreamCaptureModeThreadLocal` and `Global`, with
pageable and pinned scalar arguments, and with and without a synchronisation
between the argument copies and `cudaGraphLaunch`. A control graph of 333
trivial kernels replays correctly, so it is not a node-count limit; it is the
driver's handling of this kernel mix. The graph code was removed rather than
shipped as a crashing option.

## Reproduce

```sh
make -C apps/musa-infer -j3
M=../musa-data/models/tinyllama-1.1b-chat-q8_0/tinyllama-1.1b-chat-v1.0.Q8_0.gguf
apps/musa-infer/musa-infer --model "$M" --backend compare --prompt Hello --compare-steps 8
apps/musa-infer/musa-infer --model "$M" --backend gpu --chat \
  --prompt 'What is the capital of France?' --tokens 10
tests/musa-infer/run.sh
```

`tests/musa-infer/run.sh` sets `MODEL` from the environment or uses the path
above, checks the compare status, the prompt ids, the greedy continuation and
the chat answer, and exits non-zero on any mismatch.

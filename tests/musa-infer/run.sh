#!/usr/bin/env bash
# run.sh — verify apps/musa-infer on the live MTT S30.
#
# Checks the GPU/CPU comparison, the tokenizer contract, greedy decoding and
# the chat template against fixed expected values recorded in
# docs/musa-infer.md. Requires the TinyLlama Q8_0 GGUF.
#
# Usage: run.sh [--model FILE]
#   --model FILE   GGUF path (default $MODEL, then ../musa-data/...)
# Exit: 0 pass, 1 mismatch, 2 environment problem.
#
# SPDX-License-Identifier: MIT
set -uo pipefail

HERE="$(cd -- "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT="$(cd -- "$HERE/../.." && pwd)"
BIN="${BIN:-$PROJECT/apps/musa-infer/musa-infer}"
MODEL="${MODEL:-}"

while (($#)); do
    case "$1" in
        --model) MODEL="${2:?--model needs a file}"; shift 2;;
        -h|--help) sed -n '2,12p' "$0"; exit 0;;
        *) echo "usage: $0 [--model FILE]" >&2; exit 2;;
    esac
done

if [[ -z "$MODEL" ]]; then
    MODEL="$PROJECT/../musa-data/models/tinyllama-1.1b-chat-q8_0/tinyllama-1.1b-chat-v1.0.Q8_0.gguf"
fi
[[ -x "$BIN" ]] || { echo "MUSA_INFER_TEST: FAIL (build apps/musa-infer first)"; exit 2; }
[[ -f "$MODEL" ]] || { echo "MUSA_INFER_TEST: FAIL (model not found: $MODEL)"; exit 2; }

WORK="$(mktemp -d "${TMPDIR:-/tmp}/musa-infer-test.XXXXXX")"
trap 'rm -rf "$WORK"' EXIT
fail=0

check() {
    local name="$1" expected="$2" file="$3"
    if grep -Fq -- "$expected" "$file"; then
        echo "PASS $name"
    else
        echo "FAIL $name (missing: $expected)"
        sed 's/^/  /' "$file"
        fail=1
    fi
}

echo "== compare CPU vs GPU =="
"$BIN" --model "$MODEL" --backend compare --prompt Hello --tokens 4 --compare-steps 8 \
    >"$WORK/compare.log" 2>&1
check "compare status" "MUSA_INFER_COMPARE_STATUS: PASS" "$WORK/compare.log"
check "compare top-1" "top1_same=10/10" "$WORK/compare.log"

echo "== GPU tokenizer and greedy decode =="
"$BIN" --model "$MODEL" --backend gpu --prompt Hello --tokens 8 --ids \
    >"$WORK/gpu.log" 2>&1
check "prompt ids" "MUSA_INFER_PROMPT_IDS: 1 15043" "$WORK/gpu.log"
check "top-1 token" "MUSA_INFER_TOP1: id=29892" "$WORK/gpu.log"
check "greedy text" "MUSA_INFER_GENERATED: , World!" "$WORK/gpu.log"

echo "== GPU chat template =="
"$BIN" --model "$MODEL" --backend gpu --chat \
    --prompt 'What is the capital of France?' --tokens 10 >"$WORK/chat.log" 2>&1
check "chat answer" "MUSA_INFER_GENERATED: The capital of France is Paris." "$WORK/chat.log"

if ((fail)); then
    echo "MUSA_INFER_TEST: FAIL"
    exit 1
fi
echo "MUSA_INFER_TEST: PASS"

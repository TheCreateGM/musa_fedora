#!/usr/bin/env bash
# run.sh — exercise the musa-cuda-compat layer against the live GPU.
#
# Every program under this directory is plain CUDA source: it uses `cuda*`
# names, `cuda_runtime.h`, `cuda.h` and `<<<...>>>`, and is built only with the
# compat nvcc wrapper. A test passes when the kernel(s) actually ran and the
# host verified the results on the MTT S30.
#
# Usage: run.sh [--root DIR] [--keep]
#   --root DIR   use an already-built compat root instead of building one
#   --keep       keep the build directory
# Exit: 0 all tests passed, 1 a test failed, 2 environment problem.
#
# SPDX-License-Identifier: MIT
set -uo pipefail

HERE="$(cd -- "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT="$(cd -- "$HERE/../.." && pwd)"
WORK="$(mktemp -d "${TMPDIR:-/tmp}/musa-cuda-compat.XXXXXX")"
ROOT=""
KEEP=0
while (($#)); do
    case "$1" in
        --root) ROOT="${2:?--root needs a directory}"; shift 2;;
        --keep) KEEP=1; shift;;
        *) echo "usage: $0 [--root DIR] [--keep]" >&2; exit 2;;
    esac
done
cleanup() { ((KEEP)) && echo "work dir kept: $WORK" || rm -rf "$WORK"; }
trap cleanup EXIT

if [[ -z "$ROOT" ]]; then
    ROOT="$WORK/root"
    "$PROJECT/packaging/cuda-compat/build-root.sh" "$ROOT" >"$WORK/build-root.log" 2>&1 || {
        echo "BUILD_ROOT: FAIL"; sed 's/^/  /' "$WORK/build-root.log"; exit 2; }
fi
[[ -x "$ROOT/bin/nvcc" ]] || { echo "no nvcc in $ROOT" >&2; exit 2; }

MUSA_RT="${MUSA_RT:-/usr/local/musa}"
fail=0
pass=0
skip=0

note() { printf '%s\n' "$*"; }
ok()   { pass=$((pass + 1)); printf '  PASS  %s\n' "$*"; }
bad()  { fail=$((fail + 1)); printf '  FAIL  %s\n' "$*"; }
skipped() { skip=$((skip + 1)); printf '  SKIP  %s\n' "$*"; }

# Refuse to run without a bound device: a "pass" on no GPU would be a lie.
if ! command -v mthreads-gmi >/dev/null; then
    skipped "mthreads-gmi not installed; cannot confirm a device"
elif ! mthreads-gmi 2>/dev/null | grep -q 'MTT S'; then
    echo "DEVICE: FAIL (no MTT device reported by mthreads-gmi)" >&2
    exit 2
else
    note "device: $(mthreads-gmi 2>/dev/null | awk '/MTT S/{print $2" "$3; exit}')"
fi

note "compat root: $ROOT"
note "runtime: $MUSA_RT"
echo

run_case() {
    local src="$1" label="$2" expect="$3"
    local bin="$WORK/${src%.cu}"
    if ! "$ROOT/bin/nvcc" -O2 -o "$bin" "$HERE/$src" >"$WORK/$label.build" 2>&1; then
        bad "$label: build failed"
        sed 's/^/        /' "$WORK/$label.build" | head -15
        return
    fi
    local out rc
    out="$(LD_LIBRARY_PATH="$ROOT/lib64:$MUSA_RT/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" \
           timeout 180 "$bin" 2>&1)"
    rc=$?
    printf '%s\n' "$out" | sed 's/^/        /'
    if ((rc != 0)); then
        bad "$label: exited $rc"
    elif [[ -n "$expect" && "$out" != *"$expect"* ]]; then
        bad "$label: missing '$expect'"
    else
        ok "$label"
    fi
}

note "== CUDA runtime source compatibility =="
run_case cuda-vector-add.cu vector-add "VADD_STATUS: PASS"
run_case cuda-streams.cu streams "STREAM_STATUS: PASS"
run_case cuda-fp16.cu fp16 "FP16_STATUS: PASS"
run_case cuda-driver.cu driver-api "DRV_STATUS: PASS"

note "== Header self-consistency =="
# The 1.5.4 runtime headers reference musa_surface_types.h, which only the
# 5.2.0 header set ships. Compiling against 1.5.4 headers alone must work.
cat >"$WORK/header-selfcontained.cu" <<'EOF'
#include <cuda_runtime.h>
#include <cuda.h>
#include <cuda_fp16.h>
int main() { return 0; }
EOF
if "$ROOT/bin/nvcc" -O2 -c -o "$WORK/hdr.o" "$WORK/header-selfcontained.cu" \
        >"$WORK/hdr.log" 2>&1; then
    ok "cuda_runtime.h + cuda.h compile together"
else
    bad "compat headers do not compile"
    sed 's/^/        /' "$WORK/hdr.log" | head -10
fi

note "== ABI: every exported CUDA name has a MUSA target =="
if [[ -f "$PROJECT/packaging/cuda-compat/bin/nvcc" ]]; then
    missing=0
    for pair in "libcudart.so.11.0:musa" "libcuda.so.1:mu"; do
        shim="${pair%%:*}"; pfx="${pair##*:}"
        lib="$ROOT/lib64/$shim"
        [[ -f "$lib" ]] || { bad "missing shim $shim"; continue; }
        while read -r sym; do
            [[ -n "$sym" ]] || continue
            readelf -r "$lib" 2>/dev/null | grep -q "$sym" && continue
            grep -q "U $sym\$" <(nm -D "$lib") || missing=$((missing + 1))
        done < <(nm -D --defined-only "$lib" | awk -v p="^$pfx" '$3 ~ p {print $3}')
    done
    ((missing == 0)) && ok "no shim entry point lacks a resolution target" \
                      || bad "$missing shim entry points unresolved"
fi

note "== CUDA name coverage report =="
if python3 "$PROJECT/packaging/cuda-compat/gen-cuda-compat.py" --print-audit \
        >"$WORK/audit.txt" 2>&1; then
    sed -n '1,3p;$p' "$WORK/audit.txt" | sed 's/^/        /'
    ok "audit ran"
else
    bad "audit script failed"
fi

echo
printf 'Summary: %d passed, %d failed, %d skipped\n' "$pass" "$fail" "$skip"
if ((fail)); then
    echo "CUDA_COMPAT_STATUS: FAIL"
    exit 1
fi
echo "CUDA_COMPAT_STATUS: PASS"
exit 0

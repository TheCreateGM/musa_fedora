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

note "== musify-text: CUDA -> MUSA porting, built and run on the GPU =="
MUSIFY="$ROOT/bin/musify-text"
MUSA_SDK="${MUSA_SDK:-/usr/local/musa-5.2}"
if [[ -x "$MUSIFY" ]]; then
    PORT="$WORK/port"; mkdir -p "$PORT"
    if "$MUSIFY" --map-dir "$ROOT/include" --rename-ext --strict \
            -o "$PORT" "$HERE/cuda-streams.cu" >"$WORK/musify.log" 2>&1; then
        ok "musify-text: converted with zero CUDA-token residue (--strict)"
    else
        bad "musify-text conversion/strict check failed"
        sed 's/^/        /' "$WORK/musify.log" | head -6
    fi
    if "$MUSIFY" -d m2c --map-dir "$ROOT/include" -o "$WORK/back" \
            "$PORT/cuda-streams.mu" >/dev/null 2>&1 \
            && cmp -s "$HERE/cuda-streams.cu" "$WORK/back/cuda-streams.mu"; then
        ok "musify-text: m2c round-trip is byte-identical"
    else
        bad "musify-text: m2c round-trip differs"
    fi
    if [[ -x "$MUSA_SDK/bin/mcc" ]]; then
        for tcase in "cuda-streams.cu:STREAM:STREAM_STATUS: PASS" \
                     "cuda-vector-add.cu:VADD:VADD_STATUS: PASS"; do
            src="${tcase%%:*}"; rest="${tcase#*:}"
            stem="${rest%%:*}"; expect="${rest#*:}"
            mu="$PORT/${src%.cu}.mu"
            if ! "$MUSIFY" --map-dir "$ROOT/include" --rename-ext --strict \
                    -o "$PORT" "$HERE/$src" >/dev/null 2>&1; then
                bad "musify-text --strict failed on $src"
                continue
            fi
            if "$MUSA_SDK/bin/mcc" -O2 -x musa --offload-arch=mp_10 \
                    -I "$MUSA_SDK/include" \
                    -include "$ROOT/include/musa_rt154_names.h" \
                    "$mu" -L "$MUSA_RT/lib" \
                    -Wl,-rpath,"$MUSA_RT/lib" -lmusart -lmusa \
                    -o "$WORK/musified-$stem" >"$WORK/musified-$stem.build" 2>&1; then
                if readelf -d "$WORK/musified-$stem" 2>/dev/null | grep -q 'libmusart\.so\.1\.5]'; then
                    ok "musified $src links the 1.5.4 runtime (not the SDK's 5.x)"
                else
                    bad "musified $src does not NEEDED libmusart.so.1.5"
                fi
                out="$(LD_LIBRARY_PATH="$MUSA_RT/lib" timeout 120 "$WORK/musified-$stem" 2>&1)"
                if [[ $? -eq 0 && "$out" == *"$expect"* ]]; then
                    ok "musified $src ran on the S30"
                else
                    bad "musified $src run failed: ${out: -120}"
                fi
            else
                bad "mcc failed on musified $src"
                sed 's/^/        /' "$WORK/musified-$stem.build" | head -8
            fi
        done
    else
        skipped "mcc not found at $MUSA_SDK/bin/mcc"
    fi
else
    skipped "no musify-text in compat root"
fi

note "== CMake: find_package(MUSA) + musa_add_executable on the GPU =="
if command -v cmake >/dev/null 2>&1 && [[ -f "$ROOT/cmake/FindMUSA.cmake" ]]; then
    CT="$WORK/cmtest"; mkdir -p "$CT"
    cp "$PORT/cuda-vector-add.mu" "$CT/kernel.mu" 2>/dev/null \
        || cp "$HERE/cuda-streams.cu" "$CT/kernel.mu"
    cat >"$CT/CMakeLists.txt" <<EOF
cmake_minimum_required(VERSION 3.16)
project(musa_cmake_gate CXX)
list(APPEND CMAKE_MODULE_PATH "$ROOT/cmake")
find_package(MUSA REQUIRED)
musa_add_executable(cmstream kernel.mu)
EOF
    if cmake -S "$CT" -B "$CT/build" -D MUSA_CUDA_COMPAT_ROOT="$ROOT" \
            >"$WORK/cmake.log" 2>&1 \
            && cmake --build "$CT/build" >>"$WORK/cmake.log" 2>&1; then
        if grep -q 'Found MUSA' "$WORK/cmake.log"; then
            ok "find_package(MUSA) located mcc + 1.5.4 runtime roots"
        else
            bad "find_package ran but never reported Found MUSA"
        fi
        if readelf -d "$CT/build/musa/cmstream" 2>/dev/null | grep -q 'libmusart\.so\.1\.5]'; then
            ok "musa_add_executable linked libmusart.so.1.5"
        else
            bad "cmstream links the wrong runtime"
        fi
        out="$(LD_LIBRARY_PATH="$MUSA_RT/lib" timeout 120 "$CT/build/musa/cmstream" 2>&1)"
        if [[ $? -eq 0 && "$out" == *VADD_STATUS:\ PASS* ]]; then
            ok "musa_add_executable binary ran on the S30"
        else
            bad "cmstream run failed: ${out: -120}"
        fi
    else
        bad "CMake gate failed to configure/build"
        tail -12 "$WORK/cmake.log" | sed 's/^/        /'
    fi
else
    skipped "cmake or the staged FindMUSA.cmake unavailable"
fi

echo
printf 'Summary: %d passed, %d failed, %d skipped\n' "$pass" "$fail" "$skip"
if ((fail)); then
    echo "CUDA_COMPAT_STATUS: FAIL"
    exit 1
fi
echo "CUDA_COMPAT_STATUS: PASS"
exit 0

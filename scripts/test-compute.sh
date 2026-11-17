#!/usr/bin/env bash
# Build and run the S30 compute checks against runtime 1.5.4 and mcc 5.2.0.
# Does not load, unload, or replace kernel modules.
set -uo pipefail

ROOT="$(cd -- "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
MUSA_RT_LIB="${MUSA_RT_LIB:-/usr/local/musa/lib}"
MUSA_SDK="${MUSA_SDK:-/usr/local/musa-5.2}"
MCC="$MUSA_SDK/bin/mcc"
WORK="$(mktemp -d "${TMPDIR:-/tmp}/musa-compute.XXXXXX")"
trap 'rm -rf "$WORK"' EXIT

if [[ ! -x "$MCC" ]]; then
    printf 'mcc not found: %s\n' "$MCC" >&2
    exit 2
fi

fail=0

build_run() {
    local src="$1" bin="$2" label="$3"
    if ! "$MCC" -O2 --offload-arch=mp_10 \
        -I"$MUSA_SDK/include" -L"$MUSA_RT_LIB" -lmusart \
        -Wl,-rpath,"$MUSA_RT_LIB" \
        -o "$WORK/$bin" "$src" >"$WORK/$bin.build" 2>&1; then
        printf 'BUILD_FAIL %s\n' "$label"
        sed 's/^/  /' "$WORK/$bin.build" | head -20
        fail=1
        return
    fi
    if ! LD_LIBRARY_PATH="$MUSA_RT_LIB${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" \
        timeout 120 "$WORK/$bin"; then
        printf 'RUN_FAIL %s\n' "$label"
        fail=1
    fi
}

build_run "$ROOT/tests/musa-compute.mu" compute compute
build_run "$ROOT/tests/musa-fp64.mu" fp64 fp64

if ((fail)); then
    printf 'TEST_COMPUTE_STATUS: FAIL\n'
    exit 1
fi
printf 'TEST_COMPUTE_STATUS: SUCCESS\n'
exit 0

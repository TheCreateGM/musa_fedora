#!/usr/bin/env bash
# build-test-binaries.sh — compile the GPU test binaries used by
# scripts/hardware/load-and-test.sh into a target directory.
#
# The load test deliberately does not build anything itself: it loads the
# driver first and then runs a fixed set of already-built binaries, so that a
# compiler problem can never be confused with a driver problem.
#
# Usage: scripts/hardware/build-test-binaries.sh [TARGET_DIR]
#
# Produces (in TARGET_DIR):
#   musart-smoke     host MUSA runtime API probe      (gcc, 1.5.4 runtime ABI)
#   musa-vector-add  mp_10 vector-add kernel          (mcc --offload-arch=mp_10)
#   musa-compute     mp_10 compute suite               (mcc --offload-arch=mp_10)
#   musa-fp64        mp_10 FP64 kernel                (mcc --offload-arch=mp_10)
#   vk-enum          headless Vulkan device/buffer    (gcc)
set -uo pipefail

ROOT="$(cd -- "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
TARGET="${1:-${TARGET_DIR:-$HOME/.musa-work/test-bin}}"
MUSA_RT_LIB="${MUSA_RT_LIB:-/usr/local/musa/lib}"
MUSA_RT_INC="${MUSA_RT_INC:-/usr/local/musa/include}"
MUSA_SDK="${MUSA_SDK:-/usr/local/musa-5.2}"
MCC="${MCC:-$MUSA_SDK/bin/mcc}"

mkdir -p "$TARGET"
rc=0

# The S30 runs the MUSA 1.5.4 runtime (libmusart.so.1.5). The 5.2.0 SDK
# carries a different runtime (libmusart.so.5) whose ABI does not match the
# 3.1.0 mtgpu driver: a binary linked against it aborts inside musaMalloc()
# and makes the driver log "Invalid cmd_type".  Link against the runtime that
# is actually installed and verify the result, so a wrong soname is a build
# error instead of a runtime abort.
EXPECTED_MUSART_SONAME=""
if [[ -f "$MUSA_RT_LIB/libmusart.so" ]]; then
    EXPECTED_MUSART_SONAME="$(objdump -p "$MUSA_RT_LIB/libmusart.so" 2>/dev/null \
        | awk '/SONAME/ {print $2; exit}')"
fi

if [[ -z "$EXPECTED_MUSART_SONAME" ]]; then
    echo "FATAL: cannot read the SONAME of the installed MUSA runtime ($MUSA_RT_LIB/libmusart.so)"
    echo "       the runtime library is missing or unreadable, so the link target cannot be verified."
    echo "       Refusing to build test binaries whose runtime linkage cannot be checked."
    exit 1
fi

check_runtime_link() {
    local name="$1" bin="$TARGET/$1" needed
    if ! needed="$(readelf -d "$bin" 2>/dev/null | awk '/NEEDED/ {print $NF}' | tr -d '[]')"; then
        echo "  FAILED: cannot read the dynamic section of $name"; rc=1; return 0
    fi
    if ! grep -qx "$EXPECTED_MUSART_SONAME" <<<"$needed"; then
        echo "  FAILED: $name does not link the installed MUSA runtime"
        echo "         expected DT_NEEDED $EXPECTED_MUSART_SONAME from $MUSA_RT_LIB"
        echo "         got: $(tr '\n' ' ' <<<"$needed")"
        echo "         a binary linked against the 5.2.0 SDK runtime (libmusart.so.5)"
        echo "         aborts inside musaMalloc() against the 3.1.0 mtgpu driver"
        rc=1
        return 0
    fi
    if grep -q "libmusart.so.5" <<<"$needed"; then
        echo "  FAILED: $name also links the incompatible 5.2.0 runtime"; rc=1; return 0
    fi
    echo "  OK (links $EXPECTED_MUSART_SONAME)"
}

build_host() {
    local name="$1" src="$ROOT/tests/$2"; shift 2
    echo "building $name from $src"
    if gcc -O2 -Wall "$src" -I"$MUSA_RT_INC" -L"$MUSA_RT_LIB" "$@" \
        -Wl,-rpath,"$MUSA_RT_LIB" -o "$TARGET/$name"; then
        echo "  OK"
        check_runtime_link "$name"
    else
        echo "  FAILED"; rc=1
    fi
}

build_mcc() {
    local name="$1" src="$ROOT/tests/$2"
    echo "building $name from $src (mp_10)"
    if "$MCC" -O2 --offload-arch=mp_10 -I"$MUSA_SDK/include" \
        -L"$MUSA_RT_LIB" -lmusart -Wl,-rpath,"$MUSA_RT_LIB" \
        -o "$TARGET/$name" "$src"; then
        echo "  OK"
        check_runtime_link "$name"
    else
        echo "  FAILED"; rc=1
    fi
}

# Host runtime API probe.
build_host musart-smoke musart-smoke.c -lmusart

# mp_10 kernels compiled by the MUSA 5.2.0 mcc compiler.
build_mcc musa-vector-add musa-vector-add.mu
build_mcc musa-compute musa-compute.mu
build_mcc musa-fp64 musa-fp64.mu

# Headless Vulkan device/buffer/map probe.
echo "building vk-enum from tests/vulkan-enumerate.c"
if gcc -O2 -Wall "$ROOT/tests/vulkan-enumerate.c" -ldl -o "$TARGET/vk-enum"; then
    echo "  OK"
else
    echo "  FAILED"; rc=1
fi

echo
ls -l "$TARGET"
[[ $rc -eq 0 ]] || { echo "BUILD_STATUS: FAILED"; exit 1; }
echo "BUILD_STATUS: SUCCESS"

#!/usr/bin/env bash
# Compile-only smoke test for the MUSA compiler and headers.
# This script never initializes the MUSA runtime or opens a GPU device.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
PROJECT_ROOT="$(dirname "$PROJECT_DIR")"
OFFICIAL_RESEARCH="$PROJECT_ROOT/musa-data/research/official-musa-5.2.0"
OFFICIAL_MTCC="$OFFICIAL_RESEARCH/packages/extracted/mtcc-5-2_5.2.0_amd64/usr/local/musa-5.2"
OFFICIAL_MUSART_DEV="$OFFICIAL_RESEARCH/packages/extracted/musa-musart-dev-5-2_5.2.0_amd64/usr/local/musa-5.2"
OFFICIAL_RUNTIME_LIB="$OFFICIAL_RESEARCH/link-libs"

if [[ -z "${MUSA_PATH+x}" && -x "$OFFICIAL_MTCC/bin/mcc" &&
      -d "$OFFICIAL_MUSART_DEV/include" && -f "$OFFICIAL_RUNTIME_LIB/libmusart.so" ]]; then
    MUSA_USE_STAGING=1
    source "$PROJECT_DIR/scripts/musa-env.sh"
    TOOLCHAIN_SOURCE="staged official MUSA 5.2.0 packages"
else
    MUSA_PATH="${MUSA_PATH:-/usr/local/musa-5.2}"
    MUSA_INCLUDE_PATH="${MUSA_INCLUDE_PATH:-$MUSA_PATH/include}"
    MUSA_COMPILER_INCLUDE="${MUSA_COMPILER_INCLUDE:-$MUSA_PATH/include}"
    MUSA_RT_LIB_PATH="${MUSA_RT_LIB_PATH:-$MUSA_PATH/lib}"
    TOOLCHAIN_SOURCE="MUSA_PATH override or host-local SDK tree"
fi

MCC="${MUSA_MCC:-$MUSA_PATH/bin/mcc}"
MCC_ARGS=(--musa-path="$MUSA_PATH"
    --musa-inc-path="$MUSA_INCLUDE_PATH"
    --musa-rt-lib-path="$MUSA_RT_LIB_PATH"
    -I"$MUSA_COMPILER_INCLUDE")
TEST_SOURCE="$PROJECT_DIR/tests/musa_vector_add.mu"

if [[ ! -x "$MCC" ]]; then
    echo "ERROR: MUSA compiler not found: $MCC" >&2
    exit 1
fi
if [[ ! -f "$TEST_SOURCE" ]]; then
    echo "ERROR: MUSA compile probe not found: $TEST_SOURCE" >&2
    exit 1
fi
if [[ ! -d "$MUSA_INCLUDE_PATH" || ! -d "$MUSA_COMPILER_INCLUDE" ||
      ! -f "$MUSA_RT_LIB_PATH/libmusart.so" ]]; then
    echo "ERROR: MUSA headers or runtime link library are missing" >&2
    exit 1
fi

TMP_DIR="$(mktemp -d "${TMPDIR:-/tmp}/musa-toolchain-check.XXXXXX")"
trap 'rm -rf -- "$TMP_DIR"' EXIT

echo "=== MUSA compile-only validation ==="
echo "Toolchain source: $TOOLCHAIN_SOURCE"
echo "Compiler path: $MCC"
"$MCC" --version

echo
echo "Compiling vector-add host executable and embedded device code..."
"$MCC" "${MCC_ARGS[@]}" "$TEST_SOURCE" -o "$TMP_DIR/vector_add"
file "$TMP_DIR/vector_add"

if ! FATBIN_SECTIONS="$(readelf -SW "$TMP_DIR/vector_add" |
    grep -E '\.mt_fatbin|\.mtFatBinSegment')"; then
    echo "ERROR: compiler output has no MUSA fat-binary section" >&2
    exit 1
fi
echo "MUSA fat-binary section: present"
printf '%s\n' "$FATBIN_SECTIONS"

TARGETS="$(strings "$TMP_DIR/vector_add" | grep -Eo 'mp_[0-9]+' | sort -u || true)"
if [[ -z "$TARGETS" ]]; then
    echo "ERROR: no embedded MUSA architecture tags found" >&2
    exit 1
fi
echo "Embedded architecture tags (S30 runtime compatibility unverified):"
while IFS= read -r target; do
    printf '  %s\n' "$target"
done <<<"$TARGETS"

echo
echo "Checking the MP_10 device target explicitly..."
"$MCC" "${MCC_ARGS[@]}" --offload-arch=mp_10 \
    "$TEST_SOURCE" -o "$TMP_DIR/vector_add_mp10"
MP10_TARGETS="$(strings "$TMP_DIR/vector_add_mp10" | grep -Eo 'mp_[0-9]+' | sort -u || true)"
if [[ "$MP10_TARGETS" != "mp_10" ]]; then
    echo "ERROR: explicit mp_10 compile did not produce only the requested target" >&2
    printf 'Observed targets: %s\n' "${MP10_TARGETS:-none}" >&2
    exit 1
fi
echo "Explicit mp_10 compile: PASS (S30 runtime compatibility remains unverified)"

echo
echo "Checking runtime headers without linking or running an application..."
cat <<'EOF' | "${CC:-cc}" -x c -std=c11 -fsyntax-only \
    -I"$MUSA_INCLUDE_PATH" -I"$MUSA_COMPILER_INCLUDE" -
#include <musa_runtime.h>
#include <musa_runtime_api.h>
int main(void) {
    musaError_t status = musaSuccess;
    return (int)status;
}
EOF
echo "MUSA runtime headers: syntax check passed"

echo
echo "COMPILE_ONLY_STATUS: PASS"
echo "GPU_RUNTIME_STATUS: NOT TESTED"
echo "The generated executable was not run; no MUSA runtime query was made."

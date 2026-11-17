#!/usr/bin/env bash
# Gated MTT S30 runtime smoke test. Never loads or unloads kernel modules.
set -euo pipefail

BDF="0000:01:00.0"
DEVICE="/sys/bus/pci/devices/$BDF"
MUSA_PATH="${MUSA_PATH:-/usr/local/musa-5.2}"
MCC="$MUSA_PATH/bin/mcc"
MUSA_INFO="$MUSA_PATH/bin/musaInfo"
TEST_SOURCE="$(cd "$(dirname "${BASH_SOURCE[0]}")/../tests" && pwd)/musa_vector_add.mu"
BLOCKERS=()
RENDER_NODE=""

echo "=== MTT S30 runtime smoke test ==="
echo "No module load/unload or system configuration changes are performed."

if [[ -r /proc/cmdline ]] && grep -Eq '(^|[[:space:]])(ibt=off|noibt)([[:space:]]|$)' /proc/cmdline; then
    BLOCKERS+=("kernel command line disables IBT/CET")
fi
CONFIG_FILE="/boot/config-$(uname -r)"
if [[ ! -r "$CONFIG_FILE" ]] || ! grep -q '^CONFIG_X86_KERNEL_IBT=y$' "$CONFIG_FILE"; then
    BLOCKERS+=("active kernel configuration does not establish CONFIG_X86_KERNEL_IBT=y")
fi

if [[ ! -r "$DEVICE/vendor" || ! -r "$DEVICE/device" ]] ||
   [[ "$(cat "$DEVICE/vendor" 2>/dev/null || true)" != "0x1ed5" ]] ||
   [[ "$(cat "$DEVICE/device" 2>/dev/null || true)" != "0x0102" ]]; then
    BLOCKERS+=("expected MTT S30 PCI function $BDF (1ed5:0102) is not present")
fi

DRIVER_PATH=""
MODULE_PATH=""
if [[ -L "$DEVICE/driver" ]]; then
    DRIVER_PATH="$(readlink -f "$DEVICE/driver")"
    MODULE_PATH="$(readlink -f "$DRIVER_PATH/module" 2>/dev/null || true)"
fi
if [[ "${DRIVER_PATH##*/}" != "mtgpu" || "${MODULE_PATH##*/}" != "mtgpu" ]]; then
    BLOCKERS+=("S30 is not bound to the full mtgpu module (a binder module is insufficient)")
fi
if [[ ! -r /sys/module/mtgpu/initstate ]] ||
   [[ "$(cat /sys/module/mtgpu/initstate 2>/dev/null || true)" != "live" ]]; then
    BLOCKERS+=("full mtgpu module is not fully initialized")
fi

for sysdev in /sys/class/drm/renderD*/device; do
    [[ -e "$sysdev" ]] || continue
    resolved="$(readlink -f "$sysdev" 2>/dev/null || true)"
    if [[ "$resolved" == *"/$BDF" ]]; then
        render_name="$(basename "${sysdev%/device}")"
        candidate="/dev/dri/$render_name"
        if [[ -c "$candidate" ]]; then
            RENDER_NODE="$candidate"
            break
        fi
    fi
done
if [[ -z "$RENDER_NODE" ]]; then
    BLOCKERS+=("no DRM render node is associated with the S30 PCI function")
fi

if [[ "${MUSA_S30_TEST_APPROVED:-}" != "1" ]]; then
    BLOCKERS+=("set MUSA_S30_TEST_APPROVED=1 only after reviewing the exact driver/core and recovery plan")
fi
if [[ ! -x "$MUSA_INFO" || ! -x "$MCC" || ! -f "$TEST_SOURCE" ]]; then
    BLOCKERS+=("MUSA runtime, compiler, or vector-add source is missing under $MUSA_PATH")
fi

if (( ${#BLOCKERS[@]} > 0 )); then
    echo "GPU_TEST_STATUS: NOT RUN"
    for reason in "${BLOCKERS[@]}"; do
        printf 'BLOCKED: %s\n' "$reason"
    done
    exit 3
fi

echo "S30 PCI function: $BDF"
echo "S30 render node: $RENDER_NODE"
echo "Driver module: ${MODULE_PATH##*/}"

TMP_DIR="$(mktemp -d "${TMPDIR:-/tmp}/musa-s30-runtime.XXXXXX")"
trap 'rm -rf -- "$TMP_DIR"' EXIT
export LD_LIBRARY_PATH="$MUSA_PATH/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"

echo
echo "Querying the MUSA runtime for the device..."
"$MUSA_INFO"

echo
echo "Compiling the vector-add smoke test..."
"$MCC" --musa-path="$MUSA_PATH" "$TEST_SOURCE" -o "$TMP_DIR/vector_add"

echo
echo "Running one vector-add, copy, allocation, and synchronization check..."
"$TMP_DIR/vector_add"

echo
echo "GPU_TEST_STATUS: PASS (S30 discovery and vector-add smoke test only)"
echo "This result does not qualify other MUSA features or workloads."

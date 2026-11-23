#!/usr/bin/env bash
# Gated MTT S30 runtime smoke test. Never loads or unloads kernel modules.
set -euo pipefail

BDF="0000:01:00.0"
DEVICE="/sys/bus/pci/devices/$BDF"
MUSA_PATH="${MUSA_PATH:-/usr/local/musa-5.2}"
MUSA_RUNTIME_HOME="${MUSA_RUNTIME_HOME:-/usr/local/musa}"
MUSA_RT_LIB="${MUSA_RT_LIB:-$MUSA_RUNTIME_HOME/lib}"
MCC="$MUSA_PATH/bin/mcc"
# The runtime musaInfo (1.5.4) answers the device query correctly.
# The SDK 5.2.0 musaInfo aborts (EINVAL / libdrm-mt refcount) when it
# links libmusart.so.5, so prefer the runtime tool.
MUSA_INFO="$MUSA_RUNTIME_HOME/bin/musaInfo"
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

# Do not run GPU workloads in a boot that already logged an mtgpu kernel fault.
# A fault leaves the driver in an unknown state even though the module may
# still report initstate=live and answer status queries.
FAULT_RE='BUG: kernel NULL pointer dereference|Oops:|general protection fault|kernel BUG at|invalid opcode'
DRIVER_ERROR_RE='mtgpu .*Invalid cmd_type:'
if command -v journalctl >/dev/null 2>&1; then
    if BOOTLOG="$(journalctl -k -b 0 --no-pager 2>/dev/null)"; then
        if grep -qE "$FAULT_RE" <<<"$BOOTLOG" && grep -q '\[mtgpu\]' <<<"$BOOTLOG"; then
            BLOCKERS+=("this boot logged an mtgpu kernel fault (Oops/BUG); reboot before GPU tests")
        fi
        if grep -qE "$DRIVER_ERROR_RE" <<<"$BOOTLOG"; then
            BLOCKERS+=("this boot logged an mtgpu Invalid cmd_type error; reboot before GPU tests")
        fi
    else
        BLOCKERS+=("cannot read the kernel journal to confirm a clean boot (journalctl -k -b 0 failed)")
    fi
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
if [[ ! -x "$MUSA_INFO" || ! -x "$MCC" || ! -f "$TEST_SOURCE" ||
      ! -e "$MUSA_RT_LIB/libmusart.so" ]]; then
    BLOCKERS+=("compatible MUSA 1.5.4 runtime, compiler, or vector-add source is missing")
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
# The runtime (1.5.4) lives in /usr/local/musa; the SDK (5.2.0) in
# /usr/local/musa-5.2.  Both provide libmusart at runtime for the smoke test.
export LD_LIBRARY_PATH="$MUSA_RT_LIB${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"

echo
echo "Compiling the vector-add smoke test..."
"$MCC" --musa-path="$MUSA_PATH" "$TEST_SOURCE" \
    -O2 --offload-arch=mp_10 \
    -I"$MUSA_PATH/include" -L"$MUSA_RT_LIB" -lmusart \
    -Wl,-rpath,"$MUSA_RT_LIB" -o "$TMP_DIR/vector_add"

# Verify the built binary was linked against the runtime 1.5.4 soname
# (libmusart.so.1.5) and NOT the SDK's libmusart.so.5. A mismatch aborts
# inside musaMalloc() and is a hard build error: fail closed before any
# GPU call, never continue with a wrong runtime.
RUNTIME_SONAME="$(readelf -d "$MUSA_RT_LIB/libmusart.so" 2>/dev/null |
    sed -n 's/.*SONAME.*\[\(.*\)\].*/\1/p' | head -1)"
VECTOR_NEEDED="$(readelf -d "$TMP_DIR/vector_add" 2>/dev/null |
    sed -n 's/.*NEEDED.*\[\(.*\)\].*/\1/p' | grep '^libmusart\.so\.' || true)"
INFO_NEEDED="$(readelf -d "$MUSA_INFO" 2>/dev/null |
    sed -n 's/.*NEEDED.*\[\(.*\)\].*/\1/p' | grep '^libmusart\.so\.' || true)"
if [[ "$RUNTIME_SONAME" != "libmusart.so.1.5" ||
      "$VECTOR_NEEDED" != "$RUNTIME_SONAME" || "$INFO_NEEDED" != "$RUNTIME_SONAME" ]]; then
    echo "ERROR: MUSA runtime ABI mismatch; expected libmusart.so.1.5" >&2
    echo "       library='$RUNTIME_SONAME' vector_add='$VECTOR_NEEDED' musaInfo='$INFO_NEEDED'" >&2
    echo "       Refusing device initialization with an incompatible runtime ABI." >&2
    exit 1
fi

echo
echo "Querying the MUSA runtime for the device..."
"$MUSA_INFO"

echo
echo "Running one vector-add, copy, allocation, and synchronization check..."
"$TMP_DIR/vector_add"

echo
echo "GPU_TEST_STATUS: PASS (S30 discovery and vector-add smoke test only)"
echo "This result does not qualify other MUSA features or workloads."

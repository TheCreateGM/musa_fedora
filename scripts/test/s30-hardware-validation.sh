#!/usr/bin/env bash
# s30-hardware-validation.sh — hardware validation ladder for the MTT S30.
#
# Runs the mtgpu 3.1.0 driver and the MUSA stack on the physical MTT S30 and
# records a verbatim PASS/FAIL result for every step. The script never claims
# a feature works unless the corresponding command actually succeeded.
#
# Safety rules enforced by this script:
#   * At most ONE module load per boot. If mtgpu is already loaded, the load
#     step is skipped and reported as such.
#   * No module unload, no PCI unbind/rebind, no second load attempt.
#   * The driver is always loaded in headless mode (display=none,
#     disable_fbdev=1) so the Intel/i915 display stack keeps the console.
#   * After every step the kernel journal is checked for faults. The first
#     Oops/BUG/GPF ends the ladder immediately.
#   * Any command that can hang is run under `timeout`.
#
# Usage:
#   ./scripts/test/s30-hardware-validation.sh              # validate a loaded driver
#   ./scripts/test/s30-hardware-validation.sh --load       # load (once) and validate
#   ./scripts/test/s30-hardware-validation.sh --load --offline   # skip network
#
# Exit codes: 0 = all steps passed, 1 = a step failed, 2 = setup problem,
#             3 = validation blocked (driver not loaded / not bound).
#
# The script is safe to run detached, e.g.
#   setsid nohup ./scripts/test/s30-hardware-validation.sh --load \
#       >/tmp/s30-validation.log 2>&1 &
set -uo pipefail

ROOT="$(cd -- "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
TESTS="$ROOT/tests"
BUILD="$ROOT/build"

# Prefixes. The S30 is validated against the driver-3.1.0-compatible MUSA
# runtime 1.5.4 and the MUSA 5.2.0 compiler. The 5.2.0 *runtime* is NOT ABI
# compatible with the 3.1.0 driver and must never be used here.
MUSA_RT="${MUSA_RT:-/usr/local/musa}"
MUSA_SDK="${MUSA_SDK:-/usr/local/musa-5.2}"
MUSA_RT_LIB="$MUSA_RT/lib"
MUSA_RT_INC="$MUSA_RT/include"
MCC="$MUSA_SDK/bin/mcc"

BDF="0000:01:00.0"
DEVICE="/sys/bus/pci/devices/$BDF"
MODULE_PARAMS=(display=none disable_fbdev=1 mtgpu_drm_major=2)

DO_LOAD=0
LOG_DIR="${LOG_DIR:-}"
while (($#)); do
    case "$1" in
        --load) DO_LOAD=1; shift ;;
        --no-load) DO_LOAD=0; shift ;;
        --log-dir) LOG_DIR="$2"; shift 2 ;;
        -h|--help) sed -n '2,30p' "${BASH_SOURCE[0]}"; exit 0 ;;
        *) echo "Unknown option: $1" >&2; exit 2 ;;
    esac
done

if [[ -z "$LOG_DIR" ]]; then
    LOG_DIR="$(mktemp -d "${TMPDIR:-/tmp}/musa-s30-validation.XXXXXX")"
fi
mkdir -p "$LOG_DIR"
LOG="$LOG_DIR/validation.log"

PASS_COUNT=0
FAIL_COUNT=0
BLOCK_COUNT=0
FAULT_SEEN=""

FAULT_RE='BUG: kernel NULL pointer dereference|Oops:|general protection fault|kernel BUG at|invalid opcode|Missing ENDBR|Call Trace:'

step() { printf '\n=== %s ===\n' "$*"; }
pass() { printf '  PASS  %s\n' "$*"; PASS_COUNT=$((PASS_COUNT + 1)); }
fail() { printf '  FAIL  %s\n' "$*"; FAIL_COUNT=$((FAIL_COUNT + 1)); }
block() { printf '  BLOCK %s\n' "$*"; BLOCK_COUNT=$((BLOCK_COUNT + 1)); }
note() { printf '        %s\n' "$*"; }

log() { printf '%s\n' "$*" | tee -a "$LOG" >/dev/null; }

# --- journal fault helper -------------------------------------------------

journal_faults() {
    journalctl -k -b --no-pager 2>/dev/null | grep -E "$FAULT_RE" || true
}

check_no_fault() {
    # Returns 0 (clean) or 1 (fault seen). A fault stops the ladder.
    local new
    new="$(journal_faults | tail -40)"
    if [[ -n "$new" ]]; then
        FAULT_SEEN="$new"
        printf '  FAIL  kernel fault detected in the journal:\n'
        printf '%s\n' "$new" | sed 's/^/        /'
        return 1
    fi
    return 0
}

recorded_faults() {
    # Faults already present before this run started (from earlier boots or
    # earlier steps of this run) are not attributed to the current step.
    journal_faults
}

BASE_FAULTS="$(recorded_faults)"

printf 'MTT S30 hardware validation — %s\n' "$(date -Is)"
printf 'kernel: %s\n' "$(uname -r)"
printf 'log directory: %s\n' "$LOG_DIR"
printf 'MUSA runtime: %s   MUSA compiler: %s\n' "$MUSA_RT" "$MUSA_SDK"
printf 'pre-existing journal fault lines: %s\n' \
    "$(printf '%s' "$BASE_FAULTS" | grep -c . || true)"

# ---------------------------------------------------------------------------
step "0. Environment"
for f in "$MCC" "$MUSA_RT/bin/musaInfo" "$MUSA_RT/bin/mthreads-gmi" \
         "$MUSA_RT/bin/muMemcpyTest"; do
    if [[ -x "$f" ]]; then pass "present: $f"; else fail "missing: $f"; fi
done
for d in "$MUSA_RT_LIB" "$MUSA_RT_INC" "$MUSA_SDK/include"; do
    [[ -d "$d" ]] && pass "directory: $d" || fail "missing directory: $d"
done
(( FAIL_COUNT > 0 )) && { printf '\nVALIDATION_STATUS: BLOCKED (setup incomplete)\n'; exit 2; }

# ---------------------------------------------------------------------------
step "1. PCI identity"
if [[ -r "$DEVICE/vendor" && -r "$DEVICE/device" ]] &&
   [[ "$(cat "$DEVICE/vendor")" == "0x1ed5" && "$(cat "$DEVICE/device")" == "0x0102" ]]; then
    pass "MTT S30 present at $BDF (vendor 0x1ed5, device 0x0102)"
else
    fail "MTT S30 not found at $BDF"
    printf '\nVALIDATION_STATUS: BLOCKED (no S30)\n'; exit 3
fi

# ---------------------------------------------------------------------------
step "2. mtgpu driver state"
MODULE_FILE=""
if [[ -r /sys/module/mtgpu/initstate ]]; then
    INITSTATE="$(cat /sys/module/mtgpu/initstate)"
    if [[ "$INITSTATE" == "live" ]]; then
        pass "mtgpu already loaded (initstate=live); skipping the load step"
    else
        fail "mtgpu present but initstate=$INITSTATE (expected live)"
    fi
else
    if (( DO_LOAD )); then
        for c in "/lib/modules/$(uname -r)/extra/mtgpu.ko" \
                 "/lib/modules/$(uname -r)/extra/mtgpu.ko.xz" \
                 "/lib/modules/$(uname -r)/extra/mtgpu.ko.zst"; do
            [[ -f "$c" ]] && { MODULE_FILE="$c"; break; }
        done
        if [[ -z "$MODULE_FILE" ]]; then
            fail "no mtgpu module file for kernel $(uname -r)"
            printf '\nVALIDATION_STATUS: BLOCKED (no module)\n'; exit 3
        fi
        note "loading $MODULE_FILE ${MODULE_PARAMS[*]}"
        if timeout 300 pkexec /usr/sbin/insmod "$MODULE_FILE" "${MODULE_PARAMS[@]}" \
                >"$LOG_DIR/insmod.out" 2>&1; then
            note "insmod returned 0"
        else
            note "insmod returned $? (see $LOG_DIR/insmod.out)"
        fi
        cat "$LOG_DIR/insmod.out" 2>/dev/null | sed 's/^/        /'
        sleep 3
        if [[ -r /sys/module/mtgpu/initstate ]]; then
            note "initstate=$(cat /sys/module/mtgpu/initstate)"
        else
            fail "mtgpu did not load"
            printf '\nVALIDATION_STATUS: BLOCKED (load failed)\n'; exit 3
        fi
    else
        block "mtgpu is not loaded (pass --load to load it once)"
        printf '\nVALIDATION_STATUS: BLOCKED (driver not loaded)\n'; exit 3
    fi
fi
check_no_fault || { printf '\nVALIDATION_STATUS: FAIL (kernel fault during load)\n'; exit 1; }

if [[ -r /sys/module/mtgpu/initstate && "$(cat /sys/module/mtgpu/initstate)" == "live" ]]; then
    pass "mtgpu module is live"
else
    fail "mtgpu module is not live"
    printf '\nVALIDATION_STATUS: FAIL (driver not live)\n'; exit 1
fi

DRIVER_LINK="$(readlink -f "$DEVICE/driver" 2>/dev/null || true)"
if [[ "${DRIVER_LINK##*/}" == "mtgpu" ]]; then
    pass "S30 is bound to the mtgpu driver ($DRIVER_LINK)"
else
    fail "S30 is not bound to mtgpu (driver=${DRIVER_LINK:-none})"
fi

# Kernel message tail from the driver, for the record.
note "driver messages:"
journalctl -k -b --no-pager 2>/dev/null | grep -iE "mtgpu|mthreads|PVR|pvr" | tail -15 | sed 's/^/        /'

# ---------------------------------------------------------------------------
step "3. Device nodes"
[[ -c /dev/mtgpu.0 ]] && pass "/dev/mtgpu.0 present" || fail "/dev/mtgpu.0 missing"

RENDER_NODE=""
for sysdev in /sys/class/drm/renderD*/device; do
    [[ -e "$sysdev" ]] || continue
    if [[ "$(readlink -f "$sysdev" 2>/dev/null)" == *"/$BDF" ]]; then
        RENDER_NODE="/dev/dri/$(basename "${sysdev%/device}")"
        break
    fi
done
if [[ -n "$RENDER_NODE" && -c "$RENDER_NODE" ]]; then
    pass "S30 DRM render node: $RENDER_NODE"
else
    fail "no DRM render node associated with the S30"
fi
note "DRM nodes now present: $(ls /dev/dri 2>/dev/null | tr '\n' ' ')"

check_no_fault || { printf '\nVALIDATION_STATUS: FAIL (kernel fault after load)\n'; exit 1; }

# ---------------------------------------------------------------------------
step "4. mthreads-gmi (driver/GPU identity)"
if timeout 60 "$MUSA_RT/bin/mthreads-gmi" >"$LOG_DIR/gmi.out" 2>&1; then
    sed 's/^/        /' "$LOG_DIR/gmi.out"
    pass "mthreads-gmi completed"
else
    sed 's/^/        /' "$LOG_DIR/gmi.out"
    fail "mthreads-gmi failed (rc=$?)"
fi
check_no_fault || { printf '\nVALIDATION_STATUS: FAIL (kernel fault in gmi)\n'; exit 1; }

# ---------------------------------------------------------------------------
step "5. MUSA runtime 1.5.4 device enumeration"
export LD_LIBRARY_PATH="$MUSA_RT_LIB${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
if timeout 120 "$MUSA_RT/bin/musaInfo" >"$LOG_DIR/musaInfo.out" 2>&1; then
    sed 's/^/        /' "$LOG_DIR/musaInfo.out"
    pass "musaInfo (runtime 1.5.4) enumerated the device"
else
    sed 's/^/        /' "$LOG_DIR/musaInfo.out"
    fail "musaInfo failed"
fi
check_no_fault || { printf '\nVALIDATION_STATUS: FAIL (kernel fault in musaInfo)\n'; exit 1; }

# ---------------------------------------------------------------------------
step "6. Vendor copy test (muMemcpyTest)"
if timeout 300 "$MUSA_RT/bin/muMemcpyTest" >"$LOG_DIR/memcpytest.out" 2>&1; then
    tail -20 "$LOG_DIR/memcpytest.out" | sed 's/^/        /'
    pass "muMemcpyTest passed"
else
    tail -30 "$LOG_DIR/memcpytest.out" | sed 's/^/        /'
    fail "muMemcpyTest failed"
fi
check_no_fault || { printf '\nVALIDATION_STATUS: FAIL (kernel fault in copy test)\n'; exit 1; }

# ---------------------------------------------------------------------------
step "7. Host runtime API smoke test"
mkdir -p "$BUILD"
SMOKE="$BUILD/musart-smoke"
if [[ ! -x "$SMOKE" || "$TESTS/musart-smoke.c" -nt "$SMOKE" ]]; then
    if cc -O2 "$TESTS/musart-smoke.c" -I"$MUSA_RT_INC" \
        -L"$MUSA_RT_LIB" -lmusart -Wl,-rpath,"$MUSA_RT_LIB" \
        -o "$SMOKE" >"$LOG_DIR/musart-smoke.build" 2>&1; then
        pass "musart-smoke rebuilt from source"
    else
        sed 's/^/        /' "$LOG_DIR/musart-smoke.build"
        fail "musart-smoke failed to build"
    fi
fi
if [[ -x "$SMOKE" ]]; then
    if timeout 180 "$SMOKE" >"$LOG_DIR/musart-smoke.out" 2>&1; then
        sed 's/^/        /' "$LOG_DIR/musart-smoke.out"
        pass "host runtime API smoke test passed"
    else
        sed 's/^/        /' "$LOG_DIR/musart-smoke.out"
        fail "host runtime API smoke test failed (rc=$?)"
    fi
fi
check_no_fault || { printf '\nVALIDATION_STATUS: FAIL (kernel fault in smoke test)\n'; exit 1; }

# ---------------------------------------------------------------------------
step "8. mcc mp_10 vector-add kernel on the S30"
WORK="$(mktemp -d "${TMPDIR:-/tmp}/musa-s30-va.XXXXXX")"
if timeout 300 "$MCC" -O2 --offload-arch=mp_10 \
        -I"$MUSA_SDK/include" -L"$MUSA_RT_LIB" -lmusart \
        -Wl,-rpath,"$MUSA_RT_LIB" \
        -o "$WORK/vector_add" "$TESTS/musa_vector_add.mu" \
        >"$LOG_DIR/vector_add.build" 2>&1; then
    pass "mcc compiled musa_vector_add.mu for mp_10"
else
    sed 's/^/        /' "$LOG_DIR/vector_add.build"
    fail "mcc failed to compile the vector-add kernel"
fi
if [[ -x "$WORK/vector_add" ]]; then
    if timeout 300 "$WORK/vector_add" >"$LOG_DIR/vector_add.out" 2>&1; then
        sed 's/^/        /' "$LOG_DIR/vector_add.out"
        pass "vector-add kernel executed and verified on the S30"
    else
        sed 's/^/        /' "$LOG_DIR/vector_add.out"
        fail "vector-add kernel failed (rc=$?)"
    fi
fi
check_no_fault || { printf '\nVALIDATION_STATUS: FAIL (kernel fault in vector-add)\n'; exit 1; }

# ---------------------------------------------------------------------------
step "9. Compute suite (musa-compute.mu) and FP64"
for pair in "musa-compute.mu:compute" "musa-fp64.mu:fp64"; do
    src="${pair%%:*}"; bin="${pair##*:}"
    if timeout 300 "$MCC" -O2 --offload-arch=mp_10 \
            -I"$MUSA_SDK/include" -L"$MUSA_RT_LIB" -lmusart \
            -Wl,-rpath,"$MUSA_RT_LIB" \
            -o "$WORK/$bin" "$TESTS/$src" >"$LOG_DIR/$bin.build" 2>&1; then
        if timeout 300 "$WORK/$bin" >"$LOG_DIR/$bin.out" 2>&1; then
            sed 's/^/        /' "$LOG_DIR/$bin.out"
            pass "$src executed successfully"
        else
            sed 's/^/        /' "$LOG_DIR/$bin.out"
            fail "$src failed (rc=$?)"
        fi
    else
        sed 's/^/        /' "$LOG_DIR/$bin.build"
        fail "$src failed to compile"
    fi
    check_no_fault || { printf '\nVALIDATION_STATUS: FAIL (kernel fault in %s)\n' "$src"; exit 1; }
done

# ---------------------------------------------------------------------------
step "10. Headless Vulkan device/buffer/memory"
VULKAN_ICD=""
for c in /etc/vulkan/icd.d/musaicdconf.json /usr/share/vulkan/icd.d/musaicdconf.json; do
    [[ -f "$c" ]] && { VULKAN_ICD="$c"; break; }
done
if [[ -z "$VULKAN_ICD" ]]; then
    block "no Moore Threads Vulkan ICD manifest found (skipping)"
else
    note "ICD manifest: $VULKAN_ICD"
    if [[ ! -x "$BUILD/vulkan-enumerate" ]]; then
        cc -O2 "$TESTS/vulkan-enumerate.c" -ldl -o "$BUILD/vulkan-enumerate" \
            >"$LOG_DIR/vulkan.build" 2>&1 || \
            printf '        vulkan-enumerate build failed\n'
    fi
    if [[ -x "$BUILD/vulkan-enumerate" ]]; then
        if VK_DRIVER_FILES="$VULKAN_ICD" timeout 120 "$BUILD/vulkan-enumerate" \
                >"$LOG_DIR/vulkan.out" 2>&1; then
            sed 's/^/        /' "$LOG_DIR/vulkan.out"
            pass "headless Vulkan device/buffer/memory path passed"
        else
            sed 's/^/        /' "$LOG_DIR/vulkan.out"
            fail "headless Vulkan test failed (rc=$?)"
        fi
    else
        fail "vulkan-enumerate binary unavailable"
    fi
    check_no_fault || { printf '\nVALIDATION_STATUS: FAIL (kernel fault in Vulkan test)\n'; exit 1; }
fi

# ---------------------------------------------------------------------------
step "11. Display stack integrity"
if [[ -e /sys/class/drm/card0/device/driver ]] &&
   [[ "$(readlink -f /sys/class/drm/card0/device/driver)" == *i915* ]]; then
    pass "Intel i915 still owns card0 (display unaffected)"
else
    fail "card0 is no longer bound to i915"
fi
if systemctl is-active --quiet display-manager; then
    pass "display-manager is active"
else
    fail "display-manager is not active"
fi
if pgrep -x gnome-shell >/dev/null 2>&1; then
    pass "gnome-shell is running"
else
    fail "gnome-shell is not running"
fi

rm -rf "$WORK"

# ---------------------------------------------------------------------------
printf '\n=====================================================\n'
printf 'VALIDATION_STATUS: %s\n' \
    "$( ((FAIL_COUNT == 0)) && echo PASS || echo FAIL )"
printf 'passed=%d failed=%d blocked=%d\n' "$PASS_COUNT" "$FAIL_COUNT" "$BLOCK_COUNT"
printf 'log directory: %s\n' "$LOG_DIR"
printf '=====================================================\n'
if (( FAIL_COUNT > 0 )); then exit 1; fi
exit 0

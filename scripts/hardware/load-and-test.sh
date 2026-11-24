#!/usr/bin/env bash
# load-and-test.sh — one controlled mtgpu load followed by the gated GPU tests.
#
# Safety contract (matches docs/maintenance-plan-mtgpu-load-2026-11-20.md):
#   * the /etc/modprobe.d blacklist is never touched, so a reboot restores the
#     known-good display-only state;
#   * exactly one insmod per boot, headless arguments only, no rmmod, no
#     PCI unbind/rebind;
#   * every step is appended to a log file under $HOME/.musa-work so the
#     evidence survives a desktop crash.
#
# Exit codes: 0 all steps passed, 1 a test step failed, 2 refused to load,
# 3 a kernel fault was seen (testing stopped).
set -uo pipefail

ROOT="$(cd -- "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
MUSA_RT_LIB="${MUSA_RT_LIB:-/usr/local/musa/lib}"
MUSA_RT_INC="${MUSA_RT_INC:-/usr/local/musa/include}"
MUSA_RT_BIN="${MUSA_RT_BIN:-$(dirname "$MUSA_RT_LIB")/bin}"
MUSA_SDK="${MUSA_SDK:-/usr/local/musa-5.2}"
BDF="0000:01:00.0"
LOG_DIR="${LOG_DIR:-$HOME/.musa-work/load-test-$(date -u +%Y%m%dT%H%M%SZ)}"
LOGFILE="$LOG_DIR/load-and-test.log"
FAULT_RE='BUG: kernel NULL pointer dereference|Oops:|general protection fault|kernel BUG at|invalid opcode'

mkdir -p "$LOG_DIR"
# Keep console progress on fd 3 and the complete run (including test output)
# in the log file. Write each log line exactly once: a previous version both
# redirected stdout into the file and piped log() through tee, which duplicated
# every line in the evidence log.
exec 3>&1
exec >>"$LOGFILE" 2>&1
log() {
    local m
    m="$(date -u +%H:%M:%SZ) $*"
    printf '%s\n' "$m" >>"$LOGFILE"
    printf '%s\n' "$m" >&3
}
hdr() { log ""; log "===== $* ====="; }

hdr "start $(date -u +%FT%TZ) kernel=$(uname -r)"
log "log file: $LOGFILE"

hdr "pre-flight"
if [[ -r /sys/module/mtgpu/initstate ]]; then
    log "REFUSE: mtgpu already loaded (initstate=$(cat /sys/module/mtgpu/initstate))"
    exit 2
fi
MODULE=""
for candidate in \
    "/lib/modules/$(uname -r)/extra/mtgpu.ko" \
    "/lib/modules/$(uname -r)/extra/mtgpu.ko.xz" \
    "/lib/modules/$(uname -r)/extra/mtgpu.ko.zst"; do
    [[ -f "$candidate" ]] && { MODULE="$candidate"; break; }
done
if [[ -z "$MODULE" ]]; then
    log "REFUSE: no mtgpu module for kernel $(uname -r)"
    exit 2
fi

# Stage the GPU test binaries into the log directory. Without this the
# binary-driven steps below fail with rc=127 after the driver is already
# loaded — a load wasted on a missing binary. Prefer a staged set in
# ~/.musa-work/test-bin (built reproducibly by
# scripts/hardware/build-test-binaries.sh); build it there if absent.
stage_test_binaries() {
    local src="$HOME/.musa-work/test-bin"
    local needed=(musart-smoke musa-vector-add musa-compute musa-fp64 vk-enum)
    local missing=0 b
    for b in "${needed[@]}"; do
        [[ -x "$LOG_DIR/$b" ]] || missing=1
    done
    if ((missing == 0)); then return 0; fi
    for b in "${needed[@]}"; do
        if [[ -x "$src/$b" ]]; then
            cp -a "$src/$b" "$LOG_DIR/$b"
        fi
    done
    for b in "${needed[@]}"; do
        if [[ ! -x "$LOG_DIR/$b" ]]; then
            log "building missing test binary: $b"
            if [[ ! -x "$src/" ]]; then mkdir -p "$src"; fi
            if ! TARGET_DIR="$src" "$ROOT/scripts/hardware/build-test-binaries.sh" "$src" \
                    >>"$LOG_DIR/build-test-binaries.log" 2>&1; then
                log "REFUSE: cannot build $b (see build-test-binaries.log)"
                exit 2
            fi
            cp -a "$src/$b" "$LOG_DIR/$b"
        fi
    done
    local staged=0 b
    for b in "${needed[@]}"; do
        [[ -x "$LOG_DIR/$b" ]] && staged=$((staged + 1))
    done
    log "test binaries staged: $staged/${#needed[@]}"
}
log "module: $MODULE"
log "sha256: $(sha256sum "$MODULE")"
log "core:   $(sha256sum /usr/src/mtgpu-3.1.0/objs/x86_64/mtgpu_core.o_binary 2>/dev/null || echo n/a)"
log "cmdline: $(cat /proc/cmdline)"
log "blacklist: $(tr '\n' ' ' < /etc/modprobe.d/blacklist-mtgpu.conf)"
log "display options: $(grep -v '^#' /etc/modprobe.d/90-mtgpu-headless.conf | tr '\n' ' ')"
log "GNOME before load: $(pgrep -c gnome-shell || echo 0) gnome-shell process(es)"
log "sessions before load: $(loginctl list-sessions --no-legend | wc -l)"

stage_test_binaries

hdr "load (headless args, single attempt)"
timeout 180 pkexec /usr/sbin/insmod "$MODULE" \
    display=none disable_fbdev=1 mtgpu_drm_major=2
log "insmod rc=$?"
sleep 3

hdr "post-load kernel journal (mtgpu/mvgal/drm)"
journalctl -k -b --no-pager --since "-2 min" | grep -iE "mtgpu|mvgal|drm|ENDBR|IBT" | tail -40
log "initstate=$(cat /sys/module/mtgpu/initstate 2>/dev/null || echo absent)"
if ! grep -qE "$FAULT_RE" <<<"$(journalctl -k -b --no-pager | grep '\[mtgpu\]')"; then
    log "no mtgpu kernel fault seen so far"
else
    log "KERNEL FAULT during load"
    exit 3
fi

hdr "device nodes"
ls -la /dev/mtgpu* 2>/dev/null
for sysdev in /sys/class/drm/renderD*/device; do
    [[ -e "$sysdev" ]] || continue
    [[ "$(readlink -f "$sysdev")" == *"/$BDF" ]] && log "S30 render node: /dev/dri/$(basename "${sysdev%/device}")"
done
log "gnome-shell after load: $(pgrep -c gnome-shell || echo 0) process(es)"

step() {
    local label="$1"; shift
    hdr "$label"
    "$@"
    local rc=$?
    log "STEP RESULT: $label rc=$rc"
    if grep -qE "$FAULT_RE" <<<"$(journalctl -k -b --no-pager | grep '\[mtgpu\]')"; then
        log "KERNEL FAULT after step: $label"
        return 13
    fi
    return $rc
}

rc_total=0
run() { step "$@" || rc_total=1; }

run "telemetry: mthreads-gmi -q" env LD_LIBRARY_PATH="$MUSA_RT_LIB" timeout 60 mthreads-gmi -q
# Use the runtime's own 1.5.4 musaInfo. The 5.2 SDK copy links
# libmusart.so.5 and crashes against the 3.1.0 driver / 1.5.4 runtime
# (mtgpu_query_marketing_name EINVAL, libdrm-mt assertion).
run "runtime discovery: musaInfo" env LD_LIBRARY_PATH="$MUSA_RT_LIB" timeout 60 "$MUSA_RT_BIN/musaInfo"
run "vendor copy test: muMemcpyTest" env LD_LIBRARY_PATH="$MUSA_RT_LIB" timeout 120 /usr/local/musa/bin/muMemcpyTest
run "host runtime API: musart-smoke" env LD_LIBRARY_PATH="$MUSA_RT_LIB" timeout 60 "$LOG_DIR/musart-smoke"
run "kernel: vector-add (mp_10)" env LD_LIBRARY_PATH="$MUSA_RT_LIB" timeout 120 "$LOG_DIR/musa-vector-add"
run "kernel: compute suite" env LD_LIBRARY_PATH="$MUSA_RT_LIB" timeout 180 "$LOG_DIR/musa-compute"
run "kernel: fp64" env LD_LIBRARY_PATH="$MUSA_RT_LIB" timeout 120 "$LOG_DIR/musa-fp64"
run "vulkan headless: device/buffer/map" env \
    VK_DRIVER_FILES=/etc/vulkan/icd.d/musaicdconf.json \
    VK_ICD_FILENAMES=/etc/vulkan/icd.d/musaicdconf.json \
    timeout 60 "$LOG_DIR/vk-enum"

hdr "desktop status at end"
log "gnome-shell: $(pgrep -c gnome-shell || echo 0) process(es); gdm: $(systemctl is-active gdm)"
log "mtgpu faults in this boot: $(grep -cE "$FAULT_RE" <<<"$(journalctl -k -b --no-pager | grep '\[mtgpu\]')")"
hdr "done $(date -u +%FT%TZ) overall rc=$rc_total"
exit $rc_total

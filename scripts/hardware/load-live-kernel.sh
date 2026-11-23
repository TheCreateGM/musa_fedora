#!/usr/bin/env bash
# load-live-kernel.sh — validate MUSA on the S30 from a Live USB boot by
# loading a module built for the *running* live kernel and running the GPU
# tests inside the installed Fedora root (chroot), which supplies the MUSA
# 1.5.4 runtime, the 5.2.0 compiler-built test binaries, firmware and Vulkan
# ICD.
#
# Why this exists:
#   The installed system and the Live USB are different kernels. A module built
#   for the installed kernel cannot be loaded into the live kernel. This script
#   keeps the build (scripts/build-kernel-driver.sh, run in a disposable
#   container) separate from the load and makes the live-kernel result
#   reproducible.
#
# Safety contract:
#   * Refuses if mtgpu is already loaded, the S30 is already bound, a previous
#     mtgpu load fault is in this boot, or MUSA DRI/GBM/Xorg drivers are visible
#     on the live host (the documented mutter crash condition).
#   * One insmod with headless parameters only: display=none disable_fbdev=1
#     mtgpu_drm_major=2. No rmmod, no PCI unbind/rebind, no configuration
#     changes to the installed root.
#   * The only host changes are a temporary `/lib/firmware/mthreads` bind mount,
#     temporary /proc,/dev,/sys bind mounts inside the installed root, and the
#     loaded module. All mounts are removed on exit. A reboot restores the live
#     session's original, unbound state.
#   * Logs are written outside the live overlay (caller-supplied --log-dir) so
#     they survive a desktop or kernel failure.
#
# Exit codes:
#   0  all functional checks passed and the desktop survived
#   1  a functional check failed (see log) but no kernel fault was observed
#   2  preflight refused to load
#   3  a kernel fault was observed; testing stopped
#   4  insmod failed or the device did not initialize
#
# Usage (root, normally via pkexec):
#   pkexec scripts/hardware/load-live-kernel.sh \
#       --module /path/to/mtgpu.ko \
#       --installed-root /run/media/liveuser/fedora/root \
#       --test-bin /path/to/test-bin \
#       --log-dir /path/to/logs
#
# Add --dry-run to perform the preflight and print the plan without loading.
set -uo pipefail

MODULE=""
INSTALLED_ROOT=""
TEST_BIN=""
LOG_DIR=""
DRY_RUN=0
SKIP_VULKAN=0
BDF="0000:01:00.0"
MUSA_RT_LIB="/usr/local/musa/lib"
MUSA_SDK="/usr/local/musa-5.2"
FAULT_RE='BUG: kernel NULL pointer dereference|Oops:|general protection fault|kernel BUG at|invalid opcode|Unable to handle kernel paging request|RIP: 0010:'

usage() {
    sed -n '3,50p' "$0" | sed 's/^# \{0,1\}//'
    exit 2
}

while (($#)); do
    case "$1" in
        --module) MODULE="${2:-}"; shift 2 ;;
        --installed-root) INSTALLED_ROOT="${2:-}"; shift 2 ;;
        --test-bin) TEST_BIN="${2:-}"; shift 2 ;;
        --log-dir) LOG_DIR="${2:-}"; shift 2 ;;
        --dry-run) DRY_RUN=1; shift ;;
        --skip-vulkan) SKIP_VULKAN=1; shift ;;
        -h|--help) usage ;;
        *) echo "unknown argument: $1" >&2; usage ;;
    esac
done

[[ -n "$MODULE" && -n "$INSTALLED_ROOT" && -n "$TEST_BIN" && -n "$LOG_DIR" ]] || usage
[[ $(id -u) -eq 0 ]] || { echo "ERROR: run as root (pkexec or sudo)" >&2; exit 2; }
[[ -f "$MODULE" ]] || { echo "ERROR: module not found: $MODULE" >&2; exit 2; }
[[ -d "$INSTALLED_ROOT/usr/local/musa" ]] || {
    echo "ERROR: installed root has no /usr/local/musa: $INSTALLED_ROOT" >&2; exit 2; }
[[ -d "$TEST_BIN" ]] || { echo "ERROR: test binary directory not found: $TEST_BIN" >&2; exit 2; }

mkdir -p "$LOG_DIR"
LOGFILE="$LOG_DIR/load-live-kernel.log"
START_TS="$(date -u '+%Y-%m-%d %H:%M:%S')"
OWNER_UID="${PKEXEC_UID:-${SUDO_UID:-}}"

log()  { printf '%s %s\n' "$(date -u +%H:%M:%SZ)" "$*" | tee -a "$LOGFILE"; }
hdr()  { log ""; log "===== $* ====="; }

MOUNTS=()
CREATED_FW_DIR=0
CREATED_LOG_DIR=0
PRE_GNOME=0
DESKTOP_WAS_ACTIVE=0
LOADED=0
rc_total=0

cleanup() {
    local m
    for ((i=${#MOUNTS[@]}-1; i>=0; i--)); do
        m="${MOUNTS[$i]}"
        umount "$m" 2>/dev/null || true
    done
    if ((CREATED_FW_DIR)); then
        rmdir /lib/firmware/mthreads 2>/dev/null || true
    fi
    if ((CREATED_LOG_DIR)); then
        rmdir /var/log/mtgpu 2>/dev/null || true
    fi
    if [[ -n "$OWNER_UID" ]]; then
        chown -R "$OWNER_UID:$OWNER_UID" "$LOG_DIR" 2>/dev/null || true
    fi
}

mount_bind() {
    local src="$1" dst="$2"
    if mountpoint -q "$dst"; then
        log "already mounted (left untouched): $dst"
        return 0
    fi
    mkdir -p "$dst"
    if ! mount --bind "$src" "$dst"; then
        log "ERROR: bind mount failed: $src -> $dst"
        return 1
    fi
    MOUNTS+=("$dst")
    log "mounted: $src -> $dst"
}

faults_since_start() {
    journalctl -k -b --utc --since "$START_TS" --no-pager 2>/dev/null | grep -E "$FAULT_RE" || true
}

check_no_fault() {
    local phase="$1" hits
    hits="$(faults_since_start)"
    if [[ -n "$hits" ]]; then
        hdr "KERNEL FAULT after: $phase"
        printf '%s\n' "$hits" | tee -a "$LOGFILE"
        return 3
    fi
    log "no kernel fault after: $phase"
    return 0
}

find_s30_render() {
    local sysdev
    for sysdev in /sys/class/drm/renderD*/device; do
        [[ -e "$sysdev" ]] || continue
        if [[ "$(readlink -f "$sysdev")" == *"/$BDF" ]]; then
            printf '/dev/dri/%s\n' "$(basename "${sysdev%/device}")"
            return 0
        fi
    done
    return 1
}

run_in_root() {
    local label="$1"; shift
    local out rc
    hdr "$label"
    out="$(timeout 180 chroot "$INSTALLED_ROOT" /usr/bin/env -i \
        HOME=/root PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin \
        LD_LIBRARY_PATH="$MUSA_RT_LIB:/usr/lib64" \
        VK_DRIVER_FILES=/etc/vulkan/icd.d/musaicdconf.json \
        VK_ICD_FILENAMES=/etc/vulkan/icd.d/musaicdconf.json \
        "$@" 2>&1)"
    rc=$?
    printf '%s\n' "$out" | tee -a "$LOGFILE"
    log "STEP RESULT: $label rc=$rc"
    check_no_fault "$label" || return 3
    return $rc
}

trap cleanup EXIT

hdr "start $(date -u +%FT%TZ) kernel=$(uname -r) live-host=$(hostname)"
log "module: $MODULE"
log "module sha256: $(sha256sum "$MODULE" | awk '{print $1}')"
log "installed root: $INSTALLED_ROOT"
log "test binaries: $TEST_BIN"
log "log: $LOGFILE"
log "cmdline: $(cat /proc/cmdline)"
log "secure boot: $(mokutil --sb-state 2>&1 | tr '\n' ' ')"
log "IOMMU groups: $(ls /sys/kernel/iommu_groups 2>/dev/null | wc -l)"
log "S30 pci: $(lspci -Dn -s "$BDF" 2>&1)"

hdr "preflight: module"
if [[ "$(modinfo -F vermagic "$MODULE" | awk '{print $1}')" != "$(uname -r)" ]]; then
    log "REFUSE: module vermagic $(modinfo -F vermagic "$MODULE") does not match $(uname -r)"
    exit 2
fi
if ! modinfo -F alias "$MODULE" | grep -Fq 'pci:v00001ED5d00000102'; then
    log "REFUSE: module does not advertise the MTT S30 PCI ID"
    exit 2
fi
log "vermagic: $(modinfo -F vermagic "$MODULE")"
log "S30 alias present: pci:v00001ED5d00000102"

hdr "preflight: host state"
if [[ -e /sys/module/mtgpu ]]; then
    log "REFUSE: mtgpu is already loaded (initstate=$(cat /sys/module/mtgpu/initstate 2>/dev/null || echo unknown))"
    exit 2
fi
if [[ -e "/sys/bus/pci/devices/$BDF/driver" ]]; then
    log "REFUSE: $BDF already has a bound driver: $(basename "$(readlink -f "/sys/bus/pci/devices/$BDF/driver")")"
    exit 2
fi
if [[ -e /dev/mtgpu.0 ]]; then
    log "REFUSE: /dev/mtgpu.0 exists although mtgpu is not loaded"
    exit 2
fi
if ! lspci -Dn -s "$BDF" 2>/dev/null | grep -q '1ed5:0102'; then
    log "REFUSE: MTT S30 $BDF not present"
    exit 2
fi
PRE_GNOME="$(pgrep -c gnome-shell 2>/dev/null || true)"
log "gnome-shell processes: ${PRE_GNOME:-0}; gdm: $(systemctl is-active gdm 2>/dev/null || echo unknown)"
if [[ "${PRE_GNOME:-0}" -gt 0 ]]; then
    DESKTOP_WAS_ACTIVE=1
fi
if compgen -G '/usr/lib64/dri/*musa*' >/dev/null || compgen -G '/usr/lib64/dri/*mtgpu*' >/dev/null || \
   compgen -G '/usr/lib64/gbm/*musa*' >/dev/null || compgen -G '/usr/lib64/xorg/modules/drivers/*mtgpu*' >/dev/null; then
    log "REFUSE: MUSA DRI drivers are present on the live host; this is the documented mutter crash condition."
    log "        Remove musa-runtime-display (or the vendor DRI files) before loading mtgpu."
    exit 2
fi
if ! lspci -Dn -s "$BDF" >/dev/null 2>&1; then
    log "REFUSE: lspci cannot query the S30"
    exit 2
fi
existing_faults="$(journalctl -k -b --no-pager 2>/dev/null | grep -E "$FAULT_RE" || true)"
if [[ -n "$existing_faults" ]]; then
    log "REFUSE: this boot already contains a kernel fault; reboot before loading mtgpu"
    printf '%s\n' "$existing_faults" | tail -20 | tee -a "$LOGFILE"
    exit 2
fi
log "no kernel fault in this boot before load"

hdr "preflight: firmware and runtime"
FW_SRC="$INSTALLED_ROOT/usr/lib/firmware/mthreads"
FW_MOUNTED=0
if [[ -d "$FW_SRC" ]]; then
    if [[ -d /lib/firmware/mthreads ]] && [[ -n "$(ls -A /lib/firmware/mthreads 2>/dev/null)" ]]; then
        log "host /lib/firmware/mthreads already populated; using it"
    else
        [[ -d /lib/firmware/mthreads ]] || CREATED_FW_DIR=1
        mount_bind "$FW_SRC" /lib/firmware/mthreads || exit 2
        FW_MOUNTED=1
    fi
else
    log "WARNING: $FW_SRC not found; the module may fail to load firmware"
fi
[[ -x "$INSTALLED_ROOT/usr/local/musa/bin/musaInfo" ]] || { log "REFUSE: runtime musaInfo missing"; exit 2; }
[[ -x "$MUSA_SDK/bin/mcc" || -x "$INSTALLED_ROOT/$MUSA_SDK/bin/mcc" ]] || log "WARNING: mcc not found in installed root"
[[ -x "$INSTALLED_ROOT/usr/local/musa/bin/muMemcpyTest" ]] || log "WARNING: vendor muMemcpyTest missing"
for b in musart-smoke musa-vector-add musa-compute musa-fp64; do
    [[ -x "$TEST_BIN/$b" ]] || { log "REFUSE: missing test binary $TEST_BIN/$b"; exit 2; }
done
log "runtime: $(chroot "$INSTALLED_ROOT" /usr/local/musa/bin/musa_runtime_version 2>/dev/null | tr '\n' ' ' | cut -c1-120)"
log "runtime musaInfo: $INSTALLED_ROOT/usr/local/musa/bin/musaInfo"
log "compiler: $(chroot "$INSTALLED_ROOT" /usr/local/musa-5.2/bin/mcc --version 2>/dev/null | head -2 | tr '\n' ' ')"

if ((DRY_RUN)); then
    hdr "dry run complete"
    log "would run: insmod $MODULE display=none disable_fbdev=1 mtgpu_drm_major=2"
    log "would run tests from $TEST_BIN inside chroot $INSTALLED_ROOT"
    log "DRY_RUN_STATUS: PREFLIGHT_PASS"
    exit 0
fi

# Avoid the driver's harmless but noisy warning when this optional logging
# directory is absent. It is created only on the live overlay.
if [[ ! -d /var/log/mtgpu ]]; then
    if install -d -m 0755 /var/log/mtgpu 2>/dev/null; then
        CREATED_LOG_DIR=1
    fi
fi
install -d -m 0755 /var/log/mtgpu/event_report 2>/dev/null || true

# Prepare the installed root for the test binaries and GPU device nodes. The
# installed root is not running; these are namespace-only bind mounts.
mount_bind /proc "$INSTALLED_ROOT/proc" || exit 2
mount_bind /dev  "$INSTALLED_ROOT/dev"  || exit 2
mount_bind /sys  "$INSTALLED_ROOT/sys"  || exit 2
mkdir -p "$INSTALLED_ROOT/root/musa-live-test"
mount_bind "$TEST_BIN" "$INSTALLED_ROOT/root/musa-live-test" || exit 2

hdr "load (single insmod, headless args)"
timeout 180 insmod "$MODULE" display=none disable_fbdev=1 mtgpu_drm_major=2
insmod_rc=$?
log "insmod rc=$insmod_rc"
((insmod_rc == 0)) || { log "LOAD_STATUS: FAILED"; check_no_fault "insmod failure" || exit 3; exit 4; }
LOADED=1
sleep 5

hdr "post-load kernel journal (mtgpu/drm)"
journalctl -k -b --utc --since "$START_TS" --no-pager 2>/dev/null | \
    grep -iE 'mtgpu|mvgal|drm|firmware|ENDBR|IBT' | tail -60 | tee -a "$LOGFILE"
check_no_fault "driver load" || exit 3

hdr "post-load device state"
log "initstate=$(cat /sys/module/mtgpu/initstate 2>/dev/null || echo absent)"
log "refcnt=$(cat /sys/module/mtgpu/refcnt 2>/dev/null || echo unknown)"
ls -la /dev/mtgpu* 2>/dev/null | tee -a "$LOGFILE"
RENDER="$(find_s30_render || true)"
if [[ -n "$RENDER" ]]; then
    log "S30 render node: $RENDER"
else
    log "ERROR: no S30 render node found"
    rc_total=1
fi
POST_GNOME="$(pgrep -c gnome-shell 2>/dev/null || true)"
log "gnome-shell after load: ${POST_GNOME:-0} process(es); gdm: $(systemctl is-active gdm 2>/dev/null || echo unknown)"

hdr "telemetry: mthreads-gmi -q"
run_in_root "mthreads-gmi -q" /usr/bin/mthreads-gmi -q || {
    r=$?; ((r == 3)) && exit 3; rc_total=1; }

hdr "runtime discovery: musaInfo (1.5.4)"
run_in_root "musaInfo" /usr/local/musa/bin/musaInfo || {
    r=$?; ((r == 3)) && exit 3; rc_total=1; }

hdr "vendor copy test: muMemcpyTest"
run_in_root "muMemcpyTest" /usr/local/musa/bin/muMemcpyTest || {
    r=$?; ((r == 3)) && exit 3; rc_total=1; }

hdr "host runtime API: musart-smoke"
run_in_root "musart-smoke" /root/musa-live-test/musart-smoke || {
    r=$?; ((r == 3)) && exit 3; rc_total=1; }

hdr "kernel: vector-add (mp_10)"
run_in_root "musa-vector-add" /root/musa-live-test/musa-vector-add || {
    r=$?; ((r == 3)) && exit 3; rc_total=1; }

hdr "kernel: compute suite (mp_10)"
run_in_root "musa-compute" /root/musa-live-test/musa-compute || {
    r=$?; ((r == 3)) && exit 3; rc_total=1; }

hdr "kernel: fp64 (mp_10)"
run_in_root "musa-fp64" /root/musa-live-test/musa-fp64 || {
    r=$?; ((r == 3)) && exit 3; rc_total=1; }

if ((SKIP_VULKAN == 0)); then
    hdr "vulkan headless: device/buffer/map"
    if [[ -x "$TEST_BIN/vk-enum" ]]; then
        run_in_root "vk-enum" /root/musa-live-test/vk-enum || {
            r=$?; ((r == 3)) && exit 3; rc_total=1; }
    else
        log "SKIP: vk-enum not built"
    fi
fi

hdr "desktop and kernel status at end"
POST_GNOME="$(pgrep -c gnome-shell 2>/dev/null || true)"
GDM_STATE="$(systemctl is-active gdm 2>/dev/null || echo unknown)"
log "gnome-shell: ${POST_GNOME:-0}; gdm: $GDM_STATE"
log "mtgpu initstate=$(cat /sys/module/mtgpu/initstate 2>/dev/null || echo absent)"
log "mtgpu faults in this boot since load: $(faults_since_start | wc -l)"
if ((DESKTOP_WAS_ACTIVE)) && { [[ "${POST_GNOME:-0}" -eq 0 ]] || [[ "$GDM_STATE" != "active" ]]; }; then
    log "DESKTOP_STATUS: FAILED (GNOME/GDM did not survive the load)"
    rc_total=1
elif ((DESKTOP_WAS_ACTIVE)); then
    log "DESKTOP_STATUS: PASS (GNOME/GDM still active)"
else
    log "DESKTOP_STATUS: NOT_APPLICABLE (no GNOME session detected before load)"
fi
log "MODULE_LEFT_LOADED: yes (no rmmod/unbind; reboot to return to the unbound live state)"
check_no_fault "final" || exit 3

hdr "done $(date -u +%FT%TZ) overall rc=$rc_total"
if ((rc_total == 0)); then
    log "LIVE_TEST_STATUS: PASS"
else
    log "LIVE_TEST_STATUS: FAIL"
fi
exit $rc_total

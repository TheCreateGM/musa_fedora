#!/usr/bin/env bash
# compute-mode-boot-verify.sh — read-only evidence that MUSA compute mode
# survived a reboot on the MTT S30.
#
#   compute-mode-boot-verify.sh [--wait SECS] [--out-dir DIR]
#                               [--with-suite] [--systemd]
#
# What it proves (the items left open by
# docs/compute-mode-enabled-2026-11-24.md):
#   * udev auto-loaded `mtgpu` at boot (no blacklist, no manual insmod);
#   * the S30 is bound to `mtgpu` and a DRM render node exists;
#   * GDM / GNOME still reach the login screen with the driver live;
#   * no kernel fault (BUG/Oops/NULL deref) was logged this boot.
#
# It changes nothing: no modprobe, no insmod/rmmod, no unbind/rebind, no
# package or boot-config change. The only write is the evidence log.
#
# Exit codes: 0 all critical checks passed, 1 one or more failed.
set -uo pipefail

ROOT="$(cd -- "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
KVER="$(uname -r)"
BDF="0000:01:00.0"
S30_ID="1ed5:0102"
OUT_DIR="/var/log/musa-fedora"
WAIT_SECS=60
WITH_SUITE=0
SYSTEMD=0
UNIT_NAME="musa-boot-verify.service"
FAULT_RE='BUG: kernel NULL pointer dereference|Oops:|general protection fault|kernel BUG at|invalid opcode|RIP: 0010'

while (($#)); do
    case "$1" in
        --wait)       WAIT_SECS="${2:-60}"; shift 2 ;;
        --out-dir)    OUT_DIR="${2:-$OUT_DIR}"; shift 2 ;;
        --with-suite) WITH_SUITE=1; shift ;;
        --systemd)    SYSTEMD=1; shift ;;
        -h|--help)    sed -n '2,20p' "$0"; exit 0 ;;
        *) echo "usage: $0 [--wait SECS] [--out-dir DIR] [--with-suite] [--systemd]" >&2; exit 2 ;;
    esac
done

mkdir -p "$OUT_DIR" 2>/dev/null || OUT_DIR="${TMPDIR:-/tmp}"
# The write must actually work: /var/log/musa-fedora is root-owned and the
# systemd unit runs the script as root, but a manual run as a normal user
# would otherwise lose the whole log (exec >> fails, script keeps running).
if ! touch "$OUT_DIR/.boot-verify-write-test" 2>/dev/null; then
    printf '%s\n' "note: $OUT_DIR not writable, writing to ${TMPDIR:-/tmp}" >&2
    OUT_DIR="${TMPDIR:-/tmp}"
else
    rm -f "$OUT_DIR/.boot-verify-write-test"
fi
BOOT_ID="$(cat /proc/sys/kernel/random/boot_id 2>/dev/null)"
TS="$(date -u +%Y%m%dT%H%M%SZ)"
LOG="$OUT_DIR/compute-mode-boot-$TS.log"
LATEST="$OUT_DIR/compute-mode-boot-latest.log"

# Console progress on fd 3; the complete record goes to the log file.
exec 3>&1
exec >>"$LOG" 2>&1
log()  { local m; m="$(date -u +%H:%M:%SZ) $*"; printf '%s\n' "$m"; printf '%s\n' "$m" >&3; }

pass=0; fail=0; warn=0
ok()   { log "  [ OK ] $*"; pass=$((pass+1)); }
bad()  { log "  [FAIL] $*"; fail=$((fail+1)); }
wrn()  { log "  [WARN] $*"; warn=$((warn+1)); }
hdr()  { log ""; log "===== $* ====="; }

hdr "MUSA compute-mode boot verification"
log "  started      : $(date -u +%FT%TZ)"
log "  kernel       : $KVER"
log "  boot id      : $BOOT_ID"
log "  log file     : $LOG"
log "  invoked by   : $( ((SYSTEMD)) && echo systemd || echo "${USER:-$(id -un)}" )"

# ------------------------------------------------------------------ udev load
hdr "1. driver autoload (the point of the reboot)"
deadline=$(( $(date +%s) + WAIT_SECS ))
while (( $(date +%s) < deadline )); do
    [[ -r /sys/module/mtgpu/initstate ]] && break
    sleep 2
done
if [[ -r /sys/module/mtgpu/initstate ]]; then
    state="$(cat /sys/module/mtgpu/initstate)"
    log "  /sys/module/mtgpu/initstate = $state"
    [[ "$state" == "live" ]] && ok "mtgpu loaded automatically (initstate=live)" \
                             || bad "mtgpu loaded but initstate=$state"
else
    bad "mtgpu was NOT loaded within ${WAIT_SECS}s of boot (no udev autoload)"
fi

# ------------------------------------------------------------- driver binding
hdr "2. PCI binding"
if lspci -nn 2>/dev/null | grep -qi "$S30_ID"; then
    ok "S30 $S30_ID present on the PCI bus"
else
    bad "S30 $S30_ID not found on the PCI bus"
fi
drv="$(basename "$(readlink -f /sys/bus/pci/devices/$BDF/driver 2>/dev/null)" 2>/dev/null)"
if [[ "$drv" == "mtgpu" ]]; then
    ok "0000:01:00.0 bound to driver 'mtgpu'"
else
    bad "0000:01:00.0 bound to '${drv:-<none>}' (expected mtgpu)"
fi
lspci -k -s "$BDF" 2>/dev/null | sed 's/^/    /'

# --------------------------------------------------------------- device nodes
hdr "3. device nodes"
if [[ -e /dev/mtgpu.0 ]]; then
    ok "/dev/mtgpu.0 present"
    ls -l /dev/mtgpu.0 | sed 's/^/    /'
else
    bad "/dev/mtgpu.0 missing"
fi
render=""
for d in /sys/class/drm/renderD*/device; do
    [[ -e "$d" ]] || continue
    if [[ "$(basename "$(readlink -f "$d")")" == "$BDF" ]]; then
        render="$(basename "$(dirname "$d")")"
        break
    fi
done
if [[ -n "$render" ]]; then
    ok "S30 DRM render node: /dev/dri/$render"
else
    bad "no DRM render node bound to $BDF"
fi
log "  DRM card map:"
for d in /sys/class/drm/card*/device; do
    [[ -e "$d" ]] || continue
    printf '    %s -> %s\n' "$(basename "$(dirname "$d")")" "$(basename "$(readlink -f "$d")")"
done

# ------------------------------------------------------------------- session
hdr "4. desktop session (must survive the live driver)"
# The unit is ordered after display-manager.service, but "active" precedes
# the login screen and graphical.target can still be starting when gdm is
# already running; poll both so an early run is not a false failure
# (2026-11-25 boot logged "graphical.target is not active" for exactly
# this reason while GNOME was fine seconds later).
deadline=$(( $(date +%s) + WAIT_SECS ))
until systemctl is-active --quiet gdm 2>/dev/null; do
    (( $(date +%s) < deadline )) || break
    sleep 2
done
if systemctl is-active --quiet gdm 2>/dev/null; then
    ok "gdm is active"
else
    bad "gdm is not active within ${WAIT_SECS}s"
fi
until systemctl is-active --quiet graphical.target 2>/dev/null; do
    (( $(date +%s) < deadline )) || break
    sleep 2
done
if systemctl is-active --quiet graphical.target 2>/dev/null; then
    ok "graphical.target is active"
else
    bad "graphical.target is not active within ${WAIT_SECS}s"
fi
# pgrep -c prints "0" and exits 1 when nothing matches; appending "|| echo 0"
# therefore produced "0\n0" and an arithmetic error (2026-11-25 boot log).
n_shell="$(pgrep -c gnome-shell 2>/dev/null)"
n_shell="${n_shell:-0}"
n_shell="${n_shell//$'\n'/}"
if (( n_shell > 0 )); then
    ok "gnome-shell running ($n_shell process(es))"
else
    wrn "no gnome-shell process (console/rescue boot, or login-screen only)"
fi

# -------------------------------------------------------------- config state
hdr "5. compute-mode configuration"
if modprobe -c 2>/dev/null | grep -qiE '^[[:space:]]*blacklist[[:space:]]+mtgpu'; then
    bad "modprobe still blacklists mtgpu"
else
    ok "modprobe carries no blacklist for mtgpu"
fi
if grep -rlE '^[[:space:]]*blacklist[[:space:]]+mtgpu' /etc/modprobe.d/*.conf >/dev/null 2>&1; then
    bad "a file in /etc/modprobe.d blacklists mtgpu"
else
    ok "no /etc/modprobe.d file blacklists mtgpu"
fi
if [[ -f /etc/modprobe.d/90-mtgpu-headless.conf ]]; then
    ok "90-mtgpu-headless.conf present"
    sed 's/^/    /' /etc/modprobe.d/90-mtgpu-headless.conf
else
    wrn "90-mtgpu-headless.conf missing (module loaded with defaults)"
fi

# ------------------------------------------------------------------- faults
hdr "6. kernel fault scan (this boot)"
faults="$(journalctl -k -b 0 2>/dev/null | grep -iE "$FAULT_RE" | grep -i mtgpu || true)"
if [[ -n "$faults" ]]; then
    bad "mtgpu kernel fault(s) logged this boot:"
    printf '%s\n' "$faults" | sed 's/^/    /'
else
    ok "no mtgpu BUG/Oops/NULL-dereference in the kernel log this boot"
fi
if journalctl -k -b 0 2>/dev/null | grep -q 'Invalid cmd_type'; then
    bad "the current boot logged 'Invalid cmd_type' (incompatible runtime ABI)"
else
    ok "no 'Invalid cmd_type' entry this boot"
fi

# ------------------------------------------------------------------ packages
hdr "7. installed packages"
for p in musa-runtime musa-runtime-display musa-sdk musa-toolkit mtgpu-dkms mtgpu-firmware; do
    if rpm -q "$p" >/dev/null 2>&1; then
        printf '    %-22s %s\n' "$p" "$(rpm -q --qf '%{version}-%{release}' "$p")"
    else
        printf '    %-22s %s\n' "$p" "not installed"
    fi
done

# --------------------------------------------------------------- MUSA query
hdr "8. MUSA runtime discovery"
if [[ -f "$ROOT/scripts/musa-env.sh" ]]; then
    # shellcheck disable=SC1091
    source "$ROOT/scripts/musa-env.sh" >/dev/null 2>&1 || warn_reset=1
fi
if command -v musaInfo >/dev/null 2>&1; then
    log "  musaInfo: $(command -v musaInfo)"
    if out="$(timeout 30 musaInfo 2>&1)"; then
        printf '%s\n' "$out" | sed 's/^/    /'
        if printf '%s' "$out" | grep -qi 'S30'; then
            ok "musaInfo detected the MTT S30"
        else
            wrn "musaInfo ran but did not report the S30"
        fi
    else
        wrn "musaInfo exited non-zero"
    fi
else
    wrn "musaInfo not on PATH (runtime library layout?)"
fi

# -------------------------------------------------------------- optional suite
if (( WITH_SUITE )) && [[ -x "$ROOT/scripts/verify-musa.sh" ]]; then
    hdr "9. full MUSA suite (verify-musa.sh)"
    if bash "$ROOT/scripts/verify-musa.sh"; then
        ok "verify-musa.sh reported success"
    else
        bad "verify-musa.sh reported a failure (see output above)"
    fi
fi

# -------------------------------------------------------------------- summary
hdr "summary"
log "  passed: $pass   failed: $fail   warnings: $warn"
if (( fail == 0 )); then
    log "  RESULT: PASS — MUSA compute mode survived the reboot"
else
    log "  RESULT: FAIL — $fail critical check(s) failed"
    log "  rollback: pkexec $ROOT/scripts/compute-mode.sh disable && pkexec systemctl reboot"
fi

cp -f "$LOG" "$LATEST" 2>/dev/null || true

if (( SYSTEMD )); then
    # One-shot: remove the service after it has captured the evidence.
    systemctl disable "$UNIT_NAME" >/dev/null 2>&1 || true
    rm -f "/etc/systemd/system/$UNIT_NAME" 2>/dev/null || true
    systemctl daemon-reload >/dev/null 2>&1 || true
    log "  (boot-verify service disabled and removed — one-shot run)"
fi

exit $(( fail > 0 ? 1 : 0 ))

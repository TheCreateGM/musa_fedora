#!/usr/bin/env bash
# install-boot-verify.sh — install / remove the one-shot MUSA compute-mode
# boot-verification service.
#
#   install-boot-verify.sh install     install + enable the one-shot unit
#   install-boot-verify.sh uninstall   disable + remove the unit and its logs
#   install-boot-verify.sh status      report whether the unit is installed
#
# The unit runs scripts/compute-mode-boot-verify.sh once at the next boot,
# writes the evidence to /var/log/musa-fedora/, then the service disables and
# removes itself (a true one-shot). It reads state only; it never loads or
# unloads a module and never changes the boot configuration.
#
# Privileged steps use pkexec; nothing is written without it.
set -uo pipefail

ROOT="$(cd -- "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
UNIT_NAME="musa-boot-verify.service"
UNIT_SRC="$ROOT/packaging/systemd/$UNIT_NAME"
UNIT_DST="/etc/systemd/system/$UNIT_NAME"
LOG_DIR="/var/log/musa-fedora"
VERIFY_SCRIPT="$ROOT/scripts/compute-mode-boot-verify.sh"

say()  { printf '%s\n' "$*"; }
ok()   { printf '  [ OK ] %s\n' "$*"; }
err()  { printf '  [ERR ] %s\n' "$*"; }

need_root() {
    [[ "$(id -u)" -eq 0 ]] && return 0
    if command -v pkexec >/dev/null 2>&1; then
        exec pkexec "$0" "$@"
    fi
    echo "This command needs root privileges (pkexec is not available)." >&2
    exit 1
}

ACTION="${1:-status}"

case "$ACTION" in
install)
    need_root install
    [[ -f "$UNIT_SRC" ]] || { err "unit template missing: $UNIT_SRC"; exit 1; }
    [[ -x "$VERIFY_SCRIPT" ]] || { err "verify script missing/not executable: $VERIFY_SCRIPT"; exit 1; }

    # Refuse to arm the capture while a boot-time block is present: the unit
    # would be skipped by its ConditionPathExists and the run would be wasted.
    if grep -rlE '^[[:space:]]*blacklist[[:space:]]+mtgpu' /etc/modprobe.d/*.conf >/dev/null 2>&1; then
        err "mtgpu is still blacklisted in /etc/modprobe.d; enable compute mode first"
        err "  pkexec $ROOT/scripts/compute-mode.sh enable"
        exit 1
    fi

    install -m 0644 "$UNIT_SRC" "$UNIT_DST"
    systemctl daemon-reload
    systemctl enable "$UNIT_NAME" >/dev/null 2>&1
    ok "installed and enabled $UNIT_NAME"
    ok "evidence will be written to $LOG_DIR/compute-mode-boot-<timestamp>.log on the next boot"
    say ""
    say "Next: reboot when convenient. After the reboot, read the newest log:"
    say "  ls -t $LOG_DIR/compute-mode-boot-*.log | head -1"
    say "  cat $LOG_DIR/compute-mode-boot-latest.log"
    ;;

uninstall)
    need_root uninstall
    systemctl disable "$UNIT_NAME" >/dev/null 2>&1 || true
    rm -f "$UNIT_DST" 2>/dev/null || true
    systemctl daemon-reload >/dev/null 2>&1 || true
    ok "disabled and removed $UNIT_NAME"
    if [[ "${2:-}" == "--purge-logs" ]]; then
        rm -f "$LOG_DIR"/compute-mode-boot-*.log 2>/dev/null || true
        ok "removed boot-verify logs under $LOG_DIR"
    fi
    ;;

status)
    if [[ -f "$UNIT_DST" ]]; then
        ok "$UNIT_DST present"
    else
        say "  $UNIT_DST not installed"
    fi
    if systemctl is-enabled "$UNIT_NAME" >/dev/null 2>&1; then
        ok "$UNIT_NAME is enabled"
    else
        say "  $UNIT_NAME is not enabled"
    fi
    say ""
    say "Boot-verify logs:"
    ls -lt "$LOG_DIR"/compute-mode-boot-*.log 2>/dev/null | sed 's/^/  /' || say "  (none yet)"
    ;;

*)
    echo "usage: $0 {install|uninstall [--purge-logs]|status}" >&2
    exit 2
    ;;
esac

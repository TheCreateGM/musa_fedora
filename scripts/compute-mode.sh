#!/usr/bin/env bash
# compute-mode.sh — control the MTT S30 compute mode on Fedora.
#
#   compute-mode.sh status     report the current state (read-only)
#   compute-mode.sh enable     enable MUSA compute on the S30 at boot
#   compute-mode.sh disable    return to the blacklisted (display-only) state
#
# Why the blacklist exists and why it is not a driver workaround
# -------------------------------------------------------------
# The mtgpu driver registers a DRM card for the S30. When Mesa's vendor MUSA
# DRI drivers are installed, Mesa picks them up for that card and GNOME
# Shell / mutter segfaults in its KMS thread, so no login screen appears
# (docs/boot-fix-2026-11-23.md, docs/hardware-validation-2026-11-23-load-test.md).
# The *root cause* was fixed in packaging: the vendor DRI/GBM/Xorg drivers now
# live in the optional musa-runtime-display subpackage, so a compute-only
# install does not put them where Mesa can find them.
#
# This script therefore does not "work around" a broken driver. It removes the
# boot-time block so that the (fixed) stack is actually usable, and it refuses
# to do so while the conditions that caused the crash are still present:
#
#   * musa-runtime-display must NOT be installed
#   * the mtgpu module must exist for the running kernel
#   * the S30 must be present on the PCI bus
#
# Every file it touches is backed up first; `disable` restores the exact
# previous state. A reboot always returns to that state as well.
set -uo pipefail

BLACKLIST_CONF="/etc/modprobe.d/blacklist-mtgpu.conf"
HEADLESS_CONF="/etc/modprobe.d/90-mtgpu-headless.conf"
BACKUP_ROOT="/var/backups/musa-fedora-compute-mode"
S30_PCI_ID="1ed5:0102"
S30_BDF="0000:01:00.0"
KVER="$(uname -r)"
REPO_ROOT="$(cd -- "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

RC=0
say()  { printf '%s\n' "$*"; }
ok()   { printf '  [ OK ] %s\n' "$*"; }
warn() { printf '  [WARN] %s\n' "$*"; }
err()  { printf '  [ERR ] %s\n' "$*"; }

need_root() {
    [[ "$(id -u)" -eq 0 ]] && return 0
    if command -v pkexec >/dev/null 2>&1; then
        exec pkexec "$0" "$@"
    fi
    echo "This command needs root privileges (pkexec is not available)." >&2
    exit 1
}

# ---------------------------------------------------------------- helpers ---
pkg_installed() { rpm -q "$1" >/dev/null 2>&1; }
pkg_version() {
    local v=""
    # rpm prints its "not installed" message on stdout, so query only when
    # the package is known to be present.
    if pkg_installed "$1"; then
        v="$(rpm -q --qf '%{version}-%{release}' "$1" 2>/dev/null)"
    fi
    [[ -z "$v" ]] && v="not installed"
    printf '%s\n' "$v"
}

module_file() {
    local base="/lib/modules/$KVER/extra/mtgpu.ko"
    for c in "$base" "$base.xz" "$base.zst"; do
        [[ -f "$c" ]] && { echo "$c"; return 0; }
    done
    return 1
}

blacklisted() {
    grep -rlE '^[[:space:]]*blacklist[[:space:]]+mtgpu' /etc/modprobe.d/*.conf 2>/dev/null
}

dri_drivers_present() {
    ls /usr/lib64/dri/ 2>/dev/null | grep -iE 'musa|mtgpu'
}

s30_present() {
    lspci -nn 2>/dev/null | grep -qi "$S30_PCI_ID"
}

# ----------------------------------------------------------------- status ---
cmd_status() {
    say "MTT S30 compute-mode status (kernel $KVER)"
    say ""

    say "Packages"
    for p in musa-runtime musa-runtime-display musa-sdk musa-toolkit mtgpu-dkms mtgpu-firmware; do
        printf '  %-22s %s\n' "$p" "$(pkg_version $p)"
    done
    say ""

    say "Display drivers (must be absent for a compute-only host)"
    if dri_drivers_present >/dev/null; then
        dri_drivers_present | sed 's/^/    /'
        warn "vendor Mesa DRI drivers are installed; GNOME Shell crashes when mtgpu is loaded"
    else
        ok "no vendor Mesa DRI/GBM/Xorg driver installed"
    fi
    say ""

    say "Driver"
    if mf="$(module_file)"; then
        ok "module for running kernel: $mf"
    else
        err "no mtgpu module for kernel $KVER (install/refresh kernel-devel and dkms build)"
    fi
    if [[ -r /sys/module/mtgpu/initstate ]]; then
        ok "module loaded (initstate=$(cat /sys/module/mtgpu/initstate))"
    else
        say "  module not loaded"
    fi
    say ""

    say "Boot-time block"
    if bl="$(blacklisted)"; then
        for f in $bl; do
            warn "$f blacklists mtgpu"
        done
    else
        ok "no modprobe blacklist for mtgpu"
    fi
    [[ -f "$HEADLESS_CONF" ]] && ok "$HEADLESS_CONF present" || warn "$HEADLESS_CONF missing"
    say ""

    say "Hardware"
    if s30_present; then
        ok "S30 $S30_PCI_ID present at $S30_BDF"
    else
        err "S30 $S30_PCI_ID not found on the PCI bus"
    fi
    ls -l /dev/mtgpu.* 2>/dev/null | sed 's/^/    /'
    if compgen -G "/sys/class/drm/card*/device" >/dev/null; then
        for d in /sys/class/drm/card*/device; do
            [[ -e "$d" ]] || continue
            printf '    %s -> %s\n' "$(basename "$(dirname "$d")")" "$(basename "$(readlink -f "$d")")"
        done
    fi
    return 0
}

# ----------------------------------------------------------------- enable ---
cmd_enable() {
    need_root enable

    say "Enabling MUSA compute mode for the MTT S30"
    say ""

    # --- preflight ------------------------------------------------------
    if pkg_installed musa-runtime-display; then
        err "musa-runtime-display is installed."
        err "Its vendor Mesa DRI drivers make GNOME Shell crash when mtgpu loads."
        err "Remove it first: dnf remove musa-runtime-display"
        RC=1
    else
        ok "musa-runtime-display is not installed"
    fi

    if ! pkg_installed musa-runtime; then
        err "musa-runtime is not installed"; RC=1
    fi
    if ! pkg_installed mtgpu-dkms; then
        err "mtgpu-dkms is not installed"; RC=1
    fi

    if ! mf="$(module_file)"; then
        err "no mtgpu module for kernel $KVER — cannot enable at boot"; RC=1
    else
        ok "module present: $mf"
    fi

    if ! s30_present; then
        err "S30 ($S30_PCI_ID) not found on the PCI bus"; RC=1
    fi

    if [[ $RC -ne 0 ]]; then
        say ""
        err "preflight failed — nothing was changed"
        return 1
    fi

    # --- backup ---------------------------------------------------------
    local ts; ts="$(date -u +%Y%m%dT%H%M%SZ)"
    mkdir -p "$BACKUP_ROOT/$ts"
    if [[ -f "$BLACKLIST_CONF" ]]; then
        cp -a "$BLACKLIST_CONF" "$BACKUP_ROOT/$ts/blacklist-mtgpu.conf"
        ok "backed up $BLACKLIST_CONF -> $BACKUP_ROOT/$ts/"
    else
        say "  no $BLACKLIST_CONF to back up"
    fi
    [[ -f "$HEADLESS_CONF" ]] && cp -a "$HEADLESS_CONF" "$BACKUP_ROOT/$ts/90-mtgpu-headless.conf"
    cp -a /etc/modprobe.d/*.conf "$BACKUP_ROOT/$ts/modprobe.d-all/" 2>/dev/null || \
        { mkdir -p "$BACKUP_ROOT/$ts/modprobe.d-all"; cp -a /etc/modprobe.d/*.conf "$BACKUP_ROOT/$ts/modprobe.d-all/" 2>/dev/null; }
    say "  backup dir: $BACKUP_ROOT/$ts"

    # --- headless options ----------------------------------------------
    if [[ ! -f "$HEADLESS_CONF" ]]; then
        install -m 0644 "$REPO_ROOT/driver-s30-3.1.0/90-mtgpu-headless.conf" "$HEADLESS_CONF"
        ok "installed $HEADLESS_CONF"
    else
        grep -q 'display=none' "$HEADLESS_CONF" || {
            warn "$HEADLESS_CONF does not set display=none; leaving it untouched"; }
        ok "$HEADLESS_CONF already present"
    fi

    # --- remove the boot-time block ------------------------------------
    if [[ -f "$BLACKLIST_CONF" ]]; then
        mv "$BLACKLIST_CONF" "$BACKUP_ROOT/$ts/blacklist-mtgpu.conf.moved"
        ok "removed $BLACKLIST_CONF (copy kept in the backup dir)"
    fi
    # Any other *.conf that blacklists mtgpu would still block the module.
    local other
    for other in $(blacklisted); do
        [[ -f "$other" ]] || continue
        mv "$other" "$BACKUP_ROOT/$ts/$(basename "$other").moved"
        ok "removed additional blacklist $other"
    done

    # --- initramfs -----------------------------------------------------
    if command -v update-initramfs >/dev/null 2>&1; then
        if update-initramfs -u -k "$KVER" >/tmp/compute-mode-initramfs.log 2>&1; then
            ok "initramfs for $KVER regenerated (stale blacklist removed)"
        else
            warn "update-initramfs failed; see /tmp/compute-mode-initramfs.log"
            cat /tmp/compute-mode-initramfs.log | tail -5 | sed 's/^/    /'
        fi
    fi

    # --- load now ------------------------------------------------------
    if [[ -r /sys/module/mtgpu/initstate ]]; then
        ok "mtgpu already loaded; not loading again"
    else
        say ""
        say "Loading the driver now (headless parameters)…"
        if modprobe mtgpu 2>/tmp/compute-mode-modprobe.log; then
            sleep 2
            ok "mtgpu loaded (initstate=$(cat /sys/module/mtgpu/initstate 2>/dev/null))"
        else
            err "modprobe mtgpu failed:"
            sed 's/^/    /' /tmp/compute-mode-modprobe.log
            RC=1
        fi
    fi

    say ""
    cmd_status
    say ""
    say "Rollback: scripts/compute-mode.sh disable   (or restore from $BACKUP_ROOT)"
    return $RC
}

# ---------------------------------------------------------------- disable ---
cmd_disable() {
    need_root disable

    say "Disabling MUSA compute mode (restoring the boot-time block)"
    say ""

    local src
    src="$(find "$BACKUP_ROOT" -name 'blacklist-mtgpu.conf' -printf '%T@ %p\n' 2>/dev/null | sort -n | tail -1 | cut -d' ' -f2-)"
    if [[ -n "$src" ]]; then
        install -m 0644 "$src" "$BLACKLIST_CONF"
        ok "restored $BLACKLIST_CONF from $src"
    else
        cat > "$BLACKLIST_CONF" <<'EOF'
# Prevent the Moore Threads MTT S30 driver from loading.
blacklist mtgpu
EOF
        ok "wrote a fresh $BLACKLIST_CONF"
    fi

    if command -v update-initramfs >/dev/null 2>&1; then
        update-initramfs -u -k "$KVER" >/tmp/compute-mode-initramfs.log 2>&1 \
            && ok "initramfs regenerated" \
            || warn "update-initramfs failed (see /tmp/compute-mode-initramfs.log)"
    fi

    say ""
    say "The module is NOT unloaded: a live removal of the driver is not safe."
    say "Reboot to bring the system back to the blacklisted state."
    return 0
}

# ------------------------------------------------------------------- main ---
case "${1:-status}" in
    status)  cmd_status ;;
    enable)  cmd_enable ;;
    disable) cmd_disable ;;
    *) echo "usage: $0 {status|enable|disable}" >&2; exit 2 ;;
esac

#!/usr/bin/env bash
# uninstall.sh — report installed MUSA components and the safe removal path.
#
# --status and --plan are read-only. --remove delegates to DNF and removes
# only the pinned MUSA/MTGPU RPMs. It backs up module/config files and refuses
# while mtgpu is loaded; it never unloads/rebinds the device or changes boot args.
#
# Safe actions:
#   --status   print the currently installed component manifest
#   --plan     print the guarded package removal plan
#   --remove   remove installed MUSA/MTGPU RPMs through pkexec
#   --help     show usage
#
# --remove delegates to scripts/rpm-stack.sh. It refuses while mtgpu is loaded,
# backs up the module/configuration, and removes only the Fedora RPM set.
# Do not manually rmmod, unbind/rebind, or delete module files on this host.
set -euo pipefail

PROJECT_DIR="$(cd -- "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
KVER="$(uname -r)"
MOD_DIR="/lib/modules/${KVER}/extra"
RT_DIR="/usr/local/musa"
SDK_DIR="/usr/local/musa-5.2"
FW_DIR="/lib/firmware/mthreads"
MODPROBE_CONF="/etc/modprobe.d/mtgpu.conf"

have() { [[ -e "$1" ]] && echo "present" || echo "absent"; }
have_mod() {
    local base="$1"
    if [[ -e "$base" || -e "${base}.xz" || -e "${base}.zst" || -e "${base}.gz" ]]; then
        echo "present"
    else
        echo "absent"
    fi
}

status_line() {
    printf '  %-38s %s\n' "$1" "$2"
}

show_status() {
    echo "=== MUSA install state (host: $(cat /etc/os-release | sed -n 's/^PRETTY_NAME=//p' | tr -d '"'), kernel ${KVER} ==="
    echo
    echo "PCI / kernel:"
    status_line "MTT S30 (1ed5:0102)" "$(lspci -nn -d 1ed5:0102 >/dev/null 2>&1 && echo detected || echo not-found)"
    status_line "mtgpu module" "$([[ -d /sys/module/mtgpu ]] && echo "loaded ($(cat /sys/module/mtgpu/initstate 2>/dev/null || echo unknown))" || echo not-loaded)"
    status_line "mtgpu.ko (${MOD_DIR})" "$(have_mod "${MOD_DIR}/mtgpu.ko")"
    status_line "/dev/mtgpu.0" "$(have /dev/mtgpu.0)"
    rnode=""
    for r in /dev/dri/renderD*; do
        [[ -e "$r" ]] || continue
        d=$(basename "$(readlink -f "/sys/class/drm/$(basename "$r")/device/driver" 2>/dev/null)")
        [[ "$d" == mtgpu ]] && rnode="$r" && break
    done
    status_line "mtgpu render node" "${rnode:-none}"
    status_line "modprobe config" "$(have "${MODPROBE_CONF}")"
    echo
    echo "Userspace runtime (1.5.4):"
    status_line "${RT_DIR}/bin/musaInfo" "$(have "${RT_DIR}/bin/musaInfo")"
    status_line "${RT_DIR}/bin/muMemcpyTest" "$(have "${RT_DIR}/bin/muMemcpyTest")"
    status_line "${RT_DIR}/bin/mthreads-gmi" "$(have "${RT_DIR}/bin/mthreads-gmi")"
    status_line "${RT_DIR}/lib/libmusart.so.1.5.4" "$(have "${RT_DIR}/lib/libmusart.so.1.5.4")"
    status_line "${RT_DIR}/include/musa_runtime.h" "$(have "${RT_DIR}/include/musa_runtime.h")"
    echo
    echo "Userspace SDK/compiler (5.2.0):"
    status_line "${SDK_DIR}/bin/mcc" "$(have "${SDK_DIR}/bin/mcc")"
    status_line "${SDK_DIR}/include" "$(have "${SDK_DIR}/include")"
    echo
    echo "Firmware (${FW_DIR}):"
    status_line "mtfw-*.bin / musa.* fw files" "$(ls "${FW_DIR}"/mtfw-*.bin "${FW_DIR}"/musa.* 2>/dev/null | head -1 >/dev/null 2>&1 && echo present || echo absent)"
    echo
    echo "IOMMU:"
    status_line "iommu groups" "$([[ -d /sys/kernel/iommu_groups ]] && ls -A /sys/kernel/iommu_groups 2>/dev/null | wc -l || echo 0) group(s)"
    status_line "kernel cmdline" "$(grep -q 'intel_iommu=on' /proc/cmdline && echo 'intel_iommu=on iommu=pt present' || echo missing)"
    echo
    echo "UNINSTALL_STATUS: REPORT_ONLY (no system changes made by --status)"
}

show_plan() {
    echo "=== Guarded Fedora RPM removal plan ==="
    echo
    echo "Remove command: scripts/uninstall.sh --remove"
    echo "The package helper backs up module/config files under /var/backups/musa-fedora,"
    echo "then uses DNF with --no-autoremove to remove only the five stack RPMs."
    echo "It refuses while mtgpu is loaded and never unloads or rebinds the device."
    echo "IOMMU boot arguments and user data are left unchanged."
    echo
    echo "The active host has mtgpu loaded, so removal is currently refused."
    echo "Use only after a vendor-approved maintenance boot with the module unloaded."
    echo "Reinstall the same Fedora RPM set to recover."
    echo
    echo "UNINSTALL_STATUS: PLAN_ONLY"
}

case "${1:-}" in
    --help|-h)
        cat <<'EOF'
Usage: scripts/uninstall.sh [--status|--plan|--remove|--help]

  --status  Print the currently installed component manifest.
  --plan    Print what safe removal would look like (no system changes).
  --remove  Remove only the MUSA/MTGPU RPMs through DNF and pkexec.
  --help    Show this help.

Removal backs up the module/configuration and refuses while mtgpu is loaded.
It does not unload/rebind the device, edit boot arguments, or remove unrelated
packages.
EOF
        exit 0
        ;;
    --remove)
        shift
        exec "$PROJECT_DIR/scripts/rpm-stack.sh" remove "$@"
        ;;
    --status)
        show_status
        exit 0
        ;;
    --plan)
        show_status
        echo
        show_plan
        exit 0
        ;;
    "")
        echo "ERROR: removal is not approved without an installation manifest; no system changes were made." >&2
        echo "Run 'scripts/uninstall.sh --plan' for the safe removal path." >&2
        echo "Review package ownership and docs/live-host-recheck-2026-11-13.md." >&2
        exit 3
        ;;
    *)
        echo "Unknown option: $1" >&2
        echo "Run 'scripts/uninstall.sh --help' for usage." >&2
        exit 2
        ;;
esac

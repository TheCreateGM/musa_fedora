#!/usr/bin/env bash
# uninstall.sh — report installed MUSA components and the safe removal path.
#
# This script never removes packages, modules, firmware, SDK files, or
# libraries, and it never elevates privileges. Actual removal is intentionally
# disabled because this project has no approved installation manifest on the
# host (the working stack was installed manually from vendor packages).
#
# Safe actions:
#   --status   print the currently installed component manifest
#   --plan     print what safe removal would look like (no system changes)
#   --help     show usage
#
# Safe removal reference (run these yourself, with pkexec where elevation is
# required — never as part of this script). Remove in reverse dependency order:
#   1. unload mtgpu only if nothing is using the S30 (pkexec modprobe -r mtgpu)
#      — historically unsafe on this host: do NOT unbind/rebind as a routine
#      test; see docs/root-cause-rebind-2026-11-16.md
#   2. pkexec rm /lib/modules/$(uname -r)/extra/mtgpu.ko && pkexec depmod -a
#   3. pkexec rm /etc/modprobe.d/mtgpu.conf
#   4. remove /usr/local/musa and /usr/local/musa-5.2 contents, then pkexec ldconfig
#   5. remove /lib/firmware/mthreads/* (pkexec)
#   6. remove intel_iommu=on iommu=pt from grubby (pkexec) and reboot if desired
#
# Recovery: keep a backup of the working mtgpu.ko and BLS boot entry before
# removal; see docs/current-state-2026-11-17.md and INSTALL.md.
set -euo pipefail

PROJECT_DIR="$(cd -- "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
KVER="$(uname -r)"
MOD_DIR="/lib/modules/${KVER}/extra"
RT_DIR="/usr/local/musa"
SDK_DIR="/usr/local/musa-5.2"
FW_DIR="/lib/firmware/mthreads"
MODPROBE_CONF="/etc/modprobe.d/mtgpu.conf"

have() { [[ -e "$1" ]] && echo "present" || echo "absent"; }

status_line() {
    printf '  %-38s %s\n' "$1" "$2"
}

show_status() {
    echo "=== MUSA install state (host: $(cat /etc/os-release | sed -n 's/^PRETTY_NAME=//p' | tr -d '"'), kernel ${KVER} ==="
    echo
    echo "PCI / kernel:"
    status_line "MTT S30 (1ed5:0102)" "$(lspci -nn -d 1ed5:0102 >/dev/null 2>&1 && echo detected || echo not-found)"
    status_line "mtgpu module" "$([[ -d /sys/module/mtgpu ]] && echo "loaded ($(cat /sys/module/mtgpu/initstate 2>/dev/null || echo unknown))" || echo not-loaded)"
    status_line "mtgpu.ko (${MOD_DIR})" "$(have "${MOD_DIR}/mtgpu.ko")"
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
    echo "UNINSTALL_STATUS: BLOCKED (no system changes made by this script)"
}

show_plan() {
    echo "=== Safe removal plan (no system changes) ==="
    echo
    echo "WARNING: removing the driver while the S30 is in use can hang or fault the"
    echo "kernel. Stop all MUSA workloads first. Historically, unbind/rebind of this"
    echo "vendor module caused a kernel NULL dereference — do not unbind/rebind as a"
    echo "routine test. See docs/root-cause-rebind-2026-11-16.md."
    echo
    echo "1. Stop MUSA workloads and confirm nothing holds /dev/mtgpu.0 or renderD129."
    echo
    echo "2. Unload the module (only if idle; requires pkexec):"
    echo "   pkexec modprobe -r mtgpu"
    echo "   # If this fails or hangs, do not force it; reboot instead."
    echo
    echo "3. Remove the module file and module metadata (pkexec):"
    echo "   pkexec rm -f ${MOD_DIR}/mtgpu.ko"
    echo "   pkexec depmod -a ${KVER}"
    echo
    echo "4. Remove modprobe config (pkexec):"
    echo "   pkexec rm -f ${MODPROBE_CONF}"
    echo
    echo "5. Remove userspace runtime 1.5.4 (pkexec):"
    echo "   pkexec rm -rf ${RT_DIR}"
    echo "   pkexec ldconfig"
    echo
    echo "6. Remove SDK/compiler 5.2.0 (pkexec):"
    echo "   pkexec rm -rf ${SDK_DIR}"
    echo "   pkexec ldconfig"
    echo
    echo "7. Remove firmware (pkexec):"
    echo "   pkexec rm -rf ${FW_DIR}"
    echo
    echo "8. (Optional) remove IOMMU kernel args and reboot (pkexec):"
    echo "   ${PROJECT_DIR}/scripts/disable-iommu.sh   # removes intel_iommu=on iommu=pt"
    echo "   pkexec reboot"
    echo
    echo "Recovery: restore from backups of mtgpu.ko and the BLS boot entry."
    echo "See docs/current-state-2026-11-17.md and INSTALL.md."
    echo
    echo "UNINSTALL_STATUS: BLOCKED (plan only; nothing removed)"
}

case "${1:-}" in
    --help|-h)
        cat <<'EOF'
Usage: scripts/uninstall.sh [--status|--plan|--help]

Reports the installed MUSA component manifest and the safe removal plan.
This script makes no system changes and never elevates privileges.

  --status  Print the currently installed component manifest.
  --plan    Print what safe removal would look like (no system changes).
  --help    Show this help.

Actual removal is blocked because this project has no approved installation
manifest on the host (the working stack was installed manually from vendor
packages). Follow the plan manually, using pkexec where elevation is required,
and keep backups for recovery.
EOF
        exit 0
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

#!/usr/bin/env bash
# install.sh — report the current MUSA install state and the safe install path.
#
# This script never installs packages, builds/loads/replaces kernel modules,
# changes boot settings, or elevates privileges. Actual installation remains
# intentionally disabled until the S30 driver passes the kernel IBT safety gate
# and Moore Threads qualifies the driver/runtime combination for Fedora + S30.
#
# Safe actions:
#   --status   print the currently installed component manifest
#   --plan     print what a safe install would do (no system changes)
#   --help     show usage
#
# Manual install reference (run these yourself, with pkexec where elevation is
# required — never as part of this script):
#   1. scripts/enable-iommu.sh (grubby: intel_iommu=on iommu=pt) then reboot
#   2. extract musa_3.1.0-pc_x86.deb and copy userspace to /usr/local/musa,
#      then ldconfig + sysctl --system (vm.max_map_count=0x7fffffff)
#   3. install mtgpu.ko into /lib/modules/$(uname -r)/extra/ and run depmod
#   4. write /etc/modprobe.d/mtgpu.conf with `options mtgpu mtgpu_drm_major=2`
#   5. scripts/verify-musa.sh
#
# Recovery: keep a backup of the working mtgpu.ko and BLS boot entry before any
# change; see docs/current-state-2026-11-17.md and INSTALL.md.
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
    echo "INSTALL_STATUS: BLOCKED (no system changes made by this script)"
}

show_plan() {
    echo "=== Safe install plan (no system changes) ==="
    echo
    echo "Prerequisite (host config, requires pkexec/reboot):"
    echo "  pkexec /usr/bin/bash ${PROJECT_DIR}/scripts/enable-iommu.sh   # intel_iommu=on iommu=pt"
    echo "  pkexec /usr/bin/bash -c 'grubby --update-kernel=ALL --args=\"intel_iommu=on iommu=pt\"'  # or use enable-iommu.sh"
    echo "  pkexec reboot"
    echo
    echo "Userspace runtime 1.5.4 (extract vendor deb, do NOT run maintainer scripts):"
    echo "  dpkg-deb -x musa_3.1.0-pc_x86.deb /tmp/musa-pkg"
    echo "  pkexec cp -a /tmp/musa-pkg/usr/local/musa/lib/.   /usr/local/musa/lib/"
    echo "  pkexec cp -a /tmp/musa-pkg/usr/local/musa/include/. /usr/local/musa/include/"
    echo "  pkexec cp -a /tmp/musa-pkg/usr/local/musa/bin/.   /usr/local/musa/bin/"
    echo "  pkexec cp -a /tmp/musa-pkg/usr/lib/x86_64-linux-gnu/. /usr/lib64/"
    echo "  pkexec cp -a /tmp/musa-pkg/etc/sysctl.d/99-musa.conf /etc/sysctl.d/"
    echo "  pkexec ldconfig"
    echo "  pkexec sysctl --system            # vm.max_map_count=0x7fffffff"
    echo
    echo "Kernel driver (mtgpu 3.1.0):"
    echo "  ${PROJECT_DIR}/scripts/build-kernel-driver.sh ${KVER}   # build only, output in musa-data"
    echo "  pkexec cp <built mtgpu.ko> ${MOD_DIR}/mtgpu.ko"
    echo "  pkexec depmod -a ${KVER}"
    echo "  printf 'options mtgpu mtgpu_drm_major=2\\n' | pkexec tee ${MODPROBE_CONF}"
    echo
    echo "Verify:"
    echo "  ${PROJECT_DIR}/scripts/verify-musa.sh"
    echo
    echo "Recovery: back up ${MOD_DIR}/mtgpu.ko and the active BLS boot entry before"
    echo "any change. See docs/current-state-2026-11-17.md and INSTALL.md."
    echo
    echo "INSTALL_STATUS: BLOCKED (plan only; nothing installed)"
}

case "${1:-}" in
    --help|-h)
        cat <<'EOF'
Usage: scripts/install.sh [--status|--plan|--help]

Reports the installed MUSA component manifest and the safe install plan.
This script makes no system changes and never elevates privileges.

  --status  Print the currently installed component manifest.
  --plan    Print what a safe install would do (no system changes).
  --help    Show this help.

Actual installation is blocked until the S30 driver passes the kernel IBT
safety gate and the vendor qualifies the driver/runtime combination for
Fedora 45 + MTT S30. Follow the plan manually, using pkexec where elevation
is required, and keep backups for recovery.
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
        echo "ERROR: installation is not approved; no system changes were made." >&2
        echo "Run 'scripts/install.sh --plan' for the safe install path." >&2
        echo "See docs/live-host-recheck-2026-11-13.md and docs/limitations.md." >&2
        exit 3
        ;;
    *)
        echo "Unknown option: $1" >&2
        echo "Run 'scripts/install.sh --help' for usage." >&2
        exit 2
        ;;
esac

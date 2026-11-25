#!/usr/bin/env bash
# install.sh — report the current MUSA install state and the safe install path.
#
# The status and plan actions are read-only. --install delegates to the
# package-managed RPM workflow, which verifies package hashes, makes backups,
# and refuses a DNF/DKMS transaction while mtgpu is loaded.
#
# Safe actions:
#   --status   print the currently installed component manifest
#   --plan     print the guarded package installation plan
#   --install  install the validated local Fedora RPM set through pkexec
#   --help     show usage
#
# Driver replacement/removal and boot-argument changes require a reviewed
# maintenance window; see INSTALL.md for the package-managed workflow/recovery.
set -euo pipefail

PROJECT_DIR="$(cd -- "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
KVER="$(uname -r)"
MOD_DIR="/lib/modules/${KVER}/extra"
RT_DIR="/usr/local/musa"
SDK_DIR="/usr/local/musa-5.2"
FW_DIR="/lib/firmware/mthreads"
MODPROBE_CONF="/etc/modprobe.d/90-mtgpu-headless.conf"

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
    echo "INSTALL_STATUS: REPORT_ONLY (no system changes made by --status)"
}

show_plan() {
    echo "=== Guarded Fedora RPM install plan ==="
    echo
    echo "Build/package command: TMPDIR=/var/tmp packaging/rpm/build-all.sh"
    echo "Install command: scripts/install.sh --install"
    echo "The package helper verifies the exact six RPM versions and their digests."
    echo "It backs up existing module/config files under /var/backups/musa-fedora,"
    echo "uses pkexec with DNF, and refuses if mtgpu is loaded."
    echo "It does not load/unload/rebind a module or edit boot arguments."
    echo
    echo "The host currently has the validated RPM set installed and mtgpu live;"
    echo "the install action will report ALREADY_INSTALLED without a transaction."
    echo "If IOMMU is missing on another host, scripts/enable-iommu.sh uses pkexec"
    echo "and backs up the active BLS entries before changing the boot arguments."
    echo
    echo "INSTALL_STATUS: PLAN_ONLY"
}

case "${1:-}" in
    --help|-h)
        cat <<'EOF'
Usage: scripts/install.sh [--status|--plan|--install] [--rpm-dir DIR] [--yes]

  --status       Report the current component manifest.
  --plan         Show the validated RPM installation plan.
  --install      Install the five pinned RPMs with DNF through pkexec.
  --rpm-dir DIR  Use a local RPM tree (default: packaging/rpm/local-build/RPMS).
  --yes          Skip the interactive prompt after reviewing the plan.

The helper backs up the current module and MUSA configuration. It refuses any
DKMS transaction while mtgpu is loaded and never loads a module or edits boot
arguments.
EOF
        exit 0
        ;;
    --install)
        shift
        exec "$PROJECT_DIR/scripts/rpm-stack.sh" install "$@"
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

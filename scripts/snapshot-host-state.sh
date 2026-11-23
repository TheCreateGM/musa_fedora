#!/usr/bin/env bash
# Capture a reversible, read-only snapshot of the host state that matters for
# an MTT S30 / mtgpu driver operation.
#
# Nothing in this script changes the system. It only reads state and copies
# configuration files into a timestamped directory, so that a later operation
# (module load, package install, modprobe edit) has an exact reference to
# restore from.
#
# Usage:
#   ./scripts/snapshot-host-state.sh [--label <name>] [--dest <dir>]
#
# Default destination: <musa-data>/host-backups/pre-<label>-<YYYYmmdd-HHMMSS>
# The backup root can be overridden with MUSA_BACKUP_ROOT.
set -uo pipefail

LABEL="host-state"
DEST=""
while (($#)); do
    case "$1" in
        --label) LABEL="$2"; shift 2 ;;
        --dest)  DEST="$2";  shift 2 ;;
        -h|--help)
            sed -n '2,18p' "${BASH_SOURCE[0]}"
            exit 0 ;;
        *) echo "Unknown option: $1" >&2; exit 2 ;;
    esac
done

ROOT="$(cd -- "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BACKUP_ROOT="${MUSA_BACKUP_ROOT:-$(cd "$ROOT/.." 2>/dev/null && pwd)/musa-data/host-backups}"

if [[ -z "$DEST" ]]; then
    DEST="$BACKUP_ROOT/pre-${LABEL}-$(date +%Y%m%d-%H%M%S)"
fi
mkdir -p "$DEST" || { echo "ERROR: cannot create $DEST" >&2; exit 1; }
DEST="$(cd "$DEST" && pwd)"
echo "Snapshot directory: $DEST"

write() { # write <file> <shell-snippet>
    local out="$1"; shift
    { eval "$@" ; } >"$DEST/$out" 2>&1 || true
}

# --- kernel / boot -------------------------------------------------------
write cmdline.txt            'cat /proc/cmdline'
write uname.txt              'uname -a'
write default-kernel.txt     'grub2-editenv list 2>/dev/null'
write rescue-kernel.txt      'ls -la /boot/*rescue* /boot/initramfs-*rescue* 2>&1'
write boot-list.txt          'ls -la /boot'
write kernel-config-ibt.txt  'grep -E "CONFIG_X86_KERNEL_IBT|CONFIG_X86_KERNEL_CET|CONFIG_RETPOLINE" /boot/config-$(uname -r)'
write selinux.txt            'getenforce 2>/dev/null; sestatus 2>/dev/null | head -5'
write secureboot.txt         'mokutil --sb-state 2>&1 | head -3'

# --- journal -------------------------------------------------------------
write journal-kernel-boot.txt 'journalctl -k -b --no-pager'
write journal-boot-summary.txt 'journalctl -b --no-pager --quiet | tail -200'

# --- packages / modules --------------------------------------------------
write rpm-qa.txt             'rpm -qa'
write dkms-status.txt        'dkms status'
write modprobe-d.txt         'for f in /etc/modprobe.d/*; do echo "### $f"; cat "$f"; done'
write module-sha256.txt      'sha256sum /lib/modules/$(uname -r)/extra/mtgpu.ko* 2>&1'
write vendor-core-sha256.txt 'sha256sum /usr/src/mtgpu-*/objs/x86_64/mtgpu_core.o_binary 2>&1'

# --- PCI / devices -------------------------------------------------------
write lspci.txt              'lspci -nnk'
write lspci-s30.txt          'lspci -nnk -s 01:00.0'
write iommu-group.txt        'readlink -f /sys/bus/pci/devices/0000:01:00.0/iommu_group 2>&1'
write iommu-groups.txt       'ls -l /sys/kernel/iommu_groups/ 2>&1'
write drm-nodes.txt          'ls -la /dev/dri 2>&1; ls -la /dev/mtgpu* 2>&1'
write display-session.txt    'loginctl list-sessions 2>&1; systemctl is-active display-manager 2>&1'

# --- configuration backups (copies, not moves) ----------------------------
mkdir -p "$DEST/modprobe.d-backup"
cp -a /etc/modprobe.d/. "$DEST/modprobe.d-backup/" 2>/dev/null || true
mkdir -p "$DEST/udev-rules-backup"
cp -a /etc/udev/rules.d/70-mtgpu.rules "$DEST/udev-rules-backup/" 2>/dev/null || true

# --- MUSA tree ------------------------------------------------------------
write musa-paths.txt        'ls -la /usr/local/musa /usr/local/musa-5.2 2>&1'
write musa-firmware.txt      'ls -la /usr/lib/firmware/mthreads /lib/firmware/mthreads 2>&1'

echo "Snapshot complete: $DEST"

#!/usr/bin/env bash
# enable-iommu.sh — enable the IOMMU for the running kernel.
#
# WHY THIS IS REQUIRED
# -------------------
# The MUSA userspace runtime (libmusa/libmusart 1.5.4) refuses to initialise an
# MTT S30 unless the GPU can be guaranteed safe access to system memory:
#
#   mu-ini: numa awareness is unsupported on the current system
#   mu-ERR: There is no guarantee that the device has capability to access
#           system memory please enable IOMMU or decrease sysram to 512G
#   mu-ERR: Unable to initialize device, id: 0
#   muInit ret=801 (MUSA_ERROR_NOT_SUPPORTED)
#
# On this Fedora kernel CONFIG_INTEL_IOMMU=y but
# CONFIG_INTEL_IOMMU_DEFAULT_ON is *not* set, so there are no IOMMU groups and
# MUSA refuses the device.  The fix is to boot with intel_iommu=on iommu=pt.
#
# This script changes only the kernel command line of the target kernel via
# grubby (BLS).  It is fully reversible with disable-iommu.sh.
set -euo pipefail

KVER="${KVER:-$(uname -r)}"
KERNEL="/boot/vmlinuz-${KVER}"
ARGS='intel_iommu=on iommu=pt'

if [[ ! -e "$KERNEL" ]]; then
    echo "error: $KERNEL not found" >&2
    exit 1
fi

echo "== current boot args for $KVER =="
sudo_cmd() { if [[ $EUID -eq 0 ]]; then "$@"; else pkexec "$@"; fi; }
sudo_cmd grubby --info="$KERNEL" | grep -E '^(args|title)=' || true

echo
echo "== backing up BLS entries =="
BK="${MUSA_BACKUP_DIR:-/var/backups/musa-fedora}/bls-$(date +%Y%m%d-%H%M%S)"
sudo_cmd mkdir -p "$BK"
sudo_cmd cp -a /boot/loader/entries/. "$BK"/
echo "backup: $BK"

echo
echo "== adding: $ARGS =="
sudo_cmd grubby --update-kernel="$KERNEL" --args="$ARGS"

echo
echo "== result =="
sudo_cmd grubby --info="$KERNEL" | grep -E '^(args|title)='

cat <<EOF

IOMMU boot arguments added to $KVER.
A REBOOT is required for them to take effect.

After reboot verify with:
  scripts/verify-musa.sh
  dmesg | grep -i 'DMAR: IOMMU enabled'
  ls /sys/kernel/iommu_groups | head

To revert before reboot:
  scripts/disable-iommu.sh
To revert the boot entry from the backup:
  cp -a $BK/. /boot/loader/entries/
EOF

#!/usr/bin/env bash
# disable-iommu.sh — remove the MUSA IOMMU boot arguments again.
set -euo pipefail

KVER="${KVER:-$(uname -r)}"
KERNEL="/boot/vmlinuz-${KVER}"

sudo_cmd() { if [[ $EUID -eq 0 ]]; then "$@"; else pkexec "$@"; fi; }

echo "== before =="
sudo_cmd grubby --info="$KERNEL" | grep -E '^args=' || true

sudo_cmd grubby --update-kernel="$KERNEL" --remove-args="intel_iommu=on,iommu=pt"

echo "== after =="
sudo_cmd grubby --info="$KERNEL" | grep -E '^args=' || true
echo "Reboot required for the change to take effect."

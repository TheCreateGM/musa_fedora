#!/usr/bin/env bash
# Read-only validation of the installed mtgpu driver and S30 device.
# Does not load, unload, or open the GPU. Safe to run at any time.
set -u

KERNEL="$(uname -r)"
RC=0

hr() { printf '=== %s ===\n' "$1"; }

hr "Host / kernel"
echo "kernel:     $KERNEL"
echo "os:         $(. /etc/os-release && echo "$PRETTY_NAME")"
echo "ibt:        $(grep -qw 'ibt=off' /proc/cmdline && echo DISABLED || echo enabled)"

hr "Module state"
if [[ -r "/sys/module/mtgpu/initstate" ]]; then
    echo "initstate:  $(cat /sys/module/mtgpu/initstate)"
    echo "refcnt:     $(cat /sys/module/mtgpu/refcnt 2>/dev/null || echo n/a)"
    echo "taint:      $(cat /sys/module/mtgpu/taint 2>/dev/null || echo n/a)"
else
    echo "mtgpu module is not loaded"
fi

hr "PCI S30"
lspci -nnk -s 01:00.0 2>/dev/null || echo "lspci unavailable"
DRV="$(readlink -f /sys/bus/pci/devices/0000:01:00.0/driver 2>/dev/null || true)"
if [[ -n "$DRV" ]]; then
    echo "driver:     $DRV"
else
    echo "driver:     <unbound>"
    RC=1
fi

hr "DRM nodes"
ls -l /dev/dri/ 2>/dev/null || echo "no /dev/dri"
for c in /sys/class/drm/card*; do
    [[ -e "$c" ]] || continue
    echo "$(basename "$c") -> $(readlink -f "$c/device" 2>/dev/null)"
done

hr "MTGPU character nodes"
ls -l /dev/mtgpu* 2>/dev/null || echo "no /dev/mtgpu* nodes"

hr "Kernel log (mtgpu/firmware)"
if command -v journalctl >/dev/null 2>&1; then
    journalctl -k -b --no-pager 2>/dev/null | \
        grep -iE 'mtgpu|mtfw|mtvpu|musa|1ed5' | tail -120 || true
else
    dmesg 2>/dev/null | grep -iE 'mtgpu|mtfw|mtvpu|musa|1ed5' | tail -120 || \
        echo "kernel log not readable without privileges"
fi

hr "Runtime"
if command -v musaInfo >/dev/null 2>&1; then
    timeout 20 musaInfo 2>&1 | head -40 || true
elif [[ -x /usr/local/musa/bin/musaInfo ]]; then
    timeout 20 /usr/local/musa/bin/musaInfo 2>&1 | head -40 || true
else
    echo "musaInfo not found"
fi

echo
echo "VALIDATE_RC=$RC"
exit "$RC"
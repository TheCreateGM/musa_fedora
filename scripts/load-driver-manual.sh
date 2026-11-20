#!/usr/bin/env bash
# Manually load the installed 3.1.0 mtgpu driver for controlled testing.
# Autoload is intentionally blacklisted; this script is the only loader.
set -euo pipefail

KERNEL="$(uname -r)"
# Fedora DKMS installs the module compressed (mtgpu.ko.xz); older trees
# carried an uncompressed mtgpu.ko. The kernel has
# CONFIG_MODULE_DECOMPRESS=y, so insmod accepts the compressed file directly.
MODULE=""
for candidate in \
    "/lib/modules/${KERNEL}/extra/mtgpu.ko" \
    "/lib/modules/${KERNEL}/extra/mtgpu.ko.xz" \
    "/lib/modules/${KERNEL}/extra/mtgpu.ko.zst"; do
    if [[ -f "$candidate" ]]; then
        MODULE="$candidate"
        break
    fi
done

if [[ -z "$MODULE" ]]; then
    echo "ERROR: no mtgpu module found for kernel $KERNEL" >&2
    exit 1
fi

if [[ -r /sys/module/mtgpu/initstate ]]; then
    echo "mtgpu is already loaded (initstate=$(cat /sys/module/mtgpu/initstate))"
    echo "Reboot before retrying; a stuck module cannot be replaced in place."
    exit 1
fi

echo "Loading $MODULE on kernel $KERNEL ..."
# Extra arguments are appended verbatim, e.g.:
#   ./scripts/load-driver-manual.sh mtgpu_sched_mode=0
timeout 180 pkexec /usr/sbin/insmod "$MODULE" \
    display=none disable_fbdev=1 mtgpu_drm_major=2 "$@"

rc=$?
echo "insmod rc=$rc"
sleep 2
if [[ -r /sys/module/mtgpu/initstate ]]; then
    echo "initstate=$(cat /sys/module/mtgpu/initstate)"
    echo "refcnt=$(cat /sys/module/mtgpu/refcnt 2>/dev/null || echo n/a)"
else
    echo "mtgpu did not load"
fi
exit "$rc"
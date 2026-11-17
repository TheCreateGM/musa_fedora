#!/usr/bin/env bash
# Manually load the installed 3.1.0 mtgpu driver for controlled testing.
# Autoload is intentionally blacklisted; this script is the only loader.
set -euo pipefail

KERNEL="$(uname -r)"
MODULE="/lib/modules/${KERNEL}/extra/mtgpu.ko"

if [[ ! -f "$MODULE" ]]; then
    echo "ERROR: module not found: $MODULE" >&2
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
    mtgpu_drm_major=2 rm_feature_level=1 "$@"

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
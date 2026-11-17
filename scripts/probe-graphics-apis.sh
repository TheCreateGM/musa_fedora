#!/usr/bin/env bash
# Probe the installed Vulkan ICD and the system OpenCL loader.
# Does not change the display, the driver, or the package database.
set -uo pipefail

ROOT="$(cd -- "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ICD="${MUSA_VK_ICD:-/etc/vulkan/icd.d/musaicdconf.json}"
WORK="$(mktemp -d "${TMPDIR:-/tmp}/musa-gfx.XXXXXX")"
trap 'rm -rf "$WORK"' EXIT

fail=0

printf '== Vulkan ICD ==\n'
if [[ ! -f "$ICD" ]]; then
    printf 'VULKAN_ENUM: FAIL missing %s\n' "$ICD"
    fail=1
elif ! gcc -O2 -Wall "$ROOT/tests/vulkan-enumerate.c" -ldl -o "$WORK/vk-enum" 2>"$WORK/vk-cc"; then
    printf 'VULKAN_ENUM: FAIL compile\n'
    cat "$WORK/vk-cc"
    fail=1
else
    if ! VK_DRIVER_FILES="$ICD" VK_ICD_FILENAMES="$ICD" \
        timeout 30 "$WORK/vk-enum"; then
        fail=1
    fi
fi

printf '\n== OpenCL platforms ==\n'
if [[ ! -e /usr/include/CL/cl.h ]] || [[ ! -e /usr/lib64/libOpenCL.so.1 ]]; then
    printf 'OPENCL_MT: SKIP headers or libOpenCL missing\n'
elif ! gcc -O2 -Wall "$ROOT/tests/opencl-platforms.c" \
        /usr/lib64/libOpenCL.so.1 -o "$WORK/cl-plat" 2>"$WORK/cl-cc"; then
    printf 'OPENCL_MT: FAIL compile\n'
    cat "$WORK/cl-cc"
    fail=1
else
    timeout 30 "$WORK/cl-plat" || fail=1
fi

if ((fail)); then
    printf '\nPROBE_GRAPHICS_STATUS: FAIL\n'
    exit 1
fi
printf '\nPROBE_GRAPHICS_STATUS: DONE\n'
exit 0

#!/usr/bin/env bash
# Build the official MUSA 5.2.0 mtgpu server driver (ABI-matched vendor core)
# for the MTT S30 on Fedora, in an isolated musa-data build directory.
#
# The driver tree lives in musa-fedora/driver-mtgpu-5.2.0; the prebuilt
# conftest feature-probe binary (regenerated for kernel 7.3.3 by
# scripts/build-kernel-driver.sh) and its conftest.h results are reused so
# the opaque vendor core sees a consistent feature environment.
#
# Usage: scripts/build-mtgpu-5.2.0.sh [kernel-version]
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
DATA_DIR="/home/axogm/Documents/project/musa-data"

KERNEL_VERSION="${1:-$(uname -r)}"
ARCH="$(uname -m)"
JOBS="${JOBS:-$(nproc)}"
DRIVER_DIR="${PROJECT_DIR}/driver-mtgpu-5.2.0"

KERNEL_BUILD="/lib/modules/${KERNEL_VERSION}/build"
if [[ ! -d "$KERNEL_BUILD" ]]; then
    echo "ERROR: kernel build headers not found at $KERNEL_BUILD" >&2
    exit 1
fi

CONFTEST_SRC="${PROJECT_DIR}/conftest"
CONFTEST_H_SRC="${PROJECT_DIR}/inc/conftest.h"
if [[ ! -x "$CONFTEST_SRC" || ! -f "$CONFTEST_H_SRC" ]]; then
    echo "ERROR: prebuilt conftest binary/header missing under $PROJECT_DIR" >&2
    exit 1
fi

TS="$(date +%Y%m%d-%H%M%S)"
BUILD_DIR="${DATA_DIR}/research/builds/mtgpu-5.2.0-server-${KERNEL_VERSION}-${TS}"
mkdir -p "$BUILD_DIR"

echo "=== mtgpu 5.2.0 server build for kernel ${KERNEL_VERSION} ==="
echo "Driver tree: $DRIVER_DIR"
echo "Build dir:   $BUILD_DIR"

rsync -a --delete --exclude '.git' "$DRIVER_DIR/" "${BUILD_DIR}/source/"
cp -p "$CONFTEST_SRC" "${BUILD_DIR}/source/conftest"
cp -p "$CONFTEST_H_SRC" "${BUILD_DIR}/source/inc/conftest.h"

cd "${BUILD_DIR}/source"
if make ARCH="$ARCH" KERNELVER="$KERNEL_VERSION" -j"$JOBS" \
        > "${BUILD_DIR}/build.log" 2>&1; then
    echo "BUILD_STATUS: SUCCESS"
else
    echo "BUILD_STATUS: FAILED (see ${BUILD_DIR}/build.log)"
    grep -E 'error:' "${BUILD_DIR}/build.log" | sed 's|.*/source/||' | \
        sort -u | head -30 || true
    exit 1
fi

if [[ -f mtgpu.ko ]]; then
    echo "Module: ${BUILD_DIR}/source/mtgpu.ko"
    ls -lh mtgpu.ko
    modinfo mtgpu.ko | grep -E 'vermagic|build_version' || true
else
    echo "BUILD_STATUS: FAILED (mtgpu.ko not produced)" >&2
    exit 1
fi

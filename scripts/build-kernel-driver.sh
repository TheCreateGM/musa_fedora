#!/usr/bin/env bash
# Build the MUSA mtgpu kernel module for MTT S30 on Fedora Linux.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
PROJECT_ROOT="$(dirname "$PROJECT_DIR")"
DATA_DIR="$PROJECT_ROOT/musa-data"
KERNEL_VERSION="${1:-$(uname -r)}"
ARCH="$(uname -m)"
KERNEL_BUILD="/lib/modules/${KERNEL_VERSION}/build"
JOBS="${JOBS:-$(nproc)}"
CONFTEST_SRC="$PROJECT_DIR/conftest"
[[ -f "$CONFTEST_SRC" ]] || CONFTEST_SRC="$PROJECT_DIR/driver-s30-3.1.0/conftest"
# The vendor core object is required to link the driver. Prefer the in-repo
# copy so a plain clone of musa-fedora can build; fall back to the musa-data
# reference tree.
VENDOR_CORE="$PROJECT_DIR/driver-s30-3.1.0/objs/$ARCH/mtgpu_core.o_binary"
[[ -f "$VENDOR_CORE" ]] || \
    VENDOR_CORE="$PROJECT_ROOT/musa-data/research/builds/mtgpu-3.1.0-test/objs/$ARCH/mtgpu_core.o_binary"
VENDOR_CORE_SHA256="d0fdaceab37bce7387d376f8a87a2e9dbf210eeccf4f4f7c67f8dee83f4e8200"
# Build logs default to the musa-data reference tree when present; otherwise
# they stay inside this repository (build/ is git-ignored).
if [[ -d "$PROJECT_ROOT/musa-data" ]]; then
    DATA_DIR="$PROJECT_ROOT/musa-data"
else
    DATA_DIR="$PROJECT_DIR/build"
fi

echo "=== MUSA Kernel Driver Build for MTT S30 ==="
echo "Project directory: $PROJECT_DIR"
echo "Research/build directory: $DATA_DIR"
echo "Kernel version:    $KERNEL_VERSION"
echo "Architecture:      $ARCH"
echo "Parallel jobs:     $JOBS"

if [[ ! -d "$KERNEL_BUILD" ]]; then
    echo "ERROR: Kernel build headers not found at $KERNEL_BUILD" >&2
    echo "Please install kernel-devel for $KERNEL_VERSION" >&2
    exit 1
fi

if [[ ! -x "$CONFTEST_SRC" ]]; then
    echo "ERROR: conftest helper not found: $CONFTEST_SRC" >&2
    exit 1
fi

if [[ ! -f "$VENDOR_CORE" ]]; then
    echo "ERROR: Proprietary core object not found: $VENDOR_CORE" >&2
    exit 1
fi
actual_core_sha256="$(sha256sum "$VENDOR_CORE" | awk '{print $1}')"
if [[ "$actual_core_sha256" != "$VENDOR_CORE_SHA256" ]]; then
    echo "ERROR: vendor core hash mismatch: $actual_core_sha256" >&2
    exit 1
fi

BUILD_ROOT="$DATA_DIR/research/builds"
mkdir -p "$BUILD_ROOT"
BUILD_DIR="$(mktemp -d "$BUILD_ROOT/mtgpu-${KERNEL_VERSION}-XXXXXX")"
SOURCE_DIR="$BUILD_DIR/source"
mkdir -p "$SOURCE_DIR/inc" "$SOURCE_DIR/src" "$SOURCE_DIR/objs/$ARCH"

cp -p "$PROJECT_DIR/Makefile" "$CONFTEST_SRC" "$SOURCE_DIR/"
mkdir -p "$SOURCE_DIR/scripts"
cp -p "$PROJECT_DIR/driver-s30-3.1.0/scripts/objtool-safe.sh" "$SOURCE_DIR/scripts/"
rsync -a --exclude='/conftest.h' "$PROJECT_DIR/inc/" "$SOURCE_DIR/inc/"
rsync -a "$PROJECT_DIR/src/" "$SOURCE_DIR/src/"
cp -p "$VENDOR_CORE" "$SOURCE_DIR/objs/$ARCH/mtgpu_core.o_binary"

if [[ -e "$SOURCE_DIR/inc/conftest.h" ]]; then
    echo "ERROR: stale conftest header in isolated source copy" >&2
    exit 1
fi

echo "Isolated source:   $SOURCE_DIR"
echo "Building mtgpu module without installing or loading it..."
cd "$SOURCE_DIR"
if make ARCH="$ARCH" KERNELVER="$KERNEL_VERSION" -j"$JOBS" all \
        >"$BUILD_DIR/build.log" 2>&1; then
    echo "Build log:         $BUILD_DIR/build.log"
else
    echo "ERROR: kernel module build failed; see $BUILD_DIR/build.log" >&2
    grep -E 'error:' "$BUILD_DIR/build.log" | sed "s|$SOURCE_DIR/||g" | \
        sort -u | head -40 || true
    exit 1
fi

MODULE="$SOURCE_DIR/mtgpu.ko"
if [[ ! -f "$MODULE" ]]; then
    echo "ERROR: mtgpu.ko was not generated; see $BUILD_DIR/build.log" >&2
    exit 1
fi
VERMAGIC="$(modinfo -F vermagic "$MODULE")"
if [[ "${VERMAGIC%% *}" != "$KERNEL_VERSION" ]]; then
    echo "ERROR: module vermagic does not match requested kernel: $VERMAGIC" >&2
    exit 1
fi
ALIASES="$(modinfo -F alias "$MODULE")"
if ! grep -Fq 'pci:v00001ED5d00000102' <<<"$ALIASES"; then
    echo "ERROR: module does not advertise the MTT S30 PCI ID" >&2
    exit 1
fi
WARNING_COUNT="$(grep -c 'warning:' "$BUILD_DIR/build.log" || true)"

echo
echo "=== Build Successful ==="
ls -lh "$MODULE"
modinfo "$MODULE" | sed -n '1,25p'
sha256sum "$MODULE"
echo "Verified vermagic: $VERMAGIC"
echo "Objtool/compiler warning lines: $WARNING_COUNT (inspect $BUILD_DIR/build.log)"
echo "BUILD_STATUS: SUCCESS"
echo "INSTALL_STATUS: NOT PERFORMED"
echo "LOAD_STATUS: NOT PERFORMED"

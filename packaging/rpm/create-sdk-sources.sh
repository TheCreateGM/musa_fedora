#!/usr/bin/env bash
# create-sdk-sources.sh — regenerate SOURCES/musa-sdk-5.2.0.tar.gz from the
# vendor MUSA 5.2.0 SDK debs in the musa-data reference repository.
#
# The tarball is the compiler-only subset of the 5.2.0 SDK (mcc, mtxas, clang,
# LLVM support libraries, clang resource headers, MUSA device headers and
# bitcode). It deliberately excludes:
#   - the 5.2.0 musart runtime libraries (incompatible with the mtgpu 3.1.0
#     driver; the S30 ships musa-runtime 1.5.4 instead),
#   - development-only tooling (clang-tidy, scan-build drivers, LLVM static
#     archives, cmake configs) that is not needed to compile MUSA kernels.
#
# Inputs (all in $VENDOR_5_2_DIR, default from musa-data):
#   mtcc-5-2_5.2.0_amd64.deb          compiler + headers + libexec + share
#   mtxas-5-2_5.2.0_amd64.deb         assembler
#   musa-musart-5-2_5.2.0_amd64.deb   libmusart.so (link target only)
#   musa-musart-dev-5-2_5.2.0_amd64.deb  MUSA/CRT device headers
#
# Output: SOURCES/musa-sdk-5.2.0.tar.gz (subset of the vendor files,
# unmodified content, layout mapped to /usr/local/musa-5.2 in the spec).
set -euo pipefail

RPM_DIR="$(cd -- "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd -- "$RPM_DIR/../.." && pwd)"
VENDOR_5_2_DIR="${VENDOR_5_2_DIR:-$PROJECT_DIR/../musa-data/research/official-musa-5.2.0/packages}"
WORK="$(mktemp -d "${TMPDIR:-/tmp}/musa-sdk-sources.XXXXXX")"
trap 'rm -rf "$WORK"' EXIT

DEBS=(
    mtcc-5-2_5.2.0_amd64.deb
    mtxas-5-2_5.2.0_amd64.deb
    musa-musart-5-2_5.2.0_amd64.deb
    musa-musart-dev-5-2_5.2.0_amd64.deb
)
for d in "${DEBS[@]}"; do
    [[ -f "$VENDOR_5_2_DIR/$d" ]] || {
        echo "ERROR: vendor deb not found: $VENDOR_5_2_DIR/$d" >&2
        echo "       See musa-data/research/official-musa-5.2.0/MANIFEST.md" >&2
        exit 1
    }
    dpkg-deb -x "$VENDOR_5_2_DIR/$d" "$WORK/debroot"
done

echo "== Staging compiler subset =="
STAGE="$WORK/musa-sdk-5.2.0"
mkdir -p "$STAGE"
rsync -a "$WORK/debroot/usr/local/musa-5.2/" "$STAGE/"
[[ -d "$WORK/debroot/usr/share" ]] && rsync -a "$WORK/debroot/usr/share/" "$STAGE/share/"

# 1. Drop the Debian-only doc trees.
rm -rf "$STAGE/share/doc"

# 2. Keep only the compiler/linker/binutils binaries in bin/. Everything else
#    (clang-tidy, scan-build drivers, LLVM dev tools) is removed.
KEEP_BIN=(
    clang clang++ clang-14 ld.lld lld
    llvm-ar llvm-as llvm-bitcode-strip llvm-cat llvm-config llvm-mc
    llvm-nm llvm-objcopy llvm-objdump llvm-readelf llvm-strip
    mcc mcc_version mtxas
)
for f in "$STAGE"/bin/*; do
    name="$(basename "$f")"
    keep=0
    for k in "${KEEP_BIN[@]}"; do [[ "$name" == "$k" ]] && keep=1; done
    ((keep)) || rm -f "$f"
done

# 3. Drop development-only LLVM static archives and tooling support trees.
find "$STAGE/lib" -maxdepth 1 -name '*.a' -delete 2>/dev/null || true
rm -rf "$STAGE/lib/cmake" "$STAGE/lib/libscanbuild" "$STAGE/lib/libear" \
       "$STAGE/lib/stubs" \
       "$STAGE/lib/libarcher.so" "$STAGE/lib/libclang.so.14.0.0" \
       "$STAGE/lib/libmtrtc.so.5.2.0" "$STAGE/lib/libompd.so" \
       "$STAGE/lib/libomp.so" "$STAGE/lib/libRemarks.so.14"
# Remove symlinks left dangling by the deletions above.
find "$STAGE/lib" -maxdepth 1 -xtype l -delete 2>/dev/null || true

# 4. Sanity checks: the essentials of the validated toolchain must be present.
for f in bin/mcc bin/mtxas bin/clang bin/ld.lld \
         lib/libmusart.so lib/libLTO.so lib/libclang-cpp.so \
         mtgpu/bitcode/libdevice.bc include/musa_runtime.h; do
    [[ -e "$STAGE/$f" ]] || {
        echo "ERROR: essential compiler file missing after staging: $f" >&2
        exit 1
    }
done

echo "== Creating musa-sdk-5.2.0.tar.gz =="
tar -C "$WORK" -czf "$RPM_DIR/SOURCES/musa-sdk-5.2.0.tar.gz" musa-sdk-5.2.0
echo "  musa-sdk-5.2.0.tar.gz  $(du -h "$RPM_DIR/SOURCES/musa-sdk-5.2.0.tar.gz" | cut -f1)"
echo "== Done. =="

#!/usr/bin/env bash
# create-sources.sh — regenerate the RPM source tarballs from authoritative inputs.
#
# Inputs
#   driver-s30-3.1.0/                     kernel driver source (this repository)
#   $VENDOR_DEB                           vendor MUSA 3.1.0 userspace deb
#                                         (musa_3.1.0-pc_x86.deb from the
#                                          musa-data reference repository)
#
# Outputs (packaging/rpm/SOURCES)
#   mtgpu-dkms-3.1.0.tar.gz               driver source for DKMS
#   mtgpu-firmware-3.1.0.tar.gz           firmware blobs
#   musa-runtime-1.5.4.tar.gz             vendor userspace payload, unmodified
#
# The tarballs contain the vendor files exactly as shipped; the file layout is
# mapped to Fedora paths in the spec files, not here, so the payload stays
# verifiable against the vendor deb.
set -euo pipefail

RPM_DIR="$(cd -- "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd -- "$RPM_DIR/../.." && pwd)"
SOURCES_DIR="$RPM_DIR/SOURCES"
DRIVER_DIR="$PROJECT_DIR/driver-s30-3.1.0"
VENDOR_DEB="${VENDOR_DEB:-$PROJECT_DIR/../musa-data/research/official-musa-3.1.0-s30/musa_3.1.0-pc_x86.deb}"
WORK="$(mktemp -d "${TMPDIR:-/tmp}/musa-sources.XXXXXX")"
trap 'rm -rf "$WORK"' EXIT

[[ -d "$DRIVER_DIR" ]] || { echo "ERROR: missing $DRIVER_DIR" >&2; exit 1; }
[[ -f "$VENDOR_DEB" ]] || { echo "ERROR: vendor deb not found: $VENDOR_DEB" >&2; exit 1; }

echo "== Extracting vendor payload =="
mkdir -p "$WORK/deb"
dpkg-deb -x "$VENDOR_DEB" "$WORK/deb"

echo "== Staging driver source =="
STAGE="$WORK/mtgpu-3.1.0"
rsync -a --exclude='*.o' --exclude='*.o.d' --exclude='*.ko' --exclude='.*.cmd' \
      --exclude='*.mod' --exclude='*.mod.c' --exclude='conftest.h' \
      "$DRIVER_DIR/" "$STAGE/"
# The vendor core object is required; fail loudly if the source tree lost it.
[[ -f "$STAGE/objs/x86_64/mtgpu_core.o_binary" ]] || {
    echo "ERROR: vendor core object missing from $DRIVER_DIR/objs/x86_64" >&2; exit 1; }
echo "vendor core sha256: $(sha256sum "$STAGE/objs/x86_64/mtgpu_core.o_binary" | cut -d' ' -f1)"

echo "== Staging firmware =="
mkdir -p "$WORK/mtgpu-firmware-3.1.0/lib"
cp -a "$WORK/deb/lib/firmware" "$WORK/mtgpu-firmware-3.1.0/lib/"

echo "== Staging runtime payload =="
mkdir -p "$WORK/musa-runtime-1.5.4"
cp -a "$WORK/deb/usr" "$WORK/deb/etc" "$WORK/musa-runtime-1.5.4/"
# i386 shims: the x86_64 package does not ship the i386 loader path.
rm -rf "$WORK/musa-runtime-1.5.4/usr/lib/i386-linux-gnu"
# The driver source is packaged separately as mtgpu-dkms.
rm -rf "$WORK/musa-runtime-1.5.4/usr/src"

echo "== Staging metapackage =="
mkdir -p "$WORK/musa-toolkit-3.1.0"
cp -p "$PROJECT_DIR/README.md" "$WORK/musa-toolkit-3.1.0/README.md"

echo "== Creating tarballs =="
mkdir -p "$SOURCES_DIR"
declare -A TARBALL=(
    [mtgpu-3.1.0]=mtgpu-dkms-3.1.0.tar.gz
    [mtgpu-firmware-3.1.0]=mtgpu-firmware-3.1.0.tar.gz
    [musa-runtime-1.5.4]=musa-runtime-1.5.4.tar.gz
    [musa-toolkit-3.1.0]=musa-toolkit-3.1.0.tar.gz
)
for d in "${!TARBALL[@]}"; do
    tarball="$SOURCES_DIR/${TARBALL[$d]}"
    rm -f "$tarball"
    tar -C "$WORK" -czf "$tarball" "$d"
    echo "  $(basename "$tarball")  $(du -h "$tarball" | cut -f1)"
done

echo "== Done. Rebuild with: packaging/rpm/build-all.sh =="

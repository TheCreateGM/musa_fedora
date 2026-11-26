#!/usr/bin/env bash
# create-cuda-compat-sources.sh — stage the musa-cuda-compat source tarball.
#
# The package payload is this project's own MIT-licensed compatibility layer
# (packaging/cuda-compat). The generated header maps and jump shims are
# committed so a build does not need the vendor SDK present, but they are
# reproducible: see `make -C packaging/cuda-compat regen` on a host that has
# the MUSA SDK installed.
#
# Output: SOURCES/musa-cuda-compat-<version>.tar.gz
# SPDX-License-Identifier: MIT
set -euo pipefail

HERE="$(cd -- "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SRC="$HERE/../cuda-compat"
OUT_DIR="$HERE/SOURCES"
VERSION="${VERSION:-1.0.0}"
NAME="musa-cuda-compat-$VERSION"

mkdir -p "$OUT_DIR"
WORK="$(mktemp -d "${TMPDIR:-/tmp}/musa-cuda-compat-sources.XXXXXX")"
trap 'rm -rf "$WORK"' EXIT

mkdir -p "$WORK/$NAME"
cp -a "$SRC"/. "$WORK/$NAME"/
# Drop editor/backup noise but keep generated tables and shims.
find "$WORK/$NAME" -name '*.orig' -o -name '*~' | xargs -r rm -f

tar -C "$WORK" --owner=0 --group=0 --numeric-owner -czf "$OUT_DIR/$NAME.tar.gz" "$NAME"
echo "wrote $OUT_DIR/$NAME.tar.gz"
sha256sum "$OUT_DIR/$NAME.tar.gz"

#!/usr/bin/env bash
# build-all.sh — build every spec in packaging/rpm/SPECS locally.
#
# Builds the historically exercised MTT S30 package set. Current Fedora
# driver loading remains blocked pending safe vendor-core validation:
#   mtgpu-dkms 3.1.0, mtgpu-firmware 3.1.0, musa-runtime 1.5.4,
#   musa-sdk 5.2.0 (compiler only), musa-toolkit 3.1.0 (metapackage).
#
# Source tarballs are produced by create-sources.sh (vendor deb + driver
# source). The 5.2.0 compiler tarball is staged separately; see
# staging/create-sdk-sources.sh.
#
# Usage: build-all.sh [--srpm-only]
set -euo pipefail

RPM_DIR="$(cd -- "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SRPM_ONLY=0
(($# <= 1)) || { echo "usage: $0 [--srpm-only]" >&2; exit 2; }
(($# == 0)) || { [[ "$1" == "--srpm-only" ]] || { echo "unknown arg: $1" >&2; exit 2; }; SRPM_ONLY=1; }

TOPDIR="$RPM_DIR/local-build"
rm -rf "$TOPDIR"
mkdir -p "$TOPDIR/SRPMS" "$TOPDIR/RPMS"
# rpmbuild resolves Source0: against %{_sourcedir}; point it at the staged
# SOURCES tree produced by create-sources.sh / create-sdk-sources.sh.
ln -sfn "$RPM_DIR/SOURCES" "$TOPDIR/SOURCES"

SPEC_BASE=(mtgpu-firmware mtgpu-dkms musa-runtime musa-sdk musa-toolkit)

rpmbuild_flags=(--define "_topdir $TOPDIR" --define "_srcrpmdir %{_topdir}/SRPMS"
                --define "_rpmdir %{_topdir}/RPMS")

for spec_base in "${SPEC_BASE[@]}"; do
    spec="$RPM_DIR/SPECS/$spec_base.spec"
    [[ -f "$spec" ]] || { echo "ERROR: missing $spec" >&2; exit 1; }
    echo "== Building SRPM: $spec_base =="
    rpmbuild "${rpmbuild_flags[@]}" -bs "$spec"
    if ((SRPM_ONLY == 0)); then
        echo "== Building RPM: $spec_base =="
        rpmbuild "${rpmbuild_flags[@]}" -bb "$spec"
    fi
done

echo
echo "== Verifying package integrity (rpm -K, digests) =="
# Local builds are unsigned by design; Copr signs published packages. Verify
# digests only (rpm 6.x defaults to %_pkgverify_level all, which would also
# demand signatures and report every unsigned local package as NOT OK).
fail=0
while IFS= read -r -d '' rpm; do
    if out="$(rpm -K --nosignature "$rpm" 2>&1)"; then
        ok=1
    else
        ok=0; fail=1
    fi
    case "$out" in
        *digests*NOT\ OK*) ok=0; fail=1 ;;
    esac
    if ((ok)); then
        echo "  OK      $(basename "$rpm")"
    else
        echo "  FAILED  $(basename "$rpm"): $out"
    fi
done < <(find "$TOPDIR" -name '*.rpm' -print0 | sort -z)

if ((fail)); then
    echo "ERROR: package integrity verification failed" >&2
    exit 1
fi
echo "== Done: $(find "$TOPDIR" -name '*.rpm' | wc -l) packages verified. =="

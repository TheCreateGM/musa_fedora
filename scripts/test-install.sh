#!/usr/bin/env bash
# test-install.sh — validate the published packages without touching the host.
#
# Gate 1: dependency resolution + clean install of musa-toolkit into a
#         disposable dnf5 installroot from the Copr repository.
# Gate 2: DKMS build of the packaged driver source for the *running* kernel
#         inside a disposable Fedora container, with the host kernel headers
#         mounted read-only. Nothing is installed into /lib/modules on the host.
#
# Usage: scripts/test-install.sh [fedora-45-x86_64]
set -euo pipefail

PROJECT="${COPR_PROJECT:-axogm/musa-mtt-s30}"
CHROOT="${1:-fedora-45-x86_64}"
FEDORA_MAJOR="${CHROOT##*-}"
WORKDIR="${TESTINSTALL_WORKDIR:-/var/tmp/musa-install-test}"
REPO="https://download.copr.fedorainfracloud.org/results/${PROJECT}/${CHROOT}/"
RPMDIR="$(cd -- "$(dirname "${BASH_SOURCE[0]}")/../packaging/rpm" && pwd)"
KVER="$(uname -r)"

echo "=== Gate 1: clean installroot install from Copr (${CHROOT}) ==="
echo "repo: $REPO"
rm -rf "$WORKDIR"
mkdir -p "$WORKDIR"
FEDORA_BASE="https://dl.fedoraproject.org/pub/fedora/linux/releases/${FEDORA_MAJOR}/Everything/x86_64/os/"

# --releasever is required: an empty installroot cannot detect it.
dnf5 install --installroot="$WORKDIR" --releasever="$FEDORA_MAJOR" \
    --repofrompath=fedora,"$FEDORA_BASE" \
    --repofrompath=copr,"$REPO" \
    --disablerepo='*' --enablerepo=fedora,copr \
    -y musa-toolkit 2>&1 | tail -25

echo "--- installed packages ---"
dnf5 repoquery --installroot="$WORKDIR" --releasever="$FEDORA_MAJOR" \
    --installed -a --queryformat '%{name}-%{version}-%{release}\n' 2>/dev/null |
    grep -E '^(musa|mtgpu)' | sort

echo "--- key files ---"
rc=0
for f in /usr/src/mtgpu-3.1.0/dkms.conf \
         /usr/src/mtgpu-3.1.0/objs/x86_64/mtgpu_core.o_binary \
         /lib/firmware/mthreads/mtfw-gen1.bin \
         /usr/local/musa/lib/libmusart.so.1.5.4 \
         /usr/local/musa/bin/musaInfo \
         /usr/bin/mthreads-gmi \
         /usr/lib64/libdrm_mtgpu.so.1 \
         /usr/lib64/dri/musa_dri.so \
         /etc/ld.so.conf.d/musa-runtime.conf \
         /usr/local/musa-5.2/bin/mcc; do
    if [[ -e "$WORKDIR$f" ]]; then
        echo "  OK   $f"
    else
        echo "  MISS $f"; rc=1
    fi
done
[[ $rc -eq 0 ]] || { echo "ERROR: missing installed files" >&2; exit 1; }

echo
echo "=== Gate 2: DKMS build from the packaged source (kernel $KVER) ==="
podman run --rm \
    -v /usr/src/kernels:/usr/src/kernels:ro \
    -v /lib/modules:/lib/modules:ro \
    -v "$RPMDIR/RPMS/noarch:/rpms:ro" \
    docker.io/library/fedora:"$FEDORA_MAJOR" bash -euc '
        dnf -y -q install dkms kernel-devel "kernel-devel-${1}" gcc make \
            elfutils-libelf-devel openssl-devel >/dev/null
        rpm -Uvh --nodeps --noscripts /rpms/mtgpu-dkms-*.rpm >/dev/null
        dkms add    -m mtgpu -v 3.1.0
        dkms build  -m mtgpu -v 3.1.0 -k "$1"
        ko="/var/lib/dkms/mtgpu/3.1.0/$1/x86_64/module/mtgpu.ko"
        test -f "$ko"
        echo "built: $ko ($(stat -c %s "$ko") bytes)"
        modinfo -F vermagic "$ko"
        modinfo -F alias "$ko" | grep -c pci:v00001ED5
        readelf -n "$ko" | grep -i -A1 "properties" | grep -i ibt && echo "IBT property present"
    ' _ "$KVER"

echo
echo "TEST_INSTALL_STATUS: SUCCESS"
echo "NOTE: neither gate loads the module or touches the host driver; GPU"
echo "      execution is verified separately with scripts/verify-musa.sh."

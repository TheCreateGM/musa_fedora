#!/usr/bin/env bash
# test-container-install.sh — isolated install + real-GPU test of the built RPMs.
#
# This is the "disposable environment" gate: it installs the locally built RPM
# set into a throwaway Fedora container, with only the S30 device nodes passed
# through, and then exercises the packaged runtime and compiler against the
# physical MTT S30. It does not touch the host driver, firmware, bootloader, or
# package database.
#
# What it verifies (each step fails loudly):
#   1. The RPM set installs and its declared dependencies resolve (the SDK
#      package must pull the C++ headers that mcc needs).
#   2. The packaged MUSA 1.5.4 runtime discovers the S30 (musaInfo).
#   3. Vendor muMemcpyTest runs (skipped on tight memory limits).
#   4. Host runtime API (tests/musart-smoke.c) builds and runs on the GPU.
#   5. The packaged mcc 5.2.0 compiler builds an mp_10 kernel that runs on the
#      S30 (tests/musa-vector-add.mu).
#
# Usage:
#   scripts/test-container-install.sh [--skip-memcpy] [--keep-image]
#
# Env overrides: RPMDIR, FEDORA_MAJOR, IMAGE, S30_DEV, RENDER_DEV
#
# Safety: read-only toward the host. Suppresses RPM scriptlets so no kernel
# module is built, loaded, or replaced.
set -euo pipefail

ROOT="$(cd -- "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
RPMDIR="${RPMDIR:-$ROOT/packaging/rpm/local-build/RPMS}"
FEDORA_MAJOR="${FEDORA_MAJOR:-45}"
BASE_IMAGE="${BASE_IMAGE:-docker.io/library/fedora:${FEDORA_MAJOR}}"
IMAGE="${IMAGE:-localhost/musa-s30-container-test:${FEDORA_MAJOR}}"
S30_DEV="${S30_DEV:-/dev/mtgpu.0}"
SKIP_MEMCPY=0
KEEP_IMAGE=0

while (($#)); do
    case "$1" in
        --skip-memcpy) SKIP_MEMCPY=1; shift ;;
        --keep-image) KEEP_IMAGE=1; shift ;;
        -h|--help) sed -n '2,22p' "${BASH_SOURCE[0]}"; exit 0 ;;
        *) echo "unknown option: $1" >&2; exit 2 ;;
    esac
done

[[ -d "$RPMDIR" ]] || { echo "ERROR: RPM dir not found: $RPMDIR (run packaging/rpm/build-all.sh)" >&2; exit 1; }
mapfile -t RPMS < <(find "$RPMDIR" -name '*.rpm' | sort)
((${#RPMS[@]})) || { echo "ERROR: no RPMs in $RPMDIR" >&2; exit 1; }
[[ -e "$S30_DEV" ]] || { echo "ERROR: $S30_DEV not present; is the mtgpu driver live?" >&2; exit 1; }

# Resolve the S30 render node dynamically (its number can change between boots).
RENDER_DEV="${RENDER_DEV:-}"
if [[ -z "$RENDER_DEV" ]]; then
    for r in /dev/dri/renderD*; do
        [[ -e "$r" ]] || continue
        drv=$(basename "$(readlink -f "/sys/class/drm/$(basename "$r")/device/driver" 2>/dev/null)")
        [[ "$drv" == mtgpu ]] && { RENDER_DEV="$r"; break; }
    done
fi
[[ -n "$RENDER_DEV" && -e "$RENDER_DEV" ]] || { echo "ERROR: no mtgpu render node found" >&2; exit 1; }

echo "RPM dir     : $RPMDIR"
echo "RPMs        : ${#RPMS[@]}"
printf '              %s\n' "$(printf '%s\n' "${RPMS[@]}" | xargs -n1 basename)"
echo "Base image  : $BASE_IMAGE"
echo "S30 devices : $S30_DEV  $RENDER_DEV"
echo

WORK="$(mktemp -d "${TMPDIR:-/tmp}/musa-ctest.XXXXXX")"
cleanup() {
    rm -rf "$WORK"
    if ((KEEP_IMAGE)); then
        echo "(keeping image $IMAGE)"
    else
        podman rmi -f "$IMAGE" >/dev/null 2>&1 || true
    fi
}
trap cleanup EXIT

echo "== Staging container context =="
mkdir -p "$WORK/ctx/RPMS"
cp -a "${RPMS[@]}" "$WORK/ctx/RPMS/"
cp -a "$ROOT/tests" "$WORK/ctx/tests"
# Trim test sources to what the container actually needs.
rm -f "$WORK/ctx/tests"/*.sh 2>/dev/null || true

cat > "$WORK/ctx/Containerfile" <<EOF
FROM ${BASE_IMAGE}
COPY RPMS/ /rpms/
COPY tests/ /tests/
# The default metalink resolved to mirrors throttled below dnf's curl
# low-speed limit on 2026-11-25 (repeated curl error 28, single-digit KiB/s
# on ftp.yz.yamagata-u.ac.jp vs ~58 KiB/s on the official CDN), which stalled
# the dependency install for over an hour. Pin the repo layer to the
# dl.fedoraproject.org baseurls that ship behind the same metalink: same
# repos, same packages, same versions — only the mirror selection changes.
# dnf5 in the fedora base image keeps its defaults under /usr/share/dnf5/
# repos.d (the image's /etc/yum.repos.d is empty) and the commented baseurl
# template uses the placeholder host download.example. Only repo files that
# carry that template are rewritten; fedora-cisco-openh264 has no baseurl
# template and keeps its metalink.
RUN for f in /usr/share/dnf5/repos.d/*.repo /etc/yum.repos.d/*.repo; do \
        [ -f "\$f" ] || continue; \
        grep -q '^#baseurl=http://download.example/pub/' "\$f" || continue; \
        sed -i -e 's|^metalink=|#metalink=|' \
               -e 's|^#baseurl=|baseurl=|' \
               -e 's|baseurl=http://download.example/pub/|baseurl=https://dl.fedoraproject.org/pub/|' \
               "\$f"; \
    done
# Resolve declared dependencies from the Fedora repos (this is what proves the
# SDK package pulls the C++ headers mcc needs), but suppress RPM scriptlets so
# no kernel module is built or loaded inside the container.
RUN dnf -y --setopt=tsflags=noscripts --setopt=install_weak_deps=False \\
        install /rpms/*.rpm \\
 && dnf -y --setopt=install_weak_deps=False install gcc make \\
 && dnf clean all \\
 && rm -rf /rpms
ENV LD_LIBRARY_PATH=/usr/local/musa/lib
WORKDIR /work
EOF

echo "== Building throwaway image =="
podman build -t "$IMAGE" "$WORK/ctx"

echo
echo "== Running isolated install + GPU tests =="
SKIP_MEMCPY_ENV="$SKIP_MEMCPY" podman run --rm \
    --device "$S30_DEV" --device "$RENDER_DEV" \
    "$IMAGE" bash -euc '
set -x
# Reproduce the documented environment. The packages deliberately do not set
# PATH (matching the vendor deb, which ships no profile.d either); a user
# exports MUSA_HOME/PATH as documented in musa-sdk.spec and INSTALL.md.
export MUSA_HOME=/usr/local/musa-5.2
export PATH="/usr/local/musa-5.2/bin:/usr/local/musa/bin:$PATH"
pass=0; fail=0
ok()  { printf "  PASS  %s\n" "$*"; pass=$((pass+1)); }
bad() { printf "  FAIL  %s\n" "$*"; fail=$((fail+1)); }

echo "--- installed musa/mtgpu packages ---"
rpm -qa | grep -iE "^(musa|mtgpu)" | sort

echo "--- packaged files ---"
for f in /usr/local/musa/lib/libmusart.so.1.5.4 \
         /usr/local/musa/bin/musaInfo \
         /usr/bin/mthreads-gmi \
         /usr/local/musa-5.2/bin/mcc \
         /usr/local/musa-5.2/include/musa_runtime.h \
         /usr/local/musa/include/musa_runtime.h; do
    [[ -e "$f" ]] && ok "present $f" || bad "missing $f"
done

echo "--- C++ standard headers (musa-sdk Requires: libstdc++-devel) ---"
if compgen -G "/usr/include/c++/*/cmath" >/dev/null; then
    ok "C++ standard headers present (<cmath>)"
else
    bad "C++ standard headers missing (<cmath>)"
fi
# The packaged clang++ is the host side of mcc; it must resolve <cmath>.
if echo "#include <cmath>" | /usr/local/musa-5.2/bin/clang++ -x c++ -fsyntax-only - 2>/dev/null; then
    ok "packaged clang++ resolves <cmath>"
else
    bad "packaged clang++ cannot resolve <cmath>"
fi

echo "--- runtime device discovery (musaInfo) ---"
if timeout 60 musaInfo; then ok "musaInfo succeeded"; else bad "musaInfo failed"; fi

echo "--- vendor memcpy self-test ---"
if [[ "${SKIP_MEMCPY_ENV:-0}" == "1" ]]; then
    echo "  SKIP  muMemcpyTest skipped by request"
elif timeout 60 muMemcpyTest; then ok "muMemcpyTest succeeded"; else bad "muMemcpyTest failed"; fi

echo "--- host runtime API (musart-smoke) ---"
if gcc -O2 -Wall /tests/musart-smoke.c -I/usr/local/musa/include \
       -L/usr/local/musa/lib -lmusart -Wl,-rpath,/usr/local/musa/lib -o /work/musart-smoke; then
    if timeout 60 /work/musart-smoke; then ok "musart-smoke passed"; else bad "musart-smoke failed"; fi
else
    bad "musart-smoke did not build"
fi

echo "--- packaged mcc compiler (mp_10 vector-add) ---"
if mcc -O2 --offload-arch=mp_10 \
       -I/usr/local/musa-5.2/include -L/usr/local/musa/lib -lmusart \
       -Wl,-rpath,/usr/local/musa/lib -o /work/vector_add /tests/musa-vector-add.mu; then
    ok "mcc built mp_10 vector-add"
    if timeout 60 /work/vector_add; then ok "vector-add ran on the GPU"; else bad "vector-add did not run"; fi
else
    bad "mcc failed to build the test"
fi

printf "\nCONTAINER_SUMMARY: %d passed, %d failed\n" "$pass" "$fail"
[[ "$fail" -eq 0 ]]
'

echo
echo "TEST_CONTAINER_INSTALL_STATUS: SUCCESS"

#!/usr/bin/env bash
# container-test.sh — install musa-cuda-compat in a throwaway container with the
# S30 passed through, and run the CUDA-compat suite there.
#
# This is the isolated gate: it proves the built RPM installs, resolves its
# dependencies and works against the physical GPU without touching the host
# driver, firmware or package database.
#
# Usage: container-test.sh [--rpms DIR] [--keep-image]
# Env: BASE_IMAGE, S30_DEV, RENDER_DEV
#
# SPDX-License-Identifier: MIT
set -euo pipefail

HERE="$(cd -- "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT="$(cd -- "$HERE/../.." && pwd)"
RPMDIR="${RPMDIR:-$PROJECT/packaging/rpm/local-build/RPMS/x86_64}"
S30_DEV="${S30_DEV:-/dev/mtgpu.0}"
IMAGE="${IMAGE:-localhost/musa-cuda-compat-test:1}"
BASE_IMAGE="${BASE_IMAGE:-docker.io/library/fedora:45}"
KEEP=0
while (($#)); do
    case "$1" in
        --rpms) RPMDIR="${2:?}"; shift 2;;
        --keep-image) KEEP=1; shift;;
        -h|--help) sed -n '2,12p' "${BASH_SOURCE[0]}"; exit 0;;
        *) echo "unknown option: $1" >&2; exit 2;;
    esac
done

need=(musa-runtime musa-sdk musa-cuda-compat)
for n in "${need[@]}"; do
    compgen -G "$RPMDIR/$n-*.rpm" >/dev/null || {
        echo "ERROR: $n RPM not found in $RPMDIR" >&2; exit 1; }
done
[[ -e "$S30_DEV" ]] || { echo "ERROR: $S30_DEV absent; is mtgpu loaded?" >&2; exit 1; }

RENDER_DEV="${RENDER_DEV:-}"
if [[ -z "$RENDER_DEV" ]]; then
    for r in /dev/dri/renderD*; do
        [[ -e "$r" ]] || continue
        d=$(basename "$(readlink -f "/sys/class/drm/$(basename "$r")/device/driver" 2>/dev/null)")
        [[ "$d" == mtgpu ]] && { RENDER_DEV="$r"; break; }
    done
fi
[[ -n "$RENDER_DEV" && -e "$RENDER_DEV" ]] || { echo "ERROR: no mtgpu render node" >&2; exit 1; }

WORK="$(mktemp -d "${TMPDIR:-/tmp}/musa-ccontainer.XXXXXX")"
cleanup() {
    rm -rf "$WORK"
    if ((KEEP)); then echo "(keeping $IMAGE)"; else podman rmi -f "$IMAGE" >/dev/null 2>&1 || true; fi
}
trap cleanup EXIT

echo "RPMs   : $RPMDIR"
echo "devices: $S30_DEV $RENDER_DEV"
mkdir -p "$WORK/ctx/RPMS" "$WORK/ctx/tests"
cp -a "$RPMDIR"/musa-runtime-*.rpm "$RPMDIR"/musa-sdk-*.rpm \
      "$RPMDIR"/musa-cuda-compat-*.rpm "$WORK/ctx/RPMS/"
cp -a "$HERE"/*.cu "$WORK/ctx/tests/"
cp -a "$PROJECT/packaging/cuda-compat" "$WORK/ctx/tests/"

cat >"$WORK/ctx/Containerfile" <<EOF
FROM $BASE_IMAGE
COPY RPMS/ /rpms/
# Pin the repo layer to the dl.fedoraproject.org baseurls behind the metalink;
# the default mirror selection has stalled on this network before.
RUN for f in /usr/share/dnf5/repos.d/*.repo /etc/yum.repos.d/*.repo; do \
        [ -f "\$f" ] || continue; \
        grep -q '^#baseurl=http://download.example/pub/' "\$f" || continue; \
        sed -i -e 's|^metalink=|#metalink=|' \
               -e 's|^#baseurl=|baseurl=|' \
               -e 's|baseurl=http://download.example/pub/|baseurl=https://dl.fedoraproject.org/pub/|' \
               "\$f"; \
    done
RUN dnf -y --setopt=tsflags=noscripts --setopt=install_weak_deps=False \\
        install /rpms/*.rpm \\
 && dnf -y --setopt=install_weak_deps=False install gcc make cmake python3 binutils elfutils \\
 && dnf clean all && rm -rf /rpms
ENV LD_LIBRARY_PATH=/usr/local/musa/lib
WORKDIR /work
EOF

podman build -t "$IMAGE" "$WORK/ctx"

echo "== running in isolation =="
# label=disable: podman's default SELinux confinement denies the GMI and
# musaInfo queries against the passthrough device nodes.
podman run --rm --security-opt label=disable \
    --device "$S30_DEV" --device "$RENDER_DEV" \
    -v "$WORK/ctx/tests:/tests:ro,z" "$IMAGE" bash -euc '
pass=0; fail=0
ok()  { printf "  PASS  %s\n" "$*"; pass=$((pass+1)); }
bad() { printf "  FAIL  %s\n" "$*"; fail=$((fail+1)); }

echo "--- package ---"
rpm -q musa-cuda-compat || bad "musa-cuda-compat not installed"

echo "--- installed layout ---"
for f in /usr/local/cuda/bin/nvcc /usr/local/cuda/include/cuda_runtime.h \
         /usr/lib64/libcudart.so.11.0 /usr/lib64/libcuda.so.1 \
         /usr/lib64/musa-cuda-compat/nvvm/libdevice/libdevice.10.bc; do
    [[ -e "$f" ]] && ok "present $f" || bad "missing $f"
done

echo "--- device ---"
export PATH="/usr/local/musa/bin:/usr/local/musa-5.2/bin:$PATH"
musaInfo | grep -q "MTT S" && ok "musaInfo reports the S30" || bad "musaInfo failed"
mthreads-gmi | grep -q "MTT S" && ok "mthreads-gmi reports the S30" || bad "mthreads-gmi failed"

echo "--- plain CUDA build via nvcc ---"
if /usr/local/cuda/bin/nvcc -O2 -o /work/va /tests/cuda-vector-add.cu; then
    ok "nvcc built plain CUDA source"
    timeout 120 /work/va | tail -2
    timeout 120 /work/va >/dev/null 2>&1 && ok "vector-add ran on the GPU" \
        || bad "vector-add failed"
else
    bad "nvcc could not build CUDA source"
fi

echo "--- cuda.h driver API build ---"
if /usr/local/cuda/bin/nvcc -O2 -o /work/drv /tests/cuda-driver.cu; then
    timeout 120 /work/drv | tail -2
    timeout 120 /work/drv >/dev/null 2>&1 && ok "driver API ran" || bad "driver API failed"
else
    bad "cuda.h source did not build"
fi

echo "--- fp16 + streams ---"
for t in cuda-fp16 cuda-streams; do
    if /usr/local/cuda/bin/nvcc -O2 -o /work/$t /tests/$t.cu; then
        timeout 120 /work/$t >/dev/null 2>&1 && ok "$t ran" || bad "$t failed"
    else
        bad "$t did not build"
    fi
done

echo "--- musify-text: installed tool + musified run on the GPU ---"
if [[ -x /usr/bin/musify-text ]]; then
    mkdir -p /work/port
    if musify-text --rename-ext --strict -o /work/port /tests/cuda-vector-add.cu; then
        ok "musify-text converted with zero CUDA-token residue"
    else
        bad "musify-text --strict conversion failed"
    fi
    if mcc -O2 -x musa --offload-arch=mp_10 -I/usr/local/musa-5.2/include \
            -include /usr/lib64/musa-cuda-compat/include/musa_rt154_names.h \
            /work/port/cuda-vector-add.mu -L/usr/local/musa/lib \
            -Wl,-rpath,/usr/local/musa/lib -lmusart -lmusa -o /work/musified; then
        out="$(timeout 120 /work/musified 2>&1)" || true
        if printf "%s" "$out" | grep -q "VADD_STATUS: PASS"; then
            ok "musified CUDA source built with mcc and ran on the S30"
        else
            bad "musified run failed: ${out: -120}"
        fi
    else
        bad "mcc could not build the musified source"
    fi
else
    bad "musify-text not installed at /usr/bin/musify-text"
fi

echo "--- CMake: find_package(MUSA) from the installed config ---"
if command -v cmake >/dev/null && [[ -f /usr/lib64/cmake/musa/MUSAConfig.cmake ]]; then
    mkdir -p /work/cminst
    cp /work/port/cuda-vector-add.mu /work/cminst/kernel.mu 2>/dev/null || true
    if [[ -f /work/cminst/kernel.mu ]]; then
        cat >/work/cminst/CMakeLists.txt <<CMK
cmake_minimum_required(VERSION 3.16)
project(cminst CXX)
find_package(MUSA REQUIRED)
musa_add_executable(cmvadd kernel.mu)
CMK
        if cmake -S /work/cminst -B /work/cminst/build >/work/cminst/cfg.log 2>&1 \
                && cmake --build /work/cminst/build >>/work/cminst/cfg.log 2>&1; then
            if grep -q "Found MUSA" /work/cminst/cfg.log; then
                ok "find_package(MUSA) resolved from /usr/lib64/cmake/musa"
            else
                bad "configured without reporting Found MUSA"
            fi
            out="$(timeout 120 /work/cminst/build/musa/cmvadd 2>&1)" || true
            if printf "%s" "$out" | grep -q "VADD_STATUS: PASS"; then
                ok "musa_add_executable binary ran on the S30"
            else
                bad "installed-layout CMake run failed: ${out: -120}"
            fi
        else
            bad "installed-layout CMake gate failed"
            tail -8 /work/cminst/cfg.log
        fi
    else
        bad "no musified kernel for the CMake gate"
    fi
else
    bad "cmake or /usr/lib64/cmake/musa/MUSAConfig.cmake missing"
fi

echo "--- name coverage audit ---"
python3 /tests/cuda-compat/gen-cuda-compat.py --print-audit | head -3

printf "\nCONTAINER_COMPAT_SUMMARY: %d passed, %d failed\n" "$pass" "$fail"
[[ "$fail" -eq 0 ]]
'
echo
echo "CONTAINER_TEST_STATUS: SUCCESS"

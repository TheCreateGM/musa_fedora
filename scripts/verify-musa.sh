#!/usr/bin/env bash
# verify-musa.sh — end-to-end status check for the MTT S30 MUSA stack.
#
# Read-only except for creating a temporary build directory under $TMPDIR.
# Prints PASS/FAIL (or an explicit SKIP), and never claims a layer works
# unless the underlying command actually succeeded.
set -uo pipefail

ROOT="$(cd -- "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
MUSA_RT_LIB="${MUSA_RT_LIB:-/usr/local/musa/lib}"
MUSA_RT_INC="${MUSA_RT_INC:-/usr/local/musa/include}"
MUSA_SDK="${MUSA_SDK:-/usr/local/musa-5.2}"
SKIP_MEMCPY=0
while (($#)); do
    case "$1" in
        --skip-memcpy) SKIP_MEMCPY=1; shift ;;
        -h|--help)
            printf 'Usage: %s [--skip-memcpy]\n' "${0##*/}"
            printf '  --skip-memcpy  Skip the vendor copy test (useful with a tight memory limit).\n'
            exit 0 ;;
        *) printf 'Unknown option: %s\n' "$1" >&2; exit 2 ;;
    esac
done
WORK="$(mktemp -d "${TMPDIR:-/tmp}/musa-verify.XXXXXX")"
trap 'rm -rf "$WORK"' EXIT

pass=0; fail=0; skipped=0
ok()   { printf '  \033[32mPASS\033[0m  %s\n' "$*"; pass=$((pass+1)); }
bad()  { printf '  \033[31mFAIL\033[0m  %s\n' "$*"; fail=$((fail+1)); }
skip() { printf '  \033[33mSKIP\033[0m  %s\n' "$*"; skipped=$((skipped+1)); }
info() { printf '        %s\n' "$*"; }
hdr()  { printf '\n\033[1m== %s ==\033[0m\n' "$*"; }

hdr "Host"
info "$(cat /etc/os-release | sed -n 's/^PRETTY_NAME=//p' | tr -d '"')"
info "kernel: $(uname -r)  arch: $(uname -m)"

hdr "PCI / driver"
if lspci -nn | grep -q '1ed5:0102'; then
    ok "MTT S30 present on PCI (1ed5:0102)"
    info "$(lspci -nnk -d 1ed5:0102 | tr -s ' ' | tr '\n' ' ')"
else
    bad "MTT S30 (1ed5:0102) not found on PCI"
fi
if [[ -d /sys/module/mtgpu ]]; then
    ok "mtgpu module loaded (init state: $(cat /sys/module/mtgpu/initstate 2>/dev/null || echo live))"
else
    bad "mtgpu module not loaded"
fi
if [[ -e /dev/mtgpu.0 ]]; then ok "/dev/mtgpu.0 present"; else bad "/dev/mtgpu.0 missing"; fi
rnode=$(for r in /dev/dri/renderD*; do
            [[ -e "$r" ]] || continue
            d=$(basename "$(readlink -f "/sys/class/drm/$(basename "$r")/device/driver" 2>/dev/null)")
            [[ "$d" == mtgpu ]] && echo "$r"
        done | head -1)
if [[ -n "$rnode" ]]; then ok "mtgpu render node: $rnode"; else bad "no mtgpu render node"; fi

hdr "IOMMU"
if [[ -d /sys/kernel/iommu_groups ]] && [[ -n "$(ls -A /sys/kernel/iommu_groups 2>/dev/null)" ]]; then
    ok "IOMMU groups present ($(ls /sys/kernel/iommu_groups | wc -l) groups)"
else
    bad "no IOMMU groups - MUSA runtime will reject the device (see scripts/enable-iommu.sh)"
    info "current cmdline: $(cat /proc/cmdline)"
fi

hdr "Vendor management tool (mthreads-gmi)"
GMI_BIN=""
for candidate in /usr/bin/mthreads-gmi /usr/local/musa/bin/mthreads-gmi; do
    if [[ -x "$candidate" ]]; then GMI_BIN="$candidate"; break; fi
done
if [[ -n "$GMI_BIN" ]]; then
    if out=$(LD_LIBRARY_PATH="$MUSA_RT_LIB" timeout 30 "$GMI_BIN" 2>&1); then
        if grep -q 'MTT S30' <<<"$out"; then
            ok "mthreads-gmi reports the MTT S30"
            grep -E 'Driver Version|MTT S30' <<<"$out" | sed 's/^/        /' | head -4
        else
            bad "mthreads-gmi ran but did not report an S30"
        fi
    else
        bad "mthreads-gmi failed"
    fi
else
    bad "mthreads-gmi not installed"
fi

hdr "MUSA runtime (libmusart 1.5.4)"
if [[ -x /usr/local/musa/bin/musa_runtime_version ]]; then
    LD_LIBRARY_PATH="$MUSA_RT_LIB" /usr/local/musa/bin/musa_runtime_version 2>&1 | sed 's/^/        /'
fi
if [[ -x /usr/local/musa/bin/musaInfo ]]; then
    if LD_LIBRARY_PATH="$MUSA_RT_LIB" timeout 30 /usr/local/musa/bin/musaInfo >"$WORK/musaInfo.log" 2>&1; then
        ok "musaInfo succeeded"
    else
        bad "musaInfo failed: $(grep -m1 -i error "$WORK/musaInfo.log" || echo 'unknown error')"
        info "run with MUSA_LOG=0xffff for detail"
    fi
fi

hdr "Vendor memcpy self-test"
if ((SKIP_MEMCPY)); then
    skip "muMemcpyTest skipped by request"
elif [[ -x /usr/local/musa/bin/muMemcpyTest ]]; then
    if LD_LIBRARY_PATH="$MUSA_RT_LIB" timeout 60 /usr/local/musa/bin/muMemcpyTest >"$WORK/memcpy.log" 2>&1; then
        ok "muMemcpyTest succeeded"
    else
        bad "muMemcpyTest failed: $(grep -m1 -i 'error' "$WORK/memcpy.log" || echo 'unknown error')"
    fi
fi

hdr "MUSA runtime host API (alloc/memcpy/memset/streams)"
if [[ -f "$ROOT/tests/musart-smoke.c" ]] && [[ -f "$MUSA_RT_INC/musa_runtime.h" ]]; then
    # Compile against the runtime headers that match the installed library so
    # musaDeviceProp keeps the correct ABI (1.5.4 is 728 bytes; 5.2.0 is 1032).
    if gcc -O2 -Wall "$ROOT/tests/musart-smoke.c" -I"$MUSA_RT_INC" \
        -L"$MUSA_RT_LIB" -lmusart -Wl,-rpath,"$MUSA_RT_LIB" \
        -o "$WORK/musart-smoke" 2>"$WORK/smoke-cc.log"; then
        if LD_LIBRARY_PATH="$MUSA_RT_LIB" timeout 60 "$WORK/musart-smoke" >"$WORK/smoke.log" 2>&1; then
            ok "musart-smoke passed on the GPU"
            grep -E 'name=|totalGlobalMem=|multiProcessorCount=|d2h_memset_verify=|async_roundtrip=|event_elapsed_ms=|SMOKE_TEST' \
                "$WORK/smoke.log" | sed 's/^/        /'
        else
            bad "musart-smoke ran but failed: $(tail -1 "$WORK/smoke.log")"
        fi
    else
        bad "musart-smoke did not build"
        head -5 "$WORK/smoke-cc.log" | sed 's/^/        /'
    fi
else
    skip "musart-smoke prerequisites missing ($MUSA_RT_INC/musa_runtime.h or tests/musart-smoke.c)"
fi

hdr "Compiler + runtime vector-add"
if [[ -x "$MUSA_SDK/bin/mcc" ]] && [[ -f "$ROOT/tests/musa-vector-add.mu" ]]; then
    if "$MUSA_SDK/bin/mcc" -O2 --offload-arch=mp_10 \
        -I"$MUSA_SDK/include" -L"$MUSA_RT_LIB" -lmusart \
        -Wl,-rpath,"$MUSA_RT_LIB" \
        -o "$WORK/vector_add" "$ROOT/tests/musa-vector-add.mu" 2>"$WORK/cc.log"; then
        ok "mcc built tests/musa-vector-add.mu for mp_10"
        if LD_LIBRARY_PATH="$MUSA_RT_LIB" timeout 60 "$WORK/vector_add" >"$WORK/run.log" 2>&1; then
            ok "vector-add ran on the GPU"
            sed 's/^/        /' "$WORK/run.log"
        else
            bad "vector-add built but did not run: $(tail -1 "$WORK/run.log")"
        fi
    else
        bad "mcc failed to build the test"
        head -5 "$WORK/cc.log" | sed 's/^/        /'
    fi
else
    bad "mcc or tests/musa-vector-add.mu missing"
fi

printf '\n\033[1mSummary:\033[0m %d passed, %d failed, %d skipped\n' "$pass" "$fail" "$skipped"
[[ "$fail" -eq 0 ]]

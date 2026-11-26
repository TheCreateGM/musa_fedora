#!/usr/bin/env bash
# test-video-decode.sh — exercise hardware video *decode* on the MTT S30.
#
# Two independent decode routes exist on this host, and this script drives
# both, reports exactly where each one stops, and never claims a codec
# works unless decoded pixels were read back.
#
#   1. VA-API route   — libva VAEntrypointVLD through the packaged
#                       /usr/lib64/dri/mtgpu_drv_video.so driver.
#                       (tests/vadecode.c)
#   2. MUSA MUVID route — libvideo_musa.so from musa-runtime, the
#                       MUSA-native hardware video API.
#                       (tests/muvid-decode.c)
#
# The script is read-only with respect to the system: it writes only into its
# own scratch directory and installs nothing. It needs a test bitstream; if
# none is given it encodes one with the S30's own VA-API H.264 encoder.
#
# Usage:
#   scripts/hardware/test-video-decode.sh [--stream FILE] [--keep]
#
# Env overrides:
#   VA_RENDER_NODE   render node to use (default: the mtgpu one)
#   VA_DRIVERS_PATH  directory holding the VA driver (default /usr/lib64/dri)
#   MUSA_RUNTIME_LIB MUSA runtime library directory (default /usr/local/musa/lib)
#
# Exit status: 0 when at least one route decoded a picture, 1 otherwise.
set -uo pipefail

ROOT="$(cd -- "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
VA_DRIVERS_PATH="${VA_DRIVERS_PATH:-/usr/lib64/dri}"
VA_RENDER_NODE="${VA_RENDER_NODE:-}"
MUSA_RUNTIME_LIB="${MUSA_RUNTIME_LIB:-/usr/local/musa/lib}"
STREAM=""
KEEP=0

for arg in "$@"; do
    case "$arg" in
        --stream) STREAM="${2:-}"; shift 2 || true ;;
        --keep)   KEEP=1 ;;
        -h|--help) sed -n '2,28p' "$0"; exit 0 ;;
        *) echo "unknown argument: $arg" >&2; exit 2 ;;
    esac
done

pass=0; fail=0; skip=0; decoded=0
ok()   { printf '  \033[32mPASS\033[0m  %s\n' "$*"; pass=$((pass+1)); }
bad()  { printf '  \033[31mFAIL\033[0m  %s\n' "$*"; fail=$((fail+1)); }
note() { printf '        %s\n' "$*"; }
skipped() { printf '  \033[33mSKIP\033[0m  %s\n' "$*"; skip=$((skip+1)); }
section() { printf '\n\033[1m== %s ==\033[0m\n' "$*"; }

WORK="$(mktemp -d "${TMPDIR:-/tmp}/test-vdecode.XXXXXX")"
cleanup() { [[ $KEEP == 1 ]] || rm -rf "$WORK"; }
trap cleanup EXIT

section "Preconditions"

if ! lspci -d 1ed5:0102 >/dev/null 2>&1; then
    bad "no MTT S30 on PCI"
    echo; echo "VDECODE_SUMMARY: 0 passed, 1 failed, 0 skipped"
    echo "VDECODE_STATUS: FAIL"; exit 1
fi
ok "MTT S30 present"

if [[ -z "$VA_RENDER_NODE" ]]; then
    for r in /dev/dri/renderD*; do
        [[ -e "$r" ]] || continue
        drv="$(basename "$(readlink -f "/sys/class/drm/$(basename "$r")/device/driver" 2>/dev/null)")"
        [[ "$drv" == mtgpu ]] && { VA_RENDER_NODE="$r"; break; }
    done
fi
if [[ -n "$VA_RENDER_NODE" && -e "$VA_RENDER_NODE" ]]; then
    ok "mtgpu render node $VA_RENDER_NODE"
else
    bad "no mtgpu render node (is the mtgpu driver loaded?)"
    echo; echo "VDECODE_SUMMARY: $pass passed, $fail failed, $skip skipped"
    echo "VDECODE_STATUS: FAIL"; exit 1
fi

# ---------------------------------------------------------------------
section "Test bitstream"

if [[ -z "$STREAM" ]]; then
    # grep -q closes its input early, which trips pipefail with SIGPIPE on
    # the producer. Capture first, match on a here-string.
    encoders=""
    if command -v ffmpeg >/dev/null 2>&1; then
        encoders="$(ffmpeg -hide_banner -encoders 2>/dev/null || true)"
    fi
    if [[ -z "$encoders" ]]; then
        skipped "ffmpeg is not installed; cannot synthesise a bitstream"
    elif ! grep -q h264_vaapi <<<"$encoders"; then
        skipped "ffmpeg has no h264_vaapi encoder; pass --stream FILE"
    else
        STREAM="$WORK/stream.h264"
        if LIBVA_DRIVER_NAME=mtgpu LIBVA_DRIVERS_PATH="$VA_DRIVERS_PATH" \
           timeout 120 ffmpeg -hide_banner -v error -y \
             -vaapi_device "$VA_RENDER_NODE" \
             -f lavfi -i testsrc2=size=320x240:rate=5:duration=0.2 \
             -vf format=nv12,hwupload -c:v h264_vaapi -b:v 300k \
             -frames:v 1 "$STREAM" 2>"$WORK/encode.log"; then
            ok "encoded a 320x240 H.264 stream with the S30 (h264_vaapi)"
            note "$(stat -c %s "$STREAM") bytes"
        else
            bad "could not encode the test stream with h264_vaapi"
            sed 's/^/        /' "$WORK/encode.log" | head -5
        fi
    fi
else
    ok "using the supplied bitstream $STREAM"
fi

if [[ -z "$STREAM" || ! -s "$STREAM" ]]; then
    section "Result"
    echo "  no bitstream available; nothing to decode"
    printf '\n\033[1mVDECODE_SUMMARY:\033[0m %d passed, %d failed, %d skipped\n' "$pass" "$fail" "$skip"
    echo "VDECODE_STATUS: FAIL"; exit 1
fi

# ---------------------------------------------------------------------
section "VA-API route (libva VAEntrypointVLD)"

VADEC="$WORK/vadecode"
if gcc -O2 -Wall -Wextra -Wno-unused-parameter "$ROOT/tests/vadecode.c" \
       -ldl -o "$VADEC" 2>"$WORK/vadecode-build.log"; then
    ok "built tests/vadecode.c"
    for build in "packaged:/usr/lib64/dri" "older-build:$WORK/va1"; do
        label="${build%%:*}"; path="${build#*:}"
        if [[ "$label" == "older-build" ]]; then
            mkdir -p "$path"
            # The older vendor build of the same driver; used only to see
            # whether the newer build's failure is build-specific.
            if [[ -f /usr/lib64/dri/mtgpu_drv_video_1.so ]]; then
                cp /usr/lib64/dri/mtgpu_drv_video_1.so "$path/mtgpu_drv_video.so"
            else
                continue
            fi
        fi
        [[ -d "$path" ]] || continue
        out="$WORK/vadecode-$label.log"
        LIBVA_DRIVER_NAME=mtgpu LIBVA_DRIVERS_PATH="$path" \
            timeout 120 "$VADEC" "$VA_RENDER_NODE" "$STREAM" -y "$WORK/y-$label.raw" \
            >"$out" 2>&1
        rc=$?
        printf '        --- %s ---\n' "$label"
        grep -E 'VADECODE_|SURFACE|LUMA|CRITICAL' "$out" | sed 's/^/        /' | head -12
        if (( rc == 0 )) && grep -q 'VADECODE_STATUS: SUCCESS' "$out"; then
            ok "VA-API hardware decode produced a picture ($label)"
            decoded=1
            if [[ -s "$WORK/y-$label.raw" ]]; then
                ok "decoded luma plane read back ($(( $(stat -c %s "$WORK/y-$label.raw") )) bytes)"
            fi
            break
        else
            note "VA-API decode did not complete with the $label driver (rc=$rc)"
        fi
    done
    if (( pass == 0 )); then
        bad "no VA-API driver build could decode the stream"
        note "root causes: docs/root-cause-video-decode-2026-11-26.md"
    fi
else
    bad "tests/vadecode.c did not build"
    head -5 "$WORK/vadecode-build.log" | sed 's/^/        /'
fi

# ---------------------------------------------------------------------
section "MUSA MUVID route (libvideo_musa.so)"

MUVID="$WORK/muvid-decode"
if [[ -d "$ROOT/tests/vendor/mt-codec-headers/include" ]]; then
    if gcc -O2 -Wall -Wextra -Itests/vendor/mt-codec-headers/include \
           "$ROOT/tests/muvid-decode.c" -ldl -lm -o "$MUVID" \
           2>"$WORK/muvid-build.log"; then
        ok "built tests/muvid-decode.c"
        out="$WORK/muvid.log"
        LIBVA_DRIVER_NAME=mtgpu LIBVA_DRIVERS_PATH="$VA_DRIVERS_PATH" \
            LD_LIBRARY_PATH="$MUSA_RUNTIME_LIB${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" \
            timeout 120 "$MUVID" "$STREAM" -y "$WORK/muvid.y" >"$out" 2>&1
        rc=$?
        grep -E 'LIBRARY|DEVICE|CAPS|SEQUENCE|SUMMARY|MUVID_FAIL|MUVID_STATUS|NOTE' "$out" \
            | sed 's/^/        /' | head -14
        if (( rc == 0 )); then
            ok "MUSA MUVID hardware decode produced a picture"
            decoded=1
        else
            note "MUSA MUVID decode did not complete (rc=$rc)"
            note "root causes: docs/root-cause-video-decode-2026-11-26.md"
        fi
    else
        bad "tests/muvid-decode.c did not build"
        head -5 "$WORK/muvid-build.log" | sed 's/^/        /'
    fi
else
    skipped "the vendored MT codec headers are not present"
fi

# ---------------------------------------------------------------------
section "MUSA MUVID route with the upstream Video Codec SDK library"

# The upstream Video Codec SDK library (libvideo_musa.so.13.0) implements the
# H.264 codec id the shipped runtime rejects, but it belongs to the MUSA 5.x
# generation and is therefore driven here as an isolated comparison: it never
# replaces the installed stack. Point MUSA_UPSTREAM_MUVID_LIB at the library
# and MUSA_UPSTREAM_MUSA_LIB at the MUSA driver API to pair it with.
UP_LIB="${MUSA_UPSTREAM_MUVID_LIB:-}"
UP_MUSA="${MUSA_UPSTREAM_MUSA_LIB:-}"
if [[ -z "$UP_LIB" ]]; then
    skipped "no upstream MUVID library selected (set MUSA_UPSTREAM_MUVID_LIB)"
elif [[ ! -e "$UP_LIB" ]]; then
    bad "MUSA_UPSTREAM_MUVID_LIB does not exist: $UP_LIB"
else
    updir="$WORK/upstream"
    mkdir -p "$updir"
    ln -sf "$UP_LIB" "$updir/libvideo_musa.so.1"
    out="$WORK/muvid-upstream.log"
    LIBVA_DRIVER_NAME=mtgpu LIBVA_DRIVERS_PATH="$VA_DRIVERS_PATH" \
        LD_LIBRARY_PATH="$updir${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" \
        MUVID_LIB="$updir/libvideo_musa.so.1" \
        MUVID_MUSA_LIB="${UP_MUSA:-/usr/local/musa/lib/libmusa.so.1.5.4}" \
        timeout 120 "$MUVID" "$STREAM" -y "$WORK/muvid-up.y" >"$out" 2>&1
    rc=$?
    grep -E 'LIBRARY|CAPS|MUVID_FAIL|MUVID_STATUS|DEVICE' "$out" | sed 's/^/        /' | head -10
    if (( rc == 0 )); then
        ok "upstream MUVID decoded the stream"
        decoded=1
    elif grep -q 'mtdECs=1' "$out"; then
        note "upstream MUVID reports an MT decode engine but could not decode"
        note "it is a MUSA 5.x library: it needs a newer S30 kernel driver"
    else
        note "upstream MUVID did not complete (rc=$rc)"
    fi
fi

# ---------------------------------------------------------------------
section "Kernel log for this run"

kern="$(journalctl -k --since '-3 min' 2>/dev/null | grep -viE 'affinity has changed' \
        | grep -iE 'mtgpu|mtvpu|BUG:|Oops|general protection|invalid opcode' | tail -20 || true)"
if [[ -z "$kern" ]]; then
    ok "no mtgpu/mtvpu kernel message during the run"
else
    printf '%s\n' "$kern" | sed 's/^/        /'
    if grep -qE 'BUG:|Oops|general protection|invalid opcode' <<<"$kern"; then
        bad "kernel fault logged during the decode attempt"
    else
        ok "kernel log carries no fault"
    fi
fi

printf '\n\033[1mVDECODE_SUMMARY:\033[0m %d passed, %d failed, %d skipped\n' "$pass" "$fail" "$skip"
if (( decoded > 0 )); then
    echo "VDECODE_STATUS: PASS"
    exit 0
fi
echo "VDECODE_STATUS: FAIL"
echo "  no hardware decoder completed a picture on this host"
echo "  see docs/root-cause-video-decode-2026-11-26.md"
exit 1

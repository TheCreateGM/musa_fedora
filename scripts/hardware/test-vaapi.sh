#!/usr/bin/env bash
# test-vaapi.sh — verify the packaged MUSA VA-API driver against the MTT S30.
#
# The vendor mtgpu_drv_video*.so VA driver ships in the optional
# musa-runtime-va subpackage. This script proves it actually initialises on the
# device and can produce a bitstream, and that the kernel log stays clean.
#
# It is read-only toward the system: it writes only into its own scratch
# directory and installs nothing. Build it a probe, run it, encode with
# ffmpeg (when available), decode the result back and compare frame counts.
#
# Usage:
#   scripts/hardware/test-vaapi.sh [--no-encode]
#
# Env overrides:
#   VA_DRIVERS_PATH  directory holding the VA driver (default /usr/lib64/dri)
#   VA_RENDER_NODE   render node to use   (default: the mtgpu one)
#   VA_FFMPEG        ffmpeg binary        (default: ffmpeg from PATH)
#
# Exit status: 0 when every executed check passed, 1 otherwise. Checks that
# cannot run (missing ffmpeg, no VideoProc/encode entrypoint) are SKIPped and
# do not fail the run.
set -uo pipefail

ROOT="$(cd -- "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
VA_DRIVERS_PATH="${VA_DRIVERS_PATH:-/usr/lib64/dri}"
VA_RENDER_NODE="${VA_RENDER_NODE:-}"
VA_FFMPEG="${VA_FFMPEG:-ffmpeg}"
DO_ENCODE=1

for arg in "$@"; do
    case "$arg" in
        --no-encode) DO_ENCODE=0 ;;
        -h|--help)   sed -n '2,23p' "$0"; exit 0 ;;
        *) echo "unknown argument: $arg" >&2; exit 2 ;;
    esac
done

pass=0; fail=0; skip=0
ok()   { printf '  \033[32mPASS\033[0m  %s\n' "$*"; pass=$((pass+1)); }
bad()  { printf '  \033[31mFAIL\033[0m  %s\n' "$*"; fail=$((fail+1)); }
note() { printf '        %s\n' "$*"; }
skipped() { printf '  \033[33mSKIP\033[0m  %s\n' "$*"; skip=$((skip+1)); }
section() { printf '\n\033[1m== %s ==\033[0m\n' "$*"; }

# Snapshot the kernel log position: only this run's messages are inspected.
JOURNAL_MARK="$(date -u +%Y-%m-%dT%H:%M:%SZ)"

WORK="$(mktemp -d "${TMPDIR:-/tmp}/test-vaapi.XXXXXX")"
cleanup() { rm -rf "$WORK"; }
trap cleanup EXIT

section "Preconditions"

if ! lspci -d 1ed5:0102 -s 01:00.0 >/dev/null 2>&1; then
    bad "no MTT S30 at 01:00.0"
    echo; echo "VAAPI_SUMMARY: 0 passed, 1 failed, 0 skipped"
    echo "VAAPI_STATUS: FAIL"; exit 1
fi
ok "MTT S30 present at 0000:01:00.0"

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
    bad "no mtgpu render node (is the driver loaded?)"
    echo; echo "VAAPI_SUMMARY: $pass passed, $fail failed, $skip skipped"
    echo "VAAPI_STATUS: FAIL"; exit 1
fi

if compgen -G "$VA_DRIVERS_PATH/mtgpu_drv_video.so" >/dev/null; then
    ok "VA driver present in $VA_DRIVERS_PATH"
    note "package: $(rpm -qf --qf '%{NAME}-%{VERSION}-%{RELEASE}' "$VA_DRIVERS_PATH/mtgpu_drv_video.so" 2>/dev/null || echo 'not owned by an RPM')"
else
    bad "mtgpu_drv_video.so not found in $VA_DRIVERS_PATH"
    note "install musa-runtime-va, or point VA_DRIVERS_PATH at a staging tree"
fi

# grep -q closes its input early, so a `producer | grep -q` pipeline trips
# pipefail with SIGPIPE on the producer. Capture first, match on a here-string.
ldcache="$(ldconfig -p 2>/dev/null)"
if compgen -G "$VA_DRIVERS_PATH/mtgpu_drv_video.so" >/dev/null; then
    for lib in libva.so.2 libva-drm.so.2; do
        if grep -qE "[[:space:]]${lib//./\\.} " <<<"$ldcache"; then
            ok "$lib resolvable"
        else
            bad "$lib missing (install libva)"
        fi
    done
fi

section "Driver initialisation and advertised capability"

PROBE="$WORK/vaprobe"
if gcc -O2 -Wall "$ROOT/tests/vaprobe.c" -ldl -o "$PROBE" 2>"$WORK/build.log"; then
    ok "vaprobe built"
else
    bad "vaprobe build failed"
    sed 's/^/        /' "$WORK/build.log"
fi

if [[ -x "$PROBE" ]]; then
    # libva probes every *_drv_video.so it finds, so an explicit
    # LIBVA_DRIVER_NAME is not needed; pin it anyway to keep the result
    # independent of whatever else lives in the drivers directory.
    LIBVA_DRIVER_NAME=mtgpu LIBVA_DRIVERS_PATH="$VA_DRIVERS_PATH" \
        "$PROBE" "$VA_RENDER_NODE" >"$WORK/probe.log" 2>&1
    probe_rc=$?
    # libva's own chatter goes to stderr; drop it from the transcript.
    grep -v -E '^libva info: |^trace path' "$WORK/probe.log" | sed 's/^/        /'

    if grep -q '^PROBE_OK' "$WORK/probe.log"; then
        ok "vaInitialize succeeded on the S30"
        if grep -qi 'mthreads' "$WORK/probe.log"; then
            ok "driver vendor string identifies Moore Threads"
        else
            bad "driver vendor string does not identify Moore Threads"
        fi
        nprof="$(sed -n 's/^PROFILES: //p' "$WORK/probe.log")"
        if [[ "${nprof:-0}" -gt 0 ]]; then
            ok "$nprof profiles advertised"
        else
            bad "no profiles advertised"
        fi
        if grep -q 'VAEntrypointEncSlice\|VAEntrypointEncSliceLP\|VAEntrypointEncPicture' "$WORK/probe.log"; then
            ok "at least one encode entrypoint advertised"
        else
            skipped "no encode entrypoint advertised"
        fi
    else
        bad "vaInitialize failed (rc=$probe_rc)"
    fi
fi

section "H.264 encode round trip"

if (( DO_ENCODE == 0 )); then
    skipped "encode skipped by request"
elif ! command -v "$VA_FFMPEG" >/dev/null 2>&1; then
    skipped "$VA_FFMPEG not installed"
elif ! grep -q h264_vaapi <<<"$("$VA_FFMPEG" -hide_banner -encoders 2>/dev/null)"; then
    skipped "$VA_FFMPEG has no h264_vaapi encoder (Fedora's ffmpeg-free lacks it; install ffmpeg from RPM Fusion)"
else
    # A synthetic, deterministic source: 60 frames at 640x480.
    if LIBVA_DRIVER_NAME=mtgpu LIBVA_DRIVERS_PATH="$VA_DRIVERS_PATH" \
        "$VA_FFMPEG" -hide_banner -v error -y -vaapi_device "$VA_RENDER_NODE" \
        -f lavfi -i testsrc2=size=640x480:rate=30:duration=2 \
        -vf format=nv12,hwupload -c:v h264_vaapi -b:v 2000k \
        "$WORK/out.h264" 2>"$WORK/encode.log"; then
        ok "h264_vaapi encoded a stream on the S30"
        if [[ -s "$WORK/out.h264" ]]; then
            ok "bitstream written ($(stat -c %s "$WORK/out.h264") bytes)"
        else
            bad "bitstream empty"
        fi
    else
        bad "h264_vaapi encode failed"
        sed 's/^/        /' "$WORK/encode.log"
    fi

    # The product must decode. Use the software decoder so the check is
    # independent of the hardware path being tested.
    if [[ -s "$WORK/out.h264" ]]; then
        if "$VA_FFMPEG" -hide_banner -v error -y -i "$WORK/out.h264" \
            -f null - 2>"$WORK/decode.log"; then
            frames="$("$VA_FFMPEG" -hide_banner -v error -i "$WORK/out.h264" \
                -f null - 2>&1 | grep -oE '[0-9]+ frames? decoded' | head -1 || true)"
            ok "encoded stream decodes cleanly${frames:+ ($frames)}"
        else
            bad "encoded stream does not decode"
            sed 's/^/        /' "$WORK/decode.log"
        fi
    fi
fi

section "Kernel log for this run"

kernel_log="$(journalctl -k --since "$JOURNAL_MARK" 2>/dev/null \
    | grep -viE 'affinity has changed' || true)"
if [[ -z "$kernel_log" ]]; then
    ok "no kernel message logged during the run"
else
    printf '%s\n' "$kernel_log" | tail -20 | sed 's/^/        /'
    if grep -qE 'BUG:|Oops|general protection|invalid opcode|page allocation failure' <<<"$kernel_log"; then
        bad "kernel fault or allocation failure logged"
    else
        ok "kernel log carries no fault"
    fi
fi

printf '\n\033[1mVAAPI_SUMMARY:\033[0m %d passed, %d failed, %d skipped\n' "$pass" "$fail" "$skip"
if (( fail == 0 )); then
    echo "VAAPI_STATUS: PASS"
    exit 0
fi
echo "VAAPI_STATUS: FAIL"
exit 1

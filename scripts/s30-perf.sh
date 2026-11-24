#!/usr/bin/env bash
# s30-perf.sh — build and run the MTT S30 performance benchmark, then report
# the platform facts that actually bound performance on this host.
#
# This is a read-only measurement tool: it loads no modules, changes no
# system configuration, and writes only into its own scratch directory.
#
# Usage:
#   scripts/s30-perf.sh              build + run the benchmark
#   scripts/s30-perf.sh --no-build   run the binary in the persistent build
#                                     directory (build it once first)
#   scripts/s30-perf.sh --keep       keep the scratch directory on exit
#
# The benchmark binary is built into $BUILD_DIR (default .tmp/s30-perf) so
# that --no-build can actually reuse it; the run itself always happens in a
# fresh scratch directory that is removed on exit.
#
# Exit status: the benchmark's PERF_STATUS (0 = PASS).
set -uo pipefail

ROOT="$(cd -- "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
MUSA_RT_LIB="${MUSA_RT_LIB:-/usr/local/musa/lib}"
MUSA_SDK="${MUSA_SDK:-/usr/local/musa-5.2}"

BUILD_DIR="${BUILD_DIR:-$ROOT/.tmp/s30-perf}"
DO_BUILD=1
KEEP=0
for arg in "$@"; do
    case "$arg" in
        --no-build) DO_BUILD=0 ;;
        --keep)     KEEP=1 ;;
        -h|--help)  sed -n '2,18p' "$0"; exit 0 ;;
        *) echo "unknown argument: $arg" >&2; exit 2 ;;
    esac
done

WORK="$(mktemp -d "${TMPDIR:-/tmp}/s30-perf.XXXXXX")"
cleanup() { [[ $KEEP -eq 1 ]] && { echo "scratch kept at $WORK"; } || rm -rf "$WORK"; }
trap cleanup EXIT

export LD_LIBRARY_PATH="${MUSA_RT_LIB}${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"

echo "=== MTT S30 performance benchmark ==="
echo "host kernel : $(uname -r)"
echo "device      : $(lspci -d 1ed5: -s 01:00.0 2>/dev/null | cut -c1-70)"
link_speed="$(cat /sys/bus/pci/devices/0000:01:00.0/current_link_speed 2>/dev/null || echo unknown)"
link_width="$(cat /sys/bus/pci/devices/0000:01:00.0/current_link_width 2>/dev/null || echo 0)"
echo "pcie link   : ${link_speed} x${link_width}"
echo "scratch     : $WORK"
echo

if [[ $DO_BUILD -eq 1 ]]; then
    echo "--- building musa-perf (mp_10) ---"
    mkdir -p "$BUILD_DIR"
    if ! "$ROOT/scripts/hardware/build-test-binaries.sh" "$BUILD_DIR" >"$WORK/build.log" 2>&1; then
        echo "BUILD FAILED; last lines:"
        tail -30 "$WORK/build.log"
        echo "PERF_STATUS: FAIL (build)"
        exit 1
    fi
    grep -E "musa-perf|BUILD_STATUS" "$WORK/build.log" | sed 's/^/  /'
    echo
fi

BIN="$BUILD_DIR/musa-perf"
if [[ ! -x "$BIN" ]]; then
    echo "FATAL: $BIN not found; run scripts/s30-perf.sh without --no-build first"
    exit 2
fi

# Snapshot the kernel log position so only this run's messages are reported.
JOURNAL_MARK="$(date -u +%Y-%m-%dT%H:%M:%SZ 2>/dev/null || true)"

echo "--- running benchmark ---"
"$BIN" 2>&1 | tee "$WORK/run.log"
rc=${PIPESTATUS[0]}
echo

# A benchmark run must not leave kernel warnings behind. Warn loudly but do
# not mask the benchmark's own verdict.
echo "--- kernel messages during the run ---"
if journalctl -k --since "$JOURNAL_MARK" >/dev/null 2>&1; then
    kern="$(journalctl -k --since "$JOURNAL_MARK" --no-pager 2>/dev/null \
        | grep -iE "page allocation failure|warn_alloc|mtgpu|oom-kill|BUG:|Oops" \
        | grep -v "affinity has changed" || true)"
    if [[ -n "$kern" ]]; then
        printf '%s\n' "$kern" | sed 's/^/  /'
        echo "  NOTE: see docs/ for known non-fatal mtgpu message patterns."
    else
        echo "  none (no new mtgpu fault, allocation failure or Oops)"
    fi
fi
echo

if [[ $rc -eq 0 ]]; then
    echo "PERF_RUN: PASS (benchmark exit 0)"
else
    echo "PERF_RUN: FAIL (benchmark exit $rc)"
fi
exit "$rc"

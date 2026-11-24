#!/usr/bin/env bash
# check-musa-env.sh — verify the system-wide MUSA environment ordering.
#
# The MTT S30 runs the MUSA 1.5.4 runtime (/usr/local/musa) with the mtgpu
# 3.1.0 driver. The 5.2.0 prefix (/usr/local/musa-5.2) only provides the mcc
# compiler. If a login shell puts the 5.2 prefix first, an unqualified
# `musaInfo` loads libmusart.so.5 and aborts against the 3.1.0 driver with
# "Invalid cmd_type: 1 or cmd: 12".
#
# This check is static: it inspects PATH/LD_LIBRARY_PATH ordering and ELF
# NEEDED entries. It never executes musaInfo and never opens the GPU.
#
# Usage:
#   scripts/check-musa-env.sh
#   scripts/check-musa-env.sh --quarantine-orphans --plan
#   scripts/check-musa-env.sh --quarantine-orphans --apply   # uses pkexec
#
# Exit status: 0 environment PASS, 1 FAIL, 2 usage error.
set -uo pipefail

RUNTIME_PREFIX=/usr/local/musa
SDK_PREFIX=/usr/local/musa-5.2
PROFILE_ZZ=/etc/profile.d/zz-musa-fedora.sh
ORPHAN=/etc/profile.d/musa.sh
BACKUP_DIR=/var/backups/musa-fedora/orphans

FAIL=0
pass() { printf '  PASS  %s\n' "$*"; }
fail() { printf '  FAIL  %s\n' "$*"; FAIL=1; }
info() { printf '  INFO  %s\n' "$*"; }
warn() { printf '  WARN  %s\n' "$*"; }

QUARANTINE=0
APPLY=0
while (($#)); do
    case "$1" in
        --quarantine-orphans) QUARANTINE=1 ;;
        --plan) APPLY=0 ;;
        --apply) APPLY=1 ;;
        -h|--help) sed -n '2,22p' "${BASH_SOURCE[0]}"; exit 0 ;;
        *) echo "unknown option: $1" >&2; exit 2 ;;
    esac
    shift
done

resolve_in_list() {
    # resolve_in_list <name> <colon list> -> first executable, or nothing.
    local name=$1 list=$2 dir
    local IFS=:
    for dir in $list; do
        [[ -n "$dir" ]] || continue
        if [[ -x "$dir/$name" ]]; then
            printf '%s\n' "$dir/$name"
            return 0
        fi
    done
    return 1
}

pkg_owner() {
    # pkg_owner <path> -> package name if owned, return 1 if unowned.
    # rpm -qf prints "file ... is not owned by any package" on stdout and
    # exits non-zero, so the exit status is the reliable signal.
    local path=$1
    rpm -qf "$path" >/dev/null 2>&1 || return 1
    rpm -qf --qf '%{NAME}\n' "$path" 2>/dev/null
}

echo "=== MUSA environment ordering check ==="

# Reproduce what a login shell sees. /etc/profile sources every
# /etc/profile.d/*.sh in glob order, so a plain `bash -lc` is the real test.
_env="$(bash -lc 'printf "MUSAENV_PATH=%s\nMUSAENV_LDLP=%s\n" "$PATH" "${LD_LIBRARY_PATH:-}"' 2>/dev/null)"
SIM_PATH="$(printf '%s\n' "$_env" | sed -n 's/^MUSAENV_PATH=//p' | head -1)"
SIM_LDLP="$(printf '%s\n' "$_env" | sed -n 's/^MUSAENV_LDLP=//p' | head -1)"
[[ -n "$SIM_PATH" ]] || fail "could not read a login-shell PATH"

info "login PATH: ${SIM_PATH:-<empty>}"
info "login LD_LIBRARY_PATH: ${SIM_LDLP:-<empty>}"

INFO_BIN="$(resolve_in_list musaInfo "${SIM_PATH:-}" || true)"
if [[ "$INFO_BIN" == "$RUNTIME_PREFIX/bin/musaInfo" ]]; then
    pass "musaInfo resolves to the 1.5.4 runtime ($INFO_BIN)"
else
    fail "musaInfo resolves to '${INFO_BIN:-<none>}', expected $RUNTIME_PREFIX/bin/musaInfo"
fi

if [[ -n "$INFO_BIN" && -r "$INFO_BIN" ]]; then
    NEEDED="$(readelf -d "$INFO_BIN" 2>/dev/null |
        grep -o 'libmusart\.so\.[0-9.]*' | head -1)"
    case "$NEEDED" in
        libmusart.so.1.5*) pass "musaInfo links $NEEDED" ;;
        '') fail "cannot read musaInfo NEEDED entries ($INFO_BIN)" ;;
        *) fail "musaInfo links '$NEEDED', expected libmusart.so.1.5*" ;;
    esac
fi

MCC_BIN="$(resolve_in_list mcc "${SIM_PATH:-}" || true)"
if [[ "$MCC_BIN" == "$SDK_PREFIX/bin/mcc" ]]; then
    pass "mcc resolves to the 5.2.0 compiler ($MCC_BIN)"
else
    fail "mcc resolves to '${MCC_BIN:-<none>}', expected $SDK_PREFIX/bin/mcc"
fi

LDLP_FIRST="${SIM_LDLP%%:*}"
if [[ "$LDLP_FIRST" == "$RUNTIME_PREFIX/lib" ]]; then
    pass "LD_LIBRARY_PATH puts the 1.5.4 runtime first"
else
    fail "LD_LIBRARY_PATH first entry is '${LDLP_FIRST:-<empty>}', expected $RUNTIME_PREFIX/lib"
fi

# The 5.2 library directory must never precede the runtime directory.
RT_IDX=-1
SDK_IDX=-1
idx=0
IFS=:
for d in $SIM_LDLP; do
    [[ "$d" == "$RUNTIME_PREFIX/lib" && $RT_IDX -lt 0 ]] && RT_IDX=$idx
    [[ "$d" == "$SDK_PREFIX/lib" && $SDK_IDX -lt 0 ]] && SDK_IDX=$idx
    idx=$((idx + 1))
done
unset IFS
if [[ $RT_IDX -lt 0 ]]; then
    fail "the 1.5.4 runtime lib dir is absent from LD_LIBRARY_PATH"
elif [[ $SDK_IDX -ge 0 && $SDK_IDX -lt $RT_IDX ]]; then
    fail "the 5.2 library directory precedes the 1.5.4 runtime in LD_LIBRARY_PATH"
else
    pass "the 5.2 library directory does not precede the 1.5.4 runtime"
fi

if command -v rpm >/dev/null 2>&1; then
    if ZZ_OWNER="$(pkg_owner "$PROFILE_ZZ")"; then
        if [[ "$ZZ_OWNER" == musa-runtime ]]; then
            pass "$PROFILE_ZZ is owned by $ZZ_OWNER"
        else
            fail "$PROFILE_ZZ is owned by '$ZZ_OWNER', expected musa-runtime"
        fi
    else
        fail "$PROFILE_ZZ is missing; install musa-runtime >= 1.5.4-5"
    fi
fi

# Report (do not silently ignore) the unmanaged legacy profile snippet.
if [[ -e "$ORPHAN" ]]; then
    if ORPHAN_OWNER="$(pkg_owner "$ORPHAN")"; then
        info "$ORPHAN is owned by $ORPHAN_OWNER"
    elif grep -q 'musa-5.2' "$ORPHAN" 2>/dev/null; then
        warn "unmanaged $ORPHAN points at the 5.2 prefix"
        warn "  $PROFILE_ZZ overrides it; quarantine with:"
        warn "  $0 --quarantine-orphans --plan"
    else
        info "$ORPHAN is unowned and is not a 5.2 snippet"
    fi
fi

if ((QUARANTINE)); then
    echo
    echo "=== orphan quarantine ==="
    if [[ ! -e "$ORPHAN" ]]; then
        info "no $ORPHAN to quarantine"
    else
        if ORPHAN_OWNER="$(pkg_owner "$ORPHAN")"; then
            fail "refusing: $ORPHAN is owned by $ORPHAN_OWNER"
        elif ! grep -q 'musa-5.2' "$ORPHAN" 2>/dev/null; then
            fail "refusing: $ORPHAN does not reference the 5.2 prefix"
        else
            ts="$(date -u +%Y%m%dT%H%M%SZ)"
            dest="$BACKUP_DIR/musa.sh.$ts"
            if ((APPLY)); then
                if pkexec /bin/sh -c \
                    'mkdir -p "$1" && mv "$2" "$3" && chmod 0644 "$3"' \
                    sh "$BACKUP_DIR" "$ORPHAN" "$dest"; then
                    info "quarantined $ORPHAN -> $dest"
                else
                    fail "pkexec could not quarantine $ORPHAN"
                fi
            else
                info "PLAN: would move $ORPHAN -> $dest (no change made)"
            fi
        fi
    fi
fi

echo
if ((FAIL)); then
    echo "MUSA_ENV_STATUS: FAIL"
    exit 1
fi
echo "MUSA_ENV_STATUS: PASS"

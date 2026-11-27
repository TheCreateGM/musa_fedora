#!/usr/bin/env bash
# rpm-signing.sh — create, apply and verify OpenPGP signatures for the Fedora
# MUSA package set.
#
# The local builds are unsigned by default and Copr signs what it publishes.
# This script provides the missing half of that story: a reproducible way to
# sign a locally built set with a dedicated key and to verify a set against a
# public key without touching the system RPM keyring.
#
# Sub-commands:
#   keygen      create (or reuse) the local build-signing key
#   fingerprint print the fingerprint of the local build-signing key
#   export      write the armored public key into packaging/rpm/keys/
#   sign        sign every RPM below --rpm-dir
#   verify      verify RPM signatures against a public key
#   check       report the signing state of the local tree without changing it
#
# Nothing here requires root: the private key stays in the caller's GnuPG
# home and verification uses a private rpmkeys database below a temporary
# directory. No system RPM keyring, package database, or module is touched.
#
# Usage: scripts/rpm-signing.sh <sub-command> [options]
set -euo pipefail

ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
RPM_DIR="${RPM_DIR:-$ROOT/packaging/rpm/local-build/RPMS}"
KEY_DIR="${KEY_DIR:-$ROOT/packaging/rpm/keys}"
GNUPGHOME="${GNUPGHOME:-$HOME/.gnupg}"
export GNUPGHOME

KEY_UID="${MUSA_SIGNING_UID:-MUSA Fedora Local Signing Key <musa@fedoraproject.org>}"
KEY_TYPE="${MUSA_SIGNING_KEY_TYPE:-rsa4096}"
VERIFY_TMP=""

# gpg-agent needs a tty for its pinentry; point it at the current one when
# the script runs interactively.
[[ -t 0 ]] && export GPG_TTY="$(tty)"

usage() {
    cat <<'EOF'
Usage: scripts/rpm-signing.sh <sub-command> [options]

Sub-commands:
  keygen [--uid UID]            create or reuse the local signing key
  fingerprint                   print the local signing key fingerprint
  export [--out FILE]           export the armored public key
  sign [--rpm-dir DIR]          sign all RPMs below DIR
  verify [--rpm-dir DIR] [--key FILE]
                                 verify RPM signatures with a public key
  check [--rpm-dir DIR]         report the signing state (read-only)

Options:
  --rpm-dir DIR   directory searched recursively for *.rpm
  --key FILE      armored public key for verification (default: exported key)
  --out FILE      output file for `export`
  --uid UID       user ID of the signing key
  --force         sign again even if the RPM already carries a signature
  -h, --help      show this help
EOF
}

log()  { printf '%s\n' "$*" >&2; }
fail() { printf 'ERROR: %s\n' "$*" >&2; exit 1; }

need() {
    command -v "$1" >/dev/null 2>&1 || fail "required tool not found: $1"
}

# Locate the signing key: prefer an explicit --uid match. Otherwise use the
# keyring only when it holds exactly one secret key, so that an unrelated
# personal key is never used to sign packages by accident.
signing_fingerprint() {
    local uid="${1:-$KEY_UID}" fpr count
    fpr="$(gpg --batch --list-secret-keys --with-colons "$uid" 2>/dev/null |
           awk -F: '/^fpr:/ {print $10; exit}')"
    if [[ -z "$fpr" ]]; then
        count="$(gpg --batch --list-secret-keys --with-colons 2>/dev/null |
                 awk -F: '/^sec:/ {n++} END {print n+0}')"
        if [[ "$count" == "1" ]]; then
            fpr="$(gpg --batch --list-secret-keys --with-colons 2>/dev/null |
                   awk -F: '/^fpr:/ {print $10; exit}')"
        fi
    fi
    printf '%s' "$fpr"
}

do_keygen() {
    need gpg
    mkdir -p "$GNUPGHOME"
    chmod 700 "$GNUPGHOME"
    local fpr
    fpr="$(signing_fingerprint "$KEY_UID")"
    if [[ -n "$fpr" ]]; then
        log "Reusing existing signing key $fpr ($KEY_UID)"
        printf 'SIGNING_KEY_FINGERPRINT: %s\n' "$fpr"
        return 0
    fi
    log "Creating $KEY_TYPE signing key: $KEY_UID"
    # --batch alone is NOT enough on GnuPG 2.4: the agent's pinentry can
    # still pop a passphrase/confirm dialog and a headless build then dies
    # with "signing failed: Timeout". Forcing an empty loopback passphrase
    # stores the key genuinely unprotected, so every later --addsign is
    # silent. (2026-11-27: the original key was created interactively and
    # acquired a passphrase despite this script's claim; it was rotated.)
    gpg --batch --pinentry-mode loopback --passphrase "" \
        --quick-generate-key "$KEY_UID" "$KEY_TYPE" sign never >&2
    fpr="$(signing_fingerprint "$KEY_UID")"
    [[ -n "$fpr" ]] || fail "key generation did not produce a usable key"
    mkdir -p "$KEY_DIR"
    chmod 755 "$KEY_DIR"
    printf '%s\n' "$fpr" >"$KEY_DIR/signing-key.fpr"
    # A copy of the revocation certificate is worth keeping next to the key.
    local rev
    rev="$(find "$GNUPGHOME/openpgp-revocs.d" -name '*.rev' -newermt '-1 day' 2>/dev/null | head -1)"
    [[ -n "$rev" ]] && cp -a "$rev" "$KEY_DIR/"
    log "Wrote fingerprint to $KEY_DIR/signing-key.fpr"
    printf 'SIGNING_KEY_FINGERPRINT: %s\n' "$fpr"
}

do_fingerprint() {
    need gpg
    local fpr
    fpr="$(signing_fingerprint "${1:-$KEY_UID}")"
    [[ -n "$fpr" ]] || fail "no signing key found (run: $0 keygen)"
    printf '%s\n' "$fpr"
}

do_export() {
    need gpg
    local out="${1:-$KEY_DIR/musa-fedora-signing-key.asc}" fpr
    fpr="$(signing_fingerprint "${1:-$KEY_UID}")"
    [[ -n "$fpr" ]] || fail "no signing key found (run: $0 keygen)"
    mkdir -p "$(dirname "$out")"
    gpg --batch --armor --export "$fpr" >"$out"
    printf '%s\n' "$fpr" >"$KEY_DIR/signing-key.fpr"
    log "Exported public key to $out"
    printf 'EXPORTED_KEY: %s\n' "$out"
}

collect_rpms() {
    [[ -d "$RPM_DIR" ]] || fail "RPM directory not found: $RPM_DIR" \
        " (build the set with packaging/rpm/build-all.sh)"
    find "$RPM_DIR" -type f -name '*.rpm' ! -name '*.unsigned' -print | sort
}

# An RPM carries a signature when rpm reports a header signature with a key
# ID. Unsigned packages only ever list NOTFOUND signature headers.
# `rpm -Kv` exits non-zero when the key is unknown to the system keyring, so
# its status is discarded deliberately.
rpm_is_signed() {
    local out
    out="$(rpm -Kv "$1" 2>/dev/null || true)"
    grep -qE 'signature, key ID' <<<"$out"
}

do_sign() {
    need rpmsign
    need gpg
    local fpr rpm count=0
    fpr="$(signing_fingerprint "$KEY_UID")"
    [[ -n "$fpr" ]] || fail "no signing key found (run: $0 keygen)"
    while IFS= read -r rpm; do
        if rpm_is_signed "$rpm"; then
            if ((FORCE == 0)); then
                log "SKIP (already signed): $(basename "$rpm")"
                continue
            fi
            rpmsign --define "_gpg_name $fpr" --resign "$rpm" >&2
            log "RESIGNED: $(basename "$rpm")"
            ((count+=1))
            continue
        fi
        [[ -f "$rpm.unsigned" ]] || cp -a "$rpm" "$rpm.unsigned"
        rpmsign --define "_gpg_name $fpr" --addsign "$rpm" >&2
        log "SIGNED: $(basename "$rpm")"
        ((count+=1))
    done < <(collect_rpms)
    printf 'SIGNED_PACKAGES: %d\n' "$count"
    printf 'SIGNING_KEY_FINGERPRINT: %s\n' "$fpr"
}

do_verify() {
    need rpmkeys
    local keyfile="${1:-}" fpr count=0 failed=0
    VERIFY_TMP="$(mktemp -d "${TMPDIR:-/tmp}/musa-rpmkeys.XXXXXX")"
    trap 'rm -rf "$VERIFY_TMP"' EXIT
    if [[ -z "$keyfile" ]]; then
        keyfile="$KEY_DIR/musa-fedora-signing-key.asc"
        [[ -f "$keyfile" ]] || keyfile="$(ls "$KEY_DIR"/*.asc 2>/dev/null | head -1)"
    fi
    [[ -n "$keyfile" && -f "$keyfile" ]] ||
        fail "no public key available; pass --key FILE or run: $0 export"
    rpmkeys --dbpath "$VERIFY_TMP" --import "$keyfile" >&2
    fpr="$(gpg --batch --with-colons --import-options show-only --import "$keyfile" 2>/dev/null |
           awk -F: '/^fpr:/ {print $10; exit}')"
    log "Verifying against key $fpr"
    while IFS= read -r rpm; do
        if out="$(rpmkeys --dbpath "$VERIFY_TMP" --checksig "$rpm" 2>&1)"; then
            printf 'OK      %s\n' "$(basename "$rpm")"
            ((count+=1))
        else
            printf 'FAILED  %s: %s\n' "$(basename "$rpm")" "$out"
            failed=1
        fi
    done < <(collect_rpms)
    printf 'VERIFIED_PACKAGES: %d\n' "$count"
    ((failed == 0)) || fail "signature verification failed"
}

do_check() {
    local rpm signed=0 total=0
    printf 'rpm-dir: %s\n' "$RPM_DIR"
    printf 'gnupg home: %s\n' "$GNUPGHOME"
    printf 'signing key: %s\n' "$(signing_fingerprint 2>/dev/null || echo none)"
    while IFS= read -r rpm; do
        ((total+=1))
        if rpm_is_signed "$rpm"; then
            ((signed+=1))
        fi
    done < <(collect_rpms)
    printf 'signed packages: %d/%d\n' "$signed" "$total"
}

action="${1:-}"
[[ -n "$action" ]] || { usage >&2; exit 2; }
shift || true
FORCE=0
KEYFILE=""
OUT=""
while (($#)); do
    case "$1" in
        --rpm-dir) RPM_DIR="${2:?--rpm-dir needs a path}"; shift 2 ;;
        --key)     KEYFILE="${2:?--key needs a path}"; shift 2 ;;
        --out)     OUT="${2:?--out needs a path}"; shift 2 ;;
        --uid)     KEY_UID="${2:?--uid needs a value}"; shift 2 ;;
        --force)   FORCE=1; shift ;;
        -h|--help) usage; exit 0 ;;
        *) echo "Unknown option: $1" >&2; usage >&2; exit 2 ;;
    esac
done

case "$action" in
    keygen)     do_keygen ;;
    fingerprint) do_fingerprint ;;
    export)     do_export "$OUT" ;;
    sign)       do_sign ;;
    verify)     do_verify "$KEYFILE" ;;
    check)      do_check ;;
    *) echo "Unknown sub-command: $action" >&2; usage >&2; exit 2 ;;
esac

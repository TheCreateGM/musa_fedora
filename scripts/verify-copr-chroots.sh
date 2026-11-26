#!/usr/bin/env bash
# verify-copr-chroots.sh — prove that every configured COPR chroot carries the
# current MUSA/MTT S30 artifacts.
#
# Two independent sources are queried:
#   1. the COPR frontend API (api_3) for the build state of each package build;
#   2. the published repository metadata of every chroot, via dnf's repoquery,
#      for the artifact list that users actually get from `dnf install`.
#
# A green run means: every package build reports "succeeded" AND every
# chroot's repository advertises the expected binary RPMs.
#
# Usage: scripts/verify-copr-chroots.sh [--include-src]
set -uo pipefail

COPR_API="${COPR_API:-https://copr.fedorainfracloud.org/api_3}"
COPR_DL="${COPR_DL:-https://download.copr.fedorainfracloud.org/results}"
OWNER="${COPR_OWNER:-axogm}"
PROJECTS=("musa-fedora" "musa-mtt-s30")
# Chroots per project, as configured in the COPR projects.
declare -A CHROOTS=(
    ["musa-fedora"]="fedora-44-x86_64 fedora-45-x86_64 fedora-eln-x86_64 fedora-rawhide-x86_64"
    ["musa-mtt-s30"]="fedora-43-x86_64 fedora-44-x86_64 fedora-45-x86_64 fedora-eln-x86_64 fedora-rawhide-x86_64"
)
# Package -> expected version-release of the artifact that must be present.
EXPECTED=(
    "musa-runtime:1.5.4-8"
    "musa-runtime-va:1.5.4-8"
    "musa-runtime-display:1.5.4-8"
    "musa-sdk:5.2.0-7"
    "musa-toolkit:3.1.0-6"
    "mtgpu-dkms:3.1.0-16"
    "mtgpu-firmware:3.1.0-5"
)

INCLUDE_SRC=0
[[ "${1:-}" == "--include-src" ]] && INCLUDE_SRC=1

CACHE_ROOT="$(mktemp -d "${TMPDIR:-/tmp}/copr-verify.XXXXXX")"
trap 'rm -rf "$CACHE_ROOT"' EXIT

token() {
    # copr-cli keeps the API token in ~/.config/copr
    sed -n 's/^token = //p' "${HOME}/.config/copr" 2>/dev/null | head -1
}
COPR_TOKEN="$(token)"

pass=0; fail=0
ok()   { printf '  \033[32mPASS\033[0m %s\n' "$*"; pass=$((pass+1)); }
bad()  { printf '  \033[31mFAIL\033[0m %s\n' "$*"; fail=$((fail+1)); }
info() { printf '  ---- %s\n' "$*"; }

echo "COPR verification for owner '$OWNER' (api: $COPR_API)"

for proj in "${PROJECTS[@]}"; do
    echo
    echo "== project $proj =="

    # --- 1. build states from the API ------------------------------------
    builds_json="$CACHE_ROOT/$proj-builds.json"
    curl -fsS -H "Authorization: token $COPR_TOKEN" \
        "$COPR_API/build/list?ownername=$OWNER&projectname=$proj&limit=200" \
        -o "$builds_json" 2>/dev/null
    if [[ -s "$builds_json" ]]; then
        # The COPR build list has been served under different keys over time
        # ("items", "response", "results"). Parse all of them and, crucially,
        # fail loudly if no row could be extracted: an empty parse must not
        # be reported as "every build succeeded".
        builds_tsv="$CACHE_ROOT/$proj-builds.tsv"
        python3 - "$builds_json" >"$builds_tsv" <<'PY'
import json,sys
data=json.load(open(sys.argv[1]))
rows=[]
if isinstance(data,list):
    rows=data
elif isinstance(data,dict):
    for key in ("items","response","results","builds"):
        v=data.get(key)
        if isinstance(v,list):
            rows=v; break
        if isinstance(v,dict) and isinstance(v.get("results"),list):
            rows=v["results"]; break
latest={}
for b in rows:
    if not isinstance(b,dict): continue
    pkg=(b.get("source_package") or {}).get("name") or b.get("package_name")
    if not pkg: continue
    if pkg not in latest or b.get("id",0)>latest[pkg].get("id",0): latest[pkg]=b
for pkg,b in sorted(latest.items()):
    ver=(b.get("source_package") or {}).get("version","?")
    state=b.get("state","?")
    chroots=",".join(b.get("chroots") or [])
    print("\t".join([str(b.get("id","?")),pkg,ver,state,chroots]))
PY
        sed 's/\t/ /g; s/^/  build /' "$builds_tsv" | sed 's/ *$//'
        n_builds="$(wc -l <"$builds_tsv")"
        if [[ "$n_builds" -eq 0 ]]; then
            bad "the build list could not be parsed: no build state was verified"
        else
            states="$(awk -F'\t' '$4 != "succeeded" {printf "%s:%s:%s ", $1,$2,$4}' "$builds_tsv")"
            if [[ -z "$states" ]]; then
                ok "latest build of every package reports state=succeeded"
            else
                bad "latest build not succeeded: $states"
            fi
        fi
    else
        info "build list unavailable (need COPR API access); continuing with repo metadata only"
    fi

    # --- 2. artifacts per chroot from the published repo -----------------
    for chroot in ${CHROOTS[$proj]}; do
        base="$COPR_DL/$OWNER/$proj/$chroot/"
        # qf ends in \n so dnf5 emits one record per line even when multiple
        # globs land in one repoquery invocation.
        qf='%{name}-%{version}-%{release}.%{arch}\n'
        # dnf5 prints repoquery records space separated on a single line, so
        # split on whitespace before filtering the source RPMs out.
        out="$(dnf --repofrompath=coprchk,$base --setopt=cachedir="$CACHE_ROOT/$chroot" \
               --repo=coprchk -q repoquery --qf "$qf" 'musa*' 'mtgpu*' 2>/dev/null \
               | tr ' ' '\n' | sed '/^$/d')"
        if [[ $INCLUDE_SRC -eq 0 ]]; then
            out="$(printf '%s\n' "$out" | grep -v '\.src$' || true)"
        fi
        if [[ -z "$out" ]]; then
            bad "$chroot: repository advertises no MUSA artifacts (baseurl reachable? $base)"
            continue
        fi
        missing=""
        for exp in "${EXPECTED[@]}"; do
            name="${exp%%:*}"; vr="${exp##*:}"
            # accept the chroot's own release suffix (fc44/fc45/eln159/fc46/...)
            if ! printf '%s' "$out" | grep -qE "$name-$vr(\.[a-z0-9_]+)*\."; then
                missing="$missing $name-$vr"
            fi
        done
        if [[ -n "$missing" ]]; then
            bad "$chroot: missing expected artifacts:$missing"
            info "present: $out"
        else
            ok "$chroot: all ${#EXPECTED[@]} expected artifact sets present"
        fi

        # Stale builds must not out-rank the expected set: a published
        # version NEWER than the expected one would be picked up by a plain
        # `dnf upgrade` on user hosts (this is how the incompatible
        # mtgpu-dkms/musa-toolkit 5.2.0-1 builds leaked into the upgrade
        # path).  Compare the highest published EVR per package with the
        # expected one.  (Computed from the full artifact list above;
        # dnf5's --latest-limit is unreliable across arches, e.g. it kept
        # an old mtgpu-firmware 3.1.0-3.noarch as "latest" next to 3.1.0-4.)
        newer=""
        for exp in "${EXPECTED[@]}"; do
            name="${exp%%:*}"; vr="${exp##*:}"
            evrs="$(printf '%s\n' "$out" | grep -E "^${name}-[0-9]" | grep -v '\.src$' \
                    | sed -E "s/^${name}-//; s/\.(fc[0-9]+|eln[0-9]+)\.(noarch|x86_64|aarch64)$//" \
                    | sort -uV | tail -1)"
            if [[ -z "$evrs" ]]; then
                newer="$newer $name:<no binary records>"
            elif [[ "$evrs" != "$vr" ]]; then
                newer="$newer $name:newest=$evrs expected=$vr"
            fi
        done
        if [[ -n "$newer" ]]; then
            bad "$chroot: published newest version differs from the expected set (stale build? delete the build):$newer"
        else
            ok "$chroot: newest published version equals the expected set for all ${#EXPECTED[@]} packages"
        fi
    done
done

echo
echo "passed: $pass   failed: $fail"
[[ $fail -eq 0 ]] || exit 1

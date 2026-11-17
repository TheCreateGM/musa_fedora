#!/usr/bin/env bash
# Poll Copr build states until every selected build reaches a terminal state.
# Usage: scripts/copr-monitor.sh [--project OWNER/PROJECT]
#        [--builds ID1,ID2,...] [--interval SECONDS]
set -euo pipefail

PROJECT="axogm/musa-fedora"
BUILDS_CSV=""
INTERVAL=30

while (($#)); do
  case "$1" in
    --project)
      (($# >= 2)) || { echo "missing value for --project" >&2; exit 2; }
      PROJECT="$2"; shift 2 ;;
    --builds)
      (($# >= 2)) || { echo "missing value for --builds" >&2; exit 2; }
      BUILDS_CSV="$2"; shift 2 ;;
    --interval)
      (($# >= 2)) || { echo "missing value for --interval" >&2; exit 2; }
      INTERVAL="$2"; shift 2 ;;
    -h|--help)
      sed -n '2,4p' "$0"; exit 0 ;;
    *) echo "unknown argument: $1" >&2; exit 2 ;;
  esac
done

[[ "$INTERVAL" =~ ^[0-9]+$ ]] && ((INTERVAL > 0)) || {
  echo "interval must be a positive integer" >&2; exit 2;
}

if [[ -n "$BUILDS_CSV" ]]; then
  IFS=',' read -r -a BUILDS <<< "$BUILDS_CSV"
else
  build_json="$(copr list-builds --output-format json "$PROJECT")"
  mapfile -t BUILDS < <(python3 -c 'import json,sys; print("\n".join(str(b["id"]) for b in json.load(sys.stdin)))' <<< "$build_json")
fi

if ((${#BUILDS[@]} == 0)); then
  echo "No builds found for $PROJECT"
  exit 0
fi

for id in "${BUILDS[@]}"; do
  [[ "$id" =~ ^[0-9]+$ ]] || { echo "invalid build ID: $id" >&2; exit 2; }
done

echo "Monitoring Copr project $PROJECT: ${BUILDS[*]}"
while :; do
  active=0
  failed=0
  for id in "${BUILDS[@]}"; do
    state="$(copr status "$id")"
    state="${state,,}"
    printf '[%s] %s\n' "$id" "$state"
    case "$state" in
      succeeded|failed|canceled) [[ "$state" == succeeded ]] || failed=1 ;;
      *) active=1 ;;
    esac
  done
  ((active == 0)) && exit "$failed"
  sleep "$INTERVAL"
done

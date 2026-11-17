#!/usr/bin/env bash
set -euo pipefail
KVER="${1:-$(uname -r)}"
KERNEL="/boot/vmlinuz-${KVER}"

if [[ ! -r "$KERNEL" ]]; then
    echo "No readable kernel image for ${KVER}: ${KERNEL}" >&2
    exit 2
fi

if ! command -v pkexec >/dev/null || ! command -v grubby >/dev/null; then
    echo "pkexec and grubby are required; no changes made." >&2
    exit 2
fi

INFO="$(pkexec grubby --info="$KERNEL")"
ARGS="$(printf '%s\n' "$INFO" | sed -n 's/^args="\(.*\)"$/\1/p')"
if [[ -z "$ARGS" ]]; then
    echo "Could not read boot arguments for ${KVER}; no changes made." >&2
    exit 2
fi

if ! grep -Eq '(^|[[:space:]])ibt=off([[:space:]]|$)' <<<"$ARGS"; then
    echo "No persistent ibt=off argument found for ${KVER}; no changes made."
    exit 0
fi

echo "Removing only ibt=off from ${KERNEL}."
pkexec grubby --update-kernel="$KERNEL" --remove-args=ibt=off
INFO="$(pkexec grubby --info="$KERNEL")"
ARGS="$(printf '%s\n' "$INFO" | sed -n 's/^args="\(.*\)"$/\1/p')"
if grep -Eq '(^|[[:space:]])ibt=off([[:space:]]|$)' <<<"$ARGS"; then
    echo "ibt=off remains in the boot entry; verify the boot configuration." >&2
    exit 1
fi
echo "Verified ibt=off is absent from the ${KVER} boot entry."

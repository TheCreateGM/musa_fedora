#!/usr/bin/env bash
# List every kernel module tree that contains Moore Threads modules, so the
# running-kernel vs available-module mismatch is visible as raw evidence.
# Read-only.
set -uo pipefail
echo "Running kernel: $(uname -r)"
for ko in $(find /lib/modules -maxdepth 3 -name 'sgpu*.ko' -o -maxdepth 3 -name 'mtgpu*.ko' 2>/dev/null | sort); do
  tree="${ko#/lib/modules/}"; tree="${tree%%/*}"
  vm="$(modinfo -F vermagic "$ko" 2>/dev/null)"
  match="MISMATCH"; [[ "$vm" == *"$tree"* ]] && match="built-for-this-tree"
  [[ "$tree" == "$(uname -r)" ]] && status="AVAILABLE-FOR-RUNNING-KERNEL" || status="wrong-kernel-tree"
  printf '%s\n  tree=%s (%s)\n  vermagic=%s -> %s\n  srcversion=%s\n' \
    "$ko" "$tree" "$status" "$vm" "$match" "$(modinfo -F srcversion "$ko" 2>/dev/null)"
done

#!/usr/bin/env bash
set -euo pipefail

usage() {
    cat <<'EOF'
Usage: scripts/uninstall.sh [--plan|--help]

Removal is disabled because this project has no approved installation
manifest. It will not remove unowned modules, firmware, SDK files, or
libraries, and it does not elevate privileges or change system state.
EOF
}

case "${1:-}" in
    --help|-h)
        usage
        exit 0
        ;;
    --plan)
        usage
        echo "UNINSTALL_STATUS: BLOCKED"
        echo "No system changes were made."
        exit 0
        ;;
    "")
        echo "ERROR: removal is not approved without an installation manifest; no system changes were made." >&2
        echo "Review package ownership and docs/live-host-recheck-2026-11-13.md." >&2
        exit 3
        ;;
    *)
        usage >&2
        exit 2
        ;;
esac

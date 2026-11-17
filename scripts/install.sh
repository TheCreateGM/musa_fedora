#!/usr/bin/env bash
set -euo pipefail

usage() {
    cat <<'EOF'
Usage: scripts/install.sh [--plan|--help]

Installation is disabled until the S30 driver passes the kernel IBT safety
gate and the vendor qualifies the driver/runtime combination. This script
does not elevate privileges or change system state.
EOF
}

case "${1:-}" in
    --help|-h)
        usage
        exit 0
        ;;
    --plan)
        usage
        echo "INSTALL_STATUS: BLOCKED"
        echo "No system changes were made."
        exit 0
        ;;
    "")
        echo "ERROR: installation is not approved; no system changes were made." >&2
        echo "See docs/live-host-recheck-2026-11-13.md and docs/limitations.md." >&2
        exit 3
        ;;
    *)
        usage >&2
        exit 2
        ;;
esac

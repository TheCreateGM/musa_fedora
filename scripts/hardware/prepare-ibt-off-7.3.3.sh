#!/usr/bin/env bash
set -euo pipefail

echo "Refusing to disable kernel IBT/CET for MTT S30 testing." >&2
echo "The proprietary vendor core has unresolved ENDBR/objtool warnings." >&2
echo "No boot configuration was changed." >&2
exit 3

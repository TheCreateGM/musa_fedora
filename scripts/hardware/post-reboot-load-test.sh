#!/usr/bin/env bash
set -euo pipefail

echo "Refusing to load the unreviewed mtgpu module or run GPU tests." >&2
echo "The module has unresolved vendor-core objtool/IBT warnings and is not" >&2
echo "validated for the target kernel or MTT S30." >&2
exit 3

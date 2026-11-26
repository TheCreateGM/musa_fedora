#!/usr/bin/env bash
# quarantine-5.2-driver-api.sh — retire the unowned MUSA 5.2.0 driver API
# from the system linker path.
#
# WHY
#
# The packaged MUSA runtime (musa-runtime 1.5.4) installs the MUSA driver
# API as
#
#     /usr/local/musa/lib/libmusa.so.1.5.4   SONAME libmusa.so.1.5
#
# and libmusart.so.1.5.4 links exactly that SONAME, so the packaged stack is
# self-consistent. A leftover from the 5.2.0 SDK install, however, sits in
# /usr/lib64:
#
#     /usr/lib64/libmusa.so      -> libmusa.so.1
#     /usr/lib64/libmusa.so.1    -> libmusa.so.5.2.0   SONAME libmusa.so.1
#     /usr/lib64/libmusa.so.5    -> libmusa.so.5.2.0
#     /usr/lib64/libmusa.so.5.2.0
#
# None of those files is owned by an RPM. They are the only provider of the
# SONAME "libmusa.so.1" on the host, so every consumer that asks for that
# name (for example the Moore Threads Video Codec SDK loader, whose
# MUSA_LIBNAME is "libmusa.so.1") gets the 5.2.0 driver API. That library
# speaks a 5.2 UAPI: in muInit() it issues MTGPU_QUERY_CMD_MKT_NAME
# (sub-command 12), which the installed mtgpu 3.1.0 driver does not
# implement and rejects. The consumer then aborts with
#
#     mtgpu_device_initialize: mtgpu_query_marketing_name failed (-22)
#     mtgpu_internal.h:162: update_references: Assertion ... failed.
#
# and the kernel logs
#
#     mtgpu 0000:01:00.0: Invalid cmd_type: 1 or cmd: 12
#
# which further trips scripts/test-gpu.sh's fail-closed gate.
#
# WHAT IT DOES
#
# Moves (never deletes) the four files into
# /var/lib/musa-fedora/quarantine/musa-5.2-driver-api/ and refreshes the
# linker cache. The packaged runtime is untouched. To restore:
#
#     pkexec scripts/quarantine-5.2-driver-api.sh --restore
#
# The 5.2.0 SDK binaries under /usr/local/musa-5.2 keep working in the sense
# that they still load; they simply no longer find a driver API under the
# "libmusa.so.1" name. They were already incompatible with the 3.1.0 driver
# (every one of them issues the rejected query), so nothing that worked
# before stops working.
#
# Usage:
#   pkexec scripts/quarantine-5.2-driver-api.sh            # quarantine
#   pkexec scripts/quarantine-5.2-driver-api.sh --restore  # put them back
set -euo pipefail

DEST=/var/lib/musa-fedora/quarantine/musa-5.2-driver-api
FILES=(libmusa.so libmusa.so.1 libmusa.so.5 libmusa.so.5.2.0)
SRC=/usr/lib64

if [[ "${1:-}" == "--restore" ]]; then
    for f in "${FILES[@]}"; do
        if [[ -e "$DEST/$f" || -L "$DEST/$f" ]]; then
            mv -v "$DEST/$f" "$SRC/$f"
        fi
    done
    ldconfig
    echo "--- restored ---"
    ldconfig -p | grep -E 'libmusa\.so' || true
    exit 0
fi

if [[ $EUID -ne 0 ]]; then
    echo "This script needs root. Run it as:" >&2
    echo "    pkexec $0" >&2
    exit 1
fi

created=0
for f in "${FILES[@]}"; do
    p="$SRC/$f"
    if [[ -e "$p" || -L "$p" ]]; then
        owner=$(rpm -qf "$p" 2>/dev/null || true)
        if [[ "$owner" == *musa-runtime* ]]; then
            echo "REFUSING: $p is owned by $owner" >&2
            exit 1
        fi
        mkdir -p "$DEST"
        mv -v "$p" "$DEST/$f"
        created=1
    fi
done

if (( created == 0 )); then
    echo "Nothing to quarantine: no 5.2.0 driver-API library in $SRC."
else
    echo "Quarantined the unowned MUSA 5.2.0 driver API into $DEST"
fi

ldconfig
echo "--- linker cache now says ---"
ldconfig -p | grep -E 'libmusa\.so' || true
echo
echo "Verify with: scripts/verify-musa.sh"

#!/bin/sh
# objtool-safe.sh — objtool wrapper for the mtgpu module build (Fedora port).
#
# kbuild runs objtool over module objects/links whenever CONFIG_OBJTOOL is set
# (scripts/Makefile.lib: cmd_objtool). On a Fedora system the kernel-devel
# tree can temporarily ship an objtool linked against a binutils snapshot
# SONAME that the distro no longer carries. Observed case: rawhide
# kernel-devel 7.4.0-0.rc3 (2026-11-25) ships
# tools/objtool/objtool linked against libopcodes-2.47.50.20261029.so, while
# rawhide binutils 2.47.50-31 already moved to libopcodes-2.47.50.20261119.so.
# Every out-of-tree module build then dies in objtool with
#   objtool: error while loading shared libraries: libopcodes-...so:
#   cannot open shared object file: No such file or directory
# before any object is processed. The kernel package does not declare the
# missing SONAME as a dependency, so nothing pulls a provider in.
#
# This wrapper keeps the module build usable in that situation without
# changing anything when objtool works:
#
#   * objtool runs normally  -> the real binary is exec'd with the exact
#     arguments kbuild passed, and its exit status is propagated, so stack
#     validation and ORC annotation are identical to an unwrapped build;
#   * objtool cannot start   -> the wrapper logs the reason and exits 0 so
#     the module still links. Modules built this way load and run, but their
#     own code carries no .orc_unwind* metadata, so a kernel oops stack trace
#     may stop at the module boundary instead of unwinding through it.
#     objtool-based checks (IBT/retpoline/uaccess validation) are also
#     skipped for that build; the vendor core object (mtgpu_core.o_binary)
#     cannot be validated by objtool either way.
#
# The wrapper is handed to kbuild as its objtool on the make command line
# (see the top-level Makefile):  make ... objtool=<this script>
# MTGPU_REAL_OBJTOOL must point at the kernel tree's objtool binary.
# Set MTGPU_SKIP_OBJTOOL=1 to skip objtool explicitly (e.g. debugging).

set -u

real="${MTGPU_REAL_OBJTOOL:?objtool-safe.sh: MTGPU_REAL_OBJTOOL is not set}"

case "${MTGPU_SKIP_OBJTOOL:-0}" in
    1|y|Y|yes|YES)
        echo "objtool-safe.sh: MTGPU_SKIP_OBJTOOL set; skipping objtool: $*" >&2
        exit 0
        ;;
esac

# Probe with an invocation whose only realistic failure modes are exec/loader
# failures (a healthy objtool answers with its usage text on stderr and exit
# 129). Classify by message, not exit status.
probe="$("$real" --help 2>&1 >/dev/null)"
case "$probe" in
    *"error while loading shared libraries"*|*"cannot open shared object file"*|*"No such file or directory"*|*"cannot execute"*)
        echo "objtool-safe.sh: kernel objtool cannot start:" >&2
        echo "objtool-safe.sh:   $probe" >&2
        echo "objtool-safe.sh: linking without objtool; module code will lack" >&2
        echo "objtool-safe.sh: ORC unwind metadata and objtool validation." >&2
        exit 0
        ;;
esac

exec "$real" "$@"

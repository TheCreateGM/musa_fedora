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
# This wrapper keeps the module build usable in that situation. Resolution
# order per invocation:
#
#   1. The kernel tree's objtool starts          -> exec it unchanged with the
#      exact arguments kbuild passed and propagate its exit status, so stack
#      validation and ORC annotation are identical to an unwrapped build.
#   2. That objtool cannot start                 -> build a working objtool
#      ONCE from the kernel-devel sources (tools/objtool + tools/build) into a
#      per-kernel cache directory, then exec the rebuilt binary. This
#      restores full ORC unwind generation instead of skipping it. The
#      build is serialised with a mkdir() lock so parallel make invocations
#      share one build; a failed build is remembered for the run so it is not
#      retried on every call.
#   3. Even the rebuilt objtool cannot start     -> log the reason and exit 0
#      so the module still links. Modules built this way load and run, but
#      their own code carries no .orc_unwind* metadata, so a kernel oops
#      stack trace may stop at the module boundary, and objtool-based checks
#      (IBT/retpoline/uaccess validation) are skipped for that build. The
#      vendor core object (mtgpu_core.o_binary) cannot be validated by objtool
#      either way.
#
# Known Fedora packaging gap that makes step 2 necessary: kernel-devel ships
# the objtool sources but omits tools/build/Build (the file that tells
# Makefile.build how to compile fixdep; upstream has it). The staging step
# below synthesises that one-file makefile fragment in a scratch copy of the
# tools tree, then builds out-of-tree with O=. Nothing under /usr/src is
# modified. tools/objtool/sync-check.sh (an upstream git-sync lint) is
# replaced by an exit-0 stub in the scratch copy: it needs a git checkout.
#
# The wrapper is handed to kbuild as its objtool on the make command line
# (see the top-level Makefile):  make ... objtool=<this script>
# MTGPU_REAL_OBJTOOL must point at the kernel tree's objtool binary.
#   MTGPU_KDIR            kernel tree (defaults to the tree containing
#                         MTGPU_REAL_OBJTOOL)
#   MTGPU_OBJTOOL_CACHE   cache dir (default ${TMPDIR:-/tmp}/mtgpu-objtool-<kver>)
#   MTGPU_OBJTOOL_JOBS    parallel jobs for the fallback build (default 4)
# Set MTGPU_SKIP_OBJTOOL=1 to skip objtool explicitly (e.g. debugging).

set -u

real="${MTGPU_REAL_OBJTOOL:?objtool-safe.sh: MTGPU_REAL_OBJTOOL is not set}"

case "${MTGPU_SKIP_OBJTOOL:-0}" in
    1|y|Y|yes|YES)
        echo "objtool-safe.sh: MTGPU_SKIP_OBJTOOL set; skipping objtool: $*" >&2
        exit 0
        ;;
esac

# Classify a probe result by message, not exit status: a healthy objtool
# answers with its usage text on stderr and exit 129. Prints the probe
# output and returns 0 when the binary cannot start; returns 1 otherwise.
broken_probe() {
    _p="$("$1" --help 2>&1 >/dev/null)" || true
    case "$_p" in
        *"error while loading shared libraries"*|*"cannot open shared object file"*|\
        *"cannot execute"*|*"Exec format error"*|*"No such file or directory"*)
            printf '%s\n' "$_p"
            return 0
            ;;
    esac
    return 1
}

# Kernel tree: explicit environment wins, otherwise derive it from
# <kdir>/tools/objtool/objtool.
kdir="${MTGPU_KDIR:-}"
if [ -z "$kdir" ]; then
    _d="$(dirname "$real")"                    # .../tools/objtool
    if [ "$(basename "$_d")" = "objtool" ]; then
        kdir="$(dirname "$(dirname "$_d")")"   # strip tools/objtool
    fi
fi

# Stage the scratch tree and build a working objtool. Runs at most once per
# cache directory (guarded by the caller's mkdir lock).
build_fallback() {
    _cache="$1" _log="$2"
    _tree="$_cache/tree" _out="$_cache/out"

    if [ ! -f "$_cache/.staged" ]; then
        mkdir -p "$_tree/tools" || return 1
        cp -a "$kdir/tools/objtool" "$_tree/tools/objtool" || return 1
        cp -a "$kdir/tools/build"   "$_tree/tools/build"   || return 1
        for _d in lib include arch scripts; do
            [ -d "$kdir/tools/$_d" ] && ln -sfn "$kdir/tools/$_d" "$_tree/tools/$_d"
        done
        # Fedora kernel-devel omits tools/build/Build; upstream content is
        # this one-line obj list for the fixdep host tool.
        if [ ! -f "$_tree/tools/build/Build" ]; then
            printf '%s\n' \
                '# synthesised by objtool-safe.sh (Fedora kernel-devel omits this file)' \
                'fixdep-y := fixdep.o' \
                > "$_tree/tools/build/Build" || return 1
        fi
        # Replace the upstream git-sync lint with a no-op: it needs a git
        # checkout and has no bearing on the compiled tool.
        printf '%s\n' \
            '#!/bin/sh' \
            '# stubbed by objtool-safe.sh: upstream sync lint, needs a git tree' \
            'exit 0' \
            > "$_tree/tools/objtool/sync-check.sh" || return 1
        chmod +x "$_tree/tools/objtool/sync-check.sh" || return 1
        # Fedora kernel-devel ships the pre-generated arch/x86/lib/
        # cpu-feature-names.c (produced from this kernel's cpufeatures.h) but
        # not the generator gen-cpu-feature-names-x86.awk, so make fails on
        # the missing prerequisite. Drop that generation rule from the
        # scratch Build file; the pre-generated .c is copied into OUTPUT
        # below, and special.o still depends on it.
        _gen_c="$kdir/tools/objtool/arch/x86/lib/cpu-feature-names.c"
        if [ -f "$_gen_c" ] && [ -f "$_tree/tools/objtool/arch/x86/Build" ]; then
            awk '/^\$\(OUTPUT\)arch\/x86\/lib\/cpu-feature-names\.c:/{skip=1; next}
                 skip && /^\t/{next}
                 {skip=0; print}' \
                "$_tree/tools/objtool/arch/x86/Build" \
                > "$_tree/tools/objtool/arch/x86/Build.new" || return 1
            mv "$_tree/tools/objtool/arch/x86/Build.new" \
               "$_tree/tools/objtool/arch/x86/Build" || return 1
        fi
        touch "$_cache/.staged" || return 1
    fi

    mkdir -p "$_out" || return 1
    # Provide the shipped generated file that the removed rule used to build.
    _gen_c="$kdir/tools/objtool/arch/x86/lib/cpu-feature-names.c"
    if [ -f "$_gen_c" ]; then
        mkdir -p "$_out/arch/x86/lib" || return 1
        cp -f "$_gen_c" "$_out/arch/x86/lib/cpu-feature-names.c" || return 1
    fi
    # Run the build with a clean environment: kbuild exports srctree,
    # objtree, ARCH, CFLAGS, KBUILD_* and MAKEFLAGS into this wrapper, and a
    # leaked srctree points the tools Makefiles back at the real kernel tree
    # (where the Fedora Build-file gap would break the build again). srctree
    # is re-set explicitly so every tools/Makefile resolves paths into the
    # scratch tree.
    if ! env -i PATH="$PATH" HOME="${HOME:-/root}" LC_ALL=C \
            srctree="$_tree" \
            make -C "$_tree/tools/objtool" O="$_out/" \
                 -j"${MTGPU_OBJTOOL_JOBS:-4}" >"$_log" 2>&1; then
        return 1
    fi
    [ -x "$_out/objtool" ] || return 1
    broken_probe "$_out/objtool" >/dev/null 2>&1 && return 1
    mv "$_out/objtool" "$_cache/objtool" || return 1
    rm -rf "$_out"
    return 0
}

# Find a usable fallback objtool: use the cache when present, build it once
# under a mkdir() lock when missing, wait for a concurrent builder otherwise.
# Prints the path on success; returns 1 when no fallback is available.
ensure_fallback() {
    [ -n "${kdir:-}" ] || return 1
    [ -d "$kdir/tools/objtool" ] || return 1

    _key="$(basename "$kdir")"
    _cache="${MTGPU_OBJTOOL_CACHE:-${TMPDIR:-/tmp}/mtgpu-objtool-$_key}"
    _bin="$_cache/objtool"
    _log="$_cache/build.log"

    if [ -x "$_bin" ]; then
        broken_probe "$_bin" >/dev/null 2>&1 && return 1
        printf '%s\n' "$_bin"
        return 0
    fi
    [ -f "$_cache/failed" ] && return 1
    mkdir -p "$_cache" 2>/dev/null || return 1

    if mkdir "$_cache/lock" 2>/dev/null; then
        if build_fallback "$_cache" "$_log"; then
            rmdir "$_cache/lock" 2>/dev/null || :
            if [ ! -f "$_cache/notified" ]; then
                echo "objtool-safe.sh: prebuilt kernel objtool unusable; rebuilt" \
                     "a working objtool from kernel sources: $_bin" >&2
                touch "$_cache/notified" 2>/dev/null || :
            fi
            printf '%s\n' "$_bin"
            return 0
        fi
        echo "objtool-safe.sh: fallback objtool build FAILED; see $_log" >&2
        touch "$_cache/failed" 2>/dev/null || :
        rmdir "$_cache/lock" 2>/dev/null || :
        return 1
    fi

    # Another invocation is building; wait for it.
    _i=0
    while [ "$_i" -lt 600 ]; do
        [ -f "$_cache/failed" ] && return 1
        if [ -x "$_bin" ]; then
            broken_probe "$_bin" >/dev/null 2>&1 && return 1
            printf '%s\n' "$_bin"
            return 0
        fi
        # Lock vanished without a usable binary: the builder failed or is
        # gone; do not wait the full timeout for a sibling that no longer runs.
        [ -d "$_cache/lock" ] || return 1
        sleep 1
        _i=$((_i + 1))
    done
    return 1
}

probe="$(broken_probe "$real")" || probe=""
if [ -n "$probe" ]; then
    fallback="$(ensure_fallback)" || fallback=""
    if [ -n "$fallback" ] && [ -x "$fallback" ]; then
        exec "$fallback" "$@"
    fi
    echo "objtool-safe.sh: kernel objtool cannot start:" >&2
    echo "objtool-safe.sh:   $probe" >&2
    echo "objtool-safe.sh: linking without objtool; module code will lack" >&2
    echo "objtool-safe.sh: ORC unwind metadata and objtool validation." >&2
    exit 0
fi

exec "$real" "$@"

#!/usr/bin/env python3
"""Generate CUDA->MUSA source-compatibility mapping tables.

Reads the MUSA SDK header set and the MUSA runtime libraries, then emits:
  * musa_cuda_map_runtime.h : #define cudaFoo musaFoo for every runtime name
                              the 1.5.4 library actually exports
  * musa_cuda_map_driver.h  : #define cuFoo MUfoo / CUDA_X MUSA_X for the
                              driver API
  * low-level renaming fixes for header-level version renames whose target
    symbol the installed runtime does not export
  * libcudart / libcuda jump tables (assembly, ABI-identical tail calls)

Nothing is invented: a name is only mapped when the symbol exists in the
runtime library that will be linked.
"""
import argparse
import glob
import os
import re
import subprocess
import sys

API_MARKERS = ("MUSARTAPI", "MUSAAPI")


def exported(lib, prefixes):
    out = subprocess.run(["nm", "-D", "--defined-only", lib],
                         capture_output=True, text=True, check=True).stdout
    syms = set()
    for line in out.splitlines():
        parts = line.split()
        if len(parts) == 3 and parts[1] in ("T", "i", "W"):
            name = parts[2]
            if any(name.startswith(p) for p in prefixes):
                syms.add(name)
    return syms


DRIVER_HEADERS = ("musa.h", "musaTypedefs.h", "muComplex.h")


def parse_headers(include_dir):
    """Collect renames, function declarations, typedefs, enum constants, macros."""
    renames = {}          # base -> versioned target
    functions = {"rt": set(), "drv": set()}
    typedefs = {"rt": set(), "drv": set()}
    enum_consts = {"rt": set(), "drv": set()}
    const_macros = {"rt": set(), "drv": set()}
    vmacros = set()

    def domain(name):
        if name.startswith("musa") or name.startswith("MUSA_"):
            return "rt"
        if name.startswith("mu") or name.startswith("MU_") or name.startswith("MU"):
            return "drv"
        return None

    for path in sorted(glob.glob(os.path.join(include_dir, "*.h"))):
        file_domain = "drv" if os.path.basename(path) in DRIVER_HEADERS else "rt"
        text = open(path, encoding="utf-8", errors="replace").read()
        for line in text.splitlines():
            stripped = line.strip()
            m = re.match(r"#define\s+((?:musa|mu|MU|MUSA)[A-Za-z0-9_]+)\s+((?:musa|mu|MU|MUSA)[A-Za-z0-9_]+)\s*$",
                         stripped)
            if m and m.group(1) != m.group(2):
                renames[m.group(1)] = m.group(2)
                continue
            # per-thread-default-stream wrappers:
            #   #define muFoo __MUSA_API_PTDS(muFoo_v2)
            m = re.match(r"#define\s+((?:musa|mu)[A-Za-z0-9_]+)\s+"
                         r"__MUSA_API_PT[A-Z]*\s*\(\s*((?:musa|mu)[A-Za-z0-9_]+)\s*\)\s*$",
                         stripped)
            if m and m.group(1) != m.group(2):
                renames[m.group(1)] = m.group(2)
                continue
            m = re.match(r"#define\s+((?:MUSA|MU)_[A-Z0-9_]+)\s+((?:MUSA|MU)_[A-Z0-9_]+)\s*$",
                         stripped)
            if m and m.group(1) != m.group(2):
                renames[m.group(1)] = m.group(2)
                continue
            # simple constant macros: #define musaFoo 0x00
            m = re.match(r"#define\s+((?:musa|mu|MU|MUSA)[A-Za-z0-9_]+)\s+([-0-9][0-9A-Fa-fxX() ]*)$",
                         stripped)
            if m:
                const_macros[file_domain].add(m.group(1))
                continue
            m = re.match(r"#define\s+((?:MUSA|MU)_[A-Z0-9_]+)\s+([-0-9][0-9A-Fa-fxX() ]*)$",
                         stripped)
            if m:
                const_macros[file_domain].add(m.group(1))
                continue
            for marker, key in (("MUSARTAPI", "rt"), ("MUSAAPI", "drv")):
                if marker in stripped:
                    m = re.search(r"\b((?:musa|mu|MU|MUSA)[A-Za-z0-9_]+)\s*\(", stripped)
                    if m:
                        functions[key].add(m.group(1))
            m = re.match(r"typedef\s+.*\b((?:musa|mu|MU|MUSA)[A-Za-z0-9_]+)\s*;", stripped)
            if m:
                typedefs[file_domain].add(m.group(1))
            # Closing line of a multi-line typedef: "} musaFoo;" / "} MUresult;"
            m = re.match(r"\}\s*((?:musa|mu|MU|MUSA)[A-Za-z0-9_]+)\s*;", stripped)
            if m:
                typedefs[file_domain].add(m.group(1))
            # Tag declarations: "struct __device_builtin__ musaFoo" / "enum musaBar"
            m = re.match(r"(?:struct|union|enum)\s+(?:__[A-Za-z0-9_]+\s+)*((?:musa|mu|MU|MUSA)[A-Za-z0-9_]+)\b",
                         stripped)
            if m:
                typedefs[file_domain].add(m.group(1))
        # enum constants: identifier = value inside typedef enum blocks
        for block in re.findall(r"\benum\b[^;{]*\{(.*?)\}", text, re.S):
            for name in re.findall(r"^\s*((?:musa|MU|MUSA)[A-Za-z0-9_]+)\s*=", block, re.M):
                enum_consts[file_domain].add(name)
    return renames, functions, typedefs, enum_consts, const_macros


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--include", default="/usr/local/musa-5.2/include")
    ap.add_argument("--old-include", default="/usr/local/musa/include",
                    help="header set shipped with the runtime library; used to "
                         "restore the ABI-correct declaration of a renamed symbol")
    ap.add_argument("--libdir", default="/usr/local/musa/lib")
    ap.add_argument("--out", default=".")
    ap.add_argument("--print-audit", action="store_true")
    args = ap.parse_args()

    rt_syms = exported(os.path.join(args.libdir, "libmusart.so.1.5.4"), ("musa",))
    drv_syms = exported(os.path.join(args.libdir, "libmusa.so.1.5.4"), ("mu",))
    renames, functions, typedefs, enum_consts, const_macros = parse_headers(args.include)

    def effective(name):
        return renames.get(name, name)

    # Header renames whose target is missing but whose base symbol exists must be
    # undone; otherwise every caller of the base name fails to link.
    broken = []
    for lib, syms, key in ((rt_syms, rt_syms, "rt"), (drv_syms, drv_syms, "drv")):
        pass
    for base, target in sorted(renames.items()):
        d = "rt" if base.startswith(("musa", "MUSA_")) else "drv"
        syms = rt_syms if d == "rt" else drv_syms
        if target not in syms and base in syms:
            broken.append((base, target, d))

    broken_bases = {b for b, _, _ in broken}

    def map_name(name, domain):
        """Return the symbol this compat macro should call, or None.

        Types, enum constants and constant macros are compile-time names: they
        are mapped unconditionally. Functions are only mapped when the runtime
        library exports the symbol, so a compat build never produces an
        undefined reference that the vendor runtime cannot satisfy."""
        syms = rt_syms if domain == "rt" else drv_syms
        if name in broken_bases:
            return name                      # base symbol, rename undone
        eff = effective(name)
        if eff in syms:
            return name                      # preprocessor expands the rename
        if name in syms:
            return name
        return None

    def cuda_name(name, domain):
        if name.startswith("musa"):
            return "cuda" + name[len("musa"):]
        if name.startswith("MUSA_"):
            return "CUDA_" + name[len("MUSA_"):]
        if name.startswith("MU_"):
            return "CU_" + name[len("MU_"):]
        if name.startswith("MU"):
            return "CU" + name[len("MU"):]
        return "cu" + name[len("mu"):]

    rt_map = {}
    drv_map = {}
    for domain, out in (("rt", rt_map), ("drv", drv_map)):
        for name in sorted(functions[domain]):
            if map_name(name, domain) is None:
                continue
            cuda = cuda_name(name, domain)
            if cuda != name:
                out[cuda] = name
        for name in sorted(typedefs[domain] | enum_consts[domain] | const_macros[domain]):
            cuda = cuda_name(name, domain)
            if cuda != name:
                out.setdefault(cuda, name)

    runtime_missing = sorted(n for n in functions["rt"] if map_name(n, "rt") is None)
    driver_missing = sorted(n for n in functions["drv"] if map_name(n, "drv") is None)

    if args.print_audit:
        print(f"runtime: declared {len(functions['rt'])} exported {len(rt_syms)}")
        print(f"driver : declared {len(functions['drv'])} exported {len(drv_syms)}")
        print(f"broken version renames: {len(broken)}")
        for b in broken:
            print("  ", b)
        print(f"runtime functions without a 1.5.4 symbol: {len(runtime_missing)}")
        print(f"driver functions without a 1.5.4 symbol: {len(driver_missing)}")
        print(f"mapped cuda names: {len(rt_map)}   mapped cu names: {len(drv_map)}")
        return 0

    os.makedirs(args.out, exist_ok=True)

    def write_map(path, defines, guard, broken_list, missing, header_note):
        with open(path, "w") as fh:
            fh.write(f"/* {header_note}\n"
                     " * GENERATED by gen-cuda-compat.py -- do not edit by hand.\n"
                     f" * MUSA headers: {args.include}\n"
                     f" * runtime libraries: {args.libdir}\n"
                     " */\n"
                     f"#ifndef {guard}\n#define {guard}\n\n")
            if broken_list:
                fh.write("\n/* The SDK header renamed these entry points to a versioned\n"
                         " * symbol that this runtime does not implement. Restore the\n"
                         " * ABI-correct prototype so a caller with the newer argument list\n"
                         " * fails at compile time rather than silently mismatching. */\n")
                fh.write("extern \"C\" {\n")
                for base, target, _d in broken_list:
                    decl = old_declaration(base)
                    if decl:
                        fh.write(f"/* {base} (runtime ABI; {target} unimplemented) */\n"
                                 f"{decl}\n")
                fh.write("}\n")
            fh.write("\n/* CUDA name -> MUSA name. */\n")
            for cuda, musa in sorted(defines.items()):
                fh.write(f"#ifndef {cuda}\n#define {cuda} {musa}\n#endif\n")
            fh.write("\n/* Declared by the SDK headers but not exported by this runtime\n"
                     " * library; guarded so callers can feature-test instead of\n"
                     " * failing at link time. */\n")
            for name in missing:
                g = ("MUSA_COMPAT_HAVE_" + name.upper())
                fh.write(f"/* #undef {g} */\n")
            fh.write("\n#endif\n")
        return path

    def write_undef(path, broken_list, guard, header_note):
        """Header that must be included BEFORE the vendor MUSA headers.

        The SDK headers rename legacy entry points to versioned symbols
        (musaGetDeviceProperties -> musaGetDeviceProperties_v2) that the
        installed 1.5.4 runtime does not export. Undoing the rename before the
        header is parsed makes it declare the symbol that actually exists.
        """
        with open(path, "w") as fh:
            fh.write(f"/* {header_note}\n"
                     " * GENERATED by gen-cuda-compat.py -- do not edit by hand.\n"
                     " * Include BEFORE any vendor MUSA header.\n */\n"
                     f"#ifndef {guard}\n#define {guard}\n\n")
            if not broken_list:
                fh.write("/* no versioned renames need undoing for this runtime. */\n")
            for base, target, _d in broken_list:
                fh.write(f"/* {base}: header expects {target}, runtime exports {base} */\n"
                         f"#ifdef {base}\n#undef {base}\n#endif\n")
            fh.write("\n#endif\n")
        return path

    def old_declaration(name):
        """Pull the runtime-era prototype of `name` from the old header set."""
        pat = re.compile(r"^[^\n]*\b" + re.escape(name) + r"\s*\([^;]*\)\s*;", re.M)
        for path in sorted(glob.glob(os.path.join(args.old_include, "*.h"))):
            text = open(path, encoding="utf-8", errors="replace").read()
            for line_start in (m.start() for m in pat.finditer(text)):
                decl = text[line_start:text.index(";", line_start) + 1]
                if re.match(r"^[^\n]*\b(MUSARTAPI|MUSAAPI)\b", decl):
                    decl = re.sub(r"\s+", " ", decl).strip()
                    return decl.replace("__musart_builtin__ ", "")
        return None

    write_undef(os.path.join(args.out, "musa_cuda_undef_runtime.h"),
                [b for b in broken if b[2] == "rt"], "MUSA_CUDA_UNDEF_RUNTIME_H",
                "Runtime versioned-symbol renames to undo before including MUSA headers")
    write_undef(os.path.join(args.out, "musa_cuda_undef_driver.h"),
                [b for b in broken if b[2] == "drv"], "MUSA_CUDA_UNDEF_DRIVER_H",
                "Driver versioned-symbol renames to undo before including MUSA headers")
    write_map(os.path.join(args.out, "musa_cuda_map_runtime.h"), rt_map,
              "MUSA_CUDA_MAP_RUNTIME_H", [b for b in broken if b[2] == "rt"],
              runtime_missing, "CUDA runtime name mapping for MUSA 1.5.4")
    write_map(os.path.join(args.out, "musa_cuda_map_driver.h"), drv_map,
              "MUSA_CUDA_MAP_DRIVER_H", [b for b in broken if b[2] == "drv"],
              driver_missing, "CUDA driver API name mapping for MUSA 1.5.4")

    # Jump tables: ABI-identical tail calls so -lcudart / -lcuda resolve.
    def write_jumps(path, syms, prefix_in, prefix_out, guard_comment):
        with open(path, "w") as fh:
            fh.write(f"/* {guard_comment}\n * GENERATED -- ABI-identical tail-call jumps.\n */\n")
            fh.write("\t.text\n")
            for s in sorted(syms):
                if not s.startswith(prefix_in):
                    continue
                tgt = prefix_out + s[len(prefix_in):]
                fh.write(f"\t.globl {tgt}\n\t.type {tgt}, @function\n{tgt}:\n"
                         f"\tjmp {s}@PLT\n\t.size {tgt}, .-{tgt}\n")
            fh.write('\t.section .note.GNU-stack,"",@progbits\n')
        return path

    write_jumps(os.path.join(args.out, "cudart_jumps.S"), rt_syms, "musa", "cuda",
                "libcudart shim: cudaX -> musaX")
    write_jumps(os.path.join(args.out, "cuda_jumps.S"), drv_syms, "mu", "cu",
                "libcuda shim: cuX -> muX")
    print(f"wrote mappings to {args.out}: {len(rt_map)} cuda names, {len(drv_map)} cu names")
    return 0


if __name__ == "__main__":
    sys.exit(main())

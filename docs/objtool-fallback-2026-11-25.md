# objtool fallback rebuilt instead of skipped — mtgpu-dkms 3.1.0-16, 2026-11-25

## What was found

While monitoring the published build logs of the 2026-11-25 COPR set (not
just their `succeeded` state), the `fedora-rawhide-x86_64` log of
`mtgpu-dkms` 3.1.0-15 (build
[11388527](https://copr.fedorainfracloud.org/coprs/build/11388527/)) showed
that **objtool never ran there**:

```
objtool-safe.sh: kernel objtool cannot start:
objtool-safe.sh:   /usr/src/kernels/7.4.0-0.rc3.261119g4405c18c95d2.29.fc46.x86_64/tools/objtool/objtool:
                   error while loading shared libraries: libopcodes-2.47.50.20261029.so:
                   cannot open shared object file: No such file or directory
objtool-safe.sh: linking without objtool; module code will lack
objtool-safe.sh: ORC unwind metadata and objtool validation.
```

Three facts, verified from the log and the chroot:

1. rawhide `kernel-devel` 7.4.0-0.rc3 ships a prebuilt `tools/objtool/objtool`
   linked against the binutils snapshot SONAME `libopcodes-2.47.50.20261029`;
2. rawhide `binutils` 2.47.50-31 already provides
   `libopcodes-2.47.50.20261119` (the datestamped SONAME moves with every
   snapshot, and the kernel package declares no dependency on the old one);
3. `binutils` **was** installed in the buildroot — the package is not the
   problem, the datestamped SONAME is.

The wrapper introduced in 3.1.0-14 correctly kept the build alive in that
situation, but "alive" meant *modules linked without `.orc_unwind*`
metadata*: oops stack traces would stop at the module boundary, and
objtool's IBT/retpoline/uaccess validation was skipped for that build.
`Fedora 44/45/ELN` chroots were unaffected (their objtool starts), so this
was a rawhide-only — but real — stability regression risk.

## Why rebuilding objtool from kernel-devel sources needed two extra fixes

The wrapper's fallback compiles `tools/objtool` from the kernel tree. The
**Fedora `kernel-devel` package ships a partially filtered `tools/` tree**,
discovered while making it build:

| Gap | Consequence | Handling in the scratch staging step |
|---|---|---|
| `tools/build/Build` missing (upstream: `fixdep-y := fixdep.o`) | `Makefile.build` includes it with `-include`, gets no `obj-y`, links an empty `fixdep-in.o` archive → `undefined reference to main'` | synthesise the one-line file in the scratch copy |
| `tools/arch/x86/tools/gen-cpu-feature-names-x86.awk` missing, but the **generated** `tools/objtool/arch/x86/lib/cpu-feature-names.c` present | make dies on `No rule to make target '../arch/x86/tools/gen-cpu-feature-names-x86.awk'` | drop the generation rule in the scratch `arch/x86/Build`, pre-copy the shipped generated `.c` into `OUTPUT` (`special.o` still depends on it) |
| `sync-check.sh` is an upstream **git** sync lint | fails outside a git checkout | stub to `exit 0` in the scratch copy only |

Two implementation traps fixed during testing:

* **Environment leakage.** The wrapper runs inside a kbuild invocation, so
  `srctree`, `objtree`, `ARCH`, `KBUILD_*` and `MAKEFLAGS` are exported into
  it. A leaked `srctree` pointed the tools Makefiles back at the *real*
  `tools/build` (with the Fedora gap) and reproduced the `fixdep` failure.
  The fallback build now runs under `env -i` with only `PATH`, `HOME`,
  `LC_ALL` and an explicit `srctree=<scratch tree>`.
* **Lock without parents.** The first version did `mkdir "$cache/lock"`
  where `$cache` did not exist yet, so the lock creation always failed in
  the fresh-cache case and the wrapper waited its full timeout. Now
  `mkdir -p "$cache"` first, and a vanished lock without a usable binary
  ends the wait immediately instead of sleeping 600 s.

Nothing under `/usr/src` is modified: everything happens in
`$TMPDIR/mtgpu-objtool-<kver>/tree` (staged once, `~12 MB` copy of
`tools/`), built out-of-tree with `O=`, guarded by a `mkdir()` lock so the
parallel `make -j` invocations of one build share a single fallback build
(telemetry: the build itself takes ~2 s). A failed fallback build is cached
for the run so it degrades to the old warn-and-continue path instead of
retrying on every objtool call.

## Resolution order (scripts/objtool-safe.sh)

1. kernel tree's objtool starts → `exec` it unchanged (exit status
   propagated) — identical to an unwrapped build;
2. it cannot start → build a working objtool from the kernel sources into
   the per-kernel cache, then `exec` the rebuilt binary;
3. the rebuild also fails → log loudly and exit 0, module links without ORC
   (previous behaviour, kept as last resort; `MTGPU_SKIP_OBJTOOL=1` still
   forces it).

## Evidence

### 1. Wrapper unit checks (host, simulated broken prebuilt objtool)

```
$ MTGPU_REAL_OBJTOOL=/tmp/broken-objtool MTGPU_OBJTOOL_CACHE=/tmp/otcache sh scripts/objtool-safe.sh --help
objtool-safe.sh: prebuilt kernel objtool unusable; rebuilt a working objtool from kernel sources: /tmp/otcache/objtool

 Usage: objtool <actions> [<options>] file.o
...
```

Second invocation: cache hit (0 s, no rebuild). Healthy prebuilt: identical
to passthrough. `MTGPU_SKIP_OBJTOOL=1`: skip message, exit 0. `sh -n`: OK.
The same test with a hostile leaked environment
(`srctree=…/usr/src/kernels/… objtree=… ARCH=x86_64 MAKEFLAGS=…`) still
builds the fallback correctly — the `env -i` isolation holds.

### 2. Full driver build with the fallback forced (isolated, host kernel 7.3.4)

```
$ MTGPU_REAL_OBJTOOL=/tmp/broken-objtool scripts/build-kernel-driver.sh
objtool-safe.sh: prebuilt kernel objtool unusable; rebuilt a working objtool from kernel sources: /tmp/mtgpu-objtool-7.3.4-300.fc45.x86_64/objtool
Objtool/compiler warning lines: 5502
BUILD_STATUS: SUCCESS
INSTALL_STATUS: NOT PERFORMED
LOAD_STATUS: NOT PERFORMED
```

| build | warnings | `.orc_unwind` | `.orc_unwind_ip` | `.orc_header` |
|---|---|---|---|---|
| forced-fallback | 5502 | `0x412f2` | `0x2b74c` | `0x14` |
| normal (healthy objtool) | 5502 | `0x412f2` | `0x2b74c` | `0x14` |
| pre-fix skip path | (16, no objtool) | **absent** | **absent** | `0x14` |

Byte-equal ORC sections and equal warning counts: the rebuilt objtool
produces the same result as the kernel tree's own binary. Verification
commands: `readelf -SW mtgpu.ko | grep orc`, `grep -c 'warning:' build.log`.
Logs: `.tmp/session-20261125/build-fallback2-*.log`,
`build-normal-*.log`, build trees under
`musa-data/research/builds/mtgpu-7.3.4-*/`.

### 3. COPR rebuild — every build log inspected

Submitted only the two changed SRPMs (everything else is byte-identical to
the already published set):

| project | package | build |
|---|---|---|
| `axogm/musa-fedora` (f44/f45/ELN/rawhide) | `mtgpu-dkms` 3.1.0-16 | [11388916](https://copr.fedorainfracloud.org/coprs/build/11388916/) |
| `axogm/musa-fedora` | `musa-toolkit` 3.1.0-6 | [11388917](https://copr.fedorainfracloud.org/coprs/build/11388917/) |
| `axogm/musa-mtt-s30` (adds f43) | `mtgpu-dkms` 3.1.0-16 | [11388919](https://copr.fedorainfracloud.org/coprs/build/11388919/) |
| `axogm/musa-mtt-s30` | `musa-toolkit` 3.1.0-6 | [11388920](https://copr.fedorainfracloud.org/coprs/build/11388920/) |

All four builds `succeeded`. Every one of the **18 per-chroot builder logs**
was fetched and classified (`grep -c 'error:|BUILD FAILED'`, `RPMResults
finished`, `linking without objtool`, `warning:`):

```
musa-fedora   11388916 dkms    f44/f45/eln/rawhide  OK errs=0 finish=1 no-ORC-skip=0 warnings=5502
musa-fedora   11388917 toolkit f44/f45/eln/rawhide  OK errs=0 finish=1 no-ORC-skip=0 warnings=0
musa-mtt-s30  11388919 dkms    f43/f44/f45/eln/rawhide OK errs=0 finish=1 no-ORC-skip=0 warnings=5502
musa-mtt-s30  11388920 toolkit f43/f44/f45/eln/rawhide OK errs=0 finish=1 no-ORC-skip=0 warnings=0
=== checked=18 bad=0 ===
```

The rawhide DKMS log now says:

```
objtool-safe.sh: prebuilt kernel objtool unusable; rebuilt a working objtool from kernel sources: /tmp/mtgpu-objtool-7.4.0-0.rc3.261119g4405c18c95d2.29.fc46.x86_64/objtool
Wrote: /builddir/build/RPMS/mtgpu-dkms-3.1.0-16.fc46.noarch.rpm
```

i.e. **5502 warning lines on rawhide — exactly the Fedora 44/45/ELN
baseline — and zero `linking without objtool` occurrences anywhere**.
Raw log: `.tmp/session-20261125/dkms16-rawhide-builder-live.log`.

`scripts/verify-copr-chroots.sh` (expectations updated to `mtgpu-dkms
3.1.0-16` and `musa-toolkit 3.1.0-6`) reports **20 passed, 0 failed**: build
states and the published `repodata` of all 9 chroots across both projects.

### 4. Isolated install gate (no scriptlets, container)

`scripts/test-container-install.sh --skip-memcpy` installed the refreshed
local set — including `mtgpu-dkms-3.1.0-16` and `musa-toolkit-3.1.0-6` —
into a throwaway Fedora 45 container with scriptlets disabled (no module is
built/loaded inside), then ran on the physical S30:

```
CONTAINER_SUMMARY: 13 passed, 0 failed
TEST_CONTAINER_INSTALL_STATUS: SUCCESS
```

(`musaInfo` MTT S30, `muMemcpyTest` success, `musart-smoke` PASS, `mcc
mp_10` vector-add 1 048 576 results verified.) Log:
`.tmp/session-20261125/container-test-20261125T084506Z.log`.

The same run uncovered a test-infrastructure problem: the container's
metalink resolved to mirrors throttled below dnf's curl low-speed limit
(repeated `Curl error (28)`, single-digit KiB/s on
`ftp.yz.yamagata-u.ac.jp`), stalling the dependency install for over an
hour across two attempts. The test now pins the container's `fedora` /
`fedora-updates` repos to the `dl.fedoraproject.org` baseurls that ship
behind the same metalink (same repos, same packages, only mirror
selection changes; `fedora-cisco-openh264` keeps its metalink because its
file carries no baseurl template). With the pin, 126 packages downloaded at
268–719 KiB/s in under 2 minutes, zero curl errors.

## What was NOT changed on the host

The running module stays **mtgpu-dkms 3.1.0-15**: a DKMS transaction while
`mtgpu` is loaded would replace the on-disk module of the live driver,
which this project forbids (`scripts/rpm-stack.sh` refuses exactly that
transaction). The host therefore runs:

* installed: `mtgpu-dkms` 3.1.0-15, `musa-toolkit` 3.1.0-5 (upgraded from
  -4 today via `pkexec dnf --refresh upgrade musa-toolkit` against the
  published repo; `rpm -V` clean, `dnf check` clean);
* published (COPR): `mtgpu-dkms` 3.1.0-16, `musa-toolkit` 3.1.0-6 and the
  unchanged rest of the set.

`scripts/rpm-stack.sh`'s pinned expectations were updated to `-16`/`-6`,
so the helper will refuse to install on the live-driver host and will pick
up exactly those versions on the next opportunity (mtgpu unloaded, e.g.
`scripts/compute-mode.sh disable` window or after a reboot with the block
restored). The %post rebuild in 3.1.0-16 uses the same wrapper, so the first
DKMS build on a rawhide-like kernel gets the fallback automatically.

## Files changed

* `driver-s30-3.1.0/scripts/objtool-safe.sh` — fallback resolution (cache,
  lock, staging, clean-environment build).
* `driver-s30-3.1.0/Makefile` and `Makefile` (kept identical) — export
  `MTGPU_KDIR`, hand the wrapper to kbuild in both `all` and ION paths.
* `scripts/build-kernel-driver.sh` — copies `scripts/objtool-safe.sh` into
  the isolated source tree so local builds exercise the wrapper too.
* `packaging/rpm/SPECS/mtgpu-dkms.spec` — release 16 + changelog.
* `packaging/rpm/SPECS/musa-toolkit.spec` — release 6 + changelog (README
  payload refreshed).
* `scripts/verify-copr-chroots.sh`, `scripts/rpm-stack.sh` — pinned
  versions updated.
* `scripts/test-container-install.sh` — container repo mirror pinning.

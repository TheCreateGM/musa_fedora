# Static and build-artifact verification (2026-11-24)

Independent, **read-only** verification pass. It deliberately covers the parts
of the project that can be proven without touching the GPU, the kernel module
state, the boot configuration or the package database:

* shell static analysis of every script,
* spec parsing of every RPM spec,
* repository health and history safety of both Git repos,
* content verification of the newly published `musa-runtime 1.5.4-5` artifact,
* an isolated unit test of the new `/etc/profile.d/zz-musa-fedora.sh` fix,
* a host *read-only* inventory of the installed stack.

**Nothing on the host was modified by this pass**: no `pkexec`, no `dnf install`,
no `modprobe`/`rmmod`, no unbind/rebind, no boot-file edit, no COPR publish and
no `git commit`/`push`. Hardware (GPU) execution is out of scope for this record
and is covered by the separate hardware-validation documents.

Environment: Fedora 45 Workstation, kernel `7.3.4-300.fc45.x86_64`,
Intel i5-13500, 7.4 GiB RAM. S30 present at `0000:01:00.0` (`1ed5:0102`).

---

## 1. COPR — every configured chroot (live re-run)

`scripts/verify-copr-chroots.sh` (the working-tree copy, including the new
"newest published version equals the expected set" check):

```
COPR verification for owner 'axogm' (api: https://copr.fedorainfracloud.org/api_3)

== project musa-fedora ==            4 chroots: 44, 45, eln, rawhide
  build 11378909 mtgpu-dkms     3.1.0-12   succeeded (4 chroots)
  build 11377871 mtgpu-firmware 3.1.0-4    succeeded (4 chroots)
  build 11379194 musa-runtime   1.5.4-5    succeeded (4 chroots)
  build 11377875 musa-sdk       5.2.0-6    succeeded (4 chroots)
  build 11377876 musa-toolkit   3.1.0-3    succeeded (4 chroots)
  PASS latest build of every package reports state=succeeded
  PASS <chroot>: all 6 expected artifact sets present          (×4)
  PASS <chroot>: newest published version equals the expected set (×4)

== project musa-mtt-s30 ==           5 chroots: 43, 44, 45, eln, rawhide
  build 11378948 mtgpu-dkms     3.1.0-12   succeeded (5 chroots)
  build 11377877 mtgpu-firmware 3.1.0-4    succeeded (5 chroots)
  build 11379219 musa-runtime   1.5.4-5    succeeded (5 chroots)
  build 11377881 musa-sdk       5.2.0-6    succeeded (5 chroots)
  build 11377882 musa-toolkit   3.1.0-3    succeeded (5 chroots)
  PASS latest build of every package reports state=succeeded
  PASS <chroot>: all 6 expected artifact sets present          (×5)
  PASS <chroot>: newest published version equals the expected set (×5)

passed: 20   failed: 0
```

Both COPR projects are authenticated (`copr-cli whoami` → `axogm`); GitHub is
authenticated (`gh auth status` → account `nnsupermax`, `repo` scope). No
auth gap blocked verification or publishing.

Two independent sources agree: the COPR frontend API (build state) and the
published repository metadata of every chroot (what a user's `dnf install`
actually resolves). The `newest==expected` check passes everywhere, i.e. the
historically published stale `mtgpu-dkms 5.2.0-1` / `musa-toolkit 5.2.0-1`
builds are no longer the newest EVR in any chroot.

## 2. Static analysis

| Check | Command | Result |
|---|---|---|
| Shell syntax of every script | `bash -n` over all `*.sh` (excluding `build/`, `.git/`) | **ALL_OK**, 0 failures |
| RPM spec parsing | `rpmspec -q` on all 5 specs | **parses cleanly**: `mtgpu-dkms-3.1.0-12.fc45`, `mtgpu-firmware-3.1.0-4.fc45.x86_64`, `musa-runtime-1.5.4-5.fc45.x86_64` + `musa-runtime-display-1.5.4-5.fc45.x86_64`, `musa-sdk-5.2.0-6.fc45.x86_64`, `musa-toolkit-3.1.0-3.fc45.x86_64` |
| Repository integrity | `git fsck --connectivity-only` (musa_fedora) | rc=0; only dangling blobs/trees (normal amend/rebase leftovers), **no missing/invalid objects** |
| Credential leakage | `git ls-files` matched against `.env`, `.netrc`, `coprc`, `id_rsa`, `*.pem`, `*token*`, `*secret*`, `*credential*` | **none tracked** in either repo |
| musa_data working tree | `git status -s` | **clean**, 0 untracked-not-ignored files, `main` in sync with `origin/main` |

`shellcheck`, `rpmlint`, `mock` and `fedpkg` are **not installed** on this host,
so those lint layers were not run; `bash -n` and `rpmspec -q` were. Installing
extra tooling was left out of this pass to keep it strictly read-only.

## 3. `.gitignore` review (both repositories)

**musa_fedora** (`musa-fedora/.gitignore`) covers: `*.o` (with an explicit
`!driver-s30-3.1.0/objs/**/mtgpu_core.o_binary` re-include for the prebuilt
vendor core object that is required to link the module), `*.ko*`, `*.mod*`,
`.*.cmd`, DKMS scratch, `*.log`/`.logs/`, all `packaging/rpm/{BUILD,BUILDROOT,RPMS,SRPMS,local-build*,work}/`,
`packaging/rpm/SOURCES/*.tar.gz`, the multi-hundred-MB extracted `userspace/`
and `tools/`, `.rollback/`, credentials (`.env`, `.netrc`, `.copr`, `id_rsa`,
`*.pem`, `*.rev`), caches/venvs, and the generated GPU smoke-test binaries.
`git status --ignored=matching` shows 26 ignored entries and only three
ordinary untracked files pending commit — no generated binary, tarball, RPM or
credential is in a position to be committed.

**musa_data** uses a safer **allowlist** model: everything under `/` is ignored
except `.gitignore`, `README.md`, `source-lock.json`, `vendor-inputs.sha256`,
`scripts/export-source-lock.py` and eight explicitly named `research/*.md`
notes. Nested `**/.git/` is excluded so the many upstream checkouts never leak
their Git histories; `*.rpm`, `*.deb`, `*.tar.gz`, `*.zip`, build trees, venvs
and credentials are all excluded. Result: 13 tracked files, 212 KiB packed, no
untracked-but-unignored files.

Both files are adequate; neither repository currently has a credential, key,
binary artifact or build output queued for commit.

## 4. Verified host state (read-only)

| Item | Observed |
|---|---|
| Installed packages | `mtgpu-dkms-3.1.0-12.fc45`, `mtgpu-firmware-3.1.0-4.fc45`, **`musa-runtime-1.5.4-5.fc45`** (upgraded during this session), `musa-sdk-5.2.0-6.fc45`, `musa-toolkit-3.1.0-3.fc45` (+ third-party `mvgal* 0.7.48`) |
| Module build | `/lib/modules/7.3.4-300.fc45.x86_64/extra/mtgpu.ko.xz`, `vermagic: 7.3.4-300.fc45.x86_64 SMP preempt mod_unload` = running kernel |
| S30 PCI alias | `alias: pci:v00001ED5d00000102sv*sd*bc03sc00i00*` and `...bc03sc02i00*` present (64 aliases total) |
| Module state | `mtgpu` **not loaded**, `/sys/module/mtgpu` absent, `0000:01:00.0` **unbound**, no `/dev/mtgpu*` — the boot blacklist is in force |
| Firmware | `/lib/firmware/mthreads/` populated (`mtfw-gen*.bin`, `mtvpu-*`, `mtlog-*`, `musa.sh.1.0.0.0`, …) |
| ldconfig cache | `libmusart.so.1.5 => /usr/local/musa/lib/…` **and** `libmusart.so.5 => /usr/local/musa-5.2/lib/…` both present — the ABI fork is real on this host |
| Kernel journal | `journalctl -k -b 0 \| grep -c 'Invalid cmd_type'` → **0** for the current boot (the `test-gpu.sh` fail-closed gate is *not* tripped) |
| Package verification | `rpm -V musa-runtime musa-sdk musa-toolkit mtgpu-dkms mtgpu-firmware` → only `S.5....T. c /etc/modprobe.d/90-mtgpu-headless.conf`, which is an expected `%config(noreplace)` local edit (flag `c`) |
| Dependency graph | `dnf check` → rc=0, no broken dependencies |

`dkms status` additionally lists `mtgpu/3.1.0` for the older kernels
`7.2.15-300.fc45` and `7.3.3-300.fc45` (left-over build trees for kernels no
longer running) and `mvgal/0.7.48` built for the running kernel with
"Differences between built and installed modules" reported by DKMS — recorded
here as an observation, not changed.

## 5. Built-artifact verification of `musa-runtime-1.5.4-5`

The release published to COPR is the one built from the checked-in spec:

* `musa-runtime-1.5.4-5.fc45.src.rpm` **spec extracted and diffed against
  `packaging/rpm/SPECS/musa-runtime.spec` → IDENTICAL** (traceability: what COPR
  built is what the repo contains).
* Payload contains the new fix file `/etc/profile.d/zz-musa-fedora.sh`
  (mode `0100644`, 1809 bytes, 6d906a0b…, declared `%config(noreplace)`).
* `musa-runtime` and `musa-runtime-display` file lists **overlap in 0 paths**,
  so the split subpackages cannot collide at install time.
* `rpm -qp --scripts`: `post → ldconfig + sysctl --system || true`;
  `postun → ldconfig`. Nothing else runs as root at install time.
* All 195 payload paths are confined to four prefixes: `/usr/local/musa` (147),
  `/usr/lib64` (41), `/etc` (6: `ld.so.conf.d/`, `modprobe.d/`, `musa.ini`,
  `profile.d/`, `sysctl.d/`, `vulkan/`) and a single `/usr/bin/mthreads-gmi`
  — no unexpected system path.

### 5.1 ABI root cause, proven from the ELF headers of the shipped tools

Both prefixes ship a `musaInfo`; only their `DT_NEEDED` differs:

| Tool | File | `DT_NEEDED` (MUSA part) |
|---|---|---|
| runtime 1.5.4 | `/usr/local/musa/bin/musaInfo` | **`libmusart.so.1.5` `libmusa.so.1.5`** |
| runtime 1.5.4 | `/usr/local/musa/bin/muMemcpyTest` | **`libmusart.so.1.5` `libmusa.so.1.5`** |
| SDK 5.2.0 | `/usr/local/musa-5.2/bin/musaInfo` | **`libmusart.so.5`** |

So *which* `musaInfo` runs is decided purely by `PATH`, and the legacy,
**unowned** `/etc/profile.d/musa.sh` (2026-11-10, from the disabled deb-style
install) sets `PATH=/usr/local/musa-5.2/bin:…`, selecting the `libmusart.so.5`
tool against an `mtgpu 3.1.0` driver — the documented `Invalid cmd_type`
abort. The `mcc` compiler (`/usr/local/musa-5.2/bin/mcc -> clang`) is unaffected
by this, which is why compile-only checks always passed.

Cross-checks of the fix shipped in 1.5.4-5:

* Container/static: `rpm -qpl` of the packaged tools, above.
* Installed tool ELF types: `ELF 64-bit LSB executable, x86-64`, interpreter
  `/lib64/ld-linux-x86-64.so.2` — no foreign architecture in the payload.

### 5.2 Isolated unit test of `/etc/profile.d/zz-musa-fedora.sh`

The shipped file was extracted **from the built RPM payload** (not from the
spec) and executed in an `env -i bash --noprofile --norc` sandbox together with
a copy of the legacy `musa.sh`:

| Test | Result |
|---|---|
| `bash -n` syntax | **PASS** |
| legacy `musa.sh` alone → PATH head | `/usr/local/musa-5.2/bin` (reproduces the bug) |
| …then `zz-musa-fedora.sh` → PATH head 1–2 | `/usr/local/musa/bin:/usr/local/musa-5.2/bin` → **PASS (runtime first)** |
| …→ `LD_LIBRARY_PATH` head 1–2 | `/usr/local/musa/lib:/usr/local/musa-5.2/lib` → **PASS (runtime first)** |
| empty entry from the legacy value (`…lib:`) dropped | **PASS** (normalised, no empty `PATH`/`LD_LIBRARY_PATH` component) |
| unrelated `PATH` entries (`/opt/custom/bin`, `/usr/bin`) preserved | **PASS** |
| sourcing twice | **PASS** (idempotent — head unchanged) |
| `set -u` with `LD_LIBRARY_PATH` unset | **PASS** (no unbound-variable abort) |
| ordering rule | `m…` < `z…` in every collation, so `zz-musa-fedora.sh` always loads after `/etc/profile.d/musa.sh` — **PASS** |

Limit: this proves the *ordering* fix for login shells. It does not and cannot
prove GPU behaviour, and it does not cover non-login shells, cron or systemd user
services (use `scripts/musa-env.sh` / `scripts/check-musa-env.sh` there).

## 6. Findings / follow-ups (not changed by this pass)

1. **Docs still describing the pre-1.5.4-5 state.** `docs/FINAL_REPORT_2026-11-23.md`
   states `verify-copr-chroots` = "17 passed / 3 failed" (live is **20/0**),
   `main (HEAD a2614e4)` (actual HEAD is `10d52b7`, in sync with `origin/main`),
   and that COPR publishes `musa-runtime 1.5.4-4` (it now publishes **1.5.4-5**).
   Nine further documents and `scripts/rpm-stack.sh` still cite `1.5.4-4` as the
   current published release. Reported to the document owner; not edited here.
2. **Unowned legacy configuration** left by the disabled deb-style install:
   `/etc/profile.d/musa.sh`, `/etc/ld.so.conf.d/musa.conf` (duplicates
   `musa-sdk.conf` plus `/usr/lib64`), `/etc/ld.so.conf.d/00-mtgpu.conf`
   (also lists a non-existent `/usr/lib64/mthreads`), and
   `/etc/modprobe.d/blacklist-mtgpu.conf` (intentional — it is the opt-in boot
   block). None is owned by a package, so RPM will never clean them up.
   Safest handling is `scripts/check-musa-env.sh --quarantine-orphans --plan`
   first; doing so is a host mutation and is left to the host-side owner.
3. **`/usr/lib64/libdrm_mtgpu.so.1` provenance.** For most of this session the
   symlink pointed at the unowned leftover `libmtperf_target.so` (copied by
   `musa-data/research/disabled-deployment-drafts-2026-11-11/install-musa-sdk.sh`),
   which `rpm -V musa-runtime` flagged as `....L....` and which
   `docs/hardware-validation-2026-11-23-load-test.md` names as the most likely
   cause of `mthreads-gmi`'s `failed to initialize mtml`. By the end of this
   pass it had been restored to the packaged `libdrm_mtgpu.so.1.0.0` (mtime back
   to the payload value), `rpm -V musa-runtime` is now **clean**, and
   `dnf check` returns 0. Re-run `rpm -V musa-runtime` after any future
   `musa-runtime` upgrade to catch a regression here.
4. **`/etc/modprobe.d/90-mtgpu-headless.conf` drift** (`S.5....T. c`) — a
   `%config(noreplace)` file edited locally (and a stale `.rpmnew` from
   2026-11-20 is present). Harmless, but worth a deliberate accept/reject of the
   `.rpmnew` before the next driver release.
5. **Repository housekeeping:** `musa_fedora` `.git` is 1.2 GB and contains
   ~98 MB of garbage from interrupted transfers
   (`.git/objects/pack/tmp_pack_smA66Y`, `tmp_obj_FEUvEG`, `tmp_obj_FdQHND`).
   Removing only those `tmp_*` files is safe, but it touches the repo another
   agent commits from, so it was **not** performed here.
6. **Resource observation:** `/tmp` is a 3.8 GiB tmpfs and two duplicate
   `dnf -y download --destdir=/tmp/musarpm …` jobs were running concurrently
   during this pass (network throughput measured ≈20 KB/s from
   `download.copr.fedorainfracloud.org`). Peer-coordination risk, not a defect.
7. **Tooling gap:** `shellcheck`/`rpmlint`/`mock`/`fedpkg` are absent, so the
   packaging lint layer used by Fedora packagers is not part of CI yet.

## 7. What this record does **not** claim

No GPU workload, no module load, no unbind/rebind, no boot change and no COPR
publish happens in this pass. Nothing here establishes that the S30 runs a
kernel today — that is exactly what
[the hardware-validation records](hardware-validation-2026-11-23-suite-verify.md)
are for, and they must be re-run after the 1.5.4-5 upgrade. Everything above is
backed by the command output quoted or described in it.

# COPR publication verification — 2026-11-25

Both COPR projects were rebuilt with the current package set and verified at the
repository level, not only by "build succeeded".

## Published set

| Package | Version-release | Contents |
|---|---|---|
| `mtgpu-dkms` | 3.1.0-15 | driver source with the 16 KiB system/SVM heap page |
| `mtgpu-firmware` | 3.1.0-5 | S30 firmware |
| `musa-runtime` | 1.5.4-7 | MUSA 1.5.4 userspace runtime |
| `musa-runtime-va` | 1.5.4-7 | S30 VA-API driver (VPU) |
| `musa-runtime-display` | 1.5.4-7 | optional vendor Mesa/DRI payload (not for a GNOME session) |
| `musa-sdk` | 5.2.0-7 | `mcc` 5.2.0 compiler |
| `musa-toolkit` | 3.1.0-5 | metapackage; description corrected, exactly matching the set above |

## Builds

`axogm/musa-fedora` — 4 chroots (Fedora 44, 45, ELN, Rawhide):

| Package | Build |
|---|---|
| `mtgpu-firmware` 3.1.0-5 | [11388526](https://copr.fedorainfracloud.org/coprs/build/11388526/) |
| `mtgpu-dkms` 3.1.0-15 | [11388527](https://copr.fedorainfracloud.org/coprs/build/11388527/) |
| `musa-runtime` 1.5.4-7 | [11388528](https://copr.fedorainfracloud.org/coprs/build/11388528/) |
| `musa-sdk` 5.2.0-7 | [11388529](https://copr.fedorainfracloud.org/coprs/build/11388529/) |
| `musa-toolkit` 3.1.0-5 | [11388592](https://copr.fedorainfracloud.org/coprs/build/11388592/) |

`axogm/musa-mtt-s30` — 5 chroots (adds Fedora 43):

| Package | Build |
|---|---|
| `mtgpu-firmware` 3.1.0-5 | [11388530](https://copr.fedorainfracloud.org/coprs/build/11388530/) |
| `mtgpu-dkms` 3.1.0-15 | [11388531](https://copr.fedorainfracloud.org/coprs/build/11388531/) |
| `musa-runtime` 1.5.4-7 | [11388532](https://copr.fedorainfracloud.org/coprs/build/11388532/) |
| `musa-sdk` 5.2.0-7 | [11388533](https://copr.fedorainfracloud.org/coprs/build/11388533/) |
| `musa-toolkit` 3.1.0-5 | [11388593](https://copr.fedorainfracloud.org/coprs/build/11388593/) |

Every chroot of every build reports `succeeded` — 9/9 chroots, 45 chroot
builds.

## Repository-level verification

```sh
scripts/verify-copr-chroots.sh
```

```
== project musa-fedora ==
  PASS latest build of every package reports state=succeeded
  PASS fedora-44-x86_64: all 7 expected artifact sets present
  PASS fedora-44-x86_64: newest published version equals the expected set for all 7 packages
  ... (same for fedora-45, fedora-eln, fedora-rawhide)
== project musa-mtt-s30 ==
  PASS latest build of every package reports state=succeeded
  PASS fedora-43-x86_64: all 7 expected artifact sets present
  ... (same for fedora-44, fedora-45, fedora-eln, fedora-rawhide)

passed: 20   failed: 0
```

The check queries two independent sources: the COPR API for build state, and
each chroot's published `repodata` via `dnf repoquery` for the artifacts users
actually receive. "Build succeeded" alone would not prove the repository serves
the right versions; this does.

Cross-release resolution was also checked in fresh containers, with only the
published repository enabled and `install_weak_deps=False`:

| Container | Resolved `musa-toolkit` | Pulls |
|---|---|---|
| Fedora 44 (`musa-fedora`) | 3.1.0-5.fc44 | dkms 3.1.0-15, firmware 3.1.0-5, runtime 1.5.4-7, sdk 5.2.0-7 |
| Fedora 43 (`musa-mtt-s30`) | 3.1.0-5.fc43 | dkms 3.1.0-15, firmware 3.1.0-5, runtime 1.5.4-7, sdk 5.2.0-7 |

The full payload install was not completed in these two containers: COPR
throughput dropped to single-digit KiB/s, and the SDK is 1 GiB. The full install
and GPU run were done on Fedora 45 with the locally built RPMs. Resolution
proves the dependency chain and versions; the Fedora 45 run proves installation
and execution.

## Reproducing the submission

`copr-cli build` and a direct multipart POST to `/api_3/build/create/upload`
returned HTTP 500 for every SRPM on 2026-11-24/25, while the Python `copr.v3`
client's `build_proxy.create_from_file()` succeeded. The helper used for this
session is `.tmp/submit-copr.py` (not committed; `.tmp` is ignored). Any future
submission should verify the build id returned and then run
`scripts/verify-copr-chroots.sh`.

## Not established by a green build

A successful COPR build only proves the source compiles and the RPMs are
well-formed. It does not prove the module loads, the GPU binds, or any runtime
path works. Those are separate, hardware-backed results:
[hardware validation 2026-11-25](hardware-validation-2026-11-25.md).

## Addendum — later the same day: mtgpu-dkms 3.1.0-16, musa-toolkit 3.1.0-6

A per-chroot **log** audit of the build above (rather than trusting
`state=succeeded`) found objtool being skipped on `fedora-rawhide`: the
kernel-devel objtool's datestamped `libopcodes-…20261029` SONAME was gone
from the chroot. The wrapper was repaired to rebuild a working objtool, and
the two changed specs were rebuilt.

| package | new release | builds |
|---|---|---|
| `mtgpu-dkms` | 3.1.0-16 | [11388916](https://copr.fedorainfracloud.org/coprs/build/11388916/) (musa-fedora), [11388919](https://copr.fedorainfracloud.org/coprs/build/11388919) (musa-mtt-s30) |
| `musa-toolkit` | 3.1.0-6 | [11388917](https://copr.fedorainfracloud.org/coprs/build/11388917) (musa-fedora), [11388920](https://copr.fedorainfracloud.org/coprs/build/11388920) (musa-mtt-s30) |

All four builds `succeeded`; `scripts/verify-copr-chroots.sh` (expectations
updated) reports **20 passed, 0 failed**, i.e. the published `repodata` of
all 9 chroots across both projects now advertises the seven-package set at
dkms 3.1.0-16 and toolkit 3.1.0-6 (the other five artifacts are unchanged
from the table above).

All **18** per-chroot builder logs of this submission were fetched and
checked: 0 `error:`/`BUILD FAILED` lines, `RPMResults finished` in every
one, 0 `linking without objtool` occurrences, and 5502 warning lines in
each of the 9 DKMS chroots — including `fedora-rawhide`, where objtool had
previously been skipped. Details and raw excerpts:
[objtool-fallback-2026-11-25](objtool-fallback-2026-11-25.md).

The host keeps `mtgpu-dkms` 3.1.0-15 / `musa-toolkit` 3.1.0-5 while `mtgpu`
is loaded; the -16/-6 pair is installed in a window without the live
module. The isolated container gate with the -16/-6 RPMs reports **13
passed, 0 failed**.

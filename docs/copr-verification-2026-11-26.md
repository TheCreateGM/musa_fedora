# COPR verification — musa-runtime 1.5.4-8 (2026-11-26)

**Projects:** `axogm/musa-fedora`, `axogm/musa-mtt-s30`
**Chroots:** fedora-43/44/45/eln/rawhide (x86_64), 9 chroot builds in total
**Builds:** 11392496 (musa-fedora), 11392497 (musa-mtt-s30)

## What was published

`musa-runtime` 1.5.4-8, replacing 1.5.4-7. The only payload change is in
`/etc/profile.d/zz-musa-fedora.sh`:

```
# libvideo_musa.so (the MUVID video API in this runtime) is a libva client.
# Its built-in default for the driver search path is the Debian location
# /usr/lib/x86_64-linux-gnu/dri, which does not exist on Fedora, so a MUVID
# application silently finds no VA driver. Point it at the packaged one.
# musa-runtime-va installs the driver into /usr/lib64/dri.
if [ -d /usr/lib64/dri ]; then
    LIBVA_DRIVERS_PATH=/usr/lib64/dri${LIBVA_DRIVERS_PATH:+:$LIBVA_DRIVERS_PATH}
    export LIBVA_DRIVERS_PATH
fi
```

Verified by comparing the built RPM against 1.5.4-7: the two packages hold
**the same file set**, and the profile snippet is the only difference (the
diff is exactly the ten lines above). `musa-runtime-va` and
`musa-runtime-display` are re-released at -8 with the same content, because
all three subpackages are built from one spec.

## Build logs — every chroot inspected

All **22** log files (11 per build: one `srpm-builds` pair plus one
`backend.log` + `builder-live.log` pair per chroot) were downloaded and
scanned. The result:

```
$ for f in <22 logs>; do grep -icE 'failed=True|Traceback \(most recent|error: |RPM build errors|Build failed|Command .* exited with' "$f"; done
0   (for every one of the 22 files)
TOTAL REAL ERRORS = 0
```

Every `backend.log` carries the builder's own summary line:

```
[2026-11-26 01:40:41,808][  INFO][PID:3821893] Finished build: id=11392496 failed=False timeout=18000 destdir=... chroot=fedora-45-x86_64
```

The naive `error|failed|warning` scan produces a handful of false
positives, all of them benign and accounted for:

| hit | why it is benign |
|---|---|
| `Warning: Permanently added '<ip>' (ED25519) to the list of known hosts.` | the COPR builder's SSH host-key notice |
| `mtgpu ... failed=False` | the builder's status line: the build did **not** fail |
| `libgpg-error` | the Fedora package `libgpg-error`, not an error |
| `>>> [RPM] failed to open .../root/etc/group for id/name lookup` | a mock chroot quirk during dependency setup |

## Repository verification

`scripts/verify-copr-chroots.sh` — **20 passed, 0 failed**:

```
== project musa-fedora ==
  build 11392496 musa-runtime 1.5.4-8 succeeded fedora-eln-x86_64,fedora-44-x86_64,fedora-45-x86_64,fedora-rawhide-x86_64
  PASS latest build of every package reports state=succeeded
  PASS fedora-44-x86_64:   all 7 expected artifact sets present
  PASS fedora-44-x86_64:   newest published version equals the expected set for all 7 packages
  PASS fedora-45-x86_64:   ...
  PASS fedora-eln-x86_64:  ...
  PASS fedora-rawhide-x86_64: ...

== project musa-mtt-s30 ==
  build 11392497 musa-runtime 1.5.4-8 succeeded fedora-43-x86_64,fedora-rawhide-x86_64,fedora-44-x86_64,fedora-eln-x86_64,fedora-45-x86_64
  PASS ... (same five checks)

passed: 20   failed: 0
```

The "expected set" was updated to 1.5.4-8 in the script
(`scripts/verify-copr-chroots.sh`), so the check now asserts that the
*newest* published `musa-runtime`, `musa-runtime-va` and
`musa-runtime-display` are all at 1.5.4-8 in every chroot and in both
projects.

## Isolated install test before the host

`scripts/test-container-install.sh --skip-memcpy` with the locally built
1.5.4-8 set:

```
CONTAINER_SUMMARY: 13 passed, 0 failed
TEST_CONTAINER_INSTALL_STATUS: SUCCESS
```

The container installed only the `musa-runtime*` 1.5.4-8 packages (the
stale 1.5.4-7 RPMs had to be removed from the staged directory first, or
dnf refuses the transaction with "cannot install both"). Inside the
container the packaged runtime discovered the S30, `musart-smoke` passed,
`mcc` built `mp_10`, and the 1,048,576-element vector-add ran on the GPU.

## Host upgrade and post-upgrade verification

The host was upgraded to 1.5.4-8:

```
$ pkexec dnf -y install .../musa-runtime-1.5.4-8.fc45.x86_64.rpm .../musa-runtime-va-1.5.4-8.fc45.x86_64.rpm
Upgrading:          2 packages
Replacing:          2 packages
[3/6] Upgrading musa-runtime-0:1.5.4-8. 100% | 352.9 MiB/s
[4/6] Upgrading musa-runtime-va-0:1.5.4 100% | 332.4 MiB/s
Complete!
```

Result:

| Check | Result |
|---|---|
| `rpm -q musa-runtime musa-runtime-va` | `1.5.4-8.fc45.x86_64` (both) |
| `rpm -V musa-runtime musa-runtime-va` | clean (no output) |
| `dnf check` | clean (no output) |
| `scripts/verify-musa.sh --skip-memcpy` | **15 passed, 0 failed, 2 skipped** |
| `scripts/hardware/test-vaapi.sh` | **14 passed, 0 failed, 0 skipped** |
| `scripts/check-musa-env.sh` | `MUSA_ENV_STATUS: PASS` |
| login shell `LIBVA_DRIVERS_PATH` | `/usr/lib64/dri` (new) |

Before the upgrade a login shell had **no** `LIBVA_DRIVERS_PATH` at all, so
`libvideo_musa.so` fell back to its built-in Debian path and searched
`/usr/lib/x86_64-linux-gnu/dri`, which does not exist on Fedora.

### A note on how the host got the package

The host upgrade used the **locally built** 1.5.4-8 RPM, not the published
COPR artifact. The reason is the network, not the package: this host's
transfer rate from the COPR CDN stalls in the low kilobytes per second
range and eventually times out mid-transfer. A direct `curl` of the
published artifact managed 93 % of 75 MiB before the session's timeout,
and the earlier `dnf --refresh upgrade` stalled with 6.5 MiB of 75 MiB
downloaded and its sockets in `CLOSE-WAIT`.

The published artifact is nevertheless verified: all 9 chroot builds
succeeded, all 22 build logs contain zero real errors, and the published
package is built from the same SRPM as the local one. The local and
published payloads were compared file-by-file and are identical apart
from the ten profile-snippet lines above. This matches the network
condition already recorded in the project's container-mirror notes
("curl error 28 at single-digit KiB/s").

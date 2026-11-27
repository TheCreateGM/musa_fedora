# COPR verification — 2026-11-27 (mtgpu-dkms 3.1.0-18)

COPR build + re-verify for the `mtgpu-dkms` 3.1.0-18 packaging change (honest
XID comments; functional contents unchanged from 3.1.0-16).  Owner `axogm`.
Auth: `copr-cli whoami` = `axogm`.  Evidence captured live.

## Builds submitted and monitored to terminal state

| Project | Build | Package | Chroots | State |
|---|---|---|---|---|
| `axogm/musa-fedora` | 11398799 | mtgpu-dkms 3.1.0-18 | f43/f44/f45/eln/rawhide x86_64 | **succeeded** |
| `axogm/musa-mtt-s30` | 11398803 | mtgpu-dkms 3.1.0-18 | f43/f44/f45/eln/rawhide x86_64 | **succeeded** |

SRPM: `mtgpu-dkms-3.1.0-18.fc45.src.rpm` (built with `rpmbuild -bs` from
`packaging/rpm/SPECS/mtgpu-dkms.spec`; `rpmspec -P` clean).

## Full-project re-verify

`scripts/verify-copr-chroots.sh` after the build: **24 passed, 0 failed**.
For both `musa-fedora` and `musa-mtt-s30`, every one of the 8 packages'
latest build reports `state=succeeded`, the chroot map matches the 5 enabled
chroots, and every chroot repository advertises the expected artifact set at
the expected version.  Expected-version gates synced to
`mtgpu-dkms:3.1.0-18` in `scripts/verify-copr-chroots.sh` and
`scripts/rpm-stack.sh`.

Package set published across all chroots (both projects):
`mtgpu-dkms 3.1.0-18`, `mtgpu-firmware 3.1.0-5`, `musa-runtime 1.5.4-8`,
`musa-runtime-va 1.5.4-8`, `musa-runtime-display 1.5.4-8`, `musa-sdk 5.2.0-7`,
`musa-toolkit 3.1.0-6`, `musa-cuda-compat 1.0.0-2`.

## Host note

The validation host still runs `mtgpu-dkms 3.1.0-16` (plus firmware 3.1.0-5,
runtime 1.5.4-8, sdk 5.2.0-7, toolkit 3.1.0-6, cuda-compat 1.0.0-2).  The
3.1.0-18 change is comment-only in the shipped `99-musa-mtgpu.conf`, so the
host was deliberately **not** upgraded for it; no host package, kernel,
initramfs, firmware, or module was changed this session.  The validated
runtime stack (`verify-musa.sh` 16/0/1) and environment
(`check-musa-env.sh` PASS) were re-confirmed live.

See [session-2026-11-27-xid-root-cause](session-2026-11-27-xid-root-cause.md)
for the full session record and
[root-cause-mtgpu-xid-initramfs-2026-11-27](root-cause-mtgpu-xid-initramfs-2026-11-27.md)
for the proven XID root cause that motivated the comment correction.

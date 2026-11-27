# RPM signing and verification

The Fedora packages in this project are built locally and are **unsigned by
default**. Packages published through Copr are signed by Copr's build service;
packages built on this host are not. This document describes the local
signing path that closes that gap: a dedicated OpenPGP key in the builder's
own GnuPG home, `rpmsign` to sign the built set, and `rpmkeys` to verify a set
without touching the system RPM keyring.

Red Hat's Fedora package signing keys cannot be used or reproduced here: those
are held by the Fedora Project and the package set is therefore **not
upstream-qualified**. What this workflow provides is integrity and provenance
for the locally built artifacts.

## Tooling

| Command | Purpose |
|---|---|
| `scripts/rpm-signing.sh keygen` | create or reuse the local build-signing key |
| `scripts/rpm-signing.sh fingerprint` | print the key fingerprint |
| `scripts/rpm-signing.sh export` | write the armored public key to `packaging/rpm/keys/` |
| `scripts/rpm-signing.sh sign` | sign every RPM below the build tree |
| `scripts/rpm-signing.sh verify` | verify signatures against a public key |
| `scripts/rpm-signing.sh check` | report the signing state (read-only) |

Requirements: `gpg`, `rpmsign`, `rpmkeys` (all in `rpm`/`gnupg2` packages),
and `rpmbuild` for producing the packages.

## Verification record — 2026-11-23

The complete local 5-RPM set (`mtgpu-dkms` 3.1.0-11, `mtgpu-firmware` 3.1.0-4,
`musa-runtime` 1.5.4-3, `musa-sdk` 5.2.0-6, `musa-toolkit` 3.1.0-3) was signed
with the local build-signing key `763A7D1470E1672E236F290EFBA85A80C8D8BC89`
via `scripts/rpm-signing.sh sign` (unsigned originals preserved as
`*.rpm.unsigned`) and verified with `scripts/rpm-signing.sh verify`:
`VERIFIED_PACKAGES: 5`, every package `OK` against the armored public key in
`packaging/rpm/keys/musa-fedora-signing-key.asc`. The key is tracked in the
repository; the revocation certificate (`*.rev`) is not. This closes the
"local packages are unsigned by default" gap for the current local-build
tree; packages published by Copr remain signed by the Copr build service.

## Host-wide trust rollout — 2026-11-27

Until this session the system RPM keyring did not know the project key, so
plain `rpm -K` reported `digests SIGNATURES NOT OK` on local packages while
`scripts/rpm-signing.sh verify` (temporary keyring) reported `OK`. Both halves
of that gap are now closed:

1. **Signing.** The current local-build tree holds 8 RPMs — `mtgpu-dkms`
   3.1.0-16, `mtgpu-firmware` 3.1.0-5, `musa-runtime` 1.5.4-8,
   `musa-runtime-va` 1.5.4-8, `musa-runtime-display` 1.5.4-8, `musa-sdk`
   5.2.0-7, `musa-toolkit` 3.1.0-6 and `musa-cuda-compat` 1.0.0-1. All eight
   were unsigned (`signed packages: 0/8`); `scripts/rpm-signing.sh sign`
   signed every one (`SIGNED_PACKAGES: 8`) and `verify` then reported
   `VERIFIED_PACKAGES: 8`, each `OK` against
   `packaging/rpm/keys/musa-fedora-signing-key.asc`.
2. **Trust rollout.** The public key was imported into the system RPM keyring
   with elevated privileges and an absolute path:

   ```sh
   pkexec rpm --import /home/axogm/Documents/project/musa-fedora/packaging/rpm/keys/musa-fedora-signing-key.asc
   ```

   A relative path fails with `import read failed(2)` under `pkexec` — the
   working directory is not what the script expects. Afterwards `rpm -K` on
   the whole local set reports **`digests signatures OK` for 8/8**, and
   `rpm -qa gpg-pubkey*` shows
   `gpg-pubkey-763a7d1470e1672e236f290efba85a80c8d8bc89-6afc1175`.

Signing touches only the RPM header: the cpio payloads of the signed packages
are byte-identical to their `.unsigned` backups
(`rpm2cpio … | sha256sum` → `a4b5265f…` and `9fb64893…` for `musa-sdk` and
`mtgpu-dkms`, and `rpm -K --nosignature` still reports `digests OK`), so the
package payloads did not change. Rolling the key back out of the system
keyring is:

```sh
pkexec rpm -e gpg-pubkey-763a7d1470e1672e236f290efba85a80c8d8bc89
```

Re-run `scripts/rpm-signing.sh sign` after each local rebuild: a freshly built
package is unsigned until signed, and `check` reports the current ratio.

## Creating the key

```sh
scripts/rpm-signing.sh keygen
```

This creates an RSA-4096 signing key in the caller's `~/.gnupg` with the user
ID `MUSA Fedora Local Signing Key <musa@fedoraproject.org>`. The key has no
passphrase so that unattended local builds work; the keyring directory is
mode 0700 and the private key never leaves the builder's home directory.

A passphrase-protected key can be used instead, as long as the agent can
answer the pinentry (run `gpgconf --kill gpg-agent` after changing the
key's passphrase, and keep the agent's cache alive during the build).

The fingerprint is written to `packaging/rpm/keys/signing-key.fpr`, and the
revocation certificate that GnuPG stores next to the key is copied into the
same directory. **Keep the revocation certificate out of shared storage**: it
can revoke the key. It is excluded from version control by `.gitignore`.

## Signing

```sh
scripts/rpm-signing.sh sign
```

Every RPM below `packaging/rpm/local-build/RPMS` is signed with
`rpmsign --addsign`. The unsigned original is kept next to the package as
`<package>.rpm.unsigned` so that a build can be reproduced byte for byte.
Already signed packages are skipped; use `--force` to replace an existing
signature (`rpmsign --resign`).

## Verifying

```sh
scripts/rpm-signing.sh verify
```

Verification imports the exported public key into a *temporary* rpmkeys
database below `$TMPDIR` and checks every package with
`rpmkeys --checksig`. No root privileges, no system keyring, and no package
database are involved, so a set downloaded from a build host can be checked by
a user without administrative rights.

To verify a set against a key that is not in this tree:

```sh
scripts/rpm-signing.sh verify --key /path/to/public-key.asc
```

To make the key known to the system RPM database (needs root, and adds the
key to the trust store of every user of the machine):

```sh
sudo rpm --import packaging/rpm/keys/musa-fedora-signing-key.asc
rpm -K musa-sdk-5.2.0-6.fc45.x86_64.rpm
```

## Signed package set verified on this host

The five Fedora 45 packages built on this host were signed and verified with
this workflow:

```
$ scripts/rpm-signing.sh check
signed packages: 5/5

$ scripts/rpm-signing.sh verify
OK      mtgpu-dkms-3.1.0-5.fc45.noarch.rpm
OK      mtgpu-firmware-3.1.0-2.fc45.noarch.rpm
OK      musa-runtime-1.5.4-3.fc45.x86_64.rpm
OK      musa-sdk-5.2.0-6.fc45.x86_64.rpm
OK      musa-toolkit-3.1.0-1.fc45.x86_64.rpm
VERIFIED_PACKAGES: 5
```

Key fingerprint and the verification log are recorded in
[the current state report](current-state-2026-11-17.md).

## Limits

* The signature proves that the packages were signed by the holder of this
  key. It does not make the packages upstream Fedora packages.
* The key is local to this builder. Publishing the packages through Copr
  keeps Copr's own signature, which is what users of the Copr repository
  verify.
* `rpm -K --nosignature` (used elsewhere in this project) only checks the
  digests and stays silent about signatures; use `rpm-signing.sh verify` when
  signature status matters.

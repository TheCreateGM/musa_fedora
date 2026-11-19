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

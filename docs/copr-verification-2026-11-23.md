# COPR verification — 2026-11-23

## Scope

Independent, reproducible verification that both configured COPR projects
publish a complete, installable MUSA/MTT S30 package set in **every**
configured chroot. This closes the "Verification still required" item of
[`display-driver-subpackage-2026-11-23.md`](display-driver-subpackage-2026-11-23.md).

Method: two independent sources per project.

1. **COPR frontend API** (`https://copr.fedorainfracloud.org/api_3`) — the
   state of the latest build of every package.
2. **Published repository metadata** of every chroot, queried with
   `dnf repoquery` against the real download URLs. This is the list a user
   actually gets from `dnf install`, not just a build status.

The check is scripted and repeatable:
[`scripts/verify-copr-chroots.sh`](../scripts/verify-copr-chroots.sh).

```
$ scripts/verify-copr-chroots.sh
COPR verification for owner 'axogm' (api: https://copr.fedorainfracloud.org/api_3)

== project musa-fedora ==
  PASS latest build of every package reports state=succeeded
  PASS fedora-44-x86_64: all 6 expected artifact sets present
  PASS fedora-45-x86_64: all 6 expected artifact sets present
  PASS fedora-eln-x86_64: all 6 expected artifact sets present
  PASS fedora-rawhide-x86_64: all 6 expected artifact sets present

== project musa-mtt-s30 ==
  PASS latest build of every package reports state=succeeded
  PASS fedora-43-x86_64: all 6 expected artifact sets present
  PASS fedora-44-x86_64: all 6 expected artifact sets present
  PASS fedora-45-x86_64: all 6 expected artifact sets present
  PASS fedora-eln-x86_64: all 6 expected artifact sets present
  PASS fedora-rawhide-x86_64: all 6 expected artifact sets present

passed: 11   failed: 0
```

## Result

| Project | Chroot | musa-runtime | musa-runtime-display | musa-sdk | musa-toolkit | mtgpu-dkms | mtgpu-firmware |
|---|---|---|---|---|---|---|---|
| `axogm/musa-fedora` | fedora-44-x86_64 | 1.5.4-4 | 1.5.4-4 | 5.2.0-6 | 3.1.0-3 | 3.1.0-11 | 3.1.0-4 |
| `axogm/musa-fedora` | fedora-45-x86_64 | 1.5.4-4 | 1.5.4-4 | 5.2.0-6 | 3.1.0-3 | 3.1.0-11 | 3.1.0-4 |
| `axogm/musa-fedora` | fedora-eln-x86_64 | 1.5.4-4 | 1.5.4-4 | 5.2.0-6 | 3.1.0-3 | 3.1.0-11 | 3.1.0-4 |
| `axogm/musa-fedora` | fedora-rawhide-x86_64 | 1.5.4-4 | 1.5.4-4 | 5.2.0-6 | 3.1.0-3 | 3.1.0-11 | 3.1.0-4 |
| `axogm/musa-mtt-s30` | fedora-43-x86_64 | 1.5.4-4 | 1.5.4-4 | 5.2.0-6 | 3.1.0-3 | 3.1.0-11 | 3.1.0-4 |
| `axogm/musa-mtt-s30` | fedora-44-x86_64 | 1.5.4-4 | 1.5.4-4 | 5.2.0-6 | 3.1.0-3 | 3.1.0-11 | 3.1.0-4 |
| `axogm/musa-mtt-s30` | fedora-45-x86_64 | 1.5.4-4 | 1.5.4-4 | 5.2.0-6 | 3.1.0-3 | 3.1.0-11 | 3.1.0-4 |
| `axogm/musa-mtt-s30` | fedora-eln-x86_64 | 1.5.4-4 | 1.5.4-4 | 5.2.0-6 | 3.1.0-3 | 3.1.0-11 | 3.1.0-4 |
| `axogm/musa-mtt-s30` | fedora-rawhide-x86_64 | 1.5.4-4 | 1.5.4-4 | 5.2.0-6 | 3.1.0-3 | 3.1.0-11 | 3.1.0-4 |

Latest build IDs (COPR package builds, identical content across the chroots
of one project):

| Package | `axogm/musa-fedora` | `axogm/musa-mtt-s30` |
|---|---|---|
| musa-runtime (incl. `musa-runtime-display`) | 11377974 | 11377976 |
| mtgpu-dkms | 11377872 | 11377878 |
| mtgpu-firmware | 11377871 | 11377877 |
| musa-sdk | 11377875 | 11377881 |
| musa-toolkit | 11377876 | 11377882 |

All of these report `state=succeeded` in the COPR API.

## Architecture coverage

The MTT S30 vendor stack (kernel driver, MUSA runtime, `mcc` compiler) is
**x86_64-only**: every package is `ExclusiveArch: x86_64` or ships `x86_64`
firmware blobs, and the Moore Threads hardware ships as a PCIe x86 device.
Both COPR projects therefore publish x86_64 chroots only. The projects also
have aarch64 chroots enabled, and those correctly produce **no** artifacts
(`ExclusiveArch: x86_64` packages are skipped by the builder) — their
repository directories stay empty, which is the expected result and not a
build failure.

Historical non-succeeded builds (`mtgpu-dkms` 11365559, `mtgpu-firmware`
11366194) are superseded by the builds listed above and are irrelevant to the
current package set.

## Payload check of the display split

Downloaded from the Fedora 45 repository of `axogm/musa-fedora`:

```
musa-runtime-1.5.4-4.fc45.x86_64.rpm
sha256 4f1a708abfc59ea5ba9104ce938faa3090679a4f78e21165b8e33f0cf219224c
```

`rpm -qpl` lists 194 files and **no** path under `/usr/lib64/dri`,
`/usr/lib64/gbm` or `/usr/lib64/xorg`. The eight vendor Mesa DRI drivers
(`musa_dri.so`, `mtgpu_dri.so`, `kms_swrast_musa_dri.so`,
`swrast_musa_dri.so`, `musa_drv_video.so`, `mtgpu_drv_video.so`,
`mtgpu_drv_video_1.so`, `mtgpu_drv_video_2.so`) are therefore absent from a
default compute-only install.

This is the packaging half of the root-cause fix for the GNOME Shell crash
documented in
[`hardware-validation-2026-11-23-load-test.md`](hardware-validation-2026-11-23-load-test.md):
Mesa loaded the vendor MUSA drivers for the S30 DRM card and mutter segfaulted
in its KMS thread. A compute-only install no longer puts those drivers where
Mesa can find them.

## Limits

A successful COPR build proves that the RPMs are *produced and installable*.
It does not prove that the kernel driver binds, that the GPU computes, or
that the desktop survives a driver load. Those are established separately on
the hardware; see
[`hardware-validation-2026-11-23-load-test.md`](hardware-validation-2026-11-23-load-test.md).

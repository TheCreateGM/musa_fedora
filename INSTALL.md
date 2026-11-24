# Installation

The MTT S30 is present on this host, and the vendor **driver 3.1.0** plus
matching **MUSA runtime 1.5.4** are installed. The full stack has been verified
end-to-end on this physical card — see the
[2026-11-24 hardware run](docs/hardware-validation-2026-11-24-load-test.md), the
[suite verification](docs/hardware-validation-2026-11-23-suite-verify.md) and the
[container verification of the shipped packages](docs/container-verification-2026-11-23.md).

On a **normal boot** the card is bound: the RPMs ship no blacklist, and since
2026-11-24 the installed host also runs with compute mode enabled (the old
local `/etc/modprobe.d/blacklist-mtgpu.conf` was moved to a backup by
`scripts/compute-mode.sh enable`; see
[the compute-mode record](docs/compute-mode-enabled-2026-11-24.md)). A fresh
install therefore has a working compute node after the driver package builds
for the running kernel. If a host intentionally wants the S30 unbound (for
example, to compare against another display driver), use:

* persistently: `scripts/compute-mode.sh disable` (restores the boot-time
  block; takes effect on the next reboot) and `enable` to undo it;
* one-shot, no boot change: [`scripts/hardware/load-and-test.sh`](scripts/hardware/load-and-test.sh)
  under the [controlled-load runbook](docs/runbook-controlled-load-2026-11-23.md)
  for a host whose blacklist is still active.

This guide covers package workflows, IOMMU setup and driver activation; §6 lists
the recovery paths.

## Supported Fedora releases and architectures

| COPR project | Chroots | Architecture |
|---|---|---|
| `axogm/musa-fedora` | Fedora 44, 45, ELN, Rawhide | x86_64 |
| `axogm/musa-mtt-s30` | Fedora 43, 44, 45, ELN, Rawhide | x86_64 |

Both projects are community builds; no aarch64 chroot is configured and the
vendor payload is x86_64 only. Fedora 45 is the release with historical S30
hardware evidence. Other chroots are build-tested in COPR but are not
vendor-qualified or hardware-validated.

## Install from COPR

```sh
pkexec dnf install dnf-plugins-core
pkexec dnf copr enable axogm/musa-fedora      # Fedora 44/45/ELN/Rawhide
pkexec dnf copr enable axogm/musa-mtt-s30     # adds Fedora 43
pkexec dnf install musa-toolkit
```

`musa-toolkit` is a metapackage that pulls in `mtgpu-dkms`, `mtgpu-firmware`,
`musa-runtime` and `musa-sdk`. Installing these packages does **not** load
`mtgpu`, rebind the GPU, or change boot arguments. For a compute-only node keep
`musa-runtime-display` out: it ships the vendor Mesa DRI/GBM/Xorg drivers, which
Mesa picks up and which crashed GNOME Shell in its KMS thread. Install it only
if `mtgpu` is meant to drive the S30's own DP/HDMI outputs.

After installing, activate compute through §1 (IOMMU, if not already enabled)
and then the one-shot load or `compute-mode.sh enable` described at the top.
Verify with `scripts/verify-musa.sh`; it refuses to report a pass on a stack
that is not actually bound to `mtgpu`.

## Build dependencies

The DKMS package needs `dkms >= 2.8`, `kernel-devel`, `gcc`, `make`,
`elfutils-libelf-devel`, `openssl-devel`, `kmod` and `systemd-rpm-macros`.
The SDK requires `gcc-c++` at runtime for linking host programs. Runtime
packages depend on the Fedora base libraries, `libdrm`, `zlib`, `ldconfig`
and `linux-firmware`. To build RPMs locally you need `rpm-build`, `gcc`,
`make` and the matching `kernel-devel` for the DKMS binary package.

## Build from source / SRPMs

```sh
packaging/rpm/build-all.sh --srpm-only    # five SRPMs, no kernel-devel needed
TMPDIR=/var/tmp packaging/rpm/build-all.sh  # full local build (needs kernel-devel)
```

The SRPMs are what get submitted to COPR; COPR rebuilds the binary packages in
each clean chroot. See [the current status report](docs/current-state-2026-11-20.md)
for the actual build IDs and results.

## Package-managed install and removal

The validated Fedora RPMs are built from this tree under
packaging/rpm/local-build/RPMS. Build them on the target Fedora release with:

```sh
TMPDIR=/var/tmp packaging/rpm/build-all.sh
```

Review the package selection before using the package manager:

```sh
scripts/install.sh --status
scripts/install.sh --plan
scripts/install.sh --install
```

The installer verifies the exact package versions and RPM digests, makes a
backup under /var/backups/musa-fedora, and uses DNF through pkexec. It refuses
to change the driver package while mtgpu is loaded. It installs the DKMS
module but does not load or replace a live module and does not edit boot
arguments. A module load or reboot must be handled in a reviewed maintenance
window.

Removal uses the matching package workflow:

```sh
scripts/uninstall.sh --plan
scripts/uninstall.sh --remove
```

The removal helper backs up the active-kernel module and MUSA configuration,
uses DNF with --no-autoremove, and refuses while mtgpu is loaded. It leaves
IOMMU boot arguments, user data, and unrelated packages unchanged. Do not use
manual rm, modprobe -r, or file-copy commands to replace/remove the live module.

The install helper does not load or replace a driver. Do not unbind/rebind the
device or replace a live module: prior unbind/rebind triggered a NULL
dereference in the vendor core. Activation is always a fresh module load, either
for one boot (`load-and-test.sh`) or persistently (`compute-mode.sh enable`).


## 1. Enable the IOMMU (required for any MUSA runtime call)

```sh
scripts/enable-iommu.sh         # invokes pkexec and backs up BLS entries
pkexec /usr/bin/systemctl reboot
```

Verify after reboot:

```sh
dmesg | grep -i 'DMAR: IOMMU enabled'
ls /sys/kernel/iommu_groups | wc -l          # must be > 0
```

`scripts/disable-iommu.sh` removes the arguments again.

## 2. Driver

The package-managed install helper installs the mtgpu 3.1.0 DKMS source and
vendor core object. The DKMS package builds for the running kernel and checks
that the module has matching vermagic and the S30 PCI ID. The standalone
builder remains useful for isolated investigation:

```sh
scripts/build-kernel-driver.sh
```

It copies source into musa-data, verifies the pinned vendor-core SHA-256, builds
out of tree, and writes the build/logs under musa-data. It does not install or
load the resulting module. Do not copy a build artifact over the loaded driver.

The IOMMU is already enabled on the current host. On another host, the reversible
enable/disable scripts use pkexec and preserve the active BLS entries. Reboot
only after reviewing the boot-argument change.

## 3. Userspace runtime

The Fedora musa-runtime and musa-sdk RPMs install the MUSA 1.5.4 runtime and
MUSA 5.2.0 compiler. The official DEB under musa-data is a reference package;
do not run its maintainer scripts or manually copy its files over an RPM-managed
installation.

## 4. Compiler

Kernels are built with the MUSA 5.2.0 `mcc`:

```sh
source scripts/musa-env.sh
mcc -O2 --offload-arch=mp_10 \
    -I/usr/local/musa-5.2/include \
    -L/usr/local/musa/lib -lmusart -Wl,-rpath,/usr/local/musa/lib \
    -o vector_add tests/musa-vector-add.mu
```

`mp_10` is the S30 (SuDi) target. The 5.2 **runtime** must not be used with the
3.1.0 driver (missing `MKT_NAME` ioctl), only the 1.5.4 runtime.
The environment script keeps the 5.2.0 compiler path while placing the
compatible 1.5.4 runtime first in `LD_LIBRARY_PATH`. For host API code, use
headers from `/usr/local/musa/include`; the SDK's 5.2.0
`musaDeviceProp` layout is not ABI-compatible with the runtime 1.5.4 header.

## 5. Verify

```sh
scripts/verify-musa.sh                  # full suite; needs a bound driver
MUSA_S30_TEST_APPROVED=1 scripts/test-gpu.sh   # single gated smoke test
```

## 6. Recovery

Everything below is reversible and was exercised on this host.

**GUI does not come up after a boot with `mtgpu` active** (the historical
Mesa/GNOME crash). The desktop does not need the card, so switch the driver off
again from a TTY (`Ctrl+Alt+F3`):

```sh
pkexec /home/axogm/Documents/project/musa-fedora/scripts/compute-mode.sh disable
pkexec /usr/bin/systemctl reboot
```

That restores the exact previous `/etc/modprobe.d` state from
`/var/backups/musa-fedora-compute-mode/` (or writes a fresh blacklist if there
is no backup) and regenerates the initramfs with `dracut`, so the next boot is
the safe unbound one. Full background: [boot fix](docs/boot-fix-2026-11-23.md).

**Kernel fault / Oops logged by `mtgpu`.** Do not reuse the boot; the driver logs
are not trustworthy after a fault. Reboot, and keep the card off until the next
reviewed load. See [clean-boot resume plan](docs/clean-boot-resume-plan-2026-11-17.md).

**If IBT/`CONFIG_X86_KERNEL_IBT` blocks the module** ("missing ENDBR"): follow
[IBT-off recovery](docs/ibt-off-recovery.md); it is a temporary boot-argument
change with its own rollback.

**A kernel update replaced the running kernel** and the module no longer
builds: [kernel update recovery](docs/kernel-update-recovery.md) and
`scripts/dkms-kernel-update.sh`.

**Kernel update left no working module at all** (`vermagic` mismatch, DKMS
failed): rebuild from source with `scripts/build-kernel-driver.sh` instead of
hand-copying an artifact, then reload on a later boot. Never copy a `.ko` over a
loaded module.

**Boot-argument and bootloader rollback.** `scripts/enable-iommu.sh` and
`scripts/disable-iommu.sh` back up the BLS entries they rewrite; the
`setup`-stage backups live in `musa-data/research/system-backups/*/bls-entries/`
and restore with `pkexec /usr/bin/cp -a <backup>/. /boot/loader/entries/`. The
previous in-use module is preserved as
`musa-data/research/system-backups/*/mtgpu.ko.in-use`.

**Last-resort console recovery.** If GNOME and GDM both fail, boot the previous
kernel from the GRUB menu, or append `3` to the kernel command line for a
multi-user target; the S30 is not needed to boot or to log in.

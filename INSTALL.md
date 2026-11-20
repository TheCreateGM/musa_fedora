# Installation

The MTT S30 is present on this host, and the vendor **driver 3.1.0** plus
matching **MUSA runtime 1.5.4** are installed. The driver is currently
blacklisted and the card is unbound to protect the Intel-driven desktop after
prior DRM/KMS crashes; MUSA GPU workloads are not currently available. See the
[current live status](docs/current-state-2026-11-20.md). This guide describes
package workflows and IOMMU setup; it does not authorize loading the driver.

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

The five RPMs are installed on this host. The module is currently not loaded,
and the `mtgpu` blacklist remains in place; the install helper does not load or
replace a driver. Do not remove the blacklist or load/unbind/rebind the driver
without a reviewed display rollback and vendor-core recovery plan.


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
scripts/verify-musa.sh
```

## Recovery

* Boot entry backups: `musa-data/research/system-backups/*/bls-entries/`
* Previous module: `musa-data/research/system-backups/*/mtgpu.ko.in-use`
* To restore a boot entry: `pkexec /usr/bin/cp -a <backup>/. /boot/loader/entries/`

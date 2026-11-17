# Installation

The MTT S30 on this host already has the vendor **driver 3.1.0** loaded and the
matching **MUSA runtime 1.5.4** installed. This document describes how to
reproduce that state and how to enable the required IOMMU.

## 1. Enable the IOMMU (required for any MUSA runtime call)

```sh
sudo scripts/enable-iommu.sh     # grubby: intel_iommu=on iommu=pt
sudo reboot
```

Verify after reboot:

```sh
dmesg | grep -i 'DMAR: IOMMU enabled'
ls /sys/kernel/iommu_groups | wc -l          # must be > 0
```

`scripts/disable-iommu.sh` removes the arguments again.

## 2. Driver

```sh
scripts/build-kernel-driver.sh               # builds against the running kernel
```

The script copies the driver source to `musa-data`, verifies the pinned SHA-256
of the closed vendor core object (`mtgpu_core.o_binary`), builds out-of-tree, and
verifies `vermagic`. It does **not** install or load anything.

Install the resulting `mtgpu.ko` into `/lib/modules/$(uname -r)/extra/` and run
`depmod`, or package it with `packaging/rpm/SPECS/mtgpu-dkms.spec`.

Kernel options: `/etc/modprobe.d/mtgpu.conf` should contain only the vendor
default

```
options mtgpu mtgpu_drm_major=2
```

Do **not** set `rm_feature_level=1` (non-default RM mode) unless you are
deliberately testing it.

## 3. Userspace runtime

Install `musa_3.1.0-pc_x86.deb` from
`musa-data/research/official-musa-3.1.0-s30/` by extracting it, not by running
the Debian maintainer scripts:

```sh
dpkg-deb -x musa_3.1.0-pc_x86.deb /tmp/musa-pkg
sudo cp -a /tmp/musa-pkg/usr/local/musa/lib/.   /usr/local/musa/lib/
sudo cp -a /tmp/musa-pkg/usr/local/musa/include/. /usr/local/musa/include/
sudo cp -a /tmp/musa-pkg/usr/local/musa/bin/.   /usr/local/musa/bin/
sudo cp -a /tmp/musa-pkg/usr/lib/x86_64-linux-gnu/. /usr/lib64/
sudo cp -a /tmp/musa-pkg/etc/sysctl.d/99-musa.conf /etc/sysctl.d/
sudo ldconfig
sudo sysctl --system          # vm.max_map_count=0x7fffffff
```

## 4. Compiler

Kernels are built with the MUSA 5.2.0 `mcc`:

```sh
mcc -O2 --offload-arch=mp_10 \
    -I/usr/local/musa-5.2/include \
    -L/usr/local/musa/lib -lmusart -Wl,-rpath,/usr/local/musa/lib \
    -o vector_add tests/musa-vector-add.mu
```

`mp_10` is the S30 (SuDi) target. The 5.2 **runtime** must not be used with the
3.1.0 driver (missing `MKT_NAME` ioctl), only the 1.5.4 runtime.

## 5. Verify

```sh
scripts/verify-musa.sh
```

## Recovery

* Boot entry backups: `musa-data/research/system-backups/*/bls-entries/`
* Previous module: `musa-data/research/system-backups/*/mtgpu.ko.in-use`
* To restore a boot entry: `sudo cp -a <backup>/. /boot/loader/entries/`

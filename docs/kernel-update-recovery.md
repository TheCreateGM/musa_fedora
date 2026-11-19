# Kernel updates, DKMS, and recovery

The MTT S30 is driven by the `mtgpu` kernel module, distributed through DKMS
as the `mtgpu-dkms` RPM. A kernel update changes the module ABI: a module
built for `7.3.3-300.fc45.x86_64` cannot be loaded into a different kernel
because the vermagic no longer matches. This document describes how the stack
survives kernel updates, how to verify it afterwards, and how to recover when
a kernel update breaks the module.

## How the stack reacts to a kernel update

1. Installing a new kernel package runs `dkms autoinstall` from the kernel
   package transaction. DKMS rebuilds `mtgpu` for the new kernel from the
   source in `/usr/src/mtgpu-3.1.0`.
2. If that build succeeds, the new kernel has a matching `mtgpu.ko`, and the
   S30 is bound on the next boot exactly as it is now.
3. If the build fails, the new kernel still boots. The desktop stays on
   `i915`; the S30 simply has no driver in that kernel. The old kernel keeps
   its module and remains a working fallback.

This is the preferred behaviour: a kernel update must never be able to
produce an unbootable system because of a GPU driver.

The RPM's `%post` scriptlet additionally builds and installs the module for
the running kernel at package install time. It logs failures but does not
abort the transaction for the same reason.

## Inspecting the current state

```sh
scripts/dkms-kernel-update.sh status
```

This prints the running kernel, the DKMS registration state, and the module
file for every kernel under `/lib/modules`. It is read-only.

To check that the installed module for a specific kernel is really the
expected driver:

```sh
scripts/dkms-kernel-update.sh verify --kernel 7.3.3-300.fc45.x86_64
```

The check confirms that the module has a vermagic for that kernel and that it
advertises the MTT S30 PCI ID (`pci:v00001ED5d00000102`).

## Building and installing the module for a new kernel

Use this only when the automatic DKMS rebuild did not produce a module, for
example because `kernel-devel` for the new kernel was missing at build time.

```sh
# 1. the build step alone changes nothing outside /var/lib/dkms
scripts/dkms-kernel-update.sh build --kernel 7.4.0-300.fc45.x86_64

# 2. install it; the previous module file is backed up first
scripts/dkms-kernel-update.sh install --kernel 7.4.0-300.fc45.x86_64

# 3. confirm the result
scripts/dkms-kernel-update.sh verify --kernel 7.4.0-300.fc45.x86_64
```

Guarantees enforced by the script:

* it never unloads a module, rebinds the S30, or edits modprobe or boot
  configuration;
* it refuses to install a module for the **running** kernel while that
  kernel's module is loaded, so the validated live module cannot be replaced
  underneath a running system;
* the module file it is about to replace is copied to
  `/var/backups/musa-fedora/dkms/<timestamp>-<kernel>/` before the change;
* every privileged step is visible and runs through `pkexec`;
* every step can be inspected first with `--dry-run`.

After the new module is installed, boot the new kernel and re-run the
hardware verification:

```sh
scripts/verify-musa.sh
```

## Rolling back to a previous module

```sh
scripts/dkms-kernel-update.sh rollback --kernel 7.4.0-300.fc45.x86_64
```

This restores the newest backup taken by `install` for that kernel and
re-runs `depmod`. As with the install path, it refuses to overwrite the
module of the running kernel.

## Recovery procedures

### The S30 does not appear after a kernel update

1. Check whether a module exists for the booted kernel:
   `scripts/dkms-kernel-update.sh status`.
2. If the module is missing, install `kernel-devel` for that kernel and run
   `scripts/dkms-kernel-update.sh install --kernel "$(uname -r)"` from a
   maintenance boot of that kernel (the script refuses while the module is
   loaded, which is the intended behaviour).
3. If the module is present but the device is unbound, check
   `dmesg | grep -i mtgpu`. Do **not** unbind or rebind the device: a
   historical rebind attempt faulted inside
   `mtgpu_pstate_notifier_call_chain`
   ([root cause](root-cause-rebind-2026-11-16.md)).
4. Boot the previous kernel from the GRUB menu ("Fedora Linux ... previous
   version"). The previous kernel keeps its module and the S30 works there.

### The module fails to build for a new kernel

The port in this tree targets the Fedora 7.3 kernel API and has been adapted
for the 7.4 DRM reset-hook removal and the `drm/clients/drm_fbdev_helper.h`
header move. A newer kernel may need further adjustments:

1. Build out of tree first, without touching the system:
   `scripts/build-kernel-driver.sh "$(uname -r)"` writes the module and the
   build log under `musa-data/research/builds/`.
2. Apply the fix to `src/` and `inc/` in this repository, rebuild, and only
   then update the DKMS source in `/usr/src/mtgpu-3.1.0` through the RPM.
3. Never copy a hand-built artifact over the loaded module.

### Removing the driver completely

```sh
scripts/rpm-stack.sh remove
```

The helper removes only the five MUSA/MTGPU packages, with `--no-autoremove`,
and refuses while `mtgpu` is loaded. It leaves IOMMU boot arguments, the
desktop graphics stack, and user data untouched.

## What must not be done

* Do not unload or rebind the live `mtgpu` module.
* Do not replace the on-disk module of the running kernel.
* Do not disable IBT/CET, patch the vermagic, or modify the vendor core
  object.
* Do not install the Ubuntu DEB packages over the RPM-managed installation.
* Do not install the archived MobiMaliang 2.1.1 OpenCL ICD; it caused a
  kernel Oops (see [the current state](current-state-2026-11-17.md)).

# Root cause: mvgal-dkms "Differences between built and installed modules"

Date: 2026-11-23. Host: Fedora 45, kernel `7.3.4-300.fc45.x86_64`,
`mvgal-dkms-0.7.48-1.fc45` (COPR `axogm/mvgal`), dkms 3.4.4, Secure Boot
disabled.

## Symptom

After installing `mvgal-dkms` 0.7.48 on the host:

* `dkms status` reported
  `mvgal/0.7.48, 7.3.4-300.fc45.x86_64: installed (Original modules exist) (Differences between built and installed modules)`
* `rpm -V mvgal-dkms` reported
  `missing /usr/lib/modules/7.3.4-300.fc45.x86_64/kernel/drivers/gpu/drm/mvgal/mvgal_ntsync.ko.xz`
* `modinfo -F filename mvgal_ntsync` resolved to
  `/lib/modules/7.3.4-300.fc45.x86_64/extra/mvgal_ntsync.ko.xz`, a file owned
  by **no package** (`rpm -qf` → "not owned"), while the other six mvgal
  modules resolved to the RPM-owned `kernel/drivers/gpu/drm/mvgal/` path.

## Root cause

The install-time `%post` scriptlet (captured verbatim in `/var/log/dnf5.log`,
2026-11-23T00:40:16–00:41:04) shows dkms 3.4.4 taking **two different code
paths** for the seven built modules, keyed on the ELF `version:` field:

```
Module .../drm/mvgal/mvgal.ko.xz already installed at version 0.7.48, override by specifying --force
Module .../mvgal_nvidia.ko.xz already installed at version 0.3.0, override by specifying --force
Module .../mvgal_amd.ko.xz already installed at version 0.3.0, override by specifying --force
Module .../mvgal_intel.ko.xz already installed at version 0.3.0, override by specifying --force
Module .../mvgal_mtt.ko.xz already installed at version 0.3.0, override by specifying --force
Module .../mvgal_adreno.ko.xz already installed at version 0.1.0, override by specifying --force
Found pre-existing /lib/modules/.../drm/mvgal/mvgal_ntsync.ko.xz, archiving for uninstallation
Installing /lib/modules/7.3.4-300.fc45.x86_64/extra/mvgal_ntsync.ko.xz
```

For the six modules that carry a `MODULE_VERSION` (`mvgal.ko` = 0.7.48; the
vendor sub-drivers still embed stale 0.3.0/0.1.0 strings), dkms detected an
"already installed" module, refused to replace it without `--force`, and left
the packaged files untouched.

`kernel/mvgal_ntsync.c` declares `MODULE_LICENSE("GPL")` but **no
`MODULE_VERSION`** (verified at `/usr/src/mvgal-0.7.48/kernel/mvgal_ntsync.c:857`),
so dkms took the "archive the pre-existing file and install mine" branch.
That branch (a) deleted the RPM-owned file at the
`kernel/drivers/gpu/drm/mvgal/` path ("archiving for uninstallation"), making
`rpm -V` report it *missing*, and (b) installed the DKMS copy into
`.../extra/` despite the fixed `dkms.conf` setting
`DEST_MODULE_LOCATION[1]="/kernel/drivers/gpu/drm/mvgal"` — the archive
branch in dkms 3.4.4 does not honor the custom destination.

The packaged `dkms.conf` comments claim the DEST/INSTALL_MOD_DIR alignment
fixed this class of bug; the log above proves it is **still broken for the
fresh-install case**, and the trigger is specifically the missing
`MODULE_VERSION` in the ntsync module.

Consequences on a live system: `depmod` searches `extra/` before the
`kernel/` tree, so the kernel loaded ntsync from the unowned `extra/` file at
boot (visible in `modules.dep`), while the RPM database still owned (and
missed) the DRM-path file.

## Host remediation (performed 2026-11-23, no module loads/unloads)

1. Backed up the `extra/` copy (sha256
   `23321cfce68eb1805bc36dff63ed959832efe20db45452f386db45049017374e`) to
   `musa-data/host-backups/2026-11-23-mvgal-remediation/` (gitignored).
2. `pkexec install -m 644 -D` that same MVGAL-key-signed file into
   `/usr/lib/modules/7.3.4-300.fc45.x86_64/kernel/drivers/gpu/drm/mvgal/mvgal_ntsync.ko.xz`,
   removed the unowned `extra/` orphan, and ran `depmod -a 7.3.4-300.fc45.x86_64`.

Verified after: `rpm -V mvgal-dkms` shows only `S.5` content differences for
the seven `.ko.xz` files (expected: the spec re-signs modules in place with
the MVGAL key, and dkms rebuilds differ from the COPR-built bytes; no file is
`missing` anymore), `modinfo -F filename mvgal_ntsync` resolves to the
RPM-owned DRM path with the backed-up hash, `modules.dep` has zero `extra/`
references, and `lsmod` still shows the same `mvgal`/`mvgal_ntsync`/`mtgpu`
refcounts — the running kernel modules were never touched.

`dkms status` still prints "(Differences between built and installed
modules)" for the package as a whole: that flag compares dkms's own build
tree against the installed (re-signed) files and is benign under this
package's re-sign design.

## Recurrence and upstream fixes (for the mvgal repository)

The anomaly **returns on every future kernel update** for ntsync: the dkms
autoinstall rebuild will again "archive" the previous kernel's DRM-path file
and install ntsync into `extra/`. Proper fixes, in order of preference:

1. Add `MODULE_VERSION` to `kernel/mvgal_ntsync.c` (and refresh the stale
   0.3.0/0.1.0 strings in the vendor sub-drivers, plus the boot-log strings
   "Initialized mvgal 0.2.6" / "NTSYNC module loaded (v0.3.0)" which
   contradict package 0.7.48). Then dkms uniformly takes the
   "already installed… override" branch and the packaged file stays in place
   (with `dkms install --force` in `%post` where replacement is intended).
2. Alternatively, ship **no prebuilt** module files in the RPM and let dkms
   be the single provider, so there is no "pre-existing" file to archive.
3. The remediation one-liner for an affected host (any kernel version):

```sh
K=$(uname -r)
pkexec sh -c "install -m 644 -D /lib/modules/$K/extra/mvgal_ntsync.ko.xz \
  /usr/lib/modules/$K/kernel/drivers/gpu/drm/mvgal/mvgal_ntsync.ko.xz && \
  rm -f /lib/modules/$K/extra/mvgal_ntsync.ko.xz && depmod -a $K"
```

Only apply when `rpm -V mvgal-dkms` reports the DRM-path ntsync file missing
and `modinfo -F filename mvgal_ntsync` points at `extra/`.

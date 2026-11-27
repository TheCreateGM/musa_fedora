%define debug_package %{nil}
# The vendor core object (objs/x86_64/mtgpu_core.o_binary) is an opaque,
# pre-compiled ELF blob. Fedora's automatic binary re-packaging (BRP) strip
# pass rewrites such objects, so every DKMS payload published up to release 9
# differed from the pinned vendor bytes (33 sections with debug_info shrank
# to 22; sha256 d0fdace... became 31bb853f...). Disable the strip hooks so
# the installed source tree keeps the core byte-identical to the pinned
# blob, mirroring the mtgpu-firmware 3.1.0-4 fix for GPU ELF payloads.
%global __brp_strip %{nil}
%global __brp_strip_comment_note %{nil}
%global __brp_strip_static_archive %{nil}

Name:           mtgpu-dkms
Version:        3.1.0
Release:        17%{?dist}
Summary:        DKMS kernel driver source for Moore Threads GPUs (mtgpu)

License:        GPL-2.0-only
URL:            https://www.mthreads.com
Source0:        mtgpu-dkms-3.1.0.tar.gz

BuildArch:      noarch
Requires:       dkms >= 2.8
Requires:       kernel-devel
Requires:       gcc
Requires:       make
Requires:       elfutils-libelf-devel
Requires:       openssl-devel
Requires:       kmod
Recommends:     mtgpu-firmware

# Needed so that the %check build can compile the module in the build chroot.
BuildRequires:  kernel-devel
BuildRequires:  gcc
BuildRequires:  make
BuildRequires:  elfutils-libelf-devel
BuildRequires:  openssl-devel
BuildRequires:  kmod
# Defines the udev rules directory macro used below; without it the %files
# entry for 70-mtgpu.rules expands to a literal macro name and rpmbuild aborts
# with "File must begin with /".
BuildRequires:  systemd-rpm-macros

%description
The mtgpu-dkms package installs the Moore Threads mtgpu kernel driver source
tree under /usr/src/mtgpu-3.1.0 and registers it with DKMS, so that a loadable
mtgpu module is rebuilt automatically for the running kernel and for future
kernel updates.

This is the 3.1.0 driver release, the version validated in this project against
the MTT S30 on Fedora Linux. The package ships the complete driver source
together with the vendor core object (objs/x86_64/mtgpu_core.o_binary) that is
required to link a loadable module. See docs/driver-porting.md for the Fedora
kernel-porting work (conftest fixups, DRM ABI version, OB configuration
fix-up, vmalloc fallback).

The resulting module advertises the MTT S30 / S80 / S5000 PCI IDs. The opaque
vendor core object emits extensive objtool warnings on Fedora kernels,
including missing-ENDBR relocations and RETPOLINE naked returns. The module's
GNU property note "x86 feature: IBT" does not establish safe IBT/CET runtime
behavior. Successful packaging and kernel compilation are build checks only;
they are not approval to load the module on a Fedora desktop.

After installation the module is built immediately for the running kernel. Load
it with:

    modprobe mtgpu

This package ships /etc/modprobe.d/90-mtgpu-headless.conf, which defaults the
driver to headless compute mode (`display=none disable_fbdev=1`): the S30 is
used for compute and does not register KMS connectors or an fbdev, so a
desktop already driven by i915/amdgpu is left untouched. Users who want the
S30 to drive its own DP/HDMI outputs should switch that file to
`display=mt`.

Firmware is provided by mtgpu-firmware; install it first, otherwise the driver
probes but cannot bring the GPU up. With Secure Boot enabled the DKMS MOK must
be enrolled (mokutil --import /var/lib/dkms/mok.pub) before the module can be
loaded.

%prep
%autosetup -n mtgpu-%{version}
# The vendor core object is mandatory; a source tree without it links nothing.
test -f objs/x86_64/mtgpu_core.o_binary
test -f dkms.conf

%build
# Compile validation for the chroot kernel so packaging errors and Fedora
# kernel API breakage surface at build time. DKMS still builds the shipping
# module against the user's running kernel at install time.
%if 0%{?fedora} || 0%{?rhel}
# Fedora kernel-devel installs the tree under /usr/src/kernels; the
# /lib/modules/<ver>/build symlink only exists once a kernel image package is
# installed, which build chroots never have.
# Prefer the running kernel's tree when it exists (local build), otherwise
# use the newest kernel-devel present (mock chroots install exactly one).
if [ -f "/usr/src/kernels/$(uname -r)/Makefile" ]; then
    KVER="$(uname -r)"
else
    KVER="$(ls /usr/src/kernels 2>/dev/null | sort -V | tail -1)"
fi
if [ -n "$KVER" ] && [ -f "/usr/src/kernels/$KVER/Makefile" ]; then
    echo "compiling mtgpu against kernel-devel $KVER"
    make ARCH=x86_64 KERNELVER="$KVER" \
         KERNELDIR="/usr/src/kernels/$KVER" \
         KERNELSRC="/usr/src/kernels/$KVER" \
         -j"${RPM_BUILD_NCPUS:-4}" all
    modinfo mtgpu.ko | grep -q 'pci:v00001ED5d00000102'
    # -F vermagic prints "<ver> SMP preempt mod_unload ..."; match the prefix.
    modinfo -F vermagic mtgpu.ko | grep -q "^$KVER "
else
    echo "no kernel-devel tree in chroot, skipping compile check"
fi
%endif

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}/usr/src/mtgpu-%{version}
cp -a . %{buildroot}/usr/src/mtgpu-%{version}/

# Drop build outputs of the %build check; DKMS rebuilds from clean source.
find %{buildroot}/usr/src/mtgpu-%{version} \
    \( -name '*.o' -o -name '*.o.d' -o -name '.*.cmd' -o -name '*.ko' \
       -o -name '*.mod' -o -name '*.mod.c' -o -name 'conftest.h' \
       -o -name 'mtgpu_core.o' -o -name 'modules.order' \
       -o -name 'Module.symvers' \) \
    -delete 2>/dev/null || true

install -D -m 0644 90-mtgpu-headless.conf \
    %{buildroot}%{_sysconfdir}/modprobe.d/90-mtgpu-headless.conf
install -D -m 0644 70-mtgpu.rules %{buildroot}%{_udevrulesdir}/70-mtgpu.rules

# The mtgpu module writes event reports and FEC debug logs to /var/log/mtgpu
# and logs ENOENT warnings at every load when the directory is absent
# (observed 2026-11-23 on the S30 host). Create it at boot and at package
# install so the vendor logging works from any load path, including an
# initramfs-time load.
mkdir -p %{buildroot}%{_tmpfilesdir}
cat > %{buildroot}%{_tmpfilesdir}/mtgpu.conf <<'MTGPU_TMPFILES'
d /var/log/mtgpu 0755 root root -
d /var/log/mtgpu/event_report 0755 root root -
MTGPU_TMPFILES

# Keep the mtgpu module out of the initramfs.
#
# The vendor driver creates its event-report directories at module
# initialisation: /var/log/mtgpu, /var/log/mtgpu/event_report and one directory
# per device UUID.  dracut in hostonly mode pulls the module into the
# initramfs because the MTT S30 is present in the host, and udev coldplugs it
# there.  The initramfs has no /var/log, so every one of those creations fails
# with -ENOENT, mtgpu_event_report_persist_init() aborts before it opens the
# XID record file, and the driver logs "xid_record file create fd failed".
# The GPU then runs for the whole lifetime of the machine without ever
# writing an XID record, so GPU faults are silently lost.
#
# Excluding the module from the initramfs makes the driver initialise in the
# real root, where /var/log/mtgpu exists and the XID record file lives on a
# persistent filesystem.  The module is still loaded automatically in the real
# root by udev (the PCI modalias of 1ed5:0102 matches the mtgpu alias), so no
# manual action and no blacklist are involved.
mkdir -p %{buildroot}%{_sysconfdir}/dracut.conf.d
cat > %{buildroot}%{_sysconfdir}/dracut.conf.d/99-musa-mtgpu.conf <<'MTGPU_DRACUT'
# Exclude the Moore Threads mtgpu driver from the initramfs so that it
# initialises in the real root, where /var/log/mtgpu exists and the vendor
# event-report/XID record file can be created and kept.
#
# Without this, dracut (hostonly mode) loads mtgpu inside the initramfs, where
# /var/log does not exist; the driver then logs
#   Failed to create directory: /var/log/mtgpu, ret = -2
#   ...
#   xid_record file create fd failed
# at every boot and no GPU fault record is ever written.
omit_drivers+=" mtgpu "
MTGPU_DRACUT

%files
%defattr(-,root,root,-)
%doc README.md
/usr/src/mtgpu-%{version}/
%config(noreplace) %{_sysconfdir}/modprobe.d/90-mtgpu-headless.conf
%{_udevrulesdir}/70-mtgpu.rules
%{_tmpfilesdir}/mtgpu.conf
%config(noreplace) %{_sysconfdir}/dracut.conf.d/99-musa-mtgpu.conf

%post
# Register, build and install for the running kernel. Failures are reported but
# do not abort the transaction: on a machine without this GPU, or without a
# matching kernel-devel tree, "modprobe mtgpu" later still works after a
# "dkms autoinstall" once kernel-devel is in place.
# $1 is 1 on first install and 2 on upgrade. The previous release only
# rebuilt on first install ("-eq 1"), so upgrading an installed DKMS package
# left the old module in /lib/modules while replacing /usr/src/mtgpu-*.
# Rebuild and reinstall for the running kernel on both paths.
if [ "$1" -ge 1 ]; then
    dkms add     -m mtgpu -v %{version} --rpm_safe_upgrade || :
    dkms build   -m mtgpu -v %{version} -k "$(uname -r)" --force || :
    dkms install -m mtgpu -v %{version} -k "$(uname -r)" --force || :
fi
install -d -m 0755 /var/log/mtgpu/event_report 2>/dev/null || :
depmod -a >/dev/null 2>&1 || :
# The dracut exclusion of this module only takes effect after the initramfs is
# rebuilt.  Rebuild it for the running kernel when possible; a failure here is
# not fatal, the module simply stays in the initramfs and the XID record file
# is still not written.
if [ -x /usr/bin/dracut ] && [ -d "/lib/modules/$(uname -r)" ]; then
    /usr/bin/dracut --force --kver "$(uname -r)" >/dev/null 2>&1 || \
        echo "WARNING: dracut could not rebuild the initramfs for $(uname -r);" \
             "run 'dracut --force' manually so that mtgpu is kept out of it." >&2
fi

%preun
if [ "$1" -eq 0 ]; then
    dkms remove -m mtgpu -v %{version} --all || :
fi

%postun
depmod -a >/dev/null 2>&1 || :

%changelog
* Tue Nov 27 2026 MUSA Fedora Team <musa@fedoraproject.org> - 3.1.0-17
- Keep the mtgpu module out of the initramfs (omit_drivers+=" mtgpu" in
  /etc/dracut.conf.d/99-musa-mtgpu.conf).  The vendor driver creates its
  event-report directories at module initialisation, but dracut in hostonly
  mode loads the module inside the initramfs, where /var/log does not exist.
  Every directory creation then failed with -ENOENT, the driver logged
  "Failed to create directory: /var/log/mtgpu, ret = -2" and
  "xid_record file create fd failed" at every boot, and no GPU fault record
  was ever written: the XID record file in /var/log/mtgpu has been unchanged
  since 2026-11-19 on the validation host.  Root cause analysis, the
  disassembly of event_report_create_fd/mtgpu_drv_mkdir and the kernel probe
  that confirm it are documented in
  docs/root-cause-mtgpu-xid-initramfs-2026-11-27.md.  The driver is still
  loaded automatically in the real root by udev through the PCI modalias of
  1ed5:0102; no blacklist is used and the device stays fully enabled.
- Regenerate the initramfs from %post so that the exclusion takes effect
  without manual action.

* Wed Nov 25 2026 MUSA Fedora Team <musa@fedoraproject.org> - 3.1.0-16
- Rebuild a working objtool instead of skipping it when the kernel tree's
  prebuilt objtool cannot start (rawhide binutils SONAME drift, observed on
  the 7.4.0-0.rc3 fedora-rawhide chroot). scripts/objtool-safe.sh now
  compiles the kernel-devel objtool sources into a per-kernel cache (built
  once, under a mkdir lock, with a clean environment), then execs the
  rebuilt binary, so modules keep full ORC unwind metadata and objtool
  validation instead of linking without them. Fedora kernel-devel ships
  neither tools/build/Build nor gen-cpu-feature-names-x86.awk, so the
  staging step synthesises the one-line Build fragment, uses the shipped
  pre-generated arch/x86/lib/cpu-feature-names.c and stubs the git-only
  sync-check lint, all inside a scratch copy (nothing under /usr/src is
  modified). Parity on the 7.3.4 validation host with a forced-broken
  prebuilt objtool: .orc_unwind 0x412f2 and .orc_unwind_ip 0x2b74c and
  5502 warning lines, identical to a normal build; before this release the
  fallback path produced mtgpu.ko with .orc_header only.

* Wed Nov 25 2026 MUSA Fedora Team <musa@fedoraproject.org> - 3.1.0-15
- Lower the default host backing-page size for the system (SVM) heap from
  0x40000 (256 KiB, order 6) to 0x4000 (16 KiB, order 2) in
  inc/config_kernel.h. The S30 has no VRAM and backs every buffer object
  with host RAM, and on a 7.4 GiB Fedora host order-6 blocks are frequently
  exhausted, so musaMallocHost()/musaMalloc() failed with
  musaErrorOutOfMemory after the kernel logged
  "page allocation failure: order:6, mode:0x140cc0(GFP_USER|__GFP_COMP)".
  0x4000 is the value the driver already uses for its non-4K heap, so the
  code path is not new. See docs/host-memory-fragmentation.md.

* Wed Nov 25 2026 MUSA Fedora Team <musa@fedoraproject.org> - 3.1.0-14
- Add an objtool wrapper (scripts/objtool-safe.sh) and pass it to kbuild so
  out-of-tree module builds do not fail when a Fedora kernel-devel objtool is
  present but cannot start because its binutils SONAME is no longer installed
  (observed on rawhide 7.4.0-0.rc3 kernel-devel). When objtool runs normally,
  the wrapper execs it unchanged; when objtool cannot start, it logs a loud
  warning and exits 0 so the module still links. Modules linked without objtool
  lack ORC unwind metadata for their own code, but load and run.

* Tue Nov 24 2026 MUSA Fedora Team <musa@fedoraproject.org> - 3.1.0-13
- Ship the rm_feature_level rationale in 90-mtgpu-headless.conf. The option
  itself has been absent since 3.1.0-10, but only as a comment-less change, so
  every host carried a `%config(noreplace)` local edit whose content differed
  from the packaged file for a reason the file did not state (rpm -V flagged it
  as S.5....T, and a stale .rpmnew accumulated). No option value changes: the
  file is now byte-identical to the validated 2026-11-24 host file. [ਸ਼

* Mon Nov 23 2026 MUSA Fedora Team <musa@fedoraproject.org> - 3.1.0-12
- Ship a tmpfiles.d entry creating /var/log/mtgpu and /var/log/mtgpu/event_report,
  and create them in %post. The mtgpu module writes event reports and FEC debug
  files there (strings in the vendor core blob) and logs a warning at load when
  it cannot create its event_report tree. Scope note: on the 2026-11-23 S30 host
  these directories already existed (stale 2026-11-19 state, event_report UUID
  subdirectory with non-searchable mode) and the module still logged the same
  warnings, so this entry only guarantees the directories exist on fresh
  installs; it does not fix the vendor-core event_report failure itself.
  The warning is cosmetic: the full GPU workload passed with it present.

* Fri Nov 20 2026 MUSA Fedora Team <musa@fedoraproject.org> - 3.1.0-11
- Disable the rpmbuild BRP strip hooks. Releases 6 through 9 silently
  mangled the vendor core object in the payload: rpmbuild's automatic
  strip pass removed 11 ELF sections from mtgpu_core.o_binary, so the
  installed /usr/src tree (and any module DKMS links from it) carried a
  vendor core that matched no pinned hash and had never been hardware
  tested. The payload is now required to be byte-identical to the pinned
  core d0fdaceab37bce7387d376f8a87a2e9dbf210eeccf4f4f7c67f8dee83f4e8200.

* Fri Nov 20 2026 MUSA Fedora Team <musa@fedoraproject.org> - 3.1.0-10
- Run the DKMS add/build/install step on package upgrade as well as first
  install. The release 9 scriptlet only acted when $1 -eq 1, so upgrading an
  installed package replaced /usr/src/mtgpu-3.1.0 without rebuilding the
  module in /lib/modules. Build/install now target the running kernel and
  force a rebuild so the on-disk module matches the packaged source.

* Fri Nov 20 2026 MUSA Fedora Team <musa@fedoraproject.org> - 3.1.0-9
- Remove the unsupported claim that the module is safe to load on IBT/CET
  kernels based on its GNU property note. The opaque vendor core continues to
  emit extensive objtool warnings in the configured Fedora build chroots.

* Fri Nov 20 2026 MUSA Fedora Team <musa@fedoraproject.org> - 3.1.0-8
- Remove fatal rm_feature_level=1 option from 90-mtgpu-headless.conf.
  rm_feature_level activates an incomplete Resource Manager path in the vendor
  binary core (mtgpu_core.o_binary) which causes an immediate NULL-pointer
  dereference in gpu_phys_heap_alloc_mem (CR2: 0x128) during device initialization.
  Default rm_feature_level=0 preserves the verified PVR memory management model.

* Fri Nov 20 2026 MUSA Fedora Team <musa@fedoraproject.org> - 3.1.0-7
- BuildRequire systemd-rpm-macros so the udev rules directory macro is defined
  in the build chroot. The release 6 Copr build failed on every chroot with
  "File must begin with /" for the 70-mtgpu.rules path because the macro is not
  in the minimal build root; the local build only worked because the host has
  systemd-rpm-macros installed.

* Fri Nov 20 2026 MUSA Fedora Team <musa@fedoraproject.org> - 3.1.0-6
- The DISPC register window is absent from the component match in headless
  modes (display=none/display=dummy) because the DP/DC components are then
  skipped. inc/s30_vpu_compat.h rejected that layout (-ERANGE), so the VPU
  component bind failed, the whole DRM device aborted and the MUSA runtime
  found no render node (musaInfo "CreatePlatform failed", mthreads-gmi
  segfault). Derive the 0x116000 boundary from the pinned layout when no
  "dispc-regs" resource exists; the display=mt path is unchanged.
- Ship /etc/modprobe.d/90-mtgpu-headless.conf (display=none, disable_fbdev=1,
  rm_feature_level=1). Headless is the safe default on a Fedora desktop whose
  monitor hangs off i915/amdgpu, and it removes the historical KMS collision.
- Ship the udev rules for the musa*/mtgpu* compute nodes; they existed on the
  test host but were owned by no package.

* Tue Nov 17 2026 MUSA Fedora Team <musa@fedoraproject.org> - 3.1.0-5
- Use the DRM fbdev helper's new public header path on Linux 7.4 and newer.
  Upstream moved the helper into the DRM client library and renamed its header
  to drm/clients/drm_fbdev_helper.h; retain drm/drm_fb_helper.h on older
  kernels. Fedora 44 and 45 already build with release 4, while ELN/rawhide
  release 4 builds stopped at the removed header.

* Tue Nov 17 2026 MUSA Fedora Team <musa@fedoraproject.org> - 3.1.0-4
- Add kmod to BuildRequires: the %build compile check calls modinfo, which is
  not present in the minimal build chroot. Without it the -3 build failed with
  "modinfo: command not found" on every chroot, even though the module itself
  compiled and linked successfully.
- Handle the Linux 7.4 DRM reset-hook removal: drm_plane_funcs/drm_crtc_funcs
  lost their .reset member and drm_atomic_helper_plane_reset()/
  drm_atomic_helper_crtc_reset() were removed. Guard the four plane/crtc
  .reset initializers with LINUX_VERSION_CODE < KERNEL_VERSION(7, 4, 0) so the
  driver compiles on Fedora rawhide/ELN (7.4-rc) while keeping the validated
  7.3 path (Fedora 44/45) unchanged. Connector reset is untouched (still
  present on 7.4).

* Tue Nov 17 2026 MUSA Fedora Team <musa@fedoraproject.org> - 3.1.0-3
- Fix the chroot compile check: Fedora kernel-devel installs the kernel tree
  under /usr/src/kernels; /lib/modules/<ver>/build only exists once a kernel
  image package is installed, which mock chroots never have. The previous
  check silently skipped compilation, so the 3.1.0-2 Copr build never
  actually compiled the module. Now the module is really built against the
  chroot kernel-devel and the MTT S30 PCI ID and vermagic are verified.
- Prefer the running kernel's kernel-devel tree for local builds and fall back
  to the newest installed tree; match vermagic with a prefix test instead of
  an invalid literal-wildcard grep (which failed the build).

* Tue Nov 17 2026 MUSA Fedora Team <musa@fedoraproject.org> - 3.1.0-2
- Ship dkms.conf again (the 3.1.0-1 tarball dropped it, so "dkms add" could
  never register the tree) and give it the package name matching the
  /usr/src/mtgpu-3.1.0 directory.
- Drop the Debian-only initramfs-tools hooks; dkms_module_signing and depmod
  are enough on Fedora.
- Compile the module in %build against the chroot kernel so Fedora kernel API
  breakage fails the build instead of the user's install.
- Remove the stray build outputs from the installed source tree.

* Tue Nov 17 2026 MUSA Fedora Team <musa@fedoraproject.org> - 3.1.0-1
- Rebase the DKMS package onto the validated 3.1.0 driver source (MTT S30).

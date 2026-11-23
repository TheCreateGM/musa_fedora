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
Release:        12%{?dist}
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

%files
%defattr(-,root,root,-)
%doc README.md
/usr/src/mtgpu-%{version}/
%config(noreplace) %{_sysconfdir}/modprobe.d/90-mtgpu-headless.conf
%{_udevrulesdir}/70-mtgpu.rules
%{_tmpfilesdir}/mtgpu.conf

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

%preun
if [ "$1" -eq 0 ]; then
    dkms remove -m mtgpu -v %{version} --all || :
fi

%postun
depmod -a >/dev/null 2>&1 || :

%changelog
* Mon Nov 23 2026 MUSA Fedora Team <musa@fedoraproject.org> - 3.1.0-12
- Ship a tmpfiles.d entry creating /var/log/mtgpu and /var/log/mtgpu/event_report,
  and create them in %post. The mtgpu module writes event reports and FEC debug
  files there and logged ENOENT warnings at every load when the directory was
  absent (observed 2026-11-23 on the S30 host, kernel 7.3.4).

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

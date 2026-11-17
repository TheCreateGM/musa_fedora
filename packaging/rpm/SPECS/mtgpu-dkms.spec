%define debug_package %{nil}

Name:           mtgpu-dkms
Version:        3.1.0
Release:        5%{?dist}
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

The resulting module advertises the MTT S30 / S80 / S5000 PCI IDs, carries the
GNU property note "x86 feature: IBT", and is safe to load on Fedora kernels
with IBT/CET enabled.

After installation the module is built immediately for the running kernel. Load
it with:

    modprobe mtgpu

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

%files
%defattr(-,root,root,-)
%doc README.md
/usr/src/mtgpu-%{version}/

%post
# Register, build and install for the running kernel. Failures are reported but
# do not abort the transaction: on a machine without this GPU, or without a
# matching kernel-devel tree, "modprobe mtgpu" later still works after a
# "dkms autoinstall" once kernel-devel is in place.
if [ "$1" -eq 1 ]; then
    dkms add     -m mtgpu -v %{version} --rpm_safe_upgrade || :
    dkms build   -m mtgpu -v %{version} || :
    dkms install -m mtgpu -v %{version} --force || :
fi
depmod -a >/dev/null 2>&1 || :

%preun
if [ "$1" -eq 0 ]; then
    dkms remove -m mtgpu -v %{version} --all || :
fi

%postun
depmod -a >/dev/null 2>&1 || :

%changelog
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

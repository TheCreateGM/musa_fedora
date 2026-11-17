%define debug_package %{nil}

# Prebuilt vendor userspace binaries. brp-strip cannot process several of them
# and debuginfo extraction is not useful for a closed stack.
%define __os_install_post %{nil}

Name:           musa-runtime
Version:        1.5.4
Release:        3%{?dist}
Summary:        MUSA runtime userspace for Moore Threads GPUs (MTT S30)

License:        Proprietary
URL:            https://www.mthreads.com
Source0:        musa-runtime-1.5.4.tar.gz

# The payload is the userspace half of the vendor musa_3.1.0-pc_x86.deb
# (runtime 1.5.4), which is the release validated with the mtgpu 3.1.0 kernel
# driver on the MTT S30.
BuildArch:      x86_64
Requires:       glibc >= 2.34
Requires:       libstdc++
Requires:       libdrm >= 2.4.110
Requires:       zlib
Requires:       ldconfig
Requires:       linux-firmware
Recommends:     mtgpu-dkms >= 3.1.0
Recommends:     mtgpu-firmware >= 3.1.0

ExclusiveArch:  x86_64

%description
The musa-runtime package provides the MUSA 1.5.4 userspace runtime for Moore
Threads GPUs, as shipped in the vendor 3.1.0 PC driver release. This is the
runtime validated in this project against the MTT S30 on Fedora Linux with the
mtgpu 3.1.0 kernel driver.

Contents:
- /usr/local/musa            libmusart.so.1.5.4, libmusa.so.1.5.4, musaInfo,
                             muMemcpyTest, musa_runtime_version, headers
- /usr/lib64                 libdrm_mtgpu, mesa GL/Vulkan MUSA drivers,
                             libcapture_musa, libencode_musa, libvideo_musa
- /usr/lib64/dri, /usr/lib64/gbm, /usr/lib64/xorg/modules/drivers
                             MUSA DRI/GBM/Xorg drivers
- /usr/bin/mthreads-gmi      management/monitoring tool
- /etc                       modprobe, sysctl, Vulkan ICD and linker settings

The matching kernel driver is in mtgpu-dkms (3.1.0) and the firmware is in
mtgpu-firmware. Use musa-toolkit to pull in the complete stack.

Notes:
- The vendor deb installs its libraries under /usr/lib/x86_64-linux-gnu. This
  package maps them onto the Fedora layout (/usr/lib64 and its standard
  subdirectories) instead.
- The MUSA 5.2.0 runtime is not shipped here: it is incompatible with the
  mtgpu 3.1.0 kernel driver on the MTT S30. The 5.2.0 compiler in musa-sdk
  produces binaries that run against this 1.5.4 runtime.
- Display output stays on the firmware/i915 GPU unless the mtgpu Xorg driver is
  configured explicitly.

%prep
%autosetup -n musa-runtime-%{version}

%build
# Prebuilt vendor binaries; nothing to compile.

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}/usr/lib64/dri %{buildroot}/usr/lib64/gbm \
         %{buildroot}/usr/lib64/xorg/modules/drivers \
         %{buildroot}%{_sysconfdir}/ld.so.conf.d

# Userspace runtime tree, unchanged vendor layout under /usr/local.
mkdir -p %{buildroot}/usr/local
cp -a usr/local/. %{buildroot}/usr/local/
cp -a usr/bin/. %{buildroot}%{_bindir}/

# Vendor GL/Vulkan/DRI/Xorg payload: vendor ubuntu path -> Fedora path.
cp -a usr/lib/x86_64-linux-gnu/lib*.so* %{buildroot}/usr/lib64/
cp -a usr/lib/x86_64-linux-gnu/dri/. %{buildroot}/usr/lib64/dri/
cp -a usr/lib/x86_64-linux-gnu/gbm/. %{buildroot}/usr/lib64/gbm/
cp -a usr/lib/xorg/modules/drivers/. %{buildroot}/usr/lib64/xorg/modules/drivers/

# Configuration files.
install -Dpm 0644 etc/modprobe.d/mtgpu.conf %{buildroot}%{_sysconfdir}/modprobe.d/mtgpu.conf
install -Dpm 0644 etc/sysctl.d/99-musa.conf %{buildroot}%{_sysconfdir}/sysctl.d/99-musa.conf
install -Dpm 0644 etc/musa.ini %{buildroot}%{_sysconfdir}/musa.ini
install -Dpm 0644 etc/vulkan/icd.d/musaicdconf.json \
        %{buildroot}%{_sysconfdir}/vulkan/icd.d/musaicdconf.json

# The vendor 00-mtgpu.conf only lists the i386 loader path. Ship a Fedora
# file with the path that actually holds the runtime here.
printf '/usr/local/musa/lib\n' > %{buildroot}%{_sysconfdir}/ld.so.conf.d/musa-runtime.conf
chmod 0644 %{buildroot}%{_sysconfdir}/ld.so.conf.d/musa-runtime.conf

%post
/sbin/ldconfig
sysctl --system >/dev/null 2>&1 || true

%postun
/sbin/ldconfig

%files
%defattr(-,root,root,-)
/usr/local/musa/
/usr/local/bin/
/usr/local/include/
/usr/local/share/
/usr/lib64/lib*.so*
/usr/lib64/dri/
/usr/lib64/gbm/
/usr/lib64/xorg/
/usr/bin/mthreads-gmi
%config(noreplace) %{_sysconfdir}/modprobe.d/mtgpu.conf
%config(noreplace) %{_sysconfdir}/sysctl.d/99-musa.conf
%config(noreplace) %{_sysconfdir}/musa.ini
%config(noreplace) %{_sysconfdir}/vulkan/icd.d/musaicdconf.json
%config(noreplace) %{_sysconfdir}/ld.so.conf.d/musa-runtime.conf

%changelog
* Tue Nov 17 2026 MUSA Fedora Team <musa@fedoraproject.org> - 1.5.4-3
- Drop the obsolete mesa-libglapi package requirement. Fedora 44 and newer
  no longer publish that subpackage, and this runtime ships its own
  libglapi_musa.so.0 implementation.

* Tue Nov 17 2026 MUSA Fedora Team <musa@fedoraproject.org> - 1.5.4-2
- Map the vendor payload onto Fedora paths (/usr/lib64, /usr/lib64/dri,
  /usr/lib64/gbm, /usr/lib64/xorg/modules/drivers) instead of the Ubuntu
  multiarch directory, and ship a Fedora linker config that points at
  /usr/local/musa/lib.
- Depend on linux-firmware (owns /lib/firmware) instead of a nonexistent
  linux-firmware-filesystem, and recommend the driver/firmware packages.
- Package the Vulkan ICD, /etc/musa.ini, Xorg driver and vendor demo tools
  that the previous revision dropped.

* Tue Nov 17 2026 MUSA Fedora Team <musa@fedoraproject.org> - 1.5.4-1
- Package the validated 1.5.4 MUSA userspace runtime for the MTT S30.

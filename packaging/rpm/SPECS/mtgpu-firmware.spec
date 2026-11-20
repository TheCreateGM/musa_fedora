%define debug_package %{nil}

# The MUSA/Fedora stack currently targets x86_64 hosts. These opaque firmware
# files contain GPU ISA ELF images, not host-executable binaries; preserve their
# bytes because the x86_64 strip utility cannot process them.
%global __brp_strip %{nil}
%global __brp_strip_comment_note %{nil}

Name:           mtgpu-firmware
Version:        3.1.0
Release:        4%{?dist}
Summary:        Firmware for the Moore Threads mtgpu kernel driver

License:        Proprietary
URL:            https://www.mthreads.com
Source0:        mtgpu-firmware-3.1.0.tar.gz

BuildArch:      x86_64
ExclusiveArch:  x86_64
Requires:       linux-firmware

%description
The mtgpu-firmware package installs the firmware images required by the mtgpu
kernel driver for Moore Threads GPUs (MTT S30 and related chips). Without this
firmware the driver loads and binds, but the GPU cannot execute workloads.

Contents under /usr/lib/firmware/mthreads (also reachable through Fedora's
/lib/firmware usr-merge symlink):
- mtfw-gen*.bin, mtfw-gen*rv.elf   GPU firmware images
- mtlog-gen*.dict                  log dictionaries
- mtvpu-*.bin                      video/VPU firmware

%prep
%autosetup -n mtgpu-firmware-%{version}

%build
# Prebuilt blobs.

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}%{_prefix}/lib/firmware
cp -a lib/firmware/mthreads %{buildroot}%{_prefix}/lib/firmware/

%files
%defattr(-,root,root,-)
%{_prefix}/lib/firmware/mthreads/

%changelog
* Fri Nov 20 2026 MUSA Fedora Team <musa@fedoraproject.org> - 3.1.0-4
- Mark the package x86_64-only, matching the current MUSA/Fedora stack and
  avoiding a misleading noarch warning for GPU ISA ELF firmware images.

* Fri Nov 20 2026 MUSA Fedora Team <musa@fedoraproject.org> - 3.1.0-3
- Install firmware under /usr/lib/firmware, matching Fedora's linux-firmware
  layout and allowing the buildroot linkdupes pass to process /usr.
- Skip host strip tools on opaque GPU ELF firmware payloads; their bytes are
  device firmware and the x86_64 strip utility cannot interpret them.

* Tue Nov 17 2026 MUSA Fedora Team <musa@fedoraproject.org> - 3.1.0-2
- Require linux-firmware (which owns /usr/lib/firmware); linux-firmware-filesystem
  does not exist on Fedora.

* Tue Nov 17 2026 MUSA Fedora Team <musa@fedoraproject.org> - 3.1.0-1
- Package the mtgpu firmware blobs required by the 3.1.0 kernel driver.

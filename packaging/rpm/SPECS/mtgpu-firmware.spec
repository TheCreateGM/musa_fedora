%define debug_package %{nil}

# The firmware files are opaque vendor blobs (arch-neutral data), not
# host-executable ELF objects; allow them in a noarch package.
%define _binaries_in_noarch_packages_terminate_build 0

Name:           mtgpu-firmware
Version:        3.1.0
Release:        2%{?dist}
Summary:        Firmware for the Moore Threads mtgpu kernel driver

License:        Proprietary
URL:            https://www.mthreads.com
Source0:        mtgpu-firmware-3.1.0.tar.gz

BuildArch:      noarch
Requires:       linux-firmware

%description
The mtgpu-firmware package installs the firmware images required by the mtgpu
kernel driver for Moore Threads GPUs (MTT S30 and related chips). Without this
firmware the driver loads and binds, but the GPU cannot execute workloads.

Contents under /lib/firmware/mthreads:
- mtfw-gen*.bin, mtfw-gen*rv.elf   GPU firmware images
- mtlog-gen*.dict                  log dictionaries
- mtvpu-*.bin                      video/VPU firmware

%prep
%autosetup -n mtgpu-firmware-%{version}

%build
# Prebuilt blobs.

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}/lib/firmware
cp -a lib/firmware/mthreads %{buildroot}/lib/firmware/

%files
%defattr(-,root,root,-)
/lib/firmware/mthreads/

%changelog
* Tue Nov 17 2026 MUSA Fedora Team <musa@fedoraproject.org> - 3.1.0-2
- Require linux-firmware (which owns /lib/firmware); linux-firmware-filesystem
  does not exist on Fedora.

* Tue Nov 17 2026 MUSA Fedora Team <musa@fedoraproject.org> - 3.1.0-1
- Package the mtgpu firmware blobs required by the 3.1.0 kernel driver.

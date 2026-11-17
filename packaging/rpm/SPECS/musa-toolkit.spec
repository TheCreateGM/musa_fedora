%define debug_package %{nil}

Name:           musa-toolkit
Version:        3.1.0
Release:        1%{?dist}
Summary:        Complete MUSA stack for Moore Threads GPUs (MTT S30)

License:        Proprietary
URL:            https://www.mthreads.com
Source0:        musa-toolkit-3.1.0.tar.gz

BuildArch:      x86_64
ExclusiveArch:  x86_64

# The validated stack: driver 3.1.0 with runtime 1.5.4 and the 5.2.0 compiler.
Requires:       mtgpu-dkms = 3.1.0
Requires:       mtgpu-firmware = 3.1.0
Requires:       musa-runtime = 1.5.4
Requires:       musa-sdk = 5.2.0

%description
The MUSA Toolkit metapackage pulls in the complete, hardware-validated MUSA
stack for Moore Threads GPUs:

- mtgpu-dkms 3.1.0        kernel driver source, built through DKMS
- mtgpu-firmware 3.1.0    GPU firmware
- musa-runtime 1.5.4      MUSA userspace runtime and management tools
- musa-sdk 5.2.0          mcc/mtxas compiler toolchain

The versions are pinned because this exact combination was verified on real
MTT S30 hardware on Fedora Linux (kernel 7.3.3): driver bind, render node,
device telemetry, runtime device discovery, host<->device memcpy and a
compiled vector-add kernel that verified 1048576 results on the GPU.

The GPU must be visible for the runtime to work. An IOMMU must be enabled
(intel_iommu=on iommu=pt on Intel platforms); see docs/install.md, and
docs/troubleshooting.md if musaInfo reports a platform error.

%prep
%autosetup -n musa-toolkit-%{version}

%build
# Metapackage; nothing to build.

%install
# Metapackage; no files of its own.

%files
%doc README.md

%changelog
* Tue Nov 17 2026 MUSA Fedora Team <musa@fedoraproject.org> - 3.1.0-1
- Rebase the metapackage onto the validated driver 3.1.0 + runtime 1.5.4 +
  compiler 5.2.0 stack instead of the non-working 5.2.0 runtime combination.

* Mon Nov 16 2026 MUSA Fedora Team <musa@fedoraproject.org> - 5.2.0-1
- MUSA 5.2.0 toolkit metapackage for Fedora.

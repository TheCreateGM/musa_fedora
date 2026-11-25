%define debug_package %{nil}

Name:           musa-toolkit
Version:        3.1.0
Release:        6%{?dist}
Summary:        Complete MUSA stack for Moore Threads GPUs (MTT S30)

License:        Proprietary
URL:            https://www.mthreads.com
Source0:        musa-toolkit-3.1.0.tar.gz

ExclusiveArch:  x86_64

# The validated stack: driver 3.1.0 with runtime 1.5.4 and the 5.2.0 compiler.
Requires:       mtgpu-dkms = 3.1.0
Requires:       mtgpu-firmware = 3.1.0
Requires:       musa-runtime = 1.5.4
Recommends:     musa-runtime-va = 1.5.4
Requires:       musa-sdk = 5.2.0

%description
The MUSA Toolkit metapackage pulls in the MUSA stack package set for Moore
Threads GPUs. This combination is validated on the MTT S30 under Fedora 45
(kernel 7.3.4):

- mtgpu-dkms 3.1.0        kernel driver source, built through DKMS
- mtgpu-firmware 3.1.0    GPU firmware
- musa-runtime 1.5.4      MUSA userspace runtime and management tools
- musa-runtime-va 1.5.4   VA-API video encode/decode driver (S30 VPU)
- musa-sdk 5.2.0          mcc/mtxas compiler toolchain

The versions are pinned to the combination exercised on real MTT S30 hardware.
Driver, runtime, compiler and VA-API paths are verified on that hardware; the
verified feature boundary is documented in docs/limitations.md. Keep the
desktop on the integrated GPU: do not install musa-runtime-display next to a
GNOME session, because the vendor Mesa driver crashes GNOME Shell on this host.

On a host with compute mode enabled, udev binds the S30 to mtgpu on a normal
boot. The GPU must be visible for the runtime to work. An IOMMU must be enabled
(intel_iommu=on iommu=pt on Intel platforms); see docs/install.md, and
docs/troubleshooting.md if musaInfo reports a platform error.

%prep
%autosetup -n musa-toolkit-%{version}

%build
# Metapackage; nothing to build.

%install
# Keep the buildroot's /usr tree present for Fedora's brp-linkdupes pass.
rm -rf %{buildroot}
mkdir -p %{buildroot}%{_prefix}

%files
%doc README.md

%changelog
* Wed Nov 25 2026 MUSA Fedora Team <musa@fedoraproject.org> - 3.1.0-6
- Refresh the README.md payload shipped as the metapackage documentation:
  the 2026-11-25 session record (boot-verify root cause and fixed re-run,
  copr 20/20 verification, musa-toolkit 3.1.0-5 activation, and the
  objtool-fallback driver release 3.1.0-16). The pinned dependency set is
  unchanged.

* Tue Nov 24 2026 MUSA Fedora Team <musa@fedoraproject.org> - 3.1.0-4
- Drop the redundant 'BuildArch: x86_64' and rely on the ExclusiveArch tag
- the spec already carried. BuildArch would emit an x86_64 RPM from any
- chroot instead of skipping non-x86_64 targets, which rpmlint reports as
- buildarch-instead-of-exclusivearch-tag. Shipped payload is unchanged.

* Fri Nov 20 2026 MUSA Fedora Team <musa@fedoraproject.org> - 3.1.0-3
- Keep the packaged status note evergreen by linking to the live project
  report instead of duplicating a package release number that changes often.

* Fri Nov 20 2026 MUSA Fedora Team <musa@fedoraproject.org> - 3.1.0-2
- Describe the prior S30 results as historical and avoid claiming the current
  Fedora driver is hardware-validated or safe to load.
- Create /usr in the buildroot so Fedora's brp-linkdupes pass has its expected
  input tree even though this package owns no files there.

* Tue Nov 17 2026 MUSA Fedora Team <musa@fedoraproject.org> - 3.1.0-1
- Rebase the metapackage onto the validated driver 3.1.0 + runtime 1.5.4 +
  compiler 5.2.0 stack instead of the non-working 5.2.0 runtime combination.

* Mon Nov 16 2026 MUSA Fedora Team <musa@fedoraproject.org> - 5.2.0-1
- MUSA 5.2.0 toolkit metapackage for Fedora.

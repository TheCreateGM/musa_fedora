%define debug_package %{nil}

# Prebuilt vendor compiler binaries (clang/mcc, mtxas, LLVM libs, clang resource
# headers, MUSA bitcode) that brp-strip cannot process reliably.
%define __os_install_post %{nil}

Name:           musa-sdk
Version:        5.2.0
Release:        3%{?dist}
Summary:        MUSA compiler toolchain (mcc, mtxas) for Moore Threads GPUs

License:        Proprietary
URL:            https://www.mthreads.com
Source0:        musa-sdk-5.2.0.tar.gz

BuildArch:      x86_64
ExclusiveArch:  x86_64
Requires:       glibc >= 2.34
Requires:       libstdc++ >= 11
Requires:       zlib
Requires:       ldconfig
Recommends:     musa-runtime >= 1.5.4

%description
The musa-sdk package provides the Moore Threads MUSA compiler toolchain for
developing MUSA applications for Moore Threads GPUs (MTT S30 and others).

This is the compiler half of the MUSA 5.2.0 SDK only: the clang-based `mcc`
compiler (target `--offload-arch=mp_10` for the MTT S30 / SuDi architecture),
the `mtxas` assembler, the LLVM support libraries, clang resource headers and
the MUSA device bitcode.

It deliberately does not provide the runtime: the runtime validated with the
mtgpu 3.1.0 driver on the MTT S30 is MUSA 1.5.4, which is in musa-runtime.
Binaries built with this compiler run against that runtime.

The toolchain installs to /usr/local/musa-5.2 and does not shadow the 1.5.4
runtime in /usr/local/musa:

    export MUSA_HOME=/usr/local/musa-5.2
    export PATH="$MUSA_HOME/bin:$PATH"
    mcc -O2 --offload-arch=mp_10 -o app app.mu -L/usr/local/musa/lib -lmusart

%prep
%autosetup -n musa-sdk-%{version}

%build
# Prebuilt vendor binaries; nothing to compile.

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}/usr/local/musa-5.2
cp -a . %{buildroot}/usr/local/musa-5.2/

# Register the compiler libraries with the dynamic linker cache. The file name
# sorts after musa-runtime.conf, so /usr/local/musa/lib (1.5.4) keeps
# precedence for -lmusart while the compiler's own libraries still resolve.
install -Dpm 0644 /dev/null %{buildroot}%{_sysconfdir}/ld.so.conf.d/musa-sdk.conf
printf '/usr/local/musa-5.2/lib\n' > %{buildroot}%{_sysconfdir}/ld.so.conf.d/musa-sdk.conf

%post
/sbin/ldconfig

%postun
/sbin/ldconfig

%files
%defattr(-,root,root,-)
/usr/local/musa-5.2/
%config(noreplace) %{_sysconfdir}/ld.so.conf.d/musa-sdk.conf

%changelog
* Tue Nov 17 2026 MUSA Fedora Team <musa@fedoraproject.org> - 5.2.0-3
- Ship the linker configuration with the package instead of relying on a
  hand-written host file.
- Only recommend musa-runtime so the compiler stays installable on its own.

* Tue Nov 17 2026 MUSA Fedora Team <musa@fedoraproject.org> - 5.2.0-2
- Reduce to the compiler-only subset; the 5.2.0 runtime is incompatible with
  the mtgpu 3.1.0 driver on the S30.

* Mon Nov 16 2026 MUSA Fedora Team <musa@fedoraproject.org> - 5.2.0-1
- Initial MUSA 5.2.0 SDK package for Fedora.

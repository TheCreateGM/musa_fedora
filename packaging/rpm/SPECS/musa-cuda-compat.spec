%define debug_package %{nil}
# The payload is shell, text headers and small assembly shims; there is nothing
# to extract debuginfo from.
%define __os_install_post %{nil}

Name:           musa-cuda-compat
Version:        1.0.0
Release:        1%{?dist}
Summary:        CUDA source compatibility layer for the MUSA runtime (MTT S30)

License:        MIT
URL:            https://github.com/TheCreateGM/musa_fedora
Source0:        %{name}-%{version}.tar.gz
BuildArch:      x86_64

BuildRequires:  make
BuildRequires:  gcc
# Link verification: the shims are checked with --no-undefined against the
# exact runtime whose symbols they jump to. The project repo supplies it.
BuildRequires:  musa-runtime = 1.5.4

# The layer maps CUDA names onto MUSA 1.5.4 symbols, so it must match the
# runtime it was generated from. Install the compiler for the nvcc wrapper.
Requires:       musa-runtime = 1.5.4
Requires:       musa-sdk >= 5.2.0
Requires:       gcc
Requires:       glibc

%description
A CUDA source-compatibility layer for the Moore Threads MUSA runtime on
Fedora.

It lets unmodified CUDA source build and run on an MTT S30:

* CUDA-named headers (cuda_runtime.h, cuda.h, cuda_fp16.h, ...) that include
  the MUSA equivalents and then map the CUDA API onto MUSA names;
* an nvcc-compatible driver (bin/nvcc) that translates an nvcc command line
  into an mcc invocation for the real device (mp_10), so CMake's CUDA
  language support, Meson and plain Makefiles work;
* libcudart / libcuda jump shims whose entry points tail-call the MUSA 1.5.4
  runtime and driver API, so pre-linked CUDA objects resolve too.

This is a source-compatibility layer, not an emulator: kernels are compiled by
mcc for the S30 and library calls land on real runtime symbols. Only the
subset of the CUDA API that the installed MUSA runtime actually exports is
mapped; anything else fails at compile or link time instead of silently doing
nothing. See the compatibility matrix in the project documentation.

The layer targets the MUSA 1.5.4 runtime because that is the runtime validated
with the mtgpu 3.1.0 kernel driver on this GPU. The separate MUSA 5.x runtime
issues a query the 3.1.0 driver does not implement, so it is not used here.

%prep
%autosetup -n %{name}-%{version}

%build
# Assemble the jump shims from the committed assembly sources, and build the
# empty device-runtime stub. CMake's CUDA support always links cudadevrt and
# cudart; MUSA bundles device code in mcc, so the stub only satisfies the name.
make all
printf '/* device runtime placeholder: MUSA bundles device code in mcc */\n' >cudadevrt.c
gcc -O2 -fPIC -shared -o libcudadevrt.so cudadevrt.c -Wl,-soname,libcudadevrt.so

%install
mkdir -p %{buildroot}%{_libdir}/%{name}/{bin,include,lib64,nvvm/libdevice}
install -m 0755 bin/nvcc bin/cuda-config bin/nvlink bin/fatbinary \
    %{buildroot}%{_libdir}/%{name}/bin/
install -m 0644 include/*.h %{buildroot}%{_libdir}/%{name}/include/
install -m 0755 libcudart.so.11.0 libcuda.so.1 \
    %{buildroot}%{_libdir}/%{name}/lib64/

# Convenience aliases inside the compat root.
ln -s libcudart.so.11.0 %{buildroot}%{_libdir}/%{name}/lib64/libcudart.so.11
ln -s libcudart.so.11.0 %{buildroot}%{_libdir}/%{name}/lib64/libcudart.so
ln -s libcuda.so.1 %{buildroot}%{_libdir}/%{name}/lib64/libcuda.so
# Static runtime variants and the empty device-runtime stub map onto the MUSA
# archives: CMake's CUDA support always links these names.
ln -s ../../../local/musa/lib/libmusart_static.a \
    %{buildroot}%{_libdir}/%{name}/lib64/libcudart_static.a
ln -s ../../../local/musa/lib/libmusa_static.a \
    %{buildroot}%{_libdir}/%{name}/lib64/libcuda_static.a
install -m 0755 libcudadevrt.so %{buildroot}%{_libdir}/%{name}/lib64/

# Toolkit discovery: CMake requires <root>/nvvm/libdevice to exist.
ln -s ../../../../local/musa-5.2/mtgpu/bitcode/libdevice.bc \
    %{buildroot}%{_libdir}/%{name}/nvvm/libdevice/libdevice.10.bc

# Also expose the shims on the default library path so a plain `-lcudart`
# works without -L, and so already-linked CUDA binaries run.
ln -s %{name}/lib64/libcudart.so.11.0 %{buildroot}%{_libdir}/libcudart.so.11.0
ln -s %{name}/lib64/libcudart.so.11.0 %{buildroot}%{_libdir}/libcudart.so.11
ln -s %{name}/lib64/libcudart.so.11.0 %{buildroot}%{_libdir}/libcudart.so
ln -s %{name}/lib64/libcuda.so.1 %{buildroot}%{_libdir}/libcuda.so.1
ln -s %{name}/lib64/libcuda.so.1 %{buildroot}%{_libdir}/libcuda.so
ln -s %{name}/lib64/libcudadevrt.so %{buildroot}%{_libdir}/libcudadevrt.so

# Conventional CUDA_HOME, but only if nothing else owns it.
mkdir -p %{buildroot}/usr/local
ln -s ../lib64/%{name} %{buildroot}/usr/local/cuda

install -d -m 0755 %{buildroot}%{_sysconfdir}/ld.so.conf.d
echo '%{_libdir}' > %{buildroot}%{_sysconfdir}/ld.so.conf.d/musa-cuda-compat.conf

install -d %{buildroot}%{_mandir}/man1
%{__gzip} -9c %{_builddir}/%{name}-%{version}/README.md > %{buildroot}%{_mandir}/man1/musa-cuda-compat.1.gz

%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig

%files
%doc README.md include/musa_cuda_compat_base.h
%{_libdir}/%{name}/
%{_libdir}/libcudart.so.11.0
%{_libdir}/libcudart.so.11
%{_libdir}/libcudart.so
%{_libdir}/libcuda.so.1
%{_libdir}/libcuda.so
%{_libdir}/libcudadevrt.so
/usr/local/cuda
%config(noreplace) %{_sysconfdir}/ld.so.conf.d/musa-cuda-compat.conf
%{_mandir}/man1/musa-cuda-compat.1.gz

%changelog
* Thu Nov 26 2026 MUSA Fedora Team <musa@fedoraproject.org> - 1.0.0-1
- Initial package. Generated CUDA-to-MUSA header maps for the subset of the
  CUDA API the 1.5.4 runtime exports, the nvcc-compatible mcc driver, and
  ABI-identical libcudart/libcuda jump shims. Validated on MTT S30 hardware
  with the 3.1.0 driver + 1.5.4 runtime combination.

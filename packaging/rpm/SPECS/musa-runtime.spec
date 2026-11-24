%define debug_package %{nil}

# Prebuilt vendor userspace binaries. brp-strip cannot process several of them
# and debuginfo extraction is not useful for a closed stack.
%define __os_install_post %{nil}

Name:           musa-runtime
Version:        1.5.4
Release:        5%{?dist}
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
- /usr/lib64                 libdrm_mtgpu, mesa GL/Vulkan MUSA libraries,
                             libcapture_musa, libencode_musa, libvideo_musa
- /usr/bin/mthreads-gmi      management/monitoring tool
- /etc                       modprobe, sysctl, Vulkan ICD and linker settings

The display drivers (DRI/GBM/Xorg) are shipped separately in the optional
musa-runtime-display subpackage. On a headless compute host they must NOT be
installed: when mtgpu is loaded, Mesa loads these drivers for the S30 DRM card
and GNOME Shell/mutter segfaults in its KMS thread. The compute runtime,
Vulkan ICD and test tools do not need them.

%package display
Summary:        MUSA Mesa DRI/GBM/Xorg display drivers for Moore Threads GPUs
Requires:       %{name} = %{version}-%{release}

%description display
Optional vendor Mesa DRI, GBM and Xorg drivers for the MTT S30 and other Moore
Threads GPUs. Install this subpackage only if the S30's own display outputs are
to be used. On a compute-only host with the desktop on i915/amdgpu, installing
these drivers causes GNOME Shell to crash when mtgpu is loaded.

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

# System-wide MUSA environment ordering. The MTT S30 runs the 1.5.4 runtime
# with the mtgpu 3.1.0 driver, while the 5.2.0 prefix only supplies the mcc
# compiler. /etc/profile.d/musa.sh can exist unowned (a legacy deb-style
# install writes MUSA_HOME=/usr/local/musa-5.2 and puts that prefix first);
# RPM does not replace a pre-existing unowned file, so shipping musa.sh would
# silently leave the incompatible ordering in place. This file sorts after it
# and re-asserts the correct order unconditionally.
mkdir -p %{buildroot}%{_sysconfdir}/profile.d
cat > %{buildroot}%{_sysconfdir}/profile.d/zz-musa-fedora.sh <<'MUSA_PROFILE_EOF'
# Moore Threads MUSA environment for the MTT S30 on Fedora.
#
# This file sorts after /etc/profile.d/musa.sh, including any unmanaged
# legacy snippet that points at the MUSA 5.2 prefix, so it always wins.
# It guarantees the ordering the mtgpu 3.1.0 driver needs:
#
#   * /usr/local/musa (1.5.4) supplies musaInfo, muMemcpyTest and
#     libmusart.so.1.5, the runtime ABI validated with the 3.1.0 driver;
#   * /usr/local/musa-5.2 supplies only the mcc compiler.
#
# Loading libmusart.so.5 from the 5.2 runtime against the 3.1.0 driver fails
# with "Invalid cmd_type" and aborts the tool. Do not reorder these prefixes.

_musa_fedora_reorder() {
    # $1 = current colon list, $2 = colon list of managed prefixes,
    # $3 = colon list to place first. Prints the reordered list.
    _mf_list=$1
    _mf_managed=$2
    _mf_out=$3
    _mf_ifs=$IFS
    IFS=:
    for _mf_entry in $_mf_list; do
        [ -n "$_mf_entry" ] || continue
        _mf_skip=0
        for _mf_prefix in $_mf_managed; do
            if [ "$_mf_entry" = "$_mf_prefix" ]; then
                _mf_skip=1
                break
            fi
        done
        [ "$_mf_skip" = 1 ] && continue
        _mf_out=${_mf_out:+$_mf_out:}$_mf_entry
    done
    IFS=$_mf_ifs
    printf '%s' "$_mf_out"
}

MUSA_RUNTIME_HOME=/usr/local/musa
MUSA_HOME=/usr/local/musa-5.2
export MUSA_RUNTIME_HOME MUSA_HOME

PATH=$(_musa_fedora_reorder "$PATH" \
    /usr/local/musa/bin:/usr/local/musa-5.2/bin \
    /usr/local/musa/bin:/usr/local/musa-5.2/bin)
export PATH

LD_LIBRARY_PATH=$(_musa_fedora_reorder "${LD_LIBRARY_PATH:-}" \
    /usr/local/musa/lib:/usr/local/musa-5.2/lib \
    /usr/local/musa/lib:/usr/local/musa-5.2/lib)
export LD_LIBRARY_PATH

unset -f _musa_fedora_reorder
unset _mf_list _mf_managed _mf_out _mf_ifs _mf_entry _mf_skip _mf_prefix
MUSA_PROFILE_EOF
chmod 0644 %{buildroot}%{_sysconfdir}/profile.d/zz-musa-fedora.sh

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
/usr/bin/mthreads-gmi
%config(noreplace) %{_sysconfdir}/modprobe.d/mtgpu.conf
%config(noreplace) %{_sysconfdir}/sysctl.d/99-musa.conf
%config(noreplace) %{_sysconfdir}/musa.ini
%config(noreplace) %{_sysconfdir}/vulkan/icd.d/musaicdconf.json
%config(noreplace) %{_sysconfdir}/ld.so.conf.d/musa-runtime.conf
%config(noreplace) %{_sysconfdir}/profile.d/zz-musa-fedora.sh

%files display
%defattr(-,root,root,-)
/usr/lib64/dri/
/usr/lib64/gbm/
/usr/lib64/xorg/

%changelog
* Mon Nov 23 2026 MUSA Fedora Team <musa@fedoraproject.org> - 1.5.4-5
- Ship /etc/profile.d/zz-musa-fedora.sh so a login shell selects the 1.5.4
  runtime (libmusart.so.1.5) ahead of the 5.2.0 compiler prefix. A legacy
  unowned /etc/profile.d/musa.sh from a disabled deb-style install puts
  /usr/local/musa-5.2 first, so an unqualified musaInfo loads
  libmusart.so.5 and aborts against the mtgpu 3.1.0 driver with
  "Invalid cmd_type". RPM does not overwrite an unowned pre-existing file,
  so the fix re-asserts the order from a later-sorting file instead.

* Mon Nov 23 2026 MUSA Fedora Team <musa@fedoraproject.org> - 1.5.4-4
- Move the vendor Mesa DRI, GBM and Xorg drivers into the optional
  musa-runtime-display subpackage. The 2026-11-23 load test on the installed
  system showed that GNOME Shell crashes in its KMS thread when Mesa picks
  up these drivers for the S30 DRM card. Compute-only users now install the
  base runtime without the display drivers.

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

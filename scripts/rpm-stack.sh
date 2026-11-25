#!/usr/bin/env bash
# Package-managed MUSA install/removal. Never unload or replace a live module.
set -euo pipefail

ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
RPM_DIR="${RPM_DIR:-$ROOT/packaging/rpm/local-build/RPMS}"
KVER="$(uname -r)"
FEDORA="$(rpm -E '%{fedora}')"
# musa-runtime-display is deliberately not part of the pinned set: its vendor
# Mesa drivers crash GNOME Shell on this host. The VA driver is safe and is
# included so a removal leaves no orphaned package behind.
PACKAGES=(mtgpu-dkms mtgpu-firmware musa-runtime musa-runtime-va musa-sdk musa-toolkit)
SELECTED_RPMS=()
ASSUME_YES=0

usage() {
    cat <<'EOF'
Usage: scripts/rpm-stack.sh install|remove [--rpm-dir DIR] [--yes]

Install verifies the exact local Fedora RPM set, backs up the active-kernel
module and MUSA configuration, then uses DNF through pkexec. It does not load
a module or change boot arguments. Remove uses DNF with --no-autoremove and
refuses while mtgpu is loaded.
EOF
}

root_cmd() {
    if (( EUID == 0 )); then "$@"
    elif command -v pkexec >/dev/null 2>&1; then pkexec "$@"
    else echo "ERROR: pkexec is required for privileged package operations." >&2; return 1
    fi
}

dnf_path() {
    if command -v dnf5 >/dev/null 2>&1; then command -v dnf5
    elif command -v dnf >/dev/null 2>&1; then command -v dnf
    else echo "ERROR: dnf5/dnf not found." >&2; return 1
    fi
}

expected_version() {
    case "$1" in
        mtgpu-dkms) echo "3.1.0-15.fc$FEDORA" ;;
        mtgpu-firmware) echo "3.1.0-5.fc$FEDORA" ;;
        musa-runtime) echo "1.5.4-7.fc$FEDORA" ;;
        musa-runtime-va) echo "1.5.4-7.fc$FEDORA" ;;
        musa-sdk) echo "5.2.0-7.fc$FEDORA" ;;
        musa-toolkit) echo "3.1.0-5.fc$FEDORA" ;;
        *) return 2 ;;
    esac
}

installed_version() {
    rpm -q --qf '%{VERSION}-%{RELEASE}' "$1" 2>/dev/null || true
}

# The vendor Mesa DRI drivers are shipped in musa-runtime-display. When they
# are installed, Mesa loads them for the S30 DRM card and GNOME Shell /
# mutter segfaults in its KMS thread, leaving no login screen. A compute-only
# host must not have them.
#
# Only the Mesa drivers count. The mtgpu_drv_video*.so VA-API drivers from
# musa-runtime-va also live in /usr/lib64/dri but are opened by libva alone,
# so they do not influence Mesa's driver selection and are safe to keep.
display_drivers_present() {
    local f base
    for f in /usr/lib64/dri/*; do
        base="${f##*/}"
        case "$base" in
            *_drv_video*.so) continue ;;
            *musa*|*mtgpu*) echo "$base"; return 0 ;;
        esac
    done
    return 1
}

check_display_stack() {
    if [[ -n "$(installed_version musa-runtime-display)" ]] || display_drivers_present; then
        echo "WARNING: vendor Mesa DRI drivers for Moore Threads are installed."
        echo "         GNOME Shell crashes when mtgpu is loaded on such a host."
        echo "         For a compute-only host: dnf remove musa-runtime-display"
    fi
}

resolve_rpms() {
    local package rpm_file metadata name version matched expected
    [[ -d "$RPM_DIR" ]] || {
        echo "ERROR: RPM directory not found: $RPM_DIR" >&2
        echo "Build the Fedora set with packaging/rpm/build-all.sh." >&2
        return 1
    }
    SELECTED_RPMS=()
    for package in "${PACKAGES[@]}"; do
        matched=""
        expected="$(expected_version "$package")"
        while IFS= read -r rpm_file; do
            metadata="$(rpm -qp --qf '%{NAME} %{VERSION}-%{RELEASE}' "$rpm_file" 2>/dev/null || true)"
            name="${metadata%% *}"
            version="${metadata#* }"
            if [[ "$name" == "$package" && "$version" == "$expected" ]]; then
                matched="$rpm_file"
                break
            fi
        done < <(find "$RPM_DIR" -type f -name '*.rpm' -print | sort)
        [[ -n "$matched" ]] || {
            echo "ERROR: missing $package $expected under $RPM_DIR" >&2
            return 1
        }
        rpm -K --nosignature "$matched"
        SELECTED_RPMS+=("$matched")
    done
}

backup_system_files() {
    local label="$1" backup path
    backup="/var/backups/musa-fedora/$label-$(date +%Y%m%d-%H%M%S)"
    root_cmd /usr/bin/install -d -m 0700 "$backup"
    for path in \
        /lib/modules/"$KVER"/extra/mtgpu.ko \
        /lib/modules/"$KVER"/extra/mtgpu.ko.xz \
        /lib/modules/"$KVER"/extra/mtgpu.ko.zst \
        /etc/modprobe.d/mtgpu.conf \
        /etc/modprobe.d/90-mtgpu-headless.conf \
        /etc/ld.so.conf.d/musa-runtime.conf \
        /etc/musa.ini \
        /etc/sysctl.d/99-musa.conf \
        /etc/vulkan/icd.d/musaicdconf.json; do
        [[ -e "$path" ]] || continue
        root_cmd /usr/bin/cp -a -- "$path" "$backup/"
    done
    echo "$backup"
}

confirm_action() {
    local message="$1" answer
    (( ASSUME_YES )) && return 0
    [[ -t 0 ]] || {
        echo "ERROR: use an interactive terminal or pass --yes after reviewing the plan." >&2
        return 2
    }
    printf '%s [y/N] ' "$message"
    read -r answer
    [[ "$answer" == "y" || "$answer" == "Y" ]]
}

ensure_dkms_module() {
    local state module vermagic aliases dkms_bin depmod_bin
    dkms_bin="$(command -v dkms)"
    depmod_bin="$(command -v depmod)"
    state="$(dkms status -m mtgpu -v 3.1.0 -k "$KVER" 2>/dev/null || true)"
    if [[ "$state" != *installed* ]]; then
        root_cmd "$dkms_bin" add -m mtgpu -v 3.1.0 --rpm_safe_upgrade || true
        root_cmd "$dkms_bin" build -m mtgpu -v 3.1.0 -k "$KVER"
        root_cmd "$dkms_bin" install -m mtgpu -v 3.1.0 -k "$KVER" --force
        root_cmd "$depmod_bin" -a "$KVER"
    fi
    module="$(modinfo -k "$KVER" -n mtgpu 2>/dev/null || true)"
    [[ -n "$module" && -e "$module" ]] || {
        echo "ERROR: mtgpu module is missing for $KVER." >&2
        return 1
    }
    vermagic="$(modinfo -F vermagic "$module")"
    [[ "${vermagic%% *}" == "$KVER" ]] || {
        echo "ERROR: mtgpu vermagic mismatch: $vermagic" >&2
        return 1
    }
    aliases="$(modinfo -F alias "$module")"
    grep -Fq 'pci:v00001ED5d00000102' <<<"$aliases" || {
        echo "ERROR: mtgpu module does not advertise the S30 PCI ID." >&2
        return 1
    }
}

do_install() {
    local package count=0 backup dnf
    [[ -f /etc/fedora-release && "$FEDORA" =~ ^[0-9]+$ ]] || {
        echo "ERROR: this RPM set requires Fedora." >&2
        return 1
    }
    for package in "${PACKAGES[@]}"; do
        [[ "$(installed_version "$package")" == "$(expected_version "$package")" ]] && ((count+=1))
    done

    if [[ -d /sys/module/mtgpu ]]; then
        if (( count == ${#PACKAGES[@]} )); then
            echo "Pinned RPM set is installed and mtgpu is live; no transaction performed."
            echo "INSTALL_STATUS: ALREADY_INSTALLED"
            return 0
        fi
        echo "ERROR: mtgpu is loaded; refusing an RPM/DKMS transaction that could replace its on-disk module." >&2
        echo "This helper never unloads or rebinds the device." >&2
        return 3
    fi

    resolve_rpms

    if (( count != ${#PACKAGES[@]} )); then
        confirm_action "Install the pinned MTT S30 RPM set for Fedora $FEDORA?" || return $?
        backup="$(backup_system_files preinstall)"
        dnf="$(dnf_path)"
        if ! root_cmd "$dnf" install -y "${SELECTED_RPMS[@]}"; then
            echo "ERROR: DNF install failed. Pre-install backup: $backup" >&2
            return 1
        fi
    else
        confirm_action "Build/install the DKMS module for kernel $KVER?" || return $?
        backup="$(backup_system_files predkms)"
    fi

    if ! ensure_dkms_module; then
        echo "ERROR: DKMS verification/build failed. Backup: $backup" >&2
        return 1
    fi
    for package in "${PACKAGES[@]}"; do
        [[ "$(installed_version "$package")" == "$(expected_version "$package")" ]] || {
            echo "ERROR: package version mismatch after install: $package" >&2
            echo "Backup: $backup" >&2
            return 1
        }
    done
    echo "RPM set and DKMS module are installed for $KVER; module was not loaded."
    echo "Boot arguments were not changed. Backup: $backup"
    check_display_stack
    echo "INSTALL_STATUS: SUCCESS"
}

do_remove() {
    local package dnf backup
    local installed=()
    for package in "${PACKAGES[@]}"; do
        [[ -n "$(installed_version "$package")" ]] && installed+=("$package")
    done
    (("${#installed[@]}")) || {
        echo "No MUSA/MTGPU RPMs are installed."
        echo "REMOVE_STATUS: ALREADY_REMOVED"
        return 0
    }
    if [[ -d /sys/module/mtgpu ]]; then
        echo "ERROR: mtgpu is loaded; refusing removal. This helper never unloads or rebinds it." >&2
        return 3
    fi
    confirm_action "Remove only these packages: ${installed[*]}?" || return $?
    backup="$(backup_system_files preremove)"
    dnf="$(dnf_path)"
    if ! root_cmd "$dnf" remove --no-autoremove -y "${installed[@]}"; then
        echo "ERROR: DNF removal failed. Pre-removal backup: $backup" >&2
        return 1
    fi
    echo "Removed selected RPMs. Boot arguments and user data were left unchanged."
    echo "Backup: $backup"
    echo "Reinstall the same Fedora RPM set to recover."
    echo "REMOVE_STATUS: SUCCESS"
}

action="${1:-}"
[[ -n "$action" ]] || { usage >&2; exit 2; }
shift || true
while (($#)); do
    case "$1" in
        --rpm-dir)
            (($# >= 2)) || { echo "ERROR: --rpm-dir needs a path." >&2; exit 2; }
            RPM_DIR="$2"
            shift 2 ;;
        --yes) ASSUME_YES=1; shift ;;
        --help|-h) usage; exit 0 ;;
        *) echo "Unknown option: $1" >&2; usage >&2; exit 2 ;;
    esac
done
case "$action" in
    install) do_install ;;
    remove) do_remove ;;
    *) echo "Unknown action: $action" >&2; usage >&2; exit 2 ;;
esac

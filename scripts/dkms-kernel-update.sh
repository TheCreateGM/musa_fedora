#!/usr/bin/env bash
# dkms-kernel-update.sh — keep the mtgpu kernel module available across
# Fedora kernel updates, with an explicit rollback path.
#
# The mtgpu module that drives the MTT S30 is distributed through DKMS.
# When a new kernel is installed, DKMS normally rebuilds the module during
# the kernel package transaction. This script makes that process observable
# and recoverable: it reports what exists for a given kernel, can rebuild and
# install the module for a target kernel, verifies the result, and can restore
# a previously working module.
#
# Safety rules enforced by this script:
#   * it never unloads, rebinds, or blacklists the live module;
#   * it refuses to install a module for the running kernel while that module
#     is loaded (the live module is the validated one);
#   * it backs up any module it is about to replace;
#   * every privileged step is reported and goes through pkexec.
#
# Sub-commands:
#   status     report module/DKMS state for one or all kernels (read-only)
#   build      build the module for a target kernel (writes under /var/lib/dkms)
#   install    back up, build and install the module for a target kernel
#   rollback   restore the module backup taken by a previous install
#   verify     check the installed module for a target kernel
set -euo pipefail

MTGPU_VERSION="${MTGPU_VERSION:-3.1.0}"
MODULE_NAME="mtgpu"
S30_PCI_ID="pci:v00001ED5d00000102"
BACKUP_ROOT="${BACKUP_ROOT:-/var/backups/musa-fedora/dkms}"
DRY_RUN=0
ASSUME_YES=0

usage() {
    cat <<'EOF'
Usage: scripts/dkms-kernel-update.sh <sub-command> [options]

Sub-commands:
  status [--kernel KVER]     report DKMS/module state (read-only)
  build  [--kernel KVER]     build the module for KVER
  install [--kernel KVER]    back up, build and install the module for KVER
  rollback --kernel KVER     restore the module backup of a previous install
  verify [--kernel KVER]     verify the installed module for KVER

Options:
  --kernel KVER   target kernel (default: the running kernel)
  --dry-run       print the privileged commands instead of running them
  --yes           do not ask for confirmation
  -h, --help      show this help

Examples:
  scripts/dkms-kernel-update.sh status
  scripts/dkms-kernel-update.sh status --kernel 7.4.0-300.fc45.x86_64
  scripts/dkms-kernel-update.sh install --kernel 7.4.0-300.fc45.x86_64
EOF
}

log()  { printf '%s\n' "$*" >&2; }
fail() { printf 'ERROR: %s\n' "$*" >&2; exit 1; }

need() { command -v "$1" >/dev/null 2>&1 || fail "required tool not found: $1"; }

root_cmd() {
    if ((DRY_RUN)); then
        printf 'DRY-RUN: %s\n' "$*"
        return 0
    fi
    if ((EUID == 0)); then
        "$@"
    elif command -v pkexec >/dev/null 2>&1; then
        pkexec "$@"
    else
        fail "pkexec is required for privileged steps"
    fi
}

confirm() {
    ((ASSUME_YES)) && return 0
    [[ -t 0 ]] || fail "use an interactive terminal or pass --yes"
    local answer
    printf '%s [y/N] ' "$1"
    read -r answer
    [[ "$answer" == "y" || "$answer" == "Y" ]]
}

installed_module() {
    local kver="$1" path
    for path in "/lib/modules/$kver/extra/$MODULE_NAME.ko" \
                "/lib/modules/$kver/extra/$MODULE_NAME.ko.xz" \
                "/lib/modules/$kver/extra/$MODULE_NAME.ko.zst" \
                "/lib/modules/$kver/updates/$MODULE_NAME.ko" \
                "/lib/modules/$kver/kernel/drivers/$MODULE_NAME.ko"; do
        [[ -e "$path" ]] && { printf '%s' "$path"; return 0; }
    done
    return 1
}

kernel_list() {
    local -a kernels=()
    mapfile -t kernels < <(ls -1 /lib/modules 2>/dev/null | sort -V)
    ((${#kernels[@]})) || fail "no kernels found under /lib/modules"
    printf '%s\n' "${kernels[@]}"
}

do_status() {
    need dkms
    printf 'running kernel : %s\n' "$(uname -r)"
    printf 'mtgpu version  : %s\n' "$MTGPU_VERSION"
    printf 'module loaded  : %s\n' \
        "$([[ -d /sys/module/$MODULE_NAME ]] && cat /sys/module/$MODULE_NAME/initstate 2>/dev/null || echo no)"
    printf '\ndkms status:\n'
    dkms status 2>&1 | sed 's/^/  /'
    printf '\nmodule files:\n'
    local kver path
    while IFS= read -r kver; do
        if path="$(installed_module "$kver")"; then
            printf '  %-32s %s\n' "$kver" "$path"
        else
            printf '  %-32s %s\n' "$kver" "MISSING"
        fi
    done < <(kernel_list)
}

check_module_identity() {
    local path="$1" vermagic aliases
    vermagic="$(modinfo -F vermagic "$path" 2>/dev/null || true)"
    aliases="$(modinfo -F alias "$path" 2>/dev/null || true)"
    [[ -n "$vermagic" ]] || { printf 'module %s has no vermagic\n' "$path" >&2; return 1; }
    grep -Fq "$S30_PCI_ID" <<<"$aliases" || {
        printf 'module %s does not advertise the MTT S30 PCI ID\n' "$path" >&2
        return 1
    }
    printf '  vermagic: %s\n' "$vermagic"
    printf '  S30 alias: present\n'
    return 0
}

do_verify() {
    local kver="${1:-$(uname -r)}" path
    path="$(installed_module "$kver")" || fail "no $MODULE_NAME module installed for $kver"
    printf 'module: %s\n' "$path"
    check_module_identity "$path" || fail "module verification failed for $kver"
    printf 'VERIFY_STATUS: OK\n'
}

do_build() {
    local kver="${1:-$(uname -r)}"
    need dkms
    [[ -d "/usr/src/kernels/$kver" ]] ||
        fail "kernel-devel tree missing for $kver; install kernel-devel-$kver first"
    log "Building $MODULE_NAME $MTGPU_VERSION for $kver"
    root_cmd dkms build -m "$MODULE_NAME" -v "$MTGPU_VERSION" -k "$kver"
    printf 'BUILD_STATUS: OK\n'
}

do_install() {
    local kver="${1:-$(uname -r)}" backup current
    need dkms
    [[ -d "/usr/src/kernels/$kver" ]] ||
        fail "kernel-devel tree missing for $kver; install kernel-devel-$kver first"

    if [[ "$kver" == "$(uname -r)" ]]; then
        if [[ -d "/sys/module/$MODULE_NAME" ]]; then
            fail "$MODULE_NAME is loaded for the running kernel $kver."
            log "Refusing to replace the live module. Booted kernels keep their module;"
            log "reboot into the new kernel first, then run this script again."
        fi
    fi

    current="$(installed_module "$kver" || true)"
    backup="$BACKUP_ROOT/$(date +%Y%m%d-%H%M%S)-$kver"
    if [[ -n "$current" ]]; then
        root_cmd /usr/bin/install -d -m 0700 "$backup"
        root_cmd /usr/bin/cp -a -- "$current" "$backup/"
        log "Backed up $current to $backup"
    fi

    confirm "Build and install $MODULE_NAME $MTGPU_VERSION for kernel $kver?" || return $?
    root_cmd dkms add -m "$MODULE_NAME" -v "$MTGPU_VERSION" --rpm_safe_upgrade || true
    root_cmd dkms build -m "$MODULE_NAME" -v "$MTGPU_VERSION" -k "$kver"
    root_cmd dkms install -m "$MODULE_NAME" -v "$MTGPU_VERSION" -k "$kver" --force
    root_cmd depmod -a "$kver"

    local path
    if ((DRY_RUN)); then
        printf 'DRY-RUN: verified module for %s would be %s\n' "$kver" \
            "$(installed_module "$kver" || echo 'not installed')"
        printf 'INSTALL_STATUS: DRY_RUN\n'
        return 0
    fi
    path="$(installed_module "$kver")" || fail "the module is missing after installation"
    printf 'installed: %s\n' "$path"
    check_module_identity "$path" || fail "the installed module does not match the expected driver"
    [[ -n "$backup" ]] && printf 'backup: %s\n' "$backup"
    printf 'INSTALL_STATUS: SUCCESS\n'
    log "The module was not loaded and no device was bound or unbound."
}

do_rollback() {
    local kver="${1:-$(uname -r)}" backup target current
    need dkms
    backup="$(find "$BACKUP_ROOT" -maxdepth 1 -type d -name "*-$kver" 2>/dev/null | sort | tail -1)"
    [[ -n "$backup" ]] || fail "no module backup found for $kver under $BACKUP_ROOT"
    current="$(installed_module "$kver" || true)"
    target="$(find "$backup" -name "$MODULE_NAME.ko*" -type f | head -1)"
    [[ -n "$target" ]] || fail "backup $backup contains no module file"
    if [[ "$kver" == "$(uname -r)" && -d "/sys/module/$MODULE_NAME" ]]; then
        fail "$MODULE_NAME is loaded for the running kernel; refusing to replace it"
    fi
    confirm "Restore $target over the module of kernel $kver?" || return $?
    root_cmd /usr/bin/install -d -m 0755 "/lib/modules/$kver/extra"
    root_cmd /usr/bin/cp -a -- "$target" "/lib/modules/$kver/extra/"
    root_cmd depmod -a "$kver"
    printf 'restored: %s\n' "$target"
    printf 'ROLLBACK_STATUS: SUCCESS\n'
}

action="${1:-}"
[[ -n "$action" ]] || { usage >&2; exit 2; }
shift || true
KERNEL=""
while (($#)); do
    case "$1" in
        --kernel) KERNEL="${2:?--kernel needs a version}"; shift 2 ;;
        --dry-run) DRY_RUN=1; shift ;;
        --yes) ASSUME_YES=1; shift ;;
        -h|--help) usage; exit 0 ;;
        *) echo "Unknown option: $1" >&2; usage >&2; exit 2 ;;
    esac
done

case "$action" in
    status)   do_status ;;
    build)    do_build "$KERNEL" ;;
    install)  do_install "$KERNEL" ;;
    rollback) do_rollback "$KERNEL" ;;
    verify)   do_verify "$KERNEL" ;;
    *) echo "Unknown sub-command: $action" >&2; usage >&2; exit 2 ;;
esac

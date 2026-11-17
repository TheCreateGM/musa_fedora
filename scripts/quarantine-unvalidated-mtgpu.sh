#!/usr/bin/env bash
# Reversibly remove one known-failing, unowned mtgpu.ko from the running
# kernel's module tree. This does not unload or access a GPU device.
set -euo pipefail

ACTION="${1:---plan}"
PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SCRIPT_PATH="$(readlink -f -- "${BASH_SOURCE[0]}")"
MUSA_DATA_DIR="$(dirname "$PROJECT_DIR")/musa-data"
KERNEL_VERSION="7.3.3-300.fc45.x86_64"
MODULE_PATH="/lib/modules/${KERNEL_VERSION}/extra/mtgpu.ko"
INITRAMFS="/boot/initramfs-${KERNEL_VERSION}.img"
BACKUP_DIR="${MUSA_DATA_DIR}/research/system-backups/mtgpu-unvalidated-${KERNEL_VERSION}-2026-11-12"
BACKUP_PATH="${BACKUP_DIR}/mtgpu.ko"
EXPECTED_SHA256="b59c446472d7f1b7946efb4d559054b2c2124fa3e4c2a5fa1eab3355ec2f8ddb"
S30_DEVICE="/sys/bus/pci/devices/0000:01:00.0"

usage() {
    cat <<EOF
Usage: $0 [--plan|--quarantine|--restore]

  --plan        Read-only summary (default)
  --quarantine  Back up and remove only the verified mtgpu.ko file from the
                7.3.3 module tree, then regenerate that tree's depmod indexes.
  --restore     Restore the verified backup and regenerate depmod indexes.

The currently loaded module is not unloaded. The S30 is not opened or tested.
EOF
}

sha256_of() {
    sha256sum -- "$1" | awk '{print $1}'
}

if [[ "$ACTION" == "--help" || "$ACTION" == "-h" ]]; then
    usage
    exit 0
fi
if [[ "$ACTION" != "--plan" && "$ACTION" != "--quarantine" && "$ACTION" != "--restore" ]]; then
    usage >&2
    exit 2
fi

echo "Kernel expected: $KERNEL_VERSION"
echo "Module path:     $MODULE_PATH"
echo "Backup path:     $BACKUP_PATH"
echo "Expected SHA256: $EXPECTED_SHA256"
if [[ -r "$S30_DEVICE/vendor" && -r "$S30_DEVICE/device" ]]; then
    printf 'S30 PCI state:   %s:%s\n' \
        "$(cat "$S30_DEVICE/vendor")" "$(cat "$S30_DEVICE/device")"
    if [[ -L "$S30_DEVICE/driver" ]]; then
        printf 'S30 driver:      %s\n' "$(readlink -f "$S30_DEVICE/driver")"
    else
        echo "S30 driver:      unbound"
    fi
fi
if [[ -r /sys/module/mtgpu/initstate ]]; then
    printf 'mtgpu state:     %s\n' "$(cat /sys/module/mtgpu/initstate)"
else
    echo "mtgpu state:     not loaded"
fi
if [[ -f "$MODULE_PATH" ]]; then
    printf 'module SHA256:   %s\n' "$(sha256_of "$MODULE_PATH")"
else
    echo "module file:     absent"
fi

if [[ "$ACTION" == "--plan" ]]; then
    echo "PLAN_STATUS: READ_ONLY"
    echo "No file, module, initramfs, boot setting, or device state was changed."
    exit 0
fi

if [[ "$(id -u)" != 0 ]]; then
    echo "ERROR: this action requires authorization through pkexec." >&2
    printf 'Run: pkexec /usr/bin/bash %q %q\n' "$SCRIPT_PATH" "$ACTION" >&2
    exit 77
fi
if [[ "$(uname -r)" != "$KERNEL_VERSION" ]]; then
    echo "ERROR: running kernel changed; refusing to edit a different kernel tree." >&2
    exit 1
fi
if [[ ! -d "/lib/modules/${KERNEL_VERSION}/extra" ]]; then
    echo "ERROR: expected module directory is missing." >&2
    exit 1
fi

if [[ "$ACTION" == "--quarantine" ]]; then
    if [[ ! -f "$MODULE_PATH" || -L "$MODULE_PATH" ]]; then
        echo "ERROR: expected regular module file is absent or is a symlink." >&2
        exit 1
    fi
    actual_sha="$(sha256_of "$MODULE_PATH")"
    if [[ "$actual_sha" != "$EXPECTED_SHA256" ]]; then
        echo "ERROR: module hash changed; expected $EXPECTED_SHA256, found $actual_sha." >&2
        exit 1
    fi
    if rpm -qf "$MODULE_PATH" >/dev/null 2>&1; then
        echo "ERROR: RPM owns this module; refusing to move a packaged file." >&2
        exit 1
    fi
    if [[ ! -r "$S30_DEVICE/vendor" || ! -r "$S30_DEVICE/device" ||
          "$(cat "$S30_DEVICE/vendor")" != "0x1ed5" ||
          "$(cat "$S30_DEVICE/device")" != "0x0102" ]]; then
        echo "ERROR: expected MTT S30 PCI function is not present." >&2
        exit 1
    fi
    if [[ -L "$S30_DEVICE/driver" ]]; then
        echo "ERROR: S30 is bound; refusing to alter the module tree." >&2
        exit 1
    fi
    if [[ -r /sys/module/mtgpu/initstate ]]; then
        module_state="$(cat /sys/module/mtgpu/initstate)"
        if [[ "$module_state" != "coming" ]]; then
            echo "ERROR: mtgpu state is '$module_state', expected the recorded failed-init state 'coming'." >&2
            exit 1
        fi
    fi
    if [[ ! -r "$INITRAMFS" ]] || ! command -v lsinitrd >/dev/null 2>&1; then
        echo "ERROR: cannot verify the current initramfs contents." >&2
        exit 1
    fi
    if lsinitrd "$INITRAMFS" 2>/dev/null | grep -Eq '(^|/)mtgpu\.ko([.[:space:]]|$)'; then
        echo "ERROR: mtgpu.ko is present in the initramfs; refusing this module-tree-only action." >&2
        exit 1
    fi

    install -d -m 0755 "$BACKUP_DIR"
    if [[ -e "$BACKUP_PATH" ]]; then
        if [[ "$(sha256_of "$BACKUP_PATH")" != "$EXPECTED_SHA256" ]]; then
            echo "ERROR: backup path exists with a different hash." >&2
            exit 1
        fi
    else
        cp --preserve=mode,timestamps -- "$MODULE_PATH" "$BACKUP_PATH"
    fi
    if [[ "$(sha256_of "$BACKUP_PATH")" != "$EXPECTED_SHA256" ]]; then
        echo "ERROR: backup verification failed; original module file was kept." >&2
        exit 1
    fi

    rm -- "$MODULE_PATH"
    if ! depmod -a "$KERNEL_VERSION"; then
        cp --preserve=mode,timestamps -- "$BACKUP_PATH" "$MODULE_PATH"
        restorecon -F "$MODULE_PATH" 2>/dev/null || true
        depmod -a "$KERNEL_VERSION" || true
        echo "ERROR: depmod failed; restored the original module file from the verified backup." >&2
        exit 1
    fi

    echo "QUARANTINE_STATUS: COMPLETE"
    echo "BACKUP_SHA256: $(sha256_of "$BACKUP_PATH")"
    echo "The loaded module remains resident in this boot; it was not unloaded."
    echo "No initramfs, boot entry, configuration file, or GPU device was changed."
    echo "A reboot is required to clear the failed module initialization from memory."
    exit 0
fi

if [[ ! -f "$BACKUP_PATH" || "$(sha256_of "$BACKUP_PATH")" != "$EXPECTED_SHA256" ]]; then
    echo "ERROR: verified backup is missing or has the wrong hash." >&2
    exit 1
fi
if [[ -e "$MODULE_PATH" ]]; then
    if [[ "$(sha256_of "$MODULE_PATH")" == "$EXPECTED_SHA256" ]]; then
        echo "RESTORE_STATUS: module file is already present with the expected hash."
        depmod -a "$KERNEL_VERSION"
        exit 0
    fi
    echo "ERROR: a different file already exists at the target path." >&2
    exit 1
fi

install -d -m 0755 "$(dirname "$MODULE_PATH")"
cp --preserve=mode,timestamps -- "$BACKUP_PATH" "$MODULE_PATH"
restorecon -F "$MODULE_PATH"
if [[ "$(sha256_of "$MODULE_PATH")" != "$EXPECTED_SHA256" ]]; then
    rm -f -- "$MODULE_PATH"
    echo "ERROR: restored module hash verification failed." >&2
    exit 1
fi
depmod -a "$KERNEL_VERSION"
echo "RESTORE_STATUS: COMPLETE"
echo "MODULE_SHA256: $(sha256_of "$MODULE_PATH")"
echo "The module was restored to disk only; it was not loaded."
echo "PCI autoload may load it on a future boot of this kernel; the module is not load-approved."

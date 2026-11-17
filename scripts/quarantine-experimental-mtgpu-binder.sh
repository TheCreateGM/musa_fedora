#!/usr/bin/env bash
# Reversibly quarantine the unvalidated project-authored PCI binder from an
# inactive kernel tree and remove its stale global PCI alias. Never loads or
# unloads a module and never changes an initramfs or boot entry.
set -euo pipefail

ACTION="${1:---plan}"
PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SCRIPT_PATH="$(readlink -f -- "${BASH_SOURCE[0]}")"
MUSA_DATA_DIR="$(dirname "$PROJECT_DIR")/musa-data"
TARGET_KERNEL="7.2.15-300.fc45.x86_64"
MODULE_PATH="/lib/modules/${TARGET_KERNEL}/extra/mtgpu_bind.ko"
MODPROBE_FILE="/etc/modprobe.d/musa.conf"
INITRAMFS="/boot/initramfs-${TARGET_KERNEL}.img"
BACKUP_DIR="${MUSA_DATA_DIR}/research/system-backups/experimental-mtgpu-binder-${TARGET_KERNEL}-2026-11-12"
BACKUP_PARENT="${MUSA_DATA_DIR}/research/system-backups"
BACKUP_MODULE="${BACKUP_DIR}/mtgpu_bind.ko"
BACKUP_CONFIG="${BACKUP_DIR}/musa.conf"
EXPECTED_MODULE_SHA256="65415c54b87d5d1f2ab4bb3f9315fa71c3f3e7709fbb5bc98fa216527df5fa85"
EXPECTED_CONFIG_SHA256="abead3eeb640844a67e87cabdfdf67024074f82de82d7f2302192b22c6203f75"
S30_DEVICE="/sys/bus/pci/devices/0000:01:00.0"

usage() {
    cat <<EOF
Usage: $0 [--plan|--quarantine|--restore]

  --plan        Read-only status (default)
  --quarantine  Back up and remove the exact unvalidated mtgpu_bind.ko from
                ${TARGET_KERNEL}, remove its stale modprobe alias, and run depmod.
  --restore     Restore both files from their hash-verified backups. This
                reactivates the global alias and may auto-load the binder
                when the target kernel is booted.

The script refuses to edit the running kernel, an RPM-owned file, a changed
file, a loaded/bound binder, or an initramfs containing an MTGPU module. It
does not touch sgpu_km, another kernel tree, boot entries, initramfs files,
the resident mtgpu module, or the GPU device.
EOF
}

sha256_of() { sha256sum -- "$1" | awk '{print $1}'; }
is_regular_non_symlink() { [[ -f "$1" && ! -L "$1" ]]; }

if [[ "$ACTION" == "--help" || "$ACTION" == "-h" ]]; then
    usage
    exit 0
fi
if [[ "$ACTION" != "--plan" && "$ACTION" != "--quarantine" && "$ACTION" != "--restore" ]]; then
    usage >&2
    exit 2
fi

echo "Running kernel:  $(uname -r)"
echo "Target kernel:   ${TARGET_KERNEL}"
echo "Module path:     ${MODULE_PATH}"
echo "Modprobe file:   ${MODPROBE_FILE}"
echo "Backup path:     ${BACKUP_DIR}"
if [[ -r /sys/module/mtgpu_bind/initstate ]]; then
    printf 'Binder state:    %s\n' "$(cat /sys/module/mtgpu_bind/initstate)"
else
    echo "Binder state:    not loaded"
fi
if [[ -r "$S30_DEVICE/vendor" && -r "$S30_DEVICE/device" ]]; then
    printf 'S30 PCI state:   %s:%s\n' "$(cat "$S30_DEVICE/vendor")" "$(cat "$S30_DEVICE/device")"
    if [[ -L "$S30_DEVICE/driver" ]]; then
        printf 'S30 driver:      %s\n' "$(readlink -f "$S30_DEVICE/driver")"
    else
        echo "S30 driver:      unbound"
    fi
fi
if is_regular_non_symlink "$MODULE_PATH"; then
    printf 'Module SHA256:   %s\n' "$(sha256_of "$MODULE_PATH")"
else
    echo "Module file:     absent or not a regular file"
fi
if is_regular_non_symlink "$MODPROBE_FILE"; then
    printf 'Config SHA256:   %s\n' "$(sha256_of "$MODPROBE_FILE")"
else
    echo "Modprobe file:   absent or not a regular file"
fi

if [[ "$ACTION" == "--plan" ]]; then
    echo "PLAN_STATUS: READ_ONLY"
    echo "No module, config, initramfs, boot entry, or device state was changed."
    exit 0
fi

if [[ "$(id -u)" != 0 ]]; then
    echo "ERROR: this action requires authorization through pkexec." >&2
    printf 'Run: pkexec /usr/bin/bash %q %q\n' "$SCRIPT_PATH" "$ACTION" >&2
    exit 77
fi
if [[ "$(uname -r)" == "$TARGET_KERNEL" ]]; then
    echo "ERROR: refusing to edit the running kernel's module tree." >&2
    exit 1
fi
if [[ -r /sys/module/mtgpu_bind/initstate ]]; then
    echo "ERROR: mtgpu_bind is loaded; refusing to alter its module tree." >&2
    exit 1
fi
if [[ -L "$S30_DEVICE/driver" ]]; then
    echo "ERROR: the S30 is bound; refusing to alter module/config state." >&2
    exit 1
fi
if [[ ! -r "$S30_DEVICE/vendor" || ! -r "$S30_DEVICE/device" ||
      "$(cat "$S30_DEVICE/vendor")" != "0x1ed5" ||
      "$(cat "$S30_DEVICE/device")" != "0x0102" ]]; then
    echo "ERROR: expected S30 PCI function 0000:01:00.0 was not verified." >&2
    exit 1
fi
if [[ ! -r "$INITRAMFS" ]] || ! command -v lsinitrd >/dev/null 2>&1; then
    echo "ERROR: cannot verify the target kernel's initramfs contents." >&2
    exit 1
fi
if ! lsinitrd "$INITRAMFS" >/dev/null 2>&1; then
    echo "ERROR: lsinitrd failed; refusing to edit module/config state." >&2
    exit 1
fi
if lsinitrd "$INITRAMFS" 2>/dev/null | grep -Eq '(^|/)(mtgpu|mtgpu_bind|mtgpu_stub|sgpu_km)(\.ko|/|$)'; then
    echo "ERROR: the target initramfs contains an MTGPU module; manual review is required." >&2
    exit 1
fi
if [[ "$ACTION" == "--quarantine" ]]; then
    if ! is_regular_non_symlink "$MODULE_PATH" ||
       [[ "$(sha256_of "$MODULE_PATH")" != "$EXPECTED_MODULE_SHA256" ]]; then
        echo "ERROR: module file is absent, changed, or not a regular file." >&2
        exit 1
    fi
    if ! is_regular_non_symlink "$MODPROBE_FILE" ||
       [[ "$(sha256_of "$MODPROBE_FILE")" != "$EXPECTED_CONFIG_SHA256" ]]; then
        echo "ERROR: modprobe configuration is absent, changed, or not a regular file." >&2
        exit 1
    fi
    if rpm -qf "$MODULE_PATH" >/dev/null 2>&1 || rpm -qf "$MODPROBE_FILE" >/dev/null 2>&1; then
        echo "ERROR: an RPM owns a target; refusing to move packaged files." >&2
        exit 1
    fi
    for dir in "$MUSA_DATA_DIR" "${MUSA_DATA_DIR}/research" "$BACKUP_PARENT"; do
        if [[ ! -d "$dir" || -L "$dir" ]]; then
            echo "ERROR: expected real research directory is missing or a symlink: $dir" >&2
            exit 1
        fi
    done
    if [[ "$(stat -c '%u' "$BACKUP_PARENT")" != 0 ]]; then
        echo "ERROR: backup parent is not root-owned; refusing privileged backup." >&2
        exit 1
    fi
    if [[ -L "$BACKUP_DIR" || ( -e "$BACKUP_DIR" && ! -d "$BACKUP_DIR" ) ]]; then
        echo "ERROR: backup path exists as a symlink or non-directory." >&2
        exit 1
    fi
    install -d -m 0755 "$BACKUP_DIR"
    for pair in "$MODULE_PATH:$BACKUP_MODULE" "$MODPROBE_FILE:$BACKUP_CONFIG"; do
        src="${pair%%:*}"
        dst="${pair#*:}"
        expected="$EXPECTED_MODULE_SHA256"
        [[ "$src" == "$MODPROBE_FILE" ]] && expected="$EXPECTED_CONFIG_SHA256"
        if [[ -e "$dst" ]]; then
            if ! is_regular_non_symlink "$dst" || [[ "$(sha256_of "$dst")" != "$expected" ]]; then
                echo "ERROR: backup exists with unexpected contents: $dst" >&2
                exit 1
            fi
        else
            cp -a -- "$src" "$dst"
        fi
        if [[ "$(sha256_of "$dst")" != "$expected" ]]; then
            echo "ERROR: backup verification failed; originals were kept." >&2
            exit 1
        fi
    done

    if ! rm -- "$MODULE_PATH" "$MODPROBE_FILE"; then
        cp -a -- "$BACKUP_MODULE" "$MODULE_PATH"
        cp -a -- "$BACKUP_CONFIG" "$MODPROBE_FILE"
        restorecon -F "$MODULE_PATH" "$MODPROBE_FILE" 2>/dev/null || true
        echo "ERROR: quarantine failed; original files were restored." >&2
        exit 1
    fi
    if ! depmod -a "$TARGET_KERNEL"; then
        cp -a -- "$BACKUP_MODULE" "$MODULE_PATH"
        cp -a -- "$BACKUP_CONFIG" "$MODPROBE_FILE"
        restorecon -F "$MODULE_PATH" "$MODPROBE_FILE" 2>/dev/null || true
        depmod -a "$TARGET_KERNEL" || true
        echo "ERROR: depmod failed; original files were restored." >&2
        exit 1
    fi
    echo "QUARANTINE_STATUS: COMPLETE"
    echo "MODULE_BACKUP_SHA256: $(sha256_of "$BACKUP_MODULE")"
    echo "CONFIG_BACKUP_SHA256: $(sha256_of "$BACKUP_CONFIG")"
    echo "No module was loaded/unloaded and no initramfs or boot entry was changed."
    exit 0
fi

for dir in "$MUSA_DATA_DIR" "${MUSA_DATA_DIR}/research" "$BACKUP_PARENT" "$BACKUP_DIR"; do
    if [[ ! -d "$dir" || -L "$dir" ]]; then
        echo "ERROR: expected real backup directory is missing or a symlink: $dir" >&2
        exit 1
    fi
done
if [[ "$(stat -c '%u' "$BACKUP_PARENT")" != 0 ||
      "$(stat -c '%u' "$BACKUP_DIR")" != 0 ]]; then
    echo "ERROR: backup paths are not root-owned; refusing privileged restore." >&2
    exit 1
fi
if ! is_regular_non_symlink "$BACKUP_MODULE" ||
   [[ "$(sha256_of "$BACKUP_MODULE")" != "$EXPECTED_MODULE_SHA256" ]] ||
   ! is_regular_non_symlink "$BACKUP_CONFIG" ||
   [[ "$(sha256_of "$BACKUP_CONFIG")" != "$EXPECTED_CONFIG_SHA256" ]]; then
    echo "ERROR: one or both verified backups are absent or damaged." >&2
    exit 1
fi
if [[ -e "$MODULE_PATH" && -e "$MODPROBE_FILE" ]]; then
    if is_regular_non_symlink "$MODULE_PATH" && is_regular_non_symlink "$MODPROBE_FILE" &&
       [[ "$(sha256_of "$MODULE_PATH")" == "$EXPECTED_MODULE_SHA256" &&
          "$(sha256_of "$MODPROBE_FILE")" == "$EXPECTED_CONFIG_SHA256" ]]; then
        echo "RESTORE_STATUS: FILES_ALREADY_PRESENT"
        exit 0
    fi
    echo "ERROR: one or both targets already exist with unexpected contents." >&2
    exit 1
fi
if [[ -e "$MODULE_PATH" || -e "$MODPROBE_FILE" ]]; then
    echo "ERROR: only one target is present; refusing a partial restore." >&2
    exit 1
fi
install -d -m 0755 "$(dirname "$MODULE_PATH")"
if ! cp -a -- "$BACKUP_MODULE" "$MODULE_PATH"; then
    echo "ERROR: failed to restore the module file." >&2
    exit 1
fi
if ! cp -a -- "$BACKUP_CONFIG" "$MODPROBE_FILE"; then
    rm -f -- "$MODULE_PATH"
    echo "ERROR: config restore failed; removed the partially restored module." >&2
    exit 1
fi
if ! restorecon -F "$MODULE_PATH" "$MODPROBE_FILE"; then
    rm -f -- "$MODULE_PATH" "$MODPROBE_FILE"
    echo "ERROR: SELinux relabel failed; returned to the quarantined state." >&2
    exit 1
fi
if [[ "$(sha256_of "$MODULE_PATH")" != "$EXPECTED_MODULE_SHA256" ||
      "$(sha256_of "$MODPROBE_FILE")" != "$EXPECTED_CONFIG_SHA256" ]]; then
    rm -f -- "$MODULE_PATH" "$MODPROBE_FILE"
    echo "ERROR: restored files failed hash verification; returned to the quarantined state." >&2
    exit 1
fi
if ! depmod -a "$TARGET_KERNEL"; then
    rm -f -- "$MODULE_PATH" "$MODPROBE_FILE"
    depmod -a "$TARGET_KERNEL" || true
    echo "ERROR: depmod failed; returned to the quarantined state." >&2
    exit 1
fi
echo "RESTORE_STATUS: COMPLETE"
echo "Files were restored to disk only; no module was loaded."
echo "The restored alias may auto-load the binder on a future target-kernel boot."

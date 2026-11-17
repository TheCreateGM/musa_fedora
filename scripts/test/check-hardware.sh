#!/usr/bin/env bash
# Read-only PCI/driver inventory. This does not initialize the MUSA runtime or
# claim that a bound PCI driver can execute GPU work.
set -euo pipefail

PCI_DEVICES=/sys/bus/pci/devices
found=0
unqualified=0
driver_not_live=0

echo "=== MTT S30 read-only readiness check ==="
echo "Kernel: $(uname -r)"

for dev in "$PCI_DEVICES"/*; do
    [[ -r "$dev/vendor" && -r "$dev/device" ]] || continue
    [[ "$(cat "$dev/vendor")" == 0x1ed5 && "$(cat "$dev/device")" == 0x0102 ]] || continue

    found=1
    bdf="${dev##*/}"
    echo "PCI_STATUS: MTT S30 visible at $bdf (1ed5:0102)"

    if [[ -L "$dev/driver" ]]; then
        driver_path="$(readlink -f "$dev/driver")"
        driver="${driver_path##*/}"
        echo "DRIVER_STATUS: bound to $driver"
        module_path="$(readlink -f "$driver_path/module" 2>/dev/null || true)"
        if [[ -n "$module_path" ]]; then
            module="${module_path##*/}"
            echo "DRIVER_MODULE: $module"
            module_file="$(modinfo -F filename "$module" 2>/dev/null || true)"
            [[ -z "$module_file" ]] || echo "DRIVER_MODULE_FILE: $module_file"
        else
            echo "DRIVER_MODULE: not exposed through sysfs"
        fi
    else
        echo "DRIVER_STATUS: unbound"
    fi

    echo "DEVICE_NODES:"
    node_found=0
    for node in /dev/mtgpu* /dev/sgpu*; do
        [[ -e "$node" ]] || continue
        ls -l -- "$node"
        node_found=1
    done
    [[ "$node_found" == 1 ]] || echo "  no MTT-specific device nodes"
    echo "DRIVER_QUALIFICATION: not established by this check"
    echo "GPU_TEST_STATUS: NOT RUN (PCI enumeration/binding is not GPU execution)"
    unqualified=1
done

for module in mtgpu mtgpu_bind mtgpu_stub sgpu_km sgpu_xx; do
    initstate_path="/sys/module/$module/initstate"
    [[ -r "$initstate_path" ]] || continue
    read -r initstate <"$initstate_path"
    echo "MODULE_INITSTATE: $module=$initstate"
    if [[ "$initstate" != "live" ]]; then
        echo "GPU_ACCESS_STATUS: BLOCKED ($module initialization is incomplete or unloading)"
        driver_not_live=1
    fi
done

if [[ "$found" == 0 ]]; then
    echo "PCI_STATUS: MTT S30 not found"
    echo "GPU_TEST_STATUS: BLOCKED (no matching PCI function)"
    exit 2
fi

if [[ "$unqualified" == 1 ]]; then
    if [[ "$driver_not_live" == 1 ]]; then
        echo "RESULT: device visible; driver initialization is incomplete, so device tests must not be attempted"
    else
        echo "RESULT: device visible; a vendor-qualified driver/runtime and real GPU test remain unverified"
    fi
    exit 3
fi

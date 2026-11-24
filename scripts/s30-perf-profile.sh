#!/usr/bin/env bash
# s30-perf-profile.sh — inspect the MTT S30 driver and host tuning state.
#
# READ-ONLY by default: it never writes a file, never changes a module
# parameter, and never reloads a module. Its purpose is to show the tunables
# that actually exist, whether they are at a value that is safe and fast for
# compute use, and what the platform bottleneck is.
#
# Usage:
#   scripts/s30-perf-profile.sh                     inspect + report
#   scripts/s30-perf-profile.sh --print-dropin      print (do not install) a
#                                                   modprobe.d drop-in proposal
#
# Exiting non-zero means a *problem* was found, not that something was fixed.
set -uo pipefail

MTGPU_SYS="/sys/module/mtgpu/parameters"
PRINT_DROPIN=0
[[ "${1:-}" == "--print-dropin" ]] && PRINT_DROPIN=1
[[ "${1:-}" == "-h" || "${1:-}" == "--help" ]] && { sed -n '2,14p' "$0"; exit 0; }

problems=0
notes=0
ok()   { printf '  \033[32mOK  \033[0m %s\n' "$*"; }
warn() { printf '  \033[33mWARN\033[0m %s\n' "$*"; notes=$((notes+1)); }
bad()  { printf '  \033[31mPROB\033[0m %s\n' "$*"; problems=$((problems+1)); }
info() { printf '        %s\n' "$*"; }

# Read a single module parameter, printing "" when the module is not loaded.
mp() {
    [[ -r "$MTGPU_SYS/$1" ]] && tr -d '\n' <"$MTGPU_SYS/$1" || printf ''
}

echo "=== MTT S30 performance profile (read-only) ==="
echo "kernel: $(uname -r)"
echo

# ---------------------------------------------------------------- device ----
echo "-- device / PCIe --"
if [[ ! -e /sys/bus/pci/devices/0000:01:00.0 ]]; then
    bad "S30 (0000:01:00.0) not present on PCI"
else
    speed="$(cat /sys/bus/pci/devices/0000:01:00.0/current_link_speed 2>/dev/null)"
    width="$(cat /sys/bus/pci/devices/0000:01:00.0/current_link_width 2>/dev/null)"
    maxs="$(cat /sys/bus/pci/devices/0000:01:00.0/max_link_speed 2>/dev/null)"
    maxw="$(cat /sys/bus/pci/devices/0000:01:00.0/max_link_width 2>/dev/null)"
    drv="$(basename "$(readlink -f /sys/bus/pci/devices/0000:01:00.0/driver 2>/dev/null)" 2>/dev/null)"
    info "driver bound        : ${drv:-<none>}"
    info "current link        : ${speed} x${width}"
    info "maximum link        : ${maxs} x${maxw}"

    # PCIe 3.0 x8 => ~7.87 GB/s per direction. This, not the GPU's internal
    # bandwidth, is what sizes every host<->device transfer.
    case "$speed" in
        *8.0*)   ceil=7.9 ;;
        *16.0*)  ceil=15.75 ;;
        *2.5*)   ceil=1.97 ;;
        *)       ceil=0 ;;
    esac
    if [[ "$width" == "8" && "$ceil" == "7.9" ]]; then
        warn "S30 is running at PCIe 3.0 x8 (~7.9 GB/s/direction ceiling)"
        info "the device's internal peak is ~272 GB/s, so host<->device"
        info "traffic is the dominant bottleneck by a wide margin"
    elif [[ -n "$speed" ]]; then
        ok "link ${speed} x${width}"
    fi

    if [[ "${drv:-}" != "mtgpu" ]]; then
        bad "S30 is not bound to mtgpu (bound to '${drv:-<none>}')"
    else
        ok "S30 bound to mtgpu"
    fi
fi
echo

# ------------------------------------------------------------------ driver --
echo "-- mtgpu compute-critical parameters --"
if [[ ! -d $MTGPU_SYS ]]; then
    warn "mtgpu module not loaded; driver parameters unavailable"
else
    # rm_feature_level: documented footgun on this driver. 0 is the verified
    # PVR memory model; 1 activated an incomplete Resource Manager path and
    # caused a NULL pointer dereference in gpu_phys_heap_alloc_mem.
    rfl="$(mp rm_feature_level)"
    if [[ "$rfl" == "0" ]]; then
        ok "rm_feature_level=0 (verified PVR memory model)"
    else
        bad "rm_feature_level='${rfl}' - 1 is known to oops in gpu_phys_heap_alloc_mem; see docs/root-cause-rm-feature-level-2026-11-20.md"
    fi

    # Headless compute: the desktop stays on i915, so mtgpu must not grab KMS.
    disp="$(mp display)"
    if [[ "$disp" == "none" || "$disp" == "" ]]; then
        ok "display=none (headless compute; desktop stays on i915)"
    else
        warn "display='${disp}' - mtgpu is asked to drive outputs; see docs/compute-mode.md"
    fi

    fbdev="$(mp disable_fbdev)"
    if [[ "$fbdev" == "Y" || "$fbdev" == "1" ]]; then
        ok "disable_fbdev=1 (no fbdev console contention on the S30)"
    else
        warn "disable_fbdev is not set; fbdev can contend with compute"
    fi

    # Scheduling knobs.
    info "mtgpu_sched_mode            = $(mp mtgpu_sched_mode)"
    info "mtgpu_sched_host_policy     = $(mp mtgpu_sched_host_policy)"
    info "mtgpu_sched_early_submit    = $(mp mtgpu_sched_early_submit)"
    info "mtgpu_sched_ccbq_optimize   = $(mp mtgpu_sched_ccbq_optimize)"
    info "enable_async_job_submit     = $(mp enable_async_job_submit)"

    # Memory / heap.
    info "mtgpu_page_size             = $(mp mtgpu_page_size)"
    info "mtgpu_pb_total_size         = $(mp mtgpu_pb_total_size)  (parameter buffer)"
    info "mtgpu_reserved_mem_size     = $(mp mtgpu_reserved_mem_size)"
    info "vram_lower_limit            = $(mp vram_lower_limit)"
    info "mtgpu_vram_size_total       = $(mp mtgpu_vram_size_total)"
    info "GeneralNon4KHeapPageSize    = $(mp GeneralNon4KHeapPageSize)"

    vram="$(mp mtgpu_vram_size_total)"
    if [[ "$vram" =~ ^[0-9]+$ && "$vram" -gt $((5 * 1024 * 1024 * 1024)) ]]; then
        warn "mtgpu_vram_size_total=${vram} exceeds the S30's 4 GiB; it is a module default, not the real device size"
        info "the runtime reports the true size: 4267532288 bytes (4.0 GiB)"
    fi

    # Power / clocking - these decide whether the GPU downclocks under load.
    info "enable_gpu_dvfs             = $(mp enable_gpu_dvfs)"
    info "mtgpu_pstate_mode           = $(mp mtgpu_pstate_mode)"
    info "enable_rpm                  = $(mp enable_rpm)"
    info "DisableClockGating          = $(mp DisableClockGating)"
    if [[ "$(mp enable_gpu_dvfs)" == "N" ]]; then
        warn "DVFS disabled - the GPU will not clock up under load"
    else
        ok "DVFS enabled (enable_gpu_dvfs=$(mp enable_gpu_dvfs))"
    fi
fi
echo

# -------------------------------------------------------------- scheduling --
echo "-- interrupt placement --"
if command -v systemctl >/dev/null 2>&1; then
    if systemctl is-active --quiet irqbalance 2>/dev/null; then
        warn "irqbalance is active: it migrates mtgpu MSI vectors every 10-60 s"
        info "this only produces 'irq:... affinity has changed' log noise"
        info "see docs/root-cause-irq-affinity-2026-11-23.md - do NOT ban the device"
        info "routinely without a reviewed plan; interrupts load-balance as designed"
    else
        ok "irqbalance not active (no MSI migration churn)"
    fi
fi
echo

# ------------------------------------------------------------- host memory --
echo "-- host memory headroom (driver allocates page-backed BOs) --"
avail_kb="$(awk '/MemAvailable/{print $2}' /proc/meminfo)"
if [[ "$avail_kb" -lt $((1024 * 1024)) ]]; then
    warn "MemAvailable = $((avail_kb / 1024)) MiB; large musaMalloc/host-register"
    info "requests can hit order:6 warn_alloc under this pressure"
    info "see docs/root-cause-order6-warn-alloc-2026-11-24.md"
else
    ok "MemAvailable = $((avail_kb / 1024)) MiB"
fi
if [[ -f /sys/fs/cgroup/user.slice/memory.min ]]; then
    cmin="$(cat /sys/fs/cgroup/user.slice/memory.min 2>/dev/null || echo 0)"
    [[ "$cmin" -gt 0 ]] && info "user.slice memory.min = $((cmin / 1024 / 1024)) MiB reserved"
fi
echo

# ---------------------------------------------------------------- runtime --
echo "-- MUSA runtime environment --"
if [[ -r /etc/profile.d/zz-musa-fedora.sh ]]; then
    ok "/etc/profile.d/zz-musa-fedora.sh present (1.5.4 runtime wins ordering)"
else
    warn "missing /etc/profile.d/zz-musa-fedora.sh; the 5.2.0 prefix may shadow the 1.5.4 runtime"
fi
if [[ -x /usr/local/musa/bin/musaInfo ]]; then
    if musaInfo_out="$(LD_LIBRARY_PATH=/usr/local/musa/lib /usr/local/musa/bin/musaInfo 2>&1)"; then
        ok "musaInfo runs"
        printf '%s\n' "$musaInfo_out" | head -6 | sed 's/^/        /'
    else
        warn "musaInfo failed: $(printf '%s' "$musaInfo_out" | head -2)"
    fi
fi
echo

# ----------------------------------------------------------------- report --
if [[ $PRINT_DROPIN -eq 1 ]]; then
    cat <<'DROPIN'
# --- proposed modprobe.d drop-in (PRINTED ONLY; NOT installed) --------------
# Save as /etc/modprobe.d/91-mtgpu-compute.conf only after reviewing it.
# These are at-default, compute-oriented values: they change nothing that is
# not already the shipped default except disable_fbdev, which must be 1 for
# headless compute. rm_feature_level is stated explicitly because 1 oopses.
options mtgpu display=none disable_fbdev=1 mtgpu_drm_major=2 rm_feature_level=0
# -----------------------------------------------------------------------------
DROPIN
    echo
fi

echo "=== profile summary ==="
echo "  problems : $problems"
echo "  warnings : $notes"
if [[ $problems -gt 0 ]]; then
    echo "  RESULT   : ATTENTION REQUIRED"
    exit 1
fi
echo "  RESULT   : OK"
exit 0

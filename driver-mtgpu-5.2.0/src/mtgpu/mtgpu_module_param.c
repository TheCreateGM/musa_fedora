/*
 * @Copyright   Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License     Dual MIT/GPLv2
 */

/*
 * This is a device driver for the mtgpu framework. It creates platform
 * devices inside the MT GPU, and exports functions to manage the
 * shared interrupt handling
 */

#include <linux/moduleparam.h>
#include <linux/errno.h>

#include "mtgpu_defs.h"
#include "mtgpu_igpu.h"
#include "mtgpu_mdev.h"
#include "mtgpu_intr_poll.h"
#include "mtgpu_devfreq.h"

int mtgpu_cnt = 0;
module_param(mtgpu_cnt, int, 0444);
MODULE_PARM_DESC(mtgpu_cnt, "mtgpu card counter.");

/* TODO: Currently assume the MTGPU default video ram size is 1G.
 * It should get the video ram size from SMC in the future for each
 * mtgpu graphics card
 */
unsigned long mtgpu_vram_size_total = MTGPU_VRAM_DEFAULT_SIZE;
module_param(mtgpu_vram_size_total, ulong, 0444);
MODULE_PARM_DESC(mtgpu_vram_size_total,
		 "mtgpu video memory total size, default to 1GB ");

unsigned long mtgpu_pb_total_size = 0;
module_param(mtgpu_pb_total_size, ulong, 0444);
MODULE_PARM_DESC(mtgpu_pb_total_size,
		 "mtgpu pb size, auto set based on gpu id, manual setting override auto config ");

unsigned long mtgpu_smc_mem_size = MTGPU_SMC_MEM_SIZE;
module_param(mtgpu_smc_mem_size, ulong, 0444);
MODULE_PARM_DESC(mtgpu_smc_mem_size,
		 "mtgpu smc memory total size, default to 2M ");

unsigned long mtgpu_reserved_mem_size = MTGPU_RESERVED_MEM_SIZE;
module_param(mtgpu_reserved_mem_size, ulong, 0444);
MODULE_PARM_DESC(mtgpu_reserved_mem_size,
		 "mtgpu reserved memory size for smc/dma/fw/mmu etc, to meet system requirements, default to 256MB ");

/*
 * mtgpu_resize_enable is used for the option to control the resize bar
 * functionality is enabled or disabled.
 */
int mtgpu_resize_enable = 1;
module_param(mtgpu_resize_enable, int, 0444);
MODULE_PARM_DESC(mtgpu_resize_enable,
		 "mtgpu resize function enable(1)/disable(0) ");

char *display = MTGPU_DISPLAY_MODE;
module_param(display, charp, 0444);
MODULE_PARM_DESC(display, " <dummy>, <mt>, <none>; VPS default none, otherwise mt");

bool display_bringup_en;
module_param(display_bringup_en, bool, 0444);
MODULE_PARM_DESC(display_bringup_en, "mtgpu display bringup, enable(1)/disable(0).");

unsigned long mtgpu_cursor_size = MTGPU_MAX_CURSOR_SIZE;
module_param(mtgpu_cursor_size, ulong, 0444);
MODULE_PARM_DESC(mtgpu_cursor_size, "max cursor size in pixel");

/* Enable_sriov is used for the sriov capability. */
int enable_sriov = MTGPU_ENABLE_SRIOV;
module_param(enable_sriov, int, 0444);
MODULE_PARM_DESC(enable_sriov, "enable sriov, default to 1 is enable ");

bool force_cpu_copy;
module_param(force_cpu_copy, bool, 0644);
MODULE_PARM_DESC(force_cpu_copy,
		 "dma transfer use cpu copy force, enable(1)/disable(0) ");

int mtgpu_dma_debug;
module_param(mtgpu_dma_debug, int, 0444);
MODULE_PARM_DESC(mtgpu_dma_debug,
		 "mtgpu udma and hdma debug information enable(1)/disable(0) ");

int mtgpu_util_debug;
module_param(mtgpu_util_debug, int, 0444);
MODULE_PARM_DESC(mtgpu_util_debug,
		 "mtgpu util debug information enable(1)/disable(0) ");

int dma_desc_sysmem_enable = 0;
module_param(dma_desc_sysmem_enable, int, 0444);
MODULE_PARM_DESC(dma_desc_sysmem_enable,
		 "mtgpu dma descriptor location: 0: vram, 1: system. The default value is 0");

int mtgpu_drm_major = 2;
module_param(mtgpu_drm_major, int, 0444);
MODULE_PARM_DESC(mtgpu_drm_major, "1 - ddk1.0, 2 - ddk2.0. The default value is 2");

bool dummy_gpu;
module_param(dummy_gpu, bool, 0644);
MODULE_PARM_DESC(dummy_gpu, "0: gpu is not dummy, 1: gpu is dummy");

bool mtgpu_bt_log_enable = 1;
module_param(mtgpu_bt_log_enable, bool, 0644);
MODULE_PARM_DESC(mtgpu_bt_log_enable, "0: do not print bt log, 1: print bt log");

bool display_only;
module_param(display_only, bool, 0644);
MODULE_PARM_DESC(display_only, "0: off, 1: on");

int mtgpu_sched_mode = 2;
module_param(mtgpu_sched_mode, int, 0444);
MODULE_PARM_DESC(mtgpu_sched_mode, "gpu task scheduler, 0: gpu-fw-only, 1: host sched on nodeq, 2: host sched on ccbq, 3: fec");

bool mtgpu_sched_ccbq_optimize;
module_param(mtgpu_sched_ccbq_optimize, bool, 0644);
MODULE_PARM_DESC(
	mtgpu_sched_ccbq_optimize,
	"0:default, enable gpu-fw update semaphore value when schedule mode is host sched on ccbq");

bool mtgpu_sched_early_submit = 1;
module_param(mtgpu_sched_early_submit, bool, 0444);
MODULE_PARM_DESC(mtgpu_sched_early_submit,
	         "0:default, enable accelerate job submit if there is dependency between jobs from same entity");

unsigned int mtgpu_sched_job_hang_limit_ms = UINT_MAX;
module_param(mtgpu_sched_job_hang_limit_ms, int, 0444);
MODULE_PARM_DESC(mtgpu_sched_job_hang_limit_ms,
		 "default UINT_MAX, delay time for mtgpu sched timeout delay work");

int mtgpu_fw_reboot_retry_times = 2;
module_param(mtgpu_fw_reboot_retry_times, int, 0444);
MODULE_PARM_DESC(mtgpu_fw_reboot_retry_times,
		 "Number of times to retry firmware reboot on failure, The default value is 2");

#if defined(CONFIG_VPS)
int mtgpu_shadow_sem_sysmem_enable; /* default 0 on VPS */
#else
int mtgpu_shadow_sem_sysmem_enable = 1;
#endif
module_param(mtgpu_shadow_sem_sysmem_enable, int, 0444);
MODULE_PARM_DESC(mtgpu_shadow_sem_sysmem_enable,
		 "create mirror semaphore block on sysmem(1)/vram(0), default 0 on VPS, 1 on real machine");

bool mtgpu_enable_poison = 1;
MODULE_PARM_DESC(enable_poison, "ccb/resp/job item release with poison enable(1)/disable(0), default(1)");
module_param_named(enable_poison, mtgpu_enable_poison, bool, 0444);

int mtgpu_sched_host_policy = 1;
module_param(mtgpu_sched_host_policy, int, 0444);
MODULE_PARM_DESC(mtgpu_sched_host_policy, "Specify the scheduling policy for entities on a run-queue, 0 = Round Robin, 1 = FIFO (default).");

bool mtgpu_enable_sysmem_stats = 0;
MODULE_PARM_DESC(enable_sysmem_stats,"enable system memory stats record(0: disable, 1: enable)");
module_param_named(enable_sysmem_stats, mtgpu_enable_sysmem_stats, bool, 0444);

#if defined(MTGPU_ENABLE_PROCESS_STATS)
bool mtgpu_enable_vram_stats = 1;
#else
bool mtgpu_enable_vram_stats;
#endif
MODULE_PARM_DESC(enable_vram_stats,"enable vram stats record(0: disable, 1: enable)");
module_param_named(enable_vram_stats, mtgpu_enable_vram_stats, bool, 0444);

int rm_feature_level = -1;
module_param(rm_feature_level, int, 0644);
MODULE_PARM_DESC(rm_feature_level,
		 "rm feature level enable(0: disable, 1: memory, 2: fw load, 3: intr, 4: queue");

unsigned long long mtgpu_hash_vec_array[] = {
0x000372400721, 0x02208422324a, 0x01324ca10494, 0x0041808009c8, 0x002008091290, 0x000000911520,
0x010008015040, 0x021004008080, 0x02200300a100, 0x010418830200, 0x008150480400, 0x004800102800,
};
int mtgpu_hash_vec_array_size = ARRAY_SIZE(mtgpu_hash_vec_array);

module_param_array(mtgpu_hash_vec_array, ullong, &mtgpu_hash_vec_array_size, 0444);
MODULE_PARM_DESC(mtgpu_hash_vec_array, "gpu hash vector, must contain 12 elements.");

bool mtgpu_riscv_fw_enabled;
MODULE_PARM_DESC(riscv_fw_enabled, "Use fw type 0: gpu-fw, 1: riscv");
module_param_named(enable_riscv_fw, mtgpu_riscv_fw_enabled, bool, 0444);

bool mtgpu_fw_gcov_enabled;
#if defined(DEBUG)
MODULE_PARM_DESC(enable_fw_gcov, "Disable fw gcov: 0, Enable fw gcov: 1");
module_param_named(enable_fw_gcov, mtgpu_fw_gcov_enabled, bool, 0444);
#endif

int mtgpu_refcount_debug;
MODULE_PARM_DESC(mtgpu_refcount_debug, "Ref debug bitmask: 0x1=PMR, 0x2=SyncPrimBlock");
module_param(mtgpu_refcount_debug, int, 0444);

bool mtgpu_pmr_stacktrace_enable;
MODULE_PARM_DESC(mtgpu_pmr_stacktrace_enable, "pmr stacktrace info open (0: disable, 1: enable)");
module_param(mtgpu_pmr_stacktrace_enable, bool, 0644);

bool mtgpu_vram_history_enable;
MODULE_PARM_DESC(mtgpu_vram_history_enable, "vram history record info open (0: disable, 1: enable)");
module_param(mtgpu_vram_history_enable, bool, 0644);

#if (MUSA_NUM_OS_SUPPORTED > 1)
long mtgpu_driver_mode = MTGPU_DRIVER_MODE_HOST;
#else
long mtgpu_driver_mode = MTGPU_DRIVER_MODE_NATIVE;
#endif

static int driver_mode_set(const char *arg, const struct kernel_param *kp)
{
	int ret = 0;
	long new_driver_mode;

	ret = kstrtol(arg, 10, &new_driver_mode);
	if (ret) {
		pr_err("mtgpu: driver mode setting error(kstrtol)\n");
		goto exit;
	}

#if (MUSA_NUM_OS_SUPPORTED > 1)
	if (!(new_driver_mode == MTGPU_DRIVER_MODE_HOST ||
	      new_driver_mode == MTGPU_DRIVER_MODE_GUEST)) {
		pr_err("mtgpu: mtgpu_driver_mode(%ld), setting error in this mode(host = 0,  guest = 1)\n",
		       new_driver_mode);
		ret = -EINVAL;
		goto exit;
	}
#else
	if (new_driver_mode != MTGPU_DRIVER_MODE_NATIVE) {
		pr_err("mtgpu: mtgpu_driver_mode(%ld), setting error in this mode(native = -1)\n",
		       new_driver_mode);
		ret = -EINVAL;
		goto exit;
	}
#endif

	mtgpu_driver_mode = new_driver_mode;
	/* Ensure mtgpu_driver_mode is flushed */
	smp_mb();

exit:
	return ret;
}

static const struct kernel_param_ops driver_mode_ops = {
	.set = driver_mode_set,
	.get = param_get_int,
};

module_param_cb(mtgpu_driver_mode, &driver_mode_ops, &mtgpu_driver_mode, 0444);
MODULE_PARM_DESC(mtgpu_driver_mode,
		 "mtgpu driver mode (native = -1, host = 0, guest = 1)");

unsigned long mtvpu_reserved_mem_size;
module_param(mtvpu_reserved_mem_size, ulong, 0444);
MODULE_PARM_DESC(mtvpu_reserved_mem_size, "reserved gpu memory for VPU");

int mtgpu_ipc_debug;
module_param(mtgpu_ipc_debug, int, 0664);
MODULE_PARM_DESC(mtgpu_ipc_debug,
		 "mtgpu ipc debug information enable(1)/disable(0)");

unsigned long mtgpu_ipc_timeout_ms = 5000;
module_param(mtgpu_ipc_timeout_ms, ulong, 0664);
MODULE_PARM_DESC(mtgpu_ipc_timeout_ms,
		 "mtgpu ipc timeout(unit:ms, default:5000 ms)");

int mtgpu_ipc_tty_support;
module_param(mtgpu_ipc_tty_support, int, 0444);
MODULE_PARM_DESC(mtgpu_ipc_tty_support,
		 "mtgpu ipc tty console enable(1)/disable(0)");

bool enable_mtlink_set;
int enable_mtlink;

static int param_set_enable_mtlink(const char *val, const struct kernel_param *kp)
{
	int ret;
	int new_val;

	ret = kstrtoint(val, 0, &new_val);
	if (ret)
		return ret;

	enable_mtlink = new_val;
	enable_mtlink_set = true;
	os_pr_info("mtgpu: enable_mtlink is set to %d by user\n", enable_mtlink);
	return 0;
}

static const struct kernel_param_ops enable_mtlink_ops = {
	.set = param_set_enable_mtlink,
	.get = param_get_int,
};

module_param_cb(enable_mtlink, &enable_mtlink_ops, &enable_mtlink, 0444);
MODULE_PARM_DESC(enable_mtlink, "1:enable mtlink, 0:disable mtlink");

bool mtlink_timer_expires_set;
unsigned long mtlink_timer_expires = 25000;

static int param_set_mtlink_timer_expires(const char *val, const struct kernel_param *kp)
{
	int ret;
	unsigned long new_val;

	ret = kstrtoul(val, 0, &new_val);
	if (ret)
		return ret;

	mtlink_timer_expires = new_val;
	mtlink_timer_expires_set = true;
	os_pr_info("mtgpu: mtlink_timer_expires is set to %lu by user\n", mtlink_timer_expires);
	return 0;
}

static const struct kernel_param_ops mtlink_timer_expires_ops = {
	.set = param_set_mtlink_timer_expires,
	.get = param_get_ulong,
};

module_param_cb(mtlink_timer_expires, &mtlink_timer_expires_ops, &mtlink_timer_expires, 0444);
MODULE_PARM_DESC(mtlink_timer_expires,
		 "timer expires(ms) from pcie probe to mtlink init, default 25000ms");

unsigned long mtlink_topo_build_timer = 60000;
module_param(mtlink_topo_build_timer, ulong, 0444);
MODULE_PARM_DESC(mtlink_topo_build_timer,
		 "timer expires(ms) user for mtlink topo build, default 60000ms");

int mtlink_topo_type;
module_param(mtlink_topo_type, int, 0444);
MODULE_PARM_DESC(mtlink_topo_type,
		 "0:default, normal topology, 1:eight-card fc topology, 2:two groups of four-card fc topology");

bool mtlink_disable_hwr;
module_param(mtlink_disable_hwr, bool, 0444);
MODULE_PARM_DESC(mtlink_disable_hwr, "0:enable mtlink hwr, 1:disable mtlink hwr,default(0)");

bool fm_is_supported;
module_param(fm_is_supported, bool, 0444);
MODULE_PARM_DESC(fm_is_supported, "0:fm is not supported, 1:fm is supported,default(0)");

unsigned short supper_node_id = 0xffff;
module_param(supper_node_id, ushort, 0444);
MODULE_PARM_DESC(supper_node_id, "supper node id params, default value is invalid");

int enable_event_report = 1;
module_param(enable_event_report, int, 0444);
MODULE_PARM_DESC(enable_event_report,
		 "1:default, enable mtgpu_event_report, 0:disable mtgpu_event_report");

int rb_switch_enable;
module_param(rb_switch_enable, int, 0444);
MODULE_PARM_DESC(rb_switch_enable,
		 "0:default, disable host rx ring buffer switch, 1:enable host rx ring buffer switch");

#if defined(CONFIG_VPS)
unsigned int disable_ipc_bitmask = 0xffff;
#else
unsigned int disable_ipc_bitmask;
#endif
module_param(disable_ipc_bitmask, uint, 0444);
MODULE_PARM_DESC(disable_ipc_bitmask,
		 "0:default, enable ipc, others: disable ipc by setting a specific bit to 0, default 0 on HW, 0xffff on VPS");


int enable_vpu_test = 0;
module_param(enable_vpu_test, int, 0444);
MODULE_PARM_DESC(enable_vpu_test, "0: default, 1: enable vpu test mode");

/*
 * compute_only set 1
 *	disalbe_vpu = 1
 */
int compute_only;
module_param(compute_only, int, 0444);
MODULE_PARM_DESC(compute_only, "0:compute only disable(default), 1:compute only enable");

int force_dma32_zone;
module_param(force_dma32_zone, int, 0444);
MODULE_PARM_DESC(force_dma32_zone, "0:(default) not force use DMA32_ZONE, 1:force use DMA32_ZONE");

#if defined(CONFIG_VPS) || !defined(VPU_ENABLE)
int disable_vpu = 1;
#else
int disable_vpu = 0;
#endif
module_param(disable_vpu, int, 0444);
MODULE_PARM_DESC(disable_vpu, "0: default, 1: disable vpu module.");

unsigned int mtgpu_page_size = 0;
module_param(mtgpu_page_size, uint, 0444);
MODULE_PARM_DESC(mtgpu_page_size, "gpu page size, default to 0.");

unsigned int mtgpu_igpu_svm_heap_page_size = 0x1000;
module_param_named(igpu_svm_heap_page_size, mtgpu_igpu_svm_heap_page_size, uint, 0444);
MODULE_PARM_DESC(mtgpu_igpu_svm_heap_page_size, "svm heap page size for igpu, default to 4KB.");

bool disable_driver;
module_param(disable_driver, bool, 0444);
MODULE_PARM_DESC(disable_driver, "disable mtgpu driver, default 0.");

#if defined(CONFIG_VPS)
bool disable_watchdog = 1;
#else
bool disable_watchdog;
#endif
module_param(disable_watchdog, bool, 0444);
MODULE_PARM_DESC(disable_watchdog, "disable mtgpu watchdog, default 0 on HW, 1 on VPS.");

unsigned int watchdog_timeout_ms = 1000 * 5;
module_param(watchdog_timeout_ms, uint, 0444);
MODULE_PARM_DESC(watchdog_timeout_ms, "watchdog timeout, default 5000ms.");

bool intr_poll_mode_enable;
module_param(intr_poll_mode_enable, bool, 0444);
MODULE_PARM_DESC(intr_poll_mode_enable, "enable poll mode to receive fw response, default 0.");

unsigned int intr_poll_interval_ms = MTGPU_INTR_POLL_INTERVAL_DEFAULT_MS;
module_param(intr_poll_interval_ms, uint, 0644);
MODULE_PARM_DESC(intr_poll_interval_ms,
		 "poll mode interval in ms, default 100ms on HW and 500ms otherwise.");

int vpu_group_max;
module_param(vpu_group_max, int, 0444);
MODULE_PARM_DESC(vpu_group_max, "0: default, 1: there are 6 vpu segments.");

int irq_vector_cnt = 8;
module_param(irq_vector_cnt, int, 0444);
MODULE_PARM_DESC(irq_vector_cnt, "manually modify the number of interrupt vectors supported");

#if defined(CONFIG_VPS)
bool disable_pcie_link_monitor = 1;
#else
bool disable_pcie_link_monitor;
#endif
module_param(disable_pcie_link_monitor, bool, 0444);
MODULE_PARM_DESC(disable_pcie_link_monitor, "disable pcie link monitor, default 0 on HW, 1 on VPS.");

bool enable_reserved_memory;
module_param(enable_reserved_memory, bool, 0444);
MODULE_PARM_DESC(enable_reserved_memory, "enable reserved memory for igpu, default 0.");

bool bypass_igpu_smmu = true;
module_param(bypass_igpu_smmu, bool, 0444);
MODULE_PARM_DESC(bypass_igpu_smmu, "bypass smmu for igpu, default true.");

bool enable_gpu_dvfs = true;
module_param(enable_gpu_dvfs, bool, 0444);
MODULE_PARM_DESC(enable_gpu_dvfs, "enable dvfs for igpu, default true.");

bool igpu_enable_rpm = true;
module_param(igpu_enable_rpm, bool, 0444);
MODULE_PARM_DESC(igpu_enable_rpm, "enable PM-Runtime for igpu, default true.");

unsigned long mtgpu_igpu_rpm_delay_ms = MTGPU_IGPU_SUSPEND_DELAY_MS;
module_param(mtgpu_igpu_rpm_delay_ms, ulong, 0444);
MODULE_PARM_DESC(mtgpu_igpu_rpm_delay_ms, "PM-Runtime delay suspend time for M1000, default 100 ms.");

unsigned long mtgpu_dgpu_rpm_delay_ms = 60000;
module_param(mtgpu_dgpu_rpm_delay_ms, ulong, 0444);
MODULE_PARM_DESC(mtgpu_dgpu_rpm_delay_ms, "PM-Runtime delay suspend time for pcie gpu, default 60s.");

bool dgpu_enable_rpm;
module_param(dgpu_enable_rpm, bool, 0444);
MODULE_PARM_DESC(dgpu_enable_rpm, "Enable PM-Runtime for pcie gpu, default false.");

bool mtgpu_enable_ce_pm = true;
module_param_named(enable_ce_pm, mtgpu_enable_ce_pm, bool, 0444);
MODULE_PARM_DESC(enable_rpm, "enable power management for copy enigine, default true.");

unsigned long mtgpu_devfreq_polling_ms = MTGPU_DEVFREQ_POLLING_MS;
module_param_named(devfreq_polling_ms, mtgpu_devfreq_polling_ms, ulong, 0444);
MODULE_PARM_DESC(devfreq_polling_ms, "devfreq polling time, default 200 ms.");

unsigned int mtgpu_rpm_fast_suspend_upthreshold = MTGPU_PM_RUNTIME_FAST_SUSPEND_UTIL_THRESHOLD;
module_param_named(rpm_fast_suspend_upthreshold, mtgpu_rpm_fast_suspend_upthreshold, uint, 0664);
MODULE_PARM_DESC(rpm_fast_suspend_upthreshold, "util threshold for triggering fast rpm suspend.");

int ignore_mpc_config;
module_param(ignore_mpc_config, int, 0444);
MODULE_PARM_DESC(ignore_mpc_cfg,"ignore mpc configure infomation(0: not ignore, 1: ignore)");

int mtgpu_hwperf_debug;
module_param(mtgpu_hwperf_debug, int, 0444);
MODULE_PARM_DESC(mtgpu_hwperf_debug, "mtgpu hwperf debug ("
				     "0: default disable any debugging, use dma copy from l1 to l2 (ddk2.0), "
				     "bit0: enable event sequence debugging, "
				     "bit1: enable l1 copying debugging, "
				     "bit2~bit3: reserved for other debugging purposes, "
				     "bit4: use memcpy from l1 to l2 (ddk2.0), "
				     "bit5~bit7: reserved for other l1 copy path. )");

unsigned long axi_timeout = 0x200ffffffULL;
module_param(axi_timeout, ulong, 0444);
MODULE_PARM_DESC(axi_timeout, "axi timeout value default 0x200ffffffULL");

int mtgpu_tp_pci_ob_enable;
MODULE_PARM_DESC(tp_pci_ob_enable, "third patry pci ob enable, 1:enable 0:disable");
module_param_named(tp_pci_ob_enable, mtgpu_tp_pci_ob_enable, int, 0444);

unsigned short mtgpu_tp_pci_vender_id;
MODULE_PARM_DESC(tp_pci_vender_id, "third patry pci vender id params, default value is 0");
module_param_named(tp_pci_vender_id, mtgpu_tp_pci_vender_id, ushort, 0444);

int mtgpu_l1_hash_enable;
module_param(mtgpu_l1_hash_enable, int, 0444);
MODULE_PARM_DESC(mtgpu_l1_hash_enable, "mtgpu l1 ctrl (0: default disable, 1: enable)");

unsigned long riscv_fw_load_timeout_us = 100000000ULL;
module_param(riscv_fw_load_timeout_us, ulong, 0444);
MODULE_PARM_DESC(riscv_fw_load_timeout_us, "riscv firmware load timeout in microseconds");

#if (MUSA_NUM_OS_SUPPORTED > 1)
int mtgpu_vgpu_scheduling_policy;
module_param(mtgpu_vgpu_scheduling_policy, int, 0444);
MODULE_PARM_DESC(mtgpu_vgpu_scheduling_policy,
		 "mtgpu vgpu scheduling policy(best effort = 0, time sliced = 1)");

int mtgpu_vgpu_time_sliced_value = 20;
module_param(mtgpu_vgpu_time_sliced_value, int, 0444);
MODULE_PARM_DESC(mtgpu_vgpu_time_sliced_value,
		 "mtgpu vgpu time sliced value in millisecond");

unsigned long mtgpu_vgpu_host_mem_size = 0x80000000;
module_param(mtgpu_vgpu_host_mem_size, ulong, 0444);
MODULE_PARM_DESC(mtgpu_vgpu_host_mem_size,
		 "mtgpu vgpu host os vRAM memory size: default to 2G");

bool mtgpu_vgpu_force_mmio_in_4g;
module_param(mtgpu_vgpu_force_mmio_in_4g, bool, 0444);
MODULE_PARM_DESC(mtgpu_vgpu_force_mmio_in_4g,
		 "force mmio space allocated in 4g range (disable = 0, enable = 1 )");

int vgpu_mm_mapping_mode = 2;
module_param(vgpu_mm_mapping_mode, int, 0444);
MODULE_PARM_DESC(vgpu_mm_mapping_mode,
	"vGPU system memory mapping mode(0: non_linear, 1: linear, 2: linear_prealloc(default))");

int vgpu_iommu_mode = 2;
module_param(vgpu_iommu_mode, int, 0444);
MODULE_PARM_DESC(vgpu_iommu_mode,
		 "vGPU iommu mode(0: force disable, 1: auto, 2: follow system(default))");

int mtgpu_vdma_enable = 1;
module_param(mtgpu_vdma_enable, int, 0444);
MODULE_PARM_DESC(mtgpu_vdma_enable, "vdma enable mode (0: not enable, 1: enable(default))");

unsigned long mtgpu_win_fw_context_switch_value = 0x1E;
module_param(mtgpu_win_fw_context_switch_value, ulong, 0444);
MODULE_PARM_DESC(mtgpu_win_fw_context_switch_value,
	"windows firmware DM(TRANSFER, TA, 3D, CMP) context switch. Each bit represents each DM (0: not enable, 1: enable)");

int mtgpu_vgpu_dyn_mpc_mode = 1;
module_param(mtgpu_vgpu_dyn_mpc_mode, int, 0444);
MODULE_PARM_DESC(mtgpu_vgpu_dyn_mpc_mode, "vgpu dynamic change mpc mode (0: disable, 1: enable(default))");

bool vgpu_qos = 1;
module_param(vgpu_qos, bool, 0444);
MODULE_PARM_DESC(vgpu_qos,"vGPU PCIe bandwidth QoS(0: disable, 1: enable)");

bool vgpu_1g_support_4k = 1;
module_param(vgpu_1g_support_4k, bool, 0444);
MODULE_PARM_DESC(vgpu_1g_support_4k,"1G VGPU support 4K resolution(0: disable, 1: enable)");

bool vgpu_types_tune = 0;
module_param(vgpu_types_tune, bool, 0444);
MODULE_PARM_DESC(vgpu_types_tune,
		 "enable vgpu types tune, if 1st 1101 is tuned(0: disable, 1: enable)");

bool vgpu_host_trigger_hwr = 1;
module_param(vgpu_host_trigger_hwr, bool, 0444);
MODULE_PARM_DESC(vgpu_host_trigger_hwr,
		"Host OS checks whether GPU hardware is normal. If abnormal, host automatically triggers hwr(0: disable, 1: enable)");

#if defined(VGPU_COMPAT_CHECK_MODE_VERSION_LIST_FORCED)
/**
 * For CI release versions, this value is
 * immutable by default to prevent user modifications.
 *
 * @note This Mode version list(1) is not available on the develop branch.
 */
int vgpu_compat_check_mode = 1;
#else
int vgpu_compat_check_mode = 0;
module_param(vgpu_compat_check_mode, int, 0444);
MODULE_PARM_DESC(vgpu_compat_check_mode,
		 "vGPU compatibility check mode(0: disable, 1: version list, 2: strictly match)");
#endif

bool vgpu_time_corr = 1;
module_param(vgpu_time_corr, bool, 0444);
MODULE_PARM_DESC(vgpu_time_corr,"vGPU time correct(0: disable, 1: enable)");

int vgpu_hotplug_upgrade_vram_mode = 1;
module_param(vgpu_hotplug_upgrade_vram_mode, int, 0444);
MODULE_PARM_DESC(
	vgpu_hotplug_upgrade_vram_mode,
	"VGPU hotplug supports upgrading to a VGPU with more vram(0: disable, 1: enable to upgrade to 16G(default), 2 :enable to upgrade to max vram)");

int vgpu_upgrade_mode = 1;
MODULE_PARM_DESC(vgpu_upgrade_mode,
	"vGPU guest driver upgrade mode(0: disable, 1: user interactive)");

bool vgpu_fw_hot_upgrade = 1;
MODULE_PARM_DESC(vgpu_fw_hot_upgrade, "vGPU support fw hot upgrade(0: disable, 1: enable)");

int vgpu_support_agp = 1;
module_param(vgpu_support_agp, int, 0444);
MODULE_PARM_DESC(vgpu_support_agp, "vgpu support agp(0: disable, 1: enable)");

static int set_vgpu_upgrade_mode(const char *val, const struct kernel_param *kp)
{
    int ret;
    int new_mode;

    ret = kstrtoint(val, 10, &new_mode);
    if (ret)
        return ret;

    if (new_mode != vgpu_upgrade_mode) {
        vgpu_upgrade_mode = new_mode;
        vgpu_update_upgrade_conf();
    }

    return 0;
}
module_param_call(vgpu_upgrade_mode, set_vgpu_upgrade_mode,
		  param_get_int, &vgpu_upgrade_mode, 0644);

bool vgpu_allow_pull_driver = 1;
module_param(vgpu_allow_pull_driver, bool, 0644);
MODULE_PARM_DESC(vgpu_allow_pull_driver,
		 "vGPU allow guest to pull driver from host(0: disable, 1: enable");

u64 vgpu_guest_pkg_tag = 0;
module_param(vgpu_guest_pkg_tag, ullong, 0644);
MODULE_PARM_DESC(vgpu_guest_pkg_tag,
		 "vGPU tag info of windows guest driver in the upgrade package");

u64 vgpu_guest_pkg_tag_linux;
module_param(vgpu_guest_pkg_tag_linux, ullong, 0644);
MODULE_PARM_DESC(vgpu_guest_pkg_tag_linux,
		 "vGPU tag info of linux guest driver in the upgrade package");

bool vgpu_incore_migration = 1;
module_param(vgpu_incore_migration, bool, 0444);
MODULE_PARM_DESC(vgpu_incore_migration, "vGPU incore migration(0: disable, 1: enable)");

bool vgpu_overcommit_enable;
module_param(vgpu_overcommit_enable, bool, 0644);
MODULE_PARM_DESC(vgpu_overcommit_enable,
		 "vGPU overcommit mode (0: disable, 1: enable)");

unsigned int vgpu_overcommit_num = 8;
module_param(vgpu_overcommit_num, uint, 0644);
MODULE_PARM_DESC(vgpu_overcommit_num, "vGPU overcommit num 0~28");

#endif

bool mtgpu_pstate_mode_set;
#if defined(CONFIG_VPS)
unsigned char mtgpu_pstate_mode = PSTATE_DISABLED;
#else
unsigned char mtgpu_pstate_mode = PSTATE_ENABLED;
#endif

static int param_set_mtgpu_pstate_mode(const char *val, const struct kernel_param *kp)
{
	int ret;
	u8 new_val;

	ret = kstrtou8(val, 0, &new_val);
	if (ret)
		return ret;

	mtgpu_pstate_mode = new_val;
	mtgpu_pstate_mode_set = true;
	os_pr_info("mtgpu_pstate_mode is set to %u by user\n", mtgpu_pstate_mode);
	return 0;
}

static const struct kernel_param_ops mtgpu_pstate_mode_ops = {
	.set = param_set_mtgpu_pstate_mode,
	.get = param_get_byte,
};

/**
 * pstate mode
 * To control mtgpu pstate mode (0 = disable, 1 = enable).
 * The default is 1 (enabled).
 */
module_param_cb(mtgpu_pstate_mode, &mtgpu_pstate_mode_ops, &mtgpu_pstate_mode, 0444);
MODULE_PARM_DESC(mtgpu_pstate_mode,
		 "mtgpu pstate mode: 0 = disable, 1 = enable; default 0 on VPS");

bool mtgpu_vram_crc_enable;
module_param(mtgpu_vram_crc_enable, bool, 0444);
MODULE_PARM_DESC(mtgpu_vram_crc_enable,
		 "mtgpu VRAM CRC verification enable(1)/disable(0), default 0");

int iommu_pt_enable;
bool enable_iommu_pt_set;
static int iommu_pt_enable_set(const char *val, const struct kernel_param *kp)
{
	int ret;
	int new_val;

	ret = kstrtoint(val, 0, &new_val);
	if (ret)
		return ret;

	if (new_val != 0 && new_val != 1) {
		pr_err("mtgpu: iommu_pt_enable invalid value %d, only 0 or 1 allowed\n",
		       new_val);
		return -EINVAL;
	}

	iommu_pt_enable = new_val;
	enable_iommu_pt_set = true;
	pr_info("mtgpu: iommu_pt_enable is set to %d by user\n", iommu_pt_enable);

	return 0;
}

static const struct kernel_param_ops iommu_passthrough_enable_ops = {
	.set = iommu_pt_enable_set,
	.get = param_get_int,
};

module_param_cb(iommu_pt_enable, &iommu_passthrough_enable_ops,
		&iommu_pt_enable, 0444);
MODULE_PARM_DESC(iommu_pt_enable, "iommu passthrough mode (0: default disable, 1: enable)");

unsigned long mtgpu_debug_config;

static int mtgpu_debug_set(const char *val, const struct kernel_param *kp)
{
	int ret;
	unsigned long new_val;

	ret = kstrtoul(val, 16, &new_val);
	if (ret)
		return ret;

	mtgpu_debug_config = new_val;

	return 0;
}

static const struct kernel_param_ops mtgpu_debug_ops = {
	.set = mtgpu_debug_set,
	.get = param_get_ulong,
};

module_param_cb(debug_log, &mtgpu_debug_ops, &mtgpu_debug_config, 0664);
MODULE_PARM_DESC(debug_log, "mtgpu debug log bitmask: 0x1(CCB debug)");

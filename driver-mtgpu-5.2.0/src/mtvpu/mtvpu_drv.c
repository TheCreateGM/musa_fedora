/*
 * @Copyright   Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License     Dual MIT/GPLv2
 */

#include <linux/platform_device.h>
#include <linux/component.h>
#include <linux/debugfs.h>
#include <linux/uaccess.h>
#include <linux/firmware.h>
#include <linux/kthread.h>
#include <linux/mod_devicetable.h>
#include <linux/pci.h>
#include <linux/version.h>
#include <linux/moduleparam.h>
#include <linux/dma-mapping.h>
#include <drm/drm_gem.h>
#include <drm/drm_drv.h>
#include <linux/acpi.h>
#include <linux/of.h>
#include <linux/vmalloc.h>
#include <linux/kfifo.h> 
#include <linux/timer.h>
#include <linux/slab.h>
#if defined(OS_DRM_DRMP_H_EXIST)
#include <drm/drmP.h>
#else
#include <drm/drm_ioctl.h>
#include <drm/drm_device.h>
#include <drm/drm_file.h>
#endif

#include "mtgpu_drv.h"
#include "mtgpu_drm_drv.h"
#include "mtgpu_drm_internal.h"
#include "mtgpu_module_param.h"
#include "mtgpu_device.h"
#include "mtgpu_smc.h"
#include "vgpu_basic.h"

#include "mtvpu_drv.h"
#include "mtvpu_mem.h"
#include "mtvpu_mon.h"
#include "mtvpu_smmu.h"
#include "mtvpu_pm.h"
#include "mtvpu_clk.h"
#include "mtvpu_fw.h"
#include "mtvpu_mpc.h"
#include "mtvpu_irq.h"
#include "vpuapifunc.h"
#include "helper.h"

#if JPU_LOG_ENABLE
#include "mtvpu_jpulogexport.h"
#endif //JPU_LOG_ENABLE

#include "misc.h"

#ifndef MT_BUILD_VPU
#define MT_BUILD_VPU ""
#endif

int enable_pm_mtvpu_backup_device_memory = 0;
module_param(enable_pm_mtvpu_backup_device_memory, int, 0444);
MODULE_PARM_DESC(enable_pm_mtvpu_backup_device_memory,
		 "mtgpu enable vpu component backup device memory in power manager");

u32 mtvpu_timeout_inflation = 1000;
module_param(mtvpu_timeout_inflation, uint, 0444);
MODULE_PARM_DESC(mtvpu_timeout_inflation,
		 "mtgpu timeout coefficient for vpu operations in ms");

#ifdef DEBUG
int mtvpu_log_level = MTDEBUG;
#else
int mtvpu_log_level = WARN;
#endif
module_param(mtvpu_log_level, int, 0444);
MODULE_PARM_DESC(mtvpu_log_level,
		 "mtgpu vpu component log level lower is important.");

DEFINE_MUTEX(vpu_dir_lock);
struct kfifo *log_fifo = NULL;
void *kfifo_buf = NULL;
static int card_num = 0;

struct platform_driver vpu_rpm_core_driver;

struct file_operations vinfo_fops = {
	.owner = THIS_MODULE,
	.read = vpu_info_read,
	.write = vpu_info_write,
};

struct file_operations fwinfo_fops = {
	.owner = THIS_MODULE,
	.read = fw_info_read,
};

struct file_operations vpulog_fops = {
	.owner = THIS_MODULE,
	.read = vpu_log_read,
};

struct file_operations mtvpu_logs_fops = {
	.read = mtvpu_logs_read,
};

static int vpu_component_bind(struct device *dev, struct device *master, void *data)
{
	struct mt_chip *chip = NULL;
	struct drm_device *drm = data;
	struct mtgpu_drm_private *drm_private = drm->dev_private;
	struct platform_device *pdev = to_platform_device(dev);
	struct mtgpu_device *mtdev;
	struct mtgpu_device_node *dev_node = (struct mtgpu_device_node *)OSGetDeviceNodeFromDrm(drm);
	ulong offset = 0;
	char name[32];
	u32 core_bits = 0;
	int dec_bit_offset = 0;
	int enc_bit_offset = 8;
	int boda_bit_offset = 14;
	int i, j, ret;
	int jpu_core_size = 0;

	bool is_host = mtgpu_get_driver_mode() == MTGPU_DRIVER_MODE_HOST;
	bool is_guest = mtgpu_get_driver_mode() == MTGPU_DRIVER_MODE_GUEST;
	bool is_native = mtgpu_get_driver_mode() == MTGPU_DRIVER_MODE_NATIVE;

	vpu_info("Init %s, Version %s\n", is_native ? "Native" : is_host ? "Host" : "Guest", MT_BUILD_VPU);
	chip = kzalloc(sizeof(struct mt_chip), GFP_KERNEL);
	if (!chip)
		return -ENOMEM;

	if (log_fifo == NULL) {
		log_fifo = vmalloc(sizeof(*log_fifo));
		if (log_fifo == NULL) {
			vpu_err("log_fifo alloc failed\n");
			goto err_log;
		}
		kfifo_buf = vmalloc(LOG_FIFO_SIZE);
		if (kfifo_buf == NULL) {
			vpu_err("kfifo_buf alloc failed");
			goto err_log;
		}
		if (kfifo_init(log_fifo, kfifo_buf, LOG_FIFO_SIZE) != 0) {
			vfree(kfifo_buf);
			kfifo_buf = NULL;
			vfree(log_fifo);
			log_fifo = NULL;
			vpu_err("kfifo_init failed");
			goto err_log;
		}
		VLOG(INFO, "log buffer created, card_num:%d\n", card_num);
	}

	chip->inst_cnt = 0;
	chip->parent = master->parent;
	chip->dev = dev;
	chip->drm_host = drm;
	drm_private->chip = chip;
	chip->driver_mode = mtgpu_get_driver_mode();
	chip->auto_reset_en = 1;
	chip->vpubin_is_ok = 0;
	chip->fw = NULL;

	if (dev_is_pci(chip->parent)) {
		chip->pci_dev = to_pci_dev(chip->parent);
		chip->soc_mode = false;
	} else {
		chip->pci_dev = NULL;
		chip->soc_mode = true;
	}

	mtdev = dev_get_drvdata(chip->parent);
	if (unlikely(!mtdev)) {
		vpu_err("Failed to get mtdev\n");
		goto err_log;
	}
	if (dev_node)
		chip->resizebar_off = dev_node->resizebar_off;
	vpu_info("resize bar off %d\n", chip->resizebar_off);

	if (vpu_init_conf(mtdev->const_info.dev_id, chip, is_guest))
		goto err_log;

	if (init_timer(chip))
		goto err_timer;

	ret = vpu_mmu_context_create(drm, &chip->mmu_ctx);
	if (ret)
		goto err_create_mmu_ctx;

	chip->shared_mem_lock = kzalloc(sizeof(*chip->shared_mem_lock), GFP_KERNEL);
	if (!chip->shared_mem_lock)
		goto err_shared_mem_lock;
	spin_lock_init(chip->shared_mem_lock);

	chip->mpc_lock = kzalloc(sizeof(*chip->mpc_lock), GFP_KERNEL);
	if (!chip->mpc_lock)
		goto err_mpc_lock;
	spin_lock_init(chip->mpc_lock);

	chip->inst_cnt_lock = kzalloc(sizeof(*chip->inst_cnt_lock), GFP_KERNEL);
	if (!chip->inst_cnt_lock)
		goto err_inst_cnt_lock;
	mutex_init(chip->inst_cnt_lock);

	chip->reset_lock = kzalloc(sizeof(*chip->reset_lock), GFP_KERNEL);
	if (!chip->reset_lock)
		goto err_reset_lock;
	mutex_init(chip->reset_lock);

	for (i = 0; i < HOST_OSID_SIZE; ++i) {
		chip->vm_locks[i] = kzalloc(sizeof(*chip->vm_locks[i]), GFP_KERNEL);
		if (!chip->vm_locks[i])
			goto err_vm_lock;
		mutex_init(chip->vm_locks[i]);
	}

	chip->pool_lock = kzalloc(sizeof(*chip->pool_lock), GFP_KERNEL);
	if (!chip->pool_lock)
		goto err_pool_lock;
	spin_lock_init(chip->pool_lock);

	// power on
	vpu_report_power_state(chip, 1);
	if (vpu_is_support_pstate(chip)) {
		vpu_info("power on:%d\n", card_num);
		for (i = 0; i < chip->conf.core_size; i++)
			vpu_core_power_ctl(chip, i, 1);
	}

	for (i = 0; i < chip->conf.core_size; i++) {
		if (is_native || chip->soc_mode)
			chip->core[i].queue_len = COMMAND_QUEUE_DEPTH;
		else
			chip->core[i].queue_len = 1;
		chip->core[i].regs_lock = kzalloc(sizeof(*chip->core[i].regs_lock), GFP_KERNEL);
		if (!chip->core[i].regs_lock)
			goto err_lock;
		mutex_init(chip->core[i].regs_lock);

		chip->core[i].open_lock = kzalloc(sizeof(*chip->core[i].open_lock), GFP_KERNEL);
		if (!chip->core[i].open_lock)
			goto err_lock;
		mutex_init(chip->core[i].open_lock);

		chip->core[i].rpm_lock = kzalloc(sizeof(*chip->core[i].rpm_lock), GFP_KERNEL);
		if (!chip->core[i].rpm_lock)
			goto err_lock;
		mutex_init(chip->core[i].rpm_lock);

		if (chip->conf.product[i] == WAVE517_CODE
		    || chip->conf.product[i] == CODAJ12_CODE
			|| chip->conf.product[i] == CORTEXA35_CODE
		    || chip->conf.product[i] == WAVE627_CODE
		    || chip->conf.product[i] == WAVE627B_CODE)
			for (j = 0; j < INST_MAX_SIZE; j++) {
				chip->core[i].inst_lock[j] =
					kzalloc(sizeof(*chip->core[i].inst_lock[j]), GFP_KERNEL);
				if (!chip->core[i].inst_lock[j])
					goto err_lock;
				mutex_init(chip->core[i].inst_lock[j]);
			}
		if (chip->conf.type == TYPE_PIHU1 ||
		    chip->conf.type == TYPE_PIHU1S ||
		    chip->conf.type == TYPE_LUSHAN) {
			chip->core[i].mem_group_id = 1;
			chip->core[i].mem_group_base = 0;
		}

		chip->core[i].dbg_work = os_create_work();
		if (!chip->core[i].dbg_work)
			goto err_dbg_work;
		os_init_work(chip->core[i].dbg_work, vpu_dbg_work);
	}

	chip->mpc_drm_cnt = mtgpu_get_primary_core_count(mtdev);
	chip->mem_group_cnt = MIN(mtdev->video_group_cnt, MEM_GROUP_MAX_SIZE);

	for (i = 0, j = 0; i < MTGPU_CORE_COUNT_MAX; i++) {
		if (mtdev->drm_dev[i]) {
			chip->drms[j] = dev_get_drvdata(&mtdev->drm_dev[i]->dev);
			if (!chip->drms[j])
				goto err_get_drvdata;
#ifdef SUPPORT_ION
			chip->ions[j] = mtdev->ion_dev[j];
#endif
			chip->mpc_group_ids[j] = mtdev->video_mpc_group_ids[j];
			drm_private = chip->drms[j]->dev_private;
			drm_private->chip = chip;
			j++;
		}
	}
	if (j != chip->mpc_drm_cnt)
		goto err_mpc_drm_cnt;

	ret = mtgpu_smc_get_vpu_core_info(chip->parent, &core_bits);
	if (ret)
		core_bits = 0;

	for (i = 0; i < chip->conf.core_size; i++) {
		if (chip->conf.product[i] == WAVE517_CODE) {
			if ((core_bits >> dec_bit_offset) & 0x1) {
				vpu_info("dec core:%d offset:%d not available\n", i, dec_bit_offset);
				chip->core[i].available = 0;
			} else {
				chip->core[i].available = 1;
				chip->core[i].drm = drm;
				chip->core[i].product_id = WAVE517_CODE;
				vpu_info("dec core:%d offset:%d available\n", i, dec_bit_offset);
				for (j = 0; j < INST_MAX_SIZE; j++)
					INIT_LIST_HEAD(&chip->core[i].bo_list[j]);
			}
			dec_bit_offset++;
		} else if (chip->conf.product[i] == WAVE627_CODE ||
			   chip->conf.product[i] == WAVE627B_CODE) {
			if ((core_bits >> enc_bit_offset) & 0x1) {
				vpu_info("enc core:%d offset:%d not available\n", i, enc_bit_offset);
				chip->core[i].available = 0;
			} else {
				chip->core[i].available = 1;
				chip->core[i].drm = drm;
				chip->core[i].product_id = chip->conf.product[i];
				vpu_info("enc core:%d offset:%d available\n", i, enc_bit_offset);
			}
			enc_bit_offset++;
		} else if (chip->conf.product[i] == CODA980_CODE) {
			if ((core_bits >> boda_bit_offset) & 0x1) {
				vpu_info("dec core:%d offset:%d not available\n", i, boda_bit_offset);
				chip->core[i].available = 0;
			} else {
				chip->core[i].available = 1;
				chip->core[i].drm = drm;
				chip->core[i].product_id = CODA980_CODE;
				vpu_info("dec core:%d offset:%d available\n", i, boda_bit_offset);
			}
		} else if (chip->conf.product[i] == CODAJ12_CODE) {
			chip->core[i].available = 1;
			chip->core[i].drm = drm;
			chip->core[i].product_id = CODAJ12_CODE;
			jpu_core_size++;
			vpu_info("jpu core:%d available\n", i);
		} else if (chip->conf.product[i] == CORTEXA35_CODE) {
			chip->core[i].available = 1;
			chip->core[i].drm = drm;
			chip->core[i].product_id = CORTEXA35_CODE;
			vpu_info("mcu core:%d available\n", i);
		}
		chip->core[i].idx = i;
		chip->core[i].priv = chip;
		chip->core[i].reload_flag = 0;
		chip->core[i].log_read_pos = 0;
		chip->core[i].reset_cnt = 0;
		chip->core[i].loading_surface = 0;
		chip->core[i].serve_all = 1;
	}

	chip->jpu_core_sema = kzalloc(sizeof(*chip->jpu_core_sema), GFP_KERNEL);
	if (!chip->jpu_core_sema)
		goto err_jpu_core_sema;
	sema_init(chip->jpu_core_sema, jpu_core_size);

	if (is_native || is_host) {
		if (chip->soc_mode)
			chip->vaddr = ioremap(chip->conf.regs_base, chip->conf.regs_chip_size);
		else
			chip->vaddr = ioremap(pci_resource_start(chip->pci_dev, 0) + chip->conf.regs_base, chip->conf.regs_chip_size);
		
		chip->mmu_base_va = (u64)chip->vaddr + chip->conf.regs_mmu_offset;
		if (!chip->vaddr)
			goto err_vaddr;

		for (i = 0; i < chip->conf.core_size; i++) {
			chip->core[i].regs = chip->vaddr + offset;
			offset += chip->conf.regs_core_size[i];
		}
	}

	if (chip->soc_mode) {
		chip->bar_base = 0;
		chip->mem_group_cnt = 2;
		vpu_smmu_init(chip);
	} else {
		chip->bar_base = pci_resource_start(chip->pci_dev, 2);
		ret = vpu_init_mpc(chip);
		if (ret)
			goto err_init_mpc;
	}

	chip->fixed_decode_core = 0xff;
	init_max_support_abi_version(chip);

	if (is_guest) {
		vpu_init_guest_mem(chip);
		check_guest_abi_version(chip);
	}

	ret = vpu_init_irq(chip, pdev);
	if (ret)
		goto err_init_irq;

	platform_set_drvdata(pdev, chip);

	ret = vpu_fill_drm_ioctls(mt_drm_ioctls, 128);
	if (ret)
		goto err_fill_drm;

	if (chip->conf.type == TYPE_PIHU1 || chip->conf.type == TYPE_PIHU1S) {
		for (i = 0; i < chip->conf.core_size; i++) {
			if (!JPU_ENABLE_MCU) {
				if (chip->conf.product[i] == CODAJ12_CODE)
					jpu_mmu_init(chip);
			}
		}
	}

	if (is_host) {
		sprintf(name, "vpu-read-msg%d", chip->idx);
		chip->read_msg_threads = kthread_create(read_guest_msg_thread, chip, name);
		wake_up_process(chip->read_msg_threads);

		chip->vdi_thread_timeout = VDI_THREAD_DEFAULT_TIME;

		ret = init_sched2_threads(chip);
		if (ret)
			goto err_init_sched2;
	}

	sprintf(name, "mtvpu%d", chip->idx);
	chip->debugfs = debugfs_create_dir(name, NULL);
	if (chip->debugfs) {
		debugfs_create_file("info", 0666, chip->debugfs, chip, &vinfo_fops);
		debugfs_create_file("fw", 0444, chip->debugfs, chip, &fwinfo_fops);
		for (i = 0; i < chip->conf.core_size; i++) {
			sprintf(name, "logcore%d", i);
			if (chip->conf.product[i] == WAVE517_CODE
			    || chip->conf.product[i] == WAVE627_CODE
			    || chip->conf.product[i] == CORTEXA35_CODE)
				debugfs_create_file(name, 0444, chip->debugfs, &chip->core[i], &vpulog_fops);
		}
		debugfs_create_file("vpulog", 0444, chip->debugfs, chip, &mtvpu_logs_fops);
	}

	if (!chip->soc_mode && (is_native || is_host)) {
		for (i = 0; i < chip->conf.core_size; i++) {
			chip->core[i].core_max_freq = vpu_get_clk(chip, i);
		}
	}

	if (is_native || is_host) {
		for (i = 0; i < chip->conf.core_size; i++)
			if (chip->conf.product[i] == WAVE627_CODE)
				vpu_slice_mode_config(chip, i);
	}

	vpu_report_power_state(chip, -1);

	if (chip->soc_mode) {
		vpu_dvfs_init(chip);
		vpu_rpm_init(dev, VPU_SUSPEND_DELAY);
	}

	vpu_info("vpu driver loaded successfully, card_num %d.\n", card_num);
	++card_num;
	return 0;

err_init_sched2:
err_fill_drm:
err_init_irq:
err_init_mpc:
err_vaddr:
	kfree(chip->jpu_core_sema);
err_jpu_core_sema:
err_mpc_drm_cnt:
err_get_drvdata:
err_dbg_work:
err_lock:
	for (i = 0; i < chip->conf.core_size; i++) {
		if (chip->conf.product[i] == WAVE517_CODE
		    || chip->conf.product[i] == CODAJ12_CODE
			|| chip->conf.product[i] == CORTEXA35_CODE
		    || chip->conf.product[i] == WAVE627_CODE
		    || chip->conf.product[i] == WAVE627B_CODE) {
			for (j = 0; j < INST_MAX_SIZE; j++) {
				if (chip->core[i].inst_lock[j]) {
					mutex_destroy(chip->core[i].inst_lock[j]);
					kfree(chip->core[i].inst_lock[j]);
					chip->core[i].inst_lock[j] = NULL;
				}
			}
		}
		if (chip->core[i].open_lock) {
			mutex_destroy(chip->core[i].open_lock);
			kfree(chip->core[i].open_lock);
			chip->core[i].open_lock = NULL;
		}
		if (chip->core[i].regs_lock) {
			mutex_destroy(chip->core[i].regs_lock);
			kfree(chip->core[i].regs_lock);
			chip->core[i].regs_lock = NULL;
		}
		if (chip->core[i].rpm_lock) {
			mutex_destroy(chip->core[i].rpm_lock);
			kfree(chip->core[i].rpm_lock);
			chip->core[i].rpm_lock = NULL;
		}
	}	

	vpu_report_power_state(chip, -1);
	kfree(chip->pool_lock);
err_vm_lock:
err_pool_lock:
	mutex_destroy(chip->reset_lock);
	kfree(chip->reset_lock);

	for (i = 0; i < HOST_OSID_SIZE; ++i) {
		if (chip->vm_locks[i]) {
			mutex_destroy(chip->vm_locks[i]);
			kfree(chip->vm_locks[i]);
			chip->vm_locks[i] = NULL;
		}
	}
err_reset_lock:
	mutex_destroy(chip->inst_cnt_lock);
	kfree(chip->inst_cnt_lock);
err_inst_cnt_lock:
	kfree(chip->mpc_lock);
err_mpc_lock:
	kfree(chip->shared_mem_lock);
err_shared_mem_lock:
	if (chip->mmu_ctx) {
		vpu_mmu_context_destroy(chip->mmu_ctx);
		chip->mmu_ctx = NULL;
	}
err_create_mmu_ctx:
	kfree(chip->util_timer);
	kfree(chip->stat_timer);
err_timer:
	vpu_deinit_conf(chip);
err_log:
	if (kfifo_buf) {
		vfree(kfifo_buf);
		kfifo_buf = NULL;
	}
	if (log_fifo) {
		vfree(log_fifo);
		log_fifo = NULL;
	}

	kfree(chip);
	vpu_err("Init Error\n");
	return -ENODEV;
}

static void vpu_component_unbind(struct device *dev, struct device *master, void *data)
{
	struct mt_chip *chip;
	struct mt_core *core;
	int idx, i, j;

	chip = dev_get_drvdata(dev);

	if (chip->soc_mode) {
		if (os_pm_runtime_status_suspended(dev))
			os_pm_runtime_get_sync(dev);
		vpu_rpm_exit(dev);
		vpu_dvfs_deinit(chip);
	}

	del_timer_sync(chip->util_timer);
	del_timer_sync(chip->stat_timer);

	kfree(chip->util_timer);
	kfree(chip->stat_timer);

	if (chip->debugfs)
		debugfs_remove_recursive(chip->debugfs);

	for (i = CORE_MAX_SIZE; i >= 0; --i) {
		if (!IS_ERR(chip->host_threads[i]) && chip->host_threads[i] != NULL) {
			vpu_info("stop host thread %d-%d\n", i, chip->idx);
			kthread_stop(chip->host_threads[i]);
			vpu_info("host thread %d-%d stopped\n", i, chip->idx);
		}
		if (!IS_ERR(chip->host_thread_semas[i]) && chip->host_thread_semas[i] != NULL)
			kfree(chip->host_thread_semas[i]);
	}

	if (chip->read_msg_threads)
		kthread_stop(chip->read_msg_threads);

	vpu_free_chip_fw_mem(chip);

	for (i = 0; i < HOST_OSID_SIZE; ++i) {
		if (chip->vm_locks[i]) {
			mutex_destroy(chip->vm_locks[i]);
			kfree(chip->vm_locks[i]);
			chip->vm_locks[i] = NULL;
		}
	}

	for (idx = 0; idx < chip->conf.core_size; idx++) {
		core = &chip->core[idx];
		if (core->bak_addr) {
			vfree(core->bak_addr);
			core->bak_addr = NULL;
		}
		if (core->inited) {
			/* core has been powered off in rpm mode */
			if (!core->pm_dev)
				vpu_hw_deinit(chip->conf.core_base + idx);
			core->inited = 0;
		}

		if (core->open_lock) {
			mutex_destroy(core->open_lock);
			kfree(core->open_lock);
			core->open_lock = NULL;
		}
		if (core->regs_lock) {
			mutex_destroy(core->regs_lock);
			kfree(core->regs_lock);
			core->regs_lock = NULL;
		}
		if (core->dbg_work)
			os_destroy_work(core->dbg_work);
		if (core->rpm_lock) {
			mutex_destroy(core->rpm_lock);
			kfree(core->rpm_lock);
			core->rpm_lock = NULL;
		}
	}

	// power off
	if (vpu_is_support_pstate(chip)) {
		vpu_info("power off:%d\n", card_num);
		for (i = 0; i < chip->conf.core_size; i++)
			vpu_core_power_ctl(chip, i, 0);
	}

	vpu_free_irq(chip);
	kfree(chip->jpu_core_sema);
	kfree(chip->pool_lock);
	mutex_destroy(chip->inst_cnt_lock);
	kfree(chip->inst_cnt_lock);
	mutex_destroy(chip->reset_lock);
	kfree(chip->reset_lock);
	kfree(chip->mpc_lock);
	if (chip->fw) {
		release_firmware(chip->fw);
		chip->fw = NULL;
	}

	for (i = 0; i < chip->conf.core_size; i++) {
		if (chip->conf.product[i] == WAVE517_CODE
		    || chip->conf.product[i] == CODAJ12_CODE
			|| chip->conf.product[i] == CORTEXA35_CODE
		    || chip->conf.product[i] == WAVE627_CODE
		    || chip->conf.product[i] == WAVE627B_CODE) {
			for (j = 0; j < INST_MAX_SIZE; j++) {
				if (chip->core[i].inst_lock[j]) {
					mutex_destroy(chip->core[i].inst_lock[j]);
					kfree(chip->core[i].inst_lock[j]);
				}
			}
		}
	}

	kfree(chip->shared_mem_lock);

	for (i = 0; i < CORE_MAX_SIZE; i++) {
		if (chip->host_task_lock[i])
			kfree(chip->host_task_lock[i]);
	}

	if (chip->soc_mode)
		vpu_smmu_deinit(chip);

	if (chip->mmu_ctx) {
		vpu_mmu_context_destroy(chip->mmu_ctx);
		chip->mmu_ctx = NULL;
	}

	if (log_fifo != NULL && card_num <= 1) {
		VLOG(INFO, "log buffer free, card_num:%d\n", card_num);
		vfree(log_fifo);
		log_fifo = NULL;
		vfree(kfifo_buf);
		kfifo_buf = NULL;
	}

	vpu_deinit_conf(chip);
	kfree(chip);
	--card_num;
	vpu_info("vpu driver unloaded, card_num %d.\n", card_num);
}

static int vpu_core_component_bind(struct device *dev, struct device *master, void *data)
{
	struct platform_device *pdev;
#if defined(CONFIG_ACPI)
	struct acpi_device *adev;
#endif
	struct device_node *node;
	struct drm_device *drm;
	struct mtgpu_drm_private *drm_private;
	struct mt_chip *chip;
	const char *core_name = NULL;
	int core_idx = 0;

	if (!dev || !data)
		return -1;

	pdev = to_platform_device(dev);
	if (!pdev)
		return -1;

	drm = data;
	drm_private = drm->dev_private;
	chip = drm_private->chip;

	if (!chip) {
		vpu_err("chip is not initialized!\n");
		return -1;
	}

	if (chip->soc_mode) {
#if defined(CONFIG_ACPI)
		if (!os_is_acpi_disabled()) {
			adev = ACPI_COMPANION(dev);
			core_name = acpi_device_hid(adev);
		} else
#endif
		if (os_is_acpi_disabled()) {
			node = pdev->dev.of_node;
			core_name = node ? node->name : NULL;
		}
		if (!core_name)
			return -1;
		//os_dev_info(dev, "%s core name:%s\n", __func__, core_name);
		if (!strncmp(core_name, "MSMD0000", 8) ||
			!strncmp(core_name, "smd0", 4))
			core_idx = 0;
		else if (!strncmp(core_name, "MSMD0001", 8) ||
				!strncmp(core_name, "smd1", 4))
			core_idx = 1;
		else if (!strncmp(core_name, "MSMJ0000", 8) ||
				!strncmp(core_name, "smj0", 4))
			core_idx = 2;
		else if (!strncmp(core_name, "MSME0000", 8) ||
				!strncmp(core_name, "sme0", 4))
			core_idx = 4;

		chip->core[core_idx].pm_dev = dev;
		/* boda and codaj12 use the same power switch. */
		if (!strncmp(core_name, "MSMJ0000", 8) ||
			!strncmp(core_name, "smj0", 4))
			chip->core[3].pm_dev = dev;

		platform_set_drvdata(pdev, &chip->core[core_idx]);
		vpu_rpm_init(dev, VPU_CORE_SUSPEND_DELAY);
	} else {
		core_idx = *(int *)os_get_platform_data(dev);
		if (vpu_core_is_support_pstate(chip, core_idx)) {
			chip->core[core_idx].pm_dev = dev;
			platform_set_drvdata(pdev, &chip->core[core_idx]);
			vpu_rpm_init(dev, VPU_CORE_SUSPEND_DELAY);
		}
	}

	return 0;
}

KUNIT_STATIC void vpu_core_component_unbind(struct device *dev, struct device *master, void *data)
{
	struct platform_device *pdev;
	struct mt_core *core;
	struct mt_chip *chip;

	if (!dev)
		return;

	pdev = to_platform_device(dev);
	if (!pdev)
		return;

	core = platform_get_drvdata(pdev);
	if (!core)
		return;

	chip = core->priv;
	if (!chip)
		return;

	if (chip->soc_mode) {
		if (os_pm_runtime_status_suspended(chip->dev))
			os_pm_runtime_get_sync(chip->dev);
	}
	if (core->pm_dev) {
		if (os_pm_runtime_status_suspended(core->pm_dev))
			os_pm_runtime_get_sync(dev);

		vpu_rpm_exit(dev);
		/*
		 For M1000 DTS, WAVE627 is the first core to unbind.
		 Its power state is not correct because it was powered on before VID.
		 We should NOT access WAVE627 core at this stage.
		 Fix this when DTS direct power control is supported.
		*/
		if (chip->soc_mode &&
		    core->product_id == WAVE627_CODE &&
		    os_is_acpi_disabled())
			return;

		if (core->inited)
			vpu_hw_deinit(chip->conf.core_base + core->idx);
	}
}

static const struct component_ops mtvpu_component_ops = {
	.bind   = vpu_component_bind,
	.unbind = vpu_component_unbind,
};

static const struct component_ops mtvpu_core_component_ops = {
	.bind   = vpu_core_component_bind,
	.unbind = vpu_core_component_unbind,
};

static struct platform_device_id vpu_id_tbl[] = {
	{ .name = "mtgpu_vde" },
	{ }
};

static struct of_device_id vpu_of_id_tbl[] = {
	{ .compatible = "mthreads,vpu" },
	{ }
};

static struct acpi_device_id vpu_acpi_id_tbl[] = {
	{ .id = "MVPU0001", .driver_data = 0 },
	{ }
};

static int vpu_probe(struct platform_device *pdev)
{
	return component_add(&pdev->dev, &mtvpu_component_ops);
}

static int vpu_remove(struct platform_device *pdev)
{
	component_del(&pdev->dev, &mtvpu_component_ops);
	return 0;
}

static int vpu_rpm_core_probe(struct platform_device *pdev)
{
	return component_add(&pdev->dev, &mtvpu_core_component_ops);
}

KUNIT_STATIC int vpu_rpm_core_remove(struct platform_device *pdev)
{
	component_del(&pdev->dev, &mtvpu_core_component_ops);
	return 0;
}

static const struct dev_pm_ops vpu_pm_ops = {
	.suspend = vpu_suspend,
	.resume  = vpu_resume,
	.freeze  = vpu_suspend,
	.restore = vpu_resume,
	.complete = vpu_complete,
	.runtime_suspend = vpu_suspend,
	.runtime_resume  = vpu_resume,
};

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 11, 0)
static void _vpu_remove(struct platform_device *pdev)
{
	vpu_remove(pdev);
}
static void _vpu_rpm_core_remove(struct platform_device *pdev)
{
	vpu_rpm_core_remove(pdev);
}
#endif

struct platform_driver vpu_driver = {
	.driver = {
		.name = "mtgpu_vde",
		.pm = &vpu_pm_ops,
		.of_match_table = vpu_of_id_tbl,
		.acpi_match_table = vpu_acpi_id_tbl,
	},
	.probe = vpu_probe,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 11, 0)
	.remove = _vpu_remove,
#else
	.remove = vpu_remove,
#endif
	.id_table = vpu_id_tbl,
};

static struct dev_pm_ops vpu_core_pm_ops = {
	//.suspend = vpu_rpm_core_suspend,
	//.resume  = vpu_rpm_core_resume,
	.runtime_suspend = vpu_rpm_core_suspend,
	.runtime_resume  = vpu_rpm_core_resume,
	.prepare = vpu_core_prepare,
	.complete = vpu_core_complete,
};

static struct acpi_device_id vpu_pm_acpi_id_tbl[] = {
	{.id = "MSMD0000", .driver_data = 0},
	{.id = "MSMD0001", .driver_data = 0},
	{.id = "MSME0000", .driver_data = 0},
	{.id = "MSMJ0000", .driver_data = 0},
	{}
};

static struct platform_device_id vpu_pm_id_tbl[] = {
	{ .name = "mtgpu_vde_pm" },
	{}
};

static struct of_device_id vpu_pm_of_id_tbl[] = {
	{ .compatible = "mthreads,vcore" },
	{}
};

struct platform_driver vpu_rpm_core_driver = {
	.driver = {
		.name = "mtgpu_vde_pm",
		.pm = &vpu_core_pm_ops,
		.of_match_table = vpu_pm_of_id_tbl,
		.acpi_match_table = vpu_pm_acpi_id_tbl,
	},
	.probe = vpu_rpm_core_probe,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 11, 0)
	.remove = _vpu_rpm_core_remove,
#else
	.remove = vpu_rpm_core_remove,
#endif
	.id_table = vpu_pm_id_tbl,
};

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 13, 0)
MODULE_IMPORT_NS("DMA_BUF");
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 16, 0))
MODULE_IMPORT_NS(DMA_BUF);
#endif


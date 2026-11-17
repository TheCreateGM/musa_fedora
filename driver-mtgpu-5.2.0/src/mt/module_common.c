/*************************************************************************/ /*!
@File
@Title          Common Linux module setup
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@License        Dual MIT/GPLv2

The contents of this file are subject to the MIT license as set out below.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

Alternatively, the contents of this file may be used under the terms of
the GNU General Public License Version 2 ("GPL") in which case the provisions
of GPL are applicable instead of those above.

If you wish to allow use of your version of this file only under the terms of
GPL, and not to allow others to use your version of this file under the terms
of the MIT license, indicate your decision by deleting the provisions above
and replace them with the notice and other provisions required by GPL as set
out in the file called "GPL-COPYING" included in this distribution. If you do
not delete the provisions above, a recipient may use your version of this file
under the terms of either the MIT license or GPL.

This License is also included in this distribution in the file called
"MIT-COPYING".

EXCEPT AS OTHERWISE STATED IN A NEGOTIATED AGREEMENT: (A) THE SOFTWARE IS
PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT; AND (B) IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/ /**************************************************************************/

#if defined(OS_DRM_DRMP_H_EXIST)
#include <drm/drmP.h>
#else
#include <drm/drm_device.h>
#include <drm/drm_file.h>
#include <drm/drm_ioctl.h>
#include <linux/device.h>
#endif

#include <linux/module.h>
#include <linux/slab.h>

#if defined(CONFIG_DEBUG_FS)
#include "mt_debugfs.h"
#endif /* defined(CONFIG_DEBUG_FS) */
#if defined(CONFIG_PROC_FS)
#include "mt_procfs.h"
#endif /* defined(CONFIG_PROC_FS) */
#include "di_server.h"
#include "private_data.h"
#include "linkage.h"
#include "power.h"
#include "env_connection.h"
#include "process_stats.h"
#include "module_common.h"
#include "mt.h"
#include "srvcore.h"
#if defined(SUPPORT_MUSA)
#include "musadevice.h"
#endif
#include "mtgpu_error.h"
#include "mt_drv.h"
#include "mt_bridge_k.h"

#include "musastartstop.h"

#include "mt_fence.h"
#include "mtgpu_csc.h"
#include "mtgpu_gfx.h"
#include "mtgpu_dm_kill.h"
#include "mtgpu_ph1_cmp_patch.h"
#include "mtgpu_drv_common.h"
#include "mtgpu_rm.h"

#if defined(SUPPORT_NATIVE_FENCE_SYNC)
#include "mt_sync.h"
#if !defined(USE_MTSYNC_DEVNODE)
#include "mt_sync_ioctl_drm.h"
#endif
#endif

#include "osmt_gputrace.h"

#include "km_apphint.h"
#include "srvinit.h"

#include "mt_ion_stats.h"

#include "mtlink.h"

#if defined(SUPPORT_DMA_TRANSFER)
#include "dma_km.h"
#include "pmr.h"
#endif

#if (MUSA_NUM_OS_SUPPORTED > 1)
#include "musadebug.h"
#endif

#include "musafwutils.h"
#include "musashader.h"
#include "mtgpu_module_param.h"
#include "mtgpu_watchdog.h"
#include "mtgpu_event_report.h"
#include "mtgpu_sched.h"
#include "mtgpu_fw.h"
#include "mtgpu_buffer_sync_v2.h"
#include "mtgpu_util.h"
#include "mtgpu_devfreq.h"
#include "mtgpu_mdev.h"
#include "mtgpu_ioctl.h"
#include "mtgpu_mtext.h"
#include "vgpu_fw.h"
#include "mtgpu_device.h"

long vram_lower_limit = 0x10000000;
module_param(vram_lower_limit, long, 0444);
MODULE_PARM_DESC(vram_lower_limit, "when the remaining memory space "
		 "of vram is less than this parameter (256MB default),"
		 "the gfx buffers will be allocated from system memory");

#if defined(DEBUG)
MT_UINT32 gPMRAllocFail;
module_param(gPMRAllocFail, uint, 0644);
MODULE_PARM_DESC(gPMRAllocFail, "When number of PMR allocs reaches "
				 "this value, it will fail (default value is 0 which "
				 "means that alloc function will behave normally).");
#endif /* defined(DEBUG) */

bool disable_gpu;
module_param(disable_gpu, bool, 0444);
MODULE_PARM_DESC(disable_gpu, "0 - enable gpu, 1 - disable gpu. The default value is 0");

#if defined(CONFIG_VPS)
bool disable_hwr = 1;
#else
bool disable_hwr;
#endif
module_param(disable_hwr, bool, 0444);
MODULE_PARM_DESC(disable_hwr, "0 - enable hwr, 1 - disable hwr. Default 0 on HW, 1 on VPS");

int enable_large_mem_mode;
module_param(enable_large_mem_mode, int, 0444);
MODULE_PARM_DESC(enable_large_mem_mode,
                "1:enable compute memory mode, 0:disable compute memory mode");

bool enable_mmu_persistence = 0;
module_param(enable_mmu_persistence, bool, 0444);
MODULE_PARM_DESC(enable_mmu_persistence, "0 - disable mmu persistence,  1 - enable mmu persistence. The default value is 0");

unsigned int enable_async_job_submit;
module_param(enable_async_job_submit, uint, 0444);
MODULE_PARM_DESC(enable_async_job_submit,
		 "enable job submit asynchronously, default:0. bit0~bit3 corresponds to tq,ce,compute,render job respectively");

bool enable_pfm_mss = 0;
module_param(enable_pfm_mss, bool, 0444);
MODULE_PARM_DESC(enable_pfm_mss, "enable MSS function, 0 - disable MSS, 1 - enable MSS. The default value is 0");

bool mss_mmu_bypass = 0;
module_param(mss_mmu_bypass, bool, 0444);
MODULE_PARM_DESC(mss_mmu_bypass, "bypass MSS MMU function, 0 - enable MSS MMU, 1 - bypass MSS MMU. The default value is 0");

unsigned int enable_cache_snooping = 0x2;
module_param(enable_cache_snooping, uint, 0444);
MODULE_PARM_DESC(enable_cache_snooping,
		 "enable GPU snooping into CPU cache, bit0~bit1 corresponds to non-MMU requests and MMU requests. "
		 "The default value is 0 which means that cache snooping will be disabled.");

bool hwperf_process_enable_reset_params = true;
module_param(hwperf_process_enable_reset_params, bool, 0444);
MODULE_PARM_DESC(hwperf_process_enable_reset_params,
		 "enable reset params of hwperf process, default: true.");

int hwperf_process_log_level;
module_param(hwperf_process_log_level, int, 0444);
MODULE_PARM_DESC(hwperf_process_log_level, "hwperf process log ("
					   "0b0(0): default log, "
					   "0b1(1): normal entrance exit function log, "
					   "0b10(2): thread log, "
					   "0b100(4): hwperf data acquisition log), default: 0");

unsigned long hwperf_process_acquire_data_timeout = 5000000000ULL;
module_param(hwperf_process_acquire_data_timeout, ulong, 0444);
MODULE_PARM_DESC(hwperf_process_acquire_data_timeout, "hwperf process acquire data timeout(ns), "
						      "default: 5000000000ULL");

#if defined(SUPPORT_DISPLAY_CLASS)
/* Display class interface */
#include "kerneldisplay.h"
EXPORT_SYMBOL(DCRegisterDevice);
EXPORT_SYMBOL(DCUnregisterDevice);
EXPORT_SYMBOL(DCDisplayConfigurationRetired);
EXPORT_SYMBOL(DCDisplayHasPendingCommand);
EXPORT_SYMBOL(DCImportBufferAcquire);
EXPORT_SYMBOL(DCImportBufferRelease);

/* Physmem interface (required by LMA DC drivers) */
#include "physheap.h"
EXPORT_SYMBOL(PhysHeapAcquireByUsage);
EXPORT_SYMBOL(PhysHeapRelease);
EXPORT_SYMBOL(PhysHeapGetType);
EXPORT_SYMBOL(PhysHeapGetCpuPAddr);
EXPORT_SYMBOL(PhysHeapGetSize);
EXPORT_SYMBOL(PhysHeapCpuPAddrToDevPAddr);

EXPORT_SYMBOL(MTGPUGetDriverStatus);
EXPORT_SYMBOL(MTGPUSystemInstallDeviceLISR);
EXPORT_SYMBOL(MTGPUSystemUninstallDeviceLISR);

#include "mt_notifier.h"
EXPORT_SYMBOL(MTGPUCheckStatus);

#include "mt_debug.h"
EXPORT_SYMBOL(MTGPUGetErrorString);
#endif /* defined(SUPPORT_DISPLAY_CLASS) */

#if defined(SUPPORT_MUSA)
#include "musaapi_km.h"
#if defined(SUPPORT_SHARED_L2)
EXPORT_SYMBOL(MUSAInitL2);
#endif
EXPORT_SYMBOL(MUSAHWPerfConnect);
EXPORT_SYMBOL(MUSAHWPerfDisconnect);
EXPORT_SYMBOL(MUSAHWPerfControl);
#if defined(MUSA_FEATURE_HWPERF_MUSA)
EXPORT_SYMBOL(MUSAHWPerfConfigureCounters);
#else
EXPORT_SYMBOL(MUSAHWPerfConfigMuxCounters);
EXPORT_SYMBOL(MUSAHWPerfConfigureAndEnableCustomCounters);
#endif
EXPORT_SYMBOL(MUSAHWPerfDisableCounters);
EXPORT_SYMBOL(MUSAHWPerfAcquireEvents);
EXPORT_SYMBOL(MUSAHWPerfReleaseEvents);
EXPORT_SYMBOL(MUSAHWPerfConvertCRTimeStamp);
#if defined(SUPPORT_KERNEL_HWPERF_TEST)
EXPORT_SYMBOL(OSAddTimer);
EXPORT_SYMBOL(OSEnableTimer);
EXPORT_SYMBOL(OSDisableTimer);
EXPORT_SYMBOL(OSRemoveTimer);
#endif
#endif

#if (MUSA_NUM_OS_SUPPORTED > 1)
EXPORT_SYMBOL(_MUSADumpMUSAMMUFaultStatus);
#endif

#define MTGPU_SUSPEND_PENDING_RETRY_TIMES 5

static int MTGPUDeviceSyncOpen(struct _MTGPU_DEVICE_NODE_ *psDeviceNode,
                                struct drm_file *psDRMFile);
void MTGPUDeviceSyncRelease(void *pConnectionData);

CONNECTION_DATA *LinuxServicesConnectionFromFile(struct file *pFile)
{
	if (pFile)
	{
		struct drm_file *psDRMFile = pFile->private_data;
		MTGPU_CONNECTION_PRIV *psConnectionPriv = (MTGPU_CONNECTION_PRIV*)psDRMFile->driver_priv;

		return (CONNECTION_DATA*)psConnectionPriv->pvConnectionData;
	}

	return NULL;
}

struct mtgpu_session *mtgpu_get_session(struct file *file)
{
	return (struct mtgpu_session *)LinuxServicesConnectionFromFile(file);
}

CONNECTION_DATA *LinuxSyncConnectionFromFile(struct file *pFile)
{
	if (pFile)
	{
		struct drm_file *psDRMFile = pFile->private_data;
		MTGPU_CONNECTION_PRIV *psConnectionPriv = (MTGPU_CONNECTION_PRIV*)psDRMFile->driver_priv;

#if (MTGPU_DEVICE_INIT_MODE == MTGPU_LINUX_DEV_INIT_ON_CONNECT)
		return (CONNECTION_DATA*)psConnectionPriv->pvConnectionData;
#else
		return (CONNECTION_DATA*)psConnectionPriv->pvSyncConnectionData;
#endif
	}

	return NULL;
}

struct apphint_state apphint = {
/* statically initialise default values to ensure that any module_params
 * provided on the command line are not overwritten by defaults.
 */
	.val = {
#define UINT32Bitfield UINT32
#define UINT32List UINT32
#define X(a, b, c, d, e) \
	{ {NULL}, {NULL}, NULL, NULL, {.b=d}, false },
	APPHINT_LIST_ALL
#undef X
#undef UINT32Bitfield
#undef UINT32List
	},
	.initialized = 0,
	.num_devices = 0
};


__maybe_unused
const struct kernel_param_ops apphint_kparam_fops = {
	.set = apphint_kparam_set,
	.get = apphint_kparam_get,
};

/*
 * call module_param_cb() for all AppHints listed in APPHINT_LIST_MODPARAM_COMMON + APPHINT_LIST_MODPARAM
 * apphint_modparam_class_ ## resolves to apphint_modparam_enable() except for
 * AppHint classes that have been disabled.
 */

#define apphint_modparam_enable(name, number, perm) \
	module_param_cb(name, &apphint_kparam_fops, &apphint.val[number], perm);

#define X(a, b, c, d, e) \
	apphint_modparam_class_ ##c(a, APPHINT_ID_ ## a, 0444)
	APPHINT_LIST_MODPARAM_COMMON
	APPHINT_LIST_MODPARAM
#undef X

struct workqueue_struct *mtgpu_wq;

bool mtgpu_queue_work(struct work_struct *work)
{
	return queue_work(mtgpu_wq, work);
}

void mtgpu_flush_workqueue(void)
{
	flush_workqueue(mtgpu_wq);
}

/**************************************************************************/ /*!
@Function     MTGPUDriverInit
@Description  Common one time driver initialisation
@Return       int           0 on success and a Linux error code otherwise
*/ /***************************************************************************/
int MTGPUDriverInit(void)
{
	MTGPU_ERROR error;
	int os_err;

	error = MTOSFuncInit();
	if (error != MTGPU_OK)
	{
		return -ENOMEM;
	}

	error = MTGPUCommonDriverInit();
	if (error != MTGPU_OK)
	{
		return -ENODEV;
	}

#if defined(SUPPORT_NATIVE_FENCE_SYNC)
	error = mt_sync_register_functions();
	if (error != MTGPU_OK)
	{
		return -EPERM;
	}

	os_err = mt_sync_init();
	if (os_err != 0)
	{
		return os_err;
	}
#endif

#if !defined(NO_HARDWARE)
	os_err = mtgpu_sched_fence_slab_init();
	if (os_err != 0)
	{
		return os_err;
	}

	os_err = mtgpu_buffer_fence_ops_init();
	if (os_err != 0)
	{
		return os_err;
	}
#endif

	os_err = mt_apphint_init();
	if (os_err != 0)
	{
		MT_DPF((MT_DBG_WARNING, "%s: failed AppHint setup(%d)", __func__,
			     os_err));
	}

#if defined(SUPPORT_MUSA)
	error = MTGpuTraceSupportInit();
	if (error != MTGPU_OK)
	{
		return -ENOMEM;
	}
#endif

#if defined(CONFIG_DEBUG_FS)
	error = MTDebugFsRegister();
	if (error != MTGPU_OK)
	{
		return -ENOMEM;
	}
#elif defined(CONFIG_PROC_FS)
	error = MTProcFsRegister();
	if (error != MTGPU_OK)
	{
		return -ENOMEM;
	}
#endif /* defined(CONFIG_DEBUG_FS) || defined(CONFIG_PROC_FS) */

#if defined(SUPPORT_MUSA)
	/* calling here because we need to handle input from the file even
	 * before the devices are initialised
	 * note: we're not passing a device node because apphint callbacks don't
	 * need it */
	MTGpuTraceInitAppHintCallbacks(NULL);
#endif

	mtgpu_wq = alloc_workqueue("mtgpu-wq", 0, 0);
	if (!mtgpu_wq)
	{
		return -ENOMEM;
	}

	return 0;
}

/**************************************************************************/ /*!
@Function     MTGPUDriverDeinit
@Description  Common one time driver de-initialisation
@Return       void
*/ /***************************************************************************/
void MTGPUDriverDeinit(void)
{
	mt_apphint_deinit();

#if !defined(NO_HARDWARE)
	mtgpu_buffer_fence_ops_deinit();
	mtgpu_sched_fence_slab_fini();
#endif

#if defined(SUPPORT_NATIVE_FENCE_SYNC)
	mt_sync_deinit();
#endif

	destroy_workqueue(mtgpu_wq);

	MTGPUCommonDriverDeInit();

#if defined(SUPPORT_MUSA)
	MTGpuTraceSupportDeInit();
#endif

	MTOSFuncDeInit();
}

/**************************************************************************/ /*!
@Function     MTGPUDeviceInit
@Description  Common device related initialisation.
@Input        psDeviceNode  The device node for which initialisation should be
                            performed
@Return       int           0 on success and a Linux error code otherwise
*/ /***************************************************************************/
int MTGPUDeviceInit(MTGPU_DEVICE_NODE *psDeviceNode)
{
	int error = 0;

#if !defined(NO_HARDWARE)
	{
		struct mtgpu_device *mtdev =
			OSGetMtgpuDevice(OSGetPlatformDevice(psDeviceNode->psDevConfig->pvOSDevice));
		MTGPU_MUSADEV_INFO *psDevInfo = psDeviceNode->pvDevice;

		if (!RM_IS_ENABLED())
		{
			error = mtgpu_csc_table_init((struct mtgpu_device_node *)psDeviceNode);
			if (error)
			{
				MT_DPF((MT_DBG_ERROR, "Failed to init csc table (%u)", error));
				return error;
			}

			error = mtgpu_dm_kill_init((struct mtgpu_device_node *)psDeviceNode);
			if (error)
			{
				MT_DPF((MT_DBG_ERROR, "Failed to init dm kill buffer (%u)", error));
				goto err_dm_kill;
			}
		}

		if (!MTGPU_VZ_MODE_IS(GUEST)) {
			error = mtgpu_ph1_cmp_patch_init((struct mtgpu_device_node *)psDeviceNode);
			if (error) {
				MT_DPF((MT_DBG_ERROR, "Failed to init ph1 cmp patch (%u)", error));
				goto err_ph1_cmp_patch;
			}
		}

		mtgpu_device_get_mpx_map(mtdev, (void *)psDevInfo->pvRegsBaseKM);
	}

#endif

#if defined(SUPPORT_NATIVE_FENCE_SYNC)
	{
		MTGPU_ERROR eError = mt_sync_device_init(psDeviceNode->psDevConfig->pvOSDevice);
		if (eError != MTGPU_OK)
		{
			MT_DPF((MT_DBG_ERROR, "%s: unable to create sync (%d)",
					 __func__, eError));
			error = -EBUSY;
			goto err_mt_sync_device_init;
		}
	}
#endif

#if defined(SUPPORT_MUSA)
	{
		int error = MTGpuTraceInitDevice(psDeviceNode);
		if (error != 0)
		{
			MT_DPF((MT_DBG_WARNING,
				"%s: failed to initialise MTGPU GPU Tracing on device%d (%d)",
				__func__, psDeviceNode->sDevId.i32OsDeviceID, error));
		}
	}
#endif

	return 0;

err_mt_sync_device_init:
	if (!MTGPU_VZ_MODE_IS(GUEST))
		mtgpu_ph1_cmp_patch_deinit((struct mtgpu_device_node *)psDeviceNode);
err_ph1_cmp_patch:
	if (!RM_IS_ENABLED())
	{
		mtgpu_dm_kill_deinit((struct mtgpu_device_node *)psDeviceNode);
	}
err_dm_kill:
	if (!RM_IS_ENABLED())
	{
		mtgpu_csc_table_deinit((struct mtgpu_device_node *)psDeviceNode);
	}
	return error;
}

/**************************************************************************/ /*!
@Function     MTGPUDeviceDeinit
@Description  Common device related de-initialisation.
@Input        psDeviceNode  The device node for which de-initialisation should
                            be performed
@Return       void
*/ /***************************************************************************/
void MTGPUDeviceDeinit(MTGPU_DEVICE_NODE *psDeviceNode)
{
	struct workqueue_struct *psMtFenceWq;
	struct mtgpu_device_node *dev_node = (struct mtgpu_device_node *)psDeviceNode;

#if defined(SUPPORT_MUSA)
	MTGpuTraceDeInitDevice(psDeviceNode);
#endif

#if defined(SUPPORT_NATIVE_FENCE_SYNC)
	mt_sync_device_deinit(psDeviceNode->psDevConfig->pvOSDevice);
#endif

	if (!RM_IS_ENABLED())
	{
		mtgpu_csc_table_deinit(dev_node);
		mtgpu_dm_kill_deinit(dev_node);
		mtgpu_gfx_pb_deinit(dev_node);
	}

	if (!MTGPU_VZ_MODE_IS(GUEST))
		mtgpu_ph1_cmp_patch_deinit(dev_node);

#if defined(SUPPORT_DMA_TRANSFER)
	MTGPUDeInitialiseDMA(psDeviceNode);
#endif

#if defined(SUPPORT_NATIVE_FENCE_SYNC) || defined(SUPPORT_BUFFER_SYNC)
	psMtFenceWq = NativeSyncGetFenceStatusWq();
	if (psMtFenceWq)
	{
		flush_workqueue(psMtFenceWq);
	}
#endif

	flush_workqueue(mtgpu_wq);
}

/**************************************************************************/ /*!
@Function     MTGPUDeviceShutdown
@Description  Common device shutdown.
@Input        psDeviceNode  The device node representing the device that should
                            be shutdown
@Return       void
*/ /***************************************************************************/

void MTGPUDeviceShutdown(MTGPU_DEVICE_NODE *psDeviceNode)
{
	MTGPU_ERROR eError;

#if !defined(NO_HARDWARE)
	struct mtgpu_device_node *dev_node = (struct mtgpu_device_node *)psDeviceNode;
	struct device *dev = OSGetPcieDeviceFromOSDevice(dev_node->dev);

	if (MTGPU_VZ_MODE_IS(GUEST))
		vgpu_fw_guest_disconnect(dev_node);

	if (OSDeviceIsPCI(dev))
		mtgpu_watchdog_stop(dev_node);
	else
		MTGPUDeviceSuspend(psDeviceNode);
#endif

	/*
	 * Disable the bridge to stop processes trying to use the driver
	 * after it has been shut down.
	 */
	eError = LinuxBridgeBlockClientsAccess(MT_TRUE);

	if (eError != MTGPU_OK)
	{
		MT_DPF((MT_DBG_ERROR,
			"%s: Failed to suspend driver (%d)",
			__func__, eError));
		return;
	}

	(void) MTGPUSetDeviceSystemPowerState(psDeviceNode,
	                                       MTGPU_SYS_POWER_STATE_OFF,
	                                       MTGPU_POWER_FLAGS_NONE);
}

/**************************************************************************/ /*!
@Function     MTGPUDeviceRuntimeResumeIGPU
@Description  Common device resume.
@Input        psDeviceNode  The device node representing the device that should
                            be resumed
@Return       int           0 on success and a Linux error code otherwise
*/ /***************************************************************************/
int MTGPUDeviceRuntimeResumeIGPU(MTGPU_DEVICE_NODE *psDeviceNode)
{
	int err;
	struct mtgpu_device_node *dev_node =
		(struct mtgpu_device_node *)psDeviceNode;

	if (!MTGPU_VZ_MODE_IS(GUEST))
	{
		err = mtgpu_fw_start(dev_node);
		if (err)
		{
			return err;
		}
	}

	mtgpu_mtext_init(dev_node);

	mtgpu_watchdog_start(dev_node);

	mtgpu_util_info_reset(dev_node);

	mtgpu_devfreq_set_enabled(dev_node->devfreq);

	return 0;
}

/**************************************************************************/ /*!
@Function     MTGPUDeviceRuntimeSuspendIGPU
@Description  Common device runtime suspend.
@Input        psDeviceNode  The device node representing the device that should
                            be resumed
@Return       int           0 on success and a Linux error code otherwise
*/ /***************************************************************************/
int MTGPUDeviceRuntimeSuspendIGPU(MTGPU_DEVICE_NODE *psDeviceNode)
{
	int err;
	struct mtgpu_device_node *dev_node =
		(struct mtgpu_device_node *)psDeviceNode;

	mtgpu_devfreq_set_disabled(dev_node->devfreq);

	mtgpu_watchdog_stop(dev_node);

	if (!MTGPU_VZ_MODE_IS(GUEST))
	{
		mtgpu_fw_stop(dev_node);

		err = mtgpu_fw_wait_idle(dev_node);
		if (err)
		{
			MT_DPF((MT_DBG_ERROR, "fw wait idle failed in runtime suspend %d", err));
			return err;
		}
	}

	return 0;
}

static inline void MTGPUDeviceDrmSchedsStop(struct mtgpu_device_node *dev_node)
{
	int i;
	int max_queue_num = MTFW_FWIF_QUEUE_TYPE_MAX;

	for (i = 0; i < max_queue_num; i++)
		mtgpu_sched_stop(dev_node->scheduler[i], NULL);
}

static int MTGPUDeviceSchedulerStop(struct mtgpu_device_node *dev_node)
{
	int err;
	int i;
	int max_queue_num = MTFW_FWIF_QUEUE_TYPE_MAX;

	if (!mtgpu_sched_on_host())
		return 0;

	if (dev_node->scheduler) {
		if(!mtgpu_ioctl_wait_completed_timeout(dev_node)) {
			MT_DPF((MT_DBG_ERROR, "Timeout to wait ioctls to be completed, resubmit jobs"));
			goto resubmit;
		}

		if (!mtgpu_job_wait_all_lists_empty(dev_node)) {
			MT_DPF((MT_DBG_ERROR, "Timeout to wait job_list to be empty, resubmit jobs"));
			goto resubmit;
		}

		MTGPUDeviceDrmSchedsStop(dev_node);
	}

	return 0;

resubmit:
	MTGPUDeviceDrmSchedsStop(dev_node);

	err = mtgpu_fw_reboot(dev_node);
	if (err) {
		MT_DPF((MT_DBG_ERROR, "Reboot fw failed in suspend %d", err));
		return err;
	}

	for (i = 0; i < max_queue_num; i++)
		mtgpu_sched_resubmit_jobs(dev_node->scheduler[i], true);

	return 0;
}

static int MTGPUDeviceSuspendIGPU(MTGPU_DEVICE_NODE *psDeviceNode)
{
	struct mtgpu_device_node *dev_node = (struct mtgpu_device_node *)psDeviceNode;
	int err;

	mtgpu_devfreq_suspend(dev_node->devfreq);

	if (MTGPU_VZ_MODE_IS(HOST))
		vgpu_wait_mpc_all_vm_is_idle(dev_node->fw_info);

	/* When PM-Runtime is enabled, and the device is in RPM_SUSPENDED,
	 * waiting idle and rpm_suspend() will not be executed.
	 */
	if (OSPMRuntimeEnabled(dev_node->dev) && OSPMRuntimeStatusSuspended(dev_node->dev)) {
		/* Stop drm schedulers */
		MTGPUDeviceDrmSchedsStop(dev_node);
	} else {
		err = MTGPUDeviceSchedulerStop(dev_node);
		if (err)
			return err;

		MTGPUDeviceRuntimeSuspendIGPU(psDeviceNode);
	}

	return 0;
}

static int MTGPUDeviceSuspendDGPU(MTGPU_DEVICE_NODE *psDeviceNode)
{
	struct mtgpu_device_node *dev_node = (struct mtgpu_device_node *)psDeviceNode;
	struct device *dev = OSGetPcieDeviceFromOSDevice(dev_node->dev);
	struct mtgpu_device *mtdev = os_dev_get_drvdata(dev);
	int err;

	mtgpu_watchdog_stop(dev_node);
	mtgpu_pstate_set_suspend(mtdev);

	err = MTGPUDeviceSchedulerStop(dev_node);
	if (err)
		return err;

	if (!MTGPU_VZ_MODE_IS(GUEST)) {
		mtgpu_fw_stop(dev_node);
		mtgpu_fw_wait_idle(dev_node);
	}

	return 0;
}

/**************************************************************************/ /*!
@Function     MTGPUDeviceSuspend
@Description  Common device suspend.
@Input        psDeviceNode  The device node representing the device that should
                            be suspended
@Return       int           0 on success and a Linux error code otherwise
*/ /***************************************************************************/
int MTGPUDeviceSuspend(MTGPU_DEVICE_NODE *psDeviceNode)
{
	struct mtgpu_device_node *dev_node = (struct mtgpu_device_node *)psDeviceNode;
	struct device *dev = OSGetPcieDeviceFromOSDevice(dev_node->dev);
	int err;

	mtgpu_ioctl_block_access(dev_node);

	err = OSDeviceIsPCI(dev) ? MTGPUDeviceSuspendDGPU(psDeviceNode) : MTGPUDeviceSuspendIGPU(psDeviceNode);
	if (err) {
		MT_DPF((MT_DBG_ERROR, "suspend failed , force resume system"));
		mtgpu_ioctl_unblock_access(dev_node);
		return err;
	}

	return 0;
}

static int MTGPUDeviceResumeIGPU(MTGPU_DEVICE_NODE *psDeviceNode)
{
	struct mtgpu_device_node *dev_node = (struct mtgpu_device_node *)psDeviceNode;
	int err;

	/* When PM-Runtime is disabled, rpm_resume needs to be executed.
		* When PM-Runtime is enabled, both pm_resume() and rpm_resume() will execute the following process.
		* Therefore, when the device enters S3 with RPM_SUSPENDED, these ops will be executed in rpm_resume().
		*/
	if (!OSPMRuntimeEnabled(dev_node->dev) || !OSPMRuntimeStatusSuspended(dev_node->dev))
	{
		err = MTGPUDeviceRuntimeResumeIGPU(psDeviceNode);
		if (err)
		{
			MT_DPF((MT_DBG_ERROR, "Failed to start fw in resume (%d)", LinuxErrnoToMtError(err)));
			return err;
		}
	}

	mtgpu_devfreq_resume(dev_node->devfreq);

	return 0;
}

static int MTGPUDeviceResumeDGPU(MTGPU_DEVICE_NODE *psDeviceNode)
{
	struct mtgpu_device_node *dev_node = (struct mtgpu_device_node *)psDeviceNode;
	int err;

	mtgpu_util_info_reset(dev_node);

	if (!MTGPU_VZ_MODE_IS(GUEST)) {
		if (dev_node->fw_info->fw_need_verify) {
			err = mtgpu_fw_load_and_verify(dev_node->fw_info);
			if (err)
			{
				MT_DPF((MT_DBG_ERROR, "Failed to load and verify fw for resume (%d)",
					LinuxErrnoToMtError(err)));
				return LinuxErrnoToMtError(err);
			}
		}

		err = mtgpu_fw_start(dev_node);
		if (err)
		{
			MT_DPF((MT_DBG_ERROR, "Failed to MUSAStart for resume (%d)", LinuxErrnoToMtError(err)));
			return LinuxErrnoToMtError(err);
		}
	}

	mtgpu_watchdog_start(dev_node);

	return 0;
}

/**************************************************************************/ /*!
@Function     MTGPUDeviceResume
@Description  Common device resume.
@Input        psDeviceNode  The device node representing the device that should
                            be resumed
@Return       int           0 on success and a Linux error code otherwise
*/ /***************************************************************************/
int MTGPUDeviceResume(MTGPU_DEVICE_NODE *psDeviceNode)
{
	int err;
	struct mtgpu_device_node *dev_node = (struct mtgpu_device_node *)psDeviceNode;
	struct device *dev = OSGetPcieDeviceFromOSDevice(dev_node->dev);
	unsigned long reonline_osid = 0;

	if (RM_IS_ENABLED()) {
		err = rm_gpu_device_resume(dev_node);
		if (err)
			return err;

		if (RM_FEATURE_IS_ENABLED(FW)) {
			mtgpu_util_info_reset(dev_node);
			mtgpu_watchdog_start(dev_node);
		}
	} else {
		err = mtgpu_freelist_recovery(dev_node);
		if (err) {
			MT_DPF((MT_DBG_ERROR, "freelist recovery failed (%d)",
				LinuxErrnoToMtError(err)));
			return err;
		}

		if (MTGPU_VZ_MODE_IS(HOST) && !OSDeviceIsPCI(dev))
		{
			MT_DPF((MT_DBG_WARNING, "ActiveOSid: 0x%x", dev_node->fw_info->shared_info->ActiveOSid));
			/*
			 * The shared_info->ActiveOSid is cleared in mtgpu_fw_start(),
			 * so shared_info->ActiveOSid must be saved before mtgpu_fw_start()
			 */
			reonline_osid = dev_node->fw_info->shared_info->ActiveOSid;
		}
	}

	if (!RM_FEATURE_IS_ENABLED(FW)) {
		err = OSDeviceIsPCI(dev) ? MTGPUDeviceResumeDGPU(psDeviceNode) :
					   MTGPUDeviceResumeIGPU(psDeviceNode);
		if (err)
			return err;
	}

	if (mtgpu_sched_on_host())
	{
		if (dev_node->scheduler)
		{
			int i;

			for (i = 0; i < MTFW_FWIF_QUEUE_TYPE_MAX; i++)
			{
				mtgpu_sched_start(dev_node->scheduler[i], true);
			}
		}

		if (MTGPU_VZ_MODE_IS(HOST) && !OSDeviceIsPCI(dev))
			vgpu_reonline_all_vm(dev_node->fw_info, reonline_osid);
	}

	mtgpu_ioctl_unblock_access(dev_node);

	return 0;
}

static DEFINE_MUTEX(sConnectionInitMutex);

/**************************************************************************/ /*!
@Function     MTGPUDeviceServicesOpen
@Description  Services device open.
@Input        psDeviceNode  The device node representing the device being
                            opened by a user mode process
@Input        psDRMFile     The DRM file data that backs the file handle
                            returned to the user mode process
@Return       int           0 on success and a Linux error code otherwise
*/ /***************************************************************************/
int MTGPUDeviceServicesOpen(MTGPU_DEVICE_NODE *psDeviceNode,
                             struct drm_file *psDRMFile)
{
	static DEFINE_MUTEX(sDeviceInitMutex);
	MTGPU_DATA *psMTGPUData = MTGPUGetMTGPUData();
	ENV_CONNECTION_PRIVATE_DATA sPrivData;
	MTGPU_CONNECTION_PRIV *psConnectionPriv;
	MTGPU_ERROR eError;
	int iErr = 0;

	/*
	 * The initialization of mtlink takes a long time, and it
	 * is forbidden to use mtlink before the initialization is
	 * completed. When the mtlink function is enabled, the user
	 * needs to check whether the mtlink initialization has been
	 * completed before turning on the device; when the mtlink
	 * function is not enabled, there is no need to care.
	 */
	if (mtlink_is_enabled())
	{
		if (!mtlink_is_init_finished())
		{
			MT_DPF((MT_DBG_WARNING, "%s: MT-Link initialization not completed.",
				 __func__));
			return -EBUSY;
		}
	}

#if !defined(NO_HARDWARE)
	{
		struct mtgpu_device *mtdev;
		mtdev = OSGetMtgpuDevice(OSGetPlatformDevice(psDeviceNode->psDevConfig->pvOSDevice));
		if (DEVICE_IS_PINGHU1(mtdev) && !mtgpu_d2d_check_linked(mtdev))
		{
			MT_DPF((MT_DBG_WARNING, "%s: d2d link timeout", __func__));
			return -EPERM;
		}
	}
#endif

	if (!psMTGPUData)
	{
		MT_DPF((MT_DBG_ERROR, "%s: No device data", __func__));
		iErr = -ENODEV;
		goto out;
	}

	mutex_lock(&sDeviceInitMutex);
	/*
	 * If the first attempt already set the state to bad,
	 * there is no point in going the second time, so get out
	 */
	if (psDeviceNode->eDevState == MTGPU_DEVICE_STATE_BAD)
	{
		MT_DPF((MT_DBG_ERROR, "%s: Driver already in bad state. Device open failed.",
				 __func__));
		iErr = -ENODEV;
		mutex_unlock(&sDeviceInitMutex);
		goto out;
	}

	if (psDRMFile->driver_priv == NULL)
	{
		/* Allocate psConnectionPriv (stores private data and release pfn under driver_priv) */
		psConnectionPriv = kzalloc(sizeof(*psConnectionPriv), GFP_KERNEL);
		if (!psConnectionPriv)
		{
			MT_DPF((MT_DBG_ERROR, "%s: No memory to allocate driver_priv data", __func__));
			iErr = -ENOMEM;
			mutex_unlock(&sDeviceInitMutex);
			goto fail_alloc_connection_priv;
		}
	}
	else
	{
		psConnectionPriv = (MTGPU_CONNECTION_PRIV*)psDRMFile->driver_priv;
	}

	if (psDeviceNode->eDevState == MTGPU_DEVICE_STATE_INIT)
	{
		eError = MTGPUCommonDeviceInitialise(psDeviceNode);
		if (eError != MTGPU_OK)
		{
			MT_DPF((MT_DBG_ERROR, "%s: Failed to initialise device (%s)",
					 __func__, MTGPUGetErrorString(eError)));
			iErr = -ENODEV;
			mutex_unlock(&sDeviceInitMutex);
			goto fail_device_init;
		}

#if defined(SUPPORT_MUSA)
		MTGpuTraceInitIfEnabled(psDeviceNode);
#endif
	}
	mutex_unlock(&sDeviceInitMutex);

	sPrivData.psDevNode = psDeviceNode;

	/*
	 * Here we pass the file pointer which will passed through to our
	 * OSConnectionPrivateDataInit function where we can save it so
	 * we can back reference the file structure from its connection
	 */
	mutex_lock(&sConnectionInitMutex);
	if (!psConnectionPriv->pvConnectionData)
	{
		eError = MTGPUCommonConnectionConnect(&psConnectionPriv->pvConnectionData,
						       (void *)&sPrivData);
		if (eError != MTGPU_OK)
		{
			iErr = -ENOMEM;
			mutex_unlock(&sConnectionInitMutex);
			goto fail_connect;
		}

#if (MTGPU_DEVICE_INIT_MODE == MTGPU_LINUX_DEV_INIT_ON_CONNECT)
		psConnectionPriv->pfDeviceRelease = MTGPUCommonConnectionDisconnect;
#endif
		psDRMFile->driver_priv = (void*)psConnectionPriv;
		mutex_unlock(&sConnectionInitMutex);
		goto out;
	}
	else
	{
		CONNECTION_DATA *psConnection = psConnectionPriv->pvConnectionData;
		if (OSGetCurrentProcessID() != psConnection->pid)
		{
			MT_DPF((MT_DBG_WARNING,
				 "%s: connection already initialised, old pid= %d, new pid= %d",
				 __func__, psConnection->pid, OSGetCurrentProcessID()));
		}
		psDRMFile->driver_priv = (void*)psConnectionPriv;
		mutex_unlock(&sConnectionInitMutex);
		goto out;
	}

fail_connect:
fail_device_init:
	if (!psDRMFile->driver_priv)
		kfree(psConnectionPriv);
fail_alloc_connection_priv:
out:
	return iErr;
}

/**************************************************************************/ /*!
@Function     MTGPUDeviceSyncOpen
@Description  Sync device open.
@Input        psDeviceNode  The device node representing the device being
                            opened by a user mode process
@Input        psDRMFile     The DRM file data that backs the file handle
                            returned to the user mode process
@Return       int           0 on success and a Linux error code otherwise
*/ /***************************************************************************/
static int MTGPUDeviceSyncOpen(MTGPU_DEVICE_NODE *psDeviceNode,
                                struct drm_file *psDRMFile)
{
	MTGPU_DATA *psMTGPUData = MTGPUGetMTGPUData();
	CONNECTION_DATA *psConnection = NULL;
	ENV_CONNECTION_PRIVATE_DATA sPrivData;
	MTGPU_CONNECTION_PRIV *psConnectionPriv;
	MTGPU_ERROR eError;
	int iErr = 0;

	if (!psMTGPUData)
	{
		MT_DPF((MT_DBG_ERROR, "%s: No device data", __func__));
		iErr = -ENODEV;
		goto out;
	}

	if (psDRMFile->driver_priv == NULL)
	{
		/* Allocate psConnectionPriv (stores private data and release pfn under driver_priv) */
		psConnectionPriv = kzalloc(sizeof(*psConnectionPriv), GFP_KERNEL);
		if (!psConnectionPriv)
		{
			MT_DPF((MT_DBG_ERROR, "%s: No memory to allocate driver_priv data", __func__));
			iErr = -ENOMEM;
			goto out;
		}
	}
	else
	{
		psConnectionPriv = (MTGPU_CONNECTION_PRIV*)psDRMFile->driver_priv;
	}

	mutex_lock(&sConnectionInitMutex);
	if (!psConnectionPriv->pvConnectionData)
	{
		/* Allocate connection data area, no stats since process not registered yet */
		psConnection = kzalloc(sizeof(*psConnection), GFP_KERNEL);
		if (!psConnection)
		{
			MT_DPF((MT_DBG_ERROR, "%s: No memory to allocate connection data", __func__));
			iErr = -ENOMEM;
			mutex_lock(&sConnectionInitMutex);
			goto fail_alloc_connection;
		}
#if (MTGPU_DEVICE_INIT_MODE == MTGPU_LINUX_DEV_INIT_ON_CONNECT)
		psConnectionPriv->pvConnectionData = (void*)psConnection;
#else
		psConnectionPriv->pvSyncConnectionData = (void*)psConnection;
#endif

		sPrivData.psDevNode = psDeviceNode;

		/* Call environment specific connection data init function */
		eError = OSConnectionPrivateDataInit(&psConnection->hOsPrivateData, &sPrivData);
		if (eError != MTGPU_OK)
		{
			MT_DPF((MT_DBG_ERROR, "%s: OSConnectionPrivateDataInit() failed (%s)",
				__func__, MTGPUGetErrorString(eError)));
			mutex_lock(&sConnectionInitMutex);
			goto fail_private_data_init;
		}

#if defined(SUPPORT_NATIVE_FENCE_SYNC) && !defined(USE_MTSYNC_DEVNODE)
#if (MTGPU_DEVICE_INIT_MODE == MTGPU_LINUX_DEV_INIT_ON_CONNECT)
		iErr = mt_sync_open(psConnectionPriv->pvConnectionData, psDRMFile);
#else
		iErr = mt_sync_open(psConnectionPriv->pvSyncConnectionData, psDRMFile);
#endif
		if (iErr)
		{
			MT_DPF((MT_DBG_ERROR, "%s: mt_sync_open() failed(%d)",
					__func__, iErr));
			mutex_unlock(&sConnectionInitMutex);
			goto fail_mt_sync_open;
		}
#endif

#if defined(SUPPORT_NATIVE_FENCE_SYNC) && !defined(USE_MTSYNC_DEVNODE)
#if (MTGPU_DEVICE_INIT_MODE == MTGPU_LINUX_DEV_INIT_ON_CONNECT)
		psConnectionPriv->pfDeviceRelease = MTGPUDeviceSyncRelease;
#endif
#endif
		psDRMFile->driver_priv = psConnectionPriv;
		mutex_unlock(&sConnectionInitMutex);
		goto out;
	}
	else
	{
		psConnection = psConnectionPriv->pvConnectionData;
		if (OSGetCurrentProcessID() != psConnection->pid)
		{
			MT_DPF((MT_DBG_WARNING,
				 "%s: connection already initialised, old pid= %d, new pid= %d",
				 __func__, psConnection->pid, OSGetCurrentProcessID()));
		}
		psDRMFile->driver_priv = psConnectionPriv;
		mutex_unlock(&sConnectionInitMutex);
		goto out;
	}

#if defined(SUPPORT_NATIVE_FENCE_SYNC) && !defined(USE_MTSYNC_DEVNODE)
fail_mt_sync_open:
	OSConnectionPrivateDataDeInit(psConnection->hOsPrivateData);
#endif
fail_private_data_init:
	kfree(psConnection);
fail_alloc_connection:
	kfree(psConnectionPriv);
out:
	return iErr;
}

void MTGPUDeviceSyncRelease(void *pConnectionData)
{
	CONNECTION_DATA *psConnection = pConnectionData;

	mt_sync_close(pConnectionData);

	/* Call environment specific connection data deinit function */
	if (psConnection->hOsPrivateData != NULL)
	{
		OSConnectionPrivateDataDeInit(psConnection->hOsPrivateData);
		psConnection->hOsPrivateData = NULL;
	}

	OSFreeMemNoStats(psConnection);
}

/**************************************************************************/ /*!
@Function     MTGPUDeviceRelease
@Description  Common device release.
@Input        psDeviceNode  The device node for the device that the given file
                            represents
@Input        psDRMFile     The DRM file data that's being released
@Return       void
*/ /***************************************************************************/
void MTGPUDeviceRelease(MTGPU_DEVICE_NODE *psDeviceNode,
                         struct drm_file *psDRMFile)
{
	MT_UNREFERENCED_PARAMETER(psDeviceNode);

	if (psDRMFile->driver_priv)
	{
		MTGPU_CONNECTION_PRIV *psConnectionPriv = (MTGPU_CONNECTION_PRIV*)psDRMFile->driver_priv;

		if ((psConnectionPriv->pvConnectionData) || (psConnectionPriv->pvSyncConnectionData))
		{
#if (MTGPU_DEVICE_INIT_MODE == MTGPU_LINUX_DEV_INIT_ON_CONNECT)
			if (psConnectionPriv->pfDeviceRelease)
			{
				psConnectionPriv->pfDeviceRelease(psConnectionPriv->pvConnectionData);
			}
#else
			if (psConnectionPriv->pvConnectionData)
				MTGPUCommonConnectionDisconnect(psConnectionPriv->pvConnectionData);

#if defined(SUPPORT_NATIVE_FENCE_SYNC) && !defined(USE_MTSYNC_DEVNODE)
			if (psConnectionPriv->pvSyncConnectionData)
				mt_sync_close(psConnectionPriv->pvSyncConnectionData);
#endif
#endif
		}

		kfree(psDRMFile->driver_priv);
		psDRMFile->driver_priv = NULL;
	}
}

int
drm_mt_srvkm_init(struct drm_device *dev, void *arg, struct drm_file *psDRMFile)
{
	int iErr = 0;
	struct drm_mt_srvkm_init_data *data = arg;
	struct mt_drm_private *priv = dev->dev_private;
	MTGPU_DEVICE_NODE *psDeviceNode = priv->dev_node;

#if (MTGPU_DEVICE_INIT_MODE != MTGPU_LINUX_DEV_INIT_ON_PROBE)
	MTGPU_MUSADEV_INFO *psDevInfo = psDeviceNode->pvDevice;

	if (!psDevInfo->hTQMPSharedMem)
	{
		MTGPU_ERROR eError;

		eError = MTGPUTQLoadShaders(psDeviceNode);
		if (eError != MTGPU_OK)
		{
			return OSMTGPUToNativeError(eError);
		}
	}
#endif

	switch (data->init_module)
	{
		case MT_SRVKM_SYNC_INIT:
		{
			iErr = MTGPUDeviceSyncOpen(psDeviceNode, psDRMFile);
			break;
		}
		case MT_SRVKM_SERVICES_INIT:
		{
#if (MTGPU_DEVICE_INIT_MODE != MTGPU_LINUX_DEV_INIT_ON_OPEN)
			iErr = MTGPUDeviceServicesOpen(psDeviceNode, psDRMFile);
#endif
			break;
		}
		default:
		{
			MT_DPF((MT_DBG_ERROR, "%s: invalid init_module (%d)",
			        __func__, data->init_module));
			iErr = -EINVAL;
		}
	}

	return iErr;
}

#define MTGPU_MAX_REGISTER_PHYS_HEAP_CNT	16

MTGPU_ERROR MTGPUAcquireBackupAreasFromDevice(struct drm_device *ddev,
						MEM_REGION_INFO **ppsAreas,
						MT_INT32 *psAreasCnt)
{
	MTGPU_ERROR eError;
	MT_UINT32 i, offset = 0;
	MEM_REGION_INFO *psAreas[MTGPU_MAX_REGISTER_PHYS_HEAP_CNT];
	MT_INT32 areasCnt[MTGPU_MAX_REGISTER_PHYS_HEAP_CNT];
	MT_INT32 totalAreasCnt = 0;
	MEM_REGION_INFO *totalAreas;
	struct mt_drm_private *priv = ddev->dev_private;
	MTGPU_DEVICE_NODE *psDeviceNode = priv->dev_node;

	MT_LOG_RETURN_IF_FALSE(psDeviceNode->ui32RegisteredPhysHeaps <=
				MTGPU_MAX_REGISTER_PHYS_HEAP_CNT,
				"phys head count is to big.", MTGPU_ERROR_PHYSHEAP_CONFIG);

	for (i = 0; i < psDeviceNode->ui32RegisteredPhysHeaps; i++)
	{
		acquireBackupAreasFromHeap(psDeviceNode->papsRegisteredPhysHeaps[i],
					   &psAreas[i], &areasCnt[i]);
		totalAreasCnt += areasCnt[i];
	}

	totalAreas = OSAllocZMem(totalAreasCnt * sizeof(MEM_REGION_INFO));
	MT_GOTO_IF_NOMEM(totalAreas, eError, alloc_failed);

	for (i = 0; i < psDeviceNode->ui32RegisteredPhysHeaps; i++)
	{
		OSCachedMemCopy(totalAreas + offset, psAreas[i],
				areasCnt[i] * sizeof(MEM_REGION_INFO));
		offset += areasCnt[i];
		releaseBackupAreasFromHeap(psDeviceNode->papsRegisteredPhysHeaps[i], psAreas[i]);
	}

	*ppsAreas = totalAreas;
	*psAreasCnt = totalAreasCnt;

	return MTGPU_OK;

alloc_failed:
	for (i = 0; i < psDeviceNode->ui32RegisteredPhysHeaps; i++)
		releaseBackupAreasFromHeap(psDeviceNode->papsRegisteredPhysHeaps[i], psAreas[i]);
	return eError;
}

void MTGPUReleaseBackupAreasFromDevice(struct drm_device *ddev,
					MEM_REGION_INFO *psAreas)
{
	OSFreeMem(psAreas);
}

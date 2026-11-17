/*
 * @File
 * @Title       PowerVR DRM driver
 * @Codingstyle LinuxKernel
 * @Copyright   Copyright (c) Imagination Technologies Ltd. All Rights Reserved
 * @License     Dual MIT/GPLv2
 *
 * The contents of this file are subject to the MIT license as set out below.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * Alternatively, the contents of this file may be used under the terms of
 * the GNU General Public License Version 2 ("GPL") in which case the provisions
 * of GPL are applicable instead of those above.
 *
 * If you wish to allow use of your version of this file only under the terms of
 * GPL, and not to allow others to use your version of this file under the terms
 * of the MIT license, indicate your decision by deleting the provisions above
 * and replace them with the notice and other provisions required by GPL as set
 * out in the file called "GPL-COPYING" included in this distribution. If you do
 * not delete the provisions above, a recipient may use your version of this file
 * under the terms of either the MIT license or GPL.
 *
 * This License is also included in this distribution in the file called
 * "MIT-COPYING".
 *
 * EXCEPT AS OTHERWISE STATED IN A NEGOTIATED AGREEMENT: (A) THE SOFTWARE IS
 * PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 * BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 * PURPOSE AND NONINFRINGEMENT; AND (B) IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <linux/version.h>

#include <drm/drm.h>

#if defined(OS_DRM_DRMP_H_EXIST)
#include <drm/drmP.h>
#else
#include <drm/drm_drv.h>
#include <drm/drm_file.h>
#include <drm/drm_ioctl.h>
#include <drm/drm_print.h>
#include <linux/dma-mapping.h>
#endif

#include <drm/drm_crtc.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/pm.h>
#include <linux/mutex.h>

#include "module_common.h"
#include "mt_drm.h"
#include "mt_drv.h"
#include "mt_bridge_k.h"
#include "mtversion.h"
#include "services_kernel_client.h"
#include "mt_sync_ioctl_drm.h"

#include "mtgpu_drm.h"
#include "mtgpu_vm.h"
#include "mtgpu_bo.h"
#include "mtgpu_dma_next.h"
#include "mtgpu_ioctl.h"
#include "mtgpu_semaphore.h"
#include "mtgpu_job.h"
#include "mtgpu_transport_layer.h"
#include "mtgpu_hwperf.h"
#include "mtgpu_codec.h"
#include "mtgpu_module_param.h"
#include "mtgpu_drv_common.h"
#include "mtgpu_rm.h"
#include "mtgpu_drv.h"
#include "mtgpu_fw.h"
#include "mtgpu_watchdog.h"
#include "mtgpu_util.h"
#include "mtgpu_pcie_link_monitor.h"

#include "kernel_compatibility.h"

#define MT_DRM_DRIVER_NAME MT_DRM_NAME
#define MT_DRM_DRIVER_DESC "Imagination Technologies DRM Driver"
#define	MT_DRM_DRIVER_DATE "20170530"

/*
 * Protects global MTGPU_DATA on a multi device system. i.e. this is used to
 * protect the MTGPUCommonDeviceXXXX() APIs in the Server common layer which
 * are not re-entrant for device creation and initialisation.
 */
static DEFINE_MUTEX(g_device_mutex);

static int mt_pm_suspend(struct device *dev)
{
	struct drm_device *ddev = dev_get_drvdata(dev);
	struct mt_drm_private *priv = ddev->dev_private;
	int ret;

	DRM_DEV_INFO(dev, "gpu device suspend enter\n");

	DRM_DEBUG_DRIVER("device %p\n", dev);

	ret = MTGPUDeviceSuspend(priv->dev_node);

	DRM_DEV_INFO(dev, "gpu device suspend exit\n");

	return ret;
}

static int mt_pm_resume(struct device *dev)
{
	struct drm_device *ddev = dev_get_drvdata(dev);
	struct mt_drm_private *priv = ddev->dev_private;
	int ret;

	DRM_DEV_INFO(dev, "gpu device resume enter\n");

	DRM_DEBUG_DRIVER("device %p\n", dev);

	ret = MTGPUDeviceResume(priv->dev_node);

	DRM_DEV_INFO(dev, "gpu device resume exit\n");

	return ret;
}

static int mtgpu_device_runtime_resume(struct mtgpu_device * mtdev, MTGPU_DEVICE_NODE *psDeviceNode)
{
	struct mtgpu_device_node *dev_node = (struct mtgpu_device_node *)psDeviceNode;
	int ret = 0;

	ret = mtgpu_pstate_set_level(mtdev, PSTATE_LVL_P0);
	if (ret)
		return ret;

	/* start pcie_link_monitor. */
	if (mtdev->pcie_link_monitor)
		mtgpu_pcie_link_monitor_start(mtdev);

	if (dev_node->fw_info) {
		ret = mtgpu_fw_reboot(dev_node);
		if (ret)
			return ret;
		
		ret = mtgpu_util_info_reset(dev_node);
		if (ret)
			return ret;
	}

	if (mtgpu_watchdog_get_restart(dev_node))
		mtgpu_watchdog_start(dev_node);

	return ret;
}

static int mt_pm_runtime_resume(struct device *dev)
{
	struct drm_device *ddev = dev_get_drvdata(dev);
	struct mt_drm_private *priv = ddev->dev_private;
	struct device *pci_dev = dev->parent->parent;
	struct mtgpu_device *mtdev = pci_dev->driver_data;
	int ret = 0;

	dev_dbg(dev, "runtime resume enter\n");

#if !defined(NO_HARDWARE)
	if (priv->dev_node) {
		if (!mtgpu_is_dgpu(mtdev->dev))
			MTGPUDeviceRuntimeResumeIGPU(priv->dev_node);
		else
			mtgpu_device_runtime_resume(mtdev, priv->dev_node);
	}
#endif

	dev_dbg(dev, "runtime resume exit\n");

	return ret;
}

static int mtgpu_device_runtime_suspend(struct mtgpu_device *mtdev, MTGPU_DEVICE_NODE *psDeviceNode)
{
	struct mtgpu_device_node *dev_node = (struct mtgpu_device_node *)psDeviceNode;
	int err = 0;

	mtgpu_watchdog_set_restart(dev_node);
	mtgpu_watchdog_stop(dev_node);

	if (dev_node->fw_info) {
		mtgpu_fw_stop(dev_node);
		err = mtgpu_fw_wait_idle(dev_node);
		if (err)
			goto out1;
	}

	/* Disable pcie link monitor to avoid bar2 access. */
	if (mtdev->pcie_link_monitor)
		mtgpu_pcie_link_monitor_stop(mtdev);

	err = mtgpu_pstate_set_level(mtdev, PSTATE_LVL_P12);
	if (err)
		goto out2;

	return err;

out1:
	if (mtgpu_watchdog_get_restart(dev_node))
		mtgpu_watchdog_start(dev_node);

out2:
	if (mtdev->pcie_link_monitor)
		mtgpu_pcie_link_monitor_start(mtdev);

	return err;
}

static int mt_pm_runtime_suspend(struct device *dev)
{
	struct drm_device *ddev = dev_get_drvdata(dev);
	struct mt_drm_private *priv = ddev->dev_private;
	struct device *pci_dev = dev->parent->parent;
	struct mtgpu_device *mtdev = pci_dev->driver_data;
	int ret = 0;

	dev_dbg(dev, "runtime suspend enter\n");

#if !defined(NO_HARDWARE)
	if (priv->dev_node) {
		if (!mtgpu_is_dgpu(mtdev->dev))
			MTGPUDeviceRuntimeSuspendIGPU(priv->dev_node);
		else
			mtgpu_device_runtime_suspend(mtdev, priv->dev_node);
	}
#endif

	dev_dbg(dev, "runtime suspend exit\n");

	return ret;
}

const struct dev_pm_ops mt_pm_ops = {
	.suspend = mt_pm_suspend,
	.resume  = mt_pm_resume,
	.freeze  = mt_pm_suspend,
	.thaw    = mt_pm_resume,
	.restore = mt_pm_resume,
	.runtime_resume = mt_pm_runtime_resume,
	.runtime_suspend = mt_pm_runtime_suspend,
};

#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 18, 0))
static
#endif
int mt_drm_load(struct device *dev, struct drm_device *ddev, unsigned long flags)
{
	struct mtgpu_device *mtdev = dev_get_drvdata(dev->parent->parent);
	struct mt_drm_private *priv = ddev->dev_private;
	enum MTGPU_ERROR_TAG srv_err;
	int err, deviceId;
	int delay_time;

	DRM_DEBUG_DRIVER("device %p\n", ddev->dev);

	dev_set_drvdata(dev, ddev);

#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 12, 0))
	/*
	 * Older kernels do not have render drm_minor member in drm_device,
	 * so we fallback to primary node for device identification
	 */
	deviceId = ddev->primary->index;
#else
	if (ddev->render)
		deviceId = ddev->render->index;
	else /* when render node is NULL, fallback to primary node */
		deviceId = ddev->primary->index;
#endif

	if (!ddev->dev->dma_parms)
		ddev->dev->dma_parms = &priv->dma_parms;
	dma_set_max_seg_size(ddev->dev, DMA_BIT_MASK(32));

	mutex_lock(&g_device_mutex);

	delay_time = mtgpu_is_dgpu(mtdev->dev) ? mtgpu_dgpu_rpm_delay_ms : mtgpu_igpu_rpm_delay_ms;
	mtgpu_pm_runtime_set_autosuspend(mtdev, dev, delay_time);
	mtgpu_pm_runtime_init(mtdev, dev);

	srv_err = MTGPUCommonDeviceCreate(dev, deviceId, &priv->dev_node);
	if (srv_err != MTGPU_OK) {
		DRM_ERROR("failed to create device node for device %p (%s)\n",
			  ddev->dev, MTGPUGetErrorString(srv_err));
		if (srv_err == MTGPU_ERROR_PROBE_DEFER)
			err = -EPROBE_DEFER;
		else
			err = -ENODEV;
		goto err_unset_dma_parms;
	}

	if (RM_IS_ENABLED()) {
		err = rm_gpu_device_init((struct mtgpu_device_node *)priv->dev_node);
		if (err)
			goto err_device_destroy;
	}

	err = MTGPUDeviceInit(priv->dev_node);
	if (err) {
		DRM_ERROR("device %p initialisation failed (err=%d)\n",
			  ddev->dev, err);
		goto err_device_destroy;
	}


#if (MTGPU_DEVICE_INIT_MODE == MTGPU_LINUX_DEV_INIT_ON_PROBE)
	srv_err = MTGPUCommonDeviceInitialise(priv->dev_node);
	if (srv_err != MTGPU_OK) {
		err = -ENODEV;
		DRM_ERROR("device %p initialisation failed (err=%d)\n",
			  ddev->dev, err);
		goto err_device_deinit;
	}
#endif

	os_smp_store_release(&priv->dev_node->bIsInited, 1);
	mutex_unlock(&g_device_mutex);

#if !defined(NO_HARDWARE)
	/* Decrease power counter, the GPU could be powered off. */
	mtgpu_pm_runtime_put_async(dev);
#endif

	return 0;

#if (MTGPU_DEVICE_INIT_MODE == MTGPU_LINUX_DEV_INIT_ON_PROBE)
err_device_deinit:
	drm_mode_config_cleanup(ddev);
	MTGPUDeviceDeinit(priv->dev_node);
#endif
err_device_destroy:
	MTGPUCommonDeviceDestroy(priv->dev_node);
err_unset_dma_parms:
	mutex_unlock(&g_device_mutex);
	if (ddev->dev->dma_parms == &priv->dma_parms)
		ddev->dev->dma_parms = NULL;

	dev_set_drvdata(dev, NULL);

	return err;
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 18, 0))
static
#endif
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 11, 0))
int mt_drm_unload(struct drm_device *ddev)
#else
void mt_drm_unload(struct drm_device *ddev)
#endif
{
	struct mt_drm_private *priv = ddev->dev_private;
	struct device *dev = priv->dev_node->psDevConfig->pvOSDevice;

	DRM_DEBUG_DRIVER("device %p\n", ddev->dev);

#if !defined(NO_HARDWARE)
	mtgpu_pm_runtime_get(dev);
#endif

	MTGPUDeviceDeinit(priv->dev_node);

	mutex_lock(&g_device_mutex);
	if (RM_IS_ENABLED())
		rm_gpu_device_deinit((struct mtgpu_device_node *)priv->dev_node);

	MTGPUCommonDeviceDestroy(priv->dev_node);
	mutex_unlock(&g_device_mutex);

#if !defined(NO_HARDWARE)
	mtgpu_pm_runtime_put(dev);
	mtgpu_pm_runtime_exit(dev);
#endif

	if (ddev->dev->dma_parms == &priv->dma_parms)
		ddev->dev->dma_parms = NULL;

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 11, 0))
	return 0;
#endif
}

int mt_drm_open(struct drm_device *ddev, struct drm_file *dfile)
{
#if (MTGPU_DEVICE_INIT_MODE == MTGPU_LINUX_DEV_INIT_ON_OPEN)
	struct mt_drm_private *priv = ddev->dev_private;

	if (priv)
		return MTGPUDeviceServicesOpen(priv->dev_node, dfile);
#else
	return 0;
#endif
}

void mt_drm_release(struct drm_device *ddev, struct drm_file *dfile)
{
	struct mt_drm_private *priv = ddev->dev_private;

	if (priv)
		MTGPUDeviceRelease(priv->dev_node, dfile);
}

struct drm_ioctl_desc mt_drm_ioctls[128] = {
	DRM_IOCTL_DEF_DRV(MT_SRVKM_CMD, MTGPU_BridgeDispatchKM,
			  DRM_RENDER_ALLOW),
	DRM_IOCTL_DEF_DRV(MT_SRVKM_INIT, drm_mt_srvkm_init,
			  DRM_RENDER_ALLOW),
#if defined(SUPPORT_NATIVE_FENCE_SYNC) && !defined(USE_MTSYNC_DEVNODE)
	DRM_IOCTL_DEF_DRV(MT_SYNC_RENAME_CMD, mt_sync_rename_ioctl,
			  DRM_RENDER_ALLOW),
	DRM_IOCTL_DEF_DRV(MT_SYNC_FORCE_SW_ONLY_CMD, mt_sync_force_sw_only_ioctl,
			  DRM_RENDER_ALLOW),
	DRM_IOCTL_DEF_DRV(MT_SW_SYNC_CREATE_FENCE_CMD, mt_sw_sync_create_fence_ioctl,
			  DRM_RENDER_ALLOW),
	DRM_IOCTL_DEF_DRV(MT_SW_SYNC_INC_CMD, mt_sw_sync_inc_ioctl,
			  DRM_RENDER_ALLOW),
#endif
#if !defined(NO_HARDWARE)
	DRM_IOCTL_DEF_DRV(MTGPU_CMD, mtgpu_ioctl,
			  DRM_RENDER_ALLOW),
#endif
};

#if defined(CONFIG_COMPAT)
static long mt_compat_ioctl(struct file *file, unsigned int cmd,
			     unsigned long arg)
{
	unsigned int nr = DRM_IOCTL_NR(cmd);

	if (nr < DRM_COMMAND_BASE)
		return drm_compat_ioctl(file, cmd, arg);

	return drm_ioctl(file, cmd, arg);
}
#endif /* defined(CONFIG_COMPAT) */

const struct file_operations mt_drm_fops = {
	.owner			= THIS_MODULE,
	.open			= drm_open,
	.release		= drm_release,
	.unlocked_ioctl		= drm_ioctl,
#if defined(CONFIG_COMPAT)
	.compat_ioctl		= mt_compat_ioctl,
#endif
	.mmap			= MTGPU_MMap,
	.poll			= drm_poll,
	.read			= drm_read,
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 12, 0))
	.fasync			= drm_fasync,
#endif
};

struct drm_driver mt_drm_generic_driver = {
	.driver_features	= DRIVER_RENDER,

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 18, 0))
	.load			= NULL,
	.unload			= NULL,
#else
	.load			= mt_drm_load,
	.unload			= mt_drm_unload,
#endif
	.open			= mt_drm_open,
	.postclose		= mt_drm_release,

	.ioctls			= mt_drm_ioctls,
	.num_ioctls		= ARRAY_SIZE(mt_drm_ioctls),
	.fops			= &mt_drm_fops,

	.name			= MT_DRM_DRIVER_NAME,
	.desc			= MT_DRM_DRIVER_DESC,
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 14, 0)
	.date			= MT_DRM_DRIVER_DATE,
#endif
	.major			= MTVERSION_MAJ,
	.minor			= MTVERSION_MIN,
	.patchlevel		= MTVERSION_BUILD,
};

/*
 * @File
 * @Title       PowerVR DRM platform driver
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

#include <linux/component.h>
#include <linux/version.h>

#if defined(OS_DRM_DRMP_H_EXIST)
#include <drm/drmP.h>
#else
#include <drm/drm_drv.h>
#include <drm/drm_ioctl.h>
#include <drm/drm_print.h>
#include <linux/mod_devicetable.h>
#include <linux/dma-mapping.h>
#include <linux/of.h>
#include <linux/slab.h>
#endif

#include <linux/module.h>
#include <linux/platform_device.h>

#include "module_common.h"
#include "mt_drv.h"
#include "mtmodule.h"
#include "sysinfo.h"

/* This header must always be included last */
#include "kernel_compatibility.h"

static int mt_component_bind(struct device *dev,
			      struct device *master, void *data)
{
	struct drm_device *drm = data;
	int err;

	DRM_DEBUG_DRIVER("\n");
	err = mt_drm_load(dev, drm, 0);

	return err;
}

static void mt_component_unbind(struct device *dev,
				 struct device *master, void *data)
{
	struct drm_device *drm = data;

	DRM_DEBUG_DRIVER("\n");
	mt_drm_unload(drm);
}

static const struct component_ops mt_component_ops = {
	.bind   = mt_component_bind,
	.unbind = mt_component_unbind,
};

static int mt_probe(struct platform_device *pdev)
{
	return component_add(&pdev->dev, &mt_component_ops);
}

static int mt_remove(struct platform_device *pdev)
{
	component_del(&pdev->dev, &mt_component_ops);
	return 0;
}

static void mt_shutdown(struct platform_device *pdev)
{
	struct drm_device *ddev;
	struct mt_drm_private *priv;

	DRM_DEBUG_DRIVER("device %p\n", &pdev->dev);

	ddev = platform_get_drvdata(pdev);
	if (!ddev)
		return;

	priv = ddev->dev_private;
	MTGPUDeviceShutdown(priv->dev_node);
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 9, 0))
static const struct of_device_id mt_of_ids[] = {
#if defined(SYS_MUSA_OF_COMPATIBLE)
	{ .compatible = SYS_MUSA_OF_COMPATIBLE, },
#endif
	{},
};

#if !defined(CHROMIUMOS_KERNEL) || !defined(MODULE)
MODULE_DEVICE_TABLE(of, mt_of_ids);
#endif
#endif

static struct platform_device_id mt_platform_ids[] = {
#if defined(SYS_MUSA_DEV_NAME)
	{ SYS_MUSA_DEV_NAME, 0 },
#endif
	{ }
};

#if !defined(CHROMIUMOS_KERNEL) || !defined(MODULE)
MODULE_DEVICE_TABLE(platform, mt_platform_ids);
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 11, 0)
static void mt_remove_void(struct platform_device *pdev)
{
	mt_remove(pdev);
}
#endif

struct platform_driver mt_platform_driver = {
	.driver = {
		.name		= DRVNAME,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 9, 0))
		.of_match_table	= of_match_ptr(mt_of_ids),
#endif
		.pm		= &mt_pm_ops,
	},
	.id_table		= mt_platform_ids,
	.probe			= mt_probe,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 11, 0)
	.remove			= mt_remove_void,
#else
	.remove			= mt_remove,
#endif
	.shutdown		= mt_shutdown,
};

int mt_drv_init(void)
{
	DRM_DEBUG_DRIVER("\n");

	return MTGPUDriverInit();
}

void mt_drv_exit(void)
{
	DRM_DEBUG_DRIVER("\n");

	MTGPUDriverDeinit();
}

#if defined(NO_HARDWARE)

/*
 * This is an arbitrary value. If it's changed then the 'num_devices' module
 * parameter description should also be updated to match.
 */
#define MAX_DEVICES 16

static unsigned int mt_num_devices = 1;
static struct platform_device **mt_devices;

static int mt_num_devices_set(const char *val,
			       const struct kernel_param *param)
{
	int err;

	err = param_set_uint(val, param);
	if (err)
		return err;

	if (mt_num_devices == 0 || mt_num_devices > MAX_DEVICES)
		return -EINVAL;

	return 0;
}

static const struct kernel_param_ops mt_num_devices_ops = {
	.set = mt_num_devices_set,
	.get = param_get_uint,
};

module_param_cb(num_devices, &mt_num_devices_ops, &mt_num_devices, 0444);
MODULE_PARM_DESC(num_devices,
		 "Number of platform devices to register (default: 1 - max: 16)");

static int mt_devices_register(void)
{
	unsigned int i;
	struct platform_device_info mt_dev_info = {
		.name = SYS_MUSA_DEV_NAME,
		.id = -2,
		/* Not all cores have 40 bit physical support, but this
		 * will work unless > 32 bit address is returned on those cores.
		 * In the future this will be fixed more correctly.
		 */
		.dma_mask = DMA_BIT_MASK(40),
	};

	WARN_ON(mt_num_devices == 0 || mt_num_devices > MAX_DEVICES);

	mt_devices = kmalloc_array(mt_num_devices, sizeof(*mt_devices),
				    GFP_KERNEL);
	if (!mt_devices)
		return -ENOMEM;

	for (i = 0; i < mt_num_devices; i++) {
		mt_devices[i] = platform_device_register_full(&mt_dev_info);
		if (IS_ERR(mt_devices[i])) {
			DRM_ERROR("unable to register device %u (err=%ld)\n",
				  i, PTR_ERR(mt_devices[i]));
			mt_devices[i] = NULL;
			return -ENODEV;
		}
	}

	return 0;
}

static void mt_devices_unregister(void)
{
	unsigned int i;

	WARN_ON(!mt_devices);

	for (i = 0; i < mt_num_devices && mt_devices[i]; i++)
		platform_device_unregister(mt_devices[i]);

	kfree(mt_devices);
	mt_devices = NULL;
}

static int mt_nohw_probe(struct platform_device *pdev)
{
	struct drm_device *ddev;
	struct mt_drm_private *priv;
	int ret;

	DRM_DEBUG_DRIVER("device %p\n", &pdev->dev);

	priv = kzalloc(sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	ddev = drm_dev_alloc(&mt_drm_generic_driver, &pdev->dev);
	if (IS_ERR(ddev)) {
		ret = PTR_ERR(ddev);
		goto err_free;
	}
	ddev->dev_private = priv;

	ret = mt_drm_load(&pdev->dev, ddev, 0);
	if (ret)
		goto err_drm_dev_put;

	ret = drm_dev_register(ddev, 0);
	if (ret)
		goto err_drm_dev_unload;

	return 0;

err_drm_dev_unload:
	mt_drm_unload(ddev);
err_drm_dev_put:
	drm_dev_put(ddev);
err_free:
	kfree(priv);
	return ret;
}

static int mt_nohw_remove(struct platform_device *pdev)
{
	struct drm_device *ddev = platform_get_drvdata(pdev);

	DRM_DEBUG_DRIVER("device %p\n", &pdev->dev);

	drm_dev_unregister(ddev);

	mt_drm_unload(ddev);

	drm_dev_put(ddev);

	kfree(ddev->dev_private);

	return 0;
}

static int __init mt_nohw_init(void)
{
	int err;

	DRM_DEBUG_DRIVER("\n");

	err = MTGPUDriverInit();
	if (err)
		return err;

	mt_platform_driver.probe = mt_nohw_probe;
	mt_platform_driver.remove = mt_nohw_remove;
	err = platform_driver_register(&mt_platform_driver);
	if (err)
		return err;

	return mt_devices_register();
}

static void __exit mt_nohw_exit(void)
{
	DRM_DEBUG_DRIVER("\n");

	mt_devices_unregister();
	platform_driver_unregister(&mt_platform_driver);
	MTGPUDriverDeinit();

	DRM_DEBUG_DRIVER("done\n");
}

late_initcall(mt_nohw_init);
module_exit(mt_nohw_exit);

MODULE_AUTHOR("MooreThreads Corporation");
MODULE_LICENSE("Dual MIT/GPL");

#endif

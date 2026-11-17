/*
 * @Copyright   Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License     Dual MIT/GPLv2
 */

#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/mod_devicetable.h>
#include <linux/component.h>
#include <drm/drm_print.h>

#include "mtgpu_drv.h"

#include "linux_dummy_drv.h"

static int mtdisp_dummy_component_bind(struct device *dev, struct device *master, void *data)
{
	struct drm_device *drm = data;
	struct mtdisp_dummy_platform_data *pdata = dev_get_platdata(dev);

	DRM_DEV_INFO(dev, "%s()\n", __func__);

	return mtdisp_dummy_init(dev, drm, pdata);
}

static void mtdisp_dummy_component_unbind(struct device *dev, struct device *master, void *data)
{
	struct drm_device *drm = data;
	struct mtdisp_dummy_platform_data *pdata = dev_get_platdata(dev);

	DRM_DEV_INFO(dev, "%s()\n", __func__);

	mtdisp_dummy_deinit(dev, drm, pdata);
}

static const struct component_ops mtdisp_dummy_component_ops = {
	.bind   = mtdisp_dummy_component_bind,
	.unbind = mtdisp_dummy_component_unbind,
};

static int mtdisp_dummy_driver_probe(struct platform_device *pdev)
{
	return component_add(&pdev->dev, &mtdisp_dummy_component_ops);
}

static int mtdisp_dummy_driver_remove(struct platform_device *pdev)
{
	component_del(&pdev->dev, &mtdisp_dummy_component_ops);

	return 0;
}

static struct platform_device_id mtdisp_dummy_device_id[] = {
	{ .name = "mtdisp-dummy", },
	{ },
};
MODULE_DEVICE_TABLE(platform, mtdisp_dummy_device_id);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 11, 0)
static void mtdisp_dummy_driver_remove_void(struct platform_device *pdev)
{
	mtdisp_dummy_driver_remove(pdev);
}
#endif

struct platform_driver mtdisp_dummy_driver = {
	.probe		= mtdisp_dummy_driver_probe,
	#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 11, 0)
	.remove   = mtdisp_dummy_driver_remove_void,
#else
	.remove		= mtdisp_dummy_driver_remove,
#endif
	.driver		= {
		.owner  = THIS_MODULE,
		.name	= "mtdisp-dummy-driver",
	},
	.id_table	= mtdisp_dummy_device_id,
};

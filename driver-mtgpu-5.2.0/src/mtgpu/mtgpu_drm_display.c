/*
 * @Copyright   Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License     Dual MIT/GPLv2
 */

#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/mod_devicetable.h>
#include <linux/component.h>

#include "mtgpu_drv.h"

#include "linux_display_drv.h"

static int mtdisp_display_component_bind(struct device *dev,
					 struct device *master, void *data)
{
	struct drm_device *drm = data;
	struct mtdisp_display_platform_data *pdata = dev_get_platdata(dev);

	pr_info("%s()\n", __func__);

	return mtdisp_display_init(dev, drm, pdata);
}

static void mtdisp_display_component_unbind(struct device *dev,
					    struct device *master, void *data)
{
	struct drm_device *drm = data;
	struct mtdisp_display_platform_data *pdata = dev_get_platdata(dev);

	pr_info("%s()\n", __func__);

	mtdisp_display_deinit(dev, drm, pdata);
}

static const struct component_ops mtdisp_display_component_ops = {
	.bind   = mtdisp_display_component_bind,
	.unbind = mtdisp_display_component_unbind,
};

static int mtdisp_display_driver_probe(struct platform_device *pdev)
{
	return component_add(&pdev->dev, &mtdisp_display_component_ops);
}

static int mtdisp_display_driver_remove(struct platform_device *pdev)
{
	int err = 0;

	component_del(&pdev->dev, &mtdisp_display_component_ops);

	return err;
}

static void mtdisp_display_driver_shutdown(struct platform_device *pdev)
{
}

static int mtdisp_display_driver_resume_early(struct device *dev)
{
	mtdisp_display_resume_early(dev);

	return 0;
}

static int mtdisp_display_driver_suspend(struct device *dev)
{
	mtdisp_display_suspend(dev);

	return 0;
}

static void mtdisp_display_driver_complete(struct device *dev)
{
	mtdisp_display_complete(dev);
}

const struct dev_pm_ops mtdisp_display_driver_pm_ops = {
	/* this will be called before mtgpu_drm_resume */
	.resume_early	= mtdisp_display_driver_resume_early,
	.restore_early	= mtdisp_display_driver_resume_early,
	.suspend	= mtdisp_display_driver_suspend,
	.freeze		= mtdisp_display_driver_suspend,
	/* this will be called after mtgpu_drm_resume */
	.complete = mtdisp_display_driver_complete,
};

static struct platform_device_id mtdisp_device_id[] = {
	{ .name = "mtdisp-display", },
	{ },
};
MODULE_DEVICE_TABLE(platform, mtdisp_device_id);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 11, 0)
static void mtdisp_display_driver_remove_void(struct platform_device *pdev)
{
	mtdisp_display_driver_remove(pdev);
}
#endif

struct platform_driver mtdisp_display_driver = {
	.probe		= mtdisp_display_driver_probe,
	#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 11, 0)
	.remove   = mtdisp_display_driver_remove_void,
#else
	.remove		= mtdisp_display_driver_remove,
#endif
	.shutdown	= mtdisp_display_driver_shutdown,
	.driver		= {
		.owner  = THIS_MODULE,
		.name	= "mtdisp-display-driver",
		.pm	= &mtdisp_display_driver_pm_ops,
	},
	.id_table	= mtdisp_device_id,
};

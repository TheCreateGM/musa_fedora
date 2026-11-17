/*
 * @Copyright   Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License     Dual MIT/GPLv2
 */

#include <linux/slab.h>
#if defined(OS_DRM_DRMP_H_EXIST)
#include <drm/drmP.h>
#endif
#include <drm/drm_print.h>

#include "mtgpu_drv.h"
#include "common_int_defs.h"

#include "linux_dummy_crtc_drv.h"
#include "linux_dummy_connector_drv.h"
#include "linux_dummy_drv.h"

static struct mtdisp_dummy *
mtdisp_dummy_data_init(struct device *dev, struct mtdisp_dummy_platform_data *pdata)
{
	int i;
	struct mtdisp_dummy *dummy;

	DRM_DEV_INFO(dev, "%s()\n", __func__);

	dummy = kzalloc(sizeof(*dummy), GFP_KERNEL);
	if (!dummy) {
		DRM_DEV_ERROR(dev, "failed to create dummy\n");
		return NULL;
	}

	dummy->dev = dev;
	dev_set_drvdata(dev, dummy);

	if (!pdata || !pdata->vdisplay_num) {
		DRM_DEV_ERROR(dev, "invalid dummy platform data\n");
		goto err_invalid_pdata;
	}

	dummy->crtc_count = pdata->vdisplay_num;
	dummy->conn_count = pdata->vdisplay_num;

	dummy->crtc = kzalloc(sizeof(*dummy->crtc) * dummy->crtc_count, GFP_KERNEL);
	if (!dummy->crtc) {
		DRM_DEV_ERROR(dev, "failed to create dummy crtc\n");
		goto err_create_crtc;
	}

	dummy->conn = kzalloc(sizeof(*dummy->conn) * dummy->conn_count, GFP_KERNEL);
	if (!dummy->conn) {
		DRM_DEV_ERROR(dev, "failed to create dummy conn\n");
		goto err_create_conn;
	};

	for (i = 0; i < dummy->crtc_count; i++) {
		dummy->crtc[i].id = i;
		dummy->crtc[i].irq = MTGPU_INTERRUPT_DISPC0 + dummy->crtc[i].id;
		dummy->crtc[i].max_hres = pdata->max_hres;
		dummy->crtc[i].max_vres = pdata->max_vres;
	}

	for (i = 0; i < dummy->conn_count; i++) {
		dummy->conn[i].id = i;
		dummy->conn[i].max_hres = pdata->max_hres;
		dummy->conn[i].max_vres = pdata->max_vres;
	}

	DRM_DEV_INFO(dev, "dummy data init successfully\n");

	return dummy;

err_create_conn:
	kfree(dummy->crtc);
err_create_crtc:
err_invalid_pdata:
	dev_set_drvdata(dev, NULL);
	kfree(dummy);

	DRM_DEV_INFO(dev, "Failed to init dummy data\n");

	return NULL;
}

static void mtdisp_dummy_data_deinit(struct mtdisp_dummy *dummy,
				     struct device *dev)
{
	DRM_DEV_INFO(dev, "%s()\n", __func__);

	if (!dummy)
		return;

	if (dummy->conn) {
		kfree(dummy->conn);
		dummy->conn = NULL;
	}

	if (dummy->crtc) {
		kfree(dummy->crtc);
		dummy->crtc = NULL;
	}

	kfree(dummy);
	dev_set_drvdata(dev, NULL);

	DRM_DEV_INFO(dev, "dummy data deinit successfully\n");
}

int mtdisp_dummy_init(struct device *dev, struct drm_device *drm,
		      struct mtdisp_dummy_platform_data *pdata)
{
	int ret;
	struct mtdisp_dummy *dummy;

	DRM_DEV_INFO(dev, "%s()\n", __func__);

	dummy = mtdisp_dummy_data_init(dev, pdata);
	if (!dummy) {
		ret = -EINVAL;
		goto err_dummy_init;
	}

	ret = mtdisp_dummy_crtc_probe(dummy, dev, drm);
	if (ret) {
		DRM_DEV_ERROR(dev, "failed to register dummy crtc\n");
		goto err_dummy_crtc_probe;
	}

	ret = mtdisp_dummy_connector_probe(dummy, dev, drm);
	if (ret) {
		DRM_DEV_ERROR(dev, "failed to register connector\n");
		goto err_dummy_conn_probe;
	}

	DRM_DEV_INFO(dev, "mtdisp dummy init successfully\n");

	return 0;

err_dummy_conn_probe:
	mtdisp_dummy_crtc_remove(dummy, dev);
err_dummy_crtc_probe:
	if (dummy)
		mtdisp_dummy_data_deinit(dummy, dev);
err_dummy_init:
	DRM_DEV_ERROR(dev, "Failed to dummy init\n");

	return ret;
}

void mtdisp_dummy_deinit(struct device *dev, struct drm_device *drm,
			 struct mtdisp_dummy_platform_data *pdata)
{
	struct mtdisp_dummy *dummy = dev_get_drvdata(dev);

	DRM_DEV_INFO(dev, "%s()\n", __func__);

	if (!dummy)
		return;

	mtdisp_dummy_connector_remove(dummy, dev);
	mtdisp_dummy_crtc_remove(dummy, dev);

	mtdisp_dummy_data_deinit(dummy, dev);

	DRM_DEV_INFO(dev, "dummy deinit successfully");
}

/*
 * @Copyright   Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License     Dual MIT/GPLv2
 */

#include <linux/printk.h>
#include <linux/device.h>
#include <linux/slab.h>
#if defined(OS_DRM_DRMP_H_EXIST)
#include <drm/drmP.h>
#endif
#include <drm/drm_device.h>
#include <drm/drm_drv.h>
#include <drm/drm_plane.h>

#include "mtgpu_drv.h"

#include "dispc_common.h"
#include "encoder_common.h"

#include "dm_hwlib_access.h"
#include "dm_hwlib_common.h"

#include "linux_display_func.h"
#include "linux_data_convert.h"
#include "linux_dispc_drv.h"
#include "linux_encoder_drv.h"
#include "linux_display_drv.h"

static int mtdisp_dispc_layer_caps_init(struct mtdisp_dispc *dispc)
{
	int i, ret = 0;
	struct mtdisp_layer_capability *layer_caps = NULL;
	struct mtdisp_dispc_capability *hwlib_dispc_caps = mtdisp_get_dispc_caps(dispc->hwlib_dispc);
	struct mtdisp_layer_capability *hwlib_layer_caps = hwlib_dispc_caps->layer_caps;
	u32 layer_count = hwlib_dispc_caps->layer_count;

	layer_caps = kzalloc(sizeof(struct mtdisp_layer_capability) * layer_count, GFP_KERNEL);
	if (!layer_caps) {
		pr_err("%s() failed to create display dispc layer caps\n", __func__);
		return -ENOMEM;
	}

	for (i = 0; i < layer_count; i++) {
		layer_caps[i].fmts_ptr = kzalloc(sizeof(u32) * hwlib_layer_caps[i].fmts_cnt,
						 GFP_KERNEL);
		if (!layer_caps[i].fmts_ptr) {
			pr_err("%s() failed to create display dispc layer caps fmts_ptr\n",
			       __func__);
			ret = -ENOMEM;
			goto err_layer_caps;
		}

		layer_caps[i].modifier = kzalloc(sizeof(u64) * hwlib_layer_caps[i].modifier_cnt,
						 GFP_KERNEL);
		if (!layer_caps[i].modifier) {
			pr_err("%s() failed to create display dispc layer caps modifier\n",
			       __func__);
			ret = -ENOMEM;
			goto err_layer_caps;
		}
	}

	mtdisp_layer_caps_from_hwlib(layer_caps, hwlib_layer_caps, layer_count,
				     hwlib_dispc_caps->support_mpo_num);

	dispc->layer_count = layer_count;
	dispc->layer_caps  = layer_caps;

	return 0;

err_layer_caps:
	for (i = 0; i < layer_count; i++) {
		if (layer_caps[i].fmts_ptr)
			kfree(layer_caps[i].fmts_ptr);
		if (layer_caps[i].modifier)
			kfree(layer_caps[i].modifier);
	}
	kfree(layer_caps);

	return ret;
}

static void mtdisp_dispc_layer_caps_deinit(struct mtdisp_dispc *dispc)
{
	int i;
	struct mtdisp_layer_capability *layer_caps = dispc->layer_caps;

	for (i = 0; i < dispc->layer_count; i++) {
		if (layer_caps[i].fmts_ptr) {
			kfree(layer_caps[i].fmts_ptr);
			layer_caps[i].fmts_ptr = NULL;
		}

		if (layer_caps[i].modifier) {
			kfree(layer_caps[i].modifier);
			layer_caps[i].modifier = NULL;
		}
	}

	kfree(layer_caps);
	layer_caps = NULL;
}

static struct mtdisp_display *mtdisp_display_data_init(struct device *dev,
						       struct mtdisp_display_platform_data *pdata)
{
	int i, ret;
	int type;
	void *hwlib_dispc, *hwlib_enc, *hwlib_dp, *hwlib_hdmi;
	struct mtdisp_display *display;

	pr_info("%s() entry...\n", __func__);

	display = kzalloc(sizeof(*display), GFP_KERNEL);
	if (!display) {
		pr_err("%s() failed to create display\n", __func__);
		return NULL;
	}

	display->dev = dev;
	dev_set_drvdata(dev, display);

	display->hwlib = dm_hwlib_init(pdata->gen_id, pdata->sys_bar_addr);
	if (!display->hwlib) {
		pr_err("%s() failed to init hwlib\n", __func__);
		goto err_hwlib_init;
	}

	display->dispc_count  = mtdisp_get_hwlib_dispc_count(display->hwlib);
	display->output_count = mtdisp_get_hwlib_output_count(display->hwlib);

	display->dispc = kzalloc(sizeof(*display->dispc) * display->dispc_count, GFP_KERNEL);
	if (!display->dispc) {
		pr_err("%s() failed to create display dispc\n", __func__);
		goto err_create_dispc;
	}

	display->encoder = kzalloc(sizeof(*display->encoder) * display->output_count, GFP_KERNEL);
	if (!display->encoder) {
		pr_err("%s() failed to create display encoder\n", __func__);
		goto err_create_encoder;
	};

	for (i = 0; i < display->dispc_count; i++) {
		u64 cursor_mem_base = pdata->cursor_mem_base + pdata->cursor_mem_size * i * 2;
		hwlib_dispc = mtdisp_get_hwlib_dispc(display->hwlib, i);
		display->dispc[i].id = mtdisp_get_dispc_ctx_id(hwlib_dispc);
		display->dispc[i].irq = mtdisp_get_dispc_ctx_irq(hwlib_dispc);
		mtdisp_set_dispc_ctx_pcie_mem_base(hwlib_dispc, pdata->pcie_mem_base);
		mtdisp_set_dispc_ctx_cursor_mem_base(hwlib_dispc, cursor_mem_base);
		mtdisp_set_dispc_ctx_cursor_mem_size(hwlib_dispc, (u32)pdata->cursor_mem_size);
		mtdisp_set_dispc_ctx_cursor_buffer_index(hwlib_dispc, i);
		display->dispc[i].hwlib_dispc = hwlib_dispc;
		ret = mtdisp_dispc_layer_caps_init(&display->dispc[i]);
		if (ret < 0) {
			pr_err("%s() failed to init layer caps, i: %d\n", __func__, i);
			goto err_init_layer_caps;
		}
	}

	for (i = 0; i < display->output_count; i++) {
		hwlib_enc = mtdisp_get_hwlib_encoder(display->hwlib, i);
		type = mtdisp_get_encoder_type(hwlib_enc);

		display->encoder[i].type = type;

		switch (type) {
		case MTDISP_ENCODER_DP:
			hwlib_dp = mtdisp_get_encoder_dp(hwlib_enc);
			display->encoder[i].dp.id = mtdisp_get_dp_ctx_id(hwlib_dp);
			display->encoder[i].dp.irq = mtdisp_get_dp_ctx_irq(hwlib_dp);
			display->encoder[i].dp.port_type = pdata->dp_port_type[i];
			display->encoder[i].dp.hwlib_dp = hwlib_dp;
			break;
		case MTDISP_ENCODER_DP_MST:
			hwlib_dp = mtdisp_get_encoder_dp(hwlib_enc);
			display->encoder[i].dp.id = mtdisp_get_dp_ctx_id(hwlib_dp);
			display->encoder[i].dp.irq = mtdisp_get_dp_ctx_irq(hwlib_dp);
			display->encoder[i].dp.port_type = MTDISP_PORT_TYPE_DP;
			display->encoder[i].dp.hwlib_dp = hwlib_dp;
			break;
		case MTDISP_ENCODER_HDMI:
			hwlib_hdmi = mtdisp_get_encoder_hdmi(hwlib_enc);
			display->encoder[i].hdmi.id = mtdisp_get_hdmi_ctx_id(hwlib_hdmi);
			display->encoder[i].hdmi.irq = mtdisp_get_hdmi_ctx_irq(hwlib_hdmi);
			display->encoder[i].hdmi.port_type = pdata->hdmi_port_type;
			display->encoder[i].hdmi.hwlib_hdmi = hwlib_hdmi;
			break;
		}
	}

	pr_info("%s() display data init successfully\n", __func__);

	return display;

err_init_layer_caps:
	kfree(display->encoder);
err_create_encoder:
	kfree(display->dispc);
err_create_dispc:
	dm_hwlib_deinit(pdata->gen_id, display->hwlib);
err_hwlib_init:
	kfree(display);

	pr_err("%s() Failed to init display data\n", __func__);

	return NULL;
}

static void mtdisp_display_data_deinit(struct mtdisp_display *display,
				       struct mtdisp_display_platform_data *pdata)
{
	int i;

	pr_info("%s()\n", __func__);

	if (!display)
		return;

	for (i = 0; i < display->dispc_count; i++)
		mtdisp_dispc_layer_caps_deinit(&display->dispc[i]);

	if (display->encoder) {
		kfree(display->encoder);
		display->encoder = NULL;
	}

	if (display->dispc) {
		kfree(display->dispc);
		display->dispc = NULL;
	}

	dm_hwlib_deinit(pdata->gen_id, display->hwlib);

	kfree(display);
	display = NULL;

	pr_info("%s() display data deinit successfully\n", __func__);
}

int mtdisp_display_init(struct device *dev, struct drm_device *drm,
			struct mtdisp_display_platform_data *pdata)
{
	int ret;
	struct mtdisp_display *display;

	pr_info("%s()\n", __func__);

	linux_display_func_tbl_init();

	display = mtdisp_display_data_init(dev, pdata);
	if (!display) {
		pr_err("%s() failed to init display\n", __func__);
		ret = -EINVAL;
		goto err_display_init;
	}

	ret = mtdisp_dispc_probe(display, dev, drm);
	if (ret) {
		pr_err("%s() failed to register dispc\n", __func__);
		goto err_dispc_probe;
	}

	ret = mtdisp_encoder_probe(display, dev, drm);
	if (ret) {
		pr_err("%s() failed to register encoder\n", __func__);
		goto err_encoder_probe;
	}

	pr_info("%s() display init successfully\n", __func__);

	return 0;

err_encoder_probe:
	mtdisp_dispc_remove(display, dev);
err_dispc_probe:
	if (display)
		mtdisp_display_data_deinit(display, pdata);
err_display_init:
	linux_display_func_tbl_deinit();

	pr_err("%s() Failed to display init\n", __func__);

	return ret;
}

void mtdisp_display_deinit(struct device *dev, struct drm_device *drm,
			   struct mtdisp_display_platform_data *pdata)
{
	struct mtdisp_display *display = dev_get_drvdata(dev);

	pr_info("%s()\n", __func__);

	mtdisp_encoder_remove(display, dev);
	mtdisp_dispc_remove(display, dev);

	pr_info("%s() encoder & dispc remove done\n", __func__);

	if (display)
		mtdisp_display_data_deinit(display, pdata);

	linux_display_func_tbl_deinit();

	pr_info("%s() display deinit successfully\n", __func__);
}

int mtdisp_display_resume_early(struct device *dev)
{
	struct mtdisp_display *display = dev_get_drvdata(dev);

	pr_info("%s()\n", __func__);

	mtdisp_encoder_resume_early(display, dev);

	return 0;
}

int mtdisp_display_suspend(struct device *dev)
{
	struct mtdisp_display *display = dev_get_drvdata(dev);

	pr_info("%s()\n", __func__);

	mtdisp_encoder_suspend(display, dev);

	return 0;
}

void mtdisp_display_complete(struct device *dev)
{
	struct mtdisp_display *display = dev_get_drvdata(dev);

	pr_info("%s()\n", __func__);

	mtdisp_encoder_complete(display, dev);

}

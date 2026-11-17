/*
 * @Copyright   Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License     Dual MIT/GPLv2
 */

#include <linux/printk.h>

#include "encoder_common.h"

#include "dm_hwlib_access.h"

#include "linux_data_convert.h"
#include "linux_encoder_drv.h"
#include "linux_display_drv.h"

int mtdisp_encoder_probe(struct mtdisp_display *display,
			 struct device *dev, struct drm_device *drm)
{
	int i, ret = 0;
	struct mtdisp_dp *dp;
	struct mtdisp_hdmi *hdmi;

	pr_info("%s()\n", __func__);

	for (i = 0; i < display->output_count; i++) {
		int type = display->encoder[i].type;
		u8 mst_parent_id;

		switch (type) {
		case MTDISP_ENCODER_DP:
			dp = &display->encoder[i].dp;

			if (dp->port_type == MTDISP_PORT_DISABLED)
				continue;

			mst_parent_id = mtdisp_get_dp_mst_parent_id(dp->hwlib_dp);
			dp->dp_mst_parent = &display->encoder[mst_parent_id].dp;

			ret = mtdisp_dp_probe(dp, dev, drm);
			if (ret)
				goto err_probe;

			break;
		case MTDISP_ENCODER_DP_MST:
			dp = &display->encoder[i].dp;

			mst_parent_id = mtdisp_get_dp_mst_parent_id(dp->hwlib_dp);
			dp->dp_mst_parent = &display->encoder[mst_parent_id].dp;
			break;
		case MTDISP_ENCODER_HDMI:
			hdmi = &display->encoder[i].hdmi;

			ret = mtdisp_hdmi_probe(hdmi, dev, drm);
			if (ret)
				goto err_probe;
			break;
		}
	}

	return 0;

err_probe:
	mtdisp_encoder_remove(display, dev);

	return ret;
}

int mtdisp_encoder_remove(struct mtdisp_display *display, struct device *dev)
{
	int i;
	struct mtdisp_dp *dp;
	struct mtdisp_hdmi *hdmi;

	pr_info("%s()\n", __func__);

	for (i = 0; i < display->output_count; i++) {
		int type = display->encoder[i].type;

		switch (type) {
		case MTDISP_ENCODER_DP:
			dp = &display->encoder[i].dp;

			if (dp->port_type == MTDISP_PORT_DISABLED)
				continue;

			mtdisp_dp_remove(dp, dev);
			dp->dp_mst_parent = NULL;
			break;
		case MTDISP_ENCODER_DP_MST:
			dp = &display->encoder[i].dp;

			dp->dp_mst_parent = NULL;
			break;
		case MTDISP_ENCODER_HDMI:
			hdmi = &display->encoder[i].hdmi;

			mtdisp_hdmi_remove(hdmi, dev);
			break;
		}
	}

	pr_info("%s() successfully\n", __func__);

	return 0;
}

int mtdisp_encoder_resume_early(struct mtdisp_display *display, struct device *dev)
{
	int i;
	struct mtdisp_hdmi *hdmi;
	struct mtdisp_dp *dp;

	pr_info("%s()\n", __func__);

	for (i = 0; i < display->output_count; i++) {
		int type = display->encoder[i].type;

		switch (type) {
		case MTDISP_ENCODER_DP:
			dp = &display->encoder[i].dp;

			if (dp->port_type == MTDISP_PORT_DISABLED)
				continue;

			mtdisp_dp_resume(dp, dev);
			break;
		case MTDISP_ENCODER_HDMI:
			hdmi = &display->encoder[i].hdmi;

			mtdisp_hdmi_resume(hdmi, dev);
			break;
		}
	}

	pr_info("%s() successfully\n", __func__);

	return 0;
}

int mtdisp_encoder_suspend(struct mtdisp_display *display, struct device *dev)
{
	int i;
	struct mtdisp_dp *dp;

	pr_info("%s()\n", __func__);

	for (i = 0; i < display->output_count; i++) {
		int type = display->encoder[i].type;

		switch (type) {
		case MTDISP_ENCODER_DP:
			dp = &display->encoder[i].dp;

			if (dp->port_type == MTDISP_PORT_DISABLED)
				continue;

			mtdisp_dp_suspend(dp, dev);
			break;
		case MTDISP_ENCODER_HDMI:
			break;
		}
	}

	pr_info("%s() successfully\n", __func__);

	return 0;
}

void mtdisp_encoder_complete(struct mtdisp_display *display, struct device *dev)
{
	int i;
	struct mtdisp_hdmi *hdmi;

	pr_info("%s()\n", __func__);

	for (i = 0; i < display->output_count; i++) {
		int type = display->encoder[i].type;

		if (type == MTDISP_ENCODER_HDMI) {
			hdmi = &display->encoder[i].hdmi;

			mtdisp_hdmi_complete(hdmi, dev);
		}
	}

	pr_info("%s() successfully\n", __func__);
}

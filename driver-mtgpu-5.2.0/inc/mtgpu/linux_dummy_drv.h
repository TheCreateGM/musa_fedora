/*
 * @Copyright   Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License     Dual MIT/GPLv2
 */

#ifndef __LINUX_DUMMY_DRV_H__
#define __LINUX_DUMMY_DRV_H__

struct device;
struct mtdisp_dispc;
struct mtdisp_encoder;
struct mtdisp_dummy_platform_data;

struct mtdisp_dummy {
	struct device *dev;

	struct mtdisp_dummy_crtc      *crtc;
	struct mtdisp_dummy_connector *conn;

	u8 crtc_count;
	u8 conn_count;
};

int mtdisp_dummy_init(struct device *dev, struct drm_device *drm,
		      struct mtdisp_dummy_platform_data *pdata);
void mtdisp_dummy_deinit(struct device *dev, struct drm_device *drm,
			 struct mtdisp_dummy_platform_data *pdata);

#endif /* __LINUX_DISPLAY_DRV_H__ */

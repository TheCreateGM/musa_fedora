/*
 * @Copyright   Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License     Dual MIT/GPLv2
 */

#ifndef __LINUX_DISPLAY_DRV_H__
#define __LINUX_DISPLAY_DRV_H__

struct device;
struct mtdisp_dispc;
struct mtdisp_encoder;
struct mtdisp_display_platform_data;

struct mtdisp_display {
	struct device *dev;

	struct mtdisp_dispc   *dispc;
	struct mtdisp_encoder *encoder;

	u32 dispc_count;
	u32 output_count;

	void *hwlib;
};

int mtdisp_display_init(struct device *dev, struct drm_device *drm,
			struct mtdisp_display_platform_data *pdata);
void mtdisp_display_deinit(struct device *dev, struct drm_device *drm,
			  struct mtdisp_display_platform_data *pdata);
int mtdisp_display_resume_early(struct device *dev);
int mtdisp_display_suspend(struct device *dev);
void mtdisp_display_complete(struct device *dev);

#endif /* __LINUX_DISPLAY_DRV_H__ */

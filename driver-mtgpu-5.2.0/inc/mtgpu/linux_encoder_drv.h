/*
 * @Copyright   Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License     Dual MIT/GPLv2
 */

#ifndef __LINUX_ENCODER_DRV_H__
#define __LINUX_ENCODER_DRV_H__

#include "linux_dp_drv.h"
#include "linux_hdmi_drv.h"

struct mtdisp_display;

struct mtdisp_encoder {
	enum mtdisp_encoder_type type;
	union {
		struct mtdisp_dp   dp;
		struct mtdisp_hdmi hdmi;
	};
};

int mtdisp_encoder_probe(struct mtdisp_display *display,
			 struct device *dev, struct drm_device *drm);
int mtdisp_encoder_remove(struct mtdisp_display *display, struct device *dev);
int mtdisp_encoder_resume_early(struct mtdisp_display *display, struct device *dev);
int mtdisp_encoder_suspend(struct mtdisp_display *display, struct device *dev);
void mtdisp_encoder_complete(struct mtdisp_display *display, struct device *dev);

#endif /* __LINUX_ENCODER_DRV_H__ */

/*
 * @Copyright   Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License     Dual MIT/GPLv2
 */

#ifndef __LINUX_HDMI_DRV_H__
#define __LINUX_HDMI_DRV_H__

#include "linux_display_debug.h"

typedef void (*hdmi_codec_plugged_cb)(struct device *dev, bool plugged);

struct mtdisp_hdmi {
	int id;   // same to hwlib dp
	int irq;  // same to hwlib dp
	bool connected; // same to hwlib dp

	struct device *dev;
	struct drm_encoder encoder;
	struct drm_connector connector;

	u8 port_type;

	struct i2c_adapter *ddc;
	struct mutex i2c_lock;
	struct delayed_work hpd_work;

	/* audio codec */
	struct platform_device *hdmi_audio;
	hdmi_codec_plugged_cb codec_plugged_cb;
	struct device *codec_dev;
	struct mutex codec_lock;
	bool audio_enabled;

	struct edid *edid;

	void *hwlib_hdmi;
};

int mtdisp_hdmi_probe(struct mtdisp_hdmi *hdmi, struct device *dev, struct drm_device *drm);
int mtdisp_hdmi_remove(struct mtdisp_hdmi *hdmi, struct device *dev);
int mtdisp_hdmi_resume(struct mtdisp_hdmi *hdmi, struct device *dev);
void mtdisp_hdmi_complete(struct mtdisp_hdmi *hdmi, struct device *dev);

#endif /* __LINUX_HDMI_DRV_H__ */

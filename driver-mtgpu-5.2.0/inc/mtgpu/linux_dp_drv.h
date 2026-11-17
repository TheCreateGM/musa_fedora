/*
 * @Copyright   Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License     Dual MIT/GPLv2
 */

#ifndef __LINUX_DP_DRV_H__
#define __LINUX_DP_DRV_H__

#include <linux/printk.h>
#include <drm/drm_encoder.h>
#include <drm/drm_connector.h>
#if defined(OS_DRM_DRM_DP_HELPER_H_EXIST)
#include <drm/drm_dp_helper.h>
#elif defined(OS_DRM_DP_DRM_DP_HELPER_H_EXIST)
#include <drm/dp/drm_dp_helper.h>
#elif defined(OS_DRM_DISPLAY_DRM_DP_HELPER_H_EXIST)
#include <drm/display/drm_dp_helper.h>
#endif
#if defined(OS_DRM_DRM_DP_MST_HELPER_H_EXIST)
#include <drm/drm_dp_mst_helper.h>
#elif defined(OS_DRM_DISPLAY_DRM_DP_MST_HELPER_H_EXIST)
#include <drm/display/drm_dp_mst_helper.h>
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(6, 8, 0)
/* Newer kernels moved the MST helper under include/drm/display/. */
#include <drm/display/drm_dp_mst_helper.h>
#endif

#include "linux_display_debug.h"

#define MTGPU_DP_IRQ_TYPE_PLUG_EVENT	BIT(0)
#define MTGPU_DP_IRQ_TYPE_UNPLUG_EVENT	BIT(1)
#define MTGPU_DP_IRQ_TYPE_HPD_IRQ	BIT(2)

#define DELAY_ON_DP_PLUGOUT_DET_MS (1000)

typedef void (*hdmi_codec_plugged_cb)(struct device *dev, bool plugged);
// hdmi_codec_plugged_cb codec_plugged_cb;

struct mt_display_mode {
	int clock;
	int hdisplay;
	int hsync_start;
	int hsync_end;
	int htotal;
	int vdisplay;
	int vsync_start;
	int vsync_end;
	int vtotal;
};

/* the monitors which need patch like skip TPS4
 * as sudi do not support it when use DP1.4;
 * we can expand these array for other patchs.
 */
enum monitor_patch_type {
	PATCH_TYPE_INVALID = 0x00,
	PATCH_SKIP_TPS4_FOR_TRAINING,
	PATCH_FORCE_TRAINING,
	PATCH_FORCE_OVERWRITE_TIMING,
};

enum mtdisp_soc_gen {
	PLAT_INVALID = 0,
	PLAT_GEN1    = BIT(0),
	PLAT_GEN2    = BIT(1),
	PLAT_GEN3    = BIT(2),
	PLAT_GEN4    = BIT(3),
};

struct monitor_patch {
	enum mtdisp_soc_gen gen;

	u8  vendor[4];
	u32 product_id;
	u32 patch_type;

	struct mt_display_mode original_mode;
	struct mt_display_mode patched_mode;
};

struct mtdisp_dp_debugfs {
	struct dentry *card;
	u32 link_rate;
	u8  lane_cnt;
};

struct mtdisp_dp {
	int id;   // same to hwlib dp
	int irq;  // same to hwlib dp
	bool connected; // same to hwlib dp
	u8 port_type;

	struct device *dev;
	struct drm_encoder encoder;
	struct drm_connector connector;
	struct drm_dp_aux aux;

	struct delayed_work hpd_work;
	struct work_struct hpd_irq_work;
	struct mutex lock;

	/* audio codec */
	struct platform_device *dp_audio;
	hdmi_codec_plugged_cb codec_plugged_cb;

	bool audio_enabled;
	struct device *codec_dev;
	struct mutex codec_lock; /* codec lock */
	struct edid *edid;

	/* mst support */
	struct drm_dp_mst_topology_mgr mst_mgr;
	struct drm_dp_mst_port *port[4];

	struct mtdisp_dp *dp_mst_parent;

	struct mtdisp_dp_debugfs debugfs;

	void *hwlib_dp;
};

void mtdisp_dp_encoder_enable(struct drm_encoder *encoder);
void mtdisp_dp_encoder_disable(struct drm_encoder *encoder);
int mtdisp_dp_probe(struct mtdisp_dp *dp,
		    struct device *dev, struct drm_device *drm);
int mtdisp_dp_remove(struct mtdisp_dp *dp, struct device *dev);
int mtdisp_dp_resume(struct mtdisp_dp *dp, struct device *dev);
int mtdisp_dp_suspend(struct mtdisp_dp *dp, struct device *dev);

#endif /* __LINUX_DP_DRV_H__ */

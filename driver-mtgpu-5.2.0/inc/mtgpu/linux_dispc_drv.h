/*
 * @Copyright   Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License     Dual MIT/GPLv2
 */

#ifndef __LINUX_DISPC_DRV_H__
#define __LINUX_DISPC_DRV_H__

#include <linux/printk.h>
#include <drm/drm_crtc.h>

#include "linux_display_debug.h"

#include "mtgpu_capture.h"

struct drm_device;
struct drm_crtc;
struct drm_plane_state;
struct mtdisp_display;
struct mtdisp_layer_config;

struct mtdisp_dispc_debugfs {
	struct dentry *dentry;
	u64 underrun_cnt;
};

struct mtdisp_dispc {
	int id;   // same to hwlib dispc
	int irq;  // same to hwlib dispc

	struct device *dev;
	struct drm_crtc crtc;

	u32 bandwidth;
	bool active;

	u32 layer_count;
	struct mtdisp_layer_capability *layer_caps;

	struct mtdisp_dispc_debugfs debugfs;

	struct mtgpu_capture_context *capture_ctx;

	void *hwlib_dispc;
};

struct mtgpu_capture_context *
mtdisp_dispc_get_capture_ctx(struct drm_connector *connector);
void mtdisp_get_layer_config(struct mtdisp_layer_config *config,
			     struct drm_plane_state *state);
int mtdisp_dispc_probe(struct mtdisp_display *display,
		       struct device *dev, struct drm_device *drm);
int mtdisp_dispc_remove(struct mtdisp_display *display, struct device *dev);

#endif /* __LINUX_DISPC_DRV_H__ */

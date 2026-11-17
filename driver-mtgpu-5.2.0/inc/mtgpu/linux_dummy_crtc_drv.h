/*
 * @Copyright   Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License     Dual MIT/GPLv2
 */

#ifndef __LINUX_DUMMY_CRTC_DRV_H__
#define __LINUX_DUMMY_CRTC_DRV_H__

#include <linux/printk.h>
#include <drm/drm_crtc.h>

#include "linux_display_debug.h"

#include "mtgpu_capture.h"

struct drm_device;
struct drm_crtc;
struct mtdisp_dummy;

struct mtdisp_dummy_crtc {
	int id;
	int irq;

	u16 max_hres;
	u16 max_vres;

	struct device *dev;
	struct drm_crtc crtc;

	spinlock_t dummy_lock;
	u32 vblank_periods;
	struct hrtimer vblank_hrtimer;

	/* Track initialized/registered resources
	 * for safe error unwinding and remove.
	 */
	bool crtc_initialized;
	bool irq_registered;
	bool irq_enabled;
	bool vsync_registered;

	struct mtgpu_capture_context *capture_ctx;
};

struct mtgpu_capture_context *
mtdisp_dummy_get_capture_ctx(struct drm_connector *connector);
int mtdisp_dummy_crtc_probe(struct mtdisp_dummy *dummy,
			    struct device *dev, struct drm_device *drm);
void mtdisp_dummy_crtc_remove(struct mtdisp_dummy *dummy, struct device *dev);

#endif /* __LINUX_DUMMY_CRTC_DRV_H__ */

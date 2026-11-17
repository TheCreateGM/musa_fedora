/*
 * @Copyright   Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License     Dual MIT/GPLv2
 */

#include <linux/delay.h>
#if defined(OS_DRM_DRMP_H_EXIST)
#include <drm/drmP.h>
#endif
#include <drm/drm_atomic_helper.h>
#include <drm/drm_fourcc.h>
#include <drm/drm_plane_helper.h>
#include <drm/drm_print.h>
#include <drm/drm_vblank.h>
#include <drm/drm_gem_framebuffer_helper.h>

#include "mtgpu_irq.h"
#include "mtgpu_vgpu_ipc.h"
#include "vgpu_virtual_display.h"
#include "mtgpu_drv.h"

#include "dispc_common.h"

#include "linux_dispc_drv.h"
#include "linux_dummy_drv.h"
#include "linux_dummy_crtc_drv.h"

#define ONE_SECOND_NS		1000000000
#define DUMMY_VREFRESH_MAX	300

static const u32 dummy_formats[] = {
	DRM_FORMAT_XRGB8888,
	DRM_FORMAT_ARGB8888,
	DRM_FORMAT_YUV420,
	DRM_FORMAT_NV12,
};

static const u32 dummy_cursor_formats[] = {
	DRM_FORMAT_ARGB8888,
};

static inline struct mtdisp_dummy_crtc *to_mtdisp_dummy_crtc(struct drm_crtc *crtc)
{
	return container_of(crtc, struct mtdisp_dummy_crtc, crtc);
}

struct mtgpu_capture_context *
mtdisp_dummy_get_capture_ctx(struct drm_connector *connector)
{
	int i, try_count = 50;
	struct mtdisp_dummy_crtc *dummy_crtc;

	/* crtc maybe is null when resolution change, eg: rotation resize zoom etc */
	for (i = 0; i < try_count && !connector->state->crtc; i++)
		mdelay(10);

	if (!connector->state->crtc)
		return NULL;

	dummy_crtc = to_mtdisp_dummy_crtc(connector->state->crtc);
	if (!dummy_crtc)
		return NULL;

	return dummy_crtc->capture_ctx;
}

#if defined(OS_DRM_PLANE_HELPER_FUNCS_USE_DRM_ATOMIC_STATE)
static void mtdisp_dummy_plane_atomic_update(struct drm_plane *plane,
					     struct drm_atomic_state *state)
#else
static void mtdisp_dummy_plane_atomic_update(struct drm_plane *plane,
					     struct drm_plane_state *old_state)
#endif
{
	struct drm_plane_state *plane_state = plane->state;
	struct mtdisp_dummy_crtc *dummy_crtc = NULL;
	struct mtdisp_layer_config layer = {};
	struct drm_framebuffer *fb;

	if (!plane_state || !plane_state->crtc)
		return;

	fb = plane_state->fb;
	if (!fb)
		return;

	dummy_crtc = to_mtdisp_dummy_crtc(plane_state->crtc);
	if (!dummy_crtc) {
		DRM_ERROR("%s null dummy_crtc\n", __func__);
		return;
	}

	mtdisp_get_layer_config(&layer, plane_state);
	if (plane->type == DRM_PLANE_TYPE_PRIMARY)
		mtgpu_capture_job_process(dummy_crtc->dev, dummy_crtc->capture_ctx, &layer,
					  drm_gem_fb_get_obj(fb, 0));
}

static const struct drm_plane_helper_funcs mtdisp_dummy_primary_helper_funcs = {
	.atomic_update		= mtdisp_dummy_plane_atomic_update,
};

static void mtdisp_dummy_plane_destroy(struct drm_plane *plane)
{
	drm_plane_cleanup(plane);
	kfree(plane);
}

static const struct drm_plane_funcs mtdisp_dummy_plane_funcs = {
	.update_plane		= drm_atomic_helper_update_plane,
	.disable_plane		= drm_atomic_helper_disable_plane,
	.destroy		= mtdisp_dummy_plane_destroy,
	.reset			= drm_atomic_helper_plane_reset,
	.atomic_duplicate_state = drm_atomic_helper_plane_duplicate_state,
	.atomic_destroy_state	= drm_atomic_helper_plane_destroy_state,
};

static void mtdisp_dummy_crtc_destroy(struct drm_crtc *crtc)
{
	struct mtdisp_dummy_crtc *dummy_crtc = to_mtdisp_dummy_crtc(crtc);

	LINUX_DUMMY_CRTC_INFO("destroy dummy crtc\n");
}

static enum hrtimer_restart mtdisp_dummy_vblank_simulate(struct hrtimer *timer)
{
	struct mtdisp_dummy_crtc *dummy_crtc = container_of(timer, struct mtdisp_dummy_crtc,
					       vblank_hrtimer);
	u64 ret_overrun;
	bool ret;
	unsigned long flags;

	if (!spin_trylock_irqsave(&dummy_crtc->dummy_lock, flags)) {
		hrtimer_add_expires_ns(&dummy_crtc->vblank_hrtimer,
				       dummy_crtc->vblank_periods);
		DRM_WARN("add expires_ns for hrtimer.\n");
		return HRTIMER_RESTART;
	}

	if (dummy_crtc->capture_ctx && dummy_crtc->capture_ctx->fence_array) {
		/*  Do not re-enter the work while waiting for the copy task to complete */
		if (!work_busy(dummy_crtc->capture_ctx->work))
			schedule_work(dummy_crtc->capture_ctx->work);
	} else {
		ret = drm_crtc_handle_vblank(&dummy_crtc->crtc);
		if (!ret) {
			DRM_WARN("dummy crtc failure on handling vblank");
			spin_unlock_irqrestore(&dummy_crtc->dummy_lock, flags);
			return HRTIMER_NORESTART;
		}
	}

	ret_overrun = hrtimer_forward_now(&dummy_crtc->vblank_hrtimer,
					  dummy_crtc->vblank_periods);
	if (ret_overrun != 1)
		DRM_WARN("hrtimer_forward return overrun %lld.\n", ret_overrun);

	spin_unlock_irqrestore(&dummy_crtc->dummy_lock, flags);

	return HRTIMER_RESTART;
}

static unsigned int mtdisp_dummy_vblank_periods_calculate(struct drm_crtc *crtc)
{
	u32 vblank_periods = 0;
	int vrefresh;

	vrefresh = drm_mode_vrefresh(&crtc->state->mode);
	if (vrefresh)
		vblank_periods = ONE_SECOND_NS / vrefresh;

	return vblank_periods;
}

static int mtdisp_dummy_enable_vblank(struct drm_crtc *crtc)
{
	struct mtdisp_dummy_crtc *dummy_crtc = to_mtdisp_dummy_crtc(crtc);

	LINUX_DUMMY_CRTC_DEBUG("Enable software vsync timer\n");

	if (mtgpu_get_driver_mode() == MTGPU_DRIVER_MODE_GUEST) {
		mtgpu_vgpu_ipc_vsync_status_on(dummy_crtc->dev->parent->parent, dummy_crtc->id);
	} else {
#ifdef OS_HRTIMER_SETUP_CALLABLE
		hrtimer_setup(&dummy_crtc->vblank_hrtimer, &mtdisp_dummy_vblank_simulate,
			      CLOCK_MONOTONIC, HRTIMER_MODE_REL);
#else
		hrtimer_init(&dummy_crtc->vblank_hrtimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
		dummy_crtc->vblank_hrtimer.function = &mtdisp_dummy_vblank_simulate;
#endif
		dummy_crtc->vblank_periods = mtdisp_dummy_vblank_periods_calculate(crtc);

		if (dummy_crtc->vblank_periods)
			hrtimer_start(&dummy_crtc->vblank_hrtimer,
				      dummy_crtc->vblank_periods, HRTIMER_MODE_REL);
	}

	return 0;
}

static void mtdisp_dummy_disable_vblank(struct drm_crtc *crtc)
{
	struct mtdisp_dummy_crtc *dummy_crtc = to_mtdisp_dummy_crtc(crtc);
	unsigned long flags;

	LINUX_DUMMY_CRTC_DEBUG("Disable software vsync timer\n");
	if (mtgpu_get_driver_mode() == MTGPU_DRIVER_MODE_GUEST) {
		mtgpu_vgpu_ipc_vsync_status_off(dummy_crtc->dev->parent->parent, dummy_crtc->id);
	} else {
		if (!spin_trylock_irqsave(&dummy_crtc->dummy_lock, flags))
			return;

		hrtimer_cancel(&dummy_crtc->vblank_hrtimer);

		spin_unlock_irqrestore(&dummy_crtc->dummy_lock, flags);
	}
}

static const struct drm_crtc_funcs mtdisp_dummy_crtc_funcs = {
	.set_config             = drm_atomic_helper_set_config,
	.destroy                = mtdisp_dummy_crtc_destroy,
	.page_flip              = drm_atomic_helper_page_flip,
	.reset                  = drm_atomic_helper_crtc_reset,
	.atomic_duplicate_state = drm_atomic_helper_crtc_duplicate_state,
	.atomic_destroy_state   = drm_atomic_helper_crtc_destroy_state,
	.enable_vblank		= mtdisp_dummy_enable_vblank,
	.disable_vblank		= mtdisp_dummy_disable_vblank,
};

static enum drm_mode_status mtdisp_dummy_crtc_mode_valid(struct drm_crtc *crtc,
							 const struct drm_display_mode *mode)
{
	struct mtdisp_dummy_crtc *dummy_crtc = to_mtdisp_dummy_crtc(crtc);
	int vrefresh = drm_mode_vrefresh(mode);

	if (vrefresh <= 0 || vrefresh > DUMMY_VREFRESH_MAX) {
		LINUX_DUMMY_CRTC_ERROR("invalid vrefresh %d, "
				       "the range of vrefresh should be (0,%d]\n",
				       vrefresh, DUMMY_VREFRESH_MAX);
		return MODE_BAD_VSCAN;
	}

	return MODE_OK;
}

static void mtdisp_dummy_crtc_atomic_flush(struct drm_crtc *crtc,
#if defined(OS_DRM_CRTC_HELPER_FUNCS_USE_DRM_ATOMIC_STATE)
					   struct drm_atomic_state *old_state)
#else
					   struct drm_crtc_state *old_state)
#endif
{
	unsigned long flags;

	if (crtc->state->event) {
		spin_lock_irqsave(&crtc->dev->event_lock, flags);

		if (drm_crtc_vblank_get(crtc) != 0)
			drm_crtc_send_vblank_event(crtc, crtc->state->event);
		else
			drm_crtc_arm_vblank_event(crtc, crtc->state->event);

		spin_unlock_irqrestore(&crtc->dev->event_lock, flags);

		crtc->state->event = NULL;
	}
}

static void mtdisp_dummy_crtc_atomic_enable(struct drm_crtc *crtc,
#if defined(OS_DRM_CRTC_HELPER_FUNCS_USE_DRM_ATOMIC_STATE)
					    struct drm_atomic_state *old_state)
#else
					    struct drm_crtc_state *old_state)
#endif
{
	drm_crtc_vblank_on(crtc);
}

static void mtdisp_dummy_crtc_atomic_disable(struct drm_crtc *crtc,
#if defined(OS_DRM_CRTC_HELPER_FUNCS_USE_DRM_ATOMIC_STATE)
					     struct drm_atomic_state *old_state)
#else
					     struct drm_crtc_state *old_state)
#endif
{
	unsigned long flags;

	drm_crtc_vblank_off(crtc);

	spin_lock_irqsave(&crtc->dev->event_lock, flags);
	if (crtc->state->event) {
		drm_crtc_send_vblank_event(crtc, crtc->state->event);
		crtc->state->event = NULL;
	}
	spin_unlock_irqrestore(&crtc->dev->event_lock, flags);
}

static const struct drm_crtc_helper_funcs mtdisp_dummy_crtc_helper_funcs = {
	.mode_valid	= mtdisp_dummy_crtc_mode_valid,
	.atomic_flush	= mtdisp_dummy_crtc_atomic_flush,
	.atomic_enable	= mtdisp_dummy_crtc_atomic_enable,
	.atomic_disable	= mtdisp_dummy_crtc_atomic_disable,
};

static struct drm_plane * mtdisp_dummy_plane_init(struct drm_device *drm,
						  enum drm_plane_type type, int crtc_index)
{
	const struct drm_plane_helper_funcs *funcs;
	struct drm_plane *plane;
	const u32 *formats;
	int ret, nformats;

	plane = kzalloc(sizeof(*plane), GFP_KERNEL);
	if (!plane)
		return ERR_PTR(-ENOMEM);

	if (type == DRM_PLANE_TYPE_CURSOR) {
		formats = dummy_cursor_formats;
		nformats = ARRAY_SIZE(dummy_cursor_formats);
		funcs = &mtdisp_dummy_primary_helper_funcs;
	} else {
		formats = dummy_formats;
		nformats = ARRAY_SIZE(dummy_formats);
		funcs = &mtdisp_dummy_primary_helper_funcs;
	}

	ret = drm_universal_plane_init(drm, plane, 1 << crtc_index,
				       &mtdisp_dummy_plane_funcs,
				       formats, nformats,
				       NULL, type, NULL);
	if (ret) {
		DRM_ERROR("failed to init dummy plane\n");
		kfree(plane);
		return ERR_PTR(ret);
	}

	drm_plane_helper_add(plane, funcs);

	return plane;
}

static void mtdisp_dummy_crtc_isr(void *data)
{
	struct mtdisp_dummy_crtc *dummy_crtc = data;
	struct device *dev = dummy_crtc->dev->parent->parent;

	if (mtgpu_vgpu_ipc_vsync_status_is_irq(dev, dummy_crtc->id)) {
		mtgpu_vgpu_ipc_vsync_status_clear_irq(dev, dummy_crtc->id);
		if (dummy_crtc->capture_ctx && dummy_crtc->capture_ctx->fence_array)
			schedule_work(dummy_crtc->capture_ctx->work);
		else
			drm_crtc_handle_vblank(&dummy_crtc->crtc);
	}
}

void mtdisp_dummy_crtc_remove(struct mtdisp_dummy *dummy, struct device *dev)
{
	int i;
	struct mtdisp_dummy_crtc *dummy_crtc;

	DRM_DEV_INFO(dev, "%s()\n", __func__);

	if (!dummy || !dummy->crtc)
		return;

	for (i = 0; i < dummy->crtc_count; i++) {
		dummy_crtc = &dummy->crtc[i];

		dummy_crtc->dev = dev;

		if (mtgpu_get_driver_mode() == MTGPU_DRIVER_MODE_GUEST) {
			if (dummy_crtc->vsync_registered) {
				vgpu_vdisplay_vsync_unregister(dev->parent->parent, dummy_crtc->id);
				dummy_crtc->vsync_registered = false;
			}

			if (dummy_crtc->irq_enabled) {
				mtgpu_disable_interrupt(dev->parent->parent, dummy_crtc->irq);
				dummy_crtc->irq_enabled = false;
			}

			if (dummy_crtc->irq_registered) {
				mtgpu_unregister_interrupt(dev->parent->parent, dummy_crtc->irq);
				dummy_crtc->irq_registered = false;
			}
		}

		if (dummy_crtc->crtc_initialized) {
			drm_crtc_cleanup(&dummy_crtc->crtc);
			dummy_crtc->crtc_initialized = false;
		}

		mtgpu_capture_context_destroy(dummy_crtc->capture_ctx);
		dummy_crtc->capture_ctx = NULL;
	}
}

int mtdisp_dummy_crtc_probe(struct mtdisp_dummy *dummy,
			    struct device *dev, struct drm_device *drm)
{
	int i, ret;
	struct drm_plane *primary, *cursor;
	struct mtdisp_dummy_crtc *dummy_crtc;
	struct mtgpu_capture_context *capture_ctx;

	DRM_DEV_INFO(dev, "%s()\n", __func__);

	for (i = 0; i < dummy->crtc_count; i++) {
		dummy_crtc = &dummy->crtc[i];

		dummy_crtc->dev = dev;

		if (mtgpu_get_driver_mode() == MTGPU_DRIVER_MODE_GUEST) {
			ret = mtgpu_register_interrupt(dev->parent->parent,
						       dummy_crtc->irq,
						       mtdisp_dummy_crtc_isr,
						       dummy_crtc,
						       "dummy-crtc");
			if (ret) {
				LINUX_DUMMY_CRTC_ERROR("failed to register crtc irq handler\n");
				ret = -EINVAL;
				goto err_dummy_crtc_probe;
			}
			dummy_crtc->irq_registered = true;

			ret = mtgpu_enable_interrupt(dev->parent->parent, dummy_crtc->irq);
			if (ret) {
				LINUX_DUMMY_CRTC_ERROR("failed to enable irq\n");
				ret = -EINVAL;
				goto err_dummy_crtc_probe;
			}
			dummy_crtc->irq_enabled = true;

			vgpu_vdisplay_vsync_register(dev->parent->parent, dummy_crtc->id, 60);
			dummy_crtc->vsync_registered = true;
		}

#if defined(OS_STRUCT_DRM_MODE_CONFIG_HAS_ALLOW_FB_MODIFIERS)
		drm->mode_config.allow_fb_modifiers = false;
#endif

		primary = mtdisp_dummy_plane_init(drm, DRM_PLANE_TYPE_PRIMARY, dummy_crtc->id);
		if (IS_ERR(primary)) {
			ret = PTR_ERR(primary);
			goto err_dummy_crtc_probe;
		}

		cursor = mtdisp_dummy_plane_init(drm, DRM_PLANE_TYPE_CURSOR, dummy_crtc->id);
		if (IS_ERR(cursor)) {
			ret = PTR_ERR(cursor);
			drm_plane_cleanup(primary);
			kfree(primary);
			goto err_dummy_crtc_probe;
		}

		ret = drm_crtc_init_with_planes(drm, &dummy_crtc->crtc, primary, cursor,
						&mtdisp_dummy_crtc_funcs, NULL);
		if (ret) {
			LINUX_DUMMY_CRTC_ERROR("Failed to init dummy crtc\n");
			drm_plane_cleanup(cursor);
			kfree(cursor);
			drm_plane_cleanup(primary);
			kfree(primary);
			goto err_dummy_crtc_probe;
		}
		dummy_crtc->crtc_initialized = true;

		drm_crtc_helper_add(&dummy_crtc->crtc, &mtdisp_dummy_crtc_helper_funcs);

		spin_lock_init(&dummy_crtc->dummy_lock);

		ret = mtgpu_capture_context_create(dev, &dummy_crtc->crtc, &capture_ctx);
		if (ret)
			LINUX_DUMMY_CRTC_ERROR("Failed to init capture ctx\n");
		else
			dummy_crtc->capture_ctx = capture_ctx;
	}

	DRM_DEV_INFO(dev, "mtdisp dummy crtc driver loaded successfully\n");

	return 0;

err_dummy_crtc_probe:
	mtdisp_dummy_crtc_remove(dummy, dev);

	return ret;
}

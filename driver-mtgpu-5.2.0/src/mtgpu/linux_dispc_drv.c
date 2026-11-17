/*
 * @Copyright   Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License     Dual MIT/GPLv2
 */

#include <linux/device.h>
#include <linux/delay.h>
#include <linux/printk.h>
#include <linux/slab.h>
#include <drm/drm_atomic.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_plane_helper.h>
#include <drm/drm_gem_atomic_helper.h>
#include <drm/drm_vblank.h>
#include <drm/drm_fourcc.h>
#include <drm/drm_framebuffer.h>
#include <drm/drm_blend.h>
#if defined(OS_DRM_DRM_PANIC_H_EXIST)
#include <drm/drm_panic.h>
#endif
#if defined(OS_DRM_DRM_GEM_ATOMIC_HELPER_H_EXIST)
#include <drm/drm_gem_atomic_helper.h>
#endif
#include <drm/drm_gem_framebuffer_helper.h>
#include <video/videomode.h>

#include "mtgpu_irq.h"
#include "mtgpu_pstate.h"
#include "mtgpu_drm_gem.h"
#include "mtgpu_smc.h"

#include "dispc_common.h"
#include "mtdisp_dispc.h"

#include "dm_hwlib_access.h"

#include "linux_display_debugfs.h"
#include "linux_data_convert.h"
#include "linux_display_drv.h"
#include "linux_dispc_drv.h"

static inline void *to_mtdisp_dispc(struct drm_crtc *crtc)
{
	return container_of(crtc, struct mtdisp_dispc, crtc);
}

struct mtgpu_capture_context *
mtdisp_dispc_get_capture_ctx(struct drm_connector *connector)
{
	int i, try_count = 50;
	struct mtdisp_dispc *dispc;

	/* crtc maybe is null when resolution change, eg: rotation resize zoom etc */
	for (i = 0; i < try_count && !connector->state->crtc; i++)
		mdelay(10);

	if (!connector->state->crtc)
		return NULL;

	dispc = to_mtdisp_dispc(connector->state->crtc);
	if (!dispc)
		return NULL;

	return dispc->capture_ctx;
}

static bool mtdisp_dispc_idle_allowed(struct mtgpu_device *mtdev,
				      struct mtdisp_dispc *dispc)
{
	int i;
	u32 crtc_num = 0;
	u32 bandwidth = 0;
	struct mtdisp_display *display = dev_get_drvdata(dispc->dev);

	for (i = 0; i < display->dispc_count; i++)
		if (display->dispc[i].active) {
			crtc_num++;
			bandwidth += display->dispc[i].bandwidth;
		}

	LINUX_DISPC_DEBUG("crtc_num:%d, total bw:%d MHz!\n", crtc_num, bandwidth);

	return mtdisp_dispc_glb_is_idle_allowed(dispc->hwlib_dispc, bandwidth, crtc_num);
}

static void mtdisp_dispc_report_pstate(struct mtdisp_dispc *dispc)
{
	struct device *dev = dispc->dev;
	struct mtgpu_device *mtdev = dev_get_drvdata(dev->parent->parent);

	if (!mtdisp_dispc_idle_allowed(mtdev, dispc))
		mtgpu_pstate_notifier_call_chain(mtdev, MTGPU_PSTATE_EVENT_DISP_ACTIVE, NULL);
	else
		mtgpu_pstate_notifier_call_chain(mtdev, MTGPU_PSTATE_EVENT_DISP_IDLE, NULL);
}

static int mtdisp_plane_position_check(struct drm_plane_state *state)
{
	s16 dst_x, dst_y;
	u16 dst_w, dst_h;
	u32 hactive, vactive;
	struct mtdisp_dispc *dispc = to_mtdisp_dispc(state->crtc);

	hactive = state->crtc->mode.hdisplay;
	vactive = state->crtc->mode.vdisplay;

	/*
	 * When executing modetest, hactive = vactive = 0 will appear.
	 * In order not to affect the position check, 0 is returned.
	 */
	if (hactive == 0 || vactive == 0)
		return 0;

	dst_x = state->crtc_x;
	dst_y = state->crtc_y;
	dst_w = state->crtc_w;
	dst_h = state->crtc_h;

	/*
	 * If the intersection of the plane and the display area is less than 2,
	 * it is out of range and an error code is returned.
	 */
	if (((int)(hactive - dst_x) < 2 || (int)(vactive - dst_y) < 2) ||
	    ((int)(dst_x + dst_w) < 2   || (int)(dst_y + dst_h) < 2)) {
		LINUX_DISPC_WARN("%s() layer is out of range: "
				 "dst_x:%d, dst_y:%d, dst_w:%d, dst_h:%d\n",
				 __func__, dst_x, dst_y, dst_w, dst_h);
		return -EINVAL;
	}

	return 0;
}

#if defined(OS_DRM_PLANE_HELPER_FUNCS_USE_DRM_ATOMIC_STATE)
static int mtdisp_plane_atomic_check(struct drm_plane *plane,
				    struct drm_atomic_state *atomic_state)
{
	struct drm_plane_state *state = drm_atomic_get_plane_state(atomic_state, plane);
#else
static int mtdisp_plane_atomic_check(struct drm_plane *plane,
				     struct drm_plane_state *state)
{
#endif
	int ret;

	/*
	 * When the cursor plane slides to the bottom edge of the display,
	 * the intersection of the cursor plane and the display is 1 line.
	 * This plane will be discarded as "less than 2 lines" if its
	 * position is checked, which will cause the screen to flicker.
	 * Therefore, the position of the cursor plane is not checked and
	 * 0 is returned.
	 */
	if (plane->type == DRM_PLANE_TYPE_CURSOR)
		return 0;

	if (!state || !state->fb || !state->crtc)
		return 0;

	/*
	 * If the plane type is primary or overlay, you need to check
	 * whether the intersection of these planes and the display
	 * area is less than 2.
	 */
	ret = mtdisp_plane_position_check(state);
	if (ret)
		return ret;

	return 0;
}

void mtdisp_get_layer_config(struct mtdisp_layer_config *config,
			     struct drm_plane_state *state)
{
	struct drm_framebuffer *fb = state->fb;
	int i;

	config->src_x = state->src_x >> 16;
	config->src_y = state->src_y >> 16;
	config->src_w = state->src_w >> 16;
	config->src_h = state->src_h >> 16;
	config->dst_x = state->crtc_x;
	config->dst_y = state->crtc_y;
	config->dst_w = state->crtc_w;
	config->dst_h = state->crtc_h;
	config->alpha = state->alpha >> 8;
	config->rotation = state->rotation;
#if defined(OS_STRUCT_DRM_PLANE_STATE_HAS_PIXEL_BLEND_MODE)
	config->blend_mode = mtdisp_alpha_blend_mode_to_hw(state->pixel_blend_mode);
#endif
	config->color_encoding = state->color_encoding;
	config->color_range = state->color_range;
	config->num_planes = fb->format->num_planes;
	config->format = fb->format->format;
	config->is_fbc = !!fb->modifier;

	for (i = 0; i < fb->format->num_planes; i++) {
		config->addr[i] = mtgpu_fb_get_dma_addr(fb, state, i);
		config->pitch[i] = fb->pitches[i];
	}
}

#if defined(OS_DRM_PLANE_HELPER_FUNCS_USE_DRM_ATOMIC_STATE)
static void mtdisp_plane_atomic_update(struct drm_plane *plane,
				       struct drm_atomic_state *atomic_state)
#else
static void mtdisp_plane_atomic_update(struct drm_plane *plane,
				       struct drm_plane_state *old_state)
#endif
{
	struct drm_plane_state *state = plane->state;
	struct mtdisp_dispc *dispc = to_mtdisp_dispc(state->crtc);
	struct mtdisp_layer_config layer = {};
	struct drm_framebuffer *fb = state->fb;
	u8 index = state->normalized_zpos;

	mtdisp_get_layer_config(&layer, state);

	if (plane->type == DRM_PLANE_TYPE_PRIMARY)
		mtgpu_capture_job_process(dispc->dev, dispc->capture_ctx, &layer,
					  drm_gem_fb_get_obj(fb, 0));

	LINUX_DISPC_DEBUG("SRC_X = %d, SRC_Y = %d, SRC_W = %d, SRC_H = %d\n",
			  layer.src_x, layer.src_y, layer.src_w, layer.src_h);
	LINUX_DISPC_DEBUG("CRTC_X = %d, CRTC_Y = %d, CRTC_W = %d, CRTC_H = %d\n",
			  layer.dst_x, layer.dst_y, layer.dst_w, layer.dst_h);
	LINUX_DISPC_DEBUG("layer->addr[0] = 0x%llx, layer->pitch[0] = 0x%08x\n",
			  layer.addr[0], layer.pitch[0]);

	layer.format = mtdisp_format_fourcc_to_id(layer.format);
	mtdisp_set_dispc_ctx_primary_layer_format(dispc->hwlib_dispc, layer.format);

	/* first enable fbc and then dma addr, otherwise dpc may underflow */
	if (fb->modifier)
		mtdisp_dispc_glb_fbc_enable(dispc->hwlib_dispc, &layer, index);
	else
		mtdisp_dispc_glb_fbc_disable(dispc->hwlib_dispc, index);

	if (plane->type == DRM_PLANE_TYPE_CURSOR)
		hwlib_update_cursor(dispc->hwlib_dispc, &layer);
	else
		hwlib_update_plane(dispc->hwlib_dispc, &layer, index);
}

#if defined(OS_DRM_PLANE_HELPER_FUNCS_USE_DRM_ATOMIC_STATE)
static void mtdisp_plane_atomic_disable(struct drm_plane *plane,
					struct drm_atomic_state *state)
{
	struct drm_plane_state *old_state = drm_atomic_get_old_plane_state(state, plane);
#else
static void mtdisp_plane_atomic_disable(struct drm_plane *plane,
					struct drm_plane_state *old_state)
{
#endif
	struct mtdisp_dispc *dispc = to_mtdisp_dispc(old_state->crtc);
	u8 index = old_state->normalized_zpos;

	LINUX_DISPC_INFO("%s()\n", __func__);

	if (plane->type == DRM_PLANE_TYPE_CURSOR)
		hwlib_disable_cursor(dispc->hwlib_dispc);
	else
		hwlib_disable_plane(dispc->hwlib_dispc, index);

	mtdisp_dispc_glb_fbc_disable(dispc->hwlib_dispc, index);
}

#if defined(OS_DRM_PLANE_HELPER_FUNCS_USE_DRM_ATOMIC_STATE)
static int mtdisp_plane_atomic_async_check(struct drm_plane *plane,
					   struct drm_atomic_state *atomic_state
#ifdef OS_DRM_ASYNC_CHECK_HAS_FLIP
					   , bool flip
#endif
					   )
#else
static int mtdisp_plane_atomic_async_check(struct drm_plane *plane,
					   struct drm_plane_state *plane_state)
#endif
{
#ifdef OS_DRM_ASYNC_CHECK_HAS_FLIP
	if (flip)
		return -EINVAL;
#endif
	if (plane->type == DRM_PLANE_TYPE_CURSOR)
		return 0;

	return -EINVAL;
}

#if defined(OS_DRM_PLANE_HELPER_FUNCS_USE_DRM_ATOMIC_STATE)
static void mtdisp_plane_atomic_async_update(struct drm_plane *plane,
					     struct drm_atomic_state *atomic_state)
{
	struct drm_plane_state *new_state =
		drm_atomic_get_new_plane_state(atomic_state, plane);
#else
static void mtdisp_plane_atomic_async_update(struct drm_plane *plane,
					     struct drm_plane_state *new_state)
{
#endif
	struct mtdisp_dispc *dispc = to_mtdisp_dispc(new_state->crtc);
	struct mtdisp_layer_config config = {};

	mtdisp_get_layer_config(&config, new_state);

	config.format = mtdisp_format_fourcc_to_id(config.format);

	swap(plane->state->fb, new_state->fb);

	plane->state->src_x = new_state->src_x;
	plane->state->src_y = new_state->src_y;
	plane->state->src_w = new_state->src_w;
	plane->state->src_h = new_state->src_h;
	plane->state->crtc_x = new_state->crtc_x;
	plane->state->crtc_y = new_state->crtc_y;
	plane->state->crtc_w = new_state->crtc_w;
	plane->state->crtc_h = new_state->crtc_h;

	if (plane->type == DRM_PLANE_TYPE_CURSOR)
		hwlib_update_cursor(dispc->hwlib_dispc, &config);
}

#if defined(OS_STRUCT_DRM_PLANE_HELPER_FUNCS_HAS_GET_SCANOUT_BUFFER)
static int mtdisp_plane_get_scanout_buffer(struct drm_plane *plane,
					   struct drm_scanout_buffer *sb)
{
	struct drm_framebuffer *fb;
	struct drm_gem_object *obj;

	DRM_DEBUG("plane: %d\n", plane->index);

	if (!plane || !plane->state)
		return -ENODEV;

	fb = plane->state->fb;
	if (!fb || !fb->dev)
		return -ENODEV;

	/* Only support linear modifier */
	if (fb->modifier != DRM_FORMAT_MOD_LINEAR)
		return -ENODEV;

	if (!plane->state->crtc) {
		DRM_ERROR("%s() plane->state->crtc is NULL\n", __func__);
		return -ENODEV;
	}

	obj = drm_gem_fb_get_obj(fb, 0);
	if (!obj) {
		DRM_ERROR("%s() obj is NULL\n", __func__);
		return -ENODEV;
	}

	if (mtgpu_gem_prime_vmap(obj, &sb->map[0])) {
		DRM_INFO("vmap failed for scanout buffer\n");
		return -ENODEV;
	}

	sb->format = fb->format;
	sb->height = fb->height;
	sb->width = fb->width;
	sb->pitch[0] = fb->pitches[0];
	if (!sb->format || !sb->height || !sb->width || !sb->pitch[0]) {
		mtgpu_gem_prime_vunmap(obj, &sb->map[0]);
		return -ENODEV;
	}

	DRM_DEBUG("get scanout buffer success\n");

	return 0;
}
#endif

static const struct drm_plane_helper_funcs mtdisp_primary_helper_funcs = {
	.atomic_check		= mtdisp_plane_atomic_check,
	.atomic_update		= mtdisp_plane_atomic_update,
	.atomic_disable		= mtdisp_plane_atomic_disable,
	.atomic_async_check     = mtdisp_plane_atomic_async_check,
	.atomic_async_update    = mtdisp_plane_atomic_async_update,
#if defined(OS_FUNC_DRM_GEM_FB_PREPARE_FB_EXIST)
	.prepare_fb		= drm_gem_fb_prepare_fb,
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(5, 20, 0)
	.prepare_fb		= drm_gem_plane_helper_prepare_fb,
#else
	.prepare_fb		= drm_gem_plane_helper_prepare_fb,
#endif
#if defined(OS_STRUCT_DRM_PLANE_HELPER_FUNCS_HAS_GET_SCANOUT_BUFFER)
	.get_scanout_buffer	= mtdisp_plane_get_scanout_buffer,
#endif
};

static void mtdisp_plane_destroy(struct drm_plane *plane)
{
	pr_info("%s()\n", __func__);

	drm_plane_cleanup(plane);

	kfree(plane);
}

static bool mtdisp_format_mod_supported(struct drm_plane *plane,
					u32 format,
					u64 modifier)
{
	struct drm_plane_state *state = plane->state;
	struct mtdisp_dispc *dispc;
	u8 index;

	/* before atomic commit, state or crtc is NULL */
	if (!state || !state->crtc || !plane->crtc)
		return true;

	dispc = to_mtdisp_dispc(state->crtc);
	index = state->normalized_zpos;
	mtdisp_dispc_core_fbc_validate(dispc->hwlib_dispc,
				       format,
				       modifier,
				       index);

	return true;
}

static const struct drm_plane_funcs mtdisp_plane_funcs = {
	.update_plane		= drm_atomic_helper_update_plane,
	.disable_plane		= drm_atomic_helper_disable_plane,
	.destroy		= mtdisp_plane_destroy,
	.reset			= drm_atomic_helper_plane_reset,
	.atomic_duplicate_state = drm_atomic_helper_plane_duplicate_state,
	.atomic_destroy_state	= drm_atomic_helper_plane_destroy_state,
	.format_mod_supported   = mtdisp_format_mod_supported,
};

static void mtdisp_crtc_mode_set_nofb(struct drm_crtc *crtc)
{
	struct mtdisp_dispc *dispc = to_mtdisp_dispc(crtc);
	struct videomode vm;
	struct mtdisp_videomode mt_vm;

	drm_display_mode_to_videomode(&crtc->state->adjusted_mode, &vm);
	LINUX_DISPC_INFO(DRM_MODE_FMT "\n", DRM_MODE_ARG(&crtc->state->adjusted_mode));
	mtdisp_videomode_from_os_videomode(&mt_vm, &vm);
	mtdisp_set_dispc_ctx_vm(dispc->hwlib_dispc, &mt_vm);
}

static enum drm_mode_status
mtdisp_crtc_mode_valid(struct drm_crtc *crtc,
		      const struct drm_display_mode *mode)
{
	struct mtdisp_dispc *dispc = to_mtdisp_dispc(crtc);
	struct videomode vm;
	struct mtdisp_videomode mt_vm;

	LINUX_DISPC_DEBUG(DRM_MODE_FMT "\n", DRM_MODE_ARG(mode));

	drm_display_mode_to_videomode(mode, &vm);
	mtdisp_videomode_from_os_videomode(&mt_vm, &vm);

	/* check if the mode can be supported by DC. */
	return mtdisp_dispc_core_mode_valid(dispc->hwlib_dispc, &mt_vm);
}

static bool mtdisp_crtc_mode_fixup(struct drm_crtc *crtc,
				  const struct drm_display_mode *mode,
				  struct drm_display_mode *adjusted_mode)
{
	struct mtdisp_dispc *dispc = to_mtdisp_dispc(crtc);
	struct videomode vm;
	struct mtdisp_videomode mt_vm;

	drm_display_mode_to_videomode(adjusted_mode, &vm);
	mtdisp_videomode_from_os_videomode(&mt_vm, &vm);
	mtdisp_set_dispc_ctx_vm(dispc->hwlib_dispc, &mt_vm);

	mtdisp_dispc_core_mode_fixup(dispc->hwlib_dispc);

	mtdisp_get_dispc_ctx_vm(&mt_vm, dispc->hwlib_dispc);
	mtdisp_videomode_to_os_videomode(&vm, &mt_vm);
	drm_display_mode_from_videomode(&vm, adjusted_mode);

	LINUX_DISPC_INFO(DRM_MODE_FMT "\n", DRM_MODE_ARG(adjusted_mode));

	return true;
}

static void mtdisp_crtc_atomic_begin(struct drm_crtc *crtc,
#if defined(OS_DRM_CRTC_HELPER_FUNCS_USE_DRM_ATOMIC_STATE)
				    struct drm_atomic_state *old_state)
#else
				    struct drm_crtc_state *old_state)
#endif
{
	struct mtdisp_dispc *dispc = to_mtdisp_dispc(crtc);

	LINUX_DISPC_DEBUG("%s()\n", __func__);

	mtdisp_dispc_core_config_begin(dispc->hwlib_dispc);
}

static void mtdisp_crtc_update_gamma(struct drm_crtc *crtc)
{
	struct mtdisp_dispc *dispc = to_mtdisp_dispc(crtc);
	struct drm_crtc_state *state = crtc->state;
	struct drm_color_lut *lut = (struct drm_color_lut *)state->gamma_lut->data;
	u16 *red = crtc->gamma_store;
	u16 *green = red + crtc->gamma_size;
	u16 *blue = green + crtc->gamma_size;
	u16 *tmp_red = red;
	u16 *tmp_green = green;
	u16 *tmp_blue = blue;
	u32 i;
	struct mtdisp_gamma_config config;

	for (i = 0; i < crtc->gamma_size; i++) {
		*tmp_red   = (u16)drm_color_lut_extract(lut->red, 16);
		*tmp_green = (u16)drm_color_lut_extract(lut->green, 16);
		*tmp_blue  = (u16)drm_color_lut_extract(lut->blue, 16);

		lut++;
		tmp_red++;
		tmp_green++;
		tmp_blue++;
	}

	config.r_data = red;
	config.g_data = green;
	config.b_data = blue;
	config.size   = crtc->gamma_size;

	mtdisp_dispc_core_gamma_set(dispc->hwlib_dispc, &config);
}

static void mtdisp_crtc_atomic_flush(struct drm_crtc *crtc,
#if defined(OS_DRM_CRTC_HELPER_FUNCS_USE_DRM_ATOMIC_STATE)
				     struct drm_atomic_state *old_state)
#else
				     struct drm_crtc_state *old_state)
#endif
{
	struct mtdisp_dispc *dispc = to_mtdisp_dispc(crtc);
	unsigned long flags;

	LINUX_DISPC_DEBUG("%s()\n", __func__);

	if (crtc->state->color_mgmt_changed && crtc->state->gamma_lut)
		mtdisp_crtc_update_gamma(crtc);

	mtdisp_dispc_core_config_end(dispc->hwlib_dispc);

	spin_lock_irqsave(&crtc->dev->event_lock, flags);
	if (crtc->state->event) {

		if (drm_crtc_vblank_get(crtc) != 0)
			drm_crtc_send_vblank_event(crtc, crtc->state->event);
		else
			drm_crtc_arm_vblank_event(crtc, crtc->state->event);

		crtc->state->event = NULL;
	}
	spin_unlock_irqrestore(&crtc->dev->event_lock, flags);
}

static void mtdisp_reset_subsystem(struct mtdisp_dispc *dispc)
{
	u64 smc_bitmask	= BIT(dispc->id);
	u32 smc_ret = 0;

	/* reset DE and DSC */
	if (mtdisp_get_dispc_ctx_dsc_en(dispc->hwlib_dispc))
		smc_bitmask |= BIT(dispc->id + 4);

	smc_ret = mtgpu_smc_reset_subsystem(dispc->dev->parent->parent, SS_ID_DISP, smc_bitmask,
					    RESET_TYPE_PULSE);
	if (smc_ret)
		LINUX_DISPC_WARN("display reset fail!\n");
}

static void mtdisp_crtc_atomic_enable(struct drm_crtc *crtc,
#if defined(OS_DRM_CRTC_HELPER_FUNCS_USE_DRM_ATOMIC_STATE)
				      struct drm_atomic_state *old_state)
#else
				      struct drm_crtc_state *old_state)
#endif
{
	struct mtdisp_dispc *dispc = to_mtdisp_dispc(crtc);
	u16 *red = crtc->gamma_store;
	u16 *green = red + crtc->gamma_size;
	u16 *blue = green + crtc->gamma_size;
	u64 bandwidth = 0;
	struct drm_display_mode *dmode = &crtc->state->adjusted_mode;
	struct mtdisp_gamma_config config = {
		.r_data = red,
		.g_data = green,
		.b_data = blue,
		.size   = crtc->gamma_size,
	};

	LINUX_DISPC_INFO(DRM_MODE_FMT "\n", DRM_MODE_ARG(dmode));

	/* need reset display before dispc glb enable on quyuan2 */
	if (mtdisp_get_dispc_ctx_gen(dispc->hwlib_dispc) == MTDISP_SOC_GEN3)
		mtdisp_reset_subsystem(dispc);

	mtdisp_dispc_glb_enable(dispc->hwlib_dispc);

	mtdisp_dispc_dsc_start(dispc->hwlib_dispc);

	mtdisp_dispc_core_init(dispc->hwlib_dispc);

	mtdisp_dispc_core_gamma_set(dispc->hwlib_dispc, &config);

	bandwidth = (u64)drm_mode_vrefresh(dmode) * (u64)dmode->vtotal * (u64)dmode->htotal * 4;
	dispc->bandwidth = bandwidth / 1000000;
	dispc->active = true;

	mtdisp_dispc_report_pstate(dispc);

	dispc->debugfs.underrun_cnt = 0;

	drm_crtc_vblank_on(crtc);
}

static void mtdisp_crtc_atomic_disable(struct drm_crtc *crtc,
#if defined(OS_DRM_CRTC_HELPER_FUNCS_USE_DRM_ATOMIC_STATE)
				       struct drm_atomic_state *old_state)
#else
				       struct drm_crtc_state *old_state)
#endif
{
	struct mtdisp_dispc *dispc = to_mtdisp_dispc(crtc);
	unsigned long flags;

	LINUX_DISPC_DEBUG("%s()\n", __func__);

	drm_crtc_vblank_off(crtc);

	mtdisp_dispc_core_deinit(dispc->hwlib_dispc);

	mtdisp_dispc_dsc_stop(dispc->hwlib_dispc);

	spin_lock_irqsave(&crtc->dev->event_lock, flags);
	if (crtc->state->event) {
		drm_crtc_send_vblank_event(crtc, crtc->state->event);
		crtc->state->event = NULL;
	}
	spin_unlock_irqrestore(&crtc->dev->event_lock, flags);

	mtdisp_dispc_glb_disable(dispc->hwlib_dispc);

	dispc->bandwidth = 0;
	dispc->active = false;

	mtdisp_dispc_report_pstate(dispc);
}

static const struct drm_crtc_helper_funcs mtdisp_crtc_helper_funcs = {
	.mode_set_nofb	= mtdisp_crtc_mode_set_nofb,
	.mode_valid	= mtdisp_crtc_mode_valid,
	.mode_fixup	= mtdisp_crtc_mode_fixup,
	.atomic_begin	= mtdisp_crtc_atomic_begin,
	.atomic_flush	= mtdisp_crtc_atomic_flush,
	.atomic_enable	= mtdisp_crtc_atomic_enable,
	.atomic_disable	= mtdisp_crtc_atomic_disable,
};

static void mtdisp_crtc_destroy(struct drm_crtc *crtc)
{
	struct mtdisp_dispc *dispc = to_mtdisp_dispc(crtc);

	LINUX_DISPC_INFO("%s()\n", __func__);

	drm_crtc_cleanup(crtc);
}

static int mtdisp_crtc_enable_vblank(struct drm_crtc *crtc)
{
	struct mtdisp_dispc *dispc = to_mtdisp_dispc(crtc);

	LINUX_DISPC_DEBUG("%s()\n", __func__);

	mtdisp_dispc_core_vsync_on(dispc->hwlib_dispc);

	return 0;
}

static void mtdisp_crtc_disable_vblank(struct drm_crtc *crtc)
{
	struct mtdisp_dispc *dispc = to_mtdisp_dispc(crtc);

	LINUX_DISPC_DEBUG("%s()\n", __func__);

	mtdisp_dispc_core_vsync_off(dispc->hwlib_dispc);
}

static int mtdisp_crtc_gamma_set(struct drm_crtc *crtc, u16 *red, u16 *green,
				 u16 *blue, uint32_t size,
				 struct drm_modeset_acquire_ctx *ctx)
{
	struct mtdisp_dispc *dispc = to_mtdisp_dispc(crtc);
	struct mtdisp_gamma_config config = {
		.r_data = red,
		.g_data = green,
		.b_data = blue,
		.size   = size,
	};

	mtdisp_dispc_core_gamma_set(dispc->hwlib_dispc, &config);

	return 0;
}

static const struct drm_crtc_funcs mtdisp_crtc_funcs = {
	.set_config             = drm_atomic_helper_set_config,
	.destroy                = mtdisp_crtc_destroy,
	.page_flip              = drm_atomic_helper_page_flip,
	.reset                  = drm_atomic_helper_crtc_reset,
	.atomic_duplicate_state = drm_atomic_helper_crtc_duplicate_state,
	.atomic_destroy_state   = drm_atomic_helper_crtc_destroy_state,
	.enable_vblank		= mtdisp_crtc_enable_vblank,
	.disable_vblank		= mtdisp_crtc_disable_vblank,
	.gamma_set		= mtdisp_crtc_gamma_set,
};

static void mtdisp_plane_create_properties(struct drm_plane *plane,
					   const struct mtdisp_layer_capability *layer_cap,
					   int index)
{
#if defined(OS_FUNC_DRM_PLANE_CREATE_BLEND_MODE_PROPERTY_EXIST)
	u32 supported_modes = BIT(DRM_MODE_BLEND_PIXEL_NONE) |
			      BIT(DRM_MODE_BLEND_PREMULTI) |
			      BIT(DRM_MODE_BLEND_COVERAGE);
	drm_plane_create_blend_mode_property(plane, supported_modes);
#endif

	drm_plane_create_rotation_property(plane,
					   DRM_MODE_ROTATE_0,
					   DRM_MODE_ROTATE_0);

	drm_plane_create_alpha_property(plane);

	drm_plane_create_zpos_immutable_property(plane, index);

	if (layer_cap->supported_encodings && layer_cap->supported_ranges)
		drm_plane_create_color_properties(plane,
						  layer_cap->supported_encodings,
						  layer_cap->supported_ranges,
						  DRM_COLOR_YCBCR_BT709,
						  DRM_COLOR_YCBCR_LIMITED_RANGE);
}

static void mtdisp_dispc_isr(void *data)
{
	struct mtdisp_dispc *dispc = data;
	u32 int_sts = mtdisp_dispc_core_isr(dispc->hwlib_dispc);

	if (int_sts & DISPC_INT_BIT_VSYNC) {
		if (dispc->capture_ctx && dispc->capture_ctx->fence_array) {
			 /*  Do not re-enter the work while waiting for the copy task to complete */
			if (!work_busy(dispc->capture_ctx->work))
				schedule_work(dispc->capture_ctx->work);
		} else {
			drm_crtc_handle_vblank(&dispc->crtc);
		}
	}

	if (int_sts & DISPC_INT_BIT_UNDERRUN)
		dispc->debugfs.underrun_cnt++;
}

static bool is_int_registed(struct mtdisp_display *display, int irq)
{
	struct device *dev = display->dispc[0].dev;
	struct mtgpu_device *mtdev;

	if (!dev || !dev->parent || !dev->parent->parent)
		return false;

	mtdev = dev_get_drvdata(dev->parent->parent);
	if (!mtdev || !mtdev->irq_info || !mtdev->irq_info->desc_table)
		return false;

	return mtdev->irq_info->desc_table[irq] ? true : false;
}

static bool is_int_enabled(struct mtdisp_display *display, int irq)
{
	struct device *dev = display->dispc[0].dev;
	struct mtgpu_device *mtdev;

	if (!dev || !dev->parent || !dev->parent->parent)
		return false;

	mtdev = dev_get_drvdata(dev->parent->parent);
	if (!mtdev || !mtdev->irq_info || !mtdev->irq_info->desc_table)
		return false;

	if (!mtdev->irq_info->desc_table[irq])
		return false;

	return mtdev->irq_info->desc_table[irq]->enabled;
}

int mtdisp_dispc_remove(struct mtdisp_display *display, struct device *dev)
{
	int i, irq;
	struct mtdisp_dispc *dispc;

	pr_info("%s()\n", __func__);

	for (i = 0; i < display->dispc_count; i++) {
		dispc = &display->dispc[i];
		irq = dispc->irq;

		if (dispc->crtc.primary)
			kfree(dispc->crtc.primary);

		if (dispc->crtc.cursor)
			kfree(dispc->crtc.cursor);

		if (is_int_registed(display, irq)) {
			if (is_int_enabled(display, irq))
				mtgpu_disable_interrupt(dev->parent->parent, irq);

			mtgpu_unregister_interrupt(dev->parent->parent, irq);
		}

		mtgpu_capture_context_destroy(dispc->capture_ctx);

		mtdisp_dispc_debugfs_remove_files(dispc);
	}

	return 0;
}

int mtdisp_dispc_probe(struct mtdisp_display *display,
		       struct device *dev, struct drm_device *drm)
{
	int i, ly, ret;
	struct drm_plane *primary = NULL, *cursor = NULL;
	struct mtdisp_dispc *dispc;
	struct mtgpu_capture_context *capture_ctx;
	u32 gamma_size;

	pr_info("%s()\n", __func__);

	for (i = 0; i < display->dispc_count; i++) {
		dispc = &display->dispc[i];

		dispc->dev = dev;

		ret = mtgpu_register_interrupt(dev->parent->parent, dispc->irq,
					       mtdisp_dispc_isr, dispc, "dispc");
		if (ret) {
			LINUX_DISPC_ERROR("failed to register dispc irq handler\n");
			ret = -EINVAL;
			goto err_dispc_probe;
		}

		/* disable de configs like interrupts as firmware may config them,
		 * which may lead OOPS in irq handler as drm has not finished
		 * initial.
		 */
		mtdisp_dispc_core_deinit(dispc->hwlib_dispc);

		ret = mtgpu_enable_interrupt(dev->parent->parent, dispc->irq);
		if (ret) {
			LINUX_DISPC_ERROR("failed to enable dispc irq number: %d\n",
					(int)dispc->irq);
			ret = -EINVAL;
			goto err_dispc_probe;
		}

		mtdisp_dispc_glb_init(dispc->hwlib_dispc);

		for (ly = 0; ly < dispc->layer_count; ly++) {
			struct drm_plane *plane;

			plane = kzalloc(sizeof(*plane), GFP_KERNEL);
			if (!plane) {
				ret = -ENOMEM;
				LINUX_DISPC_ERROR("failed to create plane\n");
				goto err_dispc_probe;
			}

			ret = drm_universal_plane_init(drm, plane, (1 << dispc->id),
						       &mtdisp_plane_funcs,
						       dispc->layer_caps[ly].fmts_ptr,
						       dispc->layer_caps[ly].fmts_cnt,
						       dispc->layer_caps[ly].modifier,
						       dispc->layer_caps[ly].type,
						       NULL);
			if (ret) {
				LINUX_DISPC_ERROR("failed to init plane\n");
				goto err_dispc_probe;
			}

			drm_plane_helper_add(plane, &mtdisp_primary_helper_funcs);

			mtdisp_plane_create_properties(plane, &dispc->layer_caps[ly], ly);

			if (ly == 0)
				primary = plane;
			else if (dispc->layer_caps[ly].type == DRM_PLANE_TYPE_CURSOR)
				cursor = plane;
		}

		ret = drm_crtc_init_with_planes(drm, &dispc->crtc, primary, cursor,
						&mtdisp_crtc_funcs, NULL);
		if (ret) {
			LINUX_DISPC_ERROR("failed to init CRTC\n");
			goto err_dispc_probe;
		}

		gamma_size = mtdisp_get_dispc_ctx_gamma_size(dispc->hwlib_dispc);
		ret = drm_mode_crtc_set_gamma_size(&dispc->crtc, gamma_size);
		if (ret) {
			LINUX_DISPC_ERROR("failed to set gamma size\n");
			goto err_dispc_probe;
		}

		mtdisp_set_dispc_ctx_gamma_store(dispc->hwlib_dispc, dispc->crtc.gamma_store);
		mtdisp_set_dispc_ctx_gamma_size(dispc->hwlib_dispc, dispc->crtc.gamma_size);

		drm_crtc_enable_color_mgmt(&dispc->crtc, 0, false, gamma_size);

		drm_crtc_helper_add(&dispc->crtc, &mtdisp_crtc_helper_funcs);

		mtdisp_dispc_debugfs_create_files(dispc);

		ret = mtgpu_capture_context_create(dev, &dispc->crtc, &capture_ctx);
		if (ret)
			LINUX_DISPC_ERROR("failed to init capture ctx\n");
		else
			dispc->capture_ctx = capture_ctx;

		LINUX_DISPC_INFO("mtdisp display controller driver loaded successfully\n");
	}

	return 0;

err_dispc_probe:
	mtdisp_dispc_remove(display, dev);

	return ret;
}

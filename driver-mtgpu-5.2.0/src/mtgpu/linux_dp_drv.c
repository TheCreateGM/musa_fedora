/*
 * @Copyright   Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License     Dual MIT/GPLv2
 */

#include <linux/component.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/module.h>
#include <sound/hdmi-codec.h>
#if defined(OS_DRM_DRMP_H_EXIST)
#include <drm/drmP.h>
#endif
#include <drm/drm_atomic_helper.h>
#if defined(OS_DRM_DRM_DP_HELPER_H_EXIST)
#include <drm/drm_dp_helper.h>
#elif defined(OS_DRM_DP_DRM_DP_HELPER_H_EXIST)
#include <drm/dp/drm_dp_helper.h>
#elif defined(OS_DRM_DISPLAY_DRM_DP_HELPER_H_EXIST)
#include <drm/display/drm_dp_helper.h>
#endif
#include <drm/drm_edid.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_print.h>
#if defined(OS_DRM_DRM_PROBE_HELPER_H_EXIST)
#include <drm/drm_probe_helper.h>
#endif
#include <drm/drm_device.h>
#include <drm/drm_file.h>
#include <video/videomode.h>
#include <drm/drm_drv.h>

#include "mtgpu_irq.h"
#include "mtgpu_board_cfg.h"
#include "mtgpu_module_param.h"
#include "module_common.h"
#include "mtgpu_drv.h"

#include "dp_types.h"
#include "mtdisp_phy.h"
#include "mtdisp_dp.h"

#include "dm_hwlib_access.h"

#include "linux_display_debugfs.h"
#include "linux_data_convert.h"
#include "linux_display_drv.h"
#include "linux_dispc_drv.h"
#include "linux_dp_mst_drv.h"
#include "linux_dp_drv.h"

extern bool mst_enable;

static const struct monitor_patch monitor_patch_list[] = {
	{PLAT_GEN2 | PLAT_GEN3,
	 "AOC", 0x0000, PATCH_FORCE_TRAINING, {}, {}},
	{PLAT_GEN1,
	 "SAM", 0x7177, PATCH_SKIP_TPS4_FOR_TRAINING, {}, {}},
	{PLAT_GEN2,
	 "PHL", 0xC26A, PATCH_FORCE_OVERWRITE_TIMING,
	 { 1075000, 3840, 3848, 3880, 3920, 2160, 2273, 2281, 2287 },
	 { 1090000, 3840, 3848, 3880, 3920, 2160, 2273, 2313, 2319 }}, /* 4K119.91Hz */
	{PLAT_GEN2 | PLAT_GEN3,
	 "PHL", 0xC26A, PATCH_FORCE_OVERWRITE_TIMING,
	 { 1066610, 3840, 3888, 3952, 4000, 2160, 2163, 2168, 2222 },
	 { 1188000, 3840, 4016, 4104, 4400, 2160, 2168, 2178, 2250 }}, /* 4K120Hz */
	{PLAT_GEN2 | PLAT_GEN3,
	 "PHL", 0xC26A, PATCH_FORCE_OVERWRITE_TIMING,
	 { 1304900, 3840, 3848, 3880, 3920, 2160, 2300, 2308, 2314 },
	 { 1291000, 3840, 3848, 3880, 3920, 2160, 2273, 2281, 2287 }}, /* 4K144Hz */
	{PLAT_GEN1,
	 "HKC", 0x2732, PATCH_FORCE_OVERWRITE_TIMING,
	 {  858240, 3840, 3888, 3920, 4000, 2160, 2163, 2168, 2259 },
	 {  858250, 3840, 3888, 3920, 4000, 2160, 2163, 2168, 2259 }}, /* 4K94.98Hz */
	{PLAT_GEN2,
	 "TRG", 0x6524, PATCH_FORCE_OVERWRITE_TIMING,
	 {  138500, 1920, 1968, 2000, 2080, 1080, 1082, 1087, 1111 },
	 {  148500, 1920, 2008, 2052, 2200, 1080, 1084, 1089, 1125 }}, /* 1080p60Hz */
	{PLAT_GEN2,
	 "SUM", 0x00EE, PATCH_FORCE_OVERWRITE_TIMING,
	 {  141750, 1920, 2008, 2052, 2100, 1080, 1084, 1089, 1125 },
	 {  148500, 1920, 2008, 2052, 2200, 1080, 1084, 1089, 1125 }}, /* 1080p60Hz */
	{PLAT_GEN2,
	 "KKA", 0x2224, PATCH_FORCE_OVERWRITE_TIMING,
	 {  138500, 1920, 1968, 2000, 2080, 1080, 1082, 1087, 1111 },
	 {  148500, 1920, 2008, 2052, 2200, 1080, 1084, 1089, 1125 }}, /* suma: 1080p60Hz */
	{PLAT_GEN1,
	 "BNQ", 0x78E7, PATCH_FORCE_OVERWRITE_TIMING,
	 {  108000, 1600, 1624, 1704, 1800,  900,  901,  904, 1000 },
	 {  118250, 1600, 1696, 1856, 2112,  900,  903,  908,  934 }}, /* BenQ GW2480:
									  1600x900@59.95Hz */
};

static void
mtdisp_drm_get_edid_vendor(const struct edid *edid, char *edid_vendor)
{
	edid_vendor[0] = ((edid->mfg_id[0] & 0x7c) >> 2) + '@';
	edid_vendor[1] = (((edid->mfg_id[0] & 0x3) << 3) |
			  ((edid->mfg_id[1] & 0xe0) >> 5)) + '@';
	edid_vendor[2] = (edid->mfg_id[1] & 0x1f) + '@';
	edid_vendor[3] = '\0';
}

static bool
mtdisp_display_mode_compare(const struct drm_display_mode *mode,
			    struct mt_display_mode *mt_mode)
{
	if (mode->clock		== mt_mode->clock &&
	    mode->hdisplay	== mt_mode->hdisplay &&
	    mode->hsync_start	== mt_mode->hsync_start &&
	    mode->hsync_end	== mt_mode->hsync_end &&
	    mode->htotal	== mt_mode->htotal &&
	    mode->vdisplay	== mt_mode->vdisplay &&
	    mode->vsync_start	== mt_mode->vsync_start &&
	    mode->vsync_end	== mt_mode->vsync_end &&
	    mode->vtotal	== mt_mode->vtotal)
		return true;

	return false;
}

static void
mtdisp_display_mode_copy(struct drm_display_mode *adjusted_mode,
			 struct mt_display_mode *patch_mode)
{
	adjusted_mode->clock		= patch_mode->clock;
	adjusted_mode->hdisplay		= patch_mode->hdisplay;
	adjusted_mode->hsync_start	= patch_mode->hsync_start;
	adjusted_mode->hsync_end	= patch_mode->hsync_end;
	adjusted_mode->htotal		= patch_mode->htotal;
	adjusted_mode->vdisplay		= patch_mode->vdisplay;
	adjusted_mode->vsync_start	= patch_mode->vsync_start;
	adjusted_mode->vsync_end	= patch_mode->vsync_end;
	adjusted_mode->vtotal		= patch_mode->vtotal;
}

static struct monitor_patch *
mtdisp_dp_monitor_select_patch(struct mtdisp_dp *dp,
			       const struct drm_display_mode *mode,
			       u32 patch_type)
{
	u32 i;
	u8  vendor[4];
	u32 product_id;
	u32 soc_gen = PLAT_INVALID;

	u8 size = ARRAY_SIZE(monitor_patch_list);
	struct monitor_patch *list = (struct monitor_patch *)monitor_patch_list;

	if (!dp->edid)
		return NULL;

	switch (mtdisp_get_dp_ctx_gen(dp->hwlib_dp)) {
	case MTDISP_SOC_GEN1:
		soc_gen = PLAT_GEN1;
		break;
	case MTDISP_SOC_GEN2:
		soc_gen = PLAT_GEN2;
		break;
	case MTDISP_SOC_GEN3:
		soc_gen = PLAT_GEN3;
		break;
	case MTDISP_SOC_GEN4:
		soc_gen = PLAT_GEN4;
		break;
	}

	mtdisp_drm_get_edid_vendor(dp->edid, vendor);
	product_id = EDID_PRODUCT_ID(dp->edid);

	for (i = 0; i < size; i++) {
		/* gen, vendor, product id */
		if ((list[i].gen & soc_gen) == 0 ||
		    strcmp(vendor, list[i].vendor) ||
		    (list[i].product_id && product_id != list[i].product_id))
			continue;

		switch (patch_type) {
		case PATCH_FORCE_TRAINING:
		case PATCH_SKIP_TPS4_FOR_TRAINING:
			if (list[i].patch_type == patch_type)
				goto found_patch;
			break;
		case PATCH_FORCE_OVERWRITE_TIMING:
			if (!mode)
				return NULL;
			if (mtdisp_display_mode_compare(mode, &list[i].original_mode))
				goto found_patch;
			break;
		default:
			return NULL;
		}
	}

	return NULL;

found_patch:
	return &list[i];
}

static inline
struct mtdisp_dp *encoder_to_mtdisp_dp(struct drm_encoder *encoder)
{
	return container_of(encoder, struct mtdisp_dp, encoder);
}

static inline
struct mtdisp_dp *connector_to_mtdisp_dp(struct drm_connector *connector)
{
	return container_of(connector, struct mtdisp_dp, connector);
}

static inline
struct mtdisp_dp *aux_to_mtdisp_dp(struct drm_dp_aux *aux)
{
	return container_of(aux, struct mtdisp_dp, aux);
}

static inline
struct mtdisp_dp *dwork_to_mtdisp_dp(struct work_struct *dwork)
{
	return container_of(dwork, struct mtdisp_dp, hpd_work.work);
}

static inline
struct mtdisp_dp *work_to_mtdisp_dp(struct work_struct *work)
{
	return container_of(work, struct mtdisp_dp, hpd_irq_work);
}

static const u8 display_port_type[] = {
	DRM_MODE_CONNECTOR_Unknown,	/*PORT_DISABLED*/
	DRM_MODE_CONNECTOR_DisplayPort,	/*PORT_TYPE_DP*/
	DRM_MODE_CONNECTOR_eDP,		/*PORT_TYPE_EDP*/
	DRM_MODE_CONNECTOR_VGA,		/*PORT_TYPE_DP2VGA*/
	DRM_MODE_CONNECTOR_HDMIA,	/*PORT_TYPE_DP2HDMI*/
	DRM_MODE_CONNECTOR_LVDS		/*PORT_TYPE_DP2LVDS*/
};

/*
 * DP audio codec callbacks
 */
static int mtdisp_dp_audio_hw_params(struct device *dev, void *data,
				     struct hdmi_codec_daifmt *daifmt,
				     struct hdmi_codec_params *params)
{
	struct mtdisp_dp *dp = data;
	u8 bw_code = mtdisp_get_dp_ctx_bw_code(dp->hwlib_dp);
	bool enabled = mtdisp_get_dp_enabled(dp->hwlib_dp);
	struct mtdisp_codec_params codec_param;

	if (!params) {
		LINUX_DP_ERROR("%s: audio params is NULL.\n", __func__);
		return -EINVAL;
	}

	if (!enabled || !bw_code) {
		LINUX_DP_INFO("%s: dp stream has not enabled.\n", __func__);
		return -EPERM;
	}

	LINUX_DP_INFO("\n");

	codec_param.sample_rate = params->sample_rate;
	codec_param.sample_width = params->sample_width;
	codec_param.channels = params->channels;

	LINUX_DP_DEBUG("%u Hz, %d bit, %d channels\n",
		       codec_param.sample_rate,
		       codec_param.sample_width,
		       codec_param.channels);

	return mtdisp_dp_core_audio_set_param(dp->hwlib_dp, &codec_param);
}

static int mtdisp_dp_audio_startup(struct device *dev, void *data)
{
	struct mtdisp_dp *dp = data;

	LINUX_DP_TRACE();

	mtdisp_dp_core_audio_enable(dp->hwlib_dp);

	dp->audio_enabled = true;

	return 0;
}

static void mtdisp_dp_audio_shutdown(struct device *dev, void *data)
{
	struct mtdisp_dp *dp = data;

	LINUX_DP_TRACE();

	mtdisp_dp_core_audio_disable(dp->hwlib_dp);

	dp->audio_enabled = false;
}

#if defined(OS_STRUCT_HDMI_CODEC_OPS_HAS_MUTE_STREAM)
static int mtdisp_dp_set_audio_mute(struct device *dev, void *data, bool enable, int direction)
#else
static int mtdisp_dp_set_audio_mute(struct device *dev, void *data, bool enable)
#endif
{
	struct mtdisp_dp *dp = data;

	LINUX_DP_INFO("\n");

	mtdisp_dp_core_audio_mute(dp->hwlib_dp, enable);

	return 0;
}

static int mtdisp_dp_audio_get_eld(struct device *dev, void *data, uint8_t *buf, size_t len)
{
	struct mtdisp_dp *dp = data;

	LINUX_DP_INFO("\n");

	memcpy(buf, dp->connector.eld, min(sizeof(dp->connector.eld), len));

	return 0;
}

static int mtdisp_dp_audio_hook_plugged_cb(struct device *dev, void *data,
					    hdmi_codec_plugged_cb fn,
					    struct device *codec_dev)
{
	struct mtdisp_dp *dp = data;
	bool enabled = mtdisp_get_dp_enabled(dp->hwlib_dp);

	LINUX_DP_INFO("\n");

	mutex_lock(&dp->codec_lock);
	dp->codec_plugged_cb = fn;
	dp->codec_dev = codec_dev;
	mutex_unlock(&dp->codec_lock);

	if (dp->codec_plugged_cb && dp->codec_dev)
		dp->codec_plugged_cb(dp->codec_dev, enabled);

	return 0;
}

#if defined(OS_STRUCT_HDMI_CODEC_OPS_HAS_HOOK_PLUGGED_CB)
static const struct hdmi_codec_ops mtdisp_dp_audio_codec_ops = {
	.hw_params = mtdisp_dp_audio_hw_params,
	.audio_startup = mtdisp_dp_audio_startup,
	.audio_shutdown = mtdisp_dp_audio_shutdown,
#if defined(OS_STRUCT_HDMI_CODEC_OPS_HAS_MUTE_STREAM)
	.mute_stream = mtdisp_dp_set_audio_mute,
#else
	.digital_mute = mtdisp_dp_set_audio_mute,
#endif
	.get_eld = mtdisp_dp_audio_get_eld,
	.hook_plugged_cb = mtdisp_dp_audio_hook_plugged_cb,
};
#else
typedef int (*hook_plugged_callback)(struct device *dev, void *data,
					    hdmi_codec_plugged_cb fn,
					    struct device *codec_dev);

struct mtdisp_codec_ops {
	struct hdmi_codec_ops old_dp_audio_codec_ops;
	hook_plugged_callback hook_plugged_cb;
};

static const struct mtdisp_codec_ops mtdisp_dp_audio_codec_ops = {
	.old_dp_audio_codec_ops = {
		.hw_params = mtdisp_dp_audio_hw_params,
		.audio_startup = mtdisp_dp_audio_startup,
		.audio_shutdown = mtdisp_dp_audio_shutdown,
		.digital_mute = mtdisp_dp_set_audio_mute,
		.get_eld = mtdisp_dp_audio_get_eld,
	},
	.hook_plugged_cb = mtdisp_dp_audio_hook_plugged_cb,
};
#endif

static int mtdisp_dp_audio_register(struct mtdisp_dp *dp, struct drm_device *drm)
{
	struct hdmi_codec_pdata codec_data = {
		.ops = (struct hdmi_codec_ops *)&mtdisp_dp_audio_codec_ops,
		.max_i2s_channels = 2,
		.i2s = 1,
		.data = dp,
	};
	int idx = mtgpu_cnt - 1; // idx starts from 0
	char dev_name[64];
	const char *name = dev_name;

	snprintf(dev_name, sizeof(dev_name), "mtgpu-%d-dp-audio-codec-%d", idx, dp->id);
	dp->dp_audio = platform_device_register_data(dp->dev, name,
						     PLATFORM_DEVID_AUTO, &codec_data,
						     sizeof(codec_data));
	if (IS_ERR(dp->dp_audio)) {
		LINUX_DP_ERROR("failed to register audio codec '%s'.\n", dev_name);
		return PTR_ERR(dp->dp_audio);
	}

	LINUX_DP_INFO("audio codec '%s' registered successfully.\n", dev_name);

	mutex_init(&dp->codec_lock);

	return 0;
}

static void mtdisp_dp_audio_unregister(struct mtdisp_dp *dp)
{
	if (dp->dp_audio) {
		platform_device_unregister(dp->dp_audio);
		dp->dp_audio = NULL;
	}
}

void mtdisp_dp_encoder_enable(struct drm_encoder *encoder)
{
	struct mtdisp_dp *dp = encoder_to_mtdisp_dp(encoder);
	struct drm_crtc *crtc = encoder->crtc;
	struct mtdisp_dispc *dispc = container_of(crtc, struct mtdisp_dispc, crtc);
	void *mt_dsc_config;
	struct mtdisp_videomode mt_vm;
	u32 color_depth, range_type;

	if (!mutex_trylock(&dp->lock))
		return;

	LINUX_DP_INFO("\n");

	mt_dsc_config = mtdisp_get_dispc_dsc_config(dispc->hwlib_dispc);

	mtdisp_get_dp_ctx_vm(&mt_vm, dp->hwlib_dp, 0);
	color_depth = 8;
	range_type = OUTPUT_RANGE_SDR;

	mtdisp_dp_pre_enable(dp->hwlib_dp, mt_dsc_config,
			     mt_vm.pixelclock, color_depth, range_type);

	mtdisp_dp_enable(dp->hwlib_dp);

	if (dp->codec_plugged_cb && dp->codec_dev)
		dp->codec_plugged_cb(dp->codec_dev, true);

	mutex_unlock(&dp->lock);
}

void mtdisp_dp_encoder_disable(struct drm_encoder *encoder)
{
	struct mtdisp_dp *dp = encoder_to_mtdisp_dp(encoder);

	LINUX_DP_INFO("\n");

	if (dp->codec_plugged_cb && dp->codec_dev)
		dp->codec_plugged_cb(dp->codec_dev, false);

	mtdisp_dp_disable(dp->hwlib_dp);
}

static void
mtdisp_dp_encoder_atomic_mode_set(struct drm_encoder *encoder,
				  struct drm_crtc_state *crtc_state,
				  struct drm_connector_state *connector_state)
{
	struct mtdisp_dp *dp = encoder_to_mtdisp_dp(encoder);
	struct drm_display_mode *adjusted_mode = &crtc_state->adjusted_mode;
	struct videomode vm;
	struct mtdisp_videomode mt_vm;

	LINUX_DP_TRACE();

	drm_display_mode_to_videomode(adjusted_mode, &vm);
	mtdisp_videomode_from_os_videomode(&mt_vm, &vm);
	mtdisp_set_dp_ctx_vm(dp->hwlib_dp, &mt_vm, 0);

	return;
}

static bool
mtdisp_dp_encoder_mode_fixup(struct drm_encoder *encoder,
			    const struct drm_display_mode *mode,
			    struct drm_display_mode *adjusted_mode)
{
	struct mtdisp_dp *dp = encoder_to_mtdisp_dp(encoder);
	struct monitor_patch *patch;

	LINUX_DP_TRACE();

	patch = mtdisp_dp_monitor_select_patch(dp, mode, PATCH_FORCE_OVERWRITE_TIMING);
	if (!patch)
		return true;

	mtdisp_display_mode_copy(adjusted_mode, &patch->patched_mode);
	drm_mode_set_crtcinfo(adjusted_mode, 0);

	LINUX_DP_DEBUG( DRM_MODE_FMT "\n", DRM_MODE_ARG(adjusted_mode));

	return true;
}

static const struct drm_encoder_helper_funcs mtdisp_dp_encoder_helper_funcs = {
	.enable			= mtdisp_dp_encoder_enable,
	.disable		= mtdisp_dp_encoder_disable,
	.atomic_mode_set	= mtdisp_dp_encoder_atomic_mode_set,
	.mode_fixup		= mtdisp_dp_encoder_mode_fixup,
};

static const struct drm_encoder_funcs mtdisp_dp_encoder_funcs = {
	.destroy = drm_encoder_cleanup,
};

static enum drm_connector_status
mtdisp_dp_connector_detect(struct drm_connector *connector, bool force)
{
	struct mtdisp_dp *dp = connector_to_mtdisp_dp(connector);
	u16 retries = 100;
	int connected;

	connected = mtdisp_dp_core_is_plugin(dp->hwlib_dp);
	if (connected < 0) {
		LINUX_DP_ERROR("hotplug detect callback is not implemented\n");
		return connector_status_disconnected;
	}

	dp->connected = connected;
	pr_info("DP-%d: HPD state is %d\n", dp->id, dp->connected);

	if (dp->connected) {
		bool src_support_mst = false;
		bool sink_support_mst = false;

		if (mtdisp_dp_get_sinkcaps(dp->hwlib_dp) < 0)
			return connector_status_disconnected;

		if (mtdisp_dp_dsc_get_sinkcaps(dp->hwlib_dp) < 0)
			return connector_status_disconnected;

		src_support_mst = mtdisp_get_dp_src_support_mst(dp->hwlib_dp);
		sink_support_mst = mtdisp_dp_is_mst_support(dp->hwlib_dp);

		LINUX_DP_INFO("src_support_mst = %d, sink_support_mst = %d\n",
			      src_support_mst, sink_support_mst);

		/* mst support */
		if (mst_enable && src_support_mst && sink_support_mst)
			if (mtdisp_dp_mst_plug_in(dp) == 0)
				return connector_status_disconnected;
	} else {
		if (mst_enable && mtdisp_get_dp_ctx_mst_en(dp->hwlib_dp))
			mtdisp_dp_mst_unplug(dp);
	}

	if (dp->connected && !dp->edid) {
		do {
			dp->edid = drm_get_edid(connector, &dp->aux.ddc);
			usleep_range(2000, 3000);
			retries--;
		} while (!dp->edid && retries);

		if (!dp->edid) {
			LINUX_DP_ERROR("failed to get edid\n");
			drm_connector_update_edid_property(connector, NULL);
		} else {
			drm_connector_update_edid_property(connector, dp->edid);
		}
	} else if (!dp->connected && dp->edid) {
		kfree(dp->edid);
		dp->edid = NULL;
	}

	return dp->connected ? connector_status_connected : connector_status_disconnected;
}

static const struct drm_connector_funcs mtdisp_dp_connector_funcs = {
	.detect			= mtdisp_dp_connector_detect,
	.fill_modes		= drm_helper_probe_single_connector_modes,
	.destroy		= drm_connector_cleanup,
	.atomic_duplicate_state	= drm_atomic_helper_connector_duplicate_state,
	.atomic_destroy_state	= drm_atomic_helper_connector_destroy_state,
	.reset			= drm_atomic_helper_connector_reset,
};

static int mtdisp_dp_connector_get_modes(struct drm_connector *connector)
{
	struct mtdisp_dp *dp = connector_to_mtdisp_dp(connector);

	int ret;

	LINUX_DP_TRACE();

	if (!dp->edid) {
		LINUX_DP_ERROR("no edid\n");

		drm_connector_update_edid_property(connector, NULL);

		return -ENODEV;
	}

	ret = drm_add_edid_modes(connector, dp->edid);

	return ret;
}

static struct drm_encoder *
mtdisp_dp_connector_best_encoder(struct drm_connector *connector)
{
	struct mtdisp_dp *dp = connector_to_mtdisp_dp(connector);

	LINUX_DP_TRACE();

	return &dp->encoder;
}

static int mtdisp_dp_mode_supported_by_product(struct mtdisp_dp *dp,
					       struct drm_display_mode *mode)
{
	u32 dp_max_pclk;
	u32 max_hres = mtdisp_get_dp_src_dsc_caps_max_hres(dp->hwlib_dp);
	u32 max_vres = mtdisp_get_dp_src_dsc_caps_max_vres(dp->hwlib_dp);
	u32 max_pclk_100khz = mtdisp_get_dp_max_pclk(dp->hwlib_dp);
	u32 max_rate = mtdisp_get_dp_ctx_bw_code(dp->hwlib_dp) * 27000;
	u8 lane_cnt = mtdisp_get_dp_ctx_lane_cnt(dp->hwlib_dp);
	bool both_dsc_support = mtdisp_get_dp_sink_support_dsc(dp->hwlib_dp) &&
				mtdisp_get_dp_src_support_dsc(dp->hwlib_dp);

	if (!max_hres || !max_vres)
		LINUX_DP_ERROR("incorrect parameters for max resolution\n");

	if (mode->hdisplay > max_hres)
		return MODE_H_ILLEGAL;

	if (mode->vdisplay > max_vres)
		return MODE_V_ILLEGAL;

	if (mode->clock > max_pclk_100khz * 100)
		return MODE_CLOCK_HIGH;

	dp_max_pclk = mtdisp_dp_link_rate_to_pclk(max_rate, lane_cnt, 24);

	dp_max_pclk = both_dsc_support ? dp_max_pclk * 3 : dp_max_pclk;

	if (mode->clock > dp_max_pclk)
		return MODE_CLOCK_HIGH;

	return MODE_OK;
}

static int mtdisp_dp_mode_compare(struct drm_display_mode *a,
				  struct drm_display_mode *b)
{
	int diff;

	diff = b->hdisplay * b->vdisplay - a->hdisplay * a->vdisplay;
	if (diff)
		return diff;

	diff = drm_mode_vrefresh(b) - drm_mode_vrefresh(a);
	if (diff)
		return diff;

	diff = b->clock - a->clock;
	return diff;
}

static void mtdisp_dp_set_preferred_mode(struct drm_connector *connector,
					 struct drm_display_mode *filtered_mode)
{
	struct drm_display_mode *mode;
	struct drm_display_mode tmp_best_mode = {0};
	struct drm_display_mode *preferred_mode = NULL;
	struct mtdisp_dp *dp = connector_to_mtdisp_dp(connector);

	if (list_empty(&connector->modes))
		return;

	/* Find the best mode and set its type preferred. */
	list_for_each_entry(mode, &connector->modes, head) {
		if ((mtdisp_dp_mode_supported_by_product(dp, mode) != MODE_OK) ||
		    mode->status != MODE_OK ||
		    drm_mode_equal(mode, filtered_mode))
			continue;

		if (mtdisp_dp_mode_compare(&tmp_best_mode, mode) > 0) {
			memcpy(&tmp_best_mode, mode, sizeof(struct drm_display_mode));
			preferred_mode = mode;
		}
	}

	if (preferred_mode)
		preferred_mode->type |= DRM_MODE_TYPE_PREFERRED;
}

#ifdef OS_DRM_CONNECTOR_MODE_VALID_CONST
static enum drm_mode_status
mtdisp_dp_connector_mode_valid(struct drm_connector *connector,
			       const struct drm_display_mode *mode)
#else
static int mtdisp_dp_connector_mode_valid(struct drm_connector *connector,
					  struct drm_display_mode *mode)
#endif
{
	int mode_status;
	struct mtdisp_dp *dp = connector_to_mtdisp_dp(connector);

	LINUX_DP_TRACE();

	/* check if the mode can be supported by product. */
	mode_status = mtdisp_dp_mode_supported_by_product(dp, mode);
	if (mode_status != MODE_OK) {
		if (mode->type & DRM_MODE_TYPE_PREFERRED) {
			/*
			 * When the preferred mode is unsupported,
			 * we should set the remaining best mode preferred.
			 */
			mtdisp_dp_set_preferred_mode(connector, mode);
		}

		return mode_status;
	}

	return mode_status;
}

static const struct drm_connector_helper_funcs mtdisp_dp_connector_helper_funcs = {
	.get_modes	= mtdisp_dp_connector_get_modes,
	.best_encoder	= mtdisp_dp_connector_best_encoder,
	.mode_valid	= mtdisp_dp_connector_mode_valid,
};

static void mtdisp_dp_handle_automated_test(struct mtdisp_dp *dp)
{
	mtdisp_dp_automated_test(dp->hwlib_dp);
}

static ssize_t
mtdisp_dp_aux_transfer(struct drm_dp_aux *aux, struct drm_dp_aux_msg *msg)
{
	int ret = 0;
	struct mtdisp_dp *dp = aux_to_mtdisp_dp(aux);

	/* FIXME: We should check the msg->request first! */

	if (!dp->connected)
		return -EIO;

	if (msg->request & BIT(0))
		ret = mtdisp_dp_core_aux_read(dp->hwlib_dp, msg->request, msg->address,
					      msg->buffer, msg->size, &msg->reply);
	else
		ret = mtdisp_dp_core_aux_write(dp->hwlib_dp, msg->request, msg->address,
					       msg->buffer, msg->size, &msg->reply);

	if (ret) {
		LINUX_DP_INFO("WARNING: try to do aux transfer (%d)\n", ret);
		mdelay(2);
		return ret;
	}

	return msg->size;
}

static int mtdisp_dp_drm_init(struct mtdisp_dp *dp, struct drm_device *drm)
{
	struct drm_encoder *encoder = &dp->encoder;
	struct drm_connector *connector = &dp->connector;
	int ret;

	dp->aux.name = "MTGPU DP AUX";
	dp->aux.dev = dp->dev;
	dp->aux.transfer = mtdisp_dp_aux_transfer;
#if defined(OS_STRUCT_DRM_DP_AUX_HAS_DRM_DEV)
	dp->aux.drm_dev = drm;
#endif

	ret = drm_dp_aux_register(&dp->aux);
	if (ret) {
		LINUX_DP_ERROR("failed to initialize DP aux\n");
		return ret;
	}

	/*
	* WARNING:
	* The possible_crtcs mask is in the logical order of dispc platform device
	* registration, not the dispc hardware ID order. So if the dispc platform
	* device count changed, this possible_crtcs mask should also be changed.
	*/
	encoder->possible_crtcs = 1 << dp->id;
	ret = drm_encoder_init(drm, encoder, &mtdisp_dp_encoder_funcs,
			       DRM_MODE_ENCODER_TMDS, NULL);
	if (ret) {
		LINUX_DP_ERROR("failed to create DisplayPort encoder\n");
		return ret;
	}

	drm_encoder_helper_add(encoder, &mtdisp_dp_encoder_helper_funcs);

	connector->polled = DRM_CONNECTOR_POLL_HPD;

	if (dp->port_type >= ARRAY_SIZE(display_port_type)) {
		LINUX_DP_INFO("WARNING: invalid port_type 0x%x, use DisplayPort type as default\n",
			      dp->port_type);
		dp->port_type = PORT_TYPE_DP;
	}

#if defined(OS_FUNC_DRM_CONNECTOR_INIT_WITH_DDC_EXIST)
	ret = drm_connector_init_with_ddc(drm, connector,
					  &mtdisp_dp_connector_funcs,
					  display_port_type[dp->port_type],
					  &dp->aux.ddc);
#else
	ret = drm_connector_init(drm, connector,
				 &mtdisp_dp_connector_funcs,
				 display_port_type[dp->port_type]);
#endif
	if (ret) {
		LINUX_DP_ERROR("failed to create DisplayPort connector\n");
		return ret;
	}

	drm_connector_helper_add(connector, &mtdisp_dp_connector_helper_funcs);
	drm_connector_register(connector);
	drm_connector_attach_encoder(connector, encoder);

	return 0;
}

static bool mtdisp_dp_is_link_loss(struct mtdisp_dp *dp)
{
	int i, ret = 0;
	bool link_loss = false;
	u8 status[DP_LINK_STATUS_SIZE];
	bool enabled = mtdisp_get_dp_enabled(dp->hwlib_dp);
	u8 lane_cnt = mtdisp_get_dp_ctx_lane_cnt(dp->hwlib_dp);

	ret = drm_dp_dpcd_read(&dp->aux, DP_SINK_COUNT, status, DP_LINK_STATUS_SIZE);
	if (ret < 0)
		return false;

	LINUX_DP_INFO("link status dpcd[0x200-0x205]: 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n",
		      status[0], status[1], status[2], status[3], status[4], status[5]);

	for (i = 0; i < lane_cnt; i++) {
		if ((status[2 + i / 2] & (DP_CHANNEL_EQ_BITS << ((i % 2) << 2))) !=
		    (DP_CHANNEL_EQ_BITS << ((i % 2) << 2)))
			link_loss = true;
	}

	if (enabled && ((!(status[4] & DP_INTERLANE_ALIGN_DONE)) || link_loss))
		return true;

	return false;
}

static void mtdisp_dp_hpd_work(struct work_struct *work)
{
	int old_status, status;
	struct mtdisp_dp *dp = dwork_to_mtdisp_dp(work);
	bool dp_enabled = mtdisp_get_dp_enabled(dp->hwlib_dp);
	bool sink_support_dsc = mtdisp_get_dp_sink_support_dsc(dp->hwlib_dp);
	bool dsc_caps_cheanged = false;
	u32 gen = mtdisp_get_dp_ctx_gen(dp->hwlib_dp);
	u8 mst_parent_id = mtdisp_get_dp_mst_parent_id(dp->hwlib_dp);

	old_status = dp->connected ? connector_status_connected : connector_status_disconnected;

	/*
	 * If a dp monitor is plugged into the dp port (unsupported mst),
	 * disable mst and it will be re-enabled in connector_detect.
	 * Only sudi & quyuan1 needs to do.
	 */
	if (gen == MTDISP_SOC_GEN1 || gen == MTDISP_SOC_GEN2) {
		bool old_connected = dp->connected;
		bool new_connected = mtdisp_dp_core_is_plugin(dp->hwlib_dp) == 1;
		bool plugin_action = !old_connected && new_connected;
		struct mtdisp_dp *dp_mst_parent = dp->dp_mst_parent;

		if (dp->id != mst_parent_id && plugin_action && dp_mst_parent) {
			if (mtdisp_dp_is_mst_support(dp_mst_parent->hwlib_dp) &&
			    mtdisp_get_dp_ctx_mst_en(dp_mst_parent->hwlib_dp)) {
				LINUX_DP_INFO("disable mst on dp_mst_parent due to plugin on dp%d\n",
					      dp->id);
				mtdisp_set_dp_ctx_mst_en(dp_mst_parent->hwlib_dp, false);
				drm_dp_mst_topology_mgr_set_mst(&dp_mst_parent->mst_mgr, false);
			}
		}
	}

	status = drm_helper_probe_detect(&dp->connector, NULL, false);
	dp->connector.status = status;

	/*
	 * for some monitor, it will report dsc support
	 * through hotplug after the first trainning.
	 */
	if (sink_support_dsc != mtdisp_get_dp_sink_support_dsc(dp->hwlib_dp))
		dsc_caps_cheanged = true;

	if (old_status != status || dsc_caps_cheanged) {
		/*
		 * We only re-train the link before, but now we do the whole process
		 * of encoder enable, because there's scenario of connector is
		 * plugged during supend, this this will lead to the enabling process
		 * return earlier during resume, so need call encoder enable here.
		 */
		if (dp_enabled && status == connector_status_connected)
			mtdisp_dp_encoder_enable(&dp->encoder);

		drm_kms_helper_hotplug_event(dp->connector.dev);
	}

	if (dp->connected && dp_enabled &&
	    mtdisp_dp_monitor_select_patch(dp, NULL, PATCH_FORCE_TRAINING) &&
	    mtdisp_dp_is_link_loss(dp)) {
		mdelay(500);
		if (mtdisp_dp_is_link_loss(dp)) {
			mdelay(500);
			mtdisp_dp_encoder_enable(&dp->encoder);
		}
	}
}

static void mtdisp_dp_hpd_irq_work(struct work_struct *work)
{
	struct mtdisp_dp *dp = work_to_mtdisp_dp(work);
	int ret = 0;
	u8 val;

	LINUX_DP_TRACE();

	dp->connected = mtdisp_get_dp_connected(dp->hwlib_dp);
	if (!dp->connected)
		return;

	/* mst enabled */
	if (mtdisp_get_dp_ctx_mst_en(dp->hwlib_dp)) {
		mtdisp_dp_mst_hpd_handler(dp);
		return;
	}

	ret = drm_dp_dpcd_readb(&dp->aux, DP_DEVICE_SERVICE_IRQ_VECTOR, &val);
	if (ret <= 0) {
		LINUX_DP_INFO("WARNING: read DP_DEVICE_SERVICE_IRQ_VECTOR fail.\n");
		return;
	}

	if (val) {
		ret = drm_dp_dpcd_writeb(&dp->aux, DP_DEVICE_SERVICE_IRQ_VECTOR, val);
		if (ret < 0) {
			LINUX_DP_INFO("WARNING: write DP_DEVICE_SERVICE_IRQ_VECTOR fail.\n");
			return;
		}

		if (val & DP_AUTOMATED_TEST_REQUEST) {
			LINUX_DP_INFO("Automated test request\n");
			mtdisp_dp_handle_automated_test(dp);

			return;
		}

		if (val & DP_CP_IRQ)
			LINUX_DP_INFO("DP CP IRQ\n");

		if (val & DP_SINK_SPECIFIC_IRQ)
			LINUX_DP_INFO("Sink specific IRQ\n");
	}

	if (mtdisp_dp_is_link_loss(dp))
		mtdisp_dp_encoder_enable(&dp->encoder);
}

static void mtdisp_dp_irq_handler(void *data)
{
	struct mtdisp_dp *dp = (struct mtdisp_dp *)data;
	u32 status = 0;

	LINUX_DP_TRACE();

	status = mtdisp_dp_core_isr(dp->hwlib_dp);

	if (status & MTGPU_DP_IRQ_TYPE_PLUG_EVENT) {
		LINUX_DP_INFO("hotplug in event detected.\n");
		schedule_delayed_work(&dp->hpd_work, 0);
	}

	if (status & MTGPU_DP_IRQ_TYPE_UNPLUG_EVENT) {
		LINUX_DP_INFO("hotplug out event detected.\n");
		schedule_delayed_work(&dp->hpd_work,
				      msecs_to_jiffies(DELAY_ON_DP_PLUGOUT_DET_MS));
	}

	if (status & MTGPU_DP_IRQ_TYPE_HPD_IRQ) {
		LINUX_DP_INFO("hpd irq event detected\n");
		mtgpu_queue_work(&dp->hpd_irq_work);
	}
}

static bool is_int_registed(struct mtdisp_dp *dp, int irq)
{
	struct device *dev = dp->dev;
	struct mtgpu_device *mtdev;

	if (!dev || !dev->parent || !dev->parent->parent)
		return false;

	mtdev = dev_get_drvdata(dev->parent->parent);
	if (!mtdev || !mtdev->irq_info || !mtdev->irq_info->desc_table)
		return false;

	return mtdev->irq_info->desc_table[irq] ? true : false;
}

static bool is_int_enabled(struct mtdisp_dp *dp, int irq)
{
	struct device *dev = dp->dev;
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

int mtdisp_dp_remove(struct mtdisp_dp *dp, struct device *dev)
{
	bool src_support_mst = false;

	LINUX_DP_INFO("\n");

	if (is_int_registed(dp, dp->irq)) {
		if (is_int_enabled(dp, dp->irq))
			mtgpu_disable_interrupt(dev->parent->parent, dp->irq);

		mtgpu_unregister_interrupt(dev->parent->parent, dp->irq);
	}

	src_support_mst = mtdisp_get_dp_src_support_mst(dp->hwlib_dp);
	if (mst_enable && src_support_mst)
		mtdisp_dp_mst_remove(dp);

	mtdisp_dp_audio_unregister(dp);

	drm_dp_aux_unregister(&dp->aux);

	mtdisp_dp_debugfs_remove_files(dp);

	return 0;
}

int mtdisp_dp_probe(struct mtdisp_dp *dp,
		    struct device *dev, struct drm_device *drm)
{
	int ret;
	bool src_support_mst = false;

	LINUX_DP_INFO("\n");

	dp->dev = dev;

	ret = mtgpu_register_interrupt(dev->parent->parent, dp->irq,
				       mtdisp_dp_irq_handler, dp, "dp");
	if (ret) {
		LINUX_DP_ERROR("failed to register DisplayPort irq handler\n");
		return -EINVAL;
	}

	INIT_DELAYED_WORK(&dp->hpd_work, mtdisp_dp_hpd_work);
	INIT_WORK(&dp->hpd_irq_work, mtdisp_dp_hpd_irq_work);
	mutex_init(&dp->lock);

	mtdisp_dp_pre_probe(dp->hwlib_dp);

	ret = mtdisp_dp_audio_register(dp, drm);
	if (ret) {
		LINUX_DP_ERROR("failed to register DP audio device: %d\n", ret);
		return ret;
	}

	mtdisp_dp_drm_init(dp, drm);

	src_support_mst = mtdisp_get_dp_src_support_mst(dp->hwlib_dp);
	if (mst_enable && src_support_mst)
		mtdisp_dp_mst_probe(dp);

	ret = mtgpu_enable_interrupt(dev->parent->parent, dp->irq);
	if (ret) {
		LINUX_DP_ERROR("failed to enable DisplayPort irq\n");
		return -EINVAL;
	}

	mtdisp_dp_debugfs_create_files(dp);

	ret = mtdisp_dp_core_is_plugin(dp->hwlib_dp);
	if (ret < 0)
		LINUX_DP_ERROR("is plugin is not implemented\n");
	else
		dp->connected = ret;

	LINUX_DP_INFO("mtdisp DisplayPort driver loaded successfully\n");

	return 0;
}

int mtdisp_dp_resume(struct mtdisp_dp *dp, struct device *dev)
{
	LINUX_DP_INFO("mtdisp dp device resume early enter\n");

	mtdisp_dp_hw_resume(dp->hwlib_dp);

	/*
	 * The audio service (eg. pulseaudio) will not respond to the
	 * codec_plugged_cb() in S3/S4 suspend and resume stage since
	 * it was frozen. Restore the audio status if it was enabled
	 * before encoder suspend.
	 */
	if (dp->audio_enabled)
		mtdisp_dp_core_audio_enable(dp->hwlib_dp);

	LINUX_DP_INFO("mtdisp dp device resume early exit\n");

	return 0;
}

int mtdisp_dp_suspend(struct mtdisp_dp *dp, struct device *dev)
{
	LINUX_DP_INFO("mtdisp dp device suspend enter.\n");

	mtdisp_dp_phy_exit(dp->hwlib_dp);

	LINUX_DP_INFO("mtdisp dp device suspend exit.\n");

	return 0;
}

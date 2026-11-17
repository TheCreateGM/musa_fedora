/*
 * @Copyright   Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License     Dual MIT/GPLv2
 */

#include <linux/component.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#if defined(OS_DRM_DRMP_H_EXIST)
#include <drm/drmP.h>
#endif
#include <drm/drm_atomic_helper.h>
#include <drm/drm_edid.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_print.h>
#if defined(OS_DRM_DRM_PROBE_HELPER_H_EXIST)
#include <drm/drm_probe_helper.h>
#endif
#if defined(OS_DRM_DISPLAY_DRM_SCDC_HELPER_H_EXIST)
#include <drm/display/drm_scdc_helper.h>
#endif
#if defined(OS_DRM_DRM_SCDC_HELPER_H_EXIST)
#include <drm/drm_scdc_helper.h>
#endif
#include <drm/drm_device.h>
#include <drm/drm_file.h>

#include <video/videomode.h>
#include <sound/hdmi-codec.h>

#include "mtgpu_irq.h"
#include "mtgpu_board_cfg.h"
#include "mtgpu_module_param.h"
#include "module_common.h"

#include "hdmi_types.h"
#include "mtdisp_hdmi.h"

#include "dm_hwlib_access.h"

#include "linux_data_convert.h"
#include "linux_display_drv.h"
#include "linux_hdmi_drv.h"

static const u8 hdmi_port_type[] = {
	DRM_MODE_CONNECTOR_Unknown,	/* PORT_DISABLED */
	DRM_MODE_CONNECTOR_HDMIA,	/* PORT_TYPE_HDMIA */
	DRM_MODE_CONNECTOR_VGA,		/* PORT_TYPE_HDMIA2VGA */
	DRM_MODE_CONNECTOR_LVDS,	/* PORT_TYPE_HDMIA2LVDS */
	DRM_MODE_CONNECTOR_DisplayPort,	/* PORT_TYPE_HDMIA2DP */
};

static struct drm_display_mode *hdmi_supported_modes;

static int mtdisp_create_hdmi_supported_modes(void)
{
	int i;
	struct videomode vm;

	hdmi_supported_modes = kzalloc(hdmi_support_mode_count * sizeof(struct drm_display_mode),
				       GFP_KERNEL);
	if (!hdmi_supported_modes) {
		pr_err("%s() hdmi_supported_modes alloc fail\n", __func__);
		return -ENOMEM;
	}

	for (i = 0; i < hdmi_support_mode_count; i++) {
		/* mt vm to mode */
		mtdisp_videomode_to_os_videomode(&vm, &hdmi_support_mode_with_audio[i]);
		drm_display_mode_from_videomode(&vm, &hdmi_supported_modes[i]);
	}

	return 0;
}

static void mtdisp_destroy_hdmi_supported_modes(void)
{
	if (hdmi_supported_modes)
		kfree(hdmi_supported_modes);
}

static inline struct mtdisp_hdmi *connector_to_mtdisp_hdmi(struct drm_connector *connector)
{
	return container_of(connector, struct mtdisp_hdmi, connector);
}

static inline struct mtdisp_hdmi *encoder_to_mtdisp_hdmi(struct drm_encoder *encoder)
{
	return container_of(encoder, struct mtdisp_hdmi, encoder);
}

static inline struct mtdisp_hdmi *dwork_to_mtdisp_hdmi(struct work_struct *dwork)
{
	return container_of(dwork, struct mtdisp_hdmi, hpd_work.work);
}

static void mtdisp_hdmi_get_edid(struct drm_connector *connector)
{
	struct mtdisp_hdmi *hdmi = connector_to_mtdisp_hdmi(connector);
	u16 retries = 100;

	if (hdmi->connected && !hdmi->edid) {
		do {
			hdmi->edid = drm_get_edid(connector, hdmi->ddc);
			if (hdmi->edid)
				break;

			usleep_range(2000, 3000);
			retries--;
		} while (!hdmi->edid && retries);

		if (!hdmi->edid) {
			LINUX_HDMI_ERROR("failed to get edid\n");
			return;
		}
	}

	/* plugout */
	if (!hdmi->connected && hdmi->edid) {
		kfree(hdmi->edid);
		hdmi->edid = NULL;
	}
}

static enum drm_connector_status
mtdisp_hdmi_connector_detect(struct drm_connector *connector, bool force)
{
	struct mtdisp_hdmi *hdmi = connector_to_mtdisp_hdmi(connector);

	LINUX_HDMI_INFO("\n");

	hdmi->connected = mtdisp_hdmi_core_is_plugin(hdmi->hwlib_hdmi);
	pr_info("HDMI: HPD state is %d\n", hdmi->connected);

	mtdisp_hdmi_get_edid(connector);

	drm_connector_update_edid_property(connector, hdmi->edid);

	/* update eld earlier for audio for kylin */
	drm_add_edid_modes(connector, hdmi->edid);

	return hdmi->connected ? connector_status_connected : connector_status_disconnected;
}

static const struct drm_connector_funcs
mtdisp_hdmi_connector_funcs = {
	.detect			= mtdisp_hdmi_connector_detect,
	.fill_modes		= drm_helper_probe_single_connector_modes,
	.destroy		= drm_connector_cleanup,
	.atomic_duplicate_state	= drm_atomic_helper_connector_duplicate_state,
	.atomic_destroy_state	= drm_atomic_helper_connector_destroy_state,
	.reset			= drm_atomic_helper_connector_reset,
};

static int mtdisp_hdmi_connector_get_modes(struct drm_connector *connector)
{
	struct mtdisp_hdmi *hdmi = connector_to_mtdisp_hdmi(connector);
	bool is_dvi;
	int ret;

	LINUX_HDMI_DEBUG("\n");

	if (!hdmi->edid) {
		LINUX_HDMI_ERROR("no edid\n");

		return -ENODEV;
	}

	ret = drm_add_edid_modes(connector, hdmi->edid);

	is_dvi = !drm_detect_hdmi_monitor(hdmi->edid);
	mtdisp_hdmi_core_set_dvi(hdmi->hwlib_hdmi, is_dvi);
	LINUX_HDMI_DEBUG("Monitor type[0-HDMI,1->DVI]: %d.\n", is_dvi);

	return ret;
}

static struct drm_encoder *
mtdisp_hdmi_connector_best_encoder(struct drm_connector *connector)
{
	struct mtdisp_hdmi *hdmi = connector_to_mtdisp_hdmi(connector);

	LINUX_HDMI_DEBUG("\n");

	return &hdmi->encoder;
}

static int mtdisp_hdmi_mode_compare(struct drm_display_mode *a,
				    struct drm_display_mode *b)
{
	int diff;

	diff = b->hdisplay - a->hdisplay;
	if (diff)
		return diff;

	diff = b->vdisplay - a->vdisplay;
	if (diff)
		return diff;

	diff = drm_mode_vrefresh(b) - drm_mode_vrefresh(a);
	if (diff)
		return diff;

	return diff;
}

static int mtdisp_hdmi_mode_supported_by_product(struct mtdisp_hdmi *hdmi,
						 struct drm_display_mode *mode)
{
	u32 max_hres = mtdisp_hdmi_get_max_hres(hdmi->hwlib_hdmi);
	u32 max_vres = mtdisp_hdmi_get_max_vres(hdmi->hwlib_hdmi);
	u32 max_pclk_100khz = mtdisp_hdmi_get_max_pclk_100khz(hdmi->hwlib_hdmi);

	if (mode->clock / 100 > max_pclk_100khz)
		return MODE_CLOCK_HIGH;

	if (mode->hdisplay > max_hres)
		return MODE_H_ILLEGAL;

	if (mode->vdisplay > max_vres)
		return MODE_V_ILLEGAL;

	return MODE_OK;
}

static void mtdisp_hdmi_set_preferred_mode(struct drm_connector *connector,
					   struct drm_display_mode *filtered_mode)
{
	struct drm_display_mode *mode;
	struct drm_display_mode tmp_best_mode = {0};
	struct drm_display_mode *preferred_mode = NULL;
	struct mtdisp_hdmi *hdmi = connector_to_mtdisp_hdmi(connector);

	if (list_empty(&connector->modes))
		return;

	/* Find the best mode and set its type preferred. */
	list_for_each_entry(mode, &connector->modes, head) {
		struct videomode vm;
		struct mtdisp_videomode mt_vm;
		/* mode to mt vm */
		drm_display_mode_to_videomode(mode, &vm);
		mtdisp_videomode_from_os_videomode(&mt_vm, &vm);

		if (!mtdisp_hdmi_mode_supported_with_audio(hdmi->hwlib_hdmi, &mt_vm) ||
		    (mtdisp_hdmi_mode_supported_by_product(hdmi, mode) != MODE_OK) ||
		    mode->status != MODE_OK ||
		    drm_mode_equal(mode, filtered_mode))
			continue;

		if (mtdisp_hdmi_mode_compare(&tmp_best_mode, mode) > 0) {
			memcpy(&tmp_best_mode, mode, sizeof(struct drm_display_mode));
			preferred_mode = mode;
		}
	}

	if (preferred_mode)
		preferred_mode->type |= DRM_MODE_TYPE_PREFERRED;
}

#ifdef OS_DRM_CONNECTOR_MODE_VALID_CONST
static enum drm_mode_status
mtdisp_hdmi_connector_mode_valid(struct drm_connector *connector,
				 const struct drm_display_mode *mode)
#else
static int mtdisp_hdmi_connector_mode_valid(struct drm_connector *connector,
					    struct drm_display_mode *mode)
#endif
{
	struct videomode vm;
	struct mtdisp_videomode mt_vm;
	struct mtdisp_hdmi *hdmi = connector_to_mtdisp_hdmi(connector);
	int mode_status;

	LINUX_HDMI_DEBUG("\n");

	/* check if the mode can be supported by product. */
	mode_status = mtdisp_hdmi_mode_supported_by_product(hdmi, mode);
	if (mode_status != MODE_OK)
		goto preferred_mode_check;

	/* mode to mt vm */
	drm_display_mode_to_videomode(mode, &vm);
	mtdisp_videomode_from_os_videomode(&mt_vm, &vm);
	/*
	 * HDMI IP does not support well for reduced blank timing,
	 * if audio packet is transimited with this timing,
	 * hdmi receiver would flicker, so we only support the modes
	 * listed in the hdmi_supported_modes.
	 */
	if (!mtdisp_hdmi_mode_supported_with_audio(hdmi->hwlib_hdmi, &mt_vm)) {
		mode_status = MODE_BAD;
		goto preferred_mode_check;
	}

	return mode_status;

preferred_mode_check:
	if (mode->type & DRM_MODE_TYPE_PREFERRED) {
		/*
		 * When the preferred mode is unsupported,
		 * we should set the remaining best mode preferred.
		 */
		mtdisp_hdmi_set_preferred_mode(connector, mode);
	}

	return mode_status;
}

static const struct drm_connector_helper_funcs
mtdisp_hdmi_connector_helper_funcs = {
	.get_modes	= mtdisp_hdmi_connector_get_modes,
	.best_encoder	= mtdisp_hdmi_connector_best_encoder,
	.mode_valid	= mtdisp_hdmi_connector_mode_valid,
};

static int mtdisp_hdmi_send_audio_infoframe(struct mtdisp_hdmi *hdmi)
{
	struct hdmi_audio_infoframe frame;
	u8 buffer[HDMI_INFOFRAME_HEADER_SIZE + HDMI_AUDIO_INFOFRAME_SIZE];
	ssize_t err;

	LINUX_HDMI_INFO("\n");

	err = hdmi_audio_infoframe_init(&frame);
	if (err < 0) {
		LINUX_HDMI_ERROR("Failed to setup audio infoframe: %zd\n", err);
		return err;
	}

	frame.coding_type = HDMI_AUDIO_CODING_TYPE_STREAM;
	frame.sample_frequency = HDMI_AUDIO_SAMPLE_FREQUENCY_STREAM;
	frame.sample_size = HDMI_AUDIO_SAMPLE_SIZE_STREAM;
	frame.channels = 2;

	err = hdmi_audio_infoframe_pack(&frame, buffer, sizeof(buffer));
	if (err < 0) {
		LINUX_HDMI_ERROR("Failed to pack audio infoframe: %zd\n", err);
		return err;
	}

	mtdisp_hdmi_core_vendor_infoframe(hdmi->hwlib_hdmi, buffer, sizeof(buffer));

	return 0;
}

static int mtdisp_hdmi_send_avi_infoframe(struct mtdisp_hdmi *hdmi)
{
	struct hdmi_avi_infoframe frame;
	u8 buffer[HDMI_INFOFRAME_HEADER_SIZE + HDMI_AVI_INFOFRAME_SIZE];
	struct drm_display_mode mode = {};
	struct videomode vm;
	struct mtdisp_videomode mt_vm;
	ssize_t err;

	LINUX_HDMI_INFO("\n");

	mtdisp_get_hdmi_ctx_vm(hdmi->hwlib_hdmi, &mt_vm);
	mtdisp_videomode_to_os_videomode(&vm, &mt_vm);
	drm_display_mode_from_videomode(&vm, &mode);

#if !defined(OS_DRM_HDMI_AVI_INFOFRAME_FROM_DISPLAY_MODE_HAS_BOOL_ARG)
	err = drm_hdmi_avi_infoframe_from_display_mode(&frame,
						       &hdmi->connector, &mode);
#else
	err = drm_hdmi_avi_infoframe_from_display_mode(&frame, &mode, false);
#endif
	if (err < 0) {
		LINUX_HDMI_ERROR("Failed to get AVI infoframe from mode: %zd\n", err);
		return err;
	}

	err = hdmi_avi_infoframe_pack(&frame, buffer, sizeof(buffer));
	if (err < 0) {
		LINUX_HDMI_ERROR("Failed to pack AVI infoframe: %zd\n", err);
		return err;
	}

	mtdisp_hdmi_core_avi_infoframe(hdmi->hwlib_hdmi, buffer, sizeof(buffer));

	return 0;
}

static int mtdisp_hdmi_send_spd_infoframe(struct mtdisp_hdmi *hdmi)
{
	struct hdmi_spd_infoframe frame;
	u8 buffer[HDMI_INFOFRAME_HEADER_SIZE + HDMI_SPD_INFOFRAME_SIZE];
	ssize_t err;

	LINUX_HDMI_INFO("\n");

	err = hdmi_spd_infoframe_init(&frame, "MooreThreads", "On-Chip HDMI");
	if (err < 0) {
		LINUX_HDMI_ERROR("Failed to initialize SPD infoframe: %zd\n", err);
		return err;
	}

	err = hdmi_spd_infoframe_pack(&frame, buffer, sizeof(buffer));
	if (err < 0) {
		LINUX_HDMI_ERROR("Failed to pack SDP infoframe: %zd\n", err);
		return err;
	}

	mtdisp_hdmi_core_spd_infoframe(hdmi->hwlib_hdmi, buffer, sizeof(buffer));

	return 0;
}

static int mtdisp_hdmi_send_vendor_infoframe(struct mtdisp_hdmi *hdmi)
{
	struct drm_display_mode mode = {};
	struct videomode vm;
	struct mtdisp_videomode mt_vm;
	struct hdmi_vendor_infoframe frame;
	u8 buffer[10];
	ssize_t err;
	bool has_hdmi_infoframe = hdmi->connector.display_info.has_hdmi_infoframe;

	LINUX_HDMI_INFO("\n");

	if (!has_hdmi_infoframe)
		return 0;

	mtdisp_get_hdmi_ctx_vm(hdmi->hwlib_hdmi, &mt_vm);
	mtdisp_videomode_to_os_videomode(&vm, &mt_vm);
	drm_display_mode_from_videomode(&vm, &mode);

	err = drm_hdmi_vendor_infoframe_from_display_mode(&frame,
							  &hdmi->connector, &mode);
	if (err) {
		LINUX_HDMI_ERROR("Failed to get vendor infoframe from mode: %zd\n", err);
		return err;
	}

	err = hdmi_vendor_infoframe_pack(&frame, buffer, sizeof(buffer));
	if (err < 0) {
		LINUX_HDMI_ERROR("Failed to pack vendor infoframe: %zd\n", err);
		return err;
	}

	mtdisp_hdmi_core_vendor_infoframe(hdmi->hwlib_hdmi, buffer, sizeof(buffer));

	return 0;
}

static void mtdisp_hdmi_scramble_config(struct mtdisp_hdmi *hdmi)
{
	u8 version;
	struct drm_display_info *disp_info = &hdmi->connector.display_info;
	struct mtdisp_videomode mt_vm;

	LINUX_HDMI_INFO("\n");

	if (!disp_info->hdmi.scdc.supported) {
		mtdisp_hdmi_core_scramble_config(hdmi->hwlib_hdmi, false);
		return;
	}

	mtdisp_get_hdmi_ctx_vm(hdmi->hwlib_hdmi, &mt_vm);

	if (mt_vm.pixelclock > 340000000 ||
	    disp_info->hdmi.scdc.scrambling.low_rates) {
		drm_scdc_readb(hdmi->ddc, SCDC_SINK_VERSION, &version);

		drm_scdc_writeb(hdmi->ddc, SCDC_SOURCE_VERSION, min_t(u8, version, 0x01));

#if defined(OS_DRM_SCDC_SET_HIGH_TMDS_CLOCK_RATIO_USE_DRM_CONNECTOR_ARG)
		drm_scdc_set_high_tmds_clock_ratio(&hdmi->connector, true);
#else
		drm_scdc_set_high_tmds_clock_ratio(hdmi->ddc, true);
#endif

#if defined(OS_DRM_SCDC_SET_SCRAMBLING_USE_DRM_CONNECTOR_ARG)
		drm_scdc_set_scrambling(&hdmi->connector, true);
#else
		drm_scdc_set_scrambling(hdmi->ddc, true);
#endif

		mtdisp_hdmi_core_scramble_config(hdmi->hwlib_hdmi, true);
	} else {
#if defined(OS_DRM_SCDC_SET_HIGH_TMDS_CLOCK_RATIO_USE_DRM_CONNECTOR_ARG)
		drm_scdc_set_high_tmds_clock_ratio(&hdmi->connector, false);
#else
		drm_scdc_set_high_tmds_clock_ratio(hdmi->ddc, false);
#endif

#if defined(OS_DRM_SCDC_SET_SCRAMBLING_USE_DRM_CONNECTOR_ARG)
		drm_scdc_set_scrambling(&hdmi->connector, false);
#else
		drm_scdc_set_scrambling(hdmi->ddc, false);
#endif

		mtdisp_hdmi_core_scramble_config(hdmi->hwlib_hdmi, false);
	}
}

static void mtdisp_hdmi_encoder_enable(struct drm_encoder *encoder)
{
	struct mtdisp_hdmi *hdmi = encoder_to_mtdisp_hdmi(encoder);

	LINUX_HDMI_INFO("\n");

	if (!hdmi->connected) {
		LINUX_HDMI_INFO("Not connected, skip the enable\n");
		goto hdmi_enabled;
	}

	mtdisp_hdmi_core_config(hdmi->hwlib_hdmi);

	mtdisp_hdmi_send_audio_infoframe(hdmi);
	mtdisp_hdmi_send_avi_infoframe(hdmi);
	mtdisp_hdmi_send_spd_infoframe(hdmi);
	mtdisp_hdmi_send_vendor_infoframe(hdmi);

	/* TODO: Send other packets here, for example: GCP, ACP etc. */

	mtdisp_hdmi_scramble_config(hdmi);

	mtdisp_hdmi_core_enable(hdmi->hwlib_hdmi);

	if (hdmi->codec_plugged_cb && hdmi->codec_dev)
		hdmi->codec_plugged_cb(hdmi->codec_dev, true);

hdmi_enabled:
	mtdisp_set_hdmi_enabled(hdmi->hwlib_hdmi, true);
}

static void mtdisp_hdmi_encoder_disable(struct drm_encoder *encoder)
{
	struct mtdisp_hdmi *hdmi = encoder_to_mtdisp_hdmi(encoder);

	LINUX_HDMI_INFO("\n");

	if (hdmi->codec_plugged_cb && hdmi->codec_dev)
		hdmi->codec_plugged_cb(hdmi->codec_dev, false);

	mtdisp_hdmi_core_disable(hdmi->hwlib_hdmi);

	mtdisp_set_hdmi_enabled(hdmi->hwlib_hdmi, false);
}

static void
mtdisp_hdmi_encoder_atomic_mode_set(struct drm_encoder *encoder,
				    struct drm_crtc_state *crtc_state,
				    struct drm_connector_state *connector_state)
{
	struct mtdisp_hdmi *hdmi = encoder_to_mtdisp_hdmi(encoder);
	struct drm_display_mode *adjusted_mode = &crtc_state->adjusted_mode;
	struct videomode vm;
	struct mtdisp_videomode mt_vm;

	LINUX_HDMI_INFO("\n");

	drm_display_mode_to_videomode(adjusted_mode, &vm);
	mtdisp_videomode_from_os_videomode(&mt_vm, &vm);
	mtdisp_set_hdmi_ctx_vm(hdmi->hwlib_hdmi, &mt_vm);
}

static bool mtdisp_hdmi_match_res_fps(const struct drm_display_mode *mode1,
				      const struct drm_display_mode *mode2)
{
	return mode1->hdisplay == mode2->hdisplay &&
	       mode1->vdisplay == mode2->vdisplay &&
	       drm_mode_vrefresh(mode1) == drm_mode_vrefresh(mode2);
}

static bool mtdisp_hdmi_encoder_mode_fixup(struct drm_encoder *encoder,
					   const struct drm_display_mode *mode,
					   struct drm_display_mode *adjusted_mode)
{
	int i;
	struct mtdisp_hdmi *hdmi = encoder_to_mtdisp_hdmi(encoder);

	LINUX_HDMI_INFO("\n");

	for (i = 0; i < hdmi_support_mode_count; i++)
		if (drm_mode_match(mode, &hdmi_supported_modes[i],
				   DRM_MODE_MATCH_TIMINGS | DRM_MODE_MATCH_CLOCK))
			return true;

	for (i = 0; i < hdmi_support_mode_count; i++) {
		if (mtdisp_hdmi_match_res_fps(mode, &hdmi_supported_modes[i])) {
			drm_mode_copy(adjusted_mode, &hdmi_supported_modes[i]);
			drm_mode_set_crtcinfo(adjusted_mode, 0);
			break;
		}
	}

	return true;
}

static const struct drm_encoder_helper_funcs mtdisp_hdmi_encoder_helper_funcs = {
	.enable			= mtdisp_hdmi_encoder_enable,
	.disable		= mtdisp_hdmi_encoder_disable,
	.atomic_mode_set	= mtdisp_hdmi_encoder_atomic_mode_set,
	.mode_fixup		= mtdisp_hdmi_encoder_mode_fixup,
};

static const struct drm_encoder_funcs mtdisp_hdmi_encoder_funcs = {
	.destroy = drm_encoder_cleanup,
};

static int mtdisp_hdmi_drm_init(struct mtdisp_hdmi *hdmi, struct drm_device *drm)
{
	struct drm_encoder *encoder = &hdmi->encoder;
	struct drm_connector *connector = &hdmi->connector;
	int ret;

	/*
	* WARNING:
	* The possible_crtcs mask is in the logical order of dispc platform device
	* registration, not the dispc hardware ID order. So if the dispc platform
	* device count changed, this possible_crtcs mask should also be changed.
	*/
	encoder->possible_crtcs = BIT(2);
	ret = drm_encoder_init(drm, encoder, &mtdisp_hdmi_encoder_funcs,
			       DRM_MODE_ENCODER_TMDS, NULL);
	if (ret) {
		LINUX_HDMI_ERROR("failed to create hdmi encoder\n");
		return ret;
	}
	drm_encoder_helper_add(encoder, &mtdisp_hdmi_encoder_helper_funcs);

	connector->polled = DRM_CONNECTOR_POLL_HPD;

	if (hdmi->port_type >= ARRAY_SIZE(hdmi_port_type)) {
		LINUX_HDMI_WARN("invalid hdmi->port_type 0x%x, use HDMIA type as default\n",
				hdmi->port_type);
		hdmi->port_type = PORT_TYPE_HDMIA;
	}

#if defined(OS_FUNC_DRM_CONNECTOR_INIT_WITH_DDC_EXIST)
	ret = drm_connector_init_with_ddc(drm, connector,
					  &mtdisp_hdmi_connector_funcs,
					  hdmi_port_type[hdmi->port_type],
					  hdmi->ddc);
#else
	ret = drm_connector_init(drm, connector,
				 &mtdisp_hdmi_connector_funcs,
				 hdmi_port_type[hdmi->port_type]);
#endif
	if (ret) {
		LINUX_HDMI_ERROR("failed to create hdmi connector\n");
		return ret;
	}

	drm_connector_helper_add(connector, &mtdisp_hdmi_connector_helper_funcs);
	drm_connector_register(connector);
	drm_connector_attach_encoder(connector, encoder);

	return 0;
}

static void mtdisp_hdmi_hpd_work(struct work_struct *work)
{
	enum drm_connector_status old_status;
	struct mtdisp_hdmi *hdmi = dwork_to_mtdisp_hdmi(work);
	bool hdmi_enabled = mtdisp_get_hdmi_enabled(hdmi->hwlib_hdmi);

	LINUX_HDMI_INFO("\n");

	old_status = hdmi->connected ? connector_status_connected : connector_status_disconnected;

	mutex_lock(&hdmi->connector.dev->mode_config.mutex);
	hdmi->connector.status = drm_helper_probe_detect(&hdmi->connector, NULL, false);
	mutex_unlock(&hdmi->connector.dev->mode_config.mutex);

	if (hdmi_enabled && hdmi->connector.status == connector_status_connected)
		mtdisp_hdmi_encoder_enable(&hdmi->encoder);

	if (old_status != hdmi->connector.status)
		drm_kms_helper_hotplug_event(hdmi->connector.dev);
}

static void mtdisp_hdmi_irq_handler(void *data)
{
	struct mtdisp_hdmi *hdmi = (struct mtdisp_hdmi *)data;
	u32 status = 0;

	status = mtdisp_hdmi_core_isr(hdmi->hwlib_hdmi);

	if (status & MTDISP_HDMI_IRQ_TYPE_HPD_OUT)
		schedule_delayed_work(&hdmi->hpd_work,
				      msecs_to_jiffies(DELAY_ON_HDMI_PLUGOUT_DETECTION_MS));
	else if (status & MTDISP_HDMI_IRQ_TYPE_HPD_IN)
		schedule_delayed_work(&hdmi->hpd_work, 0);
}

/*
 * HDMI audio codec callbacks
 */
static int mtdisp_hdmi_audio_hw_params(struct device *dev, void *data,
				       struct hdmi_codec_daifmt *daifmt,
				       struct hdmi_codec_params *params)
{
	struct mtdisp_hdmi *hdmi = data;
	bool hdmi_enabled = mtdisp_get_hdmi_enabled(hdmi->hwlib_hdmi);
	struct mtdisp_codec_params codec_param;

	if (!params) {
		LINUX_HDMI_ERROR("audio params is NULL.\n");
		return -EINVAL;
	}

	if (!hdmi_enabled) {
		LINUX_HDMI_INFO("hdmi stream has not enabled.\n");
		return -EPERM;
	}

	LINUX_HDMI_INFO("\n");

	codec_param.sample_rate = params->sample_rate;
	codec_param.sample_width = params->sample_width;
	codec_param.channels = params->channels;

	LINUX_HDMI_DEBUG("%u Hz, %d bit, %d channels\n",
			 params->sample_rate,
			 params->sample_width,
			 params->channels);

	mtdisp_hdmi_core_audio_set_param(hdmi->hwlib_hdmi, &codec_param);

	return 0;
}

static int mtdisp_hdmi_audio_startup(struct device *dev, void *data)
{
	struct mtdisp_hdmi *hdmi = data;

	LINUX_HDMI_INFO("\n");

	mtdisp_hdmi_core_audio_set_stream_enable(hdmi->hwlib_hdmi);

	hdmi->audio_enabled = true;

	return 0;
}

static void mtdisp_hdmi_audio_shutdown(struct device *dev, void *data)
{
	struct mtdisp_hdmi *hdmi = data;

	LINUX_HDMI_INFO("\n");

	mtdisp_hdmi_core_audio_set_stream_disable(hdmi->hwlib_hdmi);

	hdmi->audio_enabled = false;
}

#if defined(OS_STRUCT_HDMI_CODEC_OPS_HAS_MUTE_STREAM)
static int mtdisp_hdmi_audio_mute(struct device *dev, void *data, bool enable, int direction)
#else
static int mtdisp_hdmi_audio_mute(struct device *dev, void *data, bool enable)
#endif
{
	struct mtdisp_hdmi *hdmi = data;

	LINUX_HDMI_INFO("\n");

	if (enable)
		mtdisp_hdmi_core_audio_set_mute(hdmi->hwlib_hdmi);
	else
		mtdisp_hdmi_core_audio_set_unmute(hdmi->hwlib_hdmi);

	return 0;
}

static int mtdisp_hdmi_audio_get_eld(struct device *dev, void *data, uint8_t *buf, size_t len)
{
	struct mtdisp_hdmi *hdmi = data;

	LINUX_HDMI_INFO("\n");

	memcpy(buf, hdmi->connector.eld, min(sizeof(hdmi->connector.eld), len));

	return 0;
}

static int mtdisp_hdmi_audio_hook_plugged_cb(struct device *dev, void *data,
					     hdmi_codec_plugged_cb fn,
					     struct device *codec_dev)
{
	struct mtdisp_hdmi *hdmi = data;
	bool hdmi_enabled = mtdisp_get_hdmi_enabled(hdmi->hwlib_hdmi);

	LINUX_HDMI_INFO("\n");

	mutex_lock(&hdmi->codec_lock);
	hdmi->codec_plugged_cb = fn;
	hdmi->codec_dev = codec_dev;
	mutex_unlock(&hdmi->codec_lock);

	if (hdmi->codec_plugged_cb && hdmi->codec_dev)
		hdmi->codec_plugged_cb(hdmi->codec_dev, hdmi_enabled);

	return 0;
}

#if defined(OS_STRUCT_HDMI_CODEC_OPS_HAS_HOOK_PLUGGED_CB)
static const struct hdmi_codec_ops mtdisp_hdmi_audio_codec_ops = {
	.hw_params = mtdisp_hdmi_audio_hw_params,
	.audio_startup = mtdisp_hdmi_audio_startup,
	.audio_shutdown = mtdisp_hdmi_audio_shutdown,
#if defined(OS_STRUCT_HDMI_CODEC_OPS_HAS_MUTE_STREAM)
	.mute_stream = mtdisp_hdmi_audio_mute,
#else
	.digital_mute = mtdisp_hdmi_audio_mute,
#endif
	.get_eld = mtdisp_hdmi_audio_get_eld,
	.hook_plugged_cb = mtdisp_hdmi_audio_hook_plugged_cb,
};
#else
typedef int (*hook_plugged_callback)(struct device *dev, void *data,
					    hdmi_codec_plugged_cb fn,
					    struct device *codec_dev);

struct mtdisp_codec_ops {
	struct hdmi_codec_ops old_hdmi_audio_codec_ops;
	hook_plugged_callback hook_plugged_cb;
};

static const struct mtdisp_codec_ops mtdisp_hdmi_audio_codec_ops = {
	.old_hdmi_audio_codec_ops = {
		.hw_params = mtdisp_hdmi_audio_hw_params,
		.audio_startup = mtdisp_hdmi_audio_startup,
		.audio_shutdown = mtdisp_hdmi_audio_shutdown,
		.digital_mute = mtdisp_hdmi_audio_mute,
		.get_eld = mtdisp_hdmi_audio_get_eld,
	},
	.hook_plugged_cb = mtdisp_hdmi_audio_hook_plugged_cb,
};
#endif

static int mtdisp_hdmi_audio_register(struct mtdisp_hdmi *hdmi, struct drm_device *drm)
{
	struct hdmi_codec_pdata codec_data = {
		.ops = (struct hdmi_codec_ops *)&mtdisp_hdmi_audio_codec_ops,
		.max_i2s_channels = 2,
		.i2s = 1,
		.data = hdmi,
	};
	int idx = mtgpu_cnt - 1; // idx starts from 0
	char dev_name[64];
	const char *name = dev_name;

	snprintf(dev_name, sizeof(dev_name), "mtgpu-%d-hdmi-audio-codec", idx);
	hdmi->hdmi_audio = platform_device_register_data(hdmi->dev, name,
							 PLATFORM_DEVID_AUTO, &codec_data,
							 sizeof(codec_data));
	if (IS_ERR(hdmi->hdmi_audio))
		return PTR_ERR(hdmi->hdmi_audio);

	LINUX_HDMI_INFO("audio codec '%s' registered successfully.\n", dev_name);

	mutex_init(&hdmi->codec_lock);

	return 0;
}

static void mtdisp_hdmi_audio_unregister(struct mtdisp_hdmi *hdmi)
{
	if (hdmi->hdmi_audio) {
		platform_device_unregister(hdmi->hdmi_audio);
		hdmi->hdmi_audio = NULL;
	}
}

/*
 * DDC I2C Adapter
 */
static int mtdisp_hdmi_i2c_xfer(struct i2c_adapter *ddc, struct i2c_msg *msgs, int num)
{
	struct mtdisp_hdmi *hdmi = i2c_get_adapdata(ddc);
	int i, j, ret = 0;

	mutex_lock(&hdmi->i2c_lock);

	for (i = 0; i < num; i++) {
		LINUX_HDMI_DEBUG("xfer: num: %d/%d, addr: 0x%x, len: %d, flags: %#x\n",
				 i + 1, num, msgs[i].addr, msgs[i].len, msgs[i].flags);

		if (msgs[i].flags & I2C_M_RD)
			ret = mtdisp_hdmi_core_i2c_read(hdmi->hwlib_hdmi,
							msgs[i].addr, msgs[i].buf, msgs[i].len);
		else
			ret = mtdisp_hdmi_core_i2c_write(hdmi->hwlib_hdmi,
							 msgs[i].addr, msgs[i].buf, msgs[i].len);

		if (ret < 0)
			break;

		for (j = 0; j < msgs[i].len; j++)
			LINUX_HDMI_DEBUG("0x%x\n", msgs[i].buf[j]);
	}

	if (!ret)
		ret = num;

	mutex_unlock(&hdmi->i2c_lock);

	return ret;
}

static u32 mtdisp_hdmi_i2c_func(struct i2c_adapter *ddc)
{
	return I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL;
}

static const struct i2c_algorithm mtdisp_hdmi_algorithm = {
	.master_xfer	= mtdisp_hdmi_i2c_xfer,
	.functionality	= mtdisp_hdmi_i2c_func,
};

static int mtdisp_hdmi_ddc_register(struct mtdisp_hdmi *hdmi)
{
	struct i2c_adapter *ddc;
	int ret;

	ddc = devm_kzalloc(hdmi->dev, sizeof(*ddc), GFP_KERNEL);
	if (!ddc)
		return -ENOMEM;

	ddc->owner = THIS_MODULE;
#if defined (I2C_CLASS_DDC)
	ddc->class = I2C_CLASS_DDC;
#endif
	ddc->dev.parent = hdmi->dev;
	ddc->algo = &mtdisp_hdmi_algorithm;
	snprintf(ddc->name, sizeof(ddc->name), "mtdisp hdmi i2c");

	ret = i2c_add_adapter(ddc);
	if (ret) {
		LINUX_HDMI_ERROR("failed to add I2C adapter: %d\n", ret);
		return ret;
	}

	i2c_set_adapdata(ddc, hdmi);
	hdmi->ddc = ddc;

	mutex_init(&hdmi->i2c_lock);

	return 0;
}

int mtdisp_hdmi_probe(struct mtdisp_hdmi *hdmi,
		      struct device *dev, struct drm_device *drm)
{
	int ret;

	LINUX_HDMI_INFO("\n");
	LINUX_HDMI_INFO("hdmi: id: %d, irq: %d, connected: %d\n",
			hdmi->id, hdmi->irq, hdmi->connected);

	hdmi->dev = dev;

	ret = mtgpu_register_interrupt(dev->parent->parent, hdmi->irq,
				       mtdisp_hdmi_irq_handler, hdmi, "hdmi");
	if (ret) {
		LINUX_HDMI_ERROR("failed to register HDMI irq handler\n");
		return -EINVAL;
	}

	INIT_DELAYED_WORK(&hdmi->hpd_work, mtdisp_hdmi_hpd_work);

	mtdisp_hdmi_pre_probe(hdmi->hwlib_hdmi);

	ret = mtdisp_hdmi_ddc_register(hdmi);
	if (ret) {
		LINUX_HDMI_ERROR("failed to register HDMI ddc adapter: %d\n", ret);
		return -EINVAL;
	}

	ret = mtdisp_hdmi_audio_register(hdmi, drm);
	if (ret) {
		LINUX_HDMI_ERROR("failed to register HDMI audio device: %d\n", ret);
		return -EINVAL;
	}

	mtdisp_hdmi_drm_init(hdmi, drm);

	LINUX_HDMI_INFO("hdmi: encoder:   name: %s, index: %d, encoder_type: %d, possible_crtcs: 0x%x\n",
			hdmi->encoder.name, hdmi->encoder.index, hdmi->encoder.encoder_type, hdmi->encoder.possible_crtcs);

	LINUX_HDMI_INFO("hdmi: connector: name: %s, index: %d, connector_type: %d, \n",
			hdmi->connector.name, hdmi->connector.index, hdmi->connector.connector_type);

	mtdisp_hdmi_core_hpd_enable(hdmi->hwlib_hdmi);

	ret = mtgpu_enable_interrupt(dev->parent->parent, hdmi->irq);
	if (ret) {
		LINUX_HDMI_ERROR("failed to enable HDMI irq\n");
		return -EINVAL;
	}

	ret = mtdisp_create_hdmi_supported_modes();
	if (ret) {
		LINUX_HDMI_ERROR("failed to create hdmi supported modes\n");
		return ret;
	}

	LINUX_HDMI_INFO("mtdisp HDMI driver loaded successfully\n");

	return 0;
}

int mtdisp_hdmi_remove(struct mtdisp_hdmi *hdmi, struct device *dev)
{
	int ret = 0;

	LINUX_HDMI_INFO("\n");

	mtdisp_hdmi_core_hw_deinit(hdmi->hwlib_hdmi);

	ret = mtgpu_disable_interrupt(dev->parent->parent, hdmi->irq);
	if (ret)
		LINUX_HDMI_ERROR("failed to disable HDMI irq\n");

	ret = mtgpu_unregister_interrupt(dev->parent->parent, hdmi->irq);
	if (ret)
		LINUX_HDMI_ERROR("failed to deregister HDMI irq handler\n");

	mtdisp_hdmi_audio_unregister(hdmi);

	if (hdmi->ddc)
		i2c_del_adapter(hdmi->ddc);

	mtdisp_hdmi_ctx_deinit(hdmi->hwlib_hdmi);

	mtdisp_destroy_hdmi_supported_modes();

	LINUX_HDMI_INFO("unload mtdisp HDMI driver\n");

	return 0;
}

int mtdisp_hdmi_resume(struct mtdisp_hdmi *hdmi, struct device *dev)
{
	LINUX_HDMI_INFO("mtdisp hdmi device resume early enter\n");

	/* disable first in case it's enabled by bios */
	mtdisp_hdmi_core_disable(hdmi->hwlib_hdmi);

	mtdisp_hdmi_core_hw_init(hdmi->hwlib_hdmi);

	/*
	* The audio service (eg. pluseaudio) will not response to the
	* codec_plugged_cb() in S3/S4 suspend and resume stage since
	* it was freezeed. Restore the audio status if it was enabled
	* before encoder suspend.
	*/
	if (hdmi->audio_enabled)
		mtdisp_hdmi_core_audio_enable(hdmi->hwlib_hdmi);

	hdmi->connected = mtdisp_hdmi_core_is_plugin(hdmi->hwlib_hdmi);
	pr_info("HDMI: HPD state is %d\n", hdmi->connected);

	LINUX_HDMI_INFO("mtdisp hdmi device resume early exit\n");

	return 0;
}

void mtdisp_hdmi_complete(struct mtdisp_hdmi *hdmi, struct device *dev)
{
	LINUX_HDMI_INFO("mtdisp hdmi complete enter\n");

	mtdisp_hdmi_core_hpd_enable(hdmi->hwlib_hdmi);

	LINUX_HDMI_INFO("mtdisp hdmi complete exit\n");
}

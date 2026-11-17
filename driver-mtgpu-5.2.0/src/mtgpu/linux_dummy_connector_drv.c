/*
 * @Copyright   Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License     Dual MIT/GPLv2
 */

#if defined(OS_DRM_DRMP_H_EXIST)
#include <drm/drmP.h>
#endif
#include <drm/drm_atomic_helper.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_print.h>
#if defined(OS_DRM_DRM_PROBE_HELPER_H_EXIST)
#include <drm/drm_probe_helper.h>
#endif
#include <drm/drm_edid.h>

#include "mtgpu_drv.h"

#include "linux_dummy_drv.h"
#include "linux_dummy_connector_drv.h"

static struct dummy_display_mode {
	int hdisplay;
	int vdisplay;
	int vrefresh;
} extra_modes[] = {
#if (MUSA_NUM_OS_SUPPORTED > 1)
	{  800,  600, 30 },
	{  800,  600, 60 },
	{ 1024,  768, 30 },
	{ 1024,  768, 60 },
	{ 1152,  864, 30 },
	{ 1152,  864, 60 },
	{ 1280,  720, 30 },
	{ 1280,  720, 60 },
	{ 1280,  768, 30 },
	{ 1280,  768, 60 },
	{ 1280,  800, 30 },
	{ 1280,  800, 60 },
	{ 1280,  960, 30 },
	{ 1280,  960, 60 },
	{ 1280, 1024, 30 },
	{ 1280, 1024, 60 },
	{ 1360,  768, 30 },
	{ 1360,  768, 60 },
	{ 1600,  900, 30 },
	{ 1600,  900, 60 },
	{ 1600, 1024, 30 },
	{ 1600, 1024, 60 },
	{ 1600, 1200, 30 },
	{ 1600, 1200, 60 },
	{ 1680, 1050, 30 },
	{ 1680, 1050, 60 },
	{ 1920, 1080, 30 },
	{ 1920, 1080, 60 },
	{ 1920, 1200, 30 },
	{ 1920, 1200, 60 },
	{ 2560, 1440, 30 },
	{ 2560, 1440, 60 },
	{ 2560, 1600, 30 },
	{ 2560, 1600, 60 },
	{ 3840, 2160, 30 },
	{ 3840, 2160, 60 },
	{ 4096, 2160, 30 },
	{ 4096, 2160, 60 },
#else
	{ 1920, 1080, 30 },
	{ 1920, 1080, 120 },
	{ 1920, 1080, 144 },
	{ 2560, 1440, 30 },
	{ 2560, 1440, 60 },
	{ 2560, 1440, 120 },
	{ 2560, 1440, 144 },
	{ 3440, 1440, 30 },
	{ 3440, 1440, 60 },
	{ 3440, 1440, 120 },
	{ 3440, 1440, 144 },
	{ 3840, 2160, 30 },
	{ 3840, 2160, 60 },
	{ 3840, 2160, 120 },
	{ 3840, 2160, 144 },
#endif
};

static struct drm_display_mode custom_modes[] = {
	{ DRM_MODE("1366x768", DRM_MODE_TYPE_DRIVER,
		   40028, 1366, 1400, 1534, 1702, 0, 768, 771, 781, 784, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC) },
	{ DRM_MODE("1366x768", DRM_MODE_TYPE_DRIVER,
		   85250, 1366, 1400, 1574, 1782, 0, 768, 771, 781, 798, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC) },
};

static int mtdisp_dummy_connector_add_extra_modes(struct drm_connector *connector,
						  uint32_t max_hres,
						  uint32_t max_vres)
{
	struct drm_device *dev = connector->dev;
	struct drm_display_mode *mode;
	int i, count = 0;

	for (i = 0; i < ARRAY_SIZE(extra_modes); i++) {
		if ((max_hres >= extra_modes[i].hdisplay || max_vres >= extra_modes[i].vdisplay) &&
		    ((max_hres * max_vres) >= (extra_modes[i].hdisplay * extra_modes[i].vdisplay))) {
			mode = drm_cvt_mode(dev, extra_modes[i].hdisplay, extra_modes[i].vdisplay,
					    extra_modes[i].vrefresh, false, false, false);
			drm_mode_probed_add(connector, mode);

			count++;
		}
	}

	for (i = 0;
	     mtgpu_get_driver_mode() == MTGPU_DRIVER_MODE_GUEST && i < ARRAY_SIZE(custom_modes);
	     i++) {
		if ((max_hres >= custom_modes[i].hdisplay || max_vres >= custom_modes[i].vdisplay) &&
		    ((max_hres * max_vres) >= (custom_modes[i].hdisplay * custom_modes[i].vdisplay))) {
			mode = drm_mode_duplicate(dev, &custom_modes[i]);
			drm_mode_probed_add(connector, mode);
			count++;
		}
	}

	return count;
}

static void mtdisp_dummy_connector_set_preferred_mode(struct drm_connector *connector,
						      int hdisplay_pref,
						      int vdisplay_pref,
						      int vrefresh_pref)
{
	struct drm_display_mode *mode;

	list_for_each_entry(mode, &connector->probed_modes, head) {
		if (mode->hdisplay != hdisplay_pref)
			continue;

		if (mode->vdisplay != vdisplay_pref)
			continue;

		if (drm_mode_vrefresh(mode) == vrefresh_pref) {
			mode->type |= DRM_MODE_TYPE_PREFERRED;
			break;
		}
	}
}

static int mtdisp_dummy_connector_get_modes(struct drm_connector *connector)
{
	int count = 0;
	struct mtdisp_dummy_connector *dummy_conn = container_of(connector,
								 struct mtdisp_dummy_connector,
								 connector);
	int max_hres = dummy_conn->max_hres;
	int max_vres = dummy_conn->max_vres;

	if (mtgpu_get_driver_mode() != MTGPU_DRIVER_MODE_GUEST)
		count = drm_add_modes_noedid(connector, max_hres, max_vres);

	count += mtdisp_dummy_connector_add_extra_modes(connector, max_hres, max_vres);

	/* set dmt mode 1920x1080@60 preferred */
	mtdisp_dummy_connector_set_preferred_mode(connector, 1920, 1080, 60);

	return count;
}

static struct drm_connector_helper_funcs mtdisp_dummy_connector_helper_funcs = {
	.get_modes = mtdisp_dummy_connector_get_modes,
};

static enum drm_connector_status
mtdisp_dummy_connector_detect(struct drm_connector *connector, bool force)
{
	if (mtgpu_display_is_none())
		return connector_status_disconnected;

	return connector_status_connected;
}

static void mtdisp_dummy_connector_destroy(struct drm_connector *connector)
{
	drm_connector_cleanup(connector);
}

static const struct drm_connector_funcs mtdisp_dummy_connector_funcs = {
	.detect = mtdisp_dummy_connector_detect,
	.fill_modes = drm_helper_probe_single_connector_modes,
	.destroy = mtdisp_dummy_connector_destroy,
	.reset = drm_atomic_helper_connector_reset,
	.atomic_duplicate_state = drm_atomic_helper_connector_duplicate_state,
	.atomic_destroy_state = drm_atomic_helper_connector_destroy_state,
};

static void mtdisp_dummy_encoder_destroy(struct drm_encoder *encoder)
{
	drm_encoder_cleanup(encoder);
}

static const struct drm_encoder_funcs mtdisp_dummy_encoder_funcs = {
	.destroy = mtdisp_dummy_encoder_destroy,
};

int mtdisp_dummy_connector_probe(struct mtdisp_dummy *dummy,
				 struct device *dev, struct drm_device *drm)
{
	int i, ret;
	struct drm_encoder *encoder;
	struct drm_connector *connector;
	struct mtdisp_dummy_connector *dummy_conn;

	DRM_DEV_INFO(dev, "%s()\n", __func__);

	for (i = 0; i < dummy->conn_count; i++) {
		dummy_conn = &dummy->conn[i];

		encoder = &dummy_conn->encoder;
		connector = &dummy_conn->connector;

		dummy_conn->dev = dev;

		ret = drm_encoder_init(drm, encoder, &mtdisp_dummy_encoder_funcs,
				       DRM_MODE_ENCODER_VIRTUAL, NULL);
		if (ret) {
			LINUX_DUMMY_CONN_ERROR("failed to init dummy encoder\n");
			goto err_cleanup_previous;
		}
		encoder->possible_crtcs = 1 << dummy_conn->id;

		ret = drm_connector_init(drm, connector, &mtdisp_dummy_connector_funcs,
				   DRM_MODE_CONNECTOR_VIRTUAL);
		if (ret) {
			LINUX_DUMMY_CONN_ERROR("failed to init dummy connector\n");
			drm_encoder_cleanup(encoder);
			goto err_cleanup_previous;
		}
		drm_connector_helper_add(connector,
					 &mtdisp_dummy_connector_helper_funcs);

		ret = drm_connector_attach_encoder(connector, encoder);
		if (ret) {
			LINUX_DUMMY_CONN_ERROR("Failed to attach connector to encoder\n");
			drm_connector_cleanup(connector);
			drm_encoder_cleanup(encoder);
			goto err_cleanup_previous;
		}
	}

	DRM_DEV_INFO(dev, "dummy connector driver loaded successfully\n");

	return 0;

err_cleanup_previous:
	/* Cleanup all connectors and encoders from previous iterations (0..i-1).
	 * Current iteration has already been cleaned up above on its error path.
	 * Do NOT call drm_connector_cleanup/drm_encoder_cleanup in normal unbind
	 * because drm_mode_config_cleanup (called before component_unbind_all)
	 * has already handled them.
	 */
	while (i > 0) {
		i--;
		dummy_conn = &dummy->conn[i];
		drm_connector_cleanup(&dummy_conn->connector);
		drm_encoder_cleanup(&dummy_conn->encoder);
	}

	return ret;
}

void mtdisp_dummy_connector_remove(struct mtdisp_dummy *dummy, struct device *dev)
{
	// Nothing to do for now
	DRM_DEV_INFO(dev, "dummy connector driver remove successfully\n");
}

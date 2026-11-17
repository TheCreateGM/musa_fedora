/*
 * @Copyright   Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License     Dual MIT/GPLv2
 */

#include <linux/version.h>
#include <linux/component.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/module.h>
#include <sound/hdmi-codec.h>
#if defined(OS_DRM_DRMP_H_EXIST)
#include <drm/drmP.h>
#endif
#include <drm/drm_atomic.h>
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
#if defined(OS_DRM_DRM_DP_MST_HELPER_H_EXIST)
#include <drm/drm_dp_mst_helper.h>
#elif defined(OS_DRM_DISPLAY_DRM_DP_MST_HELPER_H_EXIST)
#include <drm/display/drm_dp_mst_helper.h>
#endif
#include <drm/drm_drv.h>

#include "mtgpu_irq.h"
#include "mtgpu_board_cfg.h"
#include "mtgpu_module_param.h"
#include "module_common.h"

#include "mtdisp_dp.h"

#include "dm_hwlib_access.h"

#include "linux_data_convert.h"
#include "linux_dispc_drv.h"
#include "linux_dp_drv.h"
#include "linux_dp_mst_drv.h"

static inline
struct mtdisp_dp_mst *connector_to_mtdisp_dp_mst(struct drm_connector *connector)
{
	return container_of(connector, struct mtdisp_dp_mst, connector);
}

static inline
struct mtdisp_dp_mst *encoder_to_mtdisp_dp_mst(struct drm_encoder *encoder)
{
	return container_of(encoder, struct mtdisp_dp_mst, encoder);
}

static void mtdisp_dp_mst_connector_destroy(struct drm_connector *connector)
{
	struct mtdisp_dp_mst *dp_mst;

	if (!connector)
		return;

	dp_mst = connector_to_mtdisp_dp_mst(connector);
	if (!dp_mst)
		return;

	kfree(dp_mst->edid);

	drm_encoder_cleanup(&dp_mst->encoder);
	drm_connector_cleanup(connector);

#if defined(OS_STRUCT_DRM_DP_MST_PORT_HAS_MALLOC_KREF)
	drm_dp_mst_put_port_malloc(dp_mst->port);
#endif
	kfree(dp_mst);
}

static const struct drm_connector_funcs mtdisp_dp_mst_connector_funcs = {
	.dpms = drm_helper_connector_dpms,
	.fill_modes = drm_helper_probe_single_connector_modes,
	.destroy = mtdisp_dp_mst_connector_destroy,
	.atomic_duplicate_state	= drm_atomic_helper_connector_duplicate_state,
	.atomic_destroy_state = drm_atomic_helper_connector_destroy_state,
	.reset = drm_atomic_helper_connector_reset,
};

static int
mtdisp_dp_mst_connector_detect(struct drm_connector *connector,
			       struct drm_modeset_acquire_ctx *ctx, bool force)
{
	int status = connector_status_unknown;
	struct mtdisp_dp_mst *dp_mst = connector_to_mtdisp_dp_mst(connector);

#if defined(OS_DRM_DP_MST_DETECT_PORT_HAS_FOUR_ARGS)
	status = drm_dp_mst_detect_port(connector, ctx, dp_mst->mgr, dp_mst->port);
#elif defined(OS_DRM_DP_MST_DETECT_PORT_HAS_THREE_ARGS)
	status = drm_dp_mst_detect_port(connector, dp_mst->mgr, dp_mst->port);
#endif

	if (status != connector_status_connected && dp_mst->edid) {
		kfree(dp_mst->edid);
		dp_mst->edid = NULL;
		drm_connector_update_edid_property(connector, NULL);
	}

	return status;
}

static int mtdisp_dp_mst_get_modes(struct drm_connector *connector)
{
	struct mtdisp_dp_mst *dp_mst = connector_to_mtdisp_dp_mst(connector);
	struct edid *edid;

	if (!dp_mst->edid) {
		edid = drm_dp_mst_get_edid(connector, dp_mst->mgr, dp_mst->port);
		if (edid) {
			dp_mst->edid = edid;
			drm_connector_update_edid_property(connector, edid);
			return drm_add_edid_modes(connector, edid);
		}
	} else {
		return drm_add_edid_modes(connector, dp_mst->edid);
	}

	return 0;
}

static int mtdisp_dp_mst_mode_supported_by_product(struct mtdisp_dp_mst *dp_mst,
						   struct drm_display_mode *mode)
{
	struct mtdisp_dp *dp_mst_parent = dp_mst->dp_mst_parent;
	u32 dp_max_pclk;
	u32 max_hres = mtdisp_get_dp_src_dsc_caps_max_hres(dp_mst_parent->hwlib_dp);
	u32 max_vres = mtdisp_get_dp_src_dsc_caps_max_vres(dp_mst_parent->hwlib_dp);
	u32 max_pclk_100khz = mtdisp_get_dp_max_pclk(dp_mst_parent->hwlib_dp);
	u32 max_rate = mtdisp_get_dp_ctx_bw_code(dp_mst_parent->hwlib_dp) * 27000;
	u8 lane_cnt = mtdisp_get_dp_ctx_lane_cnt(dp_mst_parent->hwlib_dp);

	if (mode->hdisplay > max_hres)
		return MODE_H_ILLEGAL;

	if (mode->vdisplay > max_vres)
		return MODE_V_ILLEGAL;

	if (mode->clock > max_pclk_100khz * 100)
		return MODE_CLOCK_HIGH;

	dp_max_pclk = mtdisp_dp_link_rate_to_pclk(max_rate, lane_cnt, 24);

	if (mode->clock > dp_max_pclk)
		return MODE_CLOCK_HIGH;

	return MODE_OK;
}

#ifdef OS_DRM_CONNECTOR_MODE_VALID_CONST
static enum drm_mode_status
mtdisp_dp_mst_mode_valid(struct drm_connector *connector,
			 const struct drm_display_mode *mode)
#else
static enum drm_mode_status
mtdisp_dp_mst_mode_valid(struct drm_connector *connector,
			 struct drm_display_mode *mode)
#endif
{
	int mode_status;
	struct videomode vm;
	struct mtdisp_dp_mst *dp_mst = connector_to_mtdisp_dp_mst(connector);

	mode_status = mtdisp_dp_mst_mode_supported_by_product(dp_mst, mode);
	if (mode_status != MODE_OK)
		return mode_status;

	drm_display_mode_to_videomode(mode, &vm);

	return MODE_OK;
}

static struct drm_encoder *mtdisp_dp_mst_best_encoder(struct drm_connector *connector)
{
	struct mtdisp_dp_mst *dp_mst = connector_to_mtdisp_dp_mst(connector);

	return &dp_mst->encoder;
}

static const struct drm_connector_helper_funcs mtdisp_dp_mst_connector_helper_funcs = {
	.detect_ctx = mtdisp_dp_mst_connector_detect,
	.get_modes = mtdisp_dp_mst_get_modes,
	.mode_valid = mtdisp_dp_mst_mode_valid,
	.best_encoder = mtdisp_dp_mst_best_encoder,
};

/* TODO: version control should not be used, it will be changed later */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0))
static int mtdisp_dp_atomic_find_time_slots(struct mtdisp_dp_mst *dp_mst,
					    struct drm_atomic_state *state,
					    struct drm_dp_mst_topology_mgr *mgr,
					    struct drm_dp_mst_port *port,
					    int pbn,
					    u8 stream_id)
{
	struct drm_dp_mst_topology_state *topology_state;
	struct drm_dp_mst_atomic_payload *payload = NULL;
	int prev_slots = 0, prev_bw = 0, req_slots;

	topology_state = drm_atomic_get_mst_topology_state(state, mgr);
	if (IS_ERR(topology_state))
		return PTR_ERR(topology_state);

	LINUX_MST_INFO("find old payload topology_state 0x%llx\n", (u64)topology_state);

	if (!port->connector)
		LINUX_MST_INFO("port->connector NULL\n");

	if (!port->connector->encoder)
		LINUX_MST_INFO("port->connector->encoder NULL\n");

	if (!port->connector->encoder->crtc)
		LINUX_MST_INFO("port->connector->encoder->crtc NULL\n");

	topology_state->pending_crtc_mask |= drm_crtc_mask(port->connector->encoder->crtc);

	/* Find the current allocation for this port, if any */
	payload = drm_atomic_get_mst_payload_state(topology_state, port);
	if (payload) {
		LINUX_MST_INFO("find old payload 0x%llx\n", (u64)payload);
		prev_slots = payload->time_slots;
		prev_bw = payload->pbn;

		/*
		 * This should never happen, unless the driver tries
		 * releasing and allocating the same timeslot allocation,
		 * which is an error
		 */
		if (drm_WARN_ON(mgr->dev, payload->delete)) {
			drm_err(mgr->dev,
				"cannot allocate and release time slots on [MST PORT:%p] in the same state\n",
				port);
			return -EINVAL;
		}
	}

#ifdef OS_DRM_DP_MST_TOPOLOGY_STATE_HAS_PBN_DIV_HAS_FULL
	LINUX_MST_INFO("topology_state->pbn_div.full: %d\n", topology_state->pbn_div.full);
	if (!topology_state->pbn_div.full) {
		LINUX_MST_ERROR("pbn_div cannot be 0\n");
		return -EINVAL;
	}
	req_slots = pbn / topology_state->pbn_div.full;
#else
	LINUX_MST_INFO("topology_state->pbn_div: %d\n", topology_state->pbn_div);
	if (!topology_state->pbn_div) {
		LINUX_MST_ERROR("pbn_div cannot be 0\n");
		return -EINVAL;
	}
	req_slots = pbn / topology_state->pbn_div;
#endif

	drm_dbg_atomic(mgr->dev, "[CONNECTOR:%d:%s] [MST PORT:%p] TU %d -> %d\n",
		       port->connector->base.id, port->connector->name,
		       port, prev_slots, req_slots);
	drm_dbg_atomic(mgr->dev, "[CONNECTOR:%d:%s] [MST PORT:%p] PBN %d -> %d\n",
		       port->connector->base.id, port->connector->name,
		       port, prev_bw, pbn);

	/* Add the new allocation to the state, note the VCPI isn't assigned until the end */
	if (!payload) {
		payload = kzalloc(sizeof(*payload), GFP_KERNEL);
		if (!payload)
			return -ENOMEM;

		drm_dp_mst_get_port_malloc(port);
		payload->port = port;
		payload->vc_start_slot = -1;
		list_add(&payload->next, &topology_state->payloads);
	}
	payload->time_slots = req_slots;
	payload->pbn = pbn;
	if (!payload->vcpi) {
		payload->vcpi = BIT(stream_id);
		topology_state->payload_mask |= BIT(payload->vcpi - 1);
	}

	return req_slots;
}

static void mtdisp_dp_mst_encoder_atomic_enable(struct drm_encoder *encoder,
						struct drm_atomic_state *state)
{
	int ret;
	u32 pbn;
	u32 pbn_div;
	int slots;
	struct mtdisp_videomode mt_vm;
	struct mtdisp_dp_mst *dp_mst = encoder_to_mtdisp_dp_mst(encoder);
	struct drm_dp_mst_topology_mgr *mgr = dp_mst->mgr;
	struct drm_dp_mst_port *port = dp_mst->port;
	struct mtdisp_dp *dp = container_of(mgr, struct mtdisp_dp, mst_mgr);
	struct drm_dp_mst_topology_state *mst_state = NULL;
	struct drm_dp_mst_atomic_payload *payload = NULL;
	unsigned int stream_id = dp_mst->stream_id;
#if defined(OS_DRM_DP_CALC_PBN_MODE_HAS_THREE_ARGS)
	bool dsc_en = mtdisp_get_dp_ctx_mt_dsc_config_dsc_en(dp->hwlib_dp);
#endif

	mtdisp_get_dp_ctx_vm(&mt_vm, dp->hwlib_dp, stream_id);

	/* set pclk, bpp, active */
	mtdisp_set_dp_ctx_modes_pclk_khz(dp->hwlib_dp, stream_id, mt_vm.pixelclock / 1000);
	mtdisp_set_dp_ctx_modes_bpp(dp->hwlib_dp, stream_id, 24);
	mtdisp_set_dp_ctx_modes_active(dp->hwlib_dp, stream_id, true);

	dp->port[stream_id] = port;

	mst_state = drm_atomic_get_mst_topology_state(state, mgr);
	if (mst_state) {
		LINUX_MST_INFO("mst_state:0x%llx\n", (u64)mst_state);
	} else {
		LINUX_MST_INFO("mst_state NULL\n");
		return;
	}

	drm_dp_mst_update_slots(mst_state, DP_CAP_ANSI_8B10B);

	ret = drm_dp_send_power_updown_phy(mgr, port, true);
	LINUX_MST_INFO("drm_dp_send_power_updown_phy on status %d\n", ret);

#if defined(OS_DRM_DP_CALC_PBN_MODE_HAS_THREE_ARGS)
	pbn = drm_dp_calc_pbn_mode(mt_vm.pixelclock / 1000, 24 << 4, dsc_en);
#else
	pbn = drm_dp_calc_pbn_mode(mt_vm.pixelclock / 1000, 24);
#endif
	LINUX_MST_INFO("pbn %d\n", pbn);

	pbn_div = mtdisp_get_dp_ctx_bw_code(dp->hwlib_dp) *
		  mtdisp_get_dp_ctx_lane_cnt(dp->hwlib_dp) / 2;
#ifdef OS_DRM_DP_MST_TOPOLOGY_STATE_HAS_PBN_DIV_HAS_FULL
	mst_state->pbn_div.full = pbn_div;
#else
	mst_state->pbn_div = pbn_div;
#endif
	LINUX_MST_INFO("pbn_div %d\n", pbn_div);

	slots = mtdisp_dp_atomic_find_time_slots(dp_mst, state, mgr, port, pbn, stream_id);
	LINUX_MST_INFO("slots %d\n", slots);
	if (slots >= 0) {
		ret = drm_dp_mst_atomic_check(state);
		if (ret < 0) {
			LINUX_MST_ERROR("state check error, ret %d\n", ret);
			return;
		}
	} else {
		LINUX_MST_ERROR("invalid slots %d\n", slots);
		return;
	}

	mtdisp_dp_core_stream_config(dp->hwlib_dp, stream_id);

	mtdisp_dp_core_clear_payload_table(dp->hwlib_dp);

	payload = drm_atomic_get_mst_payload_state(mst_state, port);
	ret = drm_dp_add_payload_part1(mgr, mst_state, payload);
	if (ret < 0)
		LINUX_MST_ERROR("Failed to create MST payload: %d\n", ret);

	memcpy(&dp_mst->payload[stream_id], payload, sizeof(*payload));

	mtdisp_dp_core_allocate_payload(dp->hwlib_dp,
					stream_id,
					payload->vc_start_slot,
					payload->time_slots);

	LINUX_MST_INFO("mtdisp-dp-mst-%d: vcpi: %u, stream_id: %d, vc_start_slot: %d, time_slots: %d\n",
		       stream_id, payload->vcpi, stream_id,
		       payload->vc_start_slot, payload->time_slots);

	mtdisp_dp_core_trigger_act(dp->hwlib_dp);

	ret = drm_dp_check_act_status(mgr);
	LINUX_MST_INFO("act status %d\n", ret);

#if defined(OS_DRM_DP_ADD_PAYLOAD_PART2_HAS_TWO_ARGS)
	ret = drm_dp_add_payload_part2(mgr, payload);
#elif defined(OS_DRM_DP_ADD_PAYLOAD_PART2_HAS_THREE_ARGS)
	ret = drm_dp_add_payload_part2(mgr, state, payload);
#endif
	LINUX_MST_INFO("update payload part2 status %d\n", ret);

	mtdisp_dp_core_enable(dp->hwlib_dp, stream_id);
}

static void mtdisp_dp_mst_encoder_atomic_disable(struct drm_encoder *encoder,
						 struct drm_atomic_state *state)
{
	int ret;
	struct mtdisp_dp_mst *dp_mst = encoder_to_mtdisp_dp_mst(encoder);
	struct drm_dp_mst_topology_mgr *mgr = dp_mst->mgr;
	struct drm_dp_mst_port *port = dp_mst->port;
	struct mtdisp_dp *dp = container_of(mgr, struct mtdisp_dp, mst_mgr);
	struct drm_dp_mst_topology_state *mst_state = NULL;
	struct drm_dp_mst_topology_state *new_mst_state = NULL;
	struct drm_dp_mst_atomic_payload *new_payload = NULL;
	const struct drm_dp_mst_atomic_payload *old_payload = NULL;

	unsigned int stream_id = dp_mst->stream_id;

	mtdisp_dp_core_disable(dp->hwlib_dp, stream_id);
	mtdisp_dp_core_clear_payload_table(dp->hwlib_dp);

	mst_state = drm_atomic_get_mst_topology_state(state, mgr);

	old_payload = &dp_mst->payload[stream_id];
	new_payload = &dp_mst->payload[stream_id];

#if defined(OS_FUNC_DRM_DP_REMOVE_PAYLOAD_EXIST)
	drm_dp_remove_payload(mgr, new_mst_state,
			      old_payload, new_payload);
#elif (defined(OS_FUNC_DRM_DP_REMOVE_PAYLOAD_PART1_EXIST) && defined(OS_FUNC_DRM_DP_REMOVE_PAYLOAD_PART2_EXIST))
	drm_dp_remove_payload_part1(mgr, new_mst_state, new_payload);
	drm_dp_remove_payload_part2(mgr, new_mst_state,
				    old_payload, new_payload);
#endif

	mtdisp_dp_core_trigger_act(dp->hwlib_dp);

	ret = drm_dp_check_act_status(mgr);
	LINUX_MST_INFO("act status %d\n", ret);

	ret = drm_dp_send_power_updown_phy(mgr, port, false);
	LINUX_MST_INFO("drm_dp_send_power_updown_phy on status %d\n", ret);

	mtdisp_set_dp_ctx_modes_active(dp->hwlib_dp, stream_id, false);
}

#else

static void mtdisp_mst_mgr_payload_reset(struct mtdisp_dp *dp)
{
	struct drm_dp_mst_topology_mgr *mgr = &dp->mst_mgr;

	memset(mgr->payloads, 0, mgr->max_payloads * sizeof(struct drm_dp_payload));
	mgr->payload_mask = 0;
	set_bit(0, &mgr->payload_mask);
	mgr->vcpi_mask = 0;
}

static void mtdisp_dp_mst_encoder_enable(struct drm_encoder *encoder)
{
	int i;
	int ret;
	bool alloc_ret;
	struct mtdisp_videomode mt_vm;
	struct mtdisp_dp_mst *dp_mst = encoder_to_mtdisp_dp_mst(encoder);
	struct drm_dp_mst_topology_mgr *mgr = dp_mst->mgr;
	struct drm_display_mode *display_mode =
				&dp_mst->connector.state->crtc->state->adjusted_mode;
	struct mtdisp_dp *dp = container_of(mgr, struct mtdisp_dp, mst_mgr);
	unsigned int stream = dp_mst->stream_id;

	LINUX_MST_INFO(DRM_MODE_FMT "\n", DRM_MODE_ARG(display_mode));
	mtdisp_get_dp_ctx_vm(&mt_vm, dp->hwlib_dp, stream);

	/* set pclk, bpp, active */
	mtdisp_set_dp_ctx_modes_pclk_khz(dp->hwlib_dp, stream, mt_vm.pixelclock / 1000);
	mtdisp_set_dp_ctx_modes_bpp(dp->hwlib_dp, stream, 24);
	mtdisp_set_dp_ctx_modes_active(dp->hwlib_dp, stream, true);

	dp->port[stream] = dp_mst->port;

	mtdisp_mst_mgr_payload_reset(dp);

	for (i = 0; i < mgr->max_payloads; i++) {
		int pbn, slots, pbn_div;
		struct drm_dp_mst_port *port = dp->port[i];
		bool active = mtdisp_get_dp_ctx_modes_active(dp->hwlib_dp, i);
		int pclk_khz = mtdisp_get_dp_ctx_modes_pclk_khz(dp->hwlib_dp, i);
#if defined(OS_DRM_DP_CALC_PBN_MODE_HAS_THREE_ARGS)
		bool dsc_en = mtdisp_get_dp_ctx_mt_dsc_config_dsc_en(dp->hwlib_dp);
#endif

		if (!active)
			continue;

		drm_dp_send_power_updown_phy(mgr, port, true);

		mtdisp_dp_core_stream_config(dp->hwlib_dp, i);

		mtdisp_dp_core_clear_payload_table(dp->hwlib_dp);

#if defined(OS_DRM_DP_CALC_PBN_MODE_HAS_THREE_ARGS)
		pbn = drm_dp_calc_pbn_mode(pclk_khz, 24, dsc_en);
#elif defined(OS_DRM_DP_CALC_PBN_MODE_HAS_TWO_ARGS)
		pbn = drm_dp_calc_pbn_mode(pclk_khz, 24);
#endif
		mtdisp_set_dp_ctx_modes_pbn(dp->hwlib_dp, i, pbn);

		slots = drm_dp_find_vcpi_slots(mgr, pbn);
		mtdisp_set_dp_ctx_modes_slots(dp->hwlib_dp, i, slots);

		pbn_div = mgr->pbn_div;
		mtdisp_set_dp_ctx_modes_pbn_div(dp->hwlib_dp, i, pbn_div);

		drm_dp_mst_reset_vcpi_slots(mgr, port);
		port->vcpi.vcpi = 0;

		alloc_ret = drm_dp_mst_allocate_vcpi(mgr, port, pbn, slots);

		drm_dp_update_payload_part1(mgr);
	}

	for (i = 0; i < mgr->max_payloads; i++) {
		struct drm_dp_vcpi *vcpi = mgr->proposed_vcpis[i];
		struct drm_dp_payload *payload = &mgr->payloads[i];
		struct drm_dp_mst_port *port;
		struct drm_crtc *crtc;
		u8 index;

		if (!vcpi || !vcpi->num_slots)
			continue;

		port = container_of(vcpi, struct drm_dp_mst_port, vcpi);
		if (!port || !port->connector->state->crtc)
			continue;

		crtc = port->connector->state->crtc;
		if (!crtc)
			continue;

		if ((payload->start_slot + payload->num_slots) >= 64) {
			MTDISP_INFO("mtdisp-dp-mst-%d: too many slots: (%d) >= 64\n",
				    i, (payload->start_slot + payload->num_slots));
			continue;
		}

		index = drm_crtc_index(crtc);
		mtdisp_dp_core_allocate_payload(dp->hwlib_dp,
						index,
						payload->start_slot,
						payload->num_slots);

		MTDISP_INFO("mtdisp-dp-mst-%d: i:%d, vcpi:%u, stream:%d, start_slot:%d, num_slots:%d\n",
			    i, payload->vcpi, index, payload->start_slot, payload->num_slots);
	}

	mtdisp_dp_core_trigger_act(dp->hwlib_dp);

	ret = drm_dp_check_act_status(mgr);

	ret = drm_dp_update_payload_part2(mgr);

	for (i = 0; i < mgr->max_payloads; i++) {
		bool active = mtdisp_get_dp_ctx_modes_active(dp->hwlib_dp, i);

		if (!active)
			continue;

		mtdisp_dp_core_enable(dp->hwlib_dp, i);
	}
}

static void mtdisp_dp_mst_encoder_disable(struct drm_encoder *encoder)
{
	struct mtdisp_dp_mst *dp_mst = encoder_to_mtdisp_dp_mst(encoder);
	struct drm_dp_mst_port *port = dp_mst->port;
	struct drm_dp_mst_topology_mgr *mgr = dp_mst->mgr;
	struct mtdisp_dp *dp = container_of(mgr, struct mtdisp_dp, mst_mgr);
	int i;
	unsigned int stream = dp_mst->stream_id;

	mtdisp_dp_core_disable(dp->hwlib_dp, stream);

	mtdisp_dp_core_clear_payload_table(dp->hwlib_dp);

	drm_dp_mst_reset_vcpi_slots(mgr, port);
	port->vcpi.vcpi = 0;

	drm_dp_update_payload_part1(mgr);

	for (i = 0; i < mgr->max_payloads; i++) {
		struct drm_dp_vcpi *vcpi = mgr->proposed_vcpis[i];
		struct drm_dp_payload *payload = &mgr->payloads[i];
		struct drm_dp_mst_port *port;
		struct drm_crtc *crtc;

		if (!vcpi || !vcpi->num_slots)
			continue;

		port = container_of(vcpi, struct drm_dp_mst_port, vcpi);
		if (!port || !port->connector->state->crtc)
			continue;

		crtc = port->connector->state->crtc;
		if (!crtc)
			continue;

		LINUX_MST_INFO(" vcpi:%u, i:%d, stream:%d, start_slot:%d, num_slots:%d\n",
			     payload->vcpi, i, drm_crtc_index(crtc), payload->start_slot,
			     payload->num_slots);

		mtdisp_dp_core_allocate_payload(dp->hwlib_dp,
						drm_crtc_index(crtc),
						payload->start_slot,
						payload->num_slots);
	}

	mtdisp_dp_core_trigger_act(dp->hwlib_dp);

	drm_dp_check_act_status(mgr);

	drm_dp_update_payload_part2(mgr);

	drm_dp_mst_deallocate_vcpi(mgr, port);

	mtdisp_set_dp_ctx_modes_active(dp->hwlib_dp, stream, false);
}
#endif

static void
mtdisp_dp_mst_encoder_atomic_mode_set(struct drm_encoder *encoder,
				      struct drm_crtc_state *crtc_state,
				      struct drm_connector_state *connector_state)
{
	struct mtdisp_dp_mst *dp_mst = encoder_to_mtdisp_dp_mst(encoder);
	struct drm_dp_mst_topology_mgr *mgr = dp_mst->mgr;
	struct mtdisp_dp *dp = container_of(mgr, struct mtdisp_dp, mst_mgr);
	struct drm_display_mode *adjusted_mode = &crtc_state->adjusted_mode;
	unsigned int stream = drm_crtc_index(dp_mst->connector.state->crtc);
	struct mtdisp_videomode mt_vm;
	struct videomode vm;

	dp_mst->stream_id = stream;
	LINUX_MST_TRACE();

	LINUX_MST_INFO(DRM_MODE_FMT "\n", DRM_MODE_ARG(adjusted_mode));

	drm_display_mode_to_videomode(adjusted_mode, &vm);
	mtdisp_videomode_from_os_videomode(&mt_vm, &vm);
	mtdisp_set_dp_ctx_vm(dp->hwlib_dp, &mt_vm, stream);

	return;
}

static bool
mtdisp_dp_mst_encoder_mode_fixup(struct drm_encoder *encoder,
				 const struct drm_display_mode *mode,
				 struct drm_display_mode *adjusted_mode)
{
	drm_mode_set_crtcinfo(adjusted_mode, 0);

	return true;
}

static const struct drm_encoder_helper_funcs mtdisp_dp_mst_encoder_helper_funcs = {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0))
	.atomic_enable		= mtdisp_dp_mst_encoder_atomic_enable,
	.atomic_disable		= mtdisp_dp_mst_encoder_atomic_disable,
#else
	.enable			= mtdisp_dp_mst_encoder_enable,
	.disable		= mtdisp_dp_mst_encoder_disable,
#endif
	.atomic_mode_set	= mtdisp_dp_mst_encoder_atomic_mode_set,
	.mode_fixup		= mtdisp_dp_mst_encoder_mode_fixup,
};

static void mtdisp_dp_mst_encoder_destroy(struct drm_encoder *encoder)
{
}

static const struct drm_encoder_funcs mtdisp_dp_mst_encoder_funcs = {
	.destroy = mtdisp_dp_mst_encoder_destroy,
};

static struct drm_connector *mtdisp_dp_mst_add_connector(struct drm_dp_mst_topology_mgr *mgr,
							struct drm_dp_mst_port *port,
							const char *path)
{
	struct mtdisp_dp *master = container_of(mgr, struct mtdisp_dp, mst_mgr);
	struct drm_device *dev = master->connector.dev;
	struct mtdisp_dp_mst *dp_mst;
	struct drm_connector_state *state;
	struct mtdisp_dp *dp_mst_parent = container_of(mgr, struct mtdisp_dp, mst_mgr);

	pr_info("%s()\n", __func__);

	dp_mst = kzalloc(sizeof(*dp_mst), GFP_KERNEL);
	if (!dp_mst)
		return NULL;

	state = kzalloc(sizeof(*state), GFP_KERNEL);
	if (!state) {
		kfree(dp_mst);
		return NULL;
	}

	dp_mst->mgr = mgr;
	dp_mst->port = port;
	dp_mst->dp_mst_parent = dp_mst_parent;

	__drm_atomic_helper_connector_reset(&dp_mst->connector,
					    state);

	dp_mst->encoder.possible_crtcs = (1 << mgr->max_payloads) - 1;
	drm_encoder_init(dev, &dp_mst->encoder, &mtdisp_dp_mst_encoder_funcs,
			 DRM_MODE_ENCODER_DPMST, NULL);
	drm_encoder_helper_add(&dp_mst->encoder, &mtdisp_dp_mst_encoder_helper_funcs);

	drm_connector_init(dev, &dp_mst->connector, &mtdisp_dp_mst_connector_funcs,
			   DRM_MODE_CONNECTOR_DisplayPort);
	drm_connector_helper_add(&dp_mst->connector, &mtdisp_dp_mst_connector_helper_funcs);

	drm_connector_attach_encoder(&dp_mst->connector, &dp_mst->encoder);

	drm_object_attach_property(&dp_mst->connector.base, dev->mode_config.path_property, 0);
	drm_object_attach_property(&dp_mst->connector.base, dev->mode_config.tile_property, 0);
	drm_connector_set_path_property(&dp_mst->connector, path);

#if defined(OS_STRUCT_DRM_DP_MST_PORT_HAS_MALLOC_KREF)
	drm_dp_mst_get_port_malloc(port);
#endif
	return &dp_mst->connector;
}

static const struct drm_dp_mst_topology_cbs mst_cbs = {
	.add_connector = mtdisp_dp_mst_add_connector,
};

int mtdisp_dp_mst_hpd_handler(struct mtdisp_dp *dp)
{
	bool handled = true;
	u8 esi[8] = {0};
#if !defined(OS_FUNC_DRM_DP_MST_HPD_IRQ_EXIST)
	u8 ack[4] = {0};
#endif
	int ret;

	LINUX_DP_TRACE();

	while (handled) {
		ret = drm_dp_dpcd_read(&dp->aux, DP_SINK_COUNT_ESI, esi, 8);
		if (ret != 8) {
			mtdisp_dp_core_set_mst(dp->hwlib_dp, false);

			drm_dp_mst_topology_mgr_set_mst(&dp->mst_mgr, false);

			mtdisp_set_dp_enabled(dp->hwlib_dp, false);

			LINUX_DP_INFO("Failed to get ESI\n");

			return 0;
		}

#if defined(OS_FUNC_DRM_DP_MST_HPD_IRQ_EXIST)
		drm_dp_mst_hpd_irq(&dp->mst_mgr, esi, &handled);
#else
		drm_dp_mst_hpd_irq_handle_event(&dp->mst_mgr, esi, ack, &handled);
#endif
		if (!handled)
			break;

		drm_dp_dpcd_write(&dp->aux, DP_SINK_COUNT_ESI + 1, &esi[1], 3);
	}

	return 0;
}

int mtdisp_dp_mst_probe(struct mtdisp_dp *dp)
{
	int ret;
	struct drm_device *dev = dp->connector.dev;

	LINUX_DP_INFO("%s() dp: %d\n", __func__, dp->id);

	dp->mst_mgr.max_payloads = mtdisp_get_dp_ctx_max_payloads(dp->hwlib_dp);
	dp->mst_mgr.cbs = &mst_cbs;

	ret = drm_dp_mst_topology_mgr_init(&dp->mst_mgr,
					   dev,
					   &dp->aux,
					   16,
					   dp->mst_mgr.max_payloads,
#if defined(OS_DRM_DP_MST_TOPOLOGY_MGR_INIT_HAS_EIGHT_ARGS)
					   mtdisp_get_dp_ctx_src_max_lanes(dp->hwlib_dp),
					   mtdisp_get_dp_ctx_src_max_rate(dp->hwlib_dp),
#endif
					   dp->connector.base.id);
	if (ret)
		LINUX_DP_ERROR("Failed to init mst_topology_mgr\n");

	return ret;
}

void mtdisp_dp_mst_remove(struct mtdisp_dp *dp)
{
	LINUX_DP_TRACE();

	drm_dp_mst_topology_mgr_destroy(&dp->mst_mgr);

	if (mtdisp_get_dp_ctx_mst_en(dp->hwlib_dp)) {
		mtdisp_dp_core_set_mst(dp->hwlib_dp, false);
		mtdisp_set_dp_enabled(dp->hwlib_dp, false);
		mtdisp_dp_core_disable(dp->hwlib_dp, 0);
	}
}

int mtdisp_dp_mst_plug_in(struct mtdisp_dp *dp)
{
	int ret;

	LINUX_DP_TRACE();

	if (mtdisp_get_dp_ctx_mst_en(dp->hwlib_dp))
		return 0;

	ret = mtdisp_dp_mst_enable(dp->hwlib_dp);
	if (ret) {
		LINUX_DP_ERROR("Failed to enable mst\n");
		return ret;
	}

	ret = drm_dp_mst_topology_mgr_set_mst(&dp->mst_mgr, true);
	if (ret) {
		LINUX_DP_ERROR("Failed to set mst\n");
		return ret;
	}

	mtdisp_dp_core_set_mst(dp->hwlib_dp, true);

	return 0;
}

void mtdisp_dp_mst_unplug(struct mtdisp_dp *dp)
{
	LINUX_DP_TRACE();

	drm_dp_mst_topology_mgr_set_mst(&dp->mst_mgr, false);

	mtdisp_dp_mst_disable(dp->hwlib_dp);
}

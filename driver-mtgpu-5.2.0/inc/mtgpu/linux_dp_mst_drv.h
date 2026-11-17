/*
 * @Copyright   Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License     Dual MIT/GPLv2
 */

#ifndef __LINUX_DP_MST_DRV_H__
#define __LINUX_DP_MST_DRV_H__

#include <linux/printk.h>
#if defined(OS_DRM_DRM_DP_MST_HELPER_H_EXIST)
#include <drm/drm_dp_mst_helper.h>
#elif defined(OS_DRM_DISPLAY_DRM_DP_MST_HELPER_H_EXIST)
#include <drm/display/drm_dp_mst_helper.h>
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(6, 8, 0)
/* Newer kernels moved the MST helper under include/drm/display/. */
#include <drm/display/drm_dp_mst_helper.h>
#endif

#include "linux_display_debug.h"

struct mtdisp_dp;

struct mtdisp_dp_mst {
	unsigned int stream_id;

	struct drm_encoder encoder;
	struct drm_connector connector;

	struct drm_dp_mst_port *port;
	struct drm_dp_mst_topology_mgr *mgr;
#if defined(OS_STRUCT_DRM_DP_MST_ATOMIC_PAYLOAD_EXIST)
	struct drm_dp_mst_atomic_payload payload[4];
#endif
	struct mtdisp_dp *dp_mst_parent;

	struct edid *edid;
};

int mtdisp_dp_mst_hpd_handler(struct mtdisp_dp *dp);
int mtdisp_dp_mst_probe(struct mtdisp_dp *dp);
void mtdisp_dp_mst_remove(struct mtdisp_dp *dp);
int  mtdisp_dp_mst_plug_in(struct mtdisp_dp *dp);
void mtdisp_dp_mst_unplug(struct mtdisp_dp *dp);

#endif /* __LINUX_DP_MST_DRV_H__ */

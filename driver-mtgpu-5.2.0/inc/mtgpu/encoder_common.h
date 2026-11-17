/*
 * @Copyright   Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License     Dual MIT/GPLv2
 */

#ifndef __ENCODER_COMMON_H__
#define __ENCODER_COMMON_H__

#define MTDISP_DP_IRQ_TYPE_PLUGIN_EVENT 	BIT(0)
#define MTDISP_DP_IRQ_TYPE_PLUGOUT_EVENT	BIT(1)
#define MTDISP_DP_IRQ_TYPE_HPD_IRQ		BIT(2)

#define MTDISP_HDMI_IRQ_TYPE_HPD_IN	BIT(0)
#define MTDISP_HDMI_IRQ_TYPE_HPD_OUT	BIT(1)

#define DELAY_ON_HDMI_PLUGOUT_DETECTION_MS (1500)

#define MTDISP_PORT_DISABLED		0
/* DP port */
#define MTDISP_PORT_TYPE_DP		1
#define MTDISP_PORT_TYPE_EDP		2
#define MTDISP_PORT_TYPE_DP2VGA 	3
#define MTDISP_PORT_TYPE_DP2HDMI	4
#define MTDISP_PORT_TYPE_DP2LVDS	5
/* HDMI port */
#define MTDISP_PORT_TYPE_HDMIA		1
#define MTDISP_PORT_TYPE_HDMIA2VGA	2
#define MTDISP_PORT_TYPE_HDMIA2LVDS	3
#define MTDISP_PORT_TYPE_HDMIA2DP	4

#define MAX_DP_MST_STREAMS 4

enum mtdisp_encoder_type {
	MTDISP_ENCODER_HDMI = 0,
	MTDISP_ENCODER_DP,
	MTDISP_ENCODER_EDP,
	MTDISP_ENCODER_DP_MST,
	MTDISP_ENCODER_VIRTUAL_PANEL,
	MTDISP_ENCODER_VIRTUAL_DISPLAY,
};

struct dp_colorimetry_info {
	u32 red_point_x;
	u32 red_point_y;
	u32 green_point_x;
	u32 green_point_y;
	u32 blue_point_x;
	u32 blue_point_y;
	u32 white_point_x;
	u32 white_point_y;

	// Luminance values in (1 / 10000) nits
	u32 min_luminace;
	u32 max_luminace;
	u32 max_full_frame_luminance;
};

#endif /* __ENCODER_COMMON_H__ */

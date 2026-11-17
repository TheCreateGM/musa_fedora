/*
 * @Copyright   Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License     Dual MIT/GPLv2
 */

#ifndef __MTDISP_HDMI_H__
#define __MTDISP_HDMI_H__

#include "display_types.h"
#include "hdmi_types.h"

#define SIMPLE_VIDEO_MODE(pl, ha, hfp, hs, hbp, va, vfp, vs, vbp)                                  \
	.pixelclock = (pl), .hactive = (ha), .hfront_porch = (hfp), .hback_porch = (hbp),          \
	.hsync_len = (hs), .vactive = (va), .vfront_porch = (vfp), .vback_porch = (vbp),           \
	.vsync_len = (vs),

extern struct mtdisp_videomode hdmi_support_mode_with_audio[];
extern const int hdmi_support_mode_count;

void mtdisp_hdmi_ctx_init(void *hwlib_hdmi);
void mtdisp_hdmi_ctx_deinit(void *hwlib_hdmi);
void mtdisp_hdmi_core_hw_init(void *hwlib_hdmi);
void mtdisp_hdmi_core_hw_deinit(void *hwlib_hdmi);
void mtdisp_hdmi_core_hpd_enable(void *hwlib_hdmi);
u32 mtdisp_hdmi_core_isr(void *hwlib_hdmi);
void mtdisp_hdmi_core_config(void *hwlib_hdmi);
void mtdisp_hdmi_core_enable(void *hwlib_hdmi);
void mtdisp_hdmi_core_disable(void *hwlib_hdmi);
void mtdisp_hdmi_core_avi_infoframe(void *hwlib_hdmi, u8 *buf, u8 len);
void mtdisp_hdmi_core_spd_infoframe(void *hwlib_hdmi, u8 *buf, u8 len);
void mtdisp_hdmi_core_audio_infoframe(void *hwlib_hdmi, u8 *buf, u8 len);
void mtdisp_hdmi_core_vendor_infoframe(void *hwlib_hdmi, u8 *buf, u8 len);
void mtdisp_hdmi_core_scramble_config(void *hwlib_hdmi, bool enabled);
bool mtdisp_hdmi_core_is_plugin(void *hwlib_hdmi);
int mtdisp_hdmi_core_i2c_read(void *hwlib_hdmi, u8 addr, u8 *buf, u8 len);
int mtdisp_hdmi_core_i2c_write(void *hwlib_hdmi, u8 addr, u8 *buf, u8 len);
void mtdisp_hdmi_core_audio_enable(void *hwlib_hdmi);
void mtdisp_hdmi_core_audio_set_param(void *hwlib_hdmi, struct mtdisp_codec_params *params);
void mtdisp_hdmi_core_audio_set_stream_enable(void *hwlib_hdmi);
void mtdisp_hdmi_core_audio_set_stream_disable(void *hwlib_hdmi);
void mtdisp_hdmi_core_audio_set_mute(void *hwlib_hdmi);
void mtdisp_hdmi_core_audio_set_unmute(void *hwlib_hdmi);
bool mtdisp_hdmi_mode_supported_with_audio(void *hwlib_hdmi, struct mtdisp_videomode *vm);
int mtdisp_hdmi_pre_probe(void *hwlib_dp);

#endif /* __MTDISP_HDMI_H__ */

/*
 * @Copyright   Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License     Dual MIT/GPLv2
 */

#ifndef __MTDISP_DP_H__
#define __MTDISP_DP_H__

#include "display_types.h"
#include "dp_types.h"

static inline
int mtdisp_dp_link_rate_to_pclk(int link_rate, u8 lane_num, u8 bpp)
{
	return link_rate * lane_num * 8 / bpp;
}

static inline
u8 mtdisp_dp_link_rate_to_bw_code(u32 link_rate)
{
	/* Spec says link_bw = link_rate / 0.27Gbps */
	return link_rate / 27000;
}

int mtdisp_dpcd_write(void *hwlib_dp, u32 offset, u8 *buf, u8 size);
int mtdisp_i2c_write(void *hwlib_dp, u32 offset, u8 *buf, u8 size, bool is_mod);
int mtdisp_dpcd_read(void *hwlib_dp, u32 offset, u8 *buf, u8 size);
int mtdisp_i2c_read(void *hwlib_dp, u16 offset, u8 *buf, u8 size, bool is_mod);
int mtdisp_dp_get_sinkcaps(void *hwlib_dp);
int mtdisp_dp_dsc_get_sinkcaps(void *hwlib_dp);

void mtdisp_dp_automated_test(void *hwlib_dp);
void mtdisp_dp_pre_enable(void *hwlib_dp, void *mt_dsc_config,
			  unsigned long pixelclock, u32 color_depth, u32 range_type);
void mtdisp_dp_enable(void *hwlib_dp);
void mtdisp_dp_disable(void *hwlib_dp);

int mtdisp_dp_pre_probe(void *hwlib_dp);

int mtdisp_dp_hw_resume(void *hwlib_dp);

bool mtdisp_dp_is_mst_support(void *hwlib_dp);
int mtdisp_dp_mst_enable(void *hwlib_dp);
void mtdisp_dp_mst_disable(void *hwlib_dp);

int mtdisp_dp_core_is_plugin(void *hwlib_dp);
u32 mtdisp_dp_core_isr(void *hwlib_dp);
int mtdisp_dp_core_audio_set_param(void *hwlib_dp, struct mtdisp_codec_params *params);
void mtdisp_dp_core_audio_enable(void *hwlib_dp);
void mtdisp_dp_core_audio_disable(void *hwlib_dp);
void mtdisp_dp_core_audio_mute(void *hwlib_dp, bool enable);
int mtdisp_dp_core_aux_read(void *hwlib_dp, u8 request, u16 addr,
			    u8 *buf, u8 bytes, u8 *reply);
int mtdisp_dp_core_aux_write(void *hwlib_dp, u8 request, u16 addr,
			     u8 *buf, u8 bytes, u8 *reply);
void mtdisp_dp_core_set_mst(void *hwlib_dp, bool enable);
void mtdisp_dp_core_stream_config(void *hwlib_dp, u32 stream);
void mtdisp_dp_core_allocate_payload(void *hwlib_dp, u32 stream, int start, int slots);
void mtdisp_dp_core_clear_payload_table(void *hwlib_dp);
void mtdisp_dp_core_trigger_act(void *hwlib_dp);
void mtdisp_dp_core_enable(void *hwlib_dp,  u32 stream);
void mtdisp_dp_core_disable(void *hwlib_dp, u32 stream);
int mtdisp_dp_phy_power_off(void *hwlib_dp);
int mtdisp_dp_phy_exit(void *hwlib_dp);
void mtdisp_dp_core_metadata_config(void *hwlib_dp, u32 stream, struct dp_colorimetry_info* info);
int mtdisp_dp_train_loop(struct mtdisp_hwlib_dp *dp);

#endif /* __MTDISP_DP_H__ */

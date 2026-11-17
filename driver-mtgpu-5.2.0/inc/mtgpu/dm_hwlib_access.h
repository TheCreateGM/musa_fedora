/*
 * @Copyright   Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License     Dual MIT/GPLv2
 */

#ifndef __DM_DATA_CONVERT_H__
#define __DM_DATA_CONVERT_H__

struct mtdisp_videomode;
struct mtdisp_dsc_config;

void *mtdisp_get_hwlib_dispc(void *hwlib_data, u8 id);
struct mtdisp_dispc_capability *mtdisp_get_dispc_caps(void *hwlib_dispc);
u32 mtdisp_get_hwlib_dispc_count(void *hwlib_data);
int mtdisp_get_dispc_ctx_irq(void *hwlib_dispc);
int mtdisp_get_dispc_ctx_id(void *hwlib_dispc);
void mtdisp_set_dispc_ctx_vm(void *hwlib_dispc, struct mtdisp_videomode *vm);
void mtdisp_get_dispc_ctx_vm(struct mtdisp_videomode *vm, void *hwlib_dispc);
void mtdisp_set_dispc_ctx_gamma_size(void *hwlib_dispc, u32 gamma_size);
u32 mtdisp_get_dispc_ctx_gamma_size(void *hwlib_dispc);
void mtdisp_set_dispc_ctx_gamma_store(void *hwlib_dispc, u16 *gamma_store);
void mtdisp_set_dispc_ctx_pcie_mem_base(void *hwlib_dispc, u64 pcie_mem_base);
void mtdisp_set_dispc_ctx_cursor_mem_base(void *hwlib_dispc, u64 cursor_mem_base);
void mtdisp_set_dispc_ctx_cursor_mem_size(void *hwlib_dispc, u32 cursor_mem_size);
void mtdisp_set_dispc_ctx_cursor_buffer_index(void *hwlib_dispc, u8 cursor_buffer_index);
void mtdisp_set_dispc_ctx_primary_layer_format(void *hwlib_dispc, u32 primary_pixformat);
u32 mtdisp_get_dispc_ctx_primary_layer_format(void *hwlib_dispc);
void mtdisp_set_dispc_ctx_color_depth(void *hwlib_dispc, u32 color_depth);
struct mtdisp_dsc_config *mtdisp_get_dispc_dsc_config(void *hwlib_dispc);
bool mtdisp_get_dispc_ctx_dsc_en(void *hwlib_dispc);
u32 mtdisp_get_dispc_ctx_gen(void *hwlib_dispc);

/* encoder */
void *mtdisp_get_hwlib_encoder(void *hwlib_data, u8 id);
u32 mtdisp_get_hwlib_output_count(void *hwlib_data);
void *mtdisp_get_encoder_dp(void *hwlib_encoder);
void *mtdisp_get_encoder_hdmi(void *hwlib_encoder);
enum mtdisp_encoder_type mtdisp_get_encoder_type(void *hwlib_encoder);
u8 mtdisp_get_encoder_default_dc_id(void *hwlib_encoder);

/* dp */
u32 mtdisp_get_dp_ctx_gen(void *hwlib_dp);
void mtdisp_set_dp_ctx_vm(void *hwlib_dp, struct mtdisp_videomode *vm, u32 stream);
void mtdisp_get_dp_ctx_vm(struct mtdisp_videomode *vm, void *hwlib_dp, u32 stream);
u32 mtdisp_get_dp_max_pclk(void *hwlib_dp);
int mtdisp_get_dp_ctx_modes_pclk_khz(void *hwlib_dp, u32 stream);
void mtdisp_set_dp_ctx_modes_pclk_khz(void *hwlib_dp, u32 stream, int pclk_khz);
u8 mtdisp_get_dp_ctx_modes_bpp(void *hwlib_dp, u32 stream);
void mtdisp_set_dp_ctx_modes_bpp(void *hwlib_dp, u32 stream, u8 bpp);
bool mtdisp_get_dp_ctx_modes_active(void *hwlib_dp, u32 stream);
void mtdisp_set_dp_ctx_modes_active(void *hwlib_dp, u32 stream, bool active);
void mtdisp_set_dp_ctx_modes_pbn(void *hwlib_dp, u32 stream, int pbn);
void mtdisp_set_dp_ctx_modes_slots(void *hwlib_dp, u32 stream, int slots);
void mtdisp_set_dp_ctx_modes_pbn_div(void *hwlib_dp, u32 stream, int pbn_div);
u32 mtdisp_get_dp_ctx_modes_range_type(void *hwlib_dp, u32 stream);
void mtdisp_set_dp_ctx_modes_range_type(void *hwlib_dp, u32 stream, u32 range_type);
u8 mtdisp_get_dp_ctx_lane_cnt(void *hwlib_dp);
void mtdisp_set_dp_ctx_lane_count(void *hwlib_dp, u32 lane_cnt);
void mtdisp_set_dp_ctx_link_rate(void *hwlib_dp, u32 link_rate);
u8 mtdisp_get_dp_ctx_bw_code(void *hwlib_dp);
int mtdisp_get_dp_ctx_id(void *hwlib_dp);
int mtdisp_get_dp_ctx_irq(void *hwlib_dp);
int mtdisp_get_dp_ctx_src_max_lanes(void *hwlib_dp);
int mtdisp_get_dp_ctx_src_max_rate(void *hwlib_dp);
u8 mtdisp_get_dp_port_type(void *hwlib_dp);
bool mtdisp_get_dp_connected(void *hwlib_dp);
void mtdisp_set_dp_enabled(void *hwlib_dp, bool enabled);
bool mtdisp_get_dp_enabled(void *hwlib_dp);
bool mtdisp_get_dp_sink_support_dsc(void *hwlib_dp);
bool mtdisp_get_dp_src_support_dsc(void *hwlib_dp);
u32 mtdisp_get_dp_src_dsc_caps_max_hres(void *hwlib_dp);
u32 mtdisp_get_dp_src_dsc_caps_max_vres(void *hwlib_dp);
bool mtdisp_get_dp_ctx_mt_dsc_config_dsc_en(void *hwlib_dp);
u8 mtdisp_get_dp_mst_parent_id(void *hwlib_dp);
bool mtdisp_get_dp_src_support_mst(void *hwlib_dp);
bool mtdisp_get_dp_ctx_mst_en(void *hwlib_dp);
void mtdisp_set_dp_ctx_mst_en(void *hwlib_dp, bool enable);
int mtdisp_get_dp_ctx_max_payloads(void *hwlib_dp);
void mtdisp_set_dp_debugfs_link_rate(void *hwlib_dp, u32 link_rate);
void mtdisp_set_dp_debugfs_lane_cnt(void *hwlib_dp, u8 lane_cnt);
u32 mtdisp_dp_get_max_bpc(void *hwlib_dp);
bool mtdisp_dp_get_support_hdr(void *hwlib_dp);
bool mtdisp_dp_get_support_larger_screen_master(void *hwlib_dp);
u8 mtdisp_dp_get_stream_count(void *hwlib_dp);

/* hdmi */
int mtdisp_get_hdmi_ctx_id(void *hwlib_hdmi);
int mtdisp_get_hdmi_ctx_irq(void *hwlib_hdmi);
void mtdisp_set_hdmi_ctx_vm(void *hwlib_hdmi, struct mtdisp_videomode *vm);
void mtdisp_get_hdmi_ctx_vm(void *hwlib_hdmi, struct mtdisp_videomode *vm);
void mtdisp_set_hdmi_enabled(void *hwlib_hdmi, bool enabled);
bool mtdisp_get_hdmi_enabled(void *hwlib_hdmi);
void mtdisp_hdmi_core_set_dvi(void *hwlib_hdmi, bool is_dvi);
u32 mtdisp_hdmi_get_max_hres(void *hwlib_hdmi);
u32 mtdisp_hdmi_get_max_vres(void *hwlib_hdmi);
u32 mtdisp_hdmi_get_max_pclk_100khz(void *hwlib_hdmi);

#endif /* __DM_DATA_CONVERT_H__ */

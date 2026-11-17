/*
 * @Copyright   Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License     Dual MIT/GPLv2
 */

#ifndef __MTDISP_DISPC_H__
#define __MTDISP_DISPC_H__

#include "dispc_common.h"

void mtdisp_dispc_core_init(void *hwlib_dispc);
void mtdisp_dispc_core_deinit(void *hwlib_dispc);
u32 mtdisp_dispc_core_isr(void *hwlib_dispc);
void mtdisp_dispc_core_config_begin(void *hwlib_dispc);
void mtdisp_dispc_core_config_end(void *hwlib_dispc);
void mtdisp_dispc_core_vsync_on(void *hwlib_dispc);
void mtdisp_dispc_core_vsync_off(void *hwlib_dispc);
int mtdisp_dispc_core_mode_valid(void *hwlib_dispc, struct mtdisp_videomode *vm);
void mtdisp_dispc_core_mode_fixup(void *hwlib_dispc);
void mtdisp_dispc_core_gamma_set(void *hwlib_dispc, struct mtdisp_gamma_config *config);
void mtdisp_dispc_core_fbc_validate(void *hwlib_dispc, u32 format, u64 modifier, u32 index);
void mtdisp_dispc_dsc_start(void *hwlib_dispc);
void mtdisp_dispc_dsc_stop(void *hwlib_dispc);
void mtdisp_dispc_glb_init(void *hwlib_dispc);
void mtdisp_dispc_glb_enable(void *hwlib_dispc);
void mtdisp_dispc_glb_disable(void *hwlib_dispc);
void mtdisp_dispc_glb_fbc_enable(void *hwlib_dispc,
				 struct mtdisp_layer_config *layer, u32 index);
void mtdisp_dispc_glb_fbc_disable(void *hwlib_dispc, u32 index);
bool mtdisp_dispc_glb_is_idle_allowed(void *hwlib_dispc, u32 bandwidth, u32 crtc_num);
void hwlib_update_cursor(void *dispc_data, struct mtdisp_layer_config *layer);
void hwlib_disable_cursor(void *dispc_data);
void hwlib_update_plane(void *dispc_data, struct mtdisp_layer_config *layer, u8 index);
void hwlib_update_planes(void *dispc_data,struct mtdisp_layer_config *layer, u32 max_plane_num);
void hwlib_disable_plane(void *dispc_data, u8 index);
void mtdisp_dispc_core_blank(void *hwlib_dispc, bool blank);
u32  mtdisp_dispc_core_get_scanline(void *hwlib_dispc, bool* isblanking);
void mtdisp_dispc_core_set_hwsync(void *hwlib_dispc, bool enable);

#endif /* __MTDISP_DISPC_H__ */

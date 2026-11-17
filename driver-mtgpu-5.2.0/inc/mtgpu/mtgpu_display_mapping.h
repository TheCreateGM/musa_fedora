/*
 * @Copyright   Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License     Dual MIT/GPLv2
 */

#ifndef __MTGPU_DISPLAY_MAPPING_H__
#define __MTGPU_DISPLAY_MAPPING_H__

enum mtgpu_layer_type;
enum mtgpu_plane_type_id;
enum mtgpu_pixel_fmt_id;

struct mtdisp_layer_config;
struct mtgpu_layer_config;

int mtgpu_display_plane_type_drv_to_drm(enum mtgpu_plane_type_id id);
int mtgpu_display_plane_type_drm_to_drv(enum mtgpu_layer_type type);

u64 mtgpu_display_format_drv_to_drm(enum mtgpu_pixel_fmt_id id);
enum mtgpu_pixel_fmt_id mtgpu_display_format_drm_to_drv(u64 fmt);

int mtgpu_layer_to_mtdisp_layer(struct mtdisp_layer_config *mtdisp_layer,
				struct mtgpu_layer_config *mtgpu_layer);

#endif /* __MTGPU_DISPLAY_MAPPING_H__ */

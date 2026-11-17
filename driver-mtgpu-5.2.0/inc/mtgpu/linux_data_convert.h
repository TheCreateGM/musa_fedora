/*
 * @Copyright   Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License     Dual MIT/GPLv2
 */

#ifndef __LINUX_DATA_CONVERT_H__
#define __LINUX_DATA_CONVERT_H__

struct videomode;
struct mtdisp_videomode;
struct mtdisp_layer_capability;

void mtdisp_videomode_from_os_videomode(struct mtdisp_videomode *mt_vm,
					struct videomode *vm);
void mtdisp_videomode_to_os_videomode(struct videomode *vm,
				      struct mtdisp_videomode *mt_vm);
u64 mtdisp_format_fourcc_from_id(int id);
int mtdisp_format_fourcc_to_id(u64 fourcc);

int mtdisp_layer_caps_from_hwlib(struct mtdisp_layer_capability *layer_caps,
				 struct mtdisp_layer_capability *hwlib_layer_caps,
				 int layer_count, u32 support_mpo_num);

u8 mtdisp_alpha_blend_mode_to_hw(u8 blend_mode);

#endif /* __LINUX_DATA_CONVERT_H__ */

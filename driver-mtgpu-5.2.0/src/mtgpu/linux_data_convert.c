/*
 * @Copyright   Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License     Dual MIT/GPLv2
 */

#include <linux/string.h>
#include <linux/printk.h>
#include <video/videomode.h>
#include <drm/drm_fourcc.h>

#include "dispc_common.h"
#include "dispc_type.h"
#include "linux_data_convert.h"

#define DRM_FORMAT_MOD_VENDOR_MT		0x92
#define DRM_FORMAT_MOD_MT_FBCDC_16x4_V10	fourcc_mod_code(MT, 22)

void mtdisp_videomode_from_os_videomode(struct mtdisp_videomode *mt_vm,
					struct videomode *vm)
{
	mt_vm->pixelclock	= vm->pixelclock;

	mt_vm->hactive		= vm->hactive;
	mt_vm->hfront_porch	= vm->hfront_porch;
	mt_vm->hback_porch	= vm->hback_porch;
	mt_vm->hsync_len	= vm->hsync_len;

	mt_vm->vactive		= vm->vactive;
	mt_vm->vfront_porch	= vm->vfront_porch;
	mt_vm->vback_porch	= vm->vback_porch;
	mt_vm->vsync_len	= vm->vsync_len;

	mt_vm->flags		= (enum mtdisp_flags)vm->flags;
}

void mtdisp_videomode_to_os_videomode(struct videomode *vm,
				      struct mtdisp_videomode *mt_vm)
{
	vm->pixelclock		= mt_vm->pixelclock;

	vm->hactive		= mt_vm->hactive;
	vm->hfront_porch	= mt_vm->hfront_porch;
	vm->hback_porch		= mt_vm->hback_porch;
	vm->hsync_len		= mt_vm->hsync_len;

	vm->vactive		= mt_vm->vactive;
	vm->vfront_porch	= mt_vm->vfront_porch;
	vm->vback_porch		= mt_vm->vback_porch;
	vm->vsync_len		= mt_vm->vsync_len;

	vm->flags		= (enum display_flags)mt_vm->flags;
}

u64 mtdisp_format_fourcc_from_id(int id)
{
	switch (id) {
	case MT_PIXEL_FORMAT_RGB565:			return DRM_FORMAT_RGB565;
	case MT_PIXEL_FORMAT_BGR565:			return DRM_FORMAT_BGR565;

	case MT_PIXEL_FORMAT_ARGB8888:			return DRM_FORMAT_ARGB8888;
	case MT_PIXEL_FORMAT_ABGR8888:			return DRM_FORMAT_ABGR8888;

	case MT_PIXEL_FORMAT_RGBA8888:			return DRM_FORMAT_RGBA8888;
	case MT_PIXEL_FORMAT_BGRA8888:			return DRM_FORMAT_BGRA8888;

	case MT_PIXEL_FORMAT_XRGB8888:			return DRM_FORMAT_XRGB8888;
	case MT_PIXEL_FORMAT_XBGR8888:			return DRM_FORMAT_XBGR8888;
	case MT_PIXEL_FORMAT_RGBX8888:			return DRM_FORMAT_RGBX8888;
	case MT_PIXEL_FORMAT_BGRX8888:			return DRM_FORMAT_BGRX8888;

	case MT_PIXEL_FORMAT_XRGB2101010:		return DRM_FORMAT_XRGB2101010;
	case MT_PIXEL_FORMAT_XBGR2101010:		return DRM_FORMAT_XBGR2101010;
	case MT_PIXEL_FORMAT_RGBX1010102:		return DRM_FORMAT_RGBX1010102;
	case MT_PIXEL_FORMAT_BGRX1010102:		return DRM_FORMAT_BGRX1010102;

	case MT_PIXEL_FORMAT_ARGB2101010:		return DRM_FORMAT_ARGB2101010;
	case MT_PIXEL_FORMAT_ABGR2101010:		return DRM_FORMAT_ABGR2101010;

	case MT_PIXEL_FORMAT_RGBA1010102:		return DRM_FORMAT_RGBA1010102;
	case MT_PIXEL_FORMAT_BGRA1010102:		return DRM_FORMAT_BGRA1010102;

	case MT_PIXEL_FORMAT_Y410:			return DRM_FORMAT_Y410;
	case MT_PIXEL_FORMAT_Y412:			return DRM_FORMAT_Y412;
	case MT_PIXEL_FORMAT_Y416:			return DRM_FORMAT_Y416;

	case MT_PIXEL_FORMAT_YUYV8888:			return DRM_FORMAT_YUYV;
	case MT_PIXEL_FORMAT_YVYU8888:			return DRM_FORMAT_YVYU;
	case MT_PIXEL_FORMAT_UYVY8888:			return DRM_FORMAT_UYVY;
	case MT_PIXEL_FORMAT_VYUY8888:			return DRM_FORMAT_VYUY;

	case MT_PIXEL_FORMAT_XVYU2101010:		return DRM_FORMAT_XVYU2101010;
	case MT_PIXEL_FORMAT_XVYU12_16161616:		return DRM_FORMAT_XVYU12_16161616;
	case MT_PIXEL_FORMAT_XVYU16161616:		return DRM_FORMAT_XVYU16161616;

	case MT_PIXEL_FORMAT_AYUV8888:			return DRM_FORMAT_AYUV;
	case MT_PIXEL_FORMAT_XYUV8888:			return DRM_FORMAT_XYUV8888;

	case MT_PIXEL_FORMAT_YUV420_2PLANE_PACK8:	return DRM_FORMAT_NV12;
	case MT_PIXEL_FORMAT_NV16:			return DRM_FORMAT_NV16;

	case MT_PIXEL_FORMAT_MOD_MT_FBCDC_16x4_V10:	return DRM_FORMAT_MOD_MT_FBCDC_16x4_V10;
	case MT_PIXEL_FORMAT_MOD_INVALID:		return DRM_FORMAT_MOD_INVALID;

	default:
		pr_err("%s() No such format: id = %d\n", __func__, id);
		return 0;
	}
}

int mtdisp_format_fourcc_to_id(u64 fourcc)
{
	switch (fourcc) {
	case DRM_FORMAT_RGB565:			return MT_PIXEL_FORMAT_RGB565;
	case DRM_FORMAT_BGR565:			return MT_PIXEL_FORMAT_BGR565;

	case DRM_FORMAT_ARGB8888:		return MT_PIXEL_FORMAT_ARGB8888;
	case DRM_FORMAT_ABGR8888:		return MT_PIXEL_FORMAT_ABGR8888;

	case DRM_FORMAT_RGBA8888:		return MT_PIXEL_FORMAT_RGBA8888;
	case DRM_FORMAT_BGRA8888:		return MT_PIXEL_FORMAT_BGRA8888;

	case DRM_FORMAT_XRGB8888:		return MT_PIXEL_FORMAT_XRGB8888;
	case DRM_FORMAT_XBGR8888:		return MT_PIXEL_FORMAT_XBGR8888;
	case DRM_FORMAT_RGBX8888:		return MT_PIXEL_FORMAT_RGBX8888;
	case DRM_FORMAT_BGRX8888:		return MT_PIXEL_FORMAT_BGRX8888;

	case DRM_FORMAT_XRGB2101010:		return MT_PIXEL_FORMAT_XRGB2101010;
	case DRM_FORMAT_XBGR2101010:		return MT_PIXEL_FORMAT_XBGR2101010;
	case DRM_FORMAT_RGBX1010102:		return MT_PIXEL_FORMAT_RGBX1010102;
	case DRM_FORMAT_BGRX1010102:		return MT_PIXEL_FORMAT_BGRX1010102;

	case DRM_FORMAT_ARGB2101010:		return MT_PIXEL_FORMAT_ARGB2101010;
	case DRM_FORMAT_ABGR2101010:		return MT_PIXEL_FORMAT_ABGR2101010;

	case DRM_FORMAT_RGBA1010102:		return MT_PIXEL_FORMAT_RGBA1010102;
	case DRM_FORMAT_BGRA1010102:		return MT_PIXEL_FORMAT_BGRA1010102;

	case DRM_FORMAT_Y410:			return MT_PIXEL_FORMAT_Y410;
	case DRM_FORMAT_Y412:			return MT_PIXEL_FORMAT_Y412;
	case DRM_FORMAT_Y416:			return MT_PIXEL_FORMAT_Y416;

	case DRM_FORMAT_YUYV:			return MT_PIXEL_FORMAT_YUYV8888;
	case DRM_FORMAT_YVYU:			return MT_PIXEL_FORMAT_YVYU8888;
	case DRM_FORMAT_UYVY:			return MT_PIXEL_FORMAT_UYVY8888;
	case DRM_FORMAT_VYUY:			return MT_PIXEL_FORMAT_VYUY8888;

	case DRM_FORMAT_XVYU2101010:		return MT_PIXEL_FORMAT_XVYU2101010;
	case DRM_FORMAT_XVYU12_16161616:	return MT_PIXEL_FORMAT_XVYU12_16161616;
	case DRM_FORMAT_XVYU16161616:		return MT_PIXEL_FORMAT_XVYU16161616;

	case DRM_FORMAT_AYUV:			return MT_PIXEL_FORMAT_AYUV8888;
	case DRM_FORMAT_XYUV8888:		return MT_PIXEL_FORMAT_XYUV8888;

	case DRM_FORMAT_NV12:			return MT_PIXEL_FORMAT_YUV420_2PLANE_PACK8;
	case DRM_FORMAT_NV16:			return MT_PIXEL_FORMAT_NV16;

	case DRM_FORMAT_MOD_MT_FBCDC_16x4_V10:	return MT_PIXEL_FORMAT_MOD_MT_FBCDC_16x4_V10;
	case DRM_FORMAT_MOD_INVALID:		return MT_PIXEL_FORMAT_MOD_INVALID;

	default:
		pr_err("%s() No such format: fourcc = %lld\n", __func__, fourcc);
		return -1;
	}
}

static bool mtdisp_format_fourcc_contains_alpha(u32 format)
{
	return (format & 0xff) == 'A' ||
	       ((format >> 8) & 0xff) == 'A' ||
	       ((format >> 16) & 0xff) == 'A' ||
	       ((format >> 24) & 0xff) == 'A';
}

int mtdisp_layer_caps_from_hwlib(struct mtdisp_layer_capability *layer_caps,
				 struct mtdisp_layer_capability *hwlib_layer_caps,
				 int layer_count, u32 support_mpo_num)
{
	int i, ly;
	u32 *fmts_ptr;
	u64 *modifier;

	for (ly = 0; ly < layer_count; ly++) {
		struct mtdisp_layer_capability *src_caps = &hwlib_layer_caps[ly];
		struct mtdisp_layer_capability *dst_caps = &layer_caps[ly];

		fmts_ptr = (u32 *)dst_caps->fmts_ptr;
		modifier = (u64 *)dst_caps->modifier;

		dst_caps->type			= src_caps->type;
		dst_caps->fmts_cnt		= 0;
		dst_caps->modifier_cnt		= src_caps->modifier_cnt;
		dst_caps->supported_encodings	= src_caps->supported_encodings;
		dst_caps->supported_ranges	= src_caps->supported_ranges;

		for (i = 0; i < src_caps->fmts_cnt; i++) {
			u32 format = (u32)mtdisp_format_fourcc_from_id(src_caps->fmts_ptr[i]);

			if (src_caps->type == MTDISP_LAYER_TYPE_PRIMARY &&
			    support_mpo_num <= 1 &&
			    mtdisp_format_fourcc_contains_alpha(format))
				continue;

			fmts_ptr[dst_caps->fmts_cnt++] = format;
		}

		for (i = 0; ; i++) {
			modifier[i] = mtdisp_format_fourcc_from_id(src_caps->modifier[i]);

			if (src_caps->modifier[i] == MT_PIXEL_FORMAT_MOD_INVALID)
				break;
		}
	}

	return 0;
}

u8 mtdisp_alpha_blend_mode_to_hw(u8 blend_mode)
{
	return (blend_mode + 1) % 3;
}

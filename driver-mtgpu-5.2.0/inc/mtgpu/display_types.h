/*
 * @Copyright   Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License     Dual MIT/GPLv2
 */

#ifndef _DISPLAY_TYPES_H_
#define _DISPLAY_TYPES_H_

#ifndef true
#define true			1
#endif

#ifndef false
#define false			0
#endif

#ifndef BIT
#define BIT(b)			(1UL << (b))
#endif

#ifndef BIT_ULL
#define BIT_ULL(b)		(1ULL << (b))
#endif

#ifndef NULL
#define NULL			((void *)0)
#endif

#ifndef GENMASK_32
#define GENMASK_32(h, l) 	(((~(0u)) - ((1u) << (l)) + 1u) & \
				 (~(0u) >> (32 - 1 - (h))))
#endif

#ifndef GENMASK_64
#define GENMASK_64(h, l)	(((~(0ull)) - ((1ull) << (l)) + 1ull) & \
				 (~(0ull) >> (64 - 1 - (h))))
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a)		(sizeof((a)) / sizeof((a[0])))
#endif

#ifndef DIV_ROUND_UP
#define DIV_ROUND_UP(n, d)	(((n) + (d) - 1) / (d))
#endif

#ifndef MIN
#define MIN(a, b)		(((a) < (b)) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a, b)		(((a) > (b)) ? (a) : (b))
#endif

/* errno */
#ifndef EPERM
#define EPERM		 1	/* Operation not permitted */
#endif

#ifndef EIO
#define EIO		 5	/* I/O error */
#endif

#ifndef ENXIO
#define ENXIO		 6	/* No such device or address */
#endif

#ifndef EAGAIN
#define EAGAIN		11	/* Try again */
#endif

#ifndef ENOMEM
#define ENOMEM		12	/* Out of memory */
#endif

#ifndef EBUSY
#define EBUSY		16	/* Device or resource busy */
#endif

#ifndef ENODEV
#define ENODEV		19	/* No such device */
#endif

#ifndef EINVAL
#define EINVAL		22	/* Invalid argument */
#endif

#ifndef ENOSPC
#define ENOSPC		28	/* No space left on device */
#endif

#ifndef ETIMEDOUT
#define ETIMEDOUT	60	/* Connection timed out */
#endif

#ifndef DRM_FORMAT_XYUV8888
#define DRM_FORMAT_XYUV8888		fourcc_code('X', 'Y', 'U', 'V') /* [31:0] X:Y:Cb:Cr 8:8:8:8 little endian */
#endif
#ifndef DRM_FORMAT_VUY888
#define DRM_FORMAT_VUY888		fourcc_code('V', 'U', '2', '4') /* [23:0] Cr:Cb:Y 8:8:8 little endian */
#endif
#ifndef DRM_FORMAT_VUY101010
#define DRM_FORMAT_VUY101010		fourcc_code('V', 'U', '3', '0') /* Y followed by U then V, 10:10:10. Non-linear modifier only */
#endif
#ifndef DRM_FORMAT_Y410
#define DRM_FORMAT_Y410			fourcc_code('Y', '4', '1', '0') /* [31:0] A:Cr:Y:Cb 2:10:10:10 little endian */
#endif
#ifndef DRM_FORMAT_Y412
#define DRM_FORMAT_Y412			fourcc_code('Y', '4', '1', '2') /* [63:0] A:0:Cr:0:Y:0:Cb:0 12:4:12:4:12:4:12:4 little endian */
#endif
#ifndef DRM_FORMAT_Y416
#define DRM_FORMAT_Y416			fourcc_code('Y', '4', '1', '6') /* [63:0] A:Cr:Y:Cb 16:16:16:16 little endian */
#endif
#ifndef DRM_FORMAT_XVYU2101010
#define DRM_FORMAT_XVYU2101010		fourcc_code('X', 'V', '3', '0') /* [31:0] X:Cr:Y:Cb 2:10:10:10 little endian */
#endif
#ifndef DRM_FORMAT_XVYU12_16161616
#define DRM_FORMAT_XVYU12_16161616	fourcc_code('X', 'V', '3', '6') /* [63:0] X:0:Cr:0:Y:0:Cb:0 12:4:12:4:12:4:12:4 little endian */
#endif
#ifndef DRM_FORMAT_XVYU16161616
#define DRM_FORMAT_XVYU16161616		fourcc_code('X', 'V', '4', '8') /* [63:0] X:Cr:Y:Cb 16:16:16:16 little endian */
#endif

typedef unsigned char		u8;
typedef unsigned short		u16;
typedef unsigned int		u32;
typedef unsigned long int	ulong;
typedef unsigned long long	u64;
// typedef char			s8;
typedef short			s16;
typedef int			s32;
typedef long long		s64;

#if !defined(__cplusplus) && !defined(_MS_VC_TYPES_H_)
typedef _Bool bool;
#endif

typedef u16			__le16;
typedef u16			__be16;

#define MTDISP_SOC_GEN1		1
#define MTDISP_SOC_GEN2		2
#define MTDISP_SOC_GEN3		3
#define MTDISP_SOC_GEN4		4
#define MTDISP_SOC_GEN6		6

#define DISPC_MAX_COUNT		4
#define ENCODER_MAX_COUNT	8
#define OUTPUT_MAX_COUNT	ENCODER_MAX_COUNT

enum mtdisp_flags {
	MTDISP_DISPLAY_FLAGS_HSYNC_LOW		= BIT(0),
	MTDISP_DISPLAY_FLAGS_HSYNC_HIGH		= BIT(1),
	MTDISP_DISPLAY_FLAGS_VSYNC_LOW		= BIT(2),
	MTDISP_DISPLAY_FLAGS_VSYNC_HIGH		= BIT(3),

	/* data enable flag */
	MTDISP_DISPLAY_FLAGS_DE_LOW		= BIT(4),
	MTDISP_DISPLAY_FLAGS_DE_HIGH		= BIT(5),
	/* drive data on pos. edge */
	MTDISP_DISPLAY_FLAGS_PIXDATA_POSEDGE	= BIT(6),
	/* drive data on neg. edge */
	MTDISP_DISPLAY_FLAGS_PIXDATA_NEGEDGE	= BIT(7),
	MTDISP_DISPLAY_FLAGS_INTERLACED		= BIT(8),
	MTDISP_DISPLAY_FLAGS_DOUBLESCAN		= BIT(9),
	MTDISP_DISPLAY_FLAGS_DOUBLECLK		= BIT(10),
	/* drive sync on pos. edge */
	MTDISP_DISPLAY_FLAGS_SYNC_POSEDGE	= BIT(11),
	/* drive sync on neg. edge */
	MTDISP_DISPLAY_FLAGS_SYNC_NEGEDGE	= BIT(12),
};

struct mtdisp_videomode {
	unsigned long pixelclock;	/* pixelclock in Hz */

	u32 hactive;
	u32 hfront_porch;
	u32 hback_porch;
	u32 hsync_len;

	u32 vactive;
	u32 vfront_porch;
	u32 vback_porch;
	u32 vsync_len;

	u32 vic;
	u32 hdmi_vic;
	u8  bw_code;
	u8  lane_count;

	enum mtdisp_flags flags;	/* display flags */
};

/*
 * audio parameters
 */
struct mtdisp_codec_params {
	int sample_rate;
	int sample_width;
	int channels;
};

enum mtdisp_output_range_type {
	OUTPUT_RANGE_SDR,
	OUTPUT_RANGE_HDR10,
	OUTPUT_RANGE_HDR10_PLUS,
	OUTPUT_RANGE_HLG,
};

struct mtdisp_output_caps {
	u32 color_depth;
	u32 range_type;
};

#endif

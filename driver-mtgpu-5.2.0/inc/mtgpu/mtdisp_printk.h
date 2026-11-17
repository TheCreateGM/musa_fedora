/*
 * @Copyright   Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License     Dual MIT/GPLv2
 */

#ifndef __MTDISP_PRINTK_H__
#define __MTDISP_PRINTK_H__

#include "display_func.h"

extern ulong display_debug;

/* debug flags */
#define DISP_PRINT_REG		BIT(5)
#define DISP_PRINT_TRACE	BIT(4)
#define DISP_PRINT_DEBUG	BIT(3)
#define DISP_PRINT_INFO		BIT(2)
#define DISP_PRINT_WARN		BIT(1)
#define DISP_PRINT_ERROR	BIT(0)

#define DSC_DEBUG_MASK		(0xFF00000000UL)
#define DSC_DEBUG_CTL		((display_debug & DSC_DEBUG_MASK)   >> 32)
#define GLB_DEBUG_MASK		(0x00FF000000UL)
#define GLB_DEBUG_CTL		((display_debug & GLB_DEBUG_MASK)   >> 24)
#define DISPC_DEBUG_MASK	(0x0000FF0000UL)
#define DISPC_DEBUG_CTL		((display_debug & DISPC_DEBUG_MASK) >> 16)
#define HDMI_DEBUG_MASK		(0x000000FF00UL)
#define HDMI_DEBUG_CTL		((display_debug & HDMI_DEBUG_MASK)  >> 8)
#define DP_DEBUG_MASK		(0x00000000FFUL)
#define DP_DEBUG_CTL		((display_debug & DP_DEBUG_MASK)    >> 0)

#define mtdisp_pr_err(fmt, ...)								\
	do {										\
		if (mtdisp_func && mtdisp_func->mt_pr_err)				\
			mtdisp_func->mt_pr_err(fmt, ##__VA_ARGS__);			\
	} while (0)

#define mtdisp_pr_warn(fmt, ...)							\
	do {										\
		if (mtdisp_func && mtdisp_func->mt_pr_warn)				\
			mtdisp_func->mt_pr_warn(fmt, ##__VA_ARGS__);			\
	} while (0)

#define mtdisp_pr_info(fmt, ...)							\
	do {										\
		if (mtdisp_func && mtdisp_func->mt_pr_info)				\
			mtdisp_func->mt_pr_info(fmt, ##__VA_ARGS__);			\
	} while (0)

#define mtdisp_pr_err_ratelimited(fmt, ...)						\
	do {										\
		if (mtdisp_func && mtdisp_func->mt_pr_err_ratelimited)			\
			mtdisp_func->mt_pr_err_ratelimited(fmt, ##__VA_ARGS__);		\
	} while (0)

#define mtdisp_pr_warn_ratelimited(fmt, ...)						\
	do {										\
		if (mtdisp_func && mtdisp_func->mt_pr_warn_ratelimited)			\
			mtdisp_func->mt_pr_warn_ratelimited(fmt, ##__VA_ARGS__);	\
	} while (0)

#endif /* __MTDISP_PRINTK_H__ */

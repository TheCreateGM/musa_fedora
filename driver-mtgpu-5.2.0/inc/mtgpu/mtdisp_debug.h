/*
 * @Copyright   Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License     Dual MIT/GPLv2
 */

#ifndef __MTDISP_DEBUG_H__
#define __MTDISP_DEBUG_H__

#include "mtdisp_printk.h"

/* dsc */
#define DSC_DEBUG(fmt, ...)									\
	do {											\
		if (DSC_DEBUG_CTL & DISP_PRINT_DEBUG)						\
			mtdisp_pr_info("DSC: %s() " fmt, __func__, ##__VA_ARGS__);		\
	} while (0)

#define DSC_INFO(fmt, ...)									\
	do {											\
		if (DSC_DEBUG_CTL & DISP_PRINT_INFO)						\
			mtdisp_pr_info("DSC: %s() " fmt, __func__, ##__VA_ARGS__);		\
	} while (0)

#define DSC_WARN(fmt, ...)									\
	do {											\
		if (DSC_DEBUG_CTL & DISP_PRINT_WARN)						\
			mtdisp_pr_warn("DSC: %s() " fmt, __func__, ##__VA_ARGS__);		\
	} while (0)

#define DSC_ERROR(fmt, ...)									\
	do {											\
		if (DSC_DEBUG_CTL & DISP_PRINT_ERROR)						\
			mtdisp_pr_err("DSC: %s() " fmt, __func__, ##__VA_ARGS__);		\
	} while (0)

#define DSC_INFO_ONCE(fmt, ...)									\
	do {											\
		if (DSC_DEBUG_CTL & DISP_PRINT_INFO) {						\
			static bool __print_once;						\
			if (!__print_once) {							\
				__print_once = true;						\
				mtdisp_pr_info("DSC: %s() " fmt, __func__, ##__VA_ARGS__);	\
			}									\
		}										\
	} while (0)

#define DSC_WARN_ONCE(fmt, ...)									\
	do {											\
		if (DSC_DEBUG_CTL & DISP_PRINT_WARN) {						\
			static bool __print_once;						\
			if (!__print_once) {							\
				__print_once = true;						\
				mtdisp_pr_warn("DSC: %s() " fmt, __func__, ##__VA_ARGS__);	\
			}									\
		}										\
	} while (0)

#define DSC_ERROR_ONCE(fmt, ...)								\
	do {											\
		if (DSC_DEBUG_CTL & DISP_PRINT_ERROR) {						\
			static bool __print_once;						\
			if (!__print_once) {							\
				__print_once = true;						\
				mtdisp_pr_err("DSC: %s() " fmt, __func__, ##__VA_ARGS__);	\
			}									\
		}										\
	} while (0)

#define DSC_ERROR_RATELIMITATED(fmt, ...)							\
	do {											\
		if (DSC_DEBUG_CTL & DISP_PRINT_ERROR)						\
			mtdisp_pr_err_ratelimited("DSC: %s() " fmt, __func__, ##__VA_ARGS__);	\
	} while (0)

/* glb */
#define GLB_DEBUG(fmt, ...)									\
	do {											\
		if (GLB_DEBUG_CTL & DISP_PRINT_DEBUG)						\
			mtdisp_pr_info("GLB: %s() " fmt, __func__, ##__VA_ARGS__);		\
	} while (0)

#define GLB_INFO(fmt, ...)									\
	do {											\
		if (GLB_DEBUG_CTL & DISP_PRINT_INFO)						\
			mtdisp_pr_info("GLB: %s() " fmt, __func__, ##__VA_ARGS__);		\
	} while (0)

#define GLB_WARN(fmt, ...)									\
	do {											\
		if (GLB_DEBUG_CTL & DISP_PRINT_WARN)						\
			mtdisp_pr_warn("GLB: %s() " fmt, __func__, ##__VA_ARGS__);		\
	} while (0)

#define GLB_ERROR(fmt, ...)									\
	do {											\
		if (GLB_DEBUG_CTL & DISP_PRINT_ERROR)						\
			mtdisp_pr_err("GLB: %s() " fmt, __func__, ##__VA_ARGS__);		\
	} while (0)

#define DISPC_GLB_DBG_REG(fmt, ...)								\
	do {											\
		if (GLB_DEBUG_CTL & DISP_PRINT_REG)						\
			mtdisp_pr_info("DISPC-%d: %s() " fmt, ctx->id, __func__, ##__VA_ARGS__);\
	} while (0)

#define DP_GLB_DBG_REG(fmt, ...)								\
	do {											\
		if (GLB_DEBUG_CTL & DISP_PRINT_REG)						\
			mtdisp_pr_info("DP-%d: %s() " fmt, ctx->id, __func__, ##__VA_ARGS__);	\
	} while (0)

#define GLB_INFO_ONCE(fmt, ...)									\
	do {											\
		if (GLB_DEBUG_CTL & DISP_PRINT_INFO) {						\
			static bool __print_once;						\
			if (!__print_once) {							\
				__print_once = true;						\
				mtdisp_pr_info("GLB: %s() " fmt, __func__, ##__VA_ARGS__);	\
			}									\
		}										\
	} while (0)

#define GLB_WARN_ONCE(fmt, ...)									\
	do {											\
		if (GLB_DEBUG_CTL & DISP_PRINT_WARN) {						\
			static bool __print_once;						\
			if (!__print_once) {							\
				__print_once = true;						\
				mtdisp_pr_warn("GLB: %s() " fmt, __func__, ##__VA_ARGS__);	\
			}									\
		}										\
	} while (0)

#define GLB_ERROR_ONCE(fmt, ...)								\
	do {											\
		if (GLB_DEBUG_CTL & DISP_PRINT_ERROR) {						\
			static bool __print_once;						\
			if (!__print_once) {							\
				__print_once = true;						\
				mtdisp_pr_err("GLB: %s() " fmt, __func__, ##__VA_ARGS__);	\
			}									\
		}										\
	} while (0)

#define GLB_ERROR_RATELIMITATED(fmt, ...)							\
	do {											\
		if (GLB_DEBUG_CTL & DISP_PRINT_ERROR)						\
			mtdisp_pr_err_ratelimited("GLB: %s() " fmt, __func__, ##__VA_ARGS__);	\
	} while (0)

/* dispc */
#define DISPC_TRACE()										\
	do {											\
		if (DISPC_DEBUG_CTL & DISP_PRINT_TRACE)						\
			mtdisp_pr_info("DISPC-%d: %s()\n", ctx->id, __func__);			\
	} while (0)

#define DISPC_DEBUG(fmt, ...)									\
	do {											\
		if (DISPC_DEBUG_CTL & DISP_PRINT_DEBUG)						\
			mtdisp_pr_info("DISPC-%d: %s() " fmt, ctx->id, __func__, ##__VA_ARGS__);\
	} while (0)

#define DISPC_INFO(fmt, ...)									\
	do {											\
		if (DISPC_DEBUG_CTL & DISP_PRINT_INFO)						\
			mtdisp_pr_info("DISPC-%d: %s() " fmt, ctx->id, __func__, ##__VA_ARGS__);\
	} while (0)

#define DISPC_WARN(fmt, ...)									\
	do {											\
		if (DISPC_DEBUG_CTL & DISP_PRINT_WARN)						\
			mtdisp_pr_warn("DISPC-%d: %s() " fmt, ctx->id, __func__, ##__VA_ARGS__);\
	} while (0)

#define DISPC_ERROR(fmt, ...)									\
	do {											\
		if (DISPC_DEBUG_CTL & DISP_PRINT_ERROR)						\
			mtdisp_pr_err("DISPC-%d: %s() " fmt, ctx->id, __func__, ##__VA_ARGS__);	\
	} while (0)

#define DISPC_DBG_REG(fmt, ...)									\
	do {											\
		if (DISPC_DEBUG_CTL & DISP_PRINT_REG)						\
			mtdisp_pr_info("DISPC-%d: %s() " fmt, ctx->id, __func__, ##__VA_ARGS__);\
	} while (0)

#define DISPC_INFO_ONCE(fmt, ...)								\
	do {											\
		if (DISPC_DEBUG_CTL & DISP_PRINT_INFO) {					\
			static bool __print_once;						\
			if (!__print_once) {							\
				__print_once = true;						\
				mtdisp_pr_info("DISPC-%d: %s() "				\
					       fmt, ctx->id, __func__, ##__VA_ARGS__);		\
			}									\
		}										\
	} while (0)

#define DISPC_WARN_ONCE(fmt, ...)								\
	do {											\
		if (DISPC_DEBUG_CTL & DISP_PRINT_WARN) {					\
			static bool __print_once;						\
			if (!__print_once) {							\
				__print_once = true;						\
				mtdisp_pr_warn("DISPC-%d: %s() "				\
					       fmt, ctx->id, __func__, ##__VA_ARGS__);		\
			}									\
		}										\
	} while (0)

#define DISPC_ERROR_ONCE(fmt, ...)								\
	do {											\
		if (DISPC_DEBUG_CTL & DISP_PRINT_ERROR) {					\
			static bool __print_once;						\
			if (!__print_once) {							\
				__print_once = true;						\
				mtdisp_pr_err("DISPC-%d: %s() "					\
					      fmt, ctx->id, __func__, ##__VA_ARGS__);		\
			}									\
		}										\
	} while (0)

#define DISPC_ERROR_LIMITED(fmt, ...)								\
	do {											\
		if (DISPC_DEBUG_CTL & DISP_PRINT_ERROR)						\
			mtdisp_pr_err_ratelimited("DISPC-%d: %s() "				\
				fmt, ctx->id, __func__, ##__VA_ARGS__);				\
	} while (0)

/* hdmi */
#define HDMI_TRACE()										\
	do {											\
		if (HDMI_DEBUG_CTL & DISP_PRINT_TRACE)						\
			mtdisp_pr_info("HDMI-%d: %s()\n", ctx->id, __func__);			\
	} while (0)

#define HDMI_DEBUG(fmt, ...)									\
	do {											\
		if (HDMI_DEBUG_CTL & DISP_PRINT_DEBUG)						\
			mtdisp_pr_info("HDMI-%d: %s() "						\
				       fmt, ctx->id, __func__, ##__VA_ARGS__);			\
	} while (0)

#define HDMI_INFO(fmt, ...)									\
	do {											\
		if (HDMI_DEBUG_CTL & DISP_PRINT_INFO)						\
			mtdisp_pr_info("HDMI-%d: %s() " fmt, ctx->id, __func__, ##__VA_ARGS__);	\
	} while (0)

#define HDMI_WARN(fmt, ...)									\
	do {											\
		if (HDMI_DEBUG_CTL & DISP_PRINT_WARN)						\
			mtdisp_pr_warn("HDMI-%d: %s() " fmt, ctx->id, __func__, ##__VA_ARGS__);	\
	} while (0)

#define HDMI_ERROR(fmt, ...)									\
	do {											\
		if (HDMI_DEBUG_CTL & DISP_PRINT_ERROR)						\
			mtdisp_pr_err("HDMI-%d: %s() " fmt, ctx->id, __func__, ##__VA_ARGS__);	\
	} while (0)

#define HDMI_INFO_ONCE(fmt, ...)								\
	do {											\
		if (HDMI_DEBUG_CTL & DISP_PRINT_INFO) {						\
			static bool __print_once;						\
			if (!__print_once) {							\
				__print_once = true;						\
				mtdisp_pr_info("HDMI-%d: %s() "					\
					       fmt, ctx->id, __func__, ##__VA_ARGS__);		\
			}									\
		}										\
	} while (0)

#define HDMI_WARN_ONCE(fmt, ...)								\
	do {											\
		if (HDMI_DEBUG_CTL & DISP_PRINT_WARN) {						\
			static bool __print_once;						\
			if (!__print_once) {							\
				__print_once = true;						\
				mtdisp_pr_warn("HDMI-%d: %s() "					\
					       fmt, ctx->id, __func__, ##__VA_ARGS__);		\
			}									\
		}										\
	} while (0)

#define HDMI_ERROR_ONCE(fmt, ...)								\
	do {											\
		if (HDMI_DEBUG_CTL & DISP_PRINT_ERROR) {					\
			static bool __print_once;						\
			if (!__print_once) {							\
				__print_once = true;						\
				mtdisp_pr_err("HDMI-%d: %s() "					\
					      fmt, ctx->id, __func__, ##__VA_ARGS__);		\
			}									\
		}										\
	} while (0)

#define HDMI_WARN_RATELIMITED(fmt, ...)								\
	do {											\
		if (HDMI_DEBUG_CTL & DISP_PRINT_WARN)						\
			mtdisp_pr_warn_ratelimited("HDMI-%d: %s() "				\
				fmt, ctx->id, __func__, ##__VA_ARGS__);				\
	} while (0)

/* dp */
#define DP_TRACE()										\
	do {											\
		if (DP_DEBUG_CTL & DISP_PRINT_TRACE)						\
			mtdisp_pr_info("DP-%d: %s()\n", ctx->id, __func__);			\
	} while (0)

#define DP_DEBUG(fmt, ...)									\
	do {											\
		if (DP_DEBUG_CTL & DISP_PRINT_DEBUG)						\
			mtdisp_pr_info("DP-%d: %s() " fmt, ctx->id, __func__, ##__VA_ARGS__);	\
	} while (0)

#define DP_INFO(fmt, ...)									\
	do {											\
		if (DP_DEBUG_CTL & DISP_PRINT_INFO)						\
			mtdisp_pr_info("DP-%d: %s() " fmt, ctx->id, __func__, ##__VA_ARGS__);	\
	} while (0)

#define DP_WARN(fmt, ...)									\
	do {											\
		if (DP_DEBUG_CTL & DISP_PRINT_WARN)						\
			mtdisp_pr_warn("DP-%d: %s() " fmt, ctx->id, __func__, ##__VA_ARGS__);	\
	} while (0)

#define DP_ERROR(fmt, ...)									\
	do {											\
		if (DP_DEBUG_CTL & DISP_PRINT_ERROR)						\
			mtdisp_pr_err("DP-%d: %s() " fmt, ctx->id, __func__, ##__VA_ARGS__);	\
	} while (0)

#define DP_DBG_REG(fmt, ...)									\
	do {											\
		if (DP_DEBUG_CTL & DISP_PRINT_REG)						\
			mtdisp_pr_info("DP-%d: %s() " fmt, ctx->id, __func__, ##__VA_ARGS__);	\
	} while (0)

#define DP_INFO_ONCE(fmt, ...)									\
	do {											\
		if (DP_DEBUG_CTL & DISP_PRINT_INFO) {						\
			static bool __print_once;						\
			if (!__print_once) {							\
				__print_once = true;						\
				mtdisp_pr_info("DP-%d: %s() "					\
					       fmt, ctx->id, __func__, ##__VA_ARGS__);		\
			}									\
		}										\
	} while (0)

#define DP_WARN_ONCE(fmt, ...)									\
	do {											\
		if (DP_DEBUG_CTL & DISP_PRINT_WARN) {						\
			static bool __print_once;						\
			if (!__print_once) {							\
				__print_once = true;						\
				mtdisp_pr_warn("DP-%d: %s() "					\
					       fmt, ctx->id, __func__, ##__VA_ARGS__);		\
			}									\
		}										\
	} while (0)

#define DP_ERROR_ONCE(fmt, ...)									\
	do {											\
		if (DP_DEBUG_CTL & DISP_PRINT_ERROR) {						\
			static bool __print_once;						\
			if (!__print_once) {							\
				__print_once = true;						\
				mtdisp_pr_err("DP-%d: %s() "					\
					      fmt, ctx->id, __func__, ##__VA_ARGS__);		\
			}									\
		}										\
	} while (0)

/* mtdisp */
#define MTDISP_INFO(fmt, ...)	\
	mtdisp_pr_info("%s() " fmt, __func__, ##__VA_ARGS__);
#define MTDISP_WARN(fmt, ...)	\
	mtdisp_pr_warn("%s() " fmt, __func__, ##__VA_ARGS__);
#define MTDISP_ERROR(fmt, ...)	\
	mtdisp_pr_err("%s() " fmt, __func__, ##__VA_ARGS__);

#endif /* __MTDISP_DEBUG_H__ */

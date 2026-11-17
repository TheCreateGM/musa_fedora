/*
 * @Copyright   Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License     Dual MIT/GPLv2
 */

#ifndef __LINUX_DISPLAY_DEBUG_H__
#define __LINUX_DISPLAY_DEBUG_H__

#include "mtdisp_printk.h"

/* linux dispc */
#define LINUX_DISPC_TRACE()									\
({												\
	if (DISPC_DEBUG_CTL & DISP_PRINT_TRACE)							\
		pr_info("mtdisp-dispc-%d: %s()", dispc->id, __func__);				\
})

#define LINUX_DISPC_DEBUG(fmt, ...)								\
({												\
	if (DISPC_DEBUG_CTL & DISP_PRINT_DEBUG)							\
		pr_info("mtdisp-dispc-%d: %s() " fmt, dispc->id, __func__, ##__VA_ARGS__);	\
})

#define LINUX_DISPC_INFO(fmt, ...)								\
({												\
	if (DISPC_DEBUG_CTL & DISP_PRINT_INFO)							\
		pr_info("mtdisp-dispc-%d: %s() " fmt, dispc->id, __func__, ##__VA_ARGS__);	\
})

#define LINUX_DISPC_WARN(fmt, ...)								\
({												\
	if (DISPC_DEBUG_CTL & DISP_PRINT_WARN)							\
		pr_warn("mtdisp-dispc-%d: %s() " fmt, dispc->id, __func__, ##__VA_ARGS__);	\
})

#define LINUX_DISPC_ERROR(fmt, ...)								\
({												\
	if (DISPC_DEBUG_CTL & DISP_PRINT_ERROR)							\
		pr_err("mtdisp-dispc-%d: *ERROR* %s() "						\
		       fmt, dispc->id, __func__, ##__VA_ARGS__);				\
})

/* linux hdmi */
#define LINUX_HDMI_TRACE()									\
({												\
	if (HDMI_DEBUG_CTL & DISP_PRINT_TRACE)							\
		pr_info("mtdisp-hdmi-%d: %s()", hdmi->id, __func__);				\
})

#define LINUX_HDMI_DEBUG(fmt, ...)								\
({												\
	if (HDMI_DEBUG_CTL & DISP_PRINT_DEBUG)							\
		pr_info("mtdisp-hdmi-%d: %s() " fmt, hdmi->id, __func__, ##__VA_ARGS__);	\
})

#define LINUX_HDMI_INFO(fmt, ...)								\
({												\
	if (HDMI_DEBUG_CTL & DISP_PRINT_INFO)							\
		pr_info("mtdisp-hdmi-%d: %s() " fmt, hdmi->id, __func__, ##__VA_ARGS__);	\
})

#define LINUX_HDMI_WARN(fmt, ...)								\
({												\
	if (HDMI_DEBUG_CTL & DISP_PRINT_WARN)							\
		pr_warn("mtdisp-hdmi-%d: %s() " fmt, hdmi->id, __func__, ##__VA_ARGS__);	\
})

#define LINUX_HDMI_ERROR(fmt, ...)								\
({												\
	if (HDMI_DEBUG_CTL & DISP_PRINT_ERROR)							\
		pr_err("mtdisp-hdmi-%d: *ERROR* %s() " fmt, hdmi->id, __func__, ##__VA_ARGS__);	\
})

/* linux dp */
#define LINUX_DP_TRACE()									\
({												\
	if (DP_DEBUG_CTL & DISP_PRINT_TRACE)							\
		pr_info("mtdisp-dp-%d: %s()", dp->id, __func__);				\
})

#define LINUX_DP_DEBUG(fmt, ...)								\
({												\
	if (DP_DEBUG_CTL & DISP_PRINT_DEBUG)							\
		pr_info("mtdisp-dp-%d: %s() " fmt, dp->id, __func__, ##__VA_ARGS__);		\
})

#define LINUX_DP_INFO(fmt, ...)									\
({												\
	if (DP_DEBUG_CTL & DISP_PRINT_INFO)							\
		pr_info("mtdisp-dp-%d: %s() " fmt, dp->id, __func__, ##__VA_ARGS__);		\
})

#define LINUX_DP_WARN(fmt, ...)									\
({												\
	if (DP_DEBUG_CTL & DISP_PRINT_WARN)							\
		pr_warn("mtdisp-dp-%d: %s() " fmt, dp->id, __func__, ##__VA_ARGS__);		\
})

#define LINUX_DP_ERROR(fmt, ...)								\
({												\
	if (DP_DEBUG_CTL & DISP_PRINT_ERROR)							\
		pr_err("mtdisp-dp-%d: *ERROR* %s() " fmt, dp->id, __func__, ##__VA_ARGS__);	\
})

#define LINUX_MST_TRACE()									\
({												\
	if (DP_DEBUG_CTL & DISP_PRINT_TRACE)							\
		pr_info("mtdisp-dp-mst-%d: %s()", dp_mst->stream_id, __func__);			\
})

#define LINUX_MST_DEBUG(fmt, ...)								\
({												\
	if (DP_DEBUG_CTL & DISP_PRINT_DEBUG)							\
		pr_info("mtdisp-dp-mst-%d: %s() "						\
			fmt, dp_mst->stream_id, __func__, ##__VA_ARGS__);			\
})

#define LINUX_MST_INFO(fmt, ...)								\
({												\
	if (DP_DEBUG_CTL & DISP_PRINT_INFO)							\
		pr_info("mtdisp-dp-mst-%d: %s() "						\
			fmt, dp_mst->stream_id, __func__, ##__VA_ARGS__);			\
})

#define LINUX_MST_WARN(fmt, ...)								\
({												\
	if (DP_DEBUG_CTL & DISP_PRINT_WARN)							\
		pr_warn("mtdisp-dp-mst-%d: %s() "						\
			fmt, dp_mst->stream_id, __func__, ##__VA_ARGS__);			\
})

#define LINUX_MST_ERROR(fmt, ...)								\
({												\
	if (DP_DEBUG_CTL & DISP_PRINT_ERROR)							\
		pr_err("mtdisp-dp-mst-%d: *ERROR* %s() "					\
		       fmt, dp_mst->stream_id, __func__, ##__VA_ARGS__);			\
})

/* linux dummy */
#define LINUX_DUMMY_CRTC_DEBUG(fmt, ...)							\
	pr_debug("mtdisp-dummy-crtc-%d: %s() " fmt, dummy_crtc->id, __func__, ##__VA_ARGS__)
#define LINUX_DUMMY_CRTC_INFO(fmt, ...)								\
	pr_info("mtdisp-dummy-crtc-%d: %s() " fmt, dummy_crtc->id, __func__, ##__VA_ARGS__)
#define LINUX_DUMMY_CRTC_WARN(fmt, ...)								\
	pr_warn("mtdisp-dummy-crtc-%d: %s() " fmt, dummy_crtc->id, __func__, ##__VA_ARGS__)
#define LINUX_DUMMY_CRTC_ERROR(fmt, ...)							\
	pr_err("mtdisp-dummy-crtc-%d: *ERROR* %s() " fmt, dummy_crtc->id, __func__, ##__VA_ARGS__)

#define LINUX_DUMMY_CONN_DEBUG(fmt, ...)							\
	pr_debug("mtdisp-dummy-conn-%d: %s() " fmt, dummy_conn->id, __func__, ##__VA_ARGS__)
#define LINUX_DUMMY_CONN_INFO(fmt, ...)								\
	pr_info("mtdisp-dummy-conn-%d: %s() " fmt, dummy_conn->id, __func__, ##__VA_ARGS__)
#define LINUX_DUMMY_CONN_WARN(fmt, ...)								\
	pr_warn("mtdisp-dummy-conn-%d: %s() " fmt, dummy_conn->id, __func__, ##__VA_ARGS__)
#define LINUX_DUMMY_CONN_ERROR(fmt, ...)							\
	pr_err("mtdisp-dummy-conn-%d: *ERROR* %s() " fmt, dummy_conn->id, __func__, ##__VA_ARGS__)

#endif /* __LINUX_DISPLAY_DEBUG_H__ */

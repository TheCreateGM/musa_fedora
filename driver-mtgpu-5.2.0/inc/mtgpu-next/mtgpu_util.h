/*
 * @Copyright Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License Dual MIT/GPLv2
 */

#ifndef _MTGPU_UTIL_H_
#define _MTGPU_UTIL_H_

#include "lock_types.h"
#include "img_types.h"
#include "mtgpu_error.h"

struct _MTGPU_DEVICE_NODE_;
struct mtgpu_device_node;
struct work_struct;

struct mtgpu_gpu_util_stats {
	bool	valid;			/* Is the data valid */
	u64	gpu_stat_active;	/* GPU active statistic */
	u64	gpu_stat_idle;		/* GPU idle statistic */
	u64	gpu_stat_cumulative;	/* Sum of active/idle stats */
	u64	timestamp;		/* Timestamp of the most recent sample of the GPU stats */
};

struct mtgpu_gpu_util_item_info {
	u32     dm;
	char *const name;
};

int mtgpu_util_stats_register(void **gpu_util_user);

void mtgpu_util_stats_unregister(void *gpu_util_user);

MTGPU_ERROR mtgpu_util_config_apphint_query(const struct _MTGPU_DEVICE_NODE_ *mt_dev_node,
					     const void *private, MT_UINT32 *value);

MTGPU_ERROR mtgpu_util_config_apphint_set(const struct _MTGPU_DEVICE_NODE_ *mt_dev_node,
					   const void *private, MT_UINT32 value);

void mtgpu_util_delayed_work_func(struct work_struct *work);

int mtgpu_util_info_reset(struct mtgpu_device_node *dev_node);

void mtgpu_util_stats_reset(void *gpu_util_stats);

int mtgpu_util_stats_get_all(struct mtgpu_device_node *dev_node,
			     void *gpu_util_user,
			     struct mtgpu_gpu_util_stats *return_stats);

int mtgpu_util_stats_get_overall(struct mtgpu_device_node *dev_node,
				 void *gpu_util_user,
				 struct mtgpu_gpu_util_stats *return_stats);

#endif /* _MTGPU_UTIL_H_ */

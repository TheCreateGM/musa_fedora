/*
 * @Copyright Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License Dual MIT/GPLv2
 */

#ifndef __MTGPU_DEVFREQ_H__
#define __MTGPU_DEVFREQ_H__

/* devfrreq default polling time */
#define MTGPU_DEVFREQ_POLLING_MS (200)

/* gpu util threshold for pm-runtime fast suspend */
#define MTGPU_PM_RUNTIME_FAST_SUSPEND_UTIL_THRESHOLD (20)

struct mtgpu_device_node;
struct thermal_cooling_device_ops;
struct thermal_cooling_device;
struct devfreq;
struct device;
struct devfreq_dev_profile;
struct devfreq_simple_ondemand_data;

typedef int (*fn_frequency_set)(struct mtgpu_device_node *dev_node, unsigned long freq);
typedef int (*fn_voltage_set)(struct mtgpu_device_node *dev_node, unsigned long volt);

enum mtgpu_devfreq_mode {
	MTGPU_DEVFREQ_MODE_NORMAL = 0,
	MTGPU_DEVFREQ_MODE_POWER_SAVE,
	MTGPU_DEVFREQ_MODE_FULL_POWER,
	MTGPU_DEVFREQ_MODE_PERF,
	MTGPU_DEVFREQ_MODE_FIXED,
	MTGPU_DEVFREQ_MODE_MAX,
};

struct mtgpu_cooling {
	struct thermal_cooling_device_ops *ops;
	struct thermal_cooling_device *cdev;
	u32 state_current;
	u32 state_max;
};

struct mtgpu_opp {
	u32 volt;
	u32 freq;
};

struct mtgpu_devfreq_config {
	struct mtgpu_opp *opp;
	u32 opp_count;
	unsigned long polling_ms;
	u32 up_threshold;
	u32 down_differential;

	void (*set_utilization)(struct device *dev, u32 util);
	void (*set_frequency)(struct device *dev, u32 freq);
	void (*set_voltage)(struct device *dev, u32 volt);
};

struct mtgpu_devfreq {
	struct mtgpu_device_node *dev_node;
	struct devfreq *devfreq;
	struct mtgpu_devfreq_config config;
	struct devfreq_dev_profile *profile;
	struct devfreq_simple_ondemand_data *ondemand_data;

	struct mtgpu_cooling cooling;

	bool enabled;

	unsigned long freq_min;
	unsigned long freq_max;
	unsigned long freq_capping;
	unsigned long freq_latest;
	unsigned long freq_store;
	unsigned long freq_fixed;

	unsigned long *freq_table;
	u32 freq_count;

	void *gpu_util_user;

	enum mtgpu_devfreq_mode mode;

	atomic_t util_latest;
};

int mtgpu_devfreq_init(struct mtgpu_device_node *dev_node);
void mtgpu_devfreq_exit(struct mtgpu_device_node *dev_node);
int mtgpu_devfreq_suspend(struct mtgpu_devfreq *mtgpu_devfreq);
int mtgpu_devfreq_resume(struct mtgpu_devfreq *mtgpu_devfreq);
void mtgpu_devfreq_set_enabled(struct mtgpu_devfreq *mtgpu_devfreq);
void mtgpu_devfreq_set_disabled(struct mtgpu_devfreq *mtgpu_devfreq);
int mtgpu_devfreq_mode_setting(struct mtgpu_device_node *dev_node, u32 mode, unsigned long freq);
int mtgpu_devfreq_latest_util_get(struct mtgpu_device_node *dev_node, u32 *uitl);
int mtgpu_devfreq_get_dvfs_info(struct mtgpu_device_node *dev_node, u32 *mode, unsigned long *freq);

#endif

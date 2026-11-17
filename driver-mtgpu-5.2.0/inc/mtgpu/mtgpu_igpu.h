/*
 * @Copyright   Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License     Dual MIT/GPLv2
 */

#ifndef __MTGPU_IGPU_H__
#define __MTGPU_IGPU_H__

/* IGPU's default delay suspend time */
#define MTGPU_IGPU_SUSPEND_DELAY_MS 20

struct device;
struct platform_device;
struct mtgpu_device;

struct mtgpu_igpu_dvfs {
	/* minimum frequency(MHz) */
	u32 freq_min;
	/* maximum frequency(MHz) */
	u32 freq_max;
	/* backup frequency(MHz) */
	u32 freq_store;
	/* Supported frequency points count */
	u32 freq_points_count;

	/* Write frequency into memory which will be used
	 * by soc-firmware.
	 */
	void __iomem *share_mem;

	/* Write utilization into memory which will be used
	 * by soc-firmware.
	 */
	void __iomem *report_util;
};

int mtgpu_igpu_probe(struct platform_device *pdev);
int mtgpu_igpu_remove(struct platform_device *pdev);
int mtgpu_igpu_utilization_set(struct mtgpu_device *mtdev, u32 util);
int mtgpu_igpu_frequency_set(struct mtgpu_device *mtdev, u32 freq);
int mtgpu_igpu_get_frequency_limits(struct mtgpu_device *mtdev,
				    u32 *min_freq, u32 *max_freq);
int mtgpu_igpu_get_frequency_points_count(struct mtgpu_device *mtdev,
					  u32 *count);
int mtgpu_igpu_pm_suspend(struct device *dev);
int mtgpu_igpu_pm_resume(struct device *dev);
void mtgpu_igpu_shutdown(struct platform_device *pdev);
int mtgpu_igpu_pm_runtime_suspend(struct device *dev);
int mtgpu_igpu_pm_runtime_resume(struct device *dev);
void mtgpu_igpu_pm_force_reset(struct device *dev);
bool mtgpu_igpu_has_ce_pm_support(struct mtgpu_device *mtdev);

#endif /* __MTGPU_IGPU_H__ */

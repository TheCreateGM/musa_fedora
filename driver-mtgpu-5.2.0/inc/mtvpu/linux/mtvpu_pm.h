#ifndef _MTVPU_PM_H_
#define _MTVPU_PM_H_

#include "linux-types.h"
#include "os-interface.h"
#include "mtvpu_drv.h"

#define PH1_MTBIOS_SUPPORT_PM_VERSION     (40207)
#define PH1_MTBIOS_SUPPORT_MTLINK_VERSION (40330)
#define PH1_MTBIOS_VERSION_MASK       (100000000)

#define QY1_PM_REG_PMU_IDLE_SEL0  0x7f00130010
#define QY1_PM_REG_PMU_IDLE_CTRL  0x7f00130040
#define QY1_PM_REG_PMU_IDLE_DONE  0x7f00130000
#define QY1_PM_REG_PMU_VID_CLOCK  0x7f0012a200
#define QY1_PM_REG_PMU_VID_RESET  0x7f0012a204

#define SUDI_PM_REG_PMU_IDLE_SEL0  0x7F00082404
#define SUDI_PM_REG_PMU_IDLE_CTRL  0x7F0008240c
#define SUDI_PM_REG_PMU_IDLE_CTRL2 0x7F00082410
#define SUDI_PM_REG_PMU_IDLE_DONE  0x7F00082424
#define SUDI_PM_REG_PMU_VID_CLOCK  0x7F000a0078
#define SUDI_PM_REG_PMU_VID_RESET  0x7F000a0088

#define VPU_CORE_SUSPEND_DELAY 2000 /* modify this with caution! */
#define VPU_SUSPEND_DELAY      2500

int vpu_reset_core(struct mt_chip *chip, int idx);
int vpu_is_support_pstate(struct mt_chip *chip);
int vpu_core_is_support_pstate(struct mt_chip *chip, int idx);
int vpu_core_power_ctl(struct mt_chip *chip, int idx, int onoff);
int vpu_sleep_wake(u32 core_idx, int is_sleep_wake);
int vpu_hw_deinit(u32 core_idx);
int vpu_suspend(struct device *dev);
int vpu_resume(struct device *dev);
void vpu_complete(struct device *dev);

int vpu_rpm_init(struct device *dev, int delay);
void vpu_rpm_exit(struct device *dev);
int vpu_rpm_core_suspend(struct device *dev);
int vpu_rpm_core_resume(struct device *dev);
int vpu_core_prepare(struct device *dev);
void vpu_core_complete(struct device *dev);

void vpu_report_power_state(struct mt_chip *chip, int inst_cnt_change);

#endif

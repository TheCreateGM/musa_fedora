#ifndef _MTVPU_CLK_H_
#define _MTVPU_CLK_H_

#include "linux-types.h"
#include "mtvpu_drv.h"

#define VID_CRG_REG     0x2c0000
#define VID_DIV01_OFFSET     0x4
#define VID_DIV2345_OFFSET   0x8

#define M1000_SHARED_MEM_ADDR 0x280fd000
#define VID_FREQ_OFFSET            0x278
#define VID_UTIL_OFFSET            0x2bc

struct opp {
	u64 volt;
	u64 freq;
};

s64 vpu_get_clk(struct mt_chip *chip, int idx);
s64 vpu_get_max_clk(struct mt_chip *chip, int idx);
int m1000_set_freq(struct mt_chip *chip, const unsigned long freq);

int vpu_dvfs_init(struct mt_chip *chip);
int vpu_dvfs_deinit(struct mt_chip *chip);

#endif
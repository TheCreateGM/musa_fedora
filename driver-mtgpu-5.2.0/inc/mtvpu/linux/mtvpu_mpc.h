/*
 * @Copyright   Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License     Dual MIT/GPLv2
 */

#ifndef __MTVPU_MPC_H__
#define __MTVPU_MPC_H__

#include "mtvpu_drv.h"

int vpu_init_mpc(struct mt_chip *chip);
int vpu_get_drm_id(struct drm_device *drm);
bool vpu_drm_core_valid(struct mt_chip *chip, struct drm_device *drm, u32 core_idx);

#endif
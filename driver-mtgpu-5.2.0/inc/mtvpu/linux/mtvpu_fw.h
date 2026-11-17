/*
 * @Copyright   Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License     Dual MIT/GPLv2
 */

#ifndef _MTVPU_FW_H_
#define _MTVPU_FW_H_

#include "mtvpu_drv.h"

int vpu_load_firmware(struct mt_chip *chip, int idx, struct mt_virm *vm);
int vpu_check_fw_version(struct mt_chip *chip, int idx);
int vpu_fw_get_header(struct mt_chip *chip);
int vpu_fw_version_check(struct mt_chip *chip);

#endif

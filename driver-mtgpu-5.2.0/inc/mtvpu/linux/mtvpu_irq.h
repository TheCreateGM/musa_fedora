/*
 * @Copyright   Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License     Dual MIT/GPLv2
 */

#ifndef _MTVPU_IRQ_H_
#define _MTVPU_IRQ_H_

#include "mtvpu_drv.h"

int vpu_init_irq(struct mt_chip *chip, struct platform_device *pdev);
void vpu_free_irq(struct mt_chip *chip);

int vpu_create_cmd_queue(struct mt_vpu_cmd_queue **cmdq, struct CodecInst *handle, struct mt_virm *vm);
void vpu_destroy_cmd_queue(struct mt_vpu_cmd_queue *cmdq);

int vpu_create_irq_works(struct mt_core *core, int inst_idx);
void vpu_destroy_irq_works(struct mt_core *core, int inst_idx);

#endif
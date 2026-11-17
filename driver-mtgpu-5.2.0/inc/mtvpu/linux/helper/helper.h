/*
 * @Copyright   Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License     Dual MIT/GPLv2
 */

#ifndef _HELPER_H_
#define _HELPER_H_

#include "vpuapi.h"

struct core_info {
	u32 jpu;
	u32 legacy_dec;
	u32 vdec;
	u32 venc;
};

RetCode SetDecBufPool(DecHandle hdl, DecInitialInfo *info, RenderTargetInfo *render);

void setPvricValue(DecParam *decParam);
int FillBufferInfo(vpu_buffer_t* vb, u64 handle, u64 size, u32 type,
		  struct drm_file *file, struct mt_virm *vm, struct mt_core *core);
void get_core_info(struct mt_chip *chip, struct core_info *c_info);
int wait_vm_idle(struct mt_virm *vm);

int init_timer(struct mt_chip *chip);
int vpu_add_bo_to_list(struct drm_file *file, u64 handle, struct mt_core *core, int inst_id);
void vpu_free_bo_list(struct mt_core *core, int inst_id);
#endif

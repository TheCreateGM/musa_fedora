/*
 * @Copyright   Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License     Dual MIT/GPLv2
 */

#ifndef _MTVPU_MGN_H_
#define _MTVPU_MGN_H_

#include "mtvpu_cmd.h"
#include "vpuapifunc.h"

#define VM_INST_NUM_MAX INST_MAX_SIZE
#define MGN_MARK_STRING 0x1A2B3C4D

struct mt_chip;
struct mt_virm;

enum migration_status {
	MGN_STATUS_NOT_START = 0,
	MGN_STATUS_SAVEING,
	MGN_STATUS_SAVED,
	MGN_STATUS_HOT_UPGRADE,
};

enum mgn_cmd_type {
	MGN_CMD_DEFAULT = 0,
	MGN_CMD_HOST_CLOSE,
};

struct mt_logic_handle {
	CodecInst *real_handle;
	u32 core_idx;
	u32 inst_idx;

	u32 mgn_mark_string;
	u32 mgn_status;
	int skip_coreIdx;
	int request_coreIdx;

	CodecInst backup_handle;
	u32 codeInfoSize;
	struct mt_inst_info backup_inst_info;
	struct mt_inst_extra backup_inst_extra;
	u32 vm_id;
	struct mt_logic_handle  *old_logic_handle;
};

struct migration_buffer {
	u32 size;
	u32 dec_cnt;
	u32 enc_cnt;
	struct mt_logic_handle dec_ctx[INST_MAX_SIZE];
	struct mt_logic_handle enc_ctx[INST_MAX_SIZE];
	u8 *data;
};

int save_codec_handle(struct mt_chip *chip, struct mt_virm *vm,
		       struct mt_logic_handle *src_handle, struct mt_logic_handle *dst_handle);
int save_codec_info(struct mt_chip *chip, struct mt_virm *vm, struct mt_logic_handle *src_handle,
		     void *dst);
int parse_migrated_codec_inst(struct mt_chip *chip, int vm_id);
int restore_single_inst(struct mt_chip *chip, struct mt_logic_handle *logic_handle,
			struct mt_virm *vm, struct drm_file *file, int cmd_type);
int get_real_handle(struct mt_logic_handle *input, struct CodecInst **real_handle);

int vpu_mgn_check(struct mt_chip *chip,
		  struct mt_logic_handle *logic_handle,
		  CodecInst **real_handle,
		  struct mt_virm *vm,
		  struct drm_file *file,
		  int cmd_type,
		  UpdatePaInfo_t *paUpdateArray);
void vgpu_hot_upgrade_save_vpu(void *dev);
void vgpu_hot_upgrade_restore_vpu(void *dev);
int vgpu_hot_upgrade_wait_vpu_idle(void *dev);
int vgpu_hot_upgrade_vpu_end(void *dev);
#endif /* _MTVPU_MGN_H_ */

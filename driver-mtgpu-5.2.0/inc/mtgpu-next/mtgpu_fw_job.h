/*
 * @Copyright Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License Dual MIT/GPLv2
 */

#ifndef _MTGPU_FW_JOB_H_
#define _MTGPU_FW_JOB_H_

#include "musaccb.h"
#include "musajob.h"
#include "mtfw_fwif.h"
#include "mtfw_fwif_fw_dump.h"
#include "mtgpu_job.h"

#define MTGPU_WAIT_SEM_CHECKED_TIMEOUT_MS	(2000)
#define MTGPU_CCB_POOL_INDEX(queue_type, index)	((queue_type) * MTGPU_CCB_POOL_LENGTH + (index))

struct mtgpu_vm_context;

bool mtgpu_fw_is_idle(struct mtgpu_device_node *dev_node);
int mtgpu_fw_get_ccb_woff(MTFW_RING_CTRL *ring_ctrl, u32 *new_woff);
bool mtgpu_fw_is_idle(struct mtgpu_device_node *dev_node);
int mtgpu_fw_job_item_ccb_release(struct mtgpu_fw_info *fw_info,
				  u32 queue_type,
				  u32 pool_index);
int mtgpu_fw_job_item_ccb_acquire(struct mtgpu_device_node *dev_node,
				  u32 queue_type,
				  struct mtgpu_job_item **job_item_out);
int mtgpu_fw_cmd_submit(struct mtgpu_device_node *dev_node,
			MTFW_FWIF_PRIORITY priority,
			u32 queue_type,
			MTFW_SUBMISSION_CMD_TYPE sub_cmd,
			MTFW_CCB_ITEM_PARA *ccb_item_para,
			struct mtgpu_job_list *job_list,
			struct mtgpu_job_context *job_ctx,
			u32 submission_flags,
			const char *process_name,
			u32 process_id,
			bool is_legacy,
			bool need_sync,
			u32 sync_timeout_ms,
			struct mtgpu_job_item *job_item);
void mtgpu_fw_set_abort_submission_last_err(struct mtgpu_job_item *job_item);
void mtgpu_fw_save_submission_last_error(struct mtgpu_job_item *job_item, u64 submission_error);
void mtgpu_fw_set_process_fault_submission(struct mtgpu_job_item *job_item, u64 submission_error);
void mtgpu_fw_check_resp_queue(struct mtgpu_device_node *dev_node, u64 irq_ordinal);
void mtgpu_fw_ccb_blocking_wakeup(struct mtgpu_device_node *dev_node);
void mtgpu_fw_sync_cmd_wakeup(struct mtgpu_device_node *dev_node);
void mtgpu_fw_irq_work_func(void *pvData);
int mtgpu_fw_cmd_resubmit(struct mtgpu_device_node *dev_node, struct mtgpu_job_item *job_item);
int mtgpu_fw_resubmit_jobs(struct mtgpu_device_node *dev_node, struct mtgpu_vm_context *vm_ctx);
int mtgpu_fw_resubmit_memctx_jobs(struct mtgpu_device_node *dev_node, MEM_CTX *vm_ctx);
int mtgpu_fw_mmu_invalid_cmd_submit(struct mtgpu_device_node *dev_node,
				    struct mtgpu_vm_context *vm_ctx,
				    struct mtgpu_syncobj **update_syncobjs,
				    u64 *rm_update_syncobjs,
				    u64 *update_values,
				    u32 update_semaphore_count,
				    u64 vaddr,
				    u64 size);
int mtgpu_fw_cancel_cmd_submit(struct mtgpu_vm_context *vm_ctx,
			       struct mtgpu_job_context *job_ctx,
			       MTFW_SUBMISSION_CMD_TYPE sub_cmd);
int mtgpu_fw_halt_cmd_submit(struct mtgpu_device_node *dev_node, u64 reason);
void mtgpu_fw_ccb_item_dump(MTFW_CCB_ITEM *item, fw_va item_fwaddr);
int mtgpu_queue_ccb_job_item_acquire(struct mtgpu_device_node *dev_node,
				     struct mtgpu_job_item **job_item_out);
void mtgpu_queue_ccb_job_item_release(struct mtgpu_job_item *job_item);
bool mtgpu_try_to_get_available_ccb_index(unsigned long *bitmap, struct spinlock *lock,
					  u64 *start_search, u64 *pool_index);

#endif /* _MTGPU_FW_JOB_H_ */

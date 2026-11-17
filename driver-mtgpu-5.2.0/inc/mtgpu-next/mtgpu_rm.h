/*
 * @Copyright Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License Dual MIT/GPLv2
 */

#ifndef _MTGPU_RM_H_
#define _MTGPU_RM_H_

#include "mtfw_fwif_ccb.h"

struct mtgpu_device;
struct mtgpu_device_node;
struct mtgpu_vm_context;
struct mtgpu_fw_info;
struct dma_xfer_block;

typedef void (DUMPDEBUG_PRINTF_FUNC)(void *dump_debug_file, const char *fmt, ...);

struct mtgpu_queue_ccb_item_params {
	// check_sem_handle is valid for RM_FEATURE_LEVEL_SYNCOBJ
	u64 check_sem_handle[MTFW_DEPEND_Q_SEM_NUMS];
	u64 check_sem_addr[MTFW_DEPEND_Q_SEM_NUMS];
	u64 check_sem_shadow_addr[MTFW_DEPEND_Q_SEM_NUMS];
	u64 check_sem_value[MTFW_DEPEND_Q_SEM_NUMS];
	u32 check_sem_count;

	// update_sem_handle is valid for RM_FEATURE_LEVEL_SYNCOBJ
	u64 update_sem_handle[MTFW_FINISH_Q_SEM_NUMS];
	u64 update_sem_addr[MTFW_FINISH_Q_SEM_NUMS];
	u64 update_sem_shadow_addr[MTFW_FINISH_Q_SEM_NUMS];
	u64 update_sem_value[MTFW_FINISH_Q_SEM_NUMS];
	u32 update_sem_count;

	/* gpu submission para specific */
	u64 job_ctx_handle;
	u64 submission_va;
	u64 submission_size;
	u32 doorbell_id;
	union {
		struct {
			uint32_t sem_out_of_order : 1;
			uint32_t with_doorbell : 1;
		} field;
		uint32_t value;
	} submission_flags;

	/* invalid tlb para specific */
	u64 invalid_addr;
	u64 invalid_size;

	/* log config para specific */
	u32 log_group;

	/* gpu util config para specific */
	u32 gpu_util_cfg;

	/* musa-gdb config para specific */
	u64 gdb_cmd_type;
	u64 target_pc_root;
	u64 gdb_cfg_meta_addr;
	u64 gdb_cfg_date_size;

	/* vgpu cmd start */
	u64 qos_limit;
	union {
		u64 sys_mem_size; /* This is the system memory size to qos */
		u32 sche_time;   /* This is the time to reset vm schedule info */
	} sche_info;

	u32 osid;
	/* vgpu cmd end */
};

int mtgpu_rm_init(void);
void mtgpu_rm_deinit(void);
int mtgpu_rm_dump(u32 dump_level);

int rm_err_to_linux_errno(int rm_ret);

int rm_gpu_device_alloc(struct mtgpu_device_node *dev_node, u32 mpc_id);
void rm_gpu_device_free(struct mtgpu_device_node *dev_node);
int rm_gpu_device_init(struct mtgpu_device_node *dev_node);
void rm_gpu_device_deinit(struct mtgpu_device_node *dev_node);
int rm_gpu_device_resume(struct mtgpu_device_node *dev_node);
uint64_t rm_gpu_hw_capability_get(struct mtgpu_device *mtdev);

int rm_dump_fault_info(struct mtgpu_device_node *dev_node);
void rm_dump_total_process_subm_info(struct mtgpu_device_node *dev_node,
				     DUMPDEBUG_PRINTF_FUNC *dump_debug_printf,
				     void *dump_debug_file);

void rm_gpu_state_request_reboot(struct mtgpu_device_node *dev_node, u32 request_type);
void rm_gpu_state_register_hwr_callback(struct mtgpu_device_node *dev_node,
					int (*hwr_cb)(void *priv, int stage, int req_type),
					void *priv);

int rm_fw_load(uint64_t rm_gpu,
	       struct mtgpu_device *mtdev,
	       uint64_t heap_base,
	       uint64_t heap_size,
	       bool watchdog_supported,
	       bool pc_sampling_supported,
	       bool item_pool_supported,
	       struct mtgpu_fw_info *fw_info);
int rm_fw_unload(uint64_t rm_gpu);
int rm_fw_reload(uint64_t rm_gpu);
int rm_vgpu_guest_fw_heap_alloc(struct mtgpu_device_node *dev_node,
				u32 fw_index, u64 dpa, u64 size, bool use_sysmem,
				void **cpu_va);
void rm_vgpu_guest_fw_heap_free(struct mtgpu_device_node *dev_node, u32 fw_index);
int rm_vgpu_set_mem_layout(struct mtgpu_device *mtdev);

int rm_intr_register(uint64_t rm_dev,
		     uint32_t int_vec,
		     void (*handler)(void *),
		     void *data,
		     const uint8_t *name);
int rm_intr_unregister(uint64_t rm_dev, uint32_t int_vec);
int rm_intr_enable(uint64_t rm_dev, uint32_t int_vec);
int rm_intr_disable(uint64_t rm_dev, uint32_t int_vec);
int rm_intr_suspend(uint64_t rm_dev);
int rm_intr_resume(uint64_t rm_dev);

int rm_softirq_register(uint64_t rm_dev,
			uint32_t int_vec,
			void (*handler)(void *),
			void *data);
int rm_softirq_unregister(uint64_t rm_dev, uint32_t int_vec);
int rm_softirq_enable(uint64_t rm_dev, uint32_t int_vec);
int rm_softirq_disable(uint64_t rm_dev, uint32_t int_vec);

int rm_ipc_transmit(u64 rm_dev,
		    struct ipc_msg *msg, u32 msg_size);

int rm_dma_transfer(u64 rm_master_dev, u64 rm_slave_dev, struct dma_xfer_block *xfer_blocks,
		    u32 block_nums, u32 xfer_type);
int rm_dma_transfer_kernel(u64 rm_master_dev,
			   u64 device_addr,
			   u64 *system_vaddrs,
			   u64 size,
			   bool mem_to_dev);

int rm_alloc_vm_context(struct mtgpu_device_node *dev_node, u64 rm_client, u64 rm_gpu_vaspace,
			u64 *p_rm_vm_context);
void rm_free_vm_context(u64 rm_client, u64 rm_vm_context);

int rm_queue_context_create(uint64_t rm_client,
			    uint64_t rm_vm_context,
			    uint32_t type,
			    uint32_t flags,
			    uint32_t priority,
			    uint64_t *rm_queue_ctx);
void rm_queue_context_destroy(uint64_t rm_client, uint64_t rm_queue_ctx);

int rm_queue_ccb_submit(uint64_t rm_gpu,
			uint64_t rm_client,
			uint64_t rm_vm_context,
			uint64_t rm_queue_ctx,
			uint32_t submission_cmd,
			void *ccb_item_params,
			void *job_item);
int rm_queue_ccb_submit_sync(uint64_t rm_gpu,
			     uint64_t rm_client,
			     uint64_t rm_vm_context,
			     uint64_t rm_queue_ctx,
			     uint32_t submission_cmd,
			     void *ccb_item_params,
			     uint32_t sync_timeout_ms,
			     void *job_item);
int rm_queue_ccb_process_resp(uint64_t rm_gpu);
int rm_vm_context_get_state(u64 rm_client, u64 rm_vm_context, u32 *state);
int rm_vm_context_wait_cleanup(u64 rm_client, u64 rm_vm_context);
int rm_queue_ccb_submission_list_cleanup(uint64_t rm_gpu, struct mtgpu_vm_context *vm_ctx);

int rm_doorbell_acquire(u64 rm_gpu,
			u64 rm_client,
			u64 rm_queue_ctx,
			u64 user_va,
			u64 *rm_doorbell,
			u32 *doorbell_id,
			u32 *doorbell_addr_offset);
void rm_doorbell_release(u64 rm_client,
			 u64 doorbell_handle);

int rm_llc_capability_get(uint64_t rm_dev,
			  uint32_t *llc_size,
			  uint32_t *llc_persisting_hw_max_size);
int rm_llc_persistence_get(uint64_t rm_dev,
			   uint32_t *llc_size,
			   uint32_t *llc_persisting_max_size);
int rm_llc_persistence_set(uint64_t rm_dev,
			   uint32_t replace_mode,
			   uint32_t max_set_aside_size);
void rm_llc_persistence_reset(uint64_t rm_dev);

uint64_t rm_mem_allocation_flags_from_mtgpu_flags(uint64_t mt_flags);

int rm_gpu_cfg_info_get(struct mtgpu_device *mtdev, struct gpu_cfg_info *gpu_info);

int rm_syncobj_create(u64 rm_gpu,
		      u32 type,
		      u32 flag,
		      u64 *rm_syncobj);
int rm_syncobj_destroy(u64 rm_syncobj);
u64 rm_syncobj_get_mem_handle(u64 rm_syncobj);
u64 rm_syncobj_get_shadow_mem_handle(u64 rm_syncobj);
u64 rm_syncobj_get_offset(u64 rm_syncobj);
u32 rm_syncobj_get_block_size(u64 rm_syncobj);
u32 rm_syncobj_get_type(u64 rm_syncobj);
u64 rm_syncobj_get_vram_value(u64 rm_syncobj);
u64 rm_syncobj_get_shadow_value(u64 rm_syncobj);
void rm_syncobj_set_vram_value(u64 rm_syncobj, u64 value);
void rm_syncobj_set_shadow_value(u64 rm_syncobj, u64 value);
u64 rm_syncobj_get_shadow_addr(u64 rm_syncobj);
void rm_syncobj_set_waiting_count(u64 rm_syncobj, bool inc);

#endif /* _MTGPU_RM_H_ */

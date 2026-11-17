/*
 * @Copyright Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License Dual MIT/GPLv2
 */

#ifndef _MTGPU_DMA_NEXT_H
#define _MTGPU_DMA_NEXT_H

struct drm_device;
struct drm_file;
struct mtgpu_session;
struct drm_mtgpu_semaphore_submit;
struct mtgpu_bo;

struct mtgpu_dma_work_data {
	struct drm_file *file_priv;
	struct device *dev;
	struct dma_xfer_block **block_data;
	u32 *block_index;
	struct mtgpu_fence *mtgpu_fence;
	struct drm_mtgpu_dma_cmd * dma_cmds;
	int dma_cmd_count;
	u8 *args_buffer;
	struct mtgpu_vm_context *vm_ctx;
	struct drm_mtgpu_semaphore *update_sem;
	struct mtgpu_syncobj **update_sema_syncobj;
	struct drm_mtgpu_semaphore *check_sema;
	struct mtgpu_syncobj **check_sema_syncobj;
	u32 update_sem_count;
	u32 check_sem_count;
	u64 page_table_root_addr;
	u64 job_ctx_handle;
	u64 submission_id;
};

int mtgpu_dma_transfer_p2p(struct mtgpu_device_node *dev_node,
			   u32 num_dmas, struct mtgpu_bo **local_bo, u64 *local_offset,
			   struct mtgpu_bo **peer_bo, u64 *peer_offset, u64 *xfer_size,
			   u32 flag, u32 ext_job_ref);

int mtgpu_dma_job_data_process(struct drm_file *file_priv,
			       u32 dma_cmd_count,
			       struct drm_mtgpu_dma_cmd * dma_cmds,
			       struct dma_xfer_block **xfer_block_data,
			       u32 *xfer_block_index);
int mtgpu_dma_job_submit_v3(struct drm_device *drm, struct drm_file *file_priv,
			    CONNECTION_DATA *conn, struct drm_mtgpu_job_submit_v3 *args);
int mtgpu_dma_context_create_v3(struct drm_mtgpu_job_context_create *args,
				struct mtgpu_session *session,
				struct mtgpu_device_node *dev_node);

#endif /* _MTGPU_DMA_NEXT_H */

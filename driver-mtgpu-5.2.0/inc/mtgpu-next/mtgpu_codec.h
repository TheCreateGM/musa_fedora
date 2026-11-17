/*
 * @Copyright Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License Dual MIT/GPLv2
 */

#ifndef MTGPU_CODEC_H
#define MTGPU_CODEC_H

#include "private_data.h"
#include "mtgpu_syncobj.h"
#include "mtgpu_semaphore.h"

struct drm_file;
struct drm_device;
struct mtgpu_job_context;
struct drm_mtgpu_job_context_create;
struct mtgpu_device_node;
struct mtgpu_session;

#define MTCODEC_DEPEND_Q_SEM_NUMS (32)
#define MTCODEC_FINISH_Q_SEM_NUMS (8)

struct mtgpu_codec_job_data {
	u32 check_syncobj_count;
	u32 update_syncobj_count;
	struct mtgpu_syncobj *check_syncobjs[MTCODEC_DEPEND_Q_SEM_NUMS];
	struct mtgpu_syncobj *update_syncobjs[MTCODEC_FINISH_Q_SEM_NUMS];
	u64 check_value[MTCODEC_DEPEND_Q_SEM_NUMS];
	u64 update_value[MTCODEC_FINISH_Q_SEM_NUMS];

	struct dma_fence *dep_fences[MTCODEC_DEPEND_Q_SEM_NUMS] ;
	struct mtgpu_semaphore_fence *update_sem_fences[MTCODEC_FINISH_Q_SEM_NUMS] ;
};

struct mtgpu_codec_priv_data {
	u64 page_table_root_addr;
	u64 job_ctx;
	u32 page_size;
	u32 check_semaphores_num;
	u32 update_semaphores_num;
	struct drm_mtgpu_semaphore check_semaphores[MTCODEC_DEPEND_Q_SEM_NUMS];
	struct drm_mtgpu_semaphore update_semaphores[MTCODEC_FINISH_Q_SEM_NUMS];
};

int mtgpu_codec_context_create(struct drm_mtgpu_job_context_create *args,
			       struct mtgpu_session *session,
			       struct mtgpu_device_node *dev_node);
int mtgpu_codec_wait_ioctl(struct drm_device *drm, void *data, struct drm_file *file_priv);
int mtgpu_codec_job_submit(struct drm_device *drm,
			   void *data,
			   struct drm_file *file_priv,
			   struct mtgpu_job_context *job_ctx);

#endif /* MTGPU_CODEC_H */

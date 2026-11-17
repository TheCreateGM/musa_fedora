/*
 * @File
 * @Title       PowerVR Linux fence interface
 * @Codingstyle LinuxKernel
 * @Copyright   Copyright (c) Imagination Technologies Ltd. All Rights Reserved
 * @License     Dual MIT/GPLv2
 *
 * The contents of this file are subject to the MIT license as set out below.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * Alternatively, the contents of this file may be used under the terms of
 * the GNU General Public License Version 2 ("GPL") in which case the provisions
 * of GPL are applicable instead of those above.
 *
 * If you wish to allow use of your version of this file only under the terms of
 * GPL, and not to allow others to use your version of this file under the terms
 * of the MIT license, indicate your decision by deleting the provisions above
 * and replace them with the notice and other provisions required by GPL as set
 * out in the file called "GPL-COPYING" included in this distribution. If you do
 * not delete the provisions above, a recipient may use your version of this file
 * under the terms of either the MIT license or GPL.
 *
 * This License is also included in this distribution in the file called
 * "MIT-COPYING".
 *
 * EXCEPT AS OTHERWISE STATED IN A NEGOTIATED AGREEMENT: (A) THE SOFTWARE IS
 * PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 * BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 * PURPOSE AND NONINFRINGEMENT; AND (B) IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#if !defined(__MT_FENCE_H__)
#define __MT_FENCE_H__

#include <linux/version.h>
#include "services_kernel_client.h"
#include "mt_linux_fence.h"
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>

struct SYNC_CHECKPOINT_CONTEXT_TAG;
struct SYNC_CHECKPOINT_TAG;

/**
 * mt_fence_context - MT fence context used to create and manage MT fences
 * @lock: protects the context and fences created on the context
 * @name: fence context name (used for debugging)
 * @dbg_request_handle: handle for callback used to dump debug data
 * @fence_context: fence context with which to associate fences
 * @fence_seqno: sequence number to use for the next fence
 * @fence_wq: work queue for signalled fence work
 * @check_status_work: work item used to inform services when a foreign fence
 * has signalled
 * @cmd_complete_handle: handle for callback used to signal fences when fence
 * syncs are met
 * @list_lock: protects the active and active foreign lists
 * @signal_list: list of fences waiting to be signalled
 * @fence_list: list of fences (used for debugging)
 * @deferred_free_list: list of fences that we will free when we are no longer
 * holding spinlocks.  The frees get implemented when an update fence is
 * signalled or the context is freed.
 */
struct mt_fence_context {
	spinlock_t lock;
	char name[32];
	void *dbg_request_handle;
	u64 fence_context;
	atomic_t fence_seqno;

	struct workqueue_struct *fence_wq;
	struct work_struct check_status_work;

	void *cmd_complete_handle;

	spinlock_t list_lock;
	struct list_head signal_list;
	struct list_head fence_list;
	struct list_head deferred_free_list;

	struct kref kref;
	struct work_struct destroy_work;
};

struct mt_fence_prim_data {
	s32 timeline_handle;
	s32 is_bind_sync_fd;
};

/**
 * mt_fence - MT fence that represents both native and foreign fences
 * @base: fence structure
 * @fctx: fence context on which this fence was created
 * @name: fence name (used for debugging)
 * @fence: pointer to base fence structure or foreign fence
 * @sync_checkpoint: services sync checkpoint used by hardware
 * @fence_head: entry on the context fence and deferred free list
 * @signal_head: entry on the context signal list
 * @cb: foreign fence callback to set the sync to signalled
 */
struct mt_fence {
	struct dma_fence base;
	struct mt_fence_context *fctx;
	char name[32];

	struct dma_fence *fence;
	struct SYNC_CHECKPOINT_TAG *sync_checkpoint;

	struct list_head fence_head;
	struct list_head signal_head;
	struct dma_fence_cb cb;
	struct rcu_head rcu;
	struct mt_fence_prim_data fence_prim_data;
};

extern const struct dma_fence_ops mt_fence_ops;
extern const struct dma_fence_ops mt_fence_foreign_ops;

static inline bool is_our_fence(struct mt_fence_context *fctx,
				struct dma_fence *fence)
{
	return (fence->context == fctx->fence_context);
}

static inline bool is_mt_fence(struct dma_fence *fence)
{
	return ((fence->ops == &mt_fence_ops) ||
		(fence->ops == &mt_fence_foreign_ops));
}

static inline struct mt_fence *to_mt_fence(struct dma_fence *fence)
{
	if (is_mt_fence(fence))
		return container_of(fence, struct mt_fence, base);

	return NULL;
}

MTGPU_ERROR mt_fence_context_register_dbg(void *dbg_request_handle,
				void *dev,
				struct mt_fence_context *fctx);
struct mt_fence_context *
mt_fence_foreign_context_create(struct workqueue_struct *fence_status_wq,
		const char *name);
struct mt_fence_context *
mt_fence_context_create(void *dev_cookie,
			 struct workqueue_struct *fence_status_wq,
			 const char *name);
void mt_fence_context_destroy(struct mt_fence_context *fctx);
void mt_context_value_str(struct mt_fence_context *fctx, char *str, int size);

struct mt_fence *
mt_fence_create(struct mt_fence_context *fctx,
		 struct SYNC_CHECKPOINT_CONTEXT_TAG *sync_checkpoint_ctx,
		 int timeline_fd, const char *name);
struct mt_fence *
mt_fence_create_from_fence(struct mt_fence_context *fctx,
			    struct SYNC_CHECKPOINT_CONTEXT_TAG *sync_checkpoint_ctx,
			    struct dma_fence *fence,
			    MTGPU_FENCE fence_fd,
			    const char *name);
void mt_fence_destroy(struct mt_fence *mt_fence);
int mt_fence_sw_signal(struct mt_fence *mt_fence);
int mt_fence_sw_error(struct mt_fence *mt_fence);

int mt_fence_get_checkpoints(struct mt_fence **mt_fences, u32 nr_fences,
			      struct SYNC_CHECKPOINT_TAG **fence_checkpoints);
struct SYNC_CHECKPOINT_TAG *
mt_fence_get_checkpoint(struct mt_fence *update_fence);

void mt_fence_context_signal_fences_nohw(void *data);

void mt_fence_context_free_deferred_callback(void *data);

u32 mt_fence_dump_info_on_stalled_ufos(struct mt_fence_context *fctx,
					u32 nr_ufos,
					u32 *vaddrs);

#if defined(MT_FENCE_DEBUG)
#define MT_FENCE_CTX_TRACE(c, fmt, ...)                                   \
	do {                                                               \
		struct mt_fence_context *__fctx = (c);                    \
		pr_err("c %llu: (MT) " fmt, (u64) __fctx->fence_context,  \
		       ## __VA_ARGS__);                                    \
	} while (0)
#else
#define MT_FENCE_CTX_TRACE(c, fmt, ...)
#endif

#define MT_FENCE_CTX_WARN(c, fmt, ...)                                    \
	do {                                                               \
		struct mt_fence_context *__fctx = (c);                    \
		pr_warn("c %llu: (MT) " fmt, (u64) __fctx->fence_context, \
			## __VA_ARGS__);                                   \
	} while (0)

#define MT_FENCE_CTX_ERR(c, fmt, ...)                                     \
	do {                                                               \
		struct mt_fence_context *__fctx = (c);                    \
		pr_err("c %llu: (MT) " fmt, (u64) __fctx->fence_context,  \
		       ## __VA_ARGS__);                                    \
	} while (0)

#if defined(MT_FENCE_DEBUG)
#define MT_FENCE_TRACE(f, fmt, ...)                                       \
	DMA_FENCE_ERR(f, "(MT) " fmt, ## __VA_ARGS__)
#else
#define MT_FENCE_TRACE(f, fmt, ...)
#endif

#define MT_FENCE_WARN(f, fmt, ...)                                        \
	DMA_FENCE_WARN(f, "(MT) " fmt, ## __VA_ARGS__)

#define MT_FENCE_ERR(f, fmt, ...)                                         \
	DMA_FENCE_ERR(f, "(MT) " fmt, ## __VA_ARGS__)

#endif /* !defined(__MT_FENCE_H__) */

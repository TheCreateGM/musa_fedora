/*
 * @Copyright Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License Dual MIT/GPLv2
 */

#ifndef _MTGPU_VM_H_
#define _MTGPU_VM_H_

#include "mtgpu_handle.h"

struct drm_device;
struct drm_file;
struct DEVMEM_HEAP_TAG;
struct _DEVMEMINT_MAPPING_;
struct mutex;
struct mtgpu_virt_mem;
struct devmem_virt_heap;

#define DEVMEM_HEAPNAME_MAXLENGTH 160
#define TASK_COMM_LEN 16

typedef enum {
	VMCTX_STATE_OK = 0,
	VMCTX_STATE_TO_RELEASE,
	VMCTX_STATE_FORCE_CLEANUP,
} VMCTX_STATE;

struct mtgpu_vm_bo {
	struct mtgpu_bo *bo;
	uint64_t dev_vaddr;
	void *cpu_vaddr;
	void *map_data;

	struct mtgpu_virt_mem *virt_mem;
};

struct mtgpu_vm_context {
	/* WARNING: base must be the first member.
	 * The context will be automatically released when its reference count drops to zero. */
	struct mtgpu_object base;
	struct mtgpu_device_node *dev_node;
	struct _DEVMEMINT_CTX_ *devmem_int_ctx;
	struct devmem_virt_heap *reserved_int_heap;
	struct devmem_virt_heap *reserved_cbuf_heap;
	struct devmem_virt_heap **app_int_heaps;
	u32 app_heap_count;
	struct rb_root_cached *root;
	struct mutex *tree_lock;	/* Mutex lock for tree access */
	u32 reserved_heap_count;
	struct DEVMEM_HEAP_TAG **reserved_heap_array;
	struct mtgpu_vm_bo *yuv_csc_buffer;
	struct mtgpu_vm_bo *pb_bo;
	struct mtgpu_vm_bo *pmva_bo;
	struct mtgpu_vm_bo *dm_kill_bo;

	struct mtgpu_job_list *job_list;
	struct mutex *job_ctx_list_lock;
	struct list_head job_ctx_list;
	struct list_head node;

	/* used to optimise fw cfi */
	u64 page_table_uid;

	u32 vm_ctx_state;
	u32 vm_ctx_flag;
	struct mutex *state_lock;

	struct work_struct *release_work;

	char comm[TASK_COMM_LEN];
	pid_t pid;

	/* used for async mmu invalid and vm map */
	struct workqueue_struct *async_map_workq;

	uint64_t rm_client;
	uint64_t rm_vm_context;
};

struct mtgpu_vm_map_async_work_data {
	struct drm_device *drm;
	struct mtgpu_vm_context *vm_ctx;
	struct drm_gem_object *obj;

	u64 bo_mapping_flags;
	u64 gpu_va;
	u64 size;
	u32 log2_page_size;
	u32 heap_index;

	/* async mmu invalid update sema */
	struct mtgpu_syncobj **update_syncobjs;
	u64 *update_values;
	u32 update_semaphore_count;

	/* rm path*/
	u64 *rm_update_syncobjs;
};

int mtgpu_vm_ctx_get(struct mtgpu_vm_context *vm_ctx);
int mtgpu_vm_ctx_put(struct mtgpu_vm_context *vm_ctx);
void *mtgpu_vm_context_private_data(struct mtgpu_vm_context *vm_ctx);
bool mtgpu_vm_ctx_is_idle(struct mtgpu_vm_context *vm_ctx);
int mtgpu_vm_ctx_force_cleanup(struct mtgpu_vm_context *vm_ctx);

s64 atomic64_inc_mtgpu_page_table_uid(void);

#endif /* _MTGPU_VM_H_ */

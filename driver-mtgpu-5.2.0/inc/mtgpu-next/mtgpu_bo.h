/*
 * @Copyright Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License Dual MIT/GPLv2
 */

#ifndef _MTGPU_BO_H_
#define _MTGPU_BO_H_

struct drm_device;
typedef struct _CONNECTION_DATA_ CONNECTION_DATA;
typedef struct _PHYS_HEAP_ PHYS_HEAP;

#define MTGPU_METADATA_MAX_NUM 32
#define MTGPU_METADATA_MAX_SIZE (4 * 1024 * 1024)

struct mtgpu_bo {
	struct mtgpu_device_node *dev_node;
	CONNECTION_DATA *conn;
	u64 size;
	u32 align;
	u64 mt_flags;
	bool is_sysmem;
	bool is_user;
	bool is_contiguous;
	bool is_cpu_invisible;
	u64 key;
	PHYS_HEAP *heap;

	struct mtgpu_bo_metadata_list *metadata_list;

	bool is_rm;

	/* rm */
	u64 rm_client;
	u64 rm_mem;
	u64 pid;
	atomic_t refcnt;
	struct mutex *lock;

	bool is_gathered;
};

struct mtgpu_bo_metadata {
	u64 id;
	u64 size;
	void *ptr;
	struct list_head node;
};

struct mtgpu_bo_metadata_list {
	struct list_head head;
	struct spinlock *lock;
	u32 total_num;
};

enum mtgpu_address_translation {
	MTGPU_ADDR_TRANSLATION_CPU_PA,
	MTGPU_ADDR_TRANSLATION_GPU_PA,
	MTGPU_ADDR_TRANSLATION_UNIFIED_PA,
};

u64 mtgpu_bo_flag_from_mt_flag(u64 mt_flags);
u64 mtgpu_bo_flag_to_mt_flag(u64 bo_flags);
u32 mtgpu_bo_domain_from_mt_flag(u64 mt_flags);

int mtgpu_bo_alloc_handle(void *ptr, u64 *handle_out);
int mtgpu_bo_find_handle(u32 global_handle, void **ptr_out);
void mtgpu_bo_delete_global_handle(u64 global_handle);

bool mtgpu_bo_has_metadata(struct device *dev, struct mtgpu_bo *bo);
int mtgpu_bo_get_metadata(struct device *dev, struct mtgpu_bo *bo, void __user *metadata_addr,
			  u64 metadata_size, u64 metadata_id);
int mtgpu_bo_add_metadata(struct mtgpu_bo *bo, void __user *metadata_addr, u64 metadata_size,
			  u64 metadata_id);
int mtgpu_bo_metadata_list_create(struct device *dev, struct mtgpu_bo *bo);
void mtgpu_bo_metadata_list_destroy(struct mtgpu_bo_metadata_list *metadata_list);

int mtgpu_bo_alloc(struct mtgpu_device_node *dev_node, CONNECTION_DATA *connection, u64 size,
		   u32 align, u64 flags, const char *annotation, void **handle, u64 *dev_addr);

int mtgpu_bo_alloc_vram(struct drm_device *drm, int segment_id, u64 size, u64 *dev_addr,
			void **handle);

int mtgpu_bo_alloc_sysmem(struct drm_device *drm, u64 size, void **handle);

int mtgpu_bo_alloc_user(struct drm_device *drm, CONNECTION_DATA *connection, u64 size, u64 flags,
			void *user_ptr, const char *annotation, void **handle);

int mtgpu_bo_alloc_fw_mem(struct mtgpu_device_node *dev_node, u64 size, u64 align, bool is_sysmem,
			  u64 *cpu_addr, u64 *gpu_vaddr, u64 *fw_addr, struct mtgpu_bo **p_bo);

int mtgpu_bo_import_ext_dmabuf(struct drm_device *drm, struct dma_buf_attachment *attach, u64 size,
			       u64 flags, void **handle);

void mtgpu_bo_free(void *handle);

int mtgpu_bo_acquire(void *handle);

void mtgpu_bo_release(void *handle);

int mtgpu_bo_get_phys_addr(void *handle, u64 offset, u32 page_shift, u64 *phys_addrs,
			   u32 addr_translation);

int mtgpu_bo_get_phys_addrs(void *handle, u64 offset, u32 page_shift, u64 *phys_addrs, u64 count,
			    u32 addr_translation);

int mtgpu_bo_vmap(void *handle, u64 offset, size_t size, void **private_data, void **kaddr);

void mtgpu_bo_vunmap(void *handle, void *private_data);

/* deprecated interface for old path */

static inline int mtgpu_system_alloc(struct drm_device *drm, size_t size, void **handle)
{
	return mtgpu_bo_alloc_sysmem(drm, size, handle);
}

static inline int mtgpu_vram_alloc(struct drm_device *drm, int segment_id, size_t size,
				   dma_addr_t *dev_addr, void **handle)
{
	return mtgpu_bo_alloc_vram(drm, segment_id, size, dev_addr, handle);
}

static inline void mtgpu_vram_free(void *handle)
{
	mtgpu_bo_free(handle);
}

static inline int mtgpu_vram_vmap(void *handle, size_t size, u64 *private_data, void **kaddr)
{
	return mtgpu_bo_vmap(handle, 0, size, (void **)private_data, kaddr);
}

static inline void mtgpu_vram_vunmap(void *handle, u64 private_data)
{
	return mtgpu_bo_vunmap(handle, (void *)private_data);
}

#endif /* _MTGPU_BO_H_ */

/*
 * @Copyright   Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License     Dual MIT/GPLv2
 */

#ifndef _MTVPU_GEM_H_
#define _MTVPU_GEM_H_

#include "mtvpu_drv.h"

enum vram_allocater {
	LINUX_NATIVE,
	WIN_GUEST,
	LINUX_GUEST,
	LINUX_HOST,
};

int vpu_vram_alloc(struct drm_device *drm, u32 group_id, u32 pool_id, u32 type, u64 size,
		   struct mtgpu_gem_object *mtgpu_obj);
void vpu_vram_free(struct mtgpu_gem_object *mtgpu_obj);
struct sg_table *vpu_gem_map_guest_pages(struct device *dev, u32 vm_id, u32 *gpfn_array, u32 num_pages);
void vpu_gem_unmap_internal(struct sg_table *sgt);
struct mt_node *gem_malloc_node(struct mt_chip *chip, int idx,
					 struct drm_device *drm, u64 size, u32 type);
void gem_free_node(struct mt_chip *chip, struct mt_node *node);
int vpu_buffer_init(struct drm_device *drm, void *data, struct drm_file *file);
int vpu_buffer_reset(struct drm_device *drm, void *data, struct drm_file *file);
int vpu_buffer_modify(struct drm_device *drm, void *data, struct drm_file *file);
int vpu_gem_init(struct mt_chip *chip, struct mtgpu_gem_object *mtgpu_obj, u32 flags);
int vpu_gem_reset(struct mt_chip *chip, struct mtgpu_gem_object *mtgpu_obj);
int vpu_gem_modify(struct mt_chip *chip, struct mtgpu_gem_object *mtgpu_obj, u32 new_id);

#ifdef SUPPORT_ION
u64 get_dma_buf_dev_addr(struct dma_buf *dmabuf);
int get_dma_buf_vaddr(struct dma_buf *dmabuf, void **vaddr);
int vpu_dma_buf_init(struct mt_chip *chip, struct dma_buf *dmabuf, u32 flags);
int vpu_dma_buf_reset(struct mt_chip *chip, struct dma_buf *dmabuf);
int vpu_dma_buf_modify(struct mt_chip *chip, struct dma_buf *dmabuf, u32 new_id);
#endif
#endif /* _MTVPU_GEM_H_ */

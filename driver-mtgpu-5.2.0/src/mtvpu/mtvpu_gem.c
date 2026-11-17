/*
 * @Copyright   Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License     Dual MIT/GPLv2
 */
#include <linux/version.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/io.h>
#include <drm/drm_file.h>
#include <drm/drm_gem.h>
#if defined(OS_DRM_DRMP_H_EXIST)
#include <drm/drmP.h>
#else
#include <drm/drm_device.h>
#include <drm/drm_ioctl.h>
#endif

#ifdef SUPPORT_ION
#include "ion/ion.h"
#endif

#include "mtgpu_drv.h"
#include "mtgpu_drm_gem.h"
#include "mtgpu_drm_drv.h"
#include "mtgpu_drm_internal.h"
#include "mtgpu_module_param.h"
#include "mtgpu_mdev.h"
#include "os-interface-drm.h"

#include "mtvpu_drv.h"
#include "mtvpu_gem.h"
#include "mtvpu_smmu.h"
#include "mtvpu_mem.h"
#include "mtvpu_cmd.h"
#include "mtvpu_api.h"
#include "vpuapifunc.h"
#include "misc.h"
#include "helper.h"

#ifdef SUPPORT_ION
u64 get_dma_buf_dev_addr(struct dma_buf *dmabuf)
{
	struct ion_buffer *buffer;
	struct sg_table *table;
	struct mtvpu_gem_priv *gem_priv;
	u64 addr = 0;

	if (!dmabuf || !dmabuf->priv)
		return 0;

	buffer = dmabuf->priv;

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 8, 0))
	table = buffer->priv_virt;
#else
	table = buffer->sg_table;
#endif

	if (!buffer->heap)
		return 0;

	if (buffer->heap->type == ION_HEAP_TYPE_SYSTEM) {
		if (buffer->priv) {
			gem_priv = (struct mtvpu_gem_priv *)buffer->priv;
			addr = gem_priv->dev_phy_addr;
		}
	} else {
		addr = sg_dma_address(table->sgl);
	}
	// TODO: check if a valid addr, or bo_wait may fail
	return addr;
}

int get_dma_buf_vaddr(struct dma_buf *dmabuf, void **vaddr)
{
	struct ion_buffer *buffer;
	struct mtvpu_gem_priv *gem_priv;
	int ret = 0;

	if (!dmabuf || !vaddr || !dmabuf->priv)
		return -EINVAL;

	buffer = (struct ion_buffer *)dmabuf->priv;

	if (!buffer->heap)
		return -EINVAL;

	if (buffer->heap->type != ION_HEAP_TYPE_SYSTEM) {
		vpu_err("%s, unsupported heap type %d\n", __func__, buffer->heap->type);
		return -EINVAL;
	}

	if (dmabuf->vmapping_counter > 0) {
#if defined(OS_STRUCT_DMA_BUF_MAP_EXIST) || defined(OS_STRUCT_IOSYS_MAP_EXIST)
		*vaddr = dmabuf->vmap_ptr.vaddr;
#else
		*vaddr = dmabuf->vmap_ptr;
#endif
		return 0;
	}

	gem_priv = (struct mtvpu_gem_priv *)buffer->priv;
	if (!gem_priv)
		return -EINVAL;

#if defined(OS_STRUCT_DMA_BUF_MAP_EXIST) || defined(OS_STRUCT_IOSYS_MAP_EXIST)
	OS_BUG_ON(gem_priv->map);

	gem_priv->map = kzalloc(sizeof(struct iosys_map), GFP_KERNEL);
	if (!gem_priv->map)
		return -ENOMEM;

	ret = dma_buf_vmap(dmabuf, gem_priv->map);
	if (ret) {
		vpu_err("%s, failed, %d\n", __func__, ret);
		kfree(gem_priv->map);
		gem_priv->map = NULL;
		return ret;
	}
	*vaddr = gem_priv->map->vaddr;
#else
	gem_priv->virt_addr = dma_buf_vmap(dmabuf);
	if (!gem_priv->virt_addr) {
		vpu_err("%s, failed, %d\n", __func__, ret);
		*vaddr = NULL;
		return -EINVAL;
	}
	*vaddr = gem_priv->virt_addr;
#endif

	return ret;
}

int vpu_dma_buf_init(struct mt_chip *chip, struct dma_buf *dmabuf, u32 flags)
{
	struct ion_buffer *buffer;
	struct sg_table *table;
	struct mtvpu_gem_priv *gem_priv;
	int ret = 0;

	if (!chip || !dmabuf || !dmabuf->priv)
		return -EINVAL;

	buffer = (struct ion_buffer *)dmabuf->priv;

	if (!buffer->heap)
		return -EINVAL;

	if (buffer->heap->type != ION_HEAP_TYPE_SYSTEM)
		return -EINVAL;

	if (buffer->priv) {
		vpu_err("ion buffer priv has been used!\n");
		return -EINVAL;
	}

	gem_priv = kzalloc(sizeof(struct mtvpu_gem_priv), GFP_KERNEL);
	if (!gem_priv)
		return -ENOMEM;

	buffer->priv = gem_priv;
	gem_priv->flags = flags;

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 8, 0))
	table = buffer->priv_virt;
#else
	table = buffer->sg_table;
#endif

	ret = vpu_smmu_map_sg(chip, table, buffer->size, flags & 0xff, (dma_addr_t *)&gem_priv->dev_phy_addr);
	if (ret) {
		vpu_err("vpu smmu map failed!\n");
		kfree(gem_priv);
		buffer->priv = NULL;
		return ret;
	}

	return ret;
}

int vpu_dma_buf_reset(struct mt_chip *chip, struct dma_buf *dmabuf)
{
	struct ion_buffer *buffer;
	struct mtvpu_gem_priv *gem_priv;

	if (!chip || !dmabuf || !dmabuf->priv)
		return -EINVAL;

	buffer = dmabuf->priv;

	if (!buffer->heap)
		return -EINVAL;

	if (buffer->heap->type != ION_HEAP_TYPE_SYSTEM)
		return -EINVAL;

	gem_priv = (struct mtvpu_gem_priv *)buffer->priv;
	if (!gem_priv)
		return -EINVAL;

	vpu_smmu_unmap(chip, (dma_addr_t)gem_priv->dev_phy_addr);

#if defined(OS_STRUCT_DMA_BUF_MAP_EXIST) || defined(OS_STRUCT_IOSYS_MAP_EXIST)
	if (gem_priv->map) {
		vpu_info("vunmap dmabuf %llx, is_iomem %d\n", (u64)gem_priv->map->vaddr, gem_priv->map->is_iomem);
		dma_buf_vunmap(dmabuf, gem_priv->map);
		kfree(gem_priv->map);
	}
#else
	if (gem_priv->virt_addr)
		dma_buf_vunmap(dmabuf, gem_priv->virt_addr);
#endif
	kfree(gem_priv);
	buffer->priv = NULL;
	return 0;
}

int vpu_dma_buf_modify(struct mt_chip *chip, struct dma_buf *dmabuf, u32 new_id)
{
	struct ion_buffer *buffer;
	struct sg_table *table;
	struct mtvpu_gem_priv *gem_priv;
	u32 old_id;
	int ret = 0;

	if (!chip || !chip->soc_mode || !dmabuf || !dmabuf->priv)
		return -EINVAL;

	buffer = (struct ion_buffer *)dmabuf->priv;

	if (!buffer->heap)
		return -EINVAL;

	if (buffer->heap->type != ION_HEAP_TYPE_SYSTEM)
		return -EINVAL;

	OS_BUG_ON(!chip->io_domain);

	gem_priv = buffer->priv;
	if (!gem_priv)
		return -EINVAL;

	old_id = gem_priv->flags & 0xff;
	if (old_id == new_id)
		return 0;

	vpu_smmu_unmap(chip, (dma_addr_t)gem_priv->dev_phy_addr);

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 8, 0))
	table = buffer->priv_virt;
#else
	table = buffer->sg_table;
#endif

	ret = vpu_smmu_map_sg(chip, table, buffer->size, new_id, (dma_addr_t *)&gem_priv->dev_phy_addr);
	if (ret) {
		vpu_err("%s, vpu smmu map failed!\n", __func__);
		return ret;
	}

	gem_priv->flags &= 0xffffff00;
	gem_priv->flags |= new_id;

	return ret;
}
#endif

static u64 vdi_guest_mem_usage = 0;

KUNIT_STATIC struct sg_table *vpu_gem_map_internal(void *handle, size_t size)
{
	struct sg_table *sgt;
	struct scatterlist *sgl;
	struct page *cpu_page;
	IMG_CPU_PHYADDR *cpu_pa;
	MT_BOOL *valid;
	u32 num_pages;
	u64 sg_idx;
	int ret;

	ret = os_sg_table_create(&sgt);
	if (ret)
		return NULL;

	num_pages = size / OS_VAL(PAGE_SIZE);
	valid = os_kvzalloc(sizeof(*valid) * num_pages);
	if (!valid)
		goto err_free_sgt;

	cpu_pa = os_kvzalloc(sizeof(*cpu_pa) * num_pages);
	if (!cpu_pa)
		goto err_free_valid;

	ret = PMR_CpuPhysAddr(handle, OS_VAL(PAGE_SHIFT), num_pages, 0, cpu_pa, valid);
	if (ret != MTGPU_OK)
		goto err_free_pa;

	if (os_sg_alloc_table(sgt, num_pages)) {
		vpu_err("%s(): sgtable alloc multi sglist failed\n", __func__);
		goto err_free_pa;
	}
	os_for_each_sg(OS_SG_TABLE_MEMBER(sgt, sgl), sgl,
		       OS_SG_TABLE_MEMBER(sgt, nents), sg_idx) {
		cpu_page = os_phys_to_page(cpu_pa[sg_idx]);
		os_set_sg_page(sgl, cpu_page, OS_VAL(PAGE_SIZE), 0);
		os_set_sg_dma_address(sgl, cpu_pa[sg_idx]);
		os_set_sg_dma_len(sgl, OS_VAL(PAGE_SIZE));
	}
	os_kvfree(cpu_pa);
	os_kvfree(valid);

	return sgt;

err_free_pa:
	os_kvfree(cpu_pa);
err_free_valid:
	os_kvfree(valid);
err_free_sgt:
	os_sg_table_destroy(sgt);
	return NULL;
}

KUNIT_STATIC int mtvpu_vram_alloc(struct drm_device *drm, u32 group_id, size_t size,
		     dma_addr_t *dev_addr, void **handle)
{
	int ret;
	struct mt_chip *chip = to_chip(drm);
	struct sg_table *sgt;

	if (!chip->io_domain) {
		OS_DRM_ERROR("%s(): reserved memory must be used if smmu not enabled! \n", __func__);
		return -OS_VAL(ENOMEM);
	}
	ret = mtgpu_system_alloc(drm, size, handle);
	if (ret) {
		OS_DRM_ERROR("%s(): mtgpu_system_alloc() failed\n", __func__);
		return -OS_VAL(ENOMEM);
	}
	sgt = vpu_gem_map_internal(*handle, size);
	ret = vpu_smmu_map_sg(chip, sgt, size, group_id, dev_addr);
	vpu_gem_unmap_internal(sgt);
	if (ret) {
		vpu_err("vpu smmu map failed!");
		return ret;
	}
	return ret;
}

/* only for ioctl */
int vpu_vram_alloc(struct drm_device *drm, u32 group_id, u32 pool_id, u32 type, u64 size,
		   struct mtgpu_gem_object *mtgpu_obj)
{
	struct mt_chip *chip = to_chip(drm);
	int ret = 0;

	if (chip->soc_mode)
		ret = mtvpu_vram_alloc(drm, group_id, size, &mtgpu_obj->dev_addr, &mtgpu_obj->handle);
	else
		ret = mtgpu_vram_alloc(drm, group_id, size, &mtgpu_obj->dev_addr, &mtgpu_obj->handle);
	if (!ret) {
		vdi_guest_mem_usage += size;
		VLOG(MTDEBUG, "guest alloc vram succ, pid:%4d, group:%d, type:%d, addr:0x%08llx,"
			"size:0x%08llx, vdi_guest_mem_usage:0x%08llx\n", current->pid, group_id,
			type, mtgpu_obj->dev_addr, size, vdi_guest_mem_usage += size);
	} else {
		VLOG(MTDEBUG, "guest alloc vram fail, pid:%4d, group:%d, type:%d, addr:0x%08llx,"
			"size:0x%08llx, vdi_guest_mem_usage:0x%08llx\n", current->pid, group_id,
			type, mtgpu_obj->dev_addr, size, vdi_guest_mem_usage);
	}
	mtgpu_obj->type = MTGEM_TYPE_VPU_START + (group_id << 8);
	mtgpu_obj->cpu_addr = chip->bar_base + mtgpu_obj->dev_addr;

	return ret;
}

void vpu_vram_free(struct mtgpu_gem_object *mtgpu_obj)
{
	struct mt_chip *chip;
	struct mtvpu_gem_priv *priv;

	if (!(mtgpu_obj && mtgpu_obj->obj))
		return;

	chip = to_chip(mtgpu_obj->obj->dev);
	priv = (struct mtvpu_gem_priv *)mtgpu_obj->private_data;

	if (mtgpu_obj->handle) {
		if (chip->soc_mode && chip->io_domain) {
			vpu_smmu_unmap(chip, mtgpu_obj->dev_addr);
			if (priv && priv->priv_data)
				mtgpu_vram_vunmap(mtgpu_obj->handle, priv->priv_data);
		}
		mtgpu_bo_free(mtgpu_obj->handle);
	}

	if (os_get_drm_gem_object_import_attach(mtgpu_obj->obj)) {
		os_drm_prime_gem_destroy(mtgpu_obj->obj, mtgpu_obj->sgt);
		mtgpu_obj->sgt = NULL;
	}

	if (mtgpu_obj->sgt) {
		os_sg_free_table(mtgpu_obj->sgt);
		os_kfree(mtgpu_obj->sgt);
		mtgpu_obj->sgt = NULL;
	}

	if (!chip->soc_mode) {
		vdi_guest_mem_usage -= mtgpu_obj->obj->size;
		VLOG(MTDEBUG, "guest free vram, pid:%4d, addr:0x%08llx, size:0x%08llx,"
		     "vdi_guest_mem_usage:0x%08llx\n", current->pid, mtgpu_obj->dev_addr,
		     mtgpu_obj->obj->size, vdi_guest_mem_usage -= mtgpu_obj->obj->size);
	}

	os_drm_gem_object_release(mtgpu_obj->obj);
	os_kfree(mtgpu_obj->obj);
	mtgpu_obj->obj = NULL;

	if (priv) {
		os_kfree(priv);
		priv = NULL;
		mtgpu_obj->private_data = (u64)NULL;
	}
	os_kfree(mtgpu_obj);
}

void mtvpu_gem_free_obj(struct drm_gem_object *obj)
{
	struct mtgpu_gem_object *mtgpu_obj = os_get_drm_gem_object_drvdata(obj);

	vpu_vram_free(mtgpu_obj);
}

struct sg_table *vpu_gem_map_guest_pages(struct device *dev, u32 vm_id, u32 *gpfn_array, u32 num_pages)
{
	struct sg_table *sgt;
	struct scatterlist *sgl;
	struct page *cpu_page;
	struct page **pages;
	u64 sg_idx, hpa;
	phys_addr_t pa;
	int i, ret;

	ret = os_sg_table_create(&sgt);
	if (ret)
		return NULL;

	pages = os_kvzalloc(num_pages * sizeof(struct page *));
	if (!pages)
		goto err_free_sgt;

	for (i = 0; i < num_pages; i++) {
		ret = mtgpu_mdev_gpa_to_hpa(dev, vm_id, (u64)gpfn_array[i] << PAGE_SHIFT, &hpa);
		if (ret)
			goto err_free_pages;
		pages[i] = os_phys_to_page(hpa);
	}

	if (os_sg_alloc_table(sgt, num_pages)) {
		vpu_err("%s(): sgtable alloc multi sglist failed\n", __func__);
		goto err_free_pages;
	}

	os_for_each_sg(OS_SG_TABLE_MEMBER(sgt, sgl), sgl,
		       OS_SG_TABLE_MEMBER(sgt, nents), sg_idx) {
		cpu_page = pages[sg_idx];
		pa = os_page_to_phys(cpu_page);
		os_set_sg_page(sgl, cpu_page, OS_VAL(PAGE_SIZE), 0);
		os_set_sg_dma_address(sgl, pa);
		os_set_sg_dma_len(sgl, OS_VAL(PAGE_SIZE));
	}
	os_kvfree(pages);

	return sgt;

err_free_pages:
	os_kvfree(pages);
err_free_sgt:
	os_sg_table_destroy(sgt);
	return NULL;
}

void vpu_gem_unmap_internal(struct sg_table *sgt)
{
	os_sg_free_table(sgt);
	os_kfree(sgt);
}

int vpu_gem_init(struct mt_chip *chip, struct mtgpu_gem_object *mtgpu_obj, u32 flags)
{
	struct mtvpu_gem_priv *priv = NULL;
	struct sg_table *sgt;
	int ret;

	if (!(chip->soc_mode && chip->io_domain))
		return 0;

	if (mtgpu_obj->private_data) {
		vpu_err("mtgpu obj private data has been used!\n");
		return -1;
	}
	priv = os_kzalloc(sizeof(struct mtvpu_gem_priv));
	if (!priv)
		return -OS_VAL(ENOMEM);

	priv->flags = flags;

	sgt = vpu_gem_map_internal(mtgpu_obj->handle, mtgpu_obj->obj->size);
	if (OS_IS_ERR_OR_NULL(sgt)) {
		os_kfree(priv);
		return -1;
	}
	ret = vpu_smmu_map_sg(chip, sgt, mtgpu_obj->obj->size, flags & 0xff, &mtgpu_obj->dev_addr);
	vpu_gem_unmap_internal(sgt);
	if (ret) {
		vpu_err("vpu smmu map failed!");
		os_kfree(priv);
		return ret;
	}
	mtgpu_obj->cpu_addr = mtgpu_obj->dev_addr;
	mtgpu_obj->private_data = (u64)priv;
	/*
	mark it as vpu memory; such that vpu_vram_free is called
	valid only in soc mode.
	*/
	mtgpu_obj->type = MTGEM_TYPE_VPU_START;

	return 0;
}

int vpu_gem_reset(struct mt_chip *chip, struct mtgpu_gem_object *mtgpu_obj)
{
	struct mtvpu_gem_priv *priv = (struct mtvpu_gem_priv *)mtgpu_obj->private_data;

	if (!(chip->soc_mode && chip->io_domain))
		return 0;

	vpu_smmu_unmap(chip, mtgpu_obj->dev_addr);
	mtgpu_obj->dev_addr = 0;

	if (priv && priv->priv_data)
		mtgpu_vram_vunmap(mtgpu_obj->handle, priv->priv_data);

	if (priv)
		os_kfree(priv);

	mtgpu_obj->private_data = 0;

	return 0;
}

int vpu_gem_modify(struct mt_chip *chip, struct mtgpu_gem_object *mtgpu_obj, u32 new_id)
{
	struct sg_table *sgt;
	struct mtvpu_gem_priv *priv;
	u32 old_id;
	int ret, size;

	if (!chip || !chip->soc_mode || !mtgpu_obj)
		return -1;

	priv = (struct mtvpu_gem_priv *)mtgpu_obj->private_data;
	if (!priv)
		return -OS_VAL(EINVAL);

	old_id = priv->flags & 0xff;
	if (old_id == new_id)
		return 0;

	if (chip->io_domain)
		vpu_smmu_unmap(chip, mtgpu_obj->dev_addr);
	else
		return -1;

	mtgpu_obj->dev_addr = 0;
	size = os_get_drm_gem_object_size(mtgpu_obj->obj);
	sgt = vpu_gem_map_internal(mtgpu_obj->handle, size);
	ret = vpu_smmu_map_sg(chip, sgt, size, new_id, &mtgpu_obj->dev_addr);
	vpu_gem_unmap_internal(sgt);

	if (ret) {
		vpu_err("%s, vpu smmu map failed!\n", __func__);
		return ret;
	}

	priv->flags &= 0xffffff00;
	priv->flags |= new_id;
	mtgpu_obj->cpu_addr = chip->bar_base + mtgpu_obj->dev_addr;

	return ret;
}

int vpu_buffer_init(struct drm_device *drm, void *data, struct drm_file *file)
{
	struct mtvpu_buffer_operate *args = data;
	struct mtgpu_gem_object *mtgpu_obj;
	struct drm_gem_object *obj;
	struct mt_chip *chip = to_chip(drm);
	int ret = 0;

	obj = os_drm_gem_object_lookup(file, args->bo_handle);
	if (!obj)
		return -OS_VAL(EINVAL);

	mtgpu_obj = os_get_drm_gem_object_drvdata(obj);
	os_drm_gem_object_put(obj);

	ret = vpu_gem_init(chip, mtgpu_obj, args->flags.u32_val);

	return ret;
}

int vpu_buffer_reset(struct drm_device *drm, void *data, struct drm_file *file)
{
	struct mtvpu_buffer_operate *args = data;
	struct mtgpu_gem_object *mtgpu_obj;
	struct drm_gem_object *obj;
	struct mt_chip *chip = to_chip(drm);
	int ret = 0;

	obj = os_drm_gem_object_lookup(file, args->bo_handle);
	if (!obj)
		return -OS_VAL(EINVAL);

	mtgpu_obj = os_get_drm_gem_object_drvdata(obj);
	os_drm_gem_object_put(obj);

	ret = vpu_gem_reset(chip, mtgpu_obj);

	return ret;
}

int vpu_buffer_modify(struct drm_device *drm, void *data, struct drm_file *file)
{
	struct mtvpu_buffer_operate *args = data;
	struct mtgpu_gem_object *mtgpu_obj;
	struct drm_gem_object *obj;
	struct mt_chip *chip = to_chip(drm);
	int ret = 0;
	int new_group_id = args->flags.mem_grp_id;

	obj = os_drm_gem_object_lookup(file, args->bo_handle);
	if (!obj)
		return -OS_VAL(EINVAL);

	mtgpu_obj = os_get_drm_gem_object_drvdata(obj);
	os_drm_gem_object_put(obj);

	ret = vpu_gem_modify(chip, mtgpu_obj, new_group_id);

	return ret;
}

struct mt_node *gem_malloc_node(struct mt_chip *chip, int idx,
				struct drm_device *drm, u64 size, u32 type)
{
	struct mt_core *core = &chip->core[idx];
	struct mt_node *node;
	u32 max_page_size;
	int ret = 0;

	/* only for fw buffer; fix this if needed */
	if (type != FW_BUFFER)
		return NULL;

	max_page_size = gpu_page_size < OS_VAL(PAGE_SIZE) ? OS_VAL(PAGE_SIZE) : gpu_page_size;
	size = ALIGN(size, max_page_size);

	node = os_kzalloc(sizeof(*node));
	if (!node)
		return NULL;

	node->obj = os_kzalloc(sizeof(struct drm_gem_object));
	if (!node->obj) {
		os_kfree(node);
		return NULL;
	}

	if (chip->soc_mode) {
		ret = mtvpu_vram_alloc(drm, core->mem_group_id, size, &node->local_phys_addr, &node->handle);
		if(core->product_id == CORTEXA35_CODE) {
#define	MT_PH_JPU_MCU_FW_PROGRAM_LOAD_ADDR	0xF0000000
#define	MT_PH_JPU_MCU_FW_PROGRAM_LOAD_SIZE	0x00800000
			void* handle1;
			void* handle2;
			u32 program_size;
			program_size = (size >= MT_PH_JPU_MCU_FW_PROGRAM_LOAD_SIZE ? size : MT_PH_JPU_MCU_FW_PROGRAM_LOAD_SIZE);
			if(node->local_phys_addr < MT_PH_JPU_MCU_FW_PROGRAM_LOAD_ADDR && node->local_phys_addr >= (MT_PH_JPU_MCU_FW_PROGRAM_LOAD_ADDR - program_size)) {
				handle1 = node->handle;
				ret = mtvpu_vram_alloc(drm, core->mem_group_id, size, &node->local_phys_addr, &node->handle);
				if(node->local_phys_addr > MT_PH_JPU_MCU_FW_PROGRAM_LOAD_ADDR && node->local_phys_addr <= (MT_PH_JPU_MCU_FW_PROGRAM_LOAD_ADDR + program_size)) {
					handle2 = node->handle;
					ret = mtvpu_vram_alloc(drm, core->mem_group_id, size, &node->local_phys_addr, &node->handle);
					mtgpu_vram_free(handle2);
				}
				mtgpu_vram_free(handle1);
			} else if(node->local_phys_addr > MT_PH_JPU_MCU_FW_PROGRAM_LOAD_ADDR && node->local_phys_addr <= (MT_PH_JPU_MCU_FW_PROGRAM_LOAD_ADDR + program_size)) {
			    handle1 = node->handle;
				ret = mtvpu_vram_alloc(drm, core->mem_group_id, size, &node->local_phys_addr, &node->handle);
				if(node->local_phys_addr < MT_PH_JPU_MCU_FW_PROGRAM_LOAD_ADDR && node->local_phys_addr >= (MT_PH_JPU_MCU_FW_PROGRAM_LOAD_ADDR - program_size)) {
					handle2 = node->handle;
					ret = mtvpu_vram_alloc(drm, core->mem_group_id, size, &node->local_phys_addr, &node->handle);
					mtgpu_vram_free(handle2);
				}
				mtgpu_vram_free(handle1);
			}
		}
		node->unified_phys_addr = node->local_phys_addr;
	} else {
		if (chip->driver_mode == MTGPU_DRIVER_MODE_HOST) {
			ret = mtgpu_vram_alloc(drm, MTGPU_SEGMENT_ID_VPU_HOST, size, &node->local_phys_addr, &node->handle);
			node->unified_phys_addr = node->local_phys_addr;
		}
		else {
			ret = mtgpu_vram_alloc(drm, core->mem_group_id, size, &node->local_phys_addr, &node->handle);
			if (!ret) {
				ret = mtgpu_bo_get_phys_addr(node->handle, 0, gpu_page_shift, &node->unified_phys_addr,
					MTGPU_ADDR_TRANSLATION_UNIFIED_PA);
				if (ret) {
					vpu_err("unified addresee get failed, core:%d!\n", idx);
					goto unified_addr_err;
				}
			}
		}
	}
	if (ret) {
		vpu_err("vram alloc failed, core:%d!\n", idx);
		goto nomem;
	}

	drm_gem_private_object_init(drm, node->obj, size);
	VLOG(MTDEBUG, "vpu_gem_alloc_node: type:%d, size:%llx, mem_group_id:%d, local_phys_addr:%llx,"
	     "unified_phys_addr:%llx,handle:%llx\n", type, size, core->mem_group_id, node->local_phys_addr,
		 node->unified_phys_addr, node->handle);

	node->size = size;
	return node;

unified_addr_err:
	mtgpu_vram_free(node->handle);
nomem:
	os_kfree(node->obj);
	os_kfree(node);
	return NULL;
}

void gem_free_node(struct mt_chip *chip, struct mt_node *node)
{
	/* fw buffer only */

	if (chip->soc_mode && chip->io_domain)
		vpu_smmu_unmap(chip, node->local_phys_addr);
	mtgpu_vram_free(node->handle);

	drm_gem_object_release(node->obj);
}

/*
 * @Copyright   Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License     Dual MIT/GPLv2
 */

#ifndef _MTVPU_API_H_
#define _MTVPU_API_H_

#include "linux-types.h"

struct mt_node;
struct mt_file;
struct file;
struct device;
struct drm_device;
struct drm_file;
struct drm_gem_object;
struct mtgpu_codec_priv_data;
struct acpi_device;

#define vpu_warn(fmt, ...) os_pr_warn("[mtvpu] " fmt, ##__VA_ARGS__)
#define vpu_info(fmt, ...) os_pr_info("[mtvpu] " fmt, ##__VA_ARGS__)
#define vpu_err(fmt, ...) os_pr_err("[mtvpu] " fmt, ##__VA_ARGS__)

enum {
	MT_VPU_CREATE_SEMA = 0,
	MT_VPU_DESTROY_SEMA,
	MT_VPU_WAIT_SEMA,
};

extern struct platform_driver vpu_driver;
extern struct platform_driver vpu_rpm_core_driver;

struct mt_chip *to_chip(struct drm_device *drm);
struct mt_file *os_get_drm_file_private_data(struct drm_file *file);
void os_set_drm_file_private_data(struct drm_file *file, struct mt_file *priv);
void *vpu_get_pvr_node(struct drm_device *drm);

unsigned long os_memremap_wb(void);
void *os_memremap(resource_size_t offset, size_t size, unsigned long flags);
void os_memunmap(void *addr);
void dcache_flush(void *addr, size_t len);

int os_acpi_dev_pm_explicit_get(struct acpi_device *adev, int *state);
int os_acpi_dev_pm_explicit_set(struct acpi_device *adev, int state);

int mtvpu_drm_open(struct drm_device *drm, struct drm_file *file);
void mtvpu_drm_release(struct drm_device *drm, struct drm_file *file);
void mtvpu_gem_free_obj(struct drm_gem_object *obj);

int mtvpu_job_submit(struct drm_device *drm, struct drm_file *file_priv, void __user *data,
		     uint32_t size, struct mtgpu_codec_priv_data *data_priv);
int mtvpu_bo_wait(struct drm_device *drm, struct drm_file *file_priv, u64 bo_handle, u32 offset, u32 flags, s64 timeout_ns);

int vpu_sema_cmd_proc(u64 type, u64 *data, struct drm_file *file);
void vpu_sema_release(struct drm_file *file);

int vgpu_save_vpu_info(void *data, int vm_id, u8 *buf, u32 buf_size);
int vgpu_restore_vpu_info(void *data, int vm_id, u8 *buf, u32 buf_size);
int mtvpu_hot_upgrade_set_stage(void *dev, int stage);
#endif /* _MTVPU_API_H_ */

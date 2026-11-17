/*
 * @Copyright   Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License     Dual MIT/GPLv2
 */

#include <linux/device.h>
#include <linux/acpi.h>
#include <linux/version.h>
#include <drm/drm_device.h>
#include <drm/drm_gem.h>
#if defined(OS_DRM_DRMP_H_EXIST)
#include <drm/drmP.h>
#else
#include <drm/drm_file.h>
#include <drm/drm_ioctl.h>
#endif

#include "mt.h"
#include "mtgpu_drm_drv.h"
#include "mtgpu_drm_gem.h"
#include "mtgpu_drm_internal.h"
#include "mtgpu_mdev.h"
#include "os-interface.h"
#include "mtvpu_drv.h"
#include "mtvpu_api.h"

struct mt_chip *to_chip(struct drm_device *drm)
{
	struct mtgpu_drm_private *drm_private = drm->dev_private;

	if (!drm_private)
		return NULL;

	return drm_private->chip;
}

struct mt_file *os_get_drm_file_private_data(struct drm_file *file)
{
	struct mtgpu_drm_file *drv_priv = file->driver_priv;

	return drv_priv->vpu_priv;
}

void os_set_drm_file_private_data(struct drm_file *file, struct mt_file *priv)
{
	struct mtgpu_drm_file *drv_priv = file->driver_priv;

	if (!drv_priv)
		return;

	drv_priv->vpu_priv = priv;
}

void *vpu_get_pvr_node(struct drm_device *drm)
{
	struct mtgpu_drm_private *drm_private = drm->dev_private;

	return drm_private->mt_private.dev_node;
}

unsigned long os_memremap_wb(void)
{
	return MEMREMAP_WB;
}

void *os_memremap(resource_size_t offset, size_t size, unsigned long flags)
{
	return memremap(offset, size, flags);
}

void os_memunmap(void *addr)
{
	memunmap(addr);
}

#ifdef __aarch64__
void dcache_flush(void *addr, size_t len)
{
  char *base = (char *)addr;
  int offset;
  for (offset = 0; offset < len; offset += 64)
    asm volatile ("dc civac, %0" :: "r" (base + offset));
}
#else
void dcache_flush(void *addr, size_t len)
{
}
#endif

int os_acpi_dev_pm_explicit_get(struct acpi_device *device, int *state)
{
#if defined(CONFIG_ACPI)
	unsigned long long psc;
	acpi_status status;

	status = acpi_evaluate_integer(device->handle, "_PSC", NULL, &psc);
	if (ACPI_FAILURE(status))
		return -ENODEV;

	*state = psc;
	return 0;
#else
	return -ENODEV;
#endif
}

int os_acpi_dev_pm_explicit_set(struct acpi_device *adev, int state)
{
#if defined(CONFIG_ACPI)
	acpi_status status;
	if (adev->power.states[state].flags.explicit_set) {
		char method[5] = { '_', 'P', 'S', '0' + state, '\0' };

		status = acpi_evaluate_object(adev->handle, method, NULL, NULL);
		if (ACPI_FAILURE(status))
			return -ENODEV;
	}
	return 0;
#else
	return -ENODEV;
#endif
}

int vpu_sema_cmd_proc(u64 type, u64 *data, struct drm_file *file)
{
	struct mt_file *priv = os_get_drm_file_private_data(file);
	struct mtvpu_sema_list *cur, *next;
	int ret = -1;
	switch (type)
	{
	case MT_VPU_CREATE_SEMA:
		cur = kzalloc(sizeof(*cur), GFP_KERNEL);
		if (!cur)
			return -ENOMEM;
		cur->sema = kzalloc(sizeof(*(cur->sema)), GFP_KERNEL);
		if (!cur->sema) {
			kfree(cur);
			return -ENOMEM;
		}
		sema_init(cur->sema, 0);
		os_mutex_lock(priv->file_lock);
		list_add_tail(&cur->list, &priv->sema_head);
		os_mutex_unlock(priv->file_lock);
		*data = (u64)cur->sema;
		ret = 0;
		break;
	case MT_VPU_DESTROY_SEMA:
		os_mutex_lock(priv->file_lock);
		list_for_each_entry_safe(cur, next, &priv->sema_head, list) {
			if (*data == (u64)cur->sema) {
				list_del(&cur->list);
				kfree(cur->sema);
				kfree(cur);
				ret = 0;
				break;
			}
		}
		os_mutex_unlock(priv->file_lock);
		break;

	default:
		vpu_err("wrong sema cmd in vpu: %d\n", (u32)type);
		return -1;
	}
	return ret;
}

void vpu_sema_release(struct drm_file *file)
{
	struct mt_file *priv = os_get_drm_file_private_data(file);
	struct mtvpu_sema_list *cur, *next;
	os_mutex_lock(priv->file_lock);
	list_for_each_entry_safe(cur, next, &priv->sema_head, list) {
		list_del(&cur->list);
		vpu_warn("maybe need to destroy the sema %p in user space.", cur->sema);
		kfree(cur->sema);
		kfree(cur);
	}
	os_mutex_unlock(priv->file_lock);
}

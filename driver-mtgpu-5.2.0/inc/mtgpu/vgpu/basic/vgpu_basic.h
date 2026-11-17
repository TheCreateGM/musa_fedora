/*
 * @Copyright   Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License     Dual MIT/GPLv2
 */

#ifndef __VGPU_BASIC_H__
#define __VGPU_BASIC_H__

struct device;
struct work_struct;
struct delayed_work;

enum {
	HOT_UPGRADE_STAGE_READY,
	HOT_UPGRADE_STAGE_BEGIN,
	HOT_UPGRADE_STAGE_SAVE,
	HOT_UPGRADE_STAGE_RESTORE,
	HOT_UPGRADE_STAGE_END,
};

enum {
	MTGPU_INTER_MODULE_DATA_TYPE_DRIVER,
	MTGPU_INTER_MODULE_DATA_TYPE_PCI,
	MTGPU_INTER_MODULE_DATA_TYPE_MDEV,
};

enum {
	MTGPU_DRIVER_STATUS_READY = 0,
	MTGPU_DRIVER_STATUS_INIT,
	MTGPU_DRIVER_STATUS_UPGRADE_BEGIN,
	MTGPU_DRIVER_STATUS_UPGRADE_SAVE,
	MTGPU_DRIVER_STATUS_UPGRADE_RESTORE,
	MTGPU_DRIVER_STATUS_UPGRADE_END,
	MTGPU_DRIVER_STATUS_UPGRADE_FORBIDEN,
	MTGPU_DRIVER_STATUS_FATAL_ERROR,
	MTGPU_DRIVER_STATUS_ROLLBACK_ERROR,
	MTGPU_DRIVER_STATUS_MAX,
};

void *mtgpu_mdev_get_parent_state(struct device *dev);
bool vgpu_queue_work(struct work_struct *work);
bool vgpu_queue_delayed_work(struct delayed_work *dwork, unsigned long delay);
#ifdef VGPU_SUPPORT_HOT_UPGRADE
int mtgpu_get_driver_status(void);
int mtgpu_set_driver_status(int status);
int mtgpu_basic_device_attach(struct device *dev);
void mtgpu_basic_device_detach(struct device *dev);
bool mtgpu_is_in_upgrading(void);
int mtgpu_hug_data_save(struct device *dev, void *data, size_t size, u64 key, int type);
ssize_t mtgpu_hug_data_restore(struct device *dev, void *data, u64 key, int type);
void vgpu_resident_work(struct work_struct *work);

#ifdef VGPU_BASIC_MODULE
struct vgpu_hot_upgrade_data;
int vgpu_hud_create(struct vgpu_hot_upgrade_data **hud);
void vgpu_hud_destroy(struct vgpu_hot_upgrade_data *hud);
struct device *get_pci_base(struct device *dev);
const struct mtgpu_vgpu_basic_ops *mtgpu_try_get_vgpu_ops(void);
const struct mtgpu_vgpu_basic_ops *mtgpu_get_vgpu_ops(void);
bool mtgpu_vgpu_is_bdev_ready(struct device *dev);
void mtgpu_put_vgpu_ops(void);
#endif /* VGPU_BASIC_MODULE */
#else /* VGPU_SUPPORT_HOT_UPGRADE */
static inline int mtgpu_set_driver_status(int status)
{
	return 0;
}

static inline int mtgpu_basic_device_attach(struct device *dev)
{
	return 0;
}

static inline void mtgpu_basic_device_detach(struct device *dev)
{
}

static inline bool mtgpu_is_in_upgrading(void)
{
	return false;
}

static inline int mtgpu_hug_data_save(struct device *dev,
				      void *data, size_t size, u64 key, int type)
{
	return 0;
}

static inline ssize_t mtgpu_hug_data_restore(struct device *dev, void *data, u64 key, int type)
{
	return 0;
}

static inline bool mtgpu_vgpu_is_bdev_ready(struct device *dev)
{
	return true;
}

static inline void vgpu_resident_work(struct work_struct *work)
{
}
#endif /* VGPU_SUPPORT_HOT_UPGRADE */
#endif /* __VGPU_BASIC_H__ */

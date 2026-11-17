/*
 * @Copyright   Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License     Dual MIT/GPLv2
 */

#ifndef __MTGPU_VGPU_IPC_H__
#define __MTGPU_VGPU_IPC_H__

#include "mtgpu.h"

#ifndef MT_UNREFERENCED_PARAMETER
#define MT_UNREFERENCED_PARAMETER(param) ((void)(param))
#endif

#if (MUSA_NUM_OS_SUPPORTED > 1) && (MTGPU_APPHINT_DRIVERMODE == 1)
int mtgpu_vgpu_ipc_init(struct mtgpu_device *mtdev);
void mtgpu_vgpu_ipc_deinit(struct mtgpu_device *mtdev);
void mtgpu_vgpu_ipc_ack_irq(struct mtgpu_device *mtdev);
u32 mtgpu_vgpu_ipc_get_hwr_stage(struct mtgpu_device *mtdev);
u32 mtgpu_vgpu_ipc_get_fw_hotupgrade_stage(struct mtgpu_device *mtdev);
u32 mtgpu_vgpu_ipc_get_host_hug_stage(struct mtgpu_device *mtdev);
void mtgpu_vgpu_ipc_set_host_hug_stage(struct mtgpu_device *mtdev, u32 stage);
void mtgpu_vgpu_ipc_vsync_status_on(struct device *dev, int id);
void mtgpu_vgpu_ipc_vsync_status_off(struct device *dev, int id);
bool mtgpu_vgpu_ipc_vsync_status_is_irq(struct device *dev, int id);
void mtgpu_vgpu_ipc_vsync_status_clear_irq(struct device *dev, int id);
int mtgpu_vgpu_ipc_version_compat_check(struct device *dev);
int mtgpu_vgpu_ipc_get_os_version(struct device *dev);
bool mtgpu_vgpu_ipc_is_gpu_normal(struct device *dev);
#else
static inline int mtgpu_vgpu_ipc_init(struct mtgpu_device *mtdev)
{
	MT_UNREFERENCED_PARAMETER(mtdev);

	return 0;
}

static inline void mtgpu_vgpu_ipc_deinit(struct mtgpu_device *mtdev)
{
	MT_UNREFERENCED_PARAMETER(mtdev);
}

static inline void mtgpu_vgpu_ipc_ack_irq(struct mtgpu_device *mtdev)
{
	MT_UNREFERENCED_PARAMETER(mtdev);
}

static inline u32 mtgpu_vgpu_ipc_get_hwr_stage(struct mtgpu_device *mtdev)
{
	MT_UNREFERENCED_PARAMETER(mtdev);

	return 0;
}

static inline u32 mtgpu_vgpu_ipc_get_fw_hotupgrade_stage(struct mtgpu_device *mtdev)
{
	MT_UNREFERENCED_PARAMETER(mtdev);

	return 0;
}

static inline u32 mtgpu_vgpu_ipc_get_host_hug_stage(struct mtgpu_device *mtdev)
{
	MT_UNREFERENCED_PARAMETER(mtdev);

	return 0;
}

static inline void mtgpu_vgpu_ipc_set_host_hug_stage(struct mtgpu_device *mtdev, u32 stage)
{
	MT_UNREFERENCED_PARAMETER(mtdev);
	MT_UNREFERENCED_PARAMETER(stage);
}

static inline void mtgpu_vgpu_ipc_vsync_status_on(struct device *dev, int id)
{
	MT_UNREFERENCED_PARAMETER(dev);
	MT_UNREFERENCED_PARAMETER(id);
}

static inline void mtgpu_vgpu_ipc_vsync_status_off(struct device *dev, int id)
{
	MT_UNREFERENCED_PARAMETER(dev);
	MT_UNREFERENCED_PARAMETER(id);
}

static inline bool mtgpu_vgpu_ipc_vsync_status_is_irq(struct device *dev, int id)
{
	MT_UNREFERENCED_PARAMETER(dev);
	MT_UNREFERENCED_PARAMETER(id);

	return false;
}

static inline void mtgpu_vgpu_ipc_vsync_status_clear_irq(struct device *dev, int id)
{
	MT_UNREFERENCED_PARAMETER(dev);
	MT_UNREFERENCED_PARAMETER(id);
}

static inline int mtgpu_vgpu_ipc_version_compat_check(struct device *dev)
{
	MT_UNREFERENCED_PARAMETER(dev);

	return 0;
}

static inline int mtgpu_vgpu_ipc_get_os_version(struct device *dev)
{
	MT_UNREFERENCED_PARAMETER(dev);

	return 0;
}

static inline bool mtgpu_vgpu_ipc_is_gpu_normal(struct device *dev)
{
	MT_UNREFERENCED_PARAMETER(dev);

	return false;
}
#endif

#endif /* __MTGPU_VGPU_IPC_H__ */

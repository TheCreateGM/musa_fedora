/*
 * @Copyright   Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License     Dual MIT/GPLv2
 */

#ifndef __MTGPU_PCIE_LINK_MONITOR_H__
#define __MTGPU_PCIE_LINK_MONITOR_H__

/* In oreder to meet the address alignment of memory layout,
 * pcie link monitor will take up 4096 bytes.
 */
#define MTGPU_PCIE_LINK_MONITOR_BAR2_CHECK_SIZE (0x1000) /* 4096 Bytes */

/* The PCI config space is 256 byets, and PCI-Express config space is 4096 bytes.
 * And PCI-Express will be compatible with the PCI config space. So we use 256 bytes.
 */
#define MTGPU_PCIE_LINK_MONITOR_CONFIG_SPACE_CHECK_SIZE (256) /* 256 Bytes */

struct mtgpu_device;
struct workqueue_struct;
struct delayed_work;

struct mtgpu_pcie_link_monitor {
	struct workqueue_struct *work_queue;
	struct delayed_work *dwork;
	bool is_abnormal;
	u32 *bar2_check_base;
};

bool mtgpu_pcie_link_is_abnormal(struct mtgpu_device *mtdev);
int mtgpu_pcie_bar0_access_check(struct mtgpu_device *mtdev);
int mtgpu_pcie_bar2_access_check(struct mtgpu_device *mtdev);
int mtgpu_pcie_config_space_access_check(struct mtgpu_device *mtdev);
int mtgpu_pcie_link_monitor_init(struct mtgpu_device *mtdev);
int mtgpu_pcie_link_monitor_exit(struct mtgpu_device *mtdev);
void mtgpu_pcie_link_monitor_stop(struct mtgpu_device *mtdev);
void mtgpu_pcie_link_monitor_start(struct mtgpu_device *mtdev);

#endif

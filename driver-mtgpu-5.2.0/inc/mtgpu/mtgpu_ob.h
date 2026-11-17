/*
 * @Copyright Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License Dual MIT/GPLv2
 */

#ifndef _MTGPU_OB_H_
#define _MTGPU_OB_H_

#include "mtgpu_ob_map.h"

struct pci_dev;
struct mtgpu_device;
struct inode;
struct file;

struct mtgpu_ob_conf {
	u64 offset;
	u64 flag;
	u64 sysmem_addr_start;
	u64 sysmem_addr_end;
	struct mtgpu_ob_map_table map_table;
	bool force_dma32_alloc;
};

int mtgpu_ob_init(struct mtgpu_device *mtdev);
void mtgpu_ob_deinit(struct mtgpu_device *mtdev);

/*
 * mtgpu_ob_check - check whether it is necessary to transfer the cpu pa to outbound addr
 *
 * @dev: the device
 * @ob_handle: return a handle if it is neessary to get outbound addr,
 *   typically the ob conf pointer is returned
 * @return: if no need to get ob addr return 0, if need to get ob addr return 1,
 *   return error on failure
 */
int mtgpu_ob_check(struct device *dev, void **ob_handle);

/*
 * mtgpu_ob_addr_from_cpu_pa_with_hint - get the outbound addr with the given cpu pa
 *
 * @ob_conf: the mtgpu_ob_conf pointer
 * @cpu_pa: the cpu pa need to be transfered
 * @ob_addr: the returned outbound addr
 * @index_hint: null will be ignored, if set, search on ob map will start on the given index
 *     the index will be updated to the newly searched index
 * @return: 0 on success, error on failure
 */
int mtgpu_ob_addr_from_cpu_pa_with_hint(struct mtgpu_ob_conf *ob_conf, u64 cpu_pa, u64 *ob_addr,
					int *index_hint);

int mtgpu_ob_addr_from_cpu_pa(struct device *dev, u64 cpu_pa, u64 *ob_addr);
int mtgpu_cpu_pa_from_ob_addr(struct device *dev, u64 ob_addr, u64 *cpu_pa);
u64 mtgpu_get_ob_addr_flag(struct device *dev);
u64 mtgpu_get_fec_ob_addr_flag(struct device *dev);
bool mtgpu_ob_user_res_is_set(struct device *dev);
bool mtgpu_ob_sys_res_is_set(struct device *dev);
bool mtgpu_ob_pci_res_is_set(struct device *dev);
bool mtgpu_ob_is_no_snoop_set(struct device *dev);
int mtgpu_get_system_addr_range(struct device *dev, u64 *addr_start, u64 *addr_end);
int mtgpu_cpu_system_pa_to_gpu_noc_addr(struct device *dev,
					u64 cpu_paddr, u64 *ob_noc_addr);
bool mtgpu_ob_is_force_dma32_zone(struct device *dev);
int mtgpu_ob_debugfs_show(struct device *dev, char *buffer, size_t buf_size);

#endif /* _MTGPU_OB_H_ */

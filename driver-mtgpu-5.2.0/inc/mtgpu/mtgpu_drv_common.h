/*
 * @Copyright   Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License     Dual MIT/GPLv2
 */

#ifndef __MTGPU_DRV_COMMON_H__
#define __MTGPU_DRV_COMMON_H__

/* Whether mtgpu device support llc operation */
#define MTGPU_CAPABILITY_LLC		BIT(0)
/* Whether mtgpu device support ce job */
#define MTGPU_CAPABILITY_CE		BIT(1)
/* Whether mtgpu device support dma job */
#define MTGPU_CAPABILITY_DMA		BIT(2)

struct device;
struct pci_device_id;
struct mtgpu_device;
struct mtgpu_driver_data;
struct gpu_cfg_info;
struct gpu_cfg_req;
struct mtgpu_device_node;
struct scatterlist;

extern struct device_attribute dev_attr_mtgpu_info;

bool mtgpu_llc_is_supported(struct mtgpu_device *mtdev);
bool mtgpu_ce_is_supported(struct mtgpu_device *mtdev);
bool mtgpu_dma_is_supported(struct mtgpu_device *mtdev);
void mtgpu_set_capability(struct mtgpu_device *mtdev);
int mtgpu_ipc_get_gpu_cfg(struct mtgpu_device *mtdev, struct gpu_cfg_info *gpu_info);
int mtgpu_ipc_set_gpu_cfg(struct mtgpu_device *mtdev, struct gpu_cfg_req *gpu_req);
int mtgpu_get_core_cfg(struct mtgpu_device *mtdev);
int mtgpu_driver_data_init(struct mtgpu_device *mtdev, struct mtgpu_driver_data *drvdata);
void mtgpu_display_set_mt(void);
bool mtgpu_display_is_dummy(void);
bool mtgpu_display_is_none(void);
bool mtgpu_card_support_display(struct mtgpu_device *mtdev);
bool mtgpu_card_is_server(struct pci_dev *pdev);
bool mtgpu_device_restore_pci_state(struct pci_dev *pdev);
bool mtgpu_mpc_is_supported(struct mtgpu_device *mtdev);
bool mtgpu_d2d_check_linked(struct mtgpu_device *mtdev);
u16 mtgpu_get_device_flags(struct pci_dev *pdev);
void mtgpu_mpc_dump(struct mtgpu_device *mtdev);
u8 mtgpu_get_core_count_from_logic_id(struct mtgpu_device *mtdev, u32 logic_core_id);
int mtgpu_get_driver_mode(void);
int mtgpu_pm_resume(struct device *dev);
int mtgpu_pm_resume_early(struct device *dev);
int mtgpu_pm_suspend(struct device *dev);
int mtgpu_pm_suspend_late(struct device *dev);
int mtgpu_probe(struct pci_dev *pdev, const struct pci_device_id *id);
void mtgpu_remove(struct pci_dev *pdev);
void mtgpu_shutdown(struct pci_dev *pdev);
void *mt_dma_alloc_coherent(struct device *dev, size_t size, dma_addr_t *dma_handle, gfp_t gfp);
void mt_dma_free_coherent(struct device *dev, size_t size, void *cpu_addr, dma_addr_t dma_handle);
int mt_dma_map_sg(struct device *dev, struct scatterlist *sg, int nents, u64 dir);
void mt_dma_unmap_sg(struct device *dev, struct scatterlist *sg, int nents, u64 dir);
int mtgpu_drv_mkdir(const char *name, umode_t mode);
int mtgpu_pm_runtime_get(struct device *dev);
int mtgpu_pm_runtime_put(struct device *dev);
int mtgpu_pm_runtime_put_async(struct device *dev);
void mtgpu_pm_runtime_init(struct mtgpu_device *mtdev, struct device *dev);
void mtgpu_pm_runtime_set_autosuspend(struct mtgpu_device *mtdev,
				      struct device *dev,
				      u32 delay_time);
void mtgpu_pm_runtime_exit(struct device *dev);
int mtgpu_gpu_pm_runtime_get(struct mtgpu_device_node *dev_node);
int mtgpu_gpu_pm_runtime_put(struct mtgpu_device_node *dev_node);
void mtgpu_device_pm_runtime_get(struct mtgpu_device *mtdev);
void mtgpu_device_pm_runtime_put(struct mtgpu_device *mtdev);
bool mtgpu_support_pm_runtime(struct mtgpu_device *mtdev);
void *dma_alloc_on_dma32_zone(struct device *dev, size_t size, dma_addr_t *dma_handle);
void dma_free_on_dma32_zone(struct device *dev, size_t size, void *va_addr, dma_addr_t dma_handle);
int mtgpu_kick_out_firmware_fb(resource_size_t base, resource_size_t size);

bool mtgpu_is_haps_platform(void);
bool mtgpu_is_emu_platform(void);
bool mtgpu_is_vps_platform(void);
bool mtgpu_is_hw_platform(void);
void mtlink_module_param_init(void);

#endif /*__MTGPU_DRV_COMMON_H__*/

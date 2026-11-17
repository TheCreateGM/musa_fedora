/*
 * @Copyright   Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License     Dual MIT/GPLv2
 */

#ifndef __MTGPU_DEVICE_H__
#define __MTGPU_DEVICE_H__

#include "dllist.h"
#include "mtgpu.h"
#include "mtgpu_uio_km.h"

#define MTGPU_PCIE_DMA_DESC_MEM_SIZE		(0x800000)
#define MTGPU_FW_CFG_CE_QUEUE_BITMAP_DEFAULT	(0xFF)
#define MTGPU_FW_CFG_CACHE_PREFETCH_DISABLE	(0x0)
#define MTGPU_FW_CFG_CACHE_PREFETCH_MODE1	(0x1)
#define MTGPU_FW_CFG_CACHE_PREFETCH_MODE2	(0x2)
#define MTGPU_FW_CFG_CACHE_PREFETCH_MODE3	(0x3)
#define MTGPU_FW_MTS_SCHEDULE_REG_MAX_COUNT	(16)

struct mtlink_ops;
struct mtlink_device;
struct mtgpu_device;
struct mtgpu_platform_data;
struct crypto_shash;
struct mtgpu_mdev_vpu_info;
struct mtgpu_softirq_ctrl;
struct mtgpu_fw_info;
struct mtgpu_fw_ops;
struct DEVMEM_CONTEXT_TAG;
struct mtgpu_gpu_util_stats;
struct mtgpu_devfreq;

enum hw_module_type {
	MTGPU_HW_MODULE_GPU = 0,
	MTGPU_HW_MODULE_DISPC,
	MTGPU_HW_MODULE_DP,
	MTGPU_HW_MODULE_DP_PHY,
	MTGPU_HW_MODULE_HDMI,
	MTGPU_HW_MODULE_VPU,
	MTGPU_HW_MODULE_AUDIO,
	MTGPU_HW_MODULE_DSC,
};

struct mtgpu_resource {
	resource_size_t start;
	resource_size_t end;
	const char *name;
	bool is_mem;
};

struct mtgpu_chip_info {
	u32 sys_region_size;

	u32 clk_reset_reg_offset;
	u32 clk_reset_reg_size;

	u32 dsp_reg_offset;
	u32 dsp_reg_size;

	u32 fedma_reg_offset;
	u32 fedma_reg_size;

	u32 noc_reg_offset;
	u32 noc_reg_size;

	u32 vid_reg_offset;
	u32 vid_reg_size;

	u32 socif_offset;
	u32 socif_size;

	u32 fec_fts_offset;
	u32 fec_fts_size;

	u32 intc_regs_offset;
	u32 intc_regs_size;

	u32 distributor_regs_offset;
	u32 distributor_regs_size;

	u32 pcie_config_regs_offset;
	u32 pcie_config_regs_size;

	u32 gpu_ss_reg_offset;
	u32 gpu_ss_reg_size;

	u32 pcie_ss_cfg_reg_offset;
	u32 pcie_ss_cfg_reg_size;

	u32 pcie_ctrl_reg_offset;
	u32 pcie_ctrl_reg_size;

	u32 pcie_vf_msi_regs_offset;
	u32 pcie_vf_msi_regs_size;

	u32 pcie_phy_cfg_reg_offset;
	u32 pcie_phy_cfg_reg_size;

	u32 pcie_sram_shared_offset;
	u32 pcie_sram_shread_size;

	u64 ddr_mem_offset;
	u64 default_ddr_bar_size;

	u32 llc_reg_offset;
	u32 llc_reg_size;

	u32 gpu_daa_reg_offset;
	u32 gpu_daa_reg_size;

	u32 gpu_misc_reg_offset;
	u32 gpu_misc_reg_size;

	u32 cursor_reserved_size;

	u32 vps_debug_bar;

	/* gpu core num per chip*/
	u32 gpu_core_num;

	u64 ob_addr_flag;
	u64 ob_addr_size;

	u64 fec_ob_addr_flag;
	u64 fec_bl_offset;
	u64 fec_bl_startup_pc;
	u64 fec_img_offset;
	u32 fec_bl_location;

	u32 intd_cd_reg_offset;
	u32 intd_cd_reg_size;
	u32 pcie_mhi_reg_offset;
	u32 pcie_mhi_reg_size;

	u32 pfm_mmu_reg_offset;
	u32 pfm_mmu_reg_size;
	u32 pfm_hbm_reg_offset;
	u32 pfm_hbm_reg_size;
	u32 pfm_mtlink_reg_offset;
	u32 pfm_mtlink_reg_size;
	u32 pfm_pcie_ppm_reg_offset;
	u32 pfm_pcie_ppm_reg_size;

	u32 pfm_ddrc_reg_offset;
	u32 pfm_ddrc_reg_size;
	u32 pfm_d2d_1_reg_offset;
	u32 pfm_d2d_1_reg_size;
	u32 pfm_d2d_0_reg_offset;
	u32 pfm_d2d_0_reg_size;
	u32 pfm_llc_reg_offset;
	u32 pfm_llc_reg_size;

	u64 mts_reg_count;
	u32 mts_reg_offset[MTGPU_FW_MTS_SCHEDULE_REG_MAX_COUNT];

	u32 fec_sram_offset;
	u32 fec_sram_size;

	u32 mcu_mhi_reg_offset;
	u32 mcu_mhi_reg_size;
	u32 mcu_mhi_reg_group_count;
	u32 mcu_mhi_reg_gruop_stride;
	u32 mcu_mhi_set_reg_offset;
	u32 mcu_mhi_set_reg_width;

	u32 fec_mhi_reg_offset;
	u32 fec_mhi_reg_size;
	u32 fec_mhi_reg_group_count;
	u32 fec_mhi_reg_gruop_stride;
	u32 fec_mhi_set_reg_offset;
	u32 fec_mhi_set_reg_width;
};

struct mtlink_private_data {
	/* TODO: maybe capability is here */

	struct mtlink_device *link_device;
};

struct mtgpu_driver_data {
	struct mtgpu_chip_info *chip_info;
	struct mtgpu_dma_ops *dma_ops;
	struct mtgpu_int_ops *int_ops;
	struct mtgpu_display_ops *disp_ops;
	struct mtgpu_pcie_local_mgmt_ops *pcie_local_mgmt_ops;
	struct mtgpu_sriov_ops *sriov_ops;
	struct mtgpu_pcie_perf_ops *pcie_perf_ops;
	const struct mtgpu_smc_ops *smc_ops;
	const struct mtgpu_fec_ops *fec_ops;
	const struct mtgpu_cmc_ops *cmc_ops;
	struct mtgpu_llc_ops *llc_ops;
	struct mtlink_ops *link_ops;
	struct mtgpu_ob_ops *ob_ops;
	struct mtgpu_gpu_ss_ops *gpu_ss_ops;
	struct mtgpu_pcie_ss_ops *pcie_ss_ops;
	struct mtgpu_pfm_ops *pfm_ops;
	struct mtgpu_daa_ops *daa_ops;
	struct mtgpu_reg_decode_ops *reg_decode_ops;
	struct mtgpu_ecc_ops *ecc_ops;
	struct mtgpu_debug_reg_ops *debug_reg_ops;
	struct mtgpu_ce_pm_ops *ce_pm_ops;
	struct mtgpu_pstate_ops *pstate_ops;
	struct mtgpu_security_ops *security_ops;
	int (*get_platform_device_info)(struct mtgpu_device *mtdev, u32 hw_module, u32 hw_id,
					struct mtgpu_resource **mtgpu_res, u32 *num_res,
					void **data, size_t *size_data);
	void (*fw_cfg_init)(struct mtgpu_device_node *dev_node, void *fw_cfg);
	void (*set_fw_hwr_flag)(struct mtgpu_device_node *dev_node);
	void (*set_fw_cfg_ob_dummy_addr)(struct mtgpu_device *mtdev, void *fw_cfg, u64 dummy_addr);
	const u32* (*get_fw_queue_config)(void);
	int (*get_fw_pc_sample_buf_size)(void);
	u64 (*get_fw_cfg_yuv_csc_table_base)(void *fw_cfg);
	int (*register_access_check)(struct mtgpu_device *mtdev);
	int (*get_softirq_ctrls)(struct mtgpu_softirq_ctrl **softirq_ctrls, int *ctrls_cnt);
	void (*get_mpx_map)(struct mtgpu_device *mtdev, void *reg_base);
	void (*module_param_overwrite)(struct mtgpu_device *mtdev);
};

struct mtgpu_device_node {
	/* gpu platform device base */
	struct device *dev;

	u32 internal_id;
	u32 device_id;

	/* address of device registers */
	void __iomem *regs_base;

	void *gpu_reg_store;
	u64 regs_cpu_pa_base;
	u32 regs_size;

	struct _MTEXT_INFO_ *mtext_info;
	u64 mtext_info_pa;
	struct mutex *vm_ctx_list_lock;
	struct list_head vm_ctx_list;
	atomic_t ioctl_block_count;
	atomic_t ioctl_disable_count;
	atomic_t ioctl_alive_count;

	/* device states */
	u32 device_state;
	struct mutex *device_state_lock;
	struct wait_queue_head *fw_state_machine_wq;
	u64 state_machine_update_time;
	struct task_struct *fw_state_machine_thread;
	struct mtgpu_fw_reboot_ctx *fw_reboot_ctx;
	struct wait_queue_head *hwr_done_waitq;

	/* Only for legacy UMD with ddk 2.0 */
	struct mutex *mem_ctx_lock;
	DLLIST_NODE mem_ctx_list;

	struct DEVMEM_CONTEXT_TAG *kernel_mem_ctx;
	struct mtgpu_syncobj_context *syncobj_context;

	atomic64_t dev_last_error;
	atomic64_t dev_current_error;

	struct mutex *gpu_util_lock;
	void *gpu_util_stats;
	int (*get_gpu_util_stats) (struct mtgpu_device_node *dev_node,
				   void *gpu_util_user,
				   struct mtgpu_gpu_util_stats *ret_stats);
	u32 gpu_util_config;

	struct mtgpu_fw_info *fw_info;
	const struct mtgpu_fw_ops *fw_ops;
	struct mtgpu_watchdog_info *watchdog_info;
	struct mtgpu_intr_poll_info *poll_info;
	atomic_t musa_snapshot_inflight;
	u64 musa_snapshot_last_submit_ns;

	struct mtgpu_fec_work_data *fec_work_data;
	struct wait_queue_head *ccb_wq;
	struct wait_queue_head *sync_cmd_wq;
	struct wait_queue_head *job_item_cleanup_wq;
	struct wait_queue_head *sem_user_waitq;
	bool fw_hang;
	atomic_t hwr_count;

	struct mtgpu_scheduler **scheduler;
	struct mtgpu_sched_entity *gp_cmd_entity;
	atomic64_t irq_ordinal;
	struct mutex *mcu_reg_lock;
	bool idle;
	spinlock_t *doorbell_lock;
	u64 *doorbell_pool_bitmap;
	u32 doorbell_pool_size;

	struct mtgpu_devfreq *devfreq;

	u64 ob_dummy_cpu_paddr;
	u64 ob_dummy_dma_addr;
	u64 ob_dummy_noc_addr;

	struct mtgpu_bo *yuv_csc_table_bo;
	struct mtgpu_bo *dm_kill_bo;
	struct mtgpu_bo *ph1_cmp_patch_bo;

	u32 primary_core_id;
	u32 primary_core_ids;

	struct mtgpu_uio_device *uio_dev;

	/* rm */
	u64 rm_gpu;
	u64 rm_default_heap;
	bool resizebar_off;

	void *rm_fw_priv;
	int (*rm_fw_kick)(void *fw_priv, uint32_t queue_idx);

	struct wait_queue_head *sem_waitq;

	/* debugfs */
	struct {
		struct dentry *root;
		struct dentry *gpu;
	} debugfs;
};

#define MT_OPS_DECLARE(name) \
extern struct mtgpu_int_ops name##_int_ops; \
extern struct mtgpu_llc_ops name##_llc_ops; \
extern struct mtgpu_dma_ops name##_dma_ops; \
extern struct mtgpu_sriov_ops name##_sriov_ops; \
extern struct mtgpu_pcie_perf_ops name##_pcie_perf_ops; \
extern struct mtgpu_display_ops name##_display_ops; \
extern struct mtgpu_pcie_local_mgmt_ops name##_pcie_local_mgmt_ops; \
extern struct mtgpu_smc_ops name##_smc_ops; \
extern struct mtgpu_fec_ops name##_fec_ops; \
extern struct mtgpu_cmc_ops name##_cmc_ops; \
extern struct mtlink_ops name##_mtlink_ops; \
extern struct mtgpu_ob_ops name##_ob_ops; \
extern struct mtgpu_pfm_ops name##_pfm_ops; \
extern struct mtgpu_gpu_ss_ops name##_gpu_ss_ops; \
extern struct mtgpu_reg_decode_ops name##_reg_decode_ops; \
extern struct mtgpu_ecc_ops name##_ecc_ops; \
extern struct mtgpu_debug_reg_ops name##_debug_reg_ops; \
extern struct mtgpu_pstate_ops name##_pstate_ops; \
extern struct mtgpu_security_ops name##_security_ops;

#define mtdev_for_each_device_node(mtdev, dev_node)						\
	int __i;										\
	for (__i = 0; __i < MTGPU_CORE_COUNT_MAX; ++__i)					\
		if ((mtdev)->drm_dev[__i] &&                                                    \
		    !OS_IS_ERR((mtdev)->drm_dev[__i]) &&                                        \
		    (dev_node = (struct mtgpu_device_node *)                                    \
		     OSGetDeviceNodeFromDrm(os_platform_get_drvdata((mtdev)->drm_dev[__i]))))

int mtgpu_resource_init_mem(struct mtgpu_resource *res, resource_size_t start,
			    int size, const char *name);
int mtgpu_resource_init_irq(struct mtgpu_resource *res, resource_size_t irq, const char *name);
int mtgpu_platform_data_vz_init(struct mtgpu_device *mtdev, struct mtgpu_platform_data *pdata);
int mtgpu_device_common_init(struct mtgpu_device *mtdev, struct mtgpu_module_param *param);
void mtgpu_device_common_exit(struct mtgpu_device *mtdev);
int mtgpu_get_primary_core_count(struct mtgpu_device *mtdev);
int mtgpu_register_devices(struct mtgpu_device *mtdev);
void mtgpu_unregister_devices(struct mtgpu_device *mtdev);
int mtgpu_device_pm_resume(struct mtgpu_device *mtdev);
int mtgpu_vram_backup(struct mtgpu_device *mtdev);
void mtgpu_device_alloc_buffer_for_suspend(struct mtgpu_device *mtdev);
int mtgpu_device_memory_fixup(struct mtgpu_device *mtdev,
			      struct mtgpu_module_param *param);
int mtgpu_pcie_resize_videomem_bar(struct mtgpu_device *mtdev, int resize);

int mtgpu_mtrr_setup(struct mtgpu_device *mtdev);
void mtgpu_mtrr_cleanup(struct mtgpu_device *mtdev);
int request_pci_io_addr(struct pci_dev *pdev, u32 index,
			resource_size_t offset, resource_size_t length);
void release_pci_io_addr(struct pci_dev *pdev, u32 index,
			 resource_size_t start, resource_size_t length);
int remap_io_region(struct pci_dev *pdev,
		    struct mtgpu_io_region *region, u32 index,
		    resource_size_t offset, resource_size_t size);
void cleanup_io_region(struct pci_dev *pdev,
		       struct mtgpu_io_region *region, u32 index);
int mtgpu_pcie_error_report_init(struct mtgpu_device *mtdev);
void mtgpu_pcie_error_report_exit(struct mtgpu_device *mtdev);
int mtgpu_register_audio_device(struct mtgpu_device *mtdev);
void mtgpu_unregister_audio_device(struct mtgpu_device *mtdev);
int mtgpu_register_video_device(struct mtgpu_device *mtdev);
void mtgpu_unregister_video_device(struct mtgpu_device *mtdev);
int mtgpu_register_gpu_device_with_coreid(struct mtgpu_device *mtdev, int logic_core_id,
					  int physical_core_id);
void mtgpu_generate_device_uuid(struct mtgpu_device *mtdev, bool is_mpc_parent, u32 instance_id, u8 *uuid);
void mtgpu_unregister_gpu_device(struct mtgpu_device *mtdev);
int mtgpu_register_dummy_device_with_core_id(struct mtgpu_device *mtdev, u8 coreid);
void mtgpu_register_drm_device_with_core_id(struct mtgpu_device *mtdev, u8 coreid);
void mtgpu_unregister_drm_device(struct mtgpu_device *mtdev);
void mtgpu_unregister_display_device(struct mtgpu_device *mtdev);
int mtgpu_register_device_done(struct mtgpu_device *mtdev);
int mtgpu_device_register_pm_nb(struct mtgpu_device *mtdev);
int mtgpu_device_unregister_pm_nb(struct mtgpu_device *mtdev);
int mtgpu_register_dispc_device(struct mtgpu_device *mtdev, u8 hw_id);
int mtgpu_register_dp_device(struct mtgpu_device *mtdev, u8 hw_id);
int mtgpu_register_hdmi_device(struct mtgpu_device *mtdev);
void mtgpu_pci_dev_config_data_release(struct mtgpu_device *mtdev);
void mtgpu_gpu_timer_enable(struct mtgpu_device *mtdev);
void mtgpu_gpu_timer_disable(struct mtgpu_device *mtdev);
u32 mtgpu_device_get_soc_timer(struct mtgpu_device *mtdev);
int mtgpu_guest_get_vpu_addr_base(struct mtgpu_device *mtdev);
int mtgpu_guest_get_vpu_info(struct mtgpu_device *mtdev, struct mtgpu_mdev_vpu_info *vpu_info);
int mtgpu_guest_notify_host_vpu(struct device *dev);
u64 mtgpu_guest_get_host_vpu_support_version(struct device *dev);
struct platform_device *mtgpu_get_first_valid_drm_device(struct mtgpu_device *mtdev);
void mtgpu_set_gpu_page_size(struct mtgpu_device *mtdev, u32 page_size);
void mtgpu_host_notify_linux_guest_start_hwr(struct device *dev, u32 mpc_id);
void mtgpu_host_notify_linux_guest_end_hwr(struct device *dev, u32 mpc_id);
int mtgpu_hug_init(struct mtgpu_device *mtdev);
void mtgpu_hug_exit(struct mtgpu_device *mtdev);
void mtgpu_hug_check(struct mtgpu_device *mtdev);
bool mtgpu_device_node_is_active(struct mtgpu_device *mtdev);
struct device *mtgpu_get_pcie_device_base(struct mtgpu_device_node *mt_dev_node);
u32 mtgpu_device_get_mpx_map(struct mtgpu_device *mtdev, void *reg_base);
int mtgpu_device_fw_cfg_init(struct mtgpu_device_node *dev_node, void *fw_cfg);
int mtgpu_device_set_fw_hwr_flag(struct mtgpu_device_node *dev_node);
void mtgpu_device_pci_slot_info_release(struct mtgpu_device *mtdev);
void mtgpu_device_fw_versions_release(struct mtgpu_device *mtdev);
u32 mtgpu_get_device_id(struct mtgpu_device *mtdev);
void mtgpu_device_const_info_init(struct mtgpu_device *mtdev);
u64 mtgpu_device_get_fw_cfg_yuv_csc_table_base(struct mtgpu_device *mtdev, void *fw_cfg);
const char *mtgpu_gpu_platform_dev_get_int_name(u32 core_id);
const char *mtgpu_dispc_platform_dev_get_int_name(u32 hw_id);
const char *mtgpu_dp_platform_dev_get_int_name(u32 hw_id);
const char *mtgpu_dp_phy_platform_dev_get_int_name(u32 hw_id);

#define GET_DEVICE_ID(mtdev)	(mtgpu_get_device_id(mtdev))

#define DEVICE_IS_SUDI(mtdev)		(GET_DEVICE_ID(mtdev) == DEVICE_ID_SUDI104)
#define DEVICE_IS_QUYUAN1(mtdev)	(GET_DEVICE_ID(mtdev) == DEVICE_ID_QUYUAN1)
#define DEVICE_IS_QUYUAN2(mtdev)	(GET_DEVICE_ID(mtdev) == DEVICE_ID_QUYUAN2)
#define DEVICE_IS_PINGHU1(mtdev)	(GET_DEVICE_ID(mtdev) == DEVICE_ID_PINGHU1)
#define DEVICE_IS_PINGHU1S(mtdev)	(GET_DEVICE_ID(mtdev) == DEVICE_ID_PINGHU1S)
#define DEVICE_IS_HG(mtdev)		(GET_DEVICE_ID(mtdev) == DEVICE_ID_HG)
#define DEVICE_IS_LS(mtdev)		(GET_DEVICE_ID(mtdev) == DEVICE_ID_LS)
#define DEVICE_IS_HS(mtdev)		(GET_DEVICE_ID(mtdev) == DEVICE_ID_HS)
#define DEVICE_IS_AM(mtdev)		(GET_DEVICE_ID(mtdev) == DEVICE_ID_AM)

#define DEVICE_IS_QUYUAN1_OR_LATER(mtdev)	(GET_DEVICE_ID(mtdev) >= DEVICE_ID_QUYUAN1)
#define DEVICE_IS_QUYUAN2_OR_LATER(mtdev)	(GET_DEVICE_ID(mtdev) >= DEVICE_ID_QUYUAN2)
#define DEVICE_IS_PINGHU1_OR_LATER(mtdev)	(GET_DEVICE_ID(mtdev) >= DEVICE_ID_PINGHU1)
#define DEVICE_IS_PINGHU1S_OR_LATER(mtdev)	(GET_DEVICE_ID(mtdev) >= DEVICE_ID_PINGHU1S)
#define DEVICE_IS_HG_OR_LATER(mtdev)		(GET_DEVICE_ID(mtdev) >= DEVICE_ID_HG)
#define DEVICE_IS_LS_OR_LATER(mtdev)		(GET_DEVICE_ID(mtdev) >= DEVICE_ID_LS)
#define DEVICE_IS_HS_OR_LATER(mtdev)		(GET_DEVICE_ID(mtdev) >= DEVICE_ID_HS)
#define DEVICE_IS_AM_OR_LATER(mtdev)		(GET_DEVICE_ID(mtdev) >= DEVICE_ID_AM)

#endif /* __MTGPU_DEVICE_H__ */

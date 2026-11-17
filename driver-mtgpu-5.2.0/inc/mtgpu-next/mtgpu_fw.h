/*
 * @Copyright Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License Dual MIT/GPLv2
 */

#ifndef _MTGPU_FW_H_
#define _MTGPU_FW_H_

#include "musaccb.h"
#include "mtfw_fwif.h"
#include "mtfw_fwif_fw_dump.h"
#include "mtgpu_job.h"
#include "mtgpu_mdev.h"

/*
 * In VDI, the size of FW heap is only 8M, so MTGPU_CCB_POOL_LENGTH
 * cannot be 4096 and there is no need to do so.
 */
#if defined(MUSAFW_VZ_SUPPORTED)
#define MTGPU_CCB_POOL_LENGTH			(256)
#else
#define MTGPU_CCB_POOL_LENGTH			(4096)
#endif

#define MTGPU_CCB_POOL_TOTAL_LENGTH		(MTGPU_CCB_POOL_LENGTH * MTFW_FWIF_QUEUE_TYPE_MAX)
#define MTGPU_FW_SHARED_ALIGN_BYTES		(8)

#define MTGPU_RISCV_FW_IS_ENABLED(mtdev) \
	(DEVICE_IS_PINGHU1S_OR_LATER(mtdev) && mtgpu_riscv_fw_enabled)

#define MTGPU_VGPU_FW_FILE_CHANGE_FLAG 0x1  /* load new firmware */
#define MTGPU_VGPU_VZ_FW_FILE_FLAG 0x2      /* load vz firmware */

#define FIRMWARE_LOAD_PATH(binary_name)		"mthreads/" binary_name

#define MTGPU_FW_QUEUE_CONFIG_ARRAY \
	{ \
		MTFW_FWIF_QUEUE_INDEX_0, \
		MTFW_FWIF_QUEUE_INDEX_1, \
		MTFW_FWIF_QUEUE_INDEX_2, \
		MTFW_FWIF_QUEUE_INDEX_3, \
		MTFW_FWIF_QUEUE_INDEX_4, \
		MTFW_FWIF_QUEUE_INDEX_5, \
		MTFW_FWIF_QUEUE_INDEX_6, \
		MTFW_FWIF_QUEUE_INDEX_7, \
	}

struct _MUSA_SRVINIT_APPHINTS_;
#define RTOS_VER_SUPPORT_VERIFY_MIN	40327

extern MTFW_FWIF_VERSION mtgpu_fw_version_list[MTFW_FWIF_VERSION_ITEM_MAX_NUM];

struct mtgpu_fw_memory_info {
	u32 fwif_offset;
	u32 shared_data_size;
	u32 init_mem_total_size;
	u32 ccb_queue_offset;
	u32 node_queue_offset;
	u32 trace_buf_offset;
	u32 perf_event_buf_offset;
	u32 fault_info_buf_offset;
	u32 dump_cfg_array_offset;
	u32 reg_dump_cfg_array_offset;
	u32 reg_dump_buff_offset;
	u32 gpu_util_buf_offset;
	u32 watchdog_buf_offset;
	u32 pc_sample_buf_offset;
	u32 item_pool_offset;
	u32 vgpu_ext_osid_info_offset;
	u32 vgpu_shared_info_offset;
	u32 reg_dump_buffer_size;
	u32 reg_dump_cfg_array_size;
	u32 pc_sample_buf_size;
};

struct mtgpu_fw_info {
	struct mtgpu_device_node *dev_node;

	bool fw_initialised;
	bool mcu_t1_enable;

	MTFW_FWIF *fwif;
	MTFW_CCB_QUEUE *ccb_queue_array;
	MTFW_FWIF_TRACEBUF *trace_buf;
	MTFW_FWIF_PERF_EVENT_BUF_CTL *perf_event_buf_ctl;
	u64 perf_event_buf_dev_pa;
	void *pcs_drop_cnt_buf;
	u32 pcs_drop_cnt_mcu_addr;
	MTFW_FWIF_FAULT_INFO_CTRL *fw_fault_info_ctrl;
	MTFW_FWIF_GPU_UTIL_BUF *gpu_util_buf;
	MTFW_FWIF_WATCHDOG *watchdog_buf;
	MTFW_CCB_ITEM *ccb_item_pool;
	u8 fw_coremem[MUSA_MCU_COREMEM_SIZE];

	MTFW_FWIF_DUMP_CFG *fw_dump_cfg;
	MTFW_FWIF_REG_DUMP_CFG *fw_reg_dump_cfg;
	void *fw_reg_dump_buff_addr;

	u64 ccb_pool_bitmap[MTGPU_CCB_POOL_TOTAL_LENGTH / 64];
	u64 ccb_pool_next_index[MTFW_FWIF_QUEUE_TYPE_MAX];
	struct mutex *resp_ccb_lock[MTFW_FWIF_QUEUE_TYPE_MAX];
	struct spinlock *ccb_pool_lock[MTFW_FWIF_QUEUE_TYPE_MAX];
	struct mutex *ccb_queue_lock[MTFW_FWIF_QUEUE_TYPE_MAX];
	struct mtgpu_job_item job_item[MTGPU_CCB_POOL_TOTAL_LENGTH];
	struct kmem_cache *ccb_item_para_cache;
	struct kmem_cache *ccb_item_cache;

	DEVMEM_MEMDESC *init_mem_desc;
	void *init_mem_cpu_va;
	u32 init_mem_mcu_addr;
	u32 ccb_item_pool_mcu_addr;
	u64 init_mem_cpu_pa;
	u64 init_mem_dev_pa;
	struct mtgpu_fw_memory_info fw_mem_info;

	u32 fw_log_group;
	u32 fec_log_group;

	bool disable_hwr;

	struct delayed_work *mkdir_work;
	u32 mkdir_retry_count;

	u32 gpu_util_curr_config;
	struct workqueue_struct *gpu_util_workq;
	struct delayed_work *gpu_util_dwork;
	struct mutex *gpu_util_lock;

	struct mutex *fw_reboot_lock;
	bool fw_reboot_result;

	bool fw_need_verify;
	bool fw_verified;
	u16 fw_protect_handle;
	bool bypass_mode;

	/* Used only for VGPU */
	MTFW_FWIF *fw_heap_vaddr[MTGPU_PER_MPC_USED_MAX_OSID];
	u64 fw_heap_dpa_base[MTGPU_PER_MPC_USED_MAX_OSID];
	MTFW_VGPU_SHARED_INFO *shared_info;
	void *kernel_ctx_pc_vaddr;
	void *kernel_ctx_pc_backup;
	bool force_hwr;
	bool hot_upgrading;
	bool allow_upgrade_vm;
	bool pending_upgrade;
	DEVMEM_MEMDESC * guest_fw_heap_mem_desc[MTGPU_PER_MPC_USED_MAX_OSID][VGPU_FW_HEAP_MAX_SEGMENT];
	void *guest_fw_heap_cpu_va[MTGPU_PER_MPC_USED_MAX_OSID][VGPU_FW_HEAP_MAX_SEGMENT];
	bool frontend_type_ccb[MTGPU_PER_MPC_USED_MAX_OSID];
	u32 fw_switch_flags;
	char fw_build_ver[MTFWIF_FW_BUILD_VERSION_SIZE]; /* FW build version for KMD debug */
};

struct mtgpu_fw_ops {
	void (*get_binary_name)(struct mtgpu_device *mtdev,
				struct mtgpu_fw_info *fw_info, u8 *binary_name);
	u32 (*get_fwif_offset) (void);
	int (*verify) (struct mtgpu_fw_info *fw_info, const struct firmware *fw);
	int (*load_data) (struct mtgpu_fw_info *fw_info, const struct firmware *fw);
	int (*start) (struct mtgpu_fw_info *fw_info);
	int (*reload_data) (struct mtgpu_fw_info *fw_info);
	int (*parse_data) (struct mtgpu_fw_info *fw_info, const struct firmware *fw);
	void (*soft_reset) (void __iomem *base);
	void (*stop) (struct mtgpu_fw_info *fw_info);
	void (*mtext_init) (struct mtgpu_fw_info *fw_info);
	void (*kick) (struct mtgpu_device_node *dev_node, u32 type);
	int (*wait_idle)(struct mtgpu_fw_info *fw_info);
};

int mtgpu_fw_get_queue_index_by_type(struct mtgpu_device_node *dev_node, u32 type);
void mtgpu_fw_ops_init(struct mtgpu_device_node *dev_node);
void mtgpu_fw_kick(struct mtgpu_device_node *dev_node, u32 queue_index);
int mtgpu_fw_get_fec_va_uncached(struct device *dev, u64 dev_pa, fw_va *fec_user_va_out);
int mtgpu_fw_reboot(struct mtgpu_device_node *dev_node);
int mtgpu_fw_load(struct mtgpu_device_node *dev_node, struct _MUSA_SRVINIT_APPHINTS_ *hints);
void mtgpu_fw_unload(struct mtgpu_device_node *dev_node);
int mtgpu_fw_start(struct mtgpu_device_node *dev_node);
void mtgpu_fw_stop(struct mtgpu_device_node *dev_node);
int mtgpu_fw_wait_idle(struct mtgpu_device_node *dev_node);
int mtgpu_fw_global_data_init(void);
void mtgpu_fw_global_data_deinit(void);
void mtgpu_fw_image_cache_free(void);
bool vgpu_notify_firmware_connect(struct mtgpu_device_node *dev_node);
int mtgpu_fw_load_and_verify(struct mtgpu_fw_info *fw_info);
int mtgpu_fw_verify(struct device *dev, struct ipc_msg_protect_info *protect_info);
int mtgpu_fw_init_verify_info(struct mtgpu_fw_info *fw_info, const u8 *bin, u64 bin_size,
			      u64 *src_addr, void **map_data, void **handle);
u64 mtgpu_fw_get_yuv_csc_table_base(struct mtgpu_device_node *dev_node);

#endif /* _MTGPU_FW_H_ */

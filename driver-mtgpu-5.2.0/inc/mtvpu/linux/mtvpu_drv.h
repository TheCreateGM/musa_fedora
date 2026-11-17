/*
* @Copyright   Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
* @License     Dual MIT/GPLv2
*/

#ifndef _MTVPU_DRV_H_
#define _MTVPU_DRV_H_

#include "mtgpu_defs.h"
#include "vpuapi.h"
#include "jdi.h"
#include "osfunc.h"
#include "mtvpu_conf.h"
#include "mtvpu_mon.h"
#include "mtvpu_sched.h"
#include "ioctl_vpu.h"
#include "mtvpu_mgn.h"

#ifdef VGPU_SUPPORT_HOT_UPGRADE
#include "mtgpu_hot_upgrade_keys.h"
extern bool mtgpu_is_in_upgrading(void);
extern ssize_t mtgpu_hug_data_restore(struct device *dev, void *data, u64 key, int type);
extern int mtgpu_hug_data_save(struct device *dev, void *data, size_t size, u64 key, int type);
#endif

#define INST_Q_DEPTH  COMMAND_QUEUE_DEPTH

#define WAV5_MAX_CORE  6 // for QY2
#define MAX_IRQ        8
#define MAX_IRQ_PH1    5

#define MTVPU_SEGMENT_NUM       16
#define MTVPU_SEGMENT_VM       -1

#define VPU_SMMU_MEM_BASE_MASK 0xFF00000000

/* modify duration with caution! */
#define VPU_UTIL_DURATION 300 /* ms */
#define VPU_STAT_DURATION 1000 /* ms */

#define VM_MAX_RETRY 6
#define VM_WAIT_TIME 100 /* ms */

#define VDI_THREAD_DEFAULT_TIME 200 /* ms */

/* return 1GHz(meaningless) to prevent a division zero exception when get clock failed */
#define MTVPU_GET_CLK_FAIL_FREQ  (1*1000*1000*1000)

enum host_sched_versions {
	HOST_SCHED_V1 = 1,
	HOST_SCHED_V2 = 2,
};

#ifdef SUPPORT_ION
#define ion_phys_addr_t phys_addr_t
#endif

extern u32 mtvpu_timeout_inflation;
extern u32 gpu_page_size;
extern u32 gpu_page_shift;

struct file_operations;
struct platform_device;
struct drm_device;
struct drm_ioctl_desc;
struct wait_queue_head;
struct spinlock;
struct semaphore;
struct mutex;
struct list_head;
struct timer_list;
struct drm_file;
struct drm_gem_object;
struct mtgpu_gem_object;
struct vm_area_struct;
struct dma_buf;
struct iommu_group;
struct iommu_domain;
struct iova_domain;
struct devfreq;
struct devfreq_simple_ondemand_data;

struct mt_chip;

struct mt_pool {
	struct mtgpu_gem_object *mtgpu_obj;
	dma_addr_t offset;
	u32 count;
	size_t size;
};

struct mt_intr_map {
	u32 core_idx;
	u32 intr_inst;
	u32 intr_reason;
	u32 err_reason;
	u32 tick_frame;
	union {
		struct {  // 517 result
			u32 linear_y_addr;
			u32 linear_y_addr_ext;
		};
		struct {  // 627 result
			u32 stream_size;
			u32 encode_state;
		};
		u64 frameAddr;	// smj result
	};
	mtjpu_core_idx jidx;
	void* semaphore;
	u64 dstSemaWaitValue;
};

struct mt_vpu_irq_ctx {
	struct mt_chip *chip;
	struct spinlock *irq_lock;
	struct work_struct *work[INST_MAX_SIZE][COMMAND_QUEUE_DEPTH];
	struct work_struct *critical_work;
	int work_idx[INST_MAX_SIZE];
	int work_cnt;
	int critical_work_cnt;
};

struct mt_vpu_work_data {
	struct mt_intr_map map;
	void *ctx;
};

struct mt_node {
	struct drm_gem_object *obj;
	u64 local_phys_addr;
	u64 unified_phys_addr;
	u64 size;
	void *handle;
#ifdef SUPPORT_ION
	struct dma_buf *ion_buf;
#endif
	u32 pool_id;
	u64 private_data;
};

struct umd_alloc_buffer_single {
	u64 dev_addr;
	u64 virt_addr; /* for smmu use */
	u64 size;
	bool used;
};

struct umd_alloc_buffer_union{
	u64 start_pa;
	u64 virt_addr; /* for smmu use */
	u32 total_size;
	u32 used_size;
};

/* the instance mutex can guaranty safe. so we don't use lock. */
struct mt_vpu_cmd {
	u64 sur_addr;
	u64 sem;

	/* Todo remove it later*/
	volatile bool addr_blocked;
	struct wait_queue_head *addr_wait;
	int status;
};

struct mt_vpu_cmd_queue {
	struct mt_vpu_cmd cmd[INST_Q_DEPTH];
	int rd_idx;
	int wr_idx;
	int count;
	u64 pid;
	bool inst_blocked;
	struct wait_queue_head *inst_wait;
	void *handle;		// VpuHandle / JpgDecHandle
	struct mt_logic_handle *mgn_ctx;
	struct mt_virm *vm;
	struct spinlock *lock;
};

struct mt_vpu_dbg_stream {
	char *buffer;
	size_t size;
	size_t pos;
	u32 coreIdx;
	//struct mutex *lock; // enable this lock when required
};

struct mt_inst_bo_list {
	struct drm_gem_object *obj;
	struct list_head node;
};

struct mt_core {
	int idx;
	int irq[MAX_IRQ];
	int available;
	int inited;
	int reload_flag;  // bit0 set means hang need reload, bit1 set: means reset ok, can reload fw
	int product_id;

	void *regs;

	u32 bin_offset;
	u32 bin_size;

	struct mt_node *common_node; /* common mem */
	struct vpu_instance_pool pool; /* vdi mem */
	struct jpu_instance_pool jpu_pool;

	struct device *pm_dev;

	int mem_group_id;
	u64 mem_group_base;
	struct drm_device *drm;
#ifdef SUPPORT_ION
	int heap_id;
	struct ion_device *ion;
#endif
	bool serve_all;
	struct wait_queue_head *core_wait;
	struct wait_queue_head *inst_wait[INST_MAX_SIZE];
	char irq_name[32];

	int core_intr_reason;
	int core_wait_reason;
	int inst_intr_reason[INST_MAX_SIZE];
	int inst_wait_reason[INST_MAX_SIZE];
	int inst_bs_size_ret[INST_MAX_SIZE];    /* > 0: bs size; < 0: enc pic failed; == 0: needs query get result */
	int inst_err_reason[INST_MAX_SIZE];		/* 0:success other: error reason */
	Uint32 fw_version;

	struct mutex *open_lock;
	struct mutex *rpm_lock;
	struct mutex *regs_lock;

	int drm_ids[INST_MAX_SIZE];
	u32 pool_ids[INST_MAX_SIZE];
	struct mtvpu_mmu_ctx *mmu_ctx[INST_MAX_SIZE];

	struct mt_chip *priv;
	void *bak_addr;

	u64 soc_core_freq;  /* for soc_mode use */
	u64 core_max_freq;  /* for none soc_mode use */
	/* The number of ticks used by the enc pic on the VPU within one second */
	/* In MPC mode, one core may serve multiple drm nodes; */
	/* Thus we should calculate cycles for each drm node */
	u64 core_drm_cycle[MTGPU_CORE_COUNT_MAX];

	u32 inst_cnt;
	/* add a new attribute for describing the loading when cores have the same utilization */
	u64 loading_surface;
	struct mt_inst_info inst_info[INST_MAX_SIZE];
	struct mt_inst_extra inst_extra[INST_MAX_SIZE];

	struct mutex *inst_lock[INST_MAX_SIZE];
	struct list_head bo_list[INST_MAX_SIZE];

	u32 log_read_pos;
	u32 dump_flag;
	u32 reset_cnt;

	/* dump debug log */
	struct work_struct *dbg_work;
	struct mt_vpu_dbg_stream *dbg_strm;

	u32 queue_len;
	struct workqueue_struct *irq_workqueue;
	int (*pfn_irq)(struct mt_chip *chip, struct mt_core *core);
	struct mt_vpu_irq_ctx *work_ctx;
	struct mt_vpu_cmd_queue *cmd_q[INST_MAX_SIZE];
	u64 vpu_time_per_sec;
	int inst_failed[INST_MAX_SIZE];
	u32 inc_inst_cnt;
};

struct mt_file {
	struct list_head head; /* opened list */
	struct mutex *file_lock;
	struct mtvpu_mmu_ctx *mmu_ctx;
	struct list_head sema_head;
};

struct mt_host_pool {
	u32 pool_id;
	bool used;
};

struct mt_codec_limit {
	u32 dec_max_width;
	u32 dec_max_height;
	u32 enc_max_width;
	u32 enc_max_height;
	u32 max_enc_num;
	u32 max_dec_num;
};

struct mt_virm {
	int vm_id;
	u32 sched_step;	/* represents the scheduled codec capacity for this VM */
	u32 sched_max;	/* represents the total available schedule codec capacity for this VM */

	u32 dec_index;
	u32 enc_index;

	struct vm_shared_mem *mem;
	bool is_share_mem_vram;
	u32 share_mem_size;

	struct mt_file file;

	RenderTargetInfo render;
	struct list_head list;

	u32 vm_group;
	u64 vcore_base;
	u64 vcpu_base;
	u32 vcpu_multiplier;

	/* For M1000 Host */
	u32 dec_core_idx;
	u32 enc_core_idx;

	uintptr_t dec_close_record;
	uintptr_t enc_close_record;

	u32 sched_count;

	u32 remain_task[CORE_MAX_SIZE];

	struct mt_logic_handle dec_ctx[INST_MAX_SIZE];
	struct mt_logic_handle enc_ctx[INST_MAX_SIZE];

	bool is_need_rollback;
};

struct mt_chip {
	int idx;
	struct mt_conf conf;
	struct pci_dev *pci_dev;
	struct device *parent;
	struct device *dev;

	struct drm_device *drm_host; /* for host use */

	int driver_mode;

	u32 inst_cnt;
	struct mutex *inst_cnt_lock;

	bool soc_mode;
	bool resizebar_off;

	int mem_group_id[MTVPU_SEGMENT_NUM];
	u64 mem_group_base[MTVPU_SEGMENT_NUM];
	u64 mem_group_size[MTVPU_SEGMENT_NUM];
	int mem_group_cnt;

	struct spinlock *shared_mem_lock; /* for linux guest shared memory operations */

	struct spinlock *pool_lock;
	struct mt_pool pool_array[MEM_POOL_MAX_SIZE];
	int pool_idx;

	int mpc_drm_cnt;
#ifdef SUPPORT_ION
	struct ion_device *ions[MTGPU_CORE_COUNT_MAX];
#endif
	struct drm_device *drms[MTGPU_CORE_COUNT_MAX]; /* for native use */
	int mpc_group_ids[MTGPU_CORE_COUNT_MAX];
	struct spinlock *mpc_lock;
	int core_group_inst_cnt[MTVPU_SEGMENT_NUM];

	void *vaddr;
	u64 mmu_base_va;
	u64 bar_base;

	struct mt_core core[CORE_MAX_SIZE];
	int vm_count;
	struct mt_virm vm;
	struct mutex *vm_locks[HOST_OSID_SIZE];
	struct task_struct *read_msg_threads;

	/* sched 2.0. */
	struct semaphore *host_thread_semas[CORE_MAX_SIZE + 1]; //1 vpu_host_dispatch
	struct task_struct *host_threads[CORE_MAX_SIZE + 1]; //1 vpu_host_dispatch
	struct spinlock *host_task_lock[CORE_MAX_SIZE];
	struct list_head host_task_head[CORE_MAX_SIZE];
	u32 vdi_thread_timeout; /* in ms */

	// TODO:may use vm_node(mgn_data, struct mt_vrim) support in parallel
	struct mt_virm *vm_node[HOST_OSID_SIZE];

	struct mt_codec_limit codec_limit;

	struct dentry *debugfs;

	struct timer_list *util_timer; /* for utilization calculation */
	struct timer_list *stat_timer; /* for frame_rate/bitrate statistics */

	struct semaphore *jpu_core_sema;

	struct mtvpu_mmu_ctx *mmu_ctx;	/* mmu context for PH codec cores, all cores share the same context */
	bool mmu_inited;

	struct iommu_group *io_group;
	struct iommu_domain *io_domain;
	struct iova_domain *iova_domain[CORE_MAX_SIZE];

	/* devfs for soc */
	struct devfreq *dev_freq;
	struct devfreq_dev_profile *profile;
	struct devfreq_simple_ondemand_data *devfreq_data;
	u64 curr_freq;
	u32 freq_min;		/* MHz */
	u32 freq_max;		/* MHz */
	void __iomem *m1000_shared_mem;
	void __iomem *vid_crg_reg;
	atomic_t perf_mode;	   /* perf mode indicator */

	u32 fixed_decode_core; /* for soc transcode use */
	int auto_reset_en;     /* Internal testing scenarios do not allow automatic reset, which may result in missing critical issues */
	struct mutex *reset_lock;
	u32 read_guest_log;
	u32 dump_guest_log;
	u32 max_abi_version;
	u64 max_abi_checksum[2];
	const struct firmware *fw;
	struct mtvpu_header vpu_header;
	bool vpubin_is_ok;
	int debug_mgn;
	void *mgn_vm_data[HOST_OSID_SIZE];
};

struct mt_open {
	int coreIdx;
	int instIdx;
	int isDecoder;
	VpuHandle handle;

	struct list_head list;
};

struct mtvpu_gem_priv {
	struct vm_area_struct *vma;
	struct mtvpu_mmu_ctx *mmu_ctx;
	u64 dev_phy_addr;
	void *pmr;
	void *virt_addr;
	u64 priv_data; /* for vmap use in smmu */
#if defined(OS_STRUCT_DMA_BUF_MAP_EXIST) || defined(OS_STRUCT_IOSYS_MAP_EXIST)
	struct iosys_map *map; /* for ion_buffer vmap */
#endif
	u32 flags; /* for ion buffer */
};

struct mtvpu_sema_list
{
	struct list_head list;
	struct semaphore *sema;
};

extern int enable_pm_mtvpu_backup_device_memory;

void vpu_ref_core_nolock(struct mt_chip *chip, int idx, struct mt_virm *vm);
void vpu_ref_core_lock(struct mt_chip *chip, int idx, struct mt_virm *vm, bool is_enc);
void vpu_unref_core_nolock(struct mt_chip *chip, int idx, struct mt_virm *vm);
void vpu_unref_core_lock(struct mt_chip *chip, int idx, struct mt_virm *vm, bool is_enc);

int vpu_fill_drm_ioctls(struct drm_ioctl_desc *dst, int num);
int vpu_host_run_ioctl(int ioctl, struct drm_device *drm, void *data, struct mt_virm *vm);

int jpu_request_core(struct drm_device *drm);
void jpu_release_core(struct drm_device *drm, int idx);
int jpu_request_core_v2(struct drm_device *drm);
void jpu_release_core_v2(struct drm_device *drm, int idx);
void jpu_mmu_init(struct mt_chip *chip);
int make_jpu_idx(int idx, int has_a35, int a35_idx);

int read_guest_msg_thread(void *arg);

#endif /* _MTVPU_DRV_H_ */

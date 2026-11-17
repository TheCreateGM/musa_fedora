/*
 * @Copyright   Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License     Dual MIT/GPLv2
 */

#ifndef __VGPU_SHM_H__
#define __VGPU_SHM_H__

#include "vchannel.h"
#include "vgpu_version.h"

/* virtual display max num */
#define MTGPU_VGPU_DISPLAY_NUM_MAX 2

enum {
	VGPU_SHM_TYPE_SHARED_STATE,
	VGPU_SHM_TYPE_IPC_RINGBUF,
	VGPU_SHM_TYPE_BS_BUF_GUEST_TO_HOST_RD,
	VGPU_SHM_TYPE_BS_BUF_HOST_TO_GUEST_RD,

	VGPU_SHM_TYPE_MAX,
};

enum {
	VGPU_FW_HOT_UPGRADE_INIT = 0,
	VGPU_FW_HOT_UPGRADE_BEGIN,
	VGPU_FW_HOT_UPGRADE_FAILED,
};

union vgpu_shm_update_cmd {
	uint64_t cmd;

	struct {
		uint64_t attach : 1;
		uint64_t type : 8;
		uint64_t size : 32;
		uint64_t __reserved : 23;
	};
};

struct vgpu_shm_ops {
	void (*init)(union vgpu_shm_update_cmd *cmd, uint64_t shm_type);
	bool (*attach)(void *data, void *priv, uint64_t shm_type);
	void (*detach)(void *priv, uint64_t shm_type);
};

struct vgpu_shm_region {
	void *data;
	uint32_t size;
	bool initialized;
	struct vgpu_shm_ops ops;
#ifdef __linux__
	/* for host only */
	int host_status;
	atomic_t refcount;
	struct completion *free;
#endif
};

#define VGPU_NODE_NUM_MAX 8
#define VPU_NODE_NUM_MAX  2

typedef enum _VGPU_CMD_TYPE_ {
    VGPU_CMD_TYPE_NORMAL = 0,
    VGPU_CMD_TYPE_PREEMPTION,
    VGPU_CMD_TYPE_MAX,
} VGPU_CMD_TYPE;

struct vsync_time_info
{
	volatile uint64_t last_vsync_time; // us
	volatile uint64_t gap_vsync_time;
	volatile bool vsync_on;
};

struct vgpu_cmd_timestamp
{
	volatile uint32_t processId;
	volatile uint32_t fenceId;
	volatile uint64_t socExecUs;
	volatile uint64_t fwScheTime;
	volatile uint64_t gpuDealTime;
	volatile uint64_t guestDealIrqTime;
	volatile uint64_t reserved[8];
};

struct vgpu_timestamp_info
{
	struct vgpu_cmd_timestamp vgpu_cmd_ts_info[VGPU_NODE_NUM_MAX][VGPU_CMD_TYPE_MAX];
	struct vgpu_cmd_timestamp vgpu_tdr_ts_info[VGPU_NODE_NUM_MAX];
	struct vsync_time_info    vsync_time[MTGPU_VGPU_DISPLAY_NUM_MAX];
};

struct exec_info
{
	uint64_t cmd_count;
	uint64_t max_exec_time;
	uint64_t averg_exec_time;
	uint64_t total_exec_time;
};

struct tdr_node_info {
	uint32_t node;
	uint64_t execTime;
};

struct vpu_cmd_info {
	struct tdr_node_info tdr_node[VPU_NODE_NUM_MAX];
	struct exec_info vpu_node_exec_info[VPU_NODE_NUM_MAX];
};

struct vgpu_cmd_exec_info {
	/* cause windows tdr node info */
	uint32_t idx;
	struct tdr_node_info tdr_node[VGPU_NODE_NUM_MAX];

	/* command executute time */
	struct exec_info vgpu_node_exec_info[VGPU_NODE_NUM_MAX][VGPU_CMD_TYPE_MAX];

	struct vpu_cmd_info vpu_cmd_exec_info;
};

#define VGPU_FBE_MAX_ENTRIES (512)

struct fbe_info {
	volatile uint64_t hallocation;  // host <<-- guest
	volatile uint32_t format;       // host <<-- guest
	volatile uint32_t width;        // host <<-- guest
	volatile uint32_t height;       // host <<-- guest
	volatile uint32_t stride;       // host <<-- guest
	volatile uint32_t size;         // host <<-- guest
	volatile uint32_t pitch;        // host <<-- guest
	volatile uint32_t page_size;    // host <<-- guest

	volatile uint32_t need_update;  // host -->> guest
	volatile uint32_t finish;       // host <<-- guest
	volatile uint32_t guest_wo;     // host <<-- guest
	volatile uint32_t host_ro;      // host -->> guest
	volatile bool     visiable;     // host <<-- guest
	volatile bool     once_finish;  // host -->> guest
	volatile uint64_t first_addr;   // host <<-- guest
	volatile uint8_t  reserved[24];
};

struct fbe_entry {
	/* VGPU_FBE_MAX_ENTRIES */
	volatile uint32_t count;
	volatile uint64_t entries[VGPU_FBE_MAX_ENTRIES];
	volatile uint8_t  reserved[32];
};

#define VGPU_GUEST_OS_TYPE_WINDOWS  0
#define VGPU_GUEST_OS_TYPE_LINUX    1
#define VGPU_GUEST_OS_DETAIL_SIZE   128
struct guest_os_version {
	uint32_t type;
	uint32_t major;
	uint32_t minor;
	uint32_t build_number;
	uint8_t detail[VGPU_GUEST_OS_DETAIL_SIZE];
};

#define MTGPU_VGPU_MIG_GUEST_FLAG_HAS_CMD_DEFER    BIT_ULL(0)
#define MTGPU_VGPU_MIG_GUEST_FLAG_PROCESS_CREATING BIT_ULL(1)

struct vsync_status {
	volatile bool vsync_on;
	volatile bool vsync_irq;
};

/* shared struct definition between host and guest START */
/* VGPU_SHM_TYPE_SHARED_STATE */
struct vgpu_shared_state {
	volatile bool gpu_normal;

	/* interrupt status */
	volatile uint32_t vintr_status;
	volatile uint64_t vintr_ack_cnt;

	/* for vgpu live migration */
	/* guest read, host write */
	volatile uint64_t mig_host_stage;
	/* guest write, host read */
	union {
		volatile uint64_t mig_guest_flag;
		volatile uint64_t mig_can_stop_guest; // deprecated
	};
	/* gpu used memory size */
	volatile uint64_t gpu_used_mem_size;
	/* vpu used memory size */
	volatile uint64_t vpu_enc_used_mem_size;
	volatile uint64_t vpu_dec_used_mem_size;

	volatile bool is_mtml_get_sysmem;

	struct vgpu_cmd_exec_info vgpu_cmd_exec_info;

	volatile uint32_t linux_host_hwr_stage;

	/* for fw hot upgrade */
	volatile uint32_t fw_hot_upgrade_stage;

	volatile bool wait_incore_migrate;

	struct guest_os_version os_version;

	volatile uint32_t host_hot_upgrade_stage;

	struct vsync_status vsync_stat[MTGPU_VGPU_DISPLAY_NUM_MAX];

	/* virtual display framebuffer export */
	struct fbe_info fbe_info;
	struct fbe_entry fbe_entry;

	volatile uint64_t trans_gpa_addr;

	volatile uint32_t swap_host_stage;
	volatile uint32_t swap_guest_stage;

	struct vgpu_timestamp_info ts_info;
};

/* VGPU_SHM_TYPE_IPC_RINGBUF */
struct vgpu_ipc_ringbuf {
	struct vqueue vqs[VQUEUE_DIRECTION_MAX_COUNT];
};

#define VGPU_BYTE_STREAM_BUF_SIZE (4096)
/* byte stream buffer shm definition */
struct vgpu_bs_buf {
	uint8_t data[VGPU_BYTE_STREAM_BUF_SIZE];
};

enum {
	VGPU_COMPAT_CHECK_MODE_DISABLE,
	VGPU_COMPAT_CHECK_MODE_VERSION_LIST,
	VGPU_COMPAT_CHECK_MODE_STRICTLY_MATCH,

	VGPU_COMPAT_CHECKE_MAX,
};

enum {
	VGPU_DAEMON_UPGRADE_MODE_DISABLE,
	VGPU_DAEMON_UPGRADE_MODE_USER_INTERACTIVE,

	VGPU_COMPAT_UPGRADE_MAX,
};

union vgpu_upgrade_common {
	uint64_t cmd;
	uint64_t daemon_upgrade_mode;
	uint64_t guest_pkg_version;
};
typedef union vgpu_upgrade_common vgpu_upgrade_common_t;


/* shared struct definition between host and guest END */

#ifndef __linux__
typedef union  vgpu_shm_update_cmd      VGPU_SHM_UPDATE_CMD;
typedef struct vgpu_shm_ops             VGPU_SHM_CALLBACKS;
typedef struct vgpu_shm_region          VGPU_SHM_REGION;

typedef struct vgpu_shared_state        VGPU_SHARED_STATE;
typedef struct vgpu_ipc_ringbuf         VGPU_IPC_RINGBUF;

typedef struct vgpu_cmd_exec_info       VGPU_CMD_EXEC_INFO;
typedef struct vpu_cmd_info             VPU_CMD_INFO;
typedef struct exec_info                EXEC_INFO;
typedef struct tdr_node_info            TDR_NODE_INFO;
typedef struct vgpu_bs_buf              VGPU_BS_BUF;
typedef struct vgpu_cmd_timestamp	VGPU_CMD_TIMESTAMP;

typedef struct vgpu_release_version     VGPU_RELEASE_VERSION;
typedef union vgpu_compat_check_cmd     VGPU_COMPAT_CHECK_CMD;
typedef union vgpu_upgrade_common       VGPU_UPGRADE_COMMON;
#endif /* __linux__ */

#endif /* __VGPU_SHM_H__ */

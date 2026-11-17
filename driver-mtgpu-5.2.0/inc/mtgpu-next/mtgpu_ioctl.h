/*
 * @Copyright   Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License     Dual MIT/GPLv2
 */

#ifndef __MTGPU_IOCTL_H__
#define __MTGPU_IOCTL_H__

struct drm_device;
struct drm_file;
struct mtgpu_device_node;

typedef int (*mtgpu_ioctl_func_t)(struct drm_device *drm_dev, void *data, struct drm_file *file);

struct mtgpu_ioctl_desc {
	const char *name;
	mtgpu_ioctl_func_t func;
        uint64_t checksum;
	uint64_t data_size;
};

extern const struct mtgpu_ioctl_desc core_cmd_ioctl_desc[MTGPU_CORE_CMD_MAX];
extern const struct mtgpu_ioctl_desc query_cmd_ioctl_desc[MTGPU_QUERY_CMD_MAX];
extern const struct mtgpu_ioctl_desc bo_cmd_ioctl_desc[MTGPU_BO_CMD_MAX];
extern const struct mtgpu_ioctl_desc vm_cmd_ioctl_desc[MTGPU_VM_CMD_MAX];
extern const struct mtgpu_ioctl_desc sync_cmd_ioctl_desc[MTGPU_SYNC_CMD_MAX];
extern const struct mtgpu_ioctl_desc job_cmd_ioctl_desc[MTGPU_JOB_CMD_MAX];
extern const struct mtgpu_ioctl_desc perf_cmd_ioctl_desc[MTGPU_PERF_CMD_MAX];
extern const struct mtgpu_ioctl_desc gdb_cmd_ioctl_desc[MTGPU_GDB_CMD_MAX];
extern const struct mtgpu_ioctl_desc kms_cmd_ioctl_desc[MTGPU_KMS_CMD_MAX];

/* FIXME: workaround for KMD1.5 compilation error */
#define MTGPU_SYNC_CMD_TIMELINE_CREATE_CHECKSUM		0
#define MTGPU_SYNC_CMD_TIMELINE_DESTROY_CHECKSUM 	0
#define MTGPU_SYNC_CMD_TIMELINE_READ_CHECKSUM 		0
#define MTGPU_SYNC_CMD_FENCE_WAIT_CHECKSUM 		0
#define MTGPU_SYNC_CMD_FENCE_TO_FD_CHECKSUM 		0

#define MTGPU_JOB_CMD_CONTEXT_CREATE_CHECKSUM 		0
#define MTGPU_JOB_CMD_CONTEXT_DESTROY_CHECKSUM 		0
#define MTGPU_JOB_CMD_SUBMIT_CHECKSUM 			0
#define MTGPU_JOB_CMD_DMA_TRANSFER_CHECKSUM 		0
#define MTGPU_JOB_CMD_OBJECT_CREATE_CHECKSUM 		0
#define MTGPU_JOB_CMD_OBJECT_DESTROY_CHECKSUM 		0
#define MTGPU_JOB_CMD_NOTIFY_QUEUE_UPDATE_CHECKSUM 	0

#define MTGPU_SYNC_CMD_TIMELINE_CREATE_STRUCT_SIZE 	0
#define MTGPU_SYNC_CMD_TIMELINE_DESTROY_STRUCT_SIZE 	0
#define MTGPU_SYNC_CMD_TIMELINE_READ_STRUCT_SIZE 	0
#define MTGPU_SYNC_CMD_FENCE_WAIT_STRUCT_SIZE 		0
#define MTGPU_SYNC_CMD_FENCE_TO_FD_STRUCT_SIZE 		0

#define MTGPU_IOCTL_DESC(cmd, func) \
	[cmd] = {#cmd, func, cmd##_##CHECKSUM, cmd##_##STRUCT_SIZE}

int mtgpu_ioctl(struct drm_device *drm_dev, void *data, struct drm_file *file);
void mtgpu_ioctl_global_disable(void);
void mtgpu_ioctl_global_enable(void);
void mtgpu_ioctl_device_disable(struct mtgpu_device_node *dev_node);
void mtgpu_ioctl_device_enable(struct mtgpu_device_node *dev_node);
void mtgpu_ioctl_block_access(struct mtgpu_device_node *dev_node);
void mtgpu_ioctl_unblock_access(struct mtgpu_device_node *dev_node);
int mtgpu_ioctl_access_check(struct mtgpu_device_node *dev_node);
bool mtgpu_ioctl_wait_completed_timeout(struct mtgpu_device_node *dev_node);

#endif /* __MTGPU_IOCTL_H__ */

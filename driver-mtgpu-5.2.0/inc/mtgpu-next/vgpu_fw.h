/*
 * @Copyright Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License Dual MIT/GPLv2
 */

#ifndef _VGPU_FW_H_
#define _VGPU_FW_H_

#include "musadevice.h"
#include "mtgpu_fw.h"
#include "mtgpu_fw_job.h"

struct mtgpu_device_node;

#if (MUSA_NUM_OS_SUPPORTED > 1)
#define VGPU_FW_DEFAULT_LOG_GROUP	\
	(BIT(MTFW_GROUP_ERR) |	\
	BIT(MTFW_GROUP_FUNC) |	\
	BIT(MTFW_GROUP_REG) |	\
	BIT(MTFW_GROUP_MMU) |	\
	BIT(MTFW_GROUP_CORE) |	\
	BIT(MTFW_GROUP_DRV) |	\
	BIT(MTFW_GROUP_SCH) |	\
	BIT(MTFW_GROUP_NODE) |	\
	BIT(MTFW_GROUP_WORK) |	\
	BIT(MTFW_GROUP_GPU) |	\
	BIT(MTFW_GROUP_PERF) |	\
	BIT(MTFW_GROUP_PFM) |	\
	BIT(MTFW_GROUP_POW) |	\
	BIT(MTFW_GROUP_CSW) |	\
	BIT(MTFW_GROUP_CCB) |	\
	BIT(MTFW_GROUP_CTX) |	\
	BIT(MTFW_GROUP_SUBM) |	\
	BIT(MTFW_GROUP_FSCH) |	\
	BIT(MTFW_GROUP_MAIN))

char *vgpu_get_fw_log_dict_file_name(struct mtgpu_device *mtdev, struct mtgpu_fw_info *fw_info);
void vgpu_init_win_trace_buffer(struct mtgpu_fw_info *fw_info);
int vgpu_init_win_fwif(struct mtgpu_fw_info *fw_info);
int vgpu_init_guest_fwif_linux(struct mtgpu_fw_info *fw_info);
int vgpu_fw_info_init(struct mtgpu_fw_info *fw_info);
void vgpu_fw_info_init_rm(struct mtgpu_fw_info *fw_info, void *ext_osid_info, void *shared_info);
void vgpu_shared_info_init(struct mtgpu_fw_info *fw_info);
void vgpu_set_fw_info(struct mtgpu_device_node *dev_node);
void vgpu_free_fw_info(struct mtgpu_device_node *dev_node);
void vgpu_fw_log_set_group(struct mtgpu_device_node *dev_node, u32 log_group_id);
int vgpu_ccb_submit_gp_command(struct mtgpu_device_node *dev_node, MTFW_SUBMISSION_CMD_TYPE cmd_type,
				MTFW_CCB_ITEM_PARA *item_para, bool sync);
void vgpu_fw_check_node_resp_queue(struct mtgpu_device_node *data);
bool vgpu_vm_is_idle(struct mtgpu_fw_info *fw_info, u32 osid);
int vgpu_reload_fw(struct mtgpu_fw_info *fw_info, u32 mpc_id);
bool vgpu_fw_guest_connect(struct mtgpu_device_node *dev_node);
bool vgpu_fw_guest_disconnect(struct mtgpu_device_node *dev_node);
void vgpu_fw_guest_gpu_reset(struct mtgpu_device_node *dev_node);
bool ccb_is_empty(const MTFW_CCB_QUEUE *ccb_queue);
bool vgpu_reboot_new_fw(struct mtgpu_fw_info *fw_info);
bool vgpu_fw_is_upgrading(struct mtgpu_fw_info *fw_info);
void vgpu_fw_info_lock(struct mtgpu_fw_info *fw_info);
void vgpu_fw_info_unlock(struct mtgpu_fw_info *fw_info);
#else
#define VGPU_FW_DEFAULT_LOG_GROUP 0
static inline char *vgpu_get_fw_log_dict_file_name(struct mtgpu_device *mtdev,
						   struct mtgpu_fw_info *fw_info)
{
	MT_UNREFERENCED_PARAMETER(mtdev);
	MT_UNREFERENCED_PARAMETER(fw_info);
	return NULL;
}

static inline void vgpu_init_win_trace_buffer(struct mtgpu_fw_info *fw_info)
{
	MT_UNREFERENCED_PARAMETER(fw_info);
}

static inline void vgpu_fw_log_set_group(struct mtgpu_device_node *dev_node, u32 log_group_id)
{
	MT_UNREFERENCED_PARAMETER(dev_node);
	MT_UNREFERENCED_PARAMETER(log_group_id);
}

static inline int vgpu_init_win_fwif(struct mtgpu_fw_info *fw_info)
{
	MT_UNREFERENCED_PARAMETER(fw_info);
	return 0;
}

static inline int vgpu_init_guest_fwif_linux(struct mtgpu_fw_info *fw_info)
{
	MT_UNREFERENCED_PARAMETER(fw_info);
	return 0;
}

static inline int vgpu_fw_info_init(struct mtgpu_fw_info *fw_info)
{
	MT_UNREFERENCED_PARAMETER(fw_info);
	return 0;
}

static inline void vgpu_fw_info_init_rm(struct mtgpu_fw_info *fw_info, void *ext_osid_info, void *shared_info)
{
	MT_UNREFERENCED_PARAMETER(fw_info);
	MT_UNREFERENCED_PARAMETER(ext_osid_info);
	MT_UNREFERENCED_PARAMETER(shared_info);
}

static inline void vgpu_shared_info_init(struct mtgpu_fw_info *fw_info)
{
	MT_UNREFERENCED_PARAMETER(fw_info);
}

static inline void vgpu_set_fw_info(struct mtgpu_device_node *dev_node)
{
	MT_UNREFERENCED_PARAMETER(dev_node);
}

static inline void vgpu_free_fw_info(struct mtgpu_device_node *dev_node)
{
	MT_UNREFERENCED_PARAMETER(dev_node);
}

static inline int vgpu_ccb_submit_gp_command(struct mtgpu_device_node *dev_node, MTFW_SUBMISSION_CMD_TYPE cmd_type,
						MTFW_CCB_ITEM_PARA *item_para, bool sync)
{
	MT_UNREFERENCED_PARAMETER(dev_node);
	MT_UNREFERENCED_PARAMETER(cmd_type);
	MT_UNREFERENCED_PARAMETER(item_para);
	MT_UNREFERENCED_PARAMETER(sync);
	return 0;
}

static inline void vgpu_fw_check_node_resp_queue(struct mtgpu_device_node *data)
{
	MT_UNREFERENCED_PARAMETER(data);
}

static inline int vgpu_reload_fw(struct mtgpu_fw_info *fw_info, u32 mpc_id)
{
	MT_UNREFERENCED_PARAMETER(fw_info);
	MT_UNREFERENCED_PARAMETER(fw_info);
	return 0;
}

static inline bool vgpu_fw_guest_connect(struct mtgpu_device_node *dev_node)
{
	MT_UNREFERENCED_PARAMETER(dev_node);

	return true;
}

static inline bool vgpu_fw_guest_disconnect(struct mtgpu_device_node *dev_node)
{
	MT_UNREFERENCED_PARAMETER(dev_node);

	return true;
}

static inline void vgpu_fw_guest_gpu_reset(struct mtgpu_device_node *dev_node)
{
	MT_UNREFERENCED_PARAMETER(dev_node);
}

static inline bool ccb_is_empty(const MTFW_CCB_QUEUE *ccb_queue)
{
	MT_UNREFERENCED_PARAMETER(ccb_queue);
	return true;
}

static inline bool vgpu_reboot_new_fw(struct mtgpu_fw_info *fw_info)
{
	MT_UNREFERENCED_PARAMETER(fw_info);
	return false;
}

static inline bool vgpu_fw_is_upgrading(struct mtgpu_fw_info *fw_info)
{
	MT_UNREFERENCED_PARAMETER(fw_info);
	return false;
}

static inline void vgpu_fw_info_lock(struct mtgpu_fw_info *fw_info)
{
	MT_UNREFERENCED_PARAMETER(fw_info);
}

static inline void vgpu_fw_info_unlock(struct mtgpu_fw_info *fw_info)
{
	MT_UNREFERENCED_PARAMETER(fw_info);
}

#endif

#endif /* _VGPU_FW_H_ */

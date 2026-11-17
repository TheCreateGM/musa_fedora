/*
 * @Copyright Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License Dual MIT/GPLv2
 */

#ifndef __MTGPU_DM_KILL_H__
#define __MTGPU_DM_KILL_H__

int mtgpu_dm_kill_init(struct mtgpu_device_node *dev_node);
void mtgpu_dm_kill_deinit(struct mtgpu_device_node *dev_node);
int mtgpu_dm_kill_vm_map(struct mtgpu_device_node *dev_node, struct mtgpu_vm_context *vm_ctx);
void mtgpu_dm_kill_vm_unmap(struct mtgpu_vm_context *vm_ctx);

#endif /* __MTGPU_DM_KILL_H__ */

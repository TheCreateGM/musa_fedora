/*
 * @Copyright Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License Dual MIT/GPLv2
 */

#ifndef __MTGPU_PH1_CMP_PATCH_H__
#define __MTGPU_PH1_CMP_PATCH_H__

struct mtgpu_device_node;

int mtgpu_ph1_cmp_patch_init(struct mtgpu_device_node *dev_node);
void mtgpu_ph1_cmp_patch_deinit(struct mtgpu_device_node *dev_node);
int mtgpu_ph1_cmp_patch_vm_map(struct mtgpu_device_node *dev_node);
void mtgpu_ph1_cmp_patch_vm_unmap(struct mtgpu_device_node *dev_node);

#endif /* __MTGPU_PH1_CMP_PATCH_H__ */


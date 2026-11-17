/*
 * @Copyright Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License Dual MIT/GPLv2
 */

#ifndef _MTVPU_VM_H_
#define _MTVPU_VM_H_

struct drm_device;

int mtvpu_get_fw_ctx_pc_root(struct drm_device *drm, u64 *fw_ctx_pc_root);

#endif /* _MTVPU_VM_H_*/

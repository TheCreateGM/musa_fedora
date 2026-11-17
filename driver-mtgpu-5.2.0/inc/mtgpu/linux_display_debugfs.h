/*
 * @Copyright   Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License     Dual MIT/GPLv2
 */

#ifndef __MTDISPC_DRM_DEBUGFS_H__
#define __MTDISPC_DRM_DEBUGFS_H__

struct mtdisp_dp;
struct mtdisp_dispc;

void mtdisp_dp_debugfs_create_files(struct mtdisp_dp *dp);
void mtdisp_dp_debugfs_remove_files(struct mtdisp_dp *dp);

void mtdisp_dispc_debugfs_create_files(struct mtdisp_dispc *dispc);
void mtdisp_dispc_debugfs_remove_files(struct mtdisp_dispc *dispc);

#endif /* __MTDISPC_DRM_DEBUGFS_H__ */

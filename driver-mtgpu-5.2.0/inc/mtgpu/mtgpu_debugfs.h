/*
 * @Copyright   Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License     Dual MIT/GPLv2
 */

#ifndef __MTGPU_DEBUGFS_H__
#define __MTGPU_DEBUGFS_H__

#include "os-interface.h"

struct file;
struct seq_file;
struct mtgpu_device_node;

/**
 * mtgpu_debugfs_init - Initialize all debugfs entries for a device
 * @dev_node: Pointer to mtgpu device node
 * @root: Parent debugfs dentry (void* to avoid kernel type exposure)
 *
 * Creates and initializes all debugfs entries including segment debugfs.
 * This function should be called from DRM driver's debugfs_init callback.
 */
void mtgpu_debugfs_init(struct mtgpu_device_node *dev_node, void *root);

/**
 * mtgpu_debugfs_remove - Remove all debugfs entries for a device
 * @dev_node: Pointer to mtgpu device node
 *
 * Removes all debugfs entries created by mtgpu_debugfs_init.
 * This function should be called when device is being removed.
 */
void mtgpu_debugfs_remove(struct mtgpu_device_node *dev_node);

/**
 * mtgpu_segment_debugfs_show - Show segment layout information
 * @m: seq_file to write to
 *
 * This function implements the show callback for segment layout debugfs.
 * It should be used as the show callback for segment_debug_fops.
 */
int mtgpu_segment_debugfs_show(struct seq_file *m, void *data);

ssize_t rm_dump_debugfs_write(struct file *file, const char __user *buf, size_t size, loff_t *pos);

#endif /* __MTGPU_DEBUGFS_H__ */

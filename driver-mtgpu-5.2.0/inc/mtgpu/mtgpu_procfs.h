/*
 * @Copyright   Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License     Dual MIT/GPLv2
 */

#ifndef __MTGPU_PROCFS_H__
#define __MTGPU_PROCFS_H__

struct file;
struct inode;
struct poll_table_struct;

int mtgpu_procfs_create(struct mtgpu_device *mtdev);
void mtgpu_procfs_remove(struct mtgpu_device *mtdev);
int mtgpu_proc_musa_dir_create(void);
void mtgpu_proc_musa_dir_remove(void);
void mtgpu_proc_gpu_instance_remove(struct mtgpu_device *mtdev);
void mtgpu_proc_util_dir_remove(struct mtgpu_device *mtdev);
void mtgpu_proc_util_dir_create(struct mtgpu_device *mtdev);
int mtgpu_proc_mpc_topo_create(struct mtgpu_device *mtdev);
void mtgpu_proc_mpc_dir_remove(struct mtgpu_device *mtdev);
int mtgpu_proc_config_open(struct inode *inode, struct file *file);
ssize_t mtgpu_proc_config_write(struct file *file, const char __user *user_buf,
				size_t nbytes, loff_t *ppos);
int mtgpu_proc_mpc_enable_open(struct inode *inode, struct file *file);
ssize_t mtgpu_proc_mpc_enable_write(struct file *file, const char __user *user_buf,
				    size_t nbytes, loff_t *ppos);

int mtgpu_proc_util_open(struct inode *inode, struct file *file);
ssize_t mtgpu_proc_util_write(struct file *file, const char __user *user_buf,
			      size_t nbytes, loff_t *ppos);
int mtgpu_proc_event_msg_release(struct inode *inode, struct file *file);
int mtgpu_proc_event_msg_open(struct inode *inode, struct file *file);
ssize_t mtgpu_proc_event_msg_read(struct file *file, char __user *buf,
				  size_t count, loff_t *offset);
u32 mtgpu_proc_event_msg_poll(struct file *file, struct poll_table_struct *wait);
int mtgpu_proc_vgpu_monitor_open(struct inode *inode, struct file *file);
long mtgpu_proc_vgpu_monitor_ioctl(struct file *pfilp_t, unsigned int cmd, unsigned long arg);
int mtgpu_proc_vgpu_fw_version_open(struct inode *inode, struct file *file);
int mtgpu_proc_vgpu_allow_upgrade_vm_open(struct inode *inode, struct file *file);
int mtgpu_proc_vgpu_pending_upgrade_vm_open(struct inode *inode, struct file *file);
int mtgpu_proc_vgpu_hot_upgrade_mpc_open(struct inode *inode, struct file *file);
ssize_t mtgpu_proc_vgpu_fw_version_write(struct file *file,
					 const char __user *user_buf,
					 size_t nbytes,
					 loff_t *ppos);
ssize_t mtgpu_proc_vgpu_allow_upgrade_vm_write(struct file *file,
					       const char __user *user_buf,
					       size_t nbytes,
					       loff_t *ppos);
ssize_t mtgpu_proc_vgpu_pending_upgrade_write(struct file *file,
					      const char __user *user_buf,
					      size_t nbytes,
					      loff_t *ppos);
ssize_t mtgpu_proc_vgpu_hot_upgrade_mpc_write(struct file *file,
					      const char __user *user_buf,
					      size_t nbytes,
					      loff_t *ppos);
extern struct proc_dir_entry *mtgpu_proc_musa_dir;

#endif /* __MTGPU_PROCFS_H__ */


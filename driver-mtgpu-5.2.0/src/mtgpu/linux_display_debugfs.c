/*
 * @Copyright   Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License     Dual MIT/GPLv2
 */

#include <linux/debugfs.h>

#if defined(OS_DRM_DRMP_H_EXIST)
#include <drm/drmP.h>
#endif
#if defined(OS_DRM_DRM_DP_HELPER_H_EXIST)
#include <drm/drm_dp_helper.h>
#elif defined(OS_DRM_DP_DRM_DP_HELPER_H_EXIST)
#include <drm/dp/drm_dp_helper.h>
#elif defined(OS_DRM_DISPLAY_DRM_DP_HELPER_H_EXIST)
#include <drm/display/drm_dp_helper.h>
#endif
#include <drm/drm_crtc.h>
#include <drm/drm_connector.h>
#include <drm/drm_device.h>
#include <drm/drm_file.h>
#include <drm/drm_print.h>

#include "dm_hwlib_access.h"

#include "mtgpu_drv.h"
#include "linux_dp_drv.h"
#include "linux_dispc_drv.h"
#include "linux_display_debugfs.h"

static inline
struct mtdisp_dp *connector_to_mtdisp_dp(struct drm_connector *connector)
{
	return container_of(connector, struct mtdisp_dp, connector);
}

/* link rate */
static int link_rate_show(struct seq_file *m, void *data)
{
	struct drm_connector *connector = m->private;
	struct mtdisp_dp *dp = connector_to_mtdisp_dp(connector);
	struct mtdisp_dp_debugfs *debugfs = &dp->debugfs;

	seq_printf(m, "link_rate: %d.%dG\n",
		   debugfs->link_rate / 100000,
		   debugfs->link_rate % 100000 / 1000);

	return 0;
}

static int link_rate_open(struct inode *inode, struct file *file)
{
	struct drm_connector *dev = inode->i_private;

	return single_open(file, link_rate_show, dev);
}

static ssize_t link_rate_write(struct file *file, const char __user *ubuf,
			       size_t len, loff_t *offp)
{
	struct seq_file *m = file->private_data;
	struct drm_connector *connector = m->private;
	struct mtdisp_dp *dp = connector_to_mtdisp_dp(connector);
	struct mtdisp_dp_debugfs *debugfs = &dp->debugfs;
	char *buf;
	u32 val;

	buf = memdup_user(ubuf, len);
	if (IS_ERR(buf))
		return PTR_ERR(buf);

	buf[len - 1] = '\0';

	if (!strncmp(buf, "0", 1)) {
		val = 0;
	} else if (!strncmp(buf, "1.62", 4)) {
		val = drm_dp_bw_code_to_link_rate(DP_LINK_BW_1_62);
	} else if (!strncmp(buf, "2.7", 3)) {
		val = drm_dp_bw_code_to_link_rate(DP_LINK_BW_2_7);
	} else if (!strncmp(buf, "5.4", 3)) {
		val = drm_dp_bw_code_to_link_rate(DP_LINK_BW_5_4);
	} else if (!strncmp(buf, "8.1", 3)) {
		val = drm_dp_bw_code_to_link_rate(DP_LINK_BW_8_1);
	} else {
		DRM_DEV_ERROR(dp->dev, "Wrong link rate configuration, buf:'%s'.\n", buf);
		kfree(buf);
		return -EINVAL;
	}

	debugfs->link_rate = val;
	mtdisp_set_dp_debugfs_link_rate(dp->hwlib_dp, val);

	kfree(buf);

	return len;
}

static const struct file_operations mtdisp_link_rate_fops = {
	.owner = THIS_MODULE,
	.open = link_rate_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
	.write = link_rate_write
};

/* lane count */
static int lane_cnt_show(struct seq_file *m, void *data)
{
	struct drm_connector *connector = m->private;
	struct mtdisp_dp *dp = connector_to_mtdisp_dp(connector);
	struct mtdisp_dp_debugfs *debugfs = &dp->debugfs;

	seq_printf(m, "lane_cnt: %d\n", debugfs->lane_cnt);

	return 0;
}

static int lane_cnt_open(struct inode *inode, struct file *file)
{
	struct drm_connector *dev = inode->i_private;

	return single_open(file, lane_cnt_show, dev);
}

static ssize_t lane_cnt_write(struct file *file, const char __user *ubuf,
			      size_t len, loff_t *offp)
{
	struct seq_file *m = file->private_data;
	struct drm_connector *connector = m->private;
	struct mtdisp_dp *dp = connector_to_mtdisp_dp(connector);
	struct mtdisp_dp_debugfs *debugfs = &dp->debugfs;
	char *buf;
	int ret;
	u8 val;

	buf = memdup_user(ubuf, len);
	if (IS_ERR(buf))
		return PTR_ERR(buf);

	buf[len - 1] = '\0';

	ret = kstrtou8(buf, 10, &val);
	if (ret) {
		DRM_DEV_ERROR(dp->dev, "failed kstrtou8: buf:'%s', ret:%d\n", buf, ret);
		kfree(buf);
		return ret;
	}

	if (val == 3 || val > 4) {
		DRM_DEV_ERROR(dp->dev, "Wrong lane count configuration.\n");
		kfree(buf);
		return -EINVAL;
	}

	debugfs->lane_cnt = val;
	mtdisp_set_dp_debugfs_lane_cnt(dp->hwlib_dp, val);

	kfree(buf);

	return len;
}

static const struct file_operations mtdisp_lane_cnt_fops = {
	.owner = THIS_MODULE,
	.open = lane_cnt_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
	.write = lane_cnt_write
};

/* encoder restart */
static int encoder_restart_show(struct seq_file *m, void *data)
{
	seq_puts(m, "You need execute `echo 1 > encoder_restart`\n");

	return 0;
}

static int encoder_restart_open(struct inode *inode, struct file *file)
{
	struct drm_connector *dev = inode->i_private;

	return single_open(file, encoder_restart_show, dev);
}

static ssize_t encoder_restart_write(struct file *file, const char __user *ubuf,
				     size_t len, loff_t *offp)
{
	struct seq_file *m = file->private_data;
	struct drm_connector *connector = m->private;
	struct mtdisp_dp *dp = connector_to_mtdisp_dp(connector);
	char *buf;
	u8 val;
	int ret;

	if (!mtdisp_get_dp_enabled(dp->hwlib_dp)) {
		DRM_DEV_ERROR(dp->dev, "connector is not enabled");
		return -ENODEV;
	}

	buf = memdup_user(ubuf, len);
	if (IS_ERR(buf))
		return PTR_ERR(buf);

	buf[len - 1] = '\0';

	ret = kstrtou8(buf, 10, &val);
	if (ret) {
		DRM_DEV_ERROR(dp->dev, "failed kstrtou8: buf:'%s', ret:%d\n", buf, ret);
		kfree(buf);
		return ret;
	}

	if (val == 1) {
		mtdisp_dp_encoder_disable(&dp->encoder);
		mtdisp_dp_encoder_enable(&dp->encoder);
	} else {
		DRM_DEV_ERROR(dp->dev, "Wrong encoder restart configuration.\n");
		kfree(buf);
		return -EINVAL;
	}

	kfree(buf);

	return len;
}

static const struct file_operations mtdisp_encoder_restart_fops = {
	.owner = THIS_MODULE,
	.open = encoder_restart_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
	.write = encoder_restart_write
};

void mtdisp_dp_debugfs_create_files(struct mtdisp_dp *dp)
{
	struct dentry *dentry;
	int idx = dp->connector.dev->primary->index;
	char name[32];

	sprintf(name, "card%d-%s", idx, dp->connector.name);

	dentry = debugfs_create_dir(name, mtgpu_dentry);
	dp->debugfs.card = dentry;

	/* link rate */
	debugfs_create_file("link_rate", 0644, dentry, &dp->connector,
			    &mtdisp_link_rate_fops);

	/* lane count */
	debugfs_create_file("lane_cnt", 0644, dentry, &dp->connector,
			    &mtdisp_lane_cnt_fops);

	/* encoder restart */
	debugfs_create_file("encoder_restart", 0644, dentry, &dp->connector,
			    &mtdisp_encoder_restart_fops);
}

void mtdisp_dp_debugfs_remove_files(struct mtdisp_dp *dp)
{
	struct dentry *card = dp->debugfs.card;

	debugfs_remove_recursive(card);
}

/* underrun count */
static int underrun_cnt_show(struct seq_file *m, void *data)
{
	struct mtdisp_dispc *dispc = m->private;
	struct mtdisp_dispc_debugfs *debugfs = &dispc->debugfs;

	seq_printf(m, "underrun_cnt: %llu\n", debugfs->underrun_cnt);
	seq_puts(m, "Write 0 to clear the counter.\n");

	return 0;
}

static int underrun_cnt_open(struct inode *inode, struct file *file)
{
	struct mtdisp_dispc *dispc = inode->i_private;

	return single_open(file, underrun_cnt_show, dispc);
}

static ssize_t underrun_cnt_write(struct file *file, const char __user *ubuf,
				  size_t len, loff_t *offp)
{
	struct seq_file *m = file->private_data;
	struct mtdisp_dispc *dispc = m->private;
	struct mtdisp_dispc_debugfs *debugfs = &dispc->debugfs;
	char *buf;
	u64 val;
	int ret;

	buf = memdup_user(ubuf, len);
	if (IS_ERR(buf))
		return PTR_ERR(buf);

	buf[len - 1] = '\0';

	ret = kstrtou64(buf, 10, &val);
	if (ret) {
		DRM_DEV_ERROR(dispc->dev, "failed kstrtou64: buf:'%s', ret:%d\n", buf, ret);
		kfree(buf);
		return ret;
	}

	if (val != 0) {
		DRM_DEV_ERROR(dispc->dev, "Only 0 can be written to clear.\n");
		kfree(buf);
		return -EINVAL;
	}

	debugfs->underrun_cnt = val;

	kfree(buf);

	return len;
}

static const struct file_operations mtdisp_underrun_cnt_fops = {
	.owner = THIS_MODULE,
	.open = underrun_cnt_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
	.write = underrun_cnt_write,
};

void mtdisp_dispc_debugfs_create_files(struct mtdisp_dispc *dispc)
{
	struct dentry *dentry;
	int idx = dispc->crtc.dev->primary->index;
	char name[32];

	sprintf(name, "card%d-%s", idx, dispc->crtc.name);

	dentry = debugfs_create_dir(name, mtgpu_dentry);
	dispc->debugfs.dentry = dentry;

	/* underrun count */
	debugfs_create_file("underrun_cnt", 0644, dentry, dispc,
			    &mtdisp_underrun_cnt_fops);
}

void mtdisp_dispc_debugfs_remove_files(struct mtdisp_dispc *dispc)
{
	struct dentry *dentry = dispc->debugfs.dentry;

	debugfs_remove_recursive(dentry);
}

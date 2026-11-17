/* Compatibility for interfaces removed after the vendor kernel range. */
#ifndef MT_ARCH_COMPAT_H
#define MT_ARCH_COMPAT_H
#include <linux/version.h>
#if defined(OS_VFS_MKDIR_RETURNS_DENTRY) || defined(OS_VFS_MKDIR_DENTRY_FOUR_ARGS)
#define MT_VFS_MKDIR_RETURNS_DENTRY
#endif
#include <linux/printk.h>
#include <linux/device.h>
#ifdef OS_STRUCT_DRM_ATOMIC_COMMIT_EXIST
#define drm_atomic_state drm_atomic_commit
#define drm_atomic_state_get drm_atomic_commit_get
#define drm_atomic_state_put drm_atomic_commit_put
#define drm_atomic_state_alloc drm_atomic_commit_alloc
#define drm_atomic_state_clear drm_atomic_commit_clear
#endif
#ifdef OS_TIMER_DELETE_SYNC_CALLABLE
#define del_timer_sync timer_delete_sync
#endif
#ifndef from_timer
#define from_timer(var, callback_timer, timer_field) container_of(callback_timer, typeof(*var), timer_field)
#endif
#include <drm/drm_print.h>
#ifndef DRM_INFO
#define DRM_INFO(fmt, ...) pr_info("[drm] " fmt, ##__VA_ARGS__)
#define DRM_ERROR(fmt, ...) pr_err("[drm] " fmt, ##__VA_ARGS__)
#define DRM_WARN(fmt, ...) pr_warn("[drm] " fmt, ##__VA_ARGS__)
#define DRM_DEBUG(fmt, ...) pr_debug("[drm] " fmt, ##__VA_ARGS__)
#define DRM_DEV_INFO(dev, fmt, ...) dev_info(dev, fmt, ##__VA_ARGS__)
#define DRM_DEV_ERROR(dev, fmt, ...) dev_err(dev, fmt, ##__VA_ARGS__)
#define DRM_DEV_WARN(dev, fmt, ...) dev_warn(dev, fmt, ##__VA_ARGS__)
#define DRM_DEV_DEBUG(dev, fmt, ...) dev_dbg(dev, fmt, ##__VA_ARGS__)
#endif
#include <linux/dma-fence.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(7, 3, 0)
#ifndef dma_fence_enable_sw_signaling
#define dma_fence_enable_sw_signaling(fence) dma_fence_enable_signaling(fence)
#endif
#endif

static inline void mt_fence_value_str(struct dma_fence *f, char *buf, int size)
{
#ifdef OS_STRUCT_DMA_FENCE_OPS_HAS_FENCE_VALUE_STR
	if (f->ops->fence_value_str) {
		f->ops->fence_value_str(f, buf, size);
		return;
	}
#endif
	snprintf(buf, size, "%llu", (unsigned long long)f->seqno);
}
static inline void mt_timeline_value_str(struct dma_fence *f, char *buf, int size)
{
#ifdef OS_STRUCT_DMA_FENCE_OPS_HAS_TIMELINE_VALUE_STR
	if (f->ops->timeline_value_str) {
		f->ops->timeline_value_str(f, buf, size);
		return;
	}
#endif
	/* The current completed timeline value is no longer exposed by dma_fence. */
	snprintf(buf, size, "ctx=%llu", (unsigned long long)f->context);
}
#endif

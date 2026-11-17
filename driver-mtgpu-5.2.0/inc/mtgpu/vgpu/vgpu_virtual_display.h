/*
 * @Copyright   Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License     Dual MIT/GPLv2
 */

#ifndef __VGPU_VIRTUAL_DISPLAY_H__
#define __VGPU_VIRTUAL_DISPLAY_H__

#include "linux-list.h"
#include "linux-types.h"
#include "os-interface.h"

struct device;

#if (MUSA_NUM_OS_SUPPORTED > 1)
struct virtual_display {
	u32 id;
	u32 instance_id;
	u32 raw_data;
	ktime_t interval;
	ktime_t next_time;

	struct rb_node *node;
};

int vgpu_vdisplay_vsync_balancing(struct virtual_display **vdisps, u32 count);

int vgpu_vdisplay_register(struct virtual_display *vdisps, u32 data);
void vgpu_vdisplay_unregister(struct virtual_display *vdisps, u32 disp_id);
int vgpu_virtual_display_init(void);
void vgpu_virtual_display_exit(void);

void vgpu_vdisplay_vsync_register(struct device *dev, u32 disp_id, u32 vrefresh);
void vgpu_vdisplay_vsync_unregister(struct device *dev, u32 disp_id);

/*************************************************************************************************/
/*************************** virtual display FB export management ********************************/
/*************************************************************************************************/
struct mutex;
struct vgpu_fb_info;

struct fbe_item_info {
	u64 hAllocation;
	u32 format;
	u32 width;
	u32 height;
	u64 size;
	u32 pitch;
};

struct fbe_manager;
struct fbe_item;

/**
 * Initialize the fbe_manager structure
 * @fbe_mgr: fbe manager instance
 *
 * Return: 0 if everything ok, otherwise false
 */
int fbe_manager_init(struct fbe_manager **fbe_mgr);

/**
 * Clean up entire host cache manager resources
 * Called during driver unload
 * @fbe_mgr: fbe manager instance
 */
void fbe_manager_deinit(struct fbe_manager **fbe_mgr);

/**
 * Initialize resources for a specific FB cache entry on host side
 * @mdev: mdev device
 * @fbe_mgr: fbe manager instance
 * @param hallocation Allocation handle (64-bit)
 * @param info fbe_item_info containing information about the allocation
 */
int fbe_item_setup(void *mdev,
		   struct fbe_manager *fbe_mgr,
		   u64 hallocation,
		   u64 first_addr,
		   struct fbe_item_info *info);

/**
 * Update page table item received from guest
 * Writes to the next available slot in the page table array
 * @param hallocation Allocation handle (64-bit)
 * @param page_addr Page physical address (64-bit)
 */
int fbe_item_update_once(struct fbe_manager *fbe_mgr,
			 u64 hallocation,
			 u64 page_addr);

/**
 * fbe_item_destroy() - Destroy a specific fbe item by dmabuf_id
 * @fbe_mgr: fbe manager instance
 * @dmabuf_id: the dmabuf id to destroy
 */
void fbe_item_destroy(struct fbe_manager *fbe_mgr, u32 dmabuf_id);

/**
 * fbe_reset_status() - Reset fbe manager status
 * @fbe_mgr: fbe manager instance
 */
void fbe_reset_status(struct fbe_manager *fbe_mgr);

/**
 * fbe_item_get_page_ready() - Get page ready status of fbe item
 * @fbe_mgr: fbe manager instance
 * @hallocation: the allocation handle to check
 *
 * Return: true if page is ready, otherwise false
 */
bool fbe_item_get_page_ready(struct fbe_manager *fbe_mgr, u64 hallocation);

/**
 * fbe_item_set_page_ready() - Set page ready status of fbe item
 * @fbe_mgr: fbe manager instance
 * @hallocation: the allocation handle to set
 * @ready: the ready status to set
 *
 * Return: 0 on success, negative error code on failure
 */
int fbe_item_set_page_ready(struct fbe_manager *fbe_mgr, u64 hallocation, bool ready);

/**
 * Check if a specific FB cache entry with given hallocation already exists
 * @param fbe_mgr Pointer to the FB cache manager
 * @param hallocation Allocation handle (64-bit) to check
 * @param active_index Pointer to store the active index if found
 * @param width Width of the FB cache entry
 * @param height Height of the FB cache entry
 *
 * @return true if exists, false if not exists
 */
bool fbe_item_exists(struct fbe_manager *fbe_mgr, u64 hallocation,
		     u32 width, u32 height, u64 first_addr);

/**
 * fbe_item_is_ready() - Check if a specific fbe item is ready
 * @fbe_mgr: fbe manager instance
 * @hallocation: the allocation handle to check
 *
 * Return: true if both manager and item are ready, otherwise false
 */
bool fbe_item_is_ready(struct fbe_manager *fbe_mgr, u64 hallocation);

/**
 * fbe_dmabuf_get() - Get dmabuf by dmabuf_id
 * @fbe_mgr: fbe manager instance
 * @dmabuf_id: the dmabuf id to find
 *
 * Return: pointer to dmabuf if found, otherwise NULL
 */
void *fbe_dmabuf_get(struct fbe_manager *fbe_mgr, u32 dmabuf_id);

/**
 * fbe_mgr_set_ready() - Set fbe manager ready status
 * @fbe_mgr: fbe manager instance
 * @hallocation: the allocation handle to set ready
 */
void fbe_mgr_set_ready(struct fbe_manager *fbe_mgr, u64 hallocation);

/**
 * fbe_mgr_set_active() - Set active hallocation in fbe manager
 * @fbe_mgr: fbe manager instance
 * @hallocation: the allocation handle to set as active
 */
void fbe_mgr_set_active(struct fbe_manager *fbe_mgr,
			u64 hallocation);

/**
 * fbe_item_get_dmabuf() - Get dmabuf by hallocation
 * @fbe_mgr: fbe manager instance
 * @hallocation: the allocation handle to find
 *
 * Return: pointer to dmabuf if found, otherwise NULL
 */
void *fbe_item_get_dmabuf(struct fbe_manager *fbe_mgr,
			  u64 hallocation);

/**
 * fbe_item_get_info() - Get fbe item information
 * @fbe_mgr: fbe manager instance
 * @hallocation: the allocation handle to find
 * @width: pointer to store width value
 * @height: pointer to store height value
 * @size: pointer to store size value
 * @dmabuf_id: pointer to store dmabuf_id value
 *
 * Return: true if item found and info retrieved, otherwise false
 */
bool fbe_item_get_info(struct fbe_manager *fbe_mgr, u64 hallocation,
		       u32 *width, u32 *height, u64 *size,
		       u32 *dmabuf_id);

/**
 * fbe_mgr_get_item() - Get fbe item by hallocation
 * @fbe_mgr: fbe manager instance
 * @hallocation: the allocation handle to find
 *
 * Return: pointer to fbe item if found, otherwise NULL
 */
void *fbe_mgr_get_item(struct fbe_manager *fbe_mgr, u64 hallocation);

/**
 * fbe_item_get_page_count() - Get page count of fbe item
 * @fbe_mgr: fbe manager instance
 * @item: pointer to fbe item
 *
 * Return: page count of the item, 0 if item is NULL
 */
u32 fbe_item_get_page_count(struct fbe_manager *fbe_mgr, void *item);

/**
 * fbe_item_get_page_addr() - Get page address by index
 * @fbe_mgr: fbe manager instance
 * @item: pointer to fbe item
 * @page_idx: page index to get address for
 *
 * Return: page address at given index, 0 if item is NULL or index is out of range
 */
u64 fbe_item_get_page_addr(struct fbe_manager *fbe_mgr, void *item, u32 page_idx);

/**
 * fbe_mgr_get_guest_page_size() - Get guest page size from fbe manager
 * @fbe_mgr: fbe manager instance
 *
 * Return: guest page size, 0 if manager is not initialized
 */
u32 fbe_mgr_get_guest_page_size(struct fbe_manager *fbe_mgr);

/**
 * fbe_mgr_set_guest_page_size() - Set guest page size in fbe manager
 * @fbe_mgr: fbe manager instance
 * @page_size: page size to set
 */
void fbe_mgr_set_guest_page_size(struct fbe_manager *fbe_mgr, u32 page_size);

/**
 * fbe_mgr_get_active_hallocation() - Get active hallocation from fbe manager
 * @fbe_mgr: fbe manager instance
 *
 * Return: active hallocation, 0 if manager is not initialized
 */
u64 fbe_mgr_get_active_hallocation(struct fbe_manager *fbe_mgr);

void fbe_mgr_show_debug_info(struct fbe_manager *fbe_mgr, u32 dmabuf_id);
void fbe_mgr_set_enabled(struct fbe_manager *mgr, bool enabled);
bool fbe_mgr_get_enabled(struct fbe_manager *mgr);

#else
#ifndef MT_UNREFERENCED_PARAMETER
#define MT_UNREFERENCED_PARAMETER(param) ((void)(param))
#endif

struct virtual_display;
static inline int vgpu_vdisplay_register(struct virtual_display *vdisps, u32 data)
{
	MT_UNREFERENCED_PARAMETER(vdisps);
	MT_UNREFERENCED_PARAMETER(data);

	return 0;
}

static inline void vgpu_vdisplay_unregister(struct virtual_display *vdisps, u32 disp_id)
{
	MT_UNREFERENCED_PARAMETER(vdisps);
	MT_UNREFERENCED_PARAMETER(disp_id);
}

static inline int vgpu_virtual_display_init(void)
{
	return 0;
}

static inline void vgpu_virtual_display_exit(void)
{
}

static inline void vgpu_vdisplay_vsync_register(struct device *dev, u32 disp_id, u32 vrefresh)
{
	MT_UNREFERENCED_PARAMETER(dev);
	MT_UNREFERENCED_PARAMETER(disp_id);
	MT_UNREFERENCED_PARAMETER(vrefresh);

}

static inline void vgpu_vdisplay_vsync_unregister(struct device *dev, u32 disp_id)
{
	MT_UNREFERENCED_PARAMETER(dev);
	MT_UNREFERENCED_PARAMETER(disp_id);
}

#endif /* (MUSA_NUM_OS_SUPPORTED > 1) */

#endif /* __VGPU_VIRTUAL_DISPLAY_H__ */

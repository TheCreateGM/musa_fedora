/*
 * @Copyright Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License Dual MIT/GPLv2
 */

#ifndef _MTGPU_SYNCOBJ_H_
#define _MTGPU_SYNCOBJ_H_

#include "mtfw_fwif_types.h"
#include "mt_notifier.h"

enum mtgpu_syncobj_global_type {
	MTGPU_SYNCOBJ_GOLBAL_IMPORT = 1,
	MTGPU_SYNCOBJ_GOLBAL_EXPORT,
};

struct mtgpu_device_node;
struct _CONNECTION_DATA_;
struct mtgpu_semaphore_export_data;
struct mtgpu_semaphore_fence;
struct mtgpu_syncobj_context;
struct mtgpu_syncobj;
struct drm_device;
struct drm_file;

u32 mtgpu_syncobj_get_type(struct mtgpu_syncobj *syncobj);
u64 mtgpu_syncobj_get_global_type(struct mtgpu_syncobj *syncobj);
struct mtgpu_device_node *mtgpu_syncobj_get_node(struct mtgpu_syncobj *syncobj);
void mtgpu_syncobj_fence_lock(struct mtgpu_syncobj *syncobj);
void mtgpu_syncobj_fence_unlock(struct mtgpu_syncobj *syncobj);
void mtgpu_syncobj_add_export_data(struct mtgpu_syncobj *syncobj,
				   struct mtgpu_semaphore_export_data *export_data);
void mtgpu_syncobj_export_fence_signal(struct mtgpu_syncobj *syncobj);
void mtgpu_syncobj_dump_info(struct mtgpu_device_node *dev_node,
			     DUMPDEBUG_PRINTF_FUNC *dump_debug_printf, void *dump_debug_file);
struct mtgpu_semaphore_fence *mtgpu_syncobj_fence_map_lookup(struct mtgpu_syncobj *syncobj,
							     u64 value);
bool mtgpu_syncobj_fence_map_insert(struct mtgpu_syncobj *syncobj,
				    u64 value,
				    struct mtgpu_semaphore_fence *sem_fence);
struct mtgpu_semaphore_fence *mtgpu_syncobj_fence_map_remove(struct mtgpu_syncobj *syncobj,
							    u64 value);
void mtgpu_syncobj_fence_map_signal_all(struct mtgpu_syncobj *syncobj);
s64 mtgpu_syncobj_fence_seqno_inc_return(struct mtgpu_syncobj *syncobj);
u64 mtgpu_syncobj_get_fence_context(struct mtgpu_syncobj *syncobj);

void mtgpu_syncobj_waiting_count_dec(struct mtgpu_syncobj *syncobj);
void mtgpu_syncobj_waiting_count_inc(struct mtgpu_syncobj *syncobj);
bool mtgpu_syncobj_is_user_waiting(struct mtgpu_syncobj *syncobj);

void mtgpu_syncobj_wake_up(struct mtgpu_device_node *dev_node);
struct wait_queue_head *mtgpu_syncobj_get_wait_queue(struct mtgpu_syncobj *syncobj);
bool mtgpu_syncobj_use_in_buffer_sync(struct mtgpu_syncobj *syncobj);
void mtgpu_syncobj_set_buffer_sync_fence(struct mtgpu_syncobj *syncobj, struct dma_fence *data);
struct dma_fence *mtgpu_syncobj_get_buffer_sync_fence(struct mtgpu_syncobj *syncobj);
int mtgpu_syncobj_context_create(struct mtgpu_device_node *dev_node,
				 struct mtgpu_syncobj_context **syncobj_context);
void mtgpu_syncobj_context_destroy(struct mtgpu_syncobj_context *syncobj_context);
u64 mtgpu_syncobj_get_shadow_value(struct mtgpu_syncobj *syncobj);
void mtgpu_syncobj_set_shadow_value(struct mtgpu_syncobj *syncobj, u64 value);
void mtgpu_syncobj_set_error_value(struct mtgpu_syncobj *syncobj);
void mtgpu_syncobj_set_fault_value(struct mtgpu_syncobj *syncobj);
u64 mtgpu_syncobj_get_vram_value(struct mtgpu_syncobj *syncobj);
void mtgpu_syncobj_set_vram_value(struct mtgpu_syncobj *syncobj, u64 value);
u64 mtgpu_syncobj_get_value(struct mtgpu_syncobj *syncobj);
void mtgpu_syncobj_set_value(struct mtgpu_syncobj *syncobj, u64 value);
int mtgpu_syncobj_read_ref(struct mtgpu_syncobj *syncobj);
u64 mtgpu_syncobj_read_seqno(struct mtgpu_syncobj *syncobj);
int mtgpu_syncobj_get(struct mtgpu_syncobj *syncobj);
int mtgpu_syncobj_put(struct mtgpu_syncobj *syncobj);
int mtgpu_syncobj_create(struct mtgpu_syncobj_context *syncobj_context,
			 u32 type,
			 u32 flag,
			 bool with_hash_table,
			 struct mtgpu_syncobj **syncobj);
fw_va mtgpu_syncobj_get_fw_addr(struct mtgpu_syncobj *syncobj);
fw_va mtgpu_syncobj_get_fw_finish_addr(struct mtgpu_syncobj *syncobj);
fw_va mtgpu_syncobj_get_fw_depend_addr(struct mtgpu_syncobj *syncobj);
fw_va mtgpu_syncobj_get_shadow_fw_addr(struct mtgpu_syncobj *syncobj);
u64 mtgpu_syncobj_get_shadow_pa(struct mtgpu_syncobj *syncobj);
gpu_va mtgpu_syncobj_get_gpu_va(struct mtgpu_syncobj *syncobj);
gpu_va mtgpu_syncobj_get_shadow_gpu_va(struct mtgpu_syncobj *syncobj);
int mtgpu_syncobj_create_bo(struct drm_device *drm, struct drm_file *file_priv,
			    struct mtgpu_syncobj *syncobj,
			    bool import,
			    u32 *handle_out,
			    u32 *shadow_handle_out,
			    u64 *value_offset);
int mtgpu_syncobj_lookup_bo(struct drm_file *file_priv,
			    struct mtgpu_syncobj *syncobj,
			    u32 *handle_out,
			    u32 *shadow_handle_out,
			    u64 *value_offset);
int mtgpu_syncobj_block_fec_remap(struct mtgpu_device_node *dev_node);
struct mtgpu_syncobj *mtgpu_syncobj_from_handle(struct _CONNECTION_DATA_ *conn, void *handle);

struct mtgpu_syncobj *mtgpu_syncobj_create_import(struct mtgpu_device_node *dev_node,
						  struct mtgpu_syncobj *global_syncobj,
						  bool no_access_mtlink);
int mtgpu_syncobj_create_global_data(struct mtgpu_syncobj *syncobj);
u64 mtgpu_syncobj_get_global_handle(struct mtgpu_syncobj *syncobj);
void mtgpu_syncobj_set_global_handle(struct mtgpu_syncobj *syncobj, u64 handle);
bool mtgpu_syncobj_is_global_export(struct mtgpu_syncobj *syncobj);
bool mtgpu_syncobj_is_global_import(struct mtgpu_syncobj *syncobj);
void mtgpu_syncobj_set_global_type(struct mtgpu_syncobj *syncobj,
				   enum mtgpu_syncobj_global_type global_type);
void mtgpu_syncobj_find_global_import(struct mtgpu_syncobj *export_syncobj,
				      struct mtgpu_syncobj **import_syncobj_array,
				      u32 *syncobj_cnt_out);
int mtgpu_syncobj_acquire_auto_clear_timeline(struct mtgpu_syncobj *syncobj, u64 *timeline_value);
int mtgpu_syncobj_advance_auto_clear_timeline(struct mtgpu_syncobj *syncobj, u64 *timeline_value);
struct mtgpu_syncobj *
mtgpu_syncobj_find_global_export(struct mtgpu_syncobj *global_import_syncobj);
struct mtgpu_syncobj *mtgpu_syncobj_from_global_handle(void *handle);
bool mtgpu_syncobj_is_native(struct mtgpu_device_node *dev_node_native, struct mtgpu_syncobj *syncobj);

void mtgpu_syncobj_wake_up_rm(struct mtgpu_device_node *dev_node);

#endif /* _MTGPU_SYNCOBJ_H_ */

/*
 * @Copyright Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License Dual MIT/GPLv2
 */

#ifndef _MTGPU_HANDLE_H_
#define _MTGPU_HANDLE_H_

#define MTGPU_TASK_COMM_LEN 16

struct mtgpu_session {
	struct {
		u32 pid;
		u32 vpid;
		u32 tid;
		u8 comm[MTGPU_TASK_COMM_LEN];
	} process_info;

	struct idr *idr;
	spinlock_t *lock;

	struct mtgpu_device_node *dev_node;

	/* indicates if there is a fault submission,
	 * and will be clean after fw reboot.
	 */
	atomic_t process_current_error;
	/* indicates if there is a fault submission since the lastest fw reboot,
	 * and will be clean once umd read it.
	 */
	atomic64_t process_last_error;
};

/**
 * struct mtgpu_object - Represents a mtgpu object
 * @kref: Reference counter for managing the lifecycle of the mtgpu_object
 *
 * The kref is used to track references to the mtgpu object entity. When the
 * reference count drops to zero, the object will be automatically released
 * and its resources freed.
 */
struct mtgpu_object {
	int kref;
};

int mtgpu_session_init(struct mtgpu_session *session, struct mtgpu_device_node *dev_node);
void mtgpu_session_deinit(struct mtgpu_session *session);
int mtgpu_handle_create(struct mtgpu_session *session,
			void *data,
			void (*release)(void *data),
			u64 *handle);
void mtgpu_handle_destroy(struct mtgpu_session *session, u64 handle);
void *mtgpu_handle_object_lookup(struct mtgpu_session *session, u64 handle);

#endif /* _MTGPU_HANDLE_H_ */

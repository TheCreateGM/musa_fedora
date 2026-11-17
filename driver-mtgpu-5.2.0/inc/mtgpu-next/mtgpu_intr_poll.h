/*
 * @Copyright Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License Dual MIT/GPLv2
 */

#ifndef _MTGPU_INTR_POLL_H_
#define _MTGPU_INTR_POLL_H_

#define MTGPU_INTR_POLL_INTERVAL_MIN_MS	(100)

#ifdef CONFIG_HW
#define MTGPU_INTR_POLL_INTERVAL_DEFAULT_MS	(MTGPU_INTR_POLL_INTERVAL_MIN_MS)
#else
#define MTGPU_INTR_POLL_INTERVAL_DEFAULT_MS	(500)
#endif

struct mtgpu_device_node;

int mtgpu_intr_poll_init(struct mtgpu_device_node *dev_node);
void mtgpu_intr_poll_deinit(struct mtgpu_device_node *dev_node);
void mtgpu_intr_poll_start(struct mtgpu_device_node *dev_node);
void mtgpu_intr_poll_stop(struct mtgpu_device_node *dev_node);

#endif /* _MTGPU_INTR_POLL_H_ */

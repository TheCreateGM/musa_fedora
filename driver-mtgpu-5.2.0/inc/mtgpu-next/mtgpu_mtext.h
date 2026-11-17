/*
 * @Copyright Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License Dual MIT/GPLv2
 */

#ifndef _MTGPU_MTEXT_H_
#define _MTGPU_MTEXT_H_

int mtgpu_mtext_info_init(struct mtgpu_device_node *dev_node);
void mtgpu_mtext_info_deinit(struct mtgpu_device_node *dev_node);
void mtgpu_mtext_init(struct mtgpu_device_node *dev_node);

#endif /* _MTGPU_MTEXT_H_ */

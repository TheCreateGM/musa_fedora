/*
 * @Copyright Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License Dual MIT/GPLv2
 */

#ifndef _MTGPU_UIO_KM_H_
#define _MTGPU_UIO_KM_H_

struct _MTGPU_DEVICE_NODE_;
struct mtgpu_fw_info;
struct mtgpu_uio_device;

#if defined(DEBUG)
int mtgpu_uio_device_create(struct _MTGPU_DEVICE_NODE_ *psDeviceNode);
void mtgpu_uio_device_destroy(struct _MTGPU_DEVICE_NODE_ *psDeviceNode);
void mtgpu_uio_set_fw_info(struct mtgpu_uio_device *uio_dev, struct mtgpu_fw_info *fw_info);
#else
static inline int mtgpu_uio_device_create(struct _MTGPU_DEVICE_NODE_ *psDeviceNode) { return 0; }
static inline void mtgpu_uio_device_destroy(struct _MTGPU_DEVICE_NODE_ *psDeviceNode) {}
static inline void mtgpu_uio_set_fw_info(struct mtgpu_uio_device *uio_dev,
					 struct mtgpu_fw_info *fw_info) {}
#endif

#endif /* _MTGPU_UIO_H_ */

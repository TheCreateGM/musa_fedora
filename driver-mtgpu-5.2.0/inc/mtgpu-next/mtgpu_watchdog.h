/*
 * @Copyright Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License Dual MIT/GPLv2
 */

#ifndef _MTGPU_WATCHDOG_H_
#define _MTGPU_WATCHDOG_H_

struct mtgpu_device_node;

void watchdog_info_reset(struct mtgpu_device_node *dev_node);
int mtgpu_watchdog_init(struct mtgpu_device_node *dev_node);
void mtgpu_watchdog_deinit(struct mtgpu_device_node *dev_node);
void mtgpu_watchdog_start(struct mtgpu_device_node *dev_node);
void mtgpu_watchdog_stop(struct mtgpu_device_node *dev_node);
void mtgpu_watchdog_set_response(struct mtgpu_device_node *dev_node);
void mtgpu_watchdog_enable(struct mtgpu_device_node *dev_node);
void mtgpu_watchdog_disable(struct mtgpu_device_node *dev_node);
bool mtgpu_watchdog_is_disabled(struct mtgpu_device_node *dev_node);
bool mtgpu_is_mcu_hang(struct mtgpu_device_node *dev_node);
bool mtgpu_is_fec_hang(struct mtgpu_device_node *dev_node);
void mtgpu_watchdog_set_restart(struct mtgpu_device_node *dev_node);
bool mtgpu_watchdog_get_restart(struct mtgpu_device_node *dev_node);

#endif /* _MTGPU_WATCHDOG_H_ */

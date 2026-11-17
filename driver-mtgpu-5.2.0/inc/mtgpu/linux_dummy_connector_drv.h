/*
 * @Copyright   Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License     Dual MIT/GPLv2
 */

#ifndef __LINUX_DUMMY_CONNECTOR_DRV_H__
#define __LINUX_DUMMY_CONNECTOR_DRV_H__

#include <linux/printk.h>
#include <drm/drm_encoder.h>
#include <drm/drm_connector.h>

#include "linux_display_debug.h"

struct mtdisp_dummy;

struct mtdisp_dummy_connector {
	u8 id;

	u16 max_hres;
	u16 max_vres;

	struct device *dev;

	struct drm_encoder encoder;
	struct drm_connector connector;
};

int mtdisp_dummy_connector_probe(struct mtdisp_dummy *dummy,
				 struct device *dev, struct drm_device *drm);
void mtdisp_dummy_connector_remove(struct mtdisp_dummy *dummy, struct device *dev);

#endif /* __LINUX_DUMMY_CONNECTOR_DRV_H__ */

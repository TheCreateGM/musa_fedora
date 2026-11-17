/*
 * @Copyright   Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License     Dual MIT/GPLv2
 */

#ifndef _MTGPU_DRM_DRV_H_
#define _MTGPU_DRM_DRV_H_

#include <drm/drm_mm.h>
#include "mt_drv.h"
#include "module_common.h"

struct pci_dev;

struct mtgpu_drm_private {
	struct mt_drm_private mt_private;
	struct mt_chip *chip;
};

struct mtgpu_drm_file {
	MTGPU_CONNECTION_PRIV mt_drm_driver_priv;
	void *vpu_priv;
};

extern struct platform_driver dummy_crtc_driver;
extern struct platform_driver dummy_connector_driver;
extern struct platform_driver mtgpu_dispc_driver;
extern struct platform_driver mtgpu_hdmi_driver;
extern struct platform_driver mtgpu_dp_driver;
extern struct platform_driver mtgpu_phy_driver;
extern struct platform_driver mtgpu_dsc_driver;
extern struct platform_driver mtdisp_dummy_driver;
extern struct platform_driver mtdisp_display_driver;

int mtgpu_drm_init(void);
void mtgpu_drm_fini(void);

#endif /* _MTGPU_DRM_DRV_H_ */

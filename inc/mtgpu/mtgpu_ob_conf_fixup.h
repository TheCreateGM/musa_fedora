/*
 * @Copyright   Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License     Dual MIT/GPLv2
 */

#ifndef _MTGPU_OB_CONF_FIXUP_H_
#define _MTGPU_OB_CONF_FIXUP_H_

struct pci_dev;

/*
 * mtgpu_ob_conf_fixup - complete the outbound (OB) bookkeeping that the
 * closed vendor core skips for chips whose chip_info has no ob_ops.
 *
 * Called from os_pci_save_state(), which the vendor's mtgpu_probe() invokes
 * after mtgpu_device_common_init()/mtgpu_ob_init().  Must be idempotent:
 * it is a no-op when the chip provides ob_ops or when mtdev->ob_conf has
 * already been allocated (by the vendor core or by a previous call).
 *
 * See src/mtgpu/mtgpu_ob_conf_fixup.c for the full rationale.
 */
void mtgpu_ob_conf_fixup(struct pci_dev *pdev);

#endif /* _MTGPU_OB_CONF_FIXUP_H_ */

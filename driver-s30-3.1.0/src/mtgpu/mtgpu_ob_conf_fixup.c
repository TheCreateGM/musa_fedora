/*
 * @Copyright   Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License     Dual MIT/GPLv2
 */

/*
 * Outbound (OB) configuration fix-up for MTT S30 (SuDi).
 *
 * ---------------------------------------------------------------------------
 * Why this file exists
 * ---------------------------------------------------------------------------
 * The MUSA runtime refuses to initialise the device unless the driver reports
 * that the GPU can address system memory:
 *
 *   mu-ERR ... There is no guarantee that the device has capability to access
 *               system memory, please enable IOMMU or decrease sysram to 512G
 *   muInit ret=801 (MUSA_ERROR_NOT_SUPPORTED)
 *
 * That capability is reported by mtgpu_query_pci_info_ioctl(), which returns
 * mtgpu_ob_sys_res_is_set(dev).  In the closed vendor core
 * (objs/x86_64/mtgpu_core.o) that helper is:
 *
 *   mtdev = os_dev_get_drvdata(dev);
 *   if (!mtdev)                                    return false;
 *   if (mtdev->chip_info-><field at 0xa8> > 512GiB) return true;
 *   if (!mtdev->ob_conf)                            return false;
 *   return (mtdev->ob_conf->flag & MTGPU_OB_RES_SYS) != 0;
 *
 * and mtgpu_ob_init() - the only function that allocates mtdev->ob_conf -
 * starts with:
 *
 *   if (!mtdev->ob_ops) return 0;          // ob_ops lives at mtdev + 0x650
 *
 * The per-chip table in .rodata of the vendor core has an ob_ops member at
 * offset 0x60.  pinghu1, pinghu1s, quyuan1 and quyuan2 all point at their own
 * <chip>_ob_ops, but the sudi entry (struct at .rodata + 0x226a0) has *no*
 * relocation at +0x60, i.e. sudi_chip_info.ob_ops == NULL.  So on the MTT S30
 * mtgpu_ob_init() returns immediately, mtdev->ob_conf stays NULL forever, and
 * every MUSA runtime entry point fails with 801.
 *
 * ---------------------------------------------------------------------------
 * What this file does
 * ---------------------------------------------------------------------------
 * It performs the bookkeeping mtgpu_ob_init() would have done, minus the
 * outbound *register* programming that only exists as <chip>_ob_ops:
 *
 *   - allocate struct mtgpu_ob_conf (0x4d8 bytes, size verified against the
 *     vendor core's os_kzalloc(0x4d8) in mtgpu_ob_init),
 *   - fill sysmem_addr_start/sysmem_addr_end from mtgpu_global_ob_res, the
 *     System RAM ranges that the open-source mtgpu_ob_res_init() collects by
 *     walking /proc/iomem for IORESOURCE_SYSRAM,
 *   - set MTGPU_OB_RES_SYS so mtgpu_ob_sys_res_is_set() reports true.
 *
 * MTGPU_OB_RES_PCI is deliberately *not* set: it advertises outbound windows
 * for this GPU's own BARs, which need ob_ops to be programmed and are not used
 * by the MUSA runtime.
 *
 * The fix-up only runs when all of the following hold, so it can never change
 * the behaviour of a chip the vendor already supports:
 *
 *   1. mtdev->ob_ops == NULL      (the vendor skipped OB setup for this chip),
 *   2. mtdev->ob_conf == NULL     (nothing to repair),
 *   3. an IOMMU is present on the device bus - the same condition the vendor
 *      core checks with os_iommu_present(os_get_dev_bus_type(dev)); without an
 *      IOMMU the "device can reach system memory" claim would be a lie,
 *   4. mtgpu_global_ob_res holds at least one System RAM range.
 *
 * Evidence that the claim is true on this host (IOMMU enabled, iommu=pt):
 * with the flag forced to 1 through an LD_PRELOAD diagnostic interposer,
 *   - muMemcpyTest transfers and verifies 1 GiB host<->device, and
 *   - mcc-built tests/musa-vector-add.mu executes on the S30 and verifies
 *     1048576 float results on the host.
 * See docs/root-cause-ob-conf.md.
 *
 * mtgpu_ob_deinit() unconditionally kfree()s mtdev->ob_conf, so the allocation
 * made here is released by the normal vendor teardown path.  Every consumer of
 * ob_conf that performs outbound address translation (mtgpu_ob_check(),
 * mtgpu_ob_addr_from_cpu_pa()) additionally requires mtdev->ob_ops != NULL and
 * ob_conf->flag & MTGPU_OB_RES_USER, neither of which this fix-up sets, so no
 * translation path is opened for a chip that has no OB map.
 */

#include <linux/build_bug.h>
#include <linux/slab.h>
#include <linux/stddef.h>

#include "mtgpu.h"
#include "mtgpu_ob_map.h"
#include "mtgpu_ob_res.h"
#include "mtgpu_ob_conf_fixup.h"
#include "os-interface.h"

/*
 * struct mtgpu_ob_conf is only forward-declared by the 3.1.0 open headers
 * (inc/mtgpu/mtgpu.h); the definition lives inside the closed vendor core.
 * This is the 3.1.0 layout, recovered from that core:
 *
 *   mtgpu_ob_init()        -> os_kzalloc(0x4d8)
 *   mtgpu_query_pci_info   -> reads flag at +0x08, sysmem_addr_start at +0x10,
 *                             sysmem_addr_end at +0x18
 *   mtgpu_ob_addr_from_cpu_pa()
 *                          -> reads map_nr at +0x20, maps[] at +0x28
 *   mtgpu_ob_deinit()      -> kfree()s the whole block
 *
 * 0x28 + MAX_OB_MAP_NR(50) * sizeof(struct mtgpu_ob_map_entry)(24) == 0x4d8,
 * so the 3.1.0 structure ends after map_table.  (The 5.2.0 header additionally
 * carries a trailing `bool force_dma32_alloc`, which would make sizeof() ==
 * 0x4e0 and does not exist in this core.)
 */
struct mtgpu_ob_conf {
	u64 offset;
	u64 flag;
	u64 sysmem_addr_start;
	u64 sysmem_addr_end;
	struct mtgpu_ob_map_table map_table;
};

/*
 * ob_conf->flag masks, read out of the vendor core:
 *   mtgpu_ob_sys_res_is_set()  -> mtgpu_ob_res_is_set(dev, 0x2)
 *   mtgpu_ob_pci_res_is_set()  -> mtgpu_ob_res_is_set(dev, 0x4)
 *   mtgpu_ob_deinit()/mtgpu_ob_check()/mtgpu_ob_addr_from_cpu_pa() test bit 0
 *   (MTGPU_OB_RES_USER) before touching the OB map.
 */
#define MTGPU_OB_RES_USER 0x1UL
#define MTGPU_OB_RES_SYS  0x2UL
#define MTGPU_OB_RES_PCI  0x4UL

void mtgpu_ob_conf_fixup(struct pci_dev *pdev)
{
	struct mtgpu_ob_resource *res;
	struct mtgpu_device *mtdev;
	struct mtgpu_ob_conf *conf;
	u64 sys_start = U64_MAX;
	u64 sys_end = 0;
	int i;

	if (!pdev)
		return;

	mtdev = os_pci_get_drvdata(pdev);
	if (!mtdev || !mtdev->dev)
		return;

	/* Layout must match the closed core that fills these fields. */
	BUILD_BUG_ON(offsetof(struct mtgpu_device, ob_ops) != 0x650);
	BUILD_BUG_ON(offsetof(struct mtgpu_device, ob_conf) != 0x6d8);
	BUILD_BUG_ON(sizeof(struct mtgpu_ob_conf) != 0x4d8);
	BUILD_BUG_ON(MTGPU_OB_RES_USER != 0x1UL);

	/* The vendor core owns OB setup whenever ob_ops exists. */
	if (mtdev->ob_ops)
		return;

	/* Already initialised (vendor core or a previous call). */
	if (mtdev->ob_conf)
		return;

	if (!os_iommu_present(os_get_dev_bus_type(mtdev->dev))) {
		dev_warn(mtdev->dev,
			 "no IOMMU on the device bus; not claiming system memory access\n");
		return;
	}

	res = mtgpu_global_ob_res;
	if (!res || !res->sys_res || res->sys_res_nr <= 0) {
		dev_warn(mtdev->dev,
			 "no System RAM outbound resource discovered; not claiming system memory access\n");
		return;
	}

	for (i = 0; i < res->sys_res_nr; i++) {
		u64 base = res->sys_res[i].base;
		u64 end = base + res->sys_res[i].size;

		if (base < sys_start)
			sys_start = base;
		if (end > sys_end)
			sys_end = end;
	}

	if (sys_end <= sys_start) {
		dev_warn(mtdev->dev,
			 "empty System RAM outbound range; not claiming system memory access\n");
		return;
	}

	conf = kzalloc(sizeof(*conf), GFP_KERNEL);
	if (!conf) {
		dev_err(mtdev->dev, "failed to allocate mtgpu_ob_conf\n");
		return;
	}

	conf->sysmem_addr_start = sys_start;
	conf->sysmem_addr_end = sys_end;
	conf->flag = MTGPU_OB_RES_SYS;

	mtdev->ob_conf = conf;

	dev_info(mtdev->dev,
		 "chip has no ob_ops; filled mtgpu_ob_conf for %d System RAM range(s) [%llx..%llx], flag=0x%llx\n",
		 res->sys_res_nr, (unsigned long long)sys_start,
		 (unsigned long long)sys_end,
		 (unsigned long long)conf->flag);
}

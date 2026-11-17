/* SPDX-License-Identifier: MIT */
#ifndef MTGPU_S30_RUNTIME_COMPAT_H
#define MTGPU_S30_RUNTIME_COMPAT_H

#include <linux/io.h>
#include <linux/iopoll.h>

#define MT_S30_DISPC_CONTROL 0x00
#define MT_S30_DISPC_STATUS  0x18
#define MT_S30_DISPC_MASK    0x1c
#define MT_S30_DISPC_ENABLE  BIT(0)
#define MT_S30_DISPC_STOPPED BIT(8)

/* Only for the reviewed SUDI/DB9000 firmware handover, before registering
 * the ISR. The vendor stop() waits for an ISR which is not enabled yet.
 * Observe the raw hardware completion instead; never synthesize disable_done.
 */
static int mtgpu_s30_stop_firmware(void __iomem *regs)
{
	u32 control, status;
	int ret;

	if (!regs)
		return -EINVAL;
	control = readl(regs + MT_S30_DISPC_CONTROL);
	if (control == ~0U)
		return -ENODEV;
	/* Firmware interrupts must stay masked until DRM initialization. */
	writel(0, regs + MT_S30_DISPC_MASK);
	readl(regs + MT_S30_DISPC_MASK);
	if (!(control & MT_S30_DISPC_ENABLE))
		return 0;

	/* W1C only a stale stop event, preserving unrelated status bits. */
	writel(MT_S30_DISPC_STOPPED, regs + MT_S30_DISPC_STATUS);
	readl(regs + MT_S30_DISPC_STATUS);
	writel(control & ~MT_S30_DISPC_ENABLE, regs + MT_S30_DISPC_CONTROL);
	readl(regs + MT_S30_DISPC_CONTROL);
	ret = readl_poll_timeout(regs + MT_S30_DISPC_STATUS, status,
				 status & MT_S30_DISPC_STOPPED, 1000, 500000);
	if (ret)
		return ret;
	if (status == ~0U)
		return -ENODEV;
	writel(MT_S30_DISPC_STOPPED, regs + MT_S30_DISPC_STATUS);
	readl(regs + MT_S30_DISPC_STATUS);
	return 0;
}

#endif

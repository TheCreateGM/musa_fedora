/* SPDX-License-Identifier: MIT */
#ifndef MTGPU_S30_VPU_COMPAT_H
#define MTGPU_S30_VPU_COMPAT_H

/* Called only for PCI 1ed5:0102, native/host, using the pinned 3.1.0 blob.
 * Three core windows end at 0xc000; the TOP page is 0xd000..0xdfff.
 * The vendor's extra page at 0xe000 belongs to DISPC, not the VPU.
 * Keep the vendor configuration/ABI intact; limit only the Linux mapping.
 */
static int mtgpu_s30_vpu_map_size(const struct mt_conf *conf,
				u64 bar_size, u64 display_offset, u32 *size)
{
	unsigned int i;
	const u32 end = 0xe000;

	if (conf->type != TYPE_SUDI || conf->regs_base != 0x108000 ||
	    conf->regs_chip_size != 0xf000 || conf->core_size != 3 ||
	    conf->regs_top_offset != 0xd000 || conf->regs_ata_offset != 0 ||
	    conf->product[0] != CODA980_CODE ||
	    conf->product[1] != WAVE517_CODE ||
	    conf->product[2] != WAVE627_CODE)
		return -EINVAL;
	for (i = 0; i < conf->core_size; i++)
		if (conf->regs_core_size[i] != 0x4000)
			return -EINVAL;
	if (bar_size < conf->regs_base || end > bar_size - conf->regs_base ||
	    display_offset != 0x116000 ||
	    (u64)conf->regs_base + end > display_offset)
		return -ERANGE;
	*size = end;
	return 0;
}

#endif

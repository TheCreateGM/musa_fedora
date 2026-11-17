/*
 * @Copyright   Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License     Dual MIT/GPLv2
 */

#ifndef __PHY_TYPES__
#define __PHY_TYPES__

#include "display_types.h"
#include "display_func.h"

#define PHY_INFO(fmt, ...)		do { } while(0)
#define PHY_ERROR(fmt, ...)		do { } while(0)

struct mtdisp_phy_ctx {
	u8 *regs;
	u8 *dp_regs;
	u8 *glb_regs;
	u8 *sram_regs;
	u8 lane_count;
	u8 bw_code;
	u8 vswing_level;
	u8 preemph_level;
	bool ssc_en;
	u8 id;
	struct mtdisp_phy_cfg_hdr *phy_cfg_hdr;
};

struct mtdisp_phy_ops {
	int  (*init)(struct mtdisp_phy_ctx *ctx);
	int  (*exit)(struct mtdisp_phy_ctx *ctx);
	int  (*power_on)(struct mtdisp_phy_ctx *ctx);
	int  (*power_off)(struct mtdisp_phy_ctx *ctx);
	void (*set_lane_count)(struct mtdisp_phy_ctx *ctx, u8 count);
	void (*set_link_rate)(struct mtdisp_phy_ctx *ctx, u8 bw_code);
	void (*set_vswing_preemph)(struct mtdisp_phy_ctx *ctx, u8 vswing, u8 preemph);
	void (*set_ssc)(struct mtdisp_phy_ctx *ctx, bool ssc_en);
};

struct mtdisp_hwlib_phy {
	struct mtdisp_phy_ctx ctx;
	struct mtdisp_phy_ops *ops;
};

static inline
void phy_reg_write(struct mtdisp_phy_ctx *ctx, int offset, u32 val)
{
	mtdisp_reg_write(ctx->regs + offset, val);
}

static inline
u32 phy_reg_read(struct mtdisp_phy_ctx *ctx, int offset)
{
	return mtdisp_reg_read(ctx->regs + offset);
}

static inline
void phy_dptx_reg_write(struct mtdisp_phy_ctx *ctx, int offset, u32 val)
{
	mtdisp_reg_write(ctx->dp_regs + offset, val);
	/* dummy read to make post write take effect */
	mtdisp_reg_read(ctx->dp_regs + offset);
}

static inline
u32 phy_dptx_reg_read(struct mtdisp_phy_ctx *ctx, int offset)
{
	return mtdisp_reg_read(ctx->dp_regs + offset);
}

static inline
void phy_glb_reg_write(struct mtdisp_phy_ctx *ctx, int offset, u32 val)
{
	mtdisp_reg_write(ctx->glb_regs + offset, val);
	/* dummy read to make post write take effect */
	mtdisp_reg_read(ctx->glb_regs + offset);
}

static inline
u32 phy_glb_reg_read(struct mtdisp_phy_ctx *ctx, int offset)
{
	return mtdisp_reg_read(ctx->glb_regs + offset);
}

static inline
void phy_sram_reg_write(struct mtdisp_phy_ctx *ctx, int offset, u32 val)
{
	mtdisp_reg_write(ctx->sram_regs + offset, val);
	/* dummy read to make post write take effect */
	mtdisp_reg_read(ctx->sram_regs + offset);
}

static inline
u32 phy_sram_reg_read(struct mtdisp_phy_ctx *ctx, int offset)
{
	return mtdisp_reg_read(ctx->sram_regs + offset);
}

extern struct mtdisp_phy_ops mtdisp_phy_snps;
extern struct mtdisp_phy_ops mtdisp_phy_cdns;

#endif /* __PHY_TYPES__ */

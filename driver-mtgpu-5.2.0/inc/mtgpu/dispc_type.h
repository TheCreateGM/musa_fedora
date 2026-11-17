/*
 * @Copyright   Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License     Dual MIT/GPLv2
 */

#ifndef _DISPC_TYPE_H_
#define _DISPC_TYPE_H_

#include "dispc_common.h"
#include "display_func.h"
#include "dsc_types.h"
#include "mtdisp_debug.h"

#define DISPC_GAMMA_SIZE 256

/* Supported FBC modes */
#define DISPC_FBC_MODE_LINEAR     (0x00)
#define DISPC_FBC_MODE_8X8_V10    (0x01)
#define DISPC_FBC_MODE_16X4_V10   (0x02)
#define DISPC_FBC_MODE_MAX        (0x03)

/* Supported FBC fmt */
#define DISPC_FBC_FMT_NONE       (0x00)
#define DISPC_FBC_FMT_ARGB8888   (0x0C)

enum mtdisp_layer_type {
	MTDISP_LAYER_TYPE_OVERLAY, /* DRM_PLANE_TYPE_OVERLAY */
	MTDISP_LAYER_TYPE_PRIMARY, /* DRM_PLANE_TYPE_PRIMARY */
	MTDISP_LAYER_TYPE_CURSOR,  /* DRM_PLANE_TYPE_CURSOR */
};

struct gamma_ctrl_info {
	u16 *gamma_store;
	u32 gamma_size;
	/* Whether to restore csc when updating plane. */
	bool gamma_restore;
	/* Whether to configure csc when setting gamma. */
	bool gamma_visiable;
};

struct cursor_mem_info {
	u8 cursor_buffer_index;
	u32 cursor_mem_size;
	/* the cursor ddr pa */
	u64 cursor_mem_base;
	/* the pcie ddr pa */
	u64 pcie_mem_base;
};

struct mtdisp_dispc_ctx {
	u8 *regs;
	u8 *pre_regs;
	u8 *post_regs;
	u8 *out_regs;
	u8 *glb_regs;
	u8 *dsc_regs;
	u8 *crg_regs;
	int irq;
	int id;
	u32 color_depth;
	u32 gen;
	u32 primary_layer_format;
	struct mtdisp_output_caps output_caps;
	struct mtdisp_videomode vm;
	struct gamma_ctrl_info gamma_ctrl;
	struct cursor_mem_info cursor_mem;
	void *private;
	bool update_done;
	bool disable_done;
	struct mtdisp_dsc_config mt_dsc_config;
};

struct mtdisp_dispc_ops {
	void (*init)(struct mtdisp_dispc_ctx *ctx);
	void (*deinit)(struct mtdisp_dispc_ctx *ctx);
	void (*start)(struct mtdisp_dispc_ctx *ctx);
	void (*stop)(struct mtdisp_dispc_ctx *ctx);
	u32 (*isr)(struct mtdisp_dispc_ctx *ctx);
	void (*config_begin)(struct mtdisp_dispc_ctx *ctx);
	void (*config_end)(struct mtdisp_dispc_ctx *ctx);
	void (*layer_config)(struct mtdisp_dispc_ctx *ctx,
			     struct mtdisp_layer_config *layer, u8 index);
	void (*layer_disable)(struct mtdisp_dispc_ctx *ctx, u8 index);
	void (*vsync_on)(struct mtdisp_dispc_ctx *ctx);
	void (*vsync_off)(struct mtdisp_dispc_ctx *ctx);
	u32 (*mode_valid)(struct mtdisp_dispc_ctx *ctx, struct mtdisp_videomode *vm);
	void (*blank)(struct mtdisp_dispc_ctx *ctx, bool blank);
	u32 (*get_scanline)(struct mtdisp_dispc_ctx *ctx, bool *pbIsBlanking);
	void (*cursor_config)(struct mtdisp_dispc_ctx *ctx, struct mtdisp_layer_config *layer);
	void (*gamma_set)(struct mtdisp_dispc_ctx *ctx, struct mtdisp_gamma_config *gamma_layer);
	void (*hwsync_set)(struct mtdisp_dispc_ctx *ctx, bool enable);
	void (*mode_fixup)(struct mtdisp_dispc_ctx *ctx);
	bool (*fbc_validate)(struct mtdisp_dispc_ctx *ctx,
			     u32 format, u64 modifier, u32 index);

	void (*dsc_enable)(struct mtdisp_dispc_ctx *ctx);
	void (*dsc_disable)(struct mtdisp_dispc_ctx *ctx);

};

struct mtdisp_dispc_glb_ops {
	void (*init)(struct mtdisp_dispc_ctx *ctx);
	void (*enable)(struct mtdisp_dispc_ctx *ctx);
	void (*disable)(struct mtdisp_dispc_ctx *ctx);
	void (*fbc_enable)(struct mtdisp_dispc_ctx *ctx,
			   struct mtdisp_layer_config *layer, u32 index);
	void (*fbc_disable)(struct mtdisp_dispc_ctx *ctx, u32 index);
	void (*dsc_config)(struct mtdisp_dispc_ctx *ctx, bool enable);
	bool (*is_idle_allowed)(struct mtdisp_dispc_ctx *ctx, u32 bandwidth, u32 crtc_num);
};


struct mtdisp_hwlib_dispc {
	struct mtdisp_dispc_ctx ctx;
	struct mtdisp_dispc_ops *ops;
	struct mtdisp_dispc_glb_ops *glb;
	struct mtdisp_dispc_capability *caps;

	/* dispc debugfs params */
	u64 debugfs_underrun_cnt;
};

static inline void dispc_reg_write(struct mtdisp_dispc_ctx *ctx, int offset, u32 val)
{
	mtdisp_reg_write(ctx->regs + offset, val);
	mtdisp_reg_read(ctx->regs + offset);
	DISPC_DBG_REG("offset = 0x%04x value = 0x%08x\n", offset, val);
};

static inline u32 dispc_reg_read(struct mtdisp_dispc_ctx *ctx, int offset)
{
	u32 reg_val;

	reg_val = mtdisp_reg_read(ctx->regs + offset);
	DISPC_DBG_REG("offset = 0x%04x value = 0x%08x\n", offset, reg_val);

	return reg_val;
};

static inline void dispc_reg_set(struct mtdisp_dispc_ctx *ctx, int offset, u32 bits)
{
	u32 reg_val = dispc_reg_read(ctx, offset);

	dispc_reg_write(ctx, offset, reg_val | bits);
};

static inline void dispc_reg_clr(struct mtdisp_dispc_ctx *ctx, int offset, u32 bits)
{
	u32 reg_val = dispc_reg_read(ctx, offset);

	dispc_reg_write(ctx, offset, reg_val & ~bits);
};

static inline void dispc_reg_write2(struct mtdisp_dispc_ctx* ctx, int offset, u32 val)
{
	mtdisp_reg_write(ctx->regs + offset, val);
	mtdisp_reg_read(ctx->regs + offset);
};

static inline u32 dispc_reg_read2(struct mtdisp_dispc_ctx* ctx, int offset)
{
	u32 reg_val;

	reg_val = mtdisp_reg_read(ctx->regs + offset);

	return reg_val;
};

static inline void dispc_glb_reg_write(struct mtdisp_dispc_ctx* ctx, int offset, u32 val)
{
	mtdisp_reg_write(ctx->glb_regs + offset, val);
	mtdisp_reg_read(ctx->glb_regs + offset);

	DISPC_GLB_DBG_REG("offset = 0x%04x value = 0x%08x\n", offset, val);
}

static inline u32 dispc_glb_reg_read(struct mtdisp_dispc_ctx* ctx, int offset)
{
	u32 reg_val;

	reg_val = mtdisp_reg_read(ctx->glb_regs + offset);

	DISPC_GLB_DBG_REG("offset = 0x%04x value = 0x%08x\n", offset, reg_val);

	return reg_val;
};

static inline
void dispc_glb_reg_set(struct mtdisp_dispc_ctx *ctx, int offset, u32 bits)
{
	u32 val = dispc_glb_reg_read(ctx, offset);

	dispc_glb_reg_write(ctx, offset, val | bits);
}

static inline
void dispc_glb_reg_clr(struct mtdisp_dispc_ctx *ctx, int offset, u32 bits)
{
	u32 val = dispc_glb_reg_read(ctx, offset);

	dispc_glb_reg_write(ctx, offset, val & ~bits);
}

/* dsc regs */
static inline
void dispc_dsc_reg_write(struct mtdisp_dispc_ctx *ctx, int offset, u32 val)
{
	mtdisp_reg_write(ctx->dsc_regs + offset, val);
	mtdisp_reg_read(ctx->dsc_regs + offset);
}

static inline
u32 dispc_dsc_reg_read(struct mtdisp_dispc_ctx *ctx, int offset)
{
	u32 reg_val;

	reg_val = mtdisp_reg_read(ctx->dsc_regs + offset);

	return reg_val;
}

/* crg regs */
static inline
void dispc_crg_reg_write(struct mtdisp_dispc_ctx *ctx, int offset, u32 val)
{
	mtdisp_reg_write(ctx->crg_regs + offset, val);
	mtdisp_reg_read(ctx->crg_regs + offset);
}

static inline
u32 dispc_crg_reg_read(struct mtdisp_dispc_ctx *ctx, int offset)
{
	u32 reg_val;

	reg_val = mtdisp_reg_read(ctx->crg_regs + offset);

	return reg_val;
}

extern struct mtdisp_dispc_ops mtdisp_dispc_db9000;
extern struct mtdisp_dispc_ops mtdisp_dispc_v1;
extern struct mtdisp_dispc_ops mtdisp_dispc_v3;

#endif /* _DISPC_TYPE_H_ */

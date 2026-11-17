/*
 * @Copyright   Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License     Dual MIT/GPLv2
 */

#ifndef __HDMI_TYPES_H__
#define __HDMI_TYPES_H__

#include "display_types.h"
#include "display_func.h"
#include "mtdisp_debug.h"
#include "encoder_common.h"

struct mtdisp_hdmi_ctx {
	u8 *regs;
	u8 *phy_regs;
	int id;
	u32 gen;
	int irq;
	struct mtdisp_videomode vm;
	u16 max_hres;
	u16 max_vres;
	u16 max_pclk_100khz;
	int pclk;
	u8 bpp;
	u8 conn_state;
	u32 update_done;
	//wait_queue_head_t *waitq;
	bool is_dvi;
	void *private;
	bool check_mode_with_audio;
};

struct mtdisp_hdmi_ops {
	u32 (*isr)(struct mtdisp_hdmi_ctx *ctx);
	void (*ctx_init)(struct mtdisp_hdmi_ctx *ctx);
	void (*ctx_exit)(struct mtdisp_hdmi_ctx *ctx);
	void (*hw_init)(struct mtdisp_hdmi_ctx *ctx);
	void (*hw_deinit)(struct mtdisp_hdmi_ctx *ctx);
	void (*hpd_enable)(struct mtdisp_hdmi_ctx *ctx);
	void (*config)(struct mtdisp_hdmi_ctx *ctx);
	void (*enable)(struct mtdisp_hdmi_ctx *ctx);
	void (*disable)(struct mtdisp_hdmi_ctx *ctx);
	void (*avi_infoframe)(struct mtdisp_hdmi_ctx *ctx, u8 *buf, u8 len);
	void (*spd_infoframe)(struct mtdisp_hdmi_ctx *ctx, u8 *buf, u8 len);
	void (*audio_infoframe)(struct mtdisp_hdmi_ctx *ctx, u8 *buf, u8 len);
	void (*vendor_infoframe)(struct mtdisp_hdmi_ctx *ctx, u8 *buf, u8 len);
	bool (*is_plugin)(struct mtdisp_hdmi_ctx *ctx);
	int (*i2c_read)(struct mtdisp_hdmi_ctx *ctx, u8 addr, u8 *buf, u8 len);
	int (*i2c_write)(struct mtdisp_hdmi_ctx *ctx, u8 addr, u8 *buf, u8 len);
	void (*audio_enable)(struct mtdisp_hdmi_ctx *ctx);
	void (*audio_disable)(struct mtdisp_hdmi_ctx *ctx);
	void (*audio_mute)(struct mtdisp_hdmi_ctx *ctx);
	void (*audio_unmute)(struct mtdisp_hdmi_ctx *ctx);
	void (*scramble_config)(struct mtdisp_hdmi_ctx *ctx, bool enable);
	int (*audio_set_param)(struct mtdisp_hdmi_ctx *ctx, struct mtdisp_codec_params *params);
};

struct mtdisp_hwlib_hdmi {
	struct mtdisp_hdmi_ctx ctx;
	struct mtdisp_hdmi_ops *ops;

	bool enabled;

	void *i2c_lock;
};

static inline void hdmi_reg_write(struct mtdisp_hdmi_ctx *ctx, int offset, u32 val)
{
	mtdisp_reg_write(ctx->regs + offset, val);
	/* dummy read to make post write take effect */
	mtdisp_reg_read(ctx->regs + offset);

	//DP_DBG_REG("offset = 0x%04x value = 0x%08x\n", offset, val);
}

static inline u32 hdmi_reg_read(struct mtdisp_hdmi_ctx *ctx, int offset)
{
	u32 val = mtdisp_reg_read(ctx->regs + offset);

	//DP_DBG_REG("offset = 0x%04x value = 0x%08x\n", offset, val);

	return val;
}

static inline void hdmi_reg_write2(struct mtdisp_hdmi_ctx *ctx, int offset, u32 val)
{
	mtdisp_reg_write(ctx->regs + offset, val);
	/* dummy read to make post write take effect */
	mtdisp_reg_read(ctx->regs + offset);
}

static inline u32 hdmi_reg_read2(struct mtdisp_hdmi_ctx *ctx, int offset)
{
	u32 val = mtdisp_reg_read(ctx->regs + offset);

	return val;
}

static inline u32 hdmi_reg_set(struct mtdisp_hdmi_ctx *ctx, int offset, u32 bits)
{
	u32 val = mtdisp_reg_read(ctx->regs + offset);

	hdmi_reg_write(ctx, offset, val | bits);

	return val;
}

static inline u32 hdmi_reg_clr(struct mtdisp_hdmi_ctx *ctx, int offset, u32 bits)
{
	u32 val = mtdisp_reg_read(ctx->regs + offset);

	hdmi_reg_write(ctx, offset, val & ~bits);

	return val;
}

extern struct mtdisp_hdmi_ops mtdisp_hdmi_socinext;

#endif // __HDMI_TYPES_H__

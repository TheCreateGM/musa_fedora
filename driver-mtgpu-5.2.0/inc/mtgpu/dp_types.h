/*
 * @Copyright   Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License     Dual MIT/GPLv2
 */

#ifndef __DP_TYPES_H__
#define __DP_TYPES_H__

#include "display_types.h"
#include "display_func.h"
#include "dsc_types.h"
#include "dsc_calc.h"
#include "mtdisp_debug.h"
#include "encoder_common.h"

#define MTDISP_DPCD_RETRY_COUNT			50

#define MTDISP_DP_AUX_I2C_WRITE			0x0
#define MTDISP_DP_AUX_I2C_READ			0x1
#define MTDISP_DP_AUX_I2C_WRITE_STATUS_UPDATE	0x2
#define MTDISP_DP_AUX_I2C_MOT			0x4
#define MTDISP_DP_AUX_NATIVE_WRITE		0x8
#define MTDISP_DP_AUX_NATIVE_READ		0x9

#define MTDISP_DP_AUX_NATIVE_REPLY_ACK		(0x0 << 0)
#define MTDISP_DP_AUX_NATIVE_REPLY_NACK		(0x1 << 0)
#define MTDISP_DP_AUX_NATIVE_REPLY_DEFER	(0x2 << 0)
#define MTDISP_DP_AUX_NATIVE_REPLY_MASK		(0x3 << 0)

#define MTDISP_DP_AUX_I2C_REPLY_ACK		(0x0 << 2)
#define MTDISP_DP_AUX_I2C_REPLY_NACK		(0x1 << 2)
#define MTDISP_DP_AUX_I2C_REPLY_DEFER		(0x2 << 2)
#define MTDISP_DP_AUX_I2C_REPLY_MASK		(0x3 << 2)

#define MTDISP_AUX_RETRY_INTERVAL		500 /* us */

#define DP_SINGLE_PIX_MODE	0
#define DP_DUAL_PIX_MODE	1
#define DP_QUAD_PIX_MODE	2

#define DP_DSC_SDP_BUFFER_SIZE	144

#define MTDISP_LINK_BW_1_62	0x06
#define MTDISP_LINK_BW_2_7	0x0a
#define MTDISP_LINK_BW_5_4	0x14
#define MTDISP_LINK_BW_8_1	0x1e

enum mtdisp_training_pattern {
	MTDISP_TRAINING_PATTERN_DISABLE	= 0,
	MTDISP_TRAINING_PATTERN_1	= 1,
	MTDISP_TRAINING_PATTERN_2	= 2,
	MTDISP_TRAINING_PATTERN_3	= 3,
	MTDISP_TRAINING_PATTERN_4	= 7,
};

enum mtdisp_dpcd_phy_test_pattern {
	PHY_TEST_PATTERN_NONE = 0,
	PHY_TEST_PATTERN_D10_2,
	PHY_TEST_PATTERN_SYMBOL_ERROR,
	PHY_TEST_PATTERN_PRBS7,
	PHY_TEST_PATTERN_80BIT_CUSTOM,/* For DP1.2 only */
	PHY_TEST_PATTERN_CP2520_1,
	PHY_TEST_PATTERN_CP2520_2,
	PHY_TEST_PATTERN_CP2520_3, /* same as TPS4 */
};

struct mtdisp_dp_mode {
	struct mtdisp_videomode vm;
	int pclk_khz;
	u8 bpp;
	bool active;
	int pbn;
	int slots;
	int pbn_div;
	enum mtdisp_output_range_type range_type;
};

struct mtdisp_dp_ctx {
	u8 *regs;
	u8 *glb_regs;
	u8 *tzc_regs;
	u8 *amt_regs;
	u8 *cust_regs;
	u8 *phy_regs;
	int irq;
	int id;
	u32 gen;
	struct mtdisp_dp_mode modes[MAX_DP_MST_STREAMS];
	int src_max_lanes;
	int src_max_rate;
	u8 bw_code;
	u8 lane_cnt;
	int max_rate;
	u8 max_lanes;
	u16 max_hres;
	u16 max_vres;
	u16 max_pclk_100khz;
	u32 event_flags;
	u32 pixel_mode;
	u32 aud_config1;
	struct mtdisp_dsc_config *mt_dsc_config;

	/* mst */
	bool mst_en;
	int max_payloads;
};

struct mtdisp_dp_ops {
	void (*hw_init)(struct mtdisp_dp_ctx *ctx);
	void (*hw_deinit)(struct mtdisp_dp_ctx *ctx);
	void (*config)(struct mtdisp_dp_ctx *ctx);
	void (*enable)(struct mtdisp_dp_ctx *ctx, u32 stream);
	void (*disable)(struct mtdisp_dp_ctx *ctx, u32 stream);
	u32 (*isr)(struct mtdisp_dp_ctx *ctx);
	void (*training_pattern)(struct mtdisp_dp_ctx *ctx, u8 patten);
	void (*power_up)(struct mtdisp_dp_ctx *ctx);
	void (*qual_pattern)(struct mtdisp_dp_ctx *ctx, u8 pattern, u8 *data, u8 size);
	void (*vswing_preemph)(struct mtdisp_dp_ctx *ctx, u8 vswing, u8 preemph);
	void (*enhanced_frame)(struct mtdisp_dp_ctx *ctx, bool enable);
	void (*stream_config)(struct mtdisp_dp_ctx *ctx, u32 stream);
	void (*scramble)(struct mtdisp_dp_ctx *ctx, bool enable);
	void (*downspread)(struct mtdisp_dp_ctx *ctx, u8 data);
	void (*link_bandwidth)(struct mtdisp_dp_ctx *ctx, u8 bw);
	void (*lane_count)(struct mtdisp_dp_ctx *ctx, u8 count);
	bool (*is_plugin)(struct mtdisp_dp_ctx *ctx);
	void (*hpd_enable)(struct mtdisp_dp_ctx *ctx, bool enable);

	int (*aux_read)(struct mtdisp_dp_ctx *ctx, u8 request, u16 addr,
			u8 *buf, u8 bytes, u8 *reply);
	int (*aux_write)(struct mtdisp_dp_ctx *ctx, u8 request, u16 addr,
			 u8 *buf, u8 bytes, u8 *reply);

	void (*audio_enable)(struct mtdisp_dp_ctx *ctx);
	void (*audio_disable)(struct mtdisp_dp_ctx *ctx);
	void (*audio_mute)(struct mtdisp_dp_ctx *ctx, bool enable);
	int (*audio_set_param)(struct mtdisp_dp_ctx *ctx, struct mtdisp_codec_params *params,
			       u32 stream);

	void (*dsc_enable)(struct mtdisp_dp_ctx* ctx);
	void (*dsc_disable)(struct mtdisp_dp_ctx *ctx);

	void (*hdcp_isr)(struct mtdisp_dp_ctx* ctx);
	void (*fec_config)(struct mtdisp_dp_ctx* ctx, bool enable);

	void (*set_mst)(struct mtdisp_dp_ctx* ctx, bool enable);
	void (*enable_source)(struct mtdisp_dp_ctx* ctx, u32 source);
	void (*disable_source)(struct mtdisp_dp_ctx* ctx, u32 source);
	void (*clear_payload_table)(struct mtdisp_dp_ctx* ctx);
	void (*allocate_payload)(struct mtdisp_dp_ctx* ctx, u32 stream, int start, int slots);
	void (*trigger_act)(struct mtdisp_dp_ctx* ctx);
	int (*slots_fixup)(struct mtdisp_dp_ctx* ctx,
			   struct mtdisp_videomode* vm, int pbn, int slots, int pbn_div);
	bool (*mst_mode_valid)(struct mtdisp_dp_ctx* ctx,
			       struct mtdisp_videomode* vm, int pbn, int slots, int pbn_div);

	void (*metadata_config)(struct mtdisp_dp_ctx* ctx,
				u32 stream, struct dp_colorimetry_info* info);
};

struct mtdisp_dp_glb_ops {
	void (*init)(struct mtdisp_dp_ctx *ctx);
	void (*reset)(struct mtdisp_dp_ctx *ctx);
};

struct mtdisp_dsc_caps {
	bool support;
	u32 max_hres;
	u32 max_vres;
};

struct mtdisp_hwlib_dp {
	struct mtdisp_dp_ctx ctx;
	struct mtdisp_dp_ops *ops;
	struct mtdisp_dp_glb_ops *glb;
	struct mtdisp_hwlib_phy *phy;

	u8 port_type;
	bool connected;
	bool enabled;
	u8 dpcd[16];
	u8 train_set[4];

	u8 dsc_dpcd[16];
	u32 max_bpc;
	bool src_support_hdr;
	bool src_support_larger_screen_master;
	bool sink_support_fec;
	bool sink_support_dsc;
	bool src_support_fec;
	struct mtdisp_dsc_caps src_dsc_caps;
	struct mtdisp_dsc_param dsc_param;

	/*
	 * @mst_parent_id:
	 * An array may be required if more than one DP port supports MST.
	 */
	u8 mst_parent_id;
	bool src_support_mst;
	u8 stream_count;

	void *aux_lock;

	/* dp debugfs params */
	u32 debugfs_link_rate;
	u8 debugfs_lane_cnt;
};

static inline
void dptx_reg_write(struct mtdisp_dp_ctx *ctx, int offset, u32 val)
{
	mtdisp_reg_write(ctx->regs + offset, val);
	/* dummy read to make post write take effect */
	mtdisp_reg_read(ctx->regs + offset);

	DP_DBG_REG("offset = 0x%04x value = 0x%08x\n", offset, val);
}

static inline
u32 dptx_reg_read(struct mtdisp_dp_ctx *ctx, int offset)
{
	u32 val = mtdisp_reg_read(ctx->regs + offset);

	DP_DBG_REG("offset = 0x%04x value = 0x%08x\n", offset, val);

	return val;
}

static inline
void dptx_tzc_reg_write(struct mtdisp_dp_ctx *ctx, int offset, u32 val)
{
	mtdisp_reg_write(ctx->tzc_regs + offset, val);
	/* dummy read to make post write take effect */
	mtdisp_reg_read(ctx->tzc_regs + offset);

	DP_DBG_REG("offset = 0x%04x value = 0x%08x\n", offset, val);
}

static inline
void dptx_amt_reg_write(struct mtdisp_dp_ctx *ctx, int offset, u32 val)
{
	mtdisp_reg_write(ctx->amt_regs + offset, val);
	/* dummy read to make post write take effect */
	mtdisp_reg_read(ctx->amt_regs + offset);

	DP_DBG_REG("offset = 0x%04x value = 0x%08x\n", offset, val);
}

static inline
void dptx_glb_reg_write(struct mtdisp_dp_ctx *ctx, int offset, u32 val)
{
	mtdisp_reg_write(ctx->glb_regs + offset, val);
	/* dummy read to make post write take effect */
	mtdisp_reg_read(ctx->glb_regs + offset);

	DP_GLB_DBG_REG("offset = 0x%04x value = 0x%08x\n", offset, val);
}

static inline
u32 dptx_glb_reg_read(struct mtdisp_dp_ctx *ctx, int offset)
{
	u32 val = mtdisp_reg_read(ctx->glb_regs + offset);

	DP_GLB_DBG_REG("offset = 0x%04x value = 0x%08x\n", offset, val);

	return val;
}

static inline
void dptx_cust_reg_write(struct mtdisp_dp_ctx *ctx, int offset, u32 val)
{
	mtdisp_reg_write(ctx->cust_regs + offset, val);
	mtdisp_reg_read(ctx->cust_regs + offset);

	DP_DBG_REG("offset = 0x%04x value = 0x%08x\n", offset, val);
}

static inline
u32 dptx_cust_reg_read(struct mtdisp_dp_ctx *ctx, int offset)
{
	u32 val;

	val = mtdisp_reg_read(ctx->cust_regs + offset);

	DP_DBG_REG("offset = 0x%04x value = 0x%08x\n", offset, val);

	return val;
}

extern struct mtdisp_dp_ops mtdisp_dp_snps;
extern struct mtdisp_dp_ops mtdisp_dp_trilinear;
extern struct mtdisp_dp_ops mtdisp_dp_snps_v1;

#endif /* __DP_TYPES_H__ */

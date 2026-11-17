/*
 * @Copyright   Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License     Dual MIT/GPLv2
 */

#ifndef _DSC_CALC_H_
#define _DSC_CALC_H_

#include "dsc_types.h"

struct mtdisp_dp_mode;

/* VESA Display Stream Compression DSC 1.2 constants */
#define DSC_MUX_WORD_SIZE_8_10_BPC		48
#define DSC_MUX_WORD_SIZE_12_BPC		64
#define DSC_RC_PIXELS_PER_GROUP 		3
#define DSC_SCALE_DECREMENT_INTERVAL_MAX	4095
#define DSC_RANGE_BPG_OFFSET_MASK		0x3f

/* DSC Rate Control Constants */
#define DSC_RC_MODEL_SIZE_CONST 	8192
#define DSC_RC_EDGE_FACTOR_CONST	6
#define DSC_RC_TGT_OFFSET_HI_CONST	3
#define DSC_RC_TGT_OFFSET_LO_CONST	3

/* DSC PPS constants and macros */
#define DSC_PPS_VERSION_MAJOR_SHIFT		4
#define DSC_PPS_BPC_SHIFT			4
#define DSC_PPS_MSB_SHIFT			8
#define DSC_PPS_LSB_MASK			(0xFF << 0)
#define DSC_PPS_BPP_HIGH_MASK			(0x3 << 8)
#define DSC_PPS_VBR_EN_SHIFT			2
#define DSC_PPS_SIMPLE422_SHIFT 		3
#define DSC_PPS_CONVERT_RGB_SHIFT		4
#define DSC_PPS_BLOCK_PRED_EN_SHIFT		5
#define DSC_PPS_INIT_XMIT_DELAY_HIGH_MASK	(0x3 << 8)
#define DSC_PPS_SCALE_DEC_INT_HIGH_MASK 	(0xF << 8)
#define DSC_PPS_RC_TGT_OFFSET_HI_SHIFT		4
#define DSC_PPS_RC_RANGE_MINQP_SHIFT		11
#define DSC_PPS_RC_RANGE_MAXQP_SHIFT		6
#define DSC_PPS_NATIVE_420_SHIFT		1
#define DSC_1_2_MAX_LINEBUF_DEPTH_BITS		16
#define DSC_1_2_MAX_LINEBUF_DEPTH_VAL		0
#define DSC_1_1_MAX_LINEBUF_DEPTH_BITS		13

enum row_index_bpp {
	ROW_INDEX_6BPP = 0,
	ROW_INDEX_8BPP,
	ROW_INDEX_10BPP,
	ROW_INDEX_12BPP,
	ROW_INDEX_15BPP,
	MAX_ROW_INDEX
};

enum column_index_bpc {
	COLUMN_INDEX_8BPC = 0,
	COLUMN_INDEX_10BPC,
	COLUMN_INDEX_12BPC,
	COLUMN_INDEX_14BPC,
	COLUMN_INDEX_16BPC,
	MAX_COLUMN_INDEX
};

struct mtdisp_dsc_param {
	u8 version_major;
	u8 version_minor;
	u8 line_buf_depth;
	bool block_pred_enable;
};

struct rc_parameters {
	u16 initial_xmit_delay;
	u8 first_line_bpg_offset;
	u16 initial_offset;
	u8 flatness_min_qp;
	u8 flatness_max_qp;
	u8 rc_quant_incr_limit0;
	u8 rc_quant_incr_limit1;
	struct dsc_rc_range_parameters rc_range_params[MTDISP_DSC_NUM_BUF_RANGES];
};

void dsc_compute_params(struct mtdisp_dsc_config *mtdisp_dsc_config,
			struct mtdisp_dsc_param *dsc_param, struct mtdisp_dp_mode *mode);

#endif /* _DSC_CALC_H_ */

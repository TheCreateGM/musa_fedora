/*
 * @Copyright   Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License     Dual MIT/GPLv2
 */

#ifndef _MTVPU_CMD_H_
#define _MTVPU_CMD_H_

#ifndef BUILD_MEDIA_DRIVER
#include "mtvpu_drm.h"
#endif

#include "vpuapi.h"
#include "jpuapi.h"
#include "jdi.h"

#define FBC_COUNT_MAX   18
#define HD_RESOLUTION	(1280 * 720)
#define FHD_RESOLUTION	(1920 * 1080)
#define QHD_RESOLUTION	(2560 * 1440)
#define UHD_RESOLUTION	(3840 * 2160)
#define UHD_WIDTH       (3840)
#define UHD_HEIGHT      (2160)
#define QUHD_WIDTH      (7680)
#define QUHD_HEIGHT     (4320)
#define CODEC_CAPACITY_BENCH	HD_RESOLUTION

/* Windows VDI guest manage all the buffers, and we need to do the pa update for it.
 * For decoder, we need to check work(1) + task(1) + fbc(18) + fbcc/fbcy(1) + mv(1) = 22.
 * For encoder, we need to check work(1) + fbc(8) + subsample(1) + mvcol(1) + fbcc(1) + fbcy(1) = 13.
 */
#define DEC_PA_CHECK_COUNT	22
#define ENC_PA_CHECK_COUNT	13

#define ENCODE_COMPOUND_PIC_PARAM_OFFSET      0x0
#define ENCODE_COMPOUND_PACKED_HEADER_OFFSET  0x1000
#define ENCODE_COMPOUND_PACKED_DATA_OFFSET    0x2000
#define ENCODE_COMPOUND_SLICE_PARAM_OFFSET    0xf000

struct UserDecOpen {
	CodStd         bitstreamFormat;
	BitStreamMode  bitstreamMode;

	EndianMode     streamEndian;
	EndianMode     frameEndian;

	FrameBufferFormat wtlFormat;
	int            wtlEnable;
	int            wtlMode;

	int            picWidth;
	int            picHeight;

	int            cbcrInterleave;
	int            nv21;
	int            vaEnable;
	int            pvricFbcEnable;
	int            pvricFbcMode;
	int            cmdqueDisabled;
	int            reserved[3];
};

struct ParamDecOpen {
	struct UserDecOpen param;

	Uint64         streamHandle; /* handle(32 bits) or addr(64 bits) */
	Uint32	       streamSize;

	VpuHandle      handle;
	int            coreIdx;
	int            instIdx;
	int            productId;    /* return from kmd after drm open */
	int            groupId;      /* memory group id */
	int            poolId;      /* memory pool id */

	Uint64         decWorkHandle;
	Uint32         decWorkSize;
};

struct ParamDecClose {
	VpuHandle      handle;
	UpdatePaInfo_t workPaUpdateInfo;
};

struct UserInitialInfo {
	int            lumaBitdepth;
	int            chromaBitdepth;
	int            picWidth;
	int            picHeight;
	int            minFrameBufferCount;
};

struct ParamDecInitSeq {
	VpuHandle      handle;
	Uint64	       streamHandle;
	Uint64	       paramsHandle;

	Uint32	       streamSize;
	int            numRenderTarget;
	struct UserInitialInfo info;

	Uint32          decFbcSize;
	Uint64          decFbctblHandle;
	Uint32          decFbctbleSize;
	Uint32          decFbcYTblSize;
	Uint32          decFbcCTblSize;
	Uint64          decFbcHandle[FBC_COUNT_MAX];
	Uint32          decMvSize;
	Uint64          decMvHandle;
	Uint64          decTaskHandle;
	Uint32          decTaskSize;
	Uint64          decEtcHandle[DEC_ETC_NUM];
	Uint32          decEtcSize;
	Uint32          decFbcCount;
	Uint32          decTaskParamSize;
	Uint32          decTaskVcoreSize;
	Uint32          decTaskVlcSize;
	UpdatePaInfo_t  workPaUpdateInfo;
	Uint64          reserved[2];
};

struct ParamDecOpenInit
{
    struct ParamDecOpen    decOpen;
    struct ParamDecInitSeq decSeqInit;
};

struct UserOutputInfo {
	int            indexFrameDecoded;
	int            indexFrameDisplay;
	int            numOfErrMBs;
};

struct ParamDecStartFrame {
	VpuHandle      handle;
	Uint64	       streamHandle;
	Uint64	       paramsHandle;
	Uint64	       linearHandle;
	Uint32	       streamSize;
	Uint32         vaSliceNum;
	Uint32         vaRenderTarget;

	Uint32         offsetCb;
	Uint32         offsetCr;

#if defined(_ARM64_) || defined(__arm64__) || defined(__aarch64__)
	Uint32         padding_0;
#endif

	FBCInfo        linearFBCInfo;
	Uint32         renderTargetNum;
	Uint64         frameCycle;
	Uint64	       dstSemaHandle;
	Uint64	       dstSemaWaitValue;

	UpdatePaInfo_t paUpdateArray[DEC_PA_CHECK_COUNT];
};

struct UserEncOpen {
	CodStd         bitstreamFormat;
	FrameBufferFormat srcFormat;

	EndianMode     streamEndian;
	EndianMode     frameEndian;
	EndianMode     sourceEndian;

	Uint32         rcMode;
	Uint32         enRateControl;
	int            internalBitDepth;

	int            picWidth;
	int            picHeight;

	int            cbcrInterleave;
	int            nv21;
	int            vaEnable;
};

struct ParamEncOpen {
	struct UserEncOpen param;

	VpuHandle      handle;
	int            coreIdx;
	int            instIdx;
	int            groupId; /* memory group id */
	int            poolId; /* memory pool id */

	Uint64         encWorkHandle;
	Uint32         encWorkSize;
};

struct ParamEncClose {
	VpuHandle      handle;
	UpdatePaInfo_t workPaUpdateInfo;
};

struct ParamEncInitSeq {
	VpuHandle handle;

	Uint16    seqParamNum;
	Uint16    picParamNum;
	Uint16    miscEnable;
	Uint16	  intraRefreshMode;
	Uint16    intraRefreshArg;
	Uint64    seqParamHandle;
	Uint64    picParamHandle;
	Uint64    miscParamTypeFrameRateHandle;
	Uint64    miscParamTypeRateControlHandle;
	Uint64    miscParamTypeHRDHandle;
	Uint16    packedSeqParamNum;
	Uint16    packedPicParamNum;
	Uint16    packedSliceParamNum;
	Uint16    packedSeiParamNum;

	Uint64    encDefCdfHandle;
	Uint32    encDefCdfSize;
	UpdatePaInfo_t  workPaUpdateInfo;
};

struct ParamEncStartFrame {
	VpuHandle handle;

	Uint16    seqParamNum;
	Uint16    picParamNum;
	Uint16    sliceParamNum;
	Uint16    packedSeqParamNum;
	Uint16    packedPicParamNum;
	Uint16    packedSliceParamNum;
	Uint16    packedSeiParamNum;
	Uint16    miscEnable;
	Uint64    seqParamHandle;
	Uint64    picParamHandle;
	Uint64    sliceParamHandle;
	Uint64    packedParamHandle;
	Uint64    packedParamDataHandle;
	Uint64    miscParamTypeFrameRateHandle;
	Uint64    miscParamTypeRateControlHandle;
	Uint64    miscParamTypeHRDHandle;
	Uint64    reconTargetFbcYOffsetHandle;
	Uint64    reconTargetFbcCOffsetHandle;
	Uint64    reconTargetMvColHandle;
	Uint64    reconTargetSubSampledHandle;
	Uint64    reconTargetFbcYHandle;
	Uint64    srcYHandle;

	Uint32    reconId;
	Uint32    stride;
	Uint32    height;
	UpdatePaInfo_t paUpdateArray[ENC_PA_CHECK_COUNT];

	Uint64    streamHandle;
	Uint32    streamSize;
	Uint64    roiBufHandle;
	Uint16	  uniqueHeaderFlag;
	Uint16    packedSliceHeaderOnly;
	FBCInfo   renderTargetFBCInfo;
	Uint64    frameCycle;

	Uint16    intraRefreshMode;
	Uint16    intraRefreshArg;

	Uint64    encDefCdfHandle;
	Uint32    encDefCdfSize;

	Uint32	  metadataHandleSize;
	Uint64	  metadataHandle;
};

struct ParamEncStartFrameV2 {
	VpuHandle handle;

	Uint16    seqParamNum;
	Uint16    picParamNum;
	Uint16    sliceParamNum;
	Uint16    packedSeqParamNum;
	Uint16    packedPicParamNum;
	Uint16    packedSliceParamNum;
	Uint16    packedSeiParamNum;
	Uint16    miscEnable;
	Uint64    seqParamHandle;
	Uint64    picParamHandle;
	Uint64    sliceParamHandle;
	Uint64    packedParamHandle;
	Uint64    packedParamDataHandle;
	Uint64    miscParamTypeFrameRateHandle;
	Uint64    miscParamTypeRateControlHandle;
	Uint64    miscParamTypeHRDHandle;
	Uint64    reconTargetFbcYOffsetHandle;
	Uint64    reconTargetFbcCOffsetHandle;
	Uint64    reconTargetMvColHandle;
	Uint64    reconTargetSubSampledHandle;
	Uint64    reconTargetFbcYHandle;
	Uint64    srcYHandle;

	Uint32    reconId;
	Uint32    stride;
	Uint32    height;
	UpdatePaInfo_t paUpdateArray[ENC_PA_CHECK_COUNT];

	Uint64    streamHandle;
	Uint32    streamSize;
	Uint64    roiBufHandle;
	Uint16	  uniqueHeaderFlag;
	Uint16    packedSliceHeaderOnly;
	FBCInfo   renderTargetFBCInfo;
	Uint64    frameCycle;

	Uint16    intraRefreshMode;
	Uint16    intraRefreshArg;

	Uint64    encDefCdfHandle;
	Uint32    encDefCdfSize;

	//abi version 2 add
	Uint32    srcYSize;
	Uint32    srcCbSize;
	Uint32    srcCrSize;
	Uint64    srcCbHandle;
	Uint64    srcCrHandle;
	Uint32    reconTargetFbcYHandleSize;
	Uint32    reconTargetFbcCbHandleSize;
	Uint32    reconTargetFbcCrHandleSize;
	Uint64    reconTargetFbcCbHandle;
	Uint64    reconTargetFbcCrHandle;
	Uint32    reconTargetFbcYOffsetSize;
	Uint32    reconTargetFbcCbOffsetSize;
	Uint32    reconTargetFbcCrOffsetSize;
	Uint64    reconTargetFbcCrOffsetHandle;

	Uint32    drv_err_reason;	/* enum vpu_drv_err_reason */
	Uint32	  metadataHandleSize;
	Uint64	  metadataHandle;
	Uint32    reserved[4];
};

struct mtvpu_gem_wait {
	Uint64    bo_handle;
	Uint32    offset;
	Uint32    flags;
	Int64     timeout_ns;

	VpuHandle handle;
	Uint64    reserved[4];
};

struct mtvpu_gem_dma {
	Uint64	  user_vaddr;
	Uint64	  handle;
	Uint32	  size;
	Uint32	  to_dev;
};

struct mtvpu_gem_busy {
	Uint64    bo_handle;
	Uint32    busy;

	VpuHandle handle;
	Uint64    reserved[4];
};

/* extend drm_mode_create_dumb::flags when call DRM_IOCTL_MT_GEM_CREATE_DUMB */
union drm_mode_dumb_flags {
	Uint32		u32_val;
	struct {
		Uint32  mem_grp_id	: 8;
		Uint32	mem_type	: 8; /* enum drm_dumb_mem_type*/
		Uint32	mem_pool_id	: 8;
		Uint32  reserved	: 8;
	};
};

struct mtvpu_buffer_operate {
	Uint64    bo_handle;
	union drm_mode_dumb_flags flags;
	Uint32    reserved[4];
};

enum drm_dumb_mem_type {
	MEM_SURFACE = 100,
	MEM_SURFACE_COMPRESSED,
	MEM_BUFFER,
	MEM_DEC_PARAM,
	MEM_DEC_STREAM,
	MEM_ENC_PARAM,
	MEM_ENC_STREAM,
	MEM_ENC_MV,
	MEM_ENC_SUBSAMPLED,
	MEM_ENC_FBCY,
	MEM_ENC_FBCYOFFSET,
	MEM_ENC_FBCC,
	MEM_ENC_FBCCOFFSET,
	MEM_ENC_FBCV,
	MEM_ENC_FBCVOFFSET,
};

enum mtvpu_core_series {
	MTVPU_CORE_ANY = 0,
	MTVPU_CORE_DEC = 1,					/* KMD will decide using WAVE5 or BODA */
	MTVPU_CORE_W5 = 5,
	MTVPU_CORE_W6 = 6,
	MTVPU_CORE_WAVE5 = MTVPU_CORE_W5,
	MTVPU_CORE_WAVE6 = MTVPU_CORE_W6,
	MTVPU_CORE_BODA = 9,
	MTVPU_CORE_CODA = MTVPU_CORE_BODA,
	MTVPU_CORE_JPU,
};

/**
 * By supporting JPU parallelism, UMD need jpu_idx get core-binding bsbuf.
 * 
 * When using a35:
 * 	- a35_idx is real but jpu_idx is fake
 * 	- jpu_idx is not the real core index a35 scheduled
 *  - jpu_idx is use for UMD to prevent bsbuf race
 **/
typedef union _mtjpu_core_idx {
	Int32		val;
	struct {
		Int32	jpu_idx		: 15;
		Int32	a35_idx		: 15;
		Int32	has_a35		: 2;
	};
} mtjpu_core_idx;

/**
 * request core and group
 */
struct mtvpu_request_core {
	int	core_type;		// [i] enum mtvpu_core_series
	int	format;			// [i] set to -1 if uncertain
	Uint32	mem_grp_id;		// [i] force using group;
					// [o] set to 0 will return group binding to the request core
	Uint32	prefer_grp;		// [i] should be set to 0 if no need
	int	coreIdx;		// [o] neg/0xFF means invalid index
	int	uninst_coreIdx;	// [i] for load balancing, set to neg/0xFF if no need
	int	limit_to_av1enc_grp;
	int	request_core_idx;	// [i] for debug. >= 0 means user specifies the core_idx, neg/0xFF means driver choose core_idx
};

struct mtvpu_unref_core {
	int	coreIdx;
};

struct mtvpu_create_pool {
	Uint32 group_id;
	Uint32 size;
	Uint32 pool_id;
};

struct mtvpu_modify_dump {
	Uint64 bo_handle;
	Uint32 group_id;
	Uint32 inc_size;
	Uint32 mem_recovery;
};

struct mtvpu_get_devinfo {
	Uint32 vender_id;
	Uint32 device_id;
	Uint32 gen;
	Uint32 vdec;
	Uint32 venc;
	Uint32 jpu;
	Uint32 legacy_dec;
	Uint32 mem_size;  /* MBytes for max resolution*/
	Uint32 dec_max_width;
	Uint32 dec_max_height;
	Uint32 enc_max_width;
	Uint32 enc_max_height;
	Uint32 guest_vcore_id;
	Uint32 guest_vcpu_id;
	Uint32 guest_enc_vcore_id;
	Uint32 max_abi_ver;
	Uint32 resize_bar_off;
	Uint32 reserved[16];
};

struct mtvpu_sema_cmd
{
	Uint64 type; /* 0 create, 1 destroy, 2 wait.*/
	Uint64 sema;
	Uint64 reserved[3];
};

struct JpuParamDecOpen {
	JpgHandle handle;
	JpgDecOpenParam decOp;
	Uint64 bitStreamHandle;
	mtjpu_core_idx jIdx;		// pre-reserved core index
	int cmd_version;
};

struct JpuParamDecStart {
	JpgHandle handle;
	Uint64 frameBufHandle;
	Uint32 framePitch;
	Uint32 frameHeight;
	void *bsBuf;
	JpgDecOutputInfo output;
	mtjpu_core_idx jIdx;		// pre-reserved core index
	Uint64 dstSemaHandle;
	Uint64 dstSemaWaitValue;
	int cmd_version;
};

struct JpuParamDecClose {
	JpgHandle handle;
	int cmd_version;
};

struct JpuParamEncOpen {
	JpgHandle handle;
	JpgEncOpenParam encOp;
	Uint64 bitStreamHandle;
};

struct JpuParamEncStart {
	JpgHandle handle;
	Uint64 frameBufHandle;
	Uint32 framePitch;
	Uint32 frameHeight;
	void *bsBuf;
	Uint32 hdrSize;
	JpgEncOutputInfo output;
};

struct JpuParamEncClose {
	JpgHandle handle;
};

struct JpgEncHufTable {
	void	*buf;
	BYTE	*huffVal[8];		/* huffVal[8][256]; */
	BYTE	*huffBits[8];		/* huffBits[8][256]; */
	short	*qMatTab[4];		/* qMatTab[4][64]; */
};

enum {
	MTVPU_IOCTL_DEC_OPEN = 100,
	MTVPU_IOCTL_DEC_CLOSE,
	MTVPU_IOCTL_DEC_INIT_SEQ,
	MTVPU_IOCTL_DEC_START_FRAME,
	MTVPU_IOCTL_DEC_OPEN_AND_INIT,

	MTVPU_IOCTL_ENC_OPEN = 200,
	MTVPU_IOCTL_ENC_CLOSE,
	MTVPU_IOCTL_ENC_INIT_SEQ,
	MTVPU_IOCTL_ENC_START_FRAME,
	MTVPU_IOCTL_ENC_START_FRAME_V2,
};

#define MTVPU_DEC_CMD_CHECKSUM \
	(sizeof(struct ParamDecOpen) << 0) + \
	(sizeof(struct ParamDecClose) << 1) + \
	(sizeof(struct ParamDecInitSeq) << 2) + \
	(sizeof(struct ParamDecStartFrame) << 3) + \
	(offsetof(struct ParamDecOpen, streamHandle) << 4) + \
	(offsetof(struct ParamDecOpen, handle) << 5) + \
	(offsetof(struct ParamDecOpen, instIdx) << 6) + \
	(offsetof(struct ParamDecOpen, groupId) << 7) + \
	(offsetof(struct ParamDecOpen, decWorkHandle) << 8) + \
	(offsetof(struct ParamDecInitSeq, streamHandle) << 9) + \
	(offsetof(struct ParamDecInitSeq, streamSize) << 10) + \
	(offsetof(struct ParamDecInitSeq, info) << 11) + \
	(offsetof(struct ParamDecInitSeq, decFbcSize) << 12) + \
	(offsetof(struct ParamDecInitSeq, decFbctbleSize) << 13) + \
	(offsetof(struct ParamDecInitSeq, decMvSize) << 14) + \
	(offsetof(struct ParamDecInitSeq, decTaskHandle) << 15) + \
	(offsetof(struct ParamDecInitSeq, decEtcHandle) << 16) + \
	(offsetof(struct ParamDecStartFrame, streamHandle) << 17) + \
	(offsetof(struct ParamDecStartFrame, linearHandle) << 18) + \
	(offsetof(struct ParamDecStartFrame, vaSliceNum) << 19) + \
	(offsetof(struct ParamDecStartFrame, offsetCb) << 20) + \
	(offsetof(struct ParamDecStartFrame, renderTargetNum) << 21)

#define MTVPU_ENC_CMD_CHECKSUM \
	(sizeof(struct ParamEncOpen) << 0) + \
	(sizeof(struct ParamEncClose) << 1) + \
	(sizeof(struct ParamEncInitSeq) << 2) + \
	(sizeof(struct ParamEncStartFrame) << 3) + \
	(offsetof(struct UserEncOpen, internalBitDepth) << 4) + \
	(offsetof(struct ParamEncOpen, handle) << 5) + \
	(offsetof(struct ParamEncOpen, instIdx) << 6) + \
	(offsetof(struct ParamEncOpen, poolId) << 7) + \
	(offsetof(struct ParamEncOpen, encWorkSize) << 8) + \
	(offsetof(struct ParamEncInitSeq, seqParamNum) << 9) + \
	(offsetof(struct ParamEncInitSeq, miscEnable) << 10) + \
	(offsetof(struct ParamEncInitSeq, intraRefreshArg) << 11) + \
	(offsetof(struct ParamEncInitSeq, picParamHandle) << 12) + \
	(offsetof(struct ParamEncInitSeq, miscParamTypeRateControlHandle) << 13) + \
	(offsetof(struct ParamEncInitSeq, packedSeqParamNum) << 14) + \
	(offsetof(struct ParamEncInitSeq, packedSliceParamNum) << 15) + \
	(offsetof(struct ParamEncInitSeq, encDefCdfHandle) << 16) + \
	(offsetof(struct ParamEncStartFrame, seqParamNum) << 17) + \
	(offsetof(struct ParamEncStartFrame, sliceParamNum) << 18) + \
	(offsetof(struct ParamEncStartFrame, packedPicParamNum) << 19) + \
	(offsetof(struct ParamEncStartFrame, seqParamHandle) << 20) + \
	(offsetof(struct ParamEncStartFrame, sliceParamHandle) << 21) + \
	(offsetof(struct ParamEncStartFrame, miscParamTypeFrameRateHandle) << 22) + \
	(offsetof(struct ParamEncStartFrame, reconTargetMvColHandle) << 23) + \
	(offsetof(struct ParamEncStartFrame, paUpdateArray) << 24) + \
	(offsetof(struct ParamEncStartFrame, streamSize) << 25) + \
	(offsetof(struct ParamEncStartFrame, renderTargetFBCInfo) << 26) + \
	(offsetof(struct ParamEncStartFrame, intraRefreshMode) << 27) + \
	(offsetof(struct ParamEncStartFrame, encDefCdfHandle) << 28)

#define MTVPU_DEC_VERSION2_CMD_CHECKSUM \
	(sizeof(struct ParamDecOpenInit) << 0) + \
	(sizeof(struct ParamDecClose) << 1) + \
	(sizeof(struct ParamDecStartFrame) << 2) + \
	(offsetof(struct ParamDecOpenInit, decOpen) << 3) + \
	(offsetof(struct ParamDecOpenInit, decSeqInit) << 4) + \
	(offsetof(struct ParamDecStartFrame, streamHandle) << 5) + \
	(offsetof(struct ParamDecStartFrame, linearHandle) << 6) + \
	(offsetof(struct ParamDecStartFrame, vaSliceNum) << 7) + \
	(offsetof(struct ParamDecStartFrame, offsetCb) << 8) + \
	(offsetof(struct ParamDecStartFrame, renderTargetNum) << 9)

#define MTVPU_ENC_VERSION2_CMD_CHECKSUM \
	(sizeof(struct ParamEncOpen) << 0) + \
	(sizeof(struct ParamEncClose) << 1) + \
	(sizeof(struct ParamEncStartFrameV2) << 2) + \
	(offsetof(struct ParamEncOpen, encWorkHandle) << 3) + \
	(offsetof(struct ParamEncOpen, encWorkSize) << 4) + \
	(offsetof(struct ParamEncStartFrameV2, seqParamNum) << 5) + \
	(offsetof(struct ParamEncStartFrameV2, sliceParamNum) << 6) + \
	(offsetof(struct ParamEncStartFrameV2, packedPicParamNum) << 7) + \
	(offsetof(struct ParamEncStartFrameV2, seqParamHandle) << 8) + \
	(offsetof(struct ParamEncStartFrameV2, sliceParamHandle) << 9) + \
	(offsetof(struct ParamEncStartFrameV2, miscParamTypeFrameRateHandle) << 10) + \
	(offsetof(struct ParamEncStartFrameV2, reconTargetMvColHandle) << 11) + \
	(offsetof(struct ParamEncStartFrameV2, paUpdateArray) << 12) + \
	(offsetof(struct ParamEncStartFrameV2, streamSize) << 13) + \
	(offsetof(struct ParamEncStartFrameV2, renderTargetFBCInfo) << 14) + \
	(offsetof(struct ParamEncStartFrameV2, intraRefreshMode) << 15) + \
	(offsetof(struct ParamEncStartFrameV2, encDefCdfHandle) << 16) + \
	(offsetof(struct ParamEncStartFrameV2, srcYSize) << 17) + \
	(offsetof(struct ParamEncStartFrameV2, reconTargetFbcYHandleSize) << 18) + \
	(offsetof(struct ParamEncStartFrameV2, reconTargetFbcYOffsetSize) << 19)

typedef enum _mtvpu_cmd_type_
{
	MTVPU_CMD_INVALID               = 0,
	MTVPU_CMD_REQUESET_CORE         = 1,
	MTVPU_CMD_UNREF_CORE            = 2,
	MTVPU_CMD_DECODE_OPEN           = 3,
	MTVPU_CMD_DECODE_CLOSE          = 4,
	MTVPU_CMD_DECODE_INIT_SEQ       = 5,
	MTVPU_CMD_DECODE_START_FRAME    = 6,
	MTVPU_CMD_ENCODE_OPEN           = 7,
	MTVPU_CMD_ENCODE_CLOSE          = 8,
	MTVPU_CMD_ENCODE_INIT_SEQ       = 9,
	MTVPU_CMD_ENCODE_START_FRAME    = 10,
	MTVPU_CMD_JPU_DECODE_OPEN       = 11,
	MTVPU_CMD_JPU_DECODE_CLOSE      = 12,
	MTVPU_CMD_JPU_DECODE_START      = 13,
	MTVPU_CMD_JPU_ENCODE_OPEN       = 14,
	MTVPU_CMD_JPU_ENCODE_CLOSE      = 15,
	MTVPU_CMD_JPU_ENCODE_START      = 16,
	MTVPU_CMD_GET_DEVICE_INFO       = 17,
	MTVPU_CMD_DECODE_OPENV2         = 18,
	MTVPU_CMD_ENCODE_INIT_SEQV2     = 19,
	MTVPU_CMD_ENCODE_OPENV2         = 20,
	MTVPU_CMD_ENCODE_START_FRAME_V2 = 21,
	MTVPU_CMD_BUF_INIT              = 30,
	MTVPU_CMD_BUF_RESET             = 31,
	MTVPU_CMD_BUF_MODIFY            = 32,
} mtvpu_cmd_type;

#define DRM_IOCTL_MT_GEM_CREATE_DUMB \
	DRM_IOWR(DRM_COMMAND_BASE + 0x40, struct drm_mode_create_dumb)
#define DRM_IOCTL_MT_GEM_MAP_DUMB    \
	DRM_IOWR(DRM_COMMAND_BASE + 0x41, struct drm_mode_map_dumb)
#define DRM_IOCTL_MT_GEM_DESTROY_DUMB \
	DRM_IOWR(DRM_COMMAND_BASE + 0x42, struct drm_mode_destroy_dumb)
#define DRM_IOCTL_MT_GEM_BUSY \
	DRM_IOWR(DRM_COMMAND_BASE + 0x43, struct mtvpu_gem_busy)
#define DRM_IOCTL_MT_GEM_WAIT_DUMB \
	DRM_IOWR(DRM_COMMAND_BASE + 0x44, struct mtvpu_gem_wait)

#define DRM_IOCTL_MT_VPU_DEC_OPEN \
	DRM_IOWR(DRM_COMMAND_BASE + 0x45, struct ParamDecOpen)
#define DRM_IOCTL_MT_VPU_DEC_CLOSE \
	DRM_IOWR(DRM_COMMAND_BASE + 0x46, struct ParamDecClose)
#define DRM_IOCTL_MT_VPU_DEC_INIT_SEQ \
	DRM_IOWR(DRM_COMMAND_BASE + 0x47, struct ParamDecInitSeq)
#define DRM_IOCTL_MT_VPU_DEC_START_FRAME \
	DRM_IOWR(DRM_COMMAND_BASE + 0x48, struct ParamDecStartFrame)

#define DRM_IOCTL_MT_VPU_DMA_TRANS \
	DRM_IOWR(DRM_COMMAND_BASE + 0x49, struct mtvpu_gem_dma)
#define DRM_IOCTL_MT_VPU_MGT_GET_INFO \
	DRM_IOWR(DRM_COMMAND_BASE + 0x4a, struct mtvpu_mgt_info)

#define DRM_IOCTL_MT_VPU_MGT_MKIS_PROBE \
	DRM_IOWR(DRM_COMMAND_BASE + 0x4b, struct mtvpu_mkis_probe)
#define DRM_IOCTL_MT_VPU_MGT_MKIS_LIST \
	DRM_IOWR(DRM_COMMAND_BASE + 0x4c, struct mtvpu_mkis_list)

#define DRM_IOCTL_MT_SEMA_CMD \
	DRM_IOWR(DRM_COMMAND_BASE + 0x4f, struct mtvpu_sema_cmd)

#define DRM_IOCTL_MT_VPU_ENC_OPEN \
	DRM_IOWR(DRM_COMMAND_BASE + 0x50, struct ParamEncOpen)
#define DRM_IOCTL_MT_VPU_ENC_CLOSE \
	DRM_IOWR(DRM_COMMAND_BASE + 0x51, struct ParamEncClose)
#define DRM_IOCTL_MT_VPU_ENC_INIT_SEQ \
	DRM_IOWR(DRM_COMMAND_BASE + 0x52, struct ParamEncInitSeq)
#define DRM_IOCTL_MT_VPU_ENC_START_FRAME \
	DRM_IOWR(DRM_COMMAND_BASE + 0x53, struct ParamEncStartFrame)
#define DRM_IOCTL_MT_VPU_ENC_START_FRAME_V2 \
	DRM_IOWR(DRM_COMMAND_BASE + 0x5e, struct ParamEncStartFrameV2)

#define DRM_IOCTL_MT_VPU_REQUEST_CORE \
	DRM_IOWR(DRM_COMMAND_BASE + 0x54, struct mtvpu_request_core)

#define DRM_IOCTL_MT_GEM_MODIFY_DUMB \
	DRM_IOWR(DRM_COMMAND_BASE + 0x56, struct mtvpu_modify_dump)
#define DRM_IOCTL_MT_VPU_GET_DEVINFO \
	DRM_IOWR(DRM_COMMAND_BASE + 0x57, struct mtvpu_get_devinfo)
#define DRM_IOCTL_MT_VPU_UNREF_CORE \
	DRM_IOWR(DRM_COMMAND_BASE + 0x58, struct mtvpu_unref_core)

#define DRM_IOCTL_MT_JPU_ENC_OPEN \
	DRM_IOWR(DRM_COMMAND_BASE + 0x59, struct JpuParamEncOpen)
#define DRM_IOCTL_MT_JPU_ENC_START \
	DRM_IOWR(DRM_COMMAND_BASE + 0x5a, struct JpuParamEncStart)
#define DRM_IOCTL_MT_JPU_ENC_CLOSE \
	DRM_IOWR(DRM_COMMAND_BASE + 0x5c, struct JpuParamEncClose)
#define DRM_IOCTL_MT_JPU_DEC_OPEN \
	DRM_IOWR(DRM_COMMAND_BASE + 0x4d, struct JpuParamDecOpen)
#define DRM_IOCTL_MT_JPU_DEC_START \
	DRM_IOWR(DRM_COMMAND_BASE + 0x4e, struct JpuParamDecStart)
#define DRM_IOCTL_MT_JPU_DEC_CLOSE \
	DRM_IOWR(DRM_COMMAND_BASE + 0x5d, struct JpuParamDecClose)

#endif

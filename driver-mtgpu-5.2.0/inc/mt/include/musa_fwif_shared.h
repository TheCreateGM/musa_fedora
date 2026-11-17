/*************************************************************************/ /*!
@File
@Title          MUSA firmware interface structures
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    MUSA firmware interface structures shared by both host client
                and host server
@License        Dual MIT/GPLv2

The contents of this file are subject to the MIT license as set out below.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

Alternatively, the contents of this file may be used under the terms of
the GNU General Public License Version 2 ("GPL") in which case the provisions
of GPL are applicable instead of those above.

If you wish to allow use of your version of this file only under the terms of
GPL, and not to allow others to use your version of this file under the terms
of the MIT license, indicate your decision by deleting the provisions above
and replace them with the notice and other provisions required by GPL as set
out in the file called "GPL-COPYING" included in this distribution. If you do
not delete the provisions above, a recipient may use your version of this file
under the terms of either the MIT license or GPL.

This License is also included in this distribution in the file called
"MIT-COPYING".

EXCEPT AS OTHERWISE STATED IN A NEGOTIATED AGREEMENT: (A) THE SOFTWARE IS
PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT; AND (B) IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/ /**************************************************************************/

#if !defined(MUSA_FWIF_SHARED_H)
#define MUSA_FWIF_SHARED_H

#include "img_types.h"
#include "img_defs.h"
#include "musa_common.h"
#include "mt/mem_types.h"

/* Indicates the number of RTDATAs per RTDATASET */
#if defined(SUPPORT_AGP)
#if defined(SUPPORT_AGP4)
#define MUSAMKIF_NUM_RTDATAS           4U
#define MUSAMKIF_NUM_GEOMDATAS         4U
#define MUSAMKIF_NUM_RTDATA_FREELISTS  20U /* MUSAMKIF_NUM_RTDATAS * MUSAFW_MAX_FREELISTS */
#define MUSA_NUM_GEOM_CORES           (4U)
#else
#define MUSAMKIF_NUM_RTDATAS           4U
#define MUSAMKIF_NUM_GEOMDATAS         4U
#define MUSAMKIF_NUM_RTDATA_FREELISTS  12U /* MUSAMKIF_NUM_RTDATAS * MUSAFW_MAX_FREELISTS */
#define MUSA_NUM_GEOM_CORES           (2U)
#endif
#else
#define MUSAMKIF_NUM_RTDATAS           2U
#define MUSAMKIF_NUM_GEOMDATAS         1U
#define MUSAMKIF_NUM_RTDATA_FREELISTS  16U  /* MUSA_MAX_NUM_CORES * MUSAFW_MAX_FREELISTS */
#define MUSA_NUM_GEOM_CORES           (1U)
#endif

/* Maximum number of UFOs in a CCB command.
 * The number is based on having 32 sync prims (as originally), plus 32 sync
 * checkpoints.
 * Once the use of sync prims is no longer supported, we will retain
 * the same total (64) as the number of sync checkpoints which may be
 * supporting a fence is not visible to the client driver and has to
 * allow for the number of different timelines involved in fence merges.
 */
#define MUSAFWIF_CCB_CMD_MAX_UFOS			(32U+32U)

/*
 * This is a generic limit imposed on any DM (TA,3D,CMP,TRANSFER,2D,TRANSFER)
 * command passed through the bridge.
 * Just across the bridge in the server, any incoming kick command size is
 * checked against this maximum limit.
 * In case the incoming command size is larger than the specified limit,
 * the bridge call is retired with error.
 * 
 * The size should be less than 170K theoretically.
 */
#define MUSAFWIF_DM_INDEPENDENT_KICK_CMD_SIZE	(256 * 1024U)

typedef struct MUSAFWIF_DEV_VIRTADDR_
{
	MT_UINT32	ui32Addr;
} MUSAFWIF_DEV_VIRTADDR;

typedef struct
{
	uint64_t        MUSAFW_ALIGN psDevVirtAddr;
	MUSAFWIF_DEV_VIRTADDR    pbyFWAddr;
} UNCACHED_ALIGN MUSAFWIF_DMA_ADDR;

typedef MT_UINT8	MUSAFWIF_CCCB;

typedef MUSAFWIF_DEV_VIRTADDR  PMUSAFWIF_UFO_ADDR;
typedef MUSAFWIF_DEV_VIRTADDR  PMUSAFWIF_CLEANUP_CTL;


/*!
 * @InGroup ClientCCBTypes
 * @Brief Command data for fence & update types Client CCB commands.
 */
typedef struct
{
	PMUSAFWIF_UFO_ADDR	puiAddrUFO; /*!< Address to be checked/updated */
	MT_UINT32			ui32Value;  /*!< Value to check-against/update-to */
} MUSAFWIF_UFO;

typedef struct
{
	MT_UINT32			ui32SubmittedCommands;	/*!< Number of commands received by the FW */
	MT_UINT32			ui32ExecutedCommands;	/*!< Number of commands executed by the FW */
} UNCACHED_ALIGN MUSAFWIF_CLEANUP_CTL;

#define	MUSAFWIF_PRBUFFER_START        MT_UINT32_C(0)
#define	MUSAFWIF_PRBUFFER_ZSBUFFER     MT_UINT32_C(0)
#define	MUSAFWIF_PRBUFFER_MSAABUFFER   MT_UINT32_C(1)
#define	MUSAFWIF_PRBUFFER_MAXSUPPORTED MT_UINT32_C(2)

typedef MT_UINT32 MUSAFWIF_PRBUFFER_TYPE;

typedef enum
{
	MUSAFWIF_PRBUFFER_UNBACKED = 0,
	MUSAFWIF_PRBUFFER_BACKED,
	MUSAFWIF_PRBUFFER_BACKING_PENDING,
	MUSAFWIF_PRBUFFER_UNBACKING_PENDING,
}MUSAFWIF_PRBUFFER_STATE;

/*!
 * @InGroup RenderTarget
 * @Brief OnDemand Z/S/MSAA Buffers
 */
typedef struct
{
	MT_UINT32				ui32BufferID;				/*!< Buffer ID*/
	MT_BOOL				bOnDemand;					/*!< Needs On-demand Z/S/MSAA Buffer allocation */
	MUSAFWIF_PRBUFFER_STATE	eState;						/*!< Z/S/MSAA -Buffer state */
	MUSAFWIF_CLEANUP_CTL		sCleanupState;				/*!< Cleanup state */
	MT_UINT32				ui32PRBufferFlags;		/*!< Compatibility and other flags */
} UNCACHED_ALIGN MUSAFWIF_PRBUFFER;

/*
 * Used to share frame numbers across UM-KM-FW,
 * frame number is set in UM,
 * frame number is required in both KM for HTB and FW for FW trace.
 *
 * May be used to house Kick flags in the future.
 */
typedef struct
{
	MT_UINT32 ui32FrameNum; /*!< associated frame number */
} CMD_COMMON;

/*
 * TA and 3D commands require set of firmware addresses that are stored in the
 * Kernel. Client has handle(s) to Kernel containers storing these addresses,
 * instead of raw addresses. We have to patch/write these addresses in KM to
 * prevent UM from controlling FW addresses directly.
 * Typedefs for TA and 3D commands are shared between Client and Firmware (both
 * single-BVNC). Kernel is implemented in a multi-BVNC manner, so it can't use
 * TA|3D CMD type definitions directly. Therefore we have a SHARED block that
 * is shared between UM-KM-FW across all BVNC configurations.
 */
typedef struct
{
	CMD_COMMON           sCmn;      /*!< Common command attributes */
	MUSAFWIF_DEV_VIRTADDR sHWRTData; /* RTData associated with this command,
									   this is used for context selection and for storing out HW-context,
									   when TA is switched out for continuing later */

	MUSAFWIF_DEV_VIRTADDR asPRBuffer[MUSAFWIF_PRBUFFER_MAXSUPPORTED];	/* Supported PR Buffers like Z/S/MSAA Scratch */

} CMDGFX_SHARED;

/*!
 * Client Circular Command Buffer (CCCB) control structure.
 * This is shared between the Server and the Firmware and holds byte offsets
 * into the CCCB as well as the wrapping mask to aid wrap around. A given
 * snapshot of this queue with Cmd 1 running on the GPU might be:
 *
 *          Roff                           Doff                 Woff
 * [..........|-1----------|=2===|=3===|=4===|~5~~~~|~6~~~~|~7~~~~|..........]
 *            <      runnable commands       ><   !ready to run   >
 *
 * Cmd 1    : Currently executing on the GPU data master.
 * Cmd 2,3,4: Fence dependencies met, commands runnable.
 * Cmd 5... : Fence dependency not met yet.
 */
typedef struct
{
	MT_UINT32  ui32WriteOffset;    /*!< Host write offset into CCB. This
	                                 *    must be aligned to 16 bytes. */
	MT_UINT32  ui32ReadOffset;     /*!< Firmware read offset into CCB.
	                                      Points to the command that is
	                                      runnable on GPU, if R!=W */
	MT_UINT32  ui32DepOffset;      /*!< Firmware fence dependency offset.
	                                      Points to commands not ready, i.e.
	                                      fence dependencies are not met. */
	MT_UINT32  ui32WrapMask;       /*!< Offset wrapping mask, total capacity
	                                      in bytes of the CCB-1 */

	/* Used for CMP multiqueue. */
	MT_UINT32  ui32ReadOffset2;
	MT_UINT32  ui32ReadOffset3;
	MT_UINT32  ui32ReadOffset4;

	/* Only used for linux guest hwr */
#if (MUSA_NUM_OS_SUPPORTED > 1)
	MT_UINT32 ui32WriteOffsetBackup;
	MT_UINT32 ui32ReadOffsetBackup;
	MT_UINT32 ui32DepOffsetBackup;
	MT_UINT32 ui32LastReadOffsetNeedSkip;
	MT_BOOL   bNeedSkip;
	MT_BOOL   bNeedReset;
#endif
} UNCACHED_ALIGN MUSAFWIF_CCCB_CTL;


typedef MT_UINT32 MUSAFW_FREELIST_TYPE;

#define MUSAFW_LOCAL_FREELIST     MT_UINT32_C(0)
#define MUSAFW_GLOBAL_FREELIST    MT_UINT32_C(1)
#if defined(SUPPORT_AGP4)
#define MUSAFW_GLOBAL2_FREELIST   MT_UINT32_C(2)
#define MUSAFW_GLOBAL3_FREELIST   MT_UINT32_C(3)
#define MUSAFW_GLOBAL4_FREELIST   MT_UINT32_C(4)
#define MUSAFW_MAX_FREELISTS      (MUSAFW_GLOBAL4_FREELIST + 1U)
#elif defined(SUPPORT_AGP)
#define MUSAFW_GLOBAL2_FREELIST   MT_UINT32_C(2)
#define MUSAFW_MAX_FREELISTS      (MUSAFW_GLOBAL2_FREELIST + 1U)
#else
#define MUSAFW_LOCALMCG2_FREELIST  MT_UINT32_C(2)
#define MUSAFW_LOCALMCG3_FREELIST  MT_UINT32_C(3)
#define MUSAFW_LOCALMCG4_FREELIST  MT_UINT32_C(4)
#define MUSAFW_LOCALMCG5_FREELIST  MT_UINT32_C(5)
#define MUSAFW_LOCALMCG6_FREELIST  MT_UINT32_C(6)
#define MUSAFW_LOCALMCG7_FREELIST  MT_UINT32_C(7)
#define MUSAFW_LOCALMCG8_FREELIST  MT_UINT32_C(8)
#define MUSAFW_GLOBALMCG2_FREELIST MT_UINT32_C(9)
#define MUSAFW_GLOBALMCG3_FREELIST MT_UINT32_C(10)
#define MUSAFW_GLOBALMCG4_FREELIST MT_UINT32_C(11)
#define MUSAFW_GLOBALMCG5_FREELIST MT_UINT32_C(12)
#define MUSAFW_GLOBALMCG6_FREELIST MT_UINT32_C(13)
#define MUSAFW_GLOBALMCG7_FREELIST MT_UINT32_C(14)
#define MUSAFW_GLOBALMCG8_FREELIST MT_UINT32_C(15)

#define MUSAFW_MAX_FREELISTS   (MUSAFW_GLOBALMCG8_FREELIST + 1U)
#endif
#define MUSAFW_MAX_HWFREELISTS    (2U)

/*!
 * @Defgroup ContextSwitching Context switching data interface
 * @Brief Types grouping data structures and defines used in realising the Context Switching (CSW) functionality
 * @{
 */

/*!
 * @Brief GEOM DM or TA register controls for context switch
 */
typedef struct
{
	MT_UINT64	uTAReg_DCE_CONTEXT_STATE_BASE_ADDR;
	MT_UINT64	uTAReg_TA_CONTEXT_STATE_BASE_ADDR; /*!< The base address of the TA's context state buffer */

	struct
	{
		MT_UINT64	uTAReg_DCE_CONTEXT_STORE_TASK_GTCV0;
		MT_UINT64	uTAReg_DCE_CONTEXT_STORE_TASK_GTCV1;
		MT_UINT64	uTAReg_DCE_CONTEXT_STORE_TASK_GTCV2;

		MT_UINT64	uTAReg_DCE_CONTEXT_STORE_TASK_DDM0;
		MT_UINT64	uTAReg_DCE_CONTEXT_STORE_TASK_DDM1;
		MT_UINT64	uTAReg_DCE_CONTEXT_STORE_TASK_DDM2;
		MT_UINT64	uTAReg_DCE_CONTEXT_STORE_TASK_XFB;

		/* GTCV resume state update controls */
		MT_UINT64	uTAReg_DCE_CONTEXT_RESUME_TASK_GTCV0;
		MT_UINT64	uTAReg_DCE_CONTEXT_RESUME_TASK_GTCV1;
		MT_UINT64	uTAReg_DCE_CONTEXT_RESUME_TASK_GTCV2;


		MT_UINT64	uTAReg_DCE_CONTEXT_RESUME_TASK_DDM0;
		MT_UINT64	uTAReg_DCE_CONTEXT_RESUME_TASK_DDM1;
		MT_UINT64	uTAReg_DCE_CONTEXT_RESUME_TASK_DDM2;
		MT_UINT64	uTAReg_DCE_CONTEXT_RESUME_TASK_XFB;
	} asTAState[2];

} MUSAFWIF_TAREGISTERS_CSWITCH;
/*! @} End of Defgroup ContextSwitching */

typedef struct
{
	MT_UINT64	u3DReg_IPP_CONTEXT_ADDR;
} MUSAFWIF_3DREGISTERS_CSWITCH;

typedef struct
{
	MT_UINT64	uCMPReg_CMP_CONTEXT_PDS0;
	MT_UINT64	uCMPReg_CMP_CONTEXT_PDS1;
	MT_UINT64	uCMPReg_CMP_TERMINATE_PDS;
	MT_UINT64	uCMPReg_CMP_TERMINATE_PDS1;

	/* CMP resume controls */
	MT_UINT64	uCMPReg_CMP_RESUME_PDS0;
	MT_UINT64	uCMPReg_CMP_CONTEXT_PDS0_B;
	MT_UINT64	uCMPReg_CMP_RESUME_PDS0_B;

} MUSAFWIF_CMP_REGISTERS_CSWITCH;

static_assert((sizeof(MUSAFWIF_CMP_REGISTERS_CSWITCH) % 8U) == 0U,
			"the size of the structure must be multiple of 8");

#define MUSAFWIF_CMP_REGISTERS_CSWITCH_SIZE sizeof(MUSAFWIF_CMP_REGISTERS_CSWITCH)

/*!
 * @InGroup ContextSwitching
 * @Brief Render context static register controls for context switch
 */
typedef struct
{
	MUSAFWIF_TAREGISTERS_CSWITCH MUSAFW_ALIGN asCtxSwitch_GeomRegs[MUSA_NUM_GEOM_CORES];
	MUSAFWIF_3DREGISTERS_CSWITCH MUSAFW_ALIGN sCtxSwitch_3DRegs;  /*!< 3D registers for ctx switch */
} MUSAFWIF_STATIC_RENDERCONTEXT_STATE;

#define MUSAFWIF_STATIC_RENDERCONTEXT_SIZE sizeof(MUSAFWIF_STATIC_RENDERCONTEXT_STATE)

typedef struct
{
	MUSAFWIF_CMP_REGISTERS_CSWITCH	MUSAFW_ALIGN sCtxSwitch_Regs;	/*!< CMP registers for ctx switch */
} MUSAFWIF_STATIC_COMPCONTEXT_STATE;

#define MUSAFWIF_STATIC_COMPCONTEXT_SIZE sizeof(MUSAFWIF_STATIC_COMPCONTEXT_STATE)

typedef struct
{
	MT_UINT64	uRDMReg_RDM_CONTEXT_STATE_BASE_ADDR;
} MUSAFWIF_RDM_REGISTERS_CSWITCH;

static_assert((sizeof(MUSAFWIF_RDM_REGISTERS_CSWITCH) % 8U) == 0U,
			"the size of the structure must be multiple of 8");

#define MUSAFWIF_RDM_REGISTERS_CSWITCH_SIZE sizeof(MUSAFWIF_RDM_REGISTERS_CSWITCH)

typedef struct
{
	MUSAFWIF_RDM_REGISTERS_CSWITCH	MUSAFW_ALIGN sCtxSwitch_Regs;	/*!< RDM registers for ctx switch */
} MUSAFWIF_STATIC_RAYCONTEXT_STATE;

#define MUSAFWIF_STATIC_RAYCONTEXT_SIZE sizeof(MUSAFWIF_STATIC_RAYCONTEXT_STATE)

/*!
	@Brief Context reset reason. Last reset reason for a reset context.
*/
#define    MUSA_CONTEXT_RESET_REASON_NONE                     0x000  /*!< No reset reason recorded */
#define    MUSA_CONTEXT_RESET_REASON_GUILTY_LOCKUP            0x001  /*!< Caused a reset due to locking up */
#define    MUSA_CONTEXT_RESET_REASON_INNOCENT_LOCKUP          0x002  /*!< Affected by another context locking up */
#define    MUSA_CONTEXT_RESET_REASON_GUILTY_OVERRUNING        0x004  /*!< Overran the global deadline */
#define    MUSA_CONTEXT_RESET_REASON_INNOCENT_OVERRUNING      0x008  /*!< Affected by another context overrunning */
#define    MUSA_CONTEXT_RESET_REASON_HARD_CONTEXT_SWITCH      0x010  /*!< Forced reset to ensure scheduling requirements */
#define    MUSA_CONTEXT_RESET_REASON_WGP_CHECKSUM             0x020  /*!< CMP Mission/safety checksum mismatch */
#define    MUSA_CONTEXT_RESET_REASON_TRP_CHECKSUM             0x040  /*!< TRP checksum mismatch */
#define    MUSA_CONTEXT_RESET_REASON_GPU_ECC_OK               0x080  /*!< GPU ECC error (corrected, OK) */
#define    MUSA_CONTEXT_RESET_REASON_GPU_ECC_HWR              0x100  /*!< GPU ECC error (uncorrected, HWR) */
#define    MUSA_CONTEXT_RESET_REASON_FW_ECC_OK                0x200 /*!< FW ECC error (corrected, OK) */
#define    MUSA_CONTEXT_RESET_REASON_FW_ECC_ERR               0x400 /*!< FW ECC error (uncorrected, ERR) */
#define    MUSA_CONTEXT_RESET_REASON_FW_WATCHDOG              0x800 /*!< FW Safety watchdog triggered */
#define    MUSA_CONTEXT_RESET_REASON_FW_PAGEFAULT             0x001000 /*!< FW page fault (no HWR) */
#define    MUSA_CONTEXT_RESET_REASON_FW_EXEC_ERR              0x002000 /*!< FW execution error (GPU reset requested) */
#define    MUSA_CONTEXT_RESET_REASON_HOST_WDG_FW_ERR          0x004000 /*!< Host watchdog detected FW error */
#define    MUSA_CONTEXT_GEOM_OOM_DISABLED                     0x008000 /*!< Geometry DM OOM event is not allowed */
#define    MT_MP_EXCEPTION_WRITE                             0x010000 /*!< ICTRL/RTU/LSU/TCE/MOV */
#define    MT_MP_EXCEPTION_READ                              0x020000 /*!< ICTRL */
#define    MT_MP_EXCEPTION_US_RW_CONFLICT                    0x040000 /*!< US */
#define    MT_MP_EXCEPTION_LMS_RW_NOBARRIER                  0x100000 /*!< LMS */
#define    MT_MP_EXCEPTION_LMS_OOB                           0x200000 /*!< LMA/TME access lms oob */
#define    MT_MP_EXCEPTION_ILLEGAL_INSTRUCTION               0x400000 /*!< ICTRL catch decode error */
#define    MT_MP_EXCEPTION_INVALID_PC                        0x00800000 /*!< ICTRL get invalid pc  */
#define    MT_MP_EXCEPTION_INVALID_MEMORY_SPACE              0x01000000 /*!< LMA: address target to unsupported memory space for pipeline */
#define    MT_MP_EXCEPTION_MISALIGNED_ADDRESS                0x02000000 /*!< LMA/TME/LSU misaligned address */
#define    MT_MP_EXCEPTION_ASYNC_TRANS_BAR_SIZE_OOB          0x04000000 /*!< TME/ASYNC: bar id is greater than bar size */
#define    MT_MP_EXCEPTION_ASYNC_TRANS_BAR_TRANS_OOB         0x08000000 /*!< ASYNC: transaction count is overflow or underflow */
#define    MT_MP_EXCEPTION_ASYNC_TRANS_BAR_ARRIVAL_OOB       0x10000000 /*!< ASYNC: arrival count is overflow or underflow */
#define    MT_MP_EXCEPTION_TCE_ILLEGAL_ACTIVE_MASK           0x20000000 /*!< TCE: detect illegal active mask */
#define    MT_MP_EXCEPTION_TME_ILLEGAL_PARAMETERS            0x40000000 /*!< TME: detect illegal parameters */
#define    MUSA_CONTEXT_ACE_STATUS_ERR                        0x80000000 /*!< AXI bus status error */

typedef struct
{
	/* FIXME: To ensure the normal compilation of other modules in gr-umd, we temporarily need to
	 *        use hard core to define the dimensions of the MP_SLOT registers, the reason is that
	 *        using macro definitions could lead to compilation failures in other modules.
	 *        The first dimension is MUSA_MAX_NUM_CORES, the second is MUSA_FEATURE_NUM_CLUSTERS,
	 *        the last is MUSA_FEATURE_MP_SLOTS. */
	/* TODO: Move the definition of MUSA_CONTEXT_RESET_REASON_DATA to shared_include, and replace
	 *       the hard code by macro definitions */
	MT_UINT32 ui32UscWatchDogStatus[8];
	MT_UINT64 ui64UscSlot[8][8][96];
} MP_DUMP;

typedef struct
{
	MT_UINT64 ui64Addr;
	MT_UINT64 bfPC : 32;
	MT_UINT64 bfRead : 1;
	MT_UINT64 bfFault : 1;
	MT_UINT64 bfROFault : 1;
	MT_UINT64 bfProtFault : 1;
	MT_UINT64 bfPageFault : 1;
	MT_CHAR pszTagID[16];
	MT_CHAR pszMMULevel[16];
	MT_CHAR pszTagSB[32];
} PAGE_FAULT_INFO;

typedef struct
{
	MT_UINT64 ui64MmuFaultStatus1[8];
	MT_UINT64 ui64MmuFaultStatus2[8];
	PAGE_FAULT_INFO sPageFaultPrint[8];
} MMU_FAULT_DUMP;

/*!
	@Brief Context reset data shared with the host
*/
typedef struct
{
	MT_UINT64 ui64ResetReason;       /*!< Reset reason */
	MT_UINT32 ui32ResetExtJobRef;    /*!< External Job ID */
	MT_BOOL bIsPageFault;
	MT_UINT64 ui64TimeStamp;
	union CONTEXT_RESET_INFO
	{
		MP_DUMP sUscDump;
		MMU_FAULT_DUMP sMmuFault;
	} sCtxResetInfo;
} MUSA_CONTEXT_RESET_REASON_DATA;
#endif /*  MUSA_FWIF_SHARED_H */

/******************************************************************************
 End of file (musa_fwif_shared.h)
******************************************************************************/

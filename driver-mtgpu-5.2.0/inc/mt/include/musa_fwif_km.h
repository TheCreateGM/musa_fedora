/*************************************************************************/ /*!
@File
@Title          MUSA firmware interface structures used by mtgpukm
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    MUSA firmware interface structures used by mtgpukm
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

#if !defined(MUSA_FWIF_KM_H)
#define MUSA_FWIF_KM_H

#include "img_types.h"
#include "musa_fwif_shared.h"
#include "musadefs_km.h"
#include "dllist.h"
#include "musa_hwperf.h"


/*************************************************************************/ /*!
 Logging type
*/ /**************************************************************************/
#define MUSAFWIF_LOG_TYPE_NONE			0x00000000U
#define MUSAFWIF_LOG_TYPE_TRACE			0x00000001U
#define MUSAFWIF_LOG_TYPE_GROUP_MAIN		0x00000002U
#define MUSAFWIF_LOG_TYPE_GROUP_MTS		0x00000004U
#define MUSAFWIF_LOG_TYPE_GROUP_CLEANUP	0x00000008U
#define MUSAFWIF_LOG_TYPE_GROUP_CSW		0x00000010U
#define MUSAFWIF_LOG_TYPE_GROUP_BIF		0x00000020U
#define MUSAFWIF_LOG_TYPE_GROUP_PM		0x00000040U
#define MUSAFWIF_LOG_TYPE_GROUP_RTD		0x00000080U
#define MUSAFWIF_LOG_TYPE_GROUP_SPM		0x00000100U
#define MUSAFWIF_LOG_TYPE_GROUP_POW		0x00000200U
#define MUSAFWIF_LOG_TYPE_GROUP_HWR		0x00000400U
#define MUSAFWIF_LOG_TYPE_GROUP_HWP		0x00000800U
#define MUSAFWIF_LOG_TYPE_GROUP_RPM		0x00001000U
#define MUSAFWIF_LOG_TYPE_GROUP_DMA		0x00002000U
#define MUSAFWIF_LOG_TYPE_GROUP_MISC		0x00004000U
#define MUSAFWIF_LOG_TYPE_GROUP_MCG		0x00008000U
#define MUSAFWIF_LOG_TYPE_GROUP_DEBUG	0x80000000U
#define MUSAFWIF_LOG_TYPE_GROUP_MASK		0x8000FFFEU
#define MUSAFWIF_LOG_TYPE_MASK			0x8000FFFFU

/* String used in mtdebug -h output */
#define MUSAFWIF_LOG_GROUPS_STRING_LIST   "main,mts,cleanup,csw,bif,pm,rtd,spm,pow,hwr,hwp,rpm,dma,misc,mcg,debug"

/* Table entry to map log group strings to log type value */
typedef struct {
	const MT_CHAR* pszLogGroupName;
	MT_UINT32      ui32LogGroupType;
} MUSAFWIF_LOG_GROUP_MAP_ENTRY;

/*
  Macro for use with the MUSAFWIF_LOG_GROUP_MAP_ENTRY type to create a lookup
  table where needed. Keep log group names short, no more than 20 chars.
*/
#define MUSAFWIF_LOG_GROUP_NAME_VALUE_MAP { "none",    MUSAFWIF_LOG_TYPE_NONE }, \
                                         { "main",    MUSAFWIF_LOG_TYPE_GROUP_MAIN }, \
                                         { "mts",     MUSAFWIF_LOG_TYPE_GROUP_MTS }, \
                                         { "cleanup", MUSAFWIF_LOG_TYPE_GROUP_CLEANUP }, \
                                         { "csw",     MUSAFWIF_LOG_TYPE_GROUP_CSW }, \
                                         { "bif",     MUSAFWIF_LOG_TYPE_GROUP_BIF }, \
                                         { "pm",      MUSAFWIF_LOG_TYPE_GROUP_PM }, \
                                         { "rtd",     MUSAFWIF_LOG_TYPE_GROUP_RTD }, \
                                         { "spm",     MUSAFWIF_LOG_TYPE_GROUP_SPM }, \
                                         { "pow",     MUSAFWIF_LOG_TYPE_GROUP_POW }, \
                                         { "hwr",     MUSAFWIF_LOG_TYPE_GROUP_HWR }, \
                                         { "hwp",     MUSAFWIF_LOG_TYPE_GROUP_HWP }, \
                                         { "rpm",     MUSAFWIF_LOG_TYPE_GROUP_RPM }, \
                                         { "dma",     MUSAFWIF_LOG_TYPE_GROUP_DMA }, \
                                         { "misc",    MUSAFWIF_LOG_TYPE_GROUP_MISC }, \
                                         { "mcg",     MUSAFWIF_LOG_TYPE_GROUP_MCG }, \
                                         { "debug",   MUSAFWIF_LOG_TYPE_GROUP_DEBUG }


/* Used in print statements to display log group state, one %s per group defined */
#define MUSAFWIF_LOG_ENABLED_GROUPS_LIST_PFSPEC  "%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s"

/* Used in a print statement to display log group state, one per group */
#define MUSAFWIF_LOG_ENABLED_GROUPS_LIST(types)  (((types) & MUSAFWIF_LOG_TYPE_GROUP_MAIN)	?("main ")		:("")),		\
                                                (((types) & MUSAFWIF_LOG_TYPE_GROUP_MTS)		?("mts ")		:("")),		\
                                                (((types) & MUSAFWIF_LOG_TYPE_GROUP_CLEANUP)	?("cleanup ")	:("")),		\
                                                (((types) & MUSAFWIF_LOG_TYPE_GROUP_CSW)		?("csw ")		:("")),		\
                                                (((types) & MUSAFWIF_LOG_TYPE_GROUP_BIF)		?("bif ")		:("")),		\
                                                (((types) & MUSAFWIF_LOG_TYPE_GROUP_PM)		?("pm ")		:("")),		\
                                                (((types) & MUSAFWIF_LOG_TYPE_GROUP_RTD)		?("rtd ")		:("")),		\
                                                (((types) & MUSAFWIF_LOG_TYPE_GROUP_SPM)		?("spm ")		:("")),		\
                                                (((types) & MUSAFWIF_LOG_TYPE_GROUP_POW)		?("pow ")		:("")),		\
                                                (((types) & MUSAFWIF_LOG_TYPE_GROUP_HWR)		?("hwr ")		:("")),		\
                                                (((types) & MUSAFWIF_LOG_TYPE_GROUP_HWP)		?("hwp ")		:("")),		\
                                                (((types) & MUSAFWIF_LOG_TYPE_GROUP_RPM)		?("rpm ")		:("")),		\
                                                (((types) & MUSAFWIF_LOG_TYPE_GROUP_DMA)		?("dma ")		:("")),		\
                                                (((types) & MUSAFWIF_LOG_TYPE_GROUP_MISC)	?("misc ")		:("")),		\
                                                (((types) & MUSAFWIF_LOG_TYPE_GROUP_MCG)		?("mcg ")		:("")),		\
                                                (((types) & MUSAFWIF_LOG_TYPE_GROUP_DEBUG)	?("debug ")		:(""))


/************************************************************************
* MUSA FW signature checks
************************************************************************/
#define MUSAFW_SIG_BUFFER_SIZE_MIN       (8192)

#define MUSAFWIF_TIMEDIFF_ID			((0x1UL << 28) | MUSA_CR_TIMER)

/*!
 ******************************************************************************
 * Trace Buffer
 *****************************************************************************/

/*! Default size of MUSAFWIF_TRACEBUF_SPACE in DWords */
#define MUSAFW_TRACE_BUF_DEFAULT_SIZE_IN_DWORDS 12000U
#define MUSAFW_TRACE_BUFFER_ASSERT_SIZE 200U
#if defined(MUSAFW_MCU_SUPPORT_2ND_THREAD)
#define MUSAFW_THREAD_NUM 2U
#else
#define MUSAFW_THREAD_NUM 1U
#endif

#define MUSAFW_POLL_TYPE_SET 0x80000000U

typedef struct
{
	MT_CHAR	szPath[MUSAFW_TRACE_BUFFER_ASSERT_SIZE];
	MT_CHAR	szInfo[MUSAFW_TRACE_BUFFER_ASSERT_SIZE];
	MT_UINT32	ui32LineNum;
} UNCACHED_ALIGN MUSAFWIF_FILE_INFO_BUF;

/*!
 * @Defgroup SRVAndFWTracing Services and Firmware Tracing data interface
 * @Brief The document groups/lists the data structures and the interfaces related to Services and Firmware Tracing
 * @{
 */

/*!
 * @Brief Firmware trace buffer details
 */
typedef struct
{
	MT_UINT32            ui32TracePointer;          /*!< Trace pointer (write index into Trace Buffer)*/

#if defined(MUSA_FIRMWARE)
	MT_UINT32            *pui32MUSAFWIfTraceBuffer;  /*!< Trace buffer address (FW address), to be used by firmware for writing into trace buffer */
#else
	MUSAFWIF_DEV_VIRTADDR  pui32MUSAFWIfTraceBuffer;   /*!< Trace buffer address (FW address)*/
#endif
	MT_PUINT32           pui32TraceBuffer;          /*!< Trace buffer address (Host address), to be used by host when reading from trace buffer */

	MUSAFWIF_FILE_INFO_BUF sAssertBuf;
} UNCACHED_ALIGN MUSAFWIF_TRACEBUF_SPACE;

/*! @} End of Defgroup SRVAndFWTracing */

#define MUSAFWIF_FWFAULTINFO_MAX		(8U)			/* Total number of FW fault logs stored */

typedef struct
{
	MT_UINT64 MUSAFW_ALIGN	ui64CRTimer;
	MT_UINT64 MUSAFW_ALIGN	ui64OSTimer;
	MT_UINT32 MUSAFW_ALIGN	ui32Data;
	MT_UINT32 ui32Reserved;
	MUSAFWIF_FILE_INFO_BUF	sFaultBuf;
} UNCACHED_ALIGN MUSA_FWFAULTINFO;


#define MUSAFWIF_POW_STATES \
  X(MUSAFWIF_POW_OFF)			/* idle and handshaked with the host (ready to full power down) */ \
  X(MUSAFWIF_POW_ON)				/* running HW commands */ \
  X(MUSAFWIF_POW_FORCED_IDLE)	/* forced idle */ \
  X(MUSAFWIF_POW_IDLE)			/* idle waiting for host handshake */

typedef enum
{
#define X(NAME) NAME,
	MUSAFWIF_POW_STATES
#undef X
} MUSAFWIF_POW_STATE;

/* Firmware HWR states */
#define MUSAFWIF_HWR_HARDWARE_OK			(MT_UINT32_C(0x1) << 0U)	/*!< The HW state is ok or locked up */
#define MUSAFWIF_HWR_RESET_IN_PROGRESS	(MT_UINT32_C(0x1) << 1U)	/*!< Tells if a HWR reset is in progress */
#define MUSAFWIF_HWR_GENERAL_LOCKUP		(MT_UINT32_C(0x1) << 3U)	/*!< A DM unrelated lockup has been detected */
#define MUSAFWIF_HWR_DM_RUNNING_OK		(MT_UINT32_C(0x1) << 4U)	/*!< At least one DM is running without being close to a lockup */
#define MUSAFWIF_HWR_DM_STALLING			(MT_UINT32_C(0x1) << 5U)	/*!< At least one DM is close to lockup */
#define MUSAFWIF_HWR_FW_FAULT			(MT_UINT32_C(0x1) << 6U)	/*!< The FW has faulted and needs to restart */
#define MUSAFWIF_HWR_RESTART_REQUESTED	(0x1U << 7U)	/*!< The FW has requested the host to restart it */

#define MUSAFWIF_PHR_STATE_SHIFT			(8U)
#define MUSAFWIF_PHR_RESTART_REQUESTED	(MT_UINT32_C(1) << MUSAFWIF_PHR_STATE_SHIFT)	/*!< The FW has requested the host to restart it, per PHR configuration */
#define MUSAFWIF_PHR_RESTART_FINISHED	(MT_UINT32_C(2) << MUSAFWIF_PHR_STATE_SHIFT)	/*!< A PHR triggered GPU reset has just finished */
#define MUSAFWIF_PHR_RESTART_MASK		(MUSAFWIF_PHR_RESTART_REQUESTED | MUSAFWIF_PHR_RESTART_FINISHED)

#define MUSAFWIF_PHR_MODE_OFF			(0UL)
#define MUSAFWIF_PHR_MODE_RD_RESET		(1UL)
#define MUSAFWIF_PHR_MODE_FULL_RESET		(2UL)

typedef MT_UINT32 MUSAFWIF_HWR_STATEFLAGS;

/* Firmware per-DM HWR states */
#define MUSAFWIF_DM_STATE_WORKING					(0x00U)		/*!< DM is working if all flags are cleared */
#define MUSAFWIF_DM_STATE_READY_FOR_HWR				(MT_UINT32_C(0x1) << 0)	/*!< DM is idle and ready for HWR */
#define MUSAFWIF_DM_STATE_NEEDS_SKIP					(MT_UINT32_C(0x1) << 2)	/*!< DM need to skip to next cmd before resuming processing */
#define MUSAFWIF_DM_STATE_NEEDS_PR_CLEANUP			(MT_UINT32_C(0x1) << 3)	/*!< DM need partial render cleanup before resuming processing */
#define MUSAFWIF_DM_STATE_NEEDS_TRACE_CLEAR			(MT_UINT32_C(0x1) << 4)	/*!< DM need to increment Recovery Count once fully recovered */
#define MUSAFWIF_DM_STATE_GUILTY_LOCKUP				(MT_UINT32_C(0x1) << 5)	/*!< DM was identified as locking up and causing HWR */
#define MUSAFWIF_DM_STATE_INNOCENT_LOCKUP			(MT_UINT32_C(0x1) << 6)	/*!< DM was innocently affected by another lockup which caused HWR */
#define MUSAFWIF_DM_STATE_GUILTY_OVERRUNING			(MT_UINT32_C(0x1) << 7)	/*!< DM was identified as over-running and causing HWR */
#define MUSAFWIF_DM_STATE_INNOCENT_OVERRUNING		(MT_UINT32_C(0x1) << 8)	/*!< DM was innocently affected by another DM over-running which caused HWR */
#define MUSAFWIF_DM_STATE_HARD_CONTEXT_SWITCH		(MT_UINT32_C(0x1) << 9)	/*!< DM was forced into HWR as it delayed more important workloads */
#define MUSAFWIF_DM_STATE_GPU_ECC_HWR				(MT_UINT32_C(0x1) << 10)	/*!< DM was forced into HWR due to an uncorrected GPU ECC error */

/* Firmware's connection state */
typedef enum
{
	MUSAFW_CONNECTION_FW_OFFLINE = 0,	/*!< Firmware is offline */
	MUSAFW_CONNECTION_FW_READY,			/*!< Firmware is initialised */
	MUSAFW_CONNECTION_FW_ACTIVE,			/*!< Firmware connection is fully established */
	MUSAFW_CONNECTION_FW_OFFLOADING,		/*!< Firmware is clearing up connection data */
	MUSAFW_CONNECTION_FW_STATE_COUNT
} MUSAFWIF_CONNECTION_FW_STATE;

/* OS' connection state */
typedef enum
{
	MUSAFW_CONNECTION_OS_OFFLINE = 0,	/*!< OS is offline */
	MUSAFW_CONNECTION_OS_READY,			/*!< OS's KM driver is setup and waiting */
	MUSAFW_CONNECTION_OS_ACTIVE,			/*!< OS connection is fully established */
	MUSAFW_CONNECTION_OS_STATE_COUNT
} MUSAFWIF_CONNECTION_OS_STATE;

typedef struct
{
	MT_UINT			bfOsState		: 3;
	MT_UINT			bfFLOk			: 1;
	MT_UINT			bfFLGrowPending	: 1;
	MT_UINT			bfIsolatedOS	: 1;
	MT_UINT			bfReserved		: 26;
} MUSAFWIF_OS_RUNTIME_FLAGS;

typedef MT_UINT32 MUSAFWIF_HWR_RECOVERYFLAGS;

#if defined(MTGPU_STALLED_CCB_ACTION)
#define MT_SLR_LOG_ENTRIES 10
#define MT_SLR_LOG_STRLEN  30 /*!< MAX_CLIENT_CCB_NAME not visible to this header */

typedef struct
{
	MT_UINT64 MUSAFW_ALIGN	ui64Timestamp;
	MT_UINT32				ui32FWCtxAddr;
	MT_UINT32				ui32NumUFOs;
	MT_CHAR				aszCCBName[MT_SLR_LOG_STRLEN];
} UNCACHED_ALIGN MUSAFWIF_SLR_ENTRY;
#endif

/*!
 * @InGroup SRVAndFWTracing
 * @Brief Firmware trace control data
 */
typedef struct
{
	MT_UINT32              ui32LogType;                  /*!< FW trace log group configuration */
	MUSAFWIF_TRACEBUF_SPACE  sTraceBuf[MUSAFW_THREAD_NUM];  /*!< FW Trace buffer */
	MT_UINT32              ui32TraceBufSizeInDWords;     /*!< FW Trace buffer size in dwords, Member initialised only when sTraceBuf is actually allocated
															(in MUSATraceBufferInitOnDemandResources) */
	MT_UINT32              ui32TracebufFlags;            /*!< Compatibility and other flags */
} UNCACHED_ALIGN MUSAFWIF_TRACEBUF;

/* Firmware trace time-stamp field breakup */

/* MUSA_CR_TIMER register read (48 bits) value*/
#define MUSAFWT_TIMESTAMP_TIME_SHIFT                   (0U)
#define MUSAFWT_TIMESTAMP_TIME_CLRMSK                  (MT_UINT64_C(0xFFFF000000000000))

/* Extra debug-info (16 bits) */
#define MUSAFWT_TIMESTAMP_DEBUG_INFO_SHIFT             (48U)
#define MUSAFWT_TIMESTAMP_DEBUG_INFO_CLRMSK            ~MUSAFWT_TIMESTAMP_TIME_CLRMSK


/* Debug-info sub-fields */
/* Bit 0: MUSA_CR_EVENT_STATUS_MMU_PAGE_FAULT bit from MUSA_CR_EVENT_STATUS register */
#define MUSAFWT_DEBUG_INFO_MMU_PAGE_FAULT_SHIFT        (0U)
#define MUSAFWT_DEBUG_INFO_MMU_PAGE_FAULT_SET          (1U << MUSAFWT_DEBUG_INFO_MMU_PAGE_FAULT_SHIFT)

/* Bit 1: MUSA_CR_BIF_MMU_ENTRY_PENDING bit from MUSA_CR_BIF_MMU_ENTRY register */
#define MUSAFWT_DEBUG_INFO_MMU_ENTRY_PENDING_SHIFT     (1U)
#define MUSAFWT_DEBUG_INFO_MMU_ENTRY_PENDING_SET       (1U << MUSAFWT_DEBUG_INFO_MMU_ENTRY_PENDING_SHIFT)

/* Bit 2: MUSA_CR_SLAVE_EVENT register is non-zero */
#define MUSAFWT_DEBUG_INFO_SLAVE_EVENTS_SHIFT          (2U)
#define MUSAFWT_DEBUG_INFO_SLAVE_EVENTS_SET            (1U << MUSAFWT_DEBUG_INFO_SLAVE_EVENTS_SHIFT)

/* Bit 3-15: Unused bits */

#define MUSAFWT_DEBUG_INFO_STR_MAXLEN                  64
#define MUSAFWT_DEBUG_INFO_STR_PREPEND                 " (debug info: "
#define MUSAFWT_DEBUG_INFO_STR_APPEND                  ")"

/* Table of debug info sub-field's masks and corresponding message strings
 * to be appended to firmware trace
 *
 * Mask     : 16 bit mask to be applied to debug-info field
 * String   : debug info message string
 */

#define MUSAFWT_DEBUG_INFO_MSKSTRLIST \
/*Mask,                                           String*/ \
X(MUSAFWT_DEBUG_INFO_MMU_PAGE_FAULT_SET,      "mmu pf") \
X(MUSAFWT_DEBUG_INFO_MMU_ENTRY_PENDING_SET,   "mmu pending") \
X(MUSAFWT_DEBUG_INFO_SLAVE_EVENTS_SET,        "slave events")

/*!
 ******************************************************************************
 * HWR Data
 *****************************************************************************/
/*!
 * @Defgroup HWRInfo FW HWR shared data interface
 * @Brief Types grouping data structures and defines used in realising the HWR record.
 * @{
 */
/*! @Brief HWR Lockup types */
typedef enum
{
	MUSA_HWRTYPE_UNKNOWNFAILURE = 0, /*!< Unknown failure */
	MUSA_HWRTYPE_OVERRUN        = 1, /*!< DM overrun */
	MUSA_HWRTYPE_POLLFAILURE    = 2, /*!< Poll failure */
	MUSA_HWRTYPE_BIF0FAULT      = 3, /*!< BIF0 fault */
	MUSA_HWRTYPE_BIF1FAULT      = 4, /*!< BIF1 fault */
	MUSA_HWRTYPE_GFUBIF0FAULT = 5, /*!< GFUBIF0 fault */
	MUSA_HWRTYPE_MMUFAULT       = 6, /*!< MMU fault */
	MUSA_HWRTYPE_MMUMCUFAULT   = 7, /*!< MMU MCU fault */
	MUSA_HWRTYPE_MIPSTLBFAULT   = 8, /*!< MIPS TLB fault */
	MUSA_HWRTYPE_ECCFAULT       = 9, /*!< ECC fault */
	MUSA_HWRTYPE_MMURISCVFAULT  = 10, /*!< MMU RISCV fault */
	MUSA_HWRTYPE_CEFAULT        = 11, /*!< CE fault */
} MUSA_HWRTYPE;

#define MUSAFWIF_HWRTYPE_BIF_BANK_GET(eHWRType) (((eHWRType) == MUSA_HWRTYPE_BIF0FAULT) ? 0 : 1)

#define MUSAFWIF_HWRTYPE_PAGE_FAULT_GET(eHWRType) ((((eHWRType) == MUSA_HWRTYPE_BIF0FAULT)      ||       \
                                                   ((eHWRType) == MUSA_HWRTYPE_BIF1FAULT)      ||       \
                                                   ((eHWRType) == MUSA_HWRTYPE_GFUBIF0FAULT) ||       \
                                                   ((eHWRType) == MUSA_HWRTYPE_MMUFAULT)       ||       \
                                                   ((eHWRType) == MUSA_HWRTYPE_MMUMCUFAULT)   ||       \
                                                   ((eHWRType) == MUSA_HWRTYPE_MIPSTLBFAULT)   ||       \
                                                   ((eHWRType) == MUSA_HWRTYPE_MMURISCVFAULT)) ? true : false)

/************************
 *  GPU HW error codes  *
 ************************/
typedef enum
{
	MUSA_HW_ERR_NA = 0x0,
	MUSA_HW_ERR_PRIMID_FAILURE_DURING_DMKILL = 0x101,
} MUSA_HW_ERR;


#define MUSAFWIF_CTXSWITCH_PROFILE_FAST_EN             (MT_UINT32_C(0x1))
#define MUSAFWIF_CTXSWITCH_PROFILE_MEDIUM_EN           (MT_UINT32_C(0x2))
#define MUSAFWIF_CTXSWITCH_PROFILE_SLOW_EN             (MT_UINT32_C(0x3))
#define MUSAFWIF_CTXSWITCH_PROFILE_NODELAY_EN  (MT_UINT32_C(0x4))

#define MUSAFWIF_CMP_ARBITRATION_TASK_DEMAND_EN        (MT_UINT32_C(0x1))
#define MUSAFWIF_CMP_ARBITRATION_ROUND_ROBIN_EN        (MT_UINT32_C(0x2))

#define MUSAFWIF_ISP_SCHEDMODE_VER1_IPP                        (MT_UINT32_C(0x1))
#define MUSAFWIF_ISP_SCHEDMODE_VER2_ISP                        (MT_UINT32_C(0x2))

/*!
 ******************************************************************************
 * MUSA firmware Init Config Data
 * NOTE: Please be careful to keep backwards compatibility with DDKv1 for the
 * CTXSWITCH controls.
 *****************************************************************************/

/* Flag definitions affecting the firmware globally */
#define MUSAFWIF_INICFG_CTXSWITCH_MODE_RAND					(MT_UINT32_C(0x1) << 0)	/*!< Randomise context switch requests */
#define MUSAFWIF_INICFG_CTXSWITCH_SRESET_EN					(MT_UINT32_C(0x1) << 1)
#define MUSAFWIF_INICFG_HWPERF_EN							(MT_UINT32_C(0x1) << 2)
#define MUSAFWIF_INICFG_DM_KILL_MODE_RAND_EN					(MT_UINT32_C(0x1) << 3)	/*!< Randomise DM-killing requests */
#define MUSAFWIF_INICFG_POW_RASCALDUST						(MT_UINT32_C(0x1) << 4)
#define MUSAFWIF_INICFG_DISABLE_CE							(MT_UINT32_C(0x1) << 5)
#define MUSAFWIF_INICFG_FBCDC_V3_1_EN						(MT_UINT32_C(0x1) << 6)
#define MUSAFWIF_INICFG_CHECK_MLIST_EN						(MT_UINT32_C(0x1) << 7)
#define MUSAFWIF_INICFG_DISABLE_CLKGATING_EN					(MT_UINT32_C(0x1) << 8)
/* 9 unused */
/* 10 unused */
/* 11 unused */
#define MUSAFWIF_INICFG_REGCONFIG_EN							(MT_UINT32_C(0x1) << 12)
#define MUSAFWIF_INICFG_ASSERT_ON_OUTOFMEMORY				(MT_UINT32_C(0x1) << 13)
#define MUSAFWIF_INICFG_HWP_DISABLE_FILTER					(MT_UINT32_C(0x1) << 14)
#define MUSAFWIF_INICFG_ENABLE_CALC_DM_UTIL  				(MT_UINT32_C(0x1) << 15)
#define MUSAFWIF_INICFG_CTXSWITCH_PROFILE_SHIFT				(16)
#define MUSAFWIF_INICFG_CTXSWITCH_PROFILE_FAST				(MUSAFWIF_CTXSWITCH_PROFILE_FAST_EN << MUSAFWIF_INICFG_CTXSWITCH_PROFILE_SHIFT)
#define MUSAFWIF_INICFG_CTXSWITCH_PROFILE_MEDIUM				(MUSAFWIF_CTXSWITCH_PROFILE_MEDIUM_EN << MUSAFWIF_INICFG_CTXSWITCH_PROFILE_SHIFT)
#define MUSAFWIF_INICFG_CTXSWITCH_PROFILE_SLOW				(MUSAFWIF_CTXSWITCH_PROFILE_SLOW_EN << MUSAFWIF_INICFG_CTXSWITCH_PROFILE_SHIFT)
#define MUSAFWIF_INICFG_CTXSWITCH_PROFILE_NODELAY			(MUSAFWIF_CTXSWITCH_PROFILE_NODELAY_EN << MUSAFWIF_INICFG_CTXSWITCH_PROFILE_SHIFT)
#define MUSAFWIF_INICFG_CTXSWITCH_PROFILE_MASK				(MT_UINT32_C(0x7) << MUSAFWIF_INICFG_CTXSWITCH_PROFILE_SHIFT)
#define MUSAFWIF_INICFG_DISABLE_DM_OVERLAP					(MT_UINT32_C(0x1) << 19)
#define MUSAFWIF_INICFG_ASSERT_ON_HWR_TRIGGER				(MT_UINT32_C(0x1) << 20)
#define MUSAFWIF_INICFG_FABRIC_COHERENCY_ENABLED				(MT_UINT32_C(0x1) << 21)
#define MUSAFWIF_INICFG_PARTITION_COHERENCY_ENABLED			MUSAFWIF_INICFG_FABRIC_COHERENCY_ENABLED
#define MUSAFWIF_INICFG_VALIDATE_IRQ							(MT_UINT32_C(0x1) << 22)
#define MUSAFWIF_INICFG_DISABLE_PDP_EN						(MT_UINT32_C(0x1) << 23)
#define MUSAFWIF_INICFG_MPX_POWER_STATE_MASK_CHANGE_EN		(MT_UINT32_C(0x1) << 24)
#define MUSAFWIF_INICFG_WORKEST								(MT_UINT32_C(0x1) << 25)
#define MUSAFWIF_INICFG_PDVFS								(MT_UINT32_C(0x1) << 26)
#define MUSAFWIF_INICFG_CMP_ARBITRATION_SHIFT				(27)
#define MUSAFWIF_INICFG_CMP_ARBITRATION_TASK_DEMAND			(MUSAFWIF_CMP_ARBITRATION_TASK_DEMAND_EN << MUSAFWIF_INICFG_CMP_ARBITRATION_SHIFT)
#define MUSAFWIF_INICFG_CMP_ARBITRATION_ROUND_ROBIN			(MUSAFWIF_CMP_ARBITRATION_ROUND_ROBIN_EN << MUSAFWIF_INICFG_CMP_ARBITRATION_SHIFT)
#define MUSAFWIF_INICFG_CMP_ARBITRATION_MASK					(MT_UINT32_C(0x3) << MUSAFWIF_INICFG_CMP_ARBITRATION_SHIFT)
#define MUSAFWIF_INICFG_ISPSCHEDMODE_SHIFT					(29)
#define MUSAFWIF_INICFG_ISPSCHEDMODE_NONE					(0)
#define MUSAFWIF_INICFG_ISPSCHEDMODE_VER1_IPP				(MUSAFWIF_ISP_SCHEDMODE_VER1_IPP << MUSAFWIF_INICFG_ISPSCHEDMODE_SHIFT)
#define MUSAFWIF_INICFG_ISPSCHEDMODE_VER2_ISP				(MUSAFWIF_ISP_SCHEDMODE_VER2_ISP << MUSAFWIF_INICFG_ISPSCHEDMODE_SHIFT)
#define MUSAFWIF_INICFG_ISPSCHEDMODE_MASK					(MUSAFWIF_INICFG_ISPSCHEDMODE_VER1_IPP |\
                                                             MUSAFWIF_INICFG_ISPSCHEDMODE_VER2_ISP)
#define MUSAFWIF_INICFG_VALIDATE_SOCMP_TIMER				(MT_UINT32_C(0x1) << 31)
#define MUSAFWIF_INICFG_ALL									(0xFFFFF3FFU)

/* Extended Flag definitions affecting the firmware globally */
#define MUSAFWIF_INICFG_EXT_ALL								(0x0U)

#define MUSAFWIF_INICFG_SYS_CTXSWITCH_CLRMSK					~(MUSAFWIF_INICFG_CTXSWITCH_MODE_RAND | \
															  MUSAFWIF_INICFG_CTXSWITCH_SRESET_EN)

/* Flag definitions affecting only workloads submitted by a particular OS */

/*!
 * @AddToGroup ContextSwitching
 * @{
 * @Name Per-OS DM context switch configuration flags
 * @{
 */
#define MUSAFWIF_INICFG_OS_CTXSWITCH_TRANSFER_EN					(MT_UINT32_C(0x1) << 0) /*!< Enables TRANSFER context switch */
#define MUSAFWIF_INICFG_OS_CTXSWITCH_GEOM_EN					(MT_UINT32_C(0x1) << 1) /*!< Enables GEOM-TA and GEOM-SHG context switch */
#define MUSAFWIF_INICFG_OS_CTXSWITCH_3D_EN					(MT_UINT32_C(0x1) << 2) /*!< Enables FRAG DM context switch */
#define MUSAFWIF_INICFG_OS_CTXSWITCH_CMP_EN					(MT_UINT32_C(0x1) << 3) /*!< Enables CMP context switch */
#define MUSAFWIF_INICFG_OS_CTXSWITCH_CE_EN					(MT_UINT32_C(0x1) << 4) /*!< Enables CE context switch */

#define MUSAFWIF_INICFG_OS_LOW_PRIO_CS_TRANSFER					(MT_UINT32_C(0x1) << 5)
#define MUSAFWIF_INICFG_OS_LOW_PRIO_CS_GEOM					(MT_UINT32_C(0x1) << 6)
#define MUSAFWIF_INICFG_OS_LOW_PRIO_CS_3D					(MT_UINT32_C(0x1) << 7)
#define MUSAFWIF_INICFG_OS_LOW_PRIO_CS_CMP					(MT_UINT32_C(0x1) << 8)
#define MUSAFWIF_INICFG_OS_LOW_PRIO_CS_RDM					(MT_UINT32_C(0x1) << 9)

#define MUSAFWIF_INICFG_OS_ALL								(0x3FFU)

#define MUSAFWIF_INICFG_OS_CTXSWITCH_DM_ALL					(MUSAFWIF_INICFG_OS_CTXSWITCH_GEOM_EN | \
															 MUSAFWIF_INICFG_OS_CTXSWITCH_3D_EN | \
															 MUSAFWIF_INICFG_OS_CTXSWITCH_CMP_EN | \
															 MUSAFWIF_INICFG_OS_CTXSWITCH_TRANSFER_EN | \
															 MUSAFWIF_INICFG_OS_CTXSWITCH_CE_EN)

#define MUSAFWIF_INICFG_OS_CTXSWITCH_DM_NO3D					(MUSAFWIF_INICFG_OS_CTXSWITCH_GEOM_EN | \
															 MUSAFWIF_INICFG_OS_CTXSWITCH_CMP_EN | \
															 MUSAFWIF_INICFG_OS_CTXSWITCH_TRANSFER_EN | \
															 MUSAFWIF_INICFG_OS_CTXSWITCH_CE_EN)

#define MUSAFWIF_INICFG_OS_CTXSWITCH_CLRMSK					~(MUSAFWIF_INICFG_OS_CTXSWITCH_DM_ALL)

/*!
 * @} End of Per-OS Context switch configuration flags
 * @} End of AddToGroup ContextSwitching
 */

#define MUSAFWIF_FILTCFG_TRUNCATE_HALF						(MT_UINT32_C(0x1) << 3)
#define MUSAFWIF_FILTCFG_TRUNCATE_INT						(MT_UINT32_C(0x1) << 2)
#define MUSAFWIF_FILTCFG_NEW_FILTER_MODE						(MT_UINT32_C(0x1) << 1)

typedef enum
{
	MUSA_ACTIVEPM_FORCE_OFF = 0,
	MUSA_ACTIVEPM_FORCE_ON = 1,
	MUSA_ACTIVEPM_DEFAULT = 2
} MUSA_ACTIVEPM_CONF;

typedef enum
{
	MUSA_RD_POWER_ISLAND_FORCE_OFF = 0,
	MUSA_RD_POWER_ISLAND_FORCE_ON = 1,
	MUSA_RD_POWER_ISLAND_DEFAULT = 2
} MUSA_RD_POWER_ISLAND_CONF;

typedef struct
{
	MT_UINT16 ui16RegNum;				/*!< Register number */
	MT_UINT16 ui16IndirectRegNum;		/*!< Indirect register number (or 0 if not used) */
	MT_UINT16 ui16IndirectStartVal;	/*!< Start value for indirect register */
	MT_UINT16 ui16IndirectEndVal;		/*!< End value for indirect register */
} MUSAFW_REGISTER_LIST;

#if defined(MUSA_FIRMWARE)
typedef DLLIST_NODE							MUSAFWIF_DLLIST_NODE;
#else
typedef struct {MUSAFWIF_DEV_VIRTADDR p;
                MUSAFWIF_DEV_VIRTADDR n;}	MUSAFWIF_DLLIST_NODE;
#endif

typedef MUSAFWIF_DEV_VIRTADDR  PMUSAFWIF_SIGBUFFER;
typedef MUSAFWIF_DEV_VIRTADDR  PMUSAFWIF_TRACEBUF;
typedef MUSAFWIF_DEV_VIRTADDR  PMUSAFWIF_SYSDATA;
typedef MUSAFWIF_DEV_VIRTADDR  PMUSAFWIF_OSDATA;
#if defined(SUPPORT_TBI_INTERFACE)
typedef MUSAFWIF_DEV_VIRTADDR  PMUSAFWIF_TBIBUF;
#endif
typedef MUSAFWIF_DEV_VIRTADDR  PMUSAFWIF_HWPERFBUF;
typedef MUSAFWIF_DEV_VIRTADDR  PMUSAFWIF_HWRINFOBUF;
typedef MUSAFWIF_DEV_VIRTADDR  PMUSAFWIF_RUNTIME_CFG;
typedef MUSAFWIF_DEV_VIRTADDR  PMUSAFWIF_GPU_UTIL_FWCB;
typedef MUSAFWIF_DEV_VIRTADDR  PMUSAFWIF_REG_CFG;
typedef MUSAFWIF_DEV_VIRTADDR  PMUSAFWIF_HWPERF_CTL;
typedef MUSAFWIF_DEV_VIRTADDR  PMUSA_HWPERF_CONFIG_CNTBLK;
typedef MUSAFWIF_DEV_VIRTADDR  PMUSAFWIF_CCB_CTL;
typedef MUSAFWIF_DEV_VIRTADDR  PMUSAFWIF_CCB;
typedef MUSAFWIF_DEV_VIRTADDR  PMUSAFWIF_CCB_RTN_SLOTS;
typedef MUSAFWIF_DEV_VIRTADDR  PMUSAFWIF_FWMEMCONTEXT;
typedef MUSAFWIF_DEV_VIRTADDR  PMUSAFWIF_FWCOMMONCONTEXT;
typedef MUSAFWIF_DEV_VIRTADDR  PMUSAFWIF_ZSBUFFER;
typedef MUSAFWIF_DEV_VIRTADDR  PMUSAFWIF_COMMONCTX_STATE;
typedef MUSAFWIF_DEV_VIRTADDR  PMUSAFWIF_CORE_CLK_RATE;
typedef MUSAFWIF_DEV_VIRTADDR  PMUSAFWIF_FIRMWAREGCOVBUFFER;
typedef MUSAFWIF_DEV_VIRTADDR  PMUSAFWIF_CCCB;
typedef MUSAFWIF_DEV_VIRTADDR  PMUSAFWIF_CCCB_CTL;
typedef MUSAFWIF_DEV_VIRTADDR  PMUSAFWIF_FREELIST;
typedef MUSAFWIF_DEV_VIRTADDR  PMUSAFWIF_HWRTDATA;
typedef MUSAFWIF_DEV_VIRTADDR  PMUSAFWIF_TIMESTAMP_ADDR;
typedef MUSAFWIF_DEV_VIRTADDR  PMUSAFWIF_RF_CMD;

/*!
 * This number is used to represent an invalid page catalogue physical address
 */
#define MUSAFWIF_INVALID_PC_PHYADDR 0xFFFFFFFFFFFFFFFFLLU

/*!
 * This number is used to represent an unallocated set of page catalog base registers
 */
#define MUSAFW_BIF_INVALID_PCSET 0xFFFFFFFFU


/*!
 * FW context state flags
 */
#define MUSAFWIF_CONTEXT_FLAGS_NEED_RESUME			(0x00000001U)
#define MUSAFWIF_CONTEXT_FLAGS_TRANSFER_HEADER_STALE		(0x00000002U)
#define MUSAFWIF_CONTEXT_FLAGS_LAST_KICK_SECURE		(0x00000200U)

/*!
 * @InGroup ContextSwitching
 * @Brief Firmware GEOM/TA context suspend state
 */
typedef struct
{
	MT_UINT64	MUSAFW_ALIGN uTAReg_DCE_CMD0;
	MT_UINT32				uTAReg_DCE_CMD1;
	MT_UINT32				uTAReg_DCE_WRITE;
	MT_UINT64	MUSAFW_ALIGN uTAReg_DCE_DRAW0;
	MT_UINT64	MUSAFW_ALIGN uTAReg_DCE_DRAW1;
	MT_UINT32				uTAReg_GTA_SO_PRIM[4];
	MT_UINT16	ui16TACurrentIdx;
} UNCACHED_ALIGN MUSAFWIF_TACTX_STATE_PER_GEOM;

typedef struct
{
	/* FW-accessible TA state which must be written out to memory on context store */
	MUSAFWIF_TACTX_STATE_PER_GEOM asGeomCore[MUSA_NUM_GEOM_CORES];
} UNCACHED_ALIGN MUSAFWIF_TACTX_STATE;

/* The following defines need to be auto generated using the HW defines
 * rather than hard coding it */
#define MUSAFWIF_ISP_PIPE_COUNT_MAX		(20)
#define MUSAFWIF_PIPE_COUNT_PER_ISP		(2)
#define MUSAFWIF_IPP_RESUME_REG_COUNT	(1)

#if !defined(__KERNEL__)
#define MUSAFWIF_ISP_COUNT		(MUSA_FEATURE_NUM_MPX * MUSA_FEATURE_NUM_ISP_PER_MPX)
#define MUSAFWIF_ISP_PIPE_COUNT	(MUSAFWIF_ISP_COUNT * MUSAFWIF_PIPE_COUNT_PER_ISP)
#if MUSAFWIF_ISP_PIPE_COUNT > MUSAFWIF_ISP_PIPE_COUNT_MAX
#error MUSAFWIF_ISP_PIPE_COUNT should not be greater than MUSAFWIF_ISP_PIPE_COUNT_MAX
#endif
#endif /* !defined(__KERNEL__) */

/*!
 * @InGroup ContextSwitching
 * @Brief Firmware FRAG/3D context suspend state
 */
typedef struct
{
#if defined(PM_INTERACTIVE_MODE)
	MT_UINT32	MUSAFW_ALIGN u3DReg_PM_DEALLOCATED_MASK_STATUS;	/*!< Managed by PM HW in the non-interactive mode */
#endif
	MT_UINT32	ui32CtxStateFlags;	/*!< Compatibility and other flags */

	/* FW-accessible ISP state which must be written out to memory on context store */
	/* au3DReg_ISP_STORE should be the last element of the structure
	 * as this is an array whose size is determined at runtime
	 * after detecting the MUSA core */
	MT_UINT64	MUSAFW_ALIGN au3DReg_ISP_STORE[]; /*!< ISP state (per-pipe) */
} UNCACHED_ALIGN MUSAFWIF_3DCTX_STATE;

#define MUSAFWIF_CTX_USING_BUFFER_A		(0)
#define MUSAFWIF_CTX_USING_BUFFER_B		(1U)

typedef struct
{
	MT_UINT32  ui32CtxStateFlags;		/*!< Target buffer and other flags */
} MUSAFWIF_COMPCTX_STATE;

typedef MT_UINT64 MUSAFWIF_TRP_CHECKSUM_2D[MUSA_TRP_MAX_NUM_CORES][2];
typedef MT_UINT64 MUSAFWIF_TRP_CHECKSUM_3D[MUSA_TRP_MAX_NUM_CORES][4];
typedef MT_UINT64 MUSAFWIF_TRP_CHECKSUM_GEOM[MUSA_TRP_MAX_NUM_CORES][2];

/*!
 ******************************************************************************
 * Defines for CMD_TYPE corruption detection and forward compatibility check
 *****************************************************************************/

/* CMD_TYPE 32bit contains:
 * 31:16	Reserved for magic value to detect corruption (16 bits)
 * 15		Reserved for MUSA_CCB_TYPE_TASK (1 bit)
 * 14:0		Bits available for CMD_TYPEs (15 bits) */


/* Magic value to detect corruption */
#define MUSA_CMD_MAGIC_DWORD			MT_UINT32_C(0x2ABC)
#define MUSA_CMD_MAGIC_DWORD_MASK	(0xFFFF0000U)
#define MUSA_CMD_MAGIC_DWORD_SHIFT	(16U)
#define MUSA_CMD_MAGIC_DWORD_SHIFTED	(MUSA_CMD_MAGIC_DWORD << MUSA_CMD_MAGIC_DWORD_SHIFT)

#define MUSAFWIF_MMUCACHEDATA_FLAGS_PT      (0x1U) /* MMU_CTRL_INVAL_PT_EN */
#define MUSAFWIF_MMUCACHEDATA_FLAGS_PD      (0x2U) /* MMU_CTRL_INVAL_PD_EN */
#define MUSAFWIF_MMUCACHEDATA_FLAGS_PC      (0x4U) /* MMU_CTRL_INVAL_PC_EN */
#define MUSAFWIF_MMUCACHEDATA_FLAGS_CTX_ALL (0x800U) /* MMU_CTRL_INVAL_ALL_CONTEXTS_EN */
#define MUSAFWIF_MMUCACHEDATA_FLAGS_TLB     (0x0) /* not used */


typedef enum
{
	MUSAFWIF_HWPERF_CTRL_TOGGLE = 0,
	MUSAFWIF_HWPERF_CTRL_SET    = 1,
	MUSAFWIF_HWPERF_CTRL_EMIT_FEATURES_EV = 2
} MUSAFWIF_HWPERF_UPDATE_CONFIG;

/*!
 * @Brief Command data for \ref MUSAFWIF_KCCB_CMD_HWPERF_UPDATE_CONFIG type command
 */
typedef struct
{
	MUSAFWIF_HWPERF_UPDATE_CONFIG eOpCode; /*!< Control operation code */
	MT_UINT64	MUSAFW_ALIGN	ui64Mask;   /*!< Mask of events to toggle */
} MUSAFWIF_HWPERF_CTRL;

typedef enum
{
	MUSAFWIF_HWPERF_CNTR_NOOP    = 0,   /* No-Op */
	MUSAFWIF_HWPERF_CNTR_ENABLE  = 1,   /* Enable Counters */
	MUSAFWIF_HWPERF_CNTR_DISABLE = 2    /* Disable Counters */
} MUSAFWIF_HWPERF_CNTR_CONFIG;

/*!
 * @Brief Command data for \ref MUSAFWIF_KCCB_CMD_HWPERF_CONFIG_BLKS type command
 */
typedef struct
{
	MT_UINT32                ui32CtrlWord;
	MT_UINT32                ui32NumBlocks;    /*!< Number of MUSA_HWPERF_CONFIG_CNTBLK in the array */
	PMUSA_HWPERF_CONFIG_CNTBLK sBlockConfigs;    /*!< Address of the MUSA_HWPERF_CONFIG_CNTBLK array */
} MUSAFWIF_HWPERF_CONFIG_ENABLE_BLKS;


#define MUSAFWIF_HWPERF_CTRL_BLKS_MAX	16

/*!
 * @Brief Command data for \ref MUSAFWIF_KCCB_CMD_HWPERF_CTRL_BLKS type command
 */
typedef struct
{
	bool		bEnable;
	MT_UINT32	ui32NumBlocks;                              /*!< Number of block IDs in the array */
	MT_UINT16	aeBlockIDs[MUSAFWIF_HWPERF_CTRL_BLKS_MAX];   /*!< Array of MUSA_HWPERF_CNTBLK_ID values */
} MUSAFWIF_HWPERF_CTRL_BLKS;


#if defined(MUSA_NUM_OS_SUPPORTED) && (MUSA_NUM_OS_SUPPORTED > 1)
#define MUSAFW_MAX_VGPU_DM_NUM (4) /* TRANSFER/TA/3D/CMP */

struct VGPU_SCHEDULE_INFO_LINUX
{
    MT_UINT32 cmdExecutedTime;   /* total hw execution time */
    MT_UINT32 maxCmdTime;        /* completed or preempted cmd */
    MT_UINT16 completeCmdCnt;    /* count of completed cmd */
};
#endif

/*!
 *****************************************************************************
 * Control data for MUSA
 *****************************************************************************/

typedef enum
{
	MUSAFWIF_USRM_DM_GTCV = 0,
	MUSAFWIF_USRM_DM_DDM = 1,
	MUSAFWIF_USRM_DM_CMP = 2,
	MUSAFWIF_USRM_DM_PDM = 3,
	MUSAFWIF_USRM_DM_TRANSFER = 4,
	MUSAFWIF_USRM_DM_LAST
} MUSAFWIF_USRM_DM;

typedef enum
{
	MUSAFWIF_UVBRM_DM_GTCV = 0,
	MUSAFWIF_UVBRM_DM_DDM = 1,
	MUSAFWIF_UVBRM_DM_LAST
} MUSAFWIF_UVBRM_DM;

typedef enum
{
	MUSAFWIF_TPU_DM_PDM = 0,
	MUSAFWIF_TPU_DM_GTCV = 1,
	MUSAFWIF_TPU_DM_CMP = 2,
	MUSAFWIF_TPU_DM_TRANSFER = 3,
	MUSAFWIF_TPU_DM_LAST
} MUSAFWIF_TPU_DM;

typedef enum
{
	MUSAFWIF_GPIO_VAL_OFF           = 0, /*!< No GPIO validation */
	MUSAFWIF_GPIO_VAL_GENERAL       = 1, /*!< Simple test case that
	                                         initiates by sending data via the
	                                         GPIO and then sends back any data
	                                         received over the GPIO */
	MUSAFWIF_GPIO_VAL_AP            = 2, /*!< More complex test case that writes
	                                         and reads data across the entire
	                                         GPIO AP address range.*/
#if defined(SUPPORT_STRIP_RENDERING)
	MUSAFWIF_GPIO_VAL_SR_BASIC      = 3, /*!< Strip Rendering AP based basic test.*/
	MUSAFWIF_GPIO_VAL_SR_COMPLEX    = 4, /*!< Strip Rendering AP based complex test.*/
#endif
	MUSAFWIF_GPIO_VAL_TESTBENCH     = 5, /*!< Validates the GPIO Testbench. */
	MUSAFWIF_GPIO_VAL_LOOPBACK      = 6, /*!< Send and then receive each byte
	                                         in the range 0-255. */
	MUSAFWIF_GPIO_VAL_LOOPBACK_LITE = 7, /*!< Send and then receive each power-of-2
	                                         byte in the range 0-255. */
	MUSAFWIF_GPIO_VAL_LAST
} MUSAFWIF_GPIO_VAL_MODE;

typedef enum
{
	FW_PERF_CONF_NONE = 0,
	FW_PERF_CONF_ICACHE = 1,
	FW_PERF_CONF_DCACHE = 2,
	FW_PERF_CONF_JTLB_INSTR = 5,
	FW_PERF_CONF_INSTRUCTIONS = 6
} FW_PERF_CONF;

typedef enum
{
	FW_BOOT_STAGE_TLB_INIT_FAILURE = -2,
	FW_BOOT_STAGE_NOT_AVAILABLE = -1,
	FW_BOOT_NOT_STARTED = 0,
	FW_BOOT_BLDR_STARTED = 1,
	FW_BOOT_CACHE_DONE,
	FW_BOOT_TLB_DONE,
	FW_BOOT_MAIN_STARTED,
	FW_BOOT_ALIGNCHECKS_DONE,
	FW_BOOT_INIT_DONE,
} FW_BOOT_STAGE;

#if defined(SUPPORT_GPUVIRT_VALIDATION)
#define MUSAFWIF_KICK_TEST_ENABLED_BIT  0x1
#define MUSAFWIF_KICK_TEST_OSID_SHIFT   0x1
#endif

/*!
 *****************************************************************************
 * Timer correlation shared data and defines
 *****************************************************************************/

typedef struct
{
	MT_UINT64 MUSAFW_ALIGN ui64OSTimeStamp;
	MT_UINT64 MUSAFW_ALIGN ui64OSMonoTimeStamp;
	MT_UINT64 MUSAFW_ALIGN ui64CRTimeStamp;

	/* Utility variable used to convert CR timer deltas to OS timer deltas (nS),
	 * where the deltas are relative to the timestamps above:
	 * deltaOS = (deltaCR * K) >> decimal_shift, see full explanation below */
	MT_UINT64 MUSAFW_ALIGN ui64CRDeltaToOSDeltaKNs;

	MT_UINT32             ui32CoreClockSpeed;
	MT_UINT32             ui32Reserved;
} UNCACHED_ALIGN MUSAFWIF_TIME_CORR;


/* The following macros are used to help converting FW timestamps to the Host
 * time domain. On the FW the MUSA_CR_TIMER counter is used to keep track of
 * time; it increments by 1 every 256 GPU clock ticks, so the general
 * formula to perform the conversion is:
 *
 * [ GPU clock speed in Hz, if (scale == 10^9) then deltaOS is in nS,
 *   otherwise if (scale == 10^6) then deltaOS is in uS ]
 *
 *             deltaCR * 256                                   256 * scale
 *  deltaOS = --------------- * scale = deltaCR * K    [ K = --------------- ]
 *             GPUclockspeed                                  GPUclockspeed
 *
 * The actual K is multiplied by 2^20 (and deltaCR * K is divided by 2^20)
 * to get some better accuracy and to avoid returning 0 in the integer
 * division 256000000/GPUfreq if GPUfreq is greater than 256MHz.
 * This is the same as keeping K as a decimal number.
 *
 * The maximum deltaOS is slightly more than 5hrs for all GPU frequencies
 * (deltaCR * K is more or less a constant), and it's relative to the base
 * OS timestamp sampled as a part of the timer correlation data.
 * This base is refreshed on GPU power-on, DVFS transition and periodic
 * frequency calibration (executed every few seconds if the FW is doing
 * some work), so as long as the GPU is doing something and one of these
 * events is triggered then deltaCR * K will not overflow and deltaOS will be
 * correct.
 */

#define MUSAFWIF_CRDELTA_TO_OSDELTA_ACCURACY_SHIFT  (20)

#define MUSAFWIF_GET_DELTA_OSTIME_NS(deltaCR, K) \
	(((deltaCR) * (K)) >> MUSAFWIF_CRDELTA_TO_OSDELTA_ACCURACY_SHIFT)


/*!
 ******************************************************************************
 * GPU Utilisation
 *****************************************************************************/

/* See musa_common.h for a list of GPU states */
#define MUSAFWIF_GPU_UTIL_TIME_MASK       (MT_UINT64_C(0xFFFFFFFFFFFFFFFF) & ~MUSAFWIF_GPU_UTIL_STATE_MASK)

#define MUSAFWIF_GPU_UTIL_GET_TIME(word)  ((word) & MUSAFWIF_GPU_UTIL_TIME_MASK)
#define MUSAFWIF_GPU_UTIL_GET_STATE(word) ((word) & MUSAFWIF_GPU_UTIL_STATE_MASK)

/* The OS timestamps computed by the FW are approximations of the real time,
 * which means they could be slightly behind or ahead the real timer on the Host.
 * In some cases we can perform subtractions between FW approximated
 * timestamps and real OS timestamps, so we need a form of protection against
 * negative results if for instance the FW one is a bit ahead of time.
 */
#define MUSAFWIF_GPU_UTIL_GET_PERIOD(newtime,oldtime) \
	(((MUSAFWIF_GPU_UTIL_GET_TIME(newtime)) > (MUSAFWIF_GPU_UTIL_GET_TIME(oldtime))) \
	? ((MUSAFWIF_GPU_UTIL_GET_TIME(newtime)) - (MUSAFWIF_GPU_UTIL_GET_TIME(oldtime))) : 0U)

#define MUSAFWIF_GPU_UTIL_MAKE_WORD(time,state) \
	(MUSAFWIF_GPU_UTIL_GET_TIME(time) | MUSAFWIF_GPU_UTIL_GET_STATE(state))


/* The timer correlation array must be big enough to ensure old entries won't be
 * overwritten before all the HWPerf events linked to those entries are processed
 * by the MISR. The update frequency of this array depends on how fast the system
 * can change state (basically how small the APM latency is) and perform DVFS transitions.
 *
 * The minimum size is 2 (not 1) to avoid race conditions between the FW reading
 * an entry while the Host is updating it. With 2 entries in the worst case the FW
 * will read old data, which is still quite ok if the Host is updating the timer
 * correlation at that time.
 */
#define MUSAFWIF_TIME_CORR_ARRAY_SIZE            256U
#define MUSAFWIF_TIME_CORR_CURR_INDEX(seqcount)  ((seqcount) % MUSAFWIF_TIME_CORR_ARRAY_SIZE)

/* Make sure the timer correlation array size is a power of 2 */
static_assert((MUSAFWIF_TIME_CORR_ARRAY_SIZE & (MUSAFWIF_TIME_CORR_ARRAY_SIZE - 1U)) == 0U,
			  "MUSAFWIF_TIME_CORR_ARRAY_SIZE must be a power of two");

#define MUSAFWIF_GPU_UTIL_ITEM_COUNT 5

typedef struct
{
	MUSAFWIF_DM eDM;
	MT_CHAR *const pszItemName;
} UNCACHED_ALIGN MUSAFWIF_GPU_UTIL_ITEM_INFO;

static const MUSAFWIF_GPU_UTIL_ITEM_INFO gasUtilItemInfo[MUSAFWIF_GPU_UTIL_ITEM_COUNT] =
{
	{ MUSAFWIF_GPU_OVERALL_UTIL, "GPU Overall Utilisation:     " },
	{ MUSAFWIF_DM_GEOM, "	GEOM Utilisation:    " },
	{ MUSAFWIF_DM_3D, "	3D Utilisation:      " },
	{ MUSAFWIF_DM_CMP, "	COMP Utilisation: " },
	{ MUSAFWIF_DM_TRANSFER, "	2D Utilisation:      " }
};

typedef struct
{
    /* Last GPU state + OS time of the last state update */
    MT_UINT64 MUSAFW_ALIGN ui64LastWord;

    /* Counters for the amount of time the GPU DM was active/idle/blocked */
    MT_UINT64 MUSAFW_ALIGN aui64StatsCounters[MUSAFWIF_GPU_UTIL_STATE_NUM];
} UNCACHED_ALIGN MUSAFWIF_GPU_UTIL_ITEM;

typedef struct
{
    MUSAFWIF_TIME_CORR           sTimeCorr[MUSAFWIF_TIME_CORR_ARRAY_SIZE];
    MT_UINT32            ui32TimeCorrSeqCount;

    /* Compatibility and other flags */
    MT_UINT32            ui32GpuUtilFlags;
    MUSAFWIF_GPU_UTIL_ITEM asGpuDmUtil[MUSAFWIF_GPU_UTIL_ITEM_COUNT];
} UNCACHED_ALIGN MUSAFWIF_GPU_UTIL_FWCB;

/* Sync_checkpoint firmware object.
 * This is the FW-addressable structure use to hold the sync checkpoint's
 * state and other information which needs to be accessed by the firmware.
 */
typedef struct
{
	MT_UINT32	ui32State;          /*!< Holds the current state of the sync checkpoint */
	MT_UINT32	ui32FwRefCount;     /*!< Holds the FW reference count (num of fences/updates processed) */
} SYNC_CHECKPOINT_FW_OBJ;

/* Bit mask Firmware can use to test if a checkpoint has signalled or errored */
#define SYNC_CHECKPOINT_SIGNALLED_MASK (0x1 << 0)

#endif /* MUSA_FWIF_KM_H */

/******************************************************************************
 End of file (musa_fwif_km.h)
******************************************************************************/

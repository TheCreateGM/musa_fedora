/*************************************************************************/ /*!
@File
@Title          MUSA HWPerf and Debug Types and Defines Header
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Common data types definitions for hardware performance API
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
#ifndef MUSA_HWPERF_H_
#define MUSA_HWPERF_H_

#if defined(__cplusplus)
extern "C" {
#endif

/* These structures are used on both GPU and CPU and must be a size that is a
 * multiple of 64 bits, 8 bytes to allow the FW to write 8 byte quantities at
 * 8 byte aligned addresses. MUSA_FW_STRUCT_*_ASSERT() is used to check this.
 */

/******************************************************************************
 * Includes and Defines
 *****************************************************************************/

#include "img_types.h"
#include "img_defs.h"

#include "musa_common.h"
#include "musa_hwperf_common.h"
#include "mtgpu_tlcommon.h"
#include "mtgpu_sync_km.h"


#if defined(MUSA_BVNC_CORE_KM_HEADER) && defined(MUSA_BNC_CONFIG_KM_HEADER)
/* HWPerf interface assumption checks */
static_assert(MUSA_FEATURE_NUM_CLUSTERS <= 16U,
			  "Cluster count too large for HWPerf protocol definition");
#endif

/*! Perf counter control words */
#define MUSA_HWPERF_CTRL_NOP						(0)			/*!< only update HW counters */
#define MUSA_HWPERF_CTRL_STATE_UPDATE_EN			(1U << 31)	/*!< persistent state update; see other flags below */
#define MUSA_HWPERF_CTRL_GEOM_FULLRANGE			(1U)		/*!< selectable geom and 3D counters are full range */
#define MUSA_HWPERF_CTRL_COMP_FULLRANGE			(2U)		/*!< selectable compute counters are full range */
#define MUSA_HWPERF_CTRL_TRANSFER_FULLRANGE			(4U)		/*!< selectable TRANSFER counters are full range */


/******************************************************************************
 * Data Stream Common Types
 *****************************************************************************/

/*! All the Data Masters HWPerf is aware of. When a new DM is added to this
 * list, it should be appended at the end to maintain backward compatibility
 * of HWPerf data.
 */
typedef enum {

	MUSA_HWPERF_DM_GP,
	MUSA_HWPERF_DM_2D,
	MUSA_HWPERF_DM_TA,
	MUSA_HWPERF_DM_3D,
	MUSA_HWPERF_DM_CMP,
	MUSA_HWPERF_DM_RTU,
	MUSA_HWPERF_DM_SHG,
	MUSA_HWPERF_DM_TRANSFER,

	MUSA_HWPERF_DM_LAST,

	MUSA_HWPERF_DM_INVALID = 0x1FFFFFFF
} MUSA_HWPERF_DM;

/*! Enum containing bit position for 32bit feature flags used in hwperf and api */
typedef enum {
	MUSA_HWPERF_FEATURE_PERFBUS_FLAG                = 0x0001,
	MUSA_HWPERF_FEATURE_S7_TOP_INFRASTRUCTURE_FLAG  = 0x0002,
	MUSA_HWPERF_FEATURE_XT_TOP_INFRASTRUCTURE_FLAG  = 0x0004,
	MUSA_HWPERF_FEATURE_PERF_COUNTER_BATCH_FLAG     = 0x0008,
	MUSA_HWPERF_FEATURE_MTIPXE_FLAG                = 0x0010,
	MUSA_HWPERF_FEATURE_DUST_POWER_ISLAND_S7_FLAG   = 0x0020,
	MUSA_HWPERF_FEATURE_PBE2_IN_XE_FLAG             = 0x0040,
	MUSA_HWPERF_FEATURE_WORKLOAD_ESTIMATION         = 0x0080,
	MUSA_HWPERF_FEATURE_MULTICORE_FLAG              = 0x0100,
	MUSA_HWPERF_FEATURE_RAYTRACING_FLAG             = 0x0200,
	MUSA_HWPERF_FEATURE_CXT_TOP_INFRASTRUCTURE_FLAG = 0x0400,
	MUSA_HWPERF_FEATURE_MUSA_FLAG               = 0x0800,
	MUSA_HWPERF_FEATURE_MTIP_FLAG                  = 0x1000,
	MUSA_HWPERF_FEATURE_OCEANIC_FLAG                = 0x2000
} MUSA_HWPERF_FEATURE_FLAGS;

/*! This structure holds the data of a firmware packet. */
typedef struct
{
	MUSA_HWPERF_DM eDM;				/*!< DataMaster identifier, see MUSA_HWPERF_DM */
	MT_UINT32 ui32TxtActCyc;		/*!< Mcu TXTACTCYC register value */
	MT_UINT32 ui32FWPerfCount0;	/*!< Mcu/MIPS PERF_COUNT0 register */
	MT_UINT32 ui32FWPerfCount1;	/*!< Mcu/MIPS PERF_COUNT1 register */
	MT_UINT64 ui64SoCTimestamp;	/*!< SoC timestamp */
	MT_UINT32 ui32TimeCorrIndex;	/*!< Internal field */
	MT_UINT32 ui32Padding;			/*!< Reserved */
} MUSA_HWPERF_FW_DATA;

MUSA_FW_STRUCT_SIZE_ASSERT(MUSA_HWPERF_FW_DATA);

typedef enum
{
	MUSA_HWPERF_FW_DBG_SECTION_A_START,
	MUSA_HWPERF_FW_DBG_SECTION_A_FINISH,
	MUSA_HWPERF_FW_DBG_SECTION_B_START,
	MUSA_HWPERF_FW_DBG_SECTION_B_FINISH,
	MUSA_HWPERF_FW_DBG_SECTION_C_START,
	MUSA_HWPERF_FW_DBG_SECTION_C_FINISH,
	MUSA_HWPERF_FW_DBG_SECTION_D_START,
	MUSA_HWPERF_FW_DBG_SECTION_D_FINISH,
	MUSA_HWPERF_FW_DBG_SECTION_E_START,
	MUSA_HWPERF_FW_DBG_SECTION_E_FINISH,
} MUSA_HWPERF_FW_DBG_SECTION;

typedef struct
{
	MT_UINT64                ui64SoCTimestamp;  /*!< SoC timestamp */
	MT_UINT32                ui32TimeCorrIndex; /*!< Index to the time correlation when the packet was generated */
	MUSA_HWPERF_DM             eDM;               /*!< Data Master number, see MUSA_HWPERF_DM */
	MUSA_HWPERF_FW_DBG_SECTION eSection;          /*!< Code section to profile distinguishable by A, B, C, D, E */
	MT_UINT32                ui32DMContext;     /*!< GPU Data Master (FW) Context */
	MT_UINT64                ui64UserData1;     /*!< Customized data section */
	MT_UINT64                ui64UserData2;
	MT_UINT64                ui64UserData3;
} MUSA_HWPERF_FW_DBG_DATA;

MUSA_FW_STRUCT_SIZE_ASSERT(MUSA_HWPERF_FW_DBG_DATA);

/*! This structure holds the data of a hardware packet, including counters. */
typedef struct
{
	MT_UINT32 ui32DMCyc;         /*!< DataMaster cycle count register, 0 if none */
	MT_UINT32 ui32FrameNum;      /*!< Frame number, undefined on some DataMasters */
	MT_UINT32 ui32PID;           /*!< Process identifier */
	MT_UINT32 ui32DMContext;     /*!< GPU Data Master (FW) Context */
	MT_UINT32 ui32WorkTarget;    /*!< RenderTarget for a TA,3D; Frame context for RTU, 0x0 otherwise */
	MT_UINT32 ui32ExtJobRef;     /*!< Client driver context job reference used for tracking/debugging */
	MT_UINT32 ui32IntJobRef;     /*!< MUSA Data master context job reference used for tracking/debugging */
	MT_UINT64 ui64SoCTimestamp;  /*!< SoC timestamp */
	MT_UINT32 ui32TimeCorrIndex; /*!< Index to the time correlation at the time the packet was generated */
	MT_UINT32 ui32BlkInfo;       /*!< <31..16> NumBlocks <15..0> Counter block stream offset */
	MT_UINT32 ui32WorkCtx;       /*!< Work context: Render Context for TA/3D; RayTracing Context for RTU/SHG; 0x0 otherwise */
	MT_UINT32 ui32CtxPriority;   /*!< Context priority */
	MT_UINT32 ui32GPUIdMask;     /*!< GPU IDs active within this event */
	MT_UINT64 ui64PFMNormDumpCntr; /*!< Record the number of dumps in PFM normal mode*/
	MT_UINT32 ui32KickInfo;      /*!< <31..8> Reserved <7..0> GPU Pipeline DM kick ID, 0 if not using Pipeline DMs */
	MT_UINT32 ui32Padding;       /*!< Reserved. To ensure correct alignment */
	MT_UINT32 aui32CountBlksStream[MUSA_HWPERF_ZERO_OR_MORE_ELEMENTS]; /*!< Optional variable length Counter data */
	MT_UINT32 ui32Padding2;      /*!< Reserved. To ensure correct alignment (not written in the packet) */
} MUSA_HWPERF_HW_DATA;

MUSA_FW_STRUCT_SIZE_ASSERT(MUSA_HWPERF_HW_DATA);
MUSA_FW_STRUCT_OFFSET_ASSERT(MUSA_HWPERF_HW_DATA, aui32CountBlksStream);

/*! Mask for use with the aui32CountBlksStream field when decoding the
 * counter block ID and mask word. */
#define MUSA_HWPERF_CNTBLK_ID_MASK	0xFFFF0000U
#define MUSA_HWPERF_CNTBLK_ID_SHIFT	16U

/*! MAX value used in server handling of counter config arrays */
#define MUSA_CNTBLK_COUNTERS_MAX MTGPU_HWPERF_COUNTERS_PERBLK


/*! Obtains the counter block ID word from an aui32CountBlksStream field.
 * The word combines Control bits (15-12), GPU-Id (11-8), Group (7-4), Unit
 * within group (3-0) */
#define MUSA_HWPERF_GET_CNTBLK_IDW(_word)           ((MT_UINT16)(((_word)&MUSA_HWPERF_CNTBLK_ID_MASK)>>MUSA_HWPERF_CNTBLK_ID_SHIFT))

/*! Obtains the counter block ID from the supplied MUSA_HWPERF_HW_DATA address
 * and stream index. May be used in decoding the counter block stream words of
 * a MUSA_HWPERF_HW_DATA structure. */
#define MUSA_HWPERF_GET_CNTBLK_ID(_data_addr, _idx) MUSA_HWPERF_GET_CNTBLK_IDW((_data_addr)->aui32CountBlksStream[(_idx)])

/*! Obtains the GPU ID from the supplied MUSA_HWPERF_HW_DATA CNTBLK_IDW */
#define MUSA_HWPERF_GET_CNTBLK_GPUW(_word)          ((MT_UINT16)(((_word)&MUSA_CNTBLK_ID_MC_GPU_MASK)>>MUSA_CNTBLK_ID_MC_GPU_SHIFT))

#define MUSA_HWPERF_GET_CNT_MASKW(_word)           ((MT_UINT16)((_word)&(~MUSA_HWPERF_CNTBLK_ID_MASK)))

/*! Obtains the counter mask from the supplied MUSA_HWPERF_HW_DATA address
 * and stream index. May be used in decoding the counter block stream words
 * of a MUSA_HWPERF_HW_DATA structure. */
#define MUSA_HWPERF_GET_CNT_MASK(_data_addr, _idx) MUSA_HWPERF_GET_CNT_MASKW((_data_addr)->aui32CountBlksStream[(_idx)])

/*! Context switch packet event */
typedef struct
{
	MUSA_HWPERF_DM	eDM;					/*!< DataMaster identifier, see MUSA_HWPERF_DM */
	MT_UINT32		ui32DMContext;			/*!< GPU Data Master (FW) Context */
	MT_UINT32		ui32FrameNum;			/*!< Client Frame number (TA, 3D only) */
	MT_UINT32		ui32TxtActCyc;			/*!< Mcu TXTACTCYC register value */
	MT_UINT32		ui32PerfCycle;			/*!< Cycle count. Used to measure HW context store latency */
	MT_UINT32		ui32PerfPhase;			/*!< Phase. Used to determine geometry content */
	MT_UINT32		ui32Padding[2];			/*!< Padding to 8 DWords */
} MUSA_HWPERF_CSW_DATA;

MUSA_FW_STRUCT_SIZE_ASSERT(MUSA_HWPERF_CSW_DATA);

/*! Enumeration of clocks supporting this event */
typedef enum
{
	MUSA_HWPERF_CLKS_CHG_INVALID = 0,

	MUSA_HWPERF_CLKS_CHG_NAME_CORE = 1,

	MUSA_HWPERF_CLKS_CHG_LAST,
} MUSA_HWPERF_CLKS_CHG_NAME;

/*! This structure holds the data of a clocks change packet. */
typedef struct
{
	MT_UINT64                ui64NewClockSpeed;         /*!< New Clock Speed (in Hz) */
	MUSA_HWPERF_CLKS_CHG_NAME  eClockName;                /*!< Clock name */
	MT_UINT32                ui32CalibratedClockSpeed;  /*!< Calibrated new GPU clock speed (in Hz) */
	MT_UINT64                ui64OSTimeStamp;           /*!< OSTimeStamp sampled by the host */
	MT_UINT64                ui64CRTimeStamp;           /*!< CRTimeStamp sampled by the host and
	                                                          correlated to OSTimeStamp */
} MUSA_HWPERF_CLKS_CHG_DATA;

MUSA_FW_STRUCT_SIZE_ASSERT(MUSA_HWPERF_CLKS_CHG_DATA);

/*! Enumeration of GPU utilisation states supported by this event */
typedef MT_UINT32 MUSA_HWPERF_GPU_STATE;

/*! This structure holds the data of a GPU utilisation state change packet. */
typedef struct
{
	MUSA_HWPERF_GPU_STATE	eState;		/*!< New GPU utilisation state */
	MT_UINT32				uiUnused1;	/*!< Padding */
	MT_UINT32				uiUnused2;	/*!< Padding */
	MT_UINT32				uiUnused3;	/*!< Padding */
} MUSA_HWPERF_GPU_STATE_CHG_DATA;

MUSA_FW_STRUCT_SIZE_ASSERT(MUSA_HWPERF_GPU_STATE_CHG_DATA);


/*! Signature pattern 'HPE1' found in the first word of a PWR_EST packet data */
#define HWPERF_PWR_EST_V1_SIG	0x48504531

/*! Macros to obtain a component field from a counter ID word */
#define MUSA_HWPERF_GET_PWR_EST_HIGH_FLAG(_word) (((_word)&0x80000000)>>31)
#define MUSA_HWPERF_GET_PWR_EST_GPUID(_word)     (((_word)&0x70000000)>>28)
/*!< Obtains the GPU ID from a counter ID word */
#define MUSA_HWPERF_GET_PWR_EST_UNIT(_word)      (((_word)&0x0F000000)>>24)
#define MUSA_HWPERF_GET_PWR_EST_NUMBER(_word)    ((_word)&0x0000FFFF)

#define MUSA_HWPERF_PWR_EST_HIGH_OFFSET		(31)
#define MUSA_HWPERF_PWR_EST_GPUID_OFFSET		(28)
#define MUSA_HWPERF_PWR_EST_GPUID_MASK		(0x7U)
#define MUSA_HWPERF_PWR_EST_UNIT_OFFSET		(24)
#define MUSA_HWPERF_PWR_EST_UNIT_MASK		(0xFU)
#define MUSA_HWPERF_PWR_EST_VALUE_MASK		(0xFFFFU)

/*! This macro constructs a counter ID for a power estimate data stream from
 * the component parts of: high word flag, unit id, GPU id, counter number */
#define MUSA_HWPERF_MAKE_PWR_EST_COUNTERID(_high, _unit, _core, _number) \
			((MT_UINT32)(((MT_UINT32)((MT_UINT32)(_high)&0x1U)<<MUSA_HWPERF_PWR_EST_HIGH_OFFSET) | \
			((MT_UINT32) ((MT_UINT32)(_unit)&MUSA_HWPERF_PWR_EST_UNIT_MASK)<<MUSA_HWPERF_PWR_EST_UNIT_OFFSET) | \
			((MT_UINT32) ((MT_UINT32)(_core)&MUSA_HWPERF_PWR_EST_GPUID_MASK)<<MUSA_HWPERF_PWR_EST_GPUID_OFFSET) | \
			                           ((_number)&MUSA_HWPERF_PWR_EST_VALUE_MASK)))

/*! This structure holds the data for a power estimate packet. */
typedef struct
{
	MT_UINT32  ui32StreamVersion;  /*!< Version word, HWPERF_PWR_EST_V1_SIG */
	MT_UINT32  ui32StreamSize;     /*!< Size of array in bytes of stream data
	                                     held in the aui32StreamData member */
	MT_UINT32  aui32StreamData[MUSA_HWPERF_ONE_OR_MORE_ELEMENTS]; /*!< Counter data */
	MT_UINT32  ui32Padding; /*!< Reserved. To ensure correct alignment */
} MUSA_HWPERF_PWR_EST_DATA;

MUSA_FW_STRUCT_SIZE_ASSERT(MUSA_HWPERF_PWR_EST_DATA);

/*! Enumeration of the kinds of power change events that can occur */
typedef enum
{
	MUSA_HWPERF_PWR_UNDEFINED    = 0,
	MUSA_HWPERF_PWR_ON           = 1, /*!< Whole device powered on */
	MUSA_HWPERF_PWR_OFF          = 2, /*!< Whole device powered off */
	MUSA_HWPERF_PWR_UP           = 3, /*!< Power turned on to a HW domain */
	MUSA_HWPERF_PWR_DOWN         = 4, /*!< Power turned off to a HW domain */
	MUSA_HWPERF_PWR_SAFETY_RESET = 5, /*!< Resetting the GPU HW units for safety reasons */
	MUSA_HWPERF_PWR_PHR_FULL     = 6, /*!< Periodic HW full GPU Reset */

	MUSA_HWPERF_PWR_LAST,
} MUSA_HWPERF_PWR;

/*! This structure holds the data of a power packet. */
typedef struct
{
	MUSA_HWPERF_PWR eChange;                  /*!< Defines the type of power change */
	MT_UINT32     ui32Domains;              /*!< HW Domains affected */
	MT_UINT64     ui64OSTimeStamp;          /*!< OSTimeStamp sampled by the host */
	MT_UINT64     ui64CRTimeStamp;          /*!< CRTimeStamp sampled by the host and
	                                              correlated to OSTimeStamp */
	MT_UINT32     ui32CalibratedClockSpeed; /*!< GPU clock speed (in Hz) at the time
	                                              the two timers were correlated */
	MT_UINT32     ui32Unused1;              /*!< Padding */
} MUSA_HWPERF_PWR_CHG_DATA;

MUSA_FW_STRUCT_SIZE_ASSERT(MUSA_HWPERF_PWR_CHG_DATA);


/*
 * PDVFS, GPU clock frequency changes and workload estimation profiling
 * data.
 */
/*! DVFS and work estimation events. */
typedef enum
{
	MUSA_HWPERF_DVFS_EV_INVALID,                 /*!< Invalid value. */
	MUSA_HWPERF_DVFS_EV_PROACTIVE_EST_START,     /*!< Proactive DVFS estimate start */
	MUSA_HWPERF_DVFS_EV_PROACTIVE_EST_FINISHED,  /*!< Proactive DVFS estimate finished */
	MUSA_HWPERF_DVFS_EV_REACTIVE_EST_START,      /*!< Reactive DVFS estimate start */
	MUSA_HWPERF_DVFS_EV_REACTIVE_EST_FINISHED,   /*!< Reactive DVFS estimate finished */
	/* workload estimation */
	MUSA_HWPERF_DVFS_EV_WORK_EST_START,          /*!< Workload estimation start */
	MUSA_HWPERF_DVFS_EV_WORK_EST_FINISHED,       /*!< Workload estimation finished */
	MUSA_HWPERF_DVFS_EV_FREQ_CHG,                /*!< DVFS OPP/clock frequency change */

	MUSA_HWPERF_DVFS_EV_LAST               /*!< Number of element. */
} MUSA_HWPERF_DVFS_EV;

/*! Enumeration of DVFS transitions that can occur */
typedef enum
{
	MUSA_HWPERF_DVFS_OPP_NONE        = 0x0,  /*!< No OPP change, already operating at required freq */
#if defined(SUPPORT_PDVFS_IDLE)
	MUSA_HWPERF_DVFS_OPP_IDLE        = 0x1,  /*!< GPU is idle, defer the OPP change */
#endif
	/* 0x2 to 0xF reserved */
	MUSA_HWPERF_DVFS_OPP_UPDATE      = 0x10, /*!< OPP change, new point is encoded in bits [3:0] */
	MUSA_HWPERF_DVFS_OPP_LAST        = 0x20,
} MUSA_HWPERF_DVFS_OPP;

typedef union
{
	/*! This structure holds the data of a proactive DVFS calculation packet. */
	struct
	{
		MT_UINT64     ui64DeadlineInus;         /*!< Next deadline in microseconds */
		MT_UINT32     ui32Frequency;            /*!< Required freq to meet deadline at 90% utilisation */
		MT_UINT32     ui32WorkloadCycles;       /*!< Current workload estimate in cycles */
		MT_UINT32     ui32TxtActCyc;            /*!< Mcu TXTACTCYC register value */
	} sProDVFSCalc;

	/*! This structure holds the data of a reactive DVFS calculation packet. */
	struct
	{
		MT_UINT32     ui32Frequency;            /*!< Required freq to achieve average 90% utilisation */
		MT_UINT32     ui32Utilisation;          /*!< GPU utilisation since last update */
		MT_UINT32     ui32TxtActCyc;            /*!< Mcu TXTACTCYC register value */
	} sDVFSCalc;

	/*! This structure holds the data of a work estimation packet. */
	struct
	{
		MT_UINT32     ui32CyclesPrediction;     /*!< Predicted cycle count for this workload */
		MT_UINT32     ui32CyclesTaken;          /*!< Actual cycle count for this workload */
		MUSAFWIF_DM     eDM;                      /*!< Target DM */
		MT_UINT32     ui32ReturnDataIndex;      /*!< Index into workload estimation table */
		MT_UINT32     ui32TxtActCyc;            /*!< Mcu TXTACTCYC register value */
	} sWorkEst;

	/*! This structure holds the data of an OPP clock frequency transition packet. */
	struct
	{
		MT_UINT32     ui32OPPData;              /*!< OPP transition */
	} sOPP;

} MUSA_HWPERF_DVFS_DETAIL;

/*! DVFS sub-event data structure */
typedef struct {
	MUSA_HWPERF_DVFS_EV      eEventType;          /*!< DVFS sub-event type */
	MUSA_HWPERF_DVFS_DETAIL  uData;               /*!< DVFS sub-event data */
} MUSA_HWPERF_DVFS_DATA;

MUSA_FW_STRUCT_SIZE_ASSERT(MUSA_HWPERF_DVFS_DATA);


/*! Firmware Activity event. */
typedef enum
{
	MUSA_HWPERF_FWACT_EV_INVALID,            /*!< Invalid value. */
	MUSA_HWPERF_FWACT_EV_REGS_SET,           /*!< Registers set. */
	MUSA_HWPERF_FWACT_EV_HWR_DETECTED,       /*!< HWR detected. */
	MUSA_HWPERF_FWACT_EV_HWR_RESET_REQUIRED, /*!< Reset required. */
	MUSA_HWPERF_FWACT_EV_HWR_RECOVERED,      /*!< HWR recovered. */
	MUSA_HWPERF_FWACT_EV_HWR_FREELIST_READY, /*!< Freelist ready. */
	MUSA_HWPERF_FWACT_EV_FEATURES,           /*!< Features present */
	MUSA_HWPERF_FWACT_EV_FILTER_SET,         /*!< Event filter set. */

	MUSA_HWPERF_FWACT_EV_LAST                /*!< Number of element. */
} MUSA_HWPERF_FWACT_EV;

/*! Cause of the HWR event. */
typedef enum
{
	MUSA_HWPERF_HWR_REASON_INVALID,              /*!< Invalid value. */
	MUSA_HWPERF_HWR_REASON_LOCKUP,               /*!< Lockup. */
	MUSA_HWPERF_HWR_REASON_PAGEFAULT,            /*!< Page fault. */
	MUSA_HWPERF_HWR_REASON_POLLFAIL,             /*!< Poll fail. */
	MUSA_HWPERF_HWR_REASON_DEADLINE_OVERRUN,     /*!< Deadline overrun. */
	MUSA_HWPERF_HWR_REASON_CSW_DEADLINE_OVERRUN, /*!< Hard Context Switch deadline overrun. */

	MUSA_HWPERF_HWR_REASON_LAST                  /*!< Number of elements. */
} MUSA_HWPERF_HWR_REASON;


/* Fixed size for BVNC string so it does not alter packet data format
 * Check it is large enough against official BVNC string length maximum
 */
#define MUSA_HWPERF_MAX_BVNC_LEN (24)
static_assert((MUSA_HWPERF_MAX_BVNC_LEN >= MUSA_BVNC_STR_SIZE_MAX),
			  "Space inside HWPerf packet data for BVNC string insufficient");

#define MUSA_HWPERF_MAX_BVNC_BLOCK_LEN (20U)

/*! BVNC Features */
typedef struct
{
	/*! Counter block ID, see MUSA_HWPERF_CNTBLK_ID */
	MT_UINT16 ui16BlockID;

	/*! Number of counters in this block type */
	MT_UINT16 ui16NumCounters;

	/*! Number of blocks of this type */
	MT_UINT16 ui16NumBlocks;

	/*! Reserved for future use */
	MT_UINT16 ui16Reserved;
} MUSA_HWPERF_BVNC_BLOCK;

/*! BVNC Features */
typedef struct
{
	MT_CHAR aszBvncString[MUSA_HWPERF_MAX_BVNC_LEN]; /*!< BVNC string */
	MT_UINT32 ui32BvncKmFeatureFlags;               /*!< See MUSA_HWPERF_FEATURE_FLAGS */
	MT_UINT16 ui16BvncBlocks;                       /*!< Number of blocks described in aBvncBlocks */
	MT_UINT16 ui16BvncGPUCores;                     /*!< Number of GPU cores present */
	MUSA_HWPERF_BVNC_BLOCK aBvncBlocks[MUSA_HWPERF_MAX_BVNC_BLOCK_LEN];  /*!< Supported Performance Blocks for BVNC. See MUSA_HWPERF_BVNC_BLOCK */
} MUSA_HWPERF_BVNC;

MUSA_FW_STRUCT_SIZE_ASSERT(MUSA_HWPERF_BVNC);

/*! Performance Counter Configuration data element. */
typedef struct
{
	MT_UINT32 ui32BlockID;               /*!< Counter Block ID. See MUSA_HWPERF_CNTBLK_ID */
	MT_UINT32 ui32NumCounters;           /*!< Number of counters configured */
	MT_UINT32 ui32CounterVals[MUSA_CNTBLK_COUNTERS_MAX];  /*!< Counters configured (ui32NumCounters worth of entries) */
} MUSA_HWPERF_COUNTER_CFG_DATA_EL;

MUSA_FW_STRUCT_SIZE_ASSERT(MUSA_HWPERF_COUNTER_CFG_DATA_EL);

/*! Performance Counter Configuration data. */
typedef struct
{
	MT_UINT32 ui32EnabledBlocks;          /*!< Number of Enabled Blocks. */
	MUSA_HWPERF_COUNTER_CFG_DATA_EL uData;  /*!< Start of variable length data. See MUSA_HWPERF_COUNTER_CFG_DATA_EL */
	MT_UINT32 ui32Padding;                /*!< reserved */
} MUSA_HWPERF_COUNTER_CFG;

MUSA_FW_STRUCT_SIZE_ASSERT(MUSA_HWPERF_COUNTER_CFG);

/*! Sub-event's data. */
typedef union
{
	struct
	{
		MUSA_HWPERF_DM eDM;				/*!< Data Master ID. */
		MUSA_HWPERF_HWR_REASON eReason;	/*!< Reason of the HWR. */
		MT_UINT32 ui32DMContext;		/*!< FW render context */
	} sHWR;								/*!< HWR sub-event data. */

	MUSA_HWPERF_BVNC sBVNC;              /*!< BVNC Features. See MUSA_HWPERF_BVNC */
	struct
	{
		MT_UINT32 ui32EvMaskLo;		/*!< Low order 32 bits of Filter Mask */
		MT_UINT32 ui32EvMaskHi;		/*!< High order 32 bits of Filter Mask */
	} sEvMsk;							/*!< HW Filter Mask */
	MUSA_HWPERF_COUNTER_CFG sPCC;        /*!< Performance Counter Config. See MUSA_HWPERF_COUNTER_CFG */
} MUSA_HWPERF_FWACT_DETAIL;

MUSA_FW_STRUCT_SIZE_ASSERT(MUSA_HWPERF_FWACT_DETAIL);

/*! This structure holds the data of a FW activity event packet */
typedef struct
{
	MUSA_HWPERF_FWACT_EV eEvType;           /*!< Event type. */
	MUSA_HWPERF_FWACT_DETAIL uFwActDetail;  /*!< Data of the sub-event. */
	MT_UINT32 ui32Padding;                /*!< Reserved. */
} MUSA_HWPERF_FWACT_DATA;

MUSA_FW_STRUCT_SIZE_ASSERT(MUSA_HWPERF_FWACT_DATA);


typedef enum {
	MUSA_HWPERF_UFO_EV_UPDATE,          /*!< Update on the UFO objects. */
	MUSA_HWPERF_UFO_EV_CHECK_SUCCESS,   /*!< Successful check on UFO objects. */
	MUSA_HWPERF_UFO_EV_PRCHECK_SUCCESS, /*!< Successful partial render check on UFO objects. */
	MUSA_HWPERF_UFO_EV_CHECK_FAIL,      /*!< Unsuccessful check on UFO objects. */
	MUSA_HWPERF_UFO_EV_PRCHECK_FAIL,    /*!< Unsuccessful partial render check on UFO objects. */
	MUSA_HWPERF_UFO_EV_FORCE_UPDATE,    /*!< Forced erroring of the UFO objects. */

	MUSA_HWPERF_UFO_EV_LAST             /*!< Reserved. Do not use. */
} MUSA_HWPERF_UFO_EV;

/*! Data stream tuple. */
typedef union
{
	struct
	{
		MT_UINT32 ui32FWAddr;        /*!< UFO's unique address */
		MT_UINT32 ui32Value;         /*!< Value of the UFO object */
	} sCheckSuccess;
	struct
	{
		MT_UINT32 ui32FWAddr;        /*!< UFO's unique address */
		MT_UINT32 ui32Value;         /*!< Value of the UFO object */
		MT_UINT32 ui32Required;      /*!< Value of the UFO object required by the fence */
	} sCheckFail;
	struct
	{
		MT_UINT32 ui32FWAddr;        /*!< UFO's unique address */
		MT_UINT32 ui32OldValue;      /*!< Value of UFO object before update */
		MT_UINT32 ui32NewValue;      /*!< Value of UFO object after update */
	} sUpdate;
} MUSA_HWPERF_UFO_DATA_ELEMENT;

/*! This structure holds the packet payload data for UFO event. */
typedef struct
{
	MUSA_HWPERF_UFO_EV eEvType;                                      /*!< Subtype of the event. See MUSA_HWPERF_UFO_EV */
	MT_UINT64        ui64SoCTimestamp;                             /*!< SoC timestamp */
	MT_UINT32        ui32TimeCorrIndex;                            /*!< Index to the timer correlation data
	                                                                  at the time the packet was generated.
	                                                                  Used to approximate Host timestamps for
	                                                                  these events. */
	MT_UINT32 ui32PID;                                             /*!< Client process identifier */
	MT_UINT32 ui32ExtJobRef;                                       /*!< Reference used by callers of the MUSA
	                                                                  API to track submitted work (for
	                                                                  debugging/trace purposes) */
	MT_UINT32 ui32IntJobRef;                                       /*!< Internal reference used to track
	                                                                  submitted work (for debugging / trace
	                                                                  purposes) */
	MT_UINT32 ui32DMContext;                                       /*!< GPU Data Master (FW) Context */
	MT_UINT32 ui32StreamInfo;                                      /*!< Encoded number of elements in the
	                                                                  stream and stream data offset in the
	                                                                  payload */
	MUSA_HWPERF_DM eDM;                                              /*!< Data Master number, see MUSA_HWPERF_DM */
	MT_UINT32    ui32Padding;                                      /*!< Unused, reserved */
	MT_UINT32    aui32StreamData[MUSA_HWPERF_ONE_OR_MORE_ELEMENTS]; /*!< Series of tuples holding UFO objects data */
} MUSA_HWPERF_UFO_DATA;

MUSA_FW_STRUCT_SIZE_ASSERT(MUSA_HWPERF_UFO_DATA);


/*!
 * MUSA_HWPERF_KICK_TYPE describes the type of kick for events received / sent
 * between KICK_START / KICK_END inclusively for all event types.
 */
typedef enum
{
	MUSA_HWPERF_KICK_TYPE_GFX,  /*!< Replaced by separate TA and 3D types */
	MUSA_HWPERF_KICK_TYPE_CMP,   /*!< Compute Data Master Kick */
	MUSA_HWPERF_KICK_TYPE_RS,    /*!< Ray Store Kick */
	MUSA_HWPERF_KICK_TYPE_SHG,   /*!< Scene Hierarchy Generator Kick */
	MUSA_HWPERF_KICK_TYPE_TQTRANSFER, /*!< TQ 2D Data Master Kick */
	MUSA_HWPERF_KICK_TYPE_SYNC,  /*!< Sync Kick */
	MUSA_HWPERF_KICK_TYPE_TA,    /*!< TA Kick */
	MUSA_HWPERF_KICK_TYPE_3D,    /*!< 3D Kick */
	MUSA_HWPERF_KICK_TYPE_CE,    /*!< ce Kick */
	MUSA_HWPERF_KICK_TYPE_LAST,

	MUSA_HWPERF_KICK_TYPE_FORCE_32BIT = 0x7fffffff
} MUSA_HWPERF_KICK_TYPE;

typedef struct
{
	MUSA_HWPERF_KICK_TYPE ui32EnqType; /*!< Workload type sent to FW for
	                                   scheduling on GPU hardware.
	                                   See MUSA_HWPERF_KICK_TYPE */
	MT_UINT32 ui32PID;               /*!< Client process identifier */
	MT_UINT32 ui32ExtJobRef;         /*!< Reference used by callers of the MUSA API
	                                   to track submitted work (for debugging /
	                                   trace purposes) */
	MT_UINT32 ui32IntJobRef;         /*!< internal reference used to track submitted
	                                   work (for debugging / trace purposes) */
	MT_UINT32 ui32DMContext;         /*!< GPU Data Master (FW) Context */
	MT_UINT32 ui32Padding;           /*!< Unused, reserved */
	MT_UINT64 ui64CheckFence_UID;    /*!< ID of fence gating work execution on GPU */
	MT_UINT64 ui64UpdateFence_UID;   /*!< ID of fence triggered after work completes on GPU */
	MT_UINT64 ui64DeadlineInus;      /*!< Workload deadline in system monotonic time */
	MT_UINT32 ui32CycleEstimate;     /*!< Estimated cycle time for the workload */
	MTGPU_FENCE hCheckFence;         /*!< Fence this enqueue task waits for, before starting */
	MTGPU_FENCE hUpdateFence;        /*!< Fence this enqueue task signals, on completion */
	MTGPU_TIMELINE hUpdateTimeline;  /*!< Timeline on which the above hUpdateFence is created */

	/* Align structure size to 8 bytes */
} MUSA_HWPERF_HOST_ENQ_DATA;

/* Payload size must be multiple of 8 bytes to align start of next packet. */
static_assert((sizeof(MUSA_HWPERF_HOST_ENQ_DATA) & (MTGPUTL_PACKET_ALIGNMENT-1U)) == 0U,
			  "sizeof(MUSA_HWPERF_HOST_ENQ_DATA) must be a multiple MTGPUTL_PACKET_ALIGNMENT");

typedef struct
{
	MUSA_HWPERF_KICK_TYPE ui32NotifyType; /*!< Workload type sent to FW for scheduling on GPU hardware.
	                                      See MUSA_HWPERF_KICK_TYPE */
	MT_UINT32 ui32PID;                  /*!< Client process identifier */
	MT_UINT32 ui32DMContext;            /*!< GPU Data Master (FW) Context */
	MT_UINT32 ui32Padding;              /*!< Align structure size to 8 bytes */
} MUSA_HWPERF_HOST_NOTIFY_DATA;

/* Payload size must be multiple of 8 bytes to align start of next packet. */
static_assert((sizeof(MUSA_HWPERF_HOST_NOTIFY_DATA) & (MTGPUTL_PACKET_ALIGNMENT-1U)) == 0U,
			  "sizeof(MUSA_HWPERF_HOST_NOTIFY_DATA) must be a multiple MTGPUTL_PACKET_ALIGNMENT");

typedef struct
{
	MUSA_HWPERF_UFO_EV eEvType;   /*!< Subtype of the event */
	MT_UINT32 ui32StreamInfo;   /*!< Encoded number of elements in the stream and
	                              stream data offset in the payload */
#ifdef __CHECKER__
	/* Since we're not conforming to the C99 standard by not using a flexible
	 * array member need to add a special case for Smatch static code analyser. */
	MT_UINT32 aui32StreamData[];
#else
	MT_UINT32 aui32StreamData[MUSA_HWPERF_ONE_OR_MORE_ELEMENTS];
	                             /*!< Series of tuples holding UFO objects data */

	MT_UINT32 ui32Padding;      /*!< Reserved, align structure size to 8 bytes */
#endif
} MUSA_HWPERF_HOST_UFO_DATA;

/* Payload size must be multiple of 8 bytes to align start of next packet. */
static_assert((sizeof(MUSA_HWPERF_HOST_UFO_DATA) & (MTGPUTL_PACKET_ALIGNMENT-1U)) == 0U,
			  "sizeof(MUSA_HWPERF_HOST_UFO_DATA) must be a multiple MTGPUTL_PACKET_ALIGNMENT");

/*!
 * MUSA_HWPERF_HOST_RESOURCE_TYPE describes the type of resource which has been
 * Allocated, Freed or Modified. The values are used to determine which event
 * data structure to use to decode the data from the event stream
 */
typedef enum
{
	MUSA_HWPERF_HOST_RESOURCE_TYPE_INVALID,   /*!< Invalid */
	MUSA_HWPERF_HOST_RESOURCE_TYPE_SYNC,      /*!< SyncPrim */
	MUSA_HWPERF_HOST_RESOURCE_TYPE_TIMELINE_DEPRECATED,
	                                         /*!< Timeline resource packets are
	                                           now emitted in client hwperf buffer */
	MUSA_HWPERF_HOST_RESOURCE_TYPE_FENCE_MT, /*!< Fence for use on GPU (SYNC_CP backed) */
	MUSA_HWPERF_HOST_RESOURCE_TYPE_SYNC_CP,   /*!< Sync Checkpoint */
	MUSA_HWPERF_HOST_RESOURCE_TYPE_FENCE_SW,  /*!< Fence created on SW timeline */

	MUSA_HWPERF_HOST_RESOURCE_TYPE_LAST       /*!< End of enumeration */
} MUSA_HWPERF_HOST_RESOURCE_TYPE;

typedef union
{
	/*! Data for TYPE_TIMELINE (*Deprecated*). This sub-event is no longer
     *  generated in the HOST stream. Timeline data is now provided in the
     *  CLIENT stream instead.
     */
	struct
	{
		MT_UINT32 uiPid;             /*!< Identifier of owning process */
		MT_UINT64 ui64Timeline_UID1; /*!< Unique identifier for timeline resource */
		MT_CHAR acName[MTGPU_SYNC_NAME_LENGTH];
		                              /*!< Label or name given to the sync resource */
		MT_UINT32 ui32Padding;       /*!< Reserved. Align structure size to 8 bytes */
	} sTimelineAlloc;

	/*! Data for TYPE_FENCE_MT */
	struct
	{
		MT_PID uiPID;                /*!< Identifier of owning process */
		MTGPU_FENCE hFence;          /*!< Unique identifier for the fence resource */
		MT_UINT32 ui32CheckPt_FWAddr; /*!< Unique identifier of the check point
		                                backing this fence on the GPU */
		MT_CHAR acName[MTGPU_SYNC_NAME_LENGTH];
		                              /*!< Label or name given to the sync resource */
	} sFenceAlloc;

	/*! Data for TYPE_SYNC_CP */
	struct
	{
		MT_UINT32 ui32CheckPt_FWAddr; /*!< Unique identifier for the check point resource */
		MTGPU_TIMELINE hTimeline;     /*!< Unique identifier for the timeline resource */
		MT_PID uiPID;                 /*!< Identifier of owning process */
		MTGPU_FENCE hFence;           /*!< Unique identifier for the fence resource */
		MT_CHAR acName[MTGPU_SYNC_NAME_LENGTH];
		                               /*!< Label or name given to the sync resource */
	} sSyncCheckPointAlloc;

	/*! Data for TYPE_FENCE_SW */
	struct
	{
		MT_PID uiPID;                 /*!< Identifier of owning process */
		MTGPU_FENCE hSWFence;         /*!< Unique identifier for the SWFence resource */
		MTGPU_TIMELINE hSWTimeline;   /*!< Unique identifier for the timeline resource */
		MT_UINT64 ui64SyncPtIndex;    /*!< Sync-pt index where this SW timeline has reached */
		MT_CHAR acName[MTGPU_SYNC_NAME_LENGTH];
		                               /*!< Label or name given to the sync resource */
	} sSWFenceAlloc;

	/*! Data for TYPE_SYNC */
	struct
	{
		MT_UINT32 ui32FWAddr;         /*!< Identifier of sync resource */
		MT_CHAR acName[MTGPU_SYNC_NAME_LENGTH];
		                               /*!< Label or name given to the sync resource */
	} sSyncAlloc;
} MUSA_HWPERF_HOST_ALLOC_DETAIL;

typedef struct
{
	MUSA_HWPERF_HOST_RESOURCE_TYPE ui32AllocType;
	                                 /*!< This describes the type of the resource
	                                  allocated in the driver. See
	                                  MUSA_HWPERF_HOST_RESOURCE_TYPE */
	MUSA_HWPERF_HOST_ALLOC_DETAIL MUSAFW_ALIGN uAllocDetail;
	                                 /*!< Union of structures providing further
	                                  data regarding the resource allocated.
	                                  Size of data varies with union member that
	                                  is present, check ``ui32AllocType`` value
	                                  to decode */
} MUSA_HWPERF_HOST_ALLOC_DATA;

/* Payload size must be multiple of 8 bytes to align start of next packet. */
static_assert((sizeof(MUSA_HWPERF_HOST_ALLOC_DATA) & (MTGPUTL_PACKET_ALIGNMENT-1U)) == 0U,
			  "sizeof(MUSA_HWPERF_HOST_ALLOC_DATA) must be a multiple MTGPUTL_PACKET_ALIGNMENT");

typedef union
{
	/*! Data for TYPE_TIMELINE (*Deprecated*) */
	struct
	{
		MT_UINT32 uiPid;             /*!< Identifier of owning process */
		MT_UINT64 ui64Timeline_UID1; /*!< Unique identifier for the timeline resource */
		MT_UINT32 ui32Padding;       /*!< Reserved. Align structure size to 8 bytes */
	} sTimelineDestroy;

	/*! Data for TYPE_FENCE_MT */
	struct
	{
		MT_UINT64 ui64Fence_UID;     /*!< Unique identifier for the fence resource */
		MT_UINT32 ui32Padding;       /*!< Reserved. */
	} sFenceDestroy;

	/*! Data for TYPE_SYNC_CP */
	struct
	{
		MT_UINT32 ui32CheckPt_FWAddr;  /*!< Unique identifier for the check point resource */
	} sSyncCheckPointFree;

	/*! Data for TYPE_SYNC */
	struct
	{
		MT_UINT32 ui32FWAddr;        /*!< Unique identifier for the sync resource */
	} sSyncFree;
} MUSA_HWPERF_HOST_FREE_DETAIL;

typedef struct
{
	MUSA_HWPERF_HOST_RESOURCE_TYPE ui32FreeType;
	                              /*!< This describes the type of the resource
	                               freed or released by the driver. See
	                               MUSA_HWPERF_HOST_RESOURCE_TYPE */
	MUSA_HWPERF_HOST_FREE_DETAIL uFreeDetail;
	                              /*!< Union of structures providing further data
	                               regarding the resource freed. Size of data
	                               varies with union member that is present,
	                               check ``ui32FreeType`` value to decode */
	MT_UINT32 ui32Padding;       /*!< Reserved. Align structure size to 8 bytes */
} MUSA_HWPERF_HOST_FREE_DATA;

/* Payload size must be multiple of 8 bytes to align start of next packet. */
static_assert((sizeof(MUSA_HWPERF_HOST_FREE_DATA) & (MTGPUTL_PACKET_ALIGNMENT-1U)) == 0U,
			  "sizeof(MUSA_HWPERF_HOST_FREE_DATA) must be a multiple MTGPUTL_PACKET_ALIGNMENT");

typedef struct
{
	MT_UINT64 ui64CRTimestamp;   /*!< CR timer value from the latest entry of
	                               the time domains correlation table */
	MT_UINT64 ui64OSTimestamp;   /*!< OS timestamp from the latest entry of the
	                               time domains correlation table */
	MT_UINT32 ui32ClockSpeed;    /*!< GPU clock speed from the latest entry of
	                               the time domains correlation table */
	MT_UINT32 ui32Padding;       /*!< Reserved, align structure size to 8 bytes */
} MUSA_HWPERF_HOST_CLK_SYNC_DATA;

/* Payload size must be multiple of 8 bytes to align start of next packet. */
static_assert((sizeof(MUSA_HWPERF_HOST_CLK_SYNC_DATA) & (MTGPUTL_PACKET_ALIGNMENT-1U)) == 0U,
			  "sizeof(MUSA_HWPERF_HOST_CLK_SYNC_DATA) must be a multiple MTGPUTL_PACKET_ALIGNMENT");

typedef union
{
	/*! Data for TYPE_FENCE_MT */
	struct
	{
		MT_UINT64 ui64NewFence_UID;  /*!< Unique identifier for the new merged fence
		                               resource that has been created */
		MT_UINT64 ui64InFence1_UID;  /*!< Unique identifier for the fence resource */
		MT_UINT64 ui64InFence2_UID;  /*!< Unique identifier of the check point backing
		                               the fence on the GPU */
		MT_CHAR acName[MTGPU_SYNC_NAME_LENGTH];
		                              /*!< Label or name given to the sync resource */
		MT_UINT32 ui32Padding;       /*!< Reserved. Align structure size to 8 bytes */
	} sFenceMerge;
} MUSA_HWPERF_HOST_MODIFY_DETAIL;

typedef struct
{
	MUSA_HWPERF_HOST_RESOURCE_TYPE ui32ModifyType;
	                                /*!< Describes the type of the resource
	                                 modified by the driver. See
	                                 MUSA_HWPERF_HOST_RESOURCE_TYPE */

	MUSA_HWPERF_HOST_MODIFY_DETAIL uModifyDetail;
	                                /*!< Union of structures providing further
	                                 data regarding the resource modified.
	                                 Size of data varies with union member that
	                                 is present.
	                                 Check ``uiModifyType`` value to decode */
} MUSA_HWPERF_HOST_MODIFY_DATA;

/* Payload size must be multiple of 8 bytes to align start of next packet. */
static_assert((sizeof(MUSA_HWPERF_HOST_MODIFY_DATA) & (MTGPUTL_PACKET_ALIGNMENT-1U)) == 0U,
			  "sizeof(MUSA_HWPERF_HOST_MODIFY_DATA) must be a multiple MTGPUTL_PACKET_ALIGNMENT");

typedef enum
{
	MUSA_HWPERF_HOST_DEVICE_HEALTH_STATUS_UNDEFINED = 0, /*!< Invalid */
	MUSA_HWPERF_HOST_DEVICE_HEALTH_STATUS_OK,            /*!< Device OK */
	MUSA_HWPERF_HOST_DEVICE_HEALTH_STATUS_RESPONDING,    /*!< Device responding to requests */
	MUSA_HWPERF_HOST_DEVICE_HEALTH_STATUS_DEAD,          /*!< Device not responding */
	MUSA_HWPERF_HOST_DEVICE_HEALTH_STATUS_FAULT,         /*!< Device has faulted */

	MUSA_HWPERF_HOST_DEVICE_HEALTH_STATUS_LAST
} MUSA_HWPERF_HOST_DEVICE_HEALTH_STATUS;

typedef enum
{
	MUSA_HWPERF_HOST_DEVICE_HEALTH_REASON_UNDEFINED = 0,     /*!< Invalid */
	MUSA_HWPERF_HOST_DEVICE_HEALTH_REASON_NONE,              /*!< No underlying health reason. */
	MUSA_HWPERF_HOST_DEVICE_HEALTH_REASON_ASSERTED,          /*!< Device has asserted. */
	MUSA_HWPERF_HOST_DEVICE_HEALTH_REASON_POLL_FAILING,      /*!< Device poll has failed. */
	MUSA_HWPERF_HOST_DEVICE_HEALTH_REASON_TIMEOUTS,          /*!< Device timeout has fired. */
	MUSA_HWPERF_HOST_DEVICE_HEALTH_REASON_QUEUE_CORRUPT,     /*!< Queue has become corrupt. */
	MUSA_HWPERF_HOST_DEVICE_HEALTH_REASON_QUEUE_STALLED,     /*!< Queue has stalled. */
	MUSA_HWPERF_HOST_DEVICE_HEALTH_REASON_IDLING,            /*!< Device is idling. */
	MUSA_HWPERF_HOST_DEVICE_HEALTH_REASON_RESTARTING,        /*!< Device restarting. */
	MUSA_HWPERF_HOST_DEVICE_HEALTH_REASON_MISSING_INTERRUPTS,/*!< Interrupts have been discarded. */

	MUSA_HWPERF_HOST_DEVICE_HEALTH_REASON_LAST
} MUSA_HWPERF_HOST_DEVICE_HEALTH_REASON;

/*! MUSA_HWPERF_DEV_INFO_EV values */
typedef enum
{
	MUSA_HWPERF_DEV_INFO_EV_HEALTH,      /*!< Health sub-event */

	MUSA_HWPERF_DEV_INFO_EV_LAST         /*!< Last enumeration value */
} MUSA_HWPERF_DEV_INFO_EV;

/*! MUSA_HWPERF_HOST_DEV_INFO_DETAIL is a union of structures providing
 *  further data regarding the device's status
 */
typedef union
{
	/*! Data for device status event */
	struct
	{
		MUSA_HWPERF_HOST_DEVICE_HEALTH_STATUS eDeviceHealthStatus;
		                                 /*!< Device's health status */
		MUSA_HWPERF_HOST_DEVICE_HEALTH_REASON eDeviceHealthReason;
		                                 /*!< Reason for device's health status */
	} sDeviceStatus;
} MUSA_HWPERF_HOST_DEV_INFO_DETAIL;

/*! MUSA_HWPERF_HOST_DEV_INFO_DATA contains device health status information */
typedef struct
{
	MT_UINT32                      ui32Padding;
	                            /*!< Reserved. Align structure size to 8 bytes */
	MUSA_HWPERF_DEV_INFO_EV          eEvType;
	                            /*!< Type of the sub-event. See
	                              MUSA_HWPERF_DEV_INFO_EV */
	MUSA_HWPERF_HOST_DEV_INFO_DETAIL uDevInfoDetail;
	                            /*!< Union of structures providing further data
	                              regarding the device's status. Size of data
	                              varies with union member that is present,
	                              check ``eEvType`` value to decode */
} MUSA_HWPERF_HOST_DEV_INFO_DATA;

/* Payload size must be multiple of 8 bytes to align start of next packet. */
static_assert((sizeof(MUSA_HWPERF_HOST_DEV_INFO_DATA) & (MTGPUTL_PACKET_ALIGNMENT-1U)) == 0U,
			 "sizeof(MUSA_HWPERF_HOST_DEV_INFO_DATA) must be a multiple MTGPUTL_PACKET_ALIGNMENT");

/*! MUSA_HWPERF_INFO_EV event subtype for MUSA_HWPERF_HOST_INFO_DATA events */
typedef enum
{
	MUSA_HWPERF_INFO_EV_MEM_USAGE,     /*!< Memory usage event */
	MUSA_HWPERF_INFO_EV_LAST           /*!< End of enumeration */
} MUSA_HWPERF_INFO_EV;

/*! MUSA_HWPERF_HOST_INFO_DETAIL contains the data payload for the
 * MUSA_HWPERF_HOST_INFO_DATA event.
 */
typedef union
{
	/*! Host Memory usage statistics */
	struct
	{
		MT_UINT32 ui32TotalMemoryUsage;   /*!< Total memory usage */
		/*! Detailed memory usage */
		struct
		{
			MT_UINT32 ui32Pid;              /*!< Process ID */
			MT_UINT32 ui32KernelMemUsage;   /*!< Kernel memory usage */
			MT_UINT32 ui32GraphicsMemUsage; /*!< GPU memory usage */
		} sPerProcessUsage[MUSA_HWPERF_ZERO_OR_MORE_ELEMENTS];
	} sMemUsageStats;
} MUSA_HWPERF_HOST_INFO_DETAIL;

/*! MUSA_HWPERF_HOST_INFO_DATA. Host Info data event payload contains device
 * memory usage information.
 */
typedef struct
{
	MT_UINT32 ui32Padding;       /*!< Reserved. Align structure size to 8 bytes */
	MUSA_HWPERF_INFO_EV eEvType;   /*!< Type of subevent. See MUSA_HWPERF_INFO_EV */
	MUSA_HWPERF_HOST_INFO_DETAIL uInfoDetail;
	                              /*!< Union of structures providing further data
	                               regarding memory usage. Size varies with union
	                               member that is present, check ``eEvType``
	                               value to decode */
} MUSA_HWPERF_HOST_INFO_DATA;

/* Payload size must be multiple of 8 bytes to align start of next packet. */
static_assert((sizeof(MUSA_HWPERF_HOST_INFO_DATA) & (MTGPUTL_PACKET_ALIGNMENT-1U)) == 0U,
			  "sizeof(MUSA_HWPERF_HOST_INFO_DATA) must be a multiple MTGPUTL_PACKET_ALIGNMENT");

/*! FENCE_WAIT_TYPE definitions */
typedef enum
{
	MUSA_HWPERF_HOST_SYNC_FENCE_WAIT_TYPE_BEGIN = 0,    /*!< Begin */
	MUSA_HWPERF_HOST_SYNC_FENCE_WAIT_TYPE_END,          /*!< End */

	MUSA_HWPERF_HOST_SYNC_FENCE_WAIT_TYPE_LAST,         /*!< Do not use */
} MUSA_HWPERF_HOST_SYNC_FENCE_WAIT_TYPE;

/*! FENCE_WAIT_RESULT definitions */
typedef enum
{
	MUSA_HWPERF_HOST_SYNC_FENCE_WAIT_RESULT_INVALID = 0, /*!< Invalid */
	MUSA_HWPERF_HOST_SYNC_FENCE_WAIT_RESULT_TIMEOUT,     /*!< Timed Out */
	MUSA_HWPERF_HOST_SYNC_FENCE_WAIT_RESULT_PASSED,      /*!< Passed */
	MUSA_HWPERF_HOST_SYNC_FENCE_WAIT_RESULT_ERROR,       /*!< Errored */

	MUSA_HWPERF_HOST_SYNC_FENCE_WAIT_RESULT_LAST,        /*!< Do not use */
} MUSA_HWPERF_HOST_SYNC_FENCE_WAIT_RESULT;

/*! FENCE_WAIT_DETAIL Event Payload */
typedef union
{
/*! Data for SYNC_FENCE_WAIT_TYPE_BEGIN */
	struct
	{
		MT_UINT32 ui32TimeoutInMs;                     /*!< Wait timeout (ms) */
	} sBegin;

	/*! Data for SYNC_FENCE_WAIT_TYPE_END */
	struct
	{
		MUSA_HWPERF_HOST_SYNC_FENCE_WAIT_RESULT eResult; /*!< Wait result */
	} sEnd;
} MUSA_HWPERF_HOST_SYNC_FENCE_WAIT_DETAIL;

/*! MUSA_HWPERF_HOST_SYNC_FENCE_WAIT_DATA Event Payload. This data structure
 * is received whenever the host driver handles a wait for sync event request.
 */
typedef struct
{
	MT_PID uiPID;          /*!< Identifier of the owning process */
	MTGPU_FENCE hFence;    /*!< Unique identifier for the fence resource */
	MUSA_HWPERF_HOST_SYNC_FENCE_WAIT_TYPE eType;
	                        /*!< Type of the subevent, see
	                         MUSA_HWPERF_HOST_SYNC_FENCE_WAIT_TYPE */
	MUSA_HWPERF_HOST_SYNC_FENCE_WAIT_DETAIL uDetail;
	                        /*!< Union of structures providing further data
	                         regarding device's status. Size of data varies with
	                         union member that is present, check ``eType`` value
	                         to decode */

} MUSA_HWPERF_HOST_SYNC_FENCE_WAIT_DATA;

static_assert((sizeof(MUSA_HWPERF_HOST_SYNC_FENCE_WAIT_DATA) & (MTGPUTL_PACKET_ALIGNMENT-1U)) == 0U,
			  "sizeof(MUSA_HWPERF_HOST_SYNC_FENCE_WAIT_DATA) must be a multiple MTGPUTL_PACKET_ALIGNMENT");

/*! MUSA_HWPERF_HOST_SYNC_SW_TL_ADV_DATA.
 * Software Timeline Advanced Event Payload. This data structure is received
 * whenever the host driver processes a Software Timeline Advanced event.
 */
typedef struct
{
	MT_PID uiPID;                /*!< Identifier of the owning process */
	MTGPU_TIMELINE hTimeline;    /*!< Unique identifier for the timeline resource */
	MT_UINT64 ui64SyncPtIndex;   /*!< Index of the sync point to which the
	                               timeline has advanced */

} MUSA_HWPERF_HOST_SYNC_SW_TL_ADV_DATA;

static_assert((sizeof(MUSA_HWPERF_HOST_SYNC_SW_TL_ADV_DATA) & (MTGPUTL_PACKET_ALIGNMENT-1U)) == 0U,
			  "sizeof(MUSA_HWPERF_HOST_SYNC_SW_TL_ADV_DATA) must be a multiple MTGPUTL_PACKET_ALIGNMENT");

typedef enum
{
	MUSA_HWPERF_HOST_CLIENT_INFO_TYPE_INVALID = 0,  /*!< Invalid */
	MUSA_HWPERF_HOST_CLIENT_INFO_TYPE_PROCESS_NAME, /*!< Process Name */

	MUSA_HWPERF_HOST_CLIENT_INFO_TYPE_LAST,         /*!< Do not use */
} MUSA_HWPERF_HOST_CLIENT_INFO_TYPE;

typedef struct
{
	MT_PID uiClientPID; /*!< Client process identifier */
	MT_UINT32 ui32Length;  /*!< Number of bytes present in ``acName`` */
	MT_CHAR acName[MUSA_HWPERF_ONE_OR_MORE_ELEMENTS]; /*!< Process name string, null terminated */
} MUSA_HWPERF_HOST_CLIENT_PROC_NAME;

#define MUSA_HWPERF_HOST_CLIENT_PROC_NAME_SIZE(ui32NameLen) \
	((MT_UINT32)(offsetof(MUSA_HWPERF_HOST_CLIENT_PROC_NAME, acName) + (ui32NameLen)))

typedef union
{
	struct
	{
		MT_UINT32 ui32Count; /*!< Number of elements in ``asProcNames`` */
		MUSA_HWPERF_HOST_CLIENT_PROC_NAME asProcNames[MUSA_HWPERF_ONE_OR_MORE_ELEMENTS];
	} sProcName;
} MUSA_HWPERF_HOST_CLIENT_INFO_DETAIL;

typedef struct
{
	MT_UINT32 uiReserved1; /*!< Reserved. Align structure size to 8 bytes */
	MUSA_HWPERF_HOST_CLIENT_INFO_TYPE eType;
	                        /*!< Type of the subevent, see
	                         MUSA_HWPERF_HOST_CLIENT_INFO_TYPE */
	MUSA_HWPERF_HOST_CLIENT_INFO_DETAIL uDetail;
	                        /*!< Union of structures. Size of data
	                         varies with union member that is present,
	                         check ``eType`` value to decode */

} MUSA_HWPERF_HOST_CLIENT_INFO_DATA;

#define MUSA_HWPERF_HOST_TIME_CORRELATION_MAX_COUNT 64U
typedef struct
{
	MT_UINT64 ui64OSTimeStamp;
	MT_UINT64 ui64CRTimeStamp;

	/* Utility variable used to convert CR timer deltas to OS timer deltas (nS),
	 * where the deltas are relative to the timestamps above:
	 * deltaOS = (deltaCR * K) >> decimal_shift, see full explanation below */
	MT_UINT64 ui64CRDeltaToOSDeltaKNs;

	MT_UINT32 ui32CoreClockSpeed;
	MT_UINT32 ui32Reserved;
} MUSA_HWPERF_HOST_TIME_CORRELATION_DETAIL;

typedef struct 
{
	MT_UINT16 ui16Count; /*!< Number of elements in ``asTimeCorrelation`` */
	MT_UINT16 ui16StartIndex; /*!< Start Index */
	MT_UINT16 ui16HostIndex; /*!< Host time correlation sequence count */
	MT_UINT16 ui16Padding;        /*!< Unused, reserved */
	MUSA_HWPERF_HOST_TIME_CORRELATION_DETAIL asTimeCorrelation[MUSA_HWPERF_ONE_OR_MORE_ELEMENTS];
} MUSA_HWPERF_HOST_TIME_CORRELATION_DATA;

static_assert((sizeof(MUSA_HWPERF_HOST_CLIENT_INFO_DATA) & (MTGPUTL_PACKET_ALIGNMENT-1U)) == 0U,
			  "sizeof(MUSA_HWPERF_HOST_CLIENT_INFO_DATA) must be a multiple MTGPUTL_PACKET_ALIGNMENT");
#define MUSA_HWPERF_HOST_DEBUG_VALUES_LENGTH 8
typedef struct
{
	MT_UINT32 ui32PID;
	MT_UINT32 ui32TID;
	MT_UINT32 ui32ValuesSize;
	MT_UINT32 ui32NameSize;
	MT_UINT64 aui64Values[MUSA_HWPERF_HOST_DEBUG_VALUES_LENGTH];
	MT_CHAR acFuncName[MUSA_HWPERF_ONE_OR_MORE_ELEMENTS];
} MUSA_HWPERF_HOST_DEBUG_DATA;

typedef enum
{
	MUSA_HWPERF_HOST_DMA_TYPE_INVALID = 0,  /*!< Invalid */
	MUSA_HWPERF_HOST_DMA_TYPE_USER,         /*!< DMA transfer request from host to device or from device to host */
	MUSA_HWPERF_HOST_DMA_TYPE_P2P,          /*!< DMA transfer from device to another device */
	MUSA_HWPERF_HOST_DMA_TYPE_LAST,         /*!< Do not use */
} MUSA_HWPERF_HOST_DMA_TYPE;

typedef union
{
	struct MUSA_HWPERF_HOST_DMA_USER_DATA
	{
		MT_UINT32 uiReserved1; /*!< Reserved. Align structure size to 8 bytes */
		MT_BOOL bS2D;          /*!< Is copy from host to device */
		MT_UINT64 ui64Addr;    /*!< Host address */
		MT_UINT64 ui64DevAddr; /*!< Device address */
	} sUserData;

	struct MUSA_HWPERF_HOST_DMA_P2P_DATA
	{
		MT_INT32 i32PeerDevID;      /*!< The identifier of the peer device */
		MT_BOOL bPeer2Local;        /*!< Is copy from peer device to local device */
		MT_UINT64 ui64LocalDevAddr; /*!< Local device address */
		MT_UINT64 ui64PeerDevAddr;  /*!< Peer device address */
	} sP2PData;
} MUSA_HWPERF_HOST_DMA_DETAIL;

typedef struct
{
	MUSA_HWPERF_HOST_DMA_TYPE eType;     /*!< Type of the DMA transfer */
	MUSA_HWPERF_HOST_DMA_DETAIL eDetail; /*!< Union of structures. Size of data
	                                         varies with union member that is present,
	                                         check ``eType`` value to decode */
	MT_PID uiPID;                      /*!< Identifier of the owning process */
	MT_PID uiTID;                      /*!< Identifier of the owning thread */
	MT_DEVMEM_SIZE_T uiSize;           /*!< Copy size */
	MTGPU_TIMELINE hUpdateTimeline;    /*!< Unique identifier for the update timeline resource */
	MT_UINT32 ui32ExtJobRef;           /*!< Reference used by callers of the MUSA API to track
	                                         submitted work (for debugging / trace purposes) */
} MUSA_HWPERF_HOST_DMA_DATA;

typedef enum
{
	MUSA_HWPERF_RESOURCE_CAPTURE_TYPE_NONE,
	MUSA_HWPERF_RESOURCE_CAPTURE_TYPE_DEFAULT_FRAMEBUFFER,
	MUSA_HWPERF_RESOURCE_CAPTURE_TYPE_OFFSCREEN_FB_ATTACHMENTS,
	MUSA_HWPERF_RESOURCE_CAPTURE_TYPE_TILE_LIFETIME_DATA,

	MUSA_HWPERF_RESOURCE_TYPE_COUNT
} MUSA_HWPERF_RESOURCE_CAPTURE_TYPE;

typedef struct
{
	MT_UINT32 ui32Height;
	MT_UINT32 ui32Width;
	MT_UINT32 ui32BPP;
	MT_UINT32 ui32PixFormat;
} MUSA_RESOURCE_PER_SURFACE_INFO, *PMUSA_RESOURCE_PER_SURFACE_INFO;

typedef struct
{
	MT_INT32  i32XOffset;        /*!< render surface X shift */
	MT_INT32  i32YOffset;        /*!< render surface Y shift */
	MT_UINT32 ui32WidthInTiles;  /*!< number of TLT data points in X */
	MT_UINT32 ui32HeightInTiles; /*!< number of TLT data points in Y */
} MUSA_RESOURCE_PER_TLT_BUFFER_INFO, *PMUSA_RESOURCE_PER_TLT_BUFFER_INFO;

typedef union
{
	struct MUSA_RESOURCE_CAPTURE_RENDER_SURFACES
	{
		MT_UINT32 ui32RenderSurfaceCount;
		MUSA_RESOURCE_PER_SURFACE_INFO sSurface[MUSA_HWPERF_ONE_OR_MORE_ELEMENTS];
	} sRenderSurfaces;

	struct MUSA_RESOURCE_CAPTURE_TILE_LIFETIME_BUFFERS
	{
		MUSA_RESOURCE_PER_TLT_BUFFER_INFO sTLTBufInfo[MUSA_HWPERF_ONE_OR_MORE_ELEMENTS];
	} sTLTBuffers;
} MUSA_RESOURCE_CAPTURE_DETAIL;

typedef struct
{
	MUSA_HWPERF_RESOURCE_CAPTURE_TYPE eType;
	MT_PID uPID;
	MT_UINT32 ui32ContextID;
	MT_UINT32 ui32FrameNum;
	MT_UINT32 ui32CapturedTaskJobRef;	/* The job ref of the HW task that emitted the data */
	MT_INT32 eClientModule;			/* MUSA_HWPERF_CLIENT_API - ID that the capture is originating from. */
	MUSA_RESOURCE_CAPTURE_DETAIL uDetail; /* eType determines the value of the union */
} MUSA_RESOURCE_CAPTURE_INFO, *PMUSA_RESOURCE_CAPTURE_INFO;

#define MUSA_RESOURCE_CAPTURE_INFO_BASE_SIZE() offsetof(MUSA_RESOURCE_CAPTURE_INFO, uDetail)

/*! Tile Lifetime Tracking header size. Only available if
 * MUSA_FEATURE_ISP_TILE_LIFETIME_TRACKING is present and enabled via
 * SUPPORT_TLT_PERF
 */
#define MUSA_TLT_HARDWARE_HDR_SIZE   (16U)

/* MTGPUGetHWPerfResourceCaptureResult */
typedef enum
{
	MUSA_HWPERF_RESOURCE_CAPTURE_RESULT_NONE = 0,
	MUSA_HWPERF_RESOURCE_CAPTURE_RESULT_OK,					/* We got data ok, expect more packets for this request. */
	MUSA_HWPERF_RESOURCE_CAPTURE_RESULT_NOT_READY,			/* Signals a timeout on the connection - no data available yet. */
	MUSA_HWPERF_RESOURCE_CAPTURE_RESULT_COMPLETE_SUCCESS,	/* The request completed successfully, signals the end of packets for the request. */
	MUSA_HWPERF_RESOURCE_CAPTURE_RESULT_COMPLETE_FAILURE		/* The request failed, signals the end of packets for the request. */
} MUSA_HWPERF_RESOURCE_CAPTURE_RESULT_STATUS;

typedef struct
{
	MT_PID uPID;						/* In case of a failed request pass the caller the PID and context ID. */
	MT_UINT32 ui32CtxID;
	MUSA_RESOURCE_CAPTURE_INFO *psInfo;	/* Various meta-data regarding the captured resource which aid the requester when,
											unpacking the resource data, valid if MUSA_HWPERF_RESOURCE_CAPTURE_RESULT_OK is returned. */
	MT_BYTE *pbData;					/* Buffer containing the captured resource data, valid if MUSA_HWPERF_RESOURCE_CAPTURE_RESULT_OK is returned. */
} MUSA_RESOURCE_CAPTURE_RESULT;

/*! This type is a union of packet payload data structures associated with
 * various FW and Host events */
typedef union
{
	MUSA_HWPERF_FW_DATA             sFW;           /*!< Firmware event packet data, events ``0x01-0x04`` */
	MUSA_HWPERF_FW_DBG_DATA         sFWDBG;        /*!< DBG data, events ``0x05-0x06`` */
	MUSA_HWPERF_HW_DATA             sHW;           /*!< Hardware event packet data,
	                                               events ``0x07-0x19``, ``0x28-0x29``
	                                               See MUSA_HWPERF_HW_DATA */
	MUSA_HWPERF_CLKS_CHG_DATA       sCLKSCHG;      /*!< Clock change event packet
	                                               data, events ``0x1A`` */
	MUSA_HWPERF_GPU_STATE_CHG_DATA  sGPUSTATECHG;  /*!< GPU utilisation state
	                                               change event packet data,
	                                               events ``0x1B`` */
	MUSA_HWPERF_PWR_EST_DATA        sPWREST;       /*!< Power estimate event
	                                               packet data,
	                                               events ``0x20-0x22`` */
	MUSA_HWPERF_PWR_CHG_DATA        sPWR;          /*!< Power event packet data,
	                                               events ``0x23`` */
	MUSA_HWPERF_CSW_DATA            sCSW;          /*!< Context switch packet data,
	                                               events ``0x30-0x31`` */
	MUSA_HWPERF_DVFS_DATA           sDVFS;         /*!< DVFS activity data,
	                                               events ``0x32`` */
	MUSA_HWPERF_UFO_DATA            sUFO;          /*!< UFO data, events ``0x38`` */
	MUSA_HWPERF_FWACT_DATA          sFWACT;        /*!< Firmware activity event
	                                               packet data,
	                                               events ``0x39`` */
	/* */
	MUSA_HWPERF_HOST_ENQ_DATA       sENQ;          /*!< Host ENQ data,
	                                               events ``0x01`` (Host) */
	MUSA_HWPERF_HOST_UFO_DATA       sHUFO;         /*!< Host UFO data,
	                                               events ``0x02`` (Host) */
	MUSA_HWPERF_HOST_ALLOC_DATA     sHALLOC;       /*!< Host Alloc data,
	                                               events ``0x03`` (Host) */
	MUSA_HWPERF_HOST_CLK_SYNC_DATA  sHCLKSYNC;     /*!< Host CLK_SYNC data,
	                                               events ``0x04`` (Host) */
	MUSA_HWPERF_HOST_FREE_DATA      sHFREE;        /*!< Host Free data,
	                                               events ``0x05`` (Host) */
	MUSA_HWPERF_HOST_MODIFY_DATA    sHMOD;         /*!< Host Modify data,
	                                               events ``0x06`` (Host) */
	MUSA_HWPERF_HOST_DEV_INFO_DATA  sHDEVINFO;     /*!< Host device info data,
	                                               events ``0x07`` (Host) */
	MUSA_HWPERF_HOST_INFO_DATA      sHINFO;        /*!< Host info data,
	                                               events ``0x08`` (Host) */
	MUSA_HWPERF_HOST_SYNC_FENCE_WAIT_DATA sWAIT;   /*!< Host fence-wait data,
	                                               events ``0x09`` (Host) */
	MUSA_HWPERF_HOST_SYNC_SW_TL_ADV_DATA sSWTLADV; /*!< Host SW-timeline advance
	                                               data, events ``0x0A`` (Host) */
	MUSA_HWPERF_HOST_CLIENT_INFO_DATA sHClientInfo; /*!< Host client info,
	                                               events ``0x0B`` (Host) */
	MUSA_HWPERF_HOST_TIME_CORRELATION_DATA sHTimeCorrelation; /*!< Time correlation
	                                               events ``0x0C`` (Host) */
	MUSA_HWPERF_HOST_DMA_DATA sHDMA;
	MUSA_HWPERF_HOST_NOTIFY_DATA sHNotify;         /*!< Host Notify data,
	                                               events ``0x0F`` (Host) */
	MUSA_HWPERF_HOST_DEBUG_DATA sHDebug;
} MUSA_HWPERF_V2_PACKET_DATA, *MUSA_PHWPERF_V2_PACKET_DATA;

MUSA_FW_STRUCT_SIZE_ASSERT(MUSA_HWPERF_V2_PACKET_DATA);

#define MUSA_HWPERF_GET_PACKET_DATA(_packet_addr) ((MUSA_PHWPERF_V2_PACKET_DATA) (MT_OFFSET_ADDR((_packet_addr), sizeof(MUSA_HWPERF_V2_PACKET_HDR))))

#define MUSA_HWPERF_GET_DVFS_EVENT_TYPE_PTR(_packet_addr)	\
	((MUSA_HWPERF_DVFS_EV*) (MT_OFFSET_ADDR((_packet_addr), sizeof(MUSA_HWPERF_V2_PACKET_HDR) + offsetof(MUSA_HWPERF_DVFS_DATA,eEventType))))

/******************************************************************************
 * API Types
 *****************************************************************************/

/*! Counter block IDs for all the hardware blocks with counters.
 * Directly addressable blocks must have a value between 0..15 [0..0xF].
 * Indirect groups have following encoding:
 * First hex digit (LSB) represents a unit number within the group
 * and the second hex digit represents the group number.
 * Group 0 is the direct group, all others are indirect groups.
 */
typedef MT_UINT32 MUSA_HWPERF_CNTBLK_ID;

/*! Directly addressable non bank-switched counter blocks */
#define MUSA_CNTBLK_ID_DMIC          0x0000U
#define MUSA_CNTBLK_ID_L2            0x0001U	/*!< L2-specific counter control */
#define MUSA_CNTBLK_ID_FBCDC          0x0002U
#define MUSA_CNTBLK_ID_FW_CUSTOM      0x0003U	/*!< Custom FW provided counters */

/*! Directly addressable L2 counter blocks - presence depends on GPU. */
#define MUSA_CNTBLK_ID_L2BANK0       0x0004U	/*!< L2BANK0 counter control */
#define MUSA_CNTBLK_ID_L2BANK1       0x0005U	/*!< L2BANK1 counter control */
#define MUSA_CNTBLK_ID_L2BANK2       0x0006U	/*!< L2BANK2 counter control */
#define MUSA_CNTBLK_ID_L2BANK3       0x0007U	/*!< L2BANK3 counter control */
#define MUSA_CNTBLK_ID_L2BANK_ALL    0x4004U	/*!< L2 ALL block ID */

#define MUSA_CNTBLK_ID_DIRECT_LAST    0x0008U	/*!< Indirect blocks start from here */

/*! Indirectly addressable counter blocks */
#define MUSA_CNTBLK_ID_ISP0           0x0010U	/*!< ISP 1..N ISP */
#define MUSA_CNTBLK_ID_ISP1           0x0011U
#define MUSA_CNTBLK_ID_ISP2           0x0012U
#define MUSA_CNTBLK_ID_ISP3           0x0013U
#define MUSA_CNTBLK_ID_ISP4           0x0014U
#define MUSA_CNTBLK_ID_ISP5           0x0015U
#define MUSA_CNTBLK_ID_ISP6           0x0016U
#define MUSA_CNTBLK_ID_ISP7           0x0017U
#define MUSA_CNTBLK_ID_ISP_ALL        0x4010U

#define MUSA_CNTBLK_ID_MERCER0        0x0020U	/*!< MERCER 1..N MERCER */
#define MUSA_CNTBLK_ID_MERCER1        0x0021U
#define MUSA_CNTBLK_ID_MERCER2        0x0022U
#define MUSA_CNTBLK_ID_MERCER3        0x0023U
#define MUSA_CNTBLK_ID_MERCER4        0x0024U
#define MUSA_CNTBLK_ID_MERCER5        0x0025U
#define MUSA_CNTBLK_ID_MERCER6        0x0026U
#define MUSA_CNTBLK_ID_MERCER7        0x0027U
#define MUSA_CNTBLK_ID_MERCER_ALL     0x4020U

#define MUSA_CNTBLK_ID_PBE0           0x0030U	/*!< PBE 1..N PBE_PER_MPX x N MPX */
#define MUSA_CNTBLK_ID_PBE1           0x0031U
#define MUSA_CNTBLK_ID_PBE2           0x0032U
#define MUSA_CNTBLK_ID_PBE3           0x0033U
#define MUSA_CNTBLK_ID_PBE4           0x0034U
#define MUSA_CNTBLK_ID_PBE5           0x0035U
#define MUSA_CNTBLK_ID_PBE6           0x0036U
#define MUSA_CNTBLK_ID_PBE7           0x0037U
#define MUSA_CNTBLK_ID_PBE_ALL        0x4030U

#define MUSA_CNTBLK_ID_PBE_SHARED0    0x0040U	/*!< PBE_SHARED 1..N MPX */
#define MUSA_CNTBLK_ID_PBE_SHARED1    0x0041U
#define MUSA_CNTBLK_ID_PBE_SHARED2    0x0042U
#define MUSA_CNTBLK_ID_PBE_SHARED3    0x0043U
#define MUSA_CNTBLK_ID_PBE_SHARED_ALL 0x4040U

#define MUSA_CNTBLK_ID_MP0           0x0050U	/*!< MP 1..N MP */
#define MUSA_CNTBLK_ID_MP1           0x0051U
#define MUSA_CNTBLK_ID_MP2           0x0052U
#define MUSA_CNTBLK_ID_MP3           0x0053U
#define MUSA_CNTBLK_ID_MP4           0x0054U
#define MUSA_CNTBLK_ID_MP5           0x0055U
#define MUSA_CNTBLK_ID_MP6           0x0056U
#define MUSA_CNTBLK_ID_MP7           0x0057U
#define MUSA_CNTBLK_ID_MP_ALL        0x4050U

#define MUSA_CNTBLK_ID_TPU0           0x0060U	/*!< TPU 1..N TPU */
#define MUSA_CNTBLK_ID_TPU1           0x0061U
#define MUSA_CNTBLK_ID_TPU2           0x0062U
#define MUSA_CNTBLK_ID_TPU3           0x0063U
#define MUSA_CNTBLK_ID_TPU4           0x0064U
#define MUSA_CNTBLK_ID_TPU5           0x0065U
#define MUSA_CNTBLK_ID_TPU6           0x0066U
#define MUSA_CNTBLK_ID_TPU7           0x0067U
#define MUSA_CNTBLK_ID_TPU_ALL        0x4060U

#define MUSA_CNTBLK_ID_TXU0         0x0070U	/*!< TXU 1..N TXU */
#define MUSA_CNTBLK_ID_TXU1         0x0071U
#define MUSA_CNTBLK_ID_TXU2         0x0072U
#define MUSA_CNTBLK_ID_TXU3         0x0073U
#define MUSA_CNTBLK_ID_TXU4         0x0074U
#define MUSA_CNTBLK_ID_TXU5         0x0075U
#define MUSA_CNTBLK_ID_TXU6         0x0076U
#define MUSA_CNTBLK_ID_TXU7         0x0077U
#define MUSA_CNTBLK_ID_TXU_ALL      0x4070U

#define MUSA_CNTBLK_ID_GFU0         0x0080U	/*!< GFU 1..N GFU */
#define MUSA_CNTBLK_ID_GFU1         0x0081U
#define MUSA_CNTBLK_ID_GFU2         0x0082U
#define MUSA_CNTBLK_ID_GFU3         0x0083U
#define MUSA_CNTBLK_ID_GFU_ALL      0x4080U

#define MUSA_CNTBLK_ID_RAC0           0x0090U	/*!< RAC 1..N RAC */
#define MUSA_CNTBLK_ID_RAC1           0x0091U
#define MUSA_CNTBLK_ID_RAC2           0x0092U
#define MUSA_CNTBLK_ID_RAC3           0x0093U
#define MUSA_CNTBLK_ID_RAC_ALL        0x4090U

#define MUSA_CNTBLK_ID_LAST           0x0094U	/*!< End of RAC block */

/*! Masks for the counter block ID*/
#define MUSA_CNTBLK_ID_UNIT_MASK      (0x000FU)	/*!< Unit within group */
#define MUSA_CNTBLK_ID_GROUP_MASK     (0x00F0U)	/*!< Group value */
#define MUSA_CNTBLK_ID_GROUP_SHIFT    (4U)
#define MUSA_CNTBLK_ID_MC_GPU_MASK    (0x0F00U)	/*!< GPU ID for MC use */
#define MUSA_CNTBLK_ID_MC_GPU_SHIFT   (8U)
#define MUSA_CNTBLK_ID_UNIT_ALL_MASK  (0x4000U)	/*!< Program all units within a group */

static_assert(
	((MUSA_CNTBLK_ID_DIRECT_LAST + ((MUSA_CNTBLK_ID_LAST & MUSA_CNTBLK_ID_GROUP_MASK) >> MUSA_CNTBLK_ID_GROUP_SHIFT)) <= MUSA_HWPERF_MAX_BVNC_BLOCK_LEN),
	"MUSA_HWPERF_MAX_BVNC_BLOCK_LEN insufficient");

#define MUSA_HWPERF_EVENT_MASK_VALUE(e)      (MT_UINT64_C(1) << (e))

/* When adding new counters here, make sure changes are made to musafw_hwperf_fwblk_valid() as well */
#define MUSA_CUSTOM_FW_CNTRS	\
                X(TA_LOCAL_FL_SIZE,    0x0, MUSA_HWPERF_EVENT_MASK_VALUE(MUSA_HWPERF_HW_TAKICK) |         \
                                            MUSA_HWPERF_EVENT_MASK_VALUE(MUSA_HWPERF_HW_PMOOM_TAPAUSE) |  \
                                            MUSA_HWPERF_EVENT_MASK_VALUE(MUSA_HWPERF_HW_PMOOM_TARESUME) | \
                                            MUSA_HWPERF_EVENT_MASK_VALUE(MUSA_HWPERF_HW_TAFINISHED))      \
                                                                                                        \
                X(TA_GLOBAL_FL_SIZE,   0x1, MUSA_HWPERF_EVENT_MASK_VALUE(MUSA_HWPERF_HW_TAKICK) |         \
                                            MUSA_HWPERF_EVENT_MASK_VALUE(MUSA_HWPERF_HW_PMOOM_TAPAUSE) |  \
                                            MUSA_HWPERF_EVENT_MASK_VALUE(MUSA_HWPERF_HW_PMOOM_TARESUME) | \
                                            MUSA_HWPERF_EVENT_MASK_VALUE(MUSA_HWPERF_HW_TAFINISHED))      \
                                                                                                        \
                X(3D_LOCAL_FL_SIZE,    0x2, MUSA_HWPERF_EVENT_MASK_VALUE(MUSA_HWPERF_HW_3DKICK) |         \
                                            MUSA_HWPERF_EVENT_MASK_VALUE(MUSA_HWPERF_HW_3DFINISHED))      \
                                                                                                        \
                X(3D_GLOBAL_FL_SIZE,   0x3, MUSA_HWPERF_EVENT_MASK_VALUE(MUSA_HWPERF_HW_3DKICK) |         \
                                            MUSA_HWPERF_EVENT_MASK_VALUE(MUSA_HWPERF_HW_3DFINISHED))      \
                                                                                                        \
                X(ISP_TILES_IN_FLIGHT, 0x4, MUSA_HWPERF_EVENT_MASK_VALUE(MUSA_HWPERF_HW_3DKICK) |         \
                                            MUSA_HWPERF_EVENT_MASK_VALUE(MUSA_HWPERF_HW_3DSPMKICK))

/*! Counter IDs for the firmware held statistics */
typedef enum
{
#define X(ctr, id, allow_mask)	MUSA_CUSTOM_FW_CNTR_##ctr = id,
	MUSA_CUSTOM_FW_CNTRS
#undef X

	/* always the last entry in the list */
	MUSA_CUSTOM_FW_CNTR_LAST
} MUSA_HWPERF_CUSTOM_FW_CNTR_ID;

/*! Identifier for each counter in a performance counting module */
typedef MT_UINT32 MUSA_HWPERF_CNTBLK_COUNTER_ID;

/* sets all the bits from bit _b1 to _b2, in a MT_UINT64 type */
#define MASK_RANGE_IMPL(b1, b2)	((MT_UINT64)((MT_UINT64_C(1) << ((MT_UINT32)(b2)-(MT_UINT32)(b1) + 1U)) - 1U) << (MT_UINT32)b1)
#define MASK_RANGE(R)			MASK_RANGE_IMPL(R##_FIRST_TYPE, R##_LAST_TYPE)
#define MUSA_HWPERF_HOST_EVENT_MASK_VALUE(e) (MT_UINT32_C(1) << (e))

/*! Mask macros for use with MUSACtrlHWPerf() API.
 */
#define MUSA_HWPERF_EVENT_MASK_NONE          (MT_UINT64_C(0x0000000000000000))
#define MUSA_HWPERF_EVENT_MASK_DEFAULT       MUSA_HWPERF_EVENT_MASK_VALUE(MUSA_HWPERF_FWACT) | \
                                            MUSA_HWPERF_EVENT_MASK_VALUE(MUSA_HWPERF_PWR_CHG) | \
                                            MUSA_HWPERF_EVENT_MASK_VALUE(MUSA_HWPERF_CLKS_CHG)
#define MUSA_HWPERF_EVENT_MASK_ALL           (MT_UINT64_C(0xFFFFFFFFFFFFFFFF))

/*! HWPerf Firmware event masks
 * @par
 * All FW Start/End/Debug (SED) events. */
#define MUSA_HWPERF_EVENT_MASK_FW_SED    (MASK_RANGE(MUSA_HWPERF_FW_EVENT_RANGE))

#define MUSA_HWPERF_EVENT_MASK_FW_UFO    (MUSA_HWPERF_EVENT_MASK_VALUE(MUSA_HWPERF_UFO))
#define MUSA_HWPERF_EVENT_MASK_FW_CSW    (MUSA_HWPERF_EVENT_MASK_VALUE(MUSA_HWPERF_CSW_START) |\
                                          MUSA_HWPERF_EVENT_MASK_VALUE(MUSA_HWPERF_CSW_FINISHED))
/*! All FW events. */
#define MUSA_HWPERF_EVENT_MASK_ALL_FW    (MUSA_HWPERF_EVENT_MASK_FW_SED |\
                                          MUSA_HWPERF_EVENT_MASK_FW_UFO |\
                                          MUSA_HWPERF_EVENT_MASK_FW_CSW)

/*! HW Periodic events (1ms interval). */
#define MUSA_HWPERF_EVENT_MASK_HW_PERIODIC   (MUSA_HWPERF_EVENT_MASK_VALUE(MUSA_HWPERF_HW_PERIODIC))
/*! All HW Kick/Finish events. */
#define MUSA_HWPERF_EVENT_MASK_HW_KICKFINISH ((MASK_RANGE(MUSA_HWPERF_HW_EVENT_RANGE0) |\
                                               MASK_RANGE(MUSA_HWPERF_HW_EVENT_RANGE1)) &\
                                              ~(MUSA_HWPERF_EVENT_MASK_HW_PERIODIC))

#define MUSA_HWPERF_EVENT_MASK_ALL_HW        (MUSA_HWPERF_EVENT_MASK_HW_KICKFINISH |\
                                              MUSA_HWPERF_EVENT_MASK_HW_PERIODIC)

#define MUSA_HWPERF_EVENT_MASK_ALL_PWR_EST   (MASK_RANGE(MUSA_HWPERF_PWR_EST_RANGE))

#define MUSA_HWPERF_EVENT_MASK_ALL_PWR       (MUSA_HWPERF_EVENT_MASK_VALUE(MUSA_HWPERF_CLKS_CHG) |\
                                              MUSA_HWPERF_EVENT_MASK_VALUE(MUSA_HWPERF_GPU_STATE_CHG) |\
                                              MUSA_HWPERF_EVENT_MASK_VALUE(MUSA_HWPERF_PWR_CHG))

/*! HWPerf Host event masks
 */
#define MUSA_HWPERF_EVENT_MASK_HOST_WORK_ENQ  (MUSA_HWPERF_HOST_EVENT_MASK_VALUE(MUSA_HWPERF_HOST_ENQ))
#define MUSA_HWPERF_EVENT_MASK_HOST_ALL_UFO   (MUSA_HWPERF_HOST_EVENT_MASK_VALUE(MUSA_HWPERF_HOST_UFO))
#define MUSA_HWPERF_EVENT_MASK_HOST_ALL_PWR   (MUSA_HWPERF_HOST_EVENT_MASK_VALUE(MUSA_HWPERF_HOST_CLK_SYNC))


/*! Type used in the MUSA API MUSAConfigHWPerfCounters() */
typedef struct
{
	/*! Reserved for future use */
	MT_UINT32 ui32Reserved;

	/*! Counter block ID, see MUSA_HWPERF_CNTBLK_ID */
	MT_UINT16 ui16BlockID;

	/*! Number of configured counters within this block */
	MT_UINT16 ui16NumCounters;

	/*! Counter register values */
	MT_UINT16 ui16Counters[MUSA_CNTBLK_COUNTERS_MAX];

} UNCACHED_ALIGN MUSA_HWPERF_CONFIG_CNTBLK;

MUSA_FW_STRUCT_SIZE_ASSERT(MUSA_HWPERF_CONFIG_CNTBLK);

typedef enum
{
	MUSA_HWPERF_PFM_ARCH_TYPE_QY2,
	MUSA_HWPERF_PFM_ARCH_TYPE_PH1,
} MUSA_HWPERF_PFM_ARCH_TYPE;

typedef struct
{
	MT_BOOL bGetStat;
	MT_UINT64 ui64Offset;
	MT_UINT64 ui64BufSize;
} MUSA_HWPERF_PFM_GET_STAT_CTRL;

typedef struct
{
	MUSA_HWPERF_PFM_GET_STAT_CTRL sGetStatCtrlCMSS;
	MUSA_HWPERF_PFM_GET_STAT_CTRL sGetStatCtrlMSSMMU;
	MUSA_HWPERF_PFM_GET_STAT_CTRL sGetStatCtrlMSSWrap;
	MUSA_HWPERF_PFM_GET_STAT_CTRL sGetStatCtrlGPU;
} MUSA_HWPERF_PFM_GET_STAT;

typedef struct
{
	MT_UINT32 ui32CoreId;
	MT_UINT32 ui32WrapperSelc;
	MT_UINT32 ui32InstanceSelc;
	MT_UINT64 ui64BaseAddrOffset;
	MT_UINT64 ui64PointerAddrOffset;
} MUSA_HWPERF_PFM_INSTANCE_CONFIG_QY2;

typedef struct
{
	MT_UINT32 ui32CoreId;
	MT_UINT32 ui32WrapperSelc;
	MT_UINT32 ui32InstanceEnable;
	MT_UINT64 ui64GroupSelc;
} MUSA_HWPERF_PFM_WRAPPER_CONFIG_QY2;

typedef struct
{
	MT_BOOL bEnablePFM;
	MT_UINT32 ui32TimeInterval;
	MT_UINT32 ui32NumPages;
	MT_UINT32 ui32PointerUpdateFreq;
} MUSA_HWPERF_PFM_GLOBAL_CONFIG_QY2;

#define MUSA_HWPERF_PFM_L2PU_DEST_ADDR_COUNT 12
typedef struct
{
	MT_UINT32 ui32Instance;
	MT_UINT32 ui32LayoutCtrl;
	MT_UINT64 ui64CountBaseAddrOffset;
	MT_UINT64 ui64PointerAddrOffset;
	MT_UINT64 ui64InstanceCtrl;
	MT_UINT64 ui64TriggerCtrl;
	MT_UINT32 aui32Group;
	MT_UINT64 aui64DestAddr[MUSA_HWPERF_PFM_L2PU_DEST_ADDR_COUNT];
} MUSA_HWPERF_PFM_INSTANCE_CONFIG_PH1;

#define MUSA_PFM_MAX_WRAPPER_COUNT_PER_CORE_PH1 8
typedef struct
{
	MT_UINT32 ui32CoreId;
	MT_UINT32 ui32Wrapper;
	MT_UINT32 ui32Count;
	MUSA_HWPERF_PFM_INSTANCE_CONFIG_PH1 apsInstCfgs[MUSA_PFM_MAX_WRAPPER_COUNT_PER_CORE_PH1];
} MUSA_HWPERF_PFM_WRAPPER_CONFIG_PH1;

typedef struct
{
	MT_BOOL bEnablePFM;
	MT_UINT32 ui32BufferCtrl;
	MT_UINT32 ui32MhCtl;
} MUSA_HWPERF_PFM_GLOBAL_CONFIG_PH1;

typedef struct
{
	MT_HANDLE hPmrHandle;
	MT_HANDLE hOSMMapHandle;
	MT_PBYTE pBaseAddr;
	MT_UINT64 ui64Size;
} MUSA_HWPERF_PFM_DESC;

typedef struct
{
	MUSA_HWPERF_PFM_ARCH_TYPE eArchType;
	union
	{
		MUSA_HWPERF_PFM_INSTANCE_CONFIG_QY2 sPFMInstCfgQY2;
		MUSA_HWPERF_PFM_INSTANCE_CONFIG_PH1 sPFMInstCfgPH1;
	} uConfig;
} MUSA_HWPERF_PFM_INSTANCE_CONFIG;

typedef struct
{
	MUSA_HWPERF_PFM_ARCH_TYPE eArchType;
	union
	{
		MUSA_HWPERF_PFM_WRAPPER_CONFIG_QY2 sPFMWrapCfgQY2;
		MUSA_HWPERF_PFM_WRAPPER_CONFIG_PH1 sPFMWrapCfgPH1;
	} uConfig;
} MUSA_HWPERF_PFM_WRAPPER_CONFIG;

typedef struct
{
	MUSA_HWPERF_PFM_ARCH_TYPE eArchType;
	union
	{
		MUSA_HWPERF_PFM_GLOBAL_CONFIG_QY2 sPFMGlobalCfgQY2;
		MUSA_HWPERF_PFM_GLOBAL_CONFIG_PH1 sPFMGlobalCfgPH1;
	} uConfig;
} MUSA_HWPERF_PFM_GLOBAL_CONFIG;

typedef enum
{
        MUSA_HWPERF_PFM_MSS_DDRC,
        MUSA_HWPERF_PFM_MSS_D2D_0,
        MUSA_HWPERF_PFM_MSS_D2D_1,
        MUSA_HWPERF_PFM_MSS_LLC,
} MUSA_HWPERF_PFM_MSS_TYPE_PH1;

typedef struct
{
        MUSA_HWPERF_PFM_MSS_TYPE_PH1 eMSSCfgType;
        MT_UINT32 MSSCfgOffset;
        MT_BOOL bMSSEnable;
        MT_UINT32 ui32TimeSlot;
        MT_UINT32 ui32OrderEnable;
        MT_UINT32 ui32DumpFreq;
        MT_UINT32 ui32DestSpace;
        MT_UINT32 ui32PointSelc;
        MT_UINT32 ui32GroupSelc;
        MT_UINT64 ui64BaseAddrOffset;
        MT_UINT64 ui64PointAddrOffset;
        MT_UINT32 ui32LayoutCtrl;
        MT_UINT32 ui32SpaceOverFlow;
        MT_UINT32 ui32Absolute;
        MT_UINT32 ui32TrigSource;
        MT_UINT32 ui32BroadMode;
        MT_UINT32 ui32RegMode;
        MT_UINT32 ui32RegCommand;
} MUSA_HWPERF_PFM_MSS_CONFIG_PH1;

typedef struct
{
        MUSA_HWPERF_PFM_ARCH_TYPE eArchType;
        MT_UINT32 ui32Count;
        union
        {
                MUSA_HWPERF_PFM_MSS_CONFIG_PH1 *psPFMMSSCfgPH1;
        } uConfig;
} MUSA_HWPERF_PFM_MSS_CONFIG;

typedef struct
{
        MT_UINT32 ui32DumpEnable;
        MT_UINT32 ui32DumpCmd;
        MT_UINT32 ui32PFMCtrl0;
} MUSA_HWPERF_PFM_CMSS_CONFIG_PH1;

typedef enum
{
	MUSA_HWPERF_PFM_DUMP_TRIGGER_GPU,
	MUSA_HWPERF_PFM_DUMP_TRIGGER_CMSS,
	MUSA_HWPERF_PFM_DUMP_TRIGGER_MSS,
} MUSA_HWPERF_PFM_DUMP_TRIGGER_TYPE;

typedef enum
{
	MUSA_HWPERF_PFM_NORM_DUMP_START,
	MUSA_HWPERF_PFM_NORM_DUMP_ONCE,
	MUSA_HWPERF_PFM_NORM_DUMP_END,
	MUSA_HWPERF_PFM_INTV_DUMP_START,
	MUSA_HWPERF_PFM_INTV_DUMP_END,
} MUSA_HWPERF_PFM_DUMP_TRIGGER_CONTROL;

typedef struct
{
	MUSA_HWPERF_PFM_MSS_TYPE_PH1 eMSSCfgType;
	MT_UINT32 MSSCfgOffset;
	MUSA_HWPERF_PFM_DUMP_TRIGGER_CONTROL ePFMDumpCtrl;
} MUSA_HWPERF_PFM_DUMP_TRIG_MSS_PH1;

typedef struct
{
        MUSA_HWPERF_PFM_ARCH_TYPE eArchType;
        union
        {
                MUSA_HWPERF_PFM_DUMP_TRIG_MSS_PH1 sPFMDumpTrigMSSPH1;
        } uConfig;
} MUSA_HWPERF_PFM_DUMP_TRIG_MSS;

typedef struct
{
	MUSA_HWPERF_PFM_DUMP_TRIGGER_TYPE eDumpType;
	union
	{
		MUSA_HWPERF_PFM_DUMP_TRIGGER_CONTROL ePFMDumpCtrlGPU;
		MUSA_HWPERF_PFM_DUMP_TRIGGER_CONTROL ePFMDumpCtrlCMSS;
		MUSA_HWPERF_PFM_DUMP_TRIG_MSS sPFMDumpTrigMSS;
	} uConfig;
} MUSA_HWPERF_PFM_DUMP_TRIGGER_CONFIG;

typedef struct
{
	MUSA_HWPERF_PFM_ARCH_TYPE eArchType;
	MT_BOOL bIsSysMemory;
	MT_UINT64 ui64BufSize;
	MT_UINT32 ui32PfmCMSSCtrl;
} MUSA_HWPERF_PFM_INIT_PARAM;

#if defined(__cplusplus)
}
#endif

#endif /* MUSA_HWPERF_H_ */

/******************************************************************************
 End of file
******************************************************************************/

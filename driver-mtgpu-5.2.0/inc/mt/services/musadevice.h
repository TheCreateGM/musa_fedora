/*************************************************************************/ /*!
@File
@Title          MUSA device node header file
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Header for the MUSA device node
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

#if !defined(MUSADEVICE_H)
#define MUSADEVICE_H

#include "img_types.h"
#include "img_defs.h"
#include "mtgpu_device_types.h"
#include "mmu_common.h"
#include "musa_fwif_km.h"
#include "cache_ops.h"
#include "device.h"
#include "osfunc.h"
#include "musalayer_impl.h"
#if defined(SUPPORT_WORKLOAD_ESTIMATION)
#include "hash.h"
#endif
#include "mtgpu_watchdog.h"
#include "devicemem_next.h"

struct mtgpu_gpu_util_stats;

/*!
 ******************************************************************************
 * Device state flags
 *****************************************************************************/
#define MUSAKM_DEVICE_STATE_ZERO_FREELIST                          (0x1)  /*!< Zeroing the physical pages of reconstructed free lists */
#define MUSAKM_DEVICE_STATE_DISABLE_DW_LOGGING_EN                  (0x2)  /*!< Used to disable the Devices Watchdog logging */
#define MUSAKM_DEVICE_STATE_GPU_UNITS_POWER_CHANGE_EN              (0x4)  /*!< Used for validation to inject MPX power state mask change every DM kick */
#define MUSAKM_DEVICE_STATE_CCB_GROW_EN                            (0x8)  /*!< Used to indicate CCB grow is permitted */
#define MUSAKM_DEVICE_STATE_ENABLE_MPX_UNITS_POWER_MASK_CHANGE_EN  (0x10) /*!< Used for validation to enable MPX power state mask change */
#define MUSAKM_DEVICE_STATE_MASK                                   (0x1F)

/*!
 ******************************************************************************
 * GPU DVFS Table
 *****************************************************************************/

#define MUSA_GPU_DVFS_TABLE_SIZE                      64
#define MUSA_GPU_DVFS_FIRST_CALIBRATION_TIME_US       25000     /* Time required to calibrate a clock frequency the first time */
#define MUSA_GPU_DVFS_TRANSITION_CALIBRATION_TIME_US  150000    /* Time required for a recalibration after a DVFS transition */
#define MUSA_GPU_DVFS_PERIODIC_CALIBRATION_TIME_US    10000000  /* Time before the next periodic calibration and correlation */


/*!
 ******************************************************************************
 * Global flags for driver validation
 *****************************************************************************/
#define MUSA_VAL_LS_EN                             (0x1U)  /*!< Enable dual lockstep firmware */
#define MUSA_VAL_FBDC_SIG_CHECK_NOERR_EN           (0x2U)  /*!< Enable FBDC signature check. Signatures must match */
#define MUSA_VAL_FBDC_SIG_CHECK_ERR_EN             (0x4U)  /*!< Enable FBDC signature check. Signatures must not match */
#define MUSA_VAL_GPUSTATEPIN_EN                    (0x8U)  /*!< Enable GPU state pin check */
#define MUSA_VAL_KZ_SIG_CHECK_NOERR_EN            (0x10U)  /*!< Enable KZ signature check. Signatures must match */
#define MUSA_VAL_KZ_SIG_CHECK_ERR_EN              (0x20U)  /*!< Enable KZ signature check. Signatures must not match */
#define MUSA_VAL_SIG_CHECK_ERR_EN                 (MUSA_VAL_FBDC_SIG_CHECK_ERR_EN)

typedef struct _GPU_FREQ_TRACKING_DATA_
{
	/* Core clock speed estimated by the driver */
	MT_UINT32 ui32EstCoreClockSpeed;

	/* Amount of successful calculations of the estimated core clock speed */
	MT_UINT32 ui32CalibrationCount;
} GPU_FREQ_TRACKING_DATA;

#if defined(MTGPU_TIMER_CORRELATION_HISTORY)
#define MUSA_GPU_FREQ_TRACKING_SIZE 16

typedef struct
{
	MT_UINT64 ui64BeginCRTimestamp;
	MT_UINT64 ui64BeginOSTimestamp;

	MT_UINT64 ui64EndCRTimestamp;
	MT_UINT64 ui64EndOSTimestamp;

	MT_UINT32 ui32EstCoreClockSpeed;
	MT_UINT32 ui32CoreClockSpeed;
} GPU_FREQ_TRACKING_HISTORY;
#endif

typedef struct _MUSA_GPU_DVFS_TABLE_
{
	/* Beginning of current calibration period (in us) */
	MT_UINT64 ui64CalibrationCRTimestamp;
	MT_UINT64 ui64CalibrationOSTimestamp;

	/* Calculated calibration period (in us) */
	MT_UINT64 ui64CalibrationCRTimediff;
	MT_UINT64 ui64CalibrationOSTimediff;

	/* Current calibration period (in us) */
	MT_UINT32 ui32CalibrationPeriod;

	/* System layer frequency table and frequency tracking data */
	MT_UINT32 ui32FreqIndex;
	MT_UINT32 aui32GPUFrequency[MUSA_GPU_DVFS_TABLE_SIZE];
	GPU_FREQ_TRACKING_DATA asTrackingData[MUSA_GPU_DVFS_TABLE_SIZE];

#if defined(MTGPU_TIMER_CORRELATION_HISTORY)
	MT_UINT32 ui32HistoryIndex;
	GPU_FREQ_TRACKING_HISTORY asTrackingHistory[MUSA_GPU_FREQ_TRACKING_SIZE];
#endif
} MUSA_GPU_DVFS_TABLE;


/*!
 ******************************************************************************
 * GPU utilisation statistics
 *****************************************************************************/

typedef struct _MUSAFWIF_GPU_UTIL_STATS_
{
	MT_BOOL   bValid;                /* If TRUE, statistics are valid.
	                                     FALSE if the driver couldn't get reliable stats. */
	MT_UINT64 ui64GpuStatActive;     /* GPU active statistic */
	MT_UINT64 ui64GpuStatBlocked;    /* GPU blocked statistic */
	MT_UINT64 ui64GpuStatIdle;       /* GPU idle statistic */
	MT_UINT64 ui64GpuStatCumulative; /* Sum of active/blocked/idle stats */
	MT_UINT64 ui64TimeStamp;         /* Timestamp of the most recent sample of the GPU stats */
} MUSAFWIF_GPU_UTIL_STATS;


typedef struct _MUSA_REG_CONFIG_
{
	MT_BOOL               bEnabled;
	MT_UINT32             ui32NumRegRecords;
	POS_LOCK               hLock;
} MUSA_REG_CONFIG;

typedef struct _MTGPU_STUB_PBDESC_ MTGPU_STUB_PBDESC;

#if defined(SUPPORT_VALIDATION)
/**
 * Structure containing information for calculating next MPX power domain state.
 */
typedef struct _MUSA_POWER_DOMAIN_STATE_
{
	/**
	 * Total number of power units in the core.
	 */
	MT_UINT32 ui32PowUnitsCount;
	/**
	 * Current power domain state
	 */
	MT_UINT32 ui32CurrentState;
	/**
	 * Stores last transition that happened for each power domain state.
	 */
	MT_UINT32 *paui32LastTransition;
} MUSA_POWER_DOMAIN_STATE;
#endif

typedef struct _MTGPU_DEVICE_FEATURE_CONFIG_
{
	MT_UINT64 ui64ErnsBrns;
	MT_UINT64 ui64Features;
	MT_UINT32 ui32B;
	MT_UINT32 ui32V;
	MT_UINT32 ui32N;
	MT_UINT32 ui32C;
	MT_UINT32 ui32FeaturesValues[MUSA_FEATURE_WITH_VALUES_MAX_IDX];
	MT_UINT32 ui32MAXDMCount;
	MT_UINT32 ui32MAXPowUnitCount;
	MT_UINT32 ui32MAXRACCount;
	MT_UINT32 ui32L2SizeInBytes;
	MT_PCHAR  pszBVNCString;
}MTGPU_DEVICE_FEATURE_CONFIG;

/* This is used to get the value of a specific feature.
 * Note that it will assert if the feature is disabled or value is invalid. */
#define MUSA_GET_FEATURE_VALUE(psDevInfo, Feature) \
			( psDevInfo->sDevFeatureCfg.ui32FeaturesValues[MUSA_FEATURE_##Feature##_IDX] )

/* This is used to check if the feature value (e.g. with an integer value) is available for the currently running BVNC or not */
#define MUSA_IS_FEATURE_VALUE_SUPPORTED(psDevInfo, Feature) \
			( psDevInfo->sDevFeatureCfg.ui32FeaturesValues[MUSA_FEATURE_##Feature##_IDX] < MUSA_FEATURE_VALUE_DISABLED )

/* This is used to check if the Boolean feature (e.g. WITHOUT an integer value) is available for the currently running BVNC or not */
#define MUSA_IS_FEATURE_SUPPORTED(psDevInfo, Feature) \
			BITMASK_HAS(psDevInfo->sDevFeatureCfg.ui64Features, MUSA_FEATURE_##Feature##_BIT_MASK)

/* This is used to check if the ERN is available for the currently running BVNC or not */
#define MUSA_IS_ERN_SUPPORTED(psDevInfo, ERN) \
			BITMASK_HAS(psDevInfo->sDevFeatureCfg.ui64ErnsBrns, HW_ERN_##ERN##_BIT_MASK)

/* This is used to check if the BRN is available for the currently running BVNC or not */
#define MUSA_IS_BRN_SUPPORTED(psDevInfo, BRN) \
			BITMASK_HAS(psDevInfo->sDevFeatureCfg.ui64ErnsBrns, FIX_HW_BRN_##BRN##_BIT_MASK)

/* there is a corresponding define in musaapi.h */
#define MUSA_MAX_TIMER_QUERIES 16U

#if defined(SUPPORT_WORKLOAD_ESTIMATION)
/*!
 * The host maintains a 512-deep cache of submitted workloads per device,
 * i.e. a global look-up table for TA, 3D and compute (depending on the MUSA
 * hardware support present)
 */

/*
 * For the workload estimation return data array, the max amount of commands the
 * MTS can have is 255, therefore 512 (LOG2 = 9) is large enough to account for
 * all corner cases
 */
#define RETURN_DATA_ARRAY_SIZE_LOG2 (9)
#define RETURN_DATA_ARRAY_SIZE      ((1U) << RETURN_DATA_ARRAY_SIZE_LOG2)
#define RETURN_DATA_ARRAY_WRAP_MASK (RETURN_DATA_ARRAY_SIZE - 1)

#define WORKLOAD_HASH_SIZE_LOG2		6
#define WORKLOAD_HASH_SIZE			((1U) << WORKLOAD_HASH_SIZE_LOG2)
#define WORKLOAD_HASH_WRAP_MASK		(WORKLOAD_HASH_SIZE - 1)

/*!
 * Workload characteristics for supported data masters.
 * All characteristics must match for the workload estimate to be used/updated.
 */
typedef union _MUSA_WORKLOAD_
{
	struct
	{
		MT_UINT32				ui32RenderTargetSize;
		MT_UINT32				ui32NumberOfDrawCalls;
		MT_UINT32				ui32NumberOfIndices;
		MT_UINT32				ui32NumberOfMRTs;
	} sGFX;

	struct
	{
		MT_UINT32				ui32NumberOfWorkgroups;
		MT_UINT32				ui32NumberOfWorkitems;
	} sCompute;

	struct
	{
		MT_UINT32				ui32Characteristic1;
		MT_UINT32				ui32Characteristic2;
	} sTransfer;
} MUSA_WORKLOAD;

/*!
 * Host data used to match the return data (actual cycles count) to the
 * submitted command packet.
 * The hash table is a per-DM circular buffer containing a key based on the
 * workload characteristics. On job completion, the oldest workload data
 * is evicted if the CB is full and the driver matches the characteristics
 * to the matching data.
 *
 * o If the driver finds a match the existing cycle estimate is averaged with
 *   the actual cycles used.
 * o Otherwise a new hash entry is created with the actual cycles for this
 *   workload.
 *
 * Subsequently if a match is found during command submission, the estimate
 * is passed to the scheduler, e.g. adjust the GPU frequency if PDVFS is enabled.
 */
typedef struct _WORKLOAD_MATCHING_DATA_
{
	POS_LOCK				psHashLock;
	HASH_TABLE				*psHashTable;		/*! existing workload cycle estimates for this DM */
	MUSA_WORKLOAD			asHashKeys[WORKLOAD_HASH_SIZE];
	MT_UINT64				aui64HashData[WORKLOAD_HASH_SIZE];
	MT_UINT32				ui32HashArrayWO;	/*! track the most recent workload estimates */
} WORKLOAD_MATCHING_DATA;

/*!
 * A generic container for the workload matching data for GPU contexts:
 * rendering (TA, 3D), compute, etc.
 */
typedef struct _WORKEST_HOST_DATA_
{
	union
	{
		struct
		{
			WORKLOAD_MATCHING_DATA	sDataTA;	/*!< matching data for TA commands */
			WORKLOAD_MATCHING_DATA	sData3D;	/*!< matching data for 3D commands */
		} sGFX;

		struct
		{
			WORKLOAD_MATCHING_DATA	sDataCMP;	/*!< matching data for CMP commands */
		} sCompute;

		struct
		{
			WORKLOAD_MATCHING_DATA	sDataTRANSFER;	/*!< matching data for TRANSFER-TQ commands */
		} sTransfer;
	} uWorkloadMatchingData;

	/*
	 * This is a per-context property, hence the TA and 3D share the same
	 * per render context counter.
	 */
	MT_UINT32				ui32WorkEstCCBReceived;	/*!< Used to ensure all submitted work
														 estimation commands are received
														 by the host before clean up. */
} WORKEST_HOST_DATA;

/*!
 * Entries in the list of submitted workloads, used when the completed command
 * returns data to the host.
 *
 * - the matching data is needed as it holds the hash data
 * - the host data is needed for completion updates, ensuring memory is not
 *   freed while workload estimates are in-flight.
 * - the workload characteristic is used in the hash table look-up.
 */
typedef struct _WORKEST_RETURN_DATA_
{
	WORKEST_HOST_DATA		*psWorkEstHostData;
	WORKLOAD_MATCHING_DATA	*psWorkloadMatchingData;
	MUSA_WORKLOAD			sWorkloadCharacteristics;
} WORKEST_RETURN_DATA;
#endif


#define MUSA_MAX_NUM_MMU_PAGE_SIZE_RANGES    4


/*!
 ******************************************************************************
 * MUSA Device error counts
 *****************************************************************************/
typedef struct _MTGPU_MUSADEV_ERROR_COUNTS_
{
	MT_UINT32 ui32WGPErrorCount;		/*!< count of the number of WGP checksum errors */
	MT_UINT32 ui32TRPErrorCount;		/*!< count of the number of TRP checksum errors */
} MTGPU_MUSADEV_ERROR_COUNTS;


typedef struct _PB_DATA_
{
	struct mtgpu_bo *pb_bo;
	struct mtgpu_bo *freelist_bo;
	struct mtgpu_bo *pages_bo;
	void *freelist_backup;
} PB_DATA;

/*!
 ******************************************************************************
 * MUSA Device info
 *****************************************************************************/
typedef struct _MTGPU_MUSADEV_INFO_
{
	MTGPU_DEVICE_NODE		*psDeviceNode;

	MTGPU_DEVICE_FEATURE_CONFIG	sDevFeatureCfg;

	MT_BOOL				bDevInit2Done;

	MT_INT32				ui32MemoryOptimization;

	MT_BOOL				bFirmwareInitialised;
	MT_BOOL				bPDPEnabled;

	MT_HANDLE				hDbgReqNotify;

	/* Kernel mode linear address of device registers */
	void __iomem			*pvRegsBaseKM;

	void __iomem			*pvVirtRegsBaseKM;

	/* lock to protect guest gpa to hpa processing in virtualization */
	POS_LOCK			hGpaToHpaLock;

	void *pvGpaToHpaTree;
	void *pvHpaCache;

#if defined(SUPPORT_SW_OSID_EXTENSION)
	/* 
	 * TODO:
	 * 1. Change callback set_master_kick_reg to a __iomem pointer
	 */
	void (*set_master_kick_reg)(void *master_kick_reg, void *priv_data, u32 mpc_id);
#endif

	MT_HANDLE				hRegMapping;

	/* System physical address of device registers */
	MT_CPU_PHYADDR			sRegsPhysBase;
	/* Register region size in bytes */
	MT_UINT32				ui32RegSize;

	MTGPU_STUB_PBDESC		*psStubPBDescListKM;

	/* Firmware memory context info */
	DEVMEM_CONTEXT			*psKernelDevmemCtx;
	DEVMEM_HEAP				*psFirmwareMainHeap;
	DEVMEM_HEAP				*psFirmwareConfigHeap;
	MMU_CONTEXT				*psKernelMMUCtx;

	void					*pvDeviceMemoryHeap;

	MTGPU_MEMALLOCFLAGS_T  uiFWPoisonOnFreeFlag;           /*!< Flag for poisoning FW allocations when freed */

	MT_BOOL				bIgnoreHWReportedBVNC;			/*!< Ignore BVNC reported by HW */

	/*
		if we don't preallocate the pagetables we must
		insert newly allocated page tables dynamically
	*/
	void					*pvMMUContextList;

	MT_UINT32				ui32ClkGateStatusReg;
	MT_UINT32				ui32ClkGateStatusMask;

	DEVMEM_MEMDESC			*psMUSAFWCodeMemDesc;
	uint64_t		sFWCodeDevVAddrBase;
	MT_UINT32			ui32FWCodeSizeInBytes;
	DEVMEM_MEMDESC			*psMUSAFWDataMemDesc;
	uint64_t		sFWDataDevVAddrBase;

	DEVMEM_MEMDESC			*psMUSAFWCorememCodeMemDesc;
	uint64_t		sFWCorememCodeDevVAddrBase;
	MUSAFWIF_DEV_VIRTADDR		sFWCorememCodeFWAddr;
	MT_UINT32			ui32FWCorememCodeSizeInBytes;

	DEVMEM_MEMDESC			*psMUSAFWIfCorememDataStoreMemDesc;
	uint64_t		sFWCorememDataStoreDevVAddrBase;
	MUSAFWIF_DEV_VIRTADDR		sFWCorememDataStoreFWAddr;

	DEVMEM_MEMDESC			*psMUSAFWAlignChecksMemDesc;


	POS_LOCK				hMUSAFWIfBufInitLock;								/*!< trace buffer lock for initialisation phase */

	MT_UINT32				ui32ClockSource;
	MT_UINT32				ui32LastClockSource;

	DEVMEM_MEMDESC			*psMUSAFWIfGpuUtilFWCbCtlMemDesc;
	MUSAFWIF_GPU_UTIL_FWCB	*psMUSAFWIfGpuUtilFWCb;

	DEVMEM_MEMDESC			*psMUSAFWIfHWPerfBufMemDesc;
	MT_BYTE				*psMUSAFWIfHWPerfBuf;
	MT_UINT32				ui32MUSAFWIfHWPerfBufSize; /* in bytes */

	DEVMEM_MEMDESC			*psMUSAFWIfRegCfgMemDesc;

	DEVMEM_MEMDESC			*psMUSAFWIfHWPerfCountersMemDesc;

	DEVMEM_MEMDESC			*psMUSAFWIfConnectionCtlMemDesc;

	DEVMEM_MEMDESC			*psMUSAFWHeapGuardPageReserveMemDesc;
	DEVMEM_MEMDESC			*psMUSAFWIfSysInitMemDesc;

	DEVMEM_MEMDESC			*psMUSAFWIfOsInitMemDesc;

	/* Additional guest firmware memory context info */
	DEVMEM_HEAP				*psGuestFirmwareRawHeap[MUSA_NUM_OS_SUPPORTED];
	DEVMEM_MEMDESC			*psGuestFirmwareRawMemDesc[MUSA_NUM_OS_SUPPORTED];

#if defined(SUPPORT_WORKLOAD_ESTIMATION)
	/* Array to store data needed for workload estimation when a workload
	   has finished and its cycle time is returned to the host.	 */
	WORKEST_RETURN_DATA     asReturnData[RETURN_DATA_ARRAY_SIZE];
	MT_UINT32              ui32ReturnDataWO;
	POS_LOCK                hWorkEstLock;
#endif

#if defined(SUPPORT_PDVFS)
	/**
	 * Host memdesc and pointer to memory containing core clock rate in Hz.
	 * Firmware (PDVFS) updates the memory on changing the core clock rate over
	 * GPIO.
	 * Note: Shared memory needs atomic access from Host driver and firmware,
	 * hence size should not be greater than memory transaction granularity.
	 * Currently it is chosen to be 32 bits.
	 */
	DEVMEM_MEMDESC			*psMUSAFWIFCoreClkRateMemDesc;
	volatile MT_UINT32		*pui32MUSAFWIFCoreClkRate;
	/**
	 * Last sampled core clk rate.
	 */
	volatile MT_UINT32		ui32CoreClkRateSnapshot;
#endif

	/*
	   HWPerf data for the MUSA device
	 */

	POS_LOCK    hHWPerfLock;  /*! Critical section lock that protects HWPerf code
	                           *  from multiple thread duplicate init/deinit
	                           *  and loss/freeing of FW & Host resources while in
	                           *  use in another thread e.g. MSIR. */

	MT_UINT64  ui64HWPerfFilter; /*! Event filter for FW events (settable by AppHint in ddk1.0) */
	MT_UINT64  ui64HWPerfFilterStashed; /*! Stashed event filter for FW events.
	                                      *  (stash ui64HWPerfFilter during hwperf reset) */
	MT_HANDLE  hHWPerfStream;    /*! TL Stream buffer (L2) for firmware event stream */
	MT_UINT32  ui32L2BufMaxPacketSize;/*!< Max allowed packet size in FW HWPerf TL (L2) buffer */
	MT_BOOL    bSuspendHWPerfL2DataCopy;  /*! Flag to indicate if copying HWPerf data is suspended */
	struct {
		/* Used to record the time taken by the copy action */
		MT_UINT64 ui64MinCopyTimePerEventInNs;
		MT_UINT64 ui64MaxCopyTimePerEventInNs;
		MT_UINT64 ui64AccumulatedCopyTimesInNs;
		MT_UINT64 ui64AccumulatedCopyNum;

		/* Used to record the time taken by the events producing */
		MT_UINT64 ui64ProduceTimestampInNs;
		MT_UINT64 ui64MinProduceTimePerfEventInNs;
		MT_UINT64 ui64MaxProduceTimePerfEventInNs;
	} sHWPerfDebugInfo;			/* Debug info for L1 buffer copying process */

	MT_UINT32  ui32HWPerfHostFilter;      /*! Event filter for HWPerfHost stream (settable by AppHint) */
	POS_LOCK    hLockHWPerfHostStream;     /*! Lock guarding access to HWPerfHost stream from multiple threads */
	MT_HANDLE  hHWPerfHostStream;         /*! TL Stream buffer for host only event stream */
	MT_UINT32  ui32HWPerfHostBufSize;     /*! Host side buffer size in bytes */
	MT_UINT32  ui32HWPerfHostLastOrdinal; /*! Ordinal of the last packet emitted in HWPerfHost TL stream.
	                                        *  Guarded by hLockHWPerfHostStream */
	MT_UINT32  ui32HWPerfHostNextOrdinal; /*! Ordinal number for HWPerfHost events. Guarded by hHWPerfHostSpinLock */
	MT_UINT8   *pui8DeferredEvents;       /*! List of HWPerfHost events yet to be emitted in the TL stream.
	                                        *  Events generated from atomic context are deferred "emitted"
											*  as the "emission" code can sleep */
	MT_UINT16  ui16DEReadIdx;             /*! Read index in the above deferred events buffer */
	MT_UINT16  ui16DEWriteIdx;            /*! Write index in the above deferred events buffer */
	void        *pvHostHWPerfMISR;         /*! MISR to emit pending/deferred events in HWPerfHost TL stream */
	POS_SPINLOCK hHWPerfHostSpinLock;      /*! Guards data shared between an atomic & sleepable-context */

	MT_UINT32 	ui32NumHWPerfBlocks;	/*! Number of HWPerf Block-IDs (MUSA_CNTBLK_ID) which are available */
	MT_UINT32	ui32NumConfiguredBlocks;/*! Number of HWPerf Block-IDs (MUSA_CNTBLK)ID) which are configured */
	MT_UINT32	ui32CtrlWord;		/*! Perf counter control word. See MUSA_HWPERF_CTRL_GEOM_FULLRANGE etc. */
	MUSA_HWPERF_CONFIG_CNTBLK *psBlockConfigs;
	
#if defined(MTGPU_HWPERF_HOST_DEBUG_DEFERRED_EVENTS)
	MT_UINT32  ui32DEHighWatermark;       /*! High watermark of deferred events buffer usage. Protected by
	                                        *! hHWPerfHostSpinLock */
	/* Max number of times DeferredEmission waited for an atomic-context to "finish" packet write */
	MT_UINT32  ui32WaitForAtomicCtxPktHighWatermark; /*! Protected by hLockHWPerfHostStream */
	/* Whether warning has been logged about an atomic-context packet loss (due to too long wait for "write" finish) */
	MT_BOOL    bWarnedAtomicCtxPktLost;
	/* Max number of times DeferredEmission scheduled-out to give a chance to the right-ordinal packet to be emitted */
	MT_UINT32  ui32WaitForRightOrdPktHighWatermark; /*! Protected by hLockHWPerfHostStream */
	/* Whether warning has been logged about an packet loss (due to too long wait for right ordinal to emit) */
	MT_BOOL    bWarnedPktOrdinalBroke;
#endif

	void        *pvGpuFtraceData;

	/* Poll data for detecting firmware fatal errors */
	MT_UINT32				aui32CrLastPollCount[MUSAFW_THREAD_NUM];
	MT_UINT32				ui32KCCBCmdsExecutedLastTime;
	MT_UINT32				ui32KCCBCmdConsecutiveWaitTimes;
	MT_UINT32				ui32GEOTimeoutsLastTime;
	MT_UINT32				ui32InterruptCountLastTime;
	MT_UINT32				ui32MissingInterruptsLastTime;

	/* Client stall detection */
	MT_UINT32				ui32StalledClientMask;

	MT_BOOL				bWorkEstEnabled;
	MT_BOOL				bPDVFSEnabled;

	void					*pvLISRData;
	void					*pvMISRData;
	void					*pvAPMISRData;
	MUSA_ACTIVEPM_CONF		eActivePMConf;
	MT_BOOL				bAPMEnabled;

	volatile MT_UINT32		aui32SampleIRQCount[MUSAFW_THREAD_NUM];

	DEVMEM_MEMDESC			*psMUSAFaultAddressMemDesc;

	DEVMEM_MEMDESC			*psL23FenceMemDesc;

	/* If we do 10 deferred memory allocations per second, then the ID would wrap around after 13 years */
	MT_UINT32				ui32ZSBufferCurrID;	/*!< ID assigned to the next deferred devmem allocation */
	MT_UINT32				ui32FreelistCurrID;	/*!< ID assigned to the next freelist */

	POS_LOCK				hLockZSBuffer;		/*!< Lock to protect simultaneous access to ZSBuffers */
	DLLIST_NODE				sZSBufferHead;		/*!< List of on-demand ZSBuffers */
	POS_LOCK				hLockFreeList;		/*!< Lock to protect simultaneous access to Freelists */
	DLLIST_NODE				sFreeListHead;		/*!< List of growable Freelists */
	PSYNC_PRIM_CONTEXT		hSyncPrimContext;
	MTGPU_CLIENT_SYNC_PRIM	*psPowSyncPrim;

	MT_UINT32				ui32ActivePMReqOk;
	MT_UINT32				ui32ActivePMReqDenied;
	MT_UINT32				ui32ActivePMReqNonIdle;
	MT_UINT32				ui32ActivePMReqRetry;
	MT_UINT32				ui32ActivePMReqTotal;

	MT_HANDLE				hProcessQueuesMISR;

	MT_UINT32				ui32DeviceFlags;		/*!< Flags to track general device state */

	/* GPU DVFS Table */
	MUSA_GPU_DVFS_TABLE		*psGpuDVFSTable;

	/* Pointer to function returning the GPU utilisation statistics since the last
	 * time the function was called. Supports different users at the same time.
	 *
	 * psReturnStats [out]: GPU utilisation statistics (active high/active low/idle/blocked)
	 *                      in microseconds since the last time the function was called
	 *                      by a specific user (identified by hGpuUtilUser)
	 *
	 * Returns MTGPU_OK in case the call completed without errors,
	 * some other value otherwise.
	 */
	MTGPU_ERROR (*pfnGetGpuUtilStats) (MTGPU_DEVICE_NODE *psDeviceNode,
	                                    MT_HANDLE hGpuUtilUser,
	                                    MUSAFWIF_GPU_UTIL_STATS *psReturnStats);

	/* Pointer to function that checks if the physical GPU IRQ
	 * line has been asserted and clears it if so */
	MT_BOOL (*pfnMUSAAckIrq) (struct _MTGPU_MUSADEV_INFO_ *psDevInfo);

	POS_LOCK				hGPUUtilLock;

	/* Register configuration */
	MUSA_REG_CONFIG			sRegCongfig;

	MT_BOOL				bMUSAPowered;
	DLLIST_NODE				sMemoryContextList;

	POSWR_LOCK				hRenderCtxListLock;
	POSWR_LOCK				hComputeCtxListLock;
	POSWR_LOCK				hTransferCtxListLock;
	POSWR_LOCK				hTRANSFERCtxListLock;
	POSWR_LOCK				hCECtxListLock;
	POSWR_LOCK				hMemoryCtxListLock;
	POSWR_LOCK				hKickSyncCtxListLock;

	/* Linked list of deferred KCCB commands due to a full KCCB.
	 * Access to members sKCCBDeferredCommandsListHead and ui32KCCBDeferredCommandsCount
	 * are protected by the hLockKCCBDeferredCommandsList spin lock. */
	POS_SPINLOCK			hLockKCCBDeferredCommandsList; /*!< Protects deferred KCCB commands list */
	DLLIST_NODE				sKCCBDeferredCommandsListHead;
	MT_UINT32				ui32KCCBDeferredCommandsCount; /*!< No of commands in the deferred list */

	/* Linked lists of contexts on this device */
	DLLIST_NODE				sRenderCtxtListHead;
	DLLIST_NODE				sComputeCtxtListHead;
	DLLIST_NODE				sTRANSFERCtxtListHead;
	DLLIST_NODE				sCECtxtListHead;
	DLLIST_NODE				sKickSyncCtxtListHead;

	DLLIST_NODE				sCommonCtxtListHead;
	POSWR_LOCK				hCommonCtxtListLock;
	MT_UINT32				ui32CommonCtxtCurrentID;	/*!< ID assigned to the next common context */

	POS_LOCK				hDebugFaultInfoLock;	/*!< Lock to protect the debug fault info list */
	POS_LOCK				hMMUCtxUnregLock;		/*!< Lock to protect list of unregistered MMU contexts */

#if defined(SUPPORT_VALIDATION)
	MUSA_POWER_DOMAIN_STATE	sPowerDomainState;		/*!< Power island sequence */
	MT_UINT32				ui32PowDomainKickInterval;	/*!< Power island transition interval */
	MT_UINT32				ui32ValidationFlags;	/*!< Validation flags for host driver */
#endif
	MT_UINT32				ui32AvailablePowUnitsMask;
	MT_UINT32				ui32AvailableRACMask;

	MUSA_LAYER_PARAMS		sLayerParams;

	MUSAFWIF_DM				eBPDM;					/*!< Current breakpoint data master */
	MT_BOOL				bBPSet;					/*!< A Breakpoint has been set */
	POS_LOCK				hBPLock;				/*!< Lock for break point operations */

	MT_UINT32				ui32CoherencyTestsDone;

	ATOMIC_T				iCCBSubmissionOrdinal; /* Rolling count used to indicate CCB submission order (all CCBs) */
	POS_LOCK				hCCBRecoveryLock;      /* Lock to protect pvEarliestStalledClientCCB and ui32OldestSubmissionOrdinal variables */
	void					*pvEarliestStalledClientCCB; /* Will point to cCCB command to unblock in the event of a stall */
	MT_UINT32				ui32OldestSubmissionOrdinal; /* Earliest submission ordinal of CCB entry found so far */
	MT_UINT32				ui32SLRHoldoffCounter;   /* Decremented each time health check is called until zero. SLR only happen when zero. */

	POS_LOCK				hCCBStallCheckLock; /* Lock used to guard against multiple threads simultaneously checking for stalled CCBs */

	/* Value to store for each page size range config register in MMU4 */
	MT_UINT64				aui64MMUPageSizeRangeValue[MUSA_MAX_NUM_MMU_PAGE_SIZE_RANGES];

#if defined(SUPPORT_VALIDATION) && defined(SUPPORT_SOC_TIMER)
	struct
	{
		MT_UINT64 ui64timerGray;
		MT_UINT64 ui64timerBinary;
		MT_UINT64 *pui64uscTimers;
	} sMUSATimerValues;
#endif

#if defined(SUPPORT_VALIDATION)
	struct
	{
		MT_UINT64 ui64RegVal;
		struct completion sRegComp;
	} sFwRegs;
#endif

	MT_HANDLE				hTQCLISharedMem;		/*!< TQ Client Shared Mem PMR */
	MT_HANDLE				hTQMPSharedMem;		/*!< TQ MP Shared Mem PMR */

#if defined(SUPPORT_VALIDATION)
	MT_UINT32				ui32TestSLRInterval; /* Don't enqueue an update sync checkpoint every nth kick */
	MT_UINT32				ui32TestSLRCount;    /* (used to test SLR operation) */
	MT_UINT32				ui32SLRSkipFWAddr;
#endif

#if defined(SUPPORT_SECURITY_VALIDATION)
	DEVMEM_MEMDESC			*psMUSAFWIfSecureBufMemDesc;
	DEVMEM_MEMDESC			*psMUSAFWIfNonSecureBufMemDesc;
#endif

	/* Timer Queries */
	MT_UINT32				ui32ActiveQueryId;		/*!< id of the active line */
	MT_BOOL				bSaveStart;				/*!< save the start time of the next kick on the device*/
	MT_BOOL				bSaveEnd;				/*!< save the end time of the next kick on the device*/

	DEVMEM_MEMDESC			*psStartTimeMemDesc;    /*!< memdesc for Start Times */
	MT_UINT64				*pui64StartTimeById;    /*!< CPU mapping of the above */

	DEVMEM_MEMDESC			*psEndTimeMemDesc;      /*!< memdesc for End Timer */
	MT_UINT64				*pui64EndTimeById;      /*!< CPU mapping of the above */

	MT_UINT32				aui32ScheduledOnId[MUSA_MAX_TIMER_QUERIES];	/*!< kicks Scheduled on QueryId */
	DEVMEM_MEMDESC			*psCompletedMemDesc;	/*!< kicks Completed on QueryId */
	MT_UINT32				*pui32CompletedById;	/*!< CPU mapping of the above */

#if !defined(MTGPU_USE_BRIDGE_LOCK)
	POS_LOCK				hTimerQueryLock;		/*!< lock to protect simultaneous access to timer query members */
#endif

	MTGPU_MUSADEV_ERROR_COUNTS sErrorCounts;		/*!< struct containing device error counts */

	MT_UINT32				ui32HostSafetyEventMask;/*!< mask of the safety events handled by the driver */

	MT_UINT64				ui64LastDeviceError;	/*!< device error reported to client */

	MT_UINT32              ui32Log2Non4KPgSize; /* Page size of Non4k heap in log2 form */

	MT_UINT32              ui32Log2SVMPgSize; /* Page size of SVM heap in log2 form */

	MT_UINT32              ui32HighSVMPgSizeBitMask; /* Page size bit mask of High SVM heap for ph1 or later*/

	u32					mpc_id;
	void					*priv_data;
#if (MUSA_NUM_OS_SUPPORTED > 1)
	void                                    *psLinuxFwInfo;
	void					*psWinFwInfo;
	void					(*vgpu_int_cb)(u32 int_id, bool is_osid0,
							       void *priv_data, u32 mpc_id);
#endif

	MT_UINT64			ui64DummyCpuPAddr;
	MT_UINT64			ui64DummyDmaAddr;

	PB_DATA                         sGlobalPBData;
} MTGPU_MUSADEV_INFO;


#if (MUSA_NUM_OS_SUPPORTED > 1)
/* For vGPU FW */
typedef struct _LINUX_FW_INFO_
{
	MTGPU_MUSADEV_INFO    *psDevInfo;
	MT_UINT64             fw_heap_dpa_base[MTGPU_VZ_NUM_OSID];
	void                  *fw_heap_vaddr[MTGPU_VZ_NUM_OSID];
	MT_UINT32             ui32KCCBCtlOffset;
	MT_UINT32             ui32FwOsDataOffset;
} LINUX_FW_INFO;
#endif

typedef struct _MUSA_TIMING_INFORMATION_
{
	/*! GPU default core clock speed in Hz */
	MT_UINT32			ui32CoreClockSpeed;

	/*! Active Power Management: GPU actively requests the host driver to be powered off */
	MT_BOOL			bEnableActivePM;

	/*! Enable the GPU to power off internal Power Islands independently from the host driver */
	MT_BOOL			bEnableRDPowIsland;

	/*! Active Power Management: Delay between the GPU idle and the request to the host */
	MT_UINT32			ui32ActivePMLatencyms;

} MUSA_TIMING_INFORMATION;

typedef struct _MUSA_DATA_
{
	/*! Timing information */
	MUSA_TIMING_INFORMATION	*psMUSATimingInfo;
} MUSA_DATA;

/*
	MUSA PDUMP register bank name (prefix)
*/
#define MUSA_PDUMPREG_NAME		"MUSAREG"

#endif /* MUSADEVICE_H */

/**************************************************************************/ /*!
@File
@Title          Common Device header
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Device related function templates and defines
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
*/ /***************************************************************************/

#ifndef DEVICE_H
#define DEVICE_H

#include "devicemem_heapcfg.h"
#include "mmu_common.h"
#include "ra.h"			/* RA_ARENA */
#include "mt_device.h"
#include "sync_checkpoint.h"
#include "srvkm.h"
#include "physheap.h"
#include "sync_internal.h"
#include "sysinfo.h"
#include "dllist.h"
#include "musa_bvnc_defs_km.h"
#include "lock.h"
#include "power.h"
#include "mtgpu_device.h"
#include "debug_common.h"

#if defined(SUPPORT_GPUVIRT_VALIDATION)
#include "virt_validation_defs.h"
#endif

typedef struct _MTGPU_POWER_DEV_TAG_ *PMTGPU_POWER_DEV;

struct SYNC_RECORD;

struct _CONNECTION_DATA_;
struct mtgpu_syncobj_context;
struct _MTEXT_INFO_;

/*************************************************************************/ /*!
 @Function      AllocUFOBlockCallback
 @Description   Device specific callback for allocation of a UFO block

 @Input         psDeviceNode          Pointer to device node to allocate
                                      the UFO for.
 @Output        ppsMemDesc            Pointer to pointer for the memdesc of
                                      the allocation
 @Output        pui32SyncAddr         FW Base address of the UFO block
 @Output        puiSyncPrimBlockSize  Size of the UFO block

 @Return        MTGPU_OK if allocation was successful
*/ /**************************************************************************/
typedef MTGPU_ERROR (*AllocUFOBlockCallback)(struct _MTGPU_DEVICE_NODE_ *psDeviceNode,
														DEVMEM_MEMDESC **ppsMemDesc,
														MT_UINT32 *pui32SyncAddr,
														MT_UINT32 *puiSyncPrimBlockSize,
														MT_BOOL bSystemMem);

/*************************************************************************/ /*!
 @Function      FreeUFOBlockCallback
 @Description   Device specific callback for freeing of a UFO

 @Input         psDeviceNode    Pointer to device node that the UFO block was
                                allocated from.
 @Input         psMemDesc       Pointer to pointer for the memdesc of the UFO
                                block to free.
*/ /**************************************************************************/
typedef void (*FreeUFOBlockCallback)(struct _MTGPU_DEVICE_NODE_ *psDeviceNode,
									 DEVMEM_MEMDESC *psMemDesc);

typedef struct _MTGPU_DEVICE_IDENTIFIER_
{
	/* Pdump memory and register bank names */
	MT_CHAR				*pszPDumpDevName;
	MT_CHAR				*pszPDumpRegName;

	/* Under Linux, this is the minor number of RenderNode corresponding to this Device */
	MT_INT32				i32OsDeviceID;
	/* Services layer enumeration of the device used in mtdebug */
	MT_UINT32				ui32InternalID;
} MTGPU_DEVICE_IDENTIFIER;

typedef struct _DEVICE_MEMORY_INFO_
{
	/* Heap count. Doesn't include additional heaps from MTGPUCreateDeviceMemHeap */
	MT_UINT32				ui32HeapCount;

	/* Blueprints for creating new device memory contexts */
	MT_UINT32              uiNumHeapConfigs;
	MT_UINT64              ui64ClientHeapPageSizeFlag;
	MT_UINT64              ui64FWHeapPageSizeFlag;
	DEVMEM_HEAP_CONFIG      *psDeviceMemoryHeapConfigArray;
	DEVMEM_HEAP_BLUEPRINT   *psDeviceMemoryHeap;
} DEVICE_MEMORY_INFO;

#define MMU_BAD_PHYS_ADDR (0xbadbad00badULL)
#define DUMMY_PAGE	("DUMMY_PAGE")
#define DEV_ZERO_PAGE	("DEV_ZERO_PAGE")
#define MT_DUMMY_PAGE_INIT_VALUE	(0x0)
#define MT_ZERO_PAGE_INIT_VALUE	(0x0)

typedef struct __DEFAULT_PAGE__
{
	/*Page handle for the page allocated (UMA/LMA)*/
	PG_HANDLE	sPageHandle;
	POS_LOCK	psPgLock;
	ATOMIC_T	atRefCounter;
	/*Default page size in terms of log2 */
	MT_UINT32	ui32Log2PgSize;
	MT_UINT64	ui64PgPhysAddr;
} MTGPU_DEF_PAGE;

typedef enum _MTGPU_DEVICE_STATE_
{
	MTGPU_DEVICE_STATE_UNDEFINED = 0,
	MTGPU_DEVICE_STATE_INIT,
	MTGPU_DEVICE_STATE_ACTIVE,
	MTGPU_DEVICE_STATE_DEINIT,
	MTGPU_DEVICE_STATE_BAD,
	MTGPU_DEVICE_STATE_RESET,
} MTGPU_DEVICE_STATE;

typedef enum _MTGPU_DEVICE_HEALTH_STATUS_
{
	MTGPU_DEVICE_HEALTH_STATUS_UNDEFINED = 0,
	MTGPU_DEVICE_HEALTH_STATUS_OK,
	MTGPU_DEVICE_HEALTH_STATUS_NOT_RESPONDING,
	MTGPU_DEVICE_HEALTH_STATUS_DEAD,
	MTGPU_DEVICE_HEALTH_STATUS_FAULT
} MTGPU_DEVICE_HEALTH_STATUS;

typedef enum _MTGPU_DEVICE_HEALTH_REASON_
{
	MTGPU_DEVICE_HEALTH_REASON_NONE = 0,
	MTGPU_DEVICE_HEALTH_REASON_ASSERTED,
	MTGPU_DEVICE_HEALTH_REASON_POLL_FAILING,
	MTGPU_DEVICE_HEALTH_REASON_TIMEOUTS,
	MTGPU_DEVICE_HEALTH_REASON_QUEUE_CORRUPT,
	MTGPU_DEVICE_HEALTH_REASON_QUEUE_STALLED,
	MTGPU_DEVICE_HEALTH_REASON_IDLING,
	MTGPU_DEVICE_HEALTH_REASON_RESTARTING,
	MTGPU_DEVICE_HEALTH_REASON_MISSING_INTERRUPTS
} MTGPU_DEVICE_HEALTH_REASON;

typedef enum _MTGPU_DEVICE_DEBUG_DUMP_STATUS_
{
	MTGPU_DEVICE_DEBUG_DUMP_NONE = 0,
	MTGPU_DEVICE_DEBUG_DUMP_CAPTURE
} MTGPU_DEVICE_DEBUG_DUMP_STATUS;

#ifndef DI_GROUP_DEFINED
#define DI_GROUP_DEFINED
typedef struct DI_GROUP DI_GROUP;
#endif
#ifndef DI_ENTRY_DEFINED
#define DI_ENTRY_DEFINED
typedef struct DI_ENTRY DI_ENTRY;
#endif

struct _mt_ion_stats_state_;

typedef struct _MTGPU_DEVICE_DEBUG_INFO_
{
	DI_GROUP *psGroup;
	DI_ENTRY *psAPMEnableEntry;
	DI_ENTRY *psDumpDebugEntry;
	DI_ENTRY *psFwSchSnapshotEntry;
	DI_ENTRY *psMusaSnapshotEntry;
	DI_ENTRY *psPowerModeEntry;
	DI_ENTRY *psDVFSModeEntry;
	DI_ENTRY *psDisableHWREntry;
	DI_ENTRY *psHardwareResetEntry;
#ifdef SUPPORT_MUSA
	DI_ENTRY *psFWTraceEntry;
	DI_ENTRY *psFWFECTraceEntry;
	DI_ENTRY *psFWTraceStreamingEntry;
	DI_ENTRY *psFWMappingsEntry;
	DI_ENTRY *psFWGcovEntry;
#if defined(SUPPORT_VALIDATION) || defined(SUPPORT_RISCV_GDB)
	DI_ENTRY *psRiscvDmiDIEntry;
	MT_UINT64 ui64RiscvDmi;
#endif
#endif /* SUPPORT_MUSA */
#ifdef SUPPORT_VALIDATION
	DI_ENTRY *psMUSARegsEntry;
#endif /* SUPPORT_VALIDATION */
#ifdef SUPPORT_POWER_VALIDATION_VIA_DEBUGFS
	DI_ENTRY *psPowMonEntry;
#endif
#ifdef SUPPORT_POWER_SAMPLING_VIA_DEBUGFS
	DI_ENTRY *psPowerDataEntry;
#endif
	DI_ENTRY *psGpuMemUseEntry;
	DI_ENTRY *psGpuStatusEntry;
	DI_ENTRY *psGpuIovaAllocatedSizeEntry;
	DI_ENTRY *psNOPCmdEntry;
	DI_ENTRY *psDmaTestEntry;
	DI_ENTRY *psGpuConnectionResEntry;

#if defined(MTGPU_ENABLE_MT_ION_STATS)
	struct _mt_ion_stats_state_ *psMtIonStatsState;
#endif

#if (MUSA_NUM_OS_SUPPORTED > 1)
	DI_ENTRY *psVgpuSchedulerEntry;
	DI_ENTRY *psVgpuPerfEntry;
	DI_ENTRY *psVgpuCswTimeEntry;
#endif

#ifdef DEBUG
	DI_ENTRY *psGpuRasCtrlInBandInject;
#endif

	DI_ENTRY *psGpuObMapInfo;
	DI_ENTRY *psDisableWatchdogEntry;
	DI_ENTRY *psPmrRefDumpSummary;
	DI_ENTRY *psPmrRefDumpStack;
	DI_ENTRY *psPmrRefDumpFilter;
	DI_ENTRY *psVramShowEntry;
} MTGPU_DEVICE_DEBUG_INFO;

#if defined(MTGPU_DEBUG_LISR_EXECUTION)
#define MUSA_LISR_INIT							(0U)
#define MUSA_LISR_DEVICE_NOT_POWERED				(1U)
#define MUSA_LISR_NOT_TRIGGERED_BY_HW			(2U)
#define MUSA_LISR_FW_IRQ_COUNTER_NOT_UPDATED		(3U)
#define MUSA_LISR_PROCESSED						(4U)

typedef MT_UINT32 LISR_STATUS;

typedef struct _LISR_EXECUTION_INFO_
{
	/* status of last LISR invocation */
	LISR_STATUS ui32Status;

	/* snapshot from the last LISR invocation */
#if defined(MUSA_FW_IRQ_OS_COUNTERS)
	MT_UINT32 aui32InterruptCountSnapshot[MUSA_NUM_OS_SUPPORTED];
#else
	MT_UINT32 aui32InterruptCountSnapshot[MUSAFW_THREAD_NUM];
#endif

	/* time of the last LISR invocation */
	MT_UINT64 ui64Clockns;
} LISR_EXECUTION_INFO;

#define UPDATE_LISR_DBG_STATUS(status)		psDeviceNode->sLISRExecutionInfo.ui32Status = (status)
#define UPDATE_LISR_DBG_SNAPSHOT(idx, val)	psDeviceNode->sLISRExecutionInfo.aui32InterruptCountSnapshot[idx] = (val)
#define UPDATE_LISR_DBG_TIMESTAMP()			psDeviceNode->sLISRExecutionInfo.ui64Clockns = OSClockns64()
#define UPDATE_LISR_DBG_COUNTER()			psDeviceNode->ui64nLISR++
#define UPDATE_MISR_DBG_COUNTER()			psDeviceNode->ui64nMISR++
#else
#define UPDATE_LISR_DBG_STATUS(status)
#define UPDATE_LISR_DBG_SNAPSHOT(idx, val)
#define UPDATE_LISR_DBG_TIMESTAMP()
#define UPDATE_LISR_DBG_COUNTER()
#define UPDATE_MISR_DBG_COUNTER()
#endif /* defined(MTGPU_DEBUG_LISR_EXECUTION) */

typedef struct _MTGPU_DEVICE_NODE_
{
	 /* WARNING: the sMTDeviceNode must be the first member */
	struct mtgpu_device_node 	sMTDeviceNode;
	MTGPU_DEVICE_IDENTIFIER	sDevId;
	MTGPU_DEVICE_STATE		eDevState;
	MT_BOOL			bForceLegacyHeap; /* Force to use Legacy heap layout */
	MTGPU_DEVICE_PARTITION_TYPE	eDevPartitionType;

	ATOMIC_T					eHealthStatus; /* Holds values from MTGPU_DEVICE_HEALTH_STATUS */
	ATOMIC_T					ePreHealthStatus; /* Holds values from MTGPU_DEVICE_HEALTH_STATUS */
	ATOMIC_T					eHealthReason; /* Holds values from MTGPU_DEVICE_HEALTH_REASON */
	ATOMIC_T					eDebugDumpRequested; /* Holds values from MTGPU_DEVICE_DEBUG_DUMP_STATUS */

	MT_HANDLE					*hDebugTable;

	/* device specific MMU attributes */
	MMU_DEVICEATTRIBS      *psMMUDevAttrs;
	/* Device specific MMU firmware attributes, used only in some devices */
	MMU_DEVICEATTRIBS      *psFirmwareMMUDevAttrs;

	PHYS_HEAP              *psMMUPhysHeap;

	/* lock for power state transitions */
	POS_LOCK				hPowerLock;
	MT_PID                 uiPwrLockOwnerPID; /* Only valid between lock and corresponding unlock
	                                              operations of hPowerLock */

	/* current system device power state */
	MTGPU_SYS_POWER_STATE	eCurrentSysPowerState;
	PMTGPU_POWER_DEV	psPowerDev;

    /* multicore configuration information */
    MT_UINT32              ui32MultiCoreNumCores;      /* total cores primary + secondaries. 0 for non-multi core */
    MT_UINT32              ui32MultiCorePrimaryId;     /* primary core id for this device */
    MT_UINT32              ui32PrimaryCoreIds;
    MT_UINT32              ui32TotalCores;
    MT_UINT64             *pui64MultiCoreCapabilities; /* capabilities for each core */

	/*
		callbacks the device must support:
	*/

	MTGPU_ERROR (*pfnValidateOrTweakPhysAddrs)(struct _MTGPU_DEVICE_NODE_ *psDevNode,
												MMU_DEVICEATTRIBS *psDevAttrs,
												MT_UINT64 *pui64Addr);

	void (*pfnMMUCacheInvalidate)(struct _MTGPU_DEVICE_NODE_ *psDevNode,
								  MMU_CONTEXT *psMMUContext,
								  MMU_LEVEL eLevel,
								  MT_BOOL bUnmap);

	MTGPU_ERROR (*pfnMMUCacheInvalidateKick)(struct _MTGPU_DEVICE_NODE_ *psDevNode,
	                                          MT_UINT32 *pui32NextMMUInvalidateUpdate);

	MT_UINT32 (*pfnMMUCacheGetInvalidateCounter)(struct _MTGPU_DEVICE_NODE_ *psDevNode);

	void (*pfnMMUCheckFaultAddress)(MMU_CONTEXT *psMMUContext,
					uint64_t *psDevVAddr,
					MMU_FAULT_DATA *psOutFaultData);

	MT_BOOL (*pfnCheckDevVAddrForMusa)(uint64_t sDevVAddr);

	void (*pfnDumpDebugInfo)(struct _MTGPU_DEVICE_NODE_ *psDevNode);

	MTGPU_ERROR (*pfnUpdateHealthStatus)(struct _MTGPU_DEVICE_NODE_ *psDevNode,
	                                      MT_BOOL bIsTimerPoll);

	void (*pfnGetShaderFileName)(struct _MTGPU_DEVICE_NODE_ *psDevNode,
				     MT_CHAR                    *pszShaderFilenameStr,
				     MT_CHAR                    *pszShaderpFilenameStr);

#if defined(SUPPORT_AUTOVZ)
	void (*pfnUpdateAutoVzWatchdog)(struct _MTGPU_DEVICE_NODE_ *psDevNode);
#endif

	MTGPU_ERROR (*pfnValidationGPUUnitsPowerChange)(struct _MTGPU_DEVICE_NODE_ *psDevNode, MT_UINT32 ui32NewState);

	MTGPU_ERROR (*pfnResetHWRLogs)(struct _MTGPU_DEVICE_NODE_ *psDevNode);

	MTGPU_ERROR (*pfnVerifyBVNC)(struct _MTGPU_DEVICE_NODE_ *psDevNode, MT_UINT64 ui64GivenBVNC, MT_UINT64 ui64CoreIdMask);

	/* Method to drain device HWPerf packets from firmware buffer to host buffer */
	MTGPU_ERROR (*pfnServiceHWPerf)(struct _MTGPU_DEVICE_NODE_ *psDevNode);

	MTGPU_ERROR (*pfnDeviceVersionString)(struct _MTGPU_DEVICE_NODE_ *psDevNode, MT_CHAR **ppszVersionString);

	MTGPU_ERROR (*pfnDeviceClockSpeed)(struct _MTGPU_DEVICE_NODE_ *psDevNode, MT_PUINT32 pui32MUSAClockSpeed);

	MTGPU_ERROR (*pfnSoftReset)(struct _MTGPU_DEVICE_NODE_ *psDevNode, MT_UINT64 ui64ResetValue1, MT_UINT64 ui64ResetValue2);

	MTGPU_ERROR (*pfnAlignmentCheck)(struct _MTGPU_DEVICE_NODE_ *psDevNode, MT_UINT32 ui32FWAlignChecksSize, MT_UINT32 aui32FWAlignChecks[]);
	MT_BOOL	(*pfnCheckDeviceFeature)(struct _MTGPU_DEVICE_NODE_ *psDevNode, MT_UINT64 ui64FeatureMask);

	MT_INT32	(*pfnGetDeviceFeatureValue)(struct _MTGPU_DEVICE_NODE_ *psDevNode, enum _MUSA_FEATURE_WITH_VALUE_INDEX_ eFeatureIndex);

    MTGPU_ERROR (*pfnGetMultiCoreInfo)(struct _MTGPU_DEVICE_NODE_ *psDevNode, MT_UINT32 ui32CapsSize,
                                        MT_UINT32 *pui32NumCores, MT_UINT64 *pui64Caps);

	MT_BOOL (*pfnHasFBCDCVersion31)(struct _MTGPU_DEVICE_NODE_ *psDevNode);

	MMU_DEVICEATTRIBS* (*pfnGetMMUDeviceAttributes)(struct _MTGPU_DEVICE_NODE_ *psDevNode, MT_BOOL bKernelMemoryCtx);

	MTGPU_DEVICE_CONFIG	*psDevConfig;

	/* initialise device-specific physheaps */
	MTGPU_ERROR			(*pfnPhysMemDeviceHeapsInit) (struct _MTGPU_DEVICE_NODE_ *);

	/* initialise fw mmu, if FW not using GPU mmu, NULL otherwise. */
	MTGPU_ERROR			(*pfnFwMMUInit) (struct _MTGPU_DEVICE_NODE_ *);

	void (*pfnInitBIF)(const void *hPrivate);

	void (*pfnGetPCBase)(void *pvRegsBaseKM, MT_UINT32 ui32ContextID,
			     MT_DEV_PHYADDR *psPCDevPAddr,
			     MT_BOOL *pbIsValid, MT_UINT32 *pui32Enable);

	/* store gpu register before soft reset */
	void (*pfnGpuRegisterStore)(struct mtgpu_device_node *mt_dev_node, MT_UINT32 ui32CoreCnt);

	/* load gpu register after soft reset */
	void (*pfnGpuRegisterLoad)(struct mtgpu_device_node *mt_dev_node, MT_UINT32 ui32CoreCnt);

	/* information about the device's address space and heaps */
	DEVICE_MEMORY_INFO		sDevMemoryInfo;

	/* device's shared-virtual-memory heap max virtual address */
	MT_UINT64				ui64GeneralSVMHeapTopVA;

	ATOMIC_T				iNumClockSpeedChanges;

	/* private device information */
	void					*pvDevice;

#if defined(SUPPORT_GPUVIRT_VALIDATION)
	RA_ARENA                *psOSSharedArena;
	RA_ARENA				*psOSidSubArena[GPUVIRT_VALIDATION_NUM_OS];
#endif

	/* FW_MAIN, FW_CONFIG and FW_GUEST heaps. Should be part of registered heaps? */
	PHYS_HEAP               *psFWMainPhysHeap;
	PHYS_HEAP               *psFWCfgPhysHeap;
	PHYS_HEAP               *apsFWPremapPhysHeap[MUSA_NUM_OS_SUPPORTED];

	MT_UINT32				ui32RegisteredPhysHeaps;
	PHYS_HEAP				**papsRegisteredPhysHeaps;

	/* PHYS_HEAP Mapping table to the platform's physical memory heap(s)
	 * used by this device. The physical heaps are created based on
	 * the PHYS_HEAP_CONFIG data from the platform's system layer at device
	 * creation time.
	 *
	 * Contains MTGPU_PHYS_HEAP_LAST entries for all the possible physical heaps allowed in the design.
	 * It allows the system layer PhysHeaps for the device to be identified for use in creating new PMRs.
	 * See PhysHeapCreatePMR()
	 */
	PHYS_HEAP				*apsPhysHeap[MTGPU_PHYS_HEAP_LAST];
	MT_UINT32				ui32UserAllocHeapCount;

#if defined(SUPPORT_AUTOVZ)
	/* Phys Heap reserved for storing the MMU mappings of firmware.
	 * The memory backing up this Phys Heap must persist between driver or OS reboots */
	PHYS_HEAP               *psFwMMUReservedPhysHeap;
#endif

	/* Flag indicating if the firmware has been initialised during the
	 * 1st boot of the Host driver according to the AutoVz life-cycle. */
	MT_BOOL				bAutoVzFwIsUp;

	struct _MTGPU_DEVICE_NODE_	*psNext;
	struct _MTGPU_DEVICE_NODE_	**ppsThis;

	/* Functions for notification about memory contexts */
	MTGPU_ERROR			(*pfnRegisterMemoryContext)(struct _MTGPU_DEVICE_NODE_		*psDeviceNode,
								    MMU_CONTEXT				*psMMUContext,
								    MT_HANDLE				*hPrivData);
	void					(*pfnUnregisterMemoryContext)(MT_HANDLE hPrivData);

	/* Functions for allocation/freeing of UFOs */
	AllocUFOBlockCallback	pfnAllocUFOBlock;	/*!< Callback for allocation of a block of UFO memory */
	FreeUFOBlockCallback	pfnFreeUFOBlock;	/*!< Callback for freeing of a block of UFO memory */

	MT_HANDLE				hSyncServerRecordNotify;
	POS_LOCK				hSyncServerRecordLock;
	MT_UINT32				ui32SyncServerRecordCount;
	MT_UINT32				ui32SyncServerRecordCountHighWatermark;
	DLLIST_NODE				sSyncServerRecordList;
	struct SYNC_RECORD		*apsSyncServerRecordsFreed[MTGPU_FULL_SYNC_TRACKING_HISTORY_LEN];
	MT_UINT32				uiSyncServerRecordFreeIdx;

	MT_HANDLE				hSyncCheckpointRecordNotify;
	POS_LOCK				hSyncCheckpointRecordLock;
	MT_UINT32				ui32SyncCheckpointRecordCount;
	MT_UINT32				ui32SyncCheckpointRecordCountHighWatermark;
	DLLIST_NODE				sSyncCheckpointRecordList;
	struct SYNC_CHECKPOINT_RECORD	*apsSyncCheckpointRecordsFreed[MTGPU_FULL_SYNC_TRACKING_HISTORY_LEN];
	MT_UINT32				uiSyncCheckpointRecordFreeIdx;

	MT_HANDLE				hSyncCheckpointNotify;
	POS_SPINLOCK			hSyncCheckpointListLock; /*!< Protects sSyncCheckpointSyncsList */
	DLLIST_NODE				sSyncCheckpointSyncsList;

	PSYNC_CHECKPOINT_CONTEXT		hSyncCheckpointContext;
	PSYNC_PRIM_CONTEXT			hSyncPrimContext;

	/* With this sync-prim we make sure the MMU cache is flushed
	 * before we free the page table memory */
	MTGPU_CLIENT_SYNC_PRIM	*psMMUCacheSyncPrim;
	MT_UINT32				ui32NextMMUInvalidateUpdate;

	MT_HANDLE				hCmdCompNotify;
	MT_HANDLE				hDbgReqNotify;
	MT_HANDLE				hAppHintDbgReqNotify;
	MT_HANDLE				hPhysHeapDbgReqNotify;

	MTGPU_DEF_PAGE			sDummyPage;
	MTGPU_DEF_PAGE			sDevZeroPage;

	POSWR_LOCK				hMemoryContextPageFaultNotifyListLock;
	DLLIST_NODE				sMemoryContextPageFaultNotifyListHead;

	/* System DMA capability */
	MT_BOOL				bHasSystemDMA;
	MT_HANDLE				hDmaTxChan;
	MT_HANDLE				hDmaRxChan;


#if defined(SUPPORT_VALIDATION)
	POS_LOCK			hValidationLock;
#endif

	/* Members for linking which connections are open on this device */
	POS_LOCK                hConnectionsLock;    /*!< Lock protecting sConnections */
	DLLIST_NODE             sConnections;        /*!< The list of currently active connection objects for this device node */

	/* HWPerf settings on this device */
	POS_LOCK	hHWPerfConnectionsLock;	/*!< Lock protection sHWPerfConnectionList */
	DLLIST_NODE	sHWPerfConnectionList;	/*!< The list of HWPerf setting objects for this device node */

#if defined(MTGPU_DEBUG_LISR_EXECUTION)
	LISR_EXECUTION_INFO     sLISRExecutionInfo;  /*!< Information about the last execution of the LISR */
	MT_UINT64              ui64nLISR;           /*!< Number of LISR calls seen */
	MT_UINT64              ui64nMISR;           /*!< Number of MISR calls made */
#endif

	MTGPU_DEVICE_DEBUG_INFO sDebugInfo;

	MT_UINT64 ui64StatValue[32];
#if (MUSA_NUM_OS_SUPPORTED > 1)
	MT_UINT64 ui64StatVGPUMemLMA;
	MT_UINT64 ui64StatVGPUMemLMAMax;
#endif
	POS_LOCK hGpuMemStatLock;

#ifdef SUPPORT_MUSA
	MT_HANDLE gpuUtilStaInfo;
	// Trace pointer for streaming trace
	MT_UINT32 ui32OldTracePtr[MUSAFW_THREAD_NUM];
#endif

	ATOMIC64_T iIovaAllocatedSize;
	ATOMIC64_T iDmaMapTimes;
	ATOMIC64_T iDmaUnmapTimes;

	/* Only used for linux guest hwr */
	POS_LOCK   hHwrLock;
	MT_UINT64 ui64LastHwrTime;
	MT_UINT32 ui32GuestInHwr;
	MT_UINT32 ui32GuestNeedSLR;

	/* HWPerf fw events polling on ddk2.0 */
	struct task_struct *psFwPollingThread;
	struct mutex *psFwPollingThreadLock;
	MT_BOOL bFwPollingThreadStop;
	HASH_TABLE *psPmrTraceHashTable;
	POS_LOCK hPmrTraceHashTableLock;
	MT_UINT64 u64PmrTraceFilters[20];
	struct VRAM_HISTORY_DATA *psVramHistoryData;
	MT_INT bIsInited;
} MTGPU_DEVICE_NODE;

/*
 * Macros to be used instead of calling directly the pfns since these macros
 * will expand the feature passed as argument into the bitmask/index to work
 * with the macros defined in musa_bvnc_defs_km.h
 */
#define MTGPU_GET_DEVICE_FEATURE_VALUE(psDevNode, Feature) \
		psDevNode->pfnGetDeviceFeatureValue(psDevNode, MUSA_FEATURE_##Feature##_IDX)

MTGPU_ERROR MTGPUDeviceFinalise(MTGPU_DEVICE_NODE *psDeviceNode,
											   MT_BOOL bInitSuccessful);

MTGPU_ERROR MUSAClientConnectCompatCheck_ClientAgainstFW(MTGPU_DEVICE_NODE * psDeviceNode, MT_UINT32 ui32ClientBuildOptions);


#endif /* DEVICE_H */

/******************************************************************************
 End of file (device.h)
******************************************************************************/

/*
 * @File
 * @Title       uniform application program interface header
 * @Copyright   Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @Description UAPI definitions
 * @License     Dual MIT/GPLv2
 */


#ifndef UAPI_COMMON_H
#define UAPI_COMMON_H

#include "device.h"
#include "mtfw_fwif_utilization.h"

struct mtgpu_device;

/* FIXME: remove hard code in the feature */
#if defined(CONFIG_SW64)
/* ioctl cmd size limited to 8k */
#define PROCESS_NUMS_MAX	40
#else
#define PROCESS_NUMS_MAX	80
#endif

#define MTGPU_CLEAR_MUSA_SERVER_ERROR_COUNT	(MT_UINT64_C(0x1) << 0)
#define MTGPU_CLEAR_MUSA_HWR_EVENT_COUNT	(MT_UINT64_C(0x1) << 1)
#define MTGPU_CLEAR_MUSA_CRR_EVENT_COUNT	(MT_UINT64_C(0x1) << 2)
#define MTGPU_CLEAR_MUSA_SLR_EVENT_COUNT	(MT_UINT64_C(0x1) << 3)
#define MTGPU_CLEAR_MUSA_WGP_ERROR_COUNT	(MT_UINT64_C(0x1) << 4)
#define MTGPU_CLEAR_MUSA_TRP_ERROR_COUNT	(MT_UINT64_C(0x1) << 5)
#define MTGPU_CLEAR_MUSA_FWF_EVENT_COUNT	(MT_UINT64_C(0x1) << 6)
#define MTGPU_CLEAR_MUSA_APM_EVENT_COUNT	(MT_UINT64_C(0x1) << 7)

/*
 * MKIS Managed IOCTL Interface structures begin
 *
 * !!! BE CAUTIONED !!!
 * Below structures are managed by MKIS protocol. Any changing to these
 * contents may lead to compatibility problem against libmtml, so please
 * make sure you have fully understood the purpose of MKIS and how it
 * works before doing any code change. If you indeed have necessity to
 * make changes to these interfaces but don't know how to be compliant with
 * MKIS requirement, please contact zheng.cao@mthreads.com for help.
 * Also, please make sure zheng.cao@mthreads.com is included in your PR
 * reviewer list.
 */

/*! @Brief state structure with the caller */
/* WARNING: MKIS Managed! */
typedef struct _DEVICE_STATUS_INFO_
{
    MTGPU_DEVICE_STATE          eDeviceServiceState;                                 /*!< Services State */
    MT_INT32                    i32OsDeviceID;                                       /*!< Under Linux, this is the minor number of RenderNode corresponding to this Device */
    MT_UINT32                   ui32InternalID;                                      /*!< Services layer enumeration of the device used in mtdebug */
    MTGPU_DEVICE_HEALTH_STATUS  eHealthStatus;                                       /*!< Firmware Status, relative to struct MTGPU_DEVICE_HEALTH_STATUS */
    MTGPU_DEVICE_HEALTH_REASON  eHealthReason;                                       /*!< Firmware Reason When Firmware Status Existting Error, relative to struct MTGPU_DEVICE_HEALTH_REASON */
    MT_UINT32                   ui32ServerErrorCount;                                /*!< Server Errors count */
    MT_UINT64                   ui64nLISR;                                           /*!< Number of LISR calls seen */
    MT_UINT64                   ui64nMISR;                                           /*!< Number of MISR calls made */
    MT_UINT32                   ui32HWREventCount;                                   /*!< HWR Event Count */
    MT_UINT32                   ui32CRREventCount;                                   /*!< CRR Event Count */
    MT_UINT32                   ui32SlrEventCount;                                   /*!< SLR Event Count */
    MT_UINT32                   ui32WGPErrorCount;                                   /*!< count of the number of WGP checksum errors */
    MT_UINT32                   ui32TRPErrorCount;                                   /*!< count of the number of TRP checksum errors */
    MT_UINT32                   ui32FWFaults;                                        /*!< Firmware faults count */
    MT_UINT32                   ui32ActivePMReqTotal;                                /*!< APM Event Count */
    MT_UINT32                   aui32GpuUtil[GPU_UTIL_ITEM_IDX_MAX];                 /*!< GPU Utilisation */
} DEVICE_STATUS_INFO;

/* @Brief driver stats structure with the caller */
/* WARNING: MKIS Managed! */
typedef struct _DEVICE_DRIVER_STATS_INFO_
{
    MT_UINT64                ui64MemUsageAllocPTMemoryUMA;          /*!< MemoryUsageAllocPTMemoryUMA */
    MT_UINT64                ui64MemUsageAllocPTMemoryUMAMax;       /*!< MemoryUsageAllocPTMemoryUMAMax */
    MT_UINT64                ui64MemUsageVMapPTUMA;                 /*!< MemoryUsageVMapPTUMA */
    MT_UINT64                ui64MemUsageVMapPTUMAMax;              /*!< MemoryUsageVMapPTUMAMax */
    MT_UINT64                ui64MemUsageAllocPTMemLMA;             /*!< MemoryUsageAllocPTMemoryLMA */
    MT_UINT64                ui64MemUsageAllocPTMemLMAMax;          /*!< MemoryUsageAllocPTMemoryLMAMax */
    MT_UINT64                ui64MemUsageIORemapPTLMA;              /*!< MemoryUsageIORemapPTLMA */
    MT_UINT64                ui64MemUsageIORemapPTLMAMax;           /*!< MemoryUsageIORemapPTLMAMax */
    MT_UINT64                ui64MemUsageAllocGPUMemLMA;            /*!< MemoryUsageAllocGPUMemLMA */
    MT_UINT64                ui64MemUsageAllocGPUMemLMAMax;         /*!< MemoryUsageAllocGPUMemLMAMax */
    MT_UINT64                ui64MemUsageAllocGPUMemUMA;            /*!< MemoryUsageAllocGPUMemUMA */
    MT_UINT64                ui64MemUsageAllocGPUMemUMAMax;         /*!< MemoryUsageAllocGPUMemUMAMax */
    MT_UINT64                ui64MemUsageAllocGPUMemUMAPool;        /*!< MemoryUsageAllocGPUMemUMAPool */
    MT_UINT64                ui64MemUsageAllocGPUMemUMAPoolMax;     /*!< MemoryUsageAllocGPUMemUMAPoolMax */
    MT_UINT64                ui64MemUsageMappedGPUMemUMALMA;        /*!< MemoryUsageMappedGPUMemUMA/LMA */
    MT_UINT64                ui64MemUsageMappedGPUMemUMALMAMax;     /*!< MemoryUsageMappedGPUMemUMA/LMAMax */
    MT_UINT64                ui64MemUsageDmaBufImport;              /*!< MemoryUsageDmaBufImport */
    MT_UINT64                ui64MemUsageDmaBufImportMax;           /*!< MemoryUsageDmaBufImportMax */
    MT_UINT64                ui64MemUsageAllocSystemMem;            /*!< MemoryUsageAllocSystemMem */
    MT_UINT64                ui64MemUsageAllocSystemMemMax;         /*!< MemoryUsageAllocSystemMemMax */
} DEVICE_DRIVER_STATS_INFO;

/* WARNING: MKIS Managed! */
typedef struct _PROCESS_DRIVER_STATS_INFO_BASE_ {
	MT_UINT64		ui64MemUsageAllocPTMemoryUMA;
	MT_UINT64		ui64MemUsageAllocPTMemoryUMAMax;
	MT_UINT64		ui64MemUsageAllocPTMemLMA;
	MT_UINT64		ui64MemUsageAllocPTMemLMAMax;
	MT_UINT64		ui64MemUsageAllocGPUMemLMA;
	MT_UINT64		ui64MemUsageAllocGPUMemLMAMax;
	MT_UINT64		ui64MemUsageAllocGPUMemUMA;
	MT_UINT64		ui64MemUsageAllocGPUMemUMAMax;
	MT_UINT64		ui64MemoryUsageAllocGPUMemSystem;
	MT_UINT64		ui64MemoryUsageAllocGPUMemSystemMax;
	MT_UINT64		ui64MemoryUsageAllocGPUMemUser;
	MT_UINT64		ui64MemoryUsageAllocGPUMemUserMax;
} PROCESS_DRIVER_STATS_INFO_BASE;

/* WARNING: MKIS Managed! */
typedef struct _PROCESS_DRIVER_STATS_INFO_ {
	/* i32OsDeviceID, ui32InternalID: Which device(GPU) does the process belong to */
	MT_INT32		i32OsDeviceID;	/* OUT */
	MT_UINT32		ui32InternalID;	/* OUT */
	MT_PID			pid;	/* OUT */
	PROCESS_DRIVER_STATS_INFO_BASE sDevProcessStatsInfo;
} PROCESS_DRIVER_STATS_INFO;

/*! *@Brief driver stats of process level structure with the caller */
/* WARNING: MKIS Managed! */
typedef struct _PROCESS_DRIVER_STATS_INFO_TOTAL_ {
	MT_UINT32 ui32ProcessNum;	/*!< process counts */
	PROCESS_DRIVER_STATS_INFO processStatsInfo[PROCESS_NUMS_MAX];
} PROCESS_DRIVER_STATS_INFO_TOTAL;

/*! *@Brief driver stats of process level structure with the caller include osType*/
/* WARNING: MKIS Managed! */
typedef struct _PROCESS_DRIVER_STATS_OS_TYPE_INFO_TOTAL_ {
	MT_UINT32 ui32OsType;	/*!< [in] host:0, container:1 */
	PROCESS_DRIVER_STATS_INFO_TOTAL processInfo;
} PROCESS_DRIVER_STATS_OS_TYPE_INFO_TOTAL;
/*
 * MKIS Managed IOCTL Interface structures end
 */

/*! *@Brief gpu util type with the caller */
typedef enum _GPU_UTIL_TYPE_ {

	GPU_UTIL_2D = 0x1,
	GPU_UTIL_TA = 0x2,
	GPU_UTIL_3D = 0x3,
	GPU_UTIL_CMP = 0x4,
	GPU_UTIL_CE  = 0x5,
	GPU_UTIL_TRANSFER = 0x7,

	GPU_UTIL_LAST
} GPU_UTIL_TYPE;

/*! *@Brief gpu util of process level structure with the caller */
typedef struct _PROCESS_GPU_UTIL_CELL_INFO_ {
	MT_PID         pid;                /* OUT */
	MT_UINT64      ui64MinTimeStamp;   /* OUT */
	MT_UINT64      ui64MaxTimeStamp;   /* OUT */
	MT_UINT8       asui8GpuUtil[GPU_UTIL_LAST];   /*!< OUT, detailed GPU Utilisation */
	MT_UINT8       ui8TotalGpuUtil;   /*!< OUT, total GPU Utilisation */
} PROCESS_GPU_UTIL_CELL_INFO;

typedef enum _PROCESS_GPU_UTIL_CONNECT_STATUS_ {
	UNCONNECTION,
	CONNECTION
} PROCESS_GPU_UTIL_CONNECT_STATUS;

typedef enum _PROCESS_GPU_UTIL_USE_MODE_ {
	OPEN_CONNECTION,
	QUERY_STATUS,
	QUERY_DATA,
	CLOSE_CONNECTION
} PROCESS_GPU_UTIL_USE_MODE;

/*! *@Brief total gpu util of process level structure with the caller */
typedef struct _PROCESS_GPU_UTIL_INFO_TOTAL_ {
	/*!< IN,  when eUseMode == OPEN_CONNECTION:
		* 1. put in ui32OsType only
		* 2. put in ui32OsType, ui64PollingTimeIntervalMS, ui64SampleRateMS
	*/
	PROCESS_GPU_UTIL_USE_MODE   eUseMode;
	MT_UINT32                  ui32OsType;                /*!< IN,  host:0, container:1 */
	MT_UINT64                  ui64PollingTimeIntervalMS; /*!< IN,  Polling Time Interval, ms */
	MT_UINT64                  ui64SampleRateMS;          /*!< IN,  SampleRate, ms */
	MT_UINT64                  ui64LastSeenTimeStamp;     /*!< IN,  sample timestamp greater than ui64LastSeenTimeStamp */

	PROCESS_GPU_UTIL_CONNECT_STATUS eConnectStatus;        /* OUT */
	/* i32OsDeviceID, ui32InternalID: Which device(GPU) does the process belong to */
	MT_INT32                   i32OsDeviceID;             /* OUT */
	MT_UINT32                  ui32InternalID;            /* OUT */
	MT_UINT32                  ui32ProcessNum;            /*!< OUT, process counts */
	PROCESS_GPU_UTIL_CELL_INFO  asProcessGpuUtilBaseInfo[PROCESS_NUMS_MAX]; /*!< OUT */
} PROCESS_GPU_UTIL_INFO_TOTAL;

#ifdef __cplusplus
extern "C"
{
#endif

MTGPU_ERROR MUSAClearMUSAStatus(MTGPU_DEVICE_NODE *psDeviceNode, MT_UINT64 ui64BitMask);

/*!
******************************************************************************
 @Function GpuStatusExport

 @Description
 output information about GPU utilization etc.

 @Input psDeviceNode: Pointer to device node struct.
 @Output psDeviceStatusInfo: GPU utilization etc.

 @Return MTGPU_OK upon success and MTGPU_ERROR otherwise.
******************************************************************************/ 
MTGPU_ERROR GpuStatusExport(MTGPU_DEVICE_NODE *psDeviceNode,
                             DEVICE_STATUS_INFO *psDeviceStatusInfo);

/*!
******************************************************************************
 @Function GpuMemStatExport

 @Description
 output information about GPU memory.

 @Input psDeviceNode: Pointer to device node struct.
 @Output psDeviceDriverStatsInfo: GPU memory.

 @Return MTGPU_OK upon success and MTGPU_ERROR otherwise.
******************************************************************************/ 
MTGPU_ERROR GpuMemStatExport(MTGPU_DEVICE_NODE *psDeviceNode, 
                              DEVICE_DRIVER_STATS_INFO *psDeviceDriverStatsInfo);

MTGPU_ERROR GpuProcessMemStatExport(MTGPU_DEVICE_NODE *psDeviceNode, MT_UINT32 ui32OsType,
				     PROCESS_DRIVER_STATS_INFO_TOTAL *psGpuProcessMemStatsInfo);

MTGPU_ERROR GpuProcessUtilExport(MTGPU_DEVICE_NODE *psDeviceNode,
				  PROCESS_GPU_UTIL_INFO_TOTAL *psGpuProcessUtilInfo);

#ifdef __cplusplus
}
#endif

#endif /* UAPI_COMMON_H */

/******************************************************************************
 End of file (uapi_common.h)
******************************************************************************/

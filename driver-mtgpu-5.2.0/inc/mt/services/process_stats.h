/*************************************************************************/ /*!
@File
@Title          Functions for creating and reading proc filesystem entries.
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
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

#ifndef PROCESS_STATS_H
#define PROCESS_STATS_H

#include <mt/mem_types.h>

#include "mtgpu_error.h"
#include "allocmem.h"
#include "cache_ops.h"
#include "device.h"
#include "osdi_impl.h"

/*
 * The publishing of Process Stats is controlled by the
 * MTGPU_ENABLE_PROCESS_STATS build option. The recording of all Memory
 * allocations is controlled by the MTGPU_ENABLE_MEMORY_STATS build option.
 *
 * Note: There will be a performance degradation with memory allocation
 *       recording enabled!
 */


/*
 * Memory types which can be tracked...
 */
typedef enum {
	MTGPU_MEM_ALLOC_TYPE_KMALLOC,				/* memory allocated by kmalloc() */
	MTGPU_MEM_ALLOC_TYPE_VMALLOC,				/* memory allocated by vmalloc() */
	MTGPU_MEM_ALLOC_TYPE_ALLOC_PAGES_PT_UMA,	/* pages allocated from UMA to hold page table information */
	MTGPU_MEM_ALLOC_TYPE_VMAP_PT_UMA,			/* ALLOC_PAGES_PT_UMA mapped to kernel address space */
	MTGPU_MEM_ALLOC_TYPE_ALLOC_PAGES_PT_LMA,	/* pages allocated from LMA to hold page table information */
	MTGPU_MEM_ALLOC_TYPE_IOREMAP_PT_LMA,		/* ALLOC_PAGES_PT_LMA mapped to kernel address space */
	MTGPU_MEM_ALLOC_TYPE_ALLOC_LMA_PAGES,		/* pages allocated from LMA */
	MTGPU_MEM_ALLOC_TYPE_ALLOC_UMA_PAGES,		/* pages allocated from UMA */
	MTGPU_MEM_ALLOC_TYPE_MAP_UMA_LMA_PAGES,	/* mapped UMA/LMA pages */
	MTGPU_MEM_ALLOC_TYPE_UMA_POOL_PAGES,		/* pages in the page pool */
	MTGPU_MEM_ALLOC_TYPE_DMA_BUF_IMPORT,       /* dma-buf imports */
	MTGPU_MEM_ALLOC_TYPE_ALLOC_SYSTEM_PAGES,   /* pages allocated from sysm memory pages */
	MTGPU_MEM_ALLOC_TYPE_ALLOC_USER_PAGES, /* pages allocated from user memory pages */
	/* Must be the last enum...*/
	MTGPU_MEM_ALLOC_TYPE_COUNT
} MTGPU_MEM_ALLOC_TYPE;


/*
 * Functions for managing the processes recorded...
 */
MTGPU_ERROR MTGPUStatsInitialise(void);
void MTGPUStatsDestroy(void);

MTGPU_ERROR MTGPUStatsRegisterProcess(MT_HANDLE *phProcessStats, MTGPU_DEVICE_NODE *psDevNode);

void MTGPUStatsDeregisterProcess(MT_HANDLE hProcessStats);

#define MAX_POWER_STAT_ENTRIES		51

/*
 * Functions for recording the statistics...
 */

void MTGPUStatsAddMemAllocRecord(MTGPU_MEM_ALLOC_TYPE eAllocType,
				  void *pvCpuVAddr,
				  MT_CPU_PHYADDR sCpuPAddr,
				  size_t uiBytes,
				  void *pvPrivateData,
				  MT_PID uiPid,
				  MTGPU_DEVICE_NODE *psDeivceNode
				  DEBUG_MEMSTATS_PARAMS);

void MTGPUStatsRemoveMemAllocRecord(MTGPU_MEM_ALLOC_TYPE eAllocType,
				     MT_UINT64 ui64Key,
				     MT_PID uiPid,
				     MTGPU_DEVICE_NODE *psDeivceNode,
				     size_t uiBytes);

void MTGPUStatsIncrMemAllocStat(MTGPU_MEM_ALLOC_TYPE eAllocType,
				 size_t uiBytes,
				 MT_PID uiPid,
				 MTGPU_DEVICE_NODE *psDeivceNode);

/*
 * Increases the memory stat for eAllocType. Tracks the allocation size value
 * by inserting a value into a hash table with uiCpuVAddr as key.
 * Pair with MTGPUStatsDecrMemAllocStatAndUntrack().
 */
void MTGPUStatsIncrMemAllocStatAndTrack(MTGPU_MEM_ALLOC_TYPE eAllocType,
					 size_t uiBytes,
					 MT_UINT64 uiCpuVAddr,
					 MT_PID uiPid,
					 MTGPU_DEVICE_NODE *psDeivceNode);

void MTGPUStatsDecrMemAllocStat(MTGPU_MEM_ALLOC_TYPE eAllocType,
				 size_t uiBytes,
				 MT_PID uiPid,
				 MTGPU_DEVICE_NODE *psDeivceNode);

void MTGPUStatsDecrMemKAllocStat(size_t uiBytes,
								  MT_PID decrPID,
								  MTGPU_DEVICE_NODE *psDeivceNode);

/*
 * Decrease the memory stat for eAllocType. Takes the allocation size value
 * from the hash table with uiCpuVAddr as key.
 * Pair with MTGPUStatsIncrMemAllocStatAndTrack().
 */
void MTGPUStatsDecrMemAllocStatAndUntrack(MTGPU_MEM_ALLOC_TYPE eAllocType,
					   MT_UINT64 uiCpuVAddr,
					   MTGPU_DEVICE_NODE *psDeivceNode);

void
MTGPUStatsIncrMemAllocPoolStat(size_t uiBytes);

void
MTGPUStatsDecrMemAllocPoolStat(size_t uiBytes);

void
MTGPUStatsUpdateOOMStats(MT_UINT32 ui32OOMStatType,
			  MT_PID pidOwner,
			  MTGPU_DEVICE_NODE *psDeviceNode);

MTGPU_ERROR
MTGPUServerUpdateOOMStats(MT_UINT32 ui32OOMStatType,
			   MT_PID pidOwner,
			   MTGPU_DEVICE_NODE *psDeviceNode);

void MTGPUStatsUpdateRenderContextStats(MT_UINT32 ui32TotalNumPartialRenders,
										 MT_UINT32 ui32TotalNumOutOfMemory,
										 MT_UINT32 ui32TotalTAStores,
										 MT_UINT32 ui32Total3DStores,
										 MT_UINT32 ui32TotalCMPStores,
										 MT_UINT32 ui32TotalTRANSFERStores,
										 MT_PID owner,
										 MTGPU_DEVICE_NODE *psDeviceNode);

void MTGPUStatsUpdateZSBufferStats(MT_UINT32 ui32NumReqByApp,
									MT_UINT32 ui32NumReqByFW,
									MT_PID owner,
									MTGPU_DEVICE_NODE *psDeviceNode);

void MTGPUStatsUpdateFreelistStats(MT_UINT32 ui32NumGrowReqByApp,
									MT_UINT32 ui32NumGrowReqByFW,
									MT_UINT32 ui32InitFLPages,
									MT_UINT32 ui32NumHighPages,
									MT_PID    ownerPid,
									MTGPU_DEVICE_NODE *psDeviceNode);
#if defined(MTGPU_ENABLE_CACHEOP_STATS)
void MTGPUStatsUpdateCacheOpStats(MTGPU_CACHE_OP uiCacheOp,
#if defined(MTGPU_ENABLE_GPU_MEMORY_INFO) && defined(DEBUG)
								   uint64_t sDevVAddr,
								   MT_DEV_PHYADDR sDevPAddr,
#endif
								   MT_DEVMEM_SIZE_T uiOffset,
								   MT_DEVMEM_SIZE_T uiSize,
								   MT_UINT64 ui64ExecuteTimeMs,
								   MT_BOOL bUserModeFlush,
								   MT_PID ownerPid,
								   MTGPU_DEVICE_NODE *psDeviceNode);
#endif

#if defined(MTGPU_ENABLE_PROCESS_STATS)
/* Update pre/post power transition timing statistics */
void InsertPowerTimeStatistic(MT_UINT64 ui64SysStartTime, MT_UINT64 ui64SysEndTime,
                              MT_UINT64 ui64DevStartTime, MT_UINT64 ui64DevEndTime,
                              MT_BOOL bForced, MT_BOOL bPowerOn, MT_BOOL bPrePower);

void InsertPowerTimeStatisticExtraPre(MT_UINT64 ui64StartTimer, MT_UINT64 ui64Stoptimer);
void InsertPowerTimeStatisticExtraPost(MT_UINT64 ui64StartTimer, MT_UINT64 ui64StopTimer);
#else
/* Update pre/post power transition timing statistics */
static inline
void InsertPowerTimeStatistic(MT_UINT64 ui64SysStartTime, MT_UINT64 ui64SysEndTime,
                              MT_UINT64 ui64DevStartTime, MT_UINT64 ui64DevEndTime,
                              MT_BOOL bForced, MT_BOOL bPowerOn, MT_BOOL bPrePower) {}
static inline
void InsertPowerTimeStatisticExtraPre(MT_UINT64 ui64StartTimer, MT_UINT64 ui64Stoptimer) {}

static inline
void InsertPowerTimeStatisticExtraPost(MT_UINT64 ui64StartTimer, MT_UINT64 ui64StopTimer) {}
#endif

void SetFirmwareStartTime(MT_UINT32 ui32TimeStamp);

void SetFirmwareHandshakeIdleTime(MT_UINT64 ui64Duration);

/* Functions used for calculating the memory usage statistics of a process */
MTGPU_ERROR MTGPUFindProcessMemStats(MT_PID pid, MT_UINT32 ui32ArrSize,
				       MT_BOOL bAllProcessStats,
				       MT_UINT64 *pui64MemoryStats,
				       MTGPU_DEVICE_NODE *psDeviceNode);

typedef struct {
	MT_UINT32 ui32Pid;
	MT_UINT32 ui32KernelMemUsage;
	MT_UINT32 ui32GraphicsMemUsage;
} MTGPU_PER_PROCESS_MEM_USAGE;

MTGPU_ERROR MTGPUGetProcessMemUsage(MT_UINT32 *pui32TotalMem,
									  MT_UINT32 *pui32NumberOfLivePids,
									  MTGPU_PER_PROCESS_MEM_USAGE **ppsPerProcessMemUsageData);

int GpuMemStatDumpShow(MTGPU_DEVICE_NODE *psDeviceNode, OSDI_IMPL_ENTRY *psEntry);

#if (MUSA_NUM_OS_SUPPORTED > 1)
/* Functions used to update the host and guest LMA memory usage */
void VgpuIncreaseStatValue(MTGPU_DEVICE_NODE *psDeviceNode, size_t byte);
void VgpuDecreaseStatValue(MTGPU_DEVICE_NODE *psDeviceNode, size_t byte);
MT_UINT64 VgpuGetHostUsedLMAMem(MTGPU_DEVICE_NODE *psDeviceNode);
#endif

#endif /* PROCESS_STATS_H */

/*************************************************************************/ /*!
@File
@Title          Physical heap management header
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Defines the interface for the physical heap management
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

#include "img_types.h"
#include "mtgpu_error.h"
#include "mtgpu_memallocflags.h"
#include "devicemem_typedefs.h"
#include "opaque_types.h"
#include "pmr_impl.h"
#include "physheap_config.h"
#include "mtgpu_segment.h"
#include "mt_notifier.h"

#ifndef PHYSHEAP_H
#define PHYSHEAP_H

typedef struct _PHYS_HEAP_ PHYS_HEAP;
#define INVALID_PHYS_HEAP 0xDEADDEAD

struct _CONNECTION_DATA_;

typedef struct _PG_HANDLE_
{
	union
	{
		void *pvHandle;
		MT_UINT64 ui64Handle;
	}u;
	/* The allocation order is log2 value of the number of pages to allocate.
	 * As such this is a correspondingly small value. E.g, for order 4 we
	 * are talking 2^4 * PAGE_SIZE contiguous allocation.
	 * DevPxAlloc API does not need to support orders higher than 4.
	 */
#if defined(SUPPORT_GPUVIRT_VALIDATION)
	MT_BYTE    uiOrder;    /* Order of the corresponding allocation */
	MT_BYTE    uiOSid;     /* OSid to use for allocation arena.
	                         * Connection-specific. */
	MT_BYTE    uiPad1,
	            uiPad2;     /* Spare */
#else
	MT_BYTE    uiOrder;    /* Order of the corresponding allocation */
	MT_BYTE    uiPad1,
	            uiPad2,
	            uiPad3;     /* Spare */
#endif
} PG_HANDLE;

/*! Pointer to private implementation specific data */
typedef void *PHEAP_IMPL_DATA;

/*************************************************************************/ /*!
@Function       Callback function PFN_DESTROY_DATA
@Description    Destroy private implementation specific data.
@Input          PHEAP_IMPL_DATA    Pointer to implementation data.
*/ /**************************************************************************/
typedef void (*PFN_DESTROY_DATA)(PHEAP_IMPL_DATA);
/*************************************************************************/ /*!
@Function       Callback function PFN_GET_DEV_PADDR
@Description    Get heap device physical address.
@Input          PHEAP_IMPL_DATA    Pointer to implementation data.
@Output         MT_DEV_PHYADDR    Device physical address.
@Return         MTGPU_ERROR       MTGPU_OK or error code
*/ /**************************************************************************/
typedef MTGPU_ERROR (*PFN_GET_DEV_PADDR)(PHEAP_IMPL_DATA, MT_DEV_PHYADDR*);
/*************************************************************************/ /*!
@Function       Callback function PFN_GET_CPU_PADDR
@Description    Get heap CPU physical address.
@Input          PHEAP_IMPL_DATA    Pointer to implementation data.
@Output         MT_CPU_PHYADDR    CPU physical address.
@Return         MTGPU_ERROR       MTGPU_OK or error code
*/ /**************************************************************************/
typedef MTGPU_ERROR (*PFN_GET_CPU_PADDR)(PHEAP_IMPL_DATA, MT_CPU_PHYADDR*);
/*************************************************************************/ /*!
@Function       Callback function PFN_GET_SIZE
@Description    Get size of heap.
@Input          PHEAP_IMPL_DATA    Pointer to implementation data.
@Output         MT_UINT64         Size of heap.
@Return         MTGPU_ERROR       MTGPU_OK or error code
*/ /**************************************************************************/
typedef MTGPU_ERROR (*PFN_GET_SIZE)(PHEAP_IMPL_DATA, MT_UINT64*);
/*************************************************************************/ /*!
@Function       Callback function PFN_GET_PAGE_SHIFT
@Description    Get heap log2 page shift.
@Return         MT_UINT32         Log2 page shift
*/ /**************************************************************************/
typedef MT_UINT32 (*PFN_GET_PAGE_SHIFT)(void);

/*************************************************************************/ /*!
@Function       Callback function PFN_GET_MEM_STATS
@Description    Get total and free memory size of the physical heap managed by
                the PMR Factory.
@Input          PHEAP_IMPL_DATA    Pointer to implementation data.
@Output         MT_UINT64         total Size of heap.
@Output         MT_UINT64         free Size available in a heap.
@Return         none
*/ /**************************************************************************/
typedef void (*PFN_GET_MEM_STATS)(PHEAP_IMPL_DATA, MT_UINT64 *, MT_UINT64 *);
typedef void (*PFN_GET_SEGMENT_MEM_STATS)(PHYS_HEAP *psPhysHeap,
					  MT_INT32 i32SegmentID,
					  struct mtgpu_segment_stats *psStats);

#if defined(SUPPORT_GPUVIRT_VALIDATION)
typedef MTGPU_ERROR (*PFN_PAGES_ALLOC_GPV)(PHYS_HEAP *psPhysHeap, size_t uiSize,
                                            PG_HANDLE *psMemHandle, MT_DEV_PHYADDR *psDevPAddr,
                                            MT_UINT32 ui32OSid, MT_PID uiPid);
#endif
typedef MTGPU_ERROR (*PFN_PAGES_ALLOC)(PHYS_HEAP *psPhysHeap, size_t uiSize,
                                        PG_HANDLE *psMemHandle, MT_DEV_PHYADDR *psDevPAddr,
					MT_PID uiPid, MT_UINT32 uiVmCtxFlag);

typedef void (*PFN_PAGES_FREE)(PHYS_HEAP *psPhysHeap, PG_HANDLE *psMemHandle);

typedef MTGPU_ERROR (*PFN_PAGES_MAP)(PHYS_HEAP *psPhysHeap, PG_HANDLE *pshMemHandle,
                                      size_t uiSize, MT_DEV_PHYADDR *psDevPAddr,
				     void **pvPtr, MT_UINT32 uiVmCtxFlag);

typedef void (*PFN_PAGES_UNMAP)(PHYS_HEAP *psPhysHeap,
                                PG_HANDLE *psMemHandle, void *pvPtr);

typedef MTGPU_ERROR (*PFN_PAGES_CLEAN)(PHYS_HEAP *psPhysHeap,
                                        PG_HANDLE *pshMemHandle,
                                        MT_UINT32 uiOffset,
                                        MT_UINT32 uiLength);

/*************************************************************************/ /*!
@Function       Callback function PFN_CREATE_PMR
@Description    Create a PMR physical allocation and back with RAM on creation,
                if required. The RAM page comes either directly from
                the Phys Heap's associated pool of memory or from an OS API.
@Input          psPhysHeap         Pointer to Phys Heap.
@Input          psConnection       Pointer to device connection.
@Input          uiSize             Allocation size.
@Input          uiChunkSize        Chunk size.
@Input          ui32NumPhysChunks  Physical chunk count.
@Input          ui32NumVirtChunks  Virtual chunk count.
@Input          pui32MappingTable  Mapping Table.
@Input          uiLog2PageSize     Page size.
@Input          uiFlags            Memalloc flags.
@Input          pszAnnotation      Annotation.
@Input          uiPid              Process ID.
@Output         ppsPMRPtr          Pointer to PMR.
@Input          ui32PDumpFlag      PDump flags.
@Return         MTGPU_ERROR       MTGPU_OK or error code
*/ /**************************************************************************/
typedef MTGPU_ERROR (*PFN_CREATE_PMR)(PHYS_HEAP *psPhysHeap,
									   struct _CONNECTION_DATA_ *psConnection,
									   MT_DEVMEM_SIZE_T uiSize,
									   MT_DEVMEM_SIZE_T uiChunkSize,
									   MT_UINT32 ui32NumPhysChunks,
									   MT_UINT32 ui32NumVirtChunks,
									   MT_UINT32 *pui32MappingTable,
									   MT_UINT32 uiLog2PageSize,
									   MTGPU_MEMALLOCFLAGS_T uiFlags,
									   const MT_CHAR *pszAnnotation,
									   MT_PID uiPid,
									   PMR **ppsPMRPtr,
									   MT_UINT32 ui32PDumpFlags);

/*! Implementation specific function table */
typedef struct PHEAP_IMPL_FUNCS_TAG
{
	PFN_DESTROY_DATA pfnDestroyData;
	PFN_GET_DEV_PADDR pfnGetDevPAddr;
	PFN_GET_CPU_PADDR pfnGetCPUPAddr;
	PFN_GET_SIZE pfnGetSize;
	PFN_GET_PAGE_SHIFT pfnGetPageShift;
	PFN_GET_MEM_STATS pfnGetPMRFactoryMemStats;
	PFN_CREATE_PMR pfnCreatePMR;
	PFN_GET_SEGMENT_MEM_STATS pfnGetSegmentMemStats;
#if defined(SUPPORT_GPUVIRT_VALIDATION)
	PFN_PAGES_ALLOC_GPV pfnPagesAllocGPV;
#endif
	PFN_PAGES_ALLOC pfnPagesAlloc;
	PFN_PAGES_FREE pfnPagesFree;
	PFN_PAGES_MAP pfnPagesMap;
	PFN_PAGES_UNMAP pfnPagesUnMap;
	PFN_PAGES_CLEAN pfnPagesClean;
} PHEAP_IMPL_FUNCS;

/*************************************************************************/ /*!
@Function       PhysHeapCreateDeviceHeapsFromConfigs
@Description    Create new heaps for a device from configs.
@Input          psDevNode      Pointer to device node struct
@Input          pasConfigs     Pointer to array of Heap configurations.
@Input          ui32NumConfigs Number of configurations in array.
@Return         MTGPU_ERROR MTGPU_OK or error code
*/ /**************************************************************************/
MTGPU_ERROR
PhysHeapCreateDeviceHeapsFromConfigs(PMTGPU_DEVICE_NODE psDevNode,
                                     PHYS_HEAP_CONFIG *pasConfigs,
                                     MT_UINT32 ui32NumConfigs);

/*************************************************************************/ /*!
@Function       PhysHeapCreateHeapFromConfig
@Description    Create a new heap. Calls specific heap API depending
                on heap type.
@Input          psDevNode    Pointer to device node struct.
@Input          psConfig     Heap configuration.
@Output         ppsPhysHeap  Pointer to the created heap.
@Return         MTGPU_ERROR MTGPU_OK or error code
*/ /**************************************************************************/
MTGPU_ERROR
PhysHeapCreateHeapFromConfig(PMTGPU_DEVICE_NODE psDevNode,
							 PHYS_HEAP_CONFIG *psConfig,
							 PHYS_HEAP **ppsPhysHeap);

/*************************************************************************/ /*!
@Function       PhysHeapCreate
@Description    Create a new heap. Allocated and stored internally.
                Destroy with PhysHeapDestroy when no longer required.
@Input          psDevNode    Pointer to device node struct
@Input          psConfig     Heap configuration.
@Input          pvImplData   Implementation specific data. Can be NULL.
@Input          psImplFuncs  Implementation specific function table. Must be
                             a valid pointer.
@Output         ppsPhysHeap  Pointer to the created heap. Must be a valid
                             pointer.
@Return         MTGPU_ERROR MTGPU_OK or error code
*/ /**************************************************************************/
MTGPU_ERROR PhysHeapCreate(PMTGPU_DEVICE_NODE psDevNode,
							PHYS_HEAP_CONFIG *psConfig,
							PHEAP_IMPL_DATA pvImplData,
							PHEAP_IMPL_FUNCS *psImplFuncs,
							PHYS_HEAP **ppsPhysHeap);

/*************************************************************************/ /*!
@Function       PhysHeapDestroyDeviceHeaps
@Description    Destroys all heaps referenced by a device.
@Input          psDevNode Pointer to a device node struct.
@Return         void
*/ /**************************************************************************/
void PhysHeapDestroyDeviceHeaps(PMTGPU_DEVICE_NODE psDevNode);

void PhysHeapDestroy(PHYS_HEAP *psPhysHeap);

MTGPU_ERROR PhysHeapAcquire(PHYS_HEAP *psPhysHeap);

/*************************************************************************/ /*!
@Function       PhysHeapAcquireByUsage
@Description    Acquire PhysHeap by usage flag.
@Input          ui32UsageFlag PhysHeap usage flag
@Input          psDevNode     Pointer to device node struct
@Output         ppsPhysHeap   PhysHeap if found.
@Return         MTGPU_ERROR MTGPU_OK or error code
*/ /**************************************************************************/
MTGPU_ERROR PhysHeapAcquireByUsage(PHYS_HEAP_USAGE_FLAGS ui32UsageFlag,
									PMTGPU_DEVICE_NODE psDevNode,
									PHYS_HEAP **ppsPhysHeap);

/*************************************************************************/ /*!
@Function       PhysHeapAcquireByDevPhysHeap
@Description    Acquire PhysHeap by DevPhysHeap.
@Input          eDevPhysHeap Device Phys Heap.
@Input          psDevNode    Pointer to device node struct
@Output         ppsPhysHeap  PhysHeap if found.
@Return         MTGPU_ERROR MTGPU_OK or error code
*/ /**************************************************************************/
MTGPU_ERROR PhysHeapAcquireByDevPhysHeap(MTGPU_PHYS_HEAP eDevPhysHeap,
										  PMTGPU_DEVICE_NODE psDevNode,
										  PHYS_HEAP **ppsPhysHeap);

void PhysHeapRelease(PHYS_HEAP *psPhysHeap);

/*************************************************************************/ /*!
@Function       PhysHeapGetImplData
@Description    Get physical heap implementation specific data.
@Input          psPhysHeap   Pointer to physical heap.
@Input          psConfig     Heap configuration.
@Return         pvImplData   Implementation specific data. Can be NULL.
*/ /**************************************************************************/
PHEAP_IMPL_DATA PhysHeapGetImplData(PHYS_HEAP *psPhysHeap);

PHYS_HEAP_TYPE PhysHeapGetType(PHYS_HEAP *psPhysHeap);

/*************************************************************************/ /*!
@Function       PhysHeapGetFlags
@Description    Get phys heap usage flags.
@Input          psPhysHeap   Pointer to physical heap.
@Return         PHYS_HEAP_USAGE_FLAGS Phys heap usage flags.
*/ /**************************************************************************/
PHYS_HEAP_USAGE_FLAGS PhysHeapGetFlags(PHYS_HEAP *psPhysHeap);

MT_BOOL PhysHeapValidateDefaultHeapExists(PMTGPU_DEVICE_NODE psDevNode);

MT_BOOL PhysHeapIsUMA(PHYS_HEAP *psPhysHeap);

MTGPU_ERROR PhysHeapGetCpuPAddr(PHYS_HEAP *psPhysHeap,
									   MT_CPU_PHYADDR *psCpuPAddr);


MTGPU_ERROR PhysHeapGetSize(PHYS_HEAP *psPhysHeap,
								   MT_UINT64 *puiSize);

/*************************************************************************/ /*!
@Function       MTGPUGetDevicePhysHeapCount
@Description    Get the physical heap count supported by the device.
@Input          psDevNode   Device node, the heap count is requested for.
@Output         pui32PhysHeapCount  Buffer that holds the heap count
@Return         None
*/ /**************************************************************************/
void MTGPUGetDevicePhysHeapCount(PMTGPU_DEVICE_NODE psDevNode,
								  MT_UINT32 *pui32PhysHeapCount);

/*************************************************************************/ /*!
@Function       PhysHeapGetMemInfo
@Description    Get phys heap memory statistics for a given physical heap ID.
@Input          psDevNode          Pointer to device node struct
@Input          ui32PhysHeapCount  Physical heap count
@Input          paePhysHeapID      Physical heap ID
@Output         paPhysHeapMemStats Buffer that holds the memory statistics
@Return         MTGPU_ERROR MTGPU_OK or error code
*/ /**************************************************************************/
MTGPU_ERROR
PhysHeapGetMemInfo(PMTGPU_DEVICE_NODE psDevNode,
				   MT_UINT32 ui32PhysHeapCount,
				   MTGPU_PHYS_HEAP *paePhysHeapID,
				   PHYS_HEAP_MEM_STATS_PTR paPhysHeapMemStats);

/*************************************************************************/ /*!
@Function       PhysHeapGetMemInfoPkd
@Description    Get phys heap memory statistics for a given physical heap ID.
@Input          psDevNode          Pointer to device node struct
@Input          ui32PhysHeapCount  Physical heap count
@Input          paePhysHeapID      Physical heap ID
@Output         paPhysHeapMemStats Buffer that holds the memory statistics
@Return         MTGPU_ERROR MTGPU_OK or error code
*/ /**************************************************************************/
MTGPU_ERROR
PhysHeapGetMemInfoPkd(PMTGPU_DEVICE_NODE psDevNode,
					  MT_UINT32 ui32PhysHeapCount,
					  MTGPU_PHYS_HEAP *paePhysHeapID,
					  PHYS_HEAP_MEM_STATS_PKD_PTR paPhysHeapMemStats);

void MTGPUGetHeapSegmentStats(PMTGPU_DEVICE_NODE psDeviceNode,
			       MT_INT32 i32SegmentID,
			       struct mtgpu_segment_stats *psStats);

/*************************************************************************/ /*!
@Function       PhysheapGetPhysMemUsage
@Description    Get memory statistics for a given physical heap.
@Input          psPhysHeap      Physical heap
@Output         pui64TotalSize  Buffer that holds the total memory size of the
                                given physical heap.
@Output         pui64FreeSize   Buffer that holds the free memory available in
                                a given physical heap.
@Return         none
*/ /**************************************************************************/
void PhysheapGetPhysMemUsage(PHYS_HEAP *psPhysHeap,
							 MT_UINT64 *pui64TotalSize,
							 MT_UINT64 *pui64FreeSize);

MTGPU_ERROR PhysHeapGetDevPAddr(PHYS_HEAP *psPhysHeap,
								 MT_DEV_PHYADDR *psDevPAddr);

void PhysHeapCpuPAddrToDevPAddr(PHYS_HEAP *psPhysHeap,
								MT_UINT32 ui32NumOfAddr,
								MT_DEV_PHYADDR *psDevPAddr,
								MT_CPU_PHYADDR *psCpuPAddr);

void PhysHeapDevPAddrToCpuPAddr(PHYS_HEAP *psPhysHeap,
								MT_UINT32 ui32NumOfAddr,
								MT_CPU_PHYADDR *psCpuPAddr,
								MT_DEV_PHYADDR *psDevPAddr);


/*************************************************************************/ /*!
@Function       PhysHeapCreatePMR
@Description    Function calls an implementation-specific function pointer.
                See function pointer for details.
@Return         MTGPU_ERROR       MTGPU_OK or error code
*/ /**************************************************************************/
MTGPU_ERROR PhysHeapCreatePMR(PHYS_HEAP *psPhysHeap,
							   struct _CONNECTION_DATA_ *psConnection,
							   MT_DEVMEM_SIZE_T uiSize,
							   MT_DEVMEM_SIZE_T uiChunkSize,
							   MT_UINT32 ui32NumPhysChunks,
							   MT_UINT32 ui32NumVirtChunks,
							   MT_UINT32 *pui32MappingTable,
							   MT_UINT32 uiLog2PageSize,
							   MTGPU_MEMALLOCFLAGS_T uiFlags,
							   const MT_CHAR *pszAnnotation,
							   MT_PID uiPid,
							   PMR **ppsPMRPtr,
							   MT_UINT32 ui32PDumpFlags);

SEGMENT_INFO *PhysHeapGetSegmentInfo(PHYS_HEAP *psPhysHeap);

MTGPU_ERROR PhysHeapInit(void);
void PhysHeapDeinit(void);

/*************************************************************************/ /*!
@Function       PhysHeapDeviceNode
@Description    Get pointer to the device node this heap belongs to.
@Input          psPhysHeap          Pointer to physical heap.
@Return         PMTGPU_DEVICE_NODE Pointer to device node.
*/ /**************************************************************************/
PMTGPU_DEVICE_NODE PhysHeapDeviceNode(PHYS_HEAP *psPhysHeap);

struct device *PhysHeapDeviceOSDevice(PHYS_HEAP *psPhysHeap);

/*************************************************************************/ /*!
@Function       PhysHeapMTLayerAcquire
@Description    Is phys heap to be acquired in MT layer?
@Input          ePhysHeap           phys heap
@Return         MT_BOOL            return MT_TRUE if yes
*/ /**************************************************************************/
MT_BOOL PhysHeapMTLayerAcquire(MTGPU_PHYS_HEAP ePhysHeap);

/*************************************************************************/ /*!
@Function       PhysHeapUserModeAlloc
@Description    Is allocation from UM allowed?
@Input          ePhysHeap           phys heap
@Return         MT_BOOL            return MT_TRUE if yes
*/ /**************************************************************************/
MT_BOOL PhysHeapUserModeAlloc(MTGPU_PHYS_HEAP ePhysHeap);

/*************************************************************************/ /*!
@Function       PhysHeapMMUPxSetup
@Description    Setup MMU Px allocation function pointers.
@Input          psDeviceNode Pointer to device node struct
@Return         MTGPU_ERROR MTGPU_OK on success.
*/ /**************************************************************************/
MTGPU_ERROR PhysHeapMMUPxSetup(PMTGPU_DEVICE_NODE psDeviceNode);

/*************************************************************************/ /*!
@Function       PhysHeapMMUPxDeInit
@Description    Deinit after PhysHeapMMUPxSetup.
@Input          psDeviceNode Pointer to device node struct
*/ /**************************************************************************/
void PhysHeapMMUPxDeInit(PMTGPU_DEVICE_NODE psDeviceNode);

#if defined(SUPPORT_GPUVIRT_VALIDATION)
MTGPU_ERROR PhysHeapPagesAllocGPV(PHYS_HEAP *psPhysHeap,
                                   size_t uiSize,
                                   PG_HANDLE *psMemHandle,
                                   MT_DEV_PHYADDR *psDevPAddr,
                                   MT_UINT32 ui32OSid, MT_PID uiPid);
#endif

MTGPU_ERROR PhysHeapPagesAlloc(PHYS_HEAP *psPhysHeap,
                                size_t uiSize,
                                PG_HANDLE *psMemHandle,
                                MT_DEV_PHYADDR *psDevPAddr,
				MT_PID uiPid,
				MT_UINT32 uiVmCtxFlag);

void PhysHeapPagesFree(PHYS_HEAP *psPhysHeap,
                       PG_HANDLE *psMemHandle);

MTGPU_ERROR PhysHeapPagesMap(PHYS_HEAP *psPhysHeap,
                              PG_HANDLE *pshMemHandle,
                              size_t uiSize,
                              MT_DEV_PHYADDR *psDevPAddr,
                              void **pvPtr,
			      MT_UINT32 uiVmCtxFlag);

void PhysHeapPagesUnMap(PHYS_HEAP *psPhysHeap,
                        PG_HANDLE *psMemHandle,
                        void *pvPtr);

MTGPU_ERROR PhysHeapPagesClean(PHYS_HEAP *psPhysHeap,
                                PG_HANDLE *pshMemHandle,
                                MT_UINT32 uiOffset,
                                MT_UINT32 uiLength);

/*************************************************************************/ /*!
@Function       PhysHeapGetPageShift
@Description    Get phys heap page shift.
@Input          psPhysHeap   Pointer to physical heap.
@Return         MT_UINT32   Log2 page shift
*/ /**************************************************************************/
MT_UINT32 PhysHeapGetPageShift(PHYS_HEAP *psPhysHeap);

void PhysHeapDebugDump(PMTGPU_DEVICE_NODE psDeviceNode,
		       DUMPDEBUG_PRINTF_FUNC *pfnDumpDebugPrintf,
		       void *pvDumpDebugFile);

MTGPU_ERROR acquireBackupAreasFromHeap(PHYS_HEAP *heap,
					MEM_REGION_INFO **ppsAreas,
					MT_INT32 *areasCnt);
void releaseBackupAreasFromHeap(PHYS_HEAP *heap, MEM_REGION_INFO *psAreas);

void PhysHeapSetRmGpuPhysHeap(PHYS_HEAP *psPhysHeap, uint64_t ui64RmGpuPhysHeap);
uint64_t PhysHeapGetRmGpuPhysHeap(PHYS_HEAP *psPhysHeap);
#endif /* PHYSHEAP_H */

/*
 * @Copyright   Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License     Dual MIT/GPLv2
 */

#ifndef _PHYSMEM_SYSMEM_H_
#define _PHYSMEM_SYSMEM_H_

#include "img_types.h"
#include "mtgpu_memallocflags.h"

struct page;

#ifdef CONFIG_ARM64
void PhysmemSetCpuPagesMapAttrCached(struct page **ppsPageArray,
				     MT_UINT32 ui32PageNum,
				     MT_UINT32 uiLog2PageSize,
				     MT_UINT32 *pui32MapTable);

void PhysmemSetCpuPagesMapAttrUncached(struct page **ppsPageArray,
				       MT_UINT32 ui32PageNum,
				       MT_UINT32 uiLog2PageSize,
				       MT_UINT32 *pui32MapTable);
#endif

MTGPU_ERROR
PhysmemCreateSystemMemPMR(PHYS_HEAP *psPhysHeap,
			  CONNECTION_DATA *psConnection,
			  MT_DEVMEM_SIZE_T uiSize,
			  MT_DEVMEM_SIZE_T uiChunkSize,
			  MT_UINT32 ui32NumPhysChunks,
			  MT_UINT32 ui32NumVirtChunks,
			  MT_UINT32 *pui32MappingTable,
			  MT_UINT32 uiLog2AllocPageSize,
			  MTGPU_MEMALLOCFLAGS_T uiFlags,
			  const MT_CHAR *pszAnnotation,
			  MT_PID uiPid,
			  PMR **ppsPMRPtr,
			  MT_UINT32 ui32PDumpFlags);

#endif

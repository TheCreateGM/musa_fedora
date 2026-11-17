/*
 * @Copyright   Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License     Dual MIT/GPLv2
 */

#ifndef _PHYSMEM_USERMEM_H
#define _PHYSMEM_USERMEM_H

#include "img_types.h"
#include "mtgpu_error.h"
#include "mtgpu_memallocflags.h"

MTGPU_ERROR
PhysmemCreateUserMemPMR(PHYS_HEAP *psPhysHeap,
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

#endif /* _PHYSMEM_USERMEM_H */

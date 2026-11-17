/*************************************************************************/ /*!
@File           ri_server.h
@Title          Resource Information abstraction
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Resource Information (RI) functions
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

#ifndef RI_SERVER_H
#define RI_SERVER_H

#include "img_defs.h"
#include "ri_typedefs.h"
#include "pmr.h"
#include "mtgpu_error.h"
#include "physheap.h"

MTGPU_ERROR RIInitKM(void);
void RIDeInitKM(void);

void RILockAcquireKM(void);
void RILockReleaseKM(void);

MTGPU_ERROR RIWritePMREntryKM(PMR *psPMR);

MTGPU_ERROR RIWritePMREntryWithOwnerKM(PMR *psPMR,
                                        MT_PID ui32Owner);

MTGPU_ERROR RIWriteMEMDESCEntryKM(PMR *psPMR,
                                   MT_UINT32 ui32TextBSize,
                                   const MT_CHAR ai8TextB[DEVMEM_ANNOTATION_MAX_LEN],
                                   MT_UINT64 uiOffset,
                                   MT_UINT64 uiSize,
                                   MT_BOOL bIsImport,
                                   MT_BOOL bIsSuballoc,
                                   RI_HANDLE *phRIHandle);

MTGPU_ERROR RIWriteProcListEntryKM(MT_UINT32 ui32TextBSize,
                                    const MT_CHAR *psz8TextB,
                                    MT_UINT64 ui64Size,
                                    MT_UINT64 ui64DevVAddr,
                                    RI_HANDLE *phRIHandle);

MTGPU_ERROR RIUpdateMEMDESCAddrKM(RI_HANDLE hRIHandle,
                                   uint64_t sVAddr);

MTGPU_ERROR RIDeletePMREntryKM(RI_HANDLE hRIHandle);
MTGPU_ERROR RIDeleteMEMDESCEntryKM(RI_HANDLE hRIHandle);

MTGPU_ERROR RIDeleteListKM(void);

MTGPU_ERROR RIDumpListKM(PMR *psPMR);

MTGPU_ERROR RIDumpAllKM(void);

MTGPU_ERROR RIDumpProcessKM(MT_PID pid);

#if defined(DEBUG)
MTGPU_ERROR RIDumpProcessListKM(PMR *psPMR,
                                 MT_PID pid,
                                 MT_UINT64 ui64Offset,
                                 uint64_t *psDevVAddr);
#endif

MT_BOOL RIGetListEntryKM(MT_PID pid,
                          MT_HANDLE **ppHandle,
                          MT_CHAR **ppszEntryString);

MT_INT32 RITotalAllocProcessKM(MT_PID pid, PHYS_HEAP_TYPE ePhysHeapType);

#endif /* RI_SERVER_H */

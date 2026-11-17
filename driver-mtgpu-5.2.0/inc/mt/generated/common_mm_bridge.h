/*******************************************************************************
@File
@Title          Common bridge header for mm
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Declares common defines and structures used by both the client
                and server side of the bridge for mm
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
*******************************************************************************/

#ifndef COMMON_MM_BRIDGE_H
#define COMMON_MM_BRIDGE_H

#include <mt/mem_types.h>

#include "img_defs.h"
#include "img_types.h"
#include "mtgpu_error.h"

#include "mtgpu_memallocflags.h"
#include "mtgpu_memalloc_physheap.h"
#include "devicemem_typedefs.h"

#define MTGPU_BRIDGE_MM_CMD_FIRST			0
#define MTGPU_BRIDGE_MM_PMREXPORTPMR			MTGPU_BRIDGE_MM_CMD_FIRST+0
#define MTGPU_BRIDGE_MM_PMRUNEXPORTPMR			MTGPU_BRIDGE_MM_CMD_FIRST+1
#define MTGPU_BRIDGE_MM_PMRGETUID			MTGPU_BRIDGE_MM_CMD_FIRST+2
#define MTGPU_BRIDGE_MM_PMRMAKELOCALIMPORTHANDLE			MTGPU_BRIDGE_MM_CMD_FIRST+3
#define MTGPU_BRIDGE_MM_PMRUNMAKELOCALIMPORTHANDLE			MTGPU_BRIDGE_MM_CMD_FIRST+4
#define MTGPU_BRIDGE_MM_PMRIMPORTPMR			MTGPU_BRIDGE_MM_CMD_FIRST+5
#define MTGPU_BRIDGE_MM_PMRLOCALIMPORTPMR			MTGPU_BRIDGE_MM_CMD_FIRST+6
#define MTGPU_BRIDGE_MM_PMRUNREFPMR			MTGPU_BRIDGE_MM_CMD_FIRST+7
#define MTGPU_BRIDGE_MM_PMRUNREFUNLOCKPMR			MTGPU_BRIDGE_MM_CMD_FIRST+8
#define MTGPU_BRIDGE_MM_PHYSMEMNEWRAMBACKEDPMR			MTGPU_BRIDGE_MM_CMD_FIRST+9
#define MTGPU_BRIDGE_MM_PHYSMEMNEWRAMBACKEDLOCKEDPMR			MTGPU_BRIDGE_MM_CMD_FIRST+10
#define MTGPU_BRIDGE_MM_DEVMEMINTPIN			MTGPU_BRIDGE_MM_CMD_FIRST+11
#define MTGPU_BRIDGE_MM_DEVMEMINTUNPIN			MTGPU_BRIDGE_MM_CMD_FIRST+12
#define MTGPU_BRIDGE_MM_DEVMEMINTPINVALIDATE			MTGPU_BRIDGE_MM_CMD_FIRST+13
#define MTGPU_BRIDGE_MM_DEVMEMINTUNPININVALIDATE			MTGPU_BRIDGE_MM_CMD_FIRST+14
#define MTGPU_BRIDGE_MM_DEVMEMINTCTXCREATE			MTGPU_BRIDGE_MM_CMD_FIRST+15
#define MTGPU_BRIDGE_MM_DEVMEMINTCTXDESTROY			MTGPU_BRIDGE_MM_CMD_FIRST+16
#define MTGPU_BRIDGE_MM_DEVMEMINTHEAPCREATE			MTGPU_BRIDGE_MM_CMD_FIRST+17
#define MTGPU_BRIDGE_MM_DEVMEMINTHEAPDESTROY			MTGPU_BRIDGE_MM_CMD_FIRST+18
#define MTGPU_BRIDGE_MM_DEVMEMINTMAPPMR			MTGPU_BRIDGE_MM_CMD_FIRST+19
#define MTGPU_BRIDGE_MM_DEVMEMINTUNMAPPMR			MTGPU_BRIDGE_MM_CMD_FIRST+20
#define MTGPU_BRIDGE_MM_DEVMEMINTRESERVERANGE			MTGPU_BRIDGE_MM_CMD_FIRST+21
#define MTGPU_BRIDGE_MM_DEVMEMINTUNRESERVERANGE			MTGPU_BRIDGE_MM_CMD_FIRST+22
#define MTGPU_BRIDGE_MM_CHANGESPARSEMEM			MTGPU_BRIDGE_MM_CMD_FIRST+23
#define MTGPU_BRIDGE_MM_DEVMEMINTMAPPAGES			MTGPU_BRIDGE_MM_CMD_FIRST+24
#define MTGPU_BRIDGE_MM_DEVMEMINTUNMAPPAGES			MTGPU_BRIDGE_MM_CMD_FIRST+25
#define MTGPU_BRIDGE_MM_DEVMEMISVDEVADDRVALID			MTGPU_BRIDGE_MM_CMD_FIRST+26
#define MTGPU_BRIDGE_MM_DEVMEMFLUSHDEVL2RANGE			MTGPU_BRIDGE_MM_CMD_FIRST+27
#define MTGPU_BRIDGE_MM_DEVMEMINVALIDATEFBSCTABLE			MTGPU_BRIDGE_MM_CMD_FIRST+28
#define MTGPU_BRIDGE_MM_HEAPCFGHEAPCONFIGCOUNT			MTGPU_BRIDGE_MM_CMD_FIRST+29
#define MTGPU_BRIDGE_MM_HEAPCFGHEAPCOUNT			MTGPU_BRIDGE_MM_CMD_FIRST+30
#define MTGPU_BRIDGE_MM_HEAPCFGHEAPCONFIGNAME			MTGPU_BRIDGE_MM_CMD_FIRST+31
#define MTGPU_BRIDGE_MM_HEAPCFGHEAPDETAILS			MTGPU_BRIDGE_MM_CMD_FIRST+32
#define MTGPU_BRIDGE_MM_DEVMEMINTREGISTERPFNOTIFYKM			MTGPU_BRIDGE_MM_CMD_FIRST+33
#define MTGPU_BRIDGE_MM_GETMAXPHYSHEAPCOUNT			MTGPU_BRIDGE_MM_CMD_FIRST+34
#define MTGPU_BRIDGE_MM_PHYSHEAPGETMEMINFO			MTGPU_BRIDGE_MM_CMD_FIRST+35
#define MTGPU_BRIDGE_MM_GETDEFAULTPHYSICALHEAP			MTGPU_BRIDGE_MM_CMD_FIRST+36
#define MTGPU_BRIDGE_MM_GETHEAPPHYSMEMUSAGE			MTGPU_BRIDGE_MM_CMD_FIRST+37
#define MTGPU_BRIDGE_MM_DEVMEMGETFAULTADDRESS			MTGPU_BRIDGE_MM_CMD_FIRST+38
#define MTGPU_BRIDGE_MM_MTGPUUPDATEOOMSTATS			MTGPU_BRIDGE_MM_CMD_FIRST+39
#define MTGPU_BRIDGE_MM_PHYSHEAPGETMEMINFOPKD			MTGPU_BRIDGE_MM_CMD_FIRST+40
#define MTGPU_BRIDGE_MM_GETHEAPPHYSMEMUSAGEPKD			MTGPU_BRIDGE_MM_CMD_FIRST+41
#define MTGPU_BRIDGE_MM_DEVMEMINTMAPPMRWITHRATIO		MTGPU_BRIDGE_MM_CMD_FIRST+42

/* PH1 */
#define MTGPU_BRIDGE_MM_DEVMEMINTHEAPCREATE2			MTGPU_BRIDGE_MM_CMD_FIRST+43
#define MTGPU_BRIDGE_MM_HEAPCFGHEAPDETAILS2			MTGPU_BRIDGE_MM_CMD_FIRST+44
#define MTGPU_BRIDGE_MM_DEVMEMINTRESERVERANGE2			MTGPU_BRIDGE_MM_CMD_FIRST+45
#define MTGPU_BRIDGE_MM_DEVMEMINTMAPPMR2			MTGPU_BRIDGE_MM_CMD_FIRST+46
#define MTGPU_BRIDGE_MM_DEVMEMINTMAPPMRWITHRATIO2		MTGPU_BRIDGE_MM_CMD_FIRST+47

#define MTGPU_BRIDGE_MM_PMRSETMETADATA				MTGPU_BRIDGE_MM_CMD_FIRST+48
#define MTGPU_BRIDGE_MM_GEMSETMETADATA				MTGPU_BRIDGE_MM_CMD_FIRST+49

#define MTGPU_BRIDGE_MM_PMRADDMETADATA				MTGPU_BRIDGE_MM_CMD_FIRST+50
#define MTGPU_BRIDGE_MM_GEMADDMETADATA				MTGPU_BRIDGE_MM_CMD_FIRST+51

#define MTGPU_BRIDGE_MM_CMD_LAST				(MTGPU_BRIDGE_MM_CMD_FIRST+51)

/*******************************************
            PMRExportPMR
 *******************************************/

/* Bridge in structure for PMRExportPMR */
typedef struct MTGPU_BRIDGE_IN_PMREXPORTPMR_TAG
{
	MT_HANDLE hPMR;
} __packed MTGPU_BRIDGE_IN_PMREXPORTPMR;

/* Bridge out structure for PMRExportPMR */
typedef struct MTGPU_BRIDGE_OUT_PMREXPORTPMR_TAG
{
	MT_UINT64 ui64Password;
	MT_UINT64 ui64Size;
	MT_HANDLE hPMRExport;
	MTGPU_ERROR eError;
	MT_UINT32 ui32Log2Contig;
} __packed MTGPU_BRIDGE_OUT_PMREXPORTPMR;

/*******************************************
            PMRUnexportPMR
 *******************************************/

/* Bridge in structure for PMRUnexportPMR */
typedef struct MTGPU_BRIDGE_IN_PMRUNEXPORTPMR_TAG
{
	MT_HANDLE hPMRExport;
} __packed MTGPU_BRIDGE_IN_PMRUNEXPORTPMR;

/* Bridge out structure for PMRUnexportPMR */
typedef struct MTGPU_BRIDGE_OUT_PMRUNEXPORTPMR_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_PMRUNEXPORTPMR;

/*******************************************
            PMRGetUID
 *******************************************/

/* Bridge in structure for PMRGetUID */
typedef struct MTGPU_BRIDGE_IN_PMRGETUID_TAG
{
	MT_HANDLE hPMR;
} __packed MTGPU_BRIDGE_IN_PMRGETUID;

/* Bridge out structure for PMRGetUID */
typedef struct MTGPU_BRIDGE_OUT_PMRGETUID_TAG
{
	MT_UINT64 ui64UID;
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_PMRGETUID;

/*******************************************
            PMRMakeLocalImportHandle
 *******************************************/

/* Bridge in structure for PMRMakeLocalImportHandle */
typedef struct MTGPU_BRIDGE_IN_PMRMAKELOCALIMPORTHANDLE_TAG
{
	MT_HANDLE hBuffer;
} __packed MTGPU_BRIDGE_IN_PMRMAKELOCALIMPORTHANDLE;

/* Bridge out structure for PMRMakeLocalImportHandle */
typedef struct MTGPU_BRIDGE_OUT_PMRMAKELOCALIMPORTHANDLE_TAG
{
	MT_HANDLE hExtMem;
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_PMRMAKELOCALIMPORTHANDLE;

/*******************************************
            PMRUnmakeLocalImportHandle
 *******************************************/

/* Bridge in structure for PMRUnmakeLocalImportHandle */
typedef struct MTGPU_BRIDGE_IN_PMRUNMAKELOCALIMPORTHANDLE_TAG
{
	MT_HANDLE hExtMem;
} __packed MTGPU_BRIDGE_IN_PMRUNMAKELOCALIMPORTHANDLE;

/* Bridge out structure for PMRUnmakeLocalImportHandle */
typedef struct MTGPU_BRIDGE_OUT_PMRUNMAKELOCALIMPORTHANDLE_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_PMRUNMAKELOCALIMPORTHANDLE;

/*******************************************
            PMRImportPMR
 *******************************************/

/* Bridge in structure for PMRImportPMR */
typedef struct MTGPU_BRIDGE_IN_PMRIMPORTPMR_TAG
{
	MT_UINT64 ui64uiPassword;
	MT_UINT64 ui64uiSize;
	MT_HANDLE hPMRExport;
	MT_UINT32 ui32uiLog2Contig;
} __packed MTGPU_BRIDGE_IN_PMRIMPORTPMR;

/* Bridge out structure for PMRImportPMR */
typedef struct MTGPU_BRIDGE_OUT_PMRIMPORTPMR_TAG
{
	MT_HANDLE hPMR;
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_PMRIMPORTPMR;

/*******************************************
            PMRLocalImportPMR
 *******************************************/

/* Bridge in structure for PMRLocalImportPMR */
typedef struct MTGPU_BRIDGE_IN_PMRLOCALIMPORTPMR_TAG
{
	MT_HANDLE hExtHandle;
} __packed MTGPU_BRIDGE_IN_PMRLOCALIMPORTPMR;

/* Bridge out structure for PMRLocalImportPMR */
typedef struct MTGPU_BRIDGE_OUT_PMRLOCALIMPORTPMR_TAG
{
	MT_DEVMEM_ALIGN_T uiAlign;
	MT_DEVMEM_SIZE_T uiSize;
	MT_HANDLE hPMR;
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_PMRLOCALIMPORTPMR;

/*******************************************
            PMRUnrefPMR
 *******************************************/

/* Bridge in structure for PMRUnrefPMR */
typedef struct MTGPU_BRIDGE_IN_PMRUNREFPMR_TAG
{
	MT_HANDLE hPMR;
} __packed MTGPU_BRIDGE_IN_PMRUNREFPMR;

/* Bridge out structure for PMRUnrefPMR */
typedef struct MTGPU_BRIDGE_OUT_PMRUNREFPMR_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_PMRUNREFPMR;

/*******************************************
            PMRUnrefUnlockPMR
 *******************************************/

/* Bridge in structure for PMRUnrefUnlockPMR */
typedef struct MTGPU_BRIDGE_IN_PMRUNREFUNLOCKPMR_TAG
{
	MT_HANDLE hPMR;
} __packed MTGPU_BRIDGE_IN_PMRUNREFUNLOCKPMR;

/* Bridge out structure for PMRUnrefUnlockPMR */
typedef struct MTGPU_BRIDGE_OUT_PMRUNREFUNLOCKPMR_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_PMRUNREFUNLOCKPMR;

/*******************************************
            PhysmemNewRamBackedPMR
 *******************************************/

/* Bridge in structure for PhysmemNewRamBackedPMR */
typedef struct MTGPU_BRIDGE_IN_PHYSMEMNEWRAMBACKEDPMR_TAG
{
	MT_DEVMEM_SIZE_T uiChunkSize;
	MT_DEVMEM_SIZE_T uiSize;
	MT_UINT32 *pui32MappingTable;
	const MT_CHAR *puiAnnotation;
	MT_UINT32 ui32AnnotationLength;
	MT_UINT32 ui32Log2PageSize;
	MT_UINT32 ui32NumPhysChunks;
	MT_UINT32 ui32NumVirtChunks;
	MT_UINT32 ui32PDumpFlags;
	MT_PID ui32PID;
	MTGPU_MEMALLOCFLAGS_T uiFlags;
	MT_HANDLE userAddress;
} __packed MTGPU_BRIDGE_IN_PHYSMEMNEWRAMBACKEDPMR;

/* Bridge out structure for PhysmemNewRamBackedPMR */
typedef struct MTGPU_BRIDGE_OUT_PHYSMEMNEWRAMBACKEDPMR_TAG
{
	MT_HANDLE hPMRPtr;
	MTGPU_ERROR eError;
	MTGPU_MEMALLOCFLAGS_T uiOutFlags;
	MT_BOOL isSystemMem;
} __packed MTGPU_BRIDGE_OUT_PHYSMEMNEWRAMBACKEDPMR;

/*******************************************
            PhysmemNewRamBackedLockedPMR
 *******************************************/

/* Bridge in structure for PhysmemNewRamBackedLockedPMR */
typedef struct MTGPU_BRIDGE_IN_PHYSMEMNEWRAMBACKEDLOCKEDPMR_TAG
{
	MT_DEVMEM_SIZE_T uiChunkSize;
	MT_DEVMEM_SIZE_T uiSize;
	MT_UINT32 *pui32MappingTable;
	const MT_CHAR *puiAnnotation;
	MT_UINT32 ui32AnnotationLength;
	MT_UINT32 ui32Log2PageSize;
	MT_UINT32 ui32NumPhysChunks;
	MT_UINT32 ui32NumVirtChunks;
	MT_UINT32 ui32PDumpFlags;
	MT_PID ui32PID;
	MTGPU_MEMALLOCFLAGS_T uiFlags;
} __packed MTGPU_BRIDGE_IN_PHYSMEMNEWRAMBACKEDLOCKEDPMR;

/* Bridge out structure for PhysmemNewRamBackedLockedPMR */
typedef struct MTGPU_BRIDGE_OUT_PHYSMEMNEWRAMBACKEDLOCKEDPMR_TAG
{
	MT_HANDLE hPMRPtr;
	MTGPU_ERROR eError;
	MTGPU_MEMALLOCFLAGS_T uiOutFlags;
} __packed MTGPU_BRIDGE_OUT_PHYSMEMNEWRAMBACKEDLOCKEDPMR;

/*******************************************
            DevmemIntPin
 *******************************************/

/* Bridge in structure for DevmemIntPin */
typedef struct MTGPU_BRIDGE_IN_DEVMEMINTPIN_TAG
{
	MT_HANDLE hPMR;
} __packed MTGPU_BRIDGE_IN_DEVMEMINTPIN;

/* Bridge out structure for DevmemIntPin */
typedef struct MTGPU_BRIDGE_OUT_DEVMEMINTPIN_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_DEVMEMINTPIN;

/*******************************************
            DevmemIntUnpin
 *******************************************/

/* Bridge in structure for DevmemIntUnpin */
typedef struct MTGPU_BRIDGE_IN_DEVMEMINTUNPIN_TAG
{
	MT_HANDLE hPMR;
} __packed MTGPU_BRIDGE_IN_DEVMEMINTUNPIN;

/* Bridge out structure for DevmemIntUnpin */
typedef struct MTGPU_BRIDGE_OUT_DEVMEMINTUNPIN_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_DEVMEMINTUNPIN;

/*******************************************
            DevmemIntPinValidate
 *******************************************/

/* Bridge in structure for DevmemIntPinValidate */
typedef struct MTGPU_BRIDGE_IN_DEVMEMINTPINVALIDATE_TAG
{
	MT_HANDLE hMapping;
	MT_HANDLE hPMR;
} __packed MTGPU_BRIDGE_IN_DEVMEMINTPINVALIDATE;

/* Bridge out structure for DevmemIntPinValidate */
typedef struct MTGPU_BRIDGE_OUT_DEVMEMINTPINVALIDATE_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_DEVMEMINTPINVALIDATE;

/*******************************************
            DevmemIntUnpinInvalidate
 *******************************************/

/* Bridge in structure for DevmemIntUnpinInvalidate */
typedef struct MTGPU_BRIDGE_IN_DEVMEMINTUNPININVALIDATE_TAG
{
	MT_HANDLE hMapping;
	MT_HANDLE hPMR;
} __packed MTGPU_BRIDGE_IN_DEVMEMINTUNPININVALIDATE;

/* Bridge out structure for DevmemIntUnpinInvalidate */
typedef struct MTGPU_BRIDGE_OUT_DEVMEMINTUNPININVALIDATE_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_DEVMEMINTUNPININVALIDATE;

/*******************************************
            DevmemIntCtxCreate
 *******************************************/

/* Bridge in structure for DevmemIntCtxCreate */
typedef struct MTGPU_BRIDGE_IN_DEVMEMINTCTXCREATE_TAG
{
	MT_BOOL bbKernelMemoryCtx;
} __packed MTGPU_BRIDGE_IN_DEVMEMINTCTXCREATE;

/* Bridge out structure for DevmemIntCtxCreate */
typedef struct MTGPU_BRIDGE_OUT_DEVMEMINTCTXCREATE_TAG
{
	MT_HANDLE hDevMemServerContext;
	MT_HANDLE hPrivData;
	MTGPU_ERROR eError;
	MT_UINT32 ui32CPUCacheLineSize;
} __packed MTGPU_BRIDGE_OUT_DEVMEMINTCTXCREATE;

/*******************************************
            DevmemIntCtxDestroy
 *******************************************/

/* Bridge in structure for DevmemIntCtxDestroy */
typedef struct MTGPU_BRIDGE_IN_DEVMEMINTCTXDESTROY_TAG
{
	MT_HANDLE hDevmemServerContext;
} __packed MTGPU_BRIDGE_IN_DEVMEMINTCTXDESTROY;

/* Bridge out structure for DevmemIntCtxDestroy */
typedef struct MTGPU_BRIDGE_OUT_DEVMEMINTCTXDESTROY_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_DEVMEMINTCTXDESTROY;

/*******************************************
            DevmemIntHeapCreate
 *******************************************/

/* Bridge in structure for DevmemIntHeapCreate */
typedef struct MTGPU_BRIDGE_IN_DEVMEMINTHEAPCREATE2_TAG
{
	MT_DEV_VIRTADDR sHeapBaseAddr;
	MT_DEVMEM_SIZE_T uiHeapLength;
	MT_HANDLE hDevmemCtx;
	MT_UINT32 ui32PageSizeBitMask;
} __packed MTGPU_BRIDGE_IN_DEVMEMINTHEAPCREATE2;

/* Bridge in structure for DevmemIntHeapCreate */
typedef struct MTGPU_BRIDGE_IN_DEVMEMINTHEAPCREATE_TAG
{
	MT_DEV_VIRTADDR sHeapBaseAddr;
	MT_DEVMEM_SIZE_T uiHeapLength;
	MT_HANDLE hDevmemCtx;
	MT_UINT32 ui32Log2DataPageSize;
} __packed MTGPU_BRIDGE_IN_DEVMEMINTHEAPCREATE;

/* Bridge out structure for DevmemIntHeapCreate */
typedef struct MTGPU_BRIDGE_OUT_DEVMEMINTHEAPCREATE_TAG
{
	MT_HANDLE hDevmemHeapPtr;
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_DEVMEMINTHEAPCREATE;

/*******************************************
            DevmemIntHeapDestroy
 *******************************************/

/* Bridge in structure for DevmemIntHeapDestroy */
typedef struct MTGPU_BRIDGE_IN_DEVMEMINTHEAPDESTROY_TAG
{
	MT_HANDLE hDevmemHeap;
} __packed MTGPU_BRIDGE_IN_DEVMEMINTHEAPDESTROY;

/* Bridge out structure for DevmemIntHeapDestroy */
typedef struct MTGPU_BRIDGE_OUT_DEVMEMINTHEAPDESTROY_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_DEVMEMINTHEAPDESTROY;

/*******************************************
            DevmemIntMapPMR
 *******************************************/

/* Bridge in structure for DevmemIntMapPMR */
typedef struct MTGPU_BRIDGE_IN_DEVMEMINTMAPPMR_TAG
{
	MT_HANDLE hDevmemServerHeap;
	MT_HANDLE hPMR;
	MT_HANDLE hReservation;
	MTGPU_MEMALLOCFLAGS_T uiMapFlags;
} __packed MTGPU_BRIDGE_IN_DEVMEMINTMAPPMR;

/* Bridge out structure for DevmemIntMapPMR */
typedef struct MTGPU_BRIDGE_OUT_DEVMEMINTMAPPMR_TAG
{
	MT_HANDLE hMapping;
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_DEVMEMINTMAPPMR;

/*******************************************
            DevmemIntUnmapPMR
 *******************************************/

/* Bridge in structure for DevmemIntUnmapPMR */
typedef struct MTGPU_BRIDGE_IN_DEVMEMINTUNMAPPMR_TAG
{
	MT_HANDLE hMapping;
} __packed MTGPU_BRIDGE_IN_DEVMEMINTUNMAPPMR;

/* Bridge out structure for DevmemIntUnmapPMR */
typedef struct MTGPU_BRIDGE_OUT_DEVMEMINTUNMAPPMR_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_DEVMEMINTUNMAPPMR;

/*******************************************
	  DevmemIntMapPMRWithRatio
 *******************************************/

/* Bridge in structure for DevmemIntMapPMRWithRatio */
typedef struct MTGPU_BRIDGE_IN_DEVMEMINTMAPPMRWITHRATIO_TAG
{
	MT_HANDLE hDevmemServerHeap;
	MT_HANDLE hPMR;
	MT_HANDLE hReservation;
	MTGPU_MEMALLOCFLAGS_T uiMapFlags;
	DEVMEM_INTERLEAVE_RATIO interleaveRatio;
} __packed MTGPU_BRIDGE_IN_DEVMEMINTMAPPMRWITHRATIO;

/*******************************************
            DevmemIntReserveRange
 *******************************************/

/* Bridge in structure for DevmemIntReserveRange */
typedef struct MTGPU_BRIDGE_IN_DEVMEMINTRESERVERANGE2_TAG
{
	MT_DEV_VIRTADDR sAddress;
	MT_DEVMEM_SIZE_T uiLength;
	MT_UINT32 ui32log2PageSize;
	MT_HANDLE hDevmemServerHeap;
} __packed MTGPU_BRIDGE_IN_DEVMEMINTRESERVERANGE2;

/* Bridge in structure for DevmemIntReserveRange */
typedef struct MTGPU_BRIDGE_IN_DEVMEMINTRESERVERANGE_TAG
{
	MT_DEV_VIRTADDR sAddress;
	MT_DEVMEM_SIZE_T uiLength;
	MT_HANDLE hDevmemServerHeap;
} __packed MTGPU_BRIDGE_IN_DEVMEMINTRESERVERANGE;

/* Bridge out structure for DevmemIntReserveRange */
typedef struct MTGPU_BRIDGE_OUT_DEVMEMINTRESERVERANGE_TAG
{
	MT_HANDLE hReservation;
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_DEVMEMINTRESERVERANGE;

/*******************************************
            DevmemIntUnreserveRange
 *******************************************/

/* Bridge in structure for DevmemIntUnreserveRange */
typedef struct MTGPU_BRIDGE_IN_DEVMEMINTUNRESERVERANGE_TAG
{
	MT_HANDLE hReservation;
} __packed MTGPU_BRIDGE_IN_DEVMEMINTUNRESERVERANGE;

/* Bridge out structure for DevmemIntUnreserveRange */
typedef struct MTGPU_BRIDGE_OUT_DEVMEMINTUNRESERVERANGE_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_DEVMEMINTUNRESERVERANGE;

/*******************************************
            ChangeSparseMem
 *******************************************/

/* Bridge in structure for ChangeSparseMem */
typedef struct MTGPU_BRIDGE_IN_CHANGESPARSEMEM_TAG
{
	MT_DEV_VIRTADDR sDevVAddr;
	MT_UINT64 ui64CPUVAddr;
	MT_HANDLE hPMR;
	MT_HANDLE hSrvDevMemHeap;
	MT_UINT32 *pui32AllocPageIndices;
	MT_UINT32 *pui32FreePageIndices;
	MT_UINT32 ui32AllocPageCount;
	MT_UINT32 ui32FreePageCount;
	MT_UINT32 ui32SparseFlags;
	MTGPU_MEMALLOCFLAGS_T uiFlags;
} __packed MTGPU_BRIDGE_IN_CHANGESPARSEMEM;

/* Bridge out structure for ChangeSparseMem */
typedef struct MTGPU_BRIDGE_OUT_CHANGESPARSEMEM_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_CHANGESPARSEMEM;

/*******************************************
            DevmemIntMapPages
 *******************************************/

/* Bridge in structure for DevmemIntMapPages */
typedef struct MTGPU_BRIDGE_IN_DEVMEMINTMAPPAGES_TAG
{
	MT_DEV_VIRTADDR sDevVAddr;
	MT_HANDLE hPMR;
	MT_HANDLE hReservation;
	MT_UINT32 ui32PageCount;
	MT_UINT32 ui32PhysicalPgOffset;
	MTGPU_MEMALLOCFLAGS_T uiFlags;
} __packed MTGPU_BRIDGE_IN_DEVMEMINTMAPPAGES;

/* Bridge out structure for DevmemIntMapPages */
typedef struct MTGPU_BRIDGE_OUT_DEVMEMINTMAPPAGES_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_DEVMEMINTMAPPAGES;

/*******************************************
            DevmemIntUnmapPages
 *******************************************/

/* Bridge in structure for DevmemIntUnmapPages */
typedef struct MTGPU_BRIDGE_IN_DEVMEMINTUNMAPPAGES_TAG
{
	MT_DEV_VIRTADDR sDevVAddr;
	MT_HANDLE hReservation;
	MT_UINT32 ui32PageCount;
} __packed MTGPU_BRIDGE_IN_DEVMEMINTUNMAPPAGES;

/* Bridge out structure for DevmemIntUnmapPages */
typedef struct MTGPU_BRIDGE_OUT_DEVMEMINTUNMAPPAGES_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_DEVMEMINTUNMAPPAGES;

/*******************************************
            DevmemIsVDevAddrValid
 *******************************************/

/* Bridge in structure for DevmemIsVDevAddrValid */
typedef struct MTGPU_BRIDGE_IN_DEVMEMISVDEVADDRVALID_TAG
{
	MT_DEV_VIRTADDR sAddress;
	MT_HANDLE hDevmemCtx;
} __packed MTGPU_BRIDGE_IN_DEVMEMISVDEVADDRVALID;

/* Bridge out structure for DevmemIsVDevAddrValid */
typedef struct MTGPU_BRIDGE_OUT_DEVMEMISVDEVADDRVALID_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_DEVMEMISVDEVADDRVALID;

/*******************************************
            DevmemFlushDevL2Range
 *******************************************/

/* Bridge in structure for DevmemFlushDevL2Range */
typedef struct MTGPU_BRIDGE_IN_DEVMEMFLUSHDEVL2RANGE_TAG
{
	MT_DEV_VIRTADDR sAddress;
	MT_DEVMEM_SIZE_T uiSize;
	MT_HANDLE hDevmemCtx;
	MT_BOOL bInvalidate;
} __packed MTGPU_BRIDGE_IN_DEVMEMFLUSHDEVL2RANGE;

/* Bridge out structure for DevmemFlushDevL2Range */
typedef struct MTGPU_BRIDGE_OUT_DEVMEMFLUSHDEVL2RANGE_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_DEVMEMFLUSHDEVL2RANGE;

/*******************************************
            DevmemInvalidateFBSCTable
 *******************************************/

/* Bridge in structure for DevmemInvalidateFBSCTable */
typedef struct MTGPU_BRIDGE_IN_DEVMEMINVALIDATEFBSCTABLE_TAG
{
	MT_UINT64 ui64FBSCEntries;
	MT_HANDLE hDevmemCtx;
} __packed MTGPU_BRIDGE_IN_DEVMEMINVALIDATEFBSCTABLE;

/* Bridge out structure for DevmemInvalidateFBSCTable */
typedef struct MTGPU_BRIDGE_OUT_DEVMEMINVALIDATEFBSCTABLE_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_DEVMEMINVALIDATEFBSCTABLE;

/*******************************************
            HeapCfgHeapConfigCount
 *******************************************/

/* Bridge in structure for HeapCfgHeapConfigCount */
typedef struct MTGPU_BRIDGE_IN_HEAPCFGHEAPCONFIGCOUNT_TAG
{
	MT_UINT32 ui32EmptyStructPlaceholder;
} __packed MTGPU_BRIDGE_IN_HEAPCFGHEAPCONFIGCOUNT;

/* Bridge out structure for HeapCfgHeapConfigCount */
typedef struct MTGPU_BRIDGE_OUT_HEAPCFGHEAPCONFIGCOUNT_TAG
{
	MTGPU_ERROR eError;
	MT_UINT32 ui32NumHeapConfigs;
} __packed MTGPU_BRIDGE_OUT_HEAPCFGHEAPCONFIGCOUNT;

/*******************************************
            HeapCfgHeapCount
 *******************************************/

/* Bridge in structure for HeapCfgHeapCount */
typedef struct MTGPU_BRIDGE_IN_HEAPCFGHEAPCOUNT_TAG
{
	MT_UINT32 ui32HeapConfigIndex;
} __packed MTGPU_BRIDGE_IN_HEAPCFGHEAPCOUNT;

/* Bridge out structure for HeapCfgHeapCount */
typedef struct MTGPU_BRIDGE_OUT_HEAPCFGHEAPCOUNT_TAG
{
	MTGPU_ERROR eError;
	MT_UINT32 ui32NumHeaps;
} __packed MTGPU_BRIDGE_OUT_HEAPCFGHEAPCOUNT;

/*******************************************
            HeapCfgHeapConfigName
 *******************************************/

/* Bridge in structure for HeapCfgHeapConfigName */
typedef struct MTGPU_BRIDGE_IN_HEAPCFGHEAPCONFIGNAME_TAG
{
	MT_CHAR *puiHeapConfigName;
	MT_UINT32 ui32HeapConfigIndex;
	MT_UINT32 ui32HeapConfigNameBufSz;
} __packed MTGPU_BRIDGE_IN_HEAPCFGHEAPCONFIGNAME;

/* Bridge out structure for HeapCfgHeapConfigName */
typedef struct MTGPU_BRIDGE_OUT_HEAPCFGHEAPCONFIGNAME_TAG
{
	MT_CHAR *puiHeapConfigName;
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_HEAPCFGHEAPCONFIGNAME;

/*******************************************
            HeapCfgHeapDetails
 *******************************************/

/* Bridge in structure for HeapCfgHeapDetails */
typedef struct MTGPU_BRIDGE_IN_HEAPCFGHEAPDETAILS_TAG
{
	MT_CHAR *puiHeapNameOut;
	MT_UINT32 ui32HeapConfigIndex;
	MT_UINT32 ui32HeapIndex;
	MT_UINT32 ui32HeapNameBufSz;
} __packed MTGPU_BRIDGE_IN_HEAPCFGHEAPDETAILS;

/* Bridge out structure for HeapCfgHeapDetails */
typedef struct MTGPU_BRIDGE_OUT_HEAPCFGHEAPDETAILS_TAG
{
	MT_DEV_VIRTADDR sDevVAddrBase;
	MT_DEVMEM_SIZE_T uiHeapLength;
	MT_DEVMEM_SIZE_T uiReservedRegionLength;
	MT_CHAR *puiHeapNameOut;
	MTGPU_ERROR eError;
	MT_UINT32 ui32Log2DataPageSizeOut;
	MT_UINT32 ui32Log2ImportAlignmentOut;
} __packed MTGPU_BRIDGE_OUT_HEAPCFGHEAPDETAILS;

typedef struct MTGPU_BRIDGE_OUT_HEAPCFGHEAPDETAILS2_TAG
{
	MT_DEV_VIRTADDR sDevVAddrBase;
	MT_DEVMEM_SIZE_T uiHeapLength;
	MT_DEVMEM_SIZE_T uiReservedRegionLength;
	MT_CHAR *puiHeapNameOut;
	MTGPU_ERROR eError;
	MT_UINT32 ui32PageShiftBitMask;
	MT_UINT32 ui32Log2ImportAlignmentOut;
	MT_BOOL bSupportMultiPgSize;
} __packed MTGPU_BRIDGE_OUT_HEAPCFGHEAPDETAILS2;

/*******************************************
            DevmemIntRegisterPFNotifyKM
 *******************************************/

/* Bridge in structure for DevmemIntRegisterPFNotifyKM */
typedef struct MTGPU_BRIDGE_IN_DEVMEMINTREGISTERPFNOTIFYKM_TAG
{
	MT_HANDLE hDevm;
	MT_BOOL bRegister;
	MT_UINT32 ui32PID;
} __packed MTGPU_BRIDGE_IN_DEVMEMINTREGISTERPFNOTIFYKM;

/* Bridge out structure for DevmemIntRegisterPFNotifyKM */
typedef struct MTGPU_BRIDGE_OUT_DEVMEMINTREGISTERPFNOTIFYKM_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_DEVMEMINTREGISTERPFNOTIFYKM;

/*******************************************
            GetMaxPhysHeapCount
 *******************************************/

/* Bridge in structure for GetMaxPhysHeapCount */
typedef struct MTGPU_BRIDGE_IN_GETMAXPHYSHEAPCOUNT_TAG
{
	MT_UINT32 ui32EmptyStructPlaceholder;
} __packed MTGPU_BRIDGE_IN_GETMAXPHYSHEAPCOUNT;

/* Bridge out structure for GetMaxPhysHeapCount */
typedef struct MTGPU_BRIDGE_OUT_GETMAXPHYSHEAPCOUNT_TAG
{
	MTGPU_ERROR eError;
	MT_UINT32 ui32PhysHeapCount;
} __packed MTGPU_BRIDGE_OUT_GETMAXPHYSHEAPCOUNT;

/*******************************************
            PhysHeapGetMemInfo
 *******************************************/

/* Bridge in structure for PhysHeapGetMemInfo */
typedef struct MTGPU_BRIDGE_IN_PHYSHEAPGETMEMINFO_TAG
{
	PHYS_HEAP_MEM_STATS *pasapPhysHeapMemStats;
	MTGPU_PHYS_HEAP *peaPhysHeapID;
	MT_UINT32 ui32PhysHeapCount;
} __packed MTGPU_BRIDGE_IN_PHYSHEAPGETMEMINFO;

/* Bridge out structure for PhysHeapGetMemInfo */
typedef struct MTGPU_BRIDGE_OUT_PHYSHEAPGETMEMINFO_TAG
{
	PHYS_HEAP_MEM_STATS *pasapPhysHeapMemStats;
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_PHYSHEAPGETMEMINFO;

/*******************************************
            GetDefaultPhysicalHeap
 *******************************************/

/* Bridge in structure for GetDefaultPhysicalHeap */
typedef struct MTGPU_BRIDGE_IN_GETDEFAULTPHYSICALHEAP_TAG
{
	MT_UINT32 ui32EmptyStructPlaceholder;
} __packed MTGPU_BRIDGE_IN_GETDEFAULTPHYSICALHEAP;

/* Bridge out structure for GetDefaultPhysicalHeap */
typedef struct MTGPU_BRIDGE_OUT_GETDEFAULTPHYSICALHEAP_TAG
{
	MTGPU_ERROR eError;
	MTGPU_PHYS_HEAP eHeap;
} __packed MTGPU_BRIDGE_OUT_GETDEFAULTPHYSICALHEAP;

/*******************************************
            GetHeapPhysMemUsage
 *******************************************/

/* Bridge in structure for GetHeapPhysMemUsage */
typedef struct MTGPU_BRIDGE_IN_GETHEAPPHYSMEMUSAGE_TAG
{
	PHYS_HEAP_MEM_STATS *pasapPhysHeapMemStats;
	MT_UINT32 ui32PhysHeapCount;
} __packed MTGPU_BRIDGE_IN_GETHEAPPHYSMEMUSAGE;

/* Bridge out structure for GetHeapPhysMemUsage */
typedef struct MTGPU_BRIDGE_OUT_GETHEAPPHYSMEMUSAGE_TAG
{
	PHYS_HEAP_MEM_STATS *pasapPhysHeapMemStats;
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_GETHEAPPHYSMEMUSAGE;

/*******************************************
            DevmemGetFaultAddress
 *******************************************/

/* Bridge in structure for DevmemGetFaultAddress */
typedef struct MTGPU_BRIDGE_IN_DEVMEMGETFAULTADDRESS_TAG
{
	MT_HANDLE hDevmemCtx;
} __packed MTGPU_BRIDGE_IN_DEVMEMGETFAULTADDRESS;

/* Bridge out structure for DevmemGetFaultAddress */
typedef struct MTGPU_BRIDGE_OUT_DEVMEMGETFAULTADDRESS_TAG
{
	MT_DEV_VIRTADDR sFaultAddress;
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_DEVMEMGETFAULTADDRESS;

/*******************************************
            MTGPUUpdateOOMStats
 *******************************************/

/* Bridge in structure for MTGPUUpdateOOMStats */
typedef struct MTGPU_BRIDGE_IN_MTGPUUPDATEOOMSTATS_TAG
{
	MT_PID ui32pid;
	MT_UINT32 ui32ui32StatType;
} __packed MTGPU_BRIDGE_IN_MTGPUUPDATEOOMSTATS;

/* Bridge out structure for MTGPUUpdateOOMStats */
typedef struct MTGPU_BRIDGE_OUT_MTGPUUPDATEOOMSTATS_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_MTGPUUPDATEOOMSTATS;

/*******************************************
            PhysHeapGetMemInfoPkd
 *******************************************/

/* Bridge in structure for PhysHeapGetMemInfoPkd */
typedef struct MTGPU_BRIDGE_IN_PHYSHEAPGETMEMINFOPKD_TAG
{
	PHYS_HEAP_MEM_STATS_PKD *psapPhysHeapMemStats;
	MTGPU_PHYS_HEAP *peaPhysHeapID;
	MT_UINT32 ui32PhysHeapCount;
} __packed MTGPU_BRIDGE_IN_PHYSHEAPGETMEMINFOPKD;

/* Bridge out structure for PhysHeapGetMemInfoPkd */
typedef struct MTGPU_BRIDGE_OUT_PHYSHEAPGETMEMINFOPKD_TAG
{
	PHYS_HEAP_MEM_STATS_PKD *psapPhysHeapMemStats;
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_PHYSHEAPGETMEMINFOPKD;

/*******************************************
            GetHeapPhysMemUsagePkd
 *******************************************/

/* Bridge in structure for GetHeapPhysMemUsagePkd */
typedef struct MTGPU_BRIDGE_IN_GETHEAPPHYSMEMUSAGEPKD_TAG
{
	PHYS_HEAP_MEM_STATS_PKD *psapPhysHeapMemStats;
	MT_UINT32 ui32PhysHeapCount;
} __packed MTGPU_BRIDGE_IN_GETHEAPPHYSMEMUSAGEPKD;

/* Bridge out structure for GetHeapPhysMemUsagePkd */
typedef struct MTGPU_BRIDGE_OUT_GETHEAPPHYSMEMUSAGEPKD_TAG
{
	PHYS_HEAP_MEM_STATS_PKD *psapPhysHeapMemStats;
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_GETHEAPPHYSMEMUSAGEPKD;

/*******************************************
		  PMRAddMetaData
 *******************************************/

/* Bridge in structure for PMRAddMetaData */
typedef struct MTGPU_BRIDGE_IN_PMRADDMETADATA_TAG
{
	MT_HANDLE hPMR;
	MT_UINT64 ui64MetaDataId;
	MT_BYTE *pui8MetaData;
	MT_UINT32 ui32DataSize;
} __packed MTGPU_BRIDGE_IN_PMRADDMETADATA;

/* Bridge out structure for PMRAddMetaData */
typedef struct MTGPU_BRIDGE_OUT_PMRADDMETADATA_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_PMRADDMETADATA;

/*******************************************
		  PMRSetMetaData
 *******************************************/

/* Bridge in structure for PMRSetMetaData */
typedef struct MTGPU_BRIDGE_IN_PMRSETMETADATA_TAG
{
	MT_HANDLE hPMR;
	MT_BYTE *pui8MetaData;
	MT_UINT32 ui32DataSize;
} __packed MTGPU_BRIDGE_IN_PMRSETMETADATA;

/* Bridge out structure for PMRSetMetaData */
typedef struct MTGPU_BRIDGE_OUT_PMRSETMETADATA_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_PMRSETMETADATA;

/*******************************************
		  GEMAddMetaData
 *******************************************/

/* Bridge in structure for GEMAddMetaData */
typedef struct MTGPU_BRIDGE_IN_GEMADDMETADATA_TAG
{
	MT_UINT32 ui32CardFd;
	MT_UINT32 ui32GemHandle;
	MT_UINT64 ui64MetaDataId;
	MT_BYTE *pui8MetaData;
	MT_UINT32 ui32DataSize;
} __packed MTGPU_BRIDGE_IN_GEMADDMETADATA;

/* Bridge out structure for GEMAddMetaData */
typedef struct MTGPU_BRIDGE_OUT_GEMADDMETADATA_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_GEMADDMETADATA;

/*******************************************
		  GEMSetMetaData
 *******************************************/

/* Bridge in structure for GEMSetMetaData */
typedef struct MTGPU_BRIDGE_IN_GEMSETMETADATA_TAG
{
	MT_UINT32 ui32CardFd;
	MT_UINT32 ui32GemHandle;
	MT_BYTE *pui8MetaData;
	MT_UINT32 ui32DataSize;
} __packed MTGPU_BRIDGE_IN_GEMSETMETADATA;

/* Bridge out structure for GEMSetMetaData */
typedef struct MTGPU_BRIDGE_OUT_GEMSETMETADATA_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_GEMSETMETADATA;

#endif /* COMMON_MM_BRIDGE_H */

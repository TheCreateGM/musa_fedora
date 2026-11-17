/*******************************************************************************
@File
@Title          Common bridge header for devicememhistory
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Declares common defines and structures used by both the client
                and server side of the bridge for devicememhistory
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

#ifndef COMMON_DEVICEMEMHISTORY_BRIDGE_H
#define COMMON_DEVICEMEMHISTORY_BRIDGE_H

#include <mt/mem_types.h>

#include "img_defs.h"
#include "img_types.h"
#include "mtgpu_error.h"

#include "img_types.h"
#include "img_defs.h"
#include "devicemem_typedefs.h"

#define MTGPU_BRIDGE_DEVICEMEMHISTORY_CMD_FIRST			0
#define MTGPU_BRIDGE_DEVICEMEMHISTORY_DEVICEMEMHISTORYMAP			MTGPU_BRIDGE_DEVICEMEMHISTORY_CMD_FIRST+0
#define MTGPU_BRIDGE_DEVICEMEMHISTORY_DEVICEMEMHISTORYUNMAP			MTGPU_BRIDGE_DEVICEMEMHISTORY_CMD_FIRST+1
#define MTGPU_BRIDGE_DEVICEMEMHISTORY_DEVICEMEMHISTORYMAMTANGE			MTGPU_BRIDGE_DEVICEMEMHISTORY_CMD_FIRST+2
#define MTGPU_BRIDGE_DEVICEMEMHISTORY_DEVICEMEMHISTORYUNMAMTANGE			MTGPU_BRIDGE_DEVICEMEMHISTORY_CMD_FIRST+3
#define MTGPU_BRIDGE_DEVICEMEMHISTORY_DEVICEMEMHISTORYSPARSECHANGE			MTGPU_BRIDGE_DEVICEMEMHISTORY_CMD_FIRST+4
#define MTGPU_BRIDGE_DEVICEMEMHISTORY_CMD_LAST			(MTGPU_BRIDGE_DEVICEMEMHISTORY_CMD_FIRST+4)

/*******************************************
            DevicememHistoryMap
 *******************************************/

/* Bridge in structure for DevicememHistoryMap */
typedef struct MTGPU_BRIDGE_IN_DEVICEMEMHISTORYMAP_TAG
{
	MT_DEV_VIRTADDR sDevVAddr;
	MT_DEVMEM_SIZE_T uiOffset;
	MT_DEVMEM_SIZE_T uiSize;
	MT_HANDLE hPMR;
	const MT_CHAR *puiText;
	MT_UINT32 ui32AllocationIndex;
	MT_UINT32 ui32Log2PageSize;
} __packed MTGPU_BRIDGE_IN_DEVICEMEMHISTORYMAP;

/* Bridge out structure for DevicememHistoryMap */
typedef struct MTGPU_BRIDGE_OUT_DEVICEMEMHISTORYMAP_TAG
{
	MTGPU_ERROR eError;
	MT_UINT32 ui32AllocationIndexOut;
} __packed MTGPU_BRIDGE_OUT_DEVICEMEMHISTORYMAP;

/*******************************************
            DevicememHistoryUnmap
 *******************************************/

/* Bridge in structure for DevicememHistoryUnmap */
typedef struct MTGPU_BRIDGE_IN_DEVICEMEMHISTORYUNMAP_TAG
{
	MT_DEV_VIRTADDR sDevVAddr;
	MT_DEVMEM_SIZE_T uiOffset;
	MT_DEVMEM_SIZE_T uiSize;
	MT_HANDLE hPMR;
	const MT_CHAR *puiText;
	MT_UINT32 ui32AllocationIndex;
	MT_UINT32 ui32Log2PageSize;
} __packed MTGPU_BRIDGE_IN_DEVICEMEMHISTORYUNMAP;

/* Bridge out structure for DevicememHistoryUnmap */
typedef struct MTGPU_BRIDGE_OUT_DEVICEMEMHISTORYUNMAP_TAG
{
	MTGPU_ERROR eError;
	MT_UINT32 ui32AllocationIndexOut;
} __packed MTGPU_BRIDGE_OUT_DEVICEMEMHISTORYUNMAP;

/*******************************************
            DevicememHistoryMapVRange
 *******************************************/

/* Bridge in structure for DevicememHistoryMapVRange */
typedef struct MTGPU_BRIDGE_IN_DEVICEMEMHISTORYMAMTANGE_TAG
{
	MT_DEV_VIRTADDR sBaseDevVAddr;
	MT_DEVMEM_SIZE_T uiAllocSize;
	const MT_CHAR *puiText;
	MT_UINT32 ui32AllocationIndex;
	MT_UINT32 ui32Log2PageSize;
	MT_UINT32 ui32NumPages;
	MT_UINT32 ui32ui32StartPage;
} __packed MTGPU_BRIDGE_IN_DEVICEMEMHISTORYMAMTANGE;

/* Bridge out structure for DevicememHistoryMapVRange */
typedef struct MTGPU_BRIDGE_OUT_DEVICEMEMHISTORYMAMTANGE_TAG
{
	MTGPU_ERROR eError;
	MT_UINT32 ui32AllocationIndexOut;
} __packed MTGPU_BRIDGE_OUT_DEVICEMEMHISTORYMAMTANGE;

/*******************************************
            DevicememHistoryUnmapVRange
 *******************************************/

/* Bridge in structure for DevicememHistoryUnmapVRange */
typedef struct MTGPU_BRIDGE_IN_DEVICEMEMHISTORYUNMAMTANGE_TAG
{
	MT_DEV_VIRTADDR sBaseDevVAddr;
	MT_DEVMEM_SIZE_T uiAllocSize;
	const MT_CHAR *puiText;
	MT_UINT32 ui32AllocationIndex;
	MT_UINT32 ui32Log2PageSize;
	MT_UINT32 ui32NumPages;
	MT_UINT32 ui32ui32StartPage;
} __packed MTGPU_BRIDGE_IN_DEVICEMEMHISTORYUNMAMTANGE;

/* Bridge out structure for DevicememHistoryUnmapVRange */
typedef struct MTGPU_BRIDGE_OUT_DEVICEMEMHISTORYUNMAMTANGE_TAG
{
	MTGPU_ERROR eError;
	MT_UINT32 ui32AllocationIndexOut;
} __packed MTGPU_BRIDGE_OUT_DEVICEMEMHISTORYUNMAMTANGE;

/*******************************************
            DevicememHistorySparseChange
 *******************************************/

/* Bridge in structure for DevicememHistorySparseChange */
typedef struct MTGPU_BRIDGE_IN_DEVICEMEMHISTORYSPARSECHANGE_TAG
{
	MT_DEV_VIRTADDR sDevVAddr;
	MT_DEVMEM_SIZE_T uiOffset;
	MT_DEVMEM_SIZE_T uiSize;
	MT_HANDLE hPMR;
	MT_UINT32 *pui32AllocPageIndices;
	MT_UINT32 *pui32FreePageIndices;
	const MT_CHAR *puiText;
	MT_UINT32 ui32AllocPageCount;
	MT_UINT32 ui32AllocationIndex;
	MT_UINT32 ui32FreePageCount;
	MT_UINT32 ui32Log2PageSize;
} __packed MTGPU_BRIDGE_IN_DEVICEMEMHISTORYSPARSECHANGE;

/* Bridge out structure for DevicememHistorySparseChange */
typedef struct MTGPU_BRIDGE_OUT_DEVICEMEMHISTORYSPARSECHANGE_TAG
{
	MTGPU_ERROR eError;
	MT_UINT32 ui32AllocationIndexOut;
} __packed MTGPU_BRIDGE_OUT_DEVICEMEMHISTORYSPARSECHANGE;

#endif /* COMMON_DEVICEMEMHISTORY_BRIDGE_H */

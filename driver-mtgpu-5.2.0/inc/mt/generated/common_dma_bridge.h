/*******************************************************************************
@File
@Title          Common bridge header for dma
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Declares common defines and structures used by both the client
                and server side of the bridge for dma
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

#ifndef COMMON_DMA_BRIDGE_H
#define COMMON_DMA_BRIDGE_H

#include <mt/mem_types.h>

#include "img_defs.h"
#include "img_types.h"
#include "mtgpu_error.h"

#include "mtgpu_sync_km.h"

#define MTGPU_BRIDGE_DMA_CMD_FIRST			0
#define MTGPU_BRIDGE_DMA_DMATRANSFER			(MTGPU_BRIDGE_DMA_CMD_FIRST + 0)
#define MTGPU_BRIDGE_DMA_DMASPARSEMAPPINGTABLE		(MTGPU_BRIDGE_DMA_CMD_FIRST + 1)
#define MTGPU_BRIDGE_DMA_DMADEVICEPARAMS		(MTGPU_BRIDGE_DMA_CMD_FIRST + 2)
#define MTGPU_BRIDGE_DMA_P2PTRANSFER			(MTGPU_BRIDGE_DMA_CMD_FIRST + 3)
#define MTGPU_BRIDGE_DMA_CMD_LAST			(MTGPU_BRIDGE_DMA_CMD_FIRST + 3)

/*******************************************
            DmaTransfer
 *******************************************/

/* Bridge in structure for DmaTransfer */
typedef struct MTGPU_BRIDGE_IN_DMATRANSFER_TAG
{
	MT_UINT64 *pui64Address;
	MT_DEVMEM_OFFSET_T *puiOffset;
	MT_DEVMEM_SIZE_T *puiSize;
	MT_HANDLE *phPMR;
	MTGPU_TIMELINE hUpdateTimeline;
	MT_UINT32 ui32NumDMAs;
	MT_UINT32 ui32Flags;
	MT_UINT32 ui32ExtJobRef;
} __packed MTGPU_BRIDGE_IN_DMATRANSFER;

typedef struct MTGPU_BRIDGE_IN_DMAP2PTRANSFER_TAG {
	MT_DEVMEM_OFFSET_T *puiLocalOffset;
	MT_DEVMEM_OFFSET_T *puiPeerOffset;
	MT_DEVMEM_SIZE_T *puiSize;
	MT_HANDLE *phLocalPMR;
	MT_HANDLE *phPeerPMR;
	MTGPU_TIMELINE hUpdateTimeline;
	MT_UINT32 ui32NumDMAs;
	MT_UINT32 ui32Flags;
	MT_UINT32 ui32ExtJobRef;
} __packed MTGPU_BRIDGE_IN_DMAP2PTRANSFER;

/* Bridge out structure for DmaTransfer */
typedef struct MTGPU_BRIDGE_OUT_DMATRANSFER_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_DMATRANSFER;

/*******************************************
            DmaSparseMappingTable
 *******************************************/

/* Bridge in structure for DmaSparseMappingTable */
typedef struct MTGPU_BRIDGE_IN_DMASPARSEMAPPINGTABLE_TAG
{
	MT_DEVMEM_OFFSET_T uiOffset;
	MT_HANDLE hPMR;
	MT_BOOL *pbTable;
	MT_UINT32 ui32SizeInPages;
} __packed MTGPU_BRIDGE_IN_DMASPARSEMAPPINGTABLE;

/* Bridge out structure for DmaSparseMappingTable */
typedef struct MTGPU_BRIDGE_OUT_DMASPARSEMAPPINGTABLE_TAG
{
	MT_BOOL *pbTable;
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_DMASPARSEMAPPINGTABLE;

/*******************************************
            DmaDeviceParams
 *******************************************/

/* Bridge in structure for DmaDeviceParams */
typedef struct MTGPU_BRIDGE_IN_DMADEVICEPARAMS_TAG
{
	MT_UINT32 ui32EmptyStructPlaceholder;
} __packed MTGPU_BRIDGE_IN_DMADEVICEPARAMS;

/* Bridge out structure for DmaDeviceParams */
typedef struct MTGPU_BRIDGE_OUT_DMADEVICEPARAMS_TAG
{
	MTGPU_ERROR eError;
	MT_UINT32 ui32DmaBuffAlign;
	MT_UINT32 ui32DmaTransferMult;
} __packed MTGPU_BRIDGE_OUT_DMADEVICEPARAMS;

#endif /* COMMON_DMA_BRIDGE_H */

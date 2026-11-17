/*******************************************************************************
@File
@Title          Common bridge header for musagfx
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Declares common defines and structures used by both the client
                and server side of the bridge for musagfx
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

#ifndef COMMON_MUSAGFX_BRIDGE_H
#define COMMON_MUSAGFX_BRIDGE_H

#include <mt/mem_types.h>

#include "img_defs.h"
#include "img_types.h"
#include "mtgpu_error.h"

#include "musa_fwif_shared.h"
#include "devicemem_typedefs.h"
#include "mtgpu_sync_km.h"

#define MTGPU_BRIDGE_MUSAGFX_CMD_FIRST			0
#define MTGPU_BRIDGE_MUSAGFX_MUSACREATEHWRTDATASET			MTGPU_BRIDGE_MUSAGFX_CMD_FIRST+0
#define MTGPU_BRIDGE_MUSAGFX_MUSADESTROYHWRTDATASET			MTGPU_BRIDGE_MUSAGFX_CMD_FIRST+1
#define MTGPU_BRIDGE_MUSAGFX_MUSACREATEZSBUFFER			MTGPU_BRIDGE_MUSAGFX_CMD_FIRST+2
#define MTGPU_BRIDGE_MUSAGFX_MUSADESTROYZSBUFFER			MTGPU_BRIDGE_MUSAGFX_CMD_FIRST+3
#define MTGPU_BRIDGE_MUSAGFX_MUSAPOPULATEZSBUFFER			MTGPU_BRIDGE_MUSAGFX_CMD_FIRST+4
#define MTGPU_BRIDGE_MUSAGFX_MUSAUNPOPULATEZSBUFFER			MTGPU_BRIDGE_MUSAGFX_CMD_FIRST+5
#define MTGPU_BRIDGE_MUSAGFX_MUSACREATEFREELIST			MTGPU_BRIDGE_MUSAGFX_CMD_FIRST+6
#define MTGPU_BRIDGE_MUSAGFX_MUSADESTROYFREELIST			MTGPU_BRIDGE_MUSAGFX_CMD_FIRST+7
#define MTGPU_BRIDGE_MUSAGFX_MUSACREATERENDERCONTEXT			MTGPU_BRIDGE_MUSAGFX_CMD_FIRST+8
#define MTGPU_BRIDGE_MUSAGFX_MUSADESTROYRENDERCONTEXT			MTGPU_BRIDGE_MUSAGFX_CMD_FIRST+9
#define MTGPU_BRIDGE_MUSAGFX_MUSASETRENDERCONTEXTPRIORITY			MTGPU_BRIDGE_MUSAGFX_CMD_FIRST+10
#define MTGPU_BRIDGE_MUSAGFX_MUSARENDERCONTEXTSTALLED			MTGPU_BRIDGE_MUSAGFX_CMD_FIRST+11
#define MTGPU_BRIDGE_MUSAGFX_MUSAKICKGFX2			MTGPU_BRIDGE_MUSAGFX_CMD_FIRST+12
#define MTGPU_BRIDGE_MUSAGFX_MUSASETRENDERCONTEXTPROPERTY			MTGPU_BRIDGE_MUSAGFX_CMD_FIRST+13
#define MTGPU_BRIDGE_MUSAGFX_MUSAKICKGFX3	(MTGPU_BRIDGE_MUSAGFX_CMD_FIRST + 14)
#define MTGPU_BRIDGE_MUSAGFX_CREATE_FENCE	(MTGPU_BRIDGE_MUSAGFX_CMD_FIRST + 15)
#define MTGPU_BRIDGE_MUSAGFX_MUSAKICKGFX4	(MTGPU_BRIDGE_MUSAGFX_CMD_FIRST + 16)
#define MTGPU_BRIDGE_MUSAGFX_MUSACREATEHWRTDATASET2			MTGPU_BRIDGE_MUSAGFX_CMD_FIRST+17

/* for ddk 2.0 */
#define MTGPU_BRIDGE_MUSAGFX_MUSACREATERENDERCONTEXT2			MTGPU_BRIDGE_MUSAGFX_CMD_FIRST+18
#define MTGPU_BRIDGE_MUSAGFX_MUSADESTROYRENDERCONTEXT2			MTGPU_BRIDGE_MUSAGFX_CMD_FIRST+19
#define MTGPU_BRIDGE_MUSAGFX_MUSAKICKGFX5				MTGPU_BRIDGE_MUSAGFX_CMD_FIRST+20

#define MTGPU_BRIDGE_MUSAGFX_CMD_LAST		(MTGPU_BRIDGE_MUSAGFX_CMD_FIRST + 20)

/*******************************************
            MUSACreateHWRTDataSet
 *******************************************/

/* Bridge in structure for MUSACreateHWRTDataSet */
typedef struct MTGPU_BRIDGE_IN_MUSACREATEHWRTDATASET_TAG
{
	MT_UINT32 ui32McgCoreNum;
	MT_DEV_VIRTADDR *psPMDataAddrMcg;
	MT_DEV_VIRTADDR *psTailPtrsDevVAddrMcg;
	MT_DEV_VIRTADDR sVHeapTableDevVAddr;
	MT_UINT64 ui64PPPMultiSampleCtl;
	MT_DEV_VIRTADDR *psPMDataAddr;
	MT_DEV_VIRTADDR *psPMSecureDataAddr;
	MT_DEV_VIRTADDR *psTailPtrsDevVAddr;
	MT_HANDLE *phKmHwRTDataSet;
	MT_HANDLE *phapsFreeLists;
	MT_UINT32 ui32FreeListsCount;
	MT_UINT32 ui32ISPMergeLowerX;
	MT_UINT32 ui32ISPMergeLowerY;
	MT_UINT32 ui32ISPMergeScaleX;
	MT_UINT32 ui32ISPMergeScaleY;
	MT_UINT32 ui32ISPMergeUpperX;
	MT_UINT32 ui32ISPMergeUpperY;
	MT_UINT32 ui32PPPScreen;
	MT_UINT32 ui32RgnStride;
	MT_UINT32 ui32TEAA;
	MT_UINT32 ui32TEMTILE1;
	MT_UINT32 ui32TEMTILE2;
	MT_UINT32 ui32TEScreen;
	MT_UINT32 ui32TPCSize;
	MT_UINT32 ui32TPCStride;
	MT_UINT16 ui16MaxRTs;
} __packed MTGPU_BRIDGE_IN_MUSACREATEHWRTDATASET;

typedef struct MTGPU_BRIDGE_IN_MUSACREATEHWRTDATASET_TAG2
{
	MT_UINT32 ui32McgCoreNum;
	MT_DEV_VIRTADDR *psPMDataAddrMcg;
	MT_DEV_VIRTADDR *psTailPtrsDevVAddrMcg;
	MT_DEV_VIRTADDR sVHeapTableDevVAddr;
	MT_UINT64 ui64PPPMultiSampleCtl;
	MT_DEV_VIRTADDR *psPMDataAddr;
	MT_DEV_VIRTADDR *psPMSecureDataAddr;
	MT_DEV_VIRTADDR *psTailPtrsDevVAddr;
	MT_HANDLE *phKmHwRTDataSet;
	MT_HANDLE *phapsFreeLists;
	MT_UINT32 ui32FreeListsCount;
	MT_UINT32 ui32ISPMergeLowerX;
	MT_UINT32 ui32ISPMergeLowerY;
	MT_UINT32 ui32ISPMergeScaleX;
	MT_UINT32 ui32ISPMergeScaleY;
	MT_UINT32 ui32ISPMergeUpperX;
	MT_UINT32 ui32ISPMergeUpperY;
	MT_UINT32 ui32PPPScreen;
	MT_UINT32 ui32RgnStride;
	MT_UINT32 ui32TEAA;
	MT_UINT32 ui32TEMTILE1;
	MT_UINT32 ui32TEMTILE2;
	MT_UINT32 ui32TEScreen;
	MT_UINT32 ui32TPCSize;
	MT_UINT32 ui32TPCStride;
	MT_UINT16 ui16MaxRTs;
	MT_DEV_VIRTADDR *psPMVAFreeStackStateDevVAddr;
	MT_DEV_VIRTADDR sVASpaceSegmentDevVAddr;
} __packed MTGPU_BRIDGE_IN_MUSACREATEHWRTDATASET2;

/* Bridge out structure for MUSACreateHWRTDataSet */
typedef struct MTGPU_BRIDGE_OUT_MUSACREATEHWRTDATASET_TAG
{
	MT_HANDLE *phKmHwRTDataSet;
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_MUSACREATEHWRTDATASET;

/*******************************************
            MUSADestroyHWRTDataSet
 *******************************************/

/* Bridge in structure for MUSADestroyHWRTDataSet */
typedef struct MTGPU_BRIDGE_IN_MUSADESTROYHWRTDATASET_TAG
{
	MT_HANDLE hKmHwRTDataSet;
} __packed MTGPU_BRIDGE_IN_MUSADESTROYHWRTDATASET;

/* Bridge out structure for MUSADestroyHWRTDataSet */
typedef struct MTGPU_BRIDGE_OUT_MUSADESTROYHWRTDATASET_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_MUSADESTROYHWRTDATASET;

/*******************************************
            MUSACreateZSBuffer
 *******************************************/

/* Bridge in structure for MUSACreateZSBuffer */
typedef struct MTGPU_BRIDGE_IN_MUSACREATEZSBUFFER_TAG
{
	MT_HANDLE hPMR;
	MT_HANDLE hReservation;
	MTGPU_MEMALLOCFLAGS_T uiMapFlags;
} __packed MTGPU_BRIDGE_IN_MUSACREATEZSBUFFER;

/* Bridge out structure for MUSACreateZSBuffer */
typedef struct MTGPU_BRIDGE_OUT_MUSACREATEZSBUFFER_TAG
{
	MT_HANDLE hsZSBufferKM;
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_MUSACREATEZSBUFFER;

/*******************************************
            MUSADestroyZSBuffer
 *******************************************/

/* Bridge in structure for MUSADestroyZSBuffer */
typedef struct MTGPU_BRIDGE_IN_MUSADESTROYZSBUFFER_TAG
{
	MT_HANDLE hsZSBufferMemDesc;
} __packed MTGPU_BRIDGE_IN_MUSADESTROYZSBUFFER;

/* Bridge out structure for MUSADestroyZSBuffer */
typedef struct MTGPU_BRIDGE_OUT_MUSADESTROYZSBUFFER_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_MUSADESTROYZSBUFFER;

/*******************************************
            MUSAPopulateZSBuffer
 *******************************************/

/* Bridge in structure for MUSAPopulateZSBuffer */
typedef struct MTGPU_BRIDGE_IN_MUSAPOPULATEZSBUFFER_TAG
{
	MT_HANDLE hsZSBufferKM;
} __packed MTGPU_BRIDGE_IN_MUSAPOPULATEZSBUFFER;

/* Bridge out structure for MUSAPopulateZSBuffer */
typedef struct MTGPU_BRIDGE_OUT_MUSAPOPULATEZSBUFFER_TAG
{
	MT_HANDLE hsPopulation;
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_MUSAPOPULATEZSBUFFER;

/*******************************************
            MUSAUnpopulateZSBuffer
 *******************************************/

/* Bridge in structure for MUSAUnpopulateZSBuffer */
typedef struct MTGPU_BRIDGE_IN_MUSAUNPOPULATEZSBUFFER_TAG
{
	MT_HANDLE hsPopulation;
} __packed MTGPU_BRIDGE_IN_MUSAUNPOPULATEZSBUFFER;

/* Bridge out structure for MUSAUnpopulateZSBuffer */
typedef struct MTGPU_BRIDGE_OUT_MUSAUNPOPULATEZSBUFFER_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_MUSAUNPOPULATEZSBUFFER;

/*******************************************
            MUSACreateFreeList
 *******************************************/

/* Bridge in structure for MUSACreateFreeList */
typedef struct MTGPU_BRIDGE_IN_MUSACREATEFREELIST_TAG
{
	MT_DEV_VIRTADDR spsFreeListBaseDevVAddr;
	MT_DEV_VIRTADDR spsFreeListStateDevVAddr;
	MT_DEVMEM_OFFSET_T uiPMROffset;
	MT_DEVMEM_OFFSET_T uiPMRStateOffset;
	MT_HANDLE hMemCtxPrivData;
	MT_HANDLE hsFreeListPMR;
	MT_HANDLE hsFreeListStatePMR;
	MT_HANDLE hsGlobalFreeList;
	MT_BOOL bbFreeListCheck;
	MT_UINT32 ui32GrowFLPages;
	MT_UINT32 ui32GrowParamThreshold;
	MT_UINT32 ui32InitFLPages;
	MT_UINT32 ui32MaxFLPages;
} __packed MTGPU_BRIDGE_IN_MUSACREATEFREELIST;

/* Bridge out structure for MUSACreateFreeList */
typedef struct MTGPU_BRIDGE_OUT_MUSACREATEFREELIST_TAG
{
	MT_HANDLE hCleanupCookie;
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_MUSACREATEFREELIST;

/*******************************************
            MUSADestroyFreeList
 *******************************************/

/* Bridge in structure for MUSADestroyFreeList */
typedef struct MTGPU_BRIDGE_IN_MUSADESTROYFREELIST_TAG
{
	MT_HANDLE hCleanupCookie;
} __packed MTGPU_BRIDGE_IN_MUSADESTROYFREELIST;

/* Bridge out structure for MUSADestroyFreeList */
typedef struct MTGPU_BRIDGE_OUT_MUSADESTROYFREELIST_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_MUSADESTROYFREELIST;

/*******************************************
            MUSACreateRenderContext
 *******************************************/

/* Bridge in structure for MUSACreateRenderContext */
typedef struct MTGPU_BRIDGE_IN_MUSACREATERENDERCONTEXT_TAG
{
	MT_UINT64 ui64RobustnessAddress;
	MT_HANDLE hPrivData;
	MT_BYTE *pui8FrameworkCmd;
	MT_BYTE *pui8StaticRenderContextState;
	MT_UINT32 ui32ContextFlags;
	MT_UINT32 ui32FrameworkCmdSize;
	MT_UINT32 ui32Max3DDeadlineMS;
	MT_UINT32 ui32MaxTADeadlineMS;
	MT_UINT32 ui32PackedCCBSizeU8888;
	MT_UINT32 ui32Priority;
	MT_UINT32 ui32StaticRenderContextStateSize;
} __packed MTGPU_BRIDGE_IN_MUSACREATERENDERCONTEXT;

/* Bridge out structure for MUSACreateRenderContext */
typedef struct MTGPU_BRIDGE_OUT_MUSACREATERENDERCONTEXT_TAG
{
	MT_HANDLE hRenderContext;
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_MUSACREATERENDERCONTEXT;

/*******************************************
            MUSADestroyRenderContext
 *******************************************/

/* Bridge in structure for MUSADestroyRenderContext */
typedef struct MTGPU_BRIDGE_IN_MUSADESTROYRENDERCONTEXT_TAG
{
	MT_HANDLE hCleanupCookie;
} __packed MTGPU_BRIDGE_IN_MUSADESTROYRENDERCONTEXT;

/* Bridge out structure for MUSADestroyRenderContext */
typedef struct MTGPU_BRIDGE_OUT_MUSADESTROYRENDERCONTEXT_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_MUSADESTROYRENDERCONTEXT;

/*******************************************
            MUSASetRenderContextPriority
 *******************************************/

/* Bridge in structure for MUSASetRenderContextPriority */
typedef struct MTGPU_BRIDGE_IN_MUSASETRENDERCONTEXTPRIORITY_TAG
{
	MT_HANDLE hRenderContext;
	MT_UINT32 ui32Priority;
} __packed MTGPU_BRIDGE_IN_MUSASETRENDERCONTEXTPRIORITY;

/* Bridge out structure for MUSASetRenderContextPriority */
typedef struct MTGPU_BRIDGE_OUT_MUSASETRENDERCONTEXTPRIORITY_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_MUSASETRENDERCONTEXTPRIORITY;

/*******************************************
            MUSARenderContextStalled
 *******************************************/

/* Bridge in structure for MUSARenderContextStalled */
typedef struct MTGPU_BRIDGE_IN_MUSARENDERCONTEXTSTALLED_TAG
{
	MT_HANDLE hRenderContext;
} __packed MTGPU_BRIDGE_IN_MUSARENDERCONTEXTSTALLED;

/* Bridge out structure for MUSARenderContextStalled */
typedef struct MTGPU_BRIDGE_OUT_MUSARENDERCONTEXTSTALLED_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_MUSARENDERCONTEXTSTALLED;

/*******************************************
            MUSAKickGFX2
 *******************************************/

/* Bridge in structure for MUSAKickGFX2 */
typedef struct MTGPU_BRIDGE_IN_MUSAKICKGFX2_TAG
{
	MT_UINT64 ui64Deadline;
	MT_HANDLE hKMHWRTDataSet;
	MT_HANDLE hMSAAScratchBuffer;
	MT_HANDLE hPRFenceUFOSyncPrimBlock;
	MT_HANDLE hRenderContext;
	MT_HANDLE hZSBuffer;
	MT_UINT32 *pui32Client3DUpdateSyncOffset;
	MT_UINT32 *pui32Client3DUpdateValue;
	MT_UINT32 *pui32ClientTAFenceSyncOffset;
	MT_UINT32 *pui32ClientTAFenceValue;
	MT_UINT32 *pui32ClientTAUpdateSyncOffset;
	MT_UINT32 *pui32ClientTAUpdateValue;
	MT_UINT32 *pui32SyncPMRFlags;
	MT_BYTE *pui83DCmd;
	MT_BYTE *pui83DPRCmd;
	MT_BYTE *pui8TACmd;
	MT_CHAR *puiUpdateFenceName;
	MT_CHAR *puiUpdateFenceName3D;
	MT_HANDLE *phClient3DUpdateSyncPrimBlock;
	MT_HANDLE *phClientTAFenceSyncPrimBlock;
	MT_HANDLE *phClientTAUpdateSyncPrimBlock;
	MT_HANDLE *phSyncPMRs;
	MT_BOOL bbAbort;
	MT_BOOL bbKick3D;
	MT_BOOL bbKickPR;
	MT_BOOL bbKickTA;
	MTGPU_FENCE hCheckFence;
	MTGPU_FENCE hCheckFence3D;
	MTGPU_TIMELINE hUpdateTimeline;
	MTGPU_TIMELINE hUpdateTimeline3D;
	MT_UINT32 ui323DCmdSize;
	MT_UINT32 ui323DPRCmdSize;
	MT_UINT32 ui32Client3DUpdateCount;
	MT_UINT32 ui32ClientTAFenceCount;
	MT_UINT32 ui32ClientTAUpdateCount;
	MT_UINT32 ui32ExtJobRef;
	MT_UINT32 ui32NumberOfDrawCalls;
	MT_UINT32 ui32NumberOfIndices;
	MT_UINT32 ui32NumberOfMRTs;
	MT_UINT32 ui32PDumpFlags;
	MT_UINT32 ui32PRFenceUFOSyncOffset;
	MT_UINT32 ui32PRFenceValue;
	MT_UINT32 ui32RenderTargetSize;
	MT_UINT32 ui32SyncPMRCount;
	MT_UINT32 ui32TACmdSize;
} __packed MTGPU_BRIDGE_IN_MUSAKICKGFX2;

/* Bridge out structure for MUSAKickGFX2 */
typedef struct MTGPU_BRIDGE_OUT_MUSAKICKGFX2_TAG
{
	MTGPU_ERROR eError;
	MTGPU_FENCE hUpdateFence;
	MTGPU_FENCE hUpdateFence3D;
} __packed MTGPU_BRIDGE_OUT_MUSAKICKGFX2;

/* Bridge in structure for MUSAKickGFX3 */
typedef struct MTGPU_BRIDGE_IN_MUSAKICKGFX3_TAG
{
	MT_UINT64 ui64Deadline;
	MT_HANDLE hKMHWRTDataSet;
	MT_HANDLE hMSAAScratchBuffer;
	MT_HANDLE hPRFenceUFOSyncPrimBlock;
	MT_HANDLE hRenderContext;
	MT_HANDLE hZSBuffer;
	MT_UINT32 *pui32Client3DUpdateSyncOffset;
	MT_UINT32 *pui32Client3DUpdateValue;
	MT_UINT32 *pui32ClientTAFenceSyncOffset;
	MT_UINT32 *pui32ClientTAFenceValue;
	MT_UINT32 *pui32ClientTAUpdateSyncOffset;
	MT_UINT32 *pui32ClientTAUpdateValue;
	MT_UINT32 *pui32SyncPMRFlags;
	MT_BYTE *pui83DCmd;
	MT_BYTE *pui83DPRCmd;
	MT_BYTE *pui8TACmd;
	MT_CHAR *puiUpdateFenceName;
	MT_CHAR *puiUpdateFenceName3D;
	MT_HANDLE *phClient3DUpdateSyncPrimBlock;
	MT_HANDLE *phClientTAFenceSyncPrimBlock;
	MT_HANDLE *phClientTAUpdateSyncPrimBlock;
	MT_HANDLE *phSyncPMRs;
	MT_BOOL bbAbort;
	MT_BOOL bbKick3D;
	MT_BOOL bbKickPR;
	MT_BOOL bbKickTA;
	MTGPU_FENCE hCheckFence;
	MTGPU_FENCE hCheckFence3D;
	MTGPU_FENCE hUpdateFence;
	MTGPU_FENCE hUpdateFence3D;
	MT_UINT32 ui323DCmdSize;
	MT_UINT32 ui323DPRCmdSize;
	MT_UINT32 ui32Client3DUpdateCount;
	MT_UINT32 ui32ClientTAFenceCount;
	MT_UINT32 ui32ClientTAUpdateCount;
	MT_UINT32 ui32ExtJobRef;
	MT_UINT32 ui32NumberOfDrawCalls;
	MT_UINT32 ui32NumberOfIndices;
	MT_UINT32 ui32NumberOfMRTs;
	MT_UINT32 ui32PDumpFlags;
	MT_UINT32 ui32PRFenceUFOSyncOffset;
	MT_UINT32 ui32PRFenceValue;
	MT_UINT32 ui32RenderTargetSize;
	MT_UINT32 ui32SyncPMRCount;
	MT_UINT32 ui32TACmdSize;
} __packed MTGPU_BRIDGE_IN_MUSAKICKGFX3;

typedef struct MTGPU_BRIDGE_OUT_MUSAKICKGFX3_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_MUSAKICKGFX3;

/*******************************************
            MUSAKickGFX4
 *******************************************/

/* Bridge in structure for MUSAKickGFX4 */
typedef struct MTGPU_BRIDGE_IN_MUSAKICKGFX4_TAG
{
	MT_UINT64 ui64Deadline;
	MT_HANDLE hKMHWRTDataSet;
	MT_HANDLE hMSAAScratchBuffer;
	MT_HANDLE hPRFenceUFOSyncPrimBlock;
	MT_HANDLE hRenderContext;
	MT_HANDLE hZSBuffer;
	MT_UINT32 *pui32Client3DUpdateSyncOffset;
	MT_UINT32 *pui32Client3DUpdateValue;
	MT_UINT32 *pui32ClientTAFenceSyncOffset;
	MT_UINT32 *pui32ClientTAFenceValue;
	MT_UINT32 *pui32ClientTAUpdateSyncOffset;
	MT_UINT32 *pui32ClientTAUpdateValue;
	MT_UINT32 *pui32SyncPMRFlags;
	MT_BYTE *pui83DCmd;
	MT_BYTE *pui83DPRCmd;
	MT_BYTE *pui8TACmd;
	MT_CHAR *puiUpdateFenceName;
	MT_CHAR *puiUpdateFenceName3D;
	MT_HANDLE *phClient3DUpdateSyncPrimBlock;
	MT_HANDLE *phClientTAFenceSyncPrimBlock;
	MT_HANDLE *phClientTAUpdateSyncPrimBlock;
	MT_HANDLE *phSyncPMRs;
	MT_BOOL bbAbort;
	MT_BOOL bbKick3D;
	MT_BOOL bbKickPR;
	MT_BOOL bbKickTA;
	MTGPU_FENCE hCheckFence;
	MTGPU_FENCE hCheckFence3D;
	MTGPU_TIMELINE hUpdateTimeline;
	MTGPU_TIMELINE hUpdateTimeline3D;
	MT_UINT32 ui323DCmdSize;
	MT_UINT32 ui323DPRCmdSize;
	MT_UINT32 ui32Client3DUpdateCount;
	MT_UINT32 ui32ClientTAFenceCount;
	MT_UINT32 ui32ClientTAUpdateCount;
	MT_UINT32 ui32ExtJobRef;
	MT_UINT32 ui32NumberOfDrawCalls;
	MT_UINT32 ui32NumberOfIndices;
	MT_UINT32 ui32NumberOfMRTs;
	MT_UINT32 ui32PDumpFlags;
	MT_UINT32 ui32PRFenceUFOSyncOffset;
	MT_UINT32 ui32PRFenceValue;
	MT_UINT32 ui32RenderTargetSize;
	MT_UINT32 ui32SyncPMRCount;
	MT_UINT32 ui32TACmdSize;
} __packed MTGPU_BRIDGE_IN_MUSAKICKGFX4;

/* Bridge out structure for MUSAKickGFX2 */
typedef struct MTGPU_BRIDGE_OUT_MUSAKICKGFX4_TAG
{
	MTGPU_ERROR eError;
	MTGPU_FENCE hUpdateFence;
	MTGPU_FENCE hUpdateFence3D;
} __packed MTGPU_BRIDGE_OUT_MUSAKICKGFX4;

/*******************************************
                  DDK2.0
 *******************************************/

/* Bridge in structure for MUSACreateRenderContext ddk2.0 */
typedef struct MTGPU_BRIDGE_IN_MUSACREATERENDERCONTEXT2_TAG
{
	MT_HANDLE hPrivData;
	MT_UINT32 ui32Priority;
} __packed MTGPU_BRIDGE_IN_MUSACREATERENDERCONTEXT2;

/* Bridge out structure for MUSACreateRenderContext ddk2.0 */
typedef struct MTGPU_BRIDGE_OUT_MUSACREATERENDERCONTEXT2_TAG
{
	MT_HANDLE hRenderContext;
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_MUSACREATERENDERCONTEXT2;

/* Bridge in structure for MUSADestroyRenderContext ddk2.0 */
typedef struct MTGPU_BRIDGE_IN_MUSADESTROYRENDERCONTEXT2_TAG
{
	MT_HANDLE hRenderContext;
} __packed MTGPU_BRIDGE_IN_MUSADESTROYRENDERCONTEXT2;

/* Bridge out structure for MUSADestroyRenderContext ddk2.0 */
typedef struct MTGPU_BRIDGE_OUT_MUSADESTROYRENDERCONTEXT2_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_MUSADESTROYRENDERCONTEXT2;

/* Bridge in structure for MUSAKickGFX5 ddk2.0*/
typedef struct MTGPU_BRIDGE_IN_MUSAKICKGFX5_TAG
{
	MT_HANDLE hRenderContext;

	MT_HANDLE* phCheckUFOSyncPrimBlock;
	MT_UINT32* pui32CheckSyncOffset;
	MT_UINT64* pui64CheckValue;

	MT_HANDLE* phUpdateUFOSyncPrimBlock;
	MT_UINT32* pui32UpdateSyncOffset;
	MT_UINT64* pui64UpdateValue;

	MT_UINT32* pui32SyncPMRFlags;
	MT_HANDLE* phSyncPMRs;

	MT_UINT32  ui32SubmissionFlags;
	MT_UINT64  ui64SubmissionVa;
	MT_UINT32  ui32SubmissionSize;
	MT_UINT64  ui64SubmissionId;

	MT_UINT32  ui32CheckCount;
	MT_UINT32  ui32UpdateCount;
	MT_UINT32  ui32SyncPMRCount;
} __packed MTGPU_BRIDGE_IN_MUSAKICKGFX5;

/* Bridge out structure for MUSAKickGFX5 ddk2.0*/
typedef struct MTGPU_BRIDGE_OUT_MUSAKICKGFX5_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_MUSAKICKGFX5;

/*******************************************
            MUSASetRenderContextProperty
 *******************************************/

/* Bridge in structure for MUSASetRenderContextProperty */
typedef struct MTGPU_BRIDGE_IN_MUSASETRENDERCONTEXTPROPERTY_TAG
{
	MT_UINT64 ui64Input;
	MT_HANDLE hRenderContext;
	MT_UINT32 ui32Property;
} __packed MTGPU_BRIDGE_IN_MUSASETRENDERCONTEXTPROPERTY;

/* Bridge out structure for MUSASetRenderContextProperty */
typedef struct MTGPU_BRIDGE_OUT_MUSASETRENDERCONTEXTPROPERTY_TAG
{
	MT_UINT64 ui64Output;
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_MUSASETRENDERCONTEXTPROPERTY;

/* Bridge in struct MUSA Create TA Or 3D Fence */
typedef struct MTGPU_BRIDGE_IN_MUSACREATEGFXFENCEPROPERTY_TAG
{
	MT_CHAR puiUpdateFenceName[32];
	MTGPU_TIMELINE hUpdateTimeline;
	MT_HANDLE hRenderContext;
} __packed MTGPU_BRIDGE_IN_MUSACREATEGFXFENCEPROPERTY;

/* Bridge out structure for  MUSA Create TA Or 3D Fence */
typedef struct MTGPU_BRIDGE_OUT_MUSACREATEGFXFENCEPROPERTY_TAG
{
	MTGPU_FENCE hFence;
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_MUSACREATEGFXFENCEPROPERTY;

#endif /* COMMON_MUSAGFX_BRIDGE_H */

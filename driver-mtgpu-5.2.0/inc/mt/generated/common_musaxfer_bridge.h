/*******************************************************************************
@File
@Title          Common bridge header for musaxfer
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Declares common defines and structures used by both the client
                and server side of the bridge for musaxfer
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

#ifndef COMMON_MUSAXFER_BRIDGE_H
#define COMMON_MUSAXFER_BRIDGE_H

#include <mt/mem_types.h>

#include "img_defs.h"
#include "img_types.h"
#include "mtgpu_error.h"

#include "mtgpu_sync_km.h"

#define MTGPU_BRIDGE_MUSAXFER_CMD_FIRST						0
#define MTGPU_BRIDGE_MUSAXFER_MUSACREATETRANSFERCONTEXT			MTGPU_BRIDGE_MUSAXFER_CMD_FIRST+0
#define MTGPU_BRIDGE_MUSAXFER_MUSADESTROYTRANSFERCONTEXT			MTGPU_BRIDGE_MUSAXFER_CMD_FIRST+1
#define MTGPU_BRIDGE_MUSAXFER_MUSASETTRANSFERCONTEXTPRIORITY			MTGPU_BRIDGE_MUSAXFER_CMD_FIRST+2
#define MTGPU_BRIDGE_MUSAXFER_MUSANOTIFYWRITEOFFSETUPDATE			MTGPU_BRIDGE_MUSAXFER_CMD_FIRST+3
#define MTGPU_BRIDGE_MUSAXFER_MUSASUBMITTRANSFER2				MTGPU_BRIDGE_MUSAXFER_CMD_FIRST+4
#define MTGPU_BRIDGE_MUSAXFER_MUSAGETSHAREDMEMORY				MTGPU_BRIDGE_MUSAXFER_CMD_FIRST+5
#define MTGPU_BRIDGE_MUSAXFER_MUSARELEASESHAREDMEMORY				MTGPU_BRIDGE_MUSAXFER_CMD_FIRST+6
#define MTGPU_BRIDGE_MUSAXFER_MUSASETTRANSFERCONTEXTPROPERTY			MTGPU_BRIDGE_MUSAXFER_CMD_FIRST+7
/* Just for DDK2.0 */
#define MTGPU_BRIDGE_MUSAXFER_MUSACREATETRANSFERCONTEXT2			MTGPU_BRIDGE_MUSAXFER_CMD_FIRST+8
#define MTGPU_BRIDGE_MUSAXFER_MUSADESTROYTRANSFERCONTEXT2			MTGPU_BRIDGE_MUSAXFER_CMD_FIRST+9
#define MTGPU_BRIDGE_MUSAXFER_MUSASUBMITTRANSFER3				MTGPU_BRIDGE_MUSAXFER_CMD_FIRST+10
#define MTGPU_BRIDGE_MUSAXFER_CMD_LAST						(MTGPU_BRIDGE_MUSAXFER_CMD_FIRST+10)

/*******************************************
            MUSAXFERCreateTransferContext
 *******************************************/

/* Bridge in structure for MUSAXFERCreateTransferContext */
typedef struct MTGPU_BRIDGE_IN_MUSAXFERCREATETRANSFERCONTEXT_TAG
{
	MT_UINT64 ui64RobustnessAddress;
	MT_HANDLE hPrivData;
	MT_BYTE *pui8FrameworkCmd;
	MT_UINT32 ui32ContextFlags;
	MT_UINT32 ui32FrameworkCmdSize;
	MT_UINT32 ui32PackedCCBSizeU88;
	MT_UINT32 ui32Priority;
} __packed MTGPU_BRIDGE_IN_MUSAXFERCREATETRANSFERCONTEXT;

/* Bridge out structure for MUSAXFERCreateTransferContext */
typedef struct MTGPU_BRIDGE_OUT_MUSAXFERCREATETRANSFERCONTEXT_TAG
{
	MT_HANDLE hTransferContext;
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_MUSAXFERCREATETRANSFERCONTEXT;

/*******************************************
            MUSAXFERDestroyTransferContext
 *******************************************/

/* Bridge in structure for MUSAXFERDestroyTransferContext */
typedef struct MTGPU_BRIDGE_IN_MUSAXFERDESTROYTRANSFERCONTEXT_TAG
{
	MT_HANDLE hTransferContext;
} __packed MTGPU_BRIDGE_IN_MUSAXFERDESTROYTRANSFERCONTEXT;

/* Bridge out structure for MUSAXFERDestroyTransferContext */
typedef struct MTGPU_BRIDGE_OUT_MUSAXFERDESTROYTRANSFERCONTEXT_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_MUSAXFERDESTROYTRANSFERCONTEXT;

/*******************************************
            MUSAXFERSetTransferContextPriority
 *******************************************/

/* Bridge in structure for MUSAXFERSetTransferContextPriority */
typedef struct MTGPU_BRIDGE_IN_MUSAXFERSETTRANSFERCONTEXTPRIORITY_TAG
{
	MT_HANDLE hTransferContext;
	MT_UINT32 ui32Priority;
} __packed MTGPU_BRIDGE_IN_MUSAXFERSETTRANSFERCONTEXTPRIORITY;

/* Bridge out structure for MUSAXFERSetTransferContextPriority */
typedef struct MTGPU_BRIDGE_OUT_MUSAXFERSETTRANSFERCONTEXTPRIORITY_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_MUSAXFERSETTRANSFERCONTEXTPRIORITY;

/*******************************************
            MUSAXFERNotifyWriteOffsetUpdate
 *******************************************/

/* Bridge in structure for MUSAXFERNotifyWriteOffsetUpdate */
typedef struct MTGPU_BRIDGE_IN_MUSAXFERNOTIFYWRITEOFFSETUPDATE_TAG
{
	MT_HANDLE hTransferContext;
	MT_UINT32 ui32PDumpFlags;
} __packed MTGPU_BRIDGE_IN_MUSAXFERNOTIFYWRITEOFFSETUPDATE;

/* Bridge out structure for MUSAXFERNotifyWriteOffsetUpdate */
typedef struct MTGPU_BRIDGE_OUT_MUSAXFERNOTIFYWRITEOFFSETUPDATE_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_MUSAXFERNOTIFYWRITEOFFSETUPDATE;

/*******************************************
            MUSAXFERSubmitTransfer2
 *******************************************/

/* Bridge in structure for MUSAXFERSubmitTransfer2 */
typedef struct MTGPU_BRIDGE_IN_MUSAXFERSUBMITTRANSFER2_TAG
{
	MT_UINT64 ui64DeadlineInus;
	MT_HANDLE hTransferContext;
	MT_UINT32 *pui32SyncPMRFlags;
	MT_UINT32 *pui32UpdateSyncOffset;
	MT_UINT32 *pui32UpdateValue;
	MT_UINT8 *pui8FWCommand;
	MT_CHAR *puiUpdateFenceName;
	MT_HANDLE *phSyncPMRs;
	MT_HANDLE *phUpdateUFOSyncPrimBlock;
	MTGPU_FENCE hCheckFenceFD;
	MTGPU_TIMELINE hUpdateTimeline;
	MT_UINT32 ui32Characteristic1;
	MT_UINT32 ui32Characteristic2;
	MT_UINT32 ui32ClientUpdateCount;
	MT_UINT32 ui32CommandSize;
	MT_UINT32 ui32ExternalJobReference;
	MT_UINT32 ui32PDumpFlags;
	MT_UINT32 ui32SyncPMRCount;
} __packed MTGPU_BRIDGE_IN_MUSAXFERSUBMITTRANSFER2;

/* Bridge out structure for MUSAXFERSubmitTransfer2 */
typedef struct MTGPU_BRIDGE_OUT_MUSAXFERSUBMITTRANSFER2_TAG
{
	MTGPU_ERROR eError;
	MTGPU_FENCE hUpdateFence;
} __packed MTGPU_BRIDGE_OUT_MUSAXFERSUBMITTRANSFER2;

/*******************************************
            MUSAXFERGetSharedMemory
 *******************************************/

/* Bridge in structure for MUSAXFERGetSharedMemory */
typedef struct MTGPU_BRIDGE_IN_MUSAXFERGETSHAREDMEMORY_TAG
{
	MT_UINT32 ui32EmptyStructPlaceholder;
} __packed MTGPU_BRIDGE_IN_MUSAXFERGETSHAREDMEMORY;

/* Bridge out structure for MUSAXFERGetSharedMemory */
typedef struct MTGPU_BRIDGE_OUT_MUSAXFERGETSHAREDMEMORY_TAG
{
	MT_HANDLE hCLIPMRMem;
	MT_HANDLE hMPPMRMem;
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_MUSAXFERGETSHAREDMEMORY;

/*******************************************
            MUSAXFERReleaseSharedMemory
 *******************************************/

/* Bridge in structure for MUSAXFERReleaseSharedMemory */
typedef struct MTGPU_BRIDGE_IN_MUSAXFERRELEASESHAREDMEMORY_TAG
{
	MT_HANDLE hPMRMem;
} __packed MTGPU_BRIDGE_IN_MUSAXFERRELEASESHAREDMEMORY;

/* Bridge out structure for MUSAXFERReleaseSharedMemory */
typedef struct MTGPU_BRIDGE_OUT_MUSAXFERRELEASESHAREDMEMORY_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_MUSAXFERRELEASESHAREDMEMORY;

/*******************************************
            MUSAXFERSetTransferContextProperty
 *******************************************/

/* Bridge in structure for MUSAXFERSetTransferContextProperty */
typedef struct MTGPU_BRIDGE_IN_MUSAXFERSETTRANSFERCONTEXTPROPERTY_TAG
{
	MT_UINT64 ui64Input;
	MT_HANDLE hTransferContext;
	MT_UINT32 ui32Property;
} __packed MTGPU_BRIDGE_IN_MUSAXFERSETTRANSFERCONTEXTPROPERTY;

/* Bridge out structure for MUSAXFERSetTransferContextProperty */
typedef struct MTGPU_BRIDGE_OUT_MUSAXFERSETTRANSFERCONTEXTPROPERTY_TAG
{
	MT_UINT64 ui64Output;
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_MUSAXFERSETTRANSFERCONTEXTPROPERTY;

/* Just for DDK2.0 */
typedef struct MTGPU_BRIDGE_IN_MUSAXFERCREATETRANSFERCONTEXT2_TAG
{
	MT_HANDLE hPrivData;
	MT_UINT32 ui32Priority;
} __packed MTGPU_BRIDGE_IN_MUSAXFERCREATETRANSFERCONTEXT2;

typedef struct MTGPU_BRIDGE_OUT_MUSAXFERCREATETRANSFERCONTEXT2_TAG
{
	MT_HANDLE hTransferContext;
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_MUSAXFERCREATETRANSFERCONTEXT2;

typedef struct MTGPU_BRIDGE_IN_MUSAXFERDESTROYTRANSFERCONTEXT2_TAG
{
	MT_HANDLE hTransferContext;
} __packed MTGPU_BRIDGE_IN_MUSAXFERDESTROYTRANSFERCONTEXT2;

typedef struct MTGPU_BRIDGE_OUT_MUSAXFERDESTROYTRANSFERCONTEXT2_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_MUSAXFERDESTROYTRANSFERCONTEXT2;

typedef struct MTGPU_BRIDGE_IN_MUSAXFERSUBMITTRANSFER3_TAG
{
	MT_HANDLE  hTransferContext;

	MT_UINT32  ui32ClientCheckCount;
	MT_HANDLE* phCheckUFOSyncPrimBlock;
	MT_UINT32* pui32CheckSyncOffset;
	MT_UINT64* pui64CheckValue;

	MT_UINT32  ui32ClientUpdateCount;
	MT_HANDLE* phUpdateUFOSyncPrimBlock;
	MT_UINT32* pui32UpdateSyncOffset;
	MT_UINT64* pui64UpdateValue;

	MT_UINT32  ui32SyncPMRCount;
	MT_UINT32* pui32SyncPMRFlags;
	MT_HANDLE* phSyncPMRs;

	MT_UINT32  ui32SubmissionFlags;
	MT_UINT64  ui64SubmissionVa;
	MT_UINT64  ui64SubmissionId;
	MT_UINT32  ui32SubmissionSize;
} __packed MTGPU_BRIDGE_IN_MUSAXFERSUBMITTRANSFER3;

typedef struct MTGPU_BRIDGE_OUT_MUSAXFERSUBMITTRANSFER3_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_MUSAXFERSUBMITTRANSFER3;

#endif /* COMMON_MUSAXFER_BRIDGE_H */

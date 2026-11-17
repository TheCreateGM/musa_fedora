/*******************************************************************************
@File
@Title          Common bridge header for musakicksync
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Declares common defines and structures used by both the client
                and server side of the bridge for musakicksync
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

#ifndef COMMON_MUSAKICKSYNC_BRIDGE_H
#define COMMON_MUSAKICKSYNC_BRIDGE_H

#include <mt/mem_types.h>

#include "img_defs.h"
#include "img_types.h"
#include "mtgpu_error.h"

#include "mtgpu_sync_km.h"

#define MTGPU_BRIDGE_MUSAKICKSYNC_CMD_FIRST					0
#define MTGPU_BRIDGE_MUSAKICKSYNC_MUSACREATEKICKSYNCCONTEXT			MTGPU_BRIDGE_MUSAKICKSYNC_CMD_FIRST+0
#define MTGPU_BRIDGE_MUSAKICKSYNC_MUSADESTROYKICKSYNCCONTEXT			MTGPU_BRIDGE_MUSAKICKSYNC_CMD_FIRST+1
#define MTGPU_BRIDGE_MUSAKICKSYNC_MUSAKICKSYNC2					MTGPU_BRIDGE_MUSAKICKSYNC_CMD_FIRST+2
#define MTGPU_BRIDGE_MUSAKICKSYNC_MUSASETKICKSYNCCONTEXTPROPERTY			MTGPU_BRIDGE_MUSAKICKSYNC_CMD_FIRST+3
#define MTGPU_BRIDGE_MUSAKICKSYNC_MUSAKICKSYNC3					MTGPU_BRIDGE_MUSAKICKSYNC_CMD_FIRST+4
#define MTGPU_BRIDGE_MUSAKICKSYNC_MUSACREATEKICKSYNCCONTEXT2			MTGPU_BRIDGE_MUSAKICKSYNC_CMD_FIRST+5
#define MTGPU_BRIDGE_MUSAKICKSYNC_MUSADESTROYKICKSYNCCONTEXT2			MTGPU_BRIDGE_MUSAKICKSYNC_CMD_FIRST+6
#define MTGPU_BRIDGE_MUSAKICKSYNC_MUSAKICKSYNC4					MTGPU_BRIDGE_MUSAKICKSYNC_CMD_FIRST+7
#define MTGPU_BRIDGE_MUSAKICKSYNC_CMD_LAST					(MTGPU_BRIDGE_MUSAKICKSYNC_CMD_FIRST+7)

/*******************************************
            MUSACreateKickSyncContext
 *******************************************/

/* Bridge in structure for MUSACreateKickSyncContext */
typedef struct MTGPU_BRIDGE_IN_MUSACREATEKICKSYNCCONTEXT_TAG
{
	MT_HANDLE hPrivData;
	MT_UINT32 ui32ContextFlags;
	MT_UINT32 ui32PackedCCBSizeU88;
} __packed MTGPU_BRIDGE_IN_MUSACREATEKICKSYNCCONTEXT;

/* Bridge out structure for MUSACreateKickSyncContext */
typedef struct MTGPU_BRIDGE_OUT_MUSACREATEKICKSYNCCONTEXT_TAG
{
	MT_HANDLE hKickSyncContext;
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_MUSACREATEKICKSYNCCONTEXT;

/*******************************************
            MUSADestroyKickSyncContext
 *******************************************/

/* Bridge in structure for MUSADestroyKickSyncContext */
typedef struct MTGPU_BRIDGE_IN_MUSADESTROYKICKSYNCCONTEXT_TAG
{
	MT_HANDLE hKickSyncContext;
} __packed MTGPU_BRIDGE_IN_MUSADESTROYKICKSYNCCONTEXT;

/* Bridge out structure for MUSADestroyKickSyncContext */
typedef struct MTGPU_BRIDGE_OUT_MUSADESTROYKICKSYNCCONTEXT_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_MUSADESTROYKICKSYNCCONTEXT;

/*******************************************
            MUSAKickSync2
 *******************************************/

/* Bridge in structure for MUSAKickSync2 */
typedef struct MTGPU_BRIDGE_IN_MUSAKICKSYNC2_TAG
{
	MT_HANDLE hKickSyncContext;
	MT_UINT32 *pui32UpdateDevVarOffset;
	MT_UINT32 *pui32UpdateValue;
	MT_CHAR *puiUpdateFenceName;
	MT_HANDLE *phUpdateUFODevVarBlock;
	MTGPU_FENCE hCheckFenceFD;
	MTGPU_TIMELINE hTimelineFenceFD;
	MT_UINT32 ui32ClientUpdateCount;
	MT_UINT32 ui32ExtJobRef;
} __packed MTGPU_BRIDGE_IN_MUSAKICKSYNC2;

/* Bridge out structure for MUSAKickSync2 */
typedef struct MTGPU_BRIDGE_OUT_MUSAKICKSYNC2_TAG
{
	MTGPU_ERROR eError;
	MTGPU_FENCE hUpdateFenceFD;
} __packed MTGPU_BRIDGE_OUT_MUSAKICKSYNC2;

/*******************************************
            MUSAKickSync3
 *******************************************/

/* Bridge in structure for MUSAKickSync3 */
typedef struct MTGPU_BRIDGE_IN_MUSAKICKSYNC3_TAG
{
	MT_HANDLE hKickSyncContext;
	MT_UINT32 *pui32CheckDevVarOffset;
	MT_UINT32 *pui32CheckValue;
	MT_HANDLE *phCheckUFODevVarBlock;
	MT_UINT32 ui32ClientCheckCount;
	MT_UINT32 *pui32UpdateDevVarOffset;
	MT_UINT32 *pui32UpdateValue;
	MT_HANDLE *phUpdateUFODevVarBlock;
	MT_UINT32 ui32ClientUpdateCount;
	MT_CHAR *puiUpdateFenceName;
	MTGPU_FENCE hCheckFenceFD;
	MTGPU_TIMELINE hTimelineFenceFD;
	MT_UINT32 ui32ExtJobRef;
} __packed MTGPU_BRIDGE_IN_MUSAKICKSYNC3;

/* Bridge out structure for MUSAKickSync3 */
typedef struct MTGPU_BRIDGE_OUT_MUSAKICKSYNC3_TAG
{
	MTGPU_ERROR eError;
	MTGPU_FENCE hUpdateFenceFD;
} __packed MTGPU_BRIDGE_OUT_MUSAKICKSYNC3;

/*******************************************
            MUSASetKickSyncContextProperty
 *******************************************/

/* Bridge in structure for MUSASetKickSyncContextProperty */
typedef struct MTGPU_BRIDGE_IN_MUSASETKICKSYNCCONTEXTPROPERTY_TAG
{
	MT_UINT64 ui64Input;
	MT_HANDLE hKickSyncContext;
	MT_UINT32 ui32Property;
} __packed MTGPU_BRIDGE_IN_MUSASETKICKSYNCCONTEXTPROPERTY;

/* Bridge out structure for MUSASetKickSyncContextProperty */
typedef struct MTGPU_BRIDGE_OUT_MUSASETKICKSYNCCONTEXTPROPERTY_TAG
{
	MT_UINT64 ui64Output;
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_MUSASETKICKSYNCCONTEXTPROPERTY;

/*******************************************
            MUSACreateKickSyncContext2
 *******************************************/

/* Bridge in structure for MUSACreateKickSyncContext2 */
typedef struct MTGPU_BRIDGE_IN_MUSACREATEKICKSYNCCONTEXT2_TAG
{
	MT_HANDLE hPrivData;
} __packed MTGPU_BRIDGE_IN_MUSACREATEKICKSYNCCONTEXT2;

/* Bridge out structure for MUSACreateKickSyncContext2 */
typedef struct MTGPU_BRIDGE_OUT_MUSACREATEKICKSYNCCONTEXT2_TAG
{
	MT_HANDLE hKickSyncContext;
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_MUSACREATEKICKSYNCCONTEXT2;

/*******************************************
            MUSADestroyKickSyncContext2
 *******************************************/

/* Bridge in structure for MUSADestroyKickSyncContext2 */
typedef struct MTGPU_BRIDGE_IN_MUSADESTROYKICKSYNCCONTEXT2_TAG
{
	MT_HANDLE hKickSyncContext;
} __packed MTGPU_BRIDGE_IN_MUSADESTROYKICKSYNCCONTEXT2;

/* Bridge out structure for MUSADestroyKickSyncContext2 */
typedef struct MTGPU_BRIDGE_OUT_MUSADESTROYKICKSYNCCONTEXT2_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_MUSADESTROYKICKSYNCCONTEXT2;

/*******************************************
            MUSAKickSync4
 *******************************************/

/* Bridge in structure for MUSAKickSync4 */
typedef struct MTGPU_BRIDGE_IN_MUSAKICKSYNC4_TAG
{
	MT_HANDLE hKickSyncContext;
	/* check SyncPrim */
	MT_UINT32 ui32ClientCheckCount;
	MT_HANDLE *phCheckUFOSyncPrimBlock;
	MT_UINT32 *pui32CheckSyncOffset;
	MT_UINT64 *pui64CheckValue;
	/* update SyncPrim */
	MT_UINT32  ui32ClientUpdateCount;
	MT_HANDLE *phUpdateUFOSyncPrimBlock;
	MT_UINT32 *pui32UpdateSyncOffset;
	MT_UINT64 *pui64UpdateValue;
	MT_UINT32 ui32SubmissionFlags;
} __packed MTGPU_BRIDGE_IN_MUSAKICKSYNC4;

/* Bridge out structure for MUSAKickSync4 */
typedef struct MTGPU_BRIDGE_OUT_MUSAKICKSYNC4_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_MUSAKICKSYNC4;

#endif /* COMMON_MUSAKICKSYNC_BRIDGE_H */

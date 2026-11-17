/*******************************************************************************
@File
@Title          Common bridge header for musacmp
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Declares common defines and structures used by both the client
                and server side of the bridge for musacmp
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

#ifndef COMMON_MUSACMP_BRIDGE_H
#define COMMON_MUSACMP_BRIDGE_H

#include <mt/mem_types.h>

#include "img_defs.h"
#include "img_types.h"
#include "mtgpu_error.h"

#include "mtgpu_sync_km.h"

#define MTGPU_BRIDGE_MUSACMP_CMD_FIRST			0
#define MTGPU_BRIDGE_MUSACMP_MUSACREATECOMPCONTEXT		(MTGPU_BRIDGE_MUSACMP_CMD_FIRST + 0)
#define MTGPU_BRIDGE_MUSACMP_MUSADESTROYCOMPCONTEXT		(MTGPU_BRIDGE_MUSACMP_CMD_FIRST + 1)
#define MTGPU_BRIDGE_MUSACMP_MUSAFLUSHCOMPDATA		(MTGPU_BRIDGE_MUSACMP_CMD_FIRST + 2)
#define MTGPU_BRIDGE_MUSACMP_MUSASETCOMPCONTEXTPRIORITY	(MTGPU_BRIDGE_MUSACMP_CMD_FIRST + 3)
#define MTGPU_BRIDGE_MUSACMP_MUSANOTIFYCOMPWRITEOFFSETUPDATE	(MTGPU_BRIDGE_MUSACMP_CMD_FIRST + 4)
#define MTGPU_BRIDGE_MUSACMP_MUSAKICKCMP2			(MTGPU_BRIDGE_MUSACMP_CMD_FIRST + 5)
#define MTGPU_BRIDGE_MUSACMP_MUSASETCOMPCONTEXTPROPERTY	(MTGPU_BRIDGE_MUSACMP_CMD_FIRST + 6)
#define MTGPU_BRIDGE_MUSACMP_MUSAGETLASTDEVICEERROR		(MTGPU_BRIDGE_MUSACMP_CMD_FIRST + 7)
#define MTGPU_BRIDGE_MUSACMP_MUSAGETLLCPERSISTINGPROPERTIES	(MTGPU_BRIDGE_MUSACMP_CMD_FIRST + 8)
#define MTGPU_BRIDGE_MUSACMP_MUSASETLLCPERSISTINGPROPERTIES	(MTGPU_BRIDGE_MUSACMP_CMD_FIRST + 9)
#define MTGPU_BRIDGE_MUSACMP_MUSARESETLLCPERSISTING		(MTGPU_BRIDGE_MUSACMP_CMD_FIRST + 10)
#define MTGPU_BRIDGE_MUSACMP_MUSACREATEFENCE			(MTGPU_BRIDGE_MUSACMP_CMD_FIRST + 11)
#define MTGPU_BRIDGE_MUSACMP_MUSAKICKCMP3			(MTGPU_BRIDGE_MUSACMP_CMD_FIRST + 12)
#define MTGPU_BRIDGE_MUSACMP_DECODEFAULTINFO			(MTGPU_BRIDGE_MUSACMP_CMD_FIRST + 13)
/* Just for DDK2.0 */
#define MTGPU_BRIDGE_MUSACMP_MUSACREATECOMPCONTEXT2		(MTGPU_BRIDGE_MUSACMP_CMD_FIRST + 14)
#define MTGPU_BRIDGE_MUSACMP_MUSADESTROYCOMPCONTEXT2		(MTGPU_BRIDGE_MUSACMP_CMD_FIRST + 15)
#define MTGPU_BRIDGE_MUSACMP_MUSAKICKCMP4			(MTGPU_BRIDGE_MUSACMP_CMD_FIRST + 16)
#define MTGPU_BRIDGE_MUSACMP_CMD_LAST				(MTGPU_BRIDGE_MUSACMP_CMD_FIRST + 16)

/*******************************************
            MUSACreateComputeContext
 *******************************************/

/* Bridge in structure for MUSACreateComputeContext */
typedef struct MTGPU_BRIDGE_IN_MUSACREATECOMPCONTEXT_TAG
{
	MT_UINT64 ui64RobustnessAddress;
	MT_HANDLE hPrivData;
	MT_BYTE *pui8FrameworkCmd;
	MT_BYTE *pui8StaticComputeContextState;
	MT_UINT32 ui32ContextFlags;
	MT_UINT32 ui32FrameworkCmdSize;
	MT_UINT64 ui64MaxDeadlineMS;
	MT_UINT32 ui32PackedCCBSizeU88;
	MT_UINT32 ui32Priority;
	MT_UINT32 ui32StaticComputeContextStateSize;
} __packed MTGPU_BRIDGE_IN_MUSACREATECOMPCONTEXT;

/* Bridge out structure for MUSACreateComputeContext */
typedef struct MTGPU_BRIDGE_OUT_MUSACREATECOMPCONTEXT_TAG
{
	MT_HANDLE hComputeContext;
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_MUSACREATECOMPCONTEXT;

/*******************************************
            MUSADestroyComputeContext
 *******************************************/

/* Bridge in structure for MUSADestroyComputeContext */
typedef struct MTGPU_BRIDGE_IN_MUSADESTROYCOMPCONTEXT_TAG
{
	MT_HANDLE hComputeContext;
} __packed MTGPU_BRIDGE_IN_MUSADESTROYCOMPCONTEXT;

/* Bridge out structure for MUSADestroyComputeContext */
typedef struct MTGPU_BRIDGE_OUT_MUSADESTROYCOMPCONTEXT_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_MUSADESTROYCOMPCONTEXT;

/*******************************************
            MUSAFlushComputeData
 *******************************************/

/* Bridge in structure for MUSAFlushComputeData */
typedef struct MTGPU_BRIDGE_IN_MUSAFLUSHCOMPDATA_TAG
{
	MT_HANDLE hComputeContext;
} __packed MTGPU_BRIDGE_IN_MUSAFLUSHCOMPDATA;

/* Bridge out structure for MUSAFlushComputeData */
typedef struct MTGPU_BRIDGE_OUT_MUSAFLUSHCOMPDATA_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_MUSAFLUSHCOMPDATA;

/*******************************************
            MUSASetComputeContextPriority
 *******************************************/

/* Bridge in structure for MUSASetComputeContextPriority */
typedef struct MTGPU_BRIDGE_IN_MUSASETCOMPCONTEXTPRIORITY_TAG
{
	MT_HANDLE hComputeContext;
	MT_UINT32 ui32Priority;
} __packed MTGPU_BRIDGE_IN_MUSASETCOMPCONTEXTPRIORITY;

/* Bridge out structure for MUSASetComputeContextPriority */
typedef struct MTGPU_BRIDGE_OUT_MUSASETCOMPCONTEXTPRIORITY_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_MUSASETCOMPCONTEXTPRIORITY;

/*******************************************
            MUSANotifyComputeWriteOffsetUpdate
 *******************************************/

/* Bridge in structure for MUSANotifyComputeWriteOffsetUpdate */
typedef struct MTGPU_BRIDGE_IN_MUSANOTIFYCOMPWRITEOFFSETUPDATE_TAG
{
	MT_HANDLE hComputeContext;
} __packed MTGPU_BRIDGE_IN_MUSANOTIFYCOMPWRITEOFFSETUPDATE;

/* Bridge out structure for MUSANotifyComputeWriteOffsetUpdate */
typedef struct MTGPU_BRIDGE_OUT_MUSANOTIFYCOMPWRITEOFFSETUPDATE_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_MUSANOTIFYCOMPWRITEOFFSETUPDATE;

/*******************************************
            MUSAKickCMP2
 *******************************************/

/* Bridge in structure for MUSAKickCMP2 */
typedef struct MTGPU_BRIDGE_IN_MUSAKICKCMP2_TAG
{
	MT_UINT64 ui64DeadlineInus;
	MT_HANDLE hComputeContext;
	MT_UINT32 *pui32ClientUpdateOffset;
	MT_UINT32 *pui32ClientUpdateValue;
	MT_UINT32 *pui32SyncPMRFlags;
	MT_BYTE *pui8DMCmd;
	MT_CHAR *puiUpdateFenceName;
	MT_HANDLE *phClientUpdateUFOSyncPrimBlock;
	MT_HANDLE *phSyncPMRs;
	MTGPU_FENCE hCheckFenceFd;
	MTGPU_TIMELINE hUpdateTimeline;
	MT_UINT32 ui32ClientUpdateCount;
	MT_UINT32 ui32CmdSize;
	MT_UINT32 ui32ExtJobRef;
	MT_UINT32 ui32NumOfWorkgroups;
	MT_UINT32 ui32NumOfWorkitems;
	MT_UINT32 ui32PDumpFlags;
	MT_UINT32 ui32SyncPMRCount;
} __packed MTGPU_BRIDGE_IN_MUSAKICKCMP2;

/* Bridge out structure for MUSAKickCMP2 */
typedef struct MTGPU_BRIDGE_OUT_MUSAKICKCMP2_TAG
{
	MTGPU_ERROR eError;
	MTGPU_FENCE hUpdateFence;
} __packed MTGPU_BRIDGE_OUT_MUSAKICKCMP2;

typedef struct MTGPU_BRIDGE_IN_MUSACREATECMPFENCE_TAG
{
	MT_CHAR asFenceName[32];
	MTGPU_TIMELINE hTimeline;
} __packed MTGPU_BRIDGE_IN_MUSACREATECMPFENCE;

typedef struct MTGPU_BRIDGE_OUT_MUSACREATECMPFENCE_TAG
{
	MTGPU_FENCE hFence;
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_MUSACREATECMPFENCE;

typedef struct MTGPU_BRIDGE_IN_MUSAKICKCMP3_TAG
{
	MT_UINT64 ui64DeadlineInus;
	MT_HANDLE hComputeContext;

	MT_UINT32 ui32ClientUpdateCount;
	MT_HANDLE *phClientUpdateUFOSyncPrimBlock;
	MT_UINT32 *pui32ClientUpdateOffset;
	MT_UINT32 *pui32ClientUpdateValue;

	MT_UINT32 ui32ClientCheckCount;
	MT_HANDLE *phClientCheckUFOSyncPrimBlock;
	MT_UINT32 *pui32ClientCheckOffset;
	MT_UINT32 *pui32ClientCheckValue;

	MT_UINT32 ui32SyncPMRCount;
	MT_HANDLE *phSyncPMRs;
	MT_UINT32 *pui32SyncPMRFlags;

	MTGPU_FENCE hCheckFence;
	MTGPU_FENCE hUpdateFence;

	MT_UINT32 ui32CmdSize;
	MT_BYTE *pui8DMCmd;
	MT_UINT32 ui32ExtJobRef;
	MT_UINT32 ui32NumOfWorkgroups;
	MT_UINT32 ui32NumOfWorkitems;
	MT_UINT32 ui32PDumpFlags;

} __packed MTGPU_BRIDGE_IN_MUSAKICKCMP3;

typedef struct MTGPU_BRIDGE_OUT_MUSAKICKCMP3_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_MUSAKICKCMP3;

/*******************************************
            MUSASetComputeContextProperty
 *******************************************/

/* Bridge in structure for MUSASetComputeContextProperty */
typedef struct MTGPU_BRIDGE_IN_MUSASETCOMPCONTEXTPROPERTY_TAG
{
	MT_UINT64 ui64Input;
	MT_HANDLE hComputeContext;
	MT_UINT32 ui32Property;
} __packed MTGPU_BRIDGE_IN_MUSASETCOMPCONTEXTPROPERTY;

/* Bridge out structure for MUSASetComputeContextProperty */
typedef struct MTGPU_BRIDGE_OUT_MUSASETCOMPCONTEXTPROPERTY_TAG
{
	MT_UINT64 ui64Output;
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_MUSASETCOMPCONTEXTPROPERTY;

/*******************************************
            MUSAGetLastDeviceError
 *******************************************/

/* Bridge in structure for MUSAGetLastDeviceError */
typedef struct MTGPU_BRIDGE_IN_MUSAGETLASTDEVICEERROR_TAG
{
	MT_UINT32 ui32EmptyStructPlaceholder;
} __packed MTGPU_BRIDGE_IN_MUSAGETLASTDEVICEERROR;

/* Bridge out structure for MUSAGetLastDeviceError */
typedef struct MTGPU_BRIDGE_OUT_MUSAGETLASTDEVICEERROR_TAG
{
	MTGPU_ERROR eError;
	MT_UINT32 ui32Error;
} __packed MTGPU_BRIDGE_OUT_MUSAGETLASTDEVICEERROR;

/*******************************************
            MTDecodeFaultInfo
 *******************************************/

/* Bridge in structure for MTDecodeFaultInfo */
typedef struct MTGPU_BRIDGE_IN_DECODEFAULTINFO_TAG
{
	MT_HANDLE hPMR;
	MT_UINT64 ui64LogicalOffset;
} __packed MTGPU_BRIDGE_IN_DECODEFAULTINFO;

/* Bridge out structure for MTDecodeFaultInfo */
typedef struct MTGPU_BRIDGE_OUT_DECODEFAULTINFO_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_DECODEFAULTINFO;

/*******************************************
            MUSAGetLLCPersistingProperties
 *******************************************/

/* Bridge out structure for MUSAGetLLCPersistingProperties */
typedef struct MTGPU_BRIDGE_OUT_MUSAGETLLCPERSISTINGPROPERTIES_TAG
{
	MTGPU_ERROR eError;
	MT_UINT32 ui32LLCSize;
	MT_UINT32 ui32MaxLLCPersistingSize;
} __packed MTGPU_BRIDGE_OUT_MUSAGETLLCPERSISTINGPROPERTIES;

/*******************************************
            MUSASetLLCPersistingProperties
 *******************************************/

/* Bridge in structure for MUSASetLLCPersistingProperties */
typedef struct MTGPU_BRIDGE_IN_MUSASETLLCPERSISTINGPROPERTIES_TAG
{
	MT_UINT64 ui64MaxSetAsideSize;
	MT_UINT32 ui32ReplaceMode;
} __packed MTGPU_BRIDGE_IN_MUSASETLLCPERSISTINGPROPERTIES;

/* Bridge out structure for MUSASetLLCPersistingProperties */
typedef struct MTGPU_BRIDGE_OUT_MUSASETLLCPERSISTINGPROPERTIES_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_MUSASETLLCPERSISTINGPROPERTIES;

/*******************************************
            MUSALLCPersistingReset
 *******************************************/

/* Bridge out structure for MUSALLCPersistingReset */
typedef struct MTGPU_BRIDGE_OUT_MUSALLCPERSISTINGRESET_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_MUSALLCPERSISTINGRESET;

/*******************************************
            MUSACreateComputeContext2
 *******************************************/

/* Bridge in structure for MUSACreateComputeContext2 */
typedef struct MTGPU_BRIDGE_IN_MUSACREATECOMPCONTEXT2_TAG
{
	MT_HANDLE hPrivData;
	MT_UINT32 ui32Priority;
} __packed MTGPU_BRIDGE_IN_MUSACREATECOMPCONTEXT2;

/* Bridge out structure for MUSACreateComputeContext */
typedef struct MTGPU_BRIDGE_OUT_MUSACREATECOMPCONTEXT2_TAG
{
	MT_HANDLE hComputeContext;
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_MUSACREATECOMPCONTEXT2;

/*******************************************
            MUSADestroyComputeContext2
 *******************************************/

/* Bridge in structure for MUSADestroyComputeContext2 */
typedef struct MTGPU_BRIDGE_IN_MUSADESTROYCOMPCONTEXT2_TAG
{
	MT_HANDLE hComputeContext;
} __packed MTGPU_BRIDGE_IN_MUSADESTROYCOMPCONTEXT2;

/* Bridge out structure for MUSADestroyComputeContext2 */
typedef struct MTGPU_BRIDGE_OUT_MUSADESTROYCOMPCONTEXT2_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_MUSADESTROYCOMPCONTEXT2;

/*******************************************
            MUSAKickCMP4
 *******************************************/

/* Bridge in structure for MUSAKickCMP4 */
typedef struct MTGPU_BRIDGE_IN_MUSAKICKCMP4_TAG
{
	MT_HANDLE hComputeContext;

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
} __packed MTGPU_BRIDGE_IN_MUSAKICKCMP4;

/* Bridge out structure for MUSAKickCMP4 */
typedef struct MTGPU_BRIDGE_OUT_MUSAKICKCMP4_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_MUSAKICKCMP4;

#endif /* COMMON_MUSACMP_BRIDGE_H */

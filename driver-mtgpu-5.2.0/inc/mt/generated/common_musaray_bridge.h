/*******************************************************************************
@File
@Title          Common bridge header for musaray
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Declares common defines and structures used by both the client
                and server side of the bridge for musaray
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

#ifndef COMMON_MUSARAY_BRIDGE_H
#define COMMON_MUSARAY_BRIDGE_H

#include <mt/mem_types.h>

#include "img_defs.h"
#include "img_types.h"
#include "mtgpu_error.h"

#include "mtgpu_sync_km.h"

#define MTGPU_BRIDGE_MUSARAY_CMD_FIRST			0
#define MTGPU_BRIDGE_MUSARAY_MUSACREATERAYCONTEXT			MTGPU_BRIDGE_MUSARAY_CMD_FIRST+0
#define MTGPU_BRIDGE_MUSARAY_MUSADESTROYRAYCONTEXT			MTGPU_BRIDGE_MUSARAY_CMD_FIRST+1
#define MTGPU_BRIDGE_MUSARAY_MUSAKICKRDM			MTGPU_BRIDGE_MUSARAY_CMD_FIRST+2
#define MTGPU_BRIDGE_MUSARAY_CMD_LAST			(MTGPU_BRIDGE_MUSARAY_CMD_FIRST+2)

/*******************************************
            MUSACreateRayContext
 *******************************************/

/* Bridge in structure for MUSACreateRayContext */
typedef struct MTGPU_BRIDGE_IN_MUSACREATERAYCONTEXT_TAG
{
	MT_UINT64 ui64RobustnessAddress;
	MT_HANDLE hPrivData;
	MT_BYTE *pui8sStaticRayContextState;
	MT_UINT32 ui32ContextFlags;
	MT_UINT64 ui64MaxDeadlineMS;
	MT_UINT32 ui32StaticRayContextStateSize;
	MT_UINT32 ui32ui32Priority;
} __packed MTGPU_BRIDGE_IN_MUSACREATERAYCONTEXT;

/* Bridge out structure for MUSACreateRayContext */
typedef struct MTGPU_BRIDGE_OUT_MUSACREATERAYCONTEXT_TAG
{
	MT_HANDLE hRayContext;
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_MUSACREATERAYCONTEXT;

/*******************************************
            MUSADestroyRayContext
 *******************************************/

/* Bridge in structure for MUSADestroyRayContext */
typedef struct MTGPU_BRIDGE_IN_MUSADESTROYRAYCONTEXT_TAG
{
	MT_HANDLE hRayContext;
} __packed MTGPU_BRIDGE_IN_MUSADESTROYRAYCONTEXT;

/* Bridge out structure for MUSADestroyRayContext */
typedef struct MTGPU_BRIDGE_OUT_MUSADESTROYRAYCONTEXT_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_MUSADESTROYRAYCONTEXT;

/*******************************************
            MUSAKickRDM
 *******************************************/

/* Bridge in structure for MUSAKickRDM */
typedef struct MTGPU_BRIDGE_IN_MUSAKICKRDM_TAG
{
	MT_HANDLE hRayContext;
	MT_UINT32 *pui32ClientUpdateOffset;
	MT_UINT32 *pui32ClientUpdateValue;
	MT_BYTE *pui8DMCmd;
	MT_CHAR *puiUpdateFenceName;
	MT_HANDLE *phClientUpdateUFOSyncPrimBlock;
	MTGPU_FENCE hCheckFenceFd;
	MTGPU_TIMELINE hUpdateTimeline;
	MT_UINT32 ui32ClientUpdateCount;
	MT_UINT32 ui32CmdSize;
	MT_UINT32 ui32ExtJobRef;
	MT_UINT32 ui32PDumpFlags;
} __packed MTGPU_BRIDGE_IN_MUSAKICKRDM;

/* Bridge out structure for MUSAKickRDM */
typedef struct MTGPU_BRIDGE_OUT_MUSAKICKRDM_TAG
{
	MTGPU_ERROR eError;
	MTGPU_FENCE hUpdateFence;
} __packed MTGPU_BRIDGE_OUT_MUSAKICKRDM;

#endif /* COMMON_MUSARAY_BRIDGE_H */

/*******************************************************************************
@File
@Title          Common bridge header for sync
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Declares common defines and structures used by both the client
                and server side of the bridge for sync
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

#ifndef COMMON_SYNC_BRIDGE_H
#define COMMON_SYNC_BRIDGE_H

#include <mt/mem_types.h>

#include "img_defs.h"
#include "img_types.h"
#include "mtgpu_error.h"

#include "devicemem_typedefs.h"
#include "mtgpu_sync_km.h"
#include <mt/mtgpu_sync_ext.h>

#define MTGPU_BRIDGE_SYNC_CMD_FIRST				0
#define MTGPU_BRIDGE_SYNC_ALLOCSYNCPRIMITIVEBLOCK		MTGPU_BRIDGE_SYNC_CMD_FIRST+0
#define MTGPU_BRIDGE_SYNC_FREESYNCPRIMITIVEBLOCK		MTGPU_BRIDGE_SYNC_CMD_FIRST+1
#define MTGPU_BRIDGE_SYNC_SYNCPRIMSET				MTGPU_BRIDGE_SYNC_CMD_FIRST+2
#define MTGPU_BRIDGE_SYNC_SYNCPRIMPDUMP			MTGPU_BRIDGE_SYNC_CMD_FIRST+3
#define MTGPU_BRIDGE_SYNC_SYNCPRIMPDUMPVALUE			MTGPU_BRIDGE_SYNC_CMD_FIRST+4
#define MTGPU_BRIDGE_SYNC_SYNCPRIMPDUMPPOL			MTGPU_BRIDGE_SYNC_CMD_FIRST+5
#define MTGPU_BRIDGE_SYNC_SYNCPRIMPDUMPCBP			MTGPU_BRIDGE_SYNC_CMD_FIRST+6
#define MTGPU_BRIDGE_SYNC_SYNCALLOCEVENT			MTGPU_BRIDGE_SYNC_CMD_FIRST+7
#define MTGPU_BRIDGE_SYNC_SYNCFREEEVENT			MTGPU_BRIDGE_SYNC_CMD_FIRST+8
#define MTGPU_BRIDGE_SYNC_SYNCCHECKPOINTSIGNALLEDPDUMPPOL	MTGPU_BRIDGE_SYNC_CMD_FIRST+9
#define MTGPU_BRIDGE_SYNC_SYNCPRIMCPUSIGNAL			MTGPU_BRIDGE_SYNC_CMD_FIRST+10
#define MTGPU_BRIDGE_SYNC_SYNCPRIMEXPORTFD			MTGPU_BRIDGE_SYNC_CMD_FIRST+11
#define MTGPU_BRIDGE_SYNC_SYNCPRIMIMPORTFD			MTGPU_BRIDGE_SYNC_CMD_FIRST+12
#define MTGPU_BRIDGE_SYNC_SYNCPRIMCPUSIGNAL2			MTGPU_BRIDGE_SYNC_CMD_FIRST+13
#define MTGPU_BRIDGE_SYNC_SYNCPRIMEXPORTFD2			MTGPU_BRIDGE_SYNC_CMD_FIRST+14
#define MTGPU_BRIDGE_SYNC_CMD_LAST				(MTGPU_BRIDGE_SYNC_CMD_FIRST+14)

/*******************************************
            AllocSyncPrimitiveBlock
 *******************************************/

/* Bridge in structure for AllocSyncPrimitiveBlock */
typedef struct MTGPU_BRIDGE_IN_ALLOCSYNCPRIMITIVEBLOCK_TAG
{
	MT_UINT64 ui64MemType;
} __packed MTGPU_BRIDGE_IN_ALLOCSYNCPRIMITIVEBLOCK;

/* Bridge out structure for AllocSyncPrimitiveBlock */
typedef struct MTGPU_BRIDGE_OUT_ALLOCSYNCPRIMITIVEBLOCK_TAG
{
	MT_HANDLE hSyncHandle;
	MT_HANDLE hhSyncPMR;
	MTGPU_ERROR eError;
	MT_UINT32 ui32SyncPrimBlockSize;
	MT_UINT64 ui64SyncPrimVAddr;
} __packed MTGPU_BRIDGE_OUT_ALLOCSYNCPRIMITIVEBLOCK;

/*******************************************
            FreeSyncPrimitiveBlock
 *******************************************/

/* Bridge in structure for FreeSyncPrimitiveBlock */
typedef struct MTGPU_BRIDGE_IN_FREESYNCPRIMITIVEBLOCK_TAG
{
	MT_HANDLE hSyncHandle;
} __packed MTGPU_BRIDGE_IN_FREESYNCPRIMITIVEBLOCK;

/* Bridge out structure for FreeSyncPrimitiveBlock */
typedef struct MTGPU_BRIDGE_OUT_FREESYNCPRIMITIVEBLOCK_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_FREESYNCPRIMITIVEBLOCK;

/*******************************************
            SyncPrimSet
 *******************************************/

/* Bridge in structure for SyncPrimSet */
typedef struct MTGPU_BRIDGE_IN_SYNCPRIMSET_TAG
{
	MT_HANDLE hSyncHandle;
	MT_UINT32 ui32Index;
	MT_UINT32 ui32Value;
} __packed MTGPU_BRIDGE_IN_SYNCPRIMSET;

/* Bridge out structure for SyncPrimSet */
typedef struct MTGPU_BRIDGE_OUT_SYNCPRIMSET_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_SYNCPRIMSET;

/*******************************************
            SyncPrimCpuSignal
 *******************************************/

/* Bridge in structure for SyncPrimCpuSignal */
typedef struct MTGPU_BRIDGE_IN_SYNCPRIMCPUSIGNAL_TAG
{
	MT_HANDLE hSyncHandle;
	MT_UINT32 ui32Index;
	MT_UINT32 ui32Value;
} __packed MTGPU_BRIDGE_IN_SYNCPRIMCPUSIGNAL;

/* Bridge out structure for SyncPrimCpuSignal */
typedef struct MTGPU_BRIDGE_OUT_SYNCPRIMCPUSIGNAL_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_SYNCPRIMCPUSIGNAL;

/*******************************************
            SyncAllocEvent
 *******************************************/

/* Bridge in structure for SyncAllocEvent */
typedef struct MTGPU_BRIDGE_IN_SYNCALLOCEVENT_TAG
{
	const MT_CHAR *puiClassName;
	MT_BOOL bServerSync;
	MT_UINT32 ui32ClassNameSize;
	MT_UINT32 ui32FWAddr;
} __packed MTGPU_BRIDGE_IN_SYNCALLOCEVENT;

/* Bridge out structure for SyncAllocEvent */
typedef struct MTGPU_BRIDGE_OUT_SYNCALLOCEVENT_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_SYNCALLOCEVENT;

/*******************************************
            SyncFreeEvent
 *******************************************/

/* Bridge in structure for SyncFreeEvent */
typedef struct MTGPU_BRIDGE_IN_SYNCFREEEVENT_TAG
{
	MT_UINT32 ui32FWAddr;
} __packed MTGPU_BRIDGE_IN_SYNCFREEEVENT;

/* Bridge out structure for SyncFreeEvent */
typedef struct MTGPU_BRIDGE_OUT_SYNCFREEEVENT_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_SYNCFREEEVENT;

/*******************************************
            SyncCheckpointSignalledPDumpPol
 *******************************************/

/* Bridge in structure for SyncCheckpointSignalledPDumpPol */
typedef struct MTGPU_BRIDGE_IN_SYNCCHECKPOINTSIGNALLEDPDUMPPOL_TAG
{
	MTGPU_FENCE hFence;
} __packed MTGPU_BRIDGE_IN_SYNCCHECKPOINTSIGNALLEDPDUMPPOL;

/* Bridge out structure for SyncCheckpointSignalledPDumpPol */
typedef struct MTGPU_BRIDGE_OUT_SYNCCHECKPOINTSIGNALLEDPDUMPPOL_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_SYNCCHECKPOINTSIGNALLEDPDUMPPOL;

/*******************************************
		MTGPUSyncPrimExportFD
 *******************************************/

/* Bridge in structure for MTGPUSyncPrimExportFD */
typedef struct MTGPU_BRIDGE_IN_SYNCPRIMEXPORTFD_TAG
{
	MT_HANDLE hSyncBlock;
	MT_UINT32 ui32Offset;
	MT_UINT64 ui64Value;
} __packed MTGPU_BRIDGE_IN_SYNCPRIMEXPORTFD;

/* Bridge out structure for MTGPUSyncPrimExportFD */
typedef struct MTGPU_BRIDGE_OUT_SYNCPRIMEXPORTFD_TAG
{
	MT_UINT32 ui32Fd;
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_SYNCPRIMEXPORTFD;

/*******************************************
		MTGPUSyncPrimExportFD2
 *******************************************/

/* Bridge in structure for MTGPUSyncPrimExportFD2 */
typedef struct MTGPU_BRIDGE_IN_SYNCPRIMEXPORTFD2_TAG
{
	MT_UINT32 ui32Count;
	MT_HANDLE *hSyncBlock;
	MT_UINT32 *ui32Offset;
	MT_UINT64 *ui64Value;
} __packed MTGPU_BRIDGE_IN_SYNCPRIMEXPORTFD2;

/* Bridge out structure for MTGPUSyncPrimExportFD2 */
typedef struct MTGPU_BRIDGE_OUT_SYNCPRIMEXPORTFD2_TAG
{
	MT_UINT32 ui32Fd;
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_SYNCPRIMEXPORTFD2;

/*******************************************
		MTGPUSyncPrimImportFD
 *******************************************/

/* Bridge in structure for MTGPUSyncPrimImportFD */
typedef struct MTGPU_BRIDGE_IN_SYNCPRIMIMPORTFD_TAG
{
	MT_UINT32 ui32Fd;
	MT_HANDLE hSyncBlock;
	MT_UINT32 ui32Offset;
	MT_HANDLE hDevmemCtx;
} __packed MTGPU_BRIDGE_IN_SYNCPRIMIMPORTFD;

/* Bridge out structure for MTGPUSyncPrimImportFD */
typedef struct MTGPU_BRIDGE_OUT_SYNCPRIMIMPORTFD_TAG
{
	MT_UINT64 ui64Value;
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_SYNCPRIMIMPORTFD;

/*******************************************
            SyncPrimCpuSignal2
 *******************************************/

/* Bridge in structure for SyncPrimCpuSignal2 */
typedef struct MTGPU_BRIDGE_IN_SYNCPRIMCPUSIGNAL2_TAG
{
	MT_HANDLE hDevmemCtx;
	MT_HANDLE hSyncBlock;
	MT_UINT32 ui32Index;
	MT_UINT64 ui64Value;
} __packed MTGPU_BRIDGE_IN_SYNCPRIMCPUSIGNAL2;

/* Bridge out structure for SyncPrimCpuSignal2 */
typedef struct MTGPU_BRIDGE_OUT_SYNCPRIMCPUSIGNAL2_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_SYNCPRIMCPUSIGNAL2;

#endif /* COMMON_SYNC_BRIDGE_H */

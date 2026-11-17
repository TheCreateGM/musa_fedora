/**************************************************************************/ /*!
@File
@Title          Server side synchronisation interface
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Describes the server side synchronisation functions
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
*/ /***************************************************************************/

#include "img_types.h"
#include "img_defs.h"
#include "mt.h"
#include "device.h"
#include "devicemem.h"
#include "mtgpu_error.h"
#include "connection_server.h"

#ifndef SYNC_SERVER_H
#define SYNC_SERVER_H

/* Set maximum number of FWAddrs that can be accommodated in a SYNC_ADDR_LIST.
 * This should allow for MTGPU_MAX_DEV_VARS dev vars plus
 * MAX_SYNC_CHECKPOINTS_PER_FENCE sync checkpoints for check fences.
 * The same SYNC_ADDR_LIST is also used to hold UFOs for updates. While this
 * may need to accommodate the additional sync prim update returned by Native
 * sync implementation (used for timeline debug), the size calculated from
 * MTGPU_MAX_DEV_VARS+MAX_SYNC_CHECKPOINTS_PER_FENCE should be ample.
 */
#define MTGPU_MAX_SYNC_ADDR_LIST_SIZE (MTGPU_MAX_DEV_VARS + MAX_SYNC_CHECKPOINTS_PER_FENCE)

typedef struct _SYNC_PRIMITIVE_BLOCK_ SYNC_PRIMITIVE_BLOCK;
typedef struct _SYNC_PRIM_FENCE_CONTEXT_ SYNC_PRIM_FENCE_CONTEXT;
typedef struct _SYNC_PRIM_FENCE_ SYNC_PRIM_FENCE;
typedef struct _SYNC_CONNECTION_DATA_ SYNC_CONNECTION_DATA;
typedef struct SYNC_RECORD* SYNC_RECORD_HANDLE;

typedef struct _SYNC_ADDR_LIST2_
{
	MT_UINT64 *pui64FWAddrs;
} SYNC_ADDR_LIST2;


MTGPU_ERROR
SyncPrimitiveBlockToFWAddr2(SYNC_PRIMITIVE_BLOCK *psSyncPrimBlock,
			    MT_UINT32 ui32Offset,
			    MT_UINT64 *pui64AddrOut);

void
SyncAddrListInit2(SYNC_ADDR_LIST2 *psList);

void
SyncAddrListDeinit2(SYNC_ADDR_LIST2 *psList);

MTGPU_ERROR
SyncAddrListPopulate2(SYNC_ADDR_LIST2 *psList,
		      MT_UINT32 ui32NumSyncs,
		      SYNC_PRIMITIVE_BLOCK **apsSyncPrimBlock,
		      MT_UINT32 *paui32SyncOffset);

void
SyncAddrListDeRefCheckpoints(MT_UINT32 ui32NumCheckpoints,
							 PSYNC_CHECKPOINT *apsSyncCheckpoint);

void
SyncAddrListTakeFwRefCheckpoints(MT_UINT32 ui32NumCheckpoints,
				 PSYNC_CHECKPOINT *apsSyncCheckpoint);

MTGPU_ERROR
MTGPUAllocSyncPrimitiveBlockKM(CONNECTION_DATA *psConnection,
                                MTGPU_DEVICE_NODE * psDevNode,
				MT_UINT64 ui64MemType,
				SYNC_PRIMITIVE_BLOCK **ppsSyncBlk,
				MT_UINT64 *puiSyncPrimVAddr,
				MT_UINT32 *puiSyncPrimBlockSize,
				PMR **ppsSyncPMR);

MTGPU_ERROR
MTGPUExportSyncPrimitiveBlockKM(SYNC_PRIMITIVE_BLOCK *psSyncBlk,
				 DEVMEM_EXPORTCOOKIE **psExportCookie);

MTGPU_ERROR
MTGPUUnexportSyncPrimitiveBlockKM(SYNC_PRIMITIVE_BLOCK *psSyncBlk);

MTGPU_ERROR
SyncPrimBlockGet(SYNC_PRIMITIVE_BLOCK *psSyncBlk);

MTGPU_ERROR
SyncPrimBlockPut(SYNC_PRIMITIVE_BLOCK *psSyncBlk);

MTGPU_ERROR
MTGPUFreeSyncPrimitiveBlockKM(SYNC_PRIMITIVE_BLOCK *ppsSyncBlk);

MTGPU_ERROR
MTGPUSyncPrimSetKM(SYNC_PRIMITIVE_BLOCK *psSyncBlk,
		    MT_UINT32 ui32Index,
		    MT_UINT32 ui32Value);

MTGPU_ERROR
MTGPUSyncAllocEventKM(CONNECTION_DATA *psConnection,
		       MTGPU_DEVICE_NODE *psDevNode,
		       MT_BOOL bServerSync,
		       MT_UINT32 ui32FWAddr,
                       MT_UINT32 ui32ClassNameSize,
                       const MT_CHAR *pszClassName);

MTGPU_ERROR
MTGPUSyncFreeEventKM(CONNECTION_DATA *psConnection,
					   MTGPU_DEVICE_NODE *psDevNode,
					   MT_UINT32 ui32FWAddr);

MTGPU_ERROR
MTGPUSyncRecordAddKM(CONNECTION_DATA *psConnection,
					  MTGPU_DEVICE_NODE *psDevNode,
					  SYNC_RECORD_HANDLE *phRecord,
					  SYNC_PRIMITIVE_BLOCK *hServerSyncPrimBlock,
					  MT_UINT32 ui32FwBlockAddr,
					  MT_UINT32 ui32SyncOffset,
					  MT_BOOL bServerSync,
					  MT_UINT32 ui32ClassNameSize,
					  const MT_CHAR *pszClassName);

MTGPU_ERROR
MTGPUSyncRecordRemoveByHandleKM(
			SYNC_RECORD_HANDLE hRecord);
void SyncRecordLookup(MTGPU_DEVICE_NODE *psDevNode, MT_UINT32 ui32FwAddr,
					  MT_CHAR * pszSyncInfo, size_t len);

MTGPU_ERROR SyncRegisterConnection(SYNC_CONNECTION_DATA **ppsSyncConnectionData);
void SyncUnregisterConnection(SYNC_CONNECTION_DATA *ppsSyncConnectionData);

/*!
******************************************************************************
@Function      SyncServerInit

@Description   Per-device initialisation for the ServerSync module
******************************************************************************/
MTGPU_ERROR SyncServerInit(MTGPU_DEVICE_NODE *psDevNode);
void SyncServerDeinit(MTGPU_DEVICE_NODE *psDevNode);

struct _DEVMEMINT_CTX_;
struct _MTGPU_MUSADEV_INFO_;
void MTGPUSyncPrimSignalFences(struct _MTGPU_MUSADEV_INFO_ *psDevinfo);

MT_BOOL SyncPrimIsSignaled(SYNC_PRIMITIVE_BLOCK *psSyncBlk,
			    MT_UINT32 ui32Offset,
			    MT_UINT64 ui64Value);

MTGPU_ERROR
MTGPUSyncPrimExportFD(SYNC_PRIMITIVE_BLOCK *psSyncBlk,
		       MT_UINT32 ui32Offset,
		       MT_UINT64 ui64Value,
		       MT_UINT32 *ui32Fd);
MTGPU_ERROR
MTGPUSyncPrimImportFD(CONNECTION_DATA *psConnection,
		       struct _DEVMEMINT_CTX_ *psDevmemCtx,
		       SYNC_PRIMITIVE_BLOCK *psSyncBlk,
		       MT_UINT32 ui32Fd,
		       MT_UINT32 ui32Offset,
		       MT_UINT64 *ui64Value);

MT_INT
MTGPUSyncPrimCpuSignal(MTGPU_DEVICE_NODE *psDevNode,
			struct _DEVMEMINT_CTX_ *psDevmemCtx,
			SYNC_PRIMITIVE_BLOCK *psSyncBlk,
			MT_UINT64 ui32Offset,
			MT_UINT64 ui64Value);

MTGPU_ERROR
MTGPUSyncPrimExportFD2(MT_UINT32 ui32Count,
			SYNC_PRIMITIVE_BLOCK **psSyncBlk,
			MT_UINT32 *ui32Offset,
			MT_UINT64 *ui64Value,
			MT_UINT32 *ui32FdOut);

MT_UINT64 SyncPrimGetValue(SYNC_PRIMITIVE_BLOCK *psSyncBlk, MT_UINT32 ui32Offset);


void SyncPrimSetValue(SYNC_PRIMITIVE_BLOCK *psSyncBlk,
		      MT_UINT32 ui32Offset,
		      MT_UINT64 ui64Value);

MT_UINT64 SyncPrimGetFwAddr(SYNC_PRIMITIVE_BLOCK *psSyncBlk, MT_UINT32 ui32Offset);

MTGPU_ERROR SyncPrimFenceSignal(SYNC_PRIMITIVE_BLOCK *psSyncBlk,
				 MT_UINT32 ui32Offset,
				 MT_UINT64 ui64Value,
				 MT_BOOL bCpuSignal);

void SyncPrimFenceDestroy(SYNC_PRIM_FENCE_CONTEXT *psFenceContext,
			  MT_UINT64 ui64Value);
MTGPU_ERROR
SyncPrimFenceContextGet(SYNC_PRIMITIVE_BLOCK *psSyncPrimBlock,
			MT_UINT32 ui32Offset,
			SYNC_PRIM_FENCE_CONTEXT **ppsFenceContext);

MTGPU_ERROR
SyncPrimUpdateFenceCreate(SYNC_PRIMITIVE_BLOCK *psSyncPrimBlock,
			  SYNC_PRIM_FENCE_CONTEXT *psFenceContext,
			  MT_UINT64 ui64Value,
			  MT_UINT32 ui32Offset);

MTGPU_ERROR
SyncPrimCheckFenceCreate(SYNC_PRIMITIVE_BLOCK *psSyncPrimBlock,
			 SYNC_PRIM_FENCE_CONTEXT *psFenceContext,
			 MT_UINT64 ui64Value,
			 MT_UINT32 ui32Offset,
			 struct dma_fence **ppsDmaFence);

struct KickSyncUpdateData
{
	struct _CONNECTION_DATA_ *psConnection;
	SYNC_PRIMITIVE_BLOCK **psSyncBlk;
	MT_UINT32 *ui32Offset;
	MT_UINT64 *ui64SignalValue;
	MT_UINT32 ui32UpdateCount;
	MT_UINT32 ui32CheckCount;
	struct dma_fence *psBaseDmaFence;
	struct dma_fence_cb *cb;
	struct list_head sNode;
	struct work_struct *psSignalWork;
};

MTGPU_ERROR MUSAKickSyncAddJob(CONNECTION_DATA 		  * psConnection,
			       /*update sync_prim*/
			       SYNC_PRIMITIVE_BLOCK      ** pauiUpdateSyncPrimBlock,
			       MT_UINT32                 * paui32UpdateSyncOffset,
			       MT_UINT64                 * paui64UpdateValue,
			       MT_UINT32                   ui32ClientUpdateCount,
			       /*check sync_prim*/
			       SYNC_PRIMITIVE_BLOCK      ** pauiCheckSyncPrimBlock,
			       MT_UINT32                 * paui32CheckSyncOffset,
			       MT_UINT64                 * paui64CheckValue,
			       MT_UINT32                   ui32ClientCheckCount);

#endif	/*SYNC_SERVER_H */

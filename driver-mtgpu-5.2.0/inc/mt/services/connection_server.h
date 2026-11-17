/*************************************************************************/ /*!
@File
@Title          Server side connection management
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    API for server side connection management
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
*/ /**************************************************************************/

#if !defined(CONNECTION_SERVER_H)
#define CONNECTION_SERVER_H


#include "img_types.h"
#include "img_defs.h"
#include "handle.h"
#include "mtgpu_cleanup.h"
#include "info_page_defs.h"
#include "devicemem_typedefs.h"
#include "pmr_impl.h"
#include "mtgpu_drm.h"
#include "mtgpu_handle.h"

/* Variable used to hold in memory the timeout for the current time slice*/
extern MT_UINT64 gui64TimesliceLimit;
/* Counter number of handle data freed during the current time slice */
extern MT_UINT32 gui32HandleDataFreeCounter;
/* Set the maximum time the freeing of the resources can keep the lock */
#define CONNECTION_DEFERRED_CLEANUP_TIMESLICE_NS (3000 * 1000) /* 3ms */

typedef struct _CONNECTION_DATA_
{
	struct mtgpu_session		sMtSession;
	MTGPU_HANDLE_BASE		*psHandleBase;
	PROCESS_HANDLE_BASE		*psProcessHandleBase;
	struct _SYNC_CONNECTION_DATA_	*psSyncConnectionData;
	struct _PDUMP_CONNECTION_DATA_	*psPDumpConnectionData;

	/* Holds the client flags supplied at connection time */
	MT_UINT32          ui32ClientFlags;

	/*
	 * OS specific data can be stored via this handle.
	 * See osconnection_server.h for a generic mechanism
	 * for initialising this field.
	 */
	MT_HANDLE          hOsPrivateData;

#define MTGPU_CONNECTION_PROCESS_NAME_LEN (16)
	MT_PID             pid;
	MT_PID				vpid;
	MT_UINT32			tid;
	MT_CHAR            pszProcName[MTGPU_CONNECTION_PROCESS_NAME_LEN];

	MT_HANDLE          hProcessStats;

	MT_HANDLE          hClientTLStream;

	/* Private hwperf setting for this process connection. */
	PMR		*psHWPerfSettingPMR;
	DEVMEM_MEMDESC	*psHWPerfSettingMemDesc;
	void		*psHWPerfConnection;

#if defined(SUPPORT_GPUVIRT_VALIDATION)
	/*
	 * Connection-based values per application which can be modified by the
	 * AppHint settings 'OSid, OSidReg, bOSidAxiProtReg' for each application.
	 * These control where the connection's memory allocation is sourced from.
	 * ui32OSid, ui32OSidReg range from 0..(GPUVIRT_VALIDATION_NUM_OS - 1).
	 */
	MT_UINT32          ui32OSid;
	MT_UINT32          ui32OSidReg;
	MT_BOOL            bOSidAxiProtReg;
#endif	/* defined(SUPPORT_GPUVIRT_VALIDATION) */

#if defined(SUPPORT_DMA_TRANSFER)
	MT_BOOL            bAcceptDmaRequests;
	ATOMIC_T            ui32NumDmaTransfersInFlight;
	POS_LOCK            hDmaReqLock;
	MT_HANDLE          hDmaEventObject;
#endif
	/* Structure which is hooked into the cleanup thread work list */
	MTGPU_CLEANUP_THREAD_WORK sCleanupThreadFn;

	DLLIST_NODE         sConnectionListNode;

	MT_CHAR            szLibdrmVersion[MTGPU_COMMIT_STRING_LENGTH];
	MT_CHAR            szShardIncVersion[MTGPU_COMMIT_STRING_LENGTH];

	/* List navigation for deferred freeing of connection data */
	struct _CONNECTION_DATA_	**ppsThis;
	struct _CONNECTION_DATA_	*psNext;

	/*
	 * Used for sema mmap optimise, decrease the mmap called times.
	 * The key is pmr pointer, the value is gem obj bo handle.
	 */
	struct _HASH_TABLE_ *syncobj_block_hash_table;

	struct _HASH_TABLE_ *capture_hash_table;

	/* For managing KickSyncUpdateData */
	POS_SPINLOCK sKickSyncUpdateLock;
	struct list_head sKickSyncUpdateList;

	uint64_t ui64RmClient;
} CONNECTION_DATA;

#include "osconnection_server.h"

MTGPU_ERROR MTGPUCommonConnectionConnect(void **ppvPrivData, void *pvOSData);
void MTGPUCommonConnectionDisconnect(void *pvPrivData);

/**************************************************************************/ /*!
@Function       MTGPUGetPurgeConnectionPid

@Description    Returns PID associated with Connection currently being purged by
                Cleanup Thread. If no Connection is purged 0 is returned.

@Return         PID associated with currently purged connection or 0 if no
                connection is being purged
*/ /***************************************************************************/
MT_PID MTGPUGetPurgeConnectionPid(void);

void MTGPUConnectionDebugNotify(MTGPU_DEVICE_NODE *psDevNode,
                                 DUMPDEBUG_PRINTF_FUNC *pfnDumpDebugPrintf,
                                 void *pvDumpDebugFile);

#ifdef INLINE_IS_PRAGMA
#pragma inline(MTGPUConnectionPrivateData)
#endif
static INLINE
MT_HANDLE MTGPUConnectionPrivateData(CONNECTION_DATA *psConnection)
{
	return (psConnection != NULL) ? psConnection->hOsPrivateData : NULL;
}

#endif /* !defined(CONNECTION_SERVER_H) */

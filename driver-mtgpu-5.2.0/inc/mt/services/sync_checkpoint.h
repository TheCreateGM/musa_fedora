/*************************************************************************/ /*!
@File
@Title          Synchronisation checkpoint interface header
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Defines the client side interface for synchronisation
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

#ifndef SYNC_CHECKPOINT_H
#define SYNC_CHECKPOINT_H

#include "img_types.h"
#include "mtgpu_error.h"
#include "mtgpu_sync_km.h"
#include "dllist.h"
#include "mt_debug.h"
#include "device_connection.h"
#include "opaque_types.h"

struct _CONNECTION_DATA_;
struct drm_mtgpu_fence;
struct drm_mtgpu_semaphore;

#ifndef CHECKPOINT_TYPES
#define CHECKPOINT_TYPES
typedef struct SYNC_CHECKPOINT_CONTEXT_TAG *PSYNC_CHECKPOINT_CONTEXT;

typedef struct SYNC_CHECKPOINT_TAG *PSYNC_CHECKPOINT;
#endif

/* definitions for functions to be implemented by OS-specific sync - the OS-specific sync code
   will call SyncCheckpointRegisterFunctions() when initialised, in order to register functions
   we can then call */
#ifndef CHECKPOINT_PFNS
#define CHECKPOINT_PFNS
typedef MTGPU_ERROR (*PFN_SYNC_CHECKPOINT_FENCE_RESOLVE_FN)(PSYNC_CHECKPOINT_CONTEXT psSyncCheckpointContext,
                                                             MTGPU_FENCE fence,
                                                             MT_UINT32 *nr_checkpoints,
                                                             PSYNC_CHECKPOINT **checkpoint_handles,
                                                             MT_UINT64 *pui64FenceUID);
typedef MTGPU_ERROR (*PFN_SYNC_CHECKPOINT_FENCE_CREATE_FN)(PMTGPU_DEVICE_NODE device,
                                                            const MT_CHAR *fence_name,
                                                            MTGPU_TIMELINE timeline,
                                                            PSYNC_CHECKPOINT_CONTEXT psSyncCheckpointContext,
                                                            MTGPU_FENCE *new_fence,
                                                            MT_UINT64 *pui64FenceUID,
                                                            void **ppvFenceFinaliseData,
                                                            PSYNC_CHECKPOINT *new_checkpoint_handle,
                                                            MT_HANDLE *timeline_update_sync,
                                                            MT_UINT32 *timeline_update_value);
typedef MTGPU_ERROR (*PFN_SYNC_CHECKPOINT_FENCE_ROLLBACK_DATA_FN)(MTGPU_FENCE fence_to_rollback, void *finalise_data);
typedef MTGPU_ERROR (*PFN_SYNC_CHECKPOINT_FENCE_FINALISE_FN)(MTGPU_FENCE fence_to_finalise, void *finalise_data);
typedef void (*PFN_SYNC_CHECKPOINT_NOHW_UPDATE_TIMELINES_FN)(void *private_data);
typedef void (*PFN_SYNC_CHECKPOINT_FREE_CHECKPOINT_LIST_MEM_FN)(void *mem_ptr);
typedef MT_UINT32 (*PFN_SYNC_CHECKPOINT_DUMP_INFO_ON_STALLED_UFOS_FN)(MT_UINT32 num_ufos, MT_UINT32 *vaddrs);

typedef int (*PFN_SYNC_CHECKPOINT_TIMELINE_GET_FN) (const MTGPU_FENCE fence,
							     MTGPU_TIMELINE *timeline);
typedef int (*PFN_SYNC_CHECKPOINT_GET_CHECKPOINTS_FN)(MTGPU_FENCE FenceHandle,
							       PSYNC_CHECKPOINT *ppSyncCheckPoint,
							       void **ppvTimelineUpdateSyncPrim,
							       u32 *updateCheckPointNum);

#define SYNC_CHECKPOINT_IMPL_MAX_STRLEN 20

typedef struct
{
	PFN_SYNC_CHECKPOINT_FENCE_RESOLVE_FN pfnFenceResolve;
	PFN_SYNC_CHECKPOINT_FENCE_CREATE_FN pfnFenceCreate;
	PFN_SYNC_CHECKPOINT_FENCE_ROLLBACK_DATA_FN pfnFenceDataRollback;
	PFN_SYNC_CHECKPOINT_FENCE_FINALISE_FN pfnFenceFinalise;
	PFN_SYNC_CHECKPOINT_NOHW_UPDATE_TIMELINES_FN pfnNoHWUpdateTimelines;
	PFN_SYNC_CHECKPOINT_FREE_CHECKPOINT_LIST_MEM_FN pfnFreeCheckpointListMem;
	PFN_SYNC_CHECKPOINT_DUMP_INFO_ON_STALLED_UFOS_FN pfnDumpInfoOnStalledUFOs;
	MT_CHAR pszImplName[SYNC_CHECKPOINT_IMPL_MAX_STRLEN];
	PFN_SYNC_CHECKPOINT_TIMELINE_GET_FN pfnGetTimelineHandleFromFence;
	PFN_SYNC_CHECKPOINT_GET_CHECKPOINTS_FN pfnGetFenceCheckpointData;
} PFN_SYNC_CHECKPOINT_STRUCT;

MTGPU_ERROR SyncCheckpointRegisterFunctions(PFN_SYNC_CHECKPOINT_STRUCT *psSyncCheckpointPfns);

#endif /* ifndef CHECKPOINT_PFNS */

/*************************************************************************/ /*!
@Function       SyncCheckpointContextCreate

@Description    Create a new synchronisation checkpoint context

@Input          psDevNode                 Device node

@Output         ppsSyncCheckpointContext  Handle to the created synchronisation
                                          checkpoint context

@Return         MTGPU_OK if the synchronisation checkpoint context was
                successfully created
*/
/*****************************************************************************/
MTGPU_ERROR
SyncCheckpointContextCreate(PMTGPU_DEVICE_NODE psDevNode,
                            PSYNC_CHECKPOINT_CONTEXT *ppsSyncCheckpointContext);

/*************************************************************************/ /*!
@Function       SyncCheckpointContextDestroy

@Description    Destroy a synchronisation checkpoint context

@Input          psSyncCheckpointContext  Handle to the synchronisation
                                         checkpoint context to destroy

@Return         MTGPU_OK if the synchronisation checkpoint context was
                successfully destroyed.
                MTGPU_ERROR_UNABLE_TO_DESTROY_CONTEXT if the context still
                has sync checkpoints defined
*/
/*****************************************************************************/
MTGPU_ERROR
SyncCheckpointContextDestroy(PSYNC_CHECKPOINT_CONTEXT psSyncCheckpointContext);

/*************************************************************************/ /*!
@Function       SyncCheckpointContextRef

@Description    Takes a reference on a synchronisation checkpoint context

@Input          psContext  Handle to the synchronisation checkpoint context
                           on which a ref is to be taken

@Return         None
*/
/*****************************************************************************/
void SyncCheckpointContextRef(PSYNC_CHECKPOINT_CONTEXT psContext);

/*************************************************************************/ /*!
@Function       SyncCheckpointContextUnref

@Description    Drops a reference taken on a synchronisation checkpoint
                context

@Input          psContext  Handle to the synchronisation checkpoint context
                           on which the ref is to be dropped

@Return         None
*/
/*****************************************************************************/
void SyncCheckpointContextUnref(PSYNC_CHECKPOINT_CONTEXT psContext);

/*************************************************************************/ /*!
@Function       SyncCheckpointAlloc

@Description    Allocate a new synchronisation checkpoint on the specified
                synchronisation checkpoint context

@Input          hSyncCheckpointContext  Handle to the synchronisation
                                        checkpoint context

@Input          hTimeline               Timeline on which this sync
                                        checkpoint is being created

@Input          hFence                  Fence as passed into pfnFenceResolve
                                        API, when the API encounters a non-MT
                                        fence as part of its input fence. From
                                        all other places this argument must be
                                        MTGPU_NO_FENCE.

@Input          pszClassName            Sync checkpoint source annotation
                                        (will be truncated to at most
                                         MTGPU_SYNC_NAME_LENGTH chars)

@Output         ppsSyncCheckpoint       Created synchronisation checkpoint

@Return         MTGPU_OK if the synchronisation checkpoint was
                successfully created
*/
/*****************************************************************************/
MTGPU_ERROR
SyncCheckpointAlloc(PSYNC_CHECKPOINT_CONTEXT psSyncContext,
                    MTGPU_TIMELINE hTimeline,
                    MTGPU_FENCE hFence,
                    const MT_CHAR *pszCheckpointName,
                    PSYNC_CHECKPOINT *ppsSyncCheckpoint);

/*************************************************************************/ /*!
@Function       SyncCheckpointFree

@Description    Free a synchronisation checkpoint
                The reference count held for the synchronisation checkpoint
                is decremented - if it has becomes zero, it is also freed.

@Input          psSyncCheckpoint        The synchronisation checkpoint to free

@Return         None
*/
/*****************************************************************************/
void
SyncCheckpointFree(PSYNC_CHECKPOINT psSyncCheckpoint);

/*************************************************************************/ /*!
@Function       SyncCheckpointSignal

@Description    Signal the synchronisation checkpoint

@Input          psSyncCheckpoint        The synchronisation checkpoint to signal

@Input          ui32FenceSyncFlags      Flags used for controlling HWPerf behavior

@Return         None
*/
/*****************************************************************************/
void
SyncCheckpointSignal(PSYNC_CHECKPOINT psSyncCheckpoint, MT_UINT32 ui32FenceSyncFlags);

/*************************************************************************/ /*!
@Function       SyncCheckpointSignalNoHW

@Description    Signal the synchronisation checkpoint in NO_HARWARE build

@Input          psSyncCheckpoint        The synchronisation checkpoint to signal

@Return         None
*/
/*****************************************************************************/
void
SyncCheckpointSignalNoHW(PSYNC_CHECKPOINT psSyncCheckpoint);

/*************************************************************************/ /*!
@Function       SyncCheckpointError

@Description    Error the synchronisation checkpoint

@Input          psSyncCheckpoint        The synchronisation checkpoint to error

@Input          ui32FenceSyncFlags      Flags used for controlling HWPerf behavior

@Return         None
*/
/*****************************************************************************/
void
SyncCheckpointError(PSYNC_CHECKPOINT psSyncCheckpoint, MT_UINT32 ui32FenceSyncFlags);

/*************************************************************************/ /*!
@Function       SyncCheckpointStateFromUFO

@Description    Returns the current state of the synchronisation checkpoint
                which has the given UFO firmware address

@Input          psDevNode               The device owning the sync
                                        checkpoint

@Input          ui32FwAddr              The firmware address of the sync
                                        checkpoint

@Return         The current state (32-bit value) of the sync checkpoint
*/
/*****************************************************************************/
MT_UINT32 SyncCheckpointStateFromUFO(PMTGPU_DEVICE_NODE psDevNode,
                                MT_UINT32 ui32FwAddr);

/*************************************************************************/ /*!
@Function       SyncCheckpointErrorFromUFO

@Description    Error the synchronisation checkpoint which has the
                given UFO firmware address

@Input          psDevNode               The device owning the sync
                                        checkpoint to be errored

@Input          ui32FwAddr              The firmware address of the sync
                                        checkpoint to be errored

@Return         None
*/
/*****************************************************************************/
void
SyncCheckpointErrorFromUFO(PMTGPU_DEVICE_NODE psDevNode, MT_UINT32 ui32FwAddr);

/*************************************************************************/ /*!
@Function       SyncCheckpointRollbackFromUFO

@Description    Drop the enqueued count reference taken on the synchronisation
                checkpoint on behalf of the firmware.
                Called in the event of a DM Kick failing.

@Input          psDevNode               The device owning the sync
                                        checkpoint to be rolled back

@Input          ui32FwAddr              The firmware address of the sync
                                        checkpoint to be rolled back

@Return         None
*/
/*****************************************************************************/
void
SyncCheckpointRollbackFromUFO(PMTGPU_DEVICE_NODE psDevNode, MT_UINT32 ui32FwAddr);

/*************************************************************************/ /*!
@Function       SyncCheckpointIsSignalled

@Description    Returns MT_TRUE if the synchronisation checkpoint is
                signalled or errored

@Input          psSyncCheckpoint        The synchronisation checkpoint to test

@Input          ui32FenceSyncFlags      Flags used for controlling HWPerf behavior

@Return         None
*/
/*****************************************************************************/
MT_BOOL
SyncCheckpointIsSignalled(PSYNC_CHECKPOINT psSyncCheckpoint,
                          MT_UINT32 ui32FenceSyncFlags);

/*************************************************************************/ /*!
@Function       SyncCheckpointIsErrored

@Description    Returns MT_TRUE if the synchronisation checkpoint is
                errored

@Input          psSyncCheckpoint        The synchronisation checkpoint to test

@Input          ui32FenceSyncFlags      Flags used for controlling HWPerf behavior

@Return         None
*/
/*****************************************************************************/
MT_BOOL
SyncCheckpointIsErrored(PSYNC_CHECKPOINT psSyncCheckpoint,
                        MT_UINT32 ui32FenceSyncFlags);

/*************************************************************************/ /*!
@Function       SyncCheckpointTakeRef

@Description    Take a reference on a synchronisation checkpoint

@Input          psSyncCheckpoint        Synchronisation checkpoint to take a
                                        reference on

@Return         MTGPU_OK if a reference was taken on the synchronisation
                primitive
*/
/*****************************************************************************/
MTGPU_ERROR
SyncCheckpointTakeRef(PSYNC_CHECKPOINT psSyncCheckpoint);

/*************************************************************************/ /*!
@Function       SyncCheckpointDropRef

@Description    Drop a reference on a synchronisation checkpoint

@Input          psSyncCheckpoint        Synchronisation checkpoint to drop a
                                        reference on

@Return         MTGPU_OK if a reference was dropped on the synchronisation
                primitive
*/
/*****************************************************************************/
MTGPU_ERROR
SyncCheckpointDropRef(PSYNC_CHECKPOINT psSyncCheckpoint);

/*************************************************************************/ /*!
@Function       SyncCheckpointResolveFence

@Description    Resolve a fence, returning a list of the sync checkpoints
                that fence contains.
                This function in turn calls a function provided by the
                OS native sync implementation.

@Input          psSyncCheckpointContext The sync checkpoint context
                                        on which checkpoints should be
                                        created (in the event of the fence
                                        having a native sync pt with no
                                        associated sync checkpoint)

@Input          hFence                  The fence to be resolved

@Output         pui32NumSyncCheckpoints The number of sync checkpoints the
                                        fence contains. Can return 0 if
                                        passed a null (-1) fence.

@Output         papsSyncCheckpoints     List of sync checkpoints the fence
                                        contains

@Output         puiFenceUID             Unique ID of the resolved fence

@Return         MTGPU_OK if a valid fence was provided.
                MTGPU_ERROR_SYNC_NATIVESYNC_NOT_REGISTERED if the OS native
                sync has not registered a callback function.
*/
/*****************************************************************************/
MTGPU_ERROR
SyncCheckpointResolveFence(PSYNC_CHECKPOINT_CONTEXT psSyncCheckpointContext,
                           MTGPU_FENCE hFence,
                           MT_UINT32 *pui32NumSyncCheckpoints,
                           PSYNC_CHECKPOINT **papsSyncCheckpoints,
                           MT_UINT64 *puiFenceUID,
                           MT_UINT32 ui32PDumpFlags);

MTGPU_ERROR
SyncCheckpointResolveFenceV2(PSYNC_CHECKPOINT_CONTEXT psSyncCheckpointContext,
			     struct _CONNECTION_DATA_ *psConnection,
			     struct drm_mtgpu_fence *psUpdateFence,
			     struct drm_mtgpu_fence *pasCheckFences,
			     MT_UINT32 ui32NumCheckFence,
			     MT_INT32 iForeignFenceFd,
			     MT_UINT32 *pui32NumSyncCheckpoints,
			     PSYNC_CHECKPOINT **papsSyncCheckpoints,
			     MT_UINT64 **ppui64Seqnos,
			     MT_UINT64 *pui64FenceUID,
			     MT_UINT32 ui32PDumpFlags);

/*************************************************************************/ /*!
@Function       SyncCheckpointCreateFence

@Description    Create a fence containing a single sync checkpoint.
                Return the fence and a ptr to sync checkpoint it contains.
                This function in turn calls a function provided by the
                OS native sync implementation.

@Input          pszFenceName            String to assign to the new fence
                                        (for debugging purposes)

@Input          hTimeline               Timeline on which the new fence is
                                        to be created

@Input          psSyncCheckpointContext Sync checkpoint context to be used
                                        when creating the new fence

@Output         phNewFence              The newly created fence

@Output         pui64FenceUID           Unique ID of the created fence

@Output         ppvFenceFinaliseData    Any data needed to finalise the fence
                                        in a later call to the function
                                        SyncCheckpointFinaliseFence()

@Output         psNewSyncCheckpoint     The sync checkpoint contained in
                                        the new fence

@Return         MTGPU_OK if a valid fence was provided.
                MTGPU_ERROR_SYNC_NATIVESYNC_NOT_REGISTERED if the OS native
                sync has not registered a callback function.
*/
/*****************************************************************************/
MTGPU_ERROR
SyncCheckpointCreateFence(PMTGPU_DEVICE_NODE psDeviceNode,
                          const MT_CHAR *pszFenceName,
                          MTGPU_TIMELINE hTimeline,
                          PSYNC_CHECKPOINT_CONTEXT psSyncCheckpointContext,
                          MTGPU_FENCE *phNewFence,
                          MT_UINT64 *pui64FenceUID,
                          void **ppvFenceFinaliseData,
                          PSYNC_CHECKPOINT *psNewSyncCheckpoint,
                          void **ppvTimelineUpdateSyncPrim,
                          MT_UINT32 *pui32TimelineUpdateValue,
                          MT_UINT32 ui32PDumpFlags);

MTGPU_ERROR
SyncCheckpointCreateFenceV2(PMTGPU_DEVICE_NODE psDevNode,
			    struct _CONNECTION_DATA_ *psConnection,
			    const MT_CHAR *pszFenceName,
			    PSYNC_CHECKPOINT_CONTEXT psSyncCheckpointContext,
			    struct drm_mtgpu_fence *psUpdateFence,
			    MT_UINT64 *puiUpdateFenceUID,
			    PSYNC_CHECKPOINT *psNewSyncCheckpoint,
			    MT_UINT32 ui32PDumpFlags);

/*************************************************************************/ /*!
@Function       SyncCheckpointRollbackFenceData

@Description    'Rolls back' the fence specified (destroys the fence and
                takes any other required actions to undo the fence
                creation (eg if the implementation wishes to revert the
                incrementing of the fence's timeline, etc).
                This function in turn calls a function provided by the
                OS native sync implementation.

@Input          hFence                  Fence to be 'rolled back'

@Input          pvFinaliseData          Data needed to finalise the
                                        fence

@Return         MTGPU_OK if a valid fence was provided.
                MTGPU_ERROR_SYNC_NATIVESYNC_NOT_REGISTERED if the OS native
                sync has not registered a callback function.
*/
/*****************************************************************************/
MTGPU_ERROR
SyncCheckpointRollbackFenceData(MTGPU_FENCE hFence, void *pvFinaliseData);

/*************************************************************************/ /*!
@Function       SyncCheckpointFinaliseFence

@Description    'Finalise' the fence specified (performs any actions the
                underlying implementation may need to perform just prior
                to the fence being returned to the client.
                This function in turn calls a function provided by the
                OS native sync implementation - if the native sync
                implementation does not need to perform any actions at
                this time, this function does not need to be registered.

@Input          psDevNode               Device node

@Input          hFence                  Fence to be 'finalised'

@Input          pvFinaliseData          Data needed to finalise the fence

@Input          psSyncCheckpoint        Base sync checkpoint that this fence
                                        is formed of

@Input          pszName                 Fence annotation

@Return         MTGPU_OK if a valid fence and finalise data were provided.
                MTGPU_ERROR_INVALID_PARAMS if an invalid fence or finalise
                data were provided.
                MTGPU_ERROR_SYNC_NATIVESYNC_NOT_REGISTERED if the OS native
                sync has not registered a callback function (permitted).
*/
/*****************************************************************************/
MTGPU_ERROR
SyncCheckpointFinaliseFence(PMTGPU_DEVICE_NODE psDevNode,
                            MTGPU_FENCE hFence,
                            void *pvFinaliseData,
                            PSYNC_CHECKPOINT psSyncCheckpoint,
                            const MT_CHAR *pszName);

/*************************************************************************/ /*!
@Function       SyncCheckpointFreeCheckpointListMem

@Description    Free memory the memory which was allocated by the sync
                implementation and used to return the list of sync
                checkpoints when resolving a fence.
                to the fence being returned to the client.
                This function in turn calls a free function registered by
                the sync implementation (if a function has been registered).

@Input          pvCheckpointListMem     Pointer to the memory to be freed

@Return         None
*/
/*****************************************************************************/
void
SyncCheckpointFreeCheckpointListMem(void *pvCheckpointListMem);

/*************************************************************************/ /*!
@Function       SyncCheckpointNoHWUpdateTimelines

@Description    Called by the DDK in a NO_HARDWARE build only.
                After syncs have been manually signalled by the DDK, this
                function is called to allow the OS native sync implementation
                to update its timelines (as the usual callback notification
                of signalled checkpoints is not supported for NO_HARDWARE).
                This function in turn calls a function provided by the
                OS native sync implementation.

@Input          pvPrivateData            Any data the OS native sync
                                         implementation might require.

@Return         MTGPU_ERROR_SYNC_NATIVESYNC_NOT_REGISTERED if the OS native
                sync has not registered a callback function, otherwise
                MTGPU_OK.
*/
/*****************************************************************************/
MTGPU_ERROR
SyncCheckpointNoHWUpdateTimelines(void *pvPrivateData);

/*************************************************************************/ /*!
@Function       SyncCheckpointDumpInfoOnStalledUFOs

@Description    Called by the DDK in the event of the health check watchdog
                examining the CCBs and determining that one has failed to
                progress after 10 second when the GPU is idle due to waiting
                on one or more UFO fences.
                The DDK will pass a list of UFOs on which the CCB is waiting
                and the sync implementation will check them to see if any
                relate to sync points it has created. If so, the
                implementation should dump debug information on those sync
                points to the kernel log or other suitable output (which will
                allow the unsignalled syncs to be identified).
                The function shall return the number of syncs in the provided
                array that were syncs which it had created.

@Input          ui32NumUFOs           The number of UFOs in the array passed
                                      in the pui32VAddrs parameter.
                pui32Vaddr            The array of UFOs the CCB is waiting on.

@Output         pui32NumSyncOwnedUFOs The number of UFOs in pui32Vaddr which
                                      relate to syncs created by the sync
                                      implementation.

@Return         MTGPU_OK if a valid pointer is provided in pui32NumSyncOwnedUFOs.
                MTGPU_ERROR_INVALID_PARAMS if a NULL value is provided in
                pui32NumSyncOwnedUFOs.
                MTGPU_ERROR_SYNC_NATIVESYNC_NOT_REGISTERED if the OS native
                sync has not registered a callback function.

*/
/*****************************************************************************/
MTGPU_ERROR
SyncCheckpointDumpInfoOnStalledUFOs(MT_UINT32 ui32NumUFOs,
                                    MT_UINT32 *pui32Vaddrs,
                                    MT_UINT32 *pui32NumSyncOwnedUFOs);

/*************************************************************************/ /*!
@Function       SyncCheckpointGetStateString

@Description    Called to get a string representing the current state of a
                sync checkpoint.

@Input          psSyncCheckpoint        Synchronisation checkpoint to get the
                                        state for.

@Return         The string representing the current state of this checkpoint
*/
/*****************************************************************************/
const MT_CHAR *
SyncCheckpointGetStateString(PSYNC_CHECKPOINT psSyncCheckpoint);

/*************************************************************************/ /*!
@Function       SyncCheckpointRecordLookup

@Description    Returns a debug string with information about the
                sync checkpoint.

@Input          psDevNode               The device owning the sync
                                        checkpoint to lookup

@Input          ui32FwAddr              The firmware address of the sync
                                        checkpoint to lookup

@Input          pszSyncInfo             Character array to write to

@Input          len                     Len of the character array

@Return         None
*/
/*****************************************************************************/
void
SyncCheckpointRecordLookup(PMTGPU_DEVICE_NODE psDevNode,
                           MT_UINT32 ui32FwAddr,
                           MT_CHAR * pszSyncInfo, size_t len);

/*************************************************************************/ /*!
@Function       SyncCheckpointGetOffset

@Description    Returns the offset of checkpoint in the sync_block.

@Input          psSyncCheckpoint        Synchronisation checkpoint to get the
                                        offset for.

@Return         None
*/
/*****************************************************************************/
MT_UINT32 SyncCheckpointGetOffset(PSYNC_CHECKPOINT psSyncCheckpoint);


MTGPU_ERROR
SyncCheckpointGetTimelineHandleFromFence(MTGPU_FENCE fence, MTGPU_TIMELINE *timeline);

MTGPU_ERROR
SyncCheckpointGetCheckpointsFromFence(MTGPU_FENCE FenceHandle,
				      PSYNC_CHECKPOINT *ppSyncCheckPoint,
				      void **ppvTimelineUpdateSyncPrim,
				      u32 *updateCheckPointNum);

MTGPU_ERROR
SyncCheckpointNotifyMusaHwperfAllocFence(PMTGPU_DEVICE_NODE psDevNode,
					MTGPU_FENCE hFence,
					PSYNC_CHECKPOINT psSyncCheckpoint,
					const MT_CHAR *pszName);

MTGPU_ERROR
SyncCheckpointOnlyHandleFiniliseData(MTGPU_FENCE hFence,
				     void *pvFinaliseData);
#endif	/* SYNC_CHECKPOINT_H */

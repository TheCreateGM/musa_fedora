/*
 * @Copyright   Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License     Dual MIT/GPLv2
 */

#ifndef _MUSAJOB_H_
#define _MUSAJOB_H_

#include "sync_server.h"
#include "mtgpu_job.h"

void MUSAJobFenceSignal(struct mtgpu_job_item *psJobItem, bool guilty);

MTGPU_ERROR MUSADrmSchedulerAddJob(MTGPU_DEVICE_NODE         * psDeviceNode,
				   MT_DEV_PHYADDR 	        sPCBaseAddr,
				   MT_UINT32                   ui32Priority,
				   POS_LOCK                     hLock,
				   struct mtgpu_sched_entity  * psEntity,
				   struct SERVER_MMU_CONTEXT_TAG * psMMUContext,
				   MT_UINT32			ui32SubmissionQueueId,
				   MT_UINT32     		eSubmissionCmd,
				   MT_UINT32	     	        eCcbType,
				   /*update sync_prim*/
				   SYNC_PRIMITIVE_BLOCK      ** pauiUpdateSyncPrimBlock,
				   MT_UINT32                 * paui32UpdateSyncOffset,
				   MT_UINT64                 * paui64UpdateValue,
				   MT_UINT32                   ui32ClientUpdateCount,
				   /*check sync_prim*/
				   SYNC_PRIMITIVE_BLOCK      ** pauiCheckSyncPrimBlock,
				   MT_UINT32                 * paui32CheckSyncOffset,
				   MT_UINT64                 * paui64CheckValue,
				   MT_UINT32                   ui32ClientCheckCount,
				   struct mtgpu_syncobj      ** psCheckSyncObj,
				   MT_UINT32                   psCheckSyncObjCount,
				   struct mtgpu_syncobj       * psUpdateSyncObj,
				   MT_UINT64 		        ui64SubmissionVa,
				   MT_UINT32 		        ui32SubmissionSize,
				   MT_UINT64 		        ui64SubmissionId,
				   MT_UINT32			ui32SubmissionType,
				   MT_UINT32			ui32SubmissionFlags);
#endif /* _MUSAJOB_H_ */

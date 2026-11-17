/*************************************************************************/ /*!
@File
@Title          MUSA TA and 3D Functionality
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Header for the MUSA TA and 3D Functionality
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

#ifndef MUSAGFX_H
#define MUSAGFX_H

#include "devicemem.h"
#include "devicemem_server.h"
#include "device.h"
#include "musadevice.h"
#include "musa_fwif_shared.h"
#include "musa_fwif_resetframework.h"
#include "sync_server.h"
#include "connection_server.h"
#include "musadebug.h"
#include "mt_notifier.h"
#include "ri_server.h"

/* for ddk2.0 */
typedef struct _MUSA_SERVER_RENDER_CONTEXT2_ MUSA_SERVER_RENDER_CONTEXT2;
typedef struct _MUSA_SERVER_RENDER_CONTEXT_ MUSA_SERVER_RENDER_CONTEXT;
typedef struct _MUSA_FREELIST_ MUSA_FREELIST;
typedef struct _MUSA_PMR_NODE_ MUSA_PMR_NODE;

/*****************************************************************************
 * The Design of Data Storage System for Render Targets                      *
 * ====================================================                      *
 *   Relevant for                                                            *
 *     understanding MUSACreateHWRTDataSet & MUSADestroyHWRTDataSet            *
 *                                                                           *
 *                                                                           *
 *        +=========================================+                        *
 *        |           RenderTargetDataSet           |                        *
 *        +---------------|---------|---------------+                        *
 *                        |         |                                        *
 *                        V         V                                        *
 *  +- - - - - - - - - - - - +   +- - - - - - - - - - - - +                  *
 *  | KM_HW_RT_DATA_HANDLE_0 |   | KM_HW_RT_DATA_HANDLE_1 |                  *
 *  +- - -|- - - - - - - - - +   +- - - - - - - - - | - - +                  *
 *        |                                         |                        *
 *        |                                         |           [UM]Client   *
 *  ------|-----------------------------------------|----------------------- *
 *        |                                         |               Bridge   *
 *  ------|-----------------------------------------|----------------------- *
 *        |                                         |           [KM]Server   *
 *        |                                         |                        *
 *        | KM-ptr                                  | KM-ptr                 *
 *        V                                         V                        *
 *  +====================+           +====================+                  *
 *  |  KM_HW_RT_DATA_0   |           |   KM_HW_RT_DATA_1  |                  *
 *  +-----|------------|-+           +-|------------|-----+                  *
 *        |            |               |            |                        *
 *        |            |               |            |                        *
 *        |            |               |            |                        *
 *        |            |               |            |                        *
 *        |            | KM-ptr        | KM-ptr     |                        *
 *        |            V               V            |                        *
 *        |      +==========================+       |                        *
 *        |      | HW_RT_DATA_COMMON_COOKIE |       |                        *
 *        |      +--------------------------+       |                        *
 *        |                   |                     |                        *
 *        |                   |                     |                        *
 *  ------|-------------------|---------------------|----------------------- *
 *        |                   |                     |         [FW]Firmware   *
 *        |                   |                     |                        *
 *        | FW-addr           |                     | FW-addr                *
 *        V                   |                     V                        *
 *  +===============+         |           +===============+                  *
 *  | HW_RT_DATA_0  |         |           | HW_RT_DATA_1  |                  *
 *  +------------|--+         |           +--|------------+                  *
 *               |            |              |                               *
 *               | FW-addr    | FW-addr      | FW-addr                       *
 *               V            V              V                               *
 *        +=========================================+                        *
 *        |           HW_RT_DATA_COMMON             |                        *
 *        +-----------------------------------------+                        *
 *                                                                           *
 *****************************************************************************/

typedef struct _MUSA_HWRTDATA_COMMON_COOKIE_
{
	DEVMEM_MEMDESC			*psHWRTDataCommonFwMemDesc;
	MUSAFWIF_DEV_VIRTADDR	sHWRTDataCommonFwAddr;
	MT_UINT32				ui32RefCount;

} MUSA_HWRTDATA_COMMON_COOKIE;

typedef struct _MUSA_KM_HW_RT_DATASET_
{
	MUSA_HWRTDATA_COMMON_COOKIE *psHWRTDataCommonCookie;

	/* MUSA_RTDATA_CLEANUP_DATA */
	/* MUSAMKIF_NUM_RTDATAS */
	MTGPU_DEVICE_NODE *psDeviceNode;
	MUSAFWIF_DEV_VIRTADDR sHWRTDataFwAddr;

	DEVMEM_MEMDESC *psHWRTDataFwMemDesc;
	DEVMEM_MEMDESC *psRTArrayFwMemDesc;
	DEVMEM_MEMDESC *psRendersAccArrayFwMemDesc;

	MUSA_FREELIST *apsFreeLists[MUSAFW_MAX_FREELISTS];
	MT_UINT32    ui32FreeListsCount;
#if !defined(SUPPORT_SHADOW_FREELISTS)
	DLLIST_NODE sNodeHWRTData;
#endif

	/* For MCG config PC base pages buffer */
	PMR *psPMR;
} MUSA_KM_HW_RT_DATASET;

struct _MUSA_FREELIST_ {
	MTGPU_MUSADEV_INFO		*psDevInfo;
	CONNECTION_DATA			*psConnection;

	/* Free list PMR */
	PMR						*psFreeListPMR;
	MT_DEVMEM_OFFSET_T		uiFreeListPMROffset;

	/* Free list PM state PMR */
	PMR						*psFreeListStatePMR;
	MT_DEVMEM_OFFSET_T		uiFreeListStatePMROffset;

	/* Freelist config */
	MT_UINT32				ui32MaxFLPages;
	MT_UINT32				ui32InitFLPages;
	MT_UINT32				ui32CurrentFLPages;
	MT_UINT32				ui32GrowFLPages;
	MT_UINT32				ui32ReadyFLPages;
	MT_UINT32				ui32GrowThreshold;		/* Percentage of FL memory used that should trigger a new grow request */
	MT_UINT32				ui32FreelistID;
	MT_UINT32				ui32FreelistGlobalID;	/* related global freelist for this freelist */
	MT_UINT64				ui64FreelistChecksum;	/* checksum over freelist content */
	MT_BOOL				bCheckFreelist;			/* freelist check enabled */
	MT_UINT32				ui32RefCount;			/* freelist reference counting */

	MT_UINT32				ui32NumGrowReqByApp;	/* Total number of grow requests by Application */
	MT_UINT32				ui32NumGrowReqByFW;		/* Total Number of grow requests by Firmware */
	MT_UINT32				ui32NumHighPages;		/* High Mark of pages in the freelist */

	MT_PID					ownerPid;				/* Pid of the owner of the list */

	/* Memory Blocks */
	DLLIST_NODE				sMemoryBlockHead;
	DLLIST_NODE				sMemoryBlockInitHead;
	DLLIST_NODE				sNode;
#if !defined(SUPPORT_SHADOW_FREELISTS)
	/* HWRTData nodes linked to local freelist */
	DLLIST_NODE				sNodeHWRTDataHead;
#endif

	/* FW data structures */
	DEVMEM_MEMDESC			*psFWFreelistMemDesc;
	MUSAFWIF_DEV_VIRTADDR	sFreeListFWDevVAddr;

#if defined(SUPPORT_WORKLOAD_ESTIMATION)
	HASH_TABLE*				psWorkloadHashTable;
#endif
};

struct _MUSA_PMR_NODE_ {
	MUSA_FREELIST			*psFreeList;
	PMR						*psPMR;
	PMR_PAGELIST			*psPageList;
	DLLIST_NODE				sMemoryBlock;
	MT_UINT32				ui32NumPages;
	MT_BOOL				bFirstPageMissing;
#if defined(MTGPU_ENABLE_GPU_MEMORY_INFO)
	RI_HANDLE				hRIHandle;
#endif
};

typedef struct {
	MTGPU_MUSADEV_INFO		*psDevInfo;
	DEVMEM_MEMDESC			*psFWZSBufferMemDesc;
	MUSAFWIF_DEV_VIRTADDR	sZSBufferFWDevVAddr;

	DEVMEMINT_RESERVATION	*psReservation;
	PMR						*psPMR;
	DEVMEMINT_MAPPING		*psMapping;
	MTGPU_MEMALLOCFLAGS_T	uiMapFlags;
	MT_UINT32				ui32ZSBufferID;
	MT_UINT32				ui32RefCount;
	MT_BOOL				bOnDemand;

	MT_UINT32				ui32NumReqByApp;		/* Number of Backing Requests from Application */
	MT_UINT32				ui32NumReqByFW;			/* Number of Backing Requests from Firmware */

	MT_PID					owner;

	DLLIST_NODE	sNode;
}MUSA_ZSBUFFER_DATA;

typedef struct {
	MUSA_ZSBUFFER_DATA		*psZSBuffer;
} MUSA_POPULATION;

/* Dump the physical pages of a freelist */
MT_BOOL MUSADumpFreeListPageList(MUSA_FREELIST *psFreeList);

/* Create RenderResource */
MTGPU_ERROR MUSACreateRenderResource(CONNECTION_DATA *psConnection,
				     MTGPU_DEVICE_NODE *psDeviceNode,
				     uint64_t *psPMVAFreeStackStateDevVaddr,
				     uint64_t sVASpaceSegmentDevVAddr,
				     MUSA_FREELIST *apsFreeLists[MUSAMKIF_NUM_RTDATA_FREELISTS],
				     MT_UINT32 ui32McgCoreNum,
				     MUSA_KM_HW_RT_DATASET *pasKMHWRTDataSet[MUSAMKIF_NUM_RTDATAS]);

/* Create HWRTDataSet */
MTGPU_ERROR MUSACreateHWRTDataSet(CONNECTION_DATA          *psConnection,
							      MTGPU_DEVICE_NODE       *psDeviceNode,
							      uint64_t          psVHeapTableDevVAddr,
							      uint64_t          sPMDataDevVAddr[MUSAMKIF_NUM_RTDATAS],
							      uint64_t          sPMSecureDataDevVAddr[MUSAMKIF_NUM_RTDATAS],
							      MT_UINT32                ui32McgCoreNum,
							      uint64_t          *psPMDataAddrMcg,
							      uint64_t          *psTailPtrsDevVAddrMcg,
							      MUSA_FREELIST	           *apsFreeLists[MUSAMKIF_NUM_RTDATA_FREELISTS],
							      MT_UINT32                ui32FreeListsCount,
							      uint64_t		*psPMVAFreeStackStateDevVaddr,
							      uint64_t		sVASpaceSegmentDevVAddr,
							      MT_UINT32                ui32ScreenPixelMax,
							      MT_UINT64                ui64PPPMultiSampleCtl,
							      MT_UINT32                ui32TEStride,
							      uint64_t          asTailPtrsDevVAddr[MUSAMKIF_NUM_GEOMDATAS],
							      MT_UINT32                ui32TPCSize,
							      MT_UINT32                ui32TEScreen,
							      MT_UINT32                ui32TEAA,
							      MT_UINT32                ui32TEMTILE1,
							      MT_UINT32                ui32TEMTILE2,
							      MT_UINT32                ui32RgnStride,
							      MT_UINT32                ui32ISPMergeLowerX,
							      MT_UINT32                ui32ISPMergeLowerY,
							      MT_UINT32                ui32ISPMergeUpperX,
							      MT_UINT32                ui32ISPMergeUpperY,
							      MT_UINT32                ui32ISPMergeScaleX,
							      MT_UINT32                ui32ISPMergeScaleY,
							      MT_UINT16                ui16MaxRTs,
							      MUSA_KM_HW_RT_DATASET     *pasKMHWRTDataSet[MUSAMKIF_NUM_RTDATAS]);

/* Destroy HWRTDataSet */
MTGPU_ERROR MUSADestroyHWRTDataSet(MUSA_KM_HW_RT_DATASET *psKmHwRTDataSet);

/*
	MUSACreateZSBuffer
*/
MTGPU_ERROR MUSACreateZSBufferKM(CONNECTION_DATA * psConnection,
                                 MTGPU_DEVICE_NODE	* psDeviceNode,
                                 DEVMEMINT_RESERVATION	*psReservation,
                                 PMR					*psPMR,
                                 MTGPU_MEMALLOCFLAGS_T		uiMapFlags,
                                 MUSA_ZSBUFFER_DATA			**ppsZSBuffer);

/*
	MUSADestroyZSBufferKM
*/
MTGPU_ERROR MUSADestroyZSBufferKM(MUSA_ZSBUFFER_DATA *psZSBuffer);


/*
 * MUSABackingZSBuffer()
 *
 * Backs ZS-Buffer with physical pages
 */
MTGPU_ERROR
MUSABackingZSBuffer(MUSA_ZSBUFFER_DATA *psZSBuffer);

/*
 * MUSAPopulateZSBufferKM()
 *
 * Backs ZS-Buffer with physical pages (called by Bridge calls)
 */
MTGPU_ERROR MUSAPopulateZSBufferKM(MUSA_ZSBUFFER_DATA *psZSBuffer,
								   MUSA_POPULATION **ppsPopulation);

/*
 * MUSAUnbackingZSBuffer()
 *
 * Frees ZS-Buffer's physical pages
 */
MTGPU_ERROR MUSAUnbackingZSBuffer(MUSA_ZSBUFFER_DATA *psZSBuffer);

/*
 * MUSAUnpopulateZSBufferKM()
 *
 * Frees ZS-Buffer's physical pages (called by Bridge calls)
 */
MTGPU_ERROR MUSAUnpopulateZSBufferKM(MUSA_POPULATION *psPopulation);

/*
	MUSAProcessRequestZSBufferBacking
*/
void MUSAProcessRequestZSBufferBacking(MTGPU_MUSADEV_INFO *psDevInfo,
									  MT_UINT32 ui32ZSBufferID);

/*
	MUSAProcessRequestZSBufferUnbacking
*/
void MUSAProcessRequestZSBufferUnbacking(MTGPU_MUSADEV_INFO *psDevInfo,
										MT_UINT32 ui32ZSBufferID);

/*
	MUSAGrowFreeList
*/
MTGPU_ERROR MUSAGrowFreeList(MUSA_FREELIST *psFreeList,
                             MT_UINT32 ui32NumPages,
                             PDLLIST_NODE pListHeader);

/* Create free list */
MTGPU_ERROR MUSACreateFreeList(CONNECTION_DATA      *psConnection,
                               MTGPU_DEVICE_NODE	*psDeviceNode,
							   MT_HANDLE			hMemCtxPrivData,
							   MT_UINT32			ui32MaxFLPages,
							   MT_UINT32			ui32InitFLPages,
							   MT_UINT32			ui32GrowFLPages,
                               MT_UINT32           ui32GrowParamThreshold,
							   MUSA_FREELIST			*psGlobalFreeList,
							   MT_BOOL				bCheckFreelist,
							   uint64_t		sFreeListBaseDevVAddr,
							   uint64_t		sFreeListStateDevVAddr,
							   PMR					*psFreeListPMR,
							   MT_DEVMEM_OFFSET_T	uiFreeListPMROffset,
							   PMR					*psFreeListStatePMR,
							   MT_DEVMEM_OFFSET_T	uiFreeListStatePMROffset,
							   MUSA_FREELIST			**ppsFreeList);

/* Destroy free list */
MTGPU_ERROR MUSADestroyFreeList(MUSA_FREELIST *psFreeList);

/*
	MUSAProcessRequestGrow
*/
void MUSAProcessRequestGrow(MTGPU_MUSADEV_INFO *psDevInfo,
						   MT_UINT32 ui32FreelistID);


/* Reconstruct free list after Hardware Recovery */
void MUSAProcessRequestFreelistsReconstruction(MTGPU_MUSADEV_INFO *psDevInfo,
											  MT_UINT32 ui32FreelistsCount,
											  const MT_UINT32 *paui32Freelists);

/*!
*******************************************************************************

 @Function	MTGPUMUSACreateRenderContextKM

 @Description
	Server-side implementation of MUSACreateRenderContext

 @Input pvDeviceNode - device node
 @Input ui32Priority - context priority
 @Input hMemCtxPrivData - memory context private data
 @Input ui32PackedCCBSizeU8888 :
		ui8TACCBAllocSizeLog2 - TA CCB size
		ui8TACCBMaxAllocSizeLog2 - maximum size to which TA CCB can grow
		ui83DCCBAllocSizeLog2 - 3D CCB size
		ui83DCCBMaxAllocSizeLog2 - maximum size to which 3D CCB can grow
 @Input ui32ContextFlags - flags which specify properties of the context
 @Output ppsRenderContext - clean up data

 @Return   MTGPU_ERROR

******************************************************************************/
MTGPU_ERROR MTGPUMUSACreateRenderContextKM(CONNECTION_DATA				*psConnection,
											MTGPU_DEVICE_NODE			*psDeviceNode,
											MT_UINT32					ui32Priority,
											MT_UINT32					ui32FrameworkCommandSize,
											MT_PBYTE					pabyFrameworkCommand,
											MT_HANDLE					hMemCtxPrivData,
											MT_UINT32					ui32StaticRenderContextStateSize,
											MT_PBYTE					pStaticRenderContextState,
											MT_UINT32					ui32PackedCCBSizeU8888,
											MT_UINT32					ui32ContextFlags,
											MT_UINT64					ui64RobustnessAddress,
											MT_UINT32					ui32MaxTADeadlineMS,
											MT_UINT32					ui32Max3DDeadlineMS,
											MUSA_SERVER_RENDER_CONTEXT	**ppsRenderContext);

/*!
*******************************************************************************

 @Function	MTGPUMUSADestroyRenderContextKM

 @Description
	Server-side implementation of MUSADestroyRenderContext

 @Input psRenderContext -

 @Return   MTGPU_ERROR

******************************************************************************/
MTGPU_ERROR MTGPUMUSADestroyRenderContextKM(MUSA_SERVER_RENDER_CONTEXT *psRenderContext);


/*!
*******************************************************************************

 @Function	MTGPUMUSAKickGFXKM

 @Description
	Server-side implementation of MUSAKickGFX

 @Input psRTDataCleanup - RT data associated with the kick (or NULL)
 @Input psZBuffer - Z-buffer associated with the kick (or NULL)
 @Input psSBuffer - S-buffer associated with the kick (or NULL)

 @Return   MTGPU_ERROR

******************************************************************************/
MTGPU_ERROR MTGPUMUSAKickGFXKM(MUSA_SERVER_RENDER_CONTEXT	*psRenderContext,
								 MT_UINT32					ui32ClientTAFenceCount,
								 SYNC_PRIMITIVE_BLOCK		**apsClientTAFenceSyncPrimBlock,
								 MT_UINT32					*paui32ClientTAFenceSyncOffset,
								 MT_UINT32					*paui32ClientTAFenceValue,
								 MT_UINT32					ui32ClientTAUpdateCount,
								 SYNC_PRIMITIVE_BLOCK		**apsClientUpdateSyncPrimBlock,
								 MT_UINT32					*paui32ClientUpdateSyncOffset,
								 MT_UINT32					*paui32ClientTAUpdateValue,
								 MT_UINT32					ui32Client3DUpdateCount,
								 SYNC_PRIMITIVE_BLOCK		**apsClient3DUpdateSyncPrimBlock,
								 MT_UINT32					*paui32Client3DUpdateSyncOffset,
								 MT_UINT32					*paui32Client3DUpdateValue,
								 SYNC_PRIMITIVE_BLOCK		*psPRSyncPrimBlock,
								 MT_UINT32					ui32PRSyncOffset,
								 MT_UINT32					ui32PRFenceValue,
								 MTGPU_FENCE				iCheckFence,
								 MTGPU_TIMELINE			iUpdateTimeline,
								 MTGPU_FENCE				*piUpdateFence,
								 MT_CHAR					szFenceName[MTGPU_SYNC_NAME_LENGTH],
								 MTGPU_FENCE				iCheckFence3D,
								 MTGPU_TIMELINE			iUpdateTimeline3D,
								 MTGPU_FENCE				*piUpdateFence3D,
								 MT_CHAR					szFenceName3D[MTGPU_SYNC_NAME_LENGTH],
								 MT_UINT32					ui32TACmdSize,
								 MT_PBYTE					pui8TADMCmd,
								 MT_UINT32					ui323DPRCmdSize,
								 MT_PBYTE					pui83DPRDMCmd,
								 MT_UINT32					ui323DCmdSize,
								 MT_PBYTE					pui83GTCDCmd,
								 MT_UINT32					ui32ExtJobRef,
								 MT_BOOL					bKickTA,
								 MT_BOOL					bKickPR,
								 MT_BOOL					bKick3D,
								 MT_BOOL					bAbort,
								 MT_UINT32					ui32PDumpFlags,
								 MUSA_KM_HW_RT_DATASET		*psKMHWRTDataSet,
								 MUSA_ZSBUFFER_DATA			*psZSBuffer,
								 MUSA_ZSBUFFER_DATA			*psMSAAScratchBuffer,
								 MT_UINT32					ui32SyncPMRCount,
								 MT_UINT32					*paui32SyncPMRFlags,
								 PMR						**ppsSyncPMRs,
								 MT_UINT32					ui32RenderTargetSize,
								 MT_UINT32					ui32NumberOfDrawCalls,
								 MT_UINT32					ui32NumberOfIndices,
								 MT_UINT32					ui32NumberOfMRTs,
								 MT_UINT64					ui64DeadlineInus);

MTGPU_ERROR MTGPUMUSAKickGFX3KM(MUSA_SERVER_RENDER_CONTEXT	*psRenderContext,
				  MT_UINT32	ui32ClientTAFenceCount,
				  SYNC_PRIMITIVE_BLOCK	**apsClientTAFenceSyncPrimBlock,
				  MT_UINT32	*paui32ClientTAFenceSyncOffset,
				  MT_UINT32	*paui32ClientTAFenceValue,
				  MT_UINT32	ui32ClientTAUpdateCount,
				  SYNC_PRIMITIVE_BLOCK	**apsClientTAUpdateSyncPrimBlock,
				  MT_UINT32	*paui32ClientTAUpdateSyncOffset,
				  MT_UINT32	*paui32ClientTAUpdateValue,
				  MT_UINT32	ui32Client3DUpdateCount,
				  SYNC_PRIMITIVE_BLOCK	**apsClient3DUpdateSyncPrimBlock,
				  MT_UINT32	*paui32Client3DUpdateSyncOffset,
				  MT_UINT32	*paui32Client3DUpdateValue,
				  SYNC_PRIMITIVE_BLOCK	*psPRFenceSyncPrimBlock,
				  MT_UINT32	ui32PRFenceSyncOffset,
				  MT_UINT32	ui32PRFenceValue,
				  MTGPU_FENCE	iCheckTAFence,
				  MTGPU_FENCE	iUpdateTAFence,
				  MT_CHAR	szFenceNameTA[MTGPU_SYNC_NAME_LENGTH],
				  MTGPU_FENCE	iCheck3DFence,
				  MTGPU_FENCE	iUpdate3DFence,
				  MT_CHAR	szFenceName3D[MTGPU_SYNC_NAME_LENGTH],
				  MT_UINT32	ui32TACmdSize,
				  MT_PBYTE	pui8TADMCmd,
				  MT_UINT32	ui323DPRCmdSize,
				  MT_PBYTE	pui83DPRDMCmd,
				  MT_UINT32	ui323DCmdSize,
				  MT_PBYTE	pui83GTCDCmd,
				  MT_UINT32	ui32ExtJobRef,
				  MT_BOOL	bKickTA,
				  MT_BOOL	bKickPR,
				  MT_BOOL	bKick3D,
				  MT_BOOL	bAbort,
				  MT_UINT32	ui32PDumpFlags,
				  MUSA_KM_HW_RT_DATASET	*psKMHWRTDataSet,
				  MUSA_ZSBUFFER_DATA	*psZSBuffer,
				  MUSA_ZSBUFFER_DATA	*psMSAAScratchBuffer,
				  MT_UINT32	ui32SyncPMRCount,
				  MT_UINT32	*paui32SyncPMRFlags,
				  PMR	**ppsSyncPMRs,
				  MT_UINT32	ui32RenderTargetSize,
				  MT_UINT32	ui32NumberOfDrawCalls,
				  MT_UINT32	ui32NumberOfIndices,
				  MT_UINT32	ui32NumberOfMRTs,
				  MT_UINT64	ui64DeadlineInus);

MTGPU_ERROR MTGPUMUSAKickNopRender(MUSA_SERVER_RENDER_CONTEXT	*psRenderContext,
				    CONNECTION_DATA		*psConnection,
				    struct drm_mtgpu_semaphore	*pasTACheckSemas,
				    MT_UINT32			ui32TACheckSemaCount,
				    struct drm_mtgpu_semaphore	*pasTAUpdateSemas,
				    MT_UINT32			ui32TAUpdateSemaCount,
				    struct drm_mtgpu_semaphore	*pas3DCheckSemas,
				    MT_UINT32			ui323DCheckSemaCount,
				    struct drm_mtgpu_semaphore	*pas3DUpdateSemas,
				    MT_UINT32			ui323DUpdateSemaCount,
				    struct drm_mtgpu_fence	*pasTACheckFences,
				    MT_UINT32			ui32TACheckFenceCount,
				    struct drm_mtgpu_fence	*psUpdateFence,
				    MT_CHAR			szUpdateFenceName[MTGPU_SYNC_NAME_LENGTH],
				    MT_UINT32			ui32ExtJobRef,
				    MT_UINT64			ui64DeadlineInus,
				    MT_UINT32			ui32PDumpFlags);

MTGPU_ERROR MTGPUMUSAKickGFXKMV2(MUSA_SERVER_RENDER_CONTEXT	*psRenderContext,
				   CONNECTION_DATA		*psConnection,
				   struct drm_mtgpu_semaphore	*pasTACheckSemas,
				   MT_UINT32			ui32TACheckSemaCount,
				   struct drm_mtgpu_semaphore	*pasTAUpdateSemas,
				   MT_UINT32			ui32TAUpdateSemaCount,
				   struct drm_mtgpu_semaphore	*pas3DCheckSemas,
				   MT_UINT32			ui323DCheckSemaCount,
				   struct drm_mtgpu_semaphore	*pas3DUpdateSemas,
				   MT_UINT32			ui323DUpdateSemaCount,
				   struct drm_mtgpu_fence	*pasTACheckFences,
				   MT_UINT32			ui32TACheckFenceCount,
				   MT_INT32			iForeignTAFenceFd,
				   MT_INT32			iForeign3DFenceFd,
				   struct drm_mtgpu_fence	*psUpdate3DFence,
				   MT_CHAR			szUpdateFenceName3D[MTGPU_SYNC_NAME_LENGTH],
				   MT_UINT32			*paui32SyncBufFlags,
				   struct dma_buf		**ppsSyncBufs,
				   MT_UINT32			ui32SyncBufCount,
				   MT_UINT32			ui32TACmdSize,
				   MT_PBYTE			pui8TADMCmd,
				   MT_UINT32			ui32TACmdCount,
				   MT_UINT32			ui323DPRCmdSize,
				   MT_PBYTE			pui83DPRDMCmd,
				   MT_UINT32			ui323DPRCmdCount,
				   MT_UINT32			ui323DCmdSize,
				   MT_PBYTE			pui83GTCDCmd,
				   MT_UINT32			ui323DCmdCount,
				   MT_BOOL			bKickTA,
				   MT_BOOL			bKickPR,
				   MT_BOOL			bKick3D,
				   MT_BOOL			bAbort,
				   MUSA_KM_HW_RT_DATASET		*psKMHWRTDataSet,
				   MUSA_ZSBUFFER_DATA		*psZSBuffer,
				   MUSA_ZSBUFFER_DATA		*psMSAAScratchBuffer,
				   MT_UINT32			ui32RenderTargetSize,
				   MT_UINT32			ui32NumberOfDrawCalls,
				   MT_UINT32			ui32NumberOfIndices,
				   MT_UINT32			ui32NumberOfMRTs,
				   MT_UINT32			ui32ExtJobRef,
				   MT_UINT64			ui64DeadlineInus,
				   MT_UINT32			ui32PDumpFlags);

MTGPU_ERROR MTGPUMUSASetRenderContextPriorityKM(CONNECTION_DATA *psConnection,
                                                 MTGPU_DEVICE_NODE * psDevNode,
                                                 MUSA_SERVER_RENDER_CONTEXT *psRenderContext,
                                                 MT_UINT32 ui32Priority);

MTGPU_ERROR MTGPUMUSASetRenderContextPropertyKM(MUSA_SERVER_RENDER_CONTEXT *psRenderContext,
												 MUSA_CONTEXT_PROPERTY eContextProperty,
												 MT_UINT64 ui64Input,
												 MT_UINT64 *pui64Output);

MTGPU_ERROR MUSARenderContextStalledKM(MUSA_SERVER_RENDER_CONTEXT *psRenderContext);

MTGPU_ERROR MTGPUMUSARenderAcquireCommonContextData(void *psContext,
						     void *psContextOut);

struct workqueue_struct *MTGPUMUSARenderGetWorkqueue(MUSA_SERVER_RENDER_CONTEXT* psRenderContext);

/*******************************************
                    DDK2.0
 *******************************************/

/*!
*******************************************************************************

 @Function      MTGPUMUSACreateRenderContextKM2

 @Description
        Server-side implementation of MUSACreateRenderContext2

 @Input pvDeviceNode - device node
 @Input ui32Priority - context priority
 @Input hMemCtxPrivData - memory context private data
 @Output ppsRenderContext - clean up data

 @Return   MTGPU_ERROR

******************************************************************************/
MTGPU_ERROR MTGPUMUSACreateRenderContextKM2(CONNECTION_DATA                 * psConnection,
					     MTGPU_DEVICE_NODE              * psDeviceNode,
					     MT_UINT32                        ui32Priority,
					     MT_HANDLE                        hMemCtxPrivData,
					     MUSA_SERVER_RENDER_CONTEXT2     ** ppsRenderContextInt);

/*!
*******************************************************************************

 @Function	MTGPUMUSADestroyRenderContextKM2

 @Description
	Server-side implementation of MUSADestroyRenderContext2

 @Input psRenderContext -

 @Return   MTGPU_ERROR

******************************************************************************/
MTGPU_ERROR MTGPUMUSADestroyRenderContextKM2(MUSA_SERVER_RENDER_CONTEXT2*psRenderContext);

/*!
*******************************************************************************
 @Function	MTGPUMUSAGFX5KM

 @Description
	Server-side implementation of MUSAGFX5

 @Return   MTGPU_ERROR
******************************************************************************/
MTGPU_ERROR MTGPUMUSAGFX5KM(
	MUSA_SERVER_RENDER_CONTEXT2 * psRenderContext,
	MT_HANDLE                   hRenderContext,
	/*update sync_prim*/
	SYNC_PRIMITIVE_BLOCK      ** pauiUpdateSyncPrimBlock,
	MT_UINT32                 * paui32UpdateSyncOffset,
	MT_UINT64                 * paui64UpdateValue,
	MT_UINT32                   ui32UpdateCount,
	/*check sync_prim*/
	SYNC_PRIMITIVE_BLOCK      ** pauiCheckSyncPrimBlock,
	MT_UINT32                 * paui32CheckSyncOffset,
	MT_UINT64                 * paui64CheckValue,
	MT_UINT32                   ui32CheckCount,
	/* buffer sync */
	MT_UINT32                   ui32SyncPMRCount,
	MT_UINT32                 * paui32SyncPMRFlags,
	PMR                       ** ppsSyncPMRs,
	MT_UINT32		     ui32SubmissionFlags,
	MT_UINT64                   ui64SubmissionVa,
	MT_UINT32                   ui32SubmissionSize,
	MT_UINT64                   ui64SubmissionId);

#endif /* MUSAGFX_H */

/*************************************************************************/ /*!
@File
@Title          MUSA HW Performance header file
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Header for the MUSA HWPerf functions
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

#ifndef MUSAHWPERF_COMMON_H_
#define MUSAHWPERF_COMMON_H_

#include "img_types.h"
#include "img_defs.h"
#include "mtgpu_error.h"

#include "device.h"
#include "connection_server.h"
#include "musadevice.h"
#include "musa_hwperf.h"

/* HWPerf host buffer size constraints in KBs */
#define HWPERF_HOST_TL_STREAM_SIZE_DEFAULT MTGPU_APPHINT_HWPERFHOSTBUFSIZEINKB
#define HWPERF_HOST_TL_STREAM_SIZE_MIN     (32U)
#define HWPERF_HOST_TL_STREAM_SIZE_MAX     (3072U)

/******************************************************************************
 * MUSA HW Performance decode Bvnc Features for HWPerf
 *****************************************************************************/
MTGPU_ERROR MUSAServerFeatureFlagsToHWPerfFlags(MTGPU_MUSADEV_INFO *psDevInfo,
		                                        MUSA_HWPERF_BVNC    *psBVNC);

MTGPU_ERROR MTGPUMUSAGetHWPerfBvncFeatureFlagsKM(CONNECTION_DATA    *psConnection,
                                                  MTGPU_DEVICE_NODE *psDeviceNode,
											      MUSA_HWPERF_BVNC    *psBVNC);

/******************************************************************************
 * MUSA HW Performance Data Transport Routines
 *****************************************************************************/

MTGPU_ERROR MUSAHWPerfDataStoreCB(MTGPU_DEVICE_NODE* psDevInfo);

MTGPU_ERROR MUSAHWPerfInit(MTGPU_MUSADEV_INFO *psMusaDevInfo);
MTGPU_ERROR MUSAHWPerfInitOnDemandResources(MTGPU_MUSADEV_INFO* psMusaDevInfo);
void MUSAHWPerfDeinit(MTGPU_MUSADEV_INFO *psMusaDevInfo);
void MUSAHWPerfInitAppHintCallbacks(const MTGPU_DEVICE_NODE *psDeviceNode);
void MUSAHWPerfClientInitAppHintCallbacks(void);

/******************************************************************************
 * MUSA HW Performance Profiling API(s)
 *****************************************************************************/
MTGPU_ERROR MUSAHWPerfConnectionConnect(CONNECTION_DATA *psConnection);

typedef struct _HWPERF_SETTING_
{
	MT_UINT64 	ui64HWPerfFilter;
	MT_UINT32	ui32HWPerfHostFilter;
#define MUSA_HWPERF_CLIENT_API_COUNT (HWPERF_INFO_IDX_END - HWPERF_INFO_IDX_START)
	MT_UINT32	ui32HWPerfClientFilter[MUSA_HWPERF_CLIENT_API_COUNT];

	MT_UINT32	ui32CtrlWord;
	MT_UINT32	ui32NumConfiguredBlocks;
	MUSA_HWPERF_CONFIG_CNTBLK asBlockConfigs[1];
} HWPERF_SETTING;

typedef struct _HWPERF_CONNECTION_DATA_
{
	MT_BOOL	bConnected;
	HWPERF_SETTING *psSetting;
	DLLIST_NODE	sListNode;
} HWPERF_CONNECTION_DATA;

MTGPU_ERROR MTGPUMUSACtrlHWPerfKM(
	HWPERF_CONNECTION_DATA *psHWPerfConnection,
	MTGPU_DEVICE_NODE *psDeviceNode,
	MUSA_HWPERF_STREAM_ID eStreamId,
	MT_BOOL bToggle,
	MT_UINT64 ui64Mask);

MTGPU_ERROR MTGPUMUSAGetHWPerfMaskKM(MTGPU_DEVICE_NODE *psDeviceNode,
				      MUSA_HWPERF_STREAM_ID eStreamId,
				      MT_UINT64 *pui64Mask);

MTGPU_ERROR MTGPUMUSAGetSOCTimerKM(MTGPU_DEVICE_NODE *psDeviceNode,
				    MT_UINT64 *pui64SocTimestamp,
				    MT_UINT64 *pui64OSTimestamp);

MTGPU_ERROR MTGPUMUSAAcquireHWPerfSettingKM(CONNECTION_DATA *psConnection,
					     PMR **ppsPMR);

MTGPU_ERROR MTGPUMUSAReleaseHWPerfSettingKM(PMR *psPMR);

MTGPU_ERROR MTGPUMUSAControlHWPerfBlocksKM(
	CONNECTION_DATA       * psConnection,
	MTGPU_DEVICE_NODE    * psDeviceNode,
	MT_BOOL              bEnable,
	MT_UINT32            ui32ArrayLen,
	MT_UINT16            * psBlockIDs);

/******************************************************************************
 * MUSA HW Performance Host Stream API
 *****************************************************************************/

MTGPU_ERROR MUSAHWPerfHostInit(MTGPU_MUSADEV_INFO *psMusaDevInfo, MT_UINT32 ui32BufSizeKB);
MTGPU_ERROR MUSAHWPerfHostInitOnDemandResources(MTGPU_MUSADEV_INFO* psMusaDevInfo);
void MUSAHWPerfHostDeInit(MTGPU_MUSADEV_INFO	*psMusaDevInfo);

void MUSAHWPerfHostSetEventFilter(MTGPU_MUSADEV_INFO *psMusaDevInfo,
                                 MT_UINT32 ui32Filter);

void MUSAHWPerfHostPostRaw(MTGPU_MUSADEV_INFO *psMusaDevInfo,
						  MT_UINT32 eEvType,
						  MT_BYTE *pbPayload,
						  MT_UINT32 ui32PayloadSize);

void MUSAHWPerfHostPostEnqEvent(MTGPU_MUSADEV_INFO *psMusaDevInfo,
                               MUSA_HWPERF_KICK_TYPE eEnqType,
                               MT_UINT32 ui32Pid,
                               MT_UINT32 ui32FWDMContext,
                               MT_UINT32 ui32ExtJobRef,
                               MT_UINT32 ui32IntJobRef,
                               MTGPU_FENCE hCheckFence,
                               MTGPU_FENCE hUpdateFence,
                               MTGPU_TIMELINE hUpdateTimeline,
                               MT_UINT64 ui64CheckFenceUID,
                               MT_UINT64 ui64UpdateFenceUID,
                               MT_UINT64 ui64DeadlineInus,
                               MT_UINT32 ui32CycleEstimate);

void MUSAHWPerfHostPostNotifyEvent(MTGPU_MUSADEV_INFO *psMusaDevInfo,
				  MUSA_HWPERF_KICK_TYPE eNotifyType,
				  MT_UINT32 ui32Pid,
				  MT_UINT32 ui32FWDMContext);

void MUSAHWPerfHostPostAllocEvent(MTGPU_MUSADEV_INFO *psMusaDevInfo,
                                 MUSA_HWPERF_HOST_RESOURCE_TYPE eAllocType,
                                 const MT_CHAR *psName,
                                 MT_UINT32 ui32NameSize,
                                 MUSA_HWPERF_HOST_ALLOC_DETAIL *puAllocDetail);

void MUSAHWPerfHostPostFreeEvent(MTGPU_MUSADEV_INFO *psMusaDevInfo,
                                MUSA_HWPERF_HOST_RESOURCE_TYPE eFreeType,
                                MT_UINT64 ui64UID,
                                MT_UINT32 ui32PID,
                                MT_UINT32 ui32FWAddr);

void MUSAHWPerfHostPostModifyEvent(MTGPU_MUSADEV_INFO *psMusaDevInfo,
                                  MUSA_HWPERF_HOST_RESOURCE_TYPE eModifyType,
                                  MT_UINT64 ui64NewUID,
                                  MT_UINT64 ui64UID1,
                                  MT_UINT64 ui64UID2,
                                  const MT_CHAR *psName,
                                  MT_UINT32 ui32NameSize);

void MUSAHWPerfHostPostUfoEvent(MTGPU_MUSADEV_INFO *psMusaDevInfo,
                               MUSA_HWPERF_UFO_EV eUfoType,
                               MUSA_HWPERF_UFO_DATA_ELEMENT *psUFOData,
							   const MT_BOOL bSleepAllowed);

void MUSAHWPerfHostPostClkSyncEvent(MTGPU_MUSADEV_INFO *psMusaDevInfo);

void MUSAHWPerfHostPostDeviceInfo(MTGPU_MUSADEV_INFO *psMusaDevInfo,
								 MUSA_HWPERF_DEV_INFO_EV eEvType,
								 MTGPU_DEVICE_HEALTH_STATUS eDeviceHealthStatus,
								 MTGPU_DEVICE_HEALTH_REASON eDeviceHeathReason);

void MUSAHWPerfHostPostInfo(MTGPU_MUSADEV_INFO *psMusaDevInfo,
								 MUSA_HWPERF_INFO_EV eEvType);

void MUSAHWPerfHostPostFenceWait(MTGPU_MUSADEV_INFO *psMusaDevInfo,
								MUSA_HWPERF_HOST_SYNC_FENCE_WAIT_TYPE eType,
								MT_PID uiPID,
								MTGPU_FENCE hFence,
								MT_UINT32 ui32Data);

void MUSAHWPerfHostPostSWTimelineAdv(MTGPU_MUSADEV_INFO *psMusaDevInfo,
                                    MT_PID uiPID,
									MTGPU_TIMELINE hSWTimeline,
									MT_UINT64 ui64SyncPtIndex);

void MUSAHWPerfHostPostClientInfoProcName(MTGPU_MUSADEV_INFO *psMusaDevInfo,
                                         MT_PID uiPID,
									     const MT_CHAR *psName);

void MUSAHWPerfHostPostTimeCorrelation(MTGPU_MUSADEV_INFO *psMusaDevInfo,
									  MT_UINT32 ui32StartIndex,
									  MT_UINT32 ui32Count);

void MUSAHWPerfHostPostDMAEvent(MTGPU_MUSADEV_INFO *psMusaDevInfo,
			       MUSA_HWPERF_HOST_EVENT_TYPE eEventType,
			       MUSA_HWPERF_HOST_DMA_TYPE eDMAType,
			       MUSA_HWPERF_HOST_DMA_DETAIL *psDetail,
			       MT_PID uiPID,
			       MT_PID uiTID,
			       MT_DEVMEM_SIZE_T uiSize,
			       MTGPU_TIMELINE hTimeline,
			       MT_UINT32 ui32ExtJobRef);

void MUSAHWPerfHostPostDebugEvent(MTGPU_MUSADEV_INFO *psMusaDevInfo,
				 MUSA_HWPERF_HOST_EVENT_TYPE eEventType,
				 MT_UINT64 *aui64Values,
				 MT_UINT32 ui32ValuesCount,
				 const MT_CHAR *pszName);

MT_BOOL MUSAHWPerfHostIsEventEnabled(MTGPU_MUSADEV_INFO *psMusaDevInfo, MUSA_HWPERF_HOST_EVENT_TYPE eEvent);

#define _MUSA_HWPERF_HOST_FILTER(CTX, EV) \
		(((MTGPU_MUSADEV_INFO *)CTX->psDeviceNode->pvDevice)->ui32HWPerfHostFilter \
		& MUSA_HWPERF_EVENT_MASK_VALUE(EV))

#define _MUSA_DEVICE_INFO_FROM_CTX(CTX) \
		((MTGPU_MUSADEV_INFO *)CTX->psDeviceNode->pvDevice)

#define _MUSA_DEVICE_INFO_FROM_NODE(DEVNODE) \
		((MTGPU_MUSADEV_INFO *)DEVNODE->pvDevice)

/* Deadline and cycle estimate is not supported for all ENQ events */
#define NO_DEADLINE 0
#define NO_CYCEST   0


#if defined(SUPPORT_MUSA)

/**
 * This macro checks if HWPerfHost and the event are enabled and if they are
 * it posts event to the HWPerfHost stream.
 *
 * @param C      Kick context
 * @param P      Pid of kicking process
 * @param X      Related FW context
 * @param E      External job reference
 * @param I      Job ID
 * @param K      Kick type
 * @param CF     Check fence handle
 * @param UF     Update fence handle
 * @param UT     Update timeline (on which above UF was created) handle
 * @param CHKUID Check fence UID
 * @param UPDUID Update fence UID
 * @param D      Deadline
 * @param CE     Cycle estimate
 */
#define MUSASRV_HWPERF_ENQ(C, P, X, E, I, K, CF, UF, UT, CHKUID, UPDUID, D, CE) \
		do { \
			if (_MUSA_HWPERF_HOST_FILTER(C, MUSA_HWPERF_HOST_ENQ)) \
			{ \
				MUSAHWPerfHostPostEnqEvent(_MUSA_DEVICE_INFO_FROM_CTX(C), \
				                          (K), (P), (X), (E), (I), \
				                          (CF), (UF), (UT), \
				                          (CHKUID), (UPDUID), (D), (CE)); \
			} \
		} while (0)

/**
 * @param C      Kick context
 * @param P      Pid of kicking process
 * @param X      Related FW context
 * @param K      Kick type
*/
#define MUSASRV_HWPERF_NOTIFY(C, P, X, K) \
do { \
	if (_MUSA_HWPERF_HOST_FILTER(C, MUSA_HWPERF_HOST_NOTIFY)) \
	{ \
		MUSAHWPerfHostPostNotifyEvent(_MUSA_DEVICE_INFO_FROM_CTX(C), \
					     (K), (P), (X)); \
	} \
} while (0)

/**
 * This macro checks if HWPerfHost and the event are enabled and if they are
 * it posts event to the HWPerfHost stream.
 *
 * @param I Device Info pointer
 * @param T Host UFO event type
 * @param D Pointer to UFO data
 * @param S Is sleeping allowed?
 */
#define MUSASRV_HWPERF_UFO(I, T, D, S) \
		do { \
			if (MUSAHWPerfHostIsEventEnabled((I), MUSA_HWPERF_HOST_UFO)) \
			{ \
				MUSAHWPerfHostPostUfoEvent((I), (T), (D), (S)); \
			} \
		} while (0)

/**
 * This macro checks if HWPerfHost and the event are enabled and if they are
 * it posts event to the HWPerfHost stream.
 *
 * @param D Device node pointer
 * @param T Host ALLOC event type
 * @param FWADDR sync firmware address
 * @param N string containing sync name
 * @param Z string size including null terminating character
 */
#define MUSASRV_HWPERF_ALLOC(D, T, FWADDR, N, Z) \
		do { \
			if (MUSAHWPerfHostIsEventEnabled(_MUSA_DEVICE_INFO_FROM_NODE(D), MUSA_HWPERF_HOST_ALLOC)) \
			{ \
				MUSA_HWPERF_HOST_ALLOC_DETAIL uAllocDetail; \
				uAllocDetail.sSyncAlloc.ui32FWAddr = (FWADDR); \
				MUSAHWPerfHostPostAllocEvent(_MUSA_DEVICE_INFO_FROM_NODE(D), \
				                            MUSA_HWPERF_HOST_RESOURCE_TYPE_##T, \
											(N), (Z), &uAllocDetail); \
			} \
		} while (0)

/**
 * This macro checks if HWPerfHost and the event are enabled and if they are
 * it posts event to the HWPerfHost stream.
 *
 * @param D Device Node pointer
 * @param PID ID of allocating process
 * @param FENCE MTGPU_FENCE object
 * @param FWADDR sync firmware address
 * @param N string containing sync name
 * @param Z string size including null terminating character
 */
#define MUSASRV_HWPERF_ALLOC_FENCE(D, PID, FENCE, FWADDR, N, Z)  \
		do { \
			if (MUSAHWPerfHostIsEventEnabled(_MUSA_DEVICE_INFO_FROM_NODE(D), MUSA_HWPERF_HOST_ALLOC)) \
			{ \
				MUSA_HWPERF_HOST_ALLOC_DETAIL uAllocDetail; \
				uAllocDetail.sFenceAlloc.uiPID = (PID); \
				uAllocDetail.sFenceAlloc.hFence = (FENCE); \
				uAllocDetail.sFenceAlloc.ui32CheckPt_FWAddr = (FWADDR); \
				MUSAHWPerfHostPostAllocEvent(_MUSA_DEVICE_INFO_FROM_NODE(D), \
				                            MUSA_HWPERF_HOST_RESOURCE_TYPE_FENCE_MT, \
				                            N, Z, &uAllocDetail); \
			} \
		} while (0)

/**
 * @param D Device Node pointer
 * @param TL MTGPU_TIMELINE on which CP is allocated
 * @param PID Allocating process ID of this TL/FENCE
 * @param FENCE MTGPU_FENCE as passed to SyncCheckpointResolveFence OR MTGPU_NO_FENCE
 * @param FWADDR sync firmware address
 * @param N string containing sync name
 * @param Z string size including null terminating character
 */
#define MUSASRV_HWPERF_ALLOC_SYNC_CP(D, TL, PID, FENCE, FWADDR, N, Z)  \
		do { \
			if (MUSAHWPerfHostIsEventEnabled(_MUSA_DEVICE_INFO_FROM_NODE(D), MUSA_HWPERF_HOST_ALLOC)) \
			{ \
				MUSA_HWPERF_HOST_ALLOC_DETAIL uAllocDetail; \
				uAllocDetail.sSyncCheckPointAlloc.ui32CheckPt_FWAddr = (FWADDR); \
				uAllocDetail.sSyncCheckPointAlloc.hTimeline = (TL); \
				uAllocDetail.sSyncCheckPointAlloc.uiPID = (PID); \
				uAllocDetail.sSyncCheckPointAlloc.hFence = (FENCE); \
				MUSAHWPerfHostPostAllocEvent(_MUSA_DEVICE_INFO_FROM_NODE(D), \
				                            MUSA_HWPERF_HOST_RESOURCE_TYPE_SYNC_CP, \
				                            N, Z, &uAllocDetail); \
			} \
		} while (0)

/**
 * @param D Device Node pointer
 * @param PID ID of allocating process
 * @param SW_FENCE MTGPU_FENCE object
 * @param SW_TL MTGPU_TIMELINE on which SW_FENCE is allocated
 * @param SPI Sync point index on the SW_TL on which this SW_FENCE is allocated
 * @param N string containing sync name
 * @param Z string size including null terminating character
 */
#define MUSASRV_HWPERF_ALLOC_SW_FENCE(D, PID, SW_FENCE, SW_TL, SPI, N, Z)  \
		do { \
			if (MUSAHWPerfHostIsEventEnabled(_MUSA_DEVICE_INFO_FROM_NODE(D), MUSA_HWPERF_HOST_ALLOC)) \
			{ \
				MUSA_HWPERF_HOST_ALLOC_DETAIL uAllocDetail; \
				uAllocDetail.sSWFenceAlloc.uiPID = (PID); \
				uAllocDetail.sSWFenceAlloc.hSWFence = (SW_FENCE); \
				uAllocDetail.sSWFenceAlloc.hSWTimeline = (SW_TL); \
				uAllocDetail.sSWFenceAlloc.ui64SyncPtIndex = (SPI); \
				MUSAHWPerfHostPostAllocEvent(_MUSA_DEVICE_INFO_FROM_NODE(D), \
				                            MUSA_HWPERF_HOST_RESOURCE_TYPE_FENCE_SW, \
				                            N, Z, &uAllocDetail); \
			} \
		} while (0)

/**
 * This macro checks if HWPerfHost and the event are enabled and if they are
 * it posts event to the HWPerfHost stream.
 *
 * @param D Device Node pointer
 * @param T Host ALLOC event type
 * @param FWADDR sync firmware address
 */
#define MUSASRV_HWPERF_FREE(D, T, FWADDR) \
		do { \
			if (MUSAHWPerfHostIsEventEnabled(_MUSA_DEVICE_INFO_FROM_NODE(D), MUSA_HWPERF_HOST_FREE)) \
			{ \
				MUSAHWPerfHostPostFreeEvent(_MUSA_DEVICE_INFO_FROM_NODE(D), \
				                           MUSA_HWPERF_HOST_RESOURCE_TYPE_##T, \
				                           (0), (0), (FWADDR)); \
			} \
		} while (0)

/**
 * This macro checks if HWPerfHost and the event are enabled and if they are
 * it posts event to the HWPerfHost stream.
 *
 * @param D Device Node pointer
 * @param T Host ALLOC event type
 * @param UID ID of input object
 * @param PID ID of allocating process
 * @param FWADDR sync firmware address
 */
#define MUSASRV_HWPERF_FREE_FENCE_SYNC(D, T, UID, PID, FWADDR) \
		do { \
			if (MUSAHWPerfHostIsEventEnabled(_MUSA_DEVICE_INFO_FROM_NODE(D), MUSA_HWPERF_HOST_FREE)) \
			{ \
				MUSAHWPerfHostPostFreeEvent(_MUSA_DEVICE_INFO_FROM_NODE(D), \
				                           MUSA_HWPERF_HOST_RESOURCE_TYPE_##T, \
				                           (UID), (PID), (FWADDR)); \
			} \
		} while (0)

/**
 * This macro checks if HWPerfHost and the event are enabled and if they are
 * it posts event to the HWPerfHost stream.
 *
 * @param D Device Node pointer
 * @param T Host ALLOC event type
 * @param NEWUID ID of output object
 * @param UID1 ID of first input object
 * @param UID2 ID of second input object
 * @param N string containing new object's name
 * @param Z string size including null terminating character
 */
#define MUSASRV_HWPERF_MODIFY_FENCE_SYNC(D, T, NEWUID, UID1, UID2, N, Z) \
		do { \
			if (MUSAHWPerfHostIsEventEnabled(_MUSA_DEVICE_INFO_FROM_NODE(D), MUSA_HWPERF_HOST_MODIFY)) \
			{ \
				MUSAHWPerfHostPostModifyEvent(_MUSA_DEVICE_INFO_FROM_NODE(D), \
				                             MUSA_HWPERF_HOST_RESOURCE_TYPE_##T, \
				                             (NEWUID), (UID1), (UID2), N, Z); \
			} \
		} while (0)


/**
 * This macro checks if HWPerfHost and the event are enabled and if they are
 * it posts event to the HWPerfHost stream.
 *
 * @param I Device info pointer
 */
#define MUSASRV_HWPERF_CLK_SYNC(I) \
		do { \
			if (MUSAHWPerfHostIsEventEnabled((I), MUSA_HWPERF_HOST_CLK_SYNC)) \
			{ \
				MUSAHWPerfHostPostClkSyncEvent((I)); \
			} \
		} while (0)


/**
 * This macro checks if HWPerfHost and the event are enabled and if they are
 * it posts a device info event to the HWPerfHost stream.
 *
 * @param I      Device info pointer
 * @param T      Event type
 * @param H		 Health status enum
 * @param R		 Health reason enum
 */
#define MUSASRV_HWPERF_DEVICE_INFO(I, T, H, R) \
		do { \
			if (MUSAHWPerfHostIsEventEnabled((I), MUSA_HWPERF_HOST_DEV_INFO)) \
			{ \
				MUSAHWPerfHostPostDeviceInfo((I), (T), (H), (R)); \
			} \
		} while (0)

/**
 * This macro checks if HWPerfHost and the event are enabled and if they are
 * it posts event to the HWPerfHost stream.
 *
 * @param I      Device info pointer
 * @param T      Event type
 */
#define MUSASRV_HWPERF_HOST_INFO(I, T) \
do { \
	if (MUSAHWPerfHostIsEventEnabled((I), MUSA_HWPERF_HOST_INFO)) \
	{ \
		MUSAHWPerfHostPostInfo((I), (T)); \
	} \
} while (0)

/**
 * @param I      Device info pointer
 * @param T      Wait Event type
 * @param PID    Process ID that the following fence belongs to
 * @param F      Fence handle
 * @param D      Data for this wait event type
 */
#define MUSASRV_HWPERF_SYNC_FENCE_WAIT(I, T, PID, F, D) \
do { \
	if (MUSAHWPerfHostIsEventEnabled((I), MUSA_HWPERF_HOST_SYNC_FENCE_WAIT)) \
	{ \
		MUSAHWPerfHostPostFenceWait(I, MUSA_HWPERF_HOST_SYNC_FENCE_WAIT_TYPE_##T, \
		                           (PID), (F), (D)); \
	} \
} while (0)

/**
 * @param I      Device info pointer
 * @param PID    Process ID that the following timeline belongs to
 * @param F      SW-timeline handle
 * @param SPI    Sync-pt index where this SW-timeline has reached
 */
#define MUSASRV_HWPERF_SYNC_SW_TL_ADV(I, PID, SW_TL, SPI)\
do { \
	if (MUSAHWPerfHostIsEventEnabled((I), MUSA_HWPERF_HOST_SYNC_SW_TL_ADVANCE)) \
	{ \
		MUSAHWPerfHostPostSWTimelineAdv((I), (PID), (SW_TL), (SPI)); \
	} \
} while (0)

/**
 * @param D      Device Node pointer
 * @param PID    Process ID that the following timeline belongs to
 * @param N      Null terminated string containing the process name
 */
#define MUSASRV_HWPERF_HOST_CLIENT_INFO_PROCESS_NAME(D, PID, N) \
do { \
	if (MUSAHWPerfHostIsEventEnabled(_MUSA_DEVICE_INFO_FROM_NODE(D), MUSA_HWPERF_HOST_CLIENT_INFO)) \
	{ \
		MUSAHWPerfHostPostClientInfoProcName(_MUSA_DEVICE_INFO_FROM_NODE(D), (PID), (N)); \
	} \
} while (0)

/**
 * @param I      Device info pointer
 * @param S      Start Index of the sTimeCorr array
 * @param C      Time correlation info count
 */
#define MUSASRV_HWPERF_HOST_TIME_CORRELATION(I, S, C) \
do { \
	if (MUSAHWPerfHostIsEventEnabled((I), MUSA_HWPERF_HOST_TIME_CORRELATION)) \
	{ \
		MUSAHWPerfHostPostTimeCorrelation((I), (S), (C)); \
	} \
} while (0)

/**
 * @param I      Device info pointer
 * @param EV     DMA event type
 * @param PID    Process ID
 * @param TID    Thread ID
 * @param SW_TL  SW-timeline handle
 * @param E      External job ref id
 * @param S2D    Copy direction
 * @param SADDR  SW address
 * @param DADDR  Device address
 * @param SIZE   Copy size
 */
#define MUSASRV_HWPERF_HOST_DMA_USER(I, EV, PID, TID, SW_TL, E, S2D, SADDR, DADDR, SIZE) \
do { \
	if (MUSAHWPerfHostIsEventEnabled((I), MUSA_HWPERF_HOST_DMA_##EV)) \
	{ \
		MUSA_HWPERF_HOST_DMA_DETAIL uDetail; \
		uDetail.sUserData.bS2D = (S2D); \
		uDetail.sUserData.ui64Addr = (SADDR); \
		uDetail.sUserData.ui64DevAddr = (DADDR); \
		MUSAHWPerfHostPostDMAEvent((I), MUSA_HWPERF_HOST_DMA_##EV, MUSA_HWPERF_HOST_DMA_TYPE_USER, \
					  &uDetail, (PID), (TID), (SIZE), (SW_TL), (E)); \
	} \
} while (0)

/**
 * @param I      Device info pointer
 * @param EV     DMA event type
 * @param PID    Process ID
 * @param TID    Thread ID
 * @param SW_TL  SW-timeline handle
 * @param E      External job ref id
 * @param P2L    Copy direction
 * @param PDEV   Peer device id
 * @param LADDR  Local device address
 * @param PADDR  Peer device address
 * @param SIZE   Copy size
 */
#define MUSASRV_HWPERF_HOST_DMA_P2P(I, EV, PID, TID, SW_TL, E, P2L, PDEV, LADDR, PADDR, SIZE) \
do { \
	if (MUSAHWPerfHostIsEventEnabled((I), MUSA_HWPERF_HOST_DMA_##EV)) \
	{ \
		MUSA_HWPERF_HOST_DMA_DETAIL uDetail; \
		uDetail.sP2PData.bPeer2Local = (P2L); \
		uDetail.sP2PData.i32PeerDevID = (PDEV); \
		uDetail.sP2PData.ui64LocalDevAddr = (LADDR); \
		uDetail.sP2PData.ui64PeerDevAddr = (PADDR); \
		MUSAHWPerfHostPostDMAEvent((I), MUSA_HWPERF_HOST_DMA_##EV, MUSA_HWPERF_HOST_DMA_TYPE_P2P, \
					  &uDetail, (PID), (TID), (SIZE), (SW_TL), (E)); \
	} \
} while (0)

/**
 * @param I      Device info pointer
 * @param N      Tag name
 */
#define MUSASRV_HWPERF_HOST_DEBUG_TAG_START(I, N, ...) \
do { \
	if (MUSAHWPerfHostIsEventEnabled((I), MUSA_HWPERF_HOST_DEBUG_START)) \
	{ \
		MT_UINT64 aui64Values[] = { __VA_ARGS__ }; \
		MT_UINT32 ui32Count = ARRAY_SIZE(aui64Values); \
		MUSAHWPerfHostPostDebugEvent((I), MUSA_HWPERF_HOST_DEBUG_START, \
			aui64Values, ui32Count, (N)); \
	} \
} while (0)

/**
 * @param I      Device info pointer
 * @param N      Tag name
 */
#define MUSASRV_HWPERF_HOST_DEBUG_TAG_END(I, N, ...) \
do { \
	if (MUSAHWPerfHostIsEventEnabled((I), MUSA_HWPERF_HOST_DEBUG_END)) \
	{ \
		MT_UINT64 aui64Values[] = { __VA_ARGS__ }; \
		MT_UINT32 ui32Count = ARRAY_SIZE(aui64Values); \
		MUSAHWPerfHostPostDebugEvent((I), MUSA_HWPERF_HOST_DEBUG_END, \
			aui64Values, ui32Count, (N)); \
	} \
} while (0)

/**
 * @param I      Device info pointer
 */
#define MUSASRV_HWPERF_HOST_FUNCTION_START(I, ...) \
		MUSASRV_HWPERF_HOST_DEBUG_TAG_START((I), __FUNCTION__, __VA_ARGS__)

/**
 * @param I      Device info pointer
 */
#define MUSASRV_HWPERF_HOST_FUNCTION_END(I, ...) \
		MUSASRV_HWPERF_HOST_DEBUG_TAG_END((I), __FUNCTION__, __VA_ARGS__)

#else

#define MUSASRV_HWPERF_ENQ(C, P, X, E, I, K, CF, UF, UT, CHKUID, UPDUID, D, CE)
#define MUSASRV_HWPERF_NOTIFY(C, P, X, K)
#define MUSASRV_HWPERF_UFO(I, T, D, S)
#define MUSASRV_HWPERF_ALLOC(D, T, FWADDR, N, Z)
#define MUSASRV_HWPERF_ALLOC_FENCE(D, PID, FENCE, FWADDR, N, Z)
#define MUSASRV_HWPERF_ALLOC_SYNC_CP(D, TL, PID, FENCE, FWADDR, N, Z)
#define MUSASRV_HWPERF_ALLOC_SW_FENCE(D, PID, SW_FENCE, SW_TL, SPI, N, Z)
#define MUSASRV_HWPERF_FREE(D, T, FWADDR)
#define MUSASRV_HWPERF_FREE_FENCE_SYNC(D, T, UID, PID, FWADDR)
#define MUSASRV_HWPERF_MODIFY_FENCE_SYNC(D, T, NEWUID, UID1, UID2, N, Z)
#define MUSASRV_HWPERF_CLK_SYNC(I)
#define MUSASRV_HWPERF_DEVICE_INFO(I, T, H, R)
#define MUSASRV_HWPERF_HOST_INFO(I, T)
#define MUSASRV_HWPERF_SYNC_FENCE_WAIT(I, T, PID, F, D)
#define MUSASRV_HWPERF_SYNC_SW_TL_ADV(I, PID, SW_TL, SPI)
#define MUSASRV_HWPERF_HOST_CLIENT_INFO_PROCESS_NAME(D, PID, N)
#define MUSASRV_HWPERF_HOST_TIME_CORRELATION(I, S, C)
#define MUSASRV_HWPERF_HOST_DMA_USER(I, EV, PID, TID, SW_TL, E, S2D, SADDR, DADDR)
#define MUSASRV_HWPERF_HOST_DMA_P2P(I, EV, PID, TID, SW_TL, E, P2L, PDEV, LADDR, PADDR)
#define MUSASRV_HWPERF_HOST_DEBUG_TAG_START(I, N, ...)
#define MUSASRV_HWPERF_HOST_DEBUG_TAG_END(I, N, ...)
#define MUSASRV_HWPERF_HOST_FUNCTION_START(I, ...)
#define MUSASRV_HWPERF_HOST_FUNCTION_END(I, ...)

#endif

#endif /* MUSAHWPERF_COMMON_H_ */

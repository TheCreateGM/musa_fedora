/*************************************************************************/ /*!
@File           mt_gputrace.c
@Title          MTGPU GPU Trace module Linux implementation
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
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

#include <linux/version.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 2, 0))
#include <linux/trace_events.h>
#else
#include <linux/ftrace_event.h>
#endif

#include "mtgpu_error.h"
#include "mtgpu_apphint.h"
#include "mt_debug.h"
#include "osmt_gputrace.h"
#include "musahwperf.h"
#include "device.h"
#include "mt.h"
#include "mtgpu_tlstreams.h"
#include "tlclient.h"
#include "mt_debug.h"

/******************************************************************************
 Module internal implementation
******************************************************************************/

typedef enum {
	MT_GPUTRACE_SWITCH_TYPE_UNDEF = 0,

	MT_GPUTRACE_SWITCH_TYPE_BEGIN = 1,
	MT_GPUTRACE_SWITCH_TYPE_END = 2,
	MT_GPUTRACE_SWITCH_TYPE_SINGLE = 3
} MT_GPUTRACE_SWITCH_TYPE;

typedef struct MUSA_HWPERF_FTRACE_DATA {
	/* This lock ensures the HWPerf TL stream reading resources are not destroyed
	 * by one thread disabling it while another is reading from it. Keeps the
	 * state and resource create/destroy atomic and consistent. */
	POS_LOCK    hFTraceResourceLock;

	MT_HANDLE  hGPUTraceCmdCompleteHandle;
	MT_HANDLE  hGPUTraceTLStream;
	MT_UINT64  ui64LastSampledTimeCorrOSTimeStamp;
	MT_UINT32  ui32FTraceLastOrdinal;
} MUSA_HWPERF_FTRACE_DATA;

/* This lock ensures state change of GPU_TRACING on/off is done atomically */
static POS_LOCK ghGPUTraceStateLock;
static MT_BOOL gbFTraceGPUEventsEnabled = MTGPU_APPHINT_ENABLEFTRACEGPU;

/* This lock ensures that the reference counting operation on the FTrace UFO
 * events and enable/disable operation on firmware event are performed as
 * one atomic operation. This should ensure that there are no race conditions
 * between reference counting and firmware event state change.
 * See below comment for guiUfoEventRef.
 */
static POS_LOCK ghLockFTraceEventLock;

/* Multiple FTrace UFO events are reflected in the firmware as only one event. When
 * we enable FTrace UFO event we want to also at the same time enable it in
 * the firmware. Since there is a multiple-to-one relation between those events
 * we count how many FTrace UFO events is enabled. If at least one event is
 * enabled we enabled the firmware event. When all FTrace UFO events are disabled
 * we disable firmware event. */
static MT_UINT guiUfoEventRef;

/******************************************************************************
 Module In-bound API
******************************************************************************/

static MTGPU_ERROR _GpuTraceDisable(
	MTGPU_MUSADEV_INFO *psMusaDevInfo,
	MT_BOOL bDeInit);

static void _GpuTraceCmdCompleteNotify(MTGPU_CMDCOMP_HANDLE);

MTGPU_ERROR MTGpuTraceSupportInit(void)
{
	MTGPU_ERROR eError;

	if (ghLockFTraceEventLock != NULL)
	{
		MT_DPF((MT_DBG_ERROR, "FTrace Support is already initialized"));
		return MTGPU_OK;
	}

	/* common module params initialization */
	eError = OSLockCreate(&ghLockFTraceEventLock);
	MT_LOG_RETURN_IF_ERROR(eError, "OSLockCreate");

	eError = OSLockCreate(&ghGPUTraceStateLock);
	MT_LOG_RETURN_IF_ERROR (eError, "OSLockCreate");

	return MTGPU_OK;
}

void MTGpuTraceSupportDeInit(void)
{
	if (ghGPUTraceStateLock)
	{
		OSLockDestroy(ghGPUTraceStateLock);
	}

	if (ghLockFTraceEventLock)
	{
		OSLockDestroy(ghLockFTraceEventLock);
		ghLockFTraceEventLock = NULL;
	}
}

MTGPU_ERROR MTGpuTraceInitDevice(MTGPU_DEVICE_NODE *psDeviceNode)
{
	MTGPU_ERROR eError;
	MUSA_HWPERF_FTRACE_DATA *psData;
	MTGPU_MUSADEV_INFO *psDevInfo = psDeviceNode->pvDevice;

	MTGPU_VZ_RET_IF_MODE(GUEST, MTGPU_OK);

	psData = OSAllocZMem(sizeof(MUSA_HWPERF_FTRACE_DATA));
	psDevInfo->pvGpuFtraceData = psData;
	MT_LOG_GOTO_IF_NOMEM(psData, eError, e0);

	/* We initialise it only once because we want to track if any
	 * packets were dropped. */
	psData->ui32FTraceLastOrdinal = MT_UINT32_MAX - 1;

	eError = OSLockCreate(&psData->hFTraceResourceLock);
	MT_LOG_GOTO_IF_ERROR(eError, "OSLockCreate", e0);

	return MTGPU_OK;

e0:
	MTGpuTraceDeInitDevice(psDeviceNode);
	return eError;
}

void MTGpuTraceDeInitDevice(MTGPU_DEVICE_NODE *psDeviceNode)
{
	MTGPU_MUSADEV_INFO *psDevInfo = psDeviceNode->pvDevice;
	MUSA_HWPERF_FTRACE_DATA *psData = psDevInfo->pvGpuFtraceData;

	MTGPU_VZ_RETN_IF_MODE(GUEST);
	if (psData)
	{
		/* first disable the tracing, to free up TL resources */
		if (psData->hFTraceResourceLock)
		{
			OSLockAcquire(psData->hFTraceResourceLock);
			_GpuTraceDisable(psDeviceNode->pvDevice, MT_TRUE);
			OSLockRelease(psData->hFTraceResourceLock);

			/* now free all the FTrace resources */
			OSLockDestroy(psData->hFTraceResourceLock);
		}
		OSFreeMem(psData);
		psDevInfo->pvGpuFtraceData = NULL;
	}
}

MT_BOOL MTGpuTraceIsEnabled(void)
{
	return gbFTraceGPUEventsEnabled;
}

void MTGpuTraceInitIfEnabled(MTGPU_DEVICE_NODE *psDeviceNode)
{
	if (MTGpuTraceIsEnabled())
	{
		MTGPU_ERROR eError = MTGpuTraceSetEnabled(psDeviceNode, MT_TRUE);
		if (eError != MTGPU_OK)
		{
			MT_DPF((MT_DBG_ERROR, "Failed to initialise GPU event tracing"
					" (%s)", MTGPUGetErrorString(eError)));
		}

		/* below functions will enable FTrace events which in turn will
		 * execute HWPerf callbacks that set appropriate filter values
		 * note: unfortunately the functions don't allow to pass private
		 *       data so they enable events for all of the devices
		 *       at once, which means that this can happen more than once
		 *       if there is more than one device */

		/* single events can be enabled by calling trace_set_clr_event()
		 * with the event name, e.g.:
		 * trace_set_clr_event("mtgpu", "mt_ufo_update", 1) */
#if defined(CONFIG_EVENT_TRACING) /* this is a kernel config option */
#if defined(ANDROID) || defined(CHROMIUMOS_KERNEL)
		if (trace_set_clr_event("gpu", NULL, 1))
		{
			MT_DPF((MT_DBG_ERROR, "Failed to enable \"gpu\" event"
					" group"));
		}
		else
		{
			MT_LOG(("FTrace events from \"gpu\" group enabled"));
		}
#endif /* defined(ANDROID) || defined(CHROMIUMOS_KERNEL) */
		if (trace_set_clr_event("mtgpu", NULL, 1))
		{
			MT_DPF((MT_DBG_ERROR, "Failed to enable \"mtgpu\" event"
					" group"));
		}
		else
		{
			MT_LOG(("FTrace events from \"mtgpu\" group enabled"));
		}
#endif /* defined(CONFIG_EVENT_TRACING) */
	}
}

/* Caller must now hold hFTraceResourceLock before calling this method.
 */
static MTGPU_ERROR _GpuTraceEnable(MTGPU_MUSADEV_INFO *psMusaDevInfo)
{
	MTGPU_ERROR eError = MTGPU_OK;
	MUSA_HWPERF_FTRACE_DATA *psFtraceData;
	MTGPU_DEVICE_NODE *psMusaDevNode = psMusaDevInfo->psDeviceNode;
	MT_CHAR pszHWPerfStreamName[sizeof(MTGPU_TL_HWPERF_MUSA_FW_STREAM) + 5];

	MT_DPF_ENTERED;

	MT_ASSERT(psMusaDevInfo);

	psFtraceData = psMusaDevInfo->pvGpuFtraceData;

	MT_ASSERT(OSLockIsLocked(psFtraceData->hFTraceResourceLock));

	/* return if already enabled */
	if (psFtraceData->hGPUTraceTLStream)
	{
		return MTGPU_OK;
	}

#if defined(SUPPORT_MUSA)
	/* Signal FW to enable event generation */
	if (psMusaDevInfo->bFirmwareInitialised)
	{
		MT_UINT64 ui64UFOFilter = psMusaDevInfo->ui64HWPerfFilter &
		        (MUSA_HWPERF_EVENT_MASK_FW_SED | MUSA_HWPERF_EVENT_MASK_FW_UFO);

		/* Do not call into MTGPUMUSACtrlHWPerfKM if we're in GUEST mode. */
		if (MTGPU_VZ_MODE_IS(GUEST))
		{
			eError = MTGPU_OK;
		}
		else
		{
			eError = MTGPUMUSACtrlHWPerfKM(NULL, psMusaDevNode,
			                               MUSA_HWPERF_STREAM_ID0_FW, MT_FALSE,
			                               MUSA_HWPERF_EVENT_MASK_HW_KICKFINISH |
			                               ui64UFOFilter);
		}
		MT_LOG_GOTO_IF_ERROR(eError, "MTGPUMUSACtrlHWPerfKM", err_out);
	}
	else
#endif
	{
		/* only set filter and exit */
		psMusaDevInfo->ui64HWPerfFilter = MUSA_HWPERF_EVENT_MASK_HW_KICKFINISH |
		        ((MUSA_HWPERF_EVENT_MASK_FW_SED | MUSA_HWPERF_EVENT_MASK_FW_UFO) &
		        psMusaDevInfo->ui64HWPerfFilter);

		MT_DPF((MT_DBG_WARNING,
				 "HWPerfFW mask has been SET to (%" MT_UINT64_FMTSPECx ")",
				 psMusaDevInfo->ui64HWPerfFilter));

		return MTGPU_OK;
	}

	/* form the HWPerf stream name, corresponding to this DevNode; which can make sense in the UM */
	if (OSSNPrintf(pszHWPerfStreamName, sizeof(pszHWPerfStreamName), "%s%d",
					MTGPU_TL_HWPERF_MUSA_FW_STREAM, psMusaDevNode->sDevId.i32OsDeviceID) < 0)
	{
		MT_DPF((MT_DBG_ERROR,
		         "%s: Failed to form HWPerf stream name for device %d",
		         __func__,
		         psMusaDevNode->sDevId.i32OsDeviceID));
		return MTGPU_ERROR_INVALID_PARAMS;
	}

	/* Open the TL Stream for HWPerf data consumption */
	eError = TLClientOpenStream(DIRECT_BRIDGE_HANDLE,
								pszHWPerfStreamName,
								MTGPU_STREAM_FLAG_ACQUIRE_NONBLOCKING,
								&psFtraceData->hGPUTraceTLStream);
	MT_LOG_GOTO_IF_ERROR(eError, "TLClientOpenStream", err_out);

/* Do not change to sched_clock, because user mode can not change to sched_clock.
 * Instead, we can simply change ftrace clock to mono by "echo mono >> trace_clock".
 */
#if defined(SUPPORT_MUSA) && defined(USE_SCHED_CLOCK)
	if (MUSATimeCorrGetClockSource(psMusaDevNode) != MUSATIMECORR_CLOCK_SCHED)
	{
		/* Set clock source for timer correlation data to sched_clock */
		psMusaDevInfo->ui32LastClockSource = MUSATimeCorrGetClockSource(psMusaDevNode);
		MUSATimeCorrSetClockSource(psMusaDevNode, MUSATIMECORR_CLOCK_SCHED);
	}
#endif

	/* Reset the OS timestamp coming from the timer correlation data
	 * associated with the latest HWPerf event we processed.
	 */
	psFtraceData->ui64LastSampledTimeCorrOSTimeStamp = 0;

	/* Register a notifier to collect HWPerf data whenever the HW completes
	 * an operation.
	 */
	eError = MTGPURegisterCmdCompleteNotify(
		&psFtraceData->hGPUTraceCmdCompleteHandle,
		&_GpuTraceCmdCompleteNotify,
		psMusaDevInfo);
	MT_LOG_GOTO_IF_ERROR(eError, "MTGPURegisterCmdCompleteNotify", err_close_stream);

err_out:
	MT_DPF_RETURN_RC(eError);

err_close_stream:
	TLClientCloseStream(DIRECT_BRIDGE_HANDLE,
						psFtraceData->hGPUTraceTLStream);
	psFtraceData->hGPUTraceTLStream = NULL;
	goto err_out;
}

/* Caller must now hold hFTraceResourceLock before calling this method.
 */
static MTGPU_ERROR _GpuTraceDisable(MTGPU_MUSADEV_INFO *psMusaDevInfo, MT_BOOL bDeInit)
{
	MTGPU_ERROR eError = MTGPU_OK;
	MUSA_HWPERF_FTRACE_DATA *psFtraceData;
#if defined(SUPPORT_MUSA)
	MTGPU_DEVICE_NODE *psMusaDevNode = psMusaDevInfo->psDeviceNode;
#endif

	MT_DPF_ENTERED;

	MT_ASSERT(psMusaDevInfo);

	psFtraceData = psMusaDevInfo->pvGpuFtraceData;

	MT_ASSERT(OSLockIsLocked(psFtraceData->hFTraceResourceLock));

	/* if FW is not yet initialised, just set filter and exit */
	if (!psMusaDevInfo->bFirmwareInitialised)
	{
		psMusaDevInfo->ui64HWPerfFilter = MUSA_HWPERF_EVENT_MASK_NONE;
#if !defined(NO_HARDWARE)
		MT_DPF((MT_DBG_WARNING,
		         "HWPerfFW mask has been SET to (%" MT_UINT64_FMTSPECx ")",
		         psMusaDevInfo->ui64HWPerfFilter));
#endif
		return MTGPU_OK;
	}

	if (NULL == psFtraceData->hGPUTraceTLStream)
	{
		/* Tracing already disabled, just return */
		return MTGPU_OK;
	}

#if defined(SUPPORT_MUSA)
	if (!bDeInit)
	{
		/* Do not call into MTGPUMUSACtrlHWPerfKM if we are in GUEST mode. */
		if (MTGPU_VZ_MODE_IS(GUEST))
		{
			eError = MTGPU_OK;
		}
		else
		{
			eError = MTGPUMUSACtrlHWPerfKM(NULL, psMusaDevNode,
			                               MUSA_HWPERF_STREAM_ID0_FW, MT_FALSE,
			                               (MUSA_HWPERF_EVENT_MASK_NONE));
		}
		MT_LOG_IF_ERROR(eError, "MTGPUMUSACtrlHWPerfKM");
	}
#endif

	if (psFtraceData->hGPUTraceCmdCompleteHandle)
	{
		/* Tracing is being turned off. Unregister the notifier. */
		eError = MTGPUUnregisterCmdCompleteNotify(
				psFtraceData->hGPUTraceCmdCompleteHandle);
		MT_LOG_IF_ERROR(eError, "MTGPUUnregisterCmdCompleteNotify");
		psFtraceData->hGPUTraceCmdCompleteHandle = NULL;
	}

	if (psFtraceData->hGPUTraceTLStream)
	{
		MT_PBYTE pbTmp = NULL;
		MT_UINT32 ui32Tmp = 0;

		/* We have to flush both the L1 (FW) and L2 (Host) buffers in case there
		 * are some events left unprocessed in this FTrace/systrace "session"
		 * (note that even if we have just disabled HWPerf on the FW some packets
		 * could have been generated and already copied to L2 by the MISR handler).
		 *
		 * With the following calls we will both copy new data to the Host buffer
		 * (done by the producer callback in TLClientAcquireData) and advance
		 * the read offset in the buffer to catch up with the latest events.
		 */
		eError = TLClientAcquireData(DIRECT_BRIDGE_HANDLE,
		                             psFtraceData->hGPUTraceTLStream,
		                             &pbTmp, &ui32Tmp);
		MT_LOG_IF_ERROR(eError, "TLClientCloseStream");

		/* Let close stream perform the release data on the outstanding acquired data */
		eError = TLClientCloseStream(DIRECT_BRIDGE_HANDLE,
		                             psFtraceData->hGPUTraceTLStream);
		MT_LOG_IF_ERROR(eError, "TLClientCloseStream");

		psFtraceData->hGPUTraceTLStream = NULL;
	}

#if defined(SUPPORT_MUSA) && defined(USE_SCHED_CLOCK)
	if (psMusaDevInfo->ui32LastClockSource != MUSATIMECORR_CLOCK_SCHED)
	{
		MUSATimeCorrSetClockSource(psMusaDevNode, psMusaDevInfo->ui32LastClockSource);
	}
#endif

	MT_DPF_RETURN_RC(eError);
}

static MTGPU_ERROR _GpuTraceSetEnabled(MTGPU_MUSADEV_INFO *psMusaDevInfo,
                                        MT_BOOL bNewValue)
{
	MTGPU_ERROR eError = MTGPU_OK;
	MUSA_HWPERF_FTRACE_DATA *psFtraceData;

	MTGPU_VZ_RET_IF_MODE(GUEST, MTGPU_ERROR_NOT_IMPLEMENTED);

	MT_DPF_ENTERED;

	MT_ASSERT(psMusaDevInfo);
	psFtraceData = psMusaDevInfo->pvGpuFtraceData;

	/* About to create/destroy FTrace resources, lock critical section
	 * to avoid HWPerf MISR thread contention.
	 */
	OSLockAcquire(psFtraceData->hFTraceResourceLock);

	eError = (bNewValue ? _GpuTraceEnable(psMusaDevInfo)
					   : _GpuTraceDisable(psMusaDevInfo, MT_FALSE));

	OSLockRelease(psFtraceData->hFTraceResourceLock);

	MT_DPF_RETURN_RC(eError);
}

static MTGPU_ERROR _GpuTraceSetEnabledForAllDevices(MT_BOOL bNewValue)
{
	MTGPU_ERROR eError = MTGPU_OK;
	MTGPU_DATA *psMTGPUData = MTGPUGetMTGPUData();
	MTGPU_DEVICE_NODE *psDeviceNode;

	OSWRLockAcquireRead(psMTGPUData->hDeviceNodeListLock);
	psDeviceNode = psMTGPUData->psDeviceNodeList;

	/* enable/disable GPU trace on all devices */
	while (psDeviceNode)
	{
		eError = _GpuTraceSetEnabled(psDeviceNode->pvDevice, bNewValue);
		if (eError != MTGPU_OK)
		{
			break;
		}
		psDeviceNode = psDeviceNode->psNext;
	}

	OSWRLockReleaseRead(psMTGPUData->hDeviceNodeListLock);

	MT_DPF_RETURN_RC(eError);
}

MTGPU_ERROR MTGpuTraceSetEnabled(MTGPU_DEVICE_NODE *psDeviceNode,
                                   MT_BOOL bNewValue)
{
	return _GpuTraceSetEnabled(psDeviceNode->pvDevice, bNewValue);
}

/* ----- HWPerf to FTrace packet processing and events injection ------------ */
void MTGpuTraceEnqueueEvent(
		MTGPU_DEVICE_NODE *psDevNode,
		MT_UINT32 ui32FirmwareCtx,
		MT_UINT32 ui32ExtJobRef,
		MT_UINT32 ui32IntJobRef,
		MUSA_HWPERF_KICK_TYPE eKickType)
{
}

static void _GpuTraceWorkSwitch(
		MT_UINT64 ui64HWTimestampInOSTime,
		MT_UINT32 ui32CtxId,
		MT_UINT32 ui32CtxPriority,
		MT_UINT32 ui32ExtJobRef,
		MT_UINT32 ui32IntJobRef,
		const MT_CHAR* pszWorkType,
		MT_GPUTRACE_SWITCH_TYPE eSwType)
{
	MT_ASSERT(pszWorkType);
}

static void _GpuTraceUfo(
		MT_UINT64 ui64OSTimestamp,
		const MUSA_HWPERF_UFO_EV eEvType,
		const MT_UINT32 ui32CtxId,
		const MT_UINT32 ui32ExtJobRef,
		const MT_UINT32 ui32IntJobRef,
		const MT_UINT32 ui32UFOCount,
		const MUSA_HWPERF_UFO_DATA_ELEMENT *puData)
{
}

static void _GpuTraceFirmware(
		MT_UINT64 ui64HWTimestampInOSTime,
		const MT_CHAR* pszWorkType,
		MT_GPUTRACE_SWITCH_TYPE eSwType)
{
}

static void _GpuTraceEventsLost(
		const MUSA_HWPERF_STREAM_ID eStreamId,
		const MT_UINT32 ui32LastOrdinal,
		const MT_UINT32 ui32CurrOrdinal)
{
}

/* Calculate the OS timestamp given an MUSA timestamp in the HWPerf event. */
static uint64_t CalculateEventTimestamp(
	MTGPU_MUSADEV_INFO *psDevInfo,
	uint32_t ui32TimeCorrIndex,
	uint64_t ui64EventTimestamp)
{
	MUSAFWIF_GPU_UTIL_FWCB *psGpuUtilFWCB = psDevInfo->psMUSAFWIfGpuUtilFWCb;
	MUSA_HWPERF_FTRACE_DATA *psFtraceData = psDevInfo->pvGpuFtraceData;
	MUSAFWIF_TIME_CORR *psTimeCorr = &psGpuUtilFWCB->sTimeCorr[ui32TimeCorrIndex];
	uint64_t ui64CRTimeStamp = psTimeCorr->ui64CRTimeStamp;
	uint64_t ui64OSTimeStamp = psTimeCorr->ui64OSTimeStamp;
	uint64_t ui64CRDeltaToOSDeltaKNs = psTimeCorr->ui64CRDeltaToOSDeltaKNs;
	uint64_t ui64EventOSTimestamp, deltaMusaTimer, delta_ns;

	if (psFtraceData->ui64LastSampledTimeCorrOSTimeStamp > ui64OSTimeStamp)
	{
		/* The previous packet had a time reference (time correlation data) more
		 * recent than the one in the current packet, it means the timer
		 * correlation array wrapped too quickly (buffer too small) and in the
		 * previous call to _GpuTraceUfoEvent we read one of the
		 * newest timer correlations rather than one of the oldest ones.
		 */
		MT_DPF((MT_DBG_ERROR, "%s: The timestamps computed so far could be "
				 "wrong! The time correlation array size should be increased "
				 "to avoid this.", __func__));
	}

	psFtraceData->ui64LastSampledTimeCorrOSTimeStamp = ui64OSTimeStamp;

	/* MUSA CR timer ticks delta */
	deltaMusaTimer = ui64EventTimestamp - ui64CRTimeStamp;
	/* MUSA time delta in nanoseconds */
	delta_ns = MUSAFWIF_GET_DELTA_OSTIME_NS(deltaMusaTimer, ui64CRDeltaToOSDeltaKNs);
	/* Calculate OS time of HWPerf event */
	ui64EventOSTimestamp = ui64OSTimeStamp + delta_ns;

	MT_DPF((MT_DBG_VERBOSE, "%s: psCurrentDvfs MUSA %llu, OS %llu, DVFSCLK %u",
			 __func__, ui64CRTimeStamp, ui64OSTimeStamp,
			 psTimeCorr->ui32CoreClockSpeed));

	return ui64EventOSTimestamp;
}

static void _GpuTraceSwitchEvent(MTGPU_MUSADEV_INFO *psDevInfo,
		MUSA_HWPERF_V2_PACKET_HDR* psHWPerfPkt, const MT_CHAR* pszWorkName,
		MT_GPUTRACE_SWITCH_TYPE eSwType)
{
	MT_UINT64 ui64Timestamp;
	MUSA_HWPERF_HW_DATA* psHWPerfPktData;

	MT_DPF_ENTERED;

	MT_ASSERT(psHWPerfPkt);
	MT_ASSERT(pszWorkName);

	psHWPerfPktData = MUSA_HWPERF_GET_PACKET_DATA_BYTES(psHWPerfPkt);

	ui64Timestamp = CalculateEventTimestamp(psDevInfo, psHWPerfPktData->ui32TimeCorrIndex,
											psHWPerfPkt->ui64Timestamp);

	MT_DPF((MT_DBG_VERBOSE, "_GpuTraceSwitchEvent: %s ui32ExtJobRef=%d, ui32IntJobRef=%d, eSwType=%d",
			pszWorkName, psHWPerfPktData->ui32DMContext, psHWPerfPktData->ui32IntJobRef, eSwType));

	_GpuTraceWorkSwitch(ui64Timestamp,
	                    psHWPerfPktData->ui32DMContext,
	                    psHWPerfPktData->ui32CtxPriority,
	                    psHWPerfPktData->ui32ExtJobRef,
	                    psHWPerfPktData->ui32IntJobRef,
	                    pszWorkName,
	                    eSwType);

	MT_DPF_RETURN;
}

static void _GpuTraceUfoEvent(MTGPU_MUSADEV_INFO *psDevInfo,
                              MUSA_HWPERF_V2_PACKET_HDR* psHWPerfPkt)
{
	MT_UINT64 ui64Timestamp;
	MUSA_HWPERF_UFO_DATA *psHWPerfPktData;
	MT_UINT32 ui32UFOCount;
	MUSA_HWPERF_UFO_DATA_ELEMENT *puData;

	psHWPerfPktData = MUSA_HWPERF_GET_PACKET_DATA_BYTES(psHWPerfPkt);

	ui32UFOCount = MUSA_HWPERF_GET_UFO_STREAMSIZE(psHWPerfPktData->ui32StreamInfo);
	puData = (MUSA_HWPERF_UFO_DATA_ELEMENT *) MT_OFFSET_ADDR(psHWPerfPktData, MUSA_HWPERF_GET_UFO_STREAMOFFSET(psHWPerfPktData->ui32StreamInfo));

	ui64Timestamp = CalculateEventTimestamp(psDevInfo, psHWPerfPktData->ui32TimeCorrIndex,
											psHWPerfPkt->ui64Timestamp);

	MT_DPF((MT_DBG_VERBOSE, "_GpuTraceUfoEvent: ui32ExtJobRef=%d, "
	        "ui32IntJobRef=%d", psHWPerfPktData->ui32ExtJobRef,
	        psHWPerfPktData->ui32IntJobRef));

	_GpuTraceUfo(ui64Timestamp, psHWPerfPktData->eEvType,
	             psHWPerfPktData->ui32DMContext, psHWPerfPktData->ui32ExtJobRef,
	             psHWPerfPktData->ui32IntJobRef, ui32UFOCount, puData);
}

static void _GpuTraceFirmwareEvent(MTGPU_MUSADEV_INFO *psDevInfo,
		MUSA_HWPERF_V2_PACKET_HDR* psHWPerfPkt, const MT_CHAR* pszWorkName,
		MT_GPUTRACE_SWITCH_TYPE eSwType)

{
	uint64_t ui64Timestamp;
	MUSA_HWPERF_FW_DATA *psHWPerfPktData = MUSA_HWPERF_GET_PACKET_DATA_BYTES(psHWPerfPkt);

	ui64Timestamp = CalculateEventTimestamp(psDevInfo, psHWPerfPktData->ui32TimeCorrIndex,
											psHWPerfPkt->ui64Timestamp);

	_GpuTraceFirmware(ui64Timestamp, pszWorkName, eSwType);
}

static MT_BOOL ValidAndEmitFTraceEvent(MTGPU_MUSADEV_INFO *psDevInfo,
		MUSA_HWPERF_V2_PACKET_HDR* psHWPerfPkt)
{
	MUSA_HWPERF_EVENT_TYPE eType;
	MUSA_HWPERF_FTRACE_DATA *psFtraceData = psDevInfo->pvGpuFtraceData;
	MT_UINT32 ui32HwEventTypeIndex;
	static const struct {
		MT_CHAR* pszName;
		MT_GPUTRACE_SWITCH_TYPE eSwType;
	} aszHwEventTypeMap[] = {
#define _T(T) MT_GPUTRACE_SWITCH_TYPE_##T
		{ "BG",             _T(BEGIN)  }, /* MUSA_HWPERF_FW_BGSTART */
		{ "BG",             _T(END)    }, /* MUSA_HWPERF_FW_BGEND */
		{ "IRQ",            _T(BEGIN)  }, /* MUSA_HWPERF_FW_IRQSTART */
		{ "IRQ",            _T(END)    }, /* MUSA_HWPERF_FW_IRQEND */
		{ "DBG",            _T(BEGIN)  }, /* MUSA_HWPERF_FW_DBGSTART */
		{ "DBG",            _T(END)    }, /* MUSA_HWPERF_FW_DBGEND */
		{ "PMOOM_TAPAUSE",  _T(END)    }, /* MUSA_HWPERF_HW_PMOOM_TAPAUSE */
		{ "TA",             _T(BEGIN)  }, /* MUSA_HWPERF_HW_TAKICK */
		{ "TA",             _T(END)    }, /* MUSA_HWPERF_HW_TAFINISHED */
		{ "TQ3D",           _T(BEGIN)  }, /* MUSA_HWPERF_HW_3DTQKICK */
		{ "3D",             _T(BEGIN)  }, /* MUSA_HWPERF_HW_3DKICK */
		{ "3D",             _T(END)    }, /* MUSA_HWPERF_HW_3DFINISHED */
		{ "CMP",            _T(BEGIN)  }, /* MUSA_HWPERF_HW_CMPKICK */
		{ "CMP",            _T(END)    }, /* MUSA_HWPERF_HW_CMPFINISHED */
		{ "TQ2D",           _T(BEGIN)  }, /* MUSA_HWPERF_HW_TLAKICK */
		{ "TQ2D",           _T(END)    }, /* MUSA_HWPERF_HW_TLAFINISHED */
		{ "3DSPM",          _T(BEGIN)  }, /* MUSA_HWPERF_HW_3DSPMKICK */
		{ NULL,             0          }, /* MUSA_HWPERF_HW_PERIODIC (unsupported) */
		{ "RTU",            _T(BEGIN)  }, /* MUSA_HWPERF_HW_RTUKICK */
		{ "RTU",            _T(END)    }, /* MUSA_HWPERF_HW_RTUFINISHED */
		{ "SHG",            _T(BEGIN)  }, /* MUSA_HWPERF_HW_SHGKICK */
		{ "SHG",            _T(END)    }, /* MUSA_HWPERF_HW_SHGFINISHED */
		{ "TQ3D",           _T(END)    }, /* MUSA_HWPERF_HW_3DTQFINISHED */
		{ "3DSPM",          _T(END)    }, /* MUSA_HWPERF_HW_3DSPMFINISHED */
		{ "PMOOM_TARESUME", _T(BEGIN)  }, /* MUSA_HWPERF_HW_PMOOM_TARESUME */
		{ "TRANSFER",            _T(BEGIN)  }, /* MUSA_HWPERF_HW_TRANSFERKICK */
		{ "TRANSFER",            _T(END)    }, /* MUSA_HWPERF_HW_TRANSFERFINISHED */
		{ "NULL",           _T(SINGLE) }, /* MUSA_HWPERF_HW_NULLKICK */
#undef _T
	};
	static_assert(MUSA_HWPERF_HW_EVENT_RANGE0_FIRST_TYPE == MUSA_HWPERF_FW_EVENT_RANGE_LAST_TYPE + 1,
				  "FW and HW events are not contiguous in MUSA_HWPERF_EVENT_TYPE");

	MT_ASSERT(psHWPerfPkt);
	eType = MUSA_HWPERF_GET_TYPE(psHWPerfPkt);

	if (psFtraceData->ui32FTraceLastOrdinal != psHWPerfPkt->ui32Ordinal - 1)
	{
		MUSA_HWPERF_STREAM_ID eStreamId = MUSA_HWPERF_GET_STREAM_ID(psHWPerfPkt);
		_GpuTraceEventsLost(eStreamId,
		                    psFtraceData->ui32FTraceLastOrdinal,
		                    psHWPerfPkt->ui32Ordinal);
		MT_DPF((MT_DBG_ERROR, "FTrace events lost (stream_id = %u, ordinal: last = %u, current = %u)",
		         eStreamId, psFtraceData->ui32FTraceLastOrdinal, psHWPerfPkt->ui32Ordinal));
	}

	psFtraceData->ui32FTraceLastOrdinal = psHWPerfPkt->ui32Ordinal;

	/* Process UFO packets */
	if (eType == MUSA_HWPERF_UFO)
	{
		_GpuTraceUfoEvent(psDevInfo, psHWPerfPkt);
		return MT_TRUE;
	}

	if (eType <= MUSA_HWPERF_HW_EVENT_RANGE0_LAST_TYPE)
	{
		/* this ID belongs to range 0, so index directly in range 0 */
		ui32HwEventTypeIndex = eType - MUSA_HWPERF_FW_EVENT_RANGE_FIRST_TYPE;
	}
	else
	{
		/* this ID belongs to range 1, so first index in range 1 and skip number of slots used up for range 0 */
		ui32HwEventTypeIndex = (eType - MUSA_HWPERF_HW_EVENT_RANGE1_FIRST_TYPE) +
		                       (MUSA_HWPERF_HW_EVENT_RANGE0_LAST_TYPE - MUSA_HWPERF_FW_EVENT_RANGE_FIRST_TYPE + 1);
	}

	if (ui32HwEventTypeIndex >= ARRAY_SIZE(aszHwEventTypeMap))
		goto err_unsupported;

	if (aszHwEventTypeMap[ui32HwEventTypeIndex].pszName == NULL)
	{
		/* Not supported map entry, ignore event */
		goto err_unsupported;
	}

	if (HWPERF_PACKET_IS_HW_TYPE(eType))
	{
		if (aszHwEventTypeMap[ui32HwEventTypeIndex].eSwType == MT_GPUTRACE_SWITCH_TYPE_SINGLE)
		{
			_GpuTraceSwitchEvent(psDevInfo, psHWPerfPkt,
			                     aszHwEventTypeMap[ui32HwEventTypeIndex].pszName,
			                     MT_GPUTRACE_SWITCH_TYPE_BEGIN);
			_GpuTraceSwitchEvent(psDevInfo, psHWPerfPkt,
			                     aszHwEventTypeMap[ui32HwEventTypeIndex].pszName,
			                     MT_GPUTRACE_SWITCH_TYPE_END);
		}
		else
		{
			_GpuTraceSwitchEvent(psDevInfo, psHWPerfPkt,
			                     aszHwEventTypeMap[ui32HwEventTypeIndex].pszName,
			                     aszHwEventTypeMap[ui32HwEventTypeIndex].eSwType);
		}
	}
	else if (HWPERF_PACKET_IS_FW_TYPE(eType))
	{
		_GpuTraceFirmwareEvent(psDevInfo, psHWPerfPkt,
										aszHwEventTypeMap[ui32HwEventTypeIndex].pszName,
										aszHwEventTypeMap[ui32HwEventTypeIndex].eSwType);
	}
	else
	{
		goto err_unsupported;
	}

	return MT_TRUE;

err_unsupported:
	MT_DPF((MT_DBG_VERBOSE, "%s: Unsupported event type %d", __func__, eType));
	return MT_FALSE;
}


static void _GpuTraceProcessPackets(MTGPU_MUSADEV_INFO *psDevInfo,
		void *pBuffer, MT_UINT32 ui32ReadLen)
{
	MT_UINT32			ui32TlPackets = 0;
	MT_UINT32			ui32HWPerfPackets = 0;
	MT_UINT32			ui32HWPerfPacketsSent = 0;
	void				*pBufferEnd;
	MTGPUTL_PPACKETHDR psHDRptr;
	MTGPUTL_PACKETTYPE ui16TlType;

	MT_DPF_ENTERED;

	MT_ASSERT(psDevInfo);
	MT_ASSERT(pBuffer);
	MT_ASSERT(ui32ReadLen);

	/* Process the TL Packets
	 */
	pBufferEnd = MT_OFFSET_ADDR(pBuffer, ui32ReadLen);
	psHDRptr = GET_PACKET_HDR(pBuffer);
	while ( psHDRptr < (MTGPUTL_PPACKETHDR)pBufferEnd )
	{
		ui16TlType = GET_PACKET_TYPE(psHDRptr);
		if (ui16TlType == MTGPUTL_PACKETTYPE_DATA)
		{
			MT_UINT16 ui16DataLen = GET_PACKET_DATA_LEN(psHDRptr);
			if (0 == ui16DataLen)
			{
				MT_DPF((MT_DBG_ERROR, "_GpuTraceProcessPackets: ZERO Data in TL data packet: %p", psHDRptr));
			}
			else
			{
				MUSA_HWPERF_V2_PACKET_HDR* psHWPerfPkt;
				MUSA_HWPERF_V2_PACKET_HDR* psHWPerfEnd;

				/* Check for lost hwperf data packets */
				psHWPerfEnd = MUSA_HWPERF_GET_PACKET(GET_PACKET_DATA_PTR(psHDRptr)+ui16DataLen);
				psHWPerfPkt = MUSA_HWPERF_GET_PACKET(GET_PACKET_DATA_PTR(psHDRptr));
				do
				{
					if (ValidAndEmitFTraceEvent(psDevInfo, psHWPerfPkt))
					{
						ui32HWPerfPacketsSent++;
					}
					ui32HWPerfPackets++;
					psHWPerfPkt = MUSA_HWPERF_GET_NEXT_PACKET(psHWPerfPkt);
				}
				while (psHWPerfPkt < psHWPerfEnd);
			}
		}
		else if (ui16TlType == MTGPUTL_PACKETTYPE_MOST_RECENT_WRITE_FAILED)
		{
			MT_DPF((MT_DBG_MESSAGE, "_GpuTraceProcessPackets: Indication that the transport buffer was full"));
		}
		else
		{
			/* else Ignore padding packet type and others */
			MT_DPF((MT_DBG_MESSAGE, "_GpuTraceProcessPackets: Ignoring TL packet, type %d", ui16TlType ));
		}

		psHDRptr = GET_NEXT_PACKET_ADDR(psHDRptr);
		ui32TlPackets++;
	}

	MT_DPF((MT_DBG_VERBOSE, "_GpuTraceProcessPackets: TL "
			"Packets processed %03d, HWPerf packets %03d, sent %03d",
			ui32TlPackets, ui32HWPerfPackets, ui32HWPerfPacketsSent));

	MT_DPF_RETURN;
}


static void _GpuTraceCmdCompleteNotify(MTGPU_CMDCOMP_HANDLE hCmdCompHandle)
{
	MTGPU_MUSADEV_INFO* psDeviceInfo = hCmdCompHandle;
	MUSA_HWPERF_FTRACE_DATA* psFtraceData;
	MTGPU_ERROR		eError;
	MT_PBYTE			pBuffer;
	MT_UINT32			ui32ReadLen;
	MT_BOOL			bFTraceLockAcquired = MT_FALSE;

	MT_DPF_ENTERED;

	MT_ASSERT(psDeviceInfo != NULL);

	psFtraceData = psDeviceInfo->pvGpuFtraceData;

	/* Command-complete notifiers can run concurrently. If this is
	 * happening, just bail out and let the previous call finish.
	 * This is ok because we can process the queued packets on the next call.
	 */
	bFTraceLockAcquired = OSTryLockAcquire(psFtraceData->hFTraceResourceLock);
	if (MT_FALSE == bFTraceLockAcquired)
	{
		MT_DPF_RETURN;
	}

	/* If this notifier is called, it means the TL resources will be valid at-least
	 * until the end of this call, since the DeInit function will wait on the hFTraceResourceLock
	 * to clean-up the TL resources and un-register the notifier, so just assert here.
	 */
	MT_ASSERT(psFtraceData->hGPUTraceTLStream);

	/* If we have a valid stream attempt to acquire some data */
	eError = TLClientAcquireData(DIRECT_BRIDGE_HANDLE, psFtraceData->hGPUTraceTLStream, &pBuffer, &ui32ReadLen);
	if (eError == MTGPU_OK)
	{
		/* Process the HWPerf packets and release the data */
		if (ui32ReadLen > 0)
		{
			MT_DPF((MT_DBG_VERBOSE, "_GpuTraceCmdCompleteNotify: DATA AVAILABLE offset=%p, length=%d", pBuffer, ui32ReadLen));

			/* Process the transport layer data for HWPerf packets... */
			_GpuTraceProcessPackets(psDeviceInfo, pBuffer, ui32ReadLen);

			eError = TLClientReleaseData(DIRECT_BRIDGE_HANDLE, psFtraceData->hGPUTraceTLStream);
			if (eError != MTGPU_OK)
			{
				MT_LOG_ERROR(eError, "TLClientReleaseData");

				/* Serious error, disable FTrace GPU events */

				/* Release TraceLock so we always have the locking
				 * order BridgeLock->TraceLock to prevent AB-BA deadlocks*/
				OSLockRelease(psFtraceData->hFTraceResourceLock);
				OSLockAcquire(psFtraceData->hFTraceResourceLock);
				_GpuTraceDisable(psDeviceInfo, MT_FALSE);
				OSLockRelease(psFtraceData->hFTraceResourceLock);
				goto out;

			}
		} /* else no data, ignore */
	}
	else if (eError != MTGPU_ERROR_TIMEOUT)
	{
		MT_LOG_ERROR(eError, "TLClientAcquireData");
	}
	if (bFTraceLockAcquired)
	{
		OSLockRelease(psFtraceData->hFTraceResourceLock);
	}
out:
	MT_DPF_RETURN;
}

/* ----- AppHint interface -------------------------------------------------- */

static MTGPU_ERROR _GpuTraceIsEnabledCallback(
	const MTGPU_DEVICE_NODE *device,
	const void *private_data,
	MT_BOOL *value)
{
	MT_UNREFERENCED_PARAMETER(device);
	MT_UNREFERENCED_PARAMETER(private_data);

	*value = gbFTraceGPUEventsEnabled;

	return MTGPU_OK;
}

static MTGPU_ERROR _GpuTraceSetEnabledCallback(
	const MTGPU_DEVICE_NODE *device,
	const void *private_data,
	MT_BOOL value)
{
	MT_UNREFERENCED_PARAMETER(device);

	/* Lock down the state to avoid concurrent writes */
	OSLockAcquire(ghGPUTraceStateLock);

	if (value != gbFTraceGPUEventsEnabled)
	{
		MTGPU_ERROR eError;
		if ((eError = _GpuTraceSetEnabledForAllDevices(value)) == MTGPU_OK)
		{
			MT_TRACE(("%s GPU FTrace", value ? "ENABLED" : "DISABLED"));
			gbFTraceGPUEventsEnabled = value;
		}
		else
		{
			MT_TRACE(("FAILED to %s GPU FTrace", value ? "enable" : "disable"));
			/* On failure, partial enable/disable might have resulted.
			 * Try best to restore to previous state. Ignore error */
			_GpuTraceSetEnabledForAllDevices(gbFTraceGPUEventsEnabled);

			OSLockRelease(ghGPUTraceStateLock);
			return eError;
		}
	}
	else
	{
		MT_TRACE(("GPU FTrace already %s!", value ? "enabled" : "disabled"));
	}

	OSLockRelease(ghGPUTraceStateLock);

	return MTGPU_OK;
}

void MTGpuTraceInitAppHintCallbacks(const MTGPU_DEVICE_NODE *psDeviceNode)
{
	MTGPUAppHintRegisterHandlersBOOL(APPHINT_ID_EnableFTraceGPU,
	                                  _GpuTraceIsEnabledCallback,
	                                  _GpuTraceSetEnabledCallback,
	                                  psDeviceNode, NULL);
}

/* ----- FTrace event callbacks -------------------------------------------- */

void MTGpuTraceEnableUfoCallback(void)
{
	MTGPU_DATA *psMTGPUData = MTGPUGetMTGPUData();
	MTGPU_DEVICE_NODE *psDeviceNode;
#if defined(SUPPORT_MUSA)
	MTGPU_MUSADEV_INFO *psMusaDevInfo;
	MTGPU_ERROR eError;
#endif

	/* Lock down events state, for consistent value of guiUfoEventRef */
	OSLockAcquire(ghLockFTraceEventLock);
	if (guiUfoEventRef++ == 0)
	{
		OSWRLockAcquireRead(psMTGPUData->hDeviceNodeListLock);
		psDeviceNode = psMTGPUData->psDeviceNodeList;

		/* make sure UFO events are enabled on all mtgpu devices */
		while (psDeviceNode)
		{
#if defined(SUPPORT_MUSA)
			MT_UINT64 ui64Filter;

			psMusaDevInfo = psDeviceNode->pvDevice;
			ui64Filter = MUSA_HWPERF_EVENT_MASK_VALUE(MUSA_HWPERF_UFO) |
							psMusaDevInfo->ui64HWPerfFilter;
			/* Small chance exists that ui64HWPerfFilter can be changed here and
			 * the newest filter value will be changed to the old one + UFO event.
			 * This is not a critical problem. */
			eError = MTGPUMUSACtrlHWPerfKM(NULL, psDeviceNode, MUSA_HWPERF_STREAM_ID0_FW,
											MT_FALSE, ui64Filter);
			if (eError == MTGPU_ERROR_NOT_INITIALISED)
			{
				/* If we land here that means that the FW is not initialised yet.
				 * We stored the filter and it will be passed to the firmware
				 * during its initialisation phase. So ignore. */
			}
			else if (eError != MTGPU_OK)
			{
				MT_DPF((MT_DBG_ERROR, "Could not enable UFO HWPerf events on device %d", psDeviceNode->sDevId.i32OsDeviceID));
			}
#endif
			psDeviceNode = psDeviceNode->psNext;
		}

		OSWRLockReleaseRead(psMTGPUData->hDeviceNodeListLock);
	}
	OSLockRelease(ghLockFTraceEventLock);
}

void MTGpuTraceDisableUfoCallback(void)
{
#if defined(SUPPORT_MUSA)
	MTGPU_ERROR eError;
#endif
	MTGPU_DATA *psMTGPUData = MTGPUGetMTGPUData();
	MTGPU_DEVICE_NODE *psDeviceNode;

	/* We have to check if lock is valid because on driver unload
	 * MTGpuTraceSupportDeInit is called before kernel disables the ftrace
	 * events. This means that the lock will be destroyed before this callback
	 * is called.
	 * We can safely return if that situation happens because driver will be
	 * unloaded so we don't care about HWPerf state anymore. */
	if (ghLockFTraceEventLock == NULL)
		return;

	OSWRLockAcquireRead(psMTGPUData->hDeviceNodeListLock);
	psDeviceNode = psMTGPUData->psDeviceNodeList;

	/* Lock down events state, for consistent value of guiUfoEventRef */
	OSLockAcquire(ghLockFTraceEventLock);
	if (--guiUfoEventRef == 0)
	{
		/* make sure UFO events are disabled on all mtgpu devices */
		while (psDeviceNode)
		{
#if defined(SUPPORT_MUSA)
			MT_UINT64 ui64Filter;
			MTGPU_MUSADEV_INFO *psMusaDevInfo = psDeviceNode->pvDevice;

			ui64Filter = ~(MUSA_HWPERF_EVENT_MASK_VALUE(MUSA_HWPERF_UFO)) &
					psMusaDevInfo->ui64HWPerfFilter;
			/* Small chance exists that ui64HWPerfFilter can be changed here and
			 * the newest filter value will be changed to the old one + UFO event.
			 * This is not a critical problem. */
			eError = MTGPUMUSACtrlHWPerfKM(NULL, psDeviceNode, MUSA_HWPERF_STREAM_ID0_FW,
			                               MT_FALSE, ui64Filter);
			if (eError == MTGPU_ERROR_NOT_INITIALISED)
			{
				/* If we land here that means that the FW is not initialised yet.
				 * We stored the filter and it will be passed to the firmware
				 * during its initialisation phase. So ignore. */
			}
			else if (eError != MTGPU_OK)
			{
				MT_DPF((MT_DBG_ERROR, "Could not disable UFO HWPerf events on device %d",
				        psDeviceNode->sDevId.i32OsDeviceID));
			}
#endif

			psDeviceNode = psDeviceNode->psNext;
		}
	}
	OSLockRelease(ghLockFTraceEventLock);

	OSWRLockReleaseRead(psMTGPUData->hDeviceNodeListLock);
}

void MTGpuTraceEnableFirmwareActivityCallback(void)
{
	MTGPU_DATA *psMTGPUData = MTGPUGetMTGPUData();
	MTGPU_DEVICE_NODE *psDeviceNode;
#if defined(SUPPORT_MUSA)
	MTGPU_MUSADEV_INFO *psMusaDevInfo;
	uint64_t ui64Filter, ui64FWEventsFilter = 0;
	int i;

	for (i = MUSA_HWPERF_FW_EVENT_RANGE_FIRST_TYPE;
		 i <= MUSA_HWPERF_FW_EVENT_RANGE_LAST_TYPE; i++)
	{
		ui64FWEventsFilter |= MUSA_HWPERF_EVENT_MASK_VALUE(i);
	}
#endif

	OSWRLockAcquireRead(psMTGPUData->hDeviceNodeListLock);
	psDeviceNode = psMTGPUData->psDeviceNodeList;

	OSLockAcquire(ghLockFTraceEventLock);
	/* Enable all FW events on all the devices */
	while (psDeviceNode)
	{
#if defined(SUPPORT_MUSA)
		MTGPU_ERROR eError;
		psMusaDevInfo = psDeviceNode->pvDevice;
		ui64Filter = psMusaDevInfo->ui64HWPerfFilter | ui64FWEventsFilter;

		eError = MTGPUMUSACtrlHWPerfKM(NULL, psDeviceNode, MUSA_HWPERF_STREAM_ID0_FW,
		                               MT_FALSE, ui64Filter);
		if ((eError != MTGPU_OK) && !MTGPU_VZ_MODE_IS(GUEST))
		{
			MT_DPF((MT_DBG_ERROR, "Could not enable HWPerf event for firmware"
			        " task timings (%s).", MTGPUGetErrorString(eError)));
		}
#endif
		psDeviceNode = psDeviceNode->psNext;
	}
	OSLockRelease(ghLockFTraceEventLock);

	OSWRLockReleaseRead(psMTGPUData->hDeviceNodeListLock);
}

void MTGpuTraceDisableFirmwareActivityCallback(void)
{
	MTGPU_DATA *psMTGPUData = MTGPUGetMTGPUData();
	MTGPU_DEVICE_NODE *psDeviceNode;
#if defined(SUPPORT_MUSA)
	MT_UINT64 ui64FWEventsFilter = ~0;
	int i;
#endif

	/* We have to check if lock is valid because on driver unload
	 * MTGpuTraceSupportDeInit is called before kernel disables the ftrace
	 * events. This means that the lock will be destroyed before this callback
	 * is called.
	 * We can safely return if that situation happens because driver will be
	 * unloaded so we don't care about HWPerf state anymore. */
	if (ghLockFTraceEventLock == NULL)
		return;

	OSWRLockAcquireRead(psMTGPUData->hDeviceNodeListLock);
	psDeviceNode = psMTGPUData->psDeviceNodeList;

#if defined(SUPPORT_MUSA)
	for (i = MUSA_HWPERF_FW_EVENT_RANGE_FIRST_TYPE;
		 i <= MUSA_HWPERF_FW_EVENT_RANGE_LAST_TYPE; i++)
	{
		ui64FWEventsFilter &= ~MUSA_HWPERF_EVENT_MASK_VALUE(i);
	}
#endif

	OSLockAcquire(ghLockFTraceEventLock);

	/* Disable all FW events on all the devices */
	while (psDeviceNode)
	{
#if defined(SUPPORT_MUSA)
		MTGPU_MUSADEV_INFO *psMusaDevInfo = psDeviceNode->pvDevice;
		MT_UINT64 ui64Filter = psMusaDevInfo->ui64HWPerfFilter & ui64FWEventsFilter;

		if ((MTGPUMUSACtrlHWPerfKM(NULL, psDeviceNode, MUSA_HWPERF_STREAM_ID0_FW,
		                          MT_FALSE, ui64Filter) != MTGPU_OK) &&
		    !MTGPU_VZ_MODE_IS(GUEST))
		{
			MT_DPF((MT_DBG_ERROR, "Could not disable HWPerf event for firmware task timings."));
		}
#endif
		psDeviceNode = psDeviceNode->psNext;
	}

	OSLockRelease(ghLockFTraceEventLock);

	OSWRLockReleaseRead(psMTGPUData->hDeviceNodeListLock);
}

/******************************************************************************
 End of file (mt_gputrace.c)
******************************************************************************/

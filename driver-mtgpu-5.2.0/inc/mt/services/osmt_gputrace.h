/*************************************************************************/ /*!
@File           osmt_gputrace.h
@Title          MT GPU Trace module common environment interface
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

#ifndef MT_GPUTRACE_H_
#define MT_GPUTRACE_H_

#include "img_types.h"
#include "img_defs.h"
#include "musa_hwperf.h"
#include "device.h"

#if defined(__linux__)

void MTGpuTraceEnqueueEvent(
		MTGPU_DEVICE_NODE *psDevNode,
		MT_UINT32 ui32FirmwareCtx,
		MT_UINT32 ui32ExternalJobRef,
		MT_UINT32 ui32InternalJobRef,
		MUSA_HWPERF_KICK_TYPE eKickType);

/* Early initialisation of GPU Trace events logic.
 * This function is called on *driver* initialisation. */
MTGPU_ERROR MTGpuTraceSupportInit(void);

/* GPU Trace resources final cleanup.
 * This function is called on driver de-initialisation. */
void MTGpuTraceSupportDeInit(void);

/* Initialisation for AppHints callbacks.
 * This function is called during the late stage of driver initialisation but
 * before the device initialisation but after the debugfs sub-system has been
 * initialised. */
void MTGpuTraceInitAppHintCallbacks(const MTGPU_DEVICE_NODE *psDeviceNode);

/* Per-device initialisation of the GPU Trace resources */
MTGPU_ERROR MTGpuTraceInitDevice(MTGPU_DEVICE_NODE *psDeviceNode);

/* Per-device cleanup for the GPU Trace resources. */
void MTGpuTraceDeInitDevice(MTGPU_DEVICE_NODE *psDeviceNode);

/* Enables the gpu trace sub-system for a given device. */
MTGPU_ERROR MTGpuTraceSetEnabled(
		MTGPU_DEVICE_NODE *psDeviceNode,
		MT_BOOL bNewValue);

/* Returns MT_TRUE if the gpu trace sub-system has been enabled (but not
 * necessarily initialised). */
MT_BOOL MTGpuTraceIsEnabled(void);

/* Performs some initialisation steps if the feature was enabled
 * on driver startup. */
void MTGpuTraceInitIfEnabled(MTGPU_DEVICE_NODE *psDeviceNode);

/* FTrace events callbacks interface */

void MTGpuTraceEnableUfoCallback(void);
void MTGpuTraceDisableUfoCallback(void);

void MTGpuTraceEnableFirmwareActivityCallback(void);
void MTGpuTraceDisableFirmwareActivityCallback(void);

#else /* defined(__linux__) */

static inline void MTGpuTraceEnqueueEvent(
		MTGPU_DEVICE_NODE *psDevNode,
		MT_UINT32 ui32FirmwareCtx,
		MT_UINT32 ui32ExternalJobRef,
		MT_UINT32 ui32InternalJobRef,
		MUSA_HWPERF_KICK_TYPE eKickType)
{
	MT_UNREFERENCED_PARAMETER(psDevNode);
	MT_UNREFERENCED_PARAMETER(ui32ExternalJobRef);
	MT_UNREFERENCED_PARAMETER(ui32InternalJobRef);
	MT_UNREFERENCED_PARAMETER(eKickType);
}

static inline MTGPU_ERROR MTGpuTraceSupportInit(void) {
	return MTGPU_OK;
}

static inline void MTGpuTraceSupportDeInit(void) {}

static inline void MTGpuTraceInitAppHintCallbacks(
		const MTGPU_DEVICE_NODE *psDeviceNode)
{
	MT_UNREFERENCED_PARAMETER(psDeviceNode);
}

static inline MTGPU_ERROR MTGpuTraceInitDevice(
		MTGPU_DEVICE_NODE *psDeviceNode)
{
	MT_UNREFERENCED_PARAMETER(psDeviceNode);
	return MTGPU_OK;
}

static inline void MTGpuTraceDeInitDevice(MTGPU_DEVICE_NODE *psDeviceNode)
{
	MT_UNREFERENCED_PARAMETER(psDeviceNode);
}

static inline MTGPU_ERROR MTGpuTraceSetEnabled(
		MTGPU_DEVICE_NODE *psDeviceNode,
		MT_BOOL bNewValue)
{
	MT_UNREFERENCED_PARAMETER(psDeviceNode);
	MT_UNREFERENCED_PARAMETER(bNewValue);
	return MTGPU_OK;
}

static inline MT_BOOL MTGpuTraceIsEnabled(void)
{
	return MT_FALSE;
}

static inline void MTGpuTraceInitIfEnabled(MTGPU_DEVICE_NODE *psDeviceNode)
{
	MT_UNREFERENCED_PARAMETER(psDeviceNode);
}

static inline void MTGpuTraceEnableUfoCallback(void) {}
static inline void MTGpuTraceDisableUfoCallback(void) {}

static inline void MTGpuTraceEnableFirmwareActivityCallback(void) {}
static inline void MTGpuTraceDisableFirmwareActivityCallback(void) {}

#endif /* defined(__linux__) */

#endif /* MT_GPUTRACE_H_ */

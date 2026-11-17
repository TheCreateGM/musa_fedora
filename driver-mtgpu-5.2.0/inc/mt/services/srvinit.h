/*************************************************************************/ /*!
@File
@Title          Initialisation server internal header
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Defines the connections between the various parts of the
                initialisation server.
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

#ifndef SRVINIT_H
#define SRVINIT_H

#if defined(__cplusplus)
extern "C" {
#endif

#include "img_defs.h"
#include "mtgpu_error.h"
#include "device_connection.h"
#include "device.h"

/*
 * Container for all the apphints
 */
typedef struct _MUSA_SRVINIT_APPHINTS_
{
	MT_UINT32 ui32DriverMode;
	MT_BOOL   bEnableSignatureChecks;
	MT_UINT32 ui32SignatureChecksBufSize;

	MT_BOOL   bAssertOnOutOfMem;
	MT_BOOL   bAssertOnHWRTrigger;
#if defined(SUPPORT_VALIDATION)
	MT_UINT32 ui32RenderKillingCtl;
	MT_UINT32 ui32CMPTRANSFERKillingCtl;
	MT_BOOL   bValidateIrq;
	MT_BOOL   bValidateSOCMPTimer;
	MT_UINT32 ui32AvailablePowUnitsMask;
	MT_UINT32 ui32AvailableRACMask;
	MT_BOOL   bInjectPowUnitsStateMaskChange;
	MT_BOOL   bEnablePowUnitsStateMaskChange;
	MT_UINT32 ui32FBCDCVersionOverride;
	MT_UINT32 aui32TPUTrilinearFracMask[MUSAFWIF_TPU_DM_LAST];
	MT_UINT32 aui32USRMNumRegions[MUSAFWIF_USRM_DM_LAST];
	MT_UINT64 aui64UVBRMNumRegions[MUSAFWIF_UVBRM_DM_LAST];
#endif
	MT_BOOL   bCheckMlist;
	MT_BOOL   bEnableCalcDMUtil;
	MT_BOOL   bDisableClockGating;
	MT_BOOL   bDisableDMOverlap;
	MT_BOOL   bDisableFEDLogging;
	MT_BOOL   bDisablePDP;
	MT_BOOL   bDisableCE;
	MT_BOOL   bEnableDMKillRand;
	MT_BOOL   bEnableRandomCsw;
	MT_BOOL   bEnableSoftResetCsw;
	MT_BOOL   bFilteringMode;
	MT_BOOL   bHWPerfDisableCounterFilter;
	MT_BOOL   bZeroFreelist;
	MT_UINT32 ui32GpuUtilConfig;
	MT_UINT32 ui32EnableFWContextSwitch;
	MT_UINT32 ui32FWContextSwitchProfile;
	MT_UINT32 ui32ISPSchedulingLatencyMode;
	MT_UINT32 ui32HWPerfFWBufSize;
	MT_UINT32 ui32HWPerfHostBufSize;
	MT_UINT32 ui32HWPerfFilter0;
	MT_UINT32 ui32HWPerfFilter1;
	MT_UINT32 ui32HWPerfHostFilter;
	MT_UINT32 ui32TimeCorrClock;
	MT_UINT32 ui32HWRDebugDumpLimit;
	MT_UINT32 ui32JonesDisableMask;
	MT_UINT32 ui32LogType;
	MT_UINT32 ui32FwLogGroup;
	MT_UINT32 ui32FecLogGroup;
	MT_UINT32 ui32TruncateMode;
	MT_UINT32 ui32KCCBSizeLog2;
	MT_UINT32 ui32CMPArbitrationMode;
	FW_PERF_CONF eFirmwarePerf;
	MUSA_ACTIVEPM_CONF eMUSAActivePMConf;
	MUSA_RD_POWER_ISLAND_CONF eMUSARDPowerIslandConf;

	MT_BOOL   bEnableTrustedDeviceAceConfig;
	MT_UINT32 ui32FWContextSwitchCrossDM;
	MT_UINT32 ui32MemoryOptimization;
#if defined(SUPPORT_PHYSMEM_TEST) && !defined(INTEGRITY_OS) && !defined(__QNXNTO__)
	MT_UINT32 ui32PhysMemTestPasses;
#endif
} MUSA_SRVINIT_APPHINTS;

#if defined(SUPPORT_MUSA)
MTGPU_ERROR MUSAInit(MTGPU_DEVICE_NODE *psDeviceNode);
#endif

#if defined(__cplusplus)
}
#endif
#endif /* SRVINIT_H */

/******************************************************************************
 End of file (srvinit.h)
******************************************************************************/

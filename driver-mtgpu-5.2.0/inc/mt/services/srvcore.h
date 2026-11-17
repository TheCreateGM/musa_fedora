/**************************************************************************/ /*!
@File
@Title          MT Bridge Functionality
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Header for the MT Bridge code
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

#ifndef SRVCORE_H
#define SRVCORE_H

#include "lock_types.h"
#include "connection_server.h"
#include "mt_debug.h"

#include "mt_bridge.h"
#if defined(SUPPORT_MUSA)
#include "musa_bridge.h"
#endif

MTGPU_ERROR
CopyFromUserWrapper(CONNECTION_DATA *psConnection,
                    MT_UINT32 ui32DispatchTableEntry,
                    void *pvDest,
                    void __user *pvSrc,
                    MT_UINT32 ui32Size);
MTGPU_ERROR
CopyToUserWrapper(CONNECTION_DATA *psConnection,
                  MT_UINT32 ui32DispatchTableEntry,
                  void __user *pvDest,
                  void *pvSrc,
                  MT_UINT32 ui32Size);

MT_INT
DummyBW(MT_UINT32 ui32DispatchTableEntry,
        MT_UINT8 *psBridgeIn,
        MT_UINT8 *psBridgeOut,
        CONNECTION_DATA *psConnection);

typedef MTGPU_ERROR (*ServerResourceDestroyFunction)(MT_HANDLE, MT_HANDLE);

typedef MT_INT (*BridgeWrapperFunction)(MT_UINT32 ui32DispatchTableEntry,
									 MT_UINT8 *psBridgeIn,
									 MT_UINT8 *psBridgeOut,
									 CONNECTION_DATA *psConnection);

typedef struct _MTGPU_BRIDGE_DISPATCH_TABLE_ENTRY
{
	BridgeWrapperFunction pfFunction; /*!< The wrapper function that validates the ioctl
	                                    arguments before calling into srvkm proper */
	POS_LOCK hBridgeLock; /*!< The bridge lock which needs to be acquired
                                   before calling the above wrapper */
#if defined(DEBUG_BRIDGE_KM)
	const MT_CHAR *pszIOCName; /*!< Name of the ioctl: e.g. "MTGPU_BRIDGE_CONNECT_SERVICES" */
	const MT_CHAR *pszFunctionName; /*!< Name of the wrapper function: e.g. "MTGPUConnectBW" */
	const MT_CHAR *pszBridgeLockName; /*!< Name of bridge lock which will be acquired */
	MT_UINT32 ui32CallCount; /*!< The total number of times the ioctl has been called */
	MT_UINT32 ui32CopyFromUserTotalBytes; /*!< The total number of bytes copied from
	                                         userspace within this ioctl */
	MT_UINT32 ui32CopyToUserTotalBytes; /*!< The total number of bytes copied from
	                                       userspace within this ioctl */
	MT_UINT64 ui64TotalTimeNS; /*!< The total amount of time spent in this bridge function */
	MT_UINT64 ui64MaxTimeNS; /*!< The maximum amount of time for a single call to this bridge function */
#endif
}MTGPU_BRIDGE_DISPATCH_TABLE_ENTRY;

#if defined(SUPPORT_MUSA)
	#define BRIDGE_DISPATCH_TABLE_ENTRY_COUNT  (MTGPU_BRIDGE_MUSA_DISPATCH_LAST+1)
	#define BRIDGE_DISPATCH_TABLE_START_ENTRY_COUNT  (MTGPU_BRIDGE_MUSA_LAST+1)
#else
	#define BRIDGE_DISPATCH_TABLE_ENTRY_COUNT  (MTGPU_BRIDGE_DISPATCH_LAST+1)
	#define BRIDGE_DISPATCH_TABLE_START_ENTRY_COUNT  (MTGPU_BRIDGE_LAST+1)
#endif

extern MTGPU_BRIDGE_DISPATCH_TABLE_ENTRY g_BridgeDispatchTable[BRIDGE_DISPATCH_TABLE_ENTRY_COUNT];

void BridgeDispatchTableStartOffsetsInit(void);

void
_SetDispatchTableEntry(MT_UINT32 ui32BridgeGroup,
                       MT_UINT32 ui32Index,
                       const MT_CHAR *pszIOCName,
                       BridgeWrapperFunction pfFunction,
                       const MT_CHAR *pszFunctionName,
                       POS_LOCK hBridgeLock,
                       const MT_CHAR* pszBridgeLockName);
void
UnsetDispatchTableEntry(MT_UINT32 ui32BridgeGroup,
                        MT_UINT32 ui32Index);


/* PRQA S 0884,3410 2*/ /* macro relies on the lack of brackets */
#define SetDispatchTableEntry(ui32BridgeGroup, ui32Index, pfFunction,\
					hBridgeLock) \
	_SetDispatchTableEntry(ui32BridgeGroup, ui32Index, #ui32Index, (BridgeWrapperFunction)pfFunction, #pfFunction,\
							(POS_LOCK)hBridgeLock, #hBridgeLock)

#define DISPATCH_TABLE_GAP_THRESHOLD 5


#if defined(DEBUG_BRIDGE_KM)
typedef struct _MTGPU_BRIDGE_GLOBAL_STATS
{
	MT_UINT32 ui32IOCTLCount;
	MT_UINT32 ui32TotalCopyFromUserBytes;
	MT_UINT32 ui32TotalCopyToUserBytes;
} MTGPU_BRIDGE_GLOBAL_STATS;

void BridgeGlobalStatsLock(void);
void BridgeGlobalStatsUnlock(void);

/* OS specific code may want to report the stats held here and within the
 * BRIDGE_DISPATCH_TABLE_ENTRYs (E.g. on Linux we report these via a
 * debugfs entry /(sys/kernel/debug|proc)/mt/bridge_stats) */
extern MTGPU_BRIDGE_GLOBAL_STATS g_BridgeGlobalStats;
#endif

void SetPStateBridgecallTable(MT_UINT32 ui32BridgeGroup, MT_UINT32 ui32Index);
MTGPU_ERROR BridgeDispatcherInit(void);
void BridgeDispatcherDeinit(void);

MTGPU_ERROR
BridgedDispatchKM(CONNECTION_DATA * psConnection,
                  MTGPU_BRIDGE_PACKAGE   * psBridgePackageKM);

MTGPU_ERROR
MTGPUConnectKM(CONNECTION_DATA *psConnection,
                MTGPU_DEVICE_NODE * psDeviceNode,
                MT_UINT32 ui32Flags,
                MT_UINT32 ui32ClientBuildOptions,
                MT_UINT32 ui32ClientDDKVersion,
                MT_UINT32 ui32ClientDDKBuild,
                MT_UINT8  *pui8KernelArch,
                MT_UINT32 *ui32CapabilityFlags,
                MT_UINT64 *ui64PackedBvnc);

MTGPU_ERROR
MTGPUDisconnectKM(void);

MTGPU_ERROR
MTGPUAcquireGlobalEventObjectKM(MT_HANDLE *phGlobalEventObject);

MTGPU_ERROR
MTGPUReleaseGlobalEventObjectKM(MT_HANDLE hGlobalEventObject);

MTGPU_ERROR
MTGPUDumpDebugInfoKM(CONNECTION_DATA *psConnection,
                      MTGPU_DEVICE_NODE *psDeviceNode,
                      MT_UINT32 ui32VerbLevel);

MTGPU_ERROR
MTGPUGetDevClockSpeedKM(CONNECTION_DATA * psConnection,
                         MTGPU_DEVICE_NODE *psDeviceNode,
                         MT_PUINT32  pui32MUSAClockSpeed);

MTGPU_ERROR
MTGPUGetMemClockSpeedKM(CONNECTION_DATA *psConnection,
                         MTGPU_DEVICE_NODE *psDeviceNode,
                         MT_PUINT32  pui32MemDDRClockSpeed,
                         MT_PUINT32  pui32MemDDRClockMaxSpeed);

MTGPU_ERROR
MTGPUGetDevUuidKM(CONNECTION_DATA *psConnection,
                   MTGPU_DEVICE_NODE *psDeviceNode,
                   MT_PCHAR pacUuid);

MTGPU_ERROR
MTGPUHWOpTimeoutKM(CONNECTION_DATA *psConnection,
                    MTGPU_DEVICE_NODE *psDeviceNode);

MTGPU_ERROR MTGPUAlignmentCheckKM(CONNECTION_DATA *psConnection,
                                    MTGPU_DEVICE_NODE * psDeviceNode,
                                    MT_UINT32 ui32FWAlignChecksSize,
                                    MT_UINT32 aui32FWAlignChecks[]);

MTGPU_ERROR MTGPUGetDeviceStatusKM(CONNECTION_DATA *psConnection,
                                     MTGPU_DEVICE_NODE *psDeviceNode,
                                     MT_UINT32 *pui32DeviceStatus);

MTGPU_ERROR MTGPUGetMultiCoreInfoKM(CONNECTION_DATA *psConnection,
                                     MTGPU_DEVICE_NODE *psDeviceNode,
                                     MT_UINT32 ui32CapsSize,
                                     MT_UINT32 *pui32NumCores,
                                     MT_UINT64 *pui64Caps);

MTGPU_ERROR MTGPUFindProcessMemStatsKM(MT_PID pid,
					 MT_UINT32 ui32ArrSize,
					 MT_BOOL bAllProcessStats,
					 MT_UINT64 *ui64MemoryStats,
					 MTGPU_DEVICE_NODE *psDeviceNode);

static INLINE
MTGPU_ERROR DestroyServerResource(const SHARED_DEV_CONNECTION hConnection,
                                   MT_HANDLE hEvent,
                                   ServerResourceDestroyFunction pfnDestroyCall,
                                   MT_HANDLE hResource)
{
    MT_UNREFERENCED_PARAMETER(hEvent);

    return pfnDestroyCall(GetBridgeHandle(hConnection), hResource);
}

#endif /* SRVCORE_H */

/******************************************************************************
 End of file (srvcore.h)
******************************************************************************/

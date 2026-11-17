/*************************************************************************/ /*!
@File
@Title          PowerVR services server header file
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

#ifndef MTGPU_H
#define MTGPU_H

#include "connection_server.h"
#include "mtgpu_pool.h"
#include "device.h"
#include "power.h"
#include "syscommon.h"
#include "sysinfo.h"
#include "physheap.h"
#include "cache_ops.h"
#include "mt_notifier.h"
#include "mt_bridge.h"
#if defined(SUPPORT_MUSA)
#include "musa_bridge.h"
#endif
#if defined(__KERNEL__) && defined(__linux__) && !defined(__GENKSYMS__)
#define __mtgpu_defined_struct_enum__
#include <services_kernel_client.h>
#endif

/* temporary memory interface path for vpu module */
#if !defined(NO_HARDWARE)
#include "mtgpu_bo.h"
#else
static inline int mtgpu_system_alloc(struct drm_device *drm, size_t size, void **handle)
{
	return -1;
}

static inline int mtgpu_vram_alloc(struct drm_device *drm, int segment_id, size_t size,
				   dma_addr_t *dev_addr, void **handle)
{
	return -1;
}
#endif

#if defined(SUPPORT_GPUVIRT_VALIDATION)
#include "virt_validation_defs.h"
#endif

#include "vz_vmm_pvz.h"

/*!
 * For OSThreadDestroy(), which may require a retry
 * Try for 100 ms to destroy an OS thread before failing
 */
#define OS_THREAD_DESTROY_TIMEOUT_US 100000ULL
#define OS_THREAD_DESTROY_RETRY_COUNT 10

typedef enum _MTGPU_BUFFER_DOMAIN_
{
	MTGPU_BUF_DOMAIN_VRAM_ONLY,
	MTGPU_BUF_DOMAIN_SYSTEM_ONLY,
	MTGPU_BUF_DOMAIN_VRAM_PREFERRED
} MTGPU_BUFFER_DOMAIN;

typedef enum _POLL_FLAGS_
{
	POLL_FLAG_NONE = 0, /* No message or dump is printed on poll timeout */
	POLL_FLAG_LOG_ERROR = 1, /* Log error on poll timeout */
	POLL_FLAG_DEBUG_DUMP = 2 /* Print debug dump on poll timeout */
} POLL_FLAGS;

typedef struct _BUILD_INFO_
{
	MT_UINT32	ui32BuildOptions;
	MT_UINT32	ui32BuildVersion;
	MT_UINT32	ui32BuildRevision;
	MT_UINT32	ui32BuildType;
#define BUILD_TYPE_DEBUG	0
#define BUILD_TYPE_RELEASE	1
	/* The above fields are self explanatory */
	/* B.V.N.C can be added later if required */
} BUILD_INFO;

typedef struct _DRIVER_INFO_
{
	BUILD_INFO	sUMBuildInfo;
	BUILD_INFO	sKMBuildInfo;
	MT_UINT8	ui8UMSupportedArch;
	MT_UINT8	ui8KMBitArch;

#define	BUILD_ARCH_64BIT			(1 << 0)
#define	BUILD_ARCH_32BIT			(1 << 1)
#define	BUILD_ARCH_BOTH		(BUILD_ARCH_32BIT | BUILD_ARCH_64BIT)
	MT_BOOL	bIsNoMatch;
	MT_BOOL	bKUnitTestRunning;
}DRIVER_INFO;

#if defined(SUPPORT_VALIDATION) && defined(__linux__)
typedef struct MEM_LEAK_INTERVALS_TAG
{
	MT_UINT32 ui32OSAlloc;
	MT_UINT32 ui32GPU;
	MT_UINT32 ui32MMU;
} MEM_LEAK_INTERVALS;
#endif

typedef struct MTGPU_DATA_TAG
{
	MTGPU_DRIVER_MODE    eDriverMode;                    /*!< Driver mode (i.e. native, host or guest) */
	MT_BOOL              bForceApphintDriverMode;        /*!< Indicate if driver mode is forced via apphint */
	DRIVER_INFO           sDriverInfo;
	MT_UINT32            ui32DPFErrorCount;                 /*!< Number of Fatal/Error DPFs */

	POSWR_LOCK            hDeviceNodeListLock;            /*!< Read-Write lock to protect the list of devices */
	MTGPU_DEVICE_NODE    *psDeviceNodeList;              /*!< List head of device nodes */
	MT_UINT32            ui32RegisteredDevices;
	MTGPU_DEVICE_NODE    *psHostMemDeviceNode;           /*!< DeviceNode to be used for device independent
	                                                        host based memory allocations where the DevMem
	                                                        framework is to be used e.g. TL */
	MTGPU_SERVICES_STATE eServicesState;                 /*!< global driver state */

	MT_HANDLE            hGlobalEventObject;             /*!< OS Global Event Object */
	MT_UINT32            ui32GEOConsecutiveTimeouts;     /*!< OS Global Event Object Timeouts */

	MT_HANDLE            hCleanupThread;                 /*!< Cleanup thread */
	MT_HANDLE            hCleanupEventObject;            /*!< Event object to drive cleanup thread */
	POS_SPINLOCK          hCleanupThreadWorkListLock;     /*!< Lock protecting the cleanup thread work list */
	DLLIST_NODE           sCleanupThreadWorkList;         /*!< List of work for the cleanup thread */
	MT_PID               cleanupThreadPid;               /*!< Cleanup thread process id */
	uintptr_t             cleanupThreadTid;               /*!< Cleanup thread id */
	ATOMIC_T              i32NumCleanupItemsQueued;       /*!< Number of items in cleanup thread work list */
	ATOMIC_T              i32NumCleanupItemsNotCompleted; /*!< Number of items dropped from cleanup thread work list
	                                                           after retry limit reached */

	MT_HANDLE            hDevicesWatchdogThread;         /*!< Devices watchdog thread */
	MT_HANDLE            hInterruptTimeoutThread;
	MT_HANDLE            hDevicesWatchdogEvObj;          /*! Event object to drive devices watchdog thread */
	volatile MT_UINT32   ui32DevicesWatchdogPwrTrans;    /*! Number of off -> on power state transitions */
#if !defined(MTGPU_SERVER_THREADS_INDEFINITE_SLEEP)
	volatile MT_UINT32   ui32DevicesWatchdogTimeout;     /*! Timeout for the Devices watchdog Thread */
#endif
#ifdef MT_TESTING_UTILS
	volatile MT_UINT32   ui32DevicesWdWakeupCounter;     /* Need this for the unit tests. */
#endif

#if defined(SUPPORT_AUTOVZ)
	MT_HANDLE            hAutoVzWatchdogThread;          /*!< Devices watchdog thread */
	MT_HANDLE            hAutoVzWatchdogEvObj;           /*! Event object to drive devices watchdog thread */
#endif

	POS_LOCK              hHWPerfHostPeriodicThread_Lock; /*!< Lock for the HWPerf Host periodic thread */
	MT_HANDLE            hHWPerfHostPeriodicThread;      /*!< HWPerf Host periodic thread */
	MT_HANDLE            hHWPerfHostPeriodicEvObj;       /*! Event object to drive HWPerf thread */
	volatile MT_BOOL     bHWPerfHostThreadStop;
	MT_UINT32            ui32HWPerfHostThreadTimeout;

	MT_HANDLE            hPvzConnection;                 /*!< PVZ connection used for cross-VM hyper-calls */
	POS_LOCK              hPvzConnectionLock;             /*!< Lock protecting PVZ connection */
	MT_BOOL              abVmOnline[MUSA_NUM_OS_SUPPORTED];

	MT_BOOL              bUnload;                        /*!< Driver unload is in progress */

	MT_HANDLE            hTLCtrlStream;                  /*! Control plane for TL streams */

	MT_HANDLE            hDriverThreadEventObject;       /*! Event object relating to multi-threading in the Server */
	MT_BOOL              bDriverSuspended;               /*! if TRUE, the driver is suspended and new threads should not enter */
	ATOMIC_T              iNumActiveDriverThreads;        /*! Number of threads active in the Server */

	PMR                   *psInfoPagePMR;                 /*! Handle to exportable PMR of the information page. */
	MT_UINT32            *pui32InfoPage;                 /*! CPU memory mapping for information page. */
	DEVMEM_MEMDESC        *psInfoPageMemDesc;             /*! Memory descriptor of the information page. */
	POS_LOCK              hInfoPageLock;                  /*! Lock guarding access to information page. */

	ATOMIC64_T            ui64SubmissionToken;            /*! global unique id of submission in kmd */

#if defined(SUPPORT_VALIDATION) && defined(__linux__)
	MEM_LEAK_INTERVALS    sMemLeakIntervals;              /*!< How often certain memory leak types will trigger */
#endif
	MT_HANDLE            hThreadsDbgReqNotify;

	MT_UINT32            ui32PDumpBoundDevice;           /*!< PDump is bound to the device first connected to */

#if (MUSA_NUM_OS_SUPPORTED > 1)
	MT_HANDLE mdev_drv_data;
#endif
	POS_LOCK              hFwImageListLock;               /*!< Lock for firmware images list */
	DLLIST_NODE           sFwImageList;                   /*!< List of firmware images(OS_FW_IMAGE)*/
} MTGPU_DATA;


/*!
******************************************************************************
 @Function	MTGPUGetMTGPUData

 @Description	Get a pointer to the global data

 @Return   MTGPU_DATA *
******************************************************************************/
MTGPU_DATA *MTGPUGetMTGPUData(void);

#define MTGPU_KM_ERRORS                     (MTGPUGetMTGPUData()->ui32DPFErrorCount)
#define MTGPU_ERROR_LIMIT_REACHED                (MTGPU_KM_ERRORS == MT_UINT32_MAX)
#define MTGPU_REPORT_ERROR()                do { if (!(MTGPU_ERROR_LIMIT_REACHED)) { MTGPUGetMTGPUData()->ui32DPFErrorCount++; } } while (0)

#define MTGPU_VZ_MODE_IS(_expr)              (DRIVER_MODE_##_expr == MTGPUGetMTGPUData()->eDriverMode)
#define MTGPU_VZ_RETN_IF_MODE(_expr)         do { if (  MTGPU_VZ_MODE_IS(_expr)) { return; } } while (0)
#define MTGPU_VZ_RETN_IF_NOT_MODE(_expr)     do { if (! MTGPU_VZ_MODE_IS(_expr)) { return; } } while (0)
#define MTGPU_VZ_RET_IF_MODE(_expr, _rc)     do { if (  MTGPU_VZ_MODE_IS(_expr)) { return (_rc); } } while (0)
#define MTGPU_VZ_RET_IF_NOT_MODE(_expr, _rc) do { if (! MTGPU_VZ_MODE_IS(_expr)) { return (_rc); } } while (0)

/*!
******************************************************************************
@Note	The driver execution mode AppHint (i.e. MTGPU_APPHINT_DRIVERMODE)
		can be an override or non-override 32-bit value. An override value
		has the MSB bit set & a non-override value has this MSB bit cleared.
		Excluding this MSB bit & interpreting the remaining 31-bit as a
		signed 31-bit integer, the mode values are:
		  [-1 native <default>: 0 host : +1 guest ].
******************************************************************************/
#define MTGPU_VZ_APPHINT_MODE_IS_OVERRIDE(_expr)   ((MT_UINT32)(_expr)&(MT_UINT32)(1<<31))
#define MTGPU_VZ_APPHINT_MODE(_expr)				\
	((((MT_UINT32)(_expr)&(MT_UINT32)0x7FFFFFFF) == (MT_UINT32)0x7FFFFFFF) ? DRIVER_MODE_NATIVE : \
		!((MT_UINT32)(_expr)&(MT_UINT32)0x7FFFFFFF) ? DRIVER_MODE_HOST : \
			((MT_UINT32)((MT_UINT32)(_expr)&(MT_UINT)0x7FFFFFFF)==(MT_UINT32)0x1) ? DRIVER_MODE_GUEST : \
				((MT_UINT32)(_expr)&(MT_UINT32)0x7FFFFFFF))

/*!
******************************************************************************
 @Function	MTGPUPollForValueKM

 @Description
 Polls for a value to match a masked read

 @Input psDevNode : Pointer to device node struct
 @Input pui32LinMemAddr : CPU linear address to poll
 @Input ui32Value : required value
 @Input ui32Mask : Mask
 @Input bDebugDumpOnFailure : Whether poll failure should result into a debug
        dump. CAUTION: When calling this function from code paths which are
        also used by debug-dumping code, this argument MUST be MT_FALSE
        otherwise, we might end up requesting debug-dump in recursion and
        eventually blow-up call stack.

 @Return   MTGPU_ERROR :
******************************************************************************/
MTGPU_ERROR MTGPUPollForValueKM(MTGPU_DEVICE_NODE *psDevNode,
		volatile MT_UINT32 __iomem *pui32LinMemAddr,
		MT_UINT32                   ui32Value,
		MT_UINT32                   ui32Mask,
		POLL_FLAGS                   ePollFlags);

/*!
******************************************************************************
 @Function	MTGPUWaitForValueKM

 @Description
 Waits (using EventObjects) for a value to match a masked read

 @Input  pui32LinMemAddr       : CPU linear address to poll
 @Input  ui32Value             : Required value
 @Input  ui32Mask              : Mask to be applied before checking against
                                 ui32Value
 @Return MTGPU_ERROR          :
******************************************************************************/
MTGPU_ERROR
MTGPUWaitForValueKM(volatile MT_UINT32 __iomem *pui32LinMemAddr,
                     MT_UINT32                  ui32Value,
                     MT_UINT32                  ui32Mask);

/*!
******************************************************************************
 @Function	: MTGPUSystemHasCacheSnooping

 @Description	: Returns whether the system has cache snooping

 @Return : MT_TRUE if the system has cache snooping
******************************************************************************/
MT_BOOL MTGPUSystemHasCacheSnooping(MTGPU_DEVICE_CONFIG *psDevConfig);

/*!
******************************************************************************
 @Function	: MTGPUSystemSnoopingIsEmulated

 @Description : Returns whether system cache snooping support is emulated

 @Return : MT_TRUE if the system cache snooping is emulated in software
******************************************************************************/
MT_BOOL MTGPUSystemSnoopingIsEmulated(MTGPU_DEVICE_CONFIG *psDevConfig);

/*!
******************************************************************************
 @Function	: MTGPUSystemSnoopingOfCPUCache

 @Description	: Returns whether the system supports snooping of the CPU cache

 @Return : MT_TRUE if the system has CPU cache snooping
******************************************************************************/
MT_BOOL MTGPUSystemSnoopingOfCPUCache(MTGPU_DEVICE_CONFIG *psDevConfig);

/*!
******************************************************************************
 @Function	: MTGPUSystemSnoopingOfDeviceCache

 @Description	: Returns whether the system supports snooping of the device cache

 @Return : MT_TRUE if the system has device cache snooping
******************************************************************************/
MT_BOOL MTGPUSystemSnoopingOfDeviceCache(MTGPU_DEVICE_CONFIG *psDevConfig);

/*!
******************************************************************************
 @Function	: MTGPUSystemHasNonMappableLocalMemory

 @Description	: Returns whether the device has non-mappable part of local memory

 @Return : MT_TRUE if the device has non-mappable part of local memory
******************************************************************************/
MT_BOOL MTGPUSystemHasNonMappableLocalMemory(MTGPU_DEVICE_CONFIG *psDevConfig);

/*!
******************************************************************************
 @Function	: MTGPUSystemWaitCycles

 @Description	: Waits for at least ui32Cycles of the Device clk.
******************************************************************************/
void MTGPUSystemWaitCycles(MTGPU_DEVICE_CONFIG *psDevConfig, MT_UINT32 ui32Cycles);

MTGPU_ERROR MTGPUSystemInstallDeviceLISR(void *pvOSDevice,
										   MT_UINT32 ui32IRQ,
										   const MT_CHAR *pszName,
										   PFN_LISR pfnLISR,
										   void *pvData,
										   MT_HANDLE *phLISRData);

MTGPU_ERROR MTGPUSystemUninstallDeviceLISR(MT_HANDLE hLISRData);

int MTGPUGetDriverStatus(void);

/*!
******************************************************************************
 @Function	: MTGPUIsBridgeEnabled

 @Description	: Returns whether the given bridge group is enabled

 @Return : MT_TRUE if the given bridge group is enabled
******************************************************************************/
static inline MT_BOOL MTGPUIsBridgeEnabled(MT_HANDLE hServices, MT_UINT32 ui32BridgeGroup)
{
	MT_UINT32 ui32Bridges;
	MT_UINT32 ui32Offset;

	MT_UNREFERENCED_PARAMETER(hServices);

#if defined(SUPPORT_MUSA)
	if (ui32BridgeGroup >= MTGPU_BRIDGE_MUSA_FIRST)
	{
		ui32Bridges = gui32MUSABridges;
		ui32Offset = MTGPU_BRIDGE_MUSA_FIRST;
	}
	else
#endif /* SUPPORT_MUSA */
	{
		ui32Bridges = gui32MTBridges;
		ui32Offset = MTGPU_BRIDGE_FIRST;
	}

	return (MT_BOOL)(((1U << (ui32BridgeGroup - ui32Offset)) & ui32Bridges) != 0);
}


#if defined(SUPPORT_GPUVIRT_VALIDATION)
#if defined(EMULATOR)
	void SetAxiProtOSid(MT_UINT32 ui32OSid, MT_BOOL bState);
	void SetTrustedDeviceAceEnabled(void);
#endif
#endif

/*!
******************************************************************************
 @Function			: MTGPUCreateHWPerfHostThread

 @Description		: Creates HWPerf event object and thread unless already created

 @Input ui32Timeout	: Initial timeout (ms) between updates on the HWPerf thread

 @Return			: MTGPU_ERROR	MTGPU_OK on success. Otherwise, a MTGPU_
									error code
******************************************************************************/
MTGPU_ERROR MTGPUCreateHWPerfHostThread(MT_UINT32 ui32Timeout);

/*!
******************************************************************************
 @Function			: MTGPUDestroyHWPerfHostThread

 @Description		: Destroys HWPerf event object and thread if created

 @Return			: MTGPU_ERROR	MTGPU_OK on success. Otherwise, a MTGPU_
									error code
******************************************************************************/
MTGPU_ERROR MTGPUDestroyHWPerfHostThread(void);

/*!
******************************************************************************
 @Function			: MTGPUPhysMemHeapsInit

 @Description		: Registers and acquires physical memory heaps

 @Return			: MTGPU_ERROR	MTGPU_OK on success. Otherwise, a MTGPU_
									error code
******************************************************************************/
MTGPU_ERROR MTGPUPhysMemHeapsInit(MTGPU_DEVICE_NODE *psDeviceNode, MTGPU_DEVICE_CONFIG *psDevConfig);

/*!
******************************************************************************
 @Function			: MTGPUPhysMemHeapsDeinit

 @Description		: Releases and unregisters physical memory heaps

 @Return			: MTGPU_ERROR	MTGPU_OK on success. Otherwise, a MTGPU_
									error code
******************************************************************************/
void MTGPUPhysMemHeapsDeinit(MTGPU_DEVICE_NODE *psDeviceNode);

/*************************************************************************/ /*!
@Function       FindPhysHeapConfig
@Description    Find Phys Heap Config from Device Config.
@Input          psDevConfig  Pointer to device config.
@Input          ui32Flags    Find heap that matches flags.
@Return         PHYS_HEAP_CONFIG*  Return a config, or NULL if not found.
*/ /**************************************************************************/
PHYS_HEAP_CONFIG* FindPhysHeapConfig(MTGPU_DEVICE_CONFIG *psDevConfig,
									 PHYS_HEAP_USAGE_FLAGS ui32Flags);

/*************************************************************************/ /*!
@Function       MTGPUGetDeviceInstanceByOSId
@Description    Return the specified device instance by OS Id.
@Input          i32OSInstance        OS device Id to find
@Return         MTGPU_DEVICE_NODE*  Return a device node, or NULL if not found.
*/ /**************************************************************************/
MTGPU_DEVICE_NODE *MTGPUGetDeviceInstanceByOSId(MT_INT32 i32OSInstance);

/*************************************************************************/ /*!
@Function       MTGPUDefaultDomainPower
@Description    Returns psDevNode->eCurrentSysPowerState
@Input          MTGPU_DEVICE_NODE*     Device node
@Return         MTGPU_SYS_POWER_STATE  System power state tracked internally
*/ /**************************************************************************/
MTGPU_SYS_POWER_STATE MTGPUDefaultDomainPower(MTGPU_DEVICE_NODE *psDevNode);

struct file;
struct vm_area_struct;
int MTGPU_MMap(struct file *file, struct vm_area_struct *ps_vma);

SEGMENT_INFO *mt_get_segment_info(MTGPU_DEVICE_NODE *psDeviceNode);
MTGPU_ERROR mtgpu_get_segment_stats(struct drm_device *ddev, int segment_id, struct mtgpu_segment_stats *stats);

MT_BOOL mtgpu_vpu_segment_dedicate(struct mtgpu_segment_info *psSegmentInfo);

MTGPU_ERROR mtgpu_adjust_segment_id(struct mtgpu_segment_info *psSegmentInfo,
				     MT_INT32 *pi32SegmentID,
				     MT_UINT32 *pui32SegmentIndex,
				     MT_BOOL *pbSpecifiedSegment);

int mtgpu_buffer_alloc(struct drm_device *ddev, int segment_id, size_t size,
		       dma_addr_t *dev_addr, void **handle, MTGPU_BUFFER_DOMAIN domain);
#if (MUSA_NUM_OS_SUPPORTED > 1)
MTGPU_ERROR mtgpu_vgpu_version_check_compat(MTGPU_DEVICE_NODE *psDeviceNode);
MTGPU_ERROR mtgpu_vgpu_get_os_version(MTGPU_DEVICE_NODE *psDeviceNode);
#endif

MTGPU_ERROR MTGPUInitGpuMemStats(MTGPU_DEVICE_NODE *psDeviceNode);
void MTGPUDeInitGpuMemStats(MTGPU_DEVICE_NODE *psDeviceNode);
MT_BOOL MTGPUIsSupportDMA(MTGPU_DEVICE_NODE *psDeviceNode);
MTGPU_ERROR MTGPUGetDevice(MTGPU_DEVICE_NODE *psDeviceNode, struct device **psDev);

MTGPU_ERROR MTCpuPhysArrayToPcieObNocAddrArray(MTGPU_DEVICE_NODE *psDevNode,
						MT_CPU_PHYADDR *psCpuPhysAddr,
						MT_DEV_PHYADDR *psDevPAddr, MT_BOOL *pbValid,
						MT_UINT32 ui32PageCount);

MTGPU_ERROR MTGetPcieAccessSysMemAddrRange(MTGPU_DEVICE_NODE *psDevNode,
					    MT_UINT64 *pui64SysMemStart,
					    MT_UINT64 *pui64SysMemEnd);

MT_BOOL MTGPUIsIOMMUOn(MTGPU_DEVICE_NODE *psDevNode);
MT_BOOL MTGPUHasSamePcieRootPort(struct device *psLocalPcieDev,
				   struct device *psRemotePcieDev);
MTGPU_ERROR MTGPUGetDevMemClock(MTGPU_DEVICE_NODE *psDeviceNode, MT_PUINT32 pui32ddr_clk,
				  MT_PUINT32 pui32Maxddrclk);
MTGPU_ERROR MTGPUCheckGpuHealthStatusAndReason(MTGPU_DEVICE_NODE *psDeviceNode);
MT_BOOL MTGPUDeviceNodeIsActive(MTGPU_DEVICE_NODE *psDeviceNode);

#endif /* MTGPU_H */

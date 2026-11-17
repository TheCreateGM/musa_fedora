/*************************************************************************/ /*!
@File
@Title          MUSA firmware utility routines
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    MUSA firmware utility routines
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

#ifndef MUSAFWUTILS_H
#define MUSAFWUTILS_H

#include "musa_memallocflags.h"
#include "musadevice.h"
#include "musaccb.h"
#include "devicemem.h"
#include "device.h"
#include "mt_notifier.h"
#include "mt.h"
#include "connection_server.h"
#include "musagfx.h"
#include "devicemem_utils.h"
#include "musamem.h"

#define MT_EXPLICIT_INCLUDE_HWDEFS
#if defined(__KERNEL__)
#include "musa_cr_defs_km.h"
#endif
#undef MT_EXPLICIT_INCLUDE_HWDEFS

#define MUSA_FIRMWARE_GUEST_RAW_HEAP_IDENT   "FwRawOSID%d" /*!< MUSA Raw Firmware Heap identifier */

static INLINE MTGPU_ERROR _SelectDevMemHeap(MTGPU_MUSADEV_INFO *psDevInfo,
											 MTGPU_MEMALLOCFLAGS_T *puiFlags,
											 DEVMEM_HEAP **ppsFwHeap)
{
	MTGPU_PHYS_HEAP ePhysHeap = (MTGPU_PHYS_HEAP)(MTGPU_PHYS_HEAP)MTGPU_GET_PHYS_HEAP_HINT(*puiFlags);
	MTGPU_ERROR eError = MTGPU_OK;

	switch (ePhysHeap)
	{
#if defined(SUPPORT_SECURITY_VALIDATION)
		/* call with GPU_SECURE from MUSASetupFwSysData */
		case MTGPU_PHYS_HEAP_GPU_SECURE:
#endif
		case MTGPU_PHYS_HEAP_FW_CODE:
		case MTGPU_PHYS_HEAP_FW_PRIV_DATA:
		case MTGPU_PHYS_HEAP_FW_MAIN:
		{
			*ppsFwHeap = psDevInfo->psFirmwareMainHeap;
			break;
		}
		case MTGPU_PHYS_HEAP_FW_CONFIG:
		{
			*ppsFwHeap = psDevInfo->psFirmwareConfigHeap;
			break;
		}
		case MTGPU_PHYS_HEAP_FW_PREMAP0:
		case MTGPU_PHYS_HEAP_FW_PREMAP1:
		case MTGPU_PHYS_HEAP_FW_PREMAP2:
		case MTGPU_PHYS_HEAP_FW_PREMAP3:
		case MTGPU_PHYS_HEAP_FW_PREMAP4:
		case MTGPU_PHYS_HEAP_FW_PREMAP5:
		case MTGPU_PHYS_HEAP_FW_PREMAP6:
		case MTGPU_PHYS_HEAP_FW_PREMAP7:
#if defined(SUPPORT_SW_OSID_EXTENSION)
		case MTGPU_PHYS_HEAP_FW_PREMAP8:
		case MTGPU_PHYS_HEAP_FW_PREMAP9:
		case MTGPU_PHYS_HEAP_FW_PREMAP10:
		case MTGPU_PHYS_HEAP_FW_PREMAP11:
		case MTGPU_PHYS_HEAP_FW_PREMAP12:
		case MTGPU_PHYS_HEAP_FW_PREMAP13:
		case MTGPU_PHYS_HEAP_FW_PREMAP14:
		case MTGPU_PHYS_HEAP_FW_PREMAP15:
#endif
		{
			MT_UINT32 ui32OSID = ePhysHeap - MTGPU_PHYS_HEAP_FW_PREMAP0;

			MT_LOG_RETURN_IF_INVALID_PARAM(ui32OSID < MUSA_NUM_OS_SUPPORTED, "ui32OSID");
			*ppsFwHeap = psDevInfo->psGuestFirmwareRawHeap[ui32OSID];
			break;
		}
		default:
		{
			MT_DPF((MT_DBG_ERROR, "%s: invalid phys heap", __func__));
			eError = MTGPU_ERROR_INVALID_PARAMS;
			break;
		}
	}

	return eError;
}

/*
 * Firmware-only allocation (which are initialised by the host) must be aligned to the L2 cache line size.
 * This is because firmware-only allocations are GPU_CACHE_INCOHERENT and this causes problems
 * if two allocations share the same cache line; e.g. the initialisation of the second allocation won't
 * make it into the L2 cache because it has been already loaded when accessing the content of the first allocation.
 */
static INLINE MTGPU_ERROR DevmemFwAllocate(MTGPU_MUSADEV_INFO *psDevInfo,
											MT_DEVMEM_SIZE_T uiSize,
											MTGPU_MEMALLOCFLAGS_T uiFlags,
											const MT_CHAR *pszText,
											DEVMEM_MEMDESC **ppsMemDescPtr)
{
	uint64_t sTmpDevVAddr;
	MTGPU_ERROR eError;
	DEVMEM_HEAP *psFwHeap;
	MT_DEVMEM_ALIGN_T uiAlign;

	MT_DPF_ENTERED;

	/* Enforce the standard pre-fix naming scheme callers must follow */
	MT_ASSERT((pszText != NULL) && (pszText[0] == 'F') && (pszText[1] == 'w'));

	/* Imported from AppHint , flag to poison allocations when freed */
	uiFlags |= psDevInfo->uiFWPoisonOnFreeFlag;

	eError = _SelectDevMemHeap(psDevInfo, &uiFlags, &psFwHeap);
	if (eError != MTGPU_OK)
	{
		MT_DPF_RETURN_RC(eError);
	}

	uiAlign = (psFwHeap == psDevInfo->psFirmwareConfigHeap) ?
				(MUSA_FIRMWARE_CONFIG_HEAP_ALLOC_GRANULARITY) :
				(GET_MUSA_CACHE_LINE_SIZE(MUSA_GET_FEATURE_VALUE(psDevInfo, L2_CACHE_LINE_SIZE_BITS)));

	eError = DevmemAllocateAndMap(psFwHeap,
				uiSize,
				uiAlign,
				uiFlags,
				pszText,
				ppsMemDescPtr,
				&sTmpDevVAddr);

	MT_DPF_RETURN_RC(eError);
}

static INLINE MTGPU_ERROR DevmemFwAllocateExportable(MTGPU_DEVICE_NODE *psDeviceNode,
													  MT_DEVMEM_SIZE_T uiSize,
													  MT_DEVMEM_ALIGN_T uiAlign,
													  MTGPU_MEMALLOCFLAGS_T uiFlags,
													  const MT_CHAR *pszText,
													  DEVMEM_MEMDESC **ppsMemDescPtr)
{
	MTGPU_MUSADEV_INFO *psDevInfo = (MTGPU_MUSADEV_INFO *) psDeviceNode->pvDevice;
	uint64_t sTmpDevVAddr;
	MTGPU_ERROR eError;
	DEVMEM_HEAP *psFwHeap;

	MT_DPF_ENTERED;

	/* Enforce the standard pre-fix naming scheme callers must follow */
	MT_ASSERT((pszText != NULL) &&
			(pszText[0] == 'F') && (pszText[1] == 'w') &&
			(pszText[2] == 'E') && (pszText[3] == 'x'));

	/* Imported from AppHint , flag to poison allocations when freed */
	uiFlags |= psDevInfo->uiFWPoisonOnFreeFlag;

	eError = _SelectDevMemHeap(psDevInfo, &uiFlags, &psFwHeap);
	if (eError != MTGPU_OK)
	{
		MT_DPF_RETURN_RC(eError);
	}

	eError = DevmemAllocateExportable(psDeviceNode,
									  uiSize,
									  uiAlign,
									  DevmemGetHeapLog2PageSize(psFwHeap),
									  uiFlags,
									  pszText,
									  ppsMemDescPtr);
	if (eError != MTGPU_OK)
	{
		MT_DPF((MT_DBG_ERROR, "FW DevmemAllocateExportable failed (%u)", eError));
		MT_DPF_RETURN_RC(eError);
	}

	/*
		We need to map it so the heap for this allocation
		is set
	*/
	eError = DevmemMapToDevice(*ppsMemDescPtr,
							   psDevInfo->psFirmwareMainHeap,
							   &sTmpDevVAddr);
	if (eError != MTGPU_OK)
	{
		DevmemFree(*ppsMemDescPtr);
		MT_DPF((MT_DBG_ERROR, "FW DevmemMapToDevice failed (%u)", eError));
	}

	MT_DPF_RETURN_RC1(eError, *ppsMemDescPtr);
}

static INLINE void DevmemFwUnmapAndFree(MTGPU_MUSADEV_INFO *psDevInfo,
								DEVMEM_MEMDESC *psMemDesc)
{
	MT_DPF_ENTERED1(psMemDesc);

	DevmemUnmapFromDevice(psMemDesc);
	DevmemFree(psMemDesc);

	MT_DPF_RETURN;
}


/*
 * This function returns the value of the hardware register MUSA_CR_TIMER
 * which is a timer counting in ticks.
 */

static INLINE MT_UINT64 MUSAReadHWTimerReg(MTGPU_MUSADEV_INFO *psDevInfo)
{
	MT_UINT64 ui64Time = OSReadHWReg64(psDevInfo->pvRegsBaseKM, MUSA_CR_TIMER);

	/*
	*  In order to avoid having to issue three 32-bit reads to detect the
	*  lower 32-bits wrapping, the MSB of the low 32-bit word is duplicated
	*  in the MSB of the high 32-bit word. If the wrap happens, we just read
	*  the register again (it will not wrap again so soon).
	*/
	if ((ui64Time ^ (ui64Time << 32)) & ~MUSA_CR_TIMER_BIT31_CLRMSK)
	{
		ui64Time = OSReadHWReg64(psDevInfo->pvRegsBaseKM, MUSA_CR_TIMER);
	}

	return (ui64Time & ~MUSA_CR_TIMER_VALUE_CLRMSK) >> MUSA_CR_TIMER_VALUE_SHIFT;
}

static INLINE MT_UINT64 MUSAReadSocTimerReg(MTGPU_MUSADEV_INFO *psDevInfo)
{
	MT_UINT64 ui64Time = OSReadHWReg64(psDevInfo->pvRegsBaseKM, MUSA_CR_SOC_TIMER_BINARY);

	return ((ui64Time & ~MUSA_CR_SOC_TIMER_BINARY_VALUE_CLRMSK) >> MUSA_CR_SOC_TIMER_BINARY_VALUE_SHIFT);
}

/*
 * This FW Common Context is only mapped into kernel for initialisation and cleanup purposes.
 * Otherwise this allocation is only used by the FW.
 * Therefore the GPU cache doesn't need coherency, and write-combine will
 * suffice on the CPU side (WC buffer will be flushed at the first kick)
 */
#define MUSA_FWCOMCTX_ALLOCFLAGS      (MTGPU_MEMALLOCFLAG_DEVICE_FLAG(PMMCU_PROTECT) | \
                                      MTGPU_MEMALLOCFLAG_DEVICE_FLAG(FIRMWARE_CACHED)| \
                                      MTGPU_MEMALLOCFLAG_GPU_READABLE | \
                                      MTGPU_MEMALLOCFLAG_GPU_WRITEABLE | \
                                      MTGPU_MEMALLOCFLAG_GPU_CACHE_INCOHERENT | \
                                      MTGPU_MEMALLOCFLAG_CPU_READABLE | \
                                      MTGPU_MEMALLOCFLAG_CPU_WRITEABLE | \
                                      MTGPU_MEMALLOCFLAG_CPU_UNCACHED_WC | \
                                      MTGPU_MEMALLOCFLAG_KERNEL_CPU_MAPPABLE | \
                                      MTGPU_MEMALLOCFLAG_ZERO_ON_ALLOC | \
                                      MTGPU_MEMALLOCFLAG_PHYS_HEAP_HINT(FW_MAIN))

#define MUSA_FWSHAREDMEM_MAIN_ALLOCFLAGS (MTGPU_MEMALLOCFLAG_DEVICE_FLAG(PMMCU_PROTECT) | \
                                         MTGPU_MEMALLOCFLAG_GPU_READABLE | \
                                         MTGPU_MEMALLOCFLAG_GPU_WRITEABLE | \
                                         MTGPU_MEMALLOCFLAG_GPU_UNCACHED | \
                                         MTGPU_MEMALLOCFLAG_CPU_READABLE | \
                                         MTGPU_MEMALLOCFLAG_CPU_WRITEABLE | \
                                         MTGPU_MEMALLOCFLAG_CPU_UNCACHED_WC | \
                                         MTGPU_MEMALLOCFLAG_KERNEL_CPU_MAPPABLE | \
                                         MTGPU_MEMALLOCFLAG_ZERO_ON_ALLOC | \
                                         MTGPU_MEMALLOCFLAG_PHYS_HEAP_HINT(FW_MAIN))

#define MUSA_FWSHAREDMEM_CONFIG_ALLOCFLAGS (MTGPU_MEMALLOCFLAG_DEVICE_FLAG(PMMCU_PROTECT) | \
                                           MTGPU_MEMALLOCFLAG_GPU_READABLE | \
                                           MTGPU_MEMALLOCFLAG_GPU_WRITEABLE | \
                                           MTGPU_MEMALLOCFLAG_GPU_UNCACHED | \
                                           MTGPU_MEMALLOCFLAG_CPU_READABLE | \
                                           MTGPU_MEMALLOCFLAG_CPU_WRITEABLE | \
                                           MTGPU_MEMALLOCFLAG_CPU_UNCACHED_WC | \
                                           MTGPU_MEMALLOCFLAG_KERNEL_CPU_MAPPABLE | \
                                           MTGPU_MEMALLOCFLAG_ZERO_ON_ALLOC | \
                                           MTGPU_MEMALLOCFLAG_PHYS_HEAP_HINT(FW_CONFIG))

#define MUSA_FWSHAREDMEM_GPU_RO_ALLOCFLAGS (MTGPU_MEMALLOCFLAG_DEVICE_FLAG(PMMCU_PROTECT) | \
                                           MTGPU_MEMALLOCFLAG_GPU_READABLE | \
                                           MTGPU_MEMALLOCFLAG_GPU_UNCACHED | \
                                           MTGPU_MEMALLOCFLAG_CPU_READABLE | \
                                           MTGPU_MEMALLOCFLAG_CPU_WRITEABLE | \
                                           MTGPU_MEMALLOCFLAG_CPU_UNCACHED_WC | \
                                           MTGPU_MEMALLOCFLAG_KERNEL_CPU_MAPPABLE | \
                                           MTGPU_MEMALLOCFLAG_ZERO_ON_ALLOC | \
                                           MTGPU_MEMALLOCFLAG_PHYS_HEAP_HINT(FW_MAIN))

/* For GPU read-only data, IGPU can snoop the CPU cache. */
#define MUSA_FWSHAREDMEM_GPU_RO_WITH_COHERENCY_ALLOCFLAGS (MTGPU_MEMALLOCFLAG_DEVICE_FLAG(PMMCU_PROTECT) | \
							  MTGPU_MEMALLOCFLAG_GPU_READABLE | \
							  MTGPU_MEMALLOCFLAG_GPU_CACHE_COHERENT | \
							  MTGPU_MEMALLOCFLAG_CPU_READABLE | \
							  MTGPU_MEMALLOCFLAG_CPU_WRITEABLE | \
							  MTGPU_MEMALLOCFLAG_CPU_CACHE_CLEAN | \
							  MTGPU_MEMALLOCFLAG_CPU_CACHE_INCOHERENT | \
							  MTGPU_MEMALLOCFLAG_KERNEL_CPU_MAPPABLE | \
							  MTGPU_MEMALLOCFLAG_ZERO_ON_ALLOC | \
							  MTGPU_MEMALLOCFLAG_PHYS_HEAP_HINT(FW_MAIN))

/* Firmware memory that is not accessible by the CPU. */
#define MUSA_FWSHAREDMEM_GPU_ONLY_ALLOCFLAGS (MTGPU_MEMALLOCFLAG_DEVICE_FLAG(PMMCU_PROTECT) | \
                                             MTGPU_MEMALLOCFLAG_GPU_READABLE | \
                                             MTGPU_MEMALLOCFLAG_GPU_WRITEABLE | \
                                             MTGPU_MEMALLOCFLAG_GPU_UNCACHED | \
                                             MTGPU_MEMALLOCFLAG_ZERO_ON_ALLOC)

/* Firmware shared memory that is supposed to be read-only to the CPU.
 * In reality it isn't due to ZERO_ON_ALLOC which enforces CPU_WRITEABLE
 * flag on the allocations. */
#define MUSA_FWSHAREDMEM_CPU_RO_ALLOCFLAGS (MTGPU_MEMALLOCFLAG_DEVICE_FLAG(PMMCU_PROTECT) | \
                                           MTGPU_MEMALLOCFLAG_PHYS_HEAP_HINT(FW_MAIN) | \
                                           MTGPU_MEMALLOCFLAG_GPU_READABLE | \
                                           MTGPU_MEMALLOCFLAG_GPU_WRITEABLE | \
                                           MTGPU_MEMALLOCFLAG_CPU_READABLE | \
                                           MTGPU_MEMALLOCFLAG_KERNEL_CPU_MAPPABLE | \
                                           MTGPU_MEMALLOCFLAG_GPU_UNCACHED | \
                                           MTGPU_MEMALLOCFLAG_CPU_UNCACHED_WC | \
                                           MTGPU_MEMALLOCFLAG_ZERO_ON_ALLOC)

/* data content being kept from previous boot cycles from physical memory must not be cleared during allocation */
#define MUSA_AUTOVZ_KEEP_FW_DATA_MASK(bKeepMem) ((bKeepMem) ? (~MTGPU_MEMALLOCFLAG_ZERO_ON_ALLOC) : (~0ULL))

/******************************************************************************
 * MUSASetFirmwareAddress Flags
 *****************************************************************************/
#define RFW_FWADDR_FLAG_NONE		(0)			/*!< Void flag */
#define RFW_FWADDR_NOREF_FLAG		(1U << 0)	/*!< It is safe to immediately release the reference to the pointer,
												  otherwise MUSAUnsetFirmwareAddress() must be call when finished. */

MT_BOOL MUSATraceBufferIsInitRequired(MTGPU_MUSADEV_INFO *psDevInfo);
MTGPU_ERROR MUSATraceBufferInitOnDemandResources(MTGPU_MUSADEV_INFO* psDevInfo, MTGPU_MEMALLOCFLAGS_T uiAllocFlags);

#if defined(SUPPORT_POWMON_COMPONENT) && defined(SUPPORT_POWER_VALIDATION_VIA_DEBUGFS)
MT_BOOL MUSAPowmonBufferIsInitRequired(MTGPU_MUSADEV_INFO *psDevInfo);
MTGPU_ERROR MUSAPowmonBufferInitOnDemandResources(MTGPU_MUSADEV_INFO *psDevInfo);
#endif

MTGPU_ERROR MUSASetupFirmware(MTGPU_DEVICE_NODE       *psDeviceNode,
                              MT_BOOL                 bEnableSignatureChecks,
                              MT_UINT32               ui32SignatureChecksBufSize,
                              MT_UINT32               ui32HWPerfFWBufSizeKB,
                              MT_UINT64               ui64HWPerfFilter,
                              MT_UINT32               ui32ConfigFlags,
                              MT_UINT32               ui32ConfigFlagsExt,
                              MT_UINT32               ui32FwOsCfgFlags,
                              MT_UINT32               ui32LogType,
                              MT_UINT32               ui32FilterFlags,
                              MT_UINT32               ui32JonesDisableMask,
                              MT_UINT32               ui32HWRDebugDumpLimit,
                              MT_UINT32               ui32HWPerfCountersDataSize,
                              MT_UINT32               ui32RenderKillingCtl,
                              MT_UINT32               ui32CMPTRANSFERKillingCtl,
                              MT_UINT32               *pui32TPUTrilinearFracMask,
                              MT_UINT32               *pui32USRMNumRegions,
                              MT_UINT64               *pui64UVBRMNumRegions,
                              MUSA_RD_POWER_ISLAND_CONF eMUSARDPowerIslandConf,
                              FW_PERF_CONF             eFirmwarePerf,
                              MT_UINT32               ui32KCCBSizeLog2,
                              MT_UINT32               ui32AvailableMPXMask,
							  MT_UINT32               ui32AvailableRACMask);



void MUSAFreeFirmware(MTGPU_MUSADEV_INFO *psDevInfo);

/*************************************************************************/ /*!
@Function       MUSASetupFwAllocation

@Description    Sets a pointer in a firmware data structure.

@Input          psDevInfo       Device Info struct
@Input          uiAllocFlags    Flags determining type of memory allocation
@Input          ui32Size        Size of memory allocation
@Input          pszName         Allocation label
@Input          psFwPtr         Address of the firmware pointer to set
@Input          ppvCpuPtr       Address of the cpu pointer to set
@Input          ui32DevVAFlags  Any combination of  RFW_FWADDR_*_FLAG

@Return         MTGPU_ERROR
*/ /**************************************************************************/
MTGPU_ERROR MUSASetupFwAllocation(MTGPU_MUSADEV_INFO   *psDevInfo,
								  MTGPU_MEMALLOCFLAGS_T uiAllocFlags,
								  MT_UINT32           ui32Size,
								  const MT_CHAR       *pszName,
								  DEVMEM_MEMDESC       **ppsMemDesc,
								  MUSAFWIF_DEV_VIRTADDR *psFwPtr,
								  void                 **ppvCpuPtr,
								  MT_UINT32           ui32DevVAFlags);

/*************************************************************************/ /*!
@Function       MUSASetFirmwareAddress

@Description    Sets a pointer in a firmware data structure.

@Input          ppDest          Address of the pointer to set
@Input          psSrc           MemDesc describing the pointer
@Input          ui32Flags       Any combination of RFW_FWADDR_*_FLAG

@Return         MTGPU_ERROR
*/ /**************************************************************************/
MTGPU_ERROR MUSASetFirmwareAddress(MUSAFWIF_DEV_VIRTADDR	*ppDest,
								   DEVMEM_MEMDESC		*psSrc,
								   MT_UINT32			uiOffset,
								   MT_UINT32			ui32Flags);


/*************************************************************************/ /*!
@Function       MUSASetMcuDMAAddress

@Description    Fills a Firmware structure used to setup the Mcu DMA with two
                pointers to the same data, one on 40 bit and one on 32 bit
                (pointer in the FW memory space).

@Input          ppDest          Address of the structure to set
@Input          psSrcMemDesc    MemDesc describing the pointer
@Input          psSrcFWDevVAddr Firmware memory space pointer

@Return         void
*/ /**************************************************************************/
void MUSASetMcuDMAAddress(MUSAFWIF_DMA_ADDR		*psDest,
						  DEVMEM_MEMDESC		*psSrcMemDesc,
						  MUSAFWIF_DEV_VIRTADDR	*psSrcFWDevVAddr,
						  MT_UINT32			uiOffset);


/*************************************************************************/ /*!
@Function       MUSAUnsetFirmwareAddress

@Description    Unsets a pointer in a firmware data structure

@Input          psSrc           MemDesc describing the pointer

@Return         void
*/ /**************************************************************************/
void MUSAUnsetFirmwareAddress(DEVMEM_MEMDESC *psSrc);

MTGPU_ERROR MUSAWriteMcuRegThroughSP(const void *hPrivate, MT_UINT32 ui32RegAddr, MT_UINT32 ui32RegValue);
MTGPU_ERROR MUSAReadMcuRegThroughSP(const void *hPrivate, MT_UINT32 ui32RegAddr, MT_UINT32* ui32RegValue);


MTGPU_ERROR MTDecodePageFaultInfo(CONNECTION_DATA *psConnection,
				   MTGPU_DEVICE_NODE *psDeviceNode,
				   MT_HANDLE hPMR,
				   size_t uiLogicalOffset);


void MUSAGuestCheckHwr(MTGPU_DEVICE_NODE *psDeviceNode);
bool MUSAGuestNeedHwr(MTGPU_DEVICE_NODE *psDeviceNode);
MTGPU_ERROR MUSAHostReplay(MTGPU_DEVICE_NODE *psDeviceNode);

/*!
*******************************************************************************
@Function       MUSAScheduleProcessQueuesKM

@Description    Software command complete handler
                (sends uncounted kicks for all the DMs through the MISR)

@Input          hCmdCompHandle  MUSA device node

@Return         None
******************************************************************************/
void MUSAScheduleProcessQueuesKM(MTGPU_CMDCOMP_HANDLE hCmdCompHandle);

#if defined(SUPPORT_VALIDATION)
/*!
*******************************************************************************
@Function       MUSAScheduleMusaRegCommand

@Input          psDevInfo       Device Info struct
@Input          ui64RegVal      Value to write into FW register
@Input          ui64Size        Register size
@Input          ui32Offset      Register Offset
@Input          bWriteOp        Register Write or Read toggle

@Return         MTGPU_ERROR
******************************************************************************/
MTGPU_ERROR MUSAScheduleMusaRegCommand(MTGPU_MUSADEV_INFO *psDevInfo,
									  MT_UINT64 ui64RegVal,
									  MT_UINT64 ui64Size,
									  MT_UINT32 ui32Offset,
									  MT_BOOL bWriteOp);

#endif

/*!
*******************************************************************************

@Function       MUSAInstallProcessQueuesMISR

@Description    Installs the MISR to handle Process Queues operations

@Input          phMISR          Pointer to the MISR handler
@Input          psDeviceNode    MUSA Device node

@Return         MTGPU_ERROR
******************************************************************************/
MTGPU_ERROR MUSAInstallProcessQueuesMISR(MT_HANDLE *phMISR, MTGPU_DEVICE_NODE *psDeviceNode);

MTGPU_ERROR MUSASendCommandsFromDeferredList(MTGPU_MUSADEV_INFO *psDevInfo, MT_BOOL bPoll);

/*************************************************************************/ /*!
@Function       MTGPUMUSAFrameworkCopyCommand

@Description    Copy framework command into FW addressable buffer

@param          psDeviceNode
@param          psFWFrameworkMemDesc
@param          pbyGPUFRegisterList
@param          ui32FrameworkRegisterSize

@Return         MTGPU_ERROR
*/ /**************************************************************************/
MTGPU_ERROR MTGPUMUSAFrameworkCopyCommand(MTGPU_DEVICE_NODE *psDeviceNode,
										   DEVMEM_MEMDESC *psFWFrameworkMemDesc,
										   MT_PBYTE pbyGPUFRegisterList,
										   MT_UINT32 ui32FrameworkRegisterSize);


/*************************************************************************/ /*!
@Function       MTGPUMUSAFrameworkCreateKM

@Description    Create FW addressable buffer for framework

@param          psDeviceNode
@param          ppsFWFrameworkMemDesc
@param          ui32FrameworkRegisterSize

@Return         MTGPU_ERROR
*/ /**************************************************************************/
MTGPU_ERROR MTGPUMUSAFrameworkCreateKM(MTGPU_DEVICE_NODE * psDeviceNode,
							DEVMEM_MEMDESC ** ppsFWFrameworkMemDesc,
							MT_UINT32 ui32FrameworkRegisterSize);


/*************************************************************************/ /*!
@Function       MUSAStateFlagCtrl

@Description    Set and return FW internal state flags.

@Input          psDevInfo       Device Info
@Input          ui32Config      AppHint config flags
@Output         pui32State      Current AppHint state flag configuration
@Input          bSetNotClear    Set or clear the provided config flags

@Return         MTGPU_ERROR
*/ /**************************************************************************/
MTGPU_ERROR MUSAStateFlagCtrl(MTGPU_MUSADEV_INFO *psDevInfo,
				MT_UINT32 ui32Config,
				MT_UINT32 *pui32State,
				MT_BOOL bSetNotClear);


#if defined(SUPPORT_AUTOVZ)
/*!
*******************************************************************************
@Function       MUSAUpdateAutoVzWdgToken

@Description    If the driver-firmware connection is active, read the
                firmware's watchdog token and copy its value back into the OS
                token. This indicates to the firmware that this driver is alive
                and responsive.

@Input          psDevInfo       pointer to device info
******************************************************************************/
void MUSAUpdateAutoVzWdgToken(MTGPU_MUSADEV_INFO *psDevInfo);
#endif


/*!
*******************************************************************************
@Function       MUSACheckForStalledClientContexts

@Description    Checks all client contexts, for the device with device info
                provided, to see if any are waiting for a fence to signal and
                optionally force signalling of the fence for the context which
                has been waiting the longest.
                This function is called by MUSAUpdateHealthStatus() and also
                may be invoked from other trigger points.

@Input          psDevInfo       pointer to device info
@Input          bIgnorePrevious If MT_TRUE, any stalled contexts will be
                                indicated immediately, rather than only
                                checking against any previous stalled contexts

@Return         None
******************************************************************************/
void MUSACheckForStalledClientContexts(MTGPU_MUSADEV_INFO *psDevInfo, MT_BOOL bIgnorePrevious);

/*!
*******************************************************************************
@Function       MUSAUpdateHealthStatus

@Description    Tests a number of conditions which might indicate a fatal error
                has occurred in the firmware. The result is stored in the
                device node eHealthStatus.

@Input         psDevNode        Pointer to device node structure.
@Input         bCheckAfterTimePassed  When TRUE, the function will also test
                                for firmware queues and polls not changing
                                since the previous test.

                                Note: if not enough time has passed since the
                                last call, false positives may occur.

@Return        MTGPU_ERROR
******************************************************************************/
MTGPU_ERROR MUSAUpdateHealthStatus(MTGPU_DEVICE_NODE* psDevNode,
                                   MT_BOOL bCheckAfterTimePassed);

#if defined(SUPPORT_AUTOVZ)
/*!
*******************************************************************************
@Function       MUSAUpdateAutoVzWatchdog

@Description    Updates AutoVz watchdog that maintains the fw-driver connection

@Input         psDevNode        Pointer to device node structure.
******************************************************************************/
void MUSAUpdateAutoVzWatchdog(MTGPU_DEVICE_NODE* psDevNode);
#endif /* SUPPORT_AUTOVZ */

/*!
*******************************************************************************
@Function       MUSAResetHWRLogs

@Description    Resets the HWR Logs buffer
                (the hardware recovery count is not reset)

@Input          psDevNode       Pointer to the device

@Return         MTGPU_ERROR    MTGPU_OK on success.
                                Otherwise, a MTGPU error code
******************************************************************************/
MTGPU_ERROR MUSAResetHWRLogs(MTGPU_DEVICE_NODE *psDevNode);

/*!
*******************************************************************************
@Function       MUSAGetPhyAddr

@Description    Get the physical address of a PMR at an offset within it

@Input          psPMR           PMR of the allocation
@Input          ui32LogicalOffset  Logical offset

@Output         psPhyAddr       Physical address of the allocation

@Return         MTGPU_ERROR    MTGPU_OK on success.
                                Otherwise, a MTGPU error code
******************************************************************************/
MTGPU_ERROR MUSAGetPhyAddr(PMR *psPMR,
						   MT_DEV_PHYADDR *psPhyAddr,
						   MT_UINT32 ui32LogicalOffset,
						   MT_UINT32 ui32Log2PageSize,
						   MT_UINT32 ui32NumOfPages,
						   MT_BOOL *bValid);


/*!
*******************************************************************************
@Function       MUSAFwRawHeapAllocMap

@Description    Register and maps to device, a raw firmware physheap

@Return         MTGPU_ERROR    MTGPU_OK on success.
                                Otherwise, a MTGPU error code
******************************************************************************/
MTGPU_ERROR MUSAFwRawHeapAllocMap(MTGPU_DEVICE_NODE *psDeviceNode,
								  MT_UINT32 ui32OSID,
								  MT_DEV_PHYADDR sDevPAddr,
								  MT_UINT64 ui64DevPSize);

/*!
*******************************************************************************
@Function       MUSAFwRawHeapUnmapFree

@Description    Unregister and unmap from device, a raw firmware physheap

******************************************************************************/
void MUSAFwRawHeapUnmapFree(MTGPU_DEVICE_NODE *psDeviceNode,
						   MT_UINT32 ui32OSID);

/* Just for vgpu env */
MTGPU_ERROR MUSAFwRawHeapAllocMultiMap(MTGPU_DEVICE_NODE *psDeviceNode,
							MT_UINT32 ui32OSID,
							MT_DEV_PHYADDR sDevPAddr,
							MT_UINT64 ui64DevPSize,
							DEVMEM_MEMDESC **fw_heap_memesc);

void MUSAFwRawHeapMultiUnmapFree(MTGPU_DEVICE_NODE *psDeviceNode,
						   MT_UINT32 ui32OSID,
						   DEVMEM_MEMDESC *fw_heap_memesc);

/*!
*******************************************************************************
@Function       MUSAReadFWModuleAddr

@Description    Read a value at the given address in MCU or RISCV memory space

@Input          psDevInfo       Pointer to device info
@Input          ui32Addr        Address in MCU or RISCV memory space

@Output         pui32Value      Read value

@Return         MTGPU_ERROR
******************************************************************************/
MTGPU_ERROR MUSAReadFWModuleAddr(MTGPU_MUSADEV_INFO *psDevInfo,
                                 MT_UINT32 ui32Addr,
                                 MT_UINT32 *pui32Value);

/*!
*******************************************************************************
@Function       MUSAWriteFWModuleAddr

@Description    Write a value to the given address in MCU or RISC memory space

@Input          psDevInfo       Pointer to device info
@Input          ui32Addr        Address in RISC-V memory space
@Input          ui32Value       Write value

@Return         MTGPU_ERROR
******************************************************************************/
MTGPU_ERROR MUSAWriteFWModuleAddr(MTGPU_MUSADEV_INFO *psDevInfo,
                                  MT_UINT32 ui32MemAddr,
                                  MT_UINT32 ui32Value);

/*!
*******************************************************************************
@Function       MUSAGetFwMapping

@Description    Retrieve any of the CPU Physical Address, Device Physical
                Address or the raw value of the page table entry associated
                with the firmware virtual address given.

@Input          psDevInfo       Pointer to device info
@Input          ui32FwVA        The Fw VA that needs decoding
@Output         psCpuPA         Pointer to the resulting CPU PA
@Output         psDevPA         Pointer to the resulting Dev PA
@Output         pui64RawPTE     Pointer to  the raw Page Table Entry value

@Return         MTGPU_ERROR
******************************************************************************/
MTGPU_ERROR MUSAGetFwMapping(MTGPU_MUSADEV_INFO *psDevInfo,
                                    MT_UINT32 ui32FwVA,
                                    MT_CPU_PHYADDR *psCpuPA,
                                    MT_DEV_PHYADDR *psDevPA,
                                    MT_UINT64 *pui64RawPTE);

/*************************************************************************/ /*!
@Function       MUSAKickFW

@Description    Kick the MTS to schedule firmware.

@Input          psDevInfo       Device Info struct
*/ /**************************************************************************/
void MUSAKickFW(MTGPU_MUSADEV_INFO *psDevInfo);


#if defined(SUPPORT_AUTOVZ_HW_REGS) && !defined(SUPPORT_AUTOVZ)
#error "VZ build configuration error: use of OS scratch registers supported only in AutoVz drivers."
#endif

#if defined(SUPPORT_AUTOVZ_HW_REGS)
/* AutoVz with hw support */
#define KM_GET_FW_CONNECTION(psDevInfo)				OSReadHWReg32(psDevInfo->pvRegsBaseKM, MUSA_CR_OS0_SCRATCH3)
#define KM_GET_OS_CONNECTION(psDevInfo)				OSReadHWReg32(psDevInfo->pvRegsBaseKM, MUSA_CR_OS0_SCRATCH2)
#define KM_SET_OS_CONNECTION(val, psDevInfo)		OSWriteHWReg32(psDevInfo->pvRegsBaseKM, MUSA_CR_OS0_SCRATCH2, MUSAFW_CONNECTION_OS_##val)

#define KM_GET_FW_ALIVE_TOKEN(psDevInfo)			OSReadHWReg32(psDevInfo->pvRegsBaseKM, MUSA_CR_OS0_SCRATCH1)
#define KM_GET_OS_ALIVE_TOKEN(psDevInfo)			OSReadHWReg32(psDevInfo->pvRegsBaseKM, MUSA_CR_OS0_SCRATCH0)
#define KM_SET_OS_ALIVE_TOKEN(val, psDevInfo)		OSWriteHWReg32(psDevInfo->pvRegsBaseKM, MUSA_CR_OS0_SCRATCH0, val)
#else

#if defined(SUPPORT_AUTOVZ)
#define KM_GET_FW_ALIVE_TOKEN(psDevInfo)			(psDevInfo->psMUSAFWIfConnectionCtl->ui32AliveFwToken)
#define KM_GET_OS_ALIVE_TOKEN(psDevInfo)			(psDevInfo->psMUSAFWIfConnectionCtl->ui32AliveOsToken)
#define KM_SET_OS_ALIVE_TOKEN(val, psDevInfo)		OSWriteDeviceMem32WithWMB(&psDevInfo->psMUSAFWIfConnectionCtl->ui32AliveOsToken, val)
#endif /* defined(SUPPORT_AUTOVZ) */

#if !defined(NO_HARDWARE) && (defined(MUSA_VZ_STATIC_CARVEOUT_FW_HEAPS) || (defined(MUSA_NUM_OS_SUPPORTED) && (MUSA_NUM_OS_SUPPORTED == 1)))
/* native, static-vz and AutoVz using shared memory */
#define KM_GET_FW_CONNECTION(psDevInfo)			(psDevInfo->psMUSAFWIfConnectionCtl->eConnectionFwState)
#define KM_GET_OS_CONNECTION(psDevInfo)			(psDevInfo->psMUSAFWIfConnectionCtl->eConnectionOsState)
#define KM_SET_OS_CONNECTION(val, psDevInfo)	OSWriteDeviceMem32WithWMB(&psDevInfo->psMUSAFWIfConnectionCtl->eConnectionOsState, MUSAFW_CONNECTION_OS_##val)
#else
/* dynamic-vz & nohw */
#define KM_GET_FW_CONNECTION(psDevInfo)			(MUSAFW_CONNECTION_FW_ACTIVE)
#define KM_GET_OS_CONNECTION(psDevInfo)			(MUSAFW_CONNECTION_OS_ACTIVE)
#define KM_SET_OS_CONNECTION(val, psDevInfo)
#endif /* defined(MUSA_VZ_STATIC_CARVEOUT_FW_HEAPS) || (MUSA_NUM_OS_SUPPORTED == 1) */
#endif /* defined(SUPPORT_AUTOVZ_HW_REGS) */

#if defined(SUPPORT_AUTOVZ)
#define MUSA_FIRST_RAW_HEAP_OSID		MUSAFW_HOST_OS
#else
#define MUSA_FIRST_RAW_HEAP_OSID		MUSAFW_GUEST_OSID_START
#endif

#define KM_OS_CONNECTION_IS(val, psDevInfo)		(KM_GET_OS_CONNECTION(psDevInfo) == MUSAFW_CONNECTION_OS_##val)
#define KM_FW_CONNECTION_IS(val, psDevInfo)		(KM_GET_FW_CONNECTION(psDevInfo) == MUSAFW_CONNECTION_FW_##val)

#endif /* MUSAFWUTILS_H */
/******************************************************************************
 End of file (musafwutils.h)
******************************************************************************/

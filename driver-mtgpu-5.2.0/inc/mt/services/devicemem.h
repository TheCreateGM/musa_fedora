/*************************************************************************/ /*!
@File
@Title          Device Memory Management core internal
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Services internal interface to core device memory management
                functions that are shared between client and server code.
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

#ifndef SRVCLIENT_DEVICEMEM_H
#define SRVCLIENT_DEVICEMEM_H

/******************************************************************************
 *                                                                            *
 *  +------------+   +------------+      +--------------+   +--------------+  *
 *  | a   sub-   |   | a   sub-   |      |  an          |   | allocation   |  *
 *  | allocation |   | allocation |      |  allocation  |   | also mapped  |  *
 *  |            |   |            |      |  in proc 1   |   | into proc 2  |  *
 *  +------------+   +------------+      +--------------+   +--------------+  *
 *            |         |                       |                  |          *
 *         +--------------+              +--------------+   +--------------+  *
 *         | page   gran- |              | page   gran- |   | page   gran- |  *
 *         | ular mapping |              | ular mapping |   | ular mapping |  *
 *         +--------------+              +--------------+   +--------------+  *
 *                |                                   |       |               *
 *                |                                   |       |               *
 *                |                                   |       |               *
 *         +--------------+                       +--------------+            *
 *         |              |                       |              |            *
 *         | A  "P.M.R."  |                       | A  "P.M.R."  |            *
 *         |              |                       |              |            *
 *         +--------------+                       +--------------+            *
 *                                                                            *
 ******************************************************************************/

/*
    All device memory allocations are ultimately a view upon (not
    necessarily the whole of) a "PMR".

    A PMR is a "Physical Memory Resource", which may be a
    "pre-faulted" lump of physical memory, or it may be a
    representation of some physical memory that will be instantiated
    at some future time.

    PMRs always represent multiple of some power-of-2 "contiguity"
    promised by the PMR, which will allow them to be mapped in whole
    pages into the device MMU.  As memory allocations may be smaller
    than a page, these mappings may be suballocated and thus shared
    between multiple allocations in one process.  A PMR may also be
    mapped simultaneously into multiple device memory contexts
    (cross-process scenario), however, for security reasons, it is not
    legal to share a PMR "both ways" at once, that is, mapped into
    multiple processes and divided up amongst several suballocations.

    This PMR terminology is introduced here for background
    information, but is generally of little concern to the caller of
    this API.  This API handles suballocations and mappings, and the
    caller thus deals primarily with MEMORY DESCRIPTORS representing
    an allocation or suballocation, HEAPS representing ranges of
    virtual addresses in a CONTEXT.
*/

/*
   |<---------------------------context------------------------------>|
   |<-------heap------->|   |<-------heap------->|<-------heap------->|
   |<-alloc->|          |   |<-alloc->|<-alloc->||   |<-alloc->|      |
*/

#include "img_types.h"
#include "img_defs.h"
#include "devicemem_typedefs.h"
#include "mtgpu_error.h"
#include "mtgpu_memallocflags.h"


#include "device_connection.h"


typedef MT_UINT32 DEVMEM_HEAPCFGID;
#define DEVMEM_HEAPCFG_FORCLIENTS 0
#define DEVMEM_HEAPCFG_MCU 1
#define DEVMEM_HEAPCFG_RESERVED 2

typedef struct _MTGPU_DEVICE_NODE_ MTGPU_DEVICE_NODE;


/*
  In order to call the server side functions, we need a bridge handle.
  We abstract that here, as we may wish to change its form.
 */

typedef MT_HANDLE DEVMEM_BRIDGE_HANDLE;

MT_INTERNAL void
DevmemGetAnnotation(DEVMEM_MEMDESC *psMemDesc,
			        MT_CHAR **pszAnnotation);

/*
 * DevmemCreateContext()
 *
 * Create a device memory context
 *
 * This must be called before any heap is created in this context
 *
 * Caller to provide bridge handle which will be recorded internally and used
 * for all future operations on items from this memory context.  Caller also
 * to provide devicenode handle, as this is used for MMU configuration and
 * also to determine the heap configuration for the auto-instantiated heaps.
 *
 * Note that when compiled in services/server, the hBridge is not used and
 * is thrown away by the "fake" direct bridge.  (This may change. It is
 * recommended that NULL be passed for the handle for now.)
 *
 * hDeviceNode and uiHeapBlueprintID shall together dictate which heap-config
 * to use.
 *
 * This will cause the server side counterpart to be created also.
 *
 * If you call DevmemCreateContext() (and the call succeeds) you are promising
 * that you will later call Devmem_ContextDestroy(), except for abnormal
 * process termination in which case it is expected it will be destroyed as
 * part of handle clean up.
 *
 * Caller to provide storage for the pointer to the newly created
 * NEWDEVMEM_CONTEXT object.
 */
MTGPU_ERROR
DevmemCreateContext(MTGPU_DEVICE_NODE *psDevNode,
                    DEVMEM_HEAPCFGID uiHeapBlueprintID,
                    DEVMEM_CONTEXT **ppsCtxPtr);

/*
 * DevmemDestroyContext()
 *
 * Undoes that done by DevmemCreateContext()
 */
MTGPU_ERROR
DevmemDestroyContext(DEVMEM_CONTEXT *psCtx);

/*
 * DevmemCreateHeap()
 *
 * Create a heap in the given context.
 *
 * N.B.  Not intended to be called directly, though it can be.
 * Normally, heaps are instantiated at context creation time according
 * to the specified blueprint.  See DevmemCreateContext() for details.
 *
 * This will cause MMU code to set up data structures for the heap,
 * but may not cause page tables to be modified until allocations are
 * made from the heap.
 *
 * uiReservedRegionLength Reserved address space for static VAs shared
 * between clients and firmware
 *
 * The "Quantum" is both the device MMU page size to be configured for
 * this heap, and the unit multiples of which "quantized" allocations
 * are made (allocations smaller than this, known as "suballocations"
 * will be made from a "sub alloc RA" and will "import" chunks
 * according to this quantum)
 *
 * Where imported PMRs (or, for example, PMRs created by device class
 * buffers) are mapped into this heap, it is important that the
 * physical contiguity guarantee offered by the PMR is greater than or
 * equal to the quantum size specified here, otherwise the attempt to
 * map it will fail.  "Normal" allocations via Devmem_Allocate
 * shall automatically meet this requirement, as each "import" will
 * trigger the creation of a PMR with the desired contiguity.  The
 * supported quantum sizes in that case shall be dictated by the OS
 * specific implementation of PhysmemNewOSRamBackedPMR() (see)
 */
MTGPU_ERROR
DevmemCreateHeap(DEVMEM_CONTEXT *psCtxPtr,
                 /* base and length of heap */
                 uint64_t sBaseAddress,
                 MT_DEVMEM_SIZE_T uiLength,
                 MT_DEVMEM_SIZE_T uiReservedRegionLength,
                 /* log2 of allocation quantum, i.e. "page" size.
                    All allocations (that go to server side) are
                    multiples of this.  We use a client-side RA to
                    make sub-allocations from this */
                 MT_UINT32 ui32Log2Quantum,
                 /* The minimum import alignment for this heap */
                 MT_UINT32 ui32Log2ImportAlignment,
                 /* Name of heap for debug */
                 /* N.B.  Okay to exist on caller's stack - this
                    func takes a copy if it needs it. */
                 const MT_CHAR *pszName,
                 DEVMEM_HEAPCFGID uiHeapBlueprintID,
                 DEVMEM_HEAP **ppsHeapPtr);
/*
 * DevmemDestroyHeap()
 *
 * Reverses DevmemCreateHeap()
 *
 * N.B. All allocations must have been freed and all mappings must
 * have been unmapped before invoking this call
 */
MTGPU_ERROR
DevmemDestroyHeap(DEVMEM_HEAP *psHeap);

/*
 * DevmemExportalignAdjustSizeAndAlign()
 * Compute the Size and Align passed to avoid suballocations
 * (used when allocation with MTGPU_MEMALLOCFLAG_EXPORTALIGN).
 *
 * Returns MTGPU_ERROR_INVALID_PARAMS if uiLog2Quantum has invalid value.
 */
MT_INTERNAL MTGPU_ERROR
DevmemExportalignAdjustSizeAndAlign(MT_UINT32 uiLog2Quantum,
                                    MT_DEVMEM_SIZE_T *puiSize,
                                    MT_DEVMEM_ALIGN_T *puiAlign);

/*
 * DevmemSubAllocate()
 *
 * Makes an allocation (possibly a "suballocation", as described
 * below) of device virtual memory from this heap.
 *
 * The size and alignment of the allocation will be honoured by the RA
 * that allocates the "suballocation".  The resulting allocation will
 * be mapped into GPU virtual memory and the physical memory to back
 * it will exist, by the time this call successfully completes.
 *
 * The size must be a positive integer multiple of the alignment.
 * (i.e. the alignment specifies the alignment of both the start and
 * the end of the resulting allocation.)
 *
 * Allocations made via this API are routed through a "suballocation
 * RA" which is responsible for ensuring that small allocations can be
 * made without wasting physical memory in the server.  Furthermore,
 * such suballocations can be made entirely client side without
 * needing to go to the server unless the allocation spills into a new
 * page.
 *
 * Such suballocations cause many allocations to share the same "PMR".
 * This happens only when the flags match exactly.
 *
 */

MTGPU_ERROR
DevmemSubAllocate(MT_UINT8 uiPreAllocMultiplier,
                  DEVMEM_HEAP *psHeap,
                  MT_DEVMEM_SIZE_T uiSize,
                  MT_DEVMEM_ALIGN_T uiAlign,
                  MTGPU_MEMALLOCFLAGS_T uiFlags,
                  const MT_CHAR *pszText,
                  DEVMEM_MEMDESC **ppsMemDescPtr);

#define DevmemAllocate(...) \
    DevmemSubAllocate(DEVMEM_NO_PRE_ALLOCATE_MULTIPLIER, __VA_ARGS__)

MTGPU_ERROR
DevmemAllocateExportable(MTGPU_DEVICE_NODE *psDevNode,
                         MT_DEVMEM_SIZE_T uiSize,
                         MT_DEVMEM_ALIGN_T uiAlign,
                         MT_UINT32 uiLog2HeapPageSize,
                         MTGPU_MEMALLOCFLAGS_T uiFlags,
                         const MT_CHAR *pszText,
                         DEVMEM_MEMDESC **ppsMemDescPtr);

MTGPU_ERROR
DevmemSubAllocateAndMap(MT_UINT8 uiPreAllocMultiplier,
			DEVMEM_HEAP *psHeap,
			MT_DEVMEM_SIZE_T uiSize,
			MT_DEVMEM_ALIGN_T uiAlign,
			MTGPU_MEMALLOCFLAGS_T uiFlags,
			const MT_CHAR *pszText,
			DEVMEM_MEMDESC **ppsMemDescPtr,
			uint64_t *psDevVirtAddr);

#define DevmemAllocateAndMap(...) \
	DevmemSubAllocateAndMap(DEVMEM_NO_PRE_ALLOCATE_MULTIPLIER, __VA_ARGS__)

/*
 * DevmemFree()
 *
 * Reverses that done by DevmemSubAllocate() N.B.  The underlying
 * mapping and server side allocation _may_ not be torn down, for
 * example, if the allocation has been exported, or if multiple
 * allocations were suballocated from the same mapping, but this is
 * properly refcounted, so the caller does not have to care.
 */

MT_BOOL
DevmemFree(DEVMEM_MEMDESC *psMemDesc);

/*
	DevmemMapToDevice:

	Map an allocation to the device it was allocated from.
	This function _must_ be called before any call to
	DevmemAcquireDevVirtAddr is made as it binds the allocation
	to the heap.
	DevmemReleaseDevVirtAddr is used to release the reference
	to the device mapping this function created, but it doesn't
	mean that the memory will actually be unmapped from the
	device as other references to the mapping obtained via
	DevmemAcquireDevVirtAddr could still be active.
*/
MTGPU_ERROR DevmemMapToDevice(DEVMEM_MEMDESC *psMemDesc,
							   DEVMEM_HEAP *psHeap,
							   uint64_t *psDevVirtAddr);

void DevmemUnmapFromDevice(DEVMEM_MEMDESC *psMemDesc);

/*
	DevmemGetDevVirtAddr

	Obtain the MemDesc's device virtual address.
	This function _must_ be called after DevmemMapToDevice(Address)
	and is expected to be used be functions which didn't allocate
	the MemDesc but need to know it's address.
	It will MT_ASSERT if no device mapping exists and 0 is returned.
 */
uint64_t
DevmemGetDevVirtAddr(DEVMEM_MEMDESC *psMemDesc);

/*
	DevmemAcquireDevVirtAddr

	Acquire the MemDesc's device virtual address.
	This function _must_ be called after DevmemMapToDevice
	and is expected to be used be functions which didn't allocate
	the MemDesc but need to know it's address
 */
MTGPU_ERROR DevmemAcquireDevVirtAddr(DEVMEM_MEMDESC *psMemDesc,
                                      uint64_t *psDevVirtAddrRet);

/*
 * DevmemReleaseDevVirtAddr()
 *
 * give up the licence to use the device virtual address that was
 * acquired by "Acquire" or "MapToDevice"
 */
void
DevmemReleaseDevVirtAddr(DEVMEM_MEMDESC *psMemDesc);

/*
 * DevmemAcquireCpuVirtAddr()
 *
 * Acquires a license to use the cpu virtual address of this mapping.
 * Note that the memory may not have been mapped into cpu virtual
 * memory prior to this call.  On first "acquire" the memory will be
 * mapped in (if it wasn't statically mapped in) and on last put it
 * _may_ become unmapped.  Later calling "Acquire" again, _may_ cause
 * the memory to be mapped at a different address.
 */
MTGPU_ERROR DevmemAcquireCpuVirtAddr(DEVMEM_MEMDESC *psMemDesc,
                                      void **ppvCpuVirtAddr);

/*
 * DevmemReleaseDevVirtAddr()
 *
 * give up the licence to use the cpu virtual address that was granted
 * with the "Get" call.
 */
void
DevmemReleaseCpuVirtAddr(DEVMEM_MEMDESC *psMemDesc);

#if defined(SUPPORT_INSECURE_EXPORT)
/*
 * DevmemExport()
 *
 * Given a memory allocation allocated with DevmemAllocateExportable()
 * create a "cookie" that can be passed intact by the caller's own choice
 * of secure IPC to another process and used as the argument to "map"
 * to map this memory into a heap in the target processes.  N.B.  This can
 * also be used to map into multiple heaps in one process, though that's not
 * the intention.
 *
 * Note, the caller must later call Unexport before freeing the
 * memory.
 */
MTGPU_ERROR DevmemExport(DEVMEM_MEMDESC *psMemDesc,
                          DEVMEM_EXPORTCOOKIE *psExportCookie);


void DevmemUnexport(DEVMEM_MEMDESC *psMemDesc,
					DEVMEM_EXPORTCOOKIE *psExportCookie);

MTGPU_ERROR
DevmemImport(MTGPU_DEVICE_NODE *psDevNode,
			 DEVMEM_EXPORTCOOKIE *psCookie,
			 MTGPU_MEMALLOCFLAGS_T uiFlags,
			 DEVMEM_MEMDESC **ppsMemDescPtr);
#endif /* SUPPORT_INSECURE_EXPORT */

/*
 * Devmem_FindHeapByName()
 *
 * returns the heap handle for the named _automagic_ heap in this
 * context.  "automagic" heaps are those that are born with the
 * context from a blueprint
 */
MTGPU_ERROR
DevmemFindHeapByName(const DEVMEM_CONTEXT *psCtx,
                     const MT_CHAR *pszHeapName,
                     DEVMEM_HEAP **ppsHeapRet);

MTGPU_ERROR
DevmemLocalGetImportHandle(DEVMEM_MEMDESC *psMemDesc,
			   MT_HANDLE *phImport);

MT_INTERNAL void
DevmemGetPMRData(DEVMEM_MEMDESC *psMemDesc,
		MT_HANDLE *hPMR,
		MT_DEVMEM_OFFSET_T *puiPMROffset);

MT_INTERNAL MTGPU_DEVICE_NODE *
DevmemGetConnection(DEVMEM_MEMDESC *psMemDesc);

MTGPU_ERROR
DevmemLocalImport(MTGPU_DEVICE_NODE *psDevNode,
				  MT_HANDLE hExtHandle,
				  MTGPU_MEMALLOCFLAGS_T uiFlags,
				  DEVMEM_MEMDESC **ppsMemDescPtr,
				  MT_DEVMEM_SIZE_T *puiSizePtr,
				  const MT_CHAR *pszAnnotation);

/* DevmemGetHeapLog2PageSize()
 *
 * Get the page size used for a certain heap.
 */
MT_UINT32
DevmemGetHeapLog2PageSize(DEVMEM_HEAP *psHeap);

/*************************************************************************/ /*!
@Function       DevmemHeapSetPremapStatus
@Description    In some special cases like virtualisation, a device memory heap
			    must be entirely backed by physical memory and mapped into the
				device's virtual address space. This is done at context creation.
			    When objects are allocated from such a heap, the mapping part
			    must be skipped. The 'bPremapped' flag dictates if allocations
			    are to be mapped or not.

@Input          psHeap            Device memory heap to be updated
@Input          IsPremapped       The premapping status to be set
*/ /**************************************************************************/
MT_INTERNAL void
DevmemHeapSetPremapStatus(DEVMEM_HEAP *psHeap, MT_BOOL IsPremapped);

#endif /* #ifndef SRVCLIENT_DEVICEMEM_H */

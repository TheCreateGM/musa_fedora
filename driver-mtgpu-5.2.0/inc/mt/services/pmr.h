/*************************************************************************/ /*!
@File
@Title          Physmem (PMR) abstraction
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Part of the memory management.  This module is responsible for
                the "PMR" abstraction.  A PMR (Physical Memory Resource)
                represents some unit of physical memory which is
                allocated/freed/mapped/unmapped as an indivisible unit
                (higher software levels provide an abstraction above that
                to deal with dividing this down into smaller manageable units).
                Importantly, this module knows nothing of virtual memory, or
                of MMUs etc., with one excusable exception.  We have the
                concept of a "page size", which really means nothing in
                physical memory, but represents a "contiguity quantum" such
                that the higher level modules which map this memory are able
                to verify that it matches the needs of the page size for the
                virtual realm into which it is being mapped.
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

#ifndef SRVSRV_PMR_H
#define SRVSRV_PMR_H

/* include/ */
#include "img_types.h"
#include "img_defs.h"
#include "mtgpu_error.h"
#include "mtgpu_memallocflags.h"
#include "devicemem_typedefs.h"			/* Required for export DEVMEM_EXPORTCOOKIE */
#include "osfunc_common.h"

/* services/include */
#include "physheap.h"

/* services/server/include/ */
#include "pmr_impl.h"
#include "opaque_types.h"

#define PMR_MAX_TRANSLATION_STACK_ALLOC				(32)

extern u32 gpu_page_size;
extern u32 gpu_page_shift;

typedef MT_UINT64 PMR_BASE_T;
typedef MT_UINT64 PMR_SIZE_T;
#define PMR_SIZE_FMTSPEC "0x%010"MT_UINT64_FMTSPECX
#define PMR_VALUE32_FMTSPEC "0x%08X"
#define PMR_VALUE64_FMTSPEC "0x%016"MT_UINT64_FMTSPECX
typedef MT_UINT32 PMR_LOG2ALIGN_T;
typedef MT_UINT64 PMR_PASSWORD_T;

struct _PMR_MAPPING_TABLE_
{
	PMR_SIZE_T	uiChunkSize;			/*!< Size of a "chunk" */
	MT_UINT32	ui32NumPhysChunks;		/*!< Number of physical chunks that are valid */
	MT_UINT32	ui32NumVirtChunks;		/*!< Number of virtual chunks in the mapping */
	/* Must be last */
	MT_UINT32	aui32Translation[1];	/*!< Translation mapping for "logical" to physical */
};

#define TRANSLATION_INVALID 0xFFFFFFFFUL

typedef struct _PMR_EXPORT_ PMR_EXPORT;

typedef struct _PMR_PAGELIST_ PMR_PAGELIST;

struct _MTGPU_DEVICE_NODE_;

typedef struct _MTGPU_DEVICE_NODE_ MTGPU_DEVICE_NODE;

/**
 * struct PMR_PA_INFO the struct for obtaining the physical address of the gpu
 * @psDevNode:			a node representing the gpu card.
 * @ui32Log2PageSize:		the gpu page size expressed in base 2.
 * @ui32NumOfPages:		requested number of gpu physical pages.
 * @uiLogicalOffset:		the logical offset of the requested data.
 * @psDevPAddr:			store the physical address of the device in the form of an array.
 * @psDmaAddr:			save the temporary data in the process of obtaining the physical address,
 *				which may store the iova address or the cpu physical address.
 * @pbValid:			save whether the element value corresponding to the
 *				index of the device address (psDevPAddr) is valid.
 * @uiMappingFlags:		mapping flag is used to control how it's mapped.
 * @ui32MapPageStart:		the starting index of mapping pages.
 * @ui32MapPageCount:		total number of pages to be mapped for this time.
 * @ui32InterleaveStep:		An interleaved period represents the number of mapping pages.
 * @psInterleaveRatio:		a struct represents the interleave ratio.
 * This structure holds the information required to export device physical address. Used
 * with PMR_CreatePAInfo() only.
 */
struct PMR_PA_INFO
{
	MTGPU_DEVICE_NODE *psDevNode;
	MT_UINT32 ui32Log2PageSize;
	MT_UINT32 ui32NumOfPages;
	MT_DEVMEM_OFFSET_T uiLogicalOffset;
	MT_DEV_PHYADDR *psDevPAddr;
	MT_CPU_PHYADDR *psDmaAddr;
	MT_BOOL *pbValid;
	MT_UINT32 *pui32ConPageNumArray;
	MT_UINT64 uiMappingFlags;
	MT_UINT32 ui32MapPageStart;
	MT_UINT32 ui32MapPageCount;
	MT_UINT32 ui32InterleaveStep;
	DEVMEM_INTERLEAVE_RATIO *psInterleaveRatio;
};

struct PMR_STACK_FRAME {
	MT_UINT32 ui32RefCountPrev;
	MT_UINT32 ui32RefCountNext;
	MT_UINT32 ui32FrameCnt;
	MT_ULONG  ulFrameEnties[10];
};

struct PMR_STACK_INFO {
	MT_INT32 i32StacRefCnt;
	struct PMR_STACK_FRAME *psPmrRefStacks[64];
};

typedef struct PMR_PA_INFO PMR_PA_INFO;

/*
 * PMRCreatePMR
 *
 * Not to be called directly, only via implementations of PMR
 * factories, e.g. in physmem_osmem.c, deviceclass.c, etc.
 *
 * Creates a PMR object, with callbacks and private data as per the
 * FuncTab/PrivData args.
 *
 * Note that at creation time the PMR must set in stone the "logical
 * size" and the "contiguity guarantee"
 *
 * Flags are also set at this time.  (T.B.D.  flags also immutable for
 * the life of the PMR?)
 *
 * Logical size is the amount of Virtual space this allocation would
 * take up when mapped.  Note that this does not have to be the same
 * as the actual physical size of the memory.  For example, consider
 * the sparsely allocated non-power-of-2 texture case.  In this
 * instance, the "logical size" would be the virtual size of the
 * rounded-up power-of-2 texture.  That some pages of physical memory
 * may not exist does not affect the logical size calculation.
 *
 * The PMR must also supply the "contiguity guarantee" which is the
 * finest granularity of alignment and size of physical pages that the
 * PMR will provide after LockSysPhysAddresses is called.  Note that
 * the calling code may choose to call PMRSysPhysAddr with a finer
 * granularity than this, for example if it were to map into a device
 * MMU with a smaller page size, and it's also OK for the PMR to
 * supply physical memory in larger chunks than this.  But
 * importantly, never the other way around.
 *
 * More precisely, the following inequality must be maintained
 * whenever mappings and/or physical addresses exist:
 *
 *       (device MMU page size) <= 2**(uiLog2ContiguityGuarantee) <= (actual contiguity of physical memory)
 *
 * The function table will contain the following callbacks which may
 * be overridden by the PMR implementation:
 *
 * pfnLockPhysAddresses
 *
 *      Called when someone locks requests that Physical pages are to
 *      be locked down via the PMRLockSysPhysAddresses() API.  Note
 *      that if physical pages are prefaulted at PMR creation time and
 *      therefore static, it would not be necessary to override this
 *      function, in which case NULL may be supplied.
 *
 * pfnUnlockPhysAddresses
 *
 *      The reverse of pfnLockPhysAddresses.  Note that this should be
 *      NULL if and only if pfnLockPhysAddresses is NULL
 *
 * pfnSysPhysAddr
 *
 *      This function is mandatory.  This is the one which returns the
 *      system physical address for a given offset into this PMR.  The
 *      "lock" function will have been called, if overridden, before
 *      this function, thus the implementation should not increase any
 *      refcount when answering this call.  Refcounting, if necessary,
 *      should be done in the lock/unlock calls.  Refcounting would
 *      not be necessary in the prefaulted/static scenario, as the
 *      pmr.c abstraction will handle the refcounting for the whole
 *      PMR.
 *
 * pfnFinalize
 *
 *      Called when the PMR's refcount reaches zero and it gets
 *      destroyed.  This allows the implementation to free up any
 *      resource acquired during creation time.
 *
 */
MTGPU_ERROR
PMRCreatePMR(PHYS_HEAP *psPhysHeap,
             PMR_SIZE_T uiLogicalSize,
             PMR_SIZE_T uiChunkSize,
             MT_UINT32 ui32NumPhysChunks,
             MT_UINT32 ui32NumVirtChunks,
             MT_UINT32 *pui32MappingTable,
             PMR_LOG2ALIGN_T uiLog2ContiguityGuarantee,
             PMR_FLAGS_T uiFlags,
             const MT_CHAR *pszAnnotation,
             const PMR_IMPL_FUNCTAB *psFuncTab,
             PMR_IMPL_PRIVDATA pvPrivData,
             PMR_IMPL_TYPE eType,
             PMR **ppsPMRPtr,
             MT_UINT32 ui32PDumpFlags);

/*
 * PMRLockSysPhysAddresses()
 *
 * Calls the relevant callback to lock down the system physical addresses of
 * the memory that makes up the whole PMR.
 *
 * Before this call, it is not valid to use any of the information
 * getting APIs: PMR_Flags(), PMR_SysPhysAddr(),
 * [ see note below about lock/unlock semantics ]
 *
 * The caller of this function does not have to care about how the PMR
 * is implemented.  He only has to know that he is allowed access to
 * the physical addresses _after_ calling this function and _until_
 * calling PMRUnlockSysPhysAddresses().
 *
 *
 * Notes to callback implementers (authors of PMR Factories):
 *
 * Some PMR implementations will be such that the physical memory exists for
 * the lifetime of the PMR, with a static address, (and normally flags and
 * symbolic address are static too) and so it is legal for a PMR
 * implementation to not provide an implementation for the lock callback.
 *
 * Some PMR implementation may wish to page memory in from secondary storage
 * on demand. The lock/unlock callbacks _may_ be the place to do this.
 * (More likely, there would be a separate API for doing this, but this API
 * provides a useful place to assert that it has been done)
 */

MTGPU_ERROR
PMRLockSysPhysAddresses(PMR *psPMR);

MTGPU_ERROR
PMRLockSysPhysAddressesNested(PMR *psPMR,
                        MT_UINT32 ui32NestingLevel);

/*
 * PMRUnlockSysPhysAddresses()
 *
 * the reverse of PMRLockSysPhysAddresses()
 */
MTGPU_ERROR
PMRUnlockSysPhysAddresses(PMR *psPMR);

MTGPU_ERROR
PMRUnlockSysPhysAddressesNested(PMR *psPMR, MT_UINT32 ui32NestingLevel);


/*************************************************************************/ /*!
@Function       PMRUnpinPMR
@Description    This is the counterpart to PMRPinPMR(). It is meant to be
                called before repinning an allocation.

                For a detailed description see client API documentation.

@Input          psPMR           The physical memory to unpin.

@Input          bDevMapped      A flag that indicates if this PMR has been
                                mapped to device virtual space.
                                Needed to check if this PMR is allowed to be
                                unpinned or not.

@Return         MTGPU_ERROR:   MTGPU_OK on success and the memory is
                                registered to be reclaimed. Error otherwise.
*/ /**************************************************************************/
MTGPU_ERROR PMRUnpinPMR(PMR *psPMR, MT_BOOL bDevMapped);

/*************************************************************************/ /*!
@Function       PMRPinPMR
@Description    This is the counterpart to PMRUnpinPMR(). It is meant to be
                called after unpinning an allocation.

                For a detailed description see client API documentation.

@Input          psPMR           The physical memory to pin.

@Return         MTGPU_ERROR:   MTGPU_OK on success and the allocation content
                                was successfully restored.

                                MTGPU_ERROR_PMR_NEW_MEMORY when the content
                                could not be restored and new physical memory
                                was allocated.

                                A different error otherwise.
*/ /**************************************************************************/
MTGPU_ERROR PMRPinPMR(PMR *psPMR);

#if !defined(NO_HARDWARE)
MTGPU_ERROR PMRAddMetaData(struct _CONNECTION_DATA_ *psConnection,
			    MT_HANDLE hPMR,
			    MT_UINT64 ui64MetaDataId,
			    MT_BYTE *pui8MetaData,
			    MT_UINT32 ui32DataSize);
MTGPU_ERROR
GEMAddMetaData(MT_UINT32 ui32CardFd,
	       MT_UINT32 ui32GemHandle,
	       MT_UINT64 ui64MetaDataId,
	       MT_BYTE *pui8MetaData,
	       MT_UINT32 ui32DataSize);
#endif

/*
 * PhysmemPMRExport()
 *
 * Given a PMR, creates a PMR "Export", which is a handle that
 * provides sufficient data to be able to "import" this PMR elsewhere.
 * The PMR Export is an object in its own right, whose existence
 * implies a reference on the PMR, thus the PMR cannot be destroyed
 * while the PMR Export exists.  The intention is that the PMR Export
 * will be wrapped in the devicemem layer by a cross process handle,
 * and some IPC by which to communicate the handle value and password
 * to other processes.  The receiving process is able to unwrap this
 * to gain access to the same PMR Export in this layer, and, via
 * PhysmemPMRImport(), obtain a reference to the original PMR.
 *
 * The caller receives, along with the PMR Export object, information
 * about the size and contiguity guarantee for the PMR, and also the
 * PMRs secret password, in order to authenticate the subsequent
 * import.
 *
 * N.B.  If you call PMRExportPMR() (and it succeeds), you are
 * promising to later call PMRUnexportPMR()
 */
MTGPU_ERROR
PMRExportPMR(PMR *psPMR,
             PMR_EXPORT **ppsPMRExport,
             PMR_SIZE_T *puiSize,
             PMR_LOG2ALIGN_T *puiLog2Contig,
             PMR_PASSWORD_T *puiPassword);

void PMRExportSetPriv(PMR_EXPORT *psPMRExport, void *pvPrivData);

void *PMRExportGetPriv(PMR_EXPORT *psPMRExport);

/*!
*******************************************************************************

 @Function      PMRMakeLocalImportHandle

 @Description

 Transform a general handle type into one that we are able to import.
 Takes a PMR reference.

 @Input   psPMR     The input PMR.
 @Output  ppsPMR    The output PMR that is going to be transformed to the
                    correct handle type.

 @Return   MTGPU_ERROR

******************************************************************************/
MTGPU_ERROR
PMRMakeLocalImportHandle(PMR *psPMR,
                         PMR **ppsPMR);

/*!
*******************************************************************************

 @Function      PMRUnmakeLocalImportHandle

 @Description

 Take a PMR, destroy the handle and release a reference.
 Counterpart to PMRMakeServerExportClientExport().

 @Input   psPMR       PMR to destroy.
                      Created by PMRMakeLocalImportHandle().

 @Return   MTGPU_ERROR

******************************************************************************/
MTGPU_ERROR
PMRUnmakeLocalImportHandle(PMR *psPMR);

/*
 * PMRUnexporPMRt()
 *
 * The reverse of PMRExportPMR().  This causes the PMR to no longer be
 * exported.  If the PMR has already been imported, the imported PMR
 * reference will still be valid, but no further imports will be possible.
 */
MTGPU_ERROR
PMRUnexportPMR(PMR_EXPORT *psPMRExport);

/*
 * PMRImportPMR()
 *
 * Takes a PMR Export object, as obtained by PMRExportPMR(), and
 * obtains a reference to the original PMR.
 *
 * The password must match, and is assumed to have been (by whatever
 * means, IPC etc.) preserved intact from the former call to
 * PMRExportPMR()
 *
 * The size and contiguity arguments are entirely irrelevant for the
 * import, however they are verified in order to trap bugs.
 *
 * N.B.  If you call PhysmemPMRImport() (and it succeeds), you are
 * promising to later call PhysmemPMRUnimport()
 */
MTGPU_ERROR
PMRImportPMR(PMR_EXPORT *psPMRExport,
             PMR_PASSWORD_T uiPassword,
             PMR_SIZE_T uiSize,
             PMR_LOG2ALIGN_T uiLog2Contig,
             PMR **ppsPMR);

/* Function that alters the mutability property
 * of the PMR
 * Setting it to TRUE makes sure the PMR memory layout
 * can't be changed through future calls */
void
PMR_SetLayoutFixed(PMR *psPMR, MT_BOOL bFlag);

MT_BOOL PMR_IsMemLayoutFixed(PMR *psPMR);

/*
 * PMRUnimportPMR()
 *
 * releases the reference on the PMR as obtained by PMRImportPMR()
 */
MTGPU_ERROR
PMRUnimportPMR(PMR *psPMR);

MTGPU_ERROR
PMRLocalImportPMR(PMR *psPMR,
                  PMR **ppsPMR,
                  MT_DEVMEM_SIZE_T *puiSize,
                  MT_DEVMEM_ALIGN_T *puiAlign);

/*
 * Equivalent mapping functions when in kernel mode.
 */
MTGPU_ERROR
PMRAcquireKernelMappingData(PMR *psPMR,
                            size_t uiLogicalOffset,
                            size_t uiSize,
                            void **ppvKernelAddressOut,
                            size_t *puiLengthOut,
                            MT_HANDLE *phPrivOut);

MTGPU_ERROR
PMRAcquireSparseKernelMappingData(PMR *psPMR,
                                  size_t uiLogicalOffset,
                                  size_t uiSize,
                                  void **ppvKernelAddressOut,
                                  size_t *puiLengthOut,
                                  MT_HANDLE *phPrivOut);

MTGPU_ERROR
PMRReleaseKernelMappingData(PMR *psPMR,
                            MT_HANDLE hPriv);

/*
 * PMR_ReadBytes()
 *
 * calls into the PMR implementation to read up to uiBufSz bytes,
 * returning the actual number read in *puiNumBytes
 *
 * this will read up to the end of the PMR, or the next symbolic name
 * boundary, or until the requested number of bytes is read, whichever
 * comes first
 *
 * In the case of sparse PMR's the caller doesn't know what offsets are
 * valid and which ones aren't so we will just write 0 to invalid offsets
 */
MTGPU_ERROR
PMR_ReadBytes(PMR *psPMR,
              MT_DEVMEM_OFFSET_T uiLogicalOffset,
              MT_UINT8 *pcBuffer,
              size_t uiBufSz,
              size_t *puiNumBytes);

/*
 * PMR_WriteBytes()
 *
 * calls into the PMR implementation to write up to uiBufSz bytes,
 * returning the actual number read in *puiNumBytes
 *
 * this will write up to the end of the PMR, or the next symbolic name
 * boundary, or until the requested number of bytes is written, whichever
 * comes first
 *
 * In the case of sparse PMR's the caller doesn't know what offsets are
 * valid and which ones aren't so we will just ignore data at invalid offsets
 */
MTGPU_ERROR
PMR_WriteBytes(PMR *psPMR,
               MT_DEVMEM_OFFSET_T uiLogicalOffset,
               MT_UINT8 *pcBuffer,
               size_t uiBufSz,
               size_t *puiNumBytes);

/*************************************************************************/ /*!
@Function       PMRMMapPMR
@Description    Performs the necessary steps to map the PMR into a user process
                address space. The caller does not need to call
                PMRLockSysPhysAddresses before calling this function.

@Input          psPMR           PMR to map.

@Input          pOSMMapData     OS specific data needed to create a mapping.

@Return         MTGPU_ERROR:   MTGPU_OK on success or an error otherwise.
*/ /**************************************************************************/
MTGPU_ERROR
PMRMMapPMR(PMR *psPMR, PMR_MMAP_DATA pOSMMapData);

/*
 * PMRRefPMR()
 *
 * Take a reference on the passed in PMR
 */
void
PMRRefPMR(PMR *psPMR);

/*
 * PMRUnrefPMR()
 *
 * This undoes a call to any of the PhysmemNew* family of APIs
 * (i.e. any PMR factory "constructor")
 *
 * This relinquishes a reference to the PMR, and, where the refcount
 * reaches 0, causes the PMR to be destroyed (calling the finalizer
 * callback on the PMR, if there is one)
 */
MTGPU_ERROR
PMRUnrefPMR(PMR *psPMR);

/*
 * PMRUnrefUnlockPMR()
 *
 * Same as above but also unlocks the PMR.
 */
MTGPU_ERROR
PMRUnrefUnlockPMR(PMR *psPMR);

PMTGPU_DEVICE_NODE
PMR_DeviceNode(const PMR *psPMR);

/*
 * PMRIsPMRLive()
 *
 * This function returns true if the PMR is in use and false otherwise.
 * This function is not thread safe and hence the caller needs to ensure the
 * thread safety by explicitly taking PMR or through other means.
 */
MT_BOOL PMRIsPMRLive(PMR *psPMR);

/*
 * PMR_Flags()
 *
 * Flags are static and guaranteed for the life of the PMR.  Thus this
 * function is idempotent and acquire/release semantics is not required.
 *
 * Returns the flags as specified on the PMR.  The flags are to be
 * interpreted as mapping permissions
 */
PMR_FLAGS_T
PMR_Flags(const PMR *psPMR);

MT_BOOL
PMR_IsSparse(const PMR *psPMR);

MT_BOOL
PMR_IsSystem(const PMR *psPMR);

MT_BOOL
PMR_IsUser(const PMR *psPMR);

MT_BOOL
PMR_IsVram(const PMR *psPMR);

MT_BOOL
PMR_IsUnpinned(const PMR *psPMR);

void
PMR_LogicalSize(const PMR *psPMR,
				MT_DEVMEM_SIZE_T *puiLogicalSize);

MTGPU_ERROR
PMR_PhysicalSize(const PMR *psPMR,
				 MT_DEVMEM_SIZE_T *puiPhysicalSize);

PHYS_HEAP *
PMR_PhysHeap(const PMR *psPMR);

PMR_MAPPING_TABLE *
PMR_GetMappingTable(const PMR *psPMR);

MT_UINT32
PMR_GetLog2Contiguity(const PMR *psPMR);

const MT_CHAR *
PMR_GetAnnotation(const PMR *psPMR);

/*
 * PMR_IsOffsetValid()
 *
 * Returns if an address offset inside a PMR has a valid
 * physical backing.
 */
MTGPU_ERROR
PMR_IsOffsetValid(const PMR *psPMR,
				MT_UINT32 ui32Log2PageSize,
				MT_UINT32 ui32NumOfPages,
				MT_DEVMEM_OFFSET_T uiLogicalOffset,
				MT_BOOL *pbValid);

PMR_IMPL_TYPE
PMR_GetType(const PMR *psPMR);

MT_INT32
PMR_GetRefCount(const PMR *psPMR);

/*
 * PMR_DevPhysAddr()
 *
 * A note regarding Lock/Unlock semantics
 * ======================================
 *
 * PMR_DevPhysAddr may only be called after PMRLockSysPhysAddresses()
 * has been called.  The data returned may be used only until
 * PMRUnlockSysPhysAddresses() is called after which time the licence
 * to use the data is revoked and the information may be invalid.
 *
 * Given an offset, this function returns the device physical address of the
 * corresponding page in the PMR.  It may be called multiple times
 * until the address of all relevant pages has been determined.
 *
 * If caller only wants one physical address it is sufficient to pass in:
 * ui32Log2PageSize==0 and ui32NumOfPages==1
 */
MTGPU_ERROR
PMR_DevPhysAddr(const PMR *psPMR,
		MT_UINT32 ui32Log2PageSize,
		MT_UINT32 ui32NumOfPages,
		MT_DEVMEM_OFFSET_T uiLogicalOffset,
		MT_DEV_PHYADDR *psDevAddr,
		MT_BOOL *pbValid);

MTGPU_ERROR
PMR_GetUnifiedDevPhysAddr(const PMR *psPMR, PMR_PA_INFO *psPMRPAInfo);

MTGPU_ERROR
PMR_UserPhysAddr(const PMR *psPMR,
		 MT_UINT32 ui32Log2PageSize,
		 MT_UINT32 ui32NumOfPages,
		 MT_DEVMEM_OFFSET_T uiLogicalOffset,
		 MT_DEV_PHYADDR *psUserAddrPtr,
		 MT_BOOL *pbValid);

MTGPU_ERROR
PMR_CopySystemMem(const PMR *psPMR,
		  MT_UINT32 ui32Log2PageSize,
		  MT_UINT32 ui32NumOfPages,
		  MT_DEVMEM_OFFSET_T uiLogicalOffset,
		  MT_UINT64 uiSize,
		  void __user *userAddress,
		  MT_BOOL *pbValid,
		  MT_BOOL isToDev);

void PMR_SetUnifiedDevPhysAddr(MT_UINT64 *uiAddr, MT_INT32 i32CardId,
			       MT_UINT64 ui64PerUpaSpace);

/*
 * PMR_CpuPhysAddr()
 *
 * See note above about Lock/Unlock semantics.
 *
 * Given an offset, this function returns the CPU physical address of the
 * corresponding page in the PMR.  It may be called multiple times
 * until the address of all relevant pages has been determined.
 *
 */
MTGPU_ERROR
PMR_CpuPhysAddr(const PMR *psPMR,
                MT_UINT32 ui32Log2PageSize,
                MT_UINT32 ui32NumOfPages,
                MT_DEVMEM_OFFSET_T uiLogicalOffset,
                MT_CPU_PHYADDR *psCpuAddrPtr,
                MT_BOOL *pbValid);

MTGPU_ERROR
PMRGetUID(PMR *psPMR,
          MT_UINT64 *pui64UID);

MTGPU_ERROR
PMRGetSize(PMR *psPMR,
           MT_UINT64 *uiLogicalSize);

MTGPU_ERROR
PMRGetKey(PMR *psPMR,
          MT_UINT64 *uiKey);

MTGPU_ERROR
PMRGetAnnotation(PMR *psPMR, MT_CHAR *szAnnotation);

/*
 * PMR_ChangeSparseMem()
 *
 * See note above about Lock/Unlock semantics.
 *
 * This function alters the memory map of the given PMR in device space by
 *  adding/deleting the pages as requested.
 *
 */
MTGPU_ERROR PMR_ChangeSparseMem(PMR *psPMR,
				 MT_UINT32 ui32AllocPageCount,
				 MT_UINT32 *pai32AllocIndices,
				 MT_UINT32 ui32FreePageCount,
				 MT_UINT32 *pai32FreeIndices,
				 MT_UINT32 uiSparseFlags);

/*
 * PMR_ChangeSparseMemCPUMap()
 *
 * See note above about Lock/Unlock semantics.
 *
 * This function alters the memory map of the given PMR in CPU space by
 * adding/deleting the pages as requested.
 */
MTGPU_ERROR PMR_ChangeSparseMemCPUMap(PMR *psPMR,
				       MT_UINT64 sCpuVAddrBase,
				       MT_UINT32 ui32AllocPageCount,
				       MT_UINT32 *pai32AllocIndices,
				       MT_UINT32 ui32FreePageCount,
				       MT_UINT32 *pai32FreeIndices);

/* This function returns the private data that a pmr subtype embedded in
 * here. We use the function table pointer as "authorisation" that this
 * function is being called by the pmr subtype implementation. We can
 * assume (assert) that. It would be a bug in the implementation of the
 * pmr subtype if this assertion ever fails.
 */
void *
PMRGetPrivateData(const PMR *psPMR,
                  const PMR_IMPL_FUNCTAB *psFuncTab);

PMTGPU_DEVICE_NODE PMR_GetDeviceNode(const PMR *psPMR);

/*
 * PMRInit()
 *
 * To be called once and only once to initialise the internal data in
 * the PMR module (mutexes and such)
 *
 * Not for general use.  Only MTGPUInit(); should be calling this.
 */
MTGPU_ERROR
PMRInit(void);

/*
 * PMRDeInit()
 *
 * To be called once and only once to deinitialise the internal data in
 * the PMR module (mutexes and such) and for debug checks
 *
 * Not for general use.  Only MTGPUDeInit(); should be calling this.
 */
MTGPU_ERROR
PMRDeInit(void);


#if defined(MTGPU_ENABLE_GPU_MEMORY_INFO)
MTGPU_ERROR
PMRStoreRIHandle(PMR *psPMR, void *hRIHandle);
#endif

MT_BOOL PMR_IsP2P(struct _MTGPU_DEVICE_NODE_ *psDevNode,
		   const PMR *psPMR);

MTGPU_ERROR
PMR_CreatePAInfo(struct _MTGPU_DEVICE_NODE_ *psDevNode,
		 const PMR *psPMR,
		 MT_UINT32 ui32Log2PageSize,
		 MT_UINT32 uiReqPageNums,
		 MT_DEVMEM_OFFSET_T uiLogicalOffset,
		 PMR_PA_INFO **ppsPMRPAInfo,
		 MT_UINT64 uiMappingFlags,
		 DEVMEM_INTERLEAVE_RATIO *psInterleaveRatio,
		 MT_UINT32 uiVmCtxFlag);

void PMR_DestroyPAInfo(PMR_PA_INFO *psPMRPAInfo);

MTGPU_ERROR
PMR_GetGpuPAByPageIndex(PMR_PA_INFO *psPMRPAInfo,
			MT_UINT32 ui32PageIndex,
			MT_DEV_PHYADDR *puiDevPhysAddr,
			MT_BOOL *pbValid);

MT_BOOL PMR_NeedUnmapForPAInfo(PMR_PA_INFO *psPMRPAInfo);

void *PMRGetDmaBuf(PMR *psPMR);
void PMRSetDmaBuf(PMR *psPMR, void *pvDmabuf);

#if (MUSA_NUM_OS_SUPPORTED > 1)
MTGPU_ERROR mtgpu_vgpu_host_alloc(struct device *dev, MT_UINT32 group_id,
				   size_t size, dma_addr_t *dev_addr, void **ppsPMR);
MTGPU_ERROR mtgpu_vgpu_host_free(PMR *psPMR);
#endif

#endif /* #ifdef SRVSRV_PMR_H */

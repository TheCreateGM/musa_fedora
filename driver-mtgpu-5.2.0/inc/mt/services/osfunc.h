/*************************************************************************/ /*!
@File
@Title          OS functions header
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    OS specific API definitions
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

#ifdef DEBUG_RELEASE_BUILD
#pragma optimize( "", off )
#define DEBUG		1
#endif

#ifndef OSFUNC_H
/*! @cond Doxygen_Suppress */
#define OSFUNC_H
/*! @endcond */

#if defined(__QNXNTO__)
#include <stdio.h>
#include <string.h>
#endif

#if defined(INTEGRITY_OS)
#include <stdio.h>
#include <string.h>
#endif

#include "img_types.h"
#include "img_defs.h"
#include "device.h"
#include "mt_device.h"
#include "cache_ops.h"
#include "osfunc_common.h"
#include "pmr_impl.h"

#define OS_MT_VAL(index)	(OSMtValue[OS_MT_##index])
#define DECLEAR_OS_MT_VALUE \
	X(PAGE_SIZE) \
	X(ENOMEM) \
	X(TASK_INTERRUPTIBLE) \
	X(EBADMSG) \
	X(ENOPKG) \
	X(ENOENT) \
	X(EIO) \
	X(EFAULT) \
	X(DRM_UT_CORE) \
	X(VERIFYING_UNSPECIFIED_SIGNATURE) \
	X(ENOSPC) \
	X(ETIME) \
	X(EINVAL) \
	X(ENOSYS) \
	X(ENXIO) \
	X(EAGAIN) \
	X(EINTR) \
	X(DMA_BIDIRECTIONAL) \
	X(DMA_FROM_DEVICE) \
	X(EMFILE) \
	X(ERANGE) \
	X(EEXIST) \
	X(WQ_UNBOUND) \
	X(WQ_FREEZABLE) \
	X(O_CLOEXEC)

enum {
#define X(VALUE) OS_MT_##VALUE,
	DECLEAR_OS_MT_VALUE
#undef X
	OS_MT_VALUE_MAX,
};

#if defined(MTGPU_ENABLE_WORK_QUEUE_OPTIMIZATION)
/* The max cpu number for work queue to schedule on. */
#define MAX_CPU_NUM 128
#endif

extern const MT_UINT64 OSMtValue[];

#ifndef KERN_SOH
#define KERN_SOH	"\001"		/* ASCII Start Of Header */
#define KERN_SOH_ASCII	'\001'

#define KERN_EMERG	KERN_SOH "0"	/* system is unusable */
#define KERN_ALERT	KERN_SOH "1"	/* action must be taken immediately */
#define KERN_CRIT	KERN_SOH "2"	/* critical conditions */
#define KERN_ERR	KERN_SOH "3"	/* error conditions */
#define KERN_WARNING	KERN_SOH "4"	/* warning conditions */
#define KERN_NOTICE	KERN_SOH "5"	/* normal but significant condition */
#define KERN_INFO	KERN_SOH "6"	/* informational */
#define KERN_DEBUG	KERN_SOH "7"	/* debug-level messages */

#define KERN_DEFAULT	""		/* the default kernel loglevel */
#endif

#define OSPrintFmt(fmt) fmt

#define OSPrintEmerg(fmt, ...)							\
	OSPrintk(KERN_EMERG OSPrintFmt(fmt), ##__VA_ARGS__)
#define OSPrintAlert(fmt, ...)							\
	OSPrintk(KERN_ALERT OSPrintFmt(fmt), ##__VA_ARGS__)
#define OSPrintCrit(fmt, ...)							\
	OSPrintk(KERN_CRIT OSPrintFmt(fmt), ##__VA_ARGS__)
#define OSPrintErr(fmt, ...)							\
	OSPrintk(KERN_ERR OSPrintFmt(fmt), ##__VA_ARGS__)
#define OSPrintWarn(fmt, ...)						        \
	OSPrintk(KERN_WARNING OSPrintFmt(fmt), ##__VA_ARGS__)
#define OSPrintNotice(fmt, ...)							\
	OSPrintk(KERN_NOTICE OSPrintFmt(fmt), ##__VA_ARGS__)
#define OSPrintInfo(fmt, ...)							\
	OSPrintk(KERN_INFO OSPrintFmt(fmt), ##__VA_ARGS__)

/******************************************************************************
 * Static defines
 *****************************************************************************/

#ifndef OSAlign
#define __OSAlignMask(x, mask)	(((x) + (mask)) & ~(mask))
#define __OSAlign(x, a)		__OSAlignMask(x, (typeof(x))(a) - 1)
#define OSAlign(x, a)		__OSAlign((x), (a))
#define OSAlignDown(x, a)	__OSAlign((x) - ((a) - 1), (a))
#endif

/*!
 * Returned by OSGetCurrentProcessID() and OSGetCurrentThreadID() if the OS
 * is currently operating in the interrupt context.
 */
#define KERNEL_ID			0xffffffffL

#if defined(__linux__) && defined(__KERNEL__)
MT_UINT32 OSConfineArrayIndexNoSpeculation(MT_UINT32 ui32Index, MT_UINT32 ui32Size);
#elif defined(__QNXNTO__)
#define OSConfineArrayIndexNoSpeculation(index, size) (index)
#define MTGPU_MISSING_NO_SPEC_IMPL
#elif defined(INTEGRITY_OS)
#define OSConfineArrayIndexNoSpeculation(index, size) (index)
#define MTGPU_MISSING_NO_SPEC_IMPL
#else
/*************************************************************************/ /*!
@Function       OSConfineArrayIndexNoSpeculation
@Description    This macro aims to avoid code exposure to Cache Timing
                Side-Channel Mechanisms which rely on speculative code
                execution (Variant 1). It does so by ensuring a value to be
                used as an array index will be set to zero if outside of the
                bounds of the array, meaning any speculative execution of code
                which uses this suitably adjusted index value will not then
                attempt to load data from memory outside of the array bounds.
                Code calling this macro must still first verify that the
                original unmodified index value is within the bounds of the
                array, and should then only use the modified value returned
                by this function when accessing the array itself.
                NB. If no OS-specific implementation of this macro is
                defined, the original index is returned unmodified and no
                protection against the potential exploit is provided.
@Input          index    The original array index value that would be used to
                         access the array.
@Input          size     The number of elements in the array being accessed.
@Return         The value to use for the array index, modified so that it
                remains within array bounds.
*/ /**************************************************************************/
#define OSConfineArrayIndexNoSpeculation(index, size) (index)
#if !defined(DOXYGEN)
#define MTGPU_MISSING_NO_SPEC_IMPL
#endif
#endif

/*************************************************************************/ /*!
@Function       OSClockns64
@Description    This function returns the number of ticks since system boot
                expressed in nanoseconds. Unlike OSClockns, OSClockns64 has
                a near 64-bit range.
@Return         The 64-bit clock value, in nanoseconds.
*/ /**************************************************************************/
MT_UINT64 OSClockns64(void);

/*************************************************************************/ /*!
@Function       OSClockus64
@Description    This function returns the number of ticks since system boot
                expressed in microseconds. Unlike OSClockus, OSClockus64 has
                a near 64-bit range.
@Return         The 64-bit clock value, in microseconds.
*/ /**************************************************************************/
MT_UINT64 OSClockus64(void);

/*************************************************************************/ /*!
@Function       OSClockus
@Description    This function returns the number of ticks since system boot
                in microseconds.
@Return         The 32-bit clock value, in microseconds.
*/ /**************************************************************************/
MT_UINT32 OSClockus(void);

/*************************************************************************/ /*!
@Function       OSClockms
@Description    This function returns the number of ticks since system boot
                in milliseconds.
@Return         The 32-bit clock value, in milliseconds.
*/ /**************************************************************************/
MT_UINT32 OSClockms(void);

MT_UINT64 OSKClockns64(void);
/*************************************************************************/ /*!
@Function       OSClockMonotonicns64
@Description    This function returns a clock value based on the system
                monotonic clock.
@Output         pui64Time     The 64-bit clock value, in nanoseconds.
@Return         Error Code.
*/ /**************************************************************************/
MTGPU_ERROR OSClockMonotonicns64(MT_UINT64 *pui64Time);

/*************************************************************************/ /*!
@Function       OSClockMonotonicus64
@Description    This function returns a clock value based on the system
                monotonic clock.
@Output         pui64Time     The 64-bit clock value, in microseconds.
@Return         Error Code.
*/ /**************************************************************************/
MTGPU_ERROR OSClockMonotonicus64(MT_UINT64 *pui64Time);

/*************************************************************************/ /*!
@Function       OSClockMonotonicRawns64
@Description    This function returns a clock value based on the system
                monotonic raw clock.
@Return         64bit ns timestamp
*/ /**************************************************************************/
MT_UINT64 OSClockMonotonicRawns64(void);

/*************************************************************************/ /*!
@Function       OSClockMonotonicRawus64
@Description    This function returns a clock value based on the system
                monotonic raw clock.
@Return         64bit us timestamp
*/ /**************************************************************************/
MT_UINT64 OSClockMonotonicRawus64(void);

/*************************************************************************/ /*!
@Function       OSGetPageSize
@Description    This function returns the page size.
                If the OS is not using memory mappings it should return a
                default value of 4096.
@Return         The size of a page, in bytes.
*/ /**************************************************************************/
size_t OSGetPageSize(void);

/*************************************************************************/ /*!
@Function       OSGetPageShift
@Description    This function returns the page size expressed as a power of
                two. A number of pages, left-shifted by this value, gives the
                equivalent size in bytes.
                If the OS is not using memory mappings it should return a
                default value of 12.
@Return         The page size expressed as a power of two.
*/ /**************************************************************************/
size_t OSGetPageShift(void);

/*************************************************************************/ /*!
@Function       OSGetPageMask
@Description    This function returns a bitmask that may be applied to an
                address to mask off the least-significant bits so as to
                leave the start address of the page containing that address.
@Return         The page mask.
*/ /**************************************************************************/
size_t OSGetPageMask(void);

/*************************************************************************/ /*!
@Function       OSGetOrder
@Description    This function returns the order of power of two for a given
                size. Eg. for a uSize of 4096 bytes the function would
                return 12 (4096 = 2^12).
@Input          uSize     The size in bytes.
@Return         The order of power of two.
*/ /**************************************************************************/
size_t OSGetOrder(size_t uSize);

/*************************************************************************/ /*!
@Function       OSGetRAMSize
@Description    This function returns the total amount of GPU-addressable
                memory provided by the system. In other words, after loading
                the driver this would be the largest allocation an
                application would reasonably expect to be able to make.
                Note that this is function is not expected to return the
                current available memory but the amount which would be
                available on startup.
@Return         Total GPU-addressable memory size, in bytes.
*/ /**************************************************************************/
MT_UINT64 OSGetRAMSize(void);

/*************************************************************************/ /*!
@Description    Pointer to a Mid-level Interrupt Service Routine (MISR).
@Input  pvData  Pointer to MISR specific data.
*/ /**************************************************************************/
typedef void (*PFN_MISR)(void *pvData);

/*************************************************************************/ /*!
@Description    Pointer to a thread entry point function.
@Input  pvData  Pointer to thread specific data.
*/ /**************************************************************************/
typedef void (*PFN_THREAD)(void *pvData);

/*************************************************************************/ /*!
@Function       OSChangeSparseMemCPUAddrMap
@Description    This function changes the CPU mapping of the underlying
                sparse allocation. It is used by a PMR 'factory'
                implementation if that factory supports sparse
                allocations.
@Input          psPageArray        array representing the pages in the
                                   sparse allocation
@Input          sCpuVAddrBase      the virtual base address of the sparse
                                   allocation ('first' page)
@Input          sCpuPAHeapBase     the physical address of the virtual
                                   base address 'sCpuVAddrBase'
@Input          ui32AllocPageCount the number of pages referenced in
                                   'pai32AllocIndices'
@Input          pai32AllocIndices  list of indices of pages within
                                   'psPageArray' that we now want to
                                   allocate and map
@Input          ui32FreePageCount  the number of pages referenced in
                                   'pai32FreeIndices'
@Input          pai32FreeIndices   list of indices of pages within
                                   'psPageArray' we now want to
                                   unmap and free
@Input          bIsLMA             flag indicating if the sparse allocation
                                   is from LMA or UMA memory
@Return         MTGPU_OK on success, a failure code otherwise.
*/ /**************************************************************************/
MTGPU_ERROR OSChangeSparseMemCPUAddrMap(void **psPageArray,
                                         MT_UINT64 sCpuVAddrBase,
                                         MT_CPU_PHYADDR sCpuPAHeapBase,
                                         MT_UINT32 ui32AllocPageCount,
                                         MT_UINT32 *pai32AllocIndices,
                                         MT_UINT32 ui32FreePageCount,
                                         MT_UINT32 *pai32FreeIndices,
                                         MT_BOOL bIsLMA);

/*************************************************************************/ /*!
@Function       OSInstallMISR
@Description    Installs a Mid-level Interrupt Service Routine (MISR)
                which handles higher-level processing of interrupts from
                the device (GPU).
                An MISR runs outside of interrupt context, and so may be
                descheduled. This means it can contain code that would
                not be permitted in the LISR.
                An MISR is invoked when OSScheduleMISR() is called. This
                call should be made by installed LISR once it has completed
                its interrupt processing.
                Multiple MISRs may be installed by the driver to handle
                different causes of interrupt.
@Input          pfnMISR       pointer to the function to be installed
                              as the MISR
@Input          hData         private data provided to the MISR
@Input          pszMisrName   Name describing purpose of MISR worker thread
                              (Must be a string literal).
@Output         hMISRData     handle to the installed MISR (to be used
                              for a subsequent uninstall)
@Return         MTGPU_OK on success, a failure code otherwise.
*/ /**************************************************************************/
MTGPU_ERROR OSInstallMISR(MT_HANDLE *hMISRData,
							PFN_MISR pfnMISR,
							void *hData,
							const MT_CHAR *pszMisrName);

/*************************************************************************/ /*!
@Function       OSUninstallMISR
@Description    Uninstalls a Mid-level Interrupt Service Routine (MISR).
@Input          hMISRData     handle to the installed MISR
@Return         MTGPU_OK on success, a failure code otherwise.
*/ /**************************************************************************/
MTGPU_ERROR OSUninstallMISR(MT_HANDLE hMISRData);

/*************************************************************************/ /*!
@Function       OSCancelMISR
@Description    Cancel a Mid-level Interrupt Service Routine (MISR) workqueue.
@Input          hMISRData     handle to the installed MISR
@Return         MTGPU_OK on success, a failure code otherwise.
*/ /**************************************************************************/
MTGPU_ERROR OSCancelMISR(MT_HANDLE hMISRData);

/*************************************************************************/ /*!
@Function       OSScheduleMISR
@Description    Schedules a Mid-level Interrupt Service Routine (MISR) to be
                executed. An MISR should be executed outside of interrupt
                context, for example in a work queue.
@Input          hMISRData     handle to the installed MISR
@Return         MTGPU_OK on success, a failure code otherwise.
*/ /**************************************************************************/
MTGPU_ERROR OSScheduleMISR(MT_HANDLE hMISRData);

/*************************************************************************/ /*!
@Description    Pointer to a function implementing debug dump of thread-specific
                data.
@Input          pfnDumpDebugPrintf      Used to specify the print function used
                                        to dump any debug information. If this
                                        argument is NULL then a default print
                                        function will be used.
@Input          pvDumpDebugFile         File identifier to be passed to the
                                        print function if specified.
*/ /**************************************************************************/

typedef void (*PFN_THREAD_DEBUG_DUMP)(DUMPDEBUG_PRINTF_FUNC* pfnDumpDebugPrintf,
                                      void *pvDumpDebugFile);

/*************************************************************************/ /*!
@Function       OSThreadCreate
@Description    Creates a kernel thread and starts it running. The caller
                is responsible for informing the thread that it must finish
                and return from the pfnThread function. It is not possible
                to kill or terminate it. The new thread runs with the default
                priority provided by the Operating System.
                Note: Kernel threads are freezable which means that they
                can be frozen by the kernel on for example driver suspend.
                Because of that only OSEventObjectWaitKernel() function should
                be used to put kernel threads in waiting state.
@Output         phThread            Returned handle to the thread.
@Input          pszThreadName       Name to assign to the thread.
@Input          pfnThread           Thread entry point function.
@Input          pfnDebugDumpCB      Used to dump info of the created thread
@Input          bIsSupportingThread Set, if summary of this thread needs to
                                    be dumped in debug_dump
@Input          hData               Thread specific data pointer for pfnThread().
@Return         Standard MTGPU_ERROR error code.
*/ /**************************************************************************/

MTGPU_ERROR OSThreadCreate(MT_HANDLE *phThread,
                            MT_CHAR *pszThreadName,
                            PFN_THREAD pfnThread,
                            PFN_THREAD_DEBUG_DUMP pfnDebugDumpCB,
                            MT_BOOL bIsSupportingThread,
                            void *hData);

/*! Available priority levels for the creation of a new Kernel Thread. */
typedef enum priority_levels
{
	OS_THREAD_NOSET_PRIORITY = 0,   /* With this option the priority level is the default for the given OS */
	OS_THREAD_HIGHEST_PRIORITY,
	OS_THREAD_HIGH_PRIORITY,
	OS_THREAD_NORMAL_PRIORITY,
	OS_THREAD_LOW_PRIORITY,
	OS_THREAD_LOWEST_PRIORITY,
	OS_THREAD_LAST_PRIORITY     /* This must be always the last entry */
} OS_THREAD_LEVEL;

/*************************************************************************/ /*!
@Function       OSThreadCreatePriority
@Description    As OSThreadCreate, this function creates a kernel thread and
                starts it running. The difference is that with this function
                is possible to specify the priority used to schedule the new
                thread.

@Output         phThread            Returned handle to the thread.
@Input          pszThreadName       Name to assign to the thread.
@Input          pfnThread           Thread entry point function.
@Input          pfnDebugDumpCB      Used to dump info of the created thread
@Input          bIsSupportingThread Set, if summary of this thread needs to
                                    be dumped in debug_dump
@Input          hData               Thread specific data pointer for pfnThread().
@Input          eThreadPriority     Priority level to assign to the new thread.
@Return         Standard MTGPU_ERROR error code.
*/ /**************************************************************************/
MTGPU_ERROR OSThreadCreatePriority(MT_HANDLE *phThread,
                                    MT_CHAR *pszThreadName,
                                    PFN_THREAD pfnThread,
                                    PFN_THREAD_DEBUG_DUMP pfnDebugDumpCB,
                                    MT_BOOL bIsSupportingThread,
                                    void *hData,
                                    OS_THREAD_LEVEL eThreadPriority);

/*************************************************************************/ /*!
@Function       OSThreadDestroy
@Description    Waits for the thread to end and then destroys the thread
                handle memory. This function will block and wait for the
                thread to finish successfully, thereby providing a sync point
                for the thread completing its work. No attempt is made to kill
                or otherwise terminate the thread.
@Input          hThread   The thread handle returned by OSThreadCreate().
@Return         Standard MTGPU_ERROR error code.
*/ /**************************************************************************/
MTGPU_ERROR OSThreadDestroy(MT_HANDLE hThread);

/*************************************************************************/ /*!
@Function       OSMapPhysToLin
@Description    Maps physical memory into a linear address range.
@Input          BasePAddr    physical CPU address
@Input          ui32Bytes    number of bytes to be mapped
@Input          uiFlags      flags denoting the caching mode to be employed
                             for the mapping (uncached/write-combined,
                             cached coherent or cached incoherent).
                             See mtgpu_memallocflags.h for full flag bit
                             definitions.
@Return         Pointer to the new mapping if successful, NULL otherwise.
*/ /**************************************************************************/
void *OSMapPhysToLin(MT_CPU_PHYADDR BasePAddr, size_t ui32Bytes, MTGPU_MEMALLOCFLAGS_T uiFlags);

/*************************************************************************/ /*!
@Function       OSUnMapPhysToLin
@Description    Unmaps physical memory previously mapped by OSMapPhysToLin().
@Input          pvLinAddr    the linear mapping to be unmapped
@Input          ui32Bytes    number of bytes to be unmapped
@Return         MT_TRUE if unmapping was successful, MT_FALSE otherwise.
*/ /**************************************************************************/
MT_BOOL OSUnMapPhysToLin(void *pvLinAddr, size_t ui32Bytes);

/*************************************************************************/ /*!
@Function       OSCPUCacheFlushRangeKM
@Description    Clean and invalidate the CPU cache for the specified
                address range.
@Input          psDevNode     device on which the allocation was made
@Input          pvVirtStart   virtual start address of the range to be
                              flushed
@Input          pvVirtEnd     virtual end address of the range to be
                              flushed
@Input          sCPUPhysStart physical start address of the range to be
                              flushed
@Input          sCPUPhysEnd   physical end address of the range to be
                              flushed
@Return         None
*/ /**************************************************************************/
void OSCPUCacheFlushRangeKM(MTGPU_DEVICE_NODE *psDevNode,
                            void *pvVirtStart,
                            void *pvVirtEnd,
                            MT_CPU_PHYADDR sCPUPhysStart,
                            MT_CPU_PHYADDR sCPUPhysEnd);

/*************************************************************************/ /*!
@Function       OSCPUCacheCleanRangeKM
@Description    Clean the CPU cache for the specified address range.
                This writes out the contents of the cache and clears the
                'dirty' bit (which indicates the physical memory is
                consistent with the cache contents).
@Input          psDevNode     device on which the allocation was made
@Input          pvVirtStart   virtual start address of the range to be
                              cleaned
@Input          pvVirtEnd     virtual end address of the range to be
                              cleaned
@Input          sCPUPhysStart physical start address of the range to be
                              cleaned
@Input          sCPUPhysEnd   physical end address of the range to be
                              cleaned
@Return         None
*/ /**************************************************************************/
void OSCPUCacheCleanRangeKM(MTGPU_DEVICE_NODE *psDevNode,
                            void *pvVirtStart,
                            void *pvVirtEnd,
                            MT_CPU_PHYADDR sCPUPhysStart,
                            MT_CPU_PHYADDR sCPUPhysEnd);

/*************************************************************************/ /*!
@Function       OSCPUCacheInvalidateRangeKM
@Description    Invalidate the CPU cache for the specified address range.
                The cache must reload data from those addresses if they
                are accessed.
@Input          psDevNode     device on which the allocation was made
@Input          pvVirtStart   virtual start address of the range to be
                              invalidated
@Input          pvVirtEnd     virtual end address of the range to be
                              invalidated
@Input          sCPUPhysStart physical start address of the range to be
                              invalidated
@Input          sCPUPhysEnd   physical end address of the range to be
                              invalidated
@Return         None
*/ /**************************************************************************/
void OSCPUCacheInvalidateRangeKM(MTGPU_DEVICE_NODE *psDevNode,
                                 void *pvVirtStart,
                                 void *pvVirtEnd,
                                 MT_CPU_PHYADDR sCPUPhysStart,
                                 MT_CPU_PHYADDR sCPUPhysEnd);

/*! CPU Cache operations address domain type */
typedef enum
{
	OS_CACHE_OP_ADDR_TYPE_VIRTUAL,    /*!< Operation requires CPU virtual address only */
	OS_CACHE_OP_ADDR_TYPE_PHYSICAL,   /*!< Operation requires CPU physical address only */
	OS_CACHE_OP_ADDR_TYPE_BOTH        /*!< Operation requires both CPU virtual & physical addresses */
} OS_CACHE_OP_ADDR_TYPE;

/*************************************************************************/ /*!
@Function       OSCPUCacheOpAddressType
@Description    Returns the address type (i.e. virtual/physical/both) the CPU
                architecture performs cache maintenance operations under.
                This is used to infer whether the virtual or physical address
                supplied to the OSCPUCacheXXXRangeKM functions can be omitted
                when called.
@Return         OS_CACHE_OP_ADDR_TYPE
*/ /**************************************************************************/
OS_CACHE_OP_ADDR_TYPE OSCPUCacheOpAddressType(void);

/*! CPU Cache attributes available for retrieval, DCache unless specified */
typedef enum _OS_CPU_CACHE_ATTRIBUTE_
{
	OS_CPU_CACHE_ATTRIBUTE_LINE_SIZE, /*!< The cache line size */
	OS_CPU_CACHE_ATTRIBUTE_COUNT      /*!< The number of attributes (must be last) */
} OS_CPU_CACHE_ATTRIBUTE;

/*************************************************************************/ /*!
@Function       OSCPUCacheAttributeSize
@Description    Returns the size of a given cache attribute.
                Typically this function is used to return the cache line
                size, but may be extended to return the size of other
                cache attributes.
@Input          eCacheAttribute   the cache attribute whose size should
                                  be returned.
@Return         The size of the specified cache attribute, in bytes.
*/ /**************************************************************************/
MT_UINT32 OSCPUCacheAttributeSize(OS_CPU_CACHE_ATTRIBUTE eCacheAttribute);

/*************************************************************************/ /*!
@Function       OSGetCurrentProcessID
@Description    Returns ID of current process (thread group)
@Return         ID of current process
*****************************************************************************/
MT_PID OSGetCurrentProcessID(void);

/*************************************************************************/ /*!
@Function       OSGetCurrentVirtualProcessID
@Description    Returns ID of current process (thread group of current
                PID namespace)
@Return         ID of current process in PID namespace
*****************************************************************************/
MT_PID OSGetCurrentVirtualProcessID(void);

/*************************************************************************/ /*!
@Function       OSGetCurrentProcessName
@Description    Gets the name of current process
@Return         Process name
*****************************************************************************/
MT_CHAR *OSGetCurrentProcessName(void);

/*************************************************************************/ /*!
@Function       OSGetCurrentProcessVASpaceSize
@Description    Returns the CPU virtual address space size of current process
@Return         Process VA space size
*/ /**************************************************************************/
MT_UINT64 OSGetCurrentProcessVASpaceSize(void);

/*************************************************************************/ /*!
@Function       OSGetCurrentThreadID
@Description    Returns ID for current thread
@Return         ID of current thread
*****************************************************************************/
uintptr_t OSGetCurrentThreadID(void);

/*************************************************************************/ /*!
@Function       OSGetCurrentClientProcessIDKM
@Description    Returns ID of current client process (thread group) which
                has made a bridge call into the server.
                For some operating systems, this may simply be the current
                process id. For others, it may be that a dedicated thread
                is used to handle the processing of bridge calls and that
                some additional processing is required to obtain the ID of
                the client process making the bridge call.
@Return         ID of current client process
*****************************************************************************/
MT_PID OSGetCurrentClientProcessIDKM(void);

/*************************************************************************/ /*!
@Function       OSGetCurrentClientProcessNameKM
@Description    Gets the name of current client process
@Return         Client process name
*****************************************************************************/
MT_CHAR *OSGetCurrentClientProcessNameKM(void);

/*************************************************************************/ /*!
@Function       OSGetCurrentClientThreadIDKM
@Description    Returns ID for current client thread
                For some operating systems, this may simply be the current
                thread id. For others, it may be that a dedicated thread
                is used to handle the processing of bridge calls and that
                some additional processing is require to obtain the ID of
                the client thread making the bridge call.
@Return         ID of current client thread
*****************************************************************************/
uintptr_t OSGetCurrentClientThreadIDKM(void);
pid_t OSGetCurrentTgid(void);

/*************************************************************************/ /*!
@Function       OSMemCmp
@Description    Compares two blocks of memory for equality.
@Input          pvBufA      Pointer to the first block of memory
@Input          pvBufB      Pointer to the second block of memory
@Input          uiLen       The number of bytes to be compared
@Return         Value < 0 if pvBufA is less than pvBufB.
                Value > 0 if pvBufB is less than pvBufA.
                Value = 0 if pvBufA is equal to pvBufB.
*****************************************************************************/
MT_INT OSMemCmp(void *pvBufA, void *pvBufB, size_t uiLen);

/*************************************************************************/ /*!
@Function       OSMemSet
@Description    Fill a region of memory with the given value.
@Input          pvBuf	    Pointer to the start of the area
@Input          ui32Str	    The byte to fill the area with
@Input          uiLen       The size of the area
@Return         Pointer to the pvBuf
*****************************************************************************/
void *OSMemSet(void *pvBuf, MT_INT ui32Str, size_t uiLen);

void OSMemSetIO(void *pvBuf, MT_INT ui32Str, size_t uiLen);

/*************************************************************************/ /*!
@Function       OSMemCopy
@Description    Copy one area of memory to another.
@Input          pvDest	    Where to copy to
@Input          pvSrc	    Where to copy from
@Input          uiLen       The size of the area
@Return         Pointer to the pvDest
*****************************************************************************/
void *OSMemCopy(void *pvDest, const void *pvSrc, size_t uiLen);

/*************************************************************************/ /*!
@Function       OSPhyContigPagesAlloc
@Description    Allocates a number of contiguous physical pages.
                If allocations made by this function are CPU cached then
                OSPhyContigPagesClean has to be implemented to write the
                cached data to memory.
@Input          psPhysHeap    the heap from which to allocate
@Input          uiSize        the size of the required allocation (in bytes)
@Output         psMemHandle   a returned handle to be used to refer to this
                              allocation
@Output         psDevPAddr    the physical address of the allocation
@Input          uiPid         the process ID that this allocation should
                              be associated with
@Return         MTGPU_OK on success, a failure code otherwise.
*****************************************************************************/
MTGPU_ERROR OSPhyContigPagesAlloc(PHYS_HEAP *psPhysHeap, size_t uiSize,
							PG_HANDLE *psMemHandle, MT_DEV_PHYADDR *psDevPAddr,
							MT_PID uiPid, MT_UINT32 uiVmCtxFlag);

/*************************************************************************/ /*!
@Function       OSPhyContigPagesFree
@Description    Frees a previous allocation of contiguous physical pages
@Input          psPhysHeap    the heap from which to allocate
@Input          psMemHandle   the handle of the allocation to be freed
@Return         None.
*****************************************************************************/
void OSPhyContigPagesFree(PHYS_HEAP *psPhysHeap, PG_HANDLE *psMemHandle);

/*************************************************************************/ /*!
@Function       OSPhyContigPagesMap
@Description    Maps the specified allocation of contiguous physical pages
                to a kernel virtual address
@Input          psPhysHeap    the heap from which to allocate
@Input          psMemHandle   the handle of the allocation to be mapped
@Input          uiSize        the size of the allocation (in bytes)
@Input          psDevPAddr    the physical address of the allocation
@Output         pvPtr         the virtual kernel address to which the
                              allocation is now mapped
@Return         MTGPU_OK on success, a failure code otherwise.
*****************************************************************************/
MTGPU_ERROR OSPhyContigPagesMap(PHYS_HEAP *psPhysHeap, PG_HANDLE *psMemHandle,
						size_t uiSize, MT_DEV_PHYADDR *psDevPAddr,
						void **pvPtr, MT_UINT32 uiVmCtxFlag);

/*************************************************************************/ /*!
@Function       OSPhyContigPagesUnmap
@Description    Unmaps the kernel mapping for the specified allocation of
                contiguous physical pages
@Input          psPhysHeap    the heap from which to allocate
@Input          psMemHandle   the handle of the allocation to be unmapped
@Input          pvPtr         the virtual kernel address to which the
                              allocation is currently mapped
@Return         None.
*****************************************************************************/
void OSPhyContigPagesUnmap(PHYS_HEAP *psPhysHeap, PG_HANDLE *psMemHandle, void *pvPtr);

/*************************************************************************/ /*!
@Function       OSPhyContigPagesClean
@Description    Write the content of the specified allocation from CPU cache to
                memory from (start + uiOffset) to (start + uiOffset + uiLength)
                It is expected to be implemented as a cache clean operation but
                it is allowed to fall back to a cache clean + invalidate
                (i.e. flush).
                If allocations returned by OSPhyContigPagesAlloc are always
                uncached this can be implemented as nop.
@Input          psPhysHeap    the heap from which to allocate
@Input          psMemHandle   the handle of the allocation to be flushed
@Input          uiOffset      the offset in bytes from the start of the
                              allocation from where to start flushing
@Input          uiLength      the amount to flush from the offset in bytes
@Return         MTGPU_OK on success, a failure code otherwise.
*****************************************************************************/
MTGPU_ERROR OSPhyContigPagesClean(PHYS_HEAP *psPhysHeap,
                                   PG_HANDLE *psMemHandle,
                                   MT_UINT32 uiOffset,
                                   MT_UINT32 uiLength);


/*************************************************************************/ /*!
@Function       OSInitEnvData
@Description    Called to initialise any environment-specific data. This
                could include initialising the bridge calling infrastructure
                or device memory management infrastructure.
@Return         MTGPU_OK on success, a failure code otherwise.
*/ /**************************************************************************/
MTGPU_ERROR OSInitEnvData(void);

/*************************************************************************/ /*!
@Function       OSDeInitEnvData
@Description    The counterpart to OSInitEnvData(). Called to free any
                resources which may have been allocated by OSInitEnvData().
@Return         None.
*/ /**************************************************************************/
void OSDeInitEnvData(void);

/*************************************************************************/ /*!
@Function       OSVSScanf
@Description    OS function to support the standard C vsscanf() function.
*/ /**************************************************************************/
MT_UINT32 OSVSScanf(const MT_CHAR *pStr, const MT_CHAR *pszFormat, ...);

/*************************************************************************/ /*!
@Function       OSStringSCpy
@Description    OS function to support the standard C strscpy() function.
*/ /**************************************************************************/
size_t OSStringSCpy(MT_CHAR *pszDest, const MT_CHAR *pszSrc, size_t uDstSize);

/*************************************************************************/ /*!
@Function       OSStringLCat
@Description    OS function to support the BSD C strlcat() function.
*/ /**************************************************************************/
size_t OSStringLCat(MT_CHAR *pszDest, const MT_CHAR *pszSrc, size_t uDstSize);

/*************************************************************************/ /*!
@Function       OSStringChr
@Description    OS function to support the BSD C strchr() function.
*/ /**************************************************************************/
MT_CHAR *OSStringChr(const MT_CHAR *pStr, MT_INT iChar);

/*************************************************************************/ /*!
@Function       OSSNPrintf
@Description    OS function to support the standard C snprintf() function.
@Output         pStr        char array to print into
@Input          ui32Size    maximum size of data to write (chars)
@Input          pszFormat   format string
*/ /**************************************************************************/
MT_INT32 OSSNPrintf(MT_CHAR *pStr, size_t ui32Size, const MT_CHAR *pszFormat, ...) __printf(3, 4);

/*************************************************************************/ /*!
@Function       OSVSNPrintf
@Description    Printf to MT string using variable args (see stdarg.h).
                This is necessary because the '...' notation does not
                support nested function calls.
@Input          ui32Size           maximum size of data to write (chars)
@Input          pszFormat          format string
@Input          vaArgs             variable args structure (from stdarg.h)
@Output         pStr               char array to print into
@Return         Number of character written in buffer if successful other wise -1 on error
*/ /**************************************************************************/
MT_INT32 OSVSNPrintf(MT_CHAR *pStr, size_t ui32Size, const MT_CHAR* pszFormat, va_list vaArgs) __printf(3, 0);

/*************************************************************************/ /*!
@Function       OSStringLength
@Description    OS function to support the standard C strlen() function.
*/ /**************************************************************************/
size_t OSStringLength(const MT_CHAR *pStr);

/*************************************************************************/ /*!
@Function       OSStringNLength
@Description    Return the length of a string, excluding the terminating null
                byte ('\0'), but return at most 'uiCount' bytes. Only the first
                'uiCount' bytes of 'pStr' are interrogated.
@Input          pStr     pointer to the string
@Input          uiCount  the maximum length to return
@Return         Length of the string if less than 'uiCount' bytes, otherwise
                'uiCount'.
*/ /**************************************************************************/
size_t OSStringNLength(const MT_CHAR *pStr, size_t uiCount);

/*************************************************************************/ /*!
@Function       OSStringCompare
@Description    OS function to support the standard C strcmp() function.
*/ /**************************************************************************/
MT_INT32 OSStringCompare(const MT_CHAR *pStr1, const MT_CHAR *pStr2);

/*************************************************************************/ /*!
@Function       OSStringNCompare
@Description    OS function to support the standard C strncmp() function.
*/ /**************************************************************************/
MT_INT32 OSStringNCompare(const MT_CHAR *pStr1, const MT_CHAR *pStr2,
                           size_t uiSize);

/*************************************************************************/ /*!
@Function       OSStringSeparate
@Description    OS function to support the standard C strsep() function.
*/ /**************************************************************************/
MT_CHAR *OSStringSeparate(MT_CHAR **pStr1, const MT_CHAR *pStr2);

/*************************************************************************/ /*!
@Function       OSStringCaseCompare
@Description    OS function to support the standard C strcasecmp() function.
*/ /**************************************************************************/
int OSStringCaseCompare(const MT_CHAR *pStr1, const MT_CHAR *pStr2);

/*************************************************************************/ /*!
@Function       OSStringToUINT64
@Description    Changes string to MT_UINT64.
*/ /**************************************************************************/
MTGPU_ERROR OSStringToUINT64(const MT_CHAR *pStr, MT_UINT64 ui64Base,
			      MT_UINT64 *ui64Result);

/*************************************************************************/ /*!
@Function       OSStringToUINT32
@Description    Changes string to MT_UINT32.
*/ /**************************************************************************/
MTGPU_ERROR OSStringToUINT32(const MT_CHAR *pStr, MT_UINT32 ui32Base,
                              MT_UINT32 *ui32Result);

/*************************************************************************/ /*!
@Function       OSStringUINT32ToStr
@Description    Changes MT_UINT32 to string
@Input          pszBuf         Buffer to write output number string
@Input          uSize          Size of buffer provided, i.e. size of pszBuf
@Input          ui32Num        Number to convert to string
@Return         Returns 0 if buffer is not sufficient to hold the number string,
                else returns length of number string
*/ /**************************************************************************/
MT_UINT32 OSStringUINT32ToStr(MT_CHAR *pszBuf, size_t uSize, MT_UINT32 ui32Num);

/*************************************************************************/ /*!
@Function       OSStringString
@Description    OS function to support the standard C strstr() function.
*/ /**************************************************************************/
MT_CHAR *OSStringString(const MT_CHAR *str1, const MT_CHAR *str2);

/*************************************************************************/ /*!
@Function       OSEventObjectCreate
@Description    Create an event object.
@Input          pszName         name to assign to the new event object.
@Output         EventObject     the created event object.
@Return         MTGPU_OK on success, a failure code otherwise.
*/ /**************************************************************************/
MTGPU_ERROR OSEventObjectCreate(const MT_CHAR *pszName,
								 MT_HANDLE *EventObject);

/*************************************************************************/ /*!
@Function       OSEventObjectDestroy
@Description    Destroy an event object.
@Input          hEventObject    the event object to destroy.
@Return         MTGPU_OK on success, a failure code otherwise.
*/ /**************************************************************************/
MTGPU_ERROR OSEventObjectDestroy(MT_HANDLE hEventObject);

/*************************************************************************/ /*!
@Function       OSEventObjectSignal
@Description    Signal an event object. Any thread waiting on that event
                object will be woken.
@Input          hEventObject    the event object to signal.
@Return         MTGPU_OK on success, a failure code otherwise.
*/ /**************************************************************************/
MTGPU_ERROR OSEventObjectSignal(MT_HANDLE hEventObject);

/*************************************************************************/ /*!
@Function       OSEventObjectWait
@Description    Wait for an event object to signal. The function is passed
                an OS event object handle (which allows the OS to have the
                calling thread wait on the associated event object).
                The calling thread will be rescheduled when the associated
                event object signals.
                If the event object has not signalled after a default timeout
                period (defined in EVENT_OBJECT_TIMEOUT_MS), the function
                will return with the result code MTGPU_ERROR_TIMEOUT.


@Input          hOSEventKM    the OS event object handle associated with
                              the event object.
@Return         MTGPU_OK on success, a failure code otherwise.
*/ /**************************************************************************/
MTGPU_ERROR OSEventObjectWait(MT_HANDLE hOSEventKM);

/*************************************************************************/ /*!
@Function       OSEventObjectWaitKernel
@Description    Wait for an event object to signal. The function is passed
                an OS event object handle (which allows the OS to have the
                calling thread wait on the associated event object).
                The calling thread will be rescheduled when the associated
                event object signals.
                If the event object has not signalled after a default timeout
                period (defined in EVENT_OBJECT_TIMEOUT_MS), the function
                will return with the result code MTGPU_ERROR_TIMEOUT.

                Note: This function should be used only by kernel thread.
                This is because all kernel threads are freezable and
                this function allows the kernel to freeze the threads
                when waiting.

                See OSEventObjectWait() for more details.

@Input          hOSEventKM    the OS event object handle associated with
                              the event object.
@Return         MTGPU_OK on success, a failure code otherwise.
*/ /**************************************************************************/
#if defined(__linux__) && defined(__KERNEL__)
MTGPU_ERROR OSEventObjectWaitKernel(MT_HANDLE hOSEventKM, MT_UINT64 uiTimeoutus);
#else
#define OSEventObjectWaitKernel OSEventObjectWaitTimeout
#endif

/*************************************************************************/ /*!
@Function       OSSuspendTaskInterruptible
@Description    Suspend the current task into interruptible state.
@Return         none.
*/ /**************************************************************************/
#if defined(__linux__) && defined(__KERNEL__)
void OSSuspendTaskInterruptible(void);
#endif

/*************************************************************************/ /*!
@Function       OSEventObjectWaitTimeout
@Description    Wait for an event object to signal or timeout. The function
                is passed an OS event object handle (which allows the OS to
                have the calling thread wait on the associated event object).
                The calling thread will be rescheduled when the associated
                event object signals.
                If the event object has not signalled after the specified
                timeout period (passed in 'uiTimeoutus'), the function
                will return with the result code MTGPU_ERROR_TIMEOUT.
@Input          hOSEventKM    the OS event object handle associated with
                              the event object.
@Input          uiTimeoutus   the timeout period (in usecs)
@Return         MTGPU_OK on success, a failure code otherwise.
*/ /**************************************************************************/
MTGPU_ERROR OSEventObjectWaitTimeout(MT_HANDLE hOSEventKM, MT_UINT64 uiTimeoutus);

/*************************************************************************/ /*!
@Function       OSEventObjectDumpDebugInfo
@Description    Emits debug counters/stats related to the event object passed
@Input          hOSEventKM    the OS event object handle associated with
                              the event object.
@Return         None.
*/ /**************************************************************************/
void OSEventObjectDumpDebugInfo(MT_HANDLE hOSEventKM);

/*************************************************************************/ /*!
@Function       OSEventObjectOpen
@Description    Open an OS handle on the specified event object.
                This OS handle may then be used to make a thread wait for
                that event object to signal.
@Input          hEventObject    Event object handle.
@Output         phOSEvent       OS handle to the returned event object.
@Return         MTGPU_OK on success, a failure code otherwise.
*/ /**************************************************************************/
MTGPU_ERROR OSEventObjectOpen(MT_HANDLE hEventObject,
											MT_HANDLE *phOSEvent);

/*************************************************************************/ /*!
@Function       OSEventObjectClose
@Description    Close an OS handle previously opened for an event object.
@Input          hOSEventKM      OS event object handle to close.
@Return         MTGPU_OK on success, a failure code otherwise.
*/ /**************************************************************************/
MTGPU_ERROR OSEventObjectClose(MT_HANDLE hOSEventKM);

/*************************************************************************/ /*!
@Function      OSWaitus
@Description   Implements a busy wait of the specified number of microseconds.
               This function does NOT release thread quanta.
@Input         ui32Timeus     The duration of the wait period (in us)
@Return        None.
*/ /**************************************************************************/
void OSWaitus(MT_UINT32 ui32Timeus);

/*************************************************************************/ /*!
@Function       OSSleepms
@Description    Implements a sleep of the specified number of milliseconds.
                This function may allow pre-emption, meaning the thread
                may potentially not be rescheduled for a longer period.
@Input          ui32Timems    The duration of the sleep (in ms)
@Return         None.
*/ /**************************************************************************/
void OSSleepms(MT_UINT32 ui32Timems);

/*************************************************************************/ /*!
@Function       OSReleaseThreadQuanta
@Description    Relinquishes the current thread's execution time-slice,
                permitting the OS scheduler to schedule another thread.
@Return         None.
*/ /**************************************************************************/
void OSReleaseThreadQuanta(void);

#if defined(__linux__) && defined(__KERNEL__)
#define OSReadMemoryBarrier() rmb()
#else
/*************************************************************************/ /*!
@Function       OSReadMemoryBarrier
@Description    Insert a read memory barrier.
                The read memory barrier guarantees that all load (read)
                operations specified before the barrier will appear to happen
                before all of the load operations specified after the barrier.
*/ /**************************************************************************/
void OSReadMemoryBarrier(void);
#endif
/*************************************************************************/ /*!
@Function       OSMemoryBarrier
@Description    Insert a read/write memory barrier.
                The read and write memory barrier guarantees that all load
                (read) and all store (write) operations specified before the
                barrier will appear to happen before all of the load/store
                operations specified after the barrier.
@Input          hReadback     Optional pointer to memory to read back, can be
                useful for flushing queues in bus interconnects to RAM before
                device (GPU) access the shared memory.
@Return         None.
*/ /**************************************************************************/
void OSMemoryBarrier(volatile void *hReadback);
/*************************************************************************/ /*!
@Function       OSWriteMemoryBarrier
@Description    Insert a write memory barrier.
                The write memory barrier guarantees that all store operations
                (writes) specified before the barrier will appear to happen
                before all of the store operations specified after the barrier.
@Input          hReadback     Optional pointer to memory to read back, can be
                useful for flushing queues in bus interconnects to RAM before
                device (GPU) access the shared memory.
@Return         None.
*/ /**************************************************************************/
void OSWriteMemoryBarrier(volatile void *hReadback);

/*************************************************************************/ /*!
*/ /**************************************************************************/

/* The access method is dependent on the location of the physical memory that
 * makes up the PhyHeaps defined for the system and the CPU architecture. These
 * macros may change in future to accommodate different access requirements.
 */
/*! Performs a 32 bit word read from the device memory. */
#define OSReadDeviceMem32(addr)        (*((volatile MT_UINT32 __force *)((void*)addr)))
/*! Performs a 32 bit word write to the device memory. */
#define OSWriteDeviceMem32(addr, val)  (*((volatile MT_UINT32 __force *)((void*)addr)) = (MT_UINT32)(val))
/*! Performs a 32 bit word write to the device memory and issues a write memory barrier */
#define OSWriteDeviceMem32WithWMB(addr, val) \
	do { \
		*((volatile MT_UINT32 __force *)((void*)addr)) = (MT_UINT32)(val); \
		OSWriteMemoryBarrier(addr); \
	} while (0)

MT_UINT8 OSReadByte(const void __iomem *pvAddr);
MT_UINT16 OSReadWord(const void __iomem *pvAddr);
MT_UINT32 OSReadDWord(const void __iomem *pvAddr);
void OSWriteByte(MT_UINT8 ui8Val, void __iomem *pvAddr);
void OSWriteWord(MT_UINT16 ui16Val, void __iomem *pvAddr);
void OSWriteDWord(MT_UINT32 ui32Val, void __iomem *pvAddr);

#if defined(__linux__) && defined(__KERNEL__) && !defined(NO_HARDWARE)
	#define OSReadHWReg8(addr, off)  ((MT_UINT8)OSReadByte((MT_BYTE __iomem *)(addr) + off))
	#define OSReadHWReg16(addr, off) ((MT_UINT16)OSReadWord((MT_BYTE __iomem *)(addr) + off))
	#define OSReadHWReg32(addr, off) ((MT_UINT32)OSReadDWord((MT_BYTE __iomem *)(addr) + off))

	/* Little endian support only */
	#define OSReadHWReg64(addr, off) \
			({ \
				__typeof__(addr) _addr = addr; \
				__typeof__(off) _off = off; \
				(MT_UINT64) \
				( \
					( (MT_UINT64)(OSReadDWord((MT_BYTE __iomem *)(_addr) + _off + 4)) << 32) \
					| OSReadDWord((MT_BYTE __iomem *)(_addr) + _off) \
				); \
			})

	#define OSWriteHWReg8(addr, off, val)  OSWriteByte((MT_UINT8)(val), (MT_BYTE __iomem *)(addr) + off)
	#define OSWriteHWReg16(addr, off, val) OSWriteWord((MT_UINT16)(val), (MT_BYTE __iomem *)(addr) + off)
	#define OSWriteHWReg32(addr, off, val) OSWriteDWord((MT_UINT32)(val), (MT_BYTE __iomem *)(addr) + off)
	/* Little endian support only */
	#define OSWriteHWReg64(addr, off, val) do \
			{ \
				__typeof__(addr) _addr = addr; \
				__typeof__(off) _off = off; \
				__typeof__(val) _val = val; \
				OSWriteDWord((MT_UINT32)((_val) & 0xffffffff), (MT_BYTE __iomem *)(_addr) + _off); \
				OSWriteDWord((MT_UINT32)(((MT_UINT64)(_val) >> 32) & 0xffffffff), (MT_BYTE __iomem *)(_addr) + _off + 4); \
			} while (0)


#elif defined(NO_HARDWARE)
	/* OSReadHWReg operations skipped in no hardware builds */
	#define OSReadHWReg8(addr, off)  ((void)(addr), 0x4eU)
	#define OSReadHWReg16(addr, off) ((void)(addr), 0x3a4eU)
	#define OSReadHWReg32(addr, off) ((void)(addr), 0x30f73a4eU)
#if defined(__QNXNTO__) && __SIZEOF_LONG__ == 8
	/* This is needed for 64-bit QNX builds where the size of a long is 64 bits */
	#define OSReadHWReg64(addr, off) ((void)(addr), 0x5b376c9d30f73a4eUL)
#else
	#define OSReadHWReg64(addr, off) ((void)(addr), 0x5b376c9d30f73a4eULL)
#endif

	#define OSWriteHWReg8(addr, off, val)
	#define OSWriteHWReg16(addr, off, val)
	#define OSWriteHWReg32(addr, off, val)
	#define OSWriteHWReg64(addr, off, val)

#else
/*************************************************************************/ /*!
@Function       OSReadHWReg8
@Description    Read from an 8-bit memory-mapped device register.
                The implementation should not permit the compiler to
                reorder the I/O sequence.
                The implementation should ensure that for a NO_HARDWARE
                build the code does not attempt to read from a location
                but instead returns a constant value.
@Input          pvLinRegBaseAddr   The virtual base address of the register
                                   block.
@Input          ui32Offset         The byte offset from the base address of
                                   the register to be read.
@Return         The byte read.
*/ /**************************************************************************/
	MT_UINT8 OSReadHWReg8(volatile void *pvLinRegBaseAddr, MT_UINT32 ui32Offset);

/*************************************************************************/ /*!
@Function       OSReadHWReg16
@Description    Read from a 16-bit memory-mapped device register.
                The implementation should not permit the compiler to
                reorder the I/O sequence.
                The implementation should ensure that for a NO_HARDWARE
                build the code does not attempt to read from a location
                but instead returns a constant value.
@Input          pvLinRegBaseAddr   The virtual base address of the register
                                   block.
@Input          ui32Offset         The byte offset from the base address of
                                   the register to be read.
@Return         The word read.
*/ /**************************************************************************/
	MT_UINT16 OSReadHWReg16(volatile void *pvLinRegBaseAddr, MT_UINT32 ui32Offset);

/*************************************************************************/ /*!
@Function       OSReadHWReg32
@Description    Read from a 32-bit memory-mapped device register.
                The implementation should not permit the compiler to
                reorder the I/O sequence.
                The implementation should ensure that for a NO_HARDWARE
                build the code does not attempt to read from a location
                but instead returns a constant value.
@Input          pvLinRegBaseAddr   The virtual base address of the register
                                   block.
@Input          ui32Offset         The byte offset from the base address of
                                   the register to be read.
@Return         The long word read.
*/ /**************************************************************************/
	MT_UINT32 OSReadHWReg32(volatile void *pvLinRegBaseAddr, MT_UINT32 ui32Offset);

/*************************************************************************/ /*!
@Function       OSReadHWReg64
@Description    Read from a 64-bit memory-mapped device register.
                The implementation should not permit the compiler to
                reorder the I/O sequence.
                The implementation should ensure that for a NO_HARDWARE
                build the code does not attempt to read from a location
                but instead returns a constant value.
@Input          pvLinRegBaseAddr   The virtual base address of the register
                                   block.
@Input          ui32Offset         The byte offset from the base address of
                                   the register to be read.
@Return         The long long word read.
*/ /**************************************************************************/
	MT_UINT64 OSReadHWReg64(volatile void *pvLinRegBaseAddr, MT_UINT32 ui32Offset);

/*************************************************************************/ /*!
@Function       OSWriteHWReg8
@Description    Write to an 8-bit memory-mapped device register.
                The implementation should not permit the compiler to
                reorder the I/O sequence.
                The implementation should ensure that for a NO_HARDWARE
                build the code does not attempt to write to a location.
@Input          pvLinRegBaseAddr   The virtual base address of the register
                                   block.
@Input          ui32Offset         The byte offset from the base address of
                                   the register to be written to.
@Input          ui8Value           The byte to be written to the register.
@Return         None.
*/ /**************************************************************************/
	void OSWriteHWReg8(volatile void *pvLinRegBaseAddr, MT_UINT32 ui32Offset, MT_UINT8 ui8Value);

/*************************************************************************/ /*!
@Function       OSWriteHWReg16
@Description    Write to a 16-bit memory-mapped device register.
                The implementation should not permit the compiler to
                reorder the I/O sequence.
                The implementation should ensure that for a NO_HARDWARE
                build the code does not attempt to write to a location.
@Input          pvLinRegBaseAddr   The virtual base address of the register
                                   block.
@Input          ui32Offset         The byte offset from the base address of
                                   the register to be written to.
@Input          ui16Value          The word to be written to the register.
@Return         None.
*/ /**************************************************************************/
	void OSWriteHWReg16(volatile void *pvLinRegBaseAddr, MT_UINT32 ui32Offset, MT_UINT16 ui16Value);

/*************************************************************************/ /*!
@Function       OSWriteHWReg32
@Description    Write to a 32-bit memory-mapped device register.
                The implementation should not permit the compiler to
                reorder the I/O sequence.
                The implementation should ensure that for a NO_HARDWARE
                build the code does not attempt to write to a location.
@Input          pvLinRegBaseAddr   The virtual base address of the register
                                   block.
@Input          ui32Offset         The byte offset from the base address of
                                   the register to be written to.
@Input          ui32Value          The long word to be written to the register.
@Return         None.
*/ /**************************************************************************/
	void OSWriteHWReg32(volatile void *pvLinRegBaseAddr, MT_UINT32 ui32Offset, MT_UINT32 ui32Value);

/*************************************************************************/ /*!
@Function       OSWriteHWReg64
@Description    Write to a 64-bit memory-mapped device register.
                The implementation should not permit the compiler to
                reorder the I/O sequence.
                The implementation should ensure that for a NO_HARDWARE
                build the code does not attempt to write to a location.
@Input          pvLinRegBaseAddr   The virtual base address of the register
                                   block.
@Input          ui32Offset         The byte offset from the base address of
                                   the register to be written to.
@Input          ui64Value          The long long word to be written to the
                                   register.
@Return         None.
*/ /**************************************************************************/
	void OSWriteHWReg64(volatile void *pvLinRegBaseAddr, MT_UINT32 ui32Offset, MT_UINT64 ui64Value);
#endif

/*************************************************************************/ /*!
@Description    Pointer to a timer callback function.
@Input          pvData  Pointer to timer specific data.
*/ /**************************************************************************/
typedef void (*PFN_TIMER_FUNC)(void* pvData);

/*************************************************************************/ /*!
@Function       OSAddTimer
@Description    OS specific function to install a timer callback. The
                timer will then need to be enabled, as it is disabled by
                default.
                When enabled, the callback will be invoked once the specified
                timeout has elapsed.
@Input          pfnTimerFunc    Timer callback
@Input          *pvData         Callback data
@Input          ui32MsTimeout   Callback period
@Return         Valid handle on success, NULL if a failure
*/ /**************************************************************************/
MT_HANDLE OSAddTimer(PFN_TIMER_FUNC pfnTimerFunc, void *pvData, MT_UINT32 ui32MsTimeout);

/*************************************************************************/ /*!
@Function       OSRemoveTimer
@Description    Removes the specified timer. The handle becomes invalid and
                should no longer be used.
@Input          hTimer          handle of the timer to be removed
@Return         MTGPU_OK on success, a failure code otherwise.
*/ /**************************************************************************/
MTGPU_ERROR OSRemoveTimer(MT_HANDLE hTimer);

/*************************************************************************/ /*!
@Function       OSEnableTimer
@Description    Enable the specified timer. after enabling, the timer will
                invoke the associated callback at an interval determined by
                the configured timeout period until disabled.
@Input          hTimer          handle of the timer to be enabled
@Return         MTGPU_OK on success, a failure code otherwise.
*/ /**************************************************************************/
MTGPU_ERROR OSEnableTimer(MT_HANDLE hTimer);

/*************************************************************************/ /*!
@Function       OSDisableTimer
@Description    Disable the specified timer
@Input          hTimer          handle of the timer to be disabled
@Return         MTGPU_OK on success, a failure code otherwise.
*/ /**************************************************************************/
MTGPU_ERROR OSDisableTimer(MT_HANDLE hTimer);


/*************************************************************************/ /*!
 @Function      OSPanic
 @Description   Take action in response to an unrecoverable driver error
 @Return        None
*/ /**************************************************************************/
void OSPanic(void);

/*************************************************************************/ /*!
@Function       OSCopyToUser
@Description    Copy data to user-addressable memory from kernel-addressable
                memory.
                Note that pvDest may be an invalid address or NULL and the
                function should return an error in this case.
                For operating systems that do not have a user/kernel space
                distinction, this function should be implemented as a stub
                which simply returns MTGPU_ERROR_NOT_SUPPORTED.
@Input          pvProcess        handle of the connection
@Input          pvDest           pointer to the destination User memory
@Input          pvSrc            pointer to the source Kernel memory
@Input          ui32Bytes        size of the data to be copied
@Return         MTGPU_OK on success, a failure code otherwise.
*/ /**************************************************************************/
MTGPU_ERROR OSCopyToUser(void *pvProcess, void __user *pvDest, const void *pvSrc, size_t ui32Bytes);

/*************************************************************************/ /*!
@Function       OSCopyFromUser
@Description    Copy data from user-addressable memory to kernel-addressable
                memory.
                Note that pvSrc may be an invalid address or NULL and the
                function should return an error in this case.
                For operating systems that do not have a user/kernel space
                distinction, this function should be implemented as a stub
                which simply returns MTGPU_ERROR_NOT_SUPPORTED.
@Input          pvProcess        handle of the connection
@Input          pvDest           pointer to the destination Kernel memory
@Input          pvSrc            pointer to the source User memory
@Input          ui32Bytes        size of the data to be copied
@Return         MTGPU_OK on success, a failure code otherwise.
*/ /**************************************************************************/
MTGPU_ERROR OSCopyFromUser(void *pvProcess, void *pvDest, const void __user *pvSrc, size_t ui32Bytes);

#if defined(__linux__) || defined(INTEGRITY_OS)
#define OSBridgeCopyFromUser OSCopyFromUser
#define OSBridgeCopyToUser OSCopyToUser
#else
/*************************************************************************/ /*!
@Function       OSBridgeCopyFromUser
@Description    Copy data from user-addressable memory into kernel-addressable
                memory as part of a bridge call operation.
                For operating systems that do not have a user/kernel space
                distinction, this function will require whatever implementation
                is needed to pass data for making the bridge function call.
                For operating systems which do have a user/kernel space
                distinction (such as Linux) this function may be defined so
                as to equate to a call to OSCopyFromUser().
@Input          pvProcess        handle of the connection
@Input          pvDest           pointer to the destination Kernel memory
@Input          pvSrc            pointer to the source User memory
@Input          ui32Bytes        size of the data to be copied
@Return         MTGPU_OK on success, a failure code otherwise.
*/ /**************************************************************************/
MTGPU_ERROR OSBridgeCopyFromUser (void *pvProcess,
						void *pvDest,
						const void *pvSrc,
						size_t ui32Bytes);

/*************************************************************************/ /*!
@Function       OSBridgeCopyToUser
@Description    Copy data to user-addressable memory from kernel-addressable
                memory as part of a bridge call operation.
                For operating systems that do not have a user/kernel space
                distinction, this function will require whatever implementation
                is needed to pass data for making the bridge function call.
                For operating systems which do have a user/kernel space
                distinction (such as Linux) this function may be defined so
                as to equate to a call to OSCopyToUser().
@Input          pvProcess        handle of the connection
@Input          pvDest           pointer to the destination User memory
@Input          pvSrc            pointer to the source Kernel memory
@Input          ui32Bytes        size of the data to be copied
@Return         MTGPU_OK on success, a failure code otherwise.
*/ /**************************************************************************/
MTGPU_ERROR OSBridgeCopyToUser (void *pvProcess,
						void *pvDest,
						const void *pvSrc,
						size_t ui32Bytes);
#endif

/* To be increased if required in future */
#define MTGPU_MAX_BRIDGE_IN_SIZE      0x2000    /*!< Size of the memory block used to hold data passed in to a bridge call */
#define MTGPU_MAX_BRIDGE_OUT_SIZE     0x1000    /*!< Size of the memory block used to hold data returned from a bridge call */

/*************************************************************************/ /*!
@Function       OSPlatformBridgeInit
@Description    Called during device creation to allow the OS port to register
                other bridge modules and related resources that it requires.
@Return         MTGPU_OK on success, a failure code otherwise.
*/ /**************************************************************************/
MTGPU_ERROR OSPlatformBridgeInit(void);

/*************************************************************************/ /*!
@Function       OSPlatformBridgeDeInit
@Description    Called during device destruction to allow the OS port to
                deregister its OS specific bridges and clean up other
                related resources.
*/ /**************************************************************************/
void OSPlatformBridgeDeInit(void);

/*************************************************************************/ /*!
@Function       MTGPUToNativeError
@Description    Returns the OS-specific equivalent error number/code for
                the specified MTGPU_ERROR value.
                If there is no equivalent, or the MTGPU_ERROR value is
                MTGPU_OK (no error), 0 is returned.
@Return         The OS equivalent error code.
*/ /**************************************************************************/
int MTGPUToNativeError(MTGPU_ERROR e);
/** See MTGPUToNativeError(). */
#define OSMTGPUToNativeError(e) ( (MTGPU_OK == e)? 0: MTGPUToNativeError(e) )


#if defined(__linux__) && defined(__KERNEL__)

/* Provide LockDep friendly definitions for Services RW locks */
MTGPU_ERROR OSWRLockCreate(POSWR_LOCK *ppsLock);
void OSWRLockDestroy(POSWR_LOCK psLock);
void OSWRLockAcquireRead(POSWR_LOCK psLock);
void OSWRLockAcquireReadNested(POSWR_LOCK psLock, MT_UINT uiSubclass);
void OSWRLockReleaseRead(POSWR_LOCK psLock);
void OSWRLockAcquireWrite(POSWR_LOCK psLock);
void OSWRLockReleaseWrite(POSWR_LOCK psLock);

#elif defined(__linux__) || defined(__QNXNTO__) || defined(INTEGRITY_OS)
/* User-mode unit tests use these definitions on Linux */

MTGPU_ERROR OSWRLockCreate(POSWR_LOCK *ppsLock);
void OSWRLockDestroy(POSWR_LOCK psLock);
void OSWRLockAcquireRead(POSWR_LOCK psLock);
#define OSWRLockAcquireReadNested(psLock, subclass) OSWRLockAcquireRead((psLock))
void OSWRLockReleaseRead(POSWR_LOCK psLock);
void OSWRLockAcquireWrite(POSWR_LOCK psLock);
void OSWRLockReleaseWrite(POSWR_LOCK psLock);

#else

/*! Function not implemented definition. */
#define OSFUNC_NOT_IMPLEMENTED 0
/*! Assert used for OSFUNC_NOT_IMPLEMENTED. */
#define OSFUNC_NOT_IMPLEMENTED_ASSERT() MT_ASSERT(OSFUNC_NOT_IMPLEMENTED)

/*************************************************************************/ /*!
@Function       OSWRLockCreate
@Description    Create a writer/reader lock.
                This type of lock allows multiple concurrent readers but
                only a single writer, allowing for optimized performance.
@Output         ppsLock     A handle to the created WR lock.
@Return         MTGPU_OK on success, a failure code otherwise.
*/ /**************************************************************************/
static INLINE MTGPU_ERROR OSWRLockCreate(POSWR_LOCK *ppsLock)
{
	MT_UNREFERENCED_PARAMETER(ppsLock);

	OSFUNC_NOT_IMPLEMENTED_ASSERT();

	return MTGPU_ERROR_NOT_IMPLEMENTED;
}

/*************************************************************************/ /*!
@Function       OSWRLockDestroy
@Description    Destroys a writer/reader lock.
@Input          psLock     The handle of the WR lock to be destroyed.
@Return         None.
*/ /**************************************************************************/
static INLINE void OSWRLockDestroy(POSWR_LOCK psLock)
{
	MT_UNREFERENCED_PARAMETER(psLock);
	OSFUNC_NOT_IMPLEMENTED_ASSERT();
}

/*************************************************************************/ /*!
@Function       OSWRLockAcquireRead
@Description    Acquire a writer/reader read lock.
                If the write lock is already acquired, the caller will
                block until it is released.
@Input          psLock     The handle of the WR lock to be acquired for
                           reading.
@Return         None.
*/ /**************************************************************************/
static INLINE void OSWRLockAcquireRead(POSWR_LOCK psLock)
{
	MT_UNREFERENCED_PARAMETER(psLock);
	OSFUNC_NOT_IMPLEMENTED_ASSERT();
}

/*************************************************************************/ /*!
@Function       OSWRLockAcquireReadNested
@Description    Acquire a nested writer/reader read lock.
                If the write lock is already acquired, the caller will
                block until it is released.
                For operating systems other than Linux, this equates to an
                OSWRLockAcquireRead() call. On Linux, this function wraps a call
                to down_read_nested(). This recognises the scenario where
                there may be multiple subclasses within a particular class
                of lock. In such cases, the order in which the locks belonging
                these various subclasses are acquired is important and must be
                validated.
@Input          psLock     The handle of the WR lock to be acquired for
                           reading.
@Input          iSubclass  The subclass of the lock.
@Return         None.
*/ /**************************************************************************/
static INLINE void OSWRLockAcquireReadNested(POSWR_LOCK psLock, MT_INT iSubclass)
{
	MT_UNREFERENCED_PARAMETER(psLock);
	MT_UNREFERENCED_PARAMETER(iSubclass);
	OSFUNC_NOT_IMPLEMENTED_ASSERT();
}

/*************************************************************************/ /*!
@Function       OSWRLockReleaseRead
@Description    Release a writer/reader read lock.
@Input          psLock     The handle of the WR lock whose read lock is to
                           be released.
@Return         None.
*/ /**************************************************************************/
static INLINE void OSWRLockReleaseRead(POSWR_LOCK psLock)
{
	MT_UNREFERENCED_PARAMETER(psLock);
	OSFUNC_NOT_IMPLEMENTED_ASSERT();
}

/*************************************************************************/ /*!
@Function       OSWRLockAcquireWrite
@Description    Acquire a writer/reader write lock.
                If the write lock or any read lock are already acquired,
                the caller will block until all are released.
@Input          psLock     The handle of the WR lock to be acquired for
                           writing.
@Return         None.
*/ /**************************************************************************/
static INLINE void OSWRLockAcquireWrite(POSWR_LOCK psLock)
{
	MT_UNREFERENCED_PARAMETER(psLock);
	OSFUNC_NOT_IMPLEMENTED_ASSERT();
}

/*************************************************************************/ /*!
@Function       OSWRLockReleaseWrite
@Description    Release a writer/reader write lock.
@Input          psLock     The handle of the WR lock whose write lock is to
                           be released.
@Return         None
*/ /**************************************************************************/
static INLINE void OSWRLockReleaseWrite(POSWR_LOCK psLock)
{
	MT_UNREFERENCED_PARAMETER(psLock);
	OSFUNC_NOT_IMPLEMENTED_ASSERT();
}
#endif

/*************************************************************************/ /*!
@Function       OSDivideU64rU64
@Description    Divide a 64-bit value by a 64-bit value. Return the 64-bit
                quotient.
                The remainder is also returned in 'pui64Remainder'.
@Input          ui64Divident        The number to be divided.
@Input          ui64Divisor         The 64-bit value 'ui64Divident' is to
                                    be divided by.
@Output         pui64Remainder      The remainder of the division.
@Return         The 64-bit quotient (result of the division).
*/ /**************************************************************************/
MT_UINT64 OSDivideU64rU64(MT_UINT64 ui64Divident, MT_UINT64 ui64Divisor, MT_UINT64 *pui64Remainder);

/*************************************************************************/ /*!
@Function       OSDivide64r64
@Description    Divide a 64-bit value by a 32-bit value. Return the 64-bit
                quotient.
                The remainder is also returned in 'pui32Remainder'.
@Input          ui64Divident        The number to be divided.
@Input          ui32Divisor         The 32-bit value 'ui64Divident' is to
                                    be divided by.
@Output         pui32Remainder      The remainder of the division.
@Return         The 64-bit quotient (result of the division).
*/ /**************************************************************************/
MT_UINT64 OSDivide64r64(MT_UINT64 ui64Divident, MT_UINT32 ui32Divisor, MT_UINT32 *pui32Remainder);

/*************************************************************************/ /*!
@Function       OSDivide64
@Description    Divide a 64-bit value by a 32-bit value. Return a 32-bit
                quotient.
                The remainder is also returned in 'pui32Remainder'.
                This function allows for a more optional implementation
                of a 64-bit division when the result is known to be
                representable in 32-bits.
@Input          ui64Divident        The number to be divided.
@Input          ui32Divisor         The 32-bit value 'ui64Divident' is to
                                    be divided by.
@Output         pui32Remainder      The remainder of the division.
@Return         The 32-bit quotient (result of the division).
*/ /**************************************************************************/
MT_UINT32 OSDivide64(MT_UINT64 ui64Divident, MT_UINT32 ui32Divisor, MT_UINT32 *pui32Remainder);

/*************************************************************************/ /*!
@Function       OSDumpStack
@Description    Dump the current task information and its stack trace.
@Return         None
*/ /**************************************************************************/
void OSDumpStack(void);

/*************************************************************************/ /*!
@Function       OSUserModeAccessToPerfCountersEn
@Description    Permit User-mode access to CPU performance counter
                registers.
                This function is called during device initialisation.
                Certain CPU architectures may need to explicitly permit
                User mode access to performance counters - if this is
                required, the necessary code should be implemented inside
                this function.
@Return         None.
*/ /**************************************************************************/
void OSUserModeAccessToPerfCountersEn(void);

/*************************************************************************/ /*!
@Function       OSDebugSignalPID
@Description    Sends a SIGTRAP signal to a specific PID in user mode for
                debugging purposes. The user mode process can register a handler
                against this signal.
                This is necessary to support the Rogue debugger. If the Rogue
                debugger is not used then this function may be implemented as
                a stub.
@Input          ui32PID    The PID for the signal.
@Return         MTGPU_OK on success, a failure code otherwise.
*/ /**************************************************************************/
MTGPU_ERROR OSDebugSignalPID(MT_UINT32 ui32PID);

#if defined(__linux__) && defined(__KERNEL__) && !defined(DOXYGEN)
int OSWarnOn(MT_BOOL bCondition);
#else
/*************************************************************************/ /*!
@Function       OSWarnOn
@Description    This API allows the driver to emit a special token and stack
                dump to the server log when an issue is detected that needs the
                OS to be notified. The token or call may be used to trigger
                log collection by the OS environment.
                MT_DPF log messages will have been emitted prior to this call.
@Input          a    Expression to evaluate, if true trigger Warn signal
@Return         None
*/ /**************************************************************************/
#define OSWarnOn(a) do { if ((a)) { OSDumpStack(); } } while (0)
#endif

/*************************************************************************/ /*!
@Function       OSIsKernelThread
@Description    This API determines if the current running thread is a kernel
                thread (i.e. one not associated with any userland process,
                typically an MISR handler.)
@Return         MT_TRUE if it is a kernel thread, otherwise MT_FALSE.
*/ /**************************************************************************/
MT_BOOL OSIsKernelThread(void);

/*************************************************************************/ /*!
@Function       OSThreadDumpInfo
@Description    Traverse the thread list and call each of the stored
                callbacks to dump the info in debug_dump.
@Input          pfnDumpDebugPrintf  The 'printf' function to be called to
                                    display the debug info
@Input          pvDumpDebugFile     Optional file identifier to be passed to
                                    the 'printf' function if required
*/ /**************************************************************************/
void OSThreadDumpInfo(DUMPDEBUG_PRINTF_FUNC* pfnDumpDebugPrintf,
                      void *pvDumpDebugFile);

/*************************************************************************/ /*!
@Function       OSDumpVersionInfo
@Description    Store OS version information in debug dump.
@Input          pfnDumpDebugPrintf  The 'printf' function to be called to
                                    display the debug info
@Input          pvDumpDebugFile     Optional file identifier to be passed to
                                    the 'printf' function if required
*/ /**************************************************************************/
void OSDumpVersionInfo(DUMPDEBUG_PRINTF_FUNC *pfnDumpDebugPrintf,
                       void *pvDumpDebugFile);

/*************************************************************************/ /*!
@Function       OSIsWriteCombineUnalignedSafe
@Description    Determine if unaligned accesses to write-combine memory are
                safe to perform, i.e. whether we are safe from a CPU fault
                occurring. This test is specifically aimed at ARM64 platforms
                which cannot provide this guarantee if the memory is 'device'
                memory rather than 'normal' under the ARM memory architecture.
@Return         MT_TRUE if safe, MT_FALSE otherwise.
*/ /**************************************************************************/
MT_BOOL OSIsWriteCombineUnalignedSafe(void);

/*************************************************************************/ /*!
@Function       OSDebugLevel
@Description    Returns current value of the debug level.
@Return         Debug level.
*/ /**************************************************************************/
MT_UINT32 OSDebugLevel(void);

/*************************************************************************/ /*!
@Function       MTGPUSetDebugLevel
@Description    Sets the current value of the debug level to ui32DebugLevel.
@Input          ui32DebugLevel New debug level value.
*/ /**************************************************************************/
void OSSetDebugLevel(MT_UINT32 ui32DebugLevel);

/*************************************************************************/ /*!
@Function       MTGPUIsDebugLevel
@Description    Tests if a given debug level is enabled.
@Input          ui32DebugLevel MT_TRUE if debug level is enabled
                and MT_FALSE otherwise.
*/ /**************************************************************************/
MT_BOOL OSIsDebugLevel(MT_UINT32 ui32DebugLevel);

#if defined(SUPPORT_DMA_TRANSFER)

typedef void (*PFN_SERVER_CLEANUP)(void *pvData, MT_BOOL bAdvanceTimeline);

#define DMA_COMPLETION_TIMEOUT_MS 60000
#define DMA_ERROR_SYNC_RETRIES 100

MTGPU_ERROR OSDmaPrepareTransfer(MTGPU_DEVICE_NODE *psDevNode, void *psChan,
							   MT_DMA_ADDR* psDmaAddr, MT_UINT64* puiAddress,
							   MT_UINT64 uiSize, MT_BOOL bMemToDev,
							   MT_HANDLE pvOSData,
							   MT_HANDLE pvServerCleanupParam,PFN_SERVER_CLEANUP pfnServerCleanup, MT_BOOL bFirst);

MTGPU_ERROR OSDmaPrepareTransferSparse(MTGPU_DEVICE_NODE *psDevNode, MT_HANDLE pvChan,
										MT_DMA_ADDR* psDmaAddr, MT_BOOL *pbValid,
										MT_UINT64* puiAddress, MT_UINT64 uiSize,
										MT_UINT32 uiOffsetInPage,
										MT_UINT32 ui32SizeInPages,
										MT_BOOL bMemToDev,
										MT_HANDLE pvOSData,
										MT_HANDLE pvServerCleanupParam, PFN_SERVER_CLEANUP pfnServerCleanup,
										MT_BOOL bFirst);

MTGPU_ERROR OSDmaAllocData(MTGPU_DEVICE_NODE *psDevNode,MT_UINT32 uiNumDMA, void **pvAllocedData);
MTGPU_ERROR OSDmaSubmitTransfer(MTGPU_DEVICE_NODE *psDevNode, void *pvOSData, void *psChan, MT_BOOL bSynchronous);
void OSDmaForceCleanup(MTGPU_DEVICE_NODE *psDevNode, void *pvChan,
					   void *pvOSData, MT_HANDLE pvServerCleanupParam,
					   PFN_SERVER_CLEANUP pfnServerCleanup);
#endif

MT_INT OSRoundDownPowOfTwo(MT_UINT32 n);

void __user *OSValidateAndGetCPUUserVA(PMR *psPMR,
				       MT_CPU_VIRTADDR pvAddress,
				       MT_DEVMEM_OFFSET_T uiOffset,
				       MT_DEVMEM_SIZE_T uiSize);

MT_CPU_VIRTADDR OSDmaAllocCoherent(struct device *pvOSDevice,
				    size_t uiSize,
				    dma_addr_t *pHandle);

void OSDmaFreeCoherent(struct device *pvOSDevice,
		       size_t uiSize,
		       MT_CPU_VIRTADDR pvVirtAddr,
		       dma_addr_t Handle);

MT_PID HostPid2ContainerPID(MT_PID hostPid);
MT_PCHAR OSGetProcessNameByPid(MT_PID uiPid);

struct pci_dev *OSGetPcieDeviceFromVendor(MT_UINT32 ui32VendorID,
					  MT_UINT32 ui32DeviceID,
					  struct pci_dev *psPCIDev);
MT_UINT16 OSGetPcieDeviceIDFromPdev(struct pci_dev *psPCIDev);
MT_UINT16 OSGetPcieDeviceID(MTGPU_DEVICE_NODE *psDeviceNode);

struct platform_device;
struct resource;
void *OSGetMtgpuDevice(struct platform_device *pdev);
struct platform_device *OSGetPlatformDevice(void *pvOSDevice);
struct resource *OSPlatformGetResourceIRQ(struct platform_device *psPlatformDevice, MT_UINT32 uiIndex);
MT_UINT64 OSGetResourceStart(struct resource *psResource);
void *OSGetPlatformDeviceParent(struct platform_device *pdev);
void *OSGetPlatformData(struct platform_device *pdev);

bool OSPMRuntimeEnabled(struct device *dev);
bool OSPMRuntimeStatusSuspended(struct device *dev);

#if !defined(NO_HARDWARE)
void OSDeviceConfigInit(MTGPU_DEVICE_CONFIG *psDevConfig,
			struct platform_device *pdev,
			struct resource *registers);

struct resource *OSRequestMemRes(void *pvOSDevice);

void OSReleaseMemRes(struct resource *res);
#endif /* if !defined(NO_HARDWARE) */

void *OSGetPcieDeviceFromOSDevice(struct device *psOSDevice);

struct page;

struct page *OSGetPageByIndex(struct page *psPage, MT_UINT32 ui32Index);
struct page *OSPhysToPage(MT_UINT64 ui64Paddr);
struct page *OSPfnToPage(MT_UINT64 ui64Paddr);
MT_UINT64 OSPageToPhys(struct page *psPage);
void *OSVMapCached(struct page **ppsPage, MT_UINT32 ui32PageNums);
void *OSVMapNoncached(struct page **ppsPage, MT_UINT32 ui32PageNums);
void OSVUnmap(void *pvPtr);
MT_UINT64 OSDmaMapPage(struct device *psDev, struct page *psPage,
			size_t uiOffset, size_t uiSize, int eDirection);
void OSDmaUnmapPage(struct device *psDev, MT_UINT64 ui64DmaAddr, size_t uiSize, int eDirection);
MT_UINT64 OSDmaMapResource(struct device *psDev, MT_UINT64 ui64CpuPhysAddr,
			    size_t uiSize, int eDirection,
			    unsigned long ulAttrs);
void OSDmaUnmapResource(struct device *psDev, MT_UINT64 ui64DmaAddr,
			size_t uiSize, int eDirection);
MT_INT OSDmaMappingError(struct device *psDev, MT_UINT64 ui64DmaAddr);
void OSSetPageReserved(struct page *psPage);
void OSClearPageReserved(struct page *psPage);
struct page *OSAllocPage(void);
void OSFreePage(struct page *psPage);
struct page *OSAllocSystemPagesNode(gfp_t gfp_flags, MT_UINT32 ui32NumaNode, MT_UINT32 ui32Order);
struct page *OSAllocSystemPages(gfp_t gfp_flags ,MT_UINT32 ui32Order);
MT_INT OSGetUserPages(MT_INT64 ui64Start, MT_INT iPageCount, struct page **ppsPages);
void OSPutPage(struct page *psPage);
void OSFreePages(struct page *psPage, MT_UINT32 ui32Order);
MT_UINT64 OSUntaggedAddr(MT_UINT64 ui64Addr);
void OSPgprotNoncached(void *sPageProt);
void OSPgprotWriteCombine(void *sPageProt);
int OSX86VendorIsAMD(void);
int OSX86VendorIsHygon(void);

MT_INT OSSetPagesArrayWC(struct page **ppsPagesArray, MT_INT iPageNum);
MT_INT OSSetPagesArrayWB(struct page **ppsPagesArray, MT_INT iPageNum);
MT_INT OSSetPagesArrayUC(struct page **ppsPagesArray, MT_INT iPageNum);

void *OSPageAddress(const struct page *psPage);

struct drm_device;
struct mt_drm_private *OSGetPrivateDataFromDrm(struct drm_device *psDrmDevice);
MTGPU_DEVICE_NODE *OSGetDeviceNodeFromDrm(struct drm_device *psDrmDevice);
struct drm_device *OSGetDrmFromDeviceNode(MTGPU_DEVICE_NODE *psDevNode);
MT_BOOL OSIsErr(const void *pvPtr);
long OSPtrErr(const void *pvPtr);
MT_INT OSSScanf(const MT_CHAR *pcBuf, const MT_CHAR *pcFmt, ...);
struct device *OSGetPcieDeviceFromDeviceNode(MTGPU_DEVICE_NODE *psDevNode);
bool OSDeviceIsPCI(struct device *psDev);
bool OSDevToNode(struct device *psDev);
struct device *OSGetOSDeviceFromDeviceNode(MTGPU_DEVICE_NODE *psDevNode);

struct bus_type;
struct bus_type *OSGetDevBusType(struct device *psDev);
MT_BOOL OSIsIOMMUOn(struct bus_type *psBusType);
MT_BOOL OSIsPcieEndpoint(struct device *psDdev);
MT_BOOL OSIsPcieRootPort(const struct device *psDev);
struct device *OSPciUpStreamBridge(struct device *psDev);
MT_BOOL OSIsPciSiblingDownstreamPort(struct device *psDev1, struct device *psDev2);

MT_INT64 OSGetKernelRealTimeSeconds(void);

void OSHardwareResetLockAcquire(void);
void OSHardwareResetLockRelease(void);

void OSMMapLockAcquire(void);
void OSMMapLockRelease(void);

struct file *OSGetFileFromDrmFile(void *pDRMFile);

void OSMtDrmDbg(unsigned int category, const char *format, ...);
void OSMtDrmPrintk(const char *level, const char *format, ...);
#define OS_MT_DRM_DEBUG(fmt, ...)							\
	OSMtDrmDbg(OS_MT_VAL(DRM_UT_CORE), fmt, ##__VA_ARGS__)
#define OS_MT_DRM_INFO(fmt, ...)							\
	OSMtDrmPrintk(KERN_INFO, fmt, ##__VA_ARGS__)
#define OS_MT_DRM_WARN(fmt, ...)							\
	OSMtDrmPrintk(KERN_WARNING, fmt, ##__VA_ARGS__)
#define OS_MT_DRM_ERROR(fmt, ...)							\
	OSMtDrmPrintk(KERN_ERR, "*ERROR* " fmt, ##__VA_ARGS__)

MT_ULONG OSGetVMAreaPageOffset(void *ps_vma);

struct idr;
int OSIdrCreate(struct idr **ppsIdr);
void OSIdrDestroy(struct idr *psIdr);
void OSIdrPreload(void);
void OSIdrPreloadEnd(void);
int OSIdrAlloc(struct idr *psIdr, void *pv, int iStart, int iEnd, gfp_t gfpFlags);
void *OSIdrFind(const struct idr *psIdr, unsigned long ulId);
void *OSIdrRemove(struct idr *psIdr, unsigned long ulId);
void *OSIdrReplace(struct idr *psIdr, void *pv, unsigned long ulId);
int OSIdrForEach(const struct idr *psIdr,
		 int (*pFn)(int iId, void *pv, void *pvData),
		 void *pvData);

struct wait_queue_head;

MTGPU_ERROR OSWaitQueueHeadCreate(struct wait_queue_head **psWaitQHead);
void OSWakeUp(struct wait_queue_head *ppsWaitQHead);
void OSWakeUpInterruptible(struct wait_queue_head *psWaitQHead);
void OSWakeUpAll(struct wait_queue_head *psWaitQHead);
void OSWaitQueueHeadDestroy(struct wait_queue_head *psWaitQHead);

MTGPU_ERROR OSReadWritelockCreate(void **ppvReadWriteLock);
void OSReadWriteLockInit(void *pvReadWriteLock);
void OSWriteLockBottomHalfDisable(void *pvReadWriteLock);
void OSWriteUnlockBottomHalfEnable(void *pvReadWriteLock);
void OSReadLockBottomHalfDisable(void *pvReadWriteLock);
void OSReadUnlockBottomHalfEnable(void *pvReadWriteLock);
void OSReadWriteLockDestroy(void *pvReadWriteLock);

bool OSTryToFreeze(void);

unsigned long OSMsecsToJiffies(const unsigned int msec);
unsigned long OSUsecsToJiffies(const unsigned int usec);

struct wait_queue_entry;

MTGPU_ERROR OSWaitQueueEntryCreate(struct wait_queue_entry **ppsWaitQEntry);
void OSWaitQueueEntryDestroy(struct wait_queue_entry *psWaitQEntry);
void OSFinishWait(struct wait_queue_head *psWaitQHead, struct wait_queue_entry *psWaitQEntry);
void OSPrepareToWait(struct wait_queue_head *psWaitQHead, struct wait_queue_entry *psWaitQEntry, int state);

int OSSignalPending(void);
long OSScheduleTimeout(long timeout);
void OSSchedule(void);

int OSPrintk(const char *fmt, ...);

void OSVFree(const void *addr);
size_t OSKSize(const void *objp);
void OSKFree(const void *x);
bool OSIsVMallocAddr(const void *x);
void *OSKMalloc(size_t size);
void *OSKMallocNode(size_t size, int node);
void *OSKZalloc(size_t size);
void *OSVMalloc(unsigned long size);
void *OSVMallocNode(unsigned long size, int node);
void *OSVZalloc(unsigned long size);

unsigned int OSBigEndian32ToCPU(unsigned int uiNum);

struct key;
enum key_being_used_for;
int OSVerifyPkcs7Signature(const void *pvData, size_t uiLen,
			   const void *pvRawPkcs7, size_t uiPkcs7Len,
			   struct key *psTrusted_keys,
			   enum key_being_used_for eUsage,
			   int (*pfnViewContent)(void *pvCtx,
						 const void *pvData, size_t uilen,
						 size_t uiAsn1hdrlen),
			   void *pvCtx);

struct firmware;
int OSRequestFirmware(const struct firmware **ppsFw, const char *pszName, struct device *psDevice);
void OSReleaseFirmware(const struct firmware *psFw);
size_t OSGetFirmwareSize(const struct firmware *psFw);
const MT_UINT8 *OSGetFirmwareData(const struct firmware *psFw);

ssize_t OSKernelWrite(void *pvFile, const char __user *pszBuf, size_t count, loff_t *psPos);

void *OSGetDmaBufPrivateData(struct dma_buf *psDmaBuf);
size_t OSGetDmaBufSize(struct dma_buf *psDmaBuf);
const struct dma_buf_ops *OSGetDmaBufOps(struct dma_buf *psDmaBuf);

struct sg_table;
unsigned int OSGetSgTableNents(struct sg_table *psSgTable);
struct scatterlist *OSGetSgTableSglist(struct sg_table *psSgTable);
struct scatterlist *OSGetSglistNext(struct scatterlist *psSg);
MT_UINT64 OSGetSglistDmaAddress(struct scatterlist *psSg);
void OSSetSglistDmaAddress(struct scatterlist *psSg, MT_UINT64 ui64DmaAddr);
MT_UINT64 OSGetSglistDmaLength(struct scatterlist *psSg);
void OSSetSglistDmaLength(struct scatterlist *psSg, MT_INT64 ui64DmaLength);
int OSSgTableCreate(struct sg_table **ppsSgTable);
int OSSgTableAllocList(struct sg_table *psTable, unsigned int uiNents);
void OSSgTableFreeList(struct sg_table *psSgTable);

struct dma_buf_attachment;
void *OSGetDmaBufPrivateDataByAttachment(struct dma_buf_attachment *psAttachment);
struct device *OSGetDmaBufAttachmentDev(struct dma_buf_attachment *psAttachment);
const struct dma_buf_ops *OSGetMtDmaBufOps(void);

#if defined(OS_STRUCT_DMA_BUF_OPS_HAS_MAP_ATOMIC) || \
    defined(OS_STRUCT_DMA_BUF_OPS_HAS_MAP) || \
    defined(OS_STRUCT_DMA_BUF_OPS_HAS_KMAP_ATOMIC) || \
    defined(OS_STRUCT_DMA_BUF_OPS_HAS_KMAP)
void *MTDmaBufOpsKMap(struct dma_buf *psDmaBuf, unsigned long uiPageNum);
#endif

int OSDmaBufBeginCpuAccess(struct dma_buf *psDmBuf, int eDirection);

#if defined(OS_STRUCT_DMA_BUF_MAP_EXIST)
#define iosys_map		dma_buf_map
#define iosys_map_set_vaddr	dma_buf_map_set_vaddr
#endif

#if defined(OS_STRUCT_DMA_BUF_MAP_EXIST) || defined(OS_STRUCT_IOSYS_MAP_EXIST)
struct iosys_map;
int MTDmaBufOpsVMap(struct dma_buf *psDmaBuf, struct iosys_map *psMap);
#else
void *MTDmaBufOpsVMap(struct dma_buf *psDmaBuf);
#endif

#if defined(OS_STRUCT_DMA_BUF_MAP_EXIST) || defined(OS_STRUCT_IOSYS_MAP_EXIST)
void MTDmaBufOpsVUnmap(struct dma_buf *psDmaBuf, struct iosys_map *psMap);
#else
void MTDmaBufOpsVUnmap(struct dma_buf *psDmaBuf, void *pvKernAddr);
#endif

void *OSGetDmaBufAddrFromPrivdata(void *pvPriv, size_t uiOffset);
int MTDmaBufOpsAttach(struct dma_buf *psDmaBuf,
#if defined(OS_DMA_BUF_OPS_ATTACH_HAS_DEVICE_ARG)
		       struct device *psDev,
#endif
		       struct dma_buf_attachment *psAttachment);
int OSDmaBufVmap(struct dma_buf *psDmaBuf, void *pvPriv, void **pvMap, void **vaddr);
int OSDmaBufKmap(struct dma_buf *psDmaBuf, int iValue, const char *szFunc);
int OSSetDmaBufValue(struct dma_buf *psDmaBuf, int iValue, void *pvMap, void *vaddr);
void OSDmaBufKunmap(struct dma_buf *psDmaBuf, unsigned long ulPageNum,
		    void *pvAddr);
void OSDmaBufVunmap(struct dma_buf *psDmBuf, void *pvPriv);
int OSIosysMapCreate(void *pvPriv);
void OSIosysMapDestroy(void *pvPriv);

int OSDmaBufEndCpuAccessDmaToDevice(struct dma_buf *psDmBuf);
int OSDmaBufEndCpuAccessDmaBidirectional(struct dma_buf *psDmBuf);
void OSDmaBufUnmapAttachment(struct dma_buf_attachment *psAttach,
			     struct sg_table *psSgTable);
struct sg_table *OSDmaBufMapAttachment(struct dma_buf_attachment *psAttach);

struct vm_area_struct;
int OSDmaBufMMap(struct dma_buf *psDmBuf, struct vm_area_struct *psVma,
		 unsigned long ulPgOff);

struct _PMR_DMA_BUF_DATA_;
struct dma_buf_attachment *OSGetAttachmentFromPrivData(struct _PMR_DMA_BUF_DATA_ *psPrivData);
struct dma_buf *OSGetDmaBufFromAttachment(struct dma_buf_attachment *psAttachment);
void OSDmaBufDetach(struct dma_buf *psDmBuf, struct dma_buf_attachment *psAttach);
void OSDmaBufPut(struct dma_buf *psDmBuf);

struct dma_buf_export_info;
int OSDmaBufExportInfoCreate(struct dma_buf_export_info **ppsDmaBufExportInfo);
void OSDmaBufExportInfoDestroy(struct dma_buf_export_info *psDmaBufExportInfo);
void OSSetDmaBufExportInfo(struct dma_buf_export_info *psDmaBufExportInfo, PMR *psPMR,
			   const struct dma_buf_ops *psOps, MT_DEVMEM_SIZE_T uiPMRSize);
struct dma_buf *OSDmaBufExport(const struct dma_buf_export_info *psExpInfo);
int OSGetDmaBufFd(struct dma_buf *psDmBuf);
struct dma_buf *OSDmaBufGet(int iFd);
void OSGetDmaBuf(struct dma_buf *psDmBuf);
struct dma_buf_attachment *OSDmaBufAttach(struct dma_buf *psDmBuf,
					  struct device *psDev);

bool OSIsErrOrNull(__force const void *pvPtr);

struct kernel_param;
void *OSGetKernelParamArg(const struct kernel_param *psKernelParam);

struct work_struct;
struct workqueue_struct;
typedef void (*work_func_t)(struct work_struct *work);
bool OSQueueWork(struct workqueue_struct *psWorkQueue, struct work_struct *psWork);
struct workqueue_struct *OSAllocWorkqueue(const char *pszFormat, unsigned int flags, int max_active);
struct workqueue_struct *OSCreateSingleThreadWorkqueue(const MT_CHAR *pszFormat);
void OSFlushWorkqueue(struct workqueue_struct *psWorkQueue);
void OSDestroyWorkqueue(struct workqueue_struct *psWorkQueue);
void *OSCreateWork(work_func_t pfnFunc);
void OSDestroyWork(struct work_struct *psWork);
void OSSetWorkDrvdata(struct work_struct *psWork, void *pvData);
void *OSGetWorkDrvdata(struct work_struct *psWork);

struct delayed_work;

struct workqueue_struct *OSCreateFreezableWorkqueue(char *name);
void *OSCreateDelayedWork(void);
void OSDestroyDelayedWork(struct delayed_work *dwork);
void OSInitDelayedWork(struct delayed_work *dwork, work_func_t func);
bool OSQueueDelayedWork(struct workqueue_struct *wq,
			struct delayed_work *dwork,
			unsigned long delay);
bool OSCancelDelayedWorkSync(struct delayed_work *dwork);
struct delayed_work *OSToDelayedWork(struct work_struct *work);

void OSBitmapSet(unsigned long *map, MT_UINT uiStart, MT_UINT uiNbits);
void OSBitmapClear(unsigned long *map, MT_UINT start, MT_UINT nbits);
unsigned long OSFindFirstZeroBit(const unsigned long *addr, unsigned long size);
unsigned long OSFindFirstBit(const unsigned long *addr, unsigned long size);

struct dma_fence;
struct dma_fence_ops;
struct dma_fence_cb;
typedef void (*dma_fence_func_t)(struct dma_fence *fence, struct dma_fence_cb *cb);

void *OSCreateDmaFence(void);
void OSDestroyDmaFence(struct dma_fence *dma_fence);
void OSSetDmaFenceDrvdata(struct dma_fence *dma_fence, void *data);
void *OSGetDmaFenceDrvdata(struct dma_fence *dma_fence);
void *OSCreateDmaFenceCB(void);
void OSDestroyDmaFenceCB(struct dma_fence_cb *dma_fence_cb);
void OSSetDmaFenceCBDrvdata(struct dma_fence_cb *dma_fence_cb, void *data);
void *OSGetDmaFenceCBDrvdata(struct dma_fence_cb *dma_fence_cb);

struct MTDmaFenceOps {
        const char * (*get_driver_name)(struct dma_fence *fence);
	const char * (*get_timeline_name)(struct dma_fence *fence);
	bool (*enable_signaling)(struct dma_fence *fence);
	bool (*signaled)(struct dma_fence *fence);
	signed long (*wait)(struct dma_fence *fence, bool intr, signed long timeout);
	void (*release)(struct dma_fence *fence);
	void (*fence_value_str)(struct dma_fence *fence, char *str, int size);
	void (*timeline_value_str)(struct dma_fence *fence, char *str, int size);
};

MT_UINT64 OSDmaFenceOpsInit(struct dma_fence_ops **ops,
                             const struct MTDmaFenceOps *mt_ops);
void OSDmaFenceInit(struct dma_fence *fence,
                    const struct dma_fence_ops *ops,
                    spinlock_t *lock, u64 context, u64 seqno);
MT_UINT64 OSDmaFenceContextAlloc(MT_UINT32 num);
struct dma_fence *OSDmaFenceGetStub(void);
struct dma_fence *OSDmaFenceGet(struct dma_fence *fence);
void OSDmaFenSignal(struct dma_fence *fence);
void OSDmaFencePut(struct dma_fence *fence);
bool OSDmaFenceIsSignaled(struct dma_fence *fence);
void *OSCreateDmaFenceCB(void);
void OSDestroyDmaFenceCB(struct dma_fence_cb *dma_fence_cb);
int OSDmaFenceAddCallback(struct dma_fence *fence, struct dma_fence_cb *cb,
                          dma_fence_func_t func);
void OSDmaFenceEnableSWSignaling(struct dma_fence *fence);
struct sync_file *OSSyncFileCreate(struct dma_fence *fence);
int OSGetUnusedFDFlags(unsigned flag);
void OSFDInstallSyncFile(int fd, struct sync_file *sync_file);
struct dma_fence *OSSyncFileGetFence(int fd);

struct dma_fence_array;

struct dma_fence *OSGetBaseFormDmaFenceArray(struct dma_fence_array *array);
struct dma_fence_array *OSDmaFenceArrayCreate(int num_fences,
                                              struct dma_fence **fences);

void OSArchLinearMapAttrSetCached(phys_addr_t addr, size_t size);
void OSArchLinearMapAttrSetUncached(phys_addr_t addr, size_t size);

#endif /* OSFUNC_H */
/******************************************************************************
 End of file (osfunc.h)
******************************************************************************/


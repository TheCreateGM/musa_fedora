/*************************************************************************/ /*!
@File
@Title          Global types for use by MT APIs
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Defines type aliases for use by MT APIs.
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

#ifndef MT_TYPES_H
#define MT_TYPES_H
#if defined(__cplusplus)
extern "C" {
#endif

/* To use C99 types and definitions, there are two special cases we need to
 * cater for:
 *
 * - Visual Studio: in VS2010 or later, some standard headers are available,
 *   and MSVC has its own built-in sized types. We can define the C99 types
 *   in terms of these.
 *
 * - Linux kernel code: C99 sized types are defined in <linux/types.h>, but
 *   some other features (like macros for constants or printf format
 *   strings) are missing, so we need to fill in the gaps ourselves.
 *
 * For other cases (userspace code under Linux, Android or Neutrino, or
 * firmware code), we can include the standard headers.
 */
#if defined(_MSC_VER)
	#include <stdbool.h>		/* bool */
	#include "msvc_types.h"
#elif defined(__linux__) && defined(__KERNEL__)
	#include "linux-types.h"
	#include "kernel_types.h"
#elif defined(__linux__) || defined(__MCUG) || defined(__MINGW32__) || \
	defined(__QNXNTO__) || defined(INTEGRITY_OS) || defined(__riscv)
	#include <stddef.h>			/* NULL */
	#include <stdint.h>
	#include <inttypes.h>		/* intX_t/uintX_t, format specifiers */
	#include <limits.h>			/* INT_MIN, etc */
	#include <stdbool.h>		/* bool */
#elif defined(__mips)
	#include <stddef.h>			/* NULL */
	#include <inttypes.h>		/* intX_t/uintX_t, format specifiers */
	#include <stdbool.h>		/* bool */
#else
	#error C99 support not set up for this build
#endif

#if defined(__linux__)
#ifndef va_arg
typedef __builtin_va_list va_list;
#define va_start(v, l)		__builtin_va_start(v, l)
#define va_end(v)               __builtin_va_end(v)
#define va_arg(v, l)            __builtin_va_arg(v, l)
#define va_copy(d, s)           __builtin_va_copy(d, s)
#endif
#else
#include <stdarg.h>
#endif

/*
 * Due to a Klocwork bug, 'true'/'false' constants are not recognized to be of
 * boolean type. This results in large number of false-positives being reported
 * (MISRA.ETYPE.ASSIGN.2012: "An expression value of essential type 'signed char'
 * is assigned to an object of essential type 'bool'"). Work around this by
 * redefining those constants with cast to bool added.
 */
#if defined(__KLOCWORK__) && !defined(__cplusplus)
#undef true
#undef false
#define true ((bool) 1)
#define false ((bool) 0)
#endif

typedef unsigned int	MT_UINT;
typedef int				MT_INT;
typedef unsigned long MT_ULONG;
typedef unsigned long long MT_ULLONG;

typedef uint8_t			MT_UINT8,	*MT_PUINT8;
typedef uint8_t			MT_BYTE,	*MT_PBYTE;
typedef int8_t			MT_INT8;
typedef char			MT_CHAR,	*MT_PCHAR;

typedef uint16_t		MT_UINT16,	*MT_PUINT16;
typedef int16_t			MT_INT16;
typedef uint32_t		MT_UINT32,	*MT_PUINT32;
typedef int32_t			MT_INT32,	*MT_PINT32;
#if defined(INTEGRITY_OS)
#if __INT_BIT >= 32U
#define MT_UINT32_C(n) ((MT_UINT32)(n ## U))
#elif __LONG_BIT >= 32U
#define MT_UINT32_C(n) ((MT_UINT32)(n ## UL))
#elif defined(__LLONG_BIT) && __LLONG_BIT >= 32U
#define MT_UINT32_C(n) ((MT_UINT32)(n ## ULL))
#endif
#else /* defined(INTEGRITY_OS) */
#define MT_UINT32_C(c) ((MT_UINT32)UINT32_C(c))
#endif /* defined(INTEGRITY_OS) */

typedef uint64_t		MT_UINT64,	*MT_PUINT64;
typedef int64_t			MT_INT64;
#define MT_INT64_C(c)	INT64_C(c)
#if defined(INTEGRITY_OS)
#if __INT_BIT >= 64U
#define MT_UINT64_C(n)	(n ## U)
#elif defined(__LONG_BIT) && __LONG_BIT >= 64U
#define MT_UINT64_C(n)	(n ## UL)
#elif defined(__LLONG_BIT) && __LLONG_BIT >= 64U
#define MT_UINT64_C(n)	(n ## ULL)
#endif
#else /* defined(INTEGRITY_OS) */
#define MT_UINT64_C(c)	UINT64_C(c)
#endif /* defined(INTEGRITY_OS) */
#define MT_UINT16_C(c)	UINT16_C(c)
#define MT_UINT64_FMTSPEC PRIu64
#define MT_UINT64_FMTSPECX PRIX64
#define MT_UINT64_FMTSPECx PRIx64
#define MT_UINT64_FMTSPECo PRIo64
#define MT_INT64_FMTSPECd PRId64

#define MT_UINT16_MAX	UINT16_MAX
#define MT_UINT32_MAX	UINT32_MAX
#define MT_UINT64_MAX	UINT64_MAX

#define MT_INT16_MAX	INT16_MAX
#define MT_INT32_MAX	INT32_MAX
#define MT_INT64_MAX	INT64_MAX

/* Linux kernel mode does not use floating point */
typedef float			MT_FLOAT,	*MT_PFLOAT;
typedef double			MT_DOUBLE;

typedef union
{
	MT_UINT32 ui32;
	MT_FLOAT f;
} MT_UINT32_FLOAT;

typedef int				MT_SECURE_TYPE;

typedef	enum tag_img_bool
{
	MT_FALSE		= 0,
	MT_TRUE		= 1,
	MT_FORCE_ALIGN = 0x7FFFFFFF
} MT_BOOL, *MT_PBOOL;

#if defined(UNDER_WDDM) || defined(WINDOWS_WDF)
typedef MT_CHAR const* MT_PCCHAR;
#endif

/* Format specifiers for 'size_t' type */
#if defined(_MSC_VER) || defined(__MINGW32__)
#define MT_SIZE_FMTSPEC  "%Iu"
#define MT_SIZE_FMTSPECX "%Ix"
#else
#define MT_SIZE_FMTSPEC  "%zu"
#define MT_SIZE_FMTSPECX "%zx"
#endif

#define MT_PFN_FMTSPEC "0x%lx"

typedef void           *MT_HANDLE;

/* Process IDs */
typedef MT_UINT32      MT_PID;

/* OS connection type */
typedef int             MT_OS_CONNECTION;


/*
 * Address types.
 * All types used to refer to a block of memory are wrapped in structures
 * to enforce some degree of type safety, i.e. a MT_DEV_VIRTADDR cannot
 * be assigned to a variable of type MT_DEV_PHYADDR because they are not the
 * same thing.
 *
 * There is an assumption that the system contains at most one non-cpu mmu,
 * and a memory block is only mapped by the MMU once.
 *
 * Different devices could have offset views of the physical address space.
 *
 */


/*
 *
 * +------------+    +------------+      +------------+        +------------+
 * |    CPU     |    |    DEV     |      |    DEV     |        |    DEV     |
 * +------------+    +------------+      +------------+        +------------+
 *       |                 |                   |                     |
 *       | void *          |uint64_t   |uint64_t     |
 *       |                 \-------------------/                     |
 *       |                          |                                |
 * +------------+             +------------+                         |
 * |    MMU     |             |    MMU     |                         |
 * +------------+             +------------+                         |
 *       |                          |                                |
 *       |                          |                                |
 *       |                          |                                |
 *   +--------+                +---------+                      +--------+
 *   | Offset |                | (Offset)|                      | Offset |
 *   +--------+                +---------+                      +--------+
 *       |                          |                MT_DEV_PHYADDR |
 *       |                          |                                |
 *       |                          | MT_DEV_PHYADDR                |
 * +---------------------------------------------------------------------+
 * |                         System Address bus                          |
 * +---------------------------------------------------------------------+
 *
 */

#define MT_DEV_VIRTADDR_FMTSPEC "0x%010" MT_UINT64_FMTSPECX
#define MT_DEVMEM_SIZE_FMTSPEC "0x%010" MT_UINT64_FMTSPECX
#define MT_DEVMEM_ALIGN_FMTSPEC "0x%010" MT_UINT64_FMTSPECX
#define MT_DEVMEM_OFFSET_FMTSPEC "0x%010" MT_UINT64_FMTSPECX

/* cpu physical address */
typedef MT_UINT64 MT_CPU_PHYADDR;

/* device physical address */
typedef MT_UINT64 MT_DEV_PHYADDR;

/* dma address */
typedef MT_UINT64 MT_DMA_ADDR;

/*
	rectangle structure
*/
typedef struct
{
	MT_INT32	x0;
	MT_INT32	y0;
	MT_INT32	x1;
	MT_INT32	y1;
} MT_RECT;

typedef struct
{
	MT_INT16	x0;
	MT_INT16	y0;
	MT_INT16	x1;
	MT_INT16	y1;
} MT_RECT_16;

/*
 * box structure
 */
typedef struct
{
	MT_INT32	x0;
	MT_INT32	y0;
	MT_INT32	z0;
	MT_INT32	x1;
	MT_INT32	y1;
	MT_INT32	z1;
} MT_BOX;

#if defined(__cplusplus)
}
#endif

typedef struct _MEM_REGION_INFO_
{
	MT_UINT64 startAddress;
	MT_UINT64 size;
} MEM_REGION_INFO;

typedef struct mtgpu_segment_info SEGMENT_INFO;

/* -------------------------
   Compatibility with IMG_*
*/
typedef MT_BOOL      IMG_BOOL;
typedef MT_CPU_PHYADDR IMG_CPU_PHYADDR;

#endif /* MT_TYPES_H */
/******************************************************************************
 End of file (img_types.h)
******************************************************************************/

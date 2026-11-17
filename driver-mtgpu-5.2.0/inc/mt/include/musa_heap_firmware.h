/*************************************************************************/ /*!
@File
@Title          MUSA FW heap definitions
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

#if !defined(MUSA_HEAP_FIRMWARE_H)
#define MUSA_HEAP_FIRMWARE_H

/* Start at 903GiB. Size of 32MB per OSID (see musaheapconfig.h)
 * NOTE:
 *      The firmware heaps bases and sizes are defined here to
 *      simplify #include dependencies, see musaheapconfig.h
 *      for the full MUSA virtual address space layout.
 */

/*
 * The Config heap holds initialisation data shared between the
 * the driver and firmware (e.g. pointers to the KCCB and FWCCB).
 * The Main Firmware heap size is adjusted accordingly but most
 * of the map / unmap functions must take into consideration
 * the entire range (i.e. main and config heap).
 */
#define MUSA_FIRMWARE_NUMBER_OF_FW_HEAPS              (MT_UINT32_C(2))
#define MUSA_FIRMWARE_HEAP_SHIFT                      MUSA_FW_HEAP_SHIFT
#define MUSA_FIRMWARE_RAW_HEAP_BASE                   (0xE1C0000000ULL)
#define MUSA_FIRMWARE_RAW_HEAP_SIZE                   (MT_UINT32_C(1) << MUSA_FIRMWARE_HEAP_SHIFT)
#define MUSA_FIRMWARE_HEAP_MAX_SHIFT                  28
#define MUSA_NUMBER_OF_RESERVED_HEAPS                 (MT_UINT32_C(5))


/* To enable the firmware to compute the exact address of structures allocated by the KM
 * in the Fw Config subheap, regardless of the KM's page size (and PMR granularity),
 * objects allocated consecutively but from different PMRs (due to differing memalloc flags)
 * are allocated with a 64kb offset. This way, all structures will be located at the same base
 * addresses when the KM is running with a page size of 4k, 16k or 64k.  */
#define MUSA_FIRMWARE_CONFIG_HEAP_ALLOC_GRANULARITY    (MT_UINT32_C(0x10000))

/* Ensure the heap can hold 3 PMRs of maximum supported granularity (192KB):
 * 1st PMR: MUSAFWIF_CONNECTION_CTL
 * 2nd PMR: MUSAFWIF_OSINIT
 * 3rd PMR: MUSAFWIF_SYSINIT */
#define MUSA_FIRMWARE_CONFIG_HEAP_SIZE                (MT_UINT32_C(3)*MUSA_FIRMWARE_CONFIG_HEAP_ALLOC_GRANULARITY)

#define MUSA_FIRMWARE_DEFAULT_MAIN_HEAP_SIZE          (MUSA_FIRMWARE_RAW_HEAP_SIZE - MUSA_FIRMWARE_CONFIG_HEAP_SIZE)
/*
 * MIPS FW needs space in the Main heap to map GPU memory.
 * This space is taken from the MAIN heap, to avoid creating a new heap.
 */
#define MUSA_FIRMWARE_MIPS_GPU_MAP_RESERVED_SIZE_NORMAL       (MT_UINT32_C(0x100000)) /* 1MB */
#define MUSA_FIRMWARE_MIPS_GPU_MAP_RESERVED_SIZE_BRN65101     (MT_UINT32_C(0x400000)) /* 4MB */

#define MUSA_FIRMWARE_HOST_MIPS_MAIN_HEAP_SIZE_NORMAL      (MUSA_FIRMWARE_RAW_HEAP_SIZE -  MUSA_FIRMWARE_CONFIG_HEAP_SIZE - \
                                                           MUSA_FIRMWARE_MIPS_GPU_MAP_RESERVED_SIZE_NORMAL)

#define MUSA_FIRMWARE_HOST_MIPS_MAIN_HEAP_SIZE_BRN65101    (MUSA_FIRMWARE_RAW_HEAP_SIZE -  MUSA_FIRMWARE_CONFIG_HEAP_SIZE - \
                                                           MUSA_FIRMWARE_MIPS_GPU_MAP_RESERVED_SIZE_BRN65101)

#if !defined(__KERNEL__)
#if defined(FIX_HW_BRN_65101)
#define MUSA_FIRMWARE_MIPS_GPU_MAP_RESERVED_SIZE      MUSA_FIRMWARE_MIPS_GPU_MAP_RESERVED_SIZE_BRN65101
#define MUSA_FIRMWARE_HOST_MIPS_MAIN_HEAP_SIZE        MUSA_FIRMWARE_HOST_MIPS_MAIN_HEAP_SIZE_BRN65101

#include "img_defs.h"
static_assert((MUSA_FIRMWARE_RAW_HEAP_SIZE) >= MT_UINT32_C(0x800000), "MIPS GPU map size cannot be increased due to BRN65101 with a small FW heap");

#else
#define MUSA_FIRMWARE_MIPS_GPU_MAP_RESERVED_SIZE      MUSA_FIRMWARE_MIPS_GPU_MAP_RESERVED_SIZE_NORMAL
#define MUSA_FIRMWARE_HOST_MIPS_MAIN_HEAP_SIZE        MUSA_FIRMWARE_HOST_MIPS_MAIN_HEAP_SIZE_NORMAL
#endif
#endif /* !defined(__KERNEL__) */

#define MUSA_FIRMWARE_MAIN_HEAP_BASE             MUSA_FIRMWARE_RAW_HEAP_BASE
#define MUSA_FIRMWARE_CONFIG_HEAP_BASE           (MUSA_FIRMWARE_MAIN_HEAP_BASE + \
                                                 MUSA_FIRMWARE_RAW_HEAP_SIZE - \
                                                 MUSA_FIRMWARE_CONFIG_HEAP_SIZE)

/*
 * The maximum configurable size via MUSA_FW_HEAP_SHIFT is 256MiB (1<<28) and
 * the minimum is 4MiB (1<<22); the default firmware heap size is set to
 * maximum 32MiB.
 */
#if defined(MUSA_FW_HEAP_SHIFT) && (MUSA_FW_HEAP_SHIFT < 22 || MUSA_FW_HEAP_SHIFT > 28)
#error "MUSA_FW_HEAP_SHIFT is outside valid range [22, 28]"
#endif

#ifdef SUPPORT_AUTOVZ
#define MUSA_FIRMWARE_RAW_HEAP_MAX_SIZE    (MT_UINT32_C(1) << MUSA_FIRMWARE_HEAP_MAX_SHIFT)
#define MUSA_FIRMWARE_RAW_HEAP_TOTAL_SIZE  (MTGPU_VZ_NUM_OSID * MUSA_FIRMWARE_RAW_HEAP_SIZE)
static_assert((MUSA_FIRMWARE_RAW_HEAP_TOTAL_SIZE) <= (MUSA_FIRMWARE_RAW_HEAP_MAX_SIZE),
	       "MUSA_FIRMWARE_RAW_HEAP_TOTAL_SIZE should be less than 256MB");
#endif

#endif /* MUSA_HEAP_FIRMWARE_H */

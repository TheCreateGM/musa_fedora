/*************************************************************************/ /*!
@File
@Title          MUSA MCU definitions
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    MUSA MCU helper definitions
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

#if !defined(MUSA_MCU_H)
#define MUSA_MCU_H


/***** The MCU HW register definitions in the file are updated manually *****/


#include "img_defs.h"
#include "km/musadefs_km.h"


/******************************************************************************
* MCU registers and MACROS
******************************************************************************/
#define MCU_CR_CTRLREG_BASE(T)					(0x04800000U + (0x1000U*(T)))

#define MCU_CR_TXPRIVEXT						(0x048000E8)
#define MCU_CR_TXPRIVEXT_MINIM_EN				(MT_UINT32_C(0x1) << 7)

#define MCU_CR_SYSC_JTAG_THREAD				(0x04830030)
#define MCU_CR_SYSC_JTAG_THREAD_PRIV_EN		(0x00000004)

#define MCU_CR_PERF_COUNT0						(0x0480FFE0)
#define MCU_CR_PERF_COUNT1						(0x0480FFE8)
#define MCU_CR_PERF_COUNT_CTRL_SHIFT			(28)
#define MCU_CR_PERF_COUNT_CTRL_MASK			(0xF0000000)
#define MCU_CR_PERF_COUNT_CTRL_DCACHEHITS		(MT_UINT32_C(0x8) << MCU_CR_PERF_COUNT_CTRL_SHIFT)
#define MCU_CR_PERF_COUNT_CTRL_ICACHEHITS		(MT_UINT32_C(0x9) << MCU_CR_PERF_COUNT_CTRL_SHIFT)
#define MCU_CR_PERF_COUNT_CTRL_ICACHEMISS		(MT_UINT32_C(0xA) << MCU_CR_PERF_COUNT_CTRL_SHIFT)
#define MCU_CR_PERF_COUNT_CTRL_ICORE			(MT_UINT32_C(0xD) << MCU_CR_PERF_COUNT_CTRL_SHIFT)
#define MCU_CR_PERF_COUNT_THR_SHIFT			(24)
#define MCU_CR_PERF_COUNT_THR_MASK				(0x0F000000)
#define MCU_CR_PERF_COUNT_THR_0				(MT_UINT32_C(0x1) << MCU_CR_PERF_COUNT_THR_SHIFT)
#define MCU_CR_PERF_COUNT_THR_1				(MT_UINT32_C(0x2) << MCU_CR_PERF_COUNT_THR_SHIFT)

#define MCU_CR_TxVECINT_BHALT					(0x04820500)
#define MCU_CR_PERF_ICORE0						(0x0480FFD0)
#define MCU_CR_PERF_ICORE1						(0x0480FFD8)
#define MCU_CR_PERF_ICORE_DCACHEMISS			(0x8)

#define MCU_CR_PERF_COUNT(CTRL, THR)			((MCU_CR_PERF_COUNT_CTRL_##CTRL << MCU_CR_PERF_COUNT_CTRL_SHIFT) | \
												 (THR << MCU_CR_PERF_COUNT_THR_SHIFT))

#define MCU_CR_TXUXXRXDT_OFFSET				(MCU_CR_CTRLREG_BASE(0U) + 0x0000FFF0U)
#define MCU_CR_TXUXXRXRQ_OFFSET				(MCU_CR_CTRLREG_BASE(0U) + 0x0000FFF8U)

#define MCU_CR_TXUXXRXRQ_DREADY_BIT			(0x80000000U)	/* Poll for done */
#define MCU_CR_TXUXXRXRQ_RDnWR_BIT				(0x00010000U)	/* Set for read  */
#define MCU_CR_TXUXXRXRQ_TX_S					(12)
#define MCU_CR_TXUXXRXRQ_RX_S					(4)
#define MCU_CR_TXUXXRXRQ_UXX_S					(0)

#define MCU_CR_TXUIN_ID						(0x0)			/* Internal ctrl regs */
#define MCU_CR_TXUD0_ID						(0x1)			/* Data unit regs */
#define MCU_CR_TXUD1_ID						(0x2)			/* Data unit regs */
#define MCU_CR_TXUA0_ID						(0x3)			/* Address unit regs */
#define MCU_CR_TXUA1_ID						(0x4)			/* Address unit regs */
#define MCU_CR_TXUPC_ID						(0x5)			/* PC registers */

/* Macros to calculate register access values */
#define MCU_CR_CORE_REG(Thr, RegNum, Unit)	(((MT_UINT32)(Thr)		<< MCU_CR_TXUXXRXRQ_TX_S) | \
											 ((MT_UINT32)(RegNum)	<< MCU_CR_TXUXXRXRQ_RX_S) | \
											 ((MT_UINT32)(Unit)	<< MCU_CR_TXUXXRXRQ_UXX_S))

#define MCU_CR_THR0_PC		MCU_CR_CORE_REG(0, 0, MCU_CR_TXUPC_ID)
#define MCU_CR_THR0_PCX	MCU_CR_CORE_REG(0, 1, MCU_CR_TXUPC_ID)
#define MCU_CR_THR0_SP		MCU_CR_CORE_REG(0, 0, MCU_CR_TXUA0_ID)

#define MCU_CR_THR1_PC		MCU_CR_CORE_REG(1, 0, MCU_CR_TXUPC_ID)
#define MCU_CR_THR1_PCX	MCU_CR_CORE_REG(1, 1, MCU_CR_TXUPC_ID)
#define MCU_CR_THR1_SP		MCU_CR_CORE_REG(1, 0, MCU_CR_TXUA0_ID)

#define SP_ACCESS(Thread)	MCU_CR_CORE_REG(Thread, 0, MCU_CR_TXUA0_ID)
#define PC_ACCESS(Thread)	MCU_CR_CORE_REG(Thread, 0, MCU_CR_TXUPC_ID)

#define MCU_CR_COREREG_ENABLE			(0x0000000U)
#define MCU_CR_COREREG_STATUS			(0x0000010U)
#define MCU_CR_COREREG_CATCH0			(0x0000080)
#define MCU_CR_COREREG_CATCH1			(0x0000088)
#define MCU_CR_COREREG_CATCH2			(0x0000090)
#define MCU_CR_COREREG_CATCH3			(0x0000098)
#define MCU_CR_COREREG_DEFR			(0x00000A0U)
#define MCU_CR_COREREG_PRIVEXT			(0x00000E8U)

#define MCU_CR_T0ENABLE_OFFSET			(MCU_CR_CTRLREG_BASE(0U) + MCU_CR_COREREG_ENABLE)
#define MCU_CR_T0STATUS_OFFSET			(MCU_CR_CTRLREG_BASE(0U) + MCU_CR_COREREG_STATUS)
#define MCU_CR_T0CATCH0_OFFSET			(MCU_CR_CTRLREG_BASE(0U) + MCU_CR_COREREG_CATCH0)
#define MCU_CR_T0CATCH1_OFFSET			(MCU_CR_CTRLREG_BASE(0U) + MCU_CR_COREREG_CATCH1)
#define MCU_CR_T0CATCH2_OFFSET			(MCU_CR_CTRLREG_BASE(0U) + MCU_CR_COREREG_CATCH2)
#define MCU_CR_T0CATCH3_OFFSET			(MCU_CR_CTRLREG_BASE(0U) + MCU_CR_COREREG_CATCH3)
#define MCU_CR_T0DEFR_OFFSET			(MCU_CR_CTRLREG_BASE(0U) + MCU_CR_COREREG_DEFR)
#define MCU_CR_T0PRIVEXT_OFFSET		(MCU_CR_CTRLREG_BASE(0U) + MCU_CR_COREREG_PRIVEXT)

#define MCU_CR_T1ENABLE_OFFSET			(MCU_CR_CTRLREG_BASE(1U) + MCU_CR_COREREG_ENABLE)
#define MCU_CR_T1STATUS_OFFSET			(MCU_CR_CTRLREG_BASE(1U) + MCU_CR_COREREG_STATUS)
#define MCU_CR_T1CATCH0_OFFSET			(MCU_CR_CTRLREG_BASE(1U) + MCU_CR_COREREG_CATCH0)
#define MCU_CR_T1CATCH1_OFFSET			(MCU_CR_CTRLREG_BASE(1U) + MCU_CR_COREREG_CATCH1)
#define MCU_CR_T1CATCH2_OFFSET			(MCU_CR_CTRLREG_BASE(1U) + MCU_CR_COREREG_CATCH2)
#define MCU_CR_T1CATCH3_OFFSET			(MCU_CR_CTRLREG_BASE(1U) + MCU_CR_COREREG_CATCH3)
#define MCU_CR_T1DEFR_OFFSET			(MCU_CR_CTRLREG_BASE(1U) + MCU_CR_COREREG_DEFR)
#define MCU_CR_T1PRIVEXT_OFFSET		(MCU_CR_CTRLREG_BASE(1U) + MCU_CR_COREREG_PRIVEXT)

#define MCU_CR_TXENABLE_ENABLE_BIT		(0x00000001U)   /* Set if running */
#define MCU_CR_TXSTATUS_PRIV			(0x00020000U)
#define MCU_CR_TXPRIVEXT_MINIM			(0x00000080U)

#define MCU_MEM_GLOBAL_RANGE_BIT		(0x80000000U)

#define MCU_CR_TXCLKCTRL          (0x048000B0)
#define MCU_CR_TXCLKCTRL_ALL_ON   (0x55111111)
#define MCU_CR_TXCLKCTRL_ALL_AUTO (0xAA222222)


/******************************************************************************
* MCU LDR Format
******************************************************************************/
/* Block header structure */
typedef struct
{
	MT_UINT32	ui32DevID;
	MT_UINT32	ui32L2ode;
	MT_UINT32	ui32SLData;
	MT_UINT16	ui16PLCtrl;
	MT_UINT16	ui16CRC;

} MUSA_MCU_LDR_BLOCK_HDR;

/* High level data stream block structure */
typedef struct
{
	MT_UINT16	ui16Cmd;
	MT_UINT16	ui16Length;
	MT_UINT32	ui32Next;
	MT_UINT32	aui32CmdData[4];

} MUSA_MCU_LDR_L1_DATA_BLK;

/* High level data stream block structure */
typedef struct
{
	MT_UINT16	ui16Tag;
	MT_UINT16	ui16Length;
	MT_UINT32	aui32BlockData[4];

} MUSA_MCU_LDR_L2_DATA_BLK;

/* High level data stream block header structure (only for windows fw) */
typedef struct
{
	uint16_t ui16Tag;
	uint16_t ui16Length;
} MUSA_MCU_LDR_L2_DATA_BLK_HDR;

/* Config command structure */
typedef struct
{
	MT_UINT32	ui32Type;
	MT_UINT32	aui32BlockData[4];

} MUSA_MCU_LDR_CFG_BLK;

/* Block type definitions */
#define MUSA_MCU_LDR_COMMENT_TYPE_MASK			(0x0010U)
#define MUSA_MCU_LDR_BLK_IS_COMMENT(X)			((X & MUSA_MCU_LDR_COMMENT_TYPE_MASK) != 0U)

/* Command definitions
 *  Value   Name            Description
 *  0       LoadMem         Load memory with binary data.
 *  1       LoadCore        Load a set of core registers.
 *  2       LoadMMReg       Load a set of memory mapped registers.
 *  3       StartThreads    Set each thread PC and SP, then enable threads.
 *  4       ZeroMem         Zeros a memory region.
 *  5       Config          Perform a configuration command.
 */
#define MUSA_MCU_LDR_CMD_MASK				(0x000FU)

#define MUSA_MCU_LDR_CMD_LOADMEM			(0x0000U)
#define MUSA_MCU_LDR_CMD_LOADCORE			(0x0001U)
#define MUSA_MCU_LDR_CMD_LOADMMREG			(0x0002U)
#define MUSA_MCU_LDR_CMD_START_THREADS		(0x0003U)
#define MUSA_MCU_LDR_CMD_ZEROMEM			(0x0004U)
#define MUSA_MCU_LDR_CMD_CONFIG			(0x0005U)

/* Config Command definitions
 *  Value   Name        Description
 *  0       Pause       Pause for x times 100 instructions
 *  1       Read        Read a value from register - No value return needed.
 *                      Utilises effects of issuing reads to certain registers
 *  2       Write       Write to mem location
 *  3       MemSet      Set mem to value
 *  4       MemCheck    check mem for specific value.
 */
#define MUSA_MCU_LDR_CFG_PAUSE			(0x0000)
#define MUSA_MCU_LDR_CFG_READ			(0x0001)
#define MUSA_MCU_LDR_CFG_WRITE			(0x0002)
#define MUSA_MCU_LDR_CFG_MEMSET			(0x0003)
#define MUSA_MCU_LDR_CFG_MEMCHECK		(0x0004)


/******************************************************************************
* MUSA FW segmented MMU definitions
******************************************************************************/
/* All threads can access the segment */
#define MUSAFW_SEGMMU_ALLTHRS	(MT_UINT32_C(0xf) << 8U)
/* Writable */
#define MUSAFW_SEGMMU_WRITEABLE	(0x1U << 1U)
/* All threads can access and writable */
#define MUSAFW_SEGMMU_ALLTHRS_WRITEABLE	(MUSAFW_SEGMMU_ALLTHRS | MUSAFW_SEGMMU_WRITEABLE)

/********************************* Linux FW ***********************************/
/* Direct map region 10 used for mapping GPU memory - max 8MB */
#define MUSAFW_SEGMMU_DMAP_GPU_ID			(10U)
#define MUSAFW_SEGMMU_DMAP_GPU_ADDR_START	(0x07000000U)
#define MUSAFW_SEGMMU_DMAP_GPU_MAX_SIZE		(0x00800000U)

/* Segment IDs */
#define MUSAFW_SEGMMU_DATA_ID			(1U)
#define MUSAFW_SEGMMU_BOOTLDR_ID			(2U)
#define MUSAFW_SEGMMU_TEXT_ID			(MUSAFW_SEGMMU_BOOTLDR_ID)

/******************************** Windows FW **********************************/
/* Direct map regions mapping (8-10) */
#define MUSAFW_WDDM_SEGMMU_DMAP_ID_START   (8)
#define MUSAFW_WDDM_SEGMMU_DMAP_ADDR_START (0x06000000U)
#define MUSAFW_WDDM_SEGMMU_DMAP_ADDR_MCU  (0x06000000U)
#define MUSAFW_WDDM_SEGMMU_DMAP_SIZE       (8 * 1024 * 1024) /* 8 MB */

/* Direct map region 11 used for mapping GPU memory */
#define MUSAFW_WDDM_SEGMMU_DMAP_GPU_ID         (11)
#define MUSAFW_WDDM_SEGMMU_DMAP_GPU_ADDR_START  \
	(MUSAFW_WDDM_SEGMMU_DMAP_ADDR_START + 3 * MUSAFW_WDDM_SEGMMU_DMAP_SIZE)

/* Segment IDs */
#define MUSAFW_WDDM_SEGMMU_TEXT_ID       (0)
#define MUSAFW_WDDM_SEGMMU_SHARED_ID     (1)
#define MUSAFW_WDDM_SEGMMU_BOOTLDR_ID    (2)
#define MUSAFW_WDDM_SEGMMU_DATA_ID       (3)

#define MUSAFW_WDDM_SEGMMU_MCU_DM_ID    (0x7)

/* To configure the Page Catalog and BIF-DM fed into the BIF for Garten accesses through this segment */
#define MUSAFW_WDDM_SEGMMU_OUTADDR_TOP(pc, bifdm)  \
	((((uint64_t)((pc) & 0xF)) << 44) | (((uint64_t)((bifdm) & 0xF)) << 40))

/*
 * L2 caching strategy in S7 and musa is emitted through the segment MMU.
 * All the segments configured through the macro MUSAFW_SEGMMU_OUTADDR_TOP are
 * CACHED in the L2.
 * The interface has been kept the same to simplify the code changes.
 * The bifdm argument is ignored (no longer relevant) in S7 and musa.
 */
#define MUSAFW_SEGMMU_OUTADDR_TOP_VIVT_L2(pers, slc_policy, mmu_ctx)      ((((MT_UINT64) ((pers)    & 0x3U))  << 52) | \
                                                                           (((MT_UINT64) ((mmu_ctx) & 0xFFU)) << 44) | \
                                                                           (((MT_UINT64) ((slc_policy) & 0x1U))  << 40))
#define MUSAFW_SEGMMU_OUTADDR_TOP_VIVT_L2_CACHED(mmu_ctx)      MUSAFW_SEGMMU_OUTADDR_TOP_VIVT_L2(0x3U, 0x0U, mmu_ctx)
#define MUSAFW_SEGMMU_OUTADDR_TOP_VIVT_L2_UNCACHED(mmu_ctx)    MUSAFW_SEGMMU_OUTADDR_TOP_VIVT_L2(0x0U, 0x1U, mmu_ctx)

/* PH1 */
#define MUSAFW_SEGMMU_OUTADDR_TOP_VIVT_L2_PH1(pers, slc_policy, mmu_ctx)      ((((MT_UINT64) ((pers)    & 0x1FU))  << 54) | \
                                                                               (((MT_UINT64) ((mmu_ctx) & 0x1FU)) << 49) | \
                                                                               (((MT_UINT64) ((slc_policy) & 0x1U))  << 48))
#define MUSAFW_SEGMMU_OUTADDR_TOP_VIVT_L2_CACHED_PH1(mmu_ctx)      MUSAFW_SEGMMU_OUTADDR_TOP_VIVT_L2_PH1(0x3U, 0x0U, mmu_ctx)
#define MUSAFW_SEGMMU_OUTADDR_TOP_VIVT_L2_UNCACHED_PH1(mmu_ctx)    MUSAFW_SEGMMU_OUTADDR_TOP_VIVT_L2_PH1(0x0U, 0x1U, mmu_ctx)


/* To configure the Page Catalog and BIF-DM fed into the BIF for Garten
 * accesses through this segment
 */
#define MUSAFW_SEGMMU_OUTADDR_TOP_L2(pc, bifdm)              (((MT_UINT64)((MT_UINT64)(pc)    & 0xFU) << 44U) | \
                                                              ((MT_UINT64)((MT_UINT64)(bifdm) & 0xFU) << 40U))

#define MUSAFW_SEGMMU_MCU_BIFDM_ID   (0x7U)
#if !defined(__KERNEL__) && defined(MUSA_FEATURE_MCU)
#if defined(MUSA_FEATURE_L2_VIVT)
#define MUSAFW_SEGMMU_OUTADDR_TOP_L2_CACHED        MUSAFW_SEGMMU_OUTADDR_TOP_VIVT_L2_CACHED
#define MUSAFW_SEGMMU_OUTADDR_TOP_L2_UNCACHED      MUSAFW_SEGMMU_OUTADDR_TOP_VIVT_L2_UNCACHED
#define MUSAFW_SEGMMU_OUTADDR_TOP_MCU              MUSAFW_SEGMMU_OUTADDR_TOP_L2_CACHED
#define MUSAFW_SEGMMU_OUTADDR_TOP_L2_CACHED_PH1    MUSAFW_SEGMMU_OUTADDR_TOP_VIVT_L2_CACHED_PH1
#define MUSAFW_SEGMMU_OUTADDR_TOP_L2_UNCACHED_PH1  MUSAFW_SEGMMU_OUTADDR_TOP_VIVT_L2_UNCACHED_PH1
#define MUSAFW_SEGMMU_OUTADDR_TOP_MCU_PH1          MUSAFW_SEGMMU_OUTADDR_TOP_L2_CACHED_PH1
#else
#define MUSAFW_SEGMMU_OUTADDR_TOP_L2_CACHED    MUSAFW_SEGMMU_OUTADDR_TOP_L2
#define MUSAFW_SEGMMU_OUTADDR_TOP_L2_UNCACHED  MUSAFW_SEGMMU_OUTADDR_TOP_L2
#define MUSAFW_SEGMMU_OUTADDR_TOP_MCU(pc)      MUSAFW_SEGMMU_OUTADDR_TOP_L2(pc, MUSAFW_SEGMMU_MCU_BIFDM_ID)
#endif
#endif

/* MCU segments have 4kB minimum size */
#define MUSAFW_SEGMMU_ALIGN			(0x1000U)

/* Segmented MMU registers (n = segment id) */
#define MCU_CR_MMCU_SEGMENTn_BASE(n)			(0x04850000U + ((n)*0x10U))
#define MCU_CR_MMCU_SEGMENTn_LIMIT(n)			(0x04850004U + ((n)*0x10U))
#define MCU_CR_MMCU_SEGMENTn_OUTA0(n)			(0x04850008U + ((n)*0x10U))
#define MCU_CR_MMCU_SEGMENTn_OUTA1(n)			(0x0485000CU + ((n)*0x10U))

/* The following defines must be recalculated if the Mcu MMU segments used
 * to access Host-FW data are changed
 * Current combinations are:
 * - L2 uncached, MCU cached,   FW base address 0x70000000
 * - L2 uncached, MCU uncached, FW base address 0xF0000000
 * - L2 cached,   MCU cached,   FW base address 0x10000000
 * - L2 cached,   MCU uncached, FW base address 0x90000000
 */
#define MUSAFW_SEGMMU_DATA_BASE_ADDRESS        (0x10000000U)
#define MUSAFW_SEGMMU_DATA_MCU_CACHED         (0x0U)
#define MUSAFW_SEGMMU_DATA_MCU_UNCACHED       (MCU_MEM_GLOBAL_RANGE_BIT) // 0x80000000
#define MUSAFW_SEGMMU_DATA_MCU_CACHE_MASK     (MCU_MEM_GLOBAL_RANGE_BIT)
/* For non-VIVT L2s the cacheability of the FW data in the L2 is selected in
 * the PTEs for the FW data, not in the Mcu Segment MMU, which means these
 * defines have no real effect in those cases
 */
#define MUSAFW_SEGMMU_DATA_VIVT_L2_CACHED     (0x0U)
#define MUSAFW_SEGMMU_DATA_VIVT_L2_UNCACHED   (0x60000000U)
#define MUSAFW_SEGMMU_DATA_VIVT_L2_CACHE_MASK (0x60000000U)


#if defined(SECURE_FW_CODE_OSID) && defined(MUSA_FEATURE_MCU)
#error "SECURE_FW_CODE_OSID is not supported on MCU cores"
#endif


/******************************************************************************
* MUSA FW Bootloader defaults
******************************************************************************/
#define MUSAFW_BOOTLDR_MCU_ADDR		(0x40000000U)
#define MUSAFW_BOOTLDR_DEVV_ADDR_0	(0xC0000000U)
#define MUSAFW_BOOTLDR_DEVV_ADDR_1	(0x000000E1)
#define MUSAFW_BOOTLDR_DEVV_ADDR		((((MT_UINT64) MUSAFW_BOOTLDR_DEVV_ADDR_1) << 32) | MUSAFW_BOOTLDR_DEVV_ADDR_0)
#define MUSAFW_BOOTLDR_LIMIT		(0x1FFFF000)
#define MUSAFW_MAX_BOOTLDR_OFFSET	(0x1000)

/* Bootloader configuration offset is in dwords (512 bytes) */
#define MUSAFW_BOOTLDR_CONF_OFFSET	(0x80)


/******************************************************************************
* MUSA MCU Stack
******************************************************************************/
#define MUSA_MCU_STACK_SIZE  (0x1000U)

/******************************************************************************
 MUSA MCU Core memory
******************************************************************************/
/* code and data both map to the same physical memory */
#define MUSA_MCU_COREMEM_CODE_ADDR   (0x80000000U)
#define MUSA_MCU_COREMEM_DATA_ADDR   (0x82000000U)
#define MUSA_MCU_COREMEM_OFFSET_MASK (0x01ffffffU)

#if defined(__KERNEL__)
#define MUSA_MCU_IS_COREMEM_CODE(A, B)  (((A) >= MUSA_MCU_COREMEM_CODE_ADDR) && ((A) < (MUSA_MCU_COREMEM_CODE_ADDR + (B))))
#define MUSA_MCU_IS_COREMEM_DATA(A, B)  (((A) >= MUSA_MCU_COREMEM_DATA_ADDR) && ((A) < (MUSA_MCU_COREMEM_DATA_ADDR + (B))))
#endif

/******************************************************************************
* 2nd thread
******************************************************************************/
#define MUSAFW_THR1_PC		(0x18930000)
#define MUSAFW_THR1_SP		(0x78890000)

/******************************************************************************
* MCU compatibility
******************************************************************************/

#define MCU_CR_CORE_ID			(0x04831000)
#define MCU_CR_CORE_ID_VER_SHIFT	(16U)
#define MCU_CR_CORE_ID_VER_CLRMSK	(0XFF00FFFFU)

#if !defined(__KERNEL__) && defined(MUSA_FEATURE_MCU)

	#if (MUSA_FEATURE_MCU == MTP218)
	#define MUSA_CR_MCU_CORE_ID_VALUE 0x19
	#elif (MUSA_FEATURE_MCU == MTP219)
	#define MUSA_CR_MCU_CORE_ID_VALUE 0x1E
	#elif (MUSA_FEATURE_MCU == LTP218)
	#define MUSA_CR_MCU_CORE_ID_VALUE 0x1C
	#elif (MUSA_FEATURE_MCU == LTP217)
	#define MUSA_CR_MCU_CORE_ID_VALUE 0x1F
	#else
	#error "Unknown MCU ID"
	#endif
#else

	#define MUSA_CR_MCU_MTP218_CORE_ID_VALUE 0x19
	#define MUSA_CR_MCU_MTP219_CORE_ID_VALUE 0x1E
	#define MUSA_CR_MCU_LTP218_CORE_ID_VALUE 0x1C
	#define MUSA_CR_MCU_LTP217_CORE_ID_VALUE 0x1F

#endif
#define MUSAFW_PROCESSOR_MCU        "MCU"


#endif /* MUSA_MCU_H */

/******************************************************************************
 End of file (musa_meta.h)
******************************************************************************/

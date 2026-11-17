/*************************************************************************/ /*!
@Title          Rogue hw definitions (kernel mode)
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

#ifndef MUSADEFS_KM_H
#define MUSADEFS_KM_H

#if defined(MUSA_BVNC_CORE_KM_HEADER) && defined(MUSA_BNC_CONFIG_KM_HEADER)
#include MUSA_BVNC_CORE_KM_HEADER
#include MUSA_BNC_CONFIG_KM_HEADER
#endif

#include "musa_heap_firmware.h"

/* The following Macros are picked up through BVNC headers for no hardware
 * operations to be compatible with old build infrastructure.
 */
#if defined(NO_HARDWARE)
/******************************************************************************
 * Check for valid B.X.N.C
 *****************************************************************************/
#if !defined(MUSA_BVNC_KM_B) || !defined(MUSA_BVNC_KM_V) || !defined(MUSA_BVNC_KM_N) || !defined(MUSA_BVNC_KM_C)
#error "Need to specify BVNC (MUSA_BVNC_KM_B, MUSA_BVNC_KM_V, MUSA_BVNC_KM_N and MUSA_BVNC_C)"
#endif

/* Check core/config compatibility */
#if (MUSA_BVNC_KM_B != MUSA_BNC_KM_B) || (MUSA_BVNC_KM_N != MUSA_BNC_KM_N) || (MUSA_BVNC_KM_C != MUSA_BNC_KM_C)
#error "BVNC headers are mismatching (KM core/config)"
#endif
#endif

/******************************************************************************
 * MUSA Version name
 *****************************************************************************/
#define MUSA_BVNC_KM_ST2(S)	#S
#define MUSA_BVNC_KM_ST(S)	MUSA_BVNC_KM_ST2(S)
#define MUSA_BVNC_KM			MUSA_BVNC_KM_ST(MUSA_BVNC_KM_B) "." MUSA_BVNC_KM_ST(MUSA_BVNC_KM_V) "." MUSA_BVNC_KM_ST(MUSA_BVNC_KM_N) "." MUSA_BVNC_KM_ST(MUSA_BVNC_KM_C)
#define MUSA_BVNC_KM_V_ST	MUSA_BVNC_KM_ST(MUSA_BVNC_KM_V)

/* Maximum string size is [bb.vvvp.nnnn.cccc\0], includes null char */
#define MUSA_BVNC_STR_SIZE_MAX (2+1+4+1+4+1+4+1)
#define MUSA_BVNC_STR_FMTSPEC  "%u.%u.%u.%u"
#define MUSA_BVNC_STRP_FMTSPEC "%u.%up.%u.%u"


/******************************************************************************
 * MUSA Defines
 *****************************************************************************/

#define BVNC_FIELD_MASK     ((1 << BVNC_FIELD_WIDTH) - 1)
#define C_POSITION          (0)
#define N_POSITION          ((C_POSITION) + (BVNC_FIELD_WIDTH))
#define V_POSITION          ((N_POSITION) + (BVNC_FIELD_WIDTH))
#define B_POSITION          ((V_POSITION) + (BVNC_FIELD_WIDTH))

#define B_POSTION_MASK      (((MT_UINT64)(BVNC_FIELD_MASK) << (B_POSITION)))
#define V_POSTION_MASK      (((MT_UINT64)(BVNC_FIELD_MASK) << (V_POSITION)))
#define N_POSTION_MASK      (((MT_UINT64)(BVNC_FIELD_MASK) << (N_POSITION)))
#define C_POSTION_MASK      (((MT_UINT64)(BVNC_FIELD_MASK) << (C_POSITION)))

#define GET_B(x)            (((x) & (B_POSTION_MASK)) >> (B_POSITION))
#define GET_V(x)            (((x) & (V_POSTION_MASK)) >> (V_POSITION))
#define GET_N(x)            (((x) & (N_POSTION_MASK)) >> (N_POSITION))
#define GET_C(x)            (((x) & (C_POSTION_MASK)) >> (C_POSITION))

#define BVNC_PACK(B,V,N,C)  ((((MT_UINT64)B)) << (B_POSITION) | \
                             (((MT_UINT64)V)) << (V_POSITION) | \
                             (((MT_UINT64)N)) << (N_POSITION) | \
                             (((MT_UINT64)C)) << (C_POSITION) \
                            )

#define MUSA_CR_CORE_ID_CONFIG_N_SHIFT    (8U)
#define MUSA_CR_CORE_ID_CONFIG_C_SHIFT    (0U)

#define MUSA_CR_CORE_ID_CONFIG_N_CLRMSK   (0XFFFF00FFU)
#define MUSA_CR_CORE_ID_CONFIG_C_CLRMSK   (0XFFFFFF00U)

/* The default number of OSID is 1, higher number implies VZ enabled firmware */
#if !defined(MUSAFW_NATIVE) && defined(MTGPU_VZ_NUM_OSID) && (MTGPU_VZ_NUM_OSID + 1U > 1U)
#define MUSAFW_NUM_OS MTGPU_VZ_NUM_OSID
#else
#define MUSAFW_NUM_OS 1U
#endif

#if defined(SUPPORT_SW_OSID_EXTENSION)
#define MUSAFW_MAX_NUM_OS                                  (16U)
#define MUSAFW_EXT_OSID_SIZE                               (8U)
#define MUSAFW_EXT_OSID_CCB_SIZE                           (128U)
#else
#define MUSAFW_MAX_NUM_OS                                  (8U)
#endif
#define MUSAFW_HOST_OS                                     (0U)
#define MUSAFW_GUEST_OSID_START                            (1U)

#define MUSAFW_THREAD_0                                    (0U)
#define MUSAFW_THREAD_1                                    (1U)

/* MCU cores (required for the MUSA_FEATURE_MCU) */
#define MTP218   (1)
#define MTP219   (2)
#define LTP218   (3)
#define LTP217   (4)

/* MCU Core memory feature depending on MCU variants */
#define MUSA_MCU_COREMEM_32K      (32*1024)
#define MUSA_MCU_COREMEM_48K      (48*1024)
#define MUSA_MCU_COREMEM_64K      (64*1024)
#define MUSA_MCU_COREMEM_96K      (96*1024)
#define MUSA_MCU_COREMEM_128K     (128*1024)
#define MUSA_MCU_COREMEM_256K     (256*1024)

#define MUSA_MCU_ENABLE_COREMEM

#if defined(SUPPORT_TRUSTED_DEVICE) && !defined(MUSA_FEATURE_MCU_DMA)
#undef SUPPORT_MCU_COREMEM
#undef MUSA_FEATURE_MCU_COREMEM_SIZE
#define MUSA_FEATURE_MCU_COREMEM_SIZE (0)
#define MUSA_MCU_COREMEM_SIZE         (0)
#elif defined(MUSA_FEATURE_MCU_COREMEM_SIZE)
#define MUSA_MCU_COREMEM_SIZE         (MUSA_FEATURE_MCU_COREMEM_SIZE*1024U)
#else
#define MUSA_MCU_COREMEM_SIZE         (0)
#endif

#if MUSA_MCU_COREMEM_SIZE != 0 && defined(MUSA_MCU_ENABLE_COREMEM)
/* MUSA_MCU_COREMEM is used only by the windows fw */
#define MUSA_MCU_COREMEM
#define MUSA_MCU_COREMEM_CODE
#define MUSA_MCU_COREMEM_DATA
#endif

#define GET_MUSA_CACHE_LINE_SIZE(x)    ((((MT_INT32)x) > 0) ? ((x)/8) : (0))


#if defined(SUPPORT_AGP)
#if defined(SUPPORT_AGP4)
#define MAX_HW_GFXCONTEXTS	5U
#else
#define MAX_HW_GFXCONTEXTS	3U
#endif
#else
#define MAX_HW_GFXCONTEXTS	2U
#endif

#define MUSA_CR_CLK_CTRL0_ALL_ON   (MT_UINT64_C(0x5555555555555555)&MUSA_CR_CLK_CTRL0_MASKFULL)
#define MUSA_CR_CLK_CTRL0_ALL_AUTO (MT_UINT64_C(0xaaaaaaaaaaaaaaaa)&MUSA_CR_CLK_CTRL0_MASKFULL)
#define MUSA_CR_CLK_CTRL1_ALL_ON   (MT_UINT64_C(0x5555555555555555)&MUSA_CR_CLK_CTRL1_MASKFULL)
#define MUSA_CR_CLK_CTRL1_ALL_AUTO (MT_UINT64_C(0xaaaaaaaaaaaaaaaa)&MUSA_CR_CLK_CTRL1_MASKFULL)
#define MUSA_CR_CLK_CTRL2_ALL_ON   (MT_UINT64_C(0x5555555555555555)&MUSA_CR_CLK_CTRL2_MASKFULL)
#define MUSA_CR_CLK_CTRL2_ALL_AUTO (MT_UINT64_C(0xaaaaaaaaaaaaaaaa)&MUSA_CR_CLK_CTRL2_MASKFULL)

#define MUSA_CR_MP_AU0_SOFT_RESET_MPX_EN (MUSA_CR_MP_AU_SOFT_RESET_MPX0_MERCER0_EN | \
										  MUSA_CR_MP_AU_SOFT_RESET_MPX1_MERCER0_EN | \
										  MUSA_CR_MP_AU_SOFT_RESET_MPX2_MERCER0_EN | \
										  MUSA_CR_MP_AU_SOFT_RESET_MPX3_MERCER0_EN | \
										  MUSA_CR_MP_AU_SOFT_RESET_MPX4_MERCER0_EN | \
										  MUSA_CR_MP_AU_SOFT_RESET_MPX5_MERCER0_EN | \
										  MUSA_CR_MP_AU_SOFT_RESET_MPX6_MERCER0_EN | \
										  MUSA_CR_MP_AU_SOFT_RESET_MPX7_MERCER0_EN | \
										  MUSA_CR_MP_AU_SOFT_RESET_MPX8_MERCER0_EN | \
										  MUSA_CR_MP_AU_SOFT_RESET_MPX9_MERCER0_EN | \
										  MUSA_CR_MP_AU_SOFT_RESET_MPX10_MERCER0_EN | \
										  MUSA_CR_MP_AU_SOFT_RESET_MPX11_MERCER0_EN | \
										  MUSA_CR_MP_AU_SOFT_RESET_MPX12_MERCER0_EN | \
										  MUSA_CR_MP_AU_SOFT_RESET_MPX13_MERCER0_EN | \
										  MUSA_CR_MP_AU_SOFT_RESET_MPX14_MERCER0_EN | \
										  MUSA_CR_MP_AU_SOFT_RESET_MPX15_MERCER0_EN | \
										  MUSA_CR_MP_AU_SOFT_RESET_MPX16_MERCER0_EN | \
										  MUSA_CR_MP_AU_SOFT_RESET_MPX17_MERCER0_EN | \
										  MUSA_CR_MP_AU_SOFT_RESET_MPX18_MERCER0_EN | \
										  MUSA_CR_MP_AU_SOFT_RESET_MPX19_MERCER0_EN | \
										  MUSA_CR_MP_AU_SOFT_RESET_MPX20_MERCER0_EN)

#define MUSA_CR_MP_AU1_SOFT_RESET_MPX_EN (MUSA_CR_MP_AU_SOFT_RESET_MPX0_MERCER1_EN | \
										  MUSA_CR_MP_AU_SOFT_RESET_MPX1_MERCER1_EN | \
										  MUSA_CR_MP_AU_SOFT_RESET_MPX2_MERCER1_EN | \
										  MUSA_CR_MP_AU_SOFT_RESET_MPX3_MERCER1_EN | \
										  MUSA_CR_MP_AU_SOFT_RESET_MPX4_MERCER1_EN | \
										  MUSA_CR_MP_AU_SOFT_RESET_MPX5_MERCER1_EN | \
										  MUSA_CR_MP_AU_SOFT_RESET_MPX6_MERCER1_EN | \
										  MUSA_CR_MP_AU_SOFT_RESET_MPX7_MERCER1_EN | \
										  MUSA_CR_MP_AU_SOFT_RESET_MPX8_MERCER1_EN | \
										  MUSA_CR_MP_AU_SOFT_RESET_MPX9_MERCER1_EN | \
										  MUSA_CR_MP_AU_SOFT_RESET_MPX10_MERCER1_EN | \
										  MUSA_CR_MP_AU_SOFT_RESET_MPX11_MERCER1_EN | \
										  MUSA_CR_MP_AU_SOFT_RESET_MPX12_MERCER1_EN | \
										  MUSA_CR_MP_AU_SOFT_RESET_MPX13_MERCER1_EN | \
										  MUSA_CR_MP_AU_SOFT_RESET_MPX14_MERCER1_EN | \
										  MUSA_CR_MP_AU_SOFT_RESET_MPX15_MERCER1_EN | \
										  MUSA_CR_MP_AU_SOFT_RESET_MPX16_MERCER1_EN | \
										  MUSA_CR_MP_AU_SOFT_RESET_MPX17_MERCER1_EN | \
										  MUSA_CR_MP_AU_SOFT_RESET_MPX18_MERCER1_EN | \
										  MUSA_CR_MP_AU_SOFT_RESET_MPX19_MERCER1_EN | \
										  MUSA_CR_MP_AU_SOFT_RESET_MPX20_MERCER1_EN)

#define MUSA_CR_MP_AU2_SOFT_RESET_MPX_EN (MUSA_CR_MP_AU_SOFT_RESET_MPX0_MERCER2_EN | \
										  MUSA_CR_MP_AU_SOFT_RESET_MPX1_MERCER2_EN | \
										  MUSA_CR_MP_AU_SOFT_RESET_MPX2_MERCER2_EN | \
										  MUSA_CR_MP_AU_SOFT_RESET_MPX3_MERCER2_EN | \
										  MUSA_CR_MP_AU_SOFT_RESET_MPX4_MERCER2_EN | \
										  MUSA_CR_MP_AU_SOFT_RESET_MPX5_MERCER2_EN | \
										  MUSA_CR_MP_AU_SOFT_RESET_MPX6_MERCER2_EN | \
										  MUSA_CR_MP_AU_SOFT_RESET_MPX7_MERCER2_EN | \
										  MUSA_CR_MP_AU_SOFT_RESET_MPX8_MERCER2_EN | \
										  MUSA_CR_MP_AU_SOFT_RESET_MPX9_MERCER2_EN | \
										  MUSA_CR_MP_AU_SOFT_RESET_MPX10_MERCER2_EN | \
										  MUSA_CR_MP_AU_SOFT_RESET_MPX11_MERCER2_EN | \
										  MUSA_CR_MP_AU_SOFT_RESET_MPX12_MERCER2_EN | \
										  MUSA_CR_MP_AU_SOFT_RESET_MPX13_MERCER2_EN | \
										  MUSA_CR_MP_AU_SOFT_RESET_MPX14_MERCER2_EN | \
										  MUSA_CR_MP_AU_SOFT_RESET_MPX15_MERCER2_EN | \
										  MUSA_CR_MP_AU_SOFT_RESET_MPX16_MERCER2_EN | \
										  MUSA_CR_MP_AU_SOFT_RESET_MPX17_MERCER2_EN | \
										  MUSA_CR_MP_AU_SOFT_RESET_MPX18_MERCER2_EN | \
										  MUSA_CR_MP_AU_SOFT_RESET_MPX19_MERCER2_EN | \
										  MUSA_CR_MP_AU_SOFT_RESET_MPX20_MERCER2_EN)


/* SOFT_RESET steps as defined in the TRM */
#define MUSA_SOFT_RESET_DMIC             (MUSA_CR_SOFT_RESET_PM_EN | \
                                          MUSA_CR_SOFT_RESET_ISP_EN)
#define MUSA_SOFT_RESET_DMIC_ALL         (MUSA_SOFT_RESET_DMIC | \
                                          MUSA_CR_SOFT_RESET_BIF_GFU_EN | \
                                          MUSA_CR_SOFT_RESET_BIF_DMIC_EN | \
										  MUSA_CR_SOFT_RESET_L2_EN | \
                                          MUSA_CR_SOFT_RESET_JSP_EN)
#define MUSA_SOFT_RESET_EXTRA             (MUSA_CR_SOFT_RESET_PIXEL_EN | \
                                          MUSA_CR_SOFT_RESET_VERTEX_EN |  \
                                          MUSA_CR_SOFT_RESET_GEO_VERTEX_EN | \
                                          MUSA_CR_SOFT_RESET_GEO_SHARED_EN | \
                                          MUSA_CR_SOFT_RESET_COMP_EN | \
                                          MUSA_CR_SOFT_RESET_TRANSFER_EN)
#define MUSA_SOFT_RESET_FROM_WITHIN_CORE  (MUSA_CR_SOFT_RESET_MASKFULL ^ \
                                          (MUSA_CR_SOFT_RESET_JSP_EN | \
                                           MUSA_CR_SOFT_RESET_BIF_DMIC_EN | \
                                           MUSA_CR_SOFT_RESET_L2_EN))


#define MUSA_BIF_PM_PHYSICAL_PAGE_ALIGNSHIFT		(12U)
#define MUSA_BIF_PM_PHYSICAL_PAGE_SIZE			(1U << MUSA_BIF_PM_PHYSICAL_PAGE_ALIGNSHIFT)

#define MUSA_BIF_PM_VIRTUAL_PAGE_ALIGNSHIFT		(14U)
#define MUSA_BIF_PM_VIRTUAL_PAGE_SIZE			(1U << MUSA_BIF_PM_VIRTUAL_PAGE_ALIGNSHIFT)

#define MUSA_BIF_PM_FREELIST_BASE_ADDR_ALIGNSIZE	(32U)

/* To get the number of required Bernado/Phantom(s), divide
 * the number of clusters by 4 and round up
 */
#define MUSA_REQ_NUM_PHANTOMS(CLUSTERS) ((CLUSTERS + 3U) / 4U)
#define MUSA_REQ_NUM_BERNADOS(CLUSTERS) ((CLUSTERS + 3U) / 4U)
#define MUSA_REQ_NUM_BLACKPEARLS(CLUSTERS) ((CLUSTERS + 3U) / 4U)

#if !defined(__KERNEL__)
# define MUSA_NUM_PHANTOMS (MUSA_REQ_NUM_PHANTOMS(MUSA_FEATURE_NUM_CLUSTERS))
#endif

/* for nohw multicore, true max number of cores returned to client */
#define MUSA_MULTICORE_MAX_NOHW_CORES                (8U)

/*
 * MCU second thread feature depending on MCU variants and
 * available CoreMem
 */
#if defined(MUSA_FEATURE_MCU) && (MUSA_FEATURE_MCU == MTP218 || MUSA_FEATURE_MCU == MTP219) && (MUSA_FEATURE_MCU_COREMEM_SIZE >= 96)
#define MUSAFW_MCU_SUPPORT_2ND_THREAD
#endif


/*
 * FW MMU contexts
 */
#if defined(SUPPORT_TRUSTED_DEVICE)
#define MMU_CONTEXT_MAPPING_FWPRIV (0x0U) /* FW code/private data */
#define MMU_CONTEXT_MAPPING_FWIF   (0x7U) /* Host/FW data */
#else
#define MMU_CONTEXT_MAPPING_FWPRIV (0x0U)
#define MMU_CONTEXT_MAPPING_FWIF   (0x0U)
#endif


/*
 * FWCORE wrapper register defines
 */
#define FWCORE_ADDR_REMAP_CONFIG0_MMU_CONTEXT_SHIFT   MUSA_CR_FWCORE_ADDR_REMAP_CONFIG0_MMU_CONTEXT_SHIFT
#define FWCORE_ADDR_REMAP_CONFIG0_MMU_CONTEXT_CLRMSK  MUSA_CR_FWCORE_ADDR_REMAP_CONFIG0_MMU_CONTEXT_CLRMSK
#define FWCORE_ADDR_REMAP_CONFIG0_SIZE_ALIGNSHIFT     (12U)


/*
 * FBC clear color register defaults based on HW defaults
 * non-YUV clear colour 0: 0x00000000 (encoded as ch3,2,1,0)
 * non-YUV clear colour 1: 0x01000000 (encoded as ch3,2,1,0)
 * YUV clear colour 0: 0x000 000 (encoded as UV Y)
 * YUV clear colour 1: 0x000 3FF (encoded as UV Y)
 */
#define MUSA_FBC_CC_DEFAULT (0x0100000000000000)
#define MUSA_FBC_CC_YUV_DEFAULT (0x000003FF00000000)

/*
 * Virtualisation definitions
 */

#define MUSA_VIRTUALISATION_REG_SIZE_PER_OS (MUSA_CR_MTS_SCHEDULE1 - MUSA_CR_MTS_SCHEDULE)


/* GPU CR timer tick in GPU cycles */
#define MUSA_CRTIME_TICK_IN_CYCLES (256U)

#if defined(FIX_HW_BRN_71840)
#define MUSA_RENDERSIZE_MAXX						(16384U)
#define MUSA_RENDERSIZE_MAXY						(16384U)
#else
#define MUSA_RENDERSIZE_MAXX						(MUSA_FEATURE_RENDER_TARGET_XY_MAX)
#define MUSA_RENDERSIZE_MAXY						(MUSA_FEATURE_RENDER_TARGET_XY_MAX)
#endif

/*
 * Register used by the FW to track the current boot stage (not used in MIPS)
 */
#define MUSA_FW_BOOT_STAGE_REGISTER     (MUSA_CR_SCRATCH14)

/*
 * Define used to determine whether or not L2 range-based flush/invalidate
 * interface is supported.
 */
#define MUSA_SRV_L2_RANGEBASED_CFI_SUPPORTED 1

/*
 * Macro used to indicate which version of HWPerf is active
 */
#define MUSA_FEATURE_HWPERF_MUSA

/*
 * Maximum number of cores supported by TRP
 */
#define MUSA_TRP_MAX_NUM_CORES                           (8U)

/*
 * Maximum number of cores supported by WGP
 */
#define MUSA_WGP_MAX_NUM_CORES                           (8U)


#if defined(FIX_HW_BRN_71422)
/*
 * The BRN71422 software workaround requires a target physical address on
 * the hardware platform with a low latency response time and which will
 * not suffer from delays of DRAM hardware operations such as refresh and
 * recalibration. Only with that address defined will the workaround be used.
 */
#if !defined(PDUMP)
//#define MUSA_BRN71422_TARGET_HARDWARE_PHYSICAL_ADDR  (MT_UINT64_C(0x0000000000))
#endif
#define MUSA_BRN71422_WORKAROUND_READ_SIZE           (32U)
#endif

#endif /* MUSADEFS_KM_H */

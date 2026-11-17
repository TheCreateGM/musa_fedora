/*************************************************************************/ /*!
@File
@Title          MUSA Common Types and Defines Header
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Common types and definitions for MUSA software
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
#ifndef MUSA_COMMON_H
#define MUSA_COMMON_H

#if defined(__cplusplus)
extern "C" {
#endif

#include "img_defs.h"
#include "musa_common_asserts.h"


/* Virtualisation validation builds are meant to test the VZ-related hardware without a fully virtualised platform.
 * As such a driver can support either the vz-validation code or real virtualisation.
 * Note: MTGPU_VZ_NUM_OSID is the external build option, while MUSA_NUM_OS_SUPPORTED is the internal symbol used in the DDK */
#if defined(SUPPORT_GPUVIRT_VALIDATION) && (MUSA_NUM_OS_SUPPORTED > 1)
#error "Invalid build configuration: Virtualisation support (MTGPU_VZ_NUM_OSID > 1) and virtualisation validation code (SUPPORT_GPUVIRT_VALIDATION) are mutually exclusive."
#endif

/* The MUSAFWIF_DM defines assume only one of MUSA_FEATURE_TLA or
 * MUSA_FEATURE_FASTRENDER_DM is present. Ensure this with a compile-time check.
 */
#if defined(MUSA_FEATURE_TLA) && defined(MUSA_FEATURE_FASTRENDER_DM)
#error "Both MUSA_FEATURE_TLA and MUSA_FEATURE_FASTRENDER_DM defined. Fix code to handle this!"
#endif

/*! The master definition for data masters known to the firmware of MUSA.
 * When a new DM is added to this list, relevant entry should be added to
 * MUSA_HWPERF_DM enum list.
 * The DM in a V1 HWPerf packet uses this definition. */

typedef MT_UINT32 MUSAFWIF_DM;

#define	MUSAFWIF_GPU_OVERALL_UTIL        MT_UINT32_C(0)

#define	MUSAFWIF_DM_GP			MT_UINT32_C(0)
/* Either TRANSFER or 2D DM is present. The above build time error is present to verify this */
#define	MUSAFWIF_DM_2D			MT_UINT32_C(1) /* when MUSA_FEATURE_TLA defined */
#define	MUSAFWIF_DM_TRANSFER			MT_UINT32_C(1) /* when MUSA_FEATURE_FASTRENDER_DM defined */

#define	MUSAFWIF_DM_GEOM			MT_UINT32_C(2)
#define	MUSAFWIF_DM_3D			MT_UINT32_C(3)
#define	MUSAFWIF_DM_CMP			MT_UINT32_C(4)
#define MUSAFWIF_DM_CE    		MT_UINT32_C(5)
#define MUSAFWIF_DM_CMP2			MT_UINT32_C(6)
#define MUSAFWIF_DM_CMP3			MT_UINT32_C(7)
#define MUSAFWIF_DM_CMP4			MT_UINT32_C(8)
#define MUSAFWIF_DM_CE2			MT_UINT32_C(9)
#define MUSAFWIF_DM_CE3			MT_UINT32_C(10)
#define MUSAFWIF_DM_CE4			MT_UINT32_C(11)
#define MUSAFWIF_DM_RAY			MT_UINT32_C(12)
#define MUSAFWIF_DM_GEOM2		MT_UINT32_C(13)
#define MUSAFWIF_DM_GEOM3		MT_UINT32_C(14)
#define MUSAFWIF_DM_GEOM4		MT_UINT32_C(15)

#define MUSAFWIF_DM_LAST	 		MUSAFWIF_DM_GEOM4

typedef MT_UINT32 MUSA_KICK_TYPE_DM;
#define MUSA_KICK_TYPE_DM_GP		MT_UINT32_C(0x001)
#define MUSA_KICK_TYPE_DM_TRANSFER_2D	MT_UINT32_C(0x002)
#define MUSA_KICK_TYPE_DM_TA		MT_UINT32_C(0x004)
#define MUSA_KICK_TYPE_DM_3D		MT_UINT32_C(0x008)
#define MUSA_KICK_TYPE_DM_CMP	MT_UINT32_C(0x010)
#define MUSA_KICK_TYPE_DM_RTU	MT_UINT32_C(0x020)
#define MUSA_KICK_TYPE_DM_SHG	MT_UINT32_C(0x040)
#define MUSA_KICK_TYPE_DM_TQ2D	MT_UINT32_C(0x080)
#define MUSA_KICK_TYPE_DM_TQ3D	MT_UINT32_C(0x100)
#define MUSA_KICK_TYPE_DM_RAY	MT_UINT32_C(0x200)
#define MUSA_KICK_TYPE_DM_LAST	MT_UINT32_C(0x400)

/* Maximum number of DM in use: GP, 2D/TRANSFER, GEOM, 3D, CMP, RDM, GEOM2, GEOM3, GEOM4 */
#define MUSAFWIF_DM_MAX			(MUSAFWIF_DM_LAST + 1U)

/*
 * In virtualization scenario, MUSAFWIF_WDDM_DM_MAX will be
 * redefined in musa_wddm_fwif.h
 */
#define MUSAFWIF_WDDM_DM_MAX          (MUSAFWIF_DM_MAX)

/*
 * Data Master Tags to be appended to resources created on behalf of each MUSA
 * Context.
 */
#define MUSA_RI_DM_TAG_KS   'K'
#define MUSA_RI_DM_TAG_CMP  'C'
#define MUSA_RI_DM_TAG_RC   'R' /* To be removed once TA/3D Timelines are split */
#define MUSA_RI_DM_TAG_TA   'V'
#define MUSA_RI_DM_TAG_GEOM 'V'
#define MUSA_RI_DM_TAG_3D   'P'
#define MUSA_RI_DM_TAG_TRANSFER  'T'
#define MUSA_RI_DM_TAG_TQ2D '2'
#define MUSA_RI_DM_TAG_TQ3D 'Q'
#define MUSA_RI_DM_TAG_RAY  'r'

/*
 * Client API Tags to be appended to resources created on behalf of each
 * Client API.
 */
#define MUSA_RI_CLIENT_API_GLES1    '1'
#define MUSA_RI_CLIENT_API_GLES3    '3'
#define MUSA_RI_CLIENT_API_VULKAN   'V'
#define MUSA_RI_CLIENT_API_EGL      'E'
#define MUSA_RI_CLIENT_API_OPENCL   'C'
#define MUSA_RI_CLIENT_API_OPENGL   'G'
#define MUSA_RI_CLIENT_API_SERVICES 'S'
#define MUSA_RI_CLIENT_API_WSEGL    'W'
#define MUSA_RI_CLIENT_API_ANDROID  'A'
#define MUSA_RI_CLIENT_API_LWS      'L'

/*
 * Format a RI annotation for a given MUSA Data Master context
 */
#define MUSA_RI_FORMAT_DM_ANNOTATION(annotation, dmTag, clientAPI) do         \
	{                                                                        \
		(annotation)[0] = (dmTag);                                           \
		(annotation)[1] = (clientAPI);                                       \
		(annotation)[2] = '\0';                                              \
	} while (false)

/*!
 ******************************************************************************
 * MUSAFW Compiler alignment definitions
 *****************************************************************************/
#if defined(__GNUC__) || defined(HAS_GNUC_ATTRIBUTES) || defined(INTEGRITY_OS)
#define MUSAFW_ALIGN			__attribute__ ((aligned (8)))
#define	MUSAFW_ALIGN_DCACHEL		__attribute__((aligned (64)))
#elif defined(_MSC_VER)
#define MUSAFW_ALIGN			__declspec(align(8))
#define	MUSAFW_ALIGN_DCACHEL		__declspec(align(64))
#pragma warning (disable : 4324)
#else
#error "Align MACROS need to be defined for this compiler"
#endif

/*!
 ******************************************************************************
 * Force 8-byte alignment for structures allocated uncached.
 *****************************************************************************/
#define UNCACHED_ALIGN      MUSAFW_ALIGN


/*!
 ******************************************************************************
 * GPU Utilisation states
 *****************************************************************************/
#define MUSAFWIF_GPU_UTIL_STATE_IDLE      (0U)
#define MUSAFWIF_GPU_UTIL_STATE_ACTIVE    (1U)
#define MUSAFWIF_GPU_UTIL_STATE_BLOCKED   (2U)
#define MUSAFWIF_GPU_UTIL_STATE_NUM       (3U)
#define MUSAFWIF_GPU_UTIL_STATE_MASK      MT_UINT64_C(0x0000000000000003)


/*
 * Maximum amount of register writes that can be done by the register
 * programmer (FW or MCU DMA). This is not a HW limitation, it is only
 * a protection against malformed inputs to the register programmer.
 */
#define MUSA_MAX_NUM_REGISTER_PROGRAMMER_WRITES  (128U)

/* FW common context priority. */
/*!
 * @AddToGroup WorkloadContexts
 * @{
 */
#define MUSA_CTX_PRIORITY_REALTIME  (INT32_MAX)
#define MUSA_CTX_PRIORITY_HIGH      (2U) /*!< HIGH priority */
#define MUSA_CTX_PRIORITY_MEDIUM    (1U) /*!< MEDIUM priority */
#define MUSA_CTX_PRIORITY_LOW       (0) /*!< LOW priority */
/*!
 * @} End of AddToGroup WorkloadContexts
 */


/*
 *   Use of the 32-bit context property flags mask
 *   ( X = taken/in use, - = available/unused )
 *
 *                                   0
 *                                   |
 *    -------------------------------x
 */
/*
 * Context creation flags
 * (specify a context's properties at creation time)
 */
#define MUSA_CONTEXT_FLAG_DISABLESLR					(1UL << 0) /*!< Disable SLR */

/* Bitmask of context flags allowed to be modified after context create. */
#define MUSA_CONTEXT_FLAGS_WRITEABLE_MASK            (MUSA_CONTEXT_FLAG_DISABLESLR)

/* List of attributes that may be set for a context */
typedef enum _MUSA_CONTEXT_PROPERTY_
{
	MUSA_CONTEXT_PROPERTY_FLAGS  = 0, /*!< Context flags */
} MUSA_CONTEXT_PROPERTY;

#if defined(__cplusplus)
}
#endif

#endif /* MUSA_COMMON_H */

/******************************************************************************
 End of file
******************************************************************************/

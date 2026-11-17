/*************************************************************************/ /*!
@File
@Title          MUSA Bridge Functionality
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Header for the Rogue Bridge code
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

#ifndef MUSA_BRIDGE_H
#define MUSA_BRIDGE_H


#if defined(__cplusplus)
extern "C" {
#endif

#include "musa_fwif_km.h"

#define MUSAFWINITPARAMS_VERSION   1
#define MUSAFWINITPARAMS_EXTENSION 128

#include "common_musagfx_bridge.h"
#include "common_musacmp_bridge.h"
#if defined(SUPPORT_FASTRENDER_DM)
#include "common_musaxfer_bridge.h"
#endif
#if defined(SUPPORT_MUSATQ_BRIDGE)
#include "common_musatq_bridge.h"
#endif
#if defined(SUPPORT_MP_BREAKPOINT)
#include "common_musabreakpoint_bridge.h"
#endif
#include "common_musafwdbg_bridge.h"
#include "common_musahwperf_bridge.h"
#if !defined(EXCLUDE_MUSAREGCONFIG_BRIDGE)
#include "common_musaregconfig_bridge.h"
#endif
#include "common_musakicksync_bridge.h"
#include "common_musatimerquery_bridge.h"
#if defined(SUPPORT_MUSARAY_BRIDGE)
#include "common_musaray_bridge.h"
#endif
#include "common_musace_bridge.h"
/*
 * Bridge Cmd Ids
 */

/* *REMEMBER* to update MTGPU_BRIDGE_MUSA_LAST if you add/remove a bridge
 * group!
 * Also you need to ensure all MTGPU_BRIDGE_MUSA_xxx_DISPATCH_FIRST offsets
 * follow on from the previous bridge group's commands!
 *
 * If a bridge group is optional, ensure you *ALWAYS* define its index
 * (e.g. MTGPU_BRIDGE_MUSACMP is always 151, even is the feature is not
 * defined). If an optional bridge group is not defined you must still
 * define MTGPU_BRIDGE_MUSA_xxx_DISPATCH_FIRST for it with an assigned
 * value of 0.
 */

/* The MUSA bridge groups start at 128 (MTGPU_BRIDGE_MUSA_FIRST) rather than
 * follow-on from the other non-device bridge groups (meaning that they then
 * won't be displaced if other non-device bridge groups are added).
 */

#define MTGPU_BRIDGE_MUSA_FIRST                  128UL

/* 128: MUSA TQ interface functions */
#define MTGPU_BRIDGE_MUSATQ                      128UL
/* The MUSATQ bridge is conditional since the definitions in this header file
 * support both the rogue and musa servers, but the MUSATQ bridge is not
 * required at all on the Volcanic architecture.
 */
#if defined(SUPPORT_MUSATQ_BRIDGE)
#define MTGPU_BRIDGE_MUSATQ_DISPATCH_FIRST       (MTGPU_BRIDGE_DISPATCH_LAST + 1)
#define MTGPU_BRIDGE_MUSATQ_DISPATCH_LAST        (MTGPU_BRIDGE_MUSATQ_DISPATCH_FIRST + MTGPU_BRIDGE_MUSATQ_CMD_LAST)
#else
#define MTGPU_BRIDGE_MUSATQ_DISPATCH_FIRST       0
#define MTGPU_BRIDGE_MUSATQ_DISPATCH_LAST        (MTGPU_BRIDGE_DISPATCH_LAST)
#endif

/* 129: MUSA Compute interface functions */
#define MTGPU_BRIDGE_MUSACMP                     129UL
#define MTGPU_BRIDGE_MUSACMP_DISPATCH_FIRST      (MTGPU_BRIDGE_MUSATQ_DISPATCH_LAST + 1)
#define MTGPU_BRIDGE_MUSACMP_DISPATCH_LAST       (MTGPU_BRIDGE_MUSACMP_DISPATCH_FIRST + MTGPU_BRIDGE_MUSACMP_CMD_LAST)

/* 130: MUSA TA/3D interface functions */
#define MTGPU_BRIDGE_MUSAGFX                    130UL
#define MTGPU_BRIDGE_MUSAGFX_DISPATCH_FIRST     (MTGPU_BRIDGE_MUSACMP_DISPATCH_LAST + 1)
#define MTGPU_BRIDGE_MUSAGFX_DISPATCH_LAST      (MTGPU_BRIDGE_MUSAGFX_DISPATCH_FIRST + MTGPU_BRIDGE_MUSAGFX_CMD_LAST)

/* 131: MUSA Breakpoint interface functions */
#define MTGPU_BRIDGE_MUSABREAKPOINT                 131UL
#if defined(SUPPORT_MP_BREAKPOINT)
#define MTGPU_BRIDGE_MUSABREAKPOINT_DISPATCH_FIRST  (MTGPU_BRIDGE_MUSAGFX_DISPATCH_LAST + 1)
#define MTGPU_BRIDGE_MUSABREAKPOINT_DISPATCH_LAST   (MTGPU_BRIDGE_MUSABREAKPOINT_DISPATCH_FIRST + MTGPU_BRIDGE_MUSABREAKPOINT_CMD_LAST)
#else
#define MTGPU_BRIDGE_MUSABREAKPOINT_DISPATCH_FIRST  0
#define MTGPU_BRIDGE_MUSABREAKPOINT_DISPATCH_LAST   (MTGPU_BRIDGE_MUSAGFX_DISPATCH_LAST)
#endif

/* 132: MUSA Debug/Misc interface functions */
#define MTGPU_BRIDGE_MUSAFWDBG                   132UL
#define MTGPU_BRIDGE_MUSAFWDBG_DISPATCH_FIRST    (MTGPU_BRIDGE_MUSABREAKPOINT_DISPATCH_LAST + 1)
#define MTGPU_BRIDGE_MUSAFWDBG_DISPATCH_LAST     (MTGPU_BRIDGE_MUSAFWDBG_DISPATCH_FIRST + MTGPU_BRIDGE_MUSAFWDBG_CMD_LAST)

/* 133: MUSA PDump interface functions */
#define MTGPU_BRIDGE_MUSAPDUMP                   133UL
#define MTGPU_BRIDGE_MUSAPDUMP_DISPATCH_FIRST    0
#define MTGPU_BRIDGE_MUSAPDUMP_DISPATCH_LAST     (MTGPU_BRIDGE_MUSAFWDBG_DISPATCH_LAST)

/* 134: MUSA HWPerf interface functions */
#define MTGPU_BRIDGE_MUSAHWPERF                  134UL
#define MTGPU_BRIDGE_MUSAHWPERF_DISPATCH_FIRST   (MTGPU_BRIDGE_MUSAPDUMP_DISPATCH_LAST + 1)
#define MTGPU_BRIDGE_MUSAHWPERF_DISPATCH_LAST    (MTGPU_BRIDGE_MUSAHWPERF_DISPATCH_FIRST + MTGPU_BRIDGE_MUSAHWPERF_CMD_LAST)

/* 135: MUSA Register Configuration interface functions */
#define MTGPU_BRIDGE_MUSAREGCONFIG                  135UL
#if !defined(EXCLUDE_MUSAREGCONFIG_BRIDGE)
#define MTGPU_BRIDGE_MUSAREGCONFIG_DISPATCH_FIRST   (MTGPU_BRIDGE_MUSAHWPERF_DISPATCH_LAST + 1)
#define MTGPU_BRIDGE_MUSAREGCONFIG_DISPATCH_LAST    (MTGPU_BRIDGE_MUSAREGCONFIG_DISPATCH_FIRST + MTGPU_BRIDGE_MUSAREGCONFIG_CMD_LAST)
#else
#define MTGPU_BRIDGE_MUSAREGCONFIG_DISPATCH_FIRST   0
#define MTGPU_BRIDGE_MUSAREGCONFIG_DISPATCH_LAST    (MTGPU_BRIDGE_MUSAHWPERF_DISPATCH_LAST)
#endif

/* 136: MUSA kicksync interface */
#define MTGPU_BRIDGE_MUSAKICKSYNC                136UL
#define MTGPU_BRIDGE_MUSAKICKSYNC_DISPATCH_FIRST (MTGPU_BRIDGE_MUSAREGCONFIG_DISPATCH_LAST + 1)
#define MTGPU_BRIDGE_MUSAKICKSYNC_DISPATCH_LAST  (MTGPU_BRIDGE_MUSAKICKSYNC_DISPATCH_FIRST + MTGPU_BRIDGE_MUSAKICKSYNC_CMD_LAST)

/* 137: MUSA TQ2 interface */
#define MTGPU_BRIDGE_MUSAXFER                     137UL
#if defined(SUPPORT_FASTRENDER_DM)
#define MTGPU_BRIDGE_MUSAXFER_DISPATCH_FIRST      (MTGPU_BRIDGE_MUSAKICKSYNC_DISPATCH_LAST + 1)
#define MTGPU_BRIDGE_MUSAXFER_DISPATCH_LAST       (MTGPU_BRIDGE_MUSAXFER_DISPATCH_FIRST + MTGPU_BRIDGE_MUSAXFER_CMD_LAST)
#else
#define MTGPU_BRIDGE_MUSAXFER_DISPATCH_FIRST      (0)
#define MTGPU_BRIDGE_MUSAXFER_DISPATCH_LAST       (MTGPU_BRIDGE_MUSAKICKSYNC_DISPATCH_LAST)
#endif

/* 138: MUSA timer query interface */
#define MTGPU_BRIDGE_MUSATIMERQUERY                 138UL
#define MTGPU_BRIDGE_MUSATIMERQUERY_DISPATCH_FIRST  (MTGPU_BRIDGE_MUSAXFER_DISPATCH_LAST + 1)
#define MTGPU_BRIDGE_MUSATIMERQUERY_DISPATCH_LAST   (MTGPU_BRIDGE_MUSATIMERQUERY_DISPATCH_FIRST + MTGPU_BRIDGE_MUSATIMERQUERY_CMD_LAST)

/* 139: MUSA Ray tracing interface */
#define MTGPU_BRIDGE_MUSARAY                 139UL
#if defined(SUPPORT_MUSARAY_BRIDGE)
#define MTGPU_BRIDGE_MUSARAY_DISPATCH_FIRST  (MTGPU_BRIDGE_MUSATIMERQUERY_DISPATCH_LAST + 1)
#define MTGPU_BRIDGE_MUSARAY_DISPATCH_LAST   (MTGPU_BRIDGE_MUSARAY_DISPATCH_FIRST + MTGPU_BRIDGE_MUSARAY_CMD_LAST)
#else
#define MTGPU_BRIDGE_MUSARAY_DISPATCH_FIRST  0
#define MTGPU_BRIDGE_MUSARAY_DISPATCH_LAST   (MTGPU_BRIDGE_MUSATIMERQUERY_DISPATCH_LAST)
#endif

#define MTGPU_BRIDGE_MUSACE 140UL
#define MTGPU_BRIDGE_MUSACE_DISPATCH_FIRST (MTGPU_BRIDGE_MUSARAY_DISPATCH_LAST + 1)
#define MTGPU_BRIDGE_MUSACE_DISPATCH_LAST  (MTGPU_BRIDGE_MUSACE_DISPATCH_FIRST + MTGPU_BRIDGE_MUSACE_CMD_LAST)

#define MTGPU_BRIDGE_MUSA_LAST                   (MTGPU_BRIDGE_MUSACE)
#define MTGPU_BRIDGE_MUSA_DISPATCH_LAST          (MTGPU_BRIDGE_MUSACE_DISPATCH_LAST)

/* bit mask representing the enabled MUSA bridges */

static const MT_UINT32 gui32MUSABridges =
	  (1U << (MTGPU_BRIDGE_MUSATQ - MTGPU_BRIDGE_MUSA_FIRST))
#if defined(MUSA_FEATURE_COMP) || defined(__KERNEL__)
	| (1U << (MTGPU_BRIDGE_MUSACMP - MTGPU_BRIDGE_MUSA_FIRST))
#endif
	| (1U << (MTGPU_BRIDGE_MUSAGFX - MTGPU_BRIDGE_MUSA_FIRST))
#if defined(SUPPORT_BREAKPOINT)
	| (1U << (MTGPU_BRIDGE_BREAKPOINT - MTGPU_BRIDGE_MUSA_FIRST))
#endif
	| (1U << (MTGPU_BRIDGE_MUSAFWDBG - MTGPU_BRIDGE_MUSA_FIRST))
	| (1U << (MTGPU_BRIDGE_MUSAHWPERF - MTGPU_BRIDGE_MUSA_FIRST))
#if defined(SUPPORT_REGCONFIG)
	| (1U << (MTGPU_BRIDGE_MUSAREGCONFIG - MTGPU_BRIDGE_MUSA_FIRST))
#endif
	| (1U << (MTGPU_BRIDGE_MUSAKICKSYNC - MTGPU_BRIDGE_MUSA_FIRST))
#if defined(SUPPORT_FASTRENDER_DM) || defined(__KERNEL__)
	| (1U << (MTGPU_BRIDGE_MUSAXFER - MTGPU_BRIDGE_MUSA_FIRST))
#endif
#if defined(SUPPORT_TIMERQUERY)
	| (1U << (MTGPU_BRIDGE_MUSATIMERQUERY - MTGPU_BRIDGE_MUSA_FIRST))
#endif
	| (1U << (MTGPU_BRIDGE_MUSARAY - MTGPU_BRIDGE_MUSA_FIRST))
	;
/* bit field representing which MUSA bridge groups may optionally not
 * be present in the server
 */

#define MUSA_BRIDGES_OPTIONAL \
	( \
		0 /* no MUSA bridges are currently optional */ \
	)

#if defined(__cplusplus)
}
#endif

#endif /* MUSA_BRIDGE_H */

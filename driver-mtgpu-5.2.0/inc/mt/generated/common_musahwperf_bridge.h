/*******************************************************************************
@File
@Title          Common bridge header for musahwperf
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Declares common defines and structures used by both the client
                and server side of the bridge for musahwperf
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
*******************************************************************************/

#ifndef COMMON_MUSAHWPERF_BRIDGE_H
#define COMMON_MUSAHWPERF_BRIDGE_H

#include <mt/mem_types.h>

#include "img_defs.h"
#include "img_types.h"
#include "mtgpu_error.h"

#include "musa_hwperf.h"

#define MTGPU_BRIDGE_MUSAHWPERF_CMD_FIRST			0
#define MTGPU_BRIDGE_MUSAHWPERF_MUSACTRLHWPERF			MTGPU_BRIDGE_MUSAHWPERF_CMD_FIRST+0
#define MTGPU_BRIDGE_MUSAHWPERF_MUSACONFIGUREHWPERFBLOCKS	MTGPU_BRIDGE_MUSAHWPERF_CMD_FIRST+1
#define MTGPU_BRIDGE_MUSAHWPERF_MUSAGETHWPERFBVNCFEATUREFLAGS	MTGPU_BRIDGE_MUSAHWPERF_CMD_FIRST+2
#define MTGPU_BRIDGE_MUSAHWPERF_MUSACONTROLHWPERFBLOCKS		MTGPU_BRIDGE_MUSAHWPERF_CMD_FIRST+3
#define MTGPU_BRIDGE_MUSAHWPERF_MUSAACQUIREHWPERFFSETTINGS       MTGPU_BRIDGE_MUSAHWPERF_CMD_FIRST+4
#define MTGPU_BRIDGE_MUSAHWPERF_MUSARELEASEHWPERFFSETTINGS       MTGPU_BRIDGE_MUSAHWPERF_CMD_FIRST+5
#define MTGPU_BRIDGE_MUSAHWPERF_MUSAGETHWPERFMASK                MTGPU_BRIDGE_MUSAHWPERF_CMD_FIRST+6
#define MTGPU_BRIDGE_MUSAHWPERF_MUSAGETHWPERFBLOCKCONFIGS        MTGPU_BRIDGE_MUSAHWPERF_CMD_FIRST+7
#define MTGPU_BRIDGE_MUSAHWPERF_MUSAGETSOCTIMER                 MTGPU_BRIDGE_MUSAHWPERF_CMD_FIRST+8
#define MTGPU_BRIDGE_MUSAHWPERF_MUSAPFMINIT                     MTGPU_BRIDGE_MUSAHWPERF_CMD_FIRST+9
#define MTGPU_BRIDGE_MUSAHWPERF_MUSAPFMMSSCONFIG                MTGPU_BRIDGE_MUSAHWPERF_CMD_FIRST+10
#define MTGPU_BRIDGE_MUSAHWPERF_MUSAPFMGLOBALCONFIG             MTGPU_BRIDGE_MUSAHWPERF_CMD_FIRST+11
#define MTGPU_BRIDGE_MUSAHWPERF_MUSAPFMINSTANCECONFIG           MTGPU_BRIDGE_MUSAHWPERF_CMD_FIRST+12
#define MTGPU_BRIDGE_MUSAHWPERF_MUSAPFMWRAPPERCONFIG            MTGPU_BRIDGE_MUSAHWPERF_CMD_FIRST+13
#define MTGPU_BRIDGE_MUSAHWPERF_MUSAPFMDUMPTRIGGER              MTGPU_BRIDGE_MUSAHWPERF_CMD_FIRST+14
#define MTGPU_BRIDGE_MUSAHWPERF_MUSAPFMGETSTAT                  MTGPU_BRIDGE_MUSAHWPERF_CMD_FIRST+15
#define MTGPU_BRIDGE_MUSAHWPERF_MUSAPFMDEINIT                   MTGPU_BRIDGE_MUSAHWPERF_CMD_FIRST+16
#define MTGPU_BRIDGE_MUSAHWPERF_MUSAGETCONTAINERPID             MTGPU_BRIDGE_MUSAHWPERF_CMD_FIRST+17
#define MTGPU_BRIDGE_MUSAHWPERF_CMD_LAST            		(MTGPU_BRIDGE_MUSAHWPERF_CMD_FIRST+17)

/*******************************************
            MUSACtrlHWPerf
 *******************************************/

/* Bridge in structure for MUSACtrlHWPerf */
typedef struct MTGPU_BRIDGE_IN_MUSACTRLHWPERF_TAG
{
	MT_UINT64 ui64Mask;
	MT_BOOL bToggle;
	MT_UINT32 ui32StreamId;
} __packed MTGPU_BRIDGE_IN_MUSACTRLHWPERF;

/* Bridge out structure for MUSACtrlHWPerf */
typedef struct MTGPU_BRIDGE_OUT_MUSACTRLHWPERF_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_MUSACTRLHWPERF;

/*******************************************
	    MTGPUGetHWPerfMask
 *******************************************/
typedef struct MTGPU_BRIDGE_IN_MUSAGETHWPERFMASK_TAG
{
	MT_UINT32 ui32StreamId;
} __packed MTGPU_BRIDGE_IN_MUSAGETHWPERFMASK;

typedef struct MTGPU_BRIDGE_OUT_MUSAGETHWPERFMASK_TAG
{
	MTGPU_ERROR eError;
	MT_UINT64 ui64Mask;
} __packed MTGPU_BRIDGE_OUT_MUSAGETHWPERFMASK;

/*******************************************
            MTGPUGetSOCTimer
 *******************************************/
typedef struct MTGPU_BRIDGE_OUT_MUSAGETSOCTIMER_TAG
{
	MTGPU_ERROR eError;
	MT_UINT64 ui64SocTimestamp;
	MT_UINT64 ui64OSTimestamp;
} __packed MTGPU_BRIDGE_OUT_MUSAGETSOCTIMER;
/*******************************************
            MTGPUGetContainerPID
 *******************************************/
typedef struct MTGPU_BRIDGE_IN_MUSAGETCONTAINERPID_TAG
{
	MT_PID uHostPID;
}__packed MTGPU_BRIDGE_IN_MUSAGETCONTAINERPID;

typedef struct MTGPU_BRIDGE_OUT_MUSAGETCONTAINERPID_TAG
{
	MTGPU_ERROR eError;
	MT_PID uContainerPID;
} __packed MTGPU_BRIDGE_OUT_MUSAGETCONTAINERPID;

/*******************************************
	    MUSAAquireHWPerfSettings
 *******************************************/
typedef struct MTGPU_BRIDGE_OUT_MUSAACQUIREHWPERFSETTING_TAG
{
	MTGPU_ERROR eError;
	MT_HANDLE hPMR;
} __packed MTGPU_BRIDGE_OUT_MUSAACQUIREHWPERFSETTING;

/*******************************************
            MUSAReleaseHWPerfSettings
 *******************************************/
typedef struct MTGPU_BRIDGE_IN_MUSARELEASEHWPERFSETTING_TAG
{
	MT_HANDLE hPMR;
} __packed MTGPU_BRIDGE_IN_MUSARELEASEHWPERFSETTING;

typedef struct MTGPU_BRIDGE_OUT_MUSARELEASEHWPERFSETTING_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_MUSARELEASEHWPERFSETTING;

/*******************************************
            MUSAConfigureHWPerfBlocks
 *******************************************/

/* Bridge in structure for MUSAConfigureHWPerfBlocks */
typedef struct MTGPU_BRIDGE_IN_MUSACONFIGUREHWPERFBLOCKS_TAG
{
	MUSA_HWPERF_CONFIG_CNTBLK *psBlockConfigs;
	MT_UINT32 ui32ArrayLen;
	MT_UINT32 ui32CtrlWord;
} __packed MTGPU_BRIDGE_IN_MUSACONFIGUREHWPERFBLOCKS;

/* Bridge out structure for MUSAConfigureHWPerfBlocks */
typedef struct MTGPU_BRIDGE_OUT_MUSACONFIGUREHWPERFBLOCKS_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_MUSACONFIGUREHWPERFBLOCKS;

/*******************************************
	    MUSAGetHWPerfBlockConfigs
 *******************************************/

/* Bridge in structure for MUSAGetHWPerfBlockConfigs */
typedef struct MTGPU_BRIDGE_IN_MUSAGETHWPERFBLOCKCONFIGS_TAG
{
	MUSA_HWPERF_CONFIG_CNTBLK *psBlockConfigs;
	MT_UINT32 ui32ArrayLen;
} __packed MTGPU_BRIDGE_IN_MUSAGETHWPERFBLOCKCONFIGS;

/* Bridge out structure for MUSAGetHWPerfBlockConfigs */
typedef struct MTGPU_BRIDGE_OUT_MUSAGETHWPERFBLOCKCONFIGS_TAG
{
	MTGPU_ERROR eError;
	MUSA_HWPERF_CONFIG_CNTBLK *psBlockConfigs;
	MT_UINT32 ui32FoundLen;
} __packed MTGPU_BRIDGE_OUT_MUSAGETHWPERFBLOCKCONFIGS;

/*******************************************
            MUSAGetHWPerfBvncFeatureFlags
 *******************************************/

/* Bridge in structure for MUSAGetHWPerfBvncFeatureFlags */
typedef struct MTGPU_BRIDGE_IN_MUSAGETHWPERFBVNCFEATUREFLAGS_TAG
{
	MT_UINT32 ui32EmptyStructPlaceholder;
} __packed MTGPU_BRIDGE_IN_MUSAGETHWPERFBVNCFEATUREFLAGS;

/* Bridge out structure for MUSAGetHWPerfBvncFeatureFlags */
typedef struct MTGPU_BRIDGE_OUT_MUSAGETHWPERFBVNCFEATUREFLAGS_TAG
{
	MUSA_HWPERF_BVNC sBVNC;
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_MUSAGETHWPERFBVNCFEATUREFLAGS;

/*******************************************
            MUSAControlHWPerfBlocks
 *******************************************/

/* Bridge in structure for MUSAControlHWPerfBlocks */
typedef struct MTGPU_BRIDGE_IN_MUSACONTROLHWPERFBLOCKS_TAG
{
	MT_UINT16 *pui16BlockIDs;
	MT_BOOL bEnable;
	MT_UINT32 ui32ArrayLen;
} __packed MTGPU_BRIDGE_IN_MUSACONTROLHWPERFBLOCKS;

/* Bridge out structure for MUSAControlHWPerfBlocks */
typedef struct MTGPU_BRIDGE_OUT_MUSACONTROLHWPERFBLOCKS_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_MUSACONTROLHWPERFBLOCKS;

/*******************************************
            MUSAPFMGetStat
 *******************************************/
/* Bridge in structure for MUSAPFMGetStat */
typedef struct MTGPU_BRIDGE_IN_MUSAPFMDGETSTAT_TAG
{
	MUSA_HWPERF_PFM_GET_STAT sGetStat;
} __packed MTGPU_BRIDGE_IN_MUSAPFMGETSTAT;
/* Bridge out structure for MUSAPFMGetStat */
typedef struct MTGPU_BRIDGE_OUT_MUSAPFMGETSTAT_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_MUSAPFMGETSTAT;

/*******************************************
            MUSAPFMDeinit
 *******************************************/
/* Bridge in structure for MUSAPFMDeinit */
typedef struct MTGPU_BRIDGE_IN_MUSAPFMDEINIT_TAG
{
	MT_HANDLE hPMR;
} __packed MTGPU_BRIDGE_IN_MUSAPFMDEINIT;

/* Bridge out structure for MUSAPFMDeinit */
typedef struct MTGPU_BRIDGE_OUT_MUSAPFMDEINIT_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_MUSAPFMDEINIT;

/*******************************************
            MUSAHWPerfPFMDumpTrigger
 *******************************************/
/* Bridge in structure for MUSAPFMDumpTrig */
typedef struct MTGPU_BRIDGE_IN_MUSAPFMDUMPTRIGGER_TAG
{
	MUSA_HWPERF_PFM_DUMP_TRIGGER_CONFIG sDumpTrig;
} __packed MTGPU_BRIDGE_IN_MUSAPFMDUMPTRIGGER;

/* Bridge out structure for MUSAPFMDumpTrig */
typedef struct MTGPU_BRIDGE_OUT_MUSAPFMDUMPTRIGGER_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_MUSAPFMDUMPTRIGGER;

/*******************************************
            MUSAPFMSetMSSConfig
 *******************************************/
/* Bridge in structure for MUSAPFMSetCounterMSSConfig */
typedef struct MTGPU_BRIDGE_IN_MUSAPFMMSSCONFIG_TAG
{
        MUSA_HWPERF_PFM_MSS_CONFIG sMSSConfig;
} __packed MTGPU_BRIDGE_IN_MUSAPFMMSSCONFIG;

/* Bridge out structure for MUSAPFMSetCounterMSSConfig */
typedef struct MTGPU_BRIDGE_OUT_MUSAPFMMSSCONFIG_TAG
{
        MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_MUSAPFMMSSCONFIG;

/*******************************************
            MUSAPFMSetInstanceConfig
 *******************************************/
/* Bridge in structure for MUSAPFMSetCounterInstanceConfig */
typedef struct MTGPU_BRIDGE_IN_MUSAPFMINSTANCECONFIG_TAG
{
	MUSA_HWPERF_PFM_INSTANCE_CONFIG sInstanceConfig;
} __packed MTGPU_BRIDGE_IN_MUSAPFMINSTANCECONFIG;

/* Bridge out structure for MUSAPFMSetCounterInstanceConfig */
typedef struct MTGPU_BRIDGE_OUT_MUSAPFMINSTANCECONFIG_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_MUSAPFMINSTANCECONFIG;

/*******************************************
            MUSAPFMSetWrapperConfig
 *******************************************/
/* Bridge in structure for MUSAPFMSetCounterInstanceConfig */
typedef struct MTGPU_BRIDGE_IN_MUSAPFMWRAPPERCONFIG_TAG
{
	MUSA_HWPERF_PFM_WRAPPER_CONFIG sWrapperConfig;
} __packed MTGPU_BRIDGE_IN_MUSAPFMWRAPPERCONFIG;

/* Bridge out structure for MUSAPFMSetCounterInstanceConfig */
typedef struct MTGPU_BRIDGE_OUT_MUSAPFMWRAPPERCONFIG_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_MUSAPFMWRAPPERCONFIG;

/*******************************************
            MUSAPFMInit
 *******************************************/
/* Bridge in structure for MUSAPFMInit */
typedef struct MTGPU_BRIDGE_IN_MUSAPFMINIT_TAG
{
	MUSA_HWPERF_PFM_INIT_PARAM sInitParam;
} __packed MTGPU_BRIDGE_IN_MUSAPFMINIT;

/* Bridge out structure for MUSAPFMInit */
typedef struct MTGPU_BRIDGE_OUT_MUSAPFMINIT_TAG
{
	MT_HANDLE hPMR;
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_MUSAPFMINIT;

/*******************************************
            MUSAPFMSetGlobalCounterConfig
 *******************************************/
/* Bridge in structure for MUSAPFMSetGlobalCounterConfig */
typedef struct MTGPU_BRIDGE_IN_MUSAPFMGLOBALCONFIG_TAG
{
	MUSA_HWPERF_PFM_GLOBAL_CONFIG sGlobalConfig;
} __packed MTGPU_BRIDGE_IN_MUSAPFMGLOBALCONFIG;

/* Bridge out structure for MUSAPFMSetGlobalCounterConfig */
typedef struct MTGPU_BRIDGE_OUT_MUSAPFMGLOBALCONFIG_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_MUSAPFMGLOBALCONFIG;
#endif /* COMMON_MUSAHWPERF_BRIDGE_H */

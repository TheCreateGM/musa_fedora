/*******************************************************************************
@File
@Title          Common bridge header for musafwdbg
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Declares common defines and structures used by both the client
                and server side of the bridge for musafwdbg
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

#ifndef COMMON_MUSAFWDBG_BRIDGE_H
#define COMMON_MUSAFWDBG_BRIDGE_H

#include <mt/mem_types.h>

#include "img_defs.h"
#include "img_types.h"
#include "mtgpu_error.h"

#include "devicemem_typedefs.h"
#include "mtgpu_memallocflags.h"

#define MTGPU_BRIDGE_MUSAFWDBG_CMD_FIRST			0
#define MTGPU_BRIDGE_MUSAFWDBG_MUSAFWDEBUGSETFWLOG			MTGPU_BRIDGE_MUSAFWDBG_CMD_FIRST+0
#define MTGPU_BRIDGE_MUSAFWDBG_MUSAFWDEBUGDUMPFREELISTPAGELIST			MTGPU_BRIDGE_MUSAFWDBG_CMD_FIRST+1
#define MTGPU_BRIDGE_MUSAFWDBG_MUSAFWDEBUGSETHCSDEADLINE			MTGPU_BRIDGE_MUSAFWDBG_CMD_FIRST+2
#define MTGPU_BRIDGE_MUSAFWDBG_MUSAFWDEBUGSETOSIDPRIORITY			MTGPU_BRIDGE_MUSAFWDBG_CMD_FIRST+3
#define MTGPU_BRIDGE_MUSAFWDBG_MUSAFWDEBUGSETOSNEWONLINESTATE			MTGPU_BRIDGE_MUSAFWDBG_CMD_FIRST+4
#define MTGPU_BRIDGE_MUSAFWDBG_MUSAFWDEBUGPHRCONFIGURE			MTGPU_BRIDGE_MUSAFWDBG_CMD_FIRST+5
#define MTGPU_BRIDGE_MUSAFWDBG_MUSAFWDEBUGWDGCONFIGURE			MTGPU_BRIDGE_MUSAFWDBG_CMD_FIRST+6
#define MTGPU_BRIDGE_MUSAFWDBG_MUSACURRENTTIME			MTGPU_BRIDGE_MUSAFWDBG_CMD_FIRST+7
#define MTGPU_BRIDGE_MUSAFWDBG_CMD_LAST			(MTGPU_BRIDGE_MUSAFWDBG_CMD_FIRST+7)

/*******************************************
            MUSAFWDebugSetFWLog
 *******************************************/

/* Bridge in structure for MUSAFWDebugSetFWLog */
typedef struct MTGPU_BRIDGE_IN_MUSAFWDEBUGSETFWLOG_TAG
{
	MT_UINT32 ui32MUSAFWLogType;
} __packed MTGPU_BRIDGE_IN_MUSAFWDEBUGSETFWLOG;

/* Bridge out structure for MUSAFWDebugSetFWLog */
typedef struct MTGPU_BRIDGE_OUT_MUSAFWDEBUGSETFWLOG_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_MUSAFWDEBUGSETFWLOG;

/*******************************************
            MUSAFWDebugDumpFreelistPageList
 *******************************************/

/* Bridge in structure for MUSAFWDebugDumpFreelistPageList */
typedef struct MTGPU_BRIDGE_IN_MUSAFWDEBUGDUMPFREELISTPAGELIST_TAG
{
	MT_UINT32 ui32EmptyStructPlaceholder;
} __packed MTGPU_BRIDGE_IN_MUSAFWDEBUGDUMPFREELISTPAGELIST;

/* Bridge out structure for MUSAFWDebugDumpFreelistPageList */
typedef struct MTGPU_BRIDGE_OUT_MUSAFWDEBUGDUMPFREELISTPAGELIST_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_MUSAFWDEBUGDUMPFREELISTPAGELIST;

/*******************************************
            MUSAFWDebugSetHCSDeadline
 *******************************************/

/* Bridge in structure for MUSAFWDebugSetHCSDeadline */
typedef struct MTGPU_BRIDGE_IN_MUSAFWDEBUGSETHCSDEADLINE_TAG
{
	MT_UINT32 ui32MUSAHCSDeadline;
} __packed MTGPU_BRIDGE_IN_MUSAFWDEBUGSETHCSDEADLINE;

/* Bridge out structure for MUSAFWDebugSetHCSDeadline */
typedef struct MTGPU_BRIDGE_OUT_MUSAFWDEBUGSETHCSDEADLINE_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_MUSAFWDEBUGSETHCSDEADLINE;

/*******************************************
            MUSAFWDebugSetOSidPriority
 *******************************************/

/* Bridge in structure for MUSAFWDebugSetOSidPriority */
typedef struct MTGPU_BRIDGE_IN_MUSAFWDEBUGSETOSIDPRIORITY_TAG
{
	MT_UINT32 ui32OSid;
	MT_UINT32 ui32Priority;
} __packed MTGPU_BRIDGE_IN_MUSAFWDEBUGSETOSIDPRIORITY;

/* Bridge out structure for MUSAFWDebugSetOSidPriority */
typedef struct MTGPU_BRIDGE_OUT_MUSAFWDEBUGSETOSIDPRIORITY_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_MUSAFWDEBUGSETOSIDPRIORITY;

/*******************************************
            MUSAFWDebugSetOSNewOnlineState
 *******************************************/

/* Bridge in structure for MUSAFWDebugSetOSNewOnlineState */
typedef struct MTGPU_BRIDGE_IN_MUSAFWDEBUGSETOSNEWONLINESTATE_TAG
{
	MT_UINT32 ui32OSNewState;
	MT_UINT32 ui32OSid;
} __packed MTGPU_BRIDGE_IN_MUSAFWDEBUGSETOSNEWONLINESTATE;

/* Bridge out structure for MUSAFWDebugSetOSNewOnlineState */
typedef struct MTGPU_BRIDGE_OUT_MUSAFWDEBUGSETOSNEWONLINESTATE_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_MUSAFWDEBUGSETOSNEWONLINESTATE;

/*******************************************
            MUSAFWDebugPHRConfigure
 *******************************************/

/* Bridge in structure for MUSAFWDebugPHRConfigure */
typedef struct MTGPU_BRIDGE_IN_MUSAFWDEBUGPHRCONFIGURE_TAG
{
	MT_UINT32 ui32ui32PHRMode;
} __packed MTGPU_BRIDGE_IN_MUSAFWDEBUGPHRCONFIGURE;

/* Bridge out structure for MUSAFWDebugPHRConfigure */
typedef struct MTGPU_BRIDGE_OUT_MUSAFWDEBUGPHRCONFIGURE_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_MUSAFWDEBUGPHRCONFIGURE;

/*******************************************
            MUSAFWDebugWdgConfigure
 *******************************************/

/* Bridge in structure for MUSAFWDebugWdgConfigure */
typedef struct MTGPU_BRIDGE_IN_MUSAFWDEBUGWDGCONFIGURE_TAG
{
	MT_UINT32 ui32ui32WdgPeriodUs;
} __packed MTGPU_BRIDGE_IN_MUSAFWDEBUGWDGCONFIGURE;

/* Bridge out structure for MUSAFWDebugWdgConfigure */
typedef struct MTGPU_BRIDGE_OUT_MUSAFWDEBUGWDGCONFIGURE_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_MUSAFWDEBUGWDGCONFIGURE;

/*******************************************
            MUSACurrentTime
 *******************************************/

/* Bridge in structure for MUSACurrentTime */
typedef struct MTGPU_BRIDGE_IN_MUSACURRENTTIME_TAG
{
	MT_UINT32 ui32EmptyStructPlaceholder;
} __packed MTGPU_BRIDGE_IN_MUSACURRENTTIME;

/* Bridge out structure for MUSACurrentTime */
typedef struct MTGPU_BRIDGE_OUT_MUSACURRENTTIME_TAG
{
	MT_UINT64 ui64Time;
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_MUSACURRENTTIME;

#endif /* COMMON_MUSAFWDBG_BRIDGE_H */

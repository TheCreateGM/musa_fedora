/*******************************************************************************
@File
@Title          Common bridge header for htbuffer
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Declares common defines and structures used by both the client
                and server side of the bridge for htbuffer
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

#ifndef COMMON_HTBUFFER_BRIDGE_H
#define COMMON_HTBUFFER_BRIDGE_H

#include <mt/mem_types.h>

#include "img_defs.h"
#include "img_types.h"
#include "mtgpu_error.h"

#include "devicemem_typedefs.h"
#include "htbuffer_types.h"

#define MTGPU_BRIDGE_HTBUFFER_CMD_FIRST			0
#define MTGPU_BRIDGE_HTBUFFER_HTBCONTROL			MTGPU_BRIDGE_HTBUFFER_CMD_FIRST+0
#define MTGPU_BRIDGE_HTBUFFER_HTBLOG			MTGPU_BRIDGE_HTBUFFER_CMD_FIRST+1
#define MTGPU_BRIDGE_HTBUFFER_CMD_LAST			(MTGPU_BRIDGE_HTBUFFER_CMD_FIRST+1)

/*******************************************
            HTBControl
 *******************************************/

/* Bridge in structure for HTBControl */
typedef struct MTGPU_BRIDGE_IN_HTBCONTROL_TAG
{
	MT_UINT32 *pui32GroupEnable;
	MT_UINT32 ui32EnablePID;
	MT_UINT32 ui32LogLevel;
	MT_UINT32 ui32LogMode;
	MT_UINT32 ui32NumGroups;
	MT_UINT32 ui32OpMode;
} __packed MTGPU_BRIDGE_IN_HTBCONTROL;

/* Bridge out structure for HTBControl */
typedef struct MTGPU_BRIDGE_OUT_HTBCONTROL_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_HTBCONTROL;

/*******************************************
            HTBLog
 *******************************************/

/* Bridge in structure for HTBLog */
typedef struct MTGPU_BRIDGE_IN_HTBLOG_TAG
{
	MT_UINT64 ui64TimeStamp;
	MT_UINT32 *pui32Args;
	MT_UINT32 ui32NumArgs;
	MT_UINT32 ui32PID;
	MT_UINT32 ui32SF;
	MT_UINT32 ui32TID;
} __packed MTGPU_BRIDGE_IN_HTBLOG;

/* Bridge out structure for HTBLog */
typedef struct MTGPU_BRIDGE_OUT_HTBLOG_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_HTBLOG;

#endif /* COMMON_HTBUFFER_BRIDGE_H */

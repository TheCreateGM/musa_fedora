/*******************************************************************************
@File
@Title          Common bridge header for di
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Declares common defines and structures used by both the client
                and server side of the bridge for di
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

#ifndef COMMON_DI_BRIDGE_H
#define COMMON_DI_BRIDGE_H

#include <mt/mem_types.h>

#include "img_defs.h"
#include "img_types.h"
#include "mtgpu_error.h"

#include "mtgpu_tlcommon.h"
#include "mt_dicommon.h"

#define MTGPU_BRIDGE_DI_CMD_FIRST			0
#define MTGPU_BRIDGE_DI_DICREATECONTEXT			MTGPU_BRIDGE_DI_CMD_FIRST+0
#define MTGPU_BRIDGE_DI_DIDESTROYCONTEXT			MTGPU_BRIDGE_DI_CMD_FIRST+1
#define MTGPU_BRIDGE_DI_DIREADENTRY			MTGPU_BRIDGE_DI_CMD_FIRST+2
#define MTGPU_BRIDGE_DI_DIWRITEENTRY			MTGPU_BRIDGE_DI_CMD_FIRST+3
#define MTGPU_BRIDGE_DI_DILISTALLENTRIES			MTGPU_BRIDGE_DI_CMD_FIRST+4
#define MTGPU_BRIDGE_DI_CMD_LAST			(MTGPU_BRIDGE_DI_CMD_FIRST+4)

/*******************************************
            DICreateContext
 *******************************************/

/* Bridge in structure for DICreateContext */
typedef struct MTGPU_BRIDGE_IN_DICREATECONTEXT_TAG
{
	MT_CHAR *puiStreamName;
} __packed MTGPU_BRIDGE_IN_DICREATECONTEXT;

/* Bridge out structure for DICreateContext */
typedef struct MTGPU_BRIDGE_OUT_DICREATECONTEXT_TAG
{
	MT_HANDLE hContext;
	MT_CHAR *puiStreamName;
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_DICREATECONTEXT;

/*******************************************
            DIDestroyContext
 *******************************************/

/* Bridge in structure for DIDestroyContext */
typedef struct MTGPU_BRIDGE_IN_DIDESTROYCONTEXT_TAG
{
	MT_HANDLE hContext;
} __packed MTGPU_BRIDGE_IN_DIDESTROYCONTEXT;

/* Bridge out structure for DIDestroyContext */
typedef struct MTGPU_BRIDGE_OUT_DIDESTROYCONTEXT_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_DIDESTROYCONTEXT;

/*******************************************
            DIReadEntry
 *******************************************/

/* Bridge in structure for DIReadEntry */
typedef struct MTGPU_BRIDGE_IN_DIREADENTRY_TAG
{
	MT_UINT64 ui64Offset;
	MT_UINT64 ui64Size;
	MT_HANDLE hContext;
	const MT_CHAR *puiEntryPath;
} __packed MTGPU_BRIDGE_IN_DIREADENTRY;

/* Bridge out structure for DIReadEntry */
typedef struct MTGPU_BRIDGE_OUT_DIREADENTRY_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_DIREADENTRY;

/*******************************************
            DIWriteEntry
 *******************************************/

/* Bridge in structure for DIWriteEntry */
typedef struct MTGPU_BRIDGE_IN_DIWRITEENTRY_TAG
{
	MT_HANDLE hContext;
	const MT_CHAR *puiEntryPath;
	const MT_CHAR *puiValue;
	MT_UINT32 ui32ValueSize;
} __packed MTGPU_BRIDGE_IN_DIWRITEENTRY;

/* Bridge out structure for DIWriteEntry */
typedef struct MTGPU_BRIDGE_OUT_DIWRITEENTRY_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_DIWRITEENTRY;

/*******************************************
            DIListAllEntries
 *******************************************/

/* Bridge in structure for DIListAllEntries */
typedef struct MTGPU_BRIDGE_IN_DILISTALLENTRIES_TAG
{
	MT_HANDLE hContext;
} __packed MTGPU_BRIDGE_IN_DILISTALLENTRIES;

/* Bridge out structure for DIListAllEntries */
typedef struct MTGPU_BRIDGE_OUT_DILISTALLENTRIES_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_DILISTALLENTRIES;

#endif /* COMMON_DI_BRIDGE_H */

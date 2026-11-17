/*******************************************************************************
@File
@Title          Common bridge header for cache
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Declares common defines and structures used by both the client
                and server side of the bridge for cache
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

#ifndef COMMON_CACHE_BRIDGE_H
#define COMMON_CACHE_BRIDGE_H

#include <mt/mem_types.h>

#include "img_defs.h"
#include "img_types.h"
#include "mtgpu_error.h"

#include "cache_ops.h"

#define MTGPU_BRIDGE_CACHE_CMD_FIRST			0
#define MTGPU_BRIDGE_CACHE_CACHEOPQUEUE			MTGPU_BRIDGE_CACHE_CMD_FIRST+0
#define MTGPU_BRIDGE_CACHE_CACHEOPEXEC			MTGPU_BRIDGE_CACHE_CMD_FIRST+1
#define MTGPU_BRIDGE_CACHE_CACHEOPLOG			MTGPU_BRIDGE_CACHE_CMD_FIRST+2
#define MTGPU_BRIDGE_CACHE_CMD_LAST			(MTGPU_BRIDGE_CACHE_CMD_FIRST+2)

/*******************************************
            CacheOpQueue
 *******************************************/

/* Bridge in structure for CacheOpQueue */
typedef struct MTGPU_BRIDGE_IN_CACHEOPQUEUE_TAG
{
	MTGPU_CACHE_OP *piuCacheOp;
	MT_UINT64 *pui64Address;
	MT_DEVMEM_OFFSET_T *puiOffset;
	MT_DEVMEM_SIZE_T *puiSize;
	MT_HANDLE *phPMR;
	MT_UINT32 ui32NumCacheOps;
	MT_UINT32 ui32OpTimeline;
} __packed MTGPU_BRIDGE_IN_CACHEOPQUEUE;

/* Bridge out structure for CacheOpQueue */
typedef struct MTGPU_BRIDGE_OUT_CACHEOPQUEUE_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_CACHEOPQUEUE;

/*******************************************
            CacheOpExec
 *******************************************/

/* Bridge in structure for CacheOpExec */
typedef struct MTGPU_BRIDGE_IN_CACHEOPEXEC_TAG
{
	MT_UINT64 ui64Address;
	MT_DEVMEM_OFFSET_T uiOffset;
	MT_DEVMEM_SIZE_T uiSize;
	MT_HANDLE hPMR;
	MTGPU_CACHE_OP iuCacheOp;
} __packed MTGPU_BRIDGE_IN_CACHEOPEXEC;

/* Bridge out structure for CacheOpExec */
typedef struct MTGPU_BRIDGE_OUT_CACHEOPEXEC_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_CACHEOPEXEC;

/*******************************************
            CacheOpLog
 *******************************************/

/* Bridge in structure for CacheOpLog */
typedef struct MTGPU_BRIDGE_IN_CACHEOPLOG_TAG
{
	MT_INT64 i64EndTime;
	MT_INT64 i64StartTime;
	MT_UINT64 ui64Address;
	MT_DEVMEM_OFFSET_T uiOffset;
	MT_DEVMEM_SIZE_T uiSize;
	MT_HANDLE hPMR;
	MTGPU_CACHE_OP iuCacheOp;
} __packed MTGPU_BRIDGE_IN_CACHEOPLOG;

/* Bridge out structure for CacheOpLog */
typedef struct MTGPU_BRIDGE_OUT_CACHEOPLOG_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_CACHEOPLOG;

#endif /* COMMON_CACHE_BRIDGE_H */

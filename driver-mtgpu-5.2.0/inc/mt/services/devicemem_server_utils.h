/**************************************************************************/ /*!
@File
@Title          Device Memory Management
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Header file utilities that are specific to device memory functions
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
*/ /***************************************************************************/

#include "img_defs.h"
#include "img_types.h"
#include "device.h"
#include "mtgpu_memallocflags.h"
#include "mt.h"

static INLINE MTGPU_ERROR DevmemCPUCacheMode(MTGPU_DEVICE_NODE *psDeviceNode,
											  MTGPU_MEMALLOCFLAGS_T ulFlags,
											  MT_UINT32 *pui32Ret)
{
	MT_UINT32 ui32CPUCacheMode = MTGPU_CPU_CACHE_MODE(ulFlags);
	MT_UINT32 ui32Ret;
	MTGPU_ERROR eError = MTGPU_OK;

	MT_ASSERT(ui32CPUCacheMode == MTGPU_CPU_CACHE_MODE(ulFlags));

	switch (ui32CPUCacheMode)
	{
		case MTGPU_MEMALLOCFLAG_CPU_UNCACHED:
			ui32Ret = MTGPU_MEMALLOCFLAG_CPU_UNCACHED;
			break;

		case MTGPU_MEMALLOCFLAG_CPU_UNCACHED_WC:
			ui32Ret = MTGPU_MEMALLOCFLAG_CPU_UNCACHED_WC;
			break;

		case MTGPU_MEMALLOCFLAG_CPU_CACHE_INCOHERENT:
			ui32Ret = MTGPU_MEMALLOCFLAG_CPU_CACHED;
			break;

		case MTGPU_MEMALLOCFLAG_CPU_CACHE_COHERENT:

			/*
			 * If system has no coherency but coherency has been requested for CPU
			 * and GPU we currently fall back to write-combine.
			 * This avoids errors on arm64 when uncached is turned into ordered device memory
			 * and suffers from problems with unaligned access.
			 */
			if ( (MTGPU_GPU_CACHE_MODE(ulFlags) == MTGPU_MEMALLOCFLAG_GPU_CACHE_COHERENT) &&
				!(MTGPUSystemSnoopingOfCPUCache(psDeviceNode->psDevConfig) && MTGPUSystemSnoopingOfDeviceCache(psDeviceNode->psDevConfig)) )
			{
				ui32Ret = MTGPU_MEMALLOCFLAG_CPU_UNCACHED_WC;
			}
			else
			{
				ui32Ret = MTGPU_MEMALLOCFLAG_CPU_CACHED;
			}
			break;

		default:
			MT_LOG(("DevmemCPUCacheMode: Unknown CPU cache mode 0x%08x", ui32CPUCacheMode));
			MT_ASSERT(0);
			/*
				We should never get here, but if we do then setting the mode
				to uncached is the safest thing to do.
			*/
			ui32Ret = MTGPU_MEMALLOCFLAG_CPU_UNCACHED;
			eError = MTGPU_ERROR_UNSUPPORTED_CACHE_MODE;
			break;
	}

	*pui32Ret = ui32Ret;

	return eError;
}

static INLINE MTGPU_ERROR DevmemDeviceCacheMode(MTGPU_DEVICE_NODE *psDeviceNode,
												 MTGPU_MEMALLOCFLAGS_T ulFlags,
												 MT_UINT32 *pui32Ret)
{
	MT_UINT32 ui32DeviceCacheMode = MTGPU_GPU_CACHE_MODE(ulFlags);
	MT_UINT32 ui32Ret;
	MTGPU_ERROR eError = MTGPU_OK;

	MT_ASSERT(ui32DeviceCacheMode == MTGPU_GPU_CACHE_MODE(ulFlags));

	switch (ui32DeviceCacheMode)
	{
		case MTGPU_MEMALLOCFLAG_GPU_UNCACHED:
			ui32Ret = MTGPU_MEMALLOCFLAG_GPU_UNCACHED;
			break;

		case MTGPU_MEMALLOCFLAG_GPU_UNCACHED_WC:
			ui32Ret = MTGPU_MEMALLOCFLAG_GPU_UNCACHED_WC;
			break;

		case MTGPU_MEMALLOCFLAG_GPU_CACHE_INCOHERENT:
			ui32Ret = MTGPU_MEMALLOCFLAG_GPU_CACHED;
			break;

		case MTGPU_MEMALLOCFLAG_GPU_CACHE_COHERENT:

			/*
			 * If system has no coherency but coherency has been requested for CPU
			 * and GPU we currently fall back to write-combine.
			 * This avoids errors on arm64 when uncached is turned into ordered device memory
			 * and suffers from problems with unaligned access.
			 */
			if ( (MTGPU_CPU_CACHE_MODE(ulFlags) == MTGPU_MEMALLOCFLAG_CPU_CACHE_COHERENT) &&
				!(MTGPUSystemSnoopingOfCPUCache(psDeviceNode->psDevConfig) && MTGPUSystemSnoopingOfDeviceCache(psDeviceNode->psDevConfig)) )
			{
				ui32Ret = MTGPU_MEMALLOCFLAG_GPU_UNCACHED_WC;
			}
			else if (MTGPUSystemSnoopingOfCPUCache(psDeviceNode->psDevConfig) &&
				 !MTGPUSystemSnoopingOfDeviceCache(psDeviceNode->psDevConfig))
			{
				/* The system just supports gpu to snoop cpu cache. */
				ui32Ret = MTGPU_MEMALLOCFLAG_GPU_UNCACHED_WC;
			}
			else
			{
				ui32Ret = MTGPU_MEMALLOCFLAG_GPU_CACHED;
			}
			break;

		default:
			MT_LOG(("DevmemDeviceCacheMode: Unknown device cache mode 0x%08x", ui32DeviceCacheMode));
			MT_ASSERT(0);
			/*
				We should never get here, but if we do then setting the mode
				to uncached is the safest thing to do.
			*/
			ui32Ret = MTGPU_MEMALLOCFLAG_GPU_UNCACHED;
			eError = MTGPU_ERROR_UNSUPPORTED_CACHE_MODE;
			break;
	}

	*pui32Ret = ui32Ret;

	return eError;
}

static INLINE MT_BOOL DevmemCPUCacheCoherency(MTGPU_DEVICE_NODE *psDeviceNode,
											   MTGPU_MEMALLOCFLAGS_T ulFlags)
{
	MT_UINT32 ui32CPUCacheMode = MTGPU_CPU_CACHE_MODE(ulFlags);
	MT_BOOL bRet = MT_FALSE;

	MT_ASSERT(ui32CPUCacheMode == MTGPU_CPU_CACHE_MODE(ulFlags));

	if (ui32CPUCacheMode == MTGPU_MEMALLOCFLAG_CPU_CACHE_COHERENT)
	{
		bRet = MTGPUSystemSnoopingOfDeviceCache(psDeviceNode->psDevConfig);
	}
	return bRet;
}

static INLINE MT_BOOL DevmemDeviceCacheCoherency(MTGPU_DEVICE_NODE *psDeviceNode,
												  MTGPU_MEMALLOCFLAGS_T ulFlags)
{
	MT_UINT32 ui32DeviceCacheMode = MTGPU_GPU_CACHE_MODE(ulFlags);
	MT_BOOL bRet = MT_FALSE;

	MT_ASSERT(ui32DeviceCacheMode == MTGPU_GPU_CACHE_MODE(ulFlags));

	if (ui32DeviceCacheMode == MTGPU_MEMALLOCFLAG_GPU_CACHE_COHERENT)
	{
		bRet = MTGPUSystemSnoopingOfCPUCache(psDeviceNode->psDevConfig);
	}
	return bRet;
}

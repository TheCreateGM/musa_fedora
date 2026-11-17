/*******************************************************************************
@File
@Title          Common bridge header for mttl
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Declares common defines and structures used by both the client
                and server side of the bridge for mttl
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

#ifndef COMMON_MTTL_BRIDGE_H
#define COMMON_MTTL_BRIDGE_H

#include <mt/mem_types.h>

#include "img_defs.h"
#include "img_types.h"
#include "mtgpu_error.h"

#include "devicemem_typedefs.h"
#include "mtgpu_tlcommon.h"

#define MTGPU_BRIDGE_MTTL_CMD_FIRST			0
#define MTGPU_BRIDGE_MTTL_TLOPENSTREAM			MTGPU_BRIDGE_MTTL_CMD_FIRST+0
#define MTGPU_BRIDGE_MTTL_TLCLOSESTREAM			MTGPU_BRIDGE_MTTL_CMD_FIRST+1
#define MTGPU_BRIDGE_MTTL_TLACQUIREDATA			MTGPU_BRIDGE_MTTL_CMD_FIRST+2
#define MTGPU_BRIDGE_MTTL_TLRELEASEDATA			MTGPU_BRIDGE_MTTL_CMD_FIRST+3
#define MTGPU_BRIDGE_MTTL_TLDISCOVERSTREAMS			MTGPU_BRIDGE_MTTL_CMD_FIRST+4
#define MTGPU_BRIDGE_MTTL_TLRESERVESTREAM			MTGPU_BRIDGE_MTTL_CMD_FIRST+5
#define MTGPU_BRIDGE_MTTL_TLCOMMITSTREAM			MTGPU_BRIDGE_MTTL_CMD_FIRST+6
#define MTGPU_BRIDGE_MTTL_TLWRITEDATA			MTGPU_BRIDGE_MTTL_CMD_FIRST+7
#define MTGPU_BRIDGE_MTTL_TLOPENSTREAM2			MTGPU_BRIDGE_MTTL_CMD_FIRST+8
#define MTGPU_BRIDGE_MTTL_TLACQUIREDATA2			MTGPU_BRIDGE_MTTL_CMD_FIRST+9
#define MTGPU_BRIDGE_MTTL_CMD_LAST				(MTGPU_BRIDGE_MTTL_CMD_FIRST+9)

/*******************************************
            TLOpenStream
 *******************************************/

/* Bridge in structure for TLOpenStream */
typedef struct MTGPU_BRIDGE_IN_TLOPENSTREAM_TAG
{
	const MT_CHAR *puiName;
	MT_UINT32 ui32Mode;
} __packed MTGPU_BRIDGE_IN_TLOPENSTREAM;

/* Bridge out structure for TLOpenStream */
typedef struct MTGPU_BRIDGE_OUT_TLOPENSTREAM_TAG
{
	MT_HANDLE hSD;
	MT_HANDLE hTLPMR;
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_TLOPENSTREAM;

/* Bridge out structure for TLOpenStream2 */
typedef struct MTGPU_BRIDGE_OUT_TLOPENSTREAM_TAG2
{
	MT_HANDLE hSD;
	MT_HANDLE hTLPMR;
	MT_BOOL bMultiReadersAllowed;
	MT_UINT32 ui32ReadOffset;
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_TLOPENSTREAM2;

/*******************************************
            TLCloseStream
 *******************************************/

/* Bridge in structure for TLCloseStream */
typedef struct MTGPU_BRIDGE_IN_TLCLOSESTREAM_TAG
{
	MT_HANDLE hSD;
} __packed MTGPU_BRIDGE_IN_TLCLOSESTREAM;

/* Bridge out structure for TLCloseStream */
typedef struct MTGPU_BRIDGE_OUT_TLCLOSESTREAM_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_TLCLOSESTREAM;

/*******************************************
            TLAcquireData
 *******************************************/

/* Bridge in structure for TLAcquireData */
typedef struct MTGPU_BRIDGE_IN_TLACQUIREDATA_TAG
{
	MT_HANDLE hSD;
} __packed MTGPU_BRIDGE_IN_TLACQUIREDATA;

/* Bridge in structure for TLAcquireData2 */
typedef struct MTGPU_BRIDGE_IN_TLACQUIREDATA_TAG2
{
	MT_HANDLE hSD;
	MT_UINT32 ui32ReadOffset;
} __packed MTGPU_BRIDGE_IN_TLACQUIREDATA2;

/* Bridge out structure for TLAcquireData */
typedef struct MTGPU_BRIDGE_OUT_TLACQUIREDATA_TAG
{
	MTGPU_ERROR eError;
	MT_UINT32 ui32ReadLen;
	MT_UINT32 ui32ReadOffset;
} __packed MTGPU_BRIDGE_OUT_TLACQUIREDATA;

/*******************************************
            TLReleaseData
 *******************************************/

/* Bridge in structure for TLReleaseData */
typedef struct MTGPU_BRIDGE_IN_TLRELEASEDATA_TAG
{
	MT_HANDLE hSD;
	MT_UINT32 ui32ReadLen;
	MT_UINT32 ui32ReadOffset;
} __packed MTGPU_BRIDGE_IN_TLRELEASEDATA;

/* Bridge out structure for TLReleaseData */
typedef struct MTGPU_BRIDGE_OUT_TLRELEASEDATA_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_TLRELEASEDATA;

/*******************************************
            TLDiscoverStreams
 *******************************************/

/* Bridge in structure for TLDiscoverStreams */
typedef struct MTGPU_BRIDGE_IN_TLDISCOVERSTREAMS_TAG
{
	const MT_CHAR *puiNamePattern;
	MT_CHAR *puiStreams;
	MT_UINT32 ui32Size;
} __packed MTGPU_BRIDGE_IN_TLDISCOVERSTREAMS;

/* Bridge out structure for TLDiscoverStreams */
typedef struct MTGPU_BRIDGE_OUT_TLDISCOVERSTREAMS_TAG
{
	MT_CHAR *puiStreams;
	MTGPU_ERROR eError;
	MT_UINT32 ui32NumFound;
} __packed MTGPU_BRIDGE_OUT_TLDISCOVERSTREAMS;

/*******************************************
            TLReserveStream
 *******************************************/

/* Bridge in structure for TLReserveStream */
typedef struct MTGPU_BRIDGE_IN_TLRESERVESTREAM_TAG
{
	MT_HANDLE hSD;
	MT_UINT32 ui32Size;
	MT_UINT32 ui32SizeMin;
} __packed MTGPU_BRIDGE_IN_TLRESERVESTREAM;

/* Bridge out structure for TLReserveStream */
typedef struct MTGPU_BRIDGE_OUT_TLRESERVESTREAM_TAG
{
	MTGPU_ERROR eError;
	MT_UINT32 ui32Available;
	MT_UINT32 ui32BufferOffset;
} __packed MTGPU_BRIDGE_OUT_TLRESERVESTREAM;

/*******************************************
            TLCommitStream
 *******************************************/

/* Bridge in structure for TLCommitStream */
typedef struct MTGPU_BRIDGE_IN_TLCOMMITSTREAM_TAG
{
	MT_HANDLE hSD;
	MT_UINT32 ui32ReqSize;
} __packed MTGPU_BRIDGE_IN_TLCOMMITSTREAM;

/* Bridge out structure for TLCommitStream */
typedef struct MTGPU_BRIDGE_OUT_TLCOMMITSTREAM_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_TLCOMMITSTREAM;

/*******************************************
            TLWriteData
 *******************************************/

/* Bridge in structure for TLWriteData */
typedef struct MTGPU_BRIDGE_IN_TLWRITEDATA_TAG
{
	MT_HANDLE hSD;
	MT_BYTE *pui8Data;
	MT_UINT32 ui32Size;
} __packed MTGPU_BRIDGE_IN_TLWRITEDATA;

/* Bridge out structure for TLWriteData */
typedef struct MTGPU_BRIDGE_OUT_TLWRITEDATA_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_TLWRITEDATA;

#endif /* COMMON_MTTL_BRIDGE_H */

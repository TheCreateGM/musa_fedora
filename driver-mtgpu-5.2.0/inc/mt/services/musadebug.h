/*************************************************************************/ /*!
@File
@Title          MUSA debug header file
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Header for the MUSA debugging functions
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

#if !defined(MUSADEBUG_H)
#define MUSADEBUG_H

#include "mtgpu_error.h"
#include "img_types.h"
#include "device.h"
#include "mt_notifier.h"
#include "mt.h"
#include "musadevice.h"

/**
 * Debug utility macro for printing FW IRQ count and Last sampled IRQ count in
 * LISR for each MUSA FW thread.
 * Macro takes pointer to MTGPU_MUSADEV_INFO as input.
 */
#define MUSADEBUG_PRINT_IRQ_COUNT(psMusaDevInfo) \
	do \
	{ \
		MT_UINT32 ui32TID; \
		for (ui32TID = 0; ui32TID < MUSAFW_THREAD_NUM; ui32TID++) \
		{ \
			MT_DPF((DBGPRIV_VERBOSE, \
					"MUSA FW thread %u: FW IRQ count = %u, Last sampled IRQ count in LISR = %u)", \
					ui32TID, \
					(psMusaDevInfo)->psMUSAFWIfFwOsData->aui32InterruptCount[ui32TID], \
					(psMusaDevInfo)->aui32SampleIRQCount[ui32TID])); \
		} \
	} while (0)

/*!
*******************************************************************************

 @Function	MUSADumpMUSARegisters

 @Description

 Dumps an extensive list of MUSA registers required for debugging

 @Input pfnDumpDebugPrintf  - Optional replacement print function
 @Input pvDumpDebugFile     - Optional file identifier to be passed to the
                              'printf' function if required
 @Input psDevInfo           - MUSA device info

 @Return MTGPU_ERROR         MTGPU_OK on success, error code otherwise

******************************************************************************/
MTGPU_ERROR MUSADumpMUSARegisters(DUMPDEBUG_PRINTF_FUNC *pfnDumpDebugPrintf,
				  void *pvDumpDebugFile,
				  MTGPU_MUSADEV_INFO *psDevInfo);

MTGPU_ERROR MUSADumpMUSARegistersForFec(DUMPDEBUG_PRINTF_FUNC *pfnDumpDebugPrintf,
					void *pvDumpDebugFile,
					MTGPU_MUSADEV_INFO *psDevInfo);
/*!
*******************************************************************************

 @Function	MUSADumpFirmwareTrace

 @Description Dumps the decoded version of the firmware trace buffer.

 Dump useful debugging info

 @Input pfnDumpDebugPrintf  - Optional replacement print function
 @Input pvDumpDebugFile     - Optional file identifier to be passed to the
                              'printf' function if required
 @Input psDevInfo           - MUSA device info

 @Return   void

******************************************************************************/
void MUSADumpFirmwareTrace(DUMPDEBUG_PRINTF_FUNC *pfnDumpDebugPrintf,
				void *pvDumpDebugFile,
				MTGPU_MUSADEV_INFO  *psDevInfo);

/*!
*******************************************************************************

 @Function	MUSAStreamingDumpFirmwareTrace

 @Description Stream dumps the decoded version of the firmware trace buffer.

 Dump useful debugging info

 @Input pszBuffer		    - Buffer will copied to user
 @Input ui64Count  			- Maximum count of buffer size
 @Input pui64Pos            - File read offset
 @Input psDevInfo           - Device INFO

 @Return   The number of characters dumped to buffer

******************************************************************************/
MT_INT64 MUSAStreamingDumpFirmwareTrace(MT_CHAR *pszBuffer, MT_UINT64 ui64Count,
                                        MT_UINT64 *pui64Pos, MTGPU_MUSADEV_INFO *psDevInfo);

/*!
*******************************************************************************

 @Function MUSADebugInit

 @Description

 Setup debug requests, calls into MTGPURegisterDeviceDbgRequestNotify

 @Input          psDevInfo            MUSA device info
 @Return         MTGPU_ERROR         MTGPU_OK on success otherwise an error

******************************************************************************/
MTGPU_ERROR MUSADebugInit(MTGPU_MUSADEV_INFO *psDevInfo);

/*!
*******************************************************************************

 @Function MUSADebugDeinit

 @Description

 Remove debug requests, calls into MTGPUUnregisterDeviceDbgRequestNotify

 @Output         phNotify             Points to debug notifier handle
 @Return         MTGPU_ERROR         MTGPU_OK on success otherwise an error

******************************************************************************/
MTGPU_ERROR MUSADebugDeinit(MTGPU_MUSADEV_INFO *psDevInfo);

#if (MUSA_NUM_OS_SUPPORTED > 1)
void _MUSADumpMUSAMMUFaultStatus(DUMPDEBUG_PRINTF_FUNC *pfnDumpDebugPrintf,
			       void *pvDumpDebugFile,
			       MTGPU_MUSADEV_INFO *psDevInfo,
			       const MT_UINT64 aui64MMUStatus[],
			       const MT_PCHAR pszMcuOrCore,
			       const MT_CHAR *pszIndent);
#endif

void MTDumpMUSAMMUFaultStatus(void *pvDumpDebugFile,
			     MTGPU_MUSADEV_INFO *psDevInfo,
			     const MT_UINT64 aui64MMUStatus1,
			     const MT_UINT64 aui64MMUStatus2,
			     const MT_PCHAR pszMcuOrCore,
			     const MT_CHAR *pszIndent,
			     PAGE_FAULT_INFO *psPFInfo);
#endif /* MUSADEBUG_H */

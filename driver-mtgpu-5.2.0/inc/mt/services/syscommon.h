/**************************************************************************/ /*!
@File
@Title          Common System APIs and structures
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    This header provides common system-specific declarations and
                macros that are supported by all systems
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

#if !defined(SYSCOMMON_H)
#define SYSCOMMON_H

#include "img_types.h"
#include "mt_notifier.h"
#include "mt_device.h"
#include "mtgpu_error.h"

/*************************************************************************/ /*!
@Description    Pointer to a Low-level Interrupt Service Routine (LISR).
@Input  pvData  Private data provided to the LISR.
@Return         True if interrupt handled, false otherwise.
*/ /**************************************************************************/
typedef MT_BOOL (*PFN_LISR)(void *pvData);

/**************************************************************************/ /*!
@Function       SysDevInit
@Description    System specific device initialisation function.
@Input          pvOSDevice          pointer to the OS device reference
@Input          ppsDevConfig        returned device configuration info
@Return         MTGPU_OK on success, a failure code otherwise.
*/ /***************************************************************************/
MTGPU_ERROR SysDevInit(void *pvOSDevice, MTGPU_DEVICE_CONFIG **ppsDevConfig);

/**************************************************************************/ /*!
@Function       SysDevDeInit
@Description    System specific device deinitialisation function.
@Input          psDevConfig        device configuration info of the device to be
                                   deinitialised
@Return         None.
*/ /***************************************************************************/
void SysDevDeInit(MTGPU_DEVICE_CONFIG *psDevConfig);

/**************************************************************************/ /*!
@Function       SysDebugInfo
@Description    Dump system specific device debug information.
@Input          psDevConfig         pointer to device configuration info
@Input          pfnDumpDebugPrintf  the 'printf' function to be called to
                                    display the debug info
@Input          pvDumpDebugFile     optional file identifier to be passed to
                                    the 'printf' function if required
@Return         MTGPU_OK on success, a failure code otherwise.
*/ /***************************************************************************/
MTGPU_ERROR SysDebugInfo(MTGPU_DEVICE_CONFIG *psDevConfig,
				DUMPDEBUG_PRINTF_FUNC *pfnDumpDebugPrintf,
				void *pvDumpDebugFile);

/**************************************************************************/ /*!
@Function       SysInstallDeviceLISR
@Description    Installs the system Low-level Interrupt Service Routine (LISR)
                which handles low-level processing of interrupts from the device
                (GPU).
                The LISR will be invoked when the device raises an interrupt. An
                LISR may not be descheduled, so code which needs to do so should
                be placed in an MISR.
                The installed LISR will schedule any MISRs once it has completed
                its interrupt processing, by calling OSScheduleMISR().
@Input          hSysData      pointer to the system data of the device
@Input          ui32IRQ       the IRQ on which the LISR is to be installed
@Input          pszName       name of the module installing the LISR
@Input          pfnLISR       pointer to the function to be installed as the
                              LISR
@Input          pvData        private data provided to the LISR
@Output         phLISRData    handle to the installed LISR (to be used for a
                              subsequent uninstall)
@Return         MTGPU_OK on success, a failure code otherwise.
*/ /***************************************************************************/
MTGPU_ERROR SysInstallDeviceLISR(MT_HANDLE hSysData,
								  MT_UINT32 ui32IRQ,
								  const MT_CHAR *pszName,
								  PFN_LISR pfnLISR,
								  void *pvData,
								  MT_HANDLE *phLISRData);

/**************************************************************************/ /*!
@Function       SysUninstallDeviceLISR
@Description    Uninstalls the system Low-level Interrupt Service Routine (LISR)
                which handles low-level processing of interrupts from the device
                (GPU).
@Input          hLISRData     handle of the LISR to be uninstalled
@Return         MTGPU_OK on success, a failure code otherwise.
*/ /***************************************************************************/
MTGPU_ERROR SysUninstallDeviceLISR(MT_HANDLE hLISRData);

/**************************************************************************/ /*!
@Function       SysMUSAErrorNotify
@Description    Error reporting callback function, registered as the
                pfnSysDevErrorNotify member of the MTGPU_DEVICE_CONFIG
                struct. System layer will be notified of device errors and
                resets via this callback.
                NB. implementers should ensure that the minimal amount of
                work is done in this callback function, as it will be
                executed in the main MUSA MISR. (e.g. any blocking or lengthy
                work should be performed by a worker queue/thread instead).
@Input          hSysData      pointer to the system data of the device
@Input          psDeviceNode  pointer to the info of device
@Input          pszFuncName   the name of caller
@Output         psErrorData   structure containing details of the reported error
@Return         None.
*/ /***************************************************************************/
void SysMUSAErrorNotify(MT_HANDLE hSysData,
		       MTGPU_ROBUSTNESS_NOTIFY_DATA *psErrorData,
		       MTGPU_DEVICE_NODE *psDeviceNode,
		       const MT_CHAR *pszFuncName);

/*************************************************************************/ /*!
@Description    Trye to conver GPA to IOVA.
@Input          psDevConfig  pointer to device configuration info.
@Return         True if host enable IOMMU and convert successfullly.
                Otherwise false.
*/ /**************************************************************************/
bool TryToConvertGPAToIOVA(MTGPU_DEVICE_CONFIG *psDevConfig,
			   MT_UINT64 ui64GPA, MT_UINT64 *ui64HPA);

/**************************************************************************/ /*!
@Function	GuestDevicePAddrToHostDevicePAddr
@Description	Guest device physical address is a virtual device address.
		Translate it into real device physical address on host to
		fill in the page table.
@Input		psDevConfig   pointer to device configuration info
@Input		ui64Gdpa      guest device physical address
@Output		Real device physical address on host
@Return		None.
*/ /***************************************************************************/
MT_UINT64 GuestDevicePAddrToHostDevicePAddr(MTGPU_DEVICE_CONFIG *psDevConfig,
					     const MT_UINT64 ui64Gdpa);

#endif /* !defined(SYSCOMMON_H) */

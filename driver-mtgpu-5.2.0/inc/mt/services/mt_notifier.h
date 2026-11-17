/*************************************************************************/ /*!
@File
@Title          PowerVR notifier interface
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
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

#if !defined(MT_NOTIFIER_H)
#define MT_NOTIFIER_H

#include "img_types.h"
#include "mt_debug.h"


/*************************************************************************/ /*!
Command Complete Notifier Interface
*/ /**************************************************************************/

typedef MT_HANDLE MTGPU_CMDCOMP_HANDLE;
#ifndef CMDCOMPNOTIFY_PFN
typedef void (*PFN_CMDCOMP_NOTIFY)(MTGPU_CMDCOMP_HANDLE hCmdCompHandle);
#define CMDCOMPNOTIFY_PFN
#endif

/*************************************************************************/ /*!
@Function       MTGPUCmdCompleteInit
@Description    Performs initialisation of the command complete notifier
                interface.
@Return         MTGPU_ERROR         MTGPU_OK on success otherwise an error
*/ /**************************************************************************/
MTGPU_ERROR
MTGPUCmdCompleteInit(void);

/*************************************************************************/ /*!
@Function       MTGPUCmdCompleteDeinit
@Description    Performs cleanup for the command complete notifier interface.
@Return         MTGPU_ERROR         MTGPU_OK on success otherwise an error
*/ /**************************************************************************/
void
MTGPUCmdCompleteDeinit(void);

/*************************************************************************/ /*!
@Function       MTGPURegisterCmdCompleteNotify
@Description    Register a callback function that is called when some device
                finishes some work, which is signalled via a call to
                MTGPUCheckStatus.
@Output         phNotify             On success, points to command complete
                                     notifier handle
@Input          pfnCmdCompleteNotify Function callback
@Input          hPrivData            Data to be passed back to the caller via
                                     the callback function
@Return         MTGPU_ERROR         MTGPU_OK on success otherwise an error
*/ /**************************************************************************/
MTGPU_ERROR
MTGPURegisterCmdCompleteNotify(MT_HANDLE *phNotify,
                                PFN_CMDCOMP_NOTIFY pfnCmdCompleteNotify,
                                MTGPU_CMDCOMP_HANDLE hPrivData);

/*************************************************************************/ /*!
@Function       MTGPUUnregisterCmdCompleteNotify
@Description    Unregister a previously registered callback function.
@Input          hNotify              Command complete notifier handle
@Return         MTGPU_ERROR         MTGPU_OK on success otherwise an error
*/ /**************************************************************************/
MTGPU_ERROR
MTGPUUnregisterCmdCompleteNotify(MT_HANDLE hNotify);

/*************************************************************************/ /*!
@Function       MTGPUCheckStatus
@Description    Calls MTGPUNotifyCommandCompletion() to notify registered
                command complete handlers of work completion and then calls
                MTGPUSignalGlobalEO() to signal the global event object.
@Input          hCmdCompCallerHandle Used to prevent a handler from being
                                     notified. A NULL value results in all
                                     handlers being notified.
*/ /**************************************************************************/
void
MTGPUCheckStatus(MTGPU_CMDCOMP_HANDLE hCmdCompCallerHandle);

/*************************************************************************/ /*!
@Function       MTGPUNotifyCommandCompletion
@Description    Notify any registered command complete handlers that some work
                has been finished (unless hCmdCompCallerHandle matches a
                handler's hPrivData).
@Input          hCmdCompCallerHandle Used to prevent a handler from being
                                     notified. A NULL value results in all
                                     handlers being notified.
*/ /**************************************************************************/
void
MTGPUNotifyCommandCompletion(MTGPU_CMDCOMP_HANDLE hCmdCompCallerHandle);

/*************************************************************************/ /*!
@Function       MTGPUSignalGlobalEO
@Description    Signals the global event object.
*/ /**************************************************************************/
void
MTGPUSignalGlobalEO(void);


/*************************************************************************/ /*!
Debug Notifier Interface
*/ /**************************************************************************/

#define DEBUG_REQUEST_DC                0
#define DEBUG_REQUEST_SYNCTRACKING      1
#define DEBUG_REQUEST_SRV               2
#define DEBUG_REQUEST_SYS               3
#define DEBUG_REQUEST_MUSA               4
#define DEBUG_REQUEST_ANDROIDSYNC       5
#define DEBUG_REQUEST_LINUXFENCE        6
#define DEBUG_REQUEST_SYNCCHECKPOINT    7
#define DEBUG_REQUEST_HTB               8
#define DEBUG_REQUEST_APPHINT           9
#define DEBUG_REQUEST_FALLBACKSYNC      10

#define DEBUG_REQUEST_VERBOSITY_LOW     0
#define DEBUG_REQUEST_VERBOSITY_MEDIUM  1
#define DEBUG_REQUEST_VERBOSITY_HIGH    2
#define DEBUG_REQUEST_VERBOSITY_MAX     DEBUG_REQUEST_VERBOSITY_HIGH

#define DD_VERB_LVL_ENABLED(_verbLvl, _verbLvlChk) ((_verbLvl) >= (_verbLvlChk))

/*
 * Macro used within debug dump functions to send output either to MT_LOG or
 * a custom function. The custom function should be stored as a function
 * pointer in a local variable called 'pfnDumpDebugPrintf'. 'pvDumpDebugFile'
 * is also required as a local variable to serve as a file identifier for the
 * printf function if required.
 */
#define MT_DUMPDEBUG_LOG(...)                                \
	do                                                        \
	{                                                         \
		if (pfnDumpDebugPrintf)                               \
			pfnDumpDebugPrintf(pvDumpDebugFile, __VA_ARGS__); \
		else                                                  \
			MT_LOG((__VA_ARGS__));                           \
	} while (0)

struct _MTGPU_DEVICE_NODE_;

typedef MT_HANDLE MTGPU_DBGREQ_HANDLE;
#ifndef DBGNOTIFY_PFNS
typedef void (DUMPDEBUG_PRINTF_FUNC)(void *pvDumpDebugFile,
				     const MT_CHAR *pszFormat, ...);
typedef void (*PFN_DBGREQ_NOTIFY)(MTGPU_DBGREQ_HANDLE hDebugRequestHandle,
					MT_UINT32 ui32VerbLevel,
					DUMPDEBUG_PRINTF_FUNC *pfnDumpDebugPrintf,
					void *pvDumpDebugFile);
#define DBGNOTIFY_PFNS
#endif

/*************************************************************************/ /*!
@Function       MTGPURegisterDeviceDbgTable
@Description    Registers a debug requester table for the given device. The
                order in which the debug requester IDs appear in the
                table determine the order in which a set of notifier callbacks
                will be called. In other words, the requester ID that appears
                first will have all of its associated debug notifier callbacks
                called first. This will then be followed by all the callbacks
                associated with the next requester ID in the table and so on.
                The order table is handled internally.
@Input          psDevNode     Device node to register requester table with
@Return         MTGPU_ERROR  MTGPU_OK on success otherwise an error
*/ /**************************************************************************/
MTGPU_ERROR
MTGPURegisterDeviceDbgTable(struct _MTGPU_DEVICE_NODE_ *psDevNode);

/*************************************************************************/ /*!
@Function       MTGPURegisterDriverDbgTable
@Description    Registers a debug requester table for the driver. The
                order in which the debug requester IDs appear in the
                table determine the order in which a set of notifier callbacks
                will be called. In other words, the requester ID that appears
                first will have all of its associated debug notifier callbacks
                called first. This will then be followed by all the callbacks
                associated with the next requester ID in the table and so on.
                The order table is handled internally.
@Return         MTGPU_ERROR  MTGPU_OK on success otherwise an error
*/ /**************************************************************************/
MTGPU_ERROR
MTGPURegisterDriverDbgTable(void);

/*************************************************************************/ /*!
@Function       MTGPUUnregisterDeviceDbgTable
@Description    Unregisters a debug requester table.
@Input          psDevNode     Device node for which the requester table should
                              be unregistered
@Return         void
*/ /**************************************************************************/
void
MTGPUUnregisterDeviceDbgTable(struct _MTGPU_DEVICE_NODE_ *psDevNode);

/*************************************************************************/ /*!
@Function       MTGPUUnregisterDriverDbgTable
@Description    Unregisters the driver debug requester table.
@Return         void
*/ /**************************************************************************/
void
MTGPUUnregisterDriverDbgTable(void);

/*************************************************************************/ /*!
@Function       MTGPURegisterDeviceDbgRequestNotify
@Description    Register a callback function on a given device that is called
                when a debug request is made via a call MTGPUDebugRequest.
                There are a number of verbosity levels ranging from
                DEBUG_REQUEST_VERBOSITY_LOW up to
                DEBUG_REQUEST_VERBOSITY_MAX. The callback will be called once
                for each level up to the highest level specified to
                MTGPUDebugRequest.
@Output         phNotify             Points to debug notifier handle on success
@Input          psDevNode            Device node for which the debug callback
                                     should be registered
@Input          pfnDbgRequestNotify  Function callback
@Input          ui32RequesterID      Requester ID. This is used to determine
                                     the order in which callbacks are called
@Input          hDbgReqeustHandle    Data to be passed back to the caller via
                                     the callback function
@Return         MTGPU_ERROR         MTGPU_OK on success otherwise an error
*/ /**************************************************************************/
MTGPU_ERROR
MTGPURegisterDeviceDbgRequestNotify(MT_HANDLE *phNotify,
                                     struct _MTGPU_DEVICE_NODE_ *psDevNode,
                                     PFN_DBGREQ_NOTIFY pfnDbgRequestNotify,
                                     MT_UINT32 ui32RequesterID,
                                     MTGPU_DBGREQ_HANDLE hDbgReqeustHandle);

/*************************************************************************/ /*!
@Function       MTGPURegisterDriverDbgRequestNotify
@Description    Register a callback function that is called when a debug request
                is made via a call MTGPUDebugRequest. There are a number of
                verbosity levels ranging from DEBUG_REQUEST_VERBOSITY_LOW up to
                DEBUG_REQUEST_VERBOSITY_MAX. The callback will be called once
                for each level up to the highest level specified to
                MTGPUDebugRequest.
@Output         phNotify             Points to debug notifier handle on success
@Input          pfnDbgRequestNotify  Function callback
@Input          ui32RequesterID      Requester ID. This is used to determine
                                     the order in which callbacks are called
@Input          hDbgReqeustHandle    Data to be passed back to the caller via
                                     the callback function
@Return         MTGPU_ERROR         MTGPU_OK on success otherwise an error
*/ /**************************************************************************/
MTGPU_ERROR
MTGPURegisterDriverDbgRequestNotify(MT_HANDLE *phNotify,
									 PFN_DBGREQ_NOTIFY pfnDbgRequestNotify,
									 MT_UINT32 ui32RequesterID,
									 MTGPU_DBGREQ_HANDLE hDbgRequestHandle);

/*************************************************************************/ /*!
@Function       MTGPUUnregisterDeviceDbgRequestNotify
@Description    Unregister a previously registered (device context) callback
                function.
@Input          hNotify              Debug notifier handle.
@Return         MTGPU_ERROR         MTGPU_OK on success otherwise an error
*/ /**************************************************************************/
MTGPU_ERROR
MTGPUUnregisterDeviceDbgRequestNotify(MT_HANDLE hNotify);

/*************************************************************************/ /*!
@Function       MTGPUUnregisterDriverDbgRequestNotify
@Description    Unregister a previously registered (driver context) callback
                function.
@Input          hNotify              Debug notifier handle.
@Return         MTGPU_ERROR         MTGPU_OK on success otherwise an error
*/ /**************************************************************************/
MTGPU_ERROR
MTGPUUnregisterDriverDbgRequestNotify(MT_HANDLE hNotify);

/*************************************************************************/ /*!
@Function       MTGPUDebugRequest
@Description    Notify any registered debug request handlers that a debug
                request has been made and at what level.
@Input          psDevNode           Device node for which the debug request
                                    has been made
@Input          ui32VerbLevel       The maximum verbosity level to dump
@Input          pfnDumpDebugPrintf  Used to specify the print function that
                                    should be used to dump any debug
                                    information. If this argument is NULL then
                                    MT_LOG() will be used as the default
                                    print function.
@Input          pvDumpDebugFile     Optional file identifier to be passed to
                                    the print function if required.
@Return         void
*/ /**************************************************************************/
void
MTGPUDebugRequest(struct _MTGPU_DEVICE_NODE_ *psDevNode,
                   MT_UINT32 ui32VerbLevel,
                   DUMPDEBUG_PRINTF_FUNC *pfnDumpDebugPrintf,
                   void *pvDumpDebugFile);

#endif /* !defined(MT_NOTIFIER_H) */

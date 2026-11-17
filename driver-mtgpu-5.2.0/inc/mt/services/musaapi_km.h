/*************************************************************************/ /*!
@File
@Title          MUSA API Header kernel mode
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Exported MUSA API details
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

#ifndef MUSAAPI_KM_H
#define MUSAAPI_KM_H

#include "musa_hwperf.h"
#include "device.h"

/******************************************************************************
 * MUSA HW Performance Profiling Control API(s)
 *****************************************************************************/

/*! HWPerf device identification structure */
typedef struct _MUSA_HWPERF_DEVICE_
{
	MT_CHAR pszName[20];	/*!< Helps identify this device uniquely */
	MT_HANDLE hDevData;	/*!< Handle for the server */

	struct _MUSA_HWPERF_DEVICE_ *psNext;  /*!< Next device if any */
} MUSA_HWPERF_DEVICE;

/*! HWPerf connection structure */
typedef struct
{
	MUSA_HWPERF_DEVICE *psHWPerfDevList;  /*!< pointer to list of devices */
} MUSA_HWPERF_CONNECTION;

/*************************************************************************/ /*!
@Function       MUSAHWPerfLazyConnect
@Description    Obtain a HWPerf connection object to the MUSA device(s). The
                connections to devices are not actually opened until
                HWPerfOpen() is called.

@Output         ppsHWPerfConnection Address of a HWPerf connection object
@Return         MTGPU_ERROR        System error code
*/ /**************************************************************************/
MTGPU_ERROR MUSAHWPerfLazyConnect(MUSA_HWPERF_CONNECTION** ppsHWPerfConnection);


/*************************************************************************/ /*!
@Function       MUSAHWPerfOpen
@Description    Opens connection(s) to the MUSA device(s). Valid handle to the
                connection object has to be provided which means the this
                function needs to be preceded by the call to
                MUSAHWPerfLazyConnect() function.

@Input          psHWPerfConnection HWPerf connection object
@Return         MTGPU_ERROR       System error code
*/ /**************************************************************************/
MTGPU_ERROR MUSAHWPerfOpen(MUSA_HWPERF_CONNECTION* psHWPerfConnection);


/*************************************************************************/ /*!
@Function       MUSAHWPerfConnect
@Description    Obtain a connection object to the MUSA HWPerf module. Allocated
                connection object(s) reference opened connection(s). Calling
                this function is an equivalent of calling MUSAHWPerfLazyConnect
                and MUSAHWPerfOpen. This connect should be used when the caller
                will be retrieving event data.

@Output         ppsHWPerfConnection Address of HWPerf connection object
@Return         MTGPU_ERROR        System error code
*/ /**************************************************************************/
MTGPU_ERROR MUSAHWPerfConnect(MUSA_HWPERF_CONNECTION** ppsHWPerfConnection);


/*************************************************************************/ /*!
@Function       MUSAHWPerfFreeConnection
@Description    Frees the HWPerf connection object

@Input          psHWPerfConnection Pointer to connection object as returned
                                    from MUSAHWPerfLazyConnect()
@Return         MTGPU_ERROR       System error code
*/ /**************************************************************************/
MTGPU_ERROR MUSAHWPerfFreeConnection(MUSA_HWPERF_CONNECTION** psHWPerfConnection);


/*************************************************************************/ /*!
@Function       MUSAHWPerfClose
@Description    Closes all the opened connection(s) to MUSA device(s)

@Input          psHWPerfConnection Pointer to HWPerf connection object as
                                    returned from MUSAHWPerfConnect() or
                                    MUSAHWPerfOpen()
@Return         MTGPU_ERROR       System error code
*/ /**************************************************************************/
MTGPU_ERROR MUSAHWPerfClose(MUSA_HWPERF_CONNECTION *psHWPerfConnection);


/*************************************************************************/ /*!
@Function       MUSAHWPerfDisconnect
@Description    Disconnect from the MUSA device

@Input          ppsHWPerfConnection Pointer to HWPerf connection object as
                                     returned from MUSAHWPerfConnect() or
                                     MUSAHWPerfOpen(). Calling this function is
                                     an equivalent of calling MUSAHWPerfClose()
                                     and MUSAHWPerfFreeConnection().
@Return         MTGPU_ERROR        System error code
*/ /**************************************************************************/
MTGPU_ERROR MUSAHWPerfDisconnect(MUSA_HWPERF_CONNECTION** ppsHWPerfConnection);


/*************************************************************************/ /*!
@Function       MUSAHWPerfControl
@Description    Enable or disable the generation of MUSA HWPerf event packets.
                 See MUSACtrlHWPerf().

@Input          psHWPerfConnection Pointer to HWPerf connection object
@Input          eStreamId          ID of the HWPerf stream
@Input          bToggle            Switch to toggle or apply mask.
@Input          ui64Mask           Mask of events to control.
@Return         MTGPU_ERROR       System error code
*/ /**************************************************************************/
MTGPU_ERROR MUSAHWPerfControl(
		MUSA_HWPERF_CONNECTION *psHWPerfConnection,
		MUSA_HWPERF_STREAM_ID eStreamId,
		MT_BOOL             bToggle,
		MT_UINT64           ui64Mask);


/*************************************************************************/ /*!
@Function       MUSAHWPerfGetFilter
@Description    Reads HWPerf stream filter where stream is identified by the
                 given stream ID.

@Input          hDevData     Handle to connection/device object
@Input          eStreamId    ID of the HWPerf stream
@Output         ui64Filter   HWPerf filter value
@Return         MTGPU_ERROR System error code
*/ /**************************************************************************/
MTGPU_ERROR MUSAHWPerfGetFilter(
		MT_HANDLE  hDevData,
		MUSA_HWPERF_STREAM_ID eStreamId,
		MT_UINT64 *ui64Filter
);

/*************************************************************************/ /*!
@Function	MUSAHWPerfGetSessionFilter
@Description	Reads HWPerf stream filter of this session where stream is
                 identified by the given stream ID.

@Input		hDevData		Handle to device object
@Input		eStreamId		ID of the HWPerf stream
@Output		ui64Filter		HWPerf filter value
@Return		MTGPU_ERROR		System error code
*/ /**************************************************************************/
MTGPU_ERROR MUSAHWPerfGetSessionFilter(MT_HANDLE hDevData,
				       MUSA_HWPERF_STREAM_ID eStreamId,
				       MT_UINT64 *ui64Filter
);

/*************************************************************************/ /*!
@Function       MUSAHWPerfConfigureCounters
@Description    Enable and configure the performance counter block for one or
                 more device layout modules.
                 See MUSAConfigHWPerfCounters().

@Input          psHWPerfConnection Pointer to HWPerf connection object
@Input          ui32CtrlWord       One of <tt>MUSA_HWPERF_CTRL_NOP</tt>,
                                    <tt>MUSA_HWPERF_CTRL_GEOM_FULLrange</tt>,
                                    <tt>MUSA_HWPERF_CTRL_COMP_FULLRANGE</tt>,
                                    <tt>MUSA_HWPERF_CTRL_TRANSFER_FULLRANGE</tt>
@Input          ui32NumBlocks      Number of elements in the array
@Input          asBlockConfigs     Address of the array of configuration blocks
@Return         MTGPU_ERROR       System error code
*/ /**************************************************************************/
MTGPU_ERROR MUSAHWPerfConfigureCounters(
		MUSA_HWPERF_CONNECTION *psHWPerfConnection,
		MT_UINT32                 ui32CtrlWord,
		MT_UINT32                 ui32NumBlocks,
		MUSA_HWPERF_CONFIG_CNTBLK*  asBlockConfigs);


/*************************************************************************/ /*!
@Function       MUSAHWPerfDisableCounters
@Description    Disable the performance counter block for one or more device
                 layout modules.

@Input          psHWPerfConnection Pointer to HWPerf connection object
@Input          ui32NumBlocks      Number of elements in the array
@Input          aeBlockIDs         An array of words with values taken from
                                    the MUSA_HWPERF_CNTBLK_ID enumeration.
@Return         MTGPU_ERROR       System error code
*/ /**************************************************************************/
MTGPU_ERROR MUSAHWPerfDisableCounters(
		MUSA_HWPERF_CONNECTION *psHWPerfConnection,
		MT_UINT32   ui32NumBlocks,
		MT_UINT16*  aeBlockIDs);

/*************************************************************************/ /*!
@Function       MUSAHWPerfEnableCounters
@Description    Enable the performance counter block for one or more device
                 layout modules.

@Input          psHWPerfConnection Pointer to HWPerf connection object
@Input          ui32NumBlocks      Number of elements in the array
@Input          aeBlockIDs         An array of words with values taken from the
                                    <tt>MUSA_HWPERF_CNTBLK_ID</tt> enumeration.
@Return         MTGPU_ERROR       System error code
*/ /**************************************************************************/
MTGPU_ERROR MUSAHWPerfEnableCounters(
		MUSA_HWPERF_CONNECTION *psHWPerfConnection,
		MT_UINT32   ui32NumBlocks,
		MT_UINT16*  aeBlockIDs);

/******************************************************************************
 * MUSA HW Performance Profiling Retrieval API(s)
 *
 * The client must ensure their use of this acquire/release API for a single
 * connection/stream must not be shared with multiple execution contexts e.g.
 * between a kernel thread and an ISR handler. It is the client's
 * responsibility to ensure this API is not interrupted by a high priority
 * thread/ISR
 *****************************************************************************/

/*************************************************************************/ /*!
@Function       MUSAHWPerfAcquireEvents
@Description    When there is data available to read this call returns with OK
                 and the address and length of the data buffer the client can
                 safely read. This buffer may contain one or more event packets.
                 When there is no data to read, this call returns with OK and
                 sets *puiBufLen to 0 on exit.
                 Clients must pair this call with a MUSAHWPerfReleaseEvents()
                 call.
                 Data returned in ppBuf will be in the form of a sequence of
                 HWPerf packets which should be traversed using the pointers,
                 structures and macros provided in musa_hwperf.h

@Input          hDevData     Handle to connection/device object
@Input          eStreamId    ID of the HWPerf stream
@Output         ppBuf        Address of a pointer to a byte buffer. On exit it
                              contains the address of buffer to read from
@Output         pui32BufLen  Pointer to an integer. On exit it is the size of
                              the data to read from the buffer
@Return         MTGPU_ERROR System error code
*/ /**************************************************************************/
MTGPU_ERROR MUSAHWPerfAcquireEvents(
		MT_HANDLE  hDevData,
		MUSA_HWPERF_STREAM_ID eStreamId,
		MT_PBYTE*  ppBuf,
		MT_UINT32* pui32BufLen);


/*************************************************************************/ /*!
@Function       MUSAHWPerfReleaseEvents
@Description    Called after client has read the event data out of the buffer
                 retrieved from the Acquire Events call to release resources.
@Input          hDevData     Handle to connection/device object
@Input          eStreamId    ID of the HWPerf stream
@Return         MTGPU_ERROR System error code
*/ /**************************************************************************/
MT_INTERNAL
MTGPU_ERROR MUSAHWPerfReleaseEvents(
		MT_HANDLE hDevData,
		MUSA_HWPERF_STREAM_ID eStreamId);

/*************************************************************************/ /*!
@Function       MUSAHWPerfIsReleased
@Description    HWPerf stream if can be released.
@Input          hDevData     Handle to connection/device object
@Input          eStreamId    ID of the HWPerf stream
@Return         MT_BOOL     HWPerf stream if can be released
*/ /**************************************************************************/
MT_BOOL MUSAHWPerfIsReleased(
		MT_HANDLE  hDevData,
		MUSA_HWPERF_STREAM_ID eStreamId);

/*************************************************************************/ /*!
@Function       MUSAHWPerfConvertCRTimeStamp
@Description    Converts the timestamp given by FW events to the common OS
                 timestamp. The first three inputs are obtained via a CLK_SYNC
                 event, ui64CRTimeStamp is the CR timestamp from the FW event
                 to be converted.
@Input          ui32ClkSpeed        Clock speed given by sync event
@Input          ui64CorrCRTimeStamp CR Timestamp given by sync event
@Input          ui64CorrOSTimeStamp Correlating OS Timestamp given by sync
                                     event
@Input          ui64CRTimeStamp     CR Timestamp to convert
@Return         MT_UINT64          Calculated OS Timestamp
*/ /**************************************************************************/
MT_UINT64 MUSAHWPerfConvertCRTimeStamp(
		MT_UINT32 ui32ClkSpeed,
		MT_UINT64 ui64CorrCRTimeStamp,
		MT_UINT64 ui64CorrOSTimeStamp,
		MT_UINT64 ui64CRTimeStamp);

/*************************************************************************/ /*!
@Function       MUSAHWPerfGetDeviceNode
@Description    Get the device node pointer.
@Input          hDevData     Handle to connection/device object
@Return         MTGPU_DEVICE_NODE*  Device node pointer
*/ /**************************************************************************/
MTGPU_DEVICE_NODE *MUSAHWPerfGetDeviceNode(MT_HANDLE  hDevData);

#endif /* MUSAAPI_KM_H */

/******************************************************************************
 End of file (musaapi_km.h)
******************************************************************************/

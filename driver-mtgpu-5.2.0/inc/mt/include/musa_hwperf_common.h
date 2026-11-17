/*************************************************************************/ /*!
@File
@Title          MUSA HWPerf and Debug Types and Defines Header
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Common data types definitions for hardware performance API
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
#ifndef MUSA_HWPERF_COMMON_H_
#define MUSA_HWPERF_COMMON_H_

#if defined(__cplusplus)
extern "C" {
#endif

/* These structures are used on both GPU and CPU and must be a size that is a
 * multiple of 64 bits, 8 bytes to allow the FW to write 8 byte quantities at
 * 8 byte aligned addresses. MUSA_FW_STRUCT_*_ASSERT() is used to check this.
 */

/******************************************************************************
 * Includes and Defines
 *****************************************************************************/

#include "img_types.h"
#include "img_defs.h"

#include "musa_common_asserts.h"
#include "mtgpu_tlcommon.h"


/******************************************************************************
 * Packet Event Type Enumerations
 *****************************************************************************/

/*! Type used to encode the event that generated the packet.
 * NOTE: When this type is updated the corresponding hwperfbin2json tool
 * source needs to be updated as well. The MUSA_HWPERF_EVENT_MASK_* macros will
 * also need updating when adding new types.
 *
 * @par
 * The event type values are incrementing integers for use as a shift ordinal
 * in the event filtering process at the point events are generated.
 * This scheme thus implies a limit of 63 event types.
 */

typedef MT_UINT32 MUSA_HWPERF_EVENT_TYPE;

#define MUSA_HWPERF_INVALID				0x00U /*!< Invalid. Reserved value. */

/*! FW types 0x01..0x06 */
#define MUSA_HWPERF_FW_EVENT_RANGE_FIRST_TYPE	0x01U

#define MUSA_HWPERF_FW_BGSTART			0x01U /*!< Background task processing start */
#define MUSA_HWPERF_FW_BGEND				0x02U /*!< Background task end */
#define MUSA_HWPERF_FW_IRQSTART			0x03U /*!< IRQ task processing start */

#define MUSA_HWPERF_FW_IRQEND			0x04U /*!< IRQ task end */
#define MUSA_HWPERF_FW_DBGSTART			0x05U /*!< Debug event start */
#define MUSA_HWPERF_FW_DBGEND			0x06U /*!< Debug event end */

#define MUSA_HWPERF_FW_EVENT_RANGE_LAST_TYPE		0x06U

/*! HW types 0x07..0x19 */
#define MUSA_HWPERF_HW_EVENT_RANGE0_FIRST_TYPE	0x07U

#define MUSA_HWPERF_HW_PMOOM_TAPAUSE		0x07U /*!< TA Pause at PM Out of Memory */

#define MUSA_HWPERF_HW_TAKICK			0x08U /*!< TA task started */
#define MUSA_HWPERF_HW_TAFINISHED		0x09U /*!< TA task finished */
#define MUSA_HWPERF_HW_3DTQKICK			0x0AU /*!< 3D TQ started */
#define MUSA_HWPERF_HW_3DKICK			0x0BU /*!< 3D task started */
#define MUSA_HWPERF_HW_3DFINISHED		0x0CU /*!< 3D task finished */
#define MUSA_HWPERF_HW_CMPKICK			0x0DU /*!< CMP task started */
#define MUSA_HWPERF_HW_CMPFINISHED		0x0EU /*!< CMP task finished */
#define MUSA_HWPERF_HW_TLAKICK			0x0FU /*!< TLA task started */
#define MUSA_HWPERF_HW_TLAFINISHED		0x10U /*!< TLS task finished */
#define MUSA_HWPERF_HW_3DSPMKICK			0x11U /*!< 3D SPM task started */
#define MUSA_HWPERF_HW_PERIODIC			0x12U /*!< Periodic event with updated HW counters */
#define MUSA_HWPERF_HW_RTUKICK			0x13U /*!< Reserved, future use */
#define MUSA_HWPERF_HW_RTUFINISHED		0x14U /*!< Reserved, future use */
#define MUSA_HWPERF_HW_SHGKICK			0x15U /*!< Reserved, future use */
#define MUSA_HWPERF_HW_SHGFINISHED		0x16U /*!< Reserved, future use */
#define MUSA_HWPERF_HW_3DTQFINISHED		0x17U /*!< 3D TQ finished */
#define MUSA_HWPERF_HW_3DSPMFINISHED		0x18U /*!< 3D SPM task finished */

#define MUSA_HWPERF_HW_PMOOM_TARESUME	0x19U /*!< TA Resume after PM Out of Memory */

/*! HW_EVENT_RANGE0 used up. Use next empty range below to add new hardware events */
#define MUSA_HWPERF_HW_EVENT_RANGE0_LAST_TYPE	0x19U

/*! other types 0x1A..0x1F */
#define MUSA_HWPERF_CLKS_CHG				0x1AU /*!< Clock speed change in GPU */
#define MUSA_HWPERF_GPU_STATE_CHG		0x1BU /*!< GPU work state change */

/*! power types 0x20..0x27 */
#define MUSA_HWPERF_PWR_EST_RANGE_FIRST_TYPE	0x20U
#define MUSA_HWPERF_PWR_EST_REQUEST		0x20U /*!< Power estimate requested (via GPIO) */
#define MUSA_HWPERF_PWR_EST_READY		0x21U /*!< Power estimate inputs ready */
#define MUSA_HWPERF_PWR_EST_RESULT		0x22U /*!< Power estimate result calculated */
#define MUSA_HWPERF_PWR_EST_RANGE_LAST_TYPE	0x22U

#define MUSA_HWPERF_PWR_CHG				0x23U /*!< Power state change */

/*! HW_EVENT_RANGE1 0x28..0x2F, for accommodating new hardware events */
#define MUSA_HWPERF_HW_EVENT_RANGE1_FIRST_TYPE	0x28U

#define MUSA_HWPERF_HW_TRANSFERKICK			0x28U /*!< TRANSFER task started */
#define MUSA_HWPERF_HW_TRANSFERFINISHED		0x29U /*!< TRANSFER task finished */
#define MUSA_HWPERF_HW_NULLKICK			0x2AU /*!< NULL event */

#define MUSA_HWPERF_HW_CEKICK			0x2BU /*!< CE task started */
#define MUSA_HWPERF_HW_CEFINISHED		0x2CU /*!< CE task finished */

#define MUSA_HWPERF_HW_EVENT_RANGE1_LAST_TYPE 0x2CU

/*! context switch types 0x30..0x31 */
#define MUSA_HWPERF_CSW_START			0x30U /*!< HW context store started */
#define MUSA_HWPERF_CSW_FINISHED			0x31U /*!< HW context store finished */

/*! DVFS events */
#define MUSA_HWPERF_DVFS					0x32U /*!< Dynamic voltage/frequency scaling events */

/*! firmware misc 0x38..0x39 */
#define MUSA_HWPERF_UFO					0x38U /*!< FW UFO Check / Update */
#define MUSA_HWPERF_FWACT				0x39U /*!< FW Activity notification */

/*! last */
#define MUSA_HWPERF_LAST_TYPE			0x3BU

/*! This enumeration must have a value that is a power of two as it is
 * used in masks and a filter bit field (currently 64 bits long).
 */
#define MUSA_HWPERF_MAX_TYPE				0x40U

static_assert(MUSA_HWPERF_LAST_TYPE < MUSA_HWPERF_MAX_TYPE, "Too many HWPerf event types");

/*! Macro used to check if an event type ID is present in the known set of hardware type events */
#define HWPERF_PACKET_IS_HW_TYPE(_etype)	(((_etype) >= MUSA_HWPERF_HW_EVENT_RANGE0_FIRST_TYPE && (_etype) <= MUSA_HWPERF_HW_EVENT_RANGE0_LAST_TYPE) || \
											 ((_etype) >= MUSA_HWPERF_HW_EVENT_RANGE1_FIRST_TYPE && (_etype) <= MUSA_HWPERF_HW_EVENT_RANGE1_LAST_TYPE))

/*! Macro used to check if an event type ID is present in the known set of firmware type events */
#define HWPERF_PACKET_IS_FW_TYPE(_etype)					\
	((_etype) >= MUSA_HWPERF_FW_EVENT_RANGE_FIRST_TYPE &&	\
	 (_etype) <= MUSA_HWPERF_FW_EVENT_RANGE_LAST_TYPE)


typedef enum {
	MUSA_HWPERF_HOST_INVALID   = 0x00,           /*!< Invalid, do not use. */
	MUSA_HWPERF_HOST_ENQ       = 0x01,           /*!< ``0x01`` Kernel driver has queued GPU work.
	                                             See MUSA_HWPERF_HOST_ENQ_DATA */
	MUSA_HWPERF_HOST_UFO       = 0x02,           /*!< ``0x02`` UFO updated by the driver.
	                                             See MUSA_HWPERF_HOST_UFO_DATA */
	MUSA_HWPERF_HOST_ALLOC     = 0x03,           /*!< ``0x03`` Resource allocated.
	                                             See MUSA_HWPERF_HOST_ALLOC_DATA */
	MUSA_HWPERF_HOST_CLK_SYNC  = 0x04,           /*!< ``0x04`` GPU / Host clocks correlation data.
	                                             See MUSA_HWPERF_HOST_CLK_SYNC_DATA */
	MUSA_HWPERF_HOST_FREE      = 0x05,           /*!< ``0x05`` Resource freed,
	                                             See MUSA_HWPERF_HOST_FREE_DATA */
	MUSA_HWPERF_HOST_MODIFY    = 0x06,           /*!< ``0x06`` Resource modified / updated.
	                                             See MUSA_HWPERF_HOST_MODIFY_DATA */
	MUSA_HWPERF_HOST_DEV_INFO  = 0x07,           /*!< ``0x07`` Device Health status.
	                                             See MUSA_HWPERF_HOST_DEV_INFO_DATA */
	MUSA_HWPERF_HOST_INFO      = 0x08,           /*!< ``0x08`` Device memory usage information.
	                                             See MUSA_HWPERF_HOST_INFO_DATA */
	MUSA_HWPERF_HOST_SYNC_FENCE_WAIT = 0x09,     /*!< ``0x09`` Wait for sync event.
	                                             See MUSA_HWPERF_HOST_SYNC_FENCE_WAIT_DATA */
	MUSA_HWPERF_HOST_SYNC_SW_TL_ADVANCE  = 0x0A, /*!< ``0x0A`` Software timeline advanced.
	                                             See MUSA_HWPERF_HOST_SYNC_SW_TL_ADV_DATA */
	MUSA_HWPERF_HOST_CLIENT_INFO = 0x0B,			/*!< ``0x0B`` Additional client info.
	                                             See MUSA_HWPERF_HOST_CLIENT_INFO_DATA */
	MUSA_HWPERF_HOST_TIME_CORRELATION  = 0x0C,   /*!< ``0x0C`` time correlation info.
	                                             See MUSA_HWPERF_HOST_TIME_CORRELATION_DATA */
	MUSA_HWPERF_HOST_DMA_START  = 0x0D,          /*!< ``0x0D`` DMA transfer start.
	                                             See MUSA_HWPERF_HOST_DMA_DATA */
	MUSA_HWPERF_HOST_DMA_END    = 0x0E,          /*!< ``0x0E`` DMA transfer end.
	                                             See MUSA_HWPERF_HOST_DMA_DATA */
	MUSA_HWPERF_HOST_NOTIFY     = 0x0F,          /*!< ``0x0F`` Kernel driver has added more data to GPU.
	                                             See MUSA_HWPERF_HOST_NOTIFY_DATA */
	MUSA_HWPERF_HOST_DEBUG_START = 0x10,         /*!< ``0x10`` Debug event start.
	                                             See MUSA_HWPERF_HOST_DEBUG_DATA */
	MUSA_HWPERF_HOST_DEBUG_END = 0x11,           /*!< ``0x10`` Debug event end.
	                                             See MUSA_HWPERF_HOST_DEBUG_DATA */
	/*! last */
	MUSA_HWPERF_HOST_LAST_TYPE,

	/*! This enumeration must have a value that is a power of two as it is
	 * used in masks and a filter bit field (currently 32 bits long).
	 */
	MUSA_HWPERF_HOST_MAX_TYPE  = 0x20
} MUSA_HWPERF_HOST_EVENT_TYPE;

/*!< The event type values are incrementing integers for use as a shift ordinal
 * in the event filtering process at the point events are generated.
 * This scheme thus implies a limit of 31 event types.
 */
static_assert(MUSA_HWPERF_HOST_LAST_TYPE < MUSA_HWPERF_HOST_MAX_TYPE, "Too many HWPerf host event types");


/******************************************************************************
 * Packet Header Format Version 2 Types
 *****************************************************************************/

/*! Major version number of the protocol in operation
 */
#define MUSA_HWPERF_V2_FORMAT 2

/*! Signature ASCII pattern 'HWP2' found in the first word of a HWPerfV2 packet
 */
#define HWPERF_PACKET_V2_SIG		0x48575032

/*! Signature ASCII pattern 'HWPA' found in the first word of a HWPerfV2a packet
 */
#define HWPERF_PACKET_V2A_SIG		0x48575041

/*! Signature ASCII pattern 'HWPB' found in the first word of a HWPerfV2b packet
 */
#define HWPERF_PACKET_V2B_SIG		0x48575042

/*! Signature ASCII pattern 'HWPC' found in the first word of a HWPerfV2c packet
 */
#define HWPERF_PACKET_V2C_SIG		0x48575043

/*! Signature ASCII pattern 'HWPD' found in the first word of a HWPerfV2c packet
 */
#define HWPERF_PACKET_V2D_SIG		0x48575044

#define HWPERF_PACKET_ISVALID(_val) (((_val) == HWPERF_PACKET_V2_SIG) || ((_val) == HWPERF_PACKET_V2A_SIG) || ((_val) == HWPERF_PACKET_V2B_SIG) || ((_val) == HWPERF_PACKET_V2C_SIG) || ((_val) == HWPERF_PACKET_V2D_SIG))
/*!< Checks that the packet signature is one of the supported versions */

/*! Type defines the HWPerf packet header common to all events. */
typedef struct
{
	MT_UINT32  ui32Sig;        /*!< Always the value HWPERF_PACKET_SIG */
	MT_UINT32  ui32Size;       /*!< Overall packet size in bytes */
	MT_UINT32  eTypeId;        /*!< Event type information field */
	MT_UINT32  ui32Ordinal;    /*!< Sequential number of the packet */
	MT_UINT64  ui64Timestamp;  /*!< Event timestamp */
} MUSA_HWPERF_V2_PACKET_HDR, *MUSA_PHWPERF_V2_PACKET_HDR;

MUSA_FW_STRUCT_OFFSET_ASSERT(MUSA_HWPERF_V2_PACKET_HDR, ui64Timestamp);

MUSA_FW_STRUCT_SIZE_ASSERT(MUSA_HWPERF_V2_PACKET_HDR);


/*! Mask for use with the MT_UINT32 ui32Size header field */
#define MUSA_HWPERF_SIZE_MASK         0xFFFFU

/*! This macro defines an upper limit to which the size of the largest variable
 * length HWPerf packet must fall within, currently 3KB. This constant may be
 * used to allocate a buffer to hold one packet.
 * This upper limit is policed by packet producing code.
 */
#define MUSA_HWPERF_MAX_PACKET_SIZE   0xC00U

/*! Defines an upper limit to the size of a variable length packet payload.
 */
#define MUSA_HWPERF_MAX_PAYLOAD_SIZE	 ((MT_UINT32)(MUSA_HWPERF_MAX_PACKET_SIZE-\
	sizeof(MUSA_HWPERF_V2_PACKET_HDR)))

/*! Macro which takes a structure name and provides the packet size for
 * a fixed size payload packet, rounded up to 8 bytes to align packets
 * for 64 bit architectures. */
#define MUSA_HWPERF_MAKE_SIZE_FIXED(_struct)       ((MT_UINT32)(MUSA_HWPERF_SIZE_MASK&(sizeof(MUSA_HWPERF_V2_PACKET_HDR)+MT_ALIGN(sizeof(_struct), MTGPUTL_PACKET_ALIGNMENT))))

/*! Macro which takes the number of bytes written in the data payload of a
 * packet for a variable size payload packet, rounded up to 8 bytes to
 * align packets for 64 bit architectures. */
#define MUSA_HWPERF_MAKE_SIZE_VARIABLE(_size)      ((MT_UINT32)(MUSA_HWPERF_SIZE_MASK&((MT_UINT32)sizeof(MUSA_HWPERF_V2_PACKET_HDR)+MT_ALIGN((_size), MTGPUTL_PACKET_ALIGNMENT))))

/*! Macro to obtain the size of the packet */
#define MUSA_HWPERF_GET_SIZE(_packet_addr)         ((MT_UINT16)(((_packet_addr)->ui32Size) & MUSA_HWPERF_SIZE_MASK))

/*! Macro to obtain the size of the packet data */
#define MUSA_HWPERF_GET_DATA_SIZE(_packet_addr)    (MUSA_HWPERF_GET_SIZE(_packet_addr) - sizeof(MUSA_HWPERF_V2_PACKET_HDR))

/*! Masks for use with the MT_UINT32 eTypeId header field */
#define MUSA_HWPERF_TYPEID_MASK			0x0007FFFFU
#define MUSA_HWPERF_TYPEID_EVENT_MASK	        0x00007FFFU
#define MUSA_HWPERF_TYPEID_EVENT_MASK_V2D        0x0007FFFFU
#define MUSA_HWPERF_TYPEID_THREAD_MASK		0x00008000U
#define MUSA_HWPERF_TYPEID_STREAM_MASK		0x00070000U
#define MUSA_HWPERF_TYPEID_STREAM_MASK_V2D       0x1c000000U
#define MUSA_HWPERF_TYPEID_MCU_DMA_MASK		0x00080000U
#define MUSA_HWPERF_TYPEID_VERSION_MASK          0x03F80000U
#define MUSA_HWPERF_TYPEID_M_CORE_MASK		0x00100000U
#define MUSA_HWPERF_TYPEID_OSID_MASK		0x07000000U

/*! Mcu thread macros for encoding the ID into the type field of a packet */
#define MUSA_HWPERF_MCU_THREAD_SHIFT	15U
#define MUSA_HWPERF_MCU_THREAD_ID0		0x0U  /*!< Mcu Thread 0 ID */
#define MUSA_HWPERF_MCU_THREAD_ID1		0x1U  /*!< Mcu Thread 1 ID */
/*! Obsolete, kept for source compatibility */
#define MUSA_HWPERF_MCU_THREAD_MASK		0x1U
/*! Stream ID macros for encoding the ID into the type field of a packet */
#define MUSA_HWPERF_STREAM_SHIFT			16U
#define MUSA_HWPERF_STREAM_SHIFT_V2D 		26U
/*! Mcu DMA macro for encoding how the packet was generated into the type field of a packet */
#define MUSA_HWPERF_MCU_DMA_SHIFT		19U
/*! Bit-shift macro used for encoding multi-core data into the type field of a packet */
#define MUSA_HWPERF_M_CORE_SHIFT			20U
/*! OSID bit-shift macro used for encoding OSID into type field of a packet */
#define MUSA_HWPERF_OSID_SHIFT			24U
#define MUSA_HWPERF_VERSION_SHIFT 		19U
typedef enum {
	MUSA_HWPERF_STREAM_ID0_FW,     /*!< Events from the Firmware/GPU */
	MUSA_HWPERF_STREAM_ID1_HOST,   /*!< Events from the Server host driver component */
	MUSA_HWPERF_STREAM_ID2_CLIENT, /*!< Events from the Client host driver component */
	MUSA_HWPERF_STREAM_ID_LAST,
} MUSA_HWPERF_STREAM_ID;

/* Checks if all stream IDs can fit under MUSA_HWPERF_TYPEID_STREAM_MASK. */
static_assert(((MT_UINT32)MUSA_HWPERF_STREAM_ID_LAST - 1U) < (MUSA_HWPERF_TYPEID_STREAM_MASK >> MUSA_HWPERF_STREAM_SHIFT),
		"Too many HWPerf stream IDs.");

/*! Compile-time value used to seed the Multi-Core (MC) bit in the typeID field.
 *  Only set by MUSA_FIRMWARE builds.
 */
#if defined(MUSA_FIRMWARE)
# if defined(MUSA_FEATURE_GPU_MULTICORE_SUPPORT)
#define MUSA_HWPERF_M_CORE_VALUE 1U  /*!< 1 => Multi-core supported */
# else
#define MUSA_HWPERF_M_CORE_VALUE 0U  /*!< 0 => Multi-core not supported */
# endif
#else
#define MUSA_HWPERF_M_CORE_VALUE 0U  /*!< 0 => Multi-core not supported */
#endif

/*! Macros used to set the packet type and encode meta thread ID (0|1),
 * HWPerf stream ID, multi-core capability and OSID within the typeID */
#define MUSA_HWPERF_MAKE_TYPEID(_stream, _type, _thread, _metadma, _osid)\
		((MT_UINT32) ((MUSA_HWPERF_TYPEID_STREAM_MASK&((MT_UINT32)(_stream) << MUSA_HWPERF_STREAM_SHIFT)) | \
		(MUSA_HWPERF_TYPEID_THREAD_MASK & ((MT_UINT32)(_thread) << MUSA_HWPERF_MCU_THREAD_SHIFT)) | \
		(MUSA_HWPERF_TYPEID_EVENT_MASK & (MT_UINT32)(_type)) | \
		(MUSA_HWPERF_TYPEID_MCU_DMA_MASK & ((MT_UINT32)(_metadma) << MUSA_HWPERF_MCU_DMA_SHIFT)) | \
		(MUSA_HWPERF_TYPEID_OSID_MASK & ((MT_UINT32)(_osid) << MUSA_HWPERF_OSID_SHIFT)) | \
		(MUSA_HWPERF_TYPEID_M_CORE_MASK & ((MT_UINT32)(MUSA_HWPERF_M_CORE_VALUE) << MUSA_HWPERF_M_CORE_SHIFT))))

#define MUSA_HWPERF_MAKE_TYPEID_V2D(_stream, _version, _type)                                                          \
    ((MT_UINT32)((MUSA_HWPERF_TYPEID_STREAM_MASK_V2D & ((MT_UINT32)(_stream) << MUSA_HWPERF_STREAM_SHIFT_V2D)) |      \
                  (MUSA_HWPERF_TYPEID_VERSION_MASK & ((MT_UINT32)(_version) << MUSA_HWPERF_VERSION_SHIFT)) |           \
                  (MUSA_HWPERF_TYPEID_EVENT_MASK_V2D & (MT_UINT32)(_type))))

/*! Obtains the event type that generated the packet */
#define MUSA_HWPERF_GET_TYPE(_packet_addr)            (((_packet_addr)->eTypeId) & MUSA_HWPERF_TYPEID_EVENT_MASK)

/*! Obtains the MCU Thread number that generated the packet */
#define MUSA_HWPERF_GET_THREAD_ID(_packet_addr)       (((((_packet_addr)->eTypeId) & MUSA_HWPERF_TYPEID_THREAD_MASK) >> MUSA_HWPERF_MCU_THREAD_SHIFT))

/*! Determines if the packet generated contains multi-core data */
#define MUSA_HWPERF_GET_M_CORE(_packet_addr)          (((_packet_addr)->eTypeId & MUSA_HWPERF_TYPEID_M_CORE_MASK) >> MUSA_HWPERF_M_CORE_SHIFT)

/*! Obtains the guest OSID which resulted in packet generation */
#define MUSA_HWPERF_GET_OSID(_packet_addr)            (((_packet_addr)->eTypeId & MUSA_HWPERF_TYPEID_OSID_MASK) >> MUSA_HWPERF_OSID_SHIFT)

/*! Obtain stream id */
#define MUSA_HWPERF_GET_STREAM_ID(_packet_addr)       (((((_packet_addr)->eTypeId) & MUSA_HWPERF_TYPEID_STREAM_MASK) >> MUSA_HWPERF_STREAM_SHIFT))

/*! Obtain information about how the packet was generated, which might affect payload total size */
#define MUSA_HWPERF_GET_MCU_DMA_INFO(_packet_addr)   (((((_packet_addr)->eTypeId) & MUSA_HWPERF_TYPEID_MCU_DMA_MASK) >> MUSA_HWPERF_MCU_DMA_SHIFT))

/*! Obtains a typed pointer to a packet given a buffer address */
#define MUSA_HWPERF_GET_PACKET(_buffer_addr)            ((MUSA_HWPERF_V2_PACKET_HDR *)(void *)  (_buffer_addr))
/*! Obtains a typed pointer to a data structure given a packet address */
#define MUSA_HWPERF_GET_PACKET_DATA_BYTES(_packet_addr) (MT_OFFSET_ADDR((_packet_addr), sizeof(MUSA_HWPERF_V2_PACKET_HDR)))
/*! Obtains a typed pointer to the next packet given a packet address */
#define MUSA_HWPERF_GET_NEXT_PACKET(_packet_addr)       ((MUSA_HWPERF_V2_PACKET_HDR *)  (MT_OFFSET_ADDR((_packet_addr), MUSA_HWPERF_SIZE_MASK&((_packet_addr)->ui32Size))))

/*! Obtains a typed pointer to a packet header given the packet data address */
#define MUSA_HWPERF_GET_PACKET_HEADER(_packet_addr)     ((MUSA_HWPERF_V2_PACKET_HDR *)  (MT_OFFSET_ADDR((_packet_addr), -(MT_INT32)sizeof(MUSA_HWPERF_V2_PACKET_HDR))))


/******************************************************************************
 * Other Common Defines
 *****************************************************************************/

/*! This macro is not a real array size, but indicates the array has a variable
 * length only known at run-time but always contains at least 1 element. The
 * final size of the array is deduced from the size field of a packet header.
 */
#define MUSA_HWPERF_ONE_OR_MORE_ELEMENTS  1U

/*! This macro is not a real array size, but indicates the array is optional
 * and if present has a variable length only known at run-time. The final
 * size of the array is deduced from the size field of a packet header. */
#define MUSA_HWPERF_ZERO_OR_MORE_ELEMENTS 1U


/*! Masks for use with the MT_UINT32 ui32BlkInfo field */
#define MUSA_HWPERF_BLKINFO_BLKCOUNT_MASK	0xFFFF0000U
#define MUSA_HWPERF_BLKINFO_BLKOFFSET_MASK	0x0000FFFFU

/*! Shift for the NumBlocks and counter block offset field in ui32BlkInfo */
#define MUSA_HWPERF_BLKINFO_BLKCOUNT_SHIFT	16U
#define MUSA_HWPERF_BLKINFO_BLKOFFSET_SHIFT 0U

/*! Macro used to set the block info word as a combination of two 16-bit integers */
#define MUSA_HWPERF_MAKE_BLKINFO(_numblks, _blkoffset) ((MT_UINT32) ((MUSA_HWPERF_BLKINFO_BLKCOUNT_MASK&((_numblks) << MUSA_HWPERF_BLKINFO_BLKCOUNT_SHIFT)) | (MUSA_HWPERF_BLKINFO_BLKOFFSET_MASK&((_blkoffset) << MUSA_HWPERF_BLKINFO_BLKOFFSET_SHIFT))))

/*! Macro used to obtain the number of counter blocks present in the packet */
#define MUSA_HWPERF_GET_BLKCOUNT(_blkinfo)            (((_blkinfo) & MUSA_HWPERF_BLKINFO_BLKCOUNT_MASK) >> MUSA_HWPERF_BLKINFO_BLKCOUNT_SHIFT)

/*! Obtains the offset of the counter block stream in the packet */
#define MUSA_HWPERF_GET_BLKOFFSET(_blkinfo)           (((_blkinfo) & MUSA_HWPERF_BLKINFO_BLKOFFSET_MASK) >> MUSA_HWPERF_BLKINFO_BLKOFFSET_SHIFT)

/*! This macro gets the number of blocks depending on the packet version */
#define MUSA_HWPERF_GET_NUMBLKS(_sig, _packet_data, _numblocks) \
	do { \
		if (HWPERF_PACKET_V2B_SIG == (_sig) || HWPERF_PACKET_V2C_SIG == (_sig)) \
		{ \
			(_numblocks) = MUSA_HWPERF_GET_BLKCOUNT((_packet_data)->ui32BlkInfo);\
		} \
		else \
		{ \
			MT_UINT32 ui32VersionOffset = (((_sig) == HWPERF_PACKET_V2_SIG) ? 1 : 3);\
			(_numblocks) = *(MT_UINT16 *)(MT_OFFSET_ADDR(&(_packet_data)->ui32WorkTarget, ui32VersionOffset)); \
		} \
	} while (0)

/*! This macro gets the counter stream pointer depending on the packet version */
#define MUSA_HWPERF_GET_CNTSTRM(_sig, _hw_packet_data, _cntstream_ptr) \
{ \
	if (HWPERF_PACKET_V2B_SIG == (_sig) || HWPERF_PACKET_V2C_SIG == (_sig)) \
	{ \
		(_cntstream_ptr) = (MT_UINT32 *)(MT_OFFSET_ADDR((_hw_packet_data), MUSA_HWPERF_GET_BLKOFFSET((_hw_packet_data)->ui32BlkInfo))); \
	} \
	else \
	{ \
		MT_UINT32 ui32BlkStreamOffsetInWords = (((_sig) == HWPERF_PACKET_V2_SIG) ? 6 : 8); \
		(_cntstream_ptr) = (MT_UINT32 *)(MT_OFFSET_ADDR_DW((_hw_packet_data), ui32BlkStreamOffsetInWords)); \
	} \
}

/*! Masks for use with the MT_UINT32 ui32KickInfo field */
#define MUSA_HWPERF_KICKINFO_KICKID_MASK	0x000000FFU

/*! Shift for the Kick ID field in ui32KickInfo */
#define MUSA_HWPERF_KICKINFO_KICKID_SHIFT 0U

/*! Macro used to set the kick info field. */
#define MUSA_HWPERF_MAKE_KICKINFO(_kickid) ((MT_UINT32) (MUSA_HWPERF_KICKINFO_KICKID_MASK&((_kickid) << MUSA_HWPERF_KICKINFO_KICKID_SHIFT)))

/*! Macro used to obtain the Kick ID if present in the packet */
#define MUSA_HWPERF_GET_KICKID(_kickinfo)            (((_kickinfo) & MUSA_HWPERF_KICKINFO_KICKID_MASK) >> MUSA_HWPERF_KICKINFO_KICKID_SHIFT)

/*! Masks for use with the MUSA_HWPERF_UFO_EV eEvType field */
#define MUSA_HWPERF_UFO_STREAMSIZE_MASK 0xFFFF0000U
#define MUSA_HWPERF_UFO_STREAMOFFSET_MASK 0x0000FFFFU

/*! Shift for the UFO count and data stream fields */
#define MUSA_HWPERF_UFO_STREAMSIZE_SHIFT 16U
#define MUSA_HWPERF_UFO_STREAMOFFSET_SHIFT 0U

/*! Macro used to set UFO stream info word as a combination of two 16-bit integers */
#define MUSA_HWPERF_MAKE_UFOPKTINFO(_ssize, _soff) \
        ((MT_UINT32) ((MUSA_HWPERF_UFO_STREAMSIZE_MASK&((_ssize) << MUSA_HWPERF_UFO_STREAMSIZE_SHIFT)) | \
        (MUSA_HWPERF_UFO_STREAMOFFSET_MASK&((_soff) << MUSA_HWPERF_UFO_STREAMOFFSET_SHIFT))))

/*! Macro used to obtain UFO count*/
#define MUSA_HWPERF_GET_UFO_STREAMSIZE(_streaminfo) \
        (((_streaminfo) & MUSA_HWPERF_UFO_STREAMSIZE_MASK) >> MUSA_HWPERF_UFO_STREAMSIZE_SHIFT)

/*! Obtains the offset of the UFO stream in the packet */
#define MUSA_HWPERF_GET_UFO_STREAMOFFSET(_streaminfo) \
        (((_streaminfo) & MUSA_HWPERF_UFO_STREAMOFFSET_MASK) >> MUSA_HWPERF_UFO_STREAMOFFSET_SHIFT)


#if defined(__cplusplus)
}
#endif

#endif /* MUSA_HWPERF_COMMON_H_ */

/******************************************************************************
 End of file
******************************************************************************/

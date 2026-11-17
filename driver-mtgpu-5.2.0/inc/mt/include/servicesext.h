/*************************************************************************/ /*!
@File
@Title          Services definitions required by external drivers
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Provides services data structures, defines and prototypes
                required by external drivers
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

#if !defined(SERVICESEXT_H)
#define SERVICESEXT_H

/* include/ */
#include "mtgpu_error.h"
#include "img_types.h"
#include "img_3dtypes.h"
#include "mtgpu_device_types.h"

/*
 * Lock buffer read/write flags
 */
#define MTGPU_LOCKFLG_READONLY		(1)		/*!< The locking process will only read the locked surface */

/*!
 *****************************************************************************
 *	Services State
 *****************************************************************************/
typedef enum _MTGPU_SERVICES_STATE_
{
	MTGPU_SERVICES_STATE_UNDEFINED = 0,
	MTGPU_SERVICES_STATE_OK,
	MTGPU_SERVICES_STATE_BAD,
} MTGPU_SERVICES_STATE;


/*!
 *****************************************************************************
 *	States for power management
 *****************************************************************************/
/*!
  System Power State Enum
 */
typedef enum _MTGPU_SYS_POWER_STATE_
{
	MTGPU_SYS_POWER_STATE_Unspecified		= -1,	/*!< Unspecified : Uninitialised */
	MTGPU_SYS_POWER_STATE_OFF				= 0,	/*!< Off */
	MTGPU_SYS_POWER_STATE_ON				= 1,	/*!< On */

	MTGPU_SYS_POWER_STATE_FORCE_I32 = 0x7fffffff	/*!< Force enum to be at least 32-bits wide */

} MTGPU_SYS_POWER_STATE, *PMTGPU_SYS_POWER_STATE; /*!< Typedef for ptr to MTGPU_SYS_POWER_STATE */

/*!
  Device Power State Enum
 */
typedef MT_INT32 MTGPU_DEV_POWER_STATE;
typedef MT_INT32 *PMTGPU_DEV_POWER_STATE;	/*!< Typedef for ptr to MTGPU_DEV_POWER_STATE */ /* PRQA S 3205 */
#define MTGPU_DEV_POWER_STATE_DEFAULT	-1	/*!< Default state for the device */
#define MTGPU_DEV_POWER_STATE_OFF		 0	/*!< Unpowered */
#define MTGPU_DEV_POWER_STATE_ON		 1	/*!< Running */

/*!
  Power Flags Enum
 */
typedef MT_UINT32 MTGPU_POWER_FLAGS;
#define MTGPU_POWER_FLAGS_NONE		0U			/*!< No flags */
#define MTGPU_POWER_FLAGS_FORCED	1U << 0		/*!< Power the transition should not fail */
#define MTGPU_POWER_FLAGS_SUSPEND	1U << 1		/*!< Power transition is due to OS suspend request */

/* Clock speed handler prototypes */

/*!
  Typedef for a pointer to a Function that will be called before a transition
  from one clock speed to another. See also PFN_POST_CLOCKSPEED_CHANGE.
 */
typedef MTGPU_ERROR (*PFN_PRE_CLOCKSPEED_CHANGE) (MT_HANDLE				hDevHandle,
												   MTGPU_DEV_POWER_STATE	eCurrentPowerState);

/*!
  Typedef for a pointer to a Function that will be called after a transition
  from one clock speed to another. See also PFN_PRE_CLOCKSPEED_CHANGE.
 */
typedef MTGPU_ERROR (*PFN_POST_CLOCKSPEED_CHANGE) (MT_HANDLE				hDevHandle,
													MTGPU_DEV_POWER_STATE	eCurrentPowerState);

/*!
  Typedef for a pointer to a function that will be called to transition the
  device to a forced idle state. Used in unison with (forced) power requests,
  DVFS and cluster count changes.
 */
typedef MTGPU_ERROR (*PFN_FORCED_IDLE_REQUEST) (MT_HANDLE		hDevHandle,
												 MT_BOOL		bDeviceOffPermitted);

/*!
  Typedef for a pointer to a function that will be called to cancel a forced
  idle state and return the firmware back to a state where the hardware can be
  scheduled.
 */
typedef MTGPU_ERROR (*PFN_FORCED_IDLE_CANCEL_REQUEST) (MT_HANDLE	hDevHandle);

typedef MTGPU_ERROR (*PFN_GPU_UNITS_POWER_CHANGE) (MT_HANDLE		hDevHandle,
													MT_UINT32		ui32SESPowerState);

/*!
 *****************************************************************************
 * This structure is used for OS independent registry (profile) access
 *****************************************************************************/

typedef struct MTGPU_REGISTRY_INFO_TAG
{
	MT_UINT32	ui32DevCookie;
	MT_PCHAR	pszKey;
	MT_PCHAR	pszValue;
	MT_PCHAR	pszBuf;
	MT_UINT32	ui32BufSize;
} MTGPU_REGISTRY_INFO, *PMTGPU_REGISTRY_INFO;

#endif /* SERVICESEXT_H */
/******************************************************************************
 End of file (servicesext.h)
******************************************************************************/

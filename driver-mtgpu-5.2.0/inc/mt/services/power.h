/*************************************************************************/ /*!
@File
@Title          Power Management Functions
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Main APIs for power management functions
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
#ifndef POWER_H
#define POWER_H

#include "img_defs.h"
#include "img_types.h"
#include "mt_device.h"
#include "mtgpu_error.h"
#include "servicesext.h"
#include "opaque_types.h"

/*!
 *****************************************************************************
 *	Power management
 *****************************************************************************/

typedef struct _MTGPU_POWER_DEV_TAG_ MTGPU_POWER_DEV;

typedef MT_BOOL (*PFN_SYS_DEV_IS_DEFAULT_STATE_OFF)(MTGPU_POWER_DEV *psPowerDevice);

/* Power transition handler prototypes */

/*!
  Typedef for a pointer to a Function that will be called before a transition
  from one power state to another. See also PFN_POST_POWER.
 */
typedef MTGPU_ERROR (*PFN_PRE_POWER) (MT_HANDLE				hDevHandle,
									   MTGPU_DEV_POWER_STATE	eNewPowerState,
									   MTGPU_DEV_POWER_STATE	eCurrentPowerState,
									   MTGPU_POWER_FLAGS		ePwrFlags);
/*!
  Typedef for a pointer to a Function that will be called after a transition
  from one power state to another. See also PFN_PRE_POWER.
 */
typedef MTGPU_ERROR (*PFN_POST_POWER) (MT_HANDLE				hDevHandle,
										MTGPU_DEV_POWER_STATE	eNewPowerState,
										MTGPU_DEV_POWER_STATE	eCurrentPowerState,
										MTGPU_POWER_FLAGS		ePwrFlags);

MTGPU_ERROR MTGPUPowerLockInit(PMTGPU_DEVICE_NODE psDeviceNode);
void MTGPUPowerLockDeInit(PMTGPU_DEVICE_NODE psDeviceNode);

/*!
******************************************************************************

 @Function	MTGPUPowerLock

 @Description	Obtain the mutex for power transitions. Only allowed when
                system power is on.

 @Return	MTGPU_ERROR_SYSTEM_STATE_POWERED_OFF or MTGPU_OK

******************************************************************************/
MTGPU_ERROR MTGPUPowerLock(PMTGPU_DEVICE_NODE psDeviceNode);

/*!
******************************************************************************

 @Function	MTGPUPowerUnlock

 @Description	Release the mutex for power transitions

 @Return	MTGPU_ERROR

******************************************************************************/
void MTGPUPowerUnlock(PMTGPU_DEVICE_NODE psDeviceNode);

/*!
******************************************************************************

 @Function	MTGPUPowerTryLock

 @Description	Try to obtain the mutex for power transitions. Only allowed when
		system power is on.

 @Return	MTGPU_ERROR_RETRY or MTGPU_ERROR_SYSTEM_STATE_POWERED_OFF or
		MTGPU_OK

******************************************************************************/
MTGPU_ERROR MTGPUPowerTryLock(PMTGPU_DEVICE_NODE psDeviceNode);

/*!
******************************************************************************

 @Function     MTGPUPwrLockIsLockedByMe

 @Description  Determine if the calling context is holding the device power-lock

 @Return       MT_BOOL

******************************************************************************/
MT_BOOL MTGPUPwrLockIsLockedByMe(PCMTGPU_DEVICE_NODE psDeviceNode);
MT_BOOL MTGPUDeviceIsDefaultStateOFF(MTGPU_POWER_DEV *psPowerDevice);

/*!
******************************************************************************

 @Function	MTGPUSetDevicePowerStateKM

 @Description	Set the Device into a new state

 @Input		psDeviceNode : Device node
 @Input		eNewPowerState : New power state
 @Input		ePwrFlags : Power state change flags

 @Return	MTGPU_ERROR

******************************************************************************/
MTGPU_ERROR MTGPUSetDevicePowerStateKM(PMTGPU_DEVICE_NODE	psDeviceNode,
										 MTGPU_DEV_POWER_STATE	eNewPowerState,
										 MTGPU_POWER_FLAGS		ePwrFlags);

/*************************************************************************/ /*!
@Function     MTGPUSetDeviceSystemPowerState
@Description  Set the device into a new power state based on the systems power
              state
@Input        psDeviceNode       Device node
@Input        eNewSysPowerState  New system power state
@Input        ePwrFlags          Power state change flags
@Return       MTGPU_ERROR       MTGPU_OK on success or an error otherwise
*/ /**************************************************************************/
MTGPU_ERROR MTGPUSetDeviceSystemPowerState(PMTGPU_DEVICE_NODE psDeviceNode,
											 MTGPU_SYS_POWER_STATE eNewSysPowerState,
											 MTGPU_POWER_FLAGS ePwrFlags);

/*!
******************************************************************************

 @Function      MTGPUSetDeviceDefaultPowerState

 @Description   Set the default device power state to eNewPowerState

 @Input         psDeviceNode : Device node
 @Input         eNewPowerState : New power state

 @Return        MTGPU_ERROR

******************************************************************************/
MTGPU_ERROR MTGPUSetDeviceDefaultPowerState(PCMTGPU_DEVICE_NODE psDeviceNode,
					MTGPU_DEV_POWER_STATE eNewPowerState);

/*!
******************************************************************************

 @Function      MTGPUSetDeviceCurrentPowerState

 @Description   Set the current device power state to eNewPowerState

 @Input         psDeviceNode : Device node
 @Input         eNewPowerState : New power state

 @Return        MTGPU_ERROR

******************************************************************************/
MTGPU_ERROR MTGPUSetDeviceCurrentPowerState(PMTGPU_DEVICE_NODE psDeviceNode,
					      MTGPU_DEV_POWER_STATE eNewPowerState);

/*!
******************************************************************************

 @Function      MTGPUSetSystemPowerState

 @Description   Set the system power state to eNewPowerState

 @Input         psDeviceConfig : Device config
 @Input         eNewPowerState : New power state

 @Return        MTGPU_ERROR

******************************************************************************/
MTGPU_ERROR MTGPUSetSystemPowerState(MTGPU_DEVICE_CONFIG * psDeviceConfig,
											 MTGPU_SYS_POWER_STATE eNewSysPowerState);

/*!
******************************************************************************

 @Function      MTGPUSetPowerCallbacks

 @Description   Initialise the Power Device's function pointers
                to the appropriate callbacks depending on driver mode and
                system setup.

 @Input         psDeviceNode : Device node
 @Input         psPowerDevice : Power device
 @Input         pfnDevicePrePower : regular device pre power callback
 @Input         pfnDevicePostPower : regular device post power callback
 @Input         pfnSystemPrePower : regular system pre power callback
 @Input         pfnDevicePostPower : regular system post power callback
 @Input         pfnSystemPrePower : regular device pre power callback
 @Input         pfnSystemPostPower : regular device pre power callback
 @Input         pfnForcedIdleRequest : forced idle request callback
 @Input         pfnForcedIdleCancelRequest : forced idle request cancel callback

******************************************************************************/
void MTGPUSetPowerCallbacks(PMTGPU_DEVICE_NODE				psDeviceNode,
							 MTGPU_POWER_DEV					*psPowerDevice,
							 PFN_PRE_POWER						pfnDevicePrePower,
							 PFN_POST_POWER					    pfnDevicePostPower,
							 PFN_SYS_PRE_POWER				    pfnSystemPrePower,
							 PFN_SYS_POST_POWER			        pfnSystemPostPower,
							 PFN_FORCED_IDLE_REQUEST			pfnForcedIdleRequest,
							 PFN_FORCED_IDLE_CANCEL_REQUEST	pfnForcedIdleCancelRequest);

/* Type PFN_DC_REGISTER_POWER */
MTGPU_ERROR MTGPURegisterPowerDevice(PMTGPU_DEVICE_NODE				psDeviceNode,
									   PFN_PRE_POWER					pfnDevicePrePower,
									   PFN_POST_POWER					pfnDevicePostPower,
									   PFN_SYS_PRE_POWER			    pfnSystemPrePower,
									   PFN_SYS_POST_POWER			    pfnSystemPostPower,
									   PFN_PRE_CLOCKSPEED_CHANGE		pfnPreClockSpeedChange,
									   PFN_POST_CLOCKSPEED_CHANGE		pfnPostClockSpeedChange,
									   PFN_FORCED_IDLE_REQUEST			pfnForcedIdleRequest,
									   PFN_FORCED_IDLE_CANCEL_REQUEST	pfnForcedIdleCancelRequest,
									   PFN_GPU_UNITS_POWER_CHANGE		pfnGPUUnitsPowerChange,
									   MT_HANDLE						hDevCookie,
									   MTGPU_DEV_POWER_STATE			eCurrentPowerState,
									   MTGPU_DEV_POWER_STATE			eDefaultPowerState);

/*!
******************************************************************************

 @Function	MTGPURemovePowerDevice

 @Description

 Removes device from power management register. Device is located by Device Index

 @Input		psDeviceNode : Device node

******************************************************************************/
void MTGPURemovePowerDevice(PMTGPU_DEVICE_NODE psDeviceNode);

/*!
******************************************************************************

 @Function	MTGPUGetDevicePowerState

 @Description

	Return the device power state

 @Input		psDeviceNode : Device node
 @Output	pePowerState : Current power state

 @Return	MTGPU_ERROR_UNKNOWN_POWER_STATE if device could not be found.
            MTGPU_OK otherwise.

******************************************************************************/
MTGPU_ERROR MTGPUGetDevicePowerState(PCMTGPU_DEVICE_NODE psDeviceNode,
									   PMTGPU_DEV_POWER_STATE pePowerState);

/*!
******************************************************************************

 @Function	MTGPUIsDevicePowered

 @Description

	Whether the device is powered, for the purposes of lockup detection.

 @Input		psDeviceNode : Device node

 @Return	MT_BOOL

******************************************************************************/
MT_BOOL MTGPUIsDevicePowered(PMTGPU_DEVICE_NODE psDeviceNode);

/**************************************************************************/ /*!
@Function       MTGPUDevicePreClockSpeedChange

@Description    This function is called before a voltage/frequency change is
                made to the GPU HW. It informs the host driver of the intention
                to make a DVFS change. If allows the host driver to idle
                the GPU and begin a hold off period from starting new work
                on the GPU.
                When this call succeeds the caller *must* call
                MTGPUDevicePostClockSpeedChange() to end the hold off period
                to allow new work to be submitted to the GPU.

                Called from system layer or OS layer implementation that
                is responsible for triggering a GPU DVFS transition.

@Input          psDeviceNode pointer to the device affected by DVFS transition.
@Input          bIdleDevice  when True, the driver will wait for the GPU to
                             reach an idle state before the call returns.
@Input          pvInfo       unused

@Return         MTGPU_OK    on success, power lock acquired and held on exit,
                             GPU idle.
                MTGPU_ERROR on failure, power lock not held on exit, do not
                             call MTGPUDevicePostClockSpeedChange().
*/ /**************************************************************************/
MTGPU_ERROR MTGPUDevicePreClockSpeedChange(PMTGPU_DEVICE_NODE psDeviceNode,
											 MT_BOOL	bIdleDevice,
											 void	*pvInfo);

/**************************************************************************/ /*!
@Function       MTGPUDevicePostClockSpeedChange

@Description    This function is called after a voltage/frequency change has
                been made to the GPU HW following a call to
                MTGPUDevicePreClockSpeedChange().
                Before calling this function the caller must ensure the system
                data MUSA_DATA->MUSA_TIMING_INFORMATION->ui32CoreClockSpeed has
                been updated with the new frequency set, measured in Hz.
                The function informs the host driver that the DVFS change has
                completed. The driver will end the work hold off period, cancel
                the device idle period and update its time data records.
                When this call returns work submissions are unblocked and
                are submitted to the GPU as normal.
                This function *must* not be called if the preceding call to
                MTGPUDevicePreClockSpeedChange() failed.

                Called from system layer or OS layer implementation that
                is responsible for triggering a GPU DVFS transition.

@Input          psDeviceNode pointer to the device affected by DVFS transition.
@Input          bIdleDevice  when True, the driver will cancel the GPU
                             device idle state before the call returns. Value
                             given must match that used in the call to
                             MTGPUDevicePreClockSpeedChange() otherwise
                             undefined behaviour will result.
@Input          pvInfo       unused

@Return         void         power lock released, no longer held on exit.
*/ /**************************************************************************/
void MTGPUDevicePostClockSpeedChange(PMTGPU_DEVICE_NODE psDeviceNode,
									  MT_BOOL		bIdleDevice,
									  void		*pvInfo);

/*!
******************************************************************************

 @Function    MTGPUDeviceIdleRequestKM

 @Description Perform device-specific processing required to force the device
              idle. The device power-lock might be temporarily released (and
              again re-acquired) during the course of this call, hence to
              maintain lock-ordering power-lock should be the last acquired
              lock before calling this function

 @Input       psDeviceNode         : Device node

 @Input       pfnIsDefaultStateOff : When specified, the idle request is only
                                     processed if this function passes.

 @Input       bDeviceOffPermitted  : MT_TRUE if the transition should not fail
                                       if device off
                                     MT_FALSE if the transition should fail if
                                       device off

 @Return      MTGPU_ERROR_PWLOCK_RELEASED_REACQ_FAILED
                                     When re-acquisition of power-lock failed.
                                     This error NEEDS EXPLICIT HANDLING at call
                                     site as it signifies the caller needs to
                                     AVOID calling MTGPUPowerUnlock, since
                                     power-lock is no longer "possessed" by
                                     this context.

              MTGPU_OK              When idle request succeeded.
              MTGPU_ERROR           Other system errors.

******************************************************************************/
MTGPU_ERROR MTGPUDeviceIdleRequestKM(PMTGPU_DEVICE_NODE psDeviceNode,
					PFN_SYS_DEV_IS_DEFAULT_STATE_OFF	pfnIsDefaultStateOff,
					MT_BOOL				bDeviceOffPermitted);

/*!
******************************************************************************

 @Function	MTGPUDeviceIdleCancelRequestKM

 @Description Perform device-specific processing required to cancel the forced idle state
              on the device, returning to normal operation.

 @Input		psDeviceNode : Device node

 @Return	MTGPU_ERROR

******************************************************************************/
MTGPU_ERROR MTGPUDeviceIdleCancelRequestKM(PMTGPU_DEVICE_NODE psDeviceNode);

#endif /* POWER_H */

/******************************************************************************
 End of file (power.h)
******************************************************************************/

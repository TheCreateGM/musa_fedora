/*******************************************************************************
@File
@Title          Common bridge header for musaregconfig
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Declares common defines and structures used by both the client
                and server side of the bridge for musaregconfig
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

#ifndef COMMON_MUSAREGCONFIG_BRIDGE_H
#define COMMON_MUSAREGCONFIG_BRIDGE_H

#include <mt/mem_types.h>

#include "img_defs.h"
#include "img_types.h"
#include "mtgpu_error.h"

#define MTGPU_BRIDGE_MUSAREGCONFIG_CMD_FIRST			0
#define MTGPU_BRIDGE_MUSAREGCONFIG_MUSASETREGCONFIGTYPE			MTGPU_BRIDGE_MUSAREGCONFIG_CMD_FIRST+0
#define MTGPU_BRIDGE_MUSAREGCONFIG_MUSAADDREGCONFIG			MTGPU_BRIDGE_MUSAREGCONFIG_CMD_FIRST+1
#define MTGPU_BRIDGE_MUSAREGCONFIG_MUSACLEARREGCONFIG			MTGPU_BRIDGE_MUSAREGCONFIG_CMD_FIRST+2
#define MTGPU_BRIDGE_MUSAREGCONFIG_MUSAENABLEREGCONFIG			MTGPU_BRIDGE_MUSAREGCONFIG_CMD_FIRST+3
#define MTGPU_BRIDGE_MUSAREGCONFIG_MUSADISABLEREGCONFIG			MTGPU_BRIDGE_MUSAREGCONFIG_CMD_FIRST+4
#define MTGPU_BRIDGE_MUSAREGCONFIG_CMD_LAST			(MTGPU_BRIDGE_MUSAREGCONFIG_CMD_FIRST+4)

/*******************************************
            MUSASetRegConfigType
 *******************************************/

/* Bridge in structure for MUSASetRegConfigType */
typedef struct MTGPU_BRIDGE_IN_MUSASETREGCONFIGTYPE_TAG
{
	MT_UINT8 ui8RegPowerIsland;
} __packed MTGPU_BRIDGE_IN_MUSASETREGCONFIGTYPE;

/* Bridge out structure for MUSASetRegConfigType */
typedef struct MTGPU_BRIDGE_OUT_MUSASETREGCONFIGTYPE_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_MUSASETREGCONFIGTYPE;

/*******************************************
            MUSAAddRegconfig
 *******************************************/

/* Bridge in structure for MUSAAddRegconfig */
typedef struct MTGPU_BRIDGE_IN_MUSAADDREGCONFIG_TAG
{
	MT_UINT64 ui64RegMask;
	MT_UINT64 ui64RegValue;
	MT_UINT32 ui32RegAddr;
} __packed MTGPU_BRIDGE_IN_MUSAADDREGCONFIG;

/* Bridge out structure for MUSAAddRegconfig */
typedef struct MTGPU_BRIDGE_OUT_MUSAADDREGCONFIG_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_MUSAADDREGCONFIG;

/*******************************************
            MUSAClearRegConfig
 *******************************************/

/* Bridge in structure for MUSAClearRegConfig */
typedef struct MTGPU_BRIDGE_IN_MUSACLEARREGCONFIG_TAG
{
	MT_UINT32 ui32EmptyStructPlaceholder;
} __packed MTGPU_BRIDGE_IN_MUSACLEARREGCONFIG;

/* Bridge out structure for MUSAClearRegConfig */
typedef struct MTGPU_BRIDGE_OUT_MUSACLEARREGCONFIG_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_MUSACLEARREGCONFIG;

/*******************************************
            MUSAEnableRegConfig
 *******************************************/

/* Bridge in structure for MUSAEnableRegConfig */
typedef struct MTGPU_BRIDGE_IN_MUSAENABLEREGCONFIG_TAG
{
	MT_UINT32 ui32EmptyStructPlaceholder;
} __packed MTGPU_BRIDGE_IN_MUSAENABLEREGCONFIG;

/* Bridge out structure for MUSAEnableRegConfig */
typedef struct MTGPU_BRIDGE_OUT_MUSAENABLEREGCONFIG_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_MUSAENABLEREGCONFIG;

/*******************************************
            MUSADisableRegConfig
 *******************************************/

/* Bridge in structure for MUSADisableRegConfig */
typedef struct MTGPU_BRIDGE_IN_MUSADISABLEREGCONFIG_TAG
{
	MT_UINT32 ui32EmptyStructPlaceholder;
} __packed MTGPU_BRIDGE_IN_MUSADISABLEREGCONFIG;

/* Bridge out structure for MUSADisableRegConfig */
typedef struct MTGPU_BRIDGE_OUT_MUSADISABLEREGCONFIG_TAG
{
	MTGPU_ERROR eError;
} __packed MTGPU_BRIDGE_OUT_MUSADISABLEREGCONFIG;

#endif /* COMMON_MUSAREGCONFIG_BRIDGE_H */

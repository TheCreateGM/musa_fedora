/*************************************************************************/ /*!
@File
@Title          Header for Services abstraction layer
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Declaration of an interface layer used to abstract code that
                can be compiled outside of the DDK, potentially in a
                completely different OS.
                All the headers included by this file must also be copied to
                the alternative source tree.
                All the functions declared here must have a DDK implementation
                inside the DDK source tree (e.g. musalayer_impl.h/.c) and
                another different implementation in case they are used outside
                of the DDK.
                All of the functions accept as a first parameter a
                "const void *hPrivate" argument. It should be used to pass
                around any implementation specific data required.
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

#if !defined(MUSALAYER_H)
#define MUSALAYER_H

#if defined(__cplusplus)
extern "C" {
#endif


#include "img_defs.h"
#include "img_types.h"
#include "img_elf.h"
#include "mtgpu_error.h" /* includes mtgpu_errors.h */
#include "mt_device.h"
#include "musa_bvnc_defs_km.h"
#include "musa_fw_info.h"
#include "musa_fwif_shared.h" /* includes musa_common.h and mem_types.h */
#include "musa_meta.h"
#include "musa_riscv.h"

#include "musadefs_km.h"
/* includes:
 * musa_cr_defs_km.h,
 * MUSA_BVNC_CORE_KM_HEADER (musacore_km_B.V.N.C.h),
 * MUSA_BNC_CONFIG_KM_HEADER (musaconfig_km_B.V.N.C.h)
 */


/*!
*******************************************************************************

 @Function       MUSAMemCopy

 @Description    MemCopy implementation

 @Input          hPrivate   : Implementation specific data
 @Input          pvDst      : Pointer to the destination
 @Input          pvSrc      : Pointer to the source location
 @Input          uiSize     : The amount of memory to copy in bytes

 @Return         void

******************************************************************************/
void MUSAMemCopy(const void *hPrivate,
                void *pvDst,
                void *pvSrc,
                size_t uiSize);

/*!
*******************************************************************************

 @Function       MUSAMemSet

 @Description    MemSet implementation

 @Input          hPrivate   : Implementation specific data
 @Input          pvDst      : Pointer to the start of the memory region
 @Input          ui8Value   : The value to be written
 @Input          uiSize     : The number of bytes to be set to ui8Value

 @Return         void

******************************************************************************/
void MUSAMemSet(const void *hPrivate,
               void *pvDst,
               MT_UINT8 ui8Value,
               size_t uiSize);

/*!
*******************************************************************************

 @Function       MUSAGetMMUVersion

 @Description    Get mmu version

 @Input          hPrivate   : Implementation specific data

 @Return         MT_UINT32

******************************************************************************/
MT_UINT32 MUSAGetMMUVersion(const void *hPrivate);

/*!
*******************************************************************************

 @Function       MUSACommentLog

 @Description    Generic log function used for debugging or other purposes

 @Input          hPrivate   : Implementation specific data
 @Input          pszString  : Message to be printed
 @Input          ...        : Variadic arguments

 @Return         void

******************************************************************************/
__printf(2, 3)
void MUSACommentLog(const void *hPrivate,
                   const MT_CHAR *pszString,
                   ...);

/*!
*******************************************************************************

 @Function       MUSAErrorLog

 @Description    Generic error log function used for debugging or other purposes

 @Input          hPrivate   : Implementation specific data
 @Input          pszString  : Message to be printed
 @Input          ...        : Variadic arguments

 @Return         void

******************************************************************************/
__printf(2, 3)
void MUSAErrorLog(const void *hPrivate,
                 const MT_CHAR *pszString,
                 ...);

/* This is used to check if a specific feature is enabled.
 * Should be used instead of calling MUSADeviceHasFeature.  */
#define MUSA_DEVICE_HAS_FEATURE(hPrivate, Feature) \
			MUSADeviceHasFeature(hPrivate, MUSA_FEATURE_##Feature##_BIT_MASK)

/* This is used to check if a specific feature with value is enabled.
 * Should be used instead of calling MUSADeviceGetFeatureValue.  */
#define MUSA_DEVICE_HAS_FEATURE_VALUE(hPrivate, Feature) \
			(MUSADeviceGetFeatureValue(hPrivate, MUSA_FEATURE_##Feature##_IDX) >= 0)

/* This is used to get the value of a specific feature from hPrivate.
 * Should be used instead of calling MUSADeviceGetFeatureValue.  */
#define MUSA_DEVICE_GET_FEATURE_VALUE(hPrivate, Feature) \
			MUSADeviceGetFeatureValue(hPrivate, MUSA_FEATURE_##Feature##_IDX)

/* This is used to get the value of a specific ERN from hPrivate.
 * Should be used instead of calling MUSADeviceHasErnBrn.  */
#define MUSA_DEVICE_HAS_ERN(hPrivate, FixNum) \
			MUSADeviceHasErnBrn(hPrivate, HW_##FixNum##_BIT_MASK)

/* This is used to get the value of a specific BRN from hPrivate.
 * Should be used instead of calling MUSADeviceHasErnBrn.  */
#define MUSA_DEVICE_HAS_BRN(hPrivate, FixNum) \
			MUSADeviceHasErnBrn(hPrivate, FIX_HW_##FixNum##_BIT_MASK)

#define CLK_CTRL_FORCE_ON(X, Module) \
			X = (((X) & MUSA_CR_##Module##_CLRMSK) | MUSA_CR_##Module##_ON)
/*!
*******************************************************************************

 @Function       MUSADeviceGetFeatureValue

 @Description    Checks if a device has a particular feature with values

 @Input          hPrivate     : Implementation specific data
 @Input          ui64Feature  : Feature with values to check

 @Return         Value >= 0 if the given feature is available, -1 otherwise

******************************************************************************/
MT_INT32 MUSADeviceGetFeatureValue(const void *hPrivate, MT_UINT64 ui64Feature);

/*!
*******************************************************************************

 @Function       MUSADeviceHasFeature

 @Description    Checks if a device has a particular feature

 @Input          hPrivate     : Implementation specific data
 @Input          ui64Feature  : Feature to check

 @Return         MT_TRUE if the given feature is available, MT_FALSE otherwise

******************************************************************************/
MT_BOOL MUSADeviceHasFeature(const void *hPrivate, MT_UINT64 ui64Feature);

/*!
*******************************************************************************

 @Function       MUSADeviceHasErnBrn

 @Description    Checks if a device has a particular errata

 @Input          hPrivate     : Implementation specific data
 @Input          ui64ErnsBrns : Flags to check

 @Return         MT_TRUE if the given errata is available, MT_FALSE otherwise

******************************************************************************/
MT_BOOL MUSADeviceHasErnBrn(const void *hPrivate, MT_UINT64 ui64ErnsBrns);

/*!
*******************************************************************************

 @Function       MUSAGetFWCorememSize

 @Description    Get the FW coremem size

 @Input          hPrivate   : Implementation specific data

 @Return         FW coremem size

******************************************************************************/
MT_INTERNAL
MT_UINT32 MUSAGetFWCorememSize(const void *hPrivate);

/*!
*******************************************************************************

 @Function      MUSAWriteReg32/64

 @Description   Write a value to a 32/64 bit MUSA register

 @Input         hPrivate         : Implementation specific data
 @Input         ui32RegAddr      : Register offset inside the register bank
 @Input         ui32/64RegValue  : New register value

 @Return        void

******************************************************************************/
void MUSAWriteReg32(const void *hPrivate,
                   MT_UINT32 ui32RegAddr,
                   MT_UINT32 ui32RegValue);

void MUSAWriteReg64(const void *hPrivate,
                   MT_UINT32 ui32RegAddr,
                   MT_UINT64 ui64RegValue);

/*!
*******************************************************************************

 @Function       MUSAReadReg32/64

 @Description    Read a 32/64 bit MUSA register

 @Input          hPrivate     : Implementation specific data
 @Input          ui32RegAddr  : Register offset inside the register bank

 @Return         Register value

******************************************************************************/
MT_UINT32 MUSAReadReg32(const void *hPrivate,
                        MT_UINT32 ui32RegAddr);

MT_UINT64 MUSAReadReg64(const void *hPrivate,
                        MT_UINT32 ui32RegAddr);

/*!
*******************************************************************************

 @Function       MUSAReadModifyWriteReg32

 @Description    Read-modify-write a 32 bit MUSA register

 @Input          hPrivate     : Implementation specific data.
 @Input          ui32RegAddr  : Register offset inside the register bank.
 @Input          ui32RegValue : New register value.
 @Input          ui32RegMask  : Keep the bits set in the mask.

 @Return         Always returns MTGPU_OK

******************************************************************************/
MT_UINT32 MUSAReadModifyWriteReg64(const void *hPrivate,
                                   MT_UINT32 ui32RegAddr,
                                   MT_UINT64 ui64RegValue,
                                   MT_UINT64 ui64RegKeepMask);

/*!
*******************************************************************************

 @Function       MUSAPollReg32/64

 @Description    Poll on a 32/64 bit MUSA register until some bits are set/unset

 @Input          hPrivate         : Implementation specific data
 @Input          ui32RegAddr      : Register offset inside the register bank
 @Input          ui32/64RegValue  : Value expected from the register
 @Input          ui32/64RegMask   : Only the bits set in this mask will be
                                    checked against uiRegValue

 @Return         MTGPU_OK if the poll succeeds,
                 MTGPU_ERROR_TIMEOUT if the poll takes too long

******************************************************************************/
MTGPU_ERROR MUSAPollReg32(const void *hPrivate,
                          MT_UINT32 ui32RegAddr,
                          MT_UINT32 ui32RegValue,
                          MT_UINT32 ui32RegMask);

MTGPU_ERROR MUSAPollReg64(const void *hPrivate,
                          MT_UINT32 ui32RegAddr,
                          MT_UINT64 ui64RegValue,
                          MT_UINT64 ui64RegMask);

/*!
*******************************************************************************

 @Function       MUSAWaitCycles

 @Description    Wait for a number of GPU cycles and/or microseconds

 @Input          hPrivate    : Implementation specific data
 @Input          ui32Cycles  : Number of GPU cycles to wait for in pdumps,
                               it can also be used when running driver-live
                               if desired (ignoring the next parameter)
 @Input          ui32WaitUs  : Number of microseconds to wait for when running
                               driver-live

 @Return         void

******************************************************************************/
void MUSAWaitCycles(const void *hPrivate,
                   MT_UINT32 ui32Cycles,
                   MT_UINT32 ui32WaitUs);

/*!
*******************************************************************************

 @Function        MUSAAcquireKernelMMUPC

 @Description     Acquire the Kernel MMU Page Catalogue device physical address

 @Input           hPrivate  : Implementation specific data
 @Input           psPCAddr  : Returned page catalog address

 @Return          void

******************************************************************************/
void MUSAAcquireKernelMMUPC(const void *hPrivate, MT_DEV_PHYADDR *psPCAddr);

/*!
*******************************************************************************

 @Function        MUSAWriteKernelMMUPC32/64

 @Description     Write the Kernel MMU Page Catalogue to the 32/64 bit
                  MUSA register passed as argument.
                  In a driver-live scenario without PDump these functions
                  are the same as MUSAWriteReg32/64 and they don't need
                  to be reimplemented.

 @Input           hPrivate        : Implementation specific data
 @Input           ui32PCReg       : Register offset inside the register bank
 @Input           ui32AlignShift  : PC register alignshift
 @Input           ui32Shift       : PC register shift
 @Input           ui32/64PCVal    : Page catalog value (aligned and shifted)

 @Return          void

******************************************************************************/
#define MUSAWriteKernelMMUPC32(priv, pcreg, alignshift, shift, pcval) \
	MUSAWriteReg32(priv, pcreg, pcval)

/*!
*******************************************************************************

 @Function        MUSAWriteKernelMMUPC32/64

 @Description     Write the Kernel MMU Page Catalogue to the 32/64 bit
                  MUSA register passed as argument.
                  In a driver-live scenario without PDump these functions
                  are the same as MUSAWriteReg32/64 and they don't need
                  to be reimplemented.

 @Input           hPrivate        : Implementation specific data
 @Input           ui32PCReg       : Register offset inside the register bank
 @Input           ui32AlignShift  : PC register alignshift
 @Input           ui32Shift       : PC register shift
 @Input           ui32/64PCVal    : Page catalog value (aligned and shifted)

 @Return          void

******************************************************************************/
#define MUSAWriteKernelMMUPC64(priv, pcreg, alignshift, shift, pcval) \
        MUSAWriteReg64(priv, pcreg, pcval)

/*!
*******************************************************************************

 @Function        MUSADoFWSlaveBoot

 @Description     Returns whether or not a FW Slave Boot is required
                  while powering on

 @Input           hPrivate       : Implementation specific data

 @Return          MT_BOOL

******************************************************************************/
MT_BOOL MUSADoFWSlaveBoot(const void *hPrivate);

/*!
*******************************************************************************

 @Function       MUSAPartitionCoherencyTest

 @Description    Performs partition coherency test

 @Input          hPrivate         : Implementation specific data

 @Return         MTGPU_OK if the test succeeds,
                 MTGPU_ERROR_INIT_FAILURE if the test fails at some point

******************************************************************************/
MTGPU_ERROR MUSAPartitionCoherencyTest(const void *hPrivate);

/*!
*******************************************************************************

 @Function       MUSAGetDeviceL2Banks

 @Description    Returns the number of L2 banks used by the device

 @Input          hPrivate    : Implementation specific data

 @Return         Number of L2 banks

******************************************************************************/
MT_UINT32 MUSAGetDeviceL2Banks(const void *hPrivate);

/*!
*******************************************************************************

 @Function       MUSAGetDeviceCacheLineSize

 @Description    Returns the device cache line size

 @Input          hPrivate    : Implementation specific data

 @Return         Cache line size

******************************************************************************/
MT_UINT32 MUSAGetDeviceCacheLineSize(const void *hPrivate);

/*!
*******************************************************************************

 @Function        MUSAAcquireBootCodeAddr

 @Description     Acquire the device virtual address of the RISCV boot code

 @Input           hPrivate         : Implementation specific data
 @Output          psBootCodeAddr   : Boot code base address

 @Return          void

******************************************************************************/
void MUSAAcquireBootCodeAddr(const void *hPrivate, uint64_t *psBootCodeAddr);

/*!
*******************************************************************************

 @Function        MUSAAcquireBootDataAddr

 @Description     Acquire the device virtual address of the RISCV boot data

 @Input           hPrivate         : Implementation specific data
 @Output          psBootDataAddr   : Boot data base address

 @Return          void

******************************************************************************/
void MUSAAcquireBootDataAddr(const void *hPrivate, uint64_t *psBootDataAddr);

/*!
*******************************************************************************

 @Function      MUSADeviceAckIrq

 @Description   Checks the implementation specific IRQ status register,
                clearing it if necessary and returning the IRQ status.

 @Input          hPrivate    : Implementation specific data

 @Return:       IRQ status

******************************************************************************/
MT_BOOL MUSADeviceAckIrq(const void *hPrivate);

#if defined(__cplusplus)
}
#endif

#endif /* MUSALAYER_H */

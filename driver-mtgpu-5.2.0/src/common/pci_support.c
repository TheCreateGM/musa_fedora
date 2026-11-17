/*************************************************************************/ /*!
@File
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

#include <linux/version.h>
#include <linux/pci.h>

#if defined(CONFIG_MTRR)
#include <asm/mtrr.h>
#endif

#include "pci_support.h"
#include "allocmem.h"

typedef	struct _MT_PCI_DEV_TAG
{
	struct pci_dev		*psPCIDev;
	HOST_PCI_INIT_FLAGS	ePCIFlags;
	MT_BOOL		abPCIResourceInUse[DEVICE_COUNT_RESOURCE];
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 1, 0))
	int			iMTRR[DEVICE_COUNT_RESOURCE];
#endif
} MT_PCI_DEV;

/*************************************************************************/ /*!
@Function       OSPCISetDev
@Description    Set a PCI device for subsequent use.
@Input          pvPCICookie             Pointer to OS specific PCI structure
@Input          eFlags                  Flags
@Return		MTGPU_PCI_DEV_HANDLE   Pointer to PCI device handle
*/ /**************************************************************************/
MTGPU_PCI_DEV_HANDLE OSPCISetDev(void *pvPCICookie, HOST_PCI_INIT_FLAGS eFlags)
{
	int err;
	MT_UINT32 i;
	MT_PCI_DEV *psMTPCI;

	psMTPCI = OSAllocMem(sizeof(*psMTPCI));
	if (psMTPCI == NULL)
	{
		printk(KERN_ERR "OSPCISetDev: Couldn't allocate MTGPU PCI structure\n");
		return NULL;
	}

	psMTPCI->psPCIDev = (struct pci_dev *)pvPCICookie;
	psMTPCI->ePCIFlags = eFlags;

	err = pci_enable_device(psMTPCI->psPCIDev);
	if (err != 0)
	{
		printk(KERN_ERR "OSPCISetDev: Couldn't enable device (%d)\n", err);
		OSFreeMem(psMTPCI);
		return NULL;
	}

	if (psMTPCI->ePCIFlags & HOST_PCI_INIT_FLAG_BUS_MASTER)	/* PRQA S 3358 */ /* misuse of enums */
	{
		pci_set_master(psMTPCI->psPCIDev);
	}

	if (psMTPCI->ePCIFlags & HOST_PCI_INIT_FLAG_MSI)		/* PRQA S 3358 */ /* misuse of enums */
	{
#if defined(CONFIG_PCI_MSI)
		err = pci_enable_msi(psMTPCI->psPCIDev);
		if (err != 0)
		{
			printk(KERN_ERR "OSPCISetDev: Couldn't enable MSI (%d)", err);
			psMTPCI->ePCIFlags &= ~HOST_PCI_INIT_FLAG_MSI;	/* PRQA S 1474,3358,4130 */ /* misuse of enums */
		}
#else
		printk(KERN_ERR "OSPCISetDev: MSI support not enabled in the kernel");
#endif
	}

	/* Initialise the PCI resource and MTRR tracking array */
	for (i = 0; i < DEVICE_COUNT_RESOURCE; i++)
	{
		psMTPCI->abPCIResourceInUse[i] = MT_FALSE;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 1, 0))
		psMTPCI->iMTRR[i] = -1;
#endif
	}

	return (MTGPU_PCI_DEV_HANDLE)psMTPCI;
}

/*************************************************************************/ /*!
@Function       OSPCIAcquireDev
@Description    Acquire a PCI device for subsequent use.
@Input          ui16VendorID            Vendor PCI ID
@Input          ui16DeviceID            Device PCI ID
@Input          eFlags                  Flags
@Return		MTGPU_PCI_DEV_HANDLE   Pointer to PCI device handle
*/ /**************************************************************************/
MTGPU_PCI_DEV_HANDLE OSPCIAcquireDev(MT_UINT16 ui16VendorID,
				      MT_UINT16 ui16DeviceID,
				      HOST_PCI_INIT_FLAGS eFlags)
{
	struct pci_dev *psPCIDev;

	psPCIDev = pci_get_device(ui16VendorID, ui16DeviceID, NULL);
	if (psPCIDev == NULL)
	{
		return NULL;
	}

	return OSPCISetDev((void *)psPCIDev, eFlags);
}

/*************************************************************************/ /*!
@Function       OSPCIIRQ
@Description    Get the interrupt number for the device.
@Input          hMTPCI                 PCI device handle
@Output         pui16DeviceID           Pointer to where the interrupt number
                                        should be returned
@Return		MTGPU_ERROR            Services error code
*/ /**************************************************************************/
MTGPU_ERROR OSPCIIRQ(MTGPU_PCI_DEV_HANDLE hMTPCI, MT_UINT32 *pui32IRQ)
{
	MT_PCI_DEV *psMTPCI = (MT_PCI_DEV *)hMTPCI;

	if (pui32IRQ == NULL)
	{
		return MTGPU_ERROR_INVALID_PARAMS;
	}

	*pui32IRQ = psMTPCI->psPCIDev->irq;

	return MTGPU_OK;
}

/* Functions supported by OSPCIAddrRangeFunc */
enum HOST_PCI_ADDR_RANGE_FUNC
{
	HOST_PCI_ADDR_RANGE_FUNC_LEN,
	HOST_PCI_ADDR_RANGE_FUNC_START,
	HOST_PCI_ADDR_RANGE_FUNC_END,
	HOST_PCI_ADDR_RANGE_FUNC_REQUEST,
	HOST_PCI_ADDR_RANGE_FUNC_RELEASE
};

/*************************************************************************/ /*!
@Function       OSPCIAddrRangeFunc
@Description    Internal support function for various address range related
                functions
@Input          eFunc                   Function to perform
@Input          hMTPCI                 PCI device handle
@Input          ui32Index               Address range index
@Return		MT_UINT32              Function dependent value
*/ /**************************************************************************/
static MT_UINT64 OSPCIAddrRangeFunc(enum HOST_PCI_ADDR_RANGE_FUNC eFunc,
										 MTGPU_PCI_DEV_HANDLE hMTPCI,
										 MT_UINT32 ui32Index)
{
	MT_PCI_DEV *psMTPCI = (MT_PCI_DEV *)hMTPCI;

	if (ui32Index >= DEVICE_COUNT_RESOURCE)
	{
		printk(KERN_ERR "OSPCIAddrRangeFunc: Index out of range");
		return 0;
	}

	switch (eFunc)
	{
		case HOST_PCI_ADDR_RANGE_FUNC_LEN:
		{
			return pci_resource_len(psMTPCI->psPCIDev, ui32Index);
		}
		case HOST_PCI_ADDR_RANGE_FUNC_START:
		{
			return pci_resource_start(psMTPCI->psPCIDev, ui32Index);
		}
		case HOST_PCI_ADDR_RANGE_FUNC_END:
		{
			return pci_resource_end(psMTPCI->psPCIDev, ui32Index);
		}
		case HOST_PCI_ADDR_RANGE_FUNC_REQUEST:
		{
			int err = pci_request_region(psMTPCI->psPCIDev, (MT_INT)ui32Index, MTGPU_MODNAME);
			if (err != 0)
			{
				printk(KERN_ERR "OSPCIAddrRangeFunc: pci_request_region_failed (%d)", err);
				return 0;
			}
			psMTPCI->abPCIResourceInUse[ui32Index] = MT_TRUE;
			return 1;
		}
		case HOST_PCI_ADDR_RANGE_FUNC_RELEASE:
		{
			if (psMTPCI->abPCIResourceInUse[ui32Index])
			{
				pci_release_region(psMTPCI->psPCIDev, (MT_INT)ui32Index);
				psMTPCI->abPCIResourceInUse[ui32Index] = MT_FALSE;
			}
			return 1;
		}
		default:
		{
			printk(KERN_ERR "OSPCIAddrRangeFunc: Unknown function");
			break;
		}
	}

	return 0;
}

/*************************************************************************/ /*!
@Function       OSPCIAddrRangeLen
@Description    Returns length of a given address range
@Input          hMTPCI                 PCI device handle
@Input          ui32Index               Address range index
@Return		MT_UINT32              Length of address range or 0 if no
                                        such range
*/ /**************************************************************************/
MT_UINT64 OSPCIAddrRangeLen(MTGPU_PCI_DEV_HANDLE hMTPCI, MT_UINT32 ui32Index)
{
	return OSPCIAddrRangeFunc(HOST_PCI_ADDR_RANGE_FUNC_LEN, hMTPCI, ui32Index);
}

/*************************************************************************/ /*!
@Function       OSPCIAddrRangeStart
@Description    Returns the start of a given address range
@Input          hMTPCI                 PCI device handle
@Input          ui32Index               Address range index
@Return		MT_UINT32              Start of address range or 0 if no
                                        such range
*/ /**************************************************************************/
MT_UINT64 OSPCIAddrRangeStart(MTGPU_PCI_DEV_HANDLE hMTPCI, MT_UINT32 ui32Index)
{
	return OSPCIAddrRangeFunc(HOST_PCI_ADDR_RANGE_FUNC_START, hMTPCI, ui32Index);
}

/*************************************************************************/ /*!
@Function       OSPCIAddrRangeEnd
@Description    Returns the end of a given address range
@Input          hMTPCI                 PCI device handle
@Input          ui32Index               Address range index
@Return		MT_UINT32              End of address range or 0 if no such
                                        range
*/ /**************************************************************************/
MT_UINT64 OSPCIAddrRangeEnd(MTGPU_PCI_DEV_HANDLE hMTPCI, MT_UINT32 ui32Index)
{
	return OSPCIAddrRangeFunc(HOST_PCI_ADDR_RANGE_FUNC_END, hMTPCI, ui32Index);
}

/*************************************************************************/ /*!
@Function       OSPCIRequestAddrRange
@Description    Request a given address range index for subsequent use
@Input          hMTPCI                 PCI device handle
@Input          ui32Index               Address range index
@Return	        MTGPU_ERROR	        Services error code
*/ /**************************************************************************/
MTGPU_ERROR OSPCIRequestAddrRange(MTGPU_PCI_DEV_HANDLE hMTPCI,
								   MT_UINT32 ui32Index)
{
	if (OSPCIAddrRangeFunc(HOST_PCI_ADDR_RANGE_FUNC_REQUEST, hMTPCI, ui32Index) == 0)
	{
		return MTGPU_ERROR_PCI_CALL_FAILED;
	}
	else
	{
		return MTGPU_OK;
	}
}

/*************************************************************************/ /*!
@Function       OSPCIReleaseAddrRange
@Description    Release a given address range that is no longer being used
@Input          hMTPCI                 PCI device handle
@Input          ui32Index               Address range index
@Return	        MTGPU_ERROR	        Services error code
*/ /**************************************************************************/
MTGPU_ERROR OSPCIReleaseAddrRange(MTGPU_PCI_DEV_HANDLE hMTPCI, MT_UINT32 ui32Index)
{
	if (OSPCIAddrRangeFunc(HOST_PCI_ADDR_RANGE_FUNC_RELEASE, hMTPCI, ui32Index) == 0)
	{
		return MTGPU_ERROR_PCI_CALL_FAILED;
	}
	else
	{
		return MTGPU_OK;
	}
}

/*************************************************************************/ /*!
@Function       OSPCIRequestAddrRegion
@Description    Request a given region from an address range for subsequent use
@Input          hMTPCI                 PCI device handle
@Input          ui32Index               Address range index
@Input          uiOffset              Offset into the address range that forms
                                        the start of the region
@Input          uiLength              Length of the region
@Return	        MTGPU_ERROR	        Services error code
*/ /**************************************************************************/
MTGPU_ERROR OSPCIRequestAddrRegion(MTGPU_PCI_DEV_HANDLE hMTPCI,
									MT_UINT32 ui32Index,
									MT_UINT64 uiOffset,
									MT_UINT64 uiLength)
{
	MT_PCI_DEV *psMTPCI = (MT_PCI_DEV *)hMTPCI;
	resource_size_t start;
	resource_size_t end;

	start = pci_resource_start(psMTPCI->psPCIDev, ui32Index);
	end = pci_resource_end(psMTPCI->psPCIDev, ui32Index);

	/* Check that the requested region is valid */
	if ((start + uiOffset + uiLength - 1) > end)
	{
		return MTGPU_ERROR_BAD_REGION_SIZE_MISMATCH;
	}

	if (pci_resource_flags(psMTPCI->psPCIDev, ui32Index) & IORESOURCE_IO)
	{
		if (request_region(start + uiOffset, uiLength, MTGPU_MODNAME) == NULL)
		{
			return MTGPU_ERROR_PCI_REGION_UNAVAILABLE;
		}
	}
	else
	{
		if (request_mem_region(start + uiOffset, uiLength, MTGPU_MODNAME) == NULL)
		{
			return MTGPU_ERROR_PCI_REGION_UNAVAILABLE;
		}
	}

	return MTGPU_OK;
}

/*************************************************************************/ /*!
@Function       OSPCIReleaseAddrRegion
@Description    Release a given region, from an address range, that is no
                longer in use
@Input          hMTPCI                 PCI device handle
@Input          ui32Index               Address range index
@Input          ui32Offset              Offset into the address range that forms
                                        the start of the region
@Input          ui32Length              Length of the region
@Return	        MTGPU_ERROR	        Services error code
*/ /**************************************************************************/
MTGPU_ERROR OSPCIReleaseAddrRegion(MTGPU_PCI_DEV_HANDLE hMTPCI,
									MT_UINT32 ui32Index,
									MT_UINT64 uiOffset,
									MT_UINT64 uiLength)
{
	MT_PCI_DEV *psMTPCI = (MT_PCI_DEV *)hMTPCI;
	resource_size_t start;
	resource_size_t end;

	start = pci_resource_start(psMTPCI->psPCIDev, ui32Index);
	end = pci_resource_end(psMTPCI->psPCIDev, ui32Index);

	/* Check that the region is valid */
	if ((start + uiOffset + uiLength - 1) > end)
	{
		return MTGPU_ERROR_BAD_REGION_SIZE_MISMATCH;
	}

	if (pci_resource_flags(psMTPCI->psPCIDev, ui32Index) & IORESOURCE_IO)
	{
		release_region(start + uiOffset, uiLength);
	}
	else
	{
		release_mem_region(start + uiOffset, uiLength);
	}

	return MTGPU_OK;
}

/*************************************************************************/ /*!
@Function       OSPCIReleaseDev
@Description    Release a PCI device that is no longer being used
@Input          hMTPCI                 PCI device handle
@Return	        MTGPU_ERROR	        Services error code
*/ /**************************************************************************/
MTGPU_ERROR OSPCIReleaseDev(MTGPU_PCI_DEV_HANDLE hMTPCI)
{
	MT_PCI_DEV *psMTPCI = (MT_PCI_DEV *)hMTPCI;
	int i;

	/* Release all PCI regions that are currently in use */
	for (i = 0; i < DEVICE_COUNT_RESOURCE; i++)
	{
		if (psMTPCI->abPCIResourceInUse[i])
		{
			pci_release_region(psMTPCI->psPCIDev, i);
			psMTPCI->abPCIResourceInUse[i] = MT_FALSE;
		}
	}

#if defined(CONFIG_PCI_MSI)
	if (psMTPCI->ePCIFlags & HOST_PCI_INIT_FLAG_MSI)		/* PRQA S 3358 */ /* misuse of enums */
	{
		pci_disable_msi(psMTPCI->psPCIDev);
	}
#endif

	if (psMTPCI->ePCIFlags & HOST_PCI_INIT_FLAG_BUS_MASTER)	/* PRQA S 3358 */ /* misuse of enums */
	{
		pci_clear_master(psMTPCI->psPCIDev);
	}

	pci_disable_device(psMTPCI->psPCIDev);

	OSFreeMem(psMTPCI);
	/*not nulling pointer, copy on stack*/

	return MTGPU_OK;
}

/*************************************************************************/ /*!
@Function       OSPCISuspendDev
@Description    Prepare PCI device to be turned off by power management
@Input          hMTPCI                 PCI device handle
@Return	        MTGPU_ERROR	        Services error code
*/ /**************************************************************************/
MTGPU_ERROR OSPCISuspendDev(MTGPU_PCI_DEV_HANDLE hMTPCI)
{
	MT_PCI_DEV *psMTPCI = (MT_PCI_DEV *)hMTPCI;
	int i;
	int err;

	/* Release all PCI regions that are currently in use */
	for (i = 0; i < DEVICE_COUNT_RESOURCE; i++)
	{
		if (psMTPCI->abPCIResourceInUse[i])
		{
			pci_release_region(psMTPCI->psPCIDev, i);
		}
	}

	err = pci_save_state(psMTPCI->psPCIDev);
	if (err != 0)
	{
		printk(KERN_ERR "OSPCISuspendDev: pci_save_state_failed (%d)", err);
		return MTGPU_ERROR_PCI_CALL_FAILED;
	}

	pci_disable_device(psMTPCI->psPCIDev);

	err = pci_set_power_state(psMTPCI->psPCIDev, pci_choose_state(psMTPCI->psPCIDev, PMSG_SUSPEND));
	switch (err)
	{
		case 0:
			break;
		case -EIO:
			printk(KERN_ERR "OSPCISuspendDev: device doesn't support PCI PM");
			break;
		case -EINVAL:
			printk(KERN_ERR "OSPCISuspendDev: can't enter requested power state");
			break;
		default:
			printk(KERN_ERR "OSPCISuspendDev: pci_set_power_state failed (%d)", err);
			break;
	}

	return MTGPU_OK;
}

/*************************************************************************/ /*!
@Function       OSPCIResumeDev
@Description    Prepare a PCI device to be resumed by power management
@Input          hMTPCI                 PCI device handle
@Return	        MTGPU_ERROR	        Services error code
*/ /**************************************************************************/
MTGPU_ERROR OSPCIResumeDev(MTGPU_PCI_DEV_HANDLE hMTPCI)
{
	MT_PCI_DEV *psMTPCI = (MT_PCI_DEV *)hMTPCI;
	int err;
	int i;

	err = pci_set_power_state(psMTPCI->psPCIDev, pci_choose_state(psMTPCI->psPCIDev, PMSG_ON));
	switch (err)
	{
		case 0:
			break;
		case -EIO:
			printk(KERN_ERR "OSPCIResumeDev: device doesn't support PCI PM");
			break;
		case -EINVAL:
			printk(KERN_ERR "OSPCIResumeDev: can't enter requested power state");
			return MTGPU_ERROR_UNKNOWN_POWER_STATE;
		default:
			printk(KERN_ERR "OSPCIResumeDev: pci_set_power_state failed (%d)", err);
			return MTGPU_ERROR_UNKNOWN_POWER_STATE;
	}

	pci_restore_state(psMTPCI->psPCIDev);

	err = pci_enable_device(psMTPCI->psPCIDev);
	if (err != 0)
	{
		printk(KERN_ERR "OSPCIResumeDev: Couldn't enable device (%d)", err);
		return MTGPU_ERROR_PCI_CALL_FAILED;
	}

	if (psMTPCI->ePCIFlags & HOST_PCI_INIT_FLAG_BUS_MASTER)	/* PRQA S 3358 */ /* misuse of enums */
		pci_set_master(psMTPCI->psPCIDev);

	/* Restore the PCI resource tracking array */
	for (i = 0; i < DEVICE_COUNT_RESOURCE; i++)
	{
		if (psMTPCI->abPCIResourceInUse[i])
		{
			err = pci_request_region(psMTPCI->psPCIDev, i, MTGPU_MODNAME);
			if (err != 0)
			{
				printk(KERN_ERR "OSPCIResumeDev: pci_request_region_failed (region %d, error %d)", i, err);
			}
		}
	}

	return MTGPU_OK;
}

/*************************************************************************/ /*!
@Function       OSPCIGetVendorDeviceIDs
@Description    Retrieve PCI vendor ID and device ID.
@Input          hMTPCI                 PCI device handle
@Output         pui16VendorID           Vendor ID
@Output         pui16DeviceID           Device ID
@Return         MTGPU_ERROR            Services error code
*/ /**************************************************************************/
MTGPU_ERROR OSPCIGetVendorDeviceIDs(MTGPU_PCI_DEV_HANDLE hMTPCI,
                                     MT_UINT16 *pui16VendorID,
                                     MT_UINT16 *pui16DeviceID)
{
	MT_PCI_DEV *psMTPCI = (MT_PCI_DEV *)hMTPCI;
	struct pci_dev *psPCIDev;

	if (psMTPCI == NULL)
	{
		return MTGPU_ERROR_INVALID_PARAMS;
	}

	psPCIDev = psMTPCI->psPCIDev;
	if (psPCIDev == NULL)
	{
		return MTGPU_ERROR_INVALID_PARAMS;
	}

	*pui16VendorID = psPCIDev->vendor;
	*pui16DeviceID = psPCIDev->device;

	return MTGPU_OK;
}

#if defined(CONFIG_MTRR)

/*************************************************************************/ /*!
@Function       OSPCIClearResourceMTRRs
@Description    Clear any BIOS-configured MTRRs for a PCI memory region
@Input          hMTPCI                 PCI device handle
@Input          ui32Index               Address range index
@Return	        MTGPU_ERROR	        Services error code
*/ /**************************************************************************/
MTGPU_ERROR OSPCIClearResourceMTRRs(MTGPU_PCI_DEV_HANDLE hMTPCI, MT_UINT32 ui32Index)
{
	MT_PCI_DEV *psMTPCI = (MT_PCI_DEV *)hMTPCI;
	resource_size_t start, end;
	int res;

	start = pci_resource_start(psMTPCI->psPCIDev, ui32Index);
	end = pci_resource_end(psMTPCI->psPCIDev, ui32Index) + 1;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 1, 0))
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 9, 0))
	res = arch_io_reserve_memtype_wc(start, end - start);
	if (res)
	{
		return MTGPU_ERROR_PCI_CALL_FAILED;
	}
#endif
	res = arch_phys_wc_add(start, end - start);
	if (res < 0)
	{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 9, 0))
		arch_io_free_memtype_wc(start, end - start);
#endif

		return MTGPU_ERROR_PCI_CALL_FAILED;
	}
	psMTPCI->iMTRR[ui32Index] = res;
#else

	res = mtrr_add(start, end - start, MTRR_TYPE_UNCACHABLE, 0);
	if (res < 0)
	{
		printk(KERN_ERR "OSPCIClearResourceMTRRs: mtrr_add failed (%d)", res);
		return MTGPU_ERROR_PCI_CALL_FAILED;
	}

	res = mtrr_del(res, start, end - start);
	if (res < 0)
	{
		printk(KERN_ERR "OSPCIClearResourceMTRRs: mtrr_del failed (%d)", res);
		return MTGPU_ERROR_PCI_CALL_FAILED;
	}

	/* Workaround for overlapping MTRRs. */
	{
		MT_BOOL bGotMTRR0 = MT_FALSE;

		/* Current mobo BIOSes will normally set up a WRBACK MTRR spanning
		 * 0->4GB, and then another 4GB->6GB. If the PCI card's automatic &
		 * overlapping UNCACHABLE MTRR is deleted, we see WRBACK behaviour.
		 *
		 * WRBACK is incompatible with some PCI devices, so try to split
		 * the UNCACHABLE regions up and insert a WRCOMB region instead.
		 */
		res = mtrr_add(start, end - start, MTRR_TYPE_WRBACK, 0);
		if (res < 0)
		{
			/* If this fails, services has probably run before and created
			 * a write-combined MTRR for the test chip. Assume it has, and
			 * don't return an error here.
			 */
			return MTGPU_OK;
		}

		if (res == 0)
			bGotMTRR0 = MT_TRUE;

		res = mtrr_del(res, start, end - start);
		if (res < 0)
		{
			printk(KERN_ERR "OSPCIClearResourceMTRRs: mtrr_del failed (%d)", res);
			return MTGPU_ERROR_PCI_CALL_FAILED;
		}

		if (bGotMTRR0)
		{
			/* Replace 0 with a non-overlapping WRBACK MTRR */
			res = mtrr_add(0, start, MTRR_TYPE_WRBACK, 0);
			if (res < 0)
			{
				printk(KERN_ERR "OSPCIClearResourceMTRRs: mtrr_add failed (%d)", res);
				return MTGPU_ERROR_PCI_CALL_FAILED;
			}

			/* Add a WRCOMB MTRR for the PCI device memory bar */
			res = mtrr_add(start, end - start, MTRR_TYPE_WRCOMB, 0);
			if (res < 0)
			{
				printk(KERN_ERR "OSPCIClearResourceMTRRs: mtrr_add failed (%d)", res);
				return MTGPU_ERROR_PCI_CALL_FAILED;
			}
		}
	}
#endif

	return MTGPU_OK;
}

/*************************************************************************/ /*!
@Function       OSPCIReleaseResourceMTRRs
@Description    Release resources allocated by OSPCIClearResourceMTRRs
@Input          hMTPCI                 PCI device handle
@Input          ui32Index               Address range index
*/ /**************************************************************************/
void OSPCIReleaseResourceMTRRs(MTGPU_PCI_DEV_HANDLE hMTPCI, MT_UINT32 ui32Index)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 1, 0))
	MT_PCI_DEV *psMTPCI = (MT_PCI_DEV *)hMTPCI;

	if (psMTPCI->iMTRR[ui32Index] >= 0)
	{
		arch_phys_wc_del(psMTPCI->iMTRR[ui32Index]);
		psMTPCI->iMTRR[ui32Index] = -1;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 9, 0))
		{
			resource_size_t start, end;

			start = pci_resource_start(psMTPCI->psPCIDev, ui32Index);
			end = pci_resource_end(psMTPCI->psPCIDev, ui32Index) + 1;

			arch_io_free_memtype_wc(start, end - start);
		}
#endif
	}
#else
	MT_UNREFERENCED_PARAMETER(hMTPCI);
	MT_UNREFERENCED_PARAMETER(ui32Index);
#endif
}
#endif /* defined(CONFIG_MTRR) */

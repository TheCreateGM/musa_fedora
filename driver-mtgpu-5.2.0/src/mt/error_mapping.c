/*
 * @Copyright   Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License     Dual MIT/GPLv2
 */

#include <linux/errno.h>

#include "img_types.h"
#include "img_defs.h"
#include "mtgpu_error.h"

typedef struct _MTGPU_ERR_MAPPING_ {
	MT_INT32 i32LinuxErrno;
	MTGPU_ERROR eError;
} MTGPU_ERR_MAPPING;

static MTGPU_ERR_MAPPING asErrMappingTable[] = {
	{ 0, MTGPU_OK },
	{ -EAGAIN, MTGPU_ERROR_RETRY },
	{ -EINVAL, MTGPU_ERROR_INVALID_PARAMS },
	{ -ENOMEM, MTGPU_ERROR_OUT_OF_MEMORY },
	{ -ENOMEM, MTGPU_ERROR_MMU_FAILED_TO_ALLOCATE_PAGETABLES },
	{ -ETIME, MTGPU_ERROR_TIMEOUT },
	{ -EINTR, MTGPU_ERROR_INTERRUPTED},
	{ -ENOTSUPP, MTGPU_ERROR_NOT_SUPPORTED},
	{ -ENODEV, MTGPU_ERROR_INVALID_DEVICE},
};

MTGPU_ERROR LinuxErrnoToMtError(int errno)
{
	MT_UINT32 i;
	MTGPU_ERROR eError = MTGPU_ERROR_TASK_FAILED;

	for (i = 0; i < ARRAY_SIZE(asErrMappingTable); i++) {
		if (errno == asErrMappingTable[i].i32LinuxErrno) {
			eError = asErrMappingTable[i].eError;
			break;
		}
	}

	return eError;
}

int MtErrorToLinuxErrnoFunc(MTGPU_ERROR eError)
{
	MT_UINT32 i;
	int err = -EFAULT;

	for (i = 0; i < ARRAY_SIZE(asErrMappingTable); i++)
	{
		if (eError == asErrMappingTable[i].eError)
		{
			return asErrMappingTable[i].i32LinuxErrno;
		}
	}

	return err;
}

/**
 *
 * Copyright (C) 2025 Moore Threads Ltd. All Rights Reserved.
 *
 * Descript: This is FW/GPU dump reason.
 *
 */
#ifndef __MTFW_FWIF_FW_DUMP_REASON_H__
#define __MTFW_FWIF_FW_DUMP_REASON_H__

#include "mtfw_fwif_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

/* MTFW_FWIF_DUMP_REASON_FW_HANG sub-fault IDs */
typedef enum {
    MTFW_FWIF_FW_HANG_NONE = 0,                 /* Reserved none sub-fault id */
    MTFW_FWIF_FW_HANG_FENCE_CLEAR_TIMEOUT,      /* Fence clear time exceed */
    MTFW_FWIF_FW_HANG_MCG_OOM,                  /* MCG OOM */
    MTFW_FWIF_FW_HANG_FINISH_QUEUE_FULL,        /* Finish queue is full */
    MTFW_FWIF_FW_HANG_PM_PARA_INVALID,          /* PM parameter invalid */
    MTFW_FWIF_FW_HANG_MMU_MAP_ERROR,            /* FW MMU map error */
    MTFW_FWIF_FW_HANG_SLIDING_WINDOW_EXCEED,    /* Sliding window exceeds limit */
    MTFW_FWIF_FW_HANG_FEC_UNEXPECT_COREID,      /* FEC unexpected core ID */
    MTFW_FWIF_FW_HANG_SUBMIT_QUEUE_INIT_FAIL,   /* Submission queue init fail */
    MTFW_FWIF_FW_HANG_VECTOR_INIT_FAIL,         /* Vector init fail */
    MTFW_FWIF_FW_HANG_SCHEDULE_ABNORMAL,        /* Schedule abnormal state */
    MTFW_FWIF_FW_HANG_WORK_CURSOR_NULL,         /* Work cursor is null */
    MTFW_FWIF_FW_HANG_KICK_SEMAPHORE_DEADLOCK,  /* Kick semaphore deadlock */
    MTFW_FWIF_FW_HANG_PAGE_FAULT,               /* Gpu page fault */
    MTFW_FWIF_FW_HANG_AXI_ERROR,                /* Axi error */
    MTFW_FWIF_FW_HANG_PAGE_FAULT_AND_AXI_ERROR, /* Gpu page fault and axi error */
    MTFW_FWIF_FW_HANG_HOST_REQUEST_FALT         /* Gpu host request */
} MTFW_FWIF_FW_HANG_FAULT_ID;

/**
 * On FW panic, write MTFW_FWIF_FW_PANIC_SCRATCH_FLAG to SCRATCH14
 * (MTFW_LOG_SCRATCH_BOOT_IDX) to signal a FW hang to KMD.
 *
 * Firmware VA layout:
 *   - Bit 31     : Non-zero value indicates FW panic
 *   - Bits [15:0]: MTFW_FWIF_FW_HANG_FAULT_ID
 */
#define MTFW_FWIF_FW_PANIC_SCRATCH_FLAG 0x80000000

typedef enum
{
    MTFW_FWIF_DUMP_REASON_HOST = 0,    /* Dump FW by HOST trigger */
    MTFW_FWIF_DUMP_REASON_HALT,        /* Dump because host send MTFW_SUBMISSION_FW_HALT cmd */
    MTFW_FWIF_DUMP_REASON_FW_HANG,     /* Dump because FW hang */
    MTFW_FWIF_DUMP_REASON_RUN_TIMEOUT, /* Kick in DM run timeout */
    MTFW_FWIF_DUMP_REASON_K_SEM_TIMEOUT,
    MTFW_FWIF_DUMP_REASON_Q_SEM_TIMEOUT,
    MTFW_FWIF_DUMP_REASON_CONTINUE_TIMEOUT,
    MTFW_FWIF_DUMP_REASON_PAGE_FAULT,  /* This is for MTFW_FWIF_DUMP_PAGE_FAULT_INFO */
    MTFW_FWIF_DUMP_REASON_CSW_TIMEOUT, /* CSW but GPU finish timeout */
    MTFW_FWIF_DUMP_REASON_INVALID,
    MTFW_FWIF_DUMP_REASON_MP_EXCEPTION,        /* This is for MTFW_FWIF_DUMP_MP_EXCEPTION_INFO */
    MTFW_FWIF_DUMP_REASON_MSS_ERROR,           /* This is for MTFW_FWIF_DUMP_MSS_ERROR_INFO */
    MTFW_FWIF_DUMP_REASON_MTLINk_ERROR,        /* This is for MTFW_FWIF_DUMP_MSS_ERROR_INFO */
    MTFW_FWIF_DUMP_REASON_VMEM_ECC_ERROR,      /* This is for MTFW_FWIF_DUMP_MSS_ERROR_INFO */
    MTFW_FWIF_DUMP_REASON_STALL_TIMEOUT,       /* CSW/RESUME but cdm stream not change. */
    MTFW_FWIF_DUMP_REASON_ENGINE_SYNC_TIMEOUT, /* ENGINE sync timeout and resume total time timeout */
    MTFW_FWIF_DUMP_REASON_DM_RUN_TIMEOUT,      /* DM start but not finish nor STALL. */
    MTFW_FWIF_DUMP_REASON_KILL_TIMEOUT,        /* Kill timeout. */
    MTFW_FWIF_DUMP_REASON_K_EVENT_TIMEOUT,     /* Kick Event timeout. */
    MTFW_FWIF_DUMP_REASON_MAX_CNT,
    MTFW_FWIF_DUMP_REASON_SNAPSHOT_START = 0x80000000, /* Bit31 means snapshot cmd. */
    MTFW_FWIF_DUMP_REASON_SNAPSHOT_END   = 0x800003FF,
} MTFW_FWIF_DUMP_REASON;

#ifdef __cplusplus
}
#endif

#endif /* __MTFW_FWIF_FW_DUMP_REASON_H__ */

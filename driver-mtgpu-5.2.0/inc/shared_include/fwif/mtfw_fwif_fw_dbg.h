/**
 *
 * Copyright (C) 2025 - 2025 Moore Threads Ltd. All Rights Reserved.
 *
 * Descript: Firmware DBG defintions.
 *
 */
#ifndef __MTFW_FWIF_FW_DBG_H__
#define __MTFW_FWIF_FW_DBG_H__

#include "mtfw_fwif_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum
{
    MTFW_GPU_FW_DBG_NOP = 0,
    MTFW_GPU_FW_DBG_RUN_SNAPSHOT,      /* Get running kicks schedule snapshot */
    MTFW_GPU_FW_DBG_DUMP_REGS,         /* Dump registers */
    MTFW_GPU_FW_DBG_TRIGGER_GCOV_DUMP, /* Firmware GCOV */
    MTFW_GPU_FW_DBG_TRIGGER_RB_DUMP,   /* Trigger robust buffer dump */
} MTFW_FWIF_FW_DBG_CMD;

typedef struct MTFW_ALIGN_BYTES_8
{
    gpu_va   regDumpCfgArray; /* MTFW_FWIF_DUMP_CFG object array address, index is ERROR type. */
    gpu_va   regDumpBuffAddr; /* Point to dump buffer. This address MUST align to uint64*/
    uint32_t regDumpCfgNums;  /* MTFW_FWIF_DUMP_CFG object array size */
    uint32_t regDumpBuffNum;  /* How many uint64 in regDumpBuffAddr */

    uint32_t regDumpCfgUsed; /* FW write MTFW_FWIF_DUMP_CFG index used to dump register */
    uint32_t regDumpedNum;   /* FW write register value write into regDumpBuffer number */
} MTFW_GPU_FW_DBG_DUMP_REGS_PARA;

typedef struct MTFW_ALIGN_BYTES_8
{
    MTFW_FWIF_HEAD       head;
    MTFW_FWIF_FW_DBG_CMD fwDbgCmd;

    union
    {
        MTFW_GPU_FW_DBG_DUMP_REGS_PARA dumpRegsPara; /* MTFW_GPU_FW_DBG_DUMP_REGS */
    } fwDbgPara;
} MTFW_FWIF_FW_DBG_PARA;

#ifdef __cplusplus
}
#endif

#endif /* __MTFW_FWIF_FW_DBG_H__ */

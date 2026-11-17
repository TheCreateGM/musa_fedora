/**
 *
 * Copyright (C) 2025 Moore Threads Ltd. All Rights Reserved.
 *
 * Descript: MTFW DBG CFG data struct
 *
 */
#ifndef __MTFW_FWIF_DBG_CFG_H__
#define __MTFW_FWIF_DBG_CFG_H__

#include "mtfw_fwif_types.h"
#include "mtfw_fwif_fw_dbg.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * cswMode - Context switch test mode
 * 0: Fixed mode:       fixed interval time between csw operations
 * 1: Exponential mode: exponential time = intervalUs * base^N (N:[1:cswCount0]) for interval
 * 2: Random mode:      random time [1:intervalUs] for interval
 */
typedef struct
{
    uint32_t cswMode;    // 0: Fixed mode 1: Exponential mode 2: Random mode
    uint32_t cswCount;   // Max count for csw test
    uint32_t intervalUs; // Interval times for fixed mode or base for exponential mode
    uint32_t base;
} MTFW_FWIF_CSW_VALIDATION_CFG;

/**
 * MUSA debug parameters.
 */
typedef enum
{
    MTFW_GPU_DBG_NOP = 0,
    MTFW_GPU_DBG_DISABLE_TIMER, /* Disable schedule timer. */
    MTFW_GPU_DBG_ENABLE_TIMER,  /* Enable schedule timer. */
    MTFW_GPU_DBG_CFI_ICACHE,    /* ICache Flush and Invalid (maybe SLC ...) */
    MTFW_GPU_DBG_PAUSE,         /* Pause context schedule */
    MTFW_GPU_DBG_CONTINUE,      /* Continue context schedule */
    MTFW_GPU_DBG_NEXT,          /* Step one this context */
    MTFW_GPU_DBG_BP,            /* ICache INVALID after set/unset trap/HW BP and continue. */
    MTFW_GPU_DBG_DUMP_TRAP,     /* Point to MTFW_FWIF_DBG_TRAP_BUFFER */
    MTFW_GPU_DBG_USER_MAX,      /* Follow this enum for KMD only.*/
    /* Follow enum for KMD only */
    MTFW_GPU_DBG_KMD_START = 0x1000,
    MTFW_GPU_DBG_ENABLE,
    MTFW_GPU_DBG_DISABLE,
} MTFW_FWIF_DBG_CMD;

/* TODO: remove this later */
typedef struct MTFW_ALIGN_BYTES_8
{
    gpu_pa pageTableRoot;
    gpu_va bpAddr;

    union
    {
        struct
        {
            uint32_t isSet  : 1; /* 0: Unset, 1: Set */
            uint32_t isHwBp : 1; /* 0: Trap, 1: HW breakpointer */
        } field;
        uint32_t value;
    } flags;
} MTFW_FWIF_DBG_BP_PARA;

/* TODO: remove this later */
typedef struct MTFW_ALIGN_BYTES_8
{
    gpu_pa   pageTableRoot;
    uint64_t nextStep;
} MTFW_FWIF_DBG_NEXT_PARA;


typedef struct MTFW_ALIGN_BYTES_8
{
    MTFW_FWIF_HEAD    head;
    MTFW_FWIF_DBG_CMD dbgCmd;

    union
    {
        /**
         * MTFW_GPU_DBG_CFI_ICACHE
         * MTFW_GPU_DBG_PAUSE
         * MTFW_GPU_DBG_CONTINUE
         */
        gpu_pa pageTableRoot;

        MTFW_FWIF_DBG_BP_PARA   bpPara;   /* MTFW_GPU_DBG_BP */
        MTFW_FWIF_DBG_NEXT_PARA nextPara; /* MTFW_GPU_DBG_NEXT */
    } dbgPara; /* TODO: remove this later */

    gpu_pa pageTableRoot;

    fw_va    dbgCfgData;     /* Point to MTFW_FWIF_DBG_DATA_HEAD. */
    uint32_t dbgCfgDataSize; /* MTFW_FWIF_DBG_DATA_HEAD and data total size */
} MTFW_FWIF_DBG_PARA;

typedef struct MTFW_ALIGN_BYTES_8
{
    uint64_t mpWaveIssue;
    uint32_t cmdIdx; /* Kick ID*/
    uint8_t  dmType; /*MTFW_FWIF_GPU_DM_TYPE*/
    uint8_t  dmQueueIdx;
    uint8_t  mpcIndex;
    uint8_t  mpxIndex;
    uint8_t  mpIndex;
    uint8_t  waveIndex;
    uint8_t  reserve[6];
} MTFW_FWIF_DBG_TRAP_SCH_INFO;


#define MTFW_FWIF_DBG_SCH_INFO_MAX_NUM 8192

typedef struct MTFW_ALIGN_BYTES_8
{
    MTFW_FWIF_HEAD head;

    uint32_t schInfoNum; /* How many valid sch info in schInfo */
    MTFW_FWIF_DBG_TRAP_SCH_INFO schInfo[MTFW_FWIF_DBG_SCH_INFO_MAX_NUM];
} MTFW_FWIF_DBG_TRAP_BUFFER;

typedef struct MTFW_ALIGN_BYTES_8
{
    uint64_t reg_TRAP_WAVE_MASK;
    uint8_t  mpcIndex;
    uint8_t  mpxIndex;
    uint8_t  mpIndex;
    uint8_t  reserve[5];
} MTFW_FWIF_DBG_WAVE_MASK_INFO;

#define MTFW_FWIF_DBG_WAVE_MAXK_INFO_MAX_NUM 128

typedef struct MTFW_ALIGN_BYTES_8
{
    MTFW_FWIF_HEAD head;
    uint32_t       reserve;

    uint32_t reg_TRAP_WAVE_CTRL;
    uint32_t waveMaskInfoNum; /* How many valid wave mask info in waveMaskInfo */
    MTFW_FWIF_DBG_WAVE_MASK_INFO waveMaskInfo[MTFW_FWIF_DBG_WAVE_MAXK_INFO_MAX_NUM];

    uint64_t reg_TRAP_INSERT_INSTR0; /* 0 means no insert instruction */
    uint64_t reg_TRAP_INSERT_INSTR1;
} MTFW_FWIF_DBG_CONTINUE_NEXT_PARA;

#ifdef __cplusplus
}
#endif

#endif /* __MTFW_FWIF_DBG_CFG_H__ */

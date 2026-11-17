/**
 *
 * Copyright (C) 2024 - 2024 Moore Threads Ltd. All Rights Reserved.
 *
 * Descript: Firmware schedule info for error or CSW.
 *
 */
#ifndef __MTFW_FWIF_SCH_INFO_H__
#define __MTFW_FWIF_SCH_INFO_H__

#include "mtfw_fwif_types.h"
#include "mtfw_fwif_submission.h"
#include "mtfw_fwif_node.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define MTFW_SCH_KICK_INFO_MAX_NUM (46U)

typedef struct MTFW_ALIGN_BYTES_8
{
    union
    {
        struct
        {
            uint32_t validFlag        : 1;
            uint32_t validRunningMask : 8;
        } field;
        uint32_t value;
    } flags;
    uint32_t running[8]; /* cmdIdx kick to DM */

    uint32_t kicked;   /* Last cmdIdx kick into ready queue */
    uint32_t finished; /* Last finished cmdIdx */
} MTFW_FW_SCH_KICK_BRIEF;

typedef union MTFW_ALIGN_BYTES_8
{
    MTFW_FW_SCH_KICK_BRIEF schCounter[8];

    struct MTFW_ALIGN_BYTES_8
    {
        uint64_t cbId;          /* Cmd Buffer id */
        uint32_t kickBrefCount; /* Used count for kickBref array */
        uint32_t reserve;
        struct
        {
            uint32_t kickIdx;  /* Kick index within cmd regoin */
            uint16_t reason;   /* This is for MTFW_FWIF_DUMP_REASON*/
            uint8_t  kickType; /* Kick Dm type */
            uint8_t  queueId;  /* Hw queue id */
        } kickBref[MTFW_SCH_KICK_INFO_MAX_NUM];
    } kickInfo;
} MTFW_FW_SCH_KICK_INFO;

/* Each Submission has a MTFW_FWIF_SCH_BRIEF to store schedule info */
typedef struct MTFW_ALIGN_BYTES_8
{
    uint64_t ccbItemToken;
    gpu_pa   pageTableRootAddr;

    uint32_t processId;

    MTFW_SUBMISSION_CMD_TYPE submissionCmd;

    MTFW_FW_SCH_KICK_INFO kickInfo;
} MTFW_FWIF_SCH_CTX;

typedef union MTFW_ALIGN_BYTES_8
{
    MTFW_FWIF_SCH_CTX schCtx;

    uint8_t schBuffer[4096];
} MTFW_FW_SCH_BUFFER;

#ifdef __cplusplus
}
#endif

#endif /* __MTFW_FWIF_SCH_DUMP_H__ */

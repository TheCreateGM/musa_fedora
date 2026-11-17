/**
 *
 * Copyright (C) 2024 - 2024 Moore Threads Ltd. All Rights Reserved.
 *
 * Descript: This is FW/GPU dump data struct.
 *
 */
#ifndef __MTFW_FWIF_FW_DUMP_H__
#define __MTFW_FWIF_FW_DUMP_H__

#include "mtfw_fwif_types.h"
#include "mtfw_fwif_sch_info.h"
#include "mtfw_fwif_submission.h"
#include "mtfw_fwif_fw_dump_reason.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define MTFW_FWIF_LAST_WORD_MAGIC_KEY 0x4d545946

typedef struct MTFW_ALIGN_BYTES_8
{
    uint64_t ccbItemToken;
    uint64_t hostPrivateData;
    uint32_t queueIdx;
    uint32_t sequenceNums;
    uint32_t processId;
    uint32_t queueId; /* Valid for CDM only */

    MTFW_SUBMISSION_CMD_TYPE submissionCmd;

    uint32_t cmdOffset;
    uint32_t kickPara;   /* This is MTFW_DM_KICK_PARA */
    uint32_t kickResult; /* This is MTFW_DM_KICK_RESULT */
} MTFW_FWIF_DUMP_PAGE_FAULT_INFO;

typedef MTFW_FWIF_DUMP_PAGE_FAULT_INFO MTFW_FWIF_DUMP_MP_EXCEPTION_INFO;
typedef MTFW_FWIF_DUMP_PAGE_FAULT_INFO MTFW_FWIF_DUMP_MSS_ERROR_INFO;
typedef MTFW_FWIF_DUMP_PAGE_FAULT_INFO MTFW_FWIF_DUMP_DM_TIMEOUT_INFO;

typedef struct MTFW_ALIGN_BYTES_8
{
    /* Value is MTFW_FWIF_LAST_WORD_MAGIC_KEY */
    uint32_t magicWord;

    /* CRC from codeTag*/
    uint32_t crc32;

    /* FW commit id */
    union
    {
        uint64_t tagValue;
        uint8_t  tagString[sizeof(uint64_t)]; /* End of NULL */
    } codeTag;

    uint64_t timeStamp;

    uint64_t hostReason; /* hostReason value from MTFW_SUBMISSION_FW_HALT.haltReason */

    MTFW_FWIF_DUMP_REASON reason;

    union
    {
        struct
        {
            uint32_t isFirstDump : 1; /* This is for first dump regs flags */
            uint32_t overflow    : 1; /* Dump reg fail, because dump buffer size not enough */
            uint32_t mapFail     : 1; /* Dump reg fail, because map fail */
        } field;
        uint32_t value;
    } dumpTag;

    union
    {
        MTFW_FWIF_DUMP_PAGE_FAULT_INFO   pageFaultInfo;   /* This is for MTFW_FWIF_DUMP_REASON_PAGE_FAULT */
        MTFW_FWIF_DUMP_MP_EXCEPTION_INFO mpExceptionInfo; /* This is for MTFW_FWIF_DUMP_REASON_MP_EXCEPTION */
        MTFW_FWIF_DUMP_MSS_ERROR_INFO    mssErrorInfo;    /* This is for MTFW_FWIF_DUMP_REASON_MSS_ERROR */
        MTFW_FWIF_DUMP_DM_TIMEOUT_INFO   dmTimeoutInfo;   /* This is for MTFW_FWIF_DUMP_REASON_DM_RUN_TIMEOUT */
    } info;
} MTFW_FWIF_DUMP_INFO_DESC;

typedef enum
{
    MTFW_REG_INDIRECT_TYPE_NONE = 0,
    MTFW_REG_INDIRECT_TYPE_PBE,
    MTFW_REG_INDIRECT_TYPE_USC,
    MTFW_REG_INDIRECT_TYPE_TPU,
    MTFW_REG_INDIRECT_TYPE_TEXAS,
    MTFW_REG_INDIRECT_TYPE_MERCER,
    MTFW_REG_INDIRECT_TYPE_SHARED,
    MTFW_REG_INDIRECT_TYPE_ISP,
    MTFW_REG_INDIRECT_TYPE_SWIFT,
} MTFW_FWIF_REG_INDIRECT_TYPE;

typedef enum
{
    MTFW_REG_TYPE_NORMAL = 0,
    MTFW_REG_TYPE_CE,
} MTFW_FWIF_REG_TYPE;

typedef struct MTFW_ALIGN_BYTES_8
{
    /**
     * regAddr = (regOffset) + (idx) * (regStride)
     * If regIdxNum > 0, FW will dump register from idx = 0 to regIdxNum -1.
     * regStride default value is 8byte, when regStride is 0 and regIdxNum > 0.
     */
    uint16_t regOffset;
    uint8_t  regStride;
    uint8_t  regIdxNum;
    uint8_t  regType; /* Value is MTFW_FWIF_REG_TYPE */

    uint8_t  indirectType; /* Value is MTFW_FWIF_REG_INDIRECT_TYPE */
    uint16_t indirectRegOffset;
    /**
     *  If indirectType is NOT MTFW_REG_INDIRECT_TYPE_NONE, FW will dump indirect register from 0 to indirectNum - 1
     */
    uint8_t indirectNum;

    /**
     * dumpBufNums tell FW how many 64 buffer prepair for store register value.
     * dumpBufNums MUST more than coreNums * indirectCount * regIdxCount
     */
    uint16_t dumpBufNums;
} MTFW_FWIF_REG_DUMP_CFG;

/**
 * MTFW_FWIF_REG_DUMP_DESC.magicKey is 0x4D54
 *
 * Register buffer layout:
 *
 * uint32_t RegBankBitMask
 * uint32_t RegDumpDescNumber (=n below)
 * MTFW_FWIF_REG_DUMP_DESC 0
 * UINT64_T regValue[MTFW_FWIF_REG_DUMP_DESC 0.regDumpNum]
 * MTFW_FWIF_REG_DUMP_DESC 1
 * UINT64_T regValue[MTFW_FWIF_REG_DUMP_DESC 1.regDumpNum]
 * ...
 * MTFW_FWIF_REG_DUMP_DESC n
 * UINT64_T regValue[MTFW_FWIF_REG_DUMP_DESC n.regDumpNum]
 */
#define MTFW_FWIF_REG_DUMP_MAGIC_KEY 0x4d54
typedef union MTFW_ALIGN_BYTES_8
{
    struct
    {
        uint16_t magicKey; /* This value is MTFW_FWIF_REG_DUMP_MAGIC_KEY */

        uint16_t regDumpNum;    /* How many register value follow MTFW_FWIF_REG_DUMP_DESC */
        uint16_t regDumpCfgIdx; /* Index in MTFW_FWIF_REG_DUMP_CFG arrary */

        /**
         * Dump register sequence is for(nBank) { for(index) { for(indirect) } } }
         */
        uint8_t regIdxCount;   /* How many index for */
        uint8_t indirectCount; /* How many indirect for */
    } desc;
    uint64_t value;
} MTFW_FWIF_REG_DUMP_DESC;

typedef struct MTFW_ALIGN_BYTES_8
{
    gpu_va   regDumpCfgArray; /* Point to MTFW_FWIF_REG_DUMP_CFG array. This address MUST align to uint64 */
    uint32_t regDumpCfgNum;   /* regDumpCfgArray length. */
} MTFW_FWIF_DUMP_CFG;

typedef struct MTFW_ALIGN_BYTES_8
{
    MTFW_FWIF_DUMP_INFO_DESC dumpInfo;
    MTFW_FWIF_SCH_CTX        schInfo;

    gpu_va   regDumpCfgArray; /* MTFW_FWIF_DUMP_CFG object array address, index is ERROR type. */
    uint32_t regDumpCfgNums;  /* MTFW_FWIF_DUMP_CFG object array size */

    gpu_va   regDumpBuffAddr; /* Point to dump buffer. This address MUST align to uint64*/
    uint32_t regDumpBuffNum;  /* How many uint64 in regDumpBuffAddr */

    uint32_t regDumpCfgUsed; /* FW write MTFW_FWIF_DUMP_CFG index used to dump register */
    uint32_t regDumpedNum;   /* FW write register value write into regDumpBuffer number */
} MTFW_FWIF_ERROR_INFO;

typedef struct MTFW_ALIGN_BYTES_8
{
    MTFW_FWIF_DUMP_INFO_DESC dumpInfo;
    MTFW_FWIF_SCH_CTX        schInfo;

    fw_va    regDumpBuffAddr; /* Point to dump buffer. This address MUST align to uint64*/
    uint32_t regDumpBuffNum;  /* How many uint64 in regDumpBuffAddr */

    uint32_t regDumpCfgUsed; /* FW write MTFW_FWIF_DUMP_CFG index used to dump register */
    uint32_t regDumpedNum;   /* FW write register value write into regDumpBuffer number */
} MTFW_FWIF_FAULT_INFO;

#define MTFW_FWIF_FAULT_LOG_TRACE_NUM 64

typedef struct MTFW_ALIGN_BYTES_8
{
    MTFW_FWIF_FAULT_INFO faultInfo[MTFW_FWIF_FAULT_LOG_TRACE_NUM];

    volatile uint32_t faultInfoWrIdx;
    volatile uint32_t faultInfoRdIdx;

    fw_va    regDumpCfgArray; /* MTFW_FWIF_DUMP_CFG object array address, index is ERROR type. */
    uint32_t regDumpCfgNums;  /* MTFW_FWIF_DUMP_CFG object array size */
} MTFW_FWIF_FAULT_INFO_CTRL;

typedef union MTFW_ALIGN_BYTES_8
{
    struct
    {
        uint16_t magicKey; /* This value is MTFW_FWIF_REG_DUMP_MAGIC_KEY */

        uint16_t regDumpNum;    /* regDebugBusDumpCfgNum * coreNum */
        uint16_t regDumpCfgIdx; /* Index in MTFW_FWIF_REG_DUMP_CFG arrary */

        union
        {
            struct
            {
                uint16_t overflow    : 1; /* Dump fail, because dump buffer size not enough */
                uint16_t pollTimeout : 1; /* Dump fail, because poll timeout */
            } field;
            uint16_t value;
        } dumpTag;
    } desc;
    uint64_t value;
} MTFW_FWIF_REG_DEBUG_BUS_DUMP_DESC;

typedef struct MTFW_ALIGN_BYTES_8
{
    uint64_t indirectRegValue;
    uint64_t selRegValue;

    union
    {
        struct
        {
            uint32_t readBack : 1; /* selReg need read back after config indirectRegValue value */
            uint32_t needPoll : 1; /* Do not config selReg, and poll selReg until value is selRegValue */
        } field;
        uint32_t value;
    } selRegTag;

    uint16_t indirectRegOffset; /* 0 means no need config indirectReg */
    uint16_t selRegOffset;      /* 0 means no need config selReg */

    /**
     * regAddr = (regOffset) + (idx) * (regStride)
     * If regOffset == 0 or regIdxNum == 0, means no need dump registers, just
     * writeback deafult 0. If regIdxNum > 0, FW will dump register from idx = 0
     * to regIdxNum -1. regStride default value is 8byte, when regStride is 0 and
     * regIdxNum > 0.
     */
    uint16_t regOffset;
    uint8_t  regStride;
    uint8_t  regIdxNum;

    uint32_t reserved;
} MTFW_FWIF_REG_DEBUG_BUS_DUMP_CFG;

typedef enum
{
    MTFW_DEBUG_BUS_TYPE_MP = 0,
} MTFW_FWIF_DEBUG_DUMP_DETAIL_TYPE;

typedef struct MTFW_ALIGN_BYTES_8
{
    gpu_va   regDebugBusDumpCfgArray; /* Point to MTFW_FWIF_REG_DEBUG_BUS_DUMP_CFG array. This address MUST align to uint64 */
    uint32_t regDebugBusDumpCfgNum;   /* regDebugBusDumpCfgArray length. */
    uint8_t  regType;                 /* Value is MTFW_FWIF_DEBUG_DUMP_DETAIL_TYPE */
} MTFW_FWIF_DEBUG_BUS_DUMP_CFG;

typedef struct MTFW_ALIGN_BYTES_8
{
    gpu_va   regDebugBusDumpCfgArray; /* MTFW_FWIF_DEBUG_BUS_DUMP_CFG object array address. */
    uint32_t regDebugBusDumpCfgNums;  /* MTFW_FWIF_DEBUG_BUS_DUMP_CFG object array size */
} MTFW_FWIF_DEBUG_BUS_DUMP;

typedef enum
{
    MTFW_DEBUG_DUMP_TYPE_DEBUG_BUS = 0,
} MTFW_FWIF_DEBUG_DUMP_TYPE;

typedef struct MTFW_ALIGN_BYTES_8
{
    gpu_va   dumpCfgAddr; /* Point to detail dump array. This address MUST align to uint64 */
    uint32_t dumpType;    /* Value is MTFW_FWIF_DEBUG_DUMP_TYPE */
} MTFW_FWIF_DEBUG_DUMP_CFG;

typedef struct MTFW_ALIGN_BYTES_8
{
    MTFW_FWIF_HEAD head;

    /* Value is MTFW_FWIF_LAST_WORD_MAGIC_KEY */
    uint32_t magicWord;

    /* FW commit id */
    union
    {
        uint64_t tagValue;
        uint8_t tagString[sizeof(uint64_t)]; /* End of NULL */
    } codeTag;

    union
    {
        struct
        {
            uint32_t isFirstDump : 1; /* This is for first dump regs flags */
        } field;
        uint32_t value;
    } dumpTag;

    uint64_t timeStamp;
} MTFW_FWIF_DEBUG_DUMP_INFO_DESC;

typedef struct MTFW_ALIGN_BYTES_8
{
    MTFW_FWIF_DEBUG_DUMP_INFO_DESC dumpInfo;

    gpu_va regDumpCfgArray;  /* MTFW_FWIF_DEBUG_DUMP_CFG object array address */
    uint32_t regDumpCfgNums; /* MTFW_FWIF_DEBUG_DUMP_CFG object array size */

    gpu_va regDumpBuffAddr;  /* Point to dump buffer. This address MUST align to uint64*/
    uint32_t regDumpBuffNum; /* How many uint64 in regDumpBuffAddr */
} MTFW_FWIF_DEBUG_DUMP_INFO;

#ifdef __cplusplus
}
#endif

#endif /* __MTFW_FWIF_FW_DUMP_H__ */

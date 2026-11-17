/*
 * @Copyright   Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License     Dual MIT/GPLv2
 */

#ifndef _MTVPU_CONF_H_
#define _MTVPU_CONF_H_

#define CORE_MAX_SIZE          24
#define INST_MAX_SIZE          128
#define INST_GROUP_MAX_SIZE    64
#define MEM_GROUP_MAX_SIZE     8
#define MEM_POOL_MAX_SIZE      256
#define VPU_REG_CORE_SIZE      0x4000
#define MAX_NUM_BIN            8
#define MAX_SUPPORT_VERSION    8
#define MAX_VERSION_LEN        16

#define MTVPU_DEVICE_TYPE_MASK    (0x0F00)
#define MTVPU_DEVICE_TYPE_SUDI    (0x0100)
#define MTVPU_DEVICE_TYPE_QUYUAN1 (0x0200)
#define MTVPU_DEVICE_TYPE_QUYUAN2 (0x0300)
#define MTVPU_DEVICE_TYPE_PH1     (0x0400)
#define MTVPU_DEVICE_TYPE_PH1S    (0x0500)
#define MTVPU_DEVICE_TYPE_LUSHAN  (0x0600)

struct mt_chip;

enum {
	TYPE_SUDI,
	TYPE_QUYU1,
	TYPE_QUYU2,
	TYPE_QUYU1_GUEST,
	TYPE_QUYU2_GUEST,
	TYPE_M1000,
	TYPE_PIHU1,
	TYPE_PIHU1S,
	TYPE_LUSHAN,
};

enum {
	GUEST_LOG_NONE,
	GUEST_LOG_MANUAL,
	GUEST_LOG_HWR,
};

union mtvpu_bin_support{
	struct {
		uint32_t sudi :1;
		uint32_t qy1  :1;
		uint32_t qy2  :1;
		uint32_t ph1  :1;
		uint32_t ph1s :1;
		uint32_t hs   :1;
		uint32_t hg   :1;
		uint32_t reserved :25;
	};
	uint32_t support;
} __attribute__((packed));

union mtvpu_bin_vernum{
	struct {
		uint32_t revision_num:8;
		uint32_t sub_num     :8;
		uint32_t main_num    :8;
		uint32_t reserved    :8;
	};
	uint32_t ver_number;
} __attribute__((packed));

struct mtvpu_bin_vercode {
	char vercode[21];
	char info[27];
	union mtvpu_bin_vernum vernum;
	union mtvpu_bin_support smd;
	union mtvpu_bin_support sme;
	union mtvpu_bin_support smj;
} __attribute__((packed));

struct mtvpu_bin_info {
	char name[32];
	uint32_t offset;
	uint32_t size;
	uint32_t has_vercode;
	struct mtvpu_bin_vercode vercode;
} __attribute__((packed));

struct mtvpu_header {
	uint32_t magic;
	uint32_t check_sum;
	uint32_t bin_number;
	uint32_t reserved;
	struct mtvpu_bin_info bins[MAX_NUM_BIN];
} __attribute__((packed));

struct vpu_core_require {
	char bin_name[32]; // which bin in mtvpu.bin
	char fw_version[MAX_SUPPORT_VERSION][MAX_VERSION_LEN];  // What versions do I support 
};
struct mt_conf {
	u32 type;

	u32 regs_base;
	u32 regs_chip_size;
	u32 regs_core_size[CORE_MAX_SIZE];

	u32 regs_mmu_offset;
	u32 regs_top_offset;
	u32 regs_ata_offset;

	u32 core_base;
	u32 core_size;

	u32 product[CORE_MAX_SIZE];

	u32 core_group[MEM_GROUP_MAX_SIZE][CORE_MAX_SIZE];
	struct vpu_core_require core_req[CORE_MAX_SIZE];
};

struct mtvpu_abi_supported {
	u32 abi_version;
	u64 cmds_check[2];
};

int vpu_init_conf(int devId, struct mt_chip *chip, bool guest);
int vpu_deinit_conf(struct mt_chip *chip);

struct mt_chip *get_chip(int coreIdx);
struct mt_core *get_core(int coreIdx);
u32 get_product(int coreIdx);

int request_host_support_version(u32 req_ver, u64 req_check0, u64 req_check1);
void init_max_support_abi_version(struct mt_chip *chip);
int check_guest_abi_version(struct mt_chip *chip);
#endif

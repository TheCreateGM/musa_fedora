/*
 * @Copyright Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License Dual MIT/GPLv2
 */

#ifndef _MTGPU_DEBUG_H_
#define _MTGPU_DEBUG_H_

#define MTGPU_DUMPDEBUG_LOG(...)					\
	do								\
	{								\
		if (dump_debug_printf)					\
			dump_debug_printf(dump_debug_file, __VA_ARGS__);\
		else							\
			mtgpu_debug_printf(__VA_ARGS__);			\
	} while (0)

#define DEBUG_WIDTH			(4)
#define MTGPU_MUSA_SNAPSHOT_MIN_INTERVAL_MS	(1000)

struct mtgpu_device_node;
struct mtgpu_job_list;

struct mmu_fault_info {
	u64 address;  /* Address of the page fault */
	union {
		struct {
			u64 pc : 32; /* Context */
			u64 page_fault : 1;
			u64 read : 1;  /* Read or write? */
			u64 fault : 1;
			u64 type : 2;  /* Type (see PageFaultType definition) */
			u64 bif_id : 8; /* Bus interface id */
			u64 reserved : 19;
		};
		u64 u64_all;
	} filed;
	u8 p_tag_id[16];
	u8 p_mmu_level[16];
	u8 p_tag_sb[32];
	u8 p_module[16];
};

struct axi_error_info {
	int exception_type; /* Exception type of the AXI error */
	u64 address; /* Virtual Address that triggered AXI Error */
	union {
		struct {
			u64 read_or_write : 1; /* AXI Request type of error. 0: write/atomic; 1: read */
			u64 context_id : 5; /* Context ID that triggered AXI Error */
			u64 error_type : 2; /* AXI error type: 0: No error; 1: Timeout; 2: Slave Error; 3: Decode Error */
			u64 req_id : 5; /* Requestor ID of requestor that triggered the error */
			u64 reserved : 51; /* Reserved */
		};
		u64 u64_all; /* Combined error status */
	} error_status;
	const char *p_error_type; /* Pointer to the error type description */
	const char *p_requestor; /* Pointer to the requestor description */
};

struct ce_mmu_fault_info {
	u64 address;  /* Address of the page fault */
	union {
		struct {
			u64 context : 8; /* Page fault context */
			u64 fault : 1; /* Fault exist or not */
			u64 read_or_write : 1; /* Page fault occurs in 0: write/atomic; 1: read */
		};
		u64 u64_all;
	} fault_status;
	const char *fault_level;
	const char *fault_type;
	const char *err_type;
	const char *err_module;
};

void mtgpu_debug_printf(const char *format, ...);

void mtgpu_debug_dump(struct mtgpu_device_node *dev_node,
		      bool verbose,
		      DUMPDEBUG_PRINTF_FUNC *dump_debug_printf,
		      void *dump_debug_file);
void mtgpu_debug_dump_job_list(struct mtgpu_device_node *dev_node, struct mtgpu_job_list *job_list,
			       DUMPDEBUG_PRINTF_FUNC *dump_debug_printf, void *dump_debug_file);
int mtgpu_debug_dump_init(void);
void mtgpu_debug_dump_deinit(void);

const char *mtgpu_debug_get_queue_type_string(u32 value);
const char *mtgpu_debug_get_cmd_type_string(int cmd);

enum mtgpu_ref_op_type {
        MTGPU_REF_OP_INIT,
        MTGPU_REF_OP_GET,
        MTGPU_REF_OP_PUT,
        MTGPU_REF_OP_FREE,
        MTGPU_REF_OP_MAX
};

enum mtgpu_ref_obj_type {
        MTGPU_REF_OBJ_PMR,
        MTGPU_REF_OBJ_SYNC_PRIM,
        MTGPU_REF_OBJ_MAX
};

void mtgpu_ref_dump(void *object,
		    MTGPU_DEVICE_NODE *psDevNode,
		    u32 old_ref,
		    enum mtgpu_ref_op_type op_type,
		    enum mtgpu_ref_obj_type obj_type);

int mtgpu_fw_sch_snapshot_submit(struct mtgpu_device_node *dev_node);
int mtgpu_musa_snapshot_submit(struct mtgpu_device_node *dev_node);

bool mtgpu_ccb_debug_enabled(void);

#endif /* MTGPU_DEBUG_H */

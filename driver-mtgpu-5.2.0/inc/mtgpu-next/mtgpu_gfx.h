/*
 * @Copyright Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License Dual MIT/GPLv2
 */

#ifndef __MTGPU_GFX_H__
#define __MTGPU_GFX_H__

struct mtgpu_vm_context;
struct MTFW_FWIF_TAG;
struct mtgpu_device_node;

//#define FREELIST_DEBUGINFO

#ifdef FREELIST_DEBUGINFO
#define	PB_DBG(str, ...) os_pr_info("MTGPU PB: " str, __VA_ARGS__)
#else
#define	PB_DBG(str, ...) {}
#endif /* FREELIST_DEBUGINFO */

/* For sudi, auto set pb size 256M. */
#define MTGPU_PB_SIZE_256MB	0x10000000
/* For quyuan1,quyuan2 auto set pb size 2G. */
#define MTGPU_PB_SIZE_2GB	0x80000000
/* For other gpu, auto set pb size 512M. */
#define MTGPU_PB_SIZE_512MB		0x20000000

#define MTGPU_PB_DEFAULT_SIZE		0x20000000

#define FL_SIZE_VZ			0x200000

#if (MUSA_NUM_OS_SUPPORTED > 1)
#include "vgpu/vgpu_heap.h"
#define MTGPU_HEAP_PM_FREELIST_BASE MTGPU_VGPU_HEAP_FREELIST_BASE + MTGPU_VGPU_HEAP_FREELIST_SIZE - FL_SIZE_VZ
#else
#define MTGPU_HEAP_PM_FREELIST_BASE MTGPU_HEAP_RESERVED_PM_BUFFER_BASE
#endif

/* PM free list use 4K page size for SUDI and QUYUAN */
#define PM_PAGE_SIZE		(1 << 12)
#define PM_PAGE_SHIFT		(12)
/* PM free list use 64K page size for PH1 and later */
#define PM_CBUF_PAGE_SIZE	(1 << 16)
#define PM_CBUF_PAGE_SHIFT	(16)
#define PM_CBUF_PAGE_MASK	(0x3ffff)


#define PAGE_ENTRY_VA_ALIGN	(0x20)
/* align page entry addr. */
#define PM_PAGE_NUM_ALIGN	(PAGE_ENTRY_VA_ALIGN / sizeof(u32))

#define PB_DESC_COUNT \
		(MTFW_SCG_PB_COUNT + \
		 MTFW_MCG2_PB_COUNT + \
		 MTFW_MCG3_PB_COUNT + \
		 MTFW_MCG4_PB_COUNT + \
		 MTFW_MCG5_PB_COUNT + \
		 MTFW_MCG6_PB_COUNT + \
		 MTFW_MCG7_PB_COUNT + \
		 MTFW_MCG8_PB_COUNT)

#define PB_DESC_TOTAL_SIZE	(PB_DESC_COUNT * sizeof(union mtgpu_pb_desc))

#define PM_BUFFER_ALLOC_FLAG (MTGPU_MEMALLOCFLAG_GPU_WRITEABLE | \
			      MTGPU_MEMALLOCFLAG_GPU_READABLE | \
			      MTGPU_MEMALLOCFLAG_GPU_CACHE_INCOHERENT)

union mtgpu_pb_desc {
	/* old freelist state, used for pm. */
	struct mtgpu_freelist_state {
		/* gpu va, required to be aligned to DWORD. */
		uint64_t freelist_page_array_base;
		uint32_t stack_ptr;
	} freelist_state;

	/* cbuf descriptor, for ph1 or later pmva. */
	struct mtgpu_cbuf_desc {
		uint64_t start_addr  : 43; // sizeof(CBUF_DESC) (256 bits) aligned
		uint64_t reserve0    : 21;
		uint64_t size_in_64k : 19;
		uint64_t reserve1    : 13;
		uint64_t alloc_ptr   : 18;
		uint64_t reserve2    : 14;
		uint64_t dealloc_ptr : 18;
		uint64_t reserve3    : 14;
		uint64_t full        : 1; // Set if all VA pages in C buffer are valid (initial state)
		uint64_t empty       : 1; // Set if no VA page in C buffer is valid
		uint64_t reserve4    : 30;
		uint64_t close_ptr   : 18;
		uint64_t reserve5    : 46;
	} cbuf_desc;

	/* used for force align. */
	uint32_t value[64];
};

void mtgpu_gfx_pb_deinit(struct mtgpu_device_node *dev_node);
int mtgpu_gfx_pb_init(struct mtgpu_device_node *dev_node);

void mtgpu_gfx_pb_vm_unmap(struct mtgpu_vm_context *vm_ctx);
int mtgpu_gfx_pb_vm_map(struct mtgpu_device_node *dev_node,
			struct mtgpu_vm_context *vm_ctx);

int mtgpu_gfx_pmva_map(struct mtgpu_device_node *dev_node, struct mtgpu_vm_context *vm_ctx);
void mtgpu_gfx_pmva_unmap(struct mtgpu_vm_context *vm_ctx);

int mtgpu_freelist_recovery(struct mtgpu_device_node *dev_node);
uint32_t get_freelist_bo_phy_base(struct mtgpu_device_node *dev_node, uint64_t *fl_bo_base_addr);
int get_freelist_all_page(struct mtgpu_device_node *dev_node, u64 **fl_page_array, u64 *array_size);
void put_freelist_all_page(uint64_t *fl_page_array);

void mtgpu_pb_total_size_init(struct mtgpu_device *mtdev);
u64 mtgpu_gfx_get_pb_size(struct mtgpu_device *mtdev);

#define MTGPU_FREELIST_INIT(fw_cfg, list_prefix, count, freelist_gpuva) do { \
	int __i; \
	for (__i = 0; __i < (count); __i++) { \
		(fw_cfg)->flStateAddrs##list_prefix[__i] = *(freelist_gpuva); \
		PB_DBG("flStateAddrs" #list_prefix "[%d]: 0x%llx", __i, (fw_cfg)->flStateAddrs##list_prefix[__i]); \
		*(freelist_gpuva) += sizeof(union mtgpu_pb_desc); \
	} \
} while (0)

#define MTGPU_PB_INIT_ALL(fw_cfg) do { \
	u64 __freelist_state_gpuva = MTGPU_HEAP_PM_FREELIST_BASE; \
	MTGPU_FREELIST_INIT(fw_cfg, SCG,  MTFW_SCG_PB_COUNT,  &__freelist_state_gpuva); \
	MTGPU_FREELIST_INIT(fw_cfg, MCG2, MTFW_MCG2_PB_COUNT, &__freelist_state_gpuva); \
	MTGPU_FREELIST_INIT(fw_cfg, MCG3, MTFW_MCG3_PB_COUNT, &__freelist_state_gpuva); \
	MTGPU_FREELIST_INIT(fw_cfg, MCG4, MTFW_MCG4_PB_COUNT, &__freelist_state_gpuva); \
	MTGPU_FREELIST_INIT(fw_cfg, MCG5, MTFW_MCG5_PB_COUNT, &__freelist_state_gpuva); \
	MTGPU_FREELIST_INIT(fw_cfg, MCG6, MTFW_MCG6_PB_COUNT, &__freelist_state_gpuva); \
	MTGPU_FREELIST_INIT(fw_cfg, MCG7, MTFW_MCG7_PB_COUNT, &__freelist_state_gpuva); \
	MTGPU_FREELIST_INIT(fw_cfg, MCG8, MTFW_MCG8_PB_COUNT, &__freelist_state_gpuva); \
} while (0)

#endif /* __MTGPU_GFX_H__ */

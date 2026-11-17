#ifndef DEVICEMEM_NEXT_H
#define DEVICEMEM_NEXT_H

#include "devicemem_utils.h"

typedef struct _DEVMEMINT_CTX_ DEVMEMINT_CTX;

struct devmem_virt_heap {
	DEVMEMINT_CTX *psDevmemCtx;
	uint64_t base;
	uint64_t length;
	uint32_t page_shift;
	uint32_t page_size_bit_mask;
	ATOMIC_T uiRefCount;
	bool legacy;
};

struct devmem_virt_mem {
	struct devmem_virt_heap *heap;
	uint64_t rm_gpu_virt_mem;
	uint64_t gpu_va;
	uint64_t size;
	uint32_t page_shift;
};

struct devmem_mapping {
	struct devmem_virt_mem *virt_mem;
	PMR *psPMR;
	uint32_t num_pages;
	uint32_t pid;
};

MTGPU_ERROR devmem_virt_heap_create(DEVMEMINT_CTX *psDevmemCtx, uint64_t base, uint64_t length,
				     uint32_t page_shift, uint32_t page_size_bit_mask, bool legacy,
				     struct devmem_virt_heap **p_virt_heap);

MTGPU_ERROR devmem_virt_heap_destroy(struct devmem_virt_heap *heap);

bool devmem_virt_heap_acquire(struct devmem_virt_heap *heap);

bool devmem_virt_heap_release(struct devmem_virt_heap *heap);

MTGPU_ERROR devmem_virt_mem_alloc(struct devmem_virt_heap *heap, uint64_t gpu_va, uint64_t size,
				   uint32_t page_shift, struct devmem_virt_mem **p_virt_mem, uint32_t vm_ctx_flag);

MTGPU_ERROR devmem_virt_mem_free(struct devmem_virt_mem *virt_mem);

MTGPU_ERROR devmem_virt_mem_map(struct devmem_virt_mem *virt_mem, struct mtgpu_bo *bo,
				 MTGPU_MEMALLOCFLAGS_T flags, struct devmem_mapping **p_mapping);

MTGPU_ERROR devmem_virt_mem_unmap(struct devmem_mapping *mapping);

/* wrapper for bridge call */

static inline MTGPU_ERROR DevmemIntHeapCreate(DEVMEMINT_CTX *psDevmemCtx, uint64_t sHeapBaseAddr,
					       MT_DEVMEM_SIZE_T uiHeapLength,
					       MT_UINT32 uiLog2DataPageSize,
					       DEVMEMINT_HEAP **ppsDevmemHeapPtr)
{
	return devmem_virt_heap_create(psDevmemCtx, sHeapBaseAddr, uiHeapLength, uiLog2DataPageSize,
				       1u << uiLog2DataPageSize, true, ppsDevmemHeapPtr);
}

static inline MTGPU_ERROR DevmemIntHeapDestroy(DEVMEMINT_HEAP2 *psDevmemHeap)
{
	return devmem_virt_heap_destroy(psDevmemHeap);
}

static inline MTGPU_ERROR DevmemIntHeapCreate2(DEVMEMINT_CTX *psDevmemCtx, uint64_t sHeapBaseAddr,
						MT_DEVMEM_SIZE_T uiHeapLength,
						MT_UINT32 ui32PageSizeBitMask,
						DEVMEMINT_HEAP2 **ppsDevmemHeapPtr)
{
	return devmem_virt_heap_create(psDevmemCtx, sHeapBaseAddr, uiHeapLength, 0,
				       ui32PageSizeBitMask, false, ppsDevmemHeapPtr);
}

static inline MTGPU_ERROR DevmemIntHeapDestroy2(DEVMEMINT_HEAP2 *psDevmemHeap)
{
	return devmem_virt_heap_destroy(psDevmemHeap);
}

static inline MTGPU_ERROR DevmemIntReserveRange(DEVMEMINT_HEAP *psDevmemHeap,
						 uint64_t sAllocationDevVAddr,
						 MT_DEVMEM_SIZE_T uiAllocationSize,
						 DEVMEMINT_RESERVATION **ppsReservationPtr)
{
	return devmem_virt_mem_alloc(psDevmemHeap, sAllocationDevVAddr, uiAllocationSize, 0,
				     ppsReservationPtr, 0);
}

static inline MTGPU_ERROR DevmemIntUnreserveRange(DEVMEMINT_RESERVATION2 *psReservation)
{
	return devmem_virt_mem_free(psReservation);
}

static inline MTGPU_ERROR DevmemIntReserveRange2(DEVMEMINT_HEAP2 *psDevmemHeap,
						  MT_UINT32 ui32log2PageSize,
						  uint64_t sAllocationDevVAddr,
						  MT_DEVMEM_SIZE_T uiAllocationSize,
						  DEVMEMINT_RESERVATION2 **ppsReservationPtr)
{
	return devmem_virt_mem_alloc(psDevmemHeap, sAllocationDevVAddr, uiAllocationSize,
				     ui32log2PageSize, ppsReservationPtr, 0);
}

static inline MTGPU_ERROR DevmemIntUnreserveRange2(DEVMEMINT_RESERVATION2 *psReservation)
{
	return devmem_virt_mem_free(psReservation);
}

#endif /* DEVICEMEM_NEXT_H */

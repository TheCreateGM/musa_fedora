/*************************************************************************/ /*!
@File
@Title          MTGPU ION memory stats
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Implementation of ION memory stats.
@License        Dual MIT/GPLv2

The contents of this file are subject to the MIT license as set out below.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

Alternatively, the contents of this file may be used under the terms of
the GNU General Public License Version 2 ("GPL") in which case the provisions
of GPL are applicable instead of those above.

If you wish to allow use of your version of this file only under the terms of
GPL, and not to allow others to use your version of this file under the terms
of the MIT license, indicate your decision by deleting the provisions above
and replace them with the notice and other provisions required by GPL as set
out in the file called "GPL-COPYING" included in this distribution. If you do
not delete the provisions above, a recipient may use your version of this file
under the terms of either the MIT license or GPL.

This License is also included in this distribution in the file called
"MIT-COPYING".

EXCEPT AS OTHERWISE STATED IN A NEGOTIATED AGREEMENT: (A) THE SOFTWARE IS
PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT; AND (B) IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/ /**************************************************************************/

#include <linux/log2.h>
#include <linux/version.h>
#include <linux/rbtree.h>
#include <linux/dma-buf.h>
#include <linux/sched/task.h>
#include "mt_ion_stats.h"
#include "mt_ion_mem.h"
#include "img_types.h"
#include "mt_debug.h"
#include "allocmem.h"
#include "lock_types.h"
#include "lock.h"
#include "osfunc.h"
#include "ion.h"


#define GET_ION_HEAP_ID(mask) (ilog2(mask))

/* It's better to check dma operations but ION dma_buf_ops is not exported. */
#define ION_DMA_BUF_EXP_NAME "ion_dma_buf"

typedef struct _mt_ion_stats_buf_ {
	/* Node in a tree */
	struct rb_node node;
	/* Indicate the buffer was created from which heap */
	MT_UINT32     ui32HeapKey;
	/* A key which represents the buffer in tree */
	uintptr_t      addr;
	/* Size of the buffer */
	size_t         uiBytes;

	/* used for debugging */
	struct task_struct *psTask;

	MT_PID uiPID;
} MT_ION_STATS_BUF;

/* forward declare */
static MT_UINT32 StringHashFunc(uintptr_t input);
static MT_UINT32 DefaultHashFunc(uintptr_t input);

static MT_BOOL isIonBuf(struct dma_buf *psDmaBuf)
{
	if (!strcmp(psDmaBuf->exp_name, ION_DMA_BUF_EXP_NAME))
		return MT_TRUE;

	return MT_FALSE;
}

static MT_UINT32 mt_ion_stats_query_heaps(struct ion_device *dev, MT_ION_STATS_HEAP *heaps,
					    HashKeypfn pfnHashKey)
{
	/* The heap id is good to be a hash key as it's unique. From Linux 4.12,
	 * ION id mask has been deprecated. Getting heap ids is only supported
	 * to query ION driver from userspace. It's not possible to get correct
	 * heap ids in kernel space and we're pretty sure ION_HEAP_NAME definitions
	 * are robust here as device won't boot if the name is incorrect. Thus,
	 * creating a key from heap name instead.
	 * For Android common 5.4, ion_query_heaps_kernel is supported.
	 */
	size_t numHeapsData;
	MT_UINT32 i;

	struct ion_heap_data sDefaultIonHeapsData[MAX_NUM_HEAPS];

	numHeapsData = ion_query_heaps_kernel(dev, sDefaultIonHeapsData,
					      ARRAY_SIZE(sDefaultIonHeapsData));
	for (i = 0; i < numHeapsData; i++) {
		strscpy(heaps[i].szName, sDefaultIonHeapsData[i].name, 32);
		heaps[i].ui32HashKey = pfnHashKey(sDefaultIonHeapsData[i].heap_id);
	}

	return numHeapsData;
}

static int mt_ion_stats_show(OSDI_IMPL_ENTRY *s, void *v)
{
	MT_ION_STATS_HEAP *psHeap = (MT_ION_STATS_HEAP *)DIGetPrivData(s);
	MT_ION_STATS_STATE *psState = psHeap->stats_state;
	MT_ION_STATS_BUF *entry;
	size_t total_size = 0;
	struct rb_node *n;

	DIPrintf(s, "%16s %8s %18s %10s\n", "client", "pid", "address", "size");
	DIPuts(s, "-------------------------------------------------------\n");

	OSLockAcquire(psState->hBuffersLock);
	for (n = rb_first(psState->buffers); n; n = rb_next(n)) {
		char task_comm[TASK_COMM_LEN];
		entry = rb_entry(n, MT_ION_STATS_BUF, node);

		if (entry->ui32HeapKey != psHeap->ui32HashKey)
			continue;

		if (entry->psTask) {
			get_task_comm(task_comm, entry->psTask);
			DIPrintf(s, "%16s %8u 0x%016lx %10zu\n", task_comm,
					task_pid_nr(entry->psTask), entry->addr,
					entry->uiBytes);
		}
		else {
			DIPrintf(s, "%16s %8u 0x%016lx %10zu\n", "kernel", entry->uiPID,
					entry->addr, entry->uiBytes);
		}
		total_size += entry->uiBytes;
	}
	OSLockRelease(psState->hBuffersLock);

	DIPuts(s, "-------------------------------------------------------\n");
	DIPrintf(s, "%44s %10zu\n", "total", total_size);
	DIPuts(s, "-------------------------------------------------------\n");

	return 0;
}

static MTGPU_ERROR
ion_stats_init(MT_ION_STATS_STATE *psState, DI_GROUP *gpudir)
{
	MT_ION_STATS_HEAP *heaps = psState->sHeapData;
	MT_UINT32 ui32NumHeaps = psState->ui32NumHeaps;
	DI_GROUP **iondir = &psState->debugfs_ion;
	DI_GROUP **heapsdir = &psState->debugfs_heaps;
	DI_ENTRY **entry = psState->debugfs_heaps_entry;
	const DI_ITERATOR_CB iterator = {
		.pfnShow = mt_ion_stats_show,
	};
	MTGPU_ERROR eError;
	int i;

	if (!ui32NumHeaps) {
		/* No heaps are available, return success */
		return MTGPU_OK;
	}

	if (!gpudir) {
		MT_DPF((MT_DBG_ERROR, "gpu debugfs is null"));
		eError = MTGPU_ERROR_INVALID_PARAMS;
		goto err_out;
	}

	if (*iondir) {
		MT_DPF((MT_DBG_ERROR, "ION debugfs already created."));
		eError = MTGPU_ERROR_INVALID_PARAMS;
		goto err_out;
	}

	if (*heapsdir) {
		MT_DPF((MT_DBG_ERROR, "ion/heaps debugfs already created."));
		eError = MTGPU_ERROR_INVALID_PARAMS;
		goto err_out;
	}

	eError = DICreateGroup("ion", gpudir, iondir);
	if (eError != MTGPU_OK) {
		MT_DPF((MT_DBG_ERROR, "Failed to create ion debugfs directory."));
		goto err_out;
	}

	eError = DICreateGroup("heaps", *iondir, heapsdir);
	if (eError != MTGPU_OK) {
		MT_DPF((MT_DBG_ERROR, "Failed to create heaps debugfs directory."));
		goto err_destroy_ion_debugfs;
	}

	for (i = 0; i < ui32NumHeaps; i++) {
		eError = DICreateEntry(heaps[i].szName, *heapsdir, &iterator,
				&heaps[i], DI_ENTRY_TYPE_GENERIC, &entry[i]);
		if (eError != MTGPU_OK) {
			MT_DPF((MT_DBG_ERROR, "Failed to create heaps %s entry: %d.",
				 heaps[i].szName, eError));
			goto err_destroy_heaps_and_entry_debugfs;
		}
		heaps[i].stats_state = psState;
	}

	return MTGPU_OK;

err_destroy_heaps_and_entry_debugfs:
	DIDestroyGroup(*heapsdir);
	while ((i--) > 0) {
		DIDestroyEntry(entry[i]);
	}
err_destroy_ion_debugfs:
	DIDestroyGroup(*iondir);
err_out:
	return eError;
}

static MT_ION_STATS_BUF *GetBuf(struct rb_root *root, uintptr_t addr)
{
	struct rb_node *entry = root->rb_node;
	MT_ION_STATS_BUF *psBuf;

	while (entry != NULL) {
		psBuf = container_of(entry, MT_ION_STATS_BUF, node);

		if (addr < psBuf->addr)
			entry = entry->rb_left;
		else if (addr > psBuf->addr)
			entry = entry->rb_right;
		else
			return psBuf;
	}

	return NULL;
}

static MT_UINT32 StringHashFunc(uintptr_t input)
{
	/* Djb2 hash function */
	const char *szHeapName = (const char *)input;
	unsigned long hash = 5381;
	int c;

	while ((c = *szHeapName++))
	{
		hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
	}

	hash &= ~(1ul << 31);

	return (MT_UINT32)hash;
}

static MT_UINT32 DefaultHashFunc(uintptr_t input)
{
	return (MT_UINT32)input;
}

MTGPU_ERROR MTGPUIonStatsInitialise(MTGPU_DEVICE_NODE *psDeviceNode)
{
	MTGPU_DEVICE_DEBUG_INFO *psDebugInfo = &psDeviceNode->sDebugInfo;
	MTGPU_DEVICE_CONFIG *psDevConfig = psDeviceNode->psDevConfig;
	MT_ION_STATS_STATE *psState;
	MTGPU_ERROR eError;

	if (!psDeviceNode) {
		MT_DPF((MT_DBG_ERROR, "%s: psDeviceNode invalid.", __func__));
		return MTGPU_ERROR_INVALID_PARAMS;
	}
	if (!psDebugInfo) {
		MT_DPF((MT_DBG_ERROR, "%s: sDebugInfo NOT available.", __func__));
		return MTGPU_ERROR_INVALID_PARAMS;
	}

	if (!psDevConfig) {
		MT_DPF((MT_DBG_ERROR, "%s: psDevConfig NOT available.", __func__));
		return MTGPU_ERROR_INVALID_PARAMS;
	}

	if (!psDevConfig->ion_dev)
		return MTGPU_OK;

	psState = OSAllocZMem(sizeof(*psState));
	MT_LOG_RETURN_IF_NOMEM(psState, "OSAllocZMem");

	psState->pfnHashKey = DefaultHashFunc;

	psState->ui32NumHeaps = mt_ion_stats_query_heaps(psDevConfig->ion_dev, psState->sHeapData,
							  psState->pfnHashKey);
	if (!psState->ui32NumHeaps) {
		MT_DPF((MT_DBG_WARNING, "No ION heaps are available."));
		return MTGPU_OK;
	}

	eError = OSLockCreateNoStats(&psState->hBuffersLock);
	if (eError != MTGPU_OK) {
		MT_DPF((MT_DBG_ERROR, "Failed to create buffers lock."));
		return eError;
	}

	psState->buffers = OSAllocZMem(sizeof(*psState->buffers));
	if (!psState->buffers)
	MT_LOG_RETURN_IF_NOMEM(psState->buffers, "OSAllocZMem");

	eError = ion_stats_init(psState, psDebugInfo->psGroup);
	if (eError != MTGPU_OK)
		OSLockDestroyNoStats(psState->hBuffersLock);
	else
		*psState->buffers = RB_ROOT;

	psDebugInfo->psMtIonStatsState = psState;

	return eError;
}

void MTGPUIonStatsDestroy(MTGPU_DEVICE_NODE *psDeviceNode)
{
	MTGPU_DEVICE_DEBUG_INFO *psDebugInfo = &psDeviceNode->sDebugInfo;
	MT_ION_STATS_STATE *psState = psDebugInfo->psMtIonStatsState;
	MT_ION_STATS_BUF *entry;
	struct rb_node *n;
	int i;

	if (!psDeviceNode->psDevConfig->ion_dev)
		return;

	/* Cleanup nodes if any */
	OSLockAcquire(psState->hBuffersLock);
	for (n = rb_first(psState->buffers); n; n = rb_next(n)) {
		entry = rb_entry(n, MT_ION_STATS_BUF, node);
		rb_erase(&entry->node, psState->buffers);
		dma_buf_put((struct dma_buf *)entry->addr);
		OSFreeMemNoStats(entry);
	}
	OSLockRelease(psState->hBuffersLock);

	if (psState->buffers)
	{
		OSFreeMem(psState->buffers);
	}

	for (i = 0; i < psState->ui32NumHeaps; i++) {
		DIDestroyEntry(psState->debugfs_heaps_entry[i]);
		psState->debugfs_heaps_entry[i] = NULL;
	}

	if (psState->debugfs_heaps) {
		DIDestroyGroup(psState->debugfs_heaps);
		psState->debugfs_heaps = NULL;
	}

	if (psState->debugfs_ion) {
		DIDestroyGroup(psState->debugfs_ion);
		psState->debugfs_ion = NULL;
	}

	if (psState->hBuffersLock) {
		/* Destroy the lock */
		OSLockDestroyNoStats(psState->hBuffersLock);
		psState->hBuffersLock = NULL;
	}
}

void MTGPUIonAddMemAllocRecord(struct dma_buf *psDmaBuf, MTGPU_DEVICE_NODE *psDeviceNode)
{
	MTGPU_DEVICE_DEBUG_INFO *psDebugInfo = &psDeviceNode->sDebugInfo;
	MT_ION_STATS_STATE *psState = psDebugInfo->psMtIonStatsState;
	struct rb_node **p = &psState->buffers->rb_node;
	HashKeypfn pfnHashKey = psState->pfnHashKey;
	MT_ION_STATS_BUF *psBuf, *entry;
	struct rb_node *parent = NULL;
	struct ion_buffer *psIonBuf;
	struct task_struct *psTask;

	if (!psState || !psState->ui32NumHeaps)
		return;

	if (!psDmaBuf) {
		MT_DPF((MT_DBG_ERROR, "Invalid dma buffer"));
		return;
	}

	/* We're only interested in ION buffers */
	if (isIonBuf(psDmaBuf) == MT_FALSE)
		return;

	psIonBuf = (struct ion_buffer *)psDmaBuf->priv;
	if (!psIonBuf) {
		MT_DPF((MT_DBG_ERROR, "Invalid ION buffer"));
		return;
	}

	psBuf = OSAllocZMemNoStats(sizeof(MT_ION_STATS_BUF));
	if (!psBuf) {
		MT_DPF((MT_DBG_ERROR, "Failed to allocate memory."));
		return;
	}

	if (pfnHashKey == StringHashFunc) {
		psBuf->ui32HeapKey = pfnHashKey((uintptr_t)psIonBuf->heap->name);
		MT_UNREFERENCED_PARAMETER(DefaultHashFunc);
	}
	else
		psBuf->ui32HeapKey = pfnHashKey((uintptr_t)psIonBuf->heap->id);

	psBuf->addr = (uintptr_t)psDmaBuf;
	psBuf->uiBytes = psDmaBuf->size;

	get_task_struct(current->group_leader);
	task_lock(current->group_leader);

	psTask = current->group_leader;
	psBuf->uiPID = task_pid_nr(psTask);

	if (current->group_leader->flags & PF_KTHREAD) {
		put_task_struct(current->group_leader);
		psTask = NULL;
	}

	psBuf->psTask = psTask;

	task_unlock(current->group_leader);

	OSLockAcquire(psState->hBuffersLock);
	while (*p) {
		parent = *p;
		entry = rb_entry(parent, MT_ION_STATS_BUF, node);

		if (psBuf->addr < entry->addr) {
			p = &(*p)->rb_left;
		} else if (psBuf->addr > entry->addr) {
			p = &(*p)->rb_right;
		} else {
			/* addr should be unique. */
			MT_DPF((MT_DBG_ERROR, "Buffer have already in tree"));
			MT_ASSERT(0);
		}
	}

	rb_link_node(&psBuf->node, parent, p);
	rb_insert_color(&psBuf->node, psState->buffers);
	/* Take a ref count on this buffer in case somehow it's released */
	get_dma_buf(psDmaBuf);
	OSLockRelease(psState->hBuffersLock);
}

void MTGPUIonRemoveMemAllocRecord(struct dma_buf *psDmaBuf, MTGPU_DEVICE_NODE *psDeviceNode)
{
	MTGPU_DEVICE_DEBUG_INFO *psDebugInfo = &psDeviceNode->sDebugInfo;
	MT_ION_STATS_STATE *psState = psDebugInfo->psMtIonStatsState;
	MT_ION_STATS_BUF *psBuf;

	if (!psDmaBuf) {
		MT_DPF((MT_DBG_ERROR, "Invalid dma buffer"));
		return;
	}

	if (!psState)
		return;

	/* We're only interested in ION buffers */
	if (isIonBuf(psDmaBuf) == MT_FALSE)
		return;

	OSLockAcquire(psState->hBuffersLock);
	psBuf = GetBuf(psState->buffers, (uintptr_t)psDmaBuf);
	if (!psBuf) {
		MT_DPF((MT_DBG_ERROR, "Failed to find dma buffer"));
		goto out;
	}

	rb_erase(&psBuf->node, psState->buffers);

	dma_buf_put(psDmaBuf);

	if (psBuf->psTask)
		put_task_struct(psBuf->psTask);

	OSFreeMemNoStats(psBuf);

out:
	OSLockRelease(psState->hBuffersLock);
}

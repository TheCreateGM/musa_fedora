/*
 * @Copyright Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License Dual MIT/GPLv2
 */

#ifndef __MTGPU_MMAP_H__
#define __MTGPU_MMAP_H__

void mtgpu_vma_open(struct vm_area_struct *vma);
void mtgpu_vma_close(struct vm_area_struct *vma);
int mtgpu_vma_access(struct vm_area_struct *vma, unsigned long addr, void *buf, int len, int write);

int mtgpu_mmap_generic(void *handle, struct vm_area_struct *vma);

#endif /* __MTGPU_MMAP_H__ */

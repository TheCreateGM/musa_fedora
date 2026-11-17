/*
 * @Copyright Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License Dual MIT/GPLv2
 */
#ifndef __MTGPU_STATS_H__
#define __MTGPU_STATS_H__

void mtgpu_stats_add_system_pages(u64 cpu_va, u64 cpu_pa, u32 size,
				  const char *func_name, u32 line_num);
void mtgpu_stats_remove_system_pages(u64 key);
void mtgpu_stats_add_vmalloc(u64 cpu_va, u64 cpu_pa, u32 size,
			     const char *func_name, u32 line_num);
void mtgpu_stats_remove_vmalloc(u64 key);
void mtgpu_stats_add_kmalloc(u64 cpu_va, u64 cpu_pa, u32 size,
			     const char *func_name, u32 line_num);
void mtgpu_stats_remove_kmalloc(u64 key);

#endif

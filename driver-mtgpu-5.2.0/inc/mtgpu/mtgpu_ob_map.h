/*
 * @Copyright Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License Dual MIT/GPLv2
 */

#ifndef _MTGPU_OB_MAP_H_
#define _MTGPU_OB_MAP_H_
#define MAX_OB_MAP_NR 50

struct mtgpu_ob_map_entry {
	u64 cpu_pa;
	u64 length;
	u64 ob_addr;
};

struct mtgpu_ob_map_table {
	int map_nr;
	struct mtgpu_ob_map_entry maps[MAX_OB_MAP_NR];
};

#endif /*_MTGPU_OB_MAP_H_ */

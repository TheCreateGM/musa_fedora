/*
 * @Copyright   Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License     Dual MIT/GPLv2
 */

#ifndef __DM_HWLIB_COMMON_H__
#define __DM_HWLIB_COMMON_H__

void *dm_hwlib_init(u32 gen_id, u64 bar_addr);
void dm_hwlib_deinit(u32 gen_id, void *hwlib);

#endif /* __DM_HWLIB_COMMON_H__ */

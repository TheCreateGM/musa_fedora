/*
 * @Copyright   Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License     Dual MIT/GPLv2
 */

#ifndef _DISPLAY_FUNC_H_
#define _DISPLAY_FUNC_H_

#include "display_types.h"

struct mtdisp_os_func_tbl {
	u32 (*reg_read)(const void *addr);
	void (*reg_write)(u32 val, void *addr);

	void *(*ioremap)(u64 phys_addr, unsigned long size, bool type);
	void (*iounmap)(void *io_addr, unsigned long size);

	void *(*zalloc)(u64 size);
	void (*free)(void *ptr);
	void *(*memset)(void *ptr, int c, unsigned long n);
	void *(*memzero)(void *ptr, unsigned long n);
	void *(*memcpy)(void *dst, const void *src, unsigned long size);

	void (*udelay)(unsigned long us);
	void (*mdelay)(unsigned long ms);

	int (*aux_lock_create)(void **lock);
	void (*aux_lock)(void *lock);
	void (*aux_unlock)(void *lock);
	void (*aux_lock_destroy)(void *lock);

	void (*mt_pr_err)(const char *fmt, ...);
	void (*mt_pr_warn)(const char *fmt, ...);
	void (*mt_pr_info)(const char *fmt, ...);
	void (*mt_pr_err_ratelimited)(const char *fmt, ...);
	void (*mt_pr_warn_ratelimited)(const char *fmt, ...);
};

/* Funcs table from dm */
void mtdisp_os_func_tbl_init(struct mtdisp_os_func_tbl *os_func_tbl);
void mtdisp_os_func_tbl_deinit(void);
u32 mtdisp_reg_read(void *addr);
void mtdisp_reg_write(void *addr, u32 val);
void *mtdisp_ioremap(u64 phys_addr, unsigned long size, bool type);
void mtdisp_iounmap(void *io_addr, unsigned long size);
void *mtdisp_zalloc(u64 size);
void mtdisp_free(void *ptr);
void mtdisp_memset(void *ptr, int c, u64 n);
void mtdisp_memzero(void *ptr, u64 n);
void mtdisp_memcpy(void *dst, const void *src, unsigned long long size);
void mtdisp_mdelay(unsigned long ms);
void mtdisp_udelay(unsigned long us);
int mtdisp_aux_lock_create(void **lock);
void mtdisp_aux_lock(void *lock);
void mtdisp_aux_unlock(void *lock);
void mtdisp_aux_lock_destroy(void *lock);
int mtdisp_aux_lock_create(void **lock);
void mtdisp_aux_lock(void *lock);
void mtdisp_aux_unlock(void *lock);
void mtdisp_aux_lock_destroy(void *lock);

extern struct mtdisp_os_func_tbl *mtdisp_func;

#endif /* _DISPLAY_FUNC_H_ */

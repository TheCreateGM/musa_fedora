/*
 * @Copyright   Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License     Dual MIT/GPLv2
 */

#ifndef _MTVPU_OSAL_H_
#define _MTVPU_OSAL_H_

#include "os-interface.h"

#define LOG_FIFO_SIZE (1 << 21)   // 2 MiB

enum {MIN_LOG_LEVEL=0, ERR, WARN, INFO, MTDEBUG, TRACE, VDI_TRACE, MAX_LOG_LEVEL};

extern int mtvpu_log_level;
extern struct kfifo *log_fifo;
/* UTC is 8 hours behind local time */
#define LOCALTIME  28800

#define VLOG(level, fmt, ...)									\
do {												\
	if (unlikely((level) <= mtvpu_log_level))						\
		os_pr_info("[mtvpu] " fmt, ##__VA_ARGS__);					\
	if (unlikely((level) == INFO) && likely(log_fifo != NULL)) {				\
		char l_buf[256], l_timebuf[32];							\
		struct mt_tm tm;								\
		int len = 0;									\
		os_ktime_get_real_tm(&tm, LOCALTIME);						\
		os_snprintf(l_timebuf, sizeof(l_timebuf), "%02d %02d %02d:%02d:%02d %ld",	\
			tm.cur_tm_mon, tm.cur_tm_mday, tm.cur_tm_hour, tm.cur_tm_min, 		\
			tm.cur_tm_sec, tm.cur_tm_year);						\
		len = os_snprintf(l_buf, sizeof(l_buf), "[%s] [mtvpu] ", l_timebuf);		\
		len += os_snprintf(l_buf + len, sizeof(l_buf) - len, fmt, ##__VA_ARGS__);	\
		if (len >= sizeof(l_buf) - 1) 							\
			len = sizeof(l_buf) - 1;						\
		os_kfifo_in(log_fifo, l_buf, len);						\
	}											\
} while (0) //no use spin_lock_irqsave

u64 osal_gettime(void);
void *osal_malloc(int size);
void osal_free(void *p);
void osal_memcpy(void *dst, const void *src, int count);
void *osal_memset(void *dst, int val, int count);
int osal_snprintf(char *str, size_t buf_size, const char *format, ...);

#ifndef va_arg
typedef __builtin_va_list va_list;
#define va_start(v, l)		__builtin_va_start(v, l)
#define va_end(v)               __builtin_va_end(v)
#define va_arg(v, l)            __builtin_va_arg(v, l)
#define va_copy(d, s)           __builtin_va_copy(d, s)
#endif
#endif /* _MTVPU_MEM_H_ */

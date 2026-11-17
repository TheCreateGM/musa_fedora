/* SPDX-License-Identifier: MIT */
#ifndef MT_WORKQUEUE_COMPAT_H
#define MT_WORKQUEUE_COMPAT_H

#include <linux/workqueue.h>

static inline unsigned int mt_legacy_workqueue_flags(unsigned int flags)
{
#if defined(OS_ENUM_WQ_PERCPU_EXIST)
	/* The vendor ABI predates explicit WQ_PERCPU. Preserve its bound
	 * default, without changing callers that already select a queue type.
	 * WQ_PERCPU is an enum, so detect it by compiling, not with #ifdef.
	 */
	if (!(flags & (WQ_PERCPU | WQ_UNBOUND)))
		flags |= WQ_PERCPU;
#endif
	return flags;
}

#endif

/*
 * Compatibility definitions for kernels that removed linux/pfn_t.h and
 * changed vmf_insert_mixed() to take an unsigned long PFN.  This driver only
 * constructs PFN values with flags=0, so a plain PFN preserves that path.
 */
#ifndef MTGPU_PFN_T_COMPAT_H
#define MTGPU_PFN_T_COMPAT_H

#include <linux/mm.h>
#include <linux/pfn.h>

typedef unsigned long pfn_t;

static inline pfn_t phys_to_pfn_t(phys_addr_t phys, unsigned long flags)
{
	return PHYS_PFN(phys) | flags;
}

static inline pfn_t pfn_to_pfn_t(unsigned long pfn)
{
	return pfn;
}

static inline unsigned long pfn_t_to_pfn(pfn_t pfn)
{
	return pfn;
}

static inline bool pfn_t_valid(pfn_t pfn)
{
	return pfn_valid(pfn);
}

static inline struct page *pfn_t_to_page(pfn_t pfn)
{
	return pfn_to_page(pfn);
}

#endif

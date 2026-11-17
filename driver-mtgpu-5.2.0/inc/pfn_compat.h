#ifndef MT_PFN_COMPAT_H
#define MT_PFN_COMPAT_H
#if __has_include(<linux/pfn_t.h>)
#include <linux/pfn_t.h>
#else
#include <linux/mm.h>
#include <linux/pfn.h>
/* All vendor callers use flags=0; modern mixed mappings use raw PFNs. */
typedef unsigned long pfn_t;
static inline pfn_t phys_to_pfn_t(phys_addr_t addr, unsigned long flags)
{
	WARN_ON_ONCE(flags);
	return PHYS_PFN(addr);
}
#define pfn_t_to_pfn(pfn) (pfn)
#define pfn_t_to_page(pfn) pfn_to_page(pfn)
#define page_to_pfn_t(page) page_to_pfn(page)
#define pfn_t_valid(pfn) pfn_valid(pfn)
#endif
#endif

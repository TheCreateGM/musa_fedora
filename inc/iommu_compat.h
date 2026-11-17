#ifndef MT_IOMMU_COMPAT_H
#define MT_IOMMU_COMPAT_H
#include <linux/iommu.h>
#include <linux/device/bus.h>
#if defined(OS_IOMMU_PAGING_DOMAIN_CALLABLE) && !defined(OS_IOMMU_DOMAIN_ALLOC_CALLABLE)
/* The binary ABI supplies only a bus. Recreate the old homogeneous-bus
 * semantics, refusing a bus with more than one IOMMU driver owner. */
struct mt_iommu_bus_scan {
	struct device *dev;
	const struct iommu_ops *owner;
	bool ambiguous;
};
static int mt_iommu_scan_device(struct device *dev, void *data)
{
	struct mt_iommu_bus_scan *scan = data;
	struct iommu_domain *domain = iommu_get_domain_for_dev(dev);

	if (!domain)
		return 0;
	if (!scan->dev) {
		scan->dev = get_device(dev);
		scan->owner = domain->owner;
	} else if (domain->owner != scan->owner) {
		scan->ambiguous = true;
	}
	return 0;
}
static inline bool mt_iommu_present(const struct bus_type *bus)
{
	struct mt_iommu_bus_scan scan = {};
	bool found;

	bus_for_each_dev(bus, NULL, &scan, mt_iommu_scan_device);
	found = scan.dev != NULL;
	put_device(scan.dev);
	return found;
}
static inline struct iommu_domain *mt_iommu_domain_alloc(const struct bus_type *bus)
{
	struct mt_iommu_bus_scan scan = {};
	struct iommu_domain *domain = NULL;

	bus_for_each_dev(bus, NULL, &scan, mt_iommu_scan_device);
	if (scan.dev && !scan.ambiguous) {
		domain = iommu_paging_domain_alloc(scan.dev);
		if (IS_ERR(domain))
			domain = NULL;
	} else if (scan.ambiguous) {
		pr_err("mtgpu: cannot select an IOMMU from a heterogeneous bus\n");
	}
	put_device(scan.dev);
	return domain;
}
#else
#define mt_iommu_present iommu_present
#define mt_iommu_domain_alloc iommu_domain_alloc
#endif
#endif

/*
 * @Copyright   Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License     Dual MIT/GPLv2
 */

/*
 * This is common os interfaces.
 */

#include "iommu_compat.h"
#include <linux/sched/signal.h>
#include <linux/compiler.h>
#include <linux/types.h>
#include <linux/sysfs.h>
#include <linux/debugfs.h>
#include <linux/stacktrace.h>
#include <asm/ptrace.h>
#include <drm/drm_modes.h>
#include <drm/drm_print.h>
#include <drm/drm_property.h>
#include <drm/drm_color_mgmt.h>
#include <linux/bits.h>
#include <linux/bitops.h>
#include <linux/suspend.h>
#include <linux/aer.h>
#include <linux/delay.h>
#include <linux/dma-buf.h>
#include <linux/fb.h>
#include <linux/firmware.h>
#include <linux/irq.h>
#include <linux/jiffies.h>
#include <linux/kthread.h>
#include <linux/mm.h>
#include <linux/mm_types.h>
#if defined(OS_LINUX_MAPLE_TREE_H_EXIST)
#include <linux/maple_tree.h>
#endif
#include <linux/msi.h>
#include <linux/mutex.h>
#include <linux/notifier.h>
#include <linux/pci.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/vmalloc.h>
#include <linux/mempool.h>
#include <linux/wait.h>
#include <linux/workqueue.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include <linux/semaphore.h>
#include <linux/dma-mapping.h>
#include <linux/dma-fence.h>
#include <linux/dma-fence-array.h>
#include <linux/of_device.h>
#include <linux/of.h>
#include <linux/log2.h>
#include <linux/poll.h>
#include <linux/kref.h>
#include <linux/sched/task.h>
#include <linux/sched/mm.h>
#include <linux/pid.h>
#include <linux/dcache.h>
#include <linux/seq_file.h>
#include <linux/proc_fs.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/rbtree.h>
#include <linux/interval_tree.h>
#include <linux/iommu.h>
#if defined(OS_LINUX_DMA_MAP_OPS_H_EXIST)
#include <linux/dma-map-ops.h>
#endif
#include <linux/iova.h>
#if defined(OS_LINUX_DMA_IOMMU_H_EXIST)
#include <linux/dma-iommu.h>
#endif
#include <linux/random.h>
#include <linux/device.h>
#include <linux/ktime.h>
#include <linux/timekeeping.h>
#include <linux/crypto.h>
#include <linux/sort.h>
#include <linux/list_sort.h>
#include <linux/kfifo.h>
#include <crypto/hash.h>
#include <crypto/skcipher.h>
#include <video/display_timing.h>
#include <linux/sched/signal.h>
#include <linux/radix-tree.h>
#include <linux/utsname.h>
#include <linux/dmi.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/anon_inodes.h>
#include <linux/sync_file.h>
#include <linux/of.h>
#include <linux/pm_domain.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/acpi.h>
#include <linux/pm_runtime.h>
#include <acpi/acpixf.h>
#include <asm-generic/bitsperlong.h>
#include <linux/namei.h>
#include <linux/mount.h>
#include <linux/crc32.h>
#include <linux/devfreq.h>
#include <linux/thermal.h>
#if defined(OS_LINUX_FIND_H_EXIST)
#include <linux/find.h>
#else
#include <asm-generic/bitops/find.h>
#endif
#if defined(OS_LINUX_DMA_RESV_H_EXIST)
#include <linux/dma-resv.h>
#else
#include <linux/reservation.h>
#endif
#include <linux/genalloc.h>
#include <linux/netlink.h>
#include <net/genetlink.h>
#include <net/sock.h>
#if defined(OS_LINUX_M1000_SMMU_V3_H_EXIST)
#include <linux/m1000-smmu-v3.h>
#endif
#if defined(OS_LINUX_SVA_H_EXIST)
#include <linux/sva.h>
#endif
#if defined(OS_LINUX_SCHED_CLOCK_H_EXIST)
#include <linux/sched/clock.h>
#endif
#if !defined(OS_FUNC_SCHED_SET_FIFO_LOW_EXIST)
#include <uapi/linux/sched/types.h>
#endif
#include <linux/plist.h>
#if __has_include(<linux/pfn_t.h>)
#include <linux/pfn_t.h>
#else
#include "pfn_t_compat.h"
#endif
#include <linux/pfn.h>

#include "mtgpu_stats.h"
#include "mtgpu_device.h"
#include "mtgpu_ob.h"
#include "os-interface.h"
#if defined(SUPPORT_ION)
#include "ion/ion.h"
#include "ion_lma_heap.h"
#endif
#include "ion/ion_uapi.h"

#ifdef CONFIG_X86
#include <asm/cpufeature.h>
#include <linux/set_memory.h>
#endif

#ifndef untagged_addr
#include <linux/bitops.h>
#define __untagged_addr(addr)	\
	((__force __typeof__(addr))sign_extend64((__force u64)(addr), 55))

#define untagged_addr(addr)	({					\
	u64 __addr = (__force u64)(addr);				\
	__addr &= __untagged_addr(__addr);				\
	(__force __typeof__(addr))__addr;				\
})
#endif

#ifndef PCI_STATUS_ERROR_BITS

#define PCI_STATUS_ERROR_BITS ((PCI_STATUS_DETECTED_PARITY)  | \
			       (PCI_STATUS_SIG_SYSTEM_ERROR) | \
			       (PCI_STATUS_REC_MASTER_ABORT) | \
			       (PCI_STATUS_REC_TARGET_ABORT) | \
			       (PCI_STATUS_SIG_TARGET_ABORT) | \
			       (PCI_STATUS_PARITY))
#endif

#define TIMESPEC64_TO_NSEC(ts) ((ts).tv_sec * 1000000000ULL + (ts).tv_nsec)

#ifndef GENPD_STATE_ON
#define GENPD_STATE_ON 0
#endif

#ifndef GENPD_STATE_OFF
#define GENPD_STATE_OFF 1
#endif

struct mt_work_struct {
	struct work_struct work;
	void *data;
};

struct mt_delayed_work_struct {
	struct delayed_work dwork;
	void *data;
};

struct mt_dma_fence {
	struct dma_fence dma_fence;
	void *data;
};

struct mt_miscdevice {
	struct miscdevice miscdev;
	void *data;
};

struct mt_affinity_notify {
	struct irq_affinity_notify affinity_notify;
	void *data;
};

const u64 os_value[] = {
#define X(VALUE) VALUE,
	DECLEAR_OS_ERROR_VALUE
	OS_ERROR_MAX,
	DECLEAR_OS_NLA_ATTR_VALUE
	OS_NLA_ATTR_VALUE_MAX,
	DECLEAR_OS_VALUE
#undef X
};

void mtgpu_vma_open(struct vm_area_struct *vma);
void mtgpu_vma_close(struct vm_area_struct *vma);
int mtgpu_vma_access(struct vm_area_struct *vma, unsigned long addr, void *buf, int len, int write);

const struct vm_operations_struct g_vma_ops = {
	.open = mtgpu_vma_open,
	.close = mtgpu_vma_close,
	.access = mtgpu_vma_access,
};

u32 os_get_bits_per_long(void)
{
	return BITS_PER_LONG;
}

int os_num_online_cpus(void)
{
	return num_online_cpus();
}

struct file *os_anon_inode_getfile(const char *name, const struct mt_file_operations *ops, void *priv,
				   int flags)
{
	struct file_operations *fops;

	fops = kzalloc(sizeof(*fops), GFP_KERNEL);
	if (!fops)
		return NULL;

	fops->owner		= THIS_MODULE;
	fops->open		= ops->open;
	fops->read		= ops->read;
	fops->write		= ops->write;
	fops->unlocked_ioctl	= ops->unlocked_ioctl;
	fops->mmap		= ops->mmap;
	fops->poll		= ops->poll;
	fops->release		= ops->release;
	fops->compat_ioctl	= ops->compat_ioctl;

	return anon_inode_getfile(name, fops, priv, flags);
}

void os_kfree_fops(struct file *filp)
{
	kfree(filp->f_op);
}

int os_stream_open(struct inode *inode, struct file *filp)
{
#ifdef OS_FUNC_STREAM_OPEN_EXIST
	return stream_open(inode, filp);
#else
	return nonseekable_open(inode,filp);
#endif
}

void os_get_file(struct file *filp)
{
	get_file(filp);
}

void os_fput(struct file *filp)
{
	fput(filp);
}

IMPLEMENT_OS_STRUCT_COMMON_FUNCS(interval_tree_node);

unsigned long os_get_interval_tree_node_start(struct interval_tree_node *node)
{
	return node->start;
}

void os_set_interval_tree_node_value(struct interval_tree_node *node, u64 start, u64 length)
{
	node->start = start;
	node->last = start + length - 1;
}

void os_interval_tree_clear_node(struct interval_tree_node *node)
{
	struct rb_node *rb = &node->rb;

	RB_CLEAR_NODE(rb);
}

bool os_interval_tree_node_is_empty(struct interval_tree_node *node)
{
	struct rb_node *rb = &node->rb;

	return RB_EMPTY_NODE(rb);
}

void os_interval_tree_insert(struct interval_tree_node *node, struct rb_root_cached *root)
{
	/* This function is generated by INTERVAL_TREE_DEFINE(). */
	interval_tree_insert(node, root);
}

void os_interval_tree_remove(struct interval_tree_node *node, struct rb_root_cached *root)
{
	/* This function is generated by INTERVAL_TREE_DEFINE(). */
	interval_tree_remove(node, root);
}

struct interval_tree_node *os_interval_tree_iter_next(struct interval_tree_node *node,
						      unsigned long start, unsigned long last)
{
	/* This function is generated by INTERVAL_TREE_DEFINE(). */
	return interval_tree_iter_next(node, start, last);
}

struct interval_tree_node *os_interval_tree_iter_first(struct rb_root_cached *root,
						       unsigned long start, unsigned long last)
{
	/* This function is generated by INTERVAL_TREE_DEFINE(). */
	return interval_tree_iter_first(root, start, last);
}

IMPLEMENT_OS_STRUCT_COMMON_FUNCS(rb_root_cached);

void os_rb_root_init(struct rb_root_cached *root)
{
	*root = RB_ROOT_CACHED;
}

IMPLEMENT_OS_STRUCT_COMMON_FUNCS(rb_node);

struct rb_node *os_rb_first_cached(struct rb_root_cached *rb_root)
{
	return rb_first_cached(rb_root);
}

struct rb_node *os_rb_next(const struct rb_node *node)
{
	return rb_next(node);
}

void OS_RB_CLEAR_NODE(struct rb_node *node)
{
	RB_CLEAR_NODE(node);
}

bool OS_RB_EMPTY_NODE(struct rb_node *node)
{
	return RB_EMPTY_NODE(node);
}

bool OS_RB_EMPTY_ROOT(struct rb_root *root)
{
	return RB_EMPTY_ROOT(root);
}

void os_rb_erase_cached(struct rb_node *node, struct rb_root_cached *root)
{
	rb_erase_cached(node, root);
}

void os_rb_add_cached(struct rb_node *node, struct rb_root_cached *tree,
		      bool (*less)(struct rb_node *, const struct rb_node *))
{
	struct rb_node **link = &tree->rb_root.rb_node;
	struct rb_node *parent = NULL;
	bool leftmost = true;

	while (*link) {
		parent = *link;
		if (less(node, parent)) {
			link = &parent->rb_left;
		} else {
			link = &parent->rb_right;
			leftmost = false;
		}
	}

	rb_link_node(node, parent, link);
	rb_insert_color_cached(node, tree, leftmost);
}

struct shash_desc *os_create_shash_desc(struct crypto_shash *tfm)
{
	struct shash_desc *desc;

	desc = kmalloc(sizeof(*desc) + crypto_shash_descsize(tfm),
		       GFP_KERNEL);
	desc->tfm = tfm;

	return desc;
}

void os_destroy_shash_desc(struct shash_desc *desc)
{
	os_kfree(desc);
}

struct crypto_shash *os_crypto_alloc_shash(const char *alg_name, u32 type, u32 mask)
{
	return crypto_alloc_shash(alg_name, type, mask);
}

int os_crypto_shash_digest(struct shash_desc *desc, const u8 *data, unsigned int len, u8 *out)
{
	return crypto_shash_digest(desc, data, len, out);
}

void os_crypto_free_shash(struct crypto_shash *tfm)
{
	crypto_free_shash(tfm);
}

#if defined(SUPPORT_ION)
size_t os_ion_query_heaps_kernel(struct ion_device *idev, struct ion_heap_data *hdata, size_t size)
{
	return ion_query_heaps_kernel(idev, hdata, size);
}

void os_ion_dev_lma_heap_destroy(struct ion_device *dev, const char *name)
{
	struct ion_heap *this, *next;

	plist_for_each_entry_safe(this, next, &dev->heaps, node) {
		if (!strncmp(this->name, name, strlen(this->name) - 1))
			kfree(this->name);
		ion_lma_heap_destroy(this);
	}
}

struct ion_platform_heap *os_ion_platform_heap_create(void)
{
	return kzalloc(sizeof(struct ion_platform_heap), GFP_KERNEL);
}

void os_ion_platform_heap_init(struct ion_platform_heap *heap, u32 type, unsigned int id,
			       const char *name, phys_addr_t vram_base)
{
	heap->type = type;
	heap->id = id;
	heap->name = name;
	heap->vram_base = vram_base;
}

int os_ion_device_add_heap(struct ion_device *idev, struct ion_heap *heap)
{
	return ion_device_add_heap(idev, heap);
}

struct ion_device *os_ion_device_create(void)
{
	return ion_device_create();
}

int os_ion_device_destroy(struct ion_device *idev)
{
	return ion_device_destroy(idev);
}

const char *os_ion_get_dev_name(struct ion_device *idev)
{
	return idev->dev.name;
}

struct ion_heap_data *os_alloc_ion_heap_data_array(u32 count)
{
	return kzalloc(sizeof(struct ion_heap_data) * count, GFP_KERNEL);
}

char *os_get_ion_heap_name(struct ion_heap_data *data, int i)
{
	return data[i].name;
}

u32 os_get_ion_heap_id(struct ion_heap_data *data, int i)
{
	return data[i].heap_id;
}

#endif /*SUPPORT_ION*/

int os_sysfs_create_file(struct kobject *kobj, const struct attribute *attr)
{
	return sysfs_create_file(kobj, attr);
}

void os_sysfs_remove_file(struct kobject *kobj, const struct attribute *attr)
{
	sysfs_remove_file(kobj, attr);
}

struct dentry *os_debugfs_create_dir(const char *name, struct dentry *parent)
{
	return debugfs_create_dir(name, parent);
}

void os_debugfs_remove_recursive(struct dentry *dentry)
{
	debugfs_remove_recursive(dentry);
}

struct dentry *os_debugfs_create_file(const char *name, umode_t mode,
				      struct dentry *parent, void *data,
				      const struct file_operations *fops)
{
	return debugfs_create_file(name, mode, parent, data, fops);
}

bool os_is_power_of_2(unsigned long n)
{
	return is_power_of_2(n);
}

void *os_dev_get_drvdata(const struct device *dev)
{
	return dev_get_drvdata(dev);
}

void os_dev_set_drvdata(struct device *dev, void *data)
{
	dev_set_drvdata(dev, data);
}

void *os_dev_get_platdata(const struct device *dev)
{
	return dev_get_platdata(dev);
}

struct device *os_get_device(struct device *dev)
{
	return get_device(dev);
}

void os_put_device(struct device *dev)
{
	return put_device(dev);
}

int os_device_attach(struct device *dev)
{
	return device_attach(dev);
}

void os_device_release_driver(struct device *dev)
{
	return device_release_driver(dev);
}

const void *os_device_get_match_data(struct device *dev)
{
	return device_get_match_data(dev);
}

struct kobject *os_get_device_kobj(struct device *dev)
{
	return &dev->kobj;
}

void os_device_lock(struct device *dev)
{
	device_lock(dev);
}

void os_device_unlock(struct device *dev)
{
	device_unlock(dev);
}

int os_device_attr_create(struct device_attribute **dev_attr, const char *name, umode_t mode,
			  ssize_t (*show)(struct device *dev,
					  struct device_attribute *attr,
					  char *buf),
			  ssize_t (*store)(struct device *dev,
					   struct device_attribute *attr,
					   const char *buf,
					   size_t count))
{
	*dev_attr = kzalloc(sizeof(struct device_attribute), GFP_KERNEL);
	if (!(*dev_attr))
		return -ENOMEM;

	(*dev_attr)->attr.name = name;
	(*dev_attr)->attr.mode = mode;
	(*dev_attr)->show = show;
	(*dev_attr)->store = store;

	return 0;
}

void os_device_attr_destroy(struct device_attribute *dev_attr)
{
	kfree(dev_attr);
}

int os_device_bypass_smmu(struct device *dev)
{
#if defined(OS_FUNC_DEVICE_BYPASS_SMMU_EXIST)
	return device_bypass_smmu(dev);
#else
	/* Missing device_bypass_smmu().
	 * It's implemented in apollo-linux-kernel.
	 */
	dev_err(dev, "kernel doesn't support bypass SMMU!");
	return -EEXIST;
#endif
}

int os_device_property_read_u32(struct device *dev, const char *propname, u32 *val)
{
	return device_property_read_u32(dev, propname, val);
}

struct attribute *os_get_device_attr_attr(struct device_attribute *dev_attr)
{
	return &dev_attr->attr;
}

bool os_dev_is_pci(struct device *dev)
{
	return dev_is_pci(dev);
}

int os_dev_to_node(struct device *dev)
{
	return dev_to_node(dev);
}

struct pci_dev *os_to_pci_dev(struct device *dev)
{
	return container_of(dev, struct pci_dev, dev);
}

const char *os_pci_name(struct pci_dev *pdev)
{
	return pci_name(pdev);
}

int os_in_interrupt(void)
{
	return in_interrupt();
}

int os_in_atomic(void)
{
	return in_atomic();
}

int os_irqs_disabled(void)
{
	return irqs_disabled();
}

int os_find_first_bit(const unsigned long *p, unsigned int size)
{
	return find_first_bit(p, size);
}

int os_find_next_bit(const unsigned long *p, int size, int offset)
{
	return find_next_bit(p, size, offset);
}

void *os_kmalloc_record(size_t size, const char *func_name, u32 line_num)
{
	void *cpu_vaddr = kmalloc(size, GFP_KERNEL);

	if (cpu_vaddr)
		mtgpu_stats_add_kmalloc((u64)cpu_vaddr, 0, ksize(cpu_vaddr), func_name, line_num);

	return cpu_vaddr;
}

void *os_kmalloc_array_record(size_t n, size_t size, const char *func_name, u32 line_num)
{
	void *cpu_vaddr = kmalloc_array(n, size, GFP_KERNEL);

	if (cpu_vaddr)
		mtgpu_stats_add_kmalloc((u64)cpu_vaddr, 0, ksize(cpu_vaddr), func_name, line_num);

	return cpu_vaddr;
}

void *os_kmalloc_atomic(size_t size)
{
	return kmalloc(size, GFP_ATOMIC);
}

void *os_kzalloc_record(size_t size, const char *func_name, u32 line_num)
{
	void *cpu_vaddr = kzalloc(size, GFP_KERNEL);

	if (cpu_vaddr)
		mtgpu_stats_add_kmalloc((u64)cpu_vaddr, 0, ksize(cpu_vaddr), func_name, line_num);

	return cpu_vaddr;
}

void os_kfree_record(const void *ptr, bool record)
{
	if (record)
		mtgpu_stats_remove_kmalloc((u64)ptr);

	kfree(ptr);
}

void *os_kcalloc_record(size_t n, size_t size, const char *func_name, u32 line_num)
{
	void *cpu_vaddr = kcalloc(n, size, GFP_KERNEL);

	if (cpu_vaddr)
		mtgpu_stats_add_kmalloc((u64)cpu_vaddr, 0, ksize(cpu_vaddr), func_name, line_num);

	return cpu_vaddr;
}

void *os_devm_kzalloc(struct device *dev, size_t size)
{
	return devm_kzalloc(dev, size, GFP_KERNEL);
}

void *os_vmalloc_record(unsigned long size, const char *func_name, u32 line_num)
{
	void *cpu_vaddr = vmalloc(size);

	if (cpu_vaddr) {
		u32 record_size = ((size + OS_VAL(PAGE_SIZE) - 1) & ~(OS_VAL(PAGE_SIZE) - 1));

		mtgpu_stats_add_vmalloc((u64)cpu_vaddr, 0, record_size, func_name, line_num);
	}

	return cpu_vaddr;
}

void *os_vzalloc_record(unsigned long size, const char *func_name, u32 line_num)
{
	void *cpu_vaddr = vzalloc(size);

	if (cpu_vaddr) {
		u32 record_size = ((size + OS_VAL(PAGE_SIZE) - 1) & ~(OS_VAL(PAGE_SIZE) - 1));

		mtgpu_stats_add_vmalloc((u64)cpu_vaddr, 0, record_size, func_name, line_num);
	}

	return cpu_vaddr;
}

void *os_kvzalloc_record(size_t size, const char *func_name, u32 line_num)
{
	void *cpu_vaddr = kvzalloc(size, GFP_KERNEL);

	if (cpu_vaddr) {
		if (is_vmalloc_addr(cpu_vaddr)) {
			u32 record_size = ((size + OS_VAL(PAGE_SIZE) - 1) &
					   ~(OS_VAL(PAGE_SIZE) - 1));

			mtgpu_stats_add_vmalloc((u64)cpu_vaddr, 0, record_size,
						func_name, line_num);
		} else {
			mtgpu_stats_add_kmalloc((u64)cpu_vaddr, 0, ksize(cpu_vaddr),
						func_name, line_num);
		}
	}

	return cpu_vaddr;
}

void os_kvfree(const void *addr)
{
	if (!is_vmalloc_addr(addr))
		os_kfree(addr);
	else
		os_vfree(addr);
}

void os_vfree(const void *addr)
{
	mtgpu_stats_remove_vmalloc((u64)addr);

	vfree(addr);
}

unsigned long os_phys_to_pfn(phys_addr_t pa)
{
	return PHYS_PFN(pa);
}

unsigned long os_page_to_pfn(struct page *page)
{
	return page_to_pfn(page);
}

struct page *os_pfn_to_page(u64 pfn)
{
	return pfn_to_page(pfn);
}

bool os_pfn_valid(u64 pfn)
{
	return pfn_valid(pfn);
}

void os_get_page(struct page *page)
{
	get_page(page);
}

int os_page_count(struct page *page)
{
	return page_count(page);
}

u64 os_page_to_phys(struct page *page)
{
	return page_to_phys(page);
}

u64 *os_page_to_virt(struct page *page)
{
	return page_to_virt(page);
}

struct page *os_virt_to_page(void *addr)
{
	return virt_to_page(addr);
}

struct page *os_vmalloc_to_page(const void *vmalloc_addr)
{
	return vmalloc_to_page(vmalloc_addr);
}

struct page *os_phys_to_page(phys_addr_t pa)
{
#if !defined(CONFIG_ARM) && !defined(CONFIG_ARM64)
	return pfn_to_page(pa >> PAGE_SHIFT);
#else
	return phys_to_page(pa);
#endif
}

struct page *os_get_page_by_index(struct page *pages, u32 index)
{
	return pages + index;
}

int os_sg_table_create(struct sg_table **sgt)
{
	*sgt = kzalloc(sizeof(struct sg_table), GFP_KERNEL);

	if (!(*sgt))
		return -ENOMEM;
	else
		return 0;
}

void os_sg_table_destroy(struct sg_table *sgt)
{
	kfree(sgt);
}

/* get sg_table member */
IMPLEMENT_GET_OS_MEMBER_FUNC(sg_table, sgl);
IMPLEMENT_GET_OS_MEMBER_FUNC(sg_table, nents);
IMPLEMENT_GET_OS_MEMBER_FUNC(sg_table, orig_nents);

struct scatterlist *os_sg_next(struct scatterlist *sg)
{
	return sg_next(sg);
}

dma_addr_t os_sg_dma_address(struct scatterlist *sg)
{
	return sg_dma_address(sg);
}

unsigned int os_sg_dma_len(struct scatterlist *sg)
{
	return sg_dma_len(sg);
}

unsigned int os_sg_offset(struct scatterlist *sg)
{
	return sg->offset;
}

unsigned int os_sg_len(struct scatterlist *sg)
{
	return sg->length;
}

dma_addr_t os_sg_phys(struct scatterlist *sg)
{
	return sg_phys(sg);
}

void os_set_sg_dma_address(struct scatterlist *sg, dma_addr_t dev_addr)
{
	sg_dma_address(sg) = dev_addr;
}

struct page *os_sg_page(struct scatterlist *sg)
{
	return sg_page(sg);
}

void os_set_sg_dma_len(struct scatterlist *sg, unsigned int dma_len)
{
	sg_dma_len(sg) = dma_len;
}

void os_set_sg_page(struct scatterlist *sg, struct page *page,
		    unsigned int len, unsigned int offset)
{
	sg_set_page(sg, page, len, offset);
}

int os_sg_alloc_table_from_pages(struct sg_table *sgt, struct page **pages,
				 unsigned int n_pages, unsigned int offset,
				 unsigned long size)
{
	return sg_alloc_table_from_pages(sgt, pages, n_pages, offset, size, GFP_KERNEL);
}

int os_sg_alloc_table(struct sg_table *sgt, unsigned int nents)
{
	return sg_alloc_table(sgt, nents, GFP_KERNEL);
}

void os_sg_free_table(struct sg_table *sgt)
{
	sg_free_table(sgt);
}

void os_get_current_task_comm(char *to, int size)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 13, 0)
	WARN_ON(size < TASK_COMM_LEN);
	sized_strscpy_pad(to, current->comm, size);
#else
	__get_task_comm(to, size, current);
#endif
}

void *os_vmap(struct page **pages, unsigned int count)
{
	return vmap(pages, count, VM_MAP, pgprot_noncached(PAGE_KERNEL));
}

void *os_vmap_cache(struct page **pages, unsigned int count)
{
	return vmap(pages, count, VM_MAP, PAGE_KERNEL);
}

void os_vunmap(const void *addr)
{
	vunmap(addr);
}

int os_vmf_insert_mixed(struct vm_area_struct *vma, unsigned long addr, u64 pfn)
{
#if defined(OS_FUNC_VMF_INSERT_MIXED_EXIST)
	vm_fault_t vmf;

	vmf = vmf_insert_mixed(vma, addr, pfn_to_pfn_t(pfn));
	return vmf & VM_FAULT_ERROR ? vm_fault_to_errno(vmf, 0) : 0;
#else
	return vm_insert_mixed(vma, addr, PHYS_PFN(phys_addr));
#endif
}

int os_vm_insert_page(struct vm_area_struct *vma, unsigned long addr, struct page *page)
{
	return vm_insert_page(vma, addr, page);
}

int os_remap_pfn_range(struct vm_area_struct *vma, unsigned long addr,
		       unsigned long pfn, unsigned long size, unsigned long pgprot)
{
	pgprot_t prot;

	pgprot_val(prot) = pgprot;

	return remap_pfn_range(vma, addr, pfn, size, prot);
}

void os_unmap_pfn_range(struct vm_area_struct *vma, unsigned long address, unsigned long size)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 12, 0)
	/* zap_vma_ptes() was replaced by the special-VMA zap API. */
	zap_special_vma_range(vma, address, size);
#else
	zap_vma_ptes(vma, address, size);
#endif
}

int os_dma_map_sg(struct device *dev, struct scatterlist *sg,
		  int nents, u64 dir)
{
	return dma_map_sg_attrs(dev, sg, nents, (enum dma_data_direction)dir, 0);
}

void os_dma_unmap_sg(struct device *dev, struct scatterlist *sg,
		     int nents, u64 dir)
{
	dma_unmap_sg_attrs(dev, sg, nents, (enum dma_data_direction)dir, 0);
}

struct iommu_domain *os_iommu_domain_alloc_identity(struct device *dev)
{
	struct bus_type *bus;
	struct iommu_domain *domain = NULL;

	bus = os_get_dev_bus_type(dev);
	if (!bus)
		return domain;

#if defined(OS_STRUCT_BUS_TYPE_HAS_IOMMU_OPS) && \
	(LINUX_VERSION_CODE > KERNEL_VERSION(6, 6, 10))

	const struct iommu_ops *ops = dev->iommu->iommu_dev->ops;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(7, 0, 0))
	/* 7.x allocates identity domains through the IOMMU ops directly. */
	if (ops->domain_alloc_identity)
		domain = ops->domain_alloc_identity(dev);
	if (domain)
		domain->type = IOMMU_DOMAIN_IDENTITY;
	return domain;
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 8, 0))
	/* Newer kernels use iommu_domain_alloc */
	domain = iommu_domain_alloc(dev->bus);
#else
	domain = ops->domain_alloc(IOMMU_DOMAIN_IDENTITY);

	if (!domain)
		return NULL;

	domain->type = IOMMU_DOMAIN_IDENTITY;
#if defined(OS_STRUCT_IOMMU_DOMAIN_HAS_OPS)
#if defined(OS_STRUCT_IOMMU_OPS_HAS_DEFAULT_DOMAIN_OPS)
	domain->ops = ops->default_domain_ops;
#else
	domain->ops = ops;
#endif
#endif
#endif

	return domain;

#endif

#if defined(OS_STRUCT_BUS_TYPE_HAS_IOMMU_OPS) || (LINUX_VERSION_CODE < \
	    KERNEL_VERSION(6, 6, 0))
	if (bus->iommu_ops == NULL)
		return NULL;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(7, 0, 0))
	if (bus->iommu_ops->domain_alloc_identity)
		domain = bus->iommu_ops->domain_alloc_identity(dev);
	if (domain)
		domain->type = IOMMU_DOMAIN_IDENTITY;
	return domain;
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 8, 0))
	domain = iommu_domain_alloc(bus);
#else
	domain = bus->iommu_ops->domain_alloc(IOMMU_DOMAIN_IDENTITY);
	if (!domain)
		return NULL;

	domain->type = IOMMU_DOMAIN_IDENTITY;
#if defined(OS_STRUCT_IOMMU_DOMAIN_HAS_OPS)
#if defined(OS_STRUCT_IOMMU_OPS_HAS_DEFAULT_DOMAIN_OPS)
	domain->ops = bus->iommu_ops->default_domain_ops;
#else
	domain->ops = bus->iommu_ops;
#endif
#endif
#endif

	return domain;
#else
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(7, 0, 0))
	{
		const struct iommu_ops *iops = dev->iommu->iommu_dev->ops;

		if (iops->domain_alloc_identity)
			domain = iops->domain_alloc_identity(dev);
	}
	if (domain)
		domain->type = IOMMU_DOMAIN_IDENTITY;
	return domain;
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0))
	domain = iommu_domain_alloc(dev->bus);
#else
	const struct iommu_ops *ops = dev->iommu->iommu_dev->ops;

	domain = ops->domain_alloc(IOMMU_DOMAIN_IDENTITY);

	if (!domain)
		return NULL;

	domain->type = IOMMU_DOMAIN_IDENTITY;
#if defined(OS_STRUCT_IOMMU_DOMAIN_HAS_OPS)
#if defined(OS_STRUCT_IOMMU_OPS_HAS_DEFAULT_DOMAIN_OPS)
	domain->ops = ops->default_domain_ops;
#else
	domain->ops = ops;
#endif
#endif
#endif

	return domain;
#endif

}

dma_addr_t os_dma_map_page(struct device *dev, struct page *page, size_t offset,
			   size_t size, u64 dir)
{
	return dma_map_page(dev, page, offset, size, (enum dma_data_direction)dir);
}

void os_dma_unmap_page(struct device *dev, dma_addr_t dma_addr, size_t size, u64 dir)
{
	dma_unmap_page(dev, dma_addr, size, (enum dma_data_direction)dir);
}

dma_addr_t os_dma_map_resource(struct device *dev, phys_addr_t phys_addr,
			       size_t size, u64 dir)
{
	return dma_map_resource(dev, phys_addr, size, (enum dma_data_direction)dir, 0);
}

void os_dma_unmap_resource(struct device *dev, dma_addr_t addr, size_t size, u64 dir)
{
	dma_unmap_resource(dev, addr, size, (enum dma_data_direction)dir, 0);
}

int os_dma_mapping_error(struct device *dev, dma_addr_t dma_addr)
{
	return dma_mapping_error(dev, dma_addr);
}

void os_dma_sync_sg_for_device(struct device *dev, struct scatterlist *sg,
			       int nelems, u64 dir)
{
	dma_sync_sg_for_device(dev, sg, nelems, (enum dma_data_direction)dir);
}

void os_dma_sync_sg_for_cpu(struct device *dev, struct scatterlist *sg,
			    int nelems, u64 dir)
{
	dma_sync_sg_for_cpu(dev, sg, nelems, (enum dma_data_direction)dir);
}

struct pid *os_find_vpid(int nr)
{
	return find_vpid(nr);
}

int os_kill_pid(struct pid *pid, int sig, int priv)
{
	return kill_pid(pid, sig, priv);
}

struct task_struct *os_pid_task(struct pid *pid, int type)
{
	return pid_task(pid, (enum pid_type)type);
}

struct mm_struct *os_get_task_mm(struct task_struct *p)
{
	return p->mm;
}

void os_task_lock(struct task_struct *p)
{
	task_lock(p);
}

void os_task_unlock(struct task_struct *p)
{
	task_unlock(p);
}

void os_get_task_struct(struct task_struct *t)
{
	get_task_struct(t);
}

void os_put_task_struct(struct task_struct *t)
{
	put_task_struct(t);
}

char *os_d_path(const struct path *p, char *param, int size)
{
	return d_path(p, param, size);
}

struct path *os_get_exec_vma_file_path(struct mm_struct *mm)
{
	struct vm_area_struct *vma;

#if defined(OS_LINUX_MAPLE_TREE_H_EXIST)
	MA_STATE(mas, &mm->mm_mt, 0, 0);

	mas_for_each(&mas, vma, ULONG_MAX) {
		if ((vma->vm_flags & VM_EXEC) && vma->vm_file)
			return &vma->vm_file->f_path;
	}
#else
	vma = mm->mmap;

	while (vma) {
		if ((vma->vm_flags & VM_EXEC) && vma->vm_file)
			return &vma->vm_file->f_path;

		vma = vma->vm_next;
	}
#endif
	return NULL;
}

unsigned long os_get_vm_area_struct_vm_page_prot(struct vm_area_struct *vma)
{
	return pgprot_val(vma->vm_page_prot);
}

void os_set_vm_area_struct_vm_page_prot(struct vm_area_struct *vma, unsigned long prot)
{
	pgprot_val(vma->vm_page_prot) = prot;
}

unsigned long os_vm_get_page_prot(struct vm_area_struct *vma)
{
	return pgprot_val(vm_get_page_prot(vma->vm_flags));
}

IMPLEMENT_GET_OS_MEMBER_FUNC(vm_area_struct, vm_start);
IMPLEMENT_GET_OS_MEMBER_FUNC(vm_area_struct, vm_end);
IMPLEMENT_GET_OS_MEMBER_FUNC(vm_area_struct, vm_pgoff);
IMPLEMENT_GET_OS_MEMBER_FUNC(vm_area_struct, vm_file);
IMPLEMENT_GET_OS_MEMBER_FUNC(vm_area_struct, vm_private_data);

IMPLEMENT_SET_OS_MEMBER_FUNC(vm_area_struct, vm_private_data);
IMPLEMENT_SET_OS_MEMBER_FUNC(vm_area_struct, vm_ops);
IMPLEMENT_SET_OS_MEMBER_FUNC(vm_area_struct, vm_start);
IMPLEMENT_SET_OS_MEMBER_FUNC(vm_area_struct, vm_end);

struct vm_area_struct *os_get_vm_fault_vma(struct vm_fault *vmf)
{
	return vmf->vma;
}

unsigned long os_get_vm_fault_address(struct vm_fault *vmf)
{
	return vmf->address;
}

unsigned long os_get_vm_area_struct_vm_flags(struct vm_area_struct *vma)
{
	return vma->vm_flags;
}

void os_set_vm_area_struct_vm_pgoff(struct vm_area_struct *vma, unsigned long vm_pgoff)
{
	vma->vm_pgoff = vm_pgoff;
}

void os_set_vm_area_struct_vm_flags(struct vm_area_struct *vma, unsigned long flag)
{
#if defined(OS_VM_FLAGS_IS_NOT_CONST)
	vma->vm_flags |= flag;
#else
	vm_flags_set(vma, (vm_flags_t)flag);
#endif
}

void os_clear_vm_area_struct_vm_flags(struct vm_area_struct *vma, unsigned long flag)
{
#ifdef OS_VM_FLAGS_IS_NOT_CONST
	vma->vm_flags &= ~flag;
#else
	vm_flags_clear(vma, (vm_flags_t)flag);
#endif
}

void os_set_vm_area_struct_vm_page_prot_writecombine(struct vm_area_struct *vma)
{
	vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);
}

void os_set_vm_area_struct_vm_page_prot_noncached(struct vm_area_struct *vma)
{
	vma->vm_page_prot = pgprot_noncached(vm_get_page_prot(vma->vm_flags));
}

/* only for arm64 vgpu bar2 mmap environment */
void os_set_vm_area_any_uc_flags(struct vm_area_struct *vma)
{
#ifdef VM_ALLOW_ANY_UNCACHED
#if defined(OS_VM_FLAGS_IS_NOT_CONST)
	vma->vm_flags |= VM_ALLOW_ANY_UNCACHED;
#else
	vm_flags_set(vma, VM_ALLOW_ANY_UNCACHED);
#endif
#endif
}

bool os_has_vm_area_any_uc_flags(void)
{
#ifdef VM_ALLOW_ANY_UNCACHED
	return true;
#else
	return false;
#endif
}


void *os_memcpy(void *dst, const void *src, size_t size)
{
	return memcpy(dst, src, size);
}

void os_memcpy_flushcache(void *dst, const void *src, size_t cnt)
{
	memcpy_flushcache(dst, src, cnt);
}

int os_memcmp(const void *buf1, const void *buf2, size_t size)
{
	return memcmp(buf1, buf2, size);
}

void os_memcpy_fromio(void *dst, const void __iomem *src, size_t size)
{
/**
 * There may be two memcpy_fromio() in kernel source code,
 * First is used as EXPORT_SYMBOL in iomem.c, this one contains align
 * check, so we should use it.
 * The other is in io.h, this one does not contain align check, we
 * should handle the unaligned area.
 */
#if defined(OS_IS_EXPORT_SYMBOL_GPL_MEMCPY_FROMIO)
	return memcpy_fromio(dst, src, size);
#else
	while (size && !IS_ALIGNED((unsigned long)src, 8)) {
		*(u8 *)dst = __raw_readb(src);
		src++;
		dst++;
		size--;
	}

	while (size >= 8 && IS_ALIGNED((unsigned long)dst, 8)) {
		*(u64 *)dst = __raw_readq(src);
		src += 8;
		dst += 8;
		size -= 8;
	}

	while (size) {
		*(u8 *)dst = __raw_readb(src);
		src++;
		dst++;
		size--;
	}
#endif
}

void os_memcpy_toio(void __iomem *dst, const void *src, size_t size)
{
/**
 * There may be two memcpy_toio() in kernel source code,
 * First is used as EXPORT_SYMBOL in iomem.c, this one contains align
 * check, so we should use it.
 * The other is in io.h, this one does not contain align check, we
 * should handle the unaligned area.
 */
#if defined(OS_IS_EXPORT_SYMBOL_GPL_MEMCPY_TOIO)
	return memcpy_toio(dst, src, size);
#else
	while (size && !IS_ALIGNED((unsigned long)dst, 8)) {
		__raw_writeb(*(u8 *)src, dst);
		src++;
		dst++;
		size--;
	}

	while (size >= 8 && IS_ALIGNED((unsigned long)src, 8)) {
		__raw_writeq(*(u64 *)src, dst);
		src += 8;
		dst += 8;
		size -= 8;
	}

	while (size) {
		__raw_writeb(*(u8 *)src, dst);
		src++;
		dst++;
		size--;
	}
#endif
}

void os_memset_io(void __iomem *addr, int value, size_t size)
{
	memset_io(addr, value, size);
}

void *os_memset(void *s, int c, size_t count)
{
	return memset(s, c, count);
}

int os_sema_create(struct semaphore **sem, int val)
{
	*sem = kzalloc(sizeof(*(*sem)), GFP_KERNEL);
	if (!(*sem))
		return -ENOMEM;

	sema_init(*sem, val);

	return 0;
}

void os_up(struct semaphore *sem)
{
	up(sem);
}

int os_down_timeout(struct semaphore *sem, long timeout)
{
	return down_timeout(sem, timeout);
}

void os_sema_destroy(struct semaphore *sem)
{
	kfree(sem);
}

int os_completion_create(struct completion **x)
{
	*x = kzalloc(sizeof(*(*x)), GFP_KERNEL);
	if (!(*x))
		return -ENOMEM;

	init_completion(*x);

	return 0;
}

void os_completion_destroy(struct completion *x)
{
	kfree(x);
}

void os_wait_for_completion(struct completion *x)
{
	wait_for_completion(x);
}

unsigned long os_wait_for_completion_timeout(struct completion *x, unsigned long timeout)
{
	return wait_for_completion_timeout(x, timeout);
}

void os_complete(struct completion *x)
{
	complete(x);
}

void os_complete_all(struct completion *x)
{
	complete_all(x);
}

void os_reinit_completion(struct completion *x)
{
	reinit_completion(x);
}

int os_mutex_create(struct mutex **lock)
{
	*lock = kzalloc(sizeof(struct mutex), GFP_KERNEL);
	if (!(*lock))
		return -ENOMEM;

	mutex_init(*lock);

	return 0;
}

void os_mutex_lock(struct mutex *lock)
{
	return mutex_lock(lock);
}

int os_mutex_trylock(struct mutex *lock)
{
	return mutex_trylock(lock);
}

void os_mutex_unlock(struct mutex *lock)
{
	return mutex_unlock(lock);
}

void os_mutex_destroy(struct mutex *lock)
{
	mutex_destroy(lock);
	kfree(lock);
}

int os_mutex_is_locked(struct mutex *lock)
{
	return mutex_is_locked(lock);
}

int os_rw_semaphore_create(struct rw_semaphore **sem)
{
	*sem = kzalloc(sizeof(struct rw_semaphore), GFP_KERNEL);
	if (!(*sem))
		return -ENOMEM;

	init_rwsem(*sem);

	return 0;
}

void os_rw_semaphore_destroy(struct rw_semaphore *sem)
{
	kfree(sem);
}

void os_down_write(struct rw_semaphore *sem)
{
	down_write(sem);
}

void os_up_write(struct rw_semaphore *sem)
{
	up_write(sem);
}

void os_down_read(struct rw_semaphore *sem)
{
	down_read(sem);
}

void os_up_read(struct rw_semaphore *sem)
{
	up_read(sem);
}

bool os_down_read_trylock(struct rw_semaphore *sem)
{
	return down_read_trylock(sem);
}

bool os_down_write_trylock(struct rw_semaphore *sem)
{
	return down_write_trylock(sem);
}

int os_spin_lock_create_record(spinlock_t **lock, const char *func_name, u32 line_num)
{
	*lock = kzalloc(sizeof(spinlock_t), GFP_KERNEL);
	if (!(*lock))
		return -ENOMEM;

	mtgpu_stats_add_kmalloc((u64)*lock, 0, ksize(*lock), func_name, line_num);

	spin_lock_init(*lock);

	return 0;
}

void os_spin_lock(spinlock_t *lock)
{
	return spin_lock(lock);
}

void os_spin_unlock(spinlock_t *lock)
{
	return spin_unlock(lock);
}

void os_spin_lock_irqsave(spinlock_t *lock, unsigned long *flags)
{
	spin_lock_irqsave(lock, *flags);
}

void os_spin_unlock_irqrestore(spinlock_t *lock, unsigned long flags)
{
	return spin_unlock_irqrestore(lock, flags);
}

void os_spin_lock_bh(spinlock_t *lock)
{
	spin_lock_bh(lock);
}

void os_spin_unlock_bh(spinlock_t *lock)
{
	spin_unlock_bh(lock);
}

int os_spin_trylock_irqsave(spinlock_t *lock, unsigned long *flags)
{
	return spin_trylock_irqsave(lock, *flags);
}

void os_spin_lock_destroy(spinlock_t *lock)
{
	os_kfree(lock);
}

int os_kfifo_create(struct kfifo **fifo)
{
	*fifo = kzalloc(sizeof(*(*fifo)), GFP_KERNEL);
	if (!(*fifo))
		return -ENOMEM;

	return 0;
}

int os_kfifo_alloc(struct kfifo *fifo, unsigned int size)
{
	return kfifo_alloc(fifo, size, GFP_KERNEL);
}

void os_kfifo_free(struct kfifo *fifo)
{
	kfifo_free(fifo);
}

unsigned int os_kfifo_in_locked(struct kfifo *fifo, const void *from, const unsigned int len,
				spinlock_t *lock)
{
	return kfifo_in_locked(fifo, from, len, lock);
}

unsigned int os_kfifo_out_locked(struct kfifo *fifo, void *to, unsigned int len,
				 spinlock_t *lock)
{
	return kfifo_out_locked(fifo, to, len, lock);
}

unsigned int os_kfifo_in(struct kfifo *fifo, const void *from, unsigned int len)
{
	return kfifo_in(fifo, from, len);
}

unsigned int os_kfifo_out(struct kfifo *fifo, void *to, unsigned int len)
{
	return kfifo_out(fifo, to, len);
}

int os_kfifo_is_empty(struct kfifo *fifo)
{
	return  kfifo_is_empty(fifo);
}

int os_kfifo_is_full(struct kfifo *fifo)
{
	return  kfifo_is_full(fifo);
}

unsigned long os_get_jiffies(void)
{
	return jiffies;
}

unsigned long os_msecs_to_jiffies(const unsigned int m)
{
	return msecs_to_jiffies(m);
}

unsigned long os_nsecs_to_jiffies(u64 n)
{
	return nsecs_to_jiffies(n);
}

unsigned long os_usecs_to_jiffies(u32 n)
{
	return usecs_to_jiffies(n);
}

unsigned long os_get_tick_us(void)
{
	return 1000000 / HZ;
}

bool os_time_before(unsigned long a, unsigned long b)
{
	return time_before(a, b);
}

void os_get_random_bytes(void *buf, int nbytes)
{
	get_random_bytes(buf, nbytes);
}

struct tasklet_struct *os_create_tasklet(void (*fun)(unsigned long), unsigned long data)
{
	struct tasklet_struct *tasklet = kmalloc(sizeof(*tasklet), GFP_KERNEL);

	if (tasklet)
		tasklet_init(tasklet, fun, data);

	return tasklet;
}

void os_destroy_tasklet(struct tasklet_struct *tasklet)
{
	kfree(tasklet);
}

void os_tasklet_schedule(struct tasklet_struct *tasklet)
{
	tasklet_schedule(tasklet);
}

unsigned long os_nsecs_to_jiffies64(u64 n)
{
	return (unsigned long)nsecs_to_jiffies64(n);
}

static struct mt_work_struct *os_get_mt_work(struct work_struct *work)
{
	return container_of(work, struct mt_work_struct, work);
}

void *os_get_work_drvdata(struct work_struct *work)
{
	struct mt_work_struct *mt_work = os_get_mt_work(work);

	return mt_work->data;
}

void os_set_work_drvdata(struct work_struct *work, void *data)
{
	struct mt_work_struct *mt_work = os_get_mt_work(work);

	mt_work->data = data;
}

void *os_create_work(void)
{
	return kzalloc(sizeof(struct mt_work_struct), GFP_KERNEL);
}

void *os_create_work_atomic(void)
{
	return kzalloc(sizeof(struct mt_work_struct), GFP_ATOMIC);
}

void os_destroy_work(struct work_struct *work)
{
	kfree(work);
}

static struct mt_delayed_work_struct *os_get_mt_dwork(struct delayed_work *dwork)
{
	return container_of(dwork, struct mt_delayed_work_struct, dwork);
}

void *os_get_dwork_drvdata(struct work_struct *work)
{
	struct mt_delayed_work_struct *mt_dwork = os_get_mt_dwork((struct delayed_work *)work);

	return mt_dwork->data;
}

void os_set_dwork_drvdata(struct delayed_work *dwork, void *data)
{
	struct mt_delayed_work_struct *mt_dwork = os_get_mt_dwork(dwork);

	mt_dwork->data = data;
}

void *os_create_dwork(void)
{
	return kzalloc(sizeof(struct mt_delayed_work_struct), GFP_KERNEL);
}

void os_destroy_dwork(struct delayed_work *dwork)
{
	kfree(dwork);
}

void os_init_dwork(struct delayed_work *work, work_func_t func)
{
	INIT_DEFERRABLE_WORK(work, func);
}

void os_init_delayed_work(struct delayed_work *dwork, work_func_t func)
{
	INIT_DELAYED_WORK(dwork, func);
}

bool os_queue_delayed_work(struct workqueue_struct *wq,
			   struct delayed_work *dwork,
			   unsigned long delay)
{
	return queue_delayed_work(wq, dwork, delay);
}

bool os_delayed_work_pending(struct delayed_work *dwork)
{
	return delayed_work_pending(dwork);
}

bool os_cancel_delayed_work_sync(struct delayed_work *dwork)
{
	return cancel_delayed_work_sync(dwork);
}

bool os_cancel_delayed_work(struct delayed_work *dwork)
{
	return cancel_delayed_work(dwork);
}

bool os_mod_delayed_work(struct workqueue_struct *wq,
			 struct delayed_work *dwork,
			 unsigned long delay)
{
	return mod_delayed_work(wq, dwork, delay);
}

struct delayed_work *os_to_delayed_work(struct work_struct *work)
{
	return to_delayed_work(work);
}

unsigned long os_get_delayed_work_timer_expires(struct delayed_work *dwork)
{
	return dwork->timer.expires;
}

__poll_t os_key_to_poll(void *key)
{
	return key_to_poll(key);
}

void os_init_poll_funcptr(poll_table *pt, poll_queue_proc qproc)
{
	init_poll_funcptr(pt, qproc);
}

__poll_t os_vfs_poll(struct file *file, struct poll_table_struct *pt)
{
	return vfs_poll(file, pt);
}

bool os_check_file_exist(const char __user *path)
{
	struct file *filp;
	filp = filp_open(path, O_RDONLY, 0);
	 if (IS_ERR(filp)) {
        	return false;
	} else {
		filp_close(filp, NULL);
		return true;
	}
}

void os_poll_wait(struct file *filp, wait_queue_head_t *wait_address, struct poll_table_struct *p)
{
	poll_wait(filp, wait_address, p);
}

void os_seq_puts(struct seq_file *m, const char *s)
{
	seq_puts(m, s);
}

void os_seq_putc(struct seq_file *m, char c)
{
	seq_putc(m, c);
}

void os_seq_put_decimal_ll(struct seq_file *m, const char *delimiter, long long num)
{
	seq_put_decimal_ll(m, delimiter, num);
}

ssize_t os_seq_read(struct file *file, char __user *buf, size_t size, loff_t *ppos)
{
	return seq_read(file, buf, size, ppos);
}

loff_t os_seq_lseek(struct file *file, loff_t offset, int whence)
{
	return seq_lseek(file, offset, whence);
}

int os_single_open(struct file *file, int (*show)(struct seq_file *, void *), void *data)
{
	return single_open(file, show, data);
}

int os_single_release(struct inode *inode, struct file *file)
{
	return single_release(inode, file);
}

void *os_get_seq_file_private(struct seq_file *seq)
{
	return seq->private;
}

struct proc_dir_entry *os_proc_create_data(const char *name, umode_t mode,
					   struct proc_dir_entry *parent,
					   const void *proc_fops, void *data)
{
#if defined(OS_STRUCT_PROC_OPS_EXIST)
	struct proc_ops *new_proc_ops = (struct proc_ops *)proc_fops;
#else
	struct file_operations *new_proc_ops = (struct file_operations *)proc_fops;
#endif
	return proc_create_data(name, mode, parent, new_proc_ops, data);
}

struct proc_dir_entry *os_proc_create_single_data(const char *name, umode_t mode,
						  struct proc_dir_entry *parent,
						  int (*show)(struct seq_file *, void *),
						  void *data)
{
	return proc_create_single_data(name, mode, parent, show, data);
}

struct proc_dir_entry *os_proc_mkdir(const char *name, struct proc_dir_entry *parent)
{
	return proc_mkdir(name, parent);
}

void os_proc_remove(struct proc_dir_entry *de)
{
	proc_remove(de);
}

void os_remove_proc_entry(const char *name, struct proc_dir_entry *parent)
{
	remove_proc_entry(name, parent);
}

int os_atomic_read_this_module_refcnt(void)
{
	return atomic_read(&THIS_MODULE->refcnt);
}

void os_module_put(void)
{
	module_put(THIS_MODULE);
}

bool os_try_module_get(void)
{
	return try_module_get(THIS_MODULE);
}

struct module *OS_THIS_MODULE(void)
{
	return THIS_MODULE;
}

void *os_pde_data(const struct inode *inode)
{
#if defined(OS_FUNC_PDE_DATA_EXIST)
	return PDE_DATA(inode);
#else
	return pde_data(inode);
#endif
}

void *os_miscdevice_create(void)
{
	return kzalloc(sizeof(struct mt_miscdevice), GFP_KERNEL);
}

int os_miscdevice_init(struct miscdevice *misc, struct device *parent,
		       const char *name, int mode, const struct mt_file_operations *ops)
{
	struct file_operations *fops;

	fops = kzalloc(sizeof(*fops), GFP_KERNEL);
	if (!fops)
		return -ENOMEM;

	misc->name = kstrdup(name, GFP_KERNEL);
	if (!misc->name) {
		kfree(fops);
		return -ENOMEM;
	}

	misc->parent = parent;
	misc->mode = mode;

	fops->owner		= THIS_MODULE;
	fops->open		= ops->open;
	fops->read		= ops->read;
	fops->write		= ops->write;
	fops->unlocked_ioctl	= ops->unlocked_ioctl;
	fops->mmap		= ops->mmap;
	fops->poll		= ops->poll;
	fops->release		= ops->release;

	/* each miscdevice has it own fops */
	misc->fops = fops;
	return 0;
}

void os_miscdevice_deinit(struct miscdevice *misc)
{
	/* each miscdevice has it own fops */
	kfree(misc->name);
	kfree(misc->fops);
}

void os_miscdevice_destroy(struct miscdevice *misc)
{
	kfree(misc);
}

int os_misc_register(struct miscdevice *misc)
{
	return misc_register(misc);
}

void os_misc_deregister(struct miscdevice *misc)
{
	misc_deregister(misc);
}

void os_set_miscdevice_minor(struct miscdevice *misc, int minor)
{
	misc->minor = minor;
}

void *os_get_miscdevice_drvdata(struct miscdevice *misc)
{
	struct mt_miscdevice *mt_misc = (struct mt_miscdevice *)misc;

	return mt_misc->data;
}

void os_set_miscdevice_drvdata(struct miscdevice *misc, void *data)
{
	struct mt_miscdevice *mt_misc = (struct mt_miscdevice *)misc;

	mt_misc->data = data;
}

const char *os_get_miscdevice_name(struct miscdevice *misc)
{
	return misc->name;
}

struct device *os_get_miscdevice_parent(struct miscdevice *misc)
{
	return misc->parent;
}

int os_get_miscdevice_minor(struct miscdevice *misc)
{
	return misc->minor;
}

int os_get_miscdevice_dynamic_minor(void)
{
	return MISC_DYNAMIC_MINOR;
}

struct device *os_get_miscdevice_this_device(struct miscdevice *misc)
{
	return misc->this_device;
}

void os_set_file_fpos(struct file *file, loff_t f_pos)
{
	file->f_pos = f_pos;
}

loff_t os_get_file_fpos(struct file *file)
{
	return file->f_pos;
}

void os_set_file_private_data(struct file *file, void *private_data)
{
	file->private_data = private_data;
}

void *os_get_file_private_data(struct file *file)
{
	return file->private_data;
}

size_t os_get_file_nlink(struct file *file)
{
	return file->f_path.dentry->d_inode->i_nlink;
}

size_t os_get_file_node_size(const struct file *file)
{
	return file->f_inode->i_size;
}

void *os_get_file_node_private_data(struct file *file)
{
    return file_inode(file)->i_private;
}

unsigned int os_get_file_flags(struct file *file)
{
	return file->f_flags;
}

struct fd *os_fdget(unsigned int fd)
{
	struct fd *f = kzalloc(sizeof(*f), GFP_KERNEL);
	*f = fdget(fd);

	return f;
}

void os_fdput(struct fd *fd)
{
	fdput(*fd);
	kfree(fd);
}

struct file *os_get_fd_file(struct fd *fd)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 12, 0)
	return fd_file(*fd);
#elif defined(OS_FD_FILE_FUNC_EXIST)
	return fd_file(*fd);
#else
	return fd->file;
#endif
}

void os_kref_init(mt_kref *kref)
{
	kref_init((struct kref *)kref);
}

int os_kref_put(mt_kref *kref, void (*release)(mt_kref *kref))
{
	return kref_put((struct kref *)kref, (void *)release);
}

int os_kref_read(mt_kref *kref)
{
	return kref_read((struct kref *)kref);
}

int os_kref_get_unless_zero(mt_kref *kref)
{
	return kref_get_unless_zero((struct kref *)kref);
}

void os_kref_get(mt_kref *kref)
{
	kref_get((struct kref *)kref);
}

int os_ida_create(struct ida **ida)
{
	*ida = kmalloc(sizeof(**ida), GFP_KERNEL);
	if (!(*ida))
		return -ENOMEM;

	ida_init(*ida);

	return 0;
}

void os_ida_destroy(struct ida *ida)
{
	ida_destroy(ida);
	kfree(ida);
}

int os_ida_alloc(struct ida *ida)
{
	return ida_alloc(ida, GFP_KERNEL);
}

int os_ida_alloc_range(struct ida *ida, unsigned int min, unsigned int max, gfp_t gfp)
{
	return ida_alloc_range(ida, min, max, gfp);
}

void os_ida_free(struct ida *ida, unsigned long id)
{
	ida_free(ida, id);
}

bool os_ida_is_empty(const struct ida *ida)
{
	return ida_is_empty(ida);
}

bool os_queue_work(struct workqueue_struct *wq, struct work_struct *work)
{
	return queue_work(wq, work);
}

struct workqueue_struct *os_get_system_wq(void)
{
	return system_wq;
}

void os_destroy_workqueue(struct workqueue_struct *wq)
{
	destroy_workqueue(wq);
}

struct workqueue_struct *os_create_freezable_workqueue(char *name)
{
	return create_freezable_workqueue(name);
}

struct workqueue_struct *os_alloc_workqueue(const char *fmt, unsigned int flags, int max_active)
{
	return alloc_workqueue(fmt, flags, max_active);
}

struct workqueue_struct *os_alloc_ordered_workqueue(const char *fmt)
{
	return create_singlethread_workqueue(fmt);
}

void os_flush_workqueue(struct workqueue_struct *wq)
{
	flush_workqueue(wq);
}

void os_wake_up(struct wait_queue_head *wq_head)
{
	wake_up(wq_head);
}

void os_wake_up_interruptible(struct wait_queue_head *wq_head)
{
	wake_up_interruptible(wq_head);
}

void os_wake_up_all(struct wait_queue_head *wq_head)
{
	wake_up_all(wq_head);
}

int os_create_waitqueue_head(struct wait_queue_head **wq_head)
{
	*wq_head = kzalloc(sizeof(struct wait_queue_head), GFP_KERNEL);
	if (!(*wq_head))
		return -ENOMEM;

	init_waitqueue_head(*wq_head);

	return 0;
}

void os_destroy_waitqueue_head(struct wait_queue_head *wq_head)
{
	kfree(wq_head);
}

int os_signal_pending(void)
{
	return signal_pending(current);
}

void os_might_sleep(void)
{
	might_sleep();
}

void os_schedule(void)
{
	schedule();
}

void os_sched_set_fifo_low(struct task_struct *p)
{
#if defined(OS_FUNC_SCHED_SET_FIFO_LOW_EXIST)
	sched_set_fifo_low(p);
#else
	struct sched_param sparam = {.sched_priority = 1};

	sched_setscheduler(current, SCHED_FIFO, &sparam);
#endif
}

long os_schedule_timeout(long timeout)
{
	return schedule_timeout(timeout);
}

void os_init_wait_entry(struct wait_queue_entry *wq_entry, int flags)
{
	init_wait_entry(wq_entry, flags);
}

void os_finish_wait(struct wait_queue_head *wq_head, struct wait_queue_entry *wq_entry)
{
	finish_wait(wq_head, wq_entry);
}

long os_prepare_to_wait_event_uninterruptible(struct wait_queue_head *wq_head,
					      struct wait_queue_entry *wq_entry)
{
	return prepare_to_wait_event(wq_head, wq_entry, TASK_UNINTERRUPTIBLE);
}

long os_prepare_to_wait_event_interruptible(struct wait_queue_head *wq_head,
					    struct wait_queue_entry *wq_entry)
{
	return prepare_to_wait_event(wq_head, wq_entry, TASK_INTERRUPTIBLE);
}

int os_wait_event_killable(struct wait_queue_head *wq_head, bool condition)
{
	return wait_event_killable(*wq_head, condition);
}

void os_add_wait_queue(struct wait_queue_head *wq_head, struct wait_queue_entry *wq_entry)
{
	add_wait_queue(wq_head, wq_entry);
}

void os_init_waitqueue_func_entry(struct wait_queue_entry *wq_entry, wait_queue_func_t func)
{
	init_waitqueue_func_entry(wq_entry, func);
}

void *os_get_work_func(struct work_struct *work)
{
	return work->func;
}

void os_init_work(struct work_struct *work, work_func_t func)
{
	INIT_WORK(work, func);
}

bool os_flush_work(struct work_struct *work)
{
	return flush_work(work);
}

bool os_schedule_work(struct work_struct *work)
{
	return schedule_work(work);
}

bool os_cancel_work_sync(struct work_struct *work)
{
	return cancel_work_sync(work);
}

bool os_schedule_delayed_work(struct delayed_work *dwork, unsigned long delay)
{
	return schedule_delayed_work(dwork, delay);
}

void os_wmb(void)
{
	wmb(); /* memory barrier */
}

void os_mb(void)
{
	mb(); /* memory barrier */
}

void os_rmb(void)
{
	rmb();/* memory barrier */
}

void os_smp_mb(void)
{
	smp_mb();/* memory barrier */
}

void os_smp_wmb(void)
{
	smp_wmb();/* memory barrier */
}

void os_smp_rmb(void)
{
	smp_rmb();/* memory barrier */
}

int os_smp_load_acquire(int *p)
{
	/* To get the ptr data coherency */
	return smp_load_acquire(p);
}

void os_smp_store_release(int *p, int v)
{
	/* To save the ptr data coherency */
	smp_store_release(p, v);
}

int os_arch_io_reserve_memtype_wc(resource_size_t base, resource_size_t size)
{
	return arch_io_reserve_memtype_wc(base, size);
}

void os_arch_io_free_memtype_wc(resource_size_t base, resource_size_t size)
{
	arch_io_free_memtype_wc(base, size);
}

int os_arch_phys_wc_add(unsigned long base, unsigned long size)
{
	return arch_phys_wc_add(base, size);
}

void os_arch_phys_wc_del(int handle)
{
	arch_phys_wc_del(handle);
}

void __iomem *os_ioremap(phys_addr_t phys_addr, size_t size)
{
	return ioremap(phys_addr, size);
}

void __iomem *os_ioremap_wc(phys_addr_t phys_addr, size_t size)
{
	return ioremap_wc(phys_addr, size);
}

void __iomem *os_ioremap_cache(resource_size_t offset, unsigned long size)
{
	return ioremap_cache(offset, size);
}

void os_iounmap(void __iomem *io_addr)
{
	iounmap(io_addr);
}

int os_get_user_pages_fast(unsigned long start, int nr_pages,
			   unsigned int gup_flags, struct page **pages)
{
	return get_user_pages_fast(start, nr_pages, gup_flags, pages);
}

long os_get_user_pages(unsigned long start, unsigned long nr_pages, unsigned int gup_flags,
		       struct page **pages, struct vm_area_struct **vmas)
{
#ifdef OS_GET_USER_PAGES_USE_VM_AREA_STRUCT
	return get_user_pages(start, nr_pages, gup_flags, pages, vmas);
#else
	(void)vmas;
	return get_user_pages(start, nr_pages, gup_flags, pages);
#endif
}

/* https://git.kernel.org/pub/scm/linux/kernel/git/stable/linux.git/commit/?id=64019a2e467a */
long os_get_user_pages_remote(struct task_struct *tsk, struct mm_struct *mm, unsigned long start,
			      unsigned long nr_pages, unsigned int gup_flags, struct page **pages,
			      struct vm_area_struct **vmas, int *locked)
{
#if defined(OS_GET_USER_PAGES_REMOTE_ARGS_HAS_TASK)
	return get_user_pages_remote(tsk, mm, start, nr_pages, gup_flags, pages, vmas, locked);
#else
#ifdef OS_GET_USER_PAGES_REMOTE_USE_VM_AREA_STRUCT
	return get_user_pages_remote(mm, start, nr_pages, gup_flags, pages, vmas, locked);
#else
	return get_user_pages_remote(mm, start, nr_pages, gup_flags, pages, locked);
#endif
#endif
}

#if defined(OS_FUNC_PIN_USER_PAGES_REMOTE_EXIST)
static long _os_pin_user_pages_remote(struct task_struct *tsk, struct mm_struct *mm,
				      unsigned long start, unsigned long nr_pages,
				      unsigned int gup_flags, struct page **pages,
				      struct vm_area_struct **vmas, int *locked)
{
#if defined(OS_PIN_USER_PAGES_REMOTE_ARGS_HAS_TASK)
	return pin_user_pages_remote(tsk, mm, start, nr_pages, gup_flags, pages, vmas, locked);
#else
#ifdef OS_PIN_USER_PAGES_REMOTE_USE_VM_AREA_STRUCT
	return pin_user_pages_remote(mm, start, nr_pages, gup_flags, pages, vmas, locked);
#else
	return pin_user_pages_remote(mm, start, nr_pages, gup_flags, pages, locked);
#endif
#endif
}
#endif // OS_FUNC_PIN_USER_PAGES_REMOTE_EXIST

long os_pin_user_pages_remote(struct task_struct *tsk, struct mm_struct *mm, unsigned long start,
			      unsigned long nr_pages, unsigned int gup_flags, struct page **pages,
			      struct vm_area_struct **vmas, int *locked)
{
#if defined(OS_FUNC_PIN_USER_PAGES_REMOTE_EXIST)
	return _os_pin_user_pages_remote(tsk, mm, start, nr_pages, gup_flags | FOLL_LONGTERM, pages,
					 vmas, locked);
#else
	return os_get_user_pages_remote(tsk, mm, start, nr_pages, gup_flags, pages, vmas, locked);
#endif
}

void os_unpin_user_pages_dirty_lock(struct page **pages, unsigned long npages, bool make_dirty)
{
#if defined(OS_FUNC_UNPIN_USER_PAGES_DIRTY_LOCK_EXIST)
	unpin_user_pages_dirty_lock(pages, npages, make_dirty);
#else
	unsigned long i;
	for (i = 0; i < npages; i++) {
		if (make_dirty)
			os_set_pages_dirty(pages[i], 1);
		os_put_page(pages[i]);
	}
#endif
}

void os_put_page(struct page *page)
{
	put_page(page);
}

int os_page_reserved(struct page *page)
{
	return PageReserved(page);
}

void os_set_pages_reserved(struct page *pages, int n)
{
	int i;

	for (i = 0; i < n; i++)
		SetPageReserved(pages + i);
}

void os_clear_pages_reserved(struct page *pages, int n)
{
	int i;

	for (i = 0; i < n; i++)
		ClearPageReserved(pages + i);
}

void os_set_pages_dirty(struct page *pages, int n)
{
	int i;

	for (i = 0; i < n; i++)
		SetPageDirty(pages + i);
}

void os_set_pages_wc(struct page **pages, int n)
{
#ifdef CONFIG_X86
	set_pages_array_wc(pages, n);
#endif
}

void os_set_pages_wb(struct page **pages, int n)
{
#ifdef CONFIG_X86
	set_pages_array_wb(pages, n);
#endif
}

void os_set_pages_uc(struct page **pages, int n)
{
#ifdef CONFIG_X86
	set_pages_array_uc(pages, n);
#endif
}

struct vm_area_struct *os_find_vma_intersection(struct mm_struct *mm, unsigned long start_addr,
						unsigned long end_addr)
{
	return find_vma_intersection(mm, start_addr, end_addr);
}

struct vm_area_struct *os_find_vma_intersection_locked(struct mm_struct *mm, unsigned long start_addr,
						       unsigned long end_addr)
{
	struct vm_area_struct *vma = NULL;

#if defined(OS_STRUCT_MM_STRUCT_HAS_MMAP_SEM)
	down_read(&mm->mmap_sem);
	vma =find_vma_intersection(mm, start_addr, end_addr);
	up_read(&mm->mmap_sem);
#elif defined(OS_STRUCT_MM_STRUCT_HAS_MMAP_LOCK)
	down_read(&mm->mmap_lock);
	vma =find_vma_intersection(mm, start_addr, end_addr);
	up_read(&mm->mmap_lock);
#endif

	return vma;
}

struct vm_area_struct *os_find_vma(struct mm_struct *mm, unsigned long addr)
{
	return find_vma(mm, addr);
}

uint64_t os_untagged_addr(u64 addr)
{
	return untagged_addr(addr);
}

int os_follow_pfn(struct vm_area_struct *vma, unsigned long address, unsigned long *pfn)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 10, 0)
	/* follow_pfn() was replaced by the follow_pfnmap() API. */
	struct follow_pfnmap_args args = { .vma = vma, .address = address };
	int ret = follow_pfnmap_start(&args);

	if (ret)
		return ret;
	*pfn = args.pfn;
	follow_pfnmap_end(&args);
	return 0;
#else
	return follow_pfn(vma, address, pfn);
#endif
}

int os_fixup_user_fault(struct task_struct *tsk, struct mm_struct *mm, unsigned long address,
			unsigned int fault_flags, bool *unlocked)
{
#if defined(OS_FIXUP_USER_FAULT_ARGS_HAS_TASK)
	return fixup_user_fault(tsk, mm, address, fault_flags, unlocked);
#else
	return fixup_user_fault(mm, address, fault_flags, unlocked);
#endif
}

struct page *os_alloc_pages_record(gfp_t gfp_mask, unsigned int order,
				   const char *func_name, u32 line_num)
{
	struct page *pages = alloc_pages(gfp_mask, order);

	if (pages)
		mtgpu_stats_add_system_pages(0, page_to_phys(pages),
					     (1U << order) * PAGE_SIZE,
					     func_name, line_num);

	return pages;
}

void os_free_pages(struct page *page, unsigned int order)
{
	mtgpu_stats_remove_system_pages(page_to_phys(page));

	__free_pages(page, order);
}

int os_get_order(unsigned long size)
{
	return get_order(size);
}

u8 os_ioread8(void __iomem *addr)
{
	return ioread8(addr);
}

u16 os_ioread16(void __iomem *addr)
{
	return ioread16(addr);
}

unsigned int os_ioread32(void __iomem *addr)
{
	return ioread32(addr);
}

int os_ioread32_safe(void __iomem *addr, u32 *val)
{
    u32 tmp = os_ioread32(addr);

    if (unlikely(tmp == 0xFFFFFFFF)) {
        os_pr_err("MMIO read failed (PCIe down) addr=%px val=0x%x\n",
                  addr, tmp);
        return -OS_VAL(EIO);
    }

    *val = tmp;
    return 0;
}

u64 os_ioread64(void __iomem *addr)
{
	u32 low, high;

	low = ioread32(addr);
	high = ioread32(addr + 4);

	return ((u64)high << 32) | low;
}

void os_iowrite8(u8 b, void __iomem *addr)
{
	iowrite8(b, addr);
}

void os_iowrite16(u16 b, void __iomem *addr)
{
	iowrite16(b, addr);
}

void os_iowrite32(u32 b, void __iomem *addr)
{
	iowrite32(b, addr);
}

void os_iowrite64(u64 b, void __iomem *addr)
{
	u32 low, high;
	low = b & 0xffffffff;
	high = (b >> 32) & 0xffffffff;

	iowrite32(low, addr);
	iowrite32(high, addr + 4);
}

u8 os_readb(const void __iomem *addr)
{
	return readb(addr);
}

u16 os_readw(const void __iomem *addr)
{
	return readw(addr);
}

u32 os_readl(const void __iomem *addr)
{
	return readl(addr);
}

u64 os_readq(const void __iomem *addr)
{
	u32 low, high;

	low = readl(addr);
	high = readl(addr + 4);

	return ((u64)high << 32) | low;
}

void os_writeb(u8 value, void __iomem *addr)
{
	writeb(value, addr);
}

void os_writew(u16 value, void __iomem *addr)
{
	writew(value, addr);
}

void os_writel(u32 value, void __iomem *addr)
{
	writel(value, addr);
}

void os_writeq(u64 value, void __iomem *addr)
{
	writel((u32)value, addr);
	writel(value >> 32, addr + 4);
}

u32 os_bitrev32(u32 val)
{
	return bitrev32(val);
}

bool os_is_pci_bridge(struct pci_dev *pdev)
{
	return pci_is_bridge(pdev);
}

unsigned int os_pci_is_virtfn(struct pci_dev *pdev)
{
	return pdev->is_virtfn;
}

struct pci_dev *os_get_pci_dev_physfn(struct pci_dev *pdev)
{
	return pdev->physfn;
}

struct device *os_get_pci_device_base(struct pci_dev *pdev)
{
	return &pdev->dev;
}

unsigned short os_get_pci_device_vendor(struct pci_dev *pdev)
{
	return pdev->vendor;
}

unsigned short os_get_pci_subsystem_vendor(struct pci_dev *pdev)
{
	return pdev->subsystem_vendor;
}

unsigned short os_get_pci_device_id(struct pci_dev *pdev)
{
	return pdev->device;
}

unsigned short os_get_pci_subsystem_device_id(struct pci_dev *pdev)
{
	return pdev->subsystem_device;
}

struct pci_bus *os_get_pci_bus(struct pci_dev *pdev)
{
	return pdev->bus;
}

u8 os_get_pci_hdr_type(struct pci_dev *pdev)
{
	return pdev->hdr_type;
}

struct resource *os_get_pci_resource(struct pci_dev *pdev)
{
	return pdev->resource;
}

pci_power_t os_get_pci_current_state(struct pci_dev *pdev)
{
	return pdev->current_state;
}

kernel_ulong_t os_get_pci_device_data(const struct pci_device_id *id)
{
	return id->driver_data;
}

int os_pci_domain_nr(struct pci_dev *pdev)
{
	return pci_domain_nr(pdev->bus);
}

unsigned int os_pci_slot(unsigned int devfn)
{
	return PCI_SLOT(devfn);
}

unsigned int os_pci_func(unsigned int devfn)
{
	return PCI_FUNC(devfn);
}

unsigned int os_get_pci_devfn(struct pci_dev *pdev)
{
	return pdev->devfn;
}

unsigned char os_get_pci_bus_number(struct pci_dev *pdev)
{
	return pdev->bus->number;
}

unsigned int os_get_pci_irq(struct pci_dev *pdev)
{
	return pdev->irq;
}

resource_size_t os_pci_resource_start(struct pci_dev *pdev, int bar)
{
	return pci_resource_start(pdev, bar);
}

resource_size_t os_pci_resource_end(struct pci_dev *pdev, int bar)
{
	return pci_resource_end(pdev, bar);
}

unsigned long os_pci_resource_flags(struct pci_dev *pdev, int bar)
{
	return pci_resource_flags(pdev, bar);
}

resource_size_t os_pci_resource_len(struct pci_dev *pdev, int bar)
{
	return pci_resource_len(pdev, bar);
}

int os_pci_irq_vector(struct pci_dev *dev, unsigned int nr)
{
	return pci_irq_vector(dev, nr);
}

int os_pci_enable_device(struct pci_dev *dev)
{
	return pci_enable_device(dev);
}

void os_pci_disable_device(struct pci_dev *dev)
{
	return pci_disable_device(dev);
}

void os_pci_set_drvdata(struct pci_dev *pdev, void *drvdata)
{
	pci_set_drvdata(pdev, drvdata);
}

void *os_pci_get_drvdata(struct pci_dev *pdev)
{
	return pci_get_drvdata(pdev);
}

void os_pci_set_master(struct pci_dev *dev)
{
	return pci_set_master(dev);
}

void os_pci_clear_master(struct pci_dev *dev)
{
	return pci_clear_master(dev);
}

int os_pci_load_and_free_saved_state(struct pci_dev *dev, struct pci_saved_state **state)
{
	return pci_load_and_free_saved_state(dev, state);
}

int os_pci_load_saved_state(struct pci_dev *dev, struct pci_saved_state *state)
{
	return pci_load_saved_state(dev, state);
}

void os_pci_restore_state(struct pci_dev *dev)
{
	return pci_restore_state(dev);
}

int os_pci_save_state(struct pci_dev *dev)
{
	return pci_save_state(dev);
}

int os_pci_set_power_state(struct pci_dev *dev, pci_power_t state)
{
	return pci_set_power_state(dev, state);
}

struct pci_saved_state *os_pci_store_saved_state(struct pci_dev *dev)
{
	return pci_store_saved_state(dev);
}

int os_pci_msix_vec_count(struct pci_dev *dev)
{
	return pci_msix_vec_count(dev);
}

int os_pci_msi_vec_count(struct pci_dev *dev)
{
	return pci_msi_vec_count(dev);
}

bool os_pci_support_msix(struct pci_dev *dev)
{
	return !!dev->msix_cap;
}

bool os_pci_support_msi(struct pci_dev *dev)
{
	return !!dev->msi_cap;
}

int os_pci_alloc_irq_vectors(struct pci_dev *dev, unsigned int min_vecs,
			     unsigned int max_vecs, unsigned int flags)
{
	return pci_alloc_irq_vectors(dev, min_vecs, max_vecs, flags);
}

u64 os_pci_get_pcie_replay_rollover(struct pci_dev *pdev)
{
#ifdef OS_STRUCT_PCI_DEV_HAS_AER_STATS
	u64 *replay_num = 0;
	u64 *ptr;

	if (!pdev->aer_stats)
		return 0;

	ptr = (u64 *)pdev->aer_stats;
	replay_num = ptr + ilog2(PCI_ERR_COR_REP_ROLL);

	return *replay_num;
#endif
	return 0;
}

u64 os_pci_get_pcie_replay_timeout(struct pci_dev *pdev)
{
#ifdef OS_STRUCT_PCI_DEV_HAS_AER_STATS
	u64 *replay_timeout = 0;
	u64 *ptr;

	if (!pdev->aer_stats)
		return 0;

	ptr = (u64 *)pdev->aer_stats;
	replay_timeout = ptr + ilog2(PCI_ERR_COR_REP_TIMER);

	return *replay_timeout;
#endif
	return 0;
}

int os_pci_enable_pcie_error_reporting(struct pci_dev *pdev)
{
#ifdef OS_FUNC_PCI_ENABLE_PCIE_ERROR_REPORTING_EXIST
	return pci_enable_pcie_error_reporting(pdev);
#endif
	return 0;
}

int os_pci_disable_pcie_error_reporting(struct pci_dev *pdev)
{
#ifdef OS_FUNC_PCI_DISABLE_PCIE_ERROR_REPORTING_EXIST
	return pci_disable_pcie_error_reporting(pdev);
#endif
	return 0;
}

int os_pci_dev_wait(struct pci_dev *dev, char *reset_type, int timeout)
{
	int delay = 1;
	u32 id;

	pci_read_config_dword(dev, PCI_COMMAND, &id);
	while (id == ~0) {
		if (delay > timeout) {
			pci_warn(dev, "not ready %dms after %s; giving up\n",
				 delay - 1, reset_type);
			return -ENOTTY;
		}

		if (delay > 1000)
			pci_info(dev, "not ready %dms after %s; waiting\n",
				 delay - 1, reset_type);

		msleep(delay);
		delay *= 2;
		pci_read_config_dword(dev, PCI_COMMAND, &id);
	}

	if (delay > 1000)
		pci_info(dev, "ready %dms after %s\n", delay - 1,
			 reset_type);

	return 0;
}

int os_pci_wait_for_pending_transaction(struct pci_dev *dev)
{
	return pci_wait_for_pending_transaction(dev);
}

void os_get_cached_msi_msg(unsigned int irq, struct msi_msg *msg)
{
	get_cached_msi_msg(irq, msg);
}

struct msi_desc *os_irq_get_msi_desc(unsigned int irq)
{
	return irq_get_msi_desc(irq);
}

struct msi_msg *os_msi_msg_alloc(void)
{
	return kzalloc(sizeof(struct msi_msg), GFP_KERNEL);
}

int os_get_msi_message(struct msi_msg *msiptr, u32 *addr_lo, u32 *addr_hi, u32 *data)
{
	if (!msiptr)
		return -EPERM;

	*addr_lo = msiptr->address_lo;
	*addr_hi = msiptr->address_hi;
	*data = msiptr->data;

	return 0;
}

int os_get_msi_message_from_desc(struct msi_desc *desc, u32 *addr_lo, u32 *addr_hi, u32 *data)
{
	if (!desc)
		return -EPERM;

	*addr_lo = desc->msg.address_lo;
	*addr_hi = desc->msg.address_hi;
	*data = desc->msg.data;

	return 0;
}

int os_request_irq(unsigned int irq, void *handler, unsigned long flags,
		   const char *name, void *dev)
{
	return request_irq(irq, (irq_handler_t)handler, flags, name, dev);
}

const void *os_free_irq(unsigned int irq, void *dev_id)
{
	return free_irq(irq, dev_id);
}

void os_pci_free_irq_vectors(struct pci_dev *pdev)
{
	return pci_free_irq_vectors(pdev);
}

void os_udelay(unsigned long secs)
{
	udelay(secs);
}

void os_mdelay(unsigned long secs)
{
	mdelay(secs);
}

void os_usleep_range(unsigned long min, unsigned long max)
{
	usleep_range(min, max);
}

void os_msleep(unsigned int msecs)
{
	msleep(msecs);
}

struct dma_buf *os_dma_buf_get(int fd)
{
	return dma_buf_get(fd);
}

void os_dma_buf_put(struct dma_buf *dmabuf)
{
	dma_buf_put(dmabuf);
}

/* get dma_buf member */
IMPLEMENT_GET_OS_MEMBER_FUNC(dma_buf, size);
IMPLEMENT_GET_OS_MEMBER_FUNC(dma_buf, ops);
IMPLEMENT_GET_OS_MEMBER_FUNC(dma_buf, priv);

int os_dma_buf_fd(struct dma_buf *dma_buf, int flags)
{
	return dma_buf_fd(dma_buf, flags);
}

struct dma_resv *os_get_dma_buf_resv(struct dma_buf *dmabuf)
{
	return (struct dma_resv *)dmabuf->resv;
}

void *os_get_dmabuf_from_attachment(struct dma_buf_attachment *attach)
{
	return attach->dmabuf;
}

struct device *os_get_dma_buf_attachment_device(struct dma_buf_attachment *attach)
{
	return attach->dev;
}

struct device *os_get_device_parent(struct device *dev)
{
	return dev->parent;
}

struct device_node *os_get_device_of_node(struct device *dev)
{
	return dev->of_node;
}

struct device_node *os_get_device_node_parent_parent(struct device_node *node)
{
	if (!node || !node->parent)
		return NULL;

	return node->parent->parent;
}

const struct of_device_id *os_of_match_node(const struct of_device_id *matches, const struct device_node *node)
{
	return of_match_node(matches, node);
}

bool os_of_device_is_available(struct device_node *node)
{
	return of_device_is_available(node);
}

struct platform_device *os_of_find_device_by_node(struct device_node *np)
{
	return of_find_device_by_node(np);
}

const char *os_get_device_node_name(struct device_node *node)
{
	return node ? node->name : NULL;
}

int os_dma_set_mask_and_coherent(struct device *dev, u64 mask)
{
	return dma_set_mask_and_coherent(dev, mask);
}

int os_get_zone_type(struct page *page)
{
	return page_zonenum(page);
}

bool os_is_config_dma32_zone(void)
{
#if defined(CONFIG_ZONE_DMA32)
	return true;
#endif
	return false;
}

void *os_dma_alloc_coherent(struct device *dev, size_t size, dma_addr_t *dma_handle, gfp_t gfp)
{
	return dma_alloc_coherent(dev, size, dma_handle, gfp);
}

void os_dma_free_coherent(struct device *dev, size_t size, void *cpu_addr, dma_addr_t dma_handle)
{
	dma_free_coherent(dev, size, cpu_addr, dma_handle);
}

int os_dma_mmap_attrs(struct device *dev, struct vm_area_struct *vma, void *cpu_addr,
		      dma_addr_t dma_addr, size_t size, unsigned long attrs)
{
	return dma_mmap_attrs(dev, vma, cpu_addr, dma_addr, size, attrs);
}

void *os_memdup_user(const void __user *ptr, size_t n)
{
	return memdup_user(ptr, n);
}

unsigned long os_copy_from_user(void *to, const void *from, unsigned long n)
{
	return copy_from_user(to, from, n);
}

unsigned long os_copy_to_user(void __user *to, const void *from, unsigned long n)
{
	return copy_to_user(to, from, n);
}

unsigned long os_get_user(u64 *val, u64 __user *user_ptr)
{
	if (!val || !user_ptr)
		 return -EINVAL;

	return get_user(*val, user_ptr);
}

unsigned long os_get_user_u32(u32 *val, u32 __user *user_ptr)
{
	if (!val || !user_ptr)
		 return -EINVAL;

	return get_user(*val, user_ptr);
}

int os_request_firmware(const struct firmware **fw, const char *name, struct device *device)
{
	return request_firmware(fw, name, device);
}

void os_release_firmware(const struct firmware *fw)
{
	release_firmware(fw);
}

struct task_struct *os_kthread_create(int (*threadfn)(void *data),
				      void *data, const char *namefmt, ...)
{
	struct task_struct *task;
	va_list args;
	char name[TASK_COMM_LEN];

	va_start(args, namefmt);
	vsnprintf(name, TASK_COMM_LEN, namefmt, args);
	va_end(args);

	task = kthread_create_on_node(threadfn, data, NUMA_NO_NODE, "%s", name);
	if (IS_ERR(task)) {
		os_printk("Failed to create kernel thread\n");
		return NULL;
	}

	return task;
}

struct task_struct *os_kthread_run(int (*threadfn)(void *data),
				   void *data, const char *namefmt, ...)
{
	struct task_struct *task;
	va_list args;
	char name[TASK_COMM_LEN];

	va_start(args, namefmt);
	vsnprintf(name, TASK_COMM_LEN, namefmt, args);
	va_end(args);

	task = kthread_run(threadfn, data, "%s", name);
	if (IS_ERR(task)) {
		os_printk("Failed to create kernel thread\n");
		return NULL;
	}

	return task;
}

int os_kthread_stop(struct task_struct *k)
{
	return kthread_stop(k);
}

bool os_kthread_should_park(void)
{
	/*
	 * Only call kthread_should_park() if current is a kthread.
	 * kthread_should_park() expects current->kthread_data to be valid,
	 * which is only set for kthreads, not regular processes.
	 * Calling from non-kthread context causes NULL pointer dereference.
	 */
	if (!(current->flags & PF_KTHREAD))
		return false;
	return kthread_should_park();
}

void os_kthread_parkme(void)
{
	kthread_parkme();
}

int os_kthread_park(struct task_struct *k)
{
	return kthread_park(k);
}

void os_kthread_unpark(struct task_struct *k)
{
	kthread_unpark(k);
}

int os_wake_up_process(struct task_struct *p)
{
	return wake_up_process(p);
}

void os_cond_resched(void)
{
	cond_resched();
}

u32 os_crc32(u32 crc, const void *p, size_t len)
{
	return crc32(crc, p, len);
}

const u8 *os_get_firmware_data(const struct firmware *fw)
{
	return fw->data;
}

size_t os_get_firmware_size(const struct firmware *fw)
{
	return fw->size;
}

bool os_kthread_should_stop(void)
{
	return kthread_should_stop();
}

int os_mod_timer(struct timer_list *timer, unsigned long expires)
{
	return mod_timer(timer, expires);
}

int os_del_timer_sync(struct timer_list *timer)
{
	return del_timer_sync(timer);
}

void os_timer_setup(struct timer_list *timer, void (*function)(struct timer_list *),
		    unsigned int flags)
{
	timer_setup(timer, function, flags);
}

int os_create_timer(struct timer_list **timer)
{
	*timer = kzalloc(sizeof(*(*timer)), GFP_KERNEL);
	if (!(*timer))
		return -ENOMEM;

	return 0;
}

void os_destroy_timer(struct timer_list *timer)
{
	kfree(timer);
}

void os_hrtimer_init(struct hrtimer *timer, void *function)
{
#ifdef OS_HRTIMER_SETUP_CALLABLE
	hrtimer_setup(timer, (enum hrtimer_restart(*)(struct hrtimer *))function,
		      CLOCK_MONOTONIC, HRTIMER_MODE_REL);
#else
	hrtimer_init(timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	timer->function = (enum hrtimer_restart(*)(struct hrtimer *))function;
#endif
}

void os_hrtimer_start(struct hrtimer *timer, u64 periods)
{
	hrtimer_start(timer, periods, HRTIMER_MODE_REL);
}

int os_hrtimer_cancel(struct hrtimer *timer)
{
	return hrtimer_cancel(timer);
}

u64 os_hrtimer_forward_now(struct hrtimer *timer, u64 interval)
{
	return hrtimer_forward_now(timer, interval);
}

void os_hrtimer_add_expires_ns(struct hrtimer *timer, u64 ns)
{
	hrtimer_add_expires_ns(timer, ns);
}

u64 os_kclock_ns64(void)
{
	ktime_t sTime = ktime_get();

#if defined(OS_UNION_KTIME_EXIST)
	return sTime.tv64;
#else
	return sTime;
#endif
}

u64 os_ktime_get_ns(void)
{
	return ktime_get_ns();
}

u64 os_ktime_get_sec(void)
{
	struct timespec64 ts;

	ktime_get_real_ts64(&ts);
	return ts.tv_sec;
}

u64 os_ktime_get_real_nsec(void)
{
	struct timespec64 ts;

	ktime_get_real_ts64(&ts);

	return TIMESPEC64_TO_NSEC(ts);
}

s64 os_ktime_get_real_seconds(void)
{
	return ktime_get_real_seconds();
}

void os_ktime_get_real_tm(struct mt_tm *mt_time, int offset)
{
	struct tm time;

	time64_to_tm(ktime_get_real_seconds(), offset, &time);
	mt_time->cur_tm_year = time.tm_year + 1900;
	mt_time->cur_tm_mon  = time.tm_mon + 1;
	mt_time->cur_tm_mday = time.tm_mday;
	mt_time->cur_tm_hour = time.tm_hour;
	mt_time->cur_tm_min  = time.tm_min;
	mt_time->cur_tm_sec  = time.tm_sec;
}

void os_time64_to_tm(u64 totalsecs, struct mt_tm *mt_time, int offset)
{
	struct tm time;

	time64_to_tm(totalsecs, offset, &time);

	mt_time->cur_tm_year = time.tm_year + 1900;
	mt_time->cur_tm_mon = time.tm_mon + 1;
	mt_time->cur_tm_mday = time.tm_mday;
	mt_time->cur_tm_hour = time.tm_hour;
	mt_time->cur_tm_min = time.tm_min;
	mt_time->cur_tm_sec = time.tm_sec;
}

u64 os_sched_clock_get_ns(void)
{
	return sched_clock();
}

u64 os_ktime_get_real_ns(void)
{
	return ktime_get_real_ns();
}

u64 os_ktime_get(void)
{
	return ktime_get();
}

bool os_ktime_before(const ktime_t cmp1, const ktime_t cmp2)
{
	return ktime_before(cmp1, cmp2);
}

bool os_time_after(unsigned long a, unsigned long b)
{
	return time_after(a, b);
}

struct resource *os_request_region(resource_size_t start,
				   resource_size_t n,
				   const char *name)
{
	return request_region(start, n, name);
}

void os_release_region(resource_size_t start, resource_size_t n)
{
	release_region(start, n);
}

struct resource *os_request_mem_region(resource_size_t start,
				       resource_size_t n,
				       const char *name)
{
	return request_mem_region(start, n, name);
}

void os_release_mem_region(resource_size_t start, resource_size_t n)
{
	release_mem_region(start, n);
}

struct resource *os_alloc_resource(void)
{
	return kzalloc(sizeof(struct resource), GFP_KERNEL);
}

struct resource *os_create_resource(struct mtgpu_resource *mtgpu_res, u32 num_res)
{
	int i;
	struct resource *resource = kcalloc(num_res, sizeof(*resource), GFP_KERNEL);

	if (!resource)
		return NULL;

	for (i = 0; i < num_res; i++) {
		resource[i].start = mtgpu_res[i].start;
		resource[i].end = mtgpu_res[i].end;
		resource[i].name = mtgpu_res[i].name;
		resource[i].flags = mtgpu_res[i].is_mem ? IORESOURCE_MEM : IORESOURCE_IRQ;
	}

	return resource;
}

void os_destroy_resource(struct resource *resource)
{
	kfree(resource);
}

int os_release_resource(struct resource *new)
{
	return release_resource(new);
}

resource_size_t os_resource_size(const struct resource *res)
{
	return resource_size(res);
}

resource_size_t os_resource_start(const struct resource *res)
{
	return res->start;
}

resource_size_t os_resource_end(const struct resource *res)
{
	return res->end;
}

int os_walk_iomem_res_desc(unsigned long desc, unsigned long flags, u64 start,
			   u64 end, void *arg, int (*func)(struct resource *, void *))
{
	return walk_iomem_res_desc(desc, flags, start, end, arg, func);
}

struct inode *os_file_inode(const struct file *f)
{
	return file_inode(f);
}

void os_sort(void *base, size_t num, size_t size,
	     int (*cmp)(const void *, const void *),
	     void (*swap)(void *, void *, int))
{
	return sort(base, num, size, cmp, swap);
}

void os_list_sort(void *priv, struct list_head *head,
		  int (*cmp)(void *priv, const struct list_head *a,
			     const struct list_head *b))
{
#if defined(OS_LIST_SORT_CMP_USE_CONST_MODIFIER)
	list_sort(priv, head, cmp);
#else
typedef int (*cmp_func)(void *priv, struct list_head *a, struct list_head *b);
	list_sort(priv, head, (cmp_func)cmp);
#endif
}

const struct cpumask *os_cpumask_of_node(int node)
{
	return cpumask_of_node(node);
}

const struct cpumask *os_get_cpu_online_mask(void)
{
	return cpu_online_mask;
}

void os_cpumask_copy(struct cpumask *dstp, const struct cpumask *srcp)
{
	cpumask_copy(dstp, srcp);
}

struct irq_data *os_irq_get_irq_data(int irq)
{
	return irq_get_irq_data(irq);
}

struct irq_chip *os_irq_data_get_irq_chip(struct irq_data *data)
{
	return irq_data_get_irq_chip(data);
}

struct cpumask *os_irq_data_get_affinity_mask(struct irq_data *data)
{
	return (struct cpumask *)irq_data_get_affinity_mask(data);
}

void *os_irq_chip_support_set_affinity(struct irq_chip *chip)
{
	return chip->irq_set_affinity;
}

void os_irq_set_affinity(struct irq_chip *chip,
			      struct irq_data *data,
			      const struct cpumask *affinity,
			      bool force)
{
	chip->irq_set_affinity(data, affinity, force);
}

void os_irq_set_affinity_notifier(unsigned int irq, struct irq_affinity_notify *notify)
{
	irq_set_affinity_notifier(irq, notify);
}

int os_create_irq_affinity_notify(struct irq_affinity_notify **affinity_notify,
			    void (*notify)(struct irq_affinity_notify *, const cpumask_t *mask),
			    void (*release)(struct kref *ref))
{
	*affinity_notify = kzalloc(sizeof(struct mt_affinity_notify), GFP_KERNEL);
	if (!(*affinity_notify))
		return -ENOMEM;
	(*affinity_notify)->notify = notify;
	(*affinity_notify)->release = release;

	return 0;
}

void os_destroy_irq_affinity_notify(struct irq_affinity_notify *affinity_notify)
{
	kfree(affinity_notify);
}

void *os_get_irq_affinity_notify_drvdata(struct irq_affinity_notify *affinity_notify)
{
	struct mt_affinity_notify *mt_affinity_notify = (struct mt_affinity_notify *)affinity_notify;

	return mt_affinity_notify->data;
}

void os_set_irq_affinity_notify_drvdata(struct irq_affinity_notify *affinity_notify, void *data)
{
	struct mt_affinity_notify *mt_affinity_notify = (struct mt_affinity_notify *)affinity_notify;

	mt_affinity_notify->data = data;
}

struct radix_tree_root *os_create_radix_tree(void)
{
	struct radix_tree_root *root = kzalloc(sizeof(*root), GFP_KERNEL);

	if (root)
		INIT_RADIX_TREE(root, GFP_KERNEL);

	return root;
}

void os_destroy_radix_tree(struct radix_tree_root *root)
{
	kfree(root);
}

void *os_radix_tree_lookup(const struct radix_tree_root *root, unsigned long index)
{
	return radix_tree_lookup(root, index);
}

void *os_radix_tree_delete(struct radix_tree_root *root, unsigned long index)
{
	return radix_tree_delete(root, index);
}

int os_radix_tree_insert(struct radix_tree_root *root, unsigned long index, void *item)
{
	return radix_tree_insert(root, index, item);
}

struct radix_tree_iter *os_create_radix_tree_iter(void)
{
	return kzalloc(sizeof(struct radix_tree_iter), GFP_KERNEL);
}

void os_destroy_radix_tree_iter(struct radix_tree_iter *iter)
{
	kfree(iter);
}

unsigned long os_get_radix_tree_iter_index(struct radix_tree_iter *iter)
{
	return iter->index;
}

void **os_radix_tree_iter_init(struct radix_tree_iter *iter, unsigned long start)
{
	return radix_tree_iter_init(iter, start);
}

void **os_radix_tree_next_chunk(const struct radix_tree_root *root,
				struct radix_tree_iter *iter, unsigned int flags)
{
	return radix_tree_next_chunk(root, iter, flags);
}

void **os_radix_tree_next_slot(void **slot, struct radix_tree_iter *iter, unsigned int flags)
{
	return radix_tree_next_slot(slot, iter, flags);
}

u64 os_eventfd_signal(struct eventfd_ctx *ctx)
{
#if defined(OS_EVENTFD_SIGNAL_HAS_ONE_ARG)
	eventfd_signal(ctx);
	return 0;
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(6, 8, 0)
	eventfd_signal(ctx);
	return 0;
#else
	return eventfd_signal(ctx, 1);
#endif
}

void os_eventfd_ctx_put(struct eventfd_ctx *ctx)
{
	eventfd_ctx_put(ctx);
}

struct eventfd_ctx *os_eventfd_ctx_fdget(int fd)
{
	return eventfd_ctx_fdget(fd);
}

int os_pci_find_ext_capability(struct pci_dev *dev, int cap)
{
	return pci_find_ext_capability(dev, cap);
}

int os_pci_read_config_byte(const struct pci_dev *dev, int where, u8 *val)
{
	return pci_read_config_byte(dev, where, val);
}

int os_pci_read_config_word(const struct pci_dev *dev, int where, u16 *val)
{
	return pci_read_config_word(dev, where, val);
}

int os_pci_write_config_word(const struct pci_dev *dev, int where, u16 val)
{
	return pci_write_config_word(dev, where, val);
}

int os_pci_read_config_dword(const struct pci_dev *dev, int where, u32 *val)
{
	return pci_read_config_dword(dev, where, val);
}

int os_pci_write_config_dword(const struct pci_dev *dev, int where, u32 val)
{
	return pci_write_config_dword(dev, where, val);
}

void os_pci_release_resource(struct pci_dev *dev, int resno)
{
	pci_release_resource(dev,  resno);
}

int os_pci_resize_resource(struct pci_dev *dev, int resno, int size)
{
#ifdef OS_PCI_RESIZE_RESOURCE_HAS_FOUR_ARGS
	return pci_resize_resource(dev, resno, size, 0);
#else
	return pci_resize_resource(dev, resno, size);
#endif
}

void os_pci_assign_unassigned_bus_resources(struct pci_bus *bus)
{
	pci_assign_unassigned_bus_resources(bus);
}

struct resource *os_pci_bus_resource_n(const struct pci_bus *bus, int n)
{
	return pci_bus_resource_n(bus, n);
}

/* Check if the root BUS has 64bit memory resources */
void os_check_root_pcibus(struct pci_bus *root, struct resource **res)
{
	int i;

	while (root->parent)
		root = root->parent;

	for (i = 0; (*res = pci_bus_resource_n(root, i)) || i < PCI_BRIDGE_RESOURCE_NUM; i++) {
		if (*res && (*res)->flags & (IORESOURCE_MEM | IORESOURCE_MEM_64) &&
		    (*res)->start > 0x100000000ull)
			break;
	}
}

void os_pci_release_related_resources(struct pci_dev *pdev, int DDR_BAR_NUM)
{
	struct resource *tmp;
	struct resource *res = pdev->resource + DDR_BAR_NUM;

	if (res->parent) {
		tmp = res->parent->child;
		while (tmp) {
			if (!(tmp->flags & IORESOURCE_UNSET)) {
				release_resource(tmp);
				tmp->end = resource_size(tmp) - 1;
				tmp->start = 0;
				tmp->flags |= IORESOURCE_UNSET;
			}
			tmp = tmp->sibling;
		}
	}
}

struct pci_dev *os_find_parent_pci_dev(struct device *dev)
{
	struct device *parent;

	dev = get_device(dev);
	while (dev) {
		if (dev_is_pci(dev))
			return to_pci_dev(dev);

		parent = get_device(dev->parent);
		put_device(dev);
		dev = parent;
	}

	return NULL;
}

struct pci_dev *os_pci_dev_get(struct pci_dev *dev)
{
	return pci_dev_get(dev);
}

void os_pci_dev_put(struct pci_dev *dev)
{
	pci_dev_put(dev);
}

struct pci_dev *os_pci_get_device(unsigned int vendor, unsigned int device,
				  struct pci_dev *from)
{
	return pci_get_device(vendor, device, from);
}

int os_pci_sriov_get_totalvfs(struct pci_dev *dev)
{
	return pci_sriov_get_totalvfs(dev);
}

int os_pci_enable_sriov(struct pci_dev *dev, int nr_virtfn)
{
	return pci_enable_sriov(dev, nr_virtfn);
}

void os_pci_disable_sriov(struct pci_dev *dev)
{
	pci_disable_sriov(dev);
}

unsigned long os_ffs(unsigned long word)
{
	return __ffs(word);
}

unsigned long os_get_iodomain_aperture_end(struct iommu_domain *domain)
{
	return domain->geometry.aperture_end;
}

unsigned long os_get_iodomain_pgsize_bitmap(struct iommu_domain *domain)
{
	return domain->pgsize_bitmap;
}

void *os_create_iova_domain(void)
{
	return kzalloc(sizeof(struct iova_domain), GFP_KERNEL);
}

int os_of_dma_configure(struct device *dev,
			struct device_node *np,
			bool force_dma)
{
	return of_dma_configure(dev, np, force_dma);
}

struct device_node *os_of_find_node_by_path(const char *path)
{
	return of_find_node_by_path(path);
}

struct device_node *os_of_get_next_child(const struct device_node *node, struct device_node *prev)
{
	return of_get_next_child(node, prev);
}

const char *os_of_node_get_name(const struct device_node *device)
{
	return device->name;
}

int os_of_device_is_compatible(const struct device_node *device, const char *name)
{
	return of_device_is_compatible(device, name);
}

int os_of_address_to_resource(struct device_node *dev, int index, struct resource *r)
{
	return of_address_to_resource(dev, index, r);
}

bool os_is_acpi_disabled(void)
{
#if defined(OS_FUNC_ACPI_BUS_GET_DEVICE_EXIST) || defined(OS_FUNC_ACPI_FETCH_ACPI_DEV_EXIST)
	return acpi_disabled;
#else
	return true;
#endif
}

struct acpi_device *os_acpi_companion(struct device *dev)
{
	return ACPI_COMPANION(dev);
}

#if defined(OS_FUNC_ACPI_BUS_GET_DEVICE_EXIST) || defined(OS_FUNC_ACPI_FETCH_ACPI_DEV_EXIST)
#if defined(OS_BUS_FIND_DEVICE_MATCH_USE_CONST_MODIFIER)
static int is_device_match_acpi_dev(struct device *dev, const void *adev)
#else
static int is_device_match_acpi_dev(struct device *dev, void *adev)
#endif
{
	return ACPI_COMPANION(dev) == adev;
}
#endif

struct device *os_acpi_handle_to_device(void *handle, const struct acpi_device_id *acpi_ids)
{
#if defined(OS_FUNC_ACPI_BUS_GET_DEVICE_EXIST) || defined(OS_FUNC_ACPI_FETCH_ACPI_DEV_EXIST)
	struct device *dev;
	struct acpi_device *adev;
	const struct acpi_device_id *id;

#if defined(OS_FUNC_ACPI_BUS_GET_DEVICE_EXIST)
	if (acpi_bus_get_device(handle, &adev))
		return NULL;
#elif defined(OS_FUNC_ACPI_FETCH_ACPI_DEV_EXIST)
	adev = acpi_fetch_acpi_dev(handle);
#endif
	if (!adev || !adev->status.present)
		return NULL;

	dev = bus_find_device(&platform_bus_type, NULL, adev, is_device_match_acpi_dev);
	if (!dev)
		return NULL;

#if defined(OS_FUNC_ACPI_MATCH_ACPI_DEVICE_EXIST)
	id = acpi_match_acpi_device(acpi_ids, adev);
#else
	id = acpi_match_device(acpi_ids, dev);
#endif
	if (!id)
		return NULL;

	return dev;
#else
	return NULL;
#endif
}

void os_acpi_walk_name_space(acpi_walk_callback callback, void *data)
{
#if defined(OS_FUNC_ACPI_BUS_GET_DEVICE_EXIST) || defined(OS_FUNC_ACPI_FETCH_ACPI_DEV_EXIST)
	acpi_walk_namespace(ACPI_TYPE_ANY, ACPI_ROOT_OBJECT, ACPI_UINT32_MAX,
			    callback, NULL, data, NULL);
#endif
}

struct acpi_buffer *os_create_acpi_buffer(void)
{
	return kzalloc(sizeof(struct acpi_buffer), GFP_KERNEL);
}

void os_set_acpi_buffer_length(struct acpi_buffer *buffer, u64 size)
{
	buffer->length = size;
}

void *os_get_acpi_buffer_pointer(struct acpi_buffer *buffer)
{
	return buffer->pointer;
}

u32 os_get_acpi_object_type(union acpi_object *object)
{
	return object->type;
}

u64 os_get_acpi_object_integer_value(union acpi_object *object)
{
	return object->integer.value;
}

void *os_get_acpi_device_handle(struct acpi_device *adev)
{
#if defined(OS_FUNC_ACPI_BUS_GET_DEVICE_EXIST) || defined(OS_FUNC_ACPI_FETCH_ACPI_DEV_EXIST)
	return adev->handle;
#else
	return NULL;
#endif
}

u32 os_acpi_evaluate_object(void *object, char *path_name,
			    struct acpi_object_list *parameter_objects,
			    struct acpi_buffer *return_object_buffer)
{
	return acpi_evaluate_object(object, path_name,
				    parameter_objects,
				    return_object_buffer);
}

int os_acpi_device_set_power(struct acpi_device *device, int state)
{
#if defined(OS_FUNC_ACPI_DEVICE_SET_POWER_EXIST)
	return acpi_device_set_power(device, state);
#else
	return -ENODEV;
#endif
}

int os_get_acpi_device_power_state(struct acpi_device *device)
{
#if defined(OS_FUNC_ACPI_BUS_GET_DEVICE_EXIST) || defined(OS_FUNC_ACPI_FETCH_ACPI_DEV_EXIST)
	return device->power.state;
#else
	return -ENODEV;
#endif
}

struct acpi_device_id *os_create_acpi_device_id(const struct mt_acpi_device_id *mt_id, u32 size)
{
	int i;
	struct acpi_device_id *acpi_id;

	if (!size)
		return NULL;

	acpi_id = kzalloc(size * sizeof(*acpi_id), GFP_KERNEL);
	if (!acpi_id)
		return NULL;

	for (i = 0; i < size - 1; i++) {
		memcpy(acpi_id[i].id, mt_id[i].id, strlen(mt_id[i].id));
		acpi_id[i].driver_data = mt_id[i].driver_data;
	}

	return acpi_id;
}

void os_destroy_acpi_device_id(struct acpi_device_id *acpi_id)
{
	kfree(acpi_id);
}

struct of_device_id *os_create_of_device_id(const struct mt_of_device_id *mt_id, u32 size)
{
	int i;
	struct of_device_id *of_id;

	if (!size)
		return NULL;

	of_id = kzalloc(size * sizeof(*of_id), GFP_KERNEL);
	if (!of_id)
		return NULL;

	for (i = 0; i < size - 1; i++)
		memcpy(of_id[i].compatible, mt_id[i].compatible, strlen(mt_id[i].compatible));

	return of_id;
}

void os_destroy_of_device_id(struct of_device_id *of_id)
{
	kfree(of_id);
}

unsigned long os_iova_size(struct iova *iova)
{
	return iova_size(iova);
}

unsigned long os_iova_shift(struct iova_domain *iovad)
{
	return iova_shift(iovad);
}

unsigned long os_iova_mask(struct iova_domain *iovad)
{
	return iova_mask(iovad);
}

size_t os_iova_offset(struct iova_domain *iovad, dma_addr_t iova)
{
	return iova_offset(iovad, iova);
}

size_t os_iova_align(struct iova_domain *iovad, size_t size)
{
	return iova_align(iovad, size);
}

dma_addr_t os_iova_dma_addr(struct iova_domain *iovad, struct iova *iova)
{
	return iova_dma_addr(iovad, iova);
}

unsigned long os_iova_pfn(struct iova_domain *iovad, dma_addr_t iova)
{
	return iova_pfn(iovad, iova);
}

int os_iova_cache_get(void)
{
	return iova_cache_get();
}

void os_iova_cache_put(void)
{
	iova_cache_put();
}

void os_free_iova(struct iova_domain *iovad, struct iova *iova)
{
	__free_iova(iovad, iova);
}

struct iova *os_alloc_iova(struct iova_domain *iovad, unsigned long size,
			   unsigned long limit_pfn, bool size_aligned)
{
	return alloc_iova(iovad, size, limit_pfn, size_aligned);
}

struct iova *os_find_iova(struct iova_domain *iovad, unsigned long pfn)
{
	return find_iova(iovad, pfn);
}

void os_put_iova_domain(struct iova_domain *iovad)
{
	put_iova_domain(iovad);
}

void os_init_iova_domain(struct iova_domain *iovad, unsigned long granule,
			 unsigned long start_pfn)
{
	init_iova_domain(iovad, granule, start_pfn);
}

size_t os_iommu_map_sg(struct iommu_domain *domain, unsigned long iova,
		       struct scatterlist *sg, unsigned int nents, int prot)
{
#ifdef OS_IOMMU_MAP_USE_GFP_ARG
	return iommu_map_sg(domain, iova, sg, nents, prot, GFP_KERNEL);
#else
	return iommu_map_sg(domain, iova, sg, nents, prot);
#endif
}

int os_iommu_map(struct iommu_domain *domain, unsigned long iova,
		 phys_addr_t paddr, size_t size, int prot)
{
#ifdef OS_IOMMU_MAP_USE_GFP_ARG
	return iommu_map(domain, iova, paddr, size, prot, GFP_KERNEL);
#else
	return iommu_map(domain, iova, paddr, size, prot);
#endif
}

size_t os_iommu_unmap(struct iommu_domain *domain, unsigned long iova, size_t size)
{
	return iommu_unmap(domain, iova, size);
}

u64 os_iommu_iova_to_phys(struct iommu_domain *domain, u64 iova)
{
	return iommu_iova_to_phys(domain, iova);
}

void os_iommu_group_put(struct iommu_group *group)
{
	iommu_group_put(group);
}

int os_iommu_group_id(struct iommu_group *group)
{
	return iommu_group_id(group);
}

struct iommu_group *os_iommu_group_get(struct device *dev)
{
	return iommu_group_get(dev);
}

int os_iommu_attach_group(struct iommu_domain *domain, struct iommu_group *group)
{
	return iommu_attach_group(domain, group);
}

void os_iommu_detach_group(struct iommu_domain *domain, struct iommu_group *group)
{
	iommu_detach_group(domain, group);
}

struct iommu_domain *os_iommu_domain_alloc(struct bus_type *bus)
{
	return mt_iommu_domain_alloc(bus);
}

void os_iommu_domain_free(struct iommu_domain *domain)
{
	iommu_domain_free(domain);
}

struct iommu_domain *os_iommu_get_domain_for_dev(struct device *dev)
{
	return iommu_get_domain_for_dev(dev);
}

void os_iommu_detach_device(struct iommu_domain *domain, struct device *dev)
{
	iommu_detach_device(domain, dev);
}

int os_iommu_attach_device(struct iommu_domain *domain, struct device *dev)
{
	return iommu_attach_device(domain, dev);
}

int os_iommu_get_msi_cookie(struct iommu_domain *domain, dma_addr_t base)
{
	return iommu_get_msi_cookie(domain, base);
}

unsigned int os_get_iommu_domain_type(struct iommu_domain *domain)
{
	return domain->type;
}

bool os_iommu_present(struct bus_type *bus)
{
	return mt_iommu_present(bus);
}

int os_iommu_group_for_each_dev(struct iommu_group *group, void *data,
				int (*fn)(struct device *, void *))
{
	return iommu_group_for_each_dev(group, data, fn);
}


bool os_virt_addr_valid(void *address)
{
	return virt_addr_valid(address);
}

phys_addr_t os_virt_to_phys(void *address)
{
	return virt_to_phys(address);
}

struct bus_type *os_get_dev_bus_type(struct device *dev)
{
	return
#if !defined(OS_BUS_TYPE_IS_NOT_CONST)
		(struct bus_type *)
#endif
		dev->bus;
}

void os_bitmap_set(unsigned long *map, unsigned int start, unsigned int nbits)
{
	bitmap_set(map, start, nbits);
}

void os_bitmap_clear(unsigned long *map, unsigned int start, unsigned int nbits)
{
	bitmap_clear(map, start, nbits);
}

int os_bitmap_empty(const unsigned long *src, unsigned int nbits)
{
	return bitmap_empty(src, nbits);
}

int os_bitmap_full(const unsigned long *src, unsigned int nbits)
{
	return bitmap_full(src, nbits);
}

int os_bitmap_weight(const unsigned long *src, unsigned int nbits)
{
	return bitmap_weight(src, nbits);
}

void os_bitmap_free(const unsigned long *bitmap)
{
	bitmap_free(bitmap);
}

unsigned long *os_bitmap_zalloc(unsigned int nbits, gfp_t flags)
{
	return bitmap_zalloc(nbits, flags);
}

unsigned long *os_bitmap_zalloc_v2(unsigned long nbits)
{
	return kvmalloc_array(BITS_TO_LONGS(nbits), sizeof(unsigned long), GFP_KERNEL | __GFP_ZERO);
}

void os_bitmap_free_v2(const unsigned long *bitmap)
{
	kvfree(bitmap);
}

unsigned long os_bitmap_find_next_zero_area(unsigned long *map,
					    unsigned long size,
					    unsigned long start,
					    unsigned int nr,
					    unsigned long align_mask)
{
	return bitmap_find_next_zero_area(map, size, start, nr, align_mask);
}

unsigned long os_find_next_zero_bit(const unsigned long *addr, unsigned long size,
				    unsigned long offset)
{
	return find_next_zero_bit(addr, size, offset);
}

unsigned long os_find_first_zero_bit(const unsigned long *addr, unsigned long size)
{
	return find_first_zero_bit(addr, size);
}

int os_test_bit(int nr, const volatile unsigned long *addr)
{
	return test_bit(nr, addr);
}

void os_set_bit(long nr, volatile unsigned long *addr)
{
	set_bit(nr, addr);
}

void *os_kmem_cache_alloc_record(struct kmem_cache *cachep,
				 gfp_t flags,
				 const char *func_name,
				 u32 line_num)
{
	void *cpu_vaddr = kmem_cache_alloc(cachep, flags);

	if (cpu_vaddr && flags != GFP_ATOMIC)
		mtgpu_stats_add_kmalloc((u64)cpu_vaddr, 0, ksize(cpu_vaddr),
					func_name, line_num);

	return cpu_vaddr;
}

void *os_kmem_cache_zalloc_record(struct kmem_cache *cachep,
				  gfp_t flags,
				  const char *func_name,
				  u32 line_num)
{
	void *cpu_vaddr = kmem_cache_zalloc(cachep, flags);

	if (cpu_vaddr && flags != GFP_ATOMIC)
		mtgpu_stats_add_kmalloc((u64)cpu_vaddr, 0, ksize(cpu_vaddr),
					func_name, line_num);

	return cpu_vaddr;
}

void os_kmem_cache_free_record(struct kmem_cache *cachep,
			       void *objp,
			       bool record)
{
	if (record)
		mtgpu_stats_remove_kmalloc((u64)objp);

	kmem_cache_free(cachep, objp);
}

struct kmem_cache *os_kmem_cache_create(const char *name, unsigned int size,
					unsigned int align, slab_flags_t flags,
					void (*ctor)(void *))
{
	return kmem_cache_create(name, size, align, flags, ctor);
}

void os_kmem_cache_destroy(struct kmem_cache *s)
{
	kmem_cache_destroy(s);
}

void os_call_rcu(struct rcu_head *head, rcu_callback_t func)
{
	call_rcu(head, func);
}

void os_rcu_barrier(void)
{
	rcu_barrier();
}

void *os_rcu_dereference_check(void __rcu *p, bool c)
{
	return rcu_dereference_check(p, c);
}

void *os_rcu_dereference(void __rcu *p)
{
	return rcu_dereference(p);
}

void os_rcu_assign_pointer(void __rcu **p, void *v)
{
	rcu_assign_pointer(*p, v);
}

void os_rcu_init_pointer(void __rcu *p, void *v)
{
	RCU_INIT_POINTER(p, v);
}

void os_rcu_read_lock(void)
{
	rcu_read_lock();
}

void os_rcu_read_unlock(void)
{
	rcu_read_unlock();
}

int os_atomic_xchg(atomic_t *v, int val)
{
	return atomic_xchg(v, val);
}

void os_atomic_set(atomic_t *v, int val)
{
	atomic_set(v, val);
}

void os_atomic_inc(atomic_t *v)
{
	atomic_inc(v);
}

int os_atomic_inc_return(atomic_t *v)
{
	return atomic_inc_return(v);
}

bool os_atomic_inc_not_zero(atomic_t *v)
{
	return atomic_inc_not_zero(v);
}

void os_atomic_add(int i, atomic_t *v)
{
	atomic_add(i, v);
}

int os_atomic_sub_return(int i, atomic_t *v)
{
	return atomic_sub_return(i, v);
}

bool os_atomic_dec_and_test(atomic_t *v)
{
	return atomic_dec_and_test(v);
}

void os_atomic_dec(atomic_t *v)
{
	return atomic_dec(v);
}

int os_atomic_dec_return(atomic_t *v)
{
	return atomic_dec_return(v);
}

int os_atomic_read(atomic_t *v)
{
	return atomic_read(v);
}

int os_atomic_fetch_add(int i, atomic_t *v)
{
	return atomic_fetch_add(i, v);
}

int os_atomic_fetch_sub(int i, atomic_t *v)
{
	return atomic_fetch_sub(i, v);
}

int os_atomic_sub(int i, atomic_t *v)
{
	return atomic_sub_return(i, v);
}

void os_atomic64_set(atomic64_t *v, s64 i)
{
	atomic64_set(v, i);
}

s64 os_atomic64_inc_return(atomic64_t *v)
{
	return atomic64_inc_return(v);
}

s64 os_atomic64_read(const atomic64_t *v)
{
	return atomic64_read(v);
}

s64 os_atomic64_xchg(atomic64_t *v, s64 val)
{
	return atomic64_xchg(v, val);
}

int os_atomic_cmpxchg(atomic_t *addr, int oldval, int newval)
{
	return atomic_cmpxchg(addr, oldval, newval);
}

int os_atomic64_cmpxchg(atomic64_t *addr, int oldval, int newval)
{
	return atomic64_cmpxchg(addr, oldval, newval);
}

void *os_cmpxchg(void **ptr, void *old, void *new)
{
	return cmpxchg(ptr, old, new);
}

struct file *os_filp_open(const char *filename, int flags, umode_t mode)
{
	return filp_open(filename, flags, mode);
}

int os_filp_close(struct file *filp)
{
	return filp_close(filp, 0);
}

ssize_t os_kernel_write(struct file *filp, const void *buf, size_t count, loff_t *pos)
{
	return kernel_write(filp, buf, count, pos);
}

ssize_t os_kernel_read(struct file *filp, void *buf, size_t count, loff_t *pos)
{
	return kernel_read(filp, buf, count, pos);
}

static bool os_pci_is_downstream_port(const struct pci_dev *pdev)
{
	int type = pci_pcie_type(pdev);

	return type == PCI_EXP_TYPE_ROOT_PORT ||
	       type == PCI_EXP_TYPE_DOWNSTREAM ||
	       type == PCI_EXP_TYPE_PCIE_BRIDGE;
}

static bool os_pci_is_endpoint(const struct pci_dev *pdev)
{
	int pci_type = pci_pcie_type(pdev);

	return pci_type == PCI_EXP_TYPE_ENDPOINT ||
	       pci_type == PCI_EXP_TYPE_LEG_END;
}

static bool pci_is_sibling_downstream_port(struct pci_dev *pdev1, struct pci_dev *pdev2)
{
	if (os_pci_is_downstream_port(pdev1) && os_pci_is_downstream_port(pdev2)) {
		/* pdev1 and pdev2 have some domain and bus number,
		 * means that they are siblings.
		 */
		if (os_pci_domain_nr(pdev1) == os_pci_domain_nr(pdev2) &&
		    os_get_pci_bus_number(pdev1) == os_get_pci_bus_number(pdev2))
			return true;
	}

	return false;
}

int os_pcie_capability_read_word(struct pci_dev *pdev, int where, u16 *val)
{
	return pcie_capability_read_word(pdev, where, val);
}

int os_pcie_capability_read_dword(struct pci_dev *dev, int where, u32 *val)
{
	return pcie_capability_read_dword(dev, where, val);
}

int os_pcie_capability_write_word(struct pci_dev *pdev, int where, u16 val)
{
	return pcie_capability_write_word(pdev, where, val);
}

int os_pcie_capability_set_word(struct pci_dev *dev, int where, u16 set)
{
	return pcie_capability_set_word(dev, where, set);
}

u32 os_pcie_bandwidth_available(struct pci_dev *dev, struct pci_dev **limiting_dev,
				void *speed, void *width)
{
	return pcie_bandwidth_available(dev, limiting_dev, speed, width);
}

static struct pci_dev *os_pci_find_root_port(struct pci_dev *pdev)
{
	struct pci_dev *pdev_temp = pdev;

	while (pdev_temp) {
		if (pci_is_pcie(pdev_temp) &&
		    pci_pcie_type(pdev_temp) == PCI_EXP_TYPE_ROOT_PORT)
			return pdev_temp;
		pdev_temp = pci_upstream_bridge(pdev_temp);
	}

	return NULL;
}

bool os_pci_has_same_root_port(struct pci_dev *pdev1, struct pci_dev *pdev2)
{
	struct pci_dev  *rp1, *rp2;

	rp1 = os_pci_find_root_port(pdev1);
	rp2 = os_pci_find_root_port(pdev2);

	return rp1 && rp2 && rp1 == rp2;
}

u16 os_pci_get_aer_cap(struct pci_dev *pdev)
{
#ifdef OS_STRUCT_PCI_DEV_HAS_AER_CAP
	return pdev->aer_cap;
#endif
	return 0;
}

int os_pci_pcie_type(const struct pci_dev *pdev)
{
	return pci_pcie_type(pdev);
}

bool os_pci_is_pcie(struct pci_dev *pdev)
{
	return pci_is_pcie(pdev);
}

int os_pci_status_get_and_clear_errors(struct pci_dev *pdev)
{
#ifdef OS_FUNC_PCI_STATUS_GET_AND_CLEAR_ERRORS_EXIST
	return pci_status_get_and_clear_errors(pdev);
#else
	u16 status;
	int ret;

	ret = pci_read_config_word(pdev, PCI_STATUS, &status);
	if (ret != 0)
		return -EIO;

	status &= PCI_STATUS_ERROR_BITS;
	if (status)
		pci_write_config_word(pdev, PCI_STATUS, status);

	return status;
#endif
}

bool os_pci_is_under_same_switch(struct pci_dev *pdev1,
				 struct pci_dev *pdev2)
{
	struct pci_dev *parent1, *parent2;

	if (pdev1 == pdev2)
		return false;

	if (!os_pci_is_endpoint(pdev1))
		return false;

	if (!os_pci_is_endpoint(pdev2))
		return false;

	parent1 = pci_upstream_bridge(pdev1);
	if (!parent1)
		/* parent is NULL, means that dev1 is root port */
		return false;

	parent2 = pci_upstream_bridge(pdev2);
	if (!parent2)
		/* parent is NULL, means that dev2 is root port */
		return false;

	return pci_is_sibling_downstream_port(parent1, parent2);
}

struct pci_dev *os_pci_find_upstream_switch(struct pci_dev *pdev)
{
	struct pci_dev *parent;

	for (parent = pdev->bus->self; parent; parent = parent->bus->self) {
		if (pci_is_pcie(parent)) {
			if (pci_pcie_type(parent) == PCI_EXP_TYPE_ROOT_PORT ||
			    pci_pcie_type(parent) == PCI_EXP_TYPE_DOWNSTREAM) {
				return parent;
			}
		}
	}

	return NULL;
}

struct platform_device_info *os_create_platform_device_info(struct device *dev,
							    const char *name,
							    int id,
							    const struct resource *res,
							    unsigned int num_res,
							    const void *data,
							    size_t size_data,
							    u64 dma_mask)
{
	struct platform_device_info *pdev_info =
			kzalloc(sizeof(*pdev_info), GFP_KERNEL);

	pdev_info->parent = dev;
	pdev_info->name = name;
	pdev_info->id = id;
	pdev_info->res = res;
	pdev_info->num_res = num_res;
	pdev_info->data = data;
	pdev_info->size_data = size_data;
	pdev_info->dma_mask = dma_mask;

	return pdev_info;
}

static pte_t *os_pte_offset_map_lock(struct mm_struct *mm, pmd_t *pmd,
				     unsigned long addr, spinlock_t **ptlp)
{
#ifdef OS_FUNC___PTE_MAP_EXIST
	spinlock_t *ptl;
	pte_t *pte;

	rcu_read_unlock();

	pte = __pte_map(pmd, addr);
	if (unlikely(!pte)) {
		rcu_read_unlock();
		return pte;
	}

	ptl = pte_lockptr(mm, pmd);
	spin_lock(ptl);

	*ptlp = ptl;

	return pte;
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(6, 9, 0)
	/* pte_offset_map_lock() is not exported to modules on 6.9+; rebuild
	 * it from the exported/static-inline pieces. Callers hold mmap_lock. */
	spinlock_t *ptl;
	pte_t *pte;

	pte = pte_offset_map(pmd, addr);
	if (unlikely(!pte))
		return NULL;

	ptl = pte_lockptr(mm, pmd);
	spin_lock(ptl);

	*ptlp = ptl;

	return pte;
#else
	return pte_offset_map_lock(mm, pmd, addr, ptlp);
#endif
}

int os_user_va_to_cpu_pa(struct mm_struct *mm, u64 user_va, phys_addr_t *pa_out)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 10, 0)
	/* follow_pfn() and the pte_offset_map*() family are not exported to
	 * modules on 6.10+; follow_pfnmap_start() is the supported API for
	 * resolving a user VA to its physical address. */
	struct follow_pfnmap_args args = { .vma = NULL, .address = user_va };
	struct vm_area_struct *vma;
	int ret;

	if (!mm || !pa_out)
		return -EFAULT;

	mmap_read_lock(mm);
	vma = find_vma(mm, user_va);
	if (!vma || vma->vm_start > user_va) {
		mmap_read_unlock(mm);
		return -EFAULT;
	}

	args.vma = vma;
	ret = follow_pfnmap_start(&args);
	mmap_read_unlock(mm);
	if (ret)
		return ret;

	*pa_out = (phys_addr_t)((args.pfn << PAGE_SHIFT) |
				(user_va & ~PAGE_MASK));
	follow_pfnmap_end(&args);
	return 0;
#else
	pgd_t *pgd;
	p4d_t *p4d;
	pud_t *pud;
	pmd_t *pmd;
	pte_t *pte;
	spinlock_t *ptl;
	phys_addr_t pa;

	if (!mm || !pa_out)
		return -EFAULT;

	pgd = pgd_offset(mm, user_va);
	if (pgd_none(*pgd) || pgd_bad(*pgd))
		return -EFAULT;

	p4d = p4d_offset(pgd, user_va);
	if (p4d_none(*p4d) || p4d_bad(*p4d))
		return -EFAULT;

	pud = pud_offset(p4d, user_va);
	if (pud_none(*pud) || pud_bad(*pud))
		return -EFAULT;

	pmd = pmd_offset(pud, user_va);
	if (pmd_none(*pmd) || pmd_bad(*pmd))
		return -EFAULT;

	pte = os_pte_offset_map_lock(mm, pmd, user_va, &ptl);
	if (!pte)
		return -EFAULT;

	if (!pte_present(*pte)) {
		pte_unmap_unlock(pte, ptl);
		return -EFAULT;
	}

	pa = (pte_pfn(*pte) << PAGE_SHIFT) | (user_va & ~PAGE_MASK);
	pte_unmap_unlock(pte, ptl);
	*pa_out = pa;

	return 0;
#endif
}

void os_destroy_platform_device_info(struct platform_device_info *pdev_info)
{
	kfree(pdev_info);
}

struct device *os_get_platform_device_base(struct platform_device *pdev)
{
	return &pdev->dev;
}

struct resource *os_platform_get_resource(struct platform_device *dev,
					  unsigned int type,
					  unsigned int num)
{
	return platform_get_resource(dev, type, num);
}

void os_platform_set_drvdata(struct platform_device *pdev, void *data)
{
	pdev->dev.driver_data = data;
}

void *os_platform_get_drvdata(struct platform_device *pdev)
{
	return platform_get_drvdata(pdev);
}

int os_platform_get_irq(struct platform_device *dev, unsigned int num)
{
	return platform_get_irq(dev, num);
}

int os_platform_irq_count(struct platform_device *pdev)
{
	return platform_irq_count(pdev);
}

struct platform_device *
os_platform_device_register_full(const struct platform_device_info *pdevinfo)
{
	return platform_device_register_full(pdevinfo);
}

void os_platform_device_unregister(struct platform_device *pdev)
{
	platform_device_unregister(pdev);
}

struct platform_device *os_to_platform_device(struct device *dev)
{
	return to_platform_device(dev);
}

const char *os_get_paltform_device_name(struct platform_device *pdev)
{
	return pdev->name;
}

void *os_get_platform_data(struct device *dev)
{
	return dev->platform_data;
}

u64 os_roundup_pow_of_two(u64 size)
{
	return roundup_pow_of_two(size);
}

u32 os_order_base_2(u64 size)
{
	return order_base_2(size);
}

int os_fls(unsigned int x)
{
	return fls(x);
}

int os_fls64(unsigned long x)
{
	return fls64(x);
}

u64 os_cpu_to_le64(u64 data)
{
	return cpu_to_le64(data);
}

u32 os_cpu_to_le32(u32 data)
{
	return cpu_to_le32(data);
}

u64 os_div64_u64(u64 dividend, u64 divisor)
{
	return div64_u64(dividend, divisor);
}

u64 os_div64_u64_round_up(u64 dividend, u64 divisor)
{
	return div64_u64(dividend + divisor - 1, divisor);
}

u32 os_int_sqrt(u32 num)
{
	return int_sqrt(num);
}

resource_size_t os_get_system_available_ram_size(void)
{
	resource_size_t available_ram_size;
	struct sysinfo mem_info;

	si_meminfo(&mem_info);
	available_ram_size = si_mem_available() * mem_info.mem_unit;

	return available_ram_size;
}

resource_size_t os_get_system_free_ram_size(void)
{
	struct sysinfo mem_info;

	si_meminfo(&mem_info);
	return mem_info.freeram * mem_info.mem_unit;
}

resource_size_t os_get_system_total_ram_size(void)
{
	struct sysinfo mem_info;

	si_meminfo(&mem_info);

	return mem_info.totalram * mem_info.mem_unit;
}

void os_pm_runtime_set_autosuspend_delay(struct device *dev, int delay)
{
	pm_runtime_set_autosuspend_delay(dev, delay);
}

void os_pm_runtime_use_autosuspend(struct device *dev)
{
	pm_runtime_use_autosuspend(dev);
}

void os_pm_runtime_dont_use_autosuspend(struct device *dev)
{
	return pm_runtime_dont_use_autosuspend(dev);
}

int os_pm_runtime_set_suspended(struct device *dev)
{
	return pm_runtime_set_suspended(dev);
}

void os_pm_runtime_enable(struct device *dev)
{
	pm_runtime_enable(dev);
}

bool os_pm_runtime_enabled(struct device *dev)
{
	return pm_runtime_enabled(dev);
}

int os_pm_runtime_set_active(struct device *dev)
{
	return pm_runtime_set_active(dev);
}

void os_pm_runtime_disable(struct device *dev)
{
	return pm_runtime_disable(dev);
}

bool os_pm_runtime_active(struct device *dev)
{
	return pm_runtime_active(dev);
}

int os_pm_runtime_get_sync(struct device *dev)
{
	return pm_runtime_get_sync(dev);
}

int os_pm_runtime_put_sync(struct device *dev)
{
	return pm_runtime_put_sync(dev);
}

void os_pm_runtime_mark_last_busy(struct device *dev)
{
	pm_runtime_mark_last_busy(dev);
}

int os_pm_runtime_put_autosuspend(struct device *dev)
{
	return pm_runtime_put_autosuspend(dev);
}

void os_pm_runtime_put_noidle(struct device *dev)
{
	pm_runtime_put_noidle(dev);
}

bool os_pm_runtime_status_suspended(struct device *dev)
{
	return pm_runtime_status_suspended(dev);
}

int os_register_pm_notifier(struct notifier_block *nb)
{
	return register_pm_notifier(nb);
}

int os_unregister_pm_notifier(struct notifier_block *nb)
{
	return unregister_pm_notifier(nb);
}

int os_ilog2(u64 n)
{
	return ilog2(n);
}

/*
 * When dkms build with open ftrace kernel, gcc will insert _mcount function into
 * all function include close source code(mtgpu.o).
 * This will case build err(undefined _mcount) in close ftrace kernel.
 * So we should define the _mcount function to cover this case.
 */
#ifndef CONFIG_FUNCTION_TRACER
void _mcount(unsigned long input)
{
	return;
}
#endif

struct mt_os_path {
	struct path path;
#ifdef MT_VFS_MKDIR_RETURNS_DENTRY
	struct dentry *mkdir_result;
	bool mkdir_called;
#endif
};

void *os_path_create(void)
{
	return os_kzalloc(sizeof(struct mt_os_path));
}

void os_path_destroy(void* path)
{
	os_kfree(path);
}

struct dentry *os_kern_path_create(int dfd, const char *name,
				   struct path *path,
				   unsigned int lookup_flags)
{
#ifdef MT_VFS_MKDIR_RETURNS_DENTRY
	struct mt_os_path *ctx = container_of(path, struct mt_os_path, path);

	ctx->mkdir_called = false;
	return start_creating_path(dfd, name, path, lookup_flags);
#else
	return kern_path_create(dfd, name, path, lookup_flags);
#endif
}

int os_kern_path(const char *name, unsigned int flags, struct path *path)
{
	return kern_path(name, flags, path);
}

int os_security_path_mkdir(struct path *path, struct dentry *dentry, umode_t mode)
{
	return security_path_mkdir(path, dentry, mode);
}

int os_vfs_mkdir(struct path *path, struct dentry *dentry, umode_t mode)
{
#ifdef MT_VFS_MKDIR_RETURNS_DENTRY
	struct mt_os_path *ctx = container_of(path, struct mt_os_path, path);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(7, 0, 0)
	ctx->mkdir_result = vfs_mkdir(mnt_idmap(path->mnt),
				    d_inode(path->dentry), dentry, mode, NULL);
#elif defined(OS_VFS_MKDIR_USE_MNT_IDMAP)
	ctx->mkdir_result = vfs_mkdir(mnt_idmap(path->mnt),
				    d_inode(path->dentry), dentry, mode);
#else
	ctx->mkdir_result = vfs_mkdir(d_inode(path->dentry), dentry, mode);
#endif
	ctx->mkdir_called = true;
	return PTR_ERR_OR_ZERO(ctx->mkdir_result);
#elif defined(OS_VFS_MKDIR_USE_USER_NAMESPACE)
	struct user_namespace *mnt_userns = current_user_ns();

	return vfs_mkdir(mnt_userns, path->dentry->d_inode, dentry, mode);
#elif defined OS_VFS_MKDIR_USE_MNT_IDMAP
	return vfs_mkdir(mnt_idmap(path->mnt), path->dentry->d_inode, dentry, mode);
#else
	return vfs_mkdir(path->dentry->d_inode, dentry, mode);
#endif
}

int os_vfs_unlink(struct inode *dir, struct dentry *dentry, struct inode **delegated_inode)
{
#if defined(OS_VFS_MKDIR_RETURNS_DENTRY)
	/* struct delegated_inode is opaque on 7.x; pass NULL. */
	int ret = vfs_unlink(&nop_mnt_idmap, dir, dentry, NULL);
	if (delegated_inode)
		*delegated_inode = NULL;
	return ret;
#elif defined(OS_VFS_MKDIR_USE_MNT_IDMAP)
	return vfs_unlink(&nop_mnt_idmap, dir, dentry, delegated_inode);
#elif defined(OS_VFS_MKDIR_USE_USER_NAMESPACE)
	return vfs_unlink(current_user_ns(), dir, dentry, delegated_inode);
#else
	return vfs_unlink(dir, dentry, delegated_inode);
#endif
}

struct dentry *os_dget_parent(struct dentry *dentry)
{
	return dget_parent(dentry);
}

struct inode *os_d_inode(const struct dentry *dentry)
{
	return d_inode(dentry);
}

void os_dput(struct dentry *dentry)
{
	dput(dentry);
}

void os_path_put(const struct path *path)
{
	path_put(path);
}

void os_inode_lock(struct inode *inode)
{
	inode_lock(inode);
}

void os_inode_unlock(struct inode *inode)
{
	inode_unlock(inode);
}

void os_done_path_create(struct path *path, struct dentry *dentry)
{
#ifdef MT_VFS_MKDIR_RETURNS_DENTRY
	struct mt_os_path *ctx = container_of(path, struct mt_os_path, path);

	end_creating_path(path, ctx->mkdir_called ? ctx->mkdir_result : dentry);
	ctx->mkdir_called = false;
#else
	done_path_create(path, dentry);
#endif
}

struct inode *os_get_inode_from_path(struct path *path)
{
	return path->dentry->d_inode;
}

struct dentry *os_get_dentry_from_path(struct path *path)
{
	return path->dentry;
}

bool os_retry_estale(int error, unsigned int lookup_flags)
{
	return retry_estale(error, lookup_flags);
}

int os_current_umask(void)
{
	return current_umask();
}


mempool_t *os_mempool_create(int min_nr, mempool_alloc_t *alloc_fn,
			     mempool_free_t *free_fn, void *pool_data)
{
	return mempool_create(min_nr, alloc_fn, free_fn, pool_data);
}

void os_mempool_destroy(mempool_t *pool)
{
	mempool_destroy(pool);
}

void *os_mempool_alloc(mempool_t *pool, gfp_t gfp_mask)
{
	return mempool_alloc(pool, gfp_mask);
}

void os_mempool_free(void *element, mempool_t *pool)
{
	mempool_free(element, pool);
}

int os_vsnprintf(char *buf, size_t size, const char *fmt, va_list args)
{
	return vsnprintf(buf, size, fmt, args);
}

int os_snprintf(char *buf, size_t size, const char *fmt, ...)
{
	va_list args;
	int i;

	va_start(args, fmt);
	i = vsnprintf(buf, size, fmt, args);
	va_end(args);

	return i;
}

int os_vprintk_level(const char *level, const char *fmt, va_list args)
{
	struct va_format vaf;
	va_list args_copy;
	int r;

	va_copy(args_copy, args);
	vaf.fmt = fmt;
	vaf.va = &args_copy;
	r = printk("%s%pV", level, &vaf);
	va_end(args_copy);

	return r;
}

int os_vprintk_ratelimited_level(struct ratelimit_state **rs, const char *func,
				 const char *level, const char *fmt, va_list args)
{
	if (!*rs)
		os_create_ratelimit_state(rs,
					  OS_VAL(DEFAULT_RATELIMIT_INTERVAL),
					  OS_VAL(DEFAULT_RATELIMIT_BURST));

	if (*rs && ___os_ratelimit(*rs, func))
		return os_vprintk_level(level, fmt, args);

	return 0;
}

int os_printk(const char *fmt, ...)
{
	va_list args;
	int r;

	va_start(args, fmt);
	r = vprintk(fmt, args);
	va_end(args);

	return r;
}

struct mt_ratelimit_state {
	struct ratelimit_state state;
	struct list_head node;
};

static DEFINE_SPINLOCK(g_ratelimit_state_lock);
static LIST_HEAD(g_ratelimit_state_list);

int ___os_ratelimit(struct ratelimit_state *rs, const char *func)
{
	return ___ratelimit(rs, func);
}

int os_create_ratelimit_state(struct ratelimit_state **rs, int interval, int burst)
{
	unsigned long flags;
	struct mt_ratelimit_state *mt_rs;

	mt_rs = kmalloc(sizeof(*mt_rs), preempt_count() ? GFP_ATOMIC : GFP_KERNEL);
	if (!mt_rs)
		return -ENOMEM;

	ratelimit_state_init(&mt_rs->state, interval, burst);

	spin_lock_irqsave(&g_ratelimit_state_lock, flags);
	list_add(&mt_rs->node, &g_ratelimit_state_list);
	spin_unlock_irqrestore(&g_ratelimit_state_lock, flags);

	*rs = &mt_rs->state;

	return 0;
}

void os_destroy_ratelimit_state_all(void)
{
	unsigned long flags;
	struct mt_ratelimit_state *rs, *tmp;

	spin_lock_irqsave(&g_ratelimit_state_lock, flags);
	list_for_each_entry_safe(rs, tmp, &g_ratelimit_state_list, node) {
		list_del(&rs->node);
		kfree(rs);
	}
	spin_unlock_irqrestore(&g_ratelimit_state_lock, flags);
}

void os_blocking_init_notifier_head(struct blocking_notifier_head *nh)
{
	BLOCKING_INIT_NOTIFIER_HEAD(nh);
}

void os_notifier_block_set_notifier_call(struct notifier_block *nb, notifier_fn_t cb)
{
	nb->notifier_call = cb;
}

int os_blocking_notifier_call_chain(struct blocking_notifier_head *nh,
				    unsigned long val, void *v)
{
	return blocking_notifier_call_chain(nh, val, v);
}

int os_blocking_notifier_chain_register(struct blocking_notifier_head *nh,
					struct notifier_block *n)
{
	return blocking_notifier_chain_register(nh, n);
}

int os_blocking_notifier_chain_unregister(struct blocking_notifier_head *nh,
					  struct notifier_block *n)
{
	return blocking_notifier_chain_unregister(nh,  n);
}

int os_blocking_notifier_head_create(struct blocking_notifier_head **nh)
{
	*nh = kzalloc(sizeof(struct blocking_notifier_head), GFP_KERNEL);
	if (!(*nh))
		return -ENOMEM;

	return 0;
}

void os_blocking_notifier_head_destroy(struct blocking_notifier_head *nh)
{
	kfree(nh);
}

struct device *os_get_dev_parent_parent(struct device *dev)
{
	return dev->parent->parent;
}

struct device *os_get_dev_parent(struct device *dev)
{
	return dev->parent;
}

int os_get_dev_power_child_count(struct device *dev)
{
	return atomic_read(&dev->power.child_count);
}

void *os_get_device_driver_data(struct device *dev)
{
	return dev->driver_data;
}

void *os_get_device_driver(struct device *dev)
{
	return dev->driver;
}

char *os_get_current_comm(void)
{
	return current->comm;
}

u64 os_get_current_pid(void)
{
	return current->pid;
}

u64 os_get_current_tgid(void)
{
	return current->tgid;
}

struct task_struct *os_get_current(void)
{
	return current;
}

struct mm_struct *os_get_current_mm(void)
{
	return current->mm;
}

u64 os_get_current_tgid_v(void)
{
	return task_tgid_vnr(os_get_current());
}

struct task_struct *os_get_current_group_leader(void)
{
	return current->group_leader;
}

u32 os_get_current_flags(void)
{
	return current->flags;
}

int os_get_current_exit_code(void)
{
	return current->exit_code;
}

int os_send_sigtrap(struct task_struct *p)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 8, 0)
	/* struct siginfo was retired; the kernel ABI is kernel_siginfo. */
	struct kernel_siginfo info;
#else
	struct siginfo info;
#endif

	memset(&info, 0, sizeof(info));

	info.si_signo = SIGTRAP;
        info.si_errno = 0;
        info.si_code  = SI_KERNEL;

	send_sig_info(SIGTRAP, &info, p);

	return 0;
}

int os_mmap_write_lock_killable(struct mm_struct *mm)
{
#if defined(OS_FUNC_MMAP_WRITE_LOCK_KILLABLE_EXIST)
	return mmap_write_lock_killable(mm);
#elif defined(OS_STRUCT_MM_STRUCT_HAS_MMAP_SEM)
	down_write(&mm->mmap_sem);
	return 0;
#elif defined(OS_STRUCT_MM_STRUCT_HAS_MMAP_LOCK)
	down_write(&mm->mmap_lock);
	return 0;
#else
#error "invalid ops"
#endif
}

void os_mmap_write_unlock(struct mm_struct *mm)
{
#if defined(OS_FUNC_MMAP_WRITE_UNLOCK_EXIST)
	mmap_write_unlock(mm);
#elif defined(OS_STRUCT_MM_STRUCT_HAS_MMAP_SEM)
	up_write(&mm->mmap_sem);
#elif defined(OS_STRUCT_MM_STRUCT_HAS_MMAP_LOCK)
	up_write(&mm->mmap_lock);
#else
#error "invalid ops"
#endif
}

void os_mm_mmap_read_lock(struct mm_struct *mm)
{
#if defined(OS_FUNC_MMAP_READ_LOCK_EXIST)
	mmap_read_lock(mm);
#elif defined(OS_STRUCT_MM_STRUCT_HAS_MMAP_SEM)
	down_read(&mm->mmap_sem);
#elif defined(OS_STRUCT_MM_STRUCT_HAS_MMAP_LOCK)
	down_read(&mm->mmap_lock);
#else
#error "invalid ops"
#endif
}

void os_mm_mmap_read_unlock(struct mm_struct *mm)
{
#if defined(OS_FUNC_MMAP_READ_UNLOCK_EXIST)
	mmap_read_unlock(mm);
#elif defined(OS_STRUCT_MM_STRUCT_HAS_MMAP_SEM)
	up_read(&mm->mmap_sem);
#elif defined(OS_STRUCT_MM_STRUCT_HAS_MMAP_LOCK)
	up_read(&mm->mmap_lock);
#else
#error "invalid ops"
#endif
}

void os_mmgrab(struct mm_struct *mm)
{
	mmgrab(mm);
}

void os_mmdrop(struct mm_struct *mm)
{
	mmdrop(mm);
}

bool os_capable_cap_ipc_lock(void)
{
	return capable(CAP_IPC_LOCK);
}

char *os_get_utsname_version(void)
{
	return init_uts_ns.name.version;
}

char *os_get_uts_sysname(void)
{
	if (current->flags & PF_EXITING)
		return NULL;

	return utsname()->sysname;
}

char *os_get_uts_release(void)
{
	if (current->flags & PF_EXITING)
		return NULL;

	return utsname()->release;
}

char *os_get_uts_version(void)
{
	if (current->flags & PF_EXITING)
		return NULL;

	return utsname()->version;
}

char *os_get_uts_machine(void)
{
	if (current->flags & PF_EXITING)
		return NULL;

	return utsname()->machine;
}

DEFINE_MUTEX(fence_cache_mutex);

static struct mt_dma_fence *os_get_mt_dma_fence(struct dma_fence *dma_fence)
{
	return container_of(dma_fence, struct mt_dma_fence, dma_fence);
}

void *os_get_dma_fence_drvdata(struct dma_fence *dma_fence)
{
	struct mt_dma_fence *mt_dma_fence = os_get_mt_dma_fence(dma_fence);

	return mt_dma_fence->data;
}

void os_set_dma_fence_drvdata(struct dma_fence *dma_fence, void *data)
{
	struct mt_dma_fence *mt_dma_fence = os_get_mt_dma_fence(dma_fence);

	mt_dma_fence->data = data;
}

IMPLEMENT_GET_OS_MEMBER_FUNC(dma_fence, context);
IMPLEMENT_GET_OS_MEMBER_FUNC(dma_fence, ops);
IMPLEMENT_GET_OS_MEMBER_FUNC(dma_fence, error);

struct rcu_head *os_get_dma_fence_rcu(struct dma_fence *dma_fence)
{
	return &dma_fence->rcu;
}

unsigned long *os_get_dma_fence_flags(struct dma_fence *dma_fence)
{
	return &dma_fence->flags;
}

u64 os_get_dma_fence_timestamp(struct dma_fence *dma_fence)
{
	return dma_fence->timestamp;
}

struct kref *os_get_dma_fence_refcount(struct dma_fence *dma_fence)
{
	return &dma_fence->refcount;
}

u64 os_get_dma_fence_seqno(struct dma_fence *dma_fence)
{
	return dma_fence->seqno;
}

void os_set_dma_fence_struct_seqno(struct dma_fence *dma_fence, u64 seqno)
{
	dma_fence->seqno = seqno;
}

void os_set_dma_fence_timestamp(struct dma_fence *dma_fence, u64 timestamp)
{
	dma_fence->timestamp = timestamp;
}

void *os_create_dma_fence(void)
{
	return kzalloc(sizeof(struct mt_dma_fence), GFP_KERNEL);
}

void os_destroy_dma_fence(struct dma_fence *dma_fence)
{
	kfree(dma_fence);
}

unsigned int os_get_dma_fence_size(void)
{
	return sizeof(struct mt_dma_fence);
}

unsigned int os_get_dma_fence_alignment(void)
{
	return __alignof__(struct mt_dma_fence);
}

void os_dma_fence_init(struct dma_fence *fence,
		       const struct dma_fence_ops *ops,
		       spinlock_t *lock, u64 context, u64 seqno)
{
	dma_fence_init(fence, ops, lock, context, seqno);
}

int os_dma_fence_ops_init(struct dma_fence_ops **ops,
			  const struct mt_dma_fence_ops *mt_ops)
{
	struct dma_fence_ops *dma_ops;

	dma_ops = kzalloc(sizeof(**ops), GFP_KERNEL);
	if (!dma_ops)
		return -ENOMEM;

	dma_ops->get_driver_name = mt_ops->get_driver_name;
	dma_ops->get_timeline_name = mt_ops->get_timeline_name;
	dma_ops->enable_signaling = mt_ops->enable_signaling;
	dma_ops->signaled = mt_ops->signaled;
	dma_ops->wait = mt_ops->wait;
	dma_ops->release = mt_ops->release;
#ifndef OS_STRUCT_DMA_FENCE_OPS_HAS_FENCE_VALUE_STR_REMOVED
#if defined(OS_STRUCT_DMA_FENCE_OPS_HAS_FENCE_VALUE_STR)
	dma_ops->fence_value_str = mt_ops->fence_value_str;
#endif
#if defined(OS_STRUCT_DMA_FENCE_OPS_HAS_TIMELINE_VALUE_STR)
	dma_ops->timeline_value_str = mt_ops->timeline_value_str;
#endif
#endif

	*ops = dma_ops;

	return 0;
}

void os_set_dma_ops(struct device *dev, const struct dma_map_ops *dma_ops)
{
	set_dma_ops(dev, dma_ops);
}

int os_dma_fence_get_status(struct dma_fence *fence)
{
	return dma_fence_get_status(fence);
}

const char *os_dma_fence_get_timeline_name(struct dma_fence *fence)
{
	if (!fence || !fence->ops || !fence->ops->get_timeline_name)
		return NULL;

	return fence->ops->get_timeline_name(fence);
}

const char *os_dma_fence_get_driver_name(struct dma_fence *fence)
{
	if (!fence || !fence->ops || !fence->ops->get_driver_name)
		return NULL;

	return fence->ops->get_driver_name(fence);
}

long os_dma_fence_wait_timeout(struct dma_fence *fence, bool intr, long timeout)
{
	return dma_fence_wait_timeout(fence, intr, timeout);
}

long os_dma_fence_wait_any_timeout(struct dma_fence **fences, u32 count,
				   bool intr, long timeout, u32 *idx)
{
	return dma_fence_wait_any_timeout(fences, count, intr, timeout, idx);
}

void __os_dma_fence_put(struct dma_fence *fence)
{
	dma_fence_put(fence);
}

struct dma_fence *__os_dma_fence_get(struct dma_fence *fence)
{
	return dma_fence_get(fence);
}

struct dma_fence *__os_dma_fence_get_rcu(struct dma_fence *fence)
{
	return dma_fence_get_rcu(fence);
}

void __os_dma_fence_signal(struct dma_fence *fence)
{
	dma_fence_signal(fence);
}

u64 os_dma_fence_timestamp(struct dma_fence *fence)
{
#if defined(OS_FUNC_DMA_FENCE_TIMESTAMP_EXIST)
	return dma_fence_timestamp(fence);
#else
	return 0;
#endif
}

bool os_dma_fence_is_later(struct dma_fence *f1, struct dma_fence *f2)
{
	return dma_fence_is_later(f1, f2);
}

#if !defined(OS_FUNC_DMA_FENCE_GET_STUB_EXIST)
static const char *os_dma_fence_stub_get_name(struct dma_fence *fence)
{
	return "stub";
}
#endif

struct dma_fence *__os_dma_fence_get_stub(void)
{
#if defined(OS_FUNC_DMA_FENCE_GET_STUB_EXIST)
	return dma_fence_get_stub();
#else
	static struct dma_fence dma_fence_stub;
	static DEFINE_SPINLOCK(dma_fence_stub_lock);
	static const struct dma_fence_ops dma_fence_stub_ops = {
		.get_driver_name = os_dma_fence_stub_get_name,
		.get_timeline_name = os_dma_fence_stub_get_name,
	};

	spin_lock(&dma_fence_stub_lock);
	if (!dma_fence_stub.ops) {
		dma_fence_init(&dma_fence_stub,
			       &dma_fence_stub_ops,
			       &dma_fence_stub_lock,
			       0, 0);
		dma_fence_signal_locked(&dma_fence_stub);
	}
	spin_unlock(&dma_fence_stub_lock);

	return dma_fence_get(&dma_fence_stub);
#endif
}

bool os_dma_fence_is_array(struct dma_fence *fence)
{
	return dma_fence_is_array(fence);
}

struct dma_fence_array *os_to_dma_fence_array(struct dma_fence *fence)
{
	return to_dma_fence_array(fence);
}

struct dma_fence *os_dma_fence_from_rcu(struct rcu_head *rcu)
{
	return container_of(rcu, struct dma_fence, rcu);
}

struct dma_fence **os_dma_fence_array_get_fences(struct dma_fence_array *array)
{
	return array->fences;
}

u32 os_dma_fence_array_get_fences_num(struct dma_fence_array *array)
{
	return array->num_fences;
}

bool os_dma_fence_is_signaled(struct dma_fence *fence)
{
	return dma_fence_is_signaled(fence);
}

void os_dma_fence_set_error(struct dma_fence *fence, int error)
{
	dma_fence_set_error(fence, error);
}

int os_dma_fence_get_error(struct dma_fence *fence)
{
	return fence->error;
}

u64 os_dma_fence_context_alloc(unsigned num)
{
	return dma_fence_context_alloc(num);
}

struct dma_fence_array *os_dma_fence_array_create(int num_fences, struct dma_fence **fences,
						  u64 context, unsigned seqno, bool signal_on_any)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(7, 0, 0)
	/* signal_on_any was dropped; arrays now always signal on the last fence. */
	return dma_fence_array_create(num_fences, fences, context, seqno);
#else
	return dma_fence_array_create(num_fences, fences, context, seqno, signal_on_any);
#endif
}

int os_dma_fence_add_callback(struct dma_fence *fence, struct dma_fence_cb *cb,
			      dma_fence_func_t func)
{
	return dma_fence_add_callback(fence, cb, func);
}

bool os_dma_fence_remove_callback(struct dma_fence *fence, struct dma_fence_cb *cb)
{
	return dma_fence_remove_callback(fence, cb);
}

signed long os_dma_fence_wait(struct dma_fence *fence, bool intr)
{
	return dma_fence_wait(fence, intr);
}

void os_dma_fence_enable_sw_signaling(struct dma_fence *fence)
{
	dma_fence_enable_sw_signaling(fence);
}

struct dma_fence *os_get_fence_array_base(struct dma_fence_array *fence_array)
{
	return &fence_array->base;
}

int os_dma_resv_reserve_shared(struct dma_resv *obj, unsigned int num_fences)
{
#if defined(OS_FUNC_DMA_RESV_RESERVE_FENCES_EXIST)
	return dma_resv_reserve_fences(obj, num_fences);
#elif defined(OS_LINUX_DMA_RESV_H_EXIST)
	return dma_resv_reserve_shared(obj, num_fences);
#elif defined(OS_RESERVATION_OBJECT_RESERVE_SHARED_HAS_NUM_FENCES_ARG)
	return reservation_object_reserve_shared((struct reservation_object *)obj, num_fences);
#else
	unsigned int i;
	int err;

	for (i = 0; i < num_fences; i++) {
		err = reservation_object_reserve_shared((struct reservation_object *)obj);
		if (err)
			return err;
	}
	return 0;
#endif
}

struct ww_mutex *os_dma_resv_get_ww_mutex(struct dma_resv *resv)
{
#if defined(OS_LINUX_DMA_RESV_H_EXIST)
	return &resv->lock;
#else
	return &((struct reservation_object *)resv)->lock;
#endif
}

void os_dma_resv_add_excl_fence(struct dma_resv *obj, struct dma_fence *fence)
{
#if defined(OS_FUNC_DMA_RESV_ADD_FENCE_EXIST)
	dma_resv_reserve_fences(obj, 1);
	dma_resv_add_fence(obj, fence, DMA_RESV_USAGE_WRITE);
#elif defined(OS_LINUX_DMA_RESV_H_EXIST)
	dma_resv_add_excl_fence(obj, fence);
#else
	reservation_object_add_excl_fence((struct reservation_object *)obj, fence);
#endif
}

void os_dma_resv_add_shared_fence(struct dma_resv *obj, struct dma_fence *fence)
{
#if defined(OS_FUNC_DMA_RESV_ADD_FENCE_EXIST)
	dma_resv_add_fence(obj, fence, DMA_RESV_USAGE_READ);
#elif defined(OS_LINUX_DMA_RESV_H_EXIST)
	dma_resv_add_shared_fence(obj, fence);
#else
	reservation_object_add_shared_fence((struct reservation_object *)obj, fence);
#endif
}

int os_dma_resv_get_fences(struct dma_resv *obj,
			   struct dma_fence **pfence_excl,
			   unsigned int *num_fences,
			   struct dma_fence ***pfences,
			   bool usage_write,
			   bool *fence_overall)
{
	*fence_overall = false;

#if defined(OS_ENUM_DMA_RESV_USAGE_EXIST)
	*fence_overall = true;

	return dma_resv_get_fences(obj, usage_write ? DMA_RESV_USAGE_READ :
				   DMA_RESV_USAGE_WRITE, num_fences, pfences);
#elif defined(OS_FUNC_DMA_RESV_GET_FENCES_EXIST)
	return dma_resv_get_fences(obj, pfence_excl, num_fences, pfences);
#elif defined(OS_LINUX_DMA_RESV_H_EXIST)
	return dma_resv_get_fences_rcu(obj, pfence_excl, num_fences, pfences);
#else
	return reservation_object_get_fences_rcu((struct reservation_object *)obj,
						 pfence_excl, num_fences, pfences);
#endif
}

#if defined(OS_FUNC_DMA_RESV_USAGE_RW_EXIST)
u32 os_dma_resv_usage_rw(bool write)
{
	return dma_resv_usage_rw(write);
}
#else
u32 os_dma_resv_usage_rw(bool write)
{
	return 0;
}
#endif

#if defined(OS_FUNC_DMA_RESV_ASSERT_HELD_EXIST)
void os_dma_resv_assert_held(void *obj)
{
	dma_resv_assert_held((struct dma_resv *)obj);
}
#else
void os_dma_resv_assert_held(void *obj)
{
}
#endif

#if defined(OS_FUNC_DMA_RESV_ITER_BEGIN_EXIST)
void os_dma_resv_iter_begin(void *cursor, void *obj, u32 usage)
{
	dma_resv_iter_begin((struct dma_resv_iter *)cursor, (struct dma_resv *)obj, usage);
}
#else
void os_dma_resv_iter_begin(void *cursor, void *obj, u32 usage)
{
}
#endif

#if defined(OS_FUNC_DMA_RESV_ITER_FIRST_EXIST)
struct dma_fence *os_dma_resv_iter_first(void *cursor)
{
	return dma_resv_iter_first((struct dma_resv_iter *)cursor);
}
#else
struct dma_fence *os_dma_resv_iter_first(void *cursor)
{
	return NULL;
}
#endif

#if defined(OS_FUNC_DMA_RESV_ITER_NEXT_EXIST)
struct dma_fence *os_dma_resv_iter_next(void *cursor)
{
	return dma_resv_iter_next((struct dma_resv_iter *)cursor);
}
#else
struct dma_fence *os_dma_resv_iter_next(void *cursor)
{
	return NULL;
}
#endif

#if defined(OS_STRUCT_DMA_RESV_ITER_EXIST)
void *os_create_dma_resv_iter(void)
{
	return kzalloc(sizeof(struct dma_resv_iter), GFP_KERNEL);
}

void os_destroy_dma_resv_iter(void *iter)
{
	kfree(iter);
}
#else
void *os_create_dma_resv_iter(void)
{
	return NULL;
}

void os_destroy_dma_resv_iter(void *iter)
{
	kfree(iter);
}
#endif

struct ww_class *os_get_reservation_ww_class(void)
{
	return &reservation_ww_class;
}

struct ww_acquire_ctx *os_ww_acquire_ctx_create(void)
{
	return kzalloc(sizeof(struct ww_acquire_ctx), GFP_KERNEL);
}

void os_ww_acquire_ctx_destroy(struct ww_acquire_ctx *acquire_ctx)
{
	return kfree(acquire_ctx);
}

void os_ww_acquire_init(struct ww_acquire_ctx *ctx,
			struct ww_class *ww_class)
{
	ww_acquire_init(ctx, ww_class);
}

int os_ww_mutex_lock_interruptible(struct ww_mutex *lock, struct ww_acquire_ctx *ctx)
{
	return ww_mutex_lock_interruptible(lock, ctx);
}

void os_ww_acquire_done(struct ww_acquire_ctx *ctx)
{
	ww_acquire_done(ctx);
}

void os_ww_mutex_unlock(struct ww_mutex *lock)
{
	ww_mutex_unlock(lock);
}

void os_ww_acquire_fini(struct ww_acquire_ctx *ctx)
{
	ww_acquire_fini(ctx);
}

int os_ww_mutex_lock_slow_interruptible(struct ww_mutex *lock,
					struct ww_acquire_ctx *ctx)
{
	return ww_mutex_lock_slow_interruptible(lock, ctx);
}

int os_get_unused_fd_flags(unsigned flag)
{
	return get_unused_fd_flags(flag);
}

void os_fd_install_sync_file(int fd, struct sync_file *sync_file)
{
	fd_install(fd, sync_file->file);
}

struct sync_file *os_sync_file_create(struct dma_fence *fence)
{
	return sync_file_create(fence);
}

struct dma_fence *os_sync_file_get_fence(int fd)
{
	return sync_file_get_fence(fd);
}

struct gen_pool *os_gen_pool_create(int min_alloc_order, int nid)
{
	return gen_pool_create(min_alloc_order, nid);
}

void os_gen_pool_destroy(struct gen_pool *pool)
{
	gen_pool_destroy(pool);
}

int os_gen_pool_add_owner(struct gen_pool *pool, unsigned long virt, phys_addr_t phys,
			  size_t size, int nid, void *owner)
{
#ifdef OS_STRUCT_GEN_POOL_CHUNK_HAS_OWNER
	return gen_pool_add_owner(pool, virt, phys, size, nid, owner);
#else
	return gen_pool_add(pool, virt, size, nid);
#endif
}

unsigned long os_gen_pool_alloc_owner(struct gen_pool *pool, size_t size, void **owner)
{
#ifdef OS_STRUCT_GEN_POOL_CHUNK_HAS_OWNER
	return gen_pool_alloc_owner(pool, size, owner);
#else
	*owner = NULL;
	return gen_pool_alloc(pool, size);
#endif
}

void os_gen_pool_free(struct gen_pool *pool, unsigned long addr, size_t size)
{
	gen_pool_free(pool, addr, size);
}

#if defined(OS_STRUCT_XARRAY_EXIST)
void *os_create_xarray(void)
{
	struct xarray *array = kzalloc(sizeof(*array), GFP_KERNEL);

	if (!array)
		return NULL;

	xa_init_flags(array, XA_FLAGS_ALLOC);

	return array;
}

void os_destroy_xarray(void *array)
{
	kfree(array);
}

void *os_xa_load(void *xa, unsigned long index)
{
	return xa_load((struct xarray *)xa, index);
}

void os_xa_destroy(void *xa)
{
	xa_destroy((struct xarray *)xa);
}

void *os_xa_find(void *xa, unsigned long *indexp,
		 unsigned long max, unsigned filter)
{
	return xa_find((struct xarray *)xa, indexp, max, filter);
}

void *os_xa_find_after(void *xa, unsigned long *indexp,
		       unsigned long max, unsigned filter)
{
	return xa_find_after((struct xarray *)xa, indexp, max, filter);
}

void *os_xa_erase(void *xa, unsigned long index)
{
	return xa_erase((struct xarray *)xa, index);
}

void *os_xa_store(void *xa, unsigned long index, void *entry)
{
	return xa_store((struct xarray *)xa, index, entry, GFP_KERNEL);
}

int os_xa_alloc(void *xa, u32 *id, void *entry)
{
#if defined(OS_XA_ALLOC_USE_XA_LIMIT)
	return xa_alloc((struct xarray *)xa, id, entry, xa_limit_32b, GFP_KERNEL);
#else
	return xa_alloc((struct xarray *)xa, id, UINT_MAX, entry, GFP_KERNEL);
#endif
}
#else
/* TODO: implement on kernel without xarray*/
void *os_create_xarray(void)
{
	return NULL;
}

void os_destroy_xarray(void *array)
{
	return;
}

void *os_xa_load(void *xa, unsigned long index)
{
	return NULL;
}

void os_xa_destroy(void *xa)
{
	return;
}

void *os_xa_find(void *xa, unsigned long *indexp,
		 unsigned long max, unsigned filter)
{
	return NULL;
}

void *os_xa_find_after(void *xa, unsigned long *indexp,
		       unsigned long max, unsigned filter)
{
	return NULL;
}

void *os_xa_erase(void *xa, unsigned long index)
{
	return NULL;
}

void *os_xa_store(void *xa, unsigned long index, void *entry)
{
	return NULL;
}

int os_xa_alloc(void *xa, u32 *id, void *entry)
{
	return 0;
}

#endif

int os_devfreq_dev_profile_init(struct devfreq_dev_profile **profile,
				struct mt_devfreq_dev_profile *mt_profile)
{
	struct devfreq_dev_profile *devfreq_profile;

	devfreq_profile = kzalloc(sizeof(*devfreq_profile), GFP_KERNEL);
	if (!devfreq_profile)
		return -ENOMEM;

	devfreq_profile->initial_freq = mt_profile->initial_freq;
	devfreq_profile->polling_ms = mt_profile->polling_ms;
	devfreq_profile->freq_table = mt_profile->freq_table;
	devfreq_profile->max_state = mt_profile->max_state;
	devfreq_profile->target = mt_profile->target;
	devfreq_profile->get_cur_freq = mt_profile->get_cur_freq;
	devfreq_profile->get_dev_status = mt_profile->get_dev_status;

	*profile = devfreq_profile;

	return 0;
}

int os_devfreq_simple_ondemand_data_init(struct devfreq_simple_ondemand_data **data,
					 u32 up_threshold,
					 u32 down_differential)
{
#if defined(OS_STRUCT_DEVFREQ_SIMPLE_ONDEMAND_DATA_EXIST)
	struct devfreq_simple_ondemand_data *ondemand_data;

	ondemand_data = kzalloc(sizeof(*ondemand_data), GFP_KERNEL);
	if (!ondemand_data)
		return -ENOMEM;

	ondemand_data->upthreshold = up_threshold;
	ondemand_data->downdifferential = down_differential;

	*data = ondemand_data;

	return 0;
#else
	return -EINVAL;
#endif
}

int os_devfreq_suspend_device(struct devfreq *devfreq)
{
	return devfreq_suspend_device(devfreq);
}

int os_devfreq_resume_device(struct devfreq *devfreq)
{
	return devfreq_resume_device(devfreq);
}

int os_devfreq_register_opp_notifier(struct device *dev, struct devfreq *devfreq)
{
	return devfreq_register_opp_notifier(dev, devfreq);
}

int os_devfreq_unregister_opp_notifier(struct device *dev,
				       struct devfreq *devfreq)
{
	return devfreq_unregister_opp_notifier(dev, devfreq);
}

struct devfreq *os_devm_devfreq_add_device(struct device *dev,
					   struct devfreq_dev_profile *profile,
					   const char *governor_name,
					   void *data)
{
	return devm_devfreq_add_device(dev, profile, governor_name, data);
}

void os_devm_devfreq_remove_device(struct device *dev, struct devfreq *devfreq)
{
	devm_devfreq_remove_device(dev, devfreq);
}

void os_dev_pm_opp_put(struct dev_pm_opp *opp)
{
	dev_pm_opp_put(opp);
}

struct dev_pm_opp *os_dev_pm_opp_find_freq_ceil(struct device *dev, unsigned long *freq)
{
	return dev_pm_opp_find_freq_ceil(dev, freq);
}

int os_dev_pm_opp_get_opp_count(struct device *dev)
{
	return dev_pm_opp_get_opp_count(dev);
}

int os_dev_pm_opp_add(struct device *dev, unsigned long freq, unsigned long u_volt)
{
	return dev_pm_opp_add(dev, freq, u_volt);
}

void os_dev_pm_opp_remove_table(struct device *dev)
{
	dev_pm_opp_remove_table(dev);
}

struct dev_pm_opp *os_devfreq_recommended_opp(struct device *dev,
					      unsigned long *freq,
					      u32 flags)
{
	return devfreq_recommended_opp(dev, freq, flags);
}

unsigned long os_dev_pm_opp_get_freq(struct dev_pm_opp *opp)
{
	return dev_pm_opp_get_freq(opp);
}

unsigned long os_dev_pm_opp_get_voltage(struct dev_pm_opp *opp)
{
	return dev_pm_opp_get_voltage(opp);
}

void os_dev_pm_opp_remove(struct device *dev, unsigned long freq)
{
	dev_pm_opp_remove(dev, freq);
}

int os_dev_pm_domain_attach(struct device *dev, bool power_on)
{
	return dev_pm_domain_attach(dev, power_on);
}

void os_dev_pm_domain_detach(struct device *dev, bool power_off)
{
	dev_pm_domain_detach(dev, power_off);
}

bool os_dev_has_pm_domain(struct device *dev)
{
	return dev && dev->pm_domain;
}

void os_dev_pm_genpd_resume(struct device *dev)
{
	int err;
	struct generic_pm_domain *genpd;

	if (!dev->pm_domain)
		return;

	genpd = pd_to_genpd(dev->pm_domain);
	if (!genpd || !genpd->power_on)
		return;

	if (genpd->status == GENPD_STATE_ON)
		return;

	err = genpd->power_on(genpd);
	if (err)
		os_dev_err(dev, "genpd power on failed %d", err);

	genpd->status = GENPD_STATE_ON;
}

void os_dev_pm_genpd_suspend(struct device *dev)
{
	int err;
	struct generic_pm_domain *genpd;

	if (!dev->pm_domain)
		return;

	genpd = pd_to_genpd(dev->pm_domain);
	if (!genpd || !genpd->power_on)
		return;

	if (genpd->status == GENPD_STATE_OFF)
		return;

	err = genpd->power_off(genpd);
	if (err)
		os_dev_err(dev, "genpd power off failed %d", err);

	genpd->status = GENPD_STATE_OFF;
}

void os_thermal_cooling_device_unregister(struct thermal_cooling_device *cdev)
{
	thermal_cooling_device_unregister(cdev);
}

struct thermal_cooling_device *
os_thermal_cooling_device_register(char *type,
				   void *devdata,
				   const struct thermal_cooling_device_ops *ops)
{
	return thermal_cooling_device_register(type, devdata, ops);
}

IMPLEMENT_OS_STRUCT_COMMON_FUNCS(thermal_cooling_device_ops);
IMPLEMENT_GET_OS_MEMBER_FUNC(thermal_cooling_device, ops);
IMPLEMENT_SET_OS_MEMBER_FUNC(devfreq_dev_status, current_frequency);
IMPLEMENT_SET_OS_MEMBER_FUNC(devfreq_dev_status, busy_time);
IMPLEMENT_SET_OS_MEMBER_FUNC(devfreq_dev_status, total_time);

void os_thermal_cooling_device_ops_init(struct thermal_cooling_device_ops *ops,
					int (*get_max_state)(struct thermal_cooling_device *,
							     unsigned long *),
					int (*get_cur_state)(struct thermal_cooling_device *,
							     unsigned long *),
					int (*set_cur_state)(struct thermal_cooling_device *,
							     unsigned long))
{
	ops->get_cur_state = get_cur_state;
	ops->get_max_state = get_max_state;
	ops->set_cur_state = set_cur_state;
}

void os_set_devfreq_min_freq(struct devfreq *devfreq, unsigned long min_freq)
{
#if defined(OS_STRUCT_DEVFREQ_HAS_MIN_FREQ)
	devfreq->min_freq = min_freq;
#else
	devfreq->scaling_min_freq = min_freq;
#endif
}

void os_set_devfreq_max_freq(struct devfreq *devfreq, unsigned long max_freq)
{
#if defined(OS_STRUCT_DEVFREQ_HAS_MIN_FREQ)
	devfreq->max_freq = max_freq;
#else
	devfreq->scaling_max_freq = max_freq;
#endif
}

void os_set_devfreq_suspend_freq(struct devfreq *devfreq, unsigned long suspend_freq)
{
#if defined(OS_STRUCT_DEVFREQ_HAS_SUSPEND_FREQ)
	devfreq->suspend_freq = suspend_freq;
#endif
}

/*About dev print*/
static void __os_dev_printk(const char *level, const struct device *dev,
			    struct va_format *vaf)
{
	if (dev)
		dev_printk_emit(level[1] - '0', dev, "%s %s: %pV",
				dev_driver_string(dev), dev_name(dev), vaf);
	else
		printk("%s(NULL device *): %pV", level, vaf);
}

void *OS_ERR_CAST(__force const void *ptr)
{
	return ERR_CAST(ptr);
}

void *OS_ERR_PTR(long error)
{
	return ERR_PTR(error);
}

bool OS_IS_ERR(const void *ptr)
{
	return IS_ERR(ptr);
}

bool OS_IS_ERR_OR_NULL(__force const void *ptr)
{
	return IS_ERR_OR_NULL(ptr);
}

long OS_PTR_ERR(__force const void *ptr)
{
	return PTR_ERR(ptr);
}

int OS_READ_ONCE(int *val)
{
	return READ_ONCE(*val);
}

void OS_WRITE_ONCE(void **ptr, void *val)
{
	WRITE_ONCE(*ptr, val);
}

bool OS_WARN_ON(bool condition)
{
	return WARN_ON(condition);
}

bool OS_WARN_ON_ONCE(bool condition)
{
	return WARN_ON_ONCE(condition);
}

void OS_BUG_ON(bool condition)
{
	BUG_ON(condition);
}

int OS_IS_POSIXACL(struct inode *inode)
{
	return IS_POSIXACL(inode);
}

unsigned long OS_PAGE_OFFSET(void)
{
	return PAGE_OFFSET;
}

void os_dump_stack(void)
{
	dump_stack();
}

int os_sscanf(const char *str, const char *fmt, ...)
{
	va_list args;
	int i;

	va_start(args, fmt);
	i = vsscanf(str, fmt, args);
	va_end(args);

	return i;
}

char *os_strcat(char *dest, const char *src)
{
	return strcat(dest, src);
}

size_t os_strlen(const char *s)
{
	return strlen(s);
}

size_t os_strlcat(char *dest, const char *src, size_t count)
{
	return strlcat(dest, src, count);
}

size_t os_strscpy(char *dest, const char *src, size_t size)
{
	return strscpy(dest, src, size);
}

int os_strcmp(const char *cs, const char *ct)
{
	return strcmp(cs, ct);
}

int os_strncmp(const char *cs, const char *ct, size_t count)
{
	return strncmp(cs, ct, count);
}

char *os_strcpy(char *dest, const char *src)
{
	return strcpy(dest, src);
}

char *os_strncpy(char *dest, const char *src, size_t count)
{
	/* Keep strncpy's fixed-width, zero-padding semantics for the binary core. */
	size_t len = strnlen(src, count);

	memcpy(dest, src, len);
	memset(dest + len, 0, count - len);
	return dest;
}

char *os_strchr(const char *s, int c)
{
	return strchr(s, c);
}

char *os_strrchr(const char *s, int c)
{
	return strrchr(s, c);
}

char *os_strstr(const char *s1, const char *s2)
{
	return strstr(s1, s2);
}

int os_kstrtol(const char *s, unsigned int base, long *res)
{
	return kstrtol(s, base, res);
}

int os_kstrtoull(const char *s, unsigned int base, unsigned long long *res)
{
	return kstrtoull(s, base, res);
}

char *os_strsep(char **s, const char *delim)
{
	return strsep(s, delim);
}

int os_sprintf(char *buf, const char *fmt, ...)
{
	va_list args;
	int i;

	va_start(args, fmt);
	i = vsnprintf(buf, INT_MAX, fmt, args);
	va_end(args);

	return i;
}

char *os_strtrim(char *src)
{
	int i = 0, j;

	j = os_strlen(src);
	if (j == 0)
		return src;

	while (src[i] == ' ')
		i++;

	j -= 1;
	while (src[j] == ' ')
		j--;

	src[j + 1] = '\0';

	return src + i;
}

void os_seq_printf(struct seq_file *m, const char *f, ...)
{
	va_list args;

	va_start(args, f);
	seq_vprintf(m, f, args);
	va_end(args);
}

const struct dmi_device *os_dmi_find_slot_device(const char *name, const struct dmi_device *dev)
{
	return dmi_find_device(-4, name, dev);
}

const struct dmi_dev_onboard *os_get_dmi_device_data(const struct dmi_device *dev)
{
	return dev->device_data;
}

#if defined(OS_STRUCT_DMI_DEV_ONBOARD_EXIST)
int os_get_dmi_device_board_segment(const struct dmi_dev_onboard *dev_onboard)
{
	return dev_onboard->segment;
}

int os_get_dmi_device_board_bus(const struct dmi_dev_onboard *dev_onboard)
{
	return dev_onboard->bus;
}

int os_get_dmi_device_board_devfn(const struct dmi_dev_onboard *dev_onboard)
{
	return dev_onboard->devfn;
}

int os_get_dmi_device_board_instance(const struct dmi_dev_onboard *dev_onboard)
{
	return dev_onboard->instance;
}

const char *os_get_dmi_device_board_name(const struct dmi_dev_onboard *dev_onboard)
{
	return dev_onboard->dev.name;
}
#else
int os_get_dmi_device_board_segment(const struct dmi_dev_onboard *dev_onboard)
{
        return 0;
}

int os_get_dmi_device_board_bus(const struct dmi_dev_onboard *dev_onboard)
{
        return 0;
}

int os_get_dmi_device_board_devfn(const struct dmi_dev_onboard *dev_onboard)
{
        return 0;
}

int os_get_dmi_device_board_instance(const struct dmi_dev_onboard *dev_onboard)
{
        return 0;
}

const char *os_get_dmi_device_board_name(const struct dmi_dev_onboard *dev_onboard)
{
        return NULL;
}
#endif

DEFINE_SPINLOCK(mtgpu_global_bo_handle_idr_spinlock);

int os_idr_create_record(struct idr **idr, const char *func_name, u32 line_num)
{
	*idr = kzalloc(sizeof(**idr), GFP_KERNEL);
	if (!(*idr))
		return -ENOMEM;

	mtgpu_stats_add_kmalloc((u64)*idr, 0, ksize(*idr), func_name, line_num);

	idr_init(*idr);

	return 0;
}

void os_idr_destroy(struct idr *idr)
{
	idr_destroy(idr);
	os_kfree(idr);
}

int os_idr_alloc(struct idr *idr, void *ptr, int start, int end, gfp_t gfp_flags)
{
	return idr_alloc(idr, ptr, start, end, gfp_flags);
}

void *os_idr_find(const struct idr *idr, unsigned long id)
{
	return idr_find(idr, id);
}

void *os_idr_remove(struct idr *idr, unsigned long id)
{
	return idr_remove(idr, id);
}

void os_idr_preload(void)
{
	idr_preload(GFP_KERNEL);
}

void os_idr_preload_end(void)
{
	idr_preload_end();
}

void *os_idr_replace(struct idr *idr, void *ptr, unsigned long id)
{
	void *entry = idr_replace(idr, ptr, id);

	return IS_ERR_OR_NULL(entry) ? NULL : entry;
}

int os_idr_for_each(const struct idr *idr,
		    int (*fn)(int id, void *p, void *data),
		    void *data)
{
	return idr_for_each(idr, fn, data);
}

bool os_running_on_hypervisor(void)
{
#ifdef CONFIG_X86
	return boot_cpu_has(X86_FEATURE_HYPERVISOR);
#else
	return false;
#endif
}

bool os_x86_vendor_is_intel(void)
{
#ifdef CONFIG_X86
	return boot_cpu_data.x86_vendor == X86_VENDOR_INTEL;
#else
	return false;
#endif
}

bool os_is_kernel_thread(void)
{
	return current->mm == NULL;
}

int os_netlink_kernel_cfg_create(struct netlink_kernel_cfg **cfg,
				 void (*input)(struct sk_buff *skb))
{
	*cfg = kzalloc(sizeof(struct netlink_kernel_cfg), GFP_KERNEL);
	if (!(*cfg))
		return -ENOMEM;

	(*cfg)->input = input;

	return 0;
}

void os_netlink_kernel_cfg_destroy(struct netlink_kernel_cfg *cfg)
{
	kfree(cfg);
}

struct sock *os_netlink_kernel_create(struct net *net, int unit,
				      struct netlink_kernel_cfg *cfg)
{
	return netlink_kernel_create(net, unit, cfg);
}

unsigned int os_stack_trace_save(unsigned long *store, unsigned int size, unsigned int skipnr)
{
#if defined(OS_FUNC_STACK_TRACE_SAVE_EXIST)
	return stack_trace_save(store, size, skipnr);
#else
	return 0;
#endif
}

void os_netlink_kernel_release(struct sock *sk)
{
	netlink_kernel_release(sk);
}

struct nlmsghdr *os_nlmsg_hdr(struct sk_buff *skb)
{
	return nlmsg_hdr(skb);
}

char *os_nlmsg_data(struct nlmsghdr *nlh)
{
	return NLMSG_DATA(nlh);
}

int os_nlmsg_len(struct nlmsghdr *nlh)
{
	return nlmsg_len(nlh);
}

struct sk_buff *os_nlmsg_new(size_t payload, gfp_t flags)
{
	return nlmsg_new(payload, flags);
}

void os_nlmsg_free(struct sk_buff *skb)
{
	nlmsg_free(skb);
}

struct nlmsghdr *os_nlmsg_put(struct sk_buff *skb, u32 portid, u32 seq,
			      int type, int payload, int flags)
{
	return nlmsg_put(skb, portid, seq, type, payload, flags);
}

int os_nlmsg_unicast(struct sock *sk, struct sk_buff *skb, u32 portid)
{
	return nlmsg_unicast(sk, skb, portid);
}

struct sk_buff *os_genlmsg_new(size_t payload, gfp_t flags)
{
	return genlmsg_new(payload, flags);
}

void *os_genlmsg_put(struct sk_buff *skb, u32 portid, u32 seq,
		     const struct genl_family *family, int flags, u8 cmd)
{
	return genlmsg_put(skb, portid, seq, family, flags, cmd);
}

void *os_genlmsg_put_reply(struct sk_buff *skb, struct genl_info *info,
			   const struct genl_family *family, int flags, u8 cmd)
{
	return genlmsg_put_reply(skb, info, family, flags, cmd);
}

void os_genlmsg_cancel(struct sk_buff *skb, void *hdr)
{
	genlmsg_cancel(skb, hdr);
}

void os_genlmsg_end(struct sk_buff *skb, void *hdr)
{
	genlmsg_end(skb, hdr);
}

int os_genlmsg_unicast(struct net *net, struct sk_buff *skb, u32 portid)
{
	return genlmsg_unicast(net, skb, portid);
}

int os_genlmsg_reply(struct sk_buff *skb, struct genl_info *info)
{
	return genlmsg_reply(skb, info);
}

int os_nla_put(struct sk_buff *skb, int attrtype, int attrlen, const void *data)
{
	return nla_put(skb, attrtype, attrlen, data);
}

struct net *os_genl_info_net(struct genl_info *info)
{
	return genl_info_net(info);
}

u32 os_get_genl_snd_portid(struct genl_info *info)
{
	return info->snd_portid;
}

u32 os_get_genl_snd_seq(struct genl_info *info)
{
	return info->snd_seq;
}

struct nlattr *os_get_nlattr_data(struct genl_info *info, u32 attr)
{
	return info->attrs[attr];
}

int os_nla_len(const struct nlattr *nla)
{
	return nla_len(nla);
}

int os_nla_memcpy(void *dest, const struct nlattr *src, int count)
{
	return nla_memcpy(dest, src, count);
}

int os_genl_register_family(struct mt_genl_family *mt_family, struct genl_family **family)
{
	int ret, i;
	unsigned int maxattr, n_ops;
	struct nla_policy *policy;
	struct genl_ops *ops;

	*family = kzalloc(sizeof(struct genl_family), GFP_KERNEL);
	if (!(*family))
		return -ENOMEM;

	maxattr = mt_family->maxattr;
	n_ops = mt_family->n_ops;

	policy = kzalloc((maxattr + 1) * sizeof(struct nla_policy), GFP_KERNEL);
	if (!policy) {
		ret = -ENOMEM;
		goto policy_kalloc_err;
	}

	ops = kzalloc(n_ops * sizeof(struct genl_ops), GFP_KERNEL);
	if (!ops) {
		ret = -ENOMEM;
		goto ops_kalloc_err;
	}

	for (i = 0; i < (maxattr + 1); i++) {
		policy[i].type = mt_family->policy[i].type;
		policy[i].len = mt_family->policy[i].len;
	}

	for (i = 0; i < n_ops; i++) {
		ops[i].cmd = mt_family->ops[i].cmd;
		ops[i].flags = mt_family->ops[i].flags;
		ops[i].doit = mt_family->ops[i].doit;
#if defined(OS_GENL_OPS_HAS_POLICY)
		ops[i].policy = mt_family->ops[i].policy;
#endif
	}

	memcpy((*family)->name, mt_family->name, GENL_NAMSIZ);
	(*family)->version = mt_family->version;
	(*family)->maxattr = mt_family->maxattr;
#if defined(OS_GENL_FAMILY_HAS_POLICY)
	(*family)->policy = policy;
#endif
	(*family)->ops = ops;
	(*family)->n_ops = n_ops;

	return genl_register_family(*family);

ops_kalloc_err:
	kfree(policy);
policy_kalloc_err:
	kfree(*family);
	*family = NULL;
	return ret;
}

int os_genl_unregister_family(const struct genl_family *family)
{
	int ret;

	ret = genl_unregister_family(family);
#if defined(OS_GENL_FAMILY_HAS_POLICY)
	kfree(family->policy);
#elif defined(OS_GENL_OPS_HAS_POLICY)
	kfree(family->ops[0].policy);
#endif
	kfree(family->ops);
	kfree(family);

	return ret;
}

/*
 * Test whether a block of memory is a valid user space address.
 * Returns 1 if the range is valid, 0 otherwise.
 *
 * This is equivalent to the following test:
 * (u65)addr + (u65)size <= (u65)TASK_SIZE_MAX
 *
 * type: 0 for VERIFY_READ
 * 	 1 for VERIFY_WRITE
 */
int os_access_ok(int type, const void __user *addr, unsigned long size)
{
#ifdef OS_ACCESS_OK_HAS_TWO_ARGS
	return access_ok(addr, size);
#else
	return access_ok(type, addr, size);
#endif
}

#define define_os_dev_printk_level(func, kern_level)		\
void func(const struct device *dev, const char *fmt, ...)	\
{								\
	struct va_format vaf;					\
	va_list args;						\
								\
	va_start(args, fmt);					\
								\
	vaf.fmt = fmt;						\
	vaf.va = &args;						\
								\
	__os_dev_printk(kern_level, dev, &vaf);			\
								\
	va_end(args);						\
}								\

define_os_dev_printk_level(_os_dev_emerg, KERN_EMERG);
define_os_dev_printk_level(_os_dev_alert, KERN_ALERT);
define_os_dev_printk_level(_os_dev_crit, KERN_CRIT);
define_os_dev_printk_level(_os_dev_err, KERN_ERR);
define_os_dev_printk_level(_os_dev_warn, KERN_WARNING);
define_os_dev_printk_level(_os_dev_notice, KERN_NOTICE);
define_os_dev_printk_level(_os_dev_info, KERN_INFO);
define_os_dev_printk_level(_os_dev_dbg, KERN_DEBUG);

IMPLEMENT_OS_STRUCT_COMMON_FUNCS(notifier_block);
IMPLEMENT_OS_STRUCT_COMMON_FUNCS(poll_table_struct);
IMPLEMENT_OS_STRUCT_COMMON_FUNCS(wait_queue_entry);
IMPLEMENT_OS_STRUCT_COMMON_FUNCS(timer_list);
IMPLEMENT_OS_STRUCT_COMMON_FUNCS(dma_fence_cb);
IMPLEMENT_OS_STRUCT_COMMON_FUNCS(hrtimer);

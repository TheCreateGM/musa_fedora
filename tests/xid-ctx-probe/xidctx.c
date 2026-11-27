// SPDX-License-Identifier: MIT
/*
 * xidctx - decisive probe: does the mtgpu mkdir succeed from a kworker
 * (workqueue) context the way the vendor driver runs it, vs. process context?
 *
 * The vendor mtgpu_drv_mkdir() runs in drv_mkdir_work_func() -- a workqueue.
 * This module runs the identical start_creating_path(AT_FDCWD, "/var/log/mtgpu",
 * LOOKUP_DIRECTORY) call from BOTH:
 *   1. module_init  (process context, = the insmod/modprobe process)
 *   2. a queued work item (kworker context, = how the driver does it)
 * and reports each result plus the calling task's fs root, so a context
 * difference is directly visible.
 *
 * Read-only: only path lookups on the real /var/log/mtgpu (which exists).
 *   make
 *   pkexec insmod ./xidctx.ko
 *   dmesg | grep XIDCTX
 *   pkexec rmmod xidctx
 */
#include <linux/module.h>
#include <linux/namei.h>
#include <linux/fs.h>
#include <linux/err.h>
#include <linux/workqueue.h>
#include <linux/dcache.h>
#include <linux/sched.h>
#include <linux/nsproxy.h>

static struct work_struct wq;
static char rootstr[256];

static void describe_root(const char *who)
{
	struct path root;
	char *p;

	memset(rootstr, 0, sizeof(rootstr));
	root = current->fs->root;
	p = d_path(&root, rootstr, sizeof(rootstr) - 1);
	pr_err("XIDCTX %s: pid=%d comm=%s fs_root=%s sb_dev=0x%lx root_ino=%lu mnt_ns=%p\n",
	       who, current->pid, current->comm,
	       IS_ERR(p) ? "(err)" : p,
	       (unsigned long)root.dentry->d_sb->s_dev,
	       (unsigned long)root.dentry->d_inode->i_ino,
	       current->nsproxy->mnt_ns);
}

static void do_lookup(const char *who)
{
	struct path p;
	struct dentry *d;

	describe_root(who);
	memset(&p, 0, sizeof(p));
	d = start_creating_path(AT_FDCWD, "/var/log/mtgpu", &p, LOOKUP_DIRECTORY);
	if (IS_ERR_OR_NULL(d))
		pr_err("XIDCTX %s: start_creating_path(/var/log/mtgpu, LOOKUP_DIRECTORY) -> err=%ld%s\n",
		       who, PTR_ERR(d),
		       (PTR_ERR(d) == -EEXIST) ? " (EEXIST: found, driver=SUCCESS)"
		       : (PTR_ERR(d) == -ENOENT) ? " (ENOENT: not found in this context, driver=FAIL)"
		       : "");
	else {
		pr_err("XIDCTX %s: -> OK dentry=%pd\n", who, d);
		end_creating_path(&p, d);
	}
}

static void wq_func(struct work_struct *work)
{
	do_lookup("kworker");
}

static int __init xidctx_init(void)
{
	pr_err("XIDCTX init\n");
	do_lookup("process");
	INIT_WORK(&wq, wq_func);
	schedule_work(&wq);
	flush_work(&wq);	/* wait for the kworker to finish before we log summary */
	return 0;
}
module_init(xidctx_init);

static void __exit xidctx_exit(void)
{
	pr_err("XIDCTX exit\n");
}
module_exit(xidctx_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Compare start_creating_path() in process vs kworker context for mtgpu XID logging");

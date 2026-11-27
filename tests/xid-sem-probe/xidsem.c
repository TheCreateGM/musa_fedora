// SPDX-License-Identifier: MIT
/*
 * xidsem - decisive probe of start_creating_path() semantics for the mtgpu
 * event-report/XID directory creation.
 *
 * The vendor mtgpu_drv_mkdir() calls
 *   start_creating_path(AT_FDCWD, path, &p, LOOKUP_DIRECTORY)
 * i.e. WITHOUT LOOKUP_CREATE.  Per fs/namei.c lookup_one_qstr_excl():
 *   "Will return -ENOENT if name isn't found and LOOKUP_CREATE wasn't passed."
 * This module proves the decisive untested case: target ABSENT, parent
 * present, with and without LOOKUP_CREATE -- does the call create the dir or
 * return -ENOENT?
 *
 * It only creates throw-away directories under /var/tmp/xidsem/ and removes
 * them again.  Nothing under /var/log or the real mtgpu tree is touched.
 *
 *   make -f Makefile.sem
 *   pkexec insmod ./xidsem.ko
 *   dmesg | grep XIDSEM
 *   pkexec rmmod xidsem
 */
#include <linux/module.h>
#include <linux/namei.h>
#include <linux/fs.h>
#include <linux/err.h>
#include <linux/fcntl.h>

#define SCRATCH "/var/tmp/xidsem"

static void show(const char *label, const char *path, unsigned int flags)
{
	struct path p;
	struct dentry *d;

	memset(&p, 0, sizeof(p));
	d = start_creating_path(AT_FDCWD, path, &p, flags);
	if (IS_ERR_OR_NULL(d)) {
		pr_err("XIDSEM %s flags=0x%08x path=%s -> err=%ld%s\n",
		       label, flags, path, PTR_ERR(d),
		       (PTR_ERR(d) == -EEXIST) ? " (EEXIST: exists)"
		       : (PTR_ERR(d) == -ENOENT) ? " (ENOENT: not found, no LOOKUP_CREATE)"
		       : "");
	} else {
		pr_err("XIDSEM %s flags=0x%08x path=%s -> OK dentry=%pd (dir=%d)\n",
		       label, flags, path, d, d_is_dir(d));
		end_creating_path(&p, d);
	}
}

static int __init xidsem_init(void)
{
	/* Ensure the scratch parent exists (create via kernel API with CREATE). */
	show("mk-parent", SCRATCH, LOOKUP_DIRECTORY | LOOKUP_CREATE);

	/* Case A: target PRESENT (we just made SCRATCH), no LOOKUP_CREATE. */
	show("A-present-noCREATE", SCRATCH, LOOKUP_DIRECTORY);

	/* Case B: target ABSENT, parent present, LOOKUP_DIRECTORY only (driver's
	 * exact flags).  This is the decisive case. */
	show("B-absent-noCREATE", SCRATCH "/newdir_b", LOOKUP_DIRECTORY);

	/* Case C: target ABSENT, parent present, with LOOKUP_CREATE. */
	show("C-absent-CREATE", SCRATCH "/newdir_c", LOOKUP_DIRECTORY | LOOKUP_CREATE);

	/* Re-check what actually got created on disk. */
	{
		struct path p;
		memset(&p, 0, sizeof(p));
		if (!kern_path(SCRATCH "/newdir_b", 0, &p)) {
			pr_err("XIDSEM RESULT newdir_b EXISTS after B\n");
			path_put(&p);
		} else {
			pr_err("XIDSEM RESULT newdir_b ABSENT after B (driver's call cannot create)\n");
		}
		memset(&p, 0, sizeof(p));
		if (!kern_path(SCRATCH "/newdir_c", 0, &p)) {
			pr_err("XIDSEM RESULT newdir_c EXISTS after C\n");
			path_put(&p);
		} else {
			pr_err("XIDSEM RESULT newdir_c ABSENT after C\n");
		}
	}
	return 0;
}
module_init(xidsem_init);

static void __exit xidsem_exit(void)
{
	pr_err("XIDSEM exit\n");
}
module_exit(xidsem_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Decisive probe of start_creating_path() create semantics for mtgpu XID logging");

// SPDX-License-Identifier: MIT
/*
 * xidprobe - read-only probe of the kernel path-creation API that the
 * Moore Threads mtgpu driver uses for its event-report / XID record files.
 *
 * The vendor core is an opaque pre-built object (objs/x86_64/
 * mtgpu_core.o_binary), so the exact behaviour of mtgpu_drv_mkdir() and
 * event_report_create_fd() was recovered from the linked module's
 * disassembly.  This module lets the same API be exercised on the running
 * kernel so the recovered behaviour can be confirmed with real return
 * values instead of inference.
 *
 * It only performs path lookups.  It creates, modifies and removes nothing.
 *
 *   make
 *   pkexec insmod ./xidprobe.ko
 *   dmesg | grep XIDPROBE2
 *   pkexec rmmod xidprobe
 */
#include <linux/module.h>
#include <linux/namei.h>
#include <linux/fs.h>
#include <linux/err.h>
#include <linux/fcntl.h>

/* The directories the vendor driver creates at module initialisation. */
static const char *paths[] = {
	"/var/log/mtgpu/",
	"/var/log/mtgpu",
	"/var/log/mtgpu/event_report",
	"/var/log/mtgpu/event_report/0b336cab-1977-ccbe-4e9a-a18ef71961cc",
	"/var/log/mtgpu/event_report/0b336cab-1977-ccbe-4e9a-a18ef71961cc/xid_records",
	"/nonexistent-parent-xyz/foo",
};

/*
 * LOOKUP_DIRECTORY is the only flag the vendor driver passes
 * (os_value index 232 in the driver's os_value[] table).
 */
static void probe_flags(const char *label, unsigned int flags)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(paths); i++) {
		struct path p;
		struct dentry *d;

		memset(&p, 0, sizeof(p));
		d = start_creating_path(AT_FDCWD, paths[i], &p, flags);
		pr_err("XIDPROBE2 %s flags=0x%08x path=%-72s -> err=%ld\n",
		       label, flags, paths[i],
		       IS_ERR_OR_NULL(d) ? PTR_ERR(d) : 0L);
		if (!IS_ERR_OR_NULL(d))
			end_creating_path(&p, d);
	}
}

static int __init xidprobe_init(void)
{
	pr_err("XIDPROBE2 init\n");
	probe_flags("driver-flags", LOOKUP_DIRECTORY);
	probe_flags("with-LOOKUP_CREATE", LOOKUP_DIRECTORY | LOOKUP_CREATE);
	return 0;
}
module_init(xidprobe_init);

static void __exit xidprobe_exit(void)
{
	pr_err("XIDPROBE2 exit\n");
}
module_exit(xidprobe_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Read-only probe of start_creating_path() semantics for mtgpu XID logging");

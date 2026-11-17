/*
 * hw-probe.c — read-only MTT S30 hardware/driver evidence probe.
 *
 * This tool gathers facts about the MTT S30 without treating PCI binding or
 * a device node as proof of GPU functionality. It reads sysfs, module state,
 * and device-node metadata only. It deliberately does not open the device or
 * send an unvalidated ioctl.
 *
 * Exit code is always 0; interpretation is done by the caller/scripts.
 *
 * Build:  scripts/verify-musa.sh compiles this into a temporary directory.
 */
#define _GNU_SOURCE
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <unistd.h>

#define MTT_VENDOR 0x1ed5
#define MTT_S30_DEVICE 0x0102

static int read_sysfs_str(const char *path, char *buf, size_t sz)
{
    FILE *f = fopen(path, "r");
    if (!f)
        return -1;
    if (!fgets(buf, (int)sz, f)) {
        fclose(f);
        return -1;
    }
    fclose(f);
    buf[strcspn(buf, "\n")] = '\0';
    return 0;
}

static int pci_devices_found(void)
{
    static const char *base = "/sys/bus/pci/devices";
    struct dirent *de;
    int count = 0;
    DIR *d = opendir(base);

    if (!d) {
        printf("PCI_SCAN_ERROR: cannot open %s (%s)\n", base, strerror(errno));
        return 0;
    }
    while ((de = readdir(d)) != NULL) {
        char vendor[64] = {0}, device[64] = {0};
        char path[512];

        if (de->d_name[0] == '.')
            continue;
        snprintf(path, sizeof(path), "%s/%s/vendor", base, de->d_name);
        if (read_sysfs_str(path, vendor, sizeof(vendor)) != 0)
            continue;
        snprintf(path, sizeof(path), "%s/%s/device", base, de->d_name);
        if (read_sysfs_str(path, device, sizeof(device)) != 0)
            continue;
        if (strtoul(vendor, NULL, 0) == MTT_VENDOR &&
            strtoul(device, NULL, 0) == MTT_S30_DEVICE) {
            printf("PCI_DEVICE: %s vendor=%s device=%s\n", de->d_name, vendor, device);
            count++;

            snprintf(path, sizeof(path), "%s/%s/driver", base, de->d_name);
            char link[512];
            ssize_t n = readlink(path, link, sizeof(link) - 1);
            if (n < 0) {
                printf("PCI_DRIVER: %s unbound (no driver symlink)\n", de->d_name);
            } else {
                char resolved[512];
                if (realpath(path, resolved))
                    printf("PCI_DRIVER: %s bound to name=\"%s\" (sysfs path=%s)\n",
                           de->d_name, strrchr(resolved, '/') ? strrchr(resolved, '/') + 1 : resolved,
                           resolved);
                else
                    printf("PCI_DRIVER: %s bound link=\"%.*s\"\n", de->d_name, (int)n, link);
            }
        }
    }
    closedir(d);
    return count;
}

static int report_module(const char *name)
{
    char path[256], state[64];

    snprintf(path, sizeof(path), "/sys/module/%s", name);
    if (access(path, F_OK) != 0) {
        printf("MODULE: %-12s not loaded\n", name);
        return 0;
    }
    printf("MODULE: %-12s loaded\n", name);

    snprintf(path, sizeof(path), "/sys/module/%s/initstate", name);
    if (read_sysfs_str(path, state, sizeof(state)) == 0) {
        printf("MODULE_INITSTATE: %s=%s\n", name, state);
        if (strcmp(state, "live") != 0) {
            printf("MODULE_WARNING: %s initialization is incomplete or unloading\n",
                   name);
            return 1;
        }
    }

    snprintf(path, sizeof(path), "/sys/bus/pci/drivers/%s", name);
    if (access(path, F_OK) == 0)
        printf("MODULE: %s registered a PCI driver directory\n", name);
    return 0;
}

static void vendor_module_availability(void)
{
    struct utsname uts;
    char path[512];

    if (uname(&uts) != 0) {
        printf("RUNNING_KERNEL: unavailable (%s)\n", strerror(errno));
        return;
    }
    printf("RUNNING_KERNEL: %s\n", uts.release);

    static const char *names[] = {"sgpu_km", "sgpu_xx", "mtgpu", "mtgpu_stub", NULL};
    for (int i = 0; names[i]; i++) {
        snprintf(path, sizeof(path), "/lib/modules/%s/extra/%s.ko", uts.release, names[i]);
        if (access(path, F_OK) == 0)
            printf("VENDOR_MODULE: %s AVAILABLE for running kernel at %s\n", names[i], path);
        else
            printf("VENDOR_MODULE: %s absent for running kernel (%s)\n", names[i], path);
    }
    printf("VENDOR_MODULE_SCAN: use tests/report-module-trees.sh to list other kernel trees\n");
}

static void report_device_node(void)
{
    const char *node = "/dev/mtgpu.0";
    struct stat st;

    if (stat(node, &st) != 0) {
        printf("DEVNODE: %s does not exist\n", node);
        return;
    }
    printf("DEVNODE: %s exists major=%u minor=%u mode=%04o owner=%u:%u\n", node,
           (unsigned)(st.st_rdev >> 8), (unsigned)(st.st_rdev & 0xff),
           (unsigned)(st.st_mode & 07777), (unsigned)st.st_uid, (unsigned)st.st_gid);
    printf("DEVNODE_IO: NOT ATTEMPTED (safe inventory does not open the node or issue ioctls)\n");
}

int main(void)
{
    int module_not_live = 0;

    printf("=== MTT S30 hardware evidence probe (read-only) ===\n");
    int n = pci_devices_found();
    if (n == 0)
        printf("PCI_SCAN: no MTT S30 (1ed5:0102) function found\n");
    else
        printf("PCI_SCAN: %d matching function(s)\n", n);

    module_not_live |= report_module("mtgpu");
    module_not_live |= report_module("mtgpu_bind");
    module_not_live |= report_module("mtgpu_stub");
    module_not_live |= report_module("sgpu_km");
    module_not_live |= report_module("sgpu_xx");

    vendor_module_availability();
    report_device_node();
    if (module_not_live)
        printf("GPU_ACCESS_STATUS: BLOCKED (a driver module is not fully live)\n");

    printf("=== end probe; no system state was modified ===\n");
    return 0;
}

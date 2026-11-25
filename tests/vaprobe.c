/* vaprobe.c — user-space VA-API capability probe for the MTT S30.
 *
 * libva is dlopen()ed so the probe builds without libva-devel and links no
 * library at compile time. Read-only with respect to system configuration:
 * it opens the mtgpu DRM render node, initialises a VA display and dumps the
 * profiles/entrypoints the driver advertises, then tears the display down.
 *
 * It does not allocate GPU surfaces, so a passing probe proves the driver
 * loads and negotiates with libva — not that a particular codec can decode.
 * scripts/hardware/test-vaapi.sh adds the encode/decode round trip.
 *
 * Usage: vaprobe [/dev/dri/renderDNN]
 *        LIBVA_DRIVERS_PATH must point at the directory holding the driver.
 * Exit:  0 at least one profile, 1 none, 2 initialisation failure.
 */
#define _GNU_SOURCE
#include <dlfcn.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

typedef void *VADisplay;
typedef int VAStatus;
#define VA_STATUS_SUCCESS 0

typedef VADisplay (*vaGetDisplayDRM_t)(int fd);
typedef VAStatus (*vaInitialize_t)(VADisplay, int *, int *);
typedef VAStatus (*vaQueryConfigProfiles_t)(VADisplay, int *, int *);
typedef VAStatus (*vaQueryConfigEntrypoints_t)(VADisplay, int, int *, int *);
typedef VAStatus (*vaTerminate_t)(VADisplay);
typedef const char *(*vaErrorStr_t)(VAStatus);
typedef const char *(*vaQueryVendorString_t)(VADisplay);

/* Names for the profile ids this libva release knows about. Unknown ids are
 * printed numerically instead of being silently dropped. */
static const char *profile_name(int id)
{
    switch (id) {
    case -1: return "VAProfileNone";
    case 0:  return "VAProfileMPEG2Simple";
    case 1:  return "VAProfileMPEG2Main";
    case 2:  return "VAProfileH264Baseline";
    case 3:  return "VAProfileH264Main";
    case 4:  return "VAProfileH264High";
    case 5:  return "VAProfileH264ConstrainedBaseline";
    case 6:  return "VAProfileVC1Simple";
    case 7:  return "VAProfileVC1Main";
    case 8:  return "VAProfileVC1Advanced";
    case 9:  return "VAProfileJPEGBaseline";
    case 10: return "VAProfileVP8Version0_3";
    case 11: return "VAProfileH264MultiviewHigh";
    case 12: return "VAProfileH264StereoHigh";
    case 13: return "VAProfileHEVCMain12";
    case 14: return "VAProfileVP9Profile0";
    case 15: return "VAProfileVP9Profile1";
    case 16: return "VAProfileVP9Profile2";
    case 17: return "VAProfileVP9Profile3";
    case 18: return "VAProfileHEVCMain422_10";
    case 19: return "VAProfileHEVCMain444";
    case 21: return "VAProfileHEVCMain444_12";
    case 32: return "VAProfileAV1Profile0";
    case 33: return "VAProfileAV1Profile1";
    default: return "unknown";
    }
}

static const char *entrypoint_name(int id)
{
    switch (id) {
    case 1:  return "VAEntrypointVLD";
    case 2:  return "VAEntrypointIZZ";
    case 3:  return "VAEntrypointIDCT";
    case 4:  return "VAEntrypointMoComp";
    case 5:  return "VAEntrypointDeblocking";
    case 6:  return "VAEntrypointEncSlice";
    case 7:  return "VAEntrypointEncPicture";
    case 8:  return "VAEntrypointEncSliceLP";
    case 10: return "VAEntrypointVideoProc";
    case 11: return "VAEntrypointStats";
    default: return "VAEntrypoint?";
    }
}

int main(int argc, char **argv)
{
    void *h = dlopen("libva.so.2", RTLD_NOW);
    if (!h) { printf("PROBE_FAIL dlopen libva.so.2: %s\n", dlerror()); return 2; }
    /* vaGetDisplay* live in libva-drm, not libva. */
    void *hd = dlopen("libva-drm.so.2", RTLD_NOW);
    if (!hd) { printf("PROBE_FAIL dlopen libva-drm.so.2: %s\n", dlerror()); return 2; }

    vaGetDisplayDRM_t get_display = (vaGetDisplayDRM_t)dlsym(hd, "vaGetDisplayDRM");
    vaInitialize_t init = (vaInitialize_t)dlsym(h, "vaInitialize");
    vaQueryConfigProfiles_t profiles = (vaQueryConfigProfiles_t)dlsym(h, "vaQueryConfigProfiles");
    vaQueryConfigEntrypoints_t eps = (vaQueryConfigEntrypoints_t)dlsym(h, "vaQueryConfigEntrypoints");
    vaTerminate_t term = (vaTerminate_t)dlsym(h, "vaTerminate");
    vaErrorStr_t errstr = (vaErrorStr_t)dlsym(h, "vaErrorStr");
    vaQueryVendorString_t vendor = (vaQueryVendorString_t)dlsym(h, "vaQueryVendorString");
    if (!get_display || !init || !profiles || !term) {
        printf("PROBE_FAIL libva is missing the expected entry points\n");
        return 2;
    }

    const char *path = argc > 1 ? argv[1] : "/dev/dri/renderD129";
    int fd = open(path, O_RDWR | O_CLOEXEC);
    if (fd < 0) { perror(path); return 2; }

    VADisplay dpy = get_display(fd);
    if (!dpy) { printf("PROBE_FAIL vaGetDisplayDRM returned NULL for %s\n", path); return 2; }

    int major = -1, minor = -1;
    VAStatus st = init(dpy, &major, &minor);
    if (st != VA_STATUS_SUCCESS) {
        printf("PROBE_FAIL vaInitialize -> %d (%s)\n", st, errstr ? errstr(st) : "?");
        close(fd);
        return 2;
    }
    printf("PROBE_OK vaInitialize %d.%d on %s\n", major, minor, path);
    if (vendor) printf("VENDOR: %s\n", vendor(dpy));

    int n = 0, list[64];
    st = profiles(dpy, list, &n);
    if (st != VA_STATUS_SUCCESS) {
        printf("PROBE_FAIL vaQueryConfigProfiles -> %d\n", st);
        n = 0;
    }
    printf("PROFILES: %d\n", n);
    for (int i = 0; i < n; i++) {
        printf("  profile %-3d %s\n", list[i], profile_name(list[i]));
        if (!eps) continue;
        int en = 0, elist[8];
        if (eps(dpy, list[i], elist, &en) == VA_STATUS_SUCCESS)
            for (int j = 0; j < en; j++)
                printf("    entrypoint %-2d %s\n", elist[j], entrypoint_name(elist[j]));
    }

    term(dpy);
    close(fd);
    dlclose(hd);
    dlclose(h);
    return n > 0 ? 0 : 1;
}

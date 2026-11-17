/*
 * dri-screens-fix.c — LD_PRELOAD fix for libmusa 1.5 screen enumeration.
 *
 * The MUSA 3.1.0 runtime (libmusa.so.1.5, M3d::Drm::Device::GetScreens)
 * calls DRM mode-set ioctls on an internally cached "screen fd" that is
 * never opened on a headless S30 setup, producing DRM_IOCTL_MODE_GETRESOURCES
 * on fd=-1 (EBADF) and musaErrorNotSupported(801) from every runtime API.
 *
 * This interposer opens the card node belonging to the mtgpu DRM driver
 * (looked up via /sys/class/drm) and substitutes it whenever libmusa issues
 * a mode-set call with an invalid fd. Display enumeration then simply finds
 * zero connected screens and the compute path proceeds.
 *
 * Build:  gcc -shared -fPIC -O2 -o build/dri-screens-fix.so tests/dri-screens-fix.c -ldl
 * Run:    LD_PRELOAD=build/dri-screens-fix.so <musa program>
 */
#define _GNU_SOURCE
#include <dirent.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>

static int card_fd = -2; /* -2 = not searched yet */

static int try_open_card(void)
{
	static const char *base = "/sys/class/drm";
	DIR *d = opendir(base);
	struct dirent *de;

	if (!d)
		return -1;
	while ((de = readdir(d)) != NULL) {
		char link[512], resolved[512], buf[256];
		char path[600], drv[600];
		ssize_t n;
		char *slash;

		if (strncmp(de->d_name, "card", 4) != 0)
			continue;
		snprintf(path, sizeof(path), "%s/%s/device/driver", base,
			 de->d_name);
		n = readlink(path, link, sizeof(link) - 1);
		if (n < 0)
			continue;
		link[n] = '\0';
		/* driver symlink target ends with /mtgpu */
		slash = strrchr(link, '/');
		if (!slash || strcmp(slash + 1, "mtgpu") != 0)
			continue;
		snprintf(drv, sizeof(drv), "/dev/dri/%s", de->d_name);
		int fd = open(drv, O_RDWR | O_CLOEXEC);
		if (fd < 0)
			continue;
		/* confirm via DRM_IOCTL_VERSION that this is the mtgpu node */
		struct {
			int major, minor, patch;
			int name_len;
			char *name;
			int date_len;
			char *date;
			int desc_len;
			char *desc;
		} ver;
		char namebuf[16];
		memset(&ver, 0, sizeof(ver));
		ver.name_len = sizeof(namebuf) - 1;
		ver.name = namebuf;
		if (ioctl(fd, 0xc0406400UL /* DRM_IOCTL_VERSION */, &ver) == 0 &&
		    ver.name_len == 5 && strncmp(namebuf, "mtgpu", 5) == 0) {
			closedir(d);
			fprintf(stderr, "[dri-screens-fix] using %s (fd=%d)\n",
				drv, fd);
			return fd;
		}
		(void)resolved; (void)buf;
		close(fd);
	}
	closedir(d);
	return -1;
}

static int screen_fd(void)
{
	if (card_fd == -2)
		card_fd = try_open_card();
	return card_fd;
}

#define WRAP1(ret, name, t1)                                                \
	typedef ret (*name##_fn)(t1);                                       \
	ret name(t1 a)                                                      \
	{                                                                   \
		static name##_fn real;                                      \
		if (!real) real = (name##_fn)dlsym(RTLD_NEXT, #name);       \
		return real(a < 0 ? screen_fd() : a);                       \
	}

WRAP1(void *, drmModeGetResources, int)
WRAP1(void *, drmModeGetPlaneResources, int)

/* Some mode APIs take (fd, id). Interpose the common two-arg forms. */
typedef void *(*gr2_fn)(int, unsigned int);
static void *wrap2(const char *sym, int fd, unsigned int id)
{
	static void (*unused)(void);
	gr2_fn real = (gr2_fn)dlsym(RTLD_NEXT, sym);
	if (!real)
		return NULL;
	return real(fd < 0 ? screen_fd() : fd, id);
}

void *drmModeGetConnector(int fd, unsigned int id)
{ return wrap2("drmModeGetConnector", fd, id); }
void *drmModeGetEncoder(int fd, unsigned int id)
{ return wrap2("drmModeGetEncoder", fd, id); }
void *drmModeGetCrtc(int fd, unsigned int id)
{ return wrap2("drmModeGetCrtc", fd, id); }
void *drmModeGetObjectProperties(int fd, unsigned int id)
{ return wrap2("drmModeGetObjectProperties", fd, id); }

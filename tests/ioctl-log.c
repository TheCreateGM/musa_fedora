/*
 * ioctl-log.c — LD_PRELOAD shim that logs DRM mtgpu ioctl payloads to stderr.
 *
 * Logs _IOC direction/type/nr/size and the first bytes of the argument so
 * failing MTGPU API exchanges can be diagnosed without a kernel debugger.
 *
 * Build:  gcc -shared -fPIC -o build/ioctl-log.so tests/ioctl-log.c -ldl
 * Run:    LD_PRELOAD=build/ioctl-log.so ./build/musart-smoke-31
 */
#define _GNU_SOURCE
#include <dlfcn.h>
#include <execinfo.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

static int (*real_ioctl)(int, unsigned long, ...);

int ioctl(int fd, unsigned long req, ...)
{
	va_list ap;
	void *arg;
	long rc;

	if (!real_ioctl)
		real_ioctl = dlsym(RTLD_NEXT, "ioctl");

	va_start(ap, req);
	arg = va_arg(ap, void *);
	va_end(ap);

	rc = real_ioctl(fd, req, arg);

	if (((req >> 8) & 0xff) == 0x64 /* DRM_IOCTL_BASE */) {
		unsigned int dir = (req >> 30) & 3;
		unsigned int size = (req >> 16) & 0x3fff;
		fprintf(stderr, "[ioctl] fd=%d nr=0x%02lx size=%u dir=%u rc=%ld",
			fd, req & 0xff, size, dir, rc);
		if (arg && rc >= 0) {
			unsigned char b[24];
			size_t n = size < sizeof(b) ? size : sizeof(b);
			for (size_t i = 0; i < n; i++)
				b[i] = ((unsigned char *)arg)[i];
			fprintf(stderr, " data=");
			for (size_t i = 0; i < n; i++)
				fprintf(stderr, "%02x", b[i]);
		}
		if (fd < 0) {
			void *bt[32];
			int n = backtrace(bt, 32);
			fputs("[ioctl] BAD FD backtrace:\n", stderr);
			backtrace_symbols_fd(bt, n, STDERR_FILENO);
		}
		fputc('\n', stderr);
	}
	return (int)rc;
}

/* --- openat logging for DRM nodes --- */
#include <fcntl.h>
#include <string.h>
static int (*real_openat)(int, const char *, int, ...);
int openat(int dirfd, const char *path, int flags, ...)
{
	mode_t mode = 0;
	va_list ap;
	if (!real_openat)
		real_openat = dlsym(RTLD_NEXT, "openat");
	va_start(ap, flags);
	mode = va_arg(ap, int);
	va_end(ap);
	int fd = real_openat(dirfd, path, flags, mode);
	if (path && strstr(path, "dri"))
		fprintf(stderr, "[openat] %s flags=%#x -> fd=%d\n", path, flags, fd);
	return fd;
}

#define LOG_OPEN(fn)                                                       \
	static int (*real_##fn)(const char *, int, ...);                   \
	int fn(const char *path, int flags, ...)                           \
	{                                                                  \
		mode_t m = 0;                                              \
		va_list ap;                                                \
		if (!real_##fn) real_##fn = dlsym(RTLD_NEXT, #fn);         \
		va_start(ap, flags); m = va_arg(ap, int); va_end(ap);      \
		int fd = real_##fn(path, flags, m);                        \
		if (path && strstr(path, "dri"))                           \
			fprintf(stderr, "[" #fn "] %s flags=%#x -> %d\n",  \
				path, flags, fd);                          \
		return fd;                                                 \
	}
LOG_OPEN(open)
LOG_OPEN(open64)

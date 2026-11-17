/*
 * @Copyright   Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License     Dual MIT/GPLv2
 */

#ifndef _MTVPU_DRM_H_
#define _MTVPU_DRM_H_

#if defined(__KERNEL__)
#include "linux-types.h"
#endif

struct drm_device;
struct drm_file;

#ifdef __ASSEMBLY__
#define _AC(X,Y)        X
#define _AT(T,X)        X
#else
#define __AC(X,Y)       (X##Y)
#define _AC(X,Y)        __AC(X,Y)
#define _AT(T,X)        ((T)(X))
#endif

#ifndef _UL
#define _UL(x)          (_AC(x, UL))
#define UL(x)           (_UL(x))
#ifndef BIT
#define BIT(nr)         (UL(1) << (nr))
#endif // BIT
#endif

#ifndef _IOC_NONE
#define _IOC_NONE  0U
#define _IOC_WRITE 1U
#define _IOC_READ  2U

#define _IOC_DIRBITS  2
#define _IOC_NRBITS   8
#define _IOC_TYPEBITS 8
#define _IOC_SIZEBITS 14

#define _IOC_NRMASK  ((1 << _IOC_NRBITS) - 1)
#define _IOC_NRSHIFT 0
#define _IOC_NR(nr)  (((nr) >> _IOC_NRSHIFT) & _IOC_NRMASK)

#define _IOC_TYPESHIFT    (_IOC_NRSHIFT + _IOC_NRBITS)
#define _IOC_SIZESHIFT    (_IOC_TYPESHIFT + _IOC_TYPEBITS)
#define _IOC_DIRSHIFT     (_IOC_SIZESHIFT + _IOC_SIZEBITS)
#define _IOC_TYPECHECK(t) (sizeof(t))

#define _IOC(dir, type, nr, size) \
    (((dir) << _IOC_DIRSHIFT) | ((type) << _IOC_TYPESHIFT) | ((nr) << _IOC_NRSHIFT) | ((size) << _IOC_SIZESHIFT))

#define __IO(type, nr)        _IOC(_IOC_NONE, (type), (nr), 0)
#define _IOR(type, nr, size)  _IOC(_IOC_READ, (type), (nr), (_IOC_TYPECHECK(size)))
#define _IOW(type, nr, size)  _IOC(_IOC_WRITE, (type), (nr), (_IOC_TYPECHECK(size)))
#define _IOWR(type, nr, size) _IOC(_IOC_READ | _IOC_WRITE, (type), (nr), (_IOC_TYPECHECK(size)))

#endif

#ifndef DRM_IOCTL_NR
#define DRM_IOCTL_NR(n)    _IOC_NR(n)
#endif

#ifndef DRM_IOCTL_BASE
#define DRM_IOCTL_BASE    'd'
#endif

#ifndef DRM_MAJOR
#define DRM_MAJOR          226
#endif

#ifndef DRM_IOWR
#define DRM_IOWR(nr, type) _IOWR(DRM_IOCTL_BASE, nr, type)
#endif

#ifndef DRM_IO
#define DRM_IO(nr)        __IO(DRM_IOCTL_BASE, nr)
#endif

#ifndef DRM_IOR
#define DRM_IOR(nr, type) _IOR(DRM_IOCTL_BASE, nr, type)
#endif

#ifndef DRM_COMMAND_BASE
#define DRM_COMMAND_BASE 0x40
#endif

#ifndef DRM_COMMAND_END
#define DRM_COMMAND_END  0xA0
#endif

#ifndef DRM_IOCTL_DEF_DRV
#define DRM_IOCTL_DEF_DRV(ioctl, _func, _flags) \
	[DRM_IOCTL_NR(DRM_IOCTL_##ioctl) - DRM_COMMAND_BASE] = { \
		.cmd = DRM_IOCTL_##ioctl, \
		.func = _func, \
		.flags = _flags, \
		.name = #ioctl \
	}
#endif

#ifndef _DRM_IOCTL_H_
enum drm_ioctl_flags {
    DRM_AUTH        = BIT(0),
    DRM_MASTER      = BIT(1),
    DRM_ROOT_ONLY       = BIT(2),
    DRM_UNLOCKED        = BIT(4),
    DRM_RENDER_ALLOW    = BIT(5),
};

typedef int drm_ioctl_t(struct drm_device *dev, void *data, struct drm_file *file_priv);

struct drm_ioctl_desc {
    unsigned int cmd;
    enum drm_ioctl_flags flags;
    drm_ioctl_t *func;
    const char *name;
};
#endif

#ifndef _DRM_MODE_H
struct drm_mode_create_dumb {
    __u32 height;
    __u32 width;
    __u32 bpp;
    __u32 flags;
    /* handle, pitch, size will be returned */
    __u32 handle;
    __u32 pitch;
    __u64 size;
};

/* set up for mmap of a dumb scanout buffer */
struct drm_mode_map_dumb {
    /** Handle for the object being mapped. */
    __u32 handle;
    __u32 pad;
    /**
     * Fake offset to use for subsequent mmap call
     *
     * This is a fixed-size type for 32/64 compatibility.
     */
    __u64 offset;
};

struct drm_mode_destroy_dumb {
    __u32 handle;
};
#endif

#endif //_MTVPU_DRM_H_
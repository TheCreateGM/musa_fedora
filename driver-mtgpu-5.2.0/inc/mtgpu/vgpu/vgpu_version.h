/*
 * @Copyright   Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License     Dual MIT/GPLv2
 * @Descript    This file shared with host/guest/fw
 */

#ifndef __VGPU_VERSION_H__
#define __VGPU_VERSION_H__

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * For example version: "1.2.3[-001] <--> major.minor.patch[-update]"
 * 
 * @note field update is optional and default is 0.
 */
struct vgpu_release_version {
	uint64_t major : 16;
	uint64_t minor : 16;
	uint64_t patch : 16;
	uint64_t update : 8;

	uint64_t pass : 1;
	uint64_t __reserved : 7;
};
typedef struct vgpu_release_version vgpu_release_version_t;

union vgpu_compat_check_cmd {
	uint64_t cmd;
	vgpu_release_version_t ver;
};
typedef union vgpu_compat_check_cmd vgpu_compat_check_cmd_t;

#define VGPU_RELEAE_VER_EQ(v1, v2) \
	(((v1).major == (v2).major) && \
	((v1).minor == (v2).minor) && \
	((v1).patch == (v2).patch) && \
	((v1).update == (v2).update))

#define VGPU_RELEAE_VER_GT(v1, v2) \
	(((v1).major > (v2).major) || \
	((v1).major == (v2).major && (v1).minor > (v2).minor) || \
	((v1).major == (v2).major && (v1).minor == (v2).minor && (v1).patch > (v2).patch) || \
	((v1).major == (v2).major && (v1).minor == (v2).minor && (v1).patch == (v2).patch && (v1).update > (v2).update))

#ifndef VGPU_RELEASE_VERSION_MAJOR
#define VGPU_RELEASE_VERSION_MAJOR 1
#define VGPU_RELEASE_VERSION_MINOR 2
#define VGPU_RELEASE_VERSION_PATCH 3
#define VGPU_RELEASE_VERSION_UPDATE 4
#endif

#define VGPU_RELEASE_VER_INIT() \
	{ VGPU_RELEASE_VERSION_MAJOR, VGPU_RELEASE_VERSION_MINOR, \
	  VGPU_RELEASE_VERSION_PATCH, VGPU_RELEASE_VERSION_UPDATE }

#ifdef VGPU_RELEASE_VERSION_DFT
#define VGPU_RELEASE_VERSION_LIST_DUMMY() \
	{ 2, 4, 1, 0 },	\
	{ 2, 4, 1, 1 },	\
	{ 2, 4, 1, 2 },	\
	{ 2, 4, 2, 0 },	\
	{ 2, 4, 2, 1 },	\
			\
	{ 2, 5, 0, 0 },	\
	{ 2, 5, 0, 1 },	\
	{ 2, 5, 1, 0 },	\
	{ 2, 5, 2, 0 },	\
			\
	{ 2, 6, 0, 0 },	\
	{ 2, 6, 1, 0 }, \
	{ 2, 6, 5, 0 }, \
	{ 2, 6, 5, 1 }, \
	{ 2, 6, 6, 0 }, \
	{ 2, 8, 0, 0 }, \
	{ 3, 8, 0, 0 },
#endif

#ifdef __cplusplus
}
#endif

#endif /* __VGPU_VERSION_H__ */

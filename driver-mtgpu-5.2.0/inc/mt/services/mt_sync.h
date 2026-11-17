/*
 * @File        mt_sync.h
 * @Title       Kernel driver for Android's sync mechanism
 * @Codingstyle LinuxKernel
 * @Copyright   Copyright (c) Imagination Technologies Ltd. All Rights Reserved
 * @License     Dual MIT/GPLv2
 *
 * The contents of this file are subject to the MIT license as set out below.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * Alternatively, the contents of this file may be used under the terms of
 * the GNU General Public License Version 2 ("GPL") in which case the provisions
 * of GPL are applicable instead of those above.
 *
 * If you wish to allow use of your version of this file only under the terms of
 * GPL, and not to allow others to use your version of this file under the terms
 * of the MIT license, indicate your decision by deleting the provisions above
 * and replace them with the notice and other provisions required by GPL as set
 * out in the file called "GPL-COPYING" included in this distribution. If you do
 * not delete the provisions above, a recipient may use your version of this file
 * under the terms of either the MIT license or GPL.
 *
 * This License is also included in this distribution in the file called
 * "MIT-COPYING".
 *
 * EXCEPT AS OTHERWISE STATED IN A NEGOTIATED AGREEMENT: (A) THE SOFTWARE IS
 * PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 * BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 * PURPOSE AND NONINFRINGEMENT; AND (B) IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef _MT_SYNC_H
#define _MT_SYNC_H

#include "mt_fd_sync_kernel.h"
#include "services_kernel_client.h"

struct device;

/* Services internal interface */

/**
 * mt_sync_register_functions()
 *
 * Return: MTGPU_OK on success.
 */
enum MTGPU_ERROR_TAG mt_sync_register_functions(void);

/**
 *  mt_sync_init - register the mt_sync misc device
 *
 *  Return: error code, 0 on success.
 */
int mt_sync_init(void);

/**
 * mt_sync_deinit - unregister the mt_sync misc device
 */
void mt_sync_deinit(void);

/**
 * mt_sync_device_init() - create an internal sync context
 * @dev: Linux device
 *
 * Return: MTGPU_OK on success.
 */
enum MTGPU_ERROR_TAG mt_sync_device_init(struct device *dev);

/**
 * mt_sync_device_deinit() - destroy an internal sync context
 *
 * Drains any work items with outstanding sync fence updates/dependencies.
 */
void mt_sync_device_deinit(struct device *dev);

enum MTGPU_ERROR_TAG mt_sync_fence_wait(void *fence, u32 timeout_in_ms);

enum MTGPU_ERROR_TAG mt_sync_fence_release(void *fence);

enum MTGPU_ERROR_TAG mt_sync_fence_get(int fence_fd, void **fence_out);

enum MTGPU_ERROR_TAG
mt_sync_sw_timeline_fence_create(struct _MTGPU_DEVICE_NODE_ *mtgpu_dev_node,
				  int timeline_fd,
				  const char *fence_name,
				  int *fence_fd_out,
				  u64 *sync_pt_idx);

enum MTGPU_ERROR_TAG mt_sync_sw_timeline_advance(void *timeline,
					       u64 *sync_pt_idx);

enum MTGPU_ERROR_TAG mt_sync_sw_timeline_release(void *timeline);

enum MTGPU_ERROR_TAG mt_sync_sw_timeline_get(int timeline_fd,
					   void **timeline_out);

enum MTGPU_ERROR_TAG
sync_dump_fence(void *sw_fence_obj,
		DUMPDEBUG_PRINTF_FUNC *dump_debug_printf,
		void *dump_debug_file);

enum MTGPU_ERROR_TAG
sync_sw_dump_timeline(void *sw_timeline_obj,
		      DUMPDEBUG_PRINTF_FUNC *dump_debug_printf,
		      void *dump_debug_file);

#endif /* _MT_SYNC_H */

/*************************************************************************/ /*!
@File
@Title          Services external synchronisation interface header
@Description    Defines synchronisation structures that are visible internally
                and externally
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@License        MIT

The contents of this file are subject to the MIT license as set out below.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/ /**************************************************************************/

#ifndef MT_SYNC_EXT_H
#define MT_SYNC_EXT_H

#if defined(__cplusplus)
extern "C" {
#endif

/*!
 * Number of sync prims still used internally in operations
 */
#define MTGPU_MAX_SYNC_PRIMS 4U

/*!
 * Maximum number of dev var updates passed in a kick call
 */
#define MTGPU_MAX_DEV_VARS 13U

/*!
 * Number of UFOs in operations
 */
#define	MTGPU_MAX_SYNCS (MTGPU_MAX_SYNC_PRIMS + MTGPU_MAX_DEV_VARS)

/*! Implementation independent types for passing fence/timeline to Services.
 */
typedef int32_t MTGPU_FENCE;
typedef int32_t MTGPU_TIMELINE;

/*! Maximum length for an annotation name string for fence sync model objects.
 */
#define MTGPU_SYNC_NAME_LENGTH 32U

/* Macros for API callers using the fence sync model
 */
#define MTGPU_NO_TIMELINE      ((MTGPU_TIMELINE) -1)
#define MTGPU_NO_FENCE         ((MTGPU_FENCE)    -1)
#define MTGPU_NO_FENCE_PTR     NULL
#define MTGPU_NO_TIMELINE_PTR  NULL

#if defined(__cplusplus)
}
#endif

#endif

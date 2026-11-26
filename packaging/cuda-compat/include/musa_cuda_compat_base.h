/* CUDA source-compatibility base for the MUSA 1.5.4 runtime.
 *
 * The MUSA compiler force-includes its own runtime wrapper and then #undefs
 * __CUDACC__, so CUDA source that guards device code with `#ifdef __CUDACC__`
 * would compile the wrong branch. This header is force-included by the
 * musa-nvcc wrapper to restore the CUDA spelling, and is included by every
 * cuda*.h wrapper header.
 */
#ifndef MUSA_CUDA_COMPAT_BASE_H
#define MUSA_CUDA_COMPAT_BASE_H

/* CUDA source level this layer implements. The MUSA runtime exports a subset
 * of CUDA 11, so feature-test entry points instead of assuming all of them. */
#ifndef CUDART_VERSION
#define CUDART_VERSION 11000
#endif
#ifndef CUDA_VERSION
#define CUDA_VERSION 11000
#endif
#ifndef CUDA_API_VERSION
#define CUDA_API_VERSION CUDART_VERSION
#endif

/* Restore the CUDA compiler-identification macros the MUSA wrapper strips. */
#ifndef __CUDACC__
#define __CUDACC__ 1
#endif
#ifndef __NVCC__
#define __NVCC__ 1
#endif
#ifndef __CUDACC_VER_MAJOR__
#define __CUDACC_VER_MAJOR__ 11
#define __CUDACC_VER_MINOR__ 0
#define __CUDACC_VER_BUILD__ 0
#endif

/* Host/device split guards written for CUDA. */
#ifndef __CUDA_ARCH__
#if defined(__MUSACC_ARCH__) || defined(__MUSACC__)
#define __CUDA_ARCH__ 1000
#endif
#endif
#ifndef MUSA_DEVICE_ARCH
#define MUSA_DEVICE_ARCH "mp_10"
#endif

#endif /* MUSA_CUDA_COMPAT_BASE_H */

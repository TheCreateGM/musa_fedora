/*************************************************************************/ /*!
@File
@Title          MT Debug Declarations
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Provides debug functionality
@License        Dual MIT/GPLv2

The contents of this file are subject to the MIT license as set out below.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

Alternatively, the contents of this file may be used under the terms of
the GNU General Public License Version 2 ("GPL") in which case the provisions
of GPL are applicable instead of those above.

If you wish to allow use of your version of this file only under the terms of
GPL, and not to allow others to use your version of this file under the terms
of the MIT license, indicate your decision by deleting the provisions above
and replace them with the notice and other provisions required by GPL as set
out in the file called "GPL-COPYING" included in this distribution. If you do
not delete the provisions above, a recipient may use your version of this file
under the terms of either the MIT license or GPL.

This License is also included in this distribution in the file called
"MIT-COPYING".

EXCEPT AS OTHERWISE STATED IN A NEGOTIATED AGREEMENT: (A) THE SOFTWARE IS
PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT; AND (B) IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/ /**************************************************************************/

#ifndef MT_DEBUG_H
#define MT_DEBUG_H

#include "img_defs.h"
#include "img_types.h"
#include "mtgpu_error.h"

/*! @cond Doxygen_Suppress */
#if defined(_MSC_VER)
#	define MSC_SUPPRESS_4127 __pragma(warning(suppress:4127))
#else
#	define MSC_SUPPRESS_4127
#endif
/*! @endcond */

#if defined(__cplusplus)
extern "C" {
#endif

#define MT_MAX_DEBUG_MESSAGE_LEN	(512)   /*!< Max length of a Debug Message */

/* These are privately used by mt_debug, use the MT_DBG_ defines instead */
#define DBGPRIV_FATAL     0x001UL  /*!< Debug-Fatal. Privately used by mt_debug. */
#define DBGPRIV_ERROR     0x002UL  /*!< Debug-Error. Privately used by mt_debug. */
#define DBGPRIV_WARNING   0x004UL  /*!< Debug-Warning. Privately used by mt_debug. */
#define DBGPRIV_MESSAGE   0x008UL  /*!< Debug-Message. Privately used by mt_debug. */
#define DBGPRIV_VERBOSE   0x010UL  /*!< Debug-Verbose. Privately used by mt_debug. */
#define DBGPRIV_CALLTRACE 0x020UL  /*!< Debug-CallTrace. Privately used by mt_debug. */
#define DBGPRIV_ALLOC     0x040UL  /*!< Debug-Alloc. Privately used by mt_debug. */
#define DBGPRIV_BUFFERED  0x080UL  /*!< Debug-Buffered. Privately used by mt_debug. */
#define DBGPRIV_DEBUG     0x100UL  /*!< Debug-AdHoc-Debug. Never submitted. Privately used by mt_debug. */
#define DBGPRIV_LAST      0x100UL  /*!< Always set to highest mask value. Privately used by mt_debug. */

/* Enable DPF logging for locally from some make targets */
#if defined(MTGPU_NEED_MT_DPF_LOCAL)
#undef MTGPU_NEED_MT_DPF
#define MTGPU_NEED_MT_DPF
#endif

#if !defined(MTGPU_NEED_MT_ASSERT) && defined(DEBUG)
#define MTGPU_NEED_MT_ASSERT
#endif

#if defined(MTGPU_NEED_MT_ASSERT) && !defined(MTGPU_NEED_MT_DPF)
#define MTGPU_NEED_MT_DPF
#endif

#if !defined(MTGPU_NEED_MT_TRACE) && (defined(DEBUG) || defined(TIMING))
#define MTGPU_NEED_MT_TRACE
#endif

#if !defined(DOXYGEN)
/*************************************************************************/ /*
MTGPUGetErrorString
Returns a string describing the provided MTGPU_ERROR code
NB No doxygen comments provided as this function does not require porting
   for other operating systems
*/ /**************************************************************************/
const MT_CHAR *MTGPUGetErrorString(MTGPU_ERROR eError);
#define MTGPUGETERRORSTRING MTGPUGetErrorString
#endif

/* MT_ASSERT() and MT_DBG_BREAK handling */

#if defined(__KLOCWORK__)
/* A dummy no-return function to be used under Klocwork to mark unreachable
   paths instead of abort() in order to avoid MISRA.STDLIB.ABORT issues. */
__noreturn void klocwork_abort(void);
#endif

#if defined(MTGPU_NEED_MT_ASSERT) || defined(DOXYGEN)

/* Unfortunately the Klocwork static analysis checker doesn't understand our
 * ASSERT macros. Thus it reports lots of false positive. Defining our Assert
 * macros in a special way when the code is analysed by Klocwork avoids
 * them.
 */
#if defined(__KLOCWORK__)
#define MT_ASSERT(x) do { if (!(x)) {klocwork_abort();} } while (false)
#else /* ! __KLOCWORKS__ */

#if defined(_WIN32)
#define MT_ASSERT(expr) do										\
	{															\
		MSC_SUPPRESS_4127										\
		if (unlikely(!(expr)))								\
		{														\
			MTGPUDebugPrintf(DBGPRIV_FATAL, __FILE__, __LINE__,\
					  "*** Debug assertion failed!");			\
			__debugbreak();										\
		}														\
	MSC_SUPPRESS_4127											\
	} while (false)

#else

#if defined(__linux__) && defined(__KERNEL__)
int OSWarnOn(MT_BOOL bCondition);

/* In Linux kernel mode, use WARN_ON() directly. This produces the
 * correct filename and line number in the warning message.
 */
#define MT_ASSERT(EXPR) do											\
	{																\
		if (unlikely(!(EXPR)))										\
		{															\
			MTGPUDebugPrintf(DBGPRIV_FATAL, __FILE__, __LINE__,	\
							  "Debug assertion failed!");			\
			OSWarnOn(1);												\
		}															\
	} while (false)

#else /* defined(__linux__) && defined(__KERNEL__) */

/*************************************************************************/ /*!
@Function       MTGPUDebugAssertFail
@Description    Indicate to the user that a debug assertion has failed and
                prevent the program from continuing.
                Invoked from the macro MT_ASSERT().
@Input          pszFile       The name of the source file where the assertion failed
@Input          ui32Line      The line number of the failed assertion
@Input          pszAssertion  String describing the assertion
@Return         NEVER!
*/ /**************************************************************************/
MT_EXPORT void MT_CALLCONV __noreturn
MTGPUDebugAssertFail(const MT_CHAR *pszFile,
                      MT_UINT32 ui32Line,
                      const MT_CHAR *pszAssertion);

#define MT_ASSERT(EXPR) do										\
	{															\
		if (unlikely(!(EXPR)))									\
		{														\
			MTGPUDebugAssertFail(__FILE__, __LINE__, #EXPR);	\
		}														\
	} while (false)

#endif /* defined(__linux__) && defined(__KERNEL__) */
#endif /* defined(_WIN32) */
#endif /* defined(__KLOCWORK__) */

#if defined(__KLOCWORK__)
	#define MT_DBG_BREAK do { klocwork_abort(); } while (false)
#else
	#if defined(WIN32)
		#define MT_DBG_BREAK __debugbreak()   /*!< Implementation of MT_DBG_BREAK for (non-WinCE) Win32 */
	#else
		#if defined(MT_DBG_BREAK_ASSERT_FAIL)
		/*!< Implementation of MT_DBG_BREAK that maps onto MTGPUDebugAssertFail */
			#if defined(_WIN32)
				#define MT_DBG_BREAK	DBG_BREAK
			#else
				#if defined(__linux__) && defined(__KERNEL__)
					void OSPanic(void);
					#define MT_DBG_BREAK	OSPanic()
				#else
					#define MT_DBG_BREAK	MTGPUDebugAssertFail(__FILE__, __LINE__, "MT_DBG_BREAK")
				#endif
			#endif
		#else
			/*!< Null Implementation of MT_DBG_BREAK (does nothing) */
			#define MT_DBG_BREAK
		#endif
	#endif
#endif


#else /* defined(MTGPU_NEED_MT_ASSERT) */
	/* Unfortunately the Klocwork static analysis checker doesn't understand our
	* ASSERT macros. Thus it reports lots of false positive. Defining our Assert
	* macros in a special way when the code is analysed by Klocwork avoids
	* them.
	*/
	#if defined(__KLOCWORK__) && !defined(SERVICES_SC)
		#define MT_ASSERT(EXPR) do { if (!(EXPR)) {klocwork_abort();} } while (false)
	#else
		#define MT_ASSERT(EXPR) (void)(EXPR) /*!< Null Implementation of MT_ASSERT (does nothing) */
	#endif

	#define MT_DBG_BREAK    /*!< Null Implementation of MT_DBG_BREAK (does nothing) */

#endif /* defined(MTGPU_NEED_MT_ASSERT) */


/* MT_DPF() handling */

#if defined(MTGPU_NEED_MT_DPF) || defined(DOXYGEN)

	/* New logging mechanism */
	#define MT_DBG_FATAL     DBGPRIV_FATAL     /*!< Debug level passed to MTGPUDebugPrintf() for fatal errors. */
	#define MT_DBG_ERROR     DBGPRIV_ERROR     /*!< Debug level passed to MTGPUDebugPrintf() for non-fatal errors. */
	#define MT_DBG_WARNING   DBGPRIV_WARNING   /*!< Debug level passed to MTGPUDebugPrintf() for warnings. */
	#define MT_DBG_MESSAGE   DBGPRIV_MESSAGE   /*!< Debug level passed to MTGPUDebugPrintf() for information only. */
	#define MT_DBG_VERBOSE   DBGPRIV_VERBOSE   /*!< Debug level passed to MTGPUDebugPrintf() for very low-priority debug. */
	#define MT_DBG_CALLTRACE DBGPRIV_CALLTRACE /*!< Debug level passed to MTGPUDebugPrintf() for function tracing purposes. */
	#define MT_DBG_ALLOC     DBGPRIV_ALLOC     /*!< Debug level passed to MTGPUDebugPrintf() for tracking some of drivers memory operations. */
	#define MT_DBG_BUFFERED  DBGPRIV_BUFFERED  /*!< Debug level passed to MTGPUDebugPrintf() when debug should be written to the debug circular buffer. */
	#define MT_DBG_DEBUG     DBGPRIV_DEBUG     /*!< Debug level passed to MTGPUDebugPrintf() for debug messages. */

	/* These levels are always on with MTGPU_NEED_MT_DPF */
	/*! @cond Doxygen_Suppress */
	#define MT_DPF_0x001UL(...) MTGPUDebugPrintf(DBGPRIV_FATAL, __VA_ARGS__)
	#define MT_DPF_0x002UL(...) MTGPUDebugPrintf(DBGPRIV_ERROR, __VA_ARGS__)
	#define MT_DPF_0x080UL(...) MTGPUDebugPrintf(DBGPRIV_BUFFERED, __VA_ARGS__)

	/*
	 * The AdHoc-Debug level is only supported when enabled in the local
	 * build environment and may need to be used in both debug and release
	 * builds. An error is generated in the formal build if it is checked in.
	 */
#if defined(MT_DPF_ADHOC_DEBUG_ON)
	#define MT_DPF_0x100UL(...) MTGPUDebugPrintf(DBGPRIV_DEBUG, __VA_ARGS__)
#else
	/* Use an undefined token here to stop compilation dead in the offending module */
	#define MT_DPF_0x100UL(...) __ERROR__MT_DBG_DEBUG_is_in_use_but_has_not_been_enabled__Note_Debug_DPF_must_not_be_checked_in__Define_MT_DPF_ADHOC_DEBUG_ON_for_testing
#endif

	/* Some are compiled out completely in release builds */
#if defined(DEBUG) || defined(DOXYGEN)
	#define MT_DPF_0x004UL(...) MTGPUDebugPrintf(DBGPRIV_WARNING, __VA_ARGS__)
	#define MT_DPF_0x008UL(...) MTGPUDebugPrintf(DBGPRIV_MESSAGE, __VA_ARGS__)
	#define MT_DPF_0x010UL(...) MTGPUDebugPrintf(DBGPRIV_VERBOSE, __VA_ARGS__)
	#define MT_DPF_0x020UL(...) MTGPUDebugPrintf(DBGPRIV_CALLTRACE, __VA_ARGS__)
	#define MT_DPF_0x040UL(...) MTGPUDebugPrintf(DBGPRIV_ALLOC, __VA_ARGS__)
#else
	#define MT_DPF_0x004UL(...)
	#define MT_DPF_0x008UL(...)
	#define MT_DPF_0x010UL(...)
	#define MT_DPF_0x020UL(...)
	#define MT_DPF_0x040UL(...)
#endif

	/* Translate the different log levels to separate macros
	 * so they can each be compiled out.
	 */
#if defined(DEBUG)
	#define MT_DPF_EX(lvl, ...) MT_DPF_ ## lvl (__FILE__, __LINE__, __VA_ARGS__)
#else
	#define MT_DPF_EX(lvl, ...) MT_DPF_ ## lvl ("", __LINE__, __VA_ARGS__)
#endif
	/*! @endcond */

	/* Get rid of the double bracketing */
	#define MT_DPF(x) MT_DPF_EX x

	#define MT_LOG_ERROR(_rc, _call) \
		MT_DPF((MT_DBG_ERROR, "%s() failed (%s) in %s()", _call, MTGPUGETERRORSTRING(_rc), __func__))

	#define MT_LOG_IF_ERROR(_rc, _call) do \
		{ if (unlikely(_rc != MTGPU_OK)) { \
			MT_DPF((MT_DBG_ERROR, "%s() failed (%s) in %s()", _call, MTGPUGETERRORSTRING(_rc), __func__)); \
		  } \
		MSC_SUPPRESS_4127\
		} while (false)

	#define MT_WARN_IF_ERROR(_rc, _call) do \
		{ if (unlikely(_rc != MTGPU_OK)) { \
			MT_DPF((MT_DBG_WARNING, "%s() failed (%s) in %s()", _call, MTGPUGETERRORSTRING(_rc), __func__)); \
		  } \
		MSC_SUPPRESS_4127\
		} while (false)

	#define MT_LOG_RETURN_IF_NOMEM(_expr, _call) do \
		{ if (unlikely(_expr == NULL)) { \
			MT_DPF((MT_DBG_ERROR, "%s failed (MTGPU_ERROR_OUT_OF_MEMORY) in %s()", _call, __func__)); \
			return MTGPU_ERROR_OUT_OF_MEMORY; } \
		MSC_SUPPRESS_4127\
		} while (false)

	#define MT_LOG_GOTO_IF_NOMEM(_expr, _err, _go) do \
		{ if (unlikely(_expr == NULL)) { \
			MT_DPF((MT_DBG_ERROR, "%s failed (MTGPU_ERROR_OUT_OF_MEMORY) in %s()", #_expr, __func__)); \
			_err = MTGPU_ERROR_OUT_OF_MEMORY; \
			goto _go; } \
		MSC_SUPPRESS_4127\
		} while (false)

	#define MT_LOG_RETURN_IF_ERROR(_rc, _call) do \
		{ if (unlikely(_rc != MTGPU_OK)) { \
			MT_DPF((MT_DBG_ERROR, "%s() failed (%s) in %s()", _call, MTGPUGETERRORSTRING(_rc), __func__)); \
			return _rc; } \
		MSC_SUPPRESS_4127\
		} while (false)

	#define MT_LOG_RETURN_VOID_IF_ERROR(_rc, _call) do \
		{ if (unlikely(_rc != MTGPU_OK)) { \
			MT_DPF((MT_DBG_ERROR, "%s() failed (%s) in %s()", _call, MTGPUGETERRORSTRING(_rc), __func__)); \
			return; } \
		MSC_SUPPRESS_4127\
		} while (false)

	#define MT_LOG_GOTO_IF_ERROR(_rc, _call, _go) do \
		{ if (unlikely(_rc != MTGPU_OK)) { \
			MT_DPF((MT_DBG_ERROR, "%s() failed (%s) in %s()", _call, MTGPUGETERRORSTRING(_rc), __func__)); \
			goto _go; } \
		MSC_SUPPRESS_4127\
		} while (false)

	#define MT_LOG_GOTO_WITH_ERROR(_call, _err, _rc, _go) do \
		{ MT_DPF((MT_DBG_ERROR, "%s() failed (%s) in %s()", _call, MTGPUGETERRORSTRING(_rc), __func__)); \
			_err = _rc; \
			goto _go; \
		MSC_SUPPRESS_4127\
		} while (false)

	#define MT_LOG_IF_FALSE(_expr, _msg) do \
		{ if (unlikely(!(_expr))) { \
			MT_DPF((MT_DBG_ERROR, "%s in %s()", _msg, __func__)); \
		  } \
		MSC_SUPPRESS_4127\
		} while (false)

	#define MT_LOG_RETURN_IF_FALSE(_expr, _msg, _rc) do \
		{ if (unlikely(!(_expr))) { \
			MT_DPF((MT_DBG_ERROR, "%s in %s()", _msg, __func__)); \
			return _rc; } \
		MSC_SUPPRESS_4127\
		} while (false)

	#define MT_LOG_RETURN_VOID_IF_FALSE(_expr, _msg) do \
		{ if (unlikely(!(_expr))) { \
			MT_DPF((MT_DBG_ERROR, "%s in %s()", _msg, __func__)); \
			return; } \
		MSC_SUPPRESS_4127\
		} while (false)

	#define MT_LOG_GOTO_IF_FALSE(_expr, _msg, _go) do \
		{ if (unlikely(!(_expr))) { \
			MT_DPF((MT_DBG_ERROR, "%s in %s()", _msg, __func__)); \
			goto _go; } \
		MSC_SUPPRESS_4127\
		} while (false)

	#define MT_LOG_RETURN_IF_INVALID_PARAM(_expr, _param) do \
		{ if (unlikely(!(_expr))) { \
			MT_DPF((MT_DBG_ERROR, "%s invalid in %s()", _param, __func__)); \
			return MTGPU_ERROR_INVALID_PARAMS; } \
		MSC_SUPPRESS_4127\
		} while (false)

	#define MT_LOG_GOTO_IF_INVALID_PARAM(_expr, _err, _go) do \
		{ if (unlikely(!(_expr))) { \
			MT_DPF((MT_DBG_ERROR, "%s invalid in %s()", #_expr, __func__)); \
			_err = MTGPU_ERROR_INVALID_PARAMS; \
			goto _go; } \
		MSC_SUPPRESS_4127\
		} while (false)

	#define MT_LOG_MSG(_lvl, _msg) \
		MT_DPF((_lvl, ("In %s() "_msg), __func__))

	#define MT_LOG_VA(_lvl, _msg, ...) \
		MT_DPF((_lvl, ("In %s() "_msg), __func__, __VA_ARGS__))

	#define MT_LOG_IF_ERROR_VA(_lvl, _rc, _msg, ...) do \
		{ if (unlikely(_rc != MTGPU_OK)) { \
			MT_DPF((_lvl, ("In %s() "_msg), __func__, __VA_ARGS__)); \
		} \
		MSC_SUPPRESS_4127\
		} while (false)

	#define MT_LOG_IF_FALSE_VA(_lvl, _expr, _msg, ...) do \
		{ if (unlikely(!(_expr))) { \
			MT_DPF((_lvl, ("In %s() "_msg), __func__, __VA_ARGS__)); \
		} \
		MSC_SUPPRESS_4127\
		} while (false)

	#define MT_LOG_RETURN_IF_ERROR_VA(_rc, _msg, ...) do \
		{ if (unlikely(_rc != MTGPU_OK)) { \
			MT_DPF((MT_DBG_ERROR, ("In %s() "_msg), __func__, __VA_ARGS__)); \
			return _rc; \
		} MSC_SUPPRESS_4127\
		} while (false)

	#define MT_LOG_GOTO_IF_ERROR_VA(_rc, _go, _msg, ...) do \
		{ if (unlikely(_rc != MTGPU_OK)) { \
			MT_DPF((MT_DBG_ERROR, ("In %s() "_msg), __func__, __VA_ARGS__)); \
			goto _go; \
		} MSC_SUPPRESS_4127\
		} while (false)

	#define MT_LOG_RETURN_IF_FALSE_VA(_expr, _rc, _msg, ...) do \
		{ if (unlikely(!(_expr))) { \
			MT_DPF((MT_DBG_ERROR, ("At %s: "_msg), __func__, __VA_ARGS__)); \
			return _rc; \
		} MSC_SUPPRESS_4127\
		} while (false)

	#define MT_LOG_GOTO_IF_FALSE_VA(_expr, _go, _msg, ...) do \
		{ if (unlikely(!(_expr))) { \
			MT_DPF((MT_DBG_ERROR, ("In %s() "_msg), __func__, __VA_ARGS__)); \
			goto _go; \
		} MSC_SUPPRESS_4127\
		} while (false)

#else /* defined(MTGPU_NEED_MT_DPF) */

	#define MT_DPF(X)  /*!< Null Implementation of PowerVR Debug Printf (does nothing) */

	#define MT_LOG_MSG(_lvl, _msg)
	#define MT_LOG_VA(_lvl, _msg, ...)
	#define MT_LOG_ERROR(_rc, _call) (void)(_rc)
	#define MT_LOG_IF_ERROR(_rc, _call) (void)(_rc)
	#define MT_WARN_IF_ERROR(_rc, _call) (void)(_rc)

	#define MT_LOG_IF_ERROR_VA(_lvl, _rc, _msg, ...) (void)(_rc)
	#define MT_LOG_IF_FALSE_VA(_lvl, _expr, _msg, ...) (void)(_expr)

	#define MT_LOG_RETURN_IF_NOMEM(_expr, _call) do { if (unlikely(_expr == NULL)) { return MTGPU_ERROR_OUT_OF_MEMORY; } MSC_SUPPRESS_4127 } while (false)
	#define MT_LOG_GOTO_IF_NOMEM(_expr, _err, _go) do { if (unlikely(_expr == NULL)) { _err = MTGPU_ERROR_OUT_OF_MEMORY; goto _go; } MSC_SUPPRESS_4127	} while (false)

	#define MT_LOG_RETURN_IF_ERROR(_rc, _call) do { if (unlikely(_rc != MTGPU_OK)) { return (_rc); } MSC_SUPPRESS_4127 } while (false)
	#define MT_LOG_RETURN_IF_ERROR_VA(_rc, _msg, ...) do { if (unlikely(_rc != MTGPU_OK)) { return (_rc); } MSC_SUPPRESS_4127 } while (false)
	#define MT_LOG_RETURN_VOID_IF_ERROR(_rc, _call) do { if (unlikely(_rc != MTGPU_OK)) { return; } MSC_SUPPRESS_4127 } while (false)

	#define MT_LOG_GOTO_IF_ERROR(_rc, _call, _go) do { if (unlikely(_rc != MTGPU_OK)) { goto _go; } MSC_SUPPRESS_4127 } while (false)
	#define MT_LOG_GOTO_IF_ERROR_VA(_rc, _go, _msg, ...) do { if (unlikely(_rc != MTGPU_OK)) { goto _go; } MSC_SUPPRESS_4127 } while (false)
	#define MT_LOG_GOTO_WITH_ERROR(_call, _err, _rc, _go) do { _err = _rc; goto _go; MSC_SUPPRESS_4127 } while (false)

	#define MT_LOG_IF_FALSE(_expr, _msg) (void)(_expr)
	#define MT_LOG_RETURN_IF_FALSE(_expr, _msg, _rc) do { if (unlikely(!(_expr))) { return (_rc); } MSC_SUPPRESS_4127 } while (false)
	#define MT_LOG_RETURN_IF_FALSE_VA(_expr, _rc, _msg, ...) do { if (unlikely(!(_expr))) { return (_rc); } MSC_SUPPRESS_4127 } while (false)

	#define MT_LOG_RETURN_VOID_IF_FALSE(_expr, _msg) do { if (unlikely(!(_expr))) { return; } MSC_SUPPRESS_4127 } while (false)
	#define MT_LOG_GOTO_IF_FALSE(_expr, _msg, _go) do { if (unlikely(!(_expr))) { goto _go; } MSC_SUPPRESS_4127 } while (false)
	#define MT_LOG_GOTO_IF_FALSE_VA(_expr, _go, _msg, ...) do { if (unlikely(!(_expr))) { goto _go; } MSC_SUPPRESS_4127 } while (false)

	#define MT_LOG_RETURN_IF_INVALID_PARAM(_expr, _param) do { if (unlikely(!(_expr))) { return MTGPU_ERROR_INVALID_PARAMS; } MSC_SUPPRESS_4127 } while (false)
	#define MT_LOG_GOTO_IF_INVALID_PARAM(_expr, _err, _go) do { if (unlikely(!(_expr))) { _err = MTGPU_ERROR_INVALID_PARAMS; goto _go; } MSC_SUPPRESS_4127 } while (false)

	#undef MT_DPF_FUNCTION_TRACE_ON

#endif /* defined(MTGPU_NEED_MT_DPF) */

/*************************************************************************/ /*!
@Function       MTGPUDebugPrintf
@Description    Output a debug message to the user, using an OS-specific
                method, to a log or console which can be read by developers
                Invoked from the macro MT_DPF().
@Input          ui32DebugLevel   The debug level of the message. This can
                                 be used to restrict the output of debug
                                 messages based on their severity.
                                 If this is MT_DBG_BUFFERED, the message
                                 should be written into a debug circular
                                 buffer instead of being output immediately
                                 (useful when performance would otherwise
                                 be adversely affected).
                                 The debug circular buffer shall only be
                                 output when MTGPUDebugPrintfDumpCCB() is
                                 called.
@Input          pszFileName      The source file containing the code that is
                                 generating the message
@Input          ui32Line         The line number in the source file
@Input          pszFormat        The formatted message string
@Input          ...              Zero or more arguments for use by the
                                 formatted string
@Return         None
*/ /**************************************************************************/
MT_EXPORT void MT_CALLCONV MTGPUDebugPrintf(MT_UINT32 ui32DebugLevel,
                                               const MT_CHAR *pszFileName,
                                               MT_UINT32 ui32Line,
                                               const MT_CHAR *pszFormat,
                                               ...) __printf(4, 5);

/*************************************************************************/ /*!
@Function       MTGPUDebugPrintfDumpCCB
@Description    When MTGPUDebugPrintf() is called with the ui32DebugLevel
                specified as DBGPRIV_BUFFERED, the debug shall be written to
                the debug circular buffer instead of being output immediately.
                (This could be used to obtain debug without incurring a
                performance hit by printing it at that moment).
                This function shall dump the contents of that debug circular
                buffer to be output in an OS-specific method to a log or
                console which can be read by developers.
@Return         None
*/ /**************************************************************************/
MT_EXPORT void MT_CALLCONV MTGPUDebugPrintfDumpCCB(void);

#if !defined(DOXYGEN)
#define MT_DPF_FUNC__(lvl, message, ...) MT_DPF((lvl, "%s: " message, __func__, ##__VA_ARGS__))
#define MT_DPF_FUNC(x) MT_DPF_FUNC__ x
#endif /*!defined(DOXYGEN) */

/* Note: Use only when a log message due to the error absolutely should not
 *       be printed. Otherwise use MT_LOG_RETURN_IF_ERROR macro.
 */
#define MT_RETURN_IF_ERROR(_rc) do \
	{ if (unlikely(_rc != MTGPU_OK)) { \
		return _rc; } \
	MSC_SUPPRESS_4127 \
	} while (false)

/* Note: Use only when a log message due to the error absolutely should not
 *       be printed. Otherwise use MT_LOG_RETURN_IF_FALSE macro.
 */
#define MT_RETURN_IF_FALSE(_expr, _rc) do \
	{ if (unlikely(!(_expr))) { \
		return _rc; } \
	MSC_SUPPRESS_4127 \
	} while (false)

/* Note: Use only when a log message due to the error absolutely should not
 *       be printed. Otherwise use MT_LOG_RETURN_IF_INVALID_PARAM macro.
 */
#define MT_RETURN_IF_INVALID_PARAM(_expr) do \
	{ if (unlikely(!(_expr))) { \
		return MTGPU_ERROR_INVALID_PARAMS; } \
	MSC_SUPPRESS_4127 \
	} while (false)

/* Note: Use only when a log message due to the error absolutely should not
 *       be printed. Otherwise use MT_LOG_RETURN_IF_NOMEM macro.
 */
#define MT_RETURN_IF_NOMEM(_expr) do \
	{ if (unlikely(!(_expr))) { \
		return MTGPU_ERROR_OUT_OF_MEMORY; } \
	MSC_SUPPRESS_4127 \
	} while (false)

/* Note: Use only when a log message due to the error absolutely should not
 *       be printed. Otherwise use MT_LOG_GOTO_IF_NOMEM macro.
 */
#define MT_GOTO_IF_NOMEM(_expr, _err, _go) do \
	{ if (unlikely(_expr == NULL)) { \
		_err = MTGPU_ERROR_OUT_OF_MEMORY; \
		goto _go; } \
	MSC_SUPPRESS_4127 \
	} while (false)

/* Note: Use only when a log message due to the error absolutely should not
 *       be printed. Otherwise use MT_LOG_GOTO_IF_INVALID_PARAM macro.
 */
#define MT_GOTO_IF_INVALID_PARAM(_expr, _err, _go) do \
	{ if (unlikely(!(_expr))) { \
		_err = MTGPU_ERROR_INVALID_PARAMS; \
		goto _go; } \
	MSC_SUPPRESS_4127 \
	} while (false)

/* Note: Use only when a log message due to the error absolutely should not
 *       be printed. Otherwise use MT_LOG_GOTO_IF_FALSE macro.
 */
#define MT_GOTO_IF_FALSE(_expr, _go) do \
	{ if (unlikely(!(_expr))) { \
		goto _go; } \
	MSC_SUPPRESS_4127 \
	} while (false)

/* Note: Use only when a log message due to the error absolutely should not
 *       be printed. Otherwise use MT_LOG_GOTO_IF_ERROR macro.
 */
#define MT_GOTO_IF_ERROR(_rc, _go) do \
	{ if (unlikely(_rc != MTGPU_OK)) { \
		goto _go; } \
	MSC_SUPPRESS_4127\
	} while (false)

/* Note: Use only when a log message due to the error absolutely should not
 *       be printed. Otherwise use MT_LOG_GOTO_WITH_ERROR macro.
 */
#define MT_GOTO_WITH_ERROR(_err, _rc, _go) do \
	{ _err = _rc; goto _go; \
	MSC_SUPPRESS_4127 \
	} while (false)

/*! @cond Doxygen_Suppress */
#if defined(MT_DPF_FUNCTION_TRACE_ON)

	#define MT_DPF_ENTERED \
	MT_DPF((MT_DBG_CALLTRACE, "|-> %s:%d entered", __func__, __LINE__))

	#define MT_DPF_ENTERED1(p1) \
		MT_DPF((MT_DBG_CALLTRACE, "|-> %s:%d entered (0x%lx)", __func__, __LINE__, ((unsigned long)p1)))

	#define MT_DPF_RETURN_RC(a) \
	do { int _r = (a); MT_DPF((MT_DBG_CALLTRACE, "<-| %s:%d returned %d", __func__, __LINE__, (_r))); return (_r); MSC_SUPPRESS_4127 } while (false)

	#define MT_DPF_RETURN_RC1(a,p1) \
		do { int _r = (a); MT_DPF((MT_DBG_CALLTRACE, "<-| %s:%d returned %d (0x%lx)", __func__, __LINE__, (_r), ((unsigned long)p1))); return (_r); MSC_SUPPRESS_4127 } while (false)

	#define MT_DPF_RETURN_VAL(a) \
		do { MT_DPF((MT_DBG_CALLTRACE, "<-| %s:%d returned with value", __func__, __LINE__)); return (a); MSC_SUPPRESS_4127 } while (false)

	#define MT_DPF_RETURN_OK \
		do { MT_DPF((MT_DBG_CALLTRACE, "<-| %s:%d returned ok", __func__, __LINE__)); return MTGPU_OK; MSC_SUPPRESS_4127 } while (false)

	#define MT_DPF_RETURN \
		do { MT_DPF((MT_DBG_CALLTRACE, "<-| %s:%d returned", __func__, __LINE__)); return; MSC_SUPPRESS_4127 } while (false)

	#if !defined(DEBUG)
	#error MT DPF Function trace enabled in release build, rectify
	#endif

#else /* defined(MT_DPF_FUNCTION_TRACE_ON) */

	#define MT_DPF_ENTERED
	#define MT_DPF_ENTERED1(p1)
	#define MT_DPF_RETURN_RC(a)     return (a)
	#define MT_DPF_RETURN_RC1(a,p1) return (a)
	#define MT_DPF_RETURN_VAL(a)    return (a)
	#define MT_DPF_RETURN_OK        return MTGPU_OK
	#define MT_DPF_RETURN           return

#endif /* defined(MT_DPF_FUNCTION_TRACE_ON) */
/*! @endcond */

#if defined(__KERNEL__) || defined(DOXYGEN) || defined(__QNXNTO__)
/*Use MT_DPF() unless message is necessary in release build */
#define MT_LOG(X) MTGPUReleasePrintf X

/*************************************************************************/ /*!
@Function       MTGPUReleasePrintf
@Description    Output an important message, using an OS-specific method,
                to the Server log or console which will always be output in
                both release and debug builds.
                Invoked from the macro MT_LOG(). Used in Services Server only.
@Input          pszFormat   The message format string
@Input          ...         Zero or more arguments for use by the format string
@Return         None
*/ /**************************************************************************/
void MT_CALLCONV MTGPUReleasePrintf(const MT_CHAR *pszFormat, ...) __printf(1, 2);
#endif

/* MT_TRACE() handling */

#if defined(MTGPU_NEED_MT_TRACE) || defined(DOXYGEN)

	#define MT_TRACE(X)	MTGPUTrace X    /*!< PowerVR Debug Trace Macro */
	/* Empty string implementation that is -O0 build friendly */
	#define MT_TRACE_EMPTY_LINE()	MT_TRACE(("%s", ""))

/*************************************************************************/ /*!
@Function       MTTrace
@Description    Output a debug message to the user
                Invoked from the macro MT_TRACE().
@Input          pszFormat   The message format string
@Input          ...         Zero or more arguments for use by the format string
*/ /**************************************************************************/
MT_EXPORT void MT_CALLCONV MTGPUTrace(const MT_CHAR* pszFormat, ... )
	__printf(1, 2);

#else /* defined(MTGPU_NEED_MT_TRACE) */
	/*! Null Implementation of PowerVR Debug Trace Macro (does nothing) */
	#define MT_TRACE(X)

#endif /* defined(MTGPU_NEED_MT_TRACE) */


#if defined(MTGPU_NEED_MT_ASSERT)
#ifdef INLINE_IS_PRAGMA
#pragma inline(TRUNCATE_64BITS_TO_32BITS)
#endif
	INLINE static MT_UINT32 TRUNCATE_64BITS_TO_32BITS(MT_UINT64 uiInput)
	{
		MT_UINT32 uiTruncated;

		uiTruncated = (MT_UINT32)uiInput;
		MT_ASSERT(uiInput == uiTruncated);
		return uiTruncated;
	}


#ifdef INLINE_IS_PRAGMA
#pragma inline(TRUNCATE_64BITS_TO_SIZE_T)
#endif
	INLINE static size_t TRUNCATE_64BITS_TO_SIZE_T(MT_UINT64 uiInput)
	{
		size_t uiTruncated;

		uiTruncated = (size_t)uiInput;
		MT_ASSERT(uiInput == uiTruncated);
		return uiTruncated;
	}


#ifdef INLINE_IS_PRAGMA
#pragma inline(TRUNCATE_SIZE_T_TO_32BITS)
#endif
	INLINE static MT_UINT32 TRUNCATE_SIZE_T_TO_32BITS(size_t uiInput)
	{
		MT_UINT32 uiTruncated;

		uiTruncated = (MT_UINT32)uiInput;
		MT_ASSERT(uiInput == uiTruncated);
		return uiTruncated;
	}


#else /* defined(MTGPU_NEED_MT_ASSERT) */
	#define TRUNCATE_64BITS_TO_32BITS(expr) ((MT_UINT32)(expr))
	#define TRUNCATE_64BITS_TO_SIZE_T(expr) ((size_t)(expr))
	#define TRUNCATE_SIZE_T_TO_32BITS(expr) ((MT_UINT32)(expr))
#endif /* defined(MTGPU_NEED_MT_ASSERT) */

/*! @cond Doxygen_Suppress */
/* Macros used to trace calls */
#if defined(DEBUG)
	#define MT_DBG_FILELINE , (__FILE__), (__LINE__)
	#define MT_DBG_FILELINE_PARAM , const MT_CHAR *pszaFile, MT_UINT32 ui32Line
	#define MT_DBG_FILELINE_ARG , pszaFile, ui32Line
	#define MT_DBG_FILELINE_FMT " %s:%u"
	#define MT_DBG_FILELINE_UNREF() do { MT_UNREFERENCED_PARAMETER(pszaFile); \
				MT_UNREFERENCED_PARAMETER(ui32Line); } while (false)
#else
	#define MT_DBG_FILELINE
	#define MT_DBG_FILELINE_PARAM
	#define MT_DBG_FILELINE_ARG
	#define MT_DBG_FILELINE_FMT
	#define MT_DBG_FILELINE_UNREF()
#endif
/*! @endcond */

#if defined(__cplusplus)
}
#endif

/*!
    @def MT_ASSERT
    @brief Aborts the program if assertion fails.

    The macro will be defined only when MTGPU_NEED_MT_ASSERT macro is
    enabled. It's ignored otherwise.

    @def MT_DPF
    @brief PowerVR Debug Printf logging macro used throughout the driver.

    The macro allows to print logging messages to appropriate log. The
    destination log is based on the component (user space / kernel space) and
    operating system (Linux, Android, etc.).

    The macro also supports severity levels that allow to turn on/off messages
    based on their importance.

    This macro will print messages with severity level higher that error only
    if MTGPU_NEED_MT_DPF macro is defined.

    @def MT_LOG_ERROR
    @brief Logs error.

    @def MT_LOG_IF_ERROR
    @brief Logs error if not MTGPU_OK.

    @def MT_WARN_IF_ERROR
    @brief Logs warning if not MTGPU_OK.

    @def MT_LOG_RETURN_IF_NOMEM
    @brief Logs error if expression is NULL and returns MTGPU_ERROR_OUT_OF_MEMORY.

    @def MT_LOG_GOTO_IF_NOMEM
    @brief Logs error if expression is NULL and jumps to given label.

    @def MT_LOG_RETURN_IF_ERROR
    @brief Logs error if not MTGPU_OK and returns the error.

    @def MT_LOG_RETURN_VOID_IF_ERROR
    @brief Logs error if not MTGPU_OK and returns (used in function that return void).

    @def MT_LOG_GOTO_IF_ERROR
    @brief Logs error if not MTGPU_OK and jumps to label.

    @def MT_LOG_GOTO_WITH_ERROR
    @brief Logs error, goes to a label and sets the error code.

    @def MT_LOG_IF_FALSE
    @brief Prints error message if expression is false.

    @def MT_LOG_RETURN_IF_FALSE
    @brief Prints error message if expression is false and returns given error.

    @def MT_LOG_RETURN_VOID_IF_FALSE
    @brief Prints error message if expression is false and returns (used in function that return void).

    @def MT_LOG_GOTO_IF_FALSE
    @brief Prints error message if expression is false and jumps to label.

    @def MT_LOG_RETURN_IF_INVALID_PARAM
    @brief Prints error message if expression is false and returns MTGPU_ERROR_INVALID_PARAMS.

    @def MT_LOG_GOTO_IF_INVALID_PARAM
    @brief Prints error message if expression is false and jumps to label.

    @def MT_RETURN_IF_ERROR
    @brief Returns passed error code if it's different than MTGPU_OK;

    @def MT_RETURN_IF_FALSE
    @brief Returns passed error code if expression is false.

    @def MT_RETURN_IF_INVALID_PARAM
    @brief Returns MTGPU_ERROR_INVALID_PARAMS if expression is false.

    @def MT_RETURN_IF_NOMEM
    @brief Returns MTGPU_ERROR_OUT_OF_MEMORY if expression is NULL.

    @def MT_GOTO_IF_NOMEM
    @brief Goes to a label if expression is NULL.

    @def MT_GOTO_IF_INVALID_PARAM
    @brief Goes to a label if expression is false.

    @def MT_GOTO_IF_FALSE
    @brief Goes to a label if expression is false.

    @def MT_GOTO_IF_ERROR
    @brief Goes to a label if the error code is different than MTGPU_OK;

    @def MT_GOTO_WITH_ERROR
    @brief Goes to a label and sets the error code.

    @def MT_LOG
    @brief Prints message to a log unconditionally.

    This macro will print messages only if MTGPU_NEED_MT_LOG macro is defined.
    @def MT_LOG_MSG
    @brief Prints message to a log with the given log-level.

    @def MT_LOG_VA
    @brief Prints message with var-args to a log with the given log-level.

    @def MT_LOG_IF_ERROR_VA
    @brief Prints message with var-args to a log if the error code is different than MTGPU_OK.

    @def MT_LOG_IF_FALSE_VA
    @brief Prints message with var-args if expression is false.

    @def MT_LOG_RETURN_IF_ERROR_VA
    @brief Prints message with var-args to a log and returns the error code.

    @def MT_LOG_GOTO_IF_ERROR_VA
    @brief Prints message with var-args to a log and goes to a label if the error code is different than MTGPU_OK.

    @def MT_LOG_RETURN_IF_FALSE_VA
    @brief Logs the error message with var-args if the expression is false and returns the error code.

    @def MT_LOG_GOTO_IF_FALSE_VA
    @brief Logs the error message with var-args and goes to a label if the expression is false.

    @def MT_TRACE_EMPTY_LINE
    @brief Prints empty line to a log (MTGPU_NEED_MT_LOG must be defined).

    @def TRUNCATE_64BITS_TO_32BITS
    @brief Truncates 64 bit value to 32 bit value (with possible precision loss).

    @def TRUNCATE_64BITS_TO_SIZE_T
    @brief Truncates 64 bit value to size_t value (with possible precision loss).

    @def TRUNCATE_SIZE_T_TO_32BITS
    @brief Truncates size_t value to 32 bit value (with possible precision loss).
 */

#endif /* MT_DEBUG_H */

/******************************************************************************
 End of file (mt_debug.h)
******************************************************************************/

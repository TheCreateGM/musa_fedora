/*************************************************************************/ /*!
@File
@Title          Debug Functionality
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Provides kernel side Debug Functionality.
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

#include <linux/sched.h>
#include <linux/pid.h>
#include <linux/moduleparam.h>

#include "img_types.h"
#include "img_defs.h"
#include "mt_debug.h"
#include "linkage.h"
#include "mt.h"
#include "osfunc.h"
#include "di_server.h"

#if defined(MTGPU_NEED_MT_DPF)

/******** BUFFERED LOG MESSAGES ********/

/* Because we don't want to have to handle CCB wrapping, each buffered
 * message is rounded up to MTGPU_DEBUG_CCB_MESG_MAX bytes. This means
 * there is the same fixed number of messages that can be stored,
 * regardless of message length.
 */

#if defined(MTGPU_DEBUG_CCB_MAX)

#define MTGPU_DEBUG_CCB_MESG_MAX	MT_MAX_DEBUG_MESSAGE_LEN

typedef struct
{
	const MT_CHAR *pszFile;
	MT_INT iLine;
	MT_UINT32 ui32TID;
	MT_UINT32 ui32PID;
	MT_CHAR pcMesg[MTGPU_DEBUG_CCB_MESG_MAX];
	struct timeval sTimeVal;
}
MTGPU_DEBUG_CCB;

static MTGPU_DEBUG_CCB gsDebugCCB[MTGPU_DEBUG_CCB_MAX];

static MT_UINT giOffset;

/* protects access to gsDebugCCB */
static DEFINE_SPINLOCK(gsDebugCCBLock);

static void
AddToBufferCCB(const MT_CHAR *pszFileName, MT_UINT32 ui32Line,
			   const MT_CHAR *szBuffer)
{
	unsigned long uiFlags;

	spin_lock_irqsave(&gsDebugCCBLock, uiFlags);

	gsDebugCCB[giOffset].pszFile = pszFileName;
	gsDebugCCB[giOffset].iLine   = ui32Line;
	gsDebugCCB[giOffset].ui32TID = current->pid;
	gsDebugCCB[giOffset].ui32PID = current->tgid;

	do_gettimeofday(&gsDebugCCB[giOffset].sTimeVal);

	OSStringSCopy(gsDebugCCB[giOffset].pcMesg, szBuffer,
	              MTGPU_DEBUG_CCB_MESG_MAX);

	giOffset = (giOffset + 1) % MTGPU_DEBUG_CCB_MAX;

	spin_unlock_irqrestore(&gsDebugCCBLock, uiFlags);
}

void MTGPUDebugPrintfDumpCCB(void)
{
	int i;
	unsigned long uiFlags;

	spin_lock_irqsave(&gsDebugCCBLock, uiFlags);

	for (i = 0; i < MTGPU_DEBUG_CCB_MAX; i++)
	{
		MTGPU_DEBUG_CCB *psDebugCCBEntry =
			&gsDebugCCB[(giOffset + i) % MTGPU_DEBUG_CCB_MAX];

		/* Early on, we won't have MTGPU_DEBUG_CCB_MAX messages */
		if (!psDebugCCBEntry->pszFile)
		{
			continue;
		}

		printk(KERN_ERR "%s:%d: (%ld.%ld, tid=%u, pid=%u) %s\n",
			   psDebugCCBEntry->pszFile,
			   psDebugCCBEntry->iLine,
			   (long)psDebugCCBEntry->sTimeVal.tv_sec,
			   (long)psDebugCCBEntry->sTimeVal.tv_usec,
			   psDebugCCBEntry->ui32TID,
			   psDebugCCBEntry->ui32PID,
			   psDebugCCBEntry->pcMesg);

		/* Clear this entry so it doesn't get printed the next time again. */
		psDebugCCBEntry->pszFile = NULL;
	}

	spin_unlock_irqrestore(&gsDebugCCBLock, uiFlags);
}

#else /* defined(MTGPU_DEBUG_CCB_MAX) */

static INLINE void
AddToBufferCCB(const MT_CHAR *pszFileName, MT_UINT32 ui32Line,
			   const MT_CHAR *szBuffer)
{
	(void)pszFileName;
	(void)szBuffer;
	(void)ui32Line;
}

void MTGPUDebugPrintfDumpCCB(void)
{
	/* Not available */
}

#endif /* defined(MTGPU_DEBUG_CCB_MAX) */

static MT_UINT32 gMTDebugLevel =
	(
	 DBGPRIV_FATAL | DBGPRIV_ERROR | DBGPRIV_WARNING
#if defined(MTGPU_DEBUG_CCB_MAX)
	 | DBGPRIV_BUFFERED
#endif /* defined(MTGPU_DEBUG_CCB_MAX) */
#if defined(MT_DPF_ADHOC_DEBUG_ON)
	 | DBGPRIV_DEBUG
#endif /* defined(MT_DPF_ADHOC_DEBUG_ON) */
	);

module_param(gMTDebugLevel, uint, 0644);
MODULE_PARM_DESC(gMTDebugLevel,
                 "Sets the level of debug output (default 0x7)");

MT_UINT32 OSDebugLevel(void)
{
	return gMTDebugLevel;
}

void OSSetDebugLevel(MT_UINT32 ui32DebugLevel)
{
	gMTDebugLevel = ui32DebugLevel;
}

MT_BOOL OSIsDebugLevel(MT_UINT32 ui32DebugLevel)
{
	return (gMTDebugLevel & ui32DebugLevel) != 0;
}

#else /* defined(MTGPU_NEED_MT_DPF) */

MT_UINT32 OSDebugLevel(void)
{
	return 0;
}

void OSSetDebugLevel(MT_UINT32 ui32DebugLevel)
{
	MT_UNREFERENCED_PARAMETER(ui32DebugLevel);
}

MT_BOOL OSIsDebugLevel(MT_UINT32 ui32DebugLevel)
{
	MT_UNREFERENCED_PARAMETER(ui32DebugLevel);
	return MT_FALSE;
}

#endif /* defined(MTGPU_NEED_MT_DPF) */

#define	MT_MAX_MSG_LEN MT_MAX_DEBUG_MESSAGE_LEN

/* Message buffer for messages */
static MT_CHAR gszBuffer[MT_MAX_MSG_LEN + 1];

/* The lock is used to control access to gszBuffer */
static DEFINE_SPINLOCK(gsDebugLock);

/*
 * Append a string to a buffer using formatted conversion.
 * The function takes a variable number of arguments, pointed
 * to by the var args list.
 */
__printf(3, 0)
static MT_BOOL VBAppend(MT_CHAR *pszBuf, MT_UINT32 ui32BufSiz, const MT_CHAR *pszFormat, va_list VArgs)
{
	MT_UINT32 ui32Used;
	MT_UINT32 ui32Space;
	MT_INT32 i32Len;

	ui32Used = OSStringLength(pszBuf);
	BUG_ON(ui32Used >= ui32BufSiz);
	ui32Space = ui32BufSiz - ui32Used;

	i32Len = vsnprintf(&pszBuf[ui32Used], ui32Space, pszFormat, VArgs);
	pszBuf[ui32BufSiz - 1] = 0;

	/* Return true if string was truncated */
	return i32Len < 0 || i32Len >= (MT_INT32)ui32Space;
}

/*************************************************************************/ /*!
@Function       MTGPUReleasePrintf
@Description    To output an important message to the user in release builds
@Input          pszFormat   The message format string
@Input          ...         Zero or more arguments for use by the format string
*/ /**************************************************************************/
void MTGPUReleasePrintf(const MT_CHAR *pszFormat, ...)
{
	va_list vaArgs;
	unsigned long ulLockFlags = 0;
	MT_CHAR *pszBuf = gszBuffer;
	MT_UINT32 ui32BufSiz = sizeof(gszBuffer);
	MT_INT32  result;

	va_start(vaArgs, pszFormat);

	spin_lock_irqsave(&gsDebugLock, ulLockFlags);

	result = snprintf(pszBuf, (ui32BufSiz - 2), "MTGPU:  %u: ", current->pid);
	MT_ASSERT(result>0);
	ui32BufSiz -= result;

	if (VBAppend(pszBuf, ui32BufSiz, pszFormat, vaArgs))
	{
		printk(KERN_INFO "%s (truncated)\n", pszBuf);
	}
	else
	{
		printk(KERN_INFO "%s\n", pszBuf);
	}

	spin_unlock_irqrestore(&gsDebugLock, ulLockFlags);
	va_end(vaArgs);
}

#if defined(MTGPU_NEED_MT_TRACE)

/*************************************************************************/ /*!
@Function       MTTrace
@Description    To output a debug message to the user
@Input          pszFormat   The message format string
@Input          ...         Zero or more arguments for use by the format string
*/ /**************************************************************************/
void MTGPUTrace(const MT_CHAR *pszFormat, ...)
{
	va_list VArgs;
	unsigned long ulLockFlags = 0;
	MT_CHAR *pszBuf = gszBuffer;
	MT_UINT32 ui32BufSiz = sizeof(gszBuffer);
	MT_INT32  result;

	va_start(VArgs, pszFormat);

	spin_lock_irqsave(&gsDebugLock, ulLockFlags);

	result = snprintf(pszBuf, (ui32BufSiz - 2), "MUSA: %u: ", current->pid);
	MT_ASSERT(result>0);
	ui32BufSiz -= result;

	if (VBAppend(pszBuf, ui32BufSiz, pszFormat, VArgs))
	{
		printk(KERN_ERR "MTGPU:(Message Truncated): %s\n", pszBuf);
	}
	else
	{
		printk(KERN_ERR "%s\n", pszBuf);
	}

	spin_unlock_irqrestore(&gsDebugLock, ulLockFlags);

	va_end(VArgs);
}

#endif /* defined(MTGPU_NEED_MT_TRACE) */

#if defined(MTGPU_NEED_MT_DPF)

/*
 * Append a string to a buffer using formatted conversion.
 * The function takes a variable number of arguments, calling
 * VBAppend to do the actual work.
 */
__printf(3, 4)
static MT_BOOL BAppend(MT_CHAR *pszBuf, MT_UINT32 ui32BufSiz, const MT_CHAR *pszFormat, ...)
{
	va_list VArgs;
	MT_BOOL bTrunc;

	va_start (VArgs, pszFormat);

	bTrunc = VBAppend(pszBuf, ui32BufSiz, pszFormat, VArgs);

	va_end (VArgs);

	return bTrunc;
}

/*************************************************************************/ /*!
@Function       MTGPUDebugPrintf
@Description    To output a debug message to the user
@Input          uDebugLevel The current debug level
@Input          pszFile     The source file generating the message
@Input          uLine       The line of the source file
@Input          pszFormat   The message format string
@Input          ...         Zero or more arguments for use by the format string
*/ /**************************************************************************/
void MTGPUDebugPrintf(MT_UINT32 ui32DebugLevel,
			   const MT_CHAR *pszFullFileName,
			   MT_UINT32 ui32Line,
			   const MT_CHAR *pszFormat,
			   ...)
{
	const MT_CHAR *pszFileName = pszFullFileName;
	MT_CHAR *pszLeafName;
	va_list vaArgs;
	unsigned long ulLockFlags = 0;
	MT_CHAR *pszBuf = gszBuffer;
	MT_UINT32 ui32BufSiz = sizeof(gszBuffer);

	if (!(gMTDebugLevel & ui32DebugLevel))
	{
		return;
	}

	va_start(vaArgs, pszFormat);

	spin_lock_irqsave(&gsDebugLock, ulLockFlags);

	switch (ui32DebugLevel)
	{
		case DBGPRIV_FATAL:
		{
			OSStringSCopy(pszBuf, "MTGPU:(Fatal): ", ui32BufSiz);
			MTGPU_REPORT_ERROR();
			break;
		}
		case DBGPRIV_ERROR:
		{
			OSStringSCopy(pszBuf, "MTGPU:(Error): ", ui32BufSiz);
			MTGPU_REPORT_ERROR();
			break;
		}
		case DBGPRIV_WARNING:
		{
			OSStringSCopy(pszBuf, "MTGPU:(Warn):  ", ui32BufSiz);
			break;
		}
		case DBGPRIV_MESSAGE:
		{
			OSStringSCopy(pszBuf, "MTGPU:(Mesg):  ", ui32BufSiz);
			break;
		}
		case DBGPRIV_VERBOSE:
		{
			OSStringSCopy(pszBuf, "MTGPU:(Verb):  ", ui32BufSiz);
			break;
		}
		case DBGPRIV_DEBUG:
		{
			OSStringSCopy(pszBuf, "MTGPU:(Debug): ", ui32BufSiz);
			break;
		}
		case DBGPRIV_CALLTRACE:
		case DBGPRIV_ALLOC:
		case DBGPRIV_BUFFERED:
		default:
		{
			OSStringSCopy(pszBuf, "MTGPU: ", ui32BufSiz);
			break;
		}
	}

	if (current->pid == task_tgid_nr(current))
	{
		(void) BAppend(pszBuf, ui32BufSiz, "%5u: ", current->pid);
	}
	else
	{
		(void) BAppend(pszBuf, ui32BufSiz, "%5u-%5u: ", task_tgid_nr(current) /* pid id of group*/, current->pid /* task id */);
	}

	if (VBAppend(pszBuf, ui32BufSiz, pszFormat, vaArgs))
	{
		printk(KERN_ERR "%s (truncated)\n", pszBuf);
	}
	else
	{
		MT_BOOL bTruncated = MT_FALSE;

#if !defined(__sh__)
		pszLeafName = (MT_CHAR *)strrchr (pszFileName, '/');

		if (pszLeafName)
		{
			pszFileName = pszLeafName+1;
		}
#endif /* __sh__ */

#if defined(DEBUG)
		{
			static const MT_CHAR *lastFile;

			if (lastFile == pszFileName)
			{
				bTruncated = BAppend(pszBuf, ui32BufSiz, " [%u]", ui32Line);
			}
			else
			{
				bTruncated = BAppend(pszBuf, ui32BufSiz, " [%s:%u]", pszFileName, ui32Line);
				lastFile = pszFileName;
			}
		}
#else
		bTruncated = BAppend(pszBuf, ui32BufSiz, " [%u]", ui32Line);
#endif

		if (bTruncated)
		{
			printk(KERN_ERR "%s (truncated)\n", pszBuf);
		}
		else
		{
			if (ui32DebugLevel & DBGPRIV_BUFFERED)
			{
				AddToBufferCCB(pszFileName, ui32Line, pszBuf);
			}
			else
			{
				printk(KERN_ERR "%s\n", pszBuf);
			}
		}
	}

	spin_unlock_irqrestore(&gsDebugLock, ulLockFlags);

	va_end (vaArgs);
}

#endif /* MTGPU_NEED_MT_DPF */

/*************************************************************************/ /*!
@File           img_elf.h
@Title          MT ELF file definitions
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Platform       MUSA
@Description    Definitions for ELF file structures used in the DDK.
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

#if !defined(MT_ELF_H)
#define MT_ELF_H

#include "img_types.h"

/* ELF format defines */
#define ELF_PT_LOAD     (0x1U)   /* Program header identifier as Load */
#define ELF_SHT_SYMTAB  (0x2U)   /* Section identifier as Symbol Table */
#define ELF_SHT_STRTAB  (0x3U)   /* Section identifier as String Table */
#define MAX_STRTAB_NUM  (0x8U)   /* Maximum number of string table in the ELF file */

/* Redefined structs of ELF format */
typedef struct
{
	MT_UINT8    ui32Eident[16];
	MT_UINT16   ui32Etype;
	MT_UINT16   ui32Emachine;
	MT_UINT32   ui32Eversion;
	MT_UINT32   ui32Eentry;
	MT_UINT32   ui32Ephoff;
	MT_UINT32   ui32Eshoff;
	MT_UINT32   ui32Eflags;
	MT_UINT16   ui32Eehsize;
	MT_UINT16   ui32Ephentsize;
	MT_UINT16   ui32Ephnum;
	MT_UINT16   ui32Eshentsize;
	MT_UINT16   ui32Eshnum;
	MT_UINT16   ui32Eshtrndx;
} MT_ELF_HDR;

typedef struct
{
	MT_UINT32   ui32Stname;
	MT_UINT32   ui32Stvalue;
	MT_UINT32   ui32Stsize;
	MT_UINT8    ui32Stinfo;
	MT_UINT8    ui32Stother;
	MT_UINT16   ui32Stshndx;
} MT_ELF_SYM;

typedef struct
{
	MT_UINT32   ui32Shname;
	MT_UINT32   ui32Shtype;
	MT_UINT32   ui32Shflags;
	MT_UINT32   ui32Shaddr;
	MT_UINT32   ui32Shoffset;
	MT_UINT32   ui32Shsize;
	MT_UINT32   ui32Shlink;
	MT_UINT32   ui32Shinfo;
	MT_UINT32   ui32Shaddralign;
	MT_UINT32   ui32Shentsize;
} MT_ELF_SHDR;

typedef struct
{
	MT_UINT32   ui32Ptype;
	MT_UINT32   ui32Poffset;
	MT_UINT32   ui32Pvaddr;
	MT_UINT32   ui32Ppaddr;
	MT_UINT32   ui32Pfilesz;
	MT_UINT32   ui32Pmemsz;
	MT_UINT32   ui32Pflags;
	MT_UINT32   ui32Palign;
} MT_ELF_PROGRAM_HDR;

#endif /* MT_ELF_H */

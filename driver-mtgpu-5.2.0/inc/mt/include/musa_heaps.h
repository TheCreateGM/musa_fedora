/*************************************************************************/ /*!
@File
@Title          MUSA heap definitions
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
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

#if !defined(MUSA_HEAPS_H)
#define MUSA_HEAPS_H

/*
  Identify heaps by their names
*/
#define MUSA_GENERAL_VPU_HEAP_IDENT		"General VPU"		       /*!< MUSA General VPU Heap Identifier */
#define MUSA_GENERAL_SVM_HEAP_IDENT              "General SVM"                  /*!< SVM (shared virtual memory) Heap Identifier */
#define MUSA_GENERAL_HEAP_IDENT                  "General"                      /*!< MUSA General Heap Identifier */
#define MUSA_GENERAL_NON4K_HEAP_IDENT            "General NON-4K"               /*!< MUSA General non-4K Heap Identifier */
#define MUSA_PDSCODEDATA_HEAP_IDENT              "PDS Code and Data"            /*!< MUSA PDS Code/Data Heap Identifier */
#define MUSA_MPCODE_HEAP_IDENT                  "USC Code"                     /*!< MUSA MP Code Heap Identifier */
#define MUSA_VK_CAPT_REPLAY_HEAP_IDENT           "Vulkan Capture Replay"        /*!< MUSA vulkan capture replay buffer Heap Identifier */
#define MUSA_SIGNALS_HEAP_IDENT                  "Signals"                      /*!< Compute Signals Heap Identifier */
#define MUSA_COMPONENT_CTRL_HEAP_IDENT           "Component Control"            /*!< MUSA DCE Component Control Heap Identifier */
#define MUSA_FBCDC_HEAP_IDENT                    "FBCDC"                        /*!< MUSA FBCDC State Table Heap Identifier */
#define MUSA_FBCDC_LARGE_HEAP_IDENT              "Large FBCDC"                  /*!< MUSA Large FBCDC State Table Heap Identifier */
#define MUSA_PDS_INDIRECT_STATE_HEAP_IDENT       "PDS Indirect State"           /*!< PDS Indirect State Table Heap Identifier */
#define MUSA_CMP_MISSION_RMW_HEAP_IDENT          "Compute Mission RMW"          /*!< Compute Mission RMW Heap Identifier */
#define MUSA_CMP_SAFETY_RMW_HEAP_IDENT           "Compute Safety RMW"           /*!< Compute Safety RMW Heap Identifier */
#define MUSA_TEXTURE_STATE_HEAP_IDENT            "Texture State"                /*!< Texture State Heap Identifier */
#define MUSA_VISIBILITY_TEST_HEAP_IDENT          "Visibility Test"              /*!< Visibility Test Heap Identifier */
#define MUSA_GENERAL_RESERVED_HEAP_IDENT         "General Reserved"             /*!< MUSA General Reserved Heap Identifier */
#define MUSA_PDSCODEDATA_RESERVED_HEAP_IDENT     "PDS Reserved"                 /*!< MUSA PDS Code/Data Reserved Heap Identifier */
#define MUSA_MPCODE_RESERVED_HEAP_IDENT         "USC Reserved"                 /*!< MUSA MP Code Reserved Heap Identifier */
#define MUSA_COMPONENT_CTRL_RESERVED_HEAP_IDENT  "Component Reserved"           /*!< MUSA COMPONENT Ctrl Reserved Heap Identifier */
#define MUSA_GENERAL_RESERVED_CSC_HEAP_IDENT     "General Reserved CSC Table"   /*!< MUSA YUV CSC TABLE Reserved Heap Identifier */
#define MUSA_GENERAL_RESERVED_VPU_HEAP_IDENT	"General Reserved VPU"	       /*!< MUSA General Reserved VPU Heap Identifier */

enum {
	MUSA_FIRMWARE_MAIN_HEAP_ID = 0,
	MUSA_FIRMWARE_CONFIG_HEAP_ID,
	MUSA_FIRMWARE_GUEST_RAW_HEAP_ID,
	MUSA_GENERAL_SVM_HEAP_ID,
	MUSA_GENERAL_HEAP_ID,
	MUSA_GENERAL_NON4K_HEAP_ID,
	MUSA_PDSCODEDATA_HEAP_ID,
	MUSA_MPCODE_HEAP_ID,
	MUSA_VK_CAPT_REPLAY_HEAP_ID,
	MUSA_SIGNALS_HEAP_ID,
	MUSA_COMPONENT_CTRL_HEAP_ID,
	MUSA_FBCDC_HEAP_ID,
	MUSA_FBCDC_LARGE_HEAP_ID,
	MUSA_PDS_INDIRECT_STATE_HEAP_ID,
	MUSA_CMP_MISSION_RMW_HEAP_ID,
	MUSA_CMP_SAFETY_RMW_HEAP_ID,
	MUSA_TEXTURE_STATE_HEAP_ID,
	MUSA_VISIBILITY_TEST_HEAP_ID,
	MUSA_GENERAL_VPU_HEAP_ID,
	MUSA_GENERAL_RESERVED_HEAP_ID,
	MUSA_PDSCODEDATA_RESERVED_HEAP_ID,
	MUSA_MPCODE_RESERVED_HEAP_ID,
	MUSA_COMPONENT_CTRL_RESERVED_HEAP_ID,
	MUSA_GENERAL_RESERVED_CSC_HEAP_ID,
	MUSA_GENERAL_RESERVED_VPU_HEAP_ID,
	MUSA_INVALID_HEAP_ID,
};

#endif /* MUSA_HEAPS_H */

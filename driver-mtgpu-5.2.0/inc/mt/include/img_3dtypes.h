/*************************************************************************/ /*!
@File
@Title          Global 3D types for use by MT APIs
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Defines 3D types for use by MT APIs
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

#ifndef MT_3DTYPES_H
#define MT_3DTYPES_H

#include <mt/buffer_attribs.h>
#include "img_types.h"
#include "img_defs.h"

/**
 * Comparison functions
 * This comparison function is defined as:
 * A {CmpFunc} B
 * A is a reference value, e.g., incoming depth etc.
 * B is the sample value, e.g., value in depth buffer.
 */
typedef enum _MT_COMPFUNC_
{
	MT_COMPFUNC_NEVER,			/**< The comparison never succeeds */
	MT_COMPFUNC_LESS,			/**< The comparison is a less-than operation */
	MT_COMPFUNC_EQUAL,			/**< The comparison is an equal-to operation */
	MT_COMPFUNC_LESS_EQUAL,	/**< The comparison is a less-than or equal-to
									 operation */
	MT_COMPFUNC_GREATER,		/**< The comparison is a greater-than operation
								*/
	MT_COMPFUNC_NOT_EQUAL,		/**< The comparison is a no-equal-to operation
								*/
	MT_COMPFUNC_GREATER_EQUAL,	/**< The comparison is a greater-than or
									 equal-to operation */
	MT_COMPFUNC_ALWAYS,		/**< The comparison always succeeds */
} MT_COMPFUNC;

/**
 * Stencil op functions
 */
typedef enum _MT_STENCILOP_
{
	MT_STENCILOP_KEEP,		/**< Keep original value */
	MT_STENCILOP_ZERO,		/**< Set stencil to 0 */
	MT_STENCILOP_REPLACE,	/**< Replace stencil entry */
	MT_STENCILOP_INCR_SAT,	/**< Increment stencil entry, clamping to max */
	MT_STENCILOP_DECR_SAT,	/**< Decrement stencil entry, clamping to zero */
	MT_STENCILOP_INVERT,	/**< Invert bits in stencil entry */
	MT_STENCILOP_INCR,		/**< Increment stencil entry,
								 wrapping if necessary */
	MT_STENCILOP_DECR,		/**< Decrement stencil entry,
								 wrapping if necessary */
} MT_STENCILOP;

/**
 * Alpha blending allows colours and textures on one surface
 * to be blended with transparency onto another surface.
 * These definitions apply to both source and destination blending
 * states
 */
typedef enum _MT_BLEND_
{
	MT_BLEND_ZERO = 0,        /**< Blend factor is (0,0,0,0) */
	MT_BLEND_ONE,             /**< Blend factor is (1,1,1,1) */
	MT_BLEND_SRC_COLOUR,      /**< Blend factor is the source colour */
	MT_BLEND_INV_SRC_COLOUR,  /**< Blend factor is the inverted source colour
									(i.e. 1-src_col) */
	MT_BLEND_SRC_ALPHA,       /**< Blend factor is the source alpha */
	MT_BLEND_INV_SRC_ALPHA,   /**< Blend factor is the inverted source alpha
									(i.e. 1-src_alpha) */
	MT_BLEND_DEST_ALPHA,      /**< Blend factor is the destination alpha */
	MT_BLEND_INV_DEST_ALPHA,  /**< Blend factor is the inverted destination
									alpha */
	MT_BLEND_DEST_COLOUR,     /**< Blend factor is the destination colour */
	MT_BLEND_INV_DEST_COLOUR, /**< Blend factor is the inverted destination
									colour */
	MT_BLEND_SRC_ALPHASAT,    /**< Blend factor is the alpha saturation (the
									minimum of (Src alpha,
									1 - destination alpha)) */
	MT_BLEND_BLEND_FACTOR,    /**< Blend factor is a constant */
	MT_BLEND_INVBLEND_FACTOR, /**< Blend factor is a constant (inverted)*/
	MT_BLEND_SRC1_COLOUR,     /**< Blend factor is the colour outputted from
									the pixel shader */
	MT_BLEND_INV_SRC1_COLOUR, /**< Blend factor is the inverted colour
									outputted from the pixel shader */
	MT_BLEND_SRC1_ALPHA,      /**< Blend factor is the alpha outputted from
									the pixel shader */
	MT_BLEND_INV_SRC1_ALPHA   /**< Blend factor is the inverted alpha
									outputted from the pixel shader */
} MT_BLEND;

/**
 * The arithmetic operation to perform when blending
 */
typedef enum _MT_BLENDOP_
{
	MT_BLENDOP_ADD = 0,      /**< Result = (Source + Destination) */
	MT_BLENDOP_SUBTRACT,     /**< Result = (Source - Destination) */
	MT_BLENDOP_REV_SUBTRACT, /**< Result = (Destination - Source) */
	MT_BLENDOP_MIN,          /**< Result = min (Source, Destination) */
	MT_BLENDOP_MAX           /**< Result = max (Source, Destination) */
} MT_BLENDOP;

/**
 * Logical operation to perform when logic ops are enabled
 */
typedef enum _MT_LOGICOP_
{
	MT_LOGICOP_CLEAR = 0,     /**< Result = 0 */
	MT_LOGICOP_SET,           /**< Result = -1 */
	MT_LOGICOP_COPY,          /**< Result = Source */
	MT_LOGICOP_COPY_INVERTED, /**< Result = ~Source */
	MT_LOGICOP_NOOP,          /**< Result = Destination */
	MT_LOGICOP_INVERT,        /**< Result = ~Destination */
	MT_LOGICOP_AND,           /**< Result = Source & Destination */
	MT_LOGICOP_NAND,          /**< Result = ~(Source & Destination) */
	MT_LOGICOP_OR,            /**< Result = Source | Destination */
	MT_LOGICOP_NOR,           /**< Result = ~(Source | Destination) */
	MT_LOGICOP_XOR,           /**< Result = Source ^ Destination */
	MT_LOGICOP_EQUIV,         /**< Result = ~(Source ^ Destination) */
	MT_LOGICOP_AND_REVERSE,   /**< Result = Source & ~Destination */
	MT_LOGICOP_AND_INVERTED,  /**< Result = ~Source & Destination */
	MT_LOGICOP_OR_REVERSE,    /**< Result = Source | ~Destination */
	MT_LOGICOP_OR_INVERTED    /**< Result = ~Source | Destination */
} MT_LOGICOP;

/**
 * Type of fog blending supported
 */
typedef enum _MT_FOGMODE_
{
	MT_FOGMODE_NONE, /**< No fog blending - fog calculations are
					   *   based on the value output from the vertex phase */
	MT_FOGMODE_LINEAR, /**< Linear interpolation */
	MT_FOGMODE_EXP, /**< Exponential */
	MT_FOGMODE_EXP2, /**< Exponential squaring */
} MT_FOGMODE;

/**
 * Types of filtering
 */
typedef enum _MT_FILTER_
{
	MT_FILTER_DONTCARE,	/**< Any filtering mode is acceptable */
	MT_FILTER_POINT,		/**< Point filtering */
	MT_FILTER_LINEAR,		/**< Bi-linear filtering */
	MT_FILTER_BICUBIC,		/**< Bi-cubic filtering */
} MT_FILTER;

/**
 * Addressing modes for textures
 */
typedef enum _MT_ADDRESSMODE_
{
	MT_ADDRESSMODE_REPEAT,	/**< Texture repeats continuously */
	MT_ADDRESSMODE_FLIP, /**< Texture flips on odd integer part */
	MT_ADDRESSMODE_CLAMP, /**< Texture clamped at 0 or 1 */
	MT_ADDRESSMODE_FLIPCLAMP, /**< Flipped once, then clamp */
	MT_ADDRESSMODE_CLAMPBORDER,
	MT_ADDRESSMODE_OGL_CLAMP,
	MT_ADDRESSMODE_OVG_TILEFILL,
	MT_ADDRESSMODE_DONTCARE,
} MT_ADDRESSMODE;

/**
 * Culling based on winding order of triangle.
 */
typedef enum _MT_CULLMODE_
{
	MT_CULLMODE_NONE,			/**< Don't cull */
	MT_CULLMODE_FRONTFACING,	/**< Front facing triangles */
	MT_CULLMODE_BACKFACING,	/**< Back facing triangles */
} MT_CULLMODE;

/**
 * Colour for clearing surfaces.
 *  The four elements of the 4 x 32 bit array will map to colour
 *  R,G,B,A components, in order.
 *  For YUV colour space the order is Y,U,V.
 *  For Depth and Stencil formats D maps to R and S maps to G.
 */
typedef union MT_CLEAR_COLOUR_TAG {
	MT_UINT32        aui32[4];
	MT_INT32         ai32[4];
	MT_FLOAT         af32[4];
} MT_CLEAR_COLOUR;

static_assert(sizeof(MT_FLOAT) == sizeof(MT_INT32), "Size of MT_FLOAT is not 32 bits.");

/*! ************************************************************************//**
@brief          Specifies the MSAA resolve operation.
*/ /**************************************************************************/
typedef enum _MT_RESOLVE_OP_
{
	MT_RESOLVE_BLEND   = 0,          /*!< box filter on the samples */
	MT_RESOLVE_MIN     = 1,          /*!< minimum of the samples */
	MT_RESOLVE_MAX     = 2,          /*!< maximum of the samples */
	MT_RESOLVE_SAMPLE0 = 3,          /*!< choose sample 0 */
	MT_RESOLVE_SAMPLE1 = 4,          /*!< choose sample 1 */
	MT_RESOLVE_SAMPLE2 = 5,          /*!< choose sample 2 */
	MT_RESOLVE_SAMPLE3 = 6,          /*!< choose sample 3 */
	MT_RESOLVE_SAMPLE4 = 7,          /*!< choose sample 4 */
	MT_RESOLVE_SAMPLE5 = 8,          /*!< choose sample 5 */
	MT_RESOLVE_SAMPLE6 = 9,          /*!< choose sample 6 */
	MT_RESOLVE_SAMPLE7 = 10,         /*!< choose sample 7 */
} MT_RESOLVE_OP;


#endif /* MT_3DTYPES_H */
/******************************************************************************
 End of file (img_3dtypes.h)
******************************************************************************/

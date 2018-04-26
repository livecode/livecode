/* Copyright (C) 2003-2015 LiveCode Ltd.

This file is part of LiveCode.

LiveCode is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License v3 as published by the Free
Software Foundation.

LiveCode is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with LiveCode.  If not see <http://www.gnu.org/licenses/>.  */

#include "prefix.h"

#ifdef __VISUALC__
#pragma optimize("agt", on)
#pragma optimize("y", off)
#endif

typedef unsigned int uint32_t;
typedef unsigned short uint16_t; 
typedef unsigned char uint8_t;

static uint32_t g_current_background_colour = 0;

enum BitwiseOperation
{
	// Bitwise
	OPERATION_CLEAR,
	OPERATION_AND,
	OPERATION_AND_REVERSE,
	OPERATION_COPY,
	OPERATION_AND_INVERTED,
	OPERATION_NOOP,
	OPERATION_XOR,
	OPERATION_OR,
	OPERATION_NOR,
	OPERATION_EQUIV,
	OPERATION_INVERT,
	OPERATION_OR_REVERSE,
	OPERATION_COPY_INVERTED,
	OPERATION_OR_INVERTED,
	OPERATION_NAND,
	OPERATION_SET,
    
    LAST_BITWISE_OPERATION = OPERATION_SET,
};

enum ArithmeticOperation
{
	// Arithmetic
	OPERATION_BLEND = LAST_BITWISE_OPERATION + 1,
	OPERATION_ADD_PIN,
	OPERATION_ADD_OVER,
	OPERATION_SUB_PIN,
	OPERATION_TRANSPARENT,
	OPERATION_AD_MAX,
	OPERATION_SUB_OVER,
	OPERATION_AD_MIN,
    
    LAST_ARITHMETIC_OPERATION = OPERATION_AD_MIN,
};

enum BasicImagingOperation
{
	// Basic Imaging Blends
	OPERATION_BLEND_CLEAR = LAST_ARITHMETIC_OPERATION + 1,
	OPERATION_BLEND_SRC,
	OPERATION_BLEND_DST,
	OPERATION_BLEND_SRC_OVER,
	OPERATION_BLEND_DST_OVER,
	OPERATION_BLEND_SRC_IN,
	OPERATION_BLEND_DST_IN,
	OPERATION_BLEND_SRC_OUT,
	OPERATION_BLEND_DST_OUT,
	OPERATION_BLEND_SRC_ATOP,
	OPERATION_BLEND_DST_ATOP,
	OPERATION_BLEND_XOR,
	OPERATION_BLEND_PLUS,
	OPERATION_BLEND_MULTIPLY,
	OPERATION_BLEND_SCREEN,
    
    LAST_BASIC_IMAGING_OPERATION = OPERATION_BLEND_SCREEN,
};

enum AdvancedImagingOperation
{
	// Advanced Imaging Blends
	OPERATION_BLEND_OVERLAY = LAST_BASIC_IMAGING_OPERATION + 1,
	OPERATION_BLEND_DARKEN,
	OPERATION_BLEND_LIGHTEN,
	OPERATION_BLEND_DODGE,
	OPERATION_BLEND_BURN,
	OPERATION_BLEND_HARD_LIGHT,
	OPERATION_BLEND_SOFT_LIGHT,
	OPERATION_BLEND_DIFFERENCE,
	OPERATION_BLEND_EXCLUSION,
    
    LAST_ADVANCED_IMAGING_OPERATION = OPERATION_BLEND_EXCLUSION,
};

#define OPERATION_SRC_BIC OPERATION_AND_REVERSE
#define OPERATION_NOT_SRC_BIC OPERATION_AND

typedef void (*surface_combiner_t)(void *p_dst, int32_t p_dst_stride, const void *p_src, uint32_t p_src_stride, uint32_t p_width, uint32_t p_height, uint8_t p_opacity);

#ifdef __VISUALC__
#define INLINE __forceinline
#else
#define INLINE inline
#endif

static INLINE uint32_t _combine(uint32_t u, uint32_t v)
{
	u += 0x800080;
	v += 0x800080;
	return (((u + ((u >> 8) & 0xff00ff)) >> 8) & 0xff00ff) + (((v + ((v >> 8) & 0xff00ff))) & 0xff00ff00);
}

static INLINE uint32_t _low(uint32_t x)
{
	return x & 0xff00ff;
}

static INLINE uint32_t _high(uint32_t x)
{
	return (x >> 8) & 0xff00ff;
}

static INLINE uint32_t _scale(uint32_t x, uint8_t a)
{
	return x * a;
}

static INLINE uint32_t _scale_low(uint32_t x, uint8_t a)
{
	return _scale(_low(x), a);
}

static INLINE uint32_t _scale_high(uint32_t x, uint32_t a)
{
	return _scale(_high(x), a);
}

static INLINE uint32_t _multiply_low(uint32_t x, uint32_t y)
{
	return ((x & 0xff) * (y & 0xff)) | ((x & 0xff0000) * ((y >> 16) & 0xff));
}

static INLINE uint32_t _multiply_high(uint32_t x, uint32_t y)
{
	x = x >> 8;
	return ((x & 0xff) * ((y >> 8) & 0xff)) | ((x & 0xff0000) * (y >> 24));
}

// r_i = (x_i * a) / 255
static INLINE uint32_t packed_scale_bounded(uint32_t x, uint8_t a)
{
	uint32_t u, v;

	u = ((x & 0xff00ff) * a) + 0x800080;
	u = ((u + ((u >> 8) & 0xff00ff)) >> 8) & 0xff00ff;

	v = (((x >> 8) & 0xff00ff) * a) + 0x800080;
	v = (v + ((v >> 8) & 0xff00ff)) & 0xff00ff00;

	return u + v;
}

// r_i = x_i + a
static INLINE uint32_t packed_translate_bounded(uint32_t x, uint8_t a)
{
	uint32_t y;
	y = a | a << 8;
	y |= y << 16;
	return x + y;
}

// r_i = x_i + y_i
static INLINE uint32_t packed_add_bounded(uint32_t x, uint32_t y)
{
	return x + y;
}

// r_i = x_i * y_i / 255;
static INLINE uint32_t packed_multiply_bounded(uint32_t x, uint32_t y)
{
	return _combine(_multiply_low(x, y), _multiply_high(x, y));
}

// r_i = (x_i * a) / 255 + y_i
static INLINE uint32_t packed_scale_add_bounded(uint32_t x, uint8_t a, uint32_t y)
{
	return packed_add_bounded(packed_scale_bounded(x, a), y);
}

// r_i = (x_i * a + y_i * b) / 255
static INLINE uint32_t packed_bilinear_bounded(uint32_t x, uint8_t a, uint32_t y, uint8_t b)
{
	uint32_t u, v;

	u = (x & 0xff00ff) * a + (y & 0xff00ff) * b + 0x800080;
	u = ((u + ((u >> 8) & 0xff00ff)) >> 8) & 0xff00ff;

	v = ((x >> 8) & 0xff00ff) * a + ((y >> 8) & 0xff00ff) * b + 0x800080;
	v = (v + ((v >> 8) & 0xff00ff)) & 0xff00ff00;

	return u | v;
}

static INLINE uint32_t packed_add_saturated(uint32_t x, uint32_t y)
{
	uint32_t u, v;
	
	u = _low(x) + _low(y);
	u = (u | (0x10000100 - ((u >> 8) & 0xff00ff))) & 0xff00ff;

	v = _high(x) + _high(y);
	v = (v | (0x10000100 - ((v >> 8) & 0xff00ff))) & 0xff00ff;

	return u | (v << 8);
}

static INLINE uint32_t packed_subtract_saturated(uint32_t x, uint32_t y)
{
	uint32_t u, v;

	u = _low(x) - _low(y);
	u = (u & ~((u >> 8) & 0xff00ff)) & 0xff00ff;

	v = _high(x) - _high(y);
	v = (v & ~((v >> 8) & 0xff00ff)) & 0xff00ff;

	return u | (v << 8);
}

static INLINE uint32_t packed_divide_bounded(uint32_t x, uint8_t a)
{
	uint32_t u, v, w;
	u = ((((x & 0xff0000) << 8) - (x & 0xff0000)) / a) & 0xff0000;
	v = ((((x & 0x00ff00) << 8) - (x & 0x00ff00)) / a) & 0x00ff00;
	w = ((((x & 0x0000ff) << 8) - (x & 0x0000ff)) / a) & 0x0000ff;
	return u | v | w;
}

static INLINE uint32_t packed_inverse(uint32_t x)
{
	return ~x;
}

static INLINE uint8_t packed_alpha(uint32_t x)
{
	return x >> 24;
}

static INLINE uint8_t packed_inverse_alpha(uint32_t x)
{
	return (~x) >> 24;
}

static INLINE uint8_t saturated_add(uint8_t a, uint8_t b)
{
	if (a + b > 255)
		return 255;

	return a + b;
}

static INLINE uint8_t inverse(uint8_t a)
{
	return 255 - a;
}

static INLINE uint16_t upscale(uint8_t a)
{
	return (a << 8) - a;
}

static INLINE uint32_t upscale(uint16_t a)
{
	return (a << 8) - a;
}

static INLINE uint8_t downscale(uint16_t a)
{
	uint32_t r = ((a + 0x80) + ((a + 0x80) >> 8)) >> 8;
	return uint8_t(r);
}

static INLINE uint16_t scaled_divide(uint16_t n, uint8_t s, uint8_t d)
{
	return d != 0 ? uint16_t((n * s) / d) : 0;
}

static INLINE uint16_t long_scaled_divide(uint32_t n, uint8_t s, uint16_t d)
{
	return d != 0 ? uint16_t((n * s) / d) : 0;
}

static INLINE uint8_t sqrt(uint16_t n)
{
	return 1;
}

template<typename Type> INLINE Type fastmin(Type a, Type b)
{
	return a > b ? b : a;
}

template<typename Type> INLINE Type fastmax(Type a, Type b)
{
	return a < b ? b : a;
}

template<BitwiseOperation x_combiner, bool x_dst_alpha, bool x_src_alpha> INLINE uint32_t bitwise_combiner(uint32_t dst, uint32_t src)
{
	uint8_t sa, da;
	uint32_t r, s, d;

	if (x_dst_alpha)
		da = packed_alpha(dst);
	else
		da = 255;

	if (x_src_alpha)
		sa = packed_alpha(src);
	else
		sa = 255;

	if (sa == 0)
		return dst;

	if (da == 0)
		return src;

	if (sa != 255)
		s = packed_divide_bounded(src, sa);
	else
		s = src;

	if (da != 255)
		d = packed_divide_bounded(dst, da);
	else
		d = dst;

	switch(x_combiner)
	{
	case OPERATION_CLEAR:
		r = 0x00000000;
	break;
	case OPERATION_AND:
		r = s & d;
	break;
	case OPERATION_AND_REVERSE:
		r = (s & ~d) & 0xffffff;
	break;
	case OPERATION_COPY:
		r = s;
	break;
	case OPERATION_AND_INVERTED:
		r = (~s & d) & 0xffffff;
	break;
	case OPERATION_NOOP:
		r = d;
	break;
	case OPERATION_XOR:
		r = s ^ d;
	break;
	case OPERATION_OR:
		r = s | d;
	break;
	case OPERATION_NOR:
		r = (~(s | d)) & 0xffffff;
	break;
	case OPERATION_EQUIV:
		r = (~s ^ d) & 0xffffff;
	break;
	case OPERATION_INVERT:
		r = ~d & 0xffffff;
	break;
	case OPERATION_OR_REVERSE:
		r = (s | ~d) & 0xffffff;
	break;
	case OPERATION_COPY_INVERTED:
		r = (~s) & 0xffffff;
	break;
	case OPERATION_OR_INVERTED:
		r = (~s | d) & 0xffffff;
	break;
	case OPERATION_NAND:
		r = (~(s & d)) & 0xffffff;
	break;
	case OPERATION_SET:
		r = 0x00ffffff;
	break;
    default:
        MCUnreachableReturn(0);
	}

	if (x_src_alpha && x_dst_alpha)
	{
		uint8_t ra;
		ra = downscale(sa * da);
		r = packed_bilinear_bounded(src, 255 - da, dst, 255 - sa) + packed_scale_bounded(r | 0xff000000, ra);
	}
	else if (!x_src_alpha && x_dst_alpha)
		r = packed_bilinear_bounded(src, 255 - da, r | 0xff000000, da);
	else if (!x_dst_alpha && x_src_alpha)
		r = packed_bilinear_bounded(dst, 255 - sa, r | 0xff000000, sa);

	return r;
}

extern uint32_t g_current_background_colour;
template<ArithmeticOperation x_combiner, bool x_dst_alpha, bool x_src_alpha> INLINE uint32_t arithmetic_combiner(uint32_t dst, uint32_t src)
{
	uint8_t sa, da;
	uint32_t s, d;

	if (x_dst_alpha)
		da = packed_alpha(dst);
	else
		da = 255;

	if (x_src_alpha)
		sa = packed_alpha(src);
	else
		sa = 255;

	if (sa == 0)
		return dst;

	if (da == 0)
		return src;

	if (sa != 255)
		s = packed_divide_bounded(src, sa);
	else
		s = src;

	if (da != 255)
		d = packed_divide_bounded(dst, da);
	else
		d = dst;

	uint32_t r;
	switch(x_combiner)
	{
	case OPERATION_BLEND:
	{
		uint32_t u, v;
		u = (((d & 0xff00ff) + (s & 0xff00ff)) >> 1) & 0xff00ff;
		v = (((d & 0x00ff00) + (s & 0x00ff00)) >> 1) & 0x00ff00;
		r = u | v;
	}
	break;
	case OPERATION_ADD_PIN:
	{
		uint32_t u, v;
		u = (d & 0xff00ff) + (s & 0xff00ff);
		u = (u | (0x1000100 - ((u >> 8) & 0xff00ff))) & 0xff00ff;
		v = (d & 0x00ff00) + (s & 0x00ff00);
		v = (v | (0x0010000 - ((v >> 8) & 0x00ff00))) & 0x00ff00;
		r = u | v;
	}
	break;
	case OPERATION_ADD_OVER:
	{
		uint32_t u, v;
		u = ((d & 0xff00ff) + (s & 0xff00ff)) & 0xff00ff;
		v = ((d & 0x00ff00) + (s & 0x00ff00)) & 0x00ff00;
		r = u | v;
	}
	break;
	case OPERATION_SUB_PIN:
	{
		uint32_t u, v;
		u = (s & 0xff00ff) - (d & 0xff00ff);
		u = (u & (((u >> 8) & 0xff00ff) - 0x100101)) & 0xff00ff;
		v = (s & 0x00ff00) - (d & 0x00ff00);
		v = (v & (((v >> 8) & 0x00ff00) - 0x0010100)) & 0x00ff00;
		r = u | v;
	}
	break;
	case OPERATION_TRANSPARENT:
		r = (s == g_current_background_colour) ? d : s;
	break;
	case OPERATION_AD_MAX:
	{
		uint32_t sr, sg, sb;
		sr = s & 0x0000ff; sg = s & 0x00ff00; sb = s & 0xff0000;

		uint32_t dr, dg, db;
		dr = d & 0x0000ff; dg = d & 0x00ff00; db = d & 0xff0000;

		uint32_t rr, rg, rb;
		rr = sr > dr ? sr : dr;
		rg = sg > dg ? sg : dg;
		rb = sb > db ? sb : db;

		r = rr | rg | rb;
	}
	break;
	case OPERATION_SUB_OVER:
	{
		uint32_t u, v;
		u = ((s & 0xff00ff) - (d & 0xff00ff)) & 0xff00ff;
		v = ((s & 0x00ff00) - (d & 0x00ff00)) & 0x00ff00;
		r = u | v;
	}
	break;
	case OPERATION_AD_MIN:
	{
		uint32_t sr, sg, sb;
		sr = s & 0x0000ff; sg = s & 0x00ff00; sb = s & 0xff0000;

		uint32_t dr, dg, db;
		dr = d & 0x0000ff; dg = d & 0x00ff00; db = d & 0xff0000;

		uint32_t rr, rg, rb;
		rr = sr < dr ? sr : dr;
		rg = sg < dg ? sg : dg;
		rb = sb < db ? sb : db;

		r = rr | rg | rb;
	}
	break;
    default:
        MCUnreachableReturn(0);
	}

	if (x_src_alpha && x_dst_alpha)
	{
		uint8_t ra;
		ra = downscale(sa * da);
		r = packed_bilinear_bounded(src, 255 - da, dst, 255 - sa) + packed_scale_bounded(r | 0xff000000, ra);
	}
	else if (!x_src_alpha && x_dst_alpha)
		r = packed_bilinear_bounded(src, 255 - da, r | 0xff000000, da);
	else if (!x_dst_alpha && x_src_alpha)
		r = packed_bilinear_bounded(dst, 255 - sa, r | 0xff000000, sa);

	return r;
}

template<BasicImagingOperation x_combiner, bool x_dst_alpha, bool x_src_alpha> INLINE uint32_t basic_imaging_combiner(uint32_t dst, uint32_t src)
{
	uint32_t r;
	switch(x_combiner)
	{
	case OPERATION_BLEND_CLEAR:
		r = 0;
	break;
	case OPERATION_BLEND_SRC:
		r = src;
	break;
	case OPERATION_BLEND_DST:
		r = dst;
	break;
	case OPERATION_BLEND_SRC_OVER:
		if (x_src_alpha)
			r = packed_scale_add_bounded(dst, packed_inverse_alpha(src), src);
		else
			r = src;
	break;
	case OPERATION_BLEND_DST_OVER:
		if (x_dst_alpha)
			r = packed_scale_add_bounded(src, packed_inverse_alpha(dst), dst);
		else
			r = dst;
	break;
	case OPERATION_BLEND_SRC_IN:
		if (x_dst_alpha)
			r = packed_scale_bounded(src, packed_alpha(dst));
		else
			r = src;
	break;
	case OPERATION_BLEND_DST_IN:
		if (x_src_alpha)
			r = packed_scale_bounded(dst, packed_alpha(src));
		else
			r = dst;
	break;
	case OPERATION_BLEND_SRC_OUT:
		if (x_dst_alpha)
			r = packed_scale_bounded(src, packed_inverse_alpha(dst));
		else
			r = 0;
	break;
	case OPERATION_BLEND_DST_OUT:
		if (x_src_alpha)
			r = packed_scale_bounded(dst, packed_inverse_alpha(src));
		else
			r = 0;
	break;
	case OPERATION_BLEND_SRC_ATOP:
		if (x_dst_alpha && x_src_alpha)
			r = packed_bilinear_bounded(src, packed_alpha(dst), dst, packed_inverse_alpha(src));
		else if (!x_dst_alpha && x_src_alpha)
			r = packed_scale_add_bounded(dst, packed_inverse_alpha(src), src);
		else if (!x_src_alpha && x_dst_alpha)
			r = packed_scale_bounded(src, packed_alpha(dst));
		else
			r = src;
	break;
	case OPERATION_BLEND_DST_ATOP:
		if (x_dst_alpha && x_src_alpha)
			r = packed_bilinear_bounded(dst, packed_alpha(src), src, packed_inverse_alpha(dst));
		else if (!x_src_alpha && x_dst_alpha)
			r = packed_scale_add_bounded(src, packed_inverse_alpha(dst), dst);
		else if (!x_dst_alpha && x_src_alpha)
			r = packed_scale_bounded(dst, packed_alpha(src));
		else
			r = dst;
	break;
	case OPERATION_BLEND_XOR:
		if (x_dst_alpha && x_src_alpha)
			r = packed_bilinear_bounded(src, packed_inverse_alpha(dst), dst, packed_inverse_alpha(src));
		else if (!x_dst_alpha && x_src_alpha)
			r = packed_scale_bounded(dst, packed_inverse_alpha(src));
		else if (!x_src_alpha && x_dst_alpha)
			r = packed_scale_bounded(src, packed_inverse_alpha(dst));
		else
			r = 0;
	break;
	case OPERATION_BLEND_PLUS:
		r = packed_add_saturated(src, dst);
	break;
	case OPERATION_BLEND_MULTIPLY:
		if (x_dst_alpha && x_src_alpha)
			r = packed_multiply_bounded(src, dst) + packed_bilinear_bounded(src, packed_inverse_alpha(dst), dst, packed_inverse_alpha(src));
		else if (!x_dst_alpha && x_src_alpha)
			r = packed_multiply_bounded(src, dst) + packed_scale_bounded(dst, packed_inverse_alpha(src));
		else if (!x_src_alpha && x_dst_alpha)
			r = packed_multiply_bounded(src, dst) + packed_scale_bounded(src, packed_inverse_alpha(dst));
		else
			r = packed_multiply_bounded(src, dst);
	break;
	case OPERATION_BLEND_SCREEN:
		r = packed_multiply_bounded(src, packed_inverse(dst)) + dst;
	break;
    default:
        MCUnreachableReturn(0);
	}

	return r;
}

template<AdvancedImagingOperation x_combiner, bool x_dst_alpha, bool x_src_alpha> INLINE uint32_t advanced_imaging_combiner(uint32_t dst, uint32_t src)
{
	uint8_t t_src_red, t_src_green, t_src_blue, t_src_alpha;
	uint8_t t_dst_red, t_dst_green, t_dst_blue, t_dst_alpha;
	uint16_t t_dst_alpha_src_red, t_dst_alpha_src_green, t_dst_alpha_src_blue, t_dst_alpha_src_alpha, t_dst_alpha_dst_alpha;
	uint16_t t_dst_alpha_dst_red, t_dst_alpha_dst_green, t_dst_alpha_dst_blue;
	uint16_t t_src_alpha_dst_red, t_src_alpha_dst_green, t_src_alpha_dst_blue, t_src_alpha_dst_alpha;
	uint16_t t_inv_dst_alpha_src_red, t_inv_dst_alpha_src_green, t_inv_dst_alpha_src_blue, t_inv_dst_alpha_src_alpha ATTRIBUTE_UNUSED;
	uint16_t t_inv_src_alpha_dst_red, t_inv_src_alpha_dst_green, t_inv_src_alpha_dst_blue, t_inv_src_alpha_dst_alpha ATTRIBUTE_UNUSED;
	uint32_t t_dst_alpha_dst_alpha_src_alpha;
	uint8_t t_red, t_green, t_blue, t_alpha;

	t_src_red = src & 0xff;
	t_src_green = (src >> 8) & 0xff;
	t_src_blue = (src >> 16) & 0xff;
	if (x_src_alpha)
		t_src_alpha = src >> 24;
	else
		t_src_alpha = 255;

	t_dst_red = dst & 0xff;
	t_dst_green = (dst >> 8) & 0xff;
	t_dst_blue = (dst >>16) & 0xff;
	if (x_dst_alpha)
		t_dst_alpha = dst >> 24;
	else
		t_dst_alpha = 255;

	if (t_src_alpha == 0)
	{
		t_src_alpha_dst_red = 0;
		t_src_alpha_dst_green = 0;
		t_src_alpha_dst_blue = 0;
		t_src_alpha_dst_alpha = 0;

		t_inv_src_alpha_dst_red = upscale(t_dst_red);
		t_inv_src_alpha_dst_green = upscale(t_dst_green);
		t_inv_src_alpha_dst_blue = upscale(t_dst_blue);
		t_inv_src_alpha_dst_alpha = upscale(t_dst_alpha);
	}
	else if (t_src_alpha == 255)
	{
		t_src_alpha_dst_red = upscale(t_dst_red);
		t_src_alpha_dst_green = upscale(t_dst_green);
		t_src_alpha_dst_blue = upscale(t_dst_blue);
		t_src_alpha_dst_alpha = upscale(t_dst_alpha);

		t_inv_src_alpha_dst_red = 0;
		t_inv_src_alpha_dst_green = 0;
		t_inv_src_alpha_dst_blue = 0;
		t_inv_src_alpha_dst_alpha = 0;
	}
	else
	{
		t_src_alpha_dst_red = t_src_alpha * t_dst_red;
		t_src_alpha_dst_green = t_src_alpha * t_dst_green;
		t_src_alpha_dst_blue = t_src_alpha * t_dst_blue;
		t_src_alpha_dst_alpha = t_src_alpha * t_dst_alpha;

		t_inv_src_alpha_dst_red = (255 - t_src_alpha) * t_dst_red;
		t_inv_src_alpha_dst_green = (255 - t_src_alpha) * t_dst_green;
		t_inv_src_alpha_dst_blue = (255 - t_src_alpha) * t_dst_blue;
		t_inv_src_alpha_dst_alpha = (255 - t_src_alpha) * t_dst_alpha;
	}

	if (t_dst_alpha == 0)
	{
		t_dst_alpha_src_red = 0;
		t_dst_alpha_src_green = 0;
		t_dst_alpha_src_blue = 0;
		t_dst_alpha_src_alpha = 0;

		t_dst_alpha_dst_red = 0;
		t_dst_alpha_dst_blue = 0;
		t_dst_alpha_dst_green = 0;
		t_dst_alpha_dst_alpha = 0;

		t_dst_alpha_dst_alpha_src_alpha = 0;

		t_inv_dst_alpha_src_red = upscale(t_src_red);
		t_inv_dst_alpha_src_green = upscale(t_src_green);
		t_inv_dst_alpha_src_blue = upscale(t_src_blue);
		t_inv_dst_alpha_src_alpha = upscale(t_src_alpha);
	}
	else if (t_dst_alpha == 255)
	{
		t_dst_alpha_src_red = upscale(t_src_red);
		t_dst_alpha_src_green = upscale(t_src_green);
		t_dst_alpha_src_blue = upscale(t_src_blue);
		t_dst_alpha_src_alpha = upscale(t_src_alpha);

		t_dst_alpha_dst_red = upscale(t_dst_red);
		t_dst_alpha_dst_green = upscale(t_dst_green);
		t_dst_alpha_dst_blue = upscale(t_dst_blue);
		t_dst_alpha_dst_alpha = upscale((uint8_t)255);
		t_dst_alpha_dst_alpha_src_alpha = t_src_alpha == 255 ? upscale(upscale(uint8_t(255))) : upscale(uint16_t(255 * t_src_alpha));

		t_inv_dst_alpha_src_red = 0;
		t_inv_dst_alpha_src_green = 0;
		t_inv_dst_alpha_src_blue = 0;
		t_inv_dst_alpha_src_alpha = 0;
	}
	else
	{
		t_dst_alpha_src_red = t_dst_alpha * t_src_red;
		t_dst_alpha_src_green = t_dst_alpha * t_src_green;
		t_dst_alpha_src_blue = t_dst_alpha * t_src_blue;
		t_dst_alpha_src_alpha = t_dst_alpha * t_src_alpha;

		t_dst_alpha_dst_red = t_dst_alpha * t_dst_red;
		t_dst_alpha_dst_green = t_dst_alpha * t_dst_green;
		t_dst_alpha_dst_blue = t_dst_alpha * t_dst_blue;
		t_dst_alpha_dst_alpha = t_dst_alpha * t_dst_alpha;
		t_dst_alpha_dst_alpha_src_alpha = t_dst_alpha_dst_alpha * t_src_alpha;

		t_inv_dst_alpha_src_red = (255 - t_dst_alpha) * t_src_red;
		t_inv_dst_alpha_src_green = (255 - t_dst_alpha) * t_src_green;
		t_inv_dst_alpha_src_blue = (255 - t_dst_alpha) * t_src_blue;
		t_inv_dst_alpha_src_alpha = (255 - t_dst_alpha) * t_src_alpha;
	}

	switch(x_combiner)
	{
	case OPERATION_BLEND_OVERLAY:
		t_red = 2 * t_dst_red < t_dst_alpha ?
				downscale(2 * t_src_red * t_dst_red + t_inv_dst_alpha_src_red + t_inv_src_alpha_dst_red) :
				downscale(t_src_alpha_dst_alpha - 2 * (t_dst_alpha - t_dst_red) * (t_src_alpha - t_src_red) + t_inv_dst_alpha_src_red + t_inv_src_alpha_dst_red);
		t_green = 2 * t_dst_green < t_dst_alpha ?
				downscale(2 * t_src_green * t_dst_green + t_inv_dst_alpha_src_green + t_inv_src_alpha_dst_green) :
				downscale(t_src_alpha_dst_alpha - 2 * (t_dst_alpha - t_dst_green) * (t_src_alpha - t_src_green) + t_inv_dst_alpha_src_green + t_inv_src_alpha_dst_green);
		t_blue = 2 * t_dst_blue < t_dst_alpha ?
				downscale(2 * t_src_blue * t_dst_blue + t_inv_dst_alpha_src_blue + t_inv_src_alpha_dst_blue) :
				downscale(t_src_alpha_dst_alpha - 2 * (t_dst_alpha - t_dst_blue) * (t_src_alpha - t_src_blue) + t_inv_dst_alpha_src_blue + t_inv_src_alpha_dst_blue);
		t_alpha = t_src_alpha + t_dst_alpha - downscale(t_src_alpha_dst_alpha);
	break;
	case OPERATION_BLEND_DARKEN:
		t_red = downscale(fastmin(t_dst_alpha_src_red, t_src_alpha_dst_red) + t_inv_dst_alpha_src_red + t_inv_src_alpha_dst_red);
		t_green = downscale(fastmin(t_dst_alpha_src_green, t_src_alpha_dst_green) + t_inv_dst_alpha_src_green + t_inv_src_alpha_dst_green);
		t_blue = downscale(fastmin(t_dst_alpha_src_blue, t_src_alpha_dst_blue) + t_inv_dst_alpha_src_blue + t_inv_src_alpha_dst_blue);
		t_alpha = t_src_alpha + t_dst_alpha - downscale(t_src_alpha_dst_alpha);
	break;
	case OPERATION_BLEND_LIGHTEN:
		t_red = downscale(fastmax(t_dst_alpha_src_red, t_src_alpha_dst_red) + t_inv_dst_alpha_src_red + t_inv_src_alpha_dst_red);
		t_green = downscale(fastmax(t_dst_alpha_src_green, t_src_alpha_dst_green) + t_inv_dst_alpha_src_green + t_inv_src_alpha_dst_green);
		t_blue = downscale(fastmax(t_dst_alpha_src_blue, t_src_alpha_dst_blue) + t_inv_dst_alpha_src_blue + t_inv_src_alpha_dst_blue);
		t_alpha = t_src_alpha + t_dst_alpha - downscale(t_src_alpha_dst_alpha);
	break;
	case OPERATION_BLEND_DODGE:
		t_red = t_dst_alpha_src_red + t_src_alpha_dst_red >= t_src_alpha_dst_alpha ?
				downscale(t_dst_alpha_src_alpha + t_inv_dst_alpha_src_red + t_inv_src_alpha_dst_red) :
				downscale(scaled_divide(t_src_alpha_dst_red, t_src_alpha, t_src_alpha - t_src_red) + t_inv_dst_alpha_src_red + t_inv_src_alpha_dst_red);
		t_green = t_dst_alpha_src_green + t_src_alpha_dst_green >= t_src_alpha_dst_alpha ?
				downscale(t_dst_alpha_src_alpha + t_inv_dst_alpha_src_green + t_inv_src_alpha_dst_green) :
				downscale(scaled_divide(t_src_alpha_dst_green, t_src_alpha, t_src_alpha - t_src_green) + t_inv_dst_alpha_src_green + t_inv_src_alpha_dst_green);
		t_blue = t_dst_alpha_src_blue + t_src_alpha_dst_blue >= t_src_alpha_dst_alpha ?
				downscale(t_dst_alpha_src_alpha + t_inv_dst_alpha_src_blue + t_inv_src_alpha_dst_blue) :
				downscale(scaled_divide(t_src_alpha_dst_blue, t_src_alpha, t_src_alpha - t_src_blue) + t_inv_dst_alpha_src_blue + t_inv_src_alpha_dst_blue);
		t_alpha = t_src_alpha + t_dst_alpha - downscale(t_src_alpha_dst_alpha);
	break;
	case OPERATION_BLEND_BURN:
		t_red = t_dst_alpha_src_red + t_src_alpha_dst_red <= t_src_alpha_dst_alpha ?
				downscale(t_inv_dst_alpha_src_red + t_inv_src_alpha_dst_red) :
				downscale(scaled_divide(t_dst_alpha_src_red + t_src_alpha_dst_red - t_src_alpha_dst_alpha, t_src_alpha, t_src_red) + t_inv_dst_alpha_src_red + t_inv_src_alpha_dst_red);
		t_green = t_dst_alpha_src_green + t_src_alpha_dst_green <= t_src_alpha_dst_alpha ?
				downscale(t_inv_dst_alpha_src_green + t_inv_src_alpha_dst_green) :
				downscale(scaled_divide(t_dst_alpha_src_green + t_src_alpha_dst_green - t_src_alpha_dst_alpha, t_src_alpha, t_src_green) + t_inv_dst_alpha_src_green + t_inv_src_alpha_dst_green);
		t_blue = t_dst_alpha_src_blue + t_src_alpha_dst_blue <= t_src_alpha_dst_alpha ?
				downscale(t_inv_dst_alpha_src_blue + t_inv_src_alpha_dst_blue) :
				downscale(scaled_divide(t_dst_alpha_src_blue + t_src_alpha_dst_blue - t_src_alpha_dst_alpha, t_src_alpha, t_src_blue) + t_inv_dst_alpha_src_blue + t_inv_src_alpha_dst_blue);
		t_alpha = t_src_alpha + t_dst_alpha - downscale(t_src_alpha_dst_alpha);
	break;
	case OPERATION_BLEND_HARD_LIGHT:
		t_red = 2 * t_src_red < t_src_alpha ?
				downscale(2 * t_src_red * t_dst_red + t_inv_dst_alpha_src_red + t_inv_src_alpha_dst_red) :
				downscale(t_src_alpha_dst_alpha - 2 * (t_dst_alpha - t_dst_red) * (t_src_alpha - t_src_red) + t_inv_dst_alpha_src_red + t_inv_src_alpha_dst_red);
		t_green = 2 * t_src_green < t_src_alpha ?
				downscale(2 * t_src_green * t_dst_green + t_inv_dst_alpha_src_green + t_inv_src_alpha_dst_green) :
				downscale(t_src_alpha_dst_alpha - 2 * (t_dst_alpha - t_dst_green) * (t_src_alpha - t_src_green) + t_inv_dst_alpha_src_green + t_inv_src_alpha_dst_green);
		t_blue = 2 * t_src_blue < t_src_alpha ?
				downscale(2 * t_src_blue * t_dst_blue + t_inv_dst_alpha_src_blue + t_inv_src_alpha_dst_blue) :
				downscale(t_src_alpha_dst_alpha - 2 * (t_dst_alpha - t_dst_blue) * (t_src_alpha - t_src_blue) + t_inv_dst_alpha_src_blue + t_inv_src_alpha_dst_blue);
		t_alpha = t_src_alpha + t_dst_alpha - downscale(t_src_alpha_dst_alpha);
	break;
	case OPERATION_BLEND_SOFT_LIGHT:
		if (2 * t_src_red < t_src_alpha)
			t_red = downscale(scaled_divide(t_src_alpha_dst_alpha + (t_dst_alpha - t_dst_red) * (t_src_alpha - 2 * t_src_red), t_dst_red, t_dst_alpha) + t_inv_dst_alpha_src_red + t_inv_src_alpha_dst_red);
		else if (8 * t_dst_red < t_dst_alpha)
			t_red = downscale(long_scaled_divide(t_dst_alpha_dst_alpha_src_alpha - (t_dst_alpha - t_dst_red) * (2 * t_src_red - t_src_alpha) * (3 * t_dst_alpha - 8 * t_dst_red), t_dst_red, t_dst_alpha_dst_alpha) + t_inv_dst_alpha_src_red + t_inv_src_alpha_dst_red);
		else
			t_red = downscale(t_src_alpha_dst_red + (sqrt(t_dst_alpha_dst_red) - t_dst_red) * (2 * t_src_red - t_src_alpha) + t_inv_dst_alpha_src_red + t_inv_src_alpha_dst_red);

		if (2 * t_src_green < t_src_alpha)
			t_green = downscale(scaled_divide(t_src_alpha_dst_alpha + (t_dst_alpha - t_dst_green) * (t_src_alpha - 2 * t_src_green), t_dst_green, t_dst_alpha) + t_inv_dst_alpha_src_green + t_inv_src_alpha_dst_green);
		else if (8 * t_dst_green < t_dst_alpha)
			t_green = downscale(long_scaled_divide(t_dst_alpha_dst_alpha_src_alpha - (t_dst_alpha - t_dst_green) * (2 * t_src_green - t_src_alpha) * (3 * t_dst_alpha - 8 * t_dst_green), t_dst_green, t_dst_alpha_dst_alpha) + t_inv_dst_alpha_src_green + t_inv_src_alpha_dst_green);
		else
			t_green = downscale(t_src_alpha_dst_green + (sqrt(t_dst_alpha_dst_green) - t_dst_green) * (2 * t_src_green - t_src_alpha) + t_inv_dst_alpha_src_green + t_inv_src_alpha_dst_green);

		if (2 * t_src_blue < t_src_alpha)
			t_blue = downscale(scaled_divide(t_src_alpha_dst_alpha + (t_dst_alpha - t_dst_blue) * (t_src_alpha - 2 * t_src_blue), t_dst_blue, t_dst_alpha) + t_inv_dst_alpha_src_blue + t_inv_src_alpha_dst_blue);
		else if (8 * t_dst_blue < t_dst_alpha)
			t_blue = downscale(long_scaled_divide(t_dst_alpha_dst_alpha_src_alpha - (t_dst_alpha - t_dst_blue) * (2 * t_src_blue - t_src_alpha) * (3 * t_dst_alpha - 8 * t_dst_blue), t_dst_blue, t_dst_alpha_dst_alpha) + t_inv_dst_alpha_src_blue + t_inv_src_alpha_dst_blue);
		else
			t_blue = downscale(t_src_alpha_dst_blue + (sqrt(t_dst_alpha_dst_blue) - t_dst_blue) * (2 * t_src_blue - t_src_alpha) + t_inv_dst_alpha_src_blue + t_inv_src_alpha_dst_blue);

		t_alpha = t_src_alpha + t_dst_alpha - downscale(t_src_alpha_dst_alpha);
	break;
	case OPERATION_BLEND_DIFFERENCE:
		t_red = t_src_red + t_dst_red - 2 * downscale(fastmin(t_dst_alpha_src_red, t_src_alpha_dst_red));
		t_green = t_src_green + t_dst_green - 2 * downscale(fastmin(t_dst_alpha_src_green, t_src_alpha_dst_green));
		t_blue = t_src_blue + t_dst_blue - 2 * downscale(fastmin(t_dst_alpha_src_blue, t_src_alpha_dst_blue));
		t_alpha = t_src_alpha + t_dst_alpha - downscale(t_src_alpha_dst_alpha);
	break;
	case OPERATION_BLEND_EXCLUSION:
		t_red = downscale(t_src_red * (t_dst_alpha - t_dst_red) + t_dst_red * (t_src_alpha - t_src_red) + t_inv_dst_alpha_src_red + t_inv_src_alpha_dst_red);
		t_green = downscale(t_src_green * (t_dst_alpha - t_dst_green) + t_dst_green * (t_src_alpha - t_src_green) + t_inv_dst_alpha_src_green + t_inv_src_alpha_dst_green);
		t_blue = downscale(t_src_blue * (t_dst_alpha - t_dst_blue) + t_dst_blue * (t_src_alpha - t_src_blue) + t_inv_dst_alpha_src_blue + t_inv_src_alpha_dst_blue);
		t_alpha = t_src_alpha + t_dst_alpha - downscale(t_src_alpha_dst_alpha);
	break;
    default:
        MCUnreachableReturn(0);
	}

	if (x_dst_alpha)
		return t_red | (t_green << 8) | (t_blue << 16) | (t_alpha << 24);

	return t_red | (t_green << 8) | (t_blue << 16);
}

// These are commented out until a particular bug in Visual Studio (described
// below) is fixed.
//
/*template<BitwiseOperation x_combiner, bool x_dst_alpha, bool x_src_alpha> INLINE uint32_t pixel_combine(uint32_t dst, uint32_t src)
 {
	return bitwise_combiner<x_combiner, x_dst_alpha, x_src_alpha>(dst, src);
 }
 
 template<ArithmeticOperation x_combiner, bool x_dst_alpha, bool x_src_alpha> INLINE uint32_t pixel_combine(uint32_t dst, uint32_t src)
 {
	return arithmetic_combiner<x_combiner, x_dst_alpha, x_src_alpha>(dst, src);
 }
 
 template<BasicImagingOperation x_combiner, bool x_dst_alpha, bool x_src_alpha> INLINE uint32_t pixel_combine(uint32_t dst, uint32_t src)
 {
	return basic_imaging_combiner<x_combiner, x_dst_alpha, x_src_alpha>(dst, src);
 }
 
 template<AdvancedImagingOperation x_combiner, bool x_dst_alpha, bool x_src_alpha> INLINE uint32_t pixel_combine(uint32_t dst, uint32_t src)
 {
	return advanced_imaging_combiner<x_combiner, x_dst_alpha, x_src_alpha>(dst, src);
 }*/

// Nasty shim to work around a Visual Studio compiler bug (it doesn't handle
// templates whose parameters are overloaded on enums properly so it always
// tries to use the AdvancedImagingOperation form of the template)
template<int x_combiner, bool x_dst_alpha, bool x_src_alpha> INLINE uint32_t pixel_combine(uint32_t dst, uint32_t src)
{
    // These should get collapsed at compile-time so no need to worry about a
    // performance hit (assuming the compiler is half-way sensible...)
    if (x_combiner <= LAST_BITWISE_OPERATION)
        return bitwise_combiner<BitwiseOperation(x_combiner), x_dst_alpha, x_src_alpha>(dst, src);
    else if (x_combiner <= LAST_ARITHMETIC_OPERATION)
        return arithmetic_combiner<ArithmeticOperation(x_combiner), x_dst_alpha, x_src_alpha>(dst, src);
    else if (x_combiner <= LAST_BASIC_IMAGING_OPERATION)
        return basic_imaging_combiner<BasicImagingOperation(x_combiner), x_dst_alpha, x_src_alpha>(dst, src);
    else if (x_combiner <= LAST_ADVANCED_IMAGING_OPERATION)
        return advanced_imaging_combiner<AdvancedImagingOperation(x_combiner), x_dst_alpha, x_src_alpha>(dst, src);
    else
        MCUnreachable();
}

template<class T, T x_combiner, bool x_dst_alpha, bool x_src_alpha>
INLINE void surface_combine(void *p_dst, int32_t p_dst_stride, const void *p_src, uint32_t p_src_stride, uint32_t p_width, uint32_t p_height, uint8_t p_opacity)
{
	if (p_opacity == 0)
		return;

	uint32_t *t_dst_ptr;
	uint32_t t_dst_stride;

	t_dst_ptr = (uint32_t *)p_dst;
	t_dst_stride = (p_dst_stride >> 2) - p_width;

	uint32_t *t_src_ptr;
	uint32_t t_src_stride;

	t_src_ptr = (uint32_t *)p_src;
	t_src_stride = (p_src_stride >> 2) - p_width;

	for(; p_height > 0; --p_height, t_dst_ptr += t_dst_stride, t_src_ptr += t_src_stride)
	{
		for(uint32_t t_width = p_width; t_width > 0; --t_width)
		{
			uint32_t t_src, t_dst;
			uint32_t t_pixel;

			t_src = *t_src_ptr++;
			t_dst = *t_dst_ptr;
			t_pixel = pixel_combine<x_combiner, x_dst_alpha, x_src_alpha>(t_dst, t_src);

			if (p_opacity != 255)
				t_pixel = packed_bilinear_bounded(t_pixel, p_opacity, t_dst, 255 - p_opacity);

			*t_dst_ptr++ = t_pixel;
		}
	}
}

// These are commented out until a particular bug in Visual Studio (described
// below) is fixed.
//
/*template <BitwiseOperation x_combiner, bool x_dst_alpha, bool x_src_alpha>
void surface_combine(void *p_dst, int32_t p_dst_stride, const void *p_src, uint32_t p_src_stride, uint32_t p_width, uint32_t p_height, uint8_t p_opacity)
{
	return surface_combine<BitwiseOperation, x_combiner, x_dst_alpha, x_src_alpha>(p_dst, p_dst_stride, p_src, p_src_stride, p_width, p_height, p_opacity);
}

template <ArithmeticOperation x_combiner, bool x_dst_alpha, bool x_src_alpha>
void surface_combine(void *p_dst, int32_t p_dst_stride, const void *p_src, uint32_t p_src_stride, uint32_t p_width, uint32_t p_height, uint8_t p_opacity)
{
	return surface_combine<ArithmeticOperation, x_combiner, x_dst_alpha, x_src_alpha>(p_dst, p_dst_stride, p_src, p_src_stride, p_width, p_height, p_opacity);
}

template <BasicImagingOperation x_combiner, bool x_dst_alpha, bool x_src_alpha>
void surface_combine(void *p_dst, int32_t p_dst_stride, const void *p_src, uint32_t p_src_stride, uint32_t p_width, uint32_t p_height, uint8_t p_opacity)
{
	return surface_combine<BasicImagingOperation, x_combiner, x_dst_alpha, x_src_alpha>(p_dst, p_dst_stride, p_src, p_src_stride, p_width, p_height, p_opacity);
}

template <AdvancedImagingOperation x_combiner, bool x_dst_alpha, bool x_src_alpha>
void surface_combine(void *p_dst, int32_t p_dst_stride, const void *p_src, uint32_t p_src_stride, uint32_t p_width, uint32_t p_height, uint8_t p_opacity)
{
	return surface_combine<AdvancedImagingOperation, x_combiner, x_dst_alpha, x_src_alpha>(p_dst, p_dst_stride, p_src, p_src_stride, p_width, p_height, p_opacity);
}*/

// Nasty shim to work around a Visual Studio compiler bug (it doesn't handle
// templates whose parameters are overloaded on enums properly so it always
// tries to use the AdvancedImagingOperation form of the template)
template<int x_combiner, bool x_dst_alpha, bool x_src_alpha>
INLINE void surface_combine(void *p_dst, int32_t p_dst_stride, const void *p_src, uint32_t p_src_stride, uint32_t p_width, uint32_t p_height, uint8_t p_opacity)
{
    // These should get collapsed at compile-time so no need to worry about a
    // performance hit (assuming the compiler is half-way sensible...)
    if (x_combiner <= LAST_BITWISE_OPERATION)
        return surface_combine<BitwiseOperation, BitwiseOperation(x_combiner), x_dst_alpha, x_src_alpha>(p_dst, p_dst_stride, p_src, p_src_stride, p_width, p_height, p_opacity);
    else if (x_combiner <= LAST_ARITHMETIC_OPERATION)
        return surface_combine<ArithmeticOperation, ArithmeticOperation(x_combiner), x_dst_alpha, x_src_alpha>(p_dst, p_dst_stride, p_src, p_src_stride, p_width, p_height, p_opacity);
    else if (x_combiner <= LAST_BASIC_IMAGING_OPERATION)
        return surface_combine<BasicImagingOperation, BasicImagingOperation(x_combiner), x_dst_alpha, x_src_alpha>(p_dst, p_dst_stride, p_src, p_src_stride, p_width, p_height, p_opacity);
    else if (x_combiner <= LAST_ADVANCED_IMAGING_OPERATION)
        return surface_combine<AdvancedImagingOperation, AdvancedImagingOperation(x_combiner), x_dst_alpha, x_src_alpha>(p_dst, p_dst_stride, p_src, p_src_stride, p_width, p_height, p_opacity);
    else
        MCUnreachable();
}

// MW-2009-02-09: This is the most important combiner so we optimize it.
//   This optimization is based on the observation that:
//     (1 - e) * dst + e * (src over dst)
//   Is equiavlent to:
//     (e * src) over dst
//  In addition, we inline the fundamental operations ourselves since it
//  seems this inlining *isn't* done by GCC on PowerPC.
//
void surface_combine_blendSrcOver(void *p_dst, int32_t p_dst_stride, const void *p_src, uint32_t p_src_stride, uint32_t p_width, uint32_t p_height, uint8_t p_opacity)
{
	if (p_opacity == 0)
		return;
	
	uint32_t *t_dst_ptr;
	uint32_t t_dst_stride;
	
	t_dst_ptr = (uint32_t *)p_dst;
	t_dst_stride = (p_dst_stride >> 2) - p_width;
	
	uint32_t *t_src_ptr;
	uint32_t t_src_stride;
	
	t_src_ptr = (uint32_t *)p_src;
	t_src_stride = (p_src_stride >> 2) - p_width;
	
	for(; p_height > 0; --p_height, t_dst_ptr += t_dst_stride, t_src_ptr += t_src_stride)
	{
		for(uint32_t t_width = p_width; t_width > 0; --t_width)
		{
			uint32_t t_src;
			uint32_t t_pixel;
			
			t_src = *t_src_ptr++;
			
			if (t_src != 0)
			{
				// Compute [ opacity * src ]
				if (p_opacity != 255)
				{
					uint32_t u, v;
					
					u = ((t_src & 0xff00ff) * p_opacity) + 0x800080;
					u = ((u + ((u >> 8) & 0xff00ff)) >> 8) & 0xff00ff;
					
					v = (((t_src >> 8) & 0xff00ff) * p_opacity) + 0x800080;
					v = (v + ((v >> 8) & 0xff00ff)) & 0xff00ff00;
					
					t_src = u + v;
				}

				// Compute [ dst * (1 - src_alpha) + src ]
				{
					uint32_t x;
					uint8_t a;
					x = *t_dst_ptr;
					a = (~t_src) >> 24;
					
					uint32_t u, v;
					u = ((x & 0xff00ff) * a) + 0x800080;
					u = ((u + ((u >> 8) & 0xff00ff)) >> 8) & 0xff00ff;
					
					v = (((x >> 8) & 0xff00ff) * a) + 0x800080;
					v = (v + ((v >> 8) & 0xff00ff)) & 0xff00ff00;
					
					t_pixel = u + v + t_src;
				}
				
				*t_dst_ptr++ = t_pixel;

				continue;
			}
			
			t_dst_ptr += 1;
		}
	}
}

// MW-2011-09-22: Optimized variant of blendSrcOver for alpha-masked image. The
//    assumption here is that 'src' is not pre-multipled, its alpha mask being
//    given by the mask array.
void surface_combine_blendSrcOver_masked(void *p_dst, int32_t p_dst_stride, const void *p_src, uint32_t p_src_stride, uint32_t p_width, uint32_t p_height, uint8_t p_opacity)
{
	if (p_opacity == 0)
		return;
	
	uint32_t *t_dst_ptr;
	uint32_t t_dst_stride;
	t_dst_ptr = (uint32_t *)p_dst;
	t_dst_stride = (p_dst_stride >> 2) - p_width;
	
	uint32_t *t_src_ptr;
	uint32_t t_src_stride;
	t_src_ptr = (uint32_t *)p_src;
	t_src_stride = (p_src_stride >> 2) - p_width;
	
	if (p_opacity == 255)
	{
		// If opacity is 255 then we only need do:
		//   dst = dst * (1 - msk) + msk * src.
		for(; p_height > 0; --p_height, t_dst_ptr += t_dst_stride, t_src_ptr += t_src_stride)
		{
			for(uint32_t t_width = p_width; t_width > 0; --t_width)
			{
				uint32_t t_src;
				t_src = *t_src_ptr++;
				
				uint8_t t_mask;
				t_mask = t_src >> 24;

				if (t_mask != 0)
				{
					uint32_t t_dst;
					t_src |= 0xff000000;
					t_dst = *t_dst_ptr;

					uint8_t sa, da;
					sa = t_mask;
					da = (~t_mask) & 0xff;

					// Compute [ dst * (1 - msk) + msk * src ]
					uint32_t u, v;

					u = (t_dst & 0xff00ff) * da + (t_src & 0xff00ff) * sa + 0x800080;
					u = ((u + ((u >> 8) & 0xff00ff)) >> 8) & 0xff00ff;

					v = ((t_dst >> 8) & 0xff00ff) * da + ((t_src >> 8) & 0xff00ff) * sa + 0x800080;
					v = (v + ((v >> 8) & 0xff00ff)) & 0xff00ff00;

					*t_dst_ptr++ = u | v;
					
					continue;
				}

				t_dst_ptr += 1;
			}
		}
	}
	else
	{
		// If opacity is not 255 then we need to do:
		//   dst = dst * (1 - (msk * opc)) + (msk * opc) * src.
		for(; p_height > 0; --p_height, t_dst_ptr += t_dst_stride, t_src_ptr += t_src_stride)
		{
			for(uint32_t t_width = p_width; t_width > 0; --t_width)
			{
				uint32_t t_src;
				t_src = *t_src_ptr++;
				
				uint8_t t_mask;
				t_mask = t_src >> 24;

				if (t_mask != 0)
				{
					uint32_t t_dst;
					t_src |= 0xff000000;
					t_dst = *t_dst_ptr;

					// Compute [ mask * opacity ]
					uint32_t a;
					a = p_opacity * t_mask;

					uint8_t sa, da;
					sa = ((a + 0x80) + ((a + 0x80) >> 8)) >> 8;
					da = (~sa) & 0xff;

					// Compute [ dst * (1 - msk) + msk * src ]
					uint32_t u, v;

					u = (t_dst & 0xff00ff) * da + (t_src & 0xff00ff) * sa + 0x800080;
					u = ((u + ((u >> 8) & 0xff00ff)) >> 8) & 0xff00ff;

					v = ((t_dst >> 8) & 0xff00ff) * da + ((t_src >> 8) & 0xff00ff) * sa + 0x800080;
					v = (v + ((v >> 8) & 0xff00ff)) & 0xff00ff00;

					*t_dst_ptr++ = u | v;
					
					continue;
				}

				t_dst_ptr += 1;
			}
		}
	}	
}

// MW-2011-09-22: Optimized variant of blendSrcOver for a solid image. The
//   assumption here is that src's alpha is 255.
void surface_combine_blendSrcOver_solid(void *p_dst, int32_t p_dst_stride, const void *p_src, uint32_t p_src_stride, uint32_t p_width, uint32_t p_height, uint8_t p_opacity)
{
	if (p_opacity == 0)
		return;
	
	uint32_t *t_dst_ptr;
	uint32_t t_dst_stride;
	t_dst_ptr = (uint32_t *)p_dst;
	t_dst_stride = (p_dst_stride >> 2) - p_width;
	
	uint32_t *t_src_ptr;
	uint32_t t_src_stride;
	t_src_ptr = (uint32_t *)p_src;
	t_src_stride = (p_src_stride >> 2) - p_width;
	
	// Special-case opacity == 255, as this is just a copy operation.
	if (p_opacity == 255)
	{
		for(; p_height > 0; --p_height, t_dst_ptr += t_dst_stride, t_src_ptr += t_src_stride)
		{
			for(uint32_t t_width = p_width; t_width > 0; --t_width)
			{
				// MW-2011-10-03: [[ Bug ]] Make sure the source is opaque.
				*t_dst_ptr++ = *t_src_ptr++ | 0xff000000;
			}
		}

		return;
	}
	
	// Otherwise we must do:
	//   dst = (1 - opc) * dst + opc * src
	uint8_t t_inv_opacity;
	t_inv_opacity = 255 - p_opacity;

	for(; p_height > 0; --p_height, t_dst_ptr += t_dst_stride, t_src_ptr += t_src_stride)
	{
		for(uint32_t t_width = p_width; t_width > 0; --t_width)
		{
			uint32_t t_src;
			uint32_t t_pixel;
		
			// MW-2011-10-03: [[ Bug ]] Make sure the source is opaque.
			t_src = *t_src_ptr++ | 0xff000000;

			// Compute [ opacity * src ]
			{
				uint32_t u, v;
				
				u = ((t_src & 0xff00ff) * p_opacity) + 0x800080;
				u = ((u + ((u >> 8) & 0xff00ff)) >> 8) & 0xff00ff;
				
				v = (((t_src >> 8) & 0xff00ff) * p_opacity) + 0x800080;
				v = (v + ((v >> 8) & 0xff00ff)) & 0xff00ff00;
				
				t_src = u + v;
			}

			// Compute [ dst * (1 - src_alpha) + src ]
			{
				uint32_t x;
				x = *t_dst_ptr;
				
				uint32_t u, v;
				u = ((x & 0xff00ff) * t_inv_opacity) + 0x800080;
				u = ((u + ((u >> 8) & 0xff00ff)) >> 8) & 0xff00ff;
				
				v = (((x >> 8) & 0xff00ff) * t_inv_opacity) + 0x800080;
				v = (v + ((v >> 8) & 0xff00ff)) & 0xff00ff00;
				
				t_pixel = u + v + t_src;
			}
			
			*t_dst_ptr++ = t_pixel;
		}
	}	
}

// We have the following equivalences:
//   GXsrcBic == GXandReverse
//   GXnotSrcBic == GXand
//   GXcopy == GXblendSrc

surface_combiner_t s_surface_combiners[] =
{
	surface_combine<OPERATION_CLEAR, true, true>,
	surface_combine<OPERATION_AND, true, true>,
	surface_combine<OPERATION_AND_REVERSE, true, true>,
	surface_combine_blendSrcOver, // Was OPERATION_COPY
	surface_combine<OPERATION_AND_INVERTED, true, true>,
	surface_combine<OPERATION_NOOP, true, true>,
	surface_combine<OPERATION_XOR, true, true>,
	surface_combine<OPERATION_OR, true, true>,
	surface_combine<OPERATION_NOR, true, true>,
	surface_combine<OPERATION_EQUIV, true, true>,
	surface_combine<OPERATION_INVERT, true, true>,
	surface_combine<OPERATION_OR_REVERSE, true, true>,
	surface_combine<OPERATION_COPY_INVERTED, true, true>,
	surface_combine<OPERATION_OR_INVERTED, true, true>,
	surface_combine<OPERATION_NAND, true, true>,
	surface_combine<OPERATION_SET, true, true>,
	surface_combine<OPERATION_SRC_BIC, true, true>,
	surface_combine<OPERATION_NOT_SRC_BIC, true, true>,

	surface_combine<OPERATION_BLEND, true, true>,
	surface_combine<OPERATION_ADD_PIN, true, true>,
	surface_combine<OPERATION_ADD_OVER, true, true>,
	surface_combine<OPERATION_SUB_PIN, true, true>,
	surface_combine<OPERATION_TRANSPARENT, true, true>,
	surface_combine<OPERATION_AD_MAX, true, true>,
	surface_combine<OPERATION_SUB_OVER, true, true>,
	surface_combine<OPERATION_AD_MIN, true, true>,

	surface_combine<OPERATION_BLEND_CLEAR, true, true>,
	surface_combine<OPERATION_BLEND_SRC, true, true>,
	surface_combine<OPERATION_BLEND_DST, true, true>,
	surface_combine_blendSrcOver,
	surface_combine<OPERATION_BLEND_DST_OVER, true, true>,
	surface_combine<OPERATION_BLEND_SRC_IN, true, true>,
	surface_combine<OPERATION_BLEND_DST_IN, true, true>,
	surface_combine<OPERATION_BLEND_SRC_OUT, true, true>,
	surface_combine<OPERATION_BLEND_DST_OUT, true, true>,
	surface_combine<OPERATION_BLEND_SRC_ATOP, true, true>,
	surface_combine<OPERATION_BLEND_DST_ATOP, true, true>,
	surface_combine<OPERATION_BLEND_XOR, true, true>,
	surface_combine<OPERATION_BLEND_PLUS, true, true>,
	surface_combine<OPERATION_BLEND_MULTIPLY, true, true>,
	surface_combine<OPERATION_BLEND_SCREEN, true, true>,

	surface_combine<OPERATION_BLEND_OVERLAY, true, true>,
	surface_combine<OPERATION_BLEND_DARKEN, true, true>,
	surface_combine<OPERATION_BLEND_LIGHTEN, true, true>,
	surface_combine<OPERATION_BLEND_DODGE, true, true>,
	surface_combine<OPERATION_BLEND_BURN, true, true>,
	surface_combine<OPERATION_BLEND_HARD_LIGHT, true, true>,
	surface_combine<OPERATION_BLEND_SOFT_LIGHT, true, true>,
	surface_combine<OPERATION_BLEND_DIFFERENCE, true, true>,
	surface_combine<OPERATION_BLEND_EXCLUSION, true, true>,
};

surface_combiner_t s_surface_combiners_nda[] =
{
	surface_combine<OPERATION_CLEAR, false, true>,
	surface_combine<OPERATION_AND, false, true>,
	surface_combine<OPERATION_AND_REVERSE, false, true>,
	surface_combine_blendSrcOver, // Was OPERATION_COPY
	surface_combine<OPERATION_AND_INVERTED, false, true>,
	surface_combine<OPERATION_NOOP, false, true>,
	surface_combine<OPERATION_XOR, false, true>,
	surface_combine<OPERATION_OR, false, true>,
	surface_combine<OPERATION_NOR, false, true>,
	surface_combine<OPERATION_EQUIV, false, true>,
	surface_combine<OPERATION_INVERT, false, true>,
	surface_combine<OPERATION_OR_REVERSE, false, true>,
	surface_combine<OPERATION_COPY_INVERTED, false, true>,
	surface_combine<OPERATION_OR_INVERTED, false, true>,
	surface_combine<OPERATION_NAND, false, true>,
	surface_combine<OPERATION_SET, false, true>,
	surface_combine<OPERATION_SRC_BIC, false, true>,
	surface_combine<OPERATION_NOT_SRC_BIC, false, true>,

	surface_combine<OPERATION_BLEND, false, true>,
	surface_combine<OPERATION_ADD_PIN, false, true>,
	surface_combine<OPERATION_ADD_OVER, false, true>,
	surface_combine<OPERATION_SUB_PIN, false, true>,
	surface_combine<OPERATION_TRANSPARENT, false, true>,
	surface_combine<OPERATION_AD_MAX, false, true>,
	surface_combine<OPERATION_SUB_OVER, false, true>,
	surface_combine<OPERATION_AD_MIN, false, true>,
	
	surface_combine<OPERATION_BLEND_CLEAR, false, true>,
	surface_combine<OPERATION_BLEND_SRC, false, true>,
	surface_combine<OPERATION_BLEND_DST, false, true>,
	surface_combine_blendSrcOver,
	surface_combine<OPERATION_BLEND_DST_OVER, false, true>,
	surface_combine<OPERATION_BLEND_SRC_IN, false, true>,
	surface_combine<OPERATION_BLEND_DST_IN, false, true>,
	surface_combine<OPERATION_BLEND_SRC_OUT, false, true>,
	surface_combine<OPERATION_BLEND_DST_OUT, false, true>,
	surface_combine<OPERATION_BLEND_SRC_ATOP, false, true>,
	surface_combine<OPERATION_BLEND_DST_ATOP, false, true>,
	surface_combine<OPERATION_BLEND_XOR, false, true>,
	surface_combine<OPERATION_BLEND_PLUS, false, true>,
	surface_combine<OPERATION_BLEND_MULTIPLY, false, true>,
	surface_combine<OPERATION_BLEND_SCREEN, false, true>,

	surface_combine<OPERATION_BLEND_OVERLAY, false, true>,
	surface_combine<OPERATION_BLEND_DARKEN, false, true>,
	surface_combine<OPERATION_BLEND_LIGHTEN, false, true>,
	surface_combine<OPERATION_BLEND_DODGE, false, true>,
	surface_combine<OPERATION_BLEND_BURN, false, true>,
	surface_combine<OPERATION_BLEND_HARD_LIGHT, false, true>,
	surface_combine<OPERATION_BLEND_SOFT_LIGHT, false, true>,
	surface_combine<OPERATION_BLEND_DIFFERENCE, false, true>,
	surface_combine<OPERATION_BLEND_EXCLUSION, false, true>,
};

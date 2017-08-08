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

#include "graphics.h"
#include "graphics-internal.h"

////////////////////////////////////////////////////////////////////////////////

#if __BIG_ENDIAN__
#define iman_ 1
#else
#define iman_ 0
#endif
typedef long int32;

#ifdef _LINUX
//IM - fast double -> int code seems to be broken on linux
inline int32 fast_rint(double val) {
	return (int32)(val + 0.5);
}

inline int32 fast_floor(double val) {
	return (int32)val;
}
#else

/* Fast version of (int)rint()
 Works for -2147483648.5 .. 2147483647.49975574019
 Requires IEEE floating point.
 */
inline int32 fast_rint(double val) {
	val = val + 68719476736.0*65536.0*1.5;
	return ((int32*)&val)[iman_];
}

/* Fast version of (int)floor()
 Requires IEEE floating point.
 Rounds numbers greater than n.9999923668 to n+1 rather than n,
 this could be fixed by changing the FP rounding mode and using
 the fast_rint() code.
 Works for -32728 to 32727.99999236688
 The alternative that uses long-long works for -2147483648 to 
 2147483647.999923688
 */
inline int32 fast_floor(double val) {
	val = val + (68719476736.0*1.5);
#if 0
	return (int32)((*(long long *)&val)>>16);
#else
	return (((int32*)&val)[iman_]>>16);
#endif
}
#endif

#define STOP_DIFF_PRECISION 24
#define STOP_DIFF_MULT ((1 << STOP_DIFF_PRECISION) * (uint4)255)
#define STOP_INT_PRECISION 16
#define STOP_INT_MAX ((1 << STOP_INT_PRECISION) - 1)
#define STOP_INT_MIRROR_MAX ((2 << STOP_INT_PRECISION) - 1)
#define GRADIENT_ROUND_EPSILON (float)0.000005

#define GRADIENT_AA_SCALE (2)

////////////////////////////////////////////////////////////////////////////////

typedef unsigned char   uint1;
typedef          char   int1;
typedef unsigned short  uint2;
typedef          short  int2;
typedef unsigned int    uint4;
typedef          int    int4;
typedef float           real4;
typedef double          real8;

////////////////////////////////////////////////////////////////////////////////

enum MCGradientFillKind
{
	kMCGradientKindNone,
	kMCGradientKindLinear = 3,
	kMCGradientKindRadial,
	kMCGradientKindConical,
	kMCGradientKindDiamond,
	kMCGradientKindSpiral,
	kMCGradientKindXY,
	kMCGradientKindSqrtXY
};

struct MCPoint
{
	int2 x, y;
};

struct MCGradientFillStop
{
	uint4 offset;
	uint4 color;
	uint4 hw_color;
	uint4 difference;
};

struct MCCombiner
{
	void (*begin)(MCCombiner *self, int4 y);
	void (*advance)(MCCombiner *self, int4 y);
	void (*blend)(MCCombiner *self, int4 fx, int4 tx, uint1 value);
	void (*combine)(MCCombiner *self, int4 fx, int4 tx);
	void (*end)(MCCombiner *self);
};

struct MCSurfaceCombiner: public MCCombiner
{
	uint4 *bits;
	int4 stride;
};

struct MCSolidCombiner: public MCSurfaceCombiner
{
	uint4 pixel;
};

struct MCPatternCombiner: public MCSurfaceCombiner
{
	uint4 *pattern_bits;
	uint4 pattern_stride;
	int4 origin_x;
	int4 origin_y;
	uint4 width;
	uint4 height;
	uint4 pattern_offset;
};

struct MCGradientCombiner: public MCSurfaceCombiner
{
	MCGradientFillStop *ramp;
	uint4 ramp_length;
	MCPoint origin;
	bool mirror;
	uint4 repeat;
	bool wrap;
};

struct MCGradientAffineCombiner: public MCGradientCombiner
{
	int4 x_coef_a, x_coef_b, x_inc;
	int4 y_coef_a, y_coef_b, y_inc;
	
	uint4 buffer_width;
	uint4* buffer;
};

////////////////////////////////////////////////////////////////////////////////

#ifdef __VISUALC__
#define PACKED_INLINE __forceinline
#else
#define PACKED_INLINE inline
#endif

// r_i = (x_i * a) / 255
PACKED_INLINE uint4 packed_scale_bounded(uint4 x, uint1 a)
{
	uint4 u, v;
	
	u = ((x & 0xff00ff) * a) + 0x800080;
	u = ((u + ((u >> 8) & 0xff00ff)) >> 8) & 0xff00ff;
	
	v = (((x >> 8) & 0xff00ff) * a) + 0x800080;
	v = (v + ((v >> 8) & 0xff00ff)) & 0xff00ff00;
	
	return u | v;
}

// r_i = (x_i * a + y_i * b) / 255
PACKED_INLINE uint4 packed_bilinear_bounded(uint4 x, uint1 a, uint4 y, uint1 b)
{
	uint4 u, v;
	
	u = (x & 0xff00ff) * a + (y & 0xff00ff) * b + 0x800080;
	u = ((u + ((u >> 8) & 0xff00ff)) >> 8) & 0xff00ff;
	
	v = ((x >> 8) & 0xff00ff) * a + ((y >> 8) & 0xff00ff) * b + 0x800080;
	v = (v + ((v >> 8) & 0xff00ff)) & 0xff00ff00;
	
	return u | v;
}

PACKED_INLINE uint32_t _combine(uint32_t u, uint32_t v)
{
	u += 0x800080;
	v += 0x800080;
	return (((u + ((u >> 8) & 0xff00ff)) >> 8) & 0xff00ff) + (((v + ((v >> 8) & 0xff00ff))) & 0xff00ff00);
}

PACKED_INLINE uint32_t _multiply_low(uint32_t x, uint32_t y)
{
	return ((x & 0xff) * (y & 0xff)) | ((x & 0xff0000) * ((y >> 16) & 0xff));
}

PACKED_INLINE uint32_t _multiply_high(uint32_t x, uint32_t y)
{
	x = x >> 8;
	return ((x & 0xff) * ((y >> 8) & 0xff)) | ((x & 0xff0000) * (y >> 24));
}

// r_i = x_i * y_i / 255;
PACKED_INLINE uint32_t packed_multiply_bounded(uint32_t x, uint32_t y)
{
	return _combine(_multiply_low(x, y), _multiply_high(x, y));
}

// r_i = (x_i + y_i) / 2
PACKED_INLINE uint4 packed_avg(uint4 x, uint4 y)
{
	uint4 u, v;
	u = (((x & 0xff00ff) + (y & 0xff00ff)) >> 1) & 0xff00ff;
	v = ((((x >> 8) & 0xff00ff) + ((y >> 8) & 0xff00ff)) << 7) & 0xff00ff00;
	
	return u | v;
}

////////////////////////////////////////////////////////////////////////////////

inline uint4 MCU_abs(int4 source)
{
	return source > 0 ? source : -source;
}

inline int4 MCU_min(int4 one, int4 two) {return one > two ? two : one;}
inline uint4 MCU_min(uint4 one, uint4 two) {return one > two ? two : one;}

inline int4 MCU_max(int4 one, int4 two) {return one > two ? one : two;}
inline uint4 MCU_max(uint4 one, uint4 two) {return one > two ? one : two;}

////////////////////////////////////////////////////////////////////////////////

static void gradient_combiner_begin(MCCombiner *_self, int4 y)
{
	MCSolidCombiner *self = (MCSolidCombiner *)_self;
	self -> bits += y * self -> stride;
}

static void gradient_combiner_end(MCCombiner *_self)
{
}

static void gradient_affine_combiner_advance(MCCombiner *_self, int4 dy)
{
	MCGradientAffineCombiner *self = (MCGradientAffineCombiner *)_self;
	self -> bits += dy * self -> stride;
	self->x_inc += self->x_coef_b * dy;
	self->y_inc += self->y_coef_b * dy;
}

#define FP_2PI ((int4)(2 * M_PI * (1<<8)))
#define FP_INV_2PI ((STOP_INT_MAX << 8) / FP_2PI)

template<MCGradientFillKind x_type> static inline int4 compute_index(int4 p_x, int4 p_y, bool p_mirror, uint4 p_repeat, bool p_wrap)
{
	int4 t_index;
	switch(x_type)
	{
			// Per gradient ramp index calculation
		case kMCGradientKindLinear:
			t_index = p_x;
			break;
		case kMCGradientKindConical:
		{
			int4 t_angle = fast_rint((atan2((double)p_y, p_x) * (1<<8)));
			if (t_angle < 0)
				t_angle += FP_2PI;
			t_index = (t_angle * FP_INV_2PI) >> 8;
		}
			break;
		case kMCGradientKindRadial:
		{
			real8 t_dist = ((real8)(p_x)*p_x + (real8)(p_y)*p_y);
			t_index = !p_wrap && t_dist > ((real8)STOP_INT_MAX * STOP_INT_MAX) ? STOP_INT_MAX + 1 : fast_rint(sqrt(t_dist));
		}
			break;
		case kMCGradientKindDiamond:
			t_index = MCU_max(MCU_abs(p_x), MCU_abs(p_y));
			break;
		case kMCGradientKindSpiral:
		{
			int4 t_angle = fast_rint((atan2((double)p_y, p_x) * (1<<8)));
			real8 t_dist = sqrt((real8)(p_x)*p_x + (real8)(p_y)*p_y);
			t_index = fast_rint(t_dist);
			if (t_angle > 0)
				t_angle -= FP_2PI;
			t_index -= (t_angle * FP_INV_2PI) >> 8;
			t_index %= STOP_INT_MAX;
		}
			break;
		case kMCGradientKindXY:
		{
			uint4 t_x = MCU_abs(p_x);  uint4 t_y = MCU_abs(p_y);
			t_index = (int4) ((int64_t)t_x * t_y / STOP_INT_MAX);
		}
			break;
		case kMCGradientKindSqrtXY:
		{
			real8 t_x = MCU_abs(p_x);  real8 t_y = MCU_abs(p_y);
			t_index = fast_rint(sqrt(t_x * t_y));
		}
			break;
		default:
			//assert (false);
			return 0;
	}
	if (p_mirror)
	{
		if (p_wrap)
		{
			if (p_repeat > 1)
				t_index = (t_index * p_repeat);
			t_index &= STOP_INT_MIRROR_MAX;
			if (t_index > STOP_INT_MAX)
			{
				t_index = STOP_INT_MAX - (t_index & STOP_INT_MAX);
			}
		}
		else
		{
			if (t_index >= STOP_INT_MAX)
			{
				if ((p_repeat & 1) == 0)
					t_index = -t_index;
			}
			else if (p_repeat > 1 && t_index > 0)
			{
				t_index = (t_index * p_repeat);
				t_index &= STOP_INT_MIRROR_MAX;
				if (t_index > STOP_INT_MAX)
				{
					t_index = STOP_INT_MAX - (t_index & STOP_INT_MAX);
				}
			}
		}
	}
	else
	{
		if (p_wrap)
			t_index &= STOP_INT_MAX;
		if (p_repeat > 1 && t_index > 0 && t_index < STOP_INT_MAX)
		{
			t_index = (t_index * p_repeat);
			t_index &= 0xFFFF;
		}
	}
	return t_index;
}

template<MCGradientFillKind x_type> static void MCGradientFillBlend(MCCombiner *_self, int4 fx, int4 tx, uint1 alpha)
{
	MCGradientAffineCombiner *self = (MCGradientAffineCombiner*)_self;
	uint4 *d;
	uint4 s;
	
	d = self -> bits;
	
	int4 t_index;
	int4 t_x = self->x_inc + self->x_coef_a * ((int4)fx);
	int4 t_y = self->y_inc + self->y_coef_a * ((int4)fx);
	
	int4 t_min = (int4)self->ramp[0].offset;
	int4 t_max = (int4)self->ramp[self->ramp_length - 1].offset;
	
	if (fx == tx) return;
	
	bool t_mirror = self->mirror;
	uint4 t_repeat = self->repeat;
	
	if (alpha == 255)
	{
		uint4 t_stop_pos = 0;
		
		t_index = compute_index<x_type>(t_x, t_y, t_mirror, t_repeat);
		while (fx < tx)
		{
			if (t_index <= t_min)
			{
				s = self->ramp[0].hw_color;
				s = packed_scale_bounded(s | 0xFF000000, s >> 24);
				while (t_index <= t_min)
				{
					d[fx] = packed_scale_bounded(d[fx], 255 - (s >> 24)) + s;
					fx += 1;
					if (fx == tx)
						return;
					t_x += self->x_coef_a;
					t_y += self->y_coef_a;
					t_index = compute_index<x_type>(t_x, t_y, t_mirror, t_repeat);
				}
			}
			
			if (t_index >= t_max)
			{
				s = self->ramp[self->ramp_length - 1].hw_color;
				s = packed_scale_bounded(s | 0xFF000000, s >> 24);
				while (t_index >= t_max)
				{
					d[fx] = packed_scale_bounded(d[fx], 255 - (s >> 24)) + s;
					fx += 1;
					if (fx == tx)
						return;
					t_x += self->x_coef_a;
					t_y += self->y_coef_a;
					t_index = compute_index<x_type>(t_x, t_y, t_mirror, t_repeat);
				}
			}
			
			while (t_index >= t_min && t_index <= t_max)
			{
				MCGradientFillStop *t_current_stop = &self->ramp[t_stop_pos];
				int4 t_current_offset = t_current_stop->offset;
				int4 t_current_difference = t_current_stop->difference;
				uint4 t_current_color = t_current_stop->hw_color;
				MCGradientFillStop *t_next_stop = &self->ramp[t_stop_pos+1];
				int4 t_next_offset = t_next_stop->offset;
				uint4 t_next_color = t_next_stop->hw_color;
				
				while (t_next_offset >= t_index && t_current_offset <= t_index)
				{
					uint1 b = ((t_index - t_current_offset) * t_current_difference) >> STOP_DIFF_PRECISION ;
					uint1 a = 255 - b;
					
					s = packed_bilinear_bounded(t_current_color, a, t_next_color, b);
					d[fx] = packed_bilinear_bounded(d[fx], 255 - (s >> 24), s | 0xFF000000, s >> 24);
					fx += 1;
					if (fx == tx)
						return;
					t_x += self->x_coef_a;
					t_y += self->y_coef_a;
					t_index = compute_index<x_type>(t_x, t_y, t_mirror, t_repeat);
				}
				if (t_current_offset > t_index && t_stop_pos > 0)
					t_stop_pos -= 1;
				else if (t_next_offset < t_index && t_stop_pos < (self->ramp_length - 1))
					t_stop_pos += 1;
			}
		}
	}
}

template<MCGradientFillKind x_type> static void MCGradientFillCombine(MCCombiner *_self, int4 fx, int4 tx)
{
	MCGradientAffineCombiner *self = (MCGradientAffineCombiner*)_self;
	uint4 *d;
	uint4 s;
	
	d = self -> bits;
	
	int4 t_index;
	int4 t_x = self->x_inc + self->x_coef_a * ((int4)fx);
	int4 t_y = self->y_inc + self->y_coef_a * ((int4)fx);
	
	int4 t_min = (int4)self->ramp[0].offset;
	int4 t_max = (int4)self->ramp[self->ramp_length - 1].offset;
	
	if (fx == tx) return;
	
	bool t_mirror = self->mirror;
	uint4 t_repeat = self->repeat;
	bool t_wrap = self->wrap;
	
	uint4 t_stop_pos = 0;
	
	t_index = compute_index<x_type>(t_x, t_y, t_mirror, t_repeat, t_wrap);
	while (fx < tx)
	{
		if (t_index <= t_min)
		{
			s = self->ramp[0].hw_color;
			while (t_index <= t_min)
			{
				uint4 sa = packed_scale_bounded(s | 0xFF000000, (s >> 24));
				d[fx] = packed_scale_bounded(d[fx], 255 - (sa >> 24)) + sa;
				fx += 1;
				if (fx == tx)
					return;
				t_x += self->x_coef_a;
				t_y += self->y_coef_a;
				t_index = compute_index<x_type>(t_x, t_y, t_mirror, t_repeat, t_wrap);
			}
		}
		
		if (t_index >= t_max)
		{
			s = self->ramp[self->ramp_length - 1].hw_color;
			while (t_index >= t_max)
			{
				uint4 sa = packed_scale_bounded(s | 0xFF000000, (s >> 24));
				d[fx] = packed_scale_bounded(d[fx], 255 - (sa >> 24)) + sa;
				fx += 1;
				if (fx == tx)
					return;
				t_x += self->x_coef_a;
				t_y += self->y_coef_a;
				t_index = compute_index<x_type>(t_x, t_y, t_mirror, t_repeat, t_wrap);
			}
		}
		
		while (t_index >= t_min && t_index <= t_max)
		{
			MCGradientFillStop *t_current_stop = &self->ramp[t_stop_pos];
			int4 t_current_offset = t_current_stop->offset;
			int4 t_current_difference = t_current_stop->difference;
			uint4 t_current_color = t_current_stop->hw_color;
			MCGradientFillStop *t_next_stop = &self->ramp[t_stop_pos+1];
			int4 t_next_offset = t_next_stop->offset;
			uint4 t_next_color = t_next_stop->hw_color;
			
			while (t_next_offset >= t_index && t_current_offset <= t_index)
			{
				uint1 b = ((t_index - t_current_offset) * t_current_difference) >> STOP_DIFF_PRECISION ;
				uint1 a = 255 - b;
				
				s = packed_bilinear_bounded(t_current_color, a, t_next_color, b);
				uint4 sa = packed_scale_bounded(s | 0xFF000000, (s >> 24));
				d[fx] = packed_scale_bounded(d[fx], 255 - (sa >> 24)) + sa;
				fx += 1;
				if (fx == tx)
					return;
				t_x += self->x_coef_a;
				t_y += self->y_coef_a;
				t_index = compute_index<x_type>(t_x, t_y, t_mirror, t_repeat, t_wrap);
			}
			if (t_current_offset > t_index && t_stop_pos > 0)
				t_stop_pos -= 1;
			else if (t_next_offset < t_index && t_stop_pos < (self->ramp_length - 1))
				t_stop_pos += 1;
		}
	}
}

template<MCGradientFillKind x_type> static void blend_row(MCCombiner *_self, uint4 fx, uint4 tx, uint4 *p_buff)
{
	MCGradientAffineCombiner *self = (MCGradientAffineCombiner*)_self;
	uint4 s;
	
	int4 t_index;
	int4 t_x = self->x_inc + self->x_coef_a * ((int4)fx);
	int4 t_y = self->y_inc + self->y_coef_a * ((int4)fx);
	
	int4 t_min = (int4)self->ramp[0].offset;
	int4 t_max = (int4)self->ramp[self->ramp_length - 1].offset;
	
	uint4 t_stop_pos = 0;
	
	bool t_mirror = self->mirror;
	uint4 t_repeat = self->repeat;
	bool t_wrap = self->wrap;
	
	t_index = compute_index<x_type>(t_x, t_y, t_mirror, t_repeat, t_wrap);
	while (fx < tx)
	{
		if (t_index <= t_min)
		{
			s = self->ramp[0].hw_color;
			while (t_index <= t_min)
			{
				*p_buff = s;
				fx += 1;
				if (fx == tx)
					return;
				p_buff++;
				t_x += self->x_coef_a;
				t_y += self->y_coef_a;
				t_index = compute_index<x_type>(t_x, t_y, t_mirror, t_repeat, t_wrap);
			}
		}
		
		if (t_index >= t_max)
		{
			s = self->ramp[self->ramp_length - 1].hw_color;
			while (t_index >= t_max)
			{
				*p_buff = s;
				fx += 1;
				if (fx == tx)
					return;
				p_buff++;
				t_x += self->x_coef_a;
				t_y += self->y_coef_a;
				t_index = compute_index<x_type>(t_x, t_y, t_mirror, t_repeat, t_wrap);
			}
		}
		
		while (t_index >= t_min && t_index <= t_max)
		{
			MCGradientFillStop *t_current_stop = &self->ramp[t_stop_pos];
			int4 t_current_offset = t_current_stop->offset;
			int4 t_current_difference = t_current_stop->difference;
			uint4 t_current_color = t_current_stop->hw_color;
			MCGradientFillStop *t_next_stop = &self->ramp[t_stop_pos+1];
			int4 t_next_offset = t_next_stop->offset;
			uint4 t_next_color = t_next_stop->hw_color;
			
			while (t_next_offset >= t_index && t_current_offset <= t_index)
			{
				uint1 b = ((t_index - t_current_offset) * t_current_difference) >> STOP_DIFF_PRECISION ;
				uint1 a = 255 - b;
				
				s = packed_bilinear_bounded(t_current_color, a, t_next_color, b);
				*p_buff = s;
				fx += 1;
				if (fx == tx)
					return;
				p_buff++;
				t_x += self->x_coef_a;
				t_y += self->y_coef_a;
				t_index = compute_index<x_type>(t_x, t_y, t_mirror, t_repeat, t_wrap);
			}
			if (t_current_offset > t_index && t_stop_pos > 0)
				t_stop_pos -= 1;
			else if (t_next_offset < t_index && t_stop_pos < (self->ramp_length - 1))
				t_stop_pos += 1;
		}
	}
}

static void gradient_bilinear_affine_combiner_end(MCCombiner *_self)
{
	MCGradientAffineCombiner *self = (MCGradientAffineCombiner*)_self;
	delete self->buffer;
}

template<MCGradientFillKind x_type> static void MCGradientFillBilinearBlend(MCCombiner *_self, uint4 fx, uint4 tx, uint1 alpha)
{
	MCGradientAffineCombiner *self = (MCGradientAffineCombiner*)_self;
	uint4 *d;
	uint4 s;
	
	d = self -> bits;
	
	if (fx == tx) return;
	
	uint4 *t_buffer = self->buffer;
	uint4 t_bufflen = self->buffer_width;
	
	int4 x_a, x_b, x_inc;
	int4 y_a, y_b, y_inc;
	x_a = self->x_coef_a; x_b = self->x_coef_b; x_inc = self->x_inc;
	y_a = self->y_coef_a; y_b = self->y_coef_b; y_inc = self->y_inc;
	self->x_coef_a /= GRADIENT_AA_SCALE; self->x_coef_b /= GRADIENT_AA_SCALE;
	self->y_coef_a /= GRADIENT_AA_SCALE; self->y_coef_b /= GRADIENT_AA_SCALE;
	
	uint4 dx = (tx - fx) * GRADIENT_AA_SCALE;
	uint4 t_fx = fx * GRADIENT_AA_SCALE;
	for (int i = 0; i < GRADIENT_AA_SCALE; i++)
	{
		blend_row<x_type>(self, t_fx, t_fx + dx, t_buffer);
		t_buffer += t_bufflen;
		self->x_inc += self->x_coef_b;
		self->y_inc += self->y_coef_b;
	}
	
	self->x_coef_a = x_a; self->x_coef_b = x_b; self->x_inc = x_inc;
	self->y_coef_a = y_a; self->y_coef_b = y_b; self->y_inc = y_inc;
	
	uint4 i = 0;
	if (alpha == 255)
	{
		for (; fx < tx; fx++)
		{
			uint4 u;
			uint4 v;
			
#if GRADIENT_AA_SCALE == 2
			// unroll for GRADIENT_AA_SCALE == 2
			u = (self->buffer[i*2] & 0xFF00FF) + (self->buffer[i*2 + 1] & 0xFF00FF) + \
			(self->buffer[t_bufflen + i*2] & 0xFF00FF) + (self->buffer[t_bufflen + i*2 + 1] & 0xFF00FF);
			v = ((self->buffer[i*2] >> 8) & 0xFF00FF) + ((self->buffer[i*2 + 1] >> 8) & 0xFF00FF) + \
			((self->buffer[t_bufflen + i*2] >> 8) & 0xFF00FF) + ((self->buffer[t_bufflen + i*2 + 1] >> 8) & 0xFF00FF);
			u = (u >> 2) & 0xFF00FF;
			v = (v << 6) & 0xFF00FF00;
#endif
			i++;
			
			s = u | v;
			d[fx] = packed_bilinear_bounded(d[fx], 255 - (s >> 24), s | 0xFF000000, s >> 24);
		}
	}
}

template<MCGradientFillKind x_type> static void MCGradientFillBilinearCombine(MCCombiner *_self, int4 fx, int4 tx)
{
	MCGradientAffineCombiner *self = (MCGradientAffineCombiner*)_self;
	uint4 *d;
	uint4 s;
	
	d = self -> bits;
	
	if (fx == tx) return;
	
	uint4 *t_buffer = self->buffer;
	uint4 t_bufflen = self->buffer_width;
	
	int4 x_a, x_b, x_inc;
	int4 y_a, y_b, y_inc;
	x_a = self->x_coef_a; x_b = self->x_coef_b; x_inc = self->x_inc;
	y_a = self->y_coef_a; y_b = self->y_coef_b; y_inc = self->y_inc;
	self->x_coef_a /= GRADIENT_AA_SCALE; self->x_coef_b /= GRADIENT_AA_SCALE;
	self->y_coef_a /= GRADIENT_AA_SCALE; self->y_coef_b /= GRADIENT_AA_SCALE;
	
	uint4 dx = (tx - fx) * GRADIENT_AA_SCALE;
	uint4 t_fx = fx * GRADIENT_AA_SCALE;
	for (int i = 0; i < GRADIENT_AA_SCALE; i++)
	{
		blend_row<x_type>(self, t_fx, t_fx + dx, t_buffer);
		t_buffer += t_bufflen;
		self->x_inc += self->x_coef_b;
		self->y_inc += self->y_coef_b;
	}
	
	self->x_coef_a = x_a; self->x_coef_b = x_b; self->x_inc = x_inc;
	self->y_coef_a = y_a; self->y_coef_b = y_b; self->y_inc = y_inc;
	
	uint4 i = 0;
	for (; fx < tx; fx++)
	{
		uint4 u = (self->buffer[i*2] & 0xFF00FF) + (self->buffer[i*2 + 1] & 0xFF00FF) + \
		(self->buffer[t_bufflen + i*2] & 0xFF00FF) + (self->buffer[t_bufflen + i*2 + 1] & 0xFF00FF);
		uint4 v = ((self->buffer[i*2] >> 8) & 0xFF00FF) + ((self->buffer[i*2 + 1] >> 8) & 0xFF00FF) + \
		((self->buffer[t_bufflen + i*2] >> 8) & 0xFF00FF) + ((self->buffer[t_bufflen + i*2 + 1] >> 8) & 0xFF00FF);
		u = (u >> 2) & 0xFF00FF;
		v = (v << 6) & 0xFF00FF00;
		
		i++;
		
		s = u | v;
		uint1 alpha = (s >> 24);
		d[fx] = packed_bilinear_bounded(d[fx], 255 - alpha, s | 0xFF000000, alpha);
	}
}

////////////////////////////////////////////////////////////////////////////////

void MCGradientFillDeleteCombiner(MCGradientAffineCombiner *p_combiner)
{
	if (p_combiner == nil)
		return;
	
	MCMemoryDeleteArray(p_combiner->ramp);
	MCMemoryDeleteArray(p_combiner->buffer);
	MCMemoryDelete(p_combiner);
}

MCGradientAffineCombiner *MCGradientFillCreateCombiner(MCGGradientRef p_gradient_ref, const MCGRectangle &p_clip, const MCGAffineTransform &p_transform)
{
    // MM-2014-07-31: [[ ThreadedRendering ]] Removed use of single static combiner to make things thread safe.
	MCAutoCustomPointer<MCGradientAffineCombiner, MCGradientFillDeleteCombiner> t_combiner;
	if (!MCMemoryNew(&t_combiner))
		return nil;
	
	(*t_combiner) -> begin = gradient_combiner_begin;
	(*t_combiner) -> advance = gradient_affine_combiner_advance;
	(*t_combiner) -> combine = NULL;
	
    MCGAffineTransform t_matrix;
    MCGAffineTransformFromSkMatrix(p_gradient_ref -> transform, t_matrix);
    
    MCGAffineTransform t_transform;
    t_transform = MCGAffineTransformConcat(p_transform, t_matrix);
    
    int4 vx = (int4) t_transform . a;
    int4 vy = (int4) t_transform . b;
    int4 wx = (int4) t_transform . c;
    int4 wy = (int4) t_transform . d;
	
	int4 d = vy * wx - vx *wy;
	
	uint1 t_kind;
	switch (p_gradient_ref -> function)
	{
		case kMCGGradientFunctionLinear:
			t_kind = kMCGradientKindLinear;
			break;
		case kMCGGradientFunctionRadial:
			t_kind = kMCGradientKindRadial;
			break;
		case kMCGGradientFunctionSweep:
			t_kind = kMCGradientKindConical;
			break;
		case kMCGLegacyGradientDiamond:
			t_kind = kMCGradientKindDiamond;
			break;
		case kMCGLegacyGradientSpiral:
			t_kind = kMCGradientKindSpiral;
			break;
		case kMCGLegacyGradientXY:
			t_kind = kMCGradientKindXY;
			break;
		case kMCGLegacyGradientSqrtXY:
			t_kind = kMCGradientKindSqrtXY;
			break;
			
		default:
			// Unrecognised gradient type
			return nil;
	}
	
	if (!MCMemoryNewArray(p_gradient_ref -> ramp_length, (*t_combiner)->ramp))
		return nil;
	
	uint32_t i;
	for (i = 0; i < p_gradient_ref -> ramp_length; i++)
	{
		(*t_combiner)->ramp[i] . offset = (uint4) (p_gradient_ref -> stops[i] * STOP_INT_MAX);
		(*t_combiner)->ramp[i] . color = p_gradient_ref -> colors[i];
		
		if (i != 0)
		{
			// MM-2013-11-20: [[ Bug 11479 ]] Make sure we don't divide by zero.
			if ((*t_combiner)->ramp[i] . offset != (*t_combiner)->ramp[i - 1] . offset)
				(*t_combiner)->ramp[i - 1] . difference = (uint4) (STOP_DIFF_MULT / ((*t_combiner)->ramp[i] . offset - (*t_combiner)->ramp[i - 1] . offset));
			else
				(*t_combiner)->ramp[i - 1] . difference = (uint4) (STOP_DIFF_MULT / STOP_INT_MAX);
		}
		// AL-2014-07-21: [[ Bug 12867 ]] Ensure RBGA values are always packed in native format
		(*t_combiner)->ramp[i] . hw_color = MCGPixelToNative(kMCGPixelFormatBGRA, (*t_combiner)->ramp[i] . color);
	}
	
	// MW-2013-10-26: [[ Bug 11315 ]] Index shuold be i - 1 (otherwise memory overrun occurs!).
	(*t_combiner)->ramp[i - 1] . difference = (uint4) (STOP_DIFF_MULT / STOP_INT_MAX);
	
    (*t_combiner) -> origin . x = (int2) t_transform . tx;
    (*t_combiner) -> origin . y = (int2) t_transform . ty;
	(*t_combiner) -> ramp_length = p_gradient_ref -> ramp_length;
	(*t_combiner) -> mirror = p_gradient_ref -> mirror;
	(*t_combiner) -> repeat = p_gradient_ref -> repeats;
	(*t_combiner) -> wrap = p_gradient_ref -> wrap;
	
	if (d != 0)
	{
		(*t_combiner) -> x_coef_a = STOP_INT_MAX * -wy / d;
		(*t_combiner) -> x_coef_b = STOP_INT_MAX * wx / d;
		(*t_combiner) -> x_inc = (uint4) (STOP_INT_MAX * (int64_t)((*t_combiner) -> origin . x * wy + ((int64_t) p_clip . origin .y - (*t_combiner) -> origin . y) * wx) / d);
		
		(*t_combiner) -> y_coef_a = STOP_INT_MAX * vy / d;
		(*t_combiner) -> y_coef_b = STOP_INT_MAX * -vx / d;
		(*t_combiner) -> y_inc = (uint4) (STOP_INT_MAX * -(int64_t)((*t_combiner) -> origin . x * vy + ((int64_t) p_clip . origin .y - (*t_combiner) -> origin . y) * vx) / d);
	}
    
    // MM-2014-01-27: [[ UpdateImageFilters ]] Updated to use new libgraphics image filter types.
	switch (p_gradient_ref -> filter)
	{
		case kMCGImageFilterNone:
		{
			(*t_combiner) -> end = gradient_combiner_end;
			(*t_combiner) -> x_inc += ((*t_combiner) -> x_coef_a + (*t_combiner) -> x_coef_b) >> 1;
			(*t_combiner) -> y_inc += ((*t_combiner) -> y_coef_a + (*t_combiner) -> y_coef_b) >> 1;
			switch (t_kind)
			{
				case kMCGradientKindConical:
					(*t_combiner) -> combine = MCGradientFillCombine<kMCGradientKindConical>;
					return t_combiner.Release();
				case kMCGradientKindLinear:
					(*t_combiner) -> combine = MCGradientFillCombine<kMCGradientKindLinear>;
					return t_combiner.Release();
				case kMCGradientKindRadial:
					(*t_combiner) -> combine = MCGradientFillCombine<kMCGradientKindRadial>;
					return t_combiner.Release();
				case kMCGradientKindDiamond:
					(*t_combiner) -> combine = MCGradientFillCombine<kMCGradientKindDiamond>;
					return t_combiner.Release();
				case kMCGradientKindSpiral:
					(*t_combiner) -> combine = MCGradientFillCombine<kMCGradientKindSpiral>;
					return t_combiner.Release();
				case kMCGradientKindXY:
					(*t_combiner) -> combine = MCGradientFillCombine<kMCGradientKindXY>;
					return t_combiner.Release();
				case kMCGradientKindSqrtXY:
					(*t_combiner) -> combine = MCGradientFillCombine<kMCGradientKindSqrtXY>;
					return t_combiner.Release();
			}
			break;
		}
			
		case kMCGImageFilterLow:
		case kMCGImageFilterMedium:
        case kMCGImageFilterHigh:
		{
			(*t_combiner) -> buffer_width = GRADIENT_AA_SCALE * (uint32_t) ceilf(p_clip . size . width);
			
			if (!MCMemoryNewArray(GRADIENT_AA_SCALE * (*t_combiner) -> buffer_width, (*t_combiner) -> buffer))
				return nil;
			
			(*t_combiner) -> x_inc += ((*t_combiner) -> x_coef_a + (*t_combiner) -> x_coef_b) >> 2;
			(*t_combiner) -> y_inc += ((*t_combiner) -> y_coef_a + (*t_combiner) -> y_coef_b) >> 2;
			switch (t_kind)
			{
				case kMCGradientKindConical:
					(*t_combiner) -> combine = MCGradientFillBilinearCombine<kMCGradientKindConical>;
					return t_combiner.Release();
				case kMCGradientKindLinear:
					(*t_combiner) -> combine = MCGradientFillBilinearCombine<kMCGradientKindLinear>;
					return t_combiner.Release();
				case kMCGradientKindRadial:
					(*t_combiner) -> combine = MCGradientFillBilinearCombine<kMCGradientKindRadial>;
					return t_combiner.Release();
				case kMCGradientKindDiamond:
					(*t_combiner) -> combine = MCGradientFillBilinearCombine<kMCGradientKindDiamond>;
					return t_combiner.Release();
				case kMCGradientKindSpiral:
					(*t_combiner) -> combine = MCGradientFillBilinearCombine<kMCGradientKindSpiral>;
					return t_combiner.Release();
				case kMCGradientKindXY:
					(*t_combiner) -> combine = MCGradientFillBilinearCombine<kMCGradientKindXY>;
					return t_combiner.Release();
				case kMCGradientKindSqrtXY:
					(*t_combiner) -> combine = MCGradientFillBilinearCombine<kMCGradientKindSqrtXY>;
					return t_combiner.Release();
				default:
					return NULL;
			}
		}
	}
	
	return NULL;
}

////////////////////////////////////////////////////////////////////////////////

MCGLegacyGradientShader::MCGLegacyGradientShader(MCGGradientRef p_gradient_ref, MCGRectangle p_clip)
{
	m_gradient_ref = MCGGradientRetain(p_gradient_ref);
	m_clip = p_clip;
}

MCGLegacyGradientShader::~MCGLegacyGradientShader()
{
	MCGGradientRelease(m_gradient_ref);
}

size_t MCGLegacyGradientShader::onContextSize(const ContextRec&) const
{
    return sizeof(MCGLegacyGradientShader::MCGLegacyGradientShaderContext);
}

SkShader::Context* MCGLegacyGradientShader::onCreateContext(const ContextRec& p_rec, void* p_storage) const
{
    return new (nothrow) MCGLegacyGradientShaderContext(*this, p_rec, m_gradient_ref, m_clip);
}

#ifndef SK_IGNORE_TO_STRING
void MCGLegacyGradientShader::toString(SkString* p_str) const
{
    p_str->append("MCGLegacyGradientShader: ()");
}
#endif

sk_sp<SkFlattenable> MCGLegacyGradientShader::CreateProc(SkReadBuffer& buffer)
{
    return NULL;
}

MCGLegacyGradientShader::MCGLegacyGradientShaderContext::MCGLegacyGradientShaderContext(const MCGLegacyGradientShader& p_shader, const ContextRec& p_rec, MCGGradientRef p_gradient_ref, MCGRectangle p_clip)
: INHERITED(p_shader, p_rec), m_y(0), m_gradient_combiner(NULL)
{
    MCGRectangle t_clip;
    
	MCGAffineTransform t_matrix;
	MCGAffineTransformFromSkMatrix(*p_rec.fMatrix, t_matrix);
	t_clip = MCGRectangleApplyAffineTransform(p_shader.m_clip, t_matrix);
	
	uint32_t t_width;
	t_width = (uint32_t) ceilf(p_shader.m_clip . size . width);
	
	bool t_success;
	t_success = true;
	
	if (t_success)
	{	
		m_gradient_combiner = MCGradientFillCreateCombiner(p_gradient_ref, t_clip, t_matrix);
		t_success = m_gradient_combiner != NULL;		
	}
	
	if (t_success)
	{
		int32_t t_y;
		t_y = (int32_t) t_clip . origin . y;
		m_gradient_combiner -> begin(m_gradient_combiner, t_y);
		m_y = t_y;
	}
}

MCGLegacyGradientShader::MCGLegacyGradientShaderContext::~MCGLegacyGradientShaderContext()
{
    MCGradientFillDeleteCombiner(m_gradient_combiner);
}

void MCGLegacyGradientShader::MCGLegacyGradientShaderContext::shadeSpan(int x, int y, SkPMColor dstC[], int count)
{
	if (m_gradient_combiner != NULL)
	{
		int32_t t_dy;
		t_dy = y - m_y;
		m_gradient_combiner -> advance(m_gradient_combiner, t_dy);
		m_y = y;
		memset(dstC, 0x00, count * sizeof(SkPMColor));
		m_gradient_combiner -> bits = dstC - x;
		m_gradient_combiner -> combine(m_gradient_combiner, x, x + count);
	}
}

////////////////////////////////////////////////////////////////////////////////

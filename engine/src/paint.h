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

#ifndef __MC_PAINT__
#define __MC_PAINT__

#include "typedefs.h"
#include "path.h"
#include "gradient.h"

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


#endif

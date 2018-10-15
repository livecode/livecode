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

#ifndef __MC_GRADIENT__
#define __MC_GRADIENT__

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

enum MCGradientFillQuality
{
	kMCGradientQualityNormal,
	kMCGradientQualityGood
};

enum MCGradientFillProperty
{
	P_GRADIENT_FILL_TYPE,
	P_GRADIENT_FILL_RAMP,
	P_GRADIENT_FILL_ORIGIN,
	P_GRADIENT_FILL_PRIMARY_POINT,
	P_GRADIENT_FILL_SECONDARY_POINT,
	P_GRADIENT_FILL_QUALITY,
	P_GRADIENT_FILL_MIRROR,
	P_GRADIENT_FILL_REPEAT,
	P_GRADIENT_FILL_WRAP,
};

struct MCGradientFillStop
{
	uint4 offset;
	uint4 color;
	uint4 hw_color;
	uint4 difference;
};

struct MCGradientFill
{
	unsigned kind : 4;
	unsigned quality : 2;
	unsigned mirror : 1;
	unsigned wrap : 1;
	uint1 repeat;
	uint1 ramp_length;
	MCPoint origin;
	MCPoint primary;
	MCPoint secondary;
	MCPoint old_origin;
	MCPoint old_primary;
	MCPoint old_secondary;
	MCRectangle old_rect;
	MCGradientFillStop *ramp;
};

#define GRADIENT_HEADER_SIZE (1 + 1 + 1 + 3 * sizeof(MCPoint))

// MM-2013-03-04: Moved from gradient.cpp into header.
#define STOP_DIFF_PRECISION 24
#define STOP_DIFF_MULT ((1 << STOP_DIFF_PRECISION) * (uint4)255)
#define STOP_INT_PRECISION 16
#define STOP_INT_MAX ((1 << STOP_INT_PRECISION) - 1)
#define STOP_INT_MIRROR_MAX ((2 << STOP_INT_PRECISION) - 1)
#define GRADIENT_ROUND_EPSILON (float)0.000005

#define GRADIENT_AA_SCALE (2)

class MCString;
struct MCGradientCombiner;

void MCGradientFillInit(MCGradientFill *&r_gradient, MCRectangle p_rect);
void MCGradientFillFree(MCGradientFill *gradient);
MCGradientFill *MCGradientFillCopy(const MCGradientFill *p_gradient);

bool MCGradientFillGetProperties(MCExecContext& ctxt, MCGradientFill* p_gradient, MCExecValue& r_array);
bool MCGradientFillSetProperties(MCExecContext& ctxt, MCGradientFill* &x_gradient, MCRectangle rect, MCExecValue p_array, bool& r_dirty);
bool MCGradientFillGetElement(MCExecContext& ctxt, MCGradientFill* p_gradient, MCNameRef p_prop, MCExecValue& r_value);
bool MCGradientFillSetElement(MCExecContext& ctxt, MCGradientFill* &x_gradient, MCNameRef p_prop, MCRectangle rect, MCExecValue p_value, bool& r_dirty);

Boolean MCGradientFillRampParse(MCGradientFillStop* &r_stops, uint1 &r_stop_count, MCStringRef p_data);
bool MCGradientFillRampUnparse(MCGradientFillStop* p_stops, uint1 p_stop_count, MCStringRef &r_data);

MCGradientCombiner *MCGradientFillCreateCombiner(MCGradientFill *p_gradient, MCRectangle &p_clip);
void MCGradientFillUnserialize(MCGradientFill *p_gradient, uint1 *p_data, uint4 &r_length);

IO_stat MCGradientFillSerialize(MCGradientFill *p_gradient, MCObjectOutputStream& p_stream);
IO_stat MCGradientFillUnserialize(MCGradientFill *p_gradient, MCObjectInputStream& p_stream);
uint4 MCGradientFillMeasure(MCGradientFill *p_gradient);

#endif

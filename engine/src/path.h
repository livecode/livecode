/* Copyright (C) 2003-2013 Runtime Revolution Ltd.

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

#ifndef __PATH_H
#define __PATH_H

struct MCShape;
struct MCStrokeStyle;

struct MCCombiner
{
	void (*begin)(MCCombiner *self, int4 y);
	void (*advance)(MCCombiner *self, int4 y);
	void (*blend)(MCCombiner *self, int4 fx, int4 tx, uint1 value);
	void (*combine)(MCCombiner *self, int4 fx, int4 tx, uint1 *mask);
	void (*end)(MCCombiner *self);
};

class MCPath
{
public:
	void retain(void);
	void release(void);

	void fill(MCCombiner *p_combiner, const MCRectangle& clip, bool p_even_odd);
	void stroke(MCCombiner *p_combiner, const MCRectangle& clip, MCStrokeStyle *p_stroke);

	static void fill(uint1 *p_commands, int4 *p_data, MCCombiner *p_combiner, const MCRectangle& p_clip, bool p_even_odd);
	static void stroke(uint1 *p_commands, int4 *p_data, MCCombiner *p_combiner, const MCRectangle& p_clip, MCStrokeStyle *p_stroke);

	static MCPath *create_path(uint1 *commands, uint32_t command_count, int4 *ordinates, uint32_t ordinate_count);
	static MCPath *create_line(int2 fx, int2 fy, int2 tx, int2 ty, bool adjust);
	static MCPath *create_polyline(MCPoint *points, uint2 count, bool adjust);
	static MCPath *create_polypolyline(MCLineSegment *segments, uint2 count, bool adjust);
	static MCPath *create_rectangle(const MCRectangle& rect, bool adjust);
	static MCPath *create_rounded_rectangle(const MCRectangle& rect, uint2 radius, bool adjust);
	static MCPath *create_polygon(MCPoint *points, uint2 count, bool adjust);
	static MCPath *create_polypolygon(MCPoint *points, uint2 count, bool adjust);
	static MCPath *create_arc(const MCRectangle& rect, uint2 p_start, uint2 p_end, bool adjust);
	static MCPath *create_segment(const MCRectangle& rect, uint2 p_start, uint2 p_end, bool adjust);
	static MCPath *create_dot(int2 x, int2 y, bool adjust);
	static MCPath *create_empty(void);

	MCPath *copy_scaled(int4 p_scale);

	void get_lengths(uint4 &r_commands, uint4 &r_points);
	uint1 *get_commands(void) {return f_commands;}
	int4 *get_ordinates(void) {return f_data;}

private:
	uint4 f_references;
	uint1 *f_commands;
	int4 *f_data;

	static MCPath *allocate(uint4 commands, uint4 points);
	static void cache(MCPath*& pr_cache, uint4 commands, uint4 points);
};

#endif

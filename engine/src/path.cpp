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

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"


#include "dispatch.h"
#include "image.h"
#include "stack.h"
#include "util.h"
#include "variable.h"

#include "globals.h"

#include "path.h"

#include "meta.h"

#include "pathprivate.h"
#include "context.h"

#ifdef _WIN32
#include <crtdbg.h>
#endif

//-----------------------------------------------------------------------------
// Private Definitions
//

//-----------------------------------------------------------------------------
//  Path Construction Operators
//

static inline void __path_append_command(uint1*& pr_commands, int4*& pr_data, uint1 p_command)
{
	*pr_commands++ = p_command;
}

static inline void __path_append_point(int4*& pr_data, int4 x, int4 y, bool adjust = false)
{
	if (adjust)
		x += 1, y += 1;
	*pr_data++ = x << 7;
	*pr_data++ = y << 7;
}

// (Rcb, Rsb
static inline void __path_append_arc(uint1*& pr_commands, int4*& pr_data, int4 cx, int4 cy, int4 hr, int4 vr, int4 s, int4 e, bool first)
{
	int4 hk, vk;

	if (e - s == 90)
	{
		hk = hr * 36195 / 65536;
		vk = vr * 36195 / 65536;
	}
	else
	{
		double h = tan(M_PI * (e - s) / 720.0);
		hk = int4(4.0 * h * hr / 3.0);
		vk = int4(4.0 * h * vr / 3.0);
	}

	double ca, sa;
	double cb, sb;

	ca = cos(M_PI * s / 180);
	sa = sin(M_PI * s / 180);

	cb = cos(M_PI * e / 180);
	sb = sin(M_PI * e / 180);

	if (first)
	{
		__path_append_point(pr_data, int4(cx + hr * ca), int4(cy - vr * sa));
		*pr_commands++ = PATH_COMMAND_MOVE_TO;
	}

	__path_append_point(pr_data, int4(cx + hr * ca - hk * sa), int4(cy - vr * sa - vk * ca));
	__path_append_point(pr_data, int4(cx + hr * cb + hk * sb), int4(cy - vr * sb + vk * cb));
	__path_append_point(pr_data, int4(cx + hr * cb), int4(cy - vr * sb));

	*pr_commands++ = PATH_COMMAND_CUBIC_TO;
}

#define PATH_BEGIN(t_path) \
	if (t_path != NULL) \
	{ \
		uint1 *__command_ptr = t_path -> f_commands; \
		int4 *__data_ptr = t_path -> f_data; \

#define PATH_MOVE_TO(x, y) \
		__path_append_point(__data_ptr, x, y), __path_append_command(__command_ptr, __data_ptr, PATH_COMMAND_MOVE_TO)

#define PATH_LINE_TO(x, y) \
		__path_append_point(__data_ptr, x, y), __path_append_command(__command_ptr, __data_ptr, PATH_COMMAND_LINE_TO)

#define PATH_MOVE_TO_I(x, y, a) \
		__path_append_point(__data_ptr, (x) << 1, (y) << 1, a), __path_append_command(__command_ptr, __data_ptr, PATH_COMMAND_MOVE_TO)

#define PATH_LINE_TO_I(x, y, a) \
		__path_append_point(__data_ptr, (x) << 1, (y) << 1, a), __path_append_command(__command_ptr, __data_ptr, PATH_COMMAND_LINE_TO)

#define PATH_CUBIC_TO(ax, ay, bx, by, x, y) \
		__path_append_point(__data_ptr, ax, ay), __path_append_point(__data_ptr, bx, by), __path_append_point(__data_ptr, x, y), __path_append_command(__command_ptr, __data_ptr, PATH_COMMAND_CUBIC_TO)

#define PATH_QUADRATIC_TO(cx, cy, x, y) \
		__path_append_point(__data_ptr, cx, cy), __path_append_point(__data_ptr, x, y), __path_append_command(__command_ptr, __data_ptr, PATH_COMMAND_QUADRATIC_TO)

#define PATH_CHECK(p, x, y) \
		assert(__command_ptr <= p -> f_commands + x && __data_ptr <= p -> f_data + y * 2)

#define PATH_CLOSE \
		__path_append_command(__command_ptr, __data_ptr, PATH_COMMAND_CLOSE)

#define PATH_END \
		__path_append_command(__command_ptr, __data_ptr, PATH_COMMAND_END); \
	}


#define PATH_ARC(cx, cy, hr, vr, s, e, f) \
		__path_append_arc(__command_ptr, __data_ptr, cx, cy, hr, vr, s, e, f)

//-----------------------------------------------------------------------------
//  Path Implementation
//

void MCPath::release(void)
{
	f_references -= 1;
	if (f_references == 0)
		free(this);
}

void MCPath::retain(void)
{
	f_references += 1;
}

MCPath *MCPath::allocate(uint4 p_command_count, uint4 p_point_count)
{
	// MW-2013-05-02: [[ x64 ]] Make sure we use the actual size of MCPath
	//   rather than 12 - as it changes in 64-bit (MCPath contains two
	//   pointers).
	MCPath *t_path;
	t_path = (MCPath *)malloc(sizeof(MCPath) + ((p_command_count + 4) & ~3) + p_point_count * 8);
	t_path -> f_references = 1;
	t_path -> f_commands = (uint1 *)t_path + sizeof(MCPath);
	t_path -> f_data = (int4 *)t_path + sizeof(MCPath) / sizeof(int4) + (((p_command_count + 4) & ~3) / 4);
	return t_path;
}

inline void MCPath::cache(MCPath*& pr_cache, uint4 p_command_count, uint4 p_point_count)
{
	if (pr_cache != NULL && pr_cache -> f_references > 1)
	{
		pr_cache -> release();
		pr_cache = NULL;
	}

	if (pr_cache == NULL)
		pr_cache = allocate(p_command_count, p_point_count);

	pr_cache -> retain();
}

MCPath *MCPath::create_path(uint1 *p_commands, uint32_t p_command_count, int4 *p_ordinates, uint32_t p_ordinate_count)
{
	MCPath *t_path;
	t_path = allocate(p_command_count, p_ordinate_count);
	if (t_path == NULL)
		return NULL;

	memcpy(t_path -> f_commands, p_commands, p_command_count);
	memcpy(t_path -> f_data, p_ordinates, p_ordinate_count * sizeof(int4));

	return t_path;
}

MCPath *MCPath::create_line(int2 fx, int2 fy, int2 tx, int2 ty, bool adjust)
{
	static Meta::static_ptr_t<MCPath> s_line_cache;
	MCPath *t_line;

	t_line = s_line_cache;
	cache(t_line, 2, 2);
	s_line_cache = t_line;

	PATH_BEGIN(t_line)
		PATH_MOVE_TO_I(fx, fy, adjust);
		PATH_LINE_TO_I(tx, ty, adjust);
		
		PATH_CHECK(t_line, 2, 2);
	PATH_END

	return s_line_cache;
}

MCPath *MCPath::create_polyline(MCPoint *points, uint2 count, bool adjust)
{
	MCPath *t_path;
	uint2 t_count;
	
	t_count = count;
	t_path = allocate(t_count, t_count);

	PATH_BEGIN(t_path)
		PATH_MOVE_TO_I(points -> x, points -> y, adjust);
		points++;
		while(--count > 0)
		{
			PATH_LINE_TO_I(points -> x, points -> y, adjust);
			points++;
		}
		
		PATH_CHECK(t_path, t_count, t_count);
	PATH_END

	return t_path;
}

MCPath *MCPath::create_polypolyline(MCLineSegment *segments, uint2 count, bool adjust)
{
	MCPath *t_path;
	uint2 t_count;

	t_count = count;
	t_path = allocate(t_count * 2, t_count * 2);

	PATH_BEGIN(t_path)
		while(count-- > 0)
		{
			PATH_MOVE_TO_I(segments -> x1, segments -> y1, adjust);
			PATH_LINE_TO_I(segments -> x2, segments -> y2, adjust);
			segments++;
		}

		PATH_CHECK(t_path, t_count * 2, t_count * 2);
	PATH_END

	return t_path;
}

MCPath *MCPath::create_rectangle(const MCRectangle& rect, bool adjust)
{
	static Meta::static_ptr_t<MCPath> s_rectangle_cache;
	MCPath *t_rectangle;

	if (rect . width == 0 || rect . height == 0)
		return create_empty();

	t_rectangle = s_rectangle_cache;
	cache(t_rectangle, 5, 4);
	s_rectangle_cache = t_rectangle;

	int4 w, h;
	w = rect . width;
	h = rect . height;

	if (adjust)
		w -= 1, h -= 1;

	PATH_BEGIN(t_rectangle)
		PATH_MOVE_TO_I(rect . x, rect . y, adjust);
		PATH_LINE_TO_I(rect . x + w, rect . y, adjust);
		PATH_LINE_TO_I(rect . x + w, rect . y + h, adjust);
		PATH_LINE_TO_I(rect . x, rect . y + h, adjust);
		PATH_CLOSE;
		
		PATH_CHECK(t_rectangle, 5, 4);
	PATH_END

	return s_rectangle_cache;
}

// We approximate the circular corners by cubic bezier curves. For an arc radius R
// from angle A to angle B, the spline control points must be:
//
//   (R * cos(A), R * sin(A))
//   (R * cos(A) - h * sin(A), R * sin(A) + h * cos(A))
//   (R * cos(B) + h * sin(B), R * sin(B) - h * cos(B))
//   (R * cos(B), R * sin(B))
//
// Where
//   h = 4 / 3 * tan(angle / 4)
//
// Upper-Right Quadrant - A = 0, B = 90:
//   cos(A) = 1, sin(A) = 0
//   cos(B) = 0, sin(B) = 1
//
//     (R, 0), (R, -h), (h, -R), (0, -R)
//
// Lower-Right Quadrant - A = -90, B = 0:
//     (R, 0), (R, h), (h, R), (0, R)
// Lower-Left Quadrant:
//     (0, R), (-h, R), (-R, h), (-R, 0)
// Top-Left Quadrant:
//     (-R, 0), (-R, -h), (-h, -R), (0, -R)
//
MCPath *MCPath::create_rounded_rectangle(const MCRectangle& rect, uint2 radius, bool adjust)
{
	static Meta::static_ptr_t<MCPath> s_round_rectangle_cache;
	MCPath *t_round_rectangle;

	if (rect . width == 0 || rect . height == 0)
		return create_empty();

	if (adjust && (rect . width <= 1 || rect . height <= 1))
		return create_empty();

	if (radius == 0)
		return create_rectangle(rect, adjust);

	t_round_rectangle = s_round_rectangle_cache;
	cache(t_round_rectangle, 9, 16);
	s_round_rectangle_cache = t_round_rectangle;

	int4 x, y;
	x = rect . x * 2;
	y = rect . y * 2;

	int4 aw, ah;
	aw = rect . width * 2;
	ah = rect . height * 2;

	if (adjust)
		x += 1, y += 1, aw -= 2, ah -= 2;

	int4 hr, vr;
	hr = MCU_min(aw / 2, radius * 2);
	vr = MCU_min(ah / 2, radius * 2);

	int4 h, v;
	h = aw - hr * 2;
	v = ah - vr * 2;

	int4 l, t, r, b;
	l = x + hr;
	t = y + vr;
	r = x + hr + h;
	b = y + vr + v;	

	int4 hk, vk;
	hk = signed(hr * 36195 / 65536);
	vk = signed(vr * 36195 / 65536);

	if (hk == 0 && vk == 0)
		return create_rectangle(rect, adjust);

 	PATH_BEGIN(t_round_rectangle)
		PATH_MOVE_TO(r, t - vr);
		PATH_CUBIC_TO(r + hk, t - vr, r + hr, t - vk, r + hr, t);
		PATH_LINE_TO(r + hr, b);
		PATH_CUBIC_TO(r + hr, b + vk, r + hk, b + vr, r, b + vr);
		PATH_LINE_TO(l, b + vr);
		PATH_CUBIC_TO(l - hk, b + vr, l - hr, b + vk, l - hr, b);
		PATH_LINE_TO(l - hr, t);
		PATH_CUBIC_TO(l - hr, t - vk, l - hk, t - vr, l, t - vr);
		PATH_CLOSE;

		PATH_CHECK(t_round_rectangle, 9, 16);
	PATH_END

	return s_round_rectangle_cache;
}

MCPath *MCPath::create_polygon(MCPoint *points, uint2 count, bool adjust)
{
	MCPath *t_path;
	uint2 t_count;
	
	t_count = count;
	t_path = allocate(t_count + 1, t_count);

	PATH_BEGIN(t_path)
		PATH_MOVE_TO_I(points -> x, points -> y, adjust);
		points++;
		while(--count > 0)
		{
			PATH_LINE_TO_I(points -> x, points -> y, adjust);
			points++;
		}
		PATH_CLOSE;
		PATH_CHECK(t_path, t_count + 1, t_count);
	PATH_END

	return t_path;
}

MCPath *MCPath::create_polypolygon(MCPoint *points, uint2 count, bool adjust)
{
	MCPath *t_path;
	uint2 t_count;
	
	t_count = count;
	t_path = allocate(t_count + 1, t_count);

	bool t_close;
	t_close = false;

	PATH_BEGIN(t_path)
		PATH_MOVE_TO_I(points -> x, points -> y, adjust);
		points++;
		while(--count > 0)
		{
			if (points -> x == MININT2)
				t_close = true;
			else
			{
				if (t_close)
				{
					PATH_MOVE_TO_I(points -> x, points -> y, adjust);
					t_close = false;
				}
				else
					PATH_LINE_TO_I(points -> x, points -> y, adjust);
			}

			points++;
		}
		PATH_CLOSE;
		PATH_CHECK(t_path, t_count + 1, t_count);
	PATH_END

	return t_path;
}

MCPath *MCPath::create_arc(const MCRectangle& rect, uint2 p_start, uint2 p_angle, bool adjust)
{
	static Meta::static_ptr_t<MCPath> s_arc_cache;
	MCPath *t_arc;

	if (rect . width == 0 || rect . height == 0)
		return create_empty();

	if (p_angle == 0)
		return create_empty();

	t_arc = s_arc_cache;
	cache(t_arc, 7, 16);
	s_arc_cache = t_arc;

	if (p_angle > 360)
		p_angle = 360;

	bool closed = (p_angle == 360);

	p_start = p_start % 360;

	int4 cx, cy, hr, vr;
	cx = rect . x * 2 + rect . width;
	cy = rect . y * 2 + rect . height;
	hr = rect . width;
	vr = rect . height;

	if (adjust)
		hr -= 1, vr -= 1;

	bool t_first = true;
	int4 t_delta;

	PATH_BEGIN(t_arc)
		while(p_angle > 0)
		{
			t_delta = MCU_min(90 - p_start % 90, p_angle);
			p_angle -= t_delta;
			PATH_ARC(cx, cy, hr, vr, p_start, p_start + t_delta, t_first);
			p_start += t_delta;
			t_first = false;
		}
		if (closed)
			PATH_CLOSE;
		PATH_CHECK(t_arc, 7, 16);
	PATH_END

	return s_arc_cache;
}

MCPath *MCPath::create_segment(const MCRectangle& rect, uint2 p_start, uint2 p_angle, bool adjust)
{
	static Meta::static_ptr_t<MCPath> s_segment_cache;
	MCPath *t_segment;

	if (rect . width == 0 || rect . height == 0)
		return create_empty();

	if (p_angle == 0)
		return create_empty();

	t_segment = s_segment_cache;
	cache(t_segment, 8, 17);
	s_segment_cache = t_segment;

	if (p_angle > 360)
		p_angle = 360;

	p_start = p_start % 360;

	int4 cx, cy, hr, vr;
	cx = rect . x * 2 + rect . width;
	cy = rect . y * 2 + rect . height;
	hr = rect . width;
	vr = rect . height;

	if (adjust)
		hr -= 1, vr -= 1;

	bool t_first = true;
	int4 t_delta;

	PATH_BEGIN(t_segment)
		while(p_angle > 0)
		{
			t_delta = MCU_min(90 - p_start % 90, p_angle);
			p_angle -= t_delta;
			PATH_ARC(cx, cy, hr, vr, p_start, p_start + t_delta, t_first);
			p_start += t_delta;
			t_first = false;
		}
		PATH_LINE_TO(cx, cy);
		PATH_CLOSE;

		PATH_CHECK(t_segment, 8, 17);
	PATH_END

	return s_segment_cache;
}

MCPath *MCPath::create_dot(int2 x, int2 y, bool adjust)
{
	static Meta::static_ptr_t<MCPath> s_dot_cache;
	MCPath *t_dot;

	t_dot = s_dot_cache;
	cache(t_dot, 3, 2);
	s_dot_cache = t_dot;

	if (t_dot != NULL)
	{
		uint1 *t_commands = t_dot -> f_commands;
		int4 *t_data = t_dot -> f_data;
		
		// MW-2012-05-31: [[ Bug ]] Crash caused by failing to terminate path thus
		//   causing issues if memory block was not zero before hand.
		*t_commands++ = PATH_COMMAND_MOVE_TO;
		*t_commands++ = PATH_COMMAND_LINE_TO;
		*t_commands++ = PATH_COMMAND_END;
		
		*t_data++ = ((int4)x << 8) + 127;
		*t_data++ = ((int4)y << 8) + 128;
		*t_data++ = ((int4)x << 8) + 129;
		*t_data++ = ((int4)y << 8) + 128;
	}

	return s_dot_cache;
}

MCPath *MCPath::create_empty(void)
{
	static Meta::static_ptr_t<MCPath> s_empty_cache;

	if (s_empty_cache == NULL)
	{
		MCPath *t_empty;
		s_empty_cache = allocate(0, 0);
		t_empty = s_empty_cache;
		PATH_BEGIN(t_empty)
		PATH_END
	}

	s_empty_cache -> retain();
	return s_empty_cache;
}

void MCPath::get_lengths(uint4 &r_commands, uint4 &r_points)
{
	r_commands = 1;
	r_points = 0;

	uint1 *t_commandptr = f_commands;
	while (t_commandptr != NULL && *t_commandptr != PATH_COMMAND_END)
	{
		switch (*t_commandptr)
		{
		case PATH_COMMAND_MOVE_TO:
		case PATH_COMMAND_LINE_TO:
			r_points += 1;
			break;
		case PATH_COMMAND_QUADRATIC_TO:
			r_points += 2;
			break;
		case PATH_COMMAND_CUBIC_TO:
			r_points += 3;
			break;
		}
		t_commandptr++;
		r_commands++;
	}
}

MCPath *MCPath::copy_scaled(int4 p_scale)
{
	MCPath *t_path = new (nothrow) MCPath;
	t_path->f_references = 0;

	uint4 t_numpoints, t_numcommands;

	get_lengths(t_numpoints, t_numcommands);

	t_path->f_commands = new (nothrow) uint1[t_numcommands];
	memcpy(t_path->f_commands, f_commands, sizeof(uint1) * t_numcommands);

	t_path->f_data = new (nothrow) int4[t_numpoints * 2];
	for (uint4 i = 0; i < t_numpoints * 2; i++)
		t_path->f_data[i] = f_data[i] * p_scale;

	return t_path;
}

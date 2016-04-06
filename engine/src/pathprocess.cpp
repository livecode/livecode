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
//#include "execpt.h"

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
//-----------------------------------------------------------------------------
//
// The Path Flattening Routines
//

static inline void transform(int4 ix, int4 iy, int4& ox, int4& oy)
{
	ox = ix;
	oy = iy;
}

static inline double distance_to_point(int4 x, int4 y, int4 px, int4 py)
{
	double dx, dy;

	dx = px - x;
	dy = py - y;

	return dx * dx + dy * dy;
}

static int4 is_flat(int4 sx, int4 sy, int4 ex, int4 ey, int4 x, int4 y)
{
	double dx, dy;
	double d;

	dx = ex - sx;
	dy = ey - sy;

	if (dx == 0 && dy == 0)
		d = distance_to_point(x, y, sx, sy);
	else
	{
		double pdx, pdy;
		double u;

		pdx = x - sx;
		pdy = y - sy;

		u = (pdx * dx + pdy * dy) / (dx * dx + dy * dy);

		if (u <= 0)
			d = distance_to_point(x, y, sx, sy);
		else if (u >= 1)
			d = distance_to_point(x, y, ex, ey);
		else
			d = distance_to_point((int4)(sx + u * dx), (int4)(sy + u * dy), x, y);
	}

	return d < 16 * 16;
}

// If we have a MOVE TO followed immediately by a CLOSE, it causes problems!

static inline bool close_path(buffer_t<int4> &r_vertices, uint4 p_last_count)
{
	if (r_vertices.ensure(2))
		return true;
	int4 *t_data = r_vertices.borrow();
	r_vertices.append(t_data[p_last_count]);
	r_vertices.append(t_data[p_last_count + 1]);
	return false;
}

static inline bool append_path_offset(buffer_t<int4> &r_left, buffer_t<int4> &r_right, int4 ox, int4 oy)
{
	uint4 t_size = r_right.size();
	if (r_left.ensure(t_size))
		return true;
	int4* t_data = r_right.borrow();
	for (uint4 i=0; i<t_size; i+=2)
	{
		r_left.append(t_data[i] + ox);
		r_left.append(t_data[i + 1] + oy);
	}
	return false;
}
static inline bool append_path_reverse(buffer_t<int4> &r_left, buffer_t<int4> &r_right)
{
	uint4 t_size = r_right.size();
	if (r_left.ensure(t_size))
		return true;
	int4* t_data = r_right.borrow();
	while (t_size)
	{
		t_size -= 2;
		r_left.append(t_data[t_size]);
		r_left.append(t_data[t_size + 1]);
	}
	return false;
}

static inline bool butt_cap(buffer_t<int4> &r_vertices, MCStrokeStyle *p_stroke, int4 sx, int4 sy, angle_t &s_angle)
{
	uint2 width = p_stroke->width;
	bool err = false;
	int4 s_off_x = (int4) ((width / 2) * -s_angle.sina);
	int4 s_off_y = (int4) ((width / 2) * s_angle.cosa);

	int4 t_lsx = sx + s_off_x;
	int4 t_lsy = sy + s_off_y;
	int4 t_rsx = sx - s_off_x;
	int4 t_rsy = sy - s_off_y;

	err = r_vertices . ensure(4);
	if (!err)
	{
		r_vertices . append(t_lsx), r_vertices . append(t_lsy);
		r_vertices . append(t_rsx), r_vertices . append(t_rsy);
	}
	return err;
}

static inline bool round_cap(buffer_t<int4> &r_vertices, MCStrokeStyle *p_stroke, int4 sx, int4 sy, angle_t &s_angle)
{
	uint2 width = p_stroke->width;
	int4 s_off_x = (int4) ((width / 2) * -s_angle.sina);
	int4 s_off_y = (int4) ((width / 2) * s_angle.cosa);

	int4 t_lsx = sx + s_off_x;
	int4 t_lsy = sy + s_off_y;
	if (r_vertices.ensure(2))
		return true;
	r_vertices . append(t_lsx), r_vertices . append(t_lsy);

	real8 t_step = 256 * 2.0 / width;
	real8 t_current = M_PI / 2 - t_step;
	while (t_current > -M_PI / 2)
	{
		int4 t_x = sx + (int4)(width / 2.0 * cos(s_angle.angle + t_current));
		int4 t_y = sy + (int4)(width / 2.0 * sin(s_angle.angle + t_current));
		t_current -= t_step;

		if (r_vertices.ensure(2))
			return true;
		r_vertices.append(t_x);  r_vertices.append(t_y);
	}
	

	int4 t_rsx = sx - s_off_x;
	int4 t_rsy = sy - s_off_y;

	if (r_vertices . ensure(2))
		return true;
	r_vertices . append(t_rsx), r_vertices . append(t_rsy);

	return false;
}

static inline bool square_cap(buffer_t<int4> &r_vertices, MCStrokeStyle *p_stroke, int4 sx, int4 sy, angle_t &s_angle)
{
	uint2 width = p_stroke->width;
	bool err = false;
	int4 s_off_x = (int4) ((width / 2) * -s_angle.sina);
	int4 s_off_y = (int4) ((width / 2) * s_angle.cosa);

	int4 s_proj_x = (int4) ((width / 2) * s_angle.cosa);
	int4 s_proj_y = (int4) ((width / 2) * s_angle.sina);

	int4 t_lsx = sx + s_proj_x + s_off_x;
	int4 t_lsy = sy + s_proj_y + s_off_y;
	int4 t_rsx = sx + s_proj_x - s_off_x;
	int4 t_rsy = sy + s_proj_y - s_off_y;

	err = r_vertices . ensure(4);
	if (!err)
	{
		r_vertices . append(t_lsx), r_vertices . append(t_lsy);
		r_vertices . append(t_rsx), r_vertices . append(t_rsy);
	}
	return err;
}

static inline bool create_cap(buffer_t<int4> &r_vertices, MCStrokeStyle *p_stroke, int4 sx, int4 sy, angle_t &r_angle)
{
	switch (p_stroke->cap & CapMask)
	{
	case CapButt:
		return butt_cap(r_vertices, p_stroke, sx, sy, r_angle);
	case CapRound:
		return round_cap(r_vertices, p_stroke, sx, sy, r_angle);
	case CapProjecting:
		return square_cap(r_vertices, p_stroke, sx, sy, r_angle);
	default:
		return true;
	}
}

static inline bool curve_join(buffer_t<int4> &r_left_path, buffer_t<int4> &r_right_path, MCStrokeStyle *p_stroke, int4 sx, int4 sy, real8 s_angle, real8 e_angle)
{
	uint2 width = p_stroke->width;

	real8 t_angle = (s_angle + e_angle) / 2;
	if (fabs(t_angle - s_angle) > M_PI / 2)
		t_angle += M_PI;
	real8 t_cos = cos(t_angle);
	real8 t_sin = sin(t_angle);
	int4 t_off_x = (int4) ((width / 2) * -t_sin);
	int4 t_off_y = (int4) ((width / 2) * t_cos);

	if (r_left_path . ensure(2) || r_right_path.ensure(2))
		return true;

	r_left_path . append(sx + t_off_x), r_left_path . append(sy + t_off_y);
	r_right_path . append(sx - t_off_x), r_right_path . append(sy - t_off_y);

	return false;
}

static inline bool curve_join(buffer_t<int4> &r_left_path, buffer_t<int4> &r_right_path, MCStrokeStyle *p_stroke, int4 sx, int4 sy, angle_t &s_angle, angle_t &e_angle)
{
	return curve_join(r_left_path, r_right_path, p_stroke, sx, sy, s_angle.angle, e_angle.angle);
}

static inline bool bevel_join(buffer_t<int4> &r_left_path, buffer_t<int4> &r_right_path, MCStrokeStyle *p_stroke, int4 sx, int4 sy, angle_t &s_angle, angle_t &e_angle)
{
	uint2 width = p_stroke->width;
	int4 s_off_x = (int4) ((width / 2) * -s_angle.sina);
	int4 s_off_y = (int4) ((width / 2) * s_angle.cosa);

	int4 e_off_x = (int4) ((width / 2) * -e_angle.sina);
	int4 e_off_y = (int4) ((width / 2) * e_angle.cosa);

	if (r_left_path . ensure(6) || r_right_path.ensure(6))
		return true;

	r_left_path . append(sx + s_off_x), r_left_path . append(sy + s_off_y);
	r_right_path . append(sx - s_off_x), r_right_path . append(sy - s_off_y);

	real8 t_angle = s_angle.angle - e_angle.angle;
	if (t_angle < 0)
		t_angle += 2 * M_PI;

	if (t_angle < M_PI)
	{
		r_right_path.append(sx);  r_right_path.append(sy);
	}
	else if (t_angle > 0)
	{
		r_left_path.append(sx);  r_left_path.append(sy);
	}

	r_left_path . append(sx + e_off_x), r_left_path . append(sy + e_off_y);
	r_right_path . append(sx - e_off_x), r_right_path . append(sy - e_off_y);

	return false;
}

static inline bool round_join(buffer_t<int4> &r_left_path, buffer_t<int4> &r_right_path, MCStrokeStyle *p_stroke, int4 sx, int4 sy, angle_t &s_angle, angle_t &e_angle)
{
	uint2 width = p_stroke->width;
	bool err = false;

	real8 t_angle = s_angle.angle - e_angle.angle;
	if (t_angle < 0)
		t_angle += 2 * M_PI;


	int4 s_off_x = (int4) ((width / 2) * -s_angle.sina);
	int4 s_off_y = (int4) ((width / 2) * s_angle.cosa);

	int4 e_off_x = (int4) ((width / 2) * -e_angle.sina);
	int4 e_off_y = (int4) ((width / 2) * e_angle.cosa);

	if (r_left_path . ensure(2) || r_right_path.ensure(2))
		return true;

	r_left_path . append(sx + s_off_x), r_left_path . append(sy + s_off_y);
	r_right_path . append(sx - s_off_x), r_right_path . append(sy - s_off_y);

	if (t_angle < M_PI)
	{
		real8 t_step = 256 * 2.0 / width;
		real8 t_current = t_step ;
		while (t_current < t_angle)
		{
			int4 t_x = sx + (int4) (width / 2.0 * -sin(s_angle.angle - t_current));
			int4 t_y = sy + (int4) (width / 2.0 * cos(s_angle.angle - t_current));
			t_current += t_step;
			if (r_left_path.ensure(2))
				return true;
			r_left_path.append(t_x);  r_left_path.append(t_y);
		}
		if (r_right_path.ensure(2))
			return true;
		r_right_path.append(sx);  r_right_path.append(sy);
	}
	else if (t_angle > 0)
	{
		t_angle = 2 * M_PI - t_angle;
		real8 t_step = 256 * 2.0 / width;

		real8 t_current = t_step ;
		while (t_current < t_angle)
		{
			int4 t_x = sx + (int4) (width / 2.0 * sin(s_angle.angle + t_current));
			int4 t_y = sy + (int4) (width / 2.0 * -cos(s_angle.angle + t_current));
			t_current += t_step;
			if (r_right_path.ensure(2))
				return true;
			r_right_path.append(t_x);  r_right_path.append(t_y);
		}
		if (r_left_path.ensure(2))
			return true;
		r_left_path.append(sx);  r_left_path.append(sy);
	}

	if (r_left_path . ensure(2) || r_right_path.ensure(2))
		return true;

	r_left_path . append(sx + e_off_x), r_left_path . append(sy + e_off_y);
	r_right_path . append(sx - e_off_x), r_right_path . append(sy - e_off_y);

	return false;
}

static inline bool miter_join(buffer_t<int4> &r_left_path, buffer_t<int4> &r_right_path, MCStrokeStyle *p_stroke, int4 sx, int4 sy, angle_t &s_angle, angle_t &e_angle)
{
	uint2 width = p_stroke->width;
	bool err = false;

	real8 t_angle_limit;
	if (p_stroke->miter_limit >= 1)
		t_angle_limit = 2 * asin(1 / p_stroke->miter_limit);
	else
		t_angle_limit = 0;
	real8 t_angle = s_angle.angle - e_angle.angle;
	if (t_angle < 0)
		t_angle += 2 * M_PI;


	int4 s_off_x = (int4) ((width / 2) * -s_angle.sina);
	int4 s_off_y = (int4) ((width / 2) * s_angle.cosa);

	int4 e_off_x = (int4) ((width / 2) * -e_angle.sina);
	int4 e_off_y = (int4) ((width / 2) * e_angle.cosa);

	if (t_angle > 0 && t_angle < M_PI - t_angle_limit)
	{
		real8 t_a;
		real8 t_c = (e_angle.sina - s_angle.sina);
		real8 t_d = (s_angle.cosa - e_angle.cosa);
		if (e_angle.cosa == 0)
		{
			t_a = - t_c / s_angle.cosa;
		}
		else if (e_angle.sina == 0)
		{
			t_a = - t_d / s_angle.sina;
		}
		else
		{
			t_a = (- t_c * e_angle.sina + t_d * e_angle.cosa) / (s_angle.cosa * e_angle.sina - s_angle.sina * e_angle.cosa);
		}
		int4 t_x = (int4) (sx + width / 2.0 * (-s_angle.sina + t_a * s_angle.cosa));
		int4 t_y = (int4) (sy + width / 2.0 * (s_angle.cosa + t_a * s_angle.sina));

		if (r_left_path . ensure(2) || r_right_path.ensure(4))
			return true;

		r_left_path.append(t_x);  r_left_path.append(t_y);
		r_right_path . append(sx - s_off_x), r_right_path . append(sy - s_off_y);
		r_right_path . append(sx - e_off_x), r_right_path . append(sy - e_off_y);
	}
	else if (t_angle > M_PI && t_angle - M_PI > t_angle_limit)
	{
		real8 t_a;
		real8 t_c = (s_angle.sina - e_angle.sina);
		real8 t_d = (e_angle.cosa - s_angle.cosa);
		if (e_angle.cosa == 0)
			t_a = - t_c / s_angle.cosa;
		else if (e_angle.sina == 0)
			t_a = - t_d / s_angle.sina;
		else
			t_a = (- t_c * e_angle.sina + t_d * e_angle.cosa) / (s_angle.cosa * e_angle.sina - s_angle.sina * e_angle.cosa);
		int4 t_x = (int4) (sx + width / 2.0 * (s_angle.sina + t_a * s_angle.cosa));
		int4 t_y = (int4) (sy + width / 2.0 * (-s_angle.cosa + t_a * s_angle.sina));

		if (r_left_path . ensure(4) || r_right_path.ensure(2))
			return true;

		r_right_path.append(t_x);  r_right_path.append(t_y);
		r_left_path . append(sx + s_off_x), r_left_path . append(sy + s_off_y);
		r_left_path . append(sx + e_off_x), r_left_path . append(sy + e_off_y);
	}
	else
	{
		if (r_left_path . ensure(4) || r_right_path.ensure(4))
			return true;

		r_left_path . append(sx + s_off_x), r_left_path . append(sy + s_off_y);
		r_right_path . append(sx - s_off_x), r_right_path . append(sy - s_off_y);
		r_left_path . append(sx + e_off_x), r_left_path . append(sy + e_off_y);
		r_right_path . append(sx - e_off_x), r_right_path . append(sy - e_off_y);
	}

	return false;
}

static inline bool create_join(buffer_t<int4> &r_left_path, buffer_t<int4> &r_right_path, MCStrokeStyle *p_stroke, int4 sx, int4 sy, angle_t &s_angle, angle_t &e_angle)
{
	switch (p_stroke->join)
	{
	case JoinBevel:
		return bevel_join(r_left_path, r_right_path, p_stroke, sx, sy, s_angle, e_angle);
	case JoinRound:
		return round_join(r_left_path, r_right_path, p_stroke, sx, sy, s_angle, e_angle);
	case JoinMiter:
		return miter_join(r_left_path, r_right_path, p_stroke, sx, sy, s_angle, e_angle);
	default:
		return true;
	}
}

static inline bool dash_line(buffer_t<int4> &r_left_path, buffer_t<int4> &r_right_path, buffer_t<uint4> &r_counts, uint4 &r_last_count, MCStrokeStyle *p_stroke, int4 sx, int4 sy, int4 ex, int4 ey, uint4 &r_dash_index, int4 &r_dash_position, bool &r_dash_on)
{
	int4 t_dx = ex - sx;
	int4 t_dy = ey - sy;
	int4 t_length = (int4) sqrt((double)((double)t_dx*t_dx + (double)t_dy*t_dy));

	angle_t t_angle(t_dx, t_dy);
	angle_t t_inv_angle = t_angle;
	t_inv_angle.invert();

	uint4 t_dist = 0;

	buffer_t<int4> start_cap_cache;
	buffer_t<int4> end_cap_cache;
	start_cap_cache.initialise(256);
	end_cap_cache.initialise(256);

	while (t_length)
	{
		int4 t_min = MCU_min(t_length, r_dash_position);
		t_length -= t_min;
		r_dash_position -= t_min;
		t_dist += t_min;

		if (r_dash_position == 0)
		{
			int4 t_x = sx + (int4)(t_dist * t_angle.cosa);
			int4 t_y = sy + (int4)(t_dist * t_angle.sina);
			if (++r_dash_index == p_stroke->dash.length)
				r_dash_index = 0;
			r_dash_position = p_stroke->dash.data[r_dash_index] << 8;
			
			r_dash_on = !r_dash_on;

			if (r_dash_on)
			{
				if (start_cap_cache.size() == 0)
				{
					if (create_cap(start_cap_cache, p_stroke, 0, 0, t_inv_angle))
						return true;
				}

				if (append_path_offset(r_left_path, start_cap_cache, t_x, t_y))
					return true;
			}
			else
			{
				if (end_cap_cache.size() == 0)
				{
					if (create_cap(end_cap_cache, p_stroke, 0, 0, t_angle))
						return true;
				}

				if (append_path_offset(r_left_path, end_cap_cache, t_x, t_y))
					return true;
				if (append_path_reverse(r_left_path, r_right_path))
					return true;
				if (r_right_path.initialise(1024))
					return true;
				if (close_path(r_left_path, r_last_count))
					return true;
				if (r_counts . ensure(1))
					return true;
				r_counts . append((r_left_path . size() - r_last_count) / 2);
				r_last_count = r_left_path.size();
			}
		}
	}
	return false;
}

bool dash_stroke(uint1 const *p_commands, int4 const *p_data, MCStrokeStyle *p_stroke, polygon_t& r_cache);

bool stroke(uint1 const *p_commands, int4 const *p_data, MCStrokeStyle *p_stroke, polygon_t& r_cache)
{
	if (p_stroke->style == LineOnOffDash && p_stroke->dash.length > 0)
		return dash_stroke(p_commands, p_data, p_stroke, r_cache);
	bool err = false;
	uint2 width = p_stroke->width;

#ifdef PROFILE_AG
	static Timer s_timer("MCExtendedGraphic::path_flatten");
	s_timer . Start();
#endif

	uint32_t t_capstyle, t_start_cap, t_end_cap;
	t_capstyle = p_stroke->cap;

	t_start_cap = (t_capstyle & NoStartCap) ? CapButt : t_capstyle;
	t_end_cap = (t_capstyle & NoEndCap) ? CapButt : t_capstyle;

	buffer_t<uint4> t_counts;
	buffer_t<int4> t_vertices;
	buffer_t<int4> t_right_path;
	uint1 const *t_command;
	int4 const *t_data;

	int4 t_first_x, t_first_y;
	int4 t_last_x, t_last_y;
	angle_t t_first_angle;
	angle_t t_last_angle;
	uint4 t_last_count;

	t_command = p_commands;
	t_data = p_data;
	t_last_count = 0;

	err = t_counts . initialise(256);
	if (!err)
		err = t_vertices . initialise(1024);
	if (!err)
		err = t_right_path . initialise(1024);

	while(!err)
	{
		if ((*t_command == PATH_COMMAND_END || *t_command == PATH_COMMAND_MOVE_TO) && t_first_angle.isset)
		{
			p_stroke->cap = t_end_cap;
			err = create_cap(t_vertices, p_stroke, t_last_x, t_last_y, t_last_angle);

			uint4 t_size = t_right_path.size() / 2;
			if (!err)
				err = t_vertices.ensure(t_size * 2);
			if (!err)
			{
				int4 *t_path = t_right_path.grab();
				while (t_size)
				{
					t_size--;
					t_vertices.append(t_path[t_size * 2]);
					t_vertices.append(t_path[t_size * 2 + 1]);
				}
				free(t_path);
				t_right_path.initialise(1024);
			}
			t_first_angle.invert();
			p_stroke->cap = t_start_cap;
			if (!err)
				err = create_cap(t_vertices, p_stroke, t_first_x, t_first_y, t_first_angle);
			p_stroke->cap = t_capstyle;
			if (!err)
				err = t_vertices.ensure(2);
			if (!err)
			{
				t_vertices.append(t_vertices.borrow()[t_last_count]);
				t_vertices.append(t_vertices.borrow()[t_last_count + 1]);
			}
			if (!err)
				err = t_counts . ensure(1);
			if (!err)
				t_counts . append((t_vertices . size() - t_last_count) / 2);

		}

		if (err || *t_command == PATH_COMMAND_END)
			break;

		switch(*t_command++)
		{
			case PATH_COMMAND_MOVE_TO:
				transform(t_data[0], t_data[1], t_first_x, t_first_y);
				t_data += 2;

				t_last_x = t_first_x;
				t_last_y = t_first_y;
				t_last_count = t_vertices . size();

				t_first_angle.isset = false;
			break;

			case PATH_COMMAND_LINE_TO:
			{
				int4 t_x, t_y;
				transform(t_data[0], t_data[1], t_x, t_y);
				t_data += 2;

				if (t_last_x == t_x && t_last_y == t_y)
					continue;

				angle_t t_angle( t_x - t_last_x, t_y - t_last_y);
				if (!t_first_angle.isset)
					t_first_angle = t_angle;
				else
					err = create_join(t_vertices, t_right_path, p_stroke, t_last_x, t_last_y, t_last_angle, t_angle);

				t_last_x = t_x;
				t_last_y = t_y;
				t_last_angle = t_angle;
			}
			break;
			
			case PATH_COMMAND_QUADRATIC_TO:
			{
				int4 t_stack[70];
				uint4 t_level;
				t_stack[2 * 2 + 0] = t_last_x;
				t_stack[2 * 2 + 1] = t_last_y;
				transform(t_data[0], t_data[1], t_stack[2 * 1 + 0], t_stack[2 * 1 + 1]);
				transform(t_data[2], t_data[3], t_stack[2 * 0 + 0], t_stack[2 * 0 + 1]);
				t_data += 4;

				t_level = 1;
				while(!err && t_level > 0)
				{
					int4 sx, sy;
					int4 cx, cy;
					int4 ex, ey;

					sx = t_stack[2 * (2 * (t_level - 1) + 2) + 0];
					sy = t_stack[2 * (2 * (t_level - 1) + 2) + 1];
					cx = t_stack[2 * (2 * (t_level - 1) + 1) + 0];
					cy = t_stack[2 * (2 * (t_level - 1) + 1) + 1];
					ex = t_stack[2 * (2 * (t_level - 1) + 0) + 0];
					ey = t_stack[2 * (2 * (t_level - 1) + 0) + 1];

					if (t_level == 16 || is_flat(sx, sy, ex, ey, cx, cy))
					{
						if (ex != t_last_x || ey != t_last_y)
						{
							angle_t t_angle( ex - t_last_x, ey - t_last_y);
							if (!t_first_angle.isset)
								t_first_angle = t_angle;
							else
								err = curve_join(t_vertices, t_right_path, p_stroke, t_last_x, t_last_y, t_last_angle, t_angle);
							
							t_last_x = ex;
							t_last_y = ey;
							t_last_angle = t_angle;
						}
						t_level -= 1;
					}
					else
					{
						int4 scx, scy;
						int4 cex, cey;
						int4 scex, scey;

						scx = (sx + cx) / 2;
						scy = (sy + cy) / 2;

						cex = (cx + ex) / 2;
						cey = (cy + ey) / 2;

						scex = (scx + cex) / 2;
						scey = (scy + cey) / 2;

						t_stack[2 * (2 * (t_level - 1) + 4) + 0] = sx;
						t_stack[2 * (2 * (t_level - 1) + 4) + 1] = sy;
						t_stack[2 * (2 * (t_level - 1) + 3) + 0] = scx;
						t_stack[2 * (2 * (t_level - 1) + 3) + 1] = scy;
						t_stack[2 * (2 * (t_level - 1) + 2) + 0] = scex;
						t_stack[2 * (2 * (t_level - 1) + 2) + 1] = scey;
						t_stack[2 * (2 * (t_level - 1) + 1) + 0] = cex;
						t_stack[2 * (2 * (t_level - 1) + 1) + 1] = cey;
						t_level += 1;
					}
				}
			}
			break;

			case PATH_COMMAND_CUBIC_TO:
			{
				int4 t_stack[140];
				uint4 t_level;
				real8 t_curve_angle = t_last_angle.angle;
				t_stack[2 * 3 + 0] = t_last_x;
				t_stack[2 * 3 + 1] = t_last_y;
				transform(t_data[0], t_data[1], t_stack[2 * 2 + 0], t_stack[2 * 2 + 1]);
				transform(t_data[2], t_data[3], t_stack[2 * 1 + 0], t_stack[2 * 1 + 1]);
				transform(t_data[4], t_data[5], t_stack[2 * 0 + 0], t_stack[2 * 0 + 1]);
				t_data += 6;

				t_level = 1;
				while(!err && t_level > 0)
				{
					int4 sx, sy;
					int4 ax, ay;
					int4 bx, by;
					int4 ex, ey;

					sx = t_stack[2 * (3 * (t_level - 1) + 3) + 0];
					sy = t_stack[2 * (3 * (t_level - 1) + 3) + 1];
					ax = t_stack[2 * (3 * (t_level - 1) + 2) + 0];
					ay = t_stack[2 * (3 * (t_level - 1) + 2) + 1];
					bx = t_stack[2 * (3 * (t_level - 1) + 1) + 0];
					by = t_stack[2 * (3 * (t_level - 1) + 1) + 1];
					ex = t_stack[2 * (3 * (t_level - 1) + 0) + 0];
					ey = t_stack[2 * (3 * (t_level - 1) + 0) + 1];

					if (t_level == 16 || (is_flat(sx, sy, ex, ey, ax, ay) && is_flat(sx, sy, ex, ey, bx, by)))
					{
						if (ex != t_last_x || ey != t_last_y)
						{
							real8 t_angle = atan2((real8) (ey - t_last_y), ex - t_last_x);
							if (!t_first_angle.isset)
								t_first_angle = angle_t(t_angle);
							else
								err = curve_join(t_vertices, t_right_path, p_stroke, t_last_x, t_last_y, t_curve_angle, t_angle);

							t_last_x = ex;
							t_last_y = ey;
							t_curve_angle = t_angle;
						}
						t_level -= 1;
					}
					else
					{
						int4 sax, say;
						int4 abx, aby;
						int4 bex, bey;
						int4 sabx, saby;
						int4 abex, abey;
						int4 sabex, sabey;

						sax = (sx + ax) / 2;
						say = (sy + ay) / 2;

						abx = (ax + bx) / 2;
						aby = (ay + by) / 2;

						bex = (bx + ex) / 2;
						bey = (by + ey) / 2;

						sabx = (sax + abx) / 2;
						saby = (say + aby) / 2;

						abex = (abx + bex) / 2;
						abey = (aby + bey) / 2;

						sabex = (sabx + abex) / 2;
						sabey = (saby + abey) / 2;

						t_stack[2 * (3 * (t_level - 1) + 6) + 0] = sx;
						t_stack[2 * (3 * (t_level - 1) + 6) + 1] = sy;
						t_stack[2 * (3 * (t_level - 1) + 5) + 0] = sax;
						t_stack[2 * (3 * (t_level - 1) + 5) + 1] = say;
						t_stack[2 * (3 * (t_level - 1) + 4) + 0] = sabx;
						t_stack[2 * (3 * (t_level - 1) + 4) + 1] = saby;
						t_stack[2 * (3 * (t_level - 1) + 3) + 0] = sabex;
						t_stack[2 * (3 * (t_level - 1) + 3) + 1] = sabey;
						t_stack[2 * (3 * (t_level - 1) + 2) + 0] = abex;
						t_stack[2 * (3 * (t_level - 1) + 2) + 1] = abey;
						t_stack[2 * (3 * (t_level - 1) + 1) + 0] = bex;
						t_stack[2 * (3 * (t_level - 1) + 1) + 1] = bey;
						t_level += 1;
					}
				}
				t_last_angle.set(t_curve_angle);
			}
			break;

			case PATH_COMMAND_CLOSE:
				if (t_last_x != t_first_x || t_last_y != t_first_y)
				{
					angle_t t_angle( t_first_x - t_last_x, t_first_y - t_last_y);
					err = create_join(t_vertices, t_right_path, p_stroke, t_last_x, t_last_y, t_last_angle, t_angle);

					t_last_angle = t_angle;
				}
				else if (!t_first_angle.isset)
				{
					continue;
				}
				if (!err)
					err = create_join(t_vertices, t_right_path, p_stroke, t_first_x, t_first_y, t_last_angle, t_first_angle);
				if (!err)
					err = t_counts.ensure(2);
				if (!err)
				{
					t_counts.append((t_vertices.size() - t_last_count) / 2 + 1);
					t_counts.append(t_right_path.size() / 2 + 1);
				}
				uint4 t_size = t_right_path.size() / 2;
				if (!err)
					err = t_vertices.ensure((t_size + 2) * 2);
				if (!err)
				{
					int4 t_first_x_left = t_vertices.borrow()[t_last_count];
					int4 t_first_y_left = t_vertices.borrow()[t_last_count + 1];
					t_vertices.append(t_first_x_left);
					t_vertices.append(t_first_y_left);
					int4 *t_path = t_right_path.grab();
					int4 t_first_x_right = t_path[(t_size - 1) * 2];
					int4 t_first_y_right = t_path[(t_size - 1) * 2 + 1];
					while (t_size)
					{
						t_size--;
						t_vertices.append(t_path[t_size * 2]);
						t_vertices.append(t_path[t_size * 2 + 1]);
					}
					t_vertices.append(t_first_x_right);
					t_vertices.append(t_first_y_right);
					free(t_path);
				}
				t_last_count = t_vertices.size();
				t_first_angle.isset = false;


				
			break;
		}
	}

	if (!err)
	{
		r_cache . valency = t_counts . size();
		r_cache . counts = t_counts . grab();
		r_cache . vertices = t_vertices . grab();
	}

#ifdef PROFILE_AG
	s_timer . Stop();
#endif

	return err;
}

bool dash_stroke(uint1 const *p_commands, int4 const *p_data, MCStrokeStyle *p_stroke, polygon_t& r_cache)
{
	bool err = false;
	uint2 width = p_stroke->width;

#ifdef PROFILE_AG
	static Timer s_timer("MCExtendedGraphic::path_flatten");
	s_timer . Start();
#endif

	buffer_t<uint4> t_counts;
	buffer_t<int4> t_vertices;
	buffer_t<int4> t_right_path;
	uint1 const *t_command;
	int4 const *t_data;

	int4 t_first_x, t_first_y;
	int4 t_last_x, t_last_y;
	angle_t t_first_angle;
	angle_t t_last_angle;
	uint4 t_last_count;

	uint4 t_dash_index = -1;
	int4 t_dash_position = 0;
	bool t_dash_on = false;

	t_command = p_commands;
	t_data = p_data;
	t_last_count = 0;

	err = t_counts . initialise(256);
	if (!err)
		err = t_vertices . initialise(1024);
	if (!err)
		err = t_right_path . initialise(1024);

	while(!err)
	{
		if ((*t_command == PATH_COMMAND_END || *t_command == PATH_COMMAND_MOVE_TO) && t_dash_on && t_first_angle.isset)
		{
			err = create_cap(t_vertices, p_stroke, t_last_x, t_last_y, t_last_angle);

			if (!err)
			{
				err = append_path_reverse(t_vertices, t_right_path);
				t_right_path.initialise(1024);
			}
			if (!err)
				err = close_path(t_vertices, t_last_count);
			if (!err)
				err = t_counts . ensure(1);
			if (!err)
				t_counts . append((t_vertices . size() - t_last_count) / 2);

		}

		if (err || *t_command == PATH_COMMAND_END)
			break;

		switch(*t_command++)
		{
			case PATH_COMMAND_MOVE_TO:
				transform(t_data[0], t_data[1], t_first_x, t_first_y);
				t_data += 2;

				t_last_x = t_first_x;
				t_last_y = t_first_y;
				t_last_count = t_vertices . size();

				t_first_angle.isset = false;

				t_dash_index = -1;
				t_dash_position = 0;
				t_dash_on = false;
			break;

			case PATH_COMMAND_LINE_TO:
			{
				int4 t_x, t_y;
				transform(t_data[0], t_data[1], t_x, t_y);
				t_data += 2;

				if (t_last_x == t_x && t_last_y == t_y)
					continue;

				angle_t t_angle( t_x - t_last_x, t_y - t_last_y);
				if (!t_first_angle.isset)
					t_first_angle = t_angle;
				else
					if (t_dash_on)
						err = create_join(t_vertices, t_right_path, p_stroke, t_last_x, t_last_y, t_last_angle, t_angle);

				dash_line(t_vertices, t_right_path, t_counts, t_last_count, p_stroke, t_last_x, t_last_y, t_x, t_y, t_dash_index, t_dash_position, t_dash_on);
				t_last_x = t_x;
				t_last_y = t_y;
				t_last_angle = t_angle;
			}
			break;
			
			case PATH_COMMAND_QUADRATIC_TO:
			{
				int4 t_stack[70];
				uint4 t_level;
				t_stack[2 * 2 + 0] = t_last_x;
				t_stack[2 * 2 + 1] = t_last_y;
				transform(t_data[0], t_data[1], t_stack[2 * 1 + 0], t_stack[2 * 1 + 1]);
				transform(t_data[2], t_data[3], t_stack[2 * 0 + 0], t_stack[2 * 0 + 1]);
				t_data += 4;

				t_level = 1;
				while(!err && t_level > 0)
				{
					int4 sx, sy;
					int4 cx, cy;
					int4 ex, ey;

					sx = t_stack[2 * (2 * (t_level - 1) + 2) + 0];
					sy = t_stack[2 * (2 * (t_level - 1) + 2) + 1];
					cx = t_stack[2 * (2 * (t_level - 1) + 1) + 0];
					cy = t_stack[2 * (2 * (t_level - 1) + 1) + 1];
					ex = t_stack[2 * (2 * (t_level - 1) + 0) + 0];
					ey = t_stack[2 * (2 * (t_level - 1) + 0) + 1];

					if (t_level == 16 || is_flat(sx, sy, ex, ey, cx, cy))
					{
						if (ex != t_last_x || ey != t_last_y)
						{
							angle_t t_angle( ex - t_last_x, ey - t_last_y);
							if (!t_first_angle.isset)
								t_first_angle = t_angle;
							else
								if (t_dash_on)
									err = curve_join(t_vertices, t_right_path, p_stroke, t_last_x, t_last_y, t_last_angle, t_angle);
							
							dash_line(t_vertices, t_right_path, t_counts, t_last_count, p_stroke, t_last_x, t_last_y, ex, ey, t_dash_index, t_dash_position, t_dash_on);

							t_last_x = ex;
							t_last_y = ey;
							t_last_angle = t_angle;
						}
						t_level -= 1;
					}
					else
					{
						int4 scx, scy;
						int4 cex, cey;
						int4 scex, scey;

						scx = (sx + cx) / 2;
						scy = (sy + cy) / 2;

						cex = (cx + ex) / 2;
						cey = (cy + ey) / 2;

						scex = (scx + cex) / 2;
						scey = (scy + cey) / 2;

						t_stack[2 * (2 * (t_level - 1) + 4) + 0] = sx;
						t_stack[2 * (2 * (t_level - 1) + 4) + 1] = sy;
						t_stack[2 * (2 * (t_level - 1) + 3) + 0] = scx;
						t_stack[2 * (2 * (t_level - 1) + 3) + 1] = scy;
						t_stack[2 * (2 * (t_level - 1) + 2) + 0] = scex;
						t_stack[2 * (2 * (t_level - 1) + 2) + 1] = scey;
						t_stack[2 * (2 * (t_level - 1) + 1) + 0] = cex;
						t_stack[2 * (2 * (t_level - 1) + 1) + 1] = cey;
						t_level += 1;
					}
				}
			}
			break;

			case PATH_COMMAND_CUBIC_TO:
			{
				int4 t_stack[140];
				uint4 t_level;
				real8 t_curve_angle = t_last_angle.angle;
				t_stack[2 * 3 + 0] = t_last_x;
				t_stack[2 * 3 + 1] = t_last_y;
				transform(t_data[0], t_data[1], t_stack[2 * 2 + 0], t_stack[2 * 2 + 1]);
				transform(t_data[2], t_data[3], t_stack[2 * 1 + 0], t_stack[2 * 1 + 1]);
				transform(t_data[4], t_data[5], t_stack[2 * 0 + 0], t_stack[2 * 0 + 1]);
				t_data += 6;

				t_level = 1;
				while(!err && t_level > 0)
				{
					int4 sx, sy;
					int4 ax, ay;
					int4 bx, by;
					int4 ex, ey;

					sx = t_stack[2 * (3 * (t_level - 1) + 3) + 0];
					sy = t_stack[2 * (3 * (t_level - 1) + 3) + 1];
					ax = t_stack[2 * (3 * (t_level - 1) + 2) + 0];
					ay = t_stack[2 * (3 * (t_level - 1) + 2) + 1];
					bx = t_stack[2 * (3 * (t_level - 1) + 1) + 0];
					by = t_stack[2 * (3 * (t_level - 1) + 1) + 1];
					ex = t_stack[2 * (3 * (t_level - 1) + 0) + 0];
					ey = t_stack[2 * (3 * (t_level - 1) + 0) + 1];

					if (t_level == 16 || (is_flat(sx, sy, ex, ey, ax, ay) && is_flat(sx, sy, ex, ey, bx, by)))
					{
						if (ex != t_last_x || ey != t_last_y)
						{
							real8 t_angle = atan2((real8) (ey - t_last_y), ex - t_last_x);
							if (!t_first_angle.isset)
								t_first_angle = angle_t(t_angle);
							else
								if (t_dash_on)
									err = curve_join(t_vertices, t_right_path, p_stroke, t_last_x, t_last_y, t_curve_angle, t_angle);

							dash_line(t_vertices, t_right_path, t_counts, t_last_count, p_stroke, t_last_x, t_last_y, ex, ey, t_dash_index, t_dash_position, t_dash_on);
							t_last_x = ex;
							t_last_y = ey;
							t_curve_angle = t_angle;
						}
						t_level -= 1;
					}
					else
					{
						int4 sax, say;
						int4 abx, aby;
						int4 bex, bey;
						int4 sabx, saby;
						int4 abex, abey;
						int4 sabex, sabey;

						sax = (sx + ax) / 2;
						say = (sy + ay) / 2;

						abx = (ax + bx) / 2;
						aby = (ay + by) / 2;

						bex = (bx + ex) / 2;
						bey = (by + ey) / 2;

						sabx = (sax + abx) / 2;
						saby = (say + aby) / 2;

						abex = (abx + bex) / 2;
						abey = (aby + bey) / 2;

						sabex = (sabx + abex) / 2;
						sabey = (saby + abey) / 2;

						t_stack[2 * (3 * (t_level - 1) + 6) + 0] = sx;
						t_stack[2 * (3 * (t_level - 1) + 6) + 1] = sy;
						t_stack[2 * (3 * (t_level - 1) + 5) + 0] = sax;
						t_stack[2 * (3 * (t_level - 1) + 5) + 1] = say;
						t_stack[2 * (3 * (t_level - 1) + 4) + 0] = sabx;
						t_stack[2 * (3 * (t_level - 1) + 4) + 1] = saby;
						t_stack[2 * (3 * (t_level - 1) + 3) + 0] = sabex;
						t_stack[2 * (3 * (t_level - 1) + 3) + 1] = sabey;
						t_stack[2 * (3 * (t_level - 1) + 2) + 0] = abex;
						t_stack[2 * (3 * (t_level - 1) + 2) + 1] = abey;
						t_stack[2 * (3 * (t_level - 1) + 1) + 0] = bex;
						t_stack[2 * (3 * (t_level - 1) + 1) + 1] = bey;
						t_level += 1;
					}
				}
				t_last_angle = angle_t(t_curve_angle);
			}
			break;

			case PATH_COMMAND_CLOSE:
				if (t_last_x != t_first_x || t_last_y != t_first_y)
				{
					angle_t t_angle( t_first_x - t_last_x, t_first_y - t_last_y);
					if (t_dash_on)
					{
						err = create_join(t_vertices, t_right_path, p_stroke, t_last_x, t_last_y, t_last_angle, t_angle);
					}
					if (!err)
						err = dash_line(t_vertices, t_right_path, t_counts, t_last_count, p_stroke, t_last_x, t_last_y, t_first_x, t_first_y, t_dash_index, t_dash_position, t_dash_on);
					t_last_angle = t_angle;
				}

				if (t_dash_on)
				{
					if (!err)
						err = create_join(t_vertices, t_right_path, p_stroke, t_first_x, t_first_y, t_last_angle, t_first_angle);

					if (!err)
						err = create_cap(t_vertices, p_stroke, t_first_x, t_first_y, t_first_angle);

					if (!err)
					{
						err = append_path_reverse(t_vertices, t_right_path);
						t_right_path.initialise(1024);
					}
					if (!err)
						err = close_path(t_vertices, t_last_count);
					if (!err)
						err = t_counts . ensure(1);
					if (!err)
						t_counts . append((t_vertices . size() - t_last_count) / 2);
					t_last_count = t_vertices.size();
				}
			break;
		}
	}

	if (!err)
	{
		r_cache . valency = t_counts . size();
		r_cache . counts = t_counts . grab();
		r_cache . vertices = t_vertices . grab();
	}

#ifdef PROFILE_AG
	s_timer . Stop();
#endif

	return err;
}

bool fill(uint1 const *p_commands, int4 const *p_data, polygon_t& r_cache)
{
#ifdef PROFILE_AG
	static Timer s_timer("MCExtendedGraphic::path_fill");
	s_timer . Start();
#endif

	bool err = false;

	buffer_t<uint4> t_counts;
	buffer_t<int4> t_vertices;

	uint1 const *t_command;
	int4 const *t_data;

	int4 t_first_x, t_first_y;
	int4 t_last_x, t_last_y;
	uint4 t_count;

	t_command = p_commands;
	t_data = p_data;

	t_count = 0;

	err = t_counts . initialise(256);
	if (!err)
		err = t_vertices . initialise(1024);

	while(!err)
	{
		if (!err)
			err = t_vertices . ensure(2);

		if (!err && (*t_command == PATH_COMMAND_END || *t_command == PATH_COMMAND_MOVE_TO) && t_count > 2)
		{
			if (t_last_x != t_first_x || t_last_y != t_first_y)
			{
				t_count += 1;
				t_vertices . append(t_first_x);
				t_vertices . append(t_first_y);
			}

			err = t_counts . ensure(1);
			if (!err)
				t_counts . append(t_count);
		}

		if (err || *t_command == PATH_COMMAND_END)
			break;


		switch(*t_command++)
		{
		case PATH_COMMAND_MOVE_TO:
			t_count = 1;
			transform(t_data[0], t_data[1], t_first_x, t_first_y);
			t_vertices . append(t_first_x);
			t_vertices . append(t_first_y);
			t_last_x = t_first_x;
			t_last_y = t_first_y;
			t_data += 2;
		break;

		case PATH_COMMAND_LINE_TO:
		{
			int4 t_x, t_y;
			transform(t_data[0], t_data[1], t_x, t_y);
			if (t_x != t_last_x || t_y != t_last_y)
			{
				t_count += 1;
				t_vertices . append(t_x);
				t_vertices . append(t_y);
				t_last_x = t_x;
				t_last_y = t_y;
			}
			t_data += 2;
		}
		break;

		case PATH_COMMAND_QUADRATIC_TO:
		{
			int4 t_stack[70];
			uint4 t_level;
			t_stack[2 * 2 + 0] = t_last_x;
			t_stack[2 * 2 + 1] = t_last_y;
			transform(t_data[0], t_data[1], t_stack[2 * 1 + 0], t_stack[2 * 1 + 1]);
			transform(t_data[2], t_data[3], t_stack[2 * 0 + 0], t_stack[2 * 0 + 1]);
			t_data += 4;

			t_level = 1;
			while(!err && t_level > 0)
			{
				int4 sx, sy;
				int4 cx, cy;
				int4 ex, ey;

				sx = t_stack[2 * (2 * (t_level - 1) + 2) + 0];
				sy = t_stack[2 * (2 * (t_level - 1) + 2) + 1];
				cx = t_stack[2 * (2 * (t_level - 1) + 1) + 0];
				cy = t_stack[2 * (2 * (t_level - 1) + 1) + 1];
				ex = t_stack[2 * (2 * (t_level - 1) + 0) + 0];
				ey = t_stack[2 * (2 * (t_level - 1) + 0) + 1];

				if (t_level == 16 || is_flat(sx, sy, ex, ey, cx, cy))
				{
					if (ex != t_last_x || ey != t_last_y)
					{
						err = t_vertices . ensure(2);
						if (!err)
						{
							t_vertices . append(ex);
							t_vertices . append(ey);
							t_count += 1;
							t_last_x = ex;
							t_last_y = ey;
						}
					}
					t_level -= 1;
				}
				else
				{
					int4 scx, scy;
					int4 cex, cey;
					int4 scex, scey;

					scx = (sx + cx) / 2;
					scy = (sy + cy) / 2;

					cex = (cx + ex) / 2;
					cey = (cy + ey) / 2;

					scex = (scx + cex) / 2;
					scey = (scy + cey) / 2;

					t_stack[2 * (2 * (t_level - 1) + 4) + 0] = sx;
					t_stack[2 * (2 * (t_level - 1) + 4) + 1] = sy;
					t_stack[2 * (2 * (t_level - 1) + 3) + 0] = scx;
					t_stack[2 * (2 * (t_level - 1) + 3) + 1] = scy;
					t_stack[2 * (2 * (t_level - 1) + 2) + 0] = scex;
					t_stack[2 * (2 * (t_level - 1) + 2) + 1] = scey;
					t_stack[2 * (2 * (t_level - 1) + 1) + 0] = cex;
					t_stack[2 * (2 * (t_level - 1) + 1) + 1] = cey;
					t_level += 1;
				}
			}
		}
		break;

		case PATH_COMMAND_CUBIC_TO:
		{
			int4 t_stack[140];
			uint4 t_level;
			t_stack[2 * 3 + 0] = t_last_x;
			t_stack[2 * 3 + 1] = t_last_y;
			transform(t_data[0], t_data[1], t_stack[2 * 2 + 0], t_stack[2 * 2 + 1]);
			transform(t_data[2], t_data[3], t_stack[2 * 1 + 0], t_stack[2 * 1 + 1]);
			transform(t_data[4], t_data[5], t_stack[2 * 0 + 0], t_stack[2 * 0 + 1]);
			t_data += 6;

			t_level = 1;
			while(!err && t_level > 0)
			{
				int4 sx, sy;
				int4 ax, ay;
				int4 bx, by;
				int4 ex, ey;

				sx = t_stack[2 * (3 * (t_level - 1) + 3) + 0];
				sy = t_stack[2 * (3 * (t_level - 1) + 3) + 1];
				ax = t_stack[2 * (3 * (t_level - 1) + 2) + 0];
				ay = t_stack[2 * (3 * (t_level - 1) + 2) + 1];
				bx = t_stack[2 * (3 * (t_level - 1) + 1) + 0];
				by = t_stack[2 * (3 * (t_level - 1) + 1) + 1];
				ex = t_stack[2 * (3 * (t_level - 1) + 0) + 0];
				ey = t_stack[2 * (3 * (t_level - 1) + 0) + 1];

				if (t_level == 16 || (is_flat(sx, sy, ex, ey, ax, ay) && is_flat(sx, sy, ex, ey, bx, by)))
				{
					if (ex != t_last_x || ey != t_last_y)
					{
						err = t_vertices . ensure(2);
						if (!err)
						{
							t_vertices . append(ex);
							t_vertices . append(ey);
							t_count += 1;
							t_last_x = ex;
							t_last_y = ey;
						}
					}
					t_level -= 1;
				}
				else
				{
					int4 sax, say;
					int4 abx, aby;
					int4 bex, bey;
					int4 sabx, saby;
					int4 abex, abey;
					int4 sabex, sabey;

					sax = (sx + ax) / 2;
					say = (sy + ay) / 2;

					abx = (ax + bx) / 2;
					aby = (ay + by) / 2;

					bex = (bx + ex) / 2;
					bey = (by + ey) / 2;

					sabx = (sax + abx) / 2;
					saby = (say + aby) / 2;

					abex = (abx + bex) / 2;
					abey = (aby + bey) / 2;

					sabex = (sabx + abex) / 2;
					sabey = (saby + abey) / 2;

					t_stack[2 * (3 * (t_level - 1) + 6) + 0] = sx;
					t_stack[2 * (3 * (t_level - 1) + 6) + 1] = sy;
					t_stack[2 * (3 * (t_level - 1) + 5) + 0] = sax;
					t_stack[2 * (3 * (t_level - 1) + 5) + 1] = say;
					t_stack[2 * (3 * (t_level - 1) + 4) + 0] = sabx;
					t_stack[2 * (3 * (t_level - 1) + 4) + 1] = saby;
					t_stack[2 * (3 * (t_level - 1) + 3) + 0] = sabex;
					t_stack[2 * (3 * (t_level - 1) + 3) + 1] = sabey;
					t_stack[2 * (3 * (t_level - 1) + 2) + 0] = abex;
					t_stack[2 * (3 * (t_level - 1) + 2) + 1] = abey;
					t_stack[2 * (3 * (t_level - 1) + 1) + 0] = bex;
					t_stack[2 * (3 * (t_level - 1) + 1) + 1] = bey;
					t_level += 1;
				}
			}
		}
		break;
		}
	}

	if (!err)
	{
		r_cache . valency = t_counts . size();
		r_cache . counts = t_counts . grab();
		r_cache . vertices = t_vertices . grab();
	}

#ifdef PROFILE_AG
	s_timer . Stop();
#endif

	return err;
}

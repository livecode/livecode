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

#ifndef __PATH_PRIVATE_H
#define __PATH_PRIVATE_H

enum
{
	PATH_COMMAND_END,
	PATH_COMMAND_MOVE_TO,
	PATH_COMMAND_LINE_TO,
	PATH_COMMAND_CUBIC_TO,
	PATH_COMMAND_QUADRATIC_TO,
	PATH_COMMAND_CLOSE
};

template<typename value_t> class buffer_t
{
public:
	buffer_t(unsigned int p_maximum = 1U << 31)
		: f_data(NULL), f_index(0), f_limit(0), f_maximum(p_maximum)
	{
	}

	~buffer_t(void)
	{
		free(f_data);
	}

	unsigned int size(void) const
	{
		return f_index;
	}

	bool initialise(unsigned int p_initial)
	{
		if (f_data != NULL)
		{
			f_index = 0;
			return false;
		}
			
		f_data = (value_t *)malloc(p_initial * sizeof(value_t));
		if (f_data != NULL)
			f_index = 0, f_limit = p_initial;
		return f_data == NULL;
	}

	bool resize(unsigned int p_size)
	{
		if (p_size < f_limit)
		{
			f_data = (value_t *)realloc(f_data, p_size * sizeof(value_t));
			f_limit = p_size;
			f_index = 0;
		}
		else
			return extend(p_size - f_limit);

		return false;
	}

	bool ensure(unsigned int p_count)
	{
		assert( f_data != NULL);

		if (f_limit - f_index < p_count)
			return extend(p_count);

		return false;
	}

	bool extend(unsigned int p_by)
	{
		value_t *t_data;
		unsigned int t_new_limit;
		t_new_limit = f_limit;
		while(t_new_limit <= f_limit + p_by)
			t_new_limit *= 2;
		if (t_new_limit > f_maximum)
		{
			if (f_maximum - f_limit < p_by)
				return true;
			t_new_limit = f_maximum;
		}
		t_data = (value_t *)realloc(f_data, t_new_limit * sizeof(value_t));
		if (t_data == NULL)
			return true;
		f_limit = t_new_limit;
		f_data = t_data;

		return false;
	}

	void append(value_t p_value)
	{
		assert( f_index < f_limit );

		f_data[f_index++] = p_value;
	}

	value_t *grab(void)
	{
		value_t *t_data;

		if (f_index == 0)
			return NULL;

		t_data = (value_t *)realloc(f_data, f_index * sizeof(value_t));
		f_data = NULL;
		f_index = 0;
		f_limit = 0;
		return t_data;
	}
	
	value_t *borrow(void)
	{
		return f_data;
	}

private:
	value_t *f_data;
	unsigned int f_index;
	unsigned int f_limit;
	unsigned int f_maximum;
};

struct polygon_t
{
	uint4 valency;
	uint4 *counts;
	int4 *vertices;

	polygon_t(void)
	{
		valency = 0;
		counts = NULL;
		vertices = NULL;
	}

	~polygon_t(void)
	{
		if (counts != NULL)
			free(counts);
		if (vertices != NULL)
			free(vertices);
	}
};

struct angle_t
{
	real8 angle;
	real8 cosa;
	real8 sina;
	bool isset;
	
	angle_t(void)
	{
		isset = false;
	}

	angle_t(int4 dx, int4 dy)
	{
		set(dx, dy);
	}

	angle_t(real8 p_angle)
	{
		set(p_angle);
	}

	void set(int4 dx, int4 dy)
	{
		real8 t_angle = atan2((double)dy, dx);
		set(t_angle);
	}

	void set(real8 p_angle)
	{
		angle = p_angle;
		cosa = cos(angle);
		sina = sin(angle);
		isset = true;
	}

	void invert(void)
	{
		if (angle > 0)
			angle -= M_PI;
		else
			angle += M_PI;
		cosa = -cosa;
		sina = -sina;
	}
};

bool stroke(uint1 const *p_commands, int4 const *p_data, MCStrokeStyle *p_stroke, polygon_t& r_cache);
bool fill(uint1 const *p_commands, int4 const *p_data, polygon_t& r_cache);
bool antialias(MCCombiner *p_combiner, const MCRectangle& p_clip, const polygon_t& p_polygon, bool p_even_odd);
bool rasterize(MCCombiner *p_combiner, const MCRectangle& p_clip, const polygon_t& p_polygon, bool p_even_odd);

#endif

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

#if defined(__LITTLE_ENDIAN__)
#ifdef __GCC
	struct cell_t
	{
		union
		{
			struct
			{
				uint4 position;
				uint4 coverage;
			};
			struct
			{
				uint2 x, y;
				int2 area, cover;
			};
		};
		
		void set_x(int4 p_x)
		{
			x = uint2(p_x + 32768);
		}
		
		void set_y(int4 p_y)
		{
			y = uint2(p_y + 32768);
		}
		
		void set_position(int4 p_x, int4 p_y)
		{
			position = uint2(p_x + 32768) | (uint2(p_y + 32768) << 16);
		}
		
		void set_coverage(int4 p_area, int4 p_cover)
		{
			area = p_area;
			cover = p_cover;
		}
	};
#else
	struct cell_t
	{
                uint2 x, y;
                int2 area, cover;
		
		void set_x(int4 p_x)
		{
			x = uint2(p_x + 32768);
		}
		
		void set_y(int4 p_y)
		{
			y = uint2(p_y + 32768);
		}
		
		void set_position(int4 p_x, int4 p_y)
		{
			x = uint2(p_x + 32768);
			y = uint2(p_y + 32768);
		}
		
		void set_coverage(int4 p_area, int4 p_cover)
		{
			area = p_area;
			cover = p_cover;
		}
	};
#endif
#elif defined(__BIG_ENDIAN__)
	struct cell_t
	{
		union
		{
			struct
			{
				uint4 position;
				uint4 coverage;
			};
			struct
			{
				uint2 y, x;
				int2 area, cover;
			};
		};
		
		void set_x(int4 p_x)
		{
			x = uint2(p_x + 32768);
		}
		
		void set_y(int4 p_y)
		{
			y = uint2(p_y + 32768);
		}
		
		void set_position(int4 p_x, int4 p_y)
		{
			position = uint2(p_x + 32768) | (uint2(p_y + 32768) << 16);
		}
		
		void set_coverage(int4 p_area, int4 p_cover)
		{
			area = p_area;
			cover = p_cover;
		}
	};
#endif

#if !defined(__VISUALC__)
#define __forceinline inline
#endif

static __forceinline bool antialias_cell_less_than(const cell_t& left, const cell_t& right)
{
	return left . y < right . y || (left . y == right . y && left . x < right . x);
}

static __forceinline void antialias_cell_swap(cell_t& left, cell_t& right)
{
	cell_t mid;
	mid = left;
	left = right;
	right = mid;
}

static inline void antialias_cell_insertion_sort(cell_t *p_base, cell_t *p_limit)
{
	cell_t *i, *j;

	j = p_base;
	i = j + 1;

	for(; i < p_limit; j = i, i += 1)
		for(; antialias_cell_less_than(*(j + 1), *j); j -= 1)
		{
			antialias_cell_swap(*(j + 1), *j);
			if (j == p_base)
				break;
		}
}

static inline void antialias_cell_quick_sort(cell_t *p_cells, cell_t *p_cells_limit)
{
	cell_t *t_stack[40];
	cell_t **t_top;
	cell_t *t_base;
	cell_t *t_limit;

	t_top = t_stack;
	t_base = p_cells;
	t_limit = p_cells_limit;

	for(;;)
	{
		uint4 t_length;
		t_length = t_limit - t_base;

		if (t_length > 9)
		{
			cell_t *i, *j, *t_pivot;

			t_pivot = t_base + t_length / 2;
			antialias_cell_swap(*t_base, *t_pivot);

			i = t_base + 1;
			j = t_limit - 1;

			if (antialias_cell_less_than(*j, *i))
				antialias_cell_swap(*i, *j);

			if (antialias_cell_less_than(*t_base, *i))
				antialias_cell_swap(*t_base, *i);

			if (antialias_cell_less_than(*j, *t_base))
				antialias_cell_swap(*t_base, *j);

			for(;;)
			{
				do
					i++;
				while(antialias_cell_less_than(*i, *t_base));

				do
				  j--;
				while(antialias_cell_less_than(*t_base, *j));

				if (i > j)
					break;

				antialias_cell_swap(*i, *j);
			}

			antialias_cell_swap(*t_base, *j);

			if (j - t_base > t_limit - i)
			{
				t_top[0] = t_base;
				t_top[1] = j;
				t_base = i;
			}
			else
			{
				t_top[0] = i;
				t_top[1] = t_limit;
				t_limit = j;
			}

			t_top += 2;
		}
		else
		{
			antialias_cell_insertion_sort(t_base, t_limit);

			if (t_top > t_stack)
			{
				t_top -= 2;
				t_base = t_top[0];
				t_limit = t_top[1];
			}
			else
				break;
		}
	}
}

static inline uint1 antialias_compute_alpha_non_zero(int4 p_cover)
{
	p_cover >>= 7;
	if (p_cover < 0)
		p_cover = -p_cover;
	if (p_cover > 255)
		return 255;
	return p_cover;
}

static inline uint1 antialias_compute_alpha_even_odd(int4 p_cover)
{
	p_cover >>= 7;
	if (p_cover < 0)
		p_cover = -p_cover;
	p_cover &= 511;
	if (p_cover > 256)
		p_cover = 512 - p_cover;
	else if (p_cover == 256)
		p_cover = 255;
	return p_cover;
}

static bool antialias_cell_sorted(cell_t *p_base, cell_t *p_limit)
{
	for(; p_base < p_limit - 1; ++p_base)
		if (p_base[0] . y > p_base[1] . y || (p_base[0] . y == p_base[1] . y && p_base[0] . x > p_base[1] . x))
		{
			assert(false);
			return false;
		}

	return true;
}

inline static int4 fixed_integer_7(int4 p_value)
{
	// MW-2009-06-09: [[ Bug ]] Previously this was computing the integer-part incorrectly
	//   resulting in unpleasantness when co-ordinates were negative.
	return (int4)(p_value & 0xffffff80) >> 7;
}

inline static int4 fixed_fraction_7(int4 p_value)
{
	// MW-2009-06-09: [[ Bug ]] Previously this was computing the integer-part incorrectly
	//   resulting in unpleasantness when co-ordinates were negative.
	return p_value - (fixed_integer_7(p_value) << 7);
}

static void antialias_scanline(buffer_t<cell_t>& p_cells, cell_t& p_cell, int4 p_syi, int4 p_sx, int4 p_syf, int4 p_ex, int4 p_eyf)
{
	int4 t_sxi, t_exi;
	int4 t_sxf, t_exf;
	int4 t_dx;

	t_dx = p_ex - p_sx;

	t_sxi = fixed_integer_7(p_sx);
	t_sxf = fixed_fraction_7(p_sx);
	t_exi = fixed_integer_7(p_ex);
	t_exf = fixed_fraction_7(p_ex);

	if (p_syf == p_eyf)
	{
		if (t_exi != t_sxi)
		{
			p_cells . append(p_cell);
			p_cell . set_x(t_exi);
			p_cell . set_coverage(0, 0);
		}
		return;
	}

	if (t_sxi == t_exi)
	{
		int4 t_delta;
		t_delta = p_eyf - p_syf;
		p_cell . set_position(t_sxi, p_syi);
		p_cell . area += (t_sxf + t_exf) * t_delta;
		p_cell . cover += t_delta;
		return;
	}

	int4 t_p, t_first, t_incr;

	if (t_dx < 0)
	{
		t_p = t_sxf * (p_eyf - p_syf);
		t_first = 0;
		t_incr = -1;
		t_dx = -t_dx;
	}
	else
	{
		t_p = (128 - t_sxf) * (p_eyf - p_syf);
		t_first = 128;
		t_incr = 1;
	}

	int4 t_delta, t_mod;
	cell_t t_cell;

	t_delta = t_p / t_dx;
	t_mod = t_p % t_dx;
	if (t_mod < 0)
		t_delta -= 1, t_mod += t_dx;

	t_cell . set_position(t_sxi, p_syi);
	t_cell . set_coverage(p_cell . area + (t_sxf + t_first) * t_delta, p_cell . cover + t_delta);

	t_sxi += t_incr;
	p_syf += t_delta;

	if (t_sxi != t_exi)
	{
		int4 t_lift, t_rem;

		t_p = 128 * (p_eyf - p_syf + t_delta);
		t_lift = t_p / t_dx;
		t_rem = t_p % t_dx;
		if (t_rem < 0)
		{
			t_lift -= 1;
			t_rem += t_dx;
		}

		t_mod -= t_dx;

		while(t_sxi != t_exi)
		{
			t_delta = t_lift;
			t_mod += t_rem;
			if (t_mod >= 0)
				t_mod -= t_dx, t_delta++;

			p_cells . append(t_cell);
			t_cell . set_x(t_sxi);
			t_cell . set_coverage(128 * t_delta, t_delta);
			
			p_syf += t_delta;
			t_sxi += t_incr;
		}
	}

	t_delta = p_eyf - p_syf;
	p_cells . append(t_cell);
	p_cell . set_position(t_sxi, p_syi);
	p_cell . set_coverage((t_exf + 128 - t_first) * t_delta, t_delta);
}

bool antialias(MCCombiner *p_combiner, const MCRectangle& p_clip, const polygon_t& p_polygon, bool p_even_odd)
{
	bool err = false;

	static buffer_t<cell_t> t_cells(8 * 1024 * 1024);
	int4 *t_vertices;

	err = t_cells . initialise(8192);

	t_vertices = p_polygon . vertices;

	for(uint4 t_polygon = 0; !err && t_polygon < p_polygon . valency; ++t_polygon)
	{
		cell_t t_cell;
		int4 t_sx, t_sy;

		t_cell . set_position(32767, 32767);
		t_cell . set_coverage(0, 0);
		
		t_sx = (*t_vertices++ + 1) >> 1;
		t_sy = (*t_vertices++ + 1) >> 1;

		for(uint4 t_count = p_polygon . counts[t_polygon] - 1; t_count > 0; --t_count)
		{
			int4 t_ex, t_ey;

			t_ex = (*t_vertices++ + 1) >> 1;
			t_ey = (*t_vertices++ + 1) >> 1;

			int4 t_syi, t_eyi;
			int4 t_sxi, t_exi;

			t_syi = fixed_integer_7(t_sy);
			t_eyi = fixed_integer_7(t_ey);
			t_sxi = fixed_integer_7(t_sx);
			t_exi = fixed_integer_7(t_ex);

			int4 t_dx, t_dy;

			t_dx = t_ex - t_sx;
			t_dy = t_ey - t_sy;

			err = t_cells . ensure(MCU_abs(t_exi - t_sxi) + MCU_abs(t_eyi - t_syi) + 1);
			if (err)
				break;

			int4 t_syf, t_eyf;

			t_syf = fixed_fraction_7(t_sy);
			t_eyf = fixed_fraction_7(t_ey);

			if (t_syi == t_eyi)
				antialias_scanline(t_cells, t_cell, t_syi, t_sx, t_syf, t_ex, t_eyf);
			else if (t_dx == 0)
			{
				int4 t_two_fx, t_area, t_first, t_incr, t_delta;
				
				t_two_fx = fixed_fraction_7(t_ex) << 1;
				
				if (t_dy < 0)
					t_first = 0, t_incr = -1;
				else
					t_first = 128, t_incr = 1;

				t_delta = t_first - t_syf;

				t_cell . set_position(t_sxi, t_syi);
				t_cell . area += t_two_fx * t_delta ;
				t_cell . cover += t_delta;

				t_syi += t_incr;

				t_delta = t_first + t_first - 128;
				t_area = t_two_fx * t_delta;
				while(t_syi != t_eyi)
				{
					t_cells . append(t_cell);
					t_cell . set_position(t_sxi, t_syi);
					t_cell . set_coverage(t_area, t_delta);
					t_syi += t_incr;
				}

				t_cells . append(t_cell);
				t_delta = t_eyf - 128 + t_first;
				t_cell . set_position(t_sxi, t_syi);
				t_cell . set_coverage(t_two_fx * t_delta, t_delta);
			}
			else
			{
				int4 t_p, t_delta, t_mod, t_rem, t_lift, t_incr, t_first;

				if (t_dy < 0)
				{
					t_p = t_syf * t_dx;
					t_first = 0;
					t_incr = -1;
					t_dy = -t_dy;
				}
				else
				{
					t_p = (128 - t_syf) * t_dx;
					t_first = 128;
					t_incr = 1;
				}

				t_delta = t_p / t_dy;
				t_mod = t_p % t_dy;
				if (t_mod < 0)
					t_delta -= 1, t_mod += t_dy;

				antialias_scanline(t_cells, t_cell, t_syi, t_sx, t_syf, t_sx + t_delta, t_first);
				t_sx += t_delta;

				t_syi += t_incr;

				if (t_syi != t_eyi)
				{
					t_p = 128 * t_dx;
					t_lift = t_p / t_dy;
					t_rem = t_p % t_dy;
					if (t_rem < 0)
					{
						t_lift -= 1;
						t_rem += t_dy;
					}
					t_mod -= t_dy;

					while(t_syi != t_eyi)
					{
						t_cells . append(t_cell);
						t_cell . set_coverage(0, 0);

						t_delta = t_lift;
						t_mod += t_rem;
						if (t_mod >= 0)
							t_mod -= t_dy, t_delta++;

						antialias_scanline(t_cells, t_cell, t_syi, t_sx, 128 - t_first, t_sx + t_delta, t_first);
						t_sx += t_delta;
						
						t_syi += t_incr;
					}
				}

				t_cells . append(t_cell);
				t_cell . set_position(fixed_integer_7(t_sx), t_syi);
				t_cell . set_coverage(0, 0);
				antialias_scanline(t_cells, t_cell, t_syi, t_sx, 128 - t_first, t_ex, t_eyf);
			}
			
			t_sx = t_ex;
			t_sy = t_ey;
		}

		t_cells . append(t_cell);
	}

	if (!err)
	{
		int4 t_cell_count;
		cell_t *t_cell_ptr;
		cell_t *t_cell_base;
		cell_t *t_cell_limit;
		cell_t t_sentinal;

		t_sentinal . set_position(32767, 32767);
		t_sentinal . set_coverage(0, 0);
		t_cells . append(t_sentinal);

		int4 t_top, t_left, t_right, t_bottom;

		t_cell_count = t_cells . size();
		t_cell_ptr = t_cells . borrow();
		t_cell_base = t_cell_ptr;
		t_cell_limit = t_cell_base + t_cell_count;

		antialias_cell_quick_sort(t_cell_base, t_cell_limit);

		assert( antialias_cell_sorted(t_cell_base, t_cell_limit) );

		t_left = p_clip . x + 32768;
		t_top = p_clip . y + 32768;
		t_right = p_clip . x + p_clip . width + 32768;
		t_bottom = p_clip . y + p_clip . height + 32768;

		for(; t_cell_base -> y < t_top; ++t_cell_base)
			;

		p_combiner -> begin(p_combiner, t_top - 32768);

		if (p_combiner -> combine == NULL)
		{
			for(int4 t_last_y = t_top; t_cell_base -> y < t_bottom;)
			{
				int4 t_cover;
				int4 t_y;

				t_cover = 0;
				t_y = t_cell_base -> y;
				
				p_combiner -> advance(p_combiner, t_y - t_last_y);
				t_last_y = t_y;

				while(t_cell_base -> y == t_y)
				{
					if (t_cell_base -> x >= t_right)
						break;

					int4 t_span_left;
					
					if (t_cell_base -> x < t_left)
					{
						for(; t_cell_base -> y == t_y && t_cell_base -> x < t_left; ++t_cell_base)
							t_cover += t_cell_base -> cover;
						t_span_left = t_left;
					}
					else
						t_span_left = t_cell_base -> x;

					int4 t_area;
					t_area = 0;

					for(; t_cell_base -> y == t_y && t_cell_base -> x == t_span_left; ++t_cell_base)
						t_area += t_cell_base -> area, t_cover += t_cell_base -> cover;

					assert(t_y >= t_top);
					assert(t_y < t_bottom);

					assert(t_span_left >= t_left);
					assert(t_span_left < t_right);

					if (t_area)
					{
						// Add singleton cell
						uint1 t_alpha;
						if (p_even_odd)
							t_alpha = antialias_compute_alpha_even_odd((t_cover << 8) - t_area);
						else
							t_alpha = antialias_compute_alpha_non_zero((t_cover << 8) - t_area);

						if (t_alpha > 0)
							p_combiner -> blend(p_combiner, t_span_left - 32768, t_span_left - 32767, t_alpha);

						t_span_left += 1;
					}

					int4 t_span_right;

					if (t_cell_base -> y != t_y)
						t_span_right = t_right;
					else if (t_cell_base -> x > t_span_left)
						t_span_right = MCU_min(t_right, t_cell_base -> x);
					else
						continue;

					assert(t_span_right <= t_right);

					// Add Span
					uint1 t_alpha;
					if (p_even_odd)
						t_alpha = antialias_compute_alpha_even_odd(t_cover << 8);
					else
						t_alpha = antialias_compute_alpha_non_zero(t_cover << 8);
					
					p_combiner -> blend(p_combiner, t_span_left - 32768, t_span_right - 32768, t_alpha);

					if (t_span_right == t_right)
						break;
				}

				for(; t_cell_base -> y == t_y; ++t_cell_base)
					;
			}
		}
		else
		{
			// Allocate a mask buffer for the width of the clip
			uint1 *t_mask;
			t_mask = new uint1[p_clip . width];
			
			for(int4 t_last_y = t_top; t_cell_base -> y < t_bottom;)
			{
				int4 t_cover;
				int4 t_y;

				t_cover = 0;
				t_y = t_cell_base -> y;
				
				p_combiner -> advance(p_combiner, t_y - t_last_y);
				t_last_y = t_y;

				uint1 *t_mask_ptr;
				t_mask_ptr = t_mask;

				int4 t_first_x;
				t_first_x = MCU_max(t_left, t_cell_base -> x);

				int4 t_span_left;
				t_span_left = t_first_x;

				while(t_cell_base -> y == t_y)
				{
					if (t_cell_base -> x >= t_right)
						break;
					
					if (t_cell_base -> x < t_left)
					{
						for(; t_cell_base -> y == t_y && t_cell_base -> x < t_left; ++t_cell_base)
							t_cover += t_cell_base -> cover;
						t_span_left = t_left;
					}
					else
						t_span_left = t_cell_base -> x;

					int4 t_area;
					t_area = 0;

					for(; t_cell_base -> y == t_y && t_cell_base -> x == t_span_left; ++t_cell_base)
						t_area += t_cell_base -> area, t_cover += t_cell_base -> cover;

					if (t_area)
					{
						// Add singleton cell
						uint1 t_alpha;
						if (p_even_odd)
							t_alpha = antialias_compute_alpha_even_odd((t_cover << 8) - t_area);
						else
							t_alpha = antialias_compute_alpha_non_zero((t_cover << 8) - t_area);
							
						*t_mask_ptr++ = t_alpha;
						
						t_span_left += 1;
					}

					int4 t_span_right;

					if (t_cell_base -> y != t_y)
						t_span_right = t_right;
					else if (t_cell_base -> x > t_span_left)
						t_span_right = MCU_min(t_right, t_cell_base -> x);
					else
						continue;

					// Add Span
					uint1 t_alpha;
					if (p_even_odd)
						t_alpha = antialias_compute_alpha_even_odd(t_cover << 8);
					else
						t_alpha = antialias_compute_alpha_non_zero(t_cover << 8);

					while(t_span_left < t_span_right)
					{
						*t_mask_ptr++ = t_alpha;
						t_span_left += 1;
					}

					if (t_span_right == t_right)
						break;
				}
				
				p_combiner -> combine(p_combiner, t_first_x - 32768, t_span_left - 32768, t_mask);
				
				for(; t_cell_base -> y == t_y; ++t_cell_base)
					;
			}
			delete t_mask;
		}

		p_combiner -> end(p_combiner);
	}

	if (!err)
		t_cells . resize(8192);

	return false;
}

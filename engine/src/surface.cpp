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
#include "typedefs.h"

////

#ifdef __VISUALC__
#pragma optimize("ags", on)
#define INLINE __forceinline
#else
#define INLINE inline
#endif

////

void surface_merge_with_mask(void *p_pixels, uint4 p_pixel_stride, void *p_mask, uint4 p_mask_stride, uint4 p_offset, uint4 p_width, uint4 p_height)
{
	uint4 *t_pixel_ptr;
	uint4 t_pixel_stride;

	uint1 *t_mask_ptr;
	uint4 t_mask_stride;

	t_pixel_ptr = (uint4 *)p_pixels;
	t_pixel_stride = p_pixel_stride >> 2;

	t_mask_ptr = (uint1 *)p_mask;
	t_mask_stride = p_mask_stride;
	
	for(uint4 y = p_height; y > 0; --y, t_pixel_ptr += t_pixel_stride, t_mask_ptr += t_mask_stride)
	{
		uint4 t_byte, t_bit;
		uint1 *t_bytes;

		t_bytes = t_mask_ptr;
		t_bit = 0x80 >> p_offset;
		t_byte = *t_bytes++;

		for(uint4 x = 0; x < p_width; ++x)
		{
			if ((t_byte & t_bit) != 0)
				t_pixel_ptr[x] = 0xFF000000 | (t_pixel_ptr[x] & 0xFFFFFF);
			else
				t_pixel_ptr[x] = 0;

			t_bit >>= 1;
			if (!t_bit && x < p_width - 1)
				t_bit = 0x80, t_byte = *t_bytes++;
		}
	}
}

void surface_extract_mask(void *p_pixels, uint4 p_pixel_stride, void *p_mask, uint4 p_mask_stride, uint4 p_width, uint4 p_height, uint1 p_threshold)
{
	uint1 *t_pixel_ptr;
	uint4 t_pixel_stride;

	uint1 *t_mask_ptr;
	uint4 t_mask_stride;

	t_pixel_ptr = (uint1 *)p_pixels + 3;
	t_pixel_stride = p_pixel_stride;

	t_mask_ptr = (uint1 *)p_mask;
	t_mask_stride = p_mask_stride;
	
	for(uint4 y = p_height; y > 0; --y, t_pixel_ptr += t_pixel_stride, t_mask_ptr += t_mask_stride)
	{
		uint4 t_byte, t_bit;
		uint1 *t_bytes;

		t_bytes = t_mask_ptr;
		t_bit = 0x80;
		t_byte = 0;

		for(uint4 x = 0; x < p_width; ++x)
		{
			if (t_pixel_ptr[x * 4] > p_threshold)
				t_byte |= t_bit;

			t_bit >>= 1;
			if (!t_bit)
				t_bit = 0x80, *t_bytes++ = t_byte, t_byte = 0;
		}
		if (t_bit != 0x80)
			*t_bytes = t_byte;
	}
}

////

void surface_merge_with_alpha(void *p_pixels, uint4 p_pixel_stride, void *p_alpha, uint4 p_alpha_stride, uint4 p_width, uint4 p_height)
{
	uint4 *t_pixel_ptr;
	uint4 t_pixel_stride;

	uint1 *t_alpha_ptr;
	uint4 t_alpha_stride;

	t_pixel_ptr = (uint4 *)p_pixels;
	t_pixel_stride = p_pixel_stride >> 2;

	t_alpha_ptr = (uint1 *)p_alpha;
	t_alpha_stride = p_alpha_stride;

	for(uint4 y = p_height; y > 0; --y, t_pixel_ptr += t_pixel_stride, t_alpha_ptr += t_alpha_stride)
		for(uint4 x = 0; x < p_width; ++x)
		{
			uint4 s = t_pixel_ptr[x];
			uint1 a = t_alpha_ptr[x];
			if (a == 0)
				t_pixel_ptr[x] = 0;
			else if (a == 255)
				t_pixel_ptr[x] = (t_pixel_ptr[x] & 0xFFFFFF) | 0xFF000000;
			else
			{
				uint4 u, v;
				u = ((s & 0xff00ff) * a) + 0x800080;
				u = ((u + ((u >> 8) & 0xff00ff)) >> 8) & 0xff00ff;
				v = ((s & 0x00ff00) * a) + 0x8000;
				v = ((v + ((v >> 8) & 0x00ff00)) >> 8) & 0x00ff00;
				t_pixel_ptr[x] = (u + v) | (a << 24);
			}
		}
}

void surface_extract_alpha(void *p_pixels, uint4 p_pixel_stride, void *p_alpha, uint4 p_alpha_stride, uint4 p_width, uint4 p_height)
{
	uint1 *t_pixel_ptr;
	uint4 t_pixel_stride;

	uint1 *t_alpha_ptr;
	uint4 t_alpha_stride;

	t_pixel_ptr = (uint1 *)p_pixels + 3;
	t_pixel_stride = p_pixel_stride;

	t_alpha_ptr = (uint1 *)p_alpha;
	t_alpha_stride = p_alpha_stride;

	for(uint4 y = p_height; y > 0; --y, t_pixel_ptr += t_pixel_stride, t_alpha_ptr += t_alpha_stride)
		for(uint4 x = 0; x < p_width; ++x)
			t_alpha_ptr[x] = t_pixel_ptr[x * 4];
}

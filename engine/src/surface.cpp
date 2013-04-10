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

#include "prefix.h"
#include "typedefs.h"

#pragma optimize("ags", on)

////

#ifdef __VISUALC__
#define INLINE __forceinline
#else
#define INLINE inline
#endif

void surface_merge(void *p_pixels, uint4 p_pixel_stride, uint4 p_width, uint4 p_height)
{
	uint1 *t_pixel_ptr;
	uint4 t_pixel_stride;

	t_pixel_ptr = (uint1 *)p_pixels + 3;
	t_pixel_stride = p_pixel_stride;

	for(uint4 y = p_height; y > 0; --y, t_pixel_ptr += t_pixel_stride)
		for(uint4 x = 0; x < p_width; ++x)
			t_pixel_ptr[x * 4] = 0xFF;
}

void surface_unmerge(void *p_pixels, uint4 p_pixel_stride, uint4 p_width, uint4 p_height)
{
	uint1 *t_pixel_ptr;
	uint4 t_pixel_stride;

	t_pixel_ptr = (uint1 *)p_pixels + 3;
	t_pixel_stride = p_pixel_stride;

	for(uint4 y = p_height; y > 0; --y, t_pixel_ptr += t_pixel_stride)
		for(uint4 x = 0; x < p_width; ++x)
			t_pixel_ptr[x * 4] = 0x00;
}

static INLINE uint32_t packed_divide_bounded(uint32_t x, uint8_t a)
{
	uint32_t u, v, w;
	u = ((((x & 0xff0000) << 8) - (x & 0xff0000)) / a) & 0xff0000;
	v = ((((x & 0x00ff00) << 8) - (x & 0x00ff00)) / a) & 0x00ff00;
	w = ((((x & 0x0000ff) << 8) - (x & 0x0000ff)) / a) & 0x0000ff;
	return u | v | w;
}

void surface_unmerge_pre(void *p_pixels, uint4 p_pixel_stride, uint4 p_width, uint4 p_height)
{
	uint4 *t_pixel_ptr;
	uint4 t_pixel_stride;

	t_pixel_ptr = (uint4 *)p_pixels;
	t_pixel_stride = p_pixel_stride >> 2;

	for(uint4 y = p_height; y > 0; --y, t_pixel_ptr += t_pixel_stride)
		for(uint4 x = 0; x < p_width; ++x)
		{
			uint1 t_alpha;
			t_alpha = t_pixel_ptr[x] >> 24;
			if (t_alpha == 0)
				t_pixel_ptr[x] = 0;
			else if (t_alpha != 255)
				t_pixel_ptr[x] = packed_divide_bounded(t_pixel_ptr[x], t_alpha);
		}
}

void surface_unmerge_pre_checking(void *p_pixels, uint4 p_pixel_stride, uint4 p_width, uint4 p_height)
{
	uint4 *t_pixel_ptr;
	uint4 t_pixel_stride;

	t_pixel_ptr = (uint4 *)p_pixels;
	t_pixel_stride = p_pixel_stride >> 2;

	for(uint4 y = p_height; y > 0; --y, t_pixel_ptr += t_pixel_stride)
		for(uint4 x = 0; x < p_width; ++x)
		{
			uint1 t_alpha;
			t_alpha = t_pixel_ptr[x] >> 24;
			if (t_alpha == 0)
				t_pixel_ptr[x] = 0;
			else if (t_alpha != 255)
			{
				uint4 t_brokenbits = 0;
				if ((t_pixel_ptr[x] & 0xFF) > t_alpha)
					t_brokenbits |= 0xFF;
				if (((t_pixel_ptr[x] >> 8) & 0xFF) > t_alpha)
					t_brokenbits |= 0xFF00;
				if (((t_pixel_ptr[x] >> 16) & 0xFF) > t_alpha)
					t_brokenbits |= 0xFF0000;
				t_pixel_ptr[x] = packed_divide_bounded(t_pixel_ptr[x], t_alpha) | t_brokenbits;
			}
		}
}

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

void surface_merge_with_alpha_non_pre(void *p_pixels, uint4 p_pixel_stride, void *p_alpha, uint4 p_alpha_stride, uint4 p_width, uint4 p_height)
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
			t_pixel_ptr[x * 4] = t_alpha_ptr[x];
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

////

void surface_merge_with_alpha_and_mask(void *p_pixels, uint4 p_pixel_stride, void *p_alpha, uint4 p_alpha_stride, void *p_mask, uint4 p_mask_stride, uint4 p_offset, uint4 p_width, uint4 p_height)
{
	uint4 *t_pixel_ptr;
	uint4 t_pixel_stride;

	uint1 *t_mask_ptr;
	uint4 t_mask_stride;

	uint1 *t_alpha_ptr;
	uint4 t_alpha_stride;

	t_pixel_ptr = (uint4 *)p_pixels;
	t_pixel_stride = p_pixel_stride >> 2;

	t_mask_ptr = (uint1 *)p_mask;
	t_mask_stride = p_mask_stride;

	t_alpha_ptr = (uint1 *)p_alpha;
	t_alpha_stride = p_alpha_stride;

	for(uint4 y = p_height; y > 0; --y, t_pixel_ptr += t_pixel_stride, t_alpha_ptr += t_alpha_stride, t_mask_ptr += t_mask_stride)
	{
		uint4 t_byte, t_bit;
		uint1 *t_bytes;

		t_bytes = t_mask_ptr;
		t_bit = 0x80 >> p_offset;
		t_byte = *t_bytes++;

		for(uint4 x = 0; x < p_width; ++x)
		{
			if ((t_byte & t_bit) != 0)
			{
				uint4 s = t_pixel_ptr[x];
				uint1 a = t_alpha_ptr[x];
				uint4 u, v;

				if (a == 0)
					t_pixel_ptr[x] = 0;
				else if (a == 255)
					t_pixel_ptr[x] = (s & 0xffffff) | 0xff000000;
				else
				{
					u = ((s & 0xff00ff) * a) + 0x800080;
					u = ((u + ((u >> 8) & 0xff00ff)) >> 8) & 0xff00ff;
					v = ((s & 0x00ff00) * a) + 0x8000;
					v = ((v + ((v >> 8) & 0x00ff00)) >> 8) & 0x00ff00;
					t_pixel_ptr[x] = (u + v) | (a << 24);
				}
			}
			else
				t_pixel_ptr[x] = 0;
			t_bit >>= 1;
			if (!t_bit && x < p_width - 1)
				t_bit = 0x80, t_byte = *t_bytes, t_bytes++;
		}
	}
}


void surface_mask_flush_to_alpha_base(void *p_alpha_ptr, uint4 p_alpha_stride, void * p_mask_ptr, uint4 p_mask_stride, uint4 sw, uint4 sh, uint1 p_value)
{
	
	uint4 t_pixel_stride ;
	uint4 t_mask_stride ;
	
	uint1 * t_pixel_ptr ;
	uint1 * t_mask_ptr ;
	
	uint4 p_w, p_h ;
	
	t_pixel_stride = p_alpha_stride ;
	t_mask_stride = p_mask_stride ;

	p_w = sw ;
	p_h = sh ;

	t_pixel_ptr = (uint1 *)p_alpha_ptr ;
	t_mask_ptr = (uint1 *)p_mask_ptr ;
	
	for(uint4 y = p_h; y > 0; --y, t_pixel_ptr += t_pixel_stride, t_mask_ptr += t_mask_stride)
	{
		uint4 x;

		uint1 *t_pixels = t_pixel_ptr;
		uint1 *t_mskels = t_mask_ptr;

		for(x = p_w; x >= 8; x -= 8)
		{
			uint1 b = *t_mskels;
			*t_mskels++ = 0;

			if ((b & (1 << 7)) != 0) t_pixels[0] = p_value;
			if ((b & (1 << 6)) != 0) t_pixels[1] = p_value;
			if ((b & (1 << 5)) != 0) t_pixels[2] = p_value;
			if ((b & (1 << 4)) != 0) t_pixels[3] = p_value;
			if ((b & (1 << 3)) != 0) t_pixels[4] = p_value;
			if ((b & (1 << 2)) != 0) t_pixels[5] = p_value;
			if ((b & (1 << 1)) != 0) t_pixels[6] = p_value;
			if ((b & (1 << 0)) != 0) t_pixels[7] = p_value;

			t_pixels += 8;
		}

		if (x == 0)
			continue;

		uint1 b = *t_mskels;
		*t_mskels = 0;

		switch(7 - x)
		{
		case 0: if ((b & (1 << 1)) != 0) t_pixels[6] = p_value;
		case 1: if ((b & (1 << 2)) != 0) t_pixels[5] = p_value;
		case 2:	if ((b & (1 << 3)) != 0) t_pixels[4] = p_value;
		case 3: if ((b & (1 << 4)) != 0) t_pixels[3] = p_value;
		case 4: if ((b & (1 << 5)) != 0) t_pixels[2] = p_value;
		case 5: if ((b & (1 << 6)) != 0) t_pixels[1] = p_value;
		case 6: if ((b & (1 << 7)) != 0) t_pixels[0] = p_value;
		default:
			break;
		}
	}
	
	
}


void surface_mask_flush_to_alpha(void *p_alpha_ptr, uint4 p_alpha_stride, void * p_mask_ptr, uint4 p_mask_stride, uint4 sw, uint4 sh)
{
	surface_mask_flush_to_alpha_base( p_alpha_ptr, p_alpha_stride, p_mask_ptr, p_mask_stride, sw, sh, 0xff );
}

void surface_mask_flush_to_alpha_inverted(void *p_alpha_ptr, uint4 p_alpha_stride, void * p_mask_ptr, uint4 p_mask_stride, uint4 sw, uint4 sh)
{
	surface_mask_flush_to_alpha_base( p_alpha_ptr, p_alpha_stride, p_mask_ptr, p_mask_stride, sw, sh, 0x00 );
}


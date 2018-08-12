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
#include "parsedef.h"
#include "globals.h"

#include "uidc.h"
#include "objdefs.h"

#include "image.h"

#include "iquantization.h"

//#define IQSQUARELOOKUP
#define IQDISTLOOKUP

#ifdef IQSQUARELOOKUP
uint32_t *s_square_lookup = NULL;
bool init_s_square_lookup()
{
	if (s_square_lookup)
		return true;
	if (!MCMemoryNewArray<uint32_t>(256, s_square_lookup))
		return false;
	for (uint32_t i=0; i<256; i++)
		s_square_lookup[i] = i * i;
	return true;
}
#elif defined(IQDISTLOOKUP)
uint32_t *s_dist_lookup = NULL;
bool init_s_dist_lookup()
{
	if (s_dist_lookup)
		return true;
	if (!MCMemoryNewArray<uint32_t>(256*256, s_dist_lookup))
		return false;
	for (uint32_t i=0; i<256; i++)
		for (uint32_t j=0; j<256; j++)
		{
			int32_t t_dist = (int32_t)i - j;
			s_dist_lookup[(i << 8) + j] = t_dist * t_dist;
		}
	return true;
}
#endif

uint32_t MCImageMapColorToPalette(uint32_t p_pixel, MCColor *p_palette, uint32_t p_palette_size)
{
	uint8_t rb, gb, bb, ab;
	MCGPixelUnpackNative(p_pixel, rb, gb, bb, ab);

	uint32_t t_mindist = MAXUINT4;
	uint32_t t_mincell = 0;
#if defined(IQDISTLOOKUP)
	uint32_t *t_rlt = s_dist_lookup + (rb << 8);
	uint32_t *t_glt = s_dist_lookup + (gb << 8);
	uint32_t *t_blt = s_dist_lookup + (bb << 8);
#endif
	for (uint32_t i = 0 ; i < p_palette_size && t_mindist != 0 ; i++)
	{
#ifdef IQSQUARELOOKUP
		uint32_t t_dist = 0;
		uint32_t r = MCU_abs(int32_t(rb) - (p_palette[i].red >> 8));
		t_dist += s_square_lookup[r];
		if (t_dist >= t_mindist)
			continue;
		uint32_t g = MCU_abs(int32_t(gb) - (p_palette[i].green >> 8));
		t_dist += s_square_lookup[g];
		if (t_dist >= t_mindist)
			continue;
		uint32_t b = MCU_abs(int32_t(bb) - (p_palette[i].blue >> 8));
		t_dist += s_square_lookup[b];
		if (t_dist >= t_mindist)
			continue;
		{
			t_mindist = t_dist;
			t_mincell = i;
		}
#elif defined IQDISTLOOKUP
		uint32_t t_dist = 0;
		t_dist += t_rlt[(p_palette[i].red >> 8)];
		if (t_dist < t_mindist)
		{
			t_dist += t_glt[(p_palette[i].green >> 8)];
			if (t_dist < t_mindist)
			{
				t_dist += t_blt[(p_palette[i].blue >> 8)];
				if (t_dist < t_mindist)
				{
					t_mindist = t_dist;
					t_mincell = i;
				}
			}
		}
#else
		uint32_t t_dist = r * r + g * g + b * b;
		if (t_dist < t_mindist)
		{
			t_mindist = t_dist;
			t_mincell = i;
		}
#endif
	}

	return t_mincell;
}

static inline uint32_t apply_error(uint32_t p_pixel, int32_t p_re, int32_t p_ge, int32_t p_be)
{
	uint8_t r, g, b, a;
	MCGPixelUnpackNative(p_pixel, r, g, b, a);
	r = MCClamp(int32_t(r) + p_re, 0, 255);
	g = MCClamp(int32_t(g) + p_ge, 0, 255);
	b = MCClamp(int32_t(b) + p_be, 0, 255);
	return MCGPixelPackNative(r, g, b, a);
}

static inline uint32_t apply_error(uint32_t p_pixel, int32_t *p_errors)
{
	return apply_error(p_pixel, p_errors[0], p_errors[1], p_errors[2]);
}

static void accumulate_error(int32_t *p_errors, int32_t p_re, int32_t p_ge, int32_t p_be, int32_t p_weight)
{
	p_errors[0] += (p_weight * p_re + 8) / 16;
	p_errors[1] += (p_weight * p_ge + 8) / 16;
	p_errors[2] += (p_weight * p_be + 8) / 16;
}

bool MCImageQuantizeImageBitmap(MCImageBitmap *p_bitmap, MCColor *p_colors, uindex_t p_color_count, bool p_dither, bool p_add_transparency_index, MCImageIndexedBitmap *&r_indexed)
{
	bool t_success = true;

#if defined(IQSQUARELOOKUP)
	init_s_square_lookup();
#elif defined(IQDISTLOOKUP)
	init_s_dist_lookup();
#endif

	MCImageIndexedBitmap *t_indexed = nil;

	int32_t *t_error_buffer = nil;

	t_success = MCMemoryNewArray<int32_t>(p_bitmap->width * 3 * 2, t_error_buffer);
	int32_t *t_current_errors = t_error_buffer;
	int32_t *t_next_errors = t_error_buffer + p_bitmap->width * 3;

	if (t_success)
		t_success = MCImageCreateIndexedBitmap(p_bitmap->width, p_bitmap->height, t_indexed);

	if (t_success)
	{
		MCMemoryCopy(t_indexed->palette, p_colors, p_color_count * sizeof(MCColor));
		t_indexed->palette_size = p_color_count;
	}

	uint8_t *t_src_ptr, *t_dst_ptr;
	if (t_success)
	{
		t_src_ptr = (uint8_t*)p_bitmap->data;
		t_dst_ptr = (uint8_t*)t_indexed->data;
	}

	for (uint32_t y = 0; t_success && y < p_bitmap->height; y++)
	{
		uint32_t *t_src_row = (uint32_t*)t_src_ptr;
		uint8_t *t_dst_row = t_dst_ptr;

		uint32_t width = p_bitmap->width;
		uint32_t t_error_index = 0;
		if (!p_dither)
		{
			while (width--)
			{
				uint32_t t_pixel = *t_src_row++;
				if (MCGPixelGetNativeAlpha(t_pixel) == 0 && p_add_transparency_index)
				{
					t_success = MCImageIndexedBitmapAddTransparency(t_indexed);
					if (t_success)
						*t_dst_row++ = t_indexed->transparent_index;
				}
				else
					*t_dst_row++ = MCImageMapColorToPalette(t_pixel, p_colors, p_color_count);
			}
		}
		else
		{
			bool reverse = false;
			int32_t t_direction = 1;
			if (y & 1)
			{
				t_src_row += width - 1;
				t_dst_row += width - 1;
				reverse = true;
				t_direction = -1;
				t_error_index = (width - 1) * 3;
			}
			while (width--)
			{
				uint32_t t_pixel = *t_src_row;
				if (MCGPixelGetNativeAlpha(t_pixel) == 0 && p_add_transparency_index)
				{
					t_success = MCImageIndexedBitmapAddTransparency(t_indexed);
					if (t_success)
						*t_dst_row = t_indexed->transparent_index;
				}
				else
				{
					t_pixel = apply_error(t_pixel, &t_current_errors[t_error_index]);
					uint32_t t_index = MCImageMapColorToPalette(t_pixel, p_colors, p_color_count);
					uint8_t r, g, b, a;
					MCGPixelUnpackNative(t_pixel, r, g, b, a);
					
					int32_t re = (int32_t)(r) - (int32_t)(p_colors[t_index].red >> 8);
					int32_t ge = (int32_t)(g) - (int32_t)(p_colors[t_index].green >> 8);
					int32_t be = (int32_t)(b) - (int32_t)(p_colors[t_index].blue >> 8);
					*t_dst_row = t_index;
					if (width > 0)
						accumulate_error(t_current_errors + t_error_index + (t_direction * 3), re, ge, be, 7);
					if (y + 1 < p_bitmap->height)
					{
						if (width + 1 < p_bitmap->width)
							accumulate_error(t_next_errors + t_error_index - (t_direction * 3), re, ge, be, 3);
						accumulate_error(t_next_errors + t_error_index, re, ge, be, 5);
						if (width > 0)
							accumulate_error(t_next_errors + t_error_index + (t_direction * 3), re, ge, be, 1);
					}
				}
				t_src_row += t_direction;
				t_dst_row += t_direction;
				t_error_index += (t_direction * 3);
			}
			int32_t *t_tmp_ptr;
			t_tmp_ptr = t_next_errors; t_next_errors = t_current_errors; t_current_errors = t_tmp_ptr;
			MCMemoryClear(t_next_errors, sizeof(int32_t) * 3 * p_bitmap->width);
		}
		t_src_ptr += p_bitmap->stride;
		t_dst_ptr += t_indexed->stride;
	}

	MCMemoryDeleteArray(t_error_buffer);

	if (t_success)
		r_indexed = t_indexed;
	else
		MCImageFreeIndexedBitmap(t_indexed);

	return t_success;
}

// IM-2013-11-11: [[ RefactorGraphics ]] Fix broken implementation and modify
// to work on bitmap data in-place.
bool MCImageDitherAlphaInPlace(MCImageBitmap *p_bitmap)
{
	uint8_t *t_src_ptr = (uint8_t*)p_bitmap->data;

	int32_t *t_error_buffer;
	if (!MCMemoryNewArray<int32_t>(p_bitmap->width * 2, t_error_buffer))
		return false;

	int32_t *t_current_errors = t_error_buffer;
	int32_t *t_next_errors = t_error_buffer + p_bitmap->width;

	for (uint32_t y = 0; y < p_bitmap->height; y++)
	{
		uint32_t *t_src_row = (uint32_t*)t_src_ptr;

		uint32_t width = p_bitmap->width;
		uint32_t t_error_index = 0;

		bool reverse = false;
		int32_t t_direction = 1;

		if (y & 1)
		{
			t_src_row += width - 1;

			reverse = true;
			t_direction = -1;

			t_error_index = (width - 1);
		}

		while (width--)
		{
			int32_t t_alpha = (int32_t)MCGPixelGetNativeAlpha(*t_src_row) + t_current_errors[t_error_index];
			int32_t t_newalpha = t_alpha < 128 ? 0 : 255;

			int32_t t_error = t_alpha - t_newalpha;
			*t_src_row = MCGPixelSetNativeAlpha(*t_src_row, t_newalpha);

			if (width > 0)
				t_current_errors[t_error_index + t_direction] += (7 * t_error + 8) / 16;
			if (y + 1 < p_bitmap->height)
			{
				if (width + 1 < p_bitmap->width)
					t_next_errors[t_error_index - t_direction] += (3 * t_error + 8) / 16;
				t_next_errors[t_error_index] += (5 * t_error + 8) / 16;
				if (width > 0)
					t_next_errors[t_error_index + t_direction] += (1 * t_error + 8) / 16;
			}

			t_src_row += t_direction;
			t_error_index += t_direction;
		}
		int32_t *t_tmp_ptr;
		t_tmp_ptr = t_next_errors; t_next_errors = t_current_errors; t_current_errors = t_tmp_ptr;
		MCMemoryClear(t_next_errors, sizeof(int32_t) * p_bitmap->width);

		t_src_ptr += p_bitmap->stride;
	}

	MCMemoryDeleteArray(t_error_buffer);

	p_bitmap->has_alpha = false;

	return true;
}

bool MCImageGenerateWebsafePalette(uint32_t &r_palette_size, MCColor *&r_colours)
{
	bool t_success = true;

	if (r_colours == NULL)
		t_success = MCMemoryNewArray<MCColor>(216, r_colours);

	if (t_success)
	{
		int32_t t_index = 0;
		for (int32_t red = 0xFF; red >= 0; red -= 0x33)
			for (int32_t green = 0xFF; green >= 0; green -= 0x33)
				for (int32_t blue = 0xFF; blue >=0; blue -= 0x33)
				{
					r_colours[t_index].red = (red << 8) | red;
					r_colours[t_index].green = (green << 8) | green;
					r_colours[t_index].blue = (blue << 8) | blue;
				}
		r_palette_size = t_index;
	}

	return t_success;
}

int32_t int_cmp(const void *a, const void *b)
{
	return *(uint32_t*)a - *(uint32_t*)b;
}

bool MCImageParseColourList(MCStringRef p_input, uint32_t &r_ncolours, MCColor *&r_colours)
{
	bool t_success = true;

	t_success = !MCStringIsEmpty(p_input);

	MCColor *t_colours = NULL;
	uint32_t t_ncolours = 0;
	
	if (t_success)
	{
		MCAutoArrayRef t_lines;
        uindex_t t_nlines = 0;
        /* UNCHECKED */ MCStringSplit(p_input, MCSTR("\n"), nil, kMCStringOptionCompareExact, &t_lines);
        t_nlines = MCArrayGetCount(*t_lines);
        t_success = MCMemoryNewArray<MCColor>(t_nlines, t_colours);
        
        for (uindex_t i = 0; i < t_nlines; i++)
        {
            MCValueRef t_line = nil;
            /* UNCHECKED */ MCArrayFetchValueAtIndex(*t_lines, i + 1, t_line);
            MCStringRef t_color;
            t_color = (MCStringRef)t_line;
                            
            if (!MCscreen->parsecolor(t_color, t_colours[i], NULL))
			{
				t_success = false;
				break;
			}
		}
        
        // PM-2015-05-12: [[ Bug 15359 ]] Update t_ncolours var
        t_ncolours = t_nlines;
	}

	if (t_success)
	{
		r_ncolours = t_ncolours;
		r_colours = t_colours;
	}
	else
		MCMemoryDeleteArray(t_colours);

	return t_success;
}

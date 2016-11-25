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

#include "objdefs.h"
#include "parsedef.h"
#include "globals.h"

#include "uidc.h"

#include "image.h"

#include "imagebitmap.h"

////////////////////////////////////////////////////////////////////////////////

static bool check_point(MCImageBitmap *p_bitmap, int32_t x, int32_t y)
{
	return (x >= 0) && (x < p_bitmap->width) && (y >= 0) && (y < p_bitmap->height);
}

static bool check_bounds(MCImageBitmap *p_bitmap, int32_t x, int32_t y, uint32_t width, uint32_t height)
{
	return check_point(p_bitmap, x, y) && check_point(p_bitmap, x + width - 1, y + height - 1);
}

static void clamp_region(MCImageBitmap *p_bitmap, int32_t &x, int32_t &y, uint32_t &width, uint32_t &height)
{
	int32_t t_x1, t_y1, t_x2, t_y2;

	t_x1 = MCMax(0, MCMin((int32_t)p_bitmap->width, x));
	t_y1 = MCMax(0, MCMin((int32_t)p_bitmap->height, y));
	t_x2 = MCMax(0, MCMin((int32_t)p_bitmap->width, x + (int32_t)width));
	t_y2 = MCMax(0, MCMin((int32_t)p_bitmap->height, y + (int32_t)height));

	x = t_x1;
	y = t_y1;
	width = t_x2 - t_x1;
	height = t_y2 - t_y1;
}

bool MCImageBitmapCreate(uindex_t p_width, uindex_t p_height, MCImageBitmap *&r_bitmap)
{
	MCImageBitmap *t_bitmap = nil;

	if (!MCMemoryNew(t_bitmap))
		return false;

	t_bitmap->width = p_width;
	t_bitmap->height = p_height;
	
	t_bitmap->has_alpha = false;
	t_bitmap->has_transparency = false;

	t_bitmap->stride = p_width * 4;
	uindex_t t_data_size = t_bitmap->stride * p_height;

	if (!MCMemoryAllocate(t_data_size, t_bitmap->data))
	{
		MCMemoryDelete(t_bitmap);
		return false;
	}

	r_bitmap = t_bitmap;
	return true;
}

void MCImageFreeBitmap(MCImageBitmap *p_bitmap)
{
	if (p_bitmap != nil)
	{
		MCMemoryDeallocate(p_bitmap->data);
		MCMemoryDelete(p_bitmap);
	}
}

bool MCImageCopyBitmap(MCImageBitmap *p_bitmap, MCImageBitmap *&r_copy)
{
	if (p_bitmap == nil || !MCImageBitmapCreate(p_bitmap->width, p_bitmap->height, r_copy))
		return false;

	r_copy->has_alpha = p_bitmap->has_alpha;
	r_copy->has_transparency = p_bitmap->has_transparency;
	
	MCMemoryCopy(r_copy->data, p_bitmap->data, r_copy->stride * r_copy->height);
	return true;
}

bool MCImageCopyBitmapRegion(MCImageBitmap *p_bitmap, MCRectangle &p_region, MCImageBitmap *&r_copy)
{
	if (p_region.x == 0 && p_region.y == 0 && 
		p_region.width == p_bitmap->width && p_region.height == p_bitmap->height)
		return MCImageCopyBitmap(p_bitmap, r_copy);

	MCAssert(check_bounds(p_bitmap, p_region.x, p_region.y, p_region.width, p_region.height));

	if (!MCImageBitmapCreate(p_region.width, p_region.height, r_copy))
		return false;

	MCImageBitmapCopyRegionToBitmap(p_bitmap, r_copy, p_region.x, p_region.y, 0, 0, p_region.width, p_region.height);

	if (p_bitmap->has_transparency)
		MCImageBitmapCheckTransparency(r_copy);
	
	return true;
}

//////////

void MCImageBitmapClear(MCImageBitmap *p_bitmap)
{
	MCMemoryClear(p_bitmap->data, p_bitmap->height * p_bitmap->stride);

	p_bitmap->has_transparency = true;
}

void MCImageBitmapClearRegion(MCImageBitmap *p_bitmap, MCRectangle p_region)
{
	MCAssert(check_bounds(p_bitmap, p_region.x, p_region.y, p_region.width, p_region.height));

	uint32_t t_line_bytes = p_region.width * sizeof(uint32_t);
	uint8_t *t_row_ptr = (uint8_t*)p_bitmap->data + p_region.y * p_bitmap->stride + p_region.x * sizeof(uint32_t);
	for (uindex_t i = 0; i < p_region.height; i++)
	{
		MCMemoryClear(t_row_ptr, t_line_bytes);
		t_row_ptr += p_bitmap->stride;
	}

	p_bitmap->has_transparency = true;
}

//////////

void MCImageBitmapSet(MCImageBitmap *p_bitmap, uint32_t p_pixel_value)
{
	uint8_t *t_row_ptr = (uint8_t*)p_bitmap->data;
	for (uindex_t y = 0; y < p_bitmap->height; y++)
	{
		uint32_t *t_pix_ptr = (uint32_t*)t_row_ptr;
		for (uindex_t x = 0; x < p_bitmap->width; x++)
			*t_pix_ptr++ = p_pixel_value;
		t_row_ptr += p_bitmap->stride;
	}
}

void MCImageBitmapSetMask(MCImageBitmap *p_bitmap, uint8_t *p_mask_data, uint32_t p_mask_stride)
{
	uint8_t *t_src_ptr = p_mask_data;
	uint8_t *t_dst_ptr = (uint8_t*)p_bitmap->data;
	uindex_t t_height = p_bitmap->height;
	bool t_has_transparency = false;
	while (t_height--)
	{
		uint8_t *t_src_row = (uint8_t*)t_src_ptr;
		uint32_t *t_dst_row = (uint32_t*)t_dst_ptr;
		uindex_t t_width = p_bitmap->width;
		uint8_t t_bit = 0;
		uint8_t t_byte = 0;
		while (t_width--)
		{
			if (t_bit == 0)
			{
				t_bit = 0x80;
				t_byte = *t_src_row++;
			}
			if (t_bit & t_byte)
				*t_dst_row++ |= 0xFF000000;
			else
			{
				t_has_transparency = true;
				*t_dst_row++ &= 0x00FFFFFF;
			}
			t_bit >>= 1;
		}
		t_src_ptr += p_mask_stride;
		t_dst_ptr += p_bitmap->stride;
	}
	
	p_bitmap->has_transparency = t_has_transparency;
	p_bitmap->has_alpha = false;
}

//////////

static void copy_bitmap_data(uint8_t *p_dst, uindex_t p_dst_stride, const uint8_t *p_src, uindex_t p_src_stride, uindex_t p_width, uindex_t p_height)
{
	uint32_t t_line_bytes = p_width * sizeof(uint32_t);
	for (uindex_t y = 0; y < p_height; y++)
	{
		MCMemoryCopy(p_dst, p_src, t_line_bytes);
		p_dst += p_dst_stride;
		p_src += p_src_stride;
	}
}

void MCImageBitmapCopyRegionToBitmap(MCImageBitmap *p_src, MCImageBitmap *p_dst, int32_t sx, int32_t sy, int32_t dx, int32_t dy, uint32_t sw, uint32_t sh)
{
	int32_t t_src_x, t_src_y;
	int32_t t_dst_x, t_dst_y;
	uint32_t t_width, t_height;

	t_src_x = MCMax(0, sx);
	t_src_y = MCMax(0, sy);

	t_dst_x = MCMax(0, dx);
	t_dst_y = MCMax(0, dy);

    // SN-2014-01-31: [[ Bug 10910 ]] Make sure width and height
    // aren't less than 0 in the signed comparison.
    // The second comparison is done with unsigned integers, since the
    // floor of t_height/t_width is ensured to be 0.
	t_width = MCMax(MCMin((int32_t)p_src->width - t_src_x, (int32_t)sw), 0);
	t_height = MCMax(MCMin((int32_t)p_src->height - t_src_y, (int32_t)sh), 0);

	t_width = MCMin((uint32_t)p_dst->width - t_dst_x, (uint32_t)t_width);
	t_height = MCMin((uint32_t)p_dst->height - t_dst_y, (uint32_t)t_height);

	copy_bitmap_data((uint8_t*)p_dst->data + p_dst->stride * t_dst_y + t_dst_x * sizeof(uint32_t), p_dst->stride, (uint8_t*)p_src->data + p_src->stride * t_src_y + t_src_x * sizeof(uint32_t), p_src->stride, t_width, t_height);

	if (p_src->has_transparency || p_dst->has_transparency)
		MCImageBitmapCheckTransparency(p_dst);
}

void MCImageBitmapCopyRegionToBitmap(MCImageBitmap *p_dst, MCImageBitmap *p_src, MCPoint p_dst_offset, MCRectangle p_src_rect)
{
	int32_t t_src_x, t_src_y;
	int32_t t_dst_x, t_dst_y;
	uint32_t t_width, t_height;

	t_src_x = p_src_rect.x;
	t_src_y = p_src_rect.y;

	t_width = p_src_rect.width;
	t_height = p_src_rect.height;

	t_dst_x = p_dst_offset.x;
	t_dst_y = p_dst_offset.y;

	clamp_region(p_src, t_src_x, t_src_y, t_width, t_height);
	clamp_region(p_dst, t_dst_x, t_dst_y, t_width, t_height);

	if (t_width == 0 || t_height == 0)
		return;

	copy_bitmap_data((uint8_t*)p_dst->data + p_dst->stride * t_dst_y + t_dst_x * sizeof(uint32_t), p_dst->stride, (uint8_t*)p_src->data + p_src->stride * t_src_y + t_src_x * sizeof(uint32_t), p_src->stride, t_width, t_height);

	if (p_src->has_transparency || p_dst->has_transparency)
		MCImageBitmapCheckTransparency(p_dst);
}

void MCImageBitmapCopyRegionToBuffer(MCImageBitmap *p_bitmap, int32_t p_sx, int32_t p_sy, int32_t p_sw, int32_t p_sh, uindex_t p_buffer_stride, uint8_t *p_buffer)
{
	MCAssert(check_bounds(p_bitmap, p_sx, p_sy, p_sw, p_sh));

	copy_bitmap_data(p_buffer, p_buffer_stride,
		(uint8_t*)p_bitmap->data + p_sy * p_bitmap->stride + p_sx * sizeof(uint32_t), p_bitmap->stride,
		p_sw, p_sh);
}

void MCImageBitmapCopyRegionFromBuffer(MCImageBitmap *p_bitmap, MCRectangle &p_region, const uint8_t *p_buffer, uindex_t p_buffer_stride)
{
	MCAssert(check_bounds(p_bitmap, p_region.x, p_region.y, p_region.width, p_region.height));

	copy_bitmap_data((uint8_t*)p_bitmap->data + p_region.y * p_bitmap->stride + p_region.x * sizeof(uint32_t), p_bitmap->stride,
		p_buffer, p_buffer_stride,
		p_region.width, p_region.height);

	MCImageBitmapCheckTransparency(p_bitmap);
}

//////////

void MCImageBitmapCheckTransparency(MCImageBitmap *p_bitmap)
{
    if (p_bitmap == nil)
        return;
	uint8_t *t_row_ptr = (uint8_t*)p_bitmap->data;
	p_bitmap->has_transparency = false;
	p_bitmap->has_alpha = false;
	for (uindex_t y = 0; y < p_bitmap->height; y++)
	{
		uint32_t *t_pixel_ptr = (uint32_t*)t_row_ptr;
		
		for (uindex_t x = 0; x < p_bitmap->width; x++)
		{
			uint8_t t_alpha = *t_pixel_ptr++ >> 24;
			if (t_alpha < 255)
			{
				p_bitmap->has_transparency = true;
				if (t_alpha > 0)
				{
					p_bitmap->has_alpha = true;
					return;
				}
			}
		}
		
		t_row_ptr += p_bitmap->stride;
	}
}

bool MCImageBitmapHasTransparency(MCImageBitmap *p_bitmap)
{
    if (p_bitmap == nil)
        return false;
	return p_bitmap->has_transparency;
}

bool MCImageBitmapHasTransparency(MCImageBitmap *p_bitmap, bool &r_has_alpha)
{
    if (p_bitmap == nil)
        return false;
	r_has_alpha = p_bitmap->has_alpha;
	return p_bitmap->has_transparency;
}

//////////

uint32_t MCImageBitmapGetPixel(MCImageBitmap *p_bitmap, uindex_t x, uindex_t y)
{
	if (p_bitmap == nil || x >= p_bitmap->width || y >= p_bitmap->height)
		return 0;

	return p_bitmap->data[y * p_bitmap->stride / 4 + x];
}

void MCImageBitmapSetPixel(MCImageBitmap *p_bitmap, uindex_t x, uindex_t y, uint32_t p_pixel)
{
	p_bitmap->data[y * p_bitmap->stride / 4 + x] = p_pixel;
}

//////////

void MCImageBitmapPremultiplyRegion(MCImageBitmap *p_bitmap, int32_t p_sx, int32_t p_sy, int32_t p_sw, int32_t p_sh, uint32_t p_pixel_stride, uint32_t *p_pixel_ptr)
{
	int32_t t_src_x = MCMax(0, p_sx);
	int32_t t_src_y = MCMax(0, p_sy);

	int32_t t_dst_x = t_src_x - p_sx;
	int32_t t_dst_y = t_src_y - p_sy;

	int32_t t_width = MCMin((int32_t)p_bitmap->width - t_src_x, (int32_t)p_sw);
	int32_t t_height = MCMin((int32_t)p_bitmap->height - t_src_y, (int32_t)p_sh);

	if (t_width <= 0 || t_height <= 0)
		return;

	uint8_t *t_src_ptr = (uint8_t*)p_bitmap->data + t_src_x * 4 + t_src_y * p_bitmap -> stride;
	uint8_t *t_dst_ptr = (uint8_t*)p_pixel_ptr + t_dst_x * 4 + t_dst_y * p_pixel_stride;
	for (uindex_t y = 0; y < t_height; y++)
	{
		uint32_t *t_src_row = (uint32_t*)t_src_ptr;
		uint32_t *t_dst_row = (uint32_t *)t_dst_ptr;
		for (uindex_t x = 0; x < t_width; x++)
		{
			uint32_t t_pixel = *t_src_row++;
			uint8_t t_alpha = t_pixel >> 24;

			if (t_alpha == 0)
				t_pixel = 0;
			else if (t_alpha != 0xFF)
			{
				uint4 u, v;
				u = ((t_pixel & 0xff00ff) * t_alpha) + 0x800080;
				u = ((u + ((u >> 8) & 0xff00ff)) >> 8) & 0xff00ff;
				v = ((t_pixel & 0x00ff00) * t_alpha) + 0x8000;
				v = ((v + ((v >> 8) & 0x00ff00)) >> 8) & 0x00ff00;
				t_pixel = (u + v) | (t_alpha << 24);
			}
			*t_dst_row++ = t_pixel;
		}
		t_src_ptr += p_bitmap->stride;
		t_dst_ptr += p_pixel_stride;
	}
}

void MCImageBitmapPremultiply(MCImageBitmap *p_bitmap)
{
	if (!p_bitmap->has_transparency)
		return;

	uint8_t *t_src_ptr = (uint8_t*) p_bitmap->data;
	for (uindex_t y = 0; y < p_bitmap->height; y++)
	{
		uint32_t *t_src_row = (uint32_t*)t_src_ptr;
		for (uindex_t x = 0; x < p_bitmap->width; x++)
		{
			uint32_t t_pixel = *t_src_row;
			uint8_t t_alpha = t_pixel >> 24;

			if (t_alpha == 0)
				t_pixel = 0;
			else if (t_alpha != 0xFF)
			{
				uint4 u, v;
				u = ((t_pixel & 0xff00ff) * t_alpha) + 0x800080;
				u = ((u + ((u >> 8) & 0xff00ff)) >> 8) & 0xff00ff;
				v = ((t_pixel & 0x00ff00) * t_alpha) + 0x8000;
				v = ((v + ((v >> 8) & 0x00ff00)) >> 8) & 0x00ff00;
				t_pixel = (u + v) | (t_alpha << 24);
			}
			*t_src_row++ = t_pixel;
		}
		t_src_ptr += p_bitmap->stride;
	}
}

static inline uint32_t packed_divide_bounded(uint32_t x, uint8_t a)
{
	uint32_t u, v, w;
	u = ((((x & 0xff0000) << 8) - (x & 0xff0000)) / a) & 0xff0000;
	v = ((((x & 0x00ff00) << 8) - (x & 0x00ff00)) / a) & 0x00ff00;
	w = ((((x & 0x0000ff) << 8) - (x & 0x0000ff)) / a) & 0x0000ff;
	return u | v | w;
}

void MCImageBitmapUnpremultiply(MCImageBitmap *p_bitmap)
{
	if (!p_bitmap->has_transparency)
		return;

	uint8_t *t_src_ptr = (uint8_t*) p_bitmap->data;
	for (uindex_t y = 0; y < p_bitmap->height; y++)
	{
		uint32_t *t_src_row = (uint32_t*)t_src_ptr;
		for (uindex_t x = 0; x < p_bitmap->width; x++)
		{
			uint32_t t_pixel = *t_src_row;
			uint8_t t_alpha = t_pixel >> 24;

			if (t_alpha == 0)
				t_pixel = 0;
			else if (t_alpha != 255)
				t_pixel = (t_alpha << 24) | packed_divide_bounded(t_pixel, t_alpha);

			*t_src_row++ = t_pixel;
		}
		t_src_ptr += p_bitmap->stride;
	}
}

void MCImageBitmapFixPremultiplied(MCImageBitmap *p_bitmap)
{
	if (!p_bitmap->has_transparency)
		return;

	uint8_t *t_src_ptr = (uint8_t*) p_bitmap->data;
	for (uindex_t y = 0; y < p_bitmap->height; y++)
	{
		uint32_t *t_src_pixel = (uint32_t*)t_src_ptr;
		for (uindex_t x = 0; x < p_bitmap->width; x++)
		{
			uint32_t t_pixel = *t_src_pixel;
			uint8_t t_alpha = t_pixel >> 24;

			if (t_alpha == 0)
				t_pixel = 0;
			else if (t_alpha != 255)
			{
				if ((t_pixel & 0xFF) > t_alpha)
					t_pixel = (t_pixel & ~0xFF) | t_alpha;
				if (((t_pixel >> 8) & 0xFF) > t_alpha)
					t_pixel = (t_pixel & ~0xFF00) | (t_alpha << 8);
				if (((t_pixel >> 16) & 0xFF) > t_alpha)
					t_pixel = (t_pixel & ~0xFF0000) | (t_alpha << 16);
			}

			*t_src_pixel++ = t_pixel;
		}
		t_src_ptr += p_bitmap->stride;
	}
}

void MCImageBitmapUnpremultiplyChecking(MCImageBitmap *p_bitmap)
{
	uint8_t *t_src_ptr = (uint8_t*) p_bitmap->data;
	for (uindex_t y = 0; y < p_bitmap->height; y++)
	{
		uint32_t *t_src_row = (uint32_t*)t_src_ptr;
		for (uindex_t x = 0; x < p_bitmap->width; x++)
		{
			uint32_t t_pixel = *t_src_row;
			uint8_t t_alpha = t_pixel >> 24;

			if (t_alpha == 0)
				t_pixel = 0;
			else if (t_alpha != 255)
			{
				uint4 t_brokenbits = 0;
				if ((t_pixel & 0xFF) > t_alpha)
					t_brokenbits |= 0xFF;
				if (((t_pixel >> 8) & 0xFF) > t_alpha)
					t_brokenbits |= 0xFF00;
				if (((t_pixel >> 16) & 0xFF) > t_alpha)
					t_brokenbits |= 0xFF0000;
				t_pixel = (t_alpha << 24) | packed_divide_bounded(t_pixel, t_alpha) | t_brokenbits;
			}

			*t_src_row++ = t_pixel;
		}
		t_src_ptr += p_bitmap->stride;
	}
}

////////////////////////////////////////////////////////////////////////////////

bool MCImageCreateIndexedBitmap(uindex_t p_width, uindex_t p_height, MCImageIndexedBitmap *&r_indexed)
{
	bool t_success;

	MCImageIndexedBitmap *t_indexed = nil;

	t_success = MCMemoryNew(t_indexed) && MCMemoryNewArray(256, t_indexed->palette) && MCMemoryAllocate(p_width * p_height, t_indexed->data);

	if (t_success)
	{
		t_indexed->width = p_width;
		t_indexed->height = p_height;
		t_indexed->stride = p_width;

		t_indexed->palette_size = 0;
		t_indexed->transparent_index = 256;

		MCMemoryClear(t_indexed->data, p_width * p_height);
		r_indexed = t_indexed;
	}
	else
	{
		if (t_indexed != nil)
		{
			MCMemoryDeleteArray(t_indexed->palette);
			MCMemoryDeallocate(t_indexed->data);
			MCMemoryDelete(t_indexed);
		}
	}

	return t_success;
}

void MCImageFreeIndexedBitmap(MCImageIndexedBitmap *p_bitmap)
{
	if (p_bitmap != nil)
	{
		MCMemoryDeallocate(p_bitmap->palette);
		MCMemoryDeallocate(p_bitmap->data);
		MCMemoryDelete(p_bitmap);
	}
}

bool MCImageIndexedBitmapHasTransparency(MCImageIndexedBitmap *p_bitmap)
{
	return p_bitmap->transparent_index < p_bitmap->palette_size;
}

bool MCImageIndexedBitmapAddTransparency(MCImageIndexedBitmap *p_bitmap)
{
	if (MCImageIndexedBitmapHasTransparency(p_bitmap))
		return true;
	if (p_bitmap->palette_size == 256)
		return false;

	p_bitmap->transparent_index = p_bitmap->palette_size;
	p_bitmap->palette_size++;
	p_bitmap->palette[p_bitmap->transparent_index].red = 0xFFFF;
	p_bitmap->palette[p_bitmap->transparent_index].green = 0xFFFF;
	p_bitmap->palette[p_bitmap->transparent_index].blue = 0xFFFF;

	return true;
}

////////////////////////////////////////////////////////////////////////////////

#define COLOR_HASH_SIZE 719

typedef struct _MCimagehash
{
	_MCimagehash *next;
	uint4 pixel;
	uint2 index;
} MCimagehash;

void MCImageFreeHashTable(MCimagehash **hashtable)
{
	uint2 filled = 0;
	uint2 maxdepth = 0;
	uint2 i;
	for (i = 0 ; i < COLOR_HASH_SIZE ; i++)
		if (hashtable[i] != NULL)
		{
			filled++;
			MCimagehash *hashentry = hashtable[i];
			uint2 depth = 0;
			while (hashentry != NULL)
			{
				depth++;
				if (depth > maxdepth)
					maxdepth = depth;
				MCimagehash *tentry = hashentry;
				hashentry = hashentry->next;
				delete tentry;
			}
		}
}

bool MCImageConvertBitmapToIndexed(MCImageBitmap *p_bitmap, bool p_ignore_transparent, MCImageIndexedBitmap *&r_indexed)
{
	bool t_success = true;

	MCImageIndexedBitmap *t_indexed = nil;

	MCimagehash *hashtable[COLOR_HASH_SIZE];
	memset((char *)hashtable, 0, COLOR_HASH_SIZE * sizeof(MCimagehash *));

	t_success = MCImageCreateIndexedBitmap(p_bitmap->width, p_bitmap->height, t_indexed);

	if (t_success)
	{
		uint8_t *t_src_ptr = (uint8_t*) p_bitmap->data;
		uint8_t *t_dst_ptr = (uint8_t*) t_indexed->data;
		for (uindex_t y = 0; t_success && y < t_indexed->height; y++)
		{
			uint32_t *t_src_row = (uint32_t*)t_src_ptr;
			uint8_t *t_dst_row = t_dst_ptr;
			for (uindex_t x = 0; t_success && x < t_indexed->width; x++)
			{
				uint32_t t_pixel = *t_src_row++;
				uint8_t t_r, t_g, t_b, t_alpha;
				MCGPixelUnpackNative(t_pixel, t_r, t_g, t_b, t_alpha);
				if (t_alpha > 0 && t_alpha < 255)
				{
					t_success = false;
					break;
				}
				bool t_transparent = t_alpha == 0;

				if (t_transparent && p_ignore_transparent)
				{
					// transparency / alpha handled elsewhere, set to 1st color
					*t_dst_row++ = 0;
				}
				else if (t_transparent)
				{
					t_success = MCImageIndexedBitmapAddTransparency(t_indexed);
					if (t_success)
						*t_dst_row++ = t_indexed->transparent_index;
				}
				else
				{
					uint4 i = t_pixel % COLOR_HASH_SIZE;
					MCimagehash *hashentry = hashtable[i];
					while (hashentry != NULL && hashentry->pixel != t_pixel	&&
						hashentry->index != t_indexed->transparent_index)
						hashentry = hashentry->next;
					if (hashentry == NULL)
					{
						if (t_indexed->palette_size == 256)
						{
							// no room in palette for new colour
							t_success = false;
							break;
						}
						t_success = MCMemoryNew(hashentry);
						if (t_success)
						{
							hashentry->pixel = t_pixel;

							uint16_t t_component;
							t_component = t_r;
							t_indexed->palette[t_indexed->palette_size].red = t_component << 8 | t_component;
							t_component = t_g;
							t_indexed->palette[t_indexed->palette_size].green = t_component << 8 | t_component;
							t_component = t_b;
							t_indexed->palette[t_indexed->palette_size].blue = t_component << 8 | t_component;

							hashentry->index = t_indexed->palette_size++;
							hashentry->next = hashtable[i];
							hashtable[i] = hashentry;
						}
					}

					if (t_success)
						*t_dst_row++ = hashentry->index;
				}
			}
			t_src_ptr += p_bitmap->stride;
			t_dst_ptr += t_indexed->stride;
		}
	}

	MCImageFreeHashTable(hashtable);

	if (t_success)
		t_success = MCMemoryResizeArray(t_indexed->palette_size, t_indexed->palette, t_indexed->palette_size);

	if (t_success)
		r_indexed = t_indexed;
	else
		MCImageFreeIndexedBitmap(t_indexed);

	return t_success;
}

bool MCImageConvertIndexedToBitmap(MCImageIndexedBitmap *p_indexed, MCImageBitmap *&r_bitmap)
{
	bool t_success = true;

	MCImageBitmap *t_bitmap = nil;
	uint32_t *t_pixels = nil;

	t_success = MCImageBitmapCreate(p_indexed->width, p_indexed->height, t_bitmap);
	if (t_success)
		t_success = MCMemoryNewArray(p_indexed->palette_size, t_pixels);

	if (t_success)
	{
		for (uindex_t i = 0; i < p_indexed->palette_size; i++)
		{
			t_pixels[i] = MCGPixelPackNative(
									   p_indexed->palette[i].red >> 8,
									   p_indexed->palette[i].green >> 8,
									   p_indexed->palette[i].blue >> 8,
									   255);
		}

		if (MCImageIndexedBitmapHasTransparency(p_indexed))
			t_pixels[p_indexed->transparent_index] = 0x0;

		uint8_t *t_src_ptr = p_indexed->data;
		uint8_t *t_dst_ptr = (uint8_t*)t_bitmap->data;
		for (uindex_t y = 0; y < t_bitmap->height; y++)
		{
			uint8_t *t_src_row = t_src_ptr;
			uint32_t *t_dst_row = (uint32_t*)t_dst_ptr;
			for (uindex_t x = 0; x < t_bitmap->width; x++)
			{
				uindex_t t_color = *t_src_row++;
				if (t_color == p_indexed->transparent_index)
					t_bitmap->has_transparency = true;
				*t_dst_row++ = t_pixels[t_color];
			}
			t_src_ptr += p_indexed->stride;
			t_dst_ptr += t_bitmap->stride;
		}
	}

	MCMemoryDeleteArray(t_pixels);

	if (t_success)
		r_bitmap = t_bitmap;
	else
		MCImageFreeBitmap(t_bitmap);

	return t_success;
}

bool MCImageForceBitmapToIndexed(MCImageBitmap *p_bitmap, bool p_dither, MCImageIndexedBitmap *&r_indexed)
{
	bool t_success = true;

	if (MCImageConvertBitmapToIndexed(p_bitmap, false, r_indexed))
		return true;

	MCColor *t_colors = nil;
	
	t_success = MCMemoryNewArray(255, t_colors);

	if (t_success)
	{
		for (uindex_t i = 0; i < 255; i++)
			MCscreen->getpaletteentry(i, t_colors[i]);

		t_success = MCImageQuantizeImageBitmap(p_bitmap, t_colors, 255, p_dither, true, r_indexed);
	}

	MCMemoryDeleteArray(t_colors);

	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

void surface_extract_mask(void *p_pixels, uint4 p_pixel_stride, void *p_mask, uint4 p_mask_stride, uint4 p_width, uint4 p_height, uint1 p_threshold);
void MCImageBitmapExtractMask(MCImageBitmap *p_bitmap, void *p_mask, uint32_t p_mask_stride, uint8_t p_threshold)
{
	surface_extract_mask(p_bitmap->data, p_bitmap->stride, p_mask, p_mask_stride, p_bitmap->width, p_bitmap->height, p_threshold);
}

////////////////////////////////////////////////////////////////////////////////

bool MCImageDataIsJPEG(MCDataRef p_input)
{
	const byte_t *t_data;
	t_data = MCDataGetBytePtr(p_input);
    
	uint4 t_length;
	t_length = MCDataGetLength(p_input);
    
	if (t_length < 9)
		return false;
    
	if (t_data[0] != 0xFF || t_data[1] != 0xD8 || t_data[2] != 0xFF)
        return false;
    if (t_data[3] != 0xDB && t_data[3] != 0xE0)
        return false;
    
	return true;
}

bool MCImageDataIsPNG(MCDataRef p_input)
{
	const byte_t *t_data;
	t_data = MCDataGetBytePtr(p_input);
	
	uint4 t_length;
	t_length = MCDataGetLength(p_input);
    
	if (t_length < 8)
		return false;
    
	return memcmp(t_data, "\x89PNG\x0d\x0a\x1a\x0a", 8) == 0;
}

bool MCImageDataIsGIF(MCDataRef p_input)
{
    const byte_t *t_data;
	t_data = MCDataGetBytePtr(p_input);
	
	uint4 t_length;
	t_length = MCDataGetLength(p_input);
    
	if (t_length < 6)
		return false;
    
	return memcmp(t_data, "GIF87a", 6) == 0 || memcmp(t_data, "GIF89a", 6) == 0;
}

bool MCImageDataIsBMP(MCDataRef p_input)
{
	const byte_t* t_data = MCDataGetBytePtr(p_input);
	uindex_t t_length = MCDataGetLength(p_input);

	// This only recognises Win32-format BMPs (all other formats are obsolete)
	if (t_length < 2)
		return false;

	return memcmp(t_data, "BM", 2) == 0;
}

////////////////////////////////////////////////////////////////////////////////

MCGRaster MCImageBitmapGetMCGRaster(MCImageBitmap *p_bitmap, bool p_is_premultiplied)
{
	MCGRaster t_raster;
	t_raster.width = p_bitmap->width;
	t_raster.height = p_bitmap->height;
	t_raster.stride = p_bitmap->stride;
	t_raster.pixels = p_bitmap->data;
	t_raster.format = MCImageBitmapHasTransparency(p_bitmap) ? (p_is_premultiplied ? kMCGRasterFormat_ARGB : kMCGRasterFormat_U_ARGB) : kMCGRasterFormat_xRGB;
	
	return t_raster;
}

MCImageBitmap MCImageBitmapFromMCGRaster(const MCGRaster &p_raster)
{
	MCImageBitmap t_bitmap;
	t_bitmap.width = p_raster.width;
	t_bitmap.height = p_raster.height;
	t_bitmap.stride = p_raster.stride;
	t_bitmap.data = (uint32_t*)p_raster.pixels;
	t_bitmap.has_alpha = t_bitmap.has_transparency = p_raster.format != kMCGRasterFormat_xRGB;
	
	return t_bitmap;
}

bool MCImageBitmapCopyAsMCGImage(MCImageBitmap *p_bitmap, bool p_is_premultiplied, MCGImageRef &r_image)
{
	return MCGImageCreateWithRaster(MCImageBitmapGetMCGRaster(p_bitmap, p_is_premultiplied), r_image);
}

bool MCImageBitmapCopyAsMCGImageAndRelease(MCImageBitmap *&x_bitmap, bool p_is_premultiplied, MCGImageRef &r_image)
{
	MCGRaster t_raster;
	t_raster = MCImageBitmapGetMCGRaster(x_bitmap, p_is_premultiplied);

	bool t_success;
	t_success = MCGImageCreateWithRasterAndRelease(t_raster, r_image);

	if (t_success)
	{
		x_bitmap->data = nil;
		MCImageFreeBitmap(x_bitmap);
		x_bitmap = nil;
	}

	return t_success;
}

////////////////////////////////////////////////////////////////////////////////


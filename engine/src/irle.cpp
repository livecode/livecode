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

#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "uidc.h"
#include "image.h"

#include "graphics.h"

////////////////////////////////////////////////////////////////////////////////

void surface_extract_mask(void *p_pixels, uint4 p_pixel_stride, void *p_mask, uint4 p_mask_stride, uint4 p_width, uint4 p_height, uint1 p_threshold);

////////////////////////////////////////////////////////////////////////////////

bool rle_true_encode(const uint4 *sptr, uint4 ssize, uint1 *&dptr, uint4 &dsize)
{
	uint4 osize = ssize + (ssize / 127);
	uint1 *destptr = nil;
	if (!MCMemoryNewArray(osize, destptr))
		return false;
	const uint4 *eptr = sptr + ssize / sizeof(uint4);
	dsize = 0;
	uint2 run;
	do
	{
		run = 1;
		if (sptr < (eptr - 1) && *sptr == *(sptr + 1))
		{
			uint4 pixel = *sptr;
			while (++sptr < eptr && *sptr == pixel && run < 127)
				run++;
			destptr[dsize++] = run | 0x80;
			// pixels encoded as ARGB
			uint8_t t_r, t_g, t_b, t_a;
			MCGPixelUnpackNative(pixel, t_r, t_g, t_b, t_a);
			destptr[dsize++] = t_a;
			destptr[dsize++] = t_r;
			destptr[dsize++] = t_g;
			destptr[dsize++] = t_b;
		}
		else
		{
			while (++sptr < (eptr - 1) && *sptr != *(sptr + 1) && run < 127)
				run++;
			destptr[dsize++] = (uint1)run;
			// move back to start of run
			sptr -= run;
			while (run--)
			{
				// pixels encoded as ARGB
				uint8_t t_r, t_g, t_b, t_a;
				MCGPixelUnpackNative(*sptr++, t_r, t_g, t_b, t_a);
				destptr[dsize++] = t_a;
				destptr[dsize++] = t_r;
				destptr[dsize++] = t_g;
				destptr[dsize++] = t_b;
			}
		}
	}
	while (sptr < eptr);

	if (!MCMemoryReallocate(destptr, dsize, dptr))
	{
		MCMemoryDeleteArray(destptr);
		return false;
	}

	return true;
}

bool rle_run_encode(const uint1 *sptr, uint4 ssize, uint1 *&dptr, uint4 &dsize)
{
	uint4 osize = ssize + (ssize >> 1);
	uint1 *destptr = nil;
	if (!MCMemoryNewArray(osize, destptr))
		return false;

	const uint1 *eptr = sptr + ssize;
	dsize = 0;
	uint2 run;
	do
	{
		run = 1;
		if (*sptr == *(sptr + 1))
		{
			uint1 byte = *sptr;
			while (++sptr < eptr && *sptr == byte && run < 127)
				run++;
			destptr[dsize++] = run | 0x80;
			destptr[dsize++] = byte;
		}
		else
		{
			// MW-2008-02-05: [[ Bug 5232 ]] Crash when importing HC stack
			while (++sptr < eptr && ((sptr == eptr - 1) || *sptr != *(sptr + 1)) && run < 127)
				run++;
			destptr[dsize++] = (uint1)run;
			memcpy(&destptr[dsize], sptr - run, run);
			dsize += run;
		}
	}
	while (sptr < eptr);

	if (!MCMemoryReallocate(destptr, dsize, dptr))
	{
		MCMemoryDeleteArray(destptr);
		return false;
	}

	return true;
}

bool rle_plane_encode(const uint8_t *sptr, uindex_t p_stride, uindex_t p_height, uint1 *&dptr, uint4 &dsize)
{
	uint2 newbpl = (p_stride + 3) & 0xFFFFFFFC;
	if (newbpl == p_stride)
		return rle_run_encode(sptr, p_stride * p_height, dptr, dsize);
	else
	{
		uint1 *data = nil;
		if (!MCMemoryNewArray(newbpl * p_height, data))
			return false;
		uint2 y;
		for (y = 0 ; y < p_height ; y++)
			memcpy(&data[y * newbpl], &sptr[y * p_stride], p_stride);
		bool t_success = rle_run_encode(data, newbpl * p_height, dptr, dsize);
		MCMemoryDeleteArray(data);
		return t_success;
	}
}

//////////

bool rle_true_decode(const uint8_t *sptr, uindex_t ssize, uint32_t *dptr, uindex_t dsize)
{
	const uint1 *eptr = sptr + ssize;
	while (sptr < eptr)
	{
		bool t_repeat = (*sptr & 0x80) != 0;
		uint1 count = *sptr++ & 0x7F;
		uint32_t t_consumed = 4 * (t_repeat ? 1 : count);

		if (count * sizeof(uint32_t) > dsize || eptr - sptr < t_consumed)
			return false;

		dsize -= count * sizeof(uint32_t);
		
		if (t_repeat)
		{
			// pixels encoded as ARGB
			uint32_t t_pixel;
			t_pixel = MCGPixelPackNative(sptr[1], sptr[2], sptr[3], sptr[0]);
			while (count--)
				*dptr++ = t_pixel;
		}
		else
		{
			const uint8_t *t_src_ptr = sptr;
			while (count--)
			{
				*dptr++ = MCGPixelPackNative(t_src_ptr[1], t_src_ptr[2], t_src_ptr[3], t_src_ptr[0]);
				t_src_ptr += 4;
			}
		}

		sptr += t_consumed;
	}

	return true;
}

bool rle_run_decode(const uint8_t *sptr, uindex_t ssize, uint8_t *dptr, uindex_t dsize)
{
	const uint1 *eptr = sptr + ssize;
	while (sptr < eptr)
	{
		bool t_repeat = (*sptr & 0x80) != 0;
		uint1 count = *sptr++ & 0x7F;
		uint8_t t_consumed = t_repeat ? 1 : count;

		if (count > dsize || eptr - sptr < t_consumed)
			return false;

		if (t_repeat)
			memset((char *)dptr, *sptr, count);
		else
			memcpy(dptr, sptr, count);

		sptr += t_consumed;
		dptr += count;
		dsize -= count;
	}

	return true; //dsize == 0;
}

bool rle_plane_decode(uint1 *sptr, uint4 ssize, uindex_t p_stride, uindex_t p_height, uint8_t *p_dptr, uindex_t p_dsize)
{
	if (p_stride * p_height > p_dsize)
		return false;

	uint2 newbpl = (p_stride + 3) & 0xFFFFFFFC;
	if (newbpl == p_stride)
		return rle_run_decode(sptr, ssize, p_dptr, p_dsize);
	else
	{
		uint8_t *t_data = nil;
		uindex_t t_size = newbpl * p_height;
		bool t_success;
		t_success = MCMemoryAllocate(t_size, t_data) && rle_run_decode(sptr, ssize, t_data, t_size);
		if (t_success)
		{
			for (uindex_t y = 0 ; y < p_height ; y++)
				MCMemoryCopy(&p_dptr[y * p_stride], &t_data[y * newbpl], p_stride);
		}
		MCMemoryDeallocate(t_data);
		return t_success;
	}
}

////////////////////////////////////////////////////////////////////////////////

uint32_t MCImagePlaneStride(uint32_t p_width)
{
	// pad plane masks to 4byte alignment to avoid re-alignment by rle_plane_encode
	return (((p_width + 7) / 8) + 3) & ~0x3;
}

// split color indexes into a series of 'mask' planes for each color
bool rle_split_planes(MCImageIndexedBitmap *p_src, uindex_t t_first_plane, uindex_t p_plane_count, uint8_t *&r_planes, uindex_t &r_plane_stride)
{
	r_plane_stride = MCImagePlaneStride(p_src->width);
	uindex_t t_plane_size = r_plane_stride * p_src->height;

	if (!MCMemoryNewArray(t_plane_size * p_plane_count, r_planes))
		return false;

	uint8_t *t_src_ptr = p_src->data;
	uint8_t *t_dst_ptr = r_planes;
	for (uindex_t y = 0; y < p_src->height; y++)
	{
		uint32_t t_bit = 0x80;
		uint8_t *t_src_row = t_src_ptr;
		uint8_t *t_dst_row = t_dst_ptr;
		for (uindex_t x = 0; x < p_src->width; x++)
		{
			uint8_t t_index = *t_src_row++;
			if (t_index >= t_first_plane && t_index < t_first_plane + p_plane_count)
				t_dst_row[(t_index - t_first_plane) * t_plane_size] |= t_bit;

			t_bit >>= 1;
			if (t_bit == 0)
			{
				t_dst_row++;
				t_bit = 0x80;
			}
		}
		t_src_ptr += p_src->stride;
		t_dst_ptr += r_plane_stride;
	}

	return true;
}

static inline uint8_t rle_remap_transparent(uint8_t p_index, uindex_t p_last, uindex_t p_transparent)
{
	if (p_index == p_last)
	{
		if (p_last == p_transparent)
			return 0;
		return p_transparent;
	}
	return p_index;
}

bool MCImageCompressRLE(MCImageIndexedBitmap *p_indexed, MCImageCompressedBitmap *&r_compressed)
{
	bool t_success = true;

	MCImageCompressedBitmap *t_compressed = nil;

	bool t_has_transparency = MCImageIndexedBitmapHasTransparency(p_indexed);

	uindex_t t_colors = p_indexed->palette_size;
	// if one of the colors is transparency, then we store it separately as a mask
	if (t_has_transparency)
		t_colors--;

	t_success = MCImageCreateCompressedBitmap(F_RLE, t_compressed);

	if (t_success)
	{
		t_compressed->width = p_indexed->width;
		t_compressed->height = p_indexed->height;
		t_compressed->depth = 8;

		// IM-2014-09-10: [[ Bug 10703 ]] Special case handling of blank (fully transparent) images
		if (t_has_transparency && t_colors == 0)
		{
			// if the image is completely blank (has no color planes) then we need to create
			// a single blank color plane
			uint8_t *t_plane;
			t_plane = nil;
			
			uint32_t t_plane_stride, t_plane_size;
			t_plane_stride = MCImagePlaneStride(p_indexed->width);
			t_plane_size = t_plane_stride * p_indexed->height;
			
			// create blank mask plane
			if (t_success)
				t_success = MCMemoryNewArray(t_plane_size, t_plane);
			
			if (t_success)
				t_success = MCMemoryNewArray(1, t_compressed->planes) &&
				MCMemoryNewArray(1, t_compressed->plane_sizes);
			
			// encode mask plane
			if (t_success)
				t_success = rle_plane_encode(t_plane, t_plane_stride, p_indexed->height, t_compressed->mask, t_compressed->mask_size);
			
			// copy encoded mask plane to 1st color plane
			if (t_success)
				t_success = MCMemoryAllocateCopy(t_compressed->mask, t_compressed->mask_size, t_compressed->planes[0]);
			
			if (t_success)
			{
				t_compressed->plane_sizes[0] = t_compressed->mask_size;
				t_colors = 1;
			}
			
			MCMemoryDeleteArray(t_plane);
		}
		else if (t_colors <= MAX_PLANES)
		{
			// separate color planes
			uint8_t *t_planes = nil;
			uindex_t t_plane_stride = 0;
			t_success = rle_split_planes(p_indexed, 0, p_indexed->palette_size, t_planes, t_plane_stride);

			uindex_t t_plane_size = t_plane_stride * p_indexed->height;
			if (t_success)
				t_success = MCMemoryNewArray(p_indexed->palette_size, t_compressed->planes) &&
				MCMemoryNewArray(p_indexed->palette_size, t_compressed->plane_sizes);

			if (t_success && t_has_transparency)
			{
				// invert transparency plane so 0 -> transparent, 1 -> opaque
				uint8_t *t_trans_plane = t_planes + t_plane_size * p_indexed->transparent_index;
				for (uindex_t i = 0; i < t_plane_size; i++)
					t_trans_plane[i] = ~t_trans_plane[i];
			}

			uint8_t *t_plane_ptr = t_planes;
			// compress planes
			for (uindex_t i = 0; t_success && i < p_indexed->palette_size; i++)
			{
				t_success = rle_plane_encode(t_plane_ptr, t_plane_stride, p_indexed->height, t_compressed->planes[i], t_compressed->plane_sizes[i]);
				t_plane_ptr += t_plane_size;
			}

			MCMemoryDeleteArray(t_planes);

			// move transparency plane to mask
			if (t_has_transparency)
			{
				t_compressed->mask = t_compressed->planes[p_indexed->transparent_index];
				t_compressed->mask_size = t_compressed->plane_sizes[p_indexed->transparent_index];

				// move last plane to fill gap left by transparency plane
				if (p_indexed->transparent_index != t_colors)
				{
					t_compressed->planes[p_indexed->transparent_index] = t_compressed->planes[t_colors];
					t_compressed->plane_sizes[p_indexed->transparent_index] = t_compressed->plane_sizes[t_colors];
				}
			}
		}
		else
		{
			uint8_t *t_data = nil;
			uindex_t t_data_size = 0;
			uindex_t t_stride;

			if (t_colors <= 16)
				// pack 2 4bit values into bytes
				t_stride = (p_indexed->width + 1) / 2;
			else
				t_stride = p_indexed->width;

			t_data_size = p_indexed->height * t_stride;
			t_success = MCMemoryNewArray(t_data_size, t_data);

			if (t_success)
			{
				uint8_t *t_dst_ptr = t_data;
				uint8_t *t_src_ptr = p_indexed->data;

				for (uindex_t y = 0; y < p_indexed->height; y++)
				{
					uint8_t *t_dst_row = t_dst_ptr;
					uint8_t *t_src_row = t_src_ptr;
					for (uindex_t x = 0; x < p_indexed->width; x++)
					{
						/* if the image has transparency, then we swap the transparency color
						with the last non-transparent color */
						uint8_t t_value = *t_src_row++;
						if (t_has_transparency)
							t_value = rle_remap_transparent(t_value, t_colors, p_indexed->transparent_index);
						*t_dst_row = t_value;

						if (t_colors <= 16 && ++x < p_indexed->width)
						{
							t_value = *t_src_row++;
							if (t_has_transparency)
								t_value = rle_remap_transparent(t_value, t_colors, p_indexed->transparent_index);
							*t_dst_row |= t_value << 4;
						}
						t_dst_row++;
					}
					t_dst_ptr += t_stride;
					t_src_ptr += p_indexed->stride;
				}

				// encode bytes
				t_success = rle_run_encode(t_data, t_data_size, t_compressed->data, t_compressed->size);
			}
			MCMemoryDeleteArray(t_data);

			if (t_has_transparency)
			{
				// extract transparency plane & encode
				uint8_t *t_mask_plane = nil;
				uindex_t t_mask_plane_stride = 0;
				t_success = rle_split_planes(p_indexed, p_indexed->transparent_index, 1, t_mask_plane, t_mask_plane_stride);
				if (t_success)
				{
					// IM-2013-07-23: Invert bits of mask (split plane will produce transparent == 1, opaque == 0 bitmap)
					for (uint32_t i = 0; i < t_mask_plane_stride * p_indexed->height; i++)
						t_mask_plane[i] ^= 0xFF;
					t_success = rle_plane_encode(t_mask_plane, t_mask_plane_stride, p_indexed->height, t_compressed->mask, t_compressed->mask_size);
				}
				MCMemoryDeleteArray(t_mask_plane);
			}
		}
	}

	// IM-2014-09-10: [[ Bug 10703 ]] Copy all palette colors (including transparent color)
	if (t_success)
		t_success = MCMemoryAllocateCopy(p_indexed->palette, sizeof(MCColor) * p_indexed->palette_size, t_compressed->colors);

	if (t_success)
	{
		t_compressed->color_count = t_colors;
		if (t_has_transparency && t_colors != p_indexed->transparent_index)
		{
			// IM-2014-09-10: [[ Bug 10703 ]] swap transparent color with the end color
			MCColor t_color;
			t_color = t_compressed->colors[p_indexed->transparent_index];
			t_compressed->colors[p_indexed->transparent_index] = p_indexed->palette[p_indexed->palette_size - 1];
			t_compressed->colors[p_indexed->palette_size - 1] = t_color;
		}
	}

	if (t_success)
		r_compressed = t_compressed;
	else
		MCImageFreeCompressedBitmap(t_compressed);

	return t_success;
}

bool MCImageCompressRLE(MCImageBitmap *p_bitmap, MCImageCompressedBitmap *&r_compressed)
{
	bool t_success = true;

	MCImageIndexedBitmap *t_indexed = nil;
	MCImageCompressedBitmap *t_compressed = nil;

	if (MCImageConvertBitmapToIndexed(p_bitmap, false, t_indexed))
	{
		t_success = MCImageCompressRLE(t_indexed, r_compressed);
		MCImageFreeIndexedBitmap(t_indexed);
		return t_success;
	}

	t_success = MCImageCreateCompressedBitmap(F_RLE, t_compressed);

	if (t_success)
	{
		t_compressed->width = p_bitmap->width;
		t_compressed->height = p_bitmap->height;

		t_compressed->depth = 32;

		t_success = rle_true_encode(p_bitmap->data, p_bitmap->height * p_bitmap->stride, t_compressed->data, t_compressed->size);
	}

	if (t_success && MCImageBitmapHasTransparency(p_bitmap))
	{
		// align to 4 bytes so rle_plane_encode won't copy everything to a new buffer
		uindex_t t_mask_stride = (((p_bitmap->width + 7) / 8) + 3) & ~0x3;
		uint8_t *t_mask_data = nil;
		t_success = MCMemoryNewArray(t_mask_stride * p_bitmap->height, t_mask_data);
		if (t_success)
		{
			surface_extract_mask(p_bitmap->data, p_bitmap->stride, t_mask_data, t_mask_stride, p_bitmap->width, p_bitmap->height, 0);
			t_success = rle_plane_encode(t_mask_data, t_mask_stride, p_bitmap->height, t_compressed->mask, t_compressed->mask_size);
		}
		MCMemoryDeleteArray(t_mask_data);
	}

	if (t_success)
		r_compressed = t_compressed;
	else
		MCImageFreeCompressedBitmap(t_compressed);

	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

void MCBitmapUnpackRow(uint8_t *p_dst, const uint8_t *p_src, uint32_t p_width, uint32_t p_depth, bool p_msb_first = true);
void surface_merge_with_mask(void *p_pixels, uint4 p_pixel_stride, void *p_mask, uint4 p_mask_stride, uint4 p_offset, uint4 p_width, uint4 p_height);

void MCImageBitmapApplyPlane(MCImageBitmap *p_dst, uint8_t *p_src, uindex_t p_src_stride, uint32_t p_value)
{
	uint8_t *t_src_ptr = p_src;
	uint8_t *t_dst_ptr = (uint8_t*)p_dst->data;

	for (uindex_t y = 0; y < p_dst->height; y++)
	{
		uint8_t *t_src_row = t_src_ptr;
		uint32_t *t_dst_row = (uint32_t*)t_dst_ptr;
		uint8_t t_byte = 0;
		uint8_t t_bit = 0;
		for (uindex_t x = 0; x < p_dst->width; x++)
		{
			if (t_bit == 0)
			{
				t_byte = *t_src_row++;
				t_bit = 0x80;
			}
			if (t_byte & t_bit)
				*t_dst_row = p_value;
			t_dst_row++;
			t_bit >>= 1;
		}
		t_src_ptr += p_src_stride;
		t_dst_ptr += p_dst->stride;
	}
}

bool MCImageDecompressRLE(MCImageCompressedBitmap *p_compressed, MCImageBitmap *&r_bitmap)
{
	bool t_success = true;

	// don't expect more than 256 colors as there's no way in the current engine to generate such images
	MCAssert(p_compressed->color_count <= 256);

	MCImageBitmap *t_bitmap = nil;

	t_success = MCImageBitmapCreate(p_compressed->width, p_compressed->height, t_bitmap);

	if (t_success)
	{
		if (p_compressed->color_count == 0)
			t_success = rle_true_decode(p_compressed->data, p_compressed->size, t_bitmap->data, t_bitmap->stride * t_bitmap->height);
		else if (p_compressed->color_count <= MAX_PLANES)
		{
			uint8_t *t_plane = nil;
			uindex_t t_stride = (((p_compressed->width + 7) / 8) + 3) & ~0x3;
			uindex_t t_plane_size = t_stride * p_compressed->height;

			t_success = MCMemoryAllocate(t_plane_size, t_plane);
			for (uindex_t i = 0; t_success && i < p_compressed->color_count; i++)
			{
				t_success = rle_plane_decode(p_compressed->planes[i], p_compressed->plane_sizes[i], t_stride, p_compressed->height, t_plane, t_plane_size);
				if (t_success)
				{
					uint32_t t_pixel;
					t_pixel = MCGPixelPackNative(
										   p_compressed->colors[i].red >> 8,
										   p_compressed->colors[i].green >> 8,
										   p_compressed->colors[i].blue >> 8,
										   255);

					MCImageBitmapApplyPlane(t_bitmap, t_plane, t_stride, t_pixel);
				}
			}
			MCMemoryDeallocate(t_plane);
		}
		else
		{
			uint8_t *t_indexed = nil;
			t_success = MCMemoryNewArray(p_compressed->width * p_compressed->height, t_indexed);
			if (t_success)
			{
				uindex_t t_stride;
				if (p_compressed->color_count <= 16)
					t_stride = (p_compressed->width + 1) / 2;
				else
					t_stride = p_compressed->width;

				t_success = rle_run_decode(p_compressed->data, p_compressed->size, t_indexed, t_stride * p_compressed->height);
				if (t_success && p_compressed->color_count <= 16)
				{
					uindex_t t_height = p_compressed->height;
					uint8_t *t_src_ptr = t_indexed + t_stride * (t_height - 1);
					uint8_t *t_dst_ptr = t_indexed + p_compressed->width * (t_height - 1);
					while (t_height--)
					{
						MCBitmapUnpackRow(t_dst_ptr, t_src_ptr, p_compressed->width, 4, false);
						t_src_ptr -= t_stride;
						t_dst_ptr -= p_compressed->width;
					}
				}
			}
			if (t_success)
			{
				uint8_t *t_src_ptr = t_indexed;
				uint8_t *t_dst_ptr = (uint8_t*)t_bitmap->data;
				for (uindex_t y = 0; y < p_compressed->height; y++)
				{
					uint32_t *t_dst_row = (uint32_t*)t_dst_ptr;
					for (uindex_t x = 0; x < p_compressed->width; x++)
					{
						uint8_t t_value = *t_src_ptr++;
						uint32_t t_pixel = 0;
						if (t_value < p_compressed->color_count)
						{
							t_pixel = MCGPixelPackNative(
												   p_compressed->colors[t_value].red >> 8,
												   p_compressed->colors[t_value].green >> 8,
												   p_compressed->colors[t_value].blue >> 8,
												   255);
						}
						*t_dst_row++ = t_pixel;
					}
					t_dst_ptr += t_bitmap->stride;
				}
			}
			MCMemoryDeleteArray(t_indexed);
		}
	}

	if (t_success)
	{
		if (p_compressed->mask != nil)
	{
		uint8_t *t_mask = nil;
		uindex_t t_mask_stride = (((p_compressed->width + 7) / 8) + 3) & ~0x3;
		uindex_t t_mask_size = t_mask_stride * p_compressed->height;

		t_success = MCMemoryAllocate(t_mask_size, t_mask);
		if (t_success)
			t_success = rle_plane_decode(p_compressed->mask, p_compressed->mask_size, t_mask_stride, p_compressed->height, t_mask, t_mask_size);
		if (t_success)
			surface_merge_with_mask(t_bitmap->data, t_bitmap->stride, t_mask, t_mask_stride, 0, p_compressed->width, p_compressed->height);

		MCMemoryDeallocate(t_mask);
		
			// update image transparency after merging with mask
			MCImageBitmapCheckTransparency(t_bitmap);
		}
		else
		{
			// If no mask, set all pixels to full opacity
			MCImageBitmapSetAlphaValue(t_bitmap, 0xFF);
		}
	}

	if (t_success)
		r_bitmap = t_bitmap;
	else
		MCImageFreeBitmap(t_bitmap);

	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

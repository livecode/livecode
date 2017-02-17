/* Copyright (C) 2003-2016 LiveCode Ltd.

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
#include "mcio.h"
#include "util.h"
#include "globals.h"

#include "image.h"

#include "imageloader.h"

#include "uidc.h"

#if kMCGPixelFormatNative == kMCGPixelFormatBGRA
#define NATIVE_IMAGE_FORMAT EX_RAW_BGRA
#elif kMCGPixelFormatNative == kMCGPixelFormatRGBA
#define NATIVE_IMAGE_FORMAT EX_RAW_RGBA
#elif kMCGPixelFormatNative == kMCGPixelFormatARGB
#define NATIVE_IMAGE_FORMAT EX_RAW_ARGB
#elif kMCGPixelFormatNative == kMCGPixelFormatABGR
#define NATIVE_IMAGE_FORMAT EX_RAW_ABGR
#endif

////////////////////////////////////////////////////////////////////////////////

static inline uint32_t MCImageDepth(uint32_t p_color_count)
{
	uint32_t t_depth = 1;
	// skip depths not a power of 2
	while (p_color_count > (1 << t_depth))
		t_depth <<= 1;
	return t_depth;
}

////////////////////////////////////////////////////////////////////////////////

static inline uint32_t MCBMPDepth(uint32_t p_color_count)
{
	uint32_t t_depth =  MCImageDepth(p_color_count);
	// skip (unsupported?) 2-bpp depth
	if (t_depth == 2)
		t_depth = 4;
	return t_depth;
}

static inline uint32_t MCBMPStride(uint32_t p_width, uint32_t p_depth)
{
	// rows are padded to a multiple of 4 bytes
	return ((p_width * p_depth + 31) & ~0x1F ) / 8;
}

////////////////////////////////////////////////////////////////////////////////

static void MCBitmapPackRow(uint8_t *p_dst, const uint8_t *p_src, uint32_t p_width, uint32_t p_depth, bool p_msb_first = true)
{
	//uint32_t t_pixel_mask = (1 << p_depth) - 1;
	const uint8_t *t_src_row = p_src;
	uint8_t *t_dst_row = p_dst;

	uint32_t t_first_shift, t_last_shift;
	uint32_t t_current_shift;
	int32_t t_shift_adjust;

	if (p_msb_first)
	{
		t_first_shift = 8 - p_depth;
		t_last_shift = 0;
		t_shift_adjust = -(int32_t)p_depth;
	}
	else
	{
		t_first_shift = 0;
		t_last_shift = 8 - p_depth;
		t_shift_adjust = p_depth;
	}

	t_current_shift = t_first_shift;

	uint8_t t_byte = 0;
	while (p_width--)
	{
		t_byte |= *t_src_row++ << t_current_shift;
		if (t_current_shift == t_last_shift)
		{
			*t_dst_row++ = t_byte;
			t_byte = 0;
			t_current_shift = t_first_shift;
		}
		else
			t_current_shift += t_shift_adjust;
	}
	if (t_current_shift != t_first_shift)
		*t_dst_row = t_byte;
}

void MCBitmapUnpackRow(uint8_t *p_dst, const uint8_t *p_src, uint32_t p_width, uint32_t p_depth, bool p_msb_first = true)
{
	if (p_width == 0)
		return;

	//uint32_t t_pixel_mask = (1 << p_depth) - 1;
	const uint8_t *t_src_row = p_src;
	uint8_t *t_dst_row = p_dst;

	uint8_t t_pixel_mask;
	uint32_t t_first_shift, t_last_shift;
	uint32_t t_current_shift;
	int32_t t_shift_adjust;

	if (!p_msb_first)
	{
		t_first_shift = 8 - p_depth;
		t_last_shift = 0;
		t_shift_adjust = -(int32_t)p_depth;
	}
	else
	{
		t_first_shift = 0;
		t_last_shift = 8 - p_depth;
		t_shift_adjust = p_depth;
	}

	t_dst_row += p_width - 1;
	uindex_t t_pixels_per_byte = 8 / p_depth;
	uindex_t t_src_bytes = (p_width + t_pixels_per_byte - 1) / t_pixels_per_byte;
	uindex_t t_remainder = p_width % t_pixels_per_byte;
	t_src_row += t_src_bytes - 1;

	t_pixel_mask = (1 << p_depth) - 1;
	t_current_shift = t_last_shift;

	uint8_t t_byte = 0;
	if (t_remainder != 0)
	{
		t_byte = *t_src_row--;
		t_current_shift = t_last_shift - t_shift_adjust * t_remainder;
	}

	while (p_width--)
	{
		if (t_current_shift == t_last_shift)
		{
			t_byte = *t_src_row--;
			t_current_shift = t_first_shift;
		}
		else
			t_current_shift += t_shift_adjust;

		*t_dst_row-- = (t_byte >> t_current_shift) & t_pixel_mask;
	}
}

template <Export_format FSrc, Export_format FDst>
static void MCBitmapConvertRow(uint8_t *p_dst, const uint8_t *p_src, uint32_t p_width)
{
	while (p_width--)
	{
		uint8_t a, r, g, b;
		switch (FSrc)
		{
		case EX_RAW_RGBA:
			r = *p_src++;
			g = *p_src++;
			b = *p_src++;
			a = *p_src++;
			break;
		case EX_RAW_BGRA:
			b = *p_src++;
			g = *p_src++;
			r = *p_src++;
			a = *p_src++;
			break;
		case EX_RAW_ARGB:
			a = *p_src++;
			r = *p_src++;
			g = *p_src++;
			b = *p_src++;
			break;
		case EX_RAW_ABGR:
			a = *p_src++;
			b = *p_src++;
			g = *p_src++;
			r = *p_src++;
			break;
		case EX_RAW_RGB:
			r = *p_src++;
			g = *p_src++;
			b = *p_src++;
			a = 0xFF;
			break;
		case EX_RAW_BGR:
			b = *p_src++;
			g = *p_src++;
			r = *p_src++;
			a = 0xFF;
			break;
		case EX_RAW_BGRX:
			b = *p_src++;
			g = *p_src++;
			r = *p_src++;
			a = 0xFF;
			p_src++;
			break;
		case EX_RAW_GRAY:
			r = g = b = *p_src++;
			a = 0xFF;
			break;
		}

		switch (FDst)
		{
		case EX_RAW_RGBA:
			*p_dst++ = r;
			*p_dst++ = g;
			*p_dst++ = b;
			*p_dst++ = a;
			break;
		case EX_RAW_BGRA:
			*p_dst++ = b;
			*p_dst++ = g;
			*p_dst++ = r;
			*p_dst++ = a;
			break;
		case EX_RAW_ARGB:
			*p_dst++ = a;
			*p_dst++ = r;
			*p_dst++ = g;
			*p_dst++ = b;
			break;
		case EX_RAW_ABGR:
			*p_dst++ = a;
			*p_dst++ = b;
			*p_dst++ = g;
			*p_dst++ = r;
			break;
		case EX_RAW_RGB:
			*p_dst++ = r;
			*p_dst++ = g;
			*p_dst++ = b;
			break;
		case EX_RAW_BGR:
			*p_dst++ = b;
			*p_dst++ = g;
			*p_dst++ = r;
			break;
		case EX_RAW_BGRX:
			*p_dst++ = b;
			*p_dst++ = g;
			*p_dst++ = r;
			*p_dst++ = 0;
			break;
		case EX_RAW_GRAY:
			// simple averaging
			*p_dst++ = ((r + g + b) * a) / (0xFF * 3);
			break;
		}
	}
}

template <Export_format FDst>
static void MCBitmapConvertRow(uint8_t *p_dst, const uint32_t *p_src, uint32_t p_width)
{
	MCBitmapConvertRow<NATIVE_IMAGE_FORMAT, FDst>(p_dst, (uint8_t*)p_src, p_width);
}

bool MCImageEncodeBMP(MCImageBitmap *p_bitmap, IO_handle p_stream, uindex_t &r_bytes_written)
{
	bool t_success = true;

	MCImageIndexedBitmap *t_indexed = nil;

	uindex_t t_width = p_bitmap->width;
	uindex_t t_height = p_bitmap->height;

	uint32_t t_depth = 0;
	uint32_t t_color_count = 0;

	if (MCImageConvertBitmapToIndexed(p_bitmap, false, t_indexed))
	{
		t_depth = MCBMPDepth(t_indexed->palette_size);
		t_color_count = t_indexed->palette_size;
	}
	else
		t_depth = 24;

	uint32_t t_stride = MCBMPStride(t_width, t_depth);
	uint32_t t_data_size = t_stride * t_height;
	uint32_t t_header_size = 14 + 40;
	uint32_t t_data_offset = t_header_size + t_color_count * 4;
	uint32_t t_size = t_data_offset + t_data_size;
	uint32_t t_reserved_32 = 0;

	MCswapbytes = !MCswapbytes;
	// write BMP file header
	t_success = (IO_NORMAL == IO_write("BM", 1, 2, p_stream)) &&
		(IO_NORMAL == IO_write_uint4(t_size, p_stream)) &&
		(IO_NORMAL == IO_write_uint4(t_reserved_32, p_stream)) &&
		(IO_NORMAL == IO_write_uint4(t_data_offset, p_stream));
	// write V3 DIB header
	if (t_success)
		t_success = (IO_NORMAL == IO_write_uint4(40, p_stream)) &&
		(IO_NORMAL == IO_write_uint4(t_width, p_stream)) &&
		(IO_NORMAL == IO_write_uint4(t_height, p_stream)) &&
		(IO_NORMAL == IO_write_uint2(1, p_stream)) &&
		(IO_NORMAL == IO_write_uint2(t_depth, p_stream)) &&
		(IO_NORMAL == IO_write_uint4(0, p_stream)) &&
		(IO_NORMAL == IO_write_uint4(t_data_size, p_stream)) &&
		(IO_NORMAL == IO_write_uint4(0, p_stream)) &&
		(IO_NORMAL == IO_write_uint4(0, p_stream)) &&
		(IO_NORMAL == IO_write_uint4(t_color_count, p_stream)) &&
		(IO_NORMAL == IO_write_uint4(0, p_stream));
	MCswapbytes = !MCswapbytes;

	uint8_t *t_row_buffer = nil;

	if (t_success)
		t_success = MCMemoryNewArray(t_stride, t_row_buffer);

	if (t_indexed != nil)
	{
		// write color table
		for (uint32_t i = 0; t_success && i < t_color_count; i++)
		{
			uint8_t t_color[4];
			t_color[0] = t_indexed->palette[i].blue >> 8;
			t_color[1] = t_indexed->palette[i].green >> 8;
			t_color[2] = t_indexed->palette[i].red >> 8;
			t_color[3] = 0;
			t_success = IO_NORMAL == IO_write(t_color, sizeof(uint8_t), 4, p_stream);
		}

		// bmp row order is bottom to top, so point to last row
		uint8_t *t_src_ptr = (uint8_t*)t_indexed->data + (t_height - 1) * t_indexed->stride;
		while (t_success && t_src_ptr >= t_indexed->data)
		{
			MCBitmapPackRow(t_row_buffer, t_src_ptr, t_width, t_depth);
			t_success = IO_NORMAL == IO_write(t_row_buffer, sizeof(uint8_t), t_stride, p_stream);
			t_src_ptr -= t_indexed->stride;
		}
	}
	else
	{
		// bmp row order is bottom to top, so point to last row
		uint8_t *t_src_ptr = (uint8_t*)p_bitmap->data + (t_height - 1) * p_bitmap->stride;
		while (t_success && t_src_ptr >= (uint8_t*)p_bitmap->data)
		{
			MCBitmapConvertRow<EX_RAW_BGR>(t_row_buffer, (uint32_t*)t_src_ptr, t_width);
			t_success = IO_NORMAL == IO_write(t_row_buffer, sizeof(uint8_t), t_stride, p_stream);
			t_src_ptr -= p_bitmap->stride;
		}
	}

	MCImageFreeIndexedBitmap(t_indexed);
	MCMemoryDeleteArray(t_row_buffer);

	if (t_success)
		r_bytes_written = t_size;

	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

#define BMP_FILE_TYPE_BMP ('BM')
#define BMP_FILE_TYPE_OS2_BMP ('BA')
#define BMP_FILE_TYPE_OS2_COLOR_ICON ('CI')
#define BMP_FILE_TYPE_OS2_COLOR_POINTER ('CP')
#define BMP_FILE_TYPE_OS2_ICON ('IC')
#define BMP_FILE_TYPE_OS2_POINTER ('PT')

// IM-2013-08-16: [[ Bugfix 10278 ]] Add support for reading RLE compressed BMP images
#define BMP_COMPRESSION_RGB (0)
#define BMP_COMPRESSION_RLE8 (1)
#define BMP_COMPRESSION_RLE4 (2)
#define BMP_COMPRESSION_BITFIELDS (3)

#define BMP_FILE_HEADER_SIZE (14)

#define BMP_BITMAPCOREHEADER_SIZE (12)
#define BMP_BITMAPINFOHEADER_SIZE (40)
#define BMP_BITMAPV2INFOHEADER_SIZE (52)
#define BMP_BITMAPV3INFOHEADER_SIZE (56)
#define BMP_BITMAPV4HEADER_SIZE (108)
#define BMP_BITMAPV5HEADER_SIZE (124)

typedef struct _mcbitmapheader
{
	uint32_t header_size;
	uint32_t width;
	uint32_t height;
	uint16_t color_planes;
	uint16_t bits_per_pixel;
	uint32_t compression;
	uint32_t image_size;
	uint32_t hres;
	uint32_t vres;
	uint32_t color_count;
	uint32_t important_color_count;
	uint32_t red_mask;
	uint32_t green_mask;
	uint32_t blue_mask;
	uint32_t alpha_mask;
} MCBitmapHeader;

bool bmp_read_file_header(IO_handle p_stream, uindex_t &x_bytes_read, uint16_t &r_file_type, uint32_t &r_file_size, uint16_t &r_reserved_1, uint16_t &r_reserved_2, uint32_t &r_image_offset)
{
	bool t_success = true;

	MCswapbytes = !MCswapbytes;
	t_success = IO_NORMAL == IO_read_uint2(&r_file_type, p_stream) &&
		IO_NORMAL == IO_read_uint4(&r_file_size, p_stream) &&
		IO_NORMAL == IO_read_uint2(&r_reserved_1, p_stream) &&
		IO_NORMAL == IO_read_uint2(&r_reserved_2, p_stream) &&
		IO_NORMAL == IO_read_uint4(&r_image_offset, p_stream);
	MCswapbytes = !MCswapbytes;

	if (t_success)
		x_bytes_read += BMP_FILE_HEADER_SIZE;
	return t_success;
}

bool bmp_read_dib_header(IO_handle p_stream, uindex_t &x_bytes_read, MCBitmapHeader &r_header, bool &r_is_os2)
{
	bool t_success = true;

	MCMemoryClear(&r_header, sizeof(MCBitmapHeader));

	MCswapbytes = !MCswapbytes;
	t_success = IO_NORMAL == IO_read_uint4(&r_header.header_size, p_stream);
	if (t_success)
	{
		switch (r_header.header_size)
		{
		case BMP_BITMAPINFOHEADER_SIZE:
		case BMP_BITMAPV2INFOHEADER_SIZE:
		case BMP_BITMAPV3INFOHEADER_SIZE:
		case BMP_BITMAPV4HEADER_SIZE:
		case BMP_BITMAPV5HEADER_SIZE:
			{
				// Windows BITMAPINFOHEADER
				if (t_success)
					t_success = IO_NORMAL == IO_read_uint4(&r_header.width, p_stream) &&
					IO_NORMAL == IO_read_uint4(&r_header.height, p_stream) &&

					IO_NORMAL == IO_read_uint2(&r_header.color_planes, p_stream) &&
					IO_NORMAL == IO_read_uint2(&r_header.bits_per_pixel, p_stream) &&
					IO_NORMAL == IO_read_uint4(&r_header.compression, p_stream) &&
					IO_NORMAL == IO_read_uint4(&r_header.image_size, p_stream) &&
					IO_NORMAL == IO_read_uint4(&r_header.hres, p_stream) &&
					IO_NORMAL == IO_read_uint4(&r_header.vres, p_stream) &&
					IO_NORMAL == IO_read_uint4(&r_header.color_count, p_stream) &&
					IO_NORMAL == IO_read_uint4(&r_header.important_color_count, p_stream);

				if (r_header.header_size >= BMP_BITMAPV2INFOHEADER_SIZE)
				{
					// Windows BITMAPINFOHEADER + rgb bitfield masks
					if (t_success)
						t_success = IO_NORMAL == IO_read_uint4(&r_header.red_mask, p_stream) &&
						IO_NORMAL == IO_read_uint4(&r_header.green_mask, p_stream) &&
						IO_NORMAL == IO_read_uint4(&r_header.blue_mask, p_stream);
				}

				if (r_header.header_size >= BMP_BITMAPV3INFOHEADER_SIZE)
				{
					// Windows BITMAPV2INFOHEADER + alpha bitfield mask
					if (t_success)
						t_success = IO_NORMAL == IO_read_uint4(&r_header.alpha_mask, p_stream);
				}

				if (r_header.header_size >= BMP_BITMAPV4HEADER_SIZE)
				{
					// Windows BITMAPV3INFOHEADER + color space type & gamma
					/* OVERHAUL - REVISIT - apply colorspace info to decoded bitmap */
					/* UNIMPLEMENTED - skip to end of header */
					if (t_success)
						t_success = IO_NORMAL == MCS_seek_cur(p_stream, r_header.header_size - BMP_BITMAPV3INFOHEADER_SIZE);
				}
			}

			t_success &= r_header.color_planes == 1;
			if (t_success)
				r_is_os2 = false;

			break;

		case BMP_BITMAPCOREHEADER_SIZE:
			{
				// OS/2 BITMAPCOREHEADER
				uint16_t t_width, t_height;
				t_success = IO_NORMAL == IO_read_uint2(&t_width, p_stream) &&
					IO_NORMAL == IO_read_uint2(&t_height, p_stream) &&
					IO_NORMAL == IO_read_uint2(&r_header.color_planes, p_stream) &&
					IO_NORMAL == IO_read_uint2(&r_header.bits_per_pixel, p_stream);
				t_success &= r_header.color_planes == 1;
				if (t_success)
				{
					r_header.width = t_width;
					r_header.height = t_height;
					r_header.compression = BMP_COMPRESSION_RGB;
					r_header.color_count = 0;
					r_header.image_size = t_height * MCBMPStride(t_width, 24);
					r_is_os2 = true;
				}
			}
			break;

		default:
			t_success = false;
			break;
		}
	}
	MCswapbytes = !MCswapbytes;

	if (t_success)
		x_bytes_read += r_header.header_size;

	return t_success;
}

bool bmp_read_color_table(IO_handle p_stream, uindex_t &x_bytes_read, uint32_t p_color_count, bool p_is_os2, uint32_t *&r_color_table)
{
	bool t_success = true;

	uint32_t *t_color_table = nil;
	// color table may be larger than color count in case of invalid image data
	uindex_t t_table_size = 1 << MCBMPDepth(p_color_count);
	t_success = MCMemoryNewArray(t_table_size, t_color_table);

	uint32_t *t_dst_ptr = t_color_table;
	uint8_t t_color[4];
	uindex_t t_color_size = p_is_os2 ? 3 : 4;

	for (uindex_t i = 0; t_success && i < p_color_count; i++)
	{
		uindex_t t_byte_count = t_color_size;
		t_success = IO_NORMAL == MCS_readfixed(t_color, t_byte_count * sizeof(uint8_t), p_stream);
		MCBitmapConvertRow<EX_RAW_RGB, NATIVE_IMAGE_FORMAT>((uint8_t*)t_dst_ptr, t_color, 1);
		*t_dst_ptr++ = MCGPixelPackNative(t_color[2], t_color[1], t_color[0], 255);
	}

	if (t_success)
	{
		r_color_table = t_color_table;
		x_bytes_read += p_color_count * t_color_size;
	}
	else
		MCMemoryDeleteArray(t_color_table);

	return t_success;
}

// IM-2013-08-16: [[ Bugfix 10278 ]] Add support for reading RLE compressed BMP images
bool bmp_read_rle4_image(IO_handle p_stream, uindex_t &x_bytes_read, MCImageBitmap *p_bitmap, uint32_t *p_color_table, uindex_t p_color_count, bool p_topdown)
{
	bool t_success;
	t_success = true;
	
	bool t_end_bitmap;
	t_end_bitmap = false;
	
	uint32_t x, y;
	x = y = 0;
	
	while (t_success && !t_end_bitmap)
	{
		uint8_t t_count, t_value;
		t_success = IO_NORMAL == IO_read_uint1(&t_count, p_stream);
		if (t_success)
			t_success = IO_NORMAL == IO_read_uint1(&t_value, p_stream);
		if (t_success)
		{
			x_bytes_read += 2;
			if (t_count > 0)
			{
				// encoded mode
				uint8_t t_upper, t_lower;
				t_upper = t_value >> 4;
				t_lower = t_value & 0x0F;
				
				for (uint32_t i = 0; i < t_count; i++)
				{
					uint8_t t_index;
					t_index = ((i & 0x1) == 0) ? t_upper : t_lower;
					
					if (t_index < p_color_count && x < p_bitmap->width && y < p_bitmap->height)
						MCImageBitmapSetPixel(p_bitmap, x, p_topdown ? y : (p_bitmap->height - 1 - y), p_color_table[t_index]);
					x++;
				}
			}
			else if (t_value == 0)
			{
				// end-of-line escape
				x = 0;
				y++;
			}
			else if (t_value == 1)
			{
				// end-of-bitmap escape
				t_end_bitmap = true;
			}
			else if (t_value == 2)
			{
				// delta escape
				uint8_t t_dx, t_dy;
				t_success = IO_NORMAL == IO_read_uint1(&t_dx, p_stream);
				if (t_success)
					t_success = IO_NORMAL == IO_read_uint1(&t_dy, p_stream);
				if (t_success)
				{
					x_bytes_read += 2;
					x += t_dx;
					y += t_dy;
				}
			}
			else
			{
				// absolute mode
				uint8_t t_byte, t_upper;
				uint8_t t_lower = 0;
				
				uint8_t t_run_buffer[128];
				uint32_t t_run_bytes;
				t_run_bytes = (t_value + 1) / 2;
				t_run_bytes = (t_run_bytes + 1) & ~0x1;
				
				t_success = IO_NORMAL == MCS_readfixed(t_run_buffer, t_run_bytes, p_stream);
				
				if (t_success)
					x_bytes_read += t_run_bytes;
				
				for (uint32_t i = 0; t_success && i < t_value; i++)
				{
					if ((i & 0x1) == 0)
					{
						t_byte = t_run_buffer[i / 2];
						t_upper = t_byte >> 4;
						t_lower = t_byte & 0x0F;
					}
					
					uint8_t t_index;
					t_index = ((i & 0x1) == 0) ? t_upper : t_lower;
					
					if (t_index < p_color_count && x < p_bitmap->width && y < p_bitmap->height)
						MCImageBitmapSetPixel(p_bitmap, x, p_topdown ? y : (p_bitmap->height - 1 - y), p_color_table[t_index]);
					x++;
				}
			}
		}
	}
	
	return t_success;
}

// IM-2013-08-16: [[ Bugfix 10278 ]] Add support for reading RLE compressed BMP images
bool bmp_read_rle8_image(IO_handle p_stream, uindex_t &x_bytes_read, MCImageBitmap *p_bitmap, uint32_t *p_color_table, uindex_t p_color_count, bool p_topdown)
{
	bool t_success;
	t_success = true;
	
	bool t_end_bitmap;
	t_end_bitmap = false;
	
	uint32_t x, y;
	x = y = 0;
	
	while (t_success && !t_end_bitmap)
	{
		uint8_t t_count, t_value;
		t_success = IO_NORMAL == IO_read_uint1(&t_count, p_stream);
		if (t_success)
			t_success = IO_NORMAL == IO_read_uint1(&t_value, p_stream);
		if (t_success)
		{
			x_bytes_read += 2;
			if (t_count > 0)
			{
				// encoded mode
				for (uint32_t i = 0; i < t_count; i++)
				{
					uint8_t t_index;
					t_index = t_value;
					
					if (t_index < p_color_count && x < p_bitmap->width && y < p_bitmap->height)
						MCImageBitmapSetPixel(p_bitmap, x, p_topdown ? y : (p_bitmap->height - 1 - y), p_color_table[t_index]);
					x++;
				}
			}
			else if (t_value == 0)
			{
				// end-of-line escape
				x = 0;
				y++;
			}
			else if (t_value == 1)
			{
				// end-of-bitmap escape
				t_end_bitmap = true;
			}
			else if (t_value == 2)
			{
				// delta escape
				uint8_t t_dx, t_dy;
				t_success = IO_NORMAL == IO_read_uint1(&t_dx, p_stream);
				if (t_success)
					t_success = IO_NORMAL == IO_read_uint1(&t_dy, p_stream);
				if (t_success)
				{
					x_bytes_read += 2;
					x += t_dx;
					y += t_dy;
				}
			}
			else
			{
				// absolute mode
				uint8_t t_run_buffer[256];
				uint32_t t_run_bytes;
				t_run_bytes = (t_value + 1) & ~0x1;
				
				t_success = IO_NORMAL == MCS_readfixed(t_run_buffer, t_run_bytes, p_stream);
				
				if (t_success)
					x_bytes_read += t_run_bytes;
				
				for (uint32_t i = 0; t_success && i < t_value; i++)
				{
					uint8_t t_index;
					t_index = t_run_buffer[i];
					
					if (t_index < p_color_count && x < p_bitmap->width && y < p_bitmap->height)
						MCImageBitmapSetPixel(p_bitmap, x, p_topdown ? y : (p_bitmap->height - 1 - y), p_color_table[t_index]);
					x++;
				}
			}
		}
	}
	
	return t_success;
}

bool bmp_read_image(IO_handle p_stream, uindex_t &x_bytes_read, MCImageBitmap *p_bitmap, uindex_t p_depth, uint32_t *p_color_table, uindex_t p_color_count, bool p_top_down)
{
	bool t_success = true;

	uindex_t t_src_stride = MCBMPStride(p_bitmap->width, p_depth);
	uint8_t *t_src_buffer = nil;
	uint8_t *t_dst_ptr = (uint8_t*)p_bitmap->data;

	uint32_t t_pixel_mask, t_first_shift;

	if (p_depth <= 8)
	{
		t_pixel_mask = (1 << p_depth) - 1;
		t_first_shift = 8 - p_depth;
	}

	if (!p_top_down)
		t_dst_ptr += (p_bitmap->height - 1) * p_bitmap->stride;

	t_success = MCMemoryNewArray(t_src_stride, t_src_buffer);

	for (uindex_t y = 0; t_success && y < p_bitmap->height; y++)
	{
		t_success = IO_NORMAL == MCS_readfixed(t_src_buffer, t_src_stride, p_stream);
		if (t_success)
		{
			if (p_depth <= 8)
			{
				uint8_t *t_src_row = t_src_buffer;
				uint32_t *t_dst_row = (uint32_t*)t_dst_ptr;
				
				uint32_t t_current_shift = t_first_shift;
				
                for (uint32_t x = 0; x < p_bitmap->width; x++)
				{
					*t_dst_row++ = p_color_table[(*t_src_row >> t_current_shift) & t_pixel_mask];
					if (t_current_shift == 0)
					{
						t_src_row++;
						t_current_shift = t_first_shift;
					}
					else
						t_current_shift -= p_depth;
				}
			}
			else
			{
				if (p_depth == 24)
					MCBitmapConvertRow<EX_RAW_BGR, NATIVE_IMAGE_FORMAT>(t_dst_ptr, t_src_buffer, p_bitmap->width);
				else
					MCBitmapConvertRow<EX_RAW_BGRX, NATIVE_IMAGE_FORMAT>(t_dst_ptr, t_src_buffer, p_bitmap->width);
			}
		}

		if (p_top_down)
			t_dst_ptr += p_bitmap->stride;
		else
			t_dst_ptr -= p_bitmap->stride;
	}

	if (t_success)
		x_bytes_read += t_src_stride * p_bitmap->height;

	MCMemoryDeleteArray (t_src_buffer);

	return t_success;
}

static uint32_t bmp_mask_to_shift(uint32_t t_mask)
{
	uint32_t t_shift = 0;
	while ((t_shift < 32) && (((t_mask >> t_shift) & 1) == 0))
		t_shift++;

	return t_shift;
}

static void bmp_convert_bitfield_row(uint32_t *p_dst, const uint8_t *p_src, uint32_t p_width, uint32_t p_depth, uint32_t p_a_mask, uint32_t p_r_mask, uint32_t p_g_mask, uint32_t p_b_mask)
{
	uint32_t t_a_shift, t_r_shift, t_g_shift, t_b_shift;
	uint32_t t_a_max, t_r_max, t_g_max, t_b_max;
	t_a_shift = bmp_mask_to_shift(p_a_mask);
	t_r_shift = bmp_mask_to_shift(p_r_mask);
	t_g_shift = bmp_mask_to_shift(p_g_mask);
	t_b_shift = bmp_mask_to_shift(p_b_mask);

	t_a_max = p_a_mask >> t_a_shift;
	t_r_max = p_r_mask >> t_r_shift;
	t_g_max = p_g_mask >> t_g_shift;
	t_b_max = p_b_mask >> t_b_shift;

	if (t_a_max == 0)
		t_a_max = 1;
	if (t_r_max == 0)
		t_r_max = 1;
	if (t_g_max == 0)
		t_g_max = 1;
	if (t_b_max == 0)
		t_b_max = 1;

	while (p_width--)
	{
		uint32_t t_src_val = 0;
		for (uint32_t i = p_depth; i > 0; i -= 8)
			t_src_val |= (*p_src++) << (p_depth - i);

		uint8_t a, r, g, b;

		if (p_a_mask == 0)
			a = 0xFF;
		else
		{
			a = (t_src_val & p_a_mask) >> t_a_shift;
			a = (a * 0xFF) / t_a_max;
		}

		r = (t_src_val & p_r_mask) >> t_r_shift;
		g = (t_src_val & p_g_mask) >> t_g_shift;
		b = (t_src_val & p_b_mask) >> t_b_shift;

		r = (r * 0xFF) / t_r_max;
		g = (g * 0xFF) / t_g_max;
		b = (b * 0xFF) / t_b_max;

		*p_dst++ = MCGPixelPackNative(r, g, b, a);
	}
}

bool bmp_read_bitfield_image(IO_handle p_stream, uindex_t &x_bytes_read, MCImageBitmap *p_bitmap, uindex_t p_depth, uint32_t p_a_mask, uint32_t p_r_mask, uint32_t p_g_mask, uint32_t p_b_mask, bool p_top_down)
{
	uindex_t t_src_stride = MCBMPStride(p_bitmap->width, p_depth);
	uint8_t *t_dst_ptr = (uint8_t*)p_bitmap->data;

	if (!p_top_down)
		t_dst_ptr += (p_bitmap->height - 1) * p_bitmap->stride;

    MCAutoPointer<uint8_t[]> t_src_buffer = new (std::nothrow) uint8_t[t_src_stride];
    if (!t_src_buffer)
    {
        return false;
    }

	for (uindex_t y = 0; y < p_bitmap->height; y++)
	{
        if (IO_NORMAL != MCS_readfixed(t_src_buffer.Get(), t_src_stride, p_stream))
        {
            return false;
        }

        bmp_convert_bitfield_row((uint32_t*)t_dst_ptr, t_src_buffer.Get(),
                                 p_bitmap->width, p_depth,
                                 p_a_mask, p_r_mask, p_g_mask, p_b_mask);

		if (p_top_down)
			t_dst_ptr += p_bitmap->stride;
		else
			t_dst_ptr -= p_bitmap->stride;
	}

    x_bytes_read += t_src_stride * p_bitmap->height;

    return true;
}

bool bmp_read_rgb_bitfields(IO_handle p_stream, uindex_t &x_bytes_read, uint32_t &r_r_mask, uint32_t &r_g_mask, uint32_t &r_b_mask)
{
	bool t_success = true;

	MCswapbytes = !MCswapbytes;

	t_success = IO_NORMAL == IO_read_uint4(&r_r_mask, p_stream) &&
		IO_NORMAL == IO_read_uint4(&r_g_mask, p_stream) &&
		IO_NORMAL == IO_read_uint4(&r_b_mask, p_stream);

	MCswapbytes = !MCswapbytes;

	if (t_success)
		x_bytes_read += 3 * sizeof(uint32_t);

	return t_success;
}

class MCBitmapStructImageLoader : public MCImageLoader
{
public:
	MCBitmapStructImageLoader(IO_handle p_stream) : MCImageLoader(p_stream) {}
	
	virtual MCImageLoaderFormat GetFormat() { return kMCImageFormatBMP; }

protected:
	virtual bool LoadHeader(uint32_t &r_width, uint32_t &r_height, uint32_t &r_xhot, uint32_t &r_yhot, MCStringRef &r_name, uint32_t &r_frame_count, MCImageMetadata &r_metadata);
	virtual bool LoadFrames(MCBitmapFrame *&r_frames, uint32_t &r_count);
	
private:
	MCBitmapHeader m_header;
	bool m_is_topdown;
	bool m_is_os2;
};

bool MCBitmapStructImageLoader::LoadHeader(uint32_t &r_width, uint32_t &r_height, uint32_t &r_xhot, uint32_t &r_yhot, MCStringRef &r_name, uint32_t &r_frame_count, MCImageMetadata &r_metadata)
{
	bool t_success;
	t_success = true;
	
	IO_handle t_stream;
	t_stream = GetStream();
	
	m_is_topdown = false;
	m_is_os2 = false;
	
	uint32_t t_bytes_read;
	t_bytes_read = 0;
	
	if (t_success)
		t_success = bmp_read_dib_header(t_stream, t_bytes_read, m_header, m_is_os2);

	// IM-2013-08-16: [[ Bugfix 10278 ]] Add support for reading RLE compressed BMP images
	if (t_success)
		t_success = m_header.compression == BMP_COMPRESSION_RGB ||
		m_header.compression == BMP_COMPRESSION_RLE8 ||
		m_header.compression == BMP_COMPRESSION_RLE4 ||
		m_header.compression == BMP_COMPRESSION_BITFIELDS;
	
	if (t_success)
	{
		if (m_header.compression == BMP_COMPRESSION_BITFIELDS)
		{
			if (m_header.header_size == BMP_BITMAPINFOHEADER_SIZE)
				t_success = bmp_read_rgb_bitfields(t_stream, t_bytes_read, m_header.red_mask, m_header.green_mask, m_header.blue_mask);
		}
		else if (m_header.compression == BMP_COMPRESSION_RGB)
		{
			if (m_header.bits_per_pixel == 16)
			{
				m_header.compression = BMP_COMPRESSION_BITFIELDS;
				m_header.red_mask = 0x1F << 10;
				m_header.green_mask = 0x1F << 5;
				m_header.blue_mask = 0x1F;
			}
		}
	}

	if (t_success)
	{
        // IM-2012-04-30 - [[bz 10193]] bitmap images may have negative height indicating rows read from
        // the top down, rather than the bottom up.
        if (!m_is_os2 && ((int32_t)m_header.height) < 0)
        {
			m_header.height = -((int32_t)m_header.height);
			m_is_topdown = true;
        }
	}
	
	if (t_success)
	{
		r_width = m_header.width;
		r_height = m_header.height;
		
		r_xhot = r_yhot = 0;
		
		r_name = MCValueRetain(kMCEmptyString);
		r_frame_count = 1;
	}
	
	return t_success;
}

bool MCBitmapStructImageLoader::LoadFrames(MCBitmapFrame *&r_frames, uint32_t &r_count)
{
	bool t_success;
	t_success = true;

	IO_handle t_stream;
	t_stream = GetStream();

	uint32_t t_bytes_read;
	t_bytes_read = 0;

	MCBitmapFrame *t_frame;
	t_frame = nil;

	uint32_t *t_color_table;
	t_color_table = nil;

	if (t_success)
	{
		if (m_header.bits_per_pixel <= 8)
		{
			if (m_header.color_count == 0)
				m_header.color_count = 1 << m_header.bits_per_pixel;
			
			t_success = bmp_read_color_table(t_stream, t_bytes_read, m_header.color_count, m_is_os2, t_color_table);
		}
	}

	uint32_t t_stride = 0;
	t_stride = MCBMPStride(m_header.width, m_header.bits_per_pixel);

	if (t_success)
		t_success = MCMemoryNew(t_frame);

	if (t_success)
		t_success = MCImageBitmapCreate(m_header.width, m_header.height, t_frame->image);

	if (t_success)
	{
		switch (m_header.compression)
		{
			case BMP_COMPRESSION_RGB:
				t_success = bmp_read_image(t_stream, t_bytes_read, t_frame->image, m_header.bits_per_pixel, t_color_table, m_header.color_count, m_is_topdown);
				break;
				
				// IM-2013-08-16: [[ Bugfix 10278 ]] Add support for reading RLE compressed BMP images
			case BMP_COMPRESSION_RLE8:
				t_success = bmp_read_rle8_image(t_stream, t_bytes_read, t_frame->image, t_color_table, m_header.color_count, m_is_topdown);
				break;
				
				// IM-2013-08-16: [[ Bugfix 10278 ]] Add support for reading RLE compressed BMP images
			case BMP_COMPRESSION_RLE4:
				t_success = bmp_read_rle4_image(t_stream, t_bytes_read, t_frame->image, t_color_table, m_header.color_count, m_is_topdown);
				break;
				
			case BMP_COMPRESSION_BITFIELDS:
				t_success = bmp_read_bitfield_image(t_stream, t_bytes_read, t_frame->image, m_header.bits_per_pixel, m_header.alpha_mask, m_header.red_mask, m_header.green_mask, m_header.blue_mask, m_is_topdown);
				break;
		}
	}

	if (t_success)
	{
		if (m_header.compression == BMP_COMPRESSION_BITFIELDS && m_header.alpha_mask != 0)
			MCImageBitmapCheckTransparency(t_frame->image);
		
		r_frames = t_frame;
		r_count = 1;
	}
	else
		MCImageFreeFrames(t_frame, 1);

	MCMemoryDeleteArray(t_color_table);

	return t_success;
}

bool MCImageDecodeBMPStruct(IO_handle p_stream, uindex_t &x_bytes_read, MCImageBitmap *&r_bitmap)
{
	bool t_success;
	t_success = true;

	MCBitmapStructImageLoader *t_loader;
    t_loader = NULL;
	if (t_success)
		t_success = nil != (t_loader = new (nothrow) MCBitmapStructImageLoader(p_stream));

	MCBitmapFrame *t_frames;
	t_frames = nil;

	uint32_t t_count;
	t_count = 0;

	if (t_success)
		t_success = t_loader->TakeFrames(t_frames, t_count);

	if (t_success)
	{
		r_bitmap = t_frames->image;
		t_frames->image = nil;
	}

	if (t_loader != nil)
		delete t_loader;

	MCImageFreeFrames(t_frames, t_count);

	return t_success;
}

class MCBitmapImageLoader : public MCBitmapStructImageLoader
{
public:
	MCBitmapImageLoader(IO_handle p_stream) : MCBitmapStructImageLoader(p_stream) {} ;

	virtual MCImageLoaderFormat GetFormat() { return kMCImageFormatBMP; }

protected:
	virtual bool LoadHeader(uint32_t &r_width, uint32_t &r_height, uint32_t &r_xhot, uint32_t &r_yhot, MCStringRef &r_name, uint32_t &r_frame_count, MCImageMetadata &r_metadata);

private:
};

bool MCBitmapImageLoader::LoadHeader(uint32_t &r_width, uint32_t &r_height, uint32_t &r_xhot, uint32_t &r_yhot, MCStringRef &r_name, uint32_t &r_frame_count, MCImageMetadata &r_metadata)
{
	bool t_success;
	t_success = true;

	uint32_t t_bytes_read;
	t_bytes_read = 0;

	uint32_t t_file_size, t_image_offset;
	uint16_t t_file_type, t_reserved_1, t_reserved_2;

	if (t_success)
		t_success = bmp_read_file_header(GetStream(), t_bytes_read, t_file_type, t_file_size, t_reserved_1, t_reserved_2, t_image_offset);

	if (t_success)
		t_success = MCBitmapStructImageLoader::LoadHeader(r_width, r_height, r_xhot, r_yhot, r_name, r_frame_count, r_metadata);

	if (t_success)
	{
		r_xhot = t_reserved_1;
		r_yhot = t_reserved_2;
	}

	return t_success;
}

bool MCImageLoaderCreateForBMPStream(IO_handle p_stream, MCImageLoader *&r_loader)
{
	MCBitmapImageLoader *t_loader;
	t_loader = new (nothrow) MCBitmapImageLoader(p_stream);
	
	if (t_loader == nil)
		return false;
	
	r_loader = t_loader;
	
	return true;
}

////////////////////////////////////////////////////////////////////////////////

#define PBM_MAX_LINE 256

bool MCImageEncodePPM(MCImageBitmap *p_bitmap, IO_handle p_stream, uindex_t &r_bytes_written)
{
	bool t_success = true;

	uindex_t t_byte_count = 0;
	char *t_header = nil;
	uint32_t t_header_size;
	uint8_t *t_row_buffer = nil;

	t_success = MCCStringFormat(t_header, "P6\n%d %d\n255\n", p_bitmap->width, p_bitmap->height);

	if (t_success)
	{
		t_header_size = MCCStringLength(t_header);
		t_success = IO_NORMAL == IO_write(t_header, sizeof(char), t_header_size, p_stream);
	}

	uint32_t t_stride = p_bitmap->width * 3;

	if (t_success)
	{
		t_byte_count += t_header_size;
		t_success = MCMemoryNewArray(t_stride, t_row_buffer);
	}

	uint8_t *t_src_ptr = (uint8_t*)p_bitmap->data;
	for (uindex_t y = 0; t_success && y < p_bitmap->height; y++)
	{
		MCBitmapConvertRow<EX_RAW_RGB>(t_row_buffer, (uint32_t*)t_src_ptr, p_bitmap->width);
		t_success = IO_NORMAL == IO_write(t_row_buffer, sizeof(uint8_t), t_stride, p_stream);
		t_src_ptr += p_bitmap->stride;
		t_byte_count += t_stride;
	}

	MCMemoryDeleteArray(t_row_buffer);

	if (t_success)
		r_bytes_written = t_byte_count;

	return t_success;
}

void surface_extract_mask(void *p_pixels, uint4 p_pixel_stride, void *p_mask, uint4 p_mask_stride, uint4 p_width, uint4 p_height, uint1 p_threshold);

// creates a 1 bpp mask image - 0 == transparent, 1 == opaque
bool MCImageEncodePBM(MCImageBitmap *p_bitmap, IO_handle p_stream, uindex_t &r_bytes_written)
{
	bool t_success = true;

	uindex_t t_byte_count = 0;
	char *t_header = nil;
	uint32_t t_header_size;
	uint8_t *t_row_buffer = nil;

	t_success = MCCStringFormat(t_header, "P4\n%d %d\n", p_bitmap->width, p_bitmap->height);

	if (t_success)
	{
		t_header_size = MCCStringLength(t_header);
		t_success = IO_NORMAL == IO_write(t_header, sizeof(char), t_header_size, p_stream);
	}

	uint32_t t_stride = (p_bitmap->width + 7) / 8;

	if (t_success)
	{
		t_byte_count += t_header_size;
		t_success = MCMemoryNewArray(t_stride, t_row_buffer);
	}

	uint8_t *t_src_ptr = (uint8_t*)p_bitmap->data;
	for (uindex_t y = 0; t_success && y < p_bitmap->height; y++)
	{
		surface_extract_mask(t_src_ptr, p_bitmap->stride, t_row_buffer, t_stride, p_bitmap->width, 1, 0);
		t_success = IO_NORMAL == IO_write(t_row_buffer, sizeof(uint8_t), t_stride, p_stream);
		t_src_ptr += p_bitmap->stride;
		t_byte_count += t_stride;
	}

	MCMemoryDeleteArray(t_row_buffer);

	if (t_success)
		r_bytes_written = t_byte_count;

	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

class MCNetPBMTokenReader
{
public:
	MCNetPBMTokenReader(IO_handle p_stream)
	{
		m_start = m_end = 0;
		m_size = 0;
		m_buffer = nil;
		m_stream = p_stream;
	}
	
	~MCNetPBMTokenReader()
	{
		MCMemoryDeleteArray(m_buffer);
	}

	bool Allocate(uindex_t p_size)
	{
		if (!MCMemoryNewArray(p_size, m_buffer))
			return false;

		m_size = p_size;
		return true;
	}

	bool GetToken(const uint8_t *&r_token, uindex_t &r_token_size)
	{
		SkipWhitespace();

		uindex_t t_token_size = 0;
		bool t_have_token = false;
		while (!t_have_token)
		{
			if (!Ensure(t_token_size + 1))
				t_have_token = true;
			else
			{
				if (m_buffer[m_start + t_token_size] == '#')
					RemoveComment(t_token_size);

				if (!Ensure(t_token_size + 1) || IsWhitespace(m_buffer[m_start + t_token_size]))
					t_have_token = true;
				else
					t_token_size++;
			}
		}

		if (t_token_size == 0)
			return false;

		r_token = m_buffer + m_start;
		r_token_size = t_token_size;

		m_start += t_token_size;

		return true;
	}

	bool Read(uint8_t *t_buffer, uindex_t p_count)
	{
		while (p_count > 0)
		{
			if (m_start < m_end)
			{
				uindex_t t_count = MCMin(p_count, m_end - m_start);
				MCMemoryCopy(t_buffer, m_buffer + m_start, t_count);
				m_start += t_count;
				t_buffer += t_count;
				p_count -= t_count;
			}
			if (p_count > 0 && !Ensure(MCMin(p_count, m_size)))
				return false;
		}

		return true;
	}

private:
	bool Ensure(uindex_t p_count)
	{
		if (m_buffer == nil && !Allocate(PBM_MAX_LINE))
			return false;

		if (p_count <= (m_end - m_start))
			return true;

		if (MCS_eof(m_stream))
			return false;

		p_count -= (m_end - m_start);
		if ((m_size - m_end) < p_count)
		{
			if ((m_size - (m_end - m_start)) < p_count) // can't fit in buffer
				return false;
			MCMemoryMove(m_buffer, m_buffer + m_start, m_end - m_start);
			m_end -= m_start;
			m_start = 0;
		}

		if (IO_NORMAL != MCS_readfixed(m_buffer + m_end, p_count, m_stream))
			return false;

		m_end += p_count;

		return true;
	}

	bool IsWhitespace(char p_char)
	{
		return p_char == ' ' || p_char == '\t' || p_char == '\r' || p_char == '\n';
	}

	// move ahead to the next non-whitespace char
	void SkipWhitespace()
	{
		while (Ensure(1) && IsWhitespace(m_buffer[m_start]))
			m_start++;
	}

	// keep chars before offset but skip all chars until next new line
	void RemoveComment(uindex_t p_offset)
	{
		// IM-2014-01-16: [[ Bug 11675 ]] Rework to avoid infinite loop when skipping comment
		bool t_new_line;
		t_new_line = false;
		
		uindex_t t_start;
		t_start = m_start + p_offset;
		
		while (!t_new_line)
		{
			uindex_t t_index;
			t_index = 0;
			
			while (!t_new_line && t_start + t_index < m_end)
			{
				t_new_line = m_buffer[t_start + t_index] == '\n' || m_buffer[t_start + t_index] == '\r';
				t_index++;
			}

			MCMemoryMove(m_buffer + t_start, m_buffer + t_start + t_index, t_index);

			m_end -= t_index;

			// fetch next char or end comment at eof
			if (!t_new_line)
				t_new_line = !Ensure(p_offset + 1);
		}
	}

	IO_handle m_stream;
	uint8_t *m_buffer;
	uindex_t m_size;
	uindex_t m_start;
	uindex_t m_end;
};

void netpbm_scale_to_byte(uint8_t *p_buffer, uint32_t p_max_value, uindex_t p_width)
{
	uindex_t t_bytes_per_value;
	t_bytes_per_value = p_max_value < 256 ? 1 : 2;

	// 1-bit bitmaps - 0 == white, 1 == black.
	// All others - 0 == black, max-value == white
	
	// IM-2014-01-23: [[ Bug 11698 ]] Invert color order for 1-bit bitmaps.
	bool t_invert_value;
	t_invert_value = p_max_value == 1;
	
	uint8_t *t_src_row = p_buffer;
	uint8_t *t_dst_row = p_buffer;

	for (uindex_t x = 0; x < p_width; x++)
	{
		uint32_t t_value = 0;
		if (t_bytes_per_value == 2)
			t_value = *t_src_row++ << 8;
		t_value |= *t_src_row++;
		
		if (t_invert_value)
			t_value = p_max_value - t_value;

		*t_dst_row++ = (t_value * 255) / p_max_value;
	}
}

class MCNetPBMImageLoader : public MCImageLoader
{
public:
	MCNetPBMImageLoader(IO_handle p_stream);
	virtual ~MCNetPBMImageLoader();
	
	virtual MCImageLoaderFormat GetFormat() { return kMCImageFormatNetPBM; }
	
protected:
	virtual bool LoadHeader(uint32_t &r_width, uint32_t &r_height, uint32_t &r_xhot, uint32_t &r_yhot, MCStringRef &r_name, uint32_t &r_frame_count, MCImageMetadata &r_metadata);
	virtual bool LoadFrames(MCBitmapFrame *&r_frames, uint32_t &r_count);
	
private:
	MCNetPBMTokenReader *m_reader;
	uint8_t m_format;
	uint32_t m_max_value;
};

MCNetPBMImageLoader::MCNetPBMImageLoader(IO_handle p_stream) : MCImageLoader(p_stream)
{
	m_reader = nil;
	m_max_value = 1;
}

MCNetPBMImageLoader::~MCNetPBMImageLoader()
{
	if (m_reader != nil)
		delete m_reader;
}

bool MCNetPBMImageLoader::LoadHeader(uint32_t &r_width, uint32_t &r_height, uint32_t &r_xhot, uint32_t &r_yhot, MCStringRef &r_name, uint32_t &r_frame_count, MCImageMetadata &r_metadata)
{
	bool t_success = true;

	uint32_t t_width, t_height;
	const uint8_t *t_token;
	uindex_t t_token_size;

	if (t_success)
		t_success = nil != (m_reader = new (nothrow) MCNetPBMTokenReader(GetStream()));

	if (t_success)
		t_success = m_reader->GetToken(t_token, t_token_size);

	if (t_success)
		t_success = t_token_size == 2 &&
		t_token[0] == 'P' && t_token[1] >= '1' && t_token[1] <= '6';

	if (t_success)
	{
		m_format = t_token[1] - '0';

		t_success = m_reader->GetToken(t_token, t_token_size) &&
			MCU_stoui4(MCString((const char*)t_token, t_token_size), t_width);
	}

	if (t_success)
		t_success = m_reader->GetToken(t_token, t_token_size) &&
		MCU_stoui4(MCString((const char*)t_token, t_token_size), t_height);

	if (t_success && m_format != 1 && m_format != 4)
		t_success = m_reader->GetToken(t_token, t_token_size) &&
		MCU_stoui4(MCString((const char*)t_token, t_token_size), m_max_value);

	if (t_success)
		t_success = t_width > 0 && t_height > 0 && m_max_value < 65536;

	if (t_success)
	{
		r_width = t_width;
		r_height = t_height;
		
		r_xhot = r_yhot = 0;
		r_name = MCValueRetain(kMCEmptyString);
		r_frame_count = 1;
	}

	return t_success;
}

bool MCNetPBMImageLoader::LoadFrames(MCBitmapFrame *&r_frames, uint32_t &r_count)
{
	bool t_success = true;
	
	MCBitmapFrame *t_frame = nil;
	
	uint8_t *t_row_buffer = nil;
	uint8_t *t_unpacked_buffer = nil;
	
	const uint8_t *t_token;
	uindex_t t_token_size;
	
	bool t_binary;
	uindex_t t_depth = 1;
	uindex_t t_channel_count = 1;
	uindex_t t_stride;

	uint32_t t_width, t_height;
	
	if (t_success)
		t_success = GetGeometry(t_width, t_height);
	
	if (t_success)
		t_success = MCMemoryNew(t_frame);
	
	if (t_success)
		t_success = MCImageBitmapCreate(t_width, t_height, t_frame->image);
	
	if (t_success)
	{
		t_binary = m_format > 3;

		switch (m_format)
		{
		case 1: // monochrome ascii
			t_depth = 8;
			break;

		case 4: // monochrome binary
			t_depth = 1;
			break;

		case 2: // gray ascii
		case 5: // gray binary
			t_depth = m_max_value < 256 ? 8 : 16;
			break;

		case 3: // rgb ascii
		case 6: // rgb binary
			t_depth = m_max_value < 256 ? 8 : 16;
			t_channel_count = 3;
			break;
		}

		t_stride = (t_width * t_depth * t_channel_count + 7) / 8;

		t_success = MCMemoryNewArray(t_stride, t_row_buffer);
	}

	uint8_t *t_src_ptr = t_row_buffer;
	uint8_t *t_dst_ptr;
	if (t_success)
		t_dst_ptr = (uint8_t*)t_frame->image->data;

	if (t_success && m_format == 4)
	{
		t_success = MCMemoryNewArray(t_width, t_unpacked_buffer);
		t_src_ptr = t_unpacked_buffer;
	}

	// there should be a single whitespace char before the start of the binary data
	if (t_success && t_binary)
		t_success = m_reader->Read(t_row_buffer, 1);

	for (uindex_t y = 0; t_success && y < t_height; y++)
	{
		if (t_binary)
		{
			t_success = m_reader->Read(t_row_buffer, t_stride);

			if (t_success && m_format == 4)
				MCBitmapUnpackRow(t_unpacked_buffer, t_row_buffer, t_width, 1);
		}
		else
		{
			uint32_t t_value;

			uint8_t *t_buffer_ptr = t_row_buffer;
			for (uindex_t x = 0; t_success && x < t_width * t_channel_count; x++)
			{
				t_success = m_reader->GetToken(t_token, t_token_size) &&
				MCU_stoui4(MCString((const char*)t_token, t_token_size), t_value);

				if (t_success)
					t_success = t_value <= m_max_value;

				if (t_success)
				{
					if (t_depth == 16)
						*t_buffer_ptr++ = t_value >> 8;
					*t_buffer_ptr++ = t_value & 0xFF;
				}
			}
		}

		if (t_success)
		{
			netpbm_scale_to_byte(t_src_ptr, m_max_value, t_width * t_channel_count);
			if (t_channel_count == 1)
				MCBitmapConvertRow<EX_RAW_GRAY, NATIVE_IMAGE_FORMAT>(t_dst_ptr, t_src_ptr, t_width);
			else
				MCBitmapConvertRow<EX_RAW_RGB, NATIVE_IMAGE_FORMAT>(t_dst_ptr, t_src_ptr, t_width);

			t_dst_ptr += t_frame->image->stride;
		}
	}

	MCMemoryDeleteArray(t_row_buffer);
	MCMemoryDeleteArray(t_unpacked_buffer);

	if (t_success)
	{
		r_frames = t_frame;
		r_count = 1;
	}
	else
		MCImageFreeFrames(t_frame, 1);

	return t_success;
}

bool MCImageLoaderCreateForNetPBMStream(IO_handle p_stream, MCImageLoader *&r_loader)
{
	MCNetPBMImageLoader *t_loader;
	t_loader = new (nothrow) MCNetPBMImageLoader(p_stream);
	
	if (t_loader == nil)
		return false;
	
	r_loader = t_loader;
	
	return true;
}

bool MCImageDecodeNetPBM(IO_handle p_stream, MCImageBitmap *&r_bitmap)
{
	bool t_success;
	t_success = true;
	
	MCImageLoader *t_loader;
	t_loader = nil;
	if (t_success)
		t_success = MCImageLoaderCreateForNetPBMStream(p_stream, t_loader);
	
	MCBitmapFrame *t_frames;
	t_frames = nil;
	
	uint32_t t_count;
	t_count = 0;
	
	if (t_success)
		t_success = t_loader->TakeFrames(t_frames, t_count);
	
	if (t_success)
	{
		r_bitmap = t_frames->image;
		t_frames->image = nil;
	}
	
	if (t_loader != nil)
		delete t_loader;
	
	MCImageFreeFrames(t_frames, t_count);
	
	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

typedef enum
{
	// XBM
	kMCDefineUnknown,
	kMCDefineWidth,
	kMCDefineHeight,
	kMCDefineXHot,
	kMCDefineYHot,

	// XPM
	kMCDefineFormat,
	kMCDefineNColors,
	kMCDefineCharsPerPixel,
} c_bitmap_define;

struct c_bitmap_key_define
{
	const char *key;
	c_bitmap_define define;
};

static c_bitmap_key_define s_c_bitmap_defines[] = {
	{"_width", kMCDefineWidth},
	{"_height", kMCDefineHeight},
	{"_x_hot", kMCDefineXHot},
	{"_y_hot", kMCDefineYHot},

	{"_format", kMCDefineFormat},
	{"_ncolors", kMCDefineNColors},
	{"_chars_per_pixel", kMCDefineCharsPerPixel},

	{nil, kMCDefineUnknown}
};

bool c_bitmap_split_define(const char *p_line, char *&r_name, c_bitmap_define &r_key, int32_t &r_value)
{
	bool t_success = true;

	char *t_name = nil;
	c_bitmap_define t_define = kMCDefineUnknown;
	const char *t_key = nil;
	int32_t t_value = 0;

	if (!MCCStringBeginsWith(p_line, "#define "))
		return false;

	const char *t_name_ptr = p_line + 8; // skip over #define
	while (*t_name_ptr == ' ')
		t_name_ptr++;

	uindex_t t_value_index = 0;
	if (!MCCStringFirstIndexOf(t_name_ptr, ' ', t_value_index))
		return false;

	t_success = MCCStringCloneSubstring(t_name_ptr, t_value_index, t_name);

	if (t_success)
	{
		while (t_name_ptr[t_value_index] == ' ')
			t_value_index++;

		t_success = True == MCU_stoi4(MCString(t_name_ptr + t_value_index), t_value);
	}

	if (t_success)
	{
		c_bitmap_key_define *t_defines = s_c_bitmap_defines;
		while (t_define == kMCDefineUnknown && t_defines->key != nil)
		{
			if (MCCStringEndsWith(t_name, t_defines->key))
			{
				t_define = t_defines->define;
				t_key = t_defines->key;
			}
			t_defines++;
		}
	}

	if (t_success)
	{
		if (t_define != kMCDefineUnknown)
			t_name[MCCStringLength(t_name) - MCCStringLength(t_key)] = '\0';

		r_name = t_name;
		r_key = t_define;
		r_value = t_value;
	}
	else
		MCCStringFree(t_name);

	return t_success;
}

bool c_get_string_content_bounds(const char *p_line, uindex_t &r_content_start, uindex_t &r_content_end)
{
	if (!MCCStringFirstIndexOf(p_line, "\"", r_content_start) ||
		!MCCStringLastIndexOf(p_line + r_content_start + 1, "\"", r_content_end))
		return false;

	r_content_start += 1;
	r_content_end += r_content_start;
	return true;
}

#define XBM_MAX_LINE 128

class MCXBMImageLoader : public MCImageLoader
{
public:
	MCXBMImageLoader(IO_handle p_stream);
	virtual ~MCXBMImageLoader();
	
	virtual MCImageLoaderFormat GetFormat() { return kMCImageFormatXBM; }
	
protected:
	virtual bool LoadHeader(uint32_t &r_width, uint32_t &r_height, uint32_t &r_xhot, uint32_t &r_yhot, MCStringRef &r_name, uint32_t &r_frame_count, MCImageMetadata &r_metadata);
	virtual bool LoadFrames(MCBitmapFrame *&r_frames, uint32_t &r_count);
	
private:
	char m_line[XBM_MAX_LINE];
};

MCXBMImageLoader::MCXBMImageLoader(IO_handle p_stream) : MCImageLoader(p_stream)
{
}

MCXBMImageLoader::~MCXBMImageLoader()
{
}

bool MCXBMImageLoader::LoadHeader(uint32_t &r_width, uint32_t &r_height, uint32_t &r_xhot, uint32_t &r_yhot, MCStringRef &r_name, uint32_t &r_frame_count, MCImageMetadata &r_metadata)
{
	bool t_success = true;
	
	char *t_name = nil;
	uint32_t t_xhot, t_yhot;
	t_xhot = t_yhot = 0;
	
	uint32_t t_width, t_height;
	t_width = t_height = 0;
	
	IO_handle t_stream;
	t_stream = GetStream();
	
	while (t_success)
	{
		t_success = IO_ERROR != IO_fgets(m_line, XBM_MAX_LINE, t_stream);

		if (t_success)
		{
			if (m_line[0] == '#')
			{
				int32_t t_value = 0;
				char *t_newname = nil;
				c_bitmap_define t_define = kMCDefineUnknown;

				t_success = c_bitmap_split_define(m_line, t_newname, t_define, t_value);
				if (t_success && t_define != kMCDefineUnknown)
				{
					if (t_name == nil)
					{
						t_name = t_newname;
						t_newname = nil;
					}

					if (t_newname == nil || MCCStringEqual(t_name, t_newname))
					{
						switch (t_define)
						{
						case kMCDefineWidth:
							t_width = t_value;
							t_xhot = t_width / 2;
							break;

						case kMCDefineHeight:
							t_height = t_value;
							t_yhot = t_height / 2;
							break;

						case kMCDefineXHot:
							t_xhot = t_value;
							break;

						case kMCDefineYHot:
							t_yhot = t_value;
							break;
						default:
							MCUnreachable();
						}
					}
				}
				MCCStringFree(t_newname);
			}
			else // t_line[0] != '#'
			{
				// check we have the start of the <name>_bits array
				uindex_t t_bits_index = 0;

				t_success = t_name != nil && t_width != 0 && t_height != 0;

				if (t_success)
					t_success = MCCStringFirstIndexOf(m_line, t_name, t_bits_index);
				if (t_success)
				{
					t_bits_index += MCCStringLength(t_name);
					t_success = MCCStringEqualSubstring(m_line + t_bits_index, "_bits[] = {", 11);
				}

				break;
			}
		}
	}

	if (t_success)
	{
		r_width = t_width;
		r_height = t_height;
		r_xhot = t_xhot;
		r_yhot = t_yhot;
		/* UNCHECKED */ MCStringCreateWithCString(t_name, r_name);
		r_frame_count = 1;
	}
	else
    {
        r_name = MCValueRetain(kMCEmptyString);
    }

	MCCStringFree(t_name);
	return t_success;
}

bool MCXBMImageLoader::LoadFrames(MCBitmapFrame *&r_frames, uint32_t &r_count)
{
	bool t_success = true;
	
	uint32_t t_width, t_height;
	
	MCBitmapFrame *t_frame;
	t_frame = nil;
	
	uint8_t *t_row_buffer;
	t_row_buffer = nil;

	IO_handle t_stream;
	t_stream = GetStream();

	if (t_success)
		t_success = GetGeometry(t_width, t_height);
	
	if (t_success)
		t_success = MCMemoryNew(t_frame);
	
	if (t_success)
		t_success = MCImageBitmapCreate(t_width, t_height, t_frame->image);

	const char *t_ptr = m_line;
	if (t_success)
		t_success = IO_ERROR != IO_fgets(m_line, XBM_MAX_LINE, t_stream);

    uint8_t *t_dst_ptr = nil;

	if (t_success)
    {
        uindex_t t_stride = (t_width + 7) / 8;
		t_dst_ptr = (uint8_t*)t_frame->image->data;
		t_success = MCMemoryAllocate(t_stride, t_row_buffer);
	}

	uint32_t t_black_pixel, t_white_pixel;
	t_black_pixel = MCGPixelPackNative(0, 0, 0, 255);
	t_white_pixel = MCGPixelPackNative(255, 255, 255, 255);
	for (uindex_t y = 0 ; t_success && y < t_height ; y++)
	{
		uindex_t t_stride = (t_width + 7) >> 3;
		uint8_t *t_dst_row = t_row_buffer;
		uindex_t t_byte_count = t_stride;
		while (t_success && t_byte_count)
		{
			const char *t_sptr = t_ptr;
			*t_dst_row = (uint1)strtol(t_sptr, (char **)&t_ptr, 16);
			if (t_ptr == t_sptr)
			{
				t_success = IO_NORMAL == IO_fgets(m_line, XBM_MAX_LINE, t_stream);
				t_ptr = m_line;
			}
			else
			{
				t_byte_count--;
				t_dst_row++;
				t_ptr++;
			}
		}
		if (t_success)
		{
			MCBitmapUnpackRow(t_dst_ptr, t_row_buffer, t_width, 1, false);
			// convert 8 bit 0 / 1 values to 32bit black / white pixels
			uint32_t *t_pixel_ptr = (uint32_t*)t_dst_ptr;
			uint8_t *t_index_ptr = t_dst_ptr + (t_width - 1);
			t_pixel_ptr += t_width - 1;
			uindex_t x = t_width;
			while (x--)
			{
				uint8_t t_index = *t_index_ptr--;
				*t_pixel_ptr-- = t_index == 0 ? t_black_pixel : t_white_pixel;
			}
		}
		t_dst_ptr += t_frame->image->stride;
	}

	MCMemoryDeallocate(t_row_buffer);

	if (t_success)
	{
		r_frames = t_frame;
		r_count = 1;
	}
	else
		MCImageFreeFrames(t_frame, 1);

	return t_success;
}

bool MCImageLoaderCreateForXBMStream(IO_handle p_stream, MCImageLoader *&r_loader)
{
	MCXBMImageLoader *t_loader;
	t_loader = new (nothrow) MCXBMImageLoader(p_stream);
	
	if (t_loader == nil)
		return false;
	
	r_loader = t_loader;
	
	return true;
}

////////////////////////////////////////////////////////////////////////////////

#define XPM_MAX_LINE 2048

static const char *s_xpm_color_keys[] =
{
    "s",					/* key #1: symbol */
    "m",					/* key #2: mono visual */
    "g4",					/* key #3: 4 grays visual */
    "g",					/* key #4: gray visual */
    "c",					/* key #5: color visual */
};

typedef enum
{
	kMCXPMKeySymbol,
	kMCXPMKeyMono,
	kMCXPMKey4Gray,
	kMCXPMKeyGray,
	kMCXPMKeyColor,
} xpm_color_key_t;

static bool xpm_lookup_color_key(const char *p_line, uindex_t p_start, uindex_t p_end, uindex_t &r_key)
{
	uindex_t t_len = p_end - p_start;
	const char *t_string = p_line + p_start;
	for (uindex_t i = 0; i < ELEMENTS(s_xpm_color_keys); i++)
	{
		if (MCCStringLength(s_xpm_color_keys[i]) == t_len && MCCStringEqualSubstring(s_xpm_color_keys[i], t_string, t_len))
		{
			r_key = i;
			return true;
		}
	}

	return false;
}

static bool xpm_next_token(const char *p_line, uindex_t p_line_start, uindex_t p_line_end, uint32_t &r_token_start, uint32_t &r_token_end)
{
	r_token_start = p_line_start;
	while (r_token_start < p_line_end && (p_line[r_token_start] == ' ' || p_line[r_token_start] == '\t' || p_line[r_token_start] == '\"' || p_line[r_token_start] == ','))
		r_token_start++;

	r_token_end = r_token_start;
	while (r_token_end < p_line_end && (p_line[r_token_end] != ' ') && (p_line[r_token_end] != '\t') && (p_line[r_token_end] != '\"'))
		r_token_end++;

	return r_token_start < r_token_end;
}

static bool xpm_next_key(const char *p_line, uindex_t &x_line_start, uindex_t p_line_end, uindex_t &r_key)
{
	uindex_t t_key_start, t_key_end;
	if (!xpm_next_token(p_line, x_line_start, p_line_end, t_key_start, t_key_end) ||
		!xpm_lookup_color_key(p_line, t_key_start, t_key_end, r_key))
		return false;

	x_line_start = t_key_end;
	return true;
}

static bool xpm_next_color(const char *p_line, uindex_t p_line_start, uindex_t p_line_end, uindex_t &r_color_start, uindex_t &r_color_end)
{
	uindex_t t_next_token_start, t_next_token_end;
	uindex_t t_key;
	
	r_color_end = r_color_start = p_line_start;

	if (!xpm_next_token(p_line, p_line_start, p_line_end, r_color_start, r_color_end))
		return false;

	while (xpm_next_token(p_line, r_color_end, p_line_end, t_next_token_start, t_next_token_end) &&
		!xpm_lookup_color_key(p_line, t_next_token_start, t_next_token_end, t_key))
	{
		r_color_end = t_next_token_end;
	}

	return true;
}

static bool hex_value(char p_char, uint8_t &r_value)
{
	if (p_char >= '0' && p_char <= '9')
	{
		r_value = p_char - '0';
		return true;
	}
	else if (p_char >= 'A' && p_char <= 'F')
	{
		r_value = 10 + p_char - 'A';
		return true;
	}
	else if (p_char >= 'a' && p_char <= 'f')
	{
		r_value = 10 + p_char - 'a';
		return true;
	}

	return false;
}

static bool xpm_parse_color(const char *p_line, uindex_t p_color_start, uindex_t p_color_end, uint32_t p_key, uint32_t &r_color)
{
	// TODO - color parsing code is incomplete - ignores the key type
	// & assumes colors are hex RGB values
	if (p_color_end - p_color_start != 7 || p_line[p_color_start] != '#')
	{
		MCColor t_color;
        MCAutoStringRef t_s;
        /* UNCHECKED */ MCStringCreateWithNativeChars((const char_t *) p_line + p_color_start, p_color_end - p_color_start, &t_s);
		if (MCscreen->lookupcolor(*t_s, &t_color))
		{
			r_color = MCGPixelPackNative(t_color.red >> 8, t_color.green >> 8, t_color.blue >> 8, 255);
			return true;
		}
		if (MCCStringEqualSubstring(p_line + p_color_start, "none", p_color_end - p_color_start))
		{
			r_color = 0x00000000;
			return true;
		}
		return false;
	}
	
	// IM-2014-07-31: [[ ImageLoader ]] Skip over the '#' char
	p_color_start++;
	
	// are there enough bytes for a rrggbb hex string?
	if (p_color_end - p_color_start < 6)
		return false;
	
	uint8_t t_color[3];
	for (uint32_t i = 0; i < 3; i++)
	{
		uint8_t t_high, t_low;
		if (!hex_value(p_line[p_color_start + i * 2], t_high) || !hex_value(p_line[p_color_start + i * 2 + 1], t_low))
			return false;
		t_color[i] = (t_high << 4) | t_low;
	}

	r_color = MCGPixelPackNative(t_color[0], t_color[1], t_color[2], 255);
	return true;
}

static bool xpm_parse_v3_color_line(const char *p_line, uint32_t p_chars_per_pixel, uint32_t &r_color, uint32_t &r_color_char)
{
	uindex_t t_line_start, t_line_end;
	
	if (!c_get_string_content_bounds(p_line, t_line_start, t_line_end))
		return false;

	uint32_t t_index = 0;

	if ((t_line_end - t_line_start) < p_chars_per_pixel)
		return false;

	while (p_chars_per_pixel--)
		t_index = (t_index << 8) + p_line[t_line_start++];

	bool t_have_color = false;

	uindex_t t_highest_key = 0;
	uindex_t t_key;
	while (xpm_next_key(p_line, t_line_start, t_line_end, t_key))
	{
		uindex_t t_color_start, t_color_end;
		if (!xpm_next_color(p_line, t_line_start, t_line_end, t_color_start, t_color_end))
			return false;

		if (t_key >= t_highest_key)
		{
			if (!xpm_parse_color(p_line, t_color_start, t_color_end, t_key, r_color))
				return false;
			t_have_color = true;
			t_highest_key = t_key;
		}
		t_line_start = t_color_end;
	}

	r_color_char = t_index;
	return t_have_color;
}

bool xpm_read_v3_header(IO_handle p_stream, uindex_t &r_width, uindex_t &r_height, uindex_t &r_chars_per_pixel, uint32_t *&r_colors, uint32_t *&r_color_chars, uindex_t &r_color_count) //, MCPoint &r_hotspot, char *&r_name)
{
	bool t_success = true;

	int32_t t_width, t_height, t_color_count, t_chars_per_pixel;
	uint32_t *t_colors = nil;
	uint32_t *t_color_chars = nil;

	char t_line[XPM_MAX_LINE];

	/* read the assignment line */
	t_success = IO_NORMAL == IO_fgets(t_line, XPM_MAX_LINE, p_stream);
	if (t_success)
		t_success = MCCStringBeginsWith(t_line, "static char");

	/* read the hints line */
	if (t_success)
		t_success = IO_NORMAL == IO_fgets(t_line, XPM_MAX_LINE, p_stream);

	/* skip the comments line if any */
	if (t_success)
	{
		while (t_success && MCCStringBeginsWith(t_line, "/*"))
		{
			while (t_success && !MCCStringContains(t_line, "*/"))
				t_success = IO_NORMAL == IO_fgets(t_line, XPM_MAX_LINE, p_stream);
			if (t_success)
				t_success = IO_NORMAL == IO_fgets(t_line, XPM_MAX_LINE, p_stream);
		}
	}

	if (t_success) /* TODO - this will fail if the x, y hotspot or any extension is specified */
		t_success = 4 == sscanf(t_line, "\"%d %d %d %d\",", &t_width, &t_height, &t_color_count, &t_chars_per_pixel);

	if (t_success)
		t_success = MCMemoryNewArray(t_color_count, t_colors) &&
		MCMemoryNewArray(t_color_count, t_color_chars);

	for (uindex_t i = 0; t_success && i < t_color_count; i++)
	{
		t_success = IO_NORMAL == IO_fgets(t_line, XPM_MAX_LINE, p_stream);
		/* skip the comment line if any */
		if (t_success && MCCStringBeginsWith(t_line, "/*"))
			t_success = IO_NORMAL == IO_fgets(t_line, XPM_MAX_LINE, p_stream);

		if (t_success)
			t_success = xpm_parse_v3_color_line(t_line, t_chars_per_pixel, t_colors[i], t_color_chars[i]);
	}

	if (t_success)
	{
		r_width = t_width;
		r_height = t_height;
		r_chars_per_pixel = t_chars_per_pixel;
		r_colors = t_colors;
		r_color_chars = t_color_chars;
		r_color_count = t_color_count;
	}
	else
	{
		MCMemoryDeleteArray(t_colors);
		MCMemoryDeleteArray(t_color_chars);
	}

	return t_success;
}

static bool xpm_parse_v1_color_line(const char *p_line, uint32_t p_chars_per_pixel, uint32_t &r_color, uint32_t &r_color_char)
{
	uindex_t t_line_start, t_line_end;
	
	if (!c_get_string_content_bounds(p_line, t_line_start, t_line_end))
		return false;

	uint32_t t_index = 0;

	if ((t_line_end - t_line_start) < p_chars_per_pixel)
		return false;

	while (p_chars_per_pixel--)
		t_index = (t_index << 8) | p_line[t_line_start++];

	bool t_have_color = false;

	uindex_t t_color_start, t_color_end;
	if (!xpm_next_color(p_line, t_line_start, t_line_end, t_color_start, t_color_end) ||
		!xpm_parse_color(p_line, t_color_start, t_color_end, kMCXPMKeyColor, r_color))
		return false;

	r_color_char = t_index;
	return t_have_color;
}

static bool xpm_read_v1_header(IO_handle p_stream, char x_line[XPM_MAX_LINE], uindex_t &r_width, uindex_t &r_height, uindex_t &r_chars_per_pixel, uint32_t *&r_colors, uint32_t *&r_color_chars, uindex_t &r_color_count)
{
	bool t_success = true;

	uindex_t t_width = 0, t_height = 0;
	uindex_t t_format = 1, t_chars_per_pixel = 1;
	uint32_t *t_colors = nil;
	uint32_t *t_color_chars = nil;
	uindex_t t_color_count = 0;
	char *t_name = nil;

	bool t_at_color_table = false;

	while (t_success)
	{
		if (x_line[0] == '#')
		{
			int32_t t_value = 0;
			char *t_newname = nil;
			c_bitmap_define t_define = kMCDefineUnknown;

			t_success = c_bitmap_split_define(x_line, t_newname, t_define, t_value);
			if (t_success && t_define != kMCDefineUnknown)
			{
				if (t_name == nil)
				{
					t_name = t_newname;
					t_newname = nil;
				}

				if (t_newname == nil || MCCStringEqual(t_name, t_newname))
				{
					switch (t_define)
					{
					case kMCDefineWidth:
						t_width = t_value;
						//t_hotspot.x = t_width / 2;
						break;

					case kMCDefineHeight:
						t_height = t_value;
						//t_hotspot.y = t_height / 2;
						break;

					case kMCDefineXHot:
					//	t_hotspot.x = t_value;
						break;

					case kMCDefineYHot:
					//	t_hotspot.y = t_value;
						break;

					case kMCDefineFormat:
						t_format = t_value;
						break;

					case kMCDefineNColors:
						t_color_count = t_value;
						break;

					case kMCDefineCharsPerPixel:
						t_chars_per_pixel = t_value;
						break;
							
					case kMCDefineUnknown:
						MCUnreachable();
						break;
					}
				}
			}
			MCCStringFree(t_newname);
		}
		else // t_line[0] != '#'
		{
			// check we have the start of the <name>_colors array
			uindex_t t_colors_index = 0;

			t_success = t_name != nil && t_width != 0 && t_height != 0;

			if (t_success && MCCStringFirstIndexOf(x_line, t_name, t_colors_index))
			{
				t_colors_index += MCCStringLength(t_name);
				// may be a monochrome table, in which case we keep looking
				if (MCCStringEqualSubstring(x_line + t_colors_index, "_colors[] = {", 13))
				{
					t_at_color_table = true;
					break;
				}
			}
		}

		if (t_success)
			t_success = IO_NORMAL == IO_fgets(x_line, XPM_MAX_LINE, p_stream);
	}

	if (t_success)
		t_success = t_at_color_table && t_color_count > 0;

	if (t_success)
		t_success = MCMemoryNewArray(t_color_count, t_colors) && MCMemoryNewArray(t_color_count, t_color_chars);

	for (uindex_t i = 0; t_success && i < t_color_count; i++)
	{
		t_success = IO_NORMAL == IO_fgets(x_line, XPM_MAX_LINE, p_stream);

		if (t_success)
			t_success = xpm_parse_v1_color_line(x_line, t_chars_per_pixel, t_colors[i], t_color_chars[i]);
	}

	/* read until we reach the pixels */
	while (t_success)
	{
		t_success = IO_NORMAL == IO_fgets(x_line, XPM_MAX_LINE, p_stream);
		if (t_success)
		{
			// check we have the start of the <name>_pixels array
			uindex_t t_index = 0;

			if (MCCStringFirstIndexOf(x_line, t_name, t_index))
			{
				t_index += MCCStringLength(t_name);
				//if it's the pixels we can stop looking
				if (MCCStringEqualSubstring(x_line + t_index, "_pixels[] = {", 13))
					break;
			}
		}
	}

	if (t_success)
	{
		r_width = t_width;
		r_height = t_height;
		r_chars_per_pixel = t_chars_per_pixel;
		r_colors = t_colors;
		r_color_chars = t_color_chars;
		r_color_count = t_color_count;
	}
	else
	{
		MCMemoryDeleteArray(t_colors);
		MCMemoryDeleteArray(t_color_chars);
	}

    MCCStringFree(t_name);
	return t_success;
}

class MCXPMImageLoader : public MCImageLoader
{
public:
	MCXPMImageLoader(IO_handle p_stream);
	virtual ~MCXPMImageLoader();
	
	virtual MCImageLoaderFormat GetFormat() { return kMCImageFormatXPM; }
	
protected:
	virtual bool LoadHeader(uint32_t &r_width, uint32_t &r_height, uint32_t &r_xhot, uint32_t &r_yhot, MCStringRef &r_name, uint32_t &r_frame_count, MCImageMetadata &r_metadata);
	virtual bool LoadFrames(MCBitmapFrame *&r_frames, uint32_t &r_count);
	
private:
	char m_line[XPM_MAX_LINE];
	uindex_t m_chars_per_pixel;
	uindex_t m_color_count;
	uint32_t *m_colors;
	uint32_t *m_color_chars;
};

MCXPMImageLoader::MCXPMImageLoader(IO_handle p_stream) : MCImageLoader(p_stream)
{
	m_chars_per_pixel = 0;
	m_color_count = 0;
	m_colors = nil;
	m_color_chars = nil;
}

MCXPMImageLoader::~MCXPMImageLoader()
{
	MCMemoryDeleteArray(m_colors);
	MCMemoryDeleteArray(m_color_chars);
}

bool MCXPMImageLoader::LoadHeader(uint32_t &r_width, uint32_t &r_height, uint32_t &r_xhot, uint32_t &r_yhot, MCStringRef &r_name, uint32_t &r_frame_count, MCImageMetadata &r_metadata)
{
	bool t_success = true;
	
	uindex_t t_width = 0;
	uindex_t t_height = 0;
	
	IO_handle t_stream;
	t_stream = GetStream();

	t_success = IO_NORMAL == IO_fgets(m_line, XPM_MAX_LINE, t_stream);
	if (t_success)
	{
		if (MCCStringBeginsWith(m_line, "/* XPM") && MCCStringContains(m_line + 6, " */"))
			t_success = xpm_read_v3_header(t_stream, t_width, t_height, m_chars_per_pixel, m_colors, m_color_chars, m_color_count);
		else
			t_success = xpm_read_v1_header(t_stream, m_line, t_width, t_height, m_chars_per_pixel, m_colors, m_color_chars, m_color_count);
	}

	if (t_success)
	{
		r_width = t_width;
		r_height = t_height;
		
		r_xhot = r_yhot = 0;
		
		r_name = MCValueRetain(kMCEmptyString);
		r_frame_count = 1;
	}
	
	return t_success;
}

bool MCXPMImageLoader::LoadFrames(MCBitmapFrame *&r_frames, uint32_t &r_count)
{
	bool t_success;
	t_success = true;
	
	MCBitmapFrame *t_frame;
	t_frame = nil;
	
	IO_handle t_stream;
	t_stream = GetStream();
	
	uint32_t t_width, t_height;
	if (t_success)
		t_success = GetGeometry(t_width, t_height);
	
	if (t_success)
		t_success = MCMemoryNew(t_frame);

	if (t_success)
		t_success = MCImageBitmapCreate(t_width, t_height, t_frame->image);

	uint8_t *t_dst_ptr = nil;

	if (t_success)
		t_dst_ptr = (uint8_t*)t_frame->image->data;

	for (uindex_t y = 0 ; t_success && y < t_height ; y++)
	{
		uindex_t t_row_start, t_row_end;

		if (t_success)
			t_success = IO_NORMAL == IO_fgets(m_line, XPM_MAX_LINE, t_stream);
		while (t_success && MCCStringBeginsWith(m_line, "/*"))
			t_success = IO_NORMAL == IO_fgets(m_line, XPM_MAX_LINE, t_stream);
		
		if (t_success)
			t_success = c_get_string_content_bounds(m_line, t_row_start, t_row_end) &&
			(t_row_end - t_row_start) >= (t_width * m_chars_per_pixel);

		uint32_t *t_dst_row = (uint32_t*)t_dst_ptr;
		for (uindex_t x = 0; t_success && x < t_frame->image->width; x++)
		{
			uint32_t t_index = 0;
			for (uindex_t i = 0; i < m_chars_per_pixel; i++)
				t_index = (t_index << 8) | m_line[t_row_start++];
			bool t_found_color = false;
			uint32_t t_color = MCGPixelPackNative(0, 0, 0, 255);
			for (uindex_t i = 0; !t_found_color && i < m_color_count; i++)
			{
				if (m_color_chars[i] == t_index)
				{
					t_found_color = true;
					t_color = m_colors[i];
					// check for 'none' - transparent color
					if (t_color == 0x00)
						t_frame->image->has_transparency = true;
				}
			}

			*t_dst_row++ = t_color;
		}

		t_dst_ptr += t_frame->image->stride;
	}

	if (t_success)
	{
		r_frames = t_frame;
		r_count = 1;
	}
	else
		MCImageFreeFrames(t_frame, 1);

	return t_success;
}

bool MCImageLoaderCreateForXPMStream(IO_handle p_stream, MCImageLoader *&r_loader)
{
	MCXPMImageLoader *t_loader;
	t_loader = new (nothrow) MCXPMImageLoader(p_stream);
	
	if (t_loader == nil)
		return false;
	
	r_loader = t_loader;
	
	return true;
}

////////////////////////////////////////////////////////////////////////////////

typedef struct
{
	uint32_t header_size;
	uint32_t file_version;
	uint32_t pixmap_format;
	uint32_t pixmap_depth;
	uint32_t pixmap_width;
	uint32_t pixmap_height;
	uint32_t xoffset;
	uint32_t byte_order;
	uint32_t bitmap_unit;
	uint32_t bitmap_bit_order;
	uint32_t bitmap_pad;
	uint32_t bits_per_pixel;
	uint32_t bytes_per_line;
	uint32_t visual_class;
	uint32_t red_mask;
	uint32_t green_mask;
	uint32_t blue_mask;
	uint32_t bits_per_rgb;
	uint32_t colormap_entries;
	uint32_t ncolors;
	uint32_t window_width;
	uint32_t window_height;
	uint32_t window_x;
	uint32_t window_y;
	uint32_t window_bdr_width;
} xwd_file_header_t;

#define xwd_file_header_size 100

class MCXWDImageLoader : public MCImageLoader
{
public:
	MCXWDImageLoader(IO_handle p_stream);
	virtual ~MCXWDImageLoader();
	
	virtual MCImageLoaderFormat GetFormat() { return kMCImageFormatXWD; }
	
protected:
	virtual bool LoadHeader(uint32_t &r_width, uint32_t &r_height, uint32_t &r_xhot, uint32_t &r_yhot, MCStringRef &r_name, uint32_t &r_frame_count, MCImageMetadata &r_metadata);
	virtual bool LoadFrames(MCBitmapFrame *&r_frames, uint32_t &r_count);
	
private:
	xwd_file_header_t m_fh;
};

MCXWDImageLoader::MCXWDImageLoader(IO_handle p_stream) : MCImageLoader(p_stream)
{
}

MCXWDImageLoader::~MCXWDImageLoader()
{
}

bool MCXWDImageLoader::LoadHeader(uint32_t &r_width, uint32_t &r_height, uint32_t &r_xhot, uint32_t &r_yhot, MCStringRef &r_name, uint32_t &r_frame_count, MCImageMetadata &r_metadata)
{
	bool t_success = true;
	
	IO_handle stream;
	stream = GetStream();
	
	uint32_t t_width, t_height;
	
	uint2 i;
	uint4 *fourptr = (uint4 *)&m_fh;
	for (i = 0 ; i < (uint2)(sizeof(m_fh) >> 2) ; i++)
		if (IO_read_uint4(fourptr++, stream) != IO_NORMAL)
			return false;
	uint4 namesize = m_fh.header_size - xwd_file_header_size;
	if (m_fh.file_version != 7 || m_fh.ncolors > 256 || namesize > 256)
		return false;

	t_width = m_fh.pixmap_width;
	t_height = m_fh.pixmap_height;
	char *newname = nil;

	if (t_success)
        // SN-2015-06-19: [[ CID 48071 ]] Use MCMemoryNewArray for consistency
        t_success = MCMemoryNewArray(namesize, newname)
            && IO_read(newname, namesize, stream) == IO_NORMAL;

	if (t_success)
	{
		r_width = m_fh.pixmap_width;
		r_height = m_fh.pixmap_height;
		
		r_xhot = r_yhot = 0;
		
		/* UNCHEKCED */ MCStringCreateWithCString(newname, r_name);
		r_frame_count = 1;
	}
	else
    {
		MCCStringFree(newname);
        r_name = MCValueRetain(kMCEmptyString);
    }

	return t_success;
}

bool MCXWDImageLoader::LoadFrames(MCBitmapFrame *&r_frames, uint32_t &r_count)
{
	IO_handle stream;
	stream = GetStream();
	
    MCAutoPointer<MCColor[]> colors = new (nothrow) MCColor[m_fh.ncolors];
    if (!colors)
        return false;

	for (uint32_t i = 0 ; i < (uint2)m_fh.ncolors ; i++)
	{
		uint32_t t_pixel;
		uint8_t t_flags, t_pad;
        if (IO_read_uint4(&t_pixel, stream) != IO_NORMAL ||
            IO_read_uint2(&colors[i].red, stream) != IO_NORMAL ||
            IO_read_uint2(&colors[i].green, stream) != IO_NORMAL ||
            IO_read_uint2(&colors[i].blue, stream) != IO_NORMAL ||
            IO_read_uint1(&t_flags, stream) != IO_NORMAL ||
            IO_read_uint1(&t_pad, stream) != IO_NORMAL)
        {
            return false;
        }
	}

    if (m_fh.pixmap_depth == 24)
        m_fh.pixmap_depth = 32;
    if (m_fh.pixmap_depth == 1)
        m_fh.pixmap_format = XYPixmap;
    uint4 bytes = m_fh.bytes_per_line * m_fh.pixmap_height;
    if (m_fh.bits_per_pixel == 1)
        bytes *= m_fh.pixmap_depth;

    MCAutoPointer<char[]> t_newimage_data = new (nothrow) char[bytes];
    if (!t_newimage_data)
        return false;

    if (IO_read(t_newimage_data.Get(), bytes, stream) != IO_NORMAL)
        return false;

    uint32_t t_width, t_height;
    if (!GetGeometry(t_width, t_height))
        return false;

    MCBitmapFrame *t_frame = nullptr;
    if (!MCMemoryNew(t_frame))
        return false;

    if (!MCImageBitmapCreate(t_width, t_height, t_frame->image))
    {
        MCImageFreeFrames(t_frame, 1);
        return false;
    }

    uint2 redshift, greenshift, blueshift, redbits, greenbits, bluebits;

    if (m_fh.bits_per_pixel > 8)
    {
        MCU_getshift(m_fh.red_mask, redshift, redbits);
        MCU_getshift(m_fh.green_mask, greenshift, greenbits);
        MCU_getshift(m_fh.blue_mask, blueshift, bluebits);
    }

    uint32_t t_black = MCGPixelPackNative(0,   0,   0,   255);
    uint32_t t_white = MCGPixelPackNative(255, 255, 255, 255);
    for (uint2 y = 0 ; y < t_height ; y++)
    {
        uint4 *dptr = (uint4 *) ((uint8_t*)t_frame->image->data + y * t_frame->image->stride);
        uint1 *oneptr = (uint1 *)&t_newimage_data[y * m_fh.bytes_per_line];
        uint2 *twoptr = (uint2 *)oneptr;
        uint4 *fourptr = (uint4 *)oneptr;
        uint2 x;
        for (x = 0 ; x < t_width ; x++)
        {
            uint4 pixel;
            switch (m_fh.bits_per_pixel)
            {
            case 1:
                *dptr++ = 0x80 >> (x & 0x7) & oneptr[x >> 3] ? t_white : t_black;
                break;
            case 4:
                pixel = oneptr[x >> 1] >> 4 * (x & 1) & 0x0F;
                *dptr++ = MCColorGetPixel(colors[pixel]);
                break;
            case 8:
                pixel = oneptr[x];
                *dptr++ = MCColorGetPixel(colors[pixel]);
                break;
            case 16:
                pixel = twoptr[x];
                *dptr++ = MCGPixelPackNative(
                    ((pixel & m_fh.red_mask) >> redshift) << (8 - redbits),
                    ((pixel & m_fh.green_mask) >> greenshift) << (8 - greenbits),
                    ((pixel & m_fh.blue_mask) >> blueshift) << (8 - bluebits),
                    255);
                break;
            case 32:
                if (MCswapbytes)
                    swap_uint4(&fourptr[x]);
                *dptr++ = MCGPixelPackNative(
                    (fourptr[x] >> 24) & 0xFF,
                    (fourptr[x] >> 16) & 0xFF,
                    (fourptr[x] >> 8) & 0xFF,
                    255);
                break;
            default:
                *dptr++ = MCGPixelPackNative(
                    (fourptr[x] >> 24) & 0xFF,
                    (fourptr[x] >> 16) & 0xFF,
                    (fourptr[x] >> 8) & 0xFF,
                    255);
                break;
            }
        }
    }

    r_frames = t_frame;
    r_count = 1;
    return true;
}

bool MCImageLoaderCreateForXWDStream(IO_handle p_stream, MCImageLoader *&r_loader)
{
	MCXWDImageLoader *t_loader;
	t_loader = new (nothrow) MCXWDImageLoader(p_stream);
	
	if (t_loader == nil)
		return false;
	
	r_loader = t_loader;
	
	return true;
}

////////////////////////////////////////////////////////////////////////////////

bool MCImageEncodeRawTrueColor(MCImageBitmap *p_bitmap, IO_handle p_stream, Export_format p_format, uindex_t &r_bytes_written)
{
	bool t_success = true;

	uindex_t t_byte_count = 0;

	uint8_t *t_row_buffer = nil;
	uint32_t t_stride = p_bitmap->width * 4;

	t_success = MCMemoryAllocate(t_stride, t_row_buffer);

	uint8_t *t_src_ptr = (uint8_t*)p_bitmap->data;
	for (uindex_t y = 0; y < p_bitmap->height; y++)
	{
		switch (p_format)
		{
		case EX_RAW_RGBA:
			MCBitmapConvertRow<EX_RAW_RGBA>(t_row_buffer, (uint32_t*)t_src_ptr, p_bitmap->width);
			break;
		case EX_RAW_BGRA:
			MCBitmapConvertRow<EX_RAW_BGRA>(t_row_buffer, (uint32_t*)t_src_ptr, p_bitmap->width);
			break;
		case EX_RAW_ARGB:
			MCBitmapConvertRow<EX_RAW_ARGB>(t_row_buffer, (uint32_t*)t_src_ptr, p_bitmap->width);
			break;
		case EX_RAW_ABGR:
			MCBitmapConvertRow<EX_RAW_ABGR>(t_row_buffer, (uint32_t*)t_src_ptr, p_bitmap->width);
			break;
		default:
			MCUnreachable();
		}
		t_success = IO_NORMAL == IO_write(t_row_buffer, sizeof(uint8_t), t_stride, p_stream);
		t_byte_count += t_stride;
		t_src_ptr += p_bitmap->stride;
	}

	MCMemoryDeleteArray(t_row_buffer);

	if (t_success)
		r_bytes_written = t_byte_count;

	return t_success;
}

bool MCImageEncodeRawIndexed(MCImageIndexedBitmap *p_indexed, IO_handle p_stream, uindex_t &r_bytes_written)
{
	bool t_success = true;

	// set transparent pixels to the last index in the palette
	// TODO


	uint32_t t_depth = MCImageDepth(p_indexed->palette_size);

	uint32_t t_byte_count = 0;

	uint8_t *t_row_buffer = nil;
	uint32_t t_stride = (p_indexed->width * t_depth + 0x7) >> 3;

	t_success = MCMemoryNewArray(t_stride, t_row_buffer);

	uint8_t *t_src_ptr;
	if (t_success)
		t_src_ptr = p_indexed->data;

	for (uint32_t y = 0; t_success && y < p_indexed->height && t_success; y++)
	{
		MCBitmapPackRow(t_row_buffer, t_src_ptr, p_indexed->width, t_depth);
		t_success = IO_NORMAL == IO_write(t_row_buffer, sizeof(uint8_t), t_stride, p_stream);
		t_byte_count += t_stride;
		t_src_ptr += p_indexed->stride;
	}

	MCMemoryDeleteArray(t_row_buffer);

	if (t_success)
		r_bytes_written = t_byte_count;

	return t_success;
}

bool MCImageEncodeRawIndexed(MCImageBitmap *p_bitmap, IO_handle p_stream, uindex_t &r_bytes_written)
{
	bool t_success = true;

	MCImageIndexedBitmap *t_indexed = nil;
	if (!MCImageConvertBitmapToIndexed(p_bitmap, false, t_indexed))
		return false;

	t_success = MCImageEncodeRawIndexed(t_indexed, p_stream, r_bytes_written);

	MCImageFreeIndexedBitmap(t_indexed);

	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

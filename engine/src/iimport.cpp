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

#include "core.h"
#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "mcio.h"

#include "execpt.h"
#include "util.h"
#include "image.h"
#include "stack.h"
#include "securemode.h"

#include "globals.h"

bool read_all(IO_handle p_stream, uint8_t *&r_data, uindex_t &r_data_size)
{
	bool t_success = true;

	uint8_t *t_buffer = nil;
	uindex_t t_size = 0;

	t_size = MCS_fsize(p_stream) - MCS_tell(p_stream);

	t_success = MCMemoryAllocate(t_size, t_buffer);
	if (t_success)
		t_success = IO_NORMAL == MCS_read(t_buffer, sizeof(uint8_t), t_size, p_stream);

	if (t_success)
	{
		r_data = t_buffer;
		r_data_size = t_size;
	}
	else
		MCMemoryDeallocate(t_buffer);

	return t_success;
}

#define META_HEAD_SIZE 44
#define EMF_HEAD_SIZE 80
bool MCImageGetMetafileGeometry(IO_handle p_stream, uindex_t &r_width, uindex_t &r_height)
{
	bool t_success = true;
	bool t_is_meta = false;

	uindex_t t_stream_pos = MCS_tell(p_stream);
	uint8_t t_head[EMF_HEAD_SIZE];
	uindex_t t_size = META_HEAD_SIZE;

	t_success = IO_NORMAL == MCS_read(t_head, sizeof(uint8_t), t_size, p_stream) &&
		t_size == META_HEAD_SIZE;

	if (t_success)
	{
		// graphics metafile (wmf)
		if (memcmp(t_head, "\xD7\xCD\xC6\x9A", 4) == 0)
		{
			int16_t *t_bounds = (int16_t *)&t_head[6];
			r_width = ((t_bounds[2] - t_bounds[0]) * 72 + t_bounds[4] - 1) / t_bounds[4];
			r_height = ((t_bounds[3] - t_bounds[1]) * 72 + t_bounds[4] - 1) / t_bounds[4];
			t_is_meta = true;
		}
		// win32 metafile (emf)
		else if (memcmp(&t_head[40], "\x20\x45\x4D\x46", 4) == 0)
		{
			t_size = EMF_HEAD_SIZE;
			t_success = IO_NORMAL == MCS_seek_set(p_stream, t_stream_pos) &&
				IO_NORMAL == MCS_read(t_head, sizeof(uint8_t), t_size, p_stream) &&
				t_size == EMF_HEAD_SIZE;

			if (t_success)
			{
				int32_t *t_bounds;
				t_bounds = (int32_t*)&t_head[72];
				r_width = t_bounds[0];
				r_height = t_bounds[1];
				t_is_meta = true;
			}
		}
		else
		{
			uindex_t t_offset = 0;
			if (memcmp(t_head, "\0\0\0\0\0\0\0\0", 8) == 0)
			{
				t_size = META_HEAD_SIZE;
				t_offset = 512;
				t_success = IO_NORMAL == MCS_seek_set(p_stream, t_stream_pos + t_offset) &&
					IO_NORMAL == MCS_read(t_head, sizeof(uint8_t), t_size, p_stream) &&
					t_size == META_HEAD_SIZE;
			}

			// PICT file
			if (t_success && memcmp(&t_head[10], "\0\021\002\377", 4) == 0)
			{
				uint16_t *t_bounds = (uint16_t *)t_head;
				r_width = swap_uint2(&t_bounds[4]) - swap_uint2(&t_bounds[2]);
				r_height = swap_uint2(&t_bounds[3]) - swap_uint2(&t_bounds[1]);
				t_is_meta = true;
				t_stream_pos += t_offset;
			}
		}
	}

	MCS_seek_set(p_stream, t_stream_pos);

	return t_success && t_is_meta;
}

bool MCImageBitmapApplyMask(MCImageBitmap *p_bitmap, MCImageBitmap *p_mask)
{
	if (p_bitmap->width != p_mask->width || p_bitmap->height != p_mask->height)
		return false;

	uint8_t *t_src_ptr = (uint8_t*)p_mask->data;
	uint8_t *t_dst_ptr = (uint8_t*)p_bitmap->data;
	for (uindex_t y = 0; y < p_bitmap->height; y++)
	{
		uint32_t *t_src_row = (uint32_t*)t_src_ptr;
		uint32_t *t_dst_row = (uint32_t*)t_dst_ptr;
		for (uindex_t x = 0; x < p_bitmap->width; x++)
		{
			if (*t_src_row++ != 0xFFFFFFFF)
				*t_dst_row &= 0x00FFFFFF;
			t_dst_row++;
		}
		t_src_ptr += p_mask->stride;
		t_dst_ptr += p_bitmap->stride;
	}

	return true;
}

// decode image data to a series of frames, ignoring all the other bits & pieces
bool MCImageDecode(IO_handle p_stream, MCImageFrame *&r_frames, uindex_t &r_frame_count)
{
	bool t_success = true;

	MCImageFrame *t_frames = nil;
	uindex_t t_frame_count = 0;

	MCImageBitmap *t_bitmap = nil;
	MCImageCompressedBitmap *t_compressed = nil;

	MCPoint t_hotspot;
	char *t_name = nil;

	if (t_success)
		t_success = MCImageImport(p_stream, nil, t_hotspot, t_name, t_compressed, t_bitmap);

	if (t_success)
	{
		if (t_compressed != nil)
			t_success = MCImageDecompress(t_compressed, r_frames, r_frame_count);
		else
		{
			t_success = MCMemoryNewArray(1, r_frames);
			if (t_success)
			{
				r_frames[0].image = t_bitmap;
				t_bitmap = nil;
				r_frame_count = 1;
			}
		}
	}

	MCImageFreeCompressedBitmap(t_compressed);
	MCImageFreeBitmap(t_bitmap);
	MCCStringFree(t_name);

	return t_success;
}

bool MCImageDecode(const uint8_t *p_data, uindex_t p_size, MCImageFrame *&r_frames, uindex_t &r_frame_count)
{
	bool t_success = true;

	IO_handle t_stream = nil;

	t_success = nil != (t_stream = MCS_fakeopen(MCString((const char*)p_data, p_size)));

	if (t_success)
		t_success = MCImageDecode(t_stream, r_frames, r_frame_count);

	if (t_stream != nil)
		MCS_close(t_stream);

	return t_success;
}

// if the image is in a directly supported format return the raw data otherwise decode & return the bitmap
bool MCImageImport(IO_handle p_stream, IO_handle p_mask_stream, MCPoint &r_hotspot, char *&r_name, MCImageCompressedBitmap *&r_compressed, MCImageBitmap *&r_bitmap)
{
	bool t_success = true;

	uindex_t t_width = 0, t_height = 0;

	uint8_t t_head[8];
	uindex_t t_size = 8;

	uint32_t t_compression = F_RLE;

	if (t_success)
		t_success = MCS_read(t_head, sizeof(uint8_t), t_size, p_stream) == IO_NORMAL &&
		t_size == 8 && MCS_seek_cur(p_stream, -8) == IO_NORMAL;

	if (t_success)
	{
		if (memcmp(t_head, "GIF87a", 6) == 0)
			t_compression = F_GIF;
		else if (memcmp(t_head, "GIF89a", 6) == 0)
			t_compression = F_GIF;
		else if (memcmp(t_head, "\211PNG", 4) == 0)
			t_compression = F_PNG;
		else if (memcmp(t_head, "\xff\xd8", 2) == 0)
			t_compression = F_JPEG;
		else if (MCImageGetMetafileGeometry(p_stream, t_width, t_height))
			t_compression = F_PICT;

		if (t_compression != F_RLE)
		{
			t_success = MCImageCreateCompressedBitmap(t_compression, r_compressed);
			if (t_success)
			{
				if (t_success)
					t_success = read_all(p_stream, r_compressed->data, r_compressed->size);

				r_compressed->width = t_width;
				r_compressed->height = t_height;
			}
		}
		else
		{
			MCImageBitmap *t_bitmap = nil;
			
			if (memcmp(t_head, "BM", 2) == 0)
				t_success = MCImageDecodeBMP(p_stream, r_hotspot, t_bitmap);
			else if (memcmp(t_head, "#define", 7) == 0)
				t_success = MCImageDecodeXBM(p_stream, r_hotspot, r_name, t_bitmap);
			else if (memcmp(t_head, "/* XPM", 6) == 0)
				t_success = MCImageDecodeXPM(p_stream, t_bitmap);
			else if (t_head[0] == 'P' && (t_head[1] >= '1' && t_head[1] <= '6'))
			{
				t_success = MCImageDecodeNetPBM(p_stream, t_bitmap);
				// may have a mask image
				if (t_success && p_mask_stream != nil)
				{
					MCImageBitmap *t_mask = nil;
					t_success = MCImageDecodeNetPBM(p_mask_stream, t_mask) &&
						MCImageBitmapApplyMask(t_bitmap, t_mask);
					MCImageFreeBitmap(t_mask);
				}
			}
			else // if all else fails, assume it's an XWD
				t_success = MCImageDecodeXWD(p_stream, r_name, t_bitmap);

			if (t_success)
				r_bitmap = t_bitmap;
			else
				MCImageFreeBitmap(t_bitmap);
		}
	}

	return t_success;
}

IO_stat MCImage::import(const char *newname, IO_handle stream, IO_handle mstream)
{
	bool t_success = true;

	MCImageCompressedBitmap *t_compressed = nil;
	MCImageBitmap *t_bitmap = nil;
	char *t_name = nil;
	MCPoint t_hotspot = {1, 1};

	t_success = MCImageImport(stream, mstream, t_hotspot, t_name, t_compressed, t_bitmap);

	if (t_success)
	{
		if (t_compressed == nil)
			t_success = setbitmap(t_bitmap, 1.0);
		else
			t_success = setcompressedbitmap(t_compressed);
	}

	MCImageFreeCompressedBitmap(t_compressed);
	MCImageFreeBitmap(t_bitmap);

	uindex_t t_width, t_height;
	if (t_success)
		t_success = getsourcegeometry(t_width, t_height);

	if (t_success)
	{
		xhot = t_hotspot.x;
		yhot = t_hotspot.y;

		bool t_resize = true;
		t_resize = !(flags & F_LOCK_LOCATION);
		if (t_resize)
		{
			rect.width = t_width;
			rect.height = t_height;
		}

		if (m_rep->GetFrameCount() > 1)
		{
			if ((flags & F_REPEAT_COUNT) == 0)
				repeatcount = -1;
			irepeatcount = repeatcount;
			state |= CS_DO_START;
		}

		if (isunnamed() && t_name != nil)
			setname_cstring(t_name);
		if (isunnamed() && newname != nil)
		{
			const char *tname = strrchr(newname, PATH_SEPARATOR);
			if (tname != NULL)
				tname += 1;
			else
				tname = newname;
			setname_cstring(tname);
		}

	}

	MCCStringFree(t_name);

	return t_success ? IO_NORMAL : IO_ERROR;
}

void MCImage::set_gif(uint1 *data, uint4 length)
{
	if (data == nil)
		setrep(nil);
	else
		setdata(data, length);

	flags |= F_GIF;
	obj_id = 1;
}

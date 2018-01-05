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
#include "mcio.h"


#include "util.h"
#include "image.h"
#include "stack.h"
#include "securemode.h"

#include "globals.h"

#include "imageloader.h"

bool read_all(IO_handle p_stream, uint8_t *&r_data, uindex_t &r_data_size)
{
	bool t_success = true;

	uint8_t *t_buffer = nil;
	uindex_t t_size = 0;

	t_size = MCS_fsize(p_stream) - MCS_tell(p_stream);

	t_success = MCMemoryAllocate(t_size, t_buffer);
	if (t_success)
		t_success = IO_NORMAL == MCS_readfixed(t_buffer, t_size, p_stream);

	if (t_success)
	{
		r_data = t_buffer;
		r_data_size = t_size;
	}
	else
		MCMemoryDeallocate(t_buffer);

	return t_success;
}

#define DRAWING_HEAD_SIZE 12
bool MCImageGetMetafileGeometry(IO_handle p_stream, uindex_t &r_width, uindex_t &r_height)
{
	bool t_success = true;
	bool t_is_meta = false;

	uindex_t t_stream_pos = MCS_tell(p_stream);
	uint8_t t_head[DRAWING_HEAD_SIZE];
	uindex_t t_size = DRAWING_HEAD_SIZE;

	t_success = IO_NORMAL == MCS_readfixed(t_head, t_size, p_stream) &&
		t_size == DRAWING_HEAD_SIZE;
    
   /* A drawing's header is of the form:
     *   0: LCD\0
     *   4: width
     *   8: height
     */
	if (t_success &&
        memcmp(t_head, "LCD\0", 4) == 0)
	{
        float t_width = *(const float *)(t_head + 4);
        float t_height = *(const float *)(t_head + 8);
        
        /* The engine deals with integer bounds on sub-pixel co-ords, so
         * take the ceiling of the computer (float) width/height. */
        r_width = (uindex_t)ceilf(t_width);
        r_height = (uindex_t)ceilf(t_height);
        
        t_is_meta = true;
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
bool MCImageDecode(IO_handle p_stream, MCBitmapFrame *&r_frames, uindex_t &r_frame_count)
{
	bool t_success = true;

	// IM-2014-07-31: [[ ImageLoader ]] Update to use MCImageLoader class
	MCImageLoader *t_loader;
	t_loader = nil;

	if (t_success)
		t_success = MCImageLoader::LoaderForStream(p_stream, t_loader);

	if (t_success)
		t_success = t_loader->TakeFrames(r_frames, r_frame_count);

	if (t_loader != nil)
		delete t_loader;

	return t_success;
}

bool MCImageDecode(const uint8_t *p_data, uindex_t p_size, MCBitmapFrame *&r_frames, uindex_t &r_frame_count)
{
	bool t_success = true;
    MCAutoDataRef t_data;

	IO_handle t_stream = nil;

    if (t_success)
        t_success = nil != (t_stream = MCS_fakeopen((const char *)p_data, p_size));

	if (t_success)
		t_success = MCImageDecode(t_stream, r_frames, r_frame_count);

	if (t_stream != nil)
		MCS_close(t_stream);

	return t_success;
}

bool MCImageLoaderFormatToCompression(MCImageLoaderFormat p_format, uint32_t &r_compression)
{
	switch (p_format) {
		case kMCImageFormatGIF:
			r_compression = F_GIF;
			return true;
			
		case kMCImageFormatPNG:
			r_compression = F_PNG;
			return true;
			
		case kMCImageFormatJPEG:
			r_compression = F_JPEG;
			return true;
			
		case kMCImageFormatMetafile:
			r_compression = F_PICT;
			return true;
			
		default:
			break;
	}
	
	return false;
}

// if the image is in a directly supported format return the raw data otherwise decode & return the bitmap
bool MCImageImport(IO_handle p_stream, IO_handle p_mask_stream, MCPoint &r_hotspot, MCStringRef&r_name, MCImageCompressedBitmap *&r_compressed, MCImageBitmap *&r_bitmap)
{
	bool t_success;
	t_success = true;

	// IM-2014-07-31: [[ ImageLoader ]] Update to use MCImageLoader class
	MCImageLoaderFormat t_format;

	if (t_success)
		t_success = MCImageLoader::IdentifyFormat(p_stream, t_format);

	if (t_success)
	{
		uint32_t t_compression;
		if (MCImageLoaderFormatToCompression(t_format, t_compression))
		{
			t_success = MCImageCreateCompressedBitmap(t_compression, r_compressed);
			if (t_success)
			{
				uint32_t t_width, t_height;
				t_width = t_height = 0;
				
				if (t_success && t_compression == F_PICT)
					t_success = MCImageGetMetafileGeometry(p_stream, t_width, t_height);
				
				if (t_success)
					t_success = read_all(p_stream, r_compressed->data, r_compressed->size);

				r_compressed->width = t_width;
				r_compressed->height = t_height;
			}
		}
		else
		{
			MCImageLoader *t_loader;
			t_loader = nil;
			
			t_success = MCImageLoader::LoaderForStreamWithFormat(p_stream, t_format, t_loader);
			
			uint32_t t_xhot, t_yhot;
			
			MCAutoStringRef t_name;
			
			MCBitmapFrame *t_frames;
			t_frames = nil;
			
			uint32_t t_count;
            t_count = 0;
			
			if (t_success)
				t_success = t_loader->GetHotSpot(t_xhot, t_yhot);
			
			if (t_success)
				t_success = t_loader->GetName(&t_name);
			
			if (t_success)
				t_success = t_loader->TakeFrames(t_frames, t_count);
            
            if (t_success && p_mask_stream != nil && t_loader->GetFormat() == kMCImageFormatNetPBM)
				{
					MCImageBitmap *t_mask = nil;
					t_success = MCImageDecodeNetPBM(p_mask_stream, t_mask) &&
				MCImageBitmapApplyMask(t_frames[0].image, t_mask);
					MCImageFreeBitmap(t_mask);
				}

			if (t_success)
			{
				r_hotspot.x = t_xhot;
				r_hotspot.y = t_yhot;
                r_name = MCValueRetain(*t_name);
				r_bitmap = t_frames[0].image;
				t_frames[0].image = nil;
            }
			else
                r_name = MCValueRetain(kMCEmptyString);
			
			MCImageFreeFrames(t_frames, t_count);
			
			if (t_loader != nil)
				delete t_loader;
		}
	}

	return t_success;
}

IO_stat MCImage::import(MCStringRef newname, IO_handle stream, IO_handle mstream)
{
	bool t_success = true;

	MCImageCompressedBitmap *t_compressed = nil;
	MCImageBitmap *t_bitmap = nil;
	MCStringRef t_name = nil;
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
        {
            MCNewAutoNameRef t_name_nameref;
            /* UNCHECKED */ MCNameCreate(t_name, &t_name_nameref);
			setname(*t_name_nameref);
        }
		if (isunnamed() && newname != nil)
		{
            MCNewAutoNameRef t_name_nameref;
            uindex_t t_offset;
            if (MCStringLastIndexOfChar(newname, PATH_SEPARATOR, UINDEX_MAX, kMCCompareExact, t_offset))
            {
                /* UNCHECKED */ MCStringCopySubstring(newname, MCRangeMakeMinMax(t_offset + 1, MCStringGetLength(newname)), t_name);
                /* UNCHECKED */ MCNameCreate(t_name, &t_name_nameref);
            }
            else
                /* UNCHECKED */ MCNameCreate(newname, &t_name_nameref);

			setname(*t_name_nameref);
		}
	}

	MCValueRelease(t_name);

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

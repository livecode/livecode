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

#include "image.h"
#include "image_rep.h"

#include "graphics_util.h"

////////////////////////////////////////////////////////////////////////////////

MCImageRep::MCImageRep()
{
	m_reference_count = 0;
    
    MCMemoryClear(&m_metadata, sizeof(m_metadata));
}

MCImageRep::~MCImageRep()
{
}

MCImageRep *MCImageRep::Retain()
{
    m_reference_count += 1;
	return this;
}

void MCImageRep::Release()
{
    m_reference_count -= 1;
    if (m_reference_count == 0)
        delete this;
}

bool MCImageRep::IsLocked() const
{
    return false;
}

////////////////////////////////////////////////////////////////////////////////

#ifdef _MOBILE
#define DEFAULT_IMAGE_REP_CACHE_SIZE (1024 * 1024 * 64)
#else
#define DEFAULT_IMAGE_REP_CACHE_SIZE (1024 * 1024 * 256)
#endif

// IM-2014-11-25: [[ ImageRep ]] Rework loadable image rep to allow frame duration info to
//     be retained separately from frames.

MCLoadableImageRep::MCLoadableImageRep()
{
	m_lock_count = 0;
	m_have_header = false;
	m_have_frame_durations = false;

	m_frame_durations = nil;
	
	m_frames = nil;
	m_bitmap_frames = nil;
	m_frame_count = 0;

	m_frames_premultiplied = false;
	
	m_next = m_prev = nil;
    
}

MCLoadableImageRep::~MCLoadableImageRep()
{
	ReleaseFrames();
	MCMemoryDeleteArray(m_frame_durations);
}

////////////////////////////////////////////////////////////////////////////////

bool MCLoadableImageRep::EnsureHeader()
{
	if (!m_have_header)
	{
		if (!m_have_header)
			m_have_header = LoadHeader(m_width, m_height, m_frame_count);
	}
	
	return m_have_header;
}

bool MCLoadableImageRep::EnsureFrameDurations()
{
	if (!EnsureHeader())
		return false;
	
	if (m_have_frame_durations)
		return true;
	
	if (m_frame_count == 1)
	{
		m_have_frame_durations = true;
		return true;
	}
	
	uint32_t *t_frame_durations;
	t_frame_durations = nil;
	
	bool t_success;
	t_success = true;
	
	if (t_success)
		t_success = EnsureFrames();
	
	if (t_success && m_have_frame_durations)
		return true;
	
	if (t_success)
		t_success = m_bitmap_frames != nil;
	
	if (t_success)
		t_success = MCMemoryNewArray(m_frame_count, t_frame_durations);
	
	if (t_success)
	{
		for (uint32_t i = 0; i < m_frame_count; i++)
			t_frame_durations[i] = m_bitmap_frames[i].duration;
		
		m_frame_durations = t_frame_durations;
		m_have_frame_durations = true;
	}
	
	return m_have_frame_durations;
}

bool MCLoadableImageRep::ConvertToMCGFrames(MCBitmapFrame *&x_frames, uint32_t p_frame_count, bool p_premultiplied)
{
	bool t_success;
	t_success = true;
	
	MCGImageFrame *t_frames;
	t_frames = nil;
	
	uint32_t *t_frame_durations;
	t_frame_durations = nil;
	
	if (t_success)
		t_success = MCMemoryNewArray(p_frame_count, t_frames);
	
	if (t_success && !m_have_frame_durations && p_frame_count > 1)
		t_success = MCMemoryNewArray(p_frame_count, t_frame_durations);
	
	for (uint32_t i = 0; t_success && i < p_frame_count; i++)
	{
		// IM-2014-05-14: [[ ImageRepUpdate ]] Fix density & duration not being copied from the bitmap frames
		t_frames[i].x_scale = x_frames[i].x_scale;
		t_frames[i].y_scale = x_frames[i].y_scale;
		if (t_frame_durations != nil)
			t_frame_durations[i] = x_frames[i].duration;
		
		t_success = MCImageBitmapCopyAsMCGImageAndRelease(x_frames[i].image, p_premultiplied, t_frames[i].image);
	}
	
	if (t_success)
	{
		MCImageFreeFrames(x_frames, p_frame_count);
		x_frames = nil;

		m_frames = t_frames;
		
		if (!m_have_frame_durations)
		{
			m_frame_durations = t_frame_durations;
			m_have_frame_durations = true;
		}
		
		s_cache_size += GetFrameByteCount();
		
		if (s_cache_size > s_cache_limit)
		{
			// keep new frames in the cache while flushing
			m_lock_count++;
			FlushCacheToLimit();
			m_lock_count--;
		}
	}
	else
	{
		MCGImageFramesFree(t_frames, p_frame_count);
		MCMemoryDeleteArray(t_frame_durations);
	}
	
	return t_success;
}

bool MCLoadableImageRep::EnsureFrames()
{
	if (m_frames != nil || m_bitmap_frames != nil)
		return true;
	
	if (!EnsureHeader())
		return false;
	
	bool t_success;
	t_success = true;
	
	MCBitmapFrame *t_frames;
	t_frames = nil;
	
	uint32_t t_frame_count;
	t_frame_count = 0;
	
	bool t_premultiplied;
	
	t_success = LoadImageFrames(t_frames, t_frame_count, t_premultiplied);
	
	if (t_success)
	{
		if (t_premultiplied)
		{
			t_success = ConvertToMCGFrames(t_frames, t_frame_count, t_premultiplied);
		}
		else
		{
			m_bitmap_frames = t_frames;
			t_frames = nil;
		}
	}
	
	if (t_success)
	{
		m_frame_count = t_frame_count;
		m_frames_premultiplied = t_premultiplied;
	}
	
	return t_success;
}

bool MCLoadableImageRep::EnsureImageFrames()
{
	// IM-2013-11-05: [[ RefactorGraphics ]] Rework to allow LoadImageFrames to return either
	// premultiplied or non-premultiplied bitmaps
	if (m_frames != nil)
		return true;
	
	if (!EnsureFrames())
		return false;
	
	if (m_frames != nil)
		return true;
	
	bool t_success;
	t_success = true;
	
	if (t_success)
		t_success = ConvertToMCGFrames(m_bitmap_frames, m_frame_count, m_frames_premultiplied);
	
	if (t_success)
	{
		MCImageFreeFrames(m_bitmap_frames, m_frame_count);
		m_bitmap_frames = nil;
	}
	
	return t_success;
}

bool MCImageBitmapCreateWithPixels(void *p_pixels, uint32_t p_stride, uint32_t p_width, uint32_t p_height, MCImageBitmap *&r_bitmap)
{
	if (!MCImageBitmapCreate(p_width, p_height, r_bitmap))
		return false;
	
	if (p_stride == r_bitmap->stride)
		MCMemoryCopy(r_bitmap->data, p_pixels, p_stride * p_height);
	else
	{
		uint8_t *t_dst;
		uint8_t *t_src;
		t_dst = (uint8_t*)r_bitmap->data;
		t_src = (uint8_t*)p_pixels;
		
		for (uint32_t i = 0; i < p_height; i++)
		{
			MCMemoryCopy(t_dst, t_src, p_width * sizeof(uint32_t));
			t_dst += r_bitmap->stride;
			t_src +=  p_stride;
		}
	}
	return true;
}

bool convert_to_mcbitmapframes(MCGImageFrame *p_frames, uint32_t *p_frame_durations, uint32_t p_frame_count, MCBitmapFrame *&r_frames)
{
	if (p_frames == nil)
		return false;
	
	bool t_success;
	t_success = true;
	
	MCBitmapFrame *t_frames;
	t_frames = nil;
	
	t_success = MCMemoryNewArray(p_frame_count, t_frames);
	
	for (uint32_t i = 0; t_success && i < p_frame_count; i++)
	{
		MCGRaster t_raster;
		t_success = MCGImageGetRaster(p_frames[i].image, t_raster);
		
		if (t_success)
			t_success = MCImageBitmapCreateWithPixels(t_raster.pixels, t_raster.stride, t_raster.width, t_raster.height, t_frames[i].image);
		
		if (t_success)
		{
			t_frames[i].image->has_alpha = t_frames[i].image->has_transparency = t_raster.format == kMCGRasterFormat_ARGB;

			if (t_frames[i].image->has_transparency)
				MCImageBitmapUnpremultiply(t_frames[i].image);
			
			if (p_frame_durations != nil)
				t_frames[i].duration = p_frame_durations[i];
			else
				t_frames[i].duration = 0;
		}
	}
	
	if (t_success)
		r_frames = t_frames;
	else
		MCImageFreeFrames(t_frames, p_frame_count);
	
	return t_success;
}

bool MCLoadableImageRep::EnsureBitmapFrames()
{
	if (m_bitmap_frames != nil)
		return true;
	
	if (!EnsureFrames())
		return false;
	
	if (m_bitmap_frames != nil)
		return true;
	
	if (m_frames_premultiplied)
		return convert_to_mcbitmapframes(m_frames, m_frame_durations, m_frame_count, m_bitmap_frames);
	else
		return LoadImageFrames(m_bitmap_frames, m_frame_count, m_frames_premultiplied);
}

bool MCLoadableImageRep::LockImageFrame(uindex_t p_frame, MCGFloat p_density, MCGImageFrame& r_frame)
{
	if (!EnsureHeader())
		return false;
	
	// frame index check
	if (p_frame >= m_frame_count)
		return false;
    
	if (!EnsureImageFrames())
		return false;
	
	r_frame = m_frames[p_frame];
    MCGImageRetain(r_frame . image);
    
	MoveRepToHead(this);
	
	return true;
}

void MCLoadableImageRep::UnlockImageFrame(uindex_t p_index, MCGImageFrame& p_frame)
{
    MCGImageRelease(p_frame . image);
}

bool MCLoadableImageRep::LockBitmap(uindex_t p_frame, MCGFloat p_density, MCImageBitmap *&r_bitmap)
{
	if (!EnsureHeader())
		return false;
	
	// frame index check
	if (p_frame >= m_frame_count)
		return false;
	
	if (!EnsureBitmapFrames())
		return false;
	
	Retain();
	
	r_bitmap = m_bitmap_frames[p_frame].image;
	
	return true;
}

void MCLoadableImageRep::UnlockBitmap(uindex_t p_index, MCImageBitmap *p_bitmap)
{
	if (p_bitmap == nil)
		return;

	if (p_index >= m_frame_count)
		return;

	if (m_bitmap_frames == nil || m_bitmap_frames[p_index].image != p_bitmap)
		return;

	if (m_frames == nil)
		ConvertToMCGFrames(m_bitmap_frames, m_frame_count, false);
    
    // PM-2015-07-13: [[ Bug 15590 ]] Free the correct number of frames
	MCImageFreeFrames(m_bitmap_frames, m_frame_count);
	m_bitmap_frames = nil;
	
	Release();

	MoveRepToHead(this);
}

// MERG-2014-09-16: [[ ImageMetadata ]] Support for image metadata property
bool MCLoadableImageRep::GetMetadata(MCImageMetadata& r_metadata)
{
    if (!EnsureHeader())
        return false;
    
    r_metadata = m_metadata;
    
    return true;
}

////////////////////////////////////////////////////////////////////////////////

void MCGImageFramesFree(MCGImageFrame *p_frames, uint32_t p_frame_count)
{
	if (p_frames == nil)
		return;
	
	for (uint32_t i = 0; i < p_frame_count; i++)
		MCGImageRelease(p_frames[i].image);
	
	MCMemoryDeleteArray(p_frames);
}

uint32_t MCGImageFrameGetByteCount(const MCGImageFrame &p_frame)
{
	MCGRaster t_raster;
	if (!MCGImageGetRaster(p_frame.image, t_raster))
		return 0;
	
	return t_raster.height * t_raster.stride + sizeof(MCGRaster) + sizeof(MCGImageFrame);
}

uint32_t MCLoadableImageRep::GetFrameByteCount()
{
	if (m_frames == nil || m_frame_count == 0)
		return 0;

	return MCGImageFrameGetByteCount(m_frames[0]) * m_frame_count;
}

void MCLoadableImageRep::ReleaseFrames()
{
	if (m_lock_count > 0 || m_frames == nil)
		return;

	s_cache_size -= GetFrameByteCount();

	MCGImageFramesFree(m_frames, m_frame_count);
	m_frames = nil;
	
	MCImageFreeFrames(m_bitmap_frames, m_frame_count);
	m_bitmap_frames = nil;
}

////////////////////////////////////////////////////////////////////////////////

bool MCLoadableImageRep::GetGeometry(uindex_t &r_width, uindex_t &r_height)
{
	if (!EnsureHeader())
		return false;
	
	r_width = m_width;
	r_height = m_height;

	return true;
}

uindex_t MCLoadableImageRep::GetFrameCount()
{
	if (!EnsureHeader())
		return 0;
	
	return m_frame_count;
}

bool MCLoadableImageRep::GetFrameDuration(uindex_t p_index, uint32_t &r_duration)
{
	if (!EnsureFrameDurations())
		return false;
	
	if (p_index >= m_frame_count)
		return false;
	
	r_duration = m_frame_durations == nil ? 0 : m_frame_durations[p_index];
	return true;
}

////////////////////////////////////////////////////////////////////////////////

MCCachedImageRep *MCCachedImageRep::s_head = nil;
MCCachedImageRep *MCCachedImageRep::s_tail = nil;
uint32_t MCCachedImageRep::s_cache_size = 0;
uint32_t MCCachedImageRep::s_cache_limit = DEFAULT_IMAGE_REP_CACHE_SIZE;

MCCachedImageRep::~MCCachedImageRep()
{
	RemoveRep(this);
}

void MCCachedImageRep::FlushCache()
{
    //MCLog("MCImageRep::FlushCache() - %d bytes", s_cache_size);
	while (s_tail != nil)
	{
		s_tail->ReleaseFrames();
		
		s_tail = s_tail->m_prev;
	}
    //MCLog("%d bytes remaining", s_cache_size);
}

void MCCachedImageRep::FlushCacheToLimit()
{
	// allow tail to move forward - anything beyond the tail will have no frames
	// loaded or is locked & will move to head on unlock

    //MCLog("MCImageRep::FlushCacheToLimit() - %d bytes", s_cache_size);
	while (s_cache_size > s_cache_limit && s_tail != nil)
	{
		s_tail->ReleaseFrames();

		s_tail = s_tail->m_prev;
	}
    //MCLog("%d bytes remaining", s_cache_size);
}

void MCCachedImageRep::init()
{
	s_head = s_tail = nil;

	s_cache_size = 0;
	s_cache_limit = DEFAULT_IMAGE_REP_CACHE_SIZE;
}

bool MCCachedImageRep::FindWithKey(MCStringRef p_key, MCCachedImageRep *&r_rep)
{
	for (MCCachedImageRep *t_rep = s_head; t_rep != nil; t_rep = t_rep->m_next)
	{
		MCStringRef t_key;
		t_key = t_rep->GetSearchKey();
		if (t_key != nil && MCStringIsEqualTo(t_key, p_key, kMCStringOptionCompareExact))
		{
			r_rep = t_rep;
			return true;
		}
	}

	return false;
}

void MCCachedImageRep::AddRep(MCCachedImageRep *p_rep)
{
	if (s_head != nil)
		s_head->m_prev = p_rep;

	p_rep->m_next = s_head;
	p_rep->m_prev = nil;
	s_head = p_rep;

	if (s_tail == nil)
		s_tail = s_head;
}

void MCCachedImageRep::RemoveRep(MCCachedImageRep *p_rep)
{
	if (p_rep->m_next != nil)
		p_rep->m_next->m_prev = p_rep->m_prev;
	if (p_rep->m_prev != nil)
		p_rep->m_prev->m_next = p_rep->m_next;

	if (s_head == p_rep)
		s_head = p_rep->m_next;
	if (s_tail == p_rep)
		s_tail = p_rep->m_prev;
}

void MCCachedImageRep::MoveRepToHead(MCCachedImageRep *p_rep)
{
	if (p_rep != s_head)
	{
		RemoveRep(p_rep);
		AddRep(p_rep);
	}
}

////////////////////////////////////////////////////////////////////////////////

MCPixelDataImageRep::MCPixelDataImageRep(MCDataRef p_data, uint32_t p_width, uint32_t p_height, MCGPixelFormat p_format, bool p_premultiplied)
{
	m_pixel_data = MCValueRetain(p_data);
	m_pixel_width = p_width;
	m_pixel_height = p_height;
	m_pixel_format = p_format;
	m_pixels_premultiplied = p_premultiplied;
}

MCPixelDataImageRep::~MCPixelDataImageRep()
{
	MCValueRelease(m_pixel_data);
	m_pixel_data = nil;
}

uint32_t MCPixelDataImageRep::GetDataCompression()
{
	return F_RLE;
}

uindex_t MCPixelDataImageRep::GetFrameCount()
{
	return 1;
}

bool MCPixelDataImageRep::LoadImageFrames(MCBitmapFrame *&r_frames, uindex_t &r_frame_count, bool &r_premultiplied)
{
	bool t_success;
	t_success = true;
	
	MCBitmapFrame *t_frames;
	t_frames = nil;
	
	if (t_success)
		t_success = MCMemoryNew(t_frames);
	
	if (t_success)
		t_success = MCImageBitmapCreate(m_pixel_width, m_pixel_height, t_frames->image);
	
	if (t_success)
	{
		uint32_t t_pixel_count;
		t_pixel_count = MCMin(MCDataGetLength(m_pixel_data) / sizeof(uint32_t), m_pixel_width * m_pixel_height);
		
//		uint32_t t_pixel_width;
//		t_pixel_width = (t_pixel_count + m_pixel_height - 1) / m_pixel_height; // rounding up to nearest pixel
		
		uint32_t i = 0;
		
		uint32_t *t_src_ptr;
		t_src_ptr = (uint32_t*)MCDataGetBytePtr(m_pixel_data);
		
		uint8_t *t_dst_ptr;
		t_dst_ptr = (uint8_t*)t_frames->image->data;
		for (uint32_t y = 0; y < m_pixel_height; y++)
		{
			uint32_t *t_dst_pixel;
			t_dst_pixel = (uint32_t*)t_dst_ptr;
			
			for (uint32_t x = 0; x < m_pixel_width; x++)
			{
//				if (x < t_pixel_width && i < t_pixel_count)
				if (i < t_pixel_count)
					*t_dst_pixel++ = MCGPixelToNative(m_pixel_format, t_src_ptr[i++]);
				else
					*t_dst_pixel++ = MCGPixelPackNative(0, 0, 0, 1);
			}
			
			t_dst_ptr += t_frames->image->stride;
		}
	}
	
	if (t_success)
	{
		t_frames->duration = 0;
		t_frames->x_scale = 1.0;
		t_frames->y_scale = 1.0;
		
		r_frames = t_frames;
		r_frame_count = 1;
		r_premultiplied = m_pixels_premultiplied;
	}
	else
		MCImageFreeFrames(t_frames, 1);
	
	return t_success;
}

bool MCPixelDataImageRep::LoadHeader(uint32_t &r_width, uint32_t &r_height, uint32_t &r_frame_count)
{
	r_width = m_pixel_width;
	r_height = m_pixel_height;
	r_frame_count = 1;
	
	return true;
}

////////////////////////////////////////////////////////////////////////////////

bool MCImageRepCreateReferencedWithSearchKey(MCStringRef p_filename, MCStringRef p_searchkey, MCImageRep *&r_rep)
{
	bool t_success;
	t_success = true;
	
	MCReferencedImageRep *t_rep;
	t_rep = nil;
	
	if (t_success)
		t_success = nil != (t_rep = new (nothrow) MCReferencedImageRep(p_filename, p_searchkey));
	
	if (t_success)
	{
		MCCachedImageRep::AddRep(t_rep);
		r_rep = t_rep->Retain();
	}
	
	return t_success;
}

bool MCImageRepGetReferenced(MCStringRef p_filename, MCImageRep *&r_rep)
{
	MCCachedImageRep *t_rep = nil;
	
	if (MCCachedImageRep::FindWithKey(p_filename, t_rep))
	{
		r_rep = t_rep->Retain();
		return true;
	}
	
	return MCImageRepCreateReferencedWithSearchKey(p_filename, p_filename, r_rep);
}

////////////////////////////////////////////////////////////////////////////////

bool MCImageRepGetResident(const void *p_data, uindex_t p_size, MCImageRep *&r_rep)
{
	bool t_success = true;
	
	MCCachedImageRep *t_rep = new (nothrow) MCResidentImageRep(p_data, p_size);
	
	t_success = t_rep != nil;
	if (t_success)
	{
		MCCachedImageRep::AddRep(t_rep);
		r_rep = t_rep->Retain();
	}
	
	return t_success;
}

bool MCImageRepGetVector(const void *p_data, uindex_t p_size, MCImageRep *&r_rep)
{
	bool t_success = true;
	
	MCCachedImageRep *t_rep = new (nothrow) MCVectorImageRep(p_data, p_size);
	
	t_success = t_rep != nil;
	if (t_success)
	{
		MCCachedImageRep::AddRep(t_rep);
		r_rep = t_rep->Retain();
	}
	
	return t_success;
}

bool MCImageRepGetCompressed(MCImageCompressedBitmap *p_compressed, MCImageRep *&r_rep)
{
	bool t_success = true;
	
	MCCachedImageRep *t_rep = new (nothrow) MCCompressedImageRep(p_compressed);
	
	t_success = t_rep != nil;
	if (t_success)
	{
		MCCachedImageRep::AddRep(t_rep);
		r_rep = t_rep->Retain();
	}
	
	return t_success;
}

// IM-2013-11-05: [[ RefactorGraphics ]] Create new resampled image rep and add to the cache list
// IM-2014-07-23: [[ Bug 12842 ]] Modify resampled image rep to take a target width & height
// and explicit flip params instead of scale values.
bool MCImageRepGetResampled(uint32_t p_width, uint32_t p_height, bool p_flip_horizontal, bool p_flip_vertical, MCImageRep *p_source, MCImageRep *&r_rep)
{
	bool t_success = true;
	
	MCCachedImageRep *t_rep = new (nothrow) MCResampledImageRep(p_width, p_height, p_flip_horizontal, p_flip_vertical, p_source);
	
	t_success = t_rep != nil;
	if (t_success)
	{
		MCCachedImageRep::AddRep(t_rep);
		r_rep = t_rep->Retain();
	}
	
	return t_success;
	
}

bool MCImageRepGetPixelRep(MCDataRef p_pixel_data, uint32_t p_width, uint32_t p_height, MCGPixelFormat p_format, bool p_premultiplied, MCImageRep *&r_rep)
{
	bool t_success;
	t_success = true;
	
	MCPixelDataImageRep *t_rep;
	t_rep = new (nothrow) MCPixelDataImageRep(p_pixel_data, p_width, p_height, p_format, p_premultiplied);
	
	t_success = t_rep != nil;
	if (t_success)
	{
		MCCachedImageRep::AddRep(t_rep);
		r_rep = t_rep->Retain();
	}
	
	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

// ImageRep Function API

MCImageRep *MCImageRepRetain(MCImageRep *p_image_rep)
{
	if (p_image_rep == nil)
		return nil;
	
	return p_image_rep->Retain();
}

void MCImageRepRelease(MCImageRep *p_image_rep)
{
	if (p_image_rep == nil)
		return;
	
	p_image_rep->Release();
}

bool MCImageRepCreateWithPath(MCStringRef p_path, MCImageRep *&r_image_rep)
{
	return MCImageRepGetReferenced(p_path, r_image_rep);
}

bool MCImageRepCreateWithData(MCDataRef p_data, MCImageRep *&r_image_rep)
{
    if (MCDataGetLength(p_data) >= 3 &&
        memcmp(MCDataGetBytePtr(p_data), "LCD", 3) == 0)
    {
        return MCImageRepGetVector(MCDataGetBytePtr(p_data), MCDataGetLength(p_data), r_image_rep);
    }
    
	return MCImageRepGetResident(MCDataGetBytePtr(p_data), MCDataGetLength(p_data), r_image_rep);
}

bool MCImageRepCreateWithPixels(MCDataRef p_pixels, uint32_t p_width, uint32_t p_height, MCGPixelFormat p_format, bool p_premultiplied, MCImageRep *&r_image_rep)
{
	return MCImageRepGetPixelRep(p_pixels, p_width, p_height, p_format, p_premultiplied, r_image_rep);
}

bool MCImageRepGetGeometry(MCImageRep *p_image_rep, uint32_t &r_width, uint32_t &r_height)
{
	return p_image_rep->GetGeometry(r_width, r_height);
}

bool MCImageRepGetFrameDuration(MCImageRep *p_image_rep, uint32_t p_frame, uint32_t &r_duration)
{
	if (p_frame >= p_image_rep->GetFrameCount())
		return false;
	
	return p_image_rep->GetFrameDuration(p_frame, r_duration);
}

bool MCImageRepGetMetadata(MCImageRep *p_image_rep, MCArrayRef &r_metadata)
{
	MCImageMetadata t_metadata;
	if (!p_image_rep->GetMetadata(t_metadata))
		return false;
	
	MCAutoArrayRef t_metadata_array;
	if (!MCArrayCreateMutable(&t_metadata_array))
		return false;
	
	if (t_metadata.has_density)
	{
		MCAutoNumberRef t_density;
		if (!MCNumberCreateWithReal(t_metadata.density, &t_density))
			return false;
		if (!MCArrayStoreValue(*t_metadata_array, false, MCNAME("density"), *t_density))
			return false;
	}
	/* TODO - support other metadata fields */
	
	return MCArrayCopy(*t_metadata_array, r_metadata);
}

bool MCImageRepGetDensity(MCImageRep *p_image_rep, double &r_density)
{
	MCImageMetadata t_metadata;
	if (!p_image_rep->GetMetadata(t_metadata) || !t_metadata.has_density)
	{
		r_density = 72.0;
		return true;
	}
	
	r_density = t_metadata.density;
	return true;
}

bool MCImageRepLock(MCImageRep *p_image_rep, uint32_t p_index, MCGFloat p_density, MCGImageFrame &r_frame)
{
	return p_image_rep->LockImageFrame(p_index, p_density, r_frame);
}

void MCImageRepUnlock(MCImageRep *p_image_rep, uint32_t p_index, MCGImageFrame &p_frame)
{
	p_image_rep->UnlockImageFrame(p_index, p_frame);
}

bool MCImageRepLockRaster(MCImageRep *p_image_rep, uint32_t p_index, MCGFloat p_density, MCImageBitmap *&r_raster)
{
	return p_image_rep->LockBitmap(p_index, p_density, r_raster);
}

void MCImageRepUnlockRaster(MCImageRep *p_image_rep, uint32_t p_index, MCImageBitmap *p_raster)
{
	p_image_rep->UnlockBitmap(p_index, p_raster);
}

void MCImageRepRender(MCImageRep *p_image_rep, MCGContextRef p_gcontext, uint32_t p_index, MCGRectangle p_src_rect, MCGRectangle p_dst_rect, MCGImageFilter p_filter, MCGPaintRef p_current_color)
{
    if (p_image_rep->GetType() == kMCImageRepVector)
    {
        auto t_vector_rep = static_cast<MCVectorImageRep *>(p_image_rep);
        
        void* t_data;
        uindex_t t_data_size;
        t_vector_rep->GetData(t_data, t_data_size);
        
        MCGContextPlaybackRectOfDrawing(p_gcontext, MCMakeSpan((const byte_t*)t_data, t_data_size), p_src_rect, p_dst_rect, p_current_color);
    }
    else
    {
        MCGFloat t_scale;
        t_scale = MCGAffineTransformGetEffectiveScale(MCGContextGetDeviceTransform(p_gcontext));
        
        MCGImageFrame t_frame;
        if (MCImageRepLock(p_image_rep, p_index, t_scale, t_frame))
        {
            MCGAffineTransform t_transform;
            t_transform = MCGAffineTransformMakeScale(1.0 / t_frame.x_scale, 1.0 / t_frame.y_scale);
            
            MCGRectangle t_src_rect;
            t_src_rect = MCGRectangleScale(p_src_rect, t_frame.x_scale, t_frame.y_scale);
            
            MCGContextDrawRectOfImage(p_gcontext, t_frame.image, t_src_rect, p_dst_rect, p_filter);
            
            MCImageRepUnlock(p_image_rep, 0, t_frame);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////

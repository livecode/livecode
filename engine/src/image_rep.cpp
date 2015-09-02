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

////////////////////////////////////////////////////////////////////////////////

MCImageRep::MCImageRep()
{
	m_reference_count = 0;
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

////////////////////////////////////////////////////////////////////////////////

#ifdef _MOBILE
#define DEFAULT_IMAGE_REP_CACHE_SIZE (1024 * 1024 * 64)
#else
#define DEFAULT_IMAGE_REP_CACHE_SIZE (1024 * 1024 * 256)
#endif

MCLoadableImageRep::MCLoadableImageRep()
{
	m_lock_count = 0;
	m_have_geometry = false;

	m_frames = nil;
	m_locked_frames = nil;
	m_frame_count = 0;

	m_frames_premultiplied = false;
	
	m_next = m_prev = nil;
}

MCLoadableImageRep::~MCLoadableImageRep()
{
	ReleaseFrames();
}

////////////////////////////////////////////////////////////////////////////////

bool MCLoadableImageRep::ConvertToMCGFrames(MCBitmapFrame *&x_frames, uint32_t p_frame_count, bool p_premultiplied)
{
	bool t_success;
	t_success = true;
	
	MCGImageFrame *t_frames;
	t_frames = nil;
	
	if (t_success)
		t_success = MCMemoryNewArray(p_frame_count, t_frames);
	
	for (uint32_t i = 0; t_success && i < p_frame_count; i++)
	{
		// IM-2014-05-14: [[ ImageRepUpdate ]] Fix density & duration not being copied from the bitmap frames
		t_frames[i].x_scale = x_frames[i].x_scale;
		t_frames[i].y_scale = x_frames[i].y_scale;
		t_frames[i].duration = x_frames[i].duration;
		
		t_success = MCImageBitmapCopyAsMCGImageAndRelease(x_frames[i].image, p_premultiplied, t_frames[i].image);
	}
	
	if (t_success)
	{
		MCImageFreeFrames(x_frames, p_frame_count);
		x_frames = nil;

		m_frames = t_frames;
		m_frame_count = p_frame_count;
		m_frames_premultiplied = p_premultiplied;
		
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
		MCGImageFramesFree(t_frames, p_frame_count);
	
	return t_success;
}

bool MCLoadableImageRep::EnsureMCGImageFrames()
{
	// IM-2013-11-05: [[ RefactorGraphics ]] Rework to allow LoadImageFrames to return either
	// premultiplied or non-premultiplied bitmaps
	if (m_frames != nil)
		return true;
	
	bool t_success;
	t_success = true;
	
	MCBitmapFrame *t_frames;
	t_frames = nil;
	
	uint32_t t_frame_count;
    t_frame_count = 0;
	
	bool t_premultiplied;
	
	if (t_success)
		t_success = LoadImageFrames(t_frames, t_frame_count, t_premultiplied);
	
	if (t_success)
		t_success = ConvertToMCGFrames(t_frames, t_frame_count, t_premultiplied);
	
	MCImageFreeFrames(t_frames, t_frame_count);
	
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

bool convert_to_mcbitmapframes(MCGImageFrame *p_frames, uint32_t p_frame_count, MCBitmapFrame *&r_frames)
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
		}
	}
	
	if (t_success)
		r_frames = t_frames;
	else
		MCImageFreeFrames(t_frames, p_frame_count);
	
	return t_success;
}

bool MCLoadableImageRep::LockImageFrame(uindex_t p_frame, MCGFloat p_density, MCGImageFrame& r_frame)
{
	// frame index check
	if (m_frame_count != 0 && p_frame >= m_frame_count)
		return false;
	
    // MM-2014-07-31: [[ ThreadedRendering ]] Make sure only a single thread locks an image frame at a time.
    //  This could potentially be improved to be less obtrusive and resource hungry (mutex per image)
    
	if (m_frame_count != 0 && p_frame >= m_frame_count)
        return false;
    
	if (!EnsureMCGImageFrames())
		return false;
	
	if (p_frame >= m_frame_count)
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

bool MCLoadableImageRep::LockBitmapFrame(uindex_t p_frame, MCGFloat p_density, MCBitmapFrame *&r_frame)
{
	// frame index check
	if (m_frame_count != 0 && p_frame >= m_frame_count)
		return false;
	
	bool t_success;
	t_success = true;
	
	MCBitmapFrame *t_frames;
	t_frames = nil;
	
	uint32_t t_frame_count;
	bool t_premultiplied;

	// we want unpremultiplied bitmaps here, so if the source is unpremultiplied then load it
	if (m_frames == nil || !m_frames_premultiplied)
	{
		t_success = LoadImageFrames(t_frames, t_frame_count, t_premultiplied);

		if (t_success && t_premultiplied)
		{
			// the source frames are premultiplied, so we'll use them to construct our MCGImageFrames then convert to unpremultiplied
			t_success = ConvertToMCGFrames(t_frames, t_frame_count, t_premultiplied);
		}
	}
	
	// at this point, t_frames will either be filled or we will have m_frames to convert from
	if (t_success && t_frames == nil)
	{
		t_premultiplied = false;
		t_frame_count = m_frame_count;
		t_success = convert_to_mcbitmapframes(m_frames, m_frame_count, t_frames);
	}
	
	if (t_success)
	{
		// IM-2014-07-28: [[ Bug 13009 ]] If we have valid frames then store in locked frames and return requested frame.
		m_frame_count = t_frame_count;
		m_locked_frames = t_frames;
		t_frames = nil;
			
		Retain();
			
		r_frame = &m_locked_frames[p_frame];
	}
	
	MCImageFreeFrames(t_frames, t_frame_count);


	return t_success;
}

void MCLoadableImageRep::UnlockBitmapFrame(uindex_t p_index, MCBitmapFrame *p_frame)
{
	if (p_frame == nil)
		return;

	if (p_index >= m_frame_count)
		return;

	if (m_locked_frames == nil || &m_locked_frames[p_index] != p_frame)
		return;

	if (m_frames == nil)
		ConvertToMCGFrames(m_locked_frames, m_frame_count, false);
	
	// PM-2015-07-13: [[ Bug 15590 ]] Free the correct number of frames
	MCImageFreeFrames(m_locked_frames, m_frame_count);
	m_locked_frames = nil;
	
	Release();

	MoveRepToHead(this);
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
}

////////////////////////////////////////////////////////////////////////////////

bool MCLoadableImageRep::GetGeometry(uindex_t &r_width, uindex_t &r_height)
{
	if (!m_have_geometry)
	{
		// IM-2014-09-30: [[ Bug 13501 ]] CalculateGeometry is not thread-safe due to 
		//   possible geturl call.
		if (!m_have_geometry)
			m_have_geometry = CalculateGeometry(m_width, m_height);
	}
	
	if (m_have_geometry)
	{
		r_width = m_width;
		r_height = m_height;

		return true;
	}

	return false;
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
	MCLog("MCImageRep::FlushCache() - %d bytes", s_cache_size);
	while (s_tail != nil)
	{
		s_tail->ReleaseFrames();
		
		s_tail = s_tail->m_prev;
	}
	MCLog("%d bytes remaining", s_cache_size);
}

void MCCachedImageRep::FlushCacheToLimit()
{
	// allow tail to move forward - anything beyond the tail will have no frames
	// loaded or is locked & will move to head on unlock

	MCLog("MCImageRep::FlushCacheToLimit() - %d bytes", s_cache_size);
	while (s_cache_size > s_cache_limit && s_tail != nil)
	{
		s_tail->ReleaseFrames();

		s_tail = s_tail->m_prev;
	}
	MCLog("%d bytes remaining", s_cache_size);
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

bool MCImageRepCreateReferencedWithSearchKey(MCStringRef p_filename, MCStringRef p_searchkey, MCImageRep *&r_rep)
{
	bool t_success;
	t_success = true;
	
	MCReferencedImageRep *t_rep;
	t_rep = nil;
	
	if (t_success)
		t_success = nil != (t_rep = new MCReferencedImageRep(p_filename, p_searchkey));
	
	if (t_success)
	{
		MCCachedImageRep::AddRep(t_rep);
		r_rep = t_rep->Retain();
	}
	
	return t_success;
}

bool MCImageRepGetReferenced(MCStringRef p_filename, MCImageRep *&r_rep)
{
	bool t_success = true;
	
	MCCachedImageRep *t_rep = nil;
	
	if (MCCachedImageRep::FindWithKey(p_filename, t_rep))
	{
		MCLog("image rep cache hit for file %@", p_filename);
		r_rep = t_rep->Retain();
		return true;
	}
	
	return MCImageRepCreateReferencedWithSearchKey(p_filename, p_filename, r_rep);
}

////////////////////////////////////////////////////////////////////////////////

bool MCImageRepGetResident(void *p_data, uindex_t p_size, MCImageRep *&r_rep)
{
	bool t_success = true;
	
	MCCachedImageRep *t_rep = new MCResidentImageRep(p_data, p_size);
	
	t_success = t_rep != nil;
	if (t_success)
	{
		MCCachedImageRep::AddRep(t_rep);
		r_rep = t_rep->Retain();
	}
	
	return t_success;
}

bool MCImageRepGetVector(void *p_data, uindex_t p_size, MCImageRep *&r_rep)
{
	bool t_success = true;
	
	MCCachedImageRep *t_rep = new MCVectorImageRep(p_data, p_size);
	
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
	
	MCCachedImageRep *t_rep = new MCCompressedImageRep(p_compressed);
	
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
	
	MCCachedImageRep *t_rep = new MCResampledImageRep(p_width, p_height, p_flip_horizontal, p_flip_vertical, p_source);
	
	t_success = t_rep != nil;
	if (t_success)
	{
		MCCachedImageRep::AddRep(t_rep);
		r_rep = t_rep->Retain();
	}
	
	return t_success;
	
}

////////////////////////////////////////////////////////////////////////////////

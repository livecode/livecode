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
	m_reference_count++;
	return this;
}

void MCImageRep::Release()
{
	if (m_reference_count > 1)
	{
		m_reference_count--;
		return;
	}

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
	m_premultiplied = false;

	m_frames = nil;
	m_frame_count = 0;

	m_next = m_prev = nil;
}

MCLoadableImageRep::~MCLoadableImageRep()
{
	ReleaseFrames();
}

////////////////////////////////////////////////////////////////////////////////

uindex_t MCLoadableImageRep::GetFrameCount()
{
	if (!m_have_geometry)
	{
		if (!EnsureImageFrames(m_premultiplied))
			return false;
	}

	return m_frame_count;
}

void MCLoadableImageRep::PremultiplyFrames()
{
	if (m_premultiplied)
		return;

	for (uindex_t i = 0; i < m_frame_count; i++)
		MCImageBitmapPremultiply(m_frames[i].image);

	m_premultiplied = true;
}

void MCLoadableImageRep::UnpremultiplyFrames()
{
	if (!m_premultiplied)
		return;
	
	for (uindex_t i = 0; i < m_frame_count; i++)
		MCImageBitmapUnpremultiply(m_frames[i].image);
	
	m_premultiplied = false;
}

bool MCLoadableImageRep::EnsureImageFrames(bool p_premultiplied)
{
	// IM-2013-11-05: [[ RefactorGraphics ]] Rework to allow LoadImageFrames to return either
	// premultiplied or non-premultiplied bitmaps 
	if (m_frames != nil)
	{
		if (p_premultiplied == m_premultiplied)
			return true;
		
		if (m_premultiplied == m_frames_premultiplied)
		{
			if (p_premultiplied)
				PremultiplyFrames();
			else
				UnpremultiplyFrames();
			
			return true;
		}

		MCLog("<%p> discarding %s image frames", this, m_premultiplied ? "premultiplied" : "unpremultiplied");
		ReleaseFrames();
	}
	
	if (!LoadImageFrames(m_frames, m_frame_count, m_frames_premultiplied))
		return false;
	
	m_premultiplied = m_frames_premultiplied;
	
	if (p_premultiplied)
		PremultiplyFrames();
	else
		UnpremultiplyFrames();
	
	s_cache_size += GetFrameByteCount();
	
	if (s_cache_size > s_cache_limit)
	{
		m_lock_count++;
		FlushCacheToLimit();
		m_lock_count--;
	}
	
	return true;
}

bool MCLoadableImageRep::LockImageFrame(uindex_t p_frame, bool p_premultiplied, MCGFloat p_density, MCImageFrame *&r_frame)
{
	if (!EnsureImageFrames(p_premultiplied))
		return false;

	if (p_frame < m_frame_count)
	{
		m_lock_count++;
		//m_frame_locks[p_frame]++;
		Retain();

		r_frame = &m_frames[p_frame];

		return true;
	}

	return false;
}

void MCLoadableImageRep::UnlockImageFrame(uindex_t p_index, MCImageFrame *p_frame)
{
	if (p_frame == nil || m_lock_count == 0)
		return;

	if (p_index >= m_frame_count)
		return;

	//if (m_frame_locks == nil || m_frame_locks[p_index] == 0)
	//	return;
	if (m_frames == nil || &m_frames[p_index] != p_frame)
		return;

	m_lock_count--;
	//m_frame_locks[p_index]--;
	Release();

	MoveRepToHead(this);
}

////////////////////////////////////////////////////////////////////////////////

uint32_t MCLoadableImageRep::GetFrameByteCount()
{
	if (m_frames == nil || m_frame_count == 0)
		return 0;

	return (m_frames[0].image->height * m_frames[0].image->stride + sizeof(MCImageBitmap) + sizeof(MCImageFrame)) * m_frame_count;
}

void MCLoadableImageRep::ReleaseFrames()
{
	if (m_lock_count > 0 || m_frames == nil)
		return;

	s_cache_size -= GetFrameByteCount();

	MCImageFreeFrames(m_frames, m_frame_count);
	m_frames = nil;
	m_premultiplied = false;
}

////////////////////////////////////////////////////////////////////////////////

bool MCLoadableImageRep::GetGeometry(uindex_t &r_width, uindex_t &r_height)
{
	if (m_have_geometry || CalculateGeometry(m_width, m_height))
	{
		m_have_geometry = true;
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

bool MCCachedImageRep::FindWithKey(const char *p_key, MCCachedImageRep *&r_rep)
{
	for (MCCachedImageRep *t_rep = s_head; t_rep != nil; t_rep = t_rep->m_next)
	{
		const char *t_key;
		t_key = t_rep->GetSearchKey();
		if (t_key != nil && MCCStringEqual(t_key, p_key))
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

bool MCImageRepCreateReferencedWithSearchKey(const char *p_filename, const char *p_searchkey, MCImageRep *&r_rep)
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

bool MCImageRepGetReferenced(const char *p_filename, MCImageRep *&r_rep)
{
	bool t_success = true;
	
	MCCachedImageRep *t_rep = nil;
	
	if (MCCachedImageRep::FindWithKey(p_filename, t_rep))
	{
		MCLog("image rep cache hit for file %s", p_filename);
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
bool MCImageRepGetResampled(MCGFloat p_h_scale, MCGFloat p_v_scale, MCImageRep *p_source, MCImageRep *&r_rep)
{
	bool t_success = true;
	
	MCCachedImageRep *t_rep = new MCResampledImageRep(p_h_scale, p_v_scale, p_source);
	
	t_success = t_rep != nil;
	if (t_success)
	{
		MCCachedImageRep::AddRep(t_rep);
		r_rep = t_rep->Retain();
	}
	
	return t_success;
	
}

////////////////////////////////////////////////////////////////////////////////

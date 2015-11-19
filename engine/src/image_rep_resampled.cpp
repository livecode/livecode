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

#include "image.h"
#include "imagebitmap.h"
#include "image_rep.h"

#include "graphics_util.h"

////////////////////////////////////////////////////////////////////////////////

MCResampledImageRep::MCResampledImageRep(uint32_t p_width, uint32_t p_height, bool p_flip_horizontal, bool p_flip_vertical, MCImageRep *p_source)
{
	// IM-2013-11-07: [[ RefactorGraphics ]] If the size values are negative we need to flip the resulting resampled image
	m_target_width = p_width;
	m_target_height = p_height;
	m_h_flip = p_flip_horizontal;
	m_v_flip = p_flip_vertical;
	m_source = p_source->Retain();
}

MCResampledImageRep::~MCResampledImageRep()
{
	m_source->Release();
}

//////////

bool MCResampledImageRep::LoadHeader(uindex_t &r_width, uindex_t &r_height, uint32_t &r_frame_count)
{
	uint32_t t_frame_count;
	
	if (0 == (t_frame_count = m_source->GetFrameCount()))
		return false;
	
	r_width = m_target_width;
	r_height = m_target_height;
	r_frame_count = t_frame_count;
	
	return true;
}

bool MCResampledImageRep::LoadImageFrames(MCBitmapFrame *&r_frames, uindex_t &r_frame_count, bool &r_frames_premultiplied)
{
	uindex_t t_src_width, t_src_height;
	if (!m_source->GetGeometry(t_src_width, t_src_height))
		return false;
	
	bool t_success = true;
	
	MCBitmapFrame *t_frames = nil;
	uindex_t t_frame_count = 0;
	
	t_frame_count = m_source->GetFrameCount();
	
	// IM-2013-07-03: [[ Bug 11018 ]] fail here if the source has no frames
	t_success = t_frame_count != 0;

	if (t_success)
		t_success = MCMemoryNewArray(t_frame_count, t_frames);
	
	MCGFloat t_scale;
	t_scale = MCMax(m_target_width / (float)t_src_width, m_target_height / (float)t_src_height);
	
	for (uindex_t i = 0; t_success && i < t_frame_count; i++)
	{
		MCImageBitmap *t_src_bitmap;
		t_src_bitmap = nil;
		
		t_success = m_source->GetFrameDuration(i, t_frames[i].duration);
		
		if (t_success)
			t_success = m_source->LockBitmap(i, t_scale, t_src_bitmap);
		
		if (t_success)
		{
			t_success = MCImageScaleBitmap(t_src_bitmap, m_target_width, m_target_height, INTERPOLATION_BICUBIC, t_frames[i].image);
			if (t_success)
				MCImageFlipBitmapInPlace(t_frames[i].image, m_h_flip, m_v_flip);
			
			m_source->UnlockBitmap(i, t_src_bitmap);
		}
	}
	
	if (t_success)
	{
		r_frames = t_frames;
		r_frame_count = t_frame_count;
		r_frames_premultiplied = false;
	}
	else
		MCImageFreeFrames(t_frames, t_frame_count);
	
	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

bool MCResampledImageRep::Matches(uint32_t p_width, uint32_t p_height, bool p_flip_horizontal, bool p_flip_vertical, const MCImageRep *p_source)
{
	return m_source == p_source && m_target_width == p_width && m_target_height == p_height && m_h_flip == p_flip_horizontal && m_v_flip == p_flip_vertical;
}

////////////////////////////////////////////////////////////////////////////////

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

#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "core.h"

#include "image.h"
#include "imagebitmap.h"
#include "image_rep.h"

#include "graphics_util.h"

////////////////////////////////////////////////////////////////////////////////

MCResampledImageRep::MCResampledImageRep(MCGFloat p_h_scale, MCGFloat p_v_scale, MCImageRep *p_source)
{
	m_h_scale = p_h_scale;
	m_v_scale = p_v_scale;
	m_source = p_source->Retain();
}

MCResampledImageRep::~MCResampledImageRep()
{
	m_source->Release();
}

//////////

bool MCResampledImageRep::CalculateGeometry(uindex_t &r_width, uindex_t &r_height)
{
	uindex_t t_src_width, t_src_height;
	if (!m_source->GetGeometry(t_src_width, t_src_height))
		return false;
	
	MCGFloat t_density;
	t_density = m_source->GetDensity();
	
	MCGFloat t_h_scale, t_v_scale;
	t_h_scale = m_h_scale / t_density;
	t_v_scale = m_v_scale / t_density;
	
	r_width = ceilf(t_src_width * t_h_scale);
	r_height = ceilf(t_src_height * t_v_scale);
	
	return true;
}

bool MCResampledImageRep::LoadImageFrames(MCImageFrame *&r_frames, uindex_t &r_frame_count, bool &r_frames_premultiplied)
{
	uindex_t t_target_width, t_target_height;
	if (!GetGeometry(t_target_width, t_target_height))
		return false;
	
	bool t_success = true;
	
	MCImageFrame *t_frames = nil;
	uindex_t t_frame_count = 0;
	
	t_frame_count = m_source->GetFrameCount();
	
	// IM-2013-07-03: [[ Bug 11018 ]] fail here if the source has no frames
	t_success = t_frame_count != 0;

	if (t_success)
		t_success = MCMemoryNewArray(t_frame_count, t_frames);
	
	MCGFloat t_scale;
	t_scale = MCMax(m_h_scale, m_v_scale);
	
	for (uindex_t i = 0; t_success && i < t_frame_count; i++)
	{
		MCImageFrame *t_src_frame = nil;
		
		t_success = m_source->LockImageFrame(i, false, t_scale, t_src_frame);
		if (t_success)
		{
			t_frames[i].duration = t_src_frame->duration;
			
			t_success = MCImageScaleBitmap(t_src_frame->image, t_target_width, t_target_height, INTERPOLATION_BICUBIC, t_frames[i].image);
		}
		m_source->UnlockImageFrame(i, t_src_frame);
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

bool MCResampledImageRep::Matches(MCGFloat p_h_scale, MCGFloat p_v_scale, const MCImageRep *p_source)
{
	return m_source == p_source && m_h_scale == p_h_scale && m_v_scale == p_v_scale;
}

////////////////////////////////////////////////////////////////////////////////

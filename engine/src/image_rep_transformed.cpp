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

////////////////////////////////////////////////////////////////////////////////

MCTransformedImageRep::MCTransformedImageRep(uindex_t p_width, uindex_t p_height, int32_t p_angle, bool p_lock_rect, uint32_t p_quality, MCImageRep *p_source)
{
	m_width = p_width;
	m_height = p_height;
	m_angle = p_angle;
	m_lock_rect = p_lock_rect;
	
	m_quality = p_quality;
	
	m_source = p_source->Retain();
}

MCTransformedImageRep::~MCTransformedImageRep()
{
	m_source->Release();
}

//////////

bool MCTransformedImageRep::CalculateGeometry(uindex_t &r_width, uindex_t &r_height)
{
	if (m_lock_rect || m_angle == 0)
	{
		r_width = m_width;
		r_height = m_height;
		
		return true;
	}
	
	uindex_t t_src_width, t_src_height;
	if (!m_source->GetGeometry(t_src_width, t_src_height))
		return false;
	
	real8 t_angle = m_angle * M_PI / 180.0;
	real8 t_cos = cos(t_angle);
	real8 t_sin = sin(t_angle);
	
	r_width = (uindex_t)ceil(t_src_width * fabs(t_cos) + t_src_height * fabs(t_sin));
	r_height = (uindex_t)ceil(t_src_width * fabs(t_sin) + t_src_height * fabs(t_cos));
	
	return true;
}

bool MCTransformedImageRep::LoadImageFrames(MCImageFrame *&r_frames, uindex_t &r_frame_count)
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
	
	for (uindex_t i = 0; t_success && i < t_frame_count; i++)
	{
		MCImageFrame *t_src_frame = nil;
		
		t_success = m_source->LockImageFrame(i, t_src_frame);
		if (t_success)
		{
			t_frames[i].duration = t_src_frame->duration;
			if (m_angle != 0)
			{
				// rotate
				MCImageBitmap *t_bitmap = nil;
				MCImageBitmap *t_rotated = nil;
				t_success = MCImageCopyBitmap(t_src_frame->image, t_bitmap);
				if (t_success)
				{
					MCImageBitmapPremultiply(t_bitmap);
					t_success = MCImageRotateBitmap(t_bitmap, m_angle, m_quality, 0x0, t_rotated);
				}
				
				MCImageFreeBitmap(t_bitmap);
				
				bool t_scaled = false;
				
				if (t_success && (t_rotated->width != t_target_width || t_rotated->height != t_target_height))
				{
					MCImageBitmap *t_sbitmap = nil;
					t_success = MCImageScaleBitmap(t_rotated, t_target_width, t_target_height, m_quality, t_sbitmap);
					MCImageFreeBitmap(t_rotated);
					t_rotated = t_sbitmap;
					t_scaled = true;
				}
				
				if (t_success)
				{
					if (t_scaled && (m_quality == INTERPOLATION_BICUBIC))
						MCImageBitmapUnpremultiplyChecking(t_rotated);
					else
						MCImageBitmapUnpremultiply(t_rotated);
					
					t_frames[i].image = t_rotated;
				}
				else
					MCImageFreeBitmap(t_rotated);
				
			}
			else
			{
				// resize
				if (t_src_frame->image->width == t_target_width && t_src_frame->image->height == t_target_height)
					t_success = MCImageCopyBitmap(t_src_frame->image, t_frames[i].image);
				else
					t_success = MCImageScaleBitmap(t_src_frame->image, t_target_width, t_target_height, m_quality, t_frames[i].image);
			}
		}
		m_source->UnlockImageFrame(i, t_src_frame);
	}
	
	if (t_success)
	{
		r_frames = t_frames;
		r_frame_count = t_frame_count;
	}
	else
		MCImageFreeFrames(t_frames, t_frame_count);
	
	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

bool MCTransformedImageRep::Matches(uint32_t p_width, uint32_t p_height, int32_t p_angle, bool p_lock_rect, uint32_t p_quality, MCImageRep *p_source)
{
	if (p_source != m_source)
		return false;
	
	if (p_quality != m_quality)
		return false;
	
	if (p_angle != m_angle)
		return false;
	
	if (p_angle != 0 && !p_lock_rect)
		return true;
	
	return p_width == m_width && p_height == m_height;
}

////////////////////////////////////////////////////////////////////////////////

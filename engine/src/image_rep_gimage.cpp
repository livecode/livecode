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
#include "objdefs.h"

#include <graphics.h>

//#include "sysdefs.h"
#include "image.h"
//#include "imagebitmap.h"
#include "image_rep.h"

////////////////////////////////////////////////////////////////////////////////

// Image representation using MCGImageRef
class MCGImageImageRep : public MCImageRep
{
public:
	MCGImageImageRep(MCGImageRef p_image);
	virtual ~MCGImageImageRep();
	
	virtual MCImageRepType GetType();
	virtual uint32_t GetDataCompression();
	
	virtual uindex_t GetFrameCount();
	virtual bool GetFrameDuration(uindex_t p_index, uint32_t &r_duration);
	
	virtual bool LockBitmap(uindex_t p_index, MCGFloat p_density, MCImageBitmap *&r_bitmap);
	virtual void UnlockBitmap(uindex_t p_index, MCImageBitmap *p_bitmap);
	
	virtual bool LockImageFrame(uindex_t p_index, MCGFloat p_density, MCGImageFrame& r_frame);
	virtual void UnlockImageFrame(uindex_t p_index, MCGImageFrame& p_frame);
	
	virtual bool GetGeometry(uindex_t &r_width, uindex_t &r_height);
	
	//////////
	
	virtual bool GetMetadata(MCImageMetadata& r_metadata);
	
private:
	MCGImageFrame m_frame;
};

////////////////////////////////////////////////////////////////////////////////

MCGImageImageRep::MCGImageImageRep(MCGImageRef p_image)
{
	m_frame.x_scale = 1.0;
	m_frame.y_scale = 1.0;
	m_frame.image = MCGImageRetain(p_image);
}

MCGImageImageRep::~MCGImageImageRep()
{
	MCGImageRelease(m_frame.image);
	m_frame.image = nil;
}

MCImageRepType MCGImageImageRep::GetType()
{
	return kMCImageRepGImage;
}

uint32_t MCGImageImageRep::GetDataCompression()
{
	return F_RLE;
}

uindex_t MCGImageImageRep::GetFrameCount()
{
	return 1;
}

bool MCGImageImageRep::GetFrameDuration(uindex_t p_index, uint32_t &r_duration)
{
	if (p_index > 0)
		return false;
	
	r_duration = 0;
	return true;
}

bool MCGImageImageRep::LockBitmap(uindex_t p_index, MCGFloat p_density, MCImageBitmap *&r_bitmap)
{
	// TODO - implement (unpremultiplied) bitmap locking
	return false;
}

void MCGImageImageRep::UnlockBitmap(uindex_t p_index, MCImageBitmap *p_bitmap)
{
	// TODO - implement (unpremultiplied) bitmap locking
}

bool MCGImageImageRep::LockImageFrame(uindex_t p_index, MCGFloat p_density, MCGImageFrame &r_frame)
{
	if (p_index > 0)
		return false;
	
	Retain();
	r_frame = m_frame;
	return true;
}

void MCGImageImageRep::UnlockImageFrame(uindex_t p_index, MCGImageFrame &p_frame)
{
	if (p_index > 0)
		return;
	
	Release();
}

bool MCGImageImageRep::GetGeometry(uindex_t &r_width, uindex_t &r_height)
{
	r_width = MCGImageGetWidth(m_frame.image);
	r_height = MCGImageGetHeight(m_frame.image);
	
	return true;
}

bool MCGImageImageRep::GetMetadata(MCImageMetadata &r_metadata)
{
	MCMemoryClear(&r_metadata, sizeof(MCImageMetadata));
	return true;
}

////////////////////////////////////////////////////////////////////////////////

bool MCImageRepCreateWithGImage(MCGImageRef p_image, MCImageRep *&r_image_rep)
{
	MCGImageImageRep *t_rep;
	t_rep = new MCGImageImageRep(p_image);
	
	if (t_rep == nil)
		return false;
	
	r_image_rep = t_rep;
	return true;
}

////////////////////////////////////////////////////////////////////////////////

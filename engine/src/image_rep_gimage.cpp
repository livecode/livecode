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
	bool EnsureBitmap();
	void ReleaseBitmap();
	
	MCGImageFrame m_frame;
	MCImageBitmap *m_bitmap;
};

////////////////////////////////////////////////////////////////////////////////

MCGImageImageRep::MCGImageImageRep(MCGImageRef p_image) :
    m_bitmap(nil)
{
	m_frame.x_scale = 1.0;
	m_frame.y_scale = 1.0;
	m_frame.image = MCGImageRetain(p_image);
}

MCGImageImageRep::~MCGImageImageRep()
{
	ReleaseBitmap();
	
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
	if (p_index > 0)
		return false;
	
	if (!EnsureBitmap())
		return false;

	Retain();
	
	r_bitmap = m_bitmap;
	
	return true;
}

void MCGImageImageRep::UnlockBitmap(uindex_t p_index, MCImageBitmap *p_bitmap)
{
	if (p_index > 0)
		return;
	
	Release();
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

extern bool MCImageBitmapCreateWithPixels(void *p_pixels, uint32_t p_stride, uint32_t p_width, uint32_t p_height, MCImageBitmap *&r_bitmap);

bool MCGImageImageRep::EnsureBitmap()
{
	if (m_bitmap != nil)
		return true;
	
	bool t_success;
	t_success = true;
	
	MCGRaster t_raster;
	if (t_success)
		t_success = MCGImageGetRaster(m_frame.image, t_raster);
	
	MCImageBitmap *t_bitmap;
	t_bitmap = nil;
	
	if (t_success)
		t_success = MCImageBitmapCreateWithPixels(t_raster.pixels, t_raster.stride, t_raster.width, t_raster.height, t_bitmap);
	
	if (t_success)
	{
		t_bitmap->has_alpha = t_bitmap->has_transparency = t_raster.format == kMCGRasterFormat_ARGB;
		
		if (t_bitmap->has_transparency)
			MCImageBitmapUnpremultiply(t_bitmap);
	}
	
	if (t_success)
		m_bitmap = t_bitmap;
	
	return t_success;
}

void MCGImageImageRep::ReleaseBitmap()
{
	if (m_bitmap != nil)
		MCImageFreeBitmap(m_bitmap);
	m_bitmap = nil;
}

////////////////////////////////////////////////////////////////////////////////

bool MCImageRepCreateWithGImage(MCGImageRef p_image, MCImageRep *&r_image_rep)
{
	MCGImageImageRep *t_rep;
	t_rep = new (nothrow) MCGImageImageRep(p_image);
	
	if (t_rep == nil)
		return false;
	
	r_image_rep = t_rep;
	return true;
}

////////////////////////////////////////////////////////////////////////////////

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
#include "mcio.h"

#include "imagebitmap.h"
#include "imageloader.h"

////////////////////////////////////////////////////////////////////////////////

bool MCImageGetMetafileGeometry(IO_handle p_stream, uindex_t &r_width, uindex_t &r_height);

////////////////////////////////////////////////////////////////////////////////

MCImageLoader::MCImageLoader(IO_handle p_stream)
{
	m_stream = p_stream;
	m_valid = true;
	
	m_header_loaded = m_frames_loaded = false;
	
	m_frames = nil;
    
    MCMemoryClear(&m_metadata, sizeof(m_metadata));

    // AL-2014-09-29: [[ Bug 13353 ]] Initialize m_name to the empty string
    m_name = MCValueRetain(kMCEmptyString);
}

MCImageLoader::~MCImageLoader()
{
	MCImageFreeFrames(m_frames, m_frame_count);
    MCValueRelease(m_name);
}

bool MCImageLoader::GetGeometry(uint32_t &r_width, uint32_t &r_height)
{
	if (!EnsureHeader())
		return false;
	
	r_width = m_width;
	r_height = m_height;
	
	return true;
}

bool MCImageLoader::GetHotSpot(uint32_t &r_x, uint32_t &r_y)
{
	if (!EnsureHeader())
		return false;
	
	r_x = m_xhot;
	r_y = m_yhot;
	
	return true;
}

bool MCImageLoader::GetName(MCStringRef &r_name)
{
	if (!EnsureHeader())
		return false;
	
    // AL-2014-09-18: [[ Bug 13473 ]] Retain name, as it is released by the caller
	r_name = MCValueRetain(m_name);
	
	return true;
}

bool MCImageLoader::GetFrameCount(uint32_t &r_frame_count)
{
	if (!EnsureHeader())
		return false;
	
	r_frame_count = m_frame_count;
	
	return true;
}


// MERG 2014-09-12 [[ ImageMetadata ]] get metadata from loader
bool MCImageLoader::GetMetadata(MCImageMetadata &r_metadata)
{
	if (!EnsureHeader())
		return false;
	
	r_metadata = m_metadata;
	
	return true;
    
}

bool MCImageLoader::GetFrames(MCBitmapFrame *&r_frames, uint32_t &r_frame_count)
{
	if (!EnsureFrames())
		return false;
	
	r_frames = m_frames;
	r_frame_count = m_frame_count;
	
	return true;
}

bool MCImageLoader::TakeFrames(MCBitmapFrame *&r_frames, uint32_t &r_count)
{
	if (!EnsureFrames())
		return false;
	
	r_frames = m_frames;
	r_count = m_frame_count;
	
	m_frames = nil;
	m_frame_count = 0;
	
	return true;
}

////////////////////////////////////////////////////////////////////////////////

IO_handle MCImageLoader::GetStream()
{
	return m_stream;
}

////////////////////////////////////////////////////////////////////////////////

bool MCImageLoader::EnsureHeader()
{
	if (m_header_loaded)
		return true;
	
	if (!m_valid)
		return false;
	
    // AL-2014-09-29: [[ Bug 13353 ]] We initialize m_name to the empty string, so use MCValueAssign here.
    MCAutoStringRef t_name;
	m_valid = m_header_loaded = LoadHeader(m_width, m_height, m_xhot, m_yhot, &t_name, m_frame_count, m_metadata);
    
    if (m_valid)
        MCValueAssign(m_name, *t_name);
	
	return m_valid;
}

bool MCImageLoader::EnsureFrames()
{
	if (m_frames_loaded)
		return true;
	
	if (!EnsureHeader())
		return false;
	
	m_valid = m_frames_loaded = LoadFrames(m_frames, m_frame_count);
	
	return m_valid;
}

////////////////////////////////////////////////////////////////////////////////

bool MCImageLoader::IdentifyFormat(IO_handle p_stream, MCImageLoaderFormat &r_format)
{
	bool t_success = true;
	
	uindex_t t_width = 0, t_height = 0;
	
	uint8_t t_head[8];
	uindex_t t_size = 8;
	
	MCImageLoaderFormat t_format;
	
	if (t_success)
		t_success = MCS_readfixed(t_head, t_size, p_stream) == IO_NORMAL &&
		t_size == 8 && MCS_seek_cur(p_stream, -8) == IO_NORMAL;
	
	if (t_success)
	{
		if (memcmp(t_head, "GIF87a", 6) == 0)
			t_format = kMCImageFormatGIF;
		else if (memcmp(t_head, "GIF89a", 6) == 0)
			t_format = kMCImageFormatGIF;
		else if (memcmp(t_head, "\211PNG", 4) == 0)
			t_format = kMCImageFormatPNG;
		else if (memcmp(t_head, "\xff\xd8", 2) == 0)
			t_format = kMCImageFormatJPEG;
		else if (MCImageGetMetafileGeometry(p_stream, t_width, t_height))
			t_format = kMCImageFormatMetafile;
		else if (memcmp(t_head, "BM", 2) == 0)
			t_format = kMCImageFormatBMP;
		else if (memcmp(t_head, "#define", 7) == 0)
			t_format = kMCImageFormatXBM;
		else if (memcmp(t_head, "/* XPM", 6) == 0)
			t_format = kMCImageFormatXPM;
		else if (t_head[0] == 'P' && (t_head[1] >= '1' && t_head[1] <= '6'))
			t_format = kMCImageFormatNetPBM;
		else
			t_format = kMCImageFormatUnknown;
	}
	
	if (t_success)
		r_format = t_format;
	
	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

extern bool MCImageLoaderCreateForBMPStream(IO_handle p_stream, MCImageLoader *&r_loader);
extern bool MCImageLoaderCreateForGIFStream(IO_handle p_stream, MCImageLoader *&r_loader);
extern bool MCImageLoaderCreateForJPEGStream(IO_handle p_stream, MCImageLoader *&r_loader);
extern bool MCImageLoaderCreateForNetPBMStream(IO_handle p_stream, MCImageLoader *&r_loader);
extern bool MCImageLoaderCreateForPNGStream(IO_handle p_stream, MCImageLoader *&r_loader);
extern bool MCImageLoaderCreateForXBMStream(IO_handle p_stream, MCImageLoader *&r_loader);
extern bool MCImageLoaderCreateForXPMStream(IO_handle p_stream, MCImageLoader *&r_loader);
extern bool MCImageLoaderCreateForXWDStream(IO_handle p_stream, MCImageLoader *&r_loader);

bool MCImageLoader::LoaderForStreamWithFormat(IO_handle p_stream, MCImageLoaderFormat p_format, MCImageLoader *&r_loader)
{
	switch (p_format)
	{
		case kMCImageFormatBMP:
			return MCImageLoaderCreateForBMPStream(p_stream, r_loader);
			
		case kMCImageFormatGIF:
			return MCImageLoaderCreateForGIFStream(p_stream, r_loader);
			
		case kMCImageFormatJPEG:
			return MCImageLoaderCreateForJPEGStream(p_stream, r_loader);
			
		case kMCImageFormatNetPBM:
			return MCImageLoaderCreateForNetPBMStream(p_stream, r_loader);
			
		case kMCImageFormatPNG:
			return MCImageLoaderCreateForPNGStream(p_stream, r_loader);
			
		case kMCImageFormatXBM:
			return MCImageLoaderCreateForXBMStream(p_stream, r_loader);
			
		case kMCImageFormatXPM:
			return MCImageLoaderCreateForXPMStream(p_stream, r_loader);
			
			// If we don't recognise the format then we assume it's an XWD
		case kMCImageFormatUnknown:
		case kMCImageFormatXWD:
			return MCImageLoaderCreateForXWDStream(p_stream, r_loader);
			
		default:
			return false;
	}
}

bool MCImageLoader::LoaderForStream(IO_handle p_stream, MCImageLoader *&r_loader)
{
	MCImageLoaderFormat t_format;
	return MCImageLoader::IdentifyFormat(p_stream, t_format) &&
		MCImageLoader::LoaderForStreamWithFormat(p_stream, t_format, r_loader);
}

////////////////////////////////////////////////////////////////////////////////


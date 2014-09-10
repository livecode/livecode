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

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "mcio.h"
#include "context.h"
#include "securemode.h"

//#include "execpt.h"
#include "exec.h"
#include "util.h"
#include "stack.h"

#include "image.h"

#include "imageloader.h"

////////////////////////////////////////////////////////////////////////////////

MCEncodedImageRep::~MCEncodedImageRep()
{
}

// IM-2014-07-31: [[ ImageLoader ]] Use image loader class to read image frames
extern bool MCImageLoaderFormatToCompression(MCImageLoaderFormat p_format, uint32_t &r_compression);
bool MCEncodedImageRep::LoadImageFrames(MCBitmapFrame *&r_frames, uindex_t &r_frame_count, bool &r_frames_premultiplied)
{
	bool t_success = true;

	IO_handle t_stream = nil;

	MCImageLoader *t_loader;
	t_loader = nil;

	MCBitmapFrame *t_frames;
	uint32_t t_count;

	MCImageLoaderFormat t_format;

	if (t_success)
		t_success = GetDataStream(t_stream);

	if (t_success)
		t_success = MCImageLoader::LoaderForStream(t_stream, t_loader);
	
	if (t_success)
	{
		t_format = t_loader->GetFormat();
		t_success = t_loader->TakeFrames(t_frames, t_count);
			}
	
	if (t_loader != nil)
		delete t_loader;
	
	if (t_stream != nil)
		MCS_close(t_stream);

	if (t_success)
	{
		if (t_count == 1)
			t_frames[0].x_scale = t_frames[0].y_scale = 1.0;

		m_width = t_frames[0].image->width;
		m_height = t_frames[0].image->height;
		m_header_frame_count = t_count;

		/* UNCHECKED */ MCImageLoaderFormatToCompression(t_format, m_compression);

		m_have_geometry = true;
		
		r_frames = t_frames;
		r_frame_count = t_count;
		r_frames_premultiplied = false;
	}

	return t_success;
}
	
// IM-2014-08-01: [[ ImageLoader ]] The frame count is determined when reading the header
// during CalculateGeometry or LoadImageFrames
uindex_t MCEncodedImageRep::GetFrameCount()
{
	uint32_t t_width, t_height;
	if (m_have_geometry)
		return m_header_frame_count;

	if (GetGeometry(t_width, t_height))
		return m_header_frame_count;
	
	return 0;
}

// IM-2014-07-31: [[ ImageLoader ]] Use image loader class to read image geometry from stream header
bool MCEncodedImageRep::CalculateGeometry(uindex_t &r_width, uindex_t &r_height)
{
	bool t_success;
	t_success = true;

	IO_handle t_stream;
	t_stream = nil;

	MCImageLoader *t_loader;
	t_loader = nil;
	
	if (t_success)
		t_success = GetDataStream(t_stream);
	
	if (t_success)
		t_success = MCImageLoader::LoaderForStream(t_stream, t_loader);
	
	if (t_success)
	{
		/* UNCHECKED */ MCImageLoaderFormatToCompression(t_loader->GetFormat(), m_compression);
		t_success = t_loader->GetGeometry(r_width, r_height) && t_loader->GetFrameCount(m_header_frame_count);
	}
	
	if (t_loader != nil)
		delete t_loader;
	
	if (t_stream != nil)
		MCS_close(t_stream);

	return true;
}

// IM-2014-07-31: [[ ImageLoader ]] Use image loader method to identify stream format
uint32_t MCEncodedImageRep::GetDataCompression()
{
	if (m_have_geometry)
		return m_compression;

	IO_handle t_stream;
	t_stream = nil;
	
	MCImageLoaderFormat t_format;
	
	if (GetDataStream(t_stream) && MCImageLoader::IdentifyFormat(t_stream, t_format))
		/* UNCHECKED */ MCImageLoaderFormatToCompression(t_format, m_compression);

	if (t_stream != nil)
		MCS_close(t_stream);

	return m_compression;
}

////////////////////////////////////////////////////////////////////////////////

MCReferencedImageRep::MCReferencedImageRep(MCStringRef p_file_name, MCStringRef p_search_key)
{
	m_file_name = MCValueRetain(p_file_name);
	m_search_key = MCValueRetain(p_search_key);
	m_url_data = nil;
	
	// MW-2013-09-25: [[ Bug 10983 ]] No load has yet been attempted.
	m_url_load_attempted = false;
}

MCReferencedImageRep::~MCReferencedImageRep()
{
	MCValueRelease(m_file_name);
	MCValueRelease(m_search_key);

	MCMemoryDeallocate(m_url_data);
}

bool MCReferencedImageRep::GetDataStream(IO_handle &r_stream)
{
	IO_handle t_stream = nil;
	if (MCSecureModeCanAccessDisk())
		t_stream = MCS_open(m_file_name, kMCOpenFileModeRead, false, false, 0);
	
	// MW-2013-09-25: [[ Bug 10983 ]] Only ever try to load the rep as a url once.
	if (t_stream == nil && !m_url_load_attempted)
	{
		// MW-2013-09-25: [[ Bug 10983 ]] Mark the rep has having attempted url load.
		m_url_load_attempted = true;

        MCExecContext ctxt(MCdefaultstackptr, nil, nil);
        MCAutoValueRef t_data;
        MCU_geturl(ctxt, m_file_name, &t_data);
        if (ctxt.HasError() || MCValueIsEmpty(*t_data))
            return false;
        MCAutoDataRef t_dataref;
        /* UNCHECKED */ ctxt . ConvertToData(*t_data, &t_dataref);
        
        /* UNCHECKED */ MCMemoryAllocateCopy(MCDataGetBytePtr(*t_dataref), MCDataGetLength(*t_dataref), m_url_data);
        m_url_data_size = MCDataGetLength(*t_dataref);

        t_stream = MCS_fakeopen((const char *)m_url_data, m_url_data_size);
	}

	if (t_stream != nil)
		r_stream = t_stream;

	return t_stream != nil;
}

////////////////////////////////////////////////////////////////////////////////

MCResidentImageRep::MCResidentImageRep(const void *p_data, uindex_t p_size)
{
	/* UNCHECKED */ MCMemoryAllocateCopy(p_data, p_size, m_data);
	m_size = p_size;
}

MCResidentImageRep::~MCResidentImageRep()
{
	MCMemoryDeallocate(m_data);
}

bool MCResidentImageRep::GetDataStream(IO_handle &r_stream)
{
    r_stream = MCS_fakeopen((const char *)m_data, m_size);
	return r_stream != nil;
}

////////////////////////////////////////////////////////////////////////////////

MCVectorImageRep::MCVectorImageRep(const void *p_data, uindex_t p_size)
{
	/* UNCHECKED */ MCMemoryAllocateCopy(p_data, p_size, m_data);
	m_size = p_size;
}

MCVectorImageRep::~MCVectorImageRep()
{
	MCMemoryDeallocate(m_data);
}

bool MCVectorImageRep::Render(MCDC *p_context, bool p_embed, MCRectangle &p_image_rect, MCRectangle &p_clip_rect)
{
	p_context->drawpict((uint8_t*)m_data, m_size, p_embed, p_image_rect, p_clip_rect);
	return true;
}

bool MCVectorImageRep::LoadImageFrames(MCBitmapFrame *&r_frames, uindex_t &r_frame_count, bool &r_frames_premultiplied)
{
	/* OVERHAUL - REVISIT - should be able to render into an image context
	but we need to know the size of image required first */

	return false;
}

bool MCVectorImageRep::CalculateGeometry(uindex_t &r_width, uindex_t &r_height)
{
	bool t_success = true;
    MCAutoDataRef t_data;

	IO_handle t_stream = nil;
    if (t_success)
        t_success = nil != (t_stream = MCS_fakeopen((const char *)m_data, m_size));

	if (t_success)
		t_success = MCImageGetMetafileGeometry(t_stream, r_width, r_height);

	if (t_stream != nil)
		MCS_close(t_stream);

	return t_success;
}

uint32_t MCVectorImageRep::GetDataCompression()
{
	return F_PICT;
}

////////////////////////////////////////////////////////////////////////////////

MCCompressedImageRep::MCCompressedImageRep(MCImageCompressedBitmap *p_compressed)
{
	m_compressed = nil;
	/* UNCHECKED */ MCImageCopyCompressedBitmap(p_compressed, m_compressed);
}

MCCompressedImageRep::~MCCompressedImageRep()
{
	MCImageFreeCompressedBitmap(m_compressed);
}

bool MCCompressedImageRep::LoadImageFrames(MCBitmapFrame *&r_frames, uindex_t &r_frame_count, bool &r_frames_premultiplied)
{
	bool t_success = true;

	MCBitmapFrame *t_frame = nil;
	t_success = MCMemoryNewArray(1, t_frame);
	if (t_success)
		t_success = MCImageDecompressRLE(m_compressed, t_frame->image);

	if (t_success)
	{
		t_frame->x_scale = t_frame->y_scale = 1.0;
		
		r_frames = t_frame;
		r_frame_count = 1;
		r_frames_premultiplied = false;
	}
	else
		MCImageFreeFrames(t_frame, 1);

	return t_success;
}

bool MCCompressedImageRep::CalculateGeometry(uindex_t &r_width, uindex_t &r_height)
{
	r_width = m_compressed->width;
	r_height = m_compressed->height;

	return true;
}

uint32_t MCCompressedImageRep::GetDataCompression()
{
	return F_RLE;
}

////////////////////////////////////////////////////////////////////////////////

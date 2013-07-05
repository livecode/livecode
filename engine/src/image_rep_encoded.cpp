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
#include "mcio.h"
#include "context.h"
#include "securemode.h"

#include "execpt.h"
#include "util.h"
#include "stack.h"

#include "image.h"

////////////////////////////////////////////////////////////////////////////////

MCEncodedImageRep::~MCEncodedImageRep()
{
}

bool MCEncodedImageRep::LoadImageFrames(MCImageFrame *&r_frames, uindex_t &r_frame_count)
{
	bool t_success = true;

	// IM-2013-02-18 - switching this back to using MCImageImport as we need to
	// determine the compression type for m_compression

	IO_handle t_stream = nil;
	IO_handle t_mask_stream = nil;

	MCImageCompressedBitmap *t_compressed = nil;
	MCImageBitmap *t_bitmap = nil;

	MCPoint t_hotspot = {1, 1};
	char *t_name = nil;

	t_success = GetDataStream(t_stream) &&
		MCImageImport(t_stream, t_mask_stream, t_hotspot, t_name, t_compressed, t_bitmap);

	if (t_stream != nil)
		MCS_close(t_stream);

	if (t_success)
	{
		if (t_compressed != nil)
			t_success = MCImageDecompress(t_compressed, r_frames, r_frame_count);
		else
		{
			t_success = MCMemoryNewArray(1, r_frames);
			if (t_success)
			{
				r_frames[0].image = t_bitmap;
				t_bitmap = nil;
				r_frame_count = 1;
			}
		}
	}

	if (t_success)
	{

		m_width = r_frames[0].image->width;
		m_height = r_frames[0].image->height;

		if (t_compressed != nil)
			m_compression = t_compressed->compression;

		m_have_geometry = true;
	}

	MCCStringFree(t_name);
	
	MCImageFreeBitmap(t_bitmap);
	MCImageFreeCompressedBitmap(t_compressed);

	return t_success;
}

bool MCEncodedImageRep::CalculateGeometry(uindex_t &r_width, uindex_t &r_height)
{
	MCImageFrame *t_frame = nil;
	if (!LockImageFrame(0, m_premultiplied, t_frame))
		return false;

	r_width = t_frame->image->width;
	r_height = t_frame->image->height;

	UnlockImageFrame(0, t_frame);

	return true;
}

uint32_t MCEncodedImageRep::GetDataCompression()
{
	/* OVERHAUL - REVISIT - need to refactor image import code so we can detect
	image compression & geometry without reading whole file */

	if (m_have_geometry)
		return m_compression;

	MCImageFrame *t_frame = nil;
	if (LockImageFrame(0, m_premultiplied, t_frame))
		UnlockImageFrame(0, t_frame);

	return m_compression;
}

////////////////////////////////////////////////////////////////////////////////

MCReferencedImageRep::MCReferencedImageRep(const char *p_file_name)
{
	/* UNCHECKED */ MCCStringClone(p_file_name, m_file_name);
	m_url_data = nil;
	
	// MW-2013-09-25: [[ Bug 10983 ]] No load has yet been attempted.
	m_url_load_attempted = false;
}

MCReferencedImageRep::~MCReferencedImageRep()
{
	MCCStringFree(m_file_name);
	MCMemoryDeallocate(m_url_data);
}

bool MCReferencedImageRep::GetDataStream(IO_handle &r_stream)
{
	IO_handle t_stream = nil;
	if (MCSecureModeCanAccessDisk())
		t_stream = MCS_open(m_file_name, IO_READ_MODE, false, false, 0);
	
	// MW-2013-09-25: [[ Bug 10983 ]] Only ever try to load the rep as a url once.
	if (t_stream == nil && !m_url_load_attempted)
	{
		// MW-2013-09-25: [[ Bug 10983 ]] Mark the rep has having attempted url load.
		m_url_load_attempted = true;
		
		MCExecPoint ep(MCdefaultstackptr, nil, nil);
		ep.setsvalue(m_file_name);
		
		MCU_geturl(ep);
		
		if (ep.getsvalue().getlength() == 0)
			return false;

		/* UNCHECKED */ MCMemoryAllocateCopy(ep.getsvalue().getstring(), ep.getsvalue().getlength(), m_url_data);
		m_url_data_size = ep.getsvalue().getlength();

		t_stream = MCS_fakeopen(MCString((char*)m_url_data, m_url_data_size));
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
	r_stream = MCS_fakeopen(MCString((char*)m_data, m_size));
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

bool MCVectorImageRep::LoadImageFrames(MCImageFrame *&r_frames, uindex_t &r_frame_count)
{
	/* OVERHAUL - REVISIT - should be able to render into an image context
	but we need to know the size of image required first */

	return false;
}

bool MCVectorImageRep::CalculateGeometry(uindex_t &r_width, uindex_t &r_height)
{
	bool t_success = true;

	IO_handle t_stream = nil;
	t_success = nil != (t_stream = MCS_fakeopen(MCString((char*)m_data, m_size)));

	if (t_success)
		t_success = MCImageGetMetafileGeometry(t_stream, r_width, r_height);

	if (t_stream != nil)
		MCS_close(t_stream);

	return t_success;
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

bool MCCompressedImageRep::LoadImageFrames(MCImageFrame *&r_frames, uindex_t &r_frame_count)
{
	bool t_success = true;

	MCImageFrame *t_frame = nil;
	t_success = MCMemoryNewArray(1, t_frame);
	if (t_success)
		t_success = MCImageDecompressRLE(m_compressed, t_frame->image);

	if (t_success)
	{
		r_frames = t_frame;
		r_frame_count = 1;
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

////////////////////////////////////////////////////////////////////////////////

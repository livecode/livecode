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
#include "mcio.h"

#include "util.h"
#include "image.h"
#include "stack.h"
#include "sellst.h"


#include "globals.h"
#include "osspec.h"
#include "context.h"

#include "filepath.h"

#include "module-resources.h"

//////////////////////////////////////////////////////////////////////

// MW-2014-07-17: [[ ImageMetadata ]] Convert array to the metadata struct.
bool MCImageParseMetadata(MCExecContext& ctxt, MCArrayRef p_array, MCImageMetadata& r_metadata)
{
    MCValueRef t_value;
    real64_t t_density;
    
    if (MCArrayFetchValue(p_array, false, MCNAME("density"), t_value)
            && ctxt . ConvertToReal(t_value, t_density))
    {
        r_metadata . has_density = true;
        r_metadata . density = t_density;
    }
    else
        r_metadata . has_density = false;
    
    return true;
}

//////////////////////////////////////////////////////////////////////

bool MCImageCompress(MCImageBitmap *p_bitmap, bool p_dither, MCImageCompressedBitmap *&r_compressed)
{
	bool t_success = true;

	bool t_mask, t_alpha;

	t_mask = MCImageBitmapHasTransparency(p_bitmap, t_alpha);

	if (!t_alpha && MCpaintcompression == EX_PBM)
	{
		t_success = MCImageCompressRLE(p_bitmap, r_compressed);
	}
	else
	{
		uint32_t t_compression;
		void *t_buffer = nil;
		uindex_t t_size = 0;

		IO_handle t_stream = nil;
		t_success = nil != (t_stream = MCS_fakeopenwrite());
		if (t_success)
		{
			 if (MCpaintcompression == EX_GIF)
			 {
				 t_compression = F_GIF;
				 t_success = MCImageEncodeGIF(p_bitmap, t_stream, p_dither, t_size);
			 }
			 else if (MCpaintcompression == EX_JPEG)
			 {
				 t_compression = F_JPEG;
				 t_success = MCImageEncodeJPEG(p_bitmap, nil, t_stream, t_size);
			 }
			 else
			 {
				 t_compression = F_PNG;
				 t_success = MCImageEncodePNG(p_bitmap, nil, t_stream, t_size);
			 }
		}

		if (t_stream != nil)
			t_success = MCS_closetakingbuffer_uint32(t_stream, t_buffer, t_size) == IO_NORMAL;

		if (t_success)
			t_success = MCImageCreateCompressedBitmap(t_compression, r_compressed);

		if (t_success)
		{
			r_compressed->data = (uint8_t*)t_buffer;
			r_compressed->size = t_size;

			t_buffer = nil;
		}

		MCMemoryDelete(t_buffer);
	}

	return t_success;
}

void MCImage::recompress()
{
	if (!opened || !isediting())
		return;

	finishediting();

	state &= ~CS_EDITED;
	flags &= ~F_NEED_FIXING;
}

bool MCImage::decompressbrush(MCGImageRef &r_brush)
{
	/* TODO - may be able to improve this now by rasterizing the metafile */
	if (getcompression() == F_PICT || m_rep == nil)
	{
		return false;
		//r_mask = r_data = DNULL;
		//r_bitmap = MCscreen->createimage(1, 32, 32, False, 0x0, False, False);
		//return;
	}

	bool t_success = true;

	MCGImageFrame t_frame;

	// IM-2013-10-30: [[ FullscreenMode ]] REVISIT: We try here to acquire the brush
	// bitmap at 1.0 scale, but currently ignore the set density value of the returned frame.
	// We may have to add support for hi-res brushes OR scale the resulting bitmap appropriately.
	t_success = m_rep->LockImageFrame(0, 1.0, t_frame);

	if (t_success)
    {
		r_brush = MCGImageRetain(t_frame.image);

        m_rep->UnlockImageFrame(0, t_frame);
    }
    
	return t_success;
}

void MCImagePrepareRepForDisplayAtDensity(MCImageRep *p_rep, MCGFloat p_density)
{
	if (p_rep != nil)
	{
		/* OVERHAUL - REVISIT: for the moment, prepared images are premultiplied
		 * as we expect them to be rendered, but if not then this is actually
		 * causing more work to be done than needed */
		MCGImageFrame t_frame;
		if (p_rep->LockImageFrame(0, p_density, t_frame))
            p_rep->UnlockImageFrame(0, t_frame);
	}
}

void MCImage::prepareimage()
{
	MCImagePrepareRepForDisplayAtDensity(m_rep, getdevicescale());
}

void MCImage::openimage()
{
	uindex_t t_width, t_height;
	if (!m_image_opened && m_rep != nil && m_rep->GetGeometry(t_width, t_height))
	{
		// MW-2013-06-21: [[ Valgrind ]] Initialize width/height to defaults in
		//   case GetGeometry fails.
		t_width = rect . width;
		t_height = rect . height;
		
		/* UNCHECKED */ getsourcegeometry(t_width, t_height);
		
		// On open, set the width & height to the formatted width & height
		MCRectangle t_old_rect;
		t_old_rect = rect;
		if (t_width != rect.width || t_height != rect.height)
		{
			if ((flags & F_LOCK_LOCATION) == 0)
			{
				rect.x += (rect.width - t_width) >> 1;
				rect.width = t_width;
				rect.y += (rect.height - t_height) >> 1;
				rect.height = t_height;
			}
		}

		// IM-2013-11-21: [[ Bug 11486 ]] Apply the transform now so the rect is adjusted for rotation.
		apply_transform();
		
		// IM-2013-02-05: preload image data for buffered images
		if (getflag(F_I_ALWAYS_BUFFER) || MCbufferimages)
			prepareimage();

		// MW-2011-09-12: [[ Redraw ]] If the rect has changed then notify the layer.
		//   (note we have to check 'parent' as at the moment MCImage is used for
		//    the rb* icons which are unowned!).
		if (parent && !MCU_equal_rect(t_old_rect, rect))
			layer_rectchanged(t_old_rect, false);

		if (m_rep->GetFrameCount() > 1)
		{
			irepeatcount = repeatcount;
			state |= CS_DO_START;
		}
	}

	m_image_opened++;
}

void MCImage::closeimage()
{
	if (!m_image_opened)
		return;

	// cancel any internal messages for this object (used for GIF animation)
	MCscreen->cancelmessageobject(this, MCM_internal);

	recompress();

	m_image_opened--;
}

bool MCU_israwimageformat(Export_format p_format)
{
	return p_format == EX_RAW || p_format == EX_RAW_ARGB || p_format == EX_RAW_ABGR || \
		p_format == EX_RAW_RGBA || p_format == EX_RAW_BGRA || p_format == EX_RAW_INDEXED;
}

bool MCImageEncode(MCImageBitmap *p_bitmap, Export_format p_format, bool p_dither, MCImageMetadata *p_metadata, IO_handle p_stream, uindex_t &r_bytes_written)
{
	bool t_success = true;
	switch (p_format)
	{
	case EX_JPEG:
		t_success = MCImageEncodeJPEG(p_bitmap, p_metadata, p_stream, r_bytes_written);
		break;

	case EX_PNG:
		t_success = MCImageEncodePNG(p_bitmap, p_metadata, p_stream, r_bytes_written);
		break;

	case EX_GIF:
		t_success = MCImageEncodeGIF(p_bitmap, p_stream, p_dither, r_bytes_written);
		break;

	case EX_BMP:
		t_success = MCImageEncodeBMP(p_bitmap, p_stream, r_bytes_written);
		break;

	case EX_RAW_ARGB:
	case EX_RAW_ABGR:
	case EX_RAW_RGBA:
	case EX_RAW_BGRA:
		t_success = MCImageEncodeRawTrueColor(p_bitmap, p_stream, p_format, r_bytes_written);
		break;

	case EX_RAW_INDEXED:
		t_success = MCImageEncodeRawIndexed(p_bitmap, p_stream, r_bytes_written);
		break;

	default:
		t_success = MCImageEncodePPM(p_bitmap, p_stream, r_bytes_written);
	}

	return t_success;
}

bool MCImageEncode(MCImageIndexedBitmap *p_indexed, Export_format p_format, MCImageMetadata *p_metadata, IO_handle p_stream, uindex_t &r_bytes_written)
{
	bool t_success = true;
	switch (p_format)
	{
	case EX_PNG:
		t_success = MCImageEncodePNG(p_indexed, p_metadata, p_stream, r_bytes_written);
		break;

	case EX_GIF:
		t_success = MCImageEncodeGIF(p_indexed, p_stream, r_bytes_written);
		break;

	case EX_RAW_INDEXED:
		t_success = MCImageEncodeRawIndexed(p_indexed, p_stream, r_bytes_written);
		break;

	default:
		{
			MCImageBitmap *t_bitmap = nil;
			t_success = MCImageConvertIndexedToBitmap(p_indexed, t_bitmap);
			if (t_success)
				t_success = MCImageEncode(t_bitmap, p_format, false, p_metadata, p_stream, r_bytes_written);
			MCImageFreeBitmap(t_bitmap);
		}
	}

	return t_success;
}

bool MCImageExport(MCImageBitmap *p_bitmap, Export_format p_format, MCImagePaletteSettings *p_palette_settings, bool p_dither, MCImageMetadata *p_metadata, IO_handle p_stream, IO_handle p_mstream)
{
	bool t_success = true;

	MCImageIndexedBitmap *t_indexed = nil;
	uint32_t t_size, t_mask_size;

	if (t_success)
	{
		if (p_palette_settings != nil && p_palette_settings->type != kMCImagePaletteTypeEmpty)
		{
			t_success = MCImageQuantizeColors(p_bitmap, p_palette_settings, p_dither, p_format == EX_GIF || p_format == EX_PNG, t_indexed);
			if (t_success)
				t_success = MCImageEncode(t_indexed, p_format, p_metadata, p_stream, t_size);
		}
		else
			t_success = MCImageEncode(p_bitmap, p_format, p_dither, p_metadata, p_stream, t_size);
	}

	if (t_success && p_mstream != nil)
		t_success = MCImageEncodePBM(p_bitmap, p_mstream, t_mask_size);

	MCImageFreeIndexedBitmap(t_indexed);

	return t_success;
}

void MCImage::reopen(bool p_newfile, bool p_lock_size)
{
	if (!opened)
		return;
	uint4 oldstate = state & (CS_NO_MESSAGES | CS_SELECTED
	                          | CS_MFOCUSED | CS_KFOCUSED);
	MCControlHandle oldfocused = focused;

	state &= ~CS_SELECTED;
	uint2 opencount = opened;
	
	// MW-2011-10-01: [[ Bug 9777 ]] Make sure we don't clobber layer attrs.
	if (m_layer_id != 0)
		state |= CS_KEEP_LAYER;
	
	while (opened)
		close();

	if (p_newfile)
		flags &= ~(F_COMPRESSION | F_TRUE_COLOR);
	Boolean was = (flags & F_I_ALWAYS_BUFFER) != 0;
	flags |= F_I_ALWAYS_BUFFER;
	MCRectangle orect = rect;
	uint32_t oldflags = flags;
	if (p_lock_size)
		flags |= F_LOCK_LOCATION;
	while (opencount--)
		open();
	
	// MW-2011-10-01: [[ Bug 9777 ]] Turn off anti-layer clobbering state.
	if (m_layer_id != 0)
		state &= ~CS_KEEP_LAYER;
	
	flags = oldflags;
	if (!was)
		flags &= ~F_I_ALWAYS_BUFFER;

	state |= oldstate;

	if (oldfocused.IsBoundTo(this))
		focused = this;

	// MW-2011-08-17: [[ Layers ]] Notify of the change in rect and invalidate.
	// MW-2012-04-02: [[ Bug 10144 ]] Crash when pasting into an image - methods called on
	//   image with no parent, so check parent != nil.
	if (parent && !resizeparent())
		layer_rectchanged(orect, true);
}

////////////////////////////////////////////////////////////////////////////////

bool MCImageGetRepForReferenceWithStackContext(MCStringRef p_reference, MCStack *p_stack, MCImageRep *&r_rep)
{
	bool t_success = true;
	
	MCImageRep *t_rep;
	t_rep = nil;
	
    MCAutoStringRef t_prefixless;
	// skip over any file: / binfile: url prefix
	if (MCStringBeginsWith(p_reference, MCSTR("file:"), kMCStringOptionCompareCaseless))
    {
		MCStringCopySubstring(p_reference, MCRangeMake(5, UINDEX_MAX), &t_prefixless);
        p_reference = *t_prefixless;
    }
	else if (MCStringBeginsWith(p_reference, MCSTR("binfile:"), kMCStringOptionCompareCaseless))
    {
		MCStringCopySubstring(p_reference, MCRangeMake(8, UINDEX_MAX), &t_prefixless);
        p_reference = *t_prefixless;
    }

	if (MCPathIsRemoteURL(p_reference))
		t_success = MCImageRepGetReferenced(p_reference, t_rep);
	else
		t_success = MCImageGetRepForFileWithStackContext(p_reference, p_stack, t_rep);
	
	if (t_success)
		r_rep = t_rep;
	
	return t_success;
}

bool MCImageGetRepForFileWithStackContext(MCStringRef p_filename, MCStack *p_stack, MCImageRep *&r_rep)
{
	bool t_success;
	t_success = true;
	
	MCImageRep *t_rep;
	t_rep = nil;
	
	// with local filenames, first check if absolute path
	if (MCPathIsAbsolute(p_filename))
		t_success = MCImageRepGetDensityMapped(p_filename, t_rep);
	else
	{
		// else try to resolve from stack file location
		MCAutoStringRef t_path;
		t_success = p_stack->resolve_relative_path(p_filename, &t_path);
		if (t_success && MCS_exists(*t_path, True))
			t_success = MCImageRepGetDensityMapped(*t_path, t_rep);
		
		// else try to resolve from current folder
		if (t_success && t_rep == nil)
		{
			MCAutoStringRef t_resolved;
			t_success = MCS_resolvepath(p_filename, &t_resolved);
			if (t_success)
				t_success = MCImageRepGetDensityMapped(*t_resolved, t_rep);
		}
	}
	
	if (t_success)
		r_rep = t_rep;
	
	return t_success;
}

bool MCImageGetRepForResource(MCStringRef p_resource_file, MCImageRep *&r_rep)
{
	MCAutoStringRef t_path;
	if (!MCResourceResolvePath(p_resource_file, &t_path))
		return false;
	
	return MCImageRepGetDensityMapped(*t_path, r_rep);
}

////////////////////////////////////////////////////////////////////////////////

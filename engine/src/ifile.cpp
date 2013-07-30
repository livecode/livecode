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

#include "util.h"
#include "image.h"
#include "stack.h"
#include "sellst.h"
#include "execpt.h"

#include "globals.h"
#include "osspec.h"
#include "context.h"

#include "core.h"

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
		char *t_buffer = nil;
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
				 t_success = MCImageEncodeJPEG(p_bitmap, t_stream, t_size);
			 }
			 else
			 {
				 t_compression = F_PNG;
				 t_success = MCImageEncodePNG(p_bitmap, t_stream, t_size);
			 }
		}
		if (t_stream != nil)
			MCS_fakeclosewrite(t_stream, t_buffer, t_size);

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

bool MCImageDecompress(MCImageCompressedBitmap *p_compressed, MCImageFrame *&r_frames, uindex_t &r_frame_count)
{
	bool t_success = true;

	IO_handle t_stream = nil;
	MCImageFrame *t_frames = nil;
	uindex_t t_frame_count = 1;

	// create single frame for non-animated formats
	if (t_success && p_compressed->compression != F_GIF)
		t_success = MCMemoryNewArray(1, t_frames);

	// create stream for non-rle compressed images
	if (t_success && p_compressed->compression != F_RLE)
		t_success = nil != (t_stream = MCS_fakeopen(MCString((char*)p_compressed->data, p_compressed->size)));

	if (t_success)
	{
		switch (p_compressed->compression)
		{
		case F_GIF:
			t_success = MCImageDecodeGIF(t_stream, t_frames, t_frame_count);
			break;

		case F_PNG:
			t_success = MCImageDecodePNG(t_stream, t_frames[0].image);
			break;

		case F_JPEG:
			t_success = MCImageDecodeJPEG(t_stream, t_frames[0].image);
			break;

		case F_RLE:
			t_success = MCImageDecompressRLE(p_compressed, t_frames[0].image);
			break;
		}
	}

	if (t_stream != nil)
		MCS_close(t_stream);

	if (t_success)
	{
		r_frames = t_frames;
		r_frame_count = t_frame_count;
	}
	else
		MCImageFreeFrames(t_frames, t_frame_count);

	return t_success;
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

	MCImageFrame *t_frame = nil;

	t_success = m_rep->LockImageFrame(0, true, t_frame);

	if (t_success)
	{
		MCGRaster t_raster;
		t_raster.width = t_frame->image->width;
		t_raster.height = t_frame->image->height;
		t_raster.stride = t_frame->image->stride;
		t_raster.format = MCImageBitmapHasTransparency(t_frame->image) ? kMCGRasterFormat_ARGB : kMCGRasterFormat_xRGB;
		t_raster.pixels = t_frame->image->data;

		t_success = MCGImageCreateWithRaster(t_raster, r_brush);
	}

	m_rep->UnlockImageFrame(0, t_frame);

	return t_success;
}

void MCImage::prepareimage()
{
	if (m_rep != nil)
	{
		/* OVERHAUL - REVISIT: for the moment, prepared images are premultiplied
		 * as we expect them to be rendered, but if not then this is actually
		 * causing more work to be done than needed */
		MCImageFrame *t_frame = nil;
		m_rep->LockImageFrame(0, true, t_frame);
		m_rep->UnlockImageFrame(0, t_frame);
	}
}

void MCImage::openimage()
{
	if (!m_image_opened && m_rep != nil)
	{
		// MW-2013-06-21: [[ Valgrind ]] Initialize width/height to defaults in
		//   case GetGeometry fails.
		uindex_t t_width, t_height;
		t_width = rect . width;
		t_height = rect . height;
		
		/* UNCHECKED */ getsourcegeometry(t_width, t_height);
		
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

		// IM-2013-02-05: preload image data for buffered images
		if (getflag(F_I_ALWAYS_BUFFER) || MCbufferimages)
			prepareimage();

		// MW-2011-09-12: [[ Redraw ]] If the rect has changed then notify the layer.
		//   (note we have to check 'parent' as at the moment MCImage is used for
		//    the rb* icons which are unowned!).
		if (parent != nil && !MCU_equal_rect(t_old_rect, rect))
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

bool MCImageEncode(MCImageBitmap *p_bitmap, Export_format p_format, bool p_dither, IO_handle p_stream, uindex_t &r_bytes_written)
{
	bool t_success = true;
	switch (p_format)
	{
	case EX_JPEG:
		t_success = MCImageEncodeJPEG(p_bitmap, p_stream, r_bytes_written);
		break;

	case EX_PNG:
		t_success = MCImageEncodePNG(p_bitmap, p_stream, r_bytes_written);
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

bool MCImageEncode(MCImageIndexedBitmap *p_indexed, Export_format p_format, IO_handle p_stream, uindex_t &r_bytes_written)
{
	bool t_success = true;
	switch (p_format)
	{
	case EX_PNG:
		t_success = MCImageEncodePNG(p_indexed, p_stream, r_bytes_written);
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
				t_success = MCImageEncode(t_bitmap, p_format, false, p_stream, r_bytes_written);
			MCImageFreeBitmap(t_bitmap);
		}
	}

	return t_success;
}

bool MCImageExport(MCImageBitmap *p_bitmap, Export_format p_format, MCImagePaletteSettings *p_palette_settings, bool p_dither, IO_handle p_stream, IO_handle p_mstream)
{
	bool t_success = true;

	MCImageIndexedBitmap *t_indexed = nil;
	uint32_t t_size, t_mask_size;

	if (t_success)
	{
		if (p_palette_settings != nil && p_palette_settings->type != kMCImagePaletteTypeEmpty)
		{
			t_success = MCImageQuantizeColors(p_bitmap, p_palette_settings, p_dither, p_format == F_GIF || p_format == F_PNG, t_indexed);
			if (t_success)
				t_success = MCImageEncode(t_indexed, p_format, p_stream, t_size);
		}
		else
			t_success = MCImageEncode(p_bitmap, p_format, p_dither, p_stream, t_size);
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
	MCControl *oldfocused = focused;

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

	if (oldfocused == this)
		focused = this;

	// MW-2011-08-17: [[ Layers ]] Notify of the change in rect and invalidate.
	// MW-2012-04-02: [[ Bug 10144 ]] Crash when pasting into an image - methods called on
	//   image with no parent, so check parent != nil.
	if (parent != nil && !resizeparent())
		layer_rectchanged(orect, true);
}

////////////////////////////////////////////////////////////////////////////////
// IM-2013-07-30: [[ ResIndependence ]] Density-mapped Image Support

typedef struct _MCImageScaleLabels_t
{
	const char *label;
	MCGFloat scale;
} MCImageScaleLabel;

static MCImageScaleLabel s_image_scale_labels[] = {
	{"@ultra-low", 0.25},
	{"@extra-low", 0.5},
	{"@low", 0.75},
	{"@medium", 1.0},
	{"@high", 1.5},
	{"@extra-high", 2.0},
	{"@ultra-high", 4.0},
	
	// unlabeled default scale
	{"", 1.0},
	
	{nil, 0.0}
};

bool MCImageGetScaleForLabel(const char *p_label, uint32_t p_length, MCGFloat &r_scale)
{
	MCImageScaleLabel *t_scale_label;
	t_scale_label = s_image_scale_labels;
	
	while (t_scale_label->label != nil)
	{
		if (MCCStringEqualSubstring(p_label, t_scale_label->label, p_length))
		{
			r_scale = t_scale_label->scale;
			return true;
		}
		t_scale_label++;
	}
	
	return false;
}

void MCImageFreeScaledFileList(MCImageScaledFile *p_list, uint32_t p_count)
{
	if (p_list == nil)
		return;
	
	for (uint32_t i = 0; i < p_count; i++)
		MCCStringFree(p_list[i].filename);
	
	MCMemoryDeleteArray(p_list);
}

bool MCImageSplitScaledFilename(const char *p_filename, char *&r_base, char *&r_extension, bool &r_has_scale, MCGFloat &r_scale)
{
	if (p_filename == nil)
		return false;
	
	bool t_success;
	t_success = true;
	
	MCGFloat t_scale;
	bool t_has_scale = false;
	
	uint32_t t_length;
	t_length = MCCStringLength(p_filename);
	
	uint32_t t_index, t_name_start, t_label_start, t_label_search_start, t_ext_start;
	
	if (MCCStringLastIndexOf(p_filename, '/', t_index))
		t_name_start = t_index + 1;
	else
		t_name_start = 0;
	
	if (MCCStringLastIndexOf(p_filename + t_name_start, '.', t_index))
		t_ext_start = t_name_start + t_index;
	else
		t_ext_start = t_length;
	
	// find first '@' char before the extension part
	t_label_start = t_label_search_start = t_name_start;
	while (MCCStringFirstIndexOf(p_filename + t_label_search_start, '@', t_index))
	{
		if (t_label_start + t_index > t_ext_start)
			break;
		
		t_label_start += t_index;
		t_label_search_start = t_label_start + 1;
	}
	
	// check label begins with '@'
	if (p_filename[t_label_start] != '@')
	{
		// no scale label
		t_label_start = t_ext_start;
	}
	else
	{
		t_has_scale = MCImageGetScaleForLabel(p_filename + t_label_start, t_ext_start - t_label_start, t_scale);
		
		if (!t_has_scale)
		{
			// @... is not a recognised scale
			t_label_start = t_ext_start;
		}
	}
	
	char *t_base, *t_extension;
	t_base = t_extension = nil;
	
	t_success = MCCStringCloneSubstring(p_filename, t_label_start, t_base) && MCCStringCloneSubstring(p_filename + t_ext_start, t_length - t_ext_start, t_extension);
	
	if (t_success)
	{
		r_base = t_base;
		r_extension = t_extension;
		r_has_scale = t_has_scale;
		r_scale = t_has_scale ? t_scale : 1.0;
	}
	else
	{
		MCCStringFree(t_base);
		MCCStringFree(t_extension);
	}
	
	return t_success;
}

bool MCImageGetScaledFiles(const char *p_filename, MCStack *p_stack, MCImageScaledFile *&r_list, uint32_t &r_count)
{
	bool t_success;
	t_success = nil;
	
	MCImageScaledFile *t_filelist;
	t_filelist = nil;
	
	char *t_base, *t_extension, *t_scale_label;
	t_base = t_extension = t_scale_label = nil;
	
	uint32_t t_count;
	t_count = 0;
	
	MCGFloat t_scale;
	bool t_has_scale;
	
	t_success = MCImageSplitScaledFilename(p_filename, t_base, t_extension, t_has_scale, t_scale);
	
	if (t_success)
	{
		// if given filename is tagged for a scale then we just return that scaled file
		if (t_has_scale)
		{
			t_success = MCMemoryNewArray(1, t_filelist);
			if (t_success)
			{
				t_count = 1;
				t_filelist[0].scale = t_scale;
				t_filelist[0].filename = p_stack->resolve_filename(p_filename);
				
				t_success = t_filelist[0].filename != nil;
			}
		}
		else
		{
			MCImageScaleLabel *t_scale_labels;
			t_scale_labels = s_image_scale_labels;
			
			while (t_success && t_scale_labels->label != nil)
			{
				char *t_filename;
				t_filename = nil;
				
				char *t_resolved;
				t_resolved = nil;
				
				t_success = MCCStringFormat(t_filename, "%s%s%s", t_base, t_scale_labels->label, t_extension);
				
				if (t_success)
					t_success = nil != (t_resolved = p_stack->resolve_filename(t_filename));
				
				if (t_success)
				{
					if (MCS_exists(t_resolved, True))
					{
						t_success = MCMemoryResizeArray(t_count + 1, t_filelist, t_count);
						
						if (t_success)
						{
							t_filelist[t_count - 1].filename = t_resolved;
							t_filelist[t_count - 1].scale = t_scale_labels->scale;
							
							t_resolved = nil;
						}
					}
				}
				
				MCCStringFree(t_filename);
				MCCStringFree(t_resolved);
				
				t_scale_labels++;
			}
		}
	}
	
	if (t_success)
	{
		r_list = t_filelist;
		r_count = t_count;
	}
	else
		MCImageFreeScaledFileList(t_filelist, t_count);
	
	MCCStringFree(t_base);
	MCCStringFree(t_extension);
	
	return t_success;
}

////////////////////////////////////////////////////////////////////////////////


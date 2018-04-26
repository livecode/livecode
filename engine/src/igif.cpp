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
#include "button.h"
#include "stack.h"

#include "globals.h"

#include "context.h"

#include "imageloader.h"

#include "gif_lib.h"

void MCImage::setframe(int32_t p_newframe)
{
	// MW-2013-03-12: [[ Bug ]] Make sure we check we have a parent (an MCImage can have its frame set on
	//   startup via the templateImage which has no parent).
	MCStack *t_stack;
	t_stack = getstack();
	if (t_stack != nil && t_stack->getstate(CS_EFFECT))
		return;

	uindex_t t_framecount = 0;
	if (m_rep != nil)
		t_framecount = m_rep->GetFrameCount();

	// IM-2013-03-11: [[ BZ 10723 ]] if there is no image data or there is only one frame then currentframe should be zero
	if (t_framecount <= 1)
	{
		currentframe = 0;
		return;
	}

	if (p_newframe < 0)
		p_newframe = 0;
	else if (p_newframe >= t_framecount)
		p_newframe = t_framecount - 1;

	if (p_newframe == currentframe)
		return;

	currentframe = p_newframe;

	notifyneeds(false);

	// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
	// MW-2011-11-15: [[ Bug 9863 ]] Only invalidate the whole object if it has an
	//   owner!
	if (parent)
		layer_redrawall();
	}

void MCImage::advanceframe()
{
	uindex_t t_framecount = 0;
	if (m_rep != nil)
		t_framecount = m_rep->GetFrameCount();

	if (t_framecount <= 1)
		return;

	int32_t t_newframe = currentframe + (((state & CS_REVERSE) == 0) ? 1 : -1);

	if (t_newframe < 0 || t_newframe >= t_framecount)
	{
		if (irepeatcount > 0)
			irepeatcount--;

		if (irepeatcount == 0)
			return;

		if (flags & F_PALINDROME_FRAMES)
		{
			if (state & CS_REVERSE)
				t_newframe = 1;
			else
				t_newframe = t_framecount - 2;
			state ^= CS_REVERSE;
		}
		else
		{
			if (state & CS_REVERSE)
				t_newframe = t_framecount - 1;
			else
				t_newframe = 0;
		}
	}

	setframe(t_newframe);
}

////////////////////////////////////////////////////////////////////////////////

static uint32_t gif_color_to_pixel(GifColorType& p_color)
{
	return MCGPixelPackNative(p_color.Red, p_color.Green, p_color.Blue, 255);
}

static void gif_draw_image_into_canvas(MCImageBitmap *p_canvas, GifByteType *p_raster, int32_t p_left, int32_t p_top, int32_t p_width, int32_t p_height, ColorMapObject *p_colors, int32_t p_transparency, bool t_overlay)
{
	// If the bounds are outside of the target, ignore.
	if (p_left < 0 || p_top < 0 || (p_left + p_width) > p_canvas -> width || (p_top + p_height) > p_canvas -> height)
		return;

	// Compute the dst and src ptrs.
	uint32_t *t_dst_ptr;
	uint8_t *t_src_ptr;
	t_dst_ptr = (uint32_t *)p_canvas -> data + p_top * (p_canvas -> stride / 4) + p_left;
	t_src_ptr = (uint8_t *)p_raster;
	for(int32_t y = 0; y < p_height; y++)
	{
		for(int32_t x = 0; x < p_width; x++)
{
			uint8_t t_color;
			t_color = t_src_ptr[x];

			if (t_overlay && t_color == p_transparency)
				continue;
				
			uint32_t t_pixel;
			if (t_color < p_colors -> ColorCount)
	{
				if (t_color != p_transparency)
					t_pixel = gif_color_to_pixel(p_colors -> Colors[t_color]);
				else
					t_pixel = 0;
		}
			else
				t_pixel = 0;

			t_dst_ptr[x] = t_pixel;
		}

		t_dst_ptr += p_canvas -> stride / 4;
		t_src_ptr += p_width;
	}
	}

void gif_fill_image_region(MCImageBitmap *p_bitmap, MCRectangle &p_region, uindex_t p_pixel)
{
	int32_t t_left, t_top, t_right, t_bottom;
	t_left = p_region.x;
	t_top = p_region.y;
	t_right = t_left + p_region.width;
	t_bottom = t_top + p_region.height;

	if (t_left < 0 || t_top < 0 || t_right > p_bitmap -> width || t_bottom > p_bitmap -> height)
		return;

	uint8_t *t_dst_ptr = (uint8_t*)p_bitmap->data + t_top * p_bitmap->stride + t_left * sizeof(uint32_t);
	for (uindex_t y = t_top; y < t_bottom; y++)
{
		uint32_t *t_dst_row = (uint32_t*)t_dst_ptr;
		for (uindex_t x = t_left; x < t_right; x++)
			*t_dst_row++ = p_pixel;
		t_dst_ptr += p_bitmap->stride;
	}
	}

static void gif_paste_image(MCImageBitmap *p_dst, MCImageBitmap *p_src, int32_t p_x_offset, int32_t p_y_offset)
{
	if (p_x_offset < 0 || p_y_offset < 0 ||
		p_x_offset + p_src->width > p_dst->width ||
		p_y_offset + p_src->width > p_dst->width)
		return;

	uint8_t *t_src_ptr = (uint8_t*)p_src->data;
	uint8_t *t_dst_ptr = (uint8_t*)p_dst->data + p_y_offset * p_dst->stride + p_x_offset * sizeof(uint32_t);
	uindex_t t_row_bytes = p_src->width * sizeof(uint32_t);
	for(int32_t y = 0; y < p_src->height; y++)
	{
		MCMemoryCopy(t_dst_ptr, t_src_ptr, t_row_bytes);
		t_src_ptr += p_src->stride;
		t_dst_ptr += p_dst->stride;
	}
		}

////////////////////////////////////////////////////////////////////////////////

int gif_readFunc(GifFileType *p_gif, GifByteType *p_buffer, int p_byte_count)
{
	IO_handle t_stream = (IO_handle)p_gif->UserData;
	uindex_t t_byte_count = p_byte_count;
	/* UNCHECKED */	MCS_readfixed(p_buffer, t_byte_count, t_stream);
	return t_byte_count;
}

////////////////////////////////////////////////////////////////////////////////

class MCGIFImageLoader : public MCImageLoader
{
public:
	MCGIFImageLoader(IO_handle p_stream);
	virtual ~MCGIFImageLoader();
	
	virtual MCImageLoaderFormat GetFormat() { return kMCImageFormatGIF; }
	
	virtual bool GetFrameCount(uint32_t &r_count);
	
protected:
	virtual bool LoadHeader(uint32_t &r_width, uint32_t &r_height, uint32_t &r_xhot, uint32_t &r_yhot, MCStringRef &r_name, uint32_t &r_frame_count, MCImageMetadata &r_metadata);
	virtual bool LoadFrames(MCBitmapFrame *&r_frames, uint32_t &r_count);
	
private:
	GifFileType *m_gif;
	int m_error;
};

MCGIFImageLoader::MCGIFImageLoader(IO_handle p_stream) : MCImageLoader(p_stream)
{
	m_gif = nil;
	m_error = 0;
}

MCGIFImageLoader::~MCGIFImageLoader()
{
	if (m_gif != nil)
	{
		int t_error_code;
		DGifCloseFile(m_gif, &t_error_code);
		m_gif = nil;
	}
}

// IM-2014-08-25: [[ Bug 13273 ]] We don't have the frame count until the image is fully
// loaded, so override this method to call EnsureFrames() to force frame loading.
bool MCGIFImageLoader::GetFrameCount(uint32_t &r_count)
{
	if (!EnsureFrames())
		return false;
	
	return MCImageLoader::GetFrameCount(r_count);
}

bool MCGIFImageLoader::LoadHeader(uint32_t &r_width, uint32_t &r_height, uint32_t &r_xhot, uint32_t &r_yhot, MCStringRef &r_name, uint32_t &r_frame_count, MCImageMetadata &r_metadata)
{
	bool t_success;
	t_success = true;
	
	if (t_success)
		t_success = nil != (m_gif = DGifOpen(GetStream(), gif_readFunc, &m_error));
	
	if (t_success)
	{
		r_width = m_gif->SWidth;
		r_height = m_gif->SHeight;
		
		r_xhot = r_yhot = 0;
		r_name = MCValueRetain(kMCEmptyString);
		r_frame_count = m_gif->ImageCount;
	}
	
	return t_success;
}

bool MCGIFImageLoader::LoadFrames(MCBitmapFrame *&r_frames, uint32_t &r_count)
{
	bool t_success;
	t_success = true;
	
	MCImageBitmap *t_canvas;
	t_canvas = nil;
	
	// restoration info
	MCImageBitmap *t_restore_image = nil;
	int t_disposal_mode = DISPOSAL_UNSPECIFIED;
	MCRectangle t_disposal_region = kMCEmptyRectangle;

	// The list of frames.
	MCBitmapFrame *t_frames = nil;

	t_success = GIF_OK == DGifSlurp(m_gif);

	// Fetch the width and height of the virtual canvas.
	int32_t t_width, t_height;

	if (t_success)
	{
		t_width = m_gif -> SWidth;
		t_height = m_gif -> SHeight;

		// create the canvas image
		t_success = MCImageBitmapCreate(t_width, t_height, t_canvas);
	}

	// The current frame count. The number of frames is the same as the
	// number of images in the GIF.
	uint32_t t_frame_count;
	t_frame_count = 0;
	
	// If true, the new image will be merged with the old - otherwise the mask
	// replaces it.
	bool t_overlay;
	t_overlay = false;
	
	// Loop through all the images, making frames as we go.
	for(uindex_t i = 0; t_success && i < m_gif -> ImageCount; i++)
	{
		// Process the disposal.
		switch (t_disposal_mode)
		{
		case DISPOSE_BACKGROUND:
			gif_fill_image_region(t_canvas, t_disposal_region, 0);
			break;

		case DISPOSE_PREVIOUS:
			if (t_restore_image != nil)
				gif_paste_image(t_canvas, t_restore_image, t_disposal_region.x, t_disposal_region.y);
			break;

		case DISPOSE_DO_NOT:
			t_overlay = true;
			break;

		default:
			t_overlay = false;
			break;
		}

		// Fetch the image information.
		GraphicsControlBlock t_image_gcb;
		MCRectangle t_image_region;
		ColorMapObject *t_image_colors = nil;
		int32_t t_image_transparency;
		int32_t t_image_delay;
		int32_t t_image_disposal;
		GifByteType *t_image_raster;

		// First the information from the image description.
		t_image_region.x = m_gif -> SavedImages[i] . ImageDesc . Left;
		t_image_region.y = m_gif -> SavedImages[i] . ImageDesc . Top;
		t_image_region.width = m_gif -> SavedImages[i] . ImageDesc . Width;
		t_image_region.height = m_gif -> SavedImages[i] . ImageDesc . Height;
		t_image_colors = m_gif -> SavedImages[i] . ImageDesc . ColorMap;
		t_image_raster = m_gif -> SavedImages[i] . RasterBits;
		if (t_image_colors == nil)
			t_image_colors = m_gif -> SColorMap;
		
		// Then the information from the GCB.
		if (GIF_OK == DGifSavedExtensionToGCB(m_gif, i, &t_image_gcb))
		{
			t_image_transparency = t_image_gcb . TransparentColor;
			t_image_delay = t_image_gcb . DelayTime;
			t_image_disposal = t_image_gcb . DisposalMode;
		}
		else
		{
			t_image_transparency = -1;
			t_image_delay = 0;
			t_image_disposal = DISPOSAL_UNSPECIFIED;
			}
			
		// If disposal is 'previous' then cache the portion of the canvas we are
		// about to affect.
		if (t_image_disposal == DISPOSE_PREVIOUS)
			{
			if (t_restore_image != nil)
			{
				if (t_disposal_mode != DISPOSE_PREVIOUS ||
					!MCU_equal_rect(t_disposal_region, t_image_region))
				{
					MCImageFreeBitmap(t_restore_image);
					t_restore_image = nil;
				}
			}
			if (t_restore_image == nil)
				t_success = MCImageCopyBitmapRegion(t_canvas, t_image_region, t_restore_image);
		}
		
		if (t_success)
		{
			// Render the image into the canvas.
			gif_draw_image_into_canvas(t_canvas, t_image_raster,
				t_image_region.x, t_image_region.y, t_image_region.width, t_image_region.height,
				t_image_colors, t_image_transparency, t_overlay);
			
			// Generate our frame.
			t_success = MCMemoryResizeArray(t_frame_count + 1, t_frames, t_frame_count);
		}

		MCImageBitmap *t_frame_bitmap = nil;
		if (t_success)
			t_success = MCImageCopyBitmap(t_canvas, t_frame_bitmap);
			
		if (t_success)
		{
			MCImageBitmapCheckTransparency(t_frame_bitmap);
			t_frames[t_frame_count - 1].image = t_frame_bitmap;
			t_frames[t_frame_count - 1].duration = t_image_delay * 10; // convert 1/100 seconds to milliseconds
			t_frames[t_frame_count - 1].x_scale = t_frames[t_frame_count - 1].y_scale = 1.0;
		}

		t_disposal_region = t_image_region;
		t_disposal_mode = t_image_disposal;
	}
	
	MCImageFreeBitmap(t_canvas);
	MCImageFreeBitmap(t_restore_image);
	
	if (t_success)
	{
		r_frames = t_frames;
		r_count = t_frame_count;
	}
	else
		MCImageFreeFrames(t_frames, t_frame_count);

	return t_success;
}

bool MCImageLoaderCreateForGIFStream(IO_handle p_stream, MCImageLoader *&r_loader)
{
	MCGIFImageLoader *t_loader;
	t_loader = new (nothrow) MCGIFImageLoader(p_stream);
	
	if (t_loader == nil)
		return false;
	
	r_loader = t_loader;
	
	return true;
}

////////////////////////////////////////////////////////////////////////////////

struct MCGIFWriteContext
{
	IO_handle stream;
	uindex_t byte_count;
};

int gif_writeFunc(GifFileType *p_gif, const GifByteType *p_buffer, int p_byte_count)
{
	MCGIFWriteContext *t_context = (MCGIFWriteContext*)p_gif->UserData;
	uindex_t t_byte_count = p_byte_count;
	/* UNCHECKED */ MCS_write(p_buffer, sizeof(GifByteType), t_byte_count, t_context->stream);
	t_context->byte_count += t_byte_count;
	return t_byte_count;
}

bool MCImageEncodeGIF(MCImageIndexedBitmap *p_indexed, IO_handle p_stream, uindex_t &r_bytes_written)
{
	bool t_success = true;

	int32_t t_transparent = -1;
	uindex_t t_palette_size;
	uindex_t t_depth;


	t_depth = GifBitSize(p_indexed->palette_size);
	// GIF requires palette size to be 2^depth
	t_palette_size = 1 << t_depth;

	int t_err = 0;
	GifFileType *t_gif = nil;
	ColorMapObject *t_colormap = nil;
	
	MCGIFWriteContext t_context;
	t_context.stream = p_stream;
	t_context.byte_count = 0;

	t_success = nil != (t_gif = EGifOpen(&t_context, gif_writeFunc, &t_err));

	if (t_success)
		t_success = nil != (t_colormap = GifMakeMapObject(t_palette_size, nil));

	if (t_success)
	{
		for (uindex_t i = 0; i < p_indexed->palette_size; i++)
		{
			t_colormap->Colors[i].Red = p_indexed->palette[i].red;
			t_colormap->Colors[i].Green = p_indexed->palette[i].green;
			t_colormap->Colors[i].Blue = p_indexed->palette[i].blue;
		}
		for (uindex_t i = p_indexed->palette_size; i < t_palette_size; i++)
		{
			t_colormap->Colors[i].Red =
				t_colormap->Colors[i].Green =
				t_colormap->Colors[i].Blue = 0;
		}

		if (MCImageIndexedBitmapHasTransparency(p_indexed))
		{
			t_transparent = p_indexed->transparent_index;
			t_colormap->Colors[t_transparent].Red =
				t_colormap->Colors[t_transparent].Green = 
				t_colormap->Colors[t_transparent].Blue = 0xFF;
		}

		t_success = GIF_OK == EGifPutScreenDesc(t_gif, p_indexed->width, p_indexed->height, t_depth, 0, t_colormap);
	}

	if (t_success)
	{
		if (t_transparent != -1)
		{
			GraphicsControlBlock t_gcb;
			MCMemoryClear(&t_gcb, sizeof(t_gcb));
			t_gcb.TransparentColor = t_transparent;

			GifByteType t_extension[4];

			uindex_t t_extension_size;
			t_extension_size = EGifGCBToExtension(&t_gcb, t_extension);

			// Should always be 4 bytes
			MCAssert(t_extension_size == sizeof(t_extension));

			t_success = GIF_OK == EGifPutExtension(t_gif, GRAPHICS_EXT_FUNC_CODE, sizeof(t_extension), t_extension);
		}
	}

	if (t_success)
		t_success = GIF_OK == EGifPutImageDesc(t_gif, 0, 0, p_indexed->width, p_indexed->height, false, nil);

	for (uindex_t y = 0; t_success && y < p_indexed->height; y++)
		t_success = GIF_OK == EGifPutLine(t_gif, (uint8_t*)p_indexed->data + y * p_indexed->stride, p_indexed->width);

	int t_error_code;
	if (GIF_ERROR == EGifCloseFile(t_gif, &t_error_code))
		t_success = false;

	GifFreeMapObject(t_colormap);

	if (t_success)
		r_bytes_written = t_context.byte_count;

	return t_success;
}

bool MCImageEncodeGIF(MCImageBitmap *p_image, IO_handle p_stream, bool p_dither, uindex_t &r_bytes_written)
{
	bool t_success = true;

	MCImageIndexedBitmap *t_indexed = nil;

	if (!MCImageForceBitmapToIndexed(p_image, p_dither, t_indexed))
		return false;

	t_success = MCImageEncodeGIF(t_indexed, p_stream, r_bytes_written);

	MCImageFreeIndexedBitmap(t_indexed);

	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

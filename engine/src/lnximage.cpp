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

#include "lnxprefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "globals.h"
#include "image.h"

#include "lnxdc.h"
#include "imagebitmap.h"

// Linux image conversion functions

bool MCImageBitmapCreateWithXImage(XImage *p_image, MCImageBitmap *&r_bitmap)
{
	MCImageBitmap *t_bitmap;
	uindex_t t_width = p_image->width;
	uindex_t t_height = p_image->height;
	
	if (!MCImageBitmapCreate(t_width, t_height, t_bitmap))
		return false;

	uint8_t *t_src_ptr = (uint8_t*)p_image->data;
	uint8_t *t_dst_ptr = (uint8_t*)t_bitmap->data;
	while (t_height--)
	{
		MCMemoryCopy(t_dst_ptr, t_src_ptr, t_width * 4);
		t_src_ptr += p_image->bytes_per_line;
		t_dst_ptr += t_bitmap->stride;
	}
	
	MCImageBitmapSetAlphaValue(t_bitmap, 0xFF);
	r_bitmap = t_bitmap;
	return true;
}

bool MCGImageToX11Bitmap(MCGImageRef p_image, MCBitmap *&r_bitmap)
{
	bool t_success;
	t_success = true;
	
	MCBitmap *t_bitmap;
	t_bitmap = nil;
	
	MCGRaster t_raster;
	
	if (t_success)
		t_success = MCGImageGetRaster(p_image, t_raster);
		
	if (t_success)
		t_success = nil != (t_bitmap = ((MCScreenDC*)MCscreen)->createimage(32, t_raster.width, t_raster.height, False, 0, False, False));
		
	if (t_success)
	{
		const uint8_t *t_src_ptr;
		t_src_ptr = (uint8_t*)t_raster.pixels;
		uint8_t *t_dst_ptr;
		t_dst_ptr = (uint8_t*)t_bitmap->data;
		
		for (uint32_t i = 0; i < t_raster.height; i++)
		{
			MCMemoryCopy(t_dst_ptr, t_src_ptr, t_raster.width * sizeof(uint32_t));
			t_src_ptr += t_raster.stride;
			t_dst_ptr += t_bitmap->bytes_per_line;
		}
		
		r_bitmap = t_bitmap;
	}
	
	return t_success;
}

bool MCX11BitmapToX11Pixmap(MCBitmap *p_bitmap, Pixmap &r_pixmap)
{
	bool t_success;
	t_success = true;
	
	Pixmap t_pixmap;
	t_pixmap = nil;
	
	if (t_success)
		t_success = nil != (t_pixmap = ((MCScreenDC*)MCscreen)->createpixmap(p_bitmap->width, p_bitmap->height, p_bitmap->depth, False));
		
	if (t_success)
	{
		((MCScreenDC*)MCscreen)->putimage(t_pixmap, p_bitmap, 0, 0, 0, 0, p_bitmap->width, p_bitmap->height);
		r_pixmap = t_pixmap;
	}
	
	return t_success;
}

bool MCGImageToX11Pixmap(MCGImageRef p_image, Pixmap &r_pixmap)
{
	MCBitmap *t_bitmap;
	t_bitmap = nil;
	
	bool t_success;
	t_success = true;
	
	if (t_success)
		t_success = MCGImageToX11Bitmap(p_image, t_bitmap) && MCX11BitmapToX11Pixmap(t_bitmap, r_pixmap);
		
	if (t_bitmap != nil)
		((MCScreenDC*)MCscreen)->destroyimage(t_bitmap);
	
	return t_success;
}

// IM-2014-04-15: [[ Bug 11603 ]] Convert pattern ref to X11 Pixmap with scale transform applied
bool MCPatternToX11Pixmap(MCPatternRef p_pattern, Pixmap &r_pixmap)
{
	bool t_success;
	t_success = true;
	
	MCGImageRef t_image;
	t_image = p_pattern->image;
	
	MCGAffineTransform t_transform;
	t_transform = MCGAffineTransformMakeScale(p_pattern->scale, p_pattern->scale);
	
	MCGRectangle t_src_rect;
	t_src_rect = MCGRectangleMake(0, 0, MCGImageGetWidth(t_image), MCGImageGetHeight(t_image));
	
	MCGRectangle t_dst_rect;
	t_dst_rect = MCGRectangleApplyAffineTransform(t_src_rect, t_transform);
	
	uint32_t t_width, t_height;
	t_width = ceilf(t_dst_rect.size.width);
	t_height = ceilf(t_dst_rect.size.height);
	
	MCBitmap *t_bitmap;
	t_bitmap = nil;
	
	if (t_success)
		t_success = nil != (t_bitmap = ((MCScreenDC*)MCscreen)->createimage(32, t_width, t_height, True, 0, False, False));

	MCGContextRef t_context;
	t_context = nil;
	
	if (t_success)
		t_success = MCGContextCreateWithPixels(t_width, t_height, t_bitmap->bytes_per_line, t_bitmap->data, false, t_context);
		
	if (t_success)
	{
		// Fill background with black
		MCGContextSetFillRGBAColor(t_context, 0.0, 0.0, 0.0, 1.0);
		MCGContextAddRectangle(t_context, t_dst_rect);
		MCGContextFill(t_context);
		
		// draw transformed pattern image
		MCGContextConcatCTM(t_context, t_transform);
		MCGContextDrawImage(t_context, t_image, t_src_rect, kMCGImageFilterHigh);
	}
	
	if (t_context != nil)
		MCGContextRelease(t_context);
	
	if (t_success)
		t_success = MCX11BitmapToX11Pixmap(t_bitmap, r_pixmap);
		
	if (t_bitmap != nil)
		((MCScreenDC*)MCscreen)->destroyimage(t_bitmap);
	
	return t_success;
}

void surface_extract_alpha(void *p_pixels, uint4 p_pixel_stride, void *p_alpha, uint4 p_alpha_stride, uint4 p_width, uint4 p_height);
MCWindowShape *MCImage::makewindowshape(void)
{
	bool t_success;
	t_success = true;

	MCWindowShape *t_mask;
	if (!MCMemoryNew(t_mask))
		return nil;
	
	// Get the width / height.
	t_mask -> width = rect . width;
	t_mask -> height = rect . height;

	MCImageBitmap *t_bitmap = nil;
	bool t_has_mask = false, t_has_alpha = false;
	t_success = lockbitmap(t_bitmap, true);
	if (t_success)
	{
		t_has_mask = MCImageBitmapHasTransparency(t_bitmap, t_has_alpha);
#ifdef LIBGRAPHICS_BROKEN
		if (t_has_alpha)
#else
		if (t_has_mask)
#endif
		{
			// The mask is not sharp.
			t_mask -> is_sharp = false;

			// Set the stride;
			t_mask -> stride = (t_mask -> width + 3) & ~3;

			// Allocate.
			t_success = nil != (t_mask -> data = (char *)malloc(t_mask -> stride * t_mask -> height));

			if (t_success)
				surface_extract_alpha(t_bitmap->data, t_bitmap->stride, t_mask->data, t_mask->stride, t_mask->width, t_mask->height);
		}
#ifdef LIBGRAPHICS_BROKEN
		else if (t_has_mask)
		{
			// The mask is sharp.
			t_mask -> is_sharp = true;

			// Stride and data are zero.
			t_mask -> stride = 0;
			t_mask -> data = nil;

			// Handle is region.
			Pixmap t_drawdata = nil, t_drawmask = nil;
			MCBitmap *t_maskimagealpha = nil;
			t_success = MCImageSplitPixmaps(t_bitmap, t_drawdata, t_drawmask, t_maskimagealpha);

			if (t_success)
			{
				t_mask -> handle = (void*)t_drawmask;
				t_drawmask = nil;
			}

			MCscreen->freepixmap(t_drawdata);
			MCscreen->freepixmap(t_drawmask);
			if (t_maskimagealpha != nil)
				MCscreen->destroyimage(t_maskimagealpha);
		}
#endif
		else
			t_success = false;
	}
	unlockbitmap(t_bitmap);

	if (t_success)
		return t_mask;

	MCMemoryDelete(t_mask);

	return nil;
}

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

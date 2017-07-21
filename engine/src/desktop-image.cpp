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

#include "platform.h"

#include "globdefs.h"
#include "filedefs.h"
#include "osspec.h"
#include "typedefs.h"
#include "parsedef.h"
#include "objdefs.h"

#include "image.h"

////////////////////////////////////////////////////////////////////////////////

extern void surface_extract_alpha(void *p_pixels, uint4 p_pixel_stride, void *p_alpha, uint4 p_alpha_stride, uint4 p_width, uint4 p_height);

////////////////////////////////////////////////////////////////////////////////

MCWindowShape *MCImage::makewindowshape(const MCGIntegerSize &p_size)
{
	bool t_success = true;
	
	MCWindowShape *t_mask = nil;
	MCPlatformWindowMaskRef t_mask_image = nil;
	MCImageBitmap *t_bitmap = nil;
	uint8_t *t_alpha = nil;
	uindex_t t_alpha_stride = 0;
	uindex_t t_width, t_height;
	
	t_success = lockbitmap(true, true, &p_size, t_bitmap);
	
	if (t_success)
	{
		t_width = t_bitmap->width;
		t_height = t_bitmap->height;
		
		t_alpha_stride = (t_width + 3) & ~3;
		t_success = MCMemoryAllocate(t_alpha_stride * t_height, t_alpha);
	}
	
	if (t_success)
	{
		surface_extract_alpha(t_bitmap->data, t_bitmap->stride, t_alpha, t_alpha_stride, t_width, t_height);
		// IM-2014-10-03: [[ Bug 13432 ]] Pass ownership of alpha buffer to the new mask
		MCPlatformWindowMaskCreateWithAlphaAndRelease(t_width, t_height, t_alpha_stride, t_alpha, t_mask_image);
	}
	
	if (t_success)
		t_success = MCMemoryNew(t_mask);
	
	unlockbitmap(t_bitmap);
	
	if (!t_success)
	{
		MCMemoryDeallocate(t_mask);
		if (t_mask_image != nil)
			MCPlatformWindowMaskRelease(t_mask_image);
		return nil;
	}
	
	t_mask->width = t_width;
	t_mask->height = t_height;
	t_mask->is_sharp = false;
	
	t_mask->data = nil;
	t_mask->stride = t_alpha_stride;
	
	t_mask->handle = t_mask_image;
	
	return t_mask;	
}

////////////////////////////////////////////////////////////////////////////////

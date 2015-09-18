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

#include "graphics.h"

////////////////////////////////////////////////////////////////////////////////

// IM-2014-10-22: [[ Bug 13746 ]] Set the given rect of the raster to transparent black
void MCGRasterClearRect(MCGRaster &x_raster, const MCGIntegerRectangle &p_rect)
{
	MCGIntegerRectangle t_coverage;
	t_coverage = MCGIntegerRectangleIntersection(p_rect, MCGIntegerRectangleMake(0, 0, x_raster.width, x_raster.height));
	
	if (MCGIntegerRectangleIsEmpty(t_coverage))
		return;
	
	uint8_t *t_pixel_ptr;
	t_pixel_ptr = (uint8_t*)x_raster.pixels;
	
	if (t_coverage.origin.y > 0)
		t_pixel_ptr += t_coverage.origin.y * x_raster.stride;
	if (t_coverage.origin.x > 0)
		t_pixel_ptr += t_coverage.origin.x * sizeof(uint32_t);
	
	uint32_t t_width;
	t_width = t_coverage.size.width * sizeof(uint32_t);
	
	for (uint32_t y = 0; y < t_coverage.size.height; y++)
	{
		MCMemoryClear(t_pixel_ptr, t_width);
		t_pixel_ptr += x_raster.stride;
	}
}

// IM-2014-10-03: [[ Bug 13432 ]] Set the alpha values of the opaque image from the alpha raster, premultiplying the pixels
void MCGRasterApplyAlpha(MCGRaster &x_raster, const MCGRaster &p_alpha, const MCGIntegerPoint &p_offset)
{
	MCAssert(p_alpha.format == kMCGRasterFormat_A);
	MCAssert(x_raster.format == kMCGRasterFormat_xRGB);
	
	MCGIntegerRectangle t_coverage;
	t_coverage = MCGIntegerRectangleIntersection(MCGIntegerRectangleMake(0, 0, x_raster.width, x_raster.height),
												 MCGIntegerRectangleMake(p_offset.x, p_offset.y, p_alpha.width, p_alpha.height));
	
	if (MCGIntegerRectangleIsEmpty(t_coverage))
	{
		// IM-2014-10-22: [[ Bug 13746 ]] If the mask is not within the region of the raster then blank out everything
		MCGRasterClearRect(x_raster, MCGIntegerRectangleMake(0, 0, x_raster.width, x_raster.height));
		x_raster.format = kMCGRasterFormat_ARGB;
		return;
	}
	
	// IM-2014-10-22: [[ Bug 13746 ]] Areas outside the mask need to be blanked out
	int32_t t_left, t_top, t_right, t_bottom;
	t_left = t_coverage.origin.x;
	t_top = t_coverage.origin.y;
	t_right = t_coverage.origin.x + t_coverage.size.width;
	t_bottom = t_coverage.origin.y + t_coverage.size.height;
	
	MCGRasterClearRect(x_raster, MCGIntegerRectangleMakeLTRB(0, 0, x_raster.width, t_top));
	MCGRasterClearRect(x_raster, MCGIntegerRectangleMakeLTRB(0, t_bottom, x_raster.width, x_raster.height));
	MCGRasterClearRect(x_raster, MCGIntegerRectangleMakeLTRB(0, t_top, t_left, t_bottom));
	MCGRasterClearRect(x_raster, MCGIntegerRectangleMakeLTRB(t_right, t_top, x_raster.width, t_bottom));
	
	uint8_t *t_alpha_ptr;
	t_alpha_ptr = (uint8_t*)p_alpha.pixels;
	
	if (p_offset.y < 0)
		t_alpha_ptr += p_alpha.stride * (-p_offset.y);
	if (p_offset.x < 0)
		t_alpha_ptr += (-p_offset.x);
	
	uint8_t *t_pixel_ptr;
	t_pixel_ptr = (uint8_t*)x_raster.pixels;
	
	if (t_coverage.origin.y > 0)
		t_pixel_ptr += x_raster.stride * t_coverage.origin.y;
	if (t_coverage.origin.x > 0)
		t_pixel_ptr += t_coverage.origin.x * sizeof(uint32_t);
	
	for (uint32_t y = 0; y < t_coverage.size.height; y++)
	{
		uint8_t *t_alpha_row;
		t_alpha_row = t_alpha_ptr;
		
		uint32_t *t_pixel_row;
		t_pixel_row = (uint32_t*)t_pixel_ptr;
		
		for (uint32_t x = 0; x < t_coverage.size.width; x++)
		{
			uint32_t t_pixel;
			t_pixel = *t_pixel_row;
			
			uint8_t t_alpha;
			t_alpha = *t_alpha_row++;
			
			*t_pixel_row++ = MCGPixelPreMultiplyNative(MCGPixelSetNativeAlpha(t_pixel, t_alpha));
		}
		//
		
		t_alpha_ptr += p_alpha.stride;
		t_pixel_ptr += x_raster.stride;
	}
	
	x_raster.format = kMCGRasterFormat_ARGB;
}

////////////////////////////////////////////////////////////////////////////////

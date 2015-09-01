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
#include "imagebitmap.h"

#if defined(TARGET_PLATFORM_MACOS_X)
#include <Carbon/Carbon.h>
#elif defined(TARGET_SUBPLATFORM_IPHONE)
#include <CoreGraphics/CoreGraphics.h>
#endif

////////////////////////////////////////////////////////////////////////////////

// IM-2014-09-29: [[ Bug 13451 ]] return the sRGB colorspace (as a CGColorSpaceRef).
bool MCImageGetCGColorSpace(CGColorSpaceRef &r_colorspace)
{
	CGColorSpaceRef t_colorspace;
	
#if defined(TARGET_PLATFORM_MACOS_X)
	// on OSX request sRGB by name.
	t_colorspace = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
#elif defined(TARGET_SUBPLATFORM_IPHONE)
	// on iOS this isn't supported so we use the deviceRGB colorspace (which is sRGB anyway).
	t_colorspace = CGColorSpaceCreateDeviceRGB();
#endif
	
	if (t_colorspace == nil)
		return false;
	
	r_colorspace = t_colorspace;
	return true;
}

////////////////////////////////////////////////////////////////////////////////

void __CGDataProviderDeallocate(void *info, const void *data, size_t size)
{
	MCMemoryDeallocate(const_cast<void*>(data));
}

// IM-2013-08-21: [[ RefactorGraphics ]] Refactor CGImage creation code to be pixel-format independent
CGBitmapInfo MCGPixelFormatToCGBitmapInfo(uint32_t p_pixel_format, bool p_alpha)
{
	CGBitmapInfo t_bm_info;
	
	bool t_alpha_first;
	switch (p_pixel_format)
	{
		case kMCGPixelFormatBGRA:
			t_bm_info = kCGBitmapByteOrder32Little;
			t_alpha_first = true;
			break;
			
		case kMCGPixelFormatABGR:
			t_bm_info = kCGBitmapByteOrder32Little;
			t_alpha_first = false;
			break;
			
		case kMCGPixelFormatRGBA:
			t_bm_info = kCGBitmapByteOrder32Big;
			t_alpha_first = false;
			break;
			
		case kMCGPixelFormatARGB:
			t_bm_info = kCGBitmapByteOrder32Big;
			t_alpha_first = true;
			break;
	}
	
	if (p_alpha)
		t_bm_info |= t_alpha_first ? kCGImageAlphaPremultipliedFirst : kCGImageAlphaPremultipliedLast;
	else
		t_bm_info |= t_alpha_first ? kCGImageAlphaNoneSkipFirst : kCGImageAlphaNoneSkipLast;
	
	return t_bm_info;
}

////////////////////////////////////////////////////////////////////////////////

bool MCGRasterToCGImage(const MCGRaster &p_raster, MCGRectangle p_src_rect, CGColorSpaceRef p_colorspace, bool p_copy, bool p_invert, CGImageRef &r_image)
{
	bool t_success = true;
	
	int32_t t_x, t_y;
	uint32_t t_width, t_height;
	t_x = p_src_rect.origin.x;
	t_y = p_src_rect.origin.y;
	t_width = p_src_rect.size.width;
	t_height = p_src_rect.size.height;
	
	/* OVERHAUL - REVISIT: pixel formats */
	const uint8_t *t_src_ptr = (uint8_t*)p_raster.pixels;
	t_src_ptr += t_y * p_raster.stride + t_x * sizeof(uint32_t);
	
	uint32_t t_dst_stride;
	
	if (p_invert)
		p_copy = true;
	
	CGImageRef t_image = nil;
	CGDataProviderRef t_data_provider = nil;
	
	if (!p_copy)
	{
		t_dst_stride = p_raster.stride;
		t_success = nil != (t_data_provider = CGDataProviderCreateWithData(nil, t_src_ptr, t_height * p_raster.stride, nil));
	}
	else
	{
		uint8_t* t_buffer = nil;
		
		t_dst_stride = t_width * sizeof(uint32_t);
		uindex_t t_buffer_size = t_height * t_dst_stride;
		t_success = MCMemoryAllocate(t_buffer_size, t_buffer);
		
		if (t_success)
		{
			int32_t t_src_stride;
			
			uint8_t* t_dst_ptr = t_buffer;
			if (!p_invert)
			{
				t_src_stride = p_raster.stride;
			}
			else
			{
				t_src_ptr += ((int32_t)t_height - 1) * p_raster.stride;
				t_src_stride = -p_raster.stride;
			}
			
			for (uindex_t y = 0; y < t_height; y++)
			{
				MCMemoryCopy(t_dst_ptr, t_src_ptr, t_dst_stride);
				t_dst_ptr += t_dst_stride;
				t_src_ptr += t_src_stride;
			}
		}
		if (t_success)
			t_success = nil != (t_data_provider = CGDataProviderCreateWithData(nil, t_buffer, t_buffer_size, __CGDataProviderDeallocate));
		
		if (!t_success)
			MCMemoryDeallocate(t_buffer);
		
	}
	
	// IM-2014-05-20: [[ GraphicsPerformance ]] Opaque rasters should indicate no alpha in the bitmap info
	bool t_alpha;
	t_alpha = p_raster.format != kMCGRasterFormat_xRGB;
	
	// IM-2013-08-21: [[ RefactorGraphics ]] Refactor CGImage creation code to be pixel-format independent
	CGBitmapInfo t_bm_info;
	t_bm_info = MCGPixelFormatToCGBitmapInfo(kMCGPixelFormatNative, t_alpha);
	
	if (t_success)
		t_success = nil != (t_image = CGImageCreate(t_width, t_height, 8, 32, t_dst_stride, p_colorspace, t_bm_info, t_data_provider, nil, true, kCGRenderingIntentDefault));
	
	CGDataProviderRelease(t_data_provider);
	
	if (t_success)
		r_image = t_image;
	
	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

bool MCGImageToCGImage(MCGImageRef p_src, MCGRectangle p_src_rect, CGColorSpaceRef p_colorspace, bool p_copy, bool p_invert, CGImageRef &r_image)
{
	MCGRaster t_raster;
	
	if (!MCGImageGetRaster(p_src, t_raster))
		return false;
	
	return MCGRasterToCGImage(t_raster, p_src_rect, p_colorspace, p_copy, p_invert, r_image);
}

bool MCGImageToCGImage(MCGImageRef p_src, MCGRectangle p_src_rect, bool p_copy, bool p_invert, CGImageRef &r_image)
{
	bool t_success = true;
	
	/* OVERHAUL - REVISIT: for a grayscale image this should be CGColorSpaceCreateDeviceGray() */
	CGColorSpaceRef t_colorspace = nil;
	if (t_success)
		t_success = MCImageGetCGColorSpace(t_colorspace);
	
	if (t_success)
		t_success = MCGImageToCGImage(p_src, p_src_rect, t_colorspace, p_copy, p_invert, r_image);
	
	CGColorSpaceRelease(t_colorspace);
	
	return t_success;
}

bool MCImageBitmapToCGImage(MCImageBitmap *p_bitmap, CGColorSpaceRef p_colorspace, bool p_copy, bool p_invert, CGImageRef &r_image)
{
    if (p_bitmap == nil)
        return false;
	bool t_mask;
	t_mask = MCImageBitmapHasTransparency(p_bitmap);
	
	MCGRaster t_raster;
	t_raster = MCImageBitmapGetMCGRaster(p_bitmap, true);
	
	return MCGRasterToCGImage(t_raster, MCGRectangleMake(0, 0, p_bitmap->width, p_bitmap->height), p_colorspace, p_copy, p_invert, r_image);
}

bool MCImageBitmapToCGImage(MCImageBitmap *p_bitmap, bool p_copy, bool p_invert, CGImageRef &r_image)
{
	bool t_success = true;
	
	CGColorSpaceRef t_colorspace = nil;
	if (t_success)
		t_success = MCImageGetCGColorSpace(t_colorspace);
	
	if (t_success)
		t_success = MCImageBitmapToCGImage(p_bitmap, t_colorspace, p_copy, p_invert, r_image);
	
	CGColorSpaceRelease(t_colorspace);
	
	return t_success;
}

////////////////////////////////////////////////////////////////////////////////


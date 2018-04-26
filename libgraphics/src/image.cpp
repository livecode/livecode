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

#include "graphics.h"
#include "graphics-internal.h"

////////////////////////////////////////////////////////////////////////////////

static void MCGImageDestroy(MCGImageRef self)
{
	if (self != NULL)
	{
		if (self -> bitmap != NULL)
        {
            self -> bitmap -> unlockPixels();
			delete self -> bitmap;
        }
		MCMemoryDelete(self);
	}
}

bool MCGImageCreateWithSkBitmap(const SkBitmap &p_bitmap, MCGImageRef &r_image)
{
	bool t_success;
	t_success = true;
	
	__MCGImage *t_image;
	t_image = nil;
	if (t_success)
		t_success = MCMemoryNew(t_image);
	
	SkBitmap *t_bitmap;
	t_bitmap = nil;
	if (t_success)
	{
		t_bitmap = new (nothrow) SkBitmap(p_bitmap);
		t_success = nil != t_bitmap;
	}
	
	if (t_success)
	{
		t_bitmap -> lockPixels();
		t_image -> bitmap = t_bitmap;
		t_image -> is_valid = true;
		t_image -> references = 1;
		r_image = t_image;
	}
	else
	{
		if (t_bitmap != nil)
			delete t_bitmap;
		MCMemoryDelete(t_image);
	}
	
	return t_success;
}

bool MCGImageCreateWithRaster(const MCGRaster &p_raster, MCGPixelOwnershipType p_ownership, MCGImageRef &r_image)
{
	bool t_success;
	t_success = true;
	
	__MCGImage *t_image;
	t_image = nil;
	
	SkBitmap t_bitmap;
	
	if (t_success)
		t_success = MCGRasterToSkBitmap(p_raster, p_ownership, t_bitmap);
	
	if (t_success)
		t_success = MCGImageCreateWithSkBitmap(t_bitmap, t_image);
	
	if (t_success)
		r_image = t_image;
	else
		MCMemoryDelete(t_image);
	
	return t_success;	
}

bool MCGImageCreateWithRasterAndRelease(const MCGRaster &p_raster, MCGImageRef &r_image)
{
	return MCGImageCreateWithRaster(p_raster, kMCGPixelOwnershipTypeTake, r_image);
}

bool MCGImageCreateWithRasterNoCopy(const MCGRaster& p_raster, MCGImageRef& r_image)
{
	return MCGImageCreateWithRaster(p_raster, kMCGPixelOwnershipTypeBorrow, r_image);
}

bool MCGImageCreateWithRaster(const MCGRaster& p_raster, MCGImageRef& r_image)
{
	return MCGImageCreateWithRaster(p_raster, kMCGPixelOwnershipTypeCopy, r_image);
}

bool MCGImageGetRaster(MCGImageRef p_image, MCGRaster &r_raster)
{
	if (p_image == nil)
		return false;

	__MCGImage *t_image = (__MCGImage*)p_image;
	if (!t_image->is_valid || t_image->bitmap == nil)
		return false;

	// IM-2013-06-18: lock pixels here to force skia to update the pixel ptr
	// from the bitmap's pixel ref
	t_image->bitmap->lockPixels();
	
	// IM-2014-05-20: [[ GraphicsPerformance ]] Use bitmap opaqueness when setting the raster format.
	r_raster.format = MCGRasterFormatFromSkImageInfo(t_image->bitmap->info(), t_image->bitmap->isOpaque());
	r_raster.width = t_image->bitmap->width();
	r_raster.height = t_image->bitmap->height();
	r_raster.pixels = t_image->bitmap->getPixels();
	r_raster.stride =t_image->bitmap->rowBytes();

	t_image->bitmap->unlockPixels();
	
	return true;
}

bool MCGImageGetPixel(MCGImageRef p_image, uint32_t x, uint32_t y, uint32_t &r_pixel)
{
	MCGRaster t_raster;
	if (!MCGImageGetRaster(p_image, t_raster) || x >= t_raster.width || y >= t_raster.height)
		return false;

	uint8_t* t_row_ptr;
	t_row_ptr = ((uint8_t*)t_raster.pixels) + y * t_raster.stride;

	if (t_raster.format == kMCGRasterFormat_A)
		r_pixel = t_row_ptr[x];
	else
		r_pixel = ((uint32_t*)t_row_ptr)[x];

	return true;
}

bool MCGImageCreateWithData(const void *p_bytes, uindex_t p_byte_count, MCGImageRef& r_image)
{
	// TODO: Implement
	return false;
}

bool MCGImageCreateWithFilename(const char *p_filename, MCGImageRef& r_image)
{
	// TODO: Implement
	return false;
}

////////////////////////////////////////////////////////////////////////////////

MCGImageRef MCGImageRetain(MCGImageRef self)
{
	if (self != NULL)
        sk_atomic_inc((int32_t *)&self -> references);
	return self;	
}

void MCGImageRelease(MCGImageRef self)
{
	if (self != NULL)
	{
        if (sk_atomic_dec((int32_t *)&self -> references) == 1)
			MCGImageDestroy(self);
	}	
}

////////////////////////////////////////////////////////////////////////////////

bool MCGImageIsValid(MCGImageRef self)
{
	return self != NULL && 	self -> is_valid;
}

////////////////////////////////////////////////////////////////////////////////

int32_t MCGImageGetWidth(MCGImageRef self)
{
	return self -> bitmap -> width();
}

int32_t MCGImageGetHeight(MCGImageRef self)
{
	return self -> bitmap -> height();
}

MCGSize MCGImageGetSize(MCGImageRef self)
{
	MCGSize t_size;
	t_size . width = 0;
	t_size . height = 0;
	
	if (!MCGImageIsValid(self))
		return t_size;
	
	t_size . width = (MCGFloat) self -> bitmap -> width();
	t_size . height = (MCGFloat) self -> bitmap -> height();	
	return t_size;
}

////////////////////////////////////////////////////////////////////////////////

bool MCGImageIsOpaque(MCGImageRef self)
{
	return self != nil && self -> bitmap -> isOpaque();
}

bool MCGImageHasPartialTransparency(MCGImageRef self)
{
	if (self == nil)
		return false;

	if (self->bitmap->isOpaque())
		return false;

	MCGRaster t_raster;
	if (!MCGImageGetRaster(self, t_raster))
        return false;

	if (t_raster.format == kMCGRasterFormat_A)
		return true;

	uint8_t *t_row_ptr;
	t_row_ptr = (uint8_t*)t_raster.pixels;

	for (uint32_t y = 0; y < t_raster.height; y++)
	{
		uint32_t *t_pixel_ptr;
		t_pixel_ptr = (uint32_t*)t_row_ptr;
		for (uint32_t x = 0; x < t_raster.width; x++)
		{
			uint8_t t_alpha;
			t_alpha = MCGPixelGetNativeAlpha(*t_pixel_ptr++);
			if (t_alpha > 0 && t_alpha < 255)
				return true;
		}

		t_row_ptr += t_raster.stride;
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////////

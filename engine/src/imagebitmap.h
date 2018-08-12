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

#ifndef __MC_IMAGE_BITMAP_H__
#define __MC_IMAGE_BITMAP_H__

#include "graphics.h"
#include "sysdefs.h"

////////////////////////////////////////////////////////////////////////////////

// An MCImageBitmap is a non-premultiplied ARGB image, as decoded from the storage format
struct MCImageBitmap
{
	uint32_t width;
	uint32_t height;
	uint32_t stride;
	uint32_t *data;
	
	bool has_transparency;
	bool has_alpha;
};

struct MCImageIndexedBitmap
{
	uint32_t width;
	uint32_t height;
	uint32_t stride;
	uint8_t *data;

	uint32_t palette_size;
	uint32_t transparent_index;
	MCColor *palette;
};

////////////////////////////////////////////////////////////////////////////////

bool MCImageBitmapCreate(uindex_t p_width, uindex_t p_height, MCImageBitmap *&r_bitmap);
void MCImageFreeBitmap(MCImageBitmap *p_bitmap);
bool MCImageCopyBitmap(MCImageBitmap *p_bitmap, MCImageBitmap *&r_copy);
bool MCImageCopyBitmapRegion(MCImageBitmap *p_bitmap, MCRectangle &p_region, MCImageBitmap *&r_copy);

void MCImageBitmapClear(MCImageBitmap *p_bitmap);
void MCImageBitmapClearRegion(MCImageBitmap *p_bitmap, MCRectangle p_region);
void MCImageBitmapSet(MCImageBitmap *p_bitmap, uint32_t p_pixel_value);
void MCImageBitmapCopyRegionToBitmap(MCImageBitmap *p_src, MCImageBitmap *p_dst, int32_t sx, int32_t sy, int32_t dx, int32_t dy, uint32_t sw, uint32_t sh);
void MCImageBitmapCopyRegionToBitmap(MCImageBitmap *p_dst, MCImageBitmap *p_src, MCPoint p_dst_offset, MCRectangle p_src_rect);
void MCImageBitmapCopyRegionToBuffer(MCImageBitmap *p_bitmap, int32_t p_sx, int32_t p_sy, int32_t p_sw, int32_t p_sh, uindex_t p_buffer_stride, uint8_t *p_buffer);
void MCImageBitmapCopyRegionFromBuffer(MCImageBitmap *p_bitmap, MCRectangle &p_region, const uint8_t *p_buffer, uindex_t p_buffer_stride);

void MCImageBitmapCheckTransparency(MCImageBitmap *p_bitmap);
bool MCImageBitmapHasTransparency(MCImageBitmap *p_bitmap);
bool MCImageBitmapHasTransparency(MCImageBitmap *p_bitmap, bool &r_has_alpha);
uint32_t MCImageBitmapGetPixel(MCImageBitmap *p_bitmap, uindex_t x, uindex_t y);
void MCImageBitmapSetPixel(MCImageBitmap *p_bitmap, uindex_t x, uindex_t y, uint32_t p_pixel);

void MCImageBitmapPremultiply(MCImageBitmap *p_bitmap);
void MCImageBitmapPremultiplyRegion(MCImageBitmap *p_bitmap, int32_t p_sx, int32_t p_sy, int32_t p_sw, int32_t p_sh, uint32_t p_pixel_stride, uint32_t *p_pixel_ptr);
void MCImageBitmapUnpremultiply(MCImageBitmap *p_bitmap);
void MCImageBitmapUnpremultiplyChecking(MCImageBitmap *p_bitmap);
void MCImageBitmapFixPremultiplied(MCImageBitmap *p_bitmap);

//////////

bool MCImageCreateIndexedBitmap(uindex_t p_width, uindex_t p_height, MCImageIndexedBitmap *&r_indexed);
void MCImageFreeIndexedBitmap(MCImageIndexedBitmap *p_bitmap);

bool MCImageIndexedBitmapHasTransparency(MCImageIndexedBitmap *p_bitmap);
bool MCImageIndexedBitmapAddTransparency(MCImageIndexedBitmap *p_bitmap);

//////////

bool MCImageConvertIndexedToBitmap(MCImageIndexedBitmap *p_indexed, MCImageBitmap *&r_bitmap);
bool MCImageConvertBitmapToIndexed(MCImageBitmap *p_bitmap, bool p_ignore_transparent, MCImageIndexedBitmap *&r_indexed);
bool MCImageForceBitmapToIndexed(MCImageBitmap *p_bitmap, bool p_dither, MCImageIndexedBitmap *&r_indexed);

//////////

void MCImageBitmapExtractMask(MCImageBitmap *p_bitmap, void *p_mask, uint32_t p_mask_stride, uint8_t p_threshold);

////////////////////////////////////////////////////////////////////////////////

struct MCBitmapFrame
{
	MCImageBitmap *image;
	uint32_t duration;
	
	// IM-2013-10-30: [[ FullscreenMode ]] add density value to image frames
	// IM-2014-08-07: [[ Bug 13021 ]] Split density into x / y scale components
	MCGFloat x_scale;
	MCGFloat y_scale;
};

void MCImageFreeFrames(MCBitmapFrame *p_frames, uindex_t p_count);

////////////////////////////////////////////////////////////////////////////////

// MW-2013-05-04: Methods to determine the type of potential image data.
bool MCImageDataIsJPEG(MCDataRef p_data);
bool MCImageDataIsPNG(MCDataRef p_data);
bool MCImageDataIsGIF(MCDataRef p_data);
bool MCImageDataIsBMP(MCDataRef p_data);

////////////////////////////////////////////////////////////////////////////////

// IM-2014-04-14: [[ ImageRepUpdate ]] Legacy bitmap conversion
MCGRaster MCImageBitmapGetMCGRaster(MCImageBitmap *p_bitmap, bool p_is_premultiplied);
MCImageBitmap MCImageBitmapFromMCGRaster(const MCGRaster &p_raster);
bool MCImageBitmapCopyAsMCGImage(MCImageBitmap *p_bitmap, bool p_is_premultiplied, MCGImageRef &r_image);
bool MCImageBitmapCopyAsMCGImageAndRelease(MCImageBitmap *&x_bitmap, bool p_is_premultiplied, MCGImageRef &r_image);
// IM-2014-09-02: [[ Bug 13295 ]] Add optional target size param.
bool MCImageBitmapCreateWithTransformedMCGImage(MCGImageRef p_src, MCGAffineTransform p_transform, MCGImageFilter p_quality, const MCGIntegerSize *p_target_size, MCImageBitmap *&r_bitmap);

////////////////////////////////////////////////////////////////////////////////

#endif // __MC_IMAGE_BITMAP_H__

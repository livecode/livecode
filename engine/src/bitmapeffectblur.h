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

#ifndef __MC_BITMAP_EFFECT_BLUR__
#define __MC_BITMAP_EFFECT_BLUR__

typedef struct MCBitmapEffectBlur *MCBitmapEffectBlurRef;

enum MCBitmapEffectFilter
{
	// The faster (separated) 2*s*w*h algorithm.
	kMCBitmapEffectFilterFastGaussian,

	// One pass box blur filter
	kMCBitmapEffectFilterOnePassBox,

	// Two pass box blur filter
	kMCBitmapEffectFilterTwoPassBox,

	// Three pass box blur filter
	kMCBitmapEffectFilterThreePassBox
};

// This structure defines the parameters that affect the blur operation. At
// present it is only the radius and quality, but will include spread/range eventually.
struct MCBitmapEffectBlurParameters
{
	int32_t radius;
	uint8_t spread;
	MCBitmapEffectFilter filter;
};

// Start a blur effect using the given parameters, returning an opaque state
// object in r_blur.
//
// The output_rect defines the region in the plane for which mask values need
// to be computed. The input_rect defines the region in the plane that the
// provided 'pixels' pointer is defined. When generating the blurred mask, any
// pixels outside of the input region should be considered to be transparent.
// Note that the input pixels are ARGB (32-bit) and it is the alpha channel that
// is being blurred, while only an output mask (8-bit values) is required.
//
// MP-2013-02-05: [[ x64 ]] Change strides to be signed to avoid problems with
//   ptr arithmetic and promotions in 64-bit.
bool MCBitmapEffectBlurBegin(const MCBitmapEffectBlurParameters& params, const MCRectangle& input_rect, const MCRectangle& output_rect, uint32_t *src_pixels, int32_t src_stride, MCBitmapEffectBlurRef& r_blur);

// Continue a blur effect by generating the next output scanline.
//
// The mask argument points to a buffer of mask values big enough for a single
// output scanline. (i.e. output_rect . width in size).
//
void MCBitmapEffectBlurContinue(MCBitmapEffectBlurRef blur, uint8_t *mask);

// End the blur effect, and clean up any temporary resources.
//
void MCBitmapEffectBlurEnd(MCBitmapEffectBlurRef blur);

#endif

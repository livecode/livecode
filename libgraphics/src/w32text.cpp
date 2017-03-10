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

#include <Windows.h>

#include "graphics.h"
#include "graphics-internal.h"

#include "SkDevice.h"

////////////////////////////////////////////////////////////////////////////////

// MM-2014-07-31: [[ ThreadedRendering ]] Updated to make text rendering thread safe, by using TLS to ensure we have seperate DC for each thread.
//  This should probably be moved to a central thread library at some point, which will also help with clean up (we only clean up the main thread at the moment).

static bool s_initialized = false;
static HDC s_measure_dc = NULL;
static HDC s_draw_dc = NULL;

////////////////////////////////////////////////////////////////////////////////

static inline uint32_t packed_scale_bounded(uint32_t x, uint8_t a)
{
	uint32_t u, v;

	u = (x & 0xff00ff) * a + 0x800080;
	u = ((u + ((u >> 8) & 0xff00ff)) >> 8) & 0xff00ff;

	v = ((x >> 8) & 0xff00ff) * a + 0x800080;
	v = (v + ((v >> 8) & 0xff00ff)) & 0xff00ff00;

	return u | v;
}

static inline uint32_t packed_bilinear_bounded(uint32_t x, uint8_t a, uint32_t y, uint8_t b)
{
	uint32_t u, v;

	u = (x & 0xff00ff) * a + (y & 0xff00ff) * b + 0x800080;
	u = ((u + ((u >> 8) & 0xff00ff)) >> 8) & 0xff00ff;

	v = ((x >> 8) & 0xff00ff) * a + ((y >> 8) & 0xff00ff) * b + 0x800080;
	v = (v + ((v >> 8) & 0xff00ff)) & 0xff00ff00;

	return u | v;
}

////////////////////////////////////////////////////////////////////////////////

// MW-2014-01-14: [[ Bug 11664 ]] Make sure we render text with appropriate fill
//   colors for transparent and opaque pixels and blend the two resulting bitmaps
//   separately.

// Drawing text consistently with other apps is a little more involved than
// we'd like on Windows due to ClearType and the need to render into potentially
// partially transparent backgrounds.
//
// For each background pixel which is fully opaque, we need to render the text
// direct on top in the appropriate fill color as ClearType filtering uses both
// the fill color and the destination pixel to compute the output value.
//
// For each background pixel which is not opaque, we need to render the text
// against black or white (depending on the intensity of the fill color) in
// white or black (respectively) and take a weighted average of the RGB values
// (ClearType uses LCD style rendering, spreading the alpha across the three
// color channels); then use that as a mask to blend in the fill color.
//
// Unfortunately, this means we need to potentially render the text twice - once
// for the opaque pixels, and once for the non-opaque pixels - and then composite
// appropriately.
//

static void w32_draw_text_render_transparent_buffer(HDC p_gdi_context, const unichar_t *p_text, uindex_t p_text_length, bool p_nearest_white, int32_t p_height, uint32_t *p_rgb_pxls, int32_t p_rgb_width, bool p_rtl)
{
	// Clear the RGB buffer to black or white.
	memset(p_rgb_pxls, p_nearest_white ? 0x00 : 0xff, p_rgb_width * p_height * sizeof(uint32_t));

	// The text color is the inverse of what the RGB buffer has been set to.
	SetTextColor(p_gdi_context, p_nearest_white ? RGB(0xff, 0xff, 0xff) : RGB(0x00, 0x00, 0x00));

	SetTextAlign(p_gdi_context, TA_BASELINE | TA_LEFT | TA_NOUPDATECP);

	// Render the text.
    ExtTextOutW(p_gdi_context, 0, 0, p_rtl ? ETO_RTLREADING : 0, NULL, (LPCWSTR)p_text, p_text_length, NULL);

	// Make sure GDI has finished.
	GdiFlush();
}

static bool w32_draw_text_process_transparent_buffer(uint32_t p_fill_color, bool p_nearest_white, int32_t p_height, uint32_t *p_rgb_pxls, int32_t p_rgb_width, uint32_t *p_context_pxls, int32_t p_context_width)
{
	// This is set to true if we encounter a fully opaque pixel.
	bool t_has_opaque;
	t_has_opaque = false;

	for (uint32_t y = 0; y < p_height; y++)
	{
		for (uint32_t x = 0; x < p_rgb_width; x++)
		{
			uint32_t *t_context_pxl;
			t_context_pxl = p_context_pxls + y * p_context_width + x;

			uint32_t *t_rgb_pxl;
			t_rgb_pxl = p_rgb_pxls + y * p_rgb_width + x;
			
			// If the destination pixel is opaque, then set it to transparent black
			// and flag for opacity. Otherwise; take an average of the three RGB
			// components and blend with the fill color.
			if (false && ((*t_context_pxl >> 24) & 0xFF) == 0xFF)
			{
				*t_rgb_pxl = 0x00000000;
				t_has_opaque = true;
			}
			else
			{
				uint32_t t_lcd32_val;
				t_lcd32_val = *t_rgb_pxl;
				
				uint8_t t_a8_val;
				t_a8_val = ((t_lcd32_val & 0xFF) + ((t_lcd32_val & 0xFF00) >> 8) + ((t_lcd32_val & 0xFF0000) >> 16)) / 3;
				
				if (!p_nearest_white)
					t_a8_val = 255 - t_a8_val;

				*t_rgb_pxl = packed_scale_bounded(p_fill_color, t_a8_val);
			}
		}
	}

	return t_has_opaque;
}

static void w32_draw_text_render_opaque_buffer(HDC p_gdi_context, const unichar_t *p_text, uindex_t p_text_length, uint32_t p_fill_color, int32_t p_height, uint32_t *p_rgb_pxls, int32_t p_rgb_width, uint32_t *p_context_pxls, int32_t p_context_width, bool p_rtl)
{
	// Copy the current pixels in the context across to the RGB buffer.
	for(int32_t y = 0; y < p_height; y++)
		memcpy(p_rgb_pxls + y * p_rgb_width, p_context_pxls + y * p_context_width, p_rgb_width * sizeof(uint32_t));

	// The text color is the fill color.
	SetTextColor(p_gdi_context, RGB((p_fill_color >> 16) & 0xFF, (p_fill_color >> 8) & 0xFF, (p_fill_color >> 0) & 0xFF));

	SetTextAlign(p_gdi_context, TA_BASELINE | TA_LEFT | TA_NOUPDATECP);

	// Render the text.
	ExtTextOutW(p_gdi_context, 0, 0, p_rtl ? ETO_RTLREADING : 0, NULL, (LPCWSTR)p_text, p_text_length, NULL);

	// Make sure GDI has finished.
	GdiFlush();
}

static bool w32_draw_text_process_opaque_buffer(uint32_t p_fill_color, int32_t p_height, uint32_t *p_rgb_pxls, int32_t p_rgb_width, uint32_t *p_context_pxls, int32_t p_context_width)
{
	// This is set to true if we encounter a transparent pixel.
	bool t_has_transparency;
	t_has_transparency = false;

	for (uint32_t y = 0; y < p_height; y++)
	{
		for (uint32_t x = 0; x < p_rgb_width; x++)
		{
			uint32_t *t_context_pxl;
			t_context_pxl = p_context_pxls + y * p_context_width + x;

			uint32_t *t_rgb_pxl;
			t_rgb_pxl = p_rgb_pxls + y * p_rgb_width + x;
			
			// If the destination pixel is opaque, then just make sure our
			// RGB pixel has 0xff for its alpha (GDI does not preserve this);
			// otherwise set the rgb pixel to transparent black and flag that
			// we have transparency.
			if (((*t_context_pxl >> 24) & 0xFF) == 0xFF)
				*t_rgb_pxl = *t_rgb_pxl | 0xFF000000;
			else
			{
				*t_rgb_pxl = 0x00000000;
				t_has_transparency = true;
			}
		}
	}

	return t_has_transparency;
}

static bool w32_draw_text_using_mask_to_context_at_device_location(MCGContextRef p_context, const unichar_t *p_text, uindex_t p_length, MCGPoint p_location, MCGIntRectangle p_bounds, HDC p_gdicontext, bool p_rtl)
{
	bool t_success;
	t_success = true;

	// Setup the output RGB buffer used by GDI to render into.
	void *t_rgb_data;
	t_rgb_data = NULL;
	HBITMAP t_rgb_bitmap;
	t_rgb_bitmap = NULL;
	if (t_success)
	{
		BITMAPINFO t_bitmapinfo;
		MCMemoryClear(&t_bitmapinfo, sizeof(BITMAPINFO));
		t_bitmapinfo . bmiHeader . biSize = sizeof(BITMAPINFOHEADER);
		t_bitmapinfo . bmiHeader . biCompression = BI_RGB;
		t_bitmapinfo . bmiHeader . biWidth = p_bounds . width;
		t_bitmapinfo . bmiHeader . biHeight = -p_bounds . height;
		t_bitmapinfo . bmiHeader . biPlanes = 1;
		t_bitmapinfo . bmiHeader . biBitCount = 32;
		t_rgb_bitmap = CreateDIBSection(p_gdicontext, &t_bitmapinfo, DIB_RGB_COLORS, &t_rgb_data, NULL, 0);
		t_success = t_rgb_bitmap != NULL && t_rgb_data != NULL;
	}

	HGDIOBJ t_old_object;
	if (t_success)
	{
		t_old_object = SelectObject(p_gdicontext, t_rgb_bitmap);
		t_success = t_old_object != NULL;
	}

	// If the color is 'nearest_white' then we composite white against black for transparent
	// pixels; otherwise we composite black against white.
	bool t_nearest_white;
	if (t_success)
		t_nearest_white = (0.3 * (p_context -> state -> fill_color & 0xff) + 0.59 * ((p_context -> state -> fill_color >> 8) & 0xff) + 0.11 * ((p_context -> state -> fill_color >> 16) & 0xff)) >= 127.0;

	// Setup various pointers and resources for the rendering that is required.
	uint32_t *t_rgb_pxls, *t_context_pxls;
	uint32_t t_context_width;
	SkPaint t_paint;
	SkBitmap t_bitmap;
	if (t_success)
	{
		t_rgb_pxls = (uint32_t *) t_rgb_data;

		int32_t t_x_offset, t_y_offset;
		t_x_offset = p_location . x + p_bounds . x;
		t_y_offset = p_location . y + p_bounds . y;

		// Compute the top-left of the pixels in the context (same as 0,0 in the rgb pixels).
		t_context_pxls = (uint32_t *) p_context -> layer -> canvas -> getTopDevice() -> accessBitmap(false) . getPixels();
		t_context_width = p_context -> layer -> canvas -> getTopDevice() -> accessBitmap(false) . rowBytes() / 4;
		t_context_pxls += t_y_offset * t_context_width + t_x_offset;

		t_paint . setStyle(SkPaint::kFill_Style);	
		t_paint . setAntiAlias(p_context -> state -> should_antialias);

		t_paint.setBlendMode(MCGBlendModeToSkBlendMode(p_context->state->blend_mode));		
		
		t_bitmap . setInfo(SkImageInfo::MakeN32Premul(p_bounds.width, p_bounds.height));
		t_bitmap . setPixels(t_rgb_data);

		SetBkMode(p_gdicontext, TRANSPARENT);
	}

	// We want all drawing to ignore the transform matrix within this function
	p_context->layer->canvas->save();
	p_context->layer->canvas->resetMatrix();

	// Now process depending on the first pixel - if transparent, then first process
	// transparency then process opacity; otherwise vice-versa.
	if (t_success)
	{
		if (true || (*t_context_pxls >> 24) != 0xff)
		{
			// Render the text for the transparent pixels (white/black text on black/white background).
			w32_draw_text_render_transparent_buffer(p_gdicontext, p_text, p_length >> 1, t_nearest_white, p_bounds . height, t_rgb_pxls, p_bounds . width, p_rtl);
			
			// Process the buffer - this sets all opaque pixels to fully transparent, and blends
			// the fill color with the semi-opaque pixels. It returns whether there are any opaque
			// pixels to process.
			bool t_has_opaque;
			t_has_opaque = w32_draw_text_process_transparent_buffer(p_context -> state -> fill_color, t_nearest_white, p_bounds . height, t_rgb_pxls, p_bounds . width, t_context_pxls, t_context_width);

			// Blend the bitmap into the background (we definitely have transparent pixels).
			t_bitmap . setAlphaType(kPremul_SkAlphaType);	
			p_context -> layer -> canvas -> drawBitmap(t_bitmap, p_bounds . x + p_location . x, p_bounds . y + p_location . y, &t_paint);

			// If there are any opaque pixels, process those.
			if (t_has_opaque)
			{
				// Render the text for the opaque pixels (text with given fill color blended against
				// destination pixels).
				w32_draw_text_render_opaque_buffer(p_gdicontext, p_text, p_length >> 1, p_context -> state -> fill_color, p_bounds . height, t_rgb_pxls, p_bounds . width, t_context_pxls, t_context_width, p_rtl);

				// Process the buffer - this ensures the alpha byte of all opaque pixels
				// is 0xff (GDI doesn't preserve such things) and sets all non-opaque pixels
				// to full transparent.
				w32_draw_text_process_opaque_buffer(p_context -> state -> fill_color, p_bounds . height, t_rgb_pxls, p_bounds . width, t_context_pxls, t_context_width);

				// Blend the bitmap into the background (we definitely have transparent pixels since
				// we've set some so!).
				t_bitmap . setAlphaType(kPremul_SkAlphaType);	
				p_context -> layer -> canvas -> drawBitmap(t_bitmap, p_bounds . x + p_location . x, p_bounds . y + p_location . y, &t_paint);
			}
		}
		else
		{
			// Render the text for the opaque pixels (text with given fill color blended against
			// destination pixels).
			w32_draw_text_render_opaque_buffer(p_gdicontext, p_text, p_length >> 1, p_context -> state -> fill_color, p_bounds . height, t_rgb_pxls, p_bounds . width, t_context_pxls, t_context_width, p_rtl);

			// Process the buffer - this ensures the alpha byte of all opaque pixels
			// is 0xff (GDI doesn't preserve such things) and sets all non-opaque pixels
			// to full transparent. It returns whether there are any transparent pixels.
			bool t_has_transparent;
			t_has_transparent = w32_draw_text_process_opaque_buffer(p_context -> state -> fill_color, p_bounds . height, t_rgb_pxls, p_bounds . width, t_context_pxls, t_context_width);

			// Blend the bitmap into the background (we may have transparent pixels).
			if (t_has_transparent)
				t_bitmap . setAlphaType(kPremul_SkAlphaType);
			else
				t_bitmap . setAlphaType(kOpaque_SkAlphaType);	
			p_context -> layer -> canvas -> drawBitmap(t_bitmap, p_bounds . x + p_location . x, p_bounds . y + p_location . y, &t_paint);

			// If there are any transparent pixels, process those.
			if (t_has_transparent)
			{
				// Render the text for the transparent pixels (white/black text on black/white background).
				w32_draw_text_render_transparent_buffer(p_gdicontext, p_text, p_length >> 1, t_nearest_white, p_bounds . height, t_rgb_pxls, p_bounds . width, p_rtl);
				
				// Process the buffer - this sets all opaque pixels to fully transparent, and blends
				// the fill color with the semi-opaque pixels.
				w32_draw_text_process_transparent_buffer(p_context -> state -> fill_color, t_nearest_white, p_bounds . height, t_rgb_pxls, p_bounds . width, t_context_pxls, t_context_width);

				// Blend the bitmap into the background (we definitely have transparent pixels).
				t_bitmap . setAlphaType(kPremul_SkAlphaType);
				p_context -> layer -> canvas -> drawBitmap(t_bitmap, p_bounds . x + p_location . x, p_bounds . y + p_location . y, &t_paint);
			}
		}
	}

	// Restore the canvas' matrix
	p_context->layer->canvas->restore();

	if (t_rgb_bitmap != NULL)
	{
		SelectObject(p_gdicontext, t_old_object);
		DeleteObject(t_rgb_bitmap);
	}

	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

static void __MCGContextDrawPlatformTextScreen(MCGContextRef self, const unichar_t *p_text, uindex_t p_length, MCGPoint p_location, const MCGFont &p_font, bool p_rtl)
{
    if (!MCGContextIsValid(self))
		return;	
	
	bool t_success;
	t_success = true;
	
	if (t_success)
		t_success = SelectObject(s_draw_dc, p_font . fid) != NULL;
	
	TEXTMETRICA t_metrics;
	if (t_success)
		t_success = GetTextMetricsA(s_draw_dc, &t_metrics);

	// MM-2013-12-16: [[ Bug 11564 ]] Take into account any overhang of italic text to prevent clipping.
	// MM-2014-04-22: [[ Bug 11904 ]] Also take into account any underhang of the first char. Use this to offset
	//  the x-location we draw at. Also, make sure we measure the correct char - added a fudge on the overhang as a result.
	MCGFloat t_overhang, t_x_offset;
	if (t_success)
	{
		ABCFLOAT t_abc_widths;
		if (GetCharABCWidthsFloatW(s_draw_dc, *(p_text + p_length / 2 - 1), *(p_text + p_length / 2 - 1), &t_abc_widths) != 0)
			t_overhang = fabs(t_abc_widths . abcfC) + 1;
		else
			t_overhang = 0.0f;

		if (GetCharABCWidthsFloatW(s_draw_dc, *(p_text), *(p_text), &t_abc_widths) != 0)
			t_x_offset = t_abc_widths . abcfA, t_overhang += fabs(t_abc_widths . abcfA);
		else
			t_x_offset = 0;
	}
	
	MCGIntRectangle t_text_bounds, t_clipped_bounds;
	MCGAffineTransform t_transform;
	MCGPoint t_device_location;
	if (t_success)
	{	
		t_transform = MCGContextGetDeviceTransform(self);
		t_device_location = MCGPointApplyAffineTransform(p_location, t_transform);
		
		// MM-2014-04-16: [[ Bug 11964 ]] Use MCGContextMeasurePlatformText to fetch the width of the text.
		// MM-2014-04-22: [[ Bug 11904 ]] Offset the x value to take into account any underhang of the first char of italic text.
		MCGRectangle t_float_text_bounds;
		// AL-2014-07-16: [[ Bug 12488 ]] Fudge the text bounds back by to prevent clipping text
		t_float_text_bounds . origin . x = t_x_offset - 1;
		t_float_text_bounds . origin . y = -t_metrics . tmAscent;
		t_float_text_bounds . size . width = MCGContextMeasurePlatformText(self, p_text, p_length, p_font, t_transform) + t_overhang;
		t_float_text_bounds . size . height = t_metrics . tmAscent + t_metrics . tmDescent;
		
		// IM-2013-09-02: [[ RefactorGraphics ]] modff will round to zero rather than
		// negative infinity (which we need) so calculate manually
		MCGFloat t_int_x, t_int_y;
		t_int_x = floor(t_device_location.x);
		t_int_y = floor(t_device_location.y);
		
		t_transform . tx = t_device_location . x - t_int_x;
		t_transform . ty = t_device_location . y - t_int_y;
		t_device_location . x = t_int_x;
		t_device_location . y = t_int_y;

		// IM-2013-09-02: [[ RefactorGraphics ]] constrain device clip rect to device bounds
		SkISize t_device_size;
		t_device_size = self->layer->canvas->getDeviceSize();
		
		MCGRectangle t_device_bounds;
		t_device_bounds = MCGRectangleMake(0, 0, t_device_size.width(), t_device_size.height());
		
		MCGRectangle t_device_clip;
		t_device_clip = MCGRectangleIntersection(MCGContextGetDeviceClipBounds(self), t_device_bounds);
		
		t_device_clip . origin . x -= t_device_location . x;
		t_device_clip . origin . y -= t_device_location . y;
		
		MCGRectangle t_float_clipped_bounds;
		t_float_text_bounds = MCGRectangleApplyAffineTransform(t_float_text_bounds, t_transform);		
		t_float_clipped_bounds = MCGRectangleIntersection(t_float_text_bounds, t_device_clip);
		
		t_text_bounds = MCGRectangleIntegerBounds(t_float_text_bounds);
		t_clipped_bounds = MCGRectangleIntegerBounds(t_float_clipped_bounds);
		
		if (t_clipped_bounds . width == 0 || t_clipped_bounds . height == 0)
			return;
	}
	
	if (t_success)
	{
		// MM-2014-04-22: [[ Bug 11904 ]] Take into account any x-offset of the bounds.
		XFORM t_xform;
		t_xform . eM11 = t_transform . a;
		t_xform . eM12 = t_transform . b;
		t_xform . eM21 = t_transform . c;
		t_xform . eM22 = t_transform . d;
		t_xform . eDx = t_transform . tx - (t_clipped_bounds . x);
		t_xform . eDy = t_transform . ty - (t_clipped_bounds . y);
		t_success = SetWorldTransform(s_draw_dc, &t_xform);
	}

	if (t_success)
        t_success = w32_draw_text_using_mask_to_context_at_device_location(self, p_text, p_length, t_device_location, t_clipped_bounds, s_draw_dc, p_rtl);

	self -> is_valid = t_success;
}

static bool __MCGContextTracePlatformText(MCGContextRef self, const unichar_t *p_text, uindex_t p_length, const MCGFont& p_font, MCGPathRef& r_path, bool p_rtl)
{
	// TODO: RTL
    
    bool t_success;
	t_success = true;
	
	HDC t_gdicontext;
	t_gdicontext = NULL;
	if (t_success)
	{
		t_gdicontext = CreateCompatibleDC(NULL);
		t_success = t_gdicontext != NULL;
	}
	
	HFONT t_new_font;
	if (t_success)
	{
		LOGFONTA t_font;
		GetObjectA(p_font . fid, sizeof(LOGFONTA), &t_font);
		t_font . lfHeight = -256;

		t_new_font = CreateFontIndirectA(&t_font);
		t_success = SelectObject(t_gdicontext, t_new_font) != NULL;
	}

	TEXTMETRICA t_metrics;
	if (t_success)
		t_success = GetTextMetricsA(t_gdicontext, &t_metrics);

	if (t_success)
	{
		BeginPath(t_gdicontext);
		SetBkMode(t_gdicontext, TRANSPARENT);
		SetTextAlign(t_gdicontext, TA_BASELINE);
		if (p_font . fixed_advance == 0)
			TextOutW(t_gdicontext, 0, 0, p_text, p_length / 2);
		else
		{
			// MW-2013-12-05: [[ Bug 11535 ]] If fixed advance, then make the advance
			//   width of each char fixed_advance.
			INT *t_dxs;
			/* UNCHECKED */ MCMemoryNewArray(p_length / 2, t_dxs);
			for(uindex_t i = 0; i < p_length / 2; i++)
				t_dxs[i] = p_font . fixed_advance * 256 / p_font . size;
			ExtTextOutW(t_gdicontext, 0, 0, 0, NULL, p_text, p_length / 2, t_dxs);
			MCMemoryDeleteArray(t_dxs);
		}
		EndPath(t_gdicontext);

		int t_count;
		t_count = GetPath(t_gdicontext, NULL, NULL, 0);

		POINT *t_points;
		BYTE *t_types;
		t_points = new (nothrow) POINT[t_count];
		t_types = new (nothrow) BYTE[t_count];
		GetPath(t_gdicontext, t_points, t_types, t_count);

		MCGPathCreateMutable(r_path);
		for(int i = 0; i < t_count;)
		{
			switch(t_types[i] & ~PT_CLOSEFIGURE)
			{
			case PT_MOVETO:
				MCGPathMoveTo(r_path, MCGPointMake(t_points[i] . x * p_font . size / 256.0f, t_points[i] . y * p_font . size / 256.0f));
				i += 1;
				break;

			case PT_LINETO:
				MCGPathLineTo(r_path, MCGPointMake(t_points[i] . x * p_font . size / 256.0f, t_points[i] . y * p_font . size / 256.0f));
				if (t_types[i] & PT_CLOSEFIGURE)
					MCGPathCloseSubpath(r_path);
				i += 1;
				break;

			case PT_BEZIERTO:
				MCGPathCubicTo(r_path,
								MCGPointMake(t_points[i] . x * p_font . size / 256.0f, t_points[i] . y * p_font . size / 256.0f),
								MCGPointMake(t_points[i + 1] . x * p_font . size / 256.0f, t_points[i + 1] . y * p_font . size / 256.0f),
								MCGPointMake(t_points[i + 2] . x * p_font . size / 256.0f, t_points[i + 2] . y * p_font . size / 256.0f));
				if (t_types[i] & PT_CLOSEFIGURE)
					MCGPathCloseSubpath(r_path);
				i += 3;
				break;
			}
		}

		MCGPathCopyAndRelease(r_path, r_path);

		delete t_points;
		delete t_types;
	}

	DeleteDC(t_gdicontext);
	DeleteObject(t_new_font);

	return true;
}

// MW-2013-11-07: [[ Bug 11393 ]] Render the text using ideal metrics. At the moment
//   this works by tracing the path at 256px size, converting to a MCGPath (scaled)
//   then filling as a path. At some point it would be worth upgrading this code path
//   to use GDI+/DirectWrite (depending on Windows version) as these should give better
//   quality and performance (particularly for smaller < 15pt text sizes).
static void __MCGContextDrawPlatformTextIdeal(MCGContextRef self, const unichar_t *p_text, uindex_t p_length, MCGPoint p_location, const MCGFont &p_font, bool p_rtl)
{
    if (!MCGContextIsValid(self))
		return;	

	MCGPathRef t_path;
	__MCGContextTracePlatformText(self, p_text, p_length, p_font, t_path, p_rtl);

	MCGContextSave(self);
	MCGContextTranslateCTM(self, p_location . x, p_location . y);
	MCGContextBeginPath(self);
	MCGContextAddPath(self, t_path);
	MCGContextSetShouldAntialias(self, true);
	MCGContextFill(self);
	MCGContextRestore(self);

	MCGPathRelease(t_path);
}

// MW-2013-11-07: [[ Bug 11393 ]] What codepath we use depends on whether we are
//   using ideal metrics or not.
void MCGContextDrawPlatformText(MCGContextRef self, const unichar_t *p_text, uindex_t p_length, MCGPoint p_location, const MCGFont& p_font, bool p_rtl)
{
	if (p_font . ideal)
	{
		__MCGContextDrawPlatformTextIdeal(self, p_text, p_length, p_location, p_font, p_rtl);
		return;
	}

	__MCGContextDrawPlatformTextScreen(self, p_text, p_length, p_location, p_font, p_rtl);
}

//////////

// MM-2014-04-16: [[ Bug 11964 ]] Updated prototype to take transform parameter.
MCGFloat __MCGContextMeasurePlatformTextScreen(MCGContextRef self, const unichar_t *p_text, uindex_t p_length, const MCGFont &p_font, const MCGAffineTransform &p_transform)
{	
	bool t_success;
	t_success = true;

	if (t_success)
		t_success = SelectObject(s_measure_dc, p_font . fid) != NULL;

	// MM-2014-04-16: [[ Bug 11964 ]] Take into account any transform passed. Windows doesn't scale text
	//  linearly, so if the text we are measuring is to be drawn scaled, or measurement needs to be adjusted.
	//  GetTextExtentPoint32 returns logical units, so we don't need to revserse the transfom to convert back.
	if (t_success)
	{
		XFORM t_xform;
		t_xform . eM11 = p_transform . a;
		t_xform . eM12 = p_transform . b;
		t_xform . eM21 = p_transform . c;
		t_xform . eM22 = p_transform . d;
		t_xform . eDx = 0;
		t_xform . eDy = 0;
		t_success = SetWorldTransform(s_measure_dc, &t_xform);
	}

	SIZE t_size;
	if (t_success)
		t_success = GetTextExtentPoint32W(s_measure_dc, (LPCWSTR)p_text, p_length >> 1, &t_size);
	
	if (t_success)
		return t_size . cx;
	else
		return 0.0;
}

// MW-2013-11-07: [[ Bug 11393 ]] Measure the text using 'ideal' metrics these
//   are essentially the font metrics at 256 pixels high, scaled linearly to
//   the requested font size.
MCGFloat __MCGContextMeasurePlatformTextIdeal(MCGContextRef self, const unichar_t *p_text, uindex_t p_length, const MCGFont &p_font)
{	
	bool t_success;
	t_success = true;
	
	// MW-2013-12-05: [[ Bug 11535 ]] If fixed advance, then the width of the string
	//   is taken to be char_count * fixed_advance. (Note this isn't necessarily
	//   correct for combining chars, but the fixed width fonts on Windows don't seem
	//   to combine).
	if (p_font . fixed_advance != 0)
		return p_font . fixed_advance * (p_length >> 1);

	HDC t_gdicontext;
	t_gdicontext = NULL;
	if (t_success)
	{
		t_gdicontext = CreateCompatibleDC(NULL);
		t_success = t_gdicontext != NULL;
	}
	
	if (t_success)
		t_success = SetGraphicsMode(t_gdicontext, GM_ADVANCED) != 0;
	
	HFONT t_new_font;
	if (t_success)
	{
		LOGFONTA t_font;
		GetObjectA(p_font . fid, sizeof(LOGFONTA), &t_font);
		t_font . lfHeight = -256;

		t_new_font = CreateFontIndirectA(&t_font);
		t_success = SelectObject(t_gdicontext, t_new_font) != NULL;
	}
	
	SIZE t_size;
	if (t_success)
		t_success = GetTextExtentPoint32W(t_gdicontext, (LPCWSTR)p_text, p_length >> 1, &t_size);

	DeleteDC(t_gdicontext);
	DeleteObject(t_new_font);
	
	if (t_success)
		return t_size . cx * p_font . size / 256.0f;
	else
		return 0.0;
}

// MW-2013-11-07: [[ Bug 11393 ]] What codepath we use depends on whether we are
//   using ideal metrics or not.
// MM-2014-04-16: [[ Bug 11964 ]] Updated prototype to take transform parameter.
MCGFloat __MCGContextMeasurePlatformText(MCGContextRef self, const unichar_t *p_text, uindex_t p_length, const MCGFont &p_font, const MCGAffineTransform &p_transform)
{
	if (p_font . ideal)
		return __MCGContextMeasurePlatformTextIdeal(self, p_text, p_length, p_font);

	return __MCGContextMeasurePlatformTextScreen(self, p_text, p_length, p_font, p_transform);
}

////////////////////////////////////////////////////////////////////////////////

void MCGPlatformInitialize(void)
{
	if (s_initialized)
		return;

	s_initialized = true;

	s_measure_dc = CreateCompatibleDC(NULL);
	SetGraphicsMode(s_measure_dc, GM_ADVANCED);

	s_draw_dc = CreateCompatibleDC(NULL);
	SetGraphicsMode(s_draw_dc, GM_ADVANCED);
}

void MCGPlatformFinalize(void)
{
	if (!s_initialized)
		return;

	DeleteDC(s_draw_dc);
	DeleteDC(s_measure_dc);
	s_initialized = false;
}

////////////////////////////////////////////////////////////////////////////////

static void __expand_bounds(RECT& r, POINT& p)
{
    if (p . x < r . left)
        r . left = p . x;
    if (p . x > r . right)
        r . right = p . x;
    if (p . y < r . top)
        r . top = p . y;
    if (p . y > r . bottom)
        r . bottom = p . y;
}

bool MCGContextMeasurePlatformTextImageBounds(MCGContextRef self, const unichar_t *p_text, uindex_t p_length, const MCGFont& p_font, const MCGAffineTransform& p_transform, MCGRectangle &r_bounds)
{
    bool t_success;
    t_success = true;

    if (t_success)
        t_success = SelectObject(s_measure_dc, p_font . fid) != NULL;

    if (t_success)
    {
        XFORM t_xform;
        t_xform . eM11 = p_transform . a;
        t_xform . eM12 = 0;
        t_xform . eM21 = 0;
        t_xform . eM22 = p_transform . d;
        t_xform . eDx = 0;
        t_xform . eDy = 0;
        t_success = SetWorldTransform(s_measure_dc, &t_xform);
    }

    if (t_success)
    {
        BeginPath(s_measure_dc);
        SetBkMode(s_measure_dc, TRANSPARENT);
        SetTextAlign(s_measure_dc, TA_BASELINE);
        if (p_font . fixed_advance == 0)
            TextOutW(s_measure_dc, 0, 0, p_text, p_length / 2);
        else
        {
            // MW-2013-12-05: [[ Bug 11535 ]] If fixed advance, then make the advance
            //   width of each char fixed_advance.
            INT *t_dxs;
            /* UNCHECKED */ MCMemoryNewArray(p_length / 2, t_dxs);
            for(uindex_t i = 0; i < p_length / 2; i++)
                t_dxs[i] = p_font . fixed_advance;
            ExtTextOutW(s_measure_dc, 0, 0, 0, NULL, p_text, p_length / 2, t_dxs);
            MCMemoryDeleteArray(t_dxs);
        }
        EndPath(s_measure_dc);

        int t_count;
        t_count = GetPath(s_measure_dc, NULL, NULL, 0);

        RECT t_bounds;
        t_bounds . left = INT_MAX;
        t_bounds . top = INT_MAX;
        t_bounds . right = INT_MIN;
        t_bounds . bottom = INT_MIN;

        POINT *t_points;
        BYTE *t_types;
        t_points = new (nothrow) POINT[t_count];
        t_types = new (nothrow) BYTE[t_count];
        GetPath(s_measure_dc, t_points, t_types, t_count);

        for(int i = 0; i < t_count;)
        {
            switch(t_types[i] & ~PT_CLOSEFIGURE)
            {
                case PT_MOVETO:
                    __expand_bounds(t_bounds, t_points[i]);
                    i += 1;
                    break;

                case PT_LINETO:
                    __expand_bounds(t_bounds, t_points[i]);
                    i += 1;
                    break;

                case PT_BEZIERTO:
                    __expand_bounds(t_bounds, t_points[i + 0]);
                    __expand_bounds(t_bounds, t_points[i + 1]);
                    __expand_bounds(t_bounds, t_points[i + 2]);
                    i += 3;
                    break;
            }
        }

        delete t_points;
        delete t_types;

		r_bounds = MCGRectangleMake(t_bounds.left, t_bounds.top, t_bounds.right - t_bounds.left, t_bounds.bottom - t_bounds.top);
    }

    AbortPath(s_measure_dc);

    return true;
}

////////////////////////////////////////////////////////////////////////////////

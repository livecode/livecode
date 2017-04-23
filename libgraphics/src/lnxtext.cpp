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

#include <pango/pangoft2.h>
#include <glib.h>
#include <pthread.h>

////////////////////////////////////////////////////////////////////////////////

// MW-2013-12-19: [[ Bug 11559 ]] If 'false' after initializing we have no
//   text support as the needed libs are not available.
static bool s_has_text_support = false;

////////////////////////////////////////////////////////////////////////////////

extern "C" int initialise_weak_link_pango();
extern "C" int initialise_weak_link_pangoft2();
extern "C" int initialise_weak_link_glib();
extern "C" int initialise_weak_link_gobject();

extern void MCCStringFree(char *p_string);
extern bool MCCStringFromUnicodeSubstring(const unichar_t *p_chars, uint32_t p_char_count, char*& r_string);

////////////////////////////////////////////////////////////////////////////////

static inline MCGRectangle MCGRectangleFromPangoRectangle(const PangoRectangle &p_rect)
{
	return MCGRectangleMake(p_rect.x / (MCGFloat)PANGO_SCALE, p_rect.y / (MCGFloat)PANGO_SCALE, p_rect.width / (MCGFloat)PANGO_SCALE, p_rect.height / (MCGFloat)PANGO_SCALE);
}

////////////////////////////////////////////////////////////////////////////////

static PangoFontMap *s_font_map = NULL;
static PangoContext *s_pango = NULL;
static PangoLayout *s_layout = NULL;

static bool lnx_pango_objects_intialize()
{
    bool t_success;
	t_success = true;
    
    if (t_success)
    {
        s_font_map = pango_ft2_font_map_new();
        t_success = s_font_map != NULL;
    }
    
	if (t_success)
    {
        s_pango = pango_ft2_font_map_create_context((PangoFT2FontMap *) s_font_map);
        t_success = s_pango != NULL;
    }
	
	if (t_success)
    {
        s_layout = pango_layout_new(s_pango);
        t_success = s_layout != NULL;
    }
    
	return t_success;
}

static bool lnx_pango_initialize(void)
{
	bool t_success;
	t_success = true;
	
	// Note: these libraries should be listed in dependency order
	if (t_success)
		t_success =
			initialise_weak_link_glib() != 0 &&
			initialise_weak_link_gobject() != 0 &&
			initialise_weak_link_pango() != 0 &&
			initialise_weak_link_pangoft2() != 0;
			
    
    if (t_success)
        t_success = lnx_pango_objects_intialize();
	
	return t_success;
}

static void lnx_pango_finalize(void)
{
    if (s_layout != NULL)
        g_object_unref(s_layout);
    
    if (s_pango != NULL)
        g_object_unref(s_pango);

    if (s_font_map != NULL)
        g_object_unref(s_font_map);
}

////////////////////////////////////////////////////////////////////////////////

void MCGPlatformInitialize(void)
{
	s_has_text_support = lnx_pango_initialize();
}

void MCGPlatformFinalize(void)
{
	lnx_pango_finalize();
}
////////////////////////////////////////////////////////////////////////////////

void MCGContextDrawPlatformText(MCGContextRef self, const unichar_t *p_text, uindex_t p_length, MCGPoint p_location, const MCGFont &p_font, bool p_rtl)
{
	// TODO: RTL
    
    // MW-2013-12-19: [[ Bug 11559 ]] Do nothing if no text support.
	if (!s_has_text_support)
		return;
	
	if (!MCGContextIsValid(self))
		return;	
	
	bool t_success;
	t_success = true;

	char *t_text;
	t_text = nil;
	if (t_success)
		t_success = MCCStringFromUnicodeSubstring(p_text, p_length / 2, t_text);
	
	MCGAffineTransform t_transform;
	MCGPoint t_device_location;	
	PangoLayoutLine *t_line;
	t_line = nil;
	if (t_success)
	{
		t_transform = MCGContextGetDeviceTransform(self);
		t_device_location = MCGPointApplyAffineTransform(p_location, t_transform);		
		t_transform . tx = modff(t_device_location . x, &t_device_location . x);
		t_transform . ty = modff(t_device_location . y, &t_device_location . y);		
		
		PangoMatrix t_ptransform;		
		t_ptransform . xx = t_transform . a;
		t_ptransform . xy = t_transform . b;
		t_ptransform . yx = t_transform . c;
		t_ptransform . yy = t_transform . d;
		t_ptransform . x0 = t_transform . tx;
		t_ptransform . y0 = t_transform . ty;
		pango_context_set_matrix(s_pango, &t_ptransform);
		
		pango_layout_set_font_description(s_layout, (PangoFontDescription *) p_font . fid);
		pango_layout_set_text(s_layout, t_text, -1);
		MCCStringFree(t_text);
		
		extern PangoLayoutLine *(*pango_layout_get_line_readonly_ptr)(PangoLayout *, int);
		if (pango_layout_get_line_readonly_ptr != nil)
			t_line = pango_layout_get_line_readonly_ptr(s_layout, 0);
		else
			t_line = pango_layout_get_line(s_layout, 0);
		t_success = t_line != nil;
	}
	
	MCGIntRectangle t_text_bounds, t_clipped_bounds;
	if (t_success)
	{
		PangoRectangle t_pbounds;
		pango_layout_line_get_extents(t_line, NULL, &t_pbounds);
		
		MCGRectangle t_float_text_bounds;
		t_float_text_bounds = MCGRectangleFromPangoRectangle(t_pbounds);
		
		MCGRectangle t_device_clip;
		t_device_clip = MCGContextGetDeviceClipBounds(self);
		t_device_clip . origin . x -= t_device_location . x;
		t_device_clip . origin . y -= t_device_location . y;
		
		MCGRectangle t_float_clipped_bounds;
		t_float_clipped_bounds = MCGRectangleIntersection(t_float_text_bounds, t_device_clip);
		
		t_text_bounds = MCGRectangleIntegerBounds(t_float_text_bounds);
		t_clipped_bounds = MCGRectangleIntegerBounds(t_float_clipped_bounds);
		
		if (t_clipped_bounds . width == 0 || t_clipped_bounds . height == 0)
			return;
	}
	
	void *t_data;
	t_data = nil;
	if (t_success)
		t_success = MCMemoryNew(t_clipped_bounds . width * t_clipped_bounds . height, t_data);
	
	if (t_success)
	{
		FT_Bitmap t_ftbitmap;
		t_ftbitmap . rows = t_clipped_bounds . height;
		t_ftbitmap . width = t_clipped_bounds . width;
		t_ftbitmap . pitch = t_clipped_bounds . width;
		t_ftbitmap . buffer = (unsigned char*) t_data;
		t_ftbitmap . num_grays = 256;
		t_ftbitmap . pixel_mode = FT_PIXEL_MODE_GRAY;
		t_ftbitmap . palette_mode = 0;
		t_ftbitmap . palette = nil;
		
		pango_ft2_render_layout_line(&t_ftbitmap, t_line, -(t_clipped_bounds . x - t_text_bounds . x), -(t_clipped_bounds . y - t_text_bounds . y) - t_text_bounds . y);
		
		SkPaint t_paint;
		t_paint . setStyle(SkPaint::kFill_Style);	
		t_paint . setAntiAlias(self -> state -> should_antialias);
		t_paint . setColor(MCGColorToSkColor(self -> state -> fill_color));
		
		SkBlendMode t_blend_mode = MCGBlendModeToSkBlendMode(self->state->blend_mode);
		t_paint.setBlendMode(t_blend_mode);

		SkImageInfo t_info = SkImageInfo::MakeA8(t_clipped_bounds.width, t_clipped_bounds.height);
		
		SkBitmap t_bitmap;
		t_bitmap.setInfo(t_info);
		t_bitmap.setPixels(t_data);

		self->layer->canvas->save();
		self->layer->canvas->resetMatrix();
		self->layer->canvas->drawBitmap(t_bitmap,
						t_clipped_bounds.x + t_device_location.x,
						t_clipped_bounds.y + t_device_location.y,
						&t_paint);
		self->layer->canvas->restore();
	}
    
	MCMemoryDelete(t_data);
	self -> is_valid = t_success;
}

MCGFloat __MCGContextMeasurePlatformText(MCGContextRef self, const unichar_t *p_text, uindex_t p_length, const MCGFont &p_font, const MCGAffineTransform &p_transform)
{	
	// MW-2013-12-19: [[ Bug 11559 ]] Do nothing if no text support.
	if (!s_has_text_support)
		return 0.0;
	
	bool t_success;
	t_success = true;
	
	char *t_text;
	t_text = nil;
	if (t_success)
		t_success = MCCStringFromUnicodeSubstring(p_text, p_length / 2, t_text);
	
	MCGFloat t_width;
	t_width = 0.0;
	if (t_success)
	{
		pango_layout_set_text(s_layout, t_text, -1);
		pango_layout_set_font_description(s_layout, (PangoFontDescription *) p_font . fid);
		
		PangoRectangle t_bounds;
		pango_layout_get_pixel_extents(s_layout, NULL, &t_bounds);
		t_width = t_bounds . width;
	}
	
	MCCStringFree(t_text);

	return t_width;
}

bool MCGContextMeasurePlatformTextImageBounds(MCGContextRef self, const unichar_t *p_text, uindex_t p_length, const MCGFont &p_font, const MCGAffineTransform &p_transform, MCGRectangle &r_bounds)
{
	// MW-2013-12-19: [[ Bug 11559 ]] Do nothing if no text support.
	if (!s_has_text_support)
		return false;
	
	bool t_success;
	t_success = true;
	
	if (t_success)
		t_success = lnx_pango_objects_intialize();
	
	char *t_text;
	t_text = nil;
	if (t_success)
		t_success = MCCStringFromUnicodeSubstring(p_text, p_length / 2, t_text);
	
	// Get extents of first line in order to get correct offset to baseline
	PangoLayoutLine *t_line;
	if (t_success)
	{
		pango_layout_set_text(s_layout, t_text, -1);
		pango_layout_set_font_description(s_layout, (PangoFontDescription *) p_font . fid);
		
		extern PangoLayoutLine *(*pango_layout_get_line_readonly_ptr)(PangoLayout *, int);
		if (pango_layout_get_line_readonly_ptr != nil)
			t_line = pango_layout_get_line_readonly_ptr(s_layout, 0);
		else
			t_line = pango_layout_get_line(s_layout, 0);
		t_success = t_line != nil;
	}
	
	if (t_success)
	{
		PangoRectangle t_layout_bounds, t_image_bounds;
		pango_layout_line_get_extents(t_line, &t_image_bounds, &t_layout_bounds);
		
		r_bounds = MCGRectangleFromPangoRectangle(t_image_bounds);
	}
	
	MCCStringFree(t_text);
	
	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

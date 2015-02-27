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

// MM-2014-07-31: [[ ThreadedRendering ]] Updated to make text rendering thread safe, by using pthread TLS to ensure we have seperate panog objects for each thread.
//  This should probably be moved to a central thread library at some point, which will also help with clean up (we only clean up the main thread at the moment).
static pthread_key_t /* PangoFontMap * */ s_font_map_key = 0;
static pthread_key_t /* PangoContext * */ s_pango_key = 0;
static pthread_key_t /* PangoLayout * */ s_layout_key = 0;

static bool lnx_pango_objects_intialize()
{
    bool t_success;
	t_success = true;
    
    PangoFontMap *t_font_map;
    PangoContext *t_pango;
    PangoLayout *t_layout;
    if (t_success)
    {
        t_font_map = (PangoFontMap *)pthread_getspecific(s_font_map_key);
        t_pango = (PangoContext *)pthread_getspecific(s_pango_key);
        t_layout = (PangoLayout *)pthread_getspecific(s_layout_key);
    }
    
    if (t_success)
		if (t_font_map == NULL)
		{
			t_font_map = pango_ft2_font_map_new();
			t_success = t_font_map != NULL;
            if (t_success)
                pthread_setspecific(s_font_map_key, t_font_map);
		}
	
	if (t_success)
		if (t_pango == NULL)
		{
			t_pango = pango_ft2_font_map_create_context((PangoFT2FontMap *) t_font_map);
			t_success = t_pango != NULL;
            if (t_success)
                pthread_setspecific(s_pango_key, t_pango);
		}
	
	if (t_success)
		if (t_layout == NULL)
		{
			t_layout = pango_layout_new(t_pango);
			t_success = t_layout != NULL;
            if (t_success)
                pthread_setspecific(s_layout_key, t_layout);
		}
    
	return t_success;
}

static bool lnx_pango_initialize(void)
{
	bool t_success;
	t_success = true;
	
	// MW-2013-12-19: [[ Bug 11559 ]] Use '=' not '=='!
	if (t_success)
		t_success =
			initialise_weak_link_pango() != 0 &&
			initialise_weak_link_pangoft2() != 0 &&
			initialise_weak_link_gobject() != 0 &&
			initialise_weak_link_glib() != 0;
    
    if (t_success)
    {
        pthread_key_create(&s_font_map_key, NULL);
        pthread_key_create(&s_pango_key, NULL);
        pthread_key_create(&s_layout_key, NULL);
        t_success = lnx_pango_objects_intialize();
    }
	
	return t_success;
}

static void lnx_pango_finalize(void)
{
    PangoLayout *t_layout;
    if (s_layout_key != 0)
    {
        t_layout = (PangoLayout *)pthread_getspecific(s_layout_key);
        pthread_setspecific(s_layout_key, NULL);
        if (t_layout != NULL)
            g_object_unref(t_layout);
    }
    
    PangoContext *t_pango;
    if (s_pango_key != 0)
    {
        t_pango = (PangoContext *)pthread_getspecific(s_pango_key);
        pthread_setspecific(s_pango_key, NULL);
        if (t_pango != NULL)
            g_object_unref(t_pango);
    }
    
    PangoFontMap *t_font_map;
    if (s_font_map_key != 0)
    {
        t_font_map = (PangoFontMap *)pthread_getspecific(s_font_map_key);
        pthread_setspecific(s_font_map_key, NULL);
        if (t_font_map != NULL)
            g_object_unref(t_font_map);
    }
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
    
    if (t_success)
        t_success = lnx_pango_objects_intialize();
    
    PangoContext *t_pango;
    PangoLayout *t_layout;
    if (t_success)
    {
        t_pango = (PangoContext *)pthread_getspecific(s_pango_key);
        t_layout = (PangoLayout *)pthread_getspecific(s_layout_key);
        t_success = t_pango != NULL && t_layout != NULL;
    }
	
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
		pango_context_set_matrix(t_pango, &t_ptransform);
		
		pango_layout_set_font_description(t_layout, (PangoFontDescription *) p_font . fid);
		pango_layout_set_text(t_layout, t_text, -1);
		MCCStringFree(t_text);
		
		extern PangoLayoutLine *(*pango_layout_get_line_readonly_ptr)(PangoLayout *, int);
		if (pango_layout_get_line_readonly_ptr != nil)
			t_line = pango_layout_get_line_readonly_ptr(t_layout, 0);
		else
			t_line = pango_layout_get_line(t_layout, 0);
		t_success = t_line != nil;
	}
	
	MCGIntRectangle t_text_bounds, t_clipped_bounds;
	if (t_success)
	{
		PangoRectangle t_pbounds;
		pango_layout_line_get_extents(t_line, NULL, &t_pbounds);
		
		MCGRectangle t_float_text_bounds;
        t_float_text_bounds . origin . x = t_pbounds . x / PANGO_SCALE;
        t_float_text_bounds . origin . y = t_pbounds . y / PANGO_SCALE;
        t_float_text_bounds . size . width = t_pbounds . width / PANGO_SCALE;
        t_float_text_bounds . size . height = t_pbounds . height / PANGO_SCALE;		
		
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
		
		SkXfermode *t_blend_mode;
		t_blend_mode = MCGBlendModeToSkXfermode(self -> state -> blend_mode);
		t_paint . setXfermode(t_blend_mode);
		if (t_blend_mode != NULL)
			t_blend_mode -> unref();		
		
		SkBitmap t_bitmap;
		t_bitmap . setConfig(SkBitmap::kA8_Config, t_clipped_bounds . width, t_clipped_bounds .  height);
        t_bitmap . setAlphaType(kPremul_SkAlphaType);
		t_bitmap . setPixels(t_data);
		self -> layer -> canvas -> drawSprite(t_bitmap, t_clipped_bounds . x + t_device_location . x, 
											  t_clipped_bounds . y + t_device_location . y, &t_paint);
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
    
    if (t_success)
        t_success = lnx_pango_objects_intialize();
    
    PangoLayout *t_layout;
    if (t_success)
    {
        t_layout = (PangoLayout *)pthread_getspecific(s_layout_key);
        t_success = t_layout != NULL;
    }
	
	char *t_text;
	t_text = nil;
	if (t_success)
		t_success = MCCStringFromUnicodeSubstring(p_text, p_length / 2, t_text);
	
	MCGFloat t_width;
	t_width = 0.0;
	if (t_success)
	{
		pango_layout_set_text(t_layout, t_text, -1);
		pango_layout_set_font_description(t_layout, (PangoFontDescription *) p_font . fid);
		
		PangoRectangle t_bounds;
		pango_layout_get_pixel_extents(t_layout, NULL, &t_bounds);
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
	
	PangoLayout *t_layout;
	if (t_success)
	{
		t_layout = (PangoLayout *)pthread_getspecific(s_layout_key);
		t_success = t_layout != NULL;
	}
	
	char *t_text;
	t_text = nil;
	if (t_success)
		t_success = MCCStringFromUnicodeSubstring(p_text, p_length / 2, t_text);
	
	if (t_success)
	{
		pango_layout_set_text(t_layout, t_text, -1);
		pango_layout_set_font_description(t_layout, (PangoFontDescription *) p_font . fid);
		
		PangoRectangle t_bounds;
		pango_layout_get_extents(t_layout, &t_bounds, NULL);
		
		r_bounds = MCGRectangleMake(t_bounds.x, t_bounds.y, t_bounds.width, t_bounds.height);
	}
	
	MCCStringFree(t_text);
	
	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

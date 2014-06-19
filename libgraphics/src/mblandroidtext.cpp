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

#include <ft2build.h>
#include FT_FREETYPE_H

#include <hb.h>
#include <hb-ft.h>
#include <hb-icu.h>

#include <SkTypeface.h>

////////////////////////////////////////////////////////////////////////////////

extern bool MCCStringFromUnicodeSubstring(const unichar_t * p_text, uindex_t p_length, char*& r_text);
extern void MCCStringFree(char *p_string);

////////////////////////////////////////////////////////////////////////////////

void MCGPlatformInitialize(void)
{
}

void MCGPlatformFinalize(void)
{
}

////////////////////////////////////////////////////////////////////////////////

void MCGContextDrawPlatformText(MCGContextRef self, const unichar_t *p_text, uindex_t p_length, MCGPoint p_location, const MCGFont &p_font, bool p_rtl)
{
	// TODO: RTL
    
    if (!MCGContextIsValid(self))
		return;	
    
    SkPaint t_paint;
    t_paint . setStyle(SkPaint::kFill_Style);
    t_paint . setAntiAlias(true);
    t_paint . setColor(MCGColorToSkColor(self -> state -> fill_color));
    t_paint . setTextSize(p_font . size);
    
    SkXfermode *t_blend_mode;
    t_blend_mode = MCGBlendModeToSkXfermode(self -> state -> blend_mode);
    t_paint . setXfermode(t_blend_mode);
    if (t_blend_mode != NULL)
        t_blend_mode -> unref();
    
    SkTypeface *t_typeface = (SkTypeface *) p_font . fid;
    t_paint . setTypeface(t_typeface);
    
    FT_Face t_ft_face = (FT_Face)t_typeface -> getFace();
    
    // draw the glyphs that are output from harfbuzz
    t_paint . setTextEncoding(SkPaint::kGlyphID_TextEncoding);
    hb_font_t *font = hb_ft_font_create(t_ft_face, NULL);
    
    hb_buffer_t *buffer = hb_buffer_create();
    
    hb_buffer_set_unicode_funcs(buffer, hb_icu_get_unicode_funcs());
    hb_buffer_set_direction(buffer, p_rtl ? HB_DIRECTION_RTL : HB_DIRECTION_LTR);
    //hb_buffer_set_script(buffer, HB_SCRIPT_ARABIC);
    
    hb_buffer_add_utf16(buffer, p_text, p_length/2, 0, p_length/2);
    hb_shape(font, buffer, NULL, 0);
    
    int glyph_count = hb_buffer_get_length(buffer);
    hb_glyph_info_t *glyph_info = hb_buffer_get_glyph_infos(buffer, 0);
    hb_glyph_position_t *glyph_pos = hb_buffer_get_glyph_positions(buffer,0);
    
    uint16_t* glyph_indices;
    MCMemoryAllocate(sizeof(uint16_t) * glyph_count, glyph_indices);
    MCGPoint *glyph_positions;
    MCMemoryAllocate(sizeof(MCGPoint) * glyph_count, glyph_positions);
    
    uindex_t x_offset, y_offset;
    x_offset = p_location . x + glyph_pos[0] . x_offset / 64;
    y_offset = p_location . y + glyph_pos[0] . y_offset / 64;
    for (int i=0; i < glyph_count; ++i)
    {
        glyph_indices[i] = glyph_info[i] . codepoint;
        MCLog("glyph codepoint: %d", glyph_info[i] . codepoint);
        
        glyph_positions[i] = MCGPointMake(x_offset, y_offset);
        x_offset += glyph_pos[i] . x_advance / 64;
        y_offset += glyph_pos[i] . y_advance / 64;
    }
    
    self -> layer -> canvas -> drawPosText(&glyph_indices[0], 2*glyph_count, MCGPointsToSkPoints(glyph_positions, glyph_count), t_paint);
    
    //self -> layer -> canvas -> drawText(glyph_indices, 2*glyph_count, MCGCoordToSkCoord(p_location . x), MCGCoordToSkCoord(p_location . y), t_paint);
    
    MCMemoryDeallocate(glyph_indices);
    MCMemoryDeallocate(glyph_positions);
    hb_buffer_destroy(buffer);
	
	self -> is_valid = true;
}

MCGFloat __MCGContextMeasurePlatformText(MCGContextRef self, const unichar_t *p_text, uindex_t p_length, const MCGFont &p_font, const MCGAffineTransform &p_transform)
{	
	//if (!MCGContextIsValid(self))
	//	return 0.0;
	
	bool t_success;
	t_success = true;
    
    MCAutoStringRef t_unicode_string;
    MCAutoStringRefAsUTF8String t_utf8_string;
    
    t_success = MCStringCreateWithChars(p_text, p_length / 2, &t_unicode_string);
    
    if (t_success)
        t_success = t_utf8_string . Lock(*t_unicode_string);
    
	MCGFloat t_width;
	t_width = 0.0;
	if (t_success)
	{
		SkPaint t_paint;
		t_paint . setTextSize(p_font . size);
		
		SkTypeface *t_typeface;
		t_typeface = (SkTypeface *) p_font . fid;
		t_paint . setTypeface(t_typeface);
		
        FT_Face t_ft_face = (FT_Face)t_typeface -> getFace();
        
        // draw the glyphs that are output from harfbuzz
        t_paint . setTextEncoding(SkPaint::kGlyphID_TextEncoding);
        hb_font_t *font = hb_ft_font_create(t_ft_face, NULL);

        hb_buffer_t *buffer = hb_buffer_create();
        hb_buffer_set_unicode_funcs(buffer, hb_icu_get_unicode_funcs());
        hb_buffer_set_direction(buffer, HB_DIRECTION_LTR);
        
        hb_buffer_add_utf16(buffer, p_text, p_length/2, 0, p_length/2);
        hb_shape(font, buffer, NULL, 0);
        
        int glyph_count = hb_buffer_get_length(buffer);
        hb_glyph_info_t *glyph_info = hb_buffer_get_glyph_infos(buffer, 0);
        
        hb_buffer_destroy(buffer);

        uint16_t* glyph_indices;
        MCMemoryAllocate(sizeof(uint16_t) * glyph_count, glyph_indices);
        
        for (int i=0; i < glyph_count; ++i)
            glyph_indices[i] = glyph_info[i] . codepoint;
        
		t_width =  (MCGFloat) t_paint . measureText(&glyph_indices[0], glyph_count*2);
        
        MCMemoryDeallocate(glyph_indices);
	}
	
	//self -> is_valid = t_success;
	return t_width;
}

/*
void MCGContextDrawPlatformText(MCGContextRef self, const unichar_t *p_text, uindex_t p_length, MCGPoint p_location, const MCGFont &p_font, bool p_rtl)
{
	// TODO: RTL
    
    if (!MCGContextIsValid(self))
		return;
	
	bool t_success;
	t_success = true;
    
    MCAutoStringRef t_unicode_string;
    MCAutoStringRefAsUTF8String t_utf8_string;
    
    t_success = MCStringCreateWithChars(p_text, p_length / 2, &t_unicode_string);
    
    if (t_success)
        t_success = t_utf8_string . Lock(*t_unicode_string);
	
	if (t_success)
	{
		SkPaint t_paint;
		t_paint . setStyle(SkPaint::kFill_Style);
		t_paint . setAntiAlias(true);
		t_paint . setColor(MCGColorToSkColor(self -> state -> fill_color));
		t_paint . setTextSize(p_font . size);
		
		SkXfermode *t_blend_mode;
		t_blend_mode = MCGBlendModeToSkXfermode(self -> state -> blend_mode);
		t_paint . setXfermode(t_blend_mode);
		if (t_blend_mode != NULL)
			t_blend_mode -> unref();
		
		SkTypeface *t_typeface;
		t_typeface = (SkTypeface *) p_font . fid;
		t_paint . setTypeface(t_typeface);
		
		// MM-2013-12-05: [[ Bug 11527 ]] Make sure we calculate the UTF-8 string length correctly.
		self -> layer -> canvas -> drawText(*t_utf8_string, t_utf8_string . Size(), MCGCoordToSkCoord(p_location . x), MCGCoordToSkCoord(p_location . y), t_paint);
	}
	
	self -> is_valid = t_success;
}

MCGFloat __MCGContextMeasurePlatformText(MCGContextRef self, const unichar_t *p_text, uindex_t p_length, const MCGFont &p_font, const MCGAffineTransform &p_transform)
{
	//if (!MCGContextIsValid(self))
	//	return 0.0;
	
	bool t_success;
	t_success = true;
    
    MCAutoStringRef t_unicode_string;
    MCAutoStringRefAsUTF8String t_utf8_string;
    
    t_success = MCStringCreateWithChars(p_text, p_length / 2, &t_unicode_string);
    
    if (t_success)
        t_success = t_utf8_string . Lock(*t_unicode_string);
    
	MCGFloat t_width;
	t_width = 0.0;
	if (t_success)
	{
		SkPaint t_paint;
		t_paint . setTextSize(p_font . size);
		
		SkTypeface *t_typeface;
		t_typeface = (SkTypeface *) p_font . fid;
		t_paint . setTypeface(t_typeface);
		
		// MM-2013-12-05: [[ Bug 11527 ]] Make sure we calculate the UTF-8 string length correctly.
		t_width =  (MCGFloat) t_paint . measureText(*t_utf8_string, t_utf8_string . Size());
	}
	
	//self -> is_valid = t_success;
	return t_width;
}
*/
////////////////////////////////////////////////////////////////////////////////

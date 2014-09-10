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
#include "hb-sk.h"
#include <hb-icu.h>

#include <SkTypeface.h>
#include <SkTypeface_android.h>
#include <SkPoint.h>
#include <SkTypes.h>

#include "foundation-unicode.h"
#include <unicode/uscript.h>

#define HB_SCALE_FACTOR 64

////////////////////////////////////////////////////////////////////////////////

extern bool MCCStringFromUnicodeSubstring(const unichar_t * p_text, uindex_t p_length, char*& r_text);
extern void MCCStringFree(char *p_string);

////////////////////////////////////////////////////////////////////////////////

static void skia_get_replacement_glyph(uint16_t p_size, SkTypeface *p_typeface, uint16_t& glyph, uindex_t& advance)
{
    SkPaint paint;
    paint . setTextEncoding(SkPaint::kUTF16_TextEncoding);
    paint . setTextSize(p_size);
    paint . setTypeface(p_typeface);
    
    codepoint_t t_replacement = 0xFFFD;
    
    SkScalar skWidth;
    SkRect skBounds;
    paint . textToGlyphs(&t_replacement, 2, &glyph);
    paint . getTextWidths(&t_replacement, 2, &skWidth, &skBounds);
    
    advance = (uint16_t)skWidth;
}

struct MCGlyphRun
{
    uindex_t count;
    SkPoint *positions;
    uint16_t *glyphs;
    SkTypeface *typeface;
};

void MCGlyphRunMake(hb_glyph_info_t *p_infos, hb_glyph_position_t *p_positions, MCGPoint* x_location, uindex_t p_start, uindex_t p_end, const MCGFont &p_font, MCGlyphRun& r_run)
{
    uindex_t t_count = p_end - p_start;
    MCMemoryAllocate(sizeof(uint16_t) * t_count, r_run . glyphs);
    MCMemoryAllocate(sizeof(MCGPoint) * t_count, r_run . positions);
    
    uindex_t run_index = 0;
    uindex_t x_offset, y_offset;
    uindex_t advance_y = 0;
    uindex_t advance_x = 0;
    
    for (uindex_t i = p_start; i < p_end; i++)
    {
        x_offset = x_location -> x + p_positions[i] . x_offset / HB_SCALE_FACTOR + advance_x;
        y_offset = x_location -> y + p_positions[i] . y_offset / HB_SCALE_FACTOR + advance_y;
        r_run . positions[run_index] = SkPoint::Make(x_offset, y_offset);
        
        uint16_t t_glyph = p_infos[i] . codepoint;
        if (t_glyph)
        {
            r_run . glyphs[run_index] = t_glyph;

            advance_x += p_positions[i] . x_advance / HB_SCALE_FACTOR;
            advance_y += p_positions[i] . y_advance / HB_SCALE_FACTOR;
        }
        else
        {
            uindex_t advance;
            skia_get_replacement_glyph(p_font . size, (SkTypeface *)p_font . fid, r_run . glyphs[run_index], advance);
            advance_x += advance;
        }
            
        run_index++;
    }
    
    x_location -> x += advance_x;
    x_location -> y += advance_y;
    
    r_run . typeface = (SkTypeface *)p_font . fid;
    r_run . typeface -> ref();
    
    r_run . count = t_count;
}

void MCGlyphRunDestroy(MCGlyphRun& p_run)
{
    MCMemoryDeallocate(p_run . glyphs);
    MCMemoryDeallocate(p_run . positions);
    p_run . typeface -> unref();
}

////////////////////////////////////////////////////////////////////////////////

void MCGPlatformInitialize(void)
{
}

void MCGPlatformFinalize(void)
{
}

////////////////////////////////////////////////////////////////////////////////

static hb_font_t *HBSkiaFaceToHBFont(hb_skia_face *p_typeface)
{
    return hb_sk_font_create(p_typeface, nil);
}

static hb_script_t HBScriptFromText(const unichar_t *p_text, uindex_t p_count)
{
    UScriptCode t_script;
    t_script = USCRIPT_COMMON;
    
    while (p_count && MCUnicodeIsWhitespace(*p_text))
    {
        p_text++;
        p_count--;
    }
    
    if (p_count)
    {
        UErrorCode t_error = U_ZERO_ERROR;
        t_script = uscript_getScript((uint32_t)(*p_text), &t_error);
        
        //MCLog("script: %s", uscript_getShortName(t_script));
    }
    
    return hb_icu_script_to_script(t_script);
}

void shape(const unichar_t* p_text, uindex_t p_char_count, MCGPoint p_location, bool p_rtl, const MCGFont &p_font, MCGlyphRun*& r_runs, uindex_t& r_run_count)
{
    MCAutoArray<MCGlyphRun> t_runs;
    SkTypeface *t_typeface = (SkTypeface *)p_font . fid;
    
    hb_skia_face t_face;
    t_face . typeface = t_typeface;
    t_face . size = p_font . size;
    
    //MCLog("typeface name %d", t_typeface -> uniqueID());
    
    // Set up the HarfBuzz buffer
    hb_buffer_t *buffer = hb_buffer_create();
    hb_buffer_set_unicode_funcs(buffer, hb_icu_get_unicode_funcs());
    hb_buffer_set_direction(buffer, p_rtl ? HB_DIRECTION_RTL : HB_DIRECTION_LTR);
    
    hb_buffer_set_script(buffer, HBScriptFromText(p_text, p_char_count));
    
    hb_buffer_add_utf16(buffer, p_text, p_char_count, 0, p_char_count);
    hb_shape(HBSkiaFaceToHBFont(&t_face), buffer, NULL, 0);
    
    int glyph_count = hb_buffer_get_length(buffer);
    hb_glyph_info_t *glyph_info = hb_buffer_get_glyph_infos(buffer, 0);
    hb_glyph_position_t *glyph_pos = hb_buffer_get_glyph_positions(buffer, 0);
    
    uindex_t t_cur_glyph = 0;
    uindex_t t_start, t_end;
    uindex_t t_char_index = 0;
    
    // deal with runs of supported and unsupported glyphs
    while (t_cur_glyph < glyph_count)
    {
        MCGlyphRun t_run;
        t_start = t_end = t_cur_glyph;
        
        // Run of successfully shaped glyphs
        while (t_cur_glyph < glyph_count && glyph_info[t_cur_glyph] . codepoint != 0)
        {
            t_char_index++;
            t_end++;
            t_cur_glyph++;
        }
        
        if (t_start != t_end)
        {
            MCGlyphRunMake(glyph_info, glyph_pos, &p_location, t_start, t_end, p_font, t_run);
            t_runs . Push(t_run);
            t_start = t_cur_glyph;
            t_end = t_cur_glyph;
        }
        
        // Deal with run of unsupported characters for this font.
        while (t_cur_glyph < glyph_count && glyph_info[t_cur_glyph] . codepoint == 0)
        {
            t_end++;
            t_cur_glyph++;
        }
        
        if (t_start != t_end)
        {
            // Need to get a fallback font here.
            SkTypeface *t_fallback = SkCreateFallbackTypefaceForChar(*(p_text + t_char_index), SkTypeface::kNormal);
            
            // TODO: This currently seems to return nil all the time.
            if (t_fallback != nil)
            {
                MCGFont t_font = MCGFontMake(t_fallback, p_font . size, p_font . fixed_advance, p_font . ascent, p_font . descent, p_font . ideal);
                
                MCGlyphRun *t_fallback_runs;
                uindex_t t_fallback_run_count;

                shape(p_text + t_char_index, t_end - t_start, p_location, p_rtl, t_font, t_fallback_runs, t_fallback_run_count);
            
                for (uindex_t i = 0; i < t_fallback_run_count; i++)
                    t_runs . Push(t_fallback_runs[i]);
                
                t_fallback -> unref();
            }
            else
            {
                // For now, just fail to display the glyphs. Should display 'missing character' glyph.
                MCGlyphRunMake(glyph_info, glyph_pos, &p_location, t_start, t_end, p_font, t_run);
                t_runs . Push(t_run);
            }
            
            t_start = t_cur_glyph;
            t_end = t_cur_glyph;
        }

        t_char_index += (t_end - t_start);
    }
    
    hb_buffer_destroy(buffer);
    t_runs . Take(r_runs, r_run_count);
}

////////////////////////////////////////////////////////////////////////////////

void MCGContextDrawPlatformText(MCGContextRef self, const unichar_t *p_text, uindex_t p_length, MCGPoint p_location, const MCGFont &p_font, bool p_rtl)
{
    if (!MCGContextIsValid(self))
		return;	
    
    SkPaint t_paint;
    t_paint . setStyle(SkPaint::kFill_Style);
    t_paint . setAntiAlias(true);
    t_paint . setColor(MCGColorToSkColor(self -> state -> fill_color));
    t_paint . setTextSize(p_font . size);
    t_paint . setTextEncoding(SkPaint::kGlyphID_TextEncoding);
    
    SkXfermode *t_blend_mode;
    t_blend_mode = MCGBlendModeToSkXfermode(self -> state -> blend_mode);
    t_paint . setXfermode(t_blend_mode);
    if (t_blend_mode != NULL)
        t_blend_mode -> unref();
    
    MCGlyphRun *t_glyph_runs;
    uindex_t t_count;

    shape(p_text, p_length/2, p_location, p_rtl, p_font, t_glyph_runs, t_count);
    
    for (uindex_t i = 0; i < t_count; i++)
    {
        t_paint . setTypeface(t_glyph_runs[i] . typeface);
        self -> layer -> canvas -> drawPosText(&(t_glyph_runs[i] . glyphs[0]), t_glyph_runs[i] . count * 2, &(t_glyph_runs[i] . positions[0]), t_paint);
        
        MCGlyphRunDestroy(t_glyph_runs[i]);
    }
    
    MCMemoryDeleteArray(t_glyph_runs);
    
	self -> is_valid = true;
}

MCGFloat __MCGContextMeasurePlatformText(MCGContextRef self, const unichar_t *p_text, uindex_t p_length, const MCGFont &p_font, const MCGAffineTransform &p_transform)
{	
	//if (!MCGContextIsValid(self))
	//	return 0.0;
    
	MCGFloat t_width;
	t_width = 0.0;
    
    SkPaint t_paint;
    t_paint . setTextSize(p_font . size);
    t_paint . setTextEncoding(SkPaint::kGlyphID_TextEncoding);
    
    MCGlyphRun *t_glyph_runs;
    uindex_t t_count;
    
    MCGPoint t_dummy = MCGPointMake(0,0);
    shape(p_text, p_length/2, t_dummy, false, p_font, t_glyph_runs, t_count);
    
    for (uindex_t i = 0; i < t_count; i++)
    {
        t_paint . setTypeface(t_glyph_runs[i] . typeface);
        t_width +=  (MCGFloat) t_paint . measureText(&(t_glyph_runs[i] . glyphs[0]), t_glyph_runs[i] . count * 2);
        
        MCGlyphRunDestroy(t_glyph_runs[i]);
    }
    
    MCMemoryDeleteArray(t_glyph_runs);

	//self -> is_valid = t_success;
	return t_width;
}

////////////////////////////////////////////////////////////////////////////////

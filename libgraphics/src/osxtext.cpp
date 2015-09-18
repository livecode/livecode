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

#import <ApplicationServices/ApplicationServices.h>

#define kMCGMeasureTextFudge 3

////////////////////////////////////////////////////////////////////////////////

static ATSUStyle s_style = NULL;
static ATSUTextLayout s_layout = NULL;
static CGColorSpaceRef s_colour_space = NULL;
static CGColorRef s_colour = NULL;

static bool osx_prepare_text(const void *p_text, uindex_t p_length, const MCGFont &p_font, bool p_skip_bidi)
{
    OSStatus t_err;
	t_err = noErr;
    
	if (t_err == noErr)
		if (s_layout == NULL)
			t_err = ATSUCreateTextLayout(&s_layout);
	
	if (t_err == noErr)
		if (s_style == NULL)
			t_err = ATSUCreateStyle(&s_style);
    
    if (t_err == noErr)
        if (s_colour_space == NULL)
            s_colour_space = CGColorSpaceCreateWithName(kCGColorSpaceGenericGray);
    
    if (t_err == noErr)
        if (s_colour == NULL)
        {
            // Components are grey and alpha
            const float t_colour_components[] = {1.0, 1.0};
            s_colour = CGColorCreate(s_colour_space, t_colour_components);
        }
	
	ATSUFontID t_font_id;
	Fixed t_font_size;
	Boolean t_font_is_italic;
	ATSUTextMeasurement t_imposed_width;
    if (t_err == noErr)
    {
        t_font_size = p_font . size << 16;
		
		// MM-2013-09-16: [[ Bug 11283 ]] It appears that ATSUI doesn't like italic being passed as a style parameter to ATSUFONDtoFontID.
		//   Instead, set italic as an attribute tag.
		uint8_t t_style;
		t_style = p_font . style & ~italic;
		t_font_is_italic = p_font . style & italic;
		
		// MW-2013-12-05: [[ Bug 11535 ]] Set the imposed width to the fixed advance width
		//   if non-zero. Otherwise use the glyph advance.
		if (p_font . fixed_advance != 0)
			t_imposed_width = p_font . fixed_advance << 16;
		else
			t_imposed_width = kATSUseGlyphAdvance;
			
		// if the specified font can't be found, just use the default
		// MM-2013-09-16: [[ Bug 11283 ]] Do the same for font styles - if the font/style paring cannot be found, try font with no style.
		t_err = ATSUFONDtoFontID((short)(intptr_t)p_font . fid, t_style, &t_font_id);
		if (t_err != noErr)
			t_err = ATSUFONDtoFontID((short)(intptr_t)p_font . fid, 0, &t_font_id);
		if (t_err != noErr)
			t_err = ATSUFONDtoFontID(0, t_style, &t_font_id);
		if (t_err != noErr)
			t_err = ATSUFONDtoFontID(0, 0, &t_font_id);
    }
	ATSUAttributeTag t_tags[] =
	{
		kATSUFontTag,
		kATSUSizeTag,
		kATSUQDItalicTag,
		kATSUImposeWidthTag,
	};
	ByteCount t_sizes[] =
	{
		sizeof(ATSUFontID),
		sizeof(Fixed),
		sizeof(Boolean),
		sizeof(ATSUTextMeasurement),
	};
	ATSUAttributeValuePtr t_attrs[] =
	{
		&t_font_id,
		&t_font_size,
		&t_font_is_italic,
		&t_imposed_width,
	};
    uindex_t t_char_count;
    t_char_count = p_length / 2;
    
	if (t_err == noErr)
		t_err = ATSUSetAttributes(s_style, sizeof(t_tags) / sizeof(ATSUAttributeTag), t_tags, t_sizes, t_attrs);
	if (t_err == noErr)
		t_err = ATSUSetTextPointerLocation(s_layout, (const UniChar *) p_text, kATSUFromTextBeginning, kATSUToTextEnd, t_char_count);
	if (t_err == noErr)
		t_err = ATSUSetRunStyle(s_layout, s_style, kATSUFromTextBeginning, kATSUToTextEnd);
	if (t_err == noErr)
		t_err = ATSUSetTransientFontMatching(s_layout, true);
	
	return t_err == noErr;
}

static bool osx_measure_text_substring_width(uindex_t p_length, MCGFloat &r_width, bool p_skip_bidi)
{
	if (s_layout == NULL || s_style == NULL)
		return false;
	
    OSStatus t_err;
	t_err = noErr;
    
	MCGFloat t_width;	
	if (t_err == noErr)
	{
		uindex_t t_offset = 0;
        uindex_t t_length = p_length / 2;
        if (p_skip_bidi)
            t_offset += 1, t_length -= 2;
        
        ATSUTextMeasurement t_before, t_after, t_ascent, t_descent;
		t_err = ATSUGetUnjustifiedBounds(s_layout, t_offset, t_length, &t_before, &t_after, &t_ascent, &t_descent);
		
		t_width = t_after / 65536.0f;		
	}
	
    if (t_err == noErr)         
        r_width = t_width;
    
    return t_err == noErr;
}

// MM-2013-10-23: [[ RefactorGraphics ]] Move over to using (a fudged) ATSUMeasureTextImage when calculating the bounds.
//   Using the ATSUGetUnjustifiedBounds for the width and the fonts ascent/descent for the height was causing clipping issues for certain fonts.
static bool osx_measure_text_substring_bounds(uindex_t p_length, MCGIntRectangle &r_bounds, bool p_skip_bidi)
{
	if (s_layout == NULL || s_style == NULL)
		return false;
	
    OSStatus t_err;
	t_err = noErr;
    
    uindex_t t_offset = 0;
    uindex_t t_length = p_length / 2;
    if (p_skip_bidi)
        t_offset += 1, t_length -= 2;
    
	Rect t_bounds;
	if (t_err == noErr)
		t_err = ATSUMeasureTextImage(s_layout, t_offset, t_length, 0, 0, &t_bounds);
	
    if (t_err == noErr)
	{
		r_bounds . x = t_bounds . left - kMCGMeasureTextFudge;
		r_bounds . y = t_bounds . top - kMCGMeasureTextFudge;
		r_bounds . width = t_bounds . right - t_bounds . left + 2 * kMCGMeasureTextFudge;
		r_bounds . height = t_bounds . bottom - t_bounds . top + 2 * kMCGMeasureTextFudge;
	}
    
    return t_err == noErr;
}

static bool osx_draw_text_substring_to_cgcontext_at_location(uindex_t p_length, CGContextRef p_cgcontext, MCGPoint p_location, bool p_rtl, bool p_skip_bidi)
{
	if (s_layout == NULL || s_style == NULL)
		return false;
	
    OSStatus t_err;
	t_err = noErr;
    
    Boolean t_is_rtl = p_rtl;
	
	ATSUAttributeTag t_layout_tags[] =
	{
        kATSULineDirectionTag,
        kATSUCGContextTag,
	};
	ByteCount t_layout_sizes[] =
	{
		sizeof(Boolean),
        sizeof(CGContextRef),
	};
	ATSUAttributeValuePtr t_layout_attrs[] =
	{
		&t_is_rtl,
        &p_cgcontext,
	};

	if (t_err == noErr)
		t_err = ATSUSetLayoutControls(s_layout, sizeof(t_layout_tags) / sizeof(ATSUAttributeTag), t_layout_tags, t_layout_sizes, t_layout_attrs);
	
    uindex_t t_offset = 0;
    uindex_t t_length = p_length / 2;
    if (p_skip_bidi)
        t_offset += 1, t_length -= 2;
    
    if (t_err == noErr)
        t_err = ATSUDrawText(s_layout, t_offset, t_length, ((int32_t)p_location . x) << 16, ((int32_t)p_location . y) << 16);
    
    return t_err == noErr;
}

////////////////////////////////////////////////////////////////////////////////

#ifdef NOT_USED
static bool osx_draw_text_to_cgcontext_at_location(const void *p_text, uindex_t p_length, MCGPoint p_location, const MCGFont &p_font, CGContextRef p_cgcontext, MCGIntRectangle &r_bounds)
{	
	OSStatus t_err;
	t_err = noErr;	
	
	ATSUFontID t_font_id;
	Fixed t_font_size;
	t_font_size = p_font . size << 16;	
	
	ATSUAttributeTag t_tags[] =
	{
		kATSUFontTag,
		kATSUSizeTag,
	};
	ByteCount t_sizes[] =
	{
		sizeof(ATSUFontID),
		sizeof(Fixed),
	};
	ATSUAttributeValuePtr t_attrs[] =
	{
		&t_font_id,
		&t_font_size,
	};	
	
	ATSLineLayoutOptions t_layout_options;
	ATSUAttributeTag t_layout_tags[] =
	{
		kATSULineLayoutOptionsTag,
		kATSUCGContextTag,
	};
	ByteCount t_layout_sizes[] =
	{
		sizeof(ATSLineLayoutOptions),
		sizeof(CGContextRef),
	};
	ATSUAttributeValuePtr t_layout_attrs[] =
	{
		&t_layout_options,
		&p_cgcontext,
	};	
	
	if (t_err == noErr)
	{
		// if the specified fon't can't be found, just use the default
		if (ATSUFONDtoFontID((short)(intptr_t)p_font . fid, p_font . style, &t_font_id) != noErr)
			t_err = ATSUFONDtoFontID(0, p_font . style, &t_font_id);
	}
	
	ATSUStyle t_style;
	t_style = NULL;
	if (t_err == noErr)
		t_err = ATSUCreateStyle(&t_style);
	if (t_err == noErr)
		t_err = ATSUSetAttributes(t_style, sizeof(t_tags) / sizeof(ATSUAttributeTag), t_tags, t_sizes, t_attrs);
	
	ATSUTextLayout t_layout;
	t_layout = NULL;
	if (t_err == noErr)
	{
		UniCharCount t_run;
		t_run = p_length / 2;
		t_err = ATSUCreateTextLayoutWithTextPtr((const UniChar *)p_text, 0, p_length / 2, p_length / 2, 1, &t_run, &t_style, &t_layout);
	}
	if (t_err == noErr)
		t_err = ATSUSetTransientFontMatching(t_layout, true);
	if (t_err == noErr)
	{
		t_layout_options = kATSLineUseDeviceMetrics | kATSLineFractDisable;
		t_err = ATSUSetLayoutControls(t_layout, sizeof(t_layout_tags) / sizeof(ATSUAttributeTag), t_layout_tags, t_layout_sizes, t_layout_attrs);
	}	
	
	MCGIntRectangle t_bounds;
	if (p_cgcontext == NULL)
	{
		ATSUTextMeasurement t_before, t_after, t_ascent, t_descent;
		if (t_err == noErr)
			t_err = ATSUGetUnjustifiedBounds(t_layout, 0, p_length / 2, &t_before, &t_after, &t_ascent, &t_descent);
		
		if (t_err == noErr)
		{
			t_ascent = (t_ascent + 0xffff) >> 16;
			t_descent = (t_descent + 0xffff) >> 16;
			t_after = (t_after + 0xffff) >> 16;
			
			t_bounds . x = p_location . x;
			t_bounds . y = p_location . y - p_font . ascent;
			t_bounds . width = t_after;
			t_bounds . height = p_font . descent + p_font . ascent;
			
			r_bounds = t_bounds;
		}
	}
	
	if (t_err == noErr)
		if (p_cgcontext != NULL)
			t_err = ATSUDrawText(t_layout, 0, p_length / 2, ((int32_t)p_location . x) << 16, ((int32_t)p_location . y) << 16);
	
	if (t_layout != NULL)
		ATSUDisposeTextLayout(t_layout);
	if (t_style != NULL)
		ATSUDisposeStyle(t_style);
	
	return t_err == noErr;	
}
#endif

////////////////////////////////////////////////////////////////////////////////

void MCGPlatformInitialize(void)
{
}

void MCGPlatformFinalize(void)
{
    if (s_colour != NULL)
        CGColorRelease(s_colour);
    if (s_colour_space != NULL)
        CGColorSpaceRelease(s_colour_space);
}

////////////////////////////////////////////////////////////////////////////////

void MCGContextDrawPlatformText(MCGContextRef self, const unichar_t *p_text, uindex_t p_length, MCGPoint p_location, const MCGFont &p_font, bool p_rtl)
{
    if (!MCGContextIsValid(self))
		return;	
	
	bool t_success;
	t_success = true;
    
    // If drawing text with BiDi control characters, don't draw the characters
    bool t_skip_bidi = false;
    if (p_text[0] == 0x202D || p_text[0] == 0x202E)
        t_skip_bidi = true;
    
    if (t_success)
        t_success = osx_prepare_text(p_text, p_length, p_font, t_skip_bidi);
    
	MCGIntRectangle t_text_bounds;
	if (t_success)
		t_success = osx_measure_text_substring_bounds(p_length, t_text_bounds, t_skip_bidi);
	
	MCGIntRectangle t_clipped_bounds;
	MCGAffineTransform t_transform;
	MCGPoint t_device_location;
	if (t_success)
	{
		t_transform = MCGContextGetDeviceTransform(self);
		t_device_location = MCGPointApplyAffineTransform(p_location, t_transform);		
		t_transform . tx = modff(t_device_location . x, &t_device_location . x);
		t_transform . ty = modff(t_device_location . y, &t_device_location . y);
		
		MCGRectangle t_device_clip;
		t_device_clip = MCGContextGetDeviceClipBounds(self);
		t_device_clip . origin . x -= t_device_location . x;
		t_device_clip . origin . y -= t_device_location . y;
		
		MCGRectangle t_float_text_bounds, t_float_clipped_bounds;
		t_float_text_bounds = MCGRectangleApplyAffineTransform(MCGRectangleMake(t_text_bounds . x, t_text_bounds . y, t_text_bounds . width, t_text_bounds . height), t_transform);		
		t_float_clipped_bounds = MCGRectangleIntersection(t_float_text_bounds, t_device_clip);
		
		t_text_bounds = MCGRectangleIntegerBounds(t_float_text_bounds);
		t_clipped_bounds = MCGRectangleIntegerBounds(t_float_clipped_bounds);
		
		if (t_clipped_bounds . width == 0 || t_clipped_bounds . height == 0)
			return;
	}
	
	void *t_data;
	t_data = NULL;
	if (t_success)
		t_success = MCMemoryNew(t_clipped_bounds . width * t_clipped_bounds . height * 1, t_data);

	CGContextRef t_cgcontext;
	t_cgcontext = NULL;
	if (t_success)
	{
        t_cgcontext = CGBitmapContextCreate(t_data, t_clipped_bounds . width, t_clipped_bounds . height, 8, t_clipped_bounds . width * 1, s_colour_space, kCGImageAlphaNone);
		t_success = t_cgcontext != NULL;
	}
    
	if (t_success)
	{
        CGContextTranslateCTM(t_cgcontext, -t_clipped_bounds . x, t_clipped_bounds . height + t_clipped_bounds . y);
		CGContextConcatCTM(t_cgcontext, CGAffineTransformMake(t_transform . a, t_transform . b, t_transform . c, t_transform . d, t_transform . tx, t_transform . ty));
		CGContextSetFillColorWithColor(t_cgcontext, s_colour);
		t_success = osx_draw_text_substring_to_cgcontext_at_location(p_length, t_cgcontext, MCGPointMake(0.0, 0.0), p_rtl, t_skip_bidi);
	}
	
	if (t_success)
	{
		CGContextFlush(t_cgcontext);
		
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
	CGContextRelease(t_cgcontext);	
	self -> is_valid = t_success;
}

MCGFloat __MCGContextMeasurePlatformText(MCGContextRef self, const unichar_t *p_text, uindex_t p_length, const MCGFont &p_font, const MCGAffineTransform &p_transform)
{	
	bool t_success;
	t_success = true;
	
    bool t_skip_bidi = false;
    if (p_text[0] == 0x202D || p_text[0] == 0x202E)
        t_skip_bidi = true;
    
    if (t_success)
        t_success = osx_prepare_text(p_text, p_length, p_font, t_skip_bidi);
	
    MCGFloat t_width;
    t_width = 0;
    if (t_success)
        t_success = osx_measure_text_substring_width(p_length, t_width, t_skip_bidi);
	
    return (MCGFloat) t_width;
}

////////////////////////////////////////////////////////////////////////////////

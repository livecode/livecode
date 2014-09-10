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

#import <CoreFoundation/CoreFoundation.h>

#ifdef TARGET_SUBPLATFORM_IPHONE
#import <CoreText/CoreText.h>
#import <CoreGraphics/CoreGraphics.h>
#else
#import <ApplicationServices/ApplicationServices.h>
#endif

////////////////////////////////////////////////////////////////////////////////

static CGColorSpaceRef s_colour_space = NULL;
static CGColorRef s_text_colour = NULL;
static CGContextRef s_measure_context;
static void *s_measure_data = NULL;

void MCGPlatformInitialize(void)
{
    s_colour_space = NULL;
    s_text_colour = NULL;
    s_measure_context = NULL;
    s_measure_data = NULL;
    const float t_colour_components[] = {1.0, 1.0};
#ifdef TARGET_SUBPLATFORM_IPHONE
    // iOS doesn't support device-independent or generic color spaces
    s_colour_space = CGColorSpaceCreateDeviceGray();
#else
    s_colour_space = CGColorSpaceCreateWithName(kCGColorSpaceGenericGray);
#endif
    s_text_colour = CGColorCreate(s_colour_space, t_colour_components);
    
    /* UNCHECKED */ MCMemoryAllocate(1, s_measure_data);
    s_measure_context =  CGBitmapContextCreate(s_measure_data, 1, 1, 8, 1, NULL, kCGImageAlphaOnly);
}

void MCGPlatformFinalize(void)
{
    if (s_text_colour != NULL)
        CGColorRelease(s_text_colour);
    if (s_colour_space != NULL)
        CGColorSpaceRelease(s_colour_space);
    if (s_measure_context != NULL)
        CGContextRelease(s_measure_context);
    MCMemoryDelete(s_measure_data);
    s_colour_space = NULL;
    s_text_colour = NULL;
    s_measure_context = NULL;
    s_measure_data = NULL;
}

////////////////////////////////////////////////////////////////////////////////

static CTLineRef unitext_to_cfline(const unichar_t *p_text, uindex_t p_length, const MCGFont &p_font)
{
	bool t_success;
	t_success = true;
	
	CFStringRef t_cftext;
	t_cftext = NULL;
	if (t_success)
	{
		t_cftext = CFStringCreateWithCharacters(kCFAllocatorDefault, p_text, p_length >> 1);
		t_success = t_cftext != NULL;
	}
	
    CFDictionaryRef t_attributes;
    t_attributes = NULL;
	if (t_success)
	{
		CTFontRef t_font;
		t_font = (CTFontRef) p_font . fid;
		CFStringRef t_keys[] = {
            kCTFontAttributeName,
            kCTForegroundColorAttributeName,
        };
		CFTypeRef t_values[] = {
            t_font,
            s_text_colour,
        };
        t_attributes = CFDictionaryCreate(NULL,
                                          (const void **)&t_keys, (const void **)&t_values,
                                          sizeof(t_keys) / sizeof(t_keys[0]),
                                          &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
        t_success = t_attributes != NULL;
	}
    
    CFAttributedStringRef t_attributed_text;
	t_attributed_text = NULL;
    if (t_success)
    {
        t_attributed_text = CFAttributedStringCreate(NULL, t_cftext, t_attributes);
		t_success = t_attributed_text != NULL;
    }
	
	CTLineRef t_line;
	t_line = NULL;
	if (t_success)
		t_line = CTLineCreateWithAttributedString(t_attributed_text);
		
    if (t_attributed_text != NULL)
        CFRelease(t_attributed_text);
    if (t_attributes != NULL)
        CFRelease(t_attributes);
    if (t_cftext != NULL)
        CFRelease(t_cftext);
	
	return t_line;
}

////////////////////////////////////////////////////////////////////////////////

void MCGContextDrawPlatformText(MCGContextRef self, const unichar_t *p_text, uindex_t p_length, MCGPoint p_location, const MCGFont &p_font, bool p_rtl)
{
	if (!MCGContextIsValid(self))
		return;	
	
	bool t_success;
	t_success = true;
	
	CTLineRef t_line;
	t_line = NULL;
	if (t_success)
	{
		t_line = unitext_to_cfline(p_text, p_length, p_font);
		t_success = t_line != NULL;
	}
	
	MCGIntRectangle t_clipped_bounds, t_text_bounds;
	MCGAffineTransform t_transform;
	MCGPoint t_device_location;
	if (t_success)
	{
		MCGFloat t_ascent, t_descent;
        t_ascent = CTFontGetAscent((CTFontRef)p_font . fid);
		t_descent = CTFontGetDescent((CTFontRef)p_font . fid);
        
        CGRect t_font_bounds;
        t_font_bounds = CTFontGetBoundingBox((CTFontRef)p_font . fid);
        
        // MW-2014-06-25: [[ Bug 12690 ]] Looks like the font bounds rect is in bottom-left coord
        //   system, so calculate bottom and top bounds.
        CGFloat t_font_bounds_bottom, t_font_bounds_top;
        t_font_bounds_bottom = t_font_bounds . origin . y;
        t_font_bounds_top = t_font_bounds . origin . y + t_font_bounds . size . height;
        
        MCGFloat t_width;
        t_width = MCGContextMeasurePlatformText(self, p_text, p_length, p_font, t_transform); // CTLineGetTypographicBounds(t_line, NULL, NULL, NULL);
        
        // Italic text appears to be getting clipped on the right hand side, so we increase the width.
        // Using purley the image bounds still results in clipping so for the moment have added the width of the font to make sure we don't clip.
        CGFloat t_slant;
        t_slant = CTFontGetSlantAngle((CTFontRef)p_font . fid);
        if (t_slant != 0.0f)
        {
            CGRect t_image_bounds;
            t_image_bounds = CTLineGetImageBounds(t_line, s_measure_context);
            t_width = MCMax(t_width, t_image_bounds . size . width) + t_font_bounds . size . width;
        }
        
        // MW-2014-06-25: [[ Bug 12690 ]] I suspect ascent/descent will always be less than font
        //   bounds calcs, but let's just err on the side of caution.
		MCGRectangle t_float_text_bounds;
		t_float_text_bounds . origin . x = 0;
        t_float_text_bounds . origin . y = MCMin(-t_font_bounds_top, -t_ascent);
		t_float_text_bounds . size . width = t_width;
        t_float_text_bounds . size . height = MCMax(t_font_bounds_top - t_font_bounds_bottom, t_ascent + t_descent);
		
		t_transform = MCGContextGetDeviceTransform(self);
		t_device_location = MCGPointApplyAffineTransform(p_location, t_transform);		
		t_transform . tx = modff(t_device_location . x, &t_device_location . x);
		t_transform . ty = modff(t_device_location . y, &t_device_location . y);
		
		MCGRectangle t_device_clip;
		t_device_clip = MCGContextGetDeviceClipBounds(self);
		t_device_clip . origin . x -= t_device_location . x;
		t_device_clip . origin . y -= t_device_location . y;
		
		MCGRectangle t_float_clipped_bounds;
		t_float_text_bounds = MCGRectangleApplyAffineTransform(t_float_text_bounds, t_transform);		
		t_float_clipped_bounds = MCGRectangleIntersection(t_float_text_bounds, t_device_clip);
		
		t_text_bounds = MCGRectangleIntegerBounds(t_float_text_bounds);
		t_clipped_bounds = MCGRectangleIntegerBounds(t_float_clipped_bounds);
		
		if (t_clipped_bounds . width == 0 || t_clipped_bounds . height == 0)
        {
            if (t_line != NULL)
                CFRelease(t_line);
			return;
        }
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

		CTLineDraw(t_line, t_cgcontext);
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
	
    if (t_line != NULL)
        CFRelease(t_line);
	MCMemoryDelete(t_data);
	CGContextRelease(t_cgcontext);	
	self -> is_valid = t_success;
}

////////////////////////////////////////////////////////////////////////////////

MCGFloat __MCGContextMeasurePlatformText(MCGContextRef self, const unichar_t *p_text, uindex_t p_length, const MCGFont &p_font, const MCGAffineTransform &p_transform)
{
	bool t_success;
	t_success = true;
	
	CTLineRef t_line;
	t_line = NULL;
	if (t_success)
	{
		t_line = unitext_to_cfline(p_text, p_length, p_font);
		t_success = t_line != NULL;
	}
	
	MCGFloat t_width;
	t_width = 0.0f;
	if (t_success)
		t_width = CTLineGetTypographicBounds(t_line, NULL, NULL, NULL);
	
    if (t_line != NULL)
        CFRelease(t_line);
	return t_width;	
}

////////////////////////////////////////////////////////////////////////////////
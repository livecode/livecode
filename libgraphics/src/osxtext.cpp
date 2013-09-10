#include "graphics.h"
#include "graphics-internal.h"

#import <ApplicationServices/ApplicationServices.h>

////////////////////////////////////////////////////////////////////////////////

static ATSUStyle s_style = NULL;
static ATSUTextLayout s_layout = NULL;

static bool osx_prepare_text(const void *p_text, uindex_t p_length, const MCGFont &p_font)
{
    OSStatus t_err;
	t_err = noErr;
    
	if (t_err == noErr)
		if (s_layout == NULL)
			t_err = ATSUCreateTextLayout(&s_layout);

	if (t_err == noErr)
		if (s_style == NULL)
			t_err = ATSUCreateStyle(&s_style);
	
	ATSUFontID t_font_id;
	Fixed t_font_size;
    if (t_err == noErr)
    {
        t_font_size = p_font . size << 16;
		// if the specified font can't be found, just use the default
		if (ATSUFONDtoFontID((short)(intptr_t)p_font . fid, p_font . style, &t_font_id) != noErr)
			t_err = ATSUFONDtoFontID(0, p_font . style, &t_font_id);
    }
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
	if (t_err == noErr)
		t_err = ATSUSetAttributes(s_style, sizeof(t_tags) / sizeof(ATSUAttributeTag), t_tags, t_sizes, t_attrs);
    
	ATSLineLayoutOptions t_layout_options;
    t_layout_options = kATSLineUseDeviceMetrics | kATSLineFractDisable;
	ATSUAttributeTag t_layout_tags[] =
	{
		kATSULineLayoutOptionsTag,
	};
	ByteCount t_layout_sizes[] =
	{
		sizeof(ATSLineLayoutOptions),
	};
	ATSUAttributeValuePtr t_layout_attrs[] =
	{
		&t_layout_options,
	};
	if (t_err == noErr)
		t_err = ATSUSetTextPointerLocation(s_layout, (const UniChar *) p_text, 0, p_length / 2, p_length / 2);
	if (t_err == noErr)
		t_err = ATSUSetRunStyle(s_layout, s_style, 0, p_length / 2);
	if (t_err == noErr)
		t_err = ATSUSetTransientFontMatching(s_layout, true);
	if (t_err == noErr)
		t_err = ATSUSetLayoutControls(s_layout, sizeof(t_layout_tags) / sizeof(ATSUAttributeTag), t_layout_tags, t_layout_sizes, t_layout_attrs);
	
	return t_err == noErr;
}

static bool osx_measure_text_substring_width(uindex_t p_length, int32_t &r_width)
{
	if (s_layout == NULL || s_style == NULL)
		return false;
	
    OSStatus t_err;
	t_err = noErr;
    
    ATSUTextMeasurement t_before, t_after, t_ascent, t_descent;
    if (t_err == noErr)
        t_err = ATSUGetUnjustifiedBounds(s_layout, 0, p_length / 2, &t_before, &t_after, &t_ascent, &t_descent);
    
    if (t_err == noErr)         
        r_width = (t_after + 0xffff) >> 16;
    
    return t_err == noErr;
}

static bool osx_draw_text_substring_to_cgcontext_at_location(uindex_t p_length, CGContextRef p_cgcontext, MCGPoint p_location)
{
	if (s_layout == NULL || s_style == NULL)
		return false;

    OSStatus t_err;
	t_err = noErr;

	ATSUAttributeTag t_layout_tags[] =
	{
		kATSUCGContextTag,
	};
	ByteCount t_layout_sizes[] =
	{
		sizeof(CGContextRef),
	};
	ATSUAttributeValuePtr t_layout_attrs[] =
	{
		&p_cgcontext,
	};
	if (t_err == noErr)
		t_err = ATSUSetLayoutControls(s_layout, sizeof(t_layout_tags) / sizeof(ATSUAttributeTag), t_layout_tags, t_layout_sizes, t_layout_attrs);

    if (t_err == noErr)
        t_err = ATSUDrawText(s_layout, 0, p_length / 2, ((int32_t)p_location . x) << 16, ((int32_t)p_location . y) << 16);
    
    return t_err == noErr;
}

////////////////////////////////////////////////////////////////////////////////

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

////////////////////////////////////////////////////////////////////////////////

void MCGPlatformInitialize(void)
{
}

void MCGPlatformFinalize(void)
{
}

////////////////////////////////////////////////////////////////////////////////

void MCGContextDrawPlatformText(MCGContextRef self, const unichar_t *p_text, uindex_t p_length, MCGPoint p_location, const MCGFont &p_font)
{
	if (!MCGContextIsValid(self))
		return;	

	bool t_success;
	t_success = true;
	
    if (t_success)
        t_success = osx_prepare_text(p_text, p_length, p_font);
    
	MCGIntRectangle t_text_bounds;
	if (t_success)
    {
        t_text_bounds . x = 0;
        t_text_bounds . y = 0 - p_font . ascent;
        t_text_bounds . height = p_font . descent + p_font . ascent + 1; // the +1 fudge is to make sure chars with descent are not clipped
        //t_success = osx_measure_text_substring_width(p_length, t_text_bounds . width);
		//t_success = osx_draw_text_to_cgcontext_at_location(p_text, p_length, MCGPointMake(0.0, 0.0), p_font, NULL, t_text_bounds);
        
		// if the text is short enough to have it's width potentailly cached, use MCGContextMeasurePlatformText
        // this will potentially result in osx_prepare_text being called again
        // though ideally not, as hopefully the text will have been recently measured and in the cache
		if (p_length >= kMCGTextMeasureCacheMaxStringLength)
			t_success = osx_measure_text_substring_width(p_length, t_text_bounds . width);
		else
			t_text_bounds . width = MCGContextMeasurePlatformText(self, p_text, p_length, p_font);
    }
	
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
		t_success = MCMemoryNew(t_clipped_bounds . width * t_clipped_bounds . height, t_data);
	
	CGContextRef t_cgcontext;
	t_cgcontext = NULL;
	if (t_success)
	{
		t_cgcontext = CGBitmapContextCreate(t_data, t_clipped_bounds . width, t_clipped_bounds . height, 8, t_clipped_bounds . width, NULL, kCGImageAlphaOnly);
		t_success = t_cgcontext != NULL;
	}
	
	if (t_success)
	{
		CGContextTranslateCTM(t_cgcontext, -(t_clipped_bounds . x - t_text_bounds . x), t_clipped_bounds . height + t_clipped_bounds . y);
		CGContextConcatCTM(t_cgcontext, CGAffineTransformMake(t_transform . a, t_transform . b, t_transform . c, t_transform . d, t_transform . tx, t_transform . ty));
		CGContextSetRGBFillColor(t_cgcontext, 0.0, 0.0, 0.0, 1.0);
		//t_success = osx_draw_text_to_cgcontext_at_location(p_text, p_length, MCGPointMake(0.0, 0.0), p_font, t_cgcontext, t_clipped_bounds);
        t_success = osx_draw_text_substring_to_cgcontext_at_location(p_length, t_cgcontext, MCGPointMake(0.0, 0.0));
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
		t_bitmap . setIsOpaque(false);
		t_bitmap . setPixels(t_data);
		self -> layer -> canvas -> drawSprite(t_bitmap, t_clipped_bounds . x + t_device_location . x, 
											  t_clipped_bounds . y + t_device_location . y, &t_paint);		
	}
	
	MCMemoryDelete(t_data);	
	CGContextRelease(t_cgcontext);	
	self -> is_valid = t_success;
}

MCGFloat __MCGContextMeasurePlatformText(MCGContextRef self, const unichar_t *p_text, uindex_t p_length, const MCGFont &p_font)
{	
	//if (!MCGContextIsValid(self))
	//	return 0.0;

	bool t_success;
	t_success = true;
	
    if (t_success)
        t_success = osx_prepare_text(p_text, p_length, p_font);

    int32_t t_width;
    t_width = 0;
    if (t_success)
        t_success = osx_measure_text_substring_width(p_length, t_width);
    
	//self -> is_valid = t_success;
	
    return (MCGFloat) t_width;
}

////////////////////////////////////////////////////////////////////////////////

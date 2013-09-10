#include "graphics.h"
#include "graphics-internal.h"

#import <Foundation/Foundation.h>
#import <CoreGraphics/CoreGraphics.h>
#import <UIKit/UIKit.h>

////////////////////////////////////////////////////////////////////////////////

typedef void for_each_word_callback_t(void *context, const void *text, uindex_t text_length, bool is_unicode);

static void for_each_word(const void *p_text, uint32_t p_text_length, bool p_is_unicode, for_each_word_callback_t p_callback, void *p_context)
{
	void *t_text_ptr;
	if (p_is_unicode && ((uintptr_t)p_text & 1) != 0)
	{
		t_text_ptr = malloc(p_text_length);
		memcpy(t_text_ptr, p_text, p_text_length);
	}
	else
		t_text_ptr = (void *)p_text;
	
	if (!p_is_unicode)
	{
		char *t_native_text_ptr;
		t_native_text_ptr = (char *)t_text_ptr;
		
		uindex_t t_word_start;
		t_word_start = 0;
		while(t_word_start < p_text_length)
		{
			uindex_t t_word_end;
			t_word_end = t_word_start;
			while(t_native_text_ptr[t_word_end] != ' ' && t_word_end < p_text_length)
				t_word_end++;
			while(t_native_text_ptr[t_word_end] == ' ' && t_word_end < p_text_length)
				t_word_end++;
			
			p_callback(p_context, t_native_text_ptr + t_word_start, t_word_end - t_word_start, p_is_unicode);
			
			t_word_start = t_word_end;
		}
	}
	else
	{
		unichar_t *t_unicode_text_ptr;
		t_unicode_text_ptr = (unichar_t *)t_text_ptr;
		
		uindex_t t_word_start;
		t_word_start = 0;
		while(t_word_start < p_text_length / 2)
		{
			uindex_t t_word_end;
			t_word_end = t_word_start;
			while(t_unicode_text_ptr[t_word_end] != ' ' && t_word_end < p_text_length / 2)
				t_word_end++;
			while(t_unicode_text_ptr[t_word_end] == ' ' && t_word_end < p_text_length / 2)
				t_word_end++;
			
			p_callback(p_context, t_unicode_text_ptr + t_word_start, (t_word_end - t_word_start) * 2, p_is_unicode);
			
			t_word_start = t_word_end;
		}
	}
	
	if (t_text_ptr != p_text)
		free(t_text_ptr);
}

struct iphone_measure_text_context_t
{
	void *font;
	float width;
};

static void iphone_do_measure_text(void *p_context, const void *p_text, uint32_t p_text_length, bool p_is_unicode)
{
	iphone_measure_text_context_t *t_context;
	t_context = (iphone_measure_text_context_t *)p_context;
	
	NSString *t_string;
	t_string = [[NSString alloc] initWithBytes: (uint8_t *)p_text length: p_text_length 
									  encoding: (p_is_unicode ? NSUTF16LittleEndianStringEncoding : NSMacOSRomanStringEncoding)];
	
	UIFont *t_font;
	t_font = (UIFont *)t_context -> font;
	
	t_context -> width += ceil([ t_string sizeWithFont: t_font ] . width);
	
	[t_string release];
}

static float iphone_measure_text(void *p_font, const void *p_text, uint32_t p_text_length, bool p_is_unicode)
{
	uindex_t t_word_start;
	t_word_start = 0;
	
	iphone_measure_text_context_t t_context;
	t_context . font = p_font;
	t_context . width = 0.0f;
	for_each_word(p_text, p_text_length, p_is_unicode, iphone_do_measure_text, &t_context);
	
	return t_context . width;
}

struct iphone_draw_text_context_t
{
	void *font;
	float x;
	float y;
};

static void iphone_do_draw_text(void *p_context, const void *p_text, uint32_t p_text_length, bool p_is_unicode)
{
	iphone_draw_text_context_t *t_context;
	t_context = (iphone_draw_text_context_t *)p_context;
	
	NSString *t_string;
	t_string = [[NSString alloc] initWithBytes: (uint8_t *)p_text length: p_text_length 
									  encoding: (p_is_unicode ? NSUTF16LittleEndianStringEncoding : NSMacOSRomanStringEncoding)];
	
	UIFont *t_font;
	t_font = (UIFont *)t_context -> font;
	
	CGSize t_size;
	t_size = [t_string drawAtPoint: CGPointMake(t_context -> x, t_context -> y - ceilf([ t_font ascender ])) withFont: t_font];
	
	t_context -> x += ceil(t_size . width);
	
	[t_string release];
}

static void iphone_draw_text(void *p_font, CGContextRef p_context, CGFloat x, CGFloat y, const void *p_text, uint32_t p_text_length, bool p_is_unicode)
{
	UIGraphicsPushContext(p_context);
	
	iphone_draw_text_context_t t_context;
	t_context . font = p_font;
	t_context . x = x;
	t_context . y = y;
	for_each_word(p_text, p_text_length, p_is_unicode, iphone_do_draw_text, &t_context);
	
	UIGraphicsPopContext();
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
	
	MCGIntRectangle t_text_bounds, t_clipped_bounds;
	MCGAffineTransform t_transform;
	MCGPoint t_device_location;
	if (t_success)
	{
		MCGRectangle t_float_text_bounds;
        t_float_text_bounds . origin . x = 0;
        t_float_text_bounds . origin . y = -p_font . ascent;
        t_float_text_bounds . size . width = MCGContextMeasurePlatformText(self, p_text, p_length, p_font);
        t_float_text_bounds . size . height = p_font . ascent + p_font . descent;
		
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
        CGContextScaleCTM(t_cgcontext, 1.0, -1.0);                
		
		CGContextSetRGBFillColor(t_cgcontext, 0.0, 0.0, 0.0, 1.0);        
        iphone_draw_text(p_font . fid, t_cgcontext, 0, 0, p_text, p_length, true);
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

	return iphone_measure_text(p_font . fid, p_text, p_length, true);
}

////////////////////////////////////////////////////////////////////////////////

/* Copyright (C) 2003-2017 LiveCode Ltd.
 
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

#include <SkFont.h>
#include <SkTypeface.h>
#include <SkTypeface_mac.h>

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
    const CGFloat t_colour_components[] = {1.0, 1.0};
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

// Creates an SkTypeface from a CTFontRef
static inline sk_sp<SkTypeface> MCGSkTypefaceFromCTFontRef(CTFontRef p_ref)
{
    return sk_sp<SkTypeface>(SkCreateTypefaceFromCTFont(p_ref, nil));
}

// Creates an SkPaint from a CTFontRef
static inline SkPaint MCGSkPaintFromCTFontRef(CTFontRef p_ref, const MCGAffineTransform& p_transform)
{
    // Convert the CTFontRef into a Skia Typeface object
    auto t_typeface = MCGSkTypefaceFromCTFontRef(p_ref);
    
    // Configure the paint for text rendering.
    // Both LCD font smoothing and embedded bitmap rendering are enabled
    SkPaint t_paint;
    t_paint.setTypeface(t_typeface);
    t_paint.setTextSize(CTFontGetSize(p_ref));
    t_paint.setLCDRenderText(true);
    t_paint.setEmbeddedBitmapText(true);
    
    // Set the transform we're using
    SkMatrix t_matrix;
    MCGAffineTransformToSkMatrix(p_transform, t_matrix);
    t_paint.setTextMatrix(&t_matrix);
    
    // All our font rendering uses UTF-16 codeunits
    t_paint.setTextEncoding(SkPaint::kUTF16_TextEncoding);
    
    return t_paint;
}

////////////////////////////////////////////////////////////////////////////////

static CTLineRef MCGCreateCTLineFromText(const unichar_t *p_text, uindex_t p_length, const MCGFont &p_font)
{
    bool t_success = true;
    
    CFStringRef t_cftext = nil;

    if (t_success)
    {
        t_cftext = CFStringCreateWithCharacters(kCFAllocatorDefault, p_text, p_length / sizeof(unichar_t));
        t_success = t_cftext != nil;
    }
    
    CFDictionaryRef t_attributes = nil;

    if (t_success)
    {
        CTFontRef t_font = static_cast<CTFontRef>(p_font.fid);

        CFStringRef t_keys[] =
        {
            kCTFontAttributeName,
            kCTForegroundColorAttributeName,
        };
        
        CFTypeRef t_values[] =
        {
            t_font,
            s_text_colour,
        };
        t_attributes = CFDictionaryCreate(NULL,
                                          (const void **)&t_keys,
                                          (const void **)&t_values,
                                          sizeof(t_keys) / sizeof(t_keys[0]),
                                          &kCFTypeDictionaryKeyCallBacks,
                                          &kCFTypeDictionaryValueCallBacks);
        
        t_success = t_attributes != nil;
    }
    
    CFAttributedStringRef t_attributed_text = nil;

    if (t_success)
    {
        t_attributed_text = CFAttributedStringCreate(nil, t_cftext, t_attributes);
        t_success = t_attributed_text != nil;
    }
    
    CTLineRef t_line = nil;

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

// Wrapper used for managing glyph/position arrays
template <class T>
class MCGMaybeOwnedArray
{
public:
    
    MCGMaybeOwnedArray(uindex_t t_length) :
      m_length(t_length)
    {
    }
    
    MCGMaybeOwnedArray(const MCGMaybeOwnedArray&) = delete;
    
    ~MCGMaybeOwnedArray()
    {
        if (m_owned)
            MCMemoryDeleteArray(m_array);
    }
    
    bool TryBorrow(const T* p_array)
    {
        MCAssert(m_array == nil);
        
        if (p_array != nil)
            m_array = const_cast<T*>(p_array);
        
        return p_array != nil;
    }
    
    bool Allocate()
    {
        MCAssert(m_array == nil);
        
        m_owned = true;
        return MCMemoryNewArray(m_length, m_array);
    }
    
    operator T* ()
    {
        return m_array;
    }
    
private:
    
    T* m_array = nil;
    uindex_t m_length = 0;
    bool m_owned = false;
};

////////////////////////////////////////////////////////////////////////////////

template <class Callback>
static void MCGForEachRunInLine(CTLineRef p_line, Callback p_callback)
{
    // Get the array containing the runs
    CFArrayRef t_runs = CTLineGetGlyphRuns(p_line);
    for (CFIndex i = 0; i < CFArrayGetCount(t_runs); i++)
    {
        // Get this run
        CTRunRef t_run = static_cast<CTRunRef>(CFArrayGetValueAtIndex(t_runs, i));
        
        // Execute the callback
        p_callback(t_run);
    }
}

template <class Callback>
static void MCGForEachGlyphInRun(CTRunRef p_run, Callback p_callback)
{
    // Number of glyphs in the run
    CFIndex t_length = CTRunGetGlyphCount(p_run);
    
    // Get a copy of the glyph array
    MCGMaybeOwnedArray<CGGlyph> t_glyphs(t_length);
    if (!t_glyphs.TryBorrow(CTRunGetGlyphsPtr(p_run)))
    {
        if (t_glyphs.Allocate())
            CTRunGetGlyphs(p_run, CFRangeMake(0, t_length), t_glyphs);
    }
    
    // Get a copy of the advances array
    MCGMaybeOwnedArray<CGPoint> t_points(t_length);
    if (!t_points.TryBorrow(CTRunGetPositionsPtr(p_run)))
    {
        if (t_points.Allocate())
            CTRunGetPositions(p_run, CFRangeMake(0, t_length), t_points);
    }
    
    // Iterate over the contents of the glyph array
    if (t_glyphs != nil && t_points != nil)
    {
        for (CFIndex i = 0; i < t_length; i++)
        {
            // Execute the callback
            p_callback(t_points[i], t_glyphs[i]);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////

void MCGContextDrawPlatformText(MCGContextRef self, const unichar_t *p_text, uindex_t p_length, MCGPoint p_location, const MCGFont &p_font, bool p_rtl)
{
	if (!MCGContextIsValid(self))
		return;	
	
    // The transform that drawing will be using
    MCGAffineTransform t_transform = MCGContextGetDeviceTransform(self);
    
    // Create a fully laid-out line of text
    CTLineRef t_line = MCGCreateCTLineFromText(p_text, p_length, p_font);
    
    // Iterate over the runs of text within the line
    MCGForEachRunInLine(t_line,
    [=](CTRunRef p_run)
    {
        // Get the attribute dictionary for this run
        CFDictionaryRef t_attributes = CTRunGetAttributes(p_run);
        
        // Get the CTFontRef used for drawing this run
        CTFontRef t_font = static_cast<CTFontRef>(CFDictionaryGetValue(t_attributes, kCTFontAttributeName));
        
        // Calculate the origin of this run
        CGAffineTransform t_text_transform = CTRunGetTextMatrix(p_run);
        MCGFloat t_x = p_location.x + t_text_transform.tx;
        MCGFloat t_y = p_location.y + t_text_transform.ty;
        
        // Create a Skia paint object wrapping the font
        SkPaint t_paint = MCGSkPaintFromCTFontRef(t_font, t_transform);
        
        // Customise the paint based on the current settings
        t_paint.setStyle(SkPaint::kFill_Style);
        t_paint.setColor(MCGColorToSkColor(self->state->fill_color));
        t_paint.setBlendMode(MCGBlendModeToSkBlendMode(self->state->blend_mode));
        
        // Force anti-aliasing on so that we get nicely-rendered text
        t_paint.setAntiAlias(true);
        
        // We are going to draw text by glyph ID rather than by codeunit
        t_paint.setTextEncoding(SkPaint::kGlyphID_TextEncoding);
        
        // Iterate through the glyphs of the run
        MCGForEachGlyphInRun(p_run,
        [=](CGPoint p_point, CGGlyph p_glyph)
        {
            // Both Skia and CG glyphs are just indices into the typeface glyph list
            SkGlyphID t_glyph = p_glyph;
            
            // Adjust the position for this glyph
            MCGFloat t_glyph_x = t_x + p_point.x;
            MCGFloat t_glyph_y = t_y + p_point.y;
            
            // Finally, draw this glyph to the canvas
            self->layer->canvas->drawText(&t_glyph, sizeof(t_glyph), t_glyph_x, t_glyph_y, t_paint);
        });
    });
    
    // Release the typeset line
    CFRelease(t_line);
}

////////////////////////////////////////////////////////////////////////////////

MCGFloat __MCGContextMeasurePlatformText(MCGContextRef self, const unichar_t *p_text, uindex_t p_length, const MCGFont &p_font, const MCGAffineTransform &p_transform)
{
    // The transform that drawing will be using
    MCGAffineTransform t_transform;
    if (self != nil)
        t_transform = MCGContextGetDeviceTransform(self);
    else
        t_transform = MCGAffineTransformMakeIdentity();
    
    // Create a fully laid-out line of text
    CTLineRef t_line = MCGCreateCTLineFromText(p_text, p_length, p_font);
    
    // Left and right bounds of the line
    MCGFloat t_left = 0;
    MCGFloat t_right = 0;
    
    // Iterate over the runs of text within the line
    MCGForEachRunInLine(t_line,
    [=, &t_left, &t_right](CTRunRef p_run)
    {
        // Get the attribute dictionary for this run
        CFDictionaryRef t_attributes = CTRunGetAttributes(p_run);
        
        // Get the CTFontRef used for drawing this run
        CTFontRef t_font = static_cast<CTFontRef>(CFDictionaryGetValue(t_attributes, kCTFontAttributeName));
        
        // Calculate the origin of this run
        CGAffineTransform t_text_transform = CTRunGetTextMatrix(p_run);
        MCGFloat t_x = t_text_transform.tx;
        
        // Create a Skia paint object wrapping the font
        SkPaint t_paint = MCGSkPaintFromCTFontRef(t_font, t_transform);
        
        // Force anti-aliasing on so that we get nicely-rendered text
        t_paint.setAntiAlias(true);
        
        // We are going to draw text by glyph ID rather than by codeunit
        t_paint.setTextEncoding(SkPaint::kGlyphID_TextEncoding);
        
        // Get the glyphs belonging to this run
        CFIndex t_glyph_count = CTRunGetGlyphCount(p_run);
        MCGMaybeOwnedArray<CGGlyph> t_glyphs(t_glyph_count);
        if (!t_glyphs.TryBorrow(CTRunGetGlyphsPtr(p_run)))
        {
            if (t_glyphs.Allocate())
                CTRunGetGlyphs(p_run, CFRangeMake(0, t_glyph_count), t_glyphs);
        }
        
        // Measure the run
        // This line depends on the assumption that CGGlyph and SkGlyphID are the same type
        MCGFloat t_width = t_paint.measureText(t_glyphs, t_glyph_count * sizeof(CGGlyph));
        
        // Check for extension of the left-hand bound of the run
        t_left = MCMin(t_left, t_x);
        
        // Check for extension of the right-hand bound of the run
        t_right = MCMax(t_right, t_x + t_width);
    });
    
    // Release typeset line
    CFRelease(t_line);
    
    // Return the width of the run
    return t_right - t_left;
}

// TODO: this seems to have no callers
bool MCGContextMeasurePlatformTextImageBounds(MCGContextRef self, const unichar_t *p_text, uindex_t p_length, const MCGFont &p_font, const MCGAffineTransform &p_transform, MCGRectangle &r_bounds)
{
	// Convert the CTFontRef to a Skia paint object
    CTFontRef t_ctfont = static_cast<CTFontRef>(p_font.fid);
    SkPaint t_paint = MCGSkPaintFromCTFontRef(t_ctfont, p_transform);
    
    // Calculate the bounds of the text
    SkRect t_rect;
    t_paint.measureText(p_text, p_length, &t_rect);
    
    // Return the bounds
    r_bounds = MCGRectangleFromSkRect(t_rect);
    return true;
}

////////////////////////////////////////////////////////////////////////////////

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

#include "prefix.h"

#include "globdefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "filedefs.h"
#include "mcerror.h"


#include "util.h"
#include "font.h"
#include "dispatch.h"
#include "globals.h"
#include "uidc.h"
#include "context.h"
#include "stacklst.h"
#include "flst.h"

#include "graphics_util.h"

////////////////////////////////////////////////////////////////////////////////

#define kMCFontBreakTextCharLimit   64

////////////////////////////////////////////////////////////////////////////////

struct MCFont
{
	uint32_t references;

	MCFont *next;

	MCNameRef name;
	MCFontStyle style;
	int32_t size;
	
	// MW-2013-12-05: [[ Bug 11535 ]] If non-zero, then each glyph will have the
	//   given fixed advance width (used for monospaced fonts).
	int32_t fixed_advance;
	
	MCFontStruct *fontstruct;
};


// TD-2013-07-01: [[ DynamicFonts ]]
struct MCLoadedFont
{
    MCLoadedFont *next;
    MCStringRef path;
    bool is_global;
    void *handle;
};

static MCLoadedFont *s_loaded_fonts = nil;
static MCFont *s_fonts = nil;

bool MCFontInitialize(void)
{
	s_fonts = nil;
	s_loaded_fonts = nil;
	return true;
}

void MCFontFinalize(void)
{
    while(s_loaded_fonts != nullptr)
    {
        MCFontUnload(s_loaded_fonts->path);
    }
}

bool MCFontCreate(MCNameRef p_name, MCFontStyle p_style, int32_t p_size, MCFontRef& r_font)
{
	for(MCFont *t_font = s_fonts; t_font != nil; t_font = t_font -> next)
	{
		if (p_name == t_font -> name &&
			p_style == t_font -> style &&
			p_size == t_font -> size)
		{
			t_font -> references += 1;
			r_font = t_font;
			return true;
		}
	}
    
    uint2 t_temp_size;
    MCFontStruct* t_font_struct;
    t_temp_size = p_size;
    t_font_struct = MCdispatcher -> loadfont(p_name, t_temp_size, MCFontStyleToTextStyle(p_style), (p_style & kMCFontStylePrinterMetrics) != 0);
    
    return MCFontCreateWithFontStruct(p_name, p_style, p_size, t_font_struct, r_font);
}

bool MCFontCreateWithFontStruct(MCNameRef p_name, MCFontStyle p_style, int32_t p_size, MCFontStruct* p_font_struct, MCFontRef& r_font)
{
    for(MCFont *t_font = s_fonts; t_font != nil; t_font = t_font -> next)
    {
        if (p_font_struct == t_font->fontstruct)
        {
            t_font -> references += 1;
            r_font = t_font;
            return true;
        }
    }
    
    MCFontRef self;
	if (!MCMemoryNew(self))
		return false;

	self -> references = 1;
    self->name = MCValueRetain(p_name);
	self -> style = p_style;
	self -> size = p_size;

    self -> fontstruct = p_font_struct;

	// MW-2013-12-04: [[ Bug 11535 ]] Test to see if the font is fixed-width, at least for
	//   Roman script.
    MCGFont t_gfont;
	t_gfont = MCFontStructToMCGFont(self -> fontstruct);
	t_gfont . fixed_advance = 0;

	// We check the width of ' ', i, l, m and w. If they are all the same width
	// we assume the font is monospaced and subsequently set the fixed_advance
	// field to a suitable value.
    MCGAffineTransform t_id = MCGAffineTransformMakeIdentity();
    auto t_measure_char_func =
        [&t_gfont, &t_id](unichar_t p_char) -> MCGFloat {
        return MCGContextMeasurePlatformText(nullptr, &p_char, 1*sizeof(p_char),
                                             t_gfont, t_id);
    };

    MCGFloat t_space_width = MCMax(t_measure_char_func(' '), 0.0f);
    if (t_space_width != 0)
    {
        static const unichar_t k_check_chars[] = {'i', 'l', 'm', 'w'};
        static const int k_check_count = sizeof(k_check_chars)/sizeof(*k_check_chars);
        for (int i = 0; i < k_check_count; ++i)
        {
            /* t_space_width is guaranteed to be non-zero, so if the
             * character width is measured to be zero it will not be
             * equal to t_space_width */
            if (t_measure_char_func(k_check_chars[i]) != t_space_width)
            {
                t_space_width = 0.0f;
                break;
            }
        }
    }
    self -> fixed_advance = floorf(t_space_width + 0.5);
	
	self -> next = s_fonts;
	s_fonts = self;

	r_font = self;

	return true;
}

bool MCFontCreateWithHandle(MCSysFontHandle p_handle, MCNameRef p_name, MCFontRef& r_font)
{
    MCFontStruct* t_font_struct;
    t_font_struct = MCdispatcher->loadfontwithhandle(p_handle, p_name);
    if (t_font_struct == nil)
        return false;
    
    MCNameRef t_name;
    uint2 t_size, t_style;
    Boolean t_printer;
    if (!MCdispatcher->getfontlist()->getfontstructinfo(t_name, t_size, t_style, t_printer, t_font_struct))
        return false;
    
    // The returned style is not the same as the MCFont* style values
    t_style = 0;
    
    return MCFontCreateWithFontStruct(t_name, t_style, t_size, t_font_struct, r_font);
}

MCFontRef MCFontRetain(MCFontRef self)
{
	self -> references += 1;
	return self;
}

void MCFontRelease(MCFontRef self)
{
	if (self == nil)
		return;

	self -> references -= 1;
	if (self -> references > 0)
		return;

	if (s_fonts != self)
	{
		MCFont *t_previous_font;
		for(t_previous_font = s_fonts; t_previous_font -> next != self; t_previous_font = t_previous_font -> next)
			;
		t_previous_font -> next = self -> next;
	}
	else
		s_fonts = self -> next;

	MCValueRelease(self -> name);
	MCMemoryDelete(self);
}

MCNameRef MCFontGetName(MCFontRef self)
{
	return self->name;
}

MCFontStyle MCFontGetStyle(MCFontRef self)
{
	return self->style;
}

int32_t MCFontGetSize(MCFontRef self)
{
	return self->size;
}

void *MCFontGetHandle(MCFontRef self)
{
#ifdef TARGET_SUBPLATFORM_ANDROID
    extern void* MCAndroidCustomFontCreateTypeface(MCStringRef, bool, bool);
    return MCAndroidCustomFontCreateTypeface(MCNameGetString(self->name),
                                             self->style & kMCFontStyleBold,
                                             self->style & kMCFontStyleItalic);
#else
    return self->fontstruct->fid;
#endif
}

bool MCFontHasPrinterMetrics(MCFontRef self)
{
	// MW-2013-12-19: [[ Bug 11559 ]] If the font has a nil font, do nothing.
    // PM-2015-04-16: [[ Bug 14244 ]] If the font is nil, do nothing
	if (self == nil || self -> fontstruct == nil)
		return false;
	
	return (self -> style & kMCFontStylePrinterMetrics) != 0;
}

coord_t MCFontGetAscent(MCFontRef self)
{
	// MW-2013-12-19: [[ Bug 11559 ]] If the font has a nil font, do nothing.
    // PM-2015-04-16: [[ Bug 14244 ]] If the font is nil, do nothing
	if (self == nil || self -> fontstruct == nil)
		return 0;
	
	return self -> fontstruct -> m_ascent;
}

coord_t MCFontGetDescent(MCFontRef self)
{
	// MW-2013-12-19: [[ Bug 11559 ]] If the font has a nil font, do nothing.
    // PM-2015-04-16: [[ Bug 14244 ]] If the font is nil, do nothing
	if (self == nil || self -> fontstruct == nil)
		return 0;
	
	return self -> fontstruct -> m_descent;
}

coord_t MCFontGetLeading(MCFontRef self)
{
    // PM-2015-06-02: [[ Bug 14244 ]] If the font is nil, do nothing
    if (self == nil || self -> fontstruct == nil)
        return 0;
    
    return self -> fontstruct -> m_leading;
}

coord_t MCFontGetXHeight(MCFontRef self)
{
    // PM-2015-06-02: [[ Bug 14244 ]] If the font is nil, do nothing
    if (self == nil || self -> fontstruct == nil)
        return 0;
    
    return self -> fontstruct -> m_xheight;
}

void MCFontBreakText(MCFontRef p_font, MCStringRef p_text, MCRange p_range, MCFontBreakTextCallback p_callback, void *p_callback_data, bool p_rtl)
{
	// MW-2013-12-19: [[ Bug 11559 ]] If the font has a nil font, do nothing.
    // PM-2015-04-16: [[ Bug 14244 ]] If the font is nil, do nothing
	if (p_font == nil || p_font -> fontstruct == nil)
		return;

    // If the text is small enough, don't bother trying to break it
    /*if (p_length <= (kMCFontBreakTextCharLimit * (p_is_unicode ? 2 : 1)))
    {
        p_callback(p_font, p_text, p_length, p_is_unicode, p_callback_data);
        return;
    }*/
    
    //p_callback(p_font, p_text, p_length, p_is_unicode, p_callback_data);
    //return;
    
    // Scan forward in the string for possible break locations. Breaks are
    // assigned a quality value as some locations are better for breaking than
    // others. The qualities are:
    //
    //  0.  no break found
    //  1.  grapheme break found
    //  2.  URL break found ('/' char)
    //  3.  word break found
    //
    // This isn't a particularly good algorithm but should suffice until full
    // Unicode support is added and a proper breaking algorithm implemented.
    uint32_t t_stride;
    t_stride = kMCFontBreakTextCharLimit;
    
    uindex_t t_end = p_range.offset + p_range.length;
    
    uindex_t t_length = p_range.length;
    uindex_t t_offset = (p_rtl) ? 0 : p_range.offset;
    
    while (t_length > 0)
    {
        int t_break_quality;
        uindex_t t_break_point, t_index;

        t_break_quality = 0;
        t_break_point = 0;
        t_index = 0;

        // Find the best break within the next stride characters. If there are
        // no breaking points, extend beyond the stride until one is found.
        while ((t_index < t_stride || t_break_quality == 0) && t_index < t_length)
        {
            codepoint_t t_char;
            uindex_t t_advance;
            
            if (p_rtl)
                t_char = MCStringGetCharAtIndex(p_text, t_end - t_index - t_offset);
            else
                t_char = MCStringGetCharAtIndex(p_text, t_offset + t_index);
            
            if (MCUnicodeCodepointIsHighSurrogate(t_char))
            {
                // Surrogate pair
                if (p_rtl)
                    t_char = MCUnicodeSurrogatesToCodepoint(t_char, MCStringGetCharAtIndex(p_text, t_end - t_index - t_offset - 1));
                else
                    t_char = MCUnicodeSurrogatesToCodepoint(t_char, MCStringGetCharAtIndex(p_text, t_offset + t_index + 1));
                
                t_advance = 2;
            }
            else
            {
                t_advance = 1;
            }
            
            // Prohibit breaks at the beginning of the string
            if (t_index == 0)
            {
                t_index += t_advance;
                continue;
            }
            
            if (t_char == ' ')
            {
                t_break_point = t_index;
                t_break_quality = 3;
            }
            else if (t_break_quality < 3 && t_char == '/')
            {
                t_break_point = t_index;
                t_break_quality = 2;
            }
            else if (t_break_quality < 2 && MCUnicodeCodepointIsBase(t_char))
            {
                t_break_point = t_index;
                t_break_quality = 1;
            }
            else if (t_break_quality < 2 && t_char > 0xFFFF)
            {
                // Character outside BMP, assume can break here
                t_break_point = t_index;
                t_break_quality = 1;
            }
            
            // If the break point is a word boundary, don't look for a later
            // breaking point. Words are cached as-is where possible.
            if (t_break_quality == 3)
                break;
            
            // Advance to the next character
            t_index += t_advance;
        }
        
        // If no word break was found and the whole of the remaining text was
        // searched and the remaining text is smaller than the break size then
        // don't attempt a break just for the sake of it.
        if (t_break_quality < 3 && t_length < kMCFontBreakTextCharLimit)
            t_break_point = t_length;
        
        // If no break point was found, just process the whole line
        if (t_break_quality == 0)
            t_break_point = t_length;
        
        // Process this chunk of text
        MCRange t_range;
        if (p_rtl)
            t_range = MCRangeMake(t_end - t_offset - t_break_point, t_break_point);
        else
            t_range = MCRangeMake(t_offset, t_break_point);

#if !defined(_WIN32) && !defined(_ANDROID_MOBILE) && !defined(__EMSCRIPTEN__)
        // This is a really ugly hack to get LTR/RTL overrides working correctly -
        // ATSUI and Pango think they know better than us and won't let us suppress
        // the BiDi algorithm they uses for text layout. So instead, we need to add
        // an LRO or RLO character in front of every single bit of text :-(
        MCAutoStringRef t_temp;
        unichar_t t_override;
        if (p_rtl)
            t_override = 0x202E;
        else
            t_override = 0x202D;
        /* UNCHECKED */ MCStringCreateMutable(0, &t_temp);
        /* UNCHECKED */ MCStringAppendChar(*t_temp, t_override);
        /* UNCHECKED */ MCStringAppendSubstring(*t_temp, p_text, t_range);
        /* UNCHECKED */ MCStringAppendChar(*t_temp, 0x202C);
        
        p_callback(p_font, *t_temp, MCRangeMake(0, MCStringGetLength(*t_temp)), p_callback_data);
#else
        // Another ugly hack - this time, to avoid incoming strings being coerced
        // into Unicode strings needlessly (because the drawing code uses unichars).
        // Do a mutable copy (to ensure an actual copy) before drawing.
        MCAutoStringRef t_temp;
        /* UNCHECKED */ MCStringMutableCopySubstring(p_text, t_range, &t_temp);
        p_callback(p_font, *t_temp, MCRangeMake(0, t_range.length), p_callback_data);
#endif
        
        // Explicitly show breaking points
        //p_callback(p_font, MCSTR("|"), MCRangeMake(0, 1), p_callback_data);
        
        // Move on to the next chunk
        t_offset += t_break_point;
        // SN-2014-07-23: [[ Bug 12910 ]] Script editor crashes
        //  Make sure we get 0 as a minimum, not a negative value since t_length is a uindex_t.
        if (t_length < t_break_point)
            t_length = 0;
        else
            t_length -= t_break_point;
    }
}

struct font_measure_text_context
{
	// MW-2013-12-19: [[ Bug 11606 ]] Make sure we use a float to accumulate the width.
    MCGFloat m_width;

	// MM-2014-04-16: [[ Bug 11964 ]] Store the transform that effects the measurement.
	MCGAffineTransform m_transform;
};

static void MCFontMeasureTextCallback(MCFontRef p_font, MCStringRef p_string, MCRange p_range, font_measure_text_context *ctxt)
{
    if (MCStringIsEmpty(p_string) || p_range.length == 0)
        return;
	
    MCGFont t_font;
	t_font = MCFontStructToMCGFont(p_font->fontstruct);
	
	// MW-2013-12-04: [[ Bug 11535 ]] Pass through the fixed advance.
	t_font . fixed_advance = p_font -> fixed_advance;
	
    ctxt -> m_width += MCGContextMeasurePlatformText(NULL, MCStringGetCharPtr(p_string) + p_range.offset, p_range.length*2, t_font, ctxt -> m_transform);
}

int32_t MCFontMeasureTextSubstring(MCFontRef p_font, MCStringRef p_text, MCRange p_range, const MCGAffineTransform &p_transform)
{
    return (int32_t)floorf(MCFontMeasureTextSubstringFloat(p_font, p_text, p_range, p_transform));
}

int32_t MCFontMeasureText(MCFontRef p_font, MCStringRef p_text, const MCGAffineTransform &p_transform)
{
	MCRange t_range = MCRangeMake(0, MCStringGetLength(p_text));
    return MCFontMeasureTextSubstring(p_font, p_text, t_range, p_transform);
}

MCGFloat MCFontMeasureTextFloat(MCFontRef p_font, MCStringRef p_text, const MCGAffineTransform &p_transform)
{
    MCRange t_range = MCRangeMake(0, MCStringGetLength(p_text));
    return MCFontMeasureTextSubstringFloat(p_font, p_text, t_range, p_transform);
}

MCGFloat MCFontMeasureTextSubstringFloat(MCFontRef p_font, MCStringRef p_string, MCRange p_range, const MCGAffineTransform &p_transform)
{
    font_measure_text_context ctxt;
    ctxt.m_width = 0;
    ctxt.m_transform = p_transform;
    
    MCFontBreakText(p_font, p_string, p_range, (MCFontBreakTextCallback)MCFontMeasureTextCallback, &ctxt, false);
    
    return ctxt . m_width;
}

struct font_measure_text_image_bounds_context
{
	MCGRectangle m_bounds;
	
	// MM-2014-04-16: [[ Bug 11964 ]] Store the transform that effects the measurement.
	MCGAffineTransform m_transform;
};

static void MCFontMeasureTextImageBoundsCallback(MCFontRef p_font, MCStringRef p_string, MCRange p_range, void *p_ctxt)
{
	font_measure_text_image_bounds_context *ctxt;
	ctxt = static_cast<font_measure_text_image_bounds_context*>(p_ctxt);
	
	if (MCStringIsEmpty(p_string) || p_range.length == 0)
		return;
	
	MCGFont t_font;
	t_font = MCFontStructToMCGFont(p_font->fontstruct);
	
	// MW-2013-12-04: [[ Bug 11535 ]] Pass through the fixed advance.
	t_font . fixed_advance = p_font -> fixed_advance;
	
	MCGRectangle t_bounds;
	if (MCGContextMeasurePlatformTextImageBounds(NULL, MCStringGetCharPtr(p_string) + p_range.offset, p_range.length*2, t_font, ctxt -> m_transform, t_bounds))
	{
		t_bounds.origin.x += ctxt->m_bounds.size.width;
		ctxt->m_bounds = MCGRectangleUnion(ctxt->m_bounds, t_bounds);
	}
}

bool MCFontMeasureTextImageBounds(MCFontRef p_font, MCStringRef p_string, const MCGAffineTransform &p_transform, MCGRectangle &r_bounds)
{
	return MCFontMeasureTextSubstringImageBounds(p_font, p_string, MCRangeMake(0, MCStringGetLength(p_string)), p_transform, r_bounds);
}

bool MCFontMeasureTextSubstringImageBounds(MCFontRef p_font, MCStringRef p_string, MCRange p_range, const MCGAffineTransform &p_transform, MCGRectangle &r_bounds)
{
	font_measure_text_image_bounds_context ctxt;
	ctxt.m_transform = p_transform;
	ctxt.m_bounds = MCGRectangleMake(0, 0, 0, 0);
	
	MCFontBreakText(p_font, p_string, p_range, MCFontMeasureTextImageBoundsCallback, &ctxt, false);
	
	r_bounds = ctxt.m_bounds;
	
	return true;
}

struct font_draw_text_context
{
    MCGContextRef m_gcontext;
	// MW-2013-12-19: [[ Bug 11606 ]] Make sure we use a float to accumulate the x-offset.
    MCGFloat x;
    MCGFloat y;
    bool rtl;
};

static void MCFontDrawTextCallback(MCFontRef p_font, MCStringRef p_text, MCRange p_range, font_draw_text_context *ctxt)
{
    MCGFont t_font;
	t_font = MCFontStructToMCGFont(p_font->fontstruct);

    // MW-2013-12-04: [[ Bug 11535 ]] Pass through the fixed advance.
	t_font . fixed_advance = p_font -> fixed_advance;
    
	// The drawing is done on the UTF-16 form of the text
	MCGContextDrawPlatformText(ctxt->m_gcontext, MCStringGetCharPtr(p_text) + p_range.offset, p_range.length*2, MCGPointMake(ctxt->x, ctxt->y), t_font, ctxt->rtl);

    // The draw position needs to be advanced
    // MM-2014-04-16: [[ Bug 11964 ]] Pass through the scale of the context to make sure the measurment is correct.
    ctxt -> x += MCGContextMeasurePlatformText(NULL, MCStringGetCharPtr(p_text) + p_range.offset, p_range.length*2, t_font, MCGContextGetDeviceTransform(ctxt->m_gcontext));
}

void MCFontDrawText(MCGContextRef p_gcontext, coord_t x, coord_t y, MCStringRef p_text, MCFontRef font, bool p_rtl, bool p_can_break)
{
	MCRange t_range = MCRangeMake(0, MCStringGetLength(p_text));
	return MCFontDrawTextSubstring(p_gcontext, x, y, p_text, t_range, font, p_rtl, p_can_break);
}

void MCFontDrawTextSubstring(MCGContextRef p_gcontext, coord_t x, coord_t y, MCStringRef p_text, MCRange p_range, MCFontRef p_font, bool p_rtl, bool p_can_break)
{
    font_draw_text_context ctxt;
    ctxt.x = x;
    ctxt.y = y;
    ctxt.m_gcontext = p_gcontext;
    ctxt.rtl = p_rtl;
    
    if (p_can_break)
        MCFontBreakText(p_font, p_text, p_range, (MCFontBreakTextCallback)MCFontDrawTextCallback, &ctxt, p_rtl);
    else
    {
        // AL-2014-08-20: [[ Bug 13186 ]] Ensure strings which skip go through the breaking algorithm
        //  don't get permanently converted to UTF-16 when drawn
        MCAutoStringRef t_temp;
        /* UNCHECKED */ MCStringMutableCopy(p_text, &t_temp);
        MCFontDrawTextCallback(p_font, *t_temp, p_range, &ctxt);
    }
}

MCFontStyle MCFontStyleFromTextStyle(uint2 p_text_style)
{
	MCFontStyle t_style;
	t_style = 0;

	if ((p_text_style & FA_ITALIC) != 0)
		t_style |= kMCFontStyleItalic;
	if ((p_text_style & FA_OBLIQUE) != 0)
		t_style |= kMCFontStyleOblique;
	if ((p_text_style & FA_WEIGHT) > 0x05)
		t_style |= kMCFontStyleBold;
	if ((p_text_style & FA_EXPAND) > 0x50)
		t_style |= kMCFontStyleExpanded;
	if ((p_text_style & FA_EXPAND) < 0x50)
		t_style |= kMCFontStyleCondensed;

	return t_style;
}

uint16_t MCFontStyleToTextStyle(MCFontStyle p_font_style)
{
	uint16_t t_style;
	t_style = FA_DEFAULT_STYLE;
	if ((p_font_style & kMCFontStyleItalic) != 0)
		t_style |= FA_ITALIC;
	if ((p_font_style & kMCFontStyleOblique) != 0)
		t_style |= FA_OBLIQUE;
	if ((p_font_style & kMCFontStyleBold) != 0)
		t_style = (t_style & ~FA_WEIGHT) | 0x07;
	if ((p_font_style & kMCFontStyleExpanded) != 0)
		t_style = (t_style & ~FA_EXPAND) | 0x70;
	if ((p_font_style & kMCFontStyleCondensed) != 0)
		t_style = (t_style & ~FA_EXPAND) | 0x30;
	return t_style;
}

void MCFontRemap(void)
{
	// Make sure the fontlist is flushed (deletes all fontstructs).
	MCdispatcher -> flushfonts();
	
	// Now reload the new fontstruct for each font.
	for(MCFont *t_font = s_fonts; t_font != nil; t_font = t_font -> next)
	{
		uint2 t_temp_size;
		t_temp_size = t_font -> size;
		t_font -> fontstruct = MCdispatcher -> loadfont(t_font -> name, t_temp_size, MCFontStyleToTextStyle(t_font -> style), (t_font -> style & kMCFontStylePrinterMetrics) != 0);
	}
}

////////////////////////////////////////////////////////////////////////////////

// TD-2013-07-02: [[ DynamicFonts ]]
// MERG-2013-08-14: [[ DynamicFonts ]] Refactored to use MCLoadedFont
bool MCFontLoad(MCStringRef p_path, bool p_globally)
{
    bool t_success;
    t_success = true;
    
    if (t_success)
    {
        // check if already loaded and unload if globally is not the same as loaded version
        for(MCLoadedFont *t_font = s_loaded_fonts; t_font != nil; t_font = t_font -> next)
            if (MCStringIsEqualTo(t_font -> path, p_path, kMCStringOptionCompareCaseless))
			{
				if (t_font -> is_global != p_globally)
				{
					t_success = MCFontUnload(p_path);
					break;
				}
				else
					return true;
			}
    }
    
    if (t_success)
    {
        void * t_loaded_font_handle;
        
        if (!MCscreen -> loadfont(p_path, p_globally, t_loaded_font_handle))
            return false;
    
        MCLoadedFontRef self;
        if (!MCMemoryNew(self))
            return false;
        
        self -> is_global = p_globally;
        self -> handle = t_loaded_font_handle;
        MCValueAssign(self -> path, p_path);
		self -> next = s_loaded_fonts;
		s_loaded_fonts = self;
		
		// MW-2013-09-11: [[ DynamicFonts ]] Make sure the engine reloads all fonts.
		MCFontRemap();
		MCstacks -> purgefonts();
    }

    return t_success;
}

bool MCFontUnload(MCStringRef p_path)
{
    MCLoadedFont *t_prev_font;
    t_prev_font = nil;
    for(MCLoadedFont *t_font = s_loaded_fonts; t_font != nil; t_font = t_font -> next)
    {
        if (MCStringIsEqualTo(t_font -> path, p_path, kMCStringOptionCompareCaseless))
        {
            if (!MCscreen -> unloadfont(p_path, t_font -> is_global, t_font -> handle))
                return false;
            
            if (t_prev_font != nil)
                t_prev_font -> next = t_font -> next;
            else
                s_loaded_fonts = t_font -> next;
			
			MCMemoryDelete(t_font -> path);
            MCMemoryDelete(t_font);
			
			// MW-2013-09-11: [[ DynamicFonts ]] Make sure the engine reloads all fonts.
			MCFontRemap();
			MCstacks -> purgefonts();
			
            break;
        }
		
        t_prev_font = t_font;
    }
    
    return true;
}

bool MCFontListLoaded(uindex_t& r_count, MCStringRef*& r_list)
{
    MCAutoStringRefArray t_list;
    
    bool t_success;
    t_success = true;
 
    // AL-2015-01-22: [[ Bug 14424 ]] 'the fontfilesinuse' can cause a crash, due to overreleasing MCStringRefs.
    //  Property getters always release afterwards, use an MCAutoStringRefArray which increses the refcount.
    for(MCLoadedFont *t_font = s_loaded_fonts; t_success && t_font != nil; t_font = t_font -> next)
        t_success = t_list . Push(t_font -> path);
    
    if (t_success)
        t_list . Take(r_list, r_count);

    return t_success;
}

////////////////////////////////////////////////////////////////////////////////

static const char *weightstrings[] =
    {
        "undefined",
        "ultralight",
        "extralight",
        "light",
        "semilight",
        "medium",
        "demibold",
        "bold",
        "extrabold",
        "ultrabold",
    };

static const char *expandstrings[] =
    {
        "undefined",
        "ultracondensed",
        "extracondensed",
        "condensed",
        "semicondensed",
        "normal",
        "semiexpanded",
        "expanded",
        "extraexpanded",
        "ultraexpanded",
    };

uint2 MCF_getweightint(uint2 style)
{
	return style & FA_WEIGHT;
}

const char *MCF_getweightstring(uint2 style)
{
	uint2 weight = MCF_getweightint(style);
	if (weight > MCFW_ULTRABOLD)
		weight = MCFW_UNDEFINED;
	return weightstrings[weight];
}

Boolean MCF_setweightstring(uint2 &style, MCStringRef data)
{
	uint2 w;
	for (w = MCFW_UNDEFINED ; w <= MCFW_ULTRABOLD ; w++)
		if (MCStringIsEqualToCString(data, weightstrings[w], kMCCompareCaseless))
			break;
	if (w <= MCFW_ULTRABOLD)
	{
		style &= ~FA_WEIGHT;
		style |= w;
		return True;
	}
	return False;
}

uint2 MCF_getexpandint(uint2 style)
{
	return (style >> 4) & 0x0f;
}

const char *MCF_getexpandstring(uint2 style)
{
	uint2 expand = MCF_getexpandint(style);
	if (expand > FE_ULTRAEXPANDED)
		expand = FE_UNDEFINED;
	return expandstrings[expand];
}

Boolean MCF_setexpandstring(uint2 &style, MCStringRef data)
{
	uint2 w;
	for (w = FE_UNDEFINED ; w <= FE_ULTRAEXPANDED ; w++)
		if (MCStringIsEqualToCString(data, expandstrings[w], kMCCompareCaseless))
			break;
	if (w <= FE_ULTRAEXPANDED)
	{
		style &= ~FA_EXPAND;
		style |= w << 4;
		return True;
	}
	return False;
}

const char *MCF_getslantshortstring(uint2 style)
{
	if (style & FA_ITALIC)
		return "i";
	if (style & FA_OBLIQUE)
		return "o";
	return "r";
}

const char *MCF_getslantlongstring(uint2 style)
{
	if (style & FA_ITALIC)
		return "italic";
	if (style & FA_OBLIQUE)
		return "oblique";
	return "";
}

Boolean MCF_setslantlongstring(uint2 &style, MCStringRef data)
{
	if (MCStringIsEqualToCString(data, "oblique", kMCCompareCaseless))
	{
		style |= FA_OBLIQUE;
		return True;
	}
	if (MCStringIsEqualToCString(data, "italic", kMCCompareCaseless))
	{
		style |= FA_ITALIC;
		return True;
	}
	return False;
}

Exec_stat MCF_parsetextatts(Properties which, MCStringRef data,
                            uint4 &flags, MCStringRef &fname, uint2 &height,
                            uint2 &size, uint2 &style)
{
	int2 i1;
	switch (which)
	{
	case P_TEXT_ALIGN:
		flags &= ~F_ALIGNMENT;
		if (MCStringIsEqualToCString(data, MCleftstring, kMCCompareCaseless) || MCStringIsEmpty(data))
			flags |= F_ALIGN_LEFT;
		else
			if (MCStringIsEqualToCString(data, MCcenterstring, kMCCompareCaseless))
				flags |= F_ALIGN_CENTER;
			else
				if (MCStringIsEqualToCString(data, MCrightstring, kMCCompareCaseless))
					flags |= F_ALIGN_RIGHT;
				else
					if (MCStringIsEqualToCString(data, MCjustifystring, kMCCompareCaseless))
						flags |= F_ALIGN_JUSTIFY;
					else
					{
						MCeerror->add(EE_OBJECT_BADALIGN, 0, 0, data);
						return ES_ERROR;
					}
		break;
	case P_TEXT_FONT:
		{// MW-2012-02-17: [[ IntrinsicUnicode ]] Strip any lang tag from the
			//   fontname.
			uindex_t t_offset;
			if (MCStringFirstIndexOfChar(data, ',', 0, kMCCompareExact, t_offset))
				/* UNCHECKED */ MCStringCopySubstring(data, MCRangeMakeMinMax(t_offset + 1, MCStringGetLength(data)), fname);
			else
				fname = MCValueRetain(data);
		}
		break;
	case P_TEXT_HEIGHT:
		if (!MCU_stoi2(data, i1))
		{
			MCeerror->add
			(EE_OBJECT_TEXTHEIGHTNAN, 0, 0, data);
			return ES_ERROR;
		}
		height = i1;
		break;
	case P_TEXT_SIZE:
		if (MCStringIsEmpty(data))
			i1 = 0;
		else
			if (!MCU_stoi2(data, i1))
			{
				MCeerror->add
				(EE_OBJECT_TEXTSIZENAN, 0, 0, data);
				return ES_ERROR;
			}
		size = i1;
		break;
	case P_TEXT_STYLE:
		{
			// MW-2012-02-17: [[ SplitTextAttrs ]] If the string is empty, then
			//   return 0 for the style - indicating to unset the property.
            if (MCStringIsEmpty(data))
				style = 0;
			else
            {
                style = FA_DEFAULT_STYLE;
                uindex_t t_start_pos, t_end_pos;
                t_end_pos = 0;
                
                while (t_end_pos < MCStringGetLength(data))
                {
                    t_start_pos = t_end_pos;
                    // skip spaces at the beginning or after a comma (if any)
                    MCU_skip_spaces(data, t_start_pos);
                    
                    uindex_t t_comma;
                    if (!MCStringFirstIndexOfChar(data, ',', t_start_pos, kMCCompareExact, t_comma))
                        t_end_pos = MCStringGetLength(data);
                    else
                        t_end_pos = t_comma;
                    
                    MCAutoStringRef tdata;
                    /* UNCHECKED */ MCStringCopySubstring(data, MCRangeMakeMinMax(t_start_pos, t_end_pos), &tdata);
                    t_end_pos++;
                    if (MCF_setweightstring(style, *tdata))
						continue;
					if (MCF_setexpandstring(style, *tdata))
						continue;
					if (MCF_setslantlongstring(style, *tdata))
						continue;
					if (MCStringIsEqualToCString(*tdata, MCplainstring, kMCCompareCaseless))
					{
						style = FA_DEFAULT_STYLE;
						continue;
					}
					if (MCStringIsEqualToCString(*tdata, MCmixedstring, kMCCompareCaseless))
					{
						style = FA_DEFAULT_STYLE;
						continue;
					}
					if (MCStringIsEqualToCString(*tdata, MCboxstring, kMCCompareCaseless))
					{
						style &= ~FA_3D_BOX;
						style |= FA_BOX;
						continue;
                    }
                
					if (MCStringIsEqualToCString(*tdata, MCthreedboxstring, kMCCompareCaseless))
					{
						style &= ~FA_BOX;
						style |= FA_3D_BOX;
						continue;
					}
					if (MCStringIsEqualToCString(*tdata, MCunderlinestring, kMCCompareCaseless))
					{
						style |= FA_UNDERLINE;
						continue;
					}
					if (MCStringIsEqualToCString(*tdata, MCstrikeoutstring, kMCCompareCaseless))
					{
						style |= FA_STRIKEOUT;
						continue;
					}
					if (MCStringIsEqualToCString(*tdata, MCgroupstring, kMCCompareCaseless) || MCStringIsEqualToCString(*tdata, MClinkstring, kMCCompareCaseless))
					{
						style |= FA_LINK;
						continue;
					}
					MCeerror->add(EE_OBJECT_BADSTYLE, 0, 0, data);
					return ES_ERROR;
				}
			}
		}
		break;
	default:
		break;
	}
	return ES_NORMAL;
}

    
Exec_stat MCF_unparsetextatts(Properties which, uint4 flags, MCStringRef name, uint2 height, uint2 size, uint2 style, MCValueRef &r_result)
{
	switch (which)
	{
	case P_TEXT_ALIGN:
		switch (flags & F_ALIGNMENT)
		{
		case F_ALIGN_LEFT:
            r_result = MCSTR(MCleftstring);
            break;
		case F_ALIGN_CENTER:
			r_result = MCSTR(MCcenterstring);
			break;
		case F_ALIGN_RIGHT:
			r_result = MCSTR(MCrightstring);
			break;
		case F_ALIGN_JUSTIFY:
			r_result = MCSTR(MCjustifystring);
            break;
		}
		break;
	case P_TEXT_FONT:
        r_result = MCValueRetain(name);
		break;
	case P_TEXT_HEIGHT:
        {
        MCAutoNumberRef t_height;
        /* UNCHECKED */ MCNumberCreateWithUnsignedInteger(height, &t_height);
        r_result = MCValueRetain(*t_height);
		break;
        }
	case P_TEXT_SIZE:
        {
        MCAutoNumberRef t_size;
        /* UNCHECKED */ MCNumberCreateWithUnsignedInteger(size, &t_size);
            r_result = MCValueRetain(*t_size);
		break;
        }
	case P_TEXT_STYLE:
		{
			if (style == FA_DEFAULT_STYLE)
			{
                r_result = MCSTR(MCplainstring);
				return ES_NORMAL;
			}

			if (r_result != nil)
                MCValueRelease(r_result);
            MCAutoListRef t_list;
            /* UNCHECKED */ MCListCreateMutable(',', &t_list);
			if (MCF_getweightint(style) != MCFW_MEDIUM)
                MCListAppendCString(*t_list, MCF_getweightstring(style));
			if (style & FA_ITALIC || style & FA_OBLIQUE)
                MCListAppendCString(*t_list, MCF_getslantlongstring(style));
			if (style & FA_BOX)
                MCListAppendCString(*t_list, MCboxstring);
			if (style & FA_3D_BOX)
                MCListAppendCString(*t_list, MCthreedboxstring);
			if (style & FA_UNDERLINE)
                MCListAppendCString(*t_list, MCunderlinestring);
			if (style & FA_STRIKEOUT)
                MCListAppendCString(*t_list, MCstrikeoutstring);
			if (style & FA_LINK)
                MCListAppendCString(*t_list, MClinkstring);
			if (MCF_getexpandint(style) != FE_NORMAL)
                MCListAppendCString(*t_list, MCF_getexpandstring(style));
            
            MCAutoStringRef t_string;
            /* UNCHECKED */ MCListCopyAsString(*t_list, &t_string);
            r_result = MCValueRetain(*t_string);
		}
		break;
	default:
		break;
	}
	return ES_NORMAL;
}

// MW-2011-11-23: [[ Array TextStyle ]] Convert a textStyle name into the enum.
Exec_stat MCF_parsetextstyle(MCStringRef data, Font_textstyle &style)
{
	if (MCStringIsEqualToCString(data, "bold", kMCCompareCaseless))
		style = FTS_BOLD;
	else if (MCStringIsEqualToCString(data, "condensed", kMCCompareCaseless))
		style = FTS_CONDENSED;
	else if (MCStringIsEqualToCString(data, "expanded", kMCCompareCaseless))
		style = FTS_EXPANDED;
	else if (MCStringIsEqualToCString(data, "italic", kMCCompareCaseless))
		style = FTS_ITALIC;
	else if (MCStringIsEqualToCString(data, "oblique", kMCCompareCaseless))
		style = FTS_OBLIQUE;
	else if (MCStringIsEqualToCString(data, MCboxstring, kMCCompareCaseless))
		style = FTS_BOX;
	else if (MCStringIsEqualToCString(data, MCthreedboxstring, kMCCompareCaseless))
		style = FTS_3D_BOX;
	else if (MCStringIsEqualToCString(data, MCunderlinestring, kMCCompareCaseless))
		style = FTS_UNDERLINE;
	else if (MCStringIsEqualToCString(data, MCstrikeoutstring, kMCCompareCaseless))
		style = FTS_STRIKEOUT;
	else if (MCStringIsEqualToCString(data, MCgroupstring, kMCCompareCaseless) || MCStringIsEqualToCString(data, MClinkstring, kMCCompareCaseless))
		style = FTS_LINK;
	else
	{
		MCeerror->add(EE_OBJECT_BADSTYLE, 0, 0, data);
		return ES_ERROR;
	}

	return ES_NORMAL;
}

// MW-2011-11-23: [[ Array TextStyle ]] Convert a textStyle enum into a string.
const char *MCF_unparsetextstyle(Font_textstyle style)
{
	switch(style)
	{
	case FTS_BOLD: return "bold";
	case FTS_CONDENSED: return "condensed";
	case FTS_EXPANDED: return "expanded";
	case FTS_ITALIC: return "italic";
	case FTS_OBLIQUE: return "oblique";
	case FTS_BOX: return "box";
	case FTS_3D_BOX: return "threedbox";
	case FTS_UNDERLINE: return "underline";
	case FTS_STRIKEOUT: return "strikeout";
	case FTS_LINK: return "link";
	default:
		break;
	}
	return "";
}

// MW-2011-11-23: [[ Array TextStyle ]] Check to see if the given style set contains the
//   given style.
bool MCF_istextstyleset(uint2 p_style, Font_textstyle style)
{
	switch(style)
	{
	case FTS_BOLD: return (p_style & FA_WEIGHT) == (FA_BOLD & FA_WEIGHT);
	case FTS_CONDENSED: return (p_style & FA_EXPAND) == (FA_EXPAND & FA_CONDENSED);
	case FTS_EXPANDED: return (p_style & FA_EXPAND) == (FA_EXPAND & FA_EXPANDED);
	case FTS_ITALIC: return (p_style & FA_ITALIC) != 0;
	case FTS_OBLIQUE: return (p_style & FA_OBLIQUE) != 0;
	case FTS_BOX: return (p_style & FA_BOX) != 0;
	case FTS_3D_BOX: return (p_style & FA_3D_BOX) != 0;
	case FTS_UNDERLINE: return (p_style & FA_UNDERLINE) != 0;
	case FTS_STRIKEOUT: return (p_style & FA_STRIKEOUT) != 0;
	case FTS_LINK: return (p_style & FA_LINK) != 0;
	default:
		break;
	}
	return false;
}

// MW-2011-11-23: [[ Array TextStyle ]] Turn the given style on or off in the style
//   set.
void MCF_changetextstyle(uint2& x_style_set, Font_textstyle p_style, bool p_new_state)
{
	uint2 t_flag;
	t_flag = 0;
	switch(p_style)
	{
	case FTS_BOLD:
		MCF_setweightstring(x_style_set, p_new_state ? MCSTR("bold") : MCSTR("medium"));
		return;
	case FTS_CONDENSED:
		MCF_setexpandstring(x_style_set, p_new_state ? MCSTR("condensed") : MCSTR("normal"));
		return;
	case FTS_EXPANDED:
		MCF_setexpandstring(x_style_set, p_new_state ? MCSTR("expanded") : MCSTR("normal"));
		return;
	case FTS_ITALIC:
		t_flag = FA_ITALIC;
		break;
	case FTS_OBLIQUE:
		t_flag = FA_OBLIQUE;
		break;
	case FTS_BOX:
		t_flag = FA_BOX;
		break;
	case FTS_3D_BOX:
		t_flag = FA_3D_BOX;
		break;
	case FTS_UNDERLINE:
		t_flag = FA_UNDERLINE;
		break;
	case FTS_STRIKEOUT:
		t_flag = FA_STRIKEOUT;
		break;
	case FTS_LINK:
		t_flag = FA_LINK;
		break;
	default:
		break;
	}
	if (p_new_state)
		x_style_set |= t_flag;
	else
		x_style_set &= ~t_flag;
}

/* LEGACY */
MCFontStruct* MCFontGetFontStruct(MCFontRef font)
{
    return font->fontstruct;
}

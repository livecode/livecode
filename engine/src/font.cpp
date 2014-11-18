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

#include "prefix.h"

#include "core.h"
#include "globdefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "filedefs.h"
#include "mcerror.h"

#include "execpt.h"
#include "util.h"
#include "font.h"
#include "dispatch.h"
#include "globals.h"
#include "uidc.h"
#include "context.h"
#include "stacklst.h"
#include "flst.h"

#include "graphics_util.h"
#include "unicode.h"

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
    char *path;
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
    t_font_struct = MCdispatcher -> loadfont(MCNameGetCString(p_name), t_temp_size, MCFontStyleToTextStyle(p_style), (p_style & kMCFontStylePrinterMetrics) != 0);
    
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
	/* UNCHECKED */ MCNameClone(p_name, self -> name);
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
	MCGFloat t_last_width;
	for(uindex_t i = 0; i < 5; i++)
	{
		unichar_t t_char;
		t_char = (unichar_t)((" ilmw")[i]);
		
		// MM-2014-04-16: [[ Bug 11964 ]] MCGContextMeasurePlatformText prototype updated to take scale. Pass identity.
		MCGFloat t_this_width;
		t_this_width = MCGContextMeasurePlatformText(nil, &t_char, 2, t_gfont, MCGAffineTransformMakeIdentity());
		if (t_this_width == 0.0 ||
			(i != 0 && t_this_width != t_last_width))
		{
			t_last_width = 0;
			break;
		}
		t_last_width = t_this_width;
	}
	self -> fixed_advance = floorf(t_last_width + 0.5);
	
	self -> next = s_fonts;
	s_fonts = self;

	r_font = self;

	return true;
}

bool MCFontCreateWithHandle(MCSysFontHandle p_handle, MCFontRef& r_font)
{
    MCFontStruct* t_font_struct;
    t_font_struct = MCdispatcher->loadfontwithhandle(p_handle);
    if (t_font_struct == nil)
        return false;
    
    MCNameRef t_name;
    const char* t_name_cstring;
    uint2 t_size, t_style;
    Boolean t_printer;
    if (!MCdispatcher->getfontlist()->getfontstructinfo(t_name_cstring, t_size, t_style, t_printer, t_font_struct))
        return false;
    
    if (!MCNameCreateWithCString(t_name_cstring, t_name))
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

	MCNameDelete(self -> name);
	MCMemoryDelete(self);
}

bool MCFontHasPrinterMetrics(MCFontRef self)
{
	// MW-2013-12-19: [[ Bug 11559 ]] If the font has a nil font, do nothing.
	if (self -> fontstruct == nil)
		return false;
	
	return (self -> style & kMCFontStylePrinterMetrics) != 0;
}

coord_t MCFontGetAscent(MCFontRef self)
{
	// MW-2013-12-19: [[ Bug 11559 ]] If the font has a nil font, do nothing.
	if (self -> fontstruct == nil)
		return 0;
	
	return self -> fontstruct -> m_ascent;
}

coord_t MCFontGetDescent(MCFontRef self)
{
	// MW-2013-12-19: [[ Bug 11559 ]] If the font has a nil font, do nothing.
	if (self -> fontstruct == nil)
		return 0;
	
	return self -> fontstruct -> m_descent;
}

coord_t MCFontGetLeading(MCFontRef self)
{
    if (self -> fontstruct == nil)
        return 0;
    
    return self -> fontstruct -> m_leading;
}

coord_t MCFontGetXHeight(MCFontRef self)
{
    if (self -> fontstruct == nil)
        return 0;
    
    return self -> fontstruct -> m_xheight;
}

void MCFontBreakText(MCFontRef p_font, const char *p_text, uint32_t p_length, bool p_is_unicode, MCFontBreakTextCallback p_callback, void *p_callback_data)
{
	// MW-2013-12-19: [[ Bug 11559 ]] If the font has a nil font, do nothing.
	if (p_font -> fontstruct == nil)
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
    t_stride = kMCFontBreakTextCharLimit * (p_is_unicode ? 2 : 1);
    while (p_length > 0)
    {
        int t_break_quality;
        uint32_t t_break_point, t_index;
        t_break_quality = 0;
        t_break_point = 0;
        t_index = 0;
       
        uint32_t t_length;
        t_length = p_length / (p_is_unicode ? 2 : 1);
        
        // Find the best break within the next stride characters. If there are
        // no breaking points, extend beyond the stride until one is found.
        while ((t_index < t_stride || t_break_quality == 0) && t_index < t_length)
        {
            uint32_t t_char;
            uint32_t t_advance;
            
            if (p_is_unicode)
            {
                t_char = ((unichar_t*)p_text)[t_index];
                if (0xD800 <= t_char && t_char < 0xDC00)
                {
                    // Surrogate pair
                    t_char = ((t_char - 0xD800) << 10) + (((unichar_t*)p_text)[t_index+1] - 0xDC00);
                    t_advance = 2;
                }
                else
                {
                    t_advance = 1;
                }
            }
            else
            {
                t_char = p_text[t_index];
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
            else if (t_break_quality < 2 && 0xDC00 <= t_char && t_char <= 0xDFFF)
            {
                // Trailing character of surrogate pair
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
        uint32_t t_byte_len;
        t_byte_len = t_break_point * (p_is_unicode ? 2 : 1);
        p_callback(p_font, p_text, t_byte_len, p_is_unicode, p_callback_data);
        
        // Explicitly show breaking points
        //p_callback(p_font, "|", 1, false, p_callback_data);
        
        // Move on to the next chunk
        p_text += t_byte_len;
        p_length -= t_byte_len;
    }
}

struct font_measure_text_context
{
	// MW-2013-12-19: [[ Bug 11606 ]] Make sure we use a float to accumulate the width.
    MCGFloat m_width;

	// MM-2014-04-16: [[ Bug 11964 ]] Store the transform that effects the measurement.
	MCGAffineTransform m_transform;
};

static void MCFontMeasureTextCallback(MCFontRef p_font, const char *p_text, uint32_t p_length, bool p_is_unicode, font_measure_text_context *ctxt)
{
    if (p_length == 0 || p_text == NULL)
		return;
	
    MCGFont t_font;
	t_font = MCFontStructToMCGFont(p_font->fontstruct);
	
	// MW-2013-12-04: [[ Bug 11535 ]] Pass through the fixed advance.
	t_font . fixed_advance = p_font -> fixed_advance;
	
	// MW-2013-12-04: [[ Bug 11549 ]] Make sure unicode text is short-aligned.
	MCExecPoint ep;
	ep . setsvalue(MCString(p_text, p_length));
	if (!p_is_unicode)
		ep . nativetoutf16();
	else if ((((uintptr_t)ep . getsvalue() . getstring()) & 1) != 0)
		ep . grabsvalue();
	
	// MM-2014-04-16: [[ Bug 11964 ]] Pass through the transform.
    ctxt -> m_width += MCGContextMeasurePlatformText(NULL, (unichar_t *) ep . getsvalue() . getstring(), ep . getsvalue() . getlength(), t_font, ctxt -> m_transform);
}

// MM-2014-04-16: [[ Bug 11964 ]] Updated prototype to take transform parameter.
MCGFloat MCFontMeasureTextFloat(MCFontRef p_font, const char *p_text, uint32_t p_length, bool p_is_unicode, const MCGAffineTransform &p_transform)
{
    font_measure_text_context ctxt;
    ctxt.m_width = 0;
	ctxt.m_transform = p_transform;
    
    MCFontBreakText(p_font, p_text, p_length, p_is_unicode, (MCFontBreakTextCallback)MCFontMeasureTextCallback, &ctxt);
	
	return ctxt . m_width;
}

// MM-2014-04-16: [[ Bug 11964 ]] Updated prototype to take transform parameter.
int32_t MCFontMeasureText(MCFontRef p_font, const char *p_text, uint32_t p_length, bool p_is_unicode, const MCGAffineTransform &p_transform)
{
    return (int32_t)floorf(MCFontMeasureTextFloat(p_font, p_text, p_length, p_is_unicode, p_transform));
}

struct font_draw_text_context
{
    MCGContextRef m_gcontext;
	// MW-2013-12-19: [[ Bug 11606 ]] Make sure we use a float to accumulate the x-offset.
    MCGFloat x;
    int32_t y;
};

static void MCFontDrawTextCallback(MCFontRef p_font, const char *p_text, uint32_t p_length, bool p_is_unicode, font_draw_text_context *ctxt)
{
    MCGFont t_font;
	t_font = MCFontStructToMCGFont(p_font->fontstruct);
	
	// MW-2013-12-04: [[ Bug 11535 ]] Pass through the fixed advance.
	t_font . fixed_advance = p_font -> fixed_advance;
	
	// MW-2013-12-04: [[ Bug 11549 ]] Make sure unicode text is short-aligned.
	MCExecPoint ep;
	ep . setsvalue(MCString(p_text, p_length));
	if (!p_is_unicode)
		ep . nativetoutf16();
	else if ((((uintptr_t)ep . getsvalue() . getstring()) & 1) != 0)
		ep . grabsvalue();
	
	MCGContextDrawPlatformText(ctxt->m_gcontext, (unichar_t *) ep . getsvalue() . getstring(), ep . getsvalue() . getlength(), MCGPointMake(ctxt->x, ctxt->y), t_font);
    
    // The draw position needs to be advanced. Can this be done more efficiently?
	// MM-2014-04-16: [[ Bug 11964 ]] Pass through the scale of the context to make sure the measurment is correct.
    ctxt -> x += MCGContextMeasurePlatformText(NULL, (unichar_t*)ep.getsvalue().getstring(), ep.getsvalue().getlength(), t_font, MCGContextGetDeviceTransform(ctxt->m_gcontext));
}

void MCFontDrawText(MCGContextRef p_gcontext, coord_t x, int32_t y, const char *p_text, uint32_t p_length, MCFontRef p_font, bool p_is_unicode)
{
    font_draw_text_context ctxt;
    ctxt.x = x;
    ctxt.y = y;
    ctxt.m_gcontext = p_gcontext;
    
    MCFontBreakText(p_font, p_text, p_length, p_is_unicode, (MCFontBreakTextCallback)MCFontDrawTextCallback, &ctxt);
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
		t_font -> fontstruct = MCdispatcher -> loadfont(MCNameGetCString(t_font -> name), t_temp_size, MCFontStyleToTextStyle(t_font -> style), (t_font -> style & kMCFontStylePrinterMetrics) != 0);
	}
}

////////////////////////////////////////////////////////////////////////////////

// TD-2013-07-02: [[ DynamicFonts ]]
// MERG-2013-08-14: [[ DynamicFonts ]] Refactored to use MCLoadedFont
Exec_stat MCFontLoad(MCExecPoint& ep, const char *p_path, bool p_globally)
{
    Exec_stat t_stat;
    t_stat = ES_NORMAL;
    
    if (t_stat == ES_NORMAL)
    {
        // check if already loaded and unload if globally is not the same as loaded version
        for(MCLoadedFont *t_font = s_loaded_fonts; t_font != nil; t_font = t_font -> next)
            if (MCU_strcasecmp(t_font -> path, p_path) == 0)
			{
				if (t_font -> is_global != p_globally)
				{
					t_stat = MCFontUnload(ep,p_path);
					break;
				}
				else
					return ES_NORMAL;
			}
    }
    
    if (t_stat == ES_NORMAL)
    {
        void * t_loaded_font_handle;
        
        if (!MCscreen -> loadfont(p_path, p_globally, t_loaded_font_handle))
            return ES_ERROR;
    
        MCLoadedFontRef self;
        if (!MCMemoryNew(self))
            return ES_ERROR;
        
        self -> is_global = p_globally;
        self -> handle = t_loaded_font_handle;
        self -> path = strdup(p_path);
		self -> next = s_loaded_fonts;
		s_loaded_fonts = self;
		
		// MW-2013-09-11: [[ DynamicFonts ]] Make sure the engine reloads all fonts.
		MCFontRemap();
		MCstacks -> purgefonts();
    }

    return t_stat;
}

Exec_stat MCFontUnload(MCExecPoint& ep, const char *p_path)
{
    MCLoadedFont *t_prev_font;
    t_prev_font = nil;
    for(MCLoadedFont *t_font = s_loaded_fonts; t_font != nil; t_font = t_font -> next)
    {
        if (MCU_strcasecmp(t_font -> path, p_path) == 0)
        {
            if (!MCscreen -> unloadfont(p_path, t_font -> is_global, t_font -> handle))
                return ES_ERROR;
            
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
    
    return ES_NORMAL;
}

Exec_stat MCFontListLoaded(MCExecPoint& ep)
{
    ep.clear();
    for(MCLoadedFont *t_font = s_loaded_fonts; t_font != nil; t_font = t_font -> next)
        ep.concatcstring(t_font -> path, EC_RETURN, t_font == s_loaded_fonts);
    
    return ES_NORMAL;
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

Boolean MCF_setweightstring(uint2 &style, const MCString &data)
{
	uint2 w;
	for (w = MCFW_UNDEFINED ; w <= MCFW_ULTRABOLD ; w++)
		if (data == weightstrings[w])
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

Boolean MCF_setexpandstring(uint2 &style, const MCString &data)
{
	uint2 w;
	for (w = FE_UNDEFINED ; w <= FE_ULTRAEXPANDED ; w++)
		if (data == expandstrings[w])
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

Boolean MCF_setslantlongstring(uint2 &style, const MCString &data)
{
	if (data == "oblique")
	{
		style |= FA_OBLIQUE;
		return True;
	}
	if (data == "italic")
	{
		style |= FA_ITALIC;
		return True;
	}
	return False;
}

Exec_stat MCF_parsetextatts(Properties which, const MCString &data,
                            uint4 &flags, char *&fname, uint2 &height,
                            uint2 &size, uint2 &style)
{
	int2 i1;
	switch (which)
	{
	case P_TEXT_ALIGN:
		flags &= ~F_ALIGNMENT;
		if (data == MCleftstring || data.getlength() == 0)
			flags |= F_ALIGN_LEFT;
		else
			if (data == MCcenterstring)
				flags |= F_ALIGN_CENTER;
			else
				if (data == MCrightstring)
					flags |= F_ALIGN_RIGHT;
				else
					if (data == MCjustifystring)
						flags |= F_ALIGN_JUSTIFY;
					else
					{
						MCeerror->add(EE_OBJECT_BADALIGN, 0, 0, data);
						return ES_ERROR;
					}
		break;
	case P_TEXT_FONT:
		{
			fname = data.clone();
			
			// MW-2012-02-17: [[ IntrinsicUnicode ]] Strip any lang tag from the
			//   fontname.
			char *t_tag;
			t_tag = strchr(fname, ',');
			if (t_tag != nil)
				t_tag[0] = '\0';
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
		if (data.getlength() == 0)
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
			uint4 l = data.getlength();
			const char *sptr = data.getstring();
			if (l == 0)
				style = 0;
			else
			{
				style = FA_DEFAULT_STYLE;
				while (l)
				{
					const char *startptr = sptr;
					if (!MCU_strchr(sptr, l, ','))
					{
						sptr += l;
						l = 0;
					}
					MCString tdata(startptr, sptr - startptr);
					MCU_skip_char(sptr, l);
					MCU_skip_spaces(sptr, l);
					if (MCF_setweightstring(style, tdata))
						continue;
					if (MCF_setexpandstring(style, tdata))
						continue;
					if (MCF_setslantlongstring(style, tdata))
						continue;
					if (tdata == MCplainstring)
					{
						style = FA_DEFAULT_STYLE;
						continue;
					}
					if (tdata == MCmixedstring)
					{
						style = FA_DEFAULT_STYLE;
						continue;
					}
					if (tdata == MCboxstring)
					{
						style &= ~FA_3D_BOX;
						style |= FA_BOX;
						continue;
					}
					if (tdata == MCthreedboxstring)
					{
						style &= ~FA_BOX;
						style |= FA_3D_BOX;
						continue;
					}
					if (tdata == MCunderlinestring)
					{
						style |= FA_UNDERLINE;
						continue;
					}
					if (tdata == MCstrikeoutstring)
					{
						style |= FA_STRIKEOUT;
						continue;
					}
					if (tdata == MCgroupstring || tdata == MClinkstring)
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

Exec_stat MCF_unparsetextatts(Properties which, MCExecPoint &ep, uint4 flags,
                              const char *name, uint2 height, uint2 size,
                              uint2 style)
{
	switch (which)
	{
	case P_TEXT_ALIGN:
		switch (flags & F_ALIGNMENT)
		{
		case F_ALIGN_LEFT:
			ep.setstaticcstring(MCleftstring);
			break;
		case F_ALIGN_CENTER:
			ep.setstaticcstring(MCcenterstring);
			break;
		case F_ALIGN_RIGHT:
			ep.setstaticcstring(MCrightstring);
			break;
		case F_ALIGN_JUSTIFY:
			ep.setstaticcstring(MCjustifystring);
			break;
		}
		break;
	case P_TEXT_FONT:
		ep.setsvalue(name);
		break;
	case P_TEXT_HEIGHT:
		ep.setint(height);
		break;
	case P_TEXT_SIZE:
		ep.setint(size);
		break;
	case P_TEXT_STYLE:
		{
			if (style == FA_DEFAULT_STYLE)
			{
				ep.setstaticcstring(MCplainstring);
				return ES_NORMAL;
			}
			
			uint32_t j;
			j = 0;

			ep.clear();
			if (MCF_getweightint(style) != MCFW_MEDIUM)
				ep.concatcstring(MCF_getweightstring(style), EC_COMMA, j++ == 0);
			if (style & FA_ITALIC || style & FA_OBLIQUE)
				ep.concatcstring(MCF_getslantlongstring(style), EC_COMMA, j++ == 0);
			if (style & FA_BOX)
				ep.concatcstring(MCboxstring, EC_COMMA, j++ == 0);
			if (style & FA_3D_BOX)
				ep.concatcstring(MCthreedboxstring, EC_COMMA, j++ == 0);
			if (style & FA_UNDERLINE)
				ep.concatcstring(MCunderlinestring, EC_COMMA, j++ == 0);
			if (style & FA_STRIKEOUT)
				ep.concatcstring(MCstrikeoutstring, EC_COMMA, j++ == 0);
			if (style & FA_LINK)
				ep.concatcstring(MClinkstring, EC_COMMA, j++ == 0);
			if (MCF_getexpandint(style) != FE_NORMAL)
				ep.concatcstring(MCF_getexpandstring(style), EC_COMMA, j++ == 0);
		}
		break;
	default:
		break;
	}
	return ES_NORMAL;
}

// MW-2011-11-23: [[ Array TextStyle ]] Convert a textStyle name into the enum.
Exec_stat MCF_parsetextstyle(const MCString &data, Font_textstyle &style)
{
	if (data == "bold")
		style = FTS_BOLD;
	else if (data == "condensed")
		style = FTS_CONDENSED;
	else if (data == "expanded")
		style = FTS_EXPANDED;
	else if (data == "italic")
		style = FTS_ITALIC;
	else if (data == "oblique")
		style = FTS_OBLIQUE;
	else if (data == MCboxstring)
		style = FTS_BOX;
	else if (data == MCthreedboxstring)
		style = FTS_3D_BOX;
	else if (data == MCunderlinestring)
		style = FTS_UNDERLINE;
	else if (data == MCstrikeoutstring)
		style = FTS_STRIKEOUT;
	else if (data == MCgroupstring || data == MClinkstring)
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
		MCF_setweightstring(x_style_set, p_new_state ? "bold" : "medium");
		return;
	case FTS_CONDENSED:
		MCF_setexpandstring(x_style_set, p_new_state ? "condensed" : "normal");
		return;
	case FTS_EXPANDED:
		MCF_setexpandstring(x_style_set, p_new_state ? "expanded" : "normal");
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

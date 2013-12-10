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

#include "globdefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "filedefs.h"
#include "mcerror.h"

//#include "execpt.h"
#include "util.h"
#include "font.h"
#include "dispatch.h"
#include "globals.h"
#include "uidc.h"
#include "context.h"
#include "stacklst.h"

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

	MCFontRef self;
	if (!MCMemoryNew(self))
		return false;

	self -> references = 1;
	/* UNCHECKED */ MCNameClone(p_name, self -> name);
	self -> style = p_style;
	self -> size = p_size;

	uint2 t_temp_size;
	t_temp_size = self -> size;
	self -> fontstruct = MCdispatcher -> loadfont(self -> name, t_temp_size, MCFontStyleToTextStyle(self -> style), (self -> style & kMCFontStylePrinterMetrics) != 0);

	self -> next = s_fonts;
	s_fonts = self;

	r_font = self;

	return true;
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
	return (self -> style & kMCFontStylePrinterMetrics) != 0;
}

int32_t MCFontGetAscent(MCFontRef self)
{
	return self -> fontstruct -> ascent;
}

int32_t MCFontGetDescent(MCFontRef self)
{
	return self -> fontstruct -> descent;
}

void MCFontBreakText(MCFontRef p_font, MCStringRef p_text, MCRange p_range, MCFontBreakTextCallback p_callback, void *p_callback_data)
{
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
    
    uindex_t t_length = p_range.length;
    uindex_t t_offset = p_range.offset;
    
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
            
            t_char = MCStringGetCharAtIndex(p_text, t_offset + t_index);
            if (0xD800 <= t_char && t_char < 0xDC00)
            {
                // Surrogate pair
                t_char = ((t_char - 0xD800) << 10) + (MCStringGetCharAtIndex(p_text, t_offset + t_index + 1) - 0xDC00);
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
        t_range = MCRangeMake(t_offset, t_index);
        p_callback(p_font, p_text, t_range, p_callback_data);
        
        // Explicitly show breaking points
        //p_callback(p_font, MCSTR("|"), MCRangeMake(0, 1), p_callback_data);
        
        // Move on to the next chunk
        t_offset += t_break_point;
        t_length -= t_break_point;
    }
}

struct font_measure_text_context
{
    uint32_t m_width;
};

static void MCFontMeasureTextCallback(MCFontRef p_font, MCStringRef p_string, MCRange p_range, font_measure_text_context *ctxt)
{
    if (MCStringIsEmpty(p_string) || p_range.length == 0)
        return;
	
    MCGFont t_font;
	t_font = MCFontStructToMCGFont(p_font->fontstruct);
	
    ctxt -> m_width += MCGContextMeasurePlatformText(NULL, MCStringGetCharPtr(p_string) + p_range.offset, p_range.length*2, t_font);
}

int32_t MCFontMeasureText(MCFontRef p_font, MCStringRef p_text)
{
	MCRange t_range = MCRangeMake(0, MCStringGetLength(p_text));
	return MCFontMeasureTextSubstring(p_font, p_text, t_range);
}

int32_t MCFontMeasureTextSubstring(MCFontRef p_font, MCStringRef p_string, MCRange p_range)
{
    font_measure_text_context ctxt;
    ctxt.m_width = 0;
    
    MCFontBreakText(p_font, p_string, p_range, (MCFontBreakTextCallback)MCFontMeasureTextCallback, &ctxt);
    
    return ctxt . m_width;
}

struct font_draw_text_context
{
    MCGContextRef m_gcontext;
    int32_t x;
    int32_t y;
};

static void MCFontDrawTextCallback(MCFontRef p_font, MCStringRef p_text, MCRange p_range, font_draw_text_context *ctxt)
{
    MCGFont t_font;
	t_font = MCFontStructToMCGFont(p_font->fontstruct);

	// The drawing is done on the UTF-16 form of the text
	
	MCGContextDrawPlatformText(ctxt->m_gcontext, MCStringGetCharPtr(p_text) + p_range.offset, p_range.length*2, MCGPointMake(ctxt->x, ctxt->y), t_font);
    
    // The draw position needs to be advanced
    ctxt -> x += MCGContextMeasurePlatformText(NULL, MCStringGetCharPtr(p_text) + p_range.offset, p_range.length*2, t_font);
}

void MCFontDrawText(MCGContextRef p_gcontext, int32_t x, int32_t y, MCStringRef p_text, MCFontRef font)
{
	MCRange t_range = MCRangeMake(0, MCStringGetLength(p_text));
	return MCFontDrawTextSubstring(p_gcontext, x, y, p_text, t_range, font);
}

void MCFontDrawTextSubstring(MCGContextRef p_gcontext, int32_t x, int32_t y, MCStringRef p_text, MCRange p_range, MCFontRef p_font)
{
    font_draw_text_context ctxt;
    ctxt.x = x;
    ctxt.y = y;
    ctxt.m_gcontext = p_gcontext;
    
    MCFontBreakText(p_font, p_text, p_range, (MCFontBreakTextCallback)MCFontDrawTextCallback, &ctxt);
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
    MCAutoArray<MCStringRef> t_list;
    
    for(MCLoadedFont *t_font = s_loaded_fonts; t_font != nil; t_font = t_font -> next)
        if (!t_list . Push(t_font -> path))
            return false;
    
    t_list . Take(r_list, r_count);
    
    return true;
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
				/* UNCHECKED */ MCStringCopySubstring(data, MCRangeMake(t_offset + 1, MCStringGetLength(data) - (t_offset + 1)), fname);
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
                    /* UNCHECKED */ MCStringCopySubstring(data, MCRangeMake(t_start_pos, t_end_pos - t_start_pos), &tdata);
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

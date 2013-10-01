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

#include "execpt.h"
#include "util.h"
#include "font.h"
#include "dispatch.h"
#include "globals.h"
#include "uidc.h"
#include "context.h"

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

static MCFont *s_fonts = nil;

bool MCFontInitialize(void)
{
	s_fonts = nil;
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

int32_t MCFontMeasureText(MCFontRef p_font, MCStringRef p_text)
{
	MCRange t_range = MCRangeMake(0, MCStringGetLength(p_text));
	return MCFontMeasureTextSubstring(p_font, p_text, t_range);
}

int32_t MCFontMeasureText(MCFontRef font, const char *chars, uint32_t char_count, bool is_unicode)
{
	return MCscreen -> textwidth(font -> fontstruct, chars, char_count, is_unicode);
}

int32_t MCFontMeasureTextSubstring(MCFontRef p_font, MCStringRef p_text, MCRange p_range)
{
	const char_t *t_native_text;
	t_native_text = MCStringGetNativeCharPtr(p_text);
	if (t_native_text != nil)
		return MCFontMeasureText(p_font, (const char *)(t_native_text + p_range.offset), p_range.length, false);
	
	return MCFontMeasureText(p_font, (const char *)(MCStringGetCharPtr(p_text) + p_range.offset), p_range.length, true);
}

void MCFontDrawText(MCFontRef font, MCStringRef p_text, MCContext *context, int32_t x, int32_t y, bool image)
{
	MCRange t_range = MCRangeMake(0, MCStringGetLength(p_text));
	return MCFontDrawTextSubstring(font, p_text, t_range, context, x, y, image);
}

void MCFontDrawText(MCFontRef font, const char *chars, uint32_t char_count, bool is_unicode, MCContext *context, int32_t x, int32_t y, bool image)
{
	context -> drawtext(x, y, chars, char_count, font -> fontstruct, image, is_unicode);
}

void MCFontDrawTextSubstring(MCFontRef font, MCStringRef p_text, MCRange p_range, MCContext *context, int32_t x, int32_t y, bool image)
{
	const char_t *t_native_text;
	t_native_text = MCStringGetNativeCharPtr(p_text);
	if (t_native_text != nil)
		return MCFontDrawText(font, (const char *)(t_native_text + p_range.offset), p_range.length, false, context, x, y, image);
	
	return MCFontDrawText(font, (const char *)(MCStringGetCharPtr(p_text) + p_range.offset), p_range.length, true, context, x, y, image);
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
			uint4 l = MCStringGetLength(data);
			const char *sptr = MCStringGetCString(data);
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
                    MCAutoStringRef tdata;
					/* UNCHECKED */ MCStringCreateWithNativeChars((const char_t *)startptr, sptr - startptr, &tdata);
					MCU_skip_char(sptr, l);
					MCU_skip_spaces(sptr, l);
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

    
Exec_stat MCF_unparsetextatts(Properties which, MCExecPoint &ep, uint4 flags, MCStringRef name, uint2 height, uint2 size, uint2 style)
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
		ep.setsvalue(MCStringGetOldString(name));
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

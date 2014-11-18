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

//
// font style handling routines
//

#ifndef __MC_FONT__
#define __MC_FONT__

#include "graphics.h"

////////////////////////////////////////////////////////////////////////////////

typedef struct MCFont *MCFontRef;

typedef uint32_t MCFontStyle;
enum
{
	kMCFontStyleBold = 1 << 0,
	kMCFontStyleItalic = 1 << 1,
	kMCFontStyleOblique = 1 << 2,
	kMCFontStyleExpanded = 1 << 3,
	kMCFontStyleCondensed = 1 << 4,

	kMCFontStylePrinterMetrics = 1 << 5,
};

bool MCFontInitialize(void);
void MCFontFinalize(void);

bool MCFontCreate(MCNameRef name, MCFontStyle style, int32_t size, MCFontRef& r_font);
bool MCFontCreateWithFontStruct(MCNameRef name, MCFontStyle style, int32_t size, MCFontStruct*, MCFontRef& r_font);
bool MCFontCreateWithHandle(MCSysFontHandle, MCFontRef& r_font);
MCFontRef MCFontRetain(MCFontRef font);
void MCFontRelease(MCFontRef font);

bool MCFontHasPrinterMetrics(MCFontRef font);

coord_t MCFontGetAscent(MCFontRef font);
coord_t MCFontGetDescent(MCFontRef font);
coord_t MCFontGetLeading(MCFontRef font);
coord_t MCFontGetXHeight(MCFontRef font);

typedef void (*MCFontBreakTextCallback)(MCFontRef font, const char *start, uindex_t length, bool is_unicode, void *ctxt);
void MCFontBreakText(MCFontRef font, const char *chars, uint32_t char_count, bool is_unicode, MCFontBreakTextCallback callback, void *callback_data);
// MW-2013-12-19: [[ Bug 11606 ]] This returns the unrounded width of the text as a float - its used by
//   the field to calculate accumulated width of text in blocks.
// MM-2014-04-16: [[ Bug 11964 ]] Updated prototype to take transform parameter.
MCGFloat MCFontMeasureTextFloat(MCFontRef font, const char *chars, uint32_t char_count, bool is_unicode, const MCGAffineTransform &p_transform);
int32_t MCFontMeasureText(MCFontRef font, const char *chars, uint32_t char_count, bool is_unicode, const MCGAffineTransform &p_transform);
void MCFontDrawText(MCGContextRef ctxt, coord_t x, int32_t y, const char *p_chars, uint32_t p_char_count, MCFontRef p_font, bool is_unicode);

MCFontStyle MCFontStyleFromTextStyle(uint2 text_style);
uint16_t MCFontStyleToTextStyle(MCFontStyle font_style);

/* LEGACY */
MCFontStruct* MCFontGetFontStruct(MCFontRef font);

////////////////////////////////////////////////////////////////////////////////

// Initialize the logical font table.
bool MCLogicalFontTableInitialize(void);
// Finalize the logical font table.
void MCLogicalFontTableFinalize(void);

// Build the logical font table for object's starting at 'root'. This method
// recurses down the object hierarchy, creating entries for each font required
// by all the objects. It replaces any existing font table.
void MCLogicalFontTableBuild(MCObject *root, uint32_t part);

// Read the logical font table from a stream. It replaces any existing font table.
IO_stat MCLogicalFontTableLoad(IO_handle stream);

// Save the logical font table out to a stream.
IO_stat MCLogicalFontTableSave(IO_handle stream);

// Finish with the logical font table.
void MCLogicalFontTableFinish(void);

// Lookup the given font attrs in the logical font table and return the index
// for it.
uint2 MCLogicalFontTableMap(MCNameRef p_textfont, uint2 p_textstyle, uint2 p_textsize, bool p_unicode);

// Lookup the given index in the logical font table, and return its attrs.
void MCLogicalFontTableLookup(uint2 index, MCNameRef& r_textfont, uint2& r_textstyle, uint2& r_textsize, bool& r_unicode);


////////////////////////////////////////////////////////////////////////////////

typedef struct MCLoadedFont *MCLoadedFontRef;

// Attempt to load the given file as a font. If globally is true, the font is
// loaded for all applications, otherwise just for the current one. If the
// font is already loaded and the globally flag has changed an attempt is first
// made to unload it before loading it again.
Exec_stat MCFontLoad(MCExecPoint& ep, const char *p_path, bool p_globally);

// Attempt to unload the given file as a font.
Exec_stat MCFontUnload(MCExecPoint& ep, const char *p_path);

// List all currently loaded font files (loaded via MCFontLoad).
Exec_stat MCFontListLoaded(MCExecPoint& ep);

////////////////////////////////////////////////////////////////////////////////

enum Font_weight {
    MCFW_UNDEFINED,
    MCFW_ULTRALIGHT,
    MCFW_EXTRALIGHT,
    MCFW_LIGHT,
    MCFW_SEMILIGHT,
    MCFW_MEDIUM,
    MCFW_SEMIBOLD,
    MCFW_BOLD,
    MCFW_EXTRABOLD,
    MCFW_ULTRABOLD
};

enum Font_expand {
    FE_UNDEFINED,
    FE_ULTRACONDENSED,
    FE_EXTRACONDENSED,
    FE_CONDENSED,
    FE_SEMICONDENSED,
    FE_NORMAL,
    FE_SEMIEXPANDED,
    FE_EXPANDED,
    FE_EXTRAEXPANDED,
    FE_ULTRAEXPANDED
};

enum Font_textstyle {
	FTS_UNKNOWN,
	FTS_BOLD,
	FTS_CONDENSED,
	FTS_EXPANDED,
	FTS_ITALIC,
	FTS_OBLIQUE,
	FTS_BOX,
	FTS_3D_BOX,
	FTS_UNDERLINE,
	FTS_STRIKEOUT,
	FTS_LINK
};

extern uint2 MCF_getweightint(uint2 style);
extern const char *MCF_getweightstring(uint2 style);
extern Boolean MCF_setweightstring(uint2 &style, const MCString &data);
extern uint2 MCF_getexpandint(uint2 style);
extern const char *MCF_getexpandstring(uint2 style);
extern Boolean MCF_setexpandstring(uint2 &style, const MCString &data);
extern const char *MCF_getslantshortstring(uint2 style);
extern const char *MCF_getslantlongstring(uint2 style);
extern Boolean MCF_setslantlongstring(uint2 &style, const MCString &data);
extern Exec_stat MCF_parsetextatts(Properties which, const MCString &data,
	                                   uint4 &flags, char *&fname,
	                                   uint2 &height, uint2 &size, uint2 &style);
extern Exec_stat MCF_unparsetextatts(Properties which, MCExecPoint &ep,
	                                     uint4 flags, const char *name,
	                                     uint2 height, uint2 size, uint2 style);

extern Exec_stat MCF_parsetextstyle(const MCString &data, Font_textstyle &style);
extern const char *MCF_unparsetextstyle(Font_textstyle style);
extern void MCF_changetextstyle(uint2& x_style_set, Font_textstyle p_style, bool p_new_state);
extern bool MCF_istextstyleset(uint2 styleset, Font_textstyle style);

#endif

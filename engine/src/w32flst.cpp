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
#include "filedefs.h"
#include "parsedef.h"
#include "objdefs.h"

#include "font.h"
#include "util.h"

#include "globals.h"

#include "w32flst.h"
#include "w32printer.h"
#include "w32dc.h"

#include <strsafe.h>

typedef struct _x2W32Font
{
	const unichar_t *Xfontname;
	const unichar_t *W32fontname;
}
X2W32FontTable;



/* Can't use GetDeviceCaps() to set the screen width because the width is different */
/* in "small font" and "large font" modes. The screen width is 120 in large font mode */
/* which mass up the MC display. The workarround is to fix the screen width to 96 */
/* which is the small font mode size*/
#define SCREEN_WIDTH_FOR_FONT_USE  96

#define MAX_XFONT2W32FONT    14
#define MAX_ENCODING2FONT    14

static X2W32FontTable XWfonts[MAX_XFONT2W32FONT] =
    { //X&WIN font table
        { L"Charter", L"Courier New" },
        { L"Chicago", L"MS Sans Serif" },
        { L"Charcoal", L"MS Sans Serif" },
        { L"Clean", L"Courier New" },
        { L"Courier", L"Courier New" },
        { L"fixed", L"MS Sans Serif" },
        { L"Geneva", L"MS Sans Serif" },
        { L"Helvetica", L"Arial" },
        { L"Lucida", L"Lucida Console" },
        { L"LucidaBright", L"Lucida Console" },
        { L"LucidaTypewriter", L"Lucida Console" },
        { L"New Century Schoolbook", L"Times New Roman" },
        { L"terminal", L"Courier New" },
        { L"Times", L"Times New Roman" }
    };

uint2 weighttable[] =
    {
        FW_DONTCARE,
        FW_THIN,
        FW_EXTRALIGHT,
		FW_EXTRALIGHT,
        FW_LIGHT,
        FW_NORMAL,
        FW_MEDIUM,
        FW_SEMIBOLD,
        FW_BOLD,
        FW_EXTRABOLD,
        FW_HEAVY,
    };

/* W32 does not have this
   uint2 widthtable[] = {
   FWIDTH_DONT_CARE,
   FWIDTH_ULTRA_CONDENSED,
   FWIDTH_EXTRA_CONDENSED,
   FWIDTH_CONDENSED,
   FWIDTH_SEMI_CONDENSED,
   FWIDTH_NORMAL,
   FWIDTH_SEMI_EXPANDED,
   FWIDTH_EXPANDED,
   FWIDTH_EXTRA_EXPANDED,
   FWIDTH_ULTRA_EXPANDED,
   };*/

static void mapfacename(unichar_t *buffer, const unichar_t *source, bool printer)
{
	for (int i = 0; i < MAX_XFONT2W32FONT; i++)
	{
		if (lstrcmpiW(source, XWfonts[i].Xfontname) == 0)
		{
			/* UNCHECKED */ StringCchCopyW(buffer, LF_FACESIZE, XWfonts[i].W32fontname);
			return;
		}
	}

	// Mapping not found
	if (printer)
		StringCchCopyW(buffer, LF_FACESIZE, L"Arial");
	else if (buffer != source)
		StringCchCopyW(buffer, LF_FACESIZE, source);
}

struct Language2FontCharset
{
	Lang_charset language;
	uint1 charset;
};

static Language2FontCharset s_fontcharsetmap[] =
{
	{LCH_ENGLISH, ANSI_CHARSET},
	{LCH_ROMAN, ANSI_CHARSET},
	{LCH_JAPANESE, SHIFTJIS_CHARSET},
	{LCH_CHINESE, CHINESEBIG5_CHARSET},
	{LCH_KOREAN, HANGUL_CHARSET},
	{LCH_ARABIC, ARABIC_CHARSET},
	{LCH_HEBREW, HEBREW_CHARSET},
	{LCH_RUSSIAN, RUSSIAN_CHARSET},
	{LCH_TURKISH, TURKISH_CHARSET},
	{LCH_BULGARIAN, RUSSIAN_CHARSET},
	{LCH_UKRAINIAN, RUSSIAN_CHARSET},
	{LCH_POLISH, EASTEUROPE_CHARSET},
	{LCH_GREEK, GREEK_CHARSET},
	{LCH_SIMPLE_CHINESE, GB2312_CHARSET},
	{LCH_THAI, THAI_CHARSET},
	{LCH_VIETNAMESE, VIETNAMESE_CHARSET},
	{LCH_LITHUANIAN, BALTIC_CHARSET},
	{LCH_UNICODE, ANSI_CHARSET}
};

// Sets the lfFaceName of a LOGFONT structure to the correct name for the
// appropriate special UI font
static void set_facename_for_status_font(LOGFONTW& x_logfont)
{
    // Cache the face name to avoid repeated queries
    static WCHAR s_facename[sizeof(x_logfont.lfFaceName) / sizeof(x_logfont.lfFaceName[0])];
    if (s_facename[0] == 0)
    {
        NONCLIENTMETRICSW ncm;
        ncm.cbSize = sizeof(ncm);
        SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);
        memcpy(s_facename, ncm.lfStatusFont.lfFaceName, sizeof(s_facename));
    }
    
    memcpy(x_logfont.lfFaceName, s_facename, sizeof(s_facename));
}

static void set_facename_for_menu_font(LOGFONTW& x_logfont)
{
    // Cache the face name to avoid repeated queries
    static WCHAR s_facename[sizeof(x_logfont.lfFaceName) / sizeof(x_logfont.lfFaceName[0])];
    if (s_facename[0] == 0)
    {
        NONCLIENTMETRICSW ncm;
        ncm.cbSize = sizeof(ncm);
        SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);
        memcpy(s_facename, ncm.lfMenuFont.lfFaceName, sizeof(s_facename));
    }
    
    memcpy(x_logfont.lfFaceName, s_facename, sizeof(s_facename));
}

static void set_facename_for_message_font(LOGFONTW& x_logfont)
{
    // Cache the face name to avoid repeated queries
    static WCHAR s_facename[sizeof(x_logfont.lfFaceName) / sizeof(x_logfont.lfFaceName[0])];
    if (s_facename[0] == 0)
    {
        NONCLIENTMETRICSW ncm;
        ncm.cbSize = sizeof(ncm);
        SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);
        memcpy(s_facename, ncm.lfMessageFont.lfFaceName, sizeof(s_facename));
    }
    
    memcpy(x_logfont.lfFaceName, s_facename, sizeof(s_facename));
}

MCFontnode::MCFontnode(MCNameRef fname, uint2 &size, uint2 style, Boolean printer)
{
	// MW-2012-05-03: [[ Values* ]] 'reqname' is now an autoref type.
	reqname = fname;
	reqsize = size;
	reqstyle = style;
	reqprinter = printer;
	font = new (nothrow) MCFontStruct;
	if (MCnoui)
	{
		memset(font, 0, sizeof(MCFontStruct));
		return;
	}
	LOGFONTW logfont;
	memset(&logfont, 0, sizeof(LOGFONTW));

    // Is the font name one of the special UI font names?
    if (MCNameIsEqualTo(fname, MCN_font_usertext))
        set_facename_for_message_font(logfont);
    else if (MCNameIsEqualTo(fname, MCN_font_menutext))
        set_facename_for_menu_font(logfont);
    else if (MCNameIsEqualTo(fname, MCN_font_content))
        set_facename_for_message_font(logfont);
    else if (MCNameIsEqualTo(fname, MCN_font_message))
        set_facename_for_message_font(logfont);
    else if (MCNameIsEqualTo(fname, MCN_font_tooltip))
        set_facename_for_status_font(logfont);
    else if (MCNameIsEqualTo(fname, MCN_font_system))
        set_facename_for_message_font(logfont);
    else
    {
        //parse font and encoding
        MCStringRef t_name = MCNameGetString(fname);
        MCAutoStringRef t_font_name;
        uindex_t t_offset;
        if (MCStringFirstIndexOfChar(t_name, ',', 0, kMCCompareExact, t_offset))
        {
            MCStringCopySubstring(t_name, MCRangeMake(0, t_offset), &t_font_name);
            t_name = *t_font_name;
        }
        
        MCAutoStringRefAsWString t_fname_wstr;
        if (!t_fname_wstr.Lock(t_name))
            return;

        // Copy the family name
        if (StringCchCopyW(logfont.lfFaceName, LF_FACESIZE, *t_fname_wstr) != S_OK)
                return;
    }

	// MW-2012-05-03: [[ Bug 10180 ]] Make sure the default charset for the font
	//   is chosen - otherwise things like WingDings don't work!
	logfont.lfCharSet = DEFAULT_CHARSET;

	HDC hdc;

	// MW-2013-11-07: [[ Bug 11393 ]] 'printer' in the fontstruct now means use ideal
	//   metrics for rendering and measuring.
	MCScreenDC *pms = (MCScreenDC *)MCscreen;
	hdc = pms->getsrchdc();
	logfont.lfHeight = MulDiv(MulDiv(size, 7, 8),
	                          SCREEN_WIDTH_FOR_FONT_USE, 72);

	logfont.lfWeight = weighttable[MCF_getweightint(style)];
	if (style & FA_ITALIC)
		logfont.lfItalic = TRUE;
	if (style & FA_OBLIQUE)
		logfont.lfOrientation = 3600 - 150; /* 15 degree forward slant */
	
	HFONT newfont = CreateFontIndirectW(&logfont);
	SelectObject(hdc, newfont);

	unichar_t t_testname[LF_FACESIZE];
	memset(t_testname, 0, LF_FACESIZE * sizeof(unichar_t));
	GetTextFaceW(hdc, LF_FACESIZE, t_testname);

	// MW-2012-02-13: If we failed to find an exact match then remap the font name and
	//   try again.
	if (newfont == NULL || lstrcmpiW(t_testname, logfont.lfFaceName) != 0)
	{
		if (newfont != NULL)
			DeleteObject(newfont);

		mapfacename(logfont.lfFaceName, logfont.lfFaceName, printer == True);

		// Force the resulting font to be TrueType and with ANSI charset (otherwise
		// we get strange UI symbol font on XP).
		logfont.lfCharSet = ANSI_CHARSET;
		logfont.lfOutPrecision = OUT_TT_ONLY_PRECIS;
		newfont = CreateFontIndirectW(&logfont);
		SelectObject(hdc, newfont);
	}

	// At this point we will have either found an exact match for the textFont, mapped
	// using the defaults table and found a match, or been given a default truetype font.

	TEXTMETRICW tm;
	GetTextMetricsW(hdc, &tm);
	font->fid = (MCSysFontHandle)newfont;
	font->size = size;
	font->printer = printer;
    
    // We divide the internal leading evenly over ascent and descent. This is
    // done because we need to account for it but the text layout code doesn't
    // know about internal leading.
    font->m_ascent = tm.tmAscent + (tm.tmInternalLeading+1)/2;
    font->m_descent = tm.tmDescent + (tm.tmInternalLeading)/2;
    font->m_leading = tm.tmExternalLeading;
    font->m_xheight = tm.tmAveCharWidth;
}

MCFontnode::~MCFontnode()
{
	if (MCnoui)
		return;
	DeleteObject((HFONT)font->fid);
	delete font;
}

MCFontStruct *MCFontnode::getfont(MCNameRef fname, uint2 size,
                                  uint2 style, Boolean printer)
{
	// MW-2012-05-03: [[ Values* ]] Match the font name caselessly. 
	if (!MCNameIsEqualTo(fname, *reqname) || printer != reqprinter)
		return NULL;
	if (size == 0)
		return font;
	if (style != reqstyle || size != reqsize)
		return NULL;
	return font;
}

MCFontlist::MCFontlist()
{
	fonts = NULL;
}

MCFontlist::~MCFontlist()
{
	while (fonts != NULL)
	{
		MCFontnode *fptr = fonts->remove
		                   (fonts);
		delete fptr;
	}
}

MCFontStruct *MCFontlist::getfont(MCNameRef fname, uint2 &size, uint2 style, Boolean printer)
{
	MCFontnode *tmp = fonts;
	if (tmp != NULL)
		do
		{
			MCFontStruct *font = tmp->getfont(fname, size, style, printer);
			if (font != NULL)
				return font;
			tmp = tmp->next();
		}
		while (tmp != fonts);
	tmp = new (nothrow) MCFontnode(fname, size, style, printer);
	tmp->appendto(fonts);
	return tmp->getfont(fname, size, style, printer);
}

void MCFontlist::freeprinterfonts()
{
	MCFontnode *tmp = fonts;
	do
	{
		if (tmp->isprinterfont())
		{
			MCFontnode *nptr = tmp->next();
			tmp->remove
			(fonts);
			delete tmp;
			tmp = nptr;
		}
		else
			tmp = tmp->next();
	}
	while (tmp != fonts);
}

// SN-2015-03-02: [[ Bug 14661 ]] New context for the
//  font listing callback function.
//  An array is used to keep track of the font already
//  added (hash comparison faster than that looking for
//  a string in a list)
struct MCFontListing
{
	MCArrayRef array;
	MCListRef list;
};

int CALLBACK fontnames_FontFamProc(const LOGFONTW * lpelf,
								   const TEXTMETRICW * lpntm,
								   DWORD FontType, LPARAM lParam)
{
	// SN-2015-03-02: [[ Bug 14661 ]] A MCFontListing struct
	//  is now passed to the callback functions.
	MCFontListing* t_fontlist = (MCFontListing*)lParam;
	MCNewAutoNameRef t_name;

	size_t t_length;
	if (StringCchLengthW(lpelf->lfFaceName, LF_FACESIZE, &t_length) != S_OK)
		return False;

	if (!MCNameCreateWithChars(lpelf->lfFaceName, t_length, &t_name))
		return False;

	// SN-2015-03-02: [[ Bug 14661 ]] Skip names that 
	//  we have already added to the array, comparing
	//  full name to full name. That avoid to skip
	//  "Segoe UI" if "Segoe UI Light" has already been 
	//  loaded, like it was with MCStringContains
	MCValueRef t_value;
	if (MCArrayFetchValue(t_fontlist->array, true, *t_name, t_value))
		return True;

	// Add the font name in the array, and in the list.
	MCArrayStoreValue(t_fontlist -> array, true, *t_name, kMCEmptyString);
	return MCListAppend(t_fontlist->list, MCNameGetString(*t_name)) ? True : False;
}

bool MCFontlist::getfontnames(MCStringRef p_type, MCListRef& r_names)
{
	if (MCnoui)
	{
		r_names = MCValueRetain(kMCEmptyList);
		return true;
	}

	MCScreenDC *pms = (MCScreenDC *)MCscreen;
	HDC hdc;
#ifdef _DESKTOP
	// MM-2013-09-13:: [[ RefactorGraphics ]] Tweak to get things compiling for server.
	if (MCStringIsEqualToCString(p_type, "printer", kMCCompareCaseless))
		hdc = static_cast<MCWindowsPrinter *>(MCsystemprinter) -> GetDC();
	else
#endif
		hdc = pms->getsrchdc();
	LOGFONTW lf;
	lf.lfCharSet = DEFAULT_CHARSET;
	lf.lfFaceName[0] = '\0';
	lf.lfPitchAndFamily = 0;

	// SN-2015-03-02: [[ Bug 14661 ]] Initialise the
	//  MCFontListing members.
	bool t_success;
	MCFontListing t_fontlist = {nil, nil};

	t_success = MCArrayCreateMutable(t_fontlist.array)
				&& MCListCreateMutable('\n', t_fontlist.list);

	if (t_success)
		t_success = EnumFontFamiliesExW(hdc, &lf, (FONTENUMPROCW)fontnames_FontFamProc, (LPARAM)(&t_fontlist), 0) == True;

	if (t_success)
		t_success = MCListCopy(t_fontlist.list, r_names);

	// SN-2015-03-02: [[ Bug 14661 ]] Cleanup the
	//  MCFontListing members.
	MCValueRelease(t_fontlist . array);
	MCValueRelease(t_fontlist . list);

	return t_success;
}

typedef struct
{
	bool success;
	MCListRef list;
	uint2 *sizes;
	uindex_t size_count;
} fontsizes_context;

int CALLBACK fontsizes_FontFamProc(const ENUMLOGFONTW* lpelf,
								   const NEWTEXTMETRICW* lpntm,
								   DWORD FontType, LPARAM lParam)
{
	fontsizes_context *context = (fontsizes_context*)lParam;
	if (!(FontType & TRUETYPE_FONTTYPE))
	{ //if not true-type font
		uint2 size = uint2((((lpelf->elfLogFont.lfHeight * 72)
		               / SCREEN_WIDTH_FOR_FONT_USE) * 8 / 7));
		for (uindex_t i = 0; i < context->size_count; i++)
			if (context->sizes[i] == size)
				return True; //return to callback function again
		context->success = MCMemoryResizeArray(context->size_count + 1, context->sizes, context->size_count);
		if (context->success)
		{
			context->sizes[context->size_count - 1] = size;
			context->success = MCListAppendInteger(context->list, size);
		}
		return context->success ? True : False;
	}
	else
	{ //if true-type font, size is always 0.
		context->success = MCListAppendInteger(context->list, 0);
		return False; //stop right here. no more callback function
	}
}

bool MCFontlist::getfontsizes(MCStringRef p_fname, MCListRef& r_sizes)
{
	if (MCnoui)
	{
		r_sizes = MCValueRetain(kMCEmptyList);
		return true;
	}
	MCAutoListRef t_list;
	if (!MCListCreateMutable('\n', &t_list))
		return false;

	fontsizes_context context;
	context.list = *t_list;
	context.success = true;
	context.sizes = nil;
	context.size_count = 0;

	MCAutoStringRefAsWString t_fname_wstr;
	if (!t_fname_wstr.Lock(p_fname))
		return false;

	MCScreenDC *pms = (MCScreenDC *)MCscreen;
	HDC hdc = pms->getsrchdc();

	LOGFONTW lf;
	lf.lfCharSet = DEFAULT_CHARSET;
	lf.lfPitchAndFamily = 0;

	// Copy the font family name
	if (StringCchCopyW(lf.lfFaceName, LF_FACESIZE, *t_fname_wstr) != S_OK)
		return false;

	EnumFontFamiliesExW(hdc, &lf, (FONTENUMPROCW)fontsizes_FontFamProc, (LPARAM)&context, 0);

	MCMemoryDeleteArray(context.sizes);
	if (context.success)
		return MCListCopy(*t_list, r_sizes);

	return false;
}

#ifdef FOR_TRUE_TYPE_ONLY
static uint2 nfonts;
static MCListRef s_style_list;
enum FontQueryType {
    FQ_NAMES,
    FQ_SIZES,
    FQ_STYLES,
};

int CALLBACK MyFontFamProc(ENUMLOGFONTA FAR* lpelf,
                           NEWTEXTMETRICA FAR* lpntm,
                           int FontType, LPARAM lParam)
{
	switch (lParam)
	{
	case FQ_STYLES:
		{
			if (!(FontType & TRUETYPE_FONTTYPE))
			{ //not true-type font
				/* UNCHECKED */ MCListAppend(s_style_list, MCN_plain) &&
					MCListAppend(s_style_list, MCN_bold) &&
					MCListAppend(s_style_list, MCN_italic) &&
					MCListAppend(s_style_list, MCN_bold_italic);
				return False;  //stop right here, do not continue looping through callback
			}
			char *style = (char *)lpelf->elfStyle;
			if (strequal(style, "Regular"))
				/* UNCHECKED */ MCListAppend(s_style_list, MCN_plain);
			else if (strequal(style, "Bold Italic"))
				/* UNCHECKED */ MCListAppend(s_style_list, MCN_bold_italic)
			else
				/* UNCHECKED */ MCListAppendCString(s_style_list, (char*)lpelf->elfStyle);
			break;
		}
	default:
		break;
	}
	return True;
}
#endif

bool MCFontlist::getfontstyles(MCStringRef p_fname, uint2 fsize, MCListRef& r_styles)
{
	if (MCnoui)
	{
		r_styles = MCValueRetain(kMCEmptyList);
		return true;
	}

	MCAutoListRef t_list;
	if (!MCListCreateMutable('\n', &t_list))
		return false;

	bool t_success = true;
#ifdef FOR_TRUE_TYPE_ONLY
	s_style_list = *t_list;
	nfonts = 0;
	char mappedName[LF_FACESIZE];
	mapfacename(mappedName, fname);
	MCScreenDC *pms = (MCScreenDC *)MCscreen;
	HDC hdc = pms->getmemsrchdc();
	EnumFontFamiliesA(hdc, mappedName, (FONTENUMPROC)MyFontFamProc, FQ_STYLES);
#else
	t_success = MCListAppend(*t_list, MCN_plain) &&
		MCListAppend(*t_list, MCN_bold) &&
		MCListAppend(*t_list, MCN_italic) &&
		MCListAppend(*t_list, MCN_bold_italic);
#endif

	return t_success && MCListCopy(*t_list, r_styles);
}

bool MCFontlist::getfontstructinfo(MCNameRef& r_name, uint2 &r_size, uint2 &r_style, Boolean &r_printer, MCFontStruct *p_font)
{
	MCFontnode *t_font = fonts;
	while (t_font != NULL)
	{
		if (t_font->getfontstruct() == p_font)
		{
			r_name = t_font->getname();
			r_size = t_font->getsize();
			r_style = t_font->getstyle();
			r_printer = t_font->isprinterfont();
			return true;
		}
		t_font = t_font->next();
	}
	return false;
}

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

#include "w32prefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "parsedef.h"
#include "objdefs.h"

#include "font.h"
#include "util.h"

#include "globals.h"
#include "execpt.h"
#include "w32flst.h"
#include "w32printer.h"
#include "w32dc.h"

typedef struct _x2W32Font
{
	char *Xfontname;
	char *W32fontname;
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
        { "Charter", "Courier New" },
        { "Chicago", "MS Sans Serif" },
        { "Charcoal", "MS Sans Serif" },
        { "Clean", "Courier New" },
        { "Courier", "Courier New" },
        { "fixed", "MS Sans Serif" },
        { "Geneva", "MS Sans Serif" },
        { "Helvetica", "Arial" },
        { "Lucida", "Lucida Console" },
        { "LucidaBright", "Lucida Console" },
        { "LucidaTypewriter", "Lucida Console" },
        { "New Century Schoolbook", "Times New Roman" },
        { "terminal", "Courier New" },
        { "Times", "Times New Roman" }
    };

uint2 weighttable[] =
    {
        FW_DONTCARE,
        FW_THIN,
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

static void mapfacename(char *buffer, const char *source, bool printer)
{
	uint4 maxlength = MCU_min(LF_FACESIZE - 1U, strlen(source));
	strncpy(buffer, source, maxlength);
	buffer[maxlength] = '\0';
	uint2 i;
	for (i = 0; i < MAX_XFONT2W32FONT; i++)
		if (!MCU_strncasecmp(buffer, XWfonts[i].Xfontname, strlen(buffer) + 1))
		{
			strcpy(buffer, XWfonts[i].W32fontname);
			return;
		}
	if (printer)
		strcpy(buffer, "Arial");
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

MCFontnode::MCFontnode(MCNameRef fname, uint2 &size, uint2 style, Boolean printer)
{
	// MW-2012-05-03: [[ Values* ]] 'reqname' is now an autoref type.
	reqname = fname;
	reqsize = size;
	reqstyle = style;
	reqprinter = printer;
	font = new MCFontStruct;
	if (MCnoui)
	{
		memset(font, 0, sizeof(MCFontStruct));
		return;
	}
	LOGFONTA logfont;
	memset(&logfont, 0, sizeof(LOGFONTA));

	// MW-2012-05-03: [[ Values* ]] Fetch the native chars from the string directly. **UNICODE**
	uint4 maxlength;
	maxlength = MCStringGetNativeChars(MCNameGetString(fname), MCRangeMake(0, LF_FACESIZE - 1U), (char_t *)logfont . lfFaceName);
	logfont.lfFaceName[maxlength] = '\0';

	// MW-2012-05-03: [[ Bug 10180 ]] Make sure the default charset for the font
	//   is chosen - otherwise things like WingDings don't work!
	logfont.lfCharSet = DEFAULT_CHARSET;

	//parse font and encoding
	font->charset = 0;
	font->unicode = False;

	HDC hdc;
	if (printer)
	{
#ifdef _DESKTOP
		// MM-2013-09-13:: [[ RefactorGraphics ]] Tweak to get things compiling for server.
		hdc = static_cast<MCWindowsPrinter *>(MCsystemprinter) -> GetDC();
		logfont.lfHeight = -size;
#endif
	}
	else
	{
		MCScreenDC *pms = (MCScreenDC *)MCscreen;
		hdc = pms->getsrchdc();
		logfont.lfHeight = MulDiv(MulDiv(size, 7, 8),
		                          SCREEN_WIDTH_FOR_FONT_USE, 72);
	}
	logfont.lfWeight = weighttable[MCF_getweightint(style)];
	if (style & FA_ITALIC)
		logfont.lfItalic = TRUE;
	if (style & FA_OBLIQUE)
		logfont.lfOrientation = 3600 - 150; /* 15 degree forward slant */
	
	HFONT newfont = CreateFontIndirectA(&logfont);
	SelectObject(hdc, newfont);
	char testname[LF_FACESIZE];
	memset(testname, 0, LF_FACESIZE);
	GetTextFaceA(hdc, LF_FACESIZE, testname);

	// MW-2012-02-13: If we failed to find an exact match then remap the font name and
	//   try again.
	if (newfont == NULL || (MCU_strncasecmp(testname, logfont.lfFaceName, strlen(testname) + 1)))
	{
		if (newfont != NULL)
			DeleteObject(newfont);

		mapfacename(logfont.lfFaceName, logfont.lfFaceName, printer == True);

		// Force the resulting font to be TrueType and with ANSI charset (otherwise
		// we get strange UI symbol font on XP).
		logfont.lfCharSet = ANSI_CHARSET;
		logfont.lfOutPrecision = OUT_TT_ONLY_PRECIS;
		newfont = CreateFontIndirectA(&logfont);
		SelectObject(hdc, newfont);
	}

	// At this point we will have either found an exact match for the textFont, mapped
	// using the defaults table and found a match, or been given a default truetype font.

	TEXTMETRICA tm;
	GetTextMetricsA(hdc, &tm);
	font->fid = (MCSysFontHandle)newfont;
	font->size = size;
	font->ascent = MulDiv(tm.tmAscent, 15, 16);
	font->descent = tm.tmDescent;
	font->printer = printer;
	if (!printer)
	{
		INT table[256];
		uint2 i;
		if (GetCharWidth32A(hdc, 0, 255, table))
		{
			for (i = 0 ; i < 256 ; i++)
			{
				font->widths[i] = (uint1)table[i];
				
				// MW-2012-09-21: [[ Bug 3884 ]] If a single char width > 255 then mark
				//   the font as wide.
				if (table[i] > 255)
					font->wide = True;
			}
		}
		else
		{// must be Window 95
			MCScreenDC *pms = (MCScreenDC *)MCscreen;
			for (i = 0 ; i < 256 ; i++)
			{
				uint2 c = i;
				SIZE tsize;
				GetTextExtentPoint32A(hdc, (LPCSTR)&c, (int)1, &tsize);
				font->widths[i] = (uint1)tsize.cx;
				
				// MW-2012-09-21: [[ Bug 3884 ]] If a single char width > 255 then mark
				//   the font as wide.
				if (tsize.cx > 255)
					font->wide = True;
					
			}
		}
	}
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
	tmp = new MCFontnode(fname, size, style, printer);
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

int CALLBACK fontnames_FontFamProc(ENUMLOGFONTA FAR* lpelf,
								   NEWTEXTMETRICA FAR* lpntm,
								   int FontType, LPARAM lParam)
{
	MCListRef t_list = (MCListRef)lParam;
	return MCListAppendCString(t_list, lpelf->elfLogFont.lfFaceName) ? True : False;
}

bool MCFontlist::getfontnames(MCStringRef p_type, MCListRef& r_names)
{
	if (MCnoui)
	{
		r_names = MCValueRetain(kMCEmptyList);
		return true;
	}

	MCAutoListRef t_list;

	if (!MCListCreateMutable('\n', &t_list))
		return false;

	MCScreenDC *pms = (MCScreenDC *)MCscreen;
	HDC hdc;
#ifdef _DESKTOP
	// MM-2013-09-13:: [[ RefactorGraphics ]] Tweak to get things compiling for server.
	if (MCStringIsEqualToCString(p_type, "printer", kMCCompareCaseless))
		hdc = static_cast<MCWindowsPrinter *>(MCsystemprinter) -> GetDC();
	else
#endif
		hdc = pms->getsrchdc();
	if (EnumFontFamiliesA(hdc, NULL, (FONTENUMPROCA)fontnames_FontFamProc, (LPARAM)*t_list) != True)
		return false;

	return MCListCopy(*t_list, r_names);
}

typedef struct
{
	bool success;
	MCListRef list;
	uint2 *sizes;
	uindex_t size_count;
} fontsizes_context;

int CALLBACK fontsizes_FontFamProc(ENUMLOGFONTA FAR* lpelf,
								   NEWTEXTMETRICA FAR* lpntm,
								   int FontType, LPARAM lParam)
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

	MCScreenDC *pms = (MCScreenDC *)MCscreen;
	HDC hdc = pms->getsrchdc();
	EnumFontFamiliesA(hdc, MCStringGetCString(p_fname), (FONTENUMPROCA)fontsizes_FontFamProc, (LPARAM)&context);

	MCMemoryDeleteArray(context.sizes);
	if (context.success)
		return MCListCopy(*t_list, r_sizes);

	return false;
}

#ifdef FOR_TRUE_TYPE_ONLY
static MCExecPoint *epptr;
static uint2 nfonts;
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
				epptr->setstaticcstring("plain\nbold\nitalic\nbold-italic");
				return False;  //stop right here, do not continue looping through callback
			}
			char *style = (char *)lpelf->elfStyle;
			if (strequal(style, "Regular"))
				epptr->concatcstring("plain", EC_RETURN, nfonts++ == 0);
			else if (strequal(style, "Bold Italic"))
				epptr->concatcstring("bold-italic", EC_RETURN, nfonts++ == 0);
			else
				epptr->concatcstring((char*)lpelf->elfStyle, EC_RETURN, nfonts++ == 0);
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
	t_success = MCListAppend(*t_list, MCN_plain) &&
		MCListAppend(*t_list, MCN_bold) &&
		MCListAppend(*t_list, MCN_italic) &&
		MCListAppend(*t_list, MCN_bold_italic);
	return t_success && MCListCopy(*t_list, r_styles);

#ifdef FOR_TRUE_TYPE_ONLY

	epptr = &ep;
	nfonts = 0;
	char mappedName[LF_FACESIZE];
	mapfacename(mappedName, fname);
	MCScreenDC *pms = (MCScreenDC *)MCscreen;
	HDC hdc = pms->getmemsrchdc();
	EnumFontFamiliesA(hdc, mappedName, (FONTENUMPROC)MyFontFamProc, FQ_STYLES);
	ep.lower();
#endif
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

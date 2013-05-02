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
// MCFontlist class functions
//

#include "osxprefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "parsedef.h"
#include "objdefs.h"

#include "execpt.h"
#include "util.h"
#include "globals.h"
#include "osspec.h"

#include "osxdc.h"
#include "osxflst.h"

#define MAX_XFONT2MACFONT    11

typedef struct _x2MacFont
{
	const char *Xfontname;
	const char *Macfontname;
}
X2MacFontTable;

static X2MacFontTable XMfonts[MAX_XFONT2MACFONT] = { // X to Mac font table
            { "Charter", "Courier" },
            { "Clean", "Courier" },
            { "fixed", "Chicago" },
            { "Helvetica", "Geneva" },
            { "Lucida", "Courier" },
            { "LucidaBright", "Courier" },
            { "LucidaTypewriter", "Courier" },
            { "MS Sans Serif", "Geneva" },
            { "New Century Schoolbook", "Times" },
            { "System", "Charcoal" },
            { "terminal", "Courier" }
        };

MCFontnode::MCFontnode(MCNameRef fname, uint2 &size, uint2 style)
{
	reqname = fname;
	reqsize = size;
	reqstyle = style;
	char *tmpname;
	font = new MCFontStruct; //create MCFont structure
	font->unicode = False;
	font->charset = 0;
	font->wide = False;
	
	// MW-2005-05-10: Update this to FM type
	FMFontFamily ffamilyid;		    //font family ID
	
	tmpname = strclone(MCStringGetCString(MCNameGetString(fname)));//make a copy of the font name
	StringPtr reqnamePascal = c2pstr(tmpname);
	
	// MW-2005-05-10: Update this call to FM rountines
	ffamilyid = FMGetFontFamilyFromName(reqnamePascal);
	delete tmpname;
	if (font->charset && font->charset != LCH_UNICODE && FontToScript(ffamilyid) != MCS_charsettolangid(font->charset))
		ffamilyid = GetScriptVariable(MCS_charsettolangid(font->charset),smScriptAppFond);
	if (ffamilyid == 0)
	{ //font does not exist
		uint2 i;     // check the font mapping table
		for (i = 0 ; i < MAX_XFONT2MACFONT ; i++)
			if (MCNameIsEqualToCString(fname, XMfonts[i].Xfontname, kMCCompareCaseless))
			{ //find MAC equivalent of X font name
				tmpname = strclone(XMfonts[i].Macfontname);
				reqnamePascal = c2pstr(tmpname);
				// MW-2005-05-10: Update this call to FM rountines
				ffamilyid = FMGetFontFamilyFromName(reqnamePascal);
				delete tmpname;
				break;
			}
	}

	CGrafPtr oldport;
	GDHandle olddevice;
	GetGWorld(&oldport, &olddevice);
	MCScreenDC *pms = (MCScreenDC *)MCscreen;
	SetGWorld(GetWindowPort(pms->getinvisiblewin()), GetMainDevice());
	TextFont(ffamilyid);

	Style tstyle = 0;
	if ((style & FA_ITALIC) || (style & FA_OBLIQUE))
		tstyle |= italic;
	if ((style & FA_WEIGHT) > 0x05)
		tstyle |= bold;
	if ((style & FA_EXPAND) > 0x50)
		tstyle |= extend;
	if ((style & FA_EXPAND) < 0x50)
		tstyle |= condense;

	TextFace(tstyle);
	TextSize(size);  //set text size

	font->wide = False;
	font->fid = (MCSysFontHandle)ffamilyid;
	font->size = size;
	font->style = tstyle;
	
	font->ascent = size - 1;
	font->descent = size * 2 / 14 + 1;

	// potentially some problem here:
	// finding 16-bit fonts may cause problems with 8/16 string drawing
	uint2 i;
	char t[256];
	FillParseTable(t, smCurrentScript);
	
	FontInfo finfo;
	GetFontInfo(&finfo);     //get font info
	
	if (finfo.ascent + finfo.descent > size)
		font->ascent++;
	
	// use  GetScriptManagerVariable(smEnabled) to detect 16-bit fonts
	if (!font->charset)
		for (i = 0 ; i < 256 ; i++)
		{

			{
				char c = i;
				short w = TextWidth(&c, 0, 1);
				// MW-2012-09-21: [[ Bug 3884 ]] If a single char width > 255 then mark
				//   the font as wide.
				if (w > 255)
					font->wide = True;
				font->widths[i] = (uint1)w;
			}
		}
	SetGWorld(oldport, olddevice);
}

MCFontnode::~MCFontnode()
{
	delete font;
}

MCFontStruct *MCFontnode::getfont(MCNameRef fname, uint2 size, uint2 style)
{
	if (!MCNameIsEqualTo(fname, *reqname))
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
			MCFontStruct *font = tmp->getfont(fname, size, style);
			if (font != NULL)
				return font;
			tmp = tmp->next();
		}
		while (tmp != fonts);
	tmp = new MCFontnode(fname, size, style);
	tmp->appendto(fonts);
	return tmp->getfont(fname, size, style);
}

bool MCFontlist::getfontnames(MCStringRef p_type, MCListRef& r_names)
{ //get a list of all the available font in the system
	MCAutoListRef t_list;
	if (!MCListCreateMutable('\n', &t_list))
		return false;

	bool t_success = true;

	unsigned char fname[255]; //hold family font name
	
	FMFontFamilyIterator            fontFamilyIterator;
	FMFontFamily                    fontFamily;
	OSStatus  			    status;

	// Create an iterator to enumerate the font families.
	status = FMCreateFontFamilyIterator(NULL, NULL, kFMDefaultOptions,
										&fontFamilyIterator);
	while (t_success && (status = FMGetNextFontFamily(&fontFamilyIterator, &fontFamily)) == noErr)
	{
		if (FMGetFontFamilyName(fontFamily, fname) == noErr)
		{
			p2cstr(fname);
			if (fname[0] != '%' && fname[0] != '.')          //exclude printer fonts
				t_success = MCListAppendCString(*t_list, (char*)fname);
		}
	}
	
	FMDisposeFontFamilyIterator (&fontFamilyIterator);

	if (t_success)
		t_success = MCListCopy(*t_list, r_names);
	return t_success;
}

bool MCFontlist::getfontsizes(MCStringRef p_fname, MCListRef& r_sizes)
{
	FMFontFamily ffamilyID;
	/* UNCHECKED */ char *tmpname = strclone(MCStringGetCString(p_fname));
	StringPtr reqnamePascal = c2pstr((char *)tmpname);
	// MW-2005-05-10: Update this call to FM rountines
	ffamilyID = FMGetFontFamilyFromName(reqnamePascal);
	if (ffamilyID == 0)
	{
		r_sizes = MCValueRetain(kMCEmptyList);
		return true;
	}

	MCAutoListRef t_list;
	if (!MCListCreateMutable('\n', &t_list))
		return false;

	bool t_success = true;

	// MW-2013-03-14: [[ Bug 10744 ]] Make sure we return 0 for scalable fonts. I'm
	//   not sure we need do this anymore - I think all fonts are scalable on Mac these
	//   days. I'm also assuming that true-type and postscript fonts are always outline
	//   (which from memory they are - although they can have device-size specific
	//    bitmaps embedded).
	FMFont t_font;
	FourCharCode t_type;
	if (FMGetFontFromFontFamilyInstance(ffamilyID, 0, &t_font, NULL) == noErr &&
		FMGetFontFormat(t_font, &t_type) == noErr &&
		(t_type == kFMTrueTypeFontTechnology || t_type == kFMPostScriptFontTechnology))
		t_success = MCListAppendInteger(*t_list, 0);
	else
	{
		//loop from size 6 to 72 point, query if the size is available
		for (integer_t i = 6 ; t_success && i <= 72; i++)
			if (RealFont(ffamilyID, i) == True)
				t_success = MCListAppendInteger(*t_list, i);

	}
	delete tmpname;

	if (t_success)
		t_success = MCListCopy(*t_list, r_sizes);
	return t_success;
}

bool MCFontlist::getfontstyles(MCStringRef p_fname, uint2 fsize, MCListRef& r_styles)
{
	MCAutoListRef t_list;
	if (!MCListCreateMutable('\n', &t_list))
		return false;

	bool t_success = true;
	t_success = MCListAppend(*t_list, MCN_plain) &&
		MCListAppend(*t_list, MCN_bold) &&
		MCListAppend(*t_list, MCN_italic) &&
		MCListAppend(*t_list, MCN_bold_italic);
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
			return true;
		}
		t_font = t_font->next();
	}
	return false;
}


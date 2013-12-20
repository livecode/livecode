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

MCFontnode::MCFontnode(const MCString &fname, uint2 &size, uint2 style)
{
	reqname = fname.clone();
	reqsize = size;
	reqstyle = style;
	char *tmpname;
	font = new MCFontStruct; //create MCFont structure
	
	// MW-2005-05-10: Update this to FM type
	FMFontFamily ffamilyid;		    //font family ID
	
	tmpname = strclone(reqname);//make a copy of the font name

	char *sptr = tmpname;
	if ((sptr = strchr(tmpname, ',')) != NULL)
	{
		*sptr = '\0';
		sptr++;
	}
	StringPtr reqnamePascal = c2pstr(tmpname);
	
	// MW-2005-05-10: Update this call to FM rountines
	ffamilyid = FMGetFontFamilyFromName(reqnamePascal);
	delete tmpname;
	
	if (ffamilyid == 0)
	{ //font does not exist
		uint2 i;     // check the font mapping table
		for (i = 0 ; i < MAX_XFONT2MACFONT ; i++)
			if (fname == XMfonts[i].Xfontname)
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

	SetGWorld(oldport, olddevice);
}

MCFontnode::~MCFontnode()
{
	delete reqname;
	delete font;
}

MCFontStruct *MCFontnode::getfont(const MCString &fname, uint2 size, uint2 style)
{
	if (fname != reqname)
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

MCFontStruct *MCFontlist::getfont(const MCString &fname, uint2 &size,
                                  uint2 style, Boolean printer)
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

void MCFontlist::getfontnames(MCExecPoint &ep, char *type)
{ //get a list of all the available font in the system
	ep.clear();
	short i = 1;
	unsigned char fname[255]; //hold family font name
	
	FMFontFamilyIterator            fontFamilyIterator;
	FMFontFamily                    fontFamily;
	OSStatus  			    status;

	// Create an iterator to enumerate the font families.
	status = FMCreateFontFamilyIterator(NULL, NULL, kFMDefaultOptions,
										&fontFamilyIterator);
	while ((status = FMGetNextFontFamily(&fontFamilyIterator, &fontFamily)) == noErr)
	{
		if (FMGetFontFamilyName(fontFamily, fname) == noErr)
		{
			p2cstr(fname);
			if (fname[0] != '%' && fname[0] != '.')          //exclude printer fonts
				ep.concatcstring((char *)fname, EC_RETURN, i++ == 1);
		}
	}
	
	FMDisposeFontFamilyIterator (&fontFamilyIterator);
}

void MCFontlist::getfontsizes(const char *fname, MCExecPoint &ep)
{
	ep.clear();
	short i;
	char sizeBuf[U2L];
	FMFontFamily ffamilyID;
	char *tmpname = strclone(fname);
	StringPtr reqnamePascal = c2pstr((char *)tmpname);
	// MW-2005-05-10: Update this call to FM rountines
	ffamilyID = FMGetFontFamilyFromName(reqnamePascal);
	if (ffamilyID == 0)
	{
		ep.clear();
		return;
	}
	
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
		ep . setuint(0);
	else
	{
		//loop from size 6 to 72 point, query if the size is available
		bool first = true;
		for (i = 6 ; i <= 72; i++)
		{
			if (RealFont(ffamilyID, i) == True)
			{
				ep.concatuint(i, EC_RETURN, first);
				first = false;
			}
		}
	}
	delete tmpname;
}

void MCFontlist::getfontstyles(const char *fname, uint2 fsize, MCExecPoint &ep)
{
	ep.setstaticcstring("plain\nbold\nitalic\nbold-italic");
}

bool MCFontlist::getfontstructinfo(const char *&r_name, uint2 &r_size, uint2 &r_style, Boolean &r_printer, MCFontStruct *p_font)
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


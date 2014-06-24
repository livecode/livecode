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
#include "osxprefix-legacy.h"

#include "globdefs.h"
#include "filedefs.h"
#include "parsedef.h"
#include "objdefs.h"

//#include "execpt.h"
#include "util.h"
#include "globals.h"
#include "osspec.h"

#include "osspec.h"
#include "osxflst.h"

extern void *coretext_font_create_with_name_size_and_style(const char *p_name, uint32_t p_size, bool p_bold, bool p_italic);
extern void coretext_font_destroy(void *p_font);
extern void coretext_font_get_metrics(void *p_font, float& r_ascent, float& r_descent);
extern void coretext_get_font_names(MCExecPoint &ep);
extern void core_text_get_font_styles(const char *p_name, uint32_t p_size, MCExecPoint &ep);

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

<<<<<<< HEAD
MCFontnode::MCFontnode(MCNameRef fname, uint2 &size, uint2 style)
{
	reqname = fname;
	reqsize = size;
	reqstyle = style;
	char *tmpname;
	char *temp;
	font = new MCFontStruct; //create MCFont structure
=======
MCFontnode::MCFontnode(const MCString& fname, uint2& size, uint2 style)
{
	reqname = fname . clone();
	reqsize = size;
	reqstyle = style;
>>>>>>> develop
	
	font = new MCFontStruct;
	font -> size = size;
	
<<<<<<< HEAD
	/* UNCHECKED */ MCStringConvertToCString(MCNameGetString(fname), temp);
	tmpname = strclone(temp);//make a copy of the font name

	char *sptr = tmpname;
	if ((sptr = strchr(tmpname, ',')) != NULL)
	{
		*sptr = '\0';
		sptr++;
	}

	StringPtr reqnamePascal = c2pstr(tmpname);
	
	// MW-2005-05-10: Update this call to FM rountines
	ffamilyid = FMGetFontFamilyFromName(reqnamePascal);
	delete temp;
	delete tmpname;
	
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
=======
	char *t_comma;
	t_comma = strchr(reqname, ',');
	if (t_comma != nil)
		*t_comma = '\0';
	
    // MM-2014-06-02: [[ CoreText ]] Updated to use core text fonts.
    font -> fid = (MCSysFontHandle)coretext_font_create_with_name_size_and_style(reqname, reqsize, (reqstyle & FA_WEIGHT) > 0x05, (reqstyle & FA_ITALIC) != 0);
	
    // if font does not exist then find MAC equivalent of X font name
    if (font -> fid == NULL)
	{
		for (uint2 i = 0 ; i < MAX_XFONT2MACFONT ; i++)
			if (fname == XMfonts[i] . Xfontname)
			{
                // MM-2014-06-02: [[ CoreText ]] Updated to use core text fonts.
                font -> fid = (MCSysFontHandle)coretext_font_create_with_name_size_and_style(XMfonts[i] . Macfontname, reqsize, (reqstyle & FA_WEIGHT) > 0x05, (reqstyle & FA_ITALIC) != 0);
>>>>>>> develop
				break;
			}
	}
    
	font -> ascent = size - 1;
	font -> descent = size * 2 / 14 + 1;
	
    // MM-2014-06-02: [[ CoreText ]] Updated to use core text fonts.
	float ascent, descent;
	coretext_font_get_metrics(font -> fid,  ascent, descent);
	if (ceilf(ascent) + ceilf(descent) > size)
		font -> ascent++;
}

MCFontnode::~MCFontnode()
{
<<<<<<< HEAD
=======
    // MM-2014-06-02: [[ CoreText ]] Updated to use core text fonts.
	coretext_font_destroy(font -> fid);
	delete reqname;
>>>>>>> develop
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

<<<<<<< HEAD
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
            TextEncoding t_encoding;
            TextToUnicodeInfo t_info;
            
            FMGetFontFamilyTextEncoding(fontFamily, &t_encoding);
            CreateTextToUnicodeInfoByEncoding(t_encoding, &t_info);
            
            ByteCount t_length;
            unichar_t ufname[255];
            ConvertFromPStringToUnicode(t_info, fname, 255, &t_length, ufname);
            
            DisposeTextToUnicodeInfo(&t_info);

			if (ufname[0] != '%' && ufname[0] != '.')          //exclude printer fonts
            {
                MCAutoStringRef t_name;
                /* UNCHECKED */ MCStringCreateWithChars(ufname, t_length/2, &t_name);
                t_success = MCListAppend(*t_list, *t_name);
            }
		}
	}
	
	FMDisposeFontFamilyIterator (&fontFamilyIterator);

	if (t_success)
		t_success = MCListCopy(*t_list, r_names);
	return t_success;
=======
void MCFontlist::getfontnames(MCExecPoint &ep, char *type)
{
    // MM-2014-06-02: [[ CoreText ]] Updated to use core text routines.
    coretext_get_font_names(ep);
>>>>>>> develop
}

bool MCFontlist::getfontsizes(MCStringRef p_fname, MCListRef& r_sizes)
{
<<<<<<< HEAD
	FMFontFamily ffamilyID;
    char *t_fname;
    /* UNCHECKED */ MCStringConvertToCString(p_fname, t_fname);
	/* UNCHECKED */ char *tmpname = strclone(t_fname);
    delete t_fname;
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
=======
    // MM-2014-06-02: [[ CoreText ]] Assume all core text fonts are scaleable.
	ep . clear();
	ep . concatuint(0, EC_RETURN, true);
>>>>>>> develop
}

bool MCFontlist::getfontstyles(MCStringRef p_fname, uint2 fsize, MCListRef& r_styles)
{
<<<<<<< HEAD
	MCAutoListRef t_list;
	if (!MCListCreateMutable('\n', &t_list))
		return false;

	bool t_success = true;
	t_success = MCListAppend(*t_list, MCN_plain) &&
		MCListAppend(*t_list, MCN_bold) &&
		MCListAppend(*t_list, MCN_italic) &&
		MCListAppend(*t_list, MCN_bold_italic);
	return t_success && MCListCopy(*t_list, r_styles);
=======
    // MM-2014-06-02: [[ CoreText ]] Updated to use core text routines.
    core_text_get_font_styles(fname, fsize, ep);
>>>>>>> develop
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

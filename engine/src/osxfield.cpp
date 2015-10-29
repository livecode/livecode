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

#include "osxprefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "uidc.h"
#include "field.h"
#include "paragraf.h"
#include "cdata.h"
#include "mcerror.h"
//#include "execpt.h"
#include "exec.h"
#include "util.h"
#include "MCBlock.h"

#include "globals.h"

#include "text.h"

bool MCField::macmatchfontname(const char *p_font_name, char p_derived_font_name[])
{
	ATSUFontID t_font_id;
	if (ATSUFindFontFromName(p_font_name, strlen(p_font_name), kFontFullName, kFontNoPlatform, kFontNoScript, kFontNoLanguage, &t_font_id) == noErr ||
		ATSUFindFontFromName(p_font_name, strlen(p_font_name), kFontUniqueName, kFontNoPlatform, kFontNoScript, kFontNoLanguage, &t_font_id) == noErr ||
		ATSUFindFontFromName(p_font_name, strlen(p_font_name), kFontFamilyName, kFontNoPlatform, kFontNoScript, kFontNoLanguage, &t_font_id) == noErr ||
		ATSUFindFontFromName(p_font_name, strlen(p_font_name), kFontNoName, kFontNoPlatform, kFontNoScript, kFontNoLanguage, &t_font_id) == noErr)
	{
		// Fetch the style name
		char t_style_name[64];
		ByteCount t_style_name_length;
		t_style_name_length = 0;
		ATSUFindFontName(t_font_id, kFontStyleName, kFontMacintoshPlatform, kFontNoScript, kFontNoLanguage, 63, t_style_name, &t_style_name_length, NULL);
		t_style_name[t_style_name_length] = '\0';
		
		// Fetch the full name
		char t_full_name[256];
		ByteCount t_full_name_length;
		t_full_name_length = 0;
		ATSUFindFontName(t_font_id, kFontFullName, kFontMacintoshPlatform, kFontNoScript, kFontNoLanguage, 255, t_full_name, &t_full_name_length, NULL);
		t_full_name[t_full_name_length] = '\0';
		
		// MW-2011-09-02: Make sure we don't do anything at all if style is regular
		//   (output name should be fullname!)
		if (MCCStringEqualCaseless(t_style_name, "Regular"))
			p_font_name = p_font_name; // Do nothing
		else if (MCCStringEndsWithCaseless(t_full_name, "Bold Italic"))
			t_full_name[t_full_name_length - 12] = '\0';
		else if (MCCStringEndsWithCaseless(t_full_name, "Bold"))
			t_full_name[t_full_name_length - 5] = '\0';
		else if (MCCStringEndsWithCaseless(t_full_name, "Italic"))
			t_full_name[t_full_name_length - 7] = '\0';
		
		strcpy(p_derived_font_name, t_full_name);
		
		return true;
	}
	
	return false;
}



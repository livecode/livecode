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

#include <map>
#include <string>

#include <Carbon/Carbon.h>

#include "revfont.h"

std::map<std::string, ATSFontContainerRef> s_font_map;

bool path_to_fsspec(const char *p_filename, FSSpec* r_fsspec)
{
	bool t_success;
	t_success = true;
	
	FSRef t_ref;
	if (t_success)
	{
		OSStatus t_status;
		t_status = FSPathMakeRef((const UInt8 *)p_filename, &t_ref, NULL);
		if (t_status != noErr)
			t_success = false;
	}
	
	if (t_success)
	{
		OSErr t_error;
		t_error = FSGetCatalogInfo(&t_ref, kFSCatInfoNone, NULL, NULL, r_fsspec, NULL);
		if (t_error != noErr)
			t_success = false;
	}
	
	return t_success;
}

FontLoadStatus FontLoad(const char *p_filename)
{
	FontLoadStatus t_status;
	t_status = kFontLoadStatusSuccess;

	FSSpec t_fsspec;
	if (t_status == kFontLoadStatusSuccess)
	{
		if (!path_to_fsspec(p_filename, &t_fsspec))
			t_status = kFontLoadStatusNotFound;
	}

	ATSFontContainerRef t_container;
	t_container = NULL;
	if (t_status == kFontLoadStatusSuccess)
	{
		OSStatus t_os_status;
		t_os_status = ATSFontActivateFromFileSpecification(&t_fsspec, kATSFontContextLocal, kATSFontFormatUnspecified, NULL, kATSOptionFlagsDefault, &t_container);
		if (t_os_status != noErr)
			t_status = kFontLoadStatusBadFont;
	}

	if (t_status == kFontLoadStatusSuccess)
		s_font_map . insert(make_pair(std::string(p_filename), t_container));

	return t_status;
}

bool FontUnload(const char *p_filename)
{
	bool t_success;
	t_success = true;

	ATSFontContainerRef t_container;
	t_container = NULL;
	if (t_success)
	{
		t_container = s_font_map[p_filename];
		if (t_container == NULL)
			t_success = false;
	}

	if (t_success)
	{
		OSStatus t_status;
		t_status = ATSFontDeactivate(t_container, NULL, kATSOptionFlagsDefault);
		s_font_map . erase(p_filename);
	}

	return t_success;
}

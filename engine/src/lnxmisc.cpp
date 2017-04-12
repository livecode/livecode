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

#include "lnxprefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "globals.h"
#include "stacklst.h"
#include "stack.h"
#include "text.h"

#include "lnxdc.h"
#include "lnxgtkthemedrawing.h"
#include "lnximagecache.h"
#include "lnxtheme.h"

////////////////////////////////////////////////////////////////////////////////
//
//  REFACTORED FROM STACKLST.CPP
//

void MCStacklist::hidepalettes(Boolean hide)
{
	active = !hide;
	if (stacks == NULL)
		return;
	MCStacknode *tptr = stacks;

	// only hide palettes if a non-palette is open
	Boolean dohide = False;
	do
	{
		MCStack *sptr = tptr->getstack();
		if (sptr->getrealmode() < WM_PALETTE)
		{
			dohide = True;
			break;
		}
		tptr = tptr->next();
	}
	while (tptr != stacks);
	if (!dohide)
		return;

	restart = False;
	tptr = stacks;
	do
	{
		MCStack *sptr = tptr->getstack();
		if (sptr->getrealmode() == WM_PALETTE && sptr->getflag(F_VISIBLE))
			if (MChidepalettes)
			{
				if (hide)
					MCscreen->closewindow(sptr->getw());
				else
					sptr -> openwindow(False);
			}

		tptr = tptr->next();
	}
	while (tptr != stacks);
}

void MCLinuxWindowSetTransientFor(GdkWindow* p_window, GdkWindow* p_transient_for)
{
	gdk_window_set_transient_for(p_window, p_transient_for);
}

////////////////////////////////////////////////////////////////////////////////
//
//  REFACTORED FROM TEXT.CPP
//
//
//#include <iconv.h>
//
//struct ConverterRecord
//{
//	ConverterRecord *next;
//	const char *encoding;
//	iconv_t converter;
//};
//
//static iconv_t fetch_converter(const char *p_encoding)
//{
//	static ConverterRecord *s_records = NULL;
//
//	ConverterRecord *t_previous, *t_current;
//	for(t_previous = NULL, t_current = s_records; t_current != NULL; t_previous = t_current, t_current = t_current -> next)
//		if (t_current -> encoding == p_encoding)
//			break;
//
//	if (t_current == NULL)
//	{
//		iconv_t t_converter;
//		t_converter = iconv_open("UTF-16", p_encoding);
//
//		ConverterRecord *t_record;
//		t_record = new (nothrow) ConverterRecord;
//		t_record -> next = s_records;
//		t_record -> encoding = p_encoding;
//		t_record -> converter = t_converter;
//		s_records = t_record;
//
//		return t_record -> converter;
//	}
//
//	if (t_previous != NULL)
//	{
//		t_previous -> next = t_current -> next;
//		t_current -> next = s_records;
//		s_records = t_current;
//	}
//
//	return s_records -> converter;
//}
//
//bool MCSTextConvertToUnicode(MCTextEncoding p_input_encoding, const void *p_input, uint4 p_input_length, void *p_output, uint4 p_output_length, uint4& r_used)
//{
//	if (p_input_length == 0)
//	{
//		r_used = 0;
//		return true;
//	}
//
//	if (p_output_length == 0)
//	{
//		r_used = p_input_length * 4;
//		return false;
//	}
//
//	const char *t_encoding;
//	t_encoding = NULL;
//
//	if (p_input_encoding >= kMCTextEncodingWindowsNative)
//	{
//		struct { uint4 codepage; const char *encoding; } s_codepage_map[] =
//		{
//			{437, "CP437" },
//			{850, "CP850" },
//			{932, "CP932" },
//			{949, "CP949" },
//			{1361, "CP1361" },
//			{936, "CP936" },
//			{950, "CP950" },
//			{1253, "WINDOWS-1253" },
//			{1254, "WINDOWS-1254" },
//			{1258, "WINDOWS-1258" },
//			{1255, "WINDOWS-1255" },
//			{1256, "WINDOWS-1256" },
//			{1257, "WINDOWS-1257" },
//			{1251, "WINDOWS-1251" },
//			{874, "CP874" },
//			{1250, "WINDOWS-1250" },
//			{1252, "WINDOWS-1252" },
//			{10000, "MACINTOSH" }
//		};
//
//		for(uint4 i = 0; i < sizeof(s_codepage_map) / sizeof(s_codepage_map[0]); ++i)
//			if (s_codepage_map[i] . codepage == p_input_encoding - kMCTextEncodingWindowsNative)
//			{
//				t_encoding = s_codepage_map[i] . encoding;
//				break;
//			}
//
//	}
//	else if (p_input_encoding >= kMCTextEncodingMacNative)
//		t_encoding = "MACINTOSH";
//
//	iconv_t t_converter;
//	t_converter = fetch_converter(t_encoding);
//
//	if (t_converter == NULL)
//	{
//		r_used = 0;
//		return true;
//	}
//
//	char *t_in_bytes;
//	char *t_out_bytes;
//	size_t t_in_bytes_left;
//	size_t t_out_bytes_left;
//
//	t_in_bytes = (char *)p_input;
//	t_in_bytes_left = p_input_length;
//	t_out_bytes = (char *)p_output;
//	t_out_bytes_left = p_output_length;
//
//	iconv(t_converter, NULL, NULL, &t_out_bytes, &t_out_bytes_left);
//
//	if (iconv(t_converter, &t_in_bytes, &t_in_bytes_left, &t_out_bytes, &t_out_bytes_left) == (size_t)-1)
//	{
//		r_used = 4 * p_input_length;
//		return false;
//	}
//
//	r_used = p_output_length - t_out_bytes_left;
//
//	return true;
//}

////////////////////////////////////////////////////////////////////////////////
//
//  REFACTORED FROM GLOBALS.CPP
//

MCUIDC *MCCreateScreenDC(void)
{
	return new MCScreenDC;
}

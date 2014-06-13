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

#include "osxprefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "globals.h"
#include "stacklst.h"
#include "stack.h"
#include "text.h"
#include "button.h"
#include "hc.h"
#include "osspec.h"
#include "util.h"
#include "mode.h"

#include "osxtheme.h"

#include "resolution.h"

////////////////////////////////////////////////////////////////////////////////
//
//  REFACTORED FROM TEXT.CPP
//

struct UnicodeInfoRecord
{
	UnicodeInfoRecord *next;
	TextEncoding encoding;
	TextToUnicodeInfo info;
};

static TextToUnicodeInfo fetch_unicode_info(TextEncoding p_encoding)
{
	static UnicodeInfoRecord *s_records = NULL;
	
	UnicodeInfoRecord *t_previous, *t_current;
	for(t_previous = NULL, t_current = s_records; t_current != NULL; t_previous = t_current, t_current = t_current -> next)
		if (t_current -> encoding == p_encoding)
			break;
			
	if (t_current == NULL)
	{
		UnicodeMapping t_mapping;
		t_mapping . unicodeEncoding = CreateTextEncoding(kTextEncodingUnicodeDefault, kUnicodeNoSubset, kUnicode16BitFormat);
		t_mapping . otherEncoding = CreateTextEncoding(p_encoding, kTextEncodingDefaultVariant, kTextEncodingDefaultFormat);
		t_mapping . mappingVersion = kUnicodeUseLatestMapping;
		
		TextToUnicodeInfo t_info;
		OSErr t_err;
		t_err = CreateTextToUnicodeInfo(&t_mapping, &t_info);
		if (t_err != noErr)
			t_info = NULL;
		
		UnicodeInfoRecord *t_record;
		t_record = new UnicodeInfoRecord;
		t_record -> next = s_records;
		t_record -> encoding = p_encoding;
		t_record -> info = t_info;
		s_records = t_record;
		
		return t_record -> info;
	}
	
	if (t_previous != NULL)
	{
		t_previous -> next = t_current -> next;
		t_current -> next = s_records;
		s_records = t_current;
	}
	
	return s_records -> info;
}

bool MCSTextConvertToUnicode(MCTextEncoding p_input_encoding, const void *p_input, uint4 p_input_length, void *p_output, uint4 p_output_length, uint4& r_used)
{
	if (p_input_length == 0)
	{
		r_used = 0;
		return true;
	}

	int4 t_encoding;
	t_encoding = -1;
	
	if (p_input_encoding >= kMCTextEncodingWindowsNative)
	{
		struct { uint4 codepage; int4 encoding; } s_codepage_map[] =
		{
			{437, kTextEncodingDOSLatinUS },
			{850, kTextEncodingDOSLatinUS },
			{932, kTextEncodingDOSJapanese },
			{949, kTextEncodingDOSKorean },
			{1361, kTextEncodingWindowsKoreanJohab },
			{936, kTextEncodingDOSChineseSimplif },
			{950, kTextEncodingDOSChineseTrad },
			{1253, kTextEncodingWindowsGreek },
			{1254, kTextEncodingWindowsLatin5 },
			{1258, kTextEncodingWindowsVietnamese },
			{1255, kTextEncodingWindowsHebrew },
			{1256, kTextEncodingWindowsArabic },
			{1257, kTextEncodingWindowsBalticRim },
			{1251, kTextEncodingWindowsCyrillic },
			{874, kTextEncodingDOSThai },
			{1250, kTextEncodingWindowsLatin2 },
			{1252, kTextEncodingWindowsLatin1 }
		};
		
		for(uint4 i = 0; i < sizeof(s_codepage_map) / sizeof(s_codepage_map[0]); ++i)
			if (s_codepage_map[i] . codepage == p_input_encoding - kMCTextEncodingWindowsNative)
			{
				t_encoding = s_codepage_map[i] . encoding;
				break;
			}
			
		// MW-2008-03-24: [[ Bug 6187 ]] RTF parser doesn't like ansicpg1000
		if (t_encoding == -1 && (p_input_encoding - kMCTextEncodingWindowsNative >= 10000))
			t_encoding = p_input_encoding - kMCTextEncodingWindowsNative - 10000;
			
	}
	else if (p_input_encoding >= kMCTextEncodingMacNative)
		t_encoding = p_input_encoding - kMCTextEncodingMacNative;
	
	TextToUnicodeInfo t_info;
	t_info = fetch_unicode_info(t_encoding);
	
	if (t_info == NULL)
	{
		r_used = 0;
		return true;
	}
	
	ByteCount t_source_read, t_unicode_length;
	if (ConvertFromTextToUnicode(t_info, p_input_length, p_input, 0, 0, (ByteOffset *)NULL, (ItemCount *)NULL, NULL, p_output_length, &t_source_read, &t_unicode_length, (UniChar *)p_output) != noErr)
	{
		r_used = 4 * p_input_length;
		return false;
	}

	r_used = t_unicode_length;
	
	return true;
}

////////////////////////////////////////////////////////////////////////////////
//
//  REFACTORED FROM STACKE.CPP
//


void MCMacDisableScreenUpdates(void)
{
	DisableScreenUpdates();
}

void MCMacEnableScreenUpdates(void)
{
	EnableScreenUpdates();
}

////////////////////////////////////////////////////////////////////////////////
//
//  REFACTORED FROM CMDS.CPP 
//

char *MCSystemLowercaseInternational(const MCString& p_string)
{
	char *t_lc_string;
	t_lc_string = p_string . clone();
	LowercaseText(t_lc_string, p_string . getlength(), smSystemScript);
	return t_lc_string;
}

int MCSystemCompareInternational(const char *p_left, const char *p_right)
{
	return CompareText(p_left, p_right, strlen(p_left), strlen(p_right), NULL);
}

////////////////////////////////////////////////////////////////////////////////
//
//  REFACTORED FROM HC.CPP 
//

IO_stat MCHcstak::macreadresources(void)
{		//on MAC, read resources in MAC stack directly, by opening stack's resource fork
	short resFileRefNum;
	
	if (MCS_openresourcefile_with_path(name, fsRdPerm, false, &resFileRefNum) != NULL)
		return IO_NORMAL;
	
	short rtypeCount = Count1Types(); //get the # of total resource types in current res file
	for (int i = 1; i <= rtypeCount; i++)
	{
		ResType rtype;   //resource type
		Get1IndType(&rtype, i); //get each resource type
		if (rtype == 'ICON' || rtype == 'CURS' || rtype == 'snd ')
		{ //only want Icons, Cursors and Sound resources
			short resCount = Count1Resources(rtype); //get the # of resources of the specific type
			short j;
			Handle hres; //resource handle
			for (j=1; j <= resCount; j++)
			{ //loop through each resource of specific type
				hres = Get1IndResource(rtype, j);
				if (hres == NULL)
					continue;
				uint2 id;
				Str255 resname;
				GetResInfo(hres, (short *)&id, &rtype, resname);
				p2cstr(resname); //convert pascal string to C string
				HLock(hres);
				if (rtype == 'ICON')
				{
					MCHcbmap *newicon = new MCHcbmap;
					newicon->appendto(icons);
					newicon->icon(id, strclone((char *)resname), *hres);
				}
				else if (rtype == 'CURS')
				{
					MCHcbmap *newcurs = new MCHcbmap;
					newcurs->appendto(cursors);
					newcurs->cursor(id, strclone((char*)resname), *hres);
				}
				else if (rtype == 'snd ')
				{
					MCHcsnd *newsnd = new MCHcsnd;
					if (newsnd->import(id, strclone((char*)resname), *hres))
						newsnd->appendto(snds);
						else
							delete newsnd;
				}
				HUnlock(hres);
			} //for
		} //if (rtype =='ICON'
	}
	MCS_closeresourcefile(resFileRefNum);
	
	return IO_NORMAL;
}

////////////////////////////////////////////////////////////////////////////////
//
//  REFACTORED FROM ITRANSFORM.CPP 
//

#if 0
void MCMacScaleImageBox(void *p_src_ptr, uint4 p_src_stride, void *p_dst_ptr, uint4 p_dst_stride, uint4 p_src_width, uint4 p_src_height, uint4 p_dst_width, uint4 p_dst_height)
{
	Rect t_src_rect;
	SetRect(&t_src_rect, 0, 0, p_src_width, p_src_height);
	
	GWorldPtr t_src_gworld;
	NewGWorldFromPtr(&t_src_gworld, 32, &t_src_rect, NULL, NULL, MCmajorosversion >= 0x1040 ? kNativeEndianPixMap : 0, (Ptr)p_src_ptr, p_src_stride);
	
	PixMapHandle t_src_pixmap;
	t_src_pixmap = GetGWorldPixMap(t_src_gworld);
	
	Rect t_dst_rect;
	SetRect(&t_dst_rect, 0, 0, p_dst_width, p_dst_height);
	
	GWorldPtr t_dst_gworld;
	NewGWorldFromPtr(&t_dst_gworld, 32, &t_dst_rect, NULL, NULL, MCmajorosversion >= 0x1040 ? kNativeEndianPixMap : 0, (Ptr)p_dst_ptr, p_dst_stride);
	
	PixMapHandle t_dst_pixmap;
	t_dst_pixmap = GetGWorldPixMap(t_dst_gworld);
	
	GWorldPtr t_old_gworld;
	GDHandle t_old_device;
	GetGWorld(&t_old_gworld, &t_old_device);
	SetGWorld(t_dst_gworld, NULL);
	ForeColor(blackColor);
	BackColor(whiteColor);
	LockPixels(t_src_pixmap);
	LockPixels(t_dst_pixmap);
	CopyBits(GetPortBitMapForCopyBits(t_src_gworld), GetPortBitMapForCopyBits(t_dst_gworld), &t_src_rect, &t_dst_rect, srcCopy, NULL);
	UnlockPixels(t_dst_pixmap);
	UnlockPixels(t_src_pixmap);
	
	SetGWorld(t_old_gworld, t_old_device);
	
	DisposeGWorld(t_dst_gworld);
	DisposeGWorld(t_src_gworld);
}
#endif

////////////////////////////////////////////////////////////////////////////////
//
//  MISC 
//

bool MCMacThemeGetBackgroundPattern(Window_mode p_mode, bool p_active, MCPatternRef &r_pattern)
{
    return false;
#if 0
	bool t_success = true;
	
	static MCPatternRef s_patterns[8] = {nil, nil, nil, nil, nil, nil, nil, nil};
	
	ThemeBrush t_themebrush = 0;
	uint32_t t_index = 0;
	
	switch (p_mode)
	{
		case WM_TOP_LEVEL:
		case WM_TOP_LEVEL_LOCKED:
			t_themebrush = kThemeBrushDocumentWindowBackground;
			t_index = 0;
			break;
			
		case WM_MODELESS:
			if (p_active)
			{
				t_themebrush = kThemeBrushModelessDialogBackgroundActive;
				t_index = 1;
			}
			else
			{
				t_themebrush = kThemeBrushModelessDialogBackgroundInactive;
				t_index = 2;
			}
			break;
			
		case WM_PALETTE:
			if (p_active)
			{
				t_themebrush = kThemeBrushUtilityWindowBackgroundActive;
				t_index = 3;
			}
			else
			{
				t_themebrush = kThemeBrushUtilityWindowBackgroundInactive;
				t_index = 4;
			}
			break;
			
		case WM_DRAWER:
			t_themebrush = kThemeBrushDrawerBackground;
			t_index = 5;
			break;
			
		case WM_MODAL:
		case WM_SHEET:
		default:
			if (p_active)
			{
				t_themebrush = kThemeBrushDialogBackgroundActive;
				t_index = 6;
			}
			else
			{
				t_themebrush = kThemeBrushDialogBackgroundInactive;
				t_index = 7;
			}
			break;
	}
	
	if (s_patterns[t_index] != nil)
	{
		r_pattern = s_patterns[t_index];
		return true;
	}
	
	CGrafPtr t_gworld = nil;
	PixMapHandle t_pixmap = nil;
	
	Rect t_bounds;
	SetRect(&t_bounds, 0, 0, 64, 64);
	
	NewGWorld(&t_gworld, 32, &t_bounds, nil, nil, MCmajorosversion >= 0x1040 ? kNativeEndianPixMap : 0);

	CGrafPtr t_oldport;
	GDHandle t_olddevice;
	GetGWorld(&t_oldport, &t_olddevice);
	
	SetGWorld(t_gworld, NULL);
	t_pixmap = GetGWorldPixMap(t_gworld);
	LockPixels(t_pixmap);
	
	SetThemeBackground(t_themebrush, 32, True);
	EraseRect(&t_bounds);
	UnlockPixels(t_pixmap);
	
	SetGWorld(t_oldport, t_olddevice);
	
	void *t_bits = nil;
	uint32_t t_stride = 0;
	
	t_bits = GetPixBaseAddr(t_pixmap);
	t_stride = GetPixRowBytes(t_pixmap);
	
	MCGImageRef t_image;
	t_image = nil;
	
	MCGRaster t_raster;
	t_raster.width = t_bounds.right - t_bounds.left;
	t_raster.height = t_bounds.bottom - t_bounds.top;
	t_raster.pixels = t_bits;
	t_raster.stride = t_stride;
	t_raster.format = kMCGRasterFormat_ARGB;
	
	// IM-2014-05-14: [[ HiResPatterns ]] MCPatternCreate refactored to work with MCGRaster
	// IM-2013-08-14: [[ ResIndependence ]] create MCPattern wrapper
	if (t_success)
		t_success = MCPatternCreate(t_raster, 1.0, kMCGImageFilterNone, r_pattern);

	if (t_success)
		s_patterns[t_index] = r_pattern;
	
	DisposeGWorld(t_gworld);
	
	return t_success;
#endif
}



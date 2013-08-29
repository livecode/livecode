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
#include "contextscalewrapper.h"
#include "text.h"
#include "button.h"
#include "hc.h"
#include "osspec.h"
#include "util.h"
#include "mode.h"

#include "osxdc.h"
#include "osxcontext.h"
#include "osxtheme.h"

////////////////////////////////////////////////////////////////////////////////
//
//  REFACTORED FROM WIDGET.CPP
//

void *MCWidgetContextLockNative(MCContext *p_context)
{
	return static_cast<MCQuickDrawContext *>(p_context) -> qd_get_port();
}

void MCWidgetContextUnlockNative(MCContext *p_context)
{
}

void MCWidgetContextBeginOffscreen(MCContext *p_context, void*& r_dc)
{
}

void MCWidgetContextEndOffscreen(MCContext *p_context, void* p_dc)
{
}

void MCWidgetInvalidateRect(Window window, const MCRectangle& rect)
{
}

////////////////////////////////////////////////////////////////////////////////
//
//  REFACTORED FROM STACKLST.CPP
//

extern void dohilitewindow(WindowRef p_window, Boolean p_hilite);

extern bool WindowIsInControlGroup(WindowRef p_window);

void MCStacklist::ensureinputfocus(Window window)
{
	MCStacknode *t_node = stacks;
	if (t_node == NULL)
		return;
	
	bool t_browsers_found;
	t_browsers_found = false;
	
	do
	{
		Window t_handle;
		t_handle = t_node -> getstack() -> getwindow();
		if (t_handle != NULL)
		{
			if (WindowIsInControlGroup((WindowPtr)t_handle -> handle . window))
			{
				ItemCount t_count;
				t_count = CountWindowGroupContents(GetWindowGroup((WindowPtr)t_handle -> handle . window), kWindowGroupContentsReturnWindows);
				WindowRef *t_windows;
				t_windows = new WindowRef[t_count];
				
				ItemCount t_actual_count;
				t_actual_count = 0;
				GetWindowGroupContents(GetWindowGroup((WindowPtr)t_handle -> handle . window), kWindowGroupContentsReturnWindows, t_count, &t_actual_count, (void **)t_windows);
				for(int t_index = 0; t_index < t_actual_count; ++t_index)
					if (t_windows[t_index] != (WindowPtr)t_handle -> handle . window)
					{
						ClearKeyboardFocus(t_windows[t_index]);
						t_browsers_found = true;
					}
				
				delete t_windows;
			}
		}
		t_node = t_node -> next();
	}
	while(t_node != stacks);
	
	if (t_browsers_found && window != NULL)
		SetUserFocusWindow((WindowPtr)window -> handle . window);
}

void MCStacklist::hidepalettes(Boolean hide)
{
	active = !hide;
	if (stacks == NULL)
		return;
	MCStacknode *tptr = stacks;

	restart = False;
	tptr = stacks;
	do
	{
		MCStack *sptr = tptr->getstack();
		if (sptr->getrealmode() == WM_PALETTE && sptr->getflag(F_VISIBLE))
			if (MChidepalettes)
			{
				WindowClass wclass;
				GetWindowClass((WindowPtr)sptr->getw()->handle.window, &wclass );
				if (wclass != kUtilityWindowClass)
				{
                    // MM-2012-04-02: Use new MC*Window wrapper function - fixes bugs where a cocoa NSWindow has been
                    //  created from the carbon WindowRef.
					extern void MCShowHideWindow(void *, bool);
					MCShowHideWindow(sptr->getw()->handle.window, !hide);
					if (!hide)
						dohilitewindow((WindowPtr)sptr->getw()->handle.window, True);
				}
			}
		tptr = tptr->next();
	}
	while (tptr != stacks);
}

////////////////////////////////////////////////////////////////////////////////
//
//  REFACTORED FROM CONTEXTSCALEWRAPPER.CPP
//

extern CGImageRef PixMapToCGImage(PixMapHandle p_pixmap, bool p_grayscale, bool p_with_alpha = false);
void MCContextScaleWrapper::drawtheme(MCThemeDrawType p_type, MCThemeDrawInfo* p_parameters)
{
		// MW-2009-06-14: Previously this was attempting to extract the clip region from
	//   the theme parameters. Unfortunately, it seems that these are not 'correct'
	//   (well, correct enough for clipping). Instead we just temporary create a context
	//   that covers the size of the unscaled clip and use that. (We throw it again
	//   immediately after using, so its not very much less efficient).
	MCRectangle t_bounds;
	t_bounds = getclip();

	MCContext *t_context = MCscreen->creatememorycontext(t_bounds.width, t_bounds.height, true, true);
	t_context->setorigin(t_bounds.x, t_bounds.y);
	t_context->setclip(t_bounds);
	t_context->drawtheme(p_type, p_parameters);
	
	CGImageRef t_image = PixMapToCGImage(((MCQuickDrawContext*)t_context)->qd_get_pixmap(), false, true);
	
	CGContextRef ctx = ((MCQuickDrawContext*)m_context)->lock_as_cg();
	if (ctx)
	{
		CGRect t_rect = CGRectMake(t_bounds.x * scale, t_bounds.y * scale, t_bounds.width * scale, t_bounds.height * scale);
		CGContextClipToRect(ctx, t_rect);
		CGContextDrawImage(ctx, t_rect, t_image);
		((MCQuickDrawContext*)m_context)->unlock_as_cg(ctx);
	}
	
	CGImageRelease(t_image);
	MCscreen->freecontext(t_context);
}

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

bool MCMacIsWindowVisible(Window window)
{
	return IsWindowVisible((WindowPtr)window -> handle . window);
}

////////////////////////////////////////////////////////////////////////////////
//
//  REFACTORED FROM BUTTON.CPP
//

bool MCMacGetMenuItemTag(MenuHandle mh, uint2 mitem, MCStringRef &r_string)
{
	UInt32 t_propsize;
	OSStatus t_result = GetMenuItemPropertySize(mh, mitem, 'RRev', 'MTag', &t_propsize);
	if (t_result == noErr)
	{
		char *t_tag = new char[t_propsize + 1];
		t_result = GetMenuItemProperty(mh, mitem, 'RRev', 'MTag', t_propsize, NULL, t_tag);
		t_tag[t_propsize] = '\0';
		if (MCStringCreateWithCStringAndRelease((char_t*)t_tag, r_string))
			return true;
		
		delete[] t_tag;
		return false;
	}
	else
		return false;
}

static void getmacmenuitemtext(MenuHandle mh, uint2 mitem, MCStringRef &r_string, Boolean issubmenu)
{
	//find menu root, to get tags property
	bool t_menuhastags;
	
	MenuID t_menu = GetMenuID(mh);
	if (issubmenu)
		// Get main menu id of submenu, stored as a property of the menu
		GetMenuItemProperty(mh, 0, 'RRev', 'MMID', sizeof(t_menu), NULL, &t_menu);
	GetMenuItemProperty(GetMenuHandle(t_menu), 0, 'RRev', 'Tags', sizeof(t_menuhastags), NULL, &t_menuhastags);
	
	//isunicode &= !t_menuhastags;
	char *newmenu = NULL;

	MCStringRef t_menuitem = nil;
	MCStringRef t_tagstr = nil;
	if (MCMacGetMenuItemTag(mh, mitem, t_tagstr))
	{
		// Pass the reference on
		t_menuitem = t_tagstr;
		t_tagstr = nil;
	}
	else
	{
		CFStringRef cfmenuitemstr;
		CopyMenuItemTextAsCFString(mh, mitem,&cfmenuitemstr);
		/* UNCHECKED */ MCStringCreateWithCFString(cfmenuitemstr, t_menuitem);
		CFRelease(cfmenuitemstr);
	}
	
	MCStringRef t_menupick = nil;
	if (issubmenu)
	{
		CFStringRef cftitlestr;
		MCStringRef t_titlestr = nil;
		CopyMenuTitleAsCFString(mh, &cftitlestr);
		/* UNCHECKED */ MCStringCreateWithCFString(cftitlestr, t_titlestr);
		CFRelease(t_menupick);
		
		/* UNCHECKED */ MCStringCreateMutable(0, t_menupick);
		/* UNCHECKED */ MCStringAppend(t_menupick, t_titlestr);
		/* UNCHECKED */ MCStringAppend(t_menupick, t_menuitem);
		MCValueRelease(t_titlestr);

	}
	MCValueRelease(t_menuitem);
	/* UNCHECKED */ MCStringCopyAndRelease(t_menupick, r_string);
}

//static void getmacmenuitemtextfromaccelerator(MenuHandle menu, uint2 key, uint1 mods, MCString &s, bool isunicode, bool issubmenu)
static void getmacmenuitemtextfromaccelerator(MenuHandle menu, uint2 key, uint1 mods, MCStringRef &r_string, bool issubmenu)
{
	uint2 itemcount = CountMenuItems(menu);
	for (uint2 i = 1; i <= itemcount; i++)
	{
		MenuRef submenu;
		GetMenuItemHierarchicalMenu(menu, i, &submenu);
		if (submenu != NULL)
		{
			getmacmenuitemtextfromaccelerator(submenu, key, mods, r_string, true);
			if (!MCStringIsEmpty(r_string))
				return;
		}
		else
		{
			uint2 t_key = 0;
			GetMenuItemCommandKey(menu, i, false, &t_key);
			int2 t_glyph;
			GetMenuItemKeyGlyph(menu, i, &t_glyph);
			if (t_glyph != kMenuNullGlyph)
				t_key = MCMacGlyphToKeysym(t_glyph);
			if (t_key == key)
			{
				uint1 t_macmods = 0;
				GetMenuItemModifiers(menu, i, &t_macmods);
				uint1 t_mods = 0;
				if (t_macmods & kMenuShiftModifier)
					t_mods |= MS_SHIFT;
				if (t_macmods & kMenuOptionModifier)
					t_mods |= MS_ALT;
				if (t_macmods & kMenuControlModifier)
					t_mods |= MS_MAC_CONTROL;
				if (!(t_macmods & kMenuNoCommandModifier))
					t_mods |= MS_CONTROL;
				
				if (t_mods == mods)
				{
					getmacmenuitemtext(menu, i, r_string, issubmenu);
					return;
				}
			}
		}
	}
}


Bool MCButton::macfindmenu(bool p_just_for_accel)
{
	// MW-2011-02-08: [[ Bug 9384 ]] Only create the menu if we are using it next. 'findmenu'
	//   is often called just to keep accelerators in sync.
	if (bMenuID == 0 && !p_just_for_accel)
	{
		MCScreenDC *pms = (MCScreenDC *)MCscreen;
		//call MCScreenDC class routine to get an available
		//menu id.  Only the Application's Main menus do not
		//need to get usable id from this routine Any button
		//based menus, such as pop-up, options menus are
		//restricted to the available id to avoid duplication
		//of the meun ids.
		int2 newMenuID = pms->allocateSubMenuID(False);
		if (newMenuID == 0)
			return False;
		
		MCStringRef t_menu_name = getlabeltext();
		
		// MW-2012-02-17: Update to use CreateNewMenu / CFStringCreateWithBytes since
		//   we don't have a valid charset of font any more.
		MenuHandle mh;
		mh = nil;
		CreateNewMenu(newMenuID, 0, &mh);
		if (mh != nil)
		{
			CFStringRef t_menu_title;
			/* UNCHECKED */ MCStringConvertToCFStringRef(t_menu_name, t_menu_title);
			SetMenuTitleWithCFString(mh, t_menu_title);
			
			MCString s;
			getmenustring(s);
			if (pms->addMenuItemsAndSubMenu(newMenuID, mh, this, menumode))
			{
				bMenuID = newMenuID;
				InsertMenu(mh, -1); //-1 = insert sub-menu or popup
			}
		}
		else
		{ // delete the id only, since we can't create a menu.
			pms->freeMenuAndID(newMenuID);
			return False;
		}
	}
	
	return True;
}

void MCButton::macopenmenu(void)
{
	MenuHandle mh = GetMenuHandle(bMenuID);
	if (mh == NULL)
		return;   //can't find the menu to display, bail out
	MCRectangle trect;
	long result;
	MCScreenDC *pms = (MCScreenDC *)MCscreen;
	
	int4 tmenux,tmenuy;
	tmenux = tmenuy = 0;
	switch (menumode)
	{
		case WM_COMBO:
		case WM_OPTION:
			trect = MCU_recttoroot(MCmousestackptr, rect);
			tmenux = trect.x;
			tmenuy = trect.y;
			break;
		case WM_PULLDOWN:
			trect = MCU_recttoroot(MCmousestackptr, rect);
			tmenux = trect.x;
			tmenuy = trect.y+trect.height + 1;
			break;
		case WM_CASCADE:
			trect = MCU_recttoroot(MCmousestackptr, rect);
			tmenux = trect.x + trect.width + 1;
			tmenuy = trect.y;
			break;
		case WM_POPUP:
		default:
			trect.x = MCmousex + 1;
			trect.y = MCmousey + 1;
			trect = MCU_recttoroot(MCmousestackptr, trect);
			tmenux = trect.x;
			tmenuy = trect.y;
			break;
	}
	
	// MW-2007-12-11: [[ Bug 5670 ]] Make sure we notify things higher up in the call chain
	//   that the mdown actually did result in a menu being popped up!
	extern bool MCosxmenupoppedup;
	MCosxmenupoppedup = true;
	
	switch (menumode)
	{
		case WM_COMBO:
		case WM_OPTION:
			//= MAC's pop-up menu, displayed at the control/button location
			layer_redrawall();
			if (MCModeMakeLocalWindows())
				result = PopUpMenuSelect(mh, tmenuy, tmenux, menuhistory);
			else
				result = MCModePopUpMenu((MCMacSysMenuHandle)mh, tmenux - trect . x + rect . x, tmenuy - trect . y + rect . y, menuhistory, MCmousestackptr);
			allowmessages(False);
			pms->clearmdown(menubutton);
			allowmessages(True);
			if (result > 0)
			{
				setmenuhistoryprop(LoWord(result));
				//low word of result from PopUpMenuSelect() is the item selected
				//high word contains the menu id
				
				MCStringRef t_label = nil;
				getmacmenuitemtext(mh, menuhistory, t_label, False);
				MCValueAssign(label, t_label);
				MCValueRelease(t_label);
				Exec_stat es = message_with_valueref_args(MCM_menu_pick, label);
				if (es == ES_NOT_HANDLED || es == ES_PASS)
					message_with_args(MCM_mouse_up, menubutton);
			}
			else
				message_with_args(MCM_mouse_release, menubutton);
			state &= ~(CS_MFOCUSED | CS_ARMED | CS_HILITED);
			layer_redrawall();
			break;
		case WM_PULLDOWN:
		case WM_CASCADE:
		case WM_POPUP: //= MAC's context menu, Menu displyed at the mouse loc
			if (MCModeMakeLocalWindows())
				result = PopUpMenuSelect(mh, tmenuy, tmenux, 0);
			else
			{
				int32_t x, y;
				if (menumode == WM_POPUP)
				{
					x = MCmousex + 1;
					y = MCmousey + 1;
				}
				else
				{
					x = tmenux - trect . x + rect . x;
					y = tmenuy - trect . y + rect . y;
				}
				
				result = MCModePopUpMenu((MCMacSysMenuHandle)mh, x, y, 0, MCmousestackptr);
			}
			allowmessages(False);
			pms->clearmdown(menubutton);
			allowmessages(True);
			if (result > 0)
			{ //user selected something
				MenuHandle mhandle = GetMenuHandle(HiWord(result));
				setmenuhistoryprop(LoWord(result));
				MCStringRef t_menu_string = nil;
				getmacmenuitemtext(mhandle, menuhistory, t_menu_string, mhandle != mh);
				Exec_stat es = message_with_valueref_args(MCM_menu_pick, t_menu_string);
				MCValueRelease(t_menu_string);
				if (es == ES_NOT_HANDLED || es == ES_PASS)
					message_with_args(MCM_mouse_up, menubutton);
			}
			state &= ~(CS_MFOCUSED | CS_ARMED | CS_HILITED);
			break;
		default:
			break;
	}
	
	// MW-2011-02-08: [[ Bug 9384 ]] Free the Mac menu after use since we don't need
	//   it lingering around.
	if (bMenuID != 0)
	{
		MCScreenDC *pms = (MCScreenDC *)MCscreen;
		pms->freeMenuAndID(bMenuID, this);
		bMenuID = 0;
	}
}

void MCButton::macfreemenu(void)
{
	if (bMenuID != 0)
	{
		//free button's menu and its id
		MCScreenDC *pms = (MCScreenDC *)MCscreen;
		pms->freeMenuAndID(bMenuID, this);
		bMenuID = 0;
	}
}

void MCButton::getmacmenuitemtextfromaccelerator(short menuid, uint2 key, uint1 mods, MCStringRef &r_string, bool issubmenu)
{
	::getmacmenuitemtextfromaccelerator(GetMenu(menuid), key, mods, r_string, issubmenu);
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

////////////////////////////////////////////////////////////////////////////////
//
//  MISC 
//

void MCMacApplyPatternToFillImage(MCContext *ctxt, const MCRectangle& rect)
{
	((MCQuickDrawContext *)ctxt) -> fillrect_with_native_function(rect, notSrcBic);
}

Pixmap MCMacThemeGetBackgroundPixmap(Window_mode mode, Boolean active)
{
	return ((MCScreenDC *)MCscreen) -> getbackpm(mode, active);
}

////////////////////////////////////////////////////////////////////////////////
//
//  REFACTORED FROM GLOBALS.CPP
//

MCUIDC *MCCreateScreenDC(void)
{
	return new MCScreenDC;
}

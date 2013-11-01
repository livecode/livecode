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

#include "dispatch.h"
#include "globals.h"
#include "uidc.h"
#include "execpt.h"
#include "card.h"
#include "stack.h"
#include "image.h"

#include "w32dc.h"

#include <strsafe.h>

////////////////////////////////////////////////////////////////////////////////

void MCScreenDC::seticon(uint4 p_icon)
{
}

void MCScreenDC::seticonmenu(MCStringRef p_menu)
{
}

////////////////////////////////////////////////////////////////////////////////

static HICON create_status_icon(uint32_t p_icon_id)
{
	MCImage *t_image;
	t_image = (MCImage *)(MCdispatcher -> getobjid(CT_IMAGE, p_icon_id));
	if (t_image == nil)
		return nil;

	int t_image_width, t_image_height;
	t_image->openimage();

	MCImageBitmap *t_bitmap = nil;
	Pixmap t_drawdata = nil, t_drawmask = nil;
	MCBitmap *t_maskimagealpha = nil;
	/* UNCHECKED */ t_image->lockbitmap(t_bitmap);
	/* UNCHECKED */ MCImageSplitPixmaps(t_bitmap, t_drawdata, t_drawmask, t_maskimagealpha);
	t_image_width = t_bitmap->width;
	t_image_height = t_bitmap->height;
	t_image->unlockbitmap(t_bitmap);

	t_image->closeimage();

	if (t_maskimagealpha != nil)
		MCscreen->destroyimage(t_maskimagealpha);
	MCscreen->freepixmap(t_drawmask);


	int t_width, t_height;
	HDC t_dc;
	t_dc = GetDC(NULL);

	t_width = GetSystemMetrics(SM_CXICON);
	t_height = GetSystemMetrics(SM_CYICON);

	HBITMAP t_data;
	HBITMAP t_mask;
	HICON t_icon;
	t_icon = nil;

	// This won't work on Win98/ME - seperate code-path needed.
	t_data = CreateCompatibleBitmap(t_dc, t_width, t_height);
	t_mask = CreateBitmap(t_width, t_height, 1, 1, NULL);

	if (t_data != NULL && t_mask != NULL)
	{
		HDC t_src_dc, t_dst_dc;
		t_src_dc = CreateCompatibleDC(t_dc);
		t_dst_dc = CreateCompatibleDC(t_dc);

		if (t_src_dc != NULL && t_dst_dc != NULL)
		{
			HGDIOBJ t_old_src, t_old_dst;

			t_old_src = SelectObject(t_src_dc, t_drawdata -> handle . pixmap);
			t_old_dst = SelectObject(t_dst_dc, t_data);

			StretchBlt(t_dst_dc, 0, 0, t_width, t_height, t_src_dc, 0, 0, t_image_width, t_image_height, SRCCOPY);

			SelectObject(t_dst_dc, t_old_dst);
			SelectObject(t_src_dc, t_old_src);

			ICONINFO t_icon_data;
			t_icon_data . fIcon = TRUE;
			t_icon_data . xHotspot = 0;
			t_icon_data . yHotspot = 0;
			t_icon_data . hbmMask = t_mask;
			t_icon_data . hbmColor = t_data;
			t_icon = CreateIconIndirect(&t_icon_data);
		}

		if (t_src_dc != NULL)
			DeleteDC(t_src_dc);

		if (t_dst_dc != NULL)
			DeleteDC(t_dst_dc);
	}

	ReleaseDC(NULL, t_dc);

	if (t_data != NULL)
		DeleteObject(t_data);

	if (t_mask != NULL)
		DeleteObject(t_mask);

	MCscreen->freepixmap(t_drawdata);

	return t_icon;
}

////////////////////////////////////////////////////////////////////////////////

struct MenuItemDescriptor
{
	MenuItemDescriptor *next;
	uint4 depth;
	uint4 id;
	bool disabled;
	bool checked;
	MCStringRef m_name;
	MCStringRef m_tag;
	MenuItemDescriptor *submenu;

	MenuItemDescriptor()
	{
		m_name = MCValueRetain(kMCEmptyString);
		m_tag = MCValueRetain(kMCEmptyString);

		next = nil;
		submenu = nil;

		depth = 0;
		id = 0;
		disabled = false;
		checked = false;
	}

	~MenuItemDescriptor()
	{
		MCValueRelease(m_name);
		MCValueRelease(m_tag);

		if (submenu != nil)
			delete submenu;

		if (next != nil)
			delete next;
	}
};

static void delete_menu_descriptor(MenuItemDescriptor *p_descriptor)
{
	delete p_descriptor;
}

static void clear_menu(HMENU p_menu)
{
	for(int t_index = 0; t_index < GetMenuItemCount(p_menu); t_index++)
	{
		MENUITEMINFOW t_info;
		memset(&t_info, 0, sizeof(MENUITEMINFOW));
		t_info . cbSize = sizeof(MENUITEMINFOW);
		t_info . fMask = MIIM_DATA;
		GetMenuItemInfoW(p_menu, t_index, TRUE, &t_info);

		if (t_info . dwItemData != 0)
			delete (char *)t_info . dwItemData;
	}
}

static HMENU create_menu(MenuItemDescriptor *p_items)
{
	HMENU t_menu;
	uint4 t_index;

	t_menu = CreatePopupMenu();
	t_index = 0;

	for(MenuItemDescriptor *t_item = p_items; t_item != NULL; t_item = t_item -> next)
	{
		MENUITEMINFOW t_info;
		MCAutoStringRefAsWString t_text_wstr, t_tag_wstr;

		t_info . cbSize = sizeof(MENUITEMINFOW);
		t_info . fMask = MIIM_DATA | MIIM_ID | MIIM_SUBMENU | MIIM_TYPE | MIIM_STATE;
		t_info . fState = MFS_ENABLED;
		t_info . wID = t_item -> id;

		if (MCStringGetLength(t_item->m_name) == 1 && MCStringGetCharAtIndex(t_item->m_name, 0) == '-')
		{
			t_info . fType = MFT_SEPARATOR;
			t_info . dwTypeData = NULL;
			t_info . cch = 0;
		}
		else
		{
			/* UNCHECKED */ t_text_wstr.Lock(t_item -> m_name);
			t_info . fType = MFT_STRING;
			t_info . dwTypeData = *t_text_wstr;
			t_info . cch = MCStringGetLength(t_item -> m_name);
		}

		if (!MCStringIsEmpty(t_item -> m_tag))
		{
			/* UNCHECKED */ t_tag_wstr.Lock(t_item -> m_tag);
		}
		else
		{
			/* UNCHECKED */ t_tag_wstr.Lock(t_item -> m_name);
		}

		if (t_item -> disabled)
			t_info . fState |= MFS_DISABLED;
		
		t_info . dwItemData = (DWORD)*t_tag_wstr;
		
		if (t_item -> submenu != NULL)
			t_info . hSubMenu = create_menu(t_item -> submenu);
		else
			t_info . hSubMenu = NULL;

		InsertMenuItemW(t_menu, t_index, TRUE, &t_info);

		t_index += 1;
	}

	return t_menu;
}

static void flatten_menu(MenuItemDescriptor *p_menu)
{
	MenuItemDescriptor *t_item;
	t_item = p_menu;
	while(t_item != NULL)
	{
		if (t_item -> next != NULL && t_item -> next -> depth > t_item -> depth)
		{
			MenuItemDescriptor *t_first_subitem, *t_last_subitem;
		
			t_first_subitem = t_item -> next;
			for(t_last_subitem = t_first_subitem; t_last_subitem -> next != NULL && t_last_subitem -> next -> depth >= t_first_subitem -> depth; t_last_subitem = t_last_subitem -> next)
				;
			
			t_item -> submenu = t_first_subitem;
			t_item -> next = t_last_subitem -> next;
			
			t_last_subitem -> next = NULL;
			
			flatten_menu(t_item -> submenu);
		}
		
		t_item = t_item -> next;
	}
}

static HMENU create_icon_menu(MCStringRef p_menu)
{
	// Do nothing if there is no menu
	if (p_menu == nil)
		return NULL;
	
	MenuItemDescriptor *t_items, *t_current_item;
	uint4 t_id;
	
	t_items = NULL;
	t_current_item = NULL;
	t_id = 1;
	
	uindex_t t_offset, t_length;
	t_offset = 0;
	t_length = MCStringGetLength(p_menu);

	while(t_offset < t_length)
	{
		// Scan for the next newline to determine the end of the item
		uindex_t t_item_end;
		if (MCStringFirstIndexOfChar(p_menu, '\n', t_offset, kMCStringOptionCompareExact, t_item_end))
			t_item_end = t_length;

		// Scan for the tag separator
		uindex_t t_tag_sep;
		if (!MCStringFirstIndexOfChar(p_menu, '|', t_offset, kMCStringOptionCompareExact, t_tag_sep))
			t_tag_sep = t_item_end;
		if (t_tag_sep > t_item_end)
			t_tag_sep = t_item_end;

		// Skip any tab characters at the beginning of the item
		uindex_t t_item_start;
		t_item_start = t_offset;
		while (MCStringGetCharAtIndex(p_menu, t_item_start) == '\t')
			t_item_start++;

		MenuItemDescriptor *t_item;
		t_item = new MenuItemDescriptor;
	
		if (t_current_item == NULL)
			t_items = t_item, t_current_item = t_items;
		else
			t_current_item -> next = t_item, t_current_item = t_item;
		
		if (MCStringGetCharAtIndex(p_menu, t_item_start) == '(')
		{
			// Indicates item is disabled unless escaped as "(("
			if (MCStringGetCharAtIndex(p_menu, t_item_start + 1) != '(')
				t_item -> disabled = true;
		}

		MCValueRelease(t_item->m_name);
		/* UNCHECKED */ MCStringCopySubstring(p_menu, MCRangeMake(t_item_start, t_tag_sep - t_item_start), t_item->m_name);

		if (t_tag_sep != t_item_end)
		{
			MCValueRelease(t_item->m_tag);
			/* UNCHECKED */ MCStringCopySubstring(p_menu, MCRangeMake(t_tag_sep + 1, t_item_end - t_tag_sep - 1), t_item->m_tag);
		}

		t_item -> depth = t_item_start - t_offset;
		t_item -> submenu = NULL;
		t_item -> id = t_id++;
		
		t_offset = t_item_end;
	}
	
	if (t_items != NULL)
		flatten_menu(t_items);

	HMENU t_menu;
	t_menu = create_menu(t_items);

	delete_menu_descriptor(t_items);

	return t_menu;
}

static void destroy_icon_menu(HMENU p_menu)
{
	clear_menu(p_menu);
	DestroyMenu(p_menu);
}

////////////////////////////////////////////////////////////////////////////////

void MCScreenDC::configurestatusicon(uint32_t p_icon_id, MCStringRef p_menu, MCStringRef p_tooltip)
{
	NOTIFYICONDATAW t_nidata;
	memset(&t_nidata, 0, sizeof(NOTIFYICONDATAW));
	t_nidata . cbSize = sizeof(NOTIFYICONDATAW);
	t_nidata . hWnd = invisiblehwnd;
	t_nidata . uID = 1;

	HICON t_icon;
	if (p_icon_id != 0)
		t_icon = create_status_icon(p_icon_id);
	else
		t_icon = nil;

	if (f_icon_menu != nil)
	{
		clear_menu(f_icon_menu);
		f_icon_menu = nil;
	}

	if (t_icon == nil)
	{
		Shell_NotifyIconW(NIM_DELETE, &t_nidata);
		f_has_icon = false;
		return;
	}

	t_nidata . uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	t_nidata . uCallbackMessage = CWM_TASKBAR_NOTIFICATION;
	t_nidata . hIcon = t_icon;

	MCAutoStringRefAsWString t_tooltip_wstr;
	/* UNCHECKED */ t_tooltip_wstr.Lock(p_tooltip);

	if (p_tooltip != nil && !MCStringIsEmpty(p_tooltip))
		/* UNCHECKED */ StringCchCopy(t_nidata.szTip, 128, *t_tooltip_wstr);
	else
		t_nidata . szTip[0] = '\0';

	Shell_NotifyIconW(f_has_icon ? NIM_MODIFY : NIM_ADD, &t_nidata);

	if (p_menu != nil && !MCStringIsEmpty(p_menu))
		f_icon_menu = create_icon_menu(p_menu);

	f_has_icon = true;
}

bool build_pick_string(HMENU p_menu, UINT32 p_command, MCStringRef x_mutable)
{
	for(int4 t_index = 0; t_index < GetMenuItemCount(p_menu); t_index++)
	{
		bool t_success;
		t_success = false;
	
		MENUITEMINFOW t_info;
		memset(&t_info, 0, sizeof(MENUITEMINFOW));
		t_info . cbSize = sizeof(MENUITEMINFOW);
		t_info . fMask = MIIM_DATA | MIIM_ID | MIIM_SUBMENU;
		GetMenuItemInfoW(p_menu, t_index, TRUE, &t_info);

		if (t_info . wID == p_command)
			t_success = true;
				
		if (!t_success)
		{
			if (t_info . hSubMenu != NULL && build_pick_string(t_info . hSubMenu, p_command, x_mutable))
			{
				/* UNCHECKED */ MCStringAppendChar(x_mutable, '|');
				t_success = true;
			}
		}
		
		if (t_success)
		{
			/* UNCHECKED */ MCStringAppendChars(x_mutable, t_info . dwTypeData, lstrlenW(t_info . dwTypeData));
			return true;
		}
	}
	
	return false;
}

void MCScreenDC::processtaskbarnotify(HWND hwnd, WPARAM wparam, LPARAM lparam)
{
	static bool s_got_dblclick = false;

	switch(lparam)
	{
	case WM_RBUTTONDOWN:
	{
		HMENU t_menu;
		UINT t_command;

		t_menu = f_icon_menu;
		if (t_menu != nil)
		{
			if (MCdefaultstackptr != NULL)
				MCdefaultstackptr -> getcurcard() -> message(MCM_status_icon_menu_opening);

			if (t_menu != NULL)
			{
				POINT t_point;
				GetCursorPos(&t_point);
				SetForegroundWindow(hwnd);
				PostMessageW(hwnd, WM_NULL, 0, 0);
				t_command = TrackPopupMenu(f_icon_menu, TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD, t_point . x, t_point . y, 0, hwnd, NULL);

				if (t_command != 0)
				{
					MCAutoStringRef t_pick;
					/* UNCHECKED */ MCStringCreateMutable(0, &t_pick);
					build_pick_string(t_menu, t_command, *t_pick);
					if (MCdefaultstackptr != NULL)
						MCdefaultstackptr -> getcurcard() -> message_with_valueref_args(MCM_status_icon_menu_pick, *t_pick);
				}
			}
		}
	}
	break;

	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
	case WM_MBUTTONUP:
	// MM-2011-05-23: Send any balloon click messages as status icon click for now.  Used by auto updater in plugin installer.
	// Reorganise in the future when we update status icon handling.
	case NIN_BALLOONUSERCLICK: 
	{
		uint32_t t_button;
		if (lparam == WM_LBUTTONUP)
			t_button = 1;
		else if (lparam == WM_RBUTTONUP)
			t_button = 2;
		else
			t_button = 3;

		if (!s_got_dblclick)
		{
			if (MCdefaultstackptr != nil)
				MCdefaultstackptr -> getcurcard() -> message_with_args(MCM_status_icon_click, t_button);
		}
		else
			s_got_dblclick = false;
	}
	break;

	case WM_LBUTTONDBLCLK:
	case WM_RBUTTONDBLCLK:
	case WM_MBUTTONDBLCLK:
	{
		uint32_t t_button;
		if (lparam == WM_LBUTTONDBLCLK)
			t_button = 1;
		else if (lparam == WM_RBUTTONDBLCLK)
			t_button = 2;
		else
			t_button = 3;

		s_got_dblclick = true;
		if (MCdefaultstackptr != nil)
			MCdefaultstackptr -> getcurcard() -> message_with_args(MCM_status_icon_double_click, t_button);
	}
	break;
	}
}

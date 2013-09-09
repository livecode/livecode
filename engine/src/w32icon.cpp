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
	const char *name;
	uint4 name_length;
	const char *tag;
	uint4 tag_length;
	MenuItemDescriptor *submenu;
};

static void delete_menu_descriptor(MenuItemDescriptor *p_descriptor)
{
	while(p_descriptor != NULL)
	{
		MenuItemDescriptor *t_next;
		t_next = p_descriptor -> next;
		delete_menu_descriptor(p_descriptor -> submenu);
		delete p_descriptor;
		p_descriptor = t_next;
	}
}

static void clear_menu(HMENU p_menu)
{
	for(int t_index = 0; t_index < GetMenuItemCount(p_menu); t_index++)
	{
		MENUITEMINFOA t_info;
		memset(&t_info, 0, sizeof(MENUITEMINFOA));
		t_info . cbSize = sizeof(MENUITEMINFOA);
		t_info . fMask = MIIM_DATA;
		GetMenuItemInfoA(p_menu, t_index, TRUE, &t_info);

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
		MENUITEMINFOA t_info;
		char t_text[256];

		t_info . cbSize = sizeof(MENUITEMINFO);
		t_info . fMask = MIIM_DATA | MIIM_ID | MIIM_SUBMENU | MIIM_TYPE | MIIM_STATE;
		t_info . fState = MFS_ENABLED;
		t_info . wID = t_item -> id;

		if (t_item -> name_length == 1 && t_item -> name[0] == '-')
		{
			t_info . fType = MFT_SEPARATOR;
			t_info . dwTypeData = NULL;
			t_info . cch = 0;
		}
		else
		{
			strncpy(t_text, t_item -> name, MCU_min(t_item -> name_length, 256U));
			t_text[t_item -> name_length] = '\0';

			t_info . fType = MFT_STRING;
			t_info . dwTypeData = t_text;
			t_info . cch = t_item -> name_length;
		}

		char *t_tag;
		if (t_item -> tag_length > 0)
		{
			t_tag = new char[t_item -> tag_length + 1];
			strncpy(t_tag, t_item -> tag, t_item -> tag_length);
			t_tag[t_item -> tag_length] = '\0';
		}
		else
		{
			t_tag = new char[t_item -> name_length + 1];
			strncpy(t_tag, t_item -> name, t_item -> name_length);
			t_tag[t_item -> name_length] = '\0';
		}

		if (t_item -> disabled)
			t_info . fState |= MFS_DISABLED;
		
		t_info . dwItemData = (DWORD)t_tag;
		
		if (t_item -> submenu != NULL)
			t_info . hSubMenu = create_menu(t_item -> submenu);
		else
			t_info . hSubMenu = NULL;

		InsertMenuItemA(t_menu, t_index, TRUE, &t_info);

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

static HMENU create_icon_menu(const char *p_menu)
{
	MenuItemDescriptor *t_items, *t_current_item;
	uint4 t_id;
	
	t_items = NULL;
	t_current_item = NULL;
	t_id = 1;
	
	if (p_menu != NULL)
		while(*p_menu != '\0')
		{
			const char *t_item_start, *t_item_end, *t_item_middle;
			t_item_start = p_menu;
			t_item_end = strchr(t_item_start, '\n');
			if (t_item_end == NULL)
				t_item_end = strchr(t_item_start, '\0');
			
			t_item_middle = strchr(t_item_start, '|');
			if (t_item_middle > t_item_end)
				t_item_middle = NULL;
			
			for(; *t_item_start == '\t'; ++t_item_start)
				;
			
			MenuItemDescriptor *t_item;
			t_item = new MenuItemDescriptor;
			if (t_item == NULL)
				break;
				
			if (t_current_item == NULL)
				t_items = t_item, t_current_item = t_items;
			else
				t_current_item -> next = t_item, t_current_item = t_item;
			
			t_item -> next = NULL;
			
			if (*t_item_start == '(')
			{
				if (t_item_start[1] != '(')
					t_item -> disabled = true;
				else
					t_item -> disabled = false;
				t_item_start++;
				p_menu++;
			}
			else
				t_item -> disabled = false;
				
			t_item -> name = t_item_start;
			if (t_item_middle == NULL)
			{
				t_item -> name_length = t_item_end - t_item_start;
				t_item -> tag = NULL;
				t_item -> tag_length = 0;
			}
			else
			{
				t_item -> name_length = t_item_middle - t_item_start;
				t_item -> tag = t_item_middle + 1;
				t_item -> tag_length = t_item_end - t_item_middle - 1;
			}
			
			t_item -> depth = t_item_start - p_menu;
			t_item -> submenu = NULL;
			t_item -> id = t_id++;
			
			if (*t_item_end == '\0')
				p_menu = t_item_end;
			else
				p_menu = t_item_end + 1;
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
	NOTIFYICONDATAA t_nidata;
	memset(&t_nidata, 0, sizeof(NOTIFYICONDATAA));
	t_nidata . cbSize = sizeof(NOTIFYICONDATAA);
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
		Shell_NotifyIconA(NIM_DELETE, &t_nidata);
		f_has_icon = false;
		return;
	}

	t_nidata . uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	t_nidata . uCallbackMessage = CWM_TASKBAR_NOTIFICATION;
	t_nidata . hIcon = t_icon;
	if (p_tooltip != nil)
		strncpy(t_nidata . szTip, MCStringGetCString(p_tooltip), 63);
	else
		t_nidata . szTip[0] = '\0';

	Shell_NotifyIconA(f_has_icon ? NIM_MODIFY : NIM_ADD, &t_nidata);

	if (p_menu != nil)
		f_icon_menu = create_icon_menu(MCStringGetCString(p_menu));

	f_has_icon = true;
}

bool build_pick_string(MCExecPoint& p_result, HMENU p_menu, UINT32 p_command)
{
	for(int4 t_index = 0; t_index < GetMenuItemCount(p_menu); t_index++)
	{
		bool t_success;
		t_success = false;
	
		MENUITEMINFOA t_info;
		memset(&t_info, 0, sizeof(MENUITEMINFOA));
		t_info . cbSize = sizeof(MENUITEMINFOA);
		t_info . fMask = MIIM_DATA | MIIM_ID | MIIM_SUBMENU;
		GetMenuItemInfoA(p_menu, t_index, TRUE, &t_info);

		if (t_info . wID == p_command)
			t_success = true;
				
		if (!t_success)
		{
			if (t_info . hSubMenu != NULL && build_pick_string(p_result, t_info . hSubMenu, p_command))
			{
				p_result . insert("|", 0, 0);
				t_success = true;
			}
		}
		
		if (t_success)
		{
			p_result . insert((char *)t_info . dwItemData, 0, 0);
			
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
				PostMessageA(hwnd, WM_NULL, 0, 0);
				t_command = TrackPopupMenu(f_icon_menu, TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD, t_point . x, t_point . y, 0, hwnd, NULL);

				if (t_command != 0)
				{
					MCExecPoint ep;
					build_pick_string(ep, t_menu, t_command);
					if (MCdefaultstackptr != NULL)
						MCdefaultstackptr -> getcurcard() -> message_with_args(MCM_status_icon_menu_pick, ep . getsvalue());
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

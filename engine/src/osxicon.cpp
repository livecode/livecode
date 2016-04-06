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
#include "parsedef.h"
#include "objdefs.h"

#include "dispatch.h"
#include "globals.h"
#include "uidc.h"
//#include "execpt.h"
#include "card.h"
#include "stack.h"
#include "image.h"

#include "osxdc.h"

///////////////////////////////////////////////////////////////////////////////

void MCScreenDC::seticon(uint4 p_icon)
{
	if (p_icon != 0)
	{
		MCImage *t_image;
		t_image = (MCImage *)(MCdispatcher -> getobjid(CT_IMAGE, p_icon));
		if (t_image != NULL)
		{
			CGImageRef t_cg_image;
			t_cg_image = t_image -> makeicon(128, 128);
			
			SetApplicationDockTileImage(t_cg_image);
			CGImageRelease(t_cg_image);
		}
		else
			p_icon = 0;
	}
	
	if (p_icon == 0)
		RestoreApplicationDockTileImage();
}

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

static void clear_menu(MenuRef p_menu)
{
	for(uint4 t_index = 1; t_index <= CountMenuItems(p_menu); ++t_index)
	{
		OSStatus t_status;
		UInt32 t_ref_con;
		t_status = GetMenuItemRefCon(p_menu, t_index, &t_ref_con);
		if (t_status == noErr && t_ref_con != 0)
			CFRelease((CFStringRef)t_ref_con);
		
		MenuRef t_current_menu;
		GetMenuItemHierarchicalMenu(p_menu, t_index, &t_current_menu);
		if (t_current_menu != NULL)
			clear_menu(t_current_menu);
	}
}

static MenuRef create_menu(MenuRef p_menu, MenuItemDescriptor *p_items)
{
	MenuRef t_menu;
	
	if (p_menu != NULL)
	{
		clear_menu(p_menu);
		DeleteMenuItems(p_menu, 1, CountMenuItems(p_menu));
		t_menu = p_menu;
	}
	else
		CreateNewMenu(0, 0, &t_menu);
	
	for(MenuItemDescriptor *t_item = p_items; t_item != NULL; t_item = t_item -> next)
	{
		CFStringRef t_string;
		MenuItemAttributes t_attributes;
		MenuCommand t_command;
		MenuItemIndex t_index;
		
		if (t_item -> name_length == 1 && t_item -> name[0] == '-')
		{
			t_string = NULL;
			t_attributes = kMenuItemAttrSeparator;
			t_command = 0;
		}
		else
		{
			t_string = CFStringCreateWithBytes(kCFAllocatorDefault, (const UInt8 *)t_item -> name, t_item -> name_length, kCFStringEncodingMacRoman, false);
			t_attributes = 0;
			t_command = t_item -> id;
		}
	
		AppendMenuItemTextWithCFString(t_menu, t_string, t_attributes, t_command, &t_index);
	
		if (t_item -> tag_length != 0)
		{
			CFStringRef t_tag;
			t_tag = CFStringCreateWithBytes(kCFAllocatorDefault, (const UInt8 *)t_item -> tag, t_item -> tag_length, kCFStringEncodingMacRoman, false);
			SetMenuItemRefCon(t_menu, t_index, (UInt32)t_tag);
		}
		else
			SetMenuItemRefCon(t_menu, t_index, 0);
				
		if (t_string != NULL)
			CFRelease(t_string);
		
		if (t_item -> disabled)
			DisableMenuItem(t_menu, t_index);
		
		if (t_item -> submenu != NULL)
		{
			MenuRef t_submenu;
			t_submenu = create_menu(NULL, t_item -> submenu);
			SetMenuItemHierarchicalMenu(t_menu, t_index, t_submenu);
			ReleaseMenu(t_submenu);
		}
	}
	
	return t_menu;
}

void MCScreenDC::seticonmenu(MCStringRef p_menu)
{
	MenuItemDescriptor *t_items, *t_current_item;
	uint4 t_id;
	
	const char *t_menu;
    MCAutoStringRefAsUTF8String temp;
    /* UNCHECKED */ temp . Lock(p_menu);
	t_menu = strclone(*temp);
    	
	t_items = NULL;
	t_current_item = NULL;
	t_id = 65536;
	
	if (t_menu != NULL)
		while(*t_menu != '\0')
		{
			const char *t_item_start, *t_item_end, *t_item_middle;
			t_item_start = t_menu;
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
				t_item_start++, t_menu++, t_item -> disabled = true;
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
			
			t_item -> depth = t_item_start - t_menu;
			t_item -> submenu = NULL;
			t_item -> id = t_id++;
			
			if (*t_item_end == '\0')
				t_menu = t_item_end;
			else
				t_menu = t_item_end + 1;
		}
	
	if (t_items != NULL)
		flatten_menu(t_items);
	
	create_menu(f_icon_menu, t_items);
}

static bool build_pick_string(MenuRef p_menu, UInt32 p_command, MCStringRef& x_string)
{
	for(uint4 t_index = 1; t_index <= CountMenuItems(p_menu); t_index++)
	{
		bool t_success;
		t_success = false;
	
		MenuCommand t_current_command;		
		GetMenuItemCommandID(p_menu, t_index, &t_current_command);
		if (t_current_command == p_command)
			t_success = true;
				
		if (!t_success)
		{
			MenuRef t_current_menu;
			GetMenuItemHierarchicalMenu(p_menu, t_index, &t_current_menu);
			if (t_current_menu != NULL && build_pick_string(t_current_menu, p_command, x_string))
			{
                /* UNCHECKED */ MCStringPrependNativeChar(x_string, '|');
				t_success = true;
			}
		}
		
		if (t_success)
		{
			CFStringRef t_text;
			UInt32 t_ref_con;
			
			GetMenuItemRefCon(p_menu, t_index, &t_ref_con);
			if (t_ref_con != NULL)
			{
				t_text = (CFStringRef)t_ref_con;
				CFRetain(t_text);
			}
			else
				CopyMenuItemTextAsCFString(p_menu, t_index, &t_text);
			
			char t_text_data[256];
			t_text_data[0] = '\0';
			CFStringGetCString(t_text, t_text_data, 256, kCFStringEncodingMacRoman);
			CFRelease(t_text);
			
            return MCStringPrependNativeChars(x_string, (const char_t*)t_text_data, strlen(t_text_data));
			
		}
	}
	
	return false;
}

OSStatus MCScreenDC::handleiconmenuevent(EventHandlerCallRef p_ref, EventRef p_event, void *p_data)
{
	UInt32 t_class;

	t_class = GetEventClass(p_event);

	if (t_class == kEventClassMenu)
	{
		MenuRef t_menu;
		GetEventParameter(p_event, kEventParamDirectObject, typeMenuRef, NULL, sizeof(MenuRef), NULL, &t_menu);
	
		if (t_menu == ((MCScreenDC *)MCscreen) -> f_icon_menu)
		{
			if (MCdefaultstackptr != NULL)
				MCdefaultstackptr -> getcard() -> message(MCM_icon_menu_opening);
		
			return noErr;
		}
	}
	else if (t_class == kEventClassCommand)
	{
		HICommand t_command;
		GetEventParameter(p_event, kEventParamDirectObject, typeHICommand, NULL, sizeof(HICommand), NULL, &t_command);
		
		MenuRef t_menu;
		t_menu = ((MCScreenDC *)MCscreen) -> f_icon_menu;
		
		MCStringRef t_result;
        MCStringCreateMutable(0, t_result);
		if (build_pick_string(t_menu, t_command . commandID, t_result))
		{
			if (MCdefaultstackptr != NULL)
				MCdefaultstackptr -> getcard() -> message_with_valueref_args(MCM_icon_menu_pick, t_result);
            
			MCValueRelease(t_result);
			return noErr;
		}
        MCValueRelease(t_result);
	}
	
	return CallNextEventHandler(p_ref, p_event);
}

///////////////////////////////////////////////////////////////////////////////

void MCScreenDC::configurestatusicon(uint32_t p_icon_id, MCStringRef p_menu, MCStringRef p_tooltip)
{
}

///////////////////////////////////////////////////////////////////////////////

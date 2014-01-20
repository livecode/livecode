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

#include "platform.h"

#include "core.h"
#include "globdefs.h"
#include "filedefs.h"
#include "osspec.h"
#include "typedefs.h"
#include "parsedef.h"
#include "objdefs.h"

#include "execpt.h"
#include "scriptpt.h"
#include "mcerror.h"
#include "globals.h"
#include "util.h"
#include "button.h"
#include "menuparse.h"
#include "group.h"
#include "stacklst.h"

#include "platform.h"

#include "desktop-dc.h"

////////////////////////////////////////////////////////////////////////////////

static char *utf8cstring_from_mcstring(const MCString& p_string, bool p_is_unicode)
{
	MCExecPoint ep(nil, nil, nil);
	ep . setsvalue(p_string);
	if (p_is_unicode)
		ep . utf16toutf8();
	else
		ep . nativetoutf8();
	return strdup(ep . getcstring());
}

class MCMenuBuilderCallback: public IParseMenuCallback
{
	MCPlatformMenuRef m_root_menu;
	
	MCPlatformMenuRef *m_menus;
	uindex_t m_menu_depth;

public:
	MCMenuBuilderCallback(MCPlatformMenuRef p_root_menu)
	{
		m_root_menu = p_root_menu;
		m_menus = nil;
		m_menu_depth = 0;
	}
	
	~MCMenuBuilderCallback(void)
	{
		assert(m_menu_depth == 0);
		MCMemoryDeleteArray(m_menus);
	}
	
	MCPlatformMenuRef TopMenu(void)
	{
		return m_menus[m_menu_depth - 1];
	}
	
	// This method takes ownership of the menu.
	void PushMenu(MCPlatformMenuRef p_menu)
	{
		/* UNCHECKED */ MCMemoryResizeArray(m_menu_depth + 1, m_menus, m_menu_depth);
		m_menus[m_menu_depth - 1] = p_menu;;
	}
	
	void PushNewMenu(void)
	{
		MCPlatformMenuRef t_menu;
		MCPlatformCreateMenu(t_menu);
		
		// If we have a menu already, then we have some work to do.
		if (m_menu_depth > 0)
		{
			MCPlatformMenuRef t_top_menu;
			t_top_menu = TopMenu();
			
			// Fetch the number of items.
			uindex_t t_item_count;
			MCPlatformCountMenuItems(t_top_menu, t_item_count);
			
			// If the top-most menu has no items, add an empty one.
			if (t_item_count == 0)
			{
				MCPlatformAddMenuItem(t_top_menu, UINDEX_MAX);
				t_item_count += 1;
			}
			
			// Set the last item's submenu.
			MCPlatformSetMenuItemProperty(t_top_menu, t_item_count - 1, kMCPlatformMenuItemPropertySubmenu, kMCPlatformPropertyTypeMenuRef, &t_menu);
		}
		
		// Push the menu on the stack.
		PushMenu(t_menu);
	}
	
	void PopMenu(void)
	{
		MCPlatformReleaseMenu(m_menus[m_menu_depth - 1]);
		m_menu_depth -= 1;
	}
	
	virtual bool Start(void)
	{
		MCPlatformRetainMenu(m_root_menu);
		PushMenu(m_root_menu);
		
		return true;
	}
	
	virtual bool ProcessItem(MCMenuItem *p_menuitem)
	{
		// Make sure the depth of menus we are at is the same as the menuitem.
		while(p_menuitem -> depth > m_menu_depth - 1)
			PushNewMenu();
		while(p_menuitem -> depth < m_menu_depth - 1)
			PopMenu();
		
		MCAutoPointer<char> t_item_title;
		t_item_title = utf8cstring_from_mcstring(p_menuitem -> label, p_menuitem -> is_unicode);
		
		if (MCU_strcasecmp(*t_item_title, "-") == 0)
			MCPlatformAddMenuSeparatorItem(TopMenu(), UINDEX_MAX);
		else
		{
			uindex_t t_item_index;
			MCPlatformCountMenuItems(TopMenu(), t_item_index);
			MCPlatformAddMenuItem(TopMenu(), UINDEX_MAX);
			
			MCAutoPointer<char> t_item_tag;
			t_item_tag = utf8cstring_from_mcstring(p_menuitem -> tag, false);
			
			MCPlatformAccelerator t_item_accelerator;
			t_item_accelerator = kMCPlatformAcceleratorNone;
			
			bool t_item_enabled;
			t_item_enabled = !p_menuitem -> is_disabled;
			
			MCPlatformMenuItemHighlight t_item_highlight;
			if (p_menuitem -> is_hilited)
			{
				if (p_menuitem -> is_radio)
					t_item_highlight = kMCPlatformMenuItemHighlightDiamond;
				else
					t_item_highlight = kMCPlatformMenuItemHighlightTick;
			}
			
			MCPlatformSetMenuItemProperty(TopMenu(), t_item_index, kMCPlatformMenuItemPropertyTitle, kMCPlatformPropertyTypeUTF8CString, &t_item_title . PtrRef());
			MCPlatformSetMenuItemProperty(TopMenu(), t_item_index, kMCPlatformMenuItemPropertyTag, kMCPlatformPropertyTypeUTF8CString, &t_item_tag . PtrRef());
			MCPlatformSetMenuItemProperty(TopMenu(), t_item_index, kMCPlatformMenuItemPropertyAccelerator, kMCPlatformPropertyTypeAccelerator, &t_item_accelerator);
			MCPlatformSetMenuItemProperty(TopMenu(), t_item_index, kMCPlatformMenuItemPropertyEnabled, kMCPlatformPropertyTypeBool, &t_item_enabled);
			MCPlatformSetMenuItemProperty(TopMenu(), t_item_index, kMCPlatformMenuItemPropertyHighlight, kMCPlatformPropertyTypeMenuItemHighlight, &t_item_highlight);
		}
		
		return true;
	}
	
	virtual bool End(bool p_has_tags)
	{
		while(m_menu_depth > 0)
			PopMenu();
		
		return true;
	}
};

////////////////////////////////////////////////////////////////////////////////

void MCButton::macopenmenu(void)
{
}

void MCButton::macfreemenu(void)
{
}

Bool MCButton::macfindmenu(bool p_just_for_accel)
{
	return False;
}

void MCButton::getmacmenuitemtextfromaccelerator(short menuid, uint2 key, uint1 mods, MCString &s, bool isunicode, bool issubmenu)
{
}

////////////////////////////////////////////////////////////////////////////////

// This structure holds info about each currently set main menu.
struct MCMainMenuInfo
{
	MCPlatformMenuRef menu;
	MCObjectHandle *target;
};

static MCPlatformMenuRef s_menubar = nil;
static MCObjectHandle **s_menubar_targets = nil;
static uindex_t s_menubar_target_count = 0;
static uindex_t s_menubar_lock_count = 0;

static void populate_menubar_menu_from_button(MCPlatformMenuRef p_menubar, uindex_t p_menubar_index, MCPlatformMenuRef p_menu, MCButton *p_menu_button)
{
	MCString t_menu_title;
	bool t_is_unicode;
	p_menu_button -> getlabeltext(t_menu_title, t_is_unicode);
	
	// Get the menu string.
	MCString t_menu_string;
	p_menu_button -> getmenustring(t_menu_string);
	
	// Get the enabled state.
	bool t_menu_enabled;
	t_menu_enabled = !p_menu_button -> isdisabled();
	
	// Convert the title to a string
	MCAutoPointer<char> t_menu_title_string;
	t_menu_title_string = utf8cstring_from_mcstring(t_menu_title, t_is_unicode);
	
	// Create the menu.
	MCPlatformSetMenuTitle(p_menu, *t_menu_title_string);
	
	MCPlatformSetMenuItemProperty(p_menubar, p_menubar_index, kMCPlatformMenuItemPropertyTitle, kMCPlatformPropertyTypeUTF8CString, &t_menu_title_string . PtrRef());
	MCPlatformSetMenuItemProperty(p_menubar, p_menubar_index, kMCPlatformMenuItemPropertyEnabled, kMCPlatformPropertyTypeBool, &t_menu_enabled);
	
	// Now build the menu from the spec string.
	MCMenuBuilderCallback t_callback(p_menu);
	MCParseMenuString(t_menu_string, &t_callback, p_menu_button -> hasunicode(), WM_PULLDOWN);
}

void MCScreenDC::updatemenubar(Boolean force)
{
	// Original logic taken from previous port - only update the menubar if
	// appropriate.
	
	// If we are inside menuUpdate, we take a different approach.
	if (s_menubar_lock_count > 0)
		return;
	
	MCGroup *newMenuGroup; //pointer to the menu group
	static MCGroup *curMenuGroup = NULL; //current menu bar handle
	if (MCdefaultmenubar == NULL)   // the menu of first stack opened becomes
		MCdefaultmenubar = MCmenubar; // the default menu bar automatically
	//get current menu group
	if (MCmenubar != NULL)
		newMenuGroup = MCmenubar;
	else
		newMenuGroup = MCdefaultmenubar;
	
	//if doesn't need update and not force update then exit
	if (newMenuGroup == NULL || newMenuGroup == curMenuGroup && !force && !curMenuGroup->getstate(CS_NEED_UPDATE))
		return;
	
	MCLog("Updating menubar...", 0);
	
	// Count the number of menus.
	uint2 t_menu_count;
	t_menu_count = 0;
	newMenuGroup -> count(CT_MENU, NULL, t_menu_count);
	
	// The index of the menu in the menubar.
	uindex_t t_menu_index;
	t_menu_index = 0;
	
	// We construct the new menubar as we go along.
	MCObjectHandle **t_new_menubar_targets;
	uindex_t t_new_menubar_target_count;
	t_new_menubar_targets = nil;
	t_new_menubar_target_count = 0;
	
	// The new menubar menu.
	MCPlatformMenuRef t_new_menubar;
	MCPlatformCreateMenu(t_new_menubar);
	
	// Loop through and create the menuitems for the main menu. We defer creation
	// of the menu itself until it's actually needed.
	for(uindex_t t_menu_button_index = 0; t_menu_button_index < t_menu_count; t_menu_button_index++)
	{
		// Fetch the button representing the menu.
		MCButton *t_menu_button;
		uint2 t_menu_button_index_i;
		t_menu_button_index_i = (uint2)t_menu_button_index;
		t_menu_button = (MCButton *)newMenuGroup -> findnum(CT_MENU, t_menu_button_index_i);
		if (t_menu_button == NULL)
			break;
		
		// Remove any menu shortcuts for the current button.
		MCstacks -> deleteaccelerator(t_menu_button, t_menu_button -> getstack());
		
		// Get the menu title.
		MCString t_menu_title;
		bool t_is_unicode;
		t_menu_button -> getlabeltext(t_menu_title, t_is_unicode);
		
		// If the menu button is not visible, or has not title then continue.
		if (t_menu_title . getlength() == 0 ||
			!t_menu_button -> getflag(F_VISIBLE))
			continue;
		
		// Create the menu.
		MCPlatformMenuRef t_menu;
		MCPlatformCreateMenu(t_menu);
		
		// Add a new item to the menubar and configure.
		MCPlatformAddMenuItem(t_new_menubar, UINDEX_MAX);
		MCPlatformSetMenuItemProperty(t_new_menubar, t_menu_index, kMCPlatformMenuItemPropertySubmenu, kMCPlatformPropertyTypeMenuRef, &t_menu);

		// Populate it.
		populate_menubar_menu_from_button(t_new_menubar, t_menu_index, t_menu, t_menu_button);
		
		// Setting the submenu of the menu bar will have inc'd the refcount so
		// we can release.
		MCPlatformReleaseMenu(t_menu);
		
		// Extend the new menubar targets array by one.
		/* UNCHECKED */ MCMemoryResizeArray(t_new_menubar_target_count + 1, t_new_menubar_targets, t_new_menubar_target_count);
		t_new_menubar_targets[t_menu_index] = t_menu_button -> gethandle();
		
		// Increment the index into the menubar.
		t_menu_index++;
	}
	
	// Free the existing menubar and release the targets.
	if (s_menubar != nil)
	{
		MCPlatformReleaseMenu(s_menubar);
		for(uindex_t i = 0; i < s_menubar_target_count; i++)
			s_menubar_targets[i] -> Release();
		MCMemoryDeleteArray(s_menubar_targets);
	}
	
	// Update to the new menubar and targets.
	s_menubar = t_new_menubar;
	s_menubar_targets = t_new_menubar_targets;
	s_menubar_target_count = t_new_menubar_target_count;	
	
	// Set the menubar.
	MCPlatformSetMenubar(s_menubar);
}

////////////////////////////////////////////////////////////////////////////////

void MCScreenDC::hidemenu()
{
	MCPlatformHideMenubar();
	menubarhidden = true ;
	if (s_monitor_count != 0)
	{
		delete[] s_monitor_displays;
		s_monitor_displays = NULL;
		s_monitor_count = 0;
	}
}

void MCScreenDC::showmenu()
{
	MCPlatformShowMenubar();
	menubarhidden = false ;
	if (s_monitor_count != 0)
	{
		delete[] s_monitor_displays;
		s_monitor_displays = NULL;
		s_monitor_count = 0;
	}
}

////////////////////////////////////////////////////////////////////////////////

void MCPlatformHandleMenuUpdate(MCPlatformMenuRef p_menu)
{
	// We get MenuUpdate callbacks for all menus before they open, however at
	// the moment we are only interested in ones directly in the menubar. So
	// fetch the menu's parent and see what it is.
	MCPlatformMenuRef t_parent_menu;
	uindex_t t_parent_menu_index;
	MCPlatformGetMenuParent(p_menu, t_parent_menu, t_parent_menu_index);
	
	// If the parent menu is not the menubar, we aren't interested.
	if (t_parent_menu != s_menubar)
		return;
	
	// If the button it is 'attached' to still exists, dispatch the menu update
	// message (currently mouseDown("")). We do this whilst the menubar is locked
	// from updates as we mustn't fiddle about with it too much in this case!
	if (s_menubar_targets[t_parent_menu_index] -> Exists())
	{
		s_menubar_lock_count += 1;
		s_menubar_targets[t_parent_menu_index] -> Get() -> message_with_args(MCM_mouse_down, "");
		s_menubar_lock_count -= 1;
	}
	
	// Now we've got the menu to update, process the new menu spec, but only if the
	// menu button still exists!
	if (s_menubar_targets[t_parent_menu_index] -> Exists())
	{
		MCPlatformRemoveAllMenuItems(p_menu);
		populate_menubar_menu_from_button(s_menubar, t_parent_menu_index, p_menu, (MCButton *)s_menubar_targets[t_parent_menu_index] -> Get());
	}
}

void MCPlatformHandleMenuSelect(MCPlatformMenuRef p_menu, uindex_t p_item_index)
{
	// We will get MenuSelect callbacks for all menus. We must build the pick
	// string for all menus in the chain not including the top-level menu. Then
	// we can use the top-level menu to determine where to send the pick.
	
	// Use a temporary ep to accumulate the result.
	MCExecPoint ep(nil, nil, nil);
	ep . clear();
	
	// Keep track of the current (menu / item) pair we are working on.
	MCPlatformMenuRef t_current_menu;
	uindex_t t_current_menu_index;
	t_current_menu = p_menu;
	t_current_menu_index = p_item_index;
	while(t_current_menu != nil)
	{
		// If the current menu is the menubar, then we are done.
		if (t_current_menu == s_menubar)
			break;
		
		// Fetch the tag of menu item.
		MCAutoPointer<char> t_item_tag;
		MCPlatformGetMenuItemProperty(t_current_menu, t_current_menu_index, kMCPlatformMenuItemPropertyTag, kMCPlatformPropertyTypeUTF8CString, &(&t_item_tag));
		
		// If the ep is not empty, then prepend "|"
		if (!ep . isempty())
			ep . insert("|", 0, 0);
		
		// Now insert the tag.
		ep . insert(*t_item_tag, 0, 0);
		
		// Fetch the parent menu and loop
		MCPlatformGetMenuParent(t_current_menu, t_current_menu, t_current_menu_index);
	}
	
	// Convert the pick string from UTF8 to native.
	ep . utf8tonative();
	
	// If the current menu is non-nil, we are dispatching to a main-menu; otherwise it
	// will be the current popup menu.
	if (t_current_menu != nil)
	{
		if (s_menubar_targets[t_current_menu_index] -> Exists())
			s_menubar_targets[t_current_menu_index] -> Get() -> message_with_args(MCM_menu_pick, ep . getsvalue());
	}
	else
	{
		// COCOA-TODO: Handle popup menus.
	}
}

////////////////////////////////////////////////////////////////////////////////

/* Copyright (C) 2003-2017 LiveCode Ltd.
 
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
#include "platform-internal.h"

extern MCPlatformMenuRef MCMacPlatformCreateMenu(void);
extern void MCMacPlatformSetIconMenu(MCPlatformMenuRef p_menu);
extern void MCMacPlatformSetMenubar(MCPlatformMenuRef p_menu);
extern MCPlatformMenuRef MCMacPlatformGetMenubar(void);
extern void MCMacPlatformHideMenubar(void);
extern void MCMacPlatformShowMenubar(void);

////////////////////////////////////////////////////////////////////////////////

MCPlatformMenu::MCPlatformMenu(void)
{
}

MCPlatformMenu::~MCPlatformMenu(void)
{
}

////////////////////////////////////////////////////////////////////////////////

void MCPlatformCreateMenu(MCPlatformMenuRef& r_menu)
{
    MCPlatformMenuRef t_menu;
    t_menu = MCMacPlatformCreateMenu();
    
    r_menu = t_menu;
}

void MCPlatformRetainMenu(MCPlatformMenuRef p_menu)
{
    p_menu -> Retain();
}

void MCPlatformReleaseMenu(MCPlatformMenuRef p_menu)
{
    p_menu -> Release();
}

void MCPlatformSetMenuTitle(MCPlatformMenuRef p_menu, MCStringRef p_title)
{
    p_menu -> SetTitle(p_title);
 }

void MCPlatformCountMenuItems(MCPlatformMenuRef p_menu, uindex_t& r_count)
{
    r_count = p_menu -> CountItems();
}

void MCPlatformAddMenuItem(MCPlatformMenuRef p_menu, uindex_t p_where)
{
    p_menu -> AddItem(p_where);
}

void MCPlatformAddMenuSeparatorItem(MCPlatformMenuRef p_menu, uindex_t p_where)
{
    p_menu -> AddSeparatorItem(p_where);
}

void MCPlatformRemoveMenuItem(MCPlatformMenuRef p_menu, uindex_t p_where)
{
    p_menu -> RemoveItem(p_where);
}

void MCPlatformRemoveAllMenuItems(MCPlatformMenuRef p_menu)
{
    p_menu -> RemoveAllItems();
}

void MCPlatformGetMenuParent(MCPlatformMenuRef p_menu, MCPlatformMenuRef& r_parent, uindex_t& r_index)
{
    p_menu -> GetParent(r_parent, r_index);
}

void MCPlatformGetMenuItemProperty(MCPlatformMenuRef p_menu, uindex_t p_index, MCPlatformMenuItemProperty p_property, MCPlatformPropertyType p_type, void *r_value)
{
    p_menu -> GetItemProperty(p_index, p_property, p_type, r_value);
}

void MCPlatformSetMenuItemProperty(MCPlatformMenuRef p_menu, uindex_t p_index, MCPlatformMenuItemProperty p_property, MCPlatformPropertyType p_type, const void *p_value)
{
    p_menu -> SetItemProperty(p_index, p_property, p_type, p_value);
}

//////////

bool MCPlatformPopUpMenu(MCPlatformMenuRef p_menu, MCPlatformWindowRef p_window, MCPoint p_location, uindex_t p_item)
{
    return p_menu -> PopUp(p_window, p_location, p_item);
}

//////////

void MCPlatformShowMenubar(void)
{
    MCMacPlatformShowMenubar();
}

void MCPlatformHideMenubar(void)
{
    MCMacPlatformHideMenubar();
}

void MCPlatformSetMenubar(MCPlatformMenuRef p_menu)
{
    MCMacPlatformSetMenubar(p_menu);
}

void MCPlatformGetMenubar(MCPlatformMenuRef& r_menu)
{
    r_menu = MCMacPlatformGetMenubar();
}

//////////

void MCPlatformSetIconMenu(MCPlatformMenuRef p_menu)
{
    MCMacPlatformSetIconMenu(p_menu);
}

////////////////////////////////////////////////////////////////////////////////

// MW-2014-04-11: [[ Bug 12068 ]] On startup setup an empty default menubar
//   so that all apps get Quit / About items.
bool MCPlatformInitializeMenu(void)
{
    MCPlatformMenuRef t_menubar;
    t_menubar = MCMacPlatformCreateMenu();
    MCMacPlatformSetMenubar(t_menubar);
    t_menubar -> Release();
    return true;
}

void MCPlatformFinalizeMenu(void)
{
    
}

////////////////////////////////////////////////////////////////////////////////

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


#include <Cocoa/Cocoa.h>

#include "core.h"
#include "globdefs.h"
#include "region.h"
#include "graphics.h"

#include "platform.h"
#include "platform-internal.h"

#include "mac-internal.h"

#include <Carbon/Carbon.h>

////////////////////////////////////////////////////////////////////////////////

// NOTE:
// At the moment we implement the menu directly for cocoa as Mac is the only
// platform that uses native menus - we will probably want to abstract in a
// similar way to the MCPlatformWindow at a later date though.

////////////////////////////////////////////////////////////////////////////////

@interface com_runrev_livecode_MCMenuDelegate: NSObject<NSMenuDelegate>
{
	MCPlatformMenuRef m_menu;
}

//////////

- (id)initWithPlatformMenuRef: (MCPlatformMenuRef)p_menu_ref;
- (void)dealloc;

//////////

- (MCPlatformMenuRef)platformMenuRef;

//////////

- (void)menuNeedsUpdate: (NSMenu *)menu;
- (void)menuItemSelected: (id)sender;

@end

@compatibility_alias MCMenuDelegate com_runrev_livecode_MCMenuDelegate;

@implementation com_runrev_livecode_MCMenuDelegate

- (id)initWithPlatformMenuRef: (MCPlatformMenuRef)p_menu_ref
{
	self = [super init];
	if (self == nil)
		return nil;
	
	m_menu = p_menu_ref;
	
	return self;
}

- (void)dealloc
{
	[super dealloc];
}

//////////

- (MCPlatformMenuRef)platformMenuRef
{
	return m_menu;
}

//////////

- (void)menuNeedsUpdate: (NSMenu *)menu
{
	MCPlatformCallbackSendMenuUpdate(m_menu);
}

- (void)menuItemSelected: (id)sender
{
	NSMenuItem *t_item;
	t_item = (NSMenuItem *)sender;
	MCPlatformCallbackSendMenuSelect(m_menu, [[t_item menu] indexOfItem: t_item]);
}

@end

// At the moment, we only do menus for Mac so we implement directly without a
// 'derived' class idea. Indeed, a platform menu is nothing more than a wrapper
// around NSMenu.
struct MCPlatformMenu
{
	uint32_t references;
	NSMenu *menu;
	MCMenuDelegate *menu_delegate;
};

void MCPlatformCreateMenu(MCPlatformMenuRef& r_menu)
{
	MCPlatformMenuRef t_menu;
	/* UNCHECKED */ MCMemoryNew(t_menu);
	
	t_menu -> references = 1;
	
	t_menu -> menu = [[NSMenu alloc] initWithTitle: @""];
	t_menu -> menu_delegate = [[MCMenuDelegate alloc] initWithPlatformMenuRef: t_menu];
	[t_menu -> menu setDelegate: t_menu -> menu_delegate];
	
	r_menu = t_menu;
}

void MCPlatformRetainMenu(MCPlatformMenuRef p_menu)
{
	p_menu -> references += 1;
}

void MCPlatformReleaseMenu(MCPlatformMenuRef p_menu)
{
	p_menu -> references -= 1;
	if (p_menu -> references != 0)
		return;
	
	[p_menu -> menu release];
	[p_menu -> menu_delegate release];
	MCMemoryDelete(p_menu);
}

void MCPlatformSetMenuTitle(MCPlatformMenuRef p_menu, const char *p_title)
{
	[p_menu -> menu setTitle: [NSString stringWithCString: p_title encoding: NSUTF8StringEncoding]];
}

void MCPlatformCountMenuItems(MCPlatformMenuRef p_menu, uindex_t& r_count)
{
	r_count = [p_menu -> menu numberOfItems];
}

void MCPlatformAddMenuItem(MCPlatformMenuRef p_menu, uindex_t p_where)
{
	uindex_t t_count;
	t_count = [p_menu -> menu numberOfItems];
	
	if (p_where > t_count)
		p_where = t_count;
	
	[p_menu -> menu insertItemWithTitle: @"" action: @selector(menuItemSelected:) keyEquivalent: @"" atIndex: p_where];
}

void MCPlatformAddMenuSeparatorItem(MCPlatformMenuRef p_menu, uindex_t p_where)
{
	uindex_t t_count;
	t_count = [p_menu -> menu numberOfItems];
	
	if (p_where > t_count)
		p_where = t_count;
	
	[p_menu -> menu insertItem: [NSMenuItem separatorItem] atIndex: p_where];
}
 
void MCPlatformRemoveMenuItem(MCPlatformMenuRef p_menu, uindex_t p_where)
{
	[p_menu -> menu removeItemAtIndex: p_where];
}

void MCPlatformGetMenuParent(MCPlatformMenuRef p_menu, MCPlatformMenuRef& r_parent, uindex_t& r_index)
{
	NSMenu *t_parent;
	t_parent = [p_menu -> menu supermenu];
	if (t_parent == nil)
	{
		r_parent = nil;
		r_index = 0;
	}
	
	r_index = [t_parent indexOfItemWithSubmenu: p_menu -> menu];
	r_parent = [(MCMenuDelegate *)[t_parent delegate] platformMenuRef];
}

void MCPlatformGetMenuItemProperty(MCPlatformMenuRef p_menu, uindex_t p_index, MCPlatformMenuItemProperty p_property, MCPlatformPropertyType p_type, void *r_value)
{
}

void MCPlatformSetMenuItemProperty(MCPlatformMenuRef p_menu, uindex_t p_index, MCPlatformMenuItemProperty p_property, MCPlatformPropertyType p_type, const void *p_value)
{
	NSMenuItem *t_item;
	t_item = [p_menu -> menu itemAtIndex: p_index];
	
	switch(p_property)
	{
		case kMCPlatformMenuItemPropertyTitle:
			[t_item setTitle: [NSString stringWithCString: *(const char **)p_value encoding: NSUTF8StringEncoding]];
			break;	
		case kMCPlatformMenuItemPropertyTag:
			[t_item setRepresentedObject: [NSString stringWithCString: *(const char **)p_value encoding: NSUTF8StringEncoding]];
			break;
		case kMCPlatformMenuItemPropertyAction:
			// COCOA-TODO
			break;
		case kMCPlatformMenuItemPropertyAccelerator:
			// COCOA-TODO
			break;
		case kMCPlatformMenuItemPropertyEnabled:
			[t_item setEnabled: *(bool *)p_value];
			break;
		case kMCPlatformMenuItemPropertySubmenu:
			[t_item setSubmenu: (*(MCPlatformMenuRef *)p_value) -> menu];
			break;
		case kMCPlatformMenuItemPropertyHighlight:
			// COCOA-TODO
			break;
	   }
}

//////////

void MCPlatformShowMenubar(void)
{
	ShowMenuBar();
}

void MCPlatformHideMenubar(void)
{
	HideMenuBar();
}

void MCPlatformSetMenubar(MCPlatformMenuRef p_menu)
{
	[NSApp setMainMenu: p_menu -> menu];
}

void MCPlatformGetMenubar(MCPlatformMenuRef& r_menu)
{
	r_menu = nil;
}


////////////////////////////////////////////////////////////////////////////////

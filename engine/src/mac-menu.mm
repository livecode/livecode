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

// Helper method that frees anything associated with an NSMenuItem in an NSMenu
// (at the moment, this is the submenu).
static void MCPlatformDestroyMenuItem(MCPlatformMenuRef p_menu, uindex_t p_index)
{
	NSMenuItem *t_item;
	t_item = [p_menu -> menu itemAtIndex: p_index];
	
	NSMenu *t_submenu;
	t_submenu = [t_item submenu];
	if (t_submenu == nil)
		return;
	
	MCPlatformMenuRef t_submenu_ref;
	t_submenu_ref = [(MCMenuDelegate *)[t_submenu delegate] platformMenuRef];
	
	// Update the submenu pointer (so we don't have any dangling
	// refs).
	[t_item setSubmenu: nil];
	
	// Now release the platform menu.
	MCPlatformReleaseMenu(t_submenu_ref);
}

void MCPlatformCreateMenu(MCPlatformMenuRef& r_menu)
{
	MCPlatformMenuRef t_menu;
	/* UNCHECKED */ MCMemoryNew(t_menu);
	
	t_menu -> references = 1;
	
	t_menu -> menu = [[NSMenu alloc] initWithTitle: @""];
	t_menu -> menu_delegate = [[MCMenuDelegate alloc] initWithPlatformMenuRef: t_menu];
	[t_menu -> menu setDelegate: t_menu -> menu_delegate];
	
	// We don't use autoenablement.
	[t_menu -> menu setAutoenablesItems: NO];
	
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
	
	// Release any submenus.
	for(uindex_t i = 0; i < [p_menu -> menu numberOfItems]; i++)
		MCPlatformDestroyMenuItem(p_menu, i);
	
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
	
	NSMenuItem *t_item;
	t_item = [[NSMenuItem alloc] initWithTitle: @"" action: @selector(menuItemSelected:) keyEquivalent: @""];
	
	// Make sure we set the target of the action (to the delegate).
	[t_item setTarget: p_menu -> menu_delegate];
	
	// Insert the item in the menu.
	[p_menu -> menu insertItem: t_item atIndex: p_where];
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
	MCPlatformDestroyMenuItem(p_menu, p_where);
	[p_menu -> menu removeItemAtIndex: p_where];
}

void MCPlatformRemoveAllMenuItems(MCPlatformMenuRef p_menu)
{
	for(uindex_t i = 0; i < [p_menu -> menu numberOfItems]; i++)
		MCPlatformDestroyMenuItem(p_menu, i);
	[p_menu -> menu removeAllItems];
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
	NSMenuItem *t_item;
	t_item = [p_menu -> menu itemAtIndex: p_index];
	
	switch(p_property)
	{
		case kMCPlatformMenuItemPropertyTag:
			*(char **)r_value = strdup([(NSString *)[t_item representedObject] cStringUsingEncoding: NSUTF8StringEncoding]);
			break;
		default:
			assert(false);
			break;
	}
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
		{
			// Make sure we decrement the associated platformmenuref's count, and
			// increment the new one's (but the other way around, in case they are
			// the same).
			MCPlatformMenuRef t_submenu_ref;
			t_submenu_ref = *(MCPlatformMenuRef *)p_value;
			
			MCPlatformRetainMenu(t_submenu_ref);
			
			NSMenu *t_current_submenu;
			t_current_submenu = [t_item submenu];
			if (t_current_submenu != nil)
			{
				MCPlatformMenuRef t_current_submenu_ref;
				t_current_submenu_ref = [(MCMenuDelegate *)[t_current_submenu delegate] platformMenuRef];
				MCPlatformReleaseMenu(t_current_submenu_ref);
			}
			
			[t_item setSubmenu: (*(MCPlatformMenuRef *)p_value) -> menu];
		}
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

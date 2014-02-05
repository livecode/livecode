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

- (BOOL)menuHasKeyEquivalent:(NSMenu *)menu forEvent:(NSEvent *)event target:(id *)target action:(SEL *)action;

@end

@compatibility_alias MCMenuDelegate com_runrev_livecode_MCMenuDelegate;

@interface com_runrev_livecode_MCAppMenuDelegate: NSObject<NSMenuDelegate>

- (id)init;
- (void)dealloc;

- (void)shadowedMenuItemSelected:(NSString*)tag;
- (NSMenuItem *)findShadowedMenuItem: (NSString *)tag;

- (void)aboutMenuItemSelected: (id)sender;
- (void)preferencesMenuItemSelected: (id)sender;

- (void)menuNeedsUpdate: (NSMenu *)menu;

- (BOOL)menuHasKeyEquivalent:(NSMenu *)menu forEvent:(NSEvent *)event target:(id *)target action:(SEL *)action;

@end

@compatibility_alias MCAppMenuDelegate com_runrev_livecode_MCAppMenuDelegate;

////////////////////////////////////////////////////////////////////////////////

// At the moment, we only do menus for Mac so we implement directly without a
// 'derived' class idea. Indeed, a platform menu is nothing more than a wrapper
// around NSMenu.
struct MCPlatformMenu
{
	uint32_t references;
	NSMenu *menu;
	MCMenuDelegate *menu_delegate;
	
	// If the menu is being used as a menubar then this is true. When this
	// is the case, some items will be hidden and a (API-wise) invisible
	// menu will be inserted at the front (the application menu).
	bool is_menubar : 1;
};

////////////////////////////////////////////////////////////////////////////////

// The menuref currently set as the menubar.
static MCPlatformMenuRef s_menubar = nil;

// The delegate for the app menu.
static com_runrev_livecode_MCAppMenuDelegate *s_app_menu_delegate = nil;

// The services menu that gets populated by Cocoa.
static NSMenu *s_services_menu = nil;

////////////////////////////////////////////////////////////////////////////////

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

- (void)hideShadowedMenuItem:(NSString *)tag menu:(NSMenu *)menu
{
	NSInteger t_index;
	t_index = [menu indexOfItemWithRepresentedObject: tag];
	if (t_index == -1)
		return;
	
	NSMenuItem *t_item;
	t_item = [menu itemAtIndex: t_index];
	[t_item setHidden: YES];
	
	NSUInteger t_item_count;
	t_item_count = [menu numberOfItems];
	
	// If there is only this item in the menu, do no more.
	if (t_item_count == 1)
		return;
	
	// If we are the first item, then remove the separator after
	// us - if any.
	if (t_index == 0 &&
		[[menu itemAtIndex: 1] isSeparatorItem])
	{
		[[menu itemAtIndex: 1] setHidden: YES];
		return;
	}
	
	// If we are the last item, then remove the separator before
	// us - if any.
	if (t_index == t_item_count - 1 &&
		[[menu itemAtIndex: t_index - 1] isSeparatorItem])
	{
		[[menu itemAtIndex: t_index - 1] setHidden: YES];
		return;
	}
	
	// If we have a separated before and after, then remove the
	// separator before us.
	if ([[menu itemAtIndex: t_index - 1] isSeparatorItem] &&
		[[menu itemAtIndex: t_index + 1] isSeparatorItem])
		[[menu itemAtIndex: t_index - 1] setHidden: YES];
}

- (void)menuNeedsUpdate: (NSMenu *)menu
{
	MCPlatformCallbackSendMenuUpdate(m_menu);
	
	[self hideShadowedMenuItem: @"About" menu: menu];
	[self hideShadowedMenuItem: @"Preferences" menu: menu];
	[self hideShadowedMenuItem: @"Quit" menu: menu];
}

- (void)menuItemSelected: (id)sender
{
	NSMenuItem *t_item;
	t_item = (NSMenuItem *)sender;
	MCPlatformCallbackSendMenuSelect(m_menu, [[t_item menu] indexOfItem: t_item]);
}

//////////

- (BOOL)menuHasKeyEquivalent:(NSMenu *)menu forEvent:(NSEvent *)event target:(id *)target action:(SEL *)action
{
	return NO;
}

@end

@implementation com_runrev_livecode_MCAppMenuDelegate

- (id)init
{
	self = [super init];
	if (self == nil)
		return nil;
	
	return self;
}

- (void)dealloc
{
	[super dealloc];
}

- (void)shadowedMenuItemSelected:(NSString*)tag
{
	NSMenuItem *t_shadow;
	t_shadow = [self findShadowedMenuItem: tag];
	if (t_shadow != nil)
		[(MCMenuDelegate *)[[t_shadow menu] delegate] menuItemSelected: t_shadow];
}

- (NSMenuItem *)findShadowedMenuItem: (NSString *)tag
{
	NSMenu *t_menubar;
	t_menubar = s_menubar -> menu;
	for(uindex_t i = 1; i < [t_menubar numberOfItems]; i++)
	{
		NSMenu *t_menu;
		t_menu = [[t_menubar itemAtIndex: i] submenu];
		
		NSInteger t_index;
		t_index = [t_menu indexOfItemWithRepresentedObject: tag];
		if (t_index != -1)
			return [t_menu itemAtIndex: t_index];
	}
	
	return nil;
}

- (void)aboutMenuItemSelected: (id)sender
{
	[self shadowedMenuItemSelected: @"About"];
}

- (void)preferencesMenuItemSelected: (id)sender
{
	[self shadowedMenuItemSelected: @"Preferences"];
}

- (void)menuNeedsUpdate: (NSMenu *)menu
{
	NSMenuItem *t_prefs, *t_about;
	t_prefs = [self findShadowedMenuItem: @"Preferences"];
	t_about = [self findShadowedMenuItem: @"About"];
	
	if (t_prefs != nil)
	{
		[[[t_prefs menu] delegate] menuNeedsUpdate: [t_prefs menu]];
		t_prefs = [self findShadowedMenuItem: @"Preferences"];
		[[menu itemAtIndex: 2] setEnabled: t_prefs != nil ? [t_prefs isEnabled] : NO];
	}
	
	if (t_about != nil)
	{
		if (t_prefs == nil ||
			[t_about menu] != [t_prefs menu])
			[[[t_about menu] delegate] menuNeedsUpdate: [t_about menu]];
		t_about = [self findShadowedMenuItem: @"About"];
		[[menu itemAtIndex: 0] setEnabled: t_about != nil ? [t_about isEnabled] : NO];
	}
}

//////////

// Putting this method here means we don't get an excessive amount of
// menuUpdate messages after each key-press...
- (BOOL)menuHasKeyEquivalent:(NSMenu *)menu forEvent:(NSEvent *)event target:(id *)target action:(SEL *)action
{
	return NO;
}

@end

////////////////////////////////////////////////////////////////////////////////

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

// Map the incoming index to the internal menu item index (taking into account
// whether the menu is the menubar).
static void MCPlatformMapMenuItemIndex(MCPlatformMenuRef p_menu, uindex_t& x_index)
{
	// If the menu is the menubar, adjust for the application menu (always first).
	if (p_menu -> is_menubar)
		x_index += 1;
}

// Clamp the incoming index to an internal menu item insertion index (taking into
// account whether the menu is the menubar).
static void MCPlatformClampMenuItemIndex(MCPlatformMenuRef p_menu, uindex_t& x_index)
{
	if (x_index < UINDEX_MAX && p_menu -> is_menubar)
		x_index += 1;
	x_index = MCMin((unsigned)[p_menu -> menu numberOfItems], x_index);
}

void MCPlatformCreateMenu(MCPlatformMenuRef& r_menu)
{
	MCPlatformMenuRef t_menu;
	/* UNCHECKED */ MCMemoryNew(t_menu);
	
	t_menu -> references = 1;
	
	t_menu -> menu = [[NSMenu alloc] initWithTitle: @""];
	t_menu -> menu_delegate = [[MCMenuDelegate alloc] initWithPlatformMenuRef: t_menu];
	[t_menu -> menu setDelegate: t_menu -> menu_delegate];
	t_menu -> is_menubar = false;
	
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
	
	// If the menu is the menubar, then we've inserted the application menu.
	if (p_menu -> is_menubar)
		r_count -= 1;
}

void MCPlatformAddMenuItem(MCPlatformMenuRef p_menu, uindex_t p_where)
{
	MCPlatformClampMenuItemIndex(p_menu, p_where);
	
	NSMenuItem *t_item;
	t_item = [[NSMenuItem alloc] initWithTitle: @"" action: @selector(menuItemSelected:) keyEquivalent: @""];
	
	// Make sure we set the target of the action (to the delegate).
	[t_item setTarget: p_menu -> menu_delegate];
	
	// Insert the item in the menu.
	[p_menu -> menu insertItem: t_item atIndex: p_where];
}

void MCPlatformAddMenuSeparatorItem(MCPlatformMenuRef p_menu, uindex_t p_where)
{
	MCPlatformClampMenuItemIndex(p_menu, p_where);
	
	[p_menu -> menu insertItem: [NSMenuItem separatorItem] atIndex: p_where];
}
 
void MCPlatformRemoveMenuItem(MCPlatformMenuRef p_menu, uindex_t p_where)
{
	MCPlatformMapMenuItemIndex(p_menu, p_where);
	
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
		return;
	}
	
	r_index = [t_parent indexOfItemWithSubmenu: p_menu -> menu];
	r_parent = [(MCMenuDelegate *)[t_parent delegate] platformMenuRef];
	if (r_parent -> is_menubar)
		r_index -= 1;
}

void MCPlatformGetMenuItemProperty(MCPlatformMenuRef p_menu, uindex_t p_index, MCPlatformMenuItemProperty p_property, MCPlatformPropertyType p_type, void *r_value)
{
	MCPlatformMapMenuItemIndex(p_menu, p_index);
	
	NSMenuItem *t_item;
	t_item = [p_menu -> menu itemAtIndex: p_index];
	
	switch(p_property)
	{
		case kMCPlatformMenuItemPropertyTitle:
			*(char **)r_value = strdup([[t_item title] cStringUsingEncoding: NSUTF8StringEncoding]);
			break;
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
	MCPlatformMapMenuItemIndex(p_menu, p_index);
	
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
		{
			// COCOA-TODO: I'm not sure this will work - will need to investigate how embedded views
			//   deal with menu actions when they are implemented...
#if 0
			SEL t_selector;
			if (MCMacPlatformMapMenuItemActionToSelector(*(MCPlatformMenuItemAction *)p_value, t_selector))
				[t_item setAction: t_selector];
#endif
		}
		break;
		case kMCPlatformMenuItemPropertyAccelerator:
		{
			MCPlatformAccelerator t_accelerator;
			t_accelerator = *(MCPlatformAccelerator *)p_value;
			if (t_accelerator != 0)
			{
				NSUInteger t_modifiers;
				t_modifiers = 0;
				if ((t_accelerator & kMCPlatformAcceleratorWithShift) != 0)
					t_modifiers |= NSShiftKeyMask;
				if ((t_accelerator & kMCPlatformAcceleratorWithAlt) != 0)
					t_modifiers |= NSAlternateKeyMask;
				
				// COCOA-TODO: Abstract Command/Control switching.
				if ((t_accelerator & kMCPlatformAcceleratorWithCommand) != 0)
					t_modifiers |= NSControlKeyMask;
				if ((t_accelerator & kMCPlatformAcceleratorWithControl) != 0)
					t_modifiers |= NSCommandKeyMask;
				
				NSString *t_char;
				if (MCMacMapCodepointToNSString(t_accelerator & kMCPlatformAcceleratorKeyMask, t_char))
				{
					[t_item setKeyEquivalent: t_char];
					[t_item setKeyEquivalentModifierMask: t_modifiers];
					[t_char release];
				}
			}
			else
			{
				[t_item setKeyEquivalent: @""];
				[t_item setKeyEquivalentModifierMask: 0];
			}
		}
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
		{
			MCPlatformMenuItemHighlight t_highlight;
			t_highlight = *(MCPlatformMenuItemHighlight *)p_value;
			switch(t_highlight)
			{
			case kMCPlatformMenuItemHighlightNone:
				[t_item setState: NSOffState];
				break;
			case kMCPlatformMenuItemHighlightTick:
				[t_item setState: NSOnState];
				break;
			case kMCPlatformMenuItemHighlightDiamond:
				// COCOA-TODO: diamond
				[t_item setState: NSOnState];
				break;
			case kMCPlatformMenuItemHighlightBar:
				[t_item setState: NSMixedState];
				break;
			}
		}
		break;
	}
}

//////////

bool MCPlatformPopUpMenu(MCPlatformMenuRef p_menu, MCPlatformWindowRef p_window, MCPoint p_location, uindex_t p_item)
{
	NSMenu *t_menu;
	t_menu = p_menu -> menu;
	
	NSView *t_view;
	if (p_window != nil)
		t_view = ((MCMacPlatformWindow *)p_window) -> GetView();
	else
		t_view = nil;
	
	NSPoint t_location;
	if (t_view != nil)
	{
		MCPlatformMapPointFromScreenToWindow(p_window, p_location, p_location);
		((MCMacPlatformWindow *)p_window) -> MapMCPointToNSPoint(p_location, t_location);
	}
	else
		MCMacPlatformMapScreenMCPointToNSPoint(p_location, t_location);

	bool t_result;
	t_result = [t_menu popUpMenuPositioningItem: [t_menu itemAtIndex: p_item] atLocation: t_location inView: t_view];
	
	MCMacPlatformSyncMouseAfterTracking();
	
	return t_result;
}

//////////

static void MCPlatformStartUsingMenuAsMenubar(MCPlatformMenuRef p_menu)
{
	if (p_menu -> is_menubar)
		return;
	
	// COCOA-TODO: Finish app menu support.
	
	if (s_app_menu_delegate == nil)
		s_app_menu_delegate = [[com_runrev_livecode_MCAppMenuDelegate alloc] init];
	
	// Initialize the application menu (which is always index 0). The application
	// menu has structure:
	//
	//     About <app name>
	//     ----
	//     Preferences... / Apple-,
	//     ----
	//     Services
	//     ----
	//     Hide <app name> / Apple-H
	//     Hide Others     / Opt-Apple-H
	//     Show All
	//     ----
	//     Quit <app name> / Apple-Q
	//
	NSString *t_app_name;
	t_app_name = (NSString *)[[[NSBundle mainBundle] infoDictionary] objectForKey: (NSString *)kCFBundleNameKey];
	
	NSMenu *t_services_menu;
	t_services_menu = [[NSMenu alloc] initWithTitle: NSLocalizedString(@"Services", nil)];
	[NSApp setServicesMenu: t_services_menu];
						   
	NSMenu *t_app_menu;
	t_app_menu = [[NSMenu alloc] initWithTitle: t_app_name];
	
	[t_app_menu addItemWithTitle: [NSString stringWithFormat: NSLocalizedString(@"About %@", @"About {Application Name}"), t_app_name]
						  action: @selector(aboutMenuItemSelected:)
				   keyEquivalent: @""];
	[[t_app_menu itemAtIndex: 0] setTarget: s_app_menu_delegate];
	[t_app_menu addItem: [NSMenuItem separatorItem]];
	[t_app_menu addItemWithTitle: NSLocalizedString(@"Preferences...", nil)
						  action: @selector(preferencesMenuItemSelected:)
				   keyEquivalent: @","];
	[[t_app_menu itemAtIndex: 2] setTarget: s_app_menu_delegate];
	[t_app_menu addItem: [NSMenuItem separatorItem]];
	[t_app_menu addItemWithTitle: NSLocalizedString(@"Services", nil)
						  action: nil
				   keyEquivalent: @""];
	[[t_app_menu itemAtIndex: 4] setSubmenu: t_services_menu];
	[t_services_menu release];
	[t_app_menu addItem: [NSMenuItem separatorItem]];
	[t_app_menu addItemWithTitle: [NSString stringWithFormat: NSLocalizedString(@"Hide %@", @"Hide {Application Name}"), t_app_name]
						  action: @selector(hide:)
				   keyEquivalent: @"h"];
	[t_app_menu addItemWithTitle: NSLocalizedString(@"Hide Others", nil)
						  action: @selector(hideOtherApplications:)
				   keyEquivalent: @"h"];
	[[t_app_menu itemAtIndex: 7] setKeyEquivalentModifierMask: (NSAlternateKeyMask | NSCommandKeyMask)];
	[t_app_menu addItemWithTitle: NSLocalizedString(@"Show All", nil)
						  action: @selector(unhideAllApplications:)
				   keyEquivalent: @""];
	[t_app_menu addItem: [NSMenuItem separatorItem]];
	[t_app_menu addItemWithTitle: [NSString stringWithFormat: NSLocalizedString(@"Quit %@", @"Quit {Application Name}"), t_app_name]
						  action: @selector(terminate:)
				   keyEquivalent:@"q"];
	[t_app_menu setDelegate: s_app_menu_delegate];
	
	NSMenuItem *t_app_menu_item;
	t_app_menu_item = [[NSMenuItem alloc] initWithTitle: t_app_name action: nil keyEquivalent: @""];
	[t_app_menu_item setSubmenu: t_app_menu];
	[t_app_menu release];
	
	[p_menu -> menu insertItem: t_app_menu_item atIndex: 0];
	[t_app_menu_item release];
	
	p_menu -> is_menubar = true;
}

static void MCPlatformStopUsingMenuAsMenubar(MCPlatformMenuRef p_menu)
{
	if (!p_menu -> is_menubar)
		return;
	
	[p_menu -> menu removeItemAtIndex: 0];
	
	p_menu -> is_menubar = false;
}

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
	if (p_menu == s_menubar)
		return;
	
	MCPlatformMenuRef t_old_menubar;
	t_old_menubar = s_menubar;
	s_menubar = nil;
	
	if (p_menu != nil)
	{
		MCPlatformStartUsingMenuAsMenubar(p_menu);
		[NSApp setMainMenu: p_menu -> menu];
		MCPlatformRetainMenu(p_menu);
		s_menubar = p_menu;
	}
	else
		[NSApp setMainMenu: nil];
	
	if (t_old_menubar != nil)
	{
		MCPlatformStopUsingMenuAsMenubar(t_old_menubar);
		MCPlatformReleaseMenu(t_old_menubar);
	}
}

void MCPlatformGetMenubar(MCPlatformMenuRef& r_menu)
{
	r_menu = s_menubar;
}

////////////////////////////////////////////////////////////////////////////////

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

#include <Cocoa/Cocoa.h>

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

static uint32_t s_menu_select_lock = 0;
// SN-2014-11-06: [[ Bug 13836 ]] Stores whether a quit item got selected
static bool s_quit_selected = false;
// SN-2014-11-10: [[ Bug 13836 ]] Keeps the track about the open items in the menu bar.
static uint32_t s_open_menubar_items = 0;

// SN-2015-11-02: [[ Bug 16218 ]] We can't trust popUpMenuPositioningItem on
//  returning whether an item has been selected.
static bool s_menu_item_selected = false;


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
    
    // If the quit item in this menu has an accelerator, this is true.
    // (Cocoa seems to 'hide' the quit menu item accelerator for some inexplicable
    // reason - it returns 'empty').
    NSMenuItem* quit_item;
    
    // SN-2014-11-06: [[ Bu 13940 ]] Add a flag for the presence of a Preferences shortcut
    //  to allow the menu item to be disabled.
    NSMenuItem* preferences_item;
    
    NSMenuItem* about_item;
};

////////////////////////////////////////////////////////////////////////////////

// The menuref currently set as the menubar.
static MCPlatformMenuRef s_menubar = nil;

// The delegate for the app menu.
static com_runrev_livecode_MCAppMenuDelegate *s_app_menu_delegate = nil;

////////////////////////////////////////////////////////////////////////////////

enum MCShadowedItemTags
{
    kMCShadowedItemAbout = 'abou',
    kMCShadowedItemPreferences = 'pref',
    kMCShadowedItemQuit = 'quit'
};

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

- (void)hideShadowedMenuItem:(NSInteger)tag menu:(NSMenu *)menu
{
	NSInteger t_index;
	t_index = [menu indexOfItemWithTag: tag];
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
    // SN-2014-11-10: [[ Bug 13836 ]] Only allow the menu to refresh the whole menubar if
    //  it is the first click on the menubar (otherwise, clicking and sliding would refresh
    //  the menubar each the item hovered changes)...
    // SN-2014-11-06: [[ Bug 13849 ]] ...since rebuilding a menubar sends mouseDown to the menubar
    if ([menu supermenu] == nil || !s_open_menubar_items)
    {
        MCPlatformCallbackSendMenuUpdate(m_menu);
    }
    
    // MW-2014-10-29: [[ Bug 13848 ]] Only do the item hiding if this is part of a menubar
    //   (not a popup menu).
    if ([menu supermenu] != nil)
    {
        [self hideShadowedMenuItem: kMCShadowedItemAbout menu: menu];
        [self hideShadowedMenuItem: kMCShadowedItemPreferences menu: menu];
        [self hideShadowedMenuItem: kMCShadowedItemQuit menu: menu];
    }
}

- (void)menuItemSelected: (id)sender
{
    if (MCMacPlatformApplicationPseudoModalFor() != nil)
        return;
    
    NSMenuItem *t_item;
    t_item = (NSMenuItem *)sender;
    
    // MW-2014-10-22: [[ Bug 13510 ]] As Cocoa hides Cmd-Q for some reason as a key equivalent
    //   we mark the menu as having an accelerator for quit if one was specified. If no accelerator
    //   was specified, we handle Cmd-Q as if it weren't an accelerator but was a select.
    //   (This is for the case where the 'Exit' menu item has no accelerator, but Cocoa requires
    //    said accelerator for conformance).
    bool t_quit_accelerator_present;
    t_quit_accelerator_present = false;
    if ([[t_item keyEquivalent] isEqualToString: @"q"])
        t_quit_accelerator_present = [(com_runrev_livecode_MCMenuDelegate *)[[t_item menu] delegate] platformMenuRef] -> quit_item != nil;
    
	if (s_menu_select_lock == 0 || t_quit_accelerator_present)
    {
		MCPlatformCallbackSendMenuSelect(m_menu, [[t_item menu] indexOfItem: t_item]);
        s_menu_item_selected = true;
    }
    
    // SN-2014-11-06: [[ Bug 13836 ]] s_menu_select_occured was not used.
}

- (BOOL)validateMenuItem: (NSMenuItem *)item
{
	return [item isEnabled];
}

// MW-2014-04-30: [[ Bug 12329 ]] We want our menu items to still be activatible in modal dialogs.
- (BOOL)worksWhenModal
{
    return YES;
}

//////////

- (BOOL)menuHasKeyEquivalent:(NSMenu *)menu forEvent:(NSEvent *)event target:(id *)target action:(SEL *)action
{
    // SN-2014-11-06: [[ Bug 13836 ]] We don't update the menubar everytime an accelerator is used
	return NO;
}

//////////

// SN-2014-11-10: [[ Bug 13836 ]] We want to know how many items are open in the menubar,
//  When sliding after having clicked in the menubar, the new item receives menuWillOpen
//  before the other one receives menuWillClose.
- (void)menuWillOpen:(NSMenu *)menu
{
    if ([s_menubar -> menu isEqualTo: [menu supermenu]])
        s_open_menubar_items++;
}

- (void)menuDidClose:(NSMenu *)menu
{
    if ([s_menubar -> menu isEqualTo: [menu supermenu]])
        s_open_menubar_items--;
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
    {
        // MW-2014-10-29: [[ Bug 13847 ]] Dispatch default accelerators from app menu.
        uint32_t t_old_menu_select_lock;
        t_old_menu_select_lock = s_menu_select_lock;
        s_menu_select_lock = 0;
		[(MCMenuDelegate *)[[t_shadow menu] delegate] menuItemSelected: t_shadow];
        s_menu_select_lock = t_old_menu_select_lock;
    }
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

- (void)quitMenuItemSelected: (id)sender
{
    // SN-2014-12-16: [[ Bug 14185 ]] Only flag the the quitting state (that's the only state
    //  which causes issues.
    s_quit_selected = true;
	[self shadowedMenuItemSelected: @"Quit"];
}

// SN-2014-11-06: [[ Bug 13940 ]] The user asked to quit - and no accelerator was added for this,
//  so we quit!
- (void)quitApplicationSelected: (id)sender
{
    if (MCMacPlatformApplicationPseudoModalFor() != nil)
        return;
    
    // IM-2015-11-13: [[ Bug 16288 ]] Send shutdown request rather than terminating immediately
	//    to allow shutdown in orderly fashion.
	bool t_shutdown;
	MCPlatformCallbackSendApplicationShutdownRequest(t_shutdown);
}

// SN-2014-11-10: [[ Bug 13836 ]] The menubar should be updated if left item is clicked
- (void)menuNeedsUpdate: (NSMenu *)menu
{
    if (!s_open_menubar_items)
        MCPlatformCallbackSendMenuUpdate(s_menubar);
}

- (BOOL)validateMenuItem: (NSMenuItem *)item
{
	return [item isEnabled];
}

//////////

// Putting this method here means we don't get an excessive amount of
// menuUpdate messages after each key-press...
- (BOOL)menuHasKeyEquivalent:(NSMenu *)menu forEvent:(NSEvent *)event target:(id *)target action:(SEL *)action
{
    // SN-2014-11-06: [[ Bug 13836 ]] menuHasKeyEquivalent is called for each submenu/item of the menubar.
    //  We don't want to rebuild all of this each time an accelerator is used.
//    [self menuNeedsUpdate: menu];
    return NO;
}

//////////

- (void)menuWillOpen:(NSMenu *)menu
{
    s_open_menubar_items++;
}

- (void)menuDidClose:(NSMenu *)menu
{
    s_open_menubar_items--;
}

@end

////////////////////////////////////////////////////////////////////////////////

// We subclass NSMenu so that for any LiveCode menus, keyEquivalents don't
// result in 'menuSelect' messages, but instead propagate a keyDown/keyUp
// message which the engine handles.

@interface com_runrev_livecode_MCMenuHandlingKeys: NSMenu

- (BOOL)performKeyEquivalent: (NSEvent *)event;

@end

@compatibility_alias MCMenuHandlingKeys com_runrev_livecode_MCMenuHandlingKeys;

// SN-2014-12-16: [[ Bug 14185 ]] Functions to save and restore the selected quit state
uint32_t s_quitting_state_count = 0;
uint8_t *s_quitting_states;

void MCMacPlatformSaveQuittingState()
{
    MCMemoryReallocate(s_quitting_states, s_quitting_state_count + 1, s_quitting_states);
    s_quitting_states[s_quitting_state_count] = s_quit_selected;
    
    ++s_quitting_state_count;
    s_quit_selected = false;
}

void MCMacPlatformPopQuittingState()
{
    s_quit_selected = s_quitting_states[s_quitting_state_count - 1];
    
    --s_quitting_state_count;
    MCMemoryReallocate(s_quitting_states, s_quitting_state_count, s_quitting_states);
}

// SN-2014-11-06: [[ Bug 13836 ]] Returns whether the last item clicked was a shadowed item.
// SN-2014-12-16: [[ Bug 14185 ]] Name changed as it only returns whether a 'Quit' item
// has been selected.
bool MCMacPlatformIsInQuittingState(void)
{
	bool t_occured;
	t_occured = s_quit_selected;
	s_quit_selected = false;
	return t_occured;
}

void MCMacPlatformLockMenuSelect(void)
{
	s_menu_select_lock += 1;
}

void MCMacPlatformUnlockMenuSelect(void)
{
	s_menu_select_lock -= 1;
}

@implementation com_runrev_livecode_MCMenuHandlingKeys

- (BOOL)performKeyEquivalent: (NSEvent *)event
{
    // SN-2014-12-05: [[ Bug 14019 ]] Forbid any Cmd-key reaction when the target is the colour picker
    // (that colour picker is modal after all)
    if ([[[event window] delegate] isKindOfClass: [MCColorPanelDelegate class]])
        return NO;

	// If the event is not targetted at one of our windows, we just let things
	// flow as normal.
	if (![[[event window] delegate] isKindOfClass: [MCWindowDelegate class]])
        return [super performKeyEquivalent: event];
    
    // SN-2014-12-16; [[ Bug 14185 ]] We want to store the previous state, as when using answer
    //  for example, we still want the key event to be processed.
    MCMacPlatformSaveQuittingState();
    
	// Otherwise, we lock menuSelect firing, and propagate a keydown/keyup.
	BOOL t_key_equiv;
	MCMacPlatformLockMenuSelect();
	t_key_equiv = [super performKeyEquivalent: event];
	MCMacPlatformUnlockMenuSelect();

    BOOL t_force_keypress;
    t_force_keypress = NO;

    // AL-2014-09-16: [[ Bug 13372 ]]  Make sure tab + modifier and esc + modifier key combinations
    //  that are reserved by Cocoa trigger keypress events for the engine to handle.
    if (!t_key_equiv)
    {
        MCPlatformKeyCode t_keycode;
        MCMacPlatformMapKeyCode([event keyCode], [event modifierFlags], t_keycode);
        t_force_keypress = t_keycode == kMCPlatformKeyCodeTab || t_keycode == kMCPlatformKeyCodeEscape;
    }
    
    // SN-2014-11-06: [[ Bug 13510 ]] Ensure that we don't get further, if a shadow item was selected
    //  and that Cocoa will stop looking for key equivalent amongst the application's menus
    //  Calling MCMacPlatformWasShadowItemSelected here ensure that the state is reset for each event.
    // SN-2014-12-16; [[ Bug 14185 ]] Name changed as we only check whether a 'Quit' item was selected.
    if (MCMacPlatformIsInQuittingState())
    {
        MCMacPlatformPopQuittingState();
        return YES;
    }
    
    // SN-2014-12-16: [[ Bug 14185 ]] Pop the last state saved.
    MCMacPlatformPopQuittingState();
    
    // MW-2014-04-10: [[ Bug 12047 ]] If it was found as a key equivalent dispatch
    //   a keypress so the engine can handle it. Otherwise we return NO and the
    //   event is handled normally.
    if (t_key_equiv || t_force_keypress)
    {
        MCMacPlatformWindow *t_window = [(MCWindowDelegate *)[[event window] delegate] platformWindow];
        
        // MW-2014-04-24: [[ Bug 12284 ]] If we get here then it could have come from
        //   an 'external' view and thus the event will not have gone through any
        //   of our view code - so make sure modifiers are up to date (otherwise command
        //   key shortcuts don't work!).
        MCMacPlatformHandleModifiersChanged(MCMacPlatformMapNSModifiersToModifiers([event modifierFlags]));
        
        // SN-2014-11-06: [[ Bug 13836 ]] We don't want to recreate every menu existing for each keyEvent
        // MW-2014-10-29: [[ Bug 13847 ]] Make sure we only update menus once per accelerator.
//        if (s_update_menubar_menus)
//        {
//            s_update_menubar_menus = false;
//            for(int i = 1; i < [s_menubar -> menu numberOfItems]; i++)
//            {
//                NSMenu *t_submenu;
//                t_submenu = [[s_menubar -> menu itemAtIndex: i] submenu];
//                if (t_submenu != nil)
//                    MCPlatformCallbackSendMenuUpdate([(MCMenuDelegate *)[t_submenu delegate] platformMenuRef]);
//            }
//        }
        
        [t_window -> GetView() handleKeyPress: event isDown: YES];
        [t_window -> GetView() handleKeyPress: event isDown: NO];
        return YES;
    }
    
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
	
	t_menu -> menu = [[MCMenuHandlingKeys alloc] initWithTitle: @""];
	t_menu -> menu_delegate = [[MCMenuDelegate alloc] initWithPlatformMenuRef: t_menu];
	[t_menu -> menu setDelegate: t_menu -> menu_delegate];
    t_menu -> is_menubar = false;
	
	// Turn on auto-enablement - this allows dialogs to control the enablement
	// of items with appropriate tag.
	[t_menu -> menu setAutoenablesItems: YES];
    
    // SN-2014-11-06: [[ Bug 13940 ]] Initialises the accelerator presence flag.
    t_menu -> quit_item = nil;
    t_menu -> preferences_item = nil;
    t_menu -> about_item = nil;
	
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
    [p_menu -> about_item release];
    [p_menu -> preferences_item release];
    [p_menu -> quit_item release];
	MCMemoryDelete(p_menu);
}

void MCPlatformSetMenuTitle(MCPlatformMenuRef p_menu, MCStringRef p_title)
{
    MCAutoStringRefAsCFString t_cf_string;
    /* UNCHECKED */ t_cf_string . Lock(p_title);
    [p_menu -> menu setTitle: (NSString*)*t_cf_string];
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
	
	[t_item release];
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
			/* UNCHECKED */ MCStringCreateWithCFString((CFStringRef)[t_item title], *(MCStringRef*)r_value);
			break;
		case kMCPlatformMenuItemPropertyTag:
			/* UNCHECKED */ MCStringCreateWithCFString((CFStringRef)[t_item representedObject], *(MCStringRef*)r_value);
			break;
        case kMCPlatformMenuItemPropertySubmenu:
        {
            NSMenu *t_current_submenu = [t_item submenu];
            
            MCPlatformMenuRef t_current_submenu_ref = nil;
            if (t_current_submenu != nil)
            {
                t_current_submenu_ref = [(MCMenuDelegate *)[t_current_submenu delegate] platformMenuRef];
            }
            *(MCPlatformMenuRef *)r_value = t_current_submenu_ref;
        }
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
    
    MCAutoStringRefAsCFString t_cf_string;
	
	switch(p_property)
	{
		case kMCPlatformMenuItemPropertyTitle:
            /* UNCHECKED */ t_cf_string . Lock(*(MCStringRef*)p_value);
			[t_item setTitle: (NSString*)*t_cf_string];
			break;	
		case kMCPlatformMenuItemPropertyTag:
            /* UNCHECKED */ t_cf_string . Lock(*(MCStringRef*)p_value);
			[t_item setRepresentedObject: (NSString*)*t_cf_string];
			break;
		case kMCPlatformMenuItemPropertyAction:
		{
			MCPlatformMenuItemAction t_action;
			t_action = *(MCPlatformMenuItemAction *)p_value;
			
			if (t_action == kMCPlatformMenuItemActionNone)
			{
				[t_item setAction: @selector(menuItemSelected:)];
				[t_item setTarget: p_menu -> menu_delegate];
			}
			else
            {
                // SN-2014-11-06: [[ Bug 13940 ]] Update the parent - if any - to know that his submenu has
                //  a Quit or a Preferences accelerator
                NSMenu *t_supermenu;
                t_supermenu = [p_menu -> menu supermenu];
                
                if (t_supermenu != nil)
                {
                    MCPlatformMenuRef t_supermenu_ref;
                    t_supermenu_ref = [(MCMenuDelegate *)[t_supermenu delegate] platformMenuRef];
                    if (t_action == kMCPlatformMenuItemActionQuit)
                    {
                        [t_item setTag:kMCShadowedItemQuit];
                        t_supermenu_ref -> quit_item = [t_item retain];
                    }
                    else if (t_action == kMCPlatformMenuItemActionPreferences)
                    {
                        [t_item setTag:kMCShadowedItemPreferences];
                        t_supermenu_ref -> preferences_item = [t_item retain];
                    }
                    else if (t_action == kMCPlatformMenuItemActionAbout)
                    {
                        [t_item setTag: kMCShadowedItemAbout];
                        t_supermenu_ref -> about_item = [t_item retain];
                    }
                }
                
				SEL t_selector;
				if (MCMacPlatformMapMenuItemActionToSelector(t_action, t_selector))
				{
					[t_item setAction: t_selector];
					[t_item setTarget: nil];
				}
			}
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
			
            // PM-2015-02-09: [[ Bug 14521 ]] No action since menu item has submenus
            // SN-2015-01-12: [[ Bug 14346 ]] Menu items with a submenu should not be selectable
            [t_item setAction: nil];
            [t_item setTarget: nil];
            
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
			
		case kMCPlatformMenuItemPropertyUnknown:
			MCUnreachable();
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
    
    // MW-2014-07-29: [[ Bug 12990 ]] If item is UINDEX_MAX then don't specify an item, thus preventing
    //   one from being highlighted.
    
    // SN-2015-11-02: [[ Bug 16218 ]] popUpMenuPositioningItem always returns
    // true if the menu is open by keeping the mouse down, even if the mouse is
    // released outside of the menu list.
    // We will set s_menu_item_selected in menuItemSelected if selection occurs.
    s_menu_item_selected = false;
	[t_menu popUpMenuPositioningItem: p_item == UINDEX_MAX ? nil : [t_menu itemAtIndex: p_item] atLocation: t_location inView: t_view];
	
	MCMacPlatformSyncMouseAfterTracking();
	
	return s_menu_item_selected;
}

//////////

static void MCPlatformStartUsingMenuAsMenubar(MCPlatformMenuRef p_menu)
{
	if (p_menu -> is_menubar)
		return;
	
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
    
    // SN-2014-10-14: [[ Bug 13662 ]] We use the process name if the app is not in a bundle
    if (t_app_name == nil)
        t_app_name = [[NSProcessInfo processInfo] processName];
	
    
    // SN-2014-12-09: [[ Bug 14168 ]] Update to use actually localised strings as menu items
    // for the Application menu.
    // The menu items Preferences, About and Quit are again auto-translated.
    
	NSMenu *t_services_menu;
	t_services_menu = [[NSMenu alloc] initWithTitle: NSLocalizedStringFromTable(@"appMenu.services", @"Localisation", @"Services")];
	[NSApp setServicesMenu: t_services_menu];
						   
	NSMenu *t_app_menu;
	t_app_menu = [[NSMenu alloc] initWithTitle: t_app_name];
	
    NSString *t_about_string;
    t_about_string = [NSString stringWithFormat: NSLocalizedStringFromTable(@"appMenu.about", @"Localisation", @"Format string such as About %@"), t_app_name];
    
	[t_app_menu addItemWithTitle: t_about_string
						  action: @selector(aboutMenuItemSelected:)
				   keyEquivalent: @""];
    if (p_menu -> about_item == nil)
        [[t_app_menu itemAtIndex: 0] setEnabled:NO];
    [[t_app_menu itemAtIndex: 0] setTarget: s_app_menu_delegate];
    [[t_app_menu itemAtIndex: 0] setTag: kMCShadowedItemAbout];
        
    [t_app_menu addItem: [NSMenuItem separatorItem]];
    
    NSString *t_preferences_string;
    t_preferences_string = NSLocalizedStringFromTable(@"appMenu.preferences", @"Localisation", @"Preferences");
    
	[t_app_menu addItemWithTitle: t_preferences_string
						  action: @selector(preferencesMenuItemSelected:)
				   keyEquivalent: @","];
    // SN-2014-11-06: [[ Bug 13940 ]] Only enable the Preference menu if the shortcut exists in the menubar
    [[t_app_menu itemAtIndex: 2] setTarget: s_app_menu_delegate];
    if (p_menu -> preferences_item == nil)
        [[t_app_menu itemAtIndex: 2] setEnabled: NO];
    
    [[t_app_menu itemAtIndex: 2] setTag: kMCShadowedItemPreferences];
    
    [t_app_menu addItem: [NSMenuItem separatorItem]];
    [t_app_menu addItemWithTitle: NSLocalizedStringFromTable(@"appMenu.services", @"Localisation", @"Services")
						  action: nil
				   keyEquivalent: @""];
	[[t_app_menu itemAtIndex: 4] setSubmenu: t_services_menu];
	[t_services_menu release];
    [t_app_menu addItem: [NSMenuItem separatorItem]];
	[t_app_menu addItemWithTitle: [NSString stringWithFormat: NSLocalizedStringFromTable(@"appMenu.hide", @"Localisation", @"Format string such as Hide %@"), t_app_name]
						  action: @selector(hide:)
                   keyEquivalent: @"h"];
    [t_app_menu addItemWithTitle: NSLocalizedStringFromTable(@"appMenu.hideOthers", @"Localisation", @"Hide Others")
						  action: @selector(hideOtherApplications:)
				   keyEquivalent: @"h"];
	[[t_app_menu itemAtIndex: 7] setKeyEquivalentModifierMask: (NSAlternateKeyMask | NSCommandKeyMask)];
	[t_app_menu addItemWithTitle: NSLocalizedStringFromTable(@"appMenu.showAll", @"Localisation", @"Show All")
						  action: @selector(unhideAllApplications:)
				   keyEquivalent: @""];
	[t_app_menu addItem: [NSMenuItem separatorItem]];
    
    NSString *t_quit_string;
    t_quit_string = [NSString stringWithFormat: NSLocalizedStringFromTable(@"appMenu.quit", @"Localisation", @"Format string such as Quit %@"), t_app_name];
    
	[t_app_menu addItemWithTitle: t_quit_string
						  action: @selector(quitMenuItemSelected:)
				   keyEquivalent:@"q"];
    // SN-2014-11-06: [[ Bug 13940 ]] In case there is no Quit shortcut in this menubar,
    //  the action will simply be to close the application.
    if (p_menu -> quit_item != nil)
        [[t_app_menu itemAtIndex: 10] setAction:@selector(quitMenuItemSelected:)];
    else
        [[t_app_menu itemAtIndex: 10] setAction:@selector(quitApplicationSelected:)];
    
    [[t_app_menu itemAtIndex: 10] setTarget: s_app_menu_delegate];
    [[t_app_menu itemAtIndex: 10] setTag: kMCShadowedItemQuit];
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
    [NSMenu setMenuBarVisible:YES];
}

void MCPlatformHideMenubar(void)
{
    [NSMenu setMenuBarVisible:NO];
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

//////////

static MCPlatformMenuRef s_icon_menu = nil;

NSMenu *MCMacPlatformGetIconMenu(void)
{
	if (s_icon_menu != nil)
		MCPlatformCallbackSendMenuUpdate(s_icon_menu);
	
	if (s_icon_menu != nil)
		return s_icon_menu -> menu;
	
	return nil;
}

void MCPlatformSetIconMenu(MCPlatformMenuRef p_menu)
{
	if (s_icon_menu != nil)
		MCPlatformReleaseMenu(s_icon_menu);
	s_icon_menu = p_menu;
	if (s_icon_menu != nil)
		MCPlatformRetainMenu(s_icon_menu);
}

////////////////////////////////////////////////////////////////////////////////

static struct { MCPlatformMenuItemAction action; SEL selector; } s_menu_item_action_map[] =
{
	{ kMCPlatformMenuItemActionNone, nil },
	{ kMCPlatformMenuItemActionUndo, @selector(undo:) },
	{ kMCPlatformMenuItemActionRedo, @selector(redo:) },
	{ kMCPlatformMenuItemActionCut, @selector(cut:) },
	{ kMCPlatformMenuItemActionCopy, @selector(copy:) },
	{ kMCPlatformMenuItemActionPaste, @selector(paste:) },
	{ kMCPlatformMenuItemActionClear, @selector(delete:) },
	{ kMCPlatformMenuItemActionSelectAll, @selector(selectAll:) },
};

bool MCMacPlatformMapMenuItemActionToSelector(MCPlatformMenuItemAction action, SEL& r_selector)
{
	for(uindex_t i = 0; i < sizeof(s_menu_item_action_map) / sizeof(s_menu_item_action_map[0]); i++)
		if (action == s_menu_item_action_map[i] . action)
		{
			r_selector = s_menu_item_action_map[i] . selector;
			return true;
		}
	
	return false;
}

////////////////////////////////////////////////////////////////////////////////

// MW-2014-04-11: [[ Bug 12068 ]] On startup setup an empty default menubar
//   so that all apps get Quit / About items.
bool MCPlatformInitializeMenu(void)
{
    MCPlatformMenuRef t_menubar;
    MCPlatformCreateMenu(t_menubar);
    MCPlatformSetMenubar(t_menubar);
    MCPlatformReleaseMenu(t_menubar);
    return true;
}

void MCPlatformFinalizeMenu(void)
{

}

////////////////////////////////////////////////////////////////////////////////

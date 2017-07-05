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

#include "mac-platform.h"

#include <Carbon/Carbon.h>

////////////////////////////////////////////////////////////////////////////////

// NOTE:
// At the moment we implement the menu directly for cocoa as Mac is the only
// platform that uses native menus - we will probably want to abstract in a
// similar way to the MCPlatformWindow at a later date though.

////////////////////////////////////////////////////////////////////////////////

enum MCShadowedItemTags
{
    kMCShadowedItemAbout = 'abou',
    kMCShadowedItemPreferences = 'pref',
    kMCShadowedItemQuit = 'quit'
};

////////////////////////////////////////////////////////////////////////////////

@implementation com_runrev_livecode_MCMenuDelegate

- (id)initWithPlatformMenuRef: (MCMacPlatformMenu*)p_menu_ref
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

- (MCMacPlatformMenu *)platformMenuRef
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
    if ([menu supermenu] == nil || m_menu -> GetOpenMenuItems() == 0)
    {
        m_menu -> GetPlatform() -> SendMenuUpdate(m_menu);
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
    
    MCMacPlatformCore * t_platform = static_cast<MCMacPlatformCore *>(m_menu -> GetPlatform());
    if (t_platform -> ApplicationPseudoModalFor() != nil)
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
        t_quit_accelerator_present = [(com_runrev_livecode_MCMenuDelegate *)[[t_item menu] delegate] platformMenuRef] -> GetQuitItem() != nil;
    
	if (t_platform -> GetMenuSelectLock() == 0 || t_quit_accelerator_present)
    {
		t_platform -> SendMenuSelect(m_menu, [[t_item menu] indexOfItem: t_item]);
        m_menu -> SetMenuItemSelected(true);
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
    if ([m_menu -> GetMenu() isEqualTo: [menu supermenu]])
        m_menu -> SetOpenMenuItems(m_menu -> GetOpenMenuItems() + 1);
}

- (void)menuDidClose:(NSMenu *)menu
{
    if ([m_menu -> GetMenu() isEqualTo: [menu supermenu]])
        m_menu -> SetOpenMenuItems(m_menu -> GetOpenMenuItems() - 1);
}

@end

@implementation com_runrev_livecode_MCAppMenuDelegate

- (id)initWithPlatform:(MCMacPlatformCore *)platform
{
	self = [super init];
	if (self == nil)
		return nil;
	
    m_platform = platform;
    
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
        t_old_menu_select_lock = m_platform -> GetMenuSelectLock();
        m_platform -> SetMenuSelectLock(0);
		[(MCMenuDelegate *)[[t_shadow menu] delegate] menuItemSelected: t_shadow];
        m_platform -> SetMenuSelectLock(t_old_menu_select_lock);    }
}

- (NSMenuItem *)findShadowedMenuItem: (NSString *)tag
{
	NSMenu *t_menubar;
	t_menubar =  static_cast<MCMacPlatformMenu *>(m_platform -> GetMenubar()) -> GetMenu();
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
    m_platform -> SetQuitSelected(true);
	[self shadowedMenuItemSelected: @"Quit"];
}

// SN-2014-11-06: [[ Bug 13940 ]] The user asked to quit - and no accelerator was added for this,
//  so we quit!
- (void)quitApplicationSelected: (id)sender
{
    if (m_platform -> ApplicationPseudoModalFor() != nil)
        return;
    
    // IM-2015-11-13: [[ Bug 16288 ]] Send shutdown request rather than terminating immediately
	//    to allow shutdown in orderly fashion.
	bool t_shutdown;
	m_platform -> SendApplicationShutdownRequest(t_shutdown);
}

// SN-2014-11-10: [[ Bug 13836 ]] The menubar should be updated if left item is clicked
- (void)menuNeedsUpdate: (NSMenu *)menu
{
    if (static_cast<MCMacPlatformMenu*>(m_platform -> GetMenubar()) -> GetOpenMenuItems() != 0)
       m_platform -> SendMenuUpdate(m_platform -> GetMenubar());
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
    MCMacPlatformMenu * t_menubar = static_cast<MCMacPlatformMenu*>(m_platform -> GetMenubar());
    t_menubar -> SetOpenMenuItems(t_menubar -> GetOpenMenuItems() + 1);
}

- (void)menuDidClose:(NSMenu *)menu
{
    MCMacPlatformMenu * t_menubar = static_cast<MCMacPlatformMenu*>(m_platform -> GetMenubar());
    t_menubar -> SetOpenMenuItems(t_menubar -> GetOpenMenuItems() - 1);
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
void MCMacPlatformCore::SaveQuittingState()
{
    MCMemoryReallocate(m_quitting_states, m_quitting_state_count + 1, m_quitting_states);
    m_quitting_states[m_quitting_state_count] = m_quit_selected;
    
    ++m_quitting_state_count;
    m_quit_selected = false;
}

void MCMacPlatformCore::PopQuittingState()
{
    m_quit_selected = m_quitting_states[m_quitting_state_count - 1];
    
    --m_quitting_state_count;
    MCMemoryReallocate(m_quitting_states, m_quitting_state_count, m_quitting_states);
}

// SN-2014-11-06: [[ Bug 13836 ]] Returns whether the last item clicked was a shadowed item.
// SN-2014-12-16: [[ Bug 14185 ]] Name changed as it only returns whether a 'Quit' item
// has been selected.
bool MCMacPlatformCore::IsInQuittingState(void)
{
	bool t_occured;
	t_occured = m_quit_selected;
	m_quit_selected = false;
	return t_occured;
}

void MCMacPlatformCore::LockMenuSelect(void)
{
	m_menu_select_lock += 1;
}

void MCMacPlatformCore::UnlockMenuSelect(void)
{
	m_menu_select_lock -= 1;
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
    
    MCMacPlatformCore * t_platform =
        static_cast<MCMacPlatformCore *>([((MCMenuDelegate*)[self delegate]) platformMenuRef] -> GetPlatform());
    
    // SN-2014-12-16; [[ Bug 14185 ]] We want to store the previous state, as when using answer
    //  for example, we still want the key event to be processed.
    t_platform -> SaveQuittingState();
    
	// Otherwise, we lock menuSelect firing, and propagate a keydown/keyup.
	BOOL t_key_equiv;
	t_platform -> LockMenuSelect();
	t_key_equiv = [super performKeyEquivalent: event];
	t_platform -> UnlockMenuSelect();

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
    if (t_platform -> IsInQuittingState())
    {
        t_platform -> PopQuittingState();
        return YES;
    }
    
    // SN-2014-12-16: [[ Bug 14185 ]] Pop the last state saved.
    t_platform -> PopQuittingState();
    
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
        t_platform -> HandleModifiersChanged(MCMacPlatformMapNSModifiersToModifiers([event modifierFlags]));
        
        // SN-2014-11-06: [[ Bug 13836 ]] We don't want to recreate every menu existing for each keyEvent
        // MW-2014-10-29: [[ Bug 13847 ]] Make sure we only update menus once per accelerator.
//        if (s_update_menubar_menus)
//        {
//            s_update_menubar_menus = false;
//            for(int i = 1; i < [m_menubar -> menu numberOfItems]; i++)
//            {
//                NSMenu *t_submenu;
//                t_submenu = [[m_menubar -> menu itemAtIndex: i] submenu];
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


MCMacPlatformMenu::MCMacPlatformMenu(MCPlatformCoreRef p_platform)
{
    m_platform = p_platform;
    m_callback = p_platform -> GetCallback();
    
    menu = [[MCMenuHandlingKeys alloc] initWithTitle: @""];
    menu_delegate = [[MCMenuDelegate alloc] initWithPlatformMenuRef: this];
    [menu setDelegate: menu_delegate];
    
    // Turn on auto-enablement - this allows dialogs to control the enablement
    // of items with appropriate tag.
    [menu setAutoenablesItems: YES];
}

MCMacPlatformMenu::~MCMacPlatformMenu(void)
{
    // Release any submenus.
    for(uindex_t i = 0; i < [menu numberOfItems]; i++)
        DestroyMenuItem(i);
    
    [menu release];
    [menu_delegate release];
    [about_item release];
    [preferences_item release];
    [quit_item release];
    
    if (m_app_menu_delegate != nil)
        [m_app_menu_delegate release];
}

// Helper method that frees anything associated with an NSMenuItem in an NSMenu
// (at the moment, this is the submenu).
void MCMacPlatformMenu::DestroyMenuItem(uindex_t p_index)
{
	NSMenuItem *t_item;
	t_item = [menu itemAtIndex: p_index];
	
	NSMenu *t_submenu;
	t_submenu = [t_item submenu];
	if (t_submenu == nil)
		return;
	
	MCPlatformMenuRef t_submenu_ref;
	t_submenu_ref = [(MCMenuDelegate *)[t_submenu delegate] platformMenuRef];
	
	// Update the submenu pointer (so we don't have any dangling
	// refs).
    [t_item setSubmenu: nil];
}

// Map the incoming index to the internal menu item index (taking into account
// whether the menu is the menubar).
void MCMacPlatformMenu::MapMenuItemIndex(uindex_t& x_index)
{
	// If the menu is the menubar, adjust for the application menu (always first).
	if (is_menubar)
		x_index += 1;
}

// Clamp the incoming index to an internal menu item insertion index (taking into
// account whether the menu is the menubar).
void MCMacPlatformMenu::ClampMenuItemIndex(uindex_t& x_index)
{
	if (x_index < UINDEX_MAX && is_menubar)
		x_index += 1;
	x_index = MCMin((unsigned)[menu numberOfItems], x_index);
}

void MCMacPlatformMenu::SetTitle( MCStringRef p_title)
{
    MCPlatformAutoStringRefAsCFString t_cf_string(m_callback);
    /* UNCHECKED */ t_cf_string . Lock(p_title);
    [menu setTitle: (NSString*)*t_cf_string];
}

uindex_t MCMacPlatformMenu::CountItems(void)
{
	uindex_t t_count = [menu numberOfItems];
	
	// If the menu is the menubar, then we've inserted the application menu.
	if (is_menubar)
		t_count -= 1;
    
    return t_count;
}

void MCMacPlatformMenu::AddItem(uindex_t p_where)
{
	ClampMenuItemIndex(p_where);
	
	NSMenuItem *t_item;
	t_item = [[NSMenuItem alloc] initWithTitle: @"" action: @selector(menuItemSelected:) keyEquivalent: @""];
	
	// Make sure we set the target of the action (to the delegate).
	[t_item setTarget: menu_delegate];
	
	// Insert the item in the menu.
	[menu insertItem: t_item atIndex: p_where];
	
	[t_item release];
}

void MCMacPlatformMenu::AddSeparatorItem(uindex_t p_where)
{
	ClampMenuItemIndex(p_where);
	
    [menu insertItem: [NSMenuItem separatorItem] atIndex: p_where];
}
 
void MCMacPlatformMenu::RemoveItem(uindex_t p_where)
{
	MapMenuItemIndex(p_where);
	
	DestroyMenuItem(p_where);
	[menu removeItemAtIndex: p_where];
}

void MCMacPlatformMenu::RemoveAllItems(void)
{
	for(uindex_t i = 0; i < [menu numberOfItems]; i++)
		DestroyMenuItem(i);
	[menu removeAllItems];
}

void MCMacPlatformMenu::GetParent(MCPlatformMenuRef& r_parent, uindex_t& r_index)
{
	NSMenu *t_parent;
	t_parent = [menu supermenu];
	if (t_parent == nil)
	{
		r_parent = nil;
		r_index = 0;
		return;
	}
	
	r_index = [t_parent indexOfItemWithSubmenu: menu];
	r_parent = [(MCMenuDelegate *)[t_parent delegate] platformMenuRef];
	if (static_cast<MCMacPlatformMenu *>(r_parent) -> is_menubar)
		r_index -= 1;
}

void MCMacPlatformMenu::GetItemProperty(uindex_t p_index, MCPlatformMenuItemProperty p_property, MCPlatformPropertyType p_type, void *r_value)
{
	MapMenuItemIndex(p_index);
	
	NSMenuItem *t_item;
	t_item = [menu itemAtIndex: p_index];
    
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

void MCMacPlatformMenu::SetItemProperty(uindex_t p_index, MCPlatformMenuItemProperty p_property, MCPlatformPropertyType p_type, const void *p_value)
{
	MapMenuItemIndex(p_index);
	
	NSMenuItem *t_item;
    t_item = [menu itemAtIndex: p_index];
    
    MCPlatformAutoStringRefAsCFString t_cf_string(m_callback);
	
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
				[t_item setTarget: menu_delegate];
			}
			else
            {
                // SN-2014-11-06: [[ Bug 13940 ]] Update the parent - if any - to know that his submenu has
                //  a Quit or a Preferences accelerator
                NSMenu *t_supermenu;
                t_supermenu = [menu supermenu];
                
                if (t_supermenu != nil)
                {
                    MCMacPlatformMenu * t_supermenu_ref;
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
				if (static_cast<MCMacPlatformCore *>(m_platform) -> MapMenuItemActionToSelector(t_action, t_selector))
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
			MCMacPlatformMenu * t_submenu_ref;
			t_submenu_ref = *(MCMacPlatformMenu* *)p_value;
			
			t_submenu_ref -> Retain();
			
			NSMenu *t_current_submenu;
			t_current_submenu = [t_item submenu];
			if (t_current_submenu != nil)
			{
				MCPlatformMenuRef t_current_submenu_ref;
				t_current_submenu_ref = [(MCMenuDelegate *)[t_current_submenu delegate] platformMenuRef];
				t_current_submenu_ref->Release();
			}
			
            // PM-2015-02-09: [[ Bug 14521 ]] No action since menu item has submenus
            // SN-2015-01-12: [[ Bug 14346 ]] Menu items with a submenu should not be selectable
            [t_item setAction: nil];
            [t_item setTarget: nil];
            
			[t_item setSubmenu: t_submenu_ref -> menu];
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

bool MCMacPlatformMenu::PopUp(MCPlatformWindowRef p_window, MCPoint p_location, uindex_t p_item)
{
	NSView *t_view;
	if (p_window != nil)
		t_view = ((MCMacPlatformWindow *)p_window) -> GetView();
	else
		t_view = nil;
	
	NSPoint t_location;
	if (t_view != nil)
	{
		p_window -> MapPointFromScreenToWindow(p_location, p_location);
		((MCMacPlatformWindow *)p_window) -> MapMCPointToNSPoint(p_location, t_location);
	}
	else
		static_cast<MCMacPlatformCore *>(m_platform) -> MapScreenMCPointToNSPoint(p_location, t_location);
    
    // MW-2014-07-29: [[ Bug 12990 ]] If item is UINDEX_MAX then don't specify an item, thus preventing
    //   one from being highlighted.
    
    // SN-2015-11-02: [[ Bug 16218 ]] popUpMenuPositioningItem always returns
    // true if the menu is open by keeping the mouse down, even if the mouse is
    // released outside of the menu list.
    // We will set m_menu_item_selected in menuItemSelected if selection occurs.
    m_menu_item_selected = false;
	[menu popUpMenuPositioningItem: p_item == UINDEX_MAX ? nil : [menu itemAtIndex: p_item] atLocation: t_location inView: t_view];
	
	static_cast<MCMacPlatformCore *>(m_platform) -> SyncMouseAfterTracking();
	
	return m_menu_item_selected;
}

//////////

void MCMacPlatformMenu::StartUsingAsMenubar(void)
{
	if (is_menubar)
		return;
	
	if (m_app_menu_delegate == nil)
        m_app_menu_delegate = [[MCAppMenuDelegate alloc] initWithPlatform:static_cast<MCMacPlatformCore*>(m_platform)];
	
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
    if (about_item == nil)
        [[t_app_menu itemAtIndex: 0] setEnabled:NO];
    [[t_app_menu itemAtIndex: 0] setTarget: m_app_menu_delegate];
    [[t_app_menu itemAtIndex: 0] setTag: kMCShadowedItemAbout];
        
    [t_app_menu addItem: [NSMenuItem separatorItem]];
    
    NSString *t_preferences_string;
    t_preferences_string = NSLocalizedStringFromTable(@"appMenu.preferences", @"Localisation", @"Preferences");
    
	[t_app_menu addItemWithTitle: t_preferences_string
						  action: @selector(preferencesMenuItemSelected:)
				   keyEquivalent: @","];
    // SN-2014-11-06: [[ Bug 13940 ]] Only enable the Preference menu if the shortcut exists in the menubar
    [[t_app_menu itemAtIndex: 2] setTarget: m_app_menu_delegate];
    if (preferences_item == nil)
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
    if (quit_item != nil)
        [[t_app_menu itemAtIndex: 10] setAction:@selector(quitMenuItemSelected:)];
    else
        [[t_app_menu itemAtIndex: 10] setAction:@selector(quitApplicationSelected:)];
    
    [[t_app_menu itemAtIndex: 10] setTarget: m_app_menu_delegate];
    [[t_app_menu itemAtIndex: 10] setTag: kMCShadowedItemQuit];
	[t_app_menu setDelegate: m_app_menu_delegate];
	
	NSMenuItem *t_app_menu_item;
	t_app_menu_item = [[NSMenuItem alloc] initWithTitle: t_app_name action: nil keyEquivalent: @""];
	[t_app_menu_item setSubmenu: t_app_menu];
	[t_app_menu release];
	
	[menu insertItem: t_app_menu_item atIndex: 0];
	[t_app_menu_item release];
	
	is_menubar = true;
}

void MCMacPlatformMenu::StopUsingAsMenubar(void)
{
	if (!is_menubar)
		return;
	
	[menu removeItemAtIndex: 0];
	
	is_menubar = false;
}

void MCMacPlatformCore::ShowMenubar(void)
{
    [NSMenu setMenuBarVisible:YES];
}

void MCMacPlatformCore::HideMenubar(void)
{
    [NSMenu setMenuBarVisible:NO];
}

void MCMacPlatformCore::SetMenubar(MCPlatformMenuRef p_menu)
{
	if (p_menu == m_menubar)
		return;
	
	MCPlatformMenuRef t_old_menubar;
	t_old_menubar = m_menubar;
	m_menubar = nil;
	
	if (p_menu != nil)
	{
		p_menu->StartUsingAsMenubar();
		[NSApp setMainMenu: static_cast<MCMacPlatformMenu*>(p_menu) -> GetMenu()];
		p_menu->Retain();
		m_menubar = static_cast<MCMacPlatformMenu*>(p_menu);
	}
	else
		[NSApp setMainMenu: nil];
	
	if (t_old_menubar != nil)
	{
		t_old_menubar->StopUsingAsMenubar();
        t_old_menubar->Release();
	}
}

MCPlatformMenuRef MCMacPlatformCore::GetMenubar(void)
{
	return m_menubar;
}

MCPlatformMenuRef MCMacPlatformCore::CreateMenu()
{
    MCPlatform::Ref<MCPlatformMenu> t_ref = MCPlatform::makeRef<MCMacPlatformMenu>(this);
    
    return t_ref.unsafeTake();
}


//////////

NSMenu *MCMacPlatformCore::GetIconMenu(void)
{
	if (m_icon_menu != nil)
		SendMenuUpdate(m_icon_menu);
	
	if (m_icon_menu != nil)
		return m_icon_menu -> GetMenu();
	
	return nil;
}

void MCMacPlatformCore::SetIconMenu(MCPlatformMenuRef p_menu)
{
	if (m_icon_menu != nil)
       m_icon_menu->Release();
	m_icon_menu = static_cast<MCMacPlatformMenu *>(p_menu);
	if (m_icon_menu != nil)
		m_icon_menu->Retain();
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

bool MCMacPlatformCore::MapMenuItemActionToSelector(MCPlatformMenuItemAction action, SEL& r_selector)
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
bool MCMacPlatformCore::InitializeMenu(void)
{
    MCPlatformMenuRef t_menubar = CreateMenu();
    SetMenubar(t_menubar);
    return true;
}

void MCMacPlatformCore::FinalizeMenu(void)
{
    
}

////////////////////////////////////////////////////////////////////////////////


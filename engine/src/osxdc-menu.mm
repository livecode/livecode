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
#include "objdefs.h"
#include "parsedef.h"

#include "dispatch.h"
#include "stack.h"
#include "card.h"
#include "image.h"
#include "util.h"
#include "date.h"
#include "param.h"
#include "execpt.h"
#include "player.h"
#include "group.h"
#include "button.h"
#include "globals.h"
#include "mode.h"
#include "eventqueue.h"
#include "osspec.h"
#include "redraw.h"
#include "stacklst.h"
#include "button.h"
#include "menuparse.h"

#include "osxdc.h"
#include "osxprinter.h"
#include "resolution.h"

#include <Cocoa/Cocoa.h>

////////////////////////////////////////////////////////////////////////////////

#ifdef OLD_MAC
//for displaying and building MAC menus, wihich includes hierachical menu

#define MAX_SUBMENU_DEPTH  31    //for Hierachical menus

#define SUB_MENU_START_ID  257   //the ID of the first sub menu. MAC allows from 1 to 235
#define SUB_MENU_LAST_ID 20000
#define MENUIDSIZE 2
#define MENUIDOFFSET 3

#define MAIN_MENU_LAST_ID  32
#define MAIN_MENU_FIRST_ID 2

uint2 MCScreenDC::submenuIDs[SUB_MENU_LAST_ID];
#endif

////////////////////////////////////////////////////////////////////////////////

@interface com_runrev_livecode_MCCocoaMenuDelegate: NSObject<NSMenuDelegate>

- (void)menuWillOpen: (NSMenu *)menu;

- (void)menuItemSelected: (id)sender;

@end

// This structure holds info about each currently set main menu.
struct MCCocoaMainMenuInfo
{
	NSMenuItem *menu_item;
	MCObjectHandle *target;
};

////////////////////////////////////////////////////////////////////////////////

// This is the delegate that is used to capture the 'willOpen' event.
static com_runrev_livecode_MCCocoaMenuDelegate *s_menu_delegate = nil;

// This is the sequence of menus that are currently installed in the menubar.
static MCCocoaMainMenuInfo *s_menubar = nil;
static uindex_t s_menubar_size = 0;

////////////////////////////////////////////////////////////////////////////////

@implementation com_runrev_livecode_MCCocoaMenuDelegate

- (void)menuNeedsUpdate: (NSMenu *)menu
{	
	if ([menu supermenu] == [NSApp mainMenu])
	{
		NSMenuItem *t_menu_item;
		t_menu_item = [[NSApp mainMenu] itemAtIndex: [[NSApp mainMenu] indexOfItemWithSubmenu: menu]];
		
		NSInteger t_tag;
		t_tag = [t_menu_item tag];
		
		if (t_tag != 0)
		{
			// Remember to adjust the tag by -1 since the app menu is tag 0.
			MCObjectHandle *t_target;
			t_target = s_menubar[t_tag - 1] . target;
			
			//
			// COCOA-TODO: Only dispatch if we are not in a blocking wait
			if (t_target -> Exists())
				t_target -> Get() -> message_with_args(MCM_mouse_down, "");
			///* UNCHECKED */ MCEventQueuePostUpdateMenu(t_target);
		}
		else
		{
			// COCOA-TODO: Handle app menu.
		}
	}
	else
	{
		// COCOA-TODO: Other menu types.
	}
}

- (void)menuWillOpen: (NSMenu *)menu
{
}

- (void)menuItemSelected: (id)sender
{
	NSMenuItem *t_item;
	t_item = (NSMenuItem *)sender;
	
	NSMutableString *t_pick;
	t_pick = [[NSMutableString alloc] init];
	[t_pick insertString: [t_item representedObject] atIndex: 0];
	for(;;)
	{
		NSMenuItem *t_parent_item;
		t_parent_item = [t_item parentItem];
		
		// If the parent item's menu is the main menu, we are done.
		if ([t_parent_item menu] == [NSApp mainMenu])
			break;
		
		[t_pick insertString: @"|" atIndex: 0];
		[t_pick insertString: [t_parent_item representedObject] atIndex: 0];
		
		t_item = t_parent_item;
	}
	
	NSInteger t_tag;
	t_tag = [[t_item parentItem] tag];
	
	if (t_tag != 0)
		MCEventQueuePostMenuPick(s_menubar[t_tag - 1] . target, [t_pick cStringUsingEncoding: NSMacOSRomanStringEncoding]);
	
	[t_pick release];
}

@end

////////////////////////////////////////////////////////////////////////////////

static NSString *nsstring_from_mcstring(const MCString& p_string, bool p_is_unicode)
{
	if (p_is_unicode)
		return [NSString stringWithCharacters: (const unichar *)p_string . getstring() length: p_string . getlength() / 2];

	return [[[NSString alloc] initWithBytes: p_string . getstring() length: p_string . getlength() encoding: NSMacOSRomanStringEncoding] autorelease];
}

////////////////////////////////////////////////////////////////////////////////

void MCScreenDC::open_menus(void)
{
#if OLD_MAC
	//get handle of application menu bar
	menuBar = GetMenuBar();
	SetMenuBar(menuBar);  //set menu bar as current menulist
	
	//create Apple menu
	appleMenu = NewMenu(mApple, "\p\024"); //menu title is an apple icon
	InsertMenuItem(appleMenu, "\pAbout...", 0);
	InsertMenu(appleMenu, 0);
	
	DrawMenuBar(); //draw the menu bar with the Apple menu
	
	// preallocate these because GetItemMark can't distinguish them
	submenuIDs[0] = 1;
	submenuIDs[checkMark] = 1;
	submenuIDs[diamondMark] = 1;
#endif
	
	
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
	
	// COCOA-TODO: Fetch name from bundle (?)
	NSString *t_app_name;
	t_app_name = [[NSProcessInfo processInfo] processName];
	
	NSMenu *t_app_menu;
	t_app_menu = [[NSMenu alloc] initWithTitle: t_app_name];
	[t_app_menu addItemWithTitle: [@"Quit " stringByAppendingString: t_app_name] action: @selector(terminate:) keyEquivalent:@"q"];
	
	NSMenuItem *t_app_menu_item;
	t_app_menu_item = [[NSMenuItem alloc] initWithTitle: t_app_name action: nil keyEquivalent: @""];
	[t_app_menu_item setSubmenu: t_app_menu];
	[t_app_menu release];
	 
	NSMenu *t_menubar;
	t_menubar = [[NSMenu alloc] initWithTitle: @""];
	[t_menubar addItem: t_app_menu_item];
	[t_app_menu_item release];
	
	[NSApp setMainMenu: t_menubar];
	[t_menubar release];
	
	// Create the delegate.
	s_menu_delegate = [[com_runrev_livecode_MCCocoaMenuDelegate alloc] init];
}

void MCScreenDC::close_menus(void)
{
}

////////////////////////////////////////////////////////////////////////////////

class MCCocoaMenuCallback: public IParseMenuCallback
{
	NSMenu *m_root_menu;
	
	NSMenu **m_menus;
	uindex_t m_menu_depth;
	
public:
	MCCocoaMenuCallback(NSMenu *p_root_menu)
	{
		m_root_menu = p_root_menu;
		m_menus = nil;
		m_menu_depth = 0;
	}
	
	~MCCocoaMenuCallback(void)
	{
		assert(m_menu_depth == 0);
		MCMemoryDeleteArray(m_menus);
	}
	
	NSMenu *TopMenu(void)
	{
		return m_menus[m_menu_depth - 1];
	}
	
	// This method takes ownership of the menu.
	void PushMenu(NSMenu *p_menu)
	{
		/* UNCHECKED */ MCMemoryResizeArray(m_menu_depth + 1, m_menus, m_menu_depth);
		m_menus[m_menu_depth - 1] = p_menu;;
	}
	
	void PushNewMenu(void)
	{
		NSMenu *t_menu;
		t_menu = [[NSMenu alloc] init];
		
		// If we have a menu already, then we have some work to do.
		if (m_menu_depth > 0)
		{
			NSMenu *t_top_menu;
			t_top_menu = TopMenu();
			// If the top-most menu has no items, add an empty one.
			if ([t_top_menu numberOfItems] == 0)
				[t_top_menu addItemWithTitle: @"" action: nil keyEquivalent: @""];
			
			// Set the last item's submenu.
			[[t_top_menu itemAtIndex: [t_top_menu numberOfItems] - 1] setSubmenu: t_menu];
		}
		
		// Push the menu on the stack.
		PushMenu(t_menu);
	}
	
	void PopMenu(void)
	{
		[m_menus[m_menu_depth - 1] release];
		m_menu_depth -= 1;
	}
	
	virtual bool Start(void)
	{
		PushMenu([m_root_menu retain]);
		
		return true;
	}
	
	virtual bool ProcessItem(MCMenuItem *p_menuitem)
	{
		// Make sure the depth of menus we are at is the same as the menuitem.
		while(p_menuitem -> depth > m_menu_depth - 1)
			PushNewMenu();
		while(p_menuitem -> depth < m_menu_depth - 1)
			PopMenu();
		
		NSString *t_item_title;
		t_item_title = nsstring_from_mcstring(p_menuitem -> label, p_menuitem -> is_unicode);
		
		NSMenuItem *t_item;
		if ([t_item_title isEqualTo: @"-"])
			t_item = [NSMenuItem separatorItem];
		else
		{
			NSString *t_item_tag;
			t_item_tag = nsstring_from_mcstring(p_menuitem -> tag, false);
			
			NSString *t_item_key;
			t_item_key = @"";
			
			NSInteger t_item_state;
			bool t_use_diamond;
			t_item_state = NSOffState;
			t_use_diamond = false;
			if (p_menuitem -> is_hilited)
			{
				t_item_state = NSOnState;
				if (p_menuitem -> is_radio)
					t_use_diamond = true;
			}
			
			t_item = [[NSMenuItem alloc] initWithTitle: t_item_title action: @selector(menuItemSelected:) keyEquivalent: t_item_key];
			
			[t_item setRepresentedObject: t_item_tag];
			[t_item setEnabled: !p_menuitem -> is_disabled];
			[t_item setState: t_item_state];
			[t_item setTarget: s_menu_delegate];
			if (t_use_diamond)
				; // COCOA-TODO; use setOnStateImage to set to diamond
		}
		
		// Add the item to the top menu.
		[TopMenu() addItem: t_item];
		
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

void MCScreenDC::updatemenubar(Boolean force)
{
	// Original logic taken from previous port - only update the menubar if
	// appropriate.
	
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

	// Count the number of menus.
	uint2 t_menu_count;
	t_menu_count = 0;
	newMenuGroup -> count(CT_MENU, NULL, t_menu_count);
	
	// The index of the menu in the menubar.
	uindex_t t_menu_index;
	t_menu_index = 0;
	
	// We construct the new menubar as we go along.
	MCCocoaMainMenuInfo *t_new_menubar;
	uindex_t t_new_menubar_size;
	t_new_menubar = nil;
	t_new_menubar_size = 0;
	
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
		
		// Extend the new menubar by one menu.
		/* UNCHECKED */ MCMemoryResizeArray(t_new_menubar_size + 1, t_new_menubar, t_new_menubar_size);
		
		// Convert the title to a string
		NSString *t_menu_title_string;
		t_menu_title_string = nsstring_from_mcstring(t_menu_title, t_is_unicode);
		
		// COCOA-TODO: Change to using object handles for search.
		
		// See if we have an existing menu with the same title.
		uindex_t t_old_menu_index;
		for(t_old_menu_index = 0; t_old_menu_index < s_menubar_size; t_old_menu_index++)
			if ([[s_menubar[t_old_menu_index] . menu_item title] isEqualTo: t_menu_title_string])
				break;
		
		NSMenuItem *t_menubar_item;
		if (t_old_menu_index == s_menubar_size)
		{
			// We didn't find an existing menu, so make one.
			NSMenu *t_menubar_menu;
			t_menubar_item = [[NSMenuItem allocWithZone: [NSMenu menuZone]] initWithTitle: t_menu_title_string action: NULL keyEquivalent: @""];
			t_menubar_menu = [[NSMenu allocWithZone: [NSMenu menuZone]] initWithTitle: t_menu_title_string];
			[t_menubar_menu setDelegate: s_menu_delegate];
			[t_menubar_menu setAutoenablesItems: NO];
			[t_menubar_item setSubmenu: t_menubar_menu];
			[t_menubar_menu release];
		}
		else
		{
			// We found an existing menu, so take it.
			t_menubar_item = s_menubar[t_old_menu_index] . menu_item;
			s_menubar[t_old_menu_index] . menu_item = nil;
			s_menubar[t_old_menu_index] . target -> Release();
			s_menubar[t_old_menu_index] . target = nil;
		}
		
		// Now we have a menu to populate, do so - first remove all the existing
		// items, and then repopulate.
		[[t_menubar_item submenu] removeAllItems];
		
		MCCocoaMenuCallback t_callback([t_menubar_item submenu]);
		MCString t_menu_string;
		t_menu_button -> getmenustring(t_menu_string);
		MCParseMenuString(t_menu_string, &t_callback, t_menu_button -> hasunicode(), WM_PULLDOWN);
		
		if (t_menu_button -> isdisabled())
			[t_menubar_item setEnabled: NO];
		
		// Set the tag of the menu.
		[t_menubar_item setTag: t_menu_index + 1];
		
		t_new_menubar[t_menu_index] . menu_item = t_menubar_item;
		t_new_menubar[t_menu_index] . target = t_menu_button -> gethandle();
		
		t_menu_index++;
	}
	
	// Remove all deleted menus from the menubar.
	for(uindex_t i = 0; i < s_menubar_size; i++)
	{
		// Skip reused menus.
		if (s_menubar[i] . menu_item == nil)
			continue;
		
		// Remove the item from the mainMenu.
		[[NSApp mainMenu] removeItem: s_menubar[i] . menu_item];
		
		// Release the item.
		[s_menubar[i] . menu_item release];
		s_menubar[i] . target -> Release();
		
		// Clear the location for cleanliness.
		s_menubar[i] . menu_item = nil;
		s_menubar[i] . target = nil;
	}
	
	// Now make sure all the menus are in the right order. Note that the
	// application menu is index 0, so user menus start at index 1.
	for(uindex_t i = 0; i < t_new_menubar_size; i++)
	{
		// Get the index of the item in the menubar.
		NSInteger t_existing_index;
		t_existing_index = [[NSApp mainMenu] indexOfItem: t_new_menubar[i] . menu_item];
		
		// If the item is already in the correct place, do nothing.
		if (t_existing_index != i + 1)
		{		
			if ([[NSApp mainMenu] numberOfItems] == 0)
				[[NSApp mainMenu] addItem: t_new_menubar[i] . menu_item];
			else
				[[NSApp mainMenu] insertItem: t_new_menubar[i] . menu_item atIndex: i + 1];
		}

		[[t_new_menubar[i] . menu_item submenu] update];
	}
	
	// Finally release the old menubar array and update the mainmenu.
	MCMemoryDeleteArray(s_menubar);
	
	s_menubar = t_new_menubar;
	s_menubar_size = t_new_menubar_size;
	
	[[NSApp mainMenu] update];
}

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

typedef struct
{
	uint4 keysym;
	uint2 glyph;
}
KeyGlyph;

static KeyGlyph key_glyphs[] =
{
	{XK_F1, kMenuF1Glyph},
	{XK_F2, kMenuF2Glyph},
	{XK_F3, kMenuF3Glyph},
	{XK_F4, kMenuF4Glyph},
	{XK_F5, kMenuF5Glyph},
	{XK_F6, kMenuF6Glyph},
	{XK_F7, kMenuF7Glyph},
	{XK_F8, kMenuF8Glyph},
	{XK_F9, kMenuF9Glyph},
	{XK_F10, kMenuF10Glyph},
	{XK_F11, kMenuF11Glyph},
	{XK_F12, kMenuF12Glyph},
	{XK_F13, kMenuF13Glyph},
	{XK_F14, kMenuF14Glyph},
	{XK_F15, kMenuF15Glyph},
	{XK_BackSpace, kMenuDeleteLeftGlyph},
	{XK_Tab, kMenuTabRightGlyph},
	{XK_Return, kMenuReturnGlyph},
	{XK_Escape, kMenuEscapeGlyph},
	{XK_Delete, kMenuDeleteRightGlyph},
	{XK_Home, kMenuNorthwestArrowGlyph},
	{XK_Left, kMenuLeftArrowGlyph},
	{XK_Up, kMenuUpArrowGlyph},
	{XK_Right, kMenuRightArrowGlyph},
	{XK_Down, kMenuDownArrowGlyph},
	{XK_End, kMenuSoutheastArrowGlyph},
	{XK_Prior, kMenuPageUpGlyph},
	{XK_Next, kMenuPageDownGlyph},
	{XK_osfHelp, kMenuHelpGlyph},
	{XK_KP_Enter, kMenuEnterGlyph},
	{XK_space, kMenuSpaceGlyph},
	{0, kMenuNullGlyph},
};

////////////////////////////////////////////////////////////////////////////////

#if 0
static void domenu(short menu, short item)
{
	MenuHandle mhandle = GetMenuHandle(menu);
	if (mhandle == NULL)
		return;
	
	{
		MCGroup *mb = MCmenubar != NULL ? MCmenubar : MCdefaultmenubar;
		if (mb == NULL)
			return;
		//MetaCard's application menu, starts from id 2. id 1 is Apple menu
		
		//Aqua app's help menu is just like other menu...
		if (mb != NULL)
		{ //to response to the menu pick msg
			MCButton *bptr = NULL;
			bool isunicode = false;
			bool issubmenu = (menu >= SUB_MENU_START_ID);
			if (issubmenu)
				// Get main menu id of submenu, stored as a property of the menu
				GetMenuItemProperty(mhandle, 0, 'RRev', 'MMID', sizeof(menu), NULL, &menu);
			if (menu == 1)
			{//selection code
				uint2 helpmenu = 0;
				mb->count(CT_MENU, NULL, helpmenu);
				//search for the last mneu, that is the help menu in MC
				// MW-2005-03-08: [[MacOS Classic]] Predecrementing a reference causes problems
				helpmenu -= 1;
				bptr = (MCButton *)mb->findnum(CT_MENU, helpmenu);
			}
			else
			{
				uint2 gmenu = menu - 2;
				bptr = (MCButton *)mb->findnum(CT_MENU, gmenu);
			}
			if (bptr)
			{
				isunicode = bptr->hasunicode();
			}
			bool t_menuhastags;
			GetMenuItemProperty(GetMenuHandle(menu), 0, 'RRev', 'Tags', sizeof(t_menuhastags), NULL, &t_menuhastags);
			
			isunicode &= !t_menuhastags;
			char *menuitemname;
			uint2 menuitemlen;
			MCString t_tag;
			extern bool MCMacGetMenuItemTag(MenuHandle menu, uint2 mitem, MCString &s);
			if (t_menuhastags && MCMacGetMenuItemTag(mhandle, item, t_tag))
			{
				menuitemname = (char*)t_tag.getstring();
				menuitemlen = t_tag.getlength();
			}
			else
			{
				CFStringRef cfmenuitem;
				CopyMenuItemTextAsCFString(mhandle, item, &cfmenuitem);
				uint2 t_menuitemlen = CFStringGetLength(cfmenuitem);
				menuitemlen = t_menuitemlen * MCU_charsize(isunicode);
				if (isunicode)
				{
					menuitemname = new char[menuitemlen];
					CFStringGetCharacters(cfmenuitem, CFRangeMake(0, t_menuitemlen), (UniChar*)menuitemname);
				}
				else
				{
					menuitemname = new char[menuitemlen + 1];
					CFStringGetCString(cfmenuitem, menuitemname, menuitemlen + 1,kCFStringEncodingMacRoman);
				}
				CFRelease(cfmenuitem);
			}
			
			char *menupick = menuitemname;
			uint2 menupicklen = menuitemlen;
			char *newmenu = NULL;
			
			if (issubmenu)
			{ //item from submenu is selected
				/* get the menu hierachy to pass
				 back to the user. menu hierachy is store in the menu
				 handle's data field. It's format is :
				 mainmenu|submenu1|submenu2|....    get the
				 length of the menu hierachy. PASCAL string byte 0 is the length */
				
				CFStringRef cftitlestr;
				CopyMenuTitleAsCFString(mhandle,&cftitlestr);
				uint2 titlelen = CFStringGetLength(cftitlestr);
				uint2 t_titlestrlen = titlelen * MCU_charsize(isunicode);
				if (isunicode)
				{
					newmenu = new char[t_titlestrlen + menuitemlen];//bug menuitemlen too small for unicode
					CFStringGetCharacters(cftitlestr,CFRangeMake(0,titlelen),(UniChar *)newmenu);
				}
				else
				{
					newmenu = new char[t_titlestrlen + menuitemlen + 1];
					CFStringGetCString(cftitlestr, newmenu, titlelen + 1, kCFStringEncodingMacRoman);
				}
				
				memcpy(&newmenu[t_titlestrlen], menuitemname, menuitemlen);
				delete menuitemname;
				
				menupick = newmenu;
				menupicklen = t_titlestrlen + menuitemlen;
				
				CFRelease(cftitlestr);
			}
			if (bptr != NULL)
			{
				bptr->setmenuhistoryprop(item);
				bptr->message_with_args(MCM_menu_pick,MCString(menupick,menupicklen));
			}
			delete menupick;
		}
	}
}

/**************************************************************************
 * Following functions create and handle menus.  These menus are MC       *
 * stack-based menus i.e. menus attached to a stack.  In Buttom.cpp file  *
 * it contains codes for MAC platform to display objects/buttons based    *
 * menus such as popup, options, and pulldown menus etc.		  *
 *			                                                  *
 **************************************************************************/
/**************************************************************************
 
 * The Application's Main Menu id starts from 2 to 32.  Apple reserves
 * id 0 and 1.  The id for the submenu of the Application's Main
 * menu, the button's pulldow menu, it's submenu, button's popup
 * menu, it's submenu and button's option menus starts from 33 to
 * 235.  No menu will be created when the maximum id is reached.  
 * 
 * An array of 256 of short int is maintained to keep track of the
 * allocation non-application main menu id, and it's de-allocation.
 * The reason behine this is that user main continue to create
 * button's popup/option/pulldown menus and their submenus, delete
 * them and re-create them again in one instance of a MC session.
 * Therefore it is necessary to keep track the used and unused id
 * Only member 33 to 235 would be used in this array
 **************************************************************************/

short MCScreenDC::allocateSubMenuID(Boolean issubmenu)
{
	/*
	 routine go through each element of the array submenuID and find
	 the first available element and return the element's number as the
	 available submenu ID */
	short i = issubmenu? SUB_MENU_START_ID: 33;
	short submaxid = issubmenu? SUB_MENU_LAST_ID: 255;
	while (i < submaxid)
	{
		if (submenuIDs[i])
			i++;
		else
		{
			submenuIDs[i] = 1;
			return i;
		}
	}
	return 0;  //can't find available id for a new submenu use
}

void MCScreenDC::freeMenuAndID(short menuID, MCButton *p_menubutton)
{
	//routine to de-allocate menu ID which is allocated from submenuID
	//array and to free and delete the menu and it's submenus, if there is one
	MenuHandle menuHndl;
	submenuIDs[menuID] = 0; //zero out this id, put it back to available pool.
	// fprintf(stderr,"freemenuid getmenuhandle %d\n",menuID);
	
	menuHndl = GetMenuHandle(menuID); //get menu handl from id. if no menu
	if (menuHndl == NULL)             //for this id, we are done. The id is freed
		return;
	
	//remove accelerators assocated with this menu
	if (p_menubutton != NULL)
		MCstacks->deleteaccelerator(p_menubutton,NULL);
	
	int2 count = CountMenuItems(menuHndl);
	int2 i;
	for (i = 1; i <= count; i++)
	{
		short id;
		if (GetMenuItemHierarchicalID(menuHndl, i, &id) == noErr && id != 0)
			freeMenuAndID(id);
	}
	DeleteMenu(menuID);
	DisposeMenu(menuHndl); //release the memory
}

struct OSXMenuItem : public MCMenuItem
{
	MenuHandle mh;
	char mark;
	uint2 glyph;
	uint2 after; //insert new menu item after this number
};

static void SetMacMenuItemText(MenuHandle mh, uint2 mitem, char *mitemtext, uint2 mitemlength, bool isunicode)
{
	
	CFStringRef cfmenuitemstring;
	
	if (isunicode && mitemlength > 0)
		cfmenuitemstring = CFStringCreateWithCharacters(NULL, (UniChar *)mitemtext, mitemlength >> 1);
	else
		cfmenuitemstring = CFStringCreateWithCString(NULL, mitemtext, kCFStringEncodingMacRoman);
	
	SetMenuItemTextWithCFString(mh, mitem, cfmenuitemstring);
	CFRelease(cfmenuitemstring);
}

uint2 MCMacKeysymToGlyph(uint4 key)
{
	for (int i = 0; key_glyphs[i].glyph != 0; i++)
	{
		if (key == key_glyphs[i].keysym)
		{
			return key_glyphs[i].glyph;
		}
	}
	return kMenuNullGlyph;
}

uint4 MCMacGlyphToKeysym(uint2 glyph)
{
	for (int i = 0; key_glyphs[i].glyph != 0; i++)
	{
		if (glyph == key_glyphs[i].glyph)
		{
			return key_glyphs[i].keysym;
		}
	}
	return 0;
}

class OSXMenuCallback : public IParseMenuCallback
{
	bool firstTime;
	bool isunicode;
	OSXMenuItem curMenuStruct[MAX_SUBMENU_DEPTH + 1];
	uint2 firstitem;
	int2 curMenuLevel;
	uint2 mainMenuID;
	MCScreenDC *screen;
	MCFontStruct *font;
	MenuHandle appleMenu;
	MCButton *menubutton;
	
public:
	OSXMenuCallback(MCScreenDC *p_screen, MCButton *p_menubutton, MenuHandle p_mainmenu, MenuHandle p_applemenu, uint2 p_mainmenu_id)
	{
		screen = p_screen;
		menubutton = p_menubutton;
		appleMenu = p_applemenu;
		isunicode = p_menubutton -> hasunicode();
		firstTime = true;
		mainMenuID = p_mainmenu_id;
		memset(curMenuStruct, 0, sizeof(curMenuStruct));
		curMenuLevel = 0;
		firstitem = 0;
		curMenuStruct[0].after = firstitem;
		//fill in element 0 to indicates this is the top level menu
		curMenuStruct[0].mh = p_mainmenu; //put the top-most-level menu handle in the
		//element 0 of "mh" field
	}
	
	void SetMacMenuItemProperties(MenuHandle p_menu, uint2 p_item, OSXMenuItem *p_menuitem)
	{
		if (p_menuitem->accelerator)
		{
			if (p_menuitem->glyph)
				SetMenuItemKeyGlyph(p_menu, p_item, p_menuitem->glyph);
			else
				SetMenuItemCommandKey(p_menu, p_item, false, p_menuitem->accelerator);
			uint1 t_mods = p_menuitem->modifiers;
			uint1 t_macmods = 0;
			if (t_mods & MS_SHIFT)
				t_macmods |= kMenuShiftModifier;
			if (t_mods & MS_ALT)
				t_macmods |= kMenuOptionModifier;
			if (t_mods & MS_MAC_CONTROL)
				t_macmods |= kMenuControlModifier;
			if (!(t_mods & MS_CONTROL))
				t_macmods |= kMenuNoCommandModifier;
			SetMenuItemModifiers(p_menu, p_item, t_macmods);
			
			uint2 t_accel = p_menuitem->accelerator;
			if (t_accel < 256)
				t_accel = MCS_tolower(t_accel);
			MCstacks->addaccelerator(menubutton, menubutton->getstack(), t_accel, t_mods);
		}
		if (p_menuitem->is_disabled)
		{
			DisableMenuItem(p_menu, p_item);
		}
		if (p_menuitem->tag != NULL)
		{
			MCString t_tag = p_menuitem->tag;
			SetMenuItemProperty(p_menu, p_item,'RRev','MTag',t_tag.getlength(),(const void *)t_tag.getstring());
			
			static const struct { const char *tag; MenuCommand id; } s_tag_to_id[] =
			{
				{"undo", kHICommandUndo},
				{"redo", kHICommandRedo},
				{"cut", kHICommandCut},
				{"copy", kHICommandCopy},
				{"paste", kHICommandPaste},
				{"clear", kHICommandClear},
				{"select all", kHICommandSelectAll},
				{"preferences", kHICommandPreferences},
				{"about", kHICommandAbout},
			};
			
			for(uint4 i = 0; i < sizeof(s_tag_to_id) / sizeof(s_tag_to_id[0]); ++i)
				if (t_tag == s_tag_to_id[i] . tag)
				{
					SetMenuItemCommandID(p_menu, p_item, s_tag_to_id[i] . id);
					break;
				}
		}
	}
	
	virtual bool ProcessItem(MCMenuItem *p_menuitem)
	{
		uint2 t_keyglyph = 0;
		uint4 t_key = p_menuitem->accelerator;
		const char *t_keyname = p_menuitem->accelerator_name;
		if (t_keyname != NULL)
		{
			t_keyglyph = MCMacKeysymToGlyph(t_key);
		}
		
		char markchar = 0;
		
		if (p_menuitem->is_hilited)
		{
			if (p_menuitem->is_radio)
				markchar = diamondMark;
			else
				markchar = checkMark;
		}
		
		if (firstTime)
		{
			curMenuStruct[curMenuLevel].assignFrom(p_menuitem);
			curMenuStruct[curMenuLevel].label.set( p_menuitem->label.clone() , p_menuitem->label.getlength() );
			if (p_menuitem->tag != NULL)
				curMenuStruct[curMenuLevel].tag.set( p_menuitem->tag.clone() , p_menuitem->tag.getlength() );
			curMenuStruct[curMenuLevel].mark = markchar;
			curMenuStruct[curMenuLevel].glyph = t_keyglyph;
			firstTime = false;
			return false;
		}
		if (p_menuitem->depth > curMenuStruct[curMenuLevel].depth)
		{ //cur item goes down one level deep
			//new submenu name reflects the hierachy of it's menu level :
			
			uint4 t_combined_length = 0;
			for (int i = 0; i <= curMenuLevel; i++)
			{
				if (curMenuStruct[i].tag != NULL)
					t_combined_length += curMenuStruct[i].tag.getlength() * MCU_charsize(isunicode);
				else
					t_combined_length += curMenuStruct[i].label.getlength();
				
				t_combined_length += MCU_charsize(isunicode);
			}
			
			char *newname = new char[t_combined_length + 1];
			
			uint2 submenutitlelen = 0;
			
			MCExecPoint ep;
			for (int i = 0; i <= curMenuLevel; i++)
			{
				if (curMenuStruct[i].tag != NULL)
				{
					MCString t_tag = curMenuStruct[i].tag;
					if (isunicode)
					{
						ep.setsvalue(t_tag);
						ep.nativetoutf16();
						t_tag = ep.getsvalue();
					}
					memcpy(&newname[submenutitlelen], t_tag.getstring(), t_tag.getlength());
					submenutitlelen += t_tag.getlength();
				}
				else
				{
					memcpy(&newname[submenutitlelen], curMenuStruct[i].label.getstring(), curMenuStruct[i].label.getlength());
					submenutitlelen += curMenuStruct[i].label.getlength();
				}
				
				MCU_copychar('|',&newname[submenutitlelen],isunicode);
				submenutitlelen += MCU_charsize(isunicode);
			}
			
			newname[submenutitlelen] = 0;
			
			short menuID = screen->allocateSubMenuID(True);// get an id, if none is found, bail out
			if (menuID == 0)
			{
				return true;
			}
			
			CreateNewMenu(menuID, 0, &curMenuStruct[curMenuLevel + 1].mh);
			SetMenuItemProperty(curMenuStruct[curMenuLevel + 1].mh, 0, 'RRev','MMID',sizeof(mainMenuID),&mainMenuID);
			InsertMenu(curMenuStruct[curMenuLevel + 1].mh, -1); //-1 is submenu
			
			CFStringRef cfmenustring;
			if (isunicode)
				cfmenustring = CFStringCreateWithCharacters(NULL, (UniChar *)newname,submenutitlelen >> 1);
			else
				cfmenustring = CFStringCreateWithCString(NULL, newname, kCFStringEncodingMacRoman);
			
			SetMenuTitleWithCFString(curMenuStruct[curMenuLevel + 1].mh,cfmenustring);
			CFRelease(cfmenustring);
			delete newname;
			
			curMenuStruct[curMenuLevel + 1].after = 0;
			
			const char *t_label = (const char *)curMenuStruct[curMenuLevel].label.getstring();
			
			if (isunicode)
				cfmenustring = CFStringCreateWithCharacters(NULL, (UniChar *)t_label, curMenuStruct[curMenuLevel].label.getlength() >> 1);
			else
				cfmenustring = CFStringCreateWithCString(NULL, t_label, kCFStringEncodingMacRoman);
			// HS-2010-06-07: [[ Bug 8659 ]] Make sure OS X meta-characters are ignored except for strings like '-'
			UInt32 t_attributes;
			CFStringGetLength(cfmenustring) > 1 ? t_attributes = kMenuItemAttrIgnoreMeta : t_attributes = 0;
			//Insert the item and increment the item insertion point by 1
			InsertMenuItemTextWithCFString(curMenuStruct[curMenuLevel].mh, cfmenustring, curMenuStruct[curMenuLevel].after++, t_attributes, 0);
			CFRelease(cfmenustring);
			
			SetMacMenuItemProperties(curMenuStruct[curMenuLevel].mh, curMenuStruct[curMenuLevel].after, &curMenuStruct[curMenuLevel]);
			SetMenuItemHierarchicalID(curMenuStruct[curMenuLevel].mh,  curMenuStruct[curMenuLevel].after, menuID);
			
			//DO NOT SET MARK CHAR Here, this would reset the submenu attribute.
			//    delete newt;
			curMenuLevel++;
		}
		else
		{
			CFStringRef cfmenustring;
			if (isunicode)
				cfmenustring = CFStringCreateWithCharacters(NULL, (UniChar *)curMenuStruct[curMenuLevel].label.getstring(),curMenuStruct[curMenuLevel].label.getlength() >> 1);
			else
				cfmenustring = CFStringCreateWithCString(NULL, (char*)curMenuStruct[curMenuLevel].label.getstring(), kCFStringEncodingMacRoman);
			// HS-2010-06-07: [[ Bug 8659 ]] Make sure OS X meta-characters are ignored except for strings like '-'
			UInt32 t_attributes;
			CFStringGetLength(cfmenustring) > 1 ? t_attributes = kMenuItemAttrIgnoreMeta : t_attributes = 0;
			InsertMenuItemTextWithCFString(curMenuStruct[curMenuLevel].mh, cfmenustring, curMenuStruct[curMenuLevel].after++, t_attributes, 0);
			CFRelease(cfmenustring);
			
			SetMacMenuItemProperties(curMenuStruct[curMenuLevel].mh, curMenuStruct[curMenuLevel].after, &curMenuStruct[curMenuLevel]);
			SetItemMark(curMenuStruct[curMenuLevel].mh, curMenuStruct[curMenuLevel].after, curMenuStruct[curMenuLevel].mark);
			
			//go back up in menu levels. set curMenuLevel to reflect the changes
			
			while (curMenuLevel && curMenuStruct[curMenuLevel].depth > p_menuitem->depth)
			{
				delete curMenuStruct[curMenuLevel].label.getstring();
				curMenuStruct[curMenuLevel].label.set(NULL, 0);
				delete curMenuStruct[curMenuLevel].tag.getstring();
				curMenuStruct[curMenuLevel].tag.set(NULL, 0);
				curMenuLevel--;
			}
			if (curMenuStruct[curMenuLevel].depth < p_menuitem->depth)
				curMenuLevel++;
		}
		delete curMenuStruct[curMenuLevel].label.getstring();
		curMenuStruct[curMenuLevel].label.set(NULL, 0);
		delete curMenuStruct[curMenuLevel].tag.getstring();
		curMenuStruct[curMenuLevel].tag.set(NULL, 0);
		
		//keep record of the current item
		curMenuStruct[curMenuLevel].assignFrom(p_menuitem);
		curMenuStruct[curMenuLevel].label.set( p_menuitem->label.clone() , p_menuitem->label.getlength() );
		if (p_menuitem->tag != NULL)
			curMenuStruct[curMenuLevel].tag.set( p_menuitem->tag.clone(), p_menuitem->tag.getlength() );
		curMenuStruct[curMenuLevel].mark = markchar;
		curMenuStruct[curMenuLevel].glyph = t_keyglyph;
		
		
		return false;
	}
	
	virtual bool End(bool p_hastags)
	{
		if (curMenuStruct[curMenuLevel].label.getlength())
		{
			//handle single menu item and the last menu item situation
			//add the menu item to the menu
			CFStringRef cfmenustring;
			if (isunicode)
				cfmenustring = CFStringCreateWithCharacters(NULL, (UniChar *)curMenuStruct[curMenuLevel].label.getstring(),curMenuStruct[curMenuLevel].label.getlength() >> 1);
			else
				cfmenustring = CFStringCreateWithCString(NULL, (char*)curMenuStruct[curMenuLevel].label.getstring(), kCFStringEncodingMacRoman);
			// HS-2010-06-07: [[ Bug 8659 ]] Make sure OS X meta-characters are ignored except for strings like '-'
			UInt32 t_attributes;
			CFStringGetLength(cfmenustring) > 1 ? t_attributes = kMenuItemAttrIgnoreMeta : t_attributes = 0;
			
			// HS-2010-06-07: [[ Bug 8659 ]] Make sure OS X meta-characters are ignored except for strings like '-'
			InsertMenuItemTextWithCFString(curMenuStruct[curMenuLevel].mh, cfmenustring, curMenuStruct[curMenuLevel].after++, t_attributes, 0);
			SetItemMark(curMenuStruct[curMenuLevel].mh,
						curMenuStruct[curMenuLevel].after, curMenuStruct[curMenuLevel].mark);
			SetMacMenuItemProperties(curMenuStruct[curMenuLevel].mh, curMenuStruct[curMenuLevel].after, &curMenuStruct[curMenuLevel]);
			
			CFRelease(cfmenustring);
		}
		while (curMenuLevel >= 0)
		{
			delete curMenuStruct[curMenuLevel].label.getstring();
			curMenuStruct[curMenuLevel].label.set(NULL, 0);
			delete curMenuStruct[curMenuLevel].tag.getstring();
			curMenuStruct[curMenuLevel].tag.set(NULL, 0);
			curMenuLevel--;
		}
		MenuHandle t_mainmenu = curMenuStruct[0].mh;
		
		SetMenuItemProperty(t_mainmenu, 0, 'RRev', 'Tags', sizeof(p_hastags), &p_hastags);
		return false;
	}
};

Boolean MCScreenDC::addMenuItemsAndSubMenu(uint2 mainMenuID, MenuHandle mainMenu, MCButton *p_menubutton, uint1 menumode)
{
	OSXMenuCallback t_callback(this, p_menubutton, mainMenu, appleMenu, mainMenuID);
	bool isunicode = p_menubutton -> hasunicode();
	MCString t_string;
	p_menubutton->getmenustring(t_string);
	MCParseMenuString(t_string, &t_callback, isunicode, menumode);
	
	return True;
}

void MCScreenDC::updatemenubar(Boolean force)
{
	//get a pointer to each button (MCButton *) in the menu bar group.
	//if the stack does not have it's own menu bar group, use he default
	//menu bar group is called by reference and is changed by the
	//findnum() routine while not end of the button list
	
	MenuHandle menu; //handle for a menu
	MCButton *bptr; //point to a button
	MCGroup *newMenuGroup; //pointer to the menu group
	static MCGroup *curMenuGroup; //current menu bar handle
	static uint2 curMenuCount;    //current menu bar's menu count
	
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
	
	uint2 i;
	//delete the existing Main menus and it's submenus except Apple
	//menu. Deleting Main menu starts from menu id 2.  Aslo Delete menu
	//items in the Application Help menu, if in None-Aqua env.
	//start deleting menus
	for (i = 0; i < curMenuCount; i++)
		freeMenuAndID(i + 2);
	
	/* Delete Items in the system provided help menu. Assume Help menu
	 * does not have submenus. In Aqua, There isn't a system provided
	 * help menu that you can add your help items to, therefore no
	 * action here, the application created help menu is deleted by the
	 * freeMenuAndID() in the above. MCaqua is a global var, inited in
	 * MCS_init() in macspec.cpp */
	//get menu handle and index
	
	
	uint2 which;    //menu item index
	which = i = 0;  //init to 0
	uint2 menuCount = 0;
	
	/**********************************************************************
	 * used to keep track of the CURRENT hierachical menu level
	 * structure.  * array[0] ==the top level menu, array[1] ==first
	 * level sub menu, array[2]==2nd level sub menu, the array contains
	 * the menu handle of each level of submenu.  The Application Main
	 * menu is limited from id 2 to id 32(MAIN_MENU_LAST_ID)
	 **********************************************************************/
	
	newMenuGroup->count(CT_MENU, NULL, menuCount); //get the # of MC menus
	// the last menu is the Help menu, it's menu items will be inserted into the
	// Application Help menu if the OS is non-Aqua.  Create a menu even if
	// it's contents is empty
	
	// ****** menuCount--; //so that the last menu (help menu) will not be created
	
	bool t_pref_item_found;
	t_pref_item_found = false;
	
	bool t_about_item_found;
	t_about_item_found = false;
	while (((bptr = (MCButton *)newMenuGroup->findnum(CT_MENU, i)) != NULL) &&
		   which < menuCount &&
		   (which + MAIN_MENU_FIRST_ID) <= MAIN_MENU_LAST_ID)
	{
		// IM-2008-09-05: [[ bug 7089 ]] - existing message path puts OSX system menu shortcut handling
		// after control-specific key handling.  Now changed to use stacklist accelerator handling,
		// consistent with windows/linux menu shortcuts
		
		// remove any menu shortcuts for the current button from accelerator list
		MCstacks->deleteaccelerator(bptr, bptr->getstack());
		
		//found a menu group & is not the very last menu
		MCString t_menu_label;
		bool t_is_unicode;
		bptr->getlabeltext(t_menu_label, t_is_unicode); // clone menu title/name
		
		if (t_menu_label . getlength() != 0 && bptr->getflag(F_VISIBLE))
		{
			//unsigned char *tmp = c2pstr(mname);
			
			//Apple menu's id == 1, new application's menus starts with id 2 and up
			//menu = NewMenu(which + MAIN_MENU_FIRST_ID, tmp);
			
			menu = NULL;
			CreateNewMenu(which + MAIN_MENU_FIRST_ID, 0, &menu);
			
			//menu title.  Has to be Pascal string let's add menu items to
			//the newly created menu get menu item list, each item is
			//separated by ','. the cloned menustring is deleted in
			//addMenuItemsAndSubMenu().
			CFStringRef cfmenustring;
			if (t_is_unicode)
				cfmenustring = CFStringCreateWithCharacters(NULL, (UniChar *)t_menu_label.getstring(), t_menu_label.getlength() >> 1);
			else
				cfmenustring = CFStringCreateWithBytes(NULL, (UInt8*)t_menu_label . getstring(), t_menu_label . getlength(), kCFStringEncodingMacRoman, False);
			SetMenuTitleWithCFString(menu,cfmenustring);
			CFRelease(cfmenustring);
			
			addMenuItemsAndSubMenu(which + MAIN_MENU_FIRST_ID, menu, bptr, True);
			if (which == 0)
			{
				int2 count = CountMenuItems(menu);
				DeleteMenuItem(menu, count--);
				DeleteMenuItem(menu, count);
			}
			else
			{
				// MW-2011-02-28: [[ Bug 3071 ]] We only enable the preferences item if:
				//   1) There is an enabled edit menu
				//   2) The edit menu has as last item one with tag "Preferences", or with name that begins with "Preferences"
				//   3) The relevant item, itself, is enabled.
				//
				const char *tname = bptr->getname_cstring();
				if (!t_pref_item_found && MCU_strncasecmp("Edit", tname, strlen(tname)) == 0)
				{
					uint32_t count = CountMenuItems(menu);
					MenuCommand t_id;
					if (GetMenuItemCommandID(menu, count, &t_id) != noErr)
						t_id = 0;
					if (t_id != kHICommandPreferences)
					{
						CFStringRef t_text;
						if (CopyMenuItemTextAsCFString(menu, count, &t_text) == noErr)
						{
							if (CFStringCompareWithOptions(t_text, CFSTR("Preferences"), CFRangeMake(0, 11), kCFCompareCaseInsensitive) == 0)
								t_pref_item_found = true;
							CFRelease(t_text);
						}
					}
					else
						t_pref_item_found = true;
					
					if (t_pref_item_found)
					{
						t_pref_item_found = !bptr -> isdisabled() && IsMenuItemEnabled(menu, count);
						
						DeleteMenuItem(menu, count--);
						DeleteMenuItem(menu, count);
					}
					
					MenuRef prefmenu = NULL;
					MenuItemIndex item = 0;
					GetIndMenuItemWithCommandID(NULL, kHICommandPreferences, 1, &prefmenu, &item);
					if (prefmenu != NULL && item != 0)
						if (t_pref_item_found)
							EnableMenuItem(prefmenu, item);
						else
							DisableMenuItem(prefmenu, item);
					
					t_pref_item_found = true;
				}
				
				// MW-2011-02-28: [[ Bug 818 ]] Be more specific about the 'About' item, and
				//   remove/disable if its not present / not enabled.
				if (!t_about_item_found && MCU_strncasecmp("Help", tname, strlen(tname)) == 0)
				{
					uint32_t count = CountMenuItems(menu);
					MenuCommand t_id;
					if (GetMenuItemCommandID(menu, count, &t_id) != noErr)
						t_id = 0;
					if (t_id != kHICommandAbout)
					{
						CFStringRef t_text;
						if (CopyMenuItemTextAsCFString(menu, count, &t_text) == noErr)
						{
							if (CFStringCompareWithOptions(t_text, CFSTR("About"), CFRangeMake(0, 5), kCFCompareCaseInsensitive) == 0)
								t_about_item_found = true;
							CFRelease(t_text);
						}
					}
					else
						t_about_item_found = true;
					
					if (t_about_item_found)
					{
						CFStringRef t_about_text;
						t_about_text = NULL;
						CopyMenuItemTextAsCFString(menu, count, &t_about_text);
						
						bool t_about_enabled;
						t_about_enabled = !bptr -> isdisabled() && IsMenuItemEnabled(menu, count);
						
						DeleteMenuItem(menu, count--);
						DeleteMenuItem(menu, count);
						
						SetMenuItemTextWithCFString(appleMenu, 1, t_about_text);
						ChangeMenuItemAttributes(appleMenu, 1, t_about_enabled ? 0 : kMenuItemAttrDisabled, (t_about_enabled ? kMenuItemAttrDisabled : 0) | kMenuItemAttrHidden);
						
						if (t_about_text != NULL)
							CFRelease(t_about_text);
					}
					else
						ChangeMenuItemAttributes(appleMenu, 1, kMenuItemAttrHidden | kMenuItemAttrDisabled, 0);
					
					t_about_item_found = true;
				}
			}
			
			// MW-2005-09-05: Only add a menu if it has items...
			if (CountMenuItems(menu) > 0)
				InsertMenu(menu, which + MAIN_MENU_FIRST_ID);
			
			//if menu disabled flag is set, disabled the entire menu
			if (bptr->isdisabled())
				DisableMenuItem(menu, 0);  // 8.5, disables past 31 items
		}
		which++;
		i = which;
	}
	curMenuCount = which; //set the menu count, exclude the Apple and Help menus
	
	curMenuGroup = newMenuGroup;
	curMenuGroup->setstate(False, CS_NEED_UPDATE);
	DrawMenuBar();
	HiliteMenu(0);
}

////////////////////////////////////////////////////////////////////////////////

bool MCMacGetMenuItemTag(MenuHandle mh, uint2 mitem, MCString &s)
{
	UInt32 t_propsize;
	OSStatus t_result = GetMenuItemPropertySize(mh, mitem, 'RRev', 'MTag', &t_propsize);
	if (t_result == noErr)
	{
		char *t_tag = new char[t_propsize + 1];
		t_result = GetMenuItemProperty(mh, mitem, 'RRev', 'MTag', t_propsize, NULL, t_tag);
		t_tag[t_propsize] = '\0';
		s.set(t_tag, t_propsize);
		return true;
	}
	else
		return false;
}

static void getmacmenuitemtext(MenuHandle mh, uint2 mitem, MCString &s, Boolean issubmenu, Boolean isunicode)
{
	//find menu root, to get tags property
	bool t_menuhastags;
	
	MenuID t_menu = GetMenuID(mh);
	if (issubmenu)
		// Get main menu id of submenu, stored as a property of the menu
		GetMenuItemProperty(mh, 0, 'RRev', 'MMID', sizeof(t_menu), NULL, &t_menu);
	GetMenuItemProperty(GetMenuHandle(t_menu), 0, 'RRev', 'Tags', sizeof(t_menuhastags), NULL, &t_menuhastags);
	
	isunicode &= !t_menuhastags;
	char *newmenu = NULL;
	char *menuitemname;
	uint2 menuitemlen;
	
	MCString t_tagstr;
	if (MCMacGetMenuItemTag(mh, mitem, t_tagstr))
	{
		menuitemlen = t_tagstr.getlength();
		menuitemname = (char*)t_tagstr.getstring();
	}
	else
	{
		CFStringRef cfmenuitemstr;
		CopyMenuItemTextAsCFString(mh, mitem,&cfmenuitemstr);
		uint4 t_itemlen = CFStringGetLength(cfmenuitemstr);
		menuitemlen = t_itemlen * MCU_charsize(isunicode);
		if (isunicode)
		{
			menuitemname = new char[menuitemlen];
			CFStringGetCharacters(cfmenuitemstr, CFRangeMake(0, t_itemlen),
								  (UniChar *)menuitemname);
		}
		else
		{
			menuitemname = new char[menuitemlen + 1];
			CFStringGetCString(cfmenuitemstr, menuitemname, menuitemlen + 1,kCFStringEncodingMacRoman);
		}
	}
	char *menupick = menuitemname;
	uint2 menupicklen = menuitemlen;
	if (issubmenu)
	{
		CFStringRef cftitlestr;
		CopyMenuTitleAsCFString(mh,&cftitlestr);
		uint2 titlelen = CFStringGetLength(cftitlestr);
		uint4 t_titlestrlen = titlelen * MCU_charsize(isunicode);
		if (isunicode)
		{
			newmenu = new char[t_titlestrlen + menuitemlen];
			CFStringGetCharacters(cftitlestr, CFRangeMake(0, titlelen),
			                      (UniChar *)newmenu);
		}
		else
		{
			newmenu = new char[t_titlestrlen + menuitemlen + 1];
			CFStringGetCString(cftitlestr, newmenu, t_titlestrlen + 1, kCFStringEncodingMacRoman);
		}
		memcpy(&newmenu[t_titlestrlen],menuitemname, menuitemlen);
		delete menuitemname;
		
		menupick = newmenu;
		menupicklen = t_titlestrlen + menuitemlen;
	}
	s.set(menupick,menupicklen);
}

static void getmacmenuitemtextfromaccelerator(MenuHandle menu, uint2 key, uint1 mods, MCString &s, bool isunicode, bool issubmenu)
{
	uint2 itemcount = CountMenuItems(menu);
	for (uint2 i = 1; i <= itemcount; i++)
	{
		MenuRef submenu;
		GetMenuItemHierarchicalMenu(menu, i, &submenu);
		if (submenu != NULL)
		{
			getmacmenuitemtextfromaccelerator(submenu, key, mods, s, isunicode, true);
			if (s.getstring() != NULL)
				return;
		}
		else
		{
			uint2 t_key = 0;
			GetMenuItemCommandKey(menu, i, false, &t_key);
			int2 t_glyph;
			GetMenuItemKeyGlyph(menu, i, &t_glyph);
			if (t_glyph != kMenuNullGlyph)
				t_key = MCMacGlyphToKeysym(t_glyph);
			if (t_key == key)
			{
				uint1 t_macmods = 0;
				GetMenuItemModifiers(menu, i, &t_macmods);
				uint1 t_mods = 0;
				if (t_macmods & kMenuShiftModifier)
					t_mods |= MS_SHIFT;
				if (t_macmods & kMenuOptionModifier)
					t_mods |= MS_ALT;
				if (t_macmods & kMenuControlModifier)
					t_mods |= MS_MAC_CONTROL;
				if (!(t_macmods & kMenuNoCommandModifier))
					t_mods |= MS_CONTROL;
				
				if (t_mods == mods)
				{
					getmacmenuitemtext(menu, i, s, issubmenu, isunicode);
					return;
				}
			}
		}
	}
}


Bool MCButton::macfindmenu(bool p_just_for_accel)
{
	// MW-2011-02-08: [[ Bug 9384 ]] Only create the menu if we are using it next. 'findmenu'
	//   is often called just to keep accelerators in sync.
	if (bMenuID == 0 && !p_just_for_accel)
	{
		MCScreenDC *pms = (MCScreenDC *)MCscreen;
		//call MCScreenDC class routine to get an available
		//menu id.  Only the Application's Main menus do not
		//need to get usable id from this routine Any button
		//based menus, such as pop-up, options menus are
		//restricted to the available id to avoid duplication
		//of the meun ids.
		int2 newMenuID = pms->allocateSubMenuID(False);
		if (newMenuID == 0)
			return False;
		
		// MW-2012-02-17: [[ IntrinsicUnicode ]] Fetch the label and its encoding.
		MCString smenuname;
		bool isunicode;
		getlabeltext(smenuname, isunicode);
		
		// MW-2012-02-17: Update to use CreateNewMenu / CFStringCreateWithBytes since
		//   we don't have a valid charset of font any more.
		MenuHandle mh;
		mh = nil;
		CreateNewMenu(newMenuID, 0, &mh);
		if (mh != nil)
		{
			
			CFStringRef t_menu_title;
			t_menu_title = CFStringCreateWithBytes(kCFAllocatorDefault, (const UInt8 *)smenuname . getstring(), smenuname . getlength(), isunicode ? kCFStringEncodingUTF16 : kCFStringEncodingMacRoman, False);
			SetMenuTitleWithCFString(mh, t_menu_title);
			
			MCString s;
			getmenustring(s);
			if (pms->addMenuItemsAndSubMenu(newMenuID, mh, this, menumode))
			{
				bMenuID = newMenuID;
				InsertMenu(mh, -1); //-1 = insert sub-menu or popup
			}
		}
		else
		{ // delete the id only, since we can't create a menu.
			pms->freeMenuAndID(newMenuID);
			return False;
		}
	}
	
	return True;
}

void MCButton::macopenmenu(void)
{
	MenuHandle mh = GetMenuHandle(bMenuID);
	if (mh == NULL)
		return;   //can't find the menu to display, bail out
	MCRectangle trect;
	long result;
	MCScreenDC *pms = (MCScreenDC *)MCscreen;
	
	int4 tmenux,tmenuy;
	tmenux = tmenuy = 0;
	switch (menumode)
	{
		case WM_COMBO:
		case WM_OPTION:
			trect = MCU_recttoroot(MCmousestackptr, rect);
			tmenux = trect.x;
			tmenuy = trect.y;
			break;
		case WM_PULLDOWN:
			trect = MCU_recttoroot(MCmousestackptr, rect);
			tmenux = trect.x;
			tmenuy = trect.y+trect.height + 1;
			break;
		case WM_CASCADE:
			trect = MCU_recttoroot(MCmousestackptr, rect);
			tmenux = trect.x + trect.width + 1;
			tmenuy = trect.y;
			break;
		case WM_POPUP:
		default:
			trect.x = MCmousex + 1;
			trect.y = MCmousey + 1;
			trect = MCU_recttoroot(MCmousestackptr, trect);
			tmenux = trect.x;
			tmenuy = trect.y;
			break;
	}
	
	// IM-2013-08-16: [[ ResIndependence ]] scale menu location to device coords
	MCGFloat t_scale;
	t_scale = MCResGetDeviceScale();
	
	tmenux = tmenux * t_scale;
	tmenuy = tmenuy * t_scale;
	
	// MW-2007-12-11: [[ Bug 5670 ]] Make sure we notify things higher up in the call chain
	//   that the mdown actually did result in a menu being popped up!
	extern bool MCosxmenupoppedup;
	MCosxmenupoppedup = true;
	
	switch (menumode)
	{
		case WM_COMBO:
		case WM_OPTION:
			//= MAC's pop-up menu, displayed at the control/button location
			layer_redrawall();
			if (MCModeMakeLocalWindows())
				result = PopUpMenuSelect(mh, tmenuy, tmenux, menuhistory);
			else
				result = MCModePopUpMenu((MCMacSysMenuHandle)mh, tmenux - trect . x + rect . x, tmenuy - trect . y + rect . y, menuhistory, MCmousestackptr);
#ifdef OLD_MAC
			allowmessages(False);
			pms->clearmdown(menubutton);
			allowmessages(True);
#endif
			if (result > 0)
			{
				setmenuhistoryprop(LoWord(result));
				//low word of result from PopUpMenuSelect() is the item selected
				//high word contains the menu id
				MCString slabel;
				getmacmenuitemtext(mh, menuhistory, slabel, False, hasunicode());
				delete label;
				label = (char *)slabel.getstring();
				labelsize = slabel.getlength();
				flags |= F_LABEL;
				Exec_stat es = message_with_args(MCM_menu_pick, slabel);
				if (es == ES_NOT_HANDLED || es == ES_PASS)
					message_with_args(MCM_mouse_up, menubutton);
			}
			else
				message_with_args(MCM_mouse_release, menubutton);
			state &= ~(CS_MFOCUSED | CS_ARMED | CS_HILITED);
			layer_redrawall();
			break;
		case WM_PULLDOWN:
		case WM_CASCADE:
		case WM_POPUP: //= MAC's context menu, Menu displyed at the mouse loc
			if (MCModeMakeLocalWindows())
				result = PopUpMenuSelect(mh, tmenuy, tmenux, 0);
			else
			{
				int32_t x, y;
				if (menumode == WM_POPUP)
				{
					x = MCmousex + 1;
					y = MCmousey + 1;
				}
				else
				{
					x = tmenux - trect . x + rect . x;
					y = tmenuy - trect . y + rect . y;
				}
				
				result = MCModePopUpMenu((MCMacSysMenuHandle)mh, x, y, 0, MCmousestackptr);
			}
#ifdef OLD_MAC
			allowmessages(False);
			pms->clearmdown(menubutton);
			allowmessages(True);
#endif
			if (result > 0)
			{ //user selected something
				MenuHandle mhandle = GetMenuHandle(HiWord(result));
				setmenuhistoryprop(LoWord(result));
				MCString smenustring;
				getmacmenuitemtext(mhandle, menuhistory, smenustring, mhandle != mh, hasunicode());
				Exec_stat es = message_with_args(MCM_menu_pick, smenustring);
				delete (char *)smenustring.getstring();
				if (es == ES_NOT_HANDLED || es == ES_PASS)
					message_with_args(MCM_mouse_up, menubutton);
			}
			state &= ~(CS_MFOCUSED | CS_ARMED | CS_HILITED);
			break;
		default:
			break;
	}
	
	// MW-2011-02-08: [[ Bug 9384 ]] Free the Mac menu after use since we don't need
	//   it lingering around.
	if (bMenuID != 0)
	{
		MCScreenDC *pms = (MCScreenDC *)MCscreen;
		pms->freeMenuAndID(bMenuID, this);
		bMenuID = 0;
	}
}

void MCButton::macfreemenu(void)
{
	if (bMenuID != 0)
	{
		//free button's menu and its id
		MCScreenDC *pms = (MCScreenDC *)MCscreen;
		pms->freeMenuAndID(bMenuID, this);
		bMenuID = 0;
	}
}

void MCButton::getmacmenuitemtextfromaccelerator(short menuid, uint2 key, uint1 mods, MCString &s, bool isunicode, bool issubmenu)
{
	::getmacmenuitemtextfromaccelerator(GetMenu(menuid), key, mods, s, isunicode, issubmenu);
}
#endif

////////////////////////////////////////////////////////////////////////////////

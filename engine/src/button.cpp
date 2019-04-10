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

#include "prefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "mcio.h"


#include "sellst.h"
#include "util.h"
#include "font.h"
#include "date.h"
#include "dispatch.h"
#include "stack.h"
#include "card.h"
#include "group.h"
#include "cdata.h"
#include "image.h"
#include "button.h"
#include "field.h"
#include "stacklst.h"
#include "undolst.h"
#include "mcerror.h"
#include "param.h"
#include "objectstream.h"
#include "parentscript.h"
#include "mode.h"
#include "globals.h"
#include "mctheme.h"
#include "osspec.h"
#include "redraw.h"
#include "menuparse.h"
#include "objptr.h"
#include "stacksecurity.h"

#include "exec.h"

#include "stackfileformat.h"

uint2 MCButton::mnemonicoffset = 1;
MCRectangle MCButton::optionrect = {0, 0, 12, 8};
uint4 MCButton::clicktime;
uint2 MCButton::menubuttonheight = 4;
Boolean MCButton::starthilite;
uint2 MCButton::starttab;
MCImage *MCButton::macrb = NULL;
MCImage *MCButton::macrbtrack = NULL;
MCImage *MCButton::macrbhilite = NULL;
MCImage *MCButton::macrbhilitetrack = NULL;
uint2 MCButton::focusedtab = MAXUINT2;

bool MCmenupoppedup = false;

const Keynames MCButton::button_keys[] =
    {
        {XK_F1, "f1"},
        {XK_F2, "f2"},
        {XK_F3, "f3"},
        {XK_F4, "f4"},
        {XK_F5, "f5"},
        {XK_F6, "f6"},
        {XK_F7, "f7"},
        {XK_F8, "f8"},
        {XK_F9, "f9"},
        {XK_F10, "f10"},
        {XK_F11, "f11"},
        {XK_L1, "l1"},
        {XK_F12, "f12"},
        {XK_L2, "l2"},
        {XK_F13, "f13"},
        {XK_L3, "l3"},
        {XK_F14, "f14"},
        {XK_L4, "l4"},
        {XK_F15, "f15"},
        {XK_L5, "l5"},
        {XK_F16, "f16"},
        {XK_L6, "l6"},
        {XK_F17, "f17"},
        {XK_L7, "l7"},
        {XK_F18, "f18"},
        {XK_L8, "l8"},
        {XK_F19, "f19"},
        {XK_L9, "l9"},
        {XK_F20, "f20"},
        {XK_L10, "l10"},
        {XK_F21, "f21"},
        {XK_R1, "r1"},
        {XK_F22, "f22"},
        {XK_R2, "r2"},
        {XK_F23, "f23"},
        {XK_R3, "r3"},
        {XK_F24, "f24"},
        {XK_R4, "r4"},
        {XK_F25, "f25"},
        {XK_R5, "r5"},
        {XK_F26, "f26"},
        {XK_R6, "r6"},
        {XK_F27, "f27"},
        {XK_R7, "r7"},
        {XK_F28, "f28"},
        {XK_R8, "r8"},
        {XK_F29, "f29"},
        {XK_R9, "r9"},
        {XK_F30, "f30"},
        {XK_R10, "r10"},
        {XK_F31, "f31"},
        {XK_R11, "r11"},
        {XK_F32, "f32"},
        {XK_R12, "r12"},
        {XK_R13, "r13"},
        {XK_F33, "f33"},
        {XK_F34, "f34"},
        {XK_R14, "r14"},
        {XK_F35, "f35"},
        {XK_R15, "r15"},
        {XK_BackSpace, "backspace"},
        {XK_Tab, "tab"},
        {XK_Linefeed, "linefeed"},
        {XK_Clear, "clear"},
        {XK_Return, "return"},
        {XK_Pause, "pause"},
        {XK_Scroll_Lock, "scroll_lock"},
        {XK_Sys_Req, "sys_req"},
        {XK_Escape, "escape"},
        {XK_Delete, "delete"},
        {XK_Home, "home"},
        {XK_Left, "left"},
        {XK_Up, "up"},
        {XK_Right, "right"},
        {XK_Down, "down"},
        {XK_End, "end"},
        {XK_Prior, "page_up"},
		{XK_Prior, "pgup"},
        {XK_Next, "page_down"},
		{XK_Next, "pgdown"},
        {XK_Begin, "begin"},
        {XK_Select, "select"},
        {XK_Print, "print"},
        {XK_Execute, "execute"},
        {XK_Insert, "insert"},
        {XK_Undo, "undo"},
        {XK_Redo, "redo"},
        {XK_Menu, "menu"},
        {XK_Find, "find"},
        {XK_Cancel, "cancel"},
        {XK_Break, "break"},
        {XK_Mode_switch, "mode_switch"},
        {XK_script_switch, "script_switch"},
        {XK_Num_Lock, "num_lock"},
        {XK_osfHelp, "help"},
        {XK_KP_Space, "kp_space"},
        {XK_KP_Tab, "kp_tab"},
        {XK_KP_Enter, "kp_enter"},
		{XK_KP_Enter, "enter"},
        {XK_KP_F1, "kp_f1"},
        {XK_KP_F2, "kp_f2"},
        {XK_KP_F3, "kp_f3"},
        {XK_KP_F4, "kp_f4"},
        {XK_KP_Begin, "kp_begin"},
        {XK_KP_Home, "kp_home"},
        {XK_KP_Left, "kp_left"},
        {XK_KP_Up, "kp_up"},
        {XK_KP_Right, "kp_right"},
        {XK_KP_Down, "kp_down"},
        {XK_KP_End, "kp_end"},
        {XK_KP_Insert, "kp_insert"},
        {XK_KP_Delete, "kp_delete"},
        {XK_KP_Prior, "kp_page_up"},
        {XK_KP_Next, "kp_page_down"},
        {XK_KP_Equal, "kp_equal"},
        {XK_KP_Multiply, "kp_multiply"},
        {XK_KP_Add, "kp_add"},
        {XK_KP_Separator, "kp_separator"},
        {XK_KP_Subtract, "kp_subtract"},
        {XK_KP_Decimal, "kp_decimal"},
        {XK_KP_Divide, "kp_divide"},
        {XK_KP_0, "kp_0"},
        {XK_KP_1, "kp_1"},
        {XK_KP_2, "kp_2"},
        {XK_KP_3, "kp_3"},
        {XK_KP_4, "kp_4"},
        {XK_KP_5, "kp_5"},
        {XK_KP_6, "kp_6"},
        {XK_KP_7, "kp_7"},
        {XK_KP_8, "kp_8"},
        {XK_KP_9, "kp_9"},
		{XK_space, "space"},
        {0, ""},
    };
	
typedef struct
{
	uint1 modifier;
	uint1 token_length;
	const char *token;
} ModKeyToken;

////////////////////////////////////////////////////////////////////////////////

MCPropertyInfo MCButton::kProperties[] =
{
	DEFINE_RW_OBJ_ENUM_PROPERTY(P_STYLE, InterfaceButtonStyle, MCButton, Style)
	DEFINE_RW_OBJ_PROPERTY(P_AUTO_ARM, Bool, MCButton, AutoArm)
	DEFINE_RW_OBJ_PROPERTY(P_AUTO_HILITE, Bool, MCButton, AutoHilite)
	DEFINE_RW_OBJ_PROPERTY(P_ARM_BORDER, Bool, MCButton, ArmBorder)
	DEFINE_RW_OBJ_PROPERTY(P_ARM_FILL, Bool, MCButton, ArmFill)
	DEFINE_RW_OBJ_PROPERTY(P_HILITE_BORDER, Bool, MCButton, HiliteBorder)
	DEFINE_RW_OBJ_PROPERTY(P_HILITE_FILL, Bool, MCButton, HiliteFill)
	DEFINE_RW_OBJ_PROPERTY(P_SHOW_HILITE, Bool, MCButton, ShowHilite)
	DEFINE_RW_OBJ_PROPERTY(P_ARM, Bool, MCButton, Arm)

	DEFINE_RW_OBJ_PROPERTY(P_SHARED_HILITE, Bool, MCButton, SharedHilite)
	DEFINE_RW_OBJ_PROPERTY(P_SHOW_ICON, Bool, MCButton, ShowIcon)
	DEFINE_RW_OBJ_PROPERTY(P_SHOW_NAME, Bool, MCButton, ShowName) 

	DEFINE_RW_OBJ_NON_EFFECTIVE_PROPERTY(P_LABEL, String, MCButton, Label)
	DEFINE_RO_OBJ_EFFECTIVE_PROPERTY(P_LABEL, String, MCButton, Label)
	DEFINE_RW_OBJ_NON_EFFECTIVE_PROPERTY(P_UNICODE_LABEL, BinaryString, MCButton, UnicodeLabel)
	DEFINE_RO_OBJ_EFFECTIVE_PROPERTY(P_UNICODE_LABEL, BinaryString, MCButton, UnicodeLabel)
	DEFINE_RW_OBJ_PROPERTY(P_LABEL_WIDTH, UInt16, MCButton, LabelWidth)
	DEFINE_RW_OBJ_PROPERTY(P_FAMILY, UInt16, MCButton, Family)
	DEFINE_RW_OBJ_PROPERTY(P_VISITED, Bool, MCButton, Visited)
	DEFINE_RW_OBJ_PROPERTY(P_MENU_HISTORY, UInt16, MCButton, MenuHistory)
	DEFINE_RW_OBJ_PROPERTY(P_MENU_LINES, OptionalUInt16, MCButton, MenuLines)
	DEFINE_RW_OBJ_PROPERTY(P_MENU_BUTTON, UInt16, MCButton, MenuButton)
	DEFINE_RW_OBJ_PROPERTY(P_MENU_NAME, Name, MCButton, MenuName)
	DEFINE_RW_OBJ_PROPERTY(P_ACCELERATOR_TEXT, String, MCButton, AcceleratorText)
	DEFINE_RO_OBJ_PROPERTY(P_UNICODE_ACCELERATOR_TEXT, BinaryString, MCButton, UnicodeAcceleratorText)
	DEFINE_RW_OBJ_PROPERTY(P_ACCELERATOR_KEY, OptionalString, MCButton, AcceleratorKey)

	DEFINE_RW_OBJ_SET_PROPERTY(P_ACCELERATOR_MODIFIERS, InterfaceButtonAcceleratorModifiers, MCButton, AcceleratorModifiers)
	
	DEFINE_RW_OBJ_PROPERTY(P_MNEMONIC, UInt16, MCButton, Mnemonic)
	DEFINE_RO_OBJ_PROPERTY(P_FORMATTED_WIDTH, Int16, MCButton, FormattedWidth)
	DEFINE_RO_OBJ_PROPERTY(P_FORMATTED_HEIGHT, Int16, MCButton, FormattedHeight)
	DEFINE_RW_OBJ_PROPERTY(P_DEFAULT, Bool, MCButton, Default)
	DEFINE_RW_OBJ_PROPERTY(P_TEXT, String, MCButton, Text)
	DEFINE_RW_OBJ_PROPERTY(P_UNICODE_TEXT, BinaryString, MCButton, UnicodeText)
    // SN-2014-06-25 [[ IconGravity ]]
    DEFINE_RW_OBJ_ENUM_PROPERTY(P_ICON_GRAVITY, InterfaceButtonIconGravity, MCButton, IconGravity)
	
	DEFINE_RW_OBJ_CUSTOM_PROPERTY(P_ICON, InterfaceButtonIcon, MCButton, Icon)
	DEFINE_RW_OBJ_CUSTOM_PROPERTY(P_ARMED_ICON, InterfaceButtonIcon, MCButton, ArmedIcon)
	DEFINE_RW_OBJ_CUSTOM_PROPERTY(P_DISABLED_ICON, InterfaceButtonIcon, MCButton, DisabledIcon)
	DEFINE_RW_OBJ_CUSTOM_PROPERTY(P_HILITED_ICON, InterfaceButtonIcon, MCButton, HiliteIcon)
	DEFINE_RW_OBJ_CUSTOM_PROPERTY(P_VISITED_ICON, InterfaceButtonIcon, MCButton, VisitedIcon)
	DEFINE_RW_OBJ_CUSTOM_PROPERTY(P_HOVER_ICON, InterfaceButtonIcon, MCButton, HoverIcon)
    
    DEFINE_RW_OBJ_PART_CUSTOM_PROPERTY(P_HILITE, InterfaceTriState, MCButton, Hilite)
    DEFINE_RW_OBJ_ENUM_PROPERTY(P_MENU_MODE, InterfaceButtonMenuMode, MCButton, MenuMode)
    
    DEFINE_WO_OBJ_CHAR_CHUNK_PROPERTY(P_HILITE, Bool, MCButton, Hilite)
    DEFINE_WO_OBJ_CHAR_CHUNK_PROPERTY(P_DISABLED, Bool, MCButton, Disabled)
    DEFINE_WO_OBJ_CHAR_CHUNK_PROPERTY(P_ENABLED, Bool, MCButton, Enabled)    
};

MCObjectPropertyTable MCButton::kPropertyTable =
{
	&MCControl::kPropertyTable,
	sizeof(kProperties) / sizeof(kProperties[0]),
	&kProperties[0],
};

////////////////////////////////////////////////////////////////////////////////

MCButton::MCButton()
{
	flags |= F_STANDARD | F_TRAVERSAL_ON | F_HILITE_BORDER | F_HILITE_FILL
	         | F_SHOW_NAME | F_ALIGN_CENTER | F_AUTO_HILITE
	         | F_ARM_BORDER | F_SHARED_HILITE;
	bdata = NULL;
	icons = NULL;
	label = MCValueRetain(kMCEmptyString);
	labelwidth = 0;
	menubutton = Button1;
	menumode = WM_CLOSED;
	menuname = MCValueRetain(kMCEmptyName);
	menustring = MCValueRetain(kMCEmptyString);
	menuhistory = 1;
	menucontrol = MENUCONTROL_NONE;
	menulines = DEFAULT_MENU_LINES;
	menuhasitemtags = false;
	m_system_menu = NULL;
	entry = NULL;
	tabs = MCValueRetain(kMCEmptyArray);
	acceltext = MCValueRetain(kMCEmptyString);
	accelkey = accelmods = 0;
	mnemonic = 0;
	family = 0;
	ishovering = False;
    
    // MW-2014-06-19: [[ IconGravity ]] By default buttons use legacy behavior.
    m_icon_gravity = kMCGravityNone;
    
    // MM-2014-07-31: [[ ThreadedRendering ]] Used to ensure the default button animate message is only posted from a single thread.
    m_animate_posted = false;
	
	m_menu_handler = nil;
}

MCButton::MCButton(const MCButton &bref) : MCControl(bref)
{
	if (bref.icons != NULL)
	{
		icons = new (nothrow) iconlist;
		memcpy(icons, bref.icons, sizeof(iconlist));
		icons->curicon = NULL;
	}
	else
		icons = NULL;
	bdata = NULL;
	label = MCValueRetain(bref.label);
	labelwidth = bref.labelwidth;
	menuhistory = bref.menuhistory;
	menulines = bref.menulines;
	menubutton = bref.menubutton;
	menumode = bref.menumode;
	menuname = MCValueRetain(bref.menuname);
	menucontrol = bref.menucontrol;
	menuhasitemtags = bref.menuhasitemtags;
	menustring = MCValueRetain(bref.menustring);
	m_system_menu = NULL;
	entry = NULL;
	tabs = MCValueRetain(kMCEmptyArray);
	acceltext = MCValueRetain(bref.acceltext);
	accelkey = bref.accelkey;
	accelmods = bref.accelmods;
	mnemonic = bref.mnemonic;
	ishovering = False;

	if (bref.bdata != NULL)
	{
		MCCdata *bptr = bref.bdata;
		do
		{
			MCCdata *newbdata = new (nothrow) MCCdata(*bptr);
			newbdata->appendto(bdata);
			bptr = (MCCdata *)bptr->next();
		}
		while (bptr != bref.bdata);
	}
	family = bref.family;
    
    // MW-2014-06-19: [[ IconGravity ]] Copy the other buttons gravity
    m_icon_gravity = bref.m_icon_gravity;
    
    // MM-2014-07-31: [[ ThreadedRendering ]] Used to ensure the default button animate message is only posted from a single thread.
    m_animate_posted = false;

	m_menu_handler = nil;
}

MCButton::~MCButton()
{
	// OK-2009-04-30: [[Bug 7517]] - Ensure the button is actually closed before deletion, otherwise dangling references may still exist,
	// particuarly if the button had icons.
	while (opened)
		close();

	delete icons;
	freemenu(True);
	MCValueRelease(acceltext);
	MCValueRelease(label);
	MCValueRelease(menuname);
	MCValueRelease(menustring);
	MCValueRelease(tabs);
	if (accelkey != 0)
		MCstacks->deleteaccelerator(this, NULL);
	while (bdata != NULL)
	{
		MCCdata *bptr = (MCCdata *)bdata->remove
		                (bdata);
		delete bptr;
	}
}

void MCButton::removelink(MCObject *optr)
{
	if (menu.IsBound() && optr == menu)
    {
        if (this == MCmenuobjectptr)
            MCmenuobjectptr = nil;
        
        MCValueAssign(menuname, kMCEmptyName);
        menu = nil;
    }
}

bool MCButton::imagechanged(MCImage *p_image, bool p_deleting)
{
	if (icons != nil && p_image == icons->curicon)
	{
		if (p_deleting)
			icons->curicon = nil;
		layer_redrawall();

		return true;
	}

	return false;
}

Chunk_term MCButton::gettype() const
{
	return CT_BUTTON;
}

const char *MCButton::gettypestring()
{
	return MCbuttonstring;
}

bool MCButton::visit_self(MCObjectVisitor* p_visitor)
{
	return p_visitor -> OnButton(this);
}

void MCButton::open()
{
    MCControl::open();

	// MW-2011-02-08: [[ Bug 9382 ]] Make sure we reset icons when opening and the state
	//   has changed (i.e. background transition has occurred).
	uint32_t t_old_state;
	t_old_state = state;
	switch(gethilite(0).value)
	{
	case kMCTristateTrue:
		state |= CS_HILITED;
		state &= ~CS_MIXED;
		break;
	case kMCTristateFalse:
		state &= ~(CS_HILITED | CS_MIXED);
		break;
	case kMCTristateMixed:
		state &= ~CS_HILITED;
		state |= CS_MIXED;
		break;
	}
	if (opened == 1 || state != t_old_state)
		reseticon();
	
	if (opened == 1)
	{
		if (getflag(F_DEFAULT))
		{
			getcard() -> setdefbutton(this);
			state |= CS_SHOW_DEFAULT;
		}
		setupmnemonic();
		if (getstyleint(flags) == F_MENU)
		{
			// MW-2011-02-08:[[ Bug 9384 ]] Make sure we don't 'findmenu' for things that don't have them.
			//   i.e. popup/option/cascade types.
			switch (menumode)
			{
			case WM_TOP_LEVEL:
				MCValueRelease(tabs);
				/* UNCHECKED */ MCStringSplit(menustring, MCSTR("\n"), nil, kMCStringOptionCompareExact, tabs);
				break;
			case WM_COMBO:
				createentry();
				break;
			case WM_POPUP:
			case WM_OPTION:
			case WM_CASCADE:
				break;
			default:
				findmenu(true);
				if (!MCNameIsEmpty(menuname) && menu.IsValid())
					menu->installaccels(getstack());
				break;
			}
		}
		if (accelkey != 0)
			MCstacks->addaccelerator(this, getstack(), accelkey, accelmods);
	}
}

void MCButton::close()
{
	MCControl::close();
	if (opened == 0)
	{
		if (icons != NULL && icons->curicon != NULL)
		{
			icons->curicon->close();
			icons->curicon = NULL;
		}
		
		// Always make sure we unlink ourselves from the default button
		// points on card - just in case one isn't updated somewhere else.
		state &= ~CS_SHOW_DEFAULT;
		getcard()->freedefbutton(this);
		
		clearmnemonic();
		MCValueAssign(tabs, kMCEmptyArray);

		// AL-2013-01-13 [[ Bug 11363 ]] Close menu on button close. 
		if (state & CS_SUBMENU)
			closemenu(True, True);

		freemenu(False);
		if (entry != NULL)
		{
			MCundos->freestate();
			deleteentry();
		}
		if (accelkey != 0)
			MCstacks->deleteaccelerator(this, getstack());

		ishovering = False;
	}
}

void MCButton::kfocus()
{
	if ((!IsMacLF() || state & CS_SHOW_DEFAULT
	        || flags & F_AUTO_ARM || entry != NULL)
	        && flags & F_TRAVERSAL_ON && !(state & CS_KFOCUSED))
	{
		// Get the current transient rect
		uint2 t_old_trans;
		t_old_trans = gettransient();

		state |= CS_KFOCUSED | CS_ARMED;

		reseticon();

		bool t_need_redraw;
		t_need_redraw = false;
		if (entry != NULL)
		{
			entry->kfocus();
			t_need_redraw = true;
		}
		else if (getstyleint(flags) == F_STANDARD && flags & F_SHOW_BORDER)
		{
			if (!getstate(CS_SHOW_DEFAULT) && getcard()->getodefbutton() != NULL)
				state |= CS_SHOW_DEFAULT;
			getcard()->setdefbutton(this);
			t_need_redraw = true;
		}
		else if (getstyleint(flags) != F_MENU || menumode != WM_TOP_LEVEL)
			t_need_redraw = true;

		// MW-2011-08-18: [[ Layers ]] Invalidate the whole object, noting
		//   possible change in transient.
		if (t_need_redraw)
			layer_transientchangedandredrawall(t_old_trans);

		message(MCM_focus_in);
	}
}

Boolean MCButton::kfocusnext(Boolean top)
{
	if (((IsMacLF() && !(state & CS_SHOW_DEFAULT))
		 && !(flags & F_AUTO_ARM) && entry == NULL)
	        || !(flags & F_VISIBLE || showinvisible())
	        || !(flags & F_TRAVERSAL_ON) || state & CS_KFOCUSED || flags & F_DISABLED)
		return False;
	return True;
}

Boolean MCButton::kfocusprev(Boolean bottom)
{
	if (((IsMacLF() && !(state & CS_SHOW_DEFAULT))
		 && !(flags & F_AUTO_ARM) && entry == NULL)
	        || !(flags & F_VISIBLE || showinvisible())
	        || !(flags & F_TRAVERSAL_ON) || state & CS_KFOCUSED || flags & F_DISABLED)
		return False;
	return True;
}

void MCButton::kunfocus()
{
	if (state & CS_SUBMENU && menumode == WM_CASCADE)
		closemenu(True, True);
	if (state & CS_KFOCUSED)
	{
		uint2 t_old_trans;
		t_old_trans = gettransient();

		if (flags & F_AUTO_ARM && state & CS_ARMED && !(state & CS_SUBMENU))
			state &= ~CS_MFOCUSED;
		
		state &= ~(CS_KFOCUSED | CS_ARMED);
		
		reseticon();
		
		bool t_need_redraw;
		t_need_redraw = false;
		if (entry != NULL)
		{
			entry->kfocus();
			entry->kunfocus();
			entry->unselect(True, True);
			t_need_redraw = true;
		}

		if (getstyleint(flags) != F_MENU || menumode != WM_TOP_LEVEL)
		{
			if (getstate(CS_SHOW_DEFAULT) && !getflag(F_DEFAULT))
				state &= ~CS_SHOW_DEFAULT;
			t_need_redraw = true;
		}

		// MW-2011-08-18: [[ Layers ]] Invalidate the whole object, noting
		//   possible change in transient.
		if (t_need_redraw)
			layer_transientchangedandredrawall(t_old_trans);

		message(MCM_focus_out);
	}
}

Boolean MCButton::kdown(MCStringRef p_string, KeySym key)
{
	codepoint_t t_char;
	t_char = key & XK_Codepoint_mask;
	if (state & CS_SUBMENU)
	{
		if (MCObject::kdown(p_string, key))
			return True;
		MCButton *bptr;
		MCAutoStringRef t_pick;
		switch (key)
		{
		case XK_Escape:
			closemenu(True, True);
			return True;
		case XK_Tab:
		case XK_Left:
		case XK_Right:
			if (menu->kdown(p_string, key))
				return True;
			if (menumode == WM_CASCADE)
			{
				closemenu(True, key != XK_Left);
				return key == XK_Left;
			}
			if (menumode != WM_PULLDOWN)
				return False;
			uint2 tnum, newnum;
			getcard()->count(CT_MENU, CT_UNDEFINED, NULL, tnum, True);
			getcard()->count(CT_MENU, CT_UNDEFINED, this, newnum, True);
			while (True)
			{
				if (key == XK_Left)
					if (newnum == 1)
						newnum = tnum;
					else
						newnum--;
				else
					if (newnum == tnum)
						newnum = 1;
					else
						newnum++;
				bptr = (MCButton *)getcard()->getnumberedchild(newnum, CT_MENU, CT_UNDEFINED);
				if (bptr == this)
					break;
				if (bptr->menubutton == menubutton
				        && bptr->isvisible() && !bptr->isdisabled())
				{
					closemenu(True, True);
					bptr->message_with_args(MCM_mouse_down, menubutton);
					if (bptr->findmenu())
					{
						bptr->openmenu(True);
						return True;
					}
				}
			}
			break;
		case XK_Down:
			if (menu->kdown(p_string, key))
				return True;
			return menu->kfocusnext(False);
		case XK_Up:
			if (menu->kdown(p_string, key))
				return True;
			return menu->kfocusprev(False);
		case XK_WheelDown:
		case XK_WheelUp:
		case XK_WheelLeft:
		case XK_WheelRight:
			if (menu->getcontrols()->gettype() == CT_FIELD)
				if (menu->getcontrols()->kdown(p_string, key))
					return True;
			break;
		case XK_space:
		case XK_Return:
		case XK_KP_Enter:
			closemenu(False, True);
			menu->menukdown(p_string, key, &t_pick, menuhistory);

			// This check must be for null (not empty) because an empty pick
			// indicates that the function succeeded while a null pick means
			// that no menu responded to the event.
			if (*t_pick != nil)
			{
				if (menumode == WM_OPTION || menumode == WM_COMBO)
				{
					MCValueAssign(label, *t_pick);
                    // SN-2014-08-05: [[ Bug 13100 ]] An empty label is not an issue,
                    //  we need to rely on the F_LABEL flag
                    flags |= F_LABEL;
					if (entry != NULL)
						entry->settext(0, *t_pick, False);
					Exec_stat es = handlemenupick(*t_pick, nil);
					if (es == ES_NOT_HANDLED || es == ES_PASS)
						message_with_args(MCM_mouse_up, menubutton);
					// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
					layer_redrawall();
				}
				else
				{
					if (!(state & CS_IGNORE_MENU))
						docascade(*t_pick);
				}
			}
			else
				message_with_args(MCM_mouse_release, menubutton);
			state &= ~CS_IGNORE_MENU;
			if (MCmenuobjectptr == this)
				MCmenuobjectptr = nil;
			return True;
		default:	
			MCButton *mbptr = menu->findmnemonic(t_char);
			if (mbptr != NULL)
			{
				closemenu(False, True);
				if (!MCNameIsEmpty(menuname))
					mbptr->activate(False, t_char);
				else
				{
					MCStringRef t_label;
					if (mbptr->getmenuhastags())
						t_label = MCNameGetString(mbptr->getname());
					else
						t_label = mbptr->getlabeltext();
	
					menu->menukdown(p_string, key, &t_pick, menuhistory);
					Exec_stat es = handlemenupick(t_label, nil);
					if (es == ES_NOT_HANDLED || es == ES_PASS)
						message_with_args(MCM_mouse_up, menubutton);
				}
				if (MCmenuobjectptr == this)
					MCmenuobjectptr = nil;
				return True;
			}
			else
			{
				if (MCmodifierstate & MS_MOD1)
				{
					mbptr = MCstacks->findmnemonic(t_char);
					if (mbptr != NULL && mbptr->isvisible() && !mbptr->isdisabled())
					{
						closemenu(True, True);
						mbptr->message_with_args(MCM_mouse_down, menubutton);
						if (mbptr->findmenu())
						{
							mbptr->openmenu(True);
							return True;
						}
					}
				}
			}
		}
		return True;
	}
	else
	{
		if (entry != NULL)
			return entry->kdown(p_string, key);
		if (MCObject::kdown(p_string, key))
			return True;
		if ((((key == XK_Right || key == XK_space
		        || key == XK_Return || key == XK_KP_Enter)
		        && (menumode == WM_CASCADE || menumode == WM_OPTION))
			 || (key == XK_Down && menumode != WM_CASCADE))
		        && state & CS_KFOCUSED && findmenu())
		{
			openmenu(True);
			return True;
		}
		if ((key == XK_space || ((state & CS_SHOW_DEFAULT || state & CS_ARMED)
								 && (key == XK_Return || key == XK_KP_Enter)))
		        && !(MCmodifierstate & MS_MOD1))
		{
			activate(False, t_char);
			return True;
		}
	}
	return False;
}

Boolean MCButton::kup(MCStringRef p_string, KeySym key)
{
	if (entry != NULL)
		return entry->kup(p_string, key);
	if (MCObject::kup(p_string, key))
		return True;
	if (state & CS_SUBMENU)
		return menu->kup(p_string, key);
	return False;
}

Boolean MCButton::mfocus(int2 x, int2 y)
{
	// SMR 594 do menu stuff before visibility check

	if (state & CS_MENU_ATTACHED)
		return MCObject::mfocus(x, y);
	if (state & CS_SUBMENU)
	{
		mx = x;
		my = y;
		int2 tx = x;
		int2 ty = y;
		MCStack *sptr = menumode == WM_CASCADE ? getstack() : MCmousestackptr;
		
		//TS-2007-13-11 - [[Bug 5507]] - on linux it is possible to have sptr == NULL which causes a crash.
		// This is only when you drag a menu really quickly so the MCmousestackptr == NULL.
		if ( sptr == NULL)
			return False;

        if (!(menu.IsValid()))
            return False;
        
		sptr->translatecoords(menu, tx, ty);
		MCRectangle trect = sptr->getrect();
		Boolean handled = menu->mfocus(tx, ty);
		tx = x + trect.x;
		ty = y + trect.y;
		if (!handled)
		{
			MCCard *cptr = getcard();
			switch (menumode)
			{
			case WM_PULLDOWN:
				uint2 i;
				if (MCU_point_in_rect(menu->getrect(), tx, ty)
				        || !MCU_point_in_rect(cptr->getrect(), x, y)
				        || MCU_point_in_rect(rect, x, y))
					return True;
				for (cptr->count(CT_MENU, CT_UNDEFINED, NULL, i, True) ; i ; i--)
				{
					MCButton *bptr = (MCButton *)cptr->getnumberedchild(i, CT_MENU, CT_UNDEFINED);
					if (bptr == this)
						continue;
					
					// OK-2010-02-24: [[Bug 8628]] - Added clause to prevent this menu switching happening when bptr is a 
					// normal mac pulldown menu. 
					if (bptr->menubutton == menubutton
					        && bptr->parent == parent
					        && bptr->isvisible() && !bptr->isdisabled()
					        && MCU_point_in_rect(bptr->getrect(), x, y)
					        && bptr->mfocus(x, y) && bptr->findmenu()
							&& !(IsMacLFAM() && (bptr -> getflag(F_SHOW_BORDER)) != 0)
						)
					{
						state &= ~(CS_MFOCUSED | CS_SUBMENU | CS_ARMED);
						ishovering = False;
						reseticon();
						// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
						layer_redrawall();
						menu->setstate(True, CS_KFOCUSED); // override state
						menu->kunfocus();
						menu->close();
						menudepth--;
						MCdispatcher->addmenu(bptr);
                        // MW-2014-06-10: [[ Bug 12590 ]] Make sure we lock screen around the menu update message.
                        MCRedrawLockScreen();
						bptr->state |= CS_MFOCUSED;
						bptr->message_with_args(MCM_mouse_down, menubutton);
                        MCRedrawUnlockScreen();
						bptr->findmenu();
                        if (menudepth == 0)
                        {
                            MCmenuobjectptr = nullptr;
                        }
						bptr->openmenu(False);
						return True;
					}
				}
				break;
			case WM_CASCADE:
				// MW-2010-10-06: [[ Bug 9035 ]] We count the pointer as being inside the cascade button
				// if its vertical position is when inside a menuwindow.
				if (sptr -> menuwindow && y >= rect . y && y < rect . y + rect . height)
					return True;
				if (MCU_point_in_rect(rect, x, y))
					return True;
				if (flags & F_AUTO_ARM && MCU_point_in_rect(trect, tx, ty))
				{
					closemenu(False, True);
					state &= ~(CS_MFOCUSED | CS_ARMED);
					reseticon();
					getcard()->ungrab();
					return False;
				}
				break;
			case WM_OPTION:
				// OK-2007-08-07 : Bug 5287, a button with a menu name (stack panel) set will not have 
				// an associated field to contain items, so should break here to avoid crash.
				if ((MClook != LF_WIN95) || !MCNameIsEmpty(menuname))
					break;
			case WM_COMBO:
				// MW-2014-03-11: [[ Bug 11893 ]] Make sure we don't do anything to a stack panel.
				if (state & CS_MFOCUSED && MCNameIsEmpty(menuname))
				{
					uint2 fheight;
					fheight = gettextheight();
					MCField *fptr = (MCField *)menu->getcontrols();
					fptr->vscroll(my < rect.y + rect.height ? -fheight : fheight, True);
					fptr->resetscrollbars(True);
				}
				break;
			default:
				break;
			}
		}
		message_with_args(MCM_mouse_move, x, y);
		/* This would prevent buttons that are under the menu of this one
		from taking the focus away, but unfortunately this messes up
		grab at the card level if (menumode == WM_CASCADE) return
		handled; */
		return True;
	}
	if (!(flags & F_VISIBLE || showinvisible())
		|| (flags & F_DISABLED && getstack()->gettool(this) == T_BROWSE))
		return False;
	Tool tool = getstack()->gettool(this);

	if (tool == T_BROWSE || tool == T_HELP)
	{
		mx = x;
		my = y;
		if (entry != NULL && entry->mfocus(x, y))
			return True;
		if (getstyleint(flags) == F_MENU && menumode == WM_TOP_LEVEL
		        && IsMacLF() && state & CS_MFOCUSED)
		{
			// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
			layer_redrawall();
		}
		else
		{
			if (flags & F_AUTO_ARM)
			{
				if (MCU_point_in_rect(rect, x, y)
				        && MCU_point_in_rect(getcard()->getrect(), x, y))
				{
					if (!(state & CS_ARMED))
					{
						if (menumode == WM_CASCADE && findmenu())
						{
							getcard()->grab();
							mdown(menubutton);
						}
						state |= CS_MFOCUSED | CS_ARMED;
						getcard()->kfocusset(this);
					}
					message_with_args(MCM_mouse_move, x, y);
					return True;
				}
				else
				{
					if (state & CS_ARMED)
					{
						uint2 t_old_trans;
						t_old_trans = gettransient();
						state &= ~(CS_MFOCUSED | CS_ARMED | CS_KFOCUSED);
						ishovering = False;
						reseticon();
						getcard()->kfocusset(NULL);
						// MW-2011-08-18: [[ Layers ]] Invalidate the whole object, noting
						//   possible change in transient.
						layer_transientchangedandredrawall(t_old_trans);
					}
					return False;
				}
			}
			if (MCbuttonstate & Button1 && flags & F_AUTO_HILITE
			        && (menumode != WM_TOP_LEVEL || getstyleint(flags) != F_MENU)
			        && state & CS_MFOCUSED && !(state & CS_SELECTED))
			{
				if (MClook == LF_MOTIF || flags & F_SHOW_ICON ||
					(getstyleint(flags) != F_RADIO &&
					 getstyleint(flags) != F_CHECK))
				{
					if (MCU_point_in_rect(rect, x, y))
					{
						if ((state & CS_HILITED) != starthilite
						        && (starthilite || getstyleint(flags) != F_RADIO))
						{
							state ^= CS_HILITED;
							reseticon();
							// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
							layer_redrawall();
						}
					}
					else if ((state & CS_HILITED) == starthilite
						        && (starthilite || getstyleint(flags) != F_RADIO))
					{
						state ^= CS_HILITED;
						reseticon();
						// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
						layer_redrawall();
					}
					sethilite(0, (state & CS_HILITED) != 0);
				}
				else if (MCU_point_in_rect(rect, x, y) != starthilite)
				{
					starthilite = !starthilite;
					// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
					layer_redrawall();
				}
			}
			else if (!MCdispatcher -> isdragtarget() &&
				     ((getflag(F_HAS_ICONS) && icons -> iconids[CI_HOVER] != 0) ||
							 (MCcurtheme != NULL && (!ishovering || menumode == WM_TOP_LEVEL) && MCcurtheme->getthemepropbool(WTHEME_PROP_SUPPORTHOVERING)))
			         && MCU_point_in_rect(rect, x, y))
			{
				ishovering = True;
				reseticon();
				if (menumode == WM_TOP_LEVEL && ishovering && MCcurtheme->iswidgetsupported(WTHEME_TYPE_TAB))
				{
					int2 curx = rect.x + MCcurtheme->getmetric(WTHEME_METRIC_TABSTARTOFFSET);
					uint2 nfocusedtab = getmousetab(curx);
					if (nfocusedtab != focusedtab)
					{
						focusedtab = nfocusedtab;
						// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
						layer_redrawall();
					}
				}
				else
				{
					// MW-2011-08-30: [[ Layers ]] Potentially invalidate the whole object
					//   if flags and settings mean it is warranted.
					mayberedrawall();
				}
			}
		}
	}
	return MCControl::mfocus(x, y);
}

void MCButton::munfocus()
{
	if (entry != NULL)
		entry->munfocus();
	if (flags & F_AUTO_ARM)
	{
		state &= ~CS_ARMED;
		ishovering = False;
		reseticon();
		// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
		layer_redrawall();
	}
	else if (ishovering &&
					 ((getflag(F_HAS_ICONS) && icons->iconids[CI_HOVER] != 0) ||
					  (MCcurtheme != NULL && MCcurtheme->getthemepropbool(WTHEME_PROP_SUPPORTHOVERING))))
	{
		ishovering = False;
		reseticon();
		focusedtab = MAXUINT2;

		// MW-2011-08-30: [[ Layers ]] Potentially invalidate the whole object
		//   if flags and settings mean it is warranted.
		mayberedrawall();
	}
	if (state & CS_MFOCUSED)
	{
		state &= ~CS_MFOCUSED;
		if (flags & F_AUTO_HILITE)
		{
			state &= ~CS_HILITED;
			// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
			layer_redrawall();
		}
		if (sethilite(0, (state & CS_HILITED) != 0))
		{
			// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
			layer_redrawall();
		}
		starthilite = 0;
		state |= CS_MFOCUSED;  // restore state so MCControl:: can clear it
	}
	MCControl::munfocus();
}

bool MCButton::tabselectonmouseup()
{
	return IsMacEmulatedLF() ||
		(MCcurtheme && MCcurtheme->iswidgetsupported(WTHEME_TYPE_TAB) && MCcurtheme->getthemepropbool(WTHEME_PROP_TABSELECTONMOUSEUP));
}

Boolean MCButton::mdown(uint2 which)
{
	if (state & CS_MENU_ATTACHED)
		return MCObject::mdown(which);
	if (state & CS_MFOCUSED)
	{
		reseticon();

		// MW-2011-08-30: [[ Layers ]] Potentially invalidate the whole object
		//   if flags and settings mean it is warranted.
		mayberedrawall();
		if (flags & F_AUTO_ARM)
			message_with_args(MCM_mouse_down, which);
		return False;
	}
	state |= CS_MFOCUSED;
	if (state & CS_SUBMENU && (menubutton == 0 || (uint1)which == menubutton))
	{
        if (!(menu.IsValid()))
            return False;
        
        // SN-2014-08-26: [[ Bug 13201 ]] mx/my are now related to the button's rectangle,
        //  not the stack's rectangle anymore.
        // SN-2014-10-17: [[ Bug 13675 ]] mx/my refer to the button's rectangle on Mac only
        int2 t_right_limit, t_left_limit;
#ifdef _MACOSX
        t_left_limit = rect.width - MCscrollbarwidth;
        t_right_limit = rect.width;
#else
        t_left_limit = rect.x + rect.width - MCscrollbarwidth;
        t_right_limit = rect.x + rect.width;
#endif
        if (state & CS_SCROLLBAR && mx > t_left_limit
                && mx < t_right_limit)
		{
			menu->mdown(which);
			state |= CS_FIELD_GRAB;
			return True;
		}
		else
		{
			menu->kunfocus();
			return mfocus(mx, my);
		}
	}
	if ((!MCNameIsEmpty(menuname) || menu.IsValid() || getstyleint(flags) == F_MENU)
	        && (menubutton == 0 || (uint1)which == menubutton)
	        && (entry == NULL || !MCU_point_in_rect(entry->getrect(), mx, my))
	        && (getstack()->gettool(this) == T_BROWSE
	            || getstack()->gettool(this) == T_HELP))
	{
		message_with_args(MCM_mouse_down, which);
		if (menumode == WM_TOP_LEVEL)
		{
			if (!MCStringIsEmpty(menustring))
			{
				int2 curx = rect.x + 2;
				starttab = getmousetab(curx);
				if (tabselectonmouseup())
				{
					// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
					layer_redrawall();
				}
				else
				{
					uindex_t t_ntabs;
					t_ntabs = MCArrayGetCount(tabs);
					MCValueRef t_tab = nil;
					/* UNCHECKED */ MCArrayFetchValueAtIndex(tabs, starttab + 1, t_tab);
					if (starttab < t_ntabs && starttab + 1 != menuhistory
					        && MCStringGetCharAtIndex((MCStringRef)t_tab, 0) != '(')
					{
						MCValueRef t_oldhist = nil;
						/* UNCHECKED */ MCArrayFetchValueAtIndex(tabs, menuhistory, t_oldhist);
						setmenuhistoryprop(starttab + 1);
						// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
						layer_redrawall();
						handlemenupick(t_tab, t_oldhist);
					}
				}
			}
		}
		else
		{
			if (findmenu())
			{
				openmenu(True);
				clicktime = MCeventtime;
			}
		}
		return True;
	}
	if (which == Button1)
	{
		switch (getstack()->gettool(this))
		{
		case T_BROWSE:
			if (entry != NULL && MCU_point_in_rect(entry->getrect(), mx, my))
			{
				state |= CS_FIELD_GRAB;
				entry->mdown(which);
			}
			else
			{
				if (flags & F_AUTO_HILITE || family != 0)
				{
					if (MClook == LF_MOTIF || flags & F_SHOW_ICON ||
						(getstyleint(flags) != F_RADIO &&
						 getstyleint(flags) != F_CHECK))
					{
						if (getstyleint(flags) != F_RADIO || !(state & CS_HILITED))
						{
							state ^= CS_HILITED;
							starthilite = (Boolean)(state & CS_HILITED);
							sethilite(0, (state & CS_HILITED) != 0);
						}
						else
							starthilite = 0;
						reseticon();

						// MW-2011-08-30: [[ Layers ]] Potentially invalidate the whole object
						//   if flags and settings mean it is warranted.
						mayberedrawall();
					}
					else
					{
						starthilite = 1;
						reseticon();
						// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
						layer_redrawall();
					}
				}
			}
			if ((!IsMacLF() || entry != NULL)
			        && flags & F_TRAVERSAL_ON && !(state & CS_KFOCUSED))
				getstack()->kfocusset(this);
			message_with_valueref_args(MCM_mouse_down, MCSTR("1"));
			break;
		case T_POINTER:
		case T_BUTTON:
			start(True);
			break;
		case T_HELP:
			break;
		default:
			return False;
		}
	}
	else
	{
		message_with_args(MCM_mouse_down, which);
	}
	return True;
}

Boolean MCButton::mup(uint2 which, bool p_release)
{
	if (state & CS_MENU_ATTACHED)
		return MCObject::mup(which, p_release);
	MCAutoStringRef t_pick;
	if (state & CS_SUBMENU
	        && (which == 0 || menubutton == 0 || (uint1)which == menubutton))
	{
		// MW-2012-09-04: [[ Bug 10363 ]] Make sure we munfocus from all parent
		//   groups if we munfocus ourselves. We need this because menus cause
		//   mouse events to be funneled in a different way.
		MCObject *t_parent;
		t_parent = getparent();
		while(t_parent -> gettype() == CT_GROUP)
		{
			((MCGroup *)t_parent) -> clearmfocus();
			t_parent = t_parent -> getparent();
		}

		state &= ~CS_MFOCUSED;
		
        if (!(menu.IsValid()))
            return False;
        
        if (state & CS_FIELD_GRAB)
		{
			state &= ~CS_FIELD_GRAB;
			if (state & CS_SUBMENU)
				menu->mup(which, p_release);
			else
				entry->mup(which, p_release);
			return True;
		}
		if (menudepth > mymenudepth)
		{
            // Forward the click to the nested menu
            menu->mup(which, p_release);
			if (menudepth > mymenudepth)
				return True;
		}
		else
			if ((menumode == WM_OPTION || menumode == WM_COMBO)
			        && MCeventtime - clicktime < OPTION_TIME)
				return True;
			else
                // SN-2014-10-02: [[ Bug 13539 ]] Only consider the mouse location if we are
                //   sure that the coordinates are related to the stack, not the pulldown menu
				if (menumode == WM_PULLDOWN && MCmousestackptr == getstack() && MCU_point_in_rect(rect, mx, my))
				{
					if (state & CS_MOUSE_UP_MENU)
					{
						closemenu(True, True);
						state |= CS_VISITED;
						message_with_args(MCM_mouse_up, which);
					}
					else
					{
						menu->kfocusnext(True);
						state |= CS_MOUSE_UP_MENU;
					}
					return True;
				}
				else
					if (menumode == WM_POPUP
					        && MCU_abs(MCmousex - startx) < MCdoubledelta
					        && MCU_abs(MCmousey - starty) < MCdoubledelta)
						return True;

		// MW-2005-03-07: With this ordering, the menumup goes into an infinite
		//   loop, so lets try it the other way round.
		// MW-2008-03-27; [[ Bug 6225 ]] Make sure we send a mouseUp in this case
		//   by setting the menupoppedup global.
		MCmenupoppedup = true;
		menu->menumup(which, &t_pick, menuhistory);
		MCmenupoppedup = false;
		if (state & CS_IGNORE_MENU)
            closemenu(True, True);
        else
		{
			// An empty string means something handled the menumup while the
			// null string means nothing responded to it.
			if (*t_pick != nil)
			{
                // Something was selected so close the sub-menu
                closemenu(True, True);
                
                if (menumode == WM_OPTION || menumode == WM_COMBO)
				{
					MCValueAssign(label, *t_pick);
                    // SN-2014-08-05: [[ Bug 13100 ]] An empty label is not an issue,
                    //  we need to rely on the F_LABEL flag
                    flags |= F_LABEL;
					if (entry != NULL)
						entry->settext(0, *t_pick, False);
				}
				docascade(*t_pick);
			}
			else
            {
                // If the mouse release was handled, close the submenu. This
                // takes care of backwards compatibility. Otherwise, ignore the
                // mouse-up event.
                //
                // We also need to close the menu if the button release happened
                // outside of the menu tree.
                bool t_outside = true;
                MCObject* t_menu = menu;
                while (t_outside && t_menu != NULL)
                {
                    // Check whether the click was inside the menu (the rect
                    // that we need to check is the rect of the stack containing
                    // the menu).
                    MCRectangle t_rect = t_menu->getstack()->getrect();
                    t_outside = !MCU_point_in_rect(t_rect, mx, my);
                    
                    // Move to the parent menu, if it exists
                    if (t_menu->getstack()->getparent()    // Stack's parent
                        && t_menu->getstack()->getparent()->gettype() == CT_BUTTON
						&& t_menu->getstack()->getparent()->getstack()->getparent()
						&& t_menu->getstack()->getparent()->getstack()->getparent()->gettype() == CT_BUTTON)
                    {
                        // This is a submenu
                        t_menu = t_menu->getstack()->getparent();
                    }
                    else
                    {
                        // We walked up to the top of the submenu tree
                        t_menu = NULL;
                    }
                }
                Exec_stat es = message_with_args(MCM_mouse_release, which);
                if (t_outside || (es != ES_NOT_HANDLED && es != ES_PASS))
                    closemenu(True, True);
            }
		}
		state &= ~CS_IGNORE_MENU;
		if (MCmenuobjectptr == this)
			MCmenuobjectptr = nil;
		// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
		layer_redrawall();
		if (!opened)
			freemenu(True);
		return True;
	}
	// MW-2008-01-08: [[ Bug 5721 ]] Make sure if we mouse up and are not focused, no
	//   message is sent - I'm hoping that this only happens when a drag-drop operation
	//   finishes however it might not so watch out for more bugs...
	// MW-2008-03-19: [[ Bug 6164 ]] Make sure that it this mup was a result of closing a
	//   popup, we send a mouseUp message.
	if (!MCmenupoppedup && !(state & CS_MFOCUSED))
	{
		// MW-2008-02-26: [[ Bug 5901 ]] If we auto-hilite then reset the hilite failure
		//   to do this causes hilite to stick when popup is called from within a mouseDown
		//   handler on OS X. 
		if (flags & F_AUTO_HILITE)
			sethilite(0, False);
		reseticon();
		// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
		layer_redrawall();
		return False;
	}
	if (!(state & (CS_MFOCUSED | CS_ARMED)))
	{
		// MW-2008-02-26: [[ Bug 5901 ]] If we auto-hilite then reset the hilite failure
		//   to do this causes hilite to stick when popup is called from within a mouseDown
		//   handler on OS X.
		if (flags & F_AUTO_HILITE)
			sethilite(0, False);
		reseticon();
		// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
		layer_redrawall();
		message_with_args(MCM_mouse_up, which);
		return False;
	}
	if (MCbuttonstate == 0 || which == 0) // synthetic mup from popup
	{
		state &= ~CS_MFOCUSED;
		// IM-2008-07-02 [[ Bug 5901 ]] MCPopup calls mup(0) on the target object, use this to
		// check if this is how mup() has been called and toggle the hilite
		if (which == 0 && (flags & F_AUTO_HILITE))
			state ^= CS_HILITED;
		reseticon();

		// MW-2011-08-30: [[ Layers ]] Potentially invalidate the whole object
		//   if flags and settings mean it is warranted.
		mayberedrawall();
	}
	if (state & CS_GRAB)
	{
		if (flags & F_AUTO_HILITE)
		{
			if (starthilite)
				state &= ~CS_HILITED;
			else
				state |= CS_HILITED;
		}
		ungrab(which);
		return True;
	}
	if (which == Button1)
	{
		switch (getstack()->gettool(this))
		{
		case T_BROWSE:
			if (state & CS_FIELD_GRAB)
			{
				state &= ~CS_FIELD_GRAB;
				entry->mup(which, p_release);
			}
			else
			{
				if (getstyleint(flags) == F_MENU && menumode == WM_TOP_LEVEL
				        && tabselectonmouseup())
				{
					uindex_t t_ntabs;
					MCValueRef t_tab = nil;
					t_ntabs = MCArrayGetCount(tabs);
					/* UNCHECKED */ MCArrayFetchValueAtIndex(tabs, starttab + 1, t_tab);
					int2 curx = rect.x + 2;
					if (getmousetab(curx) == starttab && starttab < t_ntabs
					        && starttab + 1 != menuhistory
					        && MCStringGetCharAtIndex((MCStringRef)t_tab, 0) != '(')
					{
						MCValueRef t_oldhist = nil;
						/* UNCHECKED */ MCArrayFetchValueAtIndex(tabs, menuhistory, t_oldhist);
						setmenuhistoryprop(starttab + 1);
						// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
						layer_redrawall();
						handlemenupick(t_tab, t_oldhist);
					}
				}
				else
				{
					if (MClook != LF_MOTIF && flags & F_AUTO_HILITE
					        && !(flags & F_SHOW_ICON) && !(flags & F_AUTO_ARM)
					        && (getstyleint(flags) == F_RADIO
					            || getstyleint(flags) == F_CHECK))
					{
						if (MCU_point_in_rect(rect, mx, my))
						{
							if (getstyleint(flags) == F_CHECK)
								state ^= CS_HILITED;
							else
								state |= CS_HILITED;
						}
						// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
						layer_redrawall();
					}
					else
						if ((state & CS_HILITED && (flags & F_AUTO_HILITE || family != 0)) ||
							(state & CS_ARMED && flags & F_AUTO_ARM))
						{
							if (getstyleint(flags) == F_CHECK)
							{
								if (flags & F_AUTO_ARM && state & CS_ARMED
								        && flags & F_AUTO_HILITE)
									state ^= CS_HILITED;
							}
							else
								if (getstyleint(flags) == F_RADIO)
								{
									if (flags & F_AUTO_ARM && flags & F_AUTO_HILITE)
									{
										if (state & CS_ARMED)
											state |= CS_HILITED;
										else
											state &= ~CS_HILITED;
									}
								}
								else
									state &= ~CS_HILITED;
							state &= ~CS_ARMED;
							reseticon();

							// MW-2011-08-30: [[ Layers ]] Potentially invalidate the whole object
							//   if flags and settings mean it is warranted.
							mayberedrawall();
						}
					sethilite(0, (state & CS_HILITED) != 0);
					if (state & CS_HILITED)
						radio();
				}
				if (!p_release && MCU_point_in_rect(rect, mx, my))
				{
					state |= CS_VISITED;
					message_with_valueref_args(MCM_mouse_up, MCSTR("1"));
				}
				else
					message_with_valueref_args(MCM_mouse_release, MCSTR("1"));
			}
			break;
		case T_BUTTON:
		case T_POINTER:
			end(true, p_release);
			break;
		case T_HELP:
			help();
			break;

		default:
			return False;
		}
	}
	else
	{
		if (!p_release && MCU_point_in_rect(rect, mx, my))
		{
			state |= CS_VISITED;
			message_with_args(MCM_mouse_up, which);
		}
		else
			message_with_args(MCM_mouse_release, which);
	}
	if (state & CS_ARMED)
	{
		state &= ~CS_ARMED;
		reseticon();
		// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
		layer_redrawall();
	}
    
    // FG-2014-09-16: [[ Bugfix 13278 ]] Clear the mouse focus if this is not
    // an auto-arming button (e.g. a button within a menu).
    if (!(flags & F_AUTO_ARM))
        state &= ~CS_MFOCUSED;
    
	return True;
}

Boolean MCButton::doubledown(uint2 which)
{
	int2 tx = mx;
	int2 ty = my;
	if (menu.IsValid())
	{
		MCStack *sptr = MCmousestackptr;
		MCRectangle trect = sptr->getrect();
		tx = mx + trect.x;
		ty = my + trect.y;
	}

	// MW-2007-08-13: [[ Bug 3192 ]] Multiple clicks on a popup menus scrollbar closes it
	//   This seems to have been caused by messages not being passed to the control.
	if (getstack()->gettool(this) == T_BROWSE)
	{
		if (entry != NULL && MCU_point_in_rect(entry->getrect(), mx, my))
		{
			state |= CS_FIELD_GRAB;
			return entry->doubledown(which);
		}
		else if (state & CS_SUBMENU && menu.IsValid() && MCU_point_in_rect(menu->getrect(), tx, ty))
		{
			state |= CS_FIELD_GRAB;
			return menu->doubledown(which);
		}
	}

	return MCControl::doubledown(which);
}

Boolean MCButton::doubleup(uint2 which)
{
	// MW-2007-08-13: [[ Bug 3192 ]] Multiple clicks on a popup menus scrollbar closes it
	//   This seems to have been caused by messages not being passed to the control.
	if (getstack()->gettool(this) == T_BROWSE && state & CS_FIELD_GRAB)
	{
		state &= ~CS_FIELD_GRAB;
		if (entry != NULL)
			return entry->doubleup(which);
		else if (state & CS_SUBMENU && menu.IsValid())
			return menu -> doubleup(which);
	}
	return MCControl::doubleup(which);
}

#ifdef _MAC_DESKTOP
void MCButton::timer(MCNameRef mptr, MCParameter *params)
{
	if (MCNameIsEqualToCaseless(mptr, MCM_internal))
	{
		if (state & CS_SHOW_DEFAULT)
		{
            // MM-2014-07-31: [[ ThreadedRendering ]] Flag that there is no longer a default button animation message pending.
            m_animate_posted = false;            
			// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
			layer_redrawall();
		}
	}
	else
		MCControl::timer(mptr, params);
}
#endif

uint2 MCButton::gettransient() const
{
	uint2 b = 0;
	if (state & CS_SHOW_DEFAULT)
		b += MClook == LF_WIN95 ? WIN95_DEFAULT_WIDTH : MOTIF_DEFAULT_WIDTH;
	if (needfocus())
		b += MCfocuswidth;
	if (MCaqua && IsMacLFAM()
	        && getstyleint(flags) == F_MENU && menumode == WM_TOP_LEVEL)
		b += AQUA_FUDGE;
	return b;
}


void MCButton::applyrect(const MCRectangle &nrect)
{	
	rect = nrect;
	MCRectangle trect;
	if (entry != NULL)
	{
		if (MCcurtheme && MCcurtheme->iswidgetsupported(WTHEME_TYPE_COMBO))
		{
			//get rect of comboentry field in combobox
			MCWidgetInfo winfo;
			winfo.type = WTHEME_TYPE_COMBO;
			getwidgetthemeinfo(winfo);
			MCRectangle comboentryrect;
			winfo.part = WTHEME_PART_COMBOTEXT;
			MCcurtheme->getwidgetrect(winfo, WTHEME_METRIC_PARTSIZE,rect,comboentryrect);
			//now get contents rect for comboentry rect (to display MC borderless field in)
			winfo.type = WTHEME_TYPE_COMBOTEXT;
			MCcurtheme->getwidgetrect(winfo, WTHEME_METRIC_CONTENTSIZE,comboentryrect,trect);

			// PM-2015-10-12: [[ Bug 16193 ]] Make sure the label stays always within the combobox when resizing
			trect.y = nrect.y + nrect.height / 2 - trect.height / 2;
		}
		else
		{
			trect = MCU_reduce_rect(nrect, borderwidth);
			trect.width -= trect.height;

			if (IsMacEmulatedLF())
				trect.width -= 5;
		}
		entry->setrect(trect);
	}

	// MW-2010-06-07: [[ Bug 8746 ]] Make sure we rebuild the menu after freeing it,
	//   thus ensuring accelerators are not lost.
	if (menu.IsValid())
	{
		freemenu(False);
		findmenu(true);
	}
}

void MCButton::closemenu(Boolean kfocus, Boolean disarm)
{	
	if (state & CS_MENU_ATTACHED)
		MCObject::closemenu(kfocus, True);
	else
	{
		if (!(state & CS_SUBMENU))
			return;
		if (!opened)
			findmenu(true);
		if (menumode != WM_CASCADE || !(flags & F_AUTO_ARM))
		{
			MCscreen->ungrabpointer();
			MCdispatcher->removemenu();
		}
		if (disarm)
		{
			state &= ~CS_ARMED;
			ishovering = False;
			reseticon();
		}
		// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
		layer_redrawall();
        if (menu.IsValid())
        {
            if (kfocus && !(state & CS_MFOCUSED))
            {
                menu->setstate(True, CS_KFOCUSED); // override state
                menu->kunfocus();
            }
        
            MCButton *focused = (MCButton *)menu->getcurcard()->getmfocused();
            if (focused != NULL && focused->gettype() == CT_BUTTON
                    && focused->getmenumode() == WM_CASCADE)
                focused->closemenu(kfocus, disarm);

            menu->mode_closeasmenu();
            menu->close();
        }
		state &= ~(CS_SUBMENU | CS_MOUSE_UP_MENU);
		menudepth--;
	}
}

MCControl *MCButton::clone(Boolean attach, Object_pos p, bool invisible)
{
	MCButton *newbutton = new (nothrow) MCButton(*this);
	if (attach)
		newbutton->attach(p, invisible);
	return newbutton;
}


MCControl *MCButton::findnum(Chunk_term type, uint2 &num)
{
	if ((type == CT_BUTTON || type == CT_LAYER
	        || (type == CT_MENU && getstyleint(flags) == F_MENU
	            && menumode == WM_PULLDOWN)) && num-- == 0)
		return this;
	else
		return NULL;
}

MCControl *MCButton::findname(Chunk_term type, MCNameRef p_name)
{
    if ((type == gettype() || type == CT_LAYER
            || (type == CT_MENU && getstyleint(flags) == F_MENU
                && menumode == WM_PULLDOWN))
            && MCU_matchname(p_name, gettype(), getname()))
        return this;
	else
		return NULL;
}

Boolean MCButton::count(Chunk_term type, MCObject *stop, uint2 &num)
{
	if (type == CT_BUTTON || type == CT_LAYER
	        || (type == CT_MENU && getstyleint(flags) == F_MENU
	            && menumode == WM_PULLDOWN))
		num++;
	if (stop == this)
		return True;
	return False;
}

Boolean MCButton::maskrect(const MCRectangle &srect)
{
	if (!(flags & F_VISIBLE || showinvisible()))
		return False;
	MCRectangle trect = rect;
	if (getstyleint(flags) == F_MENU && menumode == WM_TOP_LEVEL)
	{
		if (getstack()->gettool(this) != T_POINTER)
			trect.height = 8 + MCFontGetAscent(m_font);
	}
	MCRectangle drect = MCU_intersect_rect(srect, trect);
	return drect.width != 0 && drect.height != 0;
}

void MCButton::installaccels(MCStack *stack)
{
	if (accelkey != 0)
		MCstacks->addaccelerator(this, stack, accelkey, accelmods);
}

void MCButton::removeaccels(MCStack *stack)
{
	if (accelkey != 0)
		MCstacks->deleteaccelerator(this, stack);
}

MCCdata *MCButton::getdata(uint4 cardid, Boolean clone)
{
	if (bdata != NULL)
	{
		if (flags & F_SHARED_HILITE)
			cardid = 0;
		MCCdata *bptr = bdata;
		do
		{
			if (bptr->getid() == cardid)
			{
				if (clone || cardid == 0)
					return new MCCdata(*bptr);
				else
					return (MCCdata *)bptr->remove(bdata);
			}
			bptr = (MCCdata *)bptr->next();
		}
		while (bptr != bdata);
	}
	return new MCCdata(cardid);
}

void MCButton::replacedata(MCCdata *&data, uint4 newid)
{
	if (data == NULL)
		return;
	if (flags & F_SHARED_HILITE)
		newid = 0;
	MCCdata *foundptr = getbptr(newid);
	foundptr->remove
	(bdata);
	delete foundptr;
	MCCdata *bptr = (MCCdata *)data->remove(data);
	bptr->setid(newid);
	bptr->appendto(bdata);
	if (opened)
	{
		switch(gethilite(newid).value)
		{
		case kMCTristateTrue:
			state |= CS_HILITED;
			state &= ~CS_MIXED;
			break;
		case kMCTristateFalse:
			state &= ~(CS_HILITED | CS_MIXED);
			break;
		case kMCTristateMixed:
			state &= ~CS_HILITED;
			state |= CS_MIXED;
			break;
		}
		// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
		layer_redrawall();
	}
}

void MCButton::compactdata()
{
	if (bdata != NULL)
	{
		MCStack *sptr = getstack();
		MCCdata *bptr = bdata;
		Boolean check;
		MCObject *tparent = this;
		while (tparent->getparent()->gettype() == CT_GROUP)
			tparent = tparent->getparent();
		uint4 tid = tparent->getid();
		do
		{
			check = False;
			MCCdata *nbptr = (MCCdata *)bptr->next();
			uint4 cid = bptr->getid();
			if (cid != 0 && (flags & F_SHARED_HILITE || !sptr->checkid(cid, tid)))
			{
				bptr->remove
				(bdata);
				delete bptr;
				if (bdata == NULL)
					break;
				check = True;
			}
			bptr = nbptr;
		}
		while (check || bptr != bdata);
	}
}

void MCButton::resetfontindex(MCStack *oldstack)
{
	if (bdata != NULL)
	{
		MCCdata *bptr = bdata;
		MCCdata *tptr = (MCCdata *)bptr->remove
		                (bptr);
		if (!(flags & F_SHARED_TEXT))
			tptr->setid(getcard()->getid());
		bdata = tptr;
		while (bptr != NULL)
		{
			tptr = (MCCdata *)bptr->remove(bptr);
			delete tptr;
		}
	}
	MCControl::resetfontindex(oldstack);
}

void MCButton::activate(Boolean notify, KeySym p_key)
{
	if (flags & F_DISABLED)
		return;
	if (findmenu(true))
	{
		bool t_disabled;
        t_disabled = false;
		MCAutoStringRef t_pick;
		
		if (menu.IsValid())
			menu->findaccel(p_key, &t_pick, t_disabled);
#ifdef _MAC_DESKTOP
		else if (m_system_menu != nil)
			getmacmenuitemtextfromaccelerator(m_system_menu, p_key, MCmodifierstate, &t_pick, false);
#endif
		if (MCStringIsEmpty(*t_pick))
		{
			if (MCmodifierstate & MS_MOD1)
			{
				message_with_args(MCM_mouse_down, menubutton);
				if (findmenu())
					openmenu(True);
			}
		}
		else
		{
			if (!t_disabled)
				handlemenupick(*t_pick, nil);
		}
	}
	else
	{
		if (flags & F_AUTO_HILITE)
		{
			if (getstyleint(flags) == F_RADIO)
			{
				sethilite(0, True);
				// MH-2007-03-20: [[ Bug 2581 ]] If a radio button is hilited using the keyboard, others in the group are not unhilited (when radioBehavior is set).
				if (parent->gettype() == CT_GROUP)
				{
					MCGroup *gptr = parent.GetAs<MCGroup>();
					gptr->radio(0, this);
				}
				reseticon();
				radio();
			}
			else if (getstyleint(flags) != F_RADIO || !(state & CS_HILITED))
			{
				state ^= CS_HILITED;
				uint2 oldstate = MCbuttonstate;
				MCbuttonstate = Button1;
				reseticon();
				// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
				layer_redrawall();
				MCbuttonstate = oldstate;
				if (getstyleint(flags) != F_CHECK)
				{
					state ^= CS_HILITED;
					// MW-2011-08-18: [[ Redraw ]] Force a screen update to
					//   ensure the hilite is shown.
					MCRedrawUpdateScreen();
					MCscreen->wait(HILITE_INTERVAL, False, False);
				}
				else
					sethilite(0, (state & CS_HILITED) != 0);
			}
			state |= CS_VISITED;
			reseticon();
			// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
			layer_redrawall();
		}
		message_with_valueref_args(MCM_mouse_up, MCSTR("1"));
		MCdispatcher->closemenus();
	}
}

void MCButton::setupmnemonic()
{
	if (opened && mnemonic != 0)
	{
		MCStringRef t_label = getlabeltext();
		if (!isdisabled() && !MCStringIsEmpty(t_label) && mnemonic <= MCStringGetLength(t_label))
		{
			codepoint_t t_codepoint = MCStringGetCodepointAtIndex(t_label, mnemonic - 1);
			getstack()->addmnemonic(this, t_codepoint);
			if (!MCStringIsEmpty(menustring) || !MCNameIsEmpty(menuname))
				MCstacks->addmenu(this, t_codepoint);
		}
	}
}

void MCButton::clearmnemonic()
{
	if (mnemonic != 0)
	{
		getstack()->deletemnemonic(this);
		MCstacks->deletemenu(this);
	}
}

MCCdata *MCButton::getbptr(uint4 cardid)
{
	MCCdata *foundptr = NULL;
	if (bdata != NULL)
	{
		MCCdata *tptr = bdata;
		do
		{
			if (tptr->getid() == cardid)
			{
				foundptr = tptr;
				break;
			}
			tptr = (MCCdata *)tptr->next();
		}
		while (tptr != bdata);
	}
	if (foundptr == NULL)
	{
		foundptr = new (nothrow) MCCdata(cardid);
		foundptr->appendto(bdata);
	}
	return foundptr;
}

uint2 MCButton::getfamily()
{
	return family;
}

MCTristate MCButton::gethilite(uint4 parid)
{
	if (flags & F_SHARED_HILITE)
		parid = 0;
	else
		if (parid == 0)
			parid = getcard()->getid();
	MCCdata *foundptr = getbptr(parid);
	foundptr->totop(bdata);
	return foundptr->getset();
}

void MCButton::setdefault(Boolean def)
{
	Boolean cs = (state & CS_SHOW_DEFAULT) != 0;
	if (def != cs)
	{
		uint2 t_old_trans;
		t_old_trans = gettransient();
		if (def)
			state |= CS_SHOW_DEFAULT;
		else
			state &= ~CS_SHOW_DEFAULT;
		// MW-2011-08-18: [[ Layers ]] Invalidate the whole object, noting
		//   possible change in transient.
		layer_transientchangedandredrawall(t_old_trans);
	}
}

Boolean MCButton::sethilite(uint4 parid, MCTristate hilite)
{
	Boolean set
		= True;
	if (flags & F_SHARED_HILITE)
		parid = 0;
	else
		if (parid == 0)
			parid = getcard()->getid();
		else
			set
				= parid == getcard()->getid();
	MCCdata *foundptr = getbptr(parid);
	
	bool t_hilite_changed;
	t_hilite_changed = hilite != MCTristate(foundptr -> getset());
	
	foundptr->setset(!hilite.isFalse());
	uint4 oldstate = state;
	if (opened && set)
			switch (hilite.value)
			{
			case kMCTristateTrue:
				state |= CS_HILITED;
				state &= ~CS_MIXED;
				break;
			case kMCTristateFalse:
				state &= ~(CS_HILITED | CS_MIXED);
				break;
			case kMCTristateMixed:
				state &= ~CS_HILITED;
				state |= CS_MIXED;
			}
	
	if (t_hilite_changed)
		signallisteners(P_HILITE);
	
	return state != oldstate;
}

void MCButton::resethilite(uint4 parid, MCTristate hilite)
{
	if (sethilite(parid, hilite))
	{
		reseticon();
		// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
		layer_redrawall();
	}
}

void MCButton::getentrytext()
{
    // MW-2012-02-21: [[ FieldExport ]] Use the new plain text export method.
    MCAutoStringRef t_label;
    if (entry -> exportasplaintext((uinteger_t)0, 0, INT32_MAX, &t_label))
        MCValueAssign(label, *t_label);
    
    // SN-2014-08-05: [[ Bug 13100 ]] An empty label is not an issue,
    //  we need to rely on the F_LABEL flag
    flags |= F_LABEL;
}

void MCButton::createentry()
{
	// MW-2012-02-16: [[ FontRefs ]] Only create the entry if the control is
	//   open. This is because 'setupentry' requires a fontref to complete.
	if (opened != 0)
	{
		entry = (MCField *)MCtemplatefield->clone(False, OP_NONE, false);
		// MW-2005-08-16: [[Bug 2820]] If we can't be selected, let us make sure our field can't either!
		entry->setextraflag(getextraflag(EF_CANT_SELECT), EF_CANT_SELECT);
		entry->setupentry(this, label);
		entry->open();
		setrect(rect);
	}
}

void MCButton::deleteentry()
{
	getentrytext();
	entry->close();
	delete entry;
	entry = NULL;
}

//major menustring
//void MCButton::getmenuptrs(const char *&sptr, const char *&eptr)
MCRange MCButton::getmenurange()
{
	uindex_t sptr;
	uindex_t i = 0;
	MCRange t_search;
	uindex_t t_length;
	t_length = MCStringGetLength(menustring);
	t_search = MCRangeMake(0, t_length);
	if (MCStringIsEmpty(menustring))
		return MCRangeMake(0, 0);

	do
	{
		// Finds the first newline character in the string.
		// If there is no newline character, then if this is the last item in the string, the returned range is the
		// remainder of the string. Otherwise, there are no more items in the string and an empty range is returned.
		//
		// If a newline was found and this is not the last item in the history, move past the newline and continue.
		// Otherwise, terminate and return.
			
		sptr = t_search.offset;
		MCRange t_temp;
		if (!MCStringFind(menustring, t_search, MCSTR("\n"), kMCStringOptionCompareExact, &t_temp))
		{
			if (++i == menuhistory)
				return MCRangeMakeMinMax(sptr, t_length);
			else
				return MCRangeMake(0, 0);
			break;
		}
			
		t_search.offset = t_temp.offset;
		if (++i < menuhistory)
			t_search.offset++;
		t_search.length = t_length - t_search.offset;
	}
	while (i < menuhistory);
		
	return MCRangeMakeMinMax(sptr, t_search.offset);
}

void MCButton::makemenu(sublist *bstack, int2 &stackdepth, uint2 menuflags, MCFontRef fontref)
{
	Boolean isxp = MCcurtheme && MCcurtheme->getthemeid() == LF_NATIVEWIN;
	uint2 pwidth = 0;
	if (stackdepth > 0)
	{
		MCStringRef t_lastname = MCNameGetString(bstack[stackdepth].parent->getname());
        // MM-2014-04-16: [[ Bug 11964 ]] Pass through the transform of the stack to make sure the measurment is correct for scaled text.
        pwidth = MCFontMeasureText(fontref, t_lastname, getstack() -> getdevicetransform()) + 16;
	}
	sublist *m = &bstack[stackdepth--];

	m -> maxwidth += m -> maxaccelwidth + 12;

	uint2 ty;
	uint2 menuwidth, menuheight;
	if (isxp && MCmajorosversion >= 0x0600)
	{
		ty = 3;
		m -> maxwidth += 22 + 4 + 2 + 4 + 17; // Icon - pad - Gutter - pad - ... - Submenu

		MCButton *bptr;
		bptr = m -> buttons;
		do
		{
			if (bptr -> menucontrol == MENUCONTROL_SEPARATOR)
			{
				bptr -> rect . x = 3 + 22 + 4 + 2;
				bptr -> rect . width = m -> maxwidth - (22 + 4 + 2);
				bptr -> rect . height = 6;
				bptr -> rect . y = ty + 1;
				ty += 1 + 6 + 3;
			}
			else
			{
				bptr -> rect . x = 3;
				bptr -> rect . width = m -> maxwidth;
				bptr -> rect . height += 2;
				bptr -> rect . y = ty;
				ty += bptr -> rect . height;

				bptr -> leftmargin = 22 + 4 + 2 + 4;
				bptr -> topmargin += 2;
				bptr -> bottommargin += 2;
				bptr -> rightmargin = 17;
			}
			bptr = (MCButton *)bptr->next();
		}
		while (bptr != m->buttons);
		menuwidth = m -> maxwidth + 3 + 3;
		menuheight = ty + 3;
	}
	else
	{
		uint1 leftmmargin,rightmmargin,topmmargin,bottommmargin;
		uint1 leftmitemmargin = (uint1)leftmargin,rightmitemmargin = (uint1)rightmargin;
		uint1  dividerpadding;
		uint4  menuitempadding = 0;
		uint1 dividersize;
		if (isxp)
		{
			dividersize = 1;
			dividerpadding = 4;
			leftmitemmargin = rightmitemmargin = 17;
			leftmmargin = rightmmargin = topmmargin = bottommmargin = 3;
		}
		else
		{
			dividersize = 2;
			dividerpadding = 2;
			leftmmargin = rightmmargin = topmmargin = bottommmargin = IsMacEmulatedLF() ? 1 : 2;
			leftmitemmargin = (uint1)leftmargin;
			rightmitemmargin = (uint1)rightmargin;
		}
		m->maxwidth += leftmitemmargin + rightmitemmargin;
		if ((getstyleint(menuflags) == F_CHECK || getstyleint(menuflags) == F_RADIO) && !isxp)
			m->maxwidth += CHECK_SIZE + leftmargin;

		if (IsMacEmulatedLF() && menumode == WM_OPTION
				&& m->maxwidth < rect.width - rect.height - 12)
			m->maxwidth = rect.width - rect.height - 12;
		ty = topmmargin;
		MCButton *bptr = m->buttons;
		do
		{
			Boolean isdivider = bptr->rect.height == dividersize;
			//xp has extra padding between dividers
			uint2 pad = isdivider ? dividerpadding: menuitempadding;
			//xp has a left margin for buttons
			if (isxp)
			{
				if ((getstyleint(menuflags) == F_CHECK || getstyleint(menuflags) == F_RADIO) && bptr->menumode != WM_CASCADE)
					bptr->leftmargin = MCU_max(leftmitemmargin - CHECK_SIZE - leftmargin + 2,0);
				else
					bptr->leftmargin = leftmitemmargin;
			}
			//dividers in xp width of menu
			bptr->rect.x = leftmmargin;
			bptr->rect.width = m->maxwidth;
			bptr->rect.y = ty + pad;
			ty += bptr->rect.height + (pad << 1);
			bptr = (MCButton *)bptr->next();
		}
		while (bptr != m->buttons);
		menuwidth = m->maxwidth + leftmmargin + rightmmargin;
		menuheight = ty + bottommmargin;
	}

	MCStack *newmenu = nil;
	/* UNCHECKED */ MCStackSecurityCreateStack(newmenu);

	newmenu->setparent(m->parent);
	newmenu->createmenu(m->buttons, menuwidth, menuheight);
	newmenu->menuwindow = True;
	MCdispatcher->appendpanel(newmenu);
	if (m->parent == this)
    {
		menu = newmenu->GetHandle();
        newmenu -> addneed(this);
    }
	else
	{
		m->parent->menu = newmenu->GetHandle();
        newmenu -> addneed(m->parent);
        
		m->parent->menumode = WM_CASCADE;
		if (getstyleint(menuflags) == F_CHECK || getstyleint(menuflags) == F_RADIO)
			m->parent->leftmargin += CHECK_SIZE + leftmargin;
		m->parent->flags = (m->parent->flags & ~F_STYLE) | F_MENU;
	}
	if (stackdepth >= 0 && pwidth > bstack[stackdepth].maxwidth)
		bstack[stackdepth].maxwidth = pwidth;
}

void setmenutagflag(MCButton *p_button, bool p_hastags)
{
	MCStack *t_menu = p_button->getmenu();
	if (t_menu != NULL)
	{
		MCControl *controls = t_menu->getcontrols();
		if (controls != NULL)
		{
			MCButton *bptr = (MCButton *)controls;
			do
			{
				bptr->setmenuhasitemtags(p_hastags);
				if (bptr->getmenumode() == WM_CASCADE && bptr->getmenu() != NULL)
				{
					setmenutagflag(bptr, p_hastags);
				}
				bptr = (MCButton *)bptr->next();

			}
			while (bptr != controls);
		}
	}
}

class ButtonMenuCallback : public IParseMenuCallback
{
	MCButton *parent;
	uint4 menuflags;
	sublist bstack[MAX_SUBMENU_DEPTH];
	int2 stackdepth;

	MCFontRef fontref;
	uint2 fheight;

public:
	ButtonMenuCallback(MCButton *p_parent, uint4 p_menuflags)
	{
		parent = p_parent;
		menuflags = p_menuflags;

		// MW-2012-02-16: [[ FontRefs ]] Menus are built from controls that may be closed.
		//   Therefore, we create a new fontref from the parent's styling here.
		MCNameRef fname;
		uint2 fsize, fstyle;
		parent -> getfontattsnew(fname, fsize, fstyle);
		/* UNCHECKED */ MCFontCreate(fname, MCFontStyleFromTextStyle(fstyle), fsize, fontref);

		fheight = parent -> gettextheight();

		bstack[0].parent = parent;
		bstack[0].buttons = NULL;
		bstack[0].f = 1;
		bstack[0].maxwidth = 0;
		bstack[0].maxaccelwidth = 0;
		stackdepth = 0;
	}

	virtual bool ProcessItem(MCMenuItem *p_menuitem)
	{
		int2 newdepth = p_menuitem->depth;
		if (newdepth > 0 && newdepth > stackdepth
		        && stackdepth < MAX_SUBMENU_DEPTH - 1
		        && bstack[stackdepth].buttons != NULL)
		{
			bstack[++stackdepth].maxwidth = 0;
			bstack[stackdepth].maxaccelwidth = 0;
			bstack[stackdepth].parent = bstack[stackdepth - 1].buttons->prev();
			bstack[stackdepth].buttons = NULL;
			bstack[stackdepth].f = 1;
		}
		while (newdepth < stackdepth)
			parent->makemenu(bstack, stackdepth, menuflags, fontref);
		MCButton *newbutton = new (nothrow) MCButton;
		newbutton->appendto(bstack[stackdepth].buttons);
		MCNameRef t_name = nil;
		if (!MCStringIsEmpty(p_menuitem->tag))
			/* UNCHECKED */ MCNameCreate(p_menuitem->tag, t_name);
		else
			/* UNCHECKED */ MCNameCreate(p_menuitem->label, t_name);
		newbutton->setname(t_name);
		MCValueRelease(t_name);

#ifndef TARGET_PLATFORM_MACOS_X
		MCStringRef t_label;
		/* UNCHECKED */ MCStringMutableCopy(p_menuitem->label, t_label);
		/* UNCHECKED */ MCStringFindAndReplaceChar(t_label, '\t', ' ', kMCStringOptionCompareExact);
		MCValueRelease(p_menuitem->label);
		/* UNCHECKED */ MCStringCopyAndRelease(t_label, p_menuitem->label);
#endif

		newbutton->menubutton = parent->menubutton;
		newbutton->menucontrol = MENUCONTROL_ITEM;
        newbutton->m_theme_type = kMCPlatformControlTypeMenu;
		if (MCStringGetNativeCharAtIndex(MCNameGetString(newbutton->getname()), 0) == '-')
		{
			newbutton->rect.height = 2;
			newbutton->flags = DIVIDER_FLAGS;
			newbutton->menucontrol = MENUCONTROL_SEPARATOR;
            newbutton->m_theme_type = kMCPlatformControlTypeMenu;
			if (MCcurtheme && MCcurtheme->getthemeid() == LF_NATIVEWIN)
			{
				newbutton->rect.height = 1;
				newbutton->flags &= ~F_3D;
			}
			bstack[stackdepth].f++;
		}
		else
		{
			newbutton->flags = menuflags;
			newbutton->rect.height = fheight + parent->menubuttonheight;

			if (p_menuitem->is_disabled)
			{
				newbutton->flags |= F_DISABLED;
			}
			
			if (p_menuitem->is_hilited)
				newbutton->sethilite(0, True);
				
			if (p_menuitem->is_radio)
			{
				newbutton->family = bstack[stackdepth].f;
				newbutton->flags &= ~F_STYLE;
				newbutton->flags |= F_RADIO;
			}
				
			if (parent->menumode != WM_OPTION)
			{
				newbutton->mnemonic = p_menuitem->mnemonic;
			}
			if (parent->menumode == WM_PULLDOWN)
			{
				if (p_menuitem->accelerator != 0)
				{
					uint1 t_mods = p_menuitem->modifiers;
					KeySym t_key = p_menuitem->accelerator;
					MCStringRef t_keyname = p_menuitem->accelerator_name;

					if ((t_mods & (MS_MAC_CONTROL | MS_CONTROL | MS_ALT)) || t_keyname)
					{
						KeySym t_accelkey = MCKeySymToLower(t_key);
						MCstacks->addaccelerator(parent, parent->getstack(), t_accelkey, t_mods);
						newbutton->accelkey = t_accelkey;
						newbutton->accelmods = t_mods;

						// ******
						// The following text requires localization
						// ******
						MCStringRef t_acceltext = nil;
						/* UNCHECKED */ MCStringCreateMutable(0, t_acceltext);
						if (t_mods & MS_MAC_CONTROL)
							/* UNCHECKED */ MCStringAppendFormat(t_acceltext, "Ctrl+");
						if (MS_MAC_CONTROL != MS_CONTROL && t_mods & MS_CONTROL)
							/* UNCHECKED */ MCStringAppendFormat(t_acceltext, "Cmd+");
						if (t_mods & MS_ALT)
							/* UNCHECKED */ MCStringAppendFormat(t_acceltext, "Alt+");
						if (t_mods & MS_SHIFT)
							/* UNCHECKED */ MCStringAppendFormat(t_acceltext, "Shift+");
						if (t_keyname != NULL && !MCStringIsEmpty(t_keyname))
							/* UNCHECKED */ MCStringAppend(t_acceltext, t_keyname);
						else
                        {
							codepoint_t t_codepoint;
                            MCAutoStringRef t_key_text;
                            
                            if (t_key < 0x7f)
                                t_codepoint = t_key;
                            else if ((t_key & XK_Class_mask) == XK_Class_codepoint)
                                t_codepoint = t_key & XK_Codepoint_mask;
                            else
                                t_codepoint = 0;    // Shouldn't happen!
                            
                            /* UNCHECKED */ MCStringCreateWithBytes((const byte_t*)&t_codepoint, 4, kMCStringEncodingUTF32, false, &t_key_text);
                            /* UNCHECKED */ MCStringAppend(t_acceltext, *t_key_text);
                        }

						MCValueRelease(newbutton->acceltext);
						/* UNCHECKED */ MCStringCopyAndRelease(t_acceltext, newbutton->acceltext);
					}
				}
			}
            // MM-2014-04-16: [[ Bug 11964 ]] Pass through the transform of the stack to make sure the measurment is correct for scaled text.
            int32_t width = MCFontMeasureText(fontref, p_menuitem->label, parent -> getstack() -> getdevicetransform());
			if (!MCStringIsEmpty(newbutton->acceltext))
                bstack[stackdepth].maxaccelwidth = MCU_max(bstack[stackdepth].maxaccelwidth, MCFontMeasureText(fontref, newbutton->acceltext, parent -> getstack() -> getdevicetransform()));

			if (width > bstack[stackdepth].maxwidth)
				bstack[stackdepth].maxwidth = width;
			MCValueAssign(newbutton->label, p_menuitem->label);
			newbutton -> flags |= F_LABEL;
		}
		return false;
	}

	virtual bool End(bool p_has_tags)
	{
		parent->menuhasitemtags = p_has_tags;
		while (stackdepth >= 0)
			parent->makemenu(bstack, stackdepth, menuflags, fontref);
		setmenutagflag(parent, true);
		// MW-2012-02-16: [[ FontRefs ]] Make sure we free the font we created.
		MCFontRelease(fontref);
		return false;
	}
};

Boolean MCButton::findmenu(bool p_just_for_accel)
{
	if (!MCNameIsEmpty(menuname))
	{
		if (!(menu.IsValid()))
		{
			MCerrorlock++;
            MCStack * t_stack = getstack()->findstackname(menuname);
            MCerrorlock--;
            if (t_stack != nil)
            {
			    menu = t_stack->GetHandle();
				t_stack->addneed(this);
            }
		}
	}
	else if (!MCStringIsEmpty(menustring) && getstyleint(flags) == F_MENU)
	{
		if (menumode == WM_TOP_LEVEL)
		{
			MCValueRelease(tabs);
			/* UNCHECKED */ MCStringSplit(menustring, MCSTR("\n"), nil, kMCStringOptionCompareExact, tabs);
		}
		else if (!(menu.IsValid()))
		{
			uint2 fheight;
			fheight = gettextheight();
			if (((!IsMacLFAM() || MCModeMakeLocalWindows()) && menumode == WM_COMBO) || (menumode == WM_OPTION && MClook == LF_WIN95))
			{
				uindex_t nlines = 1;
				//major menustring
				nlines = MCStringCountChar(menustring, MCRangeMake(0, MCStringGetLength(menustring)), '\n', kMCStringOptionCompareExact) + 1;

				MCField *fptr = new (nothrow) MCField;
				uint2 height;
				if (nlines > menulines)
				{
					height = menulines * fheight;
					fptr->setupmenu(menustring, fheight, True);
					state |= CS_SCROLLBAR;
				}
				else
				{
					height = nlines * fheight;
					fptr->setupmenu(menustring, fheight, False);
				}
				MCRectangle trect;
				MCU_set_rect(trect, 0, 0, rect.width, height + 4);
				trect = MCU_reduce_rect(trect, MClook == LF_MOTIF ? DEFAULT_BORDER : 1);
				fptr->setrect(trect);
                MCStack * t_menu;
                if (!MCStackSecurityCreateStack(t_menu))
                    return False;
                
				t_menu->setparent(this);
				t_menu->createmenu(fptr, rect.width, height + 4);
				MCdispatcher->appendpanel(t_menu);
                menu = t_menu->GetHandle();
                t_menu -> addneed(this);
			}
			else
			{
				if (IsMacLFAM() && (!MCModeMakeLocalWindows() ||
				         (menumode == WM_OPTION || menumode == WM_POPUP ||
				            (menumode == WM_PULLDOWN && flags & F_SHOW_BORDER))))
				{
#ifdef _MAC_DESKTOP
					return macfindmenu(p_just_for_accel);
#else
					return True;
#endif
				}
				else
				{
					MCValueRelease(tabs);
					/* UNCHECKED */ MCStringSplit(menustring, MCSTR("\n"), nil, kMCStringOptionCompareExact, tabs);
					uindex_t t_ntabs;
					t_ntabs = MCArrayGetCount(tabs);
					uint4 menuflags = MENU_ITEM_FLAGS;
					if (flags & F_DISABLED)
						menuflags |= F_DISABLED;
					for (uindex_t i = 0 ; i < t_ntabs ; i++)
					{
						MCValueRef t_tabval = nil;
						/* UNCHECKED */ MCArrayFetchValueAtIndex(tabs, i + 1, t_tabval);
						MCStringRef t_tab;
						t_tab = (MCStringRef)t_tabval;
						if (MCStringGetCharAtIndex(t_tab, 0) == '!' ||
							(MCStringGetCharAtIndex(t_tab, 0) == '(' &&
							 MCStringGetCharAtIndex(t_tab, 1) == '!'))
						{
							menuflags &= ~F_STYLE;
							menuflags |= F_CHECK;
							break;
						}
					}

					ButtonMenuCallback t_callback(this, menuflags);
					MCParseMenuString(menustring, &t_callback, menumode);
				}
			}
			MCValueAssign(tabs, kMCEmptyArray);
		}
	}
	return menu.IsValid();
}

void MCButton::setmenuhandler(MCButtonMenuHandler *p_handler)
{
	m_menu_handler = p_handler;
}

Exec_stat MCButton::handlemenupick(MCValueRef p_pick, MCValueRef p_old_pick)
{
	if (m_menu_handler != nil)
	{
		if (m_menu_handler->OnMenuPick(this, p_pick, p_old_pick))
			return ES_NORMAL;
	}
	
	if (p_old_pick == nil)
		return message_with_valueref_args(MCM_menu_pick, p_pick);
	else
		return message_with_valueref_args(MCM_menu_pick, p_pick, p_old_pick);
}

bool MCSystemPick(MCStringRef p_options, bool p_use_checkmark, uint32_t p_initial_index, uint32_t& r_chosen_index, MCRectangle p_button_rect);

void MCButton::openmenu(Boolean grab)
{
	if (!opened || !MCmousestackptr)
		return;
	if (!MCNameIsEmpty(menuname) && !MCModeMakeLocalWindows())
		return;
#ifdef _MOBILE
	if (menumode == WM_OPTION)
	{
		// result of picker action
		long t_result;
		// item selection
		uint32_t t_selected, t_chosen_option;

		// the selected item
		// get a pointer to this 
		MCButton *pptr;
		pptr = this;
		
		// get the label and menu item strings
		MCStringRef t_menustring;
		t_menustring = pptr->getmenustring();
		
		// process data using the pick wheel
		t_selected = menuhistory;
		t_result = MCSystemPick(t_menustring, false, t_selected, t_chosen_option, rect);
		
		// populate the label with the value returned from the pick wheel if the user changed the selection
		if (t_result && t_chosen_option > 0)
		{
			setmenuhistoryprop(t_chosen_option);
			
			uindex_t t_offset = 0;
            uindex_t t_new_offset = 0;
			bool t_success = true;
			
			MCAutoProperListRef t_options;
			
			if (t_success)
				t_success = MCStringSplitByDelimiter(t_menustring, kMCLineEndString, kMCStringOptionCompareExact, &t_options);
			
			MCStringRef t_label = static_cast<MCStringRef>(MCProperListFetchElementAtIndex(*t_options, t_chosen_option - 1));
			
			MCValueAssign(label, t_label);
			
			flags |= F_LABEL;
			handlemenupick(t_label, nil);
		}
		return;
	}
#endif
	MCStack *cascade_sptr = menumode == WM_CASCADE ? getstack() : MCmousestackptr;
	if (flags & F_TRAVERSAL_ON && !(state & CS_KFOCUSED)
	        && cascade_sptr->getmode() < WM_PULLDOWN)
	{
		MCmousestackptr->kfocusset(this);
		if (!(menu.IsValid()) && !findmenu())
			return;
	}
	if (IsMacLFAM() &&
		(!MCModeMakeLocalWindows() ||
		 (MCNameIsEmpty(menuname) && (menumode == WM_OPTION || menumode == WM_POPUP || (menumode == WM_PULLDOWN && flags & F_SHOW_BORDER)))))
	{
#ifdef _MAC_DESKTOP
		macopenmenu();
#endif
	}
	else
	{
		state |= CS_SUBMENU | CS_ARMED;
		reseticon();
		if (!MCmenuobjectptr)
			MCmenuobjectptr = this;
		mymenudepth = ++menudepth;
		MCStack *sptr = menumode == WM_POPUP ? MCmousestackptr : getstack();

		bool t_did_grab;
		t_did_grab = false;
		if (grab && (menumode != WM_CASCADE || !(flags & F_AUTO_ARM)))
		{
			MCscreen->grabpointer(sptr->getw());
			if (MCraisemenus)
			{
				sptr->raise();
				MCstacks->top(sptr);
			}

			MCdispatcher->addmenu(this);

			t_did_grab = true;
		}
		MCRectangle rel = MCU_recttoroot(sptr, rect);
		if (MClook != LF_WIN95 && menumode == WM_OPTION)
		{
			if (MClook == LF_MOTIF)
			{
				if (rect.width > optionrect.width << 1)
					rel.width -= optionrect.width << 1;
			}
			else
			{
				if (rel.width > rel.height)
					rel.width -= rel.height;
				rel.x += 2;
			}
			rel.x += labelwidth;
			rel.width -= labelwidth;
			menu->menuset(menuhistory, rect.height >> 1);
		}

		menu->openrect(rel, (Window_mode)menumode, NULL, WP_DEFAULT, OP_NONE);
		menu->mode_openasmenu(t_did_grab ? sptr : NULL);
		
		// MW-2014-03-11: [[ Bug 11893 ]] Make sure we don't do anything to a stack panel.
		if (menumode == WM_OPTION && MCNameIsEmpty(menuname))
		{
			MCField *t_field = NULL;
			MCObjptr *t_obj = menu->getcurcard()->getrefs();
			MCObjptr *t_iter = t_obj;
			do
			{
				if (t_iter->getref()->gettype() == CT_FIELD)
				{
					t_field = (MCField*)t_iter->getref();
					break;
				}
				t_iter = t_iter->next();
			} while (t_iter != t_obj);
			if (t_field != NULL)
			{
				uint32_t t_menuhistory = menuhistory;
				t_field->sethilitedlines(&t_menuhistory, 1);
				// MW-2011-08-18: [[ Layers ]] Invalidate the field object.
				t_field->layer_redrawall();
			}
		}
		int2 tx = mx;
		int2 ty = my;
		sptr->translatecoords(menu, tx, ty);
		menu->mfocus(tx, ty);
		menu->resetcursor(True);
		if (!(state & CS_MFOCUSED))
			menu->kfocusnext(True);
		// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
		layer_redrawall();
		startx = MCmousex;
		starty = MCmousey;
	}
}

void MCButton::freemenu(Boolean force)
{
#ifdef _MAC_DESKTOP
	macfreemenu();
#endif
	if (menu.IsValid() && !(state & CS_SUBMENU))
	{
		if (!MCNameIsEmpty(menuname))
		{
			menu->removeaccels(getstack());
			menu->removeneed(this);
			menu = nil;
		}
		else
		{
			if (!MCStringIsEmpty(menustring) || force)
            {
                closemenu(False, True);
                
                /* In this case the button owns the menu so after removing
                 * any references to it that might exist in the environment
                 * it must be explicitly deleted. */
				MCdispatcher->removepanel(menu);
				MCstacks->deleteaccelerator(this, NULL);
				menu->removeneed(this);
                
                /* Schedule deletion of the menu stack. */
                menu->scheduledelete();
                
				menu = nil;
			}
		}
	}
}

void MCButton::docascade(MCStringRef p_pick)
{
	MCAutoStringRef t_pick;
	MCButton *pptr = this;
	if (MCNameIsEmpty(menuname) && MCStringIsEmpty(menustring))
	{
		//get tag info
		bool t_has_tags = getmenuhastags();

		pptr = this;
		while(pptr->menumode == WM_CASCADE && pptr->parent->getparent()->getparent()->gettype() == CT_BUTTON)
		{
			MCStringRef t_label = nil;
			if (t_has_tags && !MCNameIsEmpty(pptr->getname()))
				t_label = MCNameGetString(pptr->getname());
			else
				t_label = pptr->getlabeltext();
			
            if (*t_pick == nil)
                /* UNCHECKED */ MCStringMutableCopy(p_pick, &t_pick);
            
            /* UNCHECKED */ MCStringPrependNativeChar(*t_pick, '|');
            /* UNCHECKED */ MCStringPrepend(*t_pick, t_label);

			pptr = (MCButton *)pptr->parent->getparent()->getparent();
			pptr->state |= CS_IGNORE_MENU;
		}
	}
	else
	{
			t_pick = p_pick;
	}	
	
	if (pptr != this)
	{
		// IM-2015-10-06: [[ Bug 15502 ]] Call the handler of the *parent*
		//    menu button, rather than of this one.
		if (pptr->m_menu_handler == nil || !pptr->m_menu_handler->OnMenuPick(pptr, *t_pick, nil))
		{
			MCParameter *param = new (nothrow) MCParameter;
			param->setvalueref_argument(*t_pick);
			MCscreen->addmessage(pptr, MCM_menu_pick, MCS_time(), param);
		}
	}
	else
	{
		Exec_stat es = pptr->handlemenupick(*t_pick, nil);
		if (es == ES_NOT_HANDLED || es == ES_PASS)
			pptr->message_with_args(MCM_mouse_up, menubutton);
	}
}

void MCButton::setupmenu()
{
	flags = MENU_FLAGS;
}

bool MCButton::menuisopen()
{
#ifdef _MAC_DESKTOP
	return macmenuisopen();
#else
	return menu.IsValid() && menu->getopened();
#endif
}

bool MCButton::selectedchunk(MCStringRef& r_string)
{
    MCExecContext ctxt(nil, nil, nil);
	uinteger_t t_number;
	GetNumber(ctxt, 0, t_number);
	
	MCRange t_range;
	t_range = getmenurange();
	return MCStringFormat(r_string, "char %d to %d of button %d", t_range.offset, t_range.offset + t_range.length, t_number);
}

bool MCButton::selectedline(MCStringRef& r_string)
{
    MCExecContext ctxt(nil, nil, nil);
	uinteger_t t_number;
	GetNumber(ctxt, 0, t_number);
	
	return MCStringFormat(r_string, "line %d of button %d", menuhistory, t_number);
}

bool MCButton::selectedtext(MCStringRef& r_string)
{
	if (entry != NULL)
	{
		r_string = MCValueRetain(getlabeltext());
		return true;
	}
	else
	{
		MCRange t_range;
		t_range = getmenurange();
		/* UNCHECKED */ MCStringCopySubstring(menustring, t_range, r_string);
		return true;
	}
	return false;
}

MCStringRef MCButton::getlabeltext()
{
	if (entry != nil)
		getentrytext();
    // SN-2014-08-05: [[ Bug 13100 ]] An empty label is not an issue,
    //  we need to rely on the F_LABEL flag
	if (flags & F_LABEL)
		return label;
	else
		return MCNameGetString(getname());
}

bool MCButton::resetlabel()
{
	bool changed = false;
	if (menumode == WM_OPTION || menumode == WM_COMBO)
	{
		if (MCStringIsEmpty(menustring))
		{
			if (entry != NULL)
				entry->settext(0, kMCEmptyString, False);

			if (!MCStringIsEmpty(label))
			{
				MCValueAssign(label, kMCEmptyString);
				changed = true;
			}
            
            // SN-2014-08-05: [[ Bug 13100 ]] An empty label is not an issue,
            //  we need to rely on the F_LABEL flag
            flags &= ~F_LABEL;
		}
		else
		{
			MCRange t_range;
			t_range = getmenurange();
			MCAutoStringRef t_label;
			if (t_range.length == 0)
			{
				setmenuhistoryprop(1);
				t_label = kMCEmptyString;
			}
			else
			{
				/* UNCHECKED */ MCStringCopySubstring(menustring, t_range, &t_label);
			}
            
            // AL-2014-08-04: [[ Bug 13089 ]] Set entry text to new label
			if (entry != NULL)
				entry->settext(0, *t_label, False);
            
            // SN-2014-08-05: [[ Bug 13100 ]] An empty label is not an issue,
            //  we need to rely on the F_LABEL flag
            flags |= F_LABEL;

			if (!MCStringIsEqualTo(label, *t_label, kMCStringOptionCompareExact))
			{
				MCValueAssign(label, *t_label);
				changed = true;
			}
		}
	}
	else
		if (menumode == WM_TOP_LEVEL)
		{
			MCValueRelease(tabs);
			/* UNCHECKED */ MCStringSplit(menustring, MCSTR("\n"), nil, kMCStringOptionCompareExact, tabs);
		}
	return changed;
}

void MCButton::reseticon()
{
	if (!opened || icons == NULL)
		return;
	
	Bool t_is_hovering;
	t_is_hovering = ishovering && MCU_point_in_rect(rect, mx, my);
		
	uint2 i = CI_DEFAULT;
	if (flags & F_DISABLED)
		i = CI_DISABLED;
	else if (state & CS_ARMED)
	{
		if (MCbuttonstate)
			i = CI_HILITED;
		else if (t_is_hovering && icons -> iconids[CI_HOVER] != 0)
			i = CI_HOVER;
		else
			i = CI_ARMED;
	}
	else if (state & CS_HILITED)
		i = CI_HILITED;
	else if (t_is_hovering && icons -> iconids[CI_HOVER] != 0)
		i = CI_HOVER;
	else if (state & CS_VISITED)
		i = CI_VISITED;
	if (icons->iconids[i] == 0)
		i = CI_DEFAULT;
	if (icons->curicon == NULL || icons->curicon->getid() != icons->iconids[i])
	{
		if (icons->curicon != NULL)
		{
			icons->curicon->close();
			icons->curicon = NULL;
		}
		// MW-2009-02-02: [[ Improved image search ]]
		// Search for the appropriate image object using the standard method - note here
		// the image is searched for relative to the Button itself.
		if (icons->iconids[i])
			icons->curicon = resolveimageid(icons -> iconids[i]);
		if (icons->curicon != NULL)
		{
			icons->curicon->open();
			icons->curicon->addneed(this);
		}
	}
}

void MCButton::radio()
{
	if (family == 0)
		return;
	uint2 i;
	MCCard *cptr = getcard();
	Chunk_term ptype = parent->gettype() == CT_GROUP ? CT_BACKGROUND : CT_CARD;
	for (cptr->count(CT_BUTTON, ptype, NULL, i, True) ; i ; i--)
	{
		MCButton *bptr = (MCButton *)cptr->getnumberedchild(i, CT_BUTTON, ptype);
		if (bptr == this)
			continue;
		if (bptr->parent == parent && bptr->family == family)
			bptr->resethilite(cptr->getid(), False);
	}
}

class MCMenuPickBuilder: public IParseMenuCallback
{
public:
	MCMenuPickBuilder(uint32_t index)
	{
		m_target_index = index;
		m_current_index = 0;
		m_tags = nil;
		m_tag_count = 0;
		m_done = false;
	}
	
	~MCMenuPickBuilder(void)
	{
		for (uint32_t i = 0; i < m_tag_count; i++)
			MCValueRelease(m_tags[i]);
		MCMemoryDeleteArray(m_tags);
	}
	
	bool Start(void)
	{
		return false;
	}
	
	bool ProcessItem(MCMenuItem *p_menu_item)
	{
		if (m_done)
			return true;
		
		MCStringRef t_tag = nil;
		
        // SN-2014-07-29: [[ Bug 12998 ]] has_tag member put back
        //  as it was in 6.x
		if (!MCStringIsEmpty(p_menu_item->tag))
        {
            p_menu_item -> has_tag = true;
            t_tag = p_menu_item->tag;
        }
		else
        {
            p_menu_item -> has_tag = false;
            t_tag = p_menu_item->label;
        }
		
		for(uint32_t i = p_menu_item -> depth; i < m_tag_count; i++)
			MCValueRelease(m_tags[i]);
		
		MCMemoryResizeArray(p_menu_item -> depth + 1, m_tags, m_tag_count);
		m_tags[p_menu_item -> depth] = MCValueRetain(t_tag);
		
		m_current_index += 1;
		
		if (m_current_index == m_target_index)
		{
			m_done = true;
			return true;
		}
		
		return false;
	}
	
	bool End(bool)
	{
		return false;
	}
	
	void GetPickString(MCStringRef &r_string)
	{
		MCStringRef t_string = nil;
		/* UNCHECKED */ MCStringMutableCopy(m_tags[0], t_string);
		for(uint32_t i = 1; i < m_tag_count; i++)
		{
            MCStringAppendFormat(t_string, "|%@", m_tags[i]);
		}
		MCStringCopyAndRelease(t_string, r_string);
	}
	
	uint32_t GetPickIndex(void)
	{
		return m_current_index;
	}
	
private:
	bool m_done;
	uint32_t m_current_index;
	uint32_t m_target_index;
	MCStringRef *m_tags;
	uint32_t m_tag_count;
};

void MCButton::setmenuhistoryprop(int2 newline)
{
	if (menuhistory == newline)
		return;
	
	menuhistory = newline;
	
	signallisteners(P_MENU_HISTORY);
}

void MCButton::setmenuhistory(int2 newline)
{
	// I.M. [[bz 9603]] if there is no menu, then return
	if (MCStringIsEmpty(menustring))
		return;
	if (menumode == WM_CASCADE || menumode == WM_POPUP || menumode == WM_PULLDOWN)
	{
		MCMenuPickBuilder t_builder(newline);
		MCParseMenuString(menustring, &t_builder, menumode);
		
		setmenuhistoryprop(t_builder . GetPickIndex());
		
		MCStringRef t_which;
		t_builder.GetPickString(t_which);
		handlemenupick(t_which, nil);
		
		resetlabel();
	}
	else
	{
		if (!(getstyleint(flags) == F_MENU && menumode == WM_TOP_LEVEL) || !opened)
		{
			MCValueRelease(tabs);
			/* UNCHECKED */ MCStringSplit(menustring, MCSTR("\n"), nil, kMCStringOptionCompareExact, tabs);
		}
		uindex_t t_ntabs;
		t_ntabs = MCArrayGetCount(tabs);
		uint2 oldline = menuhistory;
		setmenuhistoryprop(MCU_max(MCU_min(newline, (int2)t_ntabs), 1));
		
        // SN-2014-09-03: [[ Bug 13328 ]] menupick should no be sent if there is a
        // menuname: the oldline belongs to the panel stack, and certainly doesn't match
        // the menustring of this button. At least, it would set a bad label, at worst,
        // it gives garbage (and crashes in 7.0)
        if (MCNameIsEmpty(menuname) && menuhistory != oldline && !(state & CS_MFOCUSED) && t_ntabs > 0)
		{
			MCValueRef t_menuhistory = nil;
			MCValueRef t_oldline = nil;
			/* UNCHECKED */ MCArrayFetchValueAtIndex(tabs, menuhistory, t_menuhistory);
			/* UNCHECKED */ MCArrayFetchValueAtIndex(tabs, oldline, t_oldline);
			handlemenupick(t_menuhistory, t_oldline);
		}

		resetlabel();
		if (!(getstyleint(flags) == F_MENU && menumode == WM_TOP_LEVEL) || !opened)
		{
			MCValueAssign(tabs, kMCEmptyArray);
		}
		if (menuhistory != oldline)
		{
			// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
			layer_redrawall();
		}
	}
}

uint2 MCButton::getmousetab(int2 &curx)
{
	uint2 i = 0;
#ifdef _MAC_DESKTOP
	if (IsMacLFAM() && MCaqua)
	{
		int4 totalwidth = 0;
		for (i = 0 ; i < MCArrayGetCount(tabs) ; i++)
		{
			MCValueRef t_tabval;
			/* UNCHECKED */ MCArrayFetchValueAtIndex(tabs, i + 1, t_tabval);
			MCStringRef t_tab;
			t_tab = (MCStringRef)t_tabval;
			
			MCRange t_range;
			t_range = MCRangeMake(0, MCStringGetLength(t_tab));
			if (MCStringGetCharAtIndex(t_tab, t_range.offset) == '(')
			{
				t_range.offset++;
				t_range.length--;
			}
            // MM-2014-04-16: [[ Bug 11964 ]] Pass through the transform of the stack to make sure the measurment is correct for scaled text.
            totalwidth += MCFontMeasureTextSubstring(m_font, t_tab, t_range, getstack() -> getdevicetransform()) + 23;
		}
		if (totalwidth < rect.width)
			curx += (rect.width - totalwidth) >> 1;
		if (mx < curx)
			return MAXUINT2;
	}
#endif
	MCRectangle tsrect;
	MCU_set_rect(tsrect, mx, my, 1, 1);
	if (!maskrect(tsrect))
		return MAXUINT2;
	int2 tx = curx;
	uint2 theight = MCFontGetAscent(m_font) + 10;
	int2 taboverlap,tabrightmargin,tableftmargin;
	taboverlap = tabrightmargin = tableftmargin = 0;
	//catch the values here
	if (MCcurtheme)
	{
		taboverlap = MCcurtheme->getmetric(WTHEME_METRIC_TABOVERLAP);
		tabrightmargin = MCcurtheme->getmetric(WTHEME_METRIC_TABRIGHTMARGIN);
		tableftmargin = MCcurtheme->getmetric(WTHEME_METRIC_TABLEFTMARGIN);
	}
	uindex_t t_ntabs;
	t_ntabs = MCArrayGetCount(tabs);
	for (i = 0 ; i < t_ntabs ; i++)
	{
		MCValueRef t_tabval = nil;
		/* UNCHECKED */ MCArrayFetchValueAtIndex(tabs, i + 1, t_tabval);
		MCStringRef t_tab;
		t_tab = (MCStringRef)t_tabval;
        
        // AL-2014-09-24: [[ Bug 13528 ]] Measure disabled tab text correctly
        MCRange t_range;
        t_range = MCRangeMake(0, MCStringGetLength(t_tab));
        if (MCStringGetCharAtIndex(t_tab, t_range.offset) == '(')
        {
            t_range.offset++;
            t_range.length--;
        }
        
        // MM-2014-04-16: [[ Bug 11964 ]] Pass through the transform of the stack to make sure the measurment is correct for scaled text.
		if (MCcurtheme)
            tx += MCFontMeasureTextSubstring(m_font, t_tab, t_range, getstack() -> getdevicetransform()) + tabrightmargin + tableftmargin - taboverlap;
		else
		{
			if (IsMacLF())
                tx += MCFontMeasureTextSubstring(m_font, t_tab, t_range, getstack() -> getdevicetransform()) + theight * 2 / 3 + 7;
			else
                tx += MCFontMeasureTextSubstring(m_font, t_tab, t_range, getstack() -> getdevicetransform()) + 12;

		}
		if (mx < tx)
			break;
	}
	return i;
}

// MW-2007-07-05: [[ Bug 2328 ]] - Formatted width of tab buttons incorrect.
int4 MCButton::formattedtabwidth(void)
{
	uint2 theight = MCFontGetAscent(m_font) + 10;

	int2 taboverlap,tabrightmargin,tableftmargin;
	taboverlap = tabrightmargin = tableftmargin = 0;
	if (MCcurtheme)
	{
		taboverlap = MCcurtheme->getmetric(WTHEME_METRIC_TABOVERLAP);
		tabrightmargin = MCcurtheme->getmetric(WTHEME_METRIC_TABRIGHTMARGIN);
		tableftmargin = MCcurtheme->getmetric(WTHEME_METRIC_TABLEFTMARGIN);
	}

    int4 tx;
	tx = 0;
	uindex_t t_ntabs;
	t_ntabs = MCArrayGetCount(tabs);
	for (uint4 i = 0 ; i < t_ntabs ; i++)
    {
		MCValueRef t_tabval = nil;
		/* UNCHECKED */ MCArrayFetchValueAtIndex(tabs, i + 1, t_tabval);
		MCStringRef t_tab;
		t_tab = (MCStringRef)t_tabval;
        // MM-2014-04-16: [[ Bug 11964 ]] Pass through the transform of the stack to make sure the measurment is correct for scaled text.
		if (MCcurtheme)
            tx += MCFontMeasureText(m_font, t_tab, getstack() -> getdevicetransform()) + tabrightmargin + tableftmargin - taboverlap;
		else
		{
			if (IsMacLF())
                tx += MCFontMeasureText(m_font, t_tab, getstack() -> getdevicetransform()) + theight * 2 / 3 + 7;
			else
                tx += MCFontMeasureText(m_font, t_tab, getstack() -> getdevicetransform()) + 12;
		}
	}
	if (t_ntabs > 0)
		tx += taboverlap + tabrightmargin;
	return tx;
}

#include "icondata.cpp"

static void openicon(MCImage *&icon, uint1 *data, uint4 size)
{
	// MW-2012-02-17: [[ FontRefs ]] Make sure we set a parent on the icon, and also
	//   make it invisible. If we don't do this we get issues with parent references
	//   and fontrefs.
	icon = new (nothrow) MCImage;
	icon->setparent(MCdispatcher);
	icon->setflag(False, F_VISIBLE);
	icon->setflag(True, F_I_ALWAYS_BUFFER);
	icon->set_gif(data, size);
	icon->open();
}

void MCButton::allocicons()
{
	openicon(macrb, macrbdata, sizeof(macrbdata));
	openicon(macrbtrack, macrbtrackdata, sizeof(macrbtrackdata));
	openicon(macrbhilite, macrbhilitedata, sizeof(macrbhilitedata));
	openicon(macrbhilitetrack, macrbhilitetrackdata,
	         sizeof(macrbhilitetrackdata));
}

static void closeicon(MCImage *&icon)
{
	if (icon != NULL)
	{
		icon->close();
		icon->set_gif(NULL, 0);
		delete icon;
		icon = NULL;
	}
}

void MCButton::freeicons()
{
	closeicon(macrb);
	closeicon(macrbtrack);
	closeicon(macrbhilite);
	closeicon(macrbhilitetrack);
}

// MW-2012-02-16: [[ IntrinsicUnicode ]] This method switches all the text in
//   the button to or from unicode (to unicode if 'to_unicode' is set).
void MCButton::switchunicode(bool p_to_unicode)
{
	if (p_to_unicode)
		m_font_flags |= FF_HAS_UNICODE;
	else
		m_font_flags &= ~FF_HAS_UNICODE;
}

// MW-2012-02-16: [[ IntrinsicUnicode ]] This method attempts to coerce the text in
//   button to native, but only if no information is lost as a result.
void MCButton::trytochangetonative(void)
{
	if (!hasunicode())
		return;

	switchunicode(false);
}

///////////////////////////////////////////////////////////////////////////////
//
//  SAVING AND LOADING
//

#define BUTTON_EXTRA_ICONGRAVITY (1 << 0)

IO_stat MCButton::extendedsave(MCObjectOutputStream& p_stream, uint4 p_part, uint32_t p_version)
{
	// Extended data area for a button consists of:
	//   uint4 hover_icon;
	//   tag extension_block (null for now)
	//   MCObject::extensions

	IO_stat t_stat;
	t_stat = p_stream . WriteU32(icons == NULL ? 0 : icons -> iconids[CI_HOVER]);
    
    uint4 t_flags;
    t_flags = 0;
    
    uint4 t_length;
    t_length = 0;
    
    // MW-2014-06-20: [[ IconGravity ]] If we have a gravity then we need a gravity block.
    //   Note: we save as a uint32_t even though it only needs 4-bits; this will allow
    //         us to use the same field to save similar properties for label and such, the
    //         defaults being 0 meaning 'legacy behavior'.
    if (m_icon_gravity != kMCGravityNone)
    {
        t_flags |= BUTTON_EXTRA_ICONGRAVITY;
        t_length += sizeof(uint32_t);
    }
    
    if (t_stat == IO_NORMAL)
        t_stat = p_stream . WriteTag(t_flags, t_length);
    
    if (t_stat == IO_NORMAL && (t_flags & BUTTON_EXTRA_ICONGRAVITY))
        t_stat = p_stream . WriteU32(m_icon_gravity);
    
	if (t_stat == IO_NORMAL)
		t_stat = MCObject::extendedsave(p_stream, p_part, p_version);

	return t_stat;
}

IO_stat MCButton::extendedload(MCObjectInputStream& p_stream, uint32_t p_version, uint4 p_remaining)
{
	IO_stat t_stat;
	t_stat = IO_NORMAL;

	if (p_remaining >= 4)
	{
		uint4 t_hover_icon_id;
		t_stat = checkloadstat(p_stream . ReadU32(t_hover_icon_id));
		if (t_stat == IO_NORMAL)
		{
			icons = new (nothrow) iconlist;
			memset(icons, 0, sizeof(iconlist));
			icons -> iconids[CI_HOVER] = t_hover_icon_id;
		}

        if (t_stat == IO_NORMAL)
            p_remaining -= 4;
	}

    if (p_remaining > 0)
    {
		uint4 t_flags, t_length, t_header_length;
		t_stat = checkloadstat(p_stream . ReadTag(t_flags, t_length, t_header_length));
        
		if (t_stat == IO_NORMAL)
			t_stat = checkloadstat(p_stream . Mark());
        
        // MW-2014-06-20: [[ IconGravity ]] Read in the iconGravity property.
        if (t_stat == IO_NORMAL && (t_flags & BUTTON_EXTRA_ICONGRAVITY) != 0)
        {
            uint32_t t_value;
            t_stat = checkloadstat(p_stream . ReadU32(t_value));
            if (t_stat == IO_NORMAL)
                m_icon_gravity = (MCGravity)t_value;
        }
        
        if (t_stat == IO_NORMAL)
            t_stat = checkloadstat(p_stream . Skip(t_length));
        
        if (t_stat == IO_NORMAL)
            p_remaining -= t_length + t_header_length;
    }
    
	if (t_stat == IO_NORMAL)
		t_stat = MCObject::extendedload(p_stream, p_version, p_remaining);

	return t_stat;
}

IO_stat MCButton::save(IO_handle stream, uint4 p_part, bool p_force_ext, uint32_t p_version)
{
	IO_stat stat;
	
	if ((stat = IO_write_uint1(OT_BUTTON, stream)) != IO_NORMAL)
		return stat;
	if (entry != NULL)
		getentrytext();
	if (leftmargin != defaultmargin
	        || leftmargin != rightmargin
	        || leftmargin != topmargin
	        || leftmargin != bottommargin)
		flags &= ~F_NO_MARGINS;
	if (icons == NULL)
		flags &= ~F_HAS_ICONS;
	else
		flags |= F_HAS_ICONS;

	bool t_has_extension;
	t_has_extension = icons != NULL && icons -> iconids[CI_HOVER] != 0;
    
    // MW-2014-06-20: [[ IconGravity ]] Force an extension if non-legacy gravity.
    if (m_icon_gravity != kMCGravityNone)
        t_has_extension = true;
    
	if ((stat = MCObject::save(stream, p_part, t_has_extension || p_force_ext,
	                           p_version)) != IO_NORMAL)
		return stat;

	if (flags & F_HAS_ICONS)
	{
		uint2 i;
		for (i = CI_ARMED ; i < CI_FILE_NICONS ; i++)
			if ((stat = IO_write_uint4(icons->iconids[i], stream)) != IO_NORMAL)
				return stat;
	}
	// MW-2013-11-19: [[ UnicodeFileFormat ]] If sfv >= 7000, use unicode; otherwise use
	//   legacy unicode output.
    // SN-2014-08-05: [[ Bug 13100 ]] An empty label is not an issue,
    //  we need to rely on the F_LABEL flag
    if (flags & F_LABEL)
	{
		if (p_version < kMCStackFileFormatVersion_7_0)
		{
			if ((stat = IO_write_stringref_legacy(label, stream, hasunicode())) != IO_NORMAL)
				return stat;
		}
		else
		{
			if ((stat = IO_write_stringref_new(label, stream, true)) != IO_NORMAL)
				return stat;
		}
    }
	if (flags & F_LABEL_WIDTH)
		if ((stat = IO_write_uint2(labelwidth, stream)) != IO_NORMAL)
			return stat;
	if (!(flags & F_NO_MARGINS))
	{
		if ((stat = IO_write_int2(leftmargin, stream)) != IO_NORMAL)
			return stat;
		if ((stat = IO_write_int2(rightmargin, stream)) != IO_NORMAL)
			return stat;
		if ((stat = IO_write_int2(topmargin, stream)) != IO_NORMAL)
			return stat;
		if ((stat = IO_write_int2(bottommargin, stream)) != IO_NORMAL)
			return stat;
	}
	// MW-2013-11-19: [[ UnicodeFileFormat ]] If sfv >= 7000, use unicode.
	if ((stat = IO_write_nameref_new(menuname, stream, p_version >= kMCStackFileFormatVersion_7_0)) != IO_NORMAL)
		return stat;
	// MW-2013-11-19: [[ UnicodeFileFormat ]] If sfv >= 7000, use unicode; otherwise use
	//   legacy unicode output.
    if (flags & F_MENU_STRING)
	{
		if (p_version < kMCStackFileFormatVersion_7_0)
		{
			if ((stat = IO_write_stringref_legacy(menustring, stream, hasunicode())) != IO_NORMAL)
				return stat;
		}
		else
		{
			if ((stat = IO_write_stringref_new(menustring, stream, true)) != IO_NORMAL)
				return stat;
		}
    }
	menubutton |= family << 4;
	if ((stat = IO_write_uint1(menubutton, stream)) != IO_NORMAL)
		return stat;
	menubutton &= 0x0F;
	
	// WM_SHEET was added after the fileformat menumode enum was fixed
	// since it wasn't added to the end, we must adjust.
	if ((stat = IO_write_uint1(menumode > WM_MODAL ? menumode - 1 : menumode, stream)) != IO_NORMAL)
		return stat;
	if ((menumode == WM_OPTION || menumode == WM_TOP_LEVEL) && (!MCNameIsEmpty(menuname) || flags & F_MENU_STRING))
		if ((stat = IO_write_uint2(menuhistory, stream)) != IO_NORMAL)
			return stat;
	if (flags & F_MENU_LINES)
		if ((stat = IO_write_uint2(menulines, stream)) != IO_NORMAL)
            return stat;
	
	// MW-2013-11-19: [[ UnicodeFileFormat ]] If sfv >= 7000, use unicode; otherwise use
	//   legacy unicode output.
	if (p_version < kMCStackFileFormatVersion_7_0)
	{
		if ((stat = IO_write_stringref_legacy(acceltext, stream, hasunicode())) != IO_NORMAL)
			return stat;
	}
	else
	{
		if ((stat = IO_write_stringref_new(acceltext, stream, true)) != IO_NORMAL)
			return stat;
	}

	if ((stat = IO_write_uint2(accelkey, stream)) != IO_NORMAL)
		return stat;
	if ((stat = IO_write_uint1(accelmods, stream)) != IO_NORMAL)
		return stat;
	if ((stat = IO_write_uint1(mnemonic, stream)) != IO_NORMAL)
		return stat;

	if ((stat = savepropsets(stream, p_version)) != IO_NORMAL)
		return stat;

	MCCdata *tptr = bdata;
	if (tptr != NULL && (p_part == 0 || !getflag(F_SHARED_HILITE)))
	{
		do
		{
			if ((stat = tptr->save(stream, OT_BDATA, p_part, nil, p_version)) != IO_NORMAL)
				return stat;
			tptr = (MCCdata *)tptr->next();
		}
		while (tptr != bdata);
	}
	return IO_NORMAL;
}

IO_stat MCButton::load(IO_handle stream, uint32_t version)
{
	IO_stat stat;

	if ((stat = MCControl::load(stream, version)) != IO_NORMAL)
		return checkloadstat(stat);

	// MW-2012-02-17: [[ IntrinsicUnicode ]] If the unicode tag is set, then we are unicode.
	if ((m_font_flags & FF_HAS_UNICODE_TAG) != 0)
		m_font_flags |= FF_HAS_UNICODE;

	if (version <= kMCStackFileFormatVersion_2_3)
	{
		uint4 iconid;
		uint4 hiliteiconid = 0;
		if ((stat = IO_read_uint4(&iconid, stream)) != IO_NORMAL)
			return checkloadstat(stat);
		if (flags & F_HAS_ICONS)
			if ((stat = IO_read_uint4(&hiliteiconid, stream)) != IO_NORMAL)
				return checkloadstat(stat);
		if (iconid != 0 || hiliteiconid != 0)
		{
			flags |= F_HAS_ICONS;
			icons = new (nothrow) iconlist;
			memset(icons, 0, sizeof(iconlist));
			icons->iconids[CI_DEFAULT] = iconid;
			icons->iconids[CI_HILITED] = hiliteiconid;
		}
		else
			flags &= ~F_HAS_ICONS;
	}
	else
		if (flags & F_HAS_ICONS)
		{
			// MW-2008-08-05: If the icon list hasn't been initialized, do so.
			//   It will have been initialized if the object has an extended data
			//   area.
			if (icons == NULL)
			{
				icons = new (nothrow) iconlist;
				memset(icons, 0, sizeof(iconlist));
			}

			icons->curicon = NULL;
			uint2 i;
			for (i = CI_ARMED ; i < CI_FILE_NICONS ; i++)
				if ((stat = IO_read_uint4(&icons->iconids[i], stream)) != IO_NORMAL)
					return checkloadstat(stat);
		}
	
	// MW-2013-11-19: [[ UnicodeFileFormat ]] If sfv >= 7000, use unicode; otherwise use
	//   legacy unicode output.
	if (flags & F_LABEL)
	{
		if (version < kMCStackFileFormatVersion_7_0)
		{
			if ((stat = IO_read_stringref_legacy(label, stream, hasunicode())) != IO_NORMAL)
				return checkloadstat(stat);
		}
		else
		{
			if ((stat = IO_read_stringref_new(label, stream, true)) != IO_NORMAL)
				return checkloadstat(stat);
		}
	}

	if (flags & F_LABEL_WIDTH)
		if ((stat = IO_read_uint2(&labelwidth, stream)) != IO_NORMAL)
			return checkloadstat(stat);

	if (!(flags & F_NO_MARGINS))
	{
		if ((stat = IO_read_int2(&leftmargin, stream)) != IO_NORMAL)
			return checkloadstat(stat);
		if ((stat = IO_read_int2(&rightmargin, stream)) != IO_NORMAL)
			return checkloadstat(stat);
		if ((stat = IO_read_int2(&topmargin, stream)) != IO_NORMAL)
			return checkloadstat(stat);
		if ((stat = IO_read_int2(&bottommargin, stream)) != IO_NORMAL)
			return checkloadstat(stat);
		if (leftmargin == defaultmargin
		        && leftmargin == rightmargin
		        && leftmargin == topmargin
		        && leftmargin == bottommargin)
			flags |= F_NO_MARGINS;
	}
	
	// MW-2013-11-19: [[ UnicodeFileFormat ]] If sfv >= 7000, use unicode.
	if ((stat = IO_read_nameref_new(menuname, stream, version >= kMCStackFileFormatVersion_7_0)) != IO_NORMAL)
		return checkloadstat(stat);
	
	// MW-2013-11-19: [[ UnicodeFileFormat ]] If sfv >= 7000, use unicode; otherwise use
	//   legacy unicode output.
	if (flags &  F_MENU_STRING)
	{
		if (version < kMCStackFileFormatVersion_7_0)
		{
			if ((stat = IO_read_stringref_legacy(menustring, stream, hasunicode())) != IO_NORMAL)
				return checkloadstat(stat);
		}
		else
		{
			if ((stat = IO_read_stringref_new(menustring, stream, true)) != IO_NORMAL)
				return checkloadstat(stat);
		}
	}

	if ((stat = IO_read_uint1(&menubutton, stream)) != IO_NORMAL)
		return checkloadstat(stat);
	family = menubutton >> 4;
	menubutton &= 0x0F;

	if ((stat = IO_read_uint1(&menumode, stream)) != IO_NORMAL)
		return checkloadstat(stat);

	if (menumode > WM_MODAL)
		menumode++;
	if ((menumode == WM_OPTION || menumode == WM_TOP_LEVEL)
	        && (!MCNameIsEmpty(menuname) || flags & F_MENU_STRING))
		if ((stat = IO_read_uint2(&menuhistory, stream)) != IO_NORMAL)
			return checkloadstat(stat);

	if (flags & F_MENU_LINES)
		if ((stat = IO_read_uint2(&menulines, stream)) != IO_NORMAL)
			return checkloadstat(stat);
	
	// MW-2013-11-19: [[ UnicodeFileFormat ]] If sfv >= 7000, use unicode; otherwise use
	//   legacy unicode output.
	if (version < kMCStackFileFormatVersion_7_0)
	{
		if ((stat = IO_read_stringref_legacy(acceltext, stream, hasunicode())) != IO_NORMAL)
			return checkloadstat(stat);
	}
	else
	{
		if ((stat = IO_read_stringref_new(acceltext, stream, true)) != IO_NORMAL)
			return checkloadstat(stat);
	}

	if ((stat = IO_read_uint2(&accelkey, stream)) != IO_NORMAL)
		return checkloadstat(stat);
	if (accelkey < 256)
#ifdef __MACROMAN__
		accelkey = MCisotranslations[accelkey];
#else
		accelkey = MCmactranslations[accelkey];
#endif

	if ((stat = IO_read_uint1(&accelmods, stream)) != IO_NORMAL)
		return checkloadstat(stat);
	if ((stat = IO_read_uint1(&mnemonic, stream)) != IO_NORMAL)
		return checkloadstat(stat);
	if (version <= kMCStackFileFormatVersion_2_0)
	{
		if (flags & F_DEFAULT)
			rect = MCU_reduce_rect(rect, MOTIF_DEFAULT_WIDTH);
		else
			if (flags & F_TRAVERSAL_ON && !(flags & F_AUTO_ARM))
				rect = MCU_reduce_rect(rect, MCfocuswidth);
		if (!MCNameIsEmpty(menuname))
		{
			if (menumode != WM_CASCADE)
				flags &= ~F_AUTO_ARM;
			flags = (flags & ~F_STYLE) | F_MENU | F_OPAQUE;
		}
		if (flags & F_AUTO_ARM)
			flags |= F_OPAQUE | F_TRAVERSAL_ON;
	}

	if ((stat = loadpropsets(stream, version)) != IO_NORMAL)
		return checkloadstat(stat);

	while (True)
	{
		uint1 type;
		if ((stat = IO_read_uint1(&type, stream)) != IO_NORMAL)
			return checkloadstat(stat);
		if (type == OT_BDATA)
		{
			MCCdata *newbdata = new (nothrow) MCCdata;
			if ((stat = newbdata->load(stream, this, version)) != IO_NORMAL)
			{
				delete newbdata;
				return checkloadstat(stat);
			}
			newbdata->appendto(bdata);
		}
		else
		{
			MCS_seek_cur(stream, -1);
			break;
		}
	}
	return IO_NORMAL;
}

MCStack * MCButton::getmenu()
{
    return menu;
}

////////////////////////////////////////////////////////////////////////////////

MCPlatformControlType MCButton::getcontroltype()
{
    MCPlatformControlType t_type;
    t_type = MCObject::getcontroltype();
    
    if (t_type != kMCPlatformControlTypeGeneric)
        return t_type;
    else
        t_type = kMCPlatformControlTypeButton;
    
    if (getstyleint(flags) == F_CHECK)
        t_type = kMCPlatformControlTypeCheckbox;
    else if (getstyleint(flags) == F_RADIO)
        t_type = kMCPlatformControlTypeRadioButton;
    else if (getstyleint(flags) == F_MENU || menucontrol != MENUCONTROL_NONE)
    {
        t_type = kMCPlatformControlTypeMenu;
        switch (menumode)
        {
            case WM_POPUP:
                t_type = kMCPlatformControlTypePopupMenu;
                break;
                
            case WM_OPTION:
                t_type = kMCPlatformControlTypeOptionMenu;
                break;
                
            case WM_COMBO:
                t_type = kMCPlatformControlTypeComboBox;
                break;
                
            case WM_PULLDOWN:
                t_type = kMCPlatformControlTypePulldownMenu;
                break;
                
            case WM_TOP_LEVEL:
                t_type = kMCPlatformControlTypeTabPane;
                break;
                
            default:
                break;
        }
    }
    else if (menucontrol != MENUCONTROL_NONE)
    {
        t_type = kMCPlatformControlTypeMenuItem;
    }
    
    return t_type;
}

MCPlatformControlPart MCButton::getcontrolsubpart()
{
    return kMCPlatformControlPartNone;
}

MCPlatformControlState MCButton::getcontrolstate()
{
    int t_state;
    t_state = MCControl::getcontrolstate();
    
    if (flags & F_DEFAULT)
        t_state |= kMCPlatformControlStateDefault;
    
    if (t_state & kMCPlatformControlStateMouseFocus
        && MCbuttonstate & 1)
        t_state |= kMCPlatformControlStatePressed;
    
    return MCPlatformControlState(t_state);
}

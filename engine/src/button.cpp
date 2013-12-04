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

#include "prefix.h"

#include "core.h"
#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "mcio.h"

#include "execpt.h"
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

uint2 MCButton::mnemonicoffset = 2;
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

// MW-2007-12-11: [[ Bug 5670 ]] This global is set to false before a card issues an
//   mdown to its focused control. If the mdown resulted in a popup being shown on OS X, then it will be
//   set to true. This is to work around the problem with the mouse down message being 'cancelled' on a
//   popup-menu action resulting in the group mdown not returning true, and thus causing the card to
//   send an errant mdown message to the script.

bool MCosxmenupoppedup = false;
bool MCmenupoppedup = false;

Keynames MCButton::button_keys[] =
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

static ModKeyToken modifier_tokens[] =
	{
		{MS_SHIFT, 5, "shift"},
		{MS_SHIFT, 1, "@"},
		{MS_ALT, 6, "option"},
		{MS_ALT, 3, "opt"},
		{MS_ALT, 3, "alt"},
		{MS_ALT, 1, "#"},
		{MS_MAC_CONTROL, 7, "control"},
		{MS_MAC_CONTROL, 4, "ctrl"},
		{MS_MAC_CONTROL, 1, "%"},
		{MS_CONTROL, 7, "command"},
		{MS_CONTROL, 3, "cmd"},
		{MS_CONTROL, 1, "^"},
		{0, 0, NULL}
	};

MCButton::MCButton()
{
	flags |= F_STANDARD | F_TRAVERSAL_ON | F_HILITE_BORDER | F_HILITE_FILL
	         | F_SHOW_NAME | F_ALIGN_CENTER | F_AUTO_HILITE
	         | F_ARM_BORDER | F_SHARED_HILITE;
	bdata = NULL;
	icons = NULL;
	label = NULL;
	labelsize = 0;
	labelwidth = 0;
	menubutton = Button1;
	menumode = WM_CLOSED;
	menuname = NULL;
	menustring = NULL;
	menusize = 0;
	menuhistory = 1;
	menucontrol = MENUCONTROL_NONE;
	menulines = DEFAULT_MENU_LINES;
	menuhasitemtags = false;
	menu = NULL; //stack based menu

#ifdef _MAC_DESKTOP
	bMenuID = 0; //for button/object based menus
#endif

	entry = NULL;
	tabs = NULL;
	ntabs = 0;
	acceltext = NULL;
	acceltextsize = 0;
	accelkey = accelmods = 0;
	mnemonic = 0;
	family = 0;
	ishovering = False;
}

MCButton::MCButton(const MCButton &bref) : MCControl(bref)
{
	if (bref.icons != NULL)
	{
		icons = new iconlist;
		memcpy(icons, bref.icons, sizeof(iconlist));
		icons->curicon = NULL;
	}
	else
		icons = NULL;
	bdata = NULL;
	if (bref.labelsize)
	{
		labelsize = bref.labelsize;
		label = new char[labelsize];
		memcpy(label, bref.label, labelsize);
	}
	else
	{
		label = NULL;
		labelsize = 0;
	}
	labelwidth = bref.labelwidth;
	menuhistory = bref.menuhistory;
	menulines = bref.menulines;
	menubutton = bref.menubutton;
	menumode = bref.menumode;
	menuname = strclone(bref.menuname);
	menucontrol = bref.menucontrol;
	menuhasitemtags = bref.menuhasitemtags;
	menustring = NULL;
	menusize = 0;
	if (bref.menustring)
	{
		menusize = bref.menusize;
		menustring = new char[menusize];
		memcpy(menustring, bref.menustring, menusize);
	}
	menu = NULL;
	entry = NULL;
	tabs = NULL;
	ntabs = 0;
	acceltext = NULL;
	acceltextsize = 0;
	if (bref.acceltext)
	{
		acceltextsize = bref.acceltextsize;
		acceltext = new char[acceltextsize];
		memcpy(acceltext, bref.acceltext, acceltextsize);
	}
	accelkey = bref.accelkey;
	accelmods = bref.accelmods;
	mnemonic = bref.mnemonic;
	ishovering = False;

#ifdef _MAC_DESKTOP
	bMenuID = 0;
#endif

	if (bref.bdata != NULL)
	{
		MCCdata *bptr = bref.bdata;
		do
		{
			MCCdata *newbdata = new MCCdata(*bptr);
			newbdata->appendto(bdata);
			bptr = (MCCdata *)bptr->next();
		}
		while (bptr != bref.bdata);
	}
	family = bref.family;
}

MCButton::~MCButton()
{
	// OK-2009-04-30: [[Bug 7517]] - Ensure the button is actually closed before deletion, otherwise dangling references may still exist,
	// particuarly if the button had icons.
	while (opened)
		close();
	
	// MW-2008-10-28: [[ ParentScripts ]] Flush the parent scripts table if
	//   tsub has the state flag marked.
	if (getstate(CS_IS_PARENTSCRIPT))
		MCParentScript::FlushObject(this);

	delete icons;
	freemenu(True);
	delete label;
	delete acceltext;
	delete menuname;
	delete menustring;
	delete tabs;
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
	if (optr == menu)
		menu = NULL;
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

bool MCButton::visit(MCVisitStyle p_style, uint32_t p_part, MCObjectVisitor* p_visitor)
{
	return p_visitor -> OnButton(this);
}

void MCButton::open()
{
	// MW-2008-10-28: [[ ParentScripts ]] We have to preserve the setting of the
	//   CS_IS_PARENTSCRIPT state.
	if (!getstate(CS_IS_PARENTSCRIPT))
		MCControl::open();
	else
	{
		MCControl::open();
		setstate(True, CS_IS_PARENTSCRIPT);
	}

	// MW-2011-02-08: [[ Bug 9382 ]] Make sure we reset icons when opening and the state
	//   has changed (i.e. background transition has occured).
	uint32_t t_old_state;
	t_old_state = state;
	switch(gethilite(0))
	{
	case True:
		state |= CS_HILITED;
		state &= ~CS_MIXED;
		break;
	case False:
		state &= ~(CS_HILITED | CS_MIXED);
		break;
	case Mixed:
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
				MCU_break_string(MCString(menustring, menusize), tabs, ntabs, hasunicode());
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
				if (menuname != NULL && menu != NULL)
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
		if (tabs != NULL)
		{
			delete tabs;
			tabs = NULL;
			ntabs = 0;
		}
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
	if ((IsMacLF() && !(state & CS_SHOW_DEFAULT))
	        && !(flags & F_AUTO_ARM) && entry == NULL
	        || !(flags & F_VISIBLE || MCshowinvisibles)
	        || !(flags & F_TRAVERSAL_ON) || state & CS_KFOCUSED || flags & F_DISABLED)
		return False;
	return True;
}

Boolean MCButton::kfocusprev(Boolean bottom)
{
	if ((IsMacLF() && !(state & CS_SHOW_DEFAULT))
	        && !(flags & F_AUTO_ARM) && entry == NULL
	        || !(flags & F_VISIBLE || MCshowinvisibles)
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

Boolean MCButton::kdown(const char *string, KeySym key)
{
	if (state & CS_SUBMENU)
	{
		if (MCObject::kdown(string, key))
			return True;
		MCButton *bptr;
		MCString pick;
		switch (key)
		{
		case XK_Escape:
			closemenu(True, True);
			return True;
		case XK_Tab:
		case XK_Left:
		case XK_Right:
			if (menu->kdown(string, key))
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
				char exp[U2L];
				sprintf(exp, "%d", newnum);
				bptr = (MCButton *)getcard()->getchild(CT_EXPRESSION, exp,
				                                       CT_MENU, CT_UNDEFINED);
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
			if (menu->kdown(string, key))
				return True;
			return menu->kfocusnext(False);
		case XK_Up:
			if (menu->kdown(string, key))
				return True;
			return menu->kfocusprev(False);
		case XK_WheelDown:
		case XK_WheelUp:
		case XK_WheelLeft:
		case XK_WheelRight:
			if (menu -> getcontrols() -> gettype() == CT_FIELD)
				if (menu -> getcontrols() -> kdown(string, key))
					return True;
			break;
		case XK_space:
		case XK_Return:
		case XK_KP_Enter:
			closemenu(False, True);
			menu->menukdown(string, key, pick, menuhistory);
			if (pick.getstring() != NULL)
			{
				if (menumode == WM_OPTION || menumode == WM_COMBO)
				{
					delete label;
					label = (char *)pick.getstring();
					labelsize = pick.getlength();
					flags |= F_LABEL;
					if (entry != NULL)
						entry->settext(0, pick, False, hasunicode());
					Exec_stat es = message_with_args(MCM_menu_pick, pick);
					if (es == ES_NOT_HANDLED || es == ES_PASS)
						message_with_args(MCM_mouse_up, menubutton);
					// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
					layer_redrawall();
				}
				else
				{
					if (!(state & CS_IGNORE_MENU))
						docascade(pick);
					delete (char *)pick.getstring();
				}
			}
			else
				message_with_args(MCM_mouse_release, menubutton);
			state &= ~CS_IGNORE_MENU;
			if (MCmenuobjectptr == this)
				MCmenuobjectptr = NULL;
			return True;
		default:
			MCButton *mbptr = menu->findmnemonic(string[0]);
			if (mbptr != NULL)
			{
				closemenu(False, True);
				if (menuname != NULL)
					mbptr->activate(False, string[0]);
				else
				{
					MCString slabel;
					bool t_is_unicode;
					if (mbptr->getmenuhastags())
						slabel = mbptr -> getname_oldstring(), t_is_unicode = false;
					else
						mbptr->getlabeltext(slabel, t_is_unicode);
					menu->menukdown(string, key, pick, menuhistory);
					Exec_stat es = message_with_args(MCM_menu_pick, slabel);
					if (es == ES_NOT_HANDLED || es == ES_PASS)
						message_with_args(MCM_mouse_up, menubutton);
					delete (char *)pick.getstring();
				}
				if (MCmenuobjectptr == this)
					MCmenuobjectptr = NULL;
				return True;
			}
			else
			{
				if (MCmodifierstate & MS_MOD1)
				{
					mbptr = MCstacks->findmnemonic(string[0]);
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
			return entry->kdown(string, key);
		if (MCObject::kdown(string, key))
			return True;
		if ((((key == XK_Right || key == XK_space
		        || key == XK_Return || key == XK_KP_Enter)
		        && (menumode == WM_CASCADE || menumode == WM_OPTION))
		        || key == XK_Down && menumode != WM_CASCADE)
		        && state & CS_KFOCUSED && findmenu())
		{
			openmenu(True);
			return True;
		}
		if ((key == XK_space || (state & CS_SHOW_DEFAULT || state & CS_ARMED)
		        && (key == XK_Return || key == XK_KP_Enter))
		        && !(MCmodifierstate & MS_MOD1))
		{
			activate(False, string[0]);
			return True;
		}
	}
	return False;
}

Boolean MCButton::kup(const char *string, KeySym key)
{
	if (entry != NULL)
		return entry->kup(string, key);
	if (MCObject::kup(string, key))
		return True;
	if (state & CS_SUBMENU)
		return menu->kup(string, key);
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
					char string[U2L];
					sprintf(string, "%d", i);
					MCButton *bptr = (MCButton *)cptr->getchild(CT_EXPRESSION, string,
					                 CT_MENU,
					                 CT_UNDEFINED);
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
						bptr->state |= CS_MFOCUSED;
						bptr->message_with_args(MCM_mouse_down, menubutton);
						bptr->findmenu();
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
				if ((MClook != LF_WIN95) || menuname != NULL)
					break;
			case WM_COMBO:
				if (state & CS_MFOCUSED)
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
	if (!(flags & F_VISIBLE || MCshowinvisibles)
	        || flags & F_DISABLED && getstack()->gettool(this) == T_BROWSE)
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
			MCRectangle trect = rect;
			trect.height = 8 + MCFontGetAscent(m_font);
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
				if (MClook == LF_MOTIF || flags & F_SHOW_ICON
				        || getstyleint(flags) != F_RADIO
				        && getstyleint(flags) != F_CHECK)
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
	if (flags & F_AUTO_ARM && state & CS_ARMED)
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
		if (state & CS_SCROLLBAR && mx > rect.x + rect.width - MCscrollbarwidth
		        && mx < rect.x + rect.width)
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
	if ((menuname != NULL || menu != NULL || getstyleint(flags) == F_MENU)
	        && (menubutton == 0 || (uint1)which == menubutton)
	        && (entry == NULL || !MCU_point_in_rect(entry->getrect(), mx, my))
	        && (getstack()->gettool(this) == T_BROWSE
	            || getstack()->gettool(this) == T_HELP))
	{
		message_with_args(MCM_mouse_down, which);
		if (menumode == WM_TOP_LEVEL)
		{
			if (menustring != NULL)
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
					if (starttab < ntabs && starttab + 1 != menuhistory
					        && tabs[starttab].getstring()[0] != '(')
					{
						uint2 oldhist = menuhistory - 1;
						setmenuhistoryprop(starttab + 1);
						// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
						layer_redrawall();
						message_with_args(MCM_menu_pick, tabs[starttab], tabs[oldhist]);
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
		switch (getstack()->gettool(this))
		{
		case T_BROWSE:
			if (entry != NULL && MCU_point_in_rect(entry->getrect(), mx, my))
			{
				state |= CS_FIELD_GRAB;
				entry->mdown(which);
			}
			else
				if (flags & F_AUTO_HILITE || family != 0)
					if (MClook == LF_MOTIF || flags & F_SHOW_ICON
					        || getstyleint(flags) != F_RADIO
					        && getstyleint(flags) != F_CHECK)
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
			if ((!IsMacLF() || entry != NULL)
			        && flags & F_TRAVERSAL_ON && !(state & CS_KFOCUSED))
				getstack()->kfocusset(this);
			message_with_args(MCM_mouse_down, "1");
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
	else
		message_with_args(MCM_mouse_down, which);
	return True;
}

Boolean MCButton::mup(uint2 which)
{
	if (state & CS_MENU_ATTACHED)
		return MCObject::mup(which);
	MCString pick;
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
		if (state & CS_FIELD_GRAB)
		{
			state &= ~CS_FIELD_GRAB;
			if (state & CS_SUBMENU)
				menu->mup(which);
			else
				entry->mup(which);
			return True;
		}
		if (menudepth > mymenudepth)
		{
			menu->mup(which);
			if (menudepth > mymenudepth)
				return True;
		}
		else
			if ((menumode == WM_OPTION || menumode == WM_COMBO)
			        && MCeventtime - clicktime < OPTION_TIME)
				return True;
			else
				if (menumode == WM_PULLDOWN && MCU_point_in_rect(rect, mx, my))
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
		menu->menumup(which, pick, menuhistory);
		MCmenupoppedup = false;
		closemenu(True, True);
		if (!(state & CS_IGNORE_MENU))
			if (pick.getstring() != NULL)
			{
				if (menumode == WM_OPTION || menumode == WM_COMBO)
				{
					delete label;
					label = (char *)pick.getstring();
					labelsize = pick.getlength();
					flags |= F_LABEL;
					if (entry != NULL)
						entry->settext(0, pick, False, hasunicode());
				}
				docascade(pick);
			}
			else
				message_with_args(MCM_mouse_release, which);
		if (menumode != WM_OPTION && menumode != WM_COMBO)
			delete (char *)pick.getstring();
		state &= ~CS_IGNORE_MENU;
		if (MCmenuobjectptr == this)
			MCmenuobjectptr = NULL;
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
		if (flags && F_AUTO_HILITE)
			if (starthilite)
				state &= ~CS_HILITED;
			else
				state |= CS_HILITED;
		ungrab(which);
		return True;
	}
	if (which == Button1)
		switch (getstack()->gettool(this))
		{
		case T_BROWSE:
			if (state & CS_FIELD_GRAB)
			{
				state &= ~CS_FIELD_GRAB;
				entry->mup(which);
			}
			else
			{
				if (getstyleint(flags) == F_MENU && menumode == WM_TOP_LEVEL
				        && tabselectonmouseup())
				{
					int2 curx = rect.x + 2;
					if (getmousetab(curx) == starttab && starttab < ntabs
					        && starttab + 1 != menuhistory
					        && tabs[starttab].getstring()[0] != '(')
					{
						uint2 oldhist = menuhistory - 1;
						setmenuhistoryprop(starttab + 1);
						// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
						layer_redrawall();
						message_with_args(MCM_menu_pick, tabs[starttab], tabs[oldhist]);
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
							if (getstyleint(flags) == F_CHECK)
								state ^= CS_HILITED;
							else
								state |= CS_HILITED;
						// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
						layer_redrawall();
					}
					else
						if (state & CS_HILITED && (flags & F_AUTO_HILITE || family != 0)
						        || state & CS_ARMED && flags & F_AUTO_ARM)
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
										if (state & CS_ARMED)
											state |= CS_HILITED;
										else
											state &= ~CS_HILITED;
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
				if (MCU_point_in_rect(rect, mx, my))
				{
					state |= CS_VISITED;
					message_with_args(MCM_mouse_up, "1");
				}
				else
					message_with_args(MCM_mouse_release, "1");
			}
			break;
		case T_BUTTON:
		case T_POINTER:
			end();
			break;
		case T_HELP:
			help();
			break;

		default:
			return False;
		}
	else
		if (MCU_point_in_rect(rect, mx, my))
		{
			state |= CS_VISITED;
			message_with_args(MCM_mouse_up, which);
		}
		else
			message_with_args(MCM_mouse_release, which);
	if (state & CS_ARMED)
	{
		state &= ~CS_ARMED;
		reseticon();
		// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
		layer_redrawall();
	}
	return True;
}

Boolean MCButton::doubledown(uint2 which)
{
	int2 tx = mx;
	int2 ty = my;
	if (menu)
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
		else if (state & CS_SUBMENU && menu && MCU_point_in_rect(menu->getrect(), tx, ty))
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
		else if (state & CS_SUBMENU && menu)
			return menu -> doubleup(which);
	}
	return MCControl::doubleup(which);
}

#ifdef _MAC_DESKTOP
void MCButton::timer(MCNameRef mptr, MCParameter *params)
{
	if (MCNameIsEqualTo(mptr, MCM_internal, kMCCompareCaseless))
	{
		if (state & CS_SHOW_DEFAULT)
		{
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


void MCButton::setrect(const MCRectangle &nrect)
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

		}
		else
		{
			trect = MCU_reduce_rect(nrect, borderwidth);
			int2 tcombosize = 0;
			if (tcombosize <= 0 )
			{
				trect.width -= trect.height;
				if (tcombosize < 0)
					trect.width += tcombosize;
			}
			else
				rect.width -= tcombosize + 2;
			if (IsMacEmulatedLF())
				trect.width -= 5;
		}
		entry->setrect(trect);
	}

	// MW-2010-06-07: [[ Bug 8746 ]] Make sure we rebuild the menu after freeing it,
	//   thus ensuring accelerators are not lost.
	if (menu != NULL)
	{
		freemenu(False);
		findmenu(true);
	}
}

Exec_stat MCButton::getprop(uint4 parid, Properties which, MCExecPoint& ep, Boolean effective)
{
	uint2 fheight;
	uint2 j = 0;

	switch (which)
	{
#ifdef /* MCButton::getprop */ LEGACY_EXEC
	case P_STYLE:
		{
			const char *t_style_string;
			if (getstyleint(flags) == F_MENU)
				t_style_string = getstack()->hcaddress() ? MCpopupstring : MCmenustring;
			else if (getstyleint(flags) == F_CHECK)
				t_style_string = MCcheckboxstring;
			else if (getstyleint(flags) == F_RADIO)
				t_style_string = MCradiobuttonstring;
			else if (getstyleint(flags) == F_ROUNDRECT)
				t_style_string = MCroundrectstring;
			else if (getstyleint(flags) == F_RECTANGLE)
				t_style_string = MCrectanglestring;
			else if (getstyleint(flags) == F_OVAL_BUTTON)
				t_style_string = MCovalstring;
			else if (!(flags & F_OPAQUE))
				t_style_string = MCtransparentstring;
			else if (flags & F_SHADOW)
				t_style_string = MCshadowstring;
			else if (!(flags & F_SHOW_BORDER))
				t_style_string = MCopaquestring;
			else
				t_style_string = MCstandardstring;
			ep . setstaticcstring(t_style_string);
		}
		break;
	case P_AUTO_ARM:
		ep.setboolean(getflag(F_AUTO_ARM));
		break;
	case P_AUTO_HILITE:
		ep.setboolean(getflag(F_AUTO_HILITE));
		break;
	case P_ARM_BORDER:
		ep.setboolean(getflag(F_ARM_BORDER));
		break;
	case P_ARM_FILL:
		ep.setboolean(getflag(F_ARM_FILL));
		break;
	case P_HILITE_BORDER:
		ep.setboolean(getflag(F_HILITE_BORDER));
		break;
	case P_HILITE_FILL:
		ep.setboolean(getflag(F_HILITE_FILL));
		break;
	case P_SHOW_HILITE:
		ep.setboolean(getflag(F_SHOW_HILITE));
		break;
	case P_ARM:
		ep.setboolean(getstate(CS_ARMED));
		break;
	case P_HILITE:
		j = gethilite(parid);
		if (j == Mixed)
			ep.setstaticcstring(MCmixedstring);
		else
			ep.setboolean((Boolean)j);
		break;
	case P_ARMED_ICON:
	case P_DISABLED_ICON:
	case P_ICON:
	case P_HILITED_ICON:
	case P_VISITED_ICON:
	case P_HOVER_ICON:
			ep.setint(icons == NULL ? 0 : icons->iconids[which - P_ARMED_ICON]);
		break;
	case P_SHARED_HILITE:
		ep.setboolean(getflag(F_SHARED_HILITE));
		break;
	case P_SHOW_ICON:
		ep.setboolean(getflag(F_SHOW_ICON));
		break;
	case P_SHOW_NAME:
		ep.setboolean(getflag(F_SHOW_NAME));
		break;
	// MW-2012-02-16: [[ IntrinsicUnicode ]] Add support for a 'unicodeLabel' property.
	case P_LABEL:
	case P_UNICODE_LABEL:
		{
			// Get the label, noting whether its unicode or not.
			MCString slabel;
			bool isunicode;
			if (entry != NULL || effective)
				getlabeltext(slabel, isunicode);
			else
				slabel.set(label, labelsize), isunicode = hasunicode();
			ep.setsvalue(slabel);

			// Map the label's encoding to the requested encoding.
			ep.mapunicode(isunicode, which == P_UNICODE_LABEL);
		}
		break;
	case P_LABEL_WIDTH:
		ep.setint(labelwidth);
		break;
	case P_FAMILY:
		ep.setint(family);
		break;
	case P_VISITED:
		ep.setboolean(getstate(CS_VISITED));
		break;
	case P_MENU_HISTORY:
		ep.setint(menuhistory);
		break;
	case P_MENU_LINES:
		ep.setint(menulines);
		break;
	case P_MENU_BUTTON:
		ep.setint(menubutton);
		break;
	case P_MENU_MODE:
		{
			const char *t_menumode_string;
			switch (menumode)
			{
			case WM_TOP_LEVEL:
				t_menumode_string = MCtabstring;
				break;
			case WM_PULLDOWN:
				t_menumode_string = MCpulldownstring;
				break;
			case WM_POPUP:
				t_menumode_string = MCpopupstring;
				break;
			case WM_OPTION:
				t_menumode_string = MCoptionstring;
				break;
			case WM_CASCADE:
				t_menumode_string = MCcascadestring;
				break;
			case WM_COMBO:
				t_menumode_string = MCcombostring;
				break;
			default:
				t_menumode_string = MCnullstring;
				break;
			}
			ep . setstaticcstring(t_menumode_string);
		}
		break;
	case P_MENU_NAME:
		ep.setsvalue(menuname);
		break;
	// MW-2012-02-16: [[ IntrinsicUnicode ]] Add support for a 'unicodeAcceleratorText' property.
	case P_ACCELERATOR_TEXT:
	case P_UNICODE_ACCELERATOR_TEXT:
		ep.setsvalue(MCString(acceltext, acceltextsize));
		// Map the menustring's encoding to the requested encoding.
		ep.mapunicode(hasunicode(), which == P_UNICODE_ACCELERATOR_TEXT);
		break;
	case P_ACCELERATOR_KEY:
		if (accelkey & 0xFF00)
		{
			const char *t_keyname = MCLookupAcceleratorName(accelkey);
			if (t_keyname != NULL)
				ep.setsvalue(t_keyname);
			else
				ep.setempty();
		}
		else
			if (accelkey)
			{
				char tmp = (char)accelkey;
				ep.copysvalue(&tmp, 1);
			}
			else
				ep.clear();
		break;
	case P_ACCELERATOR_MODIFIERS:
		ep.setempty();
		if (accelmods & MS_SHIFT)
			ep.concatcstring(MCshiftstring, EC_COMMA, j++ == 0);
		if (accelmods & MS_CONTROL)
#ifdef _MAC_DESKTOP
			ep.concatcstring(MCcommandstring, EC_COMMA, j++ == 0);
		if (accelmods & MS_MAC_CONTROL)
			ep.concatcstring(MCcontrolstring, EC_COMMA, j++ == 0);
#else
			ep.concatcstring(MCcontrolstring, EC_COMMA, j++ == 0);
#endif
		if (accelmods & MS_MOD1)
			ep.concatcstring(MCmod1string, EC_COMMA, j++ == 0);
		break;
	case P_MNEMONIC:
		ep.setint(mnemonic);
		break;
	case P_FORMATTED_WIDTH:
		// MW-2012-02-16: [[ FontRefs ]] As 'formatted' properties require
		//   access to the font, we must be open before we can compute them.
		if (opened)
		{
			// MW-2007-07-05: [[ Bug 2328 ]] - Formatted width of tab buttons incorrect.
			if (getstyleint(flags) == F_MENU && menumode == WM_TOP_LEVEL)
				ep.setint(formattedtabwidth());
			else
			{
				uint2 fwidth;
				bool t_is_unicode;
				MCString slabel;
				getlabeltext(slabel, t_is_unicode);
				if (slabel.getstring() == NULL)
					fwidth = 0;
				else
					fwidth = leftmargin + rightmargin + MCFontMeasureText(m_font, slabel.getstring(), slabel.getlength(), t_is_unicode);
				if (flags & F_SHOW_ICON && icons != NULL)
				{
					reseticon();
					if (icons->curicon != NULL)
					{
						MCRectangle trect = icons->curicon->getrect();
						if (trect.width > fwidth)
							fwidth = trect.width;
					}
				}
				else
					if (getstyleint(flags) == F_CHECK || getstyleint(flags) == F_RADIO)
						fwidth += CHECK_SIZE + leftmargin;
				if (menumode == WM_OPTION)
					fwidth += optionrect.width + (optionrect.width >> 1);
				if (menumode == WM_CASCADE)
					fwidth += rect.height;
				ep.setint(fwidth);
			}
		}
		else
			ep.setint(0);
		break;
	case P_FORMATTED_HEIGHT:
		// MW-2012-02-16: [[ FontRefs ]] As 'formatted' properties require
		//   access to the font, we must be open before we can compute them.
		if (opened)
		{
			fheight = topmargin + bottommargin + MCFontGetAscent(m_font) + MCFontGetDescent(m_font);
			if (flags & F_SHOW_ICON && icons != NULL)
			{
				reseticon();
				if (icons->curicon != NULL)
				{
					MCRectangle trect = icons->curicon->getrect();
					if (trect.height > fheight)
						fheight = trect.height;
				}
			}
			else if ((getstyleint(flags) == F_CHECK || getstyleint(flags) == F_RADIO) && CHECK_SIZE > fheight)
				fheight = CHECK_SIZE;
			else if (getstyleint(flags) == F_MENU && menumode == WM_TOP_LEVEL)
				fheight += 8;
			ep.setint(fheight);
		}
		else
			ep.setint(0);
		break;
	case P_DEFAULT:
		ep.setboolean(getflag(F_DEFAULT));
		break;
	// MW-2012-02-16: [[ IntrinsicUnicode ]] Add support for a 'unicodeText' property.
	case P_TEXT:
	case P_UNICODE_TEXT:
		ep.setsvalue(MCString(menustring, menusize));

		// Map the menustring's encoding to the requested encoding.
		ep.mapunicode(hasunicode(), which == P_UNICODE_TEXT);
		break;
#endif /* MCButton::getprop */ 
	default:
		return MCControl::getprop(parid, which, ep, effective);
	}
	return ES_NORMAL;
}

Exec_stat MCButton::setprop(uint4 parid, Properties p, MCExecPoint &ep, Boolean effective)
{
	Boolean dirty = True;
	Boolean all = p == P_STYLE || p == P_LABEL_WIDTH || MCaqua && standardbtn();
	int2 i1;
	uint2 i = 0;
	uint4 newid;
	MCString data = ep.getsvalue();
	
	switch (p)
	{
#ifdef /* MCButton::setprop */ LEGACY_EXEC
	case P_NAME:
		if (MCObject::setprop(parid, p, ep, effective) != ES_NORMAL)
			return ES_ERROR;
		clearmnemonic();
		setupmnemonic();
		return ES_NORMAL;
	case P_STYLE:
		flags &= ~(F_STYLE | F_DISPLAY_STYLE | F_ALIGNMENT);
		if (entry != NULL)
			deleteentry();
		if (data == MCpopupstring || data == MCmenustring)
		{
			flags |= F_MENU | F_SHOW_BORDER | F_OPAQUE
			         | F_ALIGN_CENTER | F_ARM_BORDER;
			if (menumode == WM_COMBO)
				createentry();
			if (menumode == WM_TOP_LEVEL)
				MCU_break_string(MCString(menustring, menusize), tabs, ntabs, hasunicode());
		}
		else if (data == MCcheckboxstring)
			flags |= F_CHECK | F_ALIGN_LEFT;
		else if (data == MCradiobuttonstring)
		{
			flags |= F_RADIO | F_ALIGN_LEFT;
			flags &= ~F_SHARED_HILITE;
		}
		else if (data == MCroundrectstring)
			flags |=  F_ROUNDRECT | F_SHOW_BORDER | F_OPAQUE
			          | F_ALIGN_CENTER | F_HILITE_FILL;
		else if (data == MCovalstring)
			flags |= F_OVAL_BUTTON | F_HILITE_FILL;
		else if (data == MCtransparentstring)
			flags |= F_STANDARD | F_ALIGN_CENTER;
		else if (data == MCshadowstring)
				flags |= F_STANDARD | F_SHOW_BORDER | F_OPAQUE
			         | F_SHADOW | F_ALIGN_CENTER | F_HILITE_BOTH | F_ARM_BORDER;
		else if (data == MCopaquestring)
			flags |= F_STANDARD | F_OPAQUE
			         | F_ALIGN_CENTER | F_HILITE_FILL | F_ARM_BORDER;
		else if (data == MCrectanglestring)
			flags |= F_RECTANGLE | F_SHOW_BORDER | F_OPAQUE
			         | F_ALIGN_CENTER | F_HILITE_BOTH | F_ARM_BORDER;
		else
			flags |= F_STANDARD | F_SHOW_BORDER | F_OPAQUE
			         | F_ALIGN_CENTER | F_HILITE_BOTH | F_ARM_BORDER;

		// MW-2011-09-21: [[ Layers ]] Make sure the layerattrs are recomputed.
		m_layer_attr_changed = true;
		break;
	case P_AUTO_ARM:
		if (!MCU_matchflags(data, flags, F_AUTO_ARM, dirty))
		{
			MCeerror->add(EE_OBJECT_NAB, 0, 0, data);
			return ES_ERROR;
		}
		break;
	case P_AUTO_HILITE:
		if (!MCU_matchflags(data, flags, F_AUTO_HILITE, dirty))
		{
			MCeerror->add(EE_OBJECT_NAB, 0, 0, data);
			return ES_ERROR;
		}
		break;
	case P_ARM_BORDER:
		if (!MCU_matchflags(data, flags, F_ARM_BORDER, dirty))
		{
			MCeerror->add(EE_OBJECT_NAB, 0, 0, data);
			return ES_ERROR;
		}
		// MW-2011-09-21: [[ Layers ]] Changing the armBorder property
		//   affects the layer attrs.
		if (dirty)
			m_layer_attr_changed = true;
		break;
	case P_ARM_FILL:
		if (!MCU_matchflags(data, flags, F_ARM_FILL, dirty))
		{
			MCeerror->add(EE_OBJECT_NAB, 0, 0, data);
			return ES_ERROR;
		}
		break;
	case P_HILITE_BORDER:
		if (!MCU_matchflags(data, flags, F_HILITE_BORDER, dirty))
		{
			MCeerror->add(EE_OBJECT_NAB, 0, 0, data);
			return ES_ERROR;
		}
		// MW-2011-09-21: [[ Layers ]] Changing the hiliteBorder property
		//   affects the layer attrs.
		if (dirty)
			m_layer_attr_changed = true;
		break;
	case P_HILITE_FILL:
		if (!MCU_matchflags(data, flags, F_HILITE_FILL, dirty))
		{
			MCeerror->add(EE_OBJECT_NAB, 0, 0, data);
			return ES_ERROR;
		}
		break;
	case P_SHOW_HILITE:
		if (!MCU_matchflags(data, flags, F_SHOW_HILITE, dirty))
		{
			MCeerror->add(EE_OBJECT_NAB, 0, 0, data);
			return ES_ERROR;
		}
		break;
	case P_ARM:
		if (!MCU_matchflags(data, state, CS_ARMED, dirty))
		{
			MCeerror->add(EE_OBJECT_NAB, 0, 0, data);
			return ES_ERROR;
		}
		break;
	case P_HILITE:
		Boolean newstate;
		if (data == MCmixedstring)
			newstate = Mixed;
		else
			if (!MCU_stob(data, newstate))
			{
				MCeerror->add(EE_OBJECT_NAB, 0, 0, data);
				return ES_ERROR;
			}
		if (sethilite(parid, newstate))
		{
			if (state & CS_HILITED)
			{
				// MH-2007-03-20: [[ Bug 4035 ]] If the hilite of a radio button is set programmatically, other radio buttons were not unhilited if the radiobehavior of the group is set.
				if (getstyleint(flags) == F_RADIO && parent -> gettype() == CT_GROUP)
				{
					MCGroup *gptr = (MCGroup *)parent;
					gptr->radio(parid, this);
				}
				radio();
			}
			reseticon();
		}
		else
			dirty = False;
		break;
	case P_ARMED_ICON:
	case P_DISABLED_ICON:
	case P_HILITED_ICON:
	case P_ICON:
	case P_VISITED_ICON:
	case P_HOVER_ICON:
		if (icons == NULL)
		{
			icons = new iconlist;
			memset(icons, 0, sizeof(iconlist));
		}
		if (!MCU_stoui4(data, newid))
		{
			// MW-2013-03-06: [[ Bug 10695 ]] When searching for the image to resolve to an id,
			//   make sure we use the behavior aware search function.
			MCImage *ticon = resolveimagename(data);
			if (ticon != NULL)
				newid = ticon->getid();
			else
				newid = 0;
		}
		if (icons->iconids[p - P_ARMED_ICON] != newid)
		{
			icons->iconids[p - P_ARMED_ICON] = newid;
			dirty = True;
		}
		if (dirty)
		{
			if (icons->iconids[CI_ARMED] == 0 && icons->iconids[CI_DISABLED] == 0
			        && icons->iconids[CI_HILITED] == 0 && icons->iconids[CI_DEFAULT] == 0
							&& icons->iconids[CI_VISITED] == 0 && icons->iconids[CI_HOVER] == 0)
			{
				flags &= ~(F_SHOW_ICON | F_HAS_ICONS);
				if (icons->curicon != NULL)
				{
					icons->curicon->close();
				}
				delete icons;
				icons = NULL;
			}
			else
			{
				flags |= F_SHOW_ICON | F_HAS_ICONS;
				reseticon();
			}
		}
		break;
	case P_SHARED_HILITE:
		if (!MCU_matchflags(data, flags, F_SHARED_HILITE, dirty))
		{
			MCeerror->add(EE_OBJECT_NAB, 0, 0, data);
			return ES_ERROR;
		}
		break;
	case P_SHOW_ICON:
		if (!MCU_matchflags(data, flags, F_SHOW_ICON, dirty))
		{
			MCeerror->add(EE_OBJECT_NAB, 0, 0, data);
			return ES_ERROR;
		}
		// MW-2011-09-21: [[ Layers ]] Changing the showIcon property
		//   affects the layer attrs.
		if (dirty)
			m_layer_attr_changed = true;
		break;
	case P_SHOW_NAME:
		if (!MCU_matchflags(data, flags, F_SHOW_NAME, dirty))
		{
			MCeerror->add(EE_OBJECT_NAB, 0, 0, data);
			return ES_ERROR;
		}
		// MW-2011-09-21: [[ Layers ]] Changing the showName property
		//   affects the layer attrs.
		if (dirty)
			m_layer_attr_changed = true;
		break;
	// MW-2012-02-16: [[ IntrinsicUnicode ]] Add support for setting the 
	//   'unicodeLabel'.
	case P_LABEL:
	case P_UNICODE_LABEL:
		// Make sure the label is up to date.
		if (entry != NULL)
			getentrytext();

		// If we aren't unicode and are setting unicode, first coerce all text
		// to unicode; otherwise, if we are unicode and are setting native, convert
		// the ep to unicode.
		if (p == P_UNICODE_LABEL && !hasunicode())
			switchunicode(true);
		else if (p == P_LABEL && hasunicode())
			ep.nativetoutf16();

		// Only do anything if there is a change.
		if (label == NULL || data.getlength() != labelsize
		        || memcmp(data.getstring(), label, data.getlength()) != 0)
		{
			delete label;
			if (data != MCnullmcstring)
			{
				labelsize = data.getlength();
				label = new char[labelsize];
				memcpy(label, data.getstring(), labelsize);
				flags |= F_LABEL;
			}
			else
			{
				label = NULL;
				labelsize = 0;
				flags &= ~F_LABEL;
			}

			// Now that we've updated the label, try to change everything to native.
			trytochangetonative();

			if (entry != NULL)
				if (label == NULL)
					entry->settext(0, MCnullmcstring, False, False);
				else
					entry->settext(0, MCString(label, labelsize), False, hasunicode());

			clearmnemonic();
			setupmnemonic();
		}
		else
		{
			// Try to change everything back to native.
			trytochangetonative();

			dirty = False;
		}
		break;
	case P_LABEL_WIDTH:
		if (!MCU_stoi2(data, i1))
		{
			MCeerror->add(EE_OBJECT_MARGINNAN, 0, 0, data);
			return ES_ERROR;
		}
		labelwidth = i1;
		if (labelwidth == 0)
			flags &= ~F_LABEL_WIDTH;
		else
			flags |= F_LABEL_WIDTH;
		break;
	case P_FAMILY:
		if (!MCU_stoi2(data, i1))
		{
			MCeerror->add(EE_BUTTON_FAMILYNAN, 0, 0, data);
			return ES_ERROR;
		}
		family = i1;
		dirty = False;
		break;
	case P_VISITED:
		if (!MCU_matchflags(data, state, CS_VISITED, dirty))
		{
			MCeerror->add(EE_OBJECT_NAB, 0, 0, data);
			return ES_ERROR;
		}
		reseticon();
		break;
	case P_MENU_HISTORY:
		if (!MCU_stoi2(data, i1))
		{
			MCeerror->add(EE_BUTTON_MENUHISTORYNAN, 0, 0, data);
			return ES_ERROR;
		}
		setmenuhistory(i1);
		dirty = False;
		break;
	case P_MENU_LINES:
		if (data != MCnullmcstring)
		{
			if (!MCU_stoi2(data, i1))
			{
				MCeerror->add(EE_BUTTON_MENULINESNAN, 0, 0, data);
				return ES_ERROR;
			}
			menulines = (uint2)i1;
			flags |= F_MENU_LINES;
		}
		else
		{
			flags &= ~F_MENU_LINES;
			menulines = DEFAULT_MENU_LINES;
		}
		freemenu(False);
		dirty = False;
		break;
	case P_MENU_BUTTON:
		if (!MCU_stoi2(data, i1))
		{
			MCeerror->add(EE_BUTTON_MENUBUTTONNAN, 0, 0, data);
			return ES_ERROR;
		}
		menubutton = (uint1)i1;
		dirty = False;
		break;
	case P_MENU_MODE:
		if (entry != NULL)
			deleteentry();
		else
			freemenu(False);
		if (data == MCpulldownstring)
			menumode = WM_PULLDOWN;
		else if (data == MCpopupstring)
			menumode = WM_POPUP;
		else if (data == MCoptionstring)
			menumode = WM_OPTION;
		else if (data == MCcascadestring)
			menumode = WM_CASCADE;
		else if (data == MCcombostring)
		{
			menumode = WM_COMBO;
			createentry();
		}
		else if (data == MCtabstring)
		{
			menumode = WM_TOP_LEVEL;
			if (getstyleint(flags) == F_MENU)
				MCU_break_string(MCString(menustring, menusize), tabs, ntabs, hasunicode());
		}
		else
			menumode = WM_CLOSED;
		break;
	case P_SHOW_BORDER:
		if (MCControl::setprop(parid, p, ep, effective) != ES_NORMAL)
			return ES_ERROR;
		if (MCaqua && menumode == WM_PULLDOWN)
		{
			freemenu(False);
			findmenu(true);
		}
		break;
	case P_MENU_NAME:
		freemenu(False);
		delete menuname;
		if (data != MCnullmcstring)
		{
			menuname = data.clone();
			if (opened)
			{
				if (findmenu(true))
					menu->installaccels(getstack());
			}
		}
		else
			menuname = NULL;
		dirty = False;
		break;
	case P_ACCELERATOR_TEXT:
		delete acceltext;
		acceltext = NULL;
		acceltextsize = 0;
		if (data != MCnullmcstring)
		{
			acceltextsize  = data.getlength();
			acceltext = new char[acceltextsize];
			memcpy(acceltext, data.getstring(), acceltextsize);
		}
		break;
	case P_ACCELERATOR_KEY:
		if (data != MCnullmcstring)
		{
			accelkey = data.getstring()[0];
			if (data.getlength() > 1)
			{
				uint4 t_accelkey = MCLookupAcceleratorKeysym(data);
				if (t_accelkey != 0)
					accelkey = t_accelkey;
			}
		}
		else
			accelkey = 0;
		MCstacks->changeaccelerator(this, accelkey, accelmods);
		dirty = False;
		break;
	case P_ACCELERATOR_MODIFIERS:
		{
			uint2 naccelmods = 0;
			uint4 l = data.getlength();
			const char *sptr = data.getstring();
			MCU_skip_spaces(sptr, l);
			while (l != 0)
			{
				const char *startptr = sptr;
				if (!MCU_strchr(sptr, l, ','))
				{
					sptr += l;
					l = 0;
				}
				MCString tdata(startptr, sptr - startptr);
				MCU_skip_char(sptr, l);
				MCU_skip_spaces(sptr, l);
				if (tdata == MCshiftstring)
				{
					naccelmods |= MS_SHIFT;
					continue;
				}
				if (tdata == MCcommandstring)
				{
					naccelmods |= MS_CONTROL;
					continue;
				}
				if (tdata == MCcontrolstring)
				{
#ifdef _MAC_DESKTOP
					naccelmods |= MS_MAC_CONTROL;
#else
					naccelmods |= MS_CONTROL;
#endif
					continue;
				}
				if (tdata == MCmod1string || tdata == MCoptionstring)
				{
					naccelmods |= MS_MOD1;
					continue;
				}
				MCeerror->add
				(EE_BUTTON_BADMODIFIER, 0, 0, data);
				return ES_ERROR;
			}
			accelmods = (uint1)naccelmods;
			MCstacks->changeaccelerator(this, accelkey, accelmods);
		}
		dirty = False;
		break;
	case P_MNEMONIC:
		if (!MCU_stoi2(data, i1))
		{
			MCeerror->add
			(EE_BUTTON_MNEMONICNAN, 0, 0, data);
			return ES_ERROR;
		}
		clearmnemonic();
		mnemonic = (uint1)i1;
		setupmnemonic();
		break;
	case P_DEFAULT:
	{
		if (!MCU_matchflags(data, flags, F_DEFAULT, dirty))
		{
			MCeerror->add(EE_OBJECT_NAB, 0, 0, data);
			return ES_ERROR;
		}
		if (dirty && opened && ((flags & F_DEFAULT) != 0) != ((state & CS_SHOW_DEFAULT) != 0))
		{
			uint2 t_old_trans;
			t_old_trans = gettransient();
			if (flags & F_DEFAULT)
			{
				getcard()->setdefbutton(this);
				state |= CS_SHOW_DEFAULT;
			}
			else
			{
				getcard()->freedefbutton(this);
				state &= ~CS_SHOW_DEFAULT;
			}
			// MW-2011-08-18: [[ Layers ]] Invalidate the whole object, noting
			//   possible change in transient.
			layer_transientchangedandredrawall(t_old_trans);
			dirty = False;
		}
	}
	break;
	case P_TEXT_FONT:
	case P_TEXT_HEIGHT:
	case P_TEXT_SIZE:
	case P_TEXT_STYLE:
	case P_ENABLED:
	case P_DISABLED:
		if (MCControl::setprop(parid, p, ep, effective) != ES_NORMAL)
			return ES_ERROR;

		// MW-2007-07-05: [[ Bug 1292 ]] Field inside combo-box doesn't respect the button's properties
		if (entry != NULL)
			entry -> setprop(parid, p, ep, effective);

		reseticon();
		freemenu(False);
		findmenu(true);
		if (parent != NULL && parent->gettype() == CT_GROUP)
		{
			parent->setstate(True, CS_NEED_UPDATE);
			if ((parent == MCmenubar || parent == MCdefaultmenubar) && !MClockmenus)
				MCscreen->updatemenubar(True);
		}
		dirty = True;
		break;
	case P_MARGINS:
		// MW-2007-07-05: [[ Bug 1292 ]] We pass the margins through to the combo-box field
		if (MCControl::setprop(parid, p, ep, effective) != ES_NORMAL)
			return ES_ERROR;

		if (entry != NULL)
			entry -> setprop(parid, p, ep, effective);
		break;
	// MW-2012-02-16: [[ IntrinsicUnicode ]] Add support for setting the 
	//   'unicodeText'.
	case P_TEXT:
	case P_UNICODE_TEXT:
		// Ensure that if we are setting unicode, we are unicode; or if we are already
		// unicode, the value we are setting is unicode.
		if (p == P_UNICODE_TEXT && !hasunicode())
			switchunicode(true);
		else if (p == P_TEXT && hasunicode())
			ep.nativetoutf16();

		// If nothing has changed then just reset the label; otherwise change the text.
		if (menustring != NULL && data.getlength() == menusize
		        && memcmp(data.getstring(), menustring, data.getlength()) == 0)
		{
			// Try to coerce everything back to native.
			trytochangetonative();
			dirty = resetlabel();
		}
		else
		{
			freemenu(False);
			delete menustring;

			if (data != MCnullmcstring)
			{
				flags |= F_MENU_STRING;
				menusize = data.getlength();
				menustring = new char[menusize];
				memcpy(menustring, data.getstring(), menusize);
			}
			else
			{
				flags &= ~F_MENU_STRING;
				menustring = NULL;
				menusize = 0;
			}

			// Now that we've updated the text, try to coerce everything back to native.
			trytochangetonative();
			
			if (getflag(F_MENU_STRING))
				findmenu(true);

			menuhistory = 1;
			dirty = all = resetlabel() || menumode == WM_TOP_LEVEL;
			if (parent != NULL && parent->gettype() == CT_GROUP)
			{
				parent->setstate(True, CS_NEED_UPDATE);
				if ((parent == MCmenubar || parent == MCdefaultmenubar) && !MClockmenus)
					MCscreen->updatemenubar(True);
			}
		}
		break;
	case P_CANT_SELECT:
		// MW-2005-08-16: [[Bug 2820]] If we can't be selected, let us make sure our field can't either!
		// MW-2005-09-05: [[Bug 3167]] Only set the entry's property if it exists!
		if (entry != NULL)
			entry -> setprop(parid, p, ep, effective);
		return MCControl::setprop(parid, p, ep, effective);
	default:
		return MCControl::setprop(parid, p, ep, effective);
#endif /* MCButton::setprop */
	}
	if (dirty && opened)
	{
		// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
		layer_redrawall();
	}
	return ES_NORMAL;
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
		if (kfocus && !(state & CS_MFOCUSED))
		{
			menu->setstate(True, CS_KFOCUSED); // override state
			menu->kunfocus();
		}
		MCButton *focused = (MCButton *)menu->getcurcard()->getmfocused();
		if (focused != NULL && focused->gettype() == CT_BUTTON
		        && focused->getmenumode() == WM_CASCADE)
			focused->closemenu(kfocus, disarm);

		menu -> mode_closeasmenu();
		menu->close();

		state &= ~(CS_SUBMENU | CS_MOUSE_UP_MENU);
		menudepth--;
	}
}

MCControl *MCButton::clone(Boolean attach, Object_pos p, bool invisible)
{
	MCButton *newbutton = new MCButton(*this);
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

MCControl *MCButton::findname(Chunk_term type, const MCString &inname)
{
	if ((type == gettype() || type == CT_LAYER
	        || (type == CT_MENU && getstyleint(flags) == F_MENU
	            && menumode == WM_PULLDOWN))
	        && MCU_matchname(inname, gettype(), getname()))
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
	if (!(flags & F_VISIBLE || MCshowinvisibles))
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
		switch(gethilite(newid))
		{
		case True:
			state |= CS_HILITED;
			state &= ~CS_MIXED;
			break;
		case False:
			state &= ~(CS_HILITED | CS_MIXED);
			break;
		case Mixed:
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
			MCCdata *tptr = (MCCdata *)bptr->remove(bptr);
			delete tptr;
		}
	}
	MCControl::resetfontindex(oldstack);
}

void MCButton::activate(Boolean notify, uint2 key)
{
	if (flags & F_DISABLED)
		return;
	if (findmenu(true))
	{
		bool t_disabled;
		MCString pick;
		
		if (menu != NULL)
			menu->findaccel(key, pick, t_disabled);
#ifdef _MAC_DESKTOP
		else if (bMenuID != 0)
			getmacmenuitemtextfromaccelerator(bMenuID, key, MCmodifierstate, pick, hasunicode(), false);
#endif
		if (pick.getstring() == NULL)
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
				message_with_args(MCM_menu_pick, pick);
			delete (char *)pick.getstring();
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
					MCGroup *gptr = (MCGroup *)parent;
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
		message_with_args(MCM_mouse_up, "1");
		MCdispatcher->closemenus();
	}
}

void MCButton::setupmnemonic()
{
	if (opened && mnemonic != 0)
	{
		MCString slabel;
		bool t_is_unicode;
		getlabeltext(slabel, t_is_unicode);
		const char *sptr = slabel.getstring();
		if (!isdisabled() && sptr != NULL && mnemonic <= slabel.getlength())
		{
			getstack()->addmnemonic(this, sptr[mnemonic - 1]);
			if (menustring != NULL || menuname != NULL)
				MCstacks->addmenu(this, sptr[mnemonic - 1]);
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
		foundptr = new MCCdata(cardid);
		foundptr->appendto(bdata);
	}
	return foundptr;
}

uint2 MCButton::getfamily()
{
	return family;
}

Boolean MCButton::gethilite(uint4 parid)
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

Boolean MCButton::sethilite(uint4 parid, Boolean hilite)
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
	t_hilite_changed = hilite != foundptr -> getset();
	
	foundptr->setset(hilite);
	uint4 oldstate = state;
	if (opened && set)
			switch (hilite)
			{
			case True:
				state |= CS_HILITED;
				state &= ~CS_MIXED;
				break;
			case False:
				state &= ~(CS_HILITED | CS_MIXED);
				break;
			case Mixed:
				state &= ~CS_HILITED;
				state |= CS_MIXED;
			}
	
	if (t_hilite_changed)
		signallisteners(P_HILITE);
	
	return state != oldstate;
}

void MCButton::resethilite(uint4 parid, Boolean hilite)
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
	MCExecPoint ep;
	entry->exportasplaintext(0, ep, 0, INT32_MAX, hasunicode());
	delete label;
	labelsize = ep.getsvalue().getlength();
	if (labelsize)
	{
		label = new char[labelsize];
		memcpy(label, ep.getsvalue().getstring(), labelsize);
	}
	else
		label = NULL;
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
		entry->setupentry(this, MCString(label, labelsize), hasunicode());
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
void MCButton::getmenuptrs(const char *&sptr, const char *&eptr)
{
	uint2 i = 0;
	eptr = menustring;
	Boolean isunicode = hasunicode();
	if (menustring == NULL)
		sptr = eptr;
	else
	{
		uint4 l = menusize;
		do
		{
			sptr = eptr;
			if (MCU_strchr(eptr, l, '\n', isunicode) == False)
			{
				if (++i == menuhistory)
					eptr = menustring + menusize;
				else
					sptr = eptr = menustring;
				return;
			}

			l -= MCU_charsize(isunicode);
			if (++i < menuhistory)
				eptr += MCU_charsize(isunicode);
		}
		while (i < menuhistory); // really! eptr++
	}
}

void MCButton::makemenu(sublist *bstack, int2 &stackdepth, uint2 menuflags, MCFontRef fontref)
{
	Boolean isxp = MCcurtheme && MCcurtheme->getthemeid() == LF_NATIVEWIN;
	uint2 pwidth = 0;
	if (stackdepth > 0)
	{
		MCString lastname = bstack[stackdepth].parent->getname_oldstring();
		pwidth = MCFontMeasureText(fontref, lastname . getstring(), lastname . getlength(), false) + 16;
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
		menu = newmenu;
	else
	{
		m->parent->menu = newmenu;
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
		Boolean isunicode = p_menuitem->is_unicode ? True : False;
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
		MCButton *newbutton = new MCButton;
		newbutton->appendto(bstack[stackdepth].buttons);
		if (p_menuitem->tag != NULL)
			newbutton->setname_oldstring(p_menuitem->tag);
		else if (p_menuitem->is_unicode)
		{
				MCExecPoint ep;
				ep.setsvalue(p_menuitem->label);
				ep.utf16tonative();
			newbutton->setname_oldstring(ep.getsvalue());
		}
		else
		{
			newbutton->setname_oldstring(p_menuitem->label);
		}

#ifndef TARGET_PLATFORM_MACOS_X
		const char *t_tabptr = p_menuitem->label.getstring();
		uint4 t_labelsize = p_menuitem->label.getlength();
		// replace any tabs, as these don't display in button menus
		while (MCU_strchr(t_tabptr, t_labelsize, '\t', p_menuitem->is_unicode))
		{
			MCU_copychar(' ', (char*)t_tabptr, p_menuitem->is_unicode);
		}
#endif

		newbutton->menubutton = parent->menubutton;
		newbutton->menucontrol = MENUCONTROL_ITEM;
		if (MCNameGetCharAtIndex(newbutton -> getname(), 0) == '-')
		{
			newbutton->rect.height = 2;
			newbutton->flags = DIVIDER_FLAGS;
			newbutton->menucontrol = MENUCONTROL_SEPARATOR;
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
					uint4 t_key = p_menuitem->accelerator;
					const char *t_keyname = p_menuitem->accelerator_name;

					if ((t_mods & (MS_MAC_CONTROL | MS_CONTROL | MS_ALT)) || t_keyname)
					{
						uint2 t_accelkey = (t_key <= 255) ? MCS_tolower(t_key) : t_key;
						MCstacks->addaccelerator(parent, parent->getstack(), t_accelkey, t_mods);

						uint4 t_acceltextlen;
						if (t_keyname != NULL)
							t_acceltextlen = strlen(t_keyname) + 1;
						else
							t_acceltextlen = 2;

						if (t_mods & MS_MAC_CONTROL)
							t_acceltextlen += 5;
						if (MS_MAC_CONTROL != MS_CONTROL && t_mods & MS_CONTROL)
							t_acceltextlen += 4;
						if (t_mods & MS_ALT)
							t_acceltextlen += 4;
						if (t_mods & MS_SHIFT)
							t_acceltextlen += 6;

						newbutton->acceltext = new char[t_acceltextlen];
						newbutton->accelkey = t_accelkey;
						newbutton->accelmods = t_mods;
						char *t_acceltext = newbutton->acceltext;
						if (t_mods & MS_MAC_CONTROL)
						{
							sprintf(t_acceltext, "Ctrl+");
							t_acceltext += 5;
						}
						if (MS_MAC_CONTROL != MS_CONTROL && t_mods & MS_CONTROL)
						{
							sprintf(t_acceltext, "Cmd+");
							t_acceltext += 4;
						}
						if (t_mods & MS_ALT)
						{
							sprintf(t_acceltext, "Alt+");
							t_acceltext += 4;
						}
						if (t_mods & MS_SHIFT)
						{
							sprintf(t_acceltext, "Shift+");
							t_acceltext += 6;
						}
						if (t_keyname != NULL)
							sprintf(t_acceltext, "%s", t_keyname);
						else
							sprintf(t_acceltext, "%c", t_key);
						newbutton->acceltextsize = strlen(newbutton->acceltext);
						if (p_menuitem->is_unicode)
						{
							int t_unicodebuffersize = newbutton->acceltextsize * MCU_charsize(True);
							char *t_unicodetext = new char[t_unicodebuffersize];
							uint4 tlen;
							MCU_multibytetounicode( newbutton->acceltext , newbutton->acceltextsize , (char *)t_unicodetext, t_unicodebuffersize,  tlen , 0);
							delete newbutton->acceltext;
							newbutton->acceltext = t_unicodetext;
							newbutton->acceltextsize = tlen;
						}
					}
				}
			}
			uint2 t_labellength = p_menuitem->label.getlength();
			uint2 width = MCFontMeasureText(fontref, p_menuitem->label.getstring(), t_labellength, p_menuitem -> is_unicode);
			if (newbutton->acceltext != NULL)
				bstack[stackdepth] . maxaccelwidth = MCU_max(bstack[stackdepth].maxaccelwidth, MCFontMeasureText(fontref, newbutton->acceltext, newbutton->acceltextsize, p_menuitem -> is_unicode));
			if (width > bstack[stackdepth].maxwidth)
				bstack[stackdepth].maxwidth = width;
			newbutton->labelsize = t_labellength;
			newbutton->label = new char[t_labellength];
			memcpy(newbutton->label, p_menuitem->label.getstring(), t_labellength);
			newbutton->flags |= F_LABEL;
			if (p_menuitem -> is_unicode)
				newbutton->m_font_flags |= FF_HAS_UNICODE;

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
	Boolean isunicode = hasunicode();
	if (menuname != NULL)
	{
		if (menu == NULL)
		{
			MCerrorlock++;
			menu = getstack()->findstackname(menuname);
			MCerrorlock--;
			if (menu != NULL)
				menu->addneed(this);
		}
	}
	else if (menustring != NULL && getstyleint(flags) == F_MENU)
	{
		if (menumode == WM_TOP_LEVEL)
			MCU_break_string(MCString(menustring, menusize), tabs, ntabs, isunicode);
		else if (menu == NULL)
		{
			uint2 fheight;
			fheight = gettextheight();
			if ((!IsMacLFAM() || MCModeMakeLocalWindows()) && menumode == WM_COMBO || menumode == WM_OPTION && MClook == LF_WIN95)
			{
				uint2 dsize = menusize;
				uint2 nlines = 1;
				//major menustring
				const char *sptr = menustring;
				const char *eptr = menustring + menusize;
				while (sptr < eptr)
				{
					if (MCU_comparechar(sptr,'\n', isunicode))
						nlines++;
					sptr+= MCU_charsize(isunicode);
				}
				if (sptr > menustring)
				{
					sptr-= MCU_charsize(isunicode);
					if (MCU_comparechar(sptr,'\n', isunicode))
					{
						dsize = sptr - menustring;
						nlines--;
					}
				}
				MCField *fptr = new MCField;
				uint2 height;
				if (nlines > menulines)
				{
					height = menulines * fheight;
					fptr->setupmenu(MCString(menustring, dsize), fheight, True, isunicode);
					state |= CS_SCROLLBAR;
				}
				else
				{
					height = nlines * fheight;
					fptr->setupmenu(MCString(menustring, dsize), fheight, False, isunicode);
				}
				MCRectangle trect;
				MCU_set_rect(trect, 0, 0, rect.width, height + 4);
				trect = MCU_reduce_rect(trect, MClook == LF_MOTIF ? DEFAULT_BORDER : 1);
				fptr->setrect(trect);
				/* UNCHECKED */ MCStackSecurityCreateStack(menu);

				menu->setparent(this);
				menu->createmenu(fptr, rect.width, height + 4);
				MCdispatcher->appendpanel(menu);
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
					MCU_break_string(MCString(menustring,menusize), tabs, ntabs, isunicode);
					uint2 i;
					uint4 menuflags = MENU_ITEM_FLAGS;
					if (flags & F_DISABLED)
						menuflags |= F_DISABLED;
					for (i = 0 ; i < ntabs ; i++)
					{
						if (MCU_comparechar(tabs[i].getstring(),'!',isunicode)
						        || MCU_comparechar(tabs[i].getstring(),'(',isunicode)
						        && MCU_comparechar(&tabs[i].getstring()[MCU_charsize(isunicode)],'!',isunicode))
						{
							menuflags &= ~F_STYLE;
							menuflags |= F_CHECK;
							break;
						}
					}

					ButtonMenuCallback t_callback(this, menuflags);
					MCString t_menustring(menustring, menusize);
					MCParseMenuString(t_menustring, &t_callback, isunicode == True, menumode);

				}
			}
			delete tabs;
			tabs = NULL;
			ntabs = 0;
		}
	}
	return menu != NULL;
}

bool MCSystemPick(const char *p_options, bool p_is_unicode, bool p_use_checkmark, uint32_t p_initial_index, uint32_t& r_chosen_index, MCRectangle p_button_rect);

void MCButton::openmenu(Boolean grab)
{
	if (!opened || MCmousestackptr == NULL)
		return;
	if (menuname != NULL && !MCModeMakeLocalWindows())
		return;
#ifdef _MOBILE
	if (menumode == WM_OPTION)
	{
		// loop counter
		int i;
		// result of picker action
		long t_result;
		// item selection
		uint32_t t_selected, t_chosen_option;
		// temporary options string from which to select the new label
		MCString t_menustringcopy;
		// the selected item
		// get a pointer to this 
		MCButton *pptr;
		pptr = this;
		// get the label and menu item strings
		MCString t_menustring;
		pptr->getmenustring(t_menustring);
		// test if we are processing unicode
		bool t_is_unicode = hasunicode();
		MCExecPoint ep(nil, nil, nil);
		if (t_is_unicode)
		{
			// convert unicode to binary 
			ep . setsvalue(t_menustring);
			ep . utf16toutf8();
			t_menustring = ep . getsvalue();
		}
		
		// MW-2012-10-08: [[ Bug 10254 ]] MCSystemPick expects a C-string so make sure we give it one.
		char *t_menustring_cstring;
		t_menustring_cstring = t_menustring . clone();
		
		// process data using the pick wheel
		t_selected = menuhistory;
		t_result = MCSystemPick(t_menustring_cstring, t_is_unicode, false, t_selected, t_chosen_option, rect);
		
		free(t_menustring_cstring);
		
		// populate the label with the value returned from the pick wheel if the user changed the selection
		if (t_result && t_chosen_option > 0)
		{
			setmenuhistoryprop(t_chosen_option);
			t_menustringcopy = t_menustring.clone();
			char *t_newlabel;
			t_newlabel = strtok((char *)t_menustringcopy.getstring(), "\n");
			for (i = 1; i < t_chosen_option; i++)
				t_newlabel = strtok(NULL, "\n");
			// ensure processing takes care of unicode
			ep . setsvalue(t_newlabel);
			if (t_is_unicode)
				ep . utf8toutf16();
			// update the label text
			delete label;
			labelsize = ep . getsvalue() . getlength();
			label = new char[labelsize];
			memcpy(label, ep . getsvalue() . getstring(), labelsize);
			flags |= F_LABEL;
			message_with_args(MCM_menu_pick, ep . getsvalue());
		}
		return;
	}
#endif
	MCStack *sptr = menumode == WM_CASCADE ? getstack() : MCmousestackptr;
	if (flags & F_TRAVERSAL_ON && !(state & CS_KFOCUSED)
	        && sptr->getmode() < WM_PULLDOWN)
	{
		MCmousestackptr->kfocusset(this);
		if (menu == NULL && !findmenu())
			return;
	}
	if (IsMacLFAM() &&
		(!MCModeMakeLocalWindows() ||
		 (menuname == NULL && (menumode == WM_OPTION || menumode == WM_POPUP || (menumode == WM_PULLDOWN && flags & F_SHOW_BORDER)))))
	{
#ifdef _MAC_DESKTOP
		macopenmenu();
#endif
	}
	else
	{
		state |= CS_SUBMENU | CS_ARMED;
		reseticon();
		if (MCmenuobjectptr == NULL)
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
		menu -> mode_openasmenu(t_did_grab ? sptr : NULL);
		
		if (menumode == WM_OPTION)
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
	if (menu != NULL && !(state & CS_SUBMENU))
		if (menuname != NULL)
		{
			menu->removeaccels(getstack());
			menu->removeneed(this);
			menu = NULL;
		}
		else
			if (menustring != NULL || force)
			{
				closemenu(False, True);
				MCdispatcher->removepanel(menu);
				MCstacks->deleteaccelerator(this, NULL);
				menu->removeneed(this);
				delete menu;
				menu = NULL;
			}
}

void MCButton::docascade(MCString &pick)
{
	MCButton *pptr = this;
	if (menuname == NULL && menustring == NULL)
	{
		//get tag info
		bool t_has_tags = getmenuhastags();

		pptr = this;
		while(pptr->menumode == WM_CASCADE && pptr->parent->getparent()->getparent()->gettype() == CT_BUTTON)
		{
			bool isunicode;
			MCString slabel;

			if (t_has_tags && pptr -> getname() != nil)
				slabel = pptr -> getname_oldstring(), isunicode = false;
			else
				pptr->getlabeltext(slabel, isunicode);

			uint2 newpicksize = 0;
			char *newpick = new char[slabel.getlength() + MCU_charsize(isunicode) + pick.getlength()];
			memcpy(newpick, slabel.getstring(), slabel.getlength());
			newpicksize += slabel.getlength();
			
			MCU_copychar('|', &newpick[newpicksize], isunicode);

			newpicksize += MCU_charsize(isunicode);
			memcpy(&newpick[newpicksize],pick.getstring(),pick.getlength());
			newpicksize += pick.getlength();
			delete (char *)pick.getstring();
			pick.set(newpick,newpicksize);
			pptr = (MCButton *)pptr->parent->getparent()->getparent();
			pptr->state |= CS_IGNORE_MENU;
		}
	}
	if (pptr != this)
	{
		MCParameter *param = new MCParameter;
		param->setbuffer((char *)pick.getstring(), pick.getlength());
		MCscreen->addmessage(pptr, MCM_menu_pick, MCS_time(), param);
		pick = NULL;
	}
	else
	{
		Exec_stat es = pptr->message_with_args(MCM_menu_pick, pick);
		if (es == ES_NOT_HANDLED || es == ES_PASS)
			pptr->message_with_args(MCM_mouse_up, menubutton);
	}
}

void MCButton::setupmenu()
{
	flags = MENU_FLAGS;
}

void MCButton::selectedchunk(MCExecPoint &ep)
{
	getprop(0, P_NUMBER, ep, False);
	ep.ton();
	uint4 number = ep.getuint4();
	const char *sptr;
	const char *eptr;
	getmenuptrs(sptr, eptr);
	ep.setstringf("char %d to %d of button %d", sptr - menustring + 1, eptr - menustring, number);
}

void MCButton::selectedline(MCExecPoint &ep)
{
	getprop(0, P_NUMBER, ep, False);
	uint2 number;
	ep.getuint2(number, 0, 0, EE_UNDEFINED);
	ep.setstringf("line %d of button %d", menuhistory, number);
}

void MCButton::selectedtext(MCExecPoint &ep)
{
	if (entry != NULL)
	{
		MCString slabel;
		bool isunicode;
		getlabeltext(slabel, isunicode);
		ep.setsvalue(slabel);
	}
	else
	{
		const char *sptr;
		const char *eptr;
		getmenuptrs(sptr, eptr);
		MCString s(sptr, eptr - sptr);
		ep.setsvalue(s);
	}
}

// MW-2012-02-16: [[ IntrinsicUnicode ]] The encoding of the label text may not
//   be the same as the 'hasunicode()' setting as it could be the name of the
//   control (which, at the moment, is always native).
void MCButton::getlabeltext(MCString &s, bool& r_unicode)
{
	if (entry != NULL)
		getentrytext();
	if (flags & F_LABEL)
	{
		s.set(label, labelsize);
		r_unicode = hasunicode();
	}
	else
	{
		s = getname_oldstring();
		r_unicode = false;
	}
}

Boolean MCButton::resetlabel()
{
	Boolean changed = False;
	if (menumode == WM_OPTION || menumode == WM_COMBO)
	{
		char *oldlabel = label;
		uint2 oldlabelsize = labelsize;
		if (menustring == NULL)
		{
			label = NULL;
			labelsize = 0;
			if (entry != NULL)
				entry->settext(0, MCnullmcstring, False, hasunicode());

			flags &= ~F_LABEL;

			if (oldlabel != NULL)
				changed = True;
		}
		else
		{
			const char *sptr;
			const char *eptr;
			getmenuptrs(sptr, eptr);
			labelsize = eptr - sptr;
			if (labelsize == 0)
			{
				setmenuhistoryprop(1);
				label = NULL;
			}
			else
			{
				label = new char[labelsize];
				memcpy(label, sptr, labelsize);
			}
			if (entry != NULL)
				entry->settext(0, MCString(label, labelsize), False, hasunicode());

			flags |= F_LABEL;

			if (oldlabel == NULL || labelsize != oldlabelsize ||
			        memcmp(label, oldlabel, labelsize) != 0)
				changed = True;
		}
		delete oldlabel;
	}
	else
		if (menumode == WM_TOP_LEVEL)
			MCU_break_string(MCString(menustring, menusize), tabs, ntabs, hasunicode());
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
		char string[U2L];
		sprintf(string, "%d", i);
		MCButton *bptr = (MCButton *)cptr->getchild(CT_EXPRESSION, string,
		                 CT_BUTTON, ptype);
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
		MCCStringArrayFree(m_tags, m_tag_count);
	}
	
	bool Start(void)
	{
		return false;
	}
	
	bool ProcessItem(MCMenuItem *p_menu_item)
	{
		if (m_done)
			return true;
		
		char *t_tag;
		t_tag = nil;
		
		MCExecPoint ep;
		if (p_menu_item -> tag . getstring() != nil)
			ep . setsvalue(p_menu_item -> tag);
		else
		{
			ep . setsvalue(p_menu_item -> label);
			if (p_menu_item -> is_unicode)
				ep . utf16toutf8();
		}
		
		MCCStringClone(ep . getcstring(), t_tag);

		for(uint32_t i = p_menu_item -> depth; i < m_tag_count; i++)
			MCCStringFree(m_tags[i]);
		
		MCMemoryResizeArray(p_menu_item -> depth + 1, m_tags, m_tag_count);
		m_tags[p_menu_item -> depth] = t_tag;
		
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
	
	char *GetPickString(void)
	{
		char *t_which;
		MCCStringClone(m_tags[0], t_which);
		for(uint32_t i = 1; i < m_tag_count; i++)
			MCCStringAppendFormat(t_which, "|%s", m_tags[i]);
		return t_which;
	}
	
	uint32_t GetPickIndex(void)
	{
		return m_current_index;
	}
	
private:
	bool m_done;
	uint32_t m_current_index;
	uint32_t m_target_index;
	char **m_tags;
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
	if (menustring == nil)
		return;
	if (menumode == WM_CASCADE || menumode == WM_POPUP || menumode == WM_PULLDOWN)
	{
		MCMenuPickBuilder t_builder(newline);
		MCString t_menu_string(menustring, menusize);
		MCParseMenuString(t_menu_string, &t_builder, hasunicode(), menumode);
		
		setmenuhistoryprop(t_builder . GetPickIndex());
		
		char *t_which;
		t_which = t_builder . GetPickString();
		message_with_args(MCM_menu_pick, t_which);
		
		resetlabel();
		
		MCCStringFree(t_which);
	}
	else
	{
		if (!(getstyleint(flags) == F_MENU && menumode == WM_TOP_LEVEL) || !opened)
			MCU_break_string(MCString(menustring, menusize), tabs, ntabs, hasunicode());
		uint2 oldline = menuhistory;
		setmenuhistoryprop(MCU_max(MCU_min(newline, ntabs), 1));
		if (menuhistory != oldline && !(state & CS_MFOCUSED) && tabs != NULL)
			message_with_args(MCM_menu_pick, tabs[menuhistory - 1], tabs[oldline - 1]);
		resetlabel();
		if (!(getstyleint(flags) == F_MENU && menumode == WM_TOP_LEVEL) || !opened)
		{
			delete tabs;
			tabs = NULL;
			ntabs = 0;
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
		for (i = 0 ; i < ntabs ; i++)
		{
			const char *sptr = tabs[i].getstring();
			uint2 length = tabs[i].getlength();
			if (*sptr == '(')
			{
				sptr++;
				length--;
			}
			totalwidth += MCFontMeasureText(m_font, sptr, length, hasunicode()) + 23;
		}
		if (totalwidth < rect.width)
			curx += rect.width - totalwidth >> 1;
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
	for (i = 0 ; i < ntabs ; i++)
	{
		if (MCcurtheme)
			tx += MCFontMeasureText(m_font, tabs[i].getstring(), tabs[i].getlength(), hasunicode()) + tabrightmargin + tableftmargin - taboverlap;
		else
		{
			if (IsMacLF())
				tx += MCFontMeasureText(m_font, tabs[i].getstring(), tabs[i].getlength(), hasunicode()) + theight * 2 / 3 + 7;
			else
				tx += MCFontMeasureText(m_font, tabs[i].getstring(), tabs[i].getlength(), hasunicode()) + 12;

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
	for (uint4 i = 0 ; i < ntabs ; i++)
	{
		if (MCcurtheme)
			tx += MCFontMeasureText(m_font, tabs[i].getstring(), tabs[i].getlength(), hasunicode()) + tabrightmargin + tableftmargin - taboverlap;
		else
		{
			if (IsMacLF())
				tx += MCFontMeasureText(m_font, tabs[i].getstring(), tabs[i].getlength(), hasunicode()) + theight * 2 / 3 + 7;
			else
				tx += MCFontMeasureText(m_font, tabs[i].getstring(), tabs[i].getlength(), hasunicode()) + 12;

		}
	}
	if (ntabs > 0)
		tx += taboverlap + tabrightmargin;
	return tx;
}

#include "icondata.cpp"

static void openicon(MCImage *&icon, uint1 *data, uint4 size)
{
	// MW-2012-02-17: [[ FontRefs ]] Make sure we set a parent on the icon, and also
	//   make it invisible. If we don't do this we get issues with parent references
	//   and fontrefs.
	icon = new MCImage;
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

// MW-2012-02-16: [[ IntrinsicUnicode ]] This utility method changes the encoding
//   of the given string either to unicode (if to_unicode is true) or to native
//   otherwise.
static void switchunicodeofstring(bool p_to_unicode, char*& x_string, uint2& x_length)
{
	if (x_string == nil)
		return;

	MCExecPoint ep(nil, nil, nil);
	ep . setsvalue(MCString(x_string, x_length));
	if (p_to_unicode)
		ep . nativetoutf16();
	else
		ep . utf16tonative();
	
	delete x_string;

	x_length = ep . getsvalue() . getlength();
	x_string = new char[x_length];
	memcpy(x_string, ep . getsvalue() . getstring(), x_length);
}

// MW-2012-02-16: [[ IntrinsicUnicode ]] This method switches all the text in
//   the button to or from unicode (to unicode if 'to_unicode' is set).
void MCButton::switchunicode(bool p_to_unicode)
{
	if (hasunicode() == p_to_unicode)
		return;

	switchunicodeofstring(p_to_unicode, menustring, menusize);
	switchunicodeofstring(p_to_unicode, label, labelsize);
	switchunicodeofstring(p_to_unicode, acceltext, acceltextsize);
	
	if (p_to_unicode)
		m_font_flags |= FF_HAS_UNICODE;
	else
		m_font_flags &= ~FF_HAS_UNICODE;
}

// MW-2012-02-16: [[ IntrinsicUnicode ]] This method returns 'true' if its possible
//   to convert the given string to native without loss of information.
static bool canconverttonative(const char *p_chars, uint32_t p_char_count)
{
	MCExecPoint ep(nil, nil, nil);
	ep . setsvalue(MCString(p_chars, p_char_count));
	ep . utf16tonative();
	ep . nativetoutf16();
	return ep . getsvalue() . getlength() == p_char_count &&
				memcmp(ep . getsvalue() . getstring(), p_chars, p_char_count) == 0;
}

// MW-2012-02-16: [[ IntrinsicUnicode ]] This method attempts to coerce the text in
//   button to native, but only if no information is lost as a result.
void MCButton::trytochangetonative(void)
{
	if (!hasunicode())
		return;

	if (canconverttonative(menustring, menusize) &&
		canconverttonative(label, labelsize) &&
		canconverttonative(acceltext, acceltextsize))
		switchunicode(false);
}

///////////////////////////////////////////////////////////////////////////////
//
//  SAVING AND LOADING
//

IO_stat MCButton::extendedsave(MCObjectOutputStream& p_stream, uint4 p_part)
{
	// Extended data area for a button consists of:
	//   uint4 hover_icon;
	//   tag extension_block (null for now)
	//   MCObject::extensions

	IO_stat t_stat;
	t_stat = p_stream . WriteU32(icons == NULL ? 0 : icons -> iconids[CI_HOVER]);
	if (t_stat == IO_NORMAL)
		t_stat = defaultextendedsave(p_stream, p_part);

	return t_stat;
}

IO_stat MCButton::extendedload(MCObjectInputStream& p_stream, const char *p_version, uint4 p_remaining)
{
	IO_stat t_stat;
	t_stat = IO_NORMAL;

	if (p_remaining >= 4)
	{
		uint4 t_hover_icon_id;
		t_stat = p_stream . ReadU32(t_hover_icon_id);
		if (t_stat == IO_NORMAL)
		{
			icons = new iconlist;
			memset(icons, 0, sizeof(iconlist));
			icons -> iconids[CI_HOVER] = t_hover_icon_id;
		}

		p_remaining -= 4;
	}

	if (t_stat == IO_NORMAL && p_remaining > 0)
		t_stat = defaultextendedload(p_stream, p_version, p_remaining);

	return t_stat;
}

IO_stat MCButton::save(IO_handle stream, uint4 p_part, bool p_force_ext)
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
	if ((stat = MCObject::save(stream, p_part, t_has_extension || p_force_ext)) != IO_NORMAL)
		return stat;

	if (flags & F_HAS_ICONS)
	{
		uint2 i;
		for (i = CI_ARMED ; i < CI_FILE_NICONS ; i++)
			if ((stat = IO_write_uint4(icons->iconids[i], stream)) != IO_NORMAL)
				return stat;
	}
	if (flags & F_LABEL)
		if ((stat = IO_write_string(label, labelsize, stream, hasunicode())) != IO_NORMAL)
			return stat;
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
	if ((stat = IO_write_string(menuname, stream)) != IO_NORMAL)
		return stat;
	if (flags & F_MENU_STRING)
		if ((stat = IO_write_string(menustring, menusize, stream, hasunicode())) != IO_NORMAL)
			return stat;
	menubutton |= family << 4;
	if ((stat = IO_write_uint1(menubutton, stream)) != IO_NORMAL)
		return stat;
	menubutton &= 0x0F;
	
	// WM_SHEET was added after the fileformat menumode enum was fixed
	// since it wasn't added to the end, we must adjust.
	if ((stat = IO_write_uint1(menumode > WM_MODAL ? menumode - 1 : menumode, stream)) != IO_NORMAL)
		return stat;
	if ((menumode == WM_OPTION || menumode == WM_TOP_LEVEL) && (menuname != NULL || flags & F_MENU_STRING))
		if ((stat = IO_write_uint2(menuhistory, stream)) != IO_NORMAL)
			return stat;
	if (flags & F_MENU_LINES)
		if ((stat = IO_write_uint2(menulines, stream)) != IO_NORMAL)
			return stat;

	if ((stat = IO_write_string(acceltext, acceltextsize, stream, hasunicode())) != IO_NORMAL)
		return stat;


	if ((stat = IO_write_uint2(accelkey, stream)) != IO_NORMAL)
		return stat;
	if ((stat = IO_write_uint1(accelmods, stream)) != IO_NORMAL)
		return stat;
	if ((stat = IO_write_uint1(mnemonic, stream)) != IO_NORMAL)
		return stat;

	if ((stat = savepropsets(stream)) != IO_NORMAL)
		return stat;

	MCCdata *tptr = bdata;
	if (tptr != NULL && (p_part == 0 || !getflag(F_SHARED_HILITE)))
	{
		do
		{
			if ((stat = tptr->save(stream, OT_BDATA, p_part)) != IO_NORMAL)
				return stat;
			tptr = (MCCdata *)tptr->next();
		}
		while (tptr != bdata);
	}
	return IO_NORMAL;
}

IO_stat MCButton::load(IO_handle stream, const char *version)
{
	IO_stat stat;

	if ((stat = MCControl::load(stream, version)) != IO_NORMAL)
		return stat;

	// MW-2012-02-17: [[ IntrinsicUnicode ]] If the unicode tag is set, then we are unicode.
	if ((m_font_flags & FF_HAS_UNICODE_TAG) != 0)
		m_font_flags |= FF_HAS_UNICODE;

	if (strncmp(version, "2.3", 3) <= 0)
	{
		uint4 iconid;
		uint4 hiliteiconid = 0;
		if ((stat = IO_read_uint4(&iconid, stream)) != IO_NORMAL)
			return stat;
		if (flags & F_HAS_ICONS)
			if ((stat = IO_read_uint4(&hiliteiconid, stream)) != IO_NORMAL)
				return stat;
		if (iconid != 0 || hiliteiconid != 0)
		{
			flags |= F_HAS_ICONS;
			icons = new iconlist;
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
				icons = new iconlist;
				memset(icons, 0, sizeof(iconlist));
			}

			icons->curicon = NULL;
			uint2 i;
			for (i = CI_ARMED ; i < CI_FILE_NICONS ; i++)
				if ((stat = IO_read_uint4(&icons->iconids[i], stream)) != IO_NORMAL)
					return stat;
		}

	if (flags & F_LABEL)
	{
		uint4 tlabelsize;
		if ((stat = IO_read_string(label, tlabelsize, stream, hasunicode())) != IO_NORMAL)
			return stat;
		labelsize = tlabelsize;
	}

	if (flags & F_LABEL_WIDTH)
		if ((stat = IO_read_uint2(&labelwidth, stream)) != IO_NORMAL)
			return stat;

	if (!(flags & F_NO_MARGINS))
	{
		if ((stat = IO_read_int2(&leftmargin, stream)) != IO_NORMAL)
			return stat;
		if ((stat = IO_read_int2(&rightmargin, stream)) != IO_NORMAL)
			return stat;
		if ((stat = IO_read_int2(&topmargin, stream)) != IO_NORMAL)
			return stat;
		if ((stat = IO_read_int2(&bottommargin, stream)) != IO_NORMAL)
			return stat;
		if (leftmargin == defaultmargin
		        && leftmargin == rightmargin
		        && leftmargin == topmargin
		        && leftmargin == bottommargin)
			flags |= F_NO_MARGINS;
	}
	if ((stat = IO_read_string(menuname, stream)) != IO_NORMAL)
		return stat;
	if (flags &  F_MENU_STRING)
	{
		uint4 tmenusize;
		if ((stat = IO_read_string(menustring, tmenusize, stream, hasunicode())) != IO_NORMAL)
			return stat;
		menusize = tmenusize;
	}

	if ((stat = IO_read_uint1(&menubutton, stream)) != IO_NORMAL)
		return stat;
	family = menubutton >> 4;
	menubutton &= 0x0F;

	if ((stat = IO_read_uint1(&menumode, stream)) != IO_NORMAL)
		return stat;

	if (menumode > WM_MODAL)
		menumode++;
	if ((menumode == WM_OPTION || menumode == WM_TOP_LEVEL)
	        && (menuname != NULL || flags & F_MENU_STRING))
		if ((stat = IO_read_uint2(&menuhistory, stream)) != IO_NORMAL)
			return stat;

	if (flags & F_MENU_LINES)
		if ((stat = IO_read_uint2(&menulines, stream)) != IO_NORMAL)
			return stat;

	uint4 tacceltextsize;
	if ((stat = IO_read_string(acceltext, tacceltextsize, stream, hasunicode())) != IO_NORMAL)
		return stat;

	acceltextsize = tacceltextsize;

	if ((stat = IO_read_uint2(&accelkey, stream)) != IO_NORMAL)
		return stat;
	if (accelkey < 256)
#ifdef __MACROMAN__
		accelkey = MCisotranslations[accelkey];
#else
		accelkey = MCmactranslations[accelkey];
#endif

	if ((stat = IO_read_uint1(&accelmods, stream)) != IO_NORMAL)
		return stat;
	if ((stat = IO_read_uint1(&mnemonic, stream)) != IO_NORMAL)
		return stat;
	if (strncmp(version, "2.0", 3) <= 0)
	{
		if (flags & F_DEFAULT)
			rect = MCU_reduce_rect(rect, MOTIF_DEFAULT_WIDTH);
		else
			if (flags & F_TRAVERSAL_ON && !(flags & F_AUTO_ARM))
				rect = MCU_reduce_rect(rect, MCfocuswidth);
		if (menuname != NULL)
		{
			if (menumode != WM_CASCADE)
				flags &= ~F_AUTO_ARM;
			flags = flags & ~F_STYLE | F_MENU | F_OPAQUE;
		}
		if (flags & F_AUTO_ARM)
			flags |= F_OPAQUE | F_TRAVERSAL_ON;
	}

	if ((stat = loadpropsets(stream)) != IO_NORMAL)
		return stat;

	while (True)
	{
		uint1 type;
		if ((stat = IO_read_uint1(&type, stream)) != IO_NORMAL)
			return stat;
		if (type == OT_BDATA)
		{
			MCCdata *newbdata = new MCCdata;
			if ((stat = newbdata->load(stream, this, version)) != IO_NORMAL)
			{
				delete newbdata;
				return stat;
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
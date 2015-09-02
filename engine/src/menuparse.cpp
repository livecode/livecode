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

#include "execpt.h"
#include "param.h"
#include "util.h"
#include "stack.h"
#include "card.h"
#include "stacklst.h"
#include "sellst.h"
#include "undolst.h"
#include "aclip.h"
#include "image.h"
#include "field.h"
#include "mcerror.h"

#include "globals.h"

#include "menuparse.h"

bool ParseMenuItemString(char *p_string, uint4 p_strlen, MCMenuItem *p_menuitem);
bool ParseMenuItemTabs(char *p_string, char *&r_endptr, MCMenuItem *p_menuitem);
bool ParseMenuItemSwitches(char *p_string, char *&r_endstr, MCMenuItem *p_menuitem);
bool ParseMenuItemLabel(char *p_string, char *&r_endstr, MCMenuItem *p_menuitem);
bool ParseMenuItemAccelerator(char *p_string, char *&r_endstr, MCMenuItem *p_menuitem);
bool IsEscapeChar(char *p_string, uint4 p_strlen, uint1 p_menumode);

static Keynames accelerator_keys[] =
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

uint4 MCLookupAcceleratorKeysym(MCString &p_name)
{
	for (int i = 0; accelerator_keys[i].keysym != 0; i++)
	{
		if (p_name == accelerator_keys[i].name)
		{
			return accelerator_keys[i].keysym;
		}
	}
	return 0;
}

const char *MCLookupAcceleratorName(uint4 p_keysym)
{
	for (int i = 0; accelerator_keys[i].keysym != 0; i++)
	{
		if (p_keysym == accelerator_keys[i].keysym)
		{
			return accelerator_keys[i].name;
		}
	}
	return NULL;
}

bool MCParseMenuString(MCString &r_string, IParseMenuCallback *p_callback, bool p_is_unicode, uint1 p_menumode)
{
	MCString *t_lines = NULL;
	uint2 t_nlines = 0;
	MCMenuItem t_menuitem;
	MCU_break_string(r_string, t_lines, t_nlines, p_is_unicode);
	MCExecPoint ep;
	bool t_hastags = false;
	
	p_callback->Start();
	
	for (int i=0; i<t_nlines; i++)
	{
		memset(&t_menuitem, 0, sizeof(MCMenuItem));
		t_menuitem.is_unicode = p_is_unicode;
		t_menuitem.menumode = p_menumode;

		char *t_string;
		uint4 t_strlen;
		if (p_is_unicode)
		{
			ep.setsvalue(t_lines[i]);
			ep.utf16toutf8();
			t_string = ep.getsvalue().clone();
			t_strlen = ep.getsvalue().getlength();
		}
		else
		{
			t_string = t_lines[i].clone();
			t_strlen = t_lines[i].getlength();
		}
		
		ParseMenuItemString(t_string, t_strlen, &t_menuitem);
		
		// MW-2013-12-18: [[ Bug 11605 ]] If the tag is empty, and the label can convert
		//   to native then take that to be the tag.
		if (t_menuitem . tag . getlength() == 0)
		{
			ep . setsvalue(t_menuitem . label);
            // MW-2014-07-29: [[ Bug 13007 ]] Only go UTF8->UTF16->Native if is unicode.
            if (p_is_unicode)
                ep . utf8toutf16();
			if (!p_is_unicode || ep . trytoconvertutf16tonative())
			{
				delete t_menuitem . tag . getstring();
				t_menuitem . tag = ep . getsvalue() . clone();
			}
		}
		
		if (p_is_unicode)
		{
			ep.setsvalue(t_menuitem.label);
			ep.utf8toutf16();
			t_menuitem.label.set(ep.getsvalue().clone(), ep.getsvalue().getlength());
			
			// MW-2014-01-06: [[ Bug 11605 ]] If there is no tag, and the label can
			//   be converted to native non-lossily, then use that as the tag.
			if (t_menuitem . tag == nil &&
				ep . trytoconvertutf16tonative())
				t_menuitem.tag.set(ep.getsvalue().clone(), ep.getsvalue().getlength());
		}
				
		p_callback->ProcessItem(&t_menuitem);

		delete t_string;

		if (t_menuitem.tag != NULL)
		{
			delete t_menuitem.tag.getstring();
			t_hastags = true;
		}
		if (p_is_unicode)
			delete t_menuitem.label.getstring();
	}
	
	p_callback->End(t_hastags);

	delete t_lines;
	return false;
}

bool ParseMenuItemString(char *p_string, uint4 p_strlen, MCMenuItem *p_menuitem)
{
	char *t_endptr = p_string + p_strlen;
	char *t_strptr = p_string;

	ParseMenuItemSwitches(p_string, t_endptr, p_menuitem);
	ParseMenuItemLabel(p_string, t_endptr, p_menuitem);
	return false;
}

bool ParseMenuItemSwitches(char *p_string, char *&r_endstr, MCMenuItem *p_menuitem)
{
	while (p_string < r_endstr)
	{
		switch (p_string[0])
		{
		case '(':
			if (p_menuitem->is_disabled)
				return false;
			memmove(p_string, p_string + 1, (r_endstr - p_string) - 1);
			r_endstr--;
			if (r_endstr - p_string > 0 && p_string[0] == '(')
				p_string++;
			else
				p_menuitem->is_disabled = true;
			break;
		case '!':
			if (p_menuitem->is_hilited || p_menuitem->is_radio || p_menuitem->menumode == WM_OPTION)
				return false;
			if (r_endstr - p_string == 1)
				return false;
			else
			{
				if (p_string[1] == '!')
				{
					memmove(p_string, p_string + 1, (r_endstr - p_string) - 1);
					p_string++;
					r_endstr--;
				}
				else
				{
					if (p_string[1] == 'c')
						p_menuitem->is_hilited = true;
					else if (p_string[1] != 'n')
					{
						p_menuitem->is_radio = true;
						if (p_string[1] == 'r')
							p_menuitem->is_hilited = true;
					}
					memmove(p_string, p_string + 2, (r_endstr - p_string) - 2);
					r_endstr -= 2;
				}
			}
			break;
		case '\t':
			if (p_menuitem->depth == 0 && p_menuitem->menumode != WM_OPTION && p_menuitem->menumode != WM_COMBO)
				ParseMenuItemTabs(p_string, r_endstr, p_menuitem);
			else
				p_string++;
			break;
		default:
			return false;
		}
	}
	return false;
}

bool ParseMenuItemLabel(char *p_string, char *&r_endstr, MCMenuItem *p_menuitem)
{
	char *t_str = p_string;
	while (t_str < r_endstr)
	{
		if (IsEscapeChar(t_str, r_endstr - t_str, p_menuitem->menumode))
		{
			memmove(t_str, t_str + 1, (r_endstr - t_str) - 1);
			r_endstr--;
			t_str++;
		}
		else if (p_menuitem->menumode != WM_OPTION && t_str[0] == '&' && p_menuitem->mnemonic == 0 && (r_endstr - t_str) > 1)
		{
			p_menuitem->mnemonic = (t_str - p_string) + 1;
			memmove(t_str, t_str + 1, (r_endstr - t_str) - 1);
			r_endstr--;
		}
		else if (p_menuitem->menumode != WM_OPTION && t_str[0] == '/')
		{
			t_str[0] = '\0';
			p_menuitem->label.set(p_string, t_str - p_string);
			t_str++;
			ParseMenuItemAccelerator(t_str, r_endstr, p_menuitem);
			return false;
		}
		else
			t_str++;

	}
	p_menuitem->label.set(p_string, t_str - p_string);
	return false;
}

bool IsEscapeChar(char *p_string, uint4 p_strlen, uint1 p_menumode)
{
	if (p_strlen < 2)
		return false;

	switch (p_string[0])
	{
	case '/':
	case '!':
	case '&':
		if (p_menumode == WM_OPTION)
			return false;
	case '(':
		return p_string[0] == p_string[1];
		break;
	case '\\':
		return true;
	}

	return false;
}

bool ParseMenuItemAccelerator(char *p_string, char *&r_endstr, MCMenuItem *p_menuitem)
{
	uint1 t_mods = 0;
	uint4 t_key = 0;
	const char *t_keyname = NULL;

	r_endstr[0] = '\0';

	char *t_tag = strrchr(p_string, '|');
	if (t_tag != NULL && r_endstr - t_tag > 1)
	{
		p_menuitem->tag = strclone(t_tag + 1);
		r_endstr = t_tag;
		r_endstr[0] = '\0';
	}

	if (p_menuitem->menumode == WM_PULLDOWN || p_menuitem->menumode == WM_TOP_LEVEL)
	{
		uint4 t_accelstrlen = r_endstr - p_string;
		if (t_accelstrlen == 1)
			t_mods |= MS_CONTROL;
			
		while (r_endstr - p_string > 1)
		{
			if (p_string[0] == ' ')
			{
				p_string++;
			}
			else
			{
				uint4 i = 0;
				bool t_token_found = false;
				while (!t_token_found && modifier_tokens[i].token != NULL)
				{
					if (modifier_tokens[i].token_length == 1)
					{
						if (p_string[0] == modifier_tokens[i].token[0])
						{
							if (!(t_mods & modifier_tokens[i].modifier))
							{
								t_mods |= modifier_tokens[i].modifier;
								p_string++;
								t_token_found = true; 
							}
							break;
						}
					}
					else if (modifier_tokens[i].token_length < r_endstr - p_string && (p_string[modifier_tokens[i].token_length] == ' '))
					{
						if (MCU_strncasecmp(p_string, modifier_tokens[i].token, modifier_tokens[i].token_length) == 0)
						{
							t_mods |= modifier_tokens[i].modifier;
							p_string += modifier_tokens[i].token_length;
							t_token_found = true; 
						}
						
					}
					i++;
				}
				if (!t_token_found)
					break;
			}
		}
		if (r_endstr - p_string == 1)
		{
			t_key = *p_string;
		}
		else if (r_endstr - p_string > 1)
		{
			MCString t_string(p_string, r_endstr - p_string);
			t_key = MCLookupAcceleratorKeysym(t_string);
			if (t_key != 0)
				t_keyname = p_string;
			else
				t_mods = 0;
		}
		if (t_key <= 255)
		{
			uint1 t_lowerkey = MCS_tolower((uint1)t_key);
			if (t_lowerkey < 'a' || t_lowerkey > 'z')
				t_mods &= ~MS_SHIFT;
		}
		p_menuitem->modifiers = t_mods;
		p_menuitem->accelerator = t_key;
		p_menuitem->accelerator_name = t_keyname;
	}
	return false;
}

bool ParseMenuItemTabs(char *p_string, char *&r_endptr, MCMenuItem *p_menuitem)
{
	uint4 t_tabs = 0;
	const char *t_strptr = p_string;
	uint4 t_strlen = r_endptr - p_string;
	if (MCU_strchr(t_strptr, t_strlen, '\t', False))
	{
		while (t_strptr < r_endptr && t_strptr[0] == '\t')
		{
			t_tabs++;
			t_strptr++;
		}
		memmove((char *)t_strptr - t_tabs, (char *)t_strptr, r_endptr - t_strptr);
		r_endptr -= t_tabs;
		p_menuitem->depth = t_tabs;
	}
	return false;
}

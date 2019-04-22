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

void ParseMenuItemString(MCStringRef p_string, MCStringRef p_mutable, MCMenuItem *p_menuitem);
void ParseMenuItemTabs(MCStringRef p_string, uindex_t &x_offset, MCMenuItem *p_menuitem);
void ParseMenuItemSwitches(MCStringRef p_string, uindex_t &x_offset, MCStringRef p_mutable, MCMenuItem *p_menuitem);
void ParseMenuItemLabel(MCStringRef p_string, uindex_t &x_offset, MCStringRef p_mutable, MCMenuItem *p_menuitem);
void ParseMenuItemAccelerator(MCStringRef p_string, uindex_t &x_offset, MCMenuItem *p_menuitem);
bool IsEscapeChar(MCStringRef p_string, uindex_t p_offset, uint1 p_menumod);

static const Keynames accelerator_keys[] =
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

void MCMenuItem::assignFrom(MCMenuItem *p_other)
{
	depth = p_other->depth;
	MCValueAssign(label, p_other->label);
	is_disabled = p_other->is_disabled;
	is_radio = p_other->is_radio;
	is_hilited = p_other->is_hilited;
	accelerator = p_other->accelerator;
	MCValueAssign(accelerator_name, p_other->accelerator_name);
	modifiers = p_other->modifiers;
	mnemonic = p_other->mnemonic;
	MCValueAssign(tag, p_other->tag);
	menumode = p_other->menumode;
    // SN-2014-07-29: [[ Bug 12998 ]] has_tag member put back
    has_tag = p_other->has_tag;
}

uint4 MCLookupAcceleratorKeysym(MCStringRef p_name)
{
	for (int i = 0; accelerator_keys[i].keysym != 0; i++)
	{
		if (MCStringIsEqualToCString(p_name, accelerator_keys[i].name, kMCCompareCaseless))
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

void MCParseMenuString(MCStringRef p_string, IParseMenuCallback *p_callback, uint1 p_menumode)
{
	MCAutoArrayRef t_lines;
	uindex_t t_nlines = 0;
	/* UNCHECKED */ MCStringSplit(p_string, kMCLineEndString, nil, kMCStringOptionCompareExact, &t_lines);
	t_nlines = MCArrayGetCount(*t_lines);
	bool t_hastags = false;
	
	p_callback->Start();
	
	for (uindex_t i=0; i<t_nlines; i++)
	{
		MCMenuItem t_menuitem;
		t_menuitem.menumode = p_menumode;

		MCValueRef t_lineval = nil;
		/* UNCHECKED */ MCArrayFetchValueAtIndex(*t_lines, i + 1, t_lineval);
		MCStringRef t_line;
		t_line = (MCStringRef)t_lineval;
		
		MCStringRef t_new_line = nil;
		/* UNCHECKED */ MCStringCreateMutable(0, t_new_line);
		ParseMenuItemString(t_line, t_new_line, &t_menuitem);
		MCValueRelease(t_new_line);
		
		// MW-2013-12-18: [[ Bug 11605 ]] If the tag is empty, and the label can convert
		//   to native then take that to be the tag.
        // SN-2014-06-23: The tag can now have unicode as well
		if (MCStringIsEmpty(t_menuitem . tag))
            MCValueAssign(t_menuitem . tag, t_menuitem . label);
        
		p_callback->ProcessItem(&t_menuitem);

		if (!MCStringIsEmpty(t_menuitem.tag))
		{
			MCValueAssign(t_menuitem.tag, kMCEmptyString);
			t_hastags = true;
		}
	}
	
	p_callback->End(t_hastags);
}

void ParseMenuItemString(MCStringRef p_string, MCStringRef p_mutable, MCMenuItem *p_menuitem)
{
	// This now only traverses the input string once and copies non-special
	// characters to the output instead of erasing them in-place.
	uindex_t t_offset = 0;
	ParseMenuItemSwitches(p_string, t_offset, p_mutable, p_menuitem);
	ParseMenuItemLabel(p_string, t_offset, p_mutable, p_menuitem);
}

void ParseMenuItemSwitches(MCStringRef p_string, uindex_t &x_offset, MCStringRef p_mutable, MCMenuItem *p_menuitem)
{
	uindex_t t_length;
	t_length = MCStringGetLength(p_string);
	
	bool t_done = false;
	
	while (!t_done && x_offset < t_length)
	{
		switch (MCStringGetCharAtIndex(p_string, x_offset))
		{
			case '(':
				if (p_menuitem->is_disabled)
					t_done = true;
				
				x_offset++;
				
				if (x_offset < t_length && MCStringGetCharAtIndex(p_string, x_offset) == '(')
				{
					// Emit a literal '('
					x_offset++;
					/* UNCHECKED */ MCStringAppendFormat(p_mutable, "(");
				}
				else 
					p_menuitem->is_disabled = true;
				break;
				
			case '!':
				if (p_menuitem->is_hilited || p_menuitem->is_radio || p_menuitem->menumode == WM_OPTION)
					t_done = true;
				else if (t_length - x_offset == 2)
					t_done = true;
				else
				{
					unichar_t t_char;
					t_char = MCStringGetCharAtIndex(p_string, x_offset + 1);
					if (t_char == '!')
					{
						// Emit a literal '!'
						x_offset += 2;
						/* UNCHECKED */ MCStringAppendFormat(p_mutable, "!");
					}
					else
					{
						if (t_char == 'c')
							p_menuitem->is_hilited = true;
						else if (t_char != 'n')
						{
							p_menuitem->is_radio = true;
							if (t_char == 'r')
								p_menuitem->is_hilited = true;
						}
						x_offset += 2;
					}
				}
				break;
				
			case '\t':
				if (p_menuitem->depth == 0 && p_menuitem->menumode != WM_OPTION && p_menuitem->menumode != WM_COMBO)
				{
					ParseMenuItemTabs(p_string, x_offset, p_menuitem);
				}
				else
				{
					/* UNCHECKED */ MCStringAppendFormat(p_mutable, "\t");
					x_offset++;
				}
				break;
			default:
				t_done = true;
				break;
		}
	}
}

void ParseMenuItemLabel(MCStringRef p_string, uindex_t &x_offset, MCStringRef p_mutable, MCMenuItem *p_menuitem)
{
	uindex_t t_length;
	t_length = MCStringGetLength(p_string);
	
	while (x_offset < t_length)
	{
		unichar_t t_char;
		t_char = MCStringGetCharAtIndex(p_string, x_offset);
		if (IsEscapeChar(p_string, x_offset, p_menuitem->menumode))
		{
			// Don't treat the next character as special; copy it to the output
			/* UNCHECKED */ MCStringAppendSubstring(p_mutable, p_string, MCRangeMake(x_offset + 1, 1));
            // And move the offset past the escape char and the copied char.
			x_offset += 2;
		}
		else if (p_menuitem->menumode != WM_OPTION 
				 && t_char == '&'
				 && p_menuitem->mnemonic == 0
				 && (t_length - x_offset > 1))
		{
			// Note: the "mnemonic" field is offset by 1 from the index of the mnemonic char
			p_menuitem->mnemonic = MCStringGetLength(p_mutable) + 1;
			x_offset++;
		}
		else if (p_menuitem->menumode != WM_OPTION && t_char == '/')
		{
			// The text copied up to this point should become the label
			MCStringRef t_label = nil;
			/* UNCHECKED */ MCStringCopy(p_mutable, t_label);
			MCValueAssign(p_menuitem->label, t_label);
			MCValueRelease(t_label);
			
			ParseMenuItemAccelerator(p_string, ++x_offset, p_menuitem);
			return;
		}
		else
		{
			// Copy this character to the output
			/* UNCHECKED */ MCStringAppendSubstring(p_mutable, p_string, MCRangeMake(x_offset, 1));
			x_offset++;
		}
	}
	
	MCStringRef t_label = nil;
	/* UNCHECKED */ MCStringCopy(p_mutable, t_label);
	MCValueAssign(p_menuitem->label, t_label);
	MCValueRelease(t_label);
}

bool IsEscapeChar(MCStringRef p_string, uindex_t p_offset, uint1 p_menumode)
{
	uindex_t t_length;
	t_length = MCStringGetLength(p_string);
	if (t_length < 2)
		return false;
	
	unichar_t t_char;
	t_char = MCStringGetCharAtIndex(p_string, p_offset);
	switch (t_char)
	{
		case '/':
		case '!':
		case '&':
			if (p_menumode == WM_OPTION)
				return false;
			// Fallthrough
		case '(':
			return MCStringGetCharAtIndex(p_string, p_offset + 1) == t_char;
		case '\\':
			return true;
	}
	
	return false;
}
										   
void ParseMenuItemAccelerator(MCStringRef p_string, uindex_t &x_offset, MCMenuItem *p_menuitem)
{
	uint1 t_mods = 0;
	unichar_t t_key = 0;
    MCAutoStringRef t_keyname;
    t_keyname = kMCEmptyString;
	
	uindex_t t_tag;
	uindex_t t_length;
	t_length = MCStringGetLength(p_string);
	
	if (MCStringLastIndexOfChar(p_string, '|', t_length, kMCStringOptionCompareExact, t_tag)
		&& (t_length - t_tag) > 1)
	{
		MCStringRef t_tag_str = nil;
		/* UNCHECKED */ MCStringCopySubstring(p_string, MCRangeMakeMinMax(t_tag + 1, t_length), t_tag_str);
		MCValueAssignAndRelease(p_menuitem->tag, t_tag_str);
		
		// Don't do any further processing of the string after the '|'
		t_length = t_tag;
	}
	
	if (p_menuitem->menumode == WM_PULLDOWN || p_menuitem->menumode == WM_TOP_LEVEL)
	{
		if (t_length - x_offset == 1)
			t_mods |= MS_CONTROL;
		
		while (t_length - x_offset > 1)
		{
			if (MCStringGetCharAtIndex(p_string, x_offset) == ' ')
			{
				x_offset++;
			}
			else
			{
				// Search the string for recognised modifier key tokens
				uindex_t i = 0;
				bool t_token_found = false;
				while (!t_token_found && modifier_tokens[i].token != NULL)
				{
					MCAutoStringRef t_tok_str;
					MCRange t_range;
					t_range = MCRangeMake(x_offset, modifier_tokens[i].token_length);
					/* UNCHECKED */ MCStringCreateWithCString(modifier_tokens[i].token, &t_tok_str);
					if (MCStringSubstringIsEqualTo(p_string, t_range, *t_tok_str, kMCStringOptionCompareCaseless))
					{
						t_mods |= modifier_tokens[i].modifier;
						x_offset += modifier_tokens[i].token_length;
						t_token_found = true;
						
						if (modifier_tokens[i].token_length == 1)
							break;
					}
	
					i++;
				}
				if (!t_token_found)
					break;
			}
		}
		if ((t_length - x_offset) == 1)
			t_key = MCStringGetCharAtIndex(p_string, x_offset);
		else if ((t_length - x_offset) > 1)
		{
			MCAutoStringRef t_key_string;
			MCStringCopySubstring(p_string, MCRangeMakeMinMax(x_offset, t_length), &t_key_string);
			t_key = MCLookupAcceleratorKeysym(*t_key_string);
			if (t_key != 0)
                t_keyname.Reset(*t_key_string);
            else
				t_mods = 0;
		}
		
		// Ignore shift state for non-letter keys
		// Will this work for non-English locales? Maybe "if (!MCS_isletter(t_key))"
		if (!MCUnicodeIsAlphabetic(t_key))
				t_mods &= ~MS_SHIFT;
		p_menuitem->modifiers = t_mods;
		p_menuitem->accelerator = t_key;
		MCValueAssign(p_menuitem->accelerator_name, *t_keyname);
	}
}
	
void ParseMenuItemTabs(MCStringRef p_string, uindex_t &x_offset, MCMenuItem *p_menuitem)
{
	// Eat all consecutive tab characters in the input string
	while (MCStringGetCharAtIndex(p_string, x_offset) == '\t')
		x_offset++, p_menuitem->depth++;
}

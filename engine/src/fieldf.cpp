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

#include "stack.h"
#include "card.h"
#include "cdata.h"
#include "scrolbar.h"
#include "button.h"
#include "field.h"
#include "MCBlock.h"
#include "paragraf.h"
#include "sellst.h"
#include "undolst.h"
#include "util.h"
#include "font.h"

#include "dispatch.h"
#include "mode.h"
#include "globals.h"
#include "mctheme.h"
#include "redraw.h"
#include "line.h"

#include "context.h"

#ifndef _MACOSX
//normal, control, alt, alt-control
const Keytranslations MCField::emacs_keys[] =
{
	{XK_Home, {FT_HOME, FT_FOCUSFIRST, FT_HOME, FT_FOCUSFIRST}},
	{XK_Left, {FT_LEFTCHAR, FT_LEFTWORD, FT_LEFTCHAR, FT_FOCUSPREV}},
	{XK_Up, {FT_UP, FT_FOCUSPREV, FT_UP, FT_FOCUSPREV}},
	{XK_Right, {FT_RIGHTCHAR, FT_RIGHTWORD, FT_RIGHTCHAR, FT_FOCUSNEXT}},
	{XK_Down, {FT_DOWN, FT_FOCUSNEXT, FT_DOWN, FT_FOCUSNEXT}},
	{XK_Prior, {FT_PAGEUP, FT_PAGEUP, FT_PAGEUP, FT_PAGEUP}},
	{XK_Next, {FT_PAGEDOWN, FT_PAGEDOWN, FT_PAGEDOWN, FT_PAGEDOWN}},
	{XK_Select, {FT_END, FT_FOCUSLAST, FT_END, FT_FOCUSLAST}},
	{XK_End, {FT_END, FT_FOCUSLAST, FT_END, FT_FOCUSLAST}},
	{XK_BackSpace, {FT_DELBCHAR, FT_DELBWORD, FT_UNDO, FT_UNDEFINED}},
	{XK_Delete, {FT_DELBCHAR, FT_DELBWORD, FT_UNDO, FT_UNDEFINED}},
	{XK_Insert, {FT_UNDEFINED, FT_COPY, FT_UNDEFINED, FT_UNDEFINED}},
	{XK_Tab, {FT_TAB, FT_TAB, FT_TAB, FT_TAB}},
	{XK_KP_Tab, {FT_TAB, FT_TAB, FT_TAB, FT_TAB}},
	{XK_Return, {FT_PARAGRAPH, FT_FOCUSNEXT, FT_PARAGRAPH, FT_FOCUSNEXT}},
	{XK_KP_Enter, {FT_PARAGRAPH, FT_FOCUSNEXT, FT_PARAGRAPH, FT_FOCUSNEXT}},
	{XK_A, {FT_UNDEFINED, FT_BOL, FT_BOS, FT_LEFTPARA}},
	{XK_a, {FT_UNDEFINED, FT_BOL, FT_BOS, FT_LEFTPARA}},
	{XK_B, {FT_UNDEFINED, FT_LEFTCHAR, FT_UNDEFINED, FT_LEFTWORD}},
	{XK_b, {FT_UNDEFINED, FT_LEFTCHAR, FT_UNDEFINED, FT_LEFTWORD}},
	{XK_D, {FT_UNDEFINED, FT_DELFCHAR, FT_DELFWORD, FT_UNDEFINED}},
	{XK_d, {FT_UNDEFINED, FT_DELFCHAR, FT_DELFWORD, FT_UNDEFINED}},
	{XK_E, {FT_UNDEFINED, FT_EOL, FT_EOS, FT_RIGHTPARA}},
	{XK_e, {FT_UNDEFINED, FT_EOL, FT_EOS, FT_RIGHTPARA}},
	{XK_F, {FT_UNDEFINED, FT_RIGHTCHAR, FT_UNDEFINED, FT_RIGHTWORD}},
	{XK_f, {FT_UNDEFINED, FT_RIGHTCHAR, FT_UNDEFINED, FT_RIGHTWORD}},
	{XK_H, {FT_UNDEFINED, FT_DELBCHAR, FT_UNDEFINED, FT_DELBWORD}},
	{XK_h, {FT_UNDEFINED, FT_DELBCHAR, FT_UNDEFINED, FT_DELBWORD}},
	{XK_K, {FT_UNDEFINED, FT_CUTLINE, FT_UNDEFINED, FT_UNDEFINED}},
	{XK_k, {FT_UNDEFINED, FT_CUTLINE, FT_UNDEFINED, FT_UNDEFINED}},
	{XK_L, {FT_UNDEFINED, FT_CENTER, FT_UNDEFINED, FT_UNDEFINED}},
	{XK_l, {FT_UNDEFINED, FT_CENTER, FT_UNDEFINED, FT_UNDEFINED}},
	{XK_N, {FT_UNDEFINED, FT_DOWN, FT_UNDEFINED, FT_DOWN}},
	{XK_n, {FT_UNDEFINED, FT_DOWN, FT_UNDEFINED, FT_DOWN}},
	{XK_P, {FT_UNDEFINED, FT_UP, FT_UNDEFINED, FT_UP}},
	{XK_p, {FT_UNDEFINED, FT_UP, FT_UNDEFINED, FT_UP}},
	{XK_V, {FT_UNDEFINED, FT_PAGEDOWN, FT_PAGEUP, FT_BOF}},
	{XK_v, {FT_UNDEFINED, FT_PAGEDOWN, FT_PAGEUP, FT_BOF}},
	{XK_Y, {FT_UNDEFINED, FT_PASTE, FT_UNDEFINED, FT_UNDEFINED}},
	{XK_y, {FT_UNDEFINED, FT_PASTE, FT_UNDEFINED, FT_UNDEFINED}},
	{XK_bracketleft, {FT_UNDEFINED, FT_BOL, FT_LEFTPARA, FT_BOF}},
	{XK_bracketright, {FT_UNDEFINED, FT_EOL, FT_RIGHTPARA, FT_EOF}},
	{XK_osfHelp, {FT_HELP, FT_HELP, FT_HELP, FT_HELP}},
	{XK_osfCut, {FT_CUT, FT_CUT, FT_CUT, FT_CUT}},
	{XK_osfCopy, {FT_COPY, FT_COPY, FT_COPY, FT_COPY}},
	{XK_osfPaste, {FT_PASTE, FT_PASTE, FT_PASTE, FT_PASTE}},
	{XK_osfUndo, {FT_UNDO, FT_UNDO, FT_UNDO, FT_UNDO}},
	{XK_WheelDown, {FT_SCROLLDOWN, FT_UNDEFINED, FT_UNDEFINED, FT_UNDEFINED}},
	{XK_WheelUp, {FT_SCROLLUP, FT_UNDEFINED, FT_UNDEFINED, FT_UNDEFINED}},
	{XK_WheelLeft, {FT_SCROLLLEFT, FT_UNDEFINED, FT_UNDEFINED, FT_UNDEFINED}},
	{XK_WheelRight, {FT_SCROLLRIGHT, FT_UNDEFINED, FT_UNDEFINED, FT_UNDEFINED}},
#ifdef _MACOSX
	{XK_F1, {FT_UNDO, FT_UNDO, FT_UNDO, FT_UNDO}},
	{XK_F2, {FT_CUT, FT_CUT, FT_CUT, FT_CUT}},
	{XK_F3, {FT_COPY, FT_COPY, FT_COPY, FT_COPY}},
	{XK_F4, {FT_PASTE, FT_PASTE, FT_PASTE, FT_PASTE}},
#endif
	{0x0000, {FT_UNDEFINED, FT_UNDEFINED, FT_UNDEFINED, FT_UNDEFINED}},
};

const Keytranslations MCField::std_keys[] =
{
	{XK_Home, {FT_BOL, FT_BOF, FT_BOF, FT_FOCUSFIRST}},
	{XK_Left, {FT_LEFTCHAR, FT_LEFTWORD, FT_LEFTCHAR, FT_FOCUSPREV}},
	{XK_Up, {FT_UP, FT_BOF, FT_UP, FT_FOCUSPREV}},
	{XK_Right, {FT_RIGHTCHAR, FT_RIGHTWORD, FT_RIGHTCHAR, FT_FOCUSNEXT}},
	{XK_Down, {FT_DOWN, FT_EOF, FT_DOWN, FT_FOCUSNEXT}},
	{XK_Prior, {FT_PAGEUP, FT_PAGEUP, FT_PAGEUP, FT_PAGEUP}},
	{XK_Next, {FT_PAGEDOWN, FT_PAGEDOWN, FT_PAGEDOWN, FT_PAGEDOWN}},
	{XK_Select, {FT_END, FT_FOCUSLAST, FT_END, FT_FOCUSLAST}},
	{XK_End, {FT_EOL, FT_EOF, FT_EOF, FT_FOCUSLAST}},
	{XK_BackSpace, {FT_DELBCHAR, FT_DELBWORD, FT_UNDO, FT_UNDEFINED}},
	{XK_Delete, {FT_DELFCHAR, FT_DELFWORD, FT_UNDO, FT_UNDEFINED}},
	{XK_Insert, {FT_UNDEFINED, FT_COPY, FT_UNDEFINED, FT_UNDEFINED}},
	{XK_Tab, {FT_TAB, FT_TAB, FT_TAB, FT_TAB}},
	{XK_Return, {FT_PARAGRAPH, FT_FOCUSNEXT, FT_PARAGRAPH, FT_FOCUSNEXT}},
	{XK_KP_Enter, {FT_PARAGRAPH, FT_FOCUSNEXT, FT_PARAGRAPH, FT_FOCUSNEXT}},
	{XK_A, {FT_UNDEFINED, FT_SELECTALL, FT_UNDEFINED, FT_UNDEFINED}},
	{XK_a, {FT_UNDEFINED, FT_SELECTALL, FT_UNDEFINED, FT_UNDEFINED}},
	{XK_C, {FT_UNDEFINED, FT_COPY, FT_UNDEFINED, FT_UNDEFINED}},
	{XK_c, {FT_UNDEFINED, FT_COPY, FT_UNDEFINED, FT_UNDEFINED}},
	{XK_V, {FT_UNDEFINED, FT_PASTE, FT_UNDEFINED, FT_UNDEFINED}},
	{XK_v, {FT_UNDEFINED, FT_PASTE, FT_UNDEFINED, FT_UNDEFINED}},
	{XK_X, {FT_UNDEFINED, FT_CUT, FT_UNDEFINED, FT_UNDEFINED}},
	{XK_x, {FT_UNDEFINED, FT_CUT, FT_UNDEFINED, FT_UNDEFINED}},
	{XK_Z, {FT_UNDEFINED, FT_UNDO, FT_UNDEFINED, FT_UNDEFINED}},
	{XK_z, {FT_UNDEFINED, FT_UNDO, FT_UNDEFINED, FT_UNDEFINED}},
	{XK_osfHelp, {FT_HELP, FT_HELP, FT_HELP, FT_HELP}},
	{XK_osfCut, {FT_CUT, FT_CUT, FT_CUT, FT_CUT}},
	{XK_osfCopy, {FT_COPY, FT_COPY, FT_COPY, FT_COPY}},
	{XK_osfPaste, {FT_PASTE, FT_PASTE, FT_PASTE, FT_PASTE}},
	{XK_osfUndo, {FT_UNDO, FT_UNDO, FT_UNDO, FT_UNDO}},
	{XK_WheelDown, {FT_SCROLLDOWN, FT_UNDEFINED, FT_UNDEFINED, FT_UNDEFINED}},
	{XK_WheelUp, {FT_SCROLLUP, FT_UNDEFINED, FT_UNDEFINED, FT_UNDEFINED}},
	{XK_WheelLeft, {FT_SCROLLLEFT, FT_UNDEFINED, FT_UNDEFINED, FT_UNDEFINED}},
	{XK_WheelRight, {FT_SCROLLRIGHT, FT_UNDEFINED, FT_UNDEFINED, FT_UNDEFINED}},
#ifdef _MACOSX
	{XK_F1, {FT_UNDO, FT_UNDO, FT_UNDO, FT_UNDO}},
	{XK_F2, {FT_CUT, FT_CUT, FT_CUT, FT_CUT}},
	{XK_F3, {FT_COPY, FT_COPY, FT_COPY, FT_COPY}},
	{XK_F4, {FT_PASTE, FT_PASTE, FT_PASTE, FT_PASTE}},
#endif
	{0x0000, {FT_UNDEFINED, FT_UNDEFINED, FT_UNDEFINED, FT_UNDEFINED}},
};

Field_translations MCField::trans_lookup(const Keytranslations table[], KeySym key,
										 uint2 modifiers)
{
	uint2 i = 0;
	while (table[i].keysym != 0)
	{
		if (table[i].keysym == key)
			return (Field_translations)table[i].functions[modifiers];
		i++;
	}
	return FT_UNDEFINED;
}

#endif

#ifdef _MACOSX
struct MCKeyBinding
{
	unsigned key : 16;
	unsigned modifiers : 8;
	Field_translations action : 8;
};

#define MK_NONE 0
#define MK_ANY 0xff
#define MK_SHIFT 0x01
#define MK_CMD 0x02
#define MK_CTRL 0x04
#define MK_OPT 0x08
#define MK_IGNORE_SHIFT 0x80

static MCKeyBinding s_mac_keybindings[] =
{
	// Selection
	{ XK_A, MK_CMD | MK_IGNORE_SHIFT, FT_SELECTALL },
	{ XK_a, MK_CMD | MK_IGNORE_SHIFT, FT_SELECTALL },

	// Insertion
	{ XK_Tab, MK_ANY, FT_TAB },
	{ XK_Return, MK_ANY, FT_PARAGRAPH },
	{ XK_KP_Enter, MK_ANY, FT_PARAGRAPH },
	{ XK_O, MK_CTRL, FT_PARAGRAPHAFTER },
	{ XK_o, MK_CTRL, FT_PARAGRAPHAFTER },

	// Clipboard
	{ XK_C, MK_CMD | MK_IGNORE_SHIFT, FT_COPY },
	{ XK_c, MK_CMD | MK_IGNORE_SHIFT, FT_COPY },
	{ XK_X, MK_CMD | MK_IGNORE_SHIFT, FT_CUT },
	{ XK_x, MK_CMD | MK_IGNORE_SHIFT, FT_CUT },
	{ XK_V, MK_CMD | MK_IGNORE_SHIFT, FT_PASTE },
	{ XK_v, MK_CMD | MK_IGNORE_SHIFT, FT_PASTE },
	{ XK_Y, MK_CMD | MK_IGNORE_SHIFT, FT_PASTE },
	{ XK_y, MK_CMD | MK_IGNORE_SHIFT, FT_PASTE },
	{ XK_Z, MK_CMD | MK_IGNORE_SHIFT, FT_UNDO },
	{ XK_z, MK_CMD | MK_IGNORE_SHIFT, FT_UNDO },

	// Scroll Wheel
	{ XK_WheelDown, MK_ANY, FT_SCROLLDOWN },
	{ XK_WheelUp, MK_ANY, FT_SCROLLUP },
	{ XK_WheelLeft, MK_ANY, FT_SCROLLLEFT },
	{ XK_WheelRight, MK_ANY, FT_SCROLLRIGHT },

	// Local Navigation
	{ XK_P, MK_CTRL | MK_IGNORE_SHIFT, FT_UP },
	{ XK_p, MK_CTRL | MK_IGNORE_SHIFT, FT_UP },
	{ XK_Up, MK_NONE | MK_IGNORE_SHIFT, FT_UP },
	{ XK_Up, MK_CMD | MK_IGNORE_SHIFT, FT_BOF },
	{ XK_Up, MK_CTRL, FT_SCROLLPAGEUP },
	{ XK_Up, MK_OPT | MK_IGNORE_SHIFT, FT_BACKPARA },
	{ XK_A, MK_CTRL | MK_IGNORE_SHIFT, FT_BOP },
	{ XK_a, MK_CTRL | MK_IGNORE_SHIFT, FT_BOP },

	{ XK_N, MK_CTRL | MK_IGNORE_SHIFT, FT_DOWN },
	{ XK_n, MK_CTRL | MK_IGNORE_SHIFT, FT_DOWN },
	{ XK_Down, MK_NONE | MK_IGNORE_SHIFT, FT_DOWN },
	{ XK_Down, MK_CMD | MK_IGNORE_SHIFT, FT_EOF },
	{ XK_Down, MK_CTRL, FT_SCROLLPAGEDOWN },
	{ XK_Down, MK_OPT | MK_IGNORE_SHIFT, FT_FORWARDPARA },
	{ XK_E, MK_CTRL | MK_IGNORE_SHIFT, FT_EOP },
	{ XK_e, MK_CTRL | MK_IGNORE_SHIFT, FT_EOP },

	{ XK_B, MK_CTRL | MK_IGNORE_SHIFT, FT_BACKCHAR },
	{ XK_b, MK_CTRL | MK_IGNORE_SHIFT, FT_BACKCHAR },
	{ XK_B, MK_CTRL | MK_OPT | MK_IGNORE_SHIFT, FT_BACKWORD },
	{ XK_b, MK_CTRL | MK_OPT | MK_IGNORE_SHIFT, FT_BACKWORD },
	{ XK_Left, MK_NONE | MK_IGNORE_SHIFT, FT_LEFTCHAR },
	{ XK_Left, MK_CMD | MK_IGNORE_SHIFT, FT_BOL },
	{ XK_Left, MK_CTRL | MK_IGNORE_SHIFT, FT_BOL },
	{ XK_Left, MK_OPT | MK_IGNORE_SHIFT, FT_LEFTWORD },

	{ XK_F, MK_CTRL | MK_IGNORE_SHIFT, FT_FORWARDCHAR },
	{ XK_f, MK_CTRL | MK_IGNORE_SHIFT, FT_FORWARDCHAR },
	{ XK_F, MK_CTRL | MK_OPT | MK_IGNORE_SHIFT, FT_FORWARDWORD },
	{ XK_f, MK_CTRL | MK_OPT | MK_IGNORE_SHIFT, FT_FORWARDWORD },
	{ XK_Right, MK_NONE | MK_IGNORE_SHIFT, FT_RIGHTCHAR },
	{ XK_Right, MK_CMD | MK_IGNORE_SHIFT, FT_EOL },
	{ XK_Right, MK_CTRL | MK_IGNORE_SHIFT, FT_EOL },
	{ XK_Right, MK_OPT | MK_IGNORE_SHIFT, FT_RIGHTWORD },

	// Global Navigation
	{ XK_Prior, MK_NONE, FT_SCROLLPAGEUP },
	{ XK_Prior, MK_SHIFT, FT_PAGEUP },
	{ XK_Prior, MK_OPT | MK_IGNORE_SHIFT, FT_PAGEUP },

	{ XK_Home, MK_NONE, FT_SCROLLTOP },
	{ XK_Home, MK_SHIFT, FT_BOF },

	{ XK_Next, MK_NONE, FT_SCROLLPAGEDOWN },
	{ XK_Next, MK_SHIFT, FT_PAGEDOWN },
	{ XK_Next, MK_OPT | MK_IGNORE_SHIFT, FT_PAGEDOWN },

	{ XK_End, MK_NONE, FT_SCROLLBOTTOM },
	{ XK_End, MK_SHIFT, FT_EOF },

	// Forward Deletion
	{ XK_Delete, MK_NONE, FT_DELFCHAR },
	{ XK_Delete, MK_OPT, FT_DELFWORD },
	{ XK_D, MK_CTRL, FT_DELFCHAR },
	{ XK_d, MK_CTRL, FT_DELFCHAR },
	{ XK_K, MK_CTRL, FT_DELEOP },
	{ XK_k, MK_CTRL, FT_DELEOP },

	// Backward Deletion
	// PM-2015-09-16: [[ Bug 15934 ]] Make sure pressing Backspace key works as expected, even if Shift key is down
	{ XK_BackSpace, MK_IGNORE_SHIFT, FT_DELBCHAR },
	{ XK_BackSpace, MK_CMD, FT_DELBOL },
	{ XK_BackSpace, MK_CTRL, FT_DELBSUBCHAR },
	{ XK_BackSpace, MK_OPT, FT_DELBWORD },
	{ XK_BackSpace, MK_OPT | MK_CTRL, FT_DELBWORD },

	// Misc
	{ XK_L, MK_CTRL, FT_CENTER },
	{ XK_l, MK_CTRL, FT_CENTER },
	{ XK_T, MK_CTRL, FT_TRANSPOSE },
	{ XK_t, MK_CTRL, FT_TRANSPOSE },

	// END
	{ 0, 0, (Field_translations)0 }
};

Field_translations MCField::lookup_mac_keybinding(KeySym p_key, uint32_t p_modifiers)
{
	// Translate old modifiers to Mac-specific ones
	uint32_t t_mk_modifiers;
	t_mk_modifiers = 0;
	if ((p_modifiers & MS_SHIFT) != 0)
		t_mk_modifiers |= MK_SHIFT;
	if ((p_modifiers & MS_CONTROL) != 0)
		t_mk_modifiers |= MK_CMD;
	if ((p_modifiers & MS_MAC_CONTROL) != 0)
		t_mk_modifiers |= MK_CTRL;
	if ((p_modifiers & MS_MOD1) != 0)
		t_mk_modifiers |= MK_OPT;
	
	for(uint32_t i = 0; s_mac_keybindings[i] . key != 0; i++)
		if (s_mac_keybindings[i] . key == p_key)
		{
			uint32_t t_key_modifiers;
			t_key_modifiers = s_mac_keybindings[i] . modifiers;
			if (t_key_modifiers == MK_ANY)
				return s_mac_keybindings[i] . action;
			
			if ((t_key_modifiers & MK_IGNORE_SHIFT) == 0)
			{
				if (t_key_modifiers == t_mk_modifiers)
					return s_mac_keybindings[i] . action;
			}
			else
			{
				t_key_modifiers &= ~MK_IGNORE_SHIFT;
				if (t_key_modifiers == (t_mk_modifiers & ~MK_SHIFT))
					return s_mac_keybindings[i] . action;
			}
		}
	
	return FT_UNDEFINED;
}

#endif

void MCField::resetparagraphs()
{
	findex_t si = 0;
	findex_t ei = 0;

    MCAutoArray<uint32_t> t_lines;
    
	// MW-2005-05-13: [[Fix bug 2766]] We always need to retrieve the hilitedLines
	//   to prevent phantom selections w.r.t. focused paragraph.
	if (flags & F_LIST_BEHAVIOR)
		hilitedlines(t_lines);

	if (MCactivefield == this && focusedparagraph != NULL)
	{
		selectedmark(False, si, ei, True);
		if (flags & F_LIST_BEHAVIOR)
			sethilitedlines(NULL, 0);
		unselect(False, True);
	}
	curparagraph = focusedparagraph = paragraphs;
	firstparagraph = lastparagraph = NULL;
	cury = focusedy = topmargin;
	textx = texty = 0;

	// TS-2005-01-06: Fix for bug 2381
	//   A) reset focusedparagraph and focusedy and restore oldhilitelines if this
	//      was the active field with sethilitedlines
	//   B) pass false for second parameter to sethilitedlines so it doesn't force
	//      a scroll and cause redraw issues while resizing.

	// MW-2005-01-28: Correct small integration error, != instead of ==
	if ((flags & F_LIST_BEHAVIOR) != 0)
	{
		if (t_lines.Ptr() != nil)
        {
			sethilitedlines(t_lines.Ptr(), t_lines.Size(), False);
        }
	}
	else if (ei != 0)
		seltext(si, ei, False);
}

MCCdata *MCField::getcarddata(MCCdata *&list, uint4 parid, Boolean create)
{
	if (flags & F_SHARED_TEXT)
		parid = 0;
	MCCdata *foundptr = NULL;
	if (list != NULL)
	{
		MCCdata *tptr = list;
		do
		{
			if (tptr->getid() == parid)
			{
				foundptr = tptr;
				break;
			}
			tptr = tptr->next();
		}
		while (tptr != list);
	}
	if (foundptr == NULL && create)
	{
		foundptr = new (nothrow) MCCdata(parid);
		foundptr->appendto(list);
	}
	return foundptr;
}

void MCField::openparagraphs()
{
	MCParagraph *pgptr = paragraphs;
	uint1 oldopened = opened;
	opened = 0; // suppress redraws if open pg causes URL download
	do
	{
		pgptr->setparent(this);
		// MW-2012-02-14: [[ FontRefs ]] Pass the field's fontref to open the paragraph.
		pgptr->open(m_font);
		pgptr = pgptr->next();
	}
	while (pgptr != paragraphs);
	opened = (uint1)oldopened;
	focusedparagraph = NULL;
	resetparagraphs();
}

void MCField::closeparagraphs(MCParagraph *pgptr)
{
	MCParagraph *tpgptr = pgptr;
	do
	{
		tpgptr->close();
		tpgptr = tpgptr->next();
	}
	while (tpgptr != pgptr);
}

void MCField::gettabs(uint2 *&t, uint2 &n, Boolean &fixed)
{
	static uint2 ttabs;
	if (ntabs == 0)
	{
		// MW-2012-02-14: [[ FontRefs ]] Compute the width of a space in the field's
		//   font.
		// MW-2012-02-17: If we aren't opened, then just use a default value.
		// MM-2014-04-16: [[ Bug 11964 ]] Pass through the transform of the stack to make sure the measurment is correct for scaled text.
		int4 t_space_width;
		if (opened)
            t_space_width = MCFontMeasureText(m_font, MCSTR(" "), getstack() -> getdevicetransform());
		else
			t_space_width = 8;
		 
		if (t_space_width == 0)
			t_space_width = 8;
		   
		ttabs = t_space_width * DEFAULT_TAB_SPACING;

		t = &ttabs;
		n = 1;
	}
	else
	{
		t = tabs;
		n = ntabs;
	}

	fixed = (flags & F_VGRID) != 0;
}

void MCField::gettabaligns(intenum_t *&a, uint16_t &n)
{
    a = alignments;
    n = nalignments;
}

void MCField::getlisttabs(int32_t& r_first, int32_t& r_second)
{
	uint2 *t_tabs;
	uint2 t_ntabs;
	Boolean fixed;
	gettabs(t_tabs, t_ntabs, fixed);
	
	int32_t t_first_tab, t_second_tab;
	if (t_ntabs == 1)
	{
		t_first_tab = t_tabs[0];
		t_second_tab = t_tabs[0] * 2;
	}
	else
	{
		t_first_tab = t_tabs[0];
		t_second_tab = t_tabs[1];
	}
	
	r_first = t_first_tab;
	r_second = t_second_tab;
}

// MW-2012-01-25: [[ FieldMetrics ]] Compute the x-offset of the left of the
//   field content.
int32_t MCField::getcontentx(void) const
{
	return rect . x + borderwidth - DEFAULT_BORDER - textx + leftmargin;
}

// MW-2012-01-25: [[ FieldMetrics ]] Compute the y-offset of the top of the
//   field content.
int32_t MCField::getcontenty(void) const
{
	return rect . y + borderwidth - TEXT_Y_OFFSET;
}

// MW-2012-01-25: [[ FieldMetrics ]] Compute the optimal layoutwidth for any
//   given line.
int32_t MCField::getlayoutwidth(void) const
{
	return MCU_max(getfwidth() - (leftmargin + rightmargin), 1);
}

// MW-2012-01-25: [[ ParaStyles ]] Return the indent set at the field level.
int32_t MCField::getfirstindent(void) const
{
	return indent;
}

int32_t MCField::gettexty(void) const
{
	return texty;
}

// MW-2012-01-25: [[ ParaStyles ]] Returns whether the 'leader' lines should
//   be drawn.
bool MCField::getshowlines(void) const
{
	return getflag(F_SHOW_LINES) || getstate(CS_SIZE);
}

uint2 MCField::getfwidth() const
{
	int4 width = rect.width;
	if (borderwidth != 0)
		width -= (borderwidth - DEFAULT_BORDER) << 1;
	if (flags & F_SHADOW)
		width -= shadowoffset;
	if (flags & F_VSCROLLBAR)
		width -= vscrollbar->getrect().width;
	if (width > 0 && width < (int4) MAXUINT2)
		return (uint2)width;
	else
		return 0;
}

uint2 MCField::getfheight() const
{
	int4 height = rect.height;
	if (borderwidth != 0)
		height -= borderwidth << 1;
	if (flags & F_SHADOW)
		height -= shadowoffset;
	if (flags & F_HSCROLLBAR)
		height -= hscrollbar->getrect().height;
	if (height > 0 && height < (int4) MAXUINT2)
		return (uint2)height;
	else
		return 0;
}

MCRectangle MCField::getfrect() const
{
	MCRectangle trect = rect;
	if (borderwidth != 0)
		trect = MCU_reduce_rect(trect, borderwidth);
	uint2 soffset = shadowoffset;
	if (flags & F_SHADOW && trect.width > soffset && trect.height > soffset)
	{
		trect.width -= soffset;
		trect.height -= soffset;
	}
	if (flags & F_HSCROLLBAR && trect.height > scrollbarwidth)
		trect.height -= hscrollbar->getrect().height - 1;
	if (flags & F_VSCROLLBAR && trect.width > scrollbarwidth)
		trect.width -= vscrollbar->getrect().width - 1;
	return trect;
}

void MCField::removecursor()
{
	// MW-2006-07-05: [[ Bug 4840 ]] Previous this wouldn't do anything if 'locktext' were true
	//   this meant that the cursor wouldn't be removed correctly when the property changed.
	if (!opened)
		return;
	if (cursorfield == this && (cursoron || flags & F_LIST_BEHAVIOR))
	{
		cursoron = False;
		cursorfield = NULL;
		// MW-2011-08-18: [[ Layers ]] Invalidate the cursor rect.
		layer_redrawrect(cursorrectp);
        layer_redrawrect(cursorrects);
	}
}

void MCField::drawcursor(MCContext *p_context, const MCRectangle &dirty)
{
	setforeground(p_context, DI_FORE, False);
	if (flags & F_LIST_BEHAVIOR)
	{
		// MW-2012-01-27: [[ Bug 9511 ]] Make sure we don't render the win95-esque focus
		//   border in native GTK mode.
		if (!focusedparagraph->IsEmpty() && !IsMacLF() && !IsNativeGTK() && !getstate(CS_MENUFIELD))
		{
			if (MClook == LF_WIN95)
			{
				p_context->setforeground(p_context->getwhite());
				p_context->setbackground(p_context->getblack());
				p_context->setfillstyle(FillSolid, nil, 0, 0);
				p_context->setlineatts(0, LineDoubleDash, CapButt, JoinBevel);
				p_context->setdashes(0, dotlist, 2);
				p_context->setfunction(GXxor);
				p_context->drawrect(cursorrectp);
                p_context->drawrect(cursorrects);
				p_context->setfunction(GXcopy);
				p_context->setlineatts(0, LineSolid, CapButt, JoinBevel);
			}
			else
            {
				p_context->drawrect(cursorrectp);
                p_context->drawrect(cursorrects);
            }
		}
	}
	else
	{
		// MW-2013-08-07: [[ Bug 10840 ]] If the background is opaque then use XOR otherwise
		//   just black (XORing against transparent has no effect).
		bool t_is_opaque;
		t_is_opaque = p_context -> changeopaque(true);
		p_context -> changeopaque(t_is_opaque);
		
		if (!t_is_opaque)
			p_context -> setforeground(p_context -> getblack());
		else
		{
			// MW-2012-08-06: Use XOR to render the caret so it remains visible regardless
			//   of background color (apart from 128,128,128!).
			p_context->setforeground(p_context->getwhite());
            
            /* The XOR blend is no longer supported, fortunately exlusion
             * (and difference) are the same as XOR when the source is white or
             * black. */
			p_context->setfunction(GXblendExclusion);
			
			// MW-2012-09-19: [[ Bug 10393 ]] Draw the caret inside a layer to ensure the XOR
			//   ink works correctly.
			p_context->begin(false);
		}
		
        // MW-2014-04-10: [[ Bug 12020 ]] Make sure we use a linesize of 1 rather than 0 (hairline)
        //   to ensure caret is not too thin on retina displays.
        p_context->setlineatts(1, LineSolid, CapButt, JoinBevel);
		p_context->drawline(cursorrectp.x, cursorrectp.y, cursorrectp.x, cursorrectp.y + cursorrectp.height - 1);
        p_context->drawline(cursorrects.x, cursorrects.y, cursorrects.x, cursorrects.y + cursorrects.height - 1);
        p_context->setlineatts(0, LineSolid, CapButt, JoinBevel);
		
		if (t_is_opaque)
		{
			p_context->end();
		
			p_context->setfunction(GXcopy);
		}
	}
}

void MCField::replacecursor(Boolean force, Boolean goal)
{
	if (!opened)
		return;

	MCRectangle drectp, drects;
    // AL-2014-03-21: [[ Bug 11963 ]] If the field has list behavior, don't split
    //  the cursor rect.
    if (flags & F_LIST_BEHAVIOR)
	{
		drectp = focusedparagraph->getcursorrect(-1, fixedheight, true);
		positioncursor(force, goal, drectp, focusedy, true);
		return;
	}
    
	if (composing && composelength)
	{
		findex_t compsi, compei;
		compsi = composeoffset + composecursorindex;
		compei = composeoffset + composelength;
		indextoparagraph(paragraphs,compsi,compei);
		// MW-2012-01-25: [[ ParaStyles ]] Request the cursor-rect of the line
		//   not including any space above/below.
		drectp = focusedparagraph->getcursorrect(compsi, fixedheight, false, kMCParagraphCursorTypePrimary);
        drects = focusedparagraph->getcursorrect(compsi, fixedheight, false, kMCParagraphCursorTypeSecondary);
	}
	else
	{
		// MW-2012-01-25: [[ ParaStyles ]] Request the cursor-rect of the line
		//   not including any space above/below.
		drectp = focusedparagraph->getcursorrect(-1, fixedheight, false, kMCParagraphCursorTypePrimary);
        drects = focusedparagraph->getcursorrect(-1, fixedheight, false, kMCParagraphCursorTypeSecondary);
	}
	positioncursor(force, goal, drects, focusedy, false);
    positioncursor(force, goal, drectp, focusedy, true);
}

void MCField::positioncursor(Boolean force, Boolean goal, MCRectangle &drect, int4 yoffset, bool primary)
{
	if (flags & F_LIST_BEHAVIOR && MClook != LF_WIN95)
	{
		drect.y -= 2;
		drect.height += 4;
	}
	else
	{
		drect.y--;
		drect.height += 2;
	}

	// MW-2012-01-25: [[ FieldMetrics ]] The 'goalx' value is in field coords.
	// MW-2012-02-02: [[ Bug ]] Make sure we only set goal if it is requested.
	if (goal)
		goalx = drect . x;

	// MW-2012-01-25: [[ FieldMetrics ]] Compute the new position of the cursor
	//   in card coords.
	int32_t newx, newy;
	newx = drect . x + getcontentx();
	newy = yoffset + drect . y + getcontenty();

	MCRectangle mrect = getfrect();
	if (force)
	{
		Boolean reset = False;
		// SMR 951 don't scroll if cursor won't fit in field
		int4 dy = mrect.y + mrect.height - drect.height - newy;

		// MW-2012-02-02: [[ Bug ]] 'newy' now incorporates the rect, so no need
		//   to adjust for it.
		if (newy < mrect.y + 1 && dy > 0)
		{
			int4 oldy = texty;
			vscroll(newy - (mrect.y + 1), True);
			newy += oldy - texty;
			reset = True;
		}
		else if (dy < 0)
		{
			int4 oldy = texty;
			vscroll(-dy, True);
			newy += oldy - texty;
			reset = True;
		}

		if (!(flags & F_LIST_BEHAVIOR))
		{
			// MW-2012-02-02: [[ Bug 9681 ]] Incorrect computations for the left
			//   hand edge of the field mean hscroll doesn't work correctly.
			if (newx < mrect.x + leftmargin - DEFAULT_BORDER)
			{
				hscroll(newx - (mrect.x + leftmargin - DEFAULT_BORDER), True);
				newx = mrect.x + leftmargin - DEFAULT_BORDER;
				reset = True;
			}
			else
			{
				int4 dx = mrect.x + mrect.width - drect.width - newx;
				if (dx < 0)
				{
					hscroll(-dx, True);
					newx += dx;
					reset = True;
				}
				else
				{
					// MW-2012-03-13: [[ Bug ]] Make sure we scroll the field towards
					//   the left edge if the longest line has got cropped.
					dx = (getcontentx() + textwidth + rightmargin - DEFAULT_BORDER) - (mrect . x + mrect . width);
					if (dx < 0)
					{
						dx =  MCU_max(-textx, dx);
						hscroll(dx, True);
						newx -= dx;
						reset = True;
					}
				}					
			}
		}

		if (reset)
			resetscrollbars(True);
	}

	// MW-2006-02-26: Even if the screen is locked we still want to set the cursor
	//   position, otherwise the repositioning gets deferred too much.
	if (!(state & (CS_KFOCUSED | CS_DRAG_TEXT))
	    || (!(flags & F_LIST_BEHAVIOR) && flags & F_LOCK_TEXT))
		return;

	// OK-2008-07-22 : Crash fix.
	if (!(state & CS_DRAG_TEXT) && (focusedparagraph != NULL) && focusedparagraph -> isselection() &&
			(IsMacLF() || IsMacLFAM()))
		return;

	// MW-2012-01-25: [[ FieldMetrics ]] Compute the dirty rect of the cursor.
	drect.x = newx;
	drect.y = newy;
	drect = MCU_intersect_rect(drect, mrect);
	if (drect.width && drect.height
	        && opened && state & (CS_KFOCUSED | CS_DRAG_TEXT))
	{
		if (flags & F_LIST_BEHAVIOR)
		{
			drect.x = mrect.x + 2;
			drect.width = mrect.width - 4;
			if (MClook != LF_WIN95)
			{
				drect.x--;
				drect.width += 2;
			}
		}
        if (primary)
            cursorrectp = drect;
        else
            cursorrects = drect;
		cursorfield = this;
		cursoron = True;
		// MW-2011-08-18: [[ Layers ]] Invalidate the dirty rect.
		layer_redrawrect(drect);
	}
	else
		cursoron = False;
}

void MCField::dragtext()
{
	removecursor();

	// MW-2012-01-25: [[ FieldMetrics ]] Compute the field coords of the mouse
	//   click.
	int32_t cx, cy;
	cx = mx - getcontentx();
	cy = my - (cury + getcontenty());

	MCParagraph *pgptr = curparagraph;
	int4 y = 0;
	uint2 pgheight = pgptr->getheight(fixedheight);
	while (pgptr->next() != paragraphs && y + pgheight <= cy)
	{
		y += pgheight;
		pgptr = pgptr->next();
		pgheight = pgptr->getheight(fixedheight);
	}
	cy -= y;
	findex_t ssi, sei;
	pgptr->getclickindex(cx, cy, fixedheight, ssi, sei, False, False);

	// MW-2012-01-25: [[ ParaStyles ]] Request the cursor rect without spacing.
	MCRectangle drect = pgptr->getcursorrect(ssi, fixedheight, false);
	positioncursor(True, False, drect, paragraphtoy(pgptr), true);

	// Compute the appropriate drag action depending the state of the keyboard
	// MW-2007-12-11: [[ Bug 5542 ]] Wrong keyboard modifier for copying/moving text on OS X
	if (MCdispatcher -> isdragsource())
	{
#ifdef _MACOSX
		if ((MCmodifierstate & MS_MOD1) != 0)
#else
		if ((MCmodifierstate & MS_CONTROL) != 0)
#endif
			MCdragaction = DRAG_ACTION_COPY;
		else
			MCdragaction = (MCallowabledragactions & DRAG_ACTION_MOVE) == 0 ? DRAG_ACTION_COPY : DRAG_ACTION_MOVE;
	}
	else
	{
#ifndef _MACOSX
		if ((MCmodifierstate & MS_MOD1) != 0)
			MCdragaction = (MCallowabledragactions & DRAG_ACTION_MOVE) == 0 ? DRAG_ACTION_COPY : DRAG_ACTION_MOVE;
		else
#endif
			MCdragaction = DRAG_ACTION_COPY;
	}
}

void MCField::computedrag()
{
	findex_t ti, si, ei;
	locmark(False, False, False, False, True, ti, ei);
	selectedmark(False, si, ei, False);
	uint2 c = ti >= si && ti < ei ? PI_ARROW : PI_IBEAM;

	getstack()->setcursor(MCcursors[c], False);
	//XCURSORS
	getstack()->cursoroverride = ( c==PI_ARROW) ;
}

void MCField::adjustpixmapoffset(MCContext *dc, uint2 index, int4 dy)
{
	uint2 i;
	if (!getpindex(index, i))
		return;
	
	uint2 t_current_style;
	MCPatternRef t_current_pixmap;
	int2 t_current_x;
	int2 t_current_y;
	dc -> getfillstyle(t_current_style, t_current_pixmap, t_current_x, t_current_y);
	
	// IM-2014-05-13: [[ HiResPatterns ]] Update to use pattern geometry function
	uint32_t t_width, t_height;
	if (!MCPatternGetGeometry(t_current_pixmap, t_width, t_height))
		return;

	int4 t_offset_x, t_offset_y;
	t_offset_x = t_current_x - textx;
	t_offset_y = t_current_y - texty + dy;
    
    // Ensure the offsets are in the 16-bit signed int range.
    t_offset_y = MCSgn(t_offset_y) * (MCAbs(t_offset_y) % t_height);
    t_offset_x = MCSgn(t_offset_x) * (MCAbs(t_offset_x) % t_width);
    
	dc -> setfillstyle(t_current_style, t_current_pixmap, t_offset_x, t_offset_y);
}

void MCField::drawrect(MCDC *dc, const MCRectangle &dirty)
{
	MCRectangle frect = getfrect();
	MCRectangle trect = frect;
	MCRectangle grect = frect;
	MCRectangle textrect = frect;
	if (!(flags & F_SHOW_BORDER))
		trect = MCU_reduce_rect(trect, -DEFAULT_BORDER);
	trect = MCU_intersect_rect(trect, dirty);
	if (trect.width != 0 && trect.height != 0)
	{
		dc->save();
		dc->cliprect(trect);
		
		// MW-2008-07-23: [[ Bug ]] Previously the background wouldn't be repainted if
		//   linkstart != NULL and this was the field containing it. This caused redraw
		//   oddness on Windows, so have removed the clause.
		if (flags & F_OPAQUE)
		{
			// MW-2010-07-20: Under GTK, field borders are actually 3 pixels, not 2. So
			//   here we take this into account by only filling from an extra pixel in.
			MCRectangle t_fill_rect;
			if (getflag(F_SHOW_BORDER) && borderwidth == DEFAULT_BORDER && IsNativeGTK())
				t_fill_rect = MCU_intersect_rect(MCU_reduce_rect(frect, 1), dirty);
			else
				t_fill_rect = trect;

			setforeground(dc, DI_BACK, False, flags & F_SHOW_BORDER && (!(flags & F_F_AUTO_ARM) || MClook == LF_WIN95));
			
			// MW-2012-09-04: [[ Bug 9759 ]] Make sure any pattern is offset correctly.
			adjustpixmapoffset(dc, DI_BACK);
			
			dc->fillrect(t_fill_rect);
		}

		// Reduce the textrect by horizontal margins.
		textrect.x += leftmargin;
		if (textrect.width > leftmargin + rightmargin)
			textrect.width -= leftmargin + rightmargin;
		else
			textrect.width = 0;

		// Draw the leader lines (if required).
		trect = MCU_intersect_rect(trect, textrect);
		if (flags & F_FIXED_HEIGHT && (flags & F_SHOW_LINES || state & CS_SIZE))
		{
			dc->setforeground(dc->getblack());
			dc->setlineatts(1, LineOnOffDash, CapButt, JoinBevel);
			dc->setdashes(0, dotlist, 2);
			
			int2 x, y;
			x = textrect . x - 2 - textx;
			y = cury + frect.y + fixeda - TEXT_Y_OFFSET;
			while(y <= trect.y + trect.height)
			{
				if (y >= trect.y)
					dc -> drawline(x, y, x + MCU_max(textrect . width + 4, textwidth), y);
				y += fixedheight;
			}
			
			dc -> setlineatts(0, LineSolid, CapButt, JoinBevel);
		}

		uint2 fontstyle;
		fontstyle = gettextstyle();
		
		setforeground(dc, DI_FORE, False);

		// Compute the find range.
		MCParagraph *foundpgptr = NULL;
		findex_t fstart = 0;
		findex_t fend = 0;
		if (foundlength != 0)
		{
			fstart = foundoffset;
			fend = foundoffset + foundlength;
			foundpgptr = indextoparagraph(paragraphs, fstart, fend);
		}

		// Compute the composition range.
		MCParagraph *comppgptr = NULL;
		findex_t compstart,compend;
		compstart = compend = 0;
		if (composelength)
		{
			compstart = composeoffset;
			compend = composeoffset+composelength;
			comppgptr = indextoparagraph(paragraphs, compstart, compend);
		}

		// MW-2012-01-10: [[ Field Metrics ]] Compute the top-left of the current
		//   paragraph content in card co-ords.
		int32_t x, y;
		MCParagraph *pgptr;
		pgptr = curparagraph;
		x = getcontentx();
		y = cury + getcontenty();

		// MW-2012-01-25: [[ FieldMetrics ]] Compute the layout width.
		int32_t t_layout_width;
		t_layout_width = getlayoutwidth();

		// MW-2012-01-25: [[ FieldMetrics ]] Compute the selection offset / width.
		uint2 sx, swidth;
		if (getflag(F_LIST_BEHAVIOR))
			sx = frect . x, swidth = frect . width;
		else
			sx = x, swidth = t_layout_width;

		uint2 a = 0;
		uint2 d = 0;
		if (flags & F_FIXED_HEIGHT)
		{
			a = fixeda;
			d = fixedd;
		}

        // Calculate the total heights of all paragraphs for vertical centring
        // purposes (this is currently only for combo box entry fields)
        if (parent && parent->gettype() == CT_BUTTON)
        {
            coord_t t_totalpgheight;
            t_totalpgheight = 0.0f;
            MCParagraph* t_pg = pgptr;
            do
            {
                t_totalpgheight += pgptr->getheight(fixedheight);
                t_pg = t_pg->next();
            }
            while (t_pg != paragraphs);
            
            // Single-line fields look better when centred slightly differently
            bool t_single_line;
            t_single_line = paragraphs->next() == paragraphs && t_pg->getlines()->next() == t_pg->getlines();
            if (t_single_line)
            {
                t_totalpgheight -= paragraphs->getlines()->GetLeading();
            }
            
            // Adjust the drawing y coordinate to account for centring
            if (t_totalpgheight < getfheight())
            {
                // Amount of unused space in the field
                coord_t t_spare;
                t_spare = getfheight() - t_totalpgheight;

                if (t_single_line)
                    y += t_spare/2 + (paragraphs->getlines()->GetAscent() - paragraphs->getlines()->GetDescent())/4;
                else
                    y += t_spare/2;
            }
        }
        
		int32_t pgheight;
		do
		{
            pgheight = pgptr->getheight(fixedheight);
			
			// MW-2012-03-15: [[ Bug 10069 ]] A paragraph might render a grid line above or below
			//   so make sure we render paragraphs above and below the apparant limits.
			if (y + pgheight >= trect.y  && y <= trect.y + trect.height)
			{
				pgptr->draw(dc, x, y, a, d,
				            pgptr == foundpgptr ? fstart : 0,
				            pgptr == foundpgptr ? fend : 0,
				            pgptr == comppgptr ? compstart : 0,
				            pgptr == comppgptr ? compend : 0,
				            pgptr == comppgptr ? composeconvertingsi : 0,
				            pgptr == comppgptr ? composeconvertingei : 0,
				            t_layout_width, pgheight, sx, swidth,
							fontstyle);
			}
			y += pgheight;
			pgptr = pgptr->next();
		}
		while (y < trect.y + trect.height && pgptr != paragraphs);

		// MW-2012-03-15: [[ Bug 10069 ]] If we have hGrid set on the field, then render grid lines
		//   to fill the rest of the field using the last pgheight we had.
		if (getflag(F_HGRID))
		{
			setforeground(dc, DI_BORDER, False);
			
            // SN-2014-09-10: [[ Bug 13374 ]] If the last line is hidden, the we take the field's lineheight.
            if (pgheight == 0)
                pgheight = fixedheight;
            
            int32_t cy;
            cy = y + pgheight;
            while(cy < grect . y + grect . height)
            {
                if (y >= grect . y)
                    dc -> drawline(grect . x, cy, grect . x + grect . width, cy);
                cy += pgheight;
            }
		}
		
		// MW-2012-03-15: [[ Bug 10069 ]] If we have vGrid set on the field, then render grid lines
		//   using field tabStops in bottom part of field.
		if (getflag(F_VGRID) && y < grect . y + grect . height)
		{
			uint2 *t;
			uint2 nt;
			Boolean fixed;
			gettabs(t, nt, fixed);
			
			setforeground(dc, DI_BORDER, False);
			
			// MW-2012-03-19: [[ Bug 10069 ]] Replicate the exact same calculation as at paragraph
			//   level, otherwise a strange 'shift' occurs.
			int32_t t_delta;
			t_delta = getcontentx() - 1;
			
			uint2 ct = 0;
			int4 t_x;
			t_x = t_delta + t[0];
            
			while (t_x <= grect.x + grect.width)
			{
				// MW-2012-05-03: [[ Bug 10200 ]] If set at the field level, the vGrid should start
				//   just inside the border for backwards compatibility.
				if (t_x >= grect.x)
					dc->drawline(t_x, grect.y, t_x, grect.y + grect.height);

				if (ct < nt - 1)
					t_x = t_delta + t[++ct];
				else if (nt == 1)
					t_x += t[0];
				else
					t_x += t[nt - 1] - t[nt - 2];
				
				// MW-2012-03-19: [[ FixedTable ]] If we have reached the final tab in fixed
				//   table mode, we are done.
				// PM-2014-04-08: [[ Bug 12146 ]] Setting tabstops to 2 equal numbers and then
                //  turning VGrid on, hangs LC, because this while loop ran forever
                // MW-2015-05-28: [[ Bug 12341 ]] Only stop rendering lines if in 'fixed width table'
                //   mode - indicated by the last two tabstops being the same.
                if (nt >= 2 && t[nt - 1] == t[nt - 2] && ct == nt - 1)
                    break;
			}
		}

		if (cursoron && cursorfield == this)
			drawcursor(dc, dirty);

		dc->restore();
	}
	
	trect = MCU_intersect_rect(rect, dirty);
	if (flags & F_HSCROLLBAR)
	{
		MCRectangle hrect = MCU_intersect_rect(hscrollbar->getrect(), trect);
		if (hrect.width != 0 && hrect.height != 0)
		{
			dc->save();
			dc->cliprect(hrect);
			
			// MW-2011-09-06: [[ Redraw ]] Render the scrollbar normally (not as a sprite).
			hscrollbar->draw(dc, hrect, false, false);
			
			dc->restore();
		}
	}
	if (flags & F_VSCROLLBAR)
	{
		MCRectangle vrect = MCU_intersect_rect(vscrollbar->getrect(), trect);
		if (vrect.width != 0 && vrect.height != 0)
		{
			dc->save();
			dc->cliprect(vrect);
			
			// MW-2011-09-06: [[ Redraw ]] Render the scrollbar normally (not as a sprite).
			vscrollbar->draw(dc, vrect, false, false);
			
			dc->restore();
		}
	}
}

void MCField::draw3dhilite(MCDC *dc, const MCRectangle &trect)
{
	if (flags & F_3D_HILITE)
	{
		draw3d(dc, trect, ETCH_RAISED, borderwidth);
		parent->setforeground(dc, DI_FORE, False);
	}
}

void MCField::setfocus(int2 x, int2 y)
{
	MCParagraph *spg = focusedparagraph;
	int4 sy = focusedy;
	state &= ~(CS_DELETING | CS_PARTIAL);

	// MW-2012-01-25: [[ FieldMetrics ]] Convert card co-ords to field co-ords.
	x -= getcontentx();
	y -= getcontenty();
	int2 direction = 0;

	do
	{
		direction = focusedparagraph->setfocus(x, y - focusedy, fixedheight,
		                                       extend, extendwords, extendlines,
		                                       direction,
		                                       focusedparagraph == paragraphs,
		                                       focusedparagraph->next() == paragraphs, contiguous);
		if (direction < 0)
		{
			if (focusedparagraph != paragraphs)
			{
				if (flags & F_LIST_BEHAVIOR && contiguous)
					focusedparagraph->sethilite(focusedparagraph == firstparagraph
					                            && extend && flags & F_MULTIPLE_HILITES);
				if (extend)
				{
					if (focusedparagraph == firstparagraph)
					{
						firstparagraph = focusedparagraph->prev();
						firsty -= firstparagraph->getheight(fixedheight);
					}
					else
						lastparagraph = focusedparagraph->prev();
				}
				focusedparagraph = focusedparagraph->prev();
				focusedy -= focusedparagraph->getheight(fixedheight);
			}
		}
		else
			if (direction > 0)
				if (focusedparagraph->next() != paragraphs)
				{
					if (flags & F_LIST_BEHAVIOR && contiguous)
						focusedparagraph->sethilite(focusedparagraph == lastparagraph
						                            && extend && flags & F_MULTIPLE_HILITES);
					if (extend)
					{
						if (focusedparagraph == lastparagraph)
							lastparagraph = focusedparagraph->next();
						else
						{
							firsty += focusedparagraph->getheight(fixedheight);
							firstparagraph = focusedparagraph->next();
						}
					}
					focusedy += focusedparagraph->getheight(fixedheight);
					focusedparagraph = focusedparagraph->next();
				}
	}
	while (direction != 0);
	Boolean needredraw = (state & CS_SELECTING) != 0;
	if (flags & F_LIST_BEHAVIOR)
	{
		if (!contiguous && !extend && focusedparagraph->gethilite())
		{
			focusedparagraph->sethilite(False);
			firstparagraph = lastparagraph = NULL;
			state &= ~CS_SELECTING;

			// MW-2005-01-06: Integration of Tuviah's fix for Bug 2384
			if (state & CS_MFOCUSED)
			{
				signallisteners(P_HILITED_LINES);
				message(MCM_selection_changed);
			}

			extendwords = extendlines = False;
			contiguous = True;
		}
		else
			if (flags & F_MULTIPLE_HILITES && extend && firstparagraph != NULL
			        && lastparagraph != NULL)
			{
				MCParagraph *pgptr = firstparagraph;
				do
				{
					pgptr->sethilite(True);
					pgptr = pgptr->next();
				}
				while (pgptr != lastparagraph->next());
			}
			else
			{
				if (!(flags & F_MULTIPLE_HILITES))
					clearhilites();
				firstparagraph = lastparagraph = focusedparagraph;
				firsty = focusedy;
				firstparagraph->sethilite(True);
			}
	}
	if (!(flags & F_LOCK_TEXT))
	{
		findex_t si,ei;
		selectedmark(False, si, ei, False);
		if (composing)
			if (!(si >= composeoffset && ei <= composeoffset + composelength))
				stopcomposition(False, True);
	}
	if (needredraw)
	{
		MCRectangle drect;
		if (focusedy > sy)
		{
			drect.y = sy;
			drect.height = focusedy + focusedparagraph->getheight(fixedheight) - sy;
		}
		else
		{
			drect.y = focusedy;
			drect.height = sy + spg->getheight(fixedheight) - focusedy;
		}
		// MW-2012-01-25: [[ FieldMetrics ]] Compute the dirty-rect (in card coords).
		//   Note that we redraw the entire width of the field rect.
		drect.x = rect.x;
		drect . width = rect . width;
		drect.y += getcontenty();
		drect = MCU_intersect_rect(drect, rect);
		// MW-2011-08-18: [[ Layers ]] Invalidate the dirty rect.
		layer_redrawrect(drect);
	}
}

uint2 MCField::clearhilites()
{
	MCParagraph *pgptr = paragraphs;
	uint2 done = 0;
	do
	{
		if (pgptr->gethilite())
		{
			done++;
			pgptr->sethilite(False);
		}
		pgptr = pgptr->next();
	}
	while (pgptr != paragraphs);
	return done;
}

void MCField::reverse()
{
	MCParagraph *pgptr = firstparagraph;
	while (pgptr != lastparagraph)
	{
		pgptr->reverseselection();
		pgptr = pgptr->next();
	}
	pgptr->reverseselection();
}

void MCField::startselection(int2 x, int2 y, Boolean words)
{
	if (flags & F_NO_AUTO_HILITE)
		return;
	removecursor();
	extendwords = words;
	extendlines = MCscreen->istripleclick();
	if (MCactivefield && MCactivefield != this)
		MCactivefield->unselect(True, True);
	if (MCmodifierstate & MS_SHIFT
	        && (!(flags & F_LIST_BEHAVIOR) || flags & F_MULTIPLE_HILITES))
	{
		extend = True;
		if (!focusedparagraph->isselection()
		        && firstparagraph == lastparagraph)
		{
			firstparagraph = lastparagraph = focusedparagraph;
			firsty = focusedy;
		}
		else
		{
			// MW-2012-01-25: [[ FieldMetrics ]] The y coord is in card coords, hence the
			//   adjustment to the paragraph-y.
			if (focusedparagraph == lastparagraph)
			{
				if (y < getcontenty() + paragraphtoy(lastparagraph))
				{
					reverse();
					focusedparagraph = firstparagraph;
					focusedy = firsty;
				}
			}
			else if (focusedparagraph == firstparagraph && y > getcontenty() + firsty + firstparagraph->getheight(fixedheight))
			{
				reverse();
				focusedparagraph = lastparagraph;
				focusedy = paragraphtoy(lastparagraph);
			}
		}
		state |= CS_SELECTING;
		setfocus(x, y);
	}
	else
	{
		if (flags & F_LIST_BEHAVIOR)
			if ((MCmodifierstate & MS_CONTROL && flags & F_NONCONTIGUOUS_HILITES)
			        || flags & F_TOGGLE_HILITE)
				contiguous = False;
			else
			{
				if (clearhilites() > 1)
				{
					// MW-2011-08-18: [[ Layers ]] Invalidate the content rect.
					layer_redrawrect(getfrect());
				}
			}
		else
			if (!extendlines)
			{
				if (MCactivefield == this && !(flags & F_LOCK_TEXT)
				        && (focusedparagraph->isselection()
				            || firstparagraph != lastparagraph))
				{
					findex_t ti, si, ei;
					if (locmark(False, False, False, True, True, ti, ei))
					{
						selectedmark(False, si, ei, False);
						if (ti >= si && ti < ei && si != ei)
						{
							// Here we mark the fact a mouse-down has occurred in
							// the selection. This is used in mdrag to work out
							// whether to initiate a drag-drop operation.
							state |= CS_SOURCE_TEXT;
							return;
						}
					}
				}
				unselect(True, True);
			}
		state |= CS_SELECTING;
		setfocus(x, y);
		if (!(state & CS_SELECTING))
			return;
			
		firstparagraph = lastparagraph = focusedparagraph;
		firsty = focusedy;
	}
    // SN-2014-12-08: [[ Bug 12784 ]] Only make this field the selectedfield
    //  if it is Focusable
    if (flags & F_TRAVERSAL_ON)
        MCactivefield = this;
	if (!(flags & F_LOCK_TEXT))
	{
		replacecursor(True, True);
		MCscreen->addtimer(this, MCM_internal2, MCsyncrate);
	}
	else
		if (flags & F_LIST_BEHAVIOR)
		{
			replacecursor(True, False);
			MCscreen->addtimer(this, MCM_internal2, MCsyncrate);
		}
}

void MCField::extendselection(int2 x, int2 y)
{
	if (flags & F_NO_AUTO_HILITE)
		return;
	removecursor();
	if (!(flags & F_LIST_BEHAVIOR) || flags & F_MULTIPLE_HILITES)
		extend = True;
	setfocus(x, y);
	replacecursor(True, True);
}

void MCField::endselection()
{
	if (flags & F_NO_AUTO_HILITE || !opened)
		return;
	state &= ~CS_SELECTING;

	Boolean t_was_extend_lines;
	t_was_extend_lines = extendlines;

	extend = extendwords = extendlines = False;
	contiguous = True;
	if (!focusedparagraph->isselection() && firstparagraph == lastparagraph)
	{
		firstparagraph = lastparagraph = NULL;
		firsty = 0;
        
        // Clear the selection, if we're the owner (we don't clear if we're not
        // the owner as that means the last selection was made in another
        // program and the user might be focusing in the field to paste that
        // selection).
        if (MCselection->IsOwned())
            MCselection->Clear();
	}
	else
	{
		// MW-2008-03-27: [[ Bug 282 ]] If we were extending lines, then make sure we include the
		//   next paragraph after the last one to make sure we copy a return character.
		if (!getflag(F_LIST_BEHAVIOR) && t_was_extend_lines)
		{
			if (lastparagraph -> next() != paragraphs)
			{
				lastparagraph = lastparagraph -> next();
				lastparagraph -> setselectionindex(0, 0, False, False);
			}
		}
		if (MCscreen -> hasfeature(PLATFORM_FEATURE_TRANSIENT_SELECTION))
		{
            // Grab the currently selected text so we can place it on the OS'
            // transient-selection clipboard.
            MCAutoStringRef t_string;
			selectedtext(&t_string);
				
            if (*t_string != nil)
            {
                // Place the data on the selection clipboard
                bool t_success = MCselection->AddText(*t_string);
                
                // If successful and this field is focusable, make it the currently
                // focused field.
                if (t_success && (flags & F_TRAVERSAL_ON))
                    MCactivefield = this;
            }
        }

		if (!(flags & F_LOCK_TEXT) && MCU_point_in_rect(rect, mx, my))
		{
			findex_t ti, si, ei;
			locmark(False, False, False, False, True, ti, ei);
			selectedmark(False, si, ei, False);
			uint2 c = ti >= si && ti <= ei ? PI_ARROW : PI_IBEAM;
			getstack()->setcursor(MCcursors[c], False);
		}
	}
}

void MCField::unselect(Boolean clear, Boolean internal)
{
	if (state & CS_SELECTING)
		endselection();
	if (MCactivefield == this && internal)
    {
        // Clear the selection, if we're the owner (we don't clear if we're not
        // the owner as that means the last selection was made in another
        // program and the user might be focusing in the field to paste that
        // selection).
        if (MCselection->IsOwned())
            MCselection->Clear();
    }
	if (clear || (MCactivefield == this && !(state & CS_KFOCUSED)))
		MCactivefield = nil;
	if (!opened || focusedparagraph == NULL)
		return;
	if (!focusedparagraph->isselection() && firstparagraph == lastparagraph)
	{
		firstparagraph = lastparagraph = NULL;
		if (focusedparagraph->next() == paragraphs && !(flags & F_LIST_BEHAVIOR))
			focusedparagraph->setselectionindex(PARAGRAPH_MAX_LEN, PARAGRAPH_MAX_LEN, False, False);
		return;
	}
	if (firstparagraph == NULL)
	{
		if (!(flags & F_LIST_BEHAVIOR))
			focusedparagraph->setselectionindex(PARAGRAPH_MAX_LEN, PARAGRAPH_MAX_LEN, False, False);
		return;
	}
	if (!(flags & F_LIST_BEHAVIOR))
	{
		// MW-2012-01-25: [[ ParaStyles ]] Fetch the cursor-rect including any
		//   space above/below.
		MCRectangle drect = focusedparagraph->getcursorrect(-1, fixedheight, true);

		// MW-2012-01-25: [[ FieldMetrics ]] Convert the cursor-rect to card coords.
		drect.x += getcontentx();
		// MW-2012-08-30: [[ Bug 10331 ]] Make sure we take into account the
		//   location of the focusedparagraph, otherwise selections jump to random
		//   places!
		drect.y += getcontenty() + focusedy;
		focusedparagraph = firstparagraph;
		focusedy = firsty;
		focusedparagraph->setselectionindex(PARAGRAPH_MAX_LEN, PARAGRAPH_MAX_LEN, False, False);
		updateparagraph(False, False);
		while (focusedparagraph != lastparagraph)
		{
			focusedy += focusedparagraph->getheight(fixedheight);
			focusedparagraph = focusedparagraph->next();
			focusedparagraph->setselectionindex(PARAGRAPH_MAX_LEN, PARAGRAPH_MAX_LEN, False, False);
			updateparagraph(False, False);
		}
		setfocus(drect.x, drect.y);
	}
	firstparagraph = lastparagraph = NULL;
}

// MCField::deleteselection == TRUE => reflow
Boolean MCField::deleteselection(Boolean force)
{
	if (focusedparagraph == NULL)
		return False;
	if (!force && !focusedparagraph->isselection()
	        && firstparagraph == lastparagraph)
		return False;
	if (state & CS_SELECTING)
		endselection();
	if (focusedparagraph->isselection() ||
	        firstparagraph != lastparagraph)
	{
		if (firstparagraph != NULL)
		{
			focusedparagraph = firstparagraph;
			focusedy = firsty;
		}
		
		// May require reflow
		focusedparagraph->clearzeros();

		findex_t si, ei;
		selectedmark(False, si, ei, False);
		Ustruct *us = new (nothrow) Ustruct;
		us->type = UT_DELETE_TEXT;
		us->ud.text.index = si;
		us->ud.text.data = cloneselection();
		us->ud.text.newline = False;
		MCundos->freestate();
		MCundos->savestate(this, us);
		MCString s;
		
		// Calls MCParagraph::deletestring - may require reflow
		focusedparagraph->deleteselection();
	}
	if (firstparagraph != lastparagraph)
	{
		firstparagraph = firstparagraph->next();
		while (firstparagraph != lastparagraph)
		{
			MCParagraph *pgptr = firstparagraph->remove(firstparagraph);
			textheight -= pgptr->getheight(fixedheight);
			delete pgptr;
		}
		lastparagraph->deleteselection();

		// Calls MCParagraph::updateparagraph - will reflow
		joinparagraphs();
	}

	// Will reflow
	updateparagraph(True, False);

	firstparagraph = lastparagraph = NULL;
	unselect(False, True);

	return True;
}

void MCField::centerfound()
{
	removecursor();
	findex_t fstart = foundoffset;
	findex_t fend = foundoffset + foundlength;
	fstart = foundoffset;
	MCParagraph *foundpgptr = indextoparagraph(paragraphs, fstart, fend);
	fstart += (fend - fstart) >> 1;
	coord_t x, smally;
	foundpgptr->indextoloc(fstart, fixedheight, x, smally);

	// MW-2012-01-25: [[ FieldMetrics ]] Convert x and y to card co-ords.
	x += getcontentx();
	int4 y = getcontenty() + paragraphtoy(foundpgptr) + smally;
	MCRectangle mrect = getfrect();

	if (y < mrect.y - rect.y)
	{
		int4 offset = y - (mrect.y - rect.y) - (mrect.height >> 1);
		vscroll(offset, False);
		resetscrollbars(True);
		y -= offset;
	}
	else if (y > mrect.height - (mrect.y - rect.y))
	{
		int4 offset = y - (mrect.height - (mrect.y - rect.y)) + (mrect.height >> 1);
		vscroll(offset, False);
		resetscrollbars(True);
		y -= offset;
	}

	setfocus(x, y);

	// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
	layer_redrawall();
	replacecursor(True, True);
}

void MCField::clearfound()
{
	if (foundlength != 0)
	{
		foundoffset = 0;
		foundlength = 0;
		MCfoundfield = nil;
		if (opened)
		{
			// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
			layer_redrawall();
		}
	}
}

void MCField::updateparagraph(Boolean flow, Boolean all, Boolean dodraw)
{
	if (flow)
	{
		// MW-2012-02-27: [[ Bug ]] If the paragraph has no lines, then don't
		//   take into account its current height (otherwise spacing gets
		//   accounted for multiple times).
		uint2 oldheight;
		oldheight = 0;
		if (focusedparagraph -> getlines() != nil)
			oldheight = focusedparagraph->getheight(fixedheight);
		
		// MW-2012-01-25: [[ ParaStyles ]] Get the paragraph to flow itself.
		focusedparagraph -> layout(all);
		uint2 newheight = focusedparagraph->getheight(fixedheight);
		if (newheight != oldheight)
		{
			textheight += newheight;
			textheight -= oldheight;
			all = True;
		}
		uint2 newwidth = focusedparagraph->getwidth();
		if (newwidth > textwidth)
		{
			if (newwidth > getfwidth())
				all = True;
			textwidth = newwidth;
		}
	}
	MCRectangle drect;
	if (all)
	{
		drect = rect;
		resetscrollbars(False);
	}
	else
	{
		drect = focusedparagraph->getdirty(fixedheight);
		// MW-2012-01-25: [[ FieldMetrics ]] Convert the dirty y coord to card.
		drect.y += getcontenty() + focusedy;
		MCRectangle frect = getfrect();
		drect.x = frect.x;
		drect.width = frect.width;
	}
	focusedparagraph->clean();
	if (drect.height != 0 && dodraw)
	{
		// MW-2011-08-18: [[ Layers ]] Invalidate the dirty rect.
		layer_redrawrect(drect);
	}
}

void MCField::joinparagraphs()
{
	textheight -= focusedparagraph->getheight(fixedheight);
	textheight -= ((focusedparagraph->next()))->getheight(fixedheight);
	focusedparagraph->join();
	updateparagraph(True, True);
}

void MCField::fnop(Field_translations function, MCStringRef p_string, KeySym key)
{
}

// MW-2012-02-13: [[ Block Unicode ]] New implementation of finsert which understands
//   unicode text.
void MCField::finsertnew(Field_translations function, MCStringRef p_string, KeySym p_key)
{
	// If there is nothing to insert, do nothing.
	if (MCStringIsEmpty(p_string))
		return;

	// MW-2012-02-16: [[ Bug ]] Bracket any actions that result in
	//   textChanged message by a lock screen pair.
	MCRedrawLockScreen();
	
	// Mark the field as changed.
	state |= CS_CHANGED;

	// Remove the cursor, delete the selection and any composition range.
	removecursor();
	deleteselection(False);
	deletecomposition();
	
	// Compute the start and end point of the selection.
	findex_t si,ei;
	selectedmark(False, si, ei, False);

    // Defer to the paragraph method to insert the text.
    focusedparagraph -> finsertnew(p_string);

	// Compute the end of the selection.
	findex_t ti;
	selectedmark(False, ei, ti, False);
	if (composing)
	{
		composeoffset = si;
		composelength = ei - si;
	}
	uint2 slen = ei - si;

	if (!opened || focusedparagraph == NULL)
	{
		// MW-2012-09-21: [[ Bug 10401 ]] Make sure we unlock the screen!.
		MCRedrawUnlockScreen();
		return; // in case scroll changed contents
	}
	
	// Update the paragraph's layout and replace the cursor.
	firstparagraph = lastparagraph = NULL;
	updateparagraph(True, False);
	replacecursor(True, True);

	if (!composing)
	{
		// Add an undo record.
		Ustruct *us = MCundos->getstate();
		
		// MW-UNDO-FIX: Make sure we only append to a previous record if it
		//   is immediately after the last one.
		if (us != NULL &&
		    (us->type == UT_DELETE_TEXT || us->type == UT_TYPE_TEXT) &&
		    MCundos->getobject() == this &&
		    (findex_t) (us->ud.text.index+us->ud.text.newchars) == si)
		{
			if (us->type == UT_DELETE_TEXT)
			{
				us->type = UT_TYPE_TEXT;
				us->ud.text.newchars = slen;
			}
			else
				us->ud.text.newchars += slen;
		}
		else
		{
			MCundos->freestate();
			us = new (nothrow) Ustruct;
			us->type = UT_TYPE_TEXT;
			
			// MW-UNDO-FIX: Store the index this record starts at
			us->ud.text.index=si;

			us->ud.text.newchars = slen;
			us->ud.text.data = NULL;
			us->ud.text.newline = False;
			MCundos->savestate(this, us);
		}
	}
	
	// MW-2012-02-16: [[ Bug ]] Bracket any actions that result in
	//   textChanged message by a lock screen pair.
	MCRedrawUnlockScreen();
		
	if (!composing)
	{
		// MW-2012-02-08: [[ TextChanged ]] Invoke textChanged as this method
		//   was called as a result of a user action (key input).
		textchanged();
	}
}

void MCField::fdel(Field_translations function, MCStringRef p_string, KeySym key)
{
	// MW-2012-02-16: [[ Bug ]] Bracket any actions that result in
	//   textChanged message by a lock screen pair.
	MCRedrawLockScreen();

	removecursor();
	uint2 oldwidth = focusedparagraph->getwidth();
	if (!deleteselection(False))
	{
		focusedparagraph->clearzeros();
		Ustruct *us = NULL;
		MCParagraph *undopgptr;
		int2 deleted = focusedparagraph->fdelete(function, undopgptr);
		if (deleted < 0)
		{
			if (focusedparagraph == paragraphs)
			{
				// MW-2012-09-21: [[ Bug 10401 ]] Make sure we unlock the screen!.
				MCRedrawUnlockScreen();
				return;
			}
			else
			{
				focusedparagraph = focusedparagraph->prev();
				focusedy -= focusedparagraph->getheight(fixedheight);
				joinparagraphs();
				firstparagraph = lastparagraph = NULL;
				us = new (nothrow) Ustruct;
				us->ud.text.newline = True;
				us->ud.text.data = NULL;
			}
		}
		else
		{
			if (deleted > 0)
			{
				if (focusedparagraph == paragraphs->prev())
				{
					// MW-2012-09-21: [[ Bug 10401 ]] Make sure we unlock the screen!.
					MCRedrawUnlockScreen();
					return;
				}
				else
				{
					joinparagraphs();
					firstparagraph = lastparagraph = NULL;
					us = new (nothrow) Ustruct;
					us->ud.text.newline = True;
					us->ud.text.data = NULL;
				}
			}
			else
			{
				us = new (nothrow) Ustruct;
				us->ud.text.data = undopgptr;
				us->ud.text.newline = False;
				updateparagraph(True, False);
			}
		}
		findex_t si, ei;
		us->type = UT_DELETE_TEXT;
		selectedmark(False, si, ei, False);
		us->ud.text.index = si;
		MCundos->freestate();
		MCundos->savestate(this, us);
	}
	if (oldwidth == textwidth)
	{
		do_recompute(true);
		// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
		layer_redrawall();
	}
	replacecursor(True, True);
	state |= CS_CHANGED;

	// MW-2012-02-16: [[ Bug ]] Bracket any actions that result in
	//   textChanged message by a lock screen pair.
	MCRedrawUnlockScreen();

	// MW-2012-02-08: [[ TextChanged ]] Invoke textChanged as this method
	//   was called as a result of a user action (delete key).
	textchanged();
}

void MCField::fhelp(Field_translations function, MCStringRef p_string, KeySym key)
{
	message(MCM_help);
}

void MCField::fundo(Field_translations function, MCStringRef p_string, KeySym key)
{
	MCundos->undo();
}

void MCField::fcut(Field_translations function, MCStringRef p_string, KeySym key)
{
	cuttext();
}

void MCField::fcutline(Field_translations function, MCStringRef p_string, KeySym key)
{
#ifdef FIELD_CUTLINE_ACTION
	if (!(state & CS_DELETING))
		MCclipboard->clear();
	unselect(False, True);
	uint2 oldheight = focusedparagraph->getheight(fixedheight);
	MCParagraph *cutptr = focusedparagraph->cutline();
	if (cutptr == NULL)
	{
		if (focusedparagraph->next() == paragraphs)
			return;
		joinparagraphs();
		if (focusedparagraph->gettextsize())
			state &= ~CS_PARTIAL;
		cutptr = new (nothrow) MCParagraph;
		cutptr->setparent(this);
	}
	else
	{
		if (!(state & CS_PARTIAL))
			MCclipboard->clearlast();
		state |= CS_PARTIAL;
		textheight -= oldheight;
		updateparagraph(True, True);
	}
	MCclipboard->savetext(cutptr, getw());
	state |= CS_DELETING;
#endif
}

void MCField::fcopy(Field_translations function, MCStringRef p_string, KeySym key)
{
	copytext();
}

void MCField::fpaste(Field_translations function, MCStringRef p_string, KeySym key)
{
	MCObject *optr;
	MCdispatcher -> dopaste(optr);
}

void MCField::ftab(Field_translations function, MCStringRef p_string, KeySym key)
{
	if (message_with_valueref_args(MCM_tab_key, p_string) == ES_NORMAL)
		return;
    // MW-2014-08-12: [[ Bug 13166 ]] If we get a tab key message then we always insert \t
	if (ntabs != 0 && !(flags & F_LOCK_TEXT))
		finsertnew(FT_UNDEFINED, MCSTR("\t"), key);
	else
		if (MCmodifierstate & MS_SHIFT)
			getcard()->kfocusprev(False);
		else
			getcard()->kfocusnext(False);
}

void MCField::ffocus(Field_translations function, MCStringRef p_string, KeySym key)
{
	switch (function)
	{
	case FT_FOCUSFIRST:
		getcard()->kfocusnext(True);
		break;
	case FT_FOCUSLAST:
		getcard()->kfocusprev(True);
		break;
	case FT_FOCUSNEXT:
		getcard()->kfocusnext(False);
		break;
	case FT_FOCUSPREV:
		getcard()->kfocusprev(False);
		break;
	default:
		break;
	}
}

void MCField::freturn(Field_translations function, MCStringRef p_string, KeySym key)
{
    // FG-2014-09-30: [[ Bugfix 13548 ]] Use total height of cursor in calculation
    if (flags & F_AUTO_TAB && cursorrectp.y + ((cursorrects.height + cursorrectp.height) << 1) > rect.y + getfheight())
		getcard()->kfocusnext(False);
	else
	{
		// MW-2012-02-16: [[ Bug ]] Bracket any actions that result in
		//   textChanged message by a lock screen pair.
		MCRedrawLockScreen();

		// MW-2011-08-17: [[ Redraw ]] As updates are deferred the lock and update
		//   are no longer needed.
		removecursor();
		if (!deleteselection(False))
			focusedparagraph->clearzeros();
		textheight -= focusedparagraph->getheight(fixedheight);
		focusedparagraph->split();
		focusedparagraph->setselectionindex(PARAGRAPH_MAX_LEN, PARAGRAPH_MAX_LEN, False, False);
		updateparagraph(True, False);
		focusedy += focusedparagraph->getheight(fixedheight);
		focusedparagraph = focusedparagraph->next();
		firstparagraph = lastparagraph = NULL;
		updateparagraph(True, False);
		replacecursor(True, True);
		state |= CS_CHANGED;

		// MW-2012-02-16: [[ Bug ]] Bracket any actions that result in
		//   textChanged message by a lock screen pair.
		MCRedrawUnlockScreen();
			
		// MW-2012-02-08: [[ TextChanged ]] Invoke textChanged as this method
		//   was called as a result of a user action (return key).
		textchanged();
	}
}

void MCField::fcenter(Field_translations function, MCStringRef p_string, KeySym key)
{
	// MW-2012-01-25: [[ ParaStyles ]] Fetch the cursor rect, including any space
	//   above and below.
	MCRectangle drect = focusedparagraph->getcursorrect(-1, fixedheight, true);
	drect.y += focusedy;
	vscroll(drect.y - (getfheight() >> 1), True);
	resetscrollbars(True);
}

void MCField::fscroll(Field_translations function, MCStringRef p_string, KeySym key)
{
	// OK-2009-03-19: [[Bug 7667]] - If we are scrolling horizontally and have no horizontal scrollbar, do nothing
	if (!(flags & F_HSCROLLBAR) && (function == FT_SCROLLLEFT || function == FT_SCROLLRIGHT))
		return;

	// OK-2009-03-19: [[Bug 7667]] - If we are scrolling vertically and have no vertical scrollbar, do nothing
	if (!(flags & F_VSCROLLBAR) && (function == FT_SCROLLUP || function == FT_SCROLLDOWN))
		return;

	uint2 fheight;
	fheight = gettextheight();

	int4 newval;
	switch(function)
	{
	case FT_SCROLLLEFT:
	case FT_SCROLLRIGHT:
	case FT_SCROLLUP:
	case FT_SCROLLDOWN:
	{
		bool t_horiz;
		t_horiz = (function == FT_SCROLLLEFT || function == FT_SCROLLRIGHT);

		if (MCmodifierstate & MS_SHIFT)
			t_horiz = !t_horiz;

		int2 direction = function == FT_SCROLLUP || function == FT_SCROLLLEFT ? 1 : -1;

		if (t_horiz)
		{
			hscroll(direction * getfwidth() * 15 / 100, True);
			newval = textx;
		}
		else
		{
			vscroll(direction * getfheight() * 15 / 100, True);
			newval = texty;
		}
	}
	break;
	case FT_SCROLLTOP:
		vscroll(-texty, True);
		newval = texty;
	break;
	case FT_SCROLLPAGEUP:
		vscroll(-(getfheight() - fheight), True);
		newval = texty;
	break;
	case FT_SCROLLBOTTOM:
		vscroll(textheight - texty, True);
		newval = texty;
	break;
	case FT_SCROLLPAGEDOWN:
		vscroll(getfheight() - fheight, True);
		newval = texty;
	break;
    default:
        // If no scroll action is given, then scroll hasn't changed so do nothing.
        return;
	}
	resetscrollbars(True);
	message_with_args(MCM_scrollbar_drag, newval);
}

void MCField::fmove(Field_translations function, MCStringRef p_string, KeySym key)
{
	removecursor();

	uint2 fheight;
	fheight = gettextheight();

	if (flags & F_LIST_BEHAVIOR)
	{
		int2 ty = focusedy;
		switch (function)
		{
			case FT_PAGEUP:
				ty -= getfheight() - fheight;
				break;
			case FT_PAGEDOWN:
				ty += getfheight() - fheight;
				break;
			case FT_HOME:
				ty = fheight;
				break;
			case FT_END:
				ty = getfheight() - fheight;
				break;
			case FT_UP:
			case FT_LEFTCHAR:
				ty -= 8;
				break;
			case FT_DOWN:
			case FT_RIGHTCHAR:
				ty += focusedparagraph->getheight(fixedheight) + 4;
				break;
			default:
				return;
		}
		unselect(False, True);
		// MW-2012-01-25: [[ FieldMetrics ]] Make sure we pass in card coords.
		startselection(getcontentx(), getcontenty() + ty, False);
		state &= ~CS_SELECTING;
		extend = extendwords = extendlines = False;
		contiguous = True;
		replacecursor(True, False);
		signallisteners(P_HILITED_LINES);
		message(MCM_selection_changed);
		return;
	}
	
	if (MCmodifierstate & MS_SHIFT)
	{
		state |= CS_SELECTING;
		extend = True;
		if (firstparagraph == NULL)
		{
			firstparagraph = lastparagraph = focusedparagraph;
			firsty = focusedy;
		}
	}
	else if ((function == FT_LEFTCHAR || function == FT_RIGHTCHAR)
				&& focusedparagraph->isselection())
    {
        findex_t si, ei;
		selectedmark(False, si, ei, False);
		unselect(False, True);
		if (function == FT_LEFTCHAR)
			seltext(si, si, False);
		else
			seltext(ei, ei, False);
		function = FT_UNDEFINED;
	}
	else
		unselect(False, True);

	// MW-2012-01-25: [[ ParaStyles ]] Fetch the cursor rect including any space above
	//   and below.
	MCRectangle drect = focusedparagraph->getcursorrect(-1, fixedheight, true);
	switch (function)
	{
		case FT_UP:
			drect.y -= 4;
			drect.x = goalx;
			break;
		case FT_DOWN:
			drect.y += drect.height + 4;
			drect.x = goalx;
			break;
		case FT_PAGEUP:
			drect.y -= getfheight() - fheight;
			break;
		case FT_PAGEDOWN:
			drect.y += getfheight() - fheight;
			break;
		case FT_HOME:
		case FT_BOL:
			drect.x = -textx;
			if (indent < 0)
				drect.x += indent;
			break;
		case FT_END:
		case FT_EOL:
			drect.x = textwidth + leftmargin + indent + rect.width;
			break;
		case FT_BOF:
			drect.x = -textx - rect.width;
			drect.y = -texty - rect.height;
			break;
		case FT_EOF:
			drect.x = textwidth + leftmargin;
			drect.y = textheight + topmargin;
			break;
		default:
			uint1 moved = focusedparagraph->fmovefocus(function);
			// MW-2012-01-25: [[ ParaStyles ]] Fetch the cursor rect including any space.
			drect = focusedparagraph->getcursorrect(-1, fixedheight, true);
			MCParagraph *tptr;
			MCRectangle trect;
			switch (moved)
			{
				case FT_BACKCHAR:
				case FT_BACKWORD:
				case FT_BOS:
				case FT_LEFTPARA:
					if (focusedparagraph != paragraphs)
					{
						tptr = focusedparagraph->prev();
						tptr->fmovefocus(FT_RIGHTPARA);
                        // AL_2014-07-29: [[ Bug 12896 ]] FT_LEFTCHAR is now FT_BACKCHAR here
						if (moved != FT_BACKCHAR)
							tptr->fmovefocus((Field_translations)moved);
						// MW-2012-01-25: [[ ParaStyles ]] Fetch the cursor rect including any space.
						trect = tptr->getcursorrect(-1, fixedheight, true);
						drect.y -= tptr->getheight(fixedheight) - trect.y;
						drect.x = trect.x;
					}
					break;
				case FT_FORWARDCHAR:
				case FT_FORWARDWORD:
				case FT_EOS:
				case FT_RIGHTPARA:
					if (focusedparagraph != paragraphs->prev())
					{
						// MW-2012-01-25: [[ ParaStyles ]] Fetch the cursor rect including any space.
						trect = focusedparagraph->getcursorrect(-1, fixedheight, true);
						drect.y += focusedparagraph->getheight(fixedheight) - trect.y;
						tptr = focusedparagraph->next();
						tptr->fmovefocus(FT_LEFTPARA);
                        // AL_2014-07-29: [[ Bug 12896 ]] FT_RIGHTCHAR is now FT_FORWARDCHAR here
						if (moved != FT_FORWARDCHAR)
							tptr->fmovefocus((Field_translations)moved);

						// MW-2012-01-25: [[ ParaStyles ]] Fetch the cursor rect including any space.
						trect = tptr->getcursorrect(-1, fixedheight, true);
						drect.y += trect.y;
						drect.x = trect.x;
					}
					break;
				default:
					break;
			}
				break;
	}
	// MW-2012-01-25: [[ FieldMetrics ]] Convert the cursor rect to card co-ords.
	drect.y += getcontenty() + focusedy;
	drect.x += getcontentx();
	setfocus(drect.x, drect.y);
	replacecursor(True, function != FT_UP && function != FT_DOWN);
	
	// PM-2015-07-20: [[ Bug 7217 ]] Send selectionChanged on arrow navigation, regardless of whether Shift key is held down
	if (state & CS_SELECTING)
	{
		state &= ~CS_SELECTING;
		MCundos->freestate();
	}
	
	signallisteners(P_HILITED_LINES);
	message(MCM_selection_changed);
	
	extend = extendwords = extendlines = False;
	contiguous = True;
}

void MCField::setupmenu(MCStringRef p_string, uint2 fheight, Boolean scrolling)
{
	flags = F_VISIBLE | F_SHOW_BORDER | F_ALIGN_LEFT
	| F_TRAVERSAL_ON | F_F_AUTO_ARM | F_SHARED_TEXT | F_FIXED_HEIGHT
	| F_LOCK_TEXT | F_DONT_WRAP | F_LIST_BEHAVIOR | F_OPAQUE | F_3D;
	if (scrolling)
	{
		Boolean dirty;
		setsbprop(P_VSCROLLBAR, true, 0, 0,
							scrollbarwidth, hscrollbar, vscrollbar, dirty);
	}
	fontheight = fheight;
	topmargin = bottommargin = 6;
	borderwidth = 0;
	settext(0, p_string, False);

	// MW-2008-03-14: [[ Bug 5750 ]] Fix to focus border problem in fields used as menu lists in
	//   (for example) option menus. Set this as a menufield.
	setstate(True, CS_MENUFIELD);
}

void MCField::setupentry(MCButton *bptr, MCStringRef p_string)
{
	parent = bptr;
	obj_id = bptr->getid();
	setname(bptr -> getname());
	borderwidth = 0;
	if (MClook == LF_WIN95)
	{
		if (MCFontGetAscent(bptr->getfontref()) < 11)
			topmargin = 9;
		else
			topmargin = 5;
		leftmargin = 4;
	}
	else
		topmargin = 6;
	flags = F_VISIBLE | F_SHOW_BORDER | F_3D | F_OPAQUE | F_FIXED_HEIGHT
		| F_TRAVERSAL_ON | F_AUTO_TAB | F_DONT_WRAP | F_SHARED_TEXT;
	settext(0, p_string, False);
}

// MW-2014-05-21: [[ Bug 11878 ]] Operate on a c-string copy of newtext as the caller
//   owns it.
void MCField::typetext(MCStringRef newtext)
{
	if (MCStringIsEmpty(newtext))
		return;
    
	if (MCactivefield == this)
		unselect(False, True);
	
	MCAutoStringRef t_remaining;
	/* UNCHECKED */ MCStringCreateMutable(0, &t_remaining);
	if (MCStringGetLength(newtext) < MAX_PASTE_MESSAGES)
	{
		uindex_t t_index = 0;
		uindex_t t_length = MCStringGetLength(newtext);
		while (t_index < t_length)
		{
			// Send the next character in the buffer as a key down event
			MCAutoStringRef t_string;
			/* UNCHECKED */ MCStringCopySubstring(newtext, MCRangeMake(t_index, 1), &t_string);
			if (message_with_valueref_args(MCM_key_down, *t_string) != ES_NORMAL)
			{
				// Nothing responded to the key; keep it as text
				/* UNCHECKED */ MCStringAppendChar(*t_remaining, MCStringGetCharAtIndex(newtext, t_index));
			}
			
			// Key up event then move on
			message_with_valueref_args(MCM_key_up, *t_string);
			t_index++;
		}	
		
		// Only the non-handled keypresses should be processed further
        newtext = *t_remaining;
	}
	findex_t oldfocused;
    focusedparagraph->getselectionindex(oldfocused, oldfocused);
    state |= CS_CHANGED;

    if (!MCStringIsEmpty(newtext) && focusedparagraph->finsertnew(newtext))
	{
		recompute();
        findex_t endindex = oldfocused + MCStringGetLength(newtext);
        findex_t junk;
		MCParagraph *newfocused = indextoparagraph(focusedparagraph, endindex, junk);
		while (focusedparagraph != newfocused)
		{
			focusedy += focusedparagraph->getheight(fixedheight);
			focusedparagraph = focusedparagraph->next();
		}

		// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
		layer_redrawall();
	}
	else
		updateparagraph(True, False);
}

void MCField::startcomposition()
{
	if (composing)
		return;
	composing = True;
	composelength = 0;
}

void MCField::setcompositioncursoroffset(findex_t coffset)
{
	composecursorindex = coffset;
}

bool MCField::getcompositionrange(findex_t& si, findex_t& ei)
{
	if (!composing)
		return false;
	
	si = composeoffset;
	ei = si + composelength;

	return true;
}

void MCField::setcompositionconvertingrange(findex_t si, findex_t ei)
{
	composeconvertingsi = si;
	composeconvertingei = ei;
}

void MCField::stopcomposition(Boolean del,Boolean force)
{
	if (!composing)
		return;
	if (force)
	{
		MCModeConfigureIme(getstack(), false, 0, 0);
	}
	if (!composing)
		return;
	if (del)
	{
		removecursor();
		deletecomposition();
		updateparagraph(True, False);
		replacecursor(True, True);
	}
	composelength = 0;
    // MW-2014-08-18: [[ Bug 13196 ]] Make sure we reset the compose cursor offset.
    composecursorindex = 0;
	composing = False;
}

void MCField::deletecomposition()
{
	if (!composing)
		return;
	if (composelength)
	{
		findex_t composesi, composeei;
		composesi = composeoffset;
		composeei = composeoffset+composelength;
		MCParagraph *pgptr = indextoparagraph(paragraphs, composesi, composeei);
		pgptr->deletestring(composesi,composeei);
		state |= CS_CHANGED;
	}
	composelength = 0;
    
    // MW-2014-08-18: [[ Bug 13196 ]] Make sure we reset the compose cursor offset.
    composecursorindex = 0;
}

Boolean MCField::getcompositionrect(MCRectangle &r, findex_t offset)
{
	findex_t si,ei;
	if (!composing)
		return False;
	MCParagraph *pgptr = NULL;
	if (composelength)
	{
		if (offset > composelength)
			return False;
		if (offset == -1)
			offset = composecursorindex;
		si = composeoffset+offset;
		ei = composeoffset+composelength;
		pgptr = indextoparagraph(paragraphs,si,ei);
		// MW-2012-01-25: [[ ParaStyles ]] Fetch the cursor rect ignoring any space.
		r = pgptr->getcursorrect(si, fixedheight, false);
	}
	else
	{
		if (!focusedparagraph)
			return False;
		pgptr = focusedparagraph;
		// MW-2012-01-25: [[ ParaStyles ]] Fetch the cursor rect ignoring any space.
		r = pgptr->getcursorrect(-1, fixedheight, false);
	}

	// MW-2012-01-25: [[ FieldMetrics ]] Conver the rect to card coords.
	r.x += getcontentx();
	r.y += getcontenty() + paragraphtoy(pgptr);
	return True;
}

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

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "mcio.h"
#include "sysdefs.h"

#include "globals.h"
#include "object.h"
#include "stack.h"
#include "cdata.h"
#include "objptr.h"
#include "field.h"
#include "object.h"
#include "button.h"
#include "card.h"
#include "exec.h"
#include "util.h"
#include "group.h"
#include "image.h"
#include "menuparse.h"
#include "stacklst.h"
#include "font.h"
#include "mode.h"
#include "scrolbar.h"
#include "paragraf.h"

#include "exec-interface.h"
////////////////////////////////////////////////////////////////////////////////

enum MCInterfaceFieldStyle
{
	kMCFieldStyleScrolling,
	kMCFieldStyleRectangle,
	kMCFieldStyleTransparent,
	kMCFieldStyleShadow,
	kMCFieldStyleShowBorder,
};

//////////

static MCExecEnumTypeElementInfo _kMCInterfaceFieldStyleElementInfo[] =
{	
	{ MCscrollingstring, kMCFieldStyleScrolling, false },
	{ MCrectanglestring, kMCFieldStyleRectangle, false },
	{ MCtransparentstring, kMCFieldStyleTransparent, false },
	{ MCshadowstring, kMCFieldStyleShadow, false },
	{ MCopaquestring, kMCFieldStyleShowBorder, false },
};

static MCExecEnumTypeInfo _kMCInterfaceFieldStyleTypeInfo =
{
	"Interface.FieldStyle",
	sizeof(_kMCInterfaceFieldStyleElementInfo) / sizeof(MCExecEnumTypeElementInfo),
	_kMCInterfaceFieldStyleElementInfo
};

//////////

static void MCInterfaceFlaggedRangesParse(MCExecContext& ctxt, MCStringRef p_input, MCInterfaceFlaggedRanges& r_output)
{
    uindex_t t_length;
	t_length = MCStringGetLength(p_input);
    
    if (t_length == 0)
    {
        r_output . count = 0;
        return;
    }
    
	MCAutoArray<MCInterfaceFlaggedRange> t_list;
    
    bool t_success;
    t_success = true;
    
	uindex_t t_old_offset;
	t_old_offset = 0;
	uindex_t t_new_offset;
	t_new_offset = 0;
	
	while (t_success && t_old_offset <= t_length)
	{
		MCAutoStringRef t_uintx2_string;
		MCInterfaceFlaggedRange t_range;
		
		if (!MCStringFirstIndexOfChar(p_input, '\n', t_old_offset, kMCCompareCaseless, t_new_offset))
			t_new_offset = t_length;
		
        if (t_new_offset <= t_old_offset)
            break;
        
		if (t_success)
            t_success = MCStringCopySubstring(p_input, MCRangeMake(t_old_offset, t_new_offset - t_old_offset), &t_uintx2_string);
		
		if (t_success)
			t_success = MCU_stoui4x2(*t_uintx2_string, t_range . start, t_range . end);
		
		if (t_success)
			t_success = t_list . Push(t_range);
		
		t_old_offset = t_new_offset + 1;
	}
	
	if (t_success)
    {
        t_list . Take(r_output . ranges, r_output . count);
        return;
    }
    
	ctxt . Throw();
}

static void MCInterfaceFlaggedRangesFormat(MCExecContext& ctxt, const MCInterfaceFlaggedRanges& p_input, MCStringRef& r_output)
{
    if (p_input . count == 0)
    {
        r_output = MCValueRetain(kMCEmptyString);
        return;
    }
    
	MCAutoStringRef t_list;
	bool t_success;
	t_success = MCStringCreateMutable(0, &t_list);
	
	for (uindex_t i = 0; i < p_input . count && t_success; i++)
	{
        if (t_success && i != 0)
			t_success = MCStringAppendNativeChar(*t_list, '\n');
        
		t_success = MCStringAppendFormat(*t_list, "%d,%d", p_input . ranges[i] . start, p_input . ranges[i] . end);
	}
	
	if (t_success)
        t_success = MCStringCopy(*t_list, r_output);
	
	if (t_success)
        return;
    
    ctxt . Throw();
}

static void MCInterfaceFlaggedRangesFree(MCExecContext& ctxt, MCInterfaceMargins& p_input)
{
}

static MCExecCustomTypeInfo _kMCInterfaceFlaggedRangesTypeInfo =
{
	"Interface.FlaggedRanges",
	sizeof(MCInterfaceFlaggedRanges),
	(void *)MCInterfaceFlaggedRangesParse,
	(void *)MCInterfaceFlaggedRangesFormat,
	(void *)MCInterfaceFlaggedRangesFree
};

//////////

static MCExecEnumTypeElementInfo _kMCInterfaceLayerModeElementInfo[] =
{
	{ "static", kMCLayerModeHintStatic },
    { "dynamic", kMCLayerModeHintDynamic },
	{ "scrolling", kMCLayerModeHintScrolling },
	{ "container", kMCLayerModeHintContainer }
};

static MCExecEnumTypeInfo _kMCInterfaceLayerModeTypeInfo =
{
	"Interface.LayerMode",
	sizeof(_kMCInterfaceLayerModeElementInfo) / sizeof(MCExecEnumTypeElementInfo),
	_kMCInterfaceLayerModeElementInfo
};

//////////

MCExecEnumTypeInfo *kMCInterfaceFieldStyleTypeInfo = &_kMCInterfaceFieldStyleTypeInfo;
MCExecCustomTypeInfo *kMCInterfaceFlaggedRangesTypeInfo = &_kMCInterfaceFlaggedRangesTypeInfo;

////////////////////////////////////////////////////////////////////////////////

void MCField::Redraw(bool reset, int4 xoffset, int4 yoffset)
{
	if (!opened)
		return;

	recompute();
	if (reset)
		resetparagraphs();
	hscroll(xoffset, False);
	vscroll(yoffset, False);
	resetscrollbars(True);

	// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
	layer_redrawall();
}

void MCField::UpdateScrollbars()
{
    if (vscrollbar != NULL)
    {
        vscrollbar->setflag(flags & F_3D, F_3D);
        vscrollbar->setflag(flags & F_OPAQUE, F_OPAQUE);
        vscrollbar->setflag(flags & F_DISABLED, F_DISABLED);
    }
    if (hscrollbar != NULL)
    {
        hscrollbar->setflag(flags & F_3D, F_3D);
        hscrollbar->setflag(flags & F_OPAQUE, F_OPAQUE);
        hscrollbar->setflag(flags & F_DISABLED, F_DISABLED);
    }
}

////////////////////////////////////////////////////////////////////////////////

void MCField::GetAutoTab(MCExecContext& ctxt, bool& r_setting)
{
	r_setting = getflag(F_AUTO_TAB);
}

void MCField::SetAutoTab(MCExecContext& ctxt, bool setting)
{
	changeflag(setting, F_AUTO_TAB);
}

void MCField::GetDontSearch(MCExecContext& ctxt, bool& r_setting)
{
	r_setting = getflag(F_F_DONT_SEARCH);
}

void MCField::SetDontSearch(MCExecContext& ctxt, bool setting)
{
	changeflag(setting, F_F_DONT_SEARCH);
}

void MCField::GetDontWrap(MCExecContext& ctxt, bool& r_setting)
{
	r_setting = getflag(F_DONT_WRAP);
}

void MCField::SetDontWrap(MCExecContext& ctxt, bool setting)
{
	bool t_dirty;
	t_dirty = changeflag(setting, F_DONT_WRAP);

	if (flags & (F_LIST_BEHAVIOR | F_VGRID))
		flags |= F_DONT_WRAP;
	
	if (t_dirty)
		Redraw(true);
}

void MCField::GetFixedHeight(MCExecContext& ctxt, bool& r_setting)
{
	r_setting = getflag(F_FIXED_HEIGHT);
}

void MCField::SetFixedHeight(MCExecContext& ctxt, bool setting)
{
	// MW-2012-12-25: [[ Bug ]] Changing the fixedHeight requires a recalculation.
	if (changeflag(setting, F_FIXED_HEIGHT))
		Redraw(true);
}

void MCField::GetLockText(MCExecContext& ctxt, bool& r_setting)
{
	r_setting = getflag(F_LOCK_TEXT);
}

void MCField::DoSetInputControl(MCExecContext& ctxt, Properties which, bool setting)
{
    uint4 tflags = flags;
	bool t_dirty = false;
	
	if (setting != ((tflags & (which == P_LOCK_TEXT ? F_LOCK_TEXT : F_TRAVERSAL_ON)) == True))
	{
		if (setting)
			tflags |= (which == P_LOCK_TEXT ? F_LOCK_TEXT : F_TRAVERSAL_ON);
		else
			tflags &= ~(which == P_LOCK_TEXT ? F_LOCK_TEXT : F_TRAVERSAL_ON);
		t_dirty = true;
	}
    
	if (flags & F_LIST_BEHAVIOR)
	{
		flags |= F_LOCK_TEXT;
		t_dirty = true;
	}
    
	if (t_dirty)
	{
		bool t_was_locked;
		t_was_locked = (flags & F_LOCK_TEXT) != 0;
		flags = tflags;
		if (state & CS_IBEAM)
		{
			state &= ~CS_IBEAM;
			getstack()->clearibeam();
		}
		if (state & CS_KFOCUSED && !(flags & F_TRAVERSAL_ON))
		{
			unselect(True, True);
			getcard()->kfocusset(NULL);
		}
		
		// MW-2011-09-28: [[ Bug 9610 ]] If the lockText has changed then make sure
		//   keyboard state is in sync.
		if (t_was_locked != (getflag(F_LOCK_TEXT) == True) && getstate(CS_KFOCUSED))
			MCModeActivateIme(getstack(), !getflag(F_LOCK_TEXT));
		
		if (vscrollbar != NULL)
			vscrollbar->setflag(False, F_TRAVERSAL_ON);
		if (hscrollbar != NULL)
			hscrollbar->setflag(False, F_TRAVERSAL_ON);
        
		Redraw();
	}
}

void MCField::SetLockText(MCExecContext& ctxt, bool setting)
{
    DoSetInputControl(ctxt, P_LOCK_TEXT, setting);
}

void MCField::SetTraversalOn(MCExecContext& ctxt, bool setting)
{
    DoSetInputControl(ctxt, P_TRAVERSAL_ON, setting);
}

void MCField::GetSharedText(MCExecContext& ctxt, bool& r_setting)
{
	r_setting = getflag(F_SHARED_TEXT);
}

void MCField::SetSharedText(MCExecContext& ctxt, bool setting)
{
	if (changeflag(setting, F_SHARED_TEXT) && opened)
	{
		MCCdata *fptr;
		if (flags & F_SHARED_TEXT)
			fptr = getcarddata(fdata, 0, True);
		else
			fptr = getcarddata(fdata, getcard()->getid(), True);
		MCParagraph *pgptr = fptr->getparagraphs();
		fdata->setparagraphs(pgptr);
		fdata = fptr;
		fdata->setparagraphs(paragraphs);
		Redraw(true);
	}
}

void MCField::GetShowLines(MCExecContext& ctxt, bool& r_setting)
{
	r_setting = getflag(F_SHOW_LINES);
}

void MCField::SetShowLines(MCExecContext& ctxt, bool setting)
{
	if (changeflag(setting, F_SHOW_LINES))
		Redraw();
}

void MCField::GetHGrid(MCExecContext& ctxt, bool& r_setting)
{
	r_setting = getflag(F_LOCK_TEXT);
}

void MCField::SetHGrid(MCExecContext& ctxt, bool setting)
{
	// MW-2012-12-25: [[ Bug ]] Changing the hGrid requires a recalculation.
	if (changeflag(setting, F_SHOW_LINES))
		Redraw(true);
}

void MCField::GetVGrid(MCExecContext& ctxt, bool& r_setting)
{
	r_setting = getflag(F_LOCK_TEXT);
}

void MCField::SetVGrid(MCExecContext& ctxt, bool setting)
{
	bool t_dirty;
	t_dirty = changeflag(setting, F_SHOW_LINES);

	if (flags & F_VGRID)
		flags |= F_DONT_WRAP;

	// MW-2012-12-25: [[ Bug ]] Changing the vGrid requires a recalculation.

	if (t_dirty)
		Redraw(true);
}

void MCField::GetStyle(MCExecContext& ctxt, intenum_t& r_style)
{
	if (flags & F_VSCROLLBAR)
		r_style = kMCFieldStyleScrolling;
	else if (!(flags & F_OPAQUE))
		r_style = kMCFieldStyleTransparent;
	else if (flags & F_SHADOW)
		r_style = kMCFieldStyleShadow;
	else if (!(flags & F_SHOW_BORDER))
		r_style = kMCFieldStyleShowBorder;
	else
		r_style = kMCFieldStyleRectangle;
}

void MCField::SetStyle(MCExecContext& ctxt, intenum_t p_style)
{
	uint4 tflags = flags;

	flags &= ~(F_DISPLAY_STYLE);
	if (p_style == kMCFieldStyleScrolling)
	{
		Boolean dummy;
		if (!(flags & F_VSCROLLBAR))
			setsbprop(P_VSCROLLBAR, true, textx, texty,
			          scrollbarwidth, hscrollbar, vscrollbar, dummy);
		flags |= F_SHOW_BORDER | F_OPAQUE;
	}
	else
	{
		if (flags & F_HSCROLLBAR)
		{
			delete hscrollbar;
			hscrollbar = NULL;
			flags &= ~F_HSCROLLBAR;
		}
		if (flags & F_VSCROLLBAR)
		{
			delete vscrollbar;
			vscrollbar = NULL;
			flags &= ~F_VSCROLLBAR;
		}
		switch (p_style)
		{
		case kMCFieldStyleTransparent:
			flags |= F_3D;
			break;
		case kMCFieldStyleShadow:
			flags |= F_SHOW_BORDER | F_OPAQUE | F_SHADOW;
			break;
		case kMCFieldStyleShowBorder:
			flags |= F_OPAQUE;
			break;
		default:
			flags |= F_SHOW_BORDER | F_OPAQUE;
			break;
		}
	}
	if (flags != tflags)
	{
		// MW-2011-09-21: [[ Layers ]] Make sure we recompute the layer attrs since
		//   various props have changed!
		m_layer_attr_changed = true;
		Redraw(true);
	}
}

void MCField::GetAutoHilite(MCExecContext& ctxt, bool& r_setting)
{
	r_setting = getflag(F_AUTO_HILITE);
}

void MCField::SetAutoHilite(MCExecContext& ctxt, bool setting)
{
	changeflag(setting, F_NO_AUTO_HILITE);
	flags ^= F_NO_AUTO_HILITE;
}

void MCField::GetAutoArm(MCExecContext& ctxt, bool& r_setting)
{
	r_setting = getflag(F_F_AUTO_ARM);
}

void MCField::SetAutoArm(MCExecContext& ctxt, bool setting)
{
	changeflag(setting, F_F_AUTO_ARM);
}

void MCField::GetFirstIndent(MCExecContext& ctxt, integer_t& r_indent)
{
	r_indent = indent;
}

void MCField::SetFirstIndent(MCExecContext& ctxt, integer_t p_indent)
{
	indent = p_indent;
	Redraw();
}

void MCField::GetWideMargins(MCExecContext& ctxt, bool& r_setting)
{
	r_setting = leftmargin >= widemargin;
}

void MCField::SetWideMargins(MCExecContext& ctxt, bool setting)
{
	if (setting)
	{
		if (leftmargin != widemargin)
		{
			leftmargin = rightmargin = topmargin = bottommargin = widemargin;
			Redraw();
		}
	}
	else
	{
		if (leftmargin != narrowmargin)
		{
			leftmargin = rightmargin = topmargin = bottommargin = narrowmargin;
			Redraw();
		}
	}
}

void MCField::GetHScroll(MCExecContext& ctxt, integer_t& r_scroll)
{
	r_scroll = textx;
}

void MCField::SetHScroll(MCExecContext& ctxt, integer_t scroll)
{
	DoSetHScroll(ctxt, textx, scroll);
}

void MCField::GetVScroll(MCExecContext& ctxt, integer_t& r_scroll)
{
	r_scroll = texty;
}

void MCField::SetVScroll(MCExecContext& ctxt, integer_t scroll)
{
	DoSetVScroll(ctxt, texty, scroll);
}

void MCField::GetHScrollbar(MCExecContext& ctxt, bool& r_setting)
{
	r_setting = getflag(F_HSCROLLBAR);
}

void MCField::SetHScrollbar(MCExecContext& ctxt, bool setting)
{
	if (changeflag(setting, F_HSCROLLBAR))
	{	
		DoSetHScrollbar(ctxt, hscrollbar, scrollbarwidth);
		Redraw();
	}
}

void MCField::GetVScrollbar(MCExecContext& ctxt, bool& r_setting)
{
	r_setting = getflag(F_VSCROLLBAR);
}

void MCField::SetVScrollbar(MCExecContext& ctxt, bool setting)
{
	if (changeflag(setting, F_VSCROLLBAR))
	{	
		DoSetVScrollbar(ctxt, vscrollbar, scrollbarwidth);
		Redraw();
	}
}

void MCField::GetScrollbarWidth(MCExecContext& ctxt, uinteger_t& r_width)
{
	r_width = scrollbarwidth;
}

void MCField::SetScrollbarWidth(MCExecContext& ctxt, uinteger_t p_width)
{
	if (scrollbarwidth != p_width)
	{
		DoSetScrollbarWidth(ctxt, scrollbarwidth, p_width);
		Redraw();
	}
}

void MCField::GetFormattedWidth(MCExecContext& ctxt, uinteger_t& r_width)
{
	if (opened)
	{
		r_width = textwidth + rect.width - getfwidth()
		          + leftmargin + rightmargin
		          + (flags & F_VSCROLLBAR ? (flags & F_DONT_WRAP ? 0 : -vscrollbar->getrect().width) : 0);
	}
	else
		r_width = 0;
}

void MCField::GetFormattedHeight(MCExecContext& ctxt, uinteger_t& r_height)
{
	if (opened)
	{
		r_height = textheight + rect.height - getfheight()
		          + topmargin + bottommargin - TEXT_Y_OFFSET;
	}
	else
		r_height = 0;
}

void MCField::GetListBehavior(MCExecContext& ctxt, bool& r_setting)
{
	r_setting = getflag(F_LIST_BEHAVIOR);
}

void MCField::SetListBehavior(MCExecContext& ctxt, bool setting)
{
	if (opened)
		clearhilites();
	if (changeflag(setting, F_LIST_BEHAVIOR))
	{
		if (flags & F_LIST_BEHAVIOR)
			flags |= F_DONT_WRAP | F_LOCK_TEXT;
		else
			if (state & CS_KFOCUSED)
				MCscreen->addtimer(this, MCM_internal, MCblinkrate);
		Redraw(true);
	}
}

void MCField::GetMultipleHilites(MCExecContext& ctxt, bool& r_setting)
{
	r_setting = getflag(F_MULTIPLE_HILITES);
}

void MCField::SetMultipleHilites(MCExecContext& ctxt, bool setting)
{
	changeflag(setting, F_MULTIPLE_HILITES);
}

void MCField::GetNoncontiguousHilites(MCExecContext& ctxt, bool& r_setting)
{
	r_setting = getflag(F_NONCONTIGUOUS_HILITES);
}

void MCField::SetNoncontiguousHilites(MCExecContext& ctxt, bool setting)
{
	changeflag(setting, F_NONCONTIGUOUS_HILITES);
}

void MCField::GetText(MCExecContext& ctxt, uint32_t part, MCStringRef& r_text)
{
	if (exportastext(part, 0, INT32_MAX, r_text))
		return;

	ctxt . Throw();
}

void MCField::SetText(MCExecContext& ctxt, uint32_t part, MCStringRef p_text)
{
	settext(part, p_text, False);
}

void MCField::GetUnicodeText(MCExecContext& ctxt, uint32_t part, MCDataRef& r_text)
{
	MCAutoStringRef t_text;
	if (exportastext(part, 0, INT32_MAX, &t_text))
		if (MCStringEncode(*t_text, kMCStringEncodingUTF16, false, r_text))
			return;
	
	ctxt . Throw();
}

void MCField::SetUnicodeText(MCExecContext& ctxt, uint32_t part, MCDataRef p_text)
{
	MCAutoStringRef t_string;
	/* UNCHECKED */ MCStringDecode(p_text, kMCStringEncodingUTF16, false, &t_string);
	SetText(ctxt, part, *t_string);
}

void MCField::GetHtmlText(MCExecContext& ctxt, uint32_t part, MCStringRef& r_text)
{
	if (exportashtmltext(part, 0, INT32_MAX, false, r_text))
		return;

	ctxt . Throw();
}

void MCField::SetHtmlText(MCExecContext& ctxt, uint32_t part, MCStringRef p_text)
{
	sethtml(part, MCStringGetOldString(p_text));
}

void MCField::GetEffectiveHtmlText(MCExecContext& ctxt, uint32_t part, MCStringRef& r_text)
{
	if (exportashtmltext(part, 0, INT32_MAX, true, r_text))
		return;

	ctxt . Throw();
}

void MCField::GetRtfText(MCExecContext& ctxt, uint32_t part, MCStringRef& r_text)
{
	if (exportasrtftext(part, 0, INT32_MAX, r_text))
		return;

	ctxt . Throw();
}

void MCField::SetRtfText(MCExecContext& ctxt, uint32_t part, MCStringRef p_text)
{
	setrtf(part, MCStringGetOldString(p_text));
}

void MCField::GetStyledText(MCExecContext& ctxt, uint32_t part, MCArrayRef& r_array)
{
	if (exportasstyledtext(part, 0, INT32_MAX, false, false, r_array))
		return;

	ctxt . Throw();
}

void MCField::SetStyledText(MCExecContext& ctxt, uint32_t part, MCArrayRef p_array)
{
	setstyledtext(part, p_array);
}

void MCField::GetEffectiveStyledText(MCExecContext& ctxt, uint32_t part, MCArrayRef& r_array)
{
	if (exportasstyledtext(part, 0, INT32_MAX, false, true, r_array))
		return;

	ctxt . Throw();
}

void MCField::GetFormattedStyledText(MCExecContext& ctxt, uint32_t part, MCArrayRef& r_array)
{
	if (exportasstyledtext(part, 0, INT32_MAX, true, false, r_array))
		return;

	ctxt . Throw();
}

void MCField::GetEffectiveFormattedStyledText(MCExecContext& ctxt, uint32_t part, MCArrayRef& r_array)
{
	if (exportasstyledtext(part, 0, INT32_MAX, true, true, r_array))
		return;

	ctxt . Throw();
}

void MCField::GetPlainText(MCExecContext& ctxt, uint32_t part, MCStringRef& r_string)
{
	if (exportasplaintext(part, 0, INT32_MAX, r_string))
		return;

	ctxt . Throw();
}

void MCField::GetUnicodePlainText(MCExecContext& ctxt, uint32_t part, MCDataRef& r_string)
{
	MCAutoStringRef t_string;
	GetPlainText(ctxt, part, &t_string);
	if (ctxt . HasError())
		return;
	
	if (MCStringEncode(*t_string, kMCStringEncodingUTF16, false, r_string))
		return;
	
	ctxt . Throw();
}

void MCField::GetFormattedText(MCExecContext& ctxt, uint32_t part, MCStringRef& r_string)
{
	if (exportasformattedtext(part, 0, INT32_MAX, r_string))
		return;

	ctxt . Throw();
}

void MCField::SetFormattedText(MCExecContext& ctxt, uint32_t part, MCStringRef p_string)
{
	settext(part, p_string, True);
	Redraw(true);
}

void MCField::GetUnicodeFormattedText(MCExecContext& ctxt, uint32_t part, MCDataRef& r_string)
{
	MCAutoStringRef t_string;
	GetFormattedText(ctxt, part, &t_string);
	if (ctxt . HasError())
		return;
	
	if (MCStringEncode(*t_string, kMCStringEncodingUTF16, false, r_string))
		return;
	
	ctxt . Throw();
}

void MCField::SetUnicodeFormattedText(MCExecContext& ctxt, uint32_t part, MCDataRef p_string)
{
	MCAutoStringRef t_text;
	if (MCStringDecode(p_string, kMCStringEncodingUTF16, false, &t_text))
	{
		SetFormattedText(ctxt, part, *t_text);
		return;
	}

	ctxt . Throw();
}

void MCField::GetLabel(MCExecContext& ctxt, MCStringRef& r_string)
{
	r_string = MCValueRetain(label);
}

void MCField::SetLabel(MCExecContext& ctxt, MCStringRef p_string)
{
	MCValueAssign(label, p_string);
}

void MCField::GetToggleHilite(MCExecContext& ctxt, bool& r_setting)
{
	r_setting = getflag(F_TOGGLE_HILITE);
}

void MCField::SetToggleHilite(MCExecContext& ctxt, bool setting)
{
	changeflag(setting, F_TOGGLE_HILITE);
}

void MCField::GetThreeDHilite(MCExecContext& ctxt, bool& r_setting)
{
	r_setting = getflag(F_3D_HILITE);
}

void MCField::SetThreeDHilite(MCExecContext& ctxt, bool setting)
{
	if (changeflag(setting, F_3D_HILITE))
		Redraw();
}

void MCField::DoGetTextState(MCExecContext& ctxt, Properties which, uint32_t part, MCInterfaceTriState& r_state)
{
   	bool t_first, t_state;
	t_first = true;
	t_state = false;
    
	int4 t_line_index;
	int4 ei = INT32_MAX;
	int4 si = 0;
	MCParagraph *pgptr = getcarddata(fdata, part, True)->getparagraphs();
	MCParagraph *sptr = indextoparagraph(pgptr, si, ei, &t_line_index);
    
	do
	{
		// Fetch the flag state of the current paragraph, breaking if
		// the state has changed and this is not the first one we look at.
		bool t_new_state;
		t_new_state = false;
		if (!sptr -> getflagstate(which == P_ENCODING ? F_HAS_UNICODE : F_FLAGGED, si, ei, t_new_state) ||
			(!t_first && t_state != t_new_state))
			break;
		
		// If we are the first para, then store the state.
		if (t_first)
		{
			t_state = t_new_state;
			t_first = false;
		}
        
		// Reduce ei until we get to zero, advancing through the paras.
		ei -= sptr->gettextlengthcr();
		sptr = sptr->next();
	}
	while(ei > 0);
	
	if (ei <= 0)
    {
		r_state . type = kMCInterfaceTriStateBoolean;
        r_state . state = t_state;
    }
	else
    {
        r_state . type = kMCInterfaceTriStateMixed;
		r_state . mixed = Mixed;
    }
}

void MCField::GetEncoding(MCExecContext& ctxt, uint32_t part, intenum_t& r_encoding)
{
    MCInterfaceTriState t_state;
    DoGetTextState(ctxt, P_ENCODING, part, t_state);
    if (t_state . type == kMCInterfaceTriStateBoolean)
        r_encoding = t_state . state ? 1 : 0;
    else
        r_encoding = t_state . mixed;
}

void MCField::GetFlagged(MCExecContext& ctxt, uint32_t part, MCInterfaceTriState& r_flagged)
{
    DoGetTextState(ctxt, P_FLAGGED, part, r_flagged);
}

////////////////////////////////////////////////////////////////////////////////////////

void MCField::GetHilitedLines(MCExecContext& ctxt, uindex_t& r_count, uinteger_t*& r_lines)
{
	if (!opened || !(flags & F_LIST_BEHAVIOR))
    {
        r_count = 0;
		return;
    }
    
	uint4 line = 0;
	MCParagraph *pgptr = paragraphs;

    MCAutoArray<uinteger_t> t_lines;
	do
	{
		line++;
		if (pgptr->gethilite())
            t_lines . Push(line);
		pgptr = pgptr->next();
	}
	while (pgptr != paragraphs);
    
    t_lines . Take(r_lines, r_count);
}

void MCField::SetHilitedLines(MCExecContext& ctxt, uindex_t p_count, uinteger_t* p_lines)
{
    if (!opened)
        return;
    if (flags & F_LIST_BEHAVIOR)
        sethilitedlines(p_lines, p_count, True);
    
    // MW-2011-08-18: [[ Layers ]] Just redraw the content (we don't need a recompute).
    MCControl::Redraw();
}

void MCField::GetFlaggedRanges(MCExecContext& ctxt, uint32_t p_part, MCInterfaceFlaggedRanges& r_ranges)
{
    MCAutoArray<MCInterfaceFlaggedRange> t_ranges;
    
    int4 t_line_index, t_char_index, si, ei;
    si = 0;
    ei = INT32_MAX;
    
    if (flags & F_SHARED_TEXT)
		p_part = 0;
	MCParagraph *pgptr = getcarddata(fdata, p_part, True)->getparagraphs();
    MCParagraph *sptr = indextoparagraph(pgptr, si, ei, &t_line_index);
    
    // The ranges are adjusted by the index of the first char (i.e. they are
    // relative to the start of the range.
    int32_t t_index_offset;
    t_index_offset = -countchars(p_part, 0, si);
    
    // Loop through the paragraphs until the range is exhausted.
    do
    {
        MCInterfaceFlaggedRanges t_para_ranges;
        // Fetch the flagged ranges into ep between si and ei (sptr relative)
        // making sure the ranges are adjusted to the start of the range.
        sptr -> getflaggedranges(p_part, si, ei, t_index_offset, t_para_ranges);
        
        for (uindex_t i = 0; i < t_para_ranges . count; i++)
            t_ranges . Push(t_para_ranges . ranges[i]);
        
        // Increment the offset by the size of the paragraph.
        t_index_offset += sptr -> gettextlengthcr();
        
        // Reduce ei until we get to zero, advancing through the paras.
        si = 0;
        ei -= sptr -> gettextlengthcr();
        sptr = sptr -> next();
    }
    while(ei > 0);
    
    t_ranges . Take(r_ranges . ranges, r_ranges . count);
}

void MCField::SetFlaggedRanges(MCExecContext& ctxt, uint32_t p_part, const MCInterfaceFlaggedRanges& p_ranges)
{
    // MW-2012-02-08: [[ FlaggedField ]] Special case the 'flaggedRanges' property.
    int4 t_line_index, t_char_index, si, ei;
    si = 0;
    ei = INT32_MAX;
    
    if (flags & F_SHARED_TEXT)
		p_part = 0;
    
    // First ensure the flagged property is false along the whole range.   
    MCExecPoint ep(nil, nil, nil);
    ep . setboolean(False);
    settextatts(p_part, P_FLAGGED, ep, nil, si, ei, false);
    // All remaining ranges will have flagged set to true.
    ep . setboolean(True);
    
    for (uindex_t i = 0; i < p_ranges . count; i++)
    {
        // MW-2012-02-24: [[ FieldChars ]] Convert char indices to field indices.
        int32_t t_range_start, t_range_end;
        t_range_start = si;
        t_range_end = ei;
        resolvechars(p_part, t_range_start, t_range_end, p_ranges . ranges[i] . start - 1, p_ranges . ranges[i] . end - p_ranges . ranges[i] . start + 1);
        
        // MW-2012-03-23: [[ Bug 10118 ]] Both range_start and range_end are already
        //   offset from the start of the field.
        settextatts(p_part, P_FLAGGED, ep, nil, MCU_max(si, t_range_start), MCU_min(ei, t_range_end), false);
    }
}

void MCField::DoSetTabStops(MCExecContext& ctxt, bool is_relative, uindex_t p_count, uinteger_t *p_tabs)
{
    MCAutoArray<uint2> t_new_tabs;
    
    uint2 *t_new = nil;
    uindex_t t_new_count = 0;
    
    uint2 t_previous_tab_stop;
    t_previous_tab_stop = 0;
    
    for (uindex_t i = 0; i < p_count; i++)
    {
        if (p_tabs[i] > 65535)
        {
            ctxt . LegacyThrow(EE_PROPERTY_NAN);
            return;
        }

        if (is_relative)
        {
            t_new_tabs . Push(p_tabs[i] + t_previous_tab_stop);
            t_previous_tab_stop = t_new_tabs[i];
        }
        else
            t_new_tabs . Push(p_tabs[i]);
    }
    
    t_new_tabs . Take(t_new, t_new_count);
    
    delete tabs;
    
    if (t_new != nil)
    {
        tabs = t_new;
        ntabs = t_new_count;
        flags |= F_TABS;
    }
    else
    {
        tabs = nil;
        ntabs = 0;
        flags &= ~F_TABS;
    }
    
    Redraw(true);
}

void MCField::SetTabStops(MCExecContext& ctxt, uindex_t p_count, uinteger_t *p_tabs)
{
    DoSetTabStops(ctxt, false, p_count, p_tabs);
}

void MCField::GetTabStops(MCExecContext& ctxt, uindex_t& r_count, uinteger_t*& r_tabs)
{
    MCAutoArray<uinteger_t> t_tabs;
    
    for (uindex_t i = 0; i < ntabs; i++)
        t_tabs . Push(tabs[i]);

    t_tabs . Take(r_tabs, r_count);
}

void MCField::SetTabWidths(MCExecContext& ctxt, uindex_t p_count, uinteger_t *p_tabs)
{
    DoSetTabStops(ctxt, true, p_count, p_tabs);
}

void MCField::GetTabWidths(MCExecContext& ctxt, uindex_t& r_count, uinteger_t*& r_tabs)
{
    MCAutoArray<uinteger_t> t_tabs;
    uinteger_t t_previous_tab;
    t_previous_tab = 0;
    
    for (uindex_t i = 0; i < ntabs; i++)
    {
        t_tabs . Push(tabs[i] - t_previous_tab);
        t_previous_tab = tabs[i];
    }
    
    t_tabs . Take(r_tabs, r_count);
}

void MCField::GetPageHeights(MCExecContext& ctxt, uindex_t& r_count, uinteger_t*& r_heights)
{
    MCAutoArray<uinteger_t> t_heights;
    if (opened)
    {
        MCParagraph *pgptr = paragraphs;
        uint2 height = getfheight();
        uint2 theight = height;
        MCLine *lastline = nil;
        uint2 j = 0;
        while (true)
        {
            MCLine *oldlast = lastline;
            if (!pgptr->pageheight(fixedheight, theight, lastline))
            {
                if (theight == height)
                {
                    t_heights . Push(pgptr->getheight(fixedheight));
                    lastline = nil;
                }
                else
                {
                    t_heights . Push(height - theight);
                    theight = height;
                    if (oldlast == nil || lastline != nil)
                        continue;
                }
            }
            pgptr = pgptr->next();
            if (pgptr == paragraphs)
                break;
        }
        if (theight != height)
            t_heights . Push(height - theight);
    }
    
    t_heights . Take(r_heights, r_count);
}

////////////////////////////////////////////////////////////////////////////////////////

void MCField::SetShadow(MCExecContext& ctxt, const MCInterfaceShadow& p_shadow)
{
    MCControl::SetShadow(ctxt, p_shadow);
    Redraw(true);
}

void MCField::SetShowBorder(MCExecContext& ctxt, bool setting)
{
    MCControl::SetShowBorder(ctxt, setting);
    Redraw(true);
}


void MCField::SetTextHeight(MCExecContext& ctxt, uinteger_t* height)
{
    MCObject::SetTextHeight(ctxt, height);
    Redraw(true);
}


void MCField::SetTextFont(MCExecContext& ctxt, MCStringRef font)
{
    MCObject::SetTextFont(ctxt, font);
    Redraw(true);
}


void MCField::SetTextSize(MCExecContext& ctxt, uinteger_t* size)
{
    MCObject::SetTextSize(ctxt, size);
    Redraw(true);
}


void MCField::SetTextStyle(MCExecContext& ctxt, const MCInterfaceTextStyle& p_style)
{
    MCObject::SetTextStyle(ctxt, p_style);
    Redraw(true);
}


void MCField::SetBorderWidth(MCExecContext& ctxt, uinteger_t width)
{
    MCObject::SetBorderWidth(ctxt, width);
    Redraw(true);
}


void MCField::Set3D(MCExecContext& ctxt, bool setting)
{
    MCObject::Set3D(ctxt, setting);
    UpdateScrollbars();
    Redraw(true);
}


void MCField::SetOpaque(MCExecContext& ctxt, bool setting)
{
    MCControl::SetOpaque(ctxt, setting);
    UpdateScrollbars();
}


void MCField::SetEnabled(MCExecContext& ctxt, uint32_t part, bool setting)
{
    MCObject::SetEnabled(ctxt, part, setting);
    UpdateScrollbars();
    Redraw(true);
}


void MCField::SetDisabled(MCExecContext& ctxt, uint32_t part, bool setting)
{
    MCObject::SetDisabled(ctxt, part, setting);
    UpdateScrollbars();
    Redraw(true);
}

void MCField::SetLeftMargin(MCExecContext& ctxt, integer_t p_margin)
{
    MCControl::SetLeftMargin(ctxt, p_margin);
    Redraw(true);
}

void MCField::SetRightMargin(MCExecContext& ctxt, integer_t p_margin)
{
    MCControl::SetRightMargin(ctxt, p_margin);
    Redraw(true);
}

void MCField::SetTopMargin(MCExecContext& ctxt, integer_t p_margin)
{
    MCControl::SetTopMargin(ctxt, p_margin);
    Redraw(true);
}

void MCField::SetBottomMargin(MCExecContext& ctxt, integer_t p_margin)
{
    MCControl::SetBottomMargin(ctxt, p_margin);
    Redraw(true);
}

void MCField::SetMargins(MCExecContext& ctxt, const MCInterfaceMargins& p_margins)
{
    MCControl::SetMargins(ctxt, p_margins);
    Redraw(true);
}

void MCField::SetWidth(MCExecContext& ctxt, uinteger_t value)
{
    MCObject::SetWidth(ctxt, value);
    Redraw(true);
}

void MCField::SetHeight(MCExecContext& ctxt, uinteger_t value)
{
    MCObject::SetHeight(ctxt, value);
    Redraw(true);
}

void MCField::SetEffectiveWidth(MCExecContext& ctxt, uinteger_t value)
{
    MCObject::SetEffectiveWidth(ctxt, value);
    Redraw(true);
}

void MCField::SetEffectiveHeight(MCExecContext& ctxt, uinteger_t value)
{
    MCObject::SetEffectiveHeight(ctxt, value);
    Redraw(true);
}

void MCField::SetRectangle(MCExecContext& ctxt, MCRectangle p_rect)
{
    MCObject::SetRectangle(ctxt, p_rect);
    Redraw(true);
}

void MCField::SetEffectiveRectangle(MCExecContext& ctxt, MCRectangle p_rect)
{
    MCObject::SetEffectiveRectangle(ctxt, p_rect);
    Redraw(true);
}

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

MCExecEnumTypeInfo *kMCInterfaceFieldStyleTypeInfo = &_kMCInterfaceFieldStyleTypeInfo;

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
	if (exportastext(part, 0, INT32_MAX, false, r_text))
		return;

	ctxt . Throw();
}

void MCField::SetText(MCExecContext& ctxt, uint32_t part, MCStringRef p_text)
{
	settext(part, p_text, False);
}

void MCField::GetUnicodeText(MCExecContext& ctxt, uint32_t part, MCStringRef& r_text)
{
	if (exportastext(part, 0, INT32_MAX, true, r_text))
		return;

	ctxt . Throw();
}

void MCField::SetUnicodeText(MCExecContext& ctxt, uint32_t part, MCStringRef p_text)
{
	setpartialtext(part, MCStringGetOldString(p_text), true);
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
	if (exportasplaintext(part, 0, INT32_MAX, false, r_string))
		return;

	ctxt . Throw();
}

void MCField::GetUnicodePlainText(MCExecContext& ctxt, uint32_t part, MCStringRef& r_string)
{
	if (exportasplaintext(part, 0, INT32_MAX, true, r_string))
		return;

	ctxt . Throw();
}

void MCField::GetFormattedText(MCExecContext& ctxt, uint32_t part, MCStringRef& r_string)
{
	if (exportasformattedtext(part, 0, INT32_MAX, false, r_string))
		return;

	ctxt . Throw();
}

void MCField::SetFormattedText(MCExecContext& ctxt, uint32_t part, MCStringRef p_string)
{
	settext(part, p_string, True);
	Redraw(true);
}

void MCField::GetUnicodeFormattedText(MCExecContext& ctxt, uint32_t part, MCStringRef& r_string)
{
	if (exportasformattedtext(part, 0, INT32_MAX, true, r_string))
		return;

	ctxt . Throw();
}

void MCField::SetUnicodeFormattedText(MCExecContext& ctxt, uint32_t part, MCStringRef p_string)
{
	settext(part, p_string, True);
	Redraw(true);
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

void MCField::GetEncoding(MCExecContext& ctxt, uint32_t part, intenum_t& r_encoding)
{

	bool t_first, t_state;
	t_first = true;
	t_state = false;

	int4 t_line_index;
	int4 ei = getpgsize(nil);
	int4 si = 0;
	MCParagraph *pgptr = getcarddata(fdata, part, True)->getparagraphs();
	MCParagraph *sptr = indextoparagraph(pgptr, si, ei, &t_line_index);

	do
	{
		// Fetch the flag state of the current paragraph, breaking if
		// the state has changed and this is not the first one we look at.
		bool t_new_state;
		t_new_state = false;
		if (!sptr -> getflagstate(F_HAS_UNICODE, si, ei, t_new_state) ||
			(!t_first && t_state != t_new_state))
			break;
		
		// If we are the first para, then store the state.
		if (t_first)
		{
			t_state = t_new_state;
			t_first = false;
		}

		// Reduce ei until we get to zero, advancing through the paras.
		ei -= sptr->gettextsizecr();
		sptr = sptr->next();
	}
	while(ei > 0);
	
	if (ei <= 0)
		r_encoding = t_state ? 1 : 0;
	else
		r_encoding = 2;
}

////////////////////////////////////////////////////////////////////////////////////////
// TODO:: list types

void MCField::GetHilitedLines(MCExecContext& ctxt, MCStringRef& r_lines)
{
    MCExecPoint& ep = ctxt . GetEP();
    hilitedlines(ep);
    ep . copyasstringref(r_lines);
}

void MCField::SetHilitedLines(MCExecContext& ctxt, MCStringRef p_lines)
{
    if (!opened)
        return;
    if (flags & F_LIST_BEHAVIOR)
        if (sethilitedlines(MCStringGetOldString(p_lines)) != ES_NORMAL)
        {
            ctxt . Throw();
            return;
        }
    // MW-2011-08-18: [[ Layers ]] Just redraw the content (we don't need a recompute).
    layer_redrawall();
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

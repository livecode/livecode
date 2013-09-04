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
#include "redraw.h"
#include "scrolbar.h"

#include "exec-interface.h"

////////////////////////////////////////////////////////////////////////////////

void MCGroup::SetChildDisabled(MCExecContext& ctxt, uint32_t part, bool setting)
{
	// MW-2011-08-18: [[ Redraw ]] Update to use redraw.
	MCRedrawLockScreen();
	if (controls != NULL)
	{
		MCControl *cptr = controls;
		MCerrorlock++;
		do
		{
			cptr -> SetDisabled(ctxt, part, setting);
			// MM-2012-09-05: [[ Property Listener ]] Make sure any group props which also effect children send the propertyChanged message to listeners of the children.
			cptr -> signallisteners(setting ? P_DISABLED : P_ENABLED);
			cptr = cptr->next();
		}
		while (cptr != controls);
		MCerrorlock--;
	}

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

	MCRedrawUnlockScreen();
}

void MCGroup::UpdateMargins()
{
    if (leftmargin == defaultmargin && rightmargin == defaultmargin
        && topmargin == defaultmargin && bottommargin == defaultmargin)
        flags &= ~F_MARGINS;
    else
        flags |= F_MARGINS;
    
    bool t_dirty;
    t_dirty = computeminrect(False);
    resetscrollbars(False);
    
    if (t_dirty)
        Redraw();
}

////////////////////////////////////////////////////////////////////////////////

void MCGroup::GetCantDelete(MCExecContext& ctxt, bool& r_setting)
{
	r_setting = getflag(F_G_CANT_DELETE);
}

void MCGroup::SetCantDelete(MCExecContext& ctxt, bool setting)
{
	if (changeflag(setting, F_G_CANT_DELETE))
		Redraw();
}

void MCGroup::GetDontSearch(MCExecContext& ctxt, bool& r_setting)
{
	r_setting = getflag(F_G_DONT_SEARCH);
}

void MCGroup::SetDontSearch(MCExecContext& ctxt, bool setting)
{
	if (changeflag(setting, F_G_DONT_SEARCH))
		Redraw();
}

void MCGroup::GetShowPict(MCExecContext& ctxt, bool& r_setting)
{
	r_setting = true;
}

void MCGroup::SetShowPict(MCExecContext& ctxt, bool setting)
{
	//NO OP
}

void MCGroup::GetRadioBehavior(MCExecContext& ctxt, uint32_t part, bool& r_setting)
{
	r_setting = getflag(F_RADIO_BEHAVIOR);
}

void MCGroup::SetRadioBehavior(MCExecContext& ctxt, uint32_t part, bool setting)
{
	bool t_dirty;
	t_dirty = changeflag(setting, F_RADIO_BEHAVIOR);

	if (flags & F_RADIO_BEHAVIOR)
		flags |= F_TAB_GROUP_BEHAVIOR;
	radio(part, kfocused);
	radio(part, mfocused);

	if (t_dirty)
		Redraw();
}

void MCGroup::GetTabGroupBehavior(MCExecContext& ctxt, bool& r_setting)
{
	r_setting = getflag(F_TAB_GROUP_BEHAVIOR);
}

void MCGroup::SetTabGroupBehavior(MCExecContext& ctxt, bool setting)
{
	if (changeflag(setting, F_TAB_GROUP_BEHAVIOR))
		Redraw();
}

void MCGroup::GetHilitedButton(MCExecContext& ctxt, uint32_t part, integer_t& r_button)
{
	if (controls != NULL)
	{
		MCControl *cptr = controls;
		uint2 which = 1;
		do
		{
			if (cptr->gettype() == CT_BUTTON)
			{
				MCButton *bptr = (MCButton *)cptr;
				if (!(mgrabbed == True && cptr == mfocused)
				        && bptr->gethilite(part))
					break;
				which++;
			}
			cptr = cptr->next();
		}
		while (cptr != controls);
		r_button = (integer_t)which;
	}
	else
		r_button = 0;
}
void MCGroup::SetHilitedButton(MCExecContext& ctxt, uint32_t part, integer_t p_button)
{
	if (controls != NULL)
	{
		MCControl *cptr = controls;
		uint2 which = 1;
		do
		{
			if (cptr->gettype() == CT_BUTTON)
			{
				MCButton *bptr = (MCButton *)cptr;
				bptr->resethilite(part, p_button == (integer_t)which);
				which++;
			}
			cptr = cptr->next();
		}
		while (cptr != controls);
	}
}

void MCGroup::GetHilitedButtonId(MCExecContext& ctxt, uint32_t part, integer_t& r_id)
{
	MCButton *bptr = gethilitedbutton(part);
	if (bptr != NULL)
		r_id = bptr->getid();
	else
		r_id = 0;
}

void MCGroup::SetHilitedButtonId(MCExecContext& ctxt, uint32_t part, integer_t p_id)
{
	if (controls != NULL)
	{
		MCControl *cptr = controls;
		do
		{
			if (cptr->gettype() == CT_BUTTON)
			{
				MCButton *bptr = (MCButton *)cptr;
				bptr->resethilite(part, p_id == bptr->getid());
			}
			cptr = (MCControl *)cptr->next();
		}
		while (cptr != controls);
	}
}

void MCGroup::GetHilitedButtonName(MCExecContext& ctxt, uint32_t part, MCStringRef& r_name)
{
	MCButton *bptr = gethilitedbutton(part);
	if (bptr != NULL)
		r_name = MCValueRetain(MCNameGetString(bptr->getname()));
	else
		r_name = kMCEmptyString;
}

void MCGroup::SetHilitedButtonName(MCExecContext& ctxt, uint32_t part, MCStringRef p_name)
{
	if (controls != NULL)
	{
		MCControl *cptr = controls;
		do
		{
			if (cptr->gettype() == CT_BUTTON)
			{
				MCButton *bptr = (MCButton *)cptr;
				bptr->resethilite(part, bptr->hasname(MCNameLookup(p_name)));
			}
			cptr = cptr->next();
		}
		while (cptr != controls);
	}
}

void MCGroup::GetShowName(MCExecContext& ctxt, bool& r_setting)
{
	r_setting = getflag(F_SHOW_NAME);
}

void MCGroup::SetShowName(MCExecContext& ctxt, bool setting)
{
	if (changeflag(setting, F_SHOW_NAME))
	{
		// MW-2011-09-21: [[ Layers ]] Changing the showName property
		//   affects the layer attrs.
		m_layer_attr_changed = true;

		if (computeminrect(False))
			Redraw();
	}
}

void MCGroup::DoGetLabel(MCExecContext& ctxt, bool to_unicode, MCStringRef r_string)
{
	// Fetch the label, taking note of its encoding.
	MCString slabel;
	bool is_unicode;

	if (label == NULL)
	{
		r_string = MCValueRetain(kMCEmptyString);
		return;
	}

	slabel.set(label,labelsize), is_unicode = hasunicode();

	// If the label's encoding doesn't match the request, map.
	if (MCU_mapunicode(slabel, is_unicode, to_unicode, r_string))
		return;

	ctxt . Throw();
}

void MCGroup::DoSetLabel(MCExecContext& ctxt, bool to_unicode, MCStringRef p_label)
{
	if (label == NULL || p_label == nil ||
		MCStringIsEqualToOldString(p_label, MCString(label, labelsize), kMCCompareExact) ||
		to_unicode != hasunicode())
	{
		delete label;
		label = NULL;
		if (p_label != nil)
		{
			labelsize = MCStringGetLength(p_label);
			label = new char[labelsize];
			memcpy(label, MCStringGetCString(p_label), labelsize);
			flags |= F_LABEL;

			// If we are setting the unicode label we become unicode; else we revert
			// to native.
			if (to_unicode)
				m_font_flags |= FF_HAS_UNICODE;
			else
				m_font_flags &= ~FF_HAS_UNICODE;
		}
		else
		{
			labelsize = 0;
			flags &= ~F_LABEL;
		}
		Redraw();
	}
}

void MCGroup::GetLabel(MCExecContext& ctxt, MCStringRef& r_label)
{
	DoGetLabel(ctxt, false, r_label);
}

void MCGroup::SetLabel(MCExecContext& ctxt, MCStringRef p_label)
{
	DoSetLabel(ctxt, false, p_label);
}

void MCGroup::GetUnicodeLabel(MCExecContext& ctxt, MCStringRef& r_label)
{
	DoGetLabel(ctxt, true, r_label);
}

void MCGroup::SetUnicodeLabel(MCExecContext& ctxt, MCStringRef p_label)
{
	DoSetLabel(ctxt, true, p_label);
}

void MCGroup::GetHScroll(MCExecContext& ctxt, integer_t& r_scroll)
{
	r_scroll = scrollx;
}

void MCGroup::SetHScroll(MCExecContext& ctxt, integer_t p_scroll)
{
	DoSetHScroll(ctxt, scrollx, p_scroll);
	
	if (!ctxt . HasError())
		boundcontrols();
}

void MCGroup::GetVScroll(MCExecContext& ctxt, integer_t& r_scroll)
{
	r_scroll = scrolly;
}

void MCGroup::SetVScroll(MCExecContext& ctxt, integer_t p_scroll)
{
	DoSetVScroll(ctxt, scrolly, p_scroll);
	
	if (!ctxt . HasError())
		boundcontrols();
}

void MCGroup::GetUnboundedHScroll(MCExecContext& ctxt, bool& r_setting)
{
	r_setting = getflag(F_UNBOUNDED_HSCROLL);
}

void MCGroup::SetUnboundedHScroll(MCExecContext& ctxt, bool setting)
{
	bool t_dirty;
	t_dirty = changeflag(setting, P_UNBOUNDED_HSCROLL);
	
	if (opened && !getflag(P_UNBOUNDED_HSCROLL))
		hscroll(0, True);

	if (t_dirty)
		Redraw();
}

void MCGroup::GetUnboundedVScroll(MCExecContext& ctxt, bool& r_setting)
{
	r_setting = getflag(F_UNBOUNDED_VSCROLL);
}

void MCGroup::SetUnboundedVScroll(MCExecContext& ctxt, bool setting)
{
	bool t_dirty;
	t_dirty = changeflag(setting, P_UNBOUNDED_VSCROLL);
	
	if (opened && !getflag(P_UNBOUNDED_VSCROLL))
		hscroll(0, True);

	if (t_dirty)
		Redraw();
}

void MCGroup::GetHScrollbar(MCExecContext& ctxt, bool& r_setting)
{
	r_setting = getflag(F_HSCROLLBAR);
}

void MCGroup::SetHScrollbar(MCExecContext& ctxt, bool setting)
{
	if (changeflag(setting, F_HSCROLLBAR))
	{	
		DoSetHScrollbar(ctxt, hscrollbar, scrollbarwidth);
		if (!ctxt . HasError())
			boundcontrols();
		Redraw();
	}
}

void MCGroup::GetVScrollbar(MCExecContext& ctxt, bool& r_setting)
{
	r_setting = getflag(F_VSCROLLBAR);
}

void MCGroup::SetVScrollbar(MCExecContext& ctxt, bool setting)
{
	if (changeflag(setting, F_VSCROLLBAR))
	{	
		DoSetVScrollbar(ctxt, vscrollbar, scrollbarwidth);
		if (!ctxt . HasError())
			boundcontrols();
		Redraw();
	}
}

void MCGroup::GetScrollbarWidth(MCExecContext& ctxt, integer_t& r_width)
{
	r_width = scrollbarwidth;
}

void MCGroup::SetScrollbarWidth(MCExecContext& ctxt, integer_t p_width)
{
	if (scrollbarwidth != p_width)
	{
		DoSetScrollbarWidth(ctxt, scrollbarwidth, p_width);
		Redraw();
	}
}

void MCGroup::GetFormattedLeft(MCExecContext& ctxt, integer_t& r_left)
{
	r_left = minrect . x;
}

void MCGroup::GetFormattedHeight(MCExecContext& ctxt, integer_t& r_height)
{
	r_height = minrect . height;
}

void MCGroup::GetFormattedTop(MCExecContext& ctxt, integer_t& r_top)
{
	r_top = minrect . y;
}

void MCGroup::GetFormattedWidth(MCExecContext& ctxt, integer_t& r_width)
{
	r_width = minrect . width;
}

void MCGroup::GetFormattedRect(MCExecContext& ctxt, MCRectangle& r_rect)
{
	r_rect = minrect;
}

void MCGroup::GetBackgroundBehavior(MCExecContext& ctxt, bool& r_setting)
{
	// MW-2011-08-08: [[ Groups ]] Use 'isbackground()' rather than F_GROUP_ONLY.
	r_setting = isbackground();
}

void MCGroup::SetBackgroundBehavior(MCExecContext& ctxt, bool setting)
{
	// MW-2011-08-09: [[ Groups ]] backgroundBehavior maps to !F_GROUP_ONLY.
	//   We can only set the flag to true on non-nested groups.
	changeflag(setting, F_GROUP_ONLY);
	flags ^= F_GROUP_ONLY;

	// Compute whether the parent is a group
	bool t_parent_is_group;
	t_parent_is_group = false;
	if (getparent() != nil && getparent() -> gettype() == CT_GROUP)
		t_parent_is_group = true;
	else if (getstack() -> isediting())
		t_parent_is_group = true;
		
	// MW-2011-08-09: [[ Groups ]] If setting a group to background, make sure it is not nested.
	if (isbackground() && t_parent_is_group)
	{
		setflag(True, F_GROUP_ONLY);
		ctxt . LegacyThrow(EE_GROUP_CANNOTBEBGORSHARED);
		return;
	}

	// MW-2011-08-10: [[ Groups ]] If the group is now a background, make sure it is also shared.
	if (isbackground())
		setflag(True, F_GROUP_SHARED);
}
void MCGroup::GetSharedBehavior(MCExecContext& ctxt, bool& r_setting)
{
	// MW-2011-08-09: [[ Groups ]] Returns whether the group is shared.
	r_setting = isshared();
}

void MCGroup::SetSharedBehavior(MCExecContext& ctxt, bool setting)
{
	// MW-2011-08-09: [[ Groups ]] sharedBehavior maps to F_GROUP_SHARED.
	//   We can only set the flag to true on non-nested groups.
	//   We can only set the flag to false on groups that are placed on a single card.
	changeflag(setting, F_GROUP_SHARED);

	// Compute whether the parent is a group
	bool t_parent_is_group;
	t_parent_is_group = false;
	if (getparent() != nil && getparent() -> gettype() == CT_GROUP)
		t_parent_is_group = true;
	else if (getstack() -> isediting())
		t_parent_is_group = true;
		
	// MW-2011-08-09: [[ Groups ]] If setting a group to shared, make sure it is not nested.
	if (isshared() && t_parent_is_group)
	{
		setflag(False, F_GROUP_SHARED);
		ctxt . LegacyThrow(EE_GROUP_CANNOTBEBGORSHARED);
		return;
	}
	
	// MW-2011-08-09: [[ Groups ]] If setting a group to non-shared, make sure it is on a single card.
	if (!isshared())
	{
		int t_found;
		t_found = 0;
		MCCard *t_cards;
		t_cards = getstack() -> getcards();
		MCCard *t_card;
		t_card = t_cards;
		if (t_card != nil)
			do
			{
				if (t_card -> getchildid(getid()))
				{
					t_found += 1;
					if (t_found > 1)
						break;
				}
				t_card = t_card -> next();
			}
			while(t_card != t_cards);

		// MW-2011-08-09: If the found count is 0 or more than 1, we cannot turn the shared flag off.
		if (t_found != 1)
		{
			setflag(True, F_GROUP_SHARED);
			ctxt . LegacyThrow(EE_GROUP_CANNOTBENONSHARED);
		}
	}
}

void MCGroup::GetBoundingRect(MCExecContext& ctxt, MCRectangle*& r_rect)
{
	if (flags & F_BOUNDING_RECT)
		*r_rect = minrect;
	else
		r_rect = nil;
}

void MCGroup::SetBoundingRect(MCExecContext& ctxt, MCRectangle* p_rect)
{
	bool t_dirty;
	t_dirty = false; 

	if (p_rect == nil)
	{
		flags &= ~F_BOUNDING_RECT;
		t_dirty = computeminrect(False);
	}
	else
	{
		if (minrect . x != p_rect -> x || minrect . y != p_rect -> y || 
			minrect . width != p_rect -> width || minrect . height != p_rect -> height)
		{
			minrect = *p_rect;
			resetscrollbars(False);
			t_dirty = true;
		}
		flags |= F_BOUNDING_RECT;
	}

	if (t_dirty)
		Redraw();
}

void MCGroup::GetBackSize(MCExecContext& ctxt, MCPoint& r_size)
{
	r_size . x = rect . width;
	r_size . y = rect . height;
}

void MCGroup::SetBackSize(MCExecContext& ctxt, MCPoint p_size)
{		
	// MW-2011-08-18: [[ Layers ]] Store the old rect then notify and invalidate.
	MCRectangle t_old_rect;
	t_old_rect = rect;
	rect.width = p_size . x;
	rect.height = p_size . y;
	layer_rectchanged(t_old_rect, true);
}

void MCGroup::GetSelectGroupedControls(MCExecContext& ctxt, bool& r_setting)
{
	r_setting = (flags & F_SELECT_GROUP) != True;
}

void MCGroup::SetSelectGroupedControls(MCExecContext& ctxt, bool setting)
{
	changeflag(setting, F_SELECT_GROUP);
	flags ^= F_SELECT_GROUP;
}

void MCGroup::SetEnabled(MCExecContext& ctxt, uint32_t part, bool setting)
{
	if (!changeflag(setting, F_DISABLED))
	{
		flags ^= F_DISABLED;
		SetChildDisabled(ctxt, part, !setting);
		Redraw();
	}
}

void MCGroup::SetDisabled(MCExecContext& ctxt, uint32_t part, bool setting)
{
	if (changeflag(setting, F_DISABLED))
	{
		SetChildDisabled(ctxt, part, setting);
		Redraw();
	}
}

void MCGroup::SetShowBorder(MCExecContext& ctxt, bool setting)
{
    MCControl::SetShowBorder(ctxt, setting);
    if (computeminrect(False))
        Redraw();
}

void MCGroup::SetTextHeight(MCExecContext& ctxt, uinteger_t* height)
{
    MCObject::SetTextHeight(ctxt, height);
    resetscrollbars(False);
}

void MCGroup::SetTextSize(MCExecContext& ctxt, uinteger_t* size)
{
    MCObject::SetTextSize(ctxt, size);
    if (computeminrect(False))
        Redraw();
}

void MCGroup::SetBorderWidth(MCExecContext& ctxt, uinteger_t width)
{
    MCObject::SetBorderWidth(ctxt, width);
    if (computeminrect(False))
        Redraw();
}

void MCGroup::SetLeftMargin(MCExecContext& ctxt, integer_t p_margin)
{
    MCControl::SetLeftMargin(ctxt, p_margin);
    UpdateMargins();
}

void MCGroup::SetRightMargin(MCExecContext& ctxt, integer_t p_margin)
{
    MCControl::SetRightMargin(ctxt, p_margin);
    UpdateMargins();
}

void MCGroup::SetTopMargin(MCExecContext& ctxt, integer_t p_margin)
{
    MCControl::SetTopMargin(ctxt, p_margin);
    UpdateMargins();
}

void MCGroup::SetBottomMargin(MCExecContext& ctxt, integer_t p_margin)
{
    MCControl::SetBottomMargin(ctxt, p_margin);
    UpdateMargins();
}

void MCGroup::SetMargins(MCExecContext& ctxt, const MCInterfaceMargins& p_margins)
{
    MCControl::SetMargins(ctxt, p_margins);
    UpdateMargins();
}
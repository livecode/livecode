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
#include "core.h"

#include "execpt.h"
#include "handler.h"
#include "hndlrlst.h"
#include "sellst.h"
#include "stack.h"
#include "tooltip.h"
#include "group.h"
#include "card.h"
#include "cdata.h"
#include "button.h"
#include "scrolbar.h"
#include "graphic.h"
#include "eps.h"
#include "image.h"
#include "field.h"
#include "magnify.h"
#include "cpalette.h"
#include "player.h"
#include "mcerror.h"
#include "util.h"
#include "font.h"
#include "redraw.h"


#include "mctheme.h"
#include "globals.h"

#include "context.h"

uint2 MCGroup::labeloffset = 6;

MCGroup::MCGroup()
{
	flags |= F_TRAVERSAL_ON | F_RADIO_BEHAVIOR | F_GROUP_ONLY;
	flags &= ~(F_SHOW_BORDER | F_OPAQUE);
	label = NULL;
	labelsize = 0;
	controls = NULL;
	kfocused = mfocused = NULL;
	newkfocused = oldkfocused = NULL;
	number = MAXUINT2;
	leftmargin = rightmargin = topmargin = bottommargin = defaultmargin;
	vscrollbar = hscrollbar = NULL;
	scrollx = scrolly = 0;
	scrollbarwidth = MCscrollbarwidth;
	minrect.x = minrect.y = minrect.width = minrect.height = 0;
	
	// MERG-2013-06-02: [[ GrpLckUpdates ]] Make sure the group's updates are unlocked
	//   when created.
    m_updates_locked = false;
}

MCGroup::MCGroup(const MCGroup &gref) : MCControl(gref)
{
	MCGroup(gref, false);
}

MCGroup::MCGroup(const MCGroup &gref, bool p_copy_ids) : MCControl(gref)
{
	label = NULL;
	labelsize = 0;
	if (gref.label)
	{
		labelsize = gref.labelsize;
		label = new char[labelsize];
		memcpy(label, gref.label, labelsize);
	}
	controls = NULL;
	if (gref.controls != NULL)
	{
		MCControl *optr = gref.controls;
		do
		{
			// MW-2011-08-08: If we need to copy ids, then we must use 'doclone'
			//   and set id appropriately.
			MCControl *newcontrol;
			if (optr -> gettype() == CT_GROUP && p_copy_ids)
			{
				MCGroup *t_group;
				t_group = (MCGroup *)optr;
				newcontrol = t_group -> doclone(False, OP_NONE, true, false);
			}
			else
				newcontrol = optr -> clone(False, OP_NONE, false);

			if (p_copy_ids)
				newcontrol -> setid(optr -> getid());

			newcontrol->appendto(controls);
			newcontrol->setparent(this);
			optr = optr->next();
		}
		while (optr != gref.controls);
	}
	minrect = gref.minrect;
	kfocused = mfocused = NULL;
	number = MAXUINT2;
	if (gref.vscrollbar != NULL)
	{
		vscrollbar = new MCScrollbar(*gref.vscrollbar);
		vscrollbar->setparent(this);
		vscrollbar->allowmessages(False);
		vscrollbar->setflag(flags & F_DISABLED, F_DISABLED);
		vscrollbar->setembedded();
	}
	else
		vscrollbar = NULL;
	if (gref.hscrollbar != NULL)
	{
		hscrollbar = new MCScrollbar(*gref.hscrollbar);
		hscrollbar->setparent(this);
		hscrollbar->allowmessages(False);
		hscrollbar->setflag(flags & F_DISABLED, F_DISABLED);
		hscrollbar->setembedded();
	}
	else
		hscrollbar = NULL;
	scrollx = gref.scrollx;
	scrolly = gref.scrolly;
	scrollbarwidth = gref.scrollbarwidth;
	
	// MERG-2013-06-02: [[ GrpLckUpdates ]] Make sure the group's updates are unlocked
	//   when cloned.
    m_updates_locked = false;
}

MCGroup::~MCGroup()
{
	delete label;
	while (controls != NULL)
	{
		MCControl *cptr = controls->remove
		                  (controls);
		delete cptr;
	}
	if (this == MCmenubar)
	{
		MCmenubar = NULL;
		MCscreen->updatemenubar(True);
	}
	if (this == MCdefaultmenubar)
	{
		MCdefaultmenubar = NULL;
		MCscreen->updatemenubar(True);
	}
	delete vscrollbar;
	delete hscrollbar;
}

Chunk_term MCGroup::gettype() const
{
	return CT_GROUP;
}

const char *MCGroup::gettypestring()
{
	return MCgroupstring;
}

bool MCGroup::visit(MCVisitStyle p_style, uint32_t p_part, MCObjectVisitor* p_visitor)
{
	bool t_continue;
	t_continue = true;

	if (p_style == VISIT_STYLE_DEPTH_LAST)
		t_continue = p_visitor -> OnGroup(this);

	if (t_continue && controls != NULL)
	{
		MCControl *cptr = controls;
		do
		{
			t_continue = cptr -> visit(p_style, p_part, p_visitor);
			cptr = cptr->next();
		}
		while(t_continue && cptr != controls);
	}

	if (t_continue && p_style == VISIT_STYLE_DEPTH_FIRST)
		t_continue = p_visitor -> OnGroup(this);

	return t_continue;
}

void MCGroup::open()
{
	MCControl::open();
	if (controls != NULL)
	{
		MCControl *cptr = controls;
		do
		{
			cptr->open();
			cptr = cptr->next();
		}
		while (cptr != controls);
	}
	if (hscrollbar != NULL)
	{
		hscrollbar->open();
		scrollbarwidth = hscrollbar->getrect().height;
	}
	if (vscrollbar != NULL)
	{
		vscrollbar->open();
		scrollbarwidth = vscrollbar->getrect().width;
	}
	if (opened == 1)
	{
		mgrabbed = False;
		newkfocused = oldkfocused = kfocused = mfocused = NULL;
		computeminrect(False);
		setsbrects();
	}
}

void MCGroup::close()
{
	if (!opened)
		return;
	if (controls != NULL)
	{
		MCControl *cptr = controls;
		do
		{
			cptr->close();
			cptr = cptr->next();
		}
		while (cptr != controls);
	}
	if (vscrollbar != NULL)
		vscrollbar->close();
	if (hscrollbar != NULL)
		hscrollbar->close();
	MCControl::close();
}

void MCGroup::kfocus()
{
	if (!(state & CS_KFOCUSED))
	{
		if (oldkfocused != NULL)
		{
			kfocused = oldkfocused;
			state |= CS_KFOCUSED;
			kfocused->kfocus();
		}
		else
			if (!kfocusnext(True))
				getcard()->kfocusnext(False);
	}
}

Boolean MCGroup::kfocusnext(Boolean top)
{
	if (state & CS_KFOCUSED && flags & F_TAB_GROUP_BEHAVIOR
	        || !(flags & F_TRAVERSAL_ON) || !(flags & F_VISIBLE || MCshowinvisibles))
		return False;
	if (newkfocused != NULL)
	{
		kfocused = newkfocused;
		return True;
	}
	MCControl *startptr;
	if (top || kfocused == NULL)
		startptr = controls;
	else
		startptr = kfocused;
	if (startptr != NULL)
	{
		MCControl *cptr = startptr;
		do
		{
			if (cptr->kfocusnext(top))
			{
				if (kfocused == NULL)
					oldkfocused = cptr;
				else
					if (cptr != kfocused)
					{
						MCControl *tptr = kfocused;
						kfocused = NULL;
						tptr->kunfocus();
						if (kfocused == NULL)
						{
							kfocused = cptr;
							kfocused->kfocus();

							// MW-2007-07-05: [[ Bug 2065 ]] - Make sure the group is marked as focused if we have focused a control.
							state |= CS_KFOCUSED;
						}
					}
				return True;
			}
			cptr = cptr->next();
		}
		while (cptr != controls);
	}
	if (state & CS_KFOCUSED && kfocused != NULL)
	{
		MCControl *tptr = kfocused;
		kfocused = NULL;
		tptr->kunfocus();
		if (kfocused != NULL)
			return True;
		kfocused = tptr;
	}
	return False;
}

Boolean MCGroup::kfocusprev(Boolean bottom)
{
	if (state & CS_KFOCUSED && flags & F_TAB_GROUP_BEHAVIOR
	        || !(flags & F_TRAVERSAL_ON) || !(flags & F_VISIBLE || MCshowinvisibles))
		return False;
	MCControl *startptr = NULL;
	// MW-2005-07-18: [[Bug 2923]] Crash when tabbing backwards
	//   This is caused by calling 'prev' on a potentially NULL pointer.
	if (bottom || kfocused == NULL)
		startptr = (controls == NULL ? NULL : controls->prev());
	else
		startptr = kfocused;
	if (startptr != NULL)
	{
		MCControl *cptr = startptr;
		do
		{
			if (cptr->kfocusprev(bottom))
			{
				if (kfocused == NULL)
					oldkfocused = cptr;
				else
					if (cptr != kfocused)
					{
						MCControl *tptr = kfocused;
						kfocused = NULL;
						tptr->kunfocus();
						if (kfocused == NULL)
						{
							kfocused = cptr;
							kfocused->kfocus();
						}
					}
				return True;
			}
			cptr = cptr->prev();
		}
		while (cptr != controls->prev());
	}
	if (state & CS_KFOCUSED && kfocused != NULL)
	{
		MCControl *tptr = kfocused;
		kfocused = NULL;
		tptr->kunfocus();
		if (kfocused != NULL)
			return True;
		kfocused = tptr;
	}
	return False;
}

void MCGroup::kunfocus()
{
	if (kfocused != NULL)
	{
		MCControl *tkfocused = kfocused;
		kfocused = NULL;
		tkfocused->kunfocus();
		if (kfocused != NULL)
			return;
		oldkfocused = tkfocused;
	}
	else
		oldkfocused = NULL;
	state &= ~CS_KFOCUSED;
}

Boolean MCGroup::kdown(const char *string, KeySym key)
{
	if (kfocused == NULL)
		kfocused = oldkfocused;
	if (kfocused == NULL)
		return False;
	if (flags & F_TAB_GROUP_BEHAVIOR)
		switch (key)
		{
		case XK_Tab:
			if (MCmodifierstate & MS_SHIFT)
				getcard()->kfocusprev(False);
			else
				getcard()->kfocusnext(False);
			return True;
		case XK_Right:
		case XK_Down:
			state &= ~CS_KFOCUSED;
			if (!kfocusnext(False))
				kfocusnext(True);
			state |= CS_KFOCUSED;
			return True;
		case XK_Left:
		case XK_Up:
			state &= ~CS_KFOCUSED;
			if (!kfocusprev(False))
				kfocusprev(True);
			state |= CS_KFOCUSED;
			return True;
		default:
			newkfocused = kfocused;
			MCControl *oldfocused = kfocused;
			if (kfocused->kdown(string, key))
			{
				radio(0, oldfocused);
				newkfocused = NULL;
				return True;
			}
			newkfocused = NULL;
			return False;
		}
	return kfocused->kdown(string, key);
}

Boolean MCGroup::kup(const char *string, KeySym key)
{
	if (kfocused == NULL)
		return False;
	return kfocused->kup(string, key);
}

void MCGroup::mdrag(void)
{
	if (!mgrabbed)
		return;

	// MW-2008-02-27: [[ Bug 5968 ]] dragStart sent if click started in scrollbar
	if (getstate(CS_HSCROLL) || getstate(CS_VSCROLL))
		return;

	if (getstack() -> gettool(this) != T_BROWSE)
		return;

	if (mfocused != NULL)
	{
		mfocused -> mdrag();
	}
	else
	{
		message(MCM_drag_start);
		MCdragtargetptr = this;
	}
}

Boolean MCGroup::mfocus(int2 x, int2 y)
{
	if (!(flags & F_VISIBLE || MCshowinvisibles)
	        || flags & F_DISABLED && getstack()->gettool(this) == T_BROWSE)
		return False;
	if (state & CS_MENU_ATTACHED)
		return MCObject::mfocus(x, y);
	if (!(flags & F_VISIBLE || MCshowinvisibles))
	{
		mfocused = NULL;
		mgrabbed = False;
		return False;
	}
	if (state & CS_MOVE)
		return MCControl::mfocus(x, y);
	else
		if (getstate(CS_GRAB))
			return MCControl::mfocus(x, y);
	mx = x;
	my = y;
	Tool tool = getstack()->gettool(this);
	if (tool == T_BROWSE || tool == T_POINTER || tool == T_HELP)
	{
		if (mgrabbed && mfocused->mfocus(x, y))
			return True;
		mgrabbed = False;
		if (sbfocus(x, y, hscrollbar, vscrollbar))
		{
			if (mfocused != NULL)
			{
				mfocused->munfocus();
				mfocused = NULL;
			}
			return True;
		}
		if (MCControl::mfocus(x, y))
		{
			// MW-2008-01-30: [[ Bug 5832 ]] Previously we would have been immediately
			//   unfocusing the group if there were no controls, this resulted in much
			//   sadness as empty groups wouldn't be resizable :o(
			if (controls != NULL)
			{
				MCControl *tptr = controls->prev();
				do
				{
					if (tptr->mfocus(x, y))
					{
						if (mfocused != NULL && tptr != mfocused)
							mfocused->munfocus();
						if (tptr != mfocused)
						{
							mfocused = tptr;
							if (mfocused->gettype() != CT_GROUP)
							{
								mfocused->enter();

								// MW-2007-10-31: mouseMove sent before mouseEnter - make sure we send an mouseMove
								//   ... and now lets make sure it doesn't crash!
								//   Here mfocused can be NULL if a control was deleted in mfocused -> enter()
								if (mfocused != NULL)
									mfocused->mfocus(x, y);
							}
						}
						return True;
					}
					else if (tptr == mfocused)
					{
						mfocused->munfocus();//changed
						mfocused = NULL;
					}
					tptr = tptr->prev();
				}
				while (tptr != controls->prev());
			}
			if (state & CS_SELECTED)
				return True;
		}
		else
			if (mfocused != NULL)
			{
				MCControl *oldfocused = mfocused;
				mfocused = NULL;
				oldfocused->munfocus();
			}
	}
	else
		return MCControl::mfocus(x, y);
	return False;
}

void MCGroup::munfocus()
{
	mgrabbed = False;
	if (mfocused != NULL)
	{
		MCControl *oldfocused = mfocused;
		mfocused = NULL;
		// IM-2013-08-07: [[ Bug 10671 ]] Release grabbed controls when removing focus
		state &= ~CS_GRAB;
		oldfocused->munfocus();
	}
	else
	{
		mfocused = NULL;
		message(MCM_mouse_leave);
	}
	state &= ~(CS_MFOCUSED | CS_HSCROLL | CS_VSCROLL);
}

// MW-2012-03-01: [[ Bug 10045 ]] Clear the mfocus setting of the group without dispatching
//   'mouse_leave'. This is called by MCCard::mfocus() to stop mouseLeave being sent twice.
void MCGroup::clearmfocus(void)
{
	mgrabbed = False;
	mfocused = nil;
	state &= ~(CS_MFOCUSED | CS_HSCROLL | CS_VSCROLL);
}

Boolean MCGroup::mdown(uint2 which)
{
	if (state & CS_MENU_ATTACHED)
		return MCObject::mdown(which);
	if (sbdown(which, hscrollbar, vscrollbar))
		return True;
	Tool tool = getstack()->gettool(this);
	if (tool == T_POINTER && (mfocused == NULL || !MCselectgrouped || getflag(F_SELECT_GROUP)))
	{
		if (which == Button1)
		{
			if (state & CS_MFOCUSED)
				return False;
			state |= CS_MFOCUSED;
			start(True);
		}
		else
			message_with_args(MCM_mouse_down, which);
		return True;
	}
	if (mfocused == NULL)
		return False;
	mgrabbed = True;
	
	// MW-2012-03-15: [[ Bug 10074 ]] If a control's mdown() handler hides the group
	//   then grabbed gets reset causing this method to return false and the parent
	//   to send a message. To solve this we separately store whether the mdown() was
	//   handled or not.
	bool t_handled;
	t_handled = false;
	
	state |= CS_MFOCUSED;
	if (mfocused == NULL || !mfocused->mdown(which))
	{
		mgrabbed = False;
		state &= ~CS_MFOCUSED;
	}
	else
		t_handled = true;
	
	return t_handled;
}

Boolean MCGroup::mup(uint2 which)
{
	if (state & CS_MENU_ATTACHED)
		return MCObject::mup(which);
	if (state & CS_GRAB)
	{
		state &= ~(CS_GRAB | CS_MFOCUSED);
		mgrabbed = False;
		mfocused->mfocus(mx, my);
		return mfocused->mup(which);
	}
	if (sbup(which, hscrollbar, vscrollbar))
		return True;
	Tool tool = getstack()->gettool(this);
	if (tool == T_POINTER && (mfocused == NULL || !MCselectgrouped || getflag(F_SELECT_GROUP)))
	{
		if (which == Button1)
		{
			if (!(state & CS_MFOCUSED))
				return False;
			state &= ~CS_MFOCUSED;
			end();
		}
		else
			message_with_args(MCM_mouse_up, which);
		return True;
	}
	state &= ~CS_MFOCUSED;
	newkfocused = mfocused;
	MCControl *oldfocused = mfocused;
	// MH-2007-03-20: [[ Bug 705 ]] Selecting a radio button using pointer tool unhilites other radio buttons in the group with radiobehavior set.
	if (tool != T_POINTER)
		radio(0, oldfocused);
	mgrabbed = False;
	if (mfocused == NULL || mfocused->mup(which))
	{
		newkfocused = NULL;
		// MH-2007-03-20: [[ Bug 705 ]] Selecting a radio button using pointer tool unhilites other radio buttons in the group with radiobehavior set.
		if (tool != T_POINTER)
			radio(0, oldfocused);
		return True;
	}
	newkfocused = NULL;
	return False;
}


// MW-2010-10-15: [[ Bug 9055 ]] Restructured to mimic mdown behavior in pointer tool mode.
Boolean MCGroup::doubledown(uint2 which)
{
	if (sbdoubledown(which, hscrollbar, vscrollbar))
		return True;

	Tool tool = getstack() -> gettool(this);
	if (tool == T_POINTER && (mfocused == NULL || !MCselectgrouped || getflag(F_SELECT_GROUP)))
	{
		message_with_args(MCM_mouse_double_down, which);
		return True;
	}

	if (mfocused == NULL)
		return False;

	mgrabbed = True;
	state |= CS_MFOCUSED;
	if (mfocused == NULL || !mfocused->doubledown(which))
	{
		mgrabbed = False;
		state &= ~CS_MFOCUSED;
	}

	return mgrabbed;
}

// MW-2010-10-15: [[ Bug 9055 ]] Restructured to mimic mup behavior in pointer tool mode.
Boolean MCGroup::doubleup(uint2 which)
{
	if (sbdoubleup(which, hscrollbar, vscrollbar))
		return True;
	
	Tool tool = getstack() -> gettool(this);
	if (tool == T_POINTER && (mfocused == NULL || !MCselectgrouped || getflag(F_SELECT_GROUP)))
	{
		message_with_args(MCM_mouse_double_up, which);
		return True;
	}

	mgrabbed = False;
	state &= ~CS_MFOCUSED;
	if (mfocused == NULL || mfocused -> doubleup(which))
		return True;

	return False;
}

void MCGroup::setrect(const MCRectangle &nrect)
{
	bool t_size_changed;
	t_size_changed = nrect . width != rect . width || nrect . height != rect . height;

	if (controls != NULL)
		if (state & CS_SIZE || rect.x + rect.width == nrect.x + nrect.width
		        && rect.y + rect.height == nrect.y + nrect.height)
		{
			if (flags & F_HSCROLLBAR || flags & F_VSCROLLBAR)
			{
				scrollx += rect.x - nrect.x;
				scrolly += rect.y - nrect.y;
			}
			rect = nrect;
			setsbrects();
			boundcontrols();
		}
		else
		{
			uint1 oldopen = opened;
			opened = 0;  // suppress all updates and messages
			int4 oldx = scrollx;
			int4 oldy = scrolly;
			hscroll(-scrollx, False);
			vscroll(-scrolly, False);
			int2 dx = rect.x - nrect.x;
			int2 dy = rect.y - nrect.y;
			MCControl *cptr = controls;
			do
			{
				MCRectangle trect = cptr->getrect();
				trect.x -= dx;
				trect.y -= dy;
				cptr->setrect(trect);
				// MM-2012-09-05: [[ Property Listener ]] Resizing a group potentially alters the location of its children.
				cptr -> signallisteners(P_LOCATION);
				cptr = cptr->next();
			}
			while (cptr != controls);
			minrect.x -= dx;
			minrect.y -= dy;
			rect = nrect;
			setsbrects();
			hscroll(oldx, False);
			vscroll(oldy, False);
			resetscrollbars(False);
			opened = oldopen;
		}
	else
	{
		rect = nrect;
		setsbrects();
	}
	
	if (!getstate(CS_SENDING_RESIZE) && t_size_changed)
	{
		setstate(True, CS_SENDING_RESIZE);
		conditionalmessage(HH_RESIZE_CONTROL, MCM_resize_control);
		setstate(False, CS_SENDING_RESIZE);
	}
}

Exec_stat MCGroup::getprop(uint4 parid, Properties which, MCExecPoint &ep, Boolean effective)
{
	switch (which)
	{
#ifdef /* MCGroup::getprop */ LEGACY_EXEC
	case P_CANT_DELETE:
		ep.setboolean(getflag(F_G_CANT_DELETE));
		break;
	case P_DONT_SEARCH:
		ep.setboolean(getflag(F_G_DONT_SEARCH));
		break;
	case P_SHOW_PICT:
		ep.setboolean(True);
		break;
	case P_RADIO_BEHAVIOR:
		ep.setboolean(getflag(F_RADIO_BEHAVIOR));
		break;
	case P_TAB_GROUP_BEHAVIOR:
		ep.setboolean(getflag(F_TAB_GROUP_BEHAVIOR));
		break;
	case P_HILITED_BUTTON:
		ep.setint(gethilited(parid));
		break;
	case P_HILITED_BUTTON_ID:
		ep.setint(gethilitedid(parid));
		break;
	case P_HILITED_BUTTON_NAME:
		ep.setnameref_unsafe(gethilitedname(parid));
		break;
	case P_SHOW_NAME:
		ep.setboolean(getflag(F_SHOW_NAME));
		break;
	// MW-2012-02-16: [[ IntrinsicUnicode ]] Add support for getting the 'unicodeLabel'.
	case P_LABEL:
	case P_UNICODE_LABEL:
		if (label == NULL)
			ep.clear();
		else
		{
			ep.setsvalue(MCString(label, labelsize));

			// If the encoding of the label does not match the requst, then map it.
			ep.mapunicode(hasunicode(), which == P_UNICODE_LABEL);
		}
		break;
	case P_HSCROLL:
		ep.setint(scrollx);
		break;
	case P_VSCROLL:
		ep.setint(scrolly);
		break;
	case P_UNBOUNDED_HSCROLL:
		ep.setboolean(getflag(F_UNBOUNDED_HSCROLL));
		break;
	case P_UNBOUNDED_VSCROLL:
		ep.setboolean(getflag(F_UNBOUNDED_VSCROLL));
		break;
	case P_HSCROLLBAR:
		ep.setboolean(getflag(F_HSCROLLBAR));
		break;
	case P_VSCROLLBAR:
		ep.setboolean(getflag(F_VSCROLLBAR));
		break;
	case P_SCROLLBAR_WIDTH:
		ep.setint(scrollbarwidth);
		break;
	case P_FORMATTED_LEFT:
		ep.setint(minrect.x);
		break;
	case P_FORMATTED_HEIGHT:
		ep.setint(minrect.height);
		break;
	case P_FORMATTED_TOP:
		ep.setint(minrect.y);
		break;
	case P_FORMATTED_WIDTH:
		ep.setint(minrect.width);
		break;
	case P_FORMATTED_RECT:
		ep.setrectangle(minrect);
		break;
	case P_BACKGROUND_BEHAVIOR:
		// MW-2011-08-08: [[ Groups ]] Use 'isbackground()' rather than F_GROUP_ONLY.
		ep.setboolean(isbackground());
		break;
	case P_SHARED_BEHAVIOR:
		// MW-2011-08-09: [[ Groups ]] Returns whether the group is shared.
		ep.setboolean(isshared() && (parent == nil || parent -> gettype() == CT_CARD));
		break;
	case P_BOUNDING_RECT:
		if (flags & F_BOUNDING_RECT)
			ep.setrectangle(minrect);
		else
			ep.clear();
		break;
	case P_BACK_SIZE:
		ep.setpoint(rect.width, rect.height);
		break;
	case P_CARD_NAMES:
    case P_CARD_IDS:
		{
			// MERG-2013-05-01: [[ GrpCardIds ]] Add 'the cardIds' property to
			//   groups (returns ids rather than names).
			ep.clear();
			MCExecPoint ep2(ep);
			MCCard *startcard = getstack()->getcards();
			MCCard *cptr = startcard;
			uint2 j = 0;
            Properties t_prop;
            if (which == P_CARD_NAMES)
                t_prop = P_SHORT_NAME;
            else
                t_prop = P_SHORT_ID;
            do
			{
				if (cptr->countme(obj_id, False))
				{
					cptr->getprop(0, t_prop, ep2, False);
					ep.concatmcstring(ep2.getsvalue(), EC_RETURN, j++ == 0);
				}
				cptr = cptr->next();
			}
			while (cptr != startcard);
		}
		break;
	case P_CONTROL_NAMES:
	case P_CONTROL_IDS:
	case P_CHILD_CONTROL_NAMES:
	case P_CHILD_CONTROL_IDS:
        {
			// MERG-2013-05-01: [[ ChildControlProps ]] Add ability to list both
			//   immediate and all descendent controls of a group.
		
            ep.clear();
		
            // MERG-2013-08-14: [[ ChildControlProps ]] Resolved crash when group contains no controls
            if (controls != NULL)
            {
                MCExecPoint t_other_ep(ep);
                MCObject *t_object = controls;
                MCObject *t_start_object = t_object;
                uint2 i = 0;
                do
                {
                    Properties t_prop;
                    if (which == P_CHILD_CONTROL_NAMES)
                        t_prop = P_SHORT_NAME;
                    else
                        t_prop = P_SHORT_ID;
                    
                    t_object->getprop(0, t_prop, t_other_ep, False);
                    
                    ep.concatmcstring(t_other_ep.getsvalue(), EC_RETURN, i++ == 0);
                    
                    if (t_object->gettype() == CT_GROUP && (which == P_CONTROL_IDS || which == P_CONTROL_NAMES))
                    {
                        t_object->getprop(parid, which, t_other_ep, false);
                        ep.concatmcstring(t_other_ep.getsvalue(), EC_RETURN, i++ == 0);
                    }
                    
                    t_object = t_object -> next();
                    
                }
                while (t_object != t_start_object);
            }
        }
        break;
	case P_SELECT_GROUPED_CONTROLS:
		ep.setboolean(!(flags & F_SELECT_GROUP));
		break;
	// MW-2013-06-20: [[ GrpLckUpdates ]] [[ Bug 10960 ]] Add accessor for 'the lockUpdates'
	case P_LOCK_UPDATES:
		ep.setboolean(m_updates_locked);
		break;
#endif /* MCGroup::getprop */
	default:
		return MCControl::getprop(parid, which, ep, effective);
	}
	return ES_NORMAL;
}

Exec_stat MCGroup::setprop(uint4 parid, Properties p, MCExecPoint &ep, Boolean effective)
{
	Boolean dirty = False;
	int2 i1, i2, i3, i4;
	MCString data = ep.getsvalue();

	switch (p)
	{
#ifdef /* MCGroup::setprop */ LEGACY_EXEC
	case P_SHOW_BORDER:
	case P_BORDER_WIDTH:
	case P_TEXT_SIZE:
		if (MCControl::setprop(parid, p, ep, effective) != ES_NORMAL)
			return ES_ERROR;
		dirty = computeminrect(False);
		break;
	case P_TEXT_HEIGHT:
		if (MCControl::setprop(parid, p, ep, effective) != ES_NORMAL)
			return ES_ERROR;
		resetscrollbars(False);
		break;
	case P_SHOW_PICT:
		break;
	case P_CANT_DELETE:
		if (!MCU_matchflags(data, flags, F_G_CANT_DELETE, dirty))
		{
			MCeerror->add
			(EE_OBJECT_NAB, 0, 0, data);
			return ES_ERROR;
		}
		break;
	case P_DONT_SEARCH:
		if (!MCU_matchflags(data, flags, F_G_DONT_SEARCH, dirty))
		{
			MCeerror->add
			(EE_OBJECT_NAB, 0, 0, data);
			return ES_ERROR;
		}
		break;
	case P_TAB_GROUP_BEHAVIOR:
		if (!MCU_matchflags(data, flags, F_TAB_GROUP_BEHAVIOR, dirty))
		{
			MCeerror->add
			(EE_OBJECT_NAB, 0, 0, data);
			return ES_ERROR;
		}
		break;
	case P_RADIO_BEHAVIOR:
		if (!MCU_matchflags(data, flags, F_RADIO_BEHAVIOR, dirty))
		{
			MCeerror->add
			(EE_OBJECT_NAB, 0, 0, data);
			return ES_ERROR;
		}
		if (flags & F_RADIO_BEHAVIOR)
			flags |= F_TAB_GROUP_BEHAVIOR;
		radio(parid, kfocused);
		radio(parid, mfocused);
		break;
	case P_HILITED_BUTTON:
		uint2 button;
		if (!MCU_stoui2(data, button))
		{
			MCeerror->add
			(EE_GROUP_HILITEDNAN, 0, 0, data);
			return ES_ERROR;
		}
		sethilited(parid, button);
		break;
	case P_HILITED_BUTTON_ID:
		uint4 newid;
		if (!MCU_stoui4(data, newid))
		{
			MCeerror->add
			(EE_GROUP_HILITEDNAN, 0, 0, data);
			return ES_ERROR;
		}
		sethilitedid(parid, newid);
		break;
	case P_HILITED_BUTTON_NAME:
	{
		MCAutoNameRef t_name;
		/* UNCHECKED */ ep . copyasnameref(t_name);
		sethilitedname(parid, t_name);
	}
		break;
	case P_ENABLED:
	case P_DISABLED:
		if (!MCU_matchflags(data, flags, F_DISABLED, dirty))
		{
			MCeerror->add
			(EE_OBJECT_NAB, 0, 0, data);
			return ES_ERROR;
		}
		if (p == P_ENABLED)
		{
			flags ^= F_DISABLED;
			dirty = !dirty;
		}
		if (dirty)
			setchildprops(parid, p, ep);
		break;
	case P_SHOW_NAME:
		if (!MCU_matchflags(data, flags, F_SHOW_NAME, dirty))
		{
			MCeerror->add
			(EE_OBJECT_NAB, 0, 0, data);
			return ES_ERROR;
		}
		if (dirty)
		{
			// MW-2011-09-21: [[ Layers ]] Changing the showName property
			//   affects the layer attrs.
			m_layer_attr_changed = true;
			dirty = computeminrect(False);
		}
		break;
	// MW-2012-02-16: [[ IntrinsicUnicode ]] Add support for setting the unicode label.
	case P_LABEL:
	case P_UNICODE_LABEL:
		if (label == NULL ||
			data.getlength() != labelsize ||
			memcmp(data.getstring(), label, data.getlength()) != 0 ||
			(p == P_UNICODE_LABEL) != hasunicode())
		{
			delete label;
			label = NULL;
			if (data != MCnullmcstring)
			{
				labelsize = data.getlength();
				label = new char[labelsize];
				memcpy(label, data.getstring(), labelsize);
				flags |= F_LABEL;
				
				// If we are setting the unicodeLabel we become unicode, else we
				// revert to native.
				if (p == P_UNICODE_LABEL)
					m_font_flags |= FF_HAS_UNICODE;
				else
					m_font_flags &= ~FF_HAS_UNICODE;
			}
			else
			{
				labelsize = 0;
				flags &= ~F_LABEL;
			}
			dirty = True;
		}
		break;
	case P_MARGINS:
	case P_LEFT_MARGIN:
	case P_RIGHT_MARGIN:
	case P_TOP_MARGIN:
	case P_BOTTOM_MARGIN:
		if (MCControl::setprop(parid, p, ep, effective) != ES_NORMAL)
			return ES_ERROR;
		if (leftmargin == defaultmargin && rightmargin == defaultmargin
		        && topmargin == defaultmargin && bottommargin == defaultmargin)
			flags &= ~F_MARGINS;
		else
			flags |= F_MARGINS;
		dirty = computeminrect(False);
		resetscrollbars(False);
		break;
	case P_HSCROLL:
	case P_VSCROLL:
	case P_HSCROLLBAR:
	case P_VSCROLLBAR:
	case P_SCROLLBAR_WIDTH:
		if (setsbprop(p, data, scrollx, scrolly, scrollbarwidth,
		              hscrollbar, vscrollbar, dirty) == ES_ERROR)
			return ES_ERROR;

		boundcontrols();
		break;
	case P_UNBOUNDED_HSCROLL:
		if (!MCU_matchflags(data, flags, F_UNBOUNDED_HSCROLL, dirty))
		{
			MCeerror->add(EE_OBJECT_NAB, 0, 0, data);
			return ES_ERROR;
		}
		if (opened && !getflag(F_UNBOUNDED_HSCROLL))
			hscroll(0, True);
		break;
	case P_UNBOUNDED_VSCROLL:
		if (!MCU_matchflags(data, flags, F_UNBOUNDED_VSCROLL, dirty))
		{
			MCeerror->add(EE_OBJECT_NAB, 0, 0, data);
			return ES_ERROR;
		}
		if (opened && !getflag(F_UNBOUNDED_VSCROLL))
			vscroll(0, True);
		break;
	case P_BOUNDING_RECT:
		if (!data.getlength())
		{
			flags &= ~F_BOUNDING_RECT;
			dirty = computeminrect(False);
		}
		else
		{
			if (!MCU_stoi2x4(data, i1, i2, i3, i4))
			{
				MCeerror->add
				(EE_OBJECT_NAR, 0, 0, data);
				return ES_ERROR;
			}
			if (minrect.x != i1 || minrect.y != i2 || minrect.x + minrect.width != i3
			        || minrect.y + minrect.height != i4)
			{
				minrect.x = i1;
				minrect.y = i2;
				minrect.width = MCU_max(i3 - i1, 1);
				minrect.height = MCU_max(i4 - i2, 1);
				resetscrollbars(False);
				dirty = True;
			}
			flags |= F_BOUNDING_RECT;
		}
		break;
	case P_BACKGROUND_BEHAVIOR:
	{
		// MW-2011-08-09: [[ Groups ]] backgroundBehavior maps to !F_GROUP_ONLY.
		//   We can only set the flag to true on non-nested groups.
		if (!MCU_matchflags(data, flags, F_GROUP_ONLY, dirty))
		{
			MCeerror->add(EE_OBJECT_NAB, 0, 0, data);
			return ES_ERROR;
		}

		flags ^= F_GROUP_ONLY;
		dirty = False;

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
			MCeerror -> add(EE_GROUP_CANNOTBEBGORSHARED, 0, 0);
			return ES_ERROR;
		}

		// MW-2011-08-10: [[ Groups ]] If the group is now a background, make sure it is also shared.
		if (isbackground())
			setflag(True, F_GROUP_SHARED);
	}
	break;
	case P_SHARED_BEHAVIOR:
	{
		// MW-2011-08-09: [[ Groups ]] sharedBehavior maps to !F_GROUP_SHARED.
		//   We can only set the flag to true on non-nested groups.
		//   We can only set the flag to false on groups that are placed on a single card.
		if (!MCU_matchflags(data, flags, F_GROUP_SHARED, dirty))
		{
			MCeerror->add(EE_OBJECT_NAB, 0, 0, data);
			return ES_ERROR;
		}
			
		dirty = False;

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
			MCeerror -> add(EE_GROUP_CANNOTBEBGORSHARED, 0, 0);
			return ES_ERROR;
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
				MCeerror -> add(EE_GROUP_CANNOTBENONSHARED, 0, 0);
				return ES_ERROR;
			}
		}
	}
	break;
	case P_BACK_SIZE:
	{
		if (!MCU_stoi2x2(data, i1, i2))
		{
			MCeerror->add(EE_GROUP_BACKSIZENAP, 0, 0, data);
			return ES_ERROR;
		}
		// MW-2011-08-18: [[ Layers ]] Store the old rect then notify and invalidate.
		MCRectangle t_old_rect;
		t_old_rect = rect;
		rect.width = i1;
		rect.height = i2;
		layer_rectchanged(t_old_rect, true);
	}
	break;
	case P_SELECT_GROUPED_CONTROLS:
		if (!MCU_matchflags(data, flags, F_SELECT_GROUP, dirty))
		{
			MCeerror -> add(EE_OBJECT_NAB, 0, 0, data);
			return ES_ERROR;
		}
		dirty = False;
		flags ^= F_SELECT_GROUP;
		break;
	// MERG-2013-06-02: [[ GrpLckUpdates ]] Handle setting of the lockUpdates property.
    case P_LOCK_UPDATES:
	{
		Exec_stat t_stat;
		Boolean t_lock;
		
		t_stat = ep.getboolean(t_lock, 0, 0, EE_PROPERTY_NAB);
		if (t_stat == ES_NORMAL)
			m_updates_locked = (t_lock == True);
			
		// When the lock is turned off, make sure we update the group.
		if (!t_lock)
			computeminrect(True);
			
		return t_stat;
	}
	break;
#endif /* MCGroup::setprop */
	default:
		return MCControl::setprop(parid, p, ep, effective);
	}
	if (dirty && opened)
	{
		// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
		layer_redrawall();
	}
	return ES_NORMAL;
}

Boolean MCGroup::del()
{
	if (flags & F_G_CANT_DELETE)
	{
		MCeerror->add
		(EE_OBJECT_CANTREMOVE, 0, 0);
		return False;
	}
	if (controls != NULL)
	{
		MCControl *cptr = controls;
		do
		{
			if (cptr->getscriptdepth() != 0)
			{
				MCeerror->add
				(EE_OBJECT_CANTREMOVE, 0, 0);
				return False;
			}
			cptr = cptr->next();
		}
		while (cptr != controls);
	}
	rect = getcard()->getrect();
	return MCControl::del();
}

void MCGroup::recompute()
{
	if (controls != NULL)
	{
		MCControl *cptr = controls;
		do
		{
			cptr->recompute();
			cptr = cptr->next();
		}
		while (cptr != controls);
	}
}

Boolean MCGroup::kfocusset(MCControl *target)
{
	if (controls != NULL)
	{
		MCControl *cptr = controls;
		do
		{
			if (cptr->kfocusset(target))
			{
				oldkfocused = cptr;
				return True;
			}
			cptr = cptr->next();
		}
		while (cptr != controls);
		kunfocus();
	}
	return False;
}


MCControl *MCGroup::clone(Boolean attach, Object_pos p, bool invisible)
{
	return doclone(attach, p, false, invisible);
}

MCControl *MCGroup::doclone(Boolean attach, Object_pos p, bool p_copy_ids, bool invisible)
{
	MCGroup *newgroup;
	newgroup = new MCGroup(*this, p_copy_ids);

	if (attach)
	{
		newgroup->attach(p, invisible);
		getcard()->message(MCM_new_background);
	}
	
	return newgroup;
}

// MW-2011-08-09: This is called by MCControl::attach to ensure that all the
//   children of the group have non-zero ids.
void MCGroup::ensureids(void)
{
	if (controls == NULL)
		return;

	MCStack *t_stack;
	t_stack = getstack();

	MCControl *t_control;
	t_control = controls;
	do
	{
		if (t_control -> getid() == 0)
			t_control -> setid(t_stack -> newid());

		if (t_control -> gettype() == CT_GROUP)
			static_cast<MCGroup *>(t_control) -> ensureids();

		t_control = t_control -> next();
	}
	while(t_control != controls);
}

MCControl *MCGroup::findnum(Chunk_term type, uint2 &num)
{
	if (type == CT_GROUP || type == CT_LAYER)
		if (num-- == 0)
			return this;
	if (controls != NULL)
	{
		MCControl *cptr = controls;
		do
		{
			MCControl *foundobj;
			if ((foundobj = cptr->findnum(type, num)) != NULL)
				return foundobj;
			cptr = cptr->next();
		}
		while (cptr != controls);
	}
	return NULL;
}

MCControl *MCGroup::findname(Chunk_term type, const MCString &inname)
{
	if (type == CT_GROUP || type == CT_LAYER)
		if (MCU_matchname(inname, CT_GROUP, getname()))
			return this;
	if (controls != NULL)
	{
		MCControl *cptr = controls;
		do
		{
			MCControl *foundobj;
			if ((foundobj = cptr->findname(type, inname)) != NULL)
				return foundobj;
			cptr = cptr->next();
		}
		while (cptr != controls);
	}
	return NULL;
}

MCControl *MCGroup::findchildwithid(Chunk_term type, uint4 p_id)
{
	if ((type == CT_GROUP || type == CT_LAYER) && (p_id == obj_id))
		return this;

	if (controls != NULL)
	{
		MCControl *cptr = controls;
		do
		{
			MCControl *foundobj;

			if (cptr -> gettype() == CT_GROUP)
			{
				MCGroup *t_group;
				t_group = (MCGroup *)cptr;
				foundobj = t_group -> findchildwithid(type, p_id);
			}
			else
				foundobj = cptr -> findid(type, p_id, False);

			if (foundobj != NULL)
				return foundobj;

			cptr = cptr -> next();
		}
		while (cptr != controls);
	}
	return NULL;
}


MCControl *MCGroup::findid(Chunk_term type, uint4 inid, Boolean alt)
{
	if ((type == CT_GROUP || type == CT_LAYER)
	        && (inid == obj_id || alt && inid == altid))
		return this;

	if (controls != NULL && (alt || type == CT_IMAGE))
	{
		MCControl *cptr = controls;
		do
		{
			MCControl *foundobj;
			if ((foundobj = cptr->findid(type, inid, alt)) != NULL)
				return foundobj;
			cptr = cptr->next();
		}
		while (cptr != controls);
	}
	return NULL;
}

Boolean MCGroup::count(Chunk_term type, MCObject *stop, uint2 &num)
{
	if (type == CT_CARD)
	{
		getstack()->setbackground(this);
		getstack()->count(CT_CARD, CT_UNDEFINED, stop, num);
		getstack()->clearbackground();
		return True;
	}
	if (type == CT_GROUP || type == CT_LAYER)
		num++;
	if (stop == this)
		return True;
	if (controls != NULL)
	{
		MCControl *cptr = controls;
		do
		{
			if (cptr->count(type, stop, num))
				return True;
			cptr = cptr->next();
		}
		while (cptr != controls);
	}
	return False;
}

Boolean MCGroup::maskrect(const MCRectangle &srect)
{
	if (!(flags & F_VISIBLE || MCshowinvisibles))
		return False;
	if (MCControl::maskrect(srect))
	{
		if (flags & F_VSCROLLBAR)
		{
			MCRectangle vrect = MCU_intersect_rect(vscrollbar->getrect(), srect);
			if (vrect.width != 0 && vrect.height != 0)
				return True;
		}
		if (flags & F_HSCROLLBAR)
		{
			MCRectangle hrect = MCU_intersect_rect(hscrollbar->getrect(), srect);
			if (hrect.width != 0 && hrect.height != 0)
				return True;
		}
		if (controls != NULL)
		{
			MCControl *cptr = controls;
			do
			{
				if (cptr->maskrect(srect))
					return True;
				cptr = cptr->next();
			}
			while (cptr != controls);
		}
		else
			// In browse mode, empty groups should not hit-test - this stops the problem
			// of a lack of *move messages being sent to the card when the pointer is over
			// the group.
			return getstack() -> gettool(this) == T_BROWSE ? False : True;
	}
	return False;
}

void MCGroup::installaccels(MCStack *stack)
{
	if (controls != NULL)
	{
		MCControl *cptr = controls;
		do
		{
			cptr->installaccels(stack);
			cptr = cptr->next();
		}
		while (cptr != controls);
	}
}

void MCGroup::removeaccels(MCStack *stack)
{
	if (controls != NULL)
	{
		MCControl *cptr = controls;
		do
		{
			cptr->removeaccels(stack);
			cptr = cptr->next();
		}
		while (cptr != controls);
	}
}

MCCdata *MCGroup::getdata(uint4 cardid, Boolean clone)
{
	MCCdata *data = NULL;
	if (controls != NULL)
	{
		MCControl *cptr = controls;
		do
		{
			MCCdata *dptr = cptr->getdata(cardid, clone);
			if (dptr != NULL)
				dptr->appendto(data);
			cptr = cptr->next();
		}
		while (cptr != controls);
	}
	return data;
}

void MCGroup::replacedata(MCCdata *&data, uint4 cardid)
{
	if (controls != NULL)
	{
		MCControl *cptr = controls;
		do
		{
			cptr->replacedata(data, cardid);
			cptr = cptr->next();
		}
		while (cptr != controls);
	}
}

void MCGroup::compactdata()
{
	if (controls != NULL)
	{
		MCControl *cptr = controls;
		do
		{
			cptr->compactdata();
			cptr = cptr->next();
		}
		while (cptr != controls);
	}
}

void MCGroup::resetfontindex(MCStack *oldstack)
{
	if (controls != NULL)
	{
		MCControl *cptr = controls;
		do
		{
			cptr->resetfontindex(oldstack);
			cptr = cptr->next();
		}
		while (cptr != controls);
	}
	MCControl::resetfontindex(oldstack);
}

Exec_stat MCGroup::hscroll(int4 offset, Boolean doredraw)
{
	int4 oldx = scrollx;
	scrollx += offset;
	
	// MW-2010-12-10: [[ Out-of-bound Scroll ]] Only bound the scroll values if the
	//   relevant scrollbar is showing or unboundedHScroll is false.
	if ((getflag(F_HSCROLLBAR) || !getflag(F_UNBOUNDED_HSCROLL)))
	{
		MCRectangle grect = getgrect();
		if (scrollx < 0 || minrect.width < grect.width)
			scrollx = 0;
		else if (scrollx > minrect.width - grect.width)
			scrollx = minrect.width - grect.width;
	}
	offset = scrollx - oldx;
	if (offset == 0 || controls == NULL)
		return ES_NORMAL;
	MCControl *cptr = controls;
	do
	{
		MCRectangle crect = cptr->getrect();
		crect.x -= offset;
		cptr->setrect(crect);
		cptr = cptr->next();
	}
	while (cptr != controls);
	minrect.x -= offset;
	if (opened && doredraw)
	{
		// MW-2011-09-07: [[ Layers ]] Notify that the layer has scrolled.
		layer_scrolled();
	}
	if (opened)
		return message_with_args(MCM_scrollbar_drag, scrollx);
	return ES_NORMAL;
}

Exec_stat MCGroup::vscroll(int4 offset, Boolean doredraw)
{
	int4 oldy = scrolly;
	scrolly += offset;
	
	// MW-2010-12-10: [[ Out-of-bound Scroll ]] Only bound the scroll values if the
	//   relevant scrollbar is showing or unboundedVScroll is false.
	if (getflag(F_VSCROLLBAR) || !getflag(F_UNBOUNDED_VSCROLL))
	{
		MCRectangle grect = getgrect();
		if (scrolly < 0 || minrect.height < grect.height)
			scrolly = 0;
		else if (scrolly > minrect.height - grect.height)
			scrolly = minrect.height - grect.height;
	}
	offset = scrolly - oldy;
	if (offset == 0 || controls == NULL)
		return ES_NORMAL;
	MCControl *cptr = controls;
	do
	{
		MCRectangle crect = cptr->getrect();
		crect.y -= offset;
		cptr->setrect(crect);
		cptr = cptr->next();
	}
	while (cptr != controls);
	minrect.y -= offset;
	if (opened && doredraw)
	{
		// MW-2011-09-07: [[ Layers ]] Notify that the layer has scrolled.
		layer_scrolled();
	}
	if (opened)
		return message_with_args(MCM_scrollbar_drag, scrolly);
	return ES_NORMAL;
}

void MCGroup::readscrollbars()
{
	real8 pos;
	if (flags & F_HSCROLLBAR)
	{
		hscrollbar->getthumb(pos);
		hscroll((int4)pos - scrollx, True);
	}
	if (flags & F_VSCROLLBAR)
	{
		vscrollbar->getthumb(pos);
		vscroll((int4)pos - scrolly, True);
	}
}

void MCGroup::resetscrollbars(Boolean move)
{
	if (!(flags & F_SCROLLBAR) && minrect.width == 0)
		return;
	MCRectangle grect = getgrect();
	if (flags & F_SHOW_BORDER)
		grect = MCU_reduce_rect(grect, -borderwidth);
	uint2 fheight = fontheight == 0 ? 8 : fontheight;

	if (flags & F_HSCROLLBAR)
	{
		scrollx = MCU_max(0, grect.x - minrect.x);
		if (move)
			hscrollbar->movethumb(scrollx);
		else
			hscrollbar->setthumb(scrollx, grect.width, fheight, scrollx
			                     + MCU_max(grect.width,
			                               minrect.x + minrect.width - grect.x));
	}
	if (flags & F_VSCROLLBAR)
	{
		scrolly = MCU_max(0, grect.y - minrect.y);
		if (move)
			vscrollbar->movethumb(scrolly);
		else
			vscrollbar->setthumb(scrolly, grect.height, fheight, scrolly
			                     + MCU_max(grect.height,
			                               minrect.y + minrect.height - grect.y));
	}
}

void MCGroup::setsbrects()
{
	MCRectangle grect = rect;
	if (flags & F_SHOW_BORDER)
		grect = MCU_reduce_rect(grect, borderwidth);
	if (flags & F_SHOW_NAME)
	{
		// MW-2012-03-16: [[ Bug ]] Make sure we have a font to use to
		//   calculate the label height.
		// MW-2013-08-22: [[ MeasureText ]] Update to use new object method.
		MCRectangle t_font_metrics;
		t_font_metrics = measuretext(MCnullmcstring, false);
		
		int32_t fheight;
		fheight = t_font_metrics . height;
		grect.y += fheight >> 1;
		grect.height -= fheight >> 1;
	}
	if (flags & F_HSCROLLBAR)
	{
		MCRectangle trect = grect;
		trect.y += trect.height - scrollbarwidth;
		trect.height = scrollbarwidth;
		if (flags & F_VSCROLLBAR)
			trect.width -= scrollbarwidth - 1;
		hscrollbar->setrect(trect);
	}
	if (flags & F_VSCROLLBAR)
	{
		MCRectangle trect = grect;
		trect.x += trect.width - scrollbarwidth;
		trect.width = scrollbarwidth;
		if (flags & F_HSCROLLBAR)
			trect.height -= scrollbarwidth - 1;
		vscrollbar->setrect(trect);
	}
	resetscrollbars(False);
}

MCControl *MCGroup::getchild(Chunk_term etype, const MCString &expression,
                             Chunk_term otype, Chunk_term ptype)
{
	if (otype < CT_GROUP || controls == NULL)
		return NULL;

	MCControl *cptr = controls;
	uint2 num = 0;

	switch (etype)
	{
	case CT_FIRST:
	case CT_SECOND:
	case CT_THIRD:
	case CT_FOURTH:
	case CT_FIFTH:
	case CT_SIXTH:
	case CT_SEVENTH:
	case CT_EIGHTH:
	case CT_NINTH:
	case CT_TENTH:
		num = etype - CT_FIRST;
		break;
	case CT_LAST:
	case CT_MIDDLE:
	case CT_ANY:
		count(otype, NULL, num);
		// MW-2007-08-30: [[ Bug 4152 ]] If we're counting groups, we get one too many as it
		//   includes the owner - thus we adjust (this means you can do 'the last group of group ...')
		if (otype == CT_GROUP)
			num--;
		switch (etype)
		{
		case CT_LAST:
			num--;
			break;
		case CT_MIDDLE:
			num >>= 1;
			break;
		case CT_ANY:
			num = MCU_any(num);
			break;
		default:
			break;
		}
		break;
	case CT_ID:
		uint4 tofindid;
		if (MCU_stoui4(expression, tofindid))
			do
			{
				MCControl *foundobj;
				if ((foundobj = cptr->findid(otype, tofindid, True)) != NULL)
					return foundobj;
				cptr = cptr->next();
			}
			while (cptr != controls);
		return NULL;
	case CT_EXPRESSION:
		if (MCU_stoui2(expression, num))
		{
			if (num < 1)
				return NULL;
			num--;
		}
		else
		{
			do
			{
				MCControl *foundobj;
				if ((foundobj = cptr->findname(otype, expression)) != NULL)
					return foundobj;
				cptr = cptr->next();
			}
			while (cptr != controls);
			return NULL;
		}
		break;
	default:
		return NULL;
	}
	do
	{
		MCControl *foundobj;
		if ((foundobj = cptr->findnum(otype, num)) != NULL)
			return foundobj;
		cptr = cptr->next();
	}
	while (cptr != controls);
	return NULL;
}

void MCGroup::makegroup(MCControl *newcontrols, MCObject *newparent)
{
	if (parent->getstack() != newparent->getstack())
		obj_id = newparent->getstack()->newid();
	parent = newparent;
	setcontrols(newcontrols);
	computeminrect(False);
	attach(OP_NONE, false);
	if (parent->gettype() == CT_CARD)
		getcard()->relayer(this, number);
}

MCControl *MCGroup::getcontrols()
{
	getcard()->count(CT_LAYER, CT_UNDEFINED, this, number, True); // save number
	return controls;
}

void MCGroup::setcontrols(MCControl *newcontrols)
{
	controls = newcontrols;
	if (controls != NULL)
	{
		MCControl *cptr = controls;
		do
		{
			cptr->setparent(this);
			
			// MW-2011-09-21: [[ Layers ]] Make sure the controls all have their
			//   attrs reset.
			cptr -> layer_resetattrs();

			cptr = cptr->next();
		}
		while (cptr != controls);
	}
}

// MW-2007-07-09: [[ Bug 3547 ]] When going between cards an old card and a new
//   card can be simultaneously open. This results in any shared groups and their
//   controls having opened > 1. Thus, when we append a new control we need to
//   make sure it, also, is opened the appropriate number of times.
void MCGroup::appendcontrol(MCControl *newcontrol)
{
	newcontrol->appendto(controls);
	computeminrect(False);
	if (opened)
	{
		while(newcontrol -> getopened() < opened)
			newcontrol->open();

		// MW-2011-09-21: [[ Layers ]] Make sure the control has its attrs
		//   reset.
		newcontrol -> layer_resetattrs();

		// MW-2011-08-18: [[ Layers ]] Invalidate the new object.
		newcontrol -> layer_redrawall();
	}
}

void MCGroup::removecontrol(MCControl *cptr, Boolean cf)
{
	cptr = cptr->remove(controls);
	if (opened)
	{
		cptr->close();
		if (cf)
			clearfocus(cptr);
	}
	cptr->setparent(getstack());

	// MW-2011-08-18: [[ Layers ]] Make sure the area where the control used to be is invalidated.
	if (!computeminrect(False))
		layer_redrawrect(cptr -> geteffectiverect());
}

MCControl *MCGroup::getkfocused()
{
	return kfocused;
}

MCControl *MCGroup::getmfocused()
{
	return mfocused;
}

void MCGroup::clearfocus(MCControl *cptr)
{
	if (cptr == mfocused)
	{
		mfocused = NULL;
		mgrabbed = False;
	}
	if (cptr == oldkfocused)
		oldkfocused = NULL;
	if (cptr == newkfocused)
		newkfocused = NULL;

	// This solves the problem of a grouped control being deleted, but leaving the card 'thinking'
	// the group still contains the focused object.
	//
	// MW-2007-07-05: [[ Bug 5198 ]] Deleting a control in a group causes focus
	//   to be removed from the group but not from the currently focused control
	//   (if it is not the one deleted).
	//
	if (cptr == kfocused)
	{
		kfocused = NULL;
		if (parent -> gettype() == CT_CARD)
			static_cast<MCCard *>(parent) -> erasefocus(this);
		else
			static_cast<MCGroup *>(parent) -> clearfocus(this);
	}
}

void MCGroup::radio(uint4 parid, MCControl *focused)
{
	if (flags & F_RADIO_BEHAVIOR && focused != NULL)
	{
		if (focused->gettype() == CT_BUTTON)
		{
			MCButton *bptr = (MCButton *)focused;
			if (bptr->getstyle() == F_RADIO
			        && (bptr->gethilite(parid)
			            || (bptr->getstate(CS_MFOCUSED)
			                && MCU_point_in_rect(bptr->getrect(), mx, my))))
			{
				MCControl *cptr = controls;
				do
				{
					if (cptr != focused && cptr->gettype() == CT_BUTTON)
					{
						bptr = (MCButton *)cptr;
						// MH-2007-03-20: [[ Bug 3664 ]] Hiliting a radio button unhilites all other buttons in the group if radioBehavior is set to true.
						if (bptr -> getstyle() == F_RADIO)
							bptr->resethilite(parid, False);
					}
					cptr = cptr->next();
				}
				while (cptr != controls);
			}
		}
	}
}

uint2 MCGroup::gethilited(uint4 parid)
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
				        && bptr->gethilite(parid))
					return which;
				which++;
			}
			cptr = cptr->next();
		}
		while (cptr != controls);
	}
	return 0;
}


MCButton *MCGroup::gethilitedbutton(uint4 parid)
{
	if (controls != NULL)
	{
		MCControl *cptr = controls;
		do
		{
			if (cptr->gettype() == CT_BUTTON)
			{
				MCButton *bptr = (MCButton *)cptr;
				if (!(mgrabbed == True && cptr == mfocused)
				        && bptr->gethilite(parid))
					return bptr;
			}
			cptr = cptr->next();
		}
		while (cptr != controls);
	}
	return NULL;
}

uint4 MCGroup::gethilitedid(uint4 parid)
{
	MCButton *bptr = gethilitedbutton(parid);
	if (bptr != NULL)
		return bptr->getid();
	return 0;
}

MCNameRef MCGroup::gethilitedname(uint4 parid)
{
	MCButton *bptr = gethilitedbutton(parid);
	if (bptr != NULL)
		return bptr->getname();
	return kMCEmptyName;
}

void MCGroup::sethilited(uint4 parid, uint2 toset)
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
				bptr->resethilite(parid, toset == which);
				which++;
			}
			cptr = cptr->next();
		}
		while (cptr != controls);
	}
}

void MCGroup::sethilitedid(uint4 parid, uint4 toset)
{
	if (controls != NULL)
	{
		MCControl *cptr = controls;
		do
		{
			if (cptr->gettype() == CT_BUTTON)
			{
				MCButton *bptr = (MCButton *)cptr;
				bptr->resethilite(parid, toset == bptr->getid());
			}
			cptr = (MCControl *)cptr->next();
		}
		while (cptr != controls);
	}
}

void MCGroup::sethilitedname(uint4 parid, MCNameRef bname)
{
	if (controls != NULL)
	{
		MCControl *cptr = controls;
		do
		{
			if (cptr->gettype() == CT_BUTTON)
			{
				MCButton *bptr = (MCButton *)cptr;
				bptr->resethilite(parid, bptr->hasname(bname));
			}
			cptr = cptr->next();
		}
		while (cptr != controls);
	}
}


void MCGroup::setchildprops(uint4 parid, Properties p, MCExecPoint &ep)
{
	// MW-2011-08-18: [[ Redraw ]] Update to use redraw.
	MCRedrawLockScreen();
	if (controls != NULL)
	{
		MCControl *cptr = controls;
		MCerrorlock++;
		do
		{
			cptr->setprop(parid, p, ep, False);
			// MM-2012-09-05: [[ Property Listener ]] Make sure any group props which also effect children send the propertyChanged message to listeners of the children.
			cptr -> signallisteners(p);
			cptr = cptr->next();
		}
		while (cptr != controls);
		MCerrorlock--;
	}
	if (p == P_3D || p == P_OPAQUE || p == P_ENABLED || p == P_DISABLED)
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
	MCRedrawUnlockScreen();
}

MCRectangle MCGroup::getgrect()
{
	MCRectangle grect = rect;
	if (flags & F_SHOW_NAME)
	{
		// MW-2012-03-06: [[ Bug 10053 ]] The grect() can be requested when we don't have
		//   the font mapped (i.e. not open) so map/unmap the font as required.
		// MW-2012-03-16: [[ Bug ]] Make sure we only map/unmap a font if the group is
		//   closed *and* has no font since hscroll/vscroll set opened to 0 temporarily.
		// MW-2013-08-23: [[ MeasureText ]] Update to use measuretext() method for
		//   better encapsulation.
		MCRectangle t_font_metrics;
		t_font_metrics = measuretext(MCnullmcstring, false);
		
		int32_t fascent;
		fascent = -t_font_metrics . y;
		
		grect.y += fascent;
		grect.height -= fascent;
	}
	if (flags & F_HSCROLLBAR)
		grect.height -= scrollbarwidth;
	if (flags & F_VSCROLLBAR)
		grect.width -= scrollbarwidth;
	if (flags & F_SHOW_BORDER)
		grect = MCU_reduce_rect(grect, borderwidth);
	return grect;
}

void MCGroup::computecrect()
{
	minrect.x = rect.x + leftmargin;
	minrect.y = rect.y + topmargin;
	minrect.width = minrect.height = 0;
	if (controls != NULL)
	{
		MCControl *cptr = controls;
		do
		{
			if (cptr->getflag(F_VISIBLE))
				if (minrect.width == 0)
					minrect = cptr->getrect();
				else
					minrect = MCU_union_rect(cptr->getrect(), minrect);
			cptr = cptr->next();
		}
		while (cptr != controls);
	}
}

Boolean MCGroup::computeminrect(Boolean scrolling)
{
	MCRectangle oldrect = rect;
	MCRectangle oldmin = minrect;
	if (!(flags & F_BOUNDING_RECT))
	{
		computecrect();
		minrect.x -= leftmargin;
		minrect.y -= topmargin;
		minrect.width += leftmargin + rightmargin;
		minrect.height += topmargin + bottommargin;
		if (flags & F_SHOW_BORDER)
			minrect = MCU_reduce_rect(minrect, -borderwidth);
	}
	if (flags & F_LOCK_LOCATION)
	{
		boundcontrols();
		if (scrolling && flags & F_BOUNDING_RECT)
		{
			MCRectangle grect = getgrect();
			if (mx < grect.x)
				hscroll((mx - grect.x) >> 1, False);
			else
				if (mx > grect.x + grect.width)
					hscroll((mx - (grect.x + grect.width)) >> 1, False);
			if (my < grect.y)
				vscroll((my - grect.y) >> 1, False);
			else
				if (my > grect.y + grect.height)
					vscroll((my - (grect.y + grect.height)) >> 1, False);
		}
		resetscrollbars(False);
	}
	else
	{
		rect = minrect;
		if (flags & F_SHOW_NAME)
		{
			// MW-2012-03-16: [[ Bug ]] Make sure we have a font to use to
			//   calculate the label height.
			// MW-2013-08-22: [[ MeasureText ]] Update to use new object method.
			MCRectangle t_font_metrics;
			t_font_metrics = measuretext(MCnullmcstring, false);
			
			int32_t fheight;
			fheight = t_font_metrics . height;
			rect.y -= fheight - borderwidth;
			rect.height += fheight - borderwidth;
		}
		if (flags & F_HSCROLLBAR)
			rect.height += scrollbarwidth;
		if (flags & F_VSCROLLBAR)
			rect.width += scrollbarwidth;
		scrollx = scrolly = 0;
		setsbrects();
	}

	// MW-2011-10-03: [[ Layers ]] If the distance between the old content
	//   origin and new content origin has changed, notify the layer.
	if (layer_isscrolling())
	{
		if (oldmin . x != minrect . x || oldmin . y != minrect . y)
			layer_contentoriginchanged(oldmin . x - minrect . x, oldmin . y - minrect . y);
	}

	if (oldrect.x != rect.x || oldrect.y != rect.y
	        || oldrect.width != rect.width || oldrect.height != rect.height)
	{
		Bool t_all;

		uint4 oldstate = state;
		if (scrolling)
			state |= CS_MOVE;
		if (!resizeparent())
		{
			// MW-2011-08-18: [[ Layers ]] Notify of rect change and invalidate.
			if (!layer_isscrolling())
			{
				layer_rectchanged(oldrect, true);
				t_all = True;
			}
			else
			{
				MCRectangle t_outer_rect;
				MCRectangle t_inner_rect;
				t_outer_rect = MCU_union_rect(rect, oldrect);
				t_inner_rect = MCU_intersect_rect(rect, oldrect);
				
				// MW-2011-10-04: Make sure we clean out the content of the scrolling group that
				//   has 'appeared. The 'layer_rectchanged' will cause the card to be updated if
				//   necessary.
				if (t_outer_rect . x < t_inner_rect . x)
					layer_dirtycontentrect(MCU_make_rect(t_outer_rect . x, t_outer_rect . y, t_inner_rect . x - t_outer_rect . x, t_outer_rect . height), false);
				if (t_outer_rect . x + t_outer_rect . width > t_inner_rect . x + t_inner_rect . width)
					layer_dirtycontentrect(MCU_make_rect(t_inner_rect . x + t_inner_rect . width, rect . y, (t_outer_rect . x + t_outer_rect . width) - (t_inner_rect . x + t_inner_rect . width), t_outer_rect . height), false);
				if (t_outer_rect . y < t_inner_rect . y)
					layer_dirtycontentrect(MCU_make_rect(t_outer_rect . x, t_outer_rect . y, t_outer_rect . width, t_inner_rect . y - t_outer_rect . y), false);
				if (t_outer_rect . x + t_outer_rect . width > t_inner_rect . x + t_inner_rect . width)
					layer_dirtycontentrect(MCU_make_rect(t_outer_rect . x, t_inner_rect . y + t_inner_rect . height, t_outer_rect . width, (t_outer_rect . y + t_outer_rect . height) - (t_inner_rect . y + t_inner_rect . height)), false);
				
				layer_rectchanged(oldrect, false);
				t_all = False;
			}
		}
		else
			t_all = True;
		state = oldstate;
		return t_all;
	}
	else if ((flags & F_HSCROLLBAR || flags & F_VSCROLLBAR)
				&& (oldmin.x != minrect.x || oldmin.y != minrect.y
					|| oldmin.width != minrect.width
					|| oldmin.height != minrect.height))
	{
		// MW-2011-08-18: [[ Layers ]] Invalidate the whole object.
		layer_redrawall();
	}
	return False;
}

void MCGroup::boundcontrols()
{
	if ((flags & F_HSCROLLBAR || flags & F_VSCROLLBAR)
	        && !(flags & F_BOUNDING_RECT))
	{
		int2 dx = minrect.x > rect.x ? minrect.x - rect.x : 0;
		int2 dy = minrect.y > rect.y ? minrect.y - rect.y : 0;
		if (dx != 0 || dy != 0)
		{
			MCControl *cptr = controls;
			if (controls)
			{
				do
				{
					MCRectangle trect = cptr->getrect();
					trect.x -= dx;
					trect.y -= dy;
					cptr->setrect(trect);
					// MM-2012-09-05: [[ Property Listener ]] Scrolling a group will effect the loc propeties of its children.
					cptr -> signallisteners(P_LOCATION);
					cptr = cptr->next();
				}
				while (cptr != controls);
			}
			minrect.x -= dx;
			minrect.y -= dy;
		}
	}
}


//-----------------------------------------------------------------------------
//  Redraw Management

// if (flags & F_OPAQUE)
//   if theme && borderwidth == DEFAULT_BORDER && no back colour && no back pixmap && show border && borderwidth && is 3d && theme has group frame
//     drawthemegroup(False)
//   else if mac && no back colour && no back pix && stack picture && screen is target then
//     copy stack background
//   else 
//     fill with background

// MW-2011-09-06: [[ Redraw ]] Added 'sprite' option - if true, ink and opacity are not set.
void MCGroup::draw(MCDC *dc, const MCRectangle& p_dirty, bool p_isolated, bool p_sprite)
{
	MCRectangle dirty;
	dirty = p_dirty;

	if (!p_isolated)
	{
		// MW-2011-09-06: [[ Redraw ]] If rendering as a sprite, don't change opacity or ink.
		if (!p_sprite)
		{
			dc -> setopacity(blendlevel * 255 / 100);
			dc -> setfunction(ink);
		}

		// MW-2011-09-06: [[ Redraw ]] If rendering as a sprite, then we don't need a new layer.
		if (m_bitmap_effects == NULL)
			dc -> begin(p_sprite || ink == GXcopy ? false : true);
		else
		{
			if (!dc -> begin_with_effects(m_bitmap_effects, rect))
				return;
			dirty = dc -> getclip();
		}
	}

	// MW-2009-06-14: This flag will be set to true if when the group
	//   renders an opaque background.
	bool t_is_opaque;
	t_is_opaque = false;

	if (MCcurtheme != NULL &&
		getstack() -> hasmenubar() && hasname(getstack() -> getmenubar()) &&
		MCcurtheme -> drawmenubarbackground(dc, dirty, getrect(), MCmenubar == this))
	{
		// MW-2009-06-14: Vista menu backgrounds are assumed opaque
		t_is_opaque = true;
	}
	else if (flags & F_OPAQUE)
	{
		uint2 i;
		if (MCcurtheme && borderwidth == DEFAULT_BORDER &&
		        !getcindex(DI_BACK, i) && !getpindex(DI_BACK, i) &&
		        flags & F_SHOW_BORDER && borderwidth && flags & F_3D &&
		        MCcurtheme->iswidgetsupported(WTHEME_TYPE_GROUP_FRAME) )
		{
			// MW-2009-06-14: It appears that themed group borders are never opaque
			//   but this needs to be verified on the different platforms.
			drawthemegroup(dc,dirty,False);
		}
		else
		{
			if (MCcurtheme == NULL || !getstack() -> ismetal() ||
				!MCcurtheme -> drawmetalbackground(dc, dirty, rect, this))
			{
				setforeground(dc, DI_BACK, False);
				dc->fillrect(rect);
			}

			// MW-2009-06-14: Non-themed, opaque backgrounds are (unsurprisingly!) opaque.
			t_is_opaque = true;
		}
	}

	// MW-2009-06-14: Change opaqueness if it is so
	bool t_was_opaque;
	if (t_is_opaque)
		t_was_opaque = dc -> changeopaque(t_is_opaque);

	// MW-2011-10-03: If we are rendering in sprite mode, we don't clip further. (Previously
	//   we would clip to the 'minrect' in this case, but sometimes that isn't in sync with
	//   what we want to draw).
	MCRectangle drect;
	if (!p_sprite)
		drect = MCU_intersect_rect(dirty, getgrect());
	else
		drect = dirty;
	if (drect.width != 0 && drect.height != 0 && controls != NULL)
	{
		MCControl *cptr = controls;
		do
		{
			if (cptr -> getopened() != 0 && (MCshowinvisibles || cptr -> getflag(F_VISIBLE)))
			{
				MCRectangle trect = MCU_intersect_rect(drect, cptr -> geteffectiverect());
				if (trect.width != 0 && trect.height != 0)
				{
					dc -> setopacity(255);
					dc -> setfunction(GXcopy);
					dc -> setclip(trect);
					cptr -> draw(dc, trect, false, false);
				}

			}

			cptr = cptr->next();
		}
		while (cptr != controls);
	}

	if (t_is_opaque)
		dc -> changeopaque(t_was_opaque);

	drect = MCU_intersect_rect(dirty, rect);
	dc->setclip(drect);

	if (flags & F_HSCROLLBAR)
	{
		MCRectangle hrect = MCU_intersect_rect(hscrollbar->getrect(), drect);
		if (hrect.width != 0 && hrect.height != 0)
			hscrollbar->draw(dc, hrect, false, false);
	}

	if (flags & F_VSCROLLBAR)
	{
		MCRectangle vrect = MCU_intersect_rect(vscrollbar->getrect(), drect);
		if (vrect.width != 0 && vrect.height != 0)
			vscrollbar->draw(dc, vrect, false, false);
	}

	dc -> setopacity(255);
	dc -> setfunction(GXcopy);
	dc -> setclip(dirty);
	drawbord(dc, dirty);

	if (!p_isolated)
	{
		dc -> end();

		if (getstate(CS_SELECTED))
			drawselected(dc);
	}
}

void MCGroup::drawthemegroup(MCDC *dc, const MCRectangle &dirty, Boolean drawframe)
{
	MCRectangle trect = rect;
	if (MCcurtheme && borderwidth == DEFAULT_BORDER &&
	        flags & F_SHOW_BORDER && borderwidth && flags & F_3D &&
	        MCcurtheme->iswidgetsupported(WTHEME_TYPE_GROUP_FRAME))
	{
		Boolean showtextlabel = flags & F_SHOW_NAME && (!isunnamed() || label != NULL);
		MCRectangle textrect;
		MCString slabel;
		bool isunicode;
		MCWidgetInfo winfo;
		int32_t fascent;
		winfo.type = WTHEME_TYPE_GROUP_FRAME;
		getwidgetthemeinfo(winfo);
		if (showtextlabel)
		{
			fascent = MCFontGetAscent(m_font);
			textrect.x = rect.x + labeloffset + borderwidth;
			textrect.y = rect.y + (borderwidth >> 1) - 2;
			textrect.height = fascent + MCFontGetDescent(m_font);
			if (label != NULL)
				slabel.set(label,labelsize), isunicode = hasunicode();
			else
				slabel = getname_oldstring(), isunicode = false;
			textrect.width = MCFontMeasureText(m_font, slabel.getstring(), slabel.getlength(), isunicode) + 4;
			//exclude text area from widget drawing region for those themes that draw text on top of frame.
			winfo.datatype = WTHEME_DATA_RECT;
			winfo.data = &textrect;
			trect.y += fascent >> 1;
			trect.height -= (fascent >> 1) + (borderwidth >> 1);
		}
		if (flags & F_OPAQUE && !drawframe)
			winfo.type = parent->gettype() == CT_GROUP && parent->getflag(F_SHOW_BORDER) ?  WTHEME_TYPE_SECONDARYGROUP_FILL: WTHEME_TYPE_GROUP_FILL;
		else
			winfo.type = parent->gettype() == CT_GROUP && parent->getflag(F_SHOW_BORDER) ?  WTHEME_TYPE_SECONDARYGROUP_FRAME: WTHEME_TYPE_GROUP_FRAME;
		if (!(flags & F_OPAQUE && drawframe))
			MCcurtheme->drawwidget(dc, winfo, trect);
		if (showtextlabel && drawframe)
		{
			setforeground(dc, DI_FORE, False);
			MCFontDrawText(m_font, slabel.getstring(), slabel.getlength(), isunicode,dc, textrect.x + 2, textrect.y + fascent, False);
		}
	}
}

void MCGroup::drawbord(MCDC *dc, const MCRectangle &dirty)
{
	if (MCcurtheme && borderwidth == DEFAULT_BORDER &&
	        flags & F_SHOW_BORDER && borderwidth && flags & F_3D &&
	        MCcurtheme->iswidgetsupported(WTHEME_TYPE_GROUP_FRAME))
	{
		drawthemegroup(dc, rect,True);
	}
	else
	{
		MCRectangle trect = rect;
		if (flags & F_SHOW_NAME && (!isunnamed() || label != NULL))
		{
			uint2 halfwidth = borderwidth >> 1;
			int32_t fascent, fdescent;
			fascent = MCFontGetAscent(m_font);
			fdescent = MCFontGetDescent(m_font);

			trect.y += fascent >> 1;
			trect.height -= fascent >> 1;
			MCRectangle textrect;
			textrect.x = rect.x + labeloffset + borderwidth;
			textrect.y = rect.y + (borderwidth >> 1) - 2;
			textrect.height = fascent + fdescent;

			MCString slabel;
			bool isunicode;
			if (label != NULL)
				slabel.set(label,labelsize), isunicode = hasunicode();
			else
				slabel = getname_oldstring(), isunicode = false;
			textrect.width = MCFontMeasureText(m_font, slabel.getstring(), slabel.getlength(), isunicode) + 4;

			if (flags & F_SHOW_BORDER)
			{
				uint2 halfwidth = borderwidth >> 1;
				MCPoint p[6];
				p[0].x = textrect.x;
				p[1].x = p[2].x = trect.x;
				p[3].x = p[4].x = trect.x + trect.width - halfwidth - 1;
				p[5].x = textrect.x + textrect.width;
				p[0].y = p[1].y = p[4].y = p[5].y = trect.y;
				p[2].y = p[3].y = trect.y + trect.height - halfwidth - 1;
				
				// MW-2008-03-05: [[ Bug 6004 ]] If halfwidth is 0, we just render a sunken
				//   border of width 1.
				if (halfwidth == 0)
				{
					if (getflag(F_3D))
					{
						setforeground(dc, DI_BOTTOM, False);
						dc -> drawlines(p, 3);
						dc -> drawlines(p + 4, 2);
						setforeground(dc, DI_TOP, False);
						dc -> drawlines(p + 2, 3);
					}
					else
					{
						setforeground(dc, DI_BORDER, False);
						dc -> drawlines(p, 6);
					}
				}
				else
				{
					if (flags & F_3D)
						setforeground(dc, DI_TOP, True);
					else
						setforeground(dc, DI_BORDER, False);
					uint2 i;
					for (i = 0 ; i < halfwidth ; i++)
					{
						dc->drawlines(p, 6);
						p[0].y++;
						p[5].y++;
						p[1].x++;
						p[1].y++;
						p[2].x++;
						p[2].y--;
						p[3].x--;
						p[3].y--;
						p[4].x--;
						p[4].y++;
					}
					if (flags & F_3D)
						setforeground(dc, DI_BOTTOM, True);
					MCSegment s[5];
					s[0].x1 = textrect.x;
					s[0].x2 = s[1].x1 = s[1].x2 = trect.x + halfwidth;
					s[0].y1 = s[0].y2 = s[1].y1 = s[4].y1 = s[4].y2= trect.y + halfwidth;
					s[1].y1 = trect.y + borderwidth;
					s[1].y2 = trect.y + trect.height - borderwidth - 1;
					s[2].x1 = trect.x;
					s[2].x2 = s[3].x1 = s[3].x2 = trect.x + trect.width - 1;
					s[2].y1 = s[2].y2 = s[3].y1 = trect.y + trect.height - 1;
					s[3].y1 = trect.y + trect.height - halfwidth - 1;
					s[3].y2 = trect.y;
					s[4].x2 = textrect.x + textrect.width;
					s[4].x1 = trect.x + trect.width - borderwidth - 1;
					for (i = 0 ; i < halfwidth ; i++)
					{
						dc->drawsegments(s, 5);
						s[0].y1++;
						s[0].y2++;
						s[1].x1++;
						s[1].x2++;
						s[2].y1--;
						s[2].y2--;
						s[3].x1--;
						s[3].x2--;
						s[4].y1++;
						s[4].y2++;
					}
				}
			}
			setforeground(dc, DI_FORE, False);
			MCFontDrawText(m_font, slabel.getstring(), slabel.getlength(), isunicode, dc, textrect.x + 2, textrect.y + fascent, False);
		}
		else
		{
			if (flags & F_SHOW_BORDER)
				if (flags & F_3D)
					draw3d(dc, trect, ETCH_SUNKEN, borderwidth);
				else
					drawborder(dc, trect, borderwidth);
		}
	}
}

//  Redraw Management
//-----------------------------------------------------------------------------

///////////////////////////////////////////////////////////////////////////////
//
//  SAVING AND LOADING
//

IO_stat MCGroup::extendedsave(MCObjectOutputStream& p_stream, uint4 p_part)
{
	return defaultextendedsave(p_stream, p_part);
}

IO_stat MCGroup::extendedload(MCObjectInputStream& p_stream, const char *p_version, uint4 p_remaining)
{
	return defaultextendedload(p_stream, p_version, p_remaining);
}

IO_stat MCGroup::save(IO_handle stream, uint4 p_part, bool p_force_ext)
{
	IO_stat stat;

	if ((stat = IO_write_uint1(OT_GROUP, stream)) != IO_NORMAL)
		return stat;
	if ((stat = MCObject::save(stream, p_part, p_force_ext)) != IO_NORMAL)
		return stat;
	if (flags & F_LABEL)
		if ((stat = IO_write_string(label, labelsize, stream, hasunicode())) != IO_NORMAL)
			return stat;
	if (flags & F_MARGINS)
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
	
	// MW-2012-02-22: [[ NoScrollSave ]] Save the old group offset, and apply the
	//   delta required by this group.
	MCPoint t_old_offset;
	t_old_offset = MCgroupedobjectoffset;
	MCgroupedobjectoffset . y += scrolly;
	MCgroupedobjectoffset . x += scrollx;
	if (flags & F_BOUNDING_RECT)
	{
		// MW-2012-02-22: [[ NoScrollSave ]] Adjust the minrect by the group offset.
		if ((stat = IO_write_int2(minrect.x + MCgroupedobjectoffset . x, stream)) != IO_NORMAL)
			return stat;
		if ((stat = IO_write_int2(minrect.y + MCgroupedobjectoffset . y, stream)) != IO_NORMAL)
			return stat;
		if ((stat = IO_write_uint2(minrect.width, stream)) != IO_NORMAL)
			return stat;
		if ((stat = IO_write_uint2(minrect.height, stream)) != IO_NORMAL)
			return stat;
	}
	if ((stat = savepropsets(stream)) != IO_NORMAL)
		return stat;
	if (vscrollbar != NULL)
	{
		if ((stat = vscrollbar->save(stream, p_part, p_force_ext)) != IO_NORMAL)
			return stat;
	}
	if (hscrollbar != NULL)
	{
		if ((stat = hscrollbar->save(stream, p_part, p_force_ext)) != IO_NORMAL)
			return stat;
	}
	if (controls != NULL)
	{
		MCControl *cptr = controls;
		do
		{
			if ((stat = cptr->save(stream, p_part, p_force_ext)) != IO_NORMAL)
				return stat;
			cptr = cptr->next();
		}
		while (cptr != controls);
	}
	// MW-2012-02-22: [[ NoScrollSave ]] Restore the previous setting of the object
	//   offset.
	MCgroupedobjectoffset = t_old_offset;

	if ((stat = IO_write_uint1(OT_GROUPEND, stream)) != IO_NORMAL)
		return stat;
	return IO_NORMAL;
}

IO_stat MCGroup::load(IO_handle stream, const char *version)
{
	IO_stat stat;
	if ((stat = MCObject::load(stream, version)) != IO_NORMAL)
		return stat;

	// MW-2011-08-10: [[ Groups ]] Make sure the group has F_GROUP_SHARED set
	//   if it is a background.
	if (isbackground())
		setflag(True, F_GROUP_SHARED);

	// MW-2012-02-17: [[ IntrinsicUnicode ]] If the unicode tag is set, then we are unicode.
	if ((m_font_flags & FF_HAS_UNICODE_TAG) != 0)
		m_font_flags |= FF_HAS_UNICODE;

	if (flags & F_LABEL)
	{
		uint4 tlabelsize;
		if ((stat = IO_read_string(label, tlabelsize, stream, hasunicode())) != IO_NORMAL)
			return stat;
		labelsize = tlabelsize;
	}
	if (flags & F_MARGINS)
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
			flags &= ~F_MARGINS;
	}
	if (flags & F_BOUNDING_RECT)
	{
		if ((stat = IO_read_int2(&minrect.x, stream)) != IO_NORMAL)
			return stat;
		if ((stat = IO_read_int2(&minrect.y, stream)) != IO_NORMAL)
			return stat;
		if ((stat = IO_read_uint2(&minrect.width, stream)) != IO_NORMAL)
			return stat;
		if ((stat = IO_read_uint2(&minrect.height, stream)) != IO_NORMAL)
			return stat;
	}
	if ((stat = loadpropsets(stream)) != IO_NORMAL)
		return stat;
	while (True)
	{
		uint1 type;
		if ((stat = IO_read_uint1(&type, stream)) != IO_NORMAL)
			return stat;
		switch (type)
		{
		case OT_GROUP:
			{
				MCGroup *newgroup = new MCGroup;
				newgroup->setparent(this);
				if ((stat = newgroup->load(stream, version)) != IO_NORMAL)
				{
					delete newgroup;
					return stat;
				}
				MCControl *newcontrol = newgroup;
				
				// MW-2011-08-09: [[ Groups ]] Nested groups are not allowed to be shared.
				newcontrol -> setflag(False, F_GROUP_SHARED);
				// MW-2011-08-09: [[ Groups ]] Nested groups are not allowed to be backgrounds.
				newcontrol -> setflag(True, F_GROUP_ONLY);
				
				newcontrol->appendto(controls);
			}
			break;
		case OT_BUTTON:
			{
				MCButton *newbutton = new MCButton;
				newbutton->setparent(this);
				if ((stat = newbutton->load(stream, version)) != IO_NORMAL)
				{
					delete newbutton;
					return stat;
				}
				MCControl *cptr = (MCControl *)newbutton;
				cptr->appendto(controls);
			}
			break;
		case OT_FIELD:
			{
				MCField *newfield = new MCField;
				newfield->setparent(this);
				if ((stat = newfield->load(stream, version)) != IO_NORMAL)
				{
					delete newfield;
					return stat;
				}
				newfield->appendto(controls);
			}
			break;
		case OT_IMAGE:
			{
				MCImage *newimage = new MCImage;
				newimage->setparent(this);
				if ((stat = newimage->load(stream, version)) != IO_NORMAL)
				{
					delete newimage;
					return stat;
				}
				MCControl *cptr = (MCControl *)newimage;
				cptr->appendto(controls);
			}
			break;
		case OT_SCROLLBAR:
			{
				MCScrollbar *newscrollbar = new MCScrollbar;
				newscrollbar->setparent(this);
				if ((stat = newscrollbar->load(stream, version)) != IO_NORMAL)
				{
					delete newscrollbar;
					return stat;
				}
				if (flags & F_VSCROLLBAR && vscrollbar == NULL)
				{
					vscrollbar = newscrollbar;
					vscrollbar->allowmessages(False);
					scrollbarwidth = vscrollbar->getrect().width;
					vscrollbar->setflag(flags & F_DISABLED, F_DISABLED);
				}
				else if (flags & F_HSCROLLBAR && hscrollbar == NULL)
				{
					hscrollbar = newscrollbar;
					hscrollbar->allowmessages(False);
					hscrollbar->setflag(flags & F_DISABLED, F_DISABLED);
				}
				else
					newscrollbar->appendto(controls);
			}
			break;
		case OT_GRAPHIC:
			{
				MCGraphic *newgraphic = new MCGraphic;
				newgraphic->setparent(this);
				if ((stat = newgraphic->load(stream, version)) != IO_NORMAL)
				{
					delete newgraphic;
					return stat;
				}
				newgraphic->appendto(controls);
			}
			break;
		case OT_MCEPS:
			{
				MCEPS *neweps = new MCEPS;
				neweps->setparent(this);
				if ((stat = neweps->load(stream, version)) != IO_NORMAL)
				{
					delete neweps;
					return stat;
				}
				neweps->appendto(controls);
			}
			break;
		case OT_MAGNIFY:
			{
				MCMagnify *newmag = new MCMagnify;
				newmag->setparent(this);
				if ((stat = newmag->load(stream, version)) != IO_NORMAL)
				{
					delete newmag;
					return stat;
				}
				newmag->appendto(controls);
			}
			break;
		case OT_COLORS:
			{
				MCColors *newcolors = new MCColors;
				newcolors->setparent(this);
				if ((stat = newcolors->load(stream, version)) != IO_NORMAL)
				{
					delete newcolors;
					return stat;
				}
				newcolors->appendto(controls);
			}
			break;
		case OT_PLAYER:
			{
				MCPlayer *newplayer = new MCPlayer;
				newplayer->setparent(this);
				if ((stat = newplayer->load(stream, version)) != IO_NORMAL)
				{
					delete newplayer;
					return stat;
				}
				newplayer->appendto(controls);
			}
			break;
		case OT_GROUPEND:
			if (strncmp(version, "1.0", 3) == 0)
			{
				computecrect();
				if (rect.x == minrect.x && rect.y == minrect.y
				        && rect.width == minrect.width && rect.height == minrect.height)
					flags &= ~(F_SHOW_BORDER | F_OPAQUE);
			}
			return IO_NORMAL;
		default:
			MCS_seek_cur(stream, -1);
			return IO_NORMAL;
		}
	}
	return IO_NORMAL;
}

Exec_stat MCGroup::opencontrols(bool p_is_preopen)
{
	if (controls != NULL)
	{
		MCControl *cptr = controls;
		do
		{
			if (cptr -> gettype() == CT_GROUP)
			{
				if (cptr -> conditionalmessage(p_is_preopen ? HH_PREOPEN_CONTROL : HH_OPEN_CONTROL, p_is_preopen ? MCM_preopen_control : MCM_open_control) == ES_ERROR)
					return ES_ERROR;
			
				if (cptr -> gettype() == CT_GROUP)
					if (static_cast<MCGroup *>(cptr) -> opencontrols(p_is_preopen) == ES_ERROR)
						return ES_ERROR;
			}
				
			cptr = cptr->next();
		}
		while (cptr != controls);
	}
	
	return ES_NORMAL;
}

Exec_stat MCGroup::closecontrols(void)
{
	if (controls != NULL)
	{
		MCControl *cptr = controls;
		do
		{
			if (cptr -> gettype() == CT_GROUP)
			{
				if (static_cast<MCGroup *>(cptr) -> closecontrols() == ES_ERROR)
					return ES_ERROR;
					
				if (cptr -> conditionalmessage(HH_CLOSE_CONTROL, MCM_close_control) == ES_ERROR)
					return ES_ERROR;
			}
				
			cptr = cptr->next();
		}
		while (cptr != controls);
	}

	return ES_NORMAL;
}

// MW-2009-01-28: [[ Inherited parentScripts ]]
// This method will return 'false' if there is not enough memory to complete
// the resolution.
bool MCGroup::resolveparentscript(void)
{
	// Resolve the parentScript of the group itself
	if (!MCObject::resolveparentscript())
		return false;

	// Now resolve the parentScript of all its children
	if (controls != NULL)
	{
		MCControl *t_control;
		t_control = controls;
		do
		{
			if (!t_control -> resolveparentscript())
				return false;

			t_control = t_control -> next();
		}
		while(t_control != controls);
	}

	return true;
}

MCObject *MCGroup::hittest(int32_t x, int32_t y)
{
	if (!(flags & F_VISIBLE || MCshowinvisibles)
		|| flags & F_DISABLED && getstack()->gettool(this) == T_BROWSE)
		return nil;
	
	// If not inside the groups bounds, do nothing.
	MCRectangle srect;
	MCU_set_rect(srect, x, y, 1, 1);
	if (!MCControl::maskrect(srect))
		return nil;
	
	// If inside a controls bounds, return that.
	if (controls != nil)
	{
		MCControl *t_control;
		t_control = controls -> prev();
		do
		{
			// MW-2011-09-28: [[ Bug 9620 ]] We should return the result of the hittest
			//   rather than the control we are hittesting, since if it is a group it
			//   might be a descendent that is hit.
			MCObject *t_hit_control;
			t_hit_control = t_control -> hittest(x, y);
			if (t_hit_control != nil)
				return t_hit_control;
			
			t_control = t_control -> prev();
		}
		while(t_control != controls -> prev());
	}
	
	// Otherwise, if we are opaque, return ourselves.
	if (getflag(F_OPAQUE))
		return this;
	
	return nil;
}

// MW-2012-02-14: [[ Fonts ]] Recompute the font inheritence hierarchy.
bool MCGroup::recomputefonts(MCFontRef p_parent_font)
{
	// First update the font referenced by the group object.
	if (!MCObject::recomputefonts(p_parent_font))
		return false;

	// The group's font only has an effect in isolation if the group is
	// showing a label.
	bool t_changed;
	t_changed = (flags & F_SHOW_NAME && (!isunnamed() || label != NULL));

	// Now loop through all controls owned by the group and update
	// those.

	if (controls != NULL)
	{
		MCControl *t_control;
		t_control = controls;
		do
		{
			if (t_control -> recomputefonts(m_font))
				t_changed = true;
			t_control = t_control -> next();
		}
		while(t_control != controls);
	}

	// Return whether anything changed.
	return t_changed;
}

void MCGroup::relayercontrol(MCControl *p_source, MCControl *p_target)
{
	if (p_source == p_target)
		return;

	if (p_source -> next() != controls && p_source -> next() == p_target ||
		p_target == nil && p_source -> next() == controls)
		return;

	p_source -> remove(controls);
	if (p_target == nil)
		p_source -> appendto(controls);
	else if (p_target == controls)
		p_source -> insertto(controls);
	else
		p_source -> append(p_target);

	if (!computeminrect(False))
		p_source -> layer_redrawall();
}

void MCGroup::relayercontrol_remove(MCControl *p_control)
{
	p_control -> remove(controls);
	if (!computeminrect(False))
		layer_redrawrect(p_control -> geteffectiverect());
		
}

void MCGroup::relayercontrol_insert(MCControl *p_control, MCControl *p_target)
{
	p_control -> setparent(this);

	if (p_target == nil)
		p_control -> appendto(controls);
	else if (p_target == controls)
		p_control -> insertto(controls);
	else
		p_control -> append(p_target);

	if (!computeminrect(False))
		p_control -> layer_redrawall();
}

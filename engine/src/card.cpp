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

#include "execpt.h"
#include "util.h"
#include "stack.h"
#include "tooltip.h"
#include "card.h"
#include "cdata.h"
#include "objptr.h"
#include "sellst.h"
#include "stacklst.h"
#include "undolst.h"
#include "group.h"
#include "button.h"
#include "graphic.h"
#include "scrolbar.h"
#include "player.h"
#include "image.h"
#include "field.h"
#include "vclip.h"
#include "hndlrlst.h"
#include "handler.h"
#include "mcerror.h"
#include "debug.h"
#include "dispatch.h"
#include "field.h"
#include "scrolbar.h"
#include "graphic.h"
#include "player.h"
#include "eps.h"
#include "magnify.h"
#include "aclip.h"
#include "cpalette.h"
#include "vclip.h"
#include "redraw.h"

#include "globals.h"
#include "mctheme.h"
#include "context.h"

MCRectangle MCCard::selrect;
int2 MCCard::startx;
int2 MCCard::starty;
MCObjptr *MCCard::removedcontrol;

#ifdef _MAC_DESKTOP
extern bool MCosxmenupoppedup;
#endif

MCCard::MCCard()
{
	objptrs = NULL;
	flags &= ~F_SHOW_BORDER;
	savedata = NULL;
	mfocused = kfocused = oldkfocused = NULL;
	defbutton = odefbutton = NULL;

	// MW-2011-09-23: [[ TileCache ]] Make sure the card layer ids are zero.
	m_bg_layer_id = 0;
	m_fg_layer_id = 0;

	// MM-2012-11-05: [[ Object selection started/ended message ]]
	m_selecting_objects = false;
}

MCCard::MCCard(const MCCard &cref) : MCObject(cref)
{
	objptrs = NULL;
	if (cref.objptrs != NULL)
	{
		MCObjptr *tptr = cref.objptrs;
		do
		{
			MCObjptr *newoptr = new MCObjptr;
			newoptr->setparent(this);
			newoptr->setid(tptr->getid());
			newoptr->appendto(objptrs);
			tptr = (MCObjptr *)tptr->next();
		}
		while (tptr != cref.objptrs);
	}
	savedata = NULL;
	mfocused = kfocused = oldkfocused = NULL;
	defbutton = odefbutton = NULL;

	// MW-2011-09-23: [[ TileCache ]] Make sure the card layer ids are zero.
	m_bg_layer_id = 0;
	m_fg_layer_id = 0;
	
	// MM-2012-11-05: [[ Object selection started/ended message ]]
	m_selecting_objects = false;
}

MCCard::~MCCard()
{
	while (opened)
		close();
	while (objptrs != NULL)
	{
		if (state & CS_OWN_CONTROLS)
		{
			MCControl *cptr = objptrs->getref();
			// MW-2011-08-09: [[ Groups ]] We mustn't delete a shared group even
			//   if the card owns the controls.
			if (cptr->gettype() != CT_GROUP || !static_cast<MCGroup *>(cptr)->isshared())
				delete cptr;
		}
		MCObjptr *optr = objptrs->remove(objptrs);
		delete optr;
	}
	while (savedata != NULL)
	{
		MCDLlist *optr = savedata->remove(savedata);
		delete optr;
	}
}

Chunk_term MCCard::gettype() const
{
	return CT_CARD;
}

const char *MCCard::gettypestring()
{
	return MCcardstring;
}

bool MCCard::visit(MCVisitStyle p_style, uint32_t p_part, MCObjectVisitor *p_visitor)
{
	bool t_continue;
	t_continue = true;

	// If the card doesn't match the part number, we do nothing.
	if (p_part != 0 && getid() != p_part)
		return true;

	if (p_style == VISIT_STYLE_DEPTH_LAST)
		t_continue = p_visitor -> OnCard(this);

	if (t_continue && objptrs != NULL)
	{
		MCObjptr *t_objptr;
		t_objptr = objptrs;
		do
		{
			t_continue = t_objptr -> visit(p_style, p_part, p_visitor);
			t_objptr = t_objptr -> next();
		}
		while(t_continue && t_objptr != objptrs);
	}

	if (t_continue && p_style == VISIT_STYLE_DEPTH_FIRST)
		t_continue = p_visitor -> OnCard(this);

	return t_continue;
}

void MCCard::open()
{
	clean();
	MCObject::open();
	if (objptrs != NULL)
	{
		MCObjptr *tptr = objptrs;
		do
		{
			MCControl *control = tptr->getref();
			control->setparent(this);
			control->open();
			
			// MW-2013-03-21: [[ Bug ]] If we are opening a shared group for
			//   the second time it means we are switching between cards sharing
			//   it. Therefore, we make sure we recompute fonts and recompute it
			//   (This ensures correct inherited fonts and such are used).
			if (control -> gettype() == CT_GROUP &&
				static_cast<MCGroup *>(control) -> isshared() &&
				control -> getopened() > 1)
				if (control -> recomputefonts(m_font))
					control -> recompute();
			
			tptr = tptr->next();
		}
		while (tptr != objptrs);
	}
	odefbutton = defbutton;
	mgrabbed = False;
	mfocused = kfocused = oldkfocused = NULL;
}

void MCCard::close()
{
	if (!opened)
		return;
	if (getstack()->getmode() <= WM_SHEET)
		kunfocus();
	MCObject::close();
	if (objptrs != NULL)
	{
		MCObjptr *tptr = objptrs;
		do
		{
			tptr->getref()->close();
			tptr = tptr->next();
		}
		while (tptr != objptrs);
		clear();
	}
	defbutton = odefbutton = NULL;
	MCtooltip->clearmatch(this);
}

void MCCard::kfocus()
{
	if (oldkfocused != NULL && kfocused == NULL)
	{
		kfocused = oldkfocused;
		kfocused->getref()->kfocus();
	}
	if (kfocused == NULL)
		kfocusnext(True);
}

Boolean MCCard::kfocusnext(Boolean top)
{
	if (!opened)
		return False;
	if (objptrs == NULL)
	{
		kfocused = oldkfocused = NULL;
		return False;
	}
	MCObjptr *startptr;
	if (kfocused == NULL || top)
		startptr = objptrs;
	else
		startptr = kfocused;
	Boolean done = False;
	defbutton = NULL;
	MCObjptr *tptr = startptr;
	do
	{
		if (tptr->getref()->kfocusnext(top))
		{
			if (kfocused != tptr)
			{
				oldkfocused = kfocused;
				kfocused = NULL;
				if (oldkfocused != NULL)
				{
					if (oldkfocused->getref() == MCactivefield
					        && !MCactivefield->getflag(F_LIST_BEHAVIOR))
						MCactivefield->unselect(False, True);
					oldkfocused->getref()->kunfocus();
					if (oldkfocused == NULL)
						return False;
				}
				if (kfocused == NULL)
					kfocused = tptr;
			}
			kfocused->getref()->kfocus();
			done = True;
			break;
		}
		tptr = tptr->next();
	}
	while (tptr != startptr);
	if (!done && kfocused != NULL)
		done = kfocused->getref()->kfocusnext(True);
	if (done && odefbutton != NULL && defbutton != odefbutton)
		odefbutton->setdefault(defbutton == NULL);
	if (!done)
		message(MCM_focus_in);
	return True;
}

Boolean MCCard::kfocusprev(Boolean bottom)
{
	if (!opened)
		return False;
	if (objptrs == NULL)
	{
		kfocused = oldkfocused = NULL;
		return False;
	}
	MCObjptr *startptr;
	if (kfocused == NULL || bottom)
		startptr = objptrs->prev();
	else
		startptr = kfocused;
	Boolean done = False;
	defbutton = NULL;
	MCObjptr *tptr = startptr;
	do
	{
		if (tptr->getref()->kfocusprev(bottom))
		{
			if (kfocused != tptr)
			{
				oldkfocused = kfocused;
				kfocused = NULL;
				if (oldkfocused != NULL)
				{
					if (oldkfocused->getref() == MCactivefield
					        && !MCactivefield->getflag(F_LIST_BEHAVIOR))
						MCactivefield->unselect(False, True);
					oldkfocused->getref()->kunfocus();
					if (oldkfocused == NULL)
						return False;
				}
				if (kfocused == NULL)
					kfocused = tptr;
			}
			kfocused->getref()->kfocus();
			done = True;
			break;
		}
		tptr = tptr->prev();
	}
	while (tptr != startptr);
	if (!done && kfocused != NULL)
		done = kfocused->getref()->kfocusprev(True);
	if (done && odefbutton != NULL && defbutton != odefbutton)
		odefbutton->setdefault(defbutton == NULL);
	return True;
}

void MCCard::kunfocus()
{
	if (kfocused != NULL)
	{
		oldkfocused = kfocused;
		kfocused = NULL;
		oldkfocused->getref()->kunfocus();
	}
	else
	{
		message(MCM_focus_out);
		oldkfocused = NULL;
	}
}

Boolean MCCard::kdown(const char *string, KeySym key)
{
	MCtooltip->closetip();
	if (kfocused != NULL && getstack()->gettool(this) == T_BROWSE)
		return kfocused->getref()->kdown(string, key);
	if (MCObject::kdown(string, key))
		return True;
	return False;
}

Boolean MCCard::kup(const char *string, KeySym key)
{
	if (kfocused != NULL && getstack()->gettool(this) == T_BROWSE)
		return kfocused->getref()->kup(string, key);
	return MCObject::kup(string, key);
}

void MCCard::mdrag(void)
{
	if (!mgrabbed)
		return;

	if (getstack() -> gettool(this) != T_BROWSE)
		return;

	if (mfocused != NULL)
	{
		mfocused -> getref() -> mdrag();
		//MCdragtargetptr = mfocused -> getref();
	}
	else
	{
		message(MCM_drag_start);
		MCdragtargetptr = this;
	}
}

Boolean MCCard::mfocus(int2 x, int2 y)
{
	if (state & CS_MENU_ATTACHED)
		return MCObject::mfocus(x, y);
	if (!mgrabbed)
		MCtooltip->mousemove(x, y, this);
	if (mgrabbed || MCU_point_in_rect(rect, x, y))
		state |= CS_INSIDE;
	else
		state &= ~CS_INSIDE;

	if (objptrs == NULL)
	{
		// MW-2007-10-29: [[ Bug 3726 ]] dragMove is not sent to a card during
		//   a drag-drop operation.
		if (MCdispatcher -> isdragtarget())
			message_with_args(MCM_drag_move, x, y);
		else
			message_with_args(MCM_mouse_move, x, y);
		mfocused = NULL; 
		return False;
	}
	if (mfocused == NULL || !(mgrabbed && mfocused->getref()->mfocus(x, y)))
	{
		if (mfocused == NULL && mgrabbed)
		{
			if (state & CS_SIZE)
			{
				MCRedrawLockScreen();
				
				MCRectangle oldrect = selrect;
				selrect = MCU_compute_rect(startx, starty, x, y);
				MCRectangle drect = MCU_union_rect(oldrect, selrect);
				if (objptrs != NULL)
				{
					MCObjptr *optr = objptrs;
					do
					{
						MCControl *cptr = optr->getref();
						updateselection(cptr, oldrect, selrect, drect);
						optr = optr->next();

					}
					while (optr != objptrs);
				}
				
				MCRedrawUnlockScreen();

				// MW-2011-08-19: [[ Layers ]] Ensure the selection rect is updated.
				layer_selectedrectchanged(oldrect, selrect);
			}
			message_with_args(MCM_mouse_move, x, y);
			return True;
		}
		if (mgrabbed)
			mfocused->getref()->mfocus(x, y);
		mgrabbed = False;
		MCObjptr *tptr = objptrs->prev();
		Boolean freed;
		do
		{
			MCObject *t_tptr_object;
			t_tptr_object = tptr -> getref();
			
			freed = False;
			
			if (t_tptr_object->mfocus(x, y))
			{
				// MW-2010-10-28: If mfocus calls relayer, then the objptrs can get changed.
				//   Reloop to find the correct one.
				MCObjptr *tptr = objptrs -> prev();
				while(tptr -> getref() != t_tptr_object)
					tptr = tptr -> prev();
				
				Boolean newfocused = tptr != mfocused;
				if (newfocused && mfocused != NULL)
				{
					MCControl *oldfocused = mfocused->getref();
					mfocused = tptr;
					oldfocused->munfocus();
				}
				else
					mfocused = tptr;

				if (newfocused && mfocused != NULL && mfocused -> getref() -> gettype() != CT_GROUP)
				{
					mfocused->getref()->enter();
					
					// MW-2007-10-31: mouseMove sent before mouseEnter - make sure we send an mouseMove
					//   It is possible for mfocused to become NULL if its deleted in mouseEnter so
					//   we check first.
					if (mfocused != NULL)
						mfocused->getref()->mfocus(x, y);
				}

				return True;
			}
			if (tptr == mfocused)
			{
				// MW-2012-02-22: [[ Bug 10018 ]] Previously, if a group was hidden and it had
				//   mouse focus, then it wouldn't unmfocus as there was an explicit check to
				//   stop this (for groups) here.
				// MW-2012-03-13: [[ Bug 10074 ]] Invoke the control's munfocus() method for groups
				//   if the group has an mfocused control.
				if (mfocused -> getref() -> gettype() != CT_GROUP || static_cast<MCGroup *>(mfocused -> getref()) -> getmfocused() != nil)
				{
					MCControl *oldfocused = mfocused->getref();
					mfocused = NULL;
					oldfocused->munfocus();
				}
				else
				{
					static_cast<MCGroup *>(mfocused -> getref()) -> clearmfocus();
					mfocused = nil;
				}
				freed = True;
				tptr = objptrs->prev();
			}
			else
				tptr = tptr->prev();
		}
		while (freed || tptr != objptrs->prev());
		MCtooltip->settip(NULL);
		// MW-2007-07-09: [[ Bug 3726 ]] dragMove is not sent to a card during
		//   a drag-drop operation.
		if (MCdispatcher -> isdragtarget())
			message_with_args(MCM_drag_move, x, y);
		else
			message_with_args(MCM_mouse_move, x, y);
		return False;
	}
	else
		return True;
}

void MCCard::mfocustake(MCControl *target)
{
	MCObjptr *newmfocused = objptrs;
	do
	{
		if (newmfocused->getref() == target)
		{
			if (newmfocused != mfocused)
			{
				munfocus();
				mfocused = newmfocused;
			}
			mfocused->getref()->mfocus(MCmousex, MCmousey);
			return;
		}
		newmfocused = newmfocused->next();
	}
	while (newmfocused != objptrs);
	munfocus();
}

void MCCard::munfocus()
{
	MCtooltip->settip(NULL);
	state &= ~CS_INSIDE;
	if (mfocused != NULL && !mgrabbed)
	{
		MCControl *oldfocused = mfocused->getref();
		mfocused = NULL;
		oldfocused->munfocus();
	}
	else
	{
		if (MCdispatcher -> isdragtarget())
			message(MCM_drag_leave);
		else
			message(MCM_mouse_leave);
	}
}

Boolean MCCard::mdown(uint2 which)
{
	if (state & CS_MENU_ATTACHED)
		return MCObject::mdown(which);
	MCtooltip->closetip();
	MCControl *cptr = NULL;

	MCclickfield = NULL;
	mgrabbed = True;
	if (mfocused != NULL)
	{
		MCControl *oldfocused = mfocused->getref();

		// MW-2007-12-11: [[ Bug 5670 ]] Reset our notification var so we can check it after
#ifdef _MACOSX
		MCosxmenupoppedup = false;
#endif
		if (!mfocused->getref()->mdown(which)
		        && getstack()->gettool(this) == T_BROWSE)
		{
#ifdef _MACOSX
			if (!MCosxmenupoppedup)
#endif
				message_with_args(MCM_mouse_down, "1");
		}
		if (!(MCbuttonstate & (0x1L << (which - 1))))
		{
			Boolean oldstate = MClockmessages;
			MClockmessages = True;
			if (!mup(which))
				oldfocused->mup(which);
			MClockmessages = oldstate;
		}
		return True;
	}
	else
	{
		switch (which)
		{
		case Button1:
			switch (getstack()->gettool(this))
			{
			case T_BROWSE:
				// The following we do to ensure that a click on the card unfocuses any browsers
				// on OS X - what should really happen here is that the *card* should be focused
				// but things aren't currently engineered this way
				MCstacks -> ensureinputfocus(getstack() -> getwindow());
				kunfocus();
#ifdef _MOBILE
				// Make sure the IME has gone away on mobile if due to an explicit card
				// click.
				MCscreen -> closeIME();
#endif
				message_with_args(MCM_mouse_down, "1");
				if (!(MCbuttonstate & (0x1L << (which - 1))))
				{
					Boolean oldstate = MClockmessages;
					MClockmessages = True;
					mup(which);
					MClockmessages = oldstate;
				}
				break;
			case T_POINTER:
				// MW-2010-10-15: [[ Bug 9055 ]] Make sure the card script gets a chance to cancel selection handling
				if (message_with_args(MCM_mouse_down, "1") == ES_NORMAL && !MCexitall)
					return True;

				// MW-2010-11-24: [[ Bug 9194 ]] Make sure we reset exitall, otherwise the selectedObjectChanged message
				//   won't be sent.
				MCexitall = False;

				if (!(MCmodifierstate & MS_SHIFT))
					MCselected->clear(True);
				state |= CS_SIZE;
				startx = MCmousex;
				starty = MCmousey;
				MCU_set_rect(selrect, startx, starty, 1, 1);
				break;
			case T_HELP:
				break;
			case T_BUTTON:
				cptr = new MCButton(*MCtemplatebutton);
				break;
			case T_SCROLLBAR:
				cptr = new MCScrollbar(*MCtemplatescrollbar);
				break;
			case T_PLAYER:
				cptr = new MCPlayer(*MCtemplateplayer);
				break;
			case T_GRAPHIC:
				cptr = new MCGraphic(*MCtemplategraphic);
				break;
			case T_FIELD:
				cptr = new MCField(*MCtemplatefield);
				break;
			case T_IMAGE:
				cptr = new MCImage(*MCtemplateimage);
				break;
			case T_DROPPER:
				MCscreen->dropper(getw(), MCmousex, MCmousey, NULL);
				message(MCM_color_changed);
				break;
			case T_BRUSH:
			case T_BUCKET:
			case T_CURVE:
			case T_LINE:
			case T_OVAL:
			case T_PENCIL:
			case T_POLYGON:
			case T_RECTANGLE:
			case T_REGULAR_POLYGON:
			case T_ROUND_RECT:
			case T_SPRAY:
			case T_TEXT:
				{
					if (objptrs != NULL)
					{
						MCObjptr *optr = objptrs;
						do
						{
							if (optr->getref()->gettype() == CT_IMAGE)
								return False;
							optr = optr->next();
						}
						while (optr != objptrs);
					}
					MCImage *iptr = createimage();
					iptr->mfocus(MCmousex, MCmousey);
					iptr->mdown(which);
				}
				break;
			default:
				return False;
			}
			break;
		case Button2:
		case Button3:
			message_with_args(MCM_mouse_down, which);
			return False;
		}
	}
	if (cptr != NULL)
	{
		getstack()->appendcontrol(cptr);
		mfocused = newcontrol(cptr, False);
		cptr->create(MCmousex, MCmousey);
	}
	return True;
}

Boolean MCCard::mup(uint2 which)
{
	if (state & CS_MENU_ATTACHED)
		return MCObject::mup(which);
	if (mfocused != NULL)
	{
		mgrabbed = False;
		return mfocused->getref()->mup(which);
	}
	else
	{
		if (state & (CS_INSIDE | CS_SIZE))
		{
			switch (which)
			{
			case Button1:
				if (state & CS_SIZE)
				{
					state &= ~CS_SIZE;
					// MW-2011-08-18: // MW-2011-08-19: [[ Layers ]] Ensure the selection rect is updated.
					layer_dirtyrect(selrect);
					
					// MM-2012-11-05: [[ Object selection started/ended message ]]
					if (m_selecting_objects)
					{
						m_selecting_objects = false;
						message(MCM_object_selection_ended);
					}
				}
				switch (getstack()->gettool(this))
				{
				case T_POINTER:
				case T_BROWSE:
					// MW-2010-10-15: [[ Bug 9055 ]] Mouse message consistency improvement
					message_with_args(MCM_mouse_up, "1");
					break;
				case T_HELP:
					help();
					break;
				default:
					break;
				}
				break;
			case Button2:
			case Button3:
				message_with_args(MCM_mouse_up, which);
				break;
			}
			mgrabbed = False;
			return True;
		}
		mgrabbed = False;
	}
	return False;
}

Boolean MCCard::doubledown(uint2 which)
{
	if (mfocused != NULL)
	{
		mgrabbed = True;
		MCControl *oldfocused = mfocused->getref();
		if (!mfocused->getref()->doubledown(which)
		        && getstack()->gettool(this) == T_BROWSE)
			message_with_args(MCM_mouse_double_down, "1");
		if (!(MCbuttonstate & (0x1L << (which - 1))))
		{
			Boolean oldstate = MClockmessages;
			MClockmessages = True;
			if (!doubleup(which))
				oldfocused->doubleup(which);
			MClockmessages = oldstate;
		}
	}
	else
	{
		// MW-2010-10-15: [[ Bug 9055 ]] Don't send messages to the card, if they were sent to the control
		switch (which)
		{
		case Button1:
			switch (getstack()->gettool(this))
			{
			case T_BROWSE:
			case T_POINTER:
				message_with_args(MCM_mouse_double_down, "1");
				break;
			default:
				break;
			}
			break;
		case Button2:
		case Button3:
			message_with_args(MCM_mouse_double_down, which);
			break;
		}
	}
	return True;
}

Boolean MCCard::doubleup(uint2 which)
{
	if (mfocused != NULL)
	{
		mgrabbed = False;
		return mfocused->getref()->doubleup(which);
	}
	else
	{
		// MW-2010-10-15: [[ Bug 9055 ]] Don't send messages to the card, if they were sent to the control
		switch (which)
		{
		case Button1:
			switch (getstack()->gettool(this))
			{
			case T_BROWSE:
				message_with_args(MCM_mouse_double_up, "1");
				break;
			case T_POINTER:
				getstack()->kfocusset(NULL);
				if (MCmodifierstate & MS_MOD1)
					editscript();
				else
				{
					MCselected->replace(this);
					message_with_args(MCM_mouse_double_up, "1");
				}
				break;
			default:
				break;
			}
			break;
		case Button2:
		case Button3:
			message_with_args(MCM_mouse_double_up, which);
			break;
		}
	}
	return True;
}

void MCCard::timer(MCNameRef mptr, MCParameter *params)
{
	if (MCNameIsEqualTo(mptr, MCM_idle, kMCCompareCaseless))
	{
		if (opened)
		{
			Boolean again = False;
			Tool tool =  getstack()->gettool(this);
			if (mfocused == NULL && MCbuttonstate)
			{
				if (tool == T_BROWSE)
					if (message(MCM_mouse_still_down) == ES_ERROR)
						senderror();
					else
						again = True;
			}
			if (hashandlers & HH_IDLE)
			{
				if (tool == T_BROWSE)
					if (message(MCM_idle) == ES_ERROR)
						senderror();
					else
						again = True;
			}
			if (again)
				MCscreen->addtimer(this, MCM_idle, MCidleRate);
		}
	}
	else
		MCObject::timer(mptr, params);
}

Exec_stat MCCard::getprop(uint4 parid, Properties which, MCExecPoint& ep, Boolean effective)
{
	MCRectangle minrect;
	uint2 num;

	switch (which)
	{
#ifdef /* MCCard::getprop */ LEGACY_EXEC
	case P_NUMBER:
	case P_LAYER:
		getstack()->count(CT_CARD, CT_UNDEFINED, this, num);
		ep.setint(num);
		break;
	case P_CANT_DELETE:
		ep.setboolean(getflag(F_C_CANT_DELETE));
		break;
	case P_DONT_SEARCH:
		ep.setboolean(getflag(F_C_DONT_SEARCH));
		break;
	case P_MARKED:
		ep.setboolean(getflag(F_MARKED));
		break;
	case P_SHOW_PICT:
		ep.setboolean(True);
		break;
	case P_FORMATTED_LEFT:
		minrect = computecrect();
		ep.setint(minrect.x);
		break;
	case P_FORMATTED_TOP:
		minrect = computecrect();
		ep.setint(minrect.y);
		break;
	case P_FORMATTED_HEIGHT:
		minrect = computecrect();
		ep.setint(minrect.height);
		break;
	case P_FORMATTED_WIDTH:
		minrect = computecrect();
		ep.setint(minrect.width);
		break;
	case P_FORMATTED_RECT:
		minrect = computecrect();
		ep.setrectangle(minrect);
		break;
	case P_BACKGROUND_NAMES:
	case P_BACKGROUND_IDS:
	case P_SHARED_GROUP_NAMES:
	case P_SHARED_GROUP_IDS:
	case P_GROUP_NAMES:
	case P_GROUP_IDS:
    case P_CONTROL_NAMES:
    case P_CONTROL_IDS:
    case P_CHILD_CONTROL_NAMES:
    case P_CHILD_CONTROL_IDS:
		// MERG-2015-05-01: [[ ChildControlProps ]] Add ability to list both
		//   immediate and all descendent controls of a card.
			
        ep.clear();
		clean();
		if (objptrs != NULL)
		{
			bool t_want_background;
			t_want_background = which == P_BACKGROUND_NAMES || which == P_BACKGROUND_IDS;
			
			bool t_want_shared;
			t_want_shared = which == P_SHARED_GROUP_NAMES || which == P_SHARED_GROUP_IDS;

			MCExecPoint t_other_ep(ep);
            MCObjptr *optr = objptrs;
			uint2 i = 0;
            
            bool t_controls;
			t_controls = which == P_CHILD_CONTROL_NAMES ||  which == P_CHILD_CONTROL_IDS || which == P_CONTROL_NAMES || which == P_CONTROL_IDS;
			do
			{
				MCObject *t_object;
				t_object = optr -> getref();

				optr = optr -> next();

                if (t_object->gettype() == CT_GROUP)
                {
                    if (t_want_background && !static_cast<MCGroup *>(t_object)  -> isbackground())
                        continue;
                    
                    if (t_want_shared && !static_cast<MCGroup *>(t_object) -> isshared())
                        continue;
                }
                else if (!t_controls)
					continue;
                
				Properties t_prop;
				if (which == P_BACKGROUND_NAMES || which == P_SHARED_GROUP_NAMES || which == P_GROUP_NAMES || which == P_CONTROL_NAMES || which == P_CHILD_CONTROL_NAMES)
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
			}
			while (optr != objptrs);
			
			if (!opened)
				clear();
		}
		break;
	case P_DEFAULT_BUTTON:
		if (defbutton == NULL && odefbutton == NULL)
			ep.clear();
		else
			if (defbutton != NULL)
				defbutton->getprop(0, P_LONG_ID, ep, False);
			else
				odefbutton->getprop(0, P_LONG_ID, ep, False);
		break;
	case P_VISIBLE:
	case P_INVISIBLE:
	case P_ENABLED:
	case P_DISABLED:
	case P_TRAVERSAL_ON:
	case P_SHADOW:
	case P_LOCK_LOCATION:
	case P_TOOL_TIP:
	// MW-2012-03-13: [[ UnicodeToolTip ]] Card's don't have tooltips.
	case P_UNICODE_TOOL_TIP:
		MCeerror->add(EE_OBJECT_SETNOPROP, 0, 0);
		return ES_ERROR;
#endif /* MCCard::getprop */
	default:
		return MCObject::getprop(parid, which, ep, effective);
	}
	return ES_NORMAL;
}

Exec_stat MCCard::setprop(uint4 parid, Properties which, MCExecPoint &ep, Boolean effective)
{
	Boolean dirty = False;
	uint4 newnumber;
	MCString data = ep.getsvalue();

	switch (which)
	{
#ifdef /* MCCard::setprop */ LEGACY_EXEC
	// MW-2011-09-20: [[ Bug 9741 ]] Make sure we update the card completely if
	//   any of these are set.
	case P_FORE_PIXEL:
	case P_BACK_PIXEL:
	case P_HILITE_PIXEL:
	case P_BORDER_PIXEL:
	case P_TOP_PIXEL:
	case P_BOTTOM_PIXEL:
	case P_SHADOW_PIXEL:
	case P_FOCUS_PIXEL:
	case P_FORE_COLOR:
	case P_BACK_COLOR:
	case P_HILITE_COLOR:
	case P_BORDER_COLOR:
	case P_TOP_COLOR:
	case P_BOTTOM_COLOR:
	case P_SHADOW_COLOR:
	case P_FOCUS_COLOR:
	case P_COLORS:
	case P_FORE_PATTERN:
	case P_BACK_PATTERN:
	case P_HILITE_PATTERN:
	case P_BORDER_PATTERN:
	case P_TOP_PATTERN:
	case P_BOTTOM_PATTERN:
	case P_SHADOW_PATTERN:
	case P_FOCUS_PATTERN:
	case P_TEXT_FONT:
	case P_TEXT_SIZE:
	case P_TEXT_STYLE:
	case P_TEXT_HEIGHT:
		// Try to set the property.
		if (MCObject::setprop(parid, which, ep, effective) != ES_NORMAL)
			return ES_ERROR;
		
		// Now dirty everything in the stack (inc. flush tilecache since
		// these props are inherited!).
		getstack() -> dirtyall();
		break;
			
	case P_LOCATION:
	case P_LEFT:
	case P_TOP:
	case P_RIGHT:
	case P_BOTTOM:
	case P_TOP_LEFT:
	case P_TOP_RIGHT:
	case P_BOTTOM_LEFT:
	case P_BOTTOM_RIGHT:
	case P_WIDTH:
	case P_HEIGHT:
	case P_RECTANGLE:
	case P_VISIBLE:
	case P_INVISIBLE:
	case P_ENABLED:
	case P_DISABLED:
	case P_TRAVERSAL_ON:
	case P_SHADOW:
	case P_LOCK_LOCATION:
	case P_TOOL_TIP:
	// MW-2012-03-13: [[ UnicodeToolTip ]] Cards don't have tooltips.
	case P_UNICODE_TOOL_TIP:
		MCeerror->add(EE_OBJECT_SETNOPROP, 0, 0);
		return ES_ERROR;
	case P_LAYER:
	case P_NUMBER:
		if (data == MCtopstring)
			newnumber = MAXUINT4;
		else
			if (data == MCbottomstring)
				newnumber = 1;
			else
				if (!MCU_stoui4(data, newnumber))
				{
					MCeerror->add
					(EE_OBJECT_NAN, 0, 0, data);
					return ES_ERROR;
				}
		if (parent != NULL)
			getstack()->renumber(this, newnumber);
		break;
	case P_MARKED:
		if (!MCU_matchflags(data, flags, F_MARKED, dirty))
		{
			MCeerror->add
			(EE_OBJECT_NAB, 0, 0, data);
			return ES_ERROR;
		}
		break;
	case P_SHOW_PICT:
		break;
	case P_CANT_DELETE:
		if (!MCU_matchflags(data, flags, F_C_CANT_DELETE, dirty))
		{
			MCeerror->add
			(EE_OBJECT_NAB, 0, 0, data);
			return ES_ERROR;
		}
		break;
	case P_DONT_SEARCH:
		if (!MCU_matchflags(data, flags, F_C_DONT_SEARCH, dirty))
		{
			MCeerror->add
			(EE_OBJECT_NAB, 0, 0, data);
			return ES_ERROR;
		}
		break;
#endif /* MCCard::setprop */
	default:
		return MCObject::setprop(parid, which, ep, effective);
	}
	return ES_NORMAL;
}

Boolean MCCard::del()
{
	if (parent == NULL || scriptdepth != 0 || getstack()->islocked()
	        || getstack()->isediting() || flags & F_C_CANT_DELETE)
	{
		MCeerror->add(EE_OBJECT_CANTREMOVE, 0, 0);
		return False;
	}
	clean();
	if (objptrs != NULL)
	{
		MCObjptr *optr = objptrs;
		do
		{
			if (optr->getref()->getscriptdepth() != 0)
			{
				MCeerror->add
				(EE_OBJECT_CANTREMOVE, 0, 0);
				return False;
			}
			optr = optr->next();
		}
		while (optr != objptrs);
	}
	MCselected->remove(this);
	
	// MW-2008-10-31: [[ ParentScripts ]] Make sure we close the controls
	closecontrols();
	message(MCM_close_card);
	message(MCM_delete_card);
	
	MCundos->freestate();
	state |= CS_NO_MESSAGES | CS_OWN_CONTROLS;
	getstack()->removecard(this);
	state &= ~CS_NO_MESSAGES;
	if (objptrs != NULL)
	{
		MCObjptr *optr = objptrs;
		do
		{
			// MW-2011-08-09: [[ Groups ]] Shared groups just get reparented, rather
			//   than removed from the stack since they cannot be 'owned' by the card.
			if (optr->getref()->gettype() == CT_GROUP && static_cast<MCGroup *>(optr->getref())->isshared())
			{
				// OK-2010-02-18: [[Bug 8268]] - If the group's parent is the card being deleted, reset 
				// its parent to the stack to prevent dangling reference and crash.
				if (!optr->getref()->getopened() || optr -> getref() -> getparent() == this)
					optr->getref()->setparent(getstack());
			}
			else
			{
				getstack()->removecontrol(optr->getref());
			}
			MCCdata *dptr = optr->getref()->getdata(obj_id, False);
			if (dptr != NULL)
				dptr->appendto(savedata);
			optr = optr->next();
		}
		while (optr != objptrs);
	}
	return True;
}

struct UpdateDataIdsVisitor: public MCObjectVisitor
{
	unsigned int card_id;

	UpdateDataIdsVisitor(unsigned int p_card_id)
		: card_id(p_card_id)
	{
	}

	bool OnField(MCField *p_field)
	{
		if (!p_field -> getflag(F_SHARED_TEXT))
		{
			MCCdata *t_data;
			t_data = p_field -> getcarddata(0);
			if (t_data != NULL)
				t_data -> setid(card_id);
		}

		return true;
	}

	bool OnButton(MCButton *p_button)
	{
		if (!p_button -> getflag(F_SHARED_HILITE))
		{
			MCCdata *t_data;
			t_data = p_button -> getdata(0, False);
			if (t_data != NULL)
				t_data -> setid(card_id);
		}

		return true;
	}
};

void MCCard::paste(void)
{
	if (MCdefaultstackptr -> islocked())
		return;

	obj_id = getstack() -> newid();

	// Now we have a new id, make sure we update the ids of all attached non-shared
	// data.
	UpdateDataIdsVisitor t_visitor(obj_id);
	visit(VISIT_STYLE_DEPTH_LAST, 0, &t_visitor);	

	setparent(MCdefaultstackptr);
	if (objptrs != NULL)
	{
		MCObjptr *t_objptr;
		t_objptr = objptrs;
		do
		{
			MCControl *t_control;
			t_control = t_objptr -> getref();
			t_control -> setid(MCdefaultstackptr -> newid());
			MCdefaultstackptr -> appendcontrol(t_control);
			t_objptr -> setref(t_control);
			t_objptr = t_objptr -> next();
		}
		while(t_objptr != objptrs);
	}
	MCdefaultstackptr -> appendcard(this);
}

Exec_stat MCCard::handle(Handler_type htype, MCNameRef mess, MCParameter *params, MCObject *pass_from)
{
	if (!opened)
		clean();

	Exec_stat stat;
	stat = handleself(htype, mess, params);

	// MW-2011-02-27: [[ Bug 9412 ]] Do not pass the message onto the background the message
    //   passed through before it reached the card.
	if (pass_from != nil)
	{
		if (objptrs != NULL && (stat == ES_NOT_HANDLED || stat == ES_PASS))
		{
			bool t_has_passed;
			t_has_passed = false;
			
			MCObjptr *tptr = objptrs->prev();
			do
			{
				// MW-2011-08-08: [[ Groups ]] Use 'getrefasgroup()' to test for groupness.
				MCGroup *optr = tptr->getrefasgroup();
				// MW-2011-08-08: [[ Groups ]] Use 'isbackground()' rather than !F_GROUP_ONLY.
				if (optr != pass_from && optr != nil && optr -> isbackground())
				{
					stat = optr->handle(htype, mess, params, nil);
					if (stat == ES_PASS)
						t_has_passed = true;
				}
				tptr = tptr->prev();
			}
			while (tptr != objptrs->prev() && (stat == ES_PASS || stat == ES_NOT_HANDLED));

			if (!opened)
				clear();
				
			if (t_has_passed)
				stat = ES_PASS;
		}

		if (parent != NULL && (stat == ES_PASS || stat == ES_NOT_HANDLED))
		{
			Exec_stat oldstat = stat;
			stat = parent->handle(htype, mess, params, this);
			if (oldstat == ES_PASS && stat == ES_NOT_HANDLED)
				stat = ES_PASS;
		}
	}

	if (stat == ES_ERROR && MCerrorptr == NULL)
		MCerrorptr = this;

	return stat;
}

void MCCard::recompute()
{
	if (objptrs != NULL)
	{
		MCObjptr *optr = objptrs;
		do
		{
			optr->getref()->recompute();
			optr = optr->next();
		}
		while (optr != objptrs);
	}
}

void MCCard::kfocusset(MCControl *target)
{
	if (objptrs != NULL)
	{
		MCObjptr *tkfocused = kfocused;
		if (tkfocused != NULL && tkfocused->getref() != target)
		{
			kfocused = NULL;
			tkfocused->getref()->kunfocus();
		}
		if (kfocused != NULL)
			return;
		kfocused = objptrs;
		defbutton = NULL;
		do
		{
			if (kfocused->getref()->kfocusset(target))
			{
				kfocused->getref()->kfocus();

				// OK-2009-04-29: [[Bug 8013]] - Its possible that kfocus() can set kfocused to NULL if the 
				// user handles the message and does something to unfocus the object (e.g. select empty)
				// In this case we skip the kfocused->getref() check as it will crash.
				if (odefbutton != NULL && (kfocused == NULL || (kfocused->getref() != odefbutton)))
					odefbutton->setdefault(defbutton == NULL);
				return;
			}
			kfocused = kfocused->next();
		}
		while (kfocused != objptrs);
		kfocused = NULL;
	}
}

MCCard *MCCard::clone(Boolean attach, Boolean controls)
{
	clean();
	MCCard *newcptr = new MCCard(*this);
	newcptr->parent = MCdefaultstackptr;
	Boolean diffstack = getstack() != MCdefaultstackptr;

	if (controls)
	{
		if (!attach)
			newcptr->state |= CS_OWN_CONTROLS;
		if (objptrs != NULL)
		{
			newcptr->clonedata(this);
			MCObjptr *newoptr = newcptr->objptrs;
			MCObjptr *optr = objptrs;
			do
			{
				// MW-2011-08-09: [[ Groups ]] If a group is shared then it needs
				//   special handling.
				if (optr->getref()->gettype() == CT_GROUP && static_cast<MCGroup *>(optr -> getref()) -> isshared())
				{
					if (diffstack)
					{
						MCControl *newcontrol = optr->getref()->clone(False, OP_NONE, false);
						newcontrol->setid(newcptr->getstack()->newid());
						newcontrol->setparent(newcptr);
						newcptr->getstack()->appendcontrol(newcontrol);
						newoptr->setid(newcontrol->getid());
						newoptr->setref(newcontrol);
					}
				}
				else
				{
					MCControl *oldcontrol = optr->getref();
					MCControl *newcontrol = oldcontrol->clone(False, OP_NONE, false);
					if (diffstack)
						newcontrol->setparent(newcptr);
					if (attach)
					{
						if (state & CS_OWN_CONTROLS && oldcontrol->getid() != 0)
						{
							newcontrol->setid(oldcontrol->getid());
							oldcontrol->setid(0);
						}
						else
							newcontrol->setid(newcptr->getstack()->newid());
						newoptr->setid(newcontrol->getid());
						newcptr->getstack()->appendcontrol(newcontrol);
					}
					newoptr->setref(newcontrol);
				}
				newoptr = newoptr->next();
				optr = optr->next();
			}
			while (optr != objptrs);
		}
		if (attach)
		{
			if (state & CS_OWN_CONTROLS && obj_id != 0)
			{
				newcptr->obj_id = obj_id;
				obj_id = 0;
			}
			else
				newcptr->obj_id = newcptr->getstack()->newid();
			newcptr->getstack()->appendcard(newcptr);
			newcptr->state &= ~CS_OWN_CONTROLS;
			if (diffstack)
				newcptr->replacedata(getstack());
			else
				newcptr->replacedata(NULL);
		}
	}
	else if (attach)
	{
		// This is invoked when creating a new card. In this instance, we ensure
		// all backgrounds on 'this card' are auto-placed.
		MCCard *sourceptr = newcptr->getstack()->getchild(CT_THIS, MCnullmcstring, CT_CARD);
		if (sourceptr != NULL && sourceptr->objptrs != NULL)
		{
			MCObjptr *optr = sourceptr->objptrs;
			do
			{
				// MW-2011-08-09: [[ Groups ]] If a group is a background then auto
				//   place it on the new card.
				if (optr->getref()->gettype() == CT_GROUP && static_cast<MCGroup *>(optr -> getref()) -> isbackground())
				{
					MCObjptr *newoptr = new MCObjptr;
					newoptr->setparent(newcptr);
					newoptr->setid(optr->getid());
					newoptr->appendto(newcptr->objptrs);
				}
				optr = optr->next();
			}
			while (optr != sourceptr->objptrs);
		}
		newcptr->obj_id = newcptr->getstack()->newid();
		newcptr->getstack()->appendcard(newcptr);
	}
	if (objptrs != NULL && !opened)
		clear();
	return newcptr;
}

void MCCard::clonedata(MCCard *source)
{
	if (source->savedata != NULL)
	{
		MCCdata *cptr = source->savedata;
		do
		{
			MCCdata *dptr = new MCCdata(*cptr);
			dptr->appendto(savedata);
			cptr = cptr->next();
		}
		while (cptr != source->savedata);
	}
	else
	{
		if (source->objptrs != NULL)
		{
			MCObjptr *optr = source->objptrs;
			do
			{
				MCCdata *dptr = optr->getref()->getdata(source->obj_id, True);
				if (dptr != NULL)
					dptr->appendto(savedata);
				optr = optr->next();
			}
			while (optr != source->objptrs);
		}
	}
}

void MCCard::replacedata(MCStack *source)
{
	if (objptrs != NULL)
	{
		MCObjptr *optr = objptrs;
		do
		{
			optr->getref()->replacedata(savedata, obj_id);
			if (source != NULL)
				optr->getref()->resetfontindex(source);
			optr = optr->next();
		}
		while (optr != objptrs);
	}
}

MCObject *MCCard::getobjbylayer(uint32_t p_layer)
{
	// Layers below 1 and above 65535 don't exist.
	if (p_layer < 1 || p_layer > 65535)
		return nil;

	// If the card has no controls, then there is nothing to do.
	if (objptrs == nil)
		return nil;

	// Loop through to find the layer.
	MCObjptr *t_ptr;
	MCControl *t_found_obj;
	uint2 t_layer;
	t_layer = p_layer - 1;
	t_ptr = objptrs;
	do
	{
		t_found_obj = t_ptr -> getref() -> findnum(CT_LAYER, t_layer);
		if (t_found_obj != nil)
			break;
		t_ptr = t_ptr -> next();
	}
	while(t_ptr != objptrs);

	// Return the found object, if any.
	return t_found_obj;
}

void MCCard::relayercontrol(MCControl *p_source, MCControl *p_target)
{
	// If source == target then there is nothing to do.
	if (p_source == p_target)
		return;

	// Get the objptrs.
	MCObjptr *t_source_ptr, *t_target_ptr;
	t_source_ptr = getobjptrforcontrol(p_source);
	if (p_target != nil)
		t_target_ptr = getobjptrforcontrol(p_target);
	else
		t_target_ptr = nil;

	// Get the previous / next ptrs.
	MCObjptr *t_previous, *t_next;
	t_previous = t_source_ptr -> prev() != objptrs ? t_source_ptr -> prev() : nil;
	t_next = t_source_ptr -> next() != objptrs ? t_source_ptr -> next() : nil;

	// If the source control already precedes the target then we are done.
	if (t_next == t_target_ptr)
		return;

	// Otherwise, remove the layer.
	t_source_ptr -> remove(objptrs);
	layer_removed(p_source, t_previous, t_next);

	// Now, replace the layer.
	if (t_target_ptr != nil)
	{
		if (t_target_ptr != objptrs)
			t_source_ptr -> append(t_target_ptr);
		else
			t_source_ptr -> insertto(objptrs);
	}
	else
		t_source_ptr -> appendto(objptrs);
	layer_added(p_source, t_source_ptr -> prev() != objptrs ? t_source_ptr -> prev() : nil, t_source_ptr -> next() != objptrs ? t_source_ptr -> next() : nil);
}

void MCCard::relayercontrol_remove(MCControl *p_control)
{
	MCObjptr *t_control_ptr;
	t_control_ptr = getobjptrforcontrol(p_control);
	
	MCObjptr *t_previous, *t_next;
	t_previous = t_control_ptr -> prev() != objptrs ? t_control_ptr -> prev() : nil;
	t_next = t_control_ptr -> next() != objptrs ? t_control_ptr -> next() : nil;
	
	// Remove the control from the card's objptr list.
	t_control_ptr -> remove(objptrs);
	delete t_control_ptr;

	// Remove the control from the stack's list.
	getstack() -> removecontrol(p_control);

	// Mark the layer as being removed.
	layer_removed(p_control, t_previous, t_next);
}

void MCCard::relayercontrol_insert(MCControl *p_control, MCControl *p_target)
{	
	// Add the control to the card's objptr list.
	MCObjptr *t_control_ptr;
	t_control_ptr = new MCObjptr;
	t_control_ptr -> setparent(this);
	t_control_ptr -> setref(p_control);
	p_control -> setparent(this);

	// Add the control to the stack's object list.
	getstack() -> appendcontrol(p_control);

	// Compute the objptr for the target
	MCObjptr *t_target_ptr;
	if (p_target != nil)
		t_target_ptr = getobjptrforcontrol(p_target);
	else
		t_target_ptr = nil;

	// Insert the objptr.
	if (t_target_ptr != nil)
	{
		if (t_target_ptr != objptrs)
			t_control_ptr -> append(t_target_ptr);
		else
			t_control_ptr -> insertto(objptrs);
	}
	else
		t_control_ptr -> appendto(objptrs);
	layer_added(p_control, t_control_ptr -> prev() != objptrs ? t_control_ptr -> prev() : nil, t_control_ptr -> next() != objptrs ? t_control_ptr -> next() : nil);
}

Exec_stat MCCard::relayer(MCControl *optr, uint2 newlayer)
{
	if (!opened || !MCrelayergrouped && optr->getparent()->gettype() == CT_GROUP || (optr -> getparent() -> gettype() == CT_CARD && optr -> getparent() != this))
		return ES_ERROR;
	uint2 oldlayer = 0;
	if (!MCrelayergrouped)
		count(CT_LAYER, CT_UNDEFINED, optr, oldlayer, True);
	// MW-2011-08-18: [[ Redraw ]] Update to use redraw.
	MCRedrawLockScreen();
	optr->open(); // removing closes object
	MCObject *oldparent = optr->getparent();
	if (oldparent == this)
	{
		removecontrol((MCControl *)optr, False, False);
		getstack()->removecontrol(optr);
	}
	else
	{
		MCGroup *gptr = (MCGroup *)optr->getparent();
		gptr->removecontrol((MCControl *)optr, False);
	}
	MCRedrawUnlockScreen();

	MCObjptr *tptr = objptrs;
	MCControl *foundobj = NULL;
	if (newlayer > 1 && objptrs != NULL)
	{
		uint2 layer = newlayer - 2;
		do
		{
			if ((foundobj = tptr->getref()->findnum(CT_LAYER, layer)) != NULL)
				break;
			tptr = tptr->next();
		}
		while (tptr != objptrs);
	}

	if (MCrelayergrouped && foundobj != NULL
	        && (foundobj->gettype() == CT_GROUP
	            || foundobj->getparent()->gettype() == CT_GROUP))
	{
		MCGroup *gptr;
		if (foundobj->gettype() == CT_GROUP)
		{
			gptr = (MCGroup *)foundobj;
			MCControl *cptr = gptr->getcontrols();
			optr->insertto(cptr);
			gptr->setcontrols(optr);
		}
		else
		{
			gptr = (MCGroup *)foundobj->getparent();
			foundobj->append(optr);
		}
		optr->setparent(gptr);
		gptr->computeminrect(False);
	}
	else
	{
		MCObjptr *newptr = new MCObjptr;
		newptr->setparent(this);
		newptr->setref(optr);
		optr->setparent(this);
		getstack()->appendcontrol(optr);
		if (foundobj == NULL)
			if (newlayer <= 1)
				newptr->insertto(objptrs);
			else
				newptr->appendto(objptrs);
		else
		{
			Boolean g = False;
			if (!MCrelayergrouped)
			{
				while (foundobj->getparent() != this)
				{
					foundobj = (MCControl *)foundobj->getparent();
					g = True;
				}
			}
			tptr = objptrs;
			do
			{
				if (tptr->getref() == foundobj)
				{
					if (g)
					{
						uint2 tnum;
						count(CT_LAYER, CT_UNDEFINED, tptr->next()->getref(), tnum, True);
						if (tptr->next() == objptrs || newlayer < tnum)
						{
							if (tptr == objptrs)
								newptr->insertto(objptrs);
							else
								tptr->prev()->append(newptr);
							break;
						}
					}
					tptr->append(newptr);
					break;
				}
				tptr = tptr->next();
			}
			while (tptr != objptrs);
		}

		layer_added(optr, newptr -> prev() != objptrs -> prev() ? newptr -> prev() : nil, newptr -> next() != objptrs ? newptr -> next() : nil);
	}

	if (oldparent == this)
	{
		MCObjptr *newoptr = NULL;
		MCObjptr *tptr = objptrs;
		do
		{
			if (tptr->getref() == optr)
			{
				newoptr = tptr;
				break;
			}
			tptr = tptr->next();
		}
		while (tptr != objptrs);
		clearfocus(removedcontrol, newoptr);
	}
	else if (optr->getparent() != oldparent)
	{
		MCGroup *gptr = (MCGroup *)oldparent;
		gptr->clearfocus(optr);
	}

	if (getstack() == MCmousestackptr
	        && MCU_point_in_rect(optr->getrect(), MCmousex, MCmousey))
		mfocus(MCmousex, MCmousey);

	return ES_NORMAL;
}

MCCard *MCCard::findname(Chunk_term type, const MCString &inname)
{
	if (type == CT_CARD && MCU_matchname(inname, CT_CARD, getname()))
		return this;
	else
		return NULL;
}

MCCard *MCCard::findid(Chunk_term type, uint4 inid, Boolean alt)
{
	if (type == CT_CARD && (inid == obj_id || alt && inid == altid))
		return this;
	else
		return NULL;
}

Boolean MCCard::countme(uint4 groupid, Boolean marked)
{
	if (marked && !(flags & F_MARKED))
		return False;
	if (groupid == 0)
		return True;
	if (objptrs != NULL)
	{
		MCObjptr *tptr = objptrs;
		do
		{
			if (tptr->getid() == groupid)
				return True;
			tptr = tptr->next();
		}
		while (tptr != objptrs);
	}
	return False;
}

Boolean MCCard::count(Chunk_term otype, Chunk_term ptype,
                      MCObject *stop, uint2 &num, Boolean dohc)
{
	if (otype != CT_LAYER && otype != CT_MENU && ptype == CT_UNDEFINED
	        && dohc && getstack()->hcaddress())
		ptype = otype == CT_FIELD ? CT_BACKGROUND : CT_CARD;
	num = 0;
	if (!opened)
		clean();
	if (objptrs != NULL)
	{
		MCObjptr *optr = objptrs;
		do
		{
			Chunk_term ttype = optr->getref()->gettype();

			// MW-2011-08-08: [[ Groups ]] Use 'isbackground()' rather than !F_GROUP_ONLY.
			if (ptype == CT_UNDEFINED
			        || otype == CT_GROUP && ttype == CT_GROUP
			        || ptype != CT_BACKGROUND && ttype != CT_GROUP
			        || (ttype == CT_GROUP && ptype == CT_BACKGROUND) == static_cast<MCGroup *>(optr->getref())->isbackground())
				if (optr->getref()->count(otype, stop, num))
				{
					if (!opened)
						clear();
					return True;
				}
			optr = optr->next();
		}
		while (optr != objptrs);
		if (!opened)
			clear();
	}
	return False;
}

MCControl *MCCard::getchild(Chunk_term etype, const MCString &expression,
                            Chunk_term otype, Chunk_term ptype)
{
	if (otype <= CT_CARD)
		return NULL;
	
	// MM-2012-12-02: [[ Bug ]] Make sure we clean the object list before checking if null.
	if (!opened)
		clean();
	
	MCObjptr *optr = objptrs;
	if (optr == NULL)
		return NULL;
	
	if (otype != CT_LAYER && otype != CT_MENU && ptype == CT_UNDEFINED
	        && (getstack()->hcaddress()
	            || (getstack()->hcaddress() && etype == CT_ID)))
		ptype = otype == CT_FIELD ? CT_BACKGROUND : CT_CARD;

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
		count(otype, ptype, NULL, num, True);
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
		{
			MCStack *t_stack;
			t_stack = getstack();
			
			// MW-2012-10-10: [[ IdCache ]] First check the id-cache.
			MCObject *t_object;
			t_object = t_stack -> findobjectbyid(tofindid);
			if (t_object != nil)
			{
				MCObject *t_parent;
				t_parent = t_object -> getparent();
				while(t_parent -> gettype() != CT_CARD && t_parent -> gettype() != CT_STACK)
					t_parent = t_parent -> getparent();
				if (t_parent == this)
				{
					Chunk_term ttype = optr->getref()->gettype();
					if (ptype == CT_UNDEFINED
				        || otype == CT_GROUP && ttype == CT_GROUP
				        || ptype != CT_BACKGROUND && ttype != CT_GROUP
				        || (ttype == CT_GROUP && ptype == CT_BACKGROUND) == static_cast<MCGroup *>(optr->getref())->isbackground())
					{
						if (otype == CT_LAYER && t_object -> gettype() > CT_CARD || t_object -> gettype() == otype)
							return (MCControl *)t_object;
					}
				}
			}
		
			do
			{
				MCControl *foundobj = NULL;
				Chunk_term ttype = optr->getref()->gettype();

				// MW-2011-08-08: [[ Groups ]] Use 'isbackground()' rather than !F_GROUP_ONLY.
				if (ptype == CT_UNDEFINED
				        || otype == CT_GROUP && ttype == CT_GROUP
				        || ptype != CT_BACKGROUND && ttype != CT_GROUP
				        || (ttype == CT_GROUP && ptype == CT_BACKGROUND) == static_cast<MCGroup *>(optr->getref())->isbackground())
				{
					if (!optr->getref()->getopened())
						optr->getref()->setparent(this);
					foundobj = optr->getref()->findid(otype, tofindid, True);
				}
				if (foundobj != NULL)
				{
					if (foundobj->getparent()->gettype() == CT_STACK)
						foundobj->setparent(this);

					// MW-2012-10-10: [[ IdCache ]] Put the object in the id cache.
					t_stack -> cacheobjectbyid(foundobj);

					return foundobj;
				}
				optr = optr->next();
			}
			while (optr != objptrs);

			// If we are in group editing mode, also search the controls on the
			// phantom card if the ids match.
			MCGroup *t_editing;
			t_editing = getstack() -> getediting();

			// OK-2008-10-27 : [[Bug 7355]] - Infinite loop caused by incorrect
			// assumption that getcurcard() will always be equal to this.
			MCObjptr *t_objects;
			t_objects = getstack() -> getcurcard() -> objptrs;

			if (t_editing != NULL && t_editing -> getcard() -> obj_id == obj_id)
			{
				optr = t_objects;
				do
				{
					MCControl *foundobj = NULL;
					Chunk_term ttype = optr->getref()->gettype();

					// MW-2011-08-08: [[ Groups ]] Use 'isbackground()' rather than !F_GROUP_ONLY.
					if (ptype == CT_UNDEFINED
							|| otype == CT_GROUP && ttype == CT_GROUP
							|| ptype != CT_BACKGROUND && ttype != CT_GROUP
							|| (ttype == CT_GROUP && ptype == CT_BACKGROUND) == static_cast<MCGroup *>(optr->getref())->isbackground())
						foundobj = optr->getref()->findid(otype, tofindid, True);
					if (foundobj != NULL)
					{
						// MW-2012-10-10: [[ IdCache ]] Cache the object by id.
						t_stack -> cacheobjectbyid(foundobj);
						return foundobj;
					}
					optr = optr->next();
				}
				while (optr != t_objects);
			}
		}
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
				MCControl *foundobj = NULL;
				Chunk_term ttype = optr->getref()->gettype();					
				// MW-2011-08-08: [[ Groups ]] Use 'isbackground()' rather than !F_GROUP_ONLY.
				if (ptype == CT_UNDEFINED
					|| otype == CT_GROUP && ttype == CT_GROUP
					|| ptype != CT_BACKGROUND && ttype != CT_GROUP
					|| (ttype == CT_GROUP && ptype == CT_BACKGROUND) == static_cast<MCGroup *>(optr->getref())->isbackground())
				{
					if (!optr->getref()->getopened())
						optr->getref()->setparent(this);
					foundobj = optr->getref()->findname(otype, expression);
				}
				if (foundobj != NULL)
				{
					if (foundobj->getparent()->gettype() == CT_STACK)
						foundobj->setparent(this);
					return foundobj;
				}
				optr = optr->next();
			}
			while (optr != objptrs);
			return NULL;
		}
		break;
	default:
		return NULL;
	}
	do
	{
		MCControl *foundobj = NULL;
		Chunk_term ttype = optr->getref()->gettype();
		
		// MW-2011-08-08: [[ Groups ]] Use 'isbackground()' rather than !F_GROUP_ONLY.
		if (ptype == CT_UNDEFINED
		        || otype == CT_GROUP && ttype == CT_GROUP
		        || ptype != CT_BACKGROUND && ttype != CT_GROUP
		        || (ttype == CT_GROUP && ptype == CT_BACKGROUND) == static_cast<MCGroup *>(optr->getref())->isbackground())
		{
			if (!optr->getref()->getopened())
				optr->getref()->setparent(this);
			foundobj = optr->getref()->findnum(otype, num);
		}
		if (foundobj != NULL)
		{
			if (foundobj->getparent()->gettype() == CT_STACK)
				foundobj->setparent(this);
			return foundobj;
		}
		optr = optr->next();
	}
	while (optr != objptrs);
	return NULL;
}

Boolean MCCard::getchildid(uint4 inid)
{
	if (objptrs != NULL)
	{
		MCObjptr *optr = objptrs;
		do
		{
			if (optr->getid() == inid)
				return True;
			optr = (MCObjptr *)optr->next();
		}
		while (optr != objptrs);
	}
	return False;
}

Exec_stat MCCard::groupmessage(MCNameRef gmessage, MCCard *other)
{
	if (objptrs != NULL)
	{
		MCObjptr *optr = objptrs;
		do
		{
			// MW-2011-08-08: [[ Groups ]] Use 'getrefasgroup()' to test for groupness.
			MCGroup *t_group;
			t_group = optr -> getrefasgroup();
			// MW-2011-08-08: [[ Groups ]] Use 'isbackground()' rather than !F_GROUP_ONLY.
			if (t_group != nil && t_group -> isbackground() && (other == NULL || !other->getchildid(optr->getid())))
				if (message_with_args(gmessage, t_group->getid()) == ES_ERROR)
					return ES_ERROR;
			optr = optr->next();
		}
		while (optr != objptrs);
	}
	return ES_NORMAL;
}

void MCCard::installaccels(MCStack *stack)
{
	clean();
	if (objptrs != NULL)
	{
		MCObjptr *tptr = objptrs;
		do
		{
			tptr->getref()->installaccels(stack);
			tptr = tptr->next();
		}
		while (tptr != objptrs);
		if (!opened)
			clear();
	}
}

void MCCard::removeaccels(MCStack *stack)
{
	clean();
	if (objptrs != NULL)
	{
		MCObjptr *tptr = objptrs;
		do
		{
			tptr->getref()->removeaccels(stack);
			tptr = tptr->next();
		}
		while (tptr != objptrs);
		if (!opened)
			clear();
	}
}

void MCCard::resize(uint2 width, uint2 height)
{
	rect.x = rect.y = 0;
	rect.width = width;
	rect.height = height;
}

MCImage *MCCard::createimage()
{
	MCtemplateimage->setrect(rect);
	MCImage *iptr = new MCImage(*MCtemplateimage);
	getstack()->appendcontrol(iptr);
	mfocused = newcontrol(iptr, False);
	return iptr;
}

Boolean MCCard::removecontrol(MCControl *cptr, Boolean needredraw, Boolean cf)
{
	removedcontrol = NULL;
	if (objptrs == NULL)
		return False;

	MCStack *t_stack;
	t_stack = getstack();

	MCObjptr *optr = objptrs;
	MCObjptr *t_previous_optr = nil;
	do
	{
		if (optr->getref() == cptr)
		{
			if (cf)
				clearfocus(optr, NULL);
			else
				removedcontrol = optr;

			// MW-2011-08-19: [[ Layers ]] Compute the next objptr, or nil if we are at the end.
			MCObjptr *t_next_optr;
			t_next_optr = optr -> next();
			if (t_next_optr == objptrs)
				t_next_optr = nil;

			// Remove the control from the card and close it.
			optr->remove(objptrs);
			delete optr;
			if (opened)
			{
				cptr->close();
				cptr->setparent(t_stack);
			}

			// MW-2011-08-19: [[ Layers ]] Notify the stack that a layer has been removed.
			layer_removed(cptr, t_previous_optr, t_next_optr);

			return True;
		}
		t_previous_optr = optr;
		optr = optr->next();
	}
	while (optr != objptrs);

	return False;
}

void MCCard::clearfocus(MCObjptr *oldoptr, MCObjptr *newoptr)
{
	if (kfocused == oldoptr)
		kfocused = newoptr;
	if (oldkfocused == oldoptr)
		oldkfocused = newoptr;
	if (mfocused == oldoptr)
	{
		mfocused = newoptr;
		if (mgrabbed && newoptr == NULL)
			mgrabbed = False;
	}
}

void MCCard::erasefocus(MCObject *p_object)
{
	MCObjptr *optr = objptrs;
	do
	{
		if (optr->getref() == p_object)
		{
			clearfocus(optr, NULL);
			break;
		}

		optr = optr->next();
	}
	while (optr != objptrs);
}

MCObjptr *MCCard::newcontrol(MCControl *cptr, Boolean needredraw)
{
	if (opened)
	{
		cptr->setparent(this);
		cptr->open();
	}

	MCObjptr *newptr = new MCObjptr;
	newptr->setparent(this);
	newptr->setref(cptr);
	newptr->appendto(objptrs);

	// MW-2011-08-19: [[ Layers ]] Notify the stack that a layer may have ben inserted.
	layer_added(cptr, objptrs != newptr ? newptr -> prev() : nil, nil);

	return newptr;
}

void MCCard::resetid(uint4 oldid, uint4 newid)
{
	if (objptrs != NULL)
	{
		MCObjptr *tptr = objptrs;
		do
		{
			if (tptr->getid() == oldid)
			{
				tptr->setid(newid);
				return;
			}
			tptr = tptr->next();
		}
		while (tptr != objptrs);
	}
}

Boolean MCCard::checkid(uint4 controlid)
{
	if (objptrs != NULL)
	{
		MCObjptr *optr = objptrs;
		do
		{
			if (optr->getid() == controlid)
				return True;
			optr = optr->next();
		}
		while (optr != objptrs);
	}
	return False;
}

Boolean MCCard::find(MCExecPoint &ep, Find_mode mode, const MCString &tofind,
                     Boolean firstcard, Boolean firstword)
{
	if (flags & F_C_DONT_SEARCH)
		return False;
	
	clean();
	
	if (objptrs == NULL)
		return False;
	
	uint2 nfields;
	count(CT_FIELD, CT_UNDEFINED, NULL, nfields, False);
	uint2 i;
	for (i = 0 ; i < nfields ; i++)
	{
		MCObjptr *optr = objptrs;
		uint2 num = i;
		do
		{
			MCControl *cptr = optr->getref();
			MCField *fptr = (MCField *)cptr->findnum(CT_FIELD, num);
			if (fptr != NULL && (!firstcard || fptr == MCfoundfield))
			{
				MCObject *gptr = fptr->getparent();
				while (gptr->gettype() == CT_GROUP)
				{
					if (gptr->getflag(F_G_DONT_SEARCH))
					{
						fptr = NULL;
						break;
					}
					gptr = gptr->getparent();
				}
				firstcard = False;
				if (fptr != NULL && fptr->find(ep, obj_id, mode, tofind, firstword))
					return True;
				else
					break;
			}
			optr = optr->next();
		}
		while (optr != objptrs);
	}
	if (!opened)
		clear();
	return False;
}

MCObjptr *MCCard::getrefs()
{
	if (objptrs != NULL)
	{
		MCObjptr *optr = objptrs;
		do
		{
			optr->setref(optr->getref());
			optr = optr->next();
		}
		while (optr != objptrs);
	}
	return objptrs;
}

MCObjptr *MCCard::getobjptrforcontrol(MCControl *p_control)
{
	if (objptrs == nil)
		return nil;

	MCObjptr *t_ptr;
	t_ptr = objptrs;
	do
	{
		if (t_ptr -> getref() == p_control)
			return t_ptr;
		t_ptr = t_ptr -> next();
	}
	while(t_ptr != objptrs);
	
	return nil;
}

void MCCard::clean()
{
	if (objptrs == NULL || state & CS_OWN_CONTROLS)
		return;
	clear();
	MCObjptr *tptr = objptrs;
	Boolean check;
	do
	{
		check = False;
		MCControl *control = tptr->getref();
		MCObjptr *ntptr = tptr->next();
		if (control == NULL)
		{
			tptr->remove
			(objptrs);
			delete tptr;
			if (objptrs == NULL)
				break;
			check = True;
		}
		tptr = ntptr;
	}
	while (check || tptr != objptrs);
}

void MCCard::clear()
{
	if (state & CS_OWN_CONTROLS)
		return;
	MCObjptr *tptr = objptrs;
	do
	{
		tptr->clearref();
		tptr = tptr->next();
	}
	while (tptr != objptrs);
}

void MCCard::setmark(Boolean newstate)
{
	if (newstate)
		flags |= F_MARKED;
	else
		flags &= ~F_MARKED;
}

MCControl *MCCard::getkfocused()
{
	if (kfocused == NULL)
		return NULL;
	MCControl *cptr = kfocused->getref();
	while (cptr != NULL && cptr->gettype() == CT_GROUP)
	{
		MCGroup *gptr = (MCGroup *)cptr;
		cptr = gptr->getkfocused();
	}
	return cptr;
}

MCControl *MCCard::getmfocused()
{
	if (mfocused == NULL)
		return NULL;
	MCControl *cptr = mfocused->getref();
	while (cptr != NULL && cptr->gettype() == CT_GROUP)
	{
		MCGroup *gptr = (MCGroup *)cptr;
		cptr = gptr->getmfocused();
	}
	return cptr;
}

void MCCard::selectedbutton(uint2 n, Boolean bg, MCExecPoint &ep)
{
	Chunk_term ptype = bg ? CT_BACKGROUND : CT_CARD;
	uint2 i = 1;
	ep.clear();
	while (True)
	{
		char expression[U2L];
		sprintf(expression, "%d", i++);
		MCButton *bptr = (MCButton *)getchild(CT_EXPRESSION, expression, CT_BUTTON, ptype);
		if (bptr == NULL)
			break;
		if (bptr->getfamily() == n && bptr->gethilite(obj_id))
		{
			uint2 bnum = 0;
			getcard()->count(CT_BUTTON, bg ? CT_BACKGROUND : CT_CARD, bptr, bnum, True);
			ep.setstringf(bg ? "bkgnd button %d" : "card button %d", bnum);
			break;
		}
	}
}

void MCCard::freedefbutton(MCButton *btn)
{
	if (defbutton == btn)
		defbutton = NULL;
		
  // MW-2005-09-27: If we don't unset the odefbutton we end up with a crash periodically.
	if (odefbutton == btn)
		odefbutton = NULL;
}

MCRectangle MCCard::computecrect()
{
	MCRectangle minrect;
	MCU_set_rect(minrect, rect.width >> 1,  rect.height >> 1, 0, 0);
	if (objptrs != NULL)
	{
		MCObjptr *optr = objptrs;
		do
		{
			if (optr->getref()->isvisible())
				if (minrect.width == 0)
					minrect = optr->getref()->getrect();
				else
					minrect = MCU_union_rect(optr->getref()->getrect(), minrect);
			optr = optr->next();
		}
		while (optr != objptrs);
	}
	return minrect;
}

void MCCard::updateselection(MCControl *cptr, const MCRectangle &oldrect,
                             const MCRectangle &selrect, MCRectangle &drect)
{
	// MW-2008-01-30: [[ Bug 5749 ]] Make sure we check to see if the object is
	//   selectable - this will recurse up the object tree as necessary.
	if (!cptr -> isselectable()
	        && (!MCselectgrouped || cptr->gettype() != CT_GROUP))
		return;
	MCGroup *gptr = (MCGroup *)cptr;
	
	// MW-2008-12-04: [[ Bug ]] Make sure we honour group-local selectGrouped for
	//   select-tool drags
	if (MCselectgrouped && cptr->gettype() == CT_GROUP
	        && gptr->getcontrols() != NULL && gptr->getflag(F_VISIBLE) && !gptr->getflag(F_SELECT_GROUP))
	{
		cptr = gptr->getcontrols();
		do
		{
			updateselection(cptr, oldrect, selrect, drect);
			cptr = cptr->next();
		}
		while (cptr != gptr->getcontrols());
	}
	else
	{
		Boolean was, is;
		if (MCselectintersect)
		{
			was = cptr->maskrect(oldrect);
			is = cptr->maskrect(selrect);
		}
		else
		{
			was = MCU_rect_in_rect(cptr->getrect(), oldrect);
			is = MCU_rect_in_rect(cptr->getrect(), selrect);
		}
		if (is != was)
		{
			if (is)
				MCselected->add(cptr);
			else
				MCselected->remove(cptr);

			drect = MCU_union_rect(drect, cptr->getrect());
			
			// MM-2012-11-05: [[ Object selection started/ended message ]]
			if (!m_selecting_objects)
			{
				m_selecting_objects = true;
				message(MCM_object_selection_started);
			}
		}
	}
}

int2 MCCard::getborderwidth(void)
{
	if (!getflag(F_SHOW_BORDER))
		return 0;

	if (IsMacLF() && getstack() -> getmode() >= WM_PULLDOWN
								&& getstack() -> getmode() <= WM_CASCADE)
		return 3;

	return borderwidth;
}

void MCCard::drawcardborder(MCDC *dc, const MCRectangle &dirty)
{
	int2 bwidth;
	if (IsMacLF()
	        && getstack()->getmode() >= WM_PULLDOWN
	        && getstack()->getmode() <= WM_CASCADE)
		bwidth = 3;
	else
		bwidth = borderwidth;

	if (flags & F_SHOW_BORDER && dc -> gettype() != CONTEXT_TYPE_PRINTER
	        && (dirty.x < bwidth || dirty.y < bwidth
	            || dirty.x + dirty.width >= rect.width - bwidth
	            || dirty.y + dirty.height >= rect.height - bwidth))
	{
		dc->setclip(dirty);
		if (flags & F_3D)
			draw3d(dc, rect, ETCH_RAISED, borderwidth);
		else
			if (bwidth == 3)
			{
				rect.width--;
				rect.height--;
				drawborder(dc, rect, borderwidth);
				rect.width++;
				rect.height++;
				MCPoint p[3];
				p[0].x = p[1].x = 1;
				p[2].x = rect.width - 3;
				p[0].y = rect.height - 3;
				p[1].y = p[2].y = 1;
				dc->setforeground(dc->getwhite());
				dc->drawlines(p, 3);
				p[0].x = 2;
				p[1].x = p[2].x = rect.width - 3;
				p[0].y = p[1].y = rect.height - 3;
				p[2].y = 2;
				dc->setforeground(dc->getgray());
				dc->drawlines(p, 3);
				p[0].x = 2;
				p[1].x = p[2].x = rect.width - 1;
				p[0].y = p[1].y = rect.height - 1;
				p[2].y = 2;
				dc->setforeground(maccolors[MAC_SHADOW]);
				dc->drawlines(p, 3);
			}
			else
				drawborder(dc, rect, borderwidth);
		dc->clearclip();
	}
}


//-----------------------------------------------------------------------------
//  Redraw Management

void MCCard::draw(MCDC *dc, const MCRectangle& dirty, bool p_isolated)
{
	Window_mode wm = getstack()->getmode();
	
	bool t_draw_cardborder;
	t_draw_cardborder = true;

	// MW-2011-09-23: If we are a menuwindow, then draw a themed menu background
	//   otherwise fill a metal background, otherwise fill with background color.
	if (MCcurtheme != nil && getstack() -> menuwindow &&
		MCcurtheme -> drawmenubackground(dc, dirty, getrect(), true))
		t_draw_cardborder = false;
	else if (MCcurtheme == nil || !getstack() -> ismetal() ||
		!MCcurtheme -> drawmetalbackground(dc, dirty, rect, parent))
	{
		setforeground(dc, DI_BACK, False, MClook == LF_WIN95 && (wm == WM_COMBO || wm == WM_OPTION));
		dc -> fillrect(dirty);
	}

	if (objptrs != NULL)
	{
		MCObjptr *tptr = objptrs;
		do
		{
			tptr->getref()->redraw(dc, dirty);
			tptr = tptr->next();
		}
		while (tptr != objptrs);
	}

	dc -> setopacity(255);
	dc -> setfunction(GXcopy);
	dc -> setclip(dirty);

	if (t_draw_cardborder)
		drawcardborder(dc, dirty);
	
	if (getstate(CS_SIZE))
	{
		dc->setlineatts(0, LineDoubleDash, CapButt, JoinBevel);
		dc->setforeground(dc->getblack());
		dc->setbackground(dc->getwhite());
		dc->setdashes(0, dashlist, 2);
		dc->drawrect(selrect);
		dc->setlineatts(0, LineSolid, CapButt, JoinBevel);
		dc->setbackground(MCzerocolor);
	}
}

///////////////////////////////////////////////////////////////////////////////
//
//  SAVING AND LOADING
//

IO_stat MCCard::extendedload(MCObjectInputStream& p_stream, const char *p_version, uint4 p_length)
{
	return defaultextendedload(p_stream, p_version, p_length);
}

IO_stat MCCard::extendedsave(MCObjectOutputStream& p_stream, uint4 p_part)
{
	return defaultextendedsave(p_stream, p_part);
}

IO_stat MCCard::load(IO_handle stream, const char *version)
{
	IO_stat stat;

	if ((stat = MCObject::load(stream, version)) != IO_NORMAL)
		return stat;

//---- 2.7+:
//  . F_OPAQUE is now valid - default true
//  . ink is now valid - default GXcopy
	if (strncmp(version, "2.7", 3) < 0)
	{
		flags |= F_OPAQUE;
		ink = GXcopy;
	}
//---- 2.7+

	rect.y = 0; // in case saved on mac with editMenus false
	if ((stat = loadpropsets(stream)) != IO_NORMAL)
		return stat;
	while (True)
	{
		uint1 type;
		if ((stat = IO_read_uint1(&type, stream)) != IO_NORMAL)
			return stat;
		if (type == OT_PTR)
		{
			MCObjptr *newptr = new MCObjptr;
			if ((stat = newptr->load(stream)) != IO_NORMAL)
			{
				delete newptr;
				return stat;
			}
			newptr->setparent(this);
			newptr->appendto(objptrs);
		}
		else
		{
			MCS_seek_cur(stream, -1);
			break;
		}
	}
	return IO_NORMAL;
}

IO_stat MCCard::loadobjects(IO_handle stream, const char *version)
{
	IO_stat stat;
	
	if (objptrs != NULL)
	{
		MCObjptr *t_objptr;
		t_objptr = objptrs;
		do
		{
			uint1 t_object_type;
			if ((stat = IO_read_uint1(&t_object_type, stream)) != IO_NORMAL)
				return stat;

			MCControl *t_control;
			switch(t_object_type)
			{
			case OT_GROUP:
				t_control = new MCGroup;
			break;
			case OT_BUTTON:
				t_control = new MCButton;
			break;
			case OT_FIELD:
				t_control = new MCField;
			break;
			case OT_IMAGE:
				t_control = new MCImage;
			break;
			case OT_SCROLLBAR:
				t_control = new MCScrollbar;
			break;
			case OT_GRAPHIC:
				t_control = new MCGraphic;
			break;
			case OT_PLAYER:
				t_control = new MCPlayer;
			break;
			case OT_MCEPS:
				t_control = new MCEPS;
			break;
			case OT_MAGNIFY:
				t_control = new MCMagnify;
			break;
			case OT_COLORS:
				t_control = new MCColors;
			break;
			default:
				return IO_ERROR;
			break;
			}

			if ((stat = t_control -> load(stream, version)) != IO_NORMAL)
			{
				delete t_control;
				return stat;
			}

			t_objptr -> setref(t_control);

			t_objptr = t_objptr -> next();
		}
		while(t_objptr != objptrs);
	}

	return stat;
}

IO_stat MCCard::save(IO_handle stream, uint4 p_part, bool p_force_ext)
{
	IO_stat stat;

	if ((stat = IO_write_uint1(OT_CARD, stream)) != IO_NORMAL)
		return stat;

	uint4 t_old_flags;
	uint1 t_old_ink;
//---- 2.7+: 
//  . F_OPAQUE valid - must be true in older versions
//  . ink valid - must be GXcopy in older versions
	if (MCstackfileversion < 2700)
	{
		t_old_flags = flags;
		t_old_ink = GXcopy;
		flags |= F_OPAQUE;
	}
//---- 2.7+

	if ((stat = MCObject::save(stream, p_part, p_force_ext)) != IO_NORMAL)
		return stat;

//---- 2.7+
	if (MCstackfileversion < 2700)
	{
		flags = t_old_flags;
		ink = t_old_ink;
	}
//---- 2.7+

	if ((stat = savepropsets(stream)) != IO_NORMAL)
		return stat;
	if (objptrs != NULL)
	{
		MCObjptr *tptr = objptrs;
		do
		{
			if ((stat = tptr->save(stream, p_part)) != IO_NORMAL)
				return stat;
			tptr = tptr->next();
		}
		while (tptr != objptrs);
	}
	return IO_NORMAL;
}

IO_stat MCCard::saveobjects(IO_handle p_stream, uint4 p_part)
{
	IO_stat t_stat;

	if (objptrs != NULL)
	{
		MCObjptr *t_objptr;
		t_objptr = objptrs;
		do
		{
			if ((t_stat = t_objptr -> getref() -> save(p_stream, p_part, false)) != IO_NORMAL)
				return t_stat;
			t_objptr = t_objptr -> next();
		}
		while(t_objptr != objptrs);
	}

	return IO_NORMAL;
}
// MW-2008-10-31: [[ ParentScripts ]] This method sends the appropriate messages
//   to any backgrounds and their controls on this card
Exec_stat MCCard::openbackgrounds(bool p_is_preopen, MCCard *p_other)
{
	if (objptrs != NULL)
	{
		MCObjptr *optr = objptrs;
		do
		{
			// MW-2011-08-08: [[ Groups ]] Use 'getrefasgroup()' to test for groupness.
			MCGroup *t_group;
			t_group = optr -> getrefasgroup();
			
			// MW-2009-11-01: [[ Bug 8364 ]] Background messages were being sent incorrectly due
			//   to using p_other rather than optr - Doh!
			// MW-2011-08-08: [[ Groups ]] Use 'isbackground()' rather than !F_GROUP_ONLY.
			if (t_group != nil && t_group -> isbackground() &&
				(p_other == NULL || !p_other->getchildid(t_group->getid())))
			{
				if (message_with_args(p_is_preopen ? MCM_preopen_background : MCM_open_background, t_group -> getid()) == ES_ERROR)
					return ES_ERROR;

				if (t_group -> conditionalmessage(p_is_preopen ? HH_PREOPEN_CONTROL : HH_OPEN_CONTROL, p_is_preopen ? MCM_preopen_control : MCM_open_control) == ES_ERROR)
					return ES_ERROR;

				if (t_group -> opencontrols(p_is_preopen) == ES_ERROR)
					return ES_ERROR;
			}
			optr = optr->next();
		}
		while(optr != objptrs);
	}
	return ES_NORMAL;
}

Exec_stat MCCard::closebackgrounds(MCCard *p_other)
{
	if (objptrs != NULL)
	{
		MCObjptr *optr = objptrs;
		do
		{
			// MW-2011-08-08: [[ Groups ]] Use 'getrefasgroup()' to test for groupness.
			MCGroup *t_group;
			t_group = optr -> getrefasgroup();
			
			// MW-2009-11-01: [[ Bug 8364 ]] Background messages were being sent incorrectly due
			//   to using p_other rather than optr - Doh!
			// MW-2011-08-08: [[ Groups ]] Use 'isbackground()' rather than !F_GROUP_ONLY.
			if (t_group != nil && t_group -> isbackground() &&
				(p_other == NULL || !p_other->getchildid(t_group->getid())))
			{
				if (t_group -> closecontrols() == ES_ERROR)
					return ES_ERROR;
					
				if (t_group -> conditionalmessage(HH_CLOSE_CONTROL, MCM_close_control) == ES_ERROR)
					return ES_ERROR;
					
				if (message_with_args(MCM_close_background, t_group -> getid()) == ES_ERROR)
					return ES_ERROR;
			}
			optr = optr->next();
		}
		while(optr != objptrs);
	}
	return ES_NORMAL;
}

Exec_stat MCCard::opencontrols(bool p_is_preopen)
{
	if (objptrs != NULL)
	{
		MCObjptr *optr = objptrs;
		do
		{
			// MW-2011-08-08: [[ Groups ]] Use 'getrefasgroup()' to test for groupness.
			MCGroup *t_group;
			t_group = optr -> getrefasgroup();
			
			// MW-2011-08-08: [[ Groups ]] Use '!isbackground()' rather than F_GROUP_ONLY.
			if (t_group != nil && !t_group -> isbackground())
			{
				if (t_group -> conditionalmessage(p_is_preopen ? HH_PREOPEN_CONTROL : HH_OPEN_CONTROL, p_is_preopen ? MCM_preopen_control : MCM_open_control) == ES_ERROR)
					return ES_ERROR;
					
				if (t_group  -> opencontrols(p_is_preopen) == ES_ERROR)
					return ES_ERROR;
			}		
			
			optr = optr->next();
		}
		while (optr != objptrs);
	}
	return ES_NORMAL;
}

Exec_stat MCCard::closecontrols(void)
{
	if (objptrs != NULL)
	{
		MCObjptr *optr = objptrs -> prev();
		do
		{
			// MW-2011-08-08: [[ Groups ]] Use 'getrefasgroup()' to test for groupness.
			MCGroup *t_group;
			t_group = optr -> getrefasgroup();
			
			// MW-2011-08-08: [[ Groups ]] Use '!isbackground()' rather than F_GROUP_ONLY.
			if (t_group != nil && !t_group -> isbackground())
			{
				if (t_group -> closecontrols() == ES_ERROR)
					return ES_ERROR;
				
				if (t_group -> conditionalmessage(HH_CLOSE_CONTROL, MCM_close_control) == ES_ERROR)
					return ES_ERROR;
			}
					
			optr = optr->prev();
		}
		while (optr != objptrs -> prev());
	}
	return ES_NORMAL;
}


// OK-2009-01-19: Created to ensure MCMouseControl::eval behaves the same as MCProperty::eval where the property
// is a property of the mouseControl. Will return NULL if no mouseControl exists.
MCControl *MCCard::getmousecontrol(void)
{
	if (MCmousestackptr == NULL)
		return NULL;

	MCControl *t_focused;
	t_focused = MCmousestackptr -> getcard() -> getmfocused();

	if (t_focused != NULL)
		return t_focused;

	// OK-2008-12-11: [[Bug 7259]] - If focus has just been moved from an ungrouped control
	// to a grouped one, the mouseMove message will be sent before the card has had a chance
	// to update its mfocused member. This means that MCCard::getmfocused() will return null
	// even though the mouse is over the grouped control.
	t_focused = MCControl::getfocused();
	if (t_focused != NULL && t_focused -> getcard() == MCmousestackptr -> getcard())
		return t_focused;

	return NULL;
}

MCObject *MCCard::hittest(int32_t x, int32_t y)
{
	if (objptrs != nil)
	{
		MCObjptr *tptr = objptrs->prev();
		do
		{
			MCObject *t_object;
			t_object = tptr -> getref() -> hittest(x, y);
			if (t_object != nil)
				return t_object;
			
			tptr = tptr -> prev();
		}
		while(tptr != objptrs -> prev());
	}

	return this;
}

// MW-2011-09-20: [[ Collision ]] The card's shape is its rect.
bool MCCard::lockshape(MCObjectShape& r_shape)
{
	r_shape . type = kMCObjectShapeRectangle;
	
	// Object shapes are in card-relative co-ords.
	r_shape . bounds = getrect();
	r_shape . rectangle = r_shape . bounds;
	
	return true;
}

void MCCard::unlockshape(MCObjectShape& p_shape)
{
}

// MW-2012-02-14: [[ FontRefs ]] Update the card's font and then (if necessary) recurse
//   through all controls to do the same.
bool MCCard::recomputefonts(MCFontRef p_parent_font)
{
	// First update the font referenced by the card object. If this doesn't change
	// then none of the card's children will have either.
	if (!MCObject::recomputefonts(p_parent_font))
		return false;

	// Now iterate through all objects on the card, keeping track of whether any
	// children's fonts changed.
	bool t_changed;
	t_changed = false;

	if (objptrs != NULL)
	{
		MCObjptr *t_objptr;
		t_objptr = objptrs;
		do
		{
			if (t_objptr -> getref() -> recomputefonts(m_font))
				t_changed = true;
			t_objptr = t_objptr -> next();
		}
		while (t_objptr != objptrs);
	}

	// Return whether anything changed (the card's font has no effect other than
	// to be provided to children).
	return t_changed;
}

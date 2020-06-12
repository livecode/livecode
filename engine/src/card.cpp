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
#include "widget.h"
#include "graphics_util.h"

#include "globals.h"
#include "mctheme.h"
#include "context.h"

#include "exec.h"

#include "stackfileformat.h"

MCRectangle MCCard::selrect;
int2 MCCard::startx;
int2 MCCard::starty;
MCObjptr *MCCard::removedcontrol;

#ifdef _MAC_DESKTOP
extern bool MCosxmenupoppedup;
#endif

////////////////////////////////////////////////////////////////////////////////

MCPropertyInfo MCCard::kProperties[] =
{
	DEFINE_RW_OBJ_CUSTOM_PROPERTY(P_LAYER, InterfaceLayer, MCCard, Layer)
	DEFINE_RW_OBJ_CUSTOM_PROPERTY(P_NUMBER, InterfaceLayer, MCCard, Layer)

	DEFINE_RW_OBJ_PROPERTY(P_CANT_DELETE, Bool, MCCard, CantDelete)
	DEFINE_RW_OBJ_PROPERTY(P_DONT_SEARCH, Bool, MCCard, DontSearch)
	DEFINE_RW_OBJ_PROPERTY(P_MARKED, Bool, MCCard, Marked)
	DEFINE_RW_OBJ_PROPERTY(P_SHOW_PICT, Bool, MCCard, ShowPict)

	DEFINE_RO_OBJ_PROPERTY(P_FORMATTED_LEFT, Int16, MCCard, FormattedLeft)
	DEFINE_RO_OBJ_PROPERTY(P_FORMATTED_TOP, Int16, MCCard, FormattedTop)
	DEFINE_RO_OBJ_PROPERTY(P_FORMATTED_HEIGHT, Int16, MCCard, FormattedHeight)
	DEFINE_RO_OBJ_PROPERTY(P_FORMATTED_WIDTH, Int16, MCCard, FormattedWidth)
	DEFINE_RO_OBJ_PROPERTY(P_FORMATTED_RECT, Rectangle, MCCard, FormattedRect)

	DEFINE_RO_OBJ_PROPERTY(P_BACKGROUND_NAMES, String, MCCard, BackgroundNames)
	DEFINE_RO_OBJ_PROPERTY(P_BACKGROUND_IDS, String, MCCard, BackgroundIds)
	DEFINE_RO_OBJ_PROPERTY(P_SHARED_GROUP_NAMES, String, MCCard, SharedGroupNames)
	DEFINE_RO_OBJ_PROPERTY(P_SHARED_GROUP_IDS, String, MCCard, SharedGroupIds)
	DEFINE_RO_OBJ_PROPERTY(P_GROUP_IDS, String, MCCard, GroupIds)
	DEFINE_RO_OBJ_PROPERTY(P_GROUP_NAMES, String, MCCard, GroupNames)
    // MERG-2015-05-01: [[ ChildControlProps ]] Add ability to list both
    //   immediate and all descendent controls of a card.
    DEFINE_RO_OBJ_PART_PROPERTY(P_CONTROL_IDS, String, MCCard, ControlIds)
	DEFINE_RO_OBJ_PART_PROPERTY(P_CONTROL_NAMES, String, MCCard, ControlNames)
    DEFINE_RO_OBJ_PROPERTY(P_CHILD_CONTROL_IDS, String, MCCard, ChildControlIds)
	DEFINE_RO_OBJ_PROPERTY(P_CHILD_CONTROL_NAMES, String, MCCard, ChildControlNames)
	DEFINE_RO_OBJ_PROPERTY(P_DEFAULT_BUTTON, OptionalString, MCCard, DefaultButton)

	DEFINE_UNAVAILABLE_OBJ_PROPERTY(P_VISIBLE)
	DEFINE_UNAVAILABLE_OBJ_PROPERTY(P_INVISIBLE)
	DEFINE_UNAVAILABLE_OBJ_PROPERTY(P_ENABLED)
	DEFINE_UNAVAILABLE_OBJ_PROPERTY(P_DISABLED)
	DEFINE_UNAVAILABLE_OBJ_PROPERTY(P_TRAVERSAL_ON)
	DEFINE_UNAVAILABLE_OBJ_PROPERTY(P_SHADOW)
	DEFINE_UNAVAILABLE_OBJ_PROPERTY(P_LOCK_LOCATION)
	DEFINE_UNAVAILABLE_OBJ_PROPERTY(P_TOOL_TIP)
	// MW-2012-03-13: [[ UnicodeToolTip ]] Cards don't have tooltips.
	DEFINE_UNAVAILABLE_OBJ_PROPERTY(P_UNICODE_TOOL_TIP)

	DEFINE_RO_OBJ_NON_EFFECTIVE_PROPERTY(P_RECTANGLE, Rectangle, MCObject, Rectangle)
	DEFINE_RO_OBJ_EFFECTIVE_PROPERTY(P_RECTANGLE, Rectangle, MCObject, Rectangle)

	DEFINE_RO_OBJ_NON_EFFECTIVE_PROPERTY(P_WIDTH, UInt16, MCObject, Width)
	DEFINE_RO_OBJ_EFFECTIVE_PROPERTY(P_WIDTH, UInt16, MCObject, Width)
	DEFINE_RO_OBJ_NON_EFFECTIVE_PROPERTY(P_HEIGHT, UInt16, MCObject, Height)
	DEFINE_RO_OBJ_EFFECTIVE_PROPERTY(P_HEIGHT, UInt16, MCObject, Height)

	DEFINE_RO_OBJ_NON_EFFECTIVE_PROPERTY(P_LEFT, Int16, MCObject, Left)
	DEFINE_RO_OBJ_NON_EFFECTIVE_PROPERTY(P_RIGHT, Int16, MCObject, Right)
	DEFINE_RO_OBJ_NON_EFFECTIVE_PROPERTY(P_BOTTOM, Int16, MCObject, Bottom)
	DEFINE_RO_OBJ_NON_EFFECTIVE_PROPERTY(P_TOP, Int16, MCObject, Top)

	DEFINE_RO_OBJ_NON_EFFECTIVE_PROPERTY(P_TOP_LEFT, Point, MCObject, TopLeft)
	DEFINE_RO_OBJ_NON_EFFECTIVE_PROPERTY(P_TOP_RIGHT, Point, MCObject, TopRight)	
	DEFINE_RO_OBJ_NON_EFFECTIVE_PROPERTY(P_BOTTOM_LEFT, Point, MCObject, BottomLeft)
	DEFINE_RO_OBJ_NON_EFFECTIVE_PROPERTY(P_BOTTOM_RIGHT, Point, MCObject, BottomRight)

	DEFINE_RO_OBJ_EFFECTIVE_PROPERTY(P_LEFT, Int16, MCObject, Left)
	DEFINE_RO_OBJ_EFFECTIVE_PROPERTY(P_RIGHT, Int16, MCObject, Right)
	DEFINE_RO_OBJ_EFFECTIVE_PROPERTY(P_BOTTOM, Int16, MCObject, Bottom)
	DEFINE_RO_OBJ_EFFECTIVE_PROPERTY(P_TOP, Int16, MCObject, Top)

	DEFINE_RO_OBJ_EFFECTIVE_PROPERTY(P_TOP_LEFT, Point, MCObject, TopLeft)
	DEFINE_RO_OBJ_EFFECTIVE_PROPERTY(P_TOP_RIGHT, Point, MCObject, TopRight)	
	DEFINE_RO_OBJ_EFFECTIVE_PROPERTY(P_BOTTOM_LEFT, Point, MCObject, BottomLeft)
	DEFINE_RO_OBJ_EFFECTIVE_PROPERTY(P_BOTTOM_RIGHT, Point, MCObject, BottomRight)
};

MCObjectPropertyTable MCCard::kPropertyTable =
{
	&MCObject::kPropertyTable,
	sizeof(kProperties) / sizeof(kProperties[0]),
	&kProperties[0],
};

////////////////////////////////////////////////////////////////////////////////

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
			MCObjptr *newoptr = new (nothrow) MCObjptr;
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

bool MCCard::visit(MCObjectVisitorOptions p_options, uint32_t p_part, MCObjectVisitor *p_visitor)
{
	// If the card doesn't match the part number, we do nothing.
	if (p_part != 0 && getid() != p_part)
		return true;

	return MCObject::visit(p_options, p_part, p_visitor);
}

bool MCCard::visit_self(MCObjectVisitor *p_visitor)
{
	return p_visitor -> OnCard(this);
}

bool MCCard::visit_children(MCObjectVisitorOptions p_options, uint32_t p_part, MCObjectVisitor *p_visitor)
{
	bool t_continue;
	t_continue = true;

	if (t_continue && objptrs != NULL)
	{
		MCObjptr *t_objptr;
		t_objptr = objptrs;
		do
		{
			t_continue = t_objptr -> visit(p_options, p_part, p_visitor);
			t_objptr = t_objptr -> next();
		}
		while(t_continue && t_objptr != objptrs);
	}

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
        // MW-2014-08-12: [[ Bug 13167 ]] Sync the view focus before the engine state
        //   (otherwise the engine state can change due to script).
        MCscreen -> controlgainedfocus(getstack(), kfocused -> getid());
		// Mark card as focused
		setstate(true, CS_KFOCUSED);
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
                    // MW-2014-08-12: [[ Bug 13167 ]] Sync the view focus before the engine state
                    //   (otherwise the engine state can change due to script).
					MCscreen -> controllostfocus(getstack(), oldkfocused -> getid());
					// Mark card as unfocused
					setstate(false, CS_KFOCUSED);
					oldkfocused->getref()->kunfocus();
					if (oldkfocused == NULL)
						return False;
				}
				if (kfocused == NULL)
					kfocused = tptr;
			}
            // MW-2014-07-29: [[ Bug 13001 ]] Sync the view focus before the engine state
            //   (otherwise the engine state can change due to script).
			MCscreen -> controlgainedfocus(getstack(), kfocused -> getid());
			// Mark card as focused
			setstate(true, CS_KFOCUSED);
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
                    // MW-2014-08-12: [[ Bug 13167 ]] Sync the view focus before the engine state
                    //   (otherwise the engine state can change due to script).
					MCscreen -> controllostfocus(getstack(), oldkfocused -> getid());
					// Mark card as unfocused
					setstate(false, CS_KFOCUSED);
					oldkfocused->getref()->kunfocus();
					if (oldkfocused == NULL)
						return False;
				}
				if (kfocused == NULL)
					kfocused = tptr;
			}
            // MW-2014-07-29: [[ Bug 13001 ]] Sync the view focus before the engine state
            //   (otherwise the engine state can change due to script).
			MCscreen -> controlgainedfocus(getstack(), kfocused -> getid());
			// Mark card as focused
			setstate(true, CS_KFOCUSED);
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
        // MW-2014-08-12: [[ Bug 13167 ]] Sync the view focus before the engine state
        //   (otherwise the engine state can change due to script).
        MCscreen -> controllostfocus(getstack(), oldkfocused -> getid());
		// Mark card as unfocused
		setstate(false, CS_KFOCUSED);
		oldkfocused->getref()->kunfocus();
	}
	else
	{
		message(MCM_focus_out);
		oldkfocused = NULL;
	}
}

Boolean MCCard::kdown(MCStringRef p_string, KeySym key)
{
	MCtooltip->closetip();
	if (kfocused != NULL && getstack()->gettool(this) == T_BROWSE)
		return kfocused->getref()->kdown(p_string, key);
	if (MCObject::kdown(p_string, key))
		return True;
	return False;
}

Boolean MCCard::kup(MCStringRef p_string, KeySym key)
{
	if (kfocused != NULL && getstack()->gettool(this) == T_BROWSE)
		return kfocused->getref()->kup(p_string, key);
	return MCObject::kup(p_string, key);
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

bool MCCard::mfocus_control(int2 x, int2 y, bool p_check_selected)
{
    if (objptrs == nil)
        return false;
    
    MCObjptr *tptr = objptrs->prev();
    
    bool t_freed;
    do
    {
        MCControl *t_tptr_object;
        t_tptr_object = tptr -> getref();
        
        t_freed = false;
        
        // Check if any group's child is selected and focused
        if (p_check_selected && tptr -> getrefasgroup() != nil)
        {
            if (tptr -> getrefasgroup() -> mfocus_control(x, y, true))
            {
                mfocused = tptr;
                return true;
            }
        }
        
        bool t_focused;
        if (p_check_selected)
        {
            // On the first pass (checking selected objects), just check
            // if the object is selected and the mouse is inside a resize handle.
            t_focused = t_tptr_object -> getstate(CS_SELECTED)
                        && t_tptr_object -> sizehandles(x, y) != 0;
            
            // Make sure we still call mfocus as it updates the control's stored
            // mouse coordinates
            if (t_focused)
                t_tptr_object -> mfocus(x, y);
        }
        else
        {
            t_focused = t_tptr_object->mfocus(x, y);
        }
        
        if (t_focused)
        {
            // MW-2010-10-28: If mfocus calls relayer, then the objptrs can get changed.
            //   Reloop to find the correct one.
            tptr = objptrs -> prev();
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
            
            // The widget event manager handles enter/leave itself
            if (newfocused && mfocused != NULL &&
                mfocused -> getref() -> gettype() != CT_GROUP &&
#ifdef WIDGETS_HANDLE_DND
                mfocused -> getref() -> gettype() != CT_WIDGET)
#else
                (MCdispatcher -> isdragtarget() ||
                 mfocused -> getref() -> gettype() != CT_WIDGET))
#endif
            {
                mfocused->getref()->enter();
                
                // MW-2007-10-31: mouseMove sent before mouseEnter - make sure we send an mouseMove
                //   It is possible for mfocused to become NULL if its deleted in mouseEnter so
                //   we check first.
                if (mfocused != NULL)
                    mfocused->getref()->mfocus(x, y);
            }
            
            return true;
        }
        
        // Unset previously focused object
        if (!p_check_selected && tptr == mfocused)
        {
            // MW-2012-02-22: [[ Bug 10018 ]] Previously, if a group was hidden and it had
            //   mouse focus, then it wouldn't unmfocus as there was an explicit check to
            //   stop this (for groups) here.
            // MW-2012-03-13: [[ Bug 10074 ]] Invoke the control's munfocus() method for groups
            //   if the group has an mfocused control.
            if (mfocused -> getref() -> gettype() != CT_GROUP
                || mfocused -> getrefasgroup() -> getmfocused() != nil)
            {
                MCControl *oldfocused = mfocused->getref();
                mfocused = NULL;
                oldfocused->munfocus();
            }
            else
            {
                mfocused -> getrefasgroup() -> clearmfocus();
                mfocused = nil;
            }
            
            // If munfocus calls relayer, then the objptrs can get changed
            // so we need to loop back to the start of the objptrs again
            t_freed = true;
            tptr = objptrs->prev();
        }
        else
        {
            tptr = tptr->prev();
        }
    }
    while (t_freed || tptr != objptrs->prev());
    
    return false;
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

                /* The set of selected controls has changed so dirty the old
                 * rect, and the new. */
				dirtyselection(oldrect);
                dirtyselection(selrect);
			}
			message_with_args(MCM_mouse_move, x, y);
			return true;
		}
		if (mgrabbed)
			mfocused->getref()->mfocus(x, y);
		mgrabbed = False;
        
        if (mfocus_control(x, y, true))
            return true;
        
        if (mfocus_control(x, y, false))
            return true;
        
		MCtooltip->cleartip();
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
	MCtooltip->cleartip();
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

	MCclickfield = nil;
	mgrabbed = True;
	if (mfocused != NULL)
	{
		MCControl *oldfocused = mfocused->getref();
        
        // AL-2013-01-14: [[ Bug 11343 ]] Add timer if the object handles mouseStillDown in the behavior chain.
        // MW-2014-07-29: [[ Bug 13010 ]] Make sure we use the deepest mfocused child.
        MCControl *t_child_oldfocused = getmfocused();
        if (t_child_oldfocused != NULL &&
            t_child_oldfocused -> handlesmessage(MCM_mouse_still_down))
            MCscreen->addtimer(t_child_oldfocused, MCM_idle, MCidleRate);

		if (!mfocused->getref()->mdown(which)
		        && getstack()->gettool(this) == T_BROWSE)
		{
            message_with_valueref_args(MCM_mouse_down, MCSTR("1"));
		}
		if (!(MCbuttonstate & (0x1L << (which - 1))))
		{
			Boolean oldstate = MClockmessages;
			MClockmessages = True;
			if (!mup(which, false))
				oldfocused->mup(which, false);
			MClockmessages = oldstate;
		}
		return True;
	}
	else
	{
        // AL-2013-01-14: [[ Bug 11343 ]] Add timer to the card if it handles mouseStillDown in the behavior chain.
        if (handlesmessage(MCM_mouse_still_down))
            MCscreen->addtimer(this, MCM_idle, MCidleRate);
        
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
				message_with_valueref_args(MCM_mouse_down, MCSTR("1"));
				if (!(MCbuttonstate & (0x1L << (which - 1))))
				{
					Boolean oldstate = MClockmessages;
					MClockmessages = True;
					mup(which, false);
					MClockmessages = oldstate;
				}
				break;
			case T_POINTER:
				// MW-2010-10-15: [[ Bug 9055 ]] Make sure the card script gets a chance to cancel selection handling
				if (message_with_valueref_args(MCM_mouse_down, MCSTR("1")) == ES_NORMAL && !MCexitall)
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
				cptr = new (nothrow) MCButton(*MCtemplatebutton);
				break;
			case T_SCROLLBAR:
				cptr = new (nothrow) MCScrollbar(*MCtemplatescrollbar);
				break;
			case T_PLAYER:
				cptr = new (nothrow) MCPlayer(*MCtemplateplayer);
				break;
			case T_GRAPHIC:
				cptr = new (nothrow) MCGraphic(*MCtemplategraphic);
				break;
			case T_FIELD:
				cptr = new (nothrow) MCField(*MCtemplatefield);
				break;
			case T_IMAGE:
				cptr = new (nothrow) MCImage(*MCtemplateimage);
				break;
			case T_DROPPER:
				// IM-2013-09-23: [[ FullscreenMode ]] Get mouse loc in view coordinates
				MCStack *t_mousestack;
				MCPoint t_mouseloc;
				MCscreen->getmouseloc(t_mousestack, t_mouseloc);
				MCscreen->dropper(getw(), t_mouseloc.x, t_mouseloc.y, NULL);
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
        // When a control is created with its constructor, it will copy
        // the id of the given control. In the case of template objects
        // (as above) the id will be zero. Therefore, we must make sure
        // the object has a valid id here, otherwise it gets attached to
        // the card via an objptr with id 0 - which is not good.
        cptr -> setid(getstack() -> newid());
        
		getstack()->appendcontrol(cptr);
		mfocused = newcontrol(cptr, False);
		cptr->create(MCmousex, MCmousey);
	}
	return True;
}

Boolean MCCard::mup(uint2 which, bool p_release)
{
	if (state & CS_MENU_ATTACHED)
		return MCObject::mup(which, p_release);
	if (mfocused != NULL)
	{
		mgrabbed = False;
		return mfocused->getref()->mup(which, p_release);
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
                    
                    /* The selection marquee has finished, so update the selection
                     * layer. */
                    dirtyselection(selrect);
					
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
                    if (p_release)
                        message_with_valueref_args(MCM_mouse_release, MCSTR("1"));
                    else
                        message_with_valueref_args(MCM_mouse_up, MCSTR("1"));
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
                // MW-2014-07-24: [[ Bug 12882 ]] Make sure we pass through correct button number.
                if (p_release)
                    message_with_valueref_args(MCM_mouse_release, which == Button2 ? MCSTR("2") : MCSTR("3"));
                else
                    message_with_valueref_args(MCM_mouse_up, which == Button2 ? MCSTR("2") : MCSTR("3"));
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
			message_with_valueref_args(MCM_mouse_double_down, MCSTR("1"));
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
				message_with_valueref_args(MCM_mouse_double_down, MCSTR("1"));
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
				message_with_valueref_args(MCM_mouse_double_up, MCSTR("1"));
				break;
			case T_POINTER:
				getstack()->kfocusset(NULL);
				if (MCmodifierstate & MS_MOD1)
					editscript();
				else
				{
					MCselected->replace(this);
					message_with_valueref_args(MCM_mouse_double_up, MCSTR("1"));
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
	if (MCNameIsEqualToCaseless(mptr, MCM_idle))
	{
		if (opened)
		{
			Boolean again = False;
			Tool tool =  getstack()->gettool(this);
			if (mfocused == NULL && MCbuttonstate)
			{
				if (tool == T_BROWSE)
				{
					if (message(MCM_mouse_still_down) == ES_ERROR)
						senderror();
					else
						again = True;
				}
			}
			if (hashandlers & HH_IDLE)
			{
				if (tool == T_BROWSE)
				{
					if (message(MCM_idle) == ES_ERROR)
						senderror();
					else
						again = True;
				}
			}
			if (again)
				MCscreen->addtimer(this, MCM_idle, MCidleRate);
		}
	}
	else
		MCObject::timer(mptr, params);
}

bool MCCard::isdeletable(bool p_check_flag)
{
    if (!parent || scriptdepth != 0 ||
       (p_check_flag && getflag(F_C_CANT_DELETE)) ||
       getstack() -> isediting())
    {
        MCAutoValueRef t_long_name;
        getnameproperty(P_LONG_NAME, 0, &t_long_name);
        MCeerror->add(EE_OBJECT_CANTREMOVE, 0, 0, *t_long_name);
        return false;
    }

    if (objptrs != NULL)
    {
        MCObjptr *t_object_ptr = objptrs;
        do
        {
            MCGroup * t_group = t_object_ptr->getrefasgroup();
            
            if (t_group != nil && t_group->isshared())
            {
                // if it's a shared group then don't check deletability because the object won't be deleted
            }
            else if (!t_object_ptr->getref()->isdeletable(true))
                return false;
            
            t_object_ptr = t_object_ptr->next();
        }
        while (t_object_ptr != objptrs);
    }
    
    return true;
}

Boolean MCCard::del(bool p_check_flag)
{
	if (!isdeletable(p_check_flag))
	    return False;
	
    clean();
    
	MCselected->remove(this);
	
	// MW-2008-10-31: [[ ParentScripts ]] Make sure we close the controls
	closecontrols();
	message(MCM_close_card);
	message(MCM_delete_card);
	
	MCundos->freestate();
	state |= CS_NO_MESSAGES;
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
                MCControl *cptr = optr->getref();
                
                cptr->removereferences();
				getstack()->removecontrol(cptr);
                cptr->scheduledelete();
			}
			optr = optr->next();
		}
		while (optr != objptrs);
	}
    
    removereferences();
    
    // MCObject now does things on del(), so we must make sure we finish by
    // calling its implementation.
    return MCObject::del(p_check_flag);
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

		if (parent && (stat == ES_PASS || stat == ES_NOT_HANDLED))
		{
			Exec_stat oldstat = stat;
			stat = parent->handle(htype, mess, params, this);
			if (oldstat == ES_PASS && stat == ES_NOT_HANDLED)
				stat = ES_PASS;
		}
	}

	if (stat == ES_ERROR && !MCerrorptr)
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

void MCCard::toolchanged(Tool p_new_tool)
{
    if (objptrs != NULL)
    {
        MCObjptr *optr = objptrs;
        do
        {
            optr->getref()->toolchanged(p_new_tool);
            optr = optr->next();
        }
        while (optr != objptrs);
    }
}

void MCCard::OnViewTransformChanged()
{
	if (objptrs != nil)
	{
		MCObjptr *t_ptr = objptrs;
		do
		{
			MCObject *t_obj = t_ptr->getref();
			if (t_obj != nil)
				t_obj->OnViewTransformChanged();
			t_ptr = t_ptr->next();
		}
		while (t_ptr != objptrs);
	}
}

void MCCard::OnAttach()
{
	if (objptrs != NULL)
	{
		MCObjptr *optr = objptrs;
		do
		{
			MCObject *t_obj = optr->getref();
			if (t_obj != nil)
				t_obj->OnAttach();
			optr = optr->next();
		}
		while (optr != objptrs);
	}
}

void MCCard::OnDetach()
{
	if (objptrs != NULL)
	{
		MCObjptr *optr = objptrs;
		do
		{
			MCObject *t_obj = optr->getref();
			if (t_obj != nil)
				t_obj->OnDetach();
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
            // MW-2014-08-12: [[ Bug 13167 ]] Sync the view focus before the engine state
            //   (otherwise the engine state can change due to script).
            MCscreen -> controllostfocus(getstack(), tkfocused -> getid());
			// Mark card as unfocused
			setstate(false, CS_KFOCUSED);
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
                // MW-2014-08-12: [[ Bug 13167 ]] Sync the view focus before the engine state
                //   (otherwise the engine state can change due to script).
                MCscreen -> controlgainedfocus(getstack(), kfocused -> getid());
				// Mark card as focused
				setstate(true, CS_KFOCUSED);
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

MCCard *MCCard::clone(Boolean attach, Boolean controls, MCStack *p_parent)
{
	clean();
	MCCard *newcptr = new (nothrow) MCCard(*this);
    if (p_parent == nullptr)
    {
        p_parent = MCdefaultstackptr;
    }
	newcptr->parent = p_parent;
	Boolean diffstack = getstack() != MCdefaultstackptr;
	if (controls)
	{
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
            newcptr->obj_id = newcptr->getstack()->newid();
			newcptr->getstack()->appendcard(newcptr);
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
		MCCard *sourceptr = newcptr->getstack()->getchild(CT_THIS, kMCEmptyString, CT_CARD);
		if (sourceptr != NULL && sourceptr->objptrs != NULL)
		{
			MCObjptr *optr = sourceptr->objptrs;
			do
			{
				// MW-2011-08-09: [[ Groups ]] If a group is a background then auto
				//   place it on the new card.
				if (optr->getref()->gettype() == CT_GROUP && static_cast<MCGroup *>(optr -> getref()) -> isbackground())
				{
					MCObjptr *newoptr = new (nothrow) MCObjptr;
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
			MCCdata *dptr = new (nothrow) MCCdata(*cptr);
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
	MCControl *t_previous, *t_next;
	t_previous = MCControlPreviousByLayer(p_source);
	t_next = MCControlNextByLayer(p_source);

	// If the source control already precedes the target then we are done.
	if (t_next == p_target)
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
	layer_added(p_source, MCControlPreviousByLayer(p_source), MCControlNextByLayer(p_source));
}

void MCCard::relayercontrol_remove(MCControl *p_control)
{
	MCObjptr *t_control_ptr;
	t_control_ptr = getobjptrforcontrol(p_control);
	
	MCControl *t_previous, *t_next;
	t_previous = MCControlPreviousByLayer(p_control);
	t_next = MCControlNextByLayer(p_control);
	
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
	t_control_ptr = new (nothrow) MCObjptr;
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
	layer_added(p_control, MCControlPreviousByLayer(p_control), MCControlNextByLayer(p_control));
}

Exec_stat MCCard::relayer(MCControl *optr, uint2 newlayer)
{
	if (!opened || (!MCrelayergrouped && optr->getparent()->gettype() == CT_GROUP) || (optr -> getparent() -> gettype() == CT_CARD && optr -> getparent() != this))
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
        /* Find place in the object pointer list at which the
         * relayered object should be placed.  If t_before, the object
         * should be placed immediately before the iterator. */
        MCObjptr *t_insert_iter = objptrs;
        bool t_before = false;
        if (foundobj == nullptr)
        {
            t_before = (newlayer <= 1);
        }
        else
        {
            /* If there's a target object for relayering, and group
             * relayering is banned, then we need to find the outer
             * group of the target object. */
            bool t_found_in_group = false;
            while (!MCrelayergrouped &&
                   foundobj->getparent() != this)
            {
                foundobj = MCObjectCast<MCControl>(foundobj->getparent());
                t_found_in_group = true;
            }

            do
            {
				if (foundobj == t_insert_iter->getref())
				{
					if (t_found_in_group)
					{
						uint2 t_minimum_layer = 0;
						count(CT_LAYER, CT_UNDEFINED, t_insert_iter->next()->getref(),
							  t_minimum_layer, True);
						if (t_insert_iter->next() == objptrs ||
							newlayer < t_minimum_layer)
						{
							t_before = true;
							if (t_insert_iter == objptrs)
								foundobj = nullptr;
						}
					}
					break;
				}
				t_insert_iter = t_insert_iter->next();            }
            while (t_insert_iter != objptrs);
        }

        /* Create and insert an object pointer */
        MCObjptr *newptr = new (nothrow) MCObjptr();
        if (newptr == nullptr)
            return ES_ERROR;
        newptr->setparent(this);
        newptr->setref(optr);
        optr->setparent(this);
        getstack()->appendcontrol(optr);

        if (foundobj == nullptr)
        {
            if (t_before)
                newptr->insertto(objptrs);
            else
                newptr->appendto(objptrs);
        }
        else
        {
            if (t_before)
                t_insert_iter->prev()->append(newptr);
            else
                t_insert_iter->append(newptr);
        }

		layer_added(optr, MCControlPreviousByLayer(optr), MCControlNextByLayer(optr));
	}

	if (oldparent == this)
	{
		MCObjptr *newoptr = NULL;
		tptr = objptrs;
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

    optr->layerchanged();
    
	if (getstack() == MCmousestackptr
	        && MCU_point_in_rect(optr->getrect(), MCmousex, MCmousey))
		mfocus(MCmousex, MCmousey);

	return ES_NORMAL;
}

////////////////////////////////////////////////////////////////////////////////

struct ViewportChangedVisitor: public MCObjectVisitor
{
	MCRectangle m_viewport_rect;

	ViewportChangedVisitor(const MCRectangle &p_viewport_rect)
		: m_viewport_rect(p_viewport_rect)
	{
	}

	bool OnObject(MCObject *p_object)
	{
		p_object->viewportgeometrychanged(m_viewport_rect);
		return true;
	}
};

void MCCard::geometrychanged(const MCRectangle &p_rect)
{
	MCObject::geometrychanged(p_rect);

	// notify children of viewport change
	ViewportChangedVisitor t_visitor(p_rect);
	visit_children(kMCObjectVisitorHeirarchical, 0, &t_visitor);
}

////////////////////////////////////////////////////////////////////////////////

MCCard *MCCard::findname(Chunk_term type, MCNameRef inname)
{
	if (type == CT_CARD && MCU_matchname(inname, CT_CARD, getname()))
		return this;
	else
		return NULL;
}

MCCard *MCCard::findid(Chunk_term type, uint4 inid, Boolean alt)
{
	if (type == CT_CARD && (inid == obj_id || (alt && inid == altid)))
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
			    || (otype == CT_GROUP && ttype == CT_GROUP)
			    || (ptype != CT_BACKGROUND && ttype != CT_GROUP)
			    || ((ttype == CT_GROUP && ptype == CT_BACKGROUND) == static_cast<MCGroup *>(optr->getref())->isbackground()))
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

MCControl *MCCard::getnumberedchild(integer_t p_number, Chunk_term p_obj_type, Chunk_term p_parent_type)
{
	if (p_obj_type <= CT_CARD)
		return NULL;
	
	MCObjptr *t_optr = objptrs;
	if (t_optr == NULL)
		return NULL;
	
	if (!opened)
		clean();
	
	if (p_obj_type != CT_LAYER && p_obj_type != CT_MENU && p_parent_type == CT_UNDEFINED
	        && getstack()->hcaddress())
		p_parent_type = p_obj_type == CT_FIELD ? CT_BACKGROUND : CT_CARD;

	if (p_number < 1)
		return NULL;

	uint16_t t_num = p_number - 1;
	do
	{
		MCControl *t_foundobj = NULL;
		Chunk_term t_type = t_optr->getref()->gettype();
		
		// MW-2011-08-08: [[ Groups ]] Use 'isbackground()' rather than !F_GROUP_ONLY.
		if (p_parent_type == CT_UNDEFINED
		    || (p_obj_type == CT_GROUP && t_type == CT_GROUP)
		    || (p_parent_type != CT_BACKGROUND && t_type != CT_GROUP)
		    || ((t_type == CT_GROUP && p_parent_type == CT_BACKGROUND) == static_cast<MCGroup *>(t_optr->getref())->isbackground()))
		{
			if (!t_optr->getref()->getopened())
				t_optr->getref()->setparent(this);
			t_foundobj = t_optr->getref()->findnum(p_obj_type, t_num);
		}
		if (t_foundobj != NULL)
		{
			if (t_foundobj->getparent()->gettype() == CT_STACK)
				t_foundobj->setparent(this);
			return t_foundobj;
		}
		t_optr = t_optr->next();
	}
	while (t_optr != objptrs);
	return NULL;
}

MCControl *MCCard::getchild(Chunk_term etype, MCStringRef p_expression,
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
		if (MCU_stoui4(p_expression, tofindid))
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
					    || (otype == CT_GROUP && ttype == CT_GROUP)
					    || (ptype != CT_BACKGROUND && ttype != CT_GROUP)
					    || ((ttype == CT_GROUP && ptype == CT_BACKGROUND) == static_cast<MCGroup *>(optr->getref())->isbackground()))
					{
						if ((otype == CT_LAYER && t_object -> gettype() > CT_CARD) || t_object -> gettype() == otype)
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
				    || (otype == CT_GROUP && ttype == CT_GROUP)
				    || (ptype != CT_BACKGROUND && ttype != CT_GROUP)
				    || ((ttype == CT_GROUP && ptype == CT_BACKGROUND) == static_cast<MCGroup *>(optr->getref())->isbackground()))
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
					    || (otype == CT_GROUP && ttype == CT_GROUP)
					    || (ptype != CT_BACKGROUND && ttype != CT_GROUP)
					    || ((ttype == CT_GROUP && ptype == CT_BACKGROUND) == static_cast<MCGroup *>(optr->getref())->isbackground()))
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
		if (MCU_stoui2(p_expression, num))
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
				    || (otype == CT_GROUP && ttype == CT_GROUP)
				    || (ptype != CT_BACKGROUND && ttype != CT_GROUP)
				    || ((ttype == CT_GROUP && ptype == CT_BACKGROUND) == static_cast<MCGroup *>(optr->getref())->isbackground()))
				{
					if (!optr->getref()->getopened())
						optr->getref()->setparent(this);
					MCNewAutoNameRef t_name;
					/* UNCHECKED */ MCNameCreate(p_expression, &t_name);
					foundobj = optr->getref()->findname(otype, *t_name);
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

	return getnumberedchild(num + 1, otype, ptype);
}

MCControl *MCCard::getchildbyordinal(Chunk_term p_ordinal_type, Chunk_term p_object_type, Chunk_term p_parent_type)
{
	// MM-2012-12-02: [[ Bug ]] Make sure we clean the object list before checking if null.
	if (!opened)
		clean();
	
	MCObjptr *optr = objptrs;
	if (optr == nil)
		return nil;
	
	if (p_object_type != CT_LAYER && p_object_type != CT_MENU && p_object_type == CT_UNDEFINED && getstack()->hcaddress())
		p_parent_type = p_object_type == CT_FIELD ? CT_BACKGROUND : CT_CARD;
    
	uint2 num = 0;
    
	switch (p_ordinal_type)
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
            num = p_ordinal_type - CT_FIRST;
            break;
        case CT_LAST:
        case CT_MIDDLE:
        case CT_ANY:
            count(p_object_type, p_parent_type, NULL, num, True);
            switch (p_ordinal_type)
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
        default:
            return nil;
    }
    return getnumberedchild(num + 1, p_object_type, p_parent_type);
}

MCControl *MCCard::getchildbyid(uinteger_t p_id, Chunk_term p_object_type, Chunk_term p_parent_type)
{
	// MM-2012-12-02: [[ Bug ]] Make sure we clean the object list before checking if null.
	if (!opened)
		clean();
	
	MCObjptr *optr = objptrs;
	if (optr == nil)
		return nil;
	
	if (p_object_type != CT_LAYER && p_object_type != CT_MENU && p_parent_type == CT_UNDEFINED
        && getstack()->hcaddress())
		p_parent_type = p_object_type == CT_FIELD ? CT_BACKGROUND : CT_CARD;
    
    MCStack *t_stack;
    t_stack = getstack();
    
    // MW-2012-10-10: [[ IdCache ]] First check the id-cache.
    MCObject *t_object;
    t_object = t_stack -> findobjectbyid(p_id);
    if (t_object != nil)
    {
        MCObject *t_parent;
        t_parent = t_object -> getparent();
        while(t_parent -> gettype() != CT_CARD && t_parent -> gettype() != CT_STACK)
            t_parent = t_parent -> getparent();
        if (t_parent == this)
        {
            Chunk_term ttype = optr->getref()->gettype();
            if (p_parent_type == CT_UNDEFINED
                || (p_object_type == CT_GROUP && ttype == CT_GROUP)
                || (p_parent_type != CT_BACKGROUND && ttype != CT_GROUP)
                || (ttype == CT_GROUP && p_parent_type == CT_BACKGROUND) == static_cast<MCGroup *>(optr->getref())->isbackground())
            {
                if ((p_object_type == CT_LAYER && t_object -> gettype() > CT_CARD) || t_object -> gettype() == p_object_type)
                    return (MCControl *)t_object;
            }
        }
    }
		
    do
    {
        MCControl *foundobj = NULL;
        Chunk_term ttype = optr->getref()->gettype();
        
        // MW-2011-08-08: [[ Groups ]] Use 'isbackground()' rather than !F_GROUP_ONLY.
        if (p_parent_type == CT_UNDEFINED
            || (p_object_type == CT_GROUP && ttype == CT_GROUP)
            || (p_parent_type != CT_BACKGROUND && ttype != CT_GROUP)
            || (ttype == CT_GROUP && p_parent_type == CT_BACKGROUND) == static_cast<MCGroup *>(optr->getref())->isbackground())
        {
            if (!optr->getref()->getopened())
                optr->getref()->setparent(this);
            foundobj = optr->getref()->findid(p_object_type, p_id, True);
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
    
    // AL-2015-03-31: [[ Bug 15123 ]] Ensure we don't enter the loop if there are no objects
    if (t_objects != nil && t_editing != NULL && t_editing -> getcard() -> obj_id == obj_id)
    {
        optr = t_objects;
        do
        {
            MCControl *foundobj = NULL;
            Chunk_term ttype = optr->getref()->gettype();
            
            // MW-2011-08-08: [[ Groups ]] Use 'isbackground()' rather than !F_GROUP_ONLY.
            if (p_parent_type == CT_UNDEFINED
                || (p_object_type == CT_GROUP && ttype == CT_GROUP)
                || (p_parent_type != CT_BACKGROUND && ttype != CT_GROUP)
                || (ttype == CT_GROUP && p_parent_type == CT_BACKGROUND) == static_cast<MCGroup *>(optr->getref())->isbackground())
                foundobj = optr->getref()->findid(p_object_type, p_id, True);
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

    return NULL;
}

MCControl *MCCard::getchildbyname(MCNameRef p_name, Chunk_term p_object_type, Chunk_term p_parent_type)
{
    if (p_object_type <= CT_CARD)
        return nil;
    
	// MM-2012-12-02: [[ Bug ]] Make sure we clean the object list before checking if null.
	if (!opened)
		clean();
	
	MCObjptr *optr = objptrs;
	if (optr == nil)
		return nil;
	
	if (p_object_type != CT_LAYER && p_object_type != CT_MENU && p_parent_type == CT_UNDEFINED
        && getstack()->hcaddress())
		p_parent_type = p_object_type == CT_FIELD ? CT_BACKGROUND : CT_CARD;
    
    uint2 t_num = 0;
    if (MCU_stoui2(MCNameGetString(p_name), t_num))
    {
        if (t_num < 1)
            return nil;
        t_num--;
        
        return getnumberedchild(t_num + 1, p_object_type, p_parent_type);
    }
    
    do
    {
        MCControl *foundobj = nil;
        Chunk_term ttype = optr->getref()->gettype();
        
        // MW-2011-08-08: [[ Groups ]] Use 'isbackground()' rather than !F_GROUP_ONLY.
        if (p_parent_type == CT_UNDEFINED
            || (p_object_type == CT_GROUP && ttype == CT_GROUP)
            || (p_parent_type != CT_BACKGROUND && ttype != CT_GROUP)
            || (ttype == CT_GROUP && p_parent_type == CT_BACKGROUND) == static_cast<MCGroup *>(optr->getref())->isbackground())
        {
            if (!optr->getref()->getopened())
                optr->getref()->setparent(this);
            foundobj = optr->getref()->findname(p_object_type, p_name);
        }
        if (foundobj != nil)
        {
            if (foundobj->getparent()->gettype() == CT_STACK)
                foundobj->setparent(this);
            return foundobj;
        }
        optr = optr->next();
    }
    while (optr != objptrs);
    return nil;
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

	geometrychanged(rect);
}

MCImage *MCCard::createimage()
{
	MCtemplateimage->setrect(rect);
	MCImage *iptr = new (nothrow) MCImage(*MCtemplateimage);
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
	do
	{
		if (optr->getref() == cptr)
		{
			if (cf)
				clearfocus(optr, NULL);
			else
				removedcontrol = optr;

			// MW-2011-08-19: [[ Layers ]] Compute the next objptr, or nil if we are at the end.
			MCControl *t_previous, *t_next;
			t_previous = MCControlPreviousByLayer(optr->getref());
			t_next = MCControlNextByLayer(optr->getref());

			// Remove the control from the card and close it.
			optr->remove(objptrs);
			delete optr;
            
            // MW-2011-08-19: [[ Layers ]] Notify the stack that a layer has been removed.
            layer_removed(cptr, t_previous, t_next);
            
			if (opened)
			{
				cptr->close();
				cptr->setparent(t_stack);
			}

			return True;
		}
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
	cptr->setparent(this);

	MCObjptr *newptr = new (nothrow) MCObjptr;
	newptr->setparent(this);
	newptr->setref(cptr);
	newptr->appendto(objptrs);

	// MW-2011-08-19: [[ Layers ]] Notify the stack that a layer may have ben inserted.
	layer_added(cptr, MCControlPreviousByLayer(cptr), MCControlNextByLayer(cptr));

	if (opened)
		cptr->open();
	
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

Boolean MCCard::find(MCExecContext &ctxt, Find_mode mode, MCStringRef tofind,
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
				if (fptr != NULL && fptr->find(ctxt, obj_id, mode, tofind, firstword))
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
	if (objptrs == NULL)
		return;
	clear();
	MCObjptr *tptr = objptrs;
	Boolean check;
	do
	{
		check = False;
		MCObjectHandle control = tptr->Get();
		MCObjptr *ntptr = tptr->next();
        
        // If the control has been deleted, remove it from the object list
		if (!control.IsValid())
		{
			tptr->remove(objptrs);
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

bool MCCard::selectedbutton(integer_t p_family, bool p_background, MCStringRef& r_string)
{
	Chunk_term ptype = p_background ? CT_BACKGROUND : CT_CARD;
	integer_t i = 1;
	MCButton *bptr;
	while (nil != (bptr = (MCButton *)getnumberedchild(i, CT_BUTTON, ptype)))
	{
		if (bptr->getfamily() == p_family && !(bptr->gethilite(obj_id).isFalse()))
		{
			uint2 bnum = 0;
			getcard()->count(CT_BUTTON, ptype, bptr, bnum, True);
			return MCStringFormat(r_string, p_background ? "bkgnd button %d" : "card button %d", bnum);
		}
		i++;
	}
	r_string = MCValueRetain(kMCEmptyString);
	return true;
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
			{
				if (minrect.width == 0)
					minrect = optr->getref()->getrect();
				else
					minrect = MCU_union_rect(optr->getref()->getrect(), minrect);
			}
			optr = optr->next();
		}
		while (optr != objptrs);
	}
	return minrect;
}

void MCCard::updateselection(MCControl *cptr, const MCRectangle &oldrect,
                             const MCRectangle &p_selrect, MCRectangle &drect)
{
	// MW-2008-01-30: [[ Bug 5749 ]] Make sure we check to see if the object is
	//   selectable - this will recurse up the object tree as necessary.
	if (!cptr -> isselectable()
	        && (!MCselectgrouped || cptr->gettype() != CT_GROUP))
		return;
	
	// MW-2008-12-04: [[ Bug ]] Make sure we honour group-local selectGrouped for
	//   select-tool drags
	if (MCselectgrouped && cptr->gettype() == CT_GROUP)
	{
        MCGroup *gptr = MCObjectCast<MCGroup>(cptr);

		if (gptr->getcontrols() != NULL && gptr->getflag(F_VISIBLE) && !gptr->getflag(F_SELECT_GROUP))
		{
			cptr = gptr->getcontrols();
        
			MCRectangle t_group_rect;
			t_group_rect = gptr -> getrect();
			
			MCRectangle t_group_oldrect;
			t_group_oldrect = MCU_intersect_rect(oldrect, t_group_rect);
			
			MCRectangle t_group_selrect;
			t_group_selrect = MCU_intersect_rect(p_selrect, t_group_rect);
			
			do
			{
			    MCRectangle t_rect;
			    t_rect = cptr -> getrect();
			    if (MCU_line_intersect_rect(t_group_rect, t_rect))
				updateselection(cptr, t_group_oldrect, t_group_selrect, drect);
			    
			    cptr = cptr->next();
			}
			while (cptr != gptr->getcontrols());
		}
	}
	else
	{
		Boolean was, is;
		if (MCselectintersect)
		{
            was = cptr->maskrect(oldrect);
            is = cptr->maskrect(p_selrect);

            // AL-2015-10-07:: [[ External Handles ]] If either dimension is 0,
            //  recheck the selection intersect
            MCRectangle t_rect;
            t_rect = cptr -> getrect();

            if (t_rect . width == 0 || t_rect . height == 0)
            {
                if (!was)
                    was = MCU_line_intersect_rect(oldrect, t_rect);
                
                if (!is)
                    is = MCU_line_intersect_rect(p_selrect, t_rect);
            }
		}
		else
		{
			was = MCU_rect_in_rect(cptr->getrect(), oldrect);
			is = MCU_rect_in_rect(cptr->getrect(), p_selrect);
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
		dc->save();
		dc->cliprect(dirty);
		
		if (flags & F_3D)
			draw3d(dc, rect, ETCH_RAISED, borderwidth);
		else if (bwidth == 3)
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
		
		dc->restore();
	}
}


//-----------------------------------------------------------------------------
//  Redraw Management

// IM-2013-09-13: [[ RefactorGraphics ]] Factor out card background drawing to separate method
void MCCard::drawbackground(MCContext *p_context, const MCRectangle &p_dirty)
{
	if (MCcurtheme != nil && getstack() -> ismetal() && MCcurtheme -> drawmetalbackground(p_context, p_dirty, rect, parent))
		return;
	
	// IM-2013-09-13: [[ RefactorGraphics ]] [[ Bug 11175 ]] Rework card background drawing to handle transparent background patterns
	// transparent backgrounds will now draw on top of the stack background, which in turn draws on top of solid black if transparent
	MCColor color;
	MCPatternRef t_pattern = nil;
	int2 x, y;
	
	MCPatternRef t_stack_pattern = nil;
	int16_t t_stack_x, t_stack_y;
	
	Window_mode wm = getstack()->getmode();
	
	Boolean t_hilite;
	t_hilite = MClook == LF_WIN95 && (wm == WM_COMBO || wm == WM_OPTION);
	
	bool t_opaque;
	t_opaque = getforecolor(DI_BACK, False, t_hilite, color, t_pattern, x, y, p_context -> gettype(), this) || MCPatternIsOpaque(t_pattern);
	
	// If the card background is a pattern with transparency, then draw the stack background first
	if (!t_opaque)
	{
		t_opaque = parent->getforecolor(DI_BACK, False, t_hilite, color, t_stack_pattern, t_stack_x, t_stack_y, p_context -> gettype(), parent) || MCPatternIsOpaque(t_stack_pattern);
		
		// And if the stack background is a pattern with transparency, then fill with black first
		if (!t_opaque)
		{
			p_context->setforeground(p_context->getblack());
			p_context->setfillstyle(FillSolid, nil, 0, 0);
			p_context->fillrect(p_dirty);
		}
		
		if (t_stack_pattern != nil)
			p_context->setfillstyle(FillTiled, t_stack_pattern, t_stack_x, t_stack_y);
		else
		{
			p_context->setforeground(color);
			p_context->setfillstyle(FillSolid, nil, 0, 0);
		}
		
		p_context->fillrect(p_dirty);
	}
	
	if (t_pattern != nil)
		p_context->setfillstyle(FillTiled, t_pattern, x, y);
	else
	{
		p_context->setforeground(color);
		p_context->setfillstyle(FillSolid, nil, 0, 0);
	}
	
	p_context->fillrect(p_dirty);
}

/* The card's drawselection method first renders the selections of all children
 * and then renders the marquee. */
void MCCard::drawselection(MCContext *p_context, const MCRectangle& p_dirty)
{
    MCObjptr *tptr = objptrs;
    if (tptr == nil)
        return;

    do
    {
        MCControl *t_control = tptr->getref();
        if (t_control != nullptr &&
            t_control->getopened() != 0 &&
            (t_control->getflag(F_VISIBLE) || showinvisible()))
        {
            t_control->drawselection(p_context, p_dirty);
        }
            
        tptr = tptr->next();
    }
    while (tptr != objptrs);
    
    if (getstate(CS_SIZE))
    {
        drawmarquee(p_context, selrect);
    }
}

void MCCard::dirtyselection(const MCRectangle &p_rect)
{
    MCRectangle t_rect = MCU_reduce_rect(p_rect, -(1 + MCsizewidth / 2));
    
    MCTileCacheRef t_tilecache = getstack()->view_gettilecache();
    if (t_tilecache != nullptr)
    {
        MCGAffineTransform t_transform =
                getstack()->getdevicetransform();
        
        MCRectangle32 t_device_rect =
                MCRectangle32GetTransformedBounds(t_rect, t_transform);
        
        MCTileCacheUpdateScenery(t_tilecache, m_fg_layer_id, t_device_rect);
    }
    
    layer_dirtyrect(t_rect);
}

void MCCard::draw(MCDC *dc, const MCRectangle& dirty, bool p_isolated)
{
	bool t_draw_cardborder;
	t_draw_cardborder = true;

	// MW-2011-09-23: If we are a menuwindow, then draw a themed menu background
	//   otherwise fill a metal background, otherwise fill with background color.
	if (MCcurtheme != nil && getstack() -> menuwindow &&
		MCcurtheme -> drawmenubackground(dc, dirty, getrect(), true))
		t_draw_cardborder = false;
	else
		drawbackground(dc, dirty);

	if (objptrs != NULL)
	{
		MCObjptr *tptr = objptrs;
		do
		{
            MCControl *t_control = tptr->getref();
            if (t_control != nullptr)
                t_control->redraw(dc, dirty);
			tptr = tptr->next();
		}
		while (tptr != objptrs);
	}

	dc -> setopacity(255);
	dc -> setfunction(GXcopy);

	if (t_draw_cardborder)
		drawcardborder(dc, dirty);
	
    drawselection(dc, dirty);
}

///////////////////////////////////////////////////////////////////////////////
//
//  SAVING AND LOADING
//

IO_stat MCCard::extendedload(MCObjectInputStream& p_stream, uint32_t p_version, uint4 p_length)
{
	return defaultextendedload(p_stream, p_version, p_length);
}

IO_stat MCCard::extendedsave(MCObjectOutputStream& p_stream, uint4 p_part, uint32_t p_version)
{
	return defaultextendedsave(p_stream, p_part, p_version);
}

IO_stat MCCard::load(IO_handle stream, uint32_t version)
{
	IO_stat stat;

	if ((stat = MCObject::load(stream, version)) != IO_NORMAL)
		return checkloadstat(stat);

//---- 2.7+:
//  . F_OPAQUE is now valid - default true
//  . ink is now valid - default GXcopy
	if (version < kMCStackFileFormatVersion_2_7)
	{
		flags |= F_OPAQUE;
		ink = GXcopy;
	}
//---- 2.7+

	rect.y = 0; // in case saved on mac with editMenus false
	if ((stat = loadpropsets(stream, version)) != IO_NORMAL)
		return checkloadstat(stat);
	while (True)
	{
		uint1 type;
		if ((stat = IO_read_uint1(&type, stream)) != IO_NORMAL)
			return checkloadstat(stat);
		if (type == OT_PTR)
		{
			MCObjptr *newptr = new (nothrow) MCObjptr;
			if ((stat = newptr->load(stream)) != IO_NORMAL)
			{
				delete newptr;
				return checkloadstat(stat);
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

IO_stat MCCard::loadobjects(IO_handle stream, uint32_t version)
{
	IO_stat stat = IO_NORMAL;
	
	if (objptrs != NULL)
	{
		MCObjptr *t_objptr;
		t_objptr = objptrs;
		do
		{
			uint1 t_object_type;
			if ((stat = IO_read_uint1(&t_object_type, stream)) != IO_NORMAL)
				return checkloadstat(stat);

			MCControl *t_control;
			switch(t_object_type)
			{
			case OT_GROUP:
				t_control = new (nothrow) MCGroup;
			break;
			case OT_BUTTON:
				t_control = new (nothrow) MCButton;
			break;
			case OT_FIELD:
				t_control = new (nothrow) MCField;
			break;
			case OT_IMAGE:
				t_control = new (nothrow) MCImage;
			break;
			case OT_SCROLLBAR:
				t_control = new (nothrow) MCScrollbar;
			break;
			case OT_GRAPHIC:
				t_control = new (nothrow) MCGraphic;
			break;
			case OT_PLAYER:
				t_control = new (nothrow) MCPlayer;
			break;
			case OT_MCEPS:
				t_control = new (nothrow) MCEPS;
			break;
            case OT_WIDGET:
                t_control = new (nothrow) MCWidget;
                break;
			case OT_MAGNIFY:
				t_control = new (nothrow) MCMagnify;
			break;
			case OT_COLORS:
				t_control = new (nothrow) MCColors;
			break;
			default:
				return checkloadstat(IO_ERROR);
			break;
			}

			if ((stat = t_control -> load(stream, version)) != IO_NORMAL)
			{
				delete t_control;
				return checkloadstat(stat);
			}

			t_objptr -> setref(t_control);

			t_objptr = t_objptr -> next();
		}
		while(t_objptr != objptrs);
	}

	return checkloadstat(stat);
}

IO_stat MCCard::save(IO_handle stream, uint4 p_part, bool p_force_ext, uint32_t p_version)
{
	IO_stat stat;

	if ((stat = IO_write_uint1(OT_CARD, stream)) != IO_NORMAL)
		return stat;

	uint4 t_old_flags;
	uint1 t_old_ink;
//---- 2.7+: 
//  . F_OPAQUE valid - must be true in older versions
//  . ink valid - must be GXcopy in older versions
	if (p_version < kMCStackFileFormatVersion_2_7)
	{
		t_old_flags = flags;
		t_old_ink = GXcopy;
		flags |= F_OPAQUE;
	}
//---- 2.7+

	if ((stat = MCObject::save(stream, p_part, p_force_ext, p_version)) != IO_NORMAL)
		return stat;

//---- 2.7+
	if (p_version < kMCStackFileFormatVersion_2_7)
	{
		flags = t_old_flags;
		ink = t_old_ink;
	}
//---- 2.7+

	if ((stat = savepropsets(stream, p_version)) != IO_NORMAL)
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

IO_stat MCCard::saveobjects(IO_handle p_stream, uint4 p_part, uint32_t p_version)
{
	IO_stat t_stat;

	if (objptrs != NULL)
	{
		MCObjptr *t_objptr;
		t_objptr = objptrs;
		do
		{
			if ((t_stat = t_objptr -> getref() -> save(p_stream, p_part, false, p_version)) != IO_NORMAL)
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
	if (!MCmousestackptr)
		return nil;

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
bool MCCard::recomputefonts(MCFontRef p_parent_font, bool p_force)
{
	// First update the font referenced by the card object. If this doesn't change
	// then none of the card's children will have either.
	if (!MCObject::recomputefonts(p_parent_font, p_force))
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
			if (t_objptr -> getref() -> recomputefonts(m_font, p_force))
				t_changed = true;
			t_objptr = t_objptr -> next();
		}
		while (t_objptr != objptrs);
	}

	// Return whether anything changed (the card's font has no effect other than
	// to be provided to children).
	return t_changed;
}

void MCCard::scheduledelete(bool p_is_child)
{
    MCObject::scheduledelete(p_is_child);
    
    /* No need to recurse through children, as they are scheduled for
     * deletion in ::del */
}

MCPlatformControlType MCCard::getcontroltype()
{
    MCPlatformControlType t_type;
    t_type = MCObject::getcontroltype();
    
    if (t_type != kMCPlatformControlTypeGeneric)
        return t_type;
    else
        return kMCPlatformControlTypeWindow;
}

MCPlatformControlPart MCCard::getcontrolsubpart()
{
    return kMCPlatformControlPartNone;
}


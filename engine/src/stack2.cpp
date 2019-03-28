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


#include "stack.h"
#include "tooltip.h"
#include "dispatch.h"
#include "aclip.h"
#include "vclip.h"
#include "card.h"
#include "objptr.h"
#include "mccontrol.h"
#include "image.h"
#include "player.h"
#include "field.h"
#include "button.h"
#include "group.h"
#include "stacklst.h"
#include "cardlst.h"
#include "sellst.h"
#include "hndlrlst.h"
#include "mcerror.h"
#include "util.h"
#include "printer.h"
#include "license.h"
#include "globals.h"
#include "mctheme.h"
#include "mode.h"
#include "redraw.h"
#include "region.h"
#include "tilecache.h"
#include "stacksecurity.h"

#include "context.h"
#include "graphicscontext.h"
#include "resolution.h"

#include "stacktile.h"

void MCStack::external_idle()
{
	if (idlefunc != NULL)
		(*idlefunc)();
}

void MCStack::setidlefunc(void (*newfunc)())
{
	idlefunc = newfunc;
	hashandlers |= HH_IDLE;
	MCscreen->addtimer(this, MCM_idle, MCidleRate);
}

// MW-2014-10-24: [[ Bug 13796 ]] Separate script setting from commandline from other cases.
Boolean MCStack::setscript_from_commandline(MCStringRef newscript)
{
	MCValueAssign(_script, newscript);
	parsescript(False);
    MCAutoPointer<char> t_mccmd;
    /* UNCHECKED */ MCStringConvertToCString(MCcmd, &t_mccmd);
	if (hlist == NULL)
	{
		uint2 line, pos;
		MCperror->geterrorloc(line, pos);
		fprintf(stderr, "%s: Script parsing error at line %d, column %d\n",
		        *t_mccmd, line, pos);
		return False;
	}
	if (!hlist->hashandlers())
	{
		fprintf(stderr, "%s: Script has no handlers\n", *t_mccmd);
		return False;
	}
	return True;
}

void MCStack::checkdestroy()
{
	if (MCdispatcher->ismainstack(this))
	{
		if (opened == 0 && MCdispatcher->gethome() != this)
			if (flags & F_DESTROY_STACK)
			{
				if (substacks != NULL)
				{
					MCStack *sptr = substacks;
					do
					{
						if (sptr->opened)
							return;
						sptr = sptr->next();
					}
					while (sptr != substacks);
				}
                MCtodestroy -> remove(this); // prevent duplicates
                MCtodestroy -> add(this);
			}
	}
	else if (!MCdispatcher -> is_transient_stack(this))
	{
		MCStack *sptr = parent.GetAs<MCStack>();
		sptr->checkdestroy();
	}
}

void MCStack::resize(uint2 oldw, uint2 oldh)
{
	uint2 newy = getscroll();
	
	// IM-2014-01-07: [[ StackScale ]] compare old & new card rects and notify if changed
	MCRectangle t_new_cardrect;
	t_new_cardrect = getcardrect();
	
	MCRectangle t_old_cardrect;
	t_old_cardrect = curcard->getrect();
	
	// MW-2011-08-18: [[ Redraw ]] Use global screen lock, rather than stack.
	MCRedrawLockScreen();
	if (t_old_cardrect.width != t_new_cardrect.width || t_old_cardrect.height != t_new_cardrect.height)
	{
		updatecardsize();

		// MM-2012-09-05: [[ Property Listener ]] Make sure resizing a stack sends the propertyChanged message to any listeners.
		//    This effects both the current card and the stack.
		curcard -> signallisteners(P_RECTANGLE);
		signallisteners(P_RECTANGLE);	
		
		curcard->message_with_args(MCM_resize_stack, rect.width, rect.height + newy, oldw, oldh);
	}
	MCRedrawUnlockScreen();
}

static bool _MCStackConfigureCallback(MCStack *p_stack, void *p_context)
{
	p_stack->configure(True);
	
	return true;
}

// MW-2007-07-03: [[ Bug 2332 ]] - It makes more sense for resize to be issue before move.
//   Not doing so results in the stack's rect property being incorrect size-wise during execution
//   of moveStack.
void MCStack::configure(Boolean user)
{
	// MW-2011-08-18: [[ Redraw ]] Update to use redraw.
	if (MCRedrawIsScreenLocked() || state & CS_NO_CONFIG || !haswindow() || !opened)
		return;
#ifdef TARGET_PLATFORM_LINUX
 	if (!getflag(F_VISIBLE))
		return;
#endif
	Boolean beenchanged = False;
	MCRectangle trect;
	// IM-2014-01-16: [[ StackScale ]] Get stack viewport from the view
	trect = view_getstackviewport();
	
	if (trect.width != 0 && trect.height != 0
	        && (trect.width != rect.width || trect.height != rect.height))
	{
		uint2 oldw = rect.width;
		uint2 oldh = rect.height;
		rect.width = trect.width;
		rect.height = trect.height;
		if (opened)
		{
			beenchanged = True;
			resize(oldw, oldh + getscroll());
		}
	}
	if ((trect.x != -1 || trect.y != -1)
	        && (trect.x != rect.x || trect.y != rect.y))
	{
		rect.x = trect.x;
		rect.y = trect.y;
		beenchanged = True;
		if (user)
		{ // else WM did it
			state |= CS_BEEN_MOVED;
			// MM-2012-09-05: [[ Property Listener ]] Make sure a stack move sends the propertyChanged message to any listeners.
			signallisteners(P_RECTANGLE);
			curcard->message_with_args(MCM_move_stack, rect.x, rect.y);
		}
	}
	if (beenchanged)
		foreachchildstack(_MCStackConfigureCallback, nil);
}

void MCStack::seticonic(Boolean on)
{
	if (on && !(state & CS_ICONIC))
	{
		MCiconicstacks++;
		state |= CS_ICONIC;
	}
	else if (!on && (state & CS_ICONIC))
		{
			MCiconicstacks--;
			state &= ~CS_ICONIC;
		}
}

void MCStack::iconify()
{
	if (!(state & CS_ICONIC))
    {
        MCtooltip->cleartip();
        seticonic(true);
		MCstacks->top(NULL);
		redrawicon();
		curcard->message(MCM_iconify_stack);
	}
}

void MCStack::uniconify()
{
	if (state & CS_ICONIC)
	{
		seticonic(false);
		MCstacks->top(this);
		curcard->message(MCM_uniconify_stack);
		// MW-2011-08-17: [[ Redraw ]] Tell the stack to dirty all of itself.
		dirtyall();
		resetcursor(True);
		dirtywindowname();
	}
}

Window_mode MCStack::getmode()
{
	if (!opened)
		return WM_CLOSED;
	if (state & CS_ICONIC)
		return WM_ICONIC;
	return mode;
}

uint2 MCStack::userlevel()
{
	return getstyleint(flags);
}

Boolean MCStack::hcaddress()
{
	return (flags & F_HC_ADDRESSING) != 0;
}

Boolean MCStack::hcstack()
{
	return (flags & F_HC_STACK) != 0;
}

Boolean MCStack::islocked()
{
	return (flags & F_CANT_MODIFY) != 0;
}

Boolean MCStack::isiconic()
{
	return (state & CS_ICONIC) != 0;
}

Boolean MCStack::isediting()
{
	return editing != NULL;
}

Tool MCStack::gettool(MCObject *optr) const
{
	if (MCcurtool == T_HELP)
		return T_HELP;

	// MW-2008-01-30: [[ Bug 5749 ]] Recurse up the object tree to see if any
	//   parent of the object (up to card level) has CANT_SELECT set, if so
	//   force browse mode.
    // AL-2014-01-14: [[ Bug 11419 ]] Allow graphic drawing to begin in non-selectable regions.
    // AL-2014-01-23: [[ Bug 11702 ]] Change in cantSelect behavior makes it possible to create graphics on IDE stacks.
	if (mode == WM_TOP_LEVEL && (MCcurtool != T_POINTER || optr -> isselectable()))
		return MCcurtool;

	return T_BROWSE;
}

void MCStack::hidecursor()
{
	if (MCmousestackptr.IsBoundTo(this))
	{
		cursor = MCcursors[PI_NONE];
		MCscreen->setcursor(window, cursor);
	}
}

void MCStack::setcursor(MCCursorRef newcursor, Boolean force)
{
	if (window == NULL && MCModeMakeLocalWindows())
		return;
	
	if (MCwatchcursor)
		newcursor = MCcursors[PI_WATCH];
	else
		if (MCcursor != nil)
			newcursor = MCcursor;
	if (force || cursor != newcursor)
	{
		cursor = newcursor;
		mode_setcursor();
	}
}

MCCursorRef MCStack::getcursor()
{
	Tool t;
	if (controls != NULL && controls->gettype() == CT_MAGNIFY)
		t = MCcurtool;
	else
		t = gettool(this);
	uint2 cindex;
	switch (t)
	{
	case T_POINTER:
		cindex = PI_ARROW;
		break;
	case T_HELP:
		cindex = PI_HELP;
		break;
	case T_BRUSH:
		cindex = PI_BRUSH;
		break;
	case T_ERASER:
		cindex = PI_ERASER;
		break;
	case T_SPRAY:
		cindex = PI_SPRAY;
		break;
	case T_BUCKET:
		cindex = PI_BUCKET;
		break;
	case T_PENCIL:
		cindex = PI_PENCIL;
		break;
	case T_DROPPER:
		cindex = PI_DROPPER;
		break;
	case T_BUTTON:
	case T_SCROLLBAR:
	case T_FIELD:
	case T_IMAGE:
	case T_PLAYER:
	case T_CURVE:
	case T_LASSO:
	case T_LINE:
	case T_OVAL:
	case T_POLYGON:
	case T_RECTANGLE:
	case T_REGULAR_POLYGON:
	case T_ROUND_RECT:
	case T_SELECT:
	case T_TEXT:
	case T_GRAPHIC:
		cindex = PI_PLUS;
		break;
	default:
		if (ibeam)
			cindex = PI_IBEAM;
		else
			if (MCdefaultcursorid)
				return MCdefaultcursor;
			else
				cindex = PI_HAND;
		break;
	}
	return MCcursors[cindex];
}

void MCStack::resetcursor(Boolean force)
{
	setcursor(getcursor(), force);
}

void MCStack::clearcursor(void)
{
	for(uint32_t i = 0; i < PI_NCURSORS; i++)
		if (MCcursors[i] == cursor)
			cursor = nil;
}

void MCStack::setibeam()
{
	if (opened)
	{
		ibeam++;
		resetcursor(True);
	}
}

void MCStack::clearibeam()
{
	if (opened)
	{
		// TS 2007-22-10 : This check was put in to stop the IBEAM swapping when you have dragged and dropped
		// 					between fields. ibeam is an UNSIGNED int, so it will underflow and become confused.
		if ( ibeam > 0 ) 
			ibeam--;
		resetcursor(True);
	}
}

// MW-2012-09-07: [[ Bug 10372 ]] If 'force' is set then ignores current settings of substacks.
void MCStack::extraopen(bool p_force)
{
	if (substacks != NULL || p_force)
	{
		setextendedstate(true, ECS_ISEXTRAOPENED);
		MCObject::open();
		opened--;
	}
}


// MW-2012-09-07: [[ Bug 10372 ]] If 'force' is set then ignores current settings of substacks.
void MCStack::extraclose(bool p_force)
{
	if (substacks != NULL || p_force)
	{
		setextendedstate(false, ECS_ISEXTRAOPENED);
		opened++;
		MCObject::close();
		}
	}

Window MCStack::getwindow()
{
#if defined(_MACOSX) || defined(_LINUX)
	if (!opened)
#else
	if (!opened || state & CS_ICONIC)
#endif

		return NULL;
	else
		return window;
}

// IM-2014-07-23: [[ Bug 12930 ]] Reimplement to return the window of the parent stack
Window MCStack::getparentwindow()
{
	MCStack *t_parent;
	t_parent = getparentstack();
	
	if (t_parent == nil)
		return nil;
	
	return t_parent->getwindow();
}

void MCStack::setparentstack(MCStack *p_parent)
{
	MCStack *t_parent = getparentstack();
	
	if (t_parent == p_parent)
		return;
	
	if (p_parent != nil)
		m_parent_stack = p_parent->GetHandle();
    else
        m_parent_stack = nil;
}

MCStack *MCStack::getparentstack()
{
	if (!m_parent_stack.IsBound())
		return nil;
	
	if (!m_parent_stack.IsValid())
	{
		m_parent_stack = nil;
		return nil;
	}
	
	return m_parent_stack;
}

static bool _MCStackTakeWindowCallback(MCStack *p_stack, void *p_context)
{
	p_stack->setparentstack((MCStack*)p_context);
	
	return true;
}

Boolean MCStack::takewindow(MCStack *sptr)
{
	// If there is no window ptr and we 'have' a window (i.e. plugin)
	// we can't take another one's window.
	if (window == NULL && haswindow())
		return False;

	// IM-2016-08-16: [[ Bug 18153 ]] send OnDetach to closing stack
	sptr->OnDetach();
	
	// MW-2008-10-31: [[ ParentScripts ]] Send closeControl messages appropriately
	if (sptr -> curcard -> closecontrols() == ES_ERROR ||
		sptr->curcard->message(MCM_close_card) == ES_ERROR ||
		//sptr->curcard->groupmessage(MCM_close_background, NULL) == ES_ERROR
		sptr -> curcard -> closebackgrounds(NULL) == ES_ERROR ||
		sptr->curcard->message(MCM_close_stack) == ES_ERROR)
		return False;
	if (window != NULL)
	{
		stop_externals();
		destroywindow();
		cursor = None;
		MCValueAssign(titlestring, kMCEmptyString);
	}
	MCValueAssign(sptr -> titlestring, kMCEmptyString);
	window = sptr->window;
	iconid = sptr->iconid;
	sptr->stop_externals();
	sptr->window = NULL;
	
	// IM-2014-07-23: [[ Bug 12930 ]] Update the child stacks of sptr to be child stacks of this.
	MCdispatcher->foreachchildstack(sptr, _MCStackTakeWindowCallback, this);
	
	state = sptr->state;
	state &= ~(CS_IGNORE_CLOSE | CS_NO_FOCUS | CS_DELETE_STACK);

	// MW-2012-04-20: [[ Bug 10185 ]] Make sure we take the visibility flag of the
	//   target stack (else if we go in window with an invisible stack redraw issue
	//   happens).
	flags &= ~(F_TAKE_FLAGS | F_VISIBLE);
	flags |= sptr->flags & (F_TAKE_FLAGS | F_VISIBLE);
	decorations = sptr->decorations;
    rect = sptr->rect;
    
    // sptr has not been closed so may still have a scroll while this stack has been closed
    // so has had clearscroll called on it so scroll will be 0. applyscroll is called later.
    rect.height += getnextscroll(true);
    m_scroll = 0;
    
	minwidth = sptr->minwidth;
	minheight = sptr->minheight;
	maxwidth = sptr->maxwidth;
	maxheight = sptr->maxheight;
	mode = sptr->mode;
	
	// MW-2005-07-18: If we have taken the other stack's window we must set its
	//   mode to WM_CLOSED, lest we try to do other things with it later...
	sptr -> mode = WM_CLOSED;

	// MW-2011-01-10: [[ Effects ]] Take the snapshot.
	takewindowsnapshot(sptr);
	
    // MW-2014-03-14: [[ Bug 11915 ]] Make sure we copy the view (fullscreenmode related)
    //   props to this stack.
    view_copy(*sptr);
    
	mode_takewindow(sptr);
	
	if (MCmousestackptr.IsBoundTo(sptr))
		MCmousestackptr = this;
	start_externals();
    
#ifdef _MOBILE
    // MW-2014-03-14: [[ Bug 11813 ]] Make sure we tell MCScreenDC that the top window
    //   has changed, and mark our stack as its own window.
	// IM-2014-09-23: [[ Bug 13349 ]] Reset mobile window back to this stack, then call openwindow()
	//   to perform any shared window initialisation.
    window = (Window)this;
	openwindow(False);
#endif
    
	// IM-2016-08-16: [[ Bug 18153 ]] send OnAttach to this stack
	OnAttach();

	return True;
}

Boolean MCStack::setwindow(Window w)
{
	MCRectangle t_rect;
	if (!MCscreen->getwindowgeometry(window, t_rect))
		return False;
	rect = t_rect;
	window = w;
	state |= CS_FOREIGN_WINDOW;
	return True;
}

bool MCStack::createwindow()
{
	if (window != nil)
		return true;
	
	realize();
	if (window == nil)
		return false;
	
	OnAttach();
	
	return true;
}

void MCStack::destroywindow()
{
	if (window == nil)
		return;
	
	OnDetach();
	
	MCscreen->destroywindow(window);
}

void MCStack::kfocusset(MCControl *target)
{
	if (!opened)
		return;
	if (MCactivefield && target != NULL && MCactivefield != target)
	{
		MCactivefield->unselect(False, True);
		if (MCactivefield)
        {
			if (MCactivefield->getstack() == this)
				curcard->kunfocus();
			else
            {
				if (MCactivefield->getstack()->state & CS_KFOCUSED)
					MCactivefield->getstack()->kunfocus();
				else
					MCactivefield->unselect(True, True);
            }

        }
	}
	if (MCactivefield && target != NULL)
	{
		curcard->kfocus();
		MCstacks -> ensureinputfocus(window);
		return;
	}
	if (opened && flags & F_VISIBLE
	        && !(state & (CS_NO_FOCUS | CS_KFOCUSED | CS_LOCK_SCREEN
	                      | CS_ICONIC | CS_IGNORE_CLOSE)))
	{
		mode_takefocus();
		MCscreen->waitfocus();
	}

	if (target != NULL)
		curcard->kfocusset(target);	
	MCstacks -> ensureinputfocus(window);
}

MCStack *MCStack::clone()
{
	if (editing != NULL)
		stopedit();
	MCStack *newsptr = nil;
	/* UNCHECKED */ MCStackSecurityCopyStack(this, newsptr);
	MCdispatcher->appendstack(newsptr);
	newsptr->parent = MCdispatcher->gethome();
	if (this != MCtemplatestack || (rect.x == 0 && rect.y == 0))
	{
		newsptr->positionrel(MCdefaultstackptr->rect, OP_CENTER, OP_MIDDLE);
		newsptr->rect.x += MCcloneoffset;
		newsptr->rect.y += MCcloneoffset;
	}
	newsptr->message(MCM_new_stack);
	return newsptr;
}

void MCStack::compact()
{
	if (editing != NULL)
		stopedit();
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
	if (substacks != NULL)
	{
		MCStack *sptr = substacks;
		do
		{
			sptr->compact();
			sptr = sptr->next();
		}
		while (sptr != substacks);
	}
}

Boolean MCStack::checkid(uint4 cardid, uint4 controlid)
{
	if (cards != NULL)
	{
		MCCard *cptr = cards;
		do
		{
			if (cptr->getid() == cardid)
				return cptr->checkid(controlid);
			cptr = cptr->next();
		}
		while (cptr != cards);
	}
	if (curcard != NULL && curcard->getid() == cardid)
		return curcard->checkid(controlid);
	return False;
}

IO_stat MCStack::saveas(const MCStringRef p_fname, uint32_t p_version)
{
	Exec_stat stat = curcard->message(MCM_save_stack_request);
	if (stat == ES_NOT_HANDLED || stat == ES_PASS)
	{
		MCStack *sptr = this;
		if (!MCdispatcher->ismainstack(sptr))
			sptr = sptr->parent.GetAs<MCStack>();
		return MCdispatcher->savestack(sptr, p_fname, p_version);
	}
	return IO_NORMAL;
}

MCStack *MCStack::findname(Chunk_term type, MCNameRef p_name)
{ // should do case-sensitive match on filename on UNIX...
	if (type == CT_STACK)
	{
		if (MCU_matchname(p_name, CT_STACK, getname()))
			return this;
		
		if (!MCStringIsEmpty(filename))
		{
			MCNewAutoNameRef t_filename_name;
			/* UNCHECKED */ MCNameCreate(filename, &t_filename_name);
			if (MCU_matchname(p_name, CT_STACK, *t_filename_name))
				return this;
		}
	}

	return NULL;
}

MCStack *MCStack::findid(Chunk_term type, uint4 inid, Boolean alt)
{
	if (type == CT_STACK && (inid == obj_id || (alt && inid == altid)))
		return this;
	else
		return NULL;
}

void MCStack::setmark()
{
	state |= CS_MARKED;
}

void MCStack::clearmark()
{
	state &= ~CS_MARKED;
}

void MCStack::setbackground(MCControl *bptr)
{
	backgroundid = bptr->getid();
}

void MCStack::clearbackground()
{
	backgroundid = 0;
}

void MCStack::ungroup(MCGroup *source)
{
	MCselected->clear(True);
	MCControl *clist = source->getcontrols();
	if (!curcard->removecontrol(source, False, True))
		return;
	source->setcontrols(NULL);
	if (clist != NULL)
	{
		MCControl *tptr = clist;
		do
		{
			// MW-2013-06-21: [[ Bug 10976 ]] Make sure we uncache the object from the id
			//   cache, otherwise its id will get bumped if it has been accessed via
			//   id before (due to the subsequent check!).
			uncacheobjectbyid(tptr);
			if (getcontrolid(tptr->gettype(), tptr->getid()))
			{
				fprintf(stderr, "ERROR: found duplicate id %d\n", tptr->getid());
				tptr->setid(newid());
			}
			curcard->newcontrol(tptr, False);
			MCselected->add(tptr);
			tptr = tptr->next();
		}
		while (tptr != clist);
		MCControl *cptr = controls;
		while (cptr != source)
			cptr = cptr->next();
		clist->appendto(cptr);
	}
	MCControl *cptr = source;
	cptr->remove(controls);
	source->insertto(MCsavegroupptr);
	source->setparent(this);

	// MW-2011-08-17: [[ Redraw ]] Tell the stack to dirty all of itself.
	dirtyall();
}

#define ECS "This is not the card script, and all changes to this script\n\
will be discarded when you stop editing this background."

void MCStack::startedit(MCGroup *group)
{
	if (!opened)
		return;
	if (editing != NULL)
		stopedit();
	else
	{
		MCselected->clear(True);
		kunfocus();
	}
	curcard->close();
	MCscreen->cancelmessageobject(curcard, NULL);
	editing = group;

	// Save the current list of object references and cards
	savecontrols = controls;
	savecard = curcard;
	savecards = cards;

    // MM-2013-02-21: [[ Bug 10620 ]] Uncache the card (of the group) we are editing.
    //   If we don't, any controls pasted whil in edit group mode will be pasted onto
    //   the old card rather than the newly created card we are editing.
	uncacheobjectbyid(savecard);
	
	// Get the object references from the group
	controls = editing->getcontrols();

	// Temporarily clear the group's object list
	editing->setcontrols(NULL);

	// Create a temporary card
	cards = curcard = new (nothrow) MCCard;

	// Link the card to the parent, give it the same id as the current card and give it a temporary script
	curcard->setparent(this);
	curcard->setid(savecard->getid());
	curcard->setsprop(P_SCRIPT, MCSTR(ECS));

	// Now add references for each control in the group being edited to the card
	if (controls != NULL)
	{
		MCControl *cptr = controls;
		do
		{
			curcard->newcontrol(cptr, False);
			cptr = cptr->next();
		}
		while (cptr != controls);
	}

	curcard->open();
	updatecardsize();
	// MW-2011-08-17: [[ Redraw ]] Tell the stack to dirty all of itself.
	dirtyall();
	dirtywindowname();
}

void MCStack::stopedit()
{
	if (editing == NULL)
		return;
	MCselected->clear(True);
	curcard->close();
	MCObjptr *clist = curcard->getrefs();
	MCControl *oldcontrols = controls;
	controls = NULL;
	while (oldcontrols != NULL)
	{
		clist->getref()->remove(oldcontrols);
		clist->getref()->appendto(controls);
		clist = clist->next();
	}
	
	// MW-2007-05-07: [[ Bug 4866 ]] It seems computeminrect can cause the parents of controls to be reset
	//   Swapping these two calls around fixes this crash - although I'm not 100% happy with this.
	//
	editing->computeminrect(False);
	editing->setcontrols(controls);
	controls = savecontrols;
	cards = savecards;
	MCObject *oldcard = curcard;
	curcard = savecard;
	MCGroup *oldediting = editing;
	editing = NULL;
	
    // MM-2013-02-21: [[ Bug 10620 ]] Remove the card created for the edit from the cache.
	uncacheobjectbyid(oldcard);
	
	curcard->open();
	updatecardsize();
	// MW-2011-08-17: [[ Redraw ]] Tell the stack to dirty all of itself.
	dirtyall();
    oldcard->removereferences();
	oldcard->scheduledelete();
	kfocus();
	dirtywindowname();
	if (gettool(this) == T_POINTER)
		MCselected->add(oldediting);
}

void MCStack::updatemenubar()
{
	if (opened && state & CS_KFOCUSED && !MClockmenus)
	{
        if (!hasmenubar() || (state & CS_EDIT_MENUS
            && mode < WM_PULLDOWN && mode != WM_PALETTE)
            || (gettool(this) != T_BROWSE && MCdefaultmenubar))
        {
			MCmenubar = nil;
        }
        
		else
        {
			MCmenubar = MCObjectCast<MCGroup>(getobjname(CT_GROUP, (getmenubar())));
        }
		MCscreen->updatemenubar(False);
	}
}

// MW-2011-09-12: [[ MacScroll ]] Compute the scroll as it should be now taking
//   into account the menubar and such.
int32_t MCStack::getnextscroll(bool p_ignore_opened)
{
#ifdef _MACOSX
	MCControl *mbptr;
	if (!(state & CS_EDIT_MENUS) && hasmenubar()
	        && (mbptr = curcard->getchild(CT_EXPRESSION, MCNameGetString(getmenubar()), CT_GROUP, CT_UNDEFINED)) != NULL
	        && (p_ignore_opened || mbptr->getopened()) && mbptr->isvisible())
	{
		MCRectangle r = mbptr->getrect();
		return (r.y + r.height);
	}
#endif
	return 0;
}

// MW-2011-09-12: [[ MacScroll ]] Return the scroll of the stack as currently
//   applied.
int32_t MCStack::getscroll(void) const
{
	return m_scroll;
}
 
void MCStack::scrollintoview()
{
	MCControl *cptr = curcard->getkfocused();
	if (cptr != NULL)
	{
		MCRectangle r = cptr->getrect();
		if (menuy + menuheight > rect.height
		        && r.y + r.height > rect.height - MENU_ARROW_SIZE)
			scrollmenu(MCU_max(rect.height - menuy - menuheight,
			                   rect.height - MENU_ARROW_SIZE - r.y - r.height), True);
		if (menuy < 0 && r.y < MENU_ARROW_SIZE)
			scrollmenu(MCU_min(-menuy, MENU_ARROW_SIZE - r.y), True);
	}
}

void MCStack::scrollmenu(int2 offset, Boolean draw)
{
	MCRectangle crect;
	MCControl *cptr = controls;
	do
	{
		crect = cptr->getrect();
		crect.y += offset;
		cptr->setrect(crect);
		cptr = cptr->next();
	}
	while (cptr != controls);
	menuy += offset;

	// MW-2011-08-17: [[ Redraw ]] Tell the stack to dirty all of itself.
	if (draw)
		dirtyall();
}

void MCStack::clipmenu(MCContext *context, MCRectangle &crect)
{
	if (!IsNativeWin() && context != NULL)
		{
			draw3d(context, curcard->getrect(), ETCH_RAISED, borderwidth);
			crect = MCU_clip_rect(crect, DEFAULT_BORDER, DEFAULT_BORDER,
								  rect.width - DEFAULT_BORDER,
								  rect.height - DEFAULT_BORDER);
		}

	MCPoint p[3];
	if (menuy < 0 && crect.y < MENU_ARROW_SIZE)
	{
		if (crect.height < MENU_ARROW_SIZE - crect.y)
			crect.height = 0;
		else
		{
			crect.height -= MENU_ARROW_SIZE - crect.y;
			crect.y = MENU_ARROW_SIZE;
			if (context != NULL)
			{
				// Blank out the entry bit
				MCRectangle dirty;
					MCU_set_rect(dirty, 0, 0, rect.width, MENU_ARROW_SIZE);
				if (MCcurtheme == nil ||
					MCcurtheme -> drawmenubackground(context, dirty, curcard -> getrect(), false))
				{
					// TS-2008-01-21 : [[Bug 5566 - Handler menu not standard and blocking other menus ]]

					//if ( MClook == LF_WIN95 && MCcurtheme != NULL )
					if ( IsNativeWin() )
						MCU_set_rect(dirty, DEFAULT_BORDER - 1, DEFAULT_BORDER - 1,
									 rect.width - DEFAULT_BORDER,
									 MENU_ARROW_SIZE - DEFAULT_BORDER); 
					else
						MCU_set_rect(dirty, DEFAULT_BORDER , DEFAULT_BORDER ,
									 rect.width - ( DEFAULT_BORDER<<1),
									 MENU_ARROW_SIZE - DEFAULT_BORDER); 

		
					setforeground(context, DI_BACK, False, False);
					context->fillrect(dirty);
				}
				
				// Draw the arrow
				context->setforeground(MCscreen->getblack());
				p[0].x = rect.width >> 1;
				p[1].x = p[0].x + 5;
				p[2].x = p[0].x - 5;
				p[0].y = 4;
				p[1].y = p[2].y = 9;

				context->fillpolygon(p, 3);
			}
		}
	}
	if (menuy + menuheight > rect.height
	        && crect.y + crect.height > rect.height - MENU_ARROW_SIZE)
	{
		if (rect.height - MENU_ARROW_SIZE < crect.y)
			crect.height = 0;
		else
		{
			crect.height = rect.height - MENU_ARROW_SIZE - crect.y;
			if (context != NULL)
			{
				MCRectangle dirty;
					MCU_set_rect(dirty, 0, rect.height - MENU_ARROW_SIZE, rect.width, MENU_ARROW_SIZE);
				if (MCcurtheme == nil ||
					!MCcurtheme -> drawmenubackground(context, dirty, curcard -> getrect(), false))
				{
					// TS-2008-01-21 : [[Bug 5566 - Handler menu not standard and blocking other menus ]]
					if (IsNativeWin() )
						MCU_set_rect(dirty, DEFAULT_BORDER-1, (rect.height - MENU_ARROW_SIZE) + 1,
									 rect.width - DEFAULT_BORDER ,
									 ( MENU_ARROW_SIZE - DEFAULT_BORDER));
					else
						MCU_set_rect(dirty, DEFAULT_BORDER, rect.height - MENU_ARROW_SIZE,
									 rect.width - ( DEFAULT_BORDER << 1),
									 ( MENU_ARROW_SIZE - DEFAULT_BORDER));

					setforeground(context, DI_BACK, False, False);
					context->fillrect(dirty);
				}
				
				context->setforeground(MCscreen->getblack());
				p[0].x = rect.width >> 1;
				p[1].x = p[0].x + 5;
				p[2].x = p[0].x - 5;
				p[0].y = rect.height - 4;
				p[1].y = p[2].y = rect.height - 9;
				context->fillpolygon(p, 3);
			}
		}
	}
}

Boolean MCStack::count(Chunk_term otype, Chunk_term ptype, MCObject *stop, uint2 &num)
{
	num = 0;
	switch (otype)
	{
	case CT_AUDIO_CLIP:
		if (aclips != NULL)
		{
			MCAudioClip *aptr = aclips;
			do
			{
				num++;
				aptr = aptr->next();
			}
			while (aptr != aclips);
		}
		break;
	case CT_VIDEO_CLIP:
		if (vclips != NULL)
		{
			MCVideoClip *vptr = vclips;
			do
			{
				num++;
				vptr = vptr->next();
			}
			while (vptr != vclips);
		}
		break;
	case CT_CARD:
		if (cards != NULL)
		{
			MCCard *cptr = cards;
			do
			{
				if (cptr->countme(backgroundid, (state & CS_MARKED) != 0))
					num++;
				if (cptr == stop)
					break;
				cptr = cptr->next();
			}
			while (cptr != cards);
		}
		break;
	case CT_STACK:
		if (substacks != NULL)
		{
			MCStack *sptr = substacks;
			do
			{
				num++;
				if (sptr == stop)
					break;
				sptr = sptr->next();
			}
			while (sptr != substacks);
		}
		break;
	default:
		if (controls != NULL)
		{
			MCControl *cptr = controls;
			if (otype == CT_BACKGROUND)
				do
				{
					if (cptr->gettype() == CT_GROUP)
						num++;
					if (cptr == stop)
						break;
					cptr = cptr->next();
				}
				while (cptr != controls);
			else
				curcard->count(otype, ptype, stop, num, True);
		}
		break;
	}
	return True;
}

void MCStack::renumber(MCCard *card, uint4 newnumber)
{
	card->remove
	(cards);
	if (cards == NULL || newnumber-- <= 1)
		card->insertto(cards);
	else
	{
		MCCard *cptr = cards;
		while (--newnumber)
		{
			cptr = cptr->next();
			if (cptr->next() == cards)
				break;
		}
		cptr->append(card);
	}
	dirtywindowname();
}

MCObject *MCStack::getAV(Chunk_term etype, MCStringRef s, Chunk_term otype)
{
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
		count(otype, CT_UNDEFINED, NULL, num);
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
		uint4 inid;
		if (MCU_stoui4(s, inid))
			return getAVid(otype, inid);
		return NULL;
	case CT_EXPRESSION:
		if (!MCU_stoui2(s, num))
		{
			MCNewAutoNameRef t_name;
			/* UNCHECKED */ MCNameCreate(s, &t_name);
			MCObject* t_object;
			if (getAVname(otype, *t_name, t_object))
				return t_object;
			else
				return NULL;
		}
		if (num < 1)
			return NULL;
		num--;
		break;
	default:
		return NULL;
	}
	MCObject *objs;
	if (otype == CT_AUDIO_CLIP)
		objs = aclips;
	else
		objs = vclips;

	if (objs == NULL)
		return NULL;
	MCObject *tobj = objs;
	while (num--)
	{
		tobj = tobj->next();
		if (tobj == objs)
			return NULL;
	}
	return tobj;
}

MCCard *MCStack::getchild(Chunk_term etype, MCStringRef p_expression, Chunk_term otype)
{
	if (otype != CT_CARD)
		return NULL;

	uint2 num = 0;

	if (cards == NULL)
	{
		curcard = cards = MCtemplatecard->clone(False, False);
		cards->setparent(this);
	}

	// OK-2007-04-09 : Allow cards to be found by ID when in edit group mode.
	MCCard *cptr;
	if (editing != NULL && savecards != NULL)
		cptr = savecards;
	else
		cptr = cards;

	MCCard *found = NULL;
	if (etype == CT_EXPRESSION && MCStringIsEqualToCString(p_expression, "window", kMCCompareCaseless))
		etype = CT_THIS;
	switch (etype)
	{
	case CT_THIS:
		if (curcard != NULL)
			return curcard;
		return cards;
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
	case CT_NEXT:
		cptr = curcard;
		do
		{
			cptr = cptr->next();
			if (cptr->countme(backgroundid, (state & CS_MARKED) != 0))
				return cptr;
		}
		while (cptr != curcard);
		return NULL;
	case CT_PREV:
		cptr = curcard;
		do
		{
			cptr = cptr->prev();
			if (cptr->countme(backgroundid, (state & CS_MARKED) != 0))
				return cptr;
		}
		while (cptr != curcard);
		return NULL;
	case CT_LAST:
	case CT_MIDDLE:
	case CT_ANY:
		count(otype, CT_UNDEFINED, NULL, num);
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
		uint4 inid;
		if (MCU_stoui4(p_expression, inid))
		{
			// OK-2008-06-27: <Bug where looking up a card by id when in edit group mode could cause an infinite loop>
			MCCard *t_cards;
			if (editing != NULL && savecards != NULL)
				t_cards = savecards;
			else
				t_cards = cards;
		
			// OK-2007-04-09 : Allow cards to be found by ID when in edit group mode.
			if (editing == NULL)
				found = curcard -> findid(CT_CARD, inid, True);
			else
				found = NULL;

			if (found == NULL)
				do
				{
					found = cptr->findid(CT_CARD, inid, True);
					if (found != NULL
							&& found->countme(backgroundid, (state & CS_MARKED) != 0))
						break;
					cptr = cptr->next();
				}
				while (cptr != t_cards);
		}
		return found;
	case CT_EXPRESSION:
		if (MCU_stoui2(p_expression, num))
		{
			if (num < 1)
				return NULL;
			num--;
			break;
		}
		else
		{
			do
			{
                MCNewAutoNameRef t_expression;
                /* UNCHECKED */ MCNameCreate(p_expression, &t_expression);
				found = cptr->findname(otype, *t_expression);
				if (found != NULL
				        && found->countme(backgroundid, (state & CS_MARKED) != 0))
					break;
				cptr = cptr->next();
			}
			while (cptr != cards);
		}
		return found;
	default:
		return NULL;
	}
	do
	{
		if (cptr->countme(backgroundid, (state & CS_MARKED) != 0) && num-- == 0)
			return cptr;
		cptr = cptr->next();
	}
	while (cptr != cards);
	return NULL;
}

MCCard *MCStack::getchildbyordinal(Chunk_term p_ordinal)
{
	uint2 num = 0;
    
	if (cards == NULL)
	{
		curcard = cards = MCtemplatecard->clone(False, False);
		cards->setparent(this);
	}

    MCCard *cptr;
    
	switch (p_ordinal)
	{
        case CT_THIS:
            if (curcard != NULL)
                return curcard;
            return cards;
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
            num = p_ordinal - CT_FIRST;
            break;
        case CT_NEXT:
            cptr = curcard;
            do
            {
                cptr = cptr->next();
                if (cptr->countme(backgroundid, (state & CS_MARKED) != 0))
                    return cptr;
            }
            while (cptr != curcard);
            return NULL;
        case CT_PREV:
            cptr = curcard;
            do
            {
                cptr = cptr->prev();
                if (cptr->countme(backgroundid, (state & CS_MARKED) != 0))
                    return cptr;
            }
            while (cptr != curcard);
            return NULL;
        case CT_LAST:
        case CT_MIDDLE:
        case CT_ANY:
            count(CT_CARD, CT_UNDEFINED, NULL, num);
            switch (p_ordinal)
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
            break;
    }
    return NULL;
}

MCCard *MCStack::getchildbyid(uinteger_t p_id)
{
    if (cards == NULL)
	{
		curcard = cards = MCtemplatecard->clone(False, False);
		cards->setparent(this);
	}
    
    // OK-2007-04-09 : Allow cards to be found by ID when in edit group mode.
    MCCard *cptr;
    if (editing != nil && savecards != nil)
        cptr = savecards;
    else
        cptr = cards;
    
    MCCard *found = nil;

    // OK-2008-06-27: <Bug where looking up a card by id when in edit group mode could cause an infinite loop>
    MCCard *t_cards = cptr;
    
    // OK-2007-04-09 : Allow cards to be found by ID when in edit group mode.
    if (editing == nil)
        found = curcard -> findid(CT_CARD, p_id, True);
    
    if (found == nil)
    {
        do
        {
            found = cptr->findid(CT_CARD, p_id, True);
            if (found != nil
                && found->countme(backgroundid, (state & CS_MARKED) != 0))
                break;
            cptr = cptr->next();
        }
        while (cptr != t_cards);
    }
    
    return found;
}

MCCard *MCStack::getchildbyname(MCNameRef p_name)
{
    if (cards == NULL)
	{
		curcard = cards = MCtemplatecard->clone(False, False);
		cards->setparent(this);
	}
    
    MCCard *cptr, *cptr_sentinal;
    if (editing != NULL && savecards != NULL)
    {
        cptr_sentinal = cptr = savecards;
    }
    else
    {
        cptr_sentinal = cptr = cards;
    }
    
    uint2 t_num = 0;
    if (MCU_stoui2(MCNameGetString(p_name), t_num))
    {
        if (t_num < 1)
            return nil;
        t_num--;
        
        do
        {
            if (cptr->countme(backgroundid, (state & CS_MARKED) != 0) && t_num-- == 0)
                return cptr;
            cptr = cptr->next();
        }
        while (cptr != cptr_sentinal);
        return nil;
    }
    MCCard *found = nil;
    do
    {
        found = cptr->findname(CT_CARD, p_name);
        if (found != nil && found->countme(backgroundid, (state & CS_MARKED) != 0))
            break;
        
        cptr = cptr->next();
    }
    while (cptr != cptr_sentinal);
    
    return found;
}

MCGroup *MCStack::getbackground(Chunk_term etype, MCStringRef p_string,
                                Chunk_term otype)
{
	if (otype != CT_GROUP)
		return NULL;

	MCControl *cptr;
	if (editing != NULL)
		cptr = savecontrols;
	else
		cptr = controls;
	MCControl *startcptr = cptr;
	if (cptr == NULL)
		return NULL;

	uint2 num = 0;
	switch (etype)
	{
	case CT_THIS:
		if  (editing != 0)
			return editing;
		return (MCGroup *)curcard->getchild(CT_FIRST, kMCEmptyString, otype, CT_BACKGROUND);
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
	case CT_NEXT:
		{
			MCGroup *gptr = (MCGroup *)curcard->getchild(CT_FIRST, kMCEmptyString, otype, CT_BACKGROUND);
			while (True)
			{
				gptr = gptr->next();
				if (gptr->gettype() == CT_GROUP)
					return gptr;
			}
		}
		break;
	case CT_PREV:
		{
			MCGroup *gptr = (MCGroup *)curcard->getchild(CT_FIRST, kMCEmptyString, otype, CT_BACKGROUND);
			while (True)
			{
				gptr = gptr->prev();
				if (gptr->gettype() == CT_GROUP)
					return gptr;
			};
		}
		break;
	case CT_LAST:
	case CT_MIDDLE:
	case CT_ANY:
		count(otype, CT_UNDEFINED, NULL, num);
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
		uint4 inid;
		if (MCU_stoui4(p_string, inid))
			do
			{
				MCControl *found = cptr->findid(otype, inid, True);
				if (found != NULL)
					return (MCGroup *)found;
				cptr = cptr->next();
			}
			while (cptr != startcptr);
		return NULL;
	case CT_EXPRESSION:
		if (MCU_stoui2(p_string, num))
		{
			if (num < 1)
				return NULL;
			num--;
			break;
		}
		else
		{
			do
			{
				MCNewAutoNameRef t_name;
				/* UNCHECKED */ MCNameCreate(p_string, &t_name);
				MCControl *found = cptr->findname(otype, *t_name);
				if (found != NULL)
					return (MCGroup *)found;
				cptr = cptr->next();
			}
			while (cptr != startcptr);

			return NULL;
		}
	default:
		return NULL;
	}
	do
	{
		MCControl *foundobj = cptr->findnum(otype, num);
		if (foundobj != NULL)
			return (MCGroup *)foundobj;
		cptr = cptr->next();
	}
	while (cptr != startcptr);
	return NULL;
}

MCGroup *MCStack::getbackgroundbyordinal(Chunk_term p_ordinal)
{    
	uint2 num = 0;
	switch (p_ordinal)
	{
        case CT_THIS:
            if  (editing != 0)
                return editing;
            return (MCGroup *)curcard->getchild(CT_FIRST, kMCEmptyString, CT_GROUP, CT_BACKGROUND);
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
            num = p_ordinal - CT_FIRST;
            break;
        case CT_NEXT:
		{
			MCGroup *gptr = (MCGroup *)curcard->getchild(CT_FIRST, kMCEmptyString, CT_GROUP, CT_BACKGROUND);
			while (True)
			{
				gptr = gptr->next();
				if (gptr->gettype() == CT_GROUP)
					return gptr;
			}
		}
            break;
        case CT_PREV:
		{
			MCGroup *gptr = (MCGroup *)curcard->getchild(CT_FIRST, kMCEmptyString, CT_GROUP, CT_BACKGROUND);
			while (True)
			{
				gptr = gptr->prev();
				if (gptr->gettype() == CT_GROUP)
					return gptr;
			};
		}
            break;
        case CT_LAST:
        case CT_MIDDLE:
        case CT_ANY:
            count(CT_GROUP, CT_UNDEFINED, NULL, num);
            switch (p_ordinal)
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
            break;
    }
    return NULL;
}

MCGroup *MCStack::getbackgroundbyid(uinteger_t p_id)
{
	MCControl *cptr;
	if (editing != NULL)
		cptr = savecontrols;
	else
		cptr = controls;
	MCControl *startcptr = cptr;
	if (cptr == NULL)
		return NULL;
    do
    {
        MCControl *found = cptr->findid(CT_GROUP, p_id, True);
        if (found != NULL)
            return (MCGroup *)found;
        cptr = cptr->next();
    }
    while (cptr != startcptr);
    return NULL;
}

MCGroup *MCStack::getbackgroundbyname(MCNameRef p_name)
{
	MCControl *cptr;
	if (editing != nil)
		cptr = savecontrols;
	else
		cptr = controls;
	MCControl *startcptr = cptr;
	if (cptr == nil)
		return nil;
    
    uint2 t_num = 0;
    if (MCU_stoui2(MCNameGetString(p_name), t_num))
    {
        if (t_num < 1)
            return nil;
        t_num--;
        
        do
        {
            MCControl *foundobj = cptr->findnum(CT_GROUP, t_num);
            if (foundobj != nil)
                return (MCGroup *)foundobj;
            cptr = cptr->next();
        }
        while (cptr != startcptr);
        return nil;
    }
    
    do
    {
        MCControl *found = cptr->findname(CT_GROUP, p_name);
        if (found != nil)
            return (MCGroup *)found;
        cptr = cptr->next();
    }
    while (cptr != startcptr);
    return nil;
}

void MCStack::addmnemonic(MCButton *button, KeySym p_key)
{
	MCU_realloc((char **)&mnemonics, nmnemonics,
	            nmnemonics + 1, sizeof(Mnemonic));
	mnemonics[nmnemonics].button = button;
	
	// Ensure that letter mnemonics are added case-insensitively
	// (the shift state is handled by the button)
	KeySym t_key;
	t_key = MCKeySymToLower(p_key);
	mnemonics[nmnemonics].key = t_key;
	nmnemonics++;
}

void MCStack::deletemnemonic(MCButton *button)
{
	uint2 i;
	for (i = 0 ; i < nmnemonics ; i++)
		if (mnemonics[i].button == button)
		{
			while (++i < nmnemonics)
				mnemonics[i - 1] = mnemonics[i];
			nmnemonics--;
			break;
		}
}

void MCStack::addneed(MCButton *bptr)
{
	MCU_realloc((char **)&needs, nneeds, nneeds + 1, sizeof(MCButton *));
	needs[nneeds++] = bptr;
}

void MCStack::removeneed(MCButton *bptr)
{
	uint2 i;
	for (i = 0 ; i < nneeds ; i++)
		if (needs[i] == bptr)
		{
			while (++i < nneeds)
				needs[i - 1] = needs[i];
			nneeds--;
			break;
		}
}

MCButton *MCStack::findmnemonic(KeySym p_key)
{
	uint2 i;
	KeySym t_key;
	t_key = MCKeySymToLower(p_key);
	for (i = 0 ; i < nmnemonics ; i++)
		if (mnemonics[i].key == t_key)
			return mnemonics[i].button;
	return NULL;
}

void MCStack::installaccels(MCStack *stack)
{
	curcard->installaccels(stack);

}

void MCStack::removeaccels(MCStack *stack)
{
	curcard->removeaccels(stack);
}

void MCStack::setwindowname()
{
	if (!opened || isunnamed() || window == NULL)
		return;

	char *t_utf8_name;
	t_utf8_name = NULL;

	MCStringRef tptr;
	if (MCStringIsEmpty(title))
		tptr = MCNameGetString(getname());
	else
		tptr = title;

	MCAutoStringRef newname;
	if (editing != NULL)
	{
		MCAutoValueRef bgname;
		/* UNCHECKED */ editing->names(P_SHORT_NAME, &bgname);
		/* UNCHECKED */ MCStringFormat(&newname, "%@ (%s \"%@\")", tptr, MCbackgroundstring, *bgname);
	}
	else
	{
		if (MCStringIsEmpty(title) && mode == WM_TOP_LEVEL && MCdispatcher->cut(True))
		{
			if ((cards->next()) == cards)
				/* UNCHECKED */ MCStringFormat(&newname, "%@ *", tptr);
			else
			{
				uint2 num;
				count(CT_CARD, CT_UNDEFINED, curcard, num);
				/* UNCHECKED */ MCStringFormat(&newname, "%@ (%d) *", tptr, num);
			}
		}
		else
			if (MCStringIsEmpty(title) && mode == WM_TOP_LEVEL_LOCKED
			        && cards->next() != cards)
			{
				uint2 num;
				count(CT_CARD, CT_UNDEFINED, curcard, num);
				/* UNCHECKED */ MCStringFormat(&newname, "%@ (%d)", tptr, num);
			}
			else
				newname = tptr;
	}
	if (!MCStringIsEqualTo(*newname, titlestring, kMCStringOptionCompareExact))
	{
		MCValueAssign(titlestring, *newname);
		MCscreen->setname(window, titlestring);
	}
	
	state &= ~CS_TITLE_CHANGED;
}

void MCStack::reopenwindow()
{
	// PM-2016-02-09: [[ Bug 16889 ]] Exit if window is NULL
	if (state & CS_FOREIGN_WINDOW || window == NULL)
		return;

	stop_externals();

	// MW-2006-05-23: Only close and open the window if it is visible
	if (getflag(F_VISIBLE))
		MCscreen->closewindow(window);

	destroywindow();
	MCValueAssign(titlestring, kMCEmptyString);
	if (getstyleint(flags) != 0)
		mode = (Window_mode)(getstyleint(flags) + WM_TOP_LEVEL_LOCKED);

	// IM-2014-05-27: [[ Bug 12321 ]] Updating the view transform here after changing the fullscreen
	// allows the stack rect to be correctly restored in sync with the window reopening
	view_update_transform();

	// IM-2014-05-27: [[ Bug 12321 ]] Move font purging here to avoid second redraw after fullscreen change
	if (m_purge_fonts)
	{
		purgefonts();
		m_purge_fonts = false;
	}

	// MW-2011-08-18: [[ Redraw ]] Use global screen lock
	MCRedrawLockScreen();
	createwindow();
	sethints();
	
	// IM-2014-01-16: [[ StackScale ]] Call configure to update the stack rect after fullscreen change
	configure(True);
	
	MCRedrawUnlockScreen();

	dirtywindowname();

	if (getflag(F_VISIBLE))
		openwindow(mode >= WM_PULLDOWN);
}

Exec_stat MCStack::openrect(const MCRectangle &rel, Window_mode wm, MCStack *parentptr, Window_position wpos,  Object_pos walign)
{
	if (state & (CS_IGNORE_CLOSE | CS_NO_FOCUS | CS_DELETE_STACK))
		return ES_NORMAL;
    
    MCtodestroy -> remove(this);
	if (wm == WM_LAST)
    {
		if (opened)
			wm = mode;
		else
        {
			if (getstyleint(flags) == 0)
				wm = WM_TOP_LEVEL;
			else
				wm = (Window_mode)(getstyleint(flags) + WM_TOP_LEVEL_LOCKED);
        }
    }
	if (wm == WM_TOP_LEVEL
	        && (flags & F_CANT_MODIFY || m_is_ide_stack || !MCdispatcher->cut(True)))
		wm = WM_TOP_LEVEL_LOCKED;

	Boolean oldlock = MClockmessages;
	Boolean reopening = False;
	
	// MW-2005-07-18: If this stack has substacks that have been opened, then it is marked as open even though
	//   it might not be.
	if (opened)
	{
		if (window == NULL && !MCModeMakeLocalWindows() && ((wm != WM_MODAL && wm != WM_SHEET) || wm == mode))
			return ES_NORMAL;

		if (wm == mode && parentptr == NULL)
		{
			bool t_topped;
			t_topped = false;

			// MW-2011-08-18: [[ Redraw ]] Update to use redraw.
			if (state & CS_ICONIC)
			{
				MCscreen->uniconifywindow(window);
				state &= ~CS_ICONIC;
			}
			else if (!(state & CS_LOCK_SCREEN) && !MCRedrawIsScreenLocked())
			{
				// MW-2008-03-27: [[ Bug 3880 ]] Make sure we move the window to the top of the stack
				//   list to before raising windows.
				t_topped = true;
				MCstacks -> top(this);
				MCscreen->raisewindow(window);
				if (mode < WM_PULLDOWN && mode != WM_PALETTE)
					kfocusset(NULL);
			}
			else
			{
				state |= CS_NEED_RESIZE;
				MCRedrawScheduleUpdateForStack(this);
			}
			if (!t_topped)
				MCstacks->top(this);
			if (flags & F_DYNAMIC_PATHS)
				MCdynamiccard = curcard;
			return ES_NORMAL;
		}
		else
		{
			MClockmessages = reopening = True;
			close();
		}
	}
	
#ifdef _MAC_DESKTOP
	// MW-2008-02-28: [[ Bug 4614 ]] Ensure that sheeting inside a not yet visible window causes
	//   it to be displayed as modal.
	// MW-2008-03-03: [[ Bug 5985 ]] Crash when using 'sheet' command. It seems that 'go' is not
	//   doing appropriate parent window computations, so we repeat them here for OS X.
	// MW-2008-03-18: [[ Bug 6134 ]] Move this to here otherwise we get distinct oddness when
	//   switching cards in sheet stacks.
	if (wm == WM_SHEET)
	{
		if (parentptr == NULL)
		{
			if (MCdefaultstackptr -> getwindow() != NULL)
				parentptr = MCdefaultstackptr;
			else if (MCtopstackptr -> getwindow() != NULL)
				parentptr = MCtopstackptr;
		}

		extern bool MCPlatformIsWindowVisible(MCPlatformWindowRef window);
		if (parentptr == NULL || parentptr -> getwindow() == NULL || !MCPlatformIsWindowVisible(parentptr -> getwindow()))
			wm = WM_MODAL;
	}
#endif

	if (state & CS_FOREIGN_WINDOW)
		mode = wm;
	if ((wm != mode || parentptr != NULL) && window != NULL)
	{
		stop_externals();
		destroywindow();
		MCValueAssign(titlestring, kMCEmptyString);
	}
	mode = wm;
	wposition = wpos;
	walignment = walign;
	// IM-2014-07-23: [[ Bug 12930 ]] We can now get & set the parent stack directly
	if (parentptr == NULL)
		parentptr = getparentstack();
	else
		setparentstack(parentptr);
	
	// IM-2014-01-16: [[ StackScale ]] Ensure view has the current stack rect
	// If we have a window then set the viewport after opening to cover lockscreen
    bool t_had_window = window != NULL;
	if (!t_had_window)
    {
        view_setstackviewport(rect);
		createwindow();
    }

	if (substacks != NULL)
		opened++;
	else
	{
		MCObject::open();
	}
    
    if (t_had_window)
    {
        view_setstackviewport(rect);
    }
    
	MCRectangle trect;
	switch (mode)
	{
	case WM_TOP_LEVEL:
	case WM_TOP_LEVEL_LOCKED:
		MCstaticdefaultstackptr = this;
	case WM_MODELESS:
	case WM_PALETTE:
		break;
	case WM_MODAL:
	case WM_SHEET:
		positionrel(rel, OP_CENTER, OP_MIDDLE);
		break;
	case WM_DRAWER:
		{
			Object_pos wpx = OP_CENTER;
			Object_pos wpy = OP_MIDDLE;
			if (!MCaqua)
			{
				if (parentptr)
				{
					trect = rel;
					switch (wposition)
					{
					case WP_PARENTRIGHT:
						rect.height = MCU_min(parentptr->getrect().height,rect.height);
						wpx = OP_RIGHT;
						trect.width += 18;
						break;
					case WP_PARENTTOP:
						rect.width = MCU_min(parentptr->getrect().width,rect.width);
						wpy = OP_TOP;
						trect.y -= 25;
						break;
					case WP_PARENTBOTTOM:
						rect.width = MCU_min(parentptr->getrect().width,rect.width);
						wpy = OP_BOTTOM;
						trect.height += 25;
						break;
					case WP_PARENTLEFT:
					default:
						rect.height = MCU_min(parentptr->getrect().height, rect.height);
						trect.x -= 18;
						wpx = OP_LEFT;
						break;
					}
					positionrel(trect, wpx , wpy);
				}
			}
			break;
		}
	case WM_OPTION:
		if (MClook != LF_WIN95)
		{
			trect = rel;
			trect.y -= lasty - (trect.height >> 1);
			positionrel(trect, OP_CENTER, OP_ALIGN_TOP);
			break;
		}

	case WM_COMBO:
	case WM_PULLDOWN:
	case WM_TOOLTIP:
	{
		// MW-2010-09-28: [[ Bug 8585 ]] Make sure we use the 'workarea' of the display. This ensures
		//   tooltips place nice with the taskbar and other furniture.
		MCRectangle t_rect;
		MCU_set_rect(t_rect, rel . x + rel . width / 2, rel . y + rel . height / 2, 1, 1);
		const MCDisplay *t_display;
		t_display = MCscreen -> getnearestdisplay(t_rect);
		MCRectangle t_workarea;
		t_workarea = t_display -> workarea;
		if (rel.y + rel.height + rect.height > t_workarea . y + t_workarea . height - MENU_SPACE
		        && rel.y - rect.height > t_workarea . y + MENU_SPACE)
			positionrel(rel, OP_ALIGN_LEFT, OP_TOP);
		else
			positionrel(rel, OP_ALIGN_LEFT, OP_BOTTOM);
	}
	break;
	case WM_POPUP:
        if (wpos == WP_ASRECT)
        {
            rect = rel;
        }
        else
        {
            if (!MCmousestackptr)
                MCscreen->querymouse(trect.x, trect.y);
            else
            {
                //WEBREV
                // IM-2013-10-09: [[ FullscreenMode ]] Reimplement using MCStack::stacktogloballoc
                MCPoint t_globalloc;
                t_globalloc = MCmousestackptr->stacktogloballoc(MCPointMake(MCmousex, MCmousey));

                trect.x = t_globalloc.x;
                trect.y = t_globalloc.y;
            }
            trect.width = trect.height = 1;
            positionrel(trect, OP_ALIGN_LEFT, OP_ALIGN_TOP);
        }
		break;
	case WM_CASCADE:
		trect = rel;
		trect.x -= DEFAULT_BORDER << 1;
		trect.y -= DEFAULT_BORDER << 1;
		positionrel(trect, OP_RIGHT, OP_ALIGN_TOP);
		break;
	case WM_LICENSE:
		positionrel(rel, OP_CENTER, OP_MIDDLE);
		break;
	default:
		break;
	}
	
	if (cards == NULL)
	{
		cards = MCtemplatecard->clone(False, False);
		cards->setparent(this);
	}
	
	if (curcard == NULL)
		curcard = cards;
	
	// MW-2011-08-18: [[ Redraw ]] Update to use redraw.
	MCRedrawLockScreen();
	sethints();
	MCstacks->add(this);
	
	curcard->open();
	if (MCuseprivatecmap)
		MCscreen->setcmap(this);
	
	// MW-2011-09-12: [[ MacScroll / Bug 5268 ]] Make sure the scroll of the stack
	//   reflects what it should be taking into account menu settings.
	applyscroll();
	updatecardsize();
	
	MCCard *startcard = curcard;

	// IM-2014-09-23: [[ Bug 13349 ]] Store the lockscreen state to restore once the rest of the state is saved.
	uint16_t t_lock_screen;
	MCRedrawSaveLockScreen(t_lock_screen);
	
	// MW-2011-08-18: [[ Redraw ]] Make sure we don't save our lock.
	MCRedrawUnlockScreen();
	MCSaveprops sp;
	MCU_saveprops(sp);
	
	MCRedrawRestoreLockScreen(t_lock_screen);

	if (mode >= WM_MODELESS)
	{
		// IM-2014-09-23: [[ Bug 13349 ]] Restore lockscreen with other props.
		MCU_resetprops(True);
		MCRedrawRestoreLockScreen(t_lock_screen);
	}
	trect = rect;
	
	// "bind" the stack's rect... Or in other words, make sure its within the 
	// screens (well viewports) working area.
	if (!(flags & F_FORMAT_FOR_PRINTING) && !(state & CS_BEEN_MOVED))
		MCscreen->boundrect(rect, (!(flags & F_DECORATIONS) || decorations & WD_TITLE), mode, getflag(F_RESIZABLE));
	
	state |= CS_NO_FOCUS;
	if (flags & F_DYNAMIC_PATHS)
		MCdynamiccard = curcard;
    
#ifdef FEATURE_PLATFORM_PLAYER
    // PM-2014-10-13: [[ Bug 13569 ]] Detach all players before any messages are sent
    MCPlayer::DetachPlayers(curcard -> getstack());
#endif
		
	// MW-2008-10-31: [[ ParentScripts ]] Send preOpenControl appropriately
	if (curcard->message(MCM_preopen_stack) == ES_ERROR
	        || curcard != startcard
			|| curcard -> openbackgrounds(true, NULL) == ES_ERROR
	        || curcard != startcard
	        || curcard->message(MCM_preopen_card) == ES_ERROR
			|| curcard != startcard
			|| curcard -> opencontrols(true) == ES_ERROR
	        || curcard != startcard)
		if (curcard == startcard)
		{
			// MW-2011-08-17: [[ Redraw ]] Now using global screen lock and tell the stack to dirty all of itself.
			setgeom();
			state &= ~(CS_LOCK_SCREEN | CS_KFOCUSED | CS_NO_FOCUS);
			dirtyall();
			MCRedrawUnlockScreen();
			openwindow(mode >= WM_PULLDOWN);
			return ES_ERROR;
		}

#ifdef FEATURE_PLATFORM_PLAYER
    // PM-2014-10-13: [[ Bug 13569 ]] after any messages are sent, attach all players previously detached
    MCPlayer::AttachPlayers(curcard -> getstack());
#endif
	if (mode == WM_PULLDOWN || mode == WM_POPUP || mode == WM_CASCADE || (mode == WM_OPTION && MClook != LF_WIN95))
	{
		// MW-2014-03-12: [[ Bug 11914 ]] Only fiddle with scrolling and such
		//   if this is an engine menu.
		if (m_is_menu && menuheight == 0)
			menuheight = trect.height ;
		
		int2 oldy = rect.y;

		const MCDisplay *t_display;
		
		// TS-2008-01-21 : [[Bug 5566 - Handler menu not standard and blocking other menus ]]
		// Find the middle of the menu (in absolute screen co-ords)
		bool t_fullscreen_menu ;
		t_fullscreen_menu = false ;

		MCRectangle t_nearest;
		MCU_set_rect(t_nearest, rect . x + rect . width / 2, rect . y + rect . height / 2, 1, 1);
		t_display = MCscreen -> getnearestdisplay(t_nearest);

		MCRectangle t_workarea;
		t_workarea = t_display->workarea;
		
		// Make sure that we are not starting our menu off the top of the screen.
		if (rect . y < t_workarea . y + MENU_SPACE)
			rect . y = t_workarea . y + MENU_SPACE, oldy = rect . y;

		// Make sure that the length of the menu does not make it drop of the bottom of the screen.
		if (rect . y + rect . height > t_workarea . y + t_workarea . height - MENU_SPACE)
		{
			t_fullscreen_menu = true ;
			if (rect . height > t_workarea . height - ( 2 * MENU_SPACE) )
			{
				rect . y = t_workarea . y + MENU_SPACE ;
				rect . height = ( t_workarea . height - ( 2 * MENU_SPACE) ) ;
			}
			else
			{
				rect . y = t_workarea . y + t_workarea . height - MENU_SPACE - rect . height ;
				oldy = rect . y;
			}
		}
		
		// MW-2014-03-12: [[ Bug 11914 ]] Constrain the popup menu appropriately horizontally.
		if (rect . x < t_workarea . x + MENU_SPACE)
			rect . x = t_workarea . x + MENU_SPACE;
		if (rect . x + rect . width > t_workarea . x + t_workarea . width - MENU_SPACE)
			rect . x = t_workarea . x + t_workarea . width - rect . width - MENU_SPACE;

		// Get the middle point (Y) of the work area.
		int t_screen_mid_y ;
		t_screen_mid_y = ( t_workarea . height / 2 ) + t_workarea . y ;

		bool t_menu_downwards = ( rel.y < t_screen_mid_y ) ;

		// TS-2008-01-21 : [[Bug 5566 - Handler menu not standard and blocking other menus ]]
		// If the rel.y (i.e. the "originator" object is less than the center of the screen then we are above
		// the mid-point, so we will drop downwards, Else we will pop upwards.

		if ( t_fullscreen_menu && mode != WM_POPUP)
		{
			// Our "bound" contol is above the workarea mid point so we will be dropping down
			if ( t_menu_downwards )
			{
				int t_adjust_y ;
				t_adjust_y = ( rel.y + rel.height ) - rect.y;
				rect.y += t_adjust_y ;//- MENU_SPACE;
				rect.height -= t_adjust_y ;//+ ( 2*MENU_SPACE);
			}

			// Our "bound" control is below the workarea mid point so we will be pop-ing our menu upwards.
			else
			{
				int t_adjust_y ;
				t_adjust_y = ((rect.y + rect.height) - rel.y) ;
				rect.y -= t_adjust_y ;
				if ( rect.y < t_workarea.y + MENU_SPACE ) 
				{
					rect.y = t_workarea . y + MENU_SPACE ;
					rect.height = rel.y ;
				}
			}
		}

		if (m_is_menu)
			minheight = maxheight = rect.height;
		
		if (mode == WM_CASCADE && rect.x != trect.x)
		{
			trect = rel;
			trect.x += DEFAULT_BORDER << 1;
			trect.y -= DEFAULT_BORDER << 1;
			oldy = rect.y;
			positionrel(trect, OP_LEFT, OP_ALIGN_TOP);
			rect.y = oldy;
		}


		// TS-2008-01-21 : [[Bug 5566 - Handler menu not standard and blocking other menus ]]
		// Just do a little more adjusting for Full Screen Menus that are cascaded.
		// We need to pull the Y co-ord up again to be level (a little above in fact) of
		// the control we are bound to.
		if (( mode == WM_CASCADE ) && t_fullscreen_menu )
        {
			if ( t_menu_downwards )
				rect.y = ( rel.y - 2) ;
			else
			{
				// MW-2008-03-27: [[ Bug 6219 ]] Previously this was rect.height += ..., but I
				//   think this is a typo - we are just moving the menu, so we should just
				//   adjust the y-coord.
				rect.y += rel.height + 2 ;
			}
        }
	
	}


	state |= CS_ISOPENING;
	setgeom();
	state &= ~CS_ISOPENING;
	
	// MW-2011-08-18: [[ Redraw ]] Use global screen lock
	MCRedrawUnlockScreen();
	
	if (curcard == startcard)
	{
		
		// MW-2007-09-11: [[ Bug 5139 ]] Don't add activity to recent cards if the stack is an
		//   IDE stack.
		if ((mode == WM_TOP_LEVEL || mode == WM_TOP_LEVEL_LOCKED) && !m_is_ide_stack)
			MCrecent->addcard(curcard);

		// MW-2011-08-17: [[ Redraw ]] Tell the stack to dirty all of itself.
		dirtyall();
		dirtywindowname();
	}
	if (iconid != 0)
	{
		MCImage *iptr = (MCImage *)getobjid(CT_IMAGE, iconid);
		if (iptr != NULL)
			iptr->open();
	}

	// MW-2011-01-12: [[ Bug 9282 ]] Set to true if the props need restoring.
	bool t_restore_props;
	
    Exec_stat t_stat = ES_NORMAL;
	if (opened && flags & F_VISIBLE)
	{
		// MW-2011-08-19: [[ Redraw ]] Set the update region to everything.
		dirtyall();

		openwindow(mode >= WM_PULLDOWN);
		state |= CS_ISOPENING;
		if (state & CS_BEEN_MOVED)
			setgeom();
		state &= ~CS_ISOPENING;
		if (mode <= WM_SHEET)
		{
			MCscreen->waitconfigure(window);
			MCscreen->waitreparent(window);

			// MW-2009-09-09: If this is a plugin window, then we need to send a resizeStack
			//   as we have no control over the size of the window...
			view_configure(window != NULL ? False : True);
		}

		// MW-2008-10-31: [[ ParentScripts ]] Send openControl appropriately
		if (curcard->message(MCM_open_stack) == ES_ERROR
		        || curcard != startcard
				|| curcard -> openbackgrounds(false, NULL) == ES_ERROR
		        || curcard != startcard
		        || curcard->message(MCM_open_card) == ES_ERROR
				|| curcard != startcard
				|| curcard -> opencontrols(false) == ES_ERROR
		        || curcard != startcard)
			if (curcard == startcard)
			{
				state &= ~CS_NO_FOCUS;
				return ES_ERROR;
			}

		state &= ~CS_NO_FOCUS;
		int2 x, y;
		MCscreen->querymouse(x, y);
		if (MCU_point_in_rect(curcard->getrect(), x, y))
			resetcursor(True);

		// Only enter a modal loop if we are making local windows.
		// the rev supplied answer/ask dialogs.
		if ((mode == WM_MODAL || mode == WM_SHEET) &&
			MCModeMakeLocalWindows())
		{
			// If opening the dialog failed for some reason, this will return false.
			if (mode_openasdialog())
			{
				while (opened &&
                       (mode == WM_MODAL || mode == WM_SHEET) &&
                       !MCquit &&
                       !MCabortscript)
				{
					MCU_resetprops(True);
					// MW-2011-09-08: [[ Redraw ]] Make sure we flush any updates.
					MCRedrawUpdateScreen();
					MCscreen->siguser();
					if (MCscreen->wait(REFRESH_INTERVAL, True, True))
                    {
                        MCeerror->add(EE_WAIT_ABORT, 0, 0);
                        t_stat = ES_ERROR;
                        break;
                    }
				}
				mode_closeasdialog();
				if (MCquit)
					MCabortscript = False;
			}

            // If there was no error, make sure the mode is reset to closed so
            // it can be reopened. Otherwise, do the equivalent of 'close this
            // stack'.
            if (t_stat != ES_ERROR)
            {
                mode = WM_CLOSED;
            }
            else
            {
                close();
                checkdestroy();
			}
            
			t_restore_props = true;
		}
		else
			t_restore_props = mode >= WM_MODELESS;
	}
	else
	{
 		// MW-2008-10-31: [[ ParentScripts ]] Send openControl appropriately
		if (curcard->message(MCM_open_stack) == ES_ERROR
		        || curcard != startcard
				|| curcard -> openbackgrounds(false, NULL) == ES_ERROR
		        || curcard != startcard
		        || curcard->message(MCM_open_card) == ES_ERROR
		        || curcard != startcard
				|| curcard -> opencontrols(false) == ES_ERROR
				|| curcard != startcard)
		{
			state &= ~CS_NO_FOCUS;
			if (curcard == startcard)
				return ES_ERROR;
		}
 		state &= ~CS_NO_FOCUS;

		t_restore_props = mode >= WM_MODELESS;
	}
	if (t_restore_props)
	{
		// IM-2014-09-23: [[ Bug 13349 ]] Restore lockscreen with other props.
		MCU_restoreprops(sp);
		MCRedrawRestoreLockScreen(t_lock_screen);
		MCRedrawUnlockScreen();
	}
	if (reopening)
		MClockmessages = oldlock;
    
	return t_stat;
	
	
}


bool MCStack::getstackfiles(MCStringRef& r_stackfiles)
{
	bool t_success;
	t_success = true;
	
	MCAutoListRef t_file_list;
	
	if (t_success)
		t_success = MCListCreateMutable('\n', &t_file_list);
	
	for (uint2 i = 0; i < nstackfiles; i++)
	{
		MCAutoStringRef t_filename;
		
		if (t_success)
			t_success = MCStringFormat(&t_filename, "%@,%@", stackfiles[i].stackname, stackfiles[i].filename);
		
		if (t_success)
			t_success = MCListAppend(*t_file_list, *t_filename);
	}
	
	if (t_success)
		t_success = MCListCopyAsString(*t_file_list, r_stackfiles);
	
	return t_success;
}

bool MCStack::stringtostackfiles(MCStringRef d_strref, MCStackfile **sf, uint2 &nf)
{
	MCStackfile *newsf = NULL;
	uint2 nnewsf = 0;

    bool t_success;
	t_success = true;
    
	uindex_t t_old_offset;
	t_old_offset = 0;
	uindex_t t_new_offset;
	t_new_offset = 0;
    
	uindex_t t_length;
	t_length = MCStringGetLength(d_strref);
    
	while (t_success && t_old_offset <= t_length)
	{
		MCAutoStringRef t_line;
		
		if (!MCStringFirstIndexOfChar(d_strref, '\n', t_old_offset, kMCCompareExact, t_new_offset))
			t_new_offset = t_length;
        
		t_success = MCStringCopySubstring(d_strref, MCRangeMakeMinMax(t_old_offset, t_new_offset), &t_line);
		if (t_success && t_new_offset > t_old_offset)
		{
			MCAutoStringRef t_stack_name;
			MCAutoStringRef t_file_name;
            
			t_success = MCStringDivideAtChar(*t_line, ',', kMCCompareExact, &t_stack_name, &t_file_name);
            
			if (t_success && MCStringGetLength(*t_file_name) != 0)
			{
				MCU_realloc((char **)&newsf, nnewsf, nnewsf + 1, sizeof(MCStackfile));
				newsf[nnewsf].stackname = MCValueRetain(*t_stack_name);
				newsf[nnewsf].filename = MCValueRetain(*t_file_name);
				nnewsf++;
			}
		}
		t_old_offset = t_new_offset + 1;
	}
    
	if (t_success)
	{
		stackfiles = newsf;
		nstackfiles = nnewsf;
        
		if (nstackfiles != 0)
			flags |= F_STACK_FILES;
		else
			flags &= ~F_STACK_FILES;
	}
    
	*sf = newsf;
	nf = nnewsf;
    
    return t_success;
}

void MCStack::setstackfiles(MCStringRef s)
{
	while (nstackfiles--)
	{
		MCValueRelease(stackfiles[nstackfiles].stackname);
		MCValueRelease(stackfiles[nstackfiles].filename);
	}
	delete stackfiles;
	stringtostackfiles(s, &stackfiles, nstackfiles);
}

void MCStack::getstackfile(MCStringRef p_name, MCStringRef &r_name)
{
	if (stackfiles != NULL)
	{
		uint2 i;
		for (i = 0 ; i < nstackfiles ; i++)
			if (MCStringIsEqualTo(stackfiles[i].stackname, p_name, kMCStringOptionCompareCaseless))
			{
				if (MCStringIsEmpty(filename) || MCStringGetCharAtIndex(stackfiles[i].filename, 0) == '/' || MCStringGetCharAtIndex(stackfiles[i].filename, 1) == ':')
				{
					r_name = MCValueRetain(stackfiles[i].filename);
					return;
				}

				uindex_t t_index;
				if (!MCStringLastIndexOfChar(filename, PATH_SEPARATOR, -1, kMCStringOptionCompareExact, t_index))
				{
					r_name = MCValueRetain(filename);
					return;
				}
				
				MCStringRef t_filename;
				/* UNCHECKED */ MCStringMutableCopySubstring(filename, MCRangeMake(0, t_index + 1), t_filename);
				/* UNCHECKED */ MCStringAppend(t_filename, stackfiles[i].filename);
				/* UNCHECKED */ MCStringCopyAndRelease(t_filename, r_name);
				return;
			}
	}
	r_name = MCValueRetain(kMCEmptyString);
}

void MCStack::setfilename(MCStringRef f)
{
	MCAutoStringRef out_filename_string;
	MCU_fix_path(f, &out_filename_string);
	MCValueAssign(filename, *out_filename_string);
}

void MCStack::loadwindowshape()
{
	// MW-2009-07-22: We only set the windowShape *if* the window 'needstoopen'.
	if (windowshapeid)
	{
		destroywindowshape(); //just in case
		
#if defined(_DESKTOP)
		MCImage *t_image;
		// MW-2009-02-02: [[ Improved image search ]] Search for the appropriate image object using the standard method.
		t_image = resolveimageid(windowshapeid);
		if (t_image != NULL)
		{
			MCWindowShape *t_new_mask;
			t_image->setflag(True, F_I_ALWAYS_BUFFER);
			t_image->open();

			// IM-2014-10-22: [[ Bug 13746 ]] Scale window shape to both stack scale and backing buffer scale
			MCGFloat t_scale;
			t_scale = view_getbackingscale();
			t_scale *= view_get_content_scale();
			
			uint32_t t_width, t_height;
			t_width = t_image->getrect().width;
			t_height = t_image->getrect().height;
			
			t_new_mask = t_image -> makewindowshape(MCGIntegerSizeMake(t_width * t_scale, t_height * t_scale));
			t_image->close();
			// MW-2014-06-11: [[ Bug 12495 ]] Refactored action as different whether using platform API or not.
			if (t_new_mask != NULL)
				updatewindowshape(t_new_mask);
		}
#endif

	}
}

////////////////////////////////////////////////////////////////////////////////

void MCStack::dirtyrect(const MCRectangle& p_dirty_rect)
{
	// Make sure we are open and visible.
	if (!opened || !getflag(F_VISIBLE))
		return;

	// Make sure we have a card.
	if (curcard == nil)
		return;

	// IM-2013-12-19: [[ ShowAll ]] clip the transformed dirty rect to the visible viewport
	MCRectangle t_dirty_rect;
	t_dirty_rect = MCRectangleGetTransformedBounds(p_dirty_rect, gettransform());
	
	t_dirty_rect = MCU_intersect_rect(view_getstackvisiblerect(), t_dirty_rect);

	// Make sure the dirty rect falls within the windowshape bounds.
	if (m_window_shape != nil)
		t_dirty_rect = MCU_intersect_rect(MCU_make_rect(0, 0, m_window_shape -> width, m_window_shape -> height), t_dirty_rect);

	if (MCU_empty_rect(t_dirty_rect))
		return;
	
	MCRectangle t_view_rect;
	t_view_rect = MCRectangleGetTransformedBounds(t_dirty_rect, view_getviewtransform());
	
	view_dirty_rect(t_view_rect);
}

void MCStack::dirtyall(void)
{
	// Make sure we are open and visible.
	if (!opened || !getflag(F_VISIBLE))
		return;

	// Make sure we have a card.
	if (curcard == nil)
		return;

	// MW-2011-09-08: [[ TileCache ]] A 'dirtyall' request means wipe all cached
	//   data.
	view_flushtilecache();

	// MW-2011-09-21: [[ Layers ]] Make sure all the layers on the current card
	//   recompute their id's and other attrs.
	MCObjptr *t_objptr;
	t_objptr = curcard -> getobjptrs();
	if (t_objptr != nil)
	{
		do
		{
			t_objptr -> getref() -> layer_resetattrs();
			t_objptr = t_objptr -> next();
		}
		while(t_objptr != curcard -> getobjptrs());
	}

    dirtyrect(view_getstackvisiblerect());
}

void MCStack::dirtywindowname(void)
{
	state |= CS_TITLE_CHANGED;
	MCValueAssign(titlestring, kMCEmptyString);

	MCRedrawScheduleUpdateForStack(this);
}

#ifndef TARGET_SUBPLATFORM_IPHONE

// OpenGL stuff must be done on the system thread, so the iOS version of this handler
// does it on a block.
void MCStack::updatetilecache(void)
{
	// IM-2013-10-03: [[ FullsceenMode ]] Move implementation to stackview
	view_updatetilecache();
}

// MW-2013-06-26: [[ Bug 10990 ]] For non-iOS, we can run the MCTileCacheDeactivate
//   method on the main thread.
void MCStack::deactivatetilecache(void)
{
	// IM-2013-10-03: [[ FullsceenMode ]] Move implementation to stackview
	view_deactivatetilecache();
}

bool MCStack::snapshottilecache(MCRectangle p_area, MCGImageRef& r_pixmap)
{
	// IM-2013-10-03: [[ FullsceenMode ]] Move implementation to stackview
	return view_snapshottilecache(p_area, r_pixmap);
}

#endif

void MCStack::applyupdates(void)
{
	// MW-2011-09-13: [[ Effects ]] Ditch any snapshot.
	if (m_snapshot != nil)
	{
		MCGImageRelease(m_snapshot);
		m_snapshot = nil;
	}
	
	// Ensure the title is up to date.
	if (getstate(CS_TITLE_CHANGED))
		setwindowname();

	view_apply_updates();

#if defined(_DESKTOP)
	// MW-2011-11-03: [[ Bug 9852 ]] If the previous blendlevel value is not the current
	//   one perform an update.
	if (window != nil && old_blendlevel != blendlevel)
	{
		old_blendlevel = blendlevel;
		setopacity(blendlevel * 255 / 100);
	}
#endif
}

void MCStack::render(MCGContextRef p_context, const MCRectangle &p_rect)
{
	// IM-2014-01-07: [[ StackScale ]] Apply stack transform to context
	MCGAffineTransform t_transform;
	t_transform = gettransform();
	
	MCGContextConcatCTM(p_context, t_transform);

	// IM-2014-01-07: [[ StackScale ]] Use inverse stack transform to get redraw rect in card coords
	MCRectangle t_rect;
	t_rect = MCRectangleGetTransformedBounds(p_rect, MCGAffineTransformInvert(t_transform));
	
	MCGraphicsContext *t_old_context = nil;
	t_old_context = new (nothrow) MCGraphicsContext(p_context);
	if (t_old_context != nil)
		render(t_old_context, t_rect);
	delete t_old_context;
}

////////////////////////////////////////////////////////////////////////////////

// MM-2014-07-31: [[ ThreadedRendering ]] MCStackTile wraps a MCStackSurface and allows for the locking and rendring of the given region.
//  This way we can easily split the stack surface into multiple regions and render each on a separate thread.
class MCGContextStackTile: public MCStackTile
{
public:
    MCGContextStackTile(MCStack *p_stack, MCStackSurface *p_surface, const MCGIntegerRectangle &p_region)
    {
        m_stack = p_stack;
        m_surface = p_surface;
        m_region = p_region;
        m_context = NULL;
    }
    
    ~MCGContextStackTile()
    {
    }
    
    bool Lock(void)
    {
        return m_surface -> LockGraphics(m_region, m_context, m_raster);
    }
    
	void Unlock(void)
    {
        m_surface -> UnlockGraphics(m_region, m_context, m_raster);
    }
    
    void Render(void)
    {
#ifndef _MAC_DESKTOP
        // IM-2014-01-24: [[ HiDPI ]] Use view backing scale to transform surface -> logical coords
        MCGFloat t_backing_scale;
        t_backing_scale = m_stack -> view_getbackingscale();
        
        // p_region is in surface coordinates, translate to user-space coords & ensure any fractional pixels are accounted for
        MCRectangle t_rect;
        t_rect = MCRectangleGetScaledBounds(MCRectangleFromMCGIntegerRectangle(m_region), 1 / t_backing_scale);
        
        // scale user -> surface space
        MCGContextScaleCTM(m_context, t_backing_scale, t_backing_scale);
        
        m_stack -> view_render(m_context, t_rect);
#else
        m_stack -> view_render(m_context, MCRectangleFromMCGIntegerRectangle(m_region));
#endif
    }
    
private:
    MCStack             *m_stack;
    MCStackSurface      *m_surface;
    MCGIntegerRectangle m_region;
    MCGContextRef       m_context;
    MCGRaster           m_raster;
};

void MCStack::view_surface_redrawwindow(MCStackSurface *p_surface, MCGRegionRef p_region)
{
	MCTileCacheRef t_tilecache;
	t_tilecache = view_gettilecache();
	
    // SN-2014-08-25: [[ Bug 13187 ]] MCplayers's syncbuffering relocated
    MCPlayer::SyncPlayers(this, nil);
	if (t_tilecache == nil || !MCTileCacheIsValid(t_tilecache))
	{
        MCGIntegerRectangle t_bounds;
        t_bounds = MCGRegionGetBounds(p_region);
    
        MCGContextStackTile t_tile(this, p_surface, t_bounds);
        if (t_tile . Lock())
        {
            t_tile . Render();
            t_tile . Unlock();
        }
	}
	else
		// We have a valid tilecache, so get it to composite.
		MCTileCacheComposite(t_tilecache, p_surface, p_region);
}

////////////////////////////////////////////////////////////////////////////////


void MCStack::takewindowsnapshot(MCStack *p_other_stack)
{
	MCGImageRelease(m_snapshot);
	m_snapshot = p_other_stack -> m_snapshot;
	p_other_stack -> m_snapshot = nil;
}

void MCStack::snapshotwindow(const MCRectangle& p_area)
{
	// Dispose of any existing snapshot.
	if (m_snapshot != nil)
	{
		MCGImageRelease(m_snapshot);
		m_snapshot = nil;
	}

	// If the window isn't open, or is invisible do nothing.
	if (opened == 0 || window == NULL || !isvisible())
		return;
	
	// MW-2011-10-18: [[ Bug 9822 ]] Make sure we restrict the snapshot to the visible
	//   region of the card (taking into account menuscroll).
	MCRectangle t_effect_area;
	t_effect_area = curcard -> getrect();
	t_effect_area . y = getscroll();
	t_effect_area . height -= t_effect_area . y;
	t_effect_area = MCU_intersect_rect(t_effect_area, p_area);
	
	if (view_getacceleratedrendering() && view_need_redraw())
	{
		updatetilecache();
	}
	
	if (!snapshottilecache(t_effect_area, m_snapshot))
	{	
		bool t_success = true;
		// Render the current content.
		MCGContextRef t_context = nil;
		MCContext *t_gfxcontext = nil;

		// IM-2013-07-18: [[ ResIndependence ]] take stack snapshot at device resolution
		// IM-2013-08-21: [[ ResIndependence ]] Align snapshots to device pixel boundaries
		// IM-2013-09-30: [[ FullscreenMode ]] Use stack transform to get device coords
		MCGAffineTransform t_transform = getdevicetransform();
		
		// MW-2013-10-29: [[ Bug 11330 ]] Make sure the effect area is cropped to the visible
		//   area.
		t_effect_area = MCRectangleGetTransformedBounds(t_effect_area, getviewtransform());
		t_effect_area = MCU_intersect_rect(t_effect_area, MCU_make_rect(0, 0, view_getrect() . width, view_getrect() . height));
		
		MCRectangle t_surface_rect, t_user_rect;
		t_surface_rect = MCRectangleGetScaledBounds(t_effect_area, view_getbackingscale());
		t_user_rect = MCRectangleGetTransformedBounds(t_surface_rect, MCGAffineTransformInvert(t_transform));
		
		if (t_success)
			t_success = MCGContextCreate(t_surface_rect.width, t_surface_rect.height, false, t_context);

		if (t_success)
		{
			MCGContextTranslateCTM(t_context, -t_surface_rect.x, -t_surface_rect.y);
			
			// IM-2013-09-30: [[ FullscreenMode ]] Apply stack transform to snapshot context
			MCGContextConcatCTM(t_context, t_transform);
			
			t_success = nil != (t_gfxcontext = new (nothrow) MCGraphicsContext(t_context));
		}

		if (t_success)
		{
			t_gfxcontext -> setclip(t_user_rect);
			render(t_gfxcontext, t_user_rect);
			
			if (m_window_shape != nil && !m_window_shape -> is_sharp)
			{
				t_gfxcontext -> setclip(t_user_rect);
				t_gfxcontext -> applywindowshape(m_window_shape, t_user_rect . width, t_user_rect . height);
			}
			
		}

		if (t_success)
			t_success = MCGContextCopyImage(t_context, m_snapshot);

		delete t_gfxcontext;
		MCGContextRelease(t_context);

	}
	
#ifdef _ANDROID_MOBILE
	// MW-2012-12-09: [[ Bug 9901 ]] In OpenGL mode, we use the Android bitmap view for
	//   effects, however this will be out of sync for partial screen effects. Thus, this
	//   call makes sure it contains the current contents of the screen.
	preservescreenforvisualeffect(p_area);
#endif
}

void MCStack::render(MCContext *p_context, const MCRectangle& p_dirty)
{
	p_context -> setfunction(GXcopy);
	p_context -> setopacity(255);

    MCRectangle t_clipped_visible = p_dirty;
	
	// MW-2014-03-12: [[ Bug 11914 ]] Only fiddle with scrolling and such
	//   if this is an engine menu.
    if (m_is_menu && (menuheight > rect . height || menuy != 0))
        clipmenu(p_context, t_clipped_visible);
    
    curcard -> draw(p_context, t_clipped_visible, false);
}

////////////////////////////////////////////////////////////////////////////////

void MCStack::updatewindow(MCRegionRef p_region)
{
	view_updatestack(p_region);
}

MCGAffineTransform MCStack::gettransform(void) const
{
	MCGAffineTransform t_transform;
	// IM-2013-10-11: [[ FullscreenMode ]] Add scroll offset to stack transform
	t_transform = MCGAffineTransformMakeTranslation(0.0, -(MCGFloat)getscroll());
	
	return t_transform;
}

MCGAffineTransform MCStack::getviewtransform(void) const
{
	return MCGAffineTransformConcat(view_getviewtransform(), gettransform());
}

MCGAffineTransform MCStack::getdevicetransform(void) const
{
	MCGFloat t_scale;
	t_scale = view_getbackingscale();
	
	return MCGAffineTransformConcat(MCGAffineTransformMakeScale(t_scale, t_scale), getviewtransform());
}

MCGAffineTransform MCStack::getroottransform(void) const
{
	return MCGAffineTransformConcat(view_getroottransform(), getviewtransform());
}

////////////////////////////////////////////////////////////////////////////////

MCGFloat MCStack::getdevicescale(void) const
{
	MCGAffineTransform t_transform;
	t_transform = getdevicetransform();
	
	return MCMax(fabsf(t_transform.a), fabsf(t_transform.d));
}

////////////////////////////////////////////////////////////////////////////////

// IM-2013-10-08: [[ FullscreenMode ]] Ensure rect of resizable stacks is within min/max width & height
MCRectangle MCStack::constrainstackrect(const MCRectangle &p_rect)
{
	if (!getflag(F_RESIZABLE))
		return p_rect;
		
	uint32_t t_width, t_height;
	t_width = MCMax(p_rect.width, minwidth);
	t_height = MCMax(p_rect.height, minheight);
	
	t_width = MCMin(p_rect.width, maxwidth);
	t_height = MCMin(p_rect.height, maxheight);
	
	return MCRectangleMake(p_rect.x, p_rect.y, t_width, t_height);
}

////////////////////////////////////////////////////////////////////////////////

MCPoint MCStack::windowtostackloc(const MCPoint &p_windowloc) const
{
	// IM-2014-01-07: [[ StackScale ]] Use view->stack transform to convert coords
	return MCGPointToMCPoint(MCGPointApplyAffineTransform(MCPointToMCGPoint(p_windowloc), MCGAffineTransformInvert(getviewtransform())));
}

MCPoint MCStack::stacktowindowloc(const MCPoint &p_stackloc) const
{
	// IM-2014-01-07: [[ StackScale ]] Use stack->view transform to convert coords
	return MCGPointToMCPoint(MCGPointApplyAffineTransform(MCPointToMCGPoint(p_stackloc), getviewtransform()));
}

////////////////////////////////////////////////////////////////////////////////

MCPoint MCStack::globaltostackloc(const MCPoint &p_windowloc) const
{
	// IM-2014-01-07: [[ StackScale ]] Use root->stack transform to convert coords
	return MCGPointToMCPoint(MCGPointApplyAffineTransform(MCPointToMCGPoint(p_windowloc), MCGAffineTransformInvert(getroottransform())));
}

MCPoint MCStack::stacktogloballoc(const MCPoint &p_stackloc) const
{
	// IM-2014-01-07: [[ StackScale ]] Use stack->root transform to convert coords
	return MCGPointToMCPoint(MCGPointApplyAffineTransform(MCPointToMCGPoint(p_stackloc), getroottransform()));
}

////////////////////////////////////////////////////////////////////////////////

MCRectangle MCStack::getcardrect() const
{
	MCRectangle t_stackrect;
	t_stackrect = MCRectangleMake(0, 0, rect.width, rect.height);
	
	MCRectangle t_cardrect;
	t_cardrect = MCRectangleGetTransformedBounds(t_stackrect, MCGAffineTransformInvert(gettransform()));
	
	t_cardrect.x = t_cardrect.y = 0;
	t_cardrect.height += getscroll();
	
	return t_cardrect;
}

void MCStack::updatecardsize()
{
	MCRectangle t_cardrect;
	t_cardrect = getcardrect();
	
	// MW-2011-08-19: [[ Layers ]] Notify of change in size of canvas.
	//   This call also calls MCCard::resize to update the rect.
	curcard -> layer_setviewport(0, 0, t_cardrect.width, t_cardrect.height);
}

MCRectangle MCStack::getvisiblerect(void)
{
	MCRectangle t_rect;
	t_rect = view_getstackvisiblerect();
	
	return MCRectangleGetTransformedBounds(t_rect, MCGAffineTransformInvert(gettransform()));
}

////////////////////////////////////////////////////////////////////////////////

bool MCStack::foreachstack(MCStackForEachCallback p_callback, void *p_context)
{
    if (!p_callback(this, p_context))
        return false;
    
	bool t_continue;
	t_continue = true;
    
	if (substacks != NULL)
	{
		MCStack *t_stack = substacks;
		do
		{
            		t_continue = p_callback(t_stack, p_context);
			t_stack = (MCStack *)t_stack->next();
		}
		while (t_continue && t_stack != substacks);
	}
	
	return t_continue;
}

bool MCStack::foreachchildstack(MCStackForEachCallback p_callback, void *p_context)
{
	bool t_continue;
	t_continue = true;
	
	if (substacks != NULL)
	{
		MCStack *t_stack = substacks;
		do
		{
			if (t_stack->getparentstack() == this)
				t_continue = p_callback(t_stack, p_context);

			t_stack = (MCStack *)t_stack->next();
		}
		while (t_continue && t_stack != substacks);
	}
	
	return t_continue;
}


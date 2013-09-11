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

#include "core.h"
#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "mcio.h"

#include "execpt.h"
#include "stack.h"
#include "tooltip.h"
#include "dispatch.h"
#include "aclip.h"
#include "vclip.h"
#include "card.h"
#include "objptr.h"
#include "control.h"
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

Boolean MCStack::setscript(char *newscript)
{
	delete script;
	script = newscript;
	flags |= F_SCRIPT;
	parsescript(False);
	if (hlist == NULL)
	{
		uint2 line, pos;
		MCperror->geterrorloc(line, pos);
		fprintf(stderr, "%s: Script parsing error at line %d, column %d\n",
		        MCcmd, line, pos);
		return False;
	}
	if (!hlist->hashandlers())
	{
		fprintf(stderr, "%s: Script has no handlers\n", MCcmd);
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
				MCtodestroy->remove(this); // prevent duplicates
				MCtodestroy->add(this);
			}
	}
	else
	{
		MCStack *sptr = (MCStack *)parent;
		sptr->checkdestroy();
	}
}

void MCStack::resize(uint2 oldw, uint2 oldh)
{
	uint2 newy = getscroll();
	
	// MW-2011-08-18: [[ Redraw ]] Use global screen lock, rather than stack.
	MCRedrawLockScreen();
	if (curcard->getrect().width != rect.width || curcard->getrect().height != rect.height + newy)
	{
		// MW-2011-08-19: [[ Layers ]] Notify of change in size of canvas.
		//   This call also calls MCCard::resize to update the rect.
		curcard -> layer_setviewport(0, 0, rect . width, rect . height + newy);

		// MM-2012-09-05: [[ Property Listener ]] Make sure resizing a stack sends the propertyChanged message to any listeners.
		//    This effects both the current card and the stack.
		curcard -> signallisteners(P_RECTANGLE);
		signallisteners(P_RECTANGLE);	
		
		curcard->message_with_args(MCM_resize_stack, rect.width, rect.height + newy, oldw, oldh);
	}
	MCRedrawUnlockScreen();

	// MW-2011-08-18: [[ Redraw ]] For now, update the screen here. This should
	//   really be done 'in general' after event dispatch, but things don't work
	//   in a suitable way for that... Yet...
	MCRedrawUpdateScreen();
}

// MW-2007-07-03: [[ Bug 2332 ]] - It makes more sense for resize to be issue before move.
//   Not doing so results in the stack's rect property being incorrect size-wise during execution
//   of moveStack.
void MCStack::configure(Boolean user)
{
	// MW-2011-08-18: [[ Redraw ]] Update to use redraw.
	if (MCRedrawIsScreenLocked() || state & CS_NO_CONFIG || !mode_haswindow() || !opened)
		return;
#ifdef TARGET_PLATFORM_LINUX
 	if (!getflag(F_VISIBLE))
		return;
#endif
	Boolean beenchanged = False;
	MCRectangle trect;
	mode_getrealrect(trect);
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
	{
		MCStack *cstack = NULL;
		uint2 ccount = 1;
		while (True)
		{
			cstack = MCdispatcher->findchildstackd(window, ccount);
			if (cstack == NULL)
				break;
			ccount++;
			cstack->configure(True); //update children
		}
	}
}

void MCStack::iconify()
{
	if (!(state & CS_ICONIC))
	{
		MCtooltip->settip(NULL);
		MCiconicstacks++;
		state |= CS_ICONIC;
		MCstacks->top(NULL);
		redrawicon();
		curcard->message(MCM_iconify_stack);
	}
}

void MCStack::uniconify()
{
	if (state & CS_ICONIC)
	{
		MCiconicstacks--;
		state &= ~CS_ICONIC;
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
	if (mode == WM_TOP_LEVEL && optr -> isselectable())
		return MCcurtool;

	return T_BROWSE;
}

void MCStack::hidecursor()
{
	if (MCmousestackptr == this)
	{
		cursor = MCcursors[PI_NONE];
		MCscreen->setcursor(window, cursor);
	}
}

void MCStack::setcursor(MCCursorRef newcursor, Boolean force)
{
	if (window == DNULL && MCModeMakeLocalWindows())
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
		if (linkatts != NULL)
		{
			MCscreen->alloccolor(linkatts->color);
			MCscreen->alloccolor(linkatts->hilitecolor);
			MCscreen->alloccolor(linkatts->visitedcolor);
		}
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
#ifdef _MACOSX
	if (!opened)
#else
	if (!opened || state & CS_ICONIC)
#endif

		return DNULL;
	else
		return window;
}

Window MCStack::getparentwindow()
{
#if defined(_MACOSX) || defined(_WINDOWS)
	if (parentwindow != DNULL)
	{
		if (MCdispatcher->findstackd(parentwindow) == NULL)
		{
			delete parentwindow;
			parentwindow = DNULL;
		}
		return parentwindow;
	}
	return DNULL;
#else
	if (parentwindow != DNULL &&
		MCdispatcher -> findstackd(parentwindow) == NULL)
		parentwindow = DNULL;
	return parentwindow;
#endif
}

void MCStack::setparentwindow(Window w)
{
#if defined(_MACOSX) || defined(_WINDOWS)
	if (w != DNULL && w->handle.window != 0)
	{
		if (parentwindow == DNULL)
		{
			parentwindow = new _Drawable;
			parentwindow->type = DC_WINDOW;
		}
		parentwindow->handle.window = w->handle.window;
	}
	else
		if (parentwindow != DNULL)
		{
			delete parentwindow;
			parentwindow = DNULL;
		}
#else
	if (parentwindow == window)
		parentwindow = None;
	parentwindow = w;
#endif
}

Boolean MCStack::takewindow(MCStack *sptr)
{
	// If there is no window ptr and we 'have' a window (i.e. plugin)
	// we can't take another one's window.
	if (window == DNULL && mode_haswindow())
		return False;

	// MW-2008-10-31: [[ ParentScripts ]] Send closeControl messages appropriately
	if (sptr -> curcard -> closecontrols() == ES_ERROR ||
		sptr->curcard->message(MCM_close_card) == ES_ERROR ||
		//sptr->curcard->groupmessage(MCM_close_background, NULL) == ES_ERROR
		sptr -> curcard -> closebackgrounds(NULL) == ES_ERROR ||
		sptr->curcard->message(MCM_close_stack) == ES_ERROR)
		return False;
	if (window != DNULL)
	{
		stop_externals();
		MCscreen->destroywindow(window);
		cursor = None;
		delete titlestring;
		titlestring = NULL;
	}
	delete sptr->titlestring;
	sptr->titlestring = NULL;
	window = sptr->window;
	iconid = sptr->iconid;
	sptr->stop_externals();
	sptr->window = DNULL;
	
	state = sptr->state;
	state &= ~(CS_IGNORE_CLOSE | CS_NO_FOCUS | CS_DELETE_STACK);

	// MW-2012-04-20: [[ Bug 10185 ]] Make sure we take the visibility flag of the
	//   target stack (else if we go in window with an invisible stack redraw issue
	//   happens).
	flags &= ~(F_TAKE_FLAGS | F_VISIBLE);
	flags |= sptr->flags & (F_TAKE_FLAGS | F_VISIBLE);
	decorations = sptr->decorations;
	rect = sptr->rect;
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
	
	mode_takewindow(sptr);
	
	if (MCmousestackptr == sptr)
		MCmousestackptr = this;
	start_externals();
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

void MCStack::kfocusset(MCControl *target)
{
	if (!opened)
		return;
	if (MCactivefield != NULL && target != NULL && MCactivefield != target)
	{
		MCactivefield->unselect(False, True);
		if (MCactivefield != NULL)
			if (MCactivefield->getstack() == this)
				curcard->kunfocus();
			else
				if (MCactivefield->getstack()->state & CS_KFOCUSED)
					MCactivefield->getstack()->kunfocus();
				else
					MCactivefield->unselect(True, True);
	}
	if (MCactivefield != NULL && target != NULL)
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
	if (this != MCtemplatestack || rect.x == 0 && rect.y == 0)
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
	if (curcard != NULL)
		return curcard->checkid(controlid);
	return False;
}

IO_stat MCStack::saveas(const MCString &fname)
{
	Exec_stat stat = curcard->message(MCM_save_stack_request);
	if (stat == ES_NOT_HANDLED || stat == ES_PASS)
	{
		MCStack *sptr = this;
		if (!MCdispatcher->ismainstack(sptr))
			sptr = (MCStack *)sptr->parent;
		MCdispatcher->savestack(sptr, fname);
	}
	return IO_NORMAL;
}

MCStack *MCStack::findname(Chunk_term type, const MCString &findname)
{ // should do case-sensitive match on filename on UNIX...
	if (type == CT_STACK)
	{
		if (MCU_matchname(findname, CT_STACK, getname()))
		return this;
		
		MCAutoNameRef t_filename_name;
		if (filename != nil)
			t_filename_name . CreateWithCString(filename);

		if (MCU_matchname(findname, CT_STACK, t_filename_name))
			return this;
	}

		return NULL;
}

MCStack *MCStack::findid(Chunk_term type, uint4 inid, Boolean alt)
{
	if (type == CT_STACK && (inid == obj_id || alt && inid == altid))
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
	cards = curcard = new MCCard;

	// Link the card to the parent, give it the same id as the current card and give it a temporary script
	curcard->setparent(this);
	curcard->setid(savecard->getid());
	curcard->setsprop(P_SCRIPT, ECS);

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
	curcard->resize(rect.width, rect.height + getscroll());
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
	curcard->resize(rect.width, rect.height + getscroll());
	// MW-2011-08-17: [[ Redraw ]] Tell the stack to dirty all of itself.
	dirtyall();
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
		if (!hasmenubar() || state & CS_EDIT_MENUS
		        && mode < WM_PULLDOWN && mode != WM_PALETTE
		        || gettool(this) != T_BROWSE && MCdefaultmenubar != NULL)
			MCmenubar = NULL;
		else
			MCmenubar = (MCGroup *)getobjname(CT_GROUP, MCNameGetOldString(getmenubar()));
		MCscreen->updatemenubar(False);
	}
}

// MW-2011-09-12: [[ MacScroll ]] Compute the scroll as it should be now taking
//   into account the menubar and such.
int32_t MCStack::getnextscroll()
{
#ifdef _MACOSX
	MCControl *mbptr;
	if (!(state & CS_EDIT_MENUS) && hasmenubar()
	        && (mbptr = curcard->getchild(CT_EXPRESSION, MCNameGetOldString(getmenubar()), CT_GROUP, CT_UNDEFINED)) != NULL
	        && mbptr->getopened() && mbptr->isvisible())
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

MCObject *MCStack::getAV(Chunk_term etype, const MCString &s, Chunk_term otype)
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
			return getAVname(otype, s);
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

MCCard *MCStack::getchild(Chunk_term etype, const MCString &s,
                          Chunk_term otype)
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
	if (etype == CT_EXPRESSION && s == "window")
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
		if (MCU_stoui4(s, inid))
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
		if (MCU_stoui2(s, num))
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
				found = cptr->findname(otype, s);
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

MCGroup *MCStack::getbackground(Chunk_term etype, const MCString &s,
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
		return (MCGroup *)curcard->getchild(CT_FIRST, MCnullmcstring,
		                                    otype, CT_BACKGROUND);
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
			MCGroup *gptr = (MCGroup *)curcard->getchild(CT_FIRST, MCnullmcstring,
			                otype, CT_BACKGROUND);
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
			MCGroup *gptr = (MCGroup *)curcard->getchild(CT_FIRST, MCnullmcstring,
			                otype, CT_BACKGROUND);
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
		if (MCU_stoui4(s, inid))
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
		if (MCU_stoui2(s, num))
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
				MCControl *found = cptr->findname(otype, s);
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

void MCStack::addmnemonic(MCButton *button, uint1 key)
{
	MCU_realloc((char **)&mnemonics, nmnemonics,
	            nmnemonics + 1, sizeof(Mnemonic));
	mnemonics[nmnemonics].button = button;
	mnemonics[nmnemonics].key = MCS_tolower(key);
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

MCButton *MCStack::findmnemonic(char which)
{
	uint2 i;
	for (i = 0 ; i < nmnemonics ; i++)
		if (mnemonics[i].key == MCS_tolower(which))
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
	if (!opened || isunnamed() || window == DNULL)
		return;

	char *t_utf8_name;
	t_utf8_name = NULL;

	const char *tptr;
	if (title == NULL)
	{
		MCExecPoint ep;
		ep . setnameref_unsafe(getname());
		ep . nativetoutf8();
		t_utf8_name = ep . getsvalue() . clone();
		tptr = t_utf8_name;
	}
	else
		tptr = title;

	char *newname = NULL;
	if (editing != NULL)
	{
		MCExecPoint ep;
		editing->names(P_SHORT_NAME, ep, 0);
		ep.nativetoutf8();
		char *bgname = ep.getsvalue().clone();
		newname = new char[strlen(tptr) + strlen(MCbackgroundstring)
		                   + strlen(bgname) + 7];
		sprintf(newname, "%s (%s \"%s\")", tptr, MCbackgroundstring, bgname);
		delete bgname;
	}
	else
	{
		newname = new char[strlen(tptr) + U4L + 6];
		if (title == NULL && mode == WM_TOP_LEVEL && MCdispatcher->cut(True))
		{
			if ((cards->next()) == cards)
				sprintf(newname, "%s *", tptr);
			else
			{
				uint2 num;
				count(CT_CARD, CT_UNDEFINED, curcard, num);
				sprintf(newname, "%s (%d) *", tptr, num);
			}
		}
		else
			if (title == NULL && mode == WM_TOP_LEVEL_LOCKED
			        && cards->next() != cards)
			{
				uint2 num;
				count(CT_CARD, CT_UNDEFINED, curcard, num);
				sprintf(newname, "%s (%d)", tptr, num);
			}
			else
				strcpy(newname, tptr);
	}
	if (!strequal(newname, titlestring))
	{
		delete titlestring;
		titlestring = newname;
		MCscreen->setname(window, titlestring);
	}
	else
		delete newname;
	state &= ~CS_TITLE_CHANGED;

	if (t_utf8_name != NULL)
		delete t_utf8_name;
}

void MCStack::reopenwindow()
{
	if (state & CS_FOREIGN_WINDOW)
		return;

	stop_externals();

	// MW-2006-05-23: Only close and open the window if it is visible
	if (getflag(F_VISIBLE))
		MCscreen->closewindow(window);

	MCscreen->destroywindow(window);

	delete titlestring;
	titlestring = NULL;
	if (getstyleint(flags) != 0)
		mode = (Window_mode)(getstyleint(flags) + WM_TOP_LEVEL_LOCKED);

	// MW-2011-08-18: [[ Redraw ]] Use global screen lock
	MCRedrawLockScreen();
	realize();
	sethints();
	setgeom();
	MCRedrawUnlockScreen();

	dirtywindowname();

	if (getflag(F_VISIBLE))
		openwindow(mode >= WM_PULLDOWN);
}

Exec_stat MCStack::openrect(const MCRectangle &rel, Window_mode wm, MCStack *parentptr, Window_position wpos,  Object_pos walign)
{
	MCRectangle myoldrect = rect;
	if (state & (CS_IGNORE_CLOSE | CS_NO_FOCUS | CS_DELETE_STACK))
		return ES_NORMAL;

	MCtodestroy->remove(this); // prevent delete
	if (wm == WM_LAST)
		if (opened)
			wm = mode;
		else
			if (getstyleint(flags) == 0)
				wm = WM_TOP_LEVEL;
			else
				wm = (Window_mode)(getstyleint(flags) + WM_TOP_LEVEL_LOCKED);
	if (wm == WM_TOP_LEVEL
	        && (flags & F_CANT_MODIFY || getextendedstate(ECS_IDE) || !MCdispatcher->cut(True)))
		wm = WM_TOP_LEVEL_LOCKED;

	Boolean oldlock = MClockmessages;
	Boolean reopening = False;
	
	// MW-2005-07-18: If this stack has substacks that have been opened, then it is marked as open even though
	//   it might not be.
	if (opened)
	{
		if (window == NULL && !MCModeMakeLocalWindows() && (wm != WM_MODAL && wm != WM_SHEET || wm == mode))
			return ES_NORMAL;

		if (window == NULL && wm == mode)
			fprintf(stderr, "*** ASSERTION FAILURE: MCStack::openrect() - window == NULL\n");
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
	
#ifdef _MACOSX
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
		
		extern bool MCMacIsWindowVisible(Window window);
		if (parentptr == NULL || parentptr -> getwindow() == NULL || !MCMacIsWindowVisible(parentptr -> getwindow()))
			wm = WM_MODAL;
	}
#endif

	if (state & CS_FOREIGN_WINDOW)
		mode = wm;
	if ((wm != mode || parentptr != NULL) && window != DNULL)
	{
		stop_externals();
		MCscreen->destroywindow(window);
		delete titlestring;
		titlestring = NULL;
	}
	mode = wm;
	wposition = wpos;
	walignment = walign;
	if (parentptr == NULL)
	{
		if (parentwindow != DNULL)
			parentptr = MCdispatcher->findstackd(parentwindow);
	}
	else
		setparentwindow(parentptr->getw());
	if (window == DNULL)
		realize();

	if (substacks != NULL)
		opened++;
	else
	{
		MCObject::open();
		if (linkatts != NULL)
		{
			MCscreen->alloccolor(linkatts->color);
			MCscreen->alloccolor(linkatts->hilitecolor);
			MCscreen->alloccolor(linkatts->visitedcolor);
		}
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
		if (MCmousestackptr == NULL)
			MCscreen->querymouse(trect.x, trect.y);
		else
		{
			//WEBREV
			MCRectangle srect = MCmousestackptr->getrect();
			trect.x = MCmousex + srect.x;
			trect.y = MCmousey + srect.y
			          - MCmousestackptr->getscroll();
		}
		trect.width = trect.height = 1;
		positionrel(trect, OP_ALIGN_LEFT, OP_ALIGN_TOP);
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
	curcard->resize(rect.width, rect.height + getscroll());
	
	MCCard *startcard = curcard;

	// MW-2011-08-19: [[ Redraw ]] Reset the update region.
	MCRegionDestroy(m_update_region);
	m_update_region = nil;
	setstate(False, CS_NEED_REDRAW);

	// MW-2011-08-18: [[ Redraw ]] Make sure we don't save our lock.
	MCRedrawUnlockScreen();
	MCSaveprops sp;
	MCU_saveprops(sp);
	MCRedrawLockScreen();

	if (mode >= WM_MODELESS)
		MCU_resetprops(True);
	trect = rect;
	
	// "bind" the stack's rect... Or in other words, make sure its within the 
	// screens (well viewports) working area.
	if (!(flags & F_FORMAT_FOR_PRINTING) && !(state & CS_BEEN_MOVED))
		MCscreen->boundrect(rect, (!(flags & F_DECORATIONS) || decorations & WD_TITLE), mode);
	
	state |= CS_NO_FOCUS;
	if (flags & F_DYNAMIC_PATHS)
		MCdynamiccard = curcard;
		
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

	if (mode == WM_PULLDOWN || mode == WM_POPUP || mode == WM_CASCADE || (mode == WM_OPTION && MClook != LF_WIN95))
	{
		if (menuheight == 0)
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


	state |= CS_ISOPENING;
	setgeom();
	state &= ~CS_ISOPENING;
	
	// MW-2011-08-18: [[ Redraw ]] Use global screen lock
	MCRedrawUnlockScreen();
	
	if (curcard == startcard)
	{
		
		// MW-2007-09-11: [[ Bug 5139 ]] Don't add activity to recent cards if the stack is an
		//   IDE stack.
		if ((mode == WM_TOP_LEVEL || mode == WM_TOP_LEVEL_LOCKED) && !getextendedstate(ECS_IDE))
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
			configure(window != DNULL ? False : True);
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
		if ((mode == WM_MODAL || mode == WM_SHEET) &&
			MCModeMakeLocalWindows())
		{
			// If opening the dialog failed for some reason, this will return false.
			if (mode_openasdialog())
			{
				while (opened && (mode == WM_MODAL || mode == WM_SHEET) && !MCquit)
				{
					MCU_resetprops(True);
					// MW-2011-09-08: [[ Redraw ]] Make sure we flush any updates.
					MCRedrawUpdateScreen();
					MCscreen->siguser();
					MCscreen->wait(REFRESH_INTERVAL, True, True);
				}
				mode_closeasdialog();
				if (MCquit)
					MCabortscript = False;
			}

			// Make sure the mode is reset to closed so dialogs can be reopened.
			mode = WM_CLOSED;
			
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
		MCU_restoreprops(sp);
	if (reopening)
		MClockmessages = oldlock;
	return ES_NORMAL;
	
	
}

void MCStack::getstackfiles(MCExecPoint &ep)
{
	ep.clear();
	if (nstackfiles != 0)
	{
		uint2 i;
		for (i = 0 ; i < nstackfiles ; i++)
		{
			ep.concatcstring(stackfiles[i].stackname, EC_RETURN, i == 0);
			ep.concatcstring(stackfiles[i].filename, EC_COMMA, false);
		}
	}
}

void MCStack::stringtostackfiles(char *d, MCStackfile **sf, uint2 &nf)
{
	MCStackfile *newsf = NULL;
	uint2 nnewsf = 0;
	char *eptr = d;
	while ((eptr = strtok(eptr, "\n")) != NULL)
	{
		char *cptr = strchr(eptr, ',');
		if (cptr != NULL)
		{
			*cptr++ = '\0';
			MCU_realloc((char **)&newsf, nnewsf, nnewsf + 1, sizeof(MCStackfile));
			newsf[nnewsf].stackname = strclone(eptr);
			newsf[nnewsf].filename = strclone(cptr);
			nnewsf++;
		}
		eptr = NULL;
	}
	*sf = newsf;
	nf = nnewsf;
}

void MCStack::setstackfiles(const MCString &s)
{
	while (nstackfiles--)
	{
		delete stackfiles[nstackfiles].stackname;
		delete stackfiles[nstackfiles].filename;
	}
	delete stackfiles;
	char *d = s.clone();
	stringtostackfiles(d, &stackfiles, nstackfiles);
	delete d;
}

char *MCStack::getstackfile(const MCString &s)
{
	if (stackfiles != NULL)
	{
		uint2 i;
		for (i = 0 ; i < nstackfiles ; i++)
			if (s == stackfiles[i].stackname)
			{
				if (filename == NULL || stackfiles[i].filename[0] == '/' || stackfiles[i].filename[1] == ':')
					return strclone(stackfiles[i].filename);

				// OK-2007-11-13 : Fix for crash caused by strcpy writing over sptr. sptr extended by 1 byte to cover null termination char.
				char *sptr = new char[strlen(filename) + strlen(stackfiles[i].filename) + 1];
				strcpy(sptr, filename);
				char *eptr = strrchr(sptr, PATH_SEPARATOR);

				if (eptr == NULL)
					eptr = sptr;
				else
					eptr++;

				strcpy(eptr, stackfiles[i].filename);
				return sptr;
			}
	}
	return NULL;
}

void MCStack::setfilename(char *f)
{
	delete filename;
	filename = f;
	if (f != NULL)
		MCU_fix_path(filename);
}

void MCStack::loadwindowshape()
{
	// MW-2009-07-22: We only set the windowShape *if* the window 'needstoopen'.
	if (windowshapeid)
	{
		destroywindowshape(); //just in case
		
#if defined(_DESKTOP)
		// MW-2009-02-02: [[ Improved image search ]]
		// Search for the appropriate image object using the standard method.
		MCImage *iptr;
		iptr = resolveimageid(windowshapeid);
		if (iptr != NULL)
		{
			iptr->setflag(True, F_I_ALWAYS_BUFFER);
			iptr->open();

			m_window_shape = iptr -> makewindowshape();
			if (m_window_shape != nil)
				setextendedstate(True, ECS_MASK_CHANGED);

			iptr->close();
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

	// Make sure the dirty rect falls within the card bounds.
	MCRectangle t_actual_dirty_rect;
	t_actual_dirty_rect = MCU_intersect_rect(curcard -> getrect(), p_dirty_rect);
	if (MCU_empty_rect(t_actual_dirty_rect))
		return;

	// Make sure the dirty rect falls within the windowshape bounds.
	if (m_window_shape != nil)
	{
		t_actual_dirty_rect = MCU_intersect_rect(MCU_make_rect(0, 0, m_window_shape -> width, m_window_shape -> height), t_actual_dirty_rect);
		if (MCU_empty_rect(t_actual_dirty_rect))
			return;
	}

	// If there is no region yet, make one.
	if (m_update_region == nil)
		/* UNCHECKED */ MCRegionCreate(m_update_region);

	// If we've not previously logged a redraw, we can just set a rect, else we
	// need to do a union.
	if (!getstate(CS_NEED_REDRAW))
		MCRegionSetRect(m_update_region, t_actual_dirty_rect);
	else
		/* UNCHECKED */ MCRegionIncludeRect(m_update_region, t_actual_dirty_rect);

	// Mark the stack as needing a redraw and schedule an update.
	setstate(True, CS_NEED_REDRAW);
	MCRedrawScheduleUpdateForStack(this);
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
	if (m_tilecache != nil)
		MCTileCacheFlush(m_tilecache);

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

	// Defer to the rect dirtying routine to update the update region.
	dirtyrect(curcard -> getrect());
}

void MCStack::dirtywindowname(void)
{
	state |= CS_TITLE_CHANGED;
	delete titlestring;
	titlestring = NULL;

	MCRedrawScheduleUpdateForStack(this);
}

#ifndef TARGET_SUBPLATFORM_IPHONE

// OpenGL stuff must be done on the system thread, so the iOS version of this handler
// does it on a block.
void MCStack::updatetilecache(void)
{
	if (m_tilecache == nil)
		return;

	// If the tilecache is not valid, flush it.
	if (!MCTileCacheIsValid(m_tilecache))
		MCTileCacheFlush(m_tilecache);

	MCTileCacheBeginFrame(m_tilecache);
	curcard -> render();
	MCTileCacheEndFrame(m_tilecache);
}

// MW-2013-06-26: [[ Bug 10990 ]] For non-iOS, we can run the MCTileCacheDeactivate
//   method on the main thread.
void MCStack::deactivatetilecache(void)
{
    if (m_tilecache == nil)
        return;
    
    MCTileCacheDeactivate(gettilecache());
}

bool MCStack::snapshottilecache(MCRectangle p_area, MCGImageRef& r_pixmap)
{
	if (m_tilecache == nil)
		return false;
	
	// IM-2013-08-21: [[ ResIndependence ]] Use device coords for tilecache operation
	MCRectangle t_device_rect;
	t_device_rect = MCGRectangleGetIntegerBounds(MCResUserToDeviceRect(p_area));
	return MCTileCacheSnapshot(m_tilecache, t_device_rect, r_pixmap);
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

	// Ensure the content is up to date.
	if (getstate(CS_NEED_REDRAW))
	{
		// MW-2012-04-20: [[ Bug 10185 ]] Only update if there is a window to update.
		//   (we can get here if a stack has its window taken over due to go in window).
		if (window != nil)
		{
#ifdef _IOS_MOBILE
			// MW-2013-03-20: [[ Bug 10748 ]] We defer switching the display class on iOS as
			//   it causes flashes when switching between stacks.
			extern void MCIPhoneSyncDisplayClass(void);
			MCIPhoneSyncDisplayClass();
#endif
		
			// MW-2011-09-08: [[ TileCache ]] If we have a tilecache, then attempt to update
			//   it.
			if (m_tilecache != nil)
				updatetilecache();

			// MW-2011-09-08: [[ TileCache ]] Perform a redraw of the window within
			//   the update region.
			updatewindow(m_update_region);

			// Clear the update region.
			MCRegionSetEmpty(m_update_region);
		}

		// We no longer need to redraw.
		setstate(False, CS_NEED_REDRAW);
	}

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
	MCGraphicsContext *t_old_context = nil;
	t_old_context = new MCGraphicsContext(p_context);
	if (t_old_context != nil)
		render(t_old_context, p_rect);
	delete t_old_context;
}

void MCStack::device_redrawwindow(MCStackSurface *p_surface, MCRegionRef p_region)
{
	if (m_tilecache == nil || !MCTileCacheIsValid(m_tilecache))
	{
		// If there is no tilecache, or the tilecache is invalid then fetch an
		// MCContext for the surface and render.
		MCGContextRef t_context = nil;
		if (p_surface -> LockGraphics(p_region, t_context))
		{
			// p_region is in device coordinates, translate to user-space coords & ensure any fractional pixels are accounted for
			MCGRectangle t_user_rect;
			t_user_rect = MCGRectangleToUserSpace(MCRectangleToMCGRectangle(MCRegionGetBoundingBox(p_region)));
			
			MCRectangle t_rect;
			t_rect = MCGRectangleGetIntegerBounds(t_user_rect);
			
			// scale user -> device space
			MCGFloat t_scale;
			t_scale = MCResGetDeviceScale();
			MCGContextScaleCTM(t_context, t_scale, t_scale);
			
			render(t_context, t_rect);
			
			p_surface -> UnlockGraphics();
		}
	}
	else
	{
		// We have a valid tilecache, so get it to composite.
		MCTileCacheComposite(m_tilecache, p_surface, p_region);
	}
}

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
	
	if (m_tilecache != nil && getstate(CS_NEED_REDRAW))
	{
		updatetilecache();
		setstate(False, CS_NEED_REDRAW);
	}
	
	if (!snapshottilecache(t_effect_area, m_snapshot))
	{	
		bool t_success = true;
		// Render the current content.
		MCGContextRef t_context = nil;
		MCContext *t_gfxcontext = nil;

		// IM-2013-07-18: [[ ResIndependence ]] take stack snapshot at device resolution
		// IM-2013-08-21: [[ ResIndependence ]] Align snapshots to device pixel boundaries
		MCRectangle t_device_rect = MCGRectangleGetIntegerBounds(MCResUserToDeviceRect(t_effect_area));
		MCRectangle t_user_rect = MCGRectangleGetIntegerBounds(MCResDeviceToUserRect(t_device_rect));
		
		if (t_success)
			t_success = MCGContextCreate(t_device_rect.width, t_device_rect.height, true, t_context);

		if (t_success)
		{
			MCGContextTranslateCTM(t_context, -t_device_rect.x, -t_device_rect.y);
			
			MCGFloat t_scale;
			t_scale = MCResGetDeviceScale();
			
			MCGContextScaleCTM(t_context, t_scale, t_scale);
			
			t_success = nil != (t_gfxcontext = new MCGraphicsContext(t_context));
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
    if (menuheight > rect . height || menuy != 0)
        clipmenu(p_context, t_clipped_visible);
    
    curcard -> draw(p_context, t_clipped_visible, false);
}

////////////////////////////////////////////////////////////////////////////////

void MCStack::updatewindow(MCRegionRef p_region)
{
	MCRectangle t_update_rect;
	t_update_rect = MCRegionGetBoundingBox(p_region);
	
	// IM-2013-08-01: [[ ResIndependence ]] Scale update region to device coords
	MCRegionRef t_dev_region;
	t_dev_region = nil;
	/* UNCHECKED */ MCRegionCreate(t_dev_region);
	MCRegionSetRect(t_dev_region, MCGRectangleGetIntegerBounds(MCResUserToDeviceRect(t_update_rect)));
	
	device_updatewindow(t_dev_region);
	
	MCRegionDestroy(t_dev_region);
}

////////////////////////////////////////////////////////////////////////////////

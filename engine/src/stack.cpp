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

#include "execpt.h"
#include "dispatch.h"
#include "stack.h"
#include "stacklst.h"
#include "cardlst.h"
#include "sellst.h"
#include "undolst.h"
#include "card.h"
#include "aclip.h"
#include "vclip.h"
#include "control.h"
#include "image.h"
#include "button.h"
#include "mcerror.h"
#include "handler.h"
#include "hndlrlst.h"
#include "player.h"
#include "param.h"
#include "debug.h"
#include "util.h"
#include "date.h"
#include "parentscript.h"
#include "group.h"
#include "eventqueue.h"
#include "securemode.h"
#include "osspec.h"
#include "region.h"
#include "redraw.h"
#include "globals.h"
#include "license.h"
#include "mode.h"
#include "tilecache.h"
#include "font.h"
#include "external.h"

#ifdef _MOBILE
#include "mbldc.h"
#endif

#include "resolution.h"

static int4 s_last_stack_time = 0;
static int4 s_last_stack_index = 0;

uint2 MCStack::ibeam;

MCStack::MCStack()
{
	obj_id = START_ID;
	flags = F_VISIBLE | F_RESIZABLE | F_OPAQUE;
	window = DNULL;
	parentwindow = DNULL;
	cursor = None;
	substacks = NULL;
	cards = curcard = savecards = NULL;
	controls = NULL;
	editing = NULL;
	aclips = NULL;
	vclips = NULL;
	backgroundid = 0;
	state = 0;
	iconid = 0;
	rect.x = rect.y = 0;
	rect.width = MCminsize << 5;
	rect.height = MCminsize << 5;
	title = NULL;
	titlestring = NULL;
	minwidth = MCminstackwidth;
	minheight = MCminstackheight;
	maxwidth = MAXUINT2;
	maxheight = MAXUINT2;
	externalfiles = NULL;
	idlefunc = NULL;
	windowshapeid = 0;

	old_blendlevel = 100;

	nfuncs = 0;
	nmnemonics = 0;
	lasty = 0;
	mnemonics = NULL;
	nneeds = 0;
	needs = NULL;
	mode = WM_CLOSED;
	decorations = WD_CLEAR;
	nstackfiles = 0;
	stackfiles = NULL;
	linkatts = NULL;
	filename = NULL;
	/* UNCHECKED */ MCNameClone(kMCEmptyName, _menubar);
	menuy = menuheight = 0;
	menuwindow = False;

	f_extended_state = 0;
	m_externals = NULL;

	// MW-2011-08-19: [[ Redraw ]] Initialize the stack's update region
	m_update_region = nil;

	// MW-2011-08-26: [[ TileCache ]] Stacks start off with no tilecache.
	m_tilecache = nil;
	
	// MW-2011-09-12: [[ MacScroll ]] There is no scroll to start with.
	m_scroll = 0;

	// MW-2011-09-13: [[ Effects ]] No snapshot to begin with.
	m_snapshot = nil;
	
	// MW-2011-09-13: [[ Masks ]] The window mask starts off as nil.
	m_window_shape = nil;
	
	// MW-2011-11-24: [[ UpdateScreen ]] Start off with defer updates false.
	m_defer_updates = false;
	
	// MW-2012-10-10: [[ IdCache ]]
	m_id_cache = nil;

	cursoroverride = false ;
	old_rect.x = old_rect.y = old_rect.width = old_rect.height = 0 ;

	mode_create();
}

MCStack::MCStack(const MCStack &sref) : MCObject(sref)
{
	obj_id = sref.obj_id;

	// MW-2007-07-05: [[ Bug 3491 ]] Creating unnamed stacks in quick succession gives them the same name
	if (isunnamed())
	{
		int4 t_time;
		t_time = (int4)MCS_time();
		if (t_time == s_last_stack_time)
		{
			char t_name[U4L * 2 + 7];
			sprintf(t_name, "Stack %d.%d", t_time, s_last_stack_index);
			setname_cstring(t_name);
			s_last_stack_index += 1;
		}
		else
		{
			char t_name[U4L + 7];
			sprintf(t_name, "Stack %d", t_time);
			setname_cstring(t_name);
			s_last_stack_time = t_time;
			s_last_stack_index = 2;
		}
	}
	window = DNULL;
	parentwindow = DNULL;
	cursor = None;
	substacks = NULL;
	cards = curcard = savecards = NULL;
	controls = NULL;
	editing = NULL;
	aclips = NULL;
	vclips = NULL;
	backgroundid = 0;
	iconid = 0;
	title = strclone(sref.title);
	titlestring = NULL;
	minwidth = sref.minwidth;
	minheight = sref.minheight;
	maxwidth = sref.maxwidth;
	maxheight = sref.maxheight;
	externalfiles = strclone(sref.externalfiles);
	idlefunc = NULL;
	windowshapeid = sref.windowshapeid;

	old_blendlevel = sref . blendlevel;

	menuwindow = sref . menuwindow;

	// MW-2011-09-12: [[ MacScroll ]] Make sure the rect refers to the unscrolled size.
	rect . height += sref . getscroll();
	
	// MW-2012-10-10: [[ IdCache ]]
	m_id_cache = nil;
	
	mnemonics = NULL;
	nfuncs = 0;
	nmnemonics = 0;
	lasty = sref.lasty;
	if (sref.controls != NULL)
	{
		MCControl *optr = sref.controls;
		do
		{
			// MW-2011-08-08: We are cloning a stack so want to copy ids from
			//   the original objects, thus use 'doclone' if we are cloning
			//   a group.
			MCControl *newcontrol;
			if (optr -> gettype() == CT_GROUP)
			{
				MCGroup *t_group;
				t_group = (MCGroup *)optr;
				newcontrol = t_group -> doclone(False, OP_NONE, true, false);
			}
			else
				newcontrol = optr -> clone(False, OP_NONE, false);

			newcontrol->setid(optr->getid());
			newcontrol->appendto(controls);
			newcontrol->setparent(this);
			optr = optr->next();
		}
		while (optr != sref.controls);
	}
	if (sref.cards != NULL)
	{
		MCCard *cptr = sref.cards;
		do
		{
			MCCard *newcard = cptr->clone(False, False);
			newcard->setid(cptr->getid());
			newcard->appendto(cards);
			newcard->setparent(this);
			newcard->clonedata(cptr);
			newcard->replacedata(NULL);
			cptr = cptr->next();
		}
		while (cptr != sref.cards);
		curcard = cards;
	}
	if (sref.aclips != NULL)
	{
		MCAudioClip *aptr = sref.aclips;
		do
		{
			MCAudioClip *newaclip = new MCAudioClip(*aptr);
			newaclip->setid(aptr->getid());
			newaclip->appendto(aclips);
			newaclip->setparent(this);
			aptr = aptr->next();
		}
		while (aptr != sref.aclips);
	}
	if (sref.vclips != NULL)
	{
		MCVideoClip *vptr = sref.vclips;
		do
		{
			MCVideoClip *newvclip = new MCVideoClip(*vptr);
			newvclip->setid(vptr->getid());
			newvclip->appendto(vclips);
			newvclip->setparent(this);
			vptr = vptr->next();
		}
		while (vptr != sref.vclips);
	}
	nneeds = 0;
	needs = NULL;
	mode = WM_CLOSED;
	decorations = sref.decorations;
	nstackfiles = sref.nstackfiles;
	if (nstackfiles != 0)
	{
		uint2 ts = nstackfiles;
		stackfiles = new MCStackfile[ts];
		while (ts--)
		{
			stackfiles[ts].stackname = strclone(sref.stackfiles[ts].stackname);
			stackfiles[ts].filename = strclone(sref.stackfiles[ts].filename);
		}
	}
	else
		stackfiles = NULL;
	if (sref.linkatts != NULL)
	{
		linkatts = new Linkatts;
		memcpy(linkatts, sref.linkatts, sizeof(Linkatts));
		linkatts->colorname = strclone(sref.linkatts->colorname);
		linkatts->hilitecolorname = strclone(sref.linkatts->hilitecolorname);
		linkatts->visitedcolorname = strclone(sref.linkatts->visitedcolorname);
	}
	else
		linkatts = NULL;
	filename = NULL;
	/* UNCHECKED */ MCNameClone(sref._menubar, _menubar);
	menuy = menuheight = 0;

	f_extended_state = 0;

	m_externals = NULL;

	// MW-2011-08-19: [[ Redraw ]] Initialize the stack's update region
	m_update_region = nil;

	// MW-2011-08-26: [[ TileCache ]] Stacks start off with no tilecache.
	m_tilecache = nil;
	
	// MW-2011-09-12: [[ MacScroll ]] There is no scroll to start with.
	m_scroll = 0;
	
	// MW-2011-09-13: [[ Effects ]] No snapshot to begin with.
	m_snapshot = nil;
	
	// MW-2011-09-13: [[ Masks ]] The windowmask starts off as nil.
	m_window_shape = nil;

	// MW-2011-11-24: [[ UpdateScreen ]] Start off with defer updates false.
	m_defer_updates = false;
	
	// MW-2010-11-17: [[ Valgrind ]] Uninitialized value.
	cursoroverride = false;

	mode_copy(sref);
}

MCStack::~MCStack()
{
	mode_destroy();

	flags &= ~F_DESTROY_STACK;
	state |= CS_DELETE_STACK;
	while (opened)
		close();
	extraclose(false);

	if (needs != NULL)
	{
		while (nneeds--)

			needs[nneeds]->removelink(this);
		delete needs;
	}
	if (substacks != NULL)
	{
		while (substacks != NULL)
		{
			MCStack *sptr = substacks->remove
			                (substacks);
			delete sptr;
		}
		opened++;
		MCObject::close();
	}
	if (parentwindow != DNULL)
		setparentwindow(DNULL);
	delete mnemonics;
	delete title;
	delete titlestring;

	if (window != DNULL && !(state & CS_FOREIGN_WINDOW))
	{
		stop_externals();
		MCscreen->destroywindow(window);
	}

	while (controls != NULL)
	{
		MCControl *cptr = controls->remove
		                  (controls);
		delete cptr;
	}
	while (aclips != NULL)
	{
		MCAudioClip *aptr = aclips->remove
		                    (aclips);
		delete aptr;
	}
	while (vclips != NULL)
	{
		MCVideoClip *vptr = vclips->remove
		                    (vclips);
		delete vptr;
	}
	MCrecent->deletestack(this);
	MCcstack->deletestack(this);
	while (cards != NULL)
	{
		MCCard *cptr = cards->remove
		               (cards);
		delete cptr;
	}
	delete externalfiles;

	uint2 i = 0;
	while (i < MCnusing)
		if (MCusing[i] == this)
		{
			MCnusing--;
			uint2 j;
			for (j = i ; j < MCnusing ; j++)
				MCusing[j] = MCusing[j + 1];
		}
		else
			i++;
	// MW-2004-11-17: If this is the current Message Box, set to NULL
	if (MCmbstackptr == this)
		MCmbstackptr = NULL;
	if (MCstaticdefaultstackptr == this)
		MCstaticdefaultstackptr = MCtopstackptr;
	if (MCdefaultstackptr == this)
		MCdefaultstackptr = MCstaticdefaultstackptr;
	if (stackfiles != NULL)
	{
		while (nstackfiles--)
		{
			delete stackfiles[nstackfiles].stackname;
			delete stackfiles[nstackfiles].filename;
		}
		delete stackfiles;
	}
	if (linkatts != NULL)
	{
		delete linkatts->colorname;
		delete linkatts->hilitecolorname;
		delete linkatts->visitedcolorname;
		delete linkatts;
	}
	delete filename;

	MCNameDelete(_menubar);

	unloadexternals();

	MCEventQueueFlush(this);

	// MW-2011-08-19: [[ Redraw ]] Destroy the stack's update region.
	MCRegionDestroy(m_update_region);

	// MW-2011-08-26: [[ TileCache ]] Destroy the stack's tilecache - if any.
	MCTileCacheDestroy(m_tilecache);
	
	// MW-2011-09-13: [[ Redraw ]] If there is snapshot, get rid of it.
	MCGImageRelease(m_snapshot);
	m_snapshot = nil;
	
	// MW-2012-10-10: [[ IdCache ]] Free the idcache.
	freeobjectidcache();
}

Chunk_term MCStack::gettype() const
{
	return CT_STACK;
}

const char *MCStack::gettypestring()
{
	return MCstackstring;
}

bool MCStack::visit(MCVisitStyle p_style, uint32_t p_part, MCObjectVisitor* p_visitor)
{
	bool t_continue;
	t_continue = true;

	if (p_style == VISIT_STYLE_DEPTH_LAST)
		t_continue = p_visitor -> OnStack(this);

	if (t_continue && cards != nil)
	{
		MCCard *t_card;
		t_card = cards;
		do
		{
			t_continue = p_visitor -> OnCard(t_card);
			t_card = t_card -> next();
		}
		while(t_continue && t_card != cards);
	}

	if (t_continue && controls != nil)
	{
		MCControl *t_control;
		t_control = controls;
		do
		{
			t_continue = t_control -> visit(p_style, 0, p_visitor);
			t_control = t_control -> next();
		}
		while(t_continue && t_control != controls);
	}

	if (p_style == VISIT_STYLE_DEPTH_FIRST)
		t_continue = p_visitor -> OnStack(this);

	return true;
}

void MCStack::open()
{
	openrect(rect, WM_LAST, NULL, WP_DEFAULT,OP_NONE);
}

void MCStack::close()
{
	if (!opened)
		return;
				
	if (menuheight && (rect.height != menuheight || menuy != 0))
	{
		if (menuy != 0)
			scrollmenu(-menuy, False);
		minheight = maxheight = rect.height = menuheight;
	}
	if (state & CS_IGNORE_CLOSE)
	{
		state &= ~(CS_IGNORE_CLOSE);

		return;
	}
	if (editing != NULL)
		stopedit();
	state |= CS_IGNORE_CLOSE;
	
	// MW-2008-10-31: [[ ParentScripts ]] Send closeControl messages as appropriate.
	curcard -> closecontrols();
	curcard->message(MCM_close_card);
	curcard -> closebackgrounds(NULL);
	curcard->message(MCM_close_stack);
	
	state &= ~CS_SUSPENDED;
	
	curcard->close();
	
	// MW-2011-09-12: [[ MacScroll ]] Clear the current scroll setting from the stack.
	clearscroll();
	
	if (MCacptr != NULL && MCacptr->getmessagestack() == this)
		MCacptr->setmessagestack(NULL);
	if (state & CS_ICONIC)
	{
		MCiconicstacks--;
		state &= ~CS_ICONIC;
	}
	if (MCmousestackptr == this)
	{
		MCmousestackptr = NULL;
		int2 x, y;
		MCscreen->querymouse(x, y);
		if (MCU_point_in_rect(curcard->getrect(), x, y))
		{
			ibeam = 0;
		}
	}
	if (MCclickstackptr == this)
		MCclickstackptr = NULL;
	if (MCfocusedstackptr == this)
		MCfocusedstackptr = NULL;
	if (!(state & CS_ICONIC))
		MCstacks->remove(this);
	if (window != DNULL && !(state & CS_FOREIGN_WINDOW))
	{
		MCscreen->closewindow(window);
		if (mode == WM_MODAL || mode == WM_SHEET)
			MCscreen->closemodal();
		if (iconid != 0)
		{
			MCImage *iptr = (MCImage *)getobjid(CT_IMAGE, iconid);
			if (iptr != NULL)
				iptr->close();
		}

		MCscreen->flush(window);

		if (flags & F_DESTROY_WINDOW && MCdispatcher -> gethome() != this)
		{
			stop_externals();
			MCscreen->destroywindow(window);
			window = DNULL;
			cursor = None;
			delete titlestring;
			titlestring = NULL;
			state &= ~CS_BEEN_MOVED;
		}
	}
	if (substacks == NULL)
		MCObject::close();
	else if (opened != 0)
		opened--;

	// MW-2011-09-13: [[ Effects ]] Free any snapshot that we have.
	MCGImageRelease(m_snapshot);
	m_snapshot = nil;
	
	state &= ~(CS_IGNORE_CLOSE | CS_KFOCUSED | CS_ISOPENING);
}

void MCStack::kfocus()
{
	if (!opened || state & CS_IGNORE_CLOSE)
		return;
	MCstacks->top(this);
	MCfocusedstackptr = this;

	// MW-2007-09-11: [[ Bug 5139 ]] Don't add activity to recent cards if the stack is an
	//   IDE stack.
	if ((mode == WM_TOP_LEVEL || mode == WM_TOP_LEVEL_LOCKED) && editing == NULL && !getextendedstate(ECS_IDE))
		MCrecent->addcard(curcard);

	if (state & CS_SUSPENDED)
	{
		curcard->message(MCM_resume_stack);
		state &= ~CS_SUSPENDED;
	}
	if (!(state & CS_KFOCUSED))
	{
		state |= CS_KFOCUSED;
		if (gettool(this) == T_BROWSE)
			curcard->kfocus();
		updatemenubar();
		if (hashandlers & HH_IDLE)
			MCscreen->addtimer(this, MCM_idle, MCidleRate);
		if (curcard->gethashandlers() & HH_IDLE)
			MCscreen->addtimer(curcard, MCM_idle, MCidleRate);
	}
}

Boolean MCStack::kfocusnext(Boolean top)
{
	if (!opened || flags & F_CANT_MODIFY || gettool(this) != T_BROWSE)
		return False;
	Boolean done = curcard->kfocusnext(top);
	if (menuheight && (rect.height != menuheight || menuy != 0))
		scrollintoview();
	return done;
}

Boolean MCStack::kfocusprev(Boolean bottom)
{
	if (!opened || flags & F_CANT_MODIFY || gettool(this) != T_BROWSE)
		return False;
	Boolean done = curcard->kfocusprev(bottom);
	if (menuheight && (rect.height != menuheight || menuy != 0))
		scrollintoview();
	return done;
}


void MCStack::kunfocus()
{
	if (!(state & CS_KFOCUSED))
		return;
	if (MCfocusedstackptr == this)
		MCfocusedstackptr = NULL;
	state &= ~CS_KFOCUSED;
	if (!opened)
		return;
	curcard->message(MCM_suspend_stack);
	state |= CS_SUSPENDED;
	curcard->kunfocus();
}

Boolean MCStack::kdown(const char *string, KeySym key)
{
	if (!opened || state & CS_IGNORE_CLOSE)
		return False;
	if (curcard->kdown(string, key))
		return True;
	MCObject *optr;
	switch (key)
	{
	case XK_Delete:
		if (MCmodifierstate & MS_MOD1)
			return MCundos->undo();
		if (MCmodifierstate & MS_SHIFT)
			if (MCactiveimage != NULL)
			{
				MCactiveimage->cutimage();
				return True;
			}
			else
				return MCselected->cut();
		if (MCactiveimage != NULL)
		{
			MCactiveimage->delimage();
			return True;
		}
		return MCselected->del();
	case XK_BackSpace:
		if (MCmodifierstate & MS_MOD1)
			return MCundos->undo();
		if (MCactiveimage != NULL)
		{
			MCactiveimage->delimage();
			return True;
		}
		return MCselected->del();
	case XK_osfUndo:
		return MCundos->undo();
	case XK_osfCut:
		if (MCactiveimage != NULL)
		{
			MCactiveimage->cutimage();
			return True;
		}
		else
			return MCselected->cut();
	case XK_osfCopy:
		if (MCactiveimage != NULL)
		{
			MCactiveimage->copyimage();
			return True;
		}
		else
			return MCselected->copy();
	case XK_osfPaste:
		MCdefaultstackptr = MCtopstackptr;
		return MCdispatcher -> dopaste(optr);
	case XK_Insert:
		if (MCmodifierstate & MS_SHIFT)
		{
			MCdefaultstackptr = MCtopstackptr;
			return MCdispatcher -> dopaste(optr);
		}
		if (MCmodifierstate & MS_CONTROL)
			if (MCactiveimage != NULL)
			{
				MCactiveimage->copyimage();
				return True;
			}
		return MCselected->copy();
	case XK_Left:
		if (mode >= WM_PULLDOWN || !MCnavigationarrows)
			return False;
		if (MCmodifierstate & MS_CONTROL)
			setcard(getstack()->getchild(CT_FIRST, MCnullmcstring, CT_CARD), True, False);
		else
			setcard(getstack()->getchild(CT_PREV, MCnullmcstring, CT_CARD), True, False);
		return True;
	case XK_Right:
		if (mode >= WM_PULLDOWN || !MCnavigationarrows)
			return False;
		if (MCmodifierstate & MS_CONTROL)
			setcard(getstack()->getchild(CT_LAST, MCnullmcstring, CT_CARD), True, False);
		else
			setcard(getstack()->getchild(CT_NEXT, MCnullmcstring, CT_CARD), True, False);
		return True;
	case XK_Up:
		if (mode >= WM_PULLDOWN || !MCnavigationarrows)
			return False;
		if (MCmodifierstate & MS_CONTROL)
			MCrecent->godirect(True);
		else
			MCrecent->gorel(-1);
		return True;
	case XK_Down:
		if (mode >= WM_PULLDOWN || !MCnavigationarrows)
			return False;
		if (MCmodifierstate & MS_CONTROL)
			MCrecent->godirect(False);
		else
			MCrecent->gorel(1);
		return True;
	case XK_Return:
	case XK_KP_Enter:
		if (!(MCmodifierstate & (MS_MOD1 | MS_CONTROL)))
		{
			MCButton *bptr = curcard->getdefbutton();

			if (bptr != NULL)
			{
				bptr->activate(False, 0);
				return True;
			}
		}
		break;
	default:
		break;
	}
	if (MClook != LF_MOTIF && MCmodifierstate & MS_CONTROL)
		switch (key)
		{
		case XK_C:
		case XK_c:
			if (MCactiveimage != NULL)
			{
				MCactiveimage->copyimage();
				return True;
			}
			return MCselected->copy();
		case XK_V:
		case XK_v:
			MCdefaultstackptr = MCtopstackptr;
			return MCdispatcher -> dopaste(optr);
		case XK_X:

		case XK_x:

			if (MCactiveimage != NULL)
			{
				MCactiveimage->cutimage();
				return True;
			}
			return MCselected->cut();
		case XK_Z:
		case XK_z:
			return MCundos->undo();
		}
	uint2 i;
	for (i = 0 ; i < nmnemonics ; i++)
	{
		if (mnemonics[i].key == MCS_tolower(string[0])
		        && mnemonics[i].button->isvisible()
		        && !mnemonics[i].button->isdisabled())
		{
			mnemonics[i].button->activate(True, string[0]);
			return True;
		}
	}

	return False;
}

Boolean MCStack::kup(const char *string, KeySym key)
{
	if (!opened || state & CS_IGNORE_CLOSE)
		return False;
	Boolean done = curcard->kup(string, key);
	if (menuheight && (rect.height != menuheight || menuy != 0))
		scrollintoview();
	return done;
}

Boolean MCStack::mfocus(int2 x, int2 y)
{
	if (!opened || state & CS_IGNORE_CLOSE)
		return False;
	//XCURSORS
	if ( !cursoroverride )
		setcursor(getcursor(), False);
	if (menuheight && (rect.height != menuheight || menuy != 0))
	{
		MCControl *cptr = curcard->getmfocused();
		if (x < rect.width || cptr != NULL && !cptr->getstate(CS_SUBMENU))
		{
			uint1 oldmode = scrollmode;
			if (menuy < 0 && y < MENU_ARROW_SIZE >> 1)
			{
				if (y < 0)
					scrollmode = SM_PAGEDEC;
				else
					scrollmode = SM_LINEDEC;
				if (oldmode == SM_CLEARED)
					timer(MCM_internal, NULL);
				x = y = -MAXINT2;
			}
			else
				if (menuy + menuheight > rect.height
				        && y > rect.height - MENU_ARROW_SIZE)
				{
					if (y > rect.height)
						scrollmode = SM_PAGEINC;

					else
						scrollmode = SM_LINEINC;
					if (oldmode == SM_CLEARED)
						timer(MCM_internal, NULL);
					x = y = -MAXINT2;
				}
				else
					if (scrollmode != SM_CLEARED)
					{
						MCscreen->cancelmessageobject(curcard, MCM_internal);
						scrollmode = SM_CLEARED;
					}
		}
	}
	return curcard->mfocus(x, y);
}

void MCStack::mfocustake(MCControl *target)
{
	if (!opened || state & CS_IGNORE_CLOSE)
		return;
	curcard->mfocustake(target);
}

void MCStack::munfocus(void)
{
	if (MCmousestackptr == this)
		MCmousestackptr = NULL;
	if (curcard != 0)
	{
		if (gettool(this) != T_SELECT)
			curcard->munfocus();
		if (ibeam != 0 && !curcard->getgrab())
			ibeam = 0;
	}
}

void MCStack::mdrag(void)
{
	if (!opened || state & CS_IGNORE_CLOSE)
		return;
	curcard -> mdrag();
}

Boolean MCStack::mdown(uint2 which)
{
	if (!opened || state & CS_IGNORE_CLOSE)
		return False;
	return curcard->mdown(which);
}

Boolean MCStack::mup(uint2 which)
{
	if (!opened || state & CS_IGNORE_CLOSE)
		return False;
	Boolean handled = curcard->mup(which);
	// MW-2010-07-06: [[ Bug ]] We should probably only mfocus the card if this
	//   stack is still the mouse stack.
	if (opened && mode < WM_PULLDOWN && MCmousestackptr == this)
		curcard->mfocus(MCmousex, MCmousey);
	return handled;
}

Boolean MCStack::doubledown(uint2 which)
{
	if (!opened || state & CS_IGNORE_CLOSE)
		return False;
	return curcard->doubledown(which);
}

Boolean MCStack::doubleup(uint2 which)
{
	if (!opened || state & CS_IGNORE_CLOSE)
		return False;
	return curcard->doubleup(which);
}

void MCStack::timer(MCNameRef mptr, MCParameter *params)
{
	if (MCNameIsEqualTo(mptr, MCM_internal, kMCCompareCaseless))
	{
		if (scrollmode == SM_PAGEDEC || scrollmode == SM_LINEDEC)
		{
			int2 newoffset = controls->getrect().height;
			if (-menuy < newoffset)
			{
				newoffset = -menuy;
				scrollmode = SM_CLEARED;
			}
			scrollmenu(newoffset, True);
		}
		else if (scrollmode == SM_PAGEINC || scrollmode == SM_LINEINC)
			{
				int2 newoffset = -controls->getrect().height;
				if (menuy + menuheight + newoffset < rect.height)
				{
					newoffset = rect.height - menuy - menuheight;
					scrollmode = SM_CLEARED;
				}
				scrollmenu(newoffset, True);
			}
		if (scrollmode == SM_PAGEINC || scrollmode == SM_PAGEDEC)
			MCscreen->addtimer(this, MCM_internal, MCsyncrate);
		else
			if (scrollmode == SM_LINEINC || scrollmode == SM_LINEDEC)
				MCscreen->addtimer(this, MCM_internal, MCrepeatrate);
	}
	else if (MCNameIsEqualTo(mptr, MCM_idle, kMCCompareCaseless))
		{
			if (opened && hashandlers & HH_IDLE && state & CS_KFOCUSED
			        && getstack()->gettool(this) == T_BROWSE)
			{
				external_idle();
				if (message(mptr, params, True, True) == ES_ERROR)
					senderror();
				else
					MCscreen->addtimer(this, MCM_idle, MCidleRate);
			}
		}
		else
			MCObject::timer(mptr, params);
}

MCRectangle MCStack::getrectangle(bool p_effective) const
{
    if (!p_effective)
        return getrect();
    
    return getwindowrect();
}

void MCStack::setrect(const MCRectangle &nrect)
{
#ifdef _MOBILE
	// MW-2012-11-14: [[ Bug 10514 ]] If on mobile and this stack is currently
	//   being displayed, don't let its rect be set.
	if (((MCScreenDC *)MCscreen) -> get_current_window() == (Window)this)
		return;
#endif

	// MW-2012-10-04: [[ Bug 10436 ]] Make sure we constrain stack size to screen size
	//   if in fullscreen mode.
	MCRectangle t_new_rect;
	if (getextendedstate(ECS_FULLSCREEN))
	{
		const MCDisplay *t_display;
		t_display = MCscreen -> getnearestdisplay(rect);
		t_new_rect = t_display -> viewport;
	}
	else
		t_new_rect = nrect;
	
	if (rect.x != t_new_rect.x || rect.y != t_new_rect.y)
		state |= CS_BEEN_MOVED;

	MCRectangle oldrect = rect;
	rect = t_new_rect;
	
	menuy = menuheight = 0;
	if (opened && mode_haswindow())
	{
		mode_constrain(rect);
		if (mode == WM_PULLDOWN || mode == WM_OPTION)
		{
			rect.x = oldrect.x;
			rect.y = oldrect.y;
			menuheight = minheight = maxheight = rect.height;
		}

		// MW-2011-08-18: [[ Redraw ]] Update to use redraw.
		if (MCRedrawIsScreenLocked())
		{
			state |= CS_NEED_RESIZE;
			MCRedrawScheduleUpdateForStack(this);
		}
		else
			setgeom();
	}
	
	// MW-2012-10-23: [[ Bug 10461 ]] Make sure we do this *after* the stack has been resized
	//   otherwise we get cases where the card size isn't updated.
	// MW-2012-10-04: [[ Bug 10420 ]] If we have a card then make sure we sync the
	//   stack size to it.
	if (curcard != nil)
		resize(oldrect . width, oldrect . height);
}

Exec_stat MCStack::getprop(uint4 parid, Properties which, MCExecPoint &ep, Boolean effective)
{
	uint2 j = 0;
	uint2 k = 0;
	MCStack *sptr = this;
	uint2 num;

	switch (which)
	{
#ifdef /* MCStack::getprop */ LEGACY_EXEC		
	case P_FULLSCREEN:
			ep.setsvalue( MCU_btos(getextendedstate(ECS_FULLSCREEN)));
	break;
		
	case P_LONG_ID:
	case P_LONG_NAME:
		if (filename == NULL)
		{
			if (MCdispatcher->ismainstack(sptr))
			{
				if (!isunnamed())
					ep.setstringf("stack \"%s\"", getname_cstring());
				else
					ep.clear();
			}
			else if (isunnamed())
					ep.clear();
				else
					return names(P_LONG_NAME, ep, parid);
		}
		else
			ep.setstringf("stack \"%s\"", filename);
		break;
	case P_NUMBER:
		if (parent != NULL && !MCdispatcher->ismainstack(sptr))
		{
			sptr = (MCStack *)parent;
			sptr->count(CT_STACK, CT_UNDEFINED, this, num);
			ep.setint(num);
		}
		else
			ep.setint(0);
		break;
	case P_LAYER:
		ep.setint(0);
		break;
	case P_FILE_NAME:
		if (effective && !MCdispatcher->ismainstack(this))
			return parent->getprop(0, which, ep, effective);
		ep.setsvalue(filename);
		break;
	case P_SAVE_COMPRESSED:
		ep.setboolean(True);
		break;
	case P_CANT_ABORT:
		ep.setboolean(getflag(F_CANT_ABORT));
		break;
	case P_CANT_DELETE:
		ep.setboolean(getflag(F_S_CANT_DELETE));
		break;
	case P_STYLE:
		switch (getstyleint(flags) + WM_TOP_LEVEL_LOCKED)
		{
		case WM_MODELESS:
			ep.setstaticcstring(MCmodelessstring);
			break;
		case WM_PALETTE:
			ep.setstaticcstring(MCpalettestring);
			break;
		case WM_MODAL:
			ep.setstaticcstring(MCmodalstring);
			break;
		case WM_SHEET:
			ep.setstaticcstring(MCsheetstring);
			break;
		case WM_DRAWER:
			ep.setstaticcstring(MCdrawerstring);
			break;
		default:
			ep.setstaticcstring(MCtoplevelstring);
			break;
		}
		break;
	case P_CANT_MODIFY:
		ep.setboolean(getflag(F_CANT_MODIFY));
		break;
	case P_CANT_PEEK:
		ep.setboolean(True);
		break;
	case P_DYNAMIC_PATHS:
		ep.setboolean(getflag(F_DYNAMIC_PATHS));
		break;
	case P_KEY:
		// OK-2010-02-11: [[Bug 8610]] - Passkey property more useful if it returns
		//   whether or not the script is available.
		ep . setboolean(iskeyed());
		
		break;
	case P_PASSWORD:
		ep . clear();
		break;
	case P_DESTROY_STACK:
		ep.setboolean(getflag(F_DESTROY_STACK));
		break;
	case P_DESTROY_WINDOW:
		ep.setboolean(getflag(F_DESTROY_WINDOW));
		break;
	case P_ALWAYS_BUFFER:
		ep.setboolean(getflag(F_ALWAYS_BUFFER));
		break;
	case P_LABEL:
	case P_UNICODE_LABEL:
		// MW-2007-07-06: [[ Bug 3226 ]] Updated to take into account 'title' being
		//   stored as a UTF-8 string.
		if (title == NULL)
			ep.clear();
		else
		{
			ep.setsvalue(title);
			if (which == P_LABEL)
				ep.utf8tonative();
			else
				ep.utf8toutf16();
		}
		break;
	case P_CLOSE_BOX:
		ep.setboolean(getflag(F_DECORATIONS) && decorations & WD_CLOSE);
		break;
	case P_ZOOM_BOX:
	case P_MAXIMIZE_BOX:
		ep.setboolean(getflag(F_DECORATIONS) && decorations & WD_MAXIMIZE);
		break;
	case P_DRAGGABLE:
		ep.setboolean(getflag(F_DECORATIONS) && decorations & WD_TITLE);
		break;
	case P_COLLAPSE_BOX:
	case P_MINIMIZE_BOX:
		ep.setboolean(getflag(F_DECORATIONS) && decorations & WD_TITLE);
		break;
	case P_LIVE_RESIZING:
		ep.setboolean(getflag(F_DECORATIONS) && decorations & WD_LIVERESIZING);
		break;
	case P_SYSTEM_WINDOW:
		ep.setboolean(getflag(F_DECORATIONS) && decorations & WD_UTILITY);
		break;
	case P_METAL:
		ep.setboolean(getflag(F_DECORATIONS) && decorations & WD_METAL);
		break;
	case P_SHADOW:
		ep.setboolean(!(flags & F_DECORATIONS && decorations & WD_NOSHADOW));
		break;
	case P_RESIZABLE:
		ep.setboolean(getflag(F_RESIZABLE));
		break;
	case P_MIN_WIDTH:
		ep.setint(minwidth);
		break;
	case P_MAX_WIDTH:
		ep.setint(maxwidth);
		break;
	case P_MIN_HEIGHT:
		ep.setint(minheight);
		break;
	case P_MAX_HEIGHT:
		ep.setint(maxheight);
		break;
	case P_DECORATIONS:

		ep.clear();
		if (flags & F_DECORATIONS)
		{
			if (decorations & WD_WDEF)
				ep.setint(decorations & ~WD_WDEF);
			else
			{
				if (decorations & WD_TITLE)
					ep.concatcstring(MCtitlestring, EC_COMMA, j++ == 0);
				if (decorations & WD_MENU)
					ep.concatcstring(MCmenustring, EC_COMMA, j++ == 0);
				if (decorations & WD_MINIMIZE)
					ep.concatcstring(MCminimizestring, EC_COMMA, j++ == 0);
				if (decorations & WD_MAXIMIZE)
					ep.concatcstring(MCmaximizestring, EC_COMMA, j++ == 0);
				if (decorations & WD_CLOSE)
					ep.concatcstring(MCclosestring, EC_COMMA, j++ == 0);
				if (decorations & WD_METAL)
					ep.concatcstring(MCmetalstring, EC_COMMA, j++ == 0);
				if (decorations & WD_UTILITY)
					ep.concatcstring(MCutilitystring, EC_COMMA, j++ == 0);
				if (decorations & WD_NOSHADOW)
					ep.concatcstring(MCnoshadowstring, EC_COMMA, j++ == 0);
				if (decorations & WD_FORCETASKBAR)
					ep.concatcstring(MCforcetaskbarstring, EC_COMMA, j++ == 0);
			}
		}
		else
			ep.setstaticcstring(MCdefaultstring);
		break;
	case P_RECENT_NAMES:
		MCrecent->getnames(this, ep);

		break;
	case P_RECENT_CARDS:
		MCrecent->getlongids(this, ep);
		break;
	case P_ICONIC:
		ep.setboolean(getstate(CS_ICONIC));
		break;
	case P_START_UP_ICONIC:
		ep.setboolean(getflag(F_START_UP_ICONIC));
		break;
	case P_ICON:
		ep.setint(iconid);
		break;
	case P_OWNER:
		if (parent == NULL || MCdispatcher->ismainstack(this))
			ep.clear();
		else
			return parent->getprop(0, P_LONG_ID, ep, False);
		break;
	case P_MAIN_STACK:
		if (parent != NULL && !MCdispatcher->ismainstack(sptr))
			sptr = (MCStack *)parent;
		ep.setnameref_unsafe(sptr->getname());
		break;
	case P_SUBSTACKS:
		ep.clear();
		if (substacks != NULL)
		{
			MCStack *sptr = substacks;
			do
			{
				ep.concatnameref(sptr->getname(), EC_RETURN, sptr == substacks);
				sptr = sptr->next();
			}
			while (sptr != substacks);
		}
		break;
	// MW-2011-08-08: [[ Groups ]] Add 'sharedGroupNames' and 'sharedGroupIds' properties to
	//   stack.
	case P_BACKGROUND_NAMES:
	case P_BACKGROUND_IDS:
	case P_SHARED_GROUP_NAMES:
	case P_SHARED_GROUP_IDS:
		{
			ep.clear();
			MCControl *startptr = editing == NULL ? controls : savecontrols;
			MCControl *optr = startptr;
			if (optr != NULL)
			{
				bool t_want_background;
				t_want_background = which == P_BACKGROUND_NAMES || which == P_BACKGROUND_IDS;
				
				bool t_want_shared;
				t_want_shared = which == P_SHARED_GROUP_NAMES || which == P_SHARED_GROUP_IDS;

				MCExecPoint ep2(ep);
				do
				{
					// MW-2011-08-08: [[ Groups ]] Use 'isbackground()' rather than !F_GROUP_ONLY.
					MCGroup *t_group;
					t_group = nil;
					if (optr->gettype() == CT_GROUP)
						t_group = static_cast<MCGroup *>(optr);

					optr = optr -> next();

					if (t_group == nil)
						continue;

					if (t_want_background && !t_group -> isbackground())
						continue;

					if (t_want_shared && !t_group -> isshared())
						continue;

					Properties t_prop;
					if (which == P_BACKGROUND_NAMES || which == P_SHARED_GROUP_NAMES)
						t_prop = P_SHORT_NAME;
					else
						t_prop = P_SHORT_ID;

					t_group->getprop(0, t_prop, ep2, False);
					
					ep.concatmcstring(ep2.getsvalue(), EC_RETURN, j++ == 0);
				}
				while (optr != startptr);
			}
		}
		break;
	case P_CARD_IDS:
	case P_CARD_NAMES:
		ep.clear();
		if (cards != NULL)
		{
			MCExecPoint ep2(ep);
			MCCard *cptr = cards;
			do
			{
				if (which == P_CARD_NAMES)
					cptr->getprop(0, P_SHORT_NAME, ep2, False);
				else
					cptr->getprop(0, P_SHORT_ID, ep2, False);
				ep.concatmcstring(ep2.getsvalue(), EC_RETURN, j++ == 0);
				cptr = cptr->next();
			}
			while (cptr != cards);
		}
		break;
	case P_EDIT_BACKGROUND:
		ep.setboolean(editing != NULL);
		break;
	case P_EXTERNALS:
		if (externalfiles == NULL)
			ep.clear();
		else
			ep.setsvalue(externalfiles);
		break;
	case P_EXTERNAL_COMMANDS:
	case P_EXTERNAL_FUNCTIONS:
		if (m_externals != nil)
			m_externals -> ListHandlers(ep, which == P_EXTERNAL_COMMANDS ? HT_MESSAGE : HT_FUNCTION);
		else
		ep.clear();
		break;
	case P_EXTERNAL_PACKAGES:
		if (m_externals != nil)
			m_externals -> ListExternals(ep);
		else
		ep . clear();
		break;
	case P_MODE:
		ep.setint(getmode());
		break;
	case P_WM_PLACE:
		ep.setboolean(getflag(F_WM_PLACE));
		break;
	case P_WINDOW_ID:
		ep.setint(MCscreen->dtouint4(window));
		break;
	case P_PIXMAP_ID:
		ep.setint(0);
		break;
	case P_HC_ADDRESSING:
		ep.setboolean(getflag(F_HC_ADDRESSING));
		break;
	case P_HC_STACK:
		ep.setboolean(getflag(F_HC_STACK));
		break;
	case P_SIZE:
		ep.setstaticcstring(STACK_SIZE);
		break;
	case P_FREE_SIZE:
		ep.setstaticcstring(FREE_SIZE);
		break;
	case P_LOCK_SCREEN:
		// MW-2011-08-18: [[ Redraw ]] Update to use redraw.
		ep.setboolean(MCRedrawIsScreenLocked());
		break;
	case P_SHOW_BORDER:
	case P_BORDER_WIDTH:
	case P_ENABLED:
	case P_DISABLED:
	case P_3D:
	case P_LOCK_LOCATION:
	case P_TOOL_TIP:
	// MW-2012-03-13: [[ UnicodeToolTip ]] Stacks's don't have tooltips.
	case P_UNICODE_TOOL_TIP:
		MCeerror->add(EE_OBJECT_SETNOPROP, 0, 0);
		return ES_ERROR;
	case P_STACK_FILES:
		getstackfiles(ep);
		break;
	case P_MENU_BAR:
		ep.setnameref_unsafe(getmenubar());
		break;
	case P_EDIT_MENUS:
		ep.setboolean(getstate(CS_EDIT_MENUS));
		break;
	case P_VSCROLL:
		ep.setint(getscroll());
		break;
	case P_CHARSET:
#ifdef _MACOSX
		ep.setstaticcstring((state & CS_TRANSLATED) != 0 ? "ISO" : "MacOS");
#else
		ep.setstaticcstring((state & CS_TRANSLATED) != 0 ? "MacOS" : "ISO");
#endif
		break;
	case P_FORMAT_FOR_PRINTING:
		ep.setboolean(getflag(F_FORMAT_FOR_PRINTING));
		break;
	case P_LINK_COLOR:
	case P_LINK_HILITE_COLOR:
	case P_LINK_VISITED_COLOR:
	case P_UNDERLINE_LINKS:
		if (linkatts == NULL && !effective)
			ep.clear();
		else
		{
			Linkatts *la = getlinkatts();
			switch (which)
			{
			case P_LINK_COLOR:
				MCU_get_color(ep, la->colorname, la->color);
				break;
			case P_LINK_HILITE_COLOR:
				MCU_get_color(ep, la->hilitecolorname, la->hilitecolor);
				break;
			case P_LINK_VISITED_COLOR:
				MCU_get_color(ep, la->visitedcolorname, la->visitedcolor);
				break;
			case P_UNDERLINE_LINKS:

				ep.setboolean(la->underline);
				break;
			default:
				break;
			}
		}
		break;
	case P_WINDOW_SHAPE:
		ep.setint(windowshapeid);
		break;
	case P_SCREEN:
	{
		const MCDisplay *t_display;
		t_display = MCscreen -> getnearestdisplay(rect);
		ep . setint(t_display -> index + 1);
	}
	break;
	case P_CURRENT_CARD:
		if (curcard != nil)
			return curcard -> names(P_SHORT_NAME, ep, parid);
		else
			ep . clear();
	break;
	case P_MODIFIED_MARK:
		ep . setboolean(getextendedstate(ECS_MODIFIED_MARK));
		break;
	
	// MW-2011-11-23: [[ AccelRender ]] Return the accelerated rendering state.
	case P_ACCELERATED_RENDERING:
		ep . setboolean(getacceleratedrendering());
		break;
	
	case P_COMPOSITOR_TYPE:
	{
		if (m_tilecache == nil)
			ep . clear();
		else
		{
			const char *t_type;
			switch(MCTileCacheGetCompositor(m_tilecache))
			{
			case kMCTileCacheCompositorSoftware:
				t_type = "Software";
				break;
			case kMCTileCacheCompositorCoreGraphics:
				t_type = "CoreGraphics";
				break;
			case kMCTileCacheCompositorStaticOpenGL:
				t_type = "Static OpenGL";
				break;
			case kMCTileCacheCompositorDynamicOpenGL:
				t_type = "Dynamic OpenGL";
				break;
			default:
				assert(false);
				break;	
			}
			ep . setstaticcstring(t_type);
		}
	}
	break;
			
	case P_COMPOSITOR_TILE_SIZE:
		if (m_tilecache == nil)
			ep . clear();
		else
			ep . setuint(MCTileCacheGetTileSize(m_tilecache));
	break;

	case P_COMPOSITOR_CACHE_LIMIT:
		if (m_tilecache == nil)
			ep . clear();
		else
			ep . setuint(MCTileCacheGetCacheLimit(m_tilecache));
	break;
		
	// MW-2011-11-24: [[ UpdateScreen ]] Get the updateScreen properties.
	case P_DEFER_SCREEN_UPDATES:
		ep . setboolean(effective ? m_defer_updates && m_tilecache != nil : m_defer_updates);
		break;
#endif /* MCStack::getprop */
	default:
	{
		Exec_stat t_stat;
		t_stat = mode_getprop(parid, which, ep, MCnullmcstring, effective);
		if (t_stat == ES_NOT_HANDLED)
			return MCObject::getprop(parid, which, ep, effective);

		return t_stat;
	}
	break;
	}
	return ES_NORMAL;
}


Exec_stat MCStack::setprop(uint4 parid, Properties which, MCExecPoint &ep, Boolean effective)
{
	Boolean dirty;
	Boolean bval;
	uint4 bflags;
	Boolean newlock;

	uint2 newsize;
	MCString data = ep.getsvalue();

	switch (which)
	{
#ifdef /* MCStack::setprop */ LEGACY_EXEC
	case P_FULLSCREEN:
		{
			if (!MCU_stob(data, bval))
			{
				MCeerror->add(EE_OBJECT_NAB, 0, 0, data);
				return ES_ERROR;
			}
			
			bool t_bval;
			t_bval = bval == True;

			bool v_changed = (getextendedstate(ECS_FULLSCREEN) != t_bval);
			if ( v_changed)
			{
				setextendedstate(t_bval, ECS_FULLSCREEN);
				
				// MW-2012-10-04: [[ Bug 10436 ]] Use 'setrect' to change the rect
				//   field.
				if ( bval )
					old_rect = rect ;
				else if (( old_rect.width > 0 ) && ( old_rect.height > 0 ))
					setrect(old_rect);
				
				if ( opened > 0 ) 
					reopenwindow();
			}
		}
	break;
		
	case P_NAME:
		{
			// MW-2008-10-28: [[ ParentScripts ]] If this stack has its 'has parentscripts'
			//   flag set, temporarily store a copy of the current name.
			MCAutoNameRef t_old_name;
			if (getextendedstate(ECS_HAS_PARENTSCRIPTS))
				t_old_name . Clone(getname());

			// We don't allow ',' in stack names - so coerce to '_'.
			ep . replacechar(',', '_');

			// If the name is going to be empty, coerce to 'Untitled'.
			if (ep . isempty())
				ep . setstaticcstring(MCuntitledstring);

			if (MCObject::setprop(parid, which, ep, effective) != ES_NORMAL)
				return ES_ERROR;

			dirtywindowname();

			// MW-2008-10-28: [[ ParentScripts ]] If there is a copy of the old name, then
			//   it means this stack potentially has parent scripts...
			if (t_old_name != NULL)
			{
				// If the name has changed process...
				if (!hasname(t_old_name))
				{
					bool t_is_mainstack;
					t_is_mainstack = MCdispatcher -> ismainstack(this) == True;

					// First flush any references to parentScripts on this stack
					MCParentScript::FlushStack(this);
					setextendedstate(false, ECS_HAS_PARENTSCRIPTS);
				}
			}
		}
		return ES_NORMAL;
	case P_ID:
		uint4 newid;
		if (!MCU_stoui4(data, newid) || newid < obj_id)
		{
			MCeerror->add(EE_STACK_BADID, 0, 0, data);
			return ES_ERROR;
		}
		if (obj_id != newid)
		{
			uint4 oldid = obj_id;
			obj_id = newid;
			message_with_args(MCM_id_changed, oldid, obj_id);
		}
		break;
	case P_VISIBLE:

		dirty = True;

		if (MCObject::setprop(parid, which, ep, effective) != ES_NORMAL)
			return ES_ERROR;
		if (opened && (!(state & CS_IGNORE_CLOSE)) )
		{
			if (flags & F_VISIBLE)
			{
				dirtywindowname();
				openwindow(mode >= WM_PULLDOWN);
			}
			else
			{
				MCscreen->closewindow(window);
#ifdef X11
				//x11 will send propertynotify event which call ::close
				state |= CS_ISOPENING;
#endif

			}


			MCscreen->sync(getw());
		}
		return ES_NORMAL;
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
		if (MCObject::setprop(parid, which, ep, effective) != ES_NORMAL)
			return ES_ERROR;
		// MW-2011-08-18: [[ Redraw ]] This could be restricted to just children
		//   of this stack - but for now do the whole screen.
		MCRedrawDirtyScreen();
		return ES_NORMAL;
	case P_FILE_NAME:
		delete filename;
		// MW-2007-03-15: [[ Bug 616 ]] Throw an error if you try and set the filename of a substack
		if (!MCdispatcher->ismainstack(this))
		{
			MCeerror -> add(EE_STACK_NOTMAINSTACK, 0, 0);
			return ES_ERROR;
		}
		
		if (data.getlength() == 0)
			filename = NULL;
		else
			filename = data.clone();
		return ES_NORMAL;
	case P_SAVE_COMPRESSED:
		break;
	case P_CANT_ABORT:
		if (!MCU_matchflags(data, flags, F_CANT_ABORT, dirty))
		{
			MCeerror->add
			(EE_OBJECT_NAB, 0, 0, data);
			return ES_ERROR;
		}
		break;
	case P_CANT_DELETE:
		if (!MCU_matchflags(data, flags, F_S_CANT_DELETE, dirty))
		{
			MCeerror->add
			(EE_OBJECT_NAB, 0, 0, data);
			return ES_ERROR;
		}
		break;
	case P_STYLE:
		flags &= ~F_STYLE;
		if (data == MCsheetstring)
			flags |= WM_MODAL - WM_TOP_LEVEL_LOCKED;
		else
			if (data == MCmodalstring || data == MCdialogstring
			        || data == MCmovablestring)
				flags |= WM_MODAL - WM_TOP_LEVEL_LOCKED;
			else
			{
				if (data == MCmodelessstring)
					flags |= WM_MODELESS - WM_TOP_LEVEL_LOCKED;
				else if (data == MCpalettestring || data == MCshadowstring || data == MCroundrectstring)
						flags |= WM_PALETTE - WM_TOP_LEVEL_LOCKED;
			}

		if (opened)
		{
			mode = WM_TOP_LEVEL;
			reopenwindow();
		}
		break;

	case P_CANT_MODIFY:
		if (!MCU_matchflags(data, flags, F_CANT_MODIFY, dirty))
		{
			MCeerror->add(EE_OBJECT_NAB, 0, 0, data);
			return ES_ERROR;
		}
		if (dirty && opened)
		{
			if (!iskeyed())
			{
				flags ^= F_CANT_MODIFY;
				MCeerror->add(EE_STACK_NOKEY, 0, 0);
				return ES_ERROR;
			}
			if (mode == WM_TOP_LEVEL || mode == WM_TOP_LEVEL_LOCKED)
				if (flags & F_CANT_MODIFY || !MCdispatcher->cut(True))
					mode = WM_TOP_LEVEL_LOCKED;
				else
					mode = WM_TOP_LEVEL;
			stopedit();
			dirtywindowname();
			resetcursor(True);
			MCstacks->top(this);
		}
		break;
	case P_CANT_PEEK:
		break;
	case P_DYNAMIC_PATHS:
		if (!MCU_matchflags(data, flags, F_DYNAMIC_PATHS, dirty))
		{
			MCeerror->add
			(EE_OBJECT_NAB, 0, 0, data);
			return ES_ERROR;
		}
		break;
	case P_DESTROY_STACK:
		if (!MCU_matchflags(data, flags, F_DESTROY_STACK, dirty))
		{
			MCeerror->add
			(EE_OBJECT_NAB, 0, 0, data);
			return ES_ERROR;
		}
		break;
	case P_DESTROY_WINDOW:
		if (!MCU_matchflags(data, flags, F_DESTROY_WINDOW, dirty))
		{
			MCeerror->add
			(EE_OBJECT_NAB, 0, 0, data);
			return ES_ERROR;
		}
		break;
	case P_ALWAYS_BUFFER:
		if (!MCU_matchflags(data, flags, F_ALWAYS_BUFFER, dirty))
		{
			MCeerror->add(EE_OBJECT_NAB, 0, 0, data);
			return ES_ERROR;
		}
		break;
	case P_LABEL:
	case P_UNICODE_LABEL:
		// MW-2007-07-06: [[ Bug 3226 ]] Updated to take into account 'title' being
		//   stored as a UTF-8 string.
		delete title;
		title = NULL;
		if (data != MCnullmcstring)
		{
			if (which == P_UNICODE_LABEL)
				ep . utf16toutf8();
			else
				ep . nativetoutf8();

			title = ep.getsvalue().clone();
			flags |= F_TITLE;
		}
		else
			flags &= ~F_TITLE;
		dirtywindowname();
		break;
	case P_CLOSE_BOX:
	case P_ZOOM_BOX:
	case P_DRAGGABLE:
	case P_MAXIMIZE_BOX:
	case P_SHADOW:
	case P_SYSTEM_WINDOW:
	case P_MINIMIZE_BOX:
	case P_METAL:
	case P_LIVE_RESIZING:
	case P_COLLAPSE_BOX:
		if (!MCU_stob(data, bval))
		{
			MCeerror->add(EE_OBJECT_NAB, 0, 0, data);
			return ES_ERROR;
		}
		if (!(flags & F_DECORATIONS))
			decorations = WD_MENU | WD_TITLE | WD_MINIMIZE | WD_MAXIMIZE | WD_CLOSE;
		flags |= F_DECORATIONS;
		switch (which)
		{
		case P_CLOSE_BOX:
			bflags = WD_CLOSE;
			break;
		case P_COLLAPSE_BOX:
		case P_MINIMIZE_BOX:
			bflags = WD_MINIMIZE;
			break;
		case P_ZOOM_BOX:
		case P_MAXIMIZE_BOX:
			bflags = WD_MAXIMIZE;
			break;
		case P_LIVE_RESIZING:
			bflags = WD_LIVERESIZING;
			break;
		case P_SYSTEM_WINDOW:
			bflags = WD_UTILITY;
			break;
		case P_METAL:
			bflags = WD_METAL;
			break;
		case P_SHADOW:
			bval = !bval;
			bflags = WD_NOSHADOW;
			break;
		default:
			bflags = 0;
			break;
		}
		if (bval)
			decorations |= bflags;
		else
			decorations &= ~bflags;
		if (opened)
			reopenwindow();
		break;
	case P_RESIZABLE:
		if (!MCU_matchflags(data, flags, F_RESIZABLE, dirty))
		{
			MCeerror->add
			(EE_OBJECT_NAB, 0, 0, data);
			return ES_ERROR;
		}
		if (dirty && opened)
			reopenwindow();
		break;
	case P_MIN_WIDTH:
	case P_MAX_WIDTH:
	case P_MIN_HEIGHT:
	case P_MAX_HEIGHT:
		if (!MCU_stoui2(data, newsize))
		{
			MCeerror->add
			(EE_STACK_MINMAXNAN, 0, 0, data);
			return ES_ERROR;
		}
		switch (which)
		{
		case P_MIN_WIDTH:
			minwidth = newsize;
			if (minwidth > maxwidth)
				maxwidth = minwidth;
			break;
		case P_MAX_WIDTH:
			maxwidth = newsize;
			if (minwidth > maxwidth)
				minwidth = maxwidth;
			break;
		case P_MIN_HEIGHT:
			minheight = newsize;
			if (minheight > maxheight)
				maxheight = minheight;
			break;
		case P_MAX_HEIGHT:
			maxheight = newsize;
			if (minheight > maxheight)
				minheight = maxheight;
			break;
		default:
			break;
		}
		if (opened)
		{
			sethints();
			setgeom();
		}
		break;
	case P_ICON:
		{
			uint4 newiconid;
			if (!MCU_stoui4(data, newiconid))
			{
				MCeerror->add(EE_STACK_ICONNAN, 0, 0, data);
				return ES_ERROR;
			}
			if (opened && iconid != newiconid)
			{
				iconid = newiconid;
				if (state & CS_ICONIC)
					redrawicon();
			}
		}
		break;
	case P_ICONIC:
		{
			uint4 newstate = state;
			if (!MCU_matchflags(data, newstate, CS_ICONIC, dirty))
			{
				MCeerror->add
				(EE_OBJECT_NAB, 0, 0, data);
				return ES_ERROR;
			}
			//SMR 1261 don't set state to allow iconify() to take care of housekeeping
			// need to check X11 to make sure MCStack::iconify() (in stack2.cpp) is called when this prop is set
			if (dirty && opened)
			{
				sethints();
				if (newstate & CS_ICONIC)
					MCscreen->iconifywindow(window);
				else
				{
					MCscreen->uniconifywindow(window);
				}
			}
		}
		break;
	case P_DECORATIONS:
		{
			uint2 olddec = decorations;
			uint4 oldflags = flags;
			decorations = WD_CLEAR;
			if (data == MCdefaultstring)
				flags &= ~F_DECORATIONS;
			else
			{
				flags |= F_DECORATIONS;
				uint2 i1;
				if (MCU_stoui2(data, i1))
					decorations = i1 | WD_WDEF;
				else
				{
					uint4 l = data.getlength();
					const char *sptr = data.getstring();
					MCU_skip_spaces(sptr, l);
					if (decorations & WD_WDEF)
						ep.setint(decorations & ~WD_WDEF);
					else
					{
						while (l != 0)
						{
							const char *startptr = sptr;
							if (!MCU_strchr(sptr, l, ','))
							{
								sptr += l;
								l = 0;
							}
							MCString tdata(startptr, sptr - startptr);
							MCU_skip_char(sptr, l);
							MCU_skip_spaces(sptr, l);
							if (tdata == MCtitlestring)
							{
								decorations |= WD_TITLE;
								continue;
							}
							if (tdata == MCmenustring)
							{
								decorations |= WD_MENU | WD_TITLE;
								continue;
							}
							if (tdata == MCminimizestring)
							{
								decorations |= WD_MINIMIZE | WD_TITLE;
								continue;
							}
							if (tdata == MCmaximizestring)
							{
								decorations |= WD_MAXIMIZE | WD_TITLE;
								continue;
							}
							if (tdata == MCclosestring)
							{
								decorations |= WD_CLOSE | WD_TITLE;
								continue;
							}
							if (tdata == MCmetalstring)
							{
								decorations |= WD_METAL; //metal can not have title
								continue;
							}
							if (tdata == MCutilitystring)
							{
								decorations |= WD_UTILITY;
								continue;
							}
							if (tdata == MCnoshadowstring)
							{
								decorations |= WD_NOSHADOW;
								continue;
							}
							if (tdata == MCforcetaskbarstring)
							{
								decorations |= WD_FORCETASKBAR;
								continue;
							}
							MCeerror->add(EE_STACK_BADDECORATION, 0, 0, data);
							return ES_ERROR;
						}
					}
				}

			}
			if (flags != oldflags || decorations != olddec)
				if (opened)
					reopenwindow();
				else
				{
					if (window != DNULL)
					{
						stop_externals();
						MCscreen->destroywindow(window);
						delete titlestring;
						titlestring = NULL;
					}
				}
		}
		break;
	case P_START_UP_ICONIC:
		if (!MCU_matchflags(data, flags, F_START_UP_ICONIC, dirty))
		{
			MCeerror->add
			(EE_OBJECT_NAB, 0, 0, data);
			return ES_ERROR;
		}
		if (dirty && opened)
			sethints();
		break;
	case P_EDIT_BACKGROUND:
		Boolean edit;
		if (!MCU_stob(data, edit) || !opened)
			return ES_ERROR;
		if (edit)
		{
			MCGroup *gptr = (MCGroup *)curcard->getchild(CT_FIRST, MCnullmcstring,
			                CT_GROUP, CT_UNDEFINED);
			if (gptr == NULL)
				gptr = getbackground(CT_FIRST, MCnullmcstring, CT_GROUP);
			if (gptr != NULL)
				startedit(gptr);
		}
		else
			stopedit();
		dirtywindowname();
		break;
	case P_MAIN_STACK:
		{
			MCStack *stackptr = NULL;
			char *sname = data.clone();

			if ((stackptr = MCdispatcher->findstackname(sname)) == NULL)
			{
				MCeerror->add(EE_STACK_NOMAINSTACK, 0, 0, data);
				delete sname;
				return ES_ERROR;
			}
			delete sname;

			if (stackptr != this && !MCdispatcher->ismainstack(stackptr))
			{
				MCeerror->add(EE_STACK_NOTMAINSTACK, 0, 0, data);
				return ES_ERROR;
			}
			
			if (parent != NULL && this != MCdispatcher->gethome() && (substacks == NULL || stackptr == this))
			{
				bool t_this_is_mainstack;
				t_this_is_mainstack = MCdispatcher -> ismainstack(this) == True;

				// OK-2008-04-10 : Added parameters to mainstackChanged message to specify the new
				// and old mainstack names.
				MCObject *t_old_stackptr;
				if (t_this_is_mainstack)
					t_old_stackptr = this;
				else
					t_old_stackptr = parent;

				//   If this was previously a mainstack, then it will be referenced by (name, NULL).
				//   If this was previously a substack, it will have been referenced by (name, old_mainstack).

				if (t_this_is_mainstack)
					MCdispatcher->removestack(this);
				else
				{
					MCStack *pstack = (MCStack *)parent;
					remove(pstack->substacks);
					// MW-2012-09-07: [[ Bug 10372 ]] If the stack no longer has substacks, then 
					//   make sure we undo the extraopen.
					if (pstack->substacks == NULL)
						pstack -> extraclose(true);
				}

				if (stackptr == this)
				{
					MCdispatcher->appendstack(this);
					parent = MCdispatcher->gethome();
				}
				else
				{
					// MW-2012-09-07: [[ Bug 10372 ]] If the stack doesn't have substacks, then
					//   make sure we apply the extraopen (as it's about to have some!).
					if (stackptr -> substacks == NULL)
						stackptr -> extraopen(true);
					appendto(stackptr->substacks);
					parent = stackptr;
				}

				// OK-2008-04-10 : Added parameters to mainstackChanged message to specify the new
				// and old mainstack names.
				message_with_args(MCM_main_stack_changed, t_old_stackptr -> getname(), stackptr -> getname());
			}
			else
			{
				MCeerror->add(EE_STACK_CANTSETMAINSTACK, 0, 0, data);
				return ES_ERROR;
			}
		}
		break;
	case P_SUBSTACKS:
		{
			if (!MCdispatcher->ismainstack(this))
			{
				MCeerror->add
				(EE_STACK_NOTMAINSTACK, 0, 0, data);
				return ES_ERROR;
			}
			char *subs = data.clone();
			if (resubstack(subs) != ES_NORMAL)
			{
				delete subs;
				return ES_ERROR;
			}
			delete subs;
		}
		break;
	case P_EXTERNALS:
		delete externalfiles;
		if (data != MCnullmcstring)
			externalfiles = data.clone();
		else
			externalfiles = NULL;
		break;
	case P_WM_PLACE:
		if (!MCU_matchflags(data, flags, F_WM_PLACE, dirty))
		{
			MCeerror->add
			(EE_OBJECT_NAB, 0, 0, data);
			return ES_ERROR;
		}
		break;
	case P_HC_ADDRESSING:
		if (!MCU_matchflags(data, flags, F_HC_ADDRESSING, dirty))
		{
			MCeerror->add
			(EE_OBJECT_NAB, 0, 0, data);
			return ES_ERROR;
		}
		break;
	case P_LOCK_SCREEN:
		if (!MCU_stob(data, newlock))
		{
			MCeerror->add(EE_OBJECT_NAB, 0, 0, data);
			return ES_ERROR;
		}
		// MW-2011-08-18: [[ Redraw ]] Update to use redraw.
		if (newlock)
			MCRedrawLockScreen();
		else
			MCRedrawUnlockScreenWithEffects();
		break;
	case P_SHOW_BORDER:
	case P_BORDER_WIDTH:
	case P_ENABLED:
	case P_DISABLED:
	case P_3D:
	case P_LOCK_LOCATION:
	case P_TOOL_TIP:
	// MW-2012-03-13: [[ UnicodeToolTip ]] Stacks don't have tooltips.
	case P_UNICODE_TOOL_TIP:
	case P_LAYER:
		MCeerror->add
		(EE_OBJECT_SETNOPROP, 0, 0);
		return ES_ERROR;
	case P_STACK_FILES:
		setstackfiles(data);
		if (nstackfiles != 0)
			flags |= F_STACK_FILES;
		else
			flags &= ~F_STACK_FILES;
		break;
	case P_MENU_BAR:
	{
		MCAutoNameRef t_new_menubar;
		/* UNCHECKED */ ep . copyasnameref(t_new_menubar);
		if (!MCNameIsEqualTo(getmenubar(), t_new_menubar, kMCCompareCaseless))
		{
			MCNameDelete(_menubar);
			/* UNCHECKED */ MCNameClone(t_new_menubar, _menubar);
			if (!hasmenubar())
				flags &= ~F_MENU_BAR;
			else
				flags |= F_MENU_BAR;
			if (opened)
			{
				setgeom();
				updatemenubar();

				// MW-2011-08-17: [[ Redraw ]] Tell the stack to dirty all of itself.
				dirtyall();
			}
		}
	}
	break;
	case P_EDIT_MENUS:
		if (!MCU_matchflags(data, state, CS_EDIT_MENUS, dirty))
		{
			MCeerror->add
			(EE_OBJECT_NAB, 0, 0, data);
			return ES_ERROR;
		}
		if (dirty && opened)
		{
			setgeom();
			updatemenubar();
		}
		break;
	case P_FORMAT_FOR_PRINTING:

		if (!MCU_matchflags(data, flags, F_FORMAT_FOR_PRINTING, dirty))
		{
			MCeerror->add(EE_OBJECT_NAB, 0, 0, data);
			return ES_ERROR;
		}
		if (dirty && opened)
			purgefonts();
		break;
	case P_LINK_COLOR:
	case P_LINK_HILITE_COLOR:
	case P_LINK_VISITED_COLOR:
	case P_UNDERLINE_LINKS:
		{
			Exec_stat stat = ES_NORMAL;
			if (ep.getsvalue().getlength() == 0)
			{
				if (linkatts != NULL)
				{
					delete linkatts->colorname;
					delete linkatts->hilitecolorname;
					delete linkatts->visitedcolorname;
					delete linkatts;
					linkatts = NULL;
				}
			}
			else
			{
				if (linkatts == NULL)
				{
					linkatts = new Linkatts;
					memcpy(linkatts, &MClinkatts, sizeof(Linkatts));
					linkatts->colorname = strclone(MClinkatts.colorname);
					linkatts->hilitecolorname = strclone(MClinkatts.hilitecolorname);
					linkatts->visitedcolorname = strclone(MClinkatts.visitedcolorname);
					MCscreen->alloccolor(linkatts->color);
					MCscreen->alloccolor(linkatts->hilitecolor);
					MCscreen->alloccolor(linkatts->visitedcolor);
				}
				switch (which)
				{
				case P_LINK_COLOR:
					stat = MCU_change_color(linkatts->color, linkatts->colorname, ep, 0, 0);
					break;
				case P_LINK_HILITE_COLOR:
					stat = MCU_change_color(linkatts->hilitecolor,
					                        linkatts->hilitecolorname, ep, 0, 0);
					break;
				case P_LINK_VISITED_COLOR:
					stat = MCU_change_color(linkatts->visitedcolor,
					                        linkatts->visitedcolorname, ep, 0, 0);
					break;
				case P_UNDERLINE_LINKS:
					stat = ep.getboolean(linkatts->underline, 0, 0, EE_PROPERTY_NAB);
					break;
				default:
					break;
				}
			}
			
			// MW-2011-08-17: [[ Redraw ]] Tell the stack to dirty all of itself.
			dirtyall();
			return stat;
		}
		break;
	case P_WINDOW_SHAPE:
		{
			uint4 newwinshapeid;
			// unless we opened the window ourselves, we can't change the window shape
			if (!MCU_stoui4(data, newwinshapeid))
			{
				MCeerror->add(EE_STACK_ICONNAN, 0, 0, data);
				return ES_ERROR;
			}
			windowshapeid = newwinshapeid;
			if (windowshapeid)
			{
				// MW-2011-10-08: [[ Bug 4198 ]] Make sure we preserve the shadow status of the stack.
				decorations = WD_SHAPE | (decorations & WD_NOSHADOW);
				flags |= F_DECORATIONS;
				
#if defined(_DESKTOP)
				// MW-2004-04-27: [[Deep Masks]] If a window already has a mask, replace it now to avoid flicker
				if (m_window_shape != NULL)
				{
					MCImage *t_image;
					// MW-2009-02-02: [[ Improved image search ]] Search for the appropriate image object using the standard method.
					t_image = resolveimageid(windowshapeid);
					if (t_image != NULL)
					{
						MCWindowShape *t_new_mask;
						setextendedstate(True, ECS_MASK_CHANGED);
						t_image -> setflag(True, F_I_ALWAYS_BUFFER);
						t_image -> open();
						t_new_mask = t_image -> makewindowshape();
						t_image -> close();
						if (t_new_mask != NULL)
						{
							destroywindowshape();
							m_window_shape = t_new_mask;
							// MW-2011-08-17: [[ Redraw ]] Tell the stack to dirty all of itself.
							dirtyall();
							break;
						}
					}
				}
#endif
			}
			else
			{
				decorations &= ~WD_SHAPE;
				flags &= ~F_DECORATIONS;
			}
			
			if (opened)
			{
				reopenwindow();
				
#if defined(_DESKTOP)
				// MW-2011-08-17: [[ Redraw ]] Tell the stack to dirty all of itself.
				if (m_window_shape != NULL)
					dirtyall();
#endif
			}
		}
		break;
	case P_BLEND_LEVEL:
		old_blendlevel = blendlevel;
		if (MCObject::setprop(parid, which, ep, effective) != ES_NORMAL)
			return ES_ERROR;
		
		// MW-2011-11-03: [[ Bug 9852 ]] Make sure an update is scheduled to sync the
		//   opacity.
		MCRedrawScheduleUpdateForStack(this);
	break;
	case P_CURRENT_CARD:
	{
		MCCard *t_card;
		t_card = getchild(CT_EXPRESSION, ep . getsvalue(), CT_CARD);
		if (t_card != NULL)
			setcard(t_card, False, False);
	}
	break;
	case P_MODIFIED_MARK:
		if (!MCU_matchflags(data, f_extended_state, ECS_MODIFIED_MARK, dirty))
		{
			MCeerror->add(EE_OBJECT_NAB, 0, 0, data);
			return ES_ERROR;
		}
		if (dirty && opened)
			updatemodifiedmark();
		break;
	
	// MW-2011-11-23: [[ AccelRender ]] Configure accelerated rendering.
	case P_ACCELERATED_RENDERING:
	{
		Boolean t_accel_render;
		if (!MCU_stob(data, t_accel_render))
		{
			MCeerror->add(EE_OBJECT_NAB, 0, 0, data);
			return ES_ERROR;
		}
		setacceleratedrendering(t_accel_render == True);
	}
	break;
	
	// MW-2011-09-10: [[ TileCache ]] Configure the compositor to use.
	case P_COMPOSITOR_TYPE:
	{
		MCTileCacheCompositorType t_type;
		if (data == MCnullmcstring || data == "none")
			t_type = kMCTileCacheCompositorNone;
		else if (data == "software")
			t_type = kMCTileCacheCompositorSoftware;
		else if (data == "coregraphics")
			t_type = kMCTileCacheCompositorCoreGraphics;
		else if (data == "opengl")
		{
			if (MCTileCacheSupportsCompositor(kMCTileCacheCompositorDynamicOpenGL))
				t_type = kMCTileCacheCompositorDynamicOpenGL;
			else
				t_type = kMCTileCacheCompositorStaticOpenGL;
		}
		else if (data == "static opengl")
			t_type = kMCTileCacheCompositorStaticOpenGL;
		else if (data == "dynamic opengl")
			t_type = kMCTileCacheCompositorDynamicOpenGL;
		else
		{
			MCeerror -> add(EE_COMPOSITOR_UNKNOWNTYPE, 0, 0, data);
			return ES_ERROR;
		}
		
		if (!MCTileCacheSupportsCompositor(t_type))
		{
			MCeerror -> add(EE_COMPOSITOR_NOTSUPPORTED, 0, 0, data);
			return ES_ERROR;
		}
		
		if (t_type == kMCTileCacheCompositorNone)
		{
			MCTileCacheDestroy(m_tilecache);
			m_tilecache = nil;
		}
		else
		{
			if (m_tilecache == nil)
			{
				MCTileCacheCreate(32, 4096 * 1024, m_tilecache);
				// IM-2013-08-21: [[ ResIndependence ]] Use device coords for tilecache operation
				MCRectangle t_device_rect;
				t_device_rect = MCGRectangleGetIntegerBounds(MCResUserToDeviceRect(curcard->getrect()));
				MCTileCacheSetViewport(m_tilecache, t_device_rect);
			}
		
			MCTileCacheSetCompositor(m_tilecache, t_type);
		}
		
		dirtyall();
	}
	break;
			
	// MW-2011-09-10: [[ TileCache ]] Set the maximum number of bytes to use for the tile cache.
	case P_COMPOSITOR_CACHE_LIMIT:
	{
		uint32_t t_new_cachelimit;	
		if (!MCU_stoui4(data, t_new_cachelimit))
		{
			MCeerror->add(EE_OBJECT_NAN, 0, 0, data);
			return ES_ERROR;
		}
		if (m_tilecache != nil)
		{
			MCTileCacheSetCacheLimit(m_tilecache, t_new_cachelimit);
			dirtyall();
		}
	}
	break;

	// MW-2011-09-10: [[ TileCache ]] Set the size of tile to use for the tile cache.
	case P_COMPOSITOR_TILE_SIZE:
	{
		uint32_t t_new_tilesize;	
		if (!MCU_stoui4(data, t_new_tilesize))
		{
			MCeerror->add(EE_OBJECT_NAN, 0, 0, data);
			return ES_ERROR;
		}
		if (!MCIsPowerOfTwo(t_new_tilesize) || t_new_tilesize < 16 || t_new_tilesize > 256)
		{
			MCeerror->add(EE_COMPOSITOR_INVALIDTILESIZE, 0, 0, data);
			return ES_ERROR;
		}
		if (m_tilecache != nil)
		{
			MCTileCacheSetTileSize(m_tilecache, t_new_tilesize);
			dirtyall();
		}
	}
	break;

	// MW-2011-11-24: [[ UpdateScreen ]] Configure the updateScreen properties.
	case P_DEFER_SCREEN_UPDATES:
	{
		Boolean t_defer_updates;
		if (!MCU_stob(data, t_defer_updates))
		{
			MCeerror->add(EE_OBJECT_NAB, 0, 0, data);
			return ES_ERROR;
		}

		m_defer_updates = (t_defer_updates == True);
	}
	break;
#endif /* MCStack::setprop */	
	default:
	{
		Exec_stat t_stat;
		t_stat = mode_setprop(parid, which, ep, MCnullmcstring, MCnullmcstring, effective);
		if (t_stat == ES_NOT_HANDLED)
			return MCObject::setprop(parid, which, ep, effective);

		return t_stat;
	}
	}
	return ES_NORMAL;
}

Boolean MCStack::del()
{
	// MW-2012-10-26: [[ Bug 9918 ]] If 'cantDelete' is set, then don't delete the stack. Also
	//   make sure we throw an error.
	if (parent == NULL || scriptdepth != 0 || flags & F_S_CANT_DELETE)
	{
		MCeerror->add(EE_OBJECT_CANTREMOVE, 0, 0);
		return False;
	}
	if (MCdispatcher->gethome() == this)
		return False;
	
	if (opened)
	{
		// MW-2007-04-22: [[ Bug 4203 ]] Coerce the flags to include F_DESTROY_WINDOW to ensure we don't
		//   get system resource accumulation in a tight create/destroy loop.
		flags |= F_DESTROY_WINDOW;
		close();
	}

	if (curcard != NULL)
		curcard->message(MCM_delete_stack);
	else
		if (cards != NULL)
			cards->message(MCM_delete_stack);
	
	if (MCdispatcher->ismainstack(this))
	{
		MCdispatcher->removestack(this);
	}
	else
	{
		remove(((MCStack *)parent)->substacks);
		// MW-2012-09-07: [[ Bug 10372 ]] If the stack no longer has substacks then make sure we
		//   undo the extraopen.
		if (((MCStack *)parent) -> substacks == NULL)
			((MCStack *)parent) -> extraclose(true);
	}

	if (MCstaticdefaultstackptr == this)
		MCstaticdefaultstackptr = MCtopstackptr;
	if (MCdefaultstackptr == this)
		MCdefaultstackptr = MCstaticdefaultstackptr;

	// MW-2008-10-28: [[ ParentScripts ]] If this stack has its 'has parentscripts'
	//   flag set, flush the parentscripts table.
	if (getextendedstate(ECS_HAS_PARENTSCRIPTS))
		MCParentScript::FlushStack(this);

	return True;
}

void MCStack::paste(void)
{
	char *t_new_name;
	if (MCdispatcher -> findstackname(getname_oldstring()) != NULL)
	{
		unsigned int t_index;
		t_index = 1;

		const char *t_old_name;
		t_old_name = !isunnamed() ? getname_cstring() : "Unnamed";

		for(;;)
		{
			t_new_name = new char[strlen(t_old_name) + 8 + 16];
			if (t_index == 1)
				sprintf(t_new_name, "Copy of %s", t_old_name);
			else
				sprintf(t_new_name, "Copy (%d) of %s", t_index, t_old_name);
			if (MCdispatcher -> findstackname(t_new_name) == NULL)
				break;
			t_index += 1;
			delete t_new_name;
		}
		setname_cstring(t_new_name);
	}

	// MW-2007-12-11: [[ Bug 5441 ]] When we paste a stack, it should be parented to the home
	//   stack, just like when we create a whole new stack.
	parent = MCdispatcher -> gethome();
	MCdispatcher -> appendstack(this);

	positionrel(MCdefaultstackptr->rect, OP_CENTER, OP_MIDDLE);
	open();
}

MCStack *MCStack::getstack()
{
	return this;
}

Exec_stat MCStack::handle(Handler_type htype, MCNameRef message, MCParameter *params, MCObject *passing_object)
{
	if (!opened)
	{
		if (window == DNULL && !MCNameIsEqualTo(message, MCM_start_up, kMCCompareCaseless)
#ifdef _MACOSX
		        && !(state & CS_DELETE_STACK))
#else
				&& externalfiles != NULL && !(state & CS_DELETE_STACK))
#endif
			realize();
	}

	// MW-2009-01-28: [[ Bug ]] Card and stack parentScripts don't work.
	// First attempt to handle the message with the script and parentScript of this
	// object.
	Exec_stat stat;
	stat = ES_NOT_HANDLED;
	if (!MCdynamicpath || MCdynamiccard->getparent() != this)
		stat = handleself(htype, message, params);
	else if (passing_object != nil)
	{
		// MW-2011-06-20:  If dynamic path is enabled, and this stack is the parent
		//   of the dynamic card then instead of passing through this stack, we pass
		//   through the dynamic card (which will then pass through this stack).
		MCdynamicpath = False;
		stat = MCdynamiccard->handle(htype, message, params, this);
		return stat;
	}

	if (((passing_object != nil && stat == ES_PASS) || stat == ES_NOT_HANDLED) && m_externals != nil)
	{
		Exec_stat oldstat = stat;
		stat = m_externals -> Handle(this, htype, message, params);
		if (oldstat == ES_PASS && stat == ES_NOT_HANDLED)
			stat = ES_PASS;
	}

	// MW-2011-06-30: Cleanup of message path - this clause handles the transition
	//   through the home stack to dispatcher.
	if (passing_object != nil && (stat == ES_PASS || stat == ES_NOT_HANDLED) && parent != NULL)
	{
		Exec_stat oldstat = stat;
		if (MCModeHasHomeStack() || parent != MCdispatcher -> gethome() || !MCdispatcher->ismainstack(this))
		{
			// If the engine variant has a home stack (server / development) then we pass
			// to parent.
			// Otherwise we are one of the following:
			//   1) a substack of a stack which is not the home stack (parent != gethome()).
			//   2) the home stack (parent != gethome())
			//   3) a substack of the home stack (parent == gethome(), ismainstack() == false)
			// In all cases we want to pass to parent.
			stat = parent->handle(htype, message, params, this);
		}
		else if (parent->getparent() != NULL)
		{
			// It should be impossible for parent -> getparent() to be NULL. In any case, this
			// should always be MCdispatcher so we want to do this.
			stat = parent->getparent()->handle(htype, message, params, this);
		}
		if (oldstat == ES_PASS && stat == ES_NOT_HANDLED)
			stat = ES_PASS;
	}

	if (stat == ES_ERROR && MCerrorptr == NULL)
		MCerrorptr = this;

	return stat;
}

void MCStack::recompute()
{
	if (curcard != NULL)
		curcard->recompute();
}

///////////////////////////////////////////////////////////////////////////////

void MCStack::loadexternals(void)
{
	if (externalfiles == NULL || m_externals != NULL || !MCSecureModeCanAccessExternal())
		return;

	m_externals = new MCExternalHandlerList;

	char *ename = strclone(externalfiles);
	char *sptr = ename;
	while (*sptr)
	{
		char *tptr;
		if ((tptr = strchr(sptr, '\n')) != NULL)
			*tptr++ = '\0';
		else
			tptr = &sptr[strlen(sptr)];

		if (strlen(sptr) != 0)
		{
			uint4 offset = 0;
			m_externals -> Load(sptr);
			sptr = tptr;
		}
	}
	delete ename;

	// If the handler list is empty, then delete the object - thus preventing
	// its use in MCStack::handle.
	if (m_externals -> IsEmpty())
	{
		delete m_externals;
		m_externals = nil;
}
}

void MCStack::unloadexternals(void)
{
	if (m_externals == NULL)
		return;

	delete m_externals;
	m_externals = NULL;
}

// OK-2009-01-09: [[Bug 1161]]
// This function will attempt to resolve the specified filename relative to the stack
// and will either return an absolute path if the filename was found relative to the stack,
// or a copy of the original buffer. The returned buffer should be freed by the caller.
char *MCStack::resolve_filename(const char *filename)
{
	char *t_mode_filename;
	t_mode_filename = mode_resolve_filename(filename);
	if (t_mode_filename != NULL)
		return t_mode_filename;

	if (filename != NULL && filename[0] != '\0' && filename[0] != '/' && filename[1] != ':')
	{
		const char *t_stack_filename;
		t_stack_filename = getfilename();
		if (t_stack_filename == NULL)
		{
			MCStack *t_parent_stack;
			t_parent_stack = static_cast<MCStack *>(getparent());
			if (t_parent_stack != NULL)
				t_stack_filename = t_parent_stack -> getfilename();
		}
		if (t_stack_filename != NULL)
		{
			const char *t_last_separator;
			t_last_separator = strrchr(t_stack_filename, '/');
			if (t_last_separator != NULL)
			{
				char *t_filename;
				t_filename = new char[strlen(t_stack_filename) + strlen(filename) + 2];
				strcpy(t_filename, t_stack_filename);

				// If the relative path begins with "./" or ".\", we must remove this, otherwise
				// certain system calls will get confused by the path.
				const char *t_leaf;
				if (filename[0] == '.' && (filename[1] == '/' || filename[1] == '\\'))
					t_leaf = filename + 2;
				else
					t_leaf = filename;

				strcpy(t_filename + (t_last_separator - t_stack_filename + 1), t_leaf);

				if (MCS_exists(t_filename, True))
					return t_filename;
				else if (t_filename != NULL)
					delete t_filename;
			}
		}
	}
	return strdup(filename);
}

MCRectangle MCStack::recttoroot(const MCRectangle& p_rect)
{
	MCRectangle drect = p_rect;
	MCRectangle srect;
	srect = getrect();
	drect.x += srect.x;
	drect.y += srect.y - getscroll();
	return drect;
}

MCRectangle MCStack::rectfromroot(const MCRectangle& p_rect)
{
	MCRectangle drect = p_rect;
	MCRectangle srect;
	srect = getrect();
	drect . x -= srect . x;
	drect . y -= srect . y - getscroll();
	return drect;
}

// MW-2011-09-20: [[ Collision ]] The stack's shape is its rect. At some point it
//   might be need to update this to include the windowmask...
bool MCStack::lockshape(MCObjectShape& r_shape)
{
	r_shape . type = kMCObjectShapeRectangle;
	
	// Object shapes are in card-relative co-ords.
	r_shape . bounds = MCU_make_rect(0, getscroll(), rect . width, rect . height);
	r_shape . rectangle = r_shape . bounds;
	
	return true;
}

void MCStack::unlockshape(MCObjectShape& p_shape)
{
}

// MW-2012-02-14: [[ FontRefs ]] This method causes recursion throughout all the
//   children (cards, controls and substacks) of the stack updating the font
//   allocations.
bool MCStack::recomputefonts(MCFontRef p_parent_font)
{
	// MW-2012-02-17: [[ FontRefs ]] If the stack has formatForPrinting set,
	//   make sure all its children inherit from a font with printer metrics
	//   set.
	if (getflag(F_FORMAT_FOR_PRINTING))
	{
		MCNameRef t_textfont;
		uint2 t_textsize;
		uint2 t_textstyle;
		getfontattsnew(t_textfont, t_textsize, t_textstyle);

		MCFontStyle t_fontstyle;
		t_fontstyle = MCFontStyleFromTextStyle(t_textstyle) | kMCFontStylePrinterMetrics;

		MCFontRef t_printer_parent_font;
		/* UNCHECKED */ MCFontCreate(t_textfont, t_fontstyle, t_textsize, t_printer_parent_font);

		bool t_changed;
		t_changed = MCObject::recomputefonts(t_printer_parent_font);

		MCFontRelease(t_printer_parent_font);
	}
	else
	{
		// A stack's font is determined by the object, so defer there first and
		// only continue if something changes.
		if (!MCObject::recomputefonts(p_parent_font))
			return false;
	}

	// MW-2012-12-14: [[ Bug ]] Only recompute the card if we are open.
	// Now iterate through the current card, updating that.
	if (opened != 0 && curcard != nil)
		curcard -> recomputefonts(m_font);
	
	// Now iterate through all the sub-stacks, updating them.
	if (substacks != NULL)
	{
		MCStack *sptr = substacks;
		do
		{
			if (sptr -> getopened() != 0)
				sptr -> recomputefonts(m_font);
			sptr = sptr->next();
		}
		while (sptr != substacks);
	}
	
	// If we are in the IDE engine and this is the home stack, we must update all
	// stacks.
	if (MCModeHasHomeStack() && MCdispatcher -> gethome() == this)
	{
		MCStack *t_stack;
		t_stack = MCdispatcher -> gethome() -> next();
		if (t_stack != MCdispatcher -> gethome())
		{
			do
			{
				if (t_stack -> getopened() != 0)
					t_stack -> recomputefonts(m_font);
				t_stack = t_stack -> next();
			}
			while(t_stack != MCdispatcher -> gethome());
		}
	}

	// The return value indicates something changed. This is only used when
	// descending the hierarchy to eliminate unnecessary updates.
	return true;
}

void MCStack::purgefonts()
{
	recomputefonts(parent -> getfontref());
	recompute();
	
	// MW-2011-08-17: [[ Redraw ]] Tell the stack to dirty all of itself.
	dirtyall();
}

//////////

MCRectangle MCStack::getwindowrect(void) const
{
	if (window == nil)
		return rect;
		
	MCRectangle t_rect;
	t_rect = device_getwindowrect();
	
	return MCGRectangleGetIntegerBounds(MCResDeviceToUserRect(t_rect));
}

//////////

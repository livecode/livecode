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
#include "objdefs.h"
#include "parsedef.h"
#include "filedefs.h"
#include "mcio.h"

#include "scriptpt.h"
#include "execpt.h"
#include "handler.h"
#include "cmds.h"
#include "visual.h"
#include "chunk.h"
#include "mcerror.h"
#include "object.h"
#include "dispatch.h"
#include "stack.h"
#include "card.h"
#include "group.h"
#include "button.h"
#include "player.h"
#include "stacklst.h"
#include "cardlst.h"
#include "util.h"
#include "ans.h"
#include "debug.h"
#include "globals.h"
#include "securemode.h"
#include "redraw.h"

MCCompact::~MCCompact()
{
delete target;
}

Parse_stat MCCompact::parse(MCScriptPoint &sp)
{
	initpoint(sp);
	target = new MCChunk(False);
	if (target->parse(sp, False) != PS_NORMAL)
	{
		MCperror->add
		(PE_COMPACT_BADPARAM, sp);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

Exec_stat MCCompact::exec(MCExecPoint &ep)
{
#ifdef /* MCCompact */ LEGACY_EXEC
	MCObject *optr;
	uint4 parid;

	if (target->getobj(ep, optr, parid, True) != ES_NORMAL)
	{
		MCeerror->add
		(EE_COMPACT_NOTARGET, line, pos);
		return ES_ERROR;
	}
	if (optr->gettype() != CT_STACK)
	{
		MCeerror->add
		(EE_COMPACT_NOTASTACK, line, pos);
		return ES_ERROR;
	}
	MCStack *sptr = (MCStack *)optr;
	sptr->compact();
	return ES_NORMAL;
#endif /* MCCompact */
}

MCGo::~MCGo()
{
	while (stack != NULL)
	{
		MCCRef *tptr = stack;
		stack = stack->next;
		delete tptr;
	}
	delete stack;
	delete background;
	delete card;
	delete window;
}

Parse_stat MCGo::parse(MCScriptPoint &sp)
{
	Symbol_type type;
	Boolean need_target = True;
	const LT *te;
	MCCRef *curref = NULL;
	Chunk_term oterm = CT_UNDEFINED;
	Chunk_term nterm = CT_UNDEFINED;
	Chunk_term lterm = CT_UNDEFINED;

	initpoint(sp);
	if (sp.skip_token(SP_FACTOR, TT_PROPERTY, P_INVISIBLE) == PS_NORMAL)
		visible = False;
	while (True)
	{
		if (sp.next(type) != PS_NORMAL)
			if (need_target)
			{
				if (ct_class(oterm) == CT_ORDINAL)
				{
					card = new MCCRef;
					card->otype = CT_CARD;
					card->etype = oterm;
					return PS_NORMAL;
				}
				else
				{
					MCperror->add
					(PE_GO_NOID, sp);
					return PS_ERROR;
				}
			}
			else
				break;
		if (type == ST_ID && (sp.lookup(SP_FACTOR, te) == PS_NORMAL
		                      || sp.lookup(SP_GO, te) == PS_NORMAL))
		{
			switch (te->type)
			{
			case TT_PREP:
				if (need_target)
				{
					MCperror->add
					(PE_GO_BADPREP, sp);
					return PS_ERROR;
				}
				if (te->which == PT_AS)
				{
					if (sp.next(type) != PS_NORMAL
					        || type != ST_ID
					        || sp.lookup(SP_COMMAND, te) != PS_NORMAL
					        || te->type != TT_STATEMENT)
					{
						uint2 newmode;
						if (!MCU_stoui2(sp.gettoken(), newmode)
						        || newmode < 1 || newmode >= WM_LAST)
						{
							MCperror->add
							(PE_GO_NOMODE, sp);
							return PS_ERROR;
						}
						mode = (Window_mode)newmode;
						return PS_NORMAL;
					}
					switch (te->which)
					{
					case S_TOP_LEVEL:
						mode = WM_TOP_LEVEL;
						break;
					case S_MODELESS:
						mode = WM_MODELESS;
						break;
					case S_PALETTE:
						mode = WM_PALETTE;
						break;

					case S_MODAL:
						mode = WM_MODAL;
						break;
					case S_SHEET:
						mode = WM_SHEET;
						break;
					case S_DRAWER:
						mode = WM_DRAWER;
						break;
					case S_PULLDOWN:
						mode = WM_PULLDOWN;
						break;
					case S_POPUP:
						mode = WM_POPUP;
						break;
					case S_OPTION:
						mode = WM_OPTION;
						break;
					default:
						MCperror->add
						(PE_GO_NOMODE, sp);
						return PS_ERROR;
					}
				}
				else
				{
					sp.backup();
					sp.skip_token(SP_SUGAR, TT_PREP, PT_WITHOUT);
					sp.skip_token(SP_LOCK, TT_UNDEFINED, LC_ERRORS);
				}
				return PS_NORMAL;
			case TT_TO:
			case TT_THE:
				break;
			case TT_IN:
				sp.skip_token(SP_FACTOR, TT_THE);
				if (sp.skip_token(SP_FACTOR, TT_CHUNK, CT_STACK) == PS_NORMAL)
				{
					sp.skip_token(SP_FACTOR, TT_OF);
					if (sp.skip_token(SP_FACTOR, TT_CHUNK, CT_THIS) == PS_NORMAL)
					{
						sp.skip_token(SP_FACTOR, TT_CHUNK, CT_STACK);
						thisstack = True;
						return PS_NORMAL;
					}
					sp.skip_token(SP_FACTOR, TT_CHUNK, CT_STACK);
					if (sp.parseexp(False, True, &window) != PS_NORMAL)
					{
						MCperror->add
						(PE_GO_BADWINDOW, sp);
						return PS_ERROR;
					}
					return PS_NORMAL;
				}
				sp.skip_token(SP_VALIDATION, TT_UNDEFINED, IV_UNDEFINED); // a
				sp.skip_token(SP_SUGAR, TT_CHUNK, CT_UNDEFINED);          // new
				sp.skip_token(SP_FACTOR, TT_CHUNK, CT_STACK);             // window
				sp.skip_token(SP_SUGAR, TT_PREP, PT_WITHOUT);             // without
				sp.skip_token(SP_LOCK, TT_UNDEFINED, LC_ERRORS);          // dialog
				return PS_NORMAL;
			case TT_OF:
				need_target = True;
				break;
			case TT_CLASS:
				if (te->which != CT_MARKED)
				{
					MCperror->add
					(PE_GO_NODEST, sp);
					return PS_ERROR;
				}
				marked = True;
				break;
			case TT_CHUNK:
				nterm = (Chunk_term)te->which;
				switch (ct_class(nterm))
				{
				case CT_DIRECT:
					switch (nterm)
					{
					case CT_BACKWARD:
					case CT_FORWARD:
						{
							card = new MCCRef;
							card->etype = nterm;
							MCScriptPoint oldsp(sp);
							MCerrorlock++;
							if (sp.parseexp(False, True, &card->startpos) != PS_NORMAL)
							{
								sp = oldsp;
								delete card->startpos;
								card->startpos = NULL;
							}
							MCerrorlock--;
						}
						break;
					case CT_START:
					case CT_FINISH:
						card = new MCCRef;
						card->etype = nterm;
						sp.skip_token(SP_FACTOR, TT_CHUNK, CT_CARD);
						break;
					case CT_HOME:
					case CT_HELP:
						stack = new MCCRef;
						stack->etype = nterm;
						break;
					default:
						MCperror->add
						(PE_GO_BADDIRECT, sp);
						return PS_ERROR;
					}
					return PS_NORMAL;
				case CT_ORDINAL:
					oterm = nterm;
					break;
				case CT_TYPES:
					if (lterm != CT_UNDEFINED && nterm > lterm)
					{
						MCperror->add
						(PE_GO_BADCHUNKORDER, sp);
						return PS_ERROR;
					}
					curref = new MCCRef;
					if (oterm == CT_UNDEFINED)
					{
						if (nterm >= CT_FIELD || nterm == CT_URL)
						{
							sp.backup();
							nterm = CT_CARD;
							curref->etype = CT_EXPRESSION;
						}
						else
							if (sp.skip_token(SP_FACTOR, TT_PROPERTY) == PS_NORMAL)
								curref->etype = CT_ID;
							else
								curref->etype = CT_EXPRESSION;
						if (sp.parseexp(False, True, &curref->startpos) != PS_NORMAL)
						{
							delete curref;
							MCperror->add
							(PE_GO_BADCHUNKEXP, sp);
							return PS_ERROR;
						}
					}
					else
					{
						curref->etype = oterm;
						oterm = CT_UNDEFINED;
					}
					curref->otype = nterm;
					switch (nterm)
					{
					case CT_STACK:
						if (oterm > CT_STACK)
						{
							MCperror->add
							(PE_GO_DUPCHUNK, sp);
							delete curref;
							return PS_ERROR;
						}
						curref->next = stack;
						stack = curref;
						break;
					case CT_BACKGROUND:
						background = curref;
						break;
					case CT_CARD:
						card = curref;
						break;
					default: /* chunk type */
						MCperror->add
						(PE_GO_BADCHUNKDEST, sp);
						delete curref;
						return PS_ERROR;
					}
					lterm = nterm;
					need_target = False;
					break;
				default: /* chunk class */
					MCperror->add
					(PE_GO_BADCHUNKTYPE, sp);
					return PS_ERROR;
				}
				break;
			default: /* factor */
				if (need_target)
				{
					if (stack != NULL || card != NULL || background != NULL)
					{
						MCperror->add
						(PE_GO_NODEST, sp);
						return PS_ERROR;
					}
					sp.backup();
					card = new MCCRef;
					card->otype = CT_CARD;
					card->etype = CT_EXPRESSION;
					if (sp.parseexp(False, True, &card->startpos) != PS_NORMAL)
					{
						MCperror->add
						(PE_GO_BADCHUNKEXP, sp);
						return PS_ERROR;
					}
				}
				else
					sp.backup();
				return PS_NORMAL;
			}
		}
		else
		{ /* not id token type */
			if (need_target)
			{
				if (stack != NULL || card != NULL || background != NULL)
				{
					MCperror->add
					(PE_GO_NODEST, sp);
					return PS_ERROR;
				}
				sp.backup();
				stack = new MCCRef;
				stack->otype = CT_STACK;
				stack->etype = CT_EXPRESSION;
				if (sp.parseexp(False, True, &stack->startpos) != PS_NORMAL)
				{
					MCperror->add
					(PE_GO_BADCHUNKEXP, sp);
					return PS_ERROR;
				}
				need_target = False;
			}
			else
			{
				sp.backup();
				break;
			}
		}
	}
	return PS_NORMAL;
}
#ifdef /* MCGo::findstack */ LEGACY_EXEC
MCStack *MCGo::findstack(MCExecPoint &ep, Chunk_term etype, MCCard *&cptr)
{
	MCStack *sptr = NULL;

	MCresult->clear(False);
	uint4 offset;
	if (MCU_offset(SIGNATURE, ep.getsvalue(), offset) || (ep . getsvalue() . getlength() > 8 && strncmp(ep . getsvalue() . getstring(), "REVO", 4) == 0))
	{
		IO_handle stream = MCS_fakeopen(ep.getsvalue());
		if (MCdispatcher->readfile(NULL, NULL, stream, sptr) != IO_NORMAL)
		{
			MCS_close(stream);
			if (MCresult->isclear())
				MCresult->sets("can't build stack from string");
			return NULL;
		}
		MCS_close(stream);
		return sptr;
	}
	if (etype == CT_STACK)
		return NULL;
	if (etype == CT_ID)
		sptr = MCdefaultstackptr->findstackid(ep.getuint4());
	else
		sptr = MCdefaultstackptr->findstackname(ep.getsvalue());
	if (sptr != NULL)
		return sptr;
	MCObject *objptr;
	MCChunk *tchunk = new MCChunk(False);
	MCerrorlock++;
	MCScriptPoint sp(ep);
	Parse_stat stat = tchunk->parse(sp, False);
	if (stat == PS_NORMAL)
	{
		uint4 parid;
		if (tchunk->getobj(ep, objptr, parid, True) == ES_NORMAL)
			stat = PS_NORMAL;
		else
			stat = PS_ERROR;
	}
	MCerrorlock--;
	delete tchunk;
	if (stat == PS_NORMAL)
	{
		switch (objptr->gettype())
		{
		case CT_STACK:
			sptr = (MCStack *)objptr;
			break;
		case CT_CARD:
			cptr = (MCCard *)objptr;
			sptr = cptr->getstack();
			break;
		default:
			break;
		}
	}
	else
		if (MCresult->isclear())
			MCresult->sets("no such card");
	return sptr;
}
#endif /* MCGo::findstack */

Exec_stat MCGo::exec(MCExecPoint &ep)
{
#ifdef /* MCGo */ LEGACY_EXEC
	MCStack *sptr = MCdefaultstackptr;
	MCControl *bptr = NULL;
	MCCard *cptr = NULL;
	MCRectangle rel;
	MCStack *parentptr;

	// MW-2011-02-27: [[ Bug ]] Make sure that if we open as a sheet, we have a parent pointer!
	if (ep.getobj()->getstack()->getopened() || MCtopstackptr == NULL)
		parentptr = ep.getobj() -> getstack();
	else
		parentptr = MCtopstackptr;

	rel = parentptr -> getrect();

	MCresult->clear(False);
	
	MCString nc = "No such card";
	if (stack == NULL && background == NULL && card == NULL)
	{
		MCeerror->add(EE_GO_NODEST, line, pos);
		return ES_ERROR;
	}

	if (stack != NULL)
	{
		switch (stack->etype)
		{
		case CT_HELP:
			sptr = MCdefaultstackptr->findstackname(MChelpnamestring);
			if (sptr == NULL)
			{
				if (MCresult->isclear())
					MCresult->sets(nc);
				return ES_NORMAL;
			}
			break;
		case CT_HOME:
			sptr = MCdispatcher->gethome();
			if (sptr != MCdefaultstackptr)
			{
				MCdefaultstackptr->close();
				MCdefaultstackptr->checkdestroy();
			}
			break;
		case CT_THIS:
		case CT_EXPRESSION:
		case CT_ID:
			if (stack->etype == CT_THIS)
				sptr = MCdefaultstackptr;
			else
			{
				if (stack->startpos->eval(ep) != ES_NORMAL)
				{
					MCeerror->add(EE_GO_BADSTACKEXP, line, pos);
					return ES_ERROR;
				}
				sptr = findstack(ep, stack->etype, cptr);
			}
			if (sptr != NULL && stack->next != NULL)
			{
				switch (stack->next->etype)
				{
				case CT_ID:
				case CT_EXPRESSION:
					if (stack->next->startpos->eval(ep) != ES_NORMAL)
					{
						MCeerror->add(EE_CHUNK_BADSTACKEXP, line, pos);
						return ES_ERROR;
					}
					if (stack->next->etype == CT_ID)
						sptr = sptr->findsubstackid(ep.getuint4());
					else
						sptr = sptr->findsubstackname(ep.getsvalue());
					break;
				default:
					MCeerror->add(EE_CHUNK_BADSTACKEXP, line, pos);
					return ES_ERROR;
				}
			}
			if (sptr == NULL)
			{
				if (MCresult->isclear())
					MCresult->sets(nc);
				return ES_NORMAL;
			}
			break;
		default:
			return ES_ERROR;
		}
		if (mode == WM_PULLDOWN || mode == WM_POPUP || mode == WM_OPTION)
		{
			MCButton *bptr = (MCButton *)ep.getobj();
			if (ep.getobj()->gettype() == CT_BUTTON && bptr->attachmenu(sptr))
				rel = MCU_recttoroot(bptr->getstack(), bptr->getrect());
			else
			{
				MCeerror->add(EE_GO_CANTATTACH, line, pos);
				return ES_ERROR;
			}
		}
	}
	sptr->stopedit();

	if (background != NULL)
	{
		switch (ct_class(background->etype))
		{
		case CT_ORDINAL:
			bptr = sptr->getbackground(background->etype, MCnullmcstring, CT_GROUP);
			break;
		case CT_ID:
		case CT_EXPRESSION:
			if (background->startpos->eval(ep) != ES_NORMAL)
			{
				MCeerror->add(EE_GO_BADBACKGROUNDEXP, line, pos);
				return ES_ERROR;
			}
			bptr = sptr->getbackground(background->etype, ep.getsvalue(), CT_GROUP);
			break;
		default:
			break;
		}
		if (bptr == NULL)
		{
			if (MCresult->isclear())
				MCresult->sets(nc);
			return ES_NORMAL;
		}
		sptr->setbackground(bptr);
	}

	int2 i;
	if (card != NULL)
		switch (ct_class(card->etype))
		{
		case CT_DIRECT:
			sptr->clearbackground();
			switch (card->etype)
			{
			case CT_BACKWARD:
			case CT_FORWARD:
				real8 n;
				if (card->startpos != NULL)
				{
					if (card->startpos->eval(ep) != ES_NORMAL || ep.ton() != ES_NORMAL)
					{
						sptr->clearbackground();
						if (MCresult->isclear())
							MCresult->sets(nc);
						return ES_NORMAL;
					}
					n = ep.getnvalue();
				}
				else
					n = 1.0;
				i = (int2)(card->etype == CT_FORWARD ? n : -n);
				MCrecent->gorel(i);
				break;
			case CT_START:
			case CT_FINISH:
				MCrecent->godirect(card->etype == CT_START);
				break;
			default:
				return ES_ERROR;
			}
			return ES_NORMAL;
		case CT_ORDINAL:
			if (card->etype == CT_RECENT)
			{
				MCrecent->gorel(-1);
				return ES_NORMAL;
			}
			else
			{
				if (marked)
					sptr->setmark();
				cptr = sptr->getchild(card->etype, MCnullmcstring, card->otype);
			}
			break;
		case CT_ID:
		case CT_EXPRESSION:
			if (card->startpos->eval(ep) != ES_NORMAL)
			{

				sptr->clearbackground();
				if (MCresult->isclear())
					MCresult->sets(nc);
				return ES_NORMAL;
			}
			if (marked)
				sptr->setmark();
			cptr = sptr->getchild(card->etype, ep.getsvalue(), card->otype);
			if (cptr == NULL)
			{
				sptr = findstack(ep, CT_STACK, cptr);
				if (sptr == NULL)
				{
					if (MCresult->isclear())
						MCresult->sets(nc);
					return ES_NORMAL;
				}
				cptr = (MCCard *)sptr->getchild(CT_THIS, MCnullmcstring, CT_CARD);
			}
			break;
		default:
			sptr->clearbackground();
			fprintf(stderr, "Go: ERROR no card type %d\n", card->etype);
			return ES_ERROR;
		}
	else
		if (cptr == NULL)
			cptr = (MCCard *)sptr->getchild(CT_THIS, MCnullmcstring, CT_CARD);

	Window_mode wm = mode;
	if (wm == WM_LAST && sptr->userlevel() != 0 && window == NULL && !thisstack)
		wm = (Window_mode)(sptr->userlevel() + WM_TOP_LEVEL_LOCKED);

	sptr->clearmark();
	sptr->clearbackground();

	uint2 oldw = sptr->getrect().width;
	uint2 oldh = sptr->getrect().height;
	if (cptr == NULL)
	{
		if (MCresult->isclear())
			MCresult->sets(nc);
		return ES_NORMAL;
	}

	// Here 'oldstack' is the pointer to the stack's window we are taking over.
	// If it turns out NULL then we aren't subverting another stacks' window to
	// our cause :o)
	MCStack *oldstack = NULL;
	if (window != NULL || thisstack)
	{
		Window w = DNULL;
		if (thisstack)
		{
			oldstack = MCdefaultstackptr;
			w = oldstack->getwindow();
		}
		else
		{
			if (window->eval(ep) != ES_NORMAL)
			{
				MCeerror->add(EE_GO_BADWINDOWEXP, line, pos);
				return ES_ERROR;
			}
			if (ep.ton() == ES_NORMAL && MCscreen->uint4towindow(ep.getuint4(), w))
				oldstack = MCdispatcher->findstackd(w);
			else
				oldstack = ep.getobj()->getstack()->findstackname(ep.getsvalue());
		}
		
		if (oldstack == NULL || !oldstack->getopened())
		{
			MCeerror->add(EE_GO_BADWINDOWEXP, line, pos);
			return ES_ERROR;
		}
		
		if (oldstack == sptr)
			oldstack = NULL;
		else
		{
			// MW-2011-10-01: [[ Effects ]] Snapshot the old stack window.
			if (!MCRedrawIsScreenLocked() && MCcur_effects != NULL)
				oldstack -> snapshotwindow(oldstack -> getcurcard() -> getrect());
			
			// MW-2011-10-01: [[ Redraw ]] Lock the screen until we are done.
			MCRedrawLockScreen();
			
			// MW-2012-09-19: [[ Bug 10383 ]] Use the 'real' mode - otherwise we get one
			//   modified for ICONIC or CLOSED states which screw things up a bit!
			wm = oldstack->getrealmode();
			if (wm == WM_MODAL || wm == WM_SHEET)
			{
				MCRedrawUnlockScreen();
				MCeerror->add(EE_GO_BADWINDOWEXP, line, pos);
				return ES_ERROR;
			}
			oldstack->kunfocus();
			sptr->close();
			
			MCPlayer *tptr = MCplayers;
			while (tptr != NULL)
			{
				MCPlayer *oldptr = tptr;
				tptr = tptr->getnextplayer();
				if (oldptr->getstack() == oldstack)
					oldptr->close();
			}

			if (!sptr->takewindow(oldstack))
			{
				MCRedrawUnlockScreen();
				MCeerror->add(EE_GO_BADWINDOWEXP, line, pos);
				return ES_ERROR;
			}
		}
	}
	else if (mode != WM_LAST && wm >= WM_MODELESS)
	{
		// MW-2011-08-18: [[ Redraw ]] Move to use redraw lock/unlock.
		MCRedrawForceUnlockScreen();
	}

	Boolean oldtrace = MCtrace;
	MCtrace = False;

	// MW-2007-02-11: [[ Bug 4029 ]] - 'go invisible' fails to close stack window if window already open
	if (!visible && sptr -> getflag(F_VISIBLE))
	{
		if (sptr -> getwindow() != NULL)
			MCscreen -> closewindow(sptr -> getwindow());
		sptr->setflag(False, F_VISIBLE);
	}

	// MW-2011-02-27: [[ Bug ]] Make sure that if we open as a sheet, we have a parent pointer!
	if (wm != WM_SHEET && wm != WM_DRAWER)
		parentptr = nil;

	Exec_stat stat = ES_NORMAL;
	Boolean added = False;
	if (MCnexecutioncontexts < MAX_CONTEXTS)
	{
		ep.setline(line);
		MCexecutioncontexts[MCnexecutioncontexts++] = &ep;
		added = True;
	}

#ifdef _MOBILE
	// MW-2011-01-30: [[ Effects ]] On Mobile, we must twiddle with snapshots to
	//   ensure go stack with visual effect works.
	if (oldstack == nil && MCcur_effects != nil && MCdefaultstackptr != sptr)
	{
		MCdefaultstackptr -> snapshotwindow(MCdefaultstackptr -> getcurcard() -> getrect());
		sptr -> takewindowsnapshot(MCdefaultstackptr);
		MCRedrawLockScreen();
	}
#endif	
	
	if (sptr->setcard(cptr, True, True) == ES_ERROR
	        || sptr->openrect(rel, wm, parentptr, WP_DEFAULT, OP_NONE) == ES_ERROR)
	{
		MCtrace = oldtrace;
		stat = ES_ERROR;
	}
	
	if (oldstack != NULL)
	{
		MCRectangle trect = sptr->getcurcard()->getrect();
		sptr->getcurcard()->message_with_args(MCM_resize_stack, trect.width, trect.height, oldw, oldh);
		
		MCRedrawUnlockScreen();
		
		if (MCcur_effects != nil)
		{
			Boolean t_abort;
			sptr -> effectrect(sptr -> getcurcard() -> getrect(), t_abort);
		}
		
		Boolean oldlock = MClockmessages;
		MClockmessages = True;
		oldstack->close();
		MClockmessages = oldlock;
		sptr->kfocus();
	}
	
#ifdef _MOBILE
	// MW-2011-01-30: [[ Effects ]] Apply any stack level visual efect.
	if (oldstack == nil && MCcur_effects != nil && MCdefaultstackptr != sptr)
	{
		MCRedrawUnlockScreen();
		
		// MW-2011-10-17: [[ Bug 9811 ]] Make sure we configure the new card now.
		MCRedrawDisableScreenUpdates();
		sptr -> configure(True);
		MCRedrawEnableScreenUpdates();
			
		Boolean t_abort;
		sptr -> effectrect(sptr -> getcurcard() -> getrect(), t_abort);
	}
#endif	

	if (added)
		MCnexecutioncontexts--;
	
	MCtrace = oldtrace;
	if (sptr->getmode() == WM_TOP_LEVEL || sptr->getmode() == WM_TOP_LEVEL_LOCKED)
		MCdefaultstackptr = sptr;
	if (MCmousestackptr != NULL)
		MCmousestackptr->resetcursor(True);
	if (MCabortscript)
		return ES_ERROR;
	return stat;
#endif /* MCGo */
}

MCHide::~MCHide()
{
	delete object;
	delete effect;
}

Parse_stat MCHide::parse(MCScriptPoint &sp)
{
	initpoint(sp);
	Symbol_type type;
	const LT *te;

	initpoint(sp);
	if (sp.next(type) == PS_NORMAL)
		if (sp.lookup(SP_SHOW, te) == PS_NORMAL)
		{
			if (te->which == SO_CARD || te->which == SO_BACKGROUND)
			{
				MCScriptPoint oldsp(sp);
				if (sp.skip_token(SP_FACTOR, TT_CLASS, CT_CARD) == PS_NORMAL)
				{
					card = (Show_object)te->which == SO_CARD;
					which = SO_PICTURE;
					return PS_NORMAL;
				}
				if (sp.skip_token(SP_SHOW, TT_UNDEFINED, SO_WINDOW) == PS_NORMAL)
				{
					which = SO_PALETTE;
					return PS_NORMAL;
				}
				sp = oldsp;
				sp.backup();
			}
			else
			{
				which = (Show_object)te->which;
				if (which == SO_GROUPS)
					return PS_NORMAL;
				if (which == SO_WINDOW)
				{
					sp.backup();
					which = SO_OBJECT;
				}
				else
					if (which == SO_PICTURE)

						sp.skip_token(SP_FACTOR, TT_OF);
					else
					{
						while (sp.skip_token(SP_SHOW, TT_UNDEFINED) == PS_NORMAL)
							;
						return PS_NORMAL;
					}
			}
		}
		else
			sp.backup();
	else
	{
		MCperror->add
		(PE_HIDE_BADTARGET, sp);
		return PS_ERROR;
	}
	object = new MCChunk(False);
	if (object->parse(sp, False) != PS_NORMAL)
	{
		MCperror->add
		(PE_HIDE_BADTARGET, sp);
		return PS_ERROR;
	}
	if (sp.skip_token(SP_REPEAT, TT_UNDEFINED) == PS_NORMAL)
	{
		sp.skip_token(SP_COMMAND, TT_STATEMENT, S_VISUAL);
		effect = new MCVisualEffect;
		if (effect->parse(sp) != PS_NORMAL)
		{
			MCperror->add
			(PE_HIDE_BADEFFECT, sp);
			return PS_ERROR;
		}
	}
	return PS_NORMAL;
}

Exec_stat MCHide::exec(MCExecPoint &ep)
{
#ifdef /* MCHide */ LEGACY_EXEC
	switch (which)
	{
	case SO_GROUPS:
		MClinkatts.underline = False;
		
		// MW-2011-08-17: [[ Redraw ]] We've changed a global property that could
		//   affect the display of all stacks.
		MCRedrawDirtyScreen();
		break;
	case SO_OBJECT:
		MCObject *optr;
		uint4 parid;
		if (object->getobj(ep, optr, parid, True) != ES_NORMAL)
		{
			MCeerror->add(EE_HIDE_NOOBJ, line, pos);
			return ES_ERROR;
		}
			
		// MW-2011-09-13: [[ Effects ]] Only apply the effect if the screen is not
		//   locked.
		if (effect != NULL && !MCRedrawIsScreenLocked())
		{
			if (effect->exec(ep) != ES_NORMAL)
			{
				MCeerror->add(EE_HIDE_BADEFFECT, line, pos);
				return ES_NORMAL;
			}
			
			// MW-2010-04-26: [[ Bug 8661 ]] Make sure we use the effective rect for
			//   effectarea computation.
			MCRectangle t_rect;
			if (optr -> gettype() >= CT_GROUP)
				t_rect = static_cast<MCControl *>(optr) -> geteffectiverect();
			else
				t_rect = optr -> getrect();
			
			// MW-2011-09-13: [[ Effects ]] Cache the rect we want to play the effect
			//   in.
			optr -> getstack() -> snapshotwindow(t_rect);
			
			// MW-2011-11-15: [[ Bug 9846 ]] Lock the screen to prevent the snapshot
			//   being dumped inadvertantly.
			MCRedrawLockScreen();
			
			// MW-2011-11-15: [[ Bug 9846 ]] Make sure we use the same mechanism to
			//   set visibility as the non-effect case.
			optr->setsprop(P_VISIBLE, MCfalsemcstring);
			
			MCRedrawUnlockScreen();
			
			// Run the effect - this will use the previously cached image.
			Boolean abort = False;
			optr -> getstack() -> effectrect(t_rect, abort);
			
			if (abort)
			{
				MCeerror->add(EE_HANDLER_ABORT, line, pos);
				return ES_ERROR;
			}
		}
		else
			optr->setsprop(P_VISIBLE, MCfalsemcstring);
		break;
	case SO_MENU:
		MCscreen->hidemenu();
		break;
	case SO_TASKBAR:
		MCscreen->hidetaskbar();
		break;
	case SO_MESSAGE:
		{
			MCStack *mb = ep.getobj()->getstack()->findstackname(MCmessagenamestring);
			if (mb != NULL)
				mb->close();
		}
		break;
	default:
		break;
	}
	return ES_NORMAL;
#endif /* MCHide */
}

MCLock::~MCLock(void)
{
	delete rect;
}

Parse_stat MCLock::parse(MCScriptPoint &sp)
{
	Symbol_type type;
	const LT *te;

	initpoint(sp);
	if (sp.next(type) != PS_NORMAL)
	{
		MCperror->add(PE_LOCK_NOTARGET, sp);
		return PS_ERROR;
	}
	if (sp.lookup(SP_LOCK, te) != PS_NORMAL)
	{
		MCperror->add(PE_LOCK_NOTTARGET, sp);
		return PS_ERROR;
	}
	which = (Lock_constants)te->which;
	if (which == LC_ERRORS)
		sp.skip_token(SP_LOCK, TT_UNDEFINED);
	else if (which == LC_SCREEN)
	{
		// MW-2011-09-13: [[ Effects ]] Add a 'lock screen for effect' clause.
		if (sp . skip_token(SP_REPEAT, TT_UNDEFINED, RF_FOR) == PS_NORMAL)
		{
			if (sp . skip_token(SP_COMMAND, TT_STATEMENT, S_VISUAL) == PS_NORMAL &&
				sp . skip_token(SP_VISUAL, TT_VISUAL, VE_EFFECT) == PS_NORMAL)
			{
				// MW-2011-09-24: [[ Effects ]] Add an optional 'in rect <rect>' clause.
				if (sp . skip_token(SP_FACTOR, TT_IN, PT_IN) == PS_NORMAL)
				{
					if (sp . skip_token(SP_FACTOR, TT_PROPERTY, P_RECTANGLE) != PS_NORMAL)
					{
						MCperror -> add(PE_LOCK_NORECT, sp);
						return PS_ERROR;
					}
					
					if (sp . parseexp(False, True, &rect) != PS_NORMAL)
					{
						MCperror -> add(PE_LOCK_BADRECT, sp);
						return PS_ERROR;
					}
				}
				
				which = LC_SCREEN_FOR_EFFECT;
			}
			else 
			{
				MCperror->add(PE_LOCK_NOTTARGET, sp);
				return PS_ERROR;
			}
		}
	}
			
	return PS_NORMAL;
}

Exec_stat MCLock::exec(MCExecPoint &ep)
{
#ifdef /* MCLock */ LEGACY_EXEC
	switch(which)
	{
	case LC_COLORMAP:
		MClockcolormap = True;
		break;
	case LC_CURSOR:
		MClockcursor = True;
		break;
	case LC_ERRORS:
		MClockerrors = True;
		MCerrorlockptr = ep.getobj();
		break;
	case LC_MENUS:
		MClockmenus = True;
		break;
	case LC_MSGS:
		MClockmessages = True;
		break;
	case LC_MOVES:
		MClockmoves = True;
		break;
	case LC_RECENT:
		MClockrecent = True;
		break;
	case LC_SCREEN:
		MCRedrawLockScreen();
		break;
	// MW-2011-09-13: [[ Effects ]] If the screen is not locked capture a snapshot
	//   of the default stack.
	case LC_SCREEN_FOR_EFFECT:
		if (!MCRedrawIsScreenLocked())
		{
			// MW-2011-09-24: [[ Effects ]] Process the 'rect' clause (if any).
			if (rect == nil)
				MCcur_effects_rect = MCdefaultstackptr -> getcurcard() -> getrect();
			else
			{
				int2 i1, i2, i3, i4;
				
				if (rect -> eval(ep) != ES_NORMAL)
				{
					MCeerror -> add(EE_LOCK_BADRECT, line, pos);
					return ES_ERROR;
				}
				
				if (!MCU_stoi2x4(ep . getsvalue(), i1, i2, i3, i4))
				{
					MCeerror->add(EE_LOCK_NAR, 0, 0, ep . getsvalue());
					return ES_ERROR;
				}
				
				MCU_set_rect(MCcur_effects_rect, i1, i2, i3 - i1, i4 - i2);
			}

			MCdefaultstackptr -> snapshotwindow(MCcur_effects_rect);
		}
		MCRedrawLockScreen();
		break;
	default:
		break;
	}
	return ES_NORMAL;
#endif /* MCLock */
}

MCPop::~MCPop()
{
	delete dest;
}

Parse_stat MCPop::parse(MCScriptPoint &sp)
{
	Symbol_type type;
	const LT *te;

	initpoint(sp);
	if (sp.skip_token(SP_FACTOR, TT_CHUNK) != PS_NORMAL)
	{
		MCperror->add
		(PE_POP_NOCARD, sp);
		return PS_ERROR;
	}
	if (sp.next(type) != PS_NORMAL)
		return PS_NORMAL;
	if (sp.lookup(SP_FACTOR, te) != PS_NORMAL || te->type != TT_PREP)
	{
		sp.backup();
		return PS_NORMAL;
	}
	prep = (Preposition_type)te->which;

	if (prep != PT_BEFORE && prep != PT_INTO && prep != PT_AFTER)
	{
		MCperror->add
		(PE_POP_BADPREP, sp);
		return PS_ERROR;
	}
	dest = new MCChunk(True);
	if (dest->parse(sp, False) != PS_NORMAL)
	{
		MCperror->add
		(PE_POP_BADCHUNK, sp);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

Exec_stat MCPop::exec(MCExecPoint &ep)
{
#ifdef /* MCPop */ LEGACY_EXEC
	MCCard *cptr = MCcstack->popcard();
	if (dest == NULL)
	{
		MCStack *sptr = cptr->getstack();
		//    if (sptr != MCdefaultstackptr) {
		//      MCdefaultstackptr->close();
		//      MCdefaultstackptr->checkdestroy();
		//    }
		MCdefaultstackptr = sptr;
		Boolean oldtrace = MCtrace;
		MCtrace = False;
		if (sptr->setcard(cptr, True, False) == ES_ERROR
		        || sptr->openrect(sptr->getrect(), WM_LAST, NULL, WP_DEFAULT, OP_NONE) == ES_ERROR)
		{
			MCtrace = oldtrace;
			return ES_ERROR;
		}
		MCtrace = oldtrace;
	}
	else
	{
		cptr->getprop(0, P_LONG_ID, ep, False);
		if (dest->set
		        (ep, prep) != ES_NORMAL)
		{
			MCeerror->add
			(EE_POP_CANTSET, line, pos);
			return ES_ERROR;
		}
	}
	return ES_NORMAL;
#endif /* MCPop */
}

MCPush::~MCPush()
{
	delete card;
}

// Syntax:
//   push recent [ card ]
//   push card
//   push <chunk>
Parse_stat MCPush::parse(MCScriptPoint &sp)
{
	initpoint(sp);
	if (sp.skip_token(SP_FACTOR, TT_CHUNK, CT_RECENT) == PS_NORMAL)
	{
		recent = True;
		sp.skip_token(SP_FACTOR, TT_CHUNK, CT_CARD);
	}
	else
	{
		// MW-2007-07-03: [[ Bug 5339 ]] First attempt to parse 'card' on its own, and
		//   if this fails, backup and attempt to parse a chunk.
		if (sp . skip_token(SP_FACTOR, TT_CHUNK, CT_CARD) == PS_NORMAL)
		{
			Symbol_type t_type;
			Parse_stat t_stat;
			t_stat = sp . next(t_type);
			if (t_stat == PS_EOL || t_stat == PS_EOF)
				return PS_NORMAL;

			sp . backup();
			sp . backup();
		}

		card = new MCChunk(False);
		if (card -> parse(sp, False) != PS_NORMAL)
		{
			MCperror->add(PE_PUSH_BADEXP, sp);
			return PS_ERROR;
		}
	}
	return PS_NORMAL;
}

Exec_stat MCPush::exec(MCExecPoint &ep)
{
#ifdef /* MCPush */ LEGACY_EXEC
	MCObject *optr;
	uint4 parid;
	if (card == NULL)
		if (recent)
			MCcstack->pushcard(MCrecent->getrel(0));
		else
			MCcstack->pushcard(MCdefaultstackptr->getcurcard());
	else
	{
		if (card->getobj(ep, optr, parid, True) != ES_NORMAL)
		{
			MCeerror->add(EE_PUSH_NOTARGET, line, pos);
			return ES_ERROR;
		}
		if (optr->gettype() != CT_CARD)
		{
			MCeerror->add(EE_PUSH_NOTACARD, line, pos);
			return ES_ERROR;
		}
		MCcstack->pushcard((MCCard *)optr);
	}
	return ES_NORMAL;
#endif /* MCPush */
}

MCSave::~MCSave()
{
	delete target;
	delete filename;
}

Parse_stat MCSave::parse(MCScriptPoint &sp)
{
	Symbol_type type;
	const LT *te;

	initpoint(sp);
	target = new MCChunk(False);
	if (target->parse(sp, False) != PS_NORMAL)
	{
		MCperror->add
		(PE_SAVE_BADEXP, sp);
		return PS_ERROR;
	}
	if (sp.next(type) != PS_NORMAL)
		return PS_NORMAL;
	if (sp.lookup(SP_FACTOR, te) != PS_NORMAL || te->which != PT_AS)
	{
		sp.backup();
		return PS_NORMAL;
	}
	if (sp.parseexp(False, True, &filename) != PS_NORMAL)
	{
		MCperror->add
		(PE_SAVE_BADFILEEXP, sp);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

Exec_stat MCSave::exec(MCExecPoint &ep)
{
#ifdef /* MCSave */ LEGACY_EXEC
	MCObject *optr;
	uint4 parid;

	MCresult->clear(False);
	if (MCsecuremode & MC_SECUREMODE_DISK)
	{
		MCeerror->add
		(EE_DISK_NOPERM, line, pos);
		return ES_ERROR;
	}
	if (target->getobj(ep, optr, parid, True) != ES_NORMAL)
	{
		MCeerror->add
		(EE_SAVE_NOTARGET, line, pos);
		return ES_ERROR;
	}
	if (optr->gettype() != CT_STACK)
	{
		MCeerror->add
		(EE_SAVE_NOTASTACK, line, pos);
		return ES_ERROR;
	}
	MCStack *sptr = (MCStack *)optr;
	if (filename != NULL)
	{
		if (filename->eval(ep) != ES_NORMAL)
		{
			MCeerror->add
			(EE_SAVE_BADNOFILEEXP, line, pos);
			return ES_ERROR;
		}
	}
	else
		ep.clear();
	sptr->saveas(ep.getsvalue());
	return ES_NORMAL;
#endif /* MCSave */
}

MCShow::~MCShow()
{
	delete ton;
	delete location;
	delete effect;
}

Parse_stat MCShow::parse(MCScriptPoint &sp)
{
	Symbol_type type;
	const LT *te;

	initpoint(sp);
	if (sp.next(type) == PS_NORMAL)
		if (sp.lookup(SP_SHOW, te) == PS_NORMAL)
		{
			which = (Show_object)te->which;
			switch (which)
			{
			case SO_CARD:
			case SO_BACKGROUND:
				if (sp.skip_token(SP_SHOW, TT_UNDEFINED, SO_PICTURE) == PS_NORMAL)
				{
					card = which == SO_CARD;
					which = SO_PICTURE;
					return PS_NORMAL;
				}
				if (sp.skip_token(SP_SHOW, TT_UNDEFINED, SO_WINDOW) == PS_NORMAL)
				{
					which = SO_PALETTE;
					return PS_NORMAL;
				}
				sp.backup();
				which = SO_OBJECT;
				break;
			case SO_ALL:
			case SO_MARKED:
				if (sp.skip_token(SP_FACTOR,
				                  TT_CLASS, CT_CARD) != PS_NORMAL)
				{
					MCperror->add
					(PE_SHOW_BADTARGET, sp);
					return PS_ERROR;
				}
				return PS_NORMAL;
			case SO_GROUPS:
				return PS_NORMAL;
			case SO_PICTURE:
				sp.skip_token(SP_FACTOR, TT_OF);
				break;
			case SO_WINDOW:
				sp.backup();
				which = SO_OBJECT;
				break;
			default:
				while (sp.skip_token(SP_SHOW, TT_UNDEFINED) == PS_NORMAL)
					;
				if (sp.skip_token(SP_FACTOR, TT_PREP, PT_AT) == PS_NORMAL)
				{
					if (sp.parseexp(False, True, &location) != PS_NORMAL)
					{
						MCperror->add
						(PE_SHOW_BADLOCATION, sp);
						return PS_ERROR;
					}
				}
				return PS_NORMAL;
			}
		}
		else
			sp.backup();
	else
	{
		MCperror->add
		(PE_SHOW_BADTARGET, sp);
		return PS_ERROR;
	}
	ton = new MCChunk(False);
	if (ton->parse(sp, False) != PS_NORMAL)
	{
		MCperror->add
		(PE_SHOW_BADTARGET, sp);
		return PS_ERROR;
	}
	if (sp.skip_token(SP_FACTOR, TT_PREP, PT_AT) == PS_NORMAL)
	{
		if (sp.parseexp(False, True, &location) != PS_NORMAL)
		{
			MCperror->add
			(PE_SHOW_BADLOCATION, sp);
			return PS_ERROR;
		}
	}
	else
		if (sp.skip_token(SP_FACTOR, TT_CHUNK, CT_CARD) == PS_NORMAL
		        || sp.skip_token(SP_FACTOR, TT_CLASS, CT_CARD) == PS_NORMAL)
			which = SO_CARD;
	if (sp.skip_token(SP_REPEAT, TT_UNDEFINED) == PS_NORMAL)
	{
		sp.skip_token(SP_COMMAND, TT_STATEMENT, S_VISUAL);
		effect = new MCVisualEffect;
		if (effect->parse(sp) != PS_NORMAL)
		{
			MCperror->add
			(PE_SHOW_BADEFFECT, sp);
			return PS_ERROR;
		}
	}
	return PS_NORMAL;
}

Exec_stat MCShow::exec(MCExecPoint &ep)
{
#ifdef /* MCShow */ LEGACY_EXEC
	uint2 count;
	switch (which)
	{
	case SO_GROUPS:
		MClinkatts.underline = True;

		// MW-2011-08-17: [[ Redraw ]] We've changed a global property that could
		//   affect the display of all stacks.
		MCRedrawDirtyScreen();
		break;
	case SO_ALL:
		MCdefaultstackptr->count(CT_CARD, CT_UNDEFINED, NULL, count);
		MCdefaultstackptr->flip(count);
		break;
	case SO_MARKED:
		MCdefaultstackptr->setmark();
		MCdefaultstackptr->count(CT_CARD, CT_UNDEFINED, NULL, count);
		MCdefaultstackptr->flip(count);
		MCdefaultstackptr->clearmark();
		break;
	case SO_CARD:
		if (ton == NULL || ton->eval(ep) != ES_NORMAL || ep.ton() != ES_NORMAL)
		{
			MCeerror->add(EE_SHOW_BADNUMBER, line, pos);
			return ES_ERROR;
		}
		count = ep.getuint2();
		MCdefaultstackptr->flip(count);
		break;
	case SO_PICTURE:
		break;
	case SO_OBJECT:
		MCObject *optr;
		uint4 parid;
		if (ton->getobj(ep, optr, parid, True) != ES_NORMAL)
		{
			MCeerror->add
			(EE_SHOW_NOOBJ, line, pos);
			return ES_ERROR;
		}
		if (location != NULL)
		{
			if (location->eval(ep) != ES_NORMAL)
			{
				MCeerror->add
				(EE_SHOW_NOLOCATION, line, pos);

				return ES_ERROR;
			}
			if (optr->setprop(parid, P_LOCATION, ep, False) != ES_NORMAL)
			{
				MCeerror->add
				(EE_SHOW_BADLOCATION, line, pos, ep.getsvalue());
				return ES_ERROR;
			}
		}
		if (effect != NULL && !MCRedrawIsScreenLocked())
		{
			if (effect->exec(ep) != ES_NORMAL)
			{
				MCeerror->add(EE_SHOW_BADEFFECT, line, pos);
				return ES_NORMAL;
			}

			// MW-2010-04-26: [[ Bug 8661 ]] Make sure we use the effective rect for
			//   effectarea computation.
			MCRectangle t_rect;
			if (optr -> gettype() >= CT_GROUP)
				t_rect = static_cast<MCControl *>(optr) -> geteffectiverect();
			else
				t_rect = optr -> getrect();
			
			// MW-2011-09-13: [[ Effects ]] Cache the rect we want to play the effect
			//   in.
			optr -> getstack() -> snapshotwindow(t_rect);
			
			// MW-2011-11-15: [[ Bug 9846 ]] Lock the screen to prevent the snapshot
			//   being dumped inadvertantly.
			MCRedrawLockScreen();
			
			// MW-2011-11-15: [[ Bug 9846 ]] Make sure we use the same mechanism to
			//   set visibility as the non-effect case.
			optr->setsprop(P_VISIBLE, MCtruemcstring);
			
			MCRedrawUnlockScreen();
			
			// Run the effect - this will use the previously cached image.
			Boolean abort = False;
			optr->getstack()->effectrect(t_rect, abort);
			
			if (abort)
			{
				MCeerror->add(EE_HANDLER_ABORT, line, pos);
				return ES_ERROR;
			}
		}
		else
			optr->setsprop(P_VISIBLE, MCtruemcstring);
		break;
	case SO_MENU:
		MCscreen->showmenu();
		break;
	case SO_TASKBAR:
		MCscreen->showtaskbar();
		break;
	case SO_MESSAGE:
		{
			MCStack *mb = ep.getobj()->getstack()->findstackname(MCmessagenamestring);

			// MW-2007-08-14: [[ Bug 3310 ]] - "show message box" toplevels rather than palettes
			if (mb != NULL)
				mb->openrect(ep.getobj()->getstack()->getrect(), WM_PALETTE,
				             NULL, WP_DEFAULT, OP_NONE);
		}
		break;
	default:
		break;
	}
	return ES_NORMAL;
#endif /* MCShow */
}

MCSubwindow::~MCSubwindow()
{
	delete target;
	delete at;
	delete parent;
	delete aligned;
}

Parse_stat MCSubwindow::parse(MCScriptPoint &sp)
{
	initpoint(sp);
	target = new MCChunk(False);
	if (target->parse(sp, False) != PS_NORMAL)
	{
		MCperror->add
		(PE_SUBWINDOW_BADEXP, sp);
		return PS_ERROR;
	}
	if (sp.skip_token(SP_FACTOR, TT_PREP, PT_AT) == PS_NORMAL)
	{
		if (sp.parseexp(False, True, &at) != PS_NORMAL)
		{
			MCperror->add
			(PE_SUBWINDOW_BADEXP, sp);
			return PS_ERROR;
		}
	}
	if (sp.skip_token(SP_FACTOR, TT_IN) == PS_NORMAL)
	{
		if (sp.skip_token(SP_FACTOR, TT_CHUNK, CT_THIS) == PS_NORMAL)
		{
			sp.skip_token(SP_FACTOR, TT_CHUNK, CT_STACK);
			thisstack = True;
			return PS_NORMAL;
		}
		if (sp.skip_token(SP_FACTOR, TT_CHUNK, CT_STACK) == PS_NORMAL)
		{
			sp.skip_token(SP_FACTOR, TT_OF);
			sp.skip_token(SP_FACTOR, TT_CHUNK, CT_STACK);

			if (sp.parseexp(False, True, &parent) != PS_NORMAL)
			{
				MCperror->add
				(PE_SUBWINDOW_BADEXP, sp);
				return PS_ERROR;
			}
		}
	}
	if (sp.skip_token(SP_FACTOR, TT_PREP, PT_ALIGNED) == PS_NORMAL)
	{
		sp.skip_token(SP_FACTOR, TT_TO, PT_TO);
		if (sp.parseexp(False, True, &aligned) != PS_NORMAL)
		{
			MCperror->add
			(PE_SUBWINDOW_BADEXP, sp);
			return PS_ERROR;
		}

	}
	return PS_NORMAL;
}


Exec_stat MCSubwindow::exec(MCExecPoint &ep)
{
#ifdef /* MCSubwindow */ LEGACY_EXEC
	MCObject *optr;
	uint4 parid;
	MCresult->clear(False);
	MCerrorlock++;
	if (target->getobj(ep, optr, parid, True) != ES_NORMAL
	        || optr->gettype() != CT_BUTTON && optr->gettype() != CT_STACK)
	{
		MCerrorlock--;
		if (target->eval(ep) != ES_NORMAL)
		{
			MCeerror->add
			(EE_SUBWINDOW_BADEXP, line, pos);
			return ES_ERROR;
		}
		optr = ep.getobj()->getstack()->findstackname(ep.getsvalue());
	}
	else
		MCerrorlock--;
	if (optr == NULL)
	{
		if (MCresult->isclear())
			MCresult->sets("can't find stack");
		return ES_NORMAL;
	}
	if (optr->gettype() == CT_BUTTON)
	{
		if (mode != WM_POPUP || MCmousestackptr == NULL)
		{
			MCeerror->add
			(EE_SUBWINDOW_NOSTACK, line, pos, ep.getsvalue());
			return ES_ERROR;
		}
		if (at != NULL)
			if (at->eval(ep) != ES_NORMAL
			        || !MCU_stoi2x2(ep.getsvalue(), MCmousex, MCmousey))
			{
				MCeerror->add
				(EE_SUBWINDOW_BADEXP, line, pos);
				return ES_ERROR;
			}
		MCButton *bptr = (MCButton *)optr;
		bptr->setmenumode(WM_POPUP);
		if (bptr->findmenu())
		{
			if (MCbuttonstate)
				MCtargetptr -> mup(0);
			bptr->openmenu(True);
		}
		return ES_NORMAL;
	}
	MCStack *stackptr = (MCStack *)optr;
	if (mode != WM_PULLDOWN && mode != WM_POPUP && mode != WM_OPTION)
		MCU_watchcursor(ep.getobj()->getstack(), False);

	// MW-2007-05-01: Reverting this as it causes problems :o(
	//stackptr -> setflag(True, F_VISIBLE);

	MCStack *olddefault = MCdefaultstackptr;
	Boolean oldtrace = MCtrace;
	MCtrace = False;
	if (mode >= WM_MODELESS)
		MCRedrawForceUnlockScreen();
	switch (mode)
	{
	case WM_TOP_LEVEL:
	case WM_MODELESS:
	case WM_PALETTE:
	case WM_MODAL:
	{
		MCStack *t_target;
		if (MCdefaultstackptr->getopened() || MCtopstackptr == NULL)
			t_target = MCdefaultstackptr;
		else
			t_target = MCtopstackptr;

		stackptr->openrect(t_target -> getrect(), mode, NULL, WP_DEFAULT, OP_NONE);
	}
	break;
	case WM_SHEET:
	case WM_DRAWER:
		{
			MCStack *parentptr = NULL;
			MCerrorlock++;
			if (parent != NULL)
			{
				if (parent->eval(ep) != ES_NORMAL)
				{
					MCeerror->add
					(EE_SUBWINDOW_BADEXP, line, pos);
					return ES_ERROR;
				}
				parentptr = ep.getobj()->getstack()->findstackname(ep.getsvalue());
				if  (parentptr == NULL || !parentptr->getopened())
				{
					MCeerror->add
					(EE_SUBWINDOW_BADEXP, line, pos);
					return ES_ERROR;
				}
			}
			if (thisstack)
				parentptr = MCdefaultstackptr;
			if (parentptr == stackptr)
				parentptr = NULL;
			Window_position wpos = WP_DEFAULT;
			Object_pos walign = OP_CENTER;
			if (mode == WM_DRAWER)
			{
				if (at != NULL)
				{
					if (at->eval(ep) != ES_NORMAL)
					{
						MCeerror->add
						(EE_SUBWINDOW_BADEXP, line, pos);
						return ES_ERROR;
					}
					char *positionstr = ep.getsvalue().clone();
					char *alignment = NULL;
					char *sptr = positionstr;
					if ((sptr = strchr(positionstr, ',')) != NULL)
					{
						*sptr = '\0';
						sptr++;
						alignment = strclone(sptr);
					}
					if (aligned != NULL)
					{
						if (aligned->eval(ep) != ES_NORMAL)
						{
							MCeerror->add
							(EE_SUBWINDOW_BADEXP, line, pos);
							return ES_ERROR;
						}
						alignment = ep.getsvalue().clone();
					}
					if (MCU_strncasecmp("right", positionstr,
					                    strlen("right")) == 0)
						wpos = WP_PARENTRIGHT;
					else if (MCU_strncasecmp("left", positionstr,
					                         strlen("left")) == 0)
						wpos = WP_PARENTLEFT;
					else if (MCU_strncasecmp("top", positionstr,
					                         strlen("top")) == 0)
						wpos = WP_PARENTTOP;
					else if (MCU_strncasecmp("bottom", positionstr,
					                         strlen("bottom")) == 0)
						wpos = WP_PARENTBOTTOM;
					if (alignment != NULL)
					{
						if (MCU_strncasecmp("right", alignment,
						                    strlen("right")) == 0)
							walign = OP_RIGHT;
						else if (MCU_strncasecmp("bottom", alignment,
						                         strlen("bottom")) == 0)
							walign = OP_BOTTOM;
						else if (MCU_strncasecmp("top", alignment,
						                         strlen("top")) == 0)
							walign = OP_TOP;
						else if (MCU_strncasecmp("left", alignment,
						                         strlen("left")) == 0)
							walign = OP_LEFT;
						else if (MCU_strncasecmp("center", alignment,
						                         strlen("center")) == 0)
							walign = OP_CENTER;
						delete alignment;
					}
					delete positionstr;
				}
			}
			if (parentptr != NULL && parentptr->getopened())
				stackptr->openrect(parentptr->getrect(), mode,  parentptr, wpos, walign);
			else if (MCdefaultstackptr->getopened() || MCtopstackptr == NULL)
				stackptr->openrect(MCdefaultstackptr->getrect(), mode,  MCdefaultstackptr, wpos, walign);
			else
				stackptr->openrect(MCtopstackptr->getrect(), mode, MCtopstackptr, wpos, walign);
			break;
		}
	case WM_PULLDOWN:
	case WM_POPUP:
	case WM_OPTION:
		{
			// MW-2007-04-10: [[ Bug 4260 ]] We shouldn't attempt to attach a menu to a control that is descendent of itself
			if (MCtargetptr -> getstack() == stackptr)
			{
				MCeerror->add(EE_SUBWINDOW_BADEXP, line, pos);
				return ES_ERROR;
			}

			if (MCtargetptr->attachmenu(stackptr))
			{
				MCRectangle rel = MCU_recttoroot(MCtargetptr->getstack(),
				                                 MCtargetptr->getrect());
				if (mode == WM_POPUP && at != NULL)
					if (at->eval(ep) != ES_NORMAL
					        || !MCU_stoi2x2(ep.getsvalue(), MCmousex, MCmousey))
					{
						MCeerror->add(EE_SUBWINDOW_BADEXP, line, pos);
						return ES_ERROR;
					}
				stackptr->openrect(rel, mode, NULL, WP_DEFAULT,OP_NONE);
				if (MCabortscript)
				{
					MCtrace = oldtrace;
					return ES_ERROR;
				}
			}
		}
		break;
	default:
		fprintf(stderr, "Subwindow: ERROR bad mode\n");
		break;
	}
	if (MCwatchcursor)
	{
		MCwatchcursor = False;
		stackptr->resetcursor(True);
		if (MCmousestackptr != NULL && MCmousestackptr != stackptr)
			MCmousestackptr->resetcursor(True);
	}
	MCtrace = oldtrace;
	if (mode > WM_TOP_LEVEL)
		MCdefaultstackptr = olddefault;
	return ES_NORMAL;
#endif /* MCSubwindow */
}

MCUnlock::~MCUnlock()
{
	delete effect;
}


Parse_stat MCUnlock::parse(MCScriptPoint &sp)
{
	Symbol_type type;
	const LT *te;

	initpoint(sp);
	if (sp.next(type) != PS_NORMAL)
	{
		MCperror->add
		(PE_UNLOCK_NOTARGET, sp);
		return PS_ERROR;
	}
	if (sp.lookup(SP_LOCK, te) != PS_NORMAL)
	{
		MCperror->add
		(PE_UNLOCK_NOTTARGET, sp);
		return PS_ERROR;
	}
	which = (Lock_constants)te->which;
	if (which == LC_SCREEN)
	{
		if (sp.skip_token(SP_REPEAT, TT_UNDEFINED) == PS_NORMAL)
		{
			sp.skip_token(SP_COMMAND, TT_STATEMENT, S_VISUAL);
			effect = new MCVisualEffect;
			if (effect->parse(sp) != PS_NORMAL)
			{
				MCperror->add
				(PE_UNLOCK_BADEFFECT, sp);
				return PS_ERROR;
			}
		}
	}
	else
		if (which == LC_ERRORS)
			sp.skip_token(SP_LOCK, TT_UNDEFINED);
	return PS_NORMAL;
}

Exec_stat MCUnlock::exec(MCExecPoint &ep)
{
#ifdef /* MCUnlock */ LEGACY_EXEC
	switch (which)
	{
	case LC_COLORMAP:
		MClockcolormap = False;
		break;
	case LC_CURSOR:
		MClockcursor = False;
		MCdefaultstackptr->resetcursor(False);
		break;
	case LC_ERRORS:
		MClockerrors = False;
		break;
	case LC_MENUS:
		MClockmenus = False;
		MCscreen->updatemenubar(True);
		break;
	case LC_MSGS:
		MClockmessages = False;
		break;
	case LC_MOVES:
		MClockmoves = False;
		break;
	case LC_RECENT:
		MClockrecent = False;
		break;
	case LC_SCREEN:
		// MW-2011-08-18: [[ Redraw ]] Update to use redraw.
		if (effect != nil && effect -> exec(ep) != ES_NORMAL)
		{
			MCeerror->add(EE_UNLOCK_BADEFFECT, line, pos);
			return ES_ERROR;
		}
		MCRedrawUnlockScreenWithEffects();
		break;
	default:
		break;
	}
	return ES_NORMAL;
#endif /* MCUnlock */
}

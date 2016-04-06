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
#include "objdefs.h"
#include "parsedef.h"
#include "filedefs.h"
#include "mcio.h"

#include "scriptpt.h"
//#include "execpt.h"
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
#include "exec.h"
#include "syntax.h"
#include "variable.h"

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

void MCCompact::exec_ctxt(MCExecContext &ctxt)
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

	MCObject *optr;
	uint4 parid;

    if (!target->getobj(ctxt, optr, parid, True))
    {
        ctxt . LegacyThrow(EE_COMPACT_NOTARGET);
        return;
	}
	if (optr->gettype() != CT_STACK)
    {
        ctxt . LegacyThrow(EE_COMPACT_NOTASTACK);
        return;
	}
    MCStack *sptr = (MCStack *)optr;
    MCLegacyExecCompactStack(ctxt, sptr);
}

void MCCompact::compile(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginStatement(ctxt, line, pos);

	target -> compile_object_ptr(ctxt);

	MCSyntaxFactoryExecMethod(ctxt, kMCLegacyExecCompactStackMethodInfo);

	MCSyntaxFactoryEndStatement(ctxt);
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
	delete widget;
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
		{
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
		}
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
                        MCAutoNumberRef t_mode;

                        if (!MCNumberParse(sp.gettoken_stringref(), &t_mode)
						        || MCNumberFetchAsInteger(*t_mode) < 1 || MCNumberFetchAsInteger(*t_mode) >= WM_LAST)
						{
							MCperror->add
							(PE_GO_NOMODE, sp);
							return PS_ERROR;
						}
                        mode = (Window_mode)MCNumberFetchAsUnsignedInteger(*t_mode);
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
							if (sp.skip_token(SP_FACTOR, TT_IN) == PS_NORMAL)
							{
								widget = new MCChunk(False);
								if (widget->parse(sp, False) != PS_NORMAL)
								{
									MCperror->add(PE_GO_BADWIDGETEXP, sp);
									return PS_ERROR;
								}
								direction = nterm;
								
								break;
							}

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
						if (nterm >= CT_FIRST_TEXT_CHUNK || nterm == CT_URL)
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

MCStack *MCGo::findstack(MCExecContext &ctxt, MCStringRef p_value, Chunk_term etype, MCCard *&cptr)
{
	MCStack *sptr = NULL;
    uint4 offset;
    if (MCStringFirstIndexOf(p_value, MCSTR(SIGNATURE), 0, kMCCompareExact, offset)
            || (MCStringGetLength(p_value) > 8 && MCStringBeginsWithCString(p_value, (char_t*)"REVO", kMCCompareExact)))
    {
        char_t* t_cstring_value;
        uindex_t t_length;
        /* UNCHECKED */ MCStringConvertToNative(p_value, t_cstring_value, t_length);
        IO_handle stream = MCS_fakeopen(t_cstring_value, t_length);
		if (MCdispatcher->readfile(NULL, NULL, stream, sptr) != IO_NORMAL)
		{
			MCS_close(stream);
			if (MCresult->isclear())
                ctxt . SetTheResultToCString("can't build stack from string");
            return nil;
		}
		MCS_close(stream);
		return sptr;
	}
	if (etype == CT_STACK)
        return NULL;
	else
        sptr = MCdefaultstackptr->findstackname_string(p_value);

	if (sptr != NULL)
		return sptr;

	MCObject *objptr;
	MCChunk *tchunk = new MCChunk(False);
	MCerrorlock++;
    // AL-2014-11-10: [[ Bug 13972 ]] Parsing the chunk without passing through the context results
    //  in a parse error for unquoted stack and card names, since there is then no handler in which to
    //  create a new unquoted literal variable.
    MCScriptPoint sp(ctxt, p_value);
	Parse_stat stat = tchunk->parse(sp, False);
	if (stat == PS_NORMAL)
	{
		uint4 parid;
        if (tchunk->getobj(ctxt, objptr, parid, True))
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
            ctxt . SetTheResultToCString("no such card");
	return sptr;
}

void MCGo::exec_ctxt(MCExecContext &ctxt)
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
		Window w = NULL;
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
			if (ep.ton() == ES_NORMAL && MCscreen->uinttowindow(ep.getuint4(), w))
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
		sptr -> view_configure(True);
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

	MCStack *sptr = MCdefaultstackptr;
	MCControl *bptr = NULL;
	MCCard *cptr = NULL;

    ctxt . SetTheResultToEmpty();
    
	// go ( forward | backward ) in widget ...
	if (widget != nil)
	{
		MCObject *t_object;
		uint32_t t_parid;

		if (!widget->getobj(ctxt, t_object, t_parid, True) || t_object->gettype() != CT_WIDGET)
		{
			ctxt.LegacyThrow(EE_GO_BADWIDGETEXP);
			return;
		}
		
		if (direction == CT_BACKWARD)
			MCInterfaceExecGoBackInWidget(ctxt, (MCWidget*)t_object);
		else
			MCInterfaceExecGoForwardInWidget(ctxt, (MCWidget*)t_object);
		
		return;
	}
	
	if (stack == NULL && background == NULL && card == NULL)
	{
        ctxt . LegacyThrow(EE_GO_NODEST);
        return;
	}

	bool t_is_home;
	t_is_home = false;
	if (stack != NULL)
	{
		switch (stack->etype)
		{
		case CT_HELP:
			sptr = MCdefaultstackptr->findstackname(MCNAME("help"));
			break;
		case CT_HOME:
			t_is_home = true;
			sptr = MCdispatcher->gethome();
			break;
		case CT_THIS:
		case CT_EXPRESSION:
		case CT_ID:
			if (stack->etype == CT_THIS)
				sptr = MCdefaultstackptr;
            else if (stack -> etype == CT_ID)
            {
                uinteger_t t_stack_id;
                if (!ctxt . EvalExprAsStrictUInt(stack -> startpos, EE_GO_BADSTACKEXP, t_stack_id))
                    return;

                sptr = MCdefaultstackptr->findstackid(t_stack_id);
            }
            else
			{
                MCAutoStringRef t_value;
                if (!ctxt . EvalExprAsStringRef(stack -> startpos, EE_GO_BADSTACKEXP, &t_value))
                    return;

                sptr = findstack(ctxt, *t_value, stack->etype, cptr);
			}

			if (sptr != nil && stack->next != NULL)
			{
				switch (stack->next->etype)
				{
				case CT_ID:
                {
                    uinteger_t t_stack_id;
                    if (!ctxt . EvalExprAsStrictUInt(stack -> next -> startpos, EE_CHUNK_BADSTACKEXP, t_stack_id))
                        return;

                    sptr = sptr -> findsubstackid(t_stack_id);
                }
                    break;
				case CT_EXPRESSION:
                {
                    MCNewAutoNameRef t_name;
                    if (!ctxt . EvalExprAsNameRef(stack -> next -> startpos, EE_CHUNK_BADSTACKEXP, &t_name))
                        return;

                    sptr = sptr -> findsubstackname(*t_name);
                }
					break;
				default:
                    ctxt . LegacyThrow(EE_CHUNK_BADSTACKEXP);
                    return;
				}
			}
			break;
		default:
            ctxt . Throw();
            return;
		}
	}
    
    // SN-2014-05-13 [[ Bug 12423 ]]
    // Temporary fix: stopedit put back where it was before refactoring
    if (sptr != nil)
        sptr -> stopedit();
	
	if (background != NULL && sptr != nil)
	{
		switch (ct_class(background->etype))
		{
		case CT_ORDINAL:
			bptr = sptr->getbackground(background->etype, kMCEmptyString, CT_GROUP);
			break;
		case CT_ID:
		case CT_EXPRESSION:
        {
            MCAutoStringRef t_string;
            if (!ctxt . EvalExprAsStringRef(background -> startpos, EE_GO_BADBACKGROUNDEXP, &t_string))
                return;

            bptr = sptr->getbackground(background->etype, *t_string, CT_GROUP);
        }
			break;
		default:
			break;
		}
	}

    if (bptr != nil)
        sptr -> setbackground(bptr);
    
	real8 n;
	bool t_is_recent;
	t_is_recent = false;
	if (card != NULL && sptr != nil)
	{
		switch (ct_class(card->etype))
		{
		case CT_DIRECT:
            sptr -> clearbackground();
			t_is_recent = true;
			switch (card->etype)
			{
            case CT_BACKWARD:
            case CT_FORWARD:
                if (!ctxt . EvalOptionalExprAsDouble(card -> startpos, 1.0, EE_GO_BADCARDEXP, n))
                    return;

                break;
            case CT_START:
            case CT_FINISH:
                break;
            default:
                ctxt . Throw();
                return;
			}
			break;
		case CT_ORDINAL:
				if (card->etype == CT_RECENT)
					t_is_recent = true;
                // AL-2014-09-10: [[ Bug 13394 ]] Go marked card missing in refactor
                else if (marked)
                    sptr -> setmark();
				cptr = sptr->getchild(card->etype, kMCEmptyString, card->otype);
			break;
		case CT_ID:
		case CT_EXPRESSION:
		{
            MCAutoStringRef t_exp;
            if (!ctxt . EvalExprAsStringRef(card -> startpos, EE_GO_BADCARDEXP, &t_exp))
            {
                sptr -> clearbackground();
                return;
            }

            // AL-2014-09-10: [[ Bug 13394 ]] Go marked card missing in refactor
            if (marked)
                sptr -> setmark();
            
			cptr = sptr->getchild(card->etype, *t_exp, card->otype);

            // SN-2014-06-03 [[ Bug 12552]] go to url "internet stack path" does not work
            // If getchild() failed, we try to find a stack from the expression
            if (cptr == NULL)
            {
				sptr = findstack(ctxt, *t_exp, CT_STACK, cptr);
				if (sptr == NULL)
				{
					if (MCresult->isclear())
						MCresult->setvalueref(MCSTR("No such a card"));

					return;
				}
				cptr = (MCCard *)sptr->getchild(CT_THIS, kMCEmptyString, CT_CARD);
            }
		}
			break;
		default:
            sptr -> clearbackground();
			fprintf(stderr, "Go: ERROR no card type %d\n", card->etype);
            ctxt . Throw();
            return;
		}
        sptr -> clearbackground();
        sptr -> clearmark();
	}
	else
        if (cptr == nil && sptr != nil)
            cptr = (MCCard *)sptr->getchild(CT_THIS, kMCEmptyString, CT_CARD);
    
	MCAutoStringRef t_window;
    if (!thisstack)
    {
        if (!ctxt . EvalOptionalExprAsNullableStringRef(window, EE_GO_BADWINDOWEXP, &t_window))
            return;
	}

    ctxt . SetLineAndPos(line, pos);
	if (t_is_home)
		MCInterfaceExecGoHome(ctxt, cptr);
	else if (t_is_recent)
	{
		switch (ct_class(card->etype))
		{
		case CT_DIRECT:
			switch (card->etype)
			{
			case CT_BACKWARD:
			case CT_FORWARD:
				MCInterfaceExecGoCardRelative(ctxt, card->etype == CT_FORWARD, n);
				break;
			case CT_START:
			case CT_FINISH:
				MCInterfaceExecGoCardEnd(ctxt, card->etype == CT_START);
				break;
			default:
				break;
			}
			break;
		case CT_ORDINAL:
			MCInterfaceExecGoRecentCard(ctxt);
			break;
		default:
			break;
		}
	}
	else if (window != nil)
		MCInterfaceExecGoCardInWindow(ctxt, cptr, *t_window, visible == True, thisstack == True);
	else
        MCInterfaceExecGoCardAsMode(ctxt, cptr, mode, visible == True, thisstack == True);
}

void MCGo::compile(MCSyntaxFactoryRef ctxt)
{
	if (widget != nil)
	{
		MCSyntaxFactoryBeginStatement(ctxt, line, pos);
		widget->compile(ctxt);
		MCSyntaxFactoryExecMethod(ctxt, direction == CT_BACKWARD ? kMCInterfaceExecGoBackInWidgetMethodInfo : kMCInterfaceExecGoForwardInWidgetMethodInfo);
		MCSyntaxFactoryEndStatement(ctxt);
		
		return;
	}
	
    MCSyntaxFactoryBeginStatement(ctxt, line, pos);
    
    bool t_is_home, t_is_relative;
    t_is_home = false;
    t_is_relative = false;
    
    if (stack != nil)
    {
        switch (stack->etype)
        {
            case CT_HELP:
                MCSyntaxFactoryEvalMethod(ctxt, kMCInterfaceEvalHelpStackAsOptionalObjectMethodInfo);
            case CT_HOME:
                t_is_home = true;
                MCSyntaxFactoryEvalMethod(ctxt, kMCInterfaceEvalHomeStackAsOptionalObjectMethodInfo);
            case CT_THIS:
            case CT_ID:
            case CT_EXPRESSION:
                if (stack  -> etype == CT_THIS)
                    MCSyntaxFactoryEvalMethod(ctxt, kMCInterfaceEvalDefaultStackAsOptionalObjectMethodInfo);
                else
                {
                    stack -> startpos -> compile(ctxt);
                    MCSyntaxFactoryEvalMethod(ctxt, kMCInterfaceEvalStackByValueMethodInfo);
                }
                
                if (stack -> next != nil)
                {
                    switch (stack->next->etype)
                    {
                        case CT_ID:
                            stack -> next -> startpos -> compile(ctxt);
                            MCSyntaxFactoryEvalMethod(ctxt, kMCInterfaceEvalSubstackOfOptionalStackByIdMethodInfo);
                            break;
                        case CT_EXPRESSION:
                            stack -> next -> startpos -> compile(ctxt);
                            MCSyntaxFactoryEvalMethod(ctxt, kMCInterfaceEvalSubstackOfOptionalStackByNameMethodInfo);
                            break;
                        default:
                            // ERROR
                            break;
                    }
                    
                }
            default:
                MCSyntaxFactoryEvalMethod(ctxt, kMCInterfaceEvalDefaultStackAsOptionalObjectMethodInfo);
        }
    }
    else
        MCSyntaxFactoryEvalMethod(ctxt, kMCInterfaceEvalDefaultStackAsOptionalObjectMethodInfo);
    
	if (background != nil)
	{
		switch(ct_class(background -> etype))
        {
            case CT_ORDINAL:
                MCSyntaxFactoryEvalConstantUInt(ctxt, background -> etype);
                MCSyntaxFactoryEvalMethod(ctxt, kMCInterfaceEvalOptionalStackWithBackgroundByOrdinalMethodInfo);
                break;
            case CT_ID:
                background -> startpos -> compile(ctxt);
                MCSyntaxFactoryEvalMethod(ctxt, kMCInterfaceEvalOptionalStackWithBackgroundByIdMethodInfo);
                break;
            case CT_EXPRESSION:
                background -> startpos -> compile(ctxt);
                MCSyntaxFactoryEvalMethod(ctxt, kMCInterfaceEvalOptionalStackWithBackgroundByNameMethodInfo);
                break;
            default:
                // ERROR
                break;
        }
    }
    
    if (card != nil)
	{
		switch(ct_class(card -> etype))
		{
            case CT_DIRECT:
                t_is_relative = true;
                switch (card->etype)
                {
                case CT_BACKWARD:
                case CT_FORWARD:
                    MCSyntaxFactoryEvalConstantBool(ctxt, card->etype == CT_FORWARD);
                    if (card -> startpos != nil)
                        card -> startpos -> compile(ctxt);
                    else
                        MCSyntaxFactoryEvalConstantInt(ctxt, 1);
                    return;
                case CT_START:
                case CT_FINISH:
                    MCSyntaxFactoryEvalConstantBool(ctxt, card->etype == CT_START);

                    break;
                default:
                    // ERROR
                    break;
                }
                break;
            case CT_ORDINAL:
                if (card -> etype == CT_RECENT)
                    t_is_relative = true;
                MCSyntaxFactoryEvalConstantBool(ctxt, marked);
                MCSyntaxFactoryEvalConstantUInt(ctxt, card -> etype);
                MCSyntaxFactoryEvalMethod(ctxt, kMCInterfaceEvalCardOfOptionalStackByOrdinalMethodInfo);
                break;
            case CT_ID:
                MCSyntaxFactoryEvalConstantBool(ctxt, marked);
                card -> startpos -> compile(ctxt);
                MCSyntaxFactoryEvalMethod(ctxt, kMCInterfaceEvalCardOfOptionalStackByIdMethodInfo);
                break;
            case CT_EXPRESSION:
                MCSyntaxFactoryEvalConstantBool(ctxt, marked);
                card -> startpos -> compile(ctxt);
                MCSyntaxFactoryEvalMethod(ctxt, kMCInterfaceEvalCardOfOptionalStackByNameMethodInfo);
                break;
            default:
                // ERROR
                break;
		}
	}
    else
		MCSyntaxFactoryEvalMethod(ctxt, kMCInterfaceEvalThisCardOfOptionalStackMethodInfo);
    
	if (stack != nil && t_is_home)
		MCSyntaxFactoryExecMethod(ctxt, kMCInterfaceExecGoHomeMethodInfo); // skip background parameter
	else if (card != nil && t_is_relative)
	{
		switch (ct_class(card->etype))
		{
            case CT_DIRECT:
                switch (card->etype)
			{
                case CT_BACKWARD:
                case CT_FORWARD:
                    MCSyntaxFactoryExecMethod(ctxt, kMCInterfaceExecGoCardRelativeMethodInfo);
                    break;
                case CT_START:
                case CT_FINISH:
                    MCSyntaxFactoryExecMethod(ctxt, kMCInterfaceExecGoCardEndMethodInfo);
                    break;
                default:
                    break;
			}
                break;
            case CT_ORDINAL:
                MCSyntaxFactoryExecMethod(ctxt, kMCInterfaceExecGoRecentCardMethodInfo);
                break;
            default:
                break;
		}
	}
    else
    {
        if (window != nil)
        {
            if (!thisstack)
                window -> compile(ctxt);
            else
                MCSyntaxFactoryEvalConstantNil(ctxt);
        }
        else
            MCSyntaxFactoryEvalConstantUInt(ctxt, mode);

        MCSyntaxFactoryEvalConstantBool(ctxt, thisstack == True);
        MCSyntaxFactoryEvalConstantBool(ctxt, visible == True);
        
        if (window != nil)
            MCSyntaxFactoryExecMethod(ctxt, kMCInterfaceExecGoCardInWindowMethodInfo);
        else
            MCSyntaxFactoryExecMethod(ctxt, kMCInterfaceExecGoCardAsModeMethodInfo);
    }

    MCSyntaxFactoryEndStatement(ctxt);
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

void MCHide::exec_ctxt(MCExecContext &ctxt)
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

	switch (which)
	{
	case SO_GROUPS:
		MCInterfaceExecHideGroups(ctxt);
		break;
	case SO_OBJECT:
    {
		MCObjectPtr t_target;
        if (!object->getobj(ctxt, t_target, True))
		{
            ctxt . LegacyThrow(EE_HIDE_NOOBJ);
            return;
		}
		if (effect != NULL)
			MCInterfaceExecHideObjectWithEffect(ctxt, t_target, effect);
		else
			MCInterfaceExecHideObject(ctxt, t_target);
    }
		break;
	case SO_MENU:
		MCInterfaceExecHideMenuBar(ctxt);
		break;
	case SO_TASKBAR:
		MCInterfaceExecHideTaskBar(ctxt);
		break;
	case SO_MESSAGE:
		MCIdeExecHideMessageBox(ctxt);
		break;
	default:
		break;
    }
}

void MCHide::compile(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginStatement(ctxt, line, pos);
    
	switch (which)
	{
        case SO_GROUPS:
            MCSyntaxFactoryExecMethod(ctxt, kMCInterfaceExecHideGroupsMethodInfo);
            break;
        case SO_OBJECT:
            object -> compile_object_ptr(ctxt);
            if (effect != nil)
            {
                effect -> compile_effect(ctxt);
                MCSyntaxFactoryExecMethod(ctxt, kMCInterfaceExecHideObjectWithEffectMethodInfo);
            }
            else
                MCSyntaxFactoryExecMethod(ctxt, kMCInterfaceExecHideObjectMethodInfo);
            break;
        case SO_MENU:
            MCSyntaxFactoryExecMethod(ctxt, kMCInterfaceExecHideMenuBarMethodInfo);
            break;
        case SO_TASKBAR:
            MCSyntaxFactoryExecMethod(ctxt, kMCInterfaceExecHideTaskBarMethodInfo);
            break;
        case SO_MESSAGE:
            MCSyntaxFactoryExecMethod(ctxt, kMCIdeExecHideMessageBoxMethodInfo);
            break;
        default:
            break;
	}
    
	MCSyntaxFactoryEndStatement(ctxt);
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
    sp.skip_token(SP_FACTOR, TT_THE);
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

void MCLock::exec_ctxt(MCExecContext &ctxt)
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
		MCscreen->setlockmoves(True);
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
	
	switch(which)
	{
	case LC_COLORMAP:
		MCLegacyExecLockColormap(ctxt);
		break;
	case LC_CURSOR:
		MCInterfaceExecLockCursor(ctxt);
		break;
	case LC_ERRORS:
		MCEngineExecLockErrors(ctxt);
		break;
	case LC_MENUS:
		MCInterfaceExecLockMenus(ctxt);
		break;
	case LC_MSGS:
		MCEngineExecLockMessages(ctxt);
		break;
	case LC_MOVES:
		MCInterfaceExecLockMoves(ctxt);
		break;
	case LC_RECENT:
		MCInterfaceExecLockRecent(ctxt);
		break;
	case LC_SCREEN:
		MCInterfaceExecLockScreen(ctxt);
		break;
	// MW-2011-09-13: [[ Effects ]] If the screen is not locked capture a snapshot
	//   of the default stack.
	case LC_SCREEN_FOR_EFFECT:
		{
			MCRectangle t_region;
            MCRectangle *t_region_ptr;
            t_region_ptr = &t_region;
            if (!ctxt . EvalOptionalExprAsRectangle(rect, nil, EE_LOCK_BADRECT, t_region_ptr))
                return;
			
			MCInterfaceExecLockScreenForEffect(ctxt, t_region_ptr);
		}
		break;
    case LC_CLIPBOARD:
        MCPasteboardExecLockClipboard(ctxt);
        break;
	default:
		break;
    }
}

void MCLock::compile(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginStatement(ctxt, line, pos);

	switch(which)
	{
	case LC_COLORMAP:
		MCSyntaxFactoryExecMethod(ctxt, kMCLegacyExecLockColormapMethodInfo);
		break;
	case LC_CURSOR:
		MCSyntaxFactoryExecMethod(ctxt, kMCInterfaceExecLockCursorMethodInfo);
		break;
	case LC_ERRORS:
		MCSyntaxFactoryExecMethod(ctxt, kMCEngineExecLockErrorsMethodInfo);
		break;
	case LC_MENUS:
		MCSyntaxFactoryExecMethod(ctxt, kMCInterfaceExecLockMenusMethodInfo);
		break;
	case LC_MSGS:
		MCSyntaxFactoryExecMethod(ctxt, kMCEngineExecLockMessagesMethodInfo);
		break;
	case LC_MOVES:
		MCSyntaxFactoryExecMethod(ctxt, kMCInterfaceExecLockMovesMethodInfo);
		break;
	case LC_RECENT:
		MCSyntaxFactoryExecMethod(ctxt, kMCInterfaceExecLockRecentMethodInfo);
		break;
	case LC_SCREEN:
		MCSyntaxFactoryExecMethod(ctxt, kMCInterfaceExecLockScreenMethodInfo);
		break;
	case LC_SCREEN_FOR_EFFECT:
		if (rect != nil)
			rect -> compile(ctxt);
		else
			MCSyntaxFactoryEvalConstantNil(ctxt);

		MCSyntaxFactoryExecMethod(ctxt, kMCInterfaceExecLockScreenForEffectMethodInfo);
		break;
	default:
		break;
	}

	MCSyntaxFactoryEndStatement(ctxt);
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

void MCPop::exec_ctxt(MCExecContext &ctxt)
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

	if (dest == NULL) 
		MCInterfaceExecPopToLast(ctxt);
	else
	{
		MCAutoStringRef t_element;
		MCInterfaceExecPop(ctxt, &t_element);

		if (dest->set(ctxt, prep, *t_element))
            return;
        ctxt . LegacyThrow(EE_POP_CANTSET);
    }

}

void MCPop::compile(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginStatement(ctxt, line, pos);

	if (dest == nil)
		MCSyntaxFactoryExecMethod(ctxt, kMCInterfaceExecPopToLastMethodInfo);
	else
	{
		dest -> compile_out(ctxt);

		MCSyntaxFactoryExecMethod(ctxt, kMCInterfaceExecPopMethodInfo);
	}

	MCSyntaxFactoryEndStatement(ctxt);
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

void MCPush::exec_ctxt(MCExecContext &ctxt)
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

    if (card == NULL)
	{
		if (recent)
			MCInterfaceExecPushRecentCard(ctxt);
		else
			MCInterfaceExecPushCurrentCard(ctxt);
	}
	else
	{
		MCObject *optr;
		uint4 parid;
        if (!card->getobj(ctxt, optr, parid, True))
		{
            ctxt . LegacyThrow(EE_PUSH_NOTARGET);
            return;
		}
		if (optr->gettype() != CT_CARD)
		{
            ctxt . LegacyThrow(EE_PUSH_NOTACARD);
            return;
		}
		MCInterfaceExecPushCard(ctxt, (MCCard *)optr);
    }
}

void MCPush::compile(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginStatement(ctxt, line, pos);

	if (card == nil)
	{
		if (recent)
			MCSyntaxFactoryExecMethod(ctxt, kMCInterfaceExecPushRecentCardMethodInfo);
		else
			MCSyntaxFactoryExecMethod(ctxt, kMCInterfaceExecPushCurrentCardMethodInfo);
	}
	else
	{
		card -> compile_object_ptr(ctxt);

		MCSyntaxFactoryExecMethod(ctxt, kMCInterfaceExecPushCardMethodInfo);
	}

	MCSyntaxFactoryEndStatement(ctxt);
}

MCSave::~MCSave()
{
	delete target;
	delete filename;
}

Parse_stat MCSave::parse(MCScriptPoint &sp)
{
	initpoint(sp);
	target = new MCChunk(False);
	if (target->parse(sp, False) != PS_NORMAL)
	{
		MCperror->add
		(PE_SAVE_BADEXP, sp);
		return PS_ERROR;
	}

	/* Parse optional "as _" clause */
	if (sp.skip_token(SP_FACTOR, TT_PREP, PT_AS) == PS_NORMAL)
	{
		if (sp.parseexp(False, True, &filename) != PS_NORMAL)
		{
			MCperror->add(PE_SAVE_BADFILEEXP, sp);
			return PS_ERROR;
		}
	}

	/* Parse optional "with format _" or "with newest format" clauses */
	if (sp.skip_token(SP_REPEAT, TT_UNDEFINED, RF_WITH) == PS_NORMAL)
	{
		if (sp.skip_token(SP_FACTOR, TT_PREP, PT_NEWEST) == PS_NORMAL)
		{
			if (sp.skip_token(SP_FACTOR, TT_FUNCTION, F_FORMAT) != PS_NORMAL)
			{
				MCperror->add(PE_SAVE_BADFORMATEXP, sp);
				return PS_ERROR;
			}
			newest_format = true;
		}
		else
		{
			if (sp.skip_token(SP_FACTOR, TT_FUNCTION, F_FORMAT) != PS_NORMAL)
			{
				MCperror->add(PE_SAVE_BADFORMATEXP, sp);
				return PS_ERROR;
			}
			if (sp.parseexp(False, True, &format) != PS_NORMAL)
			{
				MCperror->add(PE_SAVE_BADFORMATEXP, sp);
				return PS_ERROR;
			}
		}
	}

	return PS_NORMAL;
}

void MCSave::exec_ctxt(MCExecContext &ctxt)
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

    MCObject *optr;
	uint4 parid;

    if (!target->getobj(ctxt, optr, parid, True))
	{
        ctxt . LegacyThrow(EE_SAVE_NOTARGET);
        return;
	}
	if (optr->gettype() != CT_STACK)
	{
        ctxt . LegacyThrow(EE_SAVE_NOTASTACK);
        return;
	}

	MCStack *t_stack = static_cast<MCStack *>(optr);

	MCAutoStringRef t_filename;
	if (filename != NULL)
	{
        if (!ctxt . EvalExprAsStringRef(filename, EE_SAVE_BADNOFILEEXP, &t_filename))
            return;
	}

	MCAutoStringRef t_format;
	if (format != NULL)
	{
		if (!ctxt.EvalExprAsStringRef(format, EE_SAVE_BADNOFORMATEXP, &t_format))
			return;
	}

	if (NULL != filename)
	{
		if (NULL != format)
		{
			MCInterfaceExecSaveStackAsWithVersion(ctxt, t_stack, *t_filename, *t_format);
		}
		else if (newest_format)
		{
			MCInterfaceExecSaveStackAsWithNewestVersion(ctxt, t_stack, *t_filename);
		}
		else
		{
			MCInterfaceExecSaveStackAs(ctxt, t_stack, *t_filename);
		}
	}
	else
	{
		if (NULL != format)
		{
			MCInterfaceExecSaveStackWithVersion(ctxt, t_stack, *t_format);
		}
		else if (newest_format)
		{
			MCInterfaceExecSaveStackWithNewestVersion(ctxt, t_stack);
		}
		else
		{
			MCInterfaceExecSaveStack(ctxt, t_stack);
		}
	}
}

void MCSave::compile(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginStatement(ctxt, line, pos);

	target -> compile_object_ptr(ctxt);

	if (filename != nil)
	{
		filename -> compile(ctxt);
		MCSyntaxFactoryExecMethod(ctxt, kMCInterfaceExecSaveStackAsMethodInfo);
	}
	else
		MCSyntaxFactoryExecMethod(ctxt, kMCInterfaceExecSaveStackMethodInfo);

	MCSyntaxFactoryEndStatement(ctxt);
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

void MCShow::exec_ctxt(MCExecContext &ctxt)
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

	switch (which)
	{
	case SO_GROUPS:
		MCInterfaceExecShowGroups(ctxt);
		break;
	case SO_ALL:
		MCInterfaceExecShowAllCards(ctxt);
		break;
	case SO_MARKED:
		MCInterfaceExecShowMarkedCards(ctxt);
		break;
	case SO_CARD:
	{
        uinteger_t t_count;

        if (ton == nil)
        {
            ctxt . LegacyThrow(EE_SHOW_BADNUMBER);
            return;
        }
        else if (!ctxt . EvalOptionalExprAsUInt(ton, 0, EE_SHOW_BADNUMBER, t_count))
            return;

		MCInterfaceExecShowCards(ctxt, t_count);
	}
		break;
	case SO_PICTURE:
		break;
	case SO_OBJECT:
    {
		MCObjectPtr t_target;
        if (!ton->getobj(ctxt, t_target, True))
		{
            ctxt . LegacyThrow(EE_SHOW_NOOBJ);
            return;
		}
		MCPoint t_location;
        MCPoint *t_location_ptr = &t_location;
        if (!ctxt . EvalOptionalExprAsPoint(location, nil, EE_SHOW_NOLOCATION, t_location_ptr))
            return;

		if (effect != NULL)
			MCInterfaceExecShowObjectWithEffect(ctxt, t_target, t_location_ptr, effect);
		else
			MCInterfaceExecShowObject(ctxt, t_target, t_location_ptr);
    }
		break;
	case SO_MENU:
		MCInterfaceExecShowMenuBar(ctxt);
		break;
	case SO_TASKBAR:
		MCInterfaceExecShowTaskBar(ctxt);
		break;
	case SO_MESSAGE:
		MCIdeExecShowMessageBox(ctxt);
		break;
	default:
		break;
    }
}

void MCShow::compile(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginStatement(ctxt, line, pos);

	switch (which)
	{
	case SO_GROUPS:
		MCSyntaxFactoryExecMethod(ctxt, kMCInterfaceExecShowGroupsMethodInfo);
		break;
	case SO_ALL:
		MCSyntaxFactoryExecMethod(ctxt, kMCInterfaceExecShowAllCardsMethodInfo);
		break;
	case SO_MARKED:
		MCSyntaxFactoryExecMethod(ctxt, kMCInterfaceExecShowMarkedCardsMethodInfo);
		break;
	case SO_CARD:
		ton -> compile(ctxt);	
		MCSyntaxFactoryExecMethod(ctxt, kMCInterfaceExecShowCardsMethodInfo);
		break;
	case SO_PICTURE:
		break;
	case SO_OBJECT:
		ton -> compile_object_ptr(ctxt);

		if (location != nil)
			location -> compile(ctxt);
		else
			MCSyntaxFactoryEvalConstantNil(ctxt);

		if (effect != nil)
		{
			effect -> compile_effect(ctxt);
			MCSyntaxFactoryExecMethod(ctxt, kMCInterfaceExecShowObjectWithEffectMethodInfo);
		}
		else
			MCSyntaxFactoryExecMethod(ctxt, kMCInterfaceExecShowObjectMethodInfo);
		break;
	case SO_MENU:
		MCSyntaxFactoryExecMethod(ctxt, kMCInterfaceExecShowMenuBarMethodInfo);
		break;
	case SO_TASKBAR:
		MCSyntaxFactoryExecMethod(ctxt, kMCInterfaceExecShowTaskBarMethodInfo);
		break;
	case SO_MESSAGE:
		MCSyntaxFactoryExecMethod(ctxt, kMCIdeExecShowMessageBoxMethodInfo);
		break;
	default:
		break;
	}

	MCSyntaxFactoryEndStatement(ctxt);
}

MCSubwindow::~MCSubwindow()
{
	delete target;
	delete at;
	delete parent;
	delete aligned;
	
	delete widget;
	delete properties;
}

Parse_stat MCSubwindow::parse(MCScriptPoint &sp)
{
	initpoint(sp);
	
	if (sp.skip_token(SP_FACTOR, TT_CHUNK, CT_WIDGET) == PS_NORMAL)
	{
		if (sp.parseexp(False, True, &widget) != PS_NORMAL)
		{
			MCperror->add(PE_SUBWINDOW_BADEXP, sp);
			return PS_ERROR;
		}
	}
	else
	{
		target = new MCChunk(False);
		if (target->parse(sp, False) != PS_NORMAL)
		{
			MCperror->add
			(PE_SUBWINDOW_BADEXP, sp);
			return PS_ERROR;
		}
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
	if (sp.skip_token(SP_REPEAT, TT_UNDEFINED, RF_WITH) == PS_NORMAL)
	{
		sp.skip_token(SP_FACTOR, TT_PROPERTY, P_PROPERTIES);
		if (sp.parseexp(False, True, &properties) != PS_NORMAL)
		{
			MCperror->add(PE_SUBWINDOW_BADEXP, sp);
			return PS_ERROR;
		}
	}
	return PS_NORMAL;
}


void MCSubwindow::exec_ctxt(MCExecContext &ctxt)
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
				MCtargetptr -> mup(0, false);
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

	if (widget != nil)
	{
		MCNewAutoNameRef t_kind;
		if (!ctxt.EvalExprAsNameRef(widget, EE_SUBWINDOW_BADEXP, &t_kind))
			return;
		
		MCPoint t_at;
		MCPoint *t_at_ptr = &t_at;
		if (!ctxt.EvalOptionalExprAsPoint(at, nil, EE_SUBWINDOW_BADEXP, t_at_ptr))
			return;
		
		MCAutoArrayRef t_properties;
		if (!ctxt.EvalOptionalExprAsArrayRef(properties, kMCEmptyArray, EE_SUBWINDOW_BADEXP, &t_properties))
			return;
		
		MCInterfaceExecPopupWidget(ctxt, *t_kind, t_at_ptr, *t_properties);
		return;
	}
	
	
	MCObject *optr;
	MCNewAutoNameRef optr_name;
	uint4 parid;
	ctxt . SetTheResultToEmpty();
    MCerrorlock++;

    // Need to have a second MCExecContext as getobj may throw a non-fatal error
    MCExecContext ctxt2(ctxt);
    if (!target -> getobj(ctxt2, optr, parid, True)
        || (optr->gettype() != CT_BUTTON && optr->gettype() != CT_STACK))
	{
		MCerrorlock--;
        if (!ctxt . EvalExprAsNameRef(target, EE_SUBWINDOW_BADEXP, &optr_name))
            return;
    }
	else
		MCerrorlock--;

	if (optr != nil && optr -> gettype() == CT_BUTTON)
	{
		if (mode != WM_POPUP)
		{
            ctxt . LegacyThrow(EE_SUBWINDOW_NOSTACK, *optr_name);
            return;
		}

		MCPoint t_location;
        MCPoint *t_location_ptr = &t_location;
        if (!ctxt . EvalOptionalExprAsPoint(at, nil, EE_SUBWINDOW_BADEXP, t_location_ptr))
            return;

		MCInterfaceExecPopupButton(ctxt, (MCButton *)optr, t_location_ptr);
	}
	else
	{
		switch (mode)
		{
		case WM_TOP_LEVEL:
		case WM_MODELESS:
		case WM_PALETTE:
		case WM_MODAL:
			if (*optr_name != nil)
				MCInterfaceExecOpenStackByName(ctxt, *optr_name, mode);
			else
				MCInterfaceExecOpenStack(ctxt, (MCStack *)optr, mode);
		break;
		case WM_SHEET:
		case WM_DRAWER:
			{
				MCNewAutoNameRef t_parent_name;
                if (!ctxt . EvalOptionalExprAsNullableNameRef(parent, EE_SUBWINDOW_BADEXP, &t_parent_name))
                    return;

				if (mode == WM_SHEET)
				{
					if (*optr_name != nil)
						MCInterfaceExecSheetStackByName(ctxt, *optr_name, *t_parent_name, thisstack == True);
					else
						MCInterfaceExecSheetStack(ctxt, (MCStack *)optr, *t_parent_name, thisstack == True);
				}
				else
				{
					Window_position t_pos = WP_DEFAULT;
					Object_pos t_align = OP_CENTER;
					if (at != NULL)
					{
                        MCAutoStringRef t_position_data;
                        if (!ctxt . EvalExprAsStringRef(at, EE_SUBWINDOW_BADEXP, &t_position_data))
                            return;

						MCAutoStringRef t_position;
						MCAutoStringRef t_alignment;
						uindex_t t_delimiter;

                        if (aligned != NULL)
                        {
                            if (!ctxt . EvalExprAsStringRef(aligned, EE_SUBWINDOW_BADEXP, &t_alignment))
                                return;

                            MCStringCopy(*t_position_data, &t_position);
                        }
						else if (MCStringFirstIndexOfChar(*t_position_data, ',', 0, kMCCompareExact, t_delimiter))
						{
							MCStringCopySubstring(*t_position_data, MCRangeMake(0, t_delimiter), &t_position);
							t_delimiter++;
							MCStringCopySubstring(*t_position_data, MCRangeMake(t_delimiter, MCStringGetLength(*t_position_data) - t_delimiter), &t_alignment);
						}
                        // AL-2014-04-07: [[ Bug 12138 ]] 'drawer ... at <position>' codepath resulted in t_position uninitialised
                        else
                            t_position = *t_position_data;
                        
						if (MCStringIsEqualToCString(*t_position, "right", kMCCompareCaseless))
							t_pos = WP_PARENTRIGHT;
						else if (MCStringIsEqualToCString(*t_position, "left", kMCCompareCaseless))
							t_pos = WP_PARENTLEFT;
						else if (MCStringIsEqualToCString(*t_position, "top", kMCCompareCaseless))
							t_pos = WP_PARENTTOP;
						else if (MCStringIsEqualToCString(*t_position, "bottom", kMCCompareCaseless))
							t_pos = WP_PARENTBOTTOM;
						if (*t_alignment != nil)
						{
							if (MCStringIsEqualToCString(*t_alignment, "right", kMCCompareCaseless))
								t_align = OP_RIGHT;
							else if (MCStringIsEqualToCString(*t_alignment, "bottom", kMCCompareCaseless))
								t_align = OP_BOTTOM;
							else if (MCStringIsEqualToCString(*t_alignment, "top", kMCCompareCaseless))
								t_align = OP_TOP;
							else if (MCStringIsEqualToCString(*t_alignment, "left", kMCCompareCaseless))
								t_align = OP_LEFT;
							else if (MCStringIsEqualToCString(*t_alignment, "center", kMCCompareCaseless))
								t_align = OP_CENTER;
						}
					}
					if (optr == nil)
						MCInterfaceExecDrawerStackByName(ctxt, *optr_name, *t_parent_name, thisstack == True, t_pos, t_align);
					else
						MCInterfaceExecDrawerStack(ctxt, (MCStack *)optr, *t_parent_name, thisstack == True, t_pos, t_align);
				}
				break;
			}
		case WM_PULLDOWN:
		case WM_POPUP:
		case WM_OPTION:
		{
			MCPoint t_location;
            MCPoint *t_location_ptr = &t_location;
            if (!ctxt . EvalOptionalExprAsPoint(at, nil, EE_SUBWINDOW_BADEXP, t_location_ptr))
                return;

			if (optr == nil)
				MCInterfaceExecPopupStackByName(ctxt, *optr_name, t_location_ptr, mode);
			else
				MCInterfaceExecPopupStack(ctxt, (MCStack *)optr, t_location_ptr, mode);
		}
		break;
		default:
			fprintf(stderr, "Subwindow: ERROR bad mode\n");
			break;
		}
    }
}

void MCSubwindow::compile(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginStatement(ctxt, line, pos);

	if (widget != nil)
	{
		widget->compile(ctxt);
		
		if (at != nil)
			at->compile(ctxt);
		else
			MCSyntaxFactoryEvalConstantNil(ctxt);
		
		if (properties != nil)
			properties->compile(ctxt);
		else
			MCSyntaxFactoryEvalConstantNil(ctxt);
		
		MCSyntaxFactoryExecMethodWithArgs(ctxt, kMCInterfaceExecPopupWidgetMethodInfo, 0, 1, 2);
		
		return;
	}
	
	
	target->compile(ctxt);

	switch (mode)
	{
	case WM_TOP_LEVEL:
	case WM_MODELESS:
	case WM_PALETTE:
	case WM_MODAL:
		MCSyntaxFactoryEvalConstantInt(ctxt, mode);

		MCSyntaxFactoryExecMethod(ctxt, kMCInterfaceExecOpenStackMethodInfo);
		MCSyntaxFactoryExecMethod(ctxt, kMCInterfaceExecOpenStackByNameMethodInfo);
		break;

	case WM_SHEET:
	case WM_DRAWER:
		if (parent != nil)
			parent -> compile(ctxt);
		else
			MCSyntaxFactoryEvalConstantNil(ctxt);
		
		MCSyntaxFactoryEvalConstantBool(ctxt, thisstack == True);

		if (mode == WM_SHEET)
		{
			MCSyntaxFactoryExecMethod(ctxt, kMCInterfaceExecSheetStackMethodInfo);
			MCSyntaxFactoryExecMethod(ctxt, kMCInterfaceExecSheetStackByNameMethodInfo);
		}
		else
		{
			if (at != nil)
			{
				at -> compile(ctxt);
				if (aligned != nil)
					aligned -> compile(ctxt);
				else
					MCSyntaxFactoryEvalConstantEnum(ctxt, kMCInterfaceWindowAlignmentTypeInfo, OP_CENTER);
			}
			else
			{
				MCSyntaxFactoryEvalConstantEnum(ctxt, kMCInterfaceWindowPositionTypeInfo, WP_DEFAULT);
				MCSyntaxFactoryEvalConstantEnum(ctxt, kMCInterfaceWindowAlignmentTypeInfo, OP_CENTER);
			}

			MCSyntaxFactoryExecMethod(ctxt, kMCInterfaceExecDrawerStackMethodInfo);
			MCSyntaxFactoryExecMethod(ctxt, kMCInterfaceExecDrawerStackByNameMethodInfo);		
		}
		break;
	case WM_PULLDOWN:
	case WM_POPUP:
	case WM_OPTION:
		if (at != nil)
			at -> compile(ctxt);
		else
			MCSyntaxFactoryEvalConstantNil(ctxt);

		MCSyntaxFactoryEvalConstantInt(ctxt, mode);

		MCSyntaxFactoryExecMethodWithArgs(ctxt, kMCInterfaceExecPopupButtonMethodInfo, 0, 1);
		MCSyntaxFactoryExecMethod(ctxt, kMCInterfaceExecPopupStackMethodInfo);
		MCSyntaxFactoryExecMethod(ctxt, kMCInterfaceExecPopupStackByNameMethodInfo);
		break;
	default:
		break;
	}
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
    sp.skip_token(SP_FACTOR, TT_THE);
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

void MCUnlock::exec_ctxt(MCExecContext &ctxt)
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
		MCscreen->setlockmoves(False);
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

	switch (which)
	{
		case LC_COLORMAP:
			MCLegacyExecUnlockColormap(ctxt);
			break;
		case LC_CURSOR:
			MCInterfaceExecUnlockCursor(ctxt);
			break;
		case LC_ERRORS:
			MCEngineExecUnlockErrors(ctxt);
			break;
		case LC_MENUS:
			MCInterfaceExecUnlockMenus(ctxt);
			break;
		case LC_MSGS:
			MCEngineExecUnlockMessages(ctxt);
			break;
		case LC_MOVES:
			MCInterfaceExecUnlockMoves(ctxt);
			break;
		case LC_RECENT:
			MCInterfaceExecUnlockRecent(ctxt);
			break;
		case LC_SCREEN:
			if (effect != nil)
				MCInterfaceExecUnlockScreenWithEffect(ctxt, effect);
			else
				MCInterfaceExecUnlockScreen(ctxt);
			break;
        case LC_CLIPBOARD:
            MCPasteboardExecUnlockClipboard(ctxt);
            break;
		default:
			break;
    }
}

void MCUnlock::compile(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginStatement(ctxt, line, pos);

	switch (which)
	{
		case LC_COLORMAP:
			MCSyntaxFactoryExecMethod(ctxt, kMCLegacyExecUnlockColormapMethodInfo);
			break;
		case LC_CURSOR:
			MCSyntaxFactoryExecMethod(ctxt, kMCInterfaceExecUnlockCursorMethodInfo);
			break;
		case LC_ERRORS:
			MCSyntaxFactoryExecMethod(ctxt, kMCEngineExecUnlockErrorsMethodInfo);
			break;
		case LC_MENUS:
			MCSyntaxFactoryExecMethod(ctxt, kMCInterfaceExecUnlockMenusMethodInfo);
			break;
		case LC_MSGS:
			MCSyntaxFactoryExecMethod(ctxt, kMCEngineExecUnlockMessagesMethodInfo);
			break;
		case LC_MOVES:
			MCSyntaxFactoryExecMethod(ctxt, kMCInterfaceExecUnlockMovesMethodInfo);
			break;
		case LC_RECENT:
			MCSyntaxFactoryExecMethod(ctxt, kMCInterfaceExecUnlockRecentMethodInfo);
			break;
		case LC_SCREEN:
			if (effect != nil)
			{
				effect -> compile_effect(ctxt);
				MCSyntaxFactoryExecMethod(ctxt, kMCInterfaceExecUnlockScreenWithEffectMethodInfo);
			}
			else
				MCSyntaxFactoryExecMethod(ctxt, kMCInterfaceExecUnlockScreenMethodInfo);
			break;
		default:
			break;
	}

	MCSyntaxFactoryEndStatement(ctxt);
}

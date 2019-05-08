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
#include "variable.h"
#include "stackfileformat.h"

MCCompact::~MCCompact()
{
delete target;
}

Parse_stat MCCompact::parse(MCScriptPoint &sp)
{
	initpoint(sp);
	target = new (nothrow) MCChunk(False);
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
		visibility_type = kMCInterfaceExecGoVisibilityExplicitInvisible;
	if (sp.skip_token(SP_FACTOR, TT_PROPERTY, P_VISIBLE) == PS_NORMAL)
		visibility_type = kMCInterfaceExecGoVisibilityExplicitVisible;
	while (True)
	{
		if (sp.next(type) != PS_NORMAL)
		{
			if (need_target)
			{
				if (ct_class(oterm) == CT_ORDINAL)
				{
					card = new (nothrow) MCCRef;
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
								widget = new (nothrow) MCChunk(False);
								if (widget->parse(sp, False) != PS_NORMAL)
								{
									MCperror->add(PE_GO_BADWIDGETEXP, sp);
									return PS_ERROR;
								}
								direction = nterm;
								
								break;
							}

							card = new (nothrow) MCCRef;
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
						card = new (nothrow) MCCRef;
						card->etype = nterm;
						sp.skip_token(SP_FACTOR, TT_CHUNK, CT_CARD);
						break;
					case CT_HOME:
					case CT_HELP:
						stack = new (nothrow) MCCRef;
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
					curref = new (nothrow) MCCRef;
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
					card = new (nothrow) MCCRef;
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
				stack = new (nothrow) MCCRef;
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

MCStack *MCGo::findstack(MCExecContext &ctxt, MCStringRef p_value, Chunk_term etype, MCCard *&cptr)
{
    MCStack *sptr = nullptr;

    if (MCInterfaceStringCouldBeStack(p_value))
    {
        sptr = MCInterfaceTryToEvalStackFromString(p_value);
        if (sptr == nullptr)
        {
            if (MCresult->isclear())
                ctxt . SetTheResultToCString("can't build stack from string");
        }
        return sptr;
    }
    
    if (etype == CT_STACK)
        return nullptr;
    else
        sptr = MCdefaultstackptr->findstackname_string(p_value);

    if (sptr != nullptr)
        return sptr;

	MCObject *objptr;
	MCChunk *tchunk = new (nothrow) MCChunk(False);
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

			if (sptr != nullptr && stack->next != NULL)
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
				if (sptr == nullptr)
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
		MCInterfaceExecGoCardInWindow(ctxt, cptr, *t_window, visibility_type, thisstack == True);
	else
        MCInterfaceExecGoCardAsMode(ctxt, cptr, mode, visibility_type, thisstack == True);
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
	object = new (nothrow) MCChunk(False);
	if (object->parse(sp, False) != PS_NORMAL)
	{
		MCperror->add
		(PE_HIDE_BADTARGET, sp);
		return PS_ERROR;
	}
	if (sp.skip_token(SP_REPEAT, TT_UNDEFINED) == PS_NORMAL)
	{
		sp.skip_token(SP_COMMAND, TT_STATEMENT, S_VISUAL);
		effect = new (nothrow) MCVisualEffect;
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
	dest = new (nothrow) MCChunk(True);
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

		card = new (nothrow) MCChunk(False);
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

MCSave::~MCSave()
{
	delete target;
	delete filename;
    delete format;
}

Parse_stat MCSave::parse(MCScriptPoint &sp)
{
	initpoint(sp);
	target = new (nothrow) MCChunk(False);
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
	ton = new (nothrow) MCChunk(False);
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
		effect = new (nothrow) MCVisualEffect;
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
		target = new (nothrow) MCChunk(False);
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
							MCStringCopySubstring(*t_position_data, MCRangeMakeMinMax(t_delimiter, MCStringGetLength(*t_position_data)), &t_alignment);
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
			effect = new (nothrow) MCVisualEffect;
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

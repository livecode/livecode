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
#include "object.h"
#include "dispatch.h"
#include "stack.h"
#include "aclip.h"
#include "vclip.h"
#include "card.h"
#include "group.h"
#include "button.h"
#include "field.h"
#include "image.h"
#include "graphic.h"
#include "eps.h"
#include "scrolbar.h"
#include "player.h"
#include "sellst.h"
#include "stacklst.h"
#include "cardlst.h"
#include "chunk.h"
#include "mcerror.h"
#include "util.h"
#include "debug.h"

#include "globals.h"

MCCRef::MCCRef()
{
	etype = otype = ptype = CT_UNDEFINED;
	startpos = endpos = NULL;
	next = NULL;
}

MCCRef::~MCCRef()
{
	delete startpos;
	delete endpos;
}

MCChunk::MCChunk(Boolean isforset)
{
	url = stack = background = card = group = object = NULL;
	cline = item = word = token = character = NULL;
	source = NULL;
	destvar = NULL;
	destobj = NULL;
	if (isforset)
		desttype = DT_ISDEST;
	else
		desttype = DT_UNDEFINED;
	function = F_UNDEFINED;
	marked = False;
	next = NULL;
    
    // MW-2014-05-28: [[ Bug 11928 ]] We assume (at first) we are not a transient text chunk (i.e. one that is evaluated from a var).
    m_transient_text_chunk = false;
}

MCChunk::~MCChunk()
{
	delete url;
	while (stack != NULL)
	{
		MCCRef *tptr = stack;
		stack = stack->next;
		delete tptr;
	}
	delete background;
	delete card;
	while(group != NULL)
	{
		MCCRef *tptr = group;
		group = group->next;
		delete tptr;
	}
	while(object != NULL)
	{
		MCCRef *tptr = object;
		object = object->next;
		delete tptr;
	}
	delete cline;
	delete item;
	delete word;
	delete token;
	delete character;
	delete source;
	delete destvar;
}

Parse_stat MCChunk::parse(MCScriptPoint &sp, Boolean doingthe)
{
	Symbol_type type;
	Boolean need_target = True;
	const LT *te;
	MCCRef *curref = NULL;
	Chunk_term oterm = CT_UNDEFINED;
	Chunk_term nterm = CT_UNDEFINED;
	Chunk_term lterm = CT_UNDEFINED;

#ifdef COLLECTING_CHUNKS
	MCSyntaxCollector collect(sp, doingthe, desttype == DT_ISDEST);
#endif

	initpoint(sp);
	while (True)
	{
		if (sp.next(type) != PS_NORMAL)
			if (need_target)
			{
				MCperror->add(PE_CHUNK_NOCHUNK, sp);
				return PS_ERROR;
			}
			else
				break;
		if (type == ST_ID)
		{
			te = NULL;
			if (sp.lookup(SP_FACTOR, te) != PS_NORMAL || te->type == TT_PROPERTY)
			{
				if (need_target)
				{
					if (desttype == DT_ISDEST && stack == NULL && background == NULL
					        && card == NULL && group == NULL && object == NULL)
					{
						MCExpression *newfact = NULL;
						// MW-2011-06-22: [[ SERVER ]] Update to use SP findvar method to take into account
						//   execution outwith a handler.
						if (doingthe
						        || sp.findvar(sp.gettoken_nameref(), &destvar) != PS_NORMAL
						        && (MCexplicitvariables
						            || sp.lookupconstant(&newfact) == PS_NORMAL
						            || sp.findnewvar(sp.gettoken_nameref(), kMCEmptyName, &destvar) != PS_NORMAL))
						{
							delete newfact;
							MCperror->add(PE_CHUNK_NOVARIABLE, sp);
							return PS_ERROR;
						}
						destvar->parsearray(sp);
						desttype = DT_VARIABLE;
						return PS_NORMAL;
					}
					else
					{
						if (doingthe)
							sp.backup();
						sp.backup();
						if (sp.parseexp(True, False, &source) != PS_NORMAL)
						{
							MCperror->add
							(PE_CHUNK_BADEXP, sp);
							return PS_ERROR;
						}
						desttype = DT_EXPRESSION;
					}
				}
				else
					sp.backup();
				break;
			}
			switch (te->type)
			{
			case TT_THE:
				// MW-2013-01-17: [[ Bug 10644 ]] If we have a string chunk of a property
				//   then it should be evaluated as a string. However, if the string chunk
				//   is of a previously parsed object then we must pass as a dest so the
				//   object resolves correctly.
				if (desttype != DT_ISDEST && (cline != NULL || item != NULL
				                              || word != NULL || token != NULL
				                              || character != NULL) &&
											 (stack == nil && background == nil &&
											  card == nil && group == nil &&
											  object == nil))
				{
					sp.backup();
					if (sp.parseexp(True, False, &source) != PS_NORMAL)
					{
						MCperror->add
						(PE_CHUNK_BADEXP, sp);
						return PS_ERROR;
					}
					return PS_NORMAL;
				}
				doingthe = True;
				break;
			case TT_PREP:
				if (need_target)
				{
					MCperror->add
					(PE_CHUNK_BADPREP, sp);
					return PS_ERROR;
				}
				sp.backup();
				return PS_NORMAL;
			case TT_TO:
				if (curref != NULL && lterm >= CT_LINE)
				{
					if (curref->etype == CT_RANGE)
					{
						MCperror->add
						(PE_CHUNK_BADRANGE, sp);
						return PS_ERROR;
					}
					curref->etype = CT_RANGE;
					if (sp.parseexp(False, False, &curref->endpos) != PS_NORMAL)
					{
						MCperror->add
						(PE_CHUNK_NOENDEXP, sp);
						return PS_ERROR;
					}
				}
				else
				{
					sp.backup();
					return PS_NORMAL;
				}
				break;
			case TT_IN:
				if (!need_target)
				{
					sp.backup();
					return PS_NORMAL;
				}
				break;
			case TT_OF:
				need_target = True;
				break;
			case TT_CLASS:
				if (te->which != CT_MARKED)
				{
					MCperror->add
					(PE_CHUNK_NOCHUNK, sp);
					return PS_ERROR;
				}
				marked = True;
				break;
			case TT_CHUNK:
				nterm = (Chunk_term)te->which;
				// MW-2013-08-05: [[ ThisMe ]] If 'this' is followed by 'me' we become 'this me'.
				if (nterm == CT_THIS &&
					sp . skip_token(SP_FACTOR, TT_FUNCTION, F_ME) == PS_NORMAL)
				{
					// Destination type is 'this me' handled in MCChunk::getobj()
					desttype = DT_THIS_ME;
					// Destination object is the script being compiled
					destobj = sp.getobj();
					// Nothing can come after 'this me' so return success.
					return PS_NORMAL;
				}
				
				switch (ct_class(nterm))
				{
				case CT_ORDINAL:
					oterm = nterm;
					break;
				case CT_TYPES:
					// MW-2011-08-08: [[ Bug ]] Allow control ... of control ...
					if (lterm != CT_UNDEFINED && nterm > lterm
					        || nterm == lterm && nterm != CT_GROUP && nterm != CT_LAYER && nterm != CT_STACK)
					{
						MCperror->add(PE_CHUNK_BADORDER, sp);
						return PS_ERROR;
					}
					curref = new MCCRef;
					curref->otype = nterm;
					if (curref->otype == CT_BACKGROUND || curref->otype == CT_CARD)
					{
						Symbol_type itype;
						const LT *ite;
						if (sp.next(itype) == PS_NORMAL)
						{
							if (sp.lookup(SP_FACTOR, ite) == PS_NORMAL
							        && ite->type == TT_CHUNK
							        && ite->which >= CT_LAYER && ite->which <= CT_FIELD)
							{
								curref->ptype = curref->otype;
								nterm = curref->otype = (Chunk_term)ite->which;
							}
							else
								sp.backup();
						}
					}
					if (oterm == CT_UNDEFINED)
					{
						if (sp.skip_token(SP_FACTOR, TT_PROPERTY) == PS_NORMAL)
							curref->etype = CT_ID;
						else
							curref->etype = CT_EXPRESSION;
						if (sp.parseexp(curref->otype <= CT_FIELD, False,
						                &curref->startpos) != PS_NORMAL)
						{
							MCperror->add(PE_CHUNK_NOSTARTEXP, sp);
							delete curref;
							return PS_ERROR;
						}
					}
					else
					{
						curref->etype = oterm;
						oterm = CT_UNDEFINED;
					}
					switch (curref->otype)
					{
					case CT_URL:
					case CT_URL_HEADER:
						url = curref;
						break;
					case CT_TOP_LEVEL:
					case CT_MODELESS:
					case CT_PALETTE:
					case CT_MODAL:
					case CT_PULLDOWN:
					case CT_POPUP:
					case CT_OPTION:
					case CT_STACK:
						if (oterm > CT_STACK)
						{
							MCperror->add(PE_CHUNK_BADSTACKREF, sp);
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
					case CT_GROUP:
						curref->next = group;
						group = curref;
						break;
					case CT_LAYER:
						// MW-2011-08-08: [[ Bug ]] Chain 'control' chunks allowing things like 'graphic ... of control ...'
						//   where the latter chunk refers to a group.
						curref->next = object;
						object = curref;
						break;
					case CT_AUDIO_CLIP:
					case CT_VIDEO_CLIP:
					case CT_MENU:
					case CT_BUTTON:
					case CT_SCROLLBAR:
					case CT_PLAYER:
					case CT_IMAGE:
					case CT_FIELD:
					case CT_GRAPHIC:
					case CT_EPS:
					case CT_COLOR_PALETTE:
					case CT_MAGNIFY:
						object = curref;
						break;

					case CT_LINE:
						cline = curref;
						break;
					case CT_ITEM:
						item = curref;
						break;
					case CT_WORD:
						word = curref;
						break;
					case CT_TOKEN:
						token = curref;
						break;
					case CT_CHARACTER:
						character = curref;
						break;
					default: /* curref->otype */
						fprintf(stderr, "MCChunk: ERROR bad chunk type %d\n", nterm);
						break;
					}
					if (nterm < CT_LINE)
						need_target = False;
					lterm = nterm;
					break;
				default: /* chunk class */
					MCperror->add(PE_CHUNK_BADCHUNK, sp);
					return PS_ERROR;
				}
				doingthe = False;
				break;
			case TT_FUNCTION:
				function = (Functions)te->which;
				switch (function)
				{
				case F_CLICK_CHUNK:
				case F_CLICK_CHAR_CHUNK:
				case F_CLICK_FIELD:
				case F_CLICK_LINE:
				case F_CLICK_TEXT:
				case F_FOUND_CHUNK:
				case F_FOUND_FIELD:
				case F_FOUND_LINE:
				case F_FOUND_TEXT:
				case F_SELECTED_CHUNK:
				case F_SELECTED_FIELD:
				case F_SELECTED_IMAGE:
				case F_SELECTED_LINE:
				case F_SELECTED_TEXT:
				case F_MOUSE_CONTROL:
				case F_MOUSE_CHUNK:
				case F_MOUSE_CHAR_CHUNK:
				case F_MOUSE_LINE:
				case F_MOUSE_TEXT:
				case F_DRAG_SOURCE:
				case F_DRAG_DESTINATION:
				case F_FOCUSED_OBJECT:
					if (desttype == DT_ISDEST)
					{ // delete the selectedLine of field x
						if (function == F_SELECTED_LINE || function == F_SELECTED_CHUNK)
						{}
					}
					else if (cline != NULL || item != NULL || word != NULL
						        || token != NULL || character != NULL)
					{
						sp.backup();
						if (sp.parseexp(True, False, &source) != PS_NORMAL)
						{
							MCperror->add(PE_CHUNK_BADEXP, sp);
							return PS_ERROR;
						}
						return PS_NORMAL;
					}
					desttype = DT_FUNCTION;
					break;
				case F_ME:
					desttype = DT_ME;
					destobj = sp.getobj();
					break;
				case F_MENU_OBJECT:
					desttype = DT_MENU_OBJECT;
					break;
				case F_TARGET:
					desttype = DT_TARGET;
					break;
				// MW-2008-11-05: [[ Owner Reference ]] If we encounter 'the owner of ...' then
				//   parse the syntax as:
				//     'the' owner [ 'of' ] <chunk>
				//     owner 'of' <chunk>
				case F_OWNER:
					desttype = DT_OWNER;
					if (sp . skip_token(SP_FACTOR, TT_OF) != PS_NORMAL && !doingthe)
					{
						MCperror -> add(PE_PROPERTY_NOTOF, sp);
						return PS_ERROR;
					}
					source = new MCChunk(False);
					if (static_cast<MCChunk *>(source) -> parse(sp, False) != PS_NORMAL)
					{
						MCperror->add(PE_PROPERTY_BADCHUNK, sp);
						return PS_ERROR;
					}
					return PS_NORMAL;
				case F_TEMPLATE_STACK:
				case F_TEMPLATE_AUDIO_CLIP:
				case F_TEMPLATE_VIDEO_CLIP:
				case F_TEMPLATE_GROUP:
				case F_TEMPLATE_CARD:
				case F_TEMPLATE_BUTTON:
				case F_TEMPLATE_FIELD:
				case F_TEMPLATE_IMAGE:
				case F_TEMPLATE_SCROLLBAR:
				case F_TEMPLATE_PLAYER:
				case F_TEMPLATE_GRAPHIC:
				case F_TEMPLATE_EPS:
					desttype = (Dest_type)(te->which - F_TEMPLATE_BUTTON + DT_BUTTON);
					break;
				case F_SELECTED_OBJECT:
					desttype = DT_SELECTED;
					break;
				case F_TOP_STACK:
					desttype = DT_TOP_STACK;
					break;
				case F_CLICK_STACK:
					desttype = DT_CLICK_STACK;
					break;
				case F_MOUSE_STACK:
					desttype = DT_MOUSE_STACK;
					break;
				default: /* function */
					if (need_target && desttype != DT_ISDEST)
					{
						if (doingthe)
							sp.backup();
						sp.backup();
						if (sp.parseexp(True, False, &source) != PS_NORMAL)
						{
							MCperror->add(PE_CHUNK_BADEXP, sp);
							return PS_ERROR;
						}
						// MW-2013-06-20: [[ Bug 10966 ]] Make sure we mark the chunk as being of
						//   'expression' type. This means 'source' will be evaluated and then parsed
						//   as a control reference in 'getobj()' context. This has wider implications
						//   that just 'controlAtLoc()' - it means any function used on the rhs of
						//   'of' for properties will now function correctly.
						desttype = DT_EXPRESSION;
						return PS_NORMAL;
					}
					else
					{
						sp.backup();
						if (need_target)
							return PS_ERROR;
						return PS_NORMAL;
					}
				}
				if (sp.skip_token(SP_FACTOR, TT_LPAREN) == PS_NORMAL)
					if (sp.skip_token(SP_FACTOR, TT_RPAREN) != PS_NORMAL)
					{
						MCperror->add
						(PE_CHUNK_BADEXP, sp);
						return PS_ERROR;
					}
				return PS_NORMAL;
			default: /* factor */
				if (need_target)
				{
					if (stack != NULL || background != NULL || card != NULL
					        || group != NULL || object != NULL)
					{
						MCperror->add
						(PE_CHUNK_NOCHUNK, sp);
						return PS_ERROR;
					}
					if (desttype != DT_UNDEFINED)
					{
						MCperror->add
						(PE_CHUNK_BADDEST, sp);
						return PS_ERROR;
					}
					if (doingthe)
						sp.backup();
					sp.backup();
					if (sp.parseexp(True, False, &source) != PS_NORMAL)
					{
						MCperror->add(PE_CHUNK_BADEXP, sp);
						return PS_ERROR;
					}
				}
				else
					sp.backup();
				return PS_NORMAL;
			}
		}
		else
		{ /* token type != ST_ID */
			if (need_target)
			{
				if (stack != NULL || background != NULL || card != NULL
				        || group != NULL || object != NULL)
				{
					MCperror->add(PE_CHUNK_NOCHUNK, sp);
					return PS_ERROR;
				}
				if (desttype != DT_UNDEFINED)
				{
					MCperror->add(PE_CHUNK_BADDEST, sp);
					return PS_ERROR;
				}
				sp.backup();
				if (sp.parseexp(True, True, &source) != PS_NORMAL)
				{
					MCperror->add(PE_CHUNK_BADEXP, sp);
					return PS_ERROR;
				}

				// MW-2007-07-03: [[ Bug 1925 ]] We have just parsed an expression so set
				//   the desttype appropriately. This means things such as the foo of (...)
				//   works correctly.
				desttype = DT_EXPRESSION;
			}
			else
				sp.backup();
			break;
		}
	}
	return PS_NORMAL;
}

MCVarref *MCChunk::getrootvarref(void)
{
	if (destvar != NULL)
		return destvar;

	if (source != NULL)
		return source -> getrootvarref();

	return NULL;
}

void MCChunk::take_components(MCChunk *tchunk)
{
	if (tchunk->character != NULL)
	{
		delete character;
		character = tchunk->character;
		tchunk->character = NULL;
	}
	if (tchunk->token != NULL)
	{
		delete token;
		token = tchunk->token;
		tchunk->token = NULL;
	}
	if (tchunk->word != NULL)
	{
		delete word;
		word = tchunk->word;
		tchunk->word = NULL;
	}
	if (tchunk->item != NULL)
	{
		delete item;
		item = tchunk->item;
		tchunk->item = NULL;
	}
	if (tchunk->cline != NULL)
	{
		delete cline;
		cline = tchunk->cline;
		tchunk->cline = NULL;
	}
    
    // MW-2014-05-28: [[ Bug 11928 ]] As soon as we take components from a chunk then
    //   mark us as transient text, so that we don't use the same components next time.
    m_transient_text_chunk = true;
}

Exec_stat MCChunk::getobj(MCExecPoint &ep, MCObject *&objptr,
                          uint4 &parid, Boolean recurse)
{
	objptr = NULL;
	parid = 0;
	MCStack *sptr = MCdefaultstackptr;
	MCGroup *bptr = NULL;
	MCCard *cptr = NULL;
	MCGroup *gptr = NULL;

	MCExecPoint ep2(ep);
	if (desttype != DT_UNDEFINED && desttype != DT_ISDEST)
	{
		Exec_stat stat = ES_ERROR;
		switch (desttype)
		{
		case DT_EXPRESSION:
		case DT_VARIABLE:
			if (recurse)
			{
				if (desttype == DT_EXPRESSION)
				{
					if (source->eval(ep2) != ES_NORMAL)
					{
						MCeerror->add(EE_CHUNK_BADOBJECTEXP, line, pos);
						return ES_ERROR;
					}
				}
				else
				{
					if (destvar->eval(ep2) != ES_NORMAL)
					{
						MCeerror->add(EE_CHUNK_BADOBJECTEXP, line, pos);
						return ES_ERROR;
					}
				}
				MCScriptPoint sp(ep2);
				MCChunk *tchunk = new MCChunk(False);
				MCerrorlock++;
				Symbol_type type;
				if (tchunk->parse(sp, False) == PS_NORMAL
				        && sp.next(type) == PS_EOF)
					stat = ES_NORMAL;
				MCerrorlock--;
				if (stat == ES_NORMAL)
					stat = tchunk->getobj(ep2, objptr, parid, False);
				if (stat == ES_NORMAL)
					take_components(tchunk);
				delete tchunk;
			}
			if (stat != ES_NORMAL)
			{
				MCeerror->add
				(EE_CHUNK_BADOBJECTEXP, line, pos, ep2.getsvalue());
				return ES_ERROR;
			}
			break;
		case DT_ME:
			// MW-2009-01-28: [[ Inherited parentScripts ]]
			// If we are executing in the context of a parent-handle invocation
			// (indicated by getparentscript() of the EP being non-NULL) 'me'
			// refers to the derived object context, otherwise it is the object
			// we were compiled in.
			if (ep.getparentscript() == NULL)
				objptr = destobj;
			else
				objptr = ep . getobj();
			break;
		// MW-2013-08-05: [[ ThisMe ]] 'this me' is the object of the script
		//   currently being executed, so it is always the object the script
		//   was compiled into.
		case DT_THIS_ME:
			objptr = destobj;
			break;
		case DT_MENU_OBJECT:
			objptr = MCmenuobjectptr;
			break;
		case DT_TARGET:
			objptr = MCtargetptr;
			break;
		// MW-2008-11-05: [[ Owner Reference ]] If the desttype is DT_OWNER it means that <source>
		//   must point to an MCChunk object (as that's how its parsed).
		//   In this case attempt to evaluate it and then step up one in the ownership chain. Note
		//   that we can't step higher than a mainstack, so we set objptr to NULL in this case.
		case DT_OWNER:
			if (static_cast<MCChunk *>(source) -> getobj(ep2, objptr, parid, True) != ES_NORMAL)
			{
				MCeerror -> add(EE_CHUNK_BADOBJECTEXP, line, pos);
				return ES_ERROR;
			}
			if (objptr -> gettype() == CT_STACK && MCdispatcher -> ismainstack(static_cast<MCStack *>(objptr)))
				objptr = NULL;
			else
				objptr = objptr -> getparent();
			break;
		case DT_STACK:
			objptr = MCtemplatestack;
			return ES_NORMAL;
		case DT_AUDIO_CLIP:
			objptr = MCtemplateaudio;
			return ES_NORMAL;
		case DT_VIDEO_CLIP:
			objptr = MCtemplatevideo;
			return ES_NORMAL;
		case DT_GROUP:
			objptr = MCtemplategroup;
			return ES_NORMAL;
		case DT_CARD:
			objptr = MCtemplatecard;
			return ES_NORMAL;
		case DT_BUTTON:
			objptr = MCtemplatebutton;
			return ES_NORMAL;
		case DT_FIELD:
			objptr = MCtemplatefield;
			return ES_NORMAL;
		case DT_IMAGE:
			objptr = MCtemplateimage;
			return ES_NORMAL;
		case DT_SCROLLBAR:
			objptr = MCtemplatescrollbar;
			return ES_NORMAL;
		case DT_PLAYER:
			objptr = MCtemplateplayer;
			return ES_NORMAL;
		case DT_GRAPHIC:
			objptr = MCtemplategraphic;
			return ES_NORMAL;
		case DT_EPS:
			objptr = MCtemplateeps;
			return ES_NORMAL;
		case DT_ERROR:
			objptr = MCerrorptr;
			break;
		case DT_SELECTED:
			objptr = MCselected->getfirst();
			break;
		case DT_TOP_STACK:
			objptr = MCtopstackptr;
			break;
		case DT_CLICK_STACK:
			objptr = MCclickstackptr;
			break;
		case DT_MOUSE_STACK:
			objptr = MCmousestackptr;
			break;
		case DT_FUNCTION:
			switch (function)
			{
			case F_CLICK_CHUNK:
			case F_CLICK_CHAR_CHUNK:
			case F_CLICK_FIELD:
			case F_CLICK_LINE:
			case F_CLICK_TEXT:
				objptr = MCclickfield;
				break;
			case F_SELECTED_CHUNK:
			case F_SELECTED_FIELD:
			case F_SELECTED_LINE:
			case F_SELECTED_TEXT:
				objptr = MCactivefield;
				break;
			case F_SELECTED_IMAGE:
				objptr = MCactiveimage;
				break;
			case F_FOUND_CHUNK:
			case F_FOUND_FIELD:
			case F_FOUND_LINE:
			case F_FOUND_TEXT:
				objptr = MCfoundfield;
				break;
			case F_MOUSE_CONTROL:
			case F_MOUSE_CHUNK:
			case F_MOUSE_CHAR_CHUNK:
			case F_MOUSE_LINE:
			case F_MOUSE_TEXT:
				// OK-2009-01-19: Refactored to ensure behaviour is the same as the mouseControl.
				if (MCmousestackptr != NULL)
					objptr = MCmousestackptr->getcard()->getmousecontrol();
				else
					objptr = NULL;
				break;
			case F_FOCUSED_OBJECT:
				objptr = MCdefaultstackptr->getcard()->getkfocused();
				if (objptr == NULL)
					objptr = MCdefaultstackptr->getcard();
				break;
			case F_DRAG_SOURCE:
				objptr = MCdragsource;
				break;
			case F_DRAG_DESTINATION:
				objptr = MCdragdest;
				break;
			default:
				objptr = NULL;
				break;
			}
			break;
		default:
			objptr = NULL;
			break;
		}
		if (objptr == NULL)
		{
			MCeerror->add(EE_CHUNK_NOTARGET, line, pos);
			return ES_ERROR;
		}
		switch (objptr->gettype())
		{
		case CT_STACK:
			sptr = (MCStack *)objptr;
			break;
		case CT_CARD:
			cptr = (MCCard *)objptr;
			sptr = cptr->getstack();
			break;
		case CT_GROUP:
			bptr = gptr = (MCGroup *)objptr;
			cptr = gptr->getcard();
			sptr = cptr->getstack();
			break;
		case CT_AUDIO_CLIP:
		case CT_VIDEO_CLIP:
		case CT_LAYER:
		case CT_MENU:
		case CT_BUTTON:
		case CT_IMAGE:
		case CT_FIELD:
		case CT_GRAPHIC:
		case CT_EPS:
		case CT_SCROLLBAR:
		case CT_PLAYER:
		case CT_MAGNIFY:
		case CT_COLOR_PALETTE:
			MCCard *t_card;
			t_card = objptr -> getcard(parid);
			if (t_card == NULL)
				return ES_ERROR;

			parid = t_card -> getid();
			return ES_NORMAL;
		default:
			break;
		}
	}
	else
		if (stack == NULL && background == NULL && card == NULL
		        && group == NULL && object == NULL)
			return ES_ERROR;
		else
			if (url != NULL)
			{
				uint4 offset;
				if (MCU_offset(SIGNATURE, ep.getsvalue(), offset) && (ep . getsvalue() . getlength() > 8 && strncmp(ep . getsvalue() . getstring(), "REVO", 4) == 0))
				{
					IO_handle stream = MCS_fakeopen(ep.getsvalue());
					if (MCdispatcher->readfile(NULL, NULL, stream, sptr) != IO_NORMAL)
					{
						MCS_close(stream);
						return ES_ERROR;
					}
					MCS_close(stream);
				}
				else
					return ES_ERROR;
			}
			else
				sptr = MCdefaultstackptr;
	if (stack != NULL)
	{
		switch (stack->etype)
		{
		case CT_EXPRESSION:
		case CT_ID:
			if (stack->startpos->eval(ep2) != ES_NORMAL)
			{
				MCeerror->add(EE_CHUNK_BADSTACKEXP, line, pos);
				return ES_ERROR;
			}
			if (stack->etype == CT_ID)
				sptr = sptr->findstackid(ep2.getuint4());
			else
				sptr = sptr->findstackname(ep2.getsvalue());
			break;
		case CT_THIS:
			break;
		default:
			MCeerror->add(EE_CHUNK_BADSTACKEXP, line, pos);
			return ES_ERROR;
		}
		if (sptr != NULL && stack->next != NULL)
		{
			switch (stack->next->etype)
			{
			case CT_EXPRESSION:
			case CT_ID:
				if (stack->next->startpos->eval(ep2) != ES_NORMAL)
				{
					MCeerror->add(EE_CHUNK_BADSTACKEXP, line, pos);
					return ES_ERROR;
				}
				if (stack->next->etype == CT_ID)
					sptr = sptr->findsubstackid(ep2.getuint4());
				else
					sptr = sptr->findsubstackname(ep2.getsvalue());
				break;
			case CT_THIS:
				break;
			default:
				MCeerror->add(EE_CHUNK_BADSTACKEXP, line, pos);
				return ES_ERROR;
			}
		}
	}
	if (sptr == NULL)
	{
		MCeerror->add(EE_CHUNK_NOSTACK, line, pos);
		return ES_ERROR;
	}

	if (object != NULL && (object->otype == CT_AUDIO_CLIP || object->otype == CT_VIDEO_CLIP))
	{
		if (ct_class(object->etype) == CT_ORDINAL)
			objptr = sptr->getAV(object->etype, MCnullmcstring, object->otype);
		else
		{
			if (object->startpos->eval(ep2) != ES_NORMAL)
			{
				MCeerror->add(EE_CHUNK_BADOBJECTEXP, line, pos);
				return ES_ERROR;
			}
			objptr = sptr->getAV(object->etype, ep2.getsvalue(), object->otype);
			if (objptr == NULL)
				objptr = sptr->getobjname(object->otype, ep2.getsvalue());
		}
		if (objptr == NULL)
		{
			// Lookup the name we are searching for. If it doesn't exist, then no object can
			// have it as a name.
			MCNameRef t_obj_name;
			t_obj_name = MCNameLookupWithOldString(ep2.getsvalue(), kMCCompareCaseless);
			if (t_obj_name != nil)
			{
			if (object->otype == CT_VIDEO_CLIP)
			{
				IO_cleanprocesses();
				MCPlayer *tptr = MCplayers;
				while (tptr != NULL)
				{
						if (tptr -> hasname(t_obj_name))
					{
						objptr = tptr;
						break;
					}
					tptr = tptr->getnextplayer();
				}
			}
				else if (MCacptr != NULL && MCacptr -> hasname(t_obj_name))
					objptr = MCacptr;
			}
		}
		if (objptr == NULL)
		{
			MCeerror->add(EE_CHUNK_NOOBJECT, line, pos, ep2.getsvalue());
			return ES_ERROR;
		}
		return ES_NORMAL;
	}

	if (background == NULL && card == NULL && group == NULL && object == NULL)
	{
		if (objptr == NULL)
			objptr = sptr;
		return ES_NORMAL;
	}

	if (background != NULL)
	{
		switch (ct_class(background->etype))
		{
		case CT_ORDINAL:
			bptr = (MCGroup *)sptr->getbackground(background->etype, MCnullmcstring,
			                                      CT_GROUP);
			break;
		case CT_ID:
		case CT_EXPRESSION:
			if (background->startpos->eval(ep2) != ES_NORMAL)
			{
				MCeerror->add(EE_CHUNK_BADBACKGROUNDEXP, line, pos);
				return ES_ERROR;
			}
			bptr = (MCGroup *)sptr->getbackground(background->etype,
			                                      ep2.getsvalue(), CT_GROUP);
			break;
		default:
			MCeerror->add(EE_CHUNK_BADBACKGROUNDEXP, line, pos);
			return ES_ERROR;
		}
		if (bptr == NULL)
		{
			MCeerror->add(EE_CHUNK_NOBACKGROUND, line, pos);
			return ES_ERROR;
		}
	}

	if (card == NULL && group == NULL && object == NULL)
	{
		objptr = bptr;
		return ES_NORMAL;
	}
	else
		if (bptr != NULL)
			sptr->setbackground(bptr);

	Boolean thiscard = False;
	if (card != NULL)
	{
		switch (ct_class(card->etype))
		{
		case CT_DIRECT:
			// recent
			break;
		case CT_ORDINAL:
			if (card->etype == CT_RECENT)
				cptr = MCrecent->getrel(-1);
			else
			{
				if (marked)
					sptr->setmark();
				cptr = sptr->getchild(card->etype, MCnullmcstring, card->otype);
			}
			break;
		case CT_ID:
		case CT_EXPRESSION:
			if (marked)
				sptr->setmark();
			if (card->startpos->eval(ep2) != ES_NORMAL)
			{
				sptr->clearbackground();
				MCeerror->add(EE_CHUNK_BADCARDEXP, line, pos);
				return ES_ERROR;
			}
			cptr = sptr->getchild(card->etype, ep2.getsvalue(), card->otype);
			break;
		default:
			sptr->clearbackground();
			MCeerror->add(EE_CHUNK_BADCARDEXP, line, pos);
			return ES_ERROR;
		}
	}
	else if (cptr == NULL && bptr == NULL)
	{
		cptr = sptr->getchild(CT_THIS, MCnullmcstring, CT_CARD);
		thiscard = True;
	}

	sptr->clearbackground();
	sptr->clearmark();
	if (cptr == NULL)
	{
		if (bptr == NULL || group == NULL && object == NULL)
		{
			MCeerror->add(EE_CHUNK_NOCARD, line, pos);
			return ES_ERROR;
		}
	}
	else
		parid = cptr->getid();

	if (group == NULL && object == NULL)
	{
		objptr = cptr;
		return ES_NORMAL;
	}

	// MW-2011-08-09: [[ Groups ]] If there was an explicit stack reference,
	//   but no explicit card, we search the stack directly for the CT_ID
	//   case.
	bool t_stack_override;
	t_stack_override = false;
	if (stack != NULL && card == NULL)
		t_stack_override = true;

	MCCRef *tgptr = group;
	while (tgptr != NULL)
	{
		if (gptr != NULL)
			cptr = NULL;

		Chunk_term t_etype;
		t_etype = ct_class(tgptr->etype);
		if (t_etype == CT_ID || t_etype == CT_EXPRESSION || t_etype == CT_ORDINAL)
		{
			if (t_etype == CT_ID || t_etype == CT_EXPRESSION)
			{
				if (tgptr->startpos->eval(ep2) != ES_NORMAL)
				{
					MCeerror->add(EE_CHUNK_BADBACKGROUNDEXP, line, pos);
					return ES_ERROR;
				}
			}

			// If the expression type isn't ID, we don't search the stack.
			if (t_etype != CT_ID)
				t_stack_override = false;

			if (cptr != NULL)
			{
				gptr = (MCGroup *)cptr->getchild(tgptr->etype, ep2.getsvalue(), tgptr->otype, tgptr->ptype);

				// If the control couldn't be found on the card, and we are in
				// stack override mode, then search the stack. We do this *after*
				// searching the card as searching the stack will take longer.
				if (gptr == nil && t_stack_override && ep2.ston() == ES_NORMAL)
				{
					t_stack_override = false;
					gptr = (MCGroup *)sptr->getcontrolid(tgptr->otype, ep2.getuint4(), true);
				}
			}
			else
				gptr = (MCGroup *)bptr->getchild(tgptr->etype, ep2.getsvalue(), tgptr->otype, tgptr->ptype);

		}
		else
		{
			MCeerror->add(EE_CHUNK_BADBACKGROUNDEXP, line, pos);
			return ES_ERROR;
		}

		if (gptr == NULL)
		{
			MCeerror->add(EE_CHUNK_NOBACKGROUND, line, pos);
			return ES_ERROR;
		}

		tgptr = tgptr->next;
		bptr = gptr;
	}
	
	if (object == NULL)
	{
		objptr = gptr;
		return ES_NORMAL;
	}
	
	// MW-2011-08-08: [[ Bug ]] Loop through chain of object chunks. This allows
	//   things like field ... of control ... of.
	MCCRef *toptr = object;
	while(toptr != NULL)
	{
		if (toptr->otype == CT_MENU)
		{
			if (MCmenubar != NULL)
				gptr = MCmenubar;
			else if (MCdefaultmenubar != NULL)
				gptr = MCdefaultmenubar;
			else
			{
				MCeerror->add(EE_CHUNK_NOOBJECT, line, pos, ep2.getsvalue());
				return ES_ERROR;
			}
		}

		Chunk_term t_etype;
		t_etype = ct_class(toptr->etype);
		if (t_etype == CT_ID || t_etype == CT_EXPRESSION || t_etype == CT_ORDINAL)
		{
			if (t_etype == CT_ID || t_etype == CT_EXPRESSION)
			{
				if (toptr->startpos->eval(ep2) != ES_NORMAL)
				{
					MCeerror->add(EE_CHUNK_BADOBJECTEXP, line, pos);
					return ES_ERROR;
				}
			}

			// If the expression type isn't ID, we don't search the stack.
			if (t_etype != CT_ID)
				t_stack_override = false;

			if (gptr != NULL)
				objptr = gptr->getchild(toptr->etype, ep2.getsvalue(), toptr->otype, toptr->ptype);
			else if (cptr != NULL)
			{
				objptr = cptr->getchild(toptr->etype, ep2.getsvalue(), toptr->otype, toptr->ptype);

				// If the control couldn't be found on the card, and we are in
				// stack override mode, then search the stack. We do this *after*
				// searching the card as searching the stack will take longer.
				if (gptr == nil && t_stack_override && ep2.ston() == ES_NORMAL)
				{
					t_stack_override = false;
					objptr = (MCGroup *)sptr->getcontrolid(toptr->otype, ep2.getuint4(), true);
				}
			}
			else
				objptr = bptr->getchild(toptr->etype, ep2.getsvalue(), toptr->otype, toptr->ptype);
		}
		else
		{
			MCeerror->add(EE_CHUNK_BADOBJECTEXP, line, pos);
			return ES_ERROR;
		}

		toptr = toptr->next;

		// If no object resolves there's nothing more to do.
		if (objptr == NULL)
			break;

		// If there is another object clause, and it isn't a group, it must be
		// an error (object clauses are chaing right to left, so as soon as a
		// non-group has been encountered we can't do any more).
		if (toptr != NULL && objptr -> gettype() != CT_GROUP)
		{
			MCeerror -> add(EE_CHUNK_NOBACKGROUND, line, pos);
			return ES_ERROR;
		}

		// We must have a group, so change the group pointer that the next
		// clause will be looked up within.
		gptr = (MCGroup *)objptr;
	}

	// This clause handles the case of 'control id ...' where there is no card
	// reference. It enables access to top-level objects in the stack via id.
	if (objptr == NULL && thiscard)
		if (ct_class(object->etype) == CT_ID)
		{
			if (ep2.ton() == ES_NORMAL)
			{
				uint4 tofindid = ep2.getuint4();
				objptr = sptr->getcontrolid(object->otype, tofindid, false);
			}
		}

	if (objptr == NULL)
	{
		MCeerror->add(EE_CHUNK_NOOBJECT, line, pos, ep2.getsvalue());
		return ES_ERROR;
	}

	return ES_NORMAL;
}

Exec_stat MCChunk::extents(MCCRef *ref, int4 &start, int4 &number,
                           MCExecPoint &ep, const char *sptr, const char *eptr,
                           int4 (*count)(MCExecPoint &ep, const char *sptr,
                                         const char *eptr))
{
	int4 nchunks = -1;
	int4 tn;
	MCExecPoint ep2(ep);
	number = 1;
	switch (ref->etype)
	{
	case CT_ANY:
		start = MCU_any((*count)(ep, sptr, eptr));
		break;
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
		start = ref->etype - CT_FIRST;
		break;
	case CT_LAST:
		start = ((*count)(ep, sptr, eptr)) - 1;
		break;
	case CT_MIDDLE:
		start = ((*count)(ep, sptr, eptr)) / 2;
		break;
	case CT_RANGE:
		if (ref->startpos->eval(ep2) != ES_NORMAL || ep2.ton() != ES_NORMAL)
		{
			MCeerror->add(EE_CHUNK_BADRANGESTART, line, pos);
			return ES_ERROR;
		}
		start = ep2.getint4();
		if (start < 0)
		{
			nchunks = (*count)(ep, sptr, eptr);
			start += nchunks;
		}
		else
			start--;
		if (ref->endpos->eval(ep2) != ES_NORMAL || ep2.ton() != ES_NORMAL)
		{
			MCeerror->add(EE_CHUNK_BADRANGEEND, line, pos);
			return ES_ERROR;
		}
		tn = ep2.getint4();
		if (tn < 0)
		{
			if (nchunks == -1)
				nchunks = (*count)(ep, sptr, eptr);
			tn += nchunks + 1;
		}
		number = tn - start;
		break;
	case CT_EXPRESSION:
		if (ref->startpos->eval(ep2) != ES_NORMAL || ep2.ton() != ES_NORMAL)
		{
			MCeerror->add(EE_CHUNK_BADEXPRESSION, line, pos);
			return ES_ERROR;
		}
		start = ep2.getint4();
		if (start < 0)
			start += (*count)(ep, sptr, eptr);
		else
			start--;
		break;
	default:
        // SN-2014-12-15: [[ Bug 14211 ]] Fix for using next with a text chunk.
        //  That was causing the extents to return an uninitialised value.
		fprintf(stderr, "MCChunk: ERROR bad extents\n");
        MCeerror->add(EE_CHUNK_BADEXTENTS, line, pos);
        return ES_ERROR;

	}
	if (start < 0)
	{
		number += start;
		start = 0;
	}
	if (number < 0)
		number = 0;
	return ES_NORMAL;
}

static int4 countlines(MCExecPoint &ep, const char *sptr, const char *eptr)
{
	int4 clines = 1;
	if (sptr < eptr)
		do
		{
			if (*sptr == ep.getlinedel() && sptr + 1 < eptr)
				clines++;
		}
		while (++sptr < eptr);
	return clines;
}

static int4 countitems(MCExecPoint &ep, const char *sptr, const char *eptr)
{
	int4 items = 1;
	if (sptr < eptr)
		do
		{
			if (*sptr == ep.getitemdel() && sptr + 1 < eptr)
				items++;
		}
		while (++sptr < eptr);
	return items;
}

static int4 countwords(MCExecPoint &ep, const char *sptr, const char *eptr)
{
	int4 words = 0;
	if (sptr < eptr)
		do
		{
			if (!isspace((uint1)*sptr))
			{
				words++;
				if (*sptr == '"')
				{
					sptr++;
					while (sptr < eptr && *sptr != '"' && *sptr != ep.getlinedel())
						sptr++;
				}
				else
					while (sptr < eptr && !isspace((uint1)*sptr))
						sptr++;
			}
		}
		while (++sptr < eptr);
	return words;
}

static int4 counttokens(MCExecPoint &ep, const char *sptr, const char *eptr)
{
	int4 tokens = 0;
	MCString s(sptr, eptr - sptr);
	MCScriptPoint sp(s);
	Parse_stat ps = sp.nexttoken();
	while (ps != PS_ERROR && ps != PS_EOF)
	{
		tokens++;
		ps = sp.nexttoken();
	}
	return tokens;
}

static int4 countchars(MCExecPoint &ep, const char *sptr, const char *eptr)
{
	return eptr - sptr;
}

static void skip_word(const char *&sptr, const char *&eptr)
{
	if (*sptr == '"')
	{
		sptr++;
		while (sptr < eptr && *sptr != '"' && *sptr != '\n')
			sptr++;
		if (sptr < eptr && *sptr == '"')
			sptr++;
	}
	else
		while (sptr < eptr && !isspace((uint1)*sptr))
			sptr++;
}

// MW-2012-02-23: [[ FieldChars ]] Added the 'includechars' flag, if true any char chunk
//   will be processed; otherwise it will be ignored.
Exec_stat MCChunk::mark(MCExecPoint &ep, int4 &start, int4 &end, Boolean force, Boolean wholechunk, bool includechars)
{
	start = 0;
	end = ep.getsvalue().getlength();
	const char *startptr = ep.getsvalue().getstring();
	const char *sptr = startptr;
	const char *eptr = sptr + ep.getsvalue().getlength();
	int4 s, n;

	if (cline != NULL)
	{
		if (extents(cline, s, n, ep, sptr, eptr, countlines) != ES_NORMAL)
		{
			MCeerror->add(EE_CHUNK_BADLINEMARK, line, pos);
			return ES_ERROR;
		}
		if (n == 0)
		{
			eptr = sptr;
			end = start;
		}
		else
		{
			uint4 add = 0;
			while (s--)
			{
				while (sptr < eptr && *sptr++ != ep.getlinedel())
					;
				if (sptr == eptr && !(s == 0 && sptr > startptr && *(sptr - 1) == ep.getlinedel()))
					add++;
			}
			start = sptr - startptr;
			while (sptr < eptr && n--)
			{
				while (sptr < eptr && *sptr != ep.getlinedel())
					sptr++;
				if (sptr < eptr && n)
					sptr++;
			}
			end = sptr - startptr;
			if (wholechunk && item == NULL && word == NULL && character == NULL)
			{
				if (sptr < eptr)
					end++;
				else if (start > 0 && !add)
					start--;
				return ES_NORMAL;
			}
			if (force && add)
			{
				ep.fill(start, ep.getlinedel(), add);
				start += add;
				end += add;
				startptr = ep.getsvalue().getstring();
			}
			sptr = startptr + start;
			eptr = startptr + end;
		}
	}

	if (item != NULL)
	{
		int4 ostart = start;
		uint4 add	= 0;
		if (extents(item, s, n, ep, sptr, eptr, countitems) != ES_NORMAL)
		{
			MCeerror->add(EE_CHUNK_BADITEMMARK, line, pos);
			return ES_ERROR;
		}
		while (s--)
		{
			while (sptr < eptr && *sptr++ != ep.getitemdel())
				;
			if (sptr == eptr
			        && !(s == 0 && sptr > startptr && *(sptr - 1) == ep.getitemdel()))
				add++;
		}
		start = sptr - startptr;
		if (n == 0)
		{
			eptr = sptr;
			end = start;
		}
		else
		{
			while (sptr < eptr && n--)
			{
				while (sptr < eptr && *sptr != ep.getitemdel())
					sptr++;
				if (sptr < eptr && n)
					sptr++;
			}
			end = sptr - startptr;
			if (wholechunk && word == NULL && character == NULL)
			{
				if (startptr + end < eptr)
					end++;
				else if (start > ostart && !add)
					start--;
				return ES_NORMAL;
			}
			if (force && add)
			{
				char id = ep.getitemdel();
				ep.fill(start, id, add);
				start += add;
				end += add;
				startptr = ep.getsvalue().getstring();
			}
			sptr = startptr + start;
			eptr = startptr + end;
		}
	}
	if (word != NULL)
	{
		const char *osptr = sptr;
		if (extents(word, s, n, ep, sptr, eptr, countwords) != ES_NORMAL)
		{
			MCeerror->add(EE_CHUNK_BADWORDMARK, line, pos);
			return ES_ERROR;
		}
		while (sptr < eptr && isspace((uint1)*sptr))
			sptr++;
		while (sptr < eptr &&  s--)
		{
			skip_word(sptr, eptr);
			while (sptr < eptr && isspace((uint1)*sptr))
				sptr++;
		}
		start = sptr - startptr;
		if (n == 0)
		{
			eptr = sptr;
			end = start;
		}
		else
		{
			while (sptr < eptr && n--)
			{
				skip_word(sptr, eptr);
				if (n)
					while (sptr < eptr && isspace((uint1)*sptr))
						sptr++;
			}
			if (wholechunk && character == NULL)
			{
				while (sptr < eptr && isspace((uint1)*sptr))
					sptr++;
				end = sptr - startptr;
				if (sptr == eptr)
				{
					const char *tsptr = startptr + start - 1;
					while (tsptr > osptr && isspace((uint1)*tsptr))
						tsptr--;
					start = tsptr - startptr + 1;
				}
				return ES_NORMAL;
			}
			osptr += start;
			while (sptr > osptr && isspace((uint1)*(sptr - 1)))
				sptr--;
			end = sptr - startptr;
			sptr = startptr + start;
			eptr = startptr + end;
		}
	}

	if (token != NULL)
	{
		if (extents(token, s, n, ep, sptr, eptr, counttokens) != ES_NORMAL)
		{
			MCeerror->add
			(EE_CHUNK_BADTOKENMARK, line, pos);
			return ES_ERROR;
		}
		uint4 offset = sptr - startptr;
		MCString string(sptr, eptr - sptr);
		MCScriptPoint sp(string);
		MCerrorlock++;

		Parse_stat ps = sp.nexttoken();
		while (s-- && ps != PS_ERROR && ps != PS_EOF)
			ps = sp.nexttoken();
		start = sp.gettoken().getstring() - sp.getscript() + offset;
		while (--n && ps != PS_ERROR && ps != PS_EOF)
			ps = sp.nexttoken();
		end = sp.gettoken().getstring() + sp.gettoken().getlength()
		      - sp.getscript() + offset;
		MCerrorlock--;
		sptr = startptr + start;
		eptr = startptr + end;
	}

	// MW-2012-02-23: [[ FieldChars ]] If we want to compute the final char chunk
	//   portion and have a char chunk, process it.
	if (character != NULL && includechars)
	{
		if (extents(character, s, n, ep, sptr, eptr, countchars) != ES_NORMAL)
		{
			MCeerror->add(EE_CHUNK_BADCHARMARK, line, pos);
			return ES_ERROR;
		}
		if (start + s < end)
			start += s;
		else
			start = end;
		end = MCU_min(end, start + n);
	}
	return ES_NORMAL;
}

Exec_stat MCChunk::gets(MCExecPoint &ep)
{
	int4 start, end;

	if (mark(ep, start, end, False, False) != ES_NORMAL)
	{
		MCeerror->add(EE_CHUNK_CANTMARK, line, pos);
		return ES_ERROR;
	}
	ep.substring(start, end);
	return ES_NORMAL;
}

Exec_stat MCChunk::eval(MCExecPoint &ep)
{
	if (source != NULL && url == NULL && stack == NULL && background == NULL && card == NULL
	        && group == NULL && object == NULL)
	{
		if (desttype != DT_OWNER)
		{
			if (source->eval(ep) != ES_NORMAL)
			{
				MCeerror->add(EE_CHUNK_CANTGETSOURCE, line, pos);
				return ES_ERROR;
			}
		}
		else
		{
			// MW-2008-11-05: [[ Owner Reference ]] This case handles the syntax:
			//     <text chunk> of the owner of ...
			//   In this case we evaluate the owner property of the resolved object.
			MCObject *t_object;
			uint4 t_part;
			MCExecPoint ep2(ep);
			if (static_cast<MCChunk *>(source) -> getobj(ep, t_object, t_part, True) != ES_NORMAL)
			{
				MCeerror -> add(EE_CHUNK_BADOBJECTEXP, line, pos);
				return ES_ERROR;
			}
			
			if (t_object -> gettype() == CT_STACK && MCdispatcher -> ismainstack(static_cast<MCStack *>(t_object)))
				ep . clear();
			else if (t_object -> getparent() -> getprop(t_part, P_OWNER, ep, False) != ES_NORMAL)
				return ES_ERROR;
		}
	}
	else if (destvar != NULL)
	{
		if (destvar->eval(ep) != ES_NORMAL)
		{
			MCeerror->add
			(EE_CHUNK_CANTGETDEST, line, pos);
			return ES_ERROR;
		}
	}
	else
	{
		if (url != NULL)
		{
			if (url->startpos == NULL || url->startpos->eval(ep) != ES_NORMAL)
			{
				MCeerror->add
				(EE_CHUNK_CANTGETDEST, line, pos);
				return ES_ERROR;
			}
			MCU_geturl(ep);
		}
		MCObject *objptr;
		uint4 parid;
		if (getobj(ep, objptr, parid, True) != ES_NORMAL)
		{
			if (url == NULL
					|| stack != NULL || background != NULL || card != NULL
					|| group != NULL || object != NULL)
			{
				MCeerror->add
				(EE_CHUNK_CANTFINDOBJECT, line, pos);
				return ES_ERROR;
			}
		}
		else
		{
			switch (objptr->gettype())
			{
			case CT_BUTTON:
			case CT_IMAGE:
			case CT_AUDIO_CLIP:
			case CT_VIDEO_CLIP:
				objptr->getprop(parid, P_TEXT, ep, False);
				break;
			case CT_FIELD:
				{
					MCField *fptr = (MCField *)objptr;
					int4 start, end;
					switch (function)
					{
					case F_CLICK_CHUNK:
					case F_CLICK_CHAR_CHUNK:
					case F_CLICK_LINE:
					case F_CLICK_TEXT:
					case F_SELECTED_CHUNK:
					case F_SELECTED_LINE:
					case F_SELECTED_TEXT:
					case F_FOUND_CHUNK:
					case F_FOUND_LINE:
					case F_FOUND_TEXT:
					case F_MOUSE_CHUNK:
					case F_MOUSE_LINE:
					case F_MOUSE_CHAR_CHUNK:
					case F_MOUSE_TEXT:
						// MW-2012-12-13: [[ Bug 10592 ]] We are eval'ing so don't want the whole
						//   chunk in this case.
						fmark(fptr, start, end, False);
						fptr->returntext(ep, start, end);
						break;
					default:
						// MW-2012-01-27: [[ UnicodeChunks ]] Defer to the 'fieldmark' routine
						//   to fetch the string - notice we set keeptext to True as we want the
						//   real content.
						return fieldmark(ep, fptr, parid, start, end, False, False, True);
					}
				}
				break;
			default:
				MCeerror->add(EE_CHUNK_OBJECTNOTCONTAINER, line, pos);
				return ES_ERROR;
			}
		}
	}
	if (cline != NULL || item != NULL || word != NULL || token != NULL
	        || character != NULL)
	{
		// MW-2007-11-28: [[ Bug 5610 ]] If we have an array, force a conversion to string (empty)
		//   for backwards compatibility.
		if (ep . getformat() == VF_ARRAY)
			ep . clear();
		if (ep.tos() != ES_NORMAL || gets(ep) != ES_NORMAL)
		{
			MCeerror->add(EE_CHUNK_CANTGETSUBSTRING, line, pos, ep.getsvalue());
			return ES_ERROR;
		}
	}
	return ES_NORMAL;
}

Exec_stat MCChunk::set(MCExecPoint &ep, Preposition_type ptype)
{
	MCObject *objptr = NULL;
	MCField *fptr = NULL;
	uint4 parid;
	int4 start, end;
	MCExecPoint ep2(ep);
	char *desturl = NULL;

	if (destvar != NULL)
		destvar->clearuql();
	if (destvar == NULL && url == NULL)
	{
		if (getobj(ep2, objptr, parid, True) != ES_NORMAL)
		{
			MCeerror->add(EE_CHUNK_SETCANTGETBOJECT, line, pos);
			return ES_ERROR;
		}
		switch (objptr->gettype())
		{
		case CT_BUTTON:
		case CT_IMAGE:
		case CT_AUDIO_CLIP:
		case CT_VIDEO_CLIP:
			break;
		case CT_FIELD:
			fptr = (MCField *)objptr;
			break;
		default:
			MCeerror->add(EE_CHUNK_SETNOTACONTAINER, line, pos);
			return ES_ERROR;
		}
	}

	if (cline == NULL && item == NULL && word == NULL
	        && token == NULL && character == NULL
	        && desttype != DT_FUNCTION && ptype == PT_INTO)
	{
		if (destvar != NULL)
		{
			if (destvar->set(ep) != ES_NORMAL)
			{
				MCeerror->add(EE_CHUNK_CANTSETDEST, line, pos);
				return ES_ERROR;
			}
		}
		else
		{
			if (url != NULL)
			{
				if (url->startpos->eval(ep2) != ES_NORMAL)
				{
					MCeerror->add(EE_CHUNK_BADEXPRESSION, line, pos);
					return ES_ERROR;
				}
				MCU_puturl(ep2, ep);
			}
			else
				if (ep.tos() != ES_NORMAL
				        || objptr->setprop(parid, P_TEXT, ep, False) != ES_NORMAL)
				{
					MCeerror->add(EE_CHUNK_CANTSETDEST, line, pos);
					return ES_ERROR;
				}
		}
		return ES_NORMAL;
	}

	// MW-2007-11-28: [[ Bug 5610 ]] If we have an array, force a conversion to string (empty)
	//   for backwards compatibility.
	if (ep.getformat() == VF_ARRAY)
		ep . clear();
	else if (ep.tos() != ES_NORMAL)
	{
		MCeerror->add(EE_CHUNK_CANTSETDEST, line, pos);
		return ES_ERROR;
	}

	if (destvar != NULL)
	{
		if (cline == NULL && item == NULL && word == NULL
		        && token == NULL && character == NULL && ptype == PT_AFTER)
		{
			destvar->set(ep, True);
			return ES_NORMAL;
		}
		if (destvar->eval(ep2) != ES_NORMAL)
		{
			MCeerror->add(EE_CHUNK_SETCANTGETDEST, line, pos);
			return ES_ERROR;
		}
		uint4 oldlength = ep2.getsvalue().getlength();
		if (mark(ep2, start, end, True, False) != ES_NORMAL)
		{
			MCeerror->add(EE_CHUNK_CANTMARK, line, pos);
			return ES_ERROR;
		}
		//ONLY DO optimization 586 if it is into not before or after
		// SMR 1941

		// MW-2005-03-10: Fix bug 2471. Only do this optimisation if the destination is *ONLY* a string
		// MW-2012-03-15: [[ Bug ]] Only take the shortcut if the varible is not env, not msg and there
		//   are no watched vars.
		if (destvar -> getisplain() && ep2.getformat() == VF_STRING && end - start == ep.getsvalue().getlength() && ptype == PT_INTO
		        && ep2.getsvalue().getlength() == oldlength && MCnwatchedvars == 0)
		{
			memcpy((char *)ep2.getsvalue().getstring() + start, ep.getsvalue().getstring(),
			       ep.getsvalue().getlength());
			return ES_NORMAL;
		}
	}
	else
	{
		if (url == NULL && objptr->gettype() == CT_FIELD)
		{
			if (fieldmark(ep2, fptr, parid, start, end, False, True) != ES_NORMAL)
				return ES_ERROR;
		}
		else
		{
			if (url != NULL)
			{
				if (url->startpos->eval(ep2) != ES_NORMAL)
				{
					MCeerror->add(EE_CHUNK_BADEXPRESSION, line, pos);
					return ES_ERROR;
				}
				desturl = ep2.getsvalue().clone();
				MCU_geturl(ep2);
			}
			else
				objptr->getprop(parid, P_TEXT, ep2, False);
			if (mark(ep2, start, end, True, False) != ES_NORMAL)
			{
				MCeerror->add(EE_CHUNK_CANTMARK, line, pos);
				return ES_ERROR;
			}
		}
	}
	switch (ptype)
	{
	case PT_BEFORE:
		if (fptr == NULL)
			ep2.insert(ep.getsvalue(), start, start);
		else
			end = start;
		break;
	case PT_INTO:
		if (fptr == NULL)
			ep2.insert(ep.getsvalue(), start, end);
		break;
	case PT_AFTER:
		if (fptr == NULL)
			ep2.insert(ep.getsvalue(), end, end);
		else
			start = end;
		break;
	default:
		fprintf(stderr, "MCChunk: ERROR bad prep in gets\n");
		break;
	}
	if (destvar != NULL)
	{
		if (destvar->set(ep2) != ES_NORMAL)
		{
			MCeerror->add(EE_CHUNK_CANTSETDEST, line, pos);
			return ES_ERROR;
		}
	}
	else
		if (url != NULL)
		{
			ep.setsvalue(desturl);
			MCU_puturl(ep, ep2);
			delete desturl;
		}
		else
			if (fptr != NULL)
			{
				if (fptr->settextindex(parid, start, end, ep.getsvalue(), False) != ES_NORMAL)
				{
					MCeerror->add(EE_CHUNK_CANTSETDEST, line, pos);
					return ES_ERROR;
				}
			}
			else
				if (objptr->setprop(parid, P_TEXT, ep2, False) != ES_NORMAL)
				{
					MCeerror->add(EE_CHUNK_CANTSETDEST, line, pos);
					return ES_ERROR;
				}
	return ES_NORMAL;
}

Exec_stat MCChunk::setunicode(MCExecPoint& ep, Preposition_type p_prep)
{
	if (destvar == nil && url == nil)
	{
		MCObject *objptr;
		uint32_t parid;
		MCExecPoint ep2(ep);
		if (getobj(ep2, objptr, parid, True) != ES_NORMAL)
		{
			MCeerror -> add(EE_CHUNK_SETCANTGETBOJECT, line, pos);
			return ES_ERROR;
		}
		
		if (ep . tos() != ES_NORMAL)
		{
			MCeerror->add(EE_CHUNK_CANTSETDEST, line, pos);
			return ES_ERROR;
		}
		
		if (objptr -> gettype() == CT_FIELD)
		{
			MCField *t_field;
			t_field = (MCField *)objptr;
			
			if (cline == NULL && item == NULL && word == NULL
				&& token == NULL && character == NULL
				&& desttype != DT_FUNCTION && p_prep == PT_INTO)
			{
				if (ep.tos() != ES_NORMAL
				        || objptr->setprop(parid, P_UNICODE_TEXT, ep, False) != ES_NORMAL)
				{
					MCeerror->add(EE_CHUNK_CANTSETDEST, line, pos);
					return ES_ERROR;
				}
				
				return ES_NORMAL;
			}
			
			int32_t t_start, t_end;
			if (fieldmark(ep2, t_field, parid, t_start, t_end, False, True) != ES_NORMAL)
				return ES_ERROR;
			
			switch(p_prep)
			{
			case PT_BEFORE:
				t_end = t_start;
				break;
			default:
			case PT_INTO:
				break;
			case PT_AFTER:
				t_start = t_end;
				break;
			}
			
			if (t_field -> settextindex(parid, t_start, t_end, ep . getsvalue(), False, true) != ES_NORMAL)
			{
				MCeerror -> add(EE_CHUNK_CANTSETDEST, line, pos);
				return ES_ERROR;
			}
			
			return ES_NORMAL;
		}
	}
	
	MCeerror -> add(EE_CHUNK_CANTSETUNICODEDEST, line, pos);
	return ES_ERROR;
}

Exec_stat MCChunk::count(Chunk_term tocount, Chunk_term ptype,
                         MCExecPoint &ep)
{
	// MW-2009-07-22: First non-control chunk is now CT_ELEMENT.
	if (tocount < CT_ELEMENT)
	{
		uint2 i = 0;
		MCObject *optr;
		uint4 parid;
		if (getobj(ep, optr, parid, True) != ES_NORMAL)
		{
			if (stack != NULL)
				return ES_ERROR;
			optr = MCdefaultstackptr;
		}
		if (tocount == CT_MARKED)
		{
			MCStack *sptr = optr->getstack();
			sptr->setmark();
			if (optr->gettype() == CT_GROUP)
			{
				MCGroup *gptr = (MCGroup *)optr;
				gptr->count(CT_CARD, NULL, i);
			}
			else
				sptr->count(CT_CARD, CT_UNDEFINED, NULL, i);
			sptr->clearmark();
		}
		else
			switch (optr->gettype())
			{
			case CT_STACK:
				{
					MCStack *sptr = (MCStack *)optr;
					sptr->count(tocount, ptype, NULL, i);
				}
				break;
			case CT_CARD:
				{
					MCCard *cptr = (MCCard *)optr;
					cptr->count(tocount, ptype, NULL, i, True);
				}
				break;
			case CT_GROUP:
				{
					MCGroup *gptr = (MCGroup *)optr;
					gptr->count(tocount, NULL, i);
					if (tocount == CT_LAYER || tocount == CT_GROUP)
						i--;
				}
				break;
			default:
				break;
			}
		ep.setnvalue(i);
	}
	else
	{
		uint4 i = 0;
		if (eval(ep) != ES_NORMAL)
			return ES_ERROR;

		// MW-2009-07-22: If the value of the chunk is an array, then either
		//   the count will be zero (if a string chunk has been requested),
		//   otherwise it will be the count of the keys...
		if (ep . getformat() != VF_ARRAY)
		{
			uint4 length = ep.getsvalue().getlength();
			if (ep.getsvalue() != MCnullmcstring)
			{
				const char *sptr = ep.getsvalue().getstring();
				const char *eptr = sptr + length;
				switch(tocount)
				{
				case CT_LINE:
					i = countlines(ep, sptr, eptr);
					break;
				case CT_ITEM:
					i = countitems(ep, sptr, eptr);
					break;
				case CT_WORD:
					i = countwords(ep, sptr, eptr);
					break;
				case CT_TOKEN:
					i = counttokens(ep, sptr, eptr);
					break;
				case CT_CHARACTER:
					i = length;
					break;
				default:
					break;
				}
			}
		}
		else
		{
			switch(tocount)
			{
			case CT_KEY:
			case CT_ELEMENT:
				i = ep . getarray() -> get_array() -> getnfilled();
				break;
			default:
				i = 0;
				break;
			}
		}

		ep.setnvalue(i);
	}
	return ES_NORMAL;
}

// MW-2012-12-13: [[ Bug 10592 ]] If wholechunk is False then we don't expand
//   line chunks to include the CR at the end.
Exec_stat MCChunk::fmark(MCField *fptr, int4 &start, int4 &end, Boolean wholechunk)
{
	Boolean wholeline = True;
	Boolean wholeword = True;
	switch (function)
	{
	case F_CLICK_CHAR_CHUNK:
		wholeword = False;
	case F_CLICK_CHUNK:
	case F_CLICK_TEXT:
		wholeline = False;
	case F_CLICK_LINE:
		if (!fptr->locmark(wholeline, wholeword, True, True, wholechunk, start, end))
			start = end = 0;
		break;
	case F_FOUND_CHUNK:
	case F_FOUND_TEXT:
		wholeline = False;
	case F_FOUND_LINE:
		if (!fptr->foundmark(wholeline, wholechunk, start, end))
			start = end = 0;
		break;
	case F_SELECTED_CHUNK:
	case F_SELECTED_TEXT:
		wholeline = False;
	case F_SELECTED_LINE:
        // MW-2014-05-28: [[ Bug 11928 ]] 'wholeline' is sufficient to determine whether the
            //  CR should be included.
		if (!fptr->selectedmark(wholeline, start, end, False))
			start = end = 0;
		break;
	case F_MOUSE_CHAR_CHUNK:
		wholeword = False;
	case F_MOUSE_CHUNK:
	case F_MOUSE_TEXT:
		wholeline = False;
	case F_MOUSE_LINE:
		if (!fptr->locmark(wholeline, wholeword, False, True, wholechunk, start, end))
			start = end = 0;
		break;
	default:
		start = 0;
		end = fptr->getpgsize(NULL);
		break;
	}
	
	return ES_NORMAL;
}

// MW-2012-02-23: [[ FieldChars ]] Computes the start and end of the char chunk in
//   the given field, taking into account the indices are given in char indices.
Exec_stat MCChunk::markcharactersinfield(uint32_t p_part_id, MCExecPoint& ep, int32_t& x_start, int32_t& x_end, MCField *p_field)
{
	MCExecPoint ep2(ep);
	int32_t t_number, t_start;
	t_start = 0;
	t_number = 1;
	switch(character -> etype)
	{
		case CT_ANY:
			t_start = MCU_any(p_field -> countchars(p_part_id, x_start, x_end));
			break;
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
			t_start = character -> etype - CT_FIRST;
			break;
		case CT_LAST:
			t_start = p_field -> countchars(p_part_id, x_start, x_end) - 1;
			break;
		case CT_MIDDLE:
			t_start = p_field -> countchars(p_part_id, x_start, x_end) / 2;
			break;
		case CT_RANGE:
		{
			int32_t t_count = -1, tn;
			if (character->startpos->eval(ep2) != ES_NORMAL || ep2.ton() != ES_NORMAL)
			{
				MCeerror->add(EE_CHUNK_BADRANGESTART, line, pos);
				return ES_ERROR;
			}
			t_start = ep2.getint4();
			if (t_start < 0)
			{
				t_count = p_field -> countchars(p_part_id, x_start, x_end);
				t_start += t_count;
			}
			else
				t_start--;
			
			if (character->endpos->eval(ep2) != ES_NORMAL || ep2.ton() != ES_NORMAL)
			{
				MCeerror->add(EE_CHUNK_BADRANGEEND, line, pos);
				return ES_ERROR;
			}
			tn = ep2.getint4();
			if (tn < 0)
			{
				if (t_count == -1)
					t_count = p_field -> countchars(p_part_id, x_start, x_end);
				tn += t_count + 1;
			}
			t_number = tn - t_start;
		}
		break;
		case CT_EXPRESSION:
			if (character->startpos->eval(ep2) != ES_NORMAL || ep2.ton() != ES_NORMAL)
			{
				MCeerror->add(EE_CHUNK_BADEXPRESSION, line, pos);
				return ES_ERROR;
			}
			t_start = ep2.getint4();
			if (t_start < 0)
				t_start += p_field -> countchars(p_part_id, x_start, x_end);
			else
				t_start--;
			break;
		default:
			assert(false);
			break;
	}
		
	if (t_start < 0)
	{
		t_number += t_start;
		t_start = 0;
	}
	
	if (t_number < 0)
		t_number = 0;
	
	p_field -> resolvechars(p_part_id, x_start, x_end, t_start, t_number);
	
	return ES_NORMAL;
}

// SMR, 1877, only expand contents when "force" is true (for set and setprop).
Exec_stat MCChunk::fieldmark(MCExecPoint &ep, MCField *fptr, uint4 parid,
                             int4 &start, int4 &end, Boolean wholechunk, Boolean force, Boolean keeptext)
{
	if (desttype == DT_FUNCTION)
	{
		// MW-2012-09-20: [[ Bug 10229 ]] If 'keeptext' is true, make sure we leave with
		//   the ep containing the field's content between start and end.
		// MW-2012-12-13: [[ Bug 10592 ]] Pass through wholechunk to ensure we only get the CR
		//   included when we need it.
		fmark(fptr, start, end, wholechunk);
		if (cline != NULL || item != NULL || word != NULL || token != NULL || character != NULL)
		{
			fptr->returntext(ep, start, end);
			int4 si, ei;
			if (mark(ep, si, ei, force, wholechunk, false) != ES_NORMAL)
			{
				MCeerror->add(EE_CHUNK_BADTEXT, line, pos);
				return ES_ERROR;
			}
			
			end = start + ei;
			start += si;
			
			if (character != nil)
				if (markcharactersinfield(parid, ep, start, end, fptr) != ES_NORMAL)
				{
					MCeerror->add(EE_CHUNK_BADTEXT, line, pos);
					return ES_ERROR;
				}
			
		}
		else if (keeptext)
			fptr -> returntext(ep, start, end);
	}
	else
	{
		// We can make do with ASCII content if the delimiters (if needed) are
		// ASCII chars.
		bool t_ascii_only;
		t_ascii_only = (cline == nil || (unsigned)ep . getlinedel() <= 127) && (item == nil || (unsigned)ep . getitemdel() <= 127);
		
		// Fetch the nativized text - the method returns true if it had to
		// coecre unicode.
		bool t_has_unicode;
		t_has_unicode = fptr->nativizetext(parid, ep, t_ascii_only);
		
		// Perform the 'mark' operation.
		MCString oldstring = ep.getsvalue();
		if (mark(ep, start, end, force, wholechunk, !t_has_unicode) != ES_NORMAL)
		{
			MCeerror->add(EE_CHUNK_BADTEXT, line, pos);
			return ES_ERROR;
		}
		
		// If the ep got changed, mutate the field appropriately.
		if (ep.getsvalue() != oldstring)
		{
			MCExecPoint ep2(ep);
			fptr->nativizetext(parid, ep2, t_ascii_only);
			int4 si = 0;
			int4 sei = ep.getsvalue().getlength() - 1;
			int4 oei = ep2.getsvalue().getlength() - 1;
			const char *sref = ep.getsvalue().getstring();
			const char *oref = ep2.getsvalue().getstring();
			while (si <= oei && sref[si] == oref[si])
				si++;
			while (oei >= si && sref[sei] == oref[oei])
			{
				sei--;
				oei--;
			}
			sei++;
			MCString newstring;
			newstring.set(&sref[si], sei - si);
			fptr->settextindex(parid, si, si, newstring, False);
		}
		
		if (t_has_unicode && character != nil)
			if (markcharactersinfield(parid, ep, start, end, fptr) != ES_NORMAL)
			{
				MCeerror->add(EE_CHUNK_BADTEXT, line, pos);
				return ES_ERROR;
			}
		
		// MW-2012-01-29: [[ Bug ]] If keeptext is true make sure we substring, remembering
		//   to refetch the text if unicode.
		if (keeptext)
		{
			// MW-2012-02-21: [[ FieldExport ]] If the buffer doesn't have native text in it
			//   (due to unicode being involved) then fetch the appropriate range as native.
			//   Otherwise, just trim the buffer.
			if (t_has_unicode)
				fptr -> exportastext(parid, ep, start, end, false);
			else
				ep . substring(start, end);
		}
	}
	return ES_NORMAL;
}

Exec_stat MCChunk::getobjforprop(MCExecPoint& ep, MCObject*& r_object, uint4& r_parid)
{
	MCObject *objptr;
	uint4 parid;

	if (getobj(ep, objptr, parid, True) != ES_NORMAL)
	{
		MCeerror->add(EE_CHUNK_CANTFINDOBJECT, line, pos);
		return ES_ERROR;
	}
	
	// MW-2013-06-20: [[ Bug 10966 ]] Use 'istextchunk()' to determine whether
	//   the chunk can be evaluated as an object.
	if (!istextchunk())
	{
		r_object = objptr;
		r_parid = parid;
		return ES_NORMAL;
	}

	MCeerror->add(EE_CHUNK_NOPROP, line, pos);
	return ES_ERROR;
}

// MW-2011-11-23: [[ Array Chunk Props ]] If index is not nil, then treat as an array chunk prop
Exec_stat MCChunk::getprop(Properties which, MCExecPoint &ep, MCNameRef index, Boolean effective)
{
	MCObject *objptr;
	uint4 parid;

	if (url != NULL)
	{
		if (url->startpos == NULL || url->startpos->eval(ep) != ES_NORMAL)
		{
			MCeerror->add(EE_CHUNK_CANTGETDEST, line, pos);
			return ES_ERROR;
		}
		MCU_geturl(ep);
	}
	if (getobj(ep, objptr, parid, True) != ES_NORMAL)
	{
		MCeerror->add(EE_CHUNK_CANTFINDOBJECT, line, pos);
		return ES_ERROR;
	}
	
	// MW-2013-06-20: [[ Bug 10966 ]] Use 'istextchunk()' to determine whether
	//   the chunk can be evaluated as an object.
	if (!istextchunk())
	{
		// MW-2011-11-23: [[ Array Chunk Props ]] If index is nil, then its just a normal
		//   prop, else its an array prop.
		Exec_stat t_stat;
		if (index == nil)
			t_stat = objptr->getprop(parid, which, ep, effective);
		else
			t_stat = objptr->getarrayprop(parid, which, ep, index, effective);

		if (t_stat != ES_NORMAL)
		{
			MCeerror->add(EE_CHUNK_NOPROP, line, pos);
			return ES_ERROR;
		}
	}
	else
	{
		int4 start, end;
		if (objptr->gettype() != CT_FIELD)
		{
			MCeerror->add(EE_CHUNK_BADCONTAINER, line, pos);
			return ES_ERROR;
		}
		MCField *fptr = (MCField *)objptr;
		if (fieldmark(ep, fptr, parid, start, end, False, False) != ES_NORMAL)
			return ES_ERROR;

		// MW-2011-11-23: [[ Array TextStyle ]] Pass the 'index' along to method to
		//   handle specific styles.
		if (fptr->gettextatts(parid, which, ep, index, effective, start, end, islinechunk()) != ES_NORMAL)
		{
			MCeerror->add(EE_CHUNK_CANTGETATTS, line, pos);
			return ES_ERROR;
		}
	}
	return ES_NORMAL;
}

// MW-2011-11-23: [[ Array Chunk Props ]] If index is not nil, then treat as an array chunk prop
Exec_stat MCChunk::setprop(Properties which, MCExecPoint &ep, MCNameRef index, Boolean effective)
{
	MCObject *objptr;
	uint4 parid;

	if (url != NULL)
	{
		if (url->startpos == NULL || url->startpos->eval(ep) != ES_NORMAL)
		{
			MCeerror->add(EE_CHUNK_CANTGETDEST, line, pos);
			return ES_ERROR;
		}
		MCU_geturl(ep);
	}
	if (getobj(ep, objptr, parid, True) != ES_NORMAL)
	{
		MCeerror->add(EE_CHUNK_CANTFINDOBJECT, line, pos);
		return ES_ERROR;
	}
	
	// MW-2013-06-20: [[ Bug 10966 ]] Use 'istextchunk()' to determine whether
	//   the chunk can be evaluated as an object.
	if (!istextchunk())
	{
		// MW-2011-11-23: [[ Array Chunk Props ]] If index is nil, then its just a normal
		//   prop, else its an array prop.
		Exec_stat t_stat;
		if (index == nil)
			t_stat = objptr->setprop(parid, which, ep, effective);
		else
			t_stat = objptr->setarrayprop(parid, which, ep, index, effective);

		if (t_stat != ES_NORMAL)
		{
			MCeerror->add(EE_CHUNK_NOPROP, line, pos);
			return ES_ERROR;
		}
	}
	else
		if (objptr->gettype() == CT_BUTTON)
		{
			Boolean value;
			if (!MCU_stob(ep.getsvalue(), value))
			{
				MCeerror->add(EE_OBJECT_NAB, 0, 0, ep.getsvalue());
				return ES_ERROR;
			}
			if (which == P_ENABLED)
			{
				which = P_DISABLED;
				value = !value;
			}
			return changeprop(ep, which, value);
		}
		else
		{
			if (objptr->gettype() != CT_FIELD)
			{
				MCeerror->add(EE_CHUNK_BADCONTAINER, line, pos);
				return ES_ERROR;
			}
			MCField *fptr = (MCField *)objptr;
			MCExecPoint ep2(ep);
			int4 start, end;
			if (fieldmark(ep2, fptr, parid, start, end, False, True) != ES_NORMAL)
				return ES_ERROR;

			// MW-2011-11-23: [[ Array TextStyle ]] Pass the 'index' along to method to
			//   handle specific styles.
			// MW-2011-12-08: [[ StyledText ]] Pass the ep, rather than the svalue of
			//   the ep.
			// MW-2012-01-25: [[ ParaStyles ]] Pass whether this was an explicit line chunk
			//   or not. This is used to disambiguate the setting of 'backColor'.
			if (fptr->settextatts(parid, which, ep, index, start, end, islinechunk()) != ES_NORMAL)
			{
				MCeerror->add(EE_CHUNK_CANTSETATTS, line, pos);
				return ES_ERROR;
			}
		}
	
	// MM-2012-09-05: [[ Property Listener ]] Make sure any listeners are updated of the property change.
	//  Handled at this point rather than MCProperty::set as here we know if it is a valid object set prop.
	objptr -> signallisteners(which);
	
	return ES_NORMAL;
}

Exec_stat MCChunk::select(MCExecPoint &ep, Preposition_type where, Boolean text, Boolean first)
{
	MCObject *objptr;
	uint4 parid;
	if (getobj(ep, objptr, parid, True) != ES_NORMAL)
	{
		MCeerror->add(EE_CHUNK_CANTFINDOBJECT, line, pos);
		return ES_ERROR;
	}
	if  (!objptr->getopened() && objptr->getid())
	{
		MCeerror->add(EE_CHUNK_NOTOPEN, line, pos);
		return ES_ERROR;
	}
	// MW-2013-06-20: [[ Bug 10966 ]] Use 'istextchunk()' to determine whether
	//   the chunk can be evaluated as an object.
	// MW-2013-06-26: [[ Bug 10986 ]] Make sure we only select the object if we aren't
	//   doing select before/after.
	if (!text && where == PT_AT && !istextchunk())
	{
		if (first)
			MCselected->clear(False);
		MCselected->add(objptr);
	}
	else
	{
		MCField *fptr = (MCField *)objptr;
		MCButton *bptr = (MCButton *)objptr;
		if (objptr->gettype() != CT_FIELD
		        && (objptr->gettype() != CT_BUTTON
		            || (fptr = bptr->getentry()) == NULL))
			if (objptr->gettype() == CT_BUTTON)
			{
				// MW-2012-02-16: [[ IntrinsicUnicode ]] For simplicity, always process the
				//   button text in utf-8.
				objptr->getprop(parid, P_UNICODE_TEXT, ep, False);

				ep . utf16toutf8();

				int4 start, end;
				if (mark(ep, start, end, True, True) != ES_NORMAL)
				{
					MCeerror->add(EE_CHUNK_CANTMARK, line, pos);
					return ES_ERROR;
				}
				int4 hiliteline = 1;
				const char *sptr = ep.getsvalue().getstring();
				if (sptr != NULL)
				{
					const char *eptr = sptr + start + 1;
					while (sptr < eptr)
						if (*sptr++ == '\n')
							hiliteline++;
					bptr->setmenuhistory(hiliteline);
				}
				return ES_NORMAL;
			}
			else
			{
				MCeerror->add(EE_CHUNK_BADCONTAINER, line, pos);
				return ES_ERROR;
			}
		int4 start = 0;
		int4 end = 0;
		if (text)
		{
			// MW-2012-02-20: We just need the length here, so use 'getpgsize()' instead.
			end = fptr -> getpgsize(nil);
		}
		else if (fieldmark(ep, fptr, 0, start, end, False, False) != ES_NORMAL)
			return ES_ERROR;
		switch (where)
		{
		case PT_AFTER:
			start = end;
			break;
		case PT_BEFORE:
			end = start;
		default:
			break;
		}
		return fptr->seltext(start, end, True);
	}
	return ES_NORMAL;
}

// This method returns true if the chunk represents a text chunk. A text chunk
// is considered to be one which:
//   - has a non-object function as base
//   - contains 'word', 'line', 'item', 'token' or 'character'.
// Its primary use is by the copy/cut/paste commands when determining if a set
// of targets is valid.
//
bool MCChunk::istextchunk(void) const
{
	Boolean tfunction;
	
	if (desttype == DT_FUNCTION && function != F_CLICK_FIELD
        && function != F_SELECTED_FIELD && function != F_FOUND_FIELD
        && function != F_MOUSE_CONTROL && function != F_FOCUSED_OBJECT
        && function != F_SELECTED_IMAGE
		&& function != F_DRAG_SOURCE && function != F_DRAG_DESTINATION)
		tfunction = True;
	else
		tfunction = False;
		
	if (!tfunction && cline == NULL && item == NULL
	        && token == NULL && word == NULL && character == NULL)
		return false;

	return true;
}

// MW-2012-01-25: [[ ParaStyles ]] Returns true if this is an explicit 'line' chunk.
// MW-2012-02-13: [[ Bug ]] Some function chunks should also be treated as lines.
bool MCChunk::islinechunk(void) const
{
	// If we have any of item, token, word or character entries then we are not a line.
	if (item != nil || token != nil || word != nil || character != nil)
		return false;

	// If there is a line entry, then we must be a line.
	if (cline != nil)
		return true;

	// Otherwise, we are only a line if we are of function type and it is one of the
	// 'line' functions.
	if (desttype == DT_FUNCTION && (function == F_SELECTED_LINE || function == F_FOUND_LINE || function == F_CLICK_LINE || function == F_MOUSE_LINE))
		return true;

	// We are not a line.
	return false;
}

// MW-2013-08-01: [[ Bug 10925 ]] Returns true if the chunk is just a var or indexed var.
bool MCChunk::isvarchunk(void) const
{
	if (source != nil)
		return false;
	
	if (cline != nil || item != nil || token != nil || word != nil || character != nil)
		return false;
	
	if (destvar != nil)
		return true;
	
	return false;
}

// This method works out the start and end points of the text chunk in the
// given field.
//
Exec_stat MCChunk::marktextchunk(MCExecPoint& ep, MCField*& r_field, uint4& r_part, uint4& r_start, uint4& r_end)
{
	MCObject *t_object;
	uint4 t_part;
	if (getobj(ep, t_object, t_part, True) != ES_NORMAL)
	{
		MCeerror -> add(EE_CHUNK_CANTFINDOBJECT, line, pos);
		return ES_ERROR;
	}

	if (t_object -> gettype() != CT_FIELD)
	{
		MCeerror->add(EE_CHUNK_BADCONTAINER, line, pos);
		return ES_ERROR;
	}

	int4 t_start, t_end;
	if (fieldmark(ep, static_cast<MCField *>(t_object), t_part, t_start, t_end, True, False) != ES_NORMAL)
		return ES_ERROR;

	r_field = static_cast<MCField *>(t_object);
	r_part = t_part;
	r_start = t_start;
	r_end = t_end;

	return ES_NORMAL;
}

Exec_stat MCChunk::del(MCExecPoint &ep)
{
	int4 start, end;
    // MW-2014-05-28: [[ Bug 11928 ]] If we are a transient text chunk then the fact that
    //   (one of) the char chunk refs are non-nil is a red-herring, we must re-evaluate
    //   destvar as a chunk.
	if (destvar != NULL
	        && !m_transient_text_chunk &&
                (cline != NULL || item != NULL || token != NULL || word != NULL || character != NULL))
	{
		if (destvar->eval(ep) != ES_NORMAL)
		{
			MCeerror->add(EE_CHUNK_SETCANTGETDEST, line, pos);
			return ES_ERROR;
		}
		if (mark(ep, start, end, True, True) != ES_NORMAL)
		{
			MCeerror->add(EE_CHUNK_CANTMARK, line, pos);
			return ES_ERROR;
		}
		ep.insert(MCnullmcstring, start, end);
		if (destvar->set(ep) != ES_NORMAL)
		{
			MCeerror->add(EE_CHUNK_CANTSETDEST, line, pos);
			return ES_ERROR;
		}
	}
	else
	{
		MCObject *objptr;
		uint4 parid;

		if (getobj(ep, objptr, parid, True) != ES_NORMAL)
		{
			MCeerror->add(EE_CHUNK_CANTFINDOBJECT, line, pos);
			return ES_ERROR;
		}
		
		// MW-2013-06-20: [[ Bug 10966 ]] Use 'istextchunk()' to determine whether
		//   the chunk can be evaluated as an object.
		if (!istextchunk())
		{
			if (!objptr->del())
			{
				MCeerror->add(EE_CHUNK_CANTDELETEOBJECT, line, pos);
				return ES_ERROR;
			}
			objptr->scheduledelete();
		}
		else
			if (objptr->gettype() == CT_BUTTON)
			{
				// MW-2012-02-16: [[ IntrinsicUnicode ]] For simplicity, always process the
				//   button text in utf-8.

				objptr->getprop(parid, P_UNICODE_TEXT, ep, False);

				ep . utf16toutf8();

				if (mark(ep, start, end, True, True) != ES_NORMAL)
				{
					MCeerror->add(EE_CHUNK_CANTMARK, line, pos);
					return ES_ERROR;
				}
				ep.insert(MCnullmcstring, start, end);

				ep . utf8toutf16();

				objptr->setprop(parid, P_UNICODE_TEXT, ep, False);
			}
			else
				if (objptr->gettype() == CT_FIELD)
				{
					MCField *fptr = (MCField *)objptr;
					if (fieldmark(ep, fptr, parid, start, end, True, False) != ES_NORMAL)
						return ES_ERROR;
					fptr->settextindex(parid, start, end, MCnullmcstring, False);
				}
				else
				{
					MCeerror->add(EE_CHUNK_BADCONTAINER, line, pos);
					return ES_ERROR;
				}
	}
	return ES_NORMAL;
}

Exec_stat MCChunk::changeprop(MCExecPoint &ep, Properties prop, Boolean value)
{
	MCObject *optr;
	uint4 parid;
	if (getobj(ep, optr, parid, True) != ES_NORMAL)
	{
		MCeerror->add(EE_DISABLE_NOOBJ, line, pos);
		return ES_ERROR;
	}
	if (cline == NULL && item == NULL && token == NULL
	        && word == NULL && character == NULL)
	{
		MCExecPoint ep(optr, NULL, NULL);
		ep.setboolean(value);
		return optr->setprop(parid, prop, ep, False);
	}

	// MW-2007-07-05: [[ Bug 2378 ]] Cannot use enable/disable, hilite/unhilite on
	//   unicode menus.
	if (optr->gettype() == CT_BUTTON)
	{
		// MW-2012-02-16: [[ IntrinsicUnicode ]] For simplicity, always process the
		//   button text in utf-8.

		optr->getprop(parid, P_UNICODE_TEXT, ep, False);

		ep . utf16toutf8();

		int4 start, end;
		if (mark(ep, start, end, True, False) != ES_NORMAL)
		{
			MCeerror->add(EE_CHUNK_CANTMARK, line, pos);
			return ES_ERROR;
		}

		bool t_changed;
		t_changed = false;
		if (prop == P_DISABLED)
			if (value)
			{
				if (ep.getsvalue().getstring()[start] != '(')
					ep.insert("(", start, start), t_changed = true;
			}
			else
			{
				if (ep.getsvalue().getstring()[start] == '(')
					ep.insert(MCnullmcstring, start, start + 1), t_changed = true;
			}
		else
		{
			if (ep.getsvalue().getstring()[start] == '(')
				start++;
			if (value)
			{
				if (ep.getsvalue().getstring()[start + 1] == 'n')
					ep.insert("c", start + 1, start + 2), t_changed = true;
				else
					if (ep.getsvalue().getstring()[start + 1] == 'u')
						ep.insert("r", start + 1, start + 2), t_changed = true;
			}
			else
			{
				if (ep.getsvalue().getstring()[start + 1] == 'c')
					ep.insert("n", start + 1, start + 2), t_changed = true;
				else
					if (ep.getsvalue().getstring()[start + 1] == 'r')
						ep.insert("u", start + 1, start + 2), t_changed = true;
			}
		}
		if (t_changed)
		{
			ep . utf8toutf16();
			if (optr->setprop(parid, P_UNICODE_TEXT, ep, False) != ES_NORMAL)
			{
				MCeerror->add(EE_CHUNK_CANTSETDEST, line, pos);
				return ES_ERROR;
			}
		}
	}
	else
	{
		MCeerror->add(EE_DISABLE_NOOBJ, line, pos);
		return ES_ERROR;
	}
	return ES_NORMAL;
}

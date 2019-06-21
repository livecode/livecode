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
#include "variable.h"

#include "globals.h"

#include "exec.h"

////////////////////////////////////////////////////////////////////////////////

static bool MCChunkTermIsNestable(Chunk_term p_chunk_term)
{
	switch (p_chunk_term)
	{
		case CT_STACK:
		case CT_GROUP:
		case CT_LAYER:
		case CT_ELEMENT:
			return true;
		default:
			return false;
	}
}

static bool MCChunkTermHasRange(Chunk_term p_chunk_term)
{
	return p_chunk_term >= CT_LINE && p_chunk_term < CT_ELEMENT;
}

////////////////////////////////////////////////////////////////////////////////

#ifdef COLLECTING_CHUNKS
#include "newchunk.h"
class MCSyntaxCollector
{
public:
	MCSyntaxCollector(MCScriptPoint& sp, Boolean doingthe, bool p_dest)
		: m_sp(sp), m_the(doingthe), m_dest(p_dest)
	{
		m_start = m_sp . getcurptr();
	}

	~MCSyntaxCollector(void)
	{
		m_end = m_sp . getcurptr();
		
		FILE *f;
		f = fopen("G:\\Scratch\\Chunks.txt", "a");
		if (f != NULL)
		{
			if (m_dest)
				fprintf(f, "C: ");
			else
				fprintf(f, "E: ");

			if (m_the)
				fprintf(f, "the ");

			for(int32_t i = 0; i < m_end - m_start; ++i)
			{
				if (m_start[i] == '\\')
				{
					fputc('\\', f);
					fputc('\\', f);
				}
				else if (m_start[i] == '\n')
				{
					fputc('\\', f);
					fputc('n', f);
				}
				else
					fputc(m_start[i], f);
			}
			fputc('\n', f);
			fclose(f);
		}
	}

private:
	MCScriptPoint& m_sp;
	bool m_dest;
	Boolean m_the;
	const uint1 *m_start;
	const uint1 *m_end;
};
#endif

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
    element = NULL;
	cline = item = word = token = character = NULL;
    paragraph = sentence = trueword = NULL;
    codepoint = codeunit = byte = NULL;
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
    while (element != NULL)
    {
		MCCRef *tptr = element;
		element = element->next;
		delete tptr;
    }
	delete cline;
	delete item;
	delete word;
	delete token;
	delete character;
    delete codepoint;
    delete codeunit;
    delete byte;
    delete paragraph;
    delete sentence;
    delete trueword;
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
		{
			if (need_target)
			{
				MCperror->add(PE_CHUNK_NOCHUNK, sp);
				return PS_ERROR;
			}
			else
				break;
		}
		if (type == ST_ID)
		{
			te = NULL;
			if (sp.lookup(SP_FACTOR, te) != PS_NORMAL || te->type == TT_PROPERTY)
			{
				if (need_target)
				{
					if (desttype == DT_ISDEST && noobjectchunks())
					{
						MCExpression *newfact = NULL;
						// MW-2011-06-22: [[ SERVER ]] Update to use SP findvar method to take into account
						//   execution outwith a handler.
						if (doingthe
						    || (sp.findvar(sp.gettoken_nameref(), &destvar) != PS_NORMAL
						        && (MCexplicitvariables
						            || sp.lookupconstant(&newfact) == PS_NORMAL
						            || sp.findnewvar(sp.gettoken_nameref(), kMCEmptyName, &destvar) != PS_NORMAL)))
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
				if (desttype != DT_ISDEST && !notextchunks() && noobjectchunks())
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
				if (curref != NULL && MCChunkTermHasRange(lterm))
				{
					if (curref->etype != CT_EXPRESSION)
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
					if ((lterm != CT_UNDEFINED && nterm > lterm)
					    || (nterm == lterm && !MCChunkTermIsNestable(nterm)))
					{
						MCperror->add(PE_CHUNK_BADORDER, sp);
						return PS_ERROR;
					}
					curref = new (nothrow) MCCRef;
					curref->otype = nterm;
					if (curref->otype == CT_BACKGROUND || curref->otype == CT_CARD)
					{
						Symbol_type itype;
						const LT *ite;
						if (sp.next(itype) == PS_NORMAL)
						{
							if (sp.lookup(SP_FACTOR, ite) == PS_NORMAL
							        && ite->type == TT_CHUNK
							        && ite->which >= CT_LAYER && ite->which <= CT_LAST_CONTROL)
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
						if (sp.parseexp(curref->otype <= CT_LAST_CONTROL, False,
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
                    case CT_WIDGET:
						object = curref;
						break;

                    case CT_ELEMENT:
                        // Allow a chain of element chunks
                        curref -> next = element;
                        element = curref;
                        break;
                            
					case CT_LINE:
						cline = curref;
                        break;
                    case CT_PARAGRAPH:
                        paragraph = curref;
                        break;
                    case CT_SENTENCE:
                        sentence = curref;
                        break;
					case CT_ITEM:
						item = curref;
						break;
					case CT_WORD:
						word = curref;
						break;
                    case CT_TRUEWORD:
                        trueword = curref;
                        break;
					case CT_TOKEN:
						token = curref;
						break;
					case CT_CHARACTER:
						character = curref;
						break;
                    case CT_CODEPOINT:
                        codepoint = curref;
                        break;
                    case CT_CODEUNIT:
                        codeunit = curref;
                        break;
                    case CT_BYTE:
                        byte = curref;
                        break;
					default: /* curref->otype */
						fprintf(stderr, "MCChunk: ERROR bad chunk type %d\n", nterm);
						break;
					}
                    // All text chunks and the element chunk need a target
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
                case F_DROP_CHUNK:
				case F_FOCUSED_OBJECT:
					if (desttype == DT_ISDEST)
					{ // delete the selectedLine of field x
						if (function == F_SELECTED_LINE || function == F_SELECTED_CHUNK)
						{}
					}
					else if (!notextchunks())
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
					source = new (nothrow) MCChunk(False);
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
					if (!noobjectchunks())
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
				if (!noobjectchunks())
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
    if (tchunk->byte != NULL)
	{
		delete byte;
		byte = tchunk->byte;
		tchunk->byte = NULL;
    }
    if (tchunk->codeunit != NULL)
	{
		delete codeunit;
		codeunit = tchunk->codeunit;
		tchunk->codeunit = NULL;
    }
    if (tchunk->codepoint != NULL)
	{
		delete codepoint;
		codepoint = tchunk->codepoint;
		tchunk->codepoint = NULL;
    }
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
    if (tchunk->trueword != NULL)
	{
		delete trueword;
		trueword = tchunk->trueword;
		tchunk->trueword = NULL;
    }
	if (tchunk->item != NULL)
	{
		delete item;
		item = tchunk->item;
		tchunk->item = NULL;
	}
    if (tchunk->sentence != NULL)
	{
		delete sentence;
		sentence = tchunk->sentence;
		tchunk->sentence = NULL;
    }
    if (tchunk->paragraph != NULL)
	{
		delete paragraph;
		paragraph = tchunk->paragraph;
		tchunk->paragraph = NULL;
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

bool MCChunk::getobj(MCExecContext& ctxt, MCObjectPtr& r_object, Boolean p_recurse)
{
    r_object . object = nil;
    
    getoptionalobj(ctxt, r_object, p_recurse);

    if (r_object . object == nil)
    {
        ctxt . Throw();
        return false;
    }

    return true;
}

void MCChunk::getoptionalobj(MCExecContext& ctxt, MCObject *&r_object, uint4& r_parid, Boolean p_recurse)
{
    MCObjectPtr t_obj_ptr;

    getoptionalobj(ctxt, t_obj_ptr, p_recurse);
    
    // ensure that the context does not contain an error until we decide how strict chunk parsing is.
    ctxt . IgnoreLastError();
    
    r_object = t_obj_ptr . object;
    r_parid = t_obj_ptr . part_id;
}

void MCChunk::getoptionalobj(MCExecContext& ctxt, MCObjectPtr &r_object, Boolean p_recurse)
{
    MCObjectPtr t_object;
    t_object . object = nil;
    t_object . part_id = 0;
    
    r_object . object = nil;
    r_object . part_id = 0;

    if (desttype != DT_UNDEFINED && desttype != DT_ISDEST)
    {
        if (desttype == DT_EXPRESSION || desttype == DT_VARIABLE)
        {
            bool t_error = true;
            MCAutoStringRef t_value;
            if (p_recurse)
            {
                if (desttype == DT_EXPRESSION)
                    ctxt . EvalExprAsStringRef(source, EE_CHUNK_BADOBJECTEXP, &t_value);
                else
                    ctxt . EvalExprAsStringRef(destvar, EE_CHUNK_BADOBJECTEXP, &t_value);

                if (ctxt . HasError())
                    return;
/*
                MCAutoValueRef t_value;
                ep . copyasvalueref(&t_value);
                MCEngineEvalValueAsObject(ctxt, *t_value, t_object);
*/

                // SN-2014-04-08 [[ Bug 12147 ]] create button in group command fails 
                MCScriptPoint sp(ctxt, *t_value);
                MCChunk *tchunk = new (nothrow) MCChunk(False);
                MCerrorlock++;
                Symbol_type type;
                if (tchunk->parse(sp, False) == PS_NORMAL
                        && sp.next(type) == PS_EOF)
                    t_error = false;
                
                MCerrorlock--;
                
                if (!t_error)
                {
                    if (tchunk->getobj(ctxt, t_object, False))
                        take_components(tchunk);
                    else
                        t_error= true;
                }
                
                delete tchunk;
            }
            // SN-2013-11-7: If no recurse, there is no error to trigger
            // Otherwise, the caller gets a 'marred' context with an error
            else
                return;

            if (t_error)
            {
                ctxt . LegacyThrow(EE_CHUNK_BADOBJECTEXP, *t_value);
                return;
            }
        }
        else if (desttype == DT_OWNER)
        {
            // MW-2008-11-05: [[ Owner Reference ]] If the desttype is DT_OWNER it means that <source>
            //   must point to an MCChunk object (as that's how its parsed).
            //   In this case attempt to evaluate it and then step up one in the ownership chain. Note
            //   that we can't step higher than a mainstack, so we set objptr to NULL in this case.
            if (!static_cast<MCChunk *>(source) -> getobj(ctxt, t_object, True))
            {
                ctxt . LegacyThrow(EE_CHUNK_BADOBJECTEXP);
                return;
            }

            MCEngineEvalOwnerAsObject(ctxt, t_object, t_object);
        }
        else if (desttype >= DT_FIRST_OBJECT && desttype < DT_LAST_OBJECT)
        {
            MCEngineEvalTemplateAsObject(ctxt, desttype, t_object);
        }
        else
        {
            switch(desttype)
            {
                case DT_THIS_ME:
                    t_object . object = destobj;
                    t_object . part_id = 0;
                    break;
                case DT_ME:
                    //MCEngineEvalMeAsObject(ctxt, t_object);
                    if (ctxt . GetParentScript() == NULL)
                    {
                        t_object . object = destobj;
                        t_object . part_id = 0;
                    }
                    else
                        t_object = ctxt . GetObjectPtr();
                    break;
                case DT_MENU_OBJECT:
                    MCEngineEvalMenuObjectAsObject(ctxt, t_object);
                    break;
                case DT_TARGET:
                    MCEngineEvalTargetAsObject(ctxt, t_object);
                    break;
                case DT_ERROR:
                    MCEngineEvalErrorObjectAsObject(ctxt, t_object);
                    break;
                case DT_SELECTED:
                    MCInterfaceEvalSelectedObjectAsObject(ctxt, t_object);
                    break;
                case DT_TOP_STACK:
                    MCInterfaceEvalTopStackAsObject(ctxt, t_object);
                    break;
                case DT_CLICK_STACK:
                    MCInterfaceEvalClickStackAsObject(ctxt, t_object);
                    break;
                case DT_MOUSE_STACK:
                    MCInterfaceEvalMouseStackAsObject(ctxt, t_object);
                    break;
                case DT_FUNCTION:
                    switch (function)
                {
                    case F_CLICK_CHUNK:
                    case F_CLICK_CHAR_CHUNK:
                    case F_CLICK_FIELD:
                    case F_CLICK_LINE:
                    case F_CLICK_TEXT:
                        MCInterfaceEvalClickFieldAsObject(ctxt, t_object);
                        break;
                    case F_SELECTED_CHUNK:
                    case F_SELECTED_FIELD:
                    case F_SELECTED_LINE:
                    case F_SELECTED_TEXT:
                        MCInterfaceEvalSelectedFieldAsObject(ctxt, t_object);
                        break;
                    case F_SELECTED_IMAGE:
                        MCInterfaceEvalSelectedImageAsObject(ctxt, t_object);
                        break;
                    case F_FOUND_CHUNK:
                    case F_FOUND_FIELD:
                    case F_FOUND_LINE:
                    case F_FOUND_TEXT:
                        MCInterfaceEvalFoundFieldAsObject(ctxt, t_object);
                        break;
                    case F_MOUSE_CONTROL:
                    case F_MOUSE_CHUNK:
                    case F_MOUSE_CHAR_CHUNK:
                    case F_MOUSE_LINE:
                    case F_MOUSE_TEXT:
                        MCInterfaceEvalMouseControlAsObject(ctxt, t_object);
                        break;
                    case F_FOCUSED_OBJECT:
                        MCInterfaceEvalFocusedObjectAsObject(ctxt, t_object);
                        break;
                    case F_DRAG_SOURCE:
                        MCPasteboardEvalDragSourceAsObject(ctxt, t_object);
                        break;
                    case F_DRAG_DESTINATION:
                        MCPasteboardEvalDragDestinationAsObject(ctxt, t_object);
                        break;
                    case F_DROP_CHUNK:
                        MCPasteboardEvalDropChunkAsObject(ctxt, t_object);
                        break;
                    default:
                        break;
                }
                    break;
                default:
                    break;
            }
        }
        if (t_object . object == nil)
        {
            ctxt . LegacyThrow(EE_CHUNK_NOTARGET);
            return;
        }

		Chunk_term t_type = t_object . object -> gettype();
		if (MCChunkTermIsControl(t_type) && t_type != CT_GROUP)
        {
                MCCard *t_card;
                t_card = t_object . object -> getcard(t_object . part_id);
                if (t_card == nil)
                {
                    // Optional obj: this doesn't throw an error
                    //  but rather leave r_object . object set to nil
                    return;
                }

                t_object . part_id = t_card -> getid();
                r_object . object = t_object . object;
                r_object . part_id = t_object . part_id;
                return;
		}
    }
    else if (noobjectchunks())
    {
        // Optional obj: this doesn't throw an error
        //  but rather leave r_object . object set to nil
        return;
    }
    else if (url != nil)
    {
        if (stack -> etype == CT_EXPRESSION)
        {
            MCAutoStringRef t_data;
            if (url -> startpos == nil || !ctxt . EvalExprAsStringRef(url -> startpos, EE_CHUNK_BADSTACKEXP, &t_data))
                return;

            MCInterfaceEvalBinaryStackAsObject(ctxt, *t_data, t_object);
        }
    }
    else
    {
        MCInterfaceEvalDefaultStackAsObject(ctxt, t_object);
    }

    if (stack != nil)
    {
        MCInterfaceEvalStackOfObject(ctxt, t_object, t_object);
        switch (stack->etype)
        {
            case CT_EXPRESSION:
            {
                MCNewAutoNameRef t_expression;
                if (!ctxt . EvalExprAsNameRef(stack -> startpos, EE_CHUNK_BADSTACKEXP, &t_expression))
                    return;

                MCInterfaceEvalStackOfStackByName(ctxt, t_object, *t_expression, t_object);

            }
                break;
            case CT_ID:
            {
                uint4 t_id;
                if (!ctxt . EvalExprAsStrictUInt(stack -> startpos, EE_CHUNK_BADSTACKEXP, t_id))
                    return;

                MCInterfaceEvalStackOfStackById(ctxt, t_object, t_id, t_object);
            }
                break;
            case CT_THIS:
                break;
            default:
                ctxt . LegacyThrow(EE_CHUNK_BADSTACKEXP);
                return;
        }

        if (stack->next != NULL)
        {
            switch (stack->next->etype)
            {
                case CT_EXPRESSION:
                {
                    MCNewAutoNameRef t_expression;
                    if (!ctxt . EvalExprAsNameRef(stack -> next -> startpos, EE_CHUNK_BADSTACKEXP, &t_expression))
                        return;

                    MCInterfaceEvalSubstackOfStackByName(ctxt, t_object, *t_expression, t_object);
                }
                    break;
                case CT_ID:
                {
                    uint4 t_id;
                    if (!ctxt . EvalExprAsStrictUInt(stack -> next -> startpos, EE_CHUNK_BADSTACKEXP, t_id))
                        return;

                    MCInterfaceEvalSubstackOfStackById(ctxt, t_object, t_id, t_object);
                }
                    break;
                case CT_THIS:
                    break;
                default:
                    ctxt . LegacyThrow(EE_CHUNK_BADSTACKEXP);
                    return;
            }
        }
    }

    if (object != nil && (object->otype == CT_AUDIO_CLIP || object->otype == CT_VIDEO_CLIP))
    {
        MCInterfaceEvalStackOfObject(ctxt, t_object, t_object);
        switch (ct_class(object -> etype))
        {
            case CT_ORDINAL:
                if (object -> otype == CT_AUDIO_CLIP)
                    MCInterfaceEvalAudioClipOfStackByOrdinal(ctxt, t_object, object -> etype, t_object);
                else
                    MCInterfaceEvalVideoClipOfStackByOrdinal(ctxt, t_object, object -> etype, t_object);
                break;
            case CT_ID:
            {
                uint4 t_id;
                if (!ctxt . EvalExprAsStrictUInt(object -> startpos, EE_CHUNK_BADOBJECTEXP, t_id))
                    return;

                if (object -> otype == CT_AUDIO_CLIP)
                    MCInterfaceEvalAudioClipOfStackById(ctxt, t_object, t_id, t_object);
                else
                    MCInterfaceEvalVideoClipOfStackById(ctxt, t_object, t_id, t_object);
            }
                break;
            case CT_EXPRESSION:
            {
                MCNewAutoNameRef t_expression;
                if (!ctxt . EvalExprAsNameRef(object -> startpos, EE_CHUNK_BADOBJECTEXP, &t_expression))
                    return;

                if (object -> otype == CT_AUDIO_CLIP)
                    MCInterfaceEvalAudioClipOfStackByName(ctxt, t_object, *t_expression, t_object);
                else
                    MCInterfaceEvalVideoClipOfStackByName(ctxt, t_object, *t_expression, t_object);
            }
                break;
            default:
                ctxt . LegacyThrow(EE_CHUNK_BADOBJECTEXP);
                break;
        }
        if (!ctxt . HasError())
        {
            r_object . object = t_object . object;
            r_object . part_id = t_object . part_id;
        }

        return;
    }

    if (background != nil)
    {
        MCInterfaceEvalStackOfObject(ctxt, t_object, t_object);
        switch (ct_class(background->etype))
        {
            case CT_ORDINAL:
                if (card == nil)
                    MCInterfaceEvalBackgroundOfStackByOrdinal(ctxt, t_object, background -> etype, t_object);
                else
                    MCInterfaceEvalStackWithBackgroundByOrdinal(ctxt, t_object, background -> etype, t_object);
                break;
            case CT_ID:
            {
                uint4 t_id;
                if (!ctxt . EvalExprAsStrictUInt(background -> startpos, EE_CHUNK_BADBACKGROUNDEXP, t_id))
                    return;

                if (card == nil)
                    MCInterfaceEvalBackgroundOfStackById(ctxt, t_object, t_id, t_object);
                else
                    MCInterfaceEvalStackWithBackgroundById(ctxt, t_object, t_id, t_object);
            }
                break;
            case CT_EXPRESSION:
            {
                MCNewAutoNameRef t_expression;
                if (!ctxt . EvalExprAsNameRef(background -> startpos, EE_CHUNK_BADBACKGROUNDEXP, &t_expression))
                    return;

                if (card == nil)
                    MCInterfaceEvalBackgroundOfStackByName(ctxt, t_object, *t_expression, t_object);
                else
                    MCInterfaceEvalStackWithBackgroundByName(ctxt, t_object, *t_expression, t_object);
            }
                break;
            default:
                ctxt . LegacyThrow(EE_CHUNK_BADBACKGROUNDEXP);
                return;
        }
    }

    if (card != nil)
    {
        MCInterfaceEvalStackWithOptionalBackground(ctxt, t_object, t_object);
        switch (ct_class(card->etype))
        {
            case CT_DIRECT:
                // recent
                break;
            case CT_ORDINAL:
                if (background != nil)
                    MCInterfaceEvalCardOfBackgroundByOrdinal(ctxt, t_object, marked, card -> etype, t_object);
                else
                    MCInterfaceEvalCardOfStackByOrdinal(ctxt, t_object, marked, card -> etype, t_object);
                break;
            case CT_ID:
            {
                uint4 t_id;
                if (!ctxt . EvalExprAsStrictUInt(card -> startpos, EE_CHUNK_BADCARDEXP, t_id))
                    return;

                if (background != nil)
                    MCInterfaceEvalCardOfBackgroundById(ctxt, t_object, marked, t_id, t_object);
                else
                    MCInterfaceEvalCardOfStackById(ctxt, t_object, marked, t_id, t_object);
            }
                break;
            case CT_EXPRESSION:
            {
                MCNewAutoNameRef t_expression;
                if (!ctxt . EvalExprAsNameRef(card -> startpos, EE_CHUNK_BADCARDEXP, &t_expression))
                    return;

                if (background != nil)
                    MCInterfaceEvalCardOfBackgroundByName(ctxt, t_object, marked, *t_expression, t_object);
                else
                    MCInterfaceEvalCardOfStackByName(ctxt, t_object, marked, *t_expression, t_object);
            }
                break;
            default:
                ctxt . LegacyThrow(EE_CHUNK_BADCARDEXP);
                return;
        }
    }
    else if (group != nil || object != nil)
    {
        MCInterfaceEvalThisCardOfStack(ctxt, t_object, t_object);
        if (t_object.object == nullptr)
        {
            ctxt.LegacyThrow(EE_CHUNK_BADCARDEXP);
            return;
        }
    }

    // MW-2011-08-09: [[ Groups ]] If there was an explicit stack reference,
    //   but no explicit card, we search the stack directly for the CT_ID
    //   case.
    bool t_stack_override;
    t_stack_override = false;
    if (stack != nil && card == nil)
        t_stack_override = true;

    MCCRef *tgptr = group;
    while (tgptr != nil)
    {
        switch (ct_class(tgptr -> etype))
        {
            case CT_ORDINAL:
//              if (tgptr == group && (card != nil || background == nil))
                if (t_object . object -> gettype() == CT_CARD)
                    MCInterfaceEvalGroupOfCardByOrdinal(ctxt, t_object, tgptr -> ptype, tgptr -> etype, t_object);
                else
                    MCInterfaceEvalGroupOfGroupByOrdinal(ctxt, t_object, tgptr -> etype, t_object);
                break;
            case CT_ID:
            {
                uint4 t_id;
                if (!ctxt . EvalExprAsStrictUInt(tgptr -> startpos, EE_CHUNK_BADBACKGROUNDEXP, t_id))
                    return;

                // MW-2011-08-09: [[ Groups ]] If there was an explicit stack reference,
                //   but no explicit card, we search the stack directly for the CT_ID
                //   case.
                // (Assuming the group wasn't found on the card)
//              if (tgptr == group && (card != nil || background == nil))
                if (t_object . object -> gettype() == CT_CARD)
                {
                    if (t_stack_override)
                        MCInterfaceEvalGroupOfCardOrStackById(ctxt, t_object, tgptr -> ptype, t_id, t_object);
                    else
                        MCInterfaceEvalGroupOfCardById(ctxt, t_object, tgptr -> ptype, t_id, t_object);
                }
                else
                {
                    // if we have a group then stack override is irrelevant.
                    MCInterfaceEvalGroupOfGroupById(ctxt, t_object, t_id, t_object);
                }
            }
                break;
            case CT_EXPRESSION:
            {
                MCNewAutoNameRef t_expression;
                if (!ctxt . EvalExprAsNameRef(tgptr -> startpos, EE_CHUNK_BADBACKGROUNDEXP, &t_expression))
                    return;

                if (t_object . object -> gettype() == CT_CARD)
                    MCInterfaceEvalGroupOfCardByName(ctxt, t_object, tgptr -> ptype, *t_expression, t_object);
                else
                    MCInterfaceEvalGroupOfGroupByName(ctxt, t_object, *t_expression, t_object);
            }
                break;
            default:
                ctxt . LegacyThrow(EE_CHUNK_BADBACKGROUNDEXP);
                return;
        }
        tgptr = tgptr -> next;
    }

    // Stack override handles the case of 'control id ...' where there is no card
    // reference. It enables access to top-level objects in the stack via id.

    if (card == nil)
        t_stack_override = true;

    // MW-2011-08-08: [[ Bug ]] Loop through chain of object chunks. This allows
    //   things like field ... of control ... of.
    if (object != nil)
    {
        MCCRef *toptr = object;
        while (toptr != nil)
        {
            if (toptr -> otype == CT_MENU)
                MCInterfaceEvalMenubarAsObject(ctxt, t_object);

            switch (ct_class(toptr -> etype))
            {
                case CT_ORDINAL:
                    //                        if (toptr == object && group == nil)
                    if (t_object . object -> gettype() == CT_CARD)
                        MCInterfaceEvalObjectOfCardByOrdinal(ctxt, t_object, toptr -> otype, toptr -> ptype, toptr -> etype, t_object);
                    else if (t_object . object -> gettype() == CT_GROUP)
                        MCInterfaceEvalObjectOfGroupByOrdinal(ctxt, t_object, toptr -> otype, toptr -> etype, t_object);
                    else
                    {
                        ctxt . LegacyThrow(EE_CHUNK_OBJECTNOTCONTAINER);
                        return;
                    }
                    break;
                case CT_ID:
                {
                    uint4 t_id;
                    if (!ctxt . EvalExprAsStrictUInt(toptr -> startpos, EE_CHUNK_BADOBJECTEXP, t_id))
                        return;
                    
                    // If we are in stack override mode, then search the stack *after*
                    // searching the card as searching the stack will take longer.
                    //                        if (toptr == object && group == nil)
                    if (t_object . object -> gettype() == CT_CARD)
                    {
                        if (t_stack_override)
                            MCInterfaceEvalObjectOfCardOrStackById(ctxt, t_object, toptr -> otype, toptr -> ptype, t_id, t_object);
                        else
                            MCInterfaceEvalObjectOfCardById(ctxt, t_object, toptr -> otype, toptr -> ptype, t_id, t_object);
                    }
                    else if (t_object . object -> gettype() == CT_GROUP)
                    {
                        // if we have a group then stack override is irrelevant.
                        MCInterfaceEvalObjectOfGroupById(ctxt, t_object, toptr -> otype, t_id, t_object);
                    }
                    else
                    {
                        ctxt . LegacyThrow(EE_CHUNK_OBJECTNOTCONTAINER);
                        return;
                    }
                }
                    break;
                case CT_EXPRESSION:
                {
                    MCNewAutoNameRef t_expression;
                    if (!ctxt . EvalExprAsNameRef(toptr -> startpos, EE_CHUNK_BADOBJECTEXP, &t_expression))
                        return;
                    
                    if (t_object . object -> gettype() == CT_CARD)
                        MCInterfaceEvalObjectOfCardByName(ctxt, t_object, toptr -> otype, toptr -> ptype, *t_expression, t_object);
                    else if (t_object . object -> gettype() == CT_GROUP)
                        MCInterfaceEvalObjectOfGroupByName(ctxt, t_object, toptr -> otype, *t_expression, t_object);
                    else
                    {
                        ctxt . LegacyThrow(EE_CHUNK_OBJECTNOTCONTAINER);
                        return;
                    }
                }
                    break;
                default:
                    ctxt . LegacyThrow(EE_CHUNK_BADOBJECTEXP);
                    return;
            }
            toptr = toptr -> next;
        }
    }

    if (!ctxt . HasError())
    {
        r_object . object = t_object . object;
        r_object . part_id = t_object . part_id;
    }
}

bool MCChunk::getobj(MCExecContext &ctxt, MCObject *&objptr, uint4 &parid, Boolean recurse)
{
    objptr = nil;
    parid = 0;

    MCObjectPtr t_object;

    if (!getobj(ctxt, t_object, recurse))
        return false;

    objptr = t_object . object;
    parid = t_object . part_id;

    return true;
}

void MCStringsMarkTextChunkByOrdinal(MCExecContext& ctxt, Chunk_term p_chunk_type, Chunk_term p_ordinal_type, bool p_force, bool p_whole_chunk, bool p_further_chunks, MCMarkedText& x_mark);
void MCStringsMarkTextChunkByRange(MCExecContext& ctxt, Chunk_term p_chunk_type, integer_t p_first, integer_t p_last, bool p_force, bool p_whole_chunk, bool p_further_chunks, MCMarkedText& x_mark);

template
<
Chunk_term ChunkType,
void (*MarkRange)(MCExecContext& ctxt, int4 first, int4 last, MCMarkedText& x_mark)
>
static inline bool __MCCRefMarkForEval(MCExecContext& ctxt, MCCRef *self, MCMarkedText& x_mark)
{
    int4 t_first, t_last;
    if (self -> etype == CT_RANGE || self -> etype == CT_EXPRESSION)
    {
        if (!ctxt . EvalExprAsInt(self -> startpos, EE_CHUNK_BADRANGESTART, t_first))
            return false;
        
        if (self -> etype != CT_RANGE)
        {
            t_last = t_first;
        }
        else
        {
            if (!ctxt . EvalExprAsInt(self -> endpos, EE_CHUNK_BADRANGEEND, t_last))
                return false;
        }
    }
    else
    {
        switch(self -> etype)
        {
            case CT_ANY:
            case CT_MIDDLE:
                if (ChunkType != CT_BYTE)
                    MCStringsMarkTextChunkByOrdinal(ctxt,
                                                    ChunkType,
                                                    self -> etype,
                                                    false,
                                                    false,
                                                    false,
                                                    x_mark);
                else
                    MCStringsMarkBytesOfTextByOrdinal(ctxt,
                                                      self -> etype,
                                                      x_mark);
                return true;
            case CT_LAST:
                t_first = -1;
                t_last = -1;
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
                t_first = (self -> etype - CT_FIRST) + 1;
                t_last = t_first;
                break;
            default:
                ctxt . LegacyThrow(EE_CHUNK_BADEXTENTS);
                return false;
        }
    }
    
    MarkRange(ctxt, t_first, t_last, x_mark);
    
    return true;
}

template
<
Chunk_term ChunkType
>
inline void __MCCRefMarkChunkRangeForEval(MCExecContext& ctxt, int4 p_first, int4 p_last, MCMarkedText& x_mark)
{
    if (ChunkType != CT_BYTE)
        MCStringsMarkTextChunkByRange(ctxt, ChunkType, p_first, p_last, false, false, false, x_mark);
    else
        MCStringsMarkBytesOfTextByRange(ctxt, p_first, p_last, x_mark);
}

inline void __MCCRefMarkDelimitedChunkRangeForEval(MCExecContext& ctxt,
                                                          Chunk_term p_chunk_type,
                                                          MCStringRef p_delimiter,
                                                          int4 p_first,
                                                          int4 p_last,
                                                          MCMarkedText& x_mark)
{
    if (p_first < 0 || p_last < 0)
    {
        MCStringsMarkTextChunkByRange(ctxt, p_chunk_type, p_first, p_last, false, false, false, x_mark);
        return;
    }
    
    if (p_first > p_last)
        p_last = p_first - 1;
    else if (p_first == 0)
        p_first = 1;
    
    MCRange t_range;
    MCStringForwardDelimitedRegion((MCStringRef)x_mark . text,
                                   MCRangeMakeMinMax(x_mark . start, x_mark . finish),
                                   p_delimiter,
                                   MCRangeMakeMinMax((uindex_t)(p_first - 1), (uindex_t)p_last),
                                   ctxt . GetStringComparisonType(),
                                   t_range);
    
    x_mark . start = t_range . offset;
    x_mark . finish = t_range . offset + t_range . length;
}

inline void __MCCRefMarkLineRangeForEval(MCExecContext& ctxt, int4 p_first, int4 p_last, MCMarkedText& x_mark)
{
    __MCCRefMarkDelimitedChunkRangeForEval(ctxt,
                                           CT_LINE,
                                           ctxt . GetLineDelimiter(),
                                           p_first,
                                           p_last,
                                           x_mark);
}

inline void __MCCRefMarkItemRangeForEval(MCExecContext& ctxt, int4 p_first, int4 p_last, MCMarkedText& x_mark)
{
    __MCCRefMarkDelimitedChunkRangeForEval(ctxt,
                                           CT_ITEM,
                                           ctxt . GetItemDelimiter(),
                                           p_first,
                                           p_last,
                                           x_mark);
}

#define __MCCRefMarkChunkForEval(chunk, ctxt, cref, x_mark) \
        __MCCRefMarkForEval< chunk, __MCCRefMarkChunkRangeForEval<chunk> >(ctxt, cref, x_mark)

void MCChunk::mark_for_eval(MCExecContext& ctxt, MCMarkedText& x_mark)
{
    if (cline != nil &&
        !__MCCRefMarkForEval<CT_LINE, __MCCRefMarkLineRangeForEval>(ctxt, cline, x_mark))
        return;
    
    if (paragraph != nil &&
        !__MCCRefMarkChunkForEval(CT_PARAGRAPH, ctxt, paragraph, x_mark))
        return;
    
    if (sentence != nil &&
        !__MCCRefMarkChunkForEval(CT_SENTENCE, ctxt, sentence, x_mark))
        return;
        
    if (item != nil &&
        !__MCCRefMarkForEval<CT_ITEM, __MCCRefMarkItemRangeForEval>(ctxt, item, x_mark))
        return;
        
    if (word != nil &&
        !__MCCRefMarkChunkForEval(CT_WORD, ctxt, word, x_mark))
        return;
    
    if (trueword != nil &&
        !__MCCRefMarkChunkForEval(CT_TRUEWORD, ctxt, trueword, x_mark))
        return;
        
    if (token != nil &&
        !__MCCRefMarkChunkForEval(CT_TOKEN, ctxt, token, x_mark))
        return;
        
    if (character != nil &&
        !__MCCRefMarkChunkForEval(CT_CHARACTER, ctxt, character, x_mark))
        return;
    
    if (codepoint != nil &&
        !__MCCRefMarkChunkForEval(CT_CODEPOINT, ctxt, codepoint, x_mark))
        return;
    
    if (codeunit != nil &&
        !__MCCRefMarkChunkForEval(CT_CODEUNIT, ctxt, codeunit, x_mark))
        return;
        
    if (byte != nil &&
        !__MCCRefMarkChunkForEval(CT_BYTE, ctxt, byte, x_mark))
        return;
}

void MCChunk::mark(MCExecContext &ctxt, bool force, bool wholechunk, MCMarkedText& x_mark, bool includechars)
{
    int4 t_first, t_last;
    // SN-2014-09-03: [[ Bug 13314 ]] MCMarkedText::changed updated to store the number of chars appended
    x_mark . changed = 0;
    bool t_further_chunks = false;
    
    if (cline != nil)
    {
        t_further_chunks = (paragraph != nil || sentence != nil || item != nil || word != nil || trueword != nil
                            || character != nil || codepoint != nil || codeunit != nil || byte != nil);
        if (cline -> etype == CT_RANGE || cline -> etype == CT_EXPRESSION)
        {
            if (!ctxt . EvalExprAsInt(cline -> startpos, EE_CHUNK_BADRANGESTART, t_first))
                return;
        
            if (cline -> etype == CT_RANGE)
            {
                if (!ctxt . EvalExprAsInt(cline -> endpos, EE_CHUNK_BADRANGEEND, t_last))
                    return;
            }
            else
                t_last = t_first;
            
            MCStringsMarkLinesOfTextByRange(ctxt, t_first, t_last, force, wholechunk, t_further_chunks, x_mark);
        }
        else
            MCStringsMarkLinesOfTextByOrdinal(ctxt, cline -> etype, force, wholechunk, t_further_chunks, x_mark);
    }
    
    if (paragraph != nil)
    {
        t_further_chunks = (sentence != nil || item != nil || word != nil || trueword != nil
                            || character != nil || codepoint != nil || codeunit != nil || byte != nil);
        if (paragraph -> etype == CT_RANGE || paragraph -> etype == CT_EXPRESSION)
        {
            if (!ctxt . EvalExprAsInt(paragraph -> startpos, EE_CHUNK_BADRANGESTART, t_first))
                return;
            
            if (paragraph -> etype == CT_RANGE)
            {
                if (!ctxt . EvalExprAsInt(paragraph -> endpos, EE_CHUNK_BADRANGEEND, t_last))
                    return;
            }
            else
                t_last = t_first;
            
            MCStringsMarkParagraphsOfTextByRange(ctxt, t_first, t_last, force, wholechunk, t_further_chunks, x_mark);
        }
        else
            MCStringsMarkParagraphsOfTextByOrdinal(ctxt, paragraph -> etype, force, wholechunk, t_further_chunks, x_mark);
    }
    
    if (sentence != nil)
    {
        t_further_chunks = (item != nil || word != nil || trueword != nil || character != nil 
                            || codepoint != nil || codeunit != nil || byte != nil);
        if (sentence -> etype == CT_RANGE || sentence -> etype == CT_EXPRESSION)
        {
            if (!ctxt . EvalExprAsInt(sentence -> startpos, EE_CHUNK_BADRANGESTART, t_first))
                return;
            
            if (sentence -> etype == CT_RANGE)
            {
                if (!ctxt . EvalExprAsInt(sentence -> endpos, EE_CHUNK_BADRANGEEND, t_last))
                    return;
            }
            else
                t_last = t_first;
            
            MCStringsMarkSentencesOfTextByRange(ctxt, t_first, t_last, force, wholechunk, t_further_chunks, x_mark);
        }
        else
            MCStringsMarkSentencesOfTextByOrdinal(ctxt, sentence -> etype, force, wholechunk, t_further_chunks, x_mark);
    }
    
    if (item != nil)
    {
        t_further_chunks = (word != nil || trueword != nil || character != nil || codepoint != nil || codeunit != nil || byte != nil);
        if (item -> etype == CT_RANGE || item -> etype == CT_EXPRESSION)
        {
            if (!ctxt . EvalExprAsInt(item -> startpos, EE_CHUNK_BADRANGESTART, t_first))
                return;
            
            if (item -> etype == CT_RANGE)
            {
                if (!ctxt . EvalExprAsInt(item -> endpos, EE_CHUNK_BADRANGEEND, t_last))
                    return;
            }
            else
                t_last = t_first;
            
            MCStringsMarkItemsOfTextByRange(ctxt, t_first, t_last, force, wholechunk, t_further_chunks, x_mark);
        }
        else
            MCStringsMarkItemsOfTextByOrdinal(ctxt, item -> etype, force, wholechunk, t_further_chunks, x_mark);
    }
    
    if (word != nil)
    {
        t_further_chunks = (trueword != nil || character != nil || codepoint != nil || codeunit != nil || byte != nil);
        if (word -> etype == CT_RANGE || word -> etype == CT_EXPRESSION)
        {
            if (!ctxt . EvalExprAsInt(word -> startpos, EE_CHUNK_BADRANGESTART, t_first))
                return;
            
            if (word -> etype == CT_RANGE)
            {
                if (!ctxt . EvalExprAsInt(word -> endpos, EE_CHUNK_BADRANGEEND, t_last))
                    return;
            }
            else
                t_last = t_first;
            
            MCStringsMarkWordsOfTextByRange(ctxt, t_first, t_last, force, wholechunk, t_further_chunks, x_mark);
        }
        else
            MCStringsMarkWordsOfTextByOrdinal(ctxt, word -> etype, force, wholechunk, t_further_chunks, x_mark);
    }
    
    if (trueword != nil)
    {
        t_further_chunks = (character != nil || codepoint != nil || codeunit != nil || byte != nil);
        if (trueword -> etype == CT_RANGE || trueword -> etype == CT_EXPRESSION)
        {
            if (!ctxt . EvalExprAsInt(trueword -> startpos, EE_CHUNK_BADRANGESTART, t_first))
                return;
            
            if (trueword -> etype == CT_RANGE)
            {
                if (!ctxt . EvalExprAsInt(trueword -> endpos, EE_CHUNK_BADRANGEEND, t_last))
                    return;
            }
            else
                t_last = t_first;
            
            MCStringsMarkTrueWordsOfTextByRange(ctxt, t_first, t_last, force, wholechunk, t_further_chunks, x_mark);
        }
        else
            MCStringsMarkTrueWordsOfTextByOrdinal(ctxt, trueword -> etype, force, wholechunk, t_further_chunks, x_mark);
    }
    
    if (token != nil)
    {
        if (token -> etype == CT_RANGE || token -> etype == CT_EXPRESSION)
        {
            if (!ctxt . EvalExprAsInt(token -> startpos, EE_CHUNK_BADRANGESTART, t_first))
                return;

            if (token -> etype == CT_RANGE)
            {
                if (!ctxt . EvalExprAsInt(token -> endpos, EE_CHUNK_BADRANGEEND, t_last))
                    return;
            }
            else
                t_last = t_first;
            
            MCStringsMarkTokensOfTextByRange(ctxt, t_first, t_last, force, wholechunk, t_further_chunks, x_mark);
        }
        else
            MCStringsMarkTokensOfTextByOrdinal(ctxt, token -> etype, force, wholechunk, t_further_chunks, x_mark);
    }
    
    if (character != nil)
    {
        if (character -> etype == CT_RANGE || character -> etype == CT_EXPRESSION)
        {
            if (!ctxt . EvalExprAsInt(character -> startpos, EE_CHUNK_BADRANGESTART, t_first))
                return;

            if (character -> etype == CT_RANGE)
            {
                if (!ctxt . EvalExprAsInt(character -> endpos, EE_CHUNK_BADRANGEEND, t_last))
                    return;
            }
            else
                t_last = t_first;
            
            MCStringsMarkCharsOfTextByRange(ctxt, t_first, t_last, force, wholechunk, t_further_chunks, x_mark);
        }
        else
            MCStringsMarkCharsOfTextByOrdinal(ctxt, character -> etype, force, wholechunk, t_further_chunks, x_mark);
    }

    if (codepoint != nil)
    {
        if (codepoint -> etype == CT_RANGE || codepoint -> etype == CT_EXPRESSION)
        {
            if (!ctxt . EvalExprAsInt(codepoint -> startpos, EE_CHUNK_BADRANGESTART, t_first))
                return;
            
            if (codepoint -> etype == CT_RANGE)
            {
                if (!ctxt . EvalExprAsInt(codepoint -> endpos, EE_CHUNK_BADRANGEEND, t_last))
                    return;
            }
            else
                t_last = t_first;
            
            MCStringsMarkCodepointsOfTextByRange(ctxt, t_first, t_last, force, wholechunk, t_further_chunks, x_mark);
        }
        else
           MCStringsMarkCodepointsOfTextByOrdinal(ctxt, codepoint -> etype, force, wholechunk, t_further_chunks, x_mark);
    }
    
    if (codeunit != nil)
    {
        if (codeunit -> etype == CT_RANGE || codeunit -> etype == CT_EXPRESSION)
        {
            if (!ctxt . EvalExprAsInt(codeunit -> startpos, EE_CHUNK_BADRANGESTART, t_first))
                return;
            
            if (codeunit -> etype == CT_RANGE)
            {
                if (!ctxt . EvalExprAsInt(codeunit -> endpos, EE_CHUNK_BADRANGEEND, t_last))
                    return;
            }
            else
                t_last = t_first;
            
            MCStringsMarkCodeunitsOfTextByRange(ctxt, t_first, t_last, force, wholechunk, t_further_chunks, x_mark);
        }
        else
            MCStringsMarkCodeunitsOfTextByOrdinal(ctxt, codeunit -> etype, force, wholechunk, t_further_chunks, x_mark);
    }
    
    if (byte != nil)
    {
        if (byte -> etype == CT_RANGE || byte -> etype == CT_EXPRESSION)
        {
            if (!ctxt . EvalExprAsInt(byte -> startpos, EE_CHUNK_BADRANGESTART, t_first))
                return;
            
            if (byte -> etype == CT_RANGE)
            {
                if (!ctxt . EvalExprAsInt(byte -> endpos, EE_CHUNK_BADRANGEEND, t_last))
                    return;
            }
            else
                t_last = t_first;
            
            MCStringsMarkBytesOfTextByRange(ctxt, t_first, t_last, x_mark);
        }
        else
            MCStringsMarkBytesOfTextByOrdinal(ctxt, byte -> etype, x_mark);
    }
}

void MCChunk::eval_ctxt(MCExecContext &ctxt, MCExecValue &r_text)
{
    MCExecValue t_text;
    MCAutoValueRef t_valueref;
    bool t_exec_expr = false;

    if (iselementchunk())
    {
        ctxt . LegacyThrow(EE_CHUNK_OBJECTNOTCONTAINER);
        return;
    }
    else if (source != NULL && url == NULL && noobjectchunks())
    {
        if (desttype != DT_OWNER)
        {
            if (!ctxt . EvaluateExpression(source, EE_CHUNK_CANTGETSOURCE, t_text))
                return;
            t_exec_expr = true;
        }
        else
        {
            // MW-2008-11-05: [[ Owner Reference ]] This case handles the syntax:
            //     <text chunk> of the owner of ...
            //   In this case we evaluate the owner property of the resolved object.
            MCObjectPtr t_object;
            if (!static_cast<MCChunk *>(source) -> getobj(ctxt, t_object, True))
            {
                ctxt . LegacyThrow(EE_CHUNK_BADOBJECTEXP);
                return;
            }

            MCEngineEvalOwner(ctxt, t_object, (MCStringRef&)&t_valueref);
        }
    }
    else if (destvar != NULL)
    {
        if (!ctxt . EvaluateExpression(destvar, EE_CHUNK_CANTGETDEST, t_text))
            return;
        t_exec_expr = true;
    }
    else
    {
        MCAutoValueRef t_url_output;
        if (url != NULL)
        {
            MCAutoStringRef t_target;
            if (!ctxt . EvalExprAsStringRef(url -> startpos, EE_CHUNK_CANTGETDEST, &t_target))
                return;

            MCU_geturl(ctxt, *t_target, &t_url_output);
        }

        MCObjectPtr t_object;
        getoptionalobj(ctxt, t_object, True);

        if (t_object . object == nil)
        {
            if (url == NULL || !noobjectchunks())
            {
                ctxt . LegacyThrow(EE_CHUNK_CANTFINDOBJECT);
                return;
            }
            t_valueref = *t_url_output;
        }
        else
        {
            switch (function)
            {
                case F_CLICK_CHUNK:
                    MCInterfaceEvalClickChunk(ctxt, (MCStringRef&)&t_valueref);
                    break;
                case F_CLICK_CHAR_CHUNK:
                    MCInterfaceEvalClickCharChunk(ctxt, (MCStringRef&)&t_valueref);
                    break;
                case F_CLICK_LINE:
                    MCInterfaceEvalClickLine(ctxt, (MCStringRef&)&t_valueref);
                    break;
                case F_CLICK_TEXT:
                    MCInterfaceEvalClickText(ctxt, (MCStringRef&)&t_valueref);
                    break;
                case F_SELECTED_CHUNK:
                    MCInterfaceEvalSelectedChunk(ctxt, (MCStringRef&)&t_valueref);
                    break;
                case F_SELECTED_LINE:
                    MCInterfaceEvalSelectedLine(ctxt, (MCStringRef&)&t_valueref);
                    break;
                case F_SELECTED_TEXT:
                    MCInterfaceEvalSelectedText(ctxt, (MCStringRef&)&t_valueref);
                    break;
                case F_FOUND_CHUNK:
                    MCInterfaceEvalFoundChunk(ctxt, (MCStringRef&)&t_valueref);
                    break;
                case F_FOUND_LINE:
                    MCInterfaceEvalFoundLine(ctxt, (MCStringRef&)&t_valueref);
                    break;
                case F_FOUND_TEXT:
                    MCInterfaceEvalFoundText(ctxt, (MCStringRef&)&t_valueref);
                    break;
                case F_MOUSE_CHUNK:
                    MCInterfaceEvalMouseChunk(ctxt, (MCStringRef&)&t_valueref);
                    break;
                case F_MOUSE_LINE:
                    MCInterfaceEvalMouseLine(ctxt, (MCStringRef&)&t_valueref);
                    break;
                case F_MOUSE_CHAR_CHUNK:
                    MCInterfaceEvalMouseCharChunk(ctxt, (MCStringRef&)&t_valueref);
                    break;
                case F_MOUSE_TEXT:
                    MCInterfaceEvalMouseText(ctxt, (MCStringRef&)&t_valueref);
                    break;
                default:
                    MCMarkedText t_mark;
                    MCInterfaceMarkContainer(ctxt, t_object, false, t_mark);
                    MCStringsEvalTextChunk(ctxt, t_mark, (MCStringRef&)&t_valueref);
                    MCValueRelease(t_mark . text);
                    break;
            }
        }
    }

    if (*t_valueref != nil || t_exec_expr)
    {
        if (isstringchunk() || isdatachunk())
        {
            MCMarkedText t_new_mark;
            t_new_mark . text = nil;
            t_new_mark . start = 0;
            
            bool t_success = true;
            
            // AL-2014-09-10: [[ Bug 13400 ]] Keep marked strings the correct type where possible
            if (isstringchunk())
            {
                // Need a string ref, at least initially
                MCAutoStringRef t_text_str;
                if (t_exec_expr)
                {
                    MCExecTypeConvertAndReleaseAlways(ctxt, t_text . type, &t_text, kMCExecValueTypeStringRef, &(&t_text_str));
                    t_success = !ctxt . HasError();
                }
                else
                    t_success = ctxt . ConvertToString(*t_valueref, &t_text_str);
                
                if (!t_success)
                {
                    ctxt.Throw();
                    return;
                }
                t_new_mark . text = MCValueRetain(*t_text_str);
                // AL-2015-04-23: [[ Bug 15267 ]] Set the correct finish for the mark.
                t_new_mark . finish = MCStringGetLength(*t_text_str);
            }
            else
            {
                // Must be data
                MCAutoDataRef t_data;
                if (t_exec_expr)
                {
                    MCExecTypeConvertAndReleaseAlways(ctxt, t_text . type, &t_text, kMCExecValueTypeDataRef, &(&t_data));
                    t_success = !ctxt . HasError();
                }
                else
                    t_success = ctxt . ConvertToData(*t_valueref, &t_data);
                
                if (!t_success)
                {
                    ctxt.Throw();
                    return;
                }
                t_new_mark . text = MCValueRetain(*t_data);
                // AL-2015-04-23: [[ Bug 15267 ]] Set the correct finish for the mark. 
                t_new_mark . finish = MCDataGetLength(*t_data);
            }
            
            mark_for_eval(ctxt, t_new_mark);
            
            // SN-2014-12-15: [[ Bug 14211 ]] mark() can throw errors
            if (ctxt . HasError())
                return;
            
            if (!isdatachunk())
            {
                MCStringsEvalTextChunk(ctxt, t_new_mark, r_text . stringref_value);
                r_text . type = kMCExecValueTypeStringRef;
            }
            else
            {
                MCStringsEvalByteChunk(ctxt, t_new_mark, r_text . dataref_value);
                r_text . type = kMCExecValueTypeDataRef;
            }

            MCValueRelease(t_new_mark . text);
        }
        else
        {
            if (t_exec_expr)
                r_text = t_text;
            else
                MCExecTypeSetValueRef(r_text, MCValueRetain(*t_valueref));
        }
    }
    else
        MCExecTypeSetValueRef(r_text, MCValueRetain(kMCEmptyString));
}

bool MCChunk::set(MCExecContext &ctxt, Preposition_type p_type, MCValueRef p_value, bool p_unicode)
{
    if (iselementchunk())
    {
        ctxt . LegacyThrow(EE_CHUNK_SETNOTACONTAINER);
        return false;
    }
    else if (destvar != nil)
    {
        MCVariableChunkPtr t_var_chunk;
        if (!evalvarchunk(ctxt, false, true, t_var_chunk))
            return false;

        MCEngineExecPutIntoVariable(ctxt, p_value, p_type, t_var_chunk);
    }
    else if (isurlchunk())
    {
        MCUrlChunkPtr t_url_chunk;
        t_url_chunk . url = nil;
        if (!evalurlchunk(ctxt, false, true, p_type, t_url_chunk))
            return false;

        MCNetworkExecPutIntoUrl(ctxt, p_value, p_type, t_url_chunk);

        MCValueRelease(t_url_chunk . url);
        MCValueRelease(t_url_chunk . mark . text);
    }
    else
    {
        MCObjectChunkPtr t_obj_chunk;
        if (!evalobjectchunk(ctxt, false, true, t_obj_chunk))
            return false;

        if (p_unicode)
        {
            if (t_obj_chunk . object -> gettype() != CT_FIELD)
            {
                ctxt . LegacyThrow(EE_CHUNK_CANTSETUNICODEDEST);
                MCValueRelease(t_obj_chunk . mark . text);
                return false;
            }

            MCAutoDataRef t_data;
            if (!ctxt . ConvertToData(p_value, &t_data))
            {
                ctxt . LegacyThrow(EE_CHUNK_CANTSETUNICODEDEST);
                MCValueRelease(t_obj_chunk . mark . text);
                return false;
            }
            MCInterfaceExecPutUnicodeIntoField(ctxt, *t_data, p_type, t_obj_chunk);
        }
        else
        {
            MCAutoStringRef t_string;
            if (!ctxt . ConvertToString(p_value, &t_string))
            {
                ctxt . LegacyThrow(EE_CHUNK_CANTSETDEST);
                MCValueRelease(t_obj_chunk . mark . text);
                return false;
            }

            if (t_obj_chunk . object -> gettype() == CT_FIELD)
                MCInterfaceExecPutIntoField(ctxt, *t_string, p_type, t_obj_chunk);
            else
            {
                // AL-2014-08-04: [[ Bug 13081 ]] 'put into <chunk>' is valid for any container type
                switch (t_obj_chunk . object -> gettype())
                {
                    case CT_BUTTON:
                    case CT_IMAGE:
                    case CT_AUDIO_CLIP:
                    case CT_VIDEO_CLIP:
                        MCInterfaceExecPutIntoObject(ctxt, *t_string, p_type, t_obj_chunk);
                        break;
                    default:
                        ctxt . LegacyThrow(EE_CHUNK_SETNOTACONTAINER);
                        break;
                }
            }
        }
        MCValueRelease(t_obj_chunk . mark . text);
    }

    if (!ctxt . HasError())
        return true;

    return false;
}

bool MCChunk::set(MCExecContext &ctxt, Preposition_type p_type, MCExecValue p_value, bool p_unicode)
{    
    if (iselementchunk())
    {
        ctxt . LegacyThrow(EE_CHUNK_SETNOTACONTAINER);
        return false;
    }
    else if (destvar != nil)
    {
        MCVariableChunkPtr t_var_chunk;
        if (!evalvarchunk(ctxt, false, true, t_var_chunk))
            return false;

        MCEngineExecPutIntoVariable(ctxt, p_value, p_type, t_var_chunk);
    }
    else if (isurlchunk())
    {
        MCAutoValueRef t_valueref;
        MCExecTypeConvertAndReleaseAlways(ctxt, p_value . type, &p_value, kMCExecValueTypeValueRef, &(&t_valueref));
        MCUrlChunkPtr t_url_chunk;
        t_url_chunk . url = nil;
        if (!evalurlchunk(ctxt, false, true, p_type, t_url_chunk))
            return false;
        
        MCNetworkExecPutIntoUrl(ctxt, *t_valueref, p_type, t_url_chunk);
        
        MCValueRelease(t_url_chunk . url);
        MCValueRelease(t_url_chunk . mark . text);
    }
    else
    {
        MCObjectChunkPtr t_obj_chunk;
        if (!evalobjectchunk(ctxt, false, true, t_obj_chunk))
            return false;
        
        if (p_unicode)
        {
            if (t_obj_chunk . object -> gettype() != CT_FIELD)
            {
                ctxt . LegacyThrow(EE_CHUNK_CANTSETUNICODEDEST);
                MCValueRelease(t_obj_chunk . mark . text);
                return false;
            }
            
            MCAutoDataRef t_data;
            MCExecTypeConvertAndReleaseAlways(ctxt, p_value . type, &p_value, kMCExecValueTypeDataRef, &(&t_data));
            if (ctxt . HasError())
            {
                ctxt . LegacyThrow(EE_CHUNK_CANTSETUNICODEDEST);
                MCValueRelease(t_obj_chunk . mark . text);
                return false;
            }
            MCInterfaceExecPutUnicodeIntoField(ctxt, *t_data, p_type, t_obj_chunk);
        }
        else if (t_obj_chunk . object -> gettype() == CT_FIELD)
        {
            MCAutoStringRef t_string;
            MCExecTypeConvertAndReleaseAlways(ctxt, p_value . type, &p_value, kMCExecValueTypeStringRef, &(&t_string));
            if (ctxt . HasError())
            {
                ctxt . LegacyThrow(EE_CHUNK_CANTSETDEST);
                MCValueRelease(t_obj_chunk . mark . text);
                return false;
            }            
            
            MCInterfaceExecPutIntoField(ctxt, *t_string, p_type, t_obj_chunk);
        }
        else
        {
            // AL-2014-08-04: [[ Bug 13081 ]] 'put into <chunk>' is valid for any container type
            switch (t_obj_chunk . object -> gettype())
            {
                case CT_BUTTON:
                case CT_IMAGE:
                case CT_AUDIO_CLIP:
                case CT_VIDEO_CLIP:
                    MCInterfaceExecPutIntoObject(ctxt, p_value, p_type, t_obj_chunk);
                    break;
                default:
                    ctxt . LegacyThrow(EE_CHUNK_SETNOTACONTAINER);
                    break;
            }
        }
        
        MCValueRelease(t_obj_chunk . mark . text);
    }
    
    if (!ctxt . HasError())
        return true;
    
    return false;
}

void MCChunk::count(MCExecContext &ctxt, Chunk_term tocount, Chunk_term ptype, uinteger_t& r_count)
{
    // MW-2009-07-22: First non-control chunk is now CT_ELEMENT.
    if (tocount <= CT_LAST_CONTROL)
    {
        uint2 i = 0;
        MCObject *optr;
        uint4 parid;
        getoptionalobj(ctxt, optr, parid, True);

        if (optr == nil)
        {
            if (stack != NULL)
            {
                // AL-2014-06-09: [[ Bug 12596 ]] Throw error when the chunk expression
                //  does not resolve to an object on the specified stack.
                ctxt . Throw();
                return;
            }
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
        r_count = i;
    }
    else if (tocount < CT_BYTE)
    {
        MCAutoStringRef t_string;
        if (!ctxt . EvalExprAsStringRef(this, EE_CHUNK_BADEXPRESSION, &t_string))
            return;
        
        MCStringsCountChunks(ctxt, tocount, *t_string, r_count);
    }
    else if (tocount == CT_BYTE)
    {
        MCAutoDataRef t_data;
        if (!ctxt . EvalExprAsDataRef(this, EE_CHUNK_BADEXPRESSION, &t_data))
            return;
        
       r_count = MCDataGetLength(*t_data);
    }
    else
    {
        MCAutoValueRef t_value;

        // MW-2009-07-22: If the value of the chunk is an array, then either
        //   the count will be zero (if a string chunk has been requested),
        //   otherwise it will be the count of the keys...
        // SN-2013-11-5: Apperently eval() is no longer able to return an array
        if (!ctxt . EvalExprAsValueRef(this, EE_CHUNK_BADEXPRESSION, &t_value))
            return;

        if (MCValueGetTypeCode(*t_value) == kMCValueTypeCodeArray)
            r_count = MCArrayGetCount((MCArrayRef)*t_value);
        else
            // AL-2014-07-10: [[ Bug 12795 ]] Don't call MCStringsCountChunks with a non-string chunk expression.
            r_count = 0;
    }
}

bool MCChunk::getobjforprop(MCExecContext& ctxt, MCObject*& r_object, uint4& r_parid)
{
	MCObject *objptr;
	uint4 parid;
    
	if (!getobj(ctxt, objptr, parid, True))
	{
		MCeerror->add(EE_CHUNK_CANTFINDOBJECT, line, pos);
		return false;
	}
	Boolean tfunction = False;
	if (desttype == DT_FUNCTION && function != F_CLICK_FIELD
        && function != F_SELECTED_FIELD && function != F_FOUND_FIELD
        && function != F_MOUSE_CONTROL && function != F_FOCUSED_OBJECT
        && function != F_SELECTED_IMAGE
        && function != F_DRAG_SOURCE && function != F_DRAG_DESTINATION)
		tfunction = True;
	if (!tfunction && notextchunks())
	{
		r_object = objptr;
		r_parid = parid;
		return true;
	}
    
	MCeerror->add(EE_CHUNK_NOPROP, line, pos);
	return false;
}

static MCPropertyInfo *lookup_object_property(const MCObjectPropertyTable *p_table, Properties p_which, bool p_effective, bool p_array_prop, MCPropertyInfoChunkType p_chunk_type)
{
	for(uindex_t i = 0; i < p_table -> size; i++)
		if (p_table -> table[i] . property == p_which && (!p_table -> table[i] . has_effective || p_table -> table[i] . effective == p_effective) &&
            (p_array_prop == p_table -> table[i] . is_array_prop) &&
            (p_chunk_type == p_table -> table[i] . chunk_type))
			return &p_table -> table[i];
	
	if (p_table -> parent != nil)
		return lookup_object_property(p_table -> parent, p_which, p_effective, p_array_prop, p_chunk_type);
	
	return nil;
}

// SN-2015-02-13: [[ Bug 14467 ]] [[ Bug 14053 ]] Refactored object properties
//  lookup, to ensure it is done the same way in MCChunk::getprop / setprop
bool MCChunk::getsetprop(MCExecContext &ctxt, Properties which, MCNameRef index, Boolean effective, bool p_is_get_operation, MCExecValue &r_value)
{
    MCObjectChunkPtr t_obj_chunk;
    // SN-2015-05-05: [[ Bug 13314 Reopen ]] We force the chunk delimiter
    //  existence when setting a string value.
    if (!evalobjectchunk(ctxt, false, !p_is_get_operation, t_obj_chunk))
        return false;
    
    MCPropertyInfo *t_info;
    
    if (t_obj_chunk . chunk == CT_UNDEFINED)
    {
        bool t_success;
        if (p_is_get_operation)
            t_success = t_obj_chunk . object -> getprop(ctxt, t_obj_chunk . part_id, which, index, effective, r_value);
        else
            t_success = t_obj_chunk . object -> setprop(ctxt, t_obj_chunk . part_id, which, index, effective, r_value);
        
        // AL-2015-03-04: [[ Bug 14737 ]] Ensure property listener is signalled.
        if (!t_success)
            ctxt . Throw();
    }
    else
    {
        // AL-2014-07-09: [[ Bug 12733 ]] Buttons are also valid containers wrt text chunk properties.
        if (t_obj_chunk . object -> gettype() != CT_FIELD &&
            t_obj_chunk . object -> gettype() != CT_BUTTON)
        {
            MCeerror->add(EE_CHUNK_BADCONTAINER, line, pos);
            MCValueRelease(t_obj_chunk . mark . text);
            return false;
        }
        
        // MW-2011-11-23: [[ Array Chunk Props ]] If index is nil or empty, then its just a normal
        //   prop, else its an array prop.
        bool t_is_array_prop;
        t_is_array_prop = (index != nil && !MCNameIsEmpty(index));
        
        t_info = lookup_object_property(t_obj_chunk . object -> getpropertytable(), which, effective == True, t_is_array_prop, islinechunk() ? kMCPropertyInfoChunkTypeLine : kMCPropertyInfoChunkTypeChar);
        
        // If we could not get the line property for this chunk, then we try to get the char prop.
        // If we could not get the char property for this chunk, then we try to get the line prop.
        if (t_info == nil)
            t_info = lookup_object_property(t_obj_chunk . object -> getpropertytable(), which, effective == True, t_is_array_prop, islinechunk() ? kMCPropertyInfoChunkTypeChar : kMCPropertyInfoChunkTypeLine);
        
        if (t_info == nil
                || (p_is_get_operation && t_info -> getter == nil)
                || (!p_is_get_operation && t_info -> setter == nil))
        {
            if (p_is_get_operation)
                MCeerror -> add(EE_OBJECT_GETNOPROP, line, pos);
            else
                MCeerror -> add(EE_OBJECT_SETNOPROP, line, pos);
            
            MCValueRelease(t_obj_chunk . mark . text);
            return false;
        }
        
        if (t_is_array_prop)
        {
            MCObjectChunkIndexPtr t_obj_chunk_index;
            t_obj_chunk_index . object = t_obj_chunk . object;
            t_obj_chunk_index . part_id = t_obj_chunk . part_id;
            t_obj_chunk_index . chunk = t_obj_chunk . chunk;
            t_obj_chunk_index . mark = t_obj_chunk . mark;
            t_obj_chunk_index . index = index;
            if (p_is_get_operation)
                MCExecFetchProperty(ctxt, t_info, &t_obj_chunk_index, r_value);
            else
                MCExecStoreProperty(ctxt, t_info, &t_obj_chunk_index, r_value);
        }
        else
        {
            if (p_is_get_operation)
                MCExecFetchProperty(ctxt, t_info, &t_obj_chunk, r_value);
            else
                MCExecStoreProperty(ctxt, t_info, &t_obj_chunk, r_value);
        }
    }
    
    MCValueRelease(t_obj_chunk . mark . text);
    
    if (!p_is_get_operation && !ctxt . HasError())
    {
        // MM-2012-09-05: [[ Property Listener ]] Make sure any listeners are updated of the property change.
        //  Handled at this point rather than MCProperty::set as here we know if it is a valid object set prop.
        t_obj_chunk . object -> signallisteners(which);
        return true;
    }
    
    return !ctxt . HasError();
}

// MW-2011-11-23: [[ Array Chunk Props ]] If index is not nil, then treat as an array chunk prop
bool MCChunk::getprop(MCExecContext& ctxt, Properties which, MCNameRef index, Boolean effective, MCExecValue& r_value)
{
    // SN-2015-02-13: [[ Bug 14467 ]] Object property getting / setting refactored
    return getsetprop(ctxt, which, index, effective, true, r_value);
}

// MW-2011-11-23: [[ Array Chunk Props ]] If index is not nil, then treat as an array chunk prop
bool MCChunk::setprop(MCExecContext& ctxt, Properties which, MCNameRef index, Boolean effective, MCExecValue p_value)
{
    // SN-2015-02-13: [[ Bug 14467 ]] Object property getting / setting refactored
    return getsetprop(ctxt, which, index, effective, false, p_value);
}

bool MCChunk::getsetcustomprop(MCExecContext &ctxt, MCNameRef p_prop_name, MCNameRef p_index_name, bool p_is_get_operation, MCExecValue &r_value)
{
    bool t_success;
    t_success = true;
    
    MCObject *t_object;
    uint4 t_parid;
    if (t_success)
        t_success = getobjforprop(ctxt, t_object, t_parid);
    
    MCAutoProperListRef t_path;
    if (t_success && iselementchunk())
        t_success = evalelementchunk(ctxt, &t_path);
    
    // MW-2011-09-02: Moved handling of customprop != nil case into resolveprop,
    //   so t_prop_name is always non-nil if t_prop == P_CUSTOM.
    // MW-2011-11-23: [[ Array Chunk Props ]] Moved handling of arrayprops into
    //   MCChunk::setprop.
    if (t_success)
    {
        if (p_index_name == nil)
        {
            if (p_is_get_operation)
                t_success = t_object -> getcustomprop(ctxt, t_object -> getdefaultpropsetname(), p_prop_name, *t_path, r_value);
            else
                t_success = t_object -> setcustomprop(ctxt, t_object -> getdefaultpropsetname(), p_prop_name, *t_path, r_value);
        }
        else
        {
            if (p_is_get_operation)
                t_success = t_object -> getcustomprop(ctxt, p_prop_name, p_index_name, *t_path, r_value);
            else
                t_success = t_object -> setcustomprop(ctxt, p_prop_name, p_index_name, *t_path, r_value);
        }
    }
    
    if (t_success && !p_is_get_operation)
    {
        // MM-2012-09-05: [[ Property Listener ]] Make sure setting a custom property sends propertyChanged message to listeners.
        t_object -> signallisteners(P_CUSTOM);
    }
    
    return t_success;
}

bool MCChunk::getcustomprop(MCExecContext& ctxt, MCNameRef p_prop_name, MCNameRef p_index_name, MCExecValue& r_value)
{
    return getsetcustomprop(ctxt, p_prop_name, p_index_name, true, r_value);
}

bool MCChunk::setcustomprop(MCExecContext& ctxt, MCNameRef p_prop_name, MCNameRef p_index_name, MCExecValue p_value)
{
    return getsetcustomprop(ctxt, p_prop_name, p_index_name, false, p_value);
}

Chunk_term MCChunk::getlastchunktype(void)
{
    if (byte != nil)
        return CT_BYTE;
    if (codeunit != nil)
        return CT_CODEUNIT;
    if (codepoint != nil)
        return CT_CODEPOINT;
	if (character != nil)
		return CT_CHARACTER;
	if (item != nil)
		return CT_ITEM;
    if (trueword != nil)
        return CT_TRUEWORD;
	if (word != nil)
		return CT_WORD;
	if (token != nil)
		return CT_TOKEN;
    if (sentence != nil)
        return CT_SENTENCE;
    if (paragraph != nil)
        return CT_PARAGRAPH;
	if (cline != nil)
		return CT_LINE;
	return CT_UNDEFINED;
}

bool MCChunk::evalvarchunk(MCExecContext& ctxt, bool p_whole_chunk, bool p_force, MCVariableChunkPtr& r_chunk)
{
    if (isstringchunk() || isdatachunk())
        MCEngineMarkVariable(ctxt, destvar, isdatachunk(), r_chunk . mark);

    mark(ctxt, p_force, p_whole_chunk, r_chunk . mark);

    if (ctxt . HasError())
    {
        ctxt . LegacyThrow(EE_CHUNK_CANTMARK);
        return false;
	}

	r_chunk . variable = destvar;
    r_chunk . chunk = getlastchunktype();

    return true;
}

bool MCChunk::evalurlchunk(MCExecContext &ctxt, bool p_whole_chunk, bool p_force, int p_type, MCUrlChunkPtr &r_chunk)
{
    MCAutoStringRef t_url;

    if (!ctxt . EvalExprAsStringRef(url -> startpos, EE_CHUNK_BADEXPRESSION, &t_url))
        return false;
    
    // AL-2014-09-10: Don't fetch the url if this is a simple 'put into url...'
    if (p_type != PT_INTO || getlastchunktype() != CT_UNDEFINED)
    {
        MCNetworkMarkUrl(ctxt, *t_url, r_chunk . mark);
        mark(ctxt, p_force, p_whole_chunk, r_chunk . mark);
    }
    else
        r_chunk . mark . text = nil;

    if (ctxt . HasError())
    {
        ctxt . LegacyThrow(EE_CHUNK_CANTMARK);
        return false;
    }

    r_chunk . url = MCValueRetain(*t_url);
    r_chunk . chunk = getlastchunktype();

    return true;
}

bool MCChunk::evalobjectchunk(MCExecContext &ctxt, bool p_whole_chunk, bool p_force, MCObjectChunkPtr &r_chunk)
{
    MCObjectPtr t_object;
    if (!getobj(ctxt, t_object, True))
    {
        ctxt . LegacyThrow(EE_CHUNK_CANTFINDOBJECT);
        return false;
    }

    bool t_function = false;
    if (desttype == DT_FUNCTION && function != F_CLICK_FIELD
        && function != F_SELECTED_FIELD && function != F_FOUND_FIELD
        && function != F_MOUSE_CONTROL && function != F_FOCUSED_OBJECT
        && function != F_SELECTED_IMAGE
        && function != F_DRAG_SOURCE && function != F_DRAG_DESTINATION)
        t_function = true;

    // AL-2014-03-27: [[ Bug 12042 ]] Object chunk evaluation should include
    //  paragraph, sentence, and trueword chunks.
    if (!t_function && !isstringchunk() && !isdatachunk())
    {
        MCMarkedText t_mark;
        t_mark . finish = INDEX_MAX;
        t_mark . start = 0;
        // SN-2014-09-03: [[ Bug 13314 ]] MCMarkedText::changed updated to store the number of chars appended
        t_mark . changed = 0;
        t_mark . text = nil;
        r_chunk . object = t_object.object;
        r_chunk . part_id = t_object.part_id;
        r_chunk . chunk = CT_UNDEFINED;
        r_chunk . mark = t_mark;
        return true;
    }

    if (t_function && t_object . object -> gettype() == CT_FIELD)
        MCInterfaceMarkFunction(ctxt, t_object, function, p_whole_chunk, r_chunk . mark);
    else
        MCInterfaceMarkContainer(ctxt, t_object, p_whole_chunk, r_chunk . mark);

    mark(ctxt, p_force, p_whole_chunk, r_chunk . mark);

    if (ctxt . HasError())
    {
        MCValueRelease(r_chunk . mark . text);
        ctxt . LegacyThrow(EE_CHUNK_CANTMARK);
        return false;
    }

    r_chunk . object = t_object . object;
    r_chunk . part_id = t_object . part_id;
    r_chunk . chunk = !t_function ? getlastchunktype() : CT_CHARACTER;

    return true;
}

bool MCChunk::evalelementchunk(MCExecContext& ctxt, MCProperListRef& r_elements)
{
    MCAutoProperListRef t_elements;
    if (!MCProperListCreateMutable(&t_elements))
        return false;
    
    MCCRef *t_chunk;

    for (t_chunk = element; t_chunk != nil; t_chunk = t_chunk -> next)
    {
        MCValueRef t_evaluated;
        if (!ctxt . EvalExprAsValueRef(t_chunk -> startpos, EE_CHUNK_BADEXPRESSION, t_evaluated))
            return false;
    
        if (MCValueIsArray(t_evaluated))
        {
            if (!MCExtensionConvertFromScriptType(ctxt, kMCProperListTypeInfo, t_evaluated))
            {
                MCValueRelease(t_evaluated);
                return false;
            }
            
            if (!MCProperListAppendList(*t_elements, (MCProperListRef)t_evaluated))
                return false;
        }
        else
        {
            if (!MCExtensionConvertFromScriptType(ctxt, kMCStringTypeInfo, t_evaluated))
            {
                MCValueRelease(t_evaluated);
                return false;
            }
            
            if (!MCProperListPushElementOntoBack(*t_elements, t_evaluated))
                return false;
        }
        
    }
        
    return MCProperListCopy(*t_elements, r_elements);
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
    bool t_function = false;
    if (desttype == DT_FUNCTION && function != F_CLICK_FIELD
        && function != F_SELECTED_FIELD && function != F_FOUND_FIELD
        && function != F_MOUSE_CONTROL && function != F_FOCUSED_OBJECT
        && function != F_SELECTED_IMAGE
        && function != F_DRAG_SOURCE && function != F_DRAG_DESTINATION)
        t_function = true;
    
    if (!t_function && !isstringchunk() && !isdatachunk())
        return false;
    
    return true;
}

// MW-2012-01-25: [[ ParaStyles ]] Returns true if this is an explicit 'line' chunk.
// MW-2012-02-13: [[ Bug ]] Some function chunks should also be treated as lines.
bool MCChunk::islinechunk(void) const
{
	// If we have any of item, token, word, character, codepoint, codeunit or byte entries then we are not a line.
	if (paragraph != nil || sentence != nil || item != nil || token != nil || trueword != nil
        || word != nil || character != nil || codepoint != nil || codeunit != nil || byte != nil)
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
	
	if (isstringchunk() || isdatachunk())
		return false;
	
	if (destvar != nil)
		return true;
	
	return false;
}

bool MCChunk::isurlchunk(void) const
{
	return url != nil;
}

bool MCChunk::issubstringchunk(void) const
{
	if (destvar == nil)
		return false;

    if (m_transient_text_chunk)
        return false;
    
	if (isstringchunk() || isdatachunk())
		return true;

	return false;
}

bool MCChunk::isstringchunk(void) const
{
    if (cline != nil || paragraph != nil || sentence != nil || item != nil || token != nil || trueword != nil
        || word != nil || character != nil || codepoint != nil || codeunit != nil)
		return true;
    
    return false;
}

////////////////////////////////////////////////////////////////////////////////

MCChunkType MCChunkTypeFromChunkTerm(Chunk_term p_chunk_term)
{
    switch (p_chunk_term)
    {
        case CT_LINE:
            return kMCChunkTypeLine;
        case CT_PARAGRAPH:
            return kMCChunkTypeParagraph;
        case CT_SENTENCE:
            return kMCChunkTypeSentence;
        case CT_ITEM:
            return kMCChunkTypeItem;
        case CT_TRUEWORD:
            return kMCChunkTypeTrueWord;
        case CT_WORD:
            return kMCChunkTypeWord;
        case CT_TOKEN:
            return kMCChunkTypeToken;
        case CT_CHARACTER:
            return kMCChunkTypeCharacter;
        case CT_CODEPOINT:
            return kMCChunkTypeCodepoint;
        case CT_CODEUNIT:
            return kMCChunkTypeCodeunit;
        case CT_BYTE:
            return kMCChunkTypeByte;
        default:
            MCUnreachableReturn(kMCChunkTypeLine);
    }
}

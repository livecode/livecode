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

#include "scriptpt.h"
//#include "execpt.h"
#include "cmds.h"
#include "chunk.h"
#include "mcerror.h"
#include "object.h"
#include "stack.h"
#include "card.h"
#include "printer.h"
#include "util.h"
#include "globals.h"
#include "securemode.h"
#include "exec.h"

#include "syntax.h"
#include "graphics_util.h"

MCPrint::MCPrint()
{
	mode = PM_CARD;
	target = NULL;
	from = NULL;
	to = NULL;
	rect = NULL;
	initial_state = NULL;
	bookmark_closed = false;
}

MCPrint::~MCPrint()
{
	delete target;
	delete from;
	delete to;
	delete rect;
	delete initial_state;
}

// This parses the following forms:
//   print break
//   print ( all | marked ) cards [ of <chunk> ] [ from <expr> to <expr> ] [ into <expr> ]
//   print <chunk> [ from <expr> to <expr> ] [ into <expr> ]
//   print <chunk> cards [ from <expr> to <expr> ] [ into <expr> ]
//
// New interactivity related print syntax:
//   print anchor <name> at <position>
//   print link to <anchor or url> with rect[angle] <rect>
//
Parse_stat MCPrint::parse(MCScriptPoint &sp)
{
	Symbol_type type;
	const LT *te;
	Boolean single = False;

	initpoint(sp);

	// print anchor <name> at <position>
	//   <name> -> from
	//   <position> -> rect
	//
	if (sp . skip_token(SP_SUGAR, TT_UNDEFINED, SG_ANCHOR) == PS_NORMAL)
	{
		mode = PM_ANCHOR;

		if (sp . parseexp(False, True, &from) != PS_NORMAL)
		{
			MCperror -> add(PE_PRINTANCHOR_BADNAMEEXP, sp);
			return PS_ERROR;
		}

		if (sp . skip_token(SP_FACTOR, TT_PREP, PT_AT) != PS_NORMAL)
		{
			MCperror -> add(PE_PRINTANCHOR_NOATEXP, sp);
			return PS_ERROR;
		}

		if (sp . parseexp(False, True, &rect) != PS_NORMAL)
		{
			MCperror -> add(PE_PRINTANCHOR_BADTOEXP, sp);
			return PS_ERROR;
		}

		return PS_NORMAL;
	}

	// print link to [ url | anchor ] <anchor or url> with rect[angle] <rect>
	//   <anchor or url> -> to
	//   <rect> -> rect
	//
	if (sp . skip_token(SP_SUGAR, TT_UNDEFINED, SG_LINK) == PS_NORMAL)
	{
		mode = PM_LINK;

		if (sp . skip_token(SP_FACTOR, TT_TO, PT_TO) != PS_NORMAL)
		{
			MCperror -> add(PE_PRINTLINK_NOTOEXP, sp);
			return PS_ERROR;
		}

		if (sp.skip_token(SP_SUGAR, TT_UNDEFINED, SG_ANCHOR) == PS_NORMAL)
			mode = PM_LINK_ANCHOR;
		else if (sp.skip_token(SP_SUGAR, TT_UNDEFINED, SG_URL) == PS_NORMAL)
			mode = PM_LINK_URL;

		if (sp . parseexp(False, True, &to) != PS_NORMAL)
		{
			MCperror -> add(PE_PRINTLINK_BADTOEXP, sp);
			return PS_ERROR;
		}
		
		if (sp . skip_token(SP_REPEAT, TT_UNDEFINED, RF_WITH) != PS_NORMAL ||
			sp . skip_token(SP_FACTOR, TT_PROPERTY, P_RECTANGLE) != PS_NORMAL)
		{
			MCperror -> add(PE_PRINTLINK_NOAREAEXP, sp);
			return PS_ERROR;
		}

		if (sp . parseexp(False, True, &rect) != PS_NORMAL)
		{
			MCperror -> add(PE_PRINTLINK_BADAREAEXP, sp);
			return PS_ERROR;
		}

		return PS_NORMAL;
	}

	// print [unicode] bookmark <title> [with level <level>] [at <position>]
	//   <title> -> from
	//   <level> -> to
	//   <position> -> rect
	if (sp.skip_token(SP_SUGAR, TT_UNDEFINED, SG_UNICODE) == PS_NORMAL)
		mode = PM_UNICODE_BOOKMARK;
	if (sp . skip_token(SP_SUGAR, TT_UNDEFINED, SG_BOOKMARK) != PS_NORMAL)
	{
		if (mode == PM_UNICODE_BOOKMARK)
		{
			MCperror->add(PE_PRINTBOOKMARK_NOBOOKMARK, sp);
			return PS_ERROR;
		}
	}
	else
	{
		if (mode != PM_UNICODE_BOOKMARK)
			mode = PM_BOOKMARK;

		if (sp.parseexp(False, True, &from) != PS_NORMAL)
		{
			MCperror->add(PE_PRINTBOOKMARK_BADTITLEEXP, sp);
			return PS_ERROR;
		}
		if (sp.skip_token(SP_REPEAT, TT_UNDEFINED, RF_WITH) == PS_NORMAL)
		{
			if (sp.skip_token(SP_SUGAR, TT_UNDEFINED, SG_LEVEL) != PS_NORMAL)
			{
				MCperror->add(PE_PRINTBOOKMARK_NOLEVEL, sp);
				return PS_ERROR;
			}
			if (sp.parseexp(False, True, &to) != PS_NORMAL)
			{
				MCperror->add(PE_PRINTBOOKMARK_BADLEVELEXP, sp);
				return PS_ERROR;
			}
		}
		if (sp.skip_token(SP_FACTOR, TT_PREP, PT_AT) == PS_NORMAL)
		{
			if (sp.parseexp(False, True, &rect) != PS_NORMAL)
			{
				MCperror->add(PE_PRINTBOOKMARK_BADATEXP, sp);
				return PS_ERROR;
			}
		}
		if (sp.skip_token(SP_SUGAR, TT_UNDEFINED, SG_INITIALLY) == PS_NORMAL)
		{
			if (sp.skip_token(SP_SUGAR, TT_UNDEFINED, SG_OPEN) == PS_NORMAL)
				bookmark_closed = false;
			else if (sp.skip_token(SP_SUGAR, TT_UNDEFINED, SG_CLOSED) == PS_NORMAL)
				bookmark_closed = true;
			else if (sp.parseexp(False, True, &initial_state) != PS_NORMAL)
			{
				MCperror->add(PE_PRINTBOOKMARK_BADINITIALEXP, sp);
				return PS_ERROR;
			}
		}

		return PS_NORMAL;
	}

	// Rest of the print syntax
	if (sp.next(type) == PS_NORMAL)
	{
		if (sp.lookup(SP_SHOW, te) == PS_NORMAL)
		{
			if (te->which == SO_ALL || te->which == SO_MARKED)
			{
				mode = (Show_object)te->which == SO_ALL ? PM_ALL : PM_MARKED;
				if (sp.skip_token(SP_FACTOR, TT_CLASS, CT_CARD) != PS_NORMAL)
				{
					MCperror->add(PE_PRINT_BADTARGET, sp);
					return PS_ERROR;
				}
			}
			else if (te->which == SO_BREAK)
			{
				sp.skip_token(SP_SHOW, TT_UNDEFINED, SO_BREAK);
				mode = PM_BREAK;
				return PS_NORMAL;
			}
			else if (te->which == SO_CARD)
			{
				MCScriptPoint oldsp(sp);
				sp.backup();
				target = new MCChunk(False);
				MCerrorlock++;
				if (target->parse(sp, False) != PS_NORMAL)
				{
					sp = oldsp;
					delete target;
					target = NULL;
					single = True;
				}
				MCerrorlock--;
			}
		}
		else
			sp . backup();
	}
	if ((mode != PM_CARD && sp . skip_token(SP_FACTOR, TT_OF) == PS_NORMAL) ||
		(mode == PM_CARD && !single && target == NULL))
	{
		target = new MCChunk(False);
		if (target->parse(sp, False) != PS_NORMAL)
		{
			MCperror->add(PE_PRINT_BADTARGET, sp);
			return PS_ERROR;
		}
	}
	if (sp.skip_token(SP_FACTOR, TT_CLASS, CT_CARD) == PS_NORMAL)
		mode = PM_SOME;
	if (sp.skip_token(SP_FACTOR, TT_FROM, PT_FROM) == PS_NORMAL)
	{
		if (sp.parseexp(False, True, &from) != PS_NORMAL)
		{
			MCperror->add(PE_PRINT_BADFROMEXP, sp);
			return PS_ERROR;
		}
		if (sp.skip_token(SP_FACTOR, TT_TO) != PS_NORMAL)
		{
			MCperror->add(PE_PRINT_NOTO, sp);
			return PS_ERROR;
		}
		if (sp.parseexp(False, True, &to) != PS_NORMAL)
		{
			MCperror->add(PE_PRINT_BADTOEXP, sp);
			return PS_ERROR;
		}
	}
	if (sp.skip_token(SP_FACTOR, TT_PREP, PT_INTO) == PS_NORMAL)
	{
		sp.skip_token(SP_FACTOR, TT_PROPERTY, P_RECTANGLE);
		if (sp.parseexp(False, True, &rect) != PS_NORMAL)
		{
			MCperror->add(PE_PRINT_BADRECTEXP, sp);
			return PS_ERROR;
		}
	}
	return PS_NORMAL;
}

bool MCPrint::evaluate_src_rect(MCExecContext& ctxt, MCPoint& r_from, MCPoint& r_to)
{
    MCPoint t_from, t_to;
    if (!ctxt . EvalExprAsPoint(from, EE_PRINT_CANTGETCOORD, t_from))
        return false;

    if (!ctxt . EvalExprAsPoint(to, EE_PRINT_CANTGETCOORD, t_to))
        return false;

    r_from = t_from;
    r_to = t_to;

    return true;
}

void MCPrint::exec_ctxt(MCExecContext &ctxt)
{
#ifdef /* MCPrint */ LEGACY_EXEC
	if (MCsecuremode & MC_SECUREMODE_PRINT)
	{
		MCeerror->add(EE_PRINT_NOPERM, line, pos);
		return ES_ERROR;
	}

	if (mode == PM_ANCHOR)
	{
		char *t_name;
		t_name = nil;
		if (from -> eval(ep) != ES_NORMAL)
		{
			MCeerror -> add(EE_PRINTANCHOR_BADNAME, line, pos);
			return ES_ERROR;
		}

		t_name = ep . getsvalue() . clone();

		int2 t_at_x, t_at_y;
		if (rect -> eval(ep) != ES_NORMAL)
		{
			MCeerror -> add(EE_PRINTANCHOR_BADLOCATION, line, pos);
			delete t_name;
			return ES_ERROR;
		}

		if (!MCU_stoi2x2(ep . getsvalue(), t_at_x, t_at_y))
		{
			MCeerror -> add(EE_PRINTANCHOR_LOCATIONNAP, line, pos, ep . getsvalue());
			delete t_name;
			return ES_ERROR;
		}

		MCprinter -> MakeAnchor(t_name, t_at_x, t_at_y);

		delete t_name;

		return ES_NORMAL;
	}
	else if (mode == PM_LINK || mode == PM_LINK_ANCHOR || mode == PM_LINK_URL)
	{
		MCPrinterLinkType t_type = kMCPrinterLinkUnspecified;
		if (mode == PM_LINK_ANCHOR)
			t_type = kMCPrinterLinkAnchor;
		else if (mode == PM_LINK_URL)
			t_type = kMCPrinterLinkURI;
		char *t_dest;
		t_dest = nil;
		if (to -> eval(ep) != ES_NORMAL)
		{
			MCeerror -> add(EE_PRINTLINK_BADDEST, line, pos);
			return ES_ERROR;
		}

		t_dest = ep . getsvalue() . clone();

		if (rect -> eval(ep) != ES_NORMAL)
		{
			MCeerror -> add(EE_PRINTLINK_BADAREA, line, pos);
			return ES_ERROR;
		}

		int2 i1, i2, i3, i4;
		if (!MCU_stoi2x4(ep . getsvalue(), i1, i2, i3, i4))
		{
			MCeerror->add(EE_PRINTLINK_AREANAR, line, pos, ep . getsvalue());
			return ES_ERROR;
		}

		MCRectangle t_area_rect;
		t_area_rect . x = i1;
		t_area_rect . y = i2;
		t_area_rect . width = MCU_max(i3 - i1, 1);
		t_area_rect . height = MCU_max(i4 - i2, 1);

		MCprinter -> MakeLink(t_dest, t_area_rect, t_type);

		delete t_dest;

		return ES_NORMAL;
	}
	else if (mode == PM_BOOKMARK || mode == PM_UNICODE_BOOKMARK)
	{
		char *t_title = nil;
		uint32_t t_level = 0;
		int16_t t_x = 0, t_y = 0;
		bool t_success = true;

		if (from->eval(ep) != ES_NORMAL)
		{
			MCeerror->add(EE_PRINTBOOKMARK_BADTITLE, line, pos, ep.getsvalue());
			t_success = false;
		}

		if (t_success)
		{
			if (mode == PM_UNICODE_BOOKMARK)
				ep.utf16toutf8();
			else
				ep.nativetoutf8();
			t_title = ep.getsvalue().clone();
		}

		if (t_success && to != NULL)
		{
			if (to->eval(ep) != ES_NORMAL ||
				ep.ton() != ES_NORMAL)
			{
				MCeerror->add(EE_PRINTBOOKMARK_BADLEVEL, line, pos, ep.getsvalue());
				t_success = false;
			}
			else
				t_level = ep.getint4();
		}

		if (t_success && rect != NULL)
		{
			if (rect->eval(ep) != ES_NORMAL ||
				!MCU_stoi2x2(ep.getsvalue(), t_x, t_y))
			{
				MCeerror->add(EE_PRINTBOOKMARK_BADAT, line, pos, ep.getsvalue());
				t_success = false;
			}
		}

		if (t_success && initial_state != NULL)
		{
			if (initial_state->eval(ep) != ES_NORMAL)
				t_success = false;
			if (t_success)
			{
				if (ep.getsvalue() == "closed")
					bookmark_closed = true;
				else if (ep.getsvalue() == "open")
					bookmark_closed = false;
				else
					t_success = false;
			}
			if (!t_success)
				MCeerror->add(EE_PRINTBOOKMARK_BADINITIAL, line, pos, ep.getsvalue());
		}

		if (t_success)
			MCprinter->MakeBookmark(t_title, t_x, t_y, t_level, bookmark_closed);
		delete t_title;

		return t_success ? ES_NORMAL : ES_ERROR;
	}

	MCObject *optr;
	uint4 parid;
	MCStack *stack = MCdefaultstackptr;
	
	MCCard *t_card;
	t_card  = NULL;
	
	uint2 count = 1;

	MCresult -> clear(False);
	
	// Syntax: print .. from .. to ..
	MCRectangle t_src_rect;
	bool t_src_rect_required;
	t_src_rect_required = true;
	if (from != NULL)
	{
		int2 lrx, lry;
		if (from -> eval(ep) != ES_NORMAL)
		{
			MCeerror -> add(EE_PRINT_CANTGETCOORD, line, pos);
			return ES_ERROR;
		}
		if (!MCU_stoi2x2(ep.getsvalue(), t_src_rect . x, t_src_rect . y))
		{
			MCeerror -> add(EE_PRINT_NAP, line, pos, ep.getsvalue());
			return ES_ERROR;
		}
		if (to -> eval(ep) != ES_NORMAL)
		{
			MCeerror -> add(EE_PRINT_CANTGETCOORD, line, pos);
			return ES_ERROR;
		}
		if (!MCU_stoi2x2(ep . getsvalue(), lrx, lry))
		{
			MCeerror -> add(EE_PRINT_NAP, line, pos, ep . getsvalue());
			return ES_ERROR;
		}
		// MJ: the old printing added 1 to both width and height below.
		// This indeed caused rectangles to look 1 pixel too large on either sides. Removing this solves this problem.
		t_src_rect . width = lrx - t_src_rect . x;//  + 1;
		t_src_rect . height = lry - t_src_rect . y; // + 1;
		
		t_src_rect_required = false;
	}
	//else the source rectangle will be the rect of the target.
	
	// Syntax: print .. into ..
	MCRectangle t_dst_rect;
	bool t_dst_rect_defined;
	t_dst_rect_defined = false;
	if (rect != NULL)
	{
		if (rect -> eval(ep) != ES_NORMAL)
		{
			MCeerror -> add(EE_PRINT_CANTGETRECT, line, pos);
			return ES_ERROR;
		}
		int2 i1, i2, i3, i4;
		if (!MCU_stoi2x4(ep . getsvalue(), i1, i2, i3, i4))
		{
			MCeerror->add(EE_PRINT_NAR, line, pos, ep . getsvalue());
			return ES_ERROR;
		}
		t_dst_rect . x = i1;
		t_dst_rect . y = i2;
		t_dst_rect . width = MCU_max(i3 - i1, 1);
		t_dst_rect . height = MCU_max(i4 - i2, 1);
		
		t_dst_rect_defined = true; // could use rect != NULL or whatever..
	}
	
	switch (mode)
	{
	case PM_ALL:
	case PM_MARKED:
		if (target != NULL)
		{
			if (target -> getobj(ep, optr, parid, True) != ES_NORMAL || optr -> gettype() != CT_STACK)
			{
				MCeerror -> add(EE_PRINT_NOTARGET, line, pos);
				return ES_ERROR;
			}
			stack = (MCStack *)optr;
		}
	break;
		
	case PM_SOME:
		if (target -> eval(ep) != ES_NORMAL || ep . ton() != ES_NORMAL)
		{
			MCeerror -> add(EE_PRINT_CANTGETCOUNT, line, pos);
			return ES_ERROR;
		}
		count = ep . getuint2();
	break;
		
	case PM_CARD:
		if (target != NULL)
		{
			if (target -> getobj(ep, optr, parid, True) != ES_NORMAL)
			{
				MCeerror -> add(EE_PRINT_NOTARGET, line, pos);
				return ES_ERROR;
			}
			if (optr->gettype() == CT_CARD)
			{
				t_card = (MCCard *)optr;
				stack = t_card -> getstack();
			}
			else
			{
				if (optr -> gettype() != CT_STACK)
				{
					MCeerror -> add(EE_PRINT_NOTACARD, line, pos);
					return ES_ERROR;
				}
				mode = PM_ALL;
				stack = (MCStack *)optr;
			}
		}
	break;
		
	default:
	break;
	}
	
	if (stack -> getopened() == 0)
	{
		MCeerror -> add(EE_PRINT_NOTOPEN, line, pos);
		return ES_ERROR;
	}
	
	MCU_watchcursor(NULL, True);
	
	Exec_stat t_exec_stat;
	t_exec_stat = ES_NORMAL;
	switch (mode)
	{
	case PM_BREAK: // syntax: print break
		MCprinter -> Break();
	break;
	
	case PM_MARKED:
		MCprinter -> LayoutStack(stack, true, t_src_rect_required ? NULL : &t_src_rect); // extend this with a stack pointer
	break;
	
	case PM_ALL:
		MCprinter -> LayoutStack(stack, false, t_src_rect_required ? NULL : &t_src_rect);
	break;
	
	case PM_CARD:
	{
		// If no t_card specified, then we get the current t_card of the default stack.
		if (t_card == NULL)
			t_card = stack -> getcurcard();
		
		if (t_src_rect_required)
			t_src_rect = t_card -> getrect();
			
		if (t_dst_rect_defined) // we have a destination rect, this is where it will go
			MCprinter -> Render(t_card, t_src_rect, t_dst_rect);
		else // the card is laid out in the current layout
			MCprinter -> LayoutCard(t_card, &t_src_rect);
	}
	break;
	
	case PM_SOME:
	{
		if (t_src_rect_required)
			t_src_rect = MCdefaultstackptr -> getcurcard() -> getrect();
		
		MCprinter -> LayoutCardSequence(MCdefaultstackptr, count, &t_src_rect);
	}
	break;
	
	default:
		// What do we do here? Could get here if mode == PM_UNDEFINED, however, for now
		// let's just put an assertion here.
		assert(false);
	break;
	}
	
	// MW-2007-12-17: [[ Bug 266 ]] The watch cursor must be reset before we
	//   return back to the caller.
	MCU_unwatchcursor(ep.getobj()->getstack(), True);
	
	return t_exec_stat;
#endif /* MCPrint */

	if (mode == PM_ANCHOR)
    {
        MCAutoStringRef t_name;
        if (!ctxt . EvalExprAsStringRef(from, EE_DO_BADEXP, &t_name))
            return;
		
		MCPoint t_location;
        if (!ctxt . EvalExprAsPoint(rect, EE_PRINTANCHOR_BADLOCATION, t_location))
            return;

		MCPrintingExecPrintAnchor(ctxt, *t_name, t_location);
	}
	else if (mode == PM_LINK || mode == PM_LINK_ANCHOR || mode == PM_LINK_URL)
	{
		MCPrinterLinkType t_type;
		if (mode == PM_LINK_ANCHOR)
			t_type = kMCPrinterLinkAnchor;
		else if (mode == PM_LINK_URL)
			t_type = kMCPrinterLinkURI;
		else
            t_type = kMCPrinterLinkUnspecified;

		MCAutoStringRef t_target;
        if (!ctxt . EvalExprAsStringRef(to, EE_PRINTLINK_BADDEST, &t_target))
            return;

		MCRectangle t_area;
        if (!ctxt . EvalExprAsRectangle(rect, EE_PRINTLINK_BADAREA, t_area))
            return;

		MCPrintingExecPrintLink(ctxt, (int)t_type, *t_target, t_area);
	}
	else if (mode == PM_BOOKMARK || mode == PM_UNICODE_BOOKMARK)
    {
		MCAutoValueRef t_title;
        if (mode != PM_UNICODE_BOOKMARK)
        {
            if (!ctxt . EvalExprAsStringRef(from, EE_PRINTBOOKMARK_BADTITLE, (MCStringRef&)&t_title))
                return;
        }
        else
        {
            if (!ctxt . EvalExprAsDataRef(from, EE_PRINTBOOKMARK_BADTITLE, (MCDataRef&)&t_title))
                return;
        }

		uint32_t t_level;
        if (!ctxt . EvalOptionalExprAsUInt(to, 0, EE_PRINTBOOKMARK_BADLEVEL, t_level))
            return;

		MCPoint t_location;
        t_location . x = t_location . y = 0;

        MCPoint *t_location_ptr;
        t_location_ptr = &t_location;
        if (!ctxt . EvalOptionalExprAsPoint(rect, t_location_ptr, EE_PRINTBOOKMARK_BADAT, t_location_ptr))
            return;

		bool t_initially_closed;
		t_initially_closed = false;
		if (initial_state != nil)
		{
            MCAutoStringRef t_state;
            if (!ctxt . EvalExprAsStringRef(initial_state, EE_PRINTBOOKMARK_BADINITIAL, &t_state))
                return;

            if (MCStringIsEqualToCString(*t_state, "closed", kMCCompareExact))
				t_initially_closed = true;
            else if (MCStringIsEqualToCString(*t_state, "open", kMCCompareExact))
				t_initially_closed = false;
			else
			{
                ctxt . LegacyThrow(EE_PRINTBOOKMARK_BADINITIAL);
                return;
			}
		}
		else
			t_initially_closed = bookmark_closed;


		if (mode != PM_UNICODE_BOOKMARK)
            MCPrintingExecPrintBookmark(ctxt, (MCStringRef)*t_title, t_location, t_level, t_initially_closed);
		else
            MCPrintingExecPrintUnicodeBookmark(ctxt, (MCDataRef)*t_title, t_location, t_level, t_initially_closed);
	}
	else if (mode == PM_BREAK)
	{
		MCPrintingExecPrintBreak(ctxt);
	}
	else if (mode == PM_MARKED || mode == PM_ALL)
	{
		MCStack *t_stack;
		t_stack = nil;
		if (target != nil)
		{
			MCObject *optr;
			uint32_t parid;
            if (!target -> getobj(ctxt, optr, parid, True)
                    || optr -> gettype() != CT_STACK)
			{
                ctxt . LegacyThrow(EE_PRINT_NOTARGET);
                return;
			}

			t_stack = (MCStack *)optr;
		}

		MCPoint t_area_from, t_area_to;
		if (from != nil &&
                !evaluate_src_rect(ctxt, t_area_from, t_area_to))
            return;

		if (from == nil)
			MCPrintingExecPrintAllCards(ctxt, t_stack, mode == PM_MARKED);
		else
			MCPrintingExecPrintRectOfAllCards(ctxt, t_stack, mode == PM_MARKED, t_area_from, t_area_to);
	}
	else if (mode == PM_CARD)
	{
		MCObject *t_object;
		if (target != NULL)
		{
			uint32_t parid;
            if (!target -> getobj(ctxt, t_object, parid, True))
			{
                ctxt . LegacyThrow(EE_PRINT_NOTARGET);
                return;
			}
			if (t_object -> gettype() != CT_CARD && t_object -> gettype() != CT_STACK)
			{
                ctxt . LegacyThrow(EE_PRINT_NOTACARD);
                return;
			}
		}
		else
			t_object = MCdefaultstackptr -> getcurcard();

		MCPoint t_from, t_to;
		if (from != nil &&
                !evaluate_src_rect(ctxt, t_from, t_to))
            return;

		if (t_object -> gettype() == CT_STACK)
		{
			if (from == nil)
				MCPrintingExecPrintAllCards(ctxt, (MCStack *)t_object, mode == PM_MARKED);
			else
				MCPrintingExecPrintRectOfAllCards(ctxt, (MCStack *)t_object, mode == PM_MARKED, t_from, t_to);
		}
		else if (rect == nil)
		{
			if (from == nil)
				MCPrintingExecPrintCard(ctxt, (MCCard *)t_object);
			else
				MCPrintingExecPrintRectOfCard(ctxt, (MCCard *)t_object, t_from, t_to);
		}
		else
		{
            MCRectangle t_dst_area;
            if (!ctxt . EvalExprAsRectangle(rect, EE_PRINT_CANTGETRECT, t_dst_area))
                return;

			if (from == nil)
				MCPrintingExecPrintCardIntoRect(ctxt, (MCCard *)t_object, t_dst_area);
			else
				MCPrintingExecPrintRectOfCardIntoRect(ctxt, (MCCard *)t_object, t_from, t_to, t_dst_area);
		}
	}
	else if (mode == PM_SOME)
	{
        integer_t t_count;
        if (!ctxt . EvalExprAsInt(target, EE_PRINT_CANTGETCOUNT, t_count))
            return;

		MCPoint t_from, t_to;
		if (from != nil &&
            !evaluate_src_rect(ctxt, t_from, t_to))
            return;

		if (from == nil)
			MCPrintingExecPrintSomeCards(ctxt, t_count);
		else
			MCPrintingExecPrintRectOfSomeCards(ctxt, t_count, t_from, t_to);
    }
}

void MCPrint::compile(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginStatement(ctxt, line, pos);
	
	if (mode == PM_ANCHOR)
	{		
		from -> compile(ctxt); // anchor name (string)
		rect -> compile(ctxt); // anchor location (point)
		
		MCSyntaxFactoryExecMethod(ctxt, kMCPrintingExecPrintAnchorMethodInfo);
	}
	else if (mode == PM_LINK || mode == PM_LINK_ANCHOR || mode == PM_LINK_URL)
	{
		MCPrinterLinkType t_type;
		if (mode == PM_LINK_ANCHOR)
			t_type = kMCPrinterLinkAnchor;
		else if (mode == PM_LINK_URL)
			t_type = kMCPrinterLinkURI;
		else
			t_type = kMCPrinterLinkUnspecified;
		
		MCSyntaxFactoryEvalConstantEnum(ctxt, kMCPrintingPrinterLinkTypeInfo, (int)t_type); // link type (MCPrinterLinkType)
		
		to -> compile(ctxt); // link target (string)
		
		rect -> compile(ctxt); // link area (rectangle)
		
		MCSyntaxFactoryExecMethod(ctxt, kMCPrintingExecPrintLinkMethodInfo);
	}
	else if (mode == PM_BOOKMARK || mode == PM_UNICODE_BOOKMARK)
	{
		from -> compile(ctxt); // bookmark title (string)
		
		if (rect != nil)
			rect -> compile(ctxt); // bookmark location (point)
		else
			MCSyntaxFactoryEvalConstantLegacyPoint(ctxt, MCPointMake(0, 0)); // bookmark location (point - 0,0)
		
		if (to != nil)
			to -> compile(ctxt);
		else
			MCSyntaxFactoryEvalConstantUInt(ctxt, 0);
		
		if (initial_state != nil)
			initial_state -> compile(ctxt); // bookmark initial state (MCPrinterBookmarkInitialState)
		else
			MCSyntaxFactoryEvalConstantEnum(ctxt, kMCPrintingPrinterBookmarkInitialStateTypeInfo, bookmark_closed ? 1 : 0); // bookmark initial state 
		
		MCSyntaxFactoryExecMethod(ctxt, mode != PM_UNICODE_BOOKMARK ? kMCPrintingExecPrintNativeBookmarkMethodInfo : kMCPrintingExecPrintUnicodeBookmarkMethodInfo);
	}
	else if (mode == PM_BREAK)
	{
		MCSyntaxFactoryExecMethod(ctxt, kMCPrintingExecPrintBreakMethodInfo);
	}
	else if (mode == PM_MARKED || mode == PM_ALL)
	{
		if (target != nil)
			target -> compile_object_ptr(ctxt);
		else
			MCSyntaxFactoryEvalConstantNil(ctxt);
		
		MCSyntaxFactoryEvalConstantBool(ctxt, mode == PM_MARKED);
		
		if (from != nil)
		{
			from -> compile(ctxt);
			to -> compile(ctxt);
			MCSyntaxFactoryExecMethod(ctxt, kMCPrintingExecPrintRectOfAllCardsMethodInfo);
		}
		else
			MCSyntaxFactoryExecMethod(ctxt, kMCPrintingExecPrintAllCardsMethodInfo);
	}
	else if (mode == PM_CARD)
	{
		if (target != nil)
			target -> compile_object_ptr(ctxt);
		else
			MCSyntaxFactoryEvalConstantNil(ctxt);
		
		if (rect == nil)
		{
			if (from != nil)
			{
				MCSyntaxFactoryEvalConstantBool(ctxt, mode == PM_MARKED);
				from -> compile(ctxt);
				to -> compile(ctxt);
				
				if (mode == PM_MARKED)
					MCSyntaxFactoryExecMethod(ctxt, kMCPrintingExecPrintRectOfAllCardsMethodInfo);
				else
				{
					MCSyntaxFactoryExecMethod(ctxt, kMCPrintingExecPrintRectOfAllCardsMethodInfo);
					MCSyntaxFactoryExecMethodWithArgs(ctxt, kMCPrintingExecPrintRectOfCardMethodInfo, 0, 2, 3);
				}
			}
			else
			{
				MCSyntaxFactoryEvalConstantBool(ctxt, mode == PM_MARKED);
				
				if (mode == PM_MARKED)
					MCSyntaxFactoryExecMethod(ctxt, kMCPrintingExecPrintAllCardsMethodInfo);
				else
				{
					MCSyntaxFactoryExecMethod(ctxt, kMCPrintingExecPrintAllCardsMethodInfo);
					MCSyntaxFactoryExecMethodWithArgs(ctxt, kMCPrintingExecPrintCardMethodInfo, 0);
				}
			}
		}
		else
		{
			if (from != nil)
			{
				from -> compile(ctxt);
				to -> compile(ctxt);
			}
			
			rect -> compile(ctxt);
			
			if (from != nil)
				MCSyntaxFactoryExecMethod(ctxt, kMCPrintingExecPrintCardIntoRectMethodInfo);
			else
				MCSyntaxFactoryExecMethod(ctxt, kMCPrintingExecPrintRectOfCardIntoRectMethodInfo);
		}
	}
	else if (mode == PM_SOME)
	{
		target -> compile(ctxt);
		if (from != nil)
		{
			from -> compile(ctxt);
			to -> compile(ctxt);

			MCSyntaxFactoryExecMethod(ctxt, kMCPrintingExecPrintRectOfSomeCardsMethodInfo);
		}
		else
			MCSyntaxFactoryExecMethod(ctxt, kMCPrintingExecPrintSomeCardsMethodInfo);
	}
	
	MCSyntaxFactoryEndStatement(ctxt);
}

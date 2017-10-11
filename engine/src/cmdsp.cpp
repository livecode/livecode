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
				target = new (nothrow) MCChunk(False);
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
		target = new (nothrow) MCChunk(False);
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

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

#include "uidc.h"

#include "hndlrlst.h"
#include "handler.h"
#include "scriptpt.h"
#include "statemnt.h"
#include "object.h"
#include "chunk.h"
#include "param.h"
#include "mcerror.h"
#include "debug.h"
#include "util.h"
#include "parentscript.h"

#include "globals.h"

#include "redraw.h"

////////////////////////////////////////////////////////////////////////////////

MCStatement::MCStatement()
{
	line = pos = 0;
	next = NULL;
}

MCStatement::~MCStatement()
{}

Parse_stat MCStatement::parse(MCScriptPoint &sp)
{
	initpoint(sp);
	return PS_NORMAL;
}

void MCStatement::exec_ctxt(MCExecContext&)
{
	fprintf(stderr, "ERROR: exec method for statement not implemented properly\n");
}

uint4 MCStatement::linecount()
{
	return 1;
}

uint4 MCStatement::countlines(MCStatement *stmp)
{
	uint4 count = 0;
	while (stmp != NULL)
	{
		count += stmp->linecount();
		stmp = stmp->getnext();
	}
	return count;
}

void MCStatement::deletestatements(MCStatement *start)
{
	MCStatement *stmp;
	while (start != NULL)
	{
		stmp = start;
		start = start->getnext();
		delete stmp;
	}
}

void MCStatement::deletetargets(MCChunk **targets)
{
	MCChunk *t1ptr = *targets;
	*targets = NULL;
	while (t1ptr != NULL)
	{
		MCChunk *t2ptr = t1ptr;
		t1ptr = t1ptr->next;
		delete t2ptr;
	}
}

Parse_stat MCStatement::gettargets(MCScriptPoint &sp, MCChunk **targets,
                                   Boolean forset)
{
	Boolean needchunk = False;
	MCChunk *pptr = NULL;
	while (True)
	{
		if (sp.skip_token(SP_COMMAND, TT_ELSE, S_UNDEFINED) == PS_NORMAL)
		{
			sp.backup();
			return PS_NORMAL;
		}
		Symbol_type type;
		switch (sp.next(type))
		{
		case PS_NORMAL:
			sp.backup();
			break;
		case PS_ERROR:
			return PS_ERROR;
		case PS_EOL:
		case PS_EOF:
			if (needchunk)
			{
				MCperror->add(PE_STATEMENT_BADCHUNK, sp);
				return PS_ERROR;
			}
			return PS_NORMAL;
		default:
			sp.backup();
			return PS_NORMAL;
		}
		MCChunk *newptr = new (nothrow) MCChunk(forset);
		if (newptr->parse(sp, False) != PS_NORMAL)
		{
			delete newptr;
			MCperror->add(PE_STATEMENT_BADCHUNK, sp);
			return PS_ERROR;
		}
		if (pptr == NULL)
			*targets = pptr = newptr;
		else
		{
			pptr->next = newptr;
			pptr = pptr->next;
		}
		if (sp.skip_token(SP_COMMAND, TT_ELSE, S_UNDEFINED) == PS_NORMAL
		        || sp.skip_token(SP_FACTOR, TT_TO, PT_TO) == PS_NORMAL)
		{
			sp.backup();
			return PS_NORMAL;
		}
		switch (sp.skip_token(SP_FACTOR, TT_BINOP, O_AND))
		{
		case PS_NORMAL:
			break;
		case PS_EOL:
		case PS_EOF:
			return PS_NORMAL;
		default:
			MCperror->add(PE_STATEMENT_NOTAND, sp);
			return PS_ERROR;
		}
		needchunk = True;
	}
	return PS_NORMAL;
}

Parse_stat MCStatement::getparams(MCScriptPoint &sp, MCParameter **params)
{
	Boolean needparam = False;
	MCParameter *pptr = NULL;
	while (True)
	{
		if (sp.skip_token(SP_COMMAND, TT_ELSE, S_UNDEFINED) == PS_NORMAL)
		{
			sp.backup();
			return PS_NORMAL;
		}
		Symbol_type type;
		switch (sp.next(type))
		{
		case PS_NORMAL:
			sp.backup();
			break;
		case PS_ERROR:
			return PS_ERROR;
		case PS_EOL:
		case PS_EOF:
			if (needparam)
			{
				MCperror->add(PE_STATEMENT_BADPARAM, sp);
				return PS_ERROR;
			}
			return PS_NORMAL;
		default:
			sp.backup();
			return PS_NORMAL;
		}
		MCParameter *newptr = new (nothrow) MCParameter;
		if (newptr->parse(sp) != PS_NORMAL)
		{
			delete newptr;
			MCperror->add(PE_STATEMENT_BADPARAM, sp);
			return PS_ERROR;
		}
		if (pptr == NULL)
			*params = pptr = newptr;
		else
		{
			pptr->setnext(newptr);
			pptr = newptr;
		}
		if (sp.skip_token(SP_COMMAND, TT_ELSE, S_UNDEFINED) == PS_NORMAL)
		{
			sp.backup();
			return PS_NORMAL;
		}
		switch (sp.next(type))
		{
		case PS_NORMAL:
			break;
		case PS_EOL:
		case PS_EOF:
			return PS_NORMAL;
		default:
			MCperror->add(PE_STATEMENT_NOTSEP, sp);
			return PS_ERROR;
		}
		if (type != ST_SEP)
		{
			MCperror->add(PE_STATEMENT_BADSEP, sp);
			return PS_ERROR;
		}
		needparam = True;
	}
	return PS_NORMAL;
}

Parse_stat MCStatement::getmods(MCScriptPoint &sp, uint2 &mstate)
{
	Symbol_type type;
	const LT *te;

	while (True)
	{
		if (sp.next(type) != PS_NORMAL)
		{
			MCperror->add(PE_STATEMENT_NOKEY, sp);
			return PS_ERROR;
		}
		if (sp.lookup(SP_FACTOR, te) != PS_NORMAL || te->type != TT_FUNCTION)
		{
			MCperror->add(PE_STATEMENT_BADKEY, sp);
			return PS_ERROR;
		}
		switch (te->which)
		{
		case F_COMMAND_KEY:
			mstate |= MS_CONTROL;
			break;
		case F_CONTROL_KEY:
			mstate |= MS_MAC_CONTROL;
			break;
		case F_OPTION_KEY:
			mstate |= MS_MOD1;
			break;
		case F_SHIFT_KEY:
			mstate |= MS_SHIFT;
			break;
		default:
			MCperror->add(PE_STATEMENT_BADKEY, sp);
			return PS_ERROR;
		}
		if (sp.skip_token(SP_COMMAND, TT_ELSE, S_UNDEFINED) == PS_NORMAL)
		{
			sp.backup();
			return PS_NORMAL;
		}
		switch (sp.next(type))
		{
		case PS_NORMAL:
			break;
		case PS_EOL:
		case PS_EOF:
			return PS_NORMAL;
		default:
			MCperror->add(PE_STATEMENT_BADSEP, sp);
			return PS_ERROR;
		}
		if (type != ST_SEP)
		{
			MCperror->add(PE_STATEMENT_BADSEP, sp);
			return PS_ERROR;
		}
	}
	return PS_NORMAL;
}

Parse_stat MCStatement::gettime(MCScriptPoint &sp, MCExpression **in,
                                Functions &units)
{
	if (sp.skip_token(SP_FACTOR, TT_IN) == PS_NORMAL)
	{
		if (sp.parseexp(False, True, in) != PS_NORMAL)
		{
			MCperror->add
			(PE_STATEMENT_BADINEXP, sp);
			return PS_ERROR;
		}
		if (sp.skip_token(SP_FACTOR, TT_FUNCTION, F_MILLISECS) == PS_NORMAL)
			units = F_MILLISECS;
		else
			if (sp.skip_token(SP_FACTOR, TT_FUNCTION, F_SECONDS) == PS_NORMAL
			        || sp.skip_token(SP_FACTOR, TT_CHUNK, CT_SECOND) == PS_NORMAL)
				units = F_SECONDS;
			else
				if (sp.skip_token(SP_FACTOR, TT_FUNCTION, F_TICKS) == PS_NORMAL)
					units = F_TICKS;
				else
					units = F_TICKS;
	}
	return PS_NORMAL;
}

void MCStatement::initpoint(MCScriptPoint &sp)
{
	line = sp.getline();
	pos = sp.getpos();
}

////////////////////////////////////////////////////////////////////////////////

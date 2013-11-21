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

#include "scriptpt.h"
#include "execpt.h"
#include "exec.h"
#include "debug.h"
#include "hndlrlst.h"
#include "handler.h"
#include "keywords.h"
#include "mcerror.h"
#include "util.h"
#include "newobj.h"
#include "cmds.h"
#include "redraw.h"
#include "variable.h"

#include "globals.h"

Parse_stat MCGlobal::parse(MCScriptPoint &sp)
{
	initpoint(sp);
	while (True)
	{
		Symbol_type type;
		Parse_stat stat = sp.next(type);
		if (stat == PS_EOL)
			return PS_NORMAL;
		const LT *te;
		MCExpression *newfact = NULL;
		if (stat != PS_NORMAL || type != ST_ID
		        || sp.lookup(SP_FACTOR, te) != PS_NO_MATCH
		        || sp.lookupconstant(&newfact) == PS_NORMAL)
		{
			delete newfact;
			MCperror->add
			(PE_GLOBAL_BADNAME, sp);
			return PS_ERROR;
		}
		if (sp.gethandler() == NULL)
			sp.gethlist()->newglobal(sp.gettoken_nameref());
		else
			sp.gethandler()->newglobal(sp.gettoken_nameref());
		switch (sp.next(type))
		{
		case PS_NORMAL:
			if (type != ST_SEP)
			{
				MCperror->add
				(PE_STATEMENT_NOTSEP, sp);
				return PS_ERROR;
			}
			break;
		case PS_EOL:
		case PS_EOF:
			return PS_NORMAL;
		default:
			MCperror->add
			(PE_STATEMENT_NOTSEP, sp);
			return PS_ERROR;
		}
	}
	return PS_NORMAL;
}

Parse_stat MCLocaltoken::parse(MCScriptPoint &sp)
{
	initpoint(sp);
	while (True)
	{
		Symbol_type type;
		Parse_stat stat = sp.next(type);
		if (stat == PS_EOL)
			return PS_NORMAL;
		const LT *te;
		MCExpression *newfact = NULL;
		if (stat != PS_NORMAL || type != ST_ID
		        || sp.lookup(SP_FACTOR, te) != PS_NO_MATCH
		        || sp.lookupconstant(&newfact) == PS_NORMAL)
		{
			delete newfact;
			MCperror->add(PE_LOCAL_BADNAME, sp);
			return PS_ERROR;
		}

		MCAutoNameRef t_token_name;
		/* UNCHECKED */ t_token_name . Clone(sp . gettoken_nameref());

		// MW-2013-11-08: [[ RefactorIt ]] The 'it' variable is always present now,
		//   so there's no need to 'local it'. However, scripts do contain this so
		//   don't do a check for an existing var in this case.
		if (!MCNameIsEqualTo(t_token_name, MCN_it, kMCCompareCaseless))
		{
			MCExpression *e = NULL;
			MCVarref *v = NULL;
			if (sp.gethandler() == NULL)
				if (constant)
					sp.gethlist()->findconstant(t_token_name, &e);
				else
					sp.gethlist()->findvar(t_token_name, false, &v);
			else
				if (constant)
					sp.gethandler()->findconstant(t_token_name, &e);
				else
					sp.gethandler()->findvar(t_token_name, &v);
			if (e != NULL || v != NULL)
			{
				MCperror->add(PE_LOCAL_SHADOW, sp);
				delete v;
				delete e;
				return PS_ERROR;
			}
		}

		MCVariable *tmp;
		for (tmp = MCglobals ; tmp != NULL ; tmp = tmp->getnext())
			if (tmp -> hasname(t_token_name))
				if (MCexplicitvariables)
				{
					MCperror->add(PE_LOCAL_SHADOW, sp);
					return PS_ERROR;
				}

		MCVarref *tvar = NULL;
		MCAutoStringRef init;
		bool initialised = false;
		if (sp.skip_token(SP_FACTOR, TT_BINOP, O_EQ) == PS_NORMAL)
		{
			if (sp.next(type) != PS_NORMAL || MCexplicitvariables && type != ST_LIT
			        && type != ST_NUM)
			{
				if (constant)
					MCperror->add(PE_CONSTANT_BADINIT, sp);
				else
					MCperror->add(PE_LOCAL_BADINIT, sp);
				return PS_ERROR;
			}
			if (type == ST_MIN)
			{ // negative initializer
				if (sp.next(type) != PS_NORMAL || type != ST_NUM)
				{
					if (constant)
						MCperror->add(PE_CONSTANT_BADINIT, sp);
					else
						MCperror->add(PE_LOCAL_BADINIT, sp);
					return PS_ERROR;
				}
				/* UNCHECKED */ MCStringFormat(&init, "-%@", sp.gettoken_stringref());
			}
			else
				init = sp.gettoken_stringref();

			initialised = true;
		}
		else if (constant)
			{
				MCperror->add(PE_CONSTANT_NOINIT, sp);
				return PS_ERROR;
			}

		MCNewAutoNameRef t_init_name;
		if (initialised)
			/* UNCHECKED */ MCNameCreate(*init, &t_init_name);
		else
			t_init_name = kMCEmptyName;

		if (sp.gethandler() == NULL)
		{
			if (constant)
				sp.gethlist()->newconstant(t_token_name, *t_init_name);
			else if (sp.gethlist()->newvar(t_token_name, *t_init_name, &tvar, initialised) != PS_NORMAL)
				{
					MCperror->add(PE_LOCAL_BADNAME, sp);
					return PS_ERROR;
				}

		}
		else if (constant)
			sp.gethandler()->newconstant(t_token_name, *t_init_name);
		else if (sp.gethandler()->newvar(t_token_name, *t_init_name, &tvar) != PS_NORMAL)
				{
					MCperror->add(PE_LOCAL_BADNAME, sp);
					return PS_ERROR;
				}

		delete tvar;

		switch (sp.next(type))
		{
		case PS_NORMAL:
			if (type != ST_SEP)
			{
				MCperror->add(PE_STATEMENT_NOTSEP, sp);
				return PS_ERROR;
			}
			break;
		case PS_EOL:
		case PS_EOF:
			return PS_NORMAL;
		default:
			MCperror->add(PE_STATEMENT_NOTSEP, sp);
			return PS_ERROR;
		}
	}
	return PS_NORMAL;
}

MCIf::~MCIf()
{
	delete cond;
	deletestatements(thenstatements);
	deletestatements(elsestatements);
}

Parse_stat MCIf::parse(MCScriptPoint &sp)
{
	initpoint(sp);
	if (sp.parseexp(False, True, &cond) != PS_NORMAL)
	{
		MCperror->add
		(PE_IF_BADCONDITION, sp);
		return PS_ERROR;
	}

	Symbol_type type;
	const LT *te;
	MCStatement *curstatement = NULL;
	MCStatement *newstatement = NULL;
	If_state state = IS_UNDEFINED;
	If_format format = IF_UNDEFINED;
	Boolean needstatement = False;

	while (True)
	{
		switch (sp.next(type))
		{
		case PS_NORMAL:
			if (type == ST_DATA || sp.lookup(SP_COMMAND, te) != PS_NORMAL)
			{
				if (needstatement)
				{
					if (type == ST_ID)
						newstatement = new MCComref(sp.gettoken_nameref());
					else if (type == ST_DATA)
						newstatement = new MCEcho;
					else
					{
						MCperror->add(PE_IF_NOTCOMMAND, sp);
						return PS_ERROR;
					}
				}
				else
				{
					if (state == IS_UNDEFINED)
					{
						MCperror->add(PE_IF_NOTHEN, sp);
						return PS_ERROR;
					}
					sp.backup();
					return PS_NORMAL;
				}
			}
			else
			{
				switch (te->type)
				{
				case TT_STATEMENT:
					if (needstatement)
						newstatement = MCN_new_statement(te->which);
					else
					{
						if (state == IS_UNDEFINED)
						{
							MCperror->add(PE_IF_NOTHEN, sp);
							return PS_ERROR;
						}
						sp.backup();
						return PS_NORMAL;
					}
					break;
				case TT_THEN:
					state = IS_THEN;
					needstatement = True;
					continue;
				case TT_ELSE:
					state = IS_ELSE;
					needstatement = True;
					continue;
				case TT_END:
					if (needstatement)
					{
						if (sp.skip_token(SP_COMMAND, TT_STATEMENT, S_IF) != PS_NORMAL)
						{
							MCperror->add(PE_IF_WANTEDENDIF, sp);
							return PS_ERROR;
						}
					}
					else
						sp.backup();
					return PS_NORMAL;
				default: /* token type */
					if (needstatement)
					{
						MCperror->add(PE_IF_WANTEDENDIF, sp);
						return PS_ERROR;
					}
					sp.backup();
					return PS_NORMAL;
				}
			}
			break;
		case PS_EOL:
			switch (format)
			{
			case IF_UNDEFINED:
				if (state == IS_THEN)
					format = IF_MULTIPLE;
				else
					format = IF_SINGLE;
				break;
			case IF_ONELINE:
				if (state == IS_ELSE)
					return PS_NORMAL;
				format = IF_SINGLE;
				needstatement = False;
				break;
			case IF_SINGLE:
				if (state == IS_ELSE)
					format = IF_ELSEMULTIPLE;
				else
					format = IF_MULTIPLE;
				break;
			case IF_MULTIPLE:
				if (state == IS_ELSE)
					format = IF_ELSEMULTIPLE;
				break;
			case IF_ELSEMULTIPLE:
				break;
			}
			if (sp.skip_eol() != PS_NORMAL)
			{
				MCperror->add(PE_IF_BADEOL, sp);
				return PS_ERROR;
			}
			continue;
		case PS_EOF:
			return PS_NORMAL;
		default:
			MCperror->add(PE_IF_BADTYPE, sp);
			return PS_ERROR;
		}
		if (newstatement->parse(sp) != PS_NORMAL)
		{
			MCperror->add(PE_IF_BADSTATEMENT, sp);
			delete newstatement;
			return PS_ERROR;
		}
		switch (state)
		{
		case IS_THEN:
			if (thenstatements == NULL)
				thenstatements = curstatement = newstatement;
			else
			{
				curstatement->setnext(newstatement);
				curstatement = newstatement;
			}
			switch (format)
			{
			case IF_UNDEFINED:
				format = IF_ONELINE;
				needstatement = False;
				break;
			case IF_SINGLE:
				needstatement = False;
				break;
			case IF_MULTIPLE:
				break;
			default: // may be unreachable
				return PS_ERROR;
			}
			break;
		case IS_ELSE:
			if (elsestatements == NULL)
				elsestatements = curstatement = newstatement;
			else
			{
				curstatement->setnext(newstatement);
				curstatement = newstatement;
			}
			if (format != IF_ELSEMULTIPLE)
				return PS_NORMAL;
			break;
		case IS_UNDEFINED:
			MCperror->add(PE_IF_NOTHEN, sp);
			delete newstatement;
			return PS_ERROR;
		}
	}
	return PS_NORMAL;
}
/*
Exec_stat MCIf::exec(MCExecPoint &ep)
{
	MCExecContext ctxt(ep);
	Exec_stat stat;
	while ((stat = cond->eval(ep)) != ES_NORMAL && (MCtrace || MCnbreakpoints)
	        && !MCtrylock && !MClockerrors)
		MCB_error(ctxt, getline(), getpos(), EE_IF_BADCOND);
	if (stat != ES_NORMAL)
	{
		MCeerror->add
		(EE_IF_BADCOND, line, pos);
		return ES_ERROR;
	}

	Boolean then = ep.getsvalue() == MCtruemcstring;
	MCStatement *tspr;
	if (then)
		tspr = thenstatements;
	else
		tspr = elsestatements;

	while (tspr != NULL)
	{
		if (MCtrace || MCnbreakpoints)
		{
			MCB_trace(ctxt, tspr->getline(), tspr->getpos());
			if (MCexitall)
				break;
		}
		ep.setline(tspr->getline());
		
		stat = tspr->exec(ep);
		
		// MW-2011-08-17: [[ Redraw ]] Flush any screen updates.
		MCRedrawUpdateScreen();

		switch(stat)
		{
		case ES_NORMAL:
			if (MCexitall)
				return ES_NORMAL;
			tspr = tspr->getnext();
			break;
		case ES_ERROR:
			if ((MCtrace || MCnbreakpoints) && !MCtrylock && !MClockerrors)
				do
				{
					MCB_error(ctxt, tspr->getline(), tspr->getpos(), EE_IF_BADSTATEMENT);
				}
				while (MCtrace && (stat = tspr->exec(ep)) != ES_NORMAL);
			if (stat == ES_ERROR)
				if (MCexitall)
					return ES_NORMAL;
				else
				{
					MCeerror->add(EE_IF_BADSTATEMENT, line, pos);
					return ES_ERROR;
				}
			else
				tspr = tspr->getnext();
			break;
		default:
			return stat;
		}
	}
	return ES_NORMAL;
}
*/

void MCIf::exec_ctxt(MCExecContext &ctxt)
{
	MCExecPoint &ep = ctxt . GetEP();
	Exec_stat stat;
	while ((stat = cond->eval(ep)) != ES_NORMAL && (MCtrace || MCnbreakpoints)
           && !MCtrylock && !MClockerrors)
		MCB_error(ctxt, getline(), getpos(), EE_IF_BADCOND);
	if (stat != ES_NORMAL)
	{
		MCeerror->add
		(EE_IF_BADCOND, line, pos);
		return;
	}
	Boolean then = ep.getsvalue() == MCtruemcstring;

	MCStatement *tspr;
	if (then)
		tspr = thenstatements;
	else
		tspr = elsestatements;
    
    stat = MCKeywordExecuteStatements(ctxt, tspr, EE_IF_BADSTATEMENT);
    ctxt . SetExecStat(stat);
}

uint4 MCIf::linecount()
{
	return countlines(thenstatements) + countlines(elsestatements);
}

MCRepeat::MCRepeat()
{
	form = RF_FOREVER;
	startcond = NULL;
	endcond = NULL;
	loopvar = NULL;
	step = NULL;
	statements = NULL;
}

MCRepeat::~MCRepeat()
{
	delete startcond;
	delete endcond;
	delete loopvar;
	delete step;
	deletestatements(statements);
}

Parse_stat MCRepeat::parse(MCScriptPoint &sp)
{
	Parse_stat stat;
	Symbol_type type;
	const LT *te;

	initpoint(sp);
	if ((stat = sp.next(type)) != PS_NORMAL)
	{
		if (stat == PS_EOL)
		{
			if (sp.skip_eol() != PS_NORMAL)
			{
				MCperror->add
				(PE_REPEAT_BADCONDEOL, sp);
				return PS_ERROR;
			}
		}
		else
		{
			MCperror->add
			(PE_REPEAT_BADCONDTYPE, sp);
			return PS_ERROR;
		}
	}
	else
		switch (stat)
		{
		case PS_NORMAL:
			if (sp.lookup(SP_REPEAT, te) != PS_NORMAL)
			{
				sp.backup();
				form = RF_FOR;
				if (sp.parseexp(False, True, &endcond) != PS_NORMAL)
				{
					MCperror->add
					(PE_REPEAT_BADCOND, sp);
					return PS_ERROR;
				}
			}
			else
			{
				MCExpression *newfact = NULL;
				switch (form = (Repeat_form)te->which)
				{
				case RF_FOREVER:
					break;
				case RF_FOR:
					if (sp.skip_token(SP_REPEAT, TT_UNDEFINED, RF_EACH) == PS_NORMAL)
					{
						if (sp.next(type) != PS_NORMAL
						        || sp.lookup(SP_UNIT, te) != PS_NORMAL
						        || sp.next(type) != PS_NORMAL)
						{
							MCperror->add(PE_REPEAT_BADCOND, sp);
							return PS_ERROR;
						}
						each = (File_unit)te->which;
						if (sp.lookupconstant(&newfact) == PS_NORMAL
						        || sp . findnewvar(sp.gettoken_nameref(), kMCEmptyName, &loopvar) != PS_NORMAL)
						{
							delete newfact;
							MCperror->add(PE_REPEAT_BADWITHVAR, sp);
							return PS_ERROR;
						}
						if (sp.skip_token(SP_FACTOR, TT_IN) != PS_NORMAL)
						{
							MCperror->add(PE_REPEAT_NOOF, sp);
							return PS_ERROR;
						}
					}
				case RF_UNTIL:
				case RF_WHILE:
					if (sp.parseexp(False, True, &endcond) != PS_NORMAL)
					{
						MCperror->add
						(PE_REPEAT_BADCOND, sp);
						return PS_ERROR;
					}
					break;
				case RF_WITH:
					if ((stat = sp.next(type)) != PS_NORMAL)
					{
						MCperror->add
						(PE_REPEAT_NOWITHVAR, sp);
						return PS_ERROR;
					}
					if (sp.lookupconstant(&newfact) == PS_NORMAL
					        || sp.findnewvar(sp.gettoken_nameref(), kMCEmptyName, &loopvar) != PS_NORMAL)
					{
						delete newfact;
						MCperror->add
						(PE_REPEAT_BADWITHVAR, sp);
						return PS_ERROR;
					}
					if ((stat = sp.next(type)) != PS_NORMAL)
					{
						MCperror->add
						(PE_REPEAT_NOEQUALS, sp);
						return PS_ERROR;
					}
					if (sp.lookup(SP_FACTOR, te) != PS_NORMAL
					        || te->type != TT_BINOP || te->which != O_EQ)
					{
						MCperror->add
						(PE_REPEAT_NOTEQUALS, sp);
						return PS_ERROR;
					}
					if (sp.parseexp(False, True, &startcond) != PS_NORMAL)
					{
						MCperror->add
						(PE_REPEAT_BADWITHSTARTEXP, sp);
						return PS_ERROR;
					}
					if ((stat = sp.next(type)) != PS_NORMAL)
					{
						MCperror->add
						(PE_REPEAT_NOWITHTO, sp);
						return PS_ERROR;
					}
					if (sp.lookup(SP_FACTOR, te) != PS_NORMAL)
					{
						if (!sp.token_is_cstring("down"))
						{
							MCperror->add
							(PE_REPEAT_NOWITHTO, sp);
							return PS_ERROR;
						}
						stepval = -1.0;
						if (sp.skip_token(SP_FACTOR, TT_TO) != PS_NORMAL)
						{
							MCperror->add
							(PE_REPEAT_NODOWNTO, sp);
							return PS_ERROR;
						}
					}
					else
						if (te->type != TT_TO)
						{
							MCperror->add
							(PE_REPEAT_NOTWITHTO, sp);
							return PS_ERROR;
						}
						else
							stepval = 1.0;
					if (sp.parseexp(False, True, &endcond) != PS_NORMAL)
					{
						MCperror->add
						(PE_REPEAT_BADWITHENDEXP, sp);
						return PS_ERROR;
					}
					if (sp.skip_token(SP_REPEAT, TT_UNDEFINED, RF_STEP) == PS_NORMAL)
						if (sp.parseexp(False, True, &step) != PS_NORMAL)
						{
							MCperror->add
							(PE_REPEAT_BADWITHSTARTEXP, sp);
							return PS_ERROR;
						}
					break;
				default: /* repeat form */
					fprintf(stderr, "Repeat: ERROR bad control form\n");
					return PS_ERROR;
				}
			}
			if (sp.skip_eol() != PS_NORMAL)
			{
				MCperror->add
				(PE_REPEAT_BADFORMEOL, sp);
				return PS_ERROR;
			}
			break;
		case PS_EOL:
			break;
		default: /* token type */
			MCperror->add
			(PE_REPEAT_BADFORMTYPE, sp);
			return PS_ERROR;
		}

	MCStatement *curstatement = NULL;
	MCStatement *newstatement = NULL;
	while (True)
	{
		switch (sp.next(type))
		{
		case PS_NORMAL:
			if (type == ST_DATA)
				newstatement = new MCEcho;
			else if (sp.lookup(SP_COMMAND, te) != PS_NORMAL)
			{
				if (type == ST_ID)
					newstatement = new MCComref(sp.gettoken_nameref());
				else
				{
					MCperror->add
					(PE_REPEAT_NOTCOMMAND, sp);
					return PS_ERROR;
				}
			}
			else
			{
				switch (te->type)
				{
				case TT_STATEMENT:
					newstatement = MCN_new_statement(te->which);
					break;
				case TT_END:
					if (sp.skip_token(SP_COMMAND, TT_STATEMENT, S_REPEAT) != PS_NORMAL)
					{
						MCperror->add
						(PE_REPEAT_WANTEDENDREPEAT, sp);
						return PS_ERROR;
					}
					return PS_NORMAL;
				default:
					MCperror->add
					(PE_REPEAT_BADSTATEMENT, sp);
					return PS_ERROR;
				}
			}
			break;
		case PS_EOL:
			if (sp.skip_eol() != PS_NORMAL)
			{
				MCperror->add
				(PE_REPEAT_WANTEDENDREPEAT, sp);
				return PS_ERROR;
			}
			continue;
		default:
			MCperror->add
			(PE_REPEAT_BADTOKEN, sp);
			return PS_ERROR;
		}
		if (newstatement->parse(sp) != PS_NORMAL)
		{
			MCperror->add
			(PE_REPEAT_BADCOMMAND, sp);
			delete newstatement;
			return PS_ERROR;
		}
		if (statements == NULL)
			statements = curstatement = newstatement;
		else
		{
			curstatement->setnext(newstatement);
			curstatement = newstatement;
		}
	}
	return PS_NORMAL;
}

#ifdef /* MCRepeat::exec */ LEGACY_EXEC
Exec_stat MCRepeat::exec(MCExecPoint &ep)
{
	real8 endn = 0.0;
	int4 count = 0;
	MCExecPoint ep2(ep);
	MCScriptPoint *sp = NULL;
	Parse_stat ps;
	const char *sptr;
	Boolean donumeric = False;
	Exec_stat stat;
	MCExecContext ctxt(ep);
	MCAutoArrayRef t_array;
	MCNameRef t_key;
	MCValueRef t_value;
	uintptr_t t_iterator;
	uint4 l;

	switch (form)
	{
	case RF_FOR:
		if (loopvar != NULL)
		{
			MCExecContext ctxt2(ep2);
			while ((stat = endcond->eval(ep2)) != ES_NORMAL
			        && (MCtrace || MCnbreakpoints) && !MCtrylock && !MClockerrors)
				MCB_error(ctxt2, getline(), getpos(), EE_REPEAT_BADFORCOND);
			if (stat != ES_NORMAL)
			{
				MCeerror->add(EE_REPEAT_BADFORCOND, line, pos);
				return ES_ERROR;
			}
			if (each == FU_ELEMENT)
			{
				ep2 . copyasarrayref(&t_array);
				t_iterator = 0;

				if (MCArrayIterate(*t_array, t_iterator, t_key, t_value))
					l = 1;
				else
					l = 0;

			}
			else if (each == FU_KEY)
			{
				ep2 . copyasarrayref(&t_array);
				t_iterator = 0;

				if (MCArrayIterate(*t_array, t_iterator, t_key, t_value))
					l = 1;
				else
					l = 0;
			}
			else
			{
				sptr = ep2.getsvalue().getstring();
				l = ep2.getsvalue().getlength();
				if (each == FU_WORD)
					MCU_skip_spaces(sptr, l);
				else
					if (each == FU_TOKEN)
					{
						sp = new MCScriptPoint(ep2);
						ps = sp->nexttoken();
						if (ps == PS_ERROR || ps == PS_EOF)
							l = 0;
					}
			}
		}
		else
		{
			while (((stat = endcond->eval(ep)) != ES_NORMAL
			        || (stat = ep.ton()) != ES_NORMAL) && (MCtrace || MCnbreakpoints)
			        && !MCtrylock && !MClockerrors)
				MCB_error(ctxt, getline(), getpos(), EE_REPEAT_BADFORCOND);
			if (stat != ES_NORMAL)
			{
				MCeerror->add
				(EE_REPEAT_BADFORCOND, line, pos);
				return ES_ERROR;
			}
			count = MCU_max(ep.getint4(), 0);
		}
		break;
	case RF_WITH:
		if (step != NULL)
		{
			while (((stat = step->eval(ep)) != ES_NORMAL
			        || (stat = ep.ton()) != ES_NORMAL) && (MCtrace || MCnbreakpoints)
			        && !MCtrylock && !MClockerrors)
				MCB_error(ctxt, getline(), getpos(), EE_REPEAT_BADWITHSTEP);
			stepval = ep.getnvalue();
			if (stat != ES_NORMAL || stepval == 0.0)
			{
				MCeerror->add
				(EE_REPEAT_BADWITHSTEP, line, pos);
				return ES_ERROR;
			}
		}
		while (((stat = startcond->eval(ep)) != ES_NORMAL
		        || (stat = ep.ton()) != ES_NORMAL) && (MCtrace || MCnbreakpoints)
		        && !MCtrylock && !MClockerrors)
			MCB_error(ctxt, getline(), getpos(), EE_REPEAT_BADWITHSTART);
		if (stat != ES_NORMAL)
		{
			MCeerror->add
			(EE_REPEAT_BADWITHSTART, line, pos);
			return ES_ERROR;
		}
		ep.setnvalue(ep.getnvalue() - stepval);
		while ((stat = loopvar->set
		               (ep)) != ES_NORMAL
		        && (MCtrace || MCnbreakpoints) && !MCtrylock && !MClockerrors)
			MCB_error(ctxt, getline(), getpos(), EE_REPEAT_BADWITHVAR);
		if (stat != ES_NORMAL)
		{
			MCeerror->add
			(EE_REPEAT_BADWITHVAR, line, pos);
			return ES_ERROR;
		}
		while (((stat = endcond->eval(ep)) != ES_NORMAL
		        || (stat = ep.ton()) != ES_NORMAL)
		        && (MCtrace || MCnbreakpoints) && !MCtrylock && !MClockerrors)
			MCB_error(ctxt, getline(), getpos(), EE_REPEAT_BADWITHEND);
		if (stat != ES_NORMAL)
		{
			MCeerror->add
			(EE_REPEAT_BADWITHEND, line, pos);
			return ES_ERROR;
		}
		endn = ep.getnvalue();
		break;
	default:
		break;
	}

	MCString s;
	Boolean done = False;
	bool t_first;
	t_first = false;
	while (True)
	{
		switch (form)
		{
		case RF_FOREVER:
			break;
		case RF_FOR:
			if (loopvar != NULL)
			{
				if (l == 0)
				{
					done = True;
					// OK-2007-12-05 : Bug 5605. If there has been at least one iteration, set the loop variable to 
					// whatever the value was in the last iteration. 
					if (!t_first)
					{
						// MW-2011-02-08: [[ Bug ]] Make sure we don't use 's' if the repeat type is 'key' or
						//   'element'.
						if (each != FU_ELEMENT && each != FU_KEY)
						{
							ep.setsvalue(s);
							loopvar->set(ep);
						}
					}
				}
				else
				{
					const char *startptr; // = sptr;
					switch (each)
					{
					case FU_KEY:
						// MW-2010-12-15: [[ Bug 9218 ]] Make a copy of the key so that it can't be mutated
						//   accidentally.
						ep . setvalueref(t_key);
						loopvar -> set(ep);
						if (!MCArrayIterate(*t_array, t_iterator, t_key, t_value))
							l = 0;
						break;
					case FU_ELEMENT:
						ep . setvalueref(t_value);
						loopvar -> set(ep);
						if (!MCArrayIterate(*t_array, t_iterator, t_key, t_value))
							l = 0;
						break;
					case FU_LINE:
						startptr = sptr;
						if (!MCU_strchr(sptr, l, ep.getlinedel()))
						{
							sptr += l;
							l = 0;
						}
						s.set(startptr, sptr - startptr);
						MCU_skip_char(sptr, l);
						break;
					case FU_ITEM:
						startptr = sptr;
						if (!MCU_strchr(sptr, l, ep.getitemdel()))
						{
							sptr += l;
							l = 0;
						}
						s.set(startptr, sptr - startptr);
						MCU_skip_char(sptr, l);
						break;
					case FU_WORD:
						startptr = sptr;
						if (*sptr == '\"')
						{
							MCU_skip_char(sptr, l);
							while (l && *sptr != '\"' && *sptr != '\n')
								MCU_skip_char(sptr, l);
							MCU_skip_char(sptr, l);
						}
						else
							while (l && !isspace((uint1)*sptr))
								MCU_skip_char(sptr, l);
						s.set(startptr, sptr - startptr);
						MCU_skip_spaces(sptr, l);
						break;
					case FU_TOKEN:
						s = sp->gettoken_oldstring();
						ps = sp->nexttoken();
						if (ps == PS_ERROR || ps == PS_EOF)
							l = 0;
						break;
					case FU_CHARACTER:
					default:
						startptr = sptr;
						s.set(startptr, 1);
						MCU_skip_char(sptr, l);
					}
					// MW-2010-12-15: [[ Bug 9218 ]] Added KEY to the type of repeat that already
					//   copies the value.
					if (each != FU_ELEMENT && each != FU_KEY)
					{
						if (MCtrace)
						{
							ep.setsvalue(s);
							loopvar->set(ep);
						}
						else
							loopvar->sets(s);
					}
				}
			}
			else
				done = count-- == 0;
			break;
		case RF_UNTIL:
			while ((stat = endcond->eval(ep)) != ES_NORMAL
			        && (MCtrace || MCnbreakpoints) && !MCtrylock && !MClockerrors)
				MCB_error(ctxt, getline(), getpos(), EE_REPEAT_BADUNTILCOND);
			if (stat != ES_NORMAL)
			{
				MCeerror->add
				(EE_REPEAT_BADUNTILCOND, line, pos);
				return ES_ERROR;
			}
			done = ep.getsvalue() == MCtruemcstring;
			break;
		case RF_WHILE:
			while ((stat = endcond->eval(ep)) != ES_NORMAL
			        && (MCtrace || MCnbreakpoints) && !MCtrylock && !MClockerrors)
				MCB_error(ctxt, getline(), getpos(), EE_REPEAT_BADWHILECOND);
			if (stat != ES_NORMAL)
			{
				MCeerror->add
				(EE_REPEAT_BADWHILECOND, line, pos);
				return ES_ERROR;
			}
			done = ep.getsvalue() != MCtruemcstring;
			break;
		case RF_WITH:
			while (((stat = loopvar->eval(ep)) != ES_NORMAL
			        || (stat = ep.ton()) != ES_NORMAL)
			        && (MCtrace || MCnbreakpoints) && !MCtrylock && !MClockerrors)
				MCB_error(ctxt, getline(), getpos(), EE_REPEAT_BADWITHVAR);
			if (stat != ES_NORMAL)
			{
				MCeerror->add
				(EE_REPEAT_BADWITHVAR, line, pos);
				return ES_ERROR;
			}
			if (stepval < 0)
			{
				if (ep.getnvalue() <= endn)
					done = True;
			}
			else
				if (ep.getnvalue() >= endn)
					done = True;
			if (!done)
			{
				ep.setnvalue(ep.getnvalue() + stepval);
				while ((stat = loopvar->set
				               (ep)) != ES_NORMAL
				        && (MCtrace || MCnbreakpoints) && !MCtrylock && !MClockerrors)
					MCB_error(ctxt, getline(), getpos(), EE_REPEAT_BADWITHVAR);
				if (stat != ES_NORMAL)
				{
					MCeerror->add
					(EE_REPEAT_BADWITHVAR, line, pos);
					return ES_ERROR;
				}
			}
			break;
		default:
			break;
		}
		if (done)
			break;

		Exec_stat stat;
		MCStatement *tspr = statements;
		while (tspr != NULL)
		{
			if (MCtrace || MCnbreakpoints)
			{
				MCB_trace(ctxt, tspr->getline(), tspr->getpos());
				if (MCexitall)
					break;
			}
			ep.setline(tspr->getline());

			stat = tspr->exec(ep);

			// MW-2011-08-17: [[ Redraw ]] Flush any screen updates.
			MCRedrawUpdateScreen();
			
			switch(stat)
			{
			case ES_NORMAL:
				if (MCexitall)
				{
					// OK-2007-12-05 : Bug 5605 : If exiting loop, set the loop variable to the value it had
					//   in the last iteration.
					// MW-2011-02-08: [[ Bug ]] Make sure we don't use 's' if the repeat type is 'key' or
					//   'element'.
					if (form == RF_FOR && loopvar != NULL && each != FU_ELEMENT && each != FU_KEY)
					{
						ep.setsvalue(s);
						loopvar->set(ep);
					}
					delete sp;
					return ES_NORMAL;
				}
				tspr = tspr->getnext();
				break;
			case ES_NEXT_REPEAT:
				tspr = NULL;
				break;
			case ES_EXIT_REPEAT:
				// OK-2007-12-05 : Bug 5605 : If exiting loop, set the loop variable to the value it had
				//   in the last iteration.
				// MW-2011-02-08: [[ Bug ]] Make sure we don't use 's' if the repeat type is 'key' or
				//   'element'.
				if (form == RF_FOR && loopvar != NULL && each != FU_ELEMENT && each != FU_KEY)
				{
					ep.setsvalue(s);
					loopvar->set(ep);
				}
				delete sp;
				return ES_NORMAL;
			case ES_ERROR:
				if ((MCtrace || MCnbreakpoints) && !MCtrylock && !MClockerrors)
					do
					{
						MCB_error(ctxt, tspr->getline(), tspr->getpos(),
						          EE_REPEAT_BADSTATEMENT);
					}
					while (MCtrace && (stat = tspr->exec(ep)) != ES_NORMAL);
				if (stat == ES_ERROR)
				{
					// OK-2007-12-05 : Bug 5605 : If exiting loop, set the loop variable to the value it had
					//   in the last iteration.
					// MW-2011-02-08: [[ Bug ]] Make sure we don't use 's' if the repeat type is 'key' or
					//   'element'.
					if (form == RF_FOR && loopvar != NULL && each != FU_ELEMENT && each != FU_KEY)
					{
						ep.setsvalue(s);
						loopvar->set(ep);
					}
					delete sp;
					if (MCexitall)
						return ES_NORMAL;
					else
					{
						MCeerror->add(EE_REPEAT_BADSTATEMENT, line, pos);
						return ES_ERROR;
					}
				}
				else
					tspr = tspr->getnext();
				break;
			default:
				// OK-2007-12-05 : Bug 5605 : If exiting loop, set the loop variable to the value it had
				//   in the last iteration.
				// MW-2011-02-08: [[ Bug ]] Make sure we don't use 's' if the repeat type is 'key' or
				//   'element'.
				if (form == RF_FOR && loopvar != NULL && each != FU_ELEMENT && each != FU_KEY)
				{
					ep.setsvalue(s);
					loopvar->set(ep);
				}
				delete sp;
				return stat;
			}
		}
		if (MCscreen->abortkey())
		{
			// OK-2007-12-05 : Bug 5605 : If exiting loop, set the loop variable to the value it had
			//   in the last iteration.
			// MW-2011-02-08: [[ Bug ]] Make sure we don't use 's' if the repeat type is 'key' or
			//   'element'.
			if (form == RF_FOR && loopvar != NULL && each != FU_ELEMENT && each != FU_KEY)
			{
				ep.setsvalue(s);
				loopvar->set(ep);
			}
			delete sp;
			MCeerror->add(EE_REPEAT_ABORT, line, pos);
			return ES_ERROR;
		}
		if (MCtrace || MCnbreakpoints)
		{
			MCB_trace(ctxt, getline(), getpos());
			if (MCexitall)
				break;
		}

		t_first = false;
	}
	delete sp;
	return ES_NORMAL;
}
#endif /* MCRepeat::exec */

void MCRepeat::execute_statements(MCExecContext& ctxt, bool& r_done)
{
    Exec_stat stat = MCKeywordExecuteStatements(ctxt, statements, EE_REPEAT_BADSTATEMENT);
    if ((stat == ES_NORMAL && MCexitall) || (stat != ES_NEXT_REPEAT && stat != ES_NORMAL))
    {
        r_done = true;
        return;
    }
    if (MCscreen->abortkey())
    {
        r_done = true;
        ctxt . LegacyThrow(EE_REPEAT_ABORT);
        return;
    }
    if (MCtrace || MCnbreakpoints)
    {
        MCB_trace(ctxt, getline(), getpos());
        if (MCexitall)
            r_done = true;
    }
}

void MCRepeat::exec_for(MCExecContext& ctxt)
{
    MCAutoArrayRef t_array;
    MCAutoStringRef t_string;
    MCRange t_chunk_range;
    t_chunk_range = MCRangeMake(0,0);
    uindex_t t_length = 0;
    MCAutoValueRef t_condition;
	MCNameRef t_key;
	MCValueRef t_value;
	uintptr_t t_iterator;
    Parse_stat ps;
    MCScriptPoint *sp = nil;
    int4 count = 0;
    
    if (!ctxt . TryToEvaluateExpression(endcond, getline(), getpos(), EE_REPEAT_BADFORCOND, &t_condition))
        return;
    
    if (loopvar != NULL)
    {
        if (each == FU_ELEMENT || each == FU_KEY)
        {
            if (!ctxt . ConvertToArray(*t_condition, &t_array))
                return;
            
            t_iterator = 0;
            
            if (!MCArrayIterate(*t_array, t_iterator, t_key, t_value))
                return;
        }
        else
        {
            if (!ctxt . ConvertToString(*t_condition, &t_string))
                return;
            
             t_length = MCStringGetLength(*t_string);
        }
    }
    else
    {
        if (!ctxt . ConvertToInteger(*t_condition, count))
            return;
        count = MCU_max(count, 0);
    }
    
    bool done;
    done = false;
    
    bool endnext;
    endnext = false;
    
    bool t_found;
    t_found = false;
    
    while (!done)
    {
        MCAutoStringRef t_unit;
        if (loopvar != NULL)
        {
            switch (each)
            {
                case FU_KEY:
                {
                    // MW-2010-12-15: [[ Bug 9218 ]] Make a copy of the key so that it can't be mutated
                    //   accidentally.
                    MCNewAutoNameRef t_key_copy;
                    MCNameClone(t_key, &t_key_copy);
                    loopvar -> set(ctxt, *t_key_copy);
                    if (!MCArrayIterate(*t_array, t_iterator, t_key, t_value))
                        endnext = true;
                }
                    break;
                case FU_ELEMENT:
                {
                    loopvar -> set(ctxt, t_value);
                    if (!MCArrayIterate(*t_array, t_iterator, t_key, t_value))
                        endnext = true;
                }
                    break;
                default:
                {
                    switch (each)
                    {
                        case FU_LINE:
                            t_found = MCStringsFindNextChunk(ctxt, *t_string, CT_LINE, t_length, t_chunk_range, t_found, endnext);
                            break;
                        case FU_ITEM:
                            t_found = MCStringsFindNextChunk(ctxt, *t_string, CT_ITEM, t_length, t_chunk_range, t_found, endnext);
                            break;
                        case FU_WORD:
                            t_found = MCStringsFindNextChunk(ctxt, *t_string, CT_WORD, t_length, t_chunk_range, t_found, endnext);
                            break;
                        case FU_TOKEN:
                            t_found = MCStringsFindNextChunk(ctxt, *t_string, CT_TOKEN, t_length, t_chunk_range, t_found, endnext);
                            break;
                        case FU_CHARACTER:
                        default:
                            t_found = MCStringsFindNextChunk(ctxt, *t_string, CT_CHARACTER, t_length, t_chunk_range, t_found, endnext);
                            break;
                    }
                    if (!t_found)
                    {
                        &t_unit = MCValueRetain(kMCEmptyString);
                        done = true;
                    }
                    else
                        MCStringCopySubstring(*t_string, t_chunk_range, &t_unit);
                }
            }
            // MW-2010-12-15: [[ Bug 9218 ]] Added KEY to the type of repeat that already
            //   copies the value.
            // MW-2011-02-08: [[ Bug ]] Make sure we don't use 't_unit' if the repeat type is 'key' or
            //   'element'.
            // Set the loop variable to whatever the value was in the last iteration.
            if (each != FU_ELEMENT && each != FU_KEY)
                loopvar->set(ctxt, *t_unit);
        }
        else
            done = count-- == 0;
        
        if (!done)
            execute_statements(ctxt, done);
        
        if (done)
        {
            if (*t_unit == nil)
            {
                bool t_true = true;
            }
            // Reset the loop variable to whatever the value was in the last iteration.
            if (loopvar != nil && each != FU_ELEMENT && each != FU_KEY)
                loopvar->set(ctxt, *t_unit);
        }
        
        done = done || endnext;
    }
}

void MCRepeat::exec_with(MCExecContext& ctxt)
{
    real8 endn = 0.0;
    MCAutoValueRef t_condition, t_step;
    
    if (step != NULL)
    {
        if (!ctxt . TryToEvaluateExpression(step, getline(), getpos(), EE_REPEAT_BADWITHSTEP, &t_step))
            return;
        
        if (!ctxt . ConvertToReal(*t_condition, stepval) || stepval == 0.0)
        {
            ctxt . LegacyThrow(EE_REPEAT_BADWITHSTEP);
            return;
        }

    }

    if (!ctxt . TryToEvaluateExpression(startcond, getline(), getpos(), EE_REPEAT_BADWITHSTART, &t_condition))
        return;
    
    real8 t_loop;
    if (!ctxt . ConvertToReal(*t_condition, t_loop))
    {
        ctxt . LegacyThrow(EE_REPEAT_BADWITHSTART);
        return;
    }
    
    t_loop -= stepval;
    
    MCAutoNumberRef t_loop_var;
    if (!MCNumberCreateWithReal(t_loop, &t_loop_var) || !ctxt . TryToSetVariable(loopvar, getline(), getpos(), EE_REPEAT_BADWITHVAR, *t_loop_var))
        return;

    MCAutoValueRef t_end_condition;
    if (!ctxt . TryToEvaluateExpression(endcond, getline(), getpos(), EE_REPEAT_BADWITHSTART, &t_end_condition) ||
        !ctxt . ConvertToReal(*t_end_condition, endn))
        return;
    
    bool done;
    done = false;
    
    while (!done)
    {
        MCAutoValueRef t_loop_pos;
        real8 t_cur_value;
        if (!ctxt . TryToEvaluateExpression(loopvar, getline(), getpos(), EE_REPEAT_BADWITHVAR, &t_loop_pos) ||
            !ctxt . ConvertToReal(*t_loop_pos, t_cur_value))
            return;
        
        if (stepval < 0)
        {
            if (t_cur_value <= endn)
                done = true;
        }
        else
            if (t_cur_value >= endn)
                done = true;
        
        if (!done)
        {
            MCAutoNumberRef t_cur_loop;
            if (!MCNumberCreateWithReal(t_cur_value + stepval, &t_cur_loop) || !ctxt . TryToSetVariable(loopvar, getline(), getpos(), EE_REPEAT_BADWITHVAR, *t_cur_loop))
                return;
            
            execute_statements(ctxt, done);
        }
        
        if (done)
            break;
    }
}

void MCRepeat::exec_forever(MCExecContext& ctxt)
{
    bool done;
    done = false;
    while (!done)
        execute_statements(ctxt, done);
}

void MCRepeat::exec_until(MCExecContext& ctxt)
{
    bool done;
    done = false;
    
    while (!done)
    {
        MCAutoValueRef t_value;
        if (!ctxt . TryToEvaluateExpression(endcond, getline(), getpos(), EE_REPEAT_BADUNTILCOND, &t_value) ||
            !ctxt . ConvertToBool(*t_value, done))
            return;
        if (!done)
            execute_statements(ctxt, done);
    }
}

void MCRepeat::exec_while(MCExecContext& ctxt)
{
    bool done;
    done = false;
    
    while (!done)
    {
        MCAutoValueRef t_value;
        bool not_done;
        if (!ctxt . TryToEvaluateExpression(endcond, getline(), getpos(), EE_REPEAT_BADWHILECOND, &t_value) ||
            !ctxt . ConvertToBool(*t_value, not_done))
            return;
        
        done = !not_done;
        
        if (not_done)
            execute_statements(ctxt, done);
    }
}

void MCRepeat::exec_ctxt(MCExecContext& ctxt)
{
    switch (form)
	{
        case RF_FOR:
            MCRepeat::exec_for(ctxt);
            break;
        case RF_WITH:
            MCRepeat::exec_with(ctxt);
            break;
        case RF_FOREVER:
            MCRepeat::exec_forever(ctxt);
            break;
        case RF_UNTIL:
            MCRepeat::exec_until(ctxt);
            break;
        case RF_WHILE:
            MCRepeat::exec_while(ctxt);
            break;
        default:
            break;
    }
}

uint4 MCRepeat::linecount()
{
	return countlines(statements);
}

Parse_stat MCExit::parse(MCScriptPoint &sp)
{
	Symbol_type type;
	const LT *te;

	initpoint(sp);
	if (sp.next(type) != PS_NORMAL
	        || sp.lookup(SP_EXIT, te) != PS_NORMAL)
	{
		if (!sp.gethandler()->hasname(sp.gettoken_nameref()))
		{
			MCperror->add(PE_EXIT_BADDEST, sp);
			return PS_ERROR;
		}
		exit = ES_EXIT_HANDLER;
	}
	else
		switch (te->which)
		{
		case ET_ALL:
			exit = ES_EXIT_ALL;
			break;
		case ET_REPEAT:
			exit = ES_EXIT_REPEAT;
			break;
		case ET_SWITCH:
			exit = ES_EXIT_SWITCH;
			break;
		case ET_TO:
			if (sp.next(type) != PS_NORMAL)
			{
				MCperror->add
				(PE_EXIT_NODEST, sp);
				return PS_ERROR;
			}
			if (sp.lookup(SP_EXIT, te) != PS_NORMAL || te->which != ET_ALL)
			{
				MCperror->add
				(PE_EXIT_BADDEST, sp);
				return PS_ERROR;
			}
			exit = ES_EXIT_ALL;
			break;
		}
	return PS_NORMAL;
}

Exec_stat MCExit::exec(MCExecPoint &ep)
{
	if (exit == ES_EXIT_ALL)
	{
		MCexitall = True;
		return ES_NORMAL;
	}
	return exit;
}

uint4 MCExit::linecount()
{
	return 0;
}

Parse_stat MCNext::parse(MCScriptPoint &sp)
{
	Symbol_type type;
	const LT *te;

	initpoint(sp);
	if (sp.next(type) != PS_NORMAL)
	{
		MCperror->add
		(PE_NEXT_NOREPEAT, sp);
		return PS_ERROR;
	}
	if (sp.lookup(SP_EXIT, te) != PS_NORMAL || te->which != ET_REPEAT)
	{
		MCperror->add
		(PE_NEXT_NOTREPEAT, sp);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

Exec_stat MCNext::exec(MCExecPoint &ep)
{
	return ES_NEXT_REPEAT;
}

uint4 MCNext::linecount()
{
	return 0;
}

Parse_stat MCPass::parse(MCScriptPoint &sp)
{
	Symbol_type type;

	initpoint(sp);
	if (sp.next(type) != PS_NORMAL ||
		!sp.gethandler()->hasname(sp.gettoken_nameref()))
	{
		MCperror->add(PE_PASS_NOMESSAGE, sp);
		return PS_ERROR;
	}
	if (sp.skip_token(SP_FACTOR, TT_TO) == PS_NORMAL)
	{
		if (sp.next(type) != PS_NORMAL)
		{
			MCperror->add(PE_PASS_NOMESSAGE, sp);
			return PS_ERROR;
		}
		all = True;
	}
	if (sp.gethandler() -> isprivate())
	{
		MCperror -> add(PE_PRIVATE_BADPASS, sp);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

Exec_stat MCPass::exec(MCExecPoint &ep)
{
	if (all)
		return ES_PASS_ALL;
	else
		return ES_PASS;
}

uint4 MCPass::linecount()
{
	return 0;
}

Exec_stat MCBreak::exec(MCExecPoint &ep)
{
	return ES_EXIT_SWITCH;
}

uint4 MCBreak::linecount()
{
	return 0;
}

MCSwitch::~MCSwitch()
{
	delete cond;
	while (ncases--)
		delete cases[ncases];
	delete cases;
	deletestatements(statements);
	delete caseoffsets;
}

Parse_stat MCSwitch::parse(MCScriptPoint &sp)
{
	Symbol_type type;
	const LT *te;

	initpoint(sp);
	if (sp.next(type) == PS_NORMAL)
	{
		sp.backup();
		if (sp.parseexp(False, True, &cond) != PS_NORMAL)
		{
			MCperror->add
			(PE_SWITCH_BADCONDITION, sp);
			return PS_ERROR;
		}
	}
	else
		if (sp.skip_eol() != PS_NORMAL)
		{
			MCperror->add
			(PE_SWITCH_WANTEDENDSWITCH, sp);
			return PS_ERROR;
		}
	uint2 snum = 0;
	MCStatement *curstatement = NULL;
	MCStatement *newstatement = NULL;
	while (True)
	{
		switch (sp.next(type))
		{
		case PS_NORMAL:
			if (type == ST_DATA)
				newstatement = new MCEcho;
			else if (sp.lookup(SP_COMMAND, te) != PS_NORMAL)
			{
				if (type == ST_ID)
					newstatement = new MCComref(sp.gettoken_nameref());
				else
				{
					MCperror->add
					(PE_SWITCH_NOTCOMMAND, sp);
					return PS_ERROR;
				}
			}
			else
			{
				switch (te->type)
				{
				case TT_STATEMENT:
					newstatement = MCN_new_statement(te->which);
					break;
				case TT_CASE:
					MCU_realloc((char **)&cases, ncases, ncases + 1,
					            sizeof(MCExpression *));
					if (sp.parseexp(False, True, &cases[ncases]) != PS_NORMAL)
					{
						MCperror->add
						(PE_SWITCH_BADCASECONDITION, sp);
						return PS_ERROR;
					}
					MCU_realloc((char **)&caseoffsets, ncases, ncases + 1,
					            sizeof(uint2));
					caseoffsets[ncases++] = snum;
					continue;
				case TT_DEFAULT:
					defaultcase = snum;
					continue;
				case TT_END:
					if (sp.skip_token(SP_COMMAND, TT_STATEMENT, S_SWITCH) != PS_NORMAL)
					{
						MCperror->add
						(PE_SWITCH_WANTEDENDSWITCH, sp);
						return PS_ERROR;
					}
					return PS_NORMAL;
				default: /* token type */
					MCperror->add
					(PE_SWITCH_BADCASECONDITION, sp);
					return PS_ERROR;
				}
			}
			break;
		case PS_EOL:
			if (sp.skip_eol() != PS_NORMAL)
			{
				MCperror->add
				(PE_SWITCH_WANTEDENDSWITCH, sp);
				return PS_ERROR;
			}
			continue;
		case PS_EOF:
			return PS_NORMAL;
		default:
			MCperror->add
			(PE_SWITCH_BADTYPE, sp);
			return PS_ERROR;
		}
		if (newstatement->parse(sp) != PS_NORMAL)
		{
			MCperror->add
			(PE_SWITCH_BADSTATEMENT, sp);
			delete newstatement;
			return PS_ERROR;
		}
		if (statements == NULL)
			statements = curstatement = newstatement;
		else
		{
			curstatement->setnext(newstatement);
			curstatement = newstatement;
		}
		snum++;
	}
	return PS_NORMAL;
}
/*
Exec_stat MCSwitch::exec(MCExecPoint &ep)
{
	MCExecPoint ep2(ep);
	MCExecContext ctxt(ep);
	MCExecContext ctxt2(ep2);
	Exec_stat stat;
	if (cond != NULL)
	{
		while ((stat = cond->eval(ep2)) != ES_NORMAL
		        && (MCtrace || MCnbreakpoints) && !MCtrylock && !MClockerrors)
			MCB_error(ctxt2, getline(), getpos(), EE_SWITCH_BADCOND);
		if (stat != ES_NORMAL)
		{
			MCeerror->add(EE_SWITCH_BADCOND, line, pos);
			return ES_ERROR;
		}
	}
	else
		ep2.setboolean(true);
	int2 match = defaultcase;
	uint2 i;
	for (i = 0 ; i < ncases ; i++)
	{
		while ((stat = cases[i]->eval(ep)) != ES_NORMAL
		        && (MCtrace || MCnbreakpoints) && !MCtrylock && !MClockerrors)
			MCB_error(ctxt2, getline(), getpos(), EE_SWITCH_BADCASE);
		if (stat != ES_NORMAL)
		{
			MCeerror->add
			(EE_SWITCH_BADCASE, line, pos);
			return ES_ERROR;
		}
		uint4 l1 = ep.getsvalue().getlength();
		uint4 l2 = ep2.getsvalue().getlength();
		if (l1 == l2)
		{
			const char *s1 = ep.getsvalue().getstring();
			const char *s2 = ep2.getsvalue().getstring();
			if (ep.getcasesensitive() && !strncmp(s1, s2, l1)
			        || !ep.getcasesensitive() && !MCU_strncasecmp(s1, s2, l1))
			{
				match = caseoffsets[i];
				break;
			}
		}
	}
	if (match >= 0)
	{
		MCStatement *tspr = statements;
		while (match--)
			tspr = tspr->getnext();
		Exec_stat stat;
		while (tspr != NULL)
		{
			if (MCtrace || MCnbreakpoints)
			{
				MCB_trace(ctxt, tspr->getline(), tspr->getpos());
				if (MCexitall)
					break;
			}
			ep.setline(tspr->getline());
			
			stat = tspr->exec(ep);
			
			// MW-2011-08-17: [[ Redraw ]] Flush any screen updates.
			MCRedrawUpdateScreen();

			switch(stat)
			{
			case ES_NORMAL:
				if (MCexitall)
					return ES_NORMAL;
				tspr = tspr->getnext();
				break;
			case ES_ERROR:
				if ((MCtrace || MCnbreakpoints) && !MCtrylock && !MClockerrors)
					do
					{
						MCB_error(ctxt, tspr->getline(), tspr->getpos(),
						          EE_SWITCH_BADSTATEMENT);
					}
					while (MCtrace && (stat = tspr->exec(ep)) != ES_NORMAL);
				if (stat == ES_ERROR)
					if (MCexitall)
						return ES_NORMAL;
					else
					{
						MCeerror->add
						(EE_SWITCH_BADSTATEMENT, line, pos);
						return ES_ERROR;
					}
				else
					tspr = tspr->getnext();
				break;
			case ES_EXIT_SWITCH:
				return ES_NORMAL;
			default:
				return stat;
			}
		}
	}
	return ES_NORMAL;
}
*/
void MCSwitch::exec_ctxt(MCExecContext& ctxt)
{
    MCKeywordsExecSwitch(ctxt, cond, cases, ncases, defaultcase, caseoffsets, statements, getline(), getpos());
}

uint4 MCSwitch::linecount()
{
	return countlines(statements);
}

MCThrowKeyword::~MCThrowKeyword()
{
	delete error;
}

Parse_stat MCThrowKeyword::parse(MCScriptPoint &sp)
{
	initpoint(sp);
	if (sp.parseexp(False, True, &error) != PS_NORMAL)
	{
		MCperror->add
		(PE_THROW_BADERROR, sp);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

Exec_stat MCThrowKeyword::exec(MCExecPoint &ep)
{
	if (error->eval(ep) != ES_NORMAL)
		MCeerror->add
		(EE_THROW_BADERROR, line, pos);
	else
    {
        MCAutoStringRef t_value;
        ep . copyasstringref(&t_value);
		MCeerror->copystringref(*t_value, True);
    }
	return ES_ERROR;
}

uint4 MCThrowKeyword::linecount()
{
	return 0;
}

MCTry::~MCTry()
{
	deletestatements(trystatements);
	deletestatements(catchstatements);
	deletestatements(finallystatements);
	delete errorvar;
}

Parse_stat MCTry::parse(MCScriptPoint &sp)
{
	initpoint(sp);
	Try_state state = TS_TRY;
	if (sp.skip_eol() != PS_NORMAL)
	{
		MCperror->add
		(PE_TRY_WANTEDENDTRY, sp);
		return PS_ERROR;
	}
	MCStatement *curstatement = NULL;
	MCStatement *newstatement = NULL;
	while (True)
	{
		Symbol_type type;
		const LT *te;
		switch (sp.next(type))
		{
		case PS_NORMAL:
			if (type == ST_DATA)
				newstatement = new MCEcho;
			else if (sp.lookup(SP_COMMAND, te) != PS_NORMAL)
			{
				if (type == ST_ID)
					newstatement = new MCComref(sp.gettoken_nameref());
				else
				{
					MCperror->add
					(PE_TRY_NOTCOMMAND, sp);
					return PS_ERROR;
				}
			}
			else
			{
				Parse_stat stat;
				MCExpression *newfact = NULL;
				switch (te->type)
				{
				case TT_STATEMENT:
					newstatement = MCN_new_statement(te->which);
					break;
				case TT_CATCH:
					state = TS_CATCH;
					curstatement = NULL;
					stat = sp.next(type);
					if (errorvar != NULL || stat != PS_NORMAL || type != ST_ID
					        || sp.lookup(SP_FACTOR, te) != PS_NO_MATCH
					        || sp.lookupconstant(&newfact) == PS_NORMAL
					        || sp . findnewvar(sp.gettoken_nameref(), kMCEmptyName, &errorvar) != PS_NORMAL)
					{
						delete newfact;
						MCperror->add(PE_LOCAL_BADNAME, sp);
						return PS_ERROR;
					}
					continue;
					break;
				case TT_FINALLY:
					state = TS_FINALLY;
					curstatement = NULL;
					continue;
					break;
				case TT_END:
					if (sp.skip_token(SP_COMMAND, TT_STATEMENT, S_TRY) != PS_NORMAL)
					{
						MCperror->add
						(PE_TRY_WANTEDENDTRY, sp);
						return PS_ERROR;
					}
					return PS_NORMAL;
				default: /* token type */
					MCperror->add
					(PE_TRY_BADSTATEMENT, sp);
					return PS_ERROR;
				}
			}
			break;
		case PS_EOL:
			if (sp.skip_eol() != PS_NORMAL)
			{
				MCperror->add
				(PE_TRY_WANTEDENDTRY, sp);
				return PS_ERROR;
			}
			continue;
		case PS_EOF:
			return PS_NORMAL;
		default:
			MCperror->add
			(PE_TRY_BADTYPE, sp);
			return PS_ERROR;
		}
		if (newstatement->parse(sp) != PS_NORMAL)
		{
			MCperror->add
			(PE_TRY_BADSTATEMENT, sp);
			delete newstatement;
			return PS_ERROR;
		}
		if (curstatement != NULL)
		{
			curstatement->setnext(newstatement);
			curstatement = newstatement;
		}
		else
			switch (state)
			{
			case TS_TRY:
				trystatements = curstatement = newstatement;
				break;
			case TS_CATCH:
				catchstatements = curstatement = newstatement;
				break;
			case TS_FINALLY:
				finallystatements = curstatement = newstatement;
				break;
			}
	}
	return PS_NORMAL;
}

Exec_stat MCTry::exec(MCExecPoint &ep)
{
	MCExecContext ctxt(ep);
	Try_state state = TS_TRY;
	MCStatement *tspr = trystatements;
	Exec_stat stat;
	Exec_stat retcode = ES_NORMAL;
	MCtrylock++;
	while (tspr != NULL)
	{
		if (MCtrace || MCnbreakpoints)
		{
			MCB_trace(ctxt, tspr->getline(), tspr->getpos());
			if (MCexitall)
				break;
		}
		ep.setline(tspr->getline());

		stat = tspr->exec(ep);

		// MW-2011-08-17: [[ Redraw ]] Flush any screen updates.
		MCRedrawUpdateScreen();

		switch(stat)
		{
		case ES_NORMAL:
			tspr = tspr->getnext();
			if (MCexitall)
			{
				retcode = ES_NORMAL;
				tspr = NULL;
			}

			if (tspr == NULL && state != TS_FINALLY)
			{
				if (state == TS_CATCH)
					MCeerror->clear();

				tspr = finallystatements;
				state = TS_FINALLY;
			}
			break;
		case ES_ERROR:
			if ((MCtrace || MCnbreakpoints) && state != TS_TRY)
				do
				{
					MCB_error(ctxt, tspr->getline(), tspr->getpos(), EE_TRY_BADSTATEMENT);
				}
				while(MCtrace && (stat = tspr->exec(ep)) != ES_NORMAL);

			if (stat == ES_ERROR)
			{
				if (MCexitall)
				{
					retcode = ES_NORMAL;
					tspr = NULL;
				}
				else
					if (state != TS_TRY)
					{
						MCtrylock--;
						MCeerror->add(EE_TRY_BADSTATEMENT, line, pos);
						return ES_ERROR;
					}
					else
					{
						if (errorvar != NULL)
							errorvar->evalvar(ep)->copysvalue(MCeerror->getsvalue());

						// MW-2007-09-04: At this point we need to clear the execution error
						//   stack so that errors inside the catch statements are reported
						//   correctly.
						MCeerror->clear();
						MCperror->clear();


						tspr = catchstatements;
						state = TS_CATCH;

						// MW-2007-07-03: [[ Bug 3029 ]] - If there is no catch clause
						//   we end up skipping the finally as the loop terminates
						//   before a state transition is made, thus we force it here.
						if (catchstatements == NULL)
						{
							MCeerror -> clear();
							tspr = finallystatements;
							state = TS_FINALLY;
						}
					}
			}
			else
				tspr = tspr->getnext();
			break;
		case ES_PASS:
			if (state == TS_CATCH)
			{
				errorvar->evalvar(ep)->eval(ep);
                MCAutoStringRef t_value;
                ep . copyasstringref(&t_value);
				MCeerror->copystringref(*t_value, False);
				MCeerror->add(EE_TRY_BADSTATEMENT, line, pos);
				stat = ES_ERROR;
			}
		default:
			if (state == TS_FINALLY)
			{
				MCeerror->clear();
				retcode = ES_NORMAL;
				tspr = NULL;
			}
			else
			{
				retcode = stat;
				tspr = finallystatements;
				state = TS_FINALLY;
			}
		}
	}
	if (state == TS_CATCH)
		MCeerror->clear();
	MCtrylock--;
	return retcode;
}

uint4 MCTry::linecount()
{
	return countlines(trystatements) + countlines(catchstatements)
	       + countlines(finallystatements);
}

////////////////////////////////////////////////////////////////////////////////

void MCKeywordsExecSwitch(MCExecContext& ctxt, MCExpression *condition, MCExpression **cases, uindex_t case_count, int2 default_case, uint2 *case_offsets, MCStatement *statements, uint2 line, uint2 pos)
{
    MCAutoValueRef t_value;
    MCAutoStringRef t_cond;
	if (condition != NULL)
	{
        if (!ctxt . TryToEvaluateExpression(condition, line, pos, EE_SWITCH_BADCOND, &t_value))
            return;
        
        if (!ctxt . ConvertToString(*t_value, &t_cond))
        {
            ctxt . LegacyThrow(EE_SWITCH_BADCOND);
            return;
        }
	}
    else
        &t_cond = MCValueRetain(kMCTrueString);
    
	int2 match = default_case;
	uint2 i;
	for (i = 0 ; i < case_count ; i++)
	{
        MCAutoValueRef t_case;
        MCAutoStringRef t_case_string;
        
        if (!ctxt . TryToEvaluateExpression(cases[i], line, pos, EE_SWITCH_BADCASE, &t_case))
            return;
        
        if (!ctxt . ConvertToString(*t_case, &t_case_string))
        {
            ctxt . LegacyThrow(EE_SWITCH_BADCASE);
            return;
        }
        
		if (MCStringIsEqualTo(*t_cond, *t_case_string, ctxt . GetCaseSensitive() ? kMCStringOptionCompareExact : kMCStringOptionCompareCaseless))
        {
            match = case_offsets[i];
            break;
        }
	}
    
	if (match >= 0)
	{
		MCStatement *tspr = statements;
		while (match--)
			tspr = tspr->getnext();
        
        ctxt . SetExecStat(MCKeywordExecuteStatements(ctxt, tspr, EE_SWITCH_BADSTATEMENT));
    }
}

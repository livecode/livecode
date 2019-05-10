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
#include "object.h"
#include "param.h"

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

        // SN-2015-11-19: [[ Bug 16452 ]] We need to check whether a local
        // variable already exists at a script/handler level, to avoid shadowing
        bool t_shadowing;
        t_shadowing = false;

        MCVarref* t_var;
        t_var = NULL;
		if (sp.gethandler() == NULL)
        {
            sp.gethlist() -> findvar(sp.gettoken_nameref(), false, &t_var);
            if (t_var == NULL || !MCexplicitvariables)
                sp.gethlist()->newglobal(sp.gettoken_nameref());
            else
                t_shadowing = true;
        }
		else
        {
            sp.gethandler()->findvar(sp.gettoken_nameref(), &t_var);
            if (t_var == NULL || !MCexplicitvariables)
                sp.gethandler()->newglobal(sp.gettoken_nameref());
            else
                t_shadowing = true;
        }

        // Clearup fetched var
        delete t_var;

        // In case we are shadowing a local variable, then we raise an error
        if (t_shadowing)
        {
            MCperror -> add(PE_GLOBAL_SHADOW, sp);
            return PS_ERROR;
        }

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

		MCNewAutoNameRef t_token_name = sp . gettoken_nameref();

		// MW-2013-11-08: [[ RefactorIt ]] The 'it' variable is always present now,
		//   so there's no need to 'local it'. However, scripts do contain this so
		//   don't do a check for an existing var in this case.
		if (!MCNameIsEqualToCaseless(*t_token_name, MCN_it))
		{
			MCExpression *e = NULL;
			MCVarref *v = NULL;
			if (sp.gethandler() == NULL)
				if (constant)
					sp.gethlist()->findconstant(*t_token_name, &e);
				else
					sp.gethlist()->findvar(*t_token_name, false, &v);
			else
				if (constant)
					sp.gethandler()->findconstant(*t_token_name, &e);
				else
					sp.gethandler()->findvar(*t_token_name, &v);
			if (e != NULL || v != NULL)
			{
				MCperror->add(PE_LOCAL_SHADOW, sp);
				delete v;
				delete e;
				return PS_ERROR;
			}
		}

		MCVarref *tvar = NULL;
		MCAutoValueRef init;
		bool initialised = false;
		if (sp.skip_token(SP_FACTOR, TT_BINOP, O_EQ) == PS_NORMAL)
		{
            // MW-2014-11-06: [[ Bug 3680 ]] If there is nothing after '=' it's an error.
			if (sp.next(type) != PS_NORMAL)
			{
				if (constant)
					MCperror->add(PE_CONSTANT_BADINIT, sp);
				else
					MCperror->add(PE_LOCAL_BADINIT, sp);
				return PS_ERROR;
			}
            
            // MW-2014-11-06: [[ Bug 3680 ]] We allow either - or + next, but only if the
            //   next token is a number.
			if (type == ST_MIN || (type == ST_OP && sp.token_is_cstring("+")))
			{
                bool t_is_minus = type == ST_MIN;
                // negative or positive initializer
				if (sp.next(type) != PS_NORMAL || type != ST_NUM)
				{
					if (constant)
						MCperror->add(PE_CONSTANT_BADINIT, sp);
					else
						MCperror->add(PE_LOCAL_BADINIT, sp);
					return PS_ERROR;
				}
                // PM-2015-01-30: [[ Bug 14439 ]] Make sure minus sign is not ignored when assigning value to var at declaration
                if (t_is_minus)
                    /* UNCHECKED */ MCStringFormat((MCStringRef&)&init, "-%@", sp.gettoken_stringref());
                else
                {
                    /* Use the name form of the token to ensure initializers are
                     * always unique. */
                    init = sp.gettoken_nameref();
                }
			}
			else
            {
                // MW-2014-11-06: [[ Bug 3680 ]] If we are in explicit var mode, and the token
                //   is not a string literal or a number, then it must be a constant in the constant
                //   table that is the same as its token.
                if (MCexplicitvariables && type == ST_ID)
                {
                    // If the unquoted literal is a recognised constant and the constant's value
                    // is identical (case-sensitively) to the value, it is fine to make it a literal.
                    if (sp . constantnameconvertstoconstantvalue())
                        type = ST_LIT;
                }
                
                // MW-2014-11-06: [[ Bug 3680 ]] If now, explicitvariables is on and we don't have a literal or
                //   a number, its an error.
                if (MCexplicitvariables && type != ST_LIT && type != ST_NUM)
                {
					if (constant)
						MCperror->add(PE_CONSTANT_BADINIT, sp);
					else
						MCperror->add(PE_LOCAL_BADINIT, sp);
					return PS_ERROR;
                }
                
                /* Use the name form of the token to ensure initializers are
                 * always unique. */
                init = sp.gettoken_nameref();
            }

			initialised = true;
		}
		else if (constant)
			{
				MCperror->add(PE_CONSTANT_NOINIT, sp);
				return PS_ERROR;
			}

		MCAutoValueRef t_init_value;
		if (initialised)
			/* UNCHECKED */ t_init_value = *init;
		else
			t_init_value = kMCNull;

		if (sp.gethandler() == NULL)
		{
			if (constant)
				sp.gethlist()->newconstant(*t_token_name, *t_init_value);
			else if (sp.gethlist()->newvar(*t_token_name, *t_init_value, &tvar, initialised) != PS_NORMAL)
				{
					MCperror->add(PE_LOCAL_BADNAME, sp);
					return PS_ERROR;
				}

		}
		else if (constant)
			sp.gethandler()->newconstant(*t_token_name, *t_init_value);
		else if (sp.gethandler()->newvar(*t_token_name, *t_init_value, &tvar) != PS_NORMAL)
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
						newstatement = new (nothrow) MCComref(sp.gettoken_nameref());
					else if (type == ST_DATA)
						newstatement = new (nothrow) MCEcho;
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

void MCIf::exec_ctxt(MCExecContext &ctxt)
{
    MCKeywordsExecIf(ctxt, cond, thenstatements, elsestatements, line, pos);
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
                {
                    // SN-2015-06-18: [[ Bug 15509 ]] repeat for <n> times
                    //  should get the whole line parsed, not only the <n> expr
                    //  Otherwise,
                    //      repeat for 4 garbage words that are not parsed
                    //          put "a"
                    //      end repeat
                    //  is parsed with no issue
                    bool t_is_for_each;
                    t_is_for_each = false;
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
                        
                        t_is_for_each = true;
					}
                    
                    // SN-2015-06-18: [[ Bug 15509 ]] Both 'repeat for each' and
                    //  'repeat for <expr> times' need an expression
                    if (sp.parseexp(False, True, &endcond) != PS_NORMAL)
                    {
                        MCperror->add
                        (PE_REPEAT_BADCOND, sp);
                        return PS_ERROR;
                    }
                    
                    //  SN-2015-06-18: [[ Bug 15509 ]] In case we have not
                    //  reached the end of the line after parsing the expression
                    //  we have two possibilies:
                    //    - in a 'repeat for each' loop, error
                    //    - in a 'repeat for x times', error only if the line
                    //      does not finish with 'times'
                    if (sp.next(type) != PS_EOL
                            && !(!t_is_for_each
                                 && sp.lookup(SP_REPEAT, te) == PS_NORMAL
                                 && te -> which == RF_TIMES
                                 && sp.next(type) == PS_EOL))
                    {
                        MCperror->add
                        (PE_REPEAT_BADCOND, sp);
                        return PS_ERROR;
                    }
                }
                    break;
				case RF_UNTIL:
                case RF_WHILE:
                    // SN-2015-06-17: [[ Bug 15509 ]] We should reach the end
                    //  of the line after having parsed the expression.
                    //  That mimics the behaviour of MCIf::parse, where a <then>
                    //  is compulsary after the expression parsed (here, an EOL)
                    if (sp.parseexp(False, True, &endcond) != PS_NORMAL
                            || sp.next(type) != PS_EOL)
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
                    {
                        if (sp.parseexp(False, True, &step) != PS_NORMAL)
                        {
                            MCperror->add
                            (PE_REPEAT_BADWITHSTARTEXP, sp);
                            return PS_ERROR;
                        }
                    }
                    else if (sp.next(type) != PS_EOL)
                    {
                        MCperror -> add
                        (PE_REPEAT_BADCOND, sp);
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
				newstatement = new (nothrow) MCEcho;
			else if (sp.lookup(SP_COMMAND, te) != PS_NORMAL)
			{
				if (type == ST_ID)
					newstatement = new (nothrow) MCComref(sp.gettoken_nameref());
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

void MCRepeat::exec_ctxt(MCExecContext& ctxt)
{
    switch (form)
	{
        case RF_FOR:
            if (loopvar != nil)
                MCKeywordsExecRepeatFor(ctxt, statements, endcond, loopvar, each, line, pos);
            else
                MCKeywordsExecRepeatCount(ctxt, statements, endcond, line, pos);
            break;
        case RF_WITH:
            MCKeywordsExecRepeatWith(ctxt, statements, step, startcond, endcond, loopvar, stepval, line, pos);
            break;
        case RF_FOREVER:
            MCKeywordsExecRepeatForever(ctxt, statements, line, pos);
            break;
        case RF_UNTIL:
            MCKeywordsExecRepeatUntil(ctxt, statements, endcond, line, pos);
            break;
        case RF_WHILE:
            MCKeywordsExecRepeatWhile(ctxt, statements, endcond, line, pos);
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
        // PM-2014-04-14: [[Bug 12107]] Do this check to avoid a crash in LC server
		if (sp.gethandler() == nil || !sp.gethandler()->hasname(sp.gettoken_nameref()))
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

void MCExit::exec_ctxt(MCExecContext& ctxt)
{
    MCKeywordsExecExit(ctxt, exit);
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


void MCNext::exec_ctxt(MCExecContext& ctxt)
{
    MCKeywordsExecNext(ctxt);
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

uint4 MCPass::linecount()
{
	return 0;
}

void MCPass::exec_ctxt(MCExecContext& ctxt)
{
	if (all)
		MCKeywordsExecPassAll(ctxt);
	else
		MCKeywordsExecPass(ctxt);
}

void MCBreak::exec_ctxt(MCExecContext& ctxt)
{
    MCKeywordsExecBreak(ctxt);
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
	delete[] cases; /* Allocated with new[] */
	deletestatements(statements);
	delete[] caseoffsets; /* Allocated with new[] */
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
				newstatement = new (nothrow) MCEcho;
			else if (sp.lookup(SP_COMMAND, te) != PS_NORMAL)
			{
				if (type == ST_ID)
					newstatement = new (nothrow) MCComref(sp.gettoken_nameref());
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
                    // SN-2015-06-16: [[ Bug 15509 ]] We should reach the end
                    //  of the line after having parsed the <case> expression.
                    //  That mimics the behaviour of MCIf::parse, where a <then>
                    //  is compulsary after the expression parsed (here, an EOL)
                    if (sp.parseexp(False, True, &cases[ncases]) != PS_NORMAL
                            || sp.next(type) != PS_EOL)
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

void MCThrowKeyword::exec_ctxt(MCExecContext& ctxt)
{
	MCAutoStringRef t_error;
	if (!ctxt . EvalExprAsStringRef(error, EE_THROW_BADERROR, &t_error))
		return;
	
	MCKeywordsExecThrow(ctxt, *t_error);
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
				newstatement = new (nothrow) MCEcho;
			else if (sp.lookup(SP_COMMAND, te) != PS_NORMAL)
			{
				if (type == ST_ID)
					newstatement = new (nothrow) MCComref(sp.gettoken_nameref());
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
				case TT_FINALLY:
					state = TS_FINALLY;
					curstatement = NULL;
					continue;
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

void MCTry::exec_ctxt(MCExecContext& ctxt)
{
    MCKeywordsExecTry(ctxt, trystatements, catchstatements, finallystatements, errorvar, line, pos);
}

uint4 MCTry::linecount()
{
	return countlines(trystatements) + countlines(catchstatements)
	       + countlines(finallystatements);
}

////////////////////////////////////////////////////////////////////////////////

MCHandref::MCHandref(MCNameRef inname)
    : name(inname)
{
    handler = nil;
    params = NULL;
    resolved = false;
    global_handler = false;
    container_count = 0;
}

MCHandref::~MCHandref()
{
    while (params != NULL)
    {
        MCParameter *tmp = params;
        params = params->getnext();
        delete tmp;
    }
}

void MCHandref::parse(void)
{
    if (params != nullptr)
    {
        container_count = params->count_containers();
    }
    
    if (MCIsGlobalHandler(*name))
    {
        global_handler = true;
        resolved = true;
    }
}

void MCHandref::exec(MCExecContext& ctxt, uint2 line, uint2 pos, bool is_function)
{
    if (!resolved)
    {
        MCKeywordsExecResolveCommandOrFunction(ctxt,
                                               *name,
                                               is_function,
                                               handler);
        resolved = true;
    }
    
    /* Attempt to allocate the number of containers needed for the call. */
    MCAutoPointer<MCContainer[]> t_containers = new MCContainer[container_count];
    if (!t_containers)
    {
        ctxt.LegacyThrow(EE_NO_MEMORY);
        return;
    }
    
    /* If the argument list is successfully evaluated, then do the function
     * execution. */
    if (MCKeywordsExecSetupCommandOrFunction(ctxt,
                                             params,
                                             *t_containers,
                                             line,
                                             pos,
                                             is_function))
    {
        MCKeywordsExecCommandOrFunction(ctxt,
                                        handler,
                                        params,
                                        *name,
                                        line,
                                        pos,
                                        global_handler,
                                        is_function);
    }
    
    /* Clean up the evaluated argument list */
    MCKeywordsExecTeardownCommandOrFunction(params);
}

MCComref::MCComref(MCNameRef n)
    : command(n)
{
}

MCComref::~MCComref(void)
{
}

Parse_stat MCComref::parse(MCScriptPoint &sp)
{
    initpoint(sp);
    if (getparams(sp, command.getparams()) != PS_NORMAL)
    {
        MCperror->add(PE_STATEMENT_BADPARAMS, sp);
        return PS_ERROR;
    }
    
    command.parse();
    
    return PS_NORMAL;
}

void MCComref::exec_ctxt(MCExecContext& ctxt)
{
    /* Execute the command */
    
    command.exec(ctxt, line, pos, false);
    
    /* If an error occurred, then we are done */
    
    if (ctxt.HasError())
    {
        return;
    }
    
    /* Process the result according to the result mode */
    
    if (MCresultmode == kMCExecResultModeReturn)
    {
        // Do nothing!
    }
    else if (MCresultmode == kMCExecResultModeReturnValue)
    {
        // Set 'it' to the result and clear the result
        MCAutoValueRef t_value;
        if (!MCresult->eval(ctxt, &t_value))
        {
            ctxt.Throw();
            return;
        }
        
        ctxt.SetItToValue(*t_value);
        ctxt.SetTheResultToEmpty();
    }
    else if (MCresultmode == kMCExecResultModeReturnError)
    {
        // Set 'it' to empty
        ctxt.SetItToEmpty();
        // Leave the result as is but make sure we reset the 'return mode' to default.
        MCresultmode = kMCExecResultModeReturn;
    }
}

MCFuncref::MCFuncref(MCNameRef n)
    : function(n)
{
}

MCFuncref::~MCFuncref(void)
{
}

Parse_stat MCFuncref::parse(MCScriptPoint &sp, Boolean the)
{
    initpoint(sp);
    if (getparams(sp, function.getparams()) != PS_NORMAL)
    {
        MCperror->add(PE_FUNCTION_BADPARAMS, sp);
        return PS_ERROR;
    }
    
    function.parse();
    
    return PS_NORMAL;
}

void MCFuncref::eval_ctxt(MCExecContext& ctxt, MCExecValue& r_value)
{
    /* Execute the function */
    
    function.exec(ctxt, line, pos, true);
    
    /* If an error occurred, then we are done */

    if (ctxt.HasError())
    {
        return;
    }
    
    /* Process the result according to the result mode */
    
    if (MCresultmode == kMCExecResultModeReturn)
    {
        if (MCresult->eval(ctxt, r_value . valueref_value))
        {
            r_value . type = kMCExecValueTypeValueRef;
            return;
        }
    }
    else if (MCresultmode == kMCExecResultModeReturnValue)
    {
        // Our return value is MCresult, and 'the result' gets set to empty.
        if (MCresult->eval(ctxt, r_value . valueref_value))
        {
            r_value . type = kMCExecValueTypeValueRef;
            ctxt.SetTheResultToEmpty();
            return;
        }
    }
    else if (MCresultmode == kMCExecResultModeReturnError)
    {
        // Our return value is empty, and 'the result' remains as it is.
        MCExecTypeSetValueRef(r_value, MCValueRetain(kMCEmptyString));
        
        // Make sure we reset the 'return mode' to default.
        MCresultmode = kMCExecResultModeReturn;
        return;
    }
    
    ctxt . Throw();
}

////////////////////////////////////////////////////////////////////////////////

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
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "scriptpt.h"
#include "execpt.h"
#include "debug.h"
#include "hndlrlst.h"
#include "handler.h"
#include "param.h"
#include "statemnt.h"
#include "literal.h"
#include "mcerror.h"
#include "newobj.h"
#include "object.h"
#include "util.h"
#include "dispatch.h"
#include "globals.h"
#include "cmds.h"
#include "license.h"
#include "redraw.h"

Boolean MCHandler::gotpass;

extern LT factor_table[];
extern const uint4 factor_table_size;

MCHandler::MCHandler(uint1 htype, bool p_is_private)
{
	statements = NULL;
	vars = NULL;
	params = NULL;
	pinfo = NULL;
	vinfo = NULL;
	cinfo = NULL;
	nglobals = nparams = nvnames = npnames = nconstants = executing = 0;
	globals = NULL;
	nglobals = 0;
	prop = P_UNDEFINED;
	array = False;
	type = htype;
	fileindex = 0;
	is_private = p_is_private ? True : False;
	name = nil;
}

MCHandler::~MCHandler()
{
	MCStatement *stmp;
	while (statements != NULL)
	{
		stmp = statements;
		statements = statements->getnext();
		delete stmp;
	}

	for(uint32_t i = 0; i < nvnames; i++)
	{
		MCNameDelete(vinfo[i] . name);
		MCNameDelete(vinfo[i] . init);
	}
	delete vinfo;

	for(uint32_t i = 0; i < npnames; i++)
		MCNameDelete(pinfo[i] . name);
	delete pinfo;

	delete globals;

	for(uint32_t i = 0; i < nconstants; i++)
	{
		MCNameDelete(cinfo[i] . name);
		MCNameDelete(cinfo[i] . value);
}
	delete cinfo;

	MCNameDelete(name);
}

Parse_stat MCHandler::newparam(MCScriptPoint& sp)
{
	MCString t_token;
	t_token = sp . gettoken();

	MCString t_token_name;
	bool t_is_reference;
	if (t_token . getstring()[0] != '@')
	{
		t_is_reference = false;
		t_token_name = t_token;
	}
	else
	{
		t_is_reference = true;
		t_token_name . set(t_token . getstring() + 1, t_token . getlength() - 1);
	}

	MCNameRef t_name;
	/* UNCHECKED */ MCNameCreateWithOldString(t_token_name, t_name);

	// OK-2010-01-11: [[Bug 7744]] - Check existing parsed parameters for duplicates.
	for (uint2 i = 0; i < npnames; i++)
	{
		if (MCNameIsEqualTo(pinfo[i] . name, t_name, kMCCompareCaseless))
		{
			MCNameDelete(t_name);
			MCperror -> add(PE_HANDLER_DUPPARAM, sp);
			return PS_ERROR;
		}
	}
		
	MCU_realloc((char **)&pinfo, npnames, npnames + 1, sizeof(MCHandlerParamInfo));
	pinfo[npnames] . is_reference = t_is_reference;
	pinfo[npnames] . name = t_name;
	npnames++;

	return PS_NORMAL;
}

Parse_stat MCHandler::parse(MCScriptPoint &sp, Boolean isprop)
{
	Parse_stat stat;
	Symbol_type type;

	firstline = sp.getline();
	hlist = sp.gethlist();
    
	if (sp.next(type) != PS_NORMAL)
	{
		MCperror->add(PE_HANDLER_NONAME, sp);
		return PS_ERROR;
	}

	/* UNCHECKED */ MCNameClone(sp . gettoken_nameref(), name);
    
    if (isprop)
    {
        uint2 i;
        for (i = 0 ; i < factor_table_size ; i++)
            if (factor_table[i].type == TT_PROPERTY)
            {
                if (MCNameIsEqualToCString(name, factor_table[i].token, kMCCompareCaseless))
                    prop = (Properties)factor_table[i].which;
            }
        if (prop == P_UNDEFINED)
            prop = P_CUSTOM;
    }
    else
        prop = P_UNDEFINED;
	
	const LT *te;
	// MW-2010-01-08: [[Bug 7792]] Check whether the handler name is a reserved function identifier
	if (type != ST_ID ||
			sp.lookup(SP_COMMAND, te) != PS_NO_MATCH ||
			(sp.lookup(SP_FACTOR, te) != PS_NO_MATCH &&
			te -> type == TT_FUNCTION))
	{
		MCperror->add(PE_HANDLER_BADNAME, sp);
		return PS_ERROR;
	}
	if (prop)
	{
		if (sp.next(type) == PS_NORMAL)
		{
			if (type == ST_LB)
			{
				if (sp.next(type) != PS_NORMAL || type != ST_ID)
				{
					MCperror->add(PE_HANDLER_BADPARAM, sp);
					return PS_ERROR;
				}

				if (newparam(sp) != PS_NORMAL)
					return PS_ERROR;

				if (sp.next(type) != PS_NORMAL || type != ST_RB)
				{
					MCperror->add(PE_HANDLER_BADPARAM, sp);
					return PS_ERROR;
				}
				array = True;
			}
			else
				sp.backup();
		}
	}
	while (sp.next(type) == PS_NORMAL)
	{
		if (type == ST_SEP)
			continue;
		const LT *te;
		MCExpression *newfact = NULL;
		if (type != ST_ID
		        || sp.lookup(SP_FACTOR, te) != PS_NO_MATCH
		        || sp.lookupconstant(&newfact) == PS_NORMAL)
		{
			delete newfact;
			MCperror->add(PE_HANDLER_BADPARAM, sp);
			return PS_ERROR;
		}

		if (newparam(sp) != PS_NORMAL)
				return PS_ERROR;
			}
		
	if (sp.skip_eol() != PS_NORMAL)
	{
		MCperror->add(PE_HANDLER_BADPARAMEOL, sp);
		return PS_ERROR;
	}
	sp.sethandler(this);
	MCStatement *curstatement = NULL;
	MCStatement *newstatement = NULL;
	while (True)
	{
		if ((stat = sp.next(type)) != PS_NORMAL)
		{
			if (stat == PS_EOL)
			{
				if (sp.skip_eol() != PS_NORMAL)
				{
					MCperror->add(PE_HANDLER_BADLINE, sp);
					return PS_ERROR;
				}
				else
					continue;
			}
			else
			{
				MCperror->add(PE_HANDLER_NOCOMMAND, sp);
				return PS_ERROR;
			}
		}
		if (type == ST_DATA)
			newstatement = new MCEcho;
		else if (sp.lookup(SP_COMMAND, te) != PS_NORMAL)
		{
			if (type != ST_ID)
			{
				MCperror->add(PE_HANDLER_NOCOMMAND, sp);
				return PS_ERROR;
			}
			newstatement = new MCComref(sp.gettoken_nameref());
		}
		else
		{
			switch (te->type)
			{
			case TT_STATEMENT:
				newstatement = MCN_new_statement(te->which);
				break;
			case TT_END:
				if ((stat = sp.next(type)) != PS_NORMAL)
				{
					MCperror->add(PE_HANDLER_NOEND, sp);
					return PS_ERROR;
				}
				if (!MCNameIsEqualTo(name, sp.gettoken_nameref(), kMCCompareCaseless))
				{
					MCperror->add(PE_HANDLER_BADEND, sp);
					return PS_ERROR;
				}
				lastline = sp.getline();
				sp.skip_eol();
				return PS_NORMAL;
			default:
				MCperror->add(PE_HANDLER_NOTCOMMAND, sp);
				return PS_ERROR;
			}
		}
		if (newstatement->parse(sp) != PS_NORMAL)
		{
			MCperror->add(PE_HANDLER_BADCOMMAND, sp);
			delete newstatement;
			return PS_ERROR;
		}
		if (curstatement == NULL)
			statements = curstatement = newstatement;
		else
		{
			curstatement->setnext(newstatement);
			curstatement = newstatement;
		}
	}
	return PS_NORMAL;
}

Exec_stat MCHandler::exec(MCExecPoint &ep, MCParameter *plist)
{
	uint2 i;
	MCParameter *tptr = plist;
	if (prop && !array && plist != NULL)
		plist = plist->getnext();
	for (npassedparams = 0 ; tptr != NULL ; npassedparams++)
		tptr = tptr->getnext();
	uint2 newnparams = MCU_max(npassedparams, npnames);
	MCVariable **newparams;
	if (newnparams == 0)
		newparams = NULL;
	else
		newparams = new MCVariable *[newnparams];

	Boolean err = False;
	for (i = 0 ; i < newnparams ; i++)
	{
		if (plist != NULL)
		{
			if (i < npnames && pinfo[i].is_reference)
			{
				if ((newparams[i] = plist->eval_argument_var()) == NULL)
				{
					err = True;
					break;
				}
			}
			else
			{
				if (plist->eval_argument(ep) != ES_NORMAL)
				{
					err = True;
					break;
				}
				/* UNCHECKED */ MCVariable::createwithname(i < npnames ? pinfo[i] . name : kMCEmptyName, newparams[i]);
				newparams[i]->store(ep, False);
			}
			plist = plist->getnext();
		}
		else
		{
			if (i < npnames && pinfo[i].is_reference)
			{
				err = True;
				break;
			}
			/* UNCHECKED */ MCVariable::createwithname(i < npnames ? pinfo[i] . name : kMCEmptyName, newparams[i]);
		}
	}
	if (err)
	{
		while (i--)
			if (i >= npnames || !pinfo[i].is_reference)
				delete newparams[i];
		delete newparams;
		MCeerror->add(EE_HANDLER_BADPARAM, firstline - 1, 1, name);
		return ES_ERROR;
	}

	MCVariable **oldparams = params;
	MCVariable **oldvars = vars;
	uint2 oldnparams = nparams;
	uint2 oldnvnames = nvnames;
	uint2 oldnconstants = nconstants;
	params = newparams;
	nparams = newnparams;
	if (nvnames == 0)
		vars = NULL;
	else
	{
		vars = new MCVariable *[nvnames];
		i = nvnames;
		while (i--)
		{
			/* UNCHECKED */ MCVariable::createwithname(vinfo[i] . name, vars[i]);

			// A UQL is indicated by 'init' being nil.
			if (vinfo[i] . init != nil)
				vars[i] -> setnameref_unsafe(vinfo[i] . init);
			else
			{
				// At the moment UQL detection relies on the fact that the 'name'
				// and 'value' of a variable share the same base ptr as well as 'is_uql'
				// being set.
				vars[i] -> setnameref_unsafe(vinfo[i] . name);
				vars[i] -> setuql();
		}
	}
	}

	executing++;
	MCresult->clear(False);
	Exec_stat stat = ES_NORMAL;
	MCStatement *tspr = statements;

	if ((MCtrace || MCnbreakpoints) && tspr != NULL)
	{
		MCB_trace(ep, firstline, 0);

		// OK-2008-09-05: [[Bug 7115]] - Debugger doesn't stop if traceAbort is set following a breakpoint on the first line of a handler.
		if (MCexitall)
			tspr = NULL;
	}
	while (tspr != NULL)
	{
		if (MCtrace || MCnbreakpoints)
		{
			MCB_trace(ep, tspr->getline(), tspr->getpos());
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
				tspr = NULL;
			else
				tspr = tspr->getnext();
			break;
		case ES_EXIT_REPEAT:
		case ES_NEXT_REPEAT:
		case ES_ERROR:
			if ((MCtrace || MCnbreakpoints) && !MCtrylock && !MClockerrors)
				do
				{
					MCB_error(ep, tspr->getline(), tspr->getpos(), EE_HANDLER_BADSTATEMENT);
				}
				while (MCtrace && (stat = tspr->exec(ep)) != ES_NORMAL);
			if (stat != ES_NORMAL)
			{
				MCeerror->add(EE_HANDLER_BADSTATEMENT, tspr->getline(), tspr->getpos(), name);
				if (MCexitall)
					stat = ES_NORMAL;
				else
					stat = ES_ERROR;
				tspr = NULL;
			}
			else
				tspr = tspr->getnext();
			break;
		case ES_RETURN_HANDLER:
			tspr = NULL;
			break;
		case ES_EXIT_HANDLER:
		case ES_EXIT_SWITCH:
			tspr = NULL;
			stat = ES_NORMAL;
			break;
		default:
			tspr = NULL;
			break;
		}
	}

	// MW-2007-07-03: [[ Bug 4570 ]] - Exiting a handler except via return should
	//   clear the result.
	// MW-2007-09-17: [[ Bug 4570 ]] - REVERTING due to backwards-compatibility
	//   problems.
	if (stat == ES_RETURN_HANDLER)
		stat = ES_NORMAL;

	if (!MCexitall && (MCtrace || MCnbreakpoints))
		MCB_trace(ep, lastline, 0);

	executing--;
	if (params != NULL)
	{
		i = newnparams;
		while (i--)
			if (i >= npnames || !pinfo[i].is_reference)
				delete params[i];
		delete params;
	}
	if (vars != NULL)
	{
		while (nvnames--)
		{
			if (nvnames >= oldnvnames)
			{
				MCNameDelete(vinfo[nvnames] . name);
				MCNameDelete(vinfo[nvnames] . init);
			}
			delete vars[nvnames];
		}
		delete vars;
	}
	params = oldparams;
	nparams = oldnparams;
	vars = oldvars;
	nvnames = oldnvnames;
	nconstants = oldnconstants;
	if (stat == ES_PASS)
		gotpass = True;  // so MCObject::timer can distinguish pass from not handled
	return stat;
}

Exec_stat MCHandler::getnparams(uint2 &index)
{
	index = npassedparams;
	return ES_NORMAL;
}

// MW-2007-07-03: [[ Bug 3174 ]] - Non-declared parameters accessed via 'param' should
//   be considered to be both empty and 0 as they are akin to undeclared variables.
Exec_stat MCHandler::getparam(uint2 index, MCExecPoint &ep)
{
	if (index == 0)
		ep.setnameref_unsafe(name);
	else if (index > nparams)
		ep.setboth(MCnullmcstring, 0);
	else if (params[index - 1]->fetch(ep) != ES_NORMAL)
	{
		MCeerror->add(EE_HANDLER_BADPARAM, 0, 0);
		return ES_ERROR;
	}

	return ES_NORMAL;
}

MCVariable *MCHandler::getit(void)
{
	for(uint32_t i = 0; i < nvnames; i++)
		if (MCNameIsEqualTo(vinfo[i] . name, MCN_it, kMCCompareCaseless))
			return vars[i];
	return NULL;
}

Parse_stat MCHandler::findvar(MCNameRef p_name, MCVarref **dptr)
{
	uint2 i;
	for (i = 0 ; i < nvnames ; i++)
		if (MCNameIsEqualTo(p_name, vinfo[i] . name, kMCCompareCaseless))
		{
			*dptr = new MCVarref(this, i, False);
			return PS_NORMAL;
		}

	for (i = 0 ; i < npnames ; i++)
		if (MCNameIsEqualTo(p_name, pinfo[i] . name, kMCCompareCaseless))
	{
			*dptr = new MCVarref(this, i, True);
			return PS_NORMAL;
		}

	for (i = 0 ; i < nglobals ; i++)
	{
		if (globals[i]->hasname(p_name))
		{
			*dptr = globals[i] -> newvarref();
			return PS_NORMAL;
		}
	}

	if (MCNameGetCharAtIndex(p_name, 0) == '$')
	{
		MCVariable *t_global;
		/* UNCHECKED */ MCVariable::ensureglobal(p_name, t_global);
		*dptr = t_global -> newvarref();
		return PS_NORMAL;
	}

	// MW-2011-08-23: [[ UQL ]] Now search in the handler list but ignore any
    //   UQL vars that are there.
	return hlist->findvar(p_name, true, dptr);
}

Parse_stat MCHandler::newvar(MCNameRef p_name, MCNameRef p_init, MCVarref **r_ref)
{
	MCU_realloc((char **)&vinfo, nvnames, nvnames + 1, sizeof(MCHandlerVarInfo));
	/* UNCHECKED */ MCNameClone(p_name, vinfo[nvnames] . name);
	if (p_init != nil)
		/* UNCHECKED */ MCNameClone(p_init, vinfo[nvnames] . init);
	else
		vinfo[nvnames] . init = nil;

	if (executing)
	{
		MCU_realloc((char **)&vars, nvnames, nvnames + 1, sizeof(MCVariable *));
		/* UNCHECKED */ MCVariable::createwithname(p_name, vars[nvnames]);

		if (p_init != nil)
			vars[nvnames] -> setnameref_unsafe(p_init);
		else
		{
			vars[nvnames] -> setnameref_unsafe(p_name);
			vars[nvnames] -> setuql();
		}
	}

	*r_ref = new MCVarref(this, nvnames++, False);

	return PS_NORMAL;
}

Parse_stat MCHandler::findconstant(MCNameRef p_name, MCExpression **dptr)
{
	uint2 i;
	for (i = 0 ; i < nconstants ; i++)
		if (MCNameIsEqualTo(p_name, cinfo[i].name, kMCCompareCaseless))
		{
			*dptr = new MCLiteral(cinfo[i].value);
			return PS_NORMAL;
		}
	return hlist->findconstant(p_name, dptr);
}

Parse_stat MCHandler::newconstant(MCNameRef p_name, MCNameRef p_value)
{
	MCU_realloc((char **)&cinfo, nconstants, nconstants + 1, sizeof(MCHandlerConstantInfo));
	/* UNCHECKED */ MCNameClone(p_name, cinfo[nconstants].name);
	/* UNCHECKED */ MCNameClone(p_value, cinfo[nconstants++].value);
	return PS_NORMAL;
}

void MCHandler::newglobal(MCNameRef p_name)
{
	uint2 i;
	for (i = 0 ; i < nglobals ; i++)
		if (globals[i]->hasname(p_name))
			return;

	MCVariable *gptr;
	/* UNCHECKED */ MCVariable::ensureglobal(p_name, gptr);

	MCU_realloc((char **)&globals, nglobals, nglobals + 1, sizeof(MCVariable *));
	globals[nglobals++] = gptr;
}

Exec_stat MCHandler::getvarnames(MCExecPoint &ep, Boolean all)
{
	ep.setempty();

	for(uint2 i = 0 ; i < npnames ; i++)
		ep.concatnameref(pinfo[i] . name, EC_COMMA, i == 0);
	ep.appendnewline();

	for(uint2 i = 0 ; i < nvnames ; i++)
		ep.concatnameref(vinfo[i] . name, EC_COMMA, i == 0);
	ep.appendnewline();

	hlist->appendlocalnames(ep);
	if (all)
	{
		ep.appendnewline();

		// OK-2008-06-25: <Bug where the variableNames property would return duplicate global names>
		for (uint2 i = 0 ; i < nglobals ; i++)
			ep . concatnameref(globals[i] -> getname(), EC_COMMA, i == 0);

		for (uint2 i = 0; i < hlist -> getnglobals(); i++)
		{
			bool t_already_appended;
			t_already_appended = false;
			for (uint2 j = 0; j < nglobals; j++)
			{
				if (globals[j] -> hasname(hlist -> getglobal(i) -> getname()))
				{
					t_already_appended = true;
					continue;
				}
			}

			// OK-2008-08-14: [[Bug 6883]] - Problem where comma delimiter was missed off if nglobals = 0.
			if (!t_already_appended)
				ep . concatnameref(hlist -> getglobal(i) -> getname(), EC_COMMA, (nglobals == 0 && i == 0));
		}
	}
	return ES_NORMAL;
}

Exec_stat MCHandler::eval(MCExecPoint &ep)
{
	MCScriptPoint sp(ep);
	sp.sethandler(this);
	MCExpression *exp = NULL;
	Symbol_type type;
	Exec_stat stat = ES_ERROR;
	if (sp.parseexp(False, True, &exp) == PS_NORMAL && sp.next(type) == PS_EOF)
	{
		stat = exp->eval(ep);
		ep.grabsvalue();
	}
	delete exp;
	return stat;
}

uint4 MCHandler::linecount()
{
	uint4 count = 0;
	MCStatement *stmp = statements;
	while (stmp != NULL)
	{
		count += stmp->linecount();
		stmp = stmp->getnext();
	}
	return count;
}

void MCHandler::deletestatements(MCStatement *statements)
{
	while (statements != NULL)
	{
		MCStatement *tsptr = statements;
		statements = statements->getnext();
		delete tsptr;
	}
}

Exec_stat MCHandler::doscript(MCExecPoint &ep, uint2 line, uint2 pos)
{
	MCScriptPoint sp(ep);
	MCStatement *curstatement = NULL;
	MCStatement *statements = NULL;
	MCStatement *newstatement = NULL;
	Symbol_type type;
	const LT *te;
	Exec_stat stat = ES_NORMAL;
	Boolean oldexplicit = MCexplicitvariables;
	MCexplicitvariables = False;
	uint4 count = 0;
	sp.setline(line - 1);
	while (stat == ES_NORMAL)
	{
		switch (sp.next(type))
		{
		case PS_NORMAL:
			if (type == ST_ID)
				if (sp.lookup(SP_COMMAND, te) != PS_NORMAL)
					newstatement = new MCComref(sp.gettoken_nameref());
				else
				{
					if (te->type != TT_STATEMENT)
					{
						MCeerror->add(EE_DO_NOTCOMMAND, line, pos, sp.gettoken());
						stat = ES_ERROR;
					}
					else
						newstatement = MCN_new_statement(te->which);
				}
			else
			{
				MCeerror->add(EE_DO_NOCOMMAND, line, pos, sp.gettoken());
				stat = ES_ERROR;
			}
			if (stat == ES_NORMAL)
			{
				if (curstatement == NULL)
					statements = curstatement = newstatement;
				else
				{
					curstatement->setnext(newstatement);
					curstatement = newstatement;
				}
				if (curstatement->parse(sp) != PS_NORMAL)
				{
					MCeerror->add(EE_DO_BADCOMMAND, line, pos, ep.getsvalue());
					stat = ES_ERROR;
				}
				count += curstatement->linecount();
			}
			break;
		case PS_EOL:
			if (sp.skip_eol() != PS_NORMAL)
			{
				MCeerror->add(EE_DO_BADLINE, line, pos, ep.getsvalue());
				stat = ES_ERROR;
			}
			break;
		case PS_EOF:
			stat = ES_PASS;
			break;
		default:
			stat = ES_ERROR;
		}
	}
	MCexplicitvariables = oldexplicit;

	if (MClicenseparameters . do_limit > 0 && count >= MClicenseparameters . do_limit)
	{
		MCeerror -> add(EE_DO_NOTLICENSED, line, pos, ep . getsvalue());
		stat = ES_ERROR;
	}

	if (stat == ES_ERROR)
	{
		deletestatements(statements);
		return ES_ERROR;
	}
	MCExecPoint ep2(ep);
	while (statements != NULL)
	{
		Exec_stat stat = statements->exec(ep2);
		if (stat == ES_ERROR)
		{
			deletestatements(statements);
			MCeerror->add(EE_DO_BADEXEC, line, pos, ep.getsvalue());
			return ES_ERROR;
		}
		if (MCexitall || stat != ES_NORMAL)
		{
			deletestatements(statements);
			if (stat != ES_ERROR)
				stat = ES_NORMAL;
			return stat;
		}
		else
		{
			MCStatement *tsptr = statements;
			statements = statements->getnext();
			delete tsptr;
		}
	}
	if (MCscreen->abortkey())
	{
		MCeerror->add(EE_DO_ABORT, line, pos);
		return ES_ERROR;
	}
	return ES_NORMAL;
}


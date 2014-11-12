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
//#include "execpt.h"
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

#include "exec.h"

#include "syntax.h"

////////////////////////////////////////////////////////////////////////////////

Boolean MCHandler::gotpass;

////////////////////////////////////////////////////////////////////////////////

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
	prop = False;
	array = False;
	type = htype;
	fileindex = 0;
	is_private = p_is_private ? True : False;
	name = nil;

	// MW-2013-11-08: [[ RefactorIt ]] The it varref is created on parsing.
	m_it = nil;
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
		MCValueRelease(vinfo[i] . init);
	}
	delete vinfo;

	for(uint32_t i = 0; i < npnames; i++)
		MCNameDelete(pinfo[i] . name);
	delete pinfo;

	delete globals;

	for(uint32_t i = 0; i < nconstants; i++)
	{
		MCNameDelete(cinfo[i] . name);
		MCValueRelease(cinfo[i] . value);
	}
	delete cinfo;
	
	// MW-2013-11-08: [[ RefactorIt ]] Delete the it varref.
	delete m_it;

	MCNameDelete(name);
}

Parse_stat MCHandler::newparam(MCScriptPoint& sp)
{
	MCStringRef t_token;
	t_token = sp . gettoken_stringref();

	MCAutoStringRef t_token_name;
	bool t_is_reference;
	if (MCStringGetNativeCharAtIndex(t_token, 0) != '@')
	{
		t_is_reference = false;
		t_token_name = t_token;
	}
	else
	{
		t_is_reference = true;
		/* UNCHECKED */ MCStringCopySubstring(t_token, MCRangeMake(1, MCStringGetLength(t_token) - 1), &t_token_name);
	}

	MCNameRef t_name;
	/* UNCHECKED */ MCNameCreate(*t_token_name, t_name);

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
	prop = isprop;

	if (sp.next(type) != PS_NORMAL)
	{
		MCperror->add(PE_HANDLER_NONAME, sp);
		return PS_ERROR;
	}

	/* UNCHECKED */ MCNameClone(sp . gettoken_nameref(), name);
	
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
    
    bool t_needs_it;
    t_needs_it = true;
    
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
        
        // AL-2014-11-04: [[ Bug 13902 ]] Check if the param we just created was called 'it'.
        if (MCNameIsEqualTo(pinfo[npnames - 1] . name, MCN_it))
            t_needs_it = false;
    }
		
    // AL-2014-11-04: [[ Bug 13902 ]] Only define it as a var if it wasn't one of the parameter names.
    if (t_needs_it)
        /* UNCHECKED */ newvar(MCN_it, kMCEmptyName, &m_it);
    
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

Exec_stat MCHandler::exec(MCExecContext& ctxt, MCParameter *plist)
{
	uint2 i;
	MCParameter *tptr = plist;
	if (prop && !array && plist != NULL)
		plist = plist->getnext();
	for (npassedparams = 0 ; tptr != NULL ; npassedparams++)
		tptr = tptr->getnext();
	uint2 newnparams = MCU_max(npassedparams, npnames);
    
    // AL-2014-08-20: [[ ArrayElementRefParams ]] All handler params are now containers
	MCContainer **newparams;
	if (newnparams == 0)
		newparams = NULL;
	else
		newparams = new MCContainer *[newnparams];
    
	Boolean err = False;
	for (i = 0 ; i < newnparams ; i++)
	{
		if (plist != NULL)
		{
			if (i < npnames && pinfo[i].is_reference)
			{
				if ((newparams[i] = plist->eval_argument_container()) == NULL)
				{
					err = True;
					break;
				}
			}
			else
			{
                MCExecValue t_value;
				if (!plist->eval_argument_ctxt(ctxt, t_value))
				{
					err = True;
					break;
				}
                
                MCVariable *t_new_var;
				/* UNCHECKED */ MCVariable::createwithname(i < npnames ? pinfo[i] . name : kMCEmptyName, t_new_var);
                /* UNCHECKED */ MCContainer::createwithvariable(t_new_var, newparams[i]);
                
				newparams[i]->give_value(ctxt, t_value);
			}
            
            // AL-2014-11-04: [[ Bug 13902 ]] If 'it' was this parameter's name then create the MCVarref as a
            //  param type, with this handler and param index, so that use of the get command syncs up correctly.
            if (i < npnames && MCNameIsEqualTo(pinfo[i] . name, MCN_it))
                m_it = new MCVarref(this, i, True);
            
			plist = plist->getnext();
		}
		else
		{
			if (i < npnames && pinfo[i].is_reference)
			{
				err = True;
				break;
			}
            MCVariable *t_new_var;
            /* UNCHECKED */ MCVariable::createwithname(i < npnames ? pinfo[i] . name : kMCEmptyName, t_new_var);
            /* UNCHECKED */ MCContainer::createwithvariable(t_new_var, newparams[i]);
		}
	}
	if (err)
	{
		while (i--)
        {
            // AL-2014-09-16: [[ Bug 13454 ]] Delete created variables before deleting containers to prevent memory leak
            if (i >= npnames || !pinfo[i].is_reference)
            {
				delete newparams[i] -> getvar();
                delete newparams[i];
            }
        }
		delete newparams;
		MCeerror->add(EE_HANDLER_BADPARAM, firstline - 1, 1, name);
		return ES_ERROR;
	}
    
	MCContainer **oldparams = params;
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
				vars[i] -> setvalueref(vinfo[i] . init);
			else
			{
				// At the moment UQL detection relies on the fact that the 'name'
				// and 'value' of a variable share the same base ptr as well as 'is_uql'
				// being set.
				vars[i] -> setvalueref(vinfo[i] . name);
				vars[i] -> setuql();
			}
		}
	}
    
	executing++;
	ctxt . SetTheResultToEmpty();
	Exec_stat stat = ES_NORMAL;
	MCStatement *tspr = statements;
    
	if ((MCtrace || MCnbreakpoints) && tspr != NULL)
	{
		MCB_trace(ctxt, firstline, 0);
        
		// OK-2008-09-05: [[Bug 7115]] - Debugger doesn't stop if traceAbort is set following a breakpoint on the first line of a handler.
		if (MCexitall)
			tspr = NULL;
	}
	while (tspr != NULL)
	{
		if (MCtrace || MCnbreakpoints)
		{
			MCB_trace(ctxt, tspr->getline(), tspr->getpos());
			if (MCexitall)
				break;
		}
		ctxt.SetLineAndPos(tspr->getline(), tspr->getpos());
        
        tspr->exec_ctxt(ctxt);
		stat = ctxt . GetExecStat();
        
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
                        MCB_error(ctxt, tspr->getline(), tspr->getpos(), EE_HANDLER_BADSTATEMENT);
                        ctxt . IgnoreLastError();
                        tspr->exec_ctxt(ctxt);
                    }
				while (MCtrace && (stat = ctxt . GetExecStat()) != ES_NORMAL);
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
		MCB_trace(ctxt, lastline, 0);
    
	executing--;
	if (params != NULL)
	{
		i = newnparams;
        // AL-2014-08-20: [[ ArrayElementRefParams ]] A container is always created for each parameter,
        //  so delete them all when the handler has finished executing
		while (i--)
        {
            // AL-2014-09-16: [[ Bug 13454 ]] Delete created variables before deleting containers to prevent memory leak
            if (i >= npnames || !pinfo[i].is_reference)
            {
				delete params[i] -> getvar();
                delete params[i];
            }
        }
		delete params;
	}
	if (vars != NULL)
	{
		while (nvnames--)
		{
			if (nvnames >= oldnvnames)
			{
				MCNameDelete(vinfo[nvnames] . name);
				MCValueRelease(vinfo[nvnames] . init);
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


#ifdef LEGACY_EXEC
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
                MCExecContext ctxt(ep);
                MCAutoValueRef t_value;
				if (!plist->eval_argument(ctxt, &t_value))
				{
					err = True;
					break;
				}
				/* UNCHECKED */ MCVariable::createwithname(i < npnames ? pinfo[i] . name : kMCEmptyName, newparams[i]);
				newparams[i]->set(ctxt, *t_value);
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
				vars[i] -> setvalueref(vinfo[i] . init);
			else
			{
				// At the moment UQL detection relies on the fact that the 'name'
				// and 'value' of a variable share the same base ptr as well as 'is_uql'
				// being set.
				vars[i] -> setvalueref(vinfo[i] . name);
				vars[i] -> setuql();
			}
		}
	}

	executing++;
	MCresult->clear(False);
	Exec_stat stat = ES_NORMAL;
	MCStatement *tspr = statements;

	MCExecContext ctxt(ep);
	if ((MCtrace || MCnbreakpoints) && tspr != NULL)
	{
		MCB_trace(ctxt, firstline, 0);

		// OK-2008-09-05: [[Bug 7115]] - Debugger doesn't stop if traceAbort is set following a breakpoint on the first line of a handler.
		if (MCexitall)
			tspr = NULL;
	}
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
					if (!MCB_error(ep, tspr->getline(), tspr->getpos(), EE_HANDLER_BADSTATEMENT))
						break;
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
		MCB_trace(ctxt, lastline, 0);

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
#endif

MCVariable *MCHandler::getvar(uint2 index, Boolean isparam)
{
    return isparam ? nil : vars[index];
}

MCContainer *MCHandler::getcontainer(uint2 index, Boolean isparam)
{
    return isparam ? params[index] : nil;
}

integer_t MCHandler::getnparams(void)
{
	return npassedparams;
}

// MW-2007-07-03: [[ Bug 3174 ]] - Non-declared parameters accessed via 'param' should
//   be considered to be both empty and 0 as they are akin to undeclared variables.
MCValueRef MCHandler::getparam(uindex_t p_index)
{
    if (p_index == 0)
        return name;
    else if (p_index > nparams)
        return kMCEmptyString;
    else
        return params[p_index - 1]->get_valueref();
}

// MW-2013-11-08: [[ RefactorIt ]] Changed to return the 'm_it' varref we always have now.
MCVarref *MCHandler::getit(void)
{
	return m_it;
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

Parse_stat MCHandler::newvar(MCNameRef p_name, MCValueRef p_init, MCVarref **r_ref)
{
	MCU_realloc((char **)&vinfo, nvnames, nvnames + 1, sizeof(MCHandlerVarInfo));
	/* UNCHECKED */ MCNameClone(p_name, vinfo[nvnames] . name);
	if (p_init != nil)
		/* UNCHECKED */ vinfo[nvnames] . init = MCValueRetain(p_init);
	else
		vinfo[nvnames] . init = nil;

	if (executing)
	{
		MCU_realloc((char **)&vars, nvnames, nvnames + 1, sizeof(MCVariable *));
		/* UNCHECKED */ MCVariable::createwithname(p_name, vars[nvnames]);

		if (p_init != nil)
			vars[nvnames] -> setvalueref(p_init);
		else
		{
			vars[nvnames] -> setvalueref(p_name);
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

Parse_stat MCHandler::newconstant(MCNameRef p_name, MCValueRef p_value)
{
	MCU_realloc((char **)&cinfo, nconstants, nconstants + 1, sizeof(MCHandlerConstantInfo));
	/* UNCHECKED */ MCNameClone(p_name, cinfo[nconstants].name);
	/* UNCHECKED */ cinfo[nconstants++].value = MCValueRetain(p_value);
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

bool MCHandler::getparamnames(MCListRef& r_list)
{
	MCAutoListRef t_list;
	if (!MCListCreateMutable(',', &t_list))
		return false;

	for (uinteger_t i = 0; i < npnames; i++)
		if (!MCListAppend(*t_list, pinfo[i].name))
			return false;

	return MCListCopy(*t_list, r_list);
}

bool MCHandler::getvariablenames(MCListRef& r_list)
{
	MCAutoListRef t_list;
	if (!MCListCreateMutable(',', &t_list))
		return false;

	for (uinteger_t i = 0; i < nvnames; i++)
		if (!MCListAppend(*t_list, vinfo[i].name))
			return false;

	return MCListCopy(*t_list, r_list);
}

bool MCHandler::getglobalnames(MCListRef& r_list)
{
	MCAutoListRef t_list;
	if (!MCListCreateMutable(',', &t_list))
		return false;

	// OK-2008-06-25: <Bug where the variableNames property would return duplicate global names>
	for (uint2 i = 0 ; i < nglobals ; i++)
		if (!MCListAppend(*t_list, globals[i]->getname()))
			return false;

	for (uint2 i = 0; i < hlist -> getnglobals(); i++)
	{
		MCNameRef t_global_name = hlist->getglobal(i)->getname();
		bool t_already_appended;
		t_already_appended = false;
		for (uint2 j = 0; (!t_already_appended) && j < nglobals; j++)
			t_already_appended = globals[j] -> hasname(t_global_name);

		if (!t_already_appended)
			if (!MCListAppend(*t_list, t_global_name))
				return false;
	}

	return MCListCopy(*t_list, r_list);
}

bool MCHandler::getvarnames(bool p_all, MCListRef& r_list)
{
	MCAutoListRef t_list;
	if (!MCListCreateMutable('\n', &t_list))
		return false;

	MCAutoListRef t_param_list, t_variable_list, t_script_variable_list;
	if (!(getparamnames(&t_param_list) &&
		MCListAppend(*t_list, *t_param_list)))
		return false;

	if (!(getvariablenames(&t_variable_list) &&
		MCListAppend(*t_list, *t_variable_list)))
		return false;

	if (!(hlist->getlocalnames(&t_script_variable_list) &&
		MCListAppend(*t_list, *t_script_variable_list)))
		return false;

	if (p_all)
	{
		MCAutoListRef t_global_list;
		if (!(getglobalnames(&t_global_list) &&
			MCListAppend(*t_list, *t_global_list)))
			return false;
	}

	return MCListCopy(*t_list, r_list);
}

#ifdef LEGACY_EXEC
Exec_stat MCHandler::getvarnames(MCExecPoint& ep, Boolean all)
{
	MCAutoListRef t_list;
	MCAutoStringRef t_string;
	/* UNCHECKED */ getvarnames(all == True, &t_list);
	/* UNCHECKED */ MCListCopyAsString(*t_list, &t_string);
	/* UNCHECKED */ ep.setvalueref(*t_string);
	return ES_NORMAL;
}
#endif

void MCHandler::eval(MCExecContext &ctxt, MCStringRef p_expression, MCValueRef &r_value)
{
    MCScriptPoint sp(ctxt, p_expression);
	sp.sethandler(this);
    MCExpression *exp = NULL;
    Symbol_type type;

	if (sp.parseexp(False, True, &exp) == PS_NORMAL && sp.next(type) == PS_EOF)
        ctxt . EvalExprAsValueRef(exp, EE_HANDLER_BADEXP, r_value);
    else
        ctxt . Throw();

    delete exp;
}

void MCHandler::eval_ctxt(MCExecContext &ctxt, MCStringRef p_expression, MCExecValue &r_value)
{
    MCScriptPoint sp(ctxt, p_expression);
	sp.sethandler(this);
    MCExpression *exp = NULL;
    Symbol_type type;
    
	if (sp.parseexp(False, True, &exp) == PS_NORMAL && sp.next(type) == PS_EOF)
        ctxt . EvaluateExpression(exp, EE_HANDLER_BADEXP, r_value);
    else
        ctxt . Throw();
    
    delete exp;
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

void MCHandler::doscript(MCExecContext& ctxt, MCStringRef p_script, uinteger_t p_line, uinteger_t p_pos)
{
    MCScriptPoint sp(ctxt, p_script);
	MCStatement *curstatement = NULL;
	MCStatement *statements = NULL;
	MCStatement *newstatement = NULL;
	Symbol_type type;
	const LT *te;
	Exec_stat stat = ES_NORMAL;
	Boolean oldexplicit = MCexplicitvariables;
	MCexplicitvariables = False;
	uint4 count = 0;
	sp.setline(p_line - 1);
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
						MCeerror->add(EE_DO_NOTCOMMAND, p_line, p_pos, sp.gettoken_stringref());
						stat = ES_ERROR;
					}
					else
						newstatement = MCN_new_statement(te->which);
				}
			else
			{
				MCeerror->add(EE_DO_NOCOMMAND, p_line, p_pos, sp.gettoken_stringref());
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
					MCeerror->add(EE_DO_BADCOMMAND, p_line, p_pos, p_script);
					stat = ES_ERROR;
				}
				count += curstatement->linecount();
			}
			break;
		case PS_EOL:
			if (sp.skip_eol() != PS_NORMAL)
			{
				MCeerror->add(EE_DO_BADLINE, p_line, p_pos, p_script);
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
		MCeerror -> add(EE_DO_NOTLICENSED, p_line, p_pos, p_script);
		stat = ES_ERROR;
	}

	if (stat == ES_ERROR)
	{
		deletestatements(statements);
		ctxt.Throw();
		return;
	}

	MCExecContext ctxt2(ctxt);
	while (statements != NULL)
	{
        statements->exec_ctxt(ctxt2);
		Exec_stat stat = ctxt2 . GetExecStat();
		if (stat == ES_ERROR)
		{
			deletestatements(statements);
			MCeerror->add(EE_DO_BADEXEC, p_line, p_pos, p_script);
			ctxt.Throw();
			return;
		}
		if (MCexitall || stat != ES_NORMAL)
		{
			deletestatements(statements);
			if (stat == ES_ERROR)
				ctxt.Throw();
			return;
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
		MCeerror->add(EE_DO_ABORT, p_line, p_pos);
		ctxt.Throw();
		return;
	}
	return;
}

////////////////////////////////////////////////////////////////////////////////

void MCHandler::compile(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxHandlerType t_type;
	switch(type)
	{
		case HT_MESSAGE:
			t_type = is_private ? kMCSyntaxHandlerTypePrivateCommand : kMCSyntaxHandlerTypeMessage;
			break;
		case HT_FUNCTION:
			t_type = is_private ? kMCSyntaxHandlerTypePrivateFunction : kMCSyntaxHandlerTypeFunction;
			break;
		case HT_GETPROP:
			t_type = kMCSyntaxHandlerTypeGetProp;
			break;
		case HT_SETPROP:
			t_type = kMCSyntaxHandlerTypeSetProp;
			break;
		case HT_BEFORE:
			t_type = kMCSyntaxHandlerTypeBeforeMessage;
			break;
		case HT_AFTER:
			t_type = kMCSyntaxHandlerTypeAfterMessage;
			break;
		default:
			MCAssert(false);
			break;
	}
	
	MCSyntaxFactoryBeginHandler(ctxt, t_type, name);
	
	for(uindex_t i = 0; i < npnames; i++)
		MCSyntaxFactoryDefineParameter(ctxt, pinfo[i] . name, pinfo[i] . is_reference);
	
	for(MCStatement *t_statement = statements; t_statement != nil; t_statement = t_statement -> getnext())
		t_statement -> compile(ctxt);
	
	MCSyntaxFactoryEndHandler(ctxt);
}

////////////////////////////////////////////////////////////////////////////////

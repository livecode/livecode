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

#include "core.h"
#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "scriptpt.h"
#include "execpt.h"
#include "hndlrlst.h"
#include "handler.h"
#include "keywords.h"
#include "literal.h"
#include "mcerror.h"
#include "object.h"
#include "util.h"
#include "debug.h"
#include "parentscript.h"

#include "globals.h"

MCVariable **MCHandlerlist::s_old_variables = NULL;
uint32_t MCHandlerlist::s_old_variable_count = 0;
uint32_t *MCHandlerlist::s_old_variable_map;

////

MCHandlerArray::MCHandlerArray(void)
{
	m_count = 0;
	m_handlers = NULL;
}

MCHandlerArray::~MCHandlerArray(void)
{
	clear();
}

// Destroy the list of handlers.
// First iterate through the list delete'ing each pointer.
// Then delete the array.
void MCHandlerArray::clear(void)
{
	for(uint32_t i = 0; i < m_count; ++i)
		delete m_handlers[i];
	free(m_handlers);

	m_handlers = NULL;
	m_count = 0;
}

void MCHandlerArray::append(MCHandler *p_handler)
{
	m_handlers = (MCHandler **)realloc(m_handlers, sizeof(MCHandler *) * (m_count + 1));
	m_handlers[m_count] = p_handler;
	m_count += 1;
}

void MCHandlerArray::sort(void)
{
	qsort(m_handlers, m_count, sizeof(MCHandler *), compare_handler);
}

MCHandler *MCHandlerArray::find(MCNameRef p_name)
{
	uint32_t t_low, t_high;
	t_low = 0;
	t_high = m_count;
	while(t_low < t_high)
	{
		uint32_t t_mid;
		t_mid = t_low + (t_high - t_low) / 2;

		compare_t d;
		d = MCCompare(MCNameGetCaselessSearchKey(p_name), MCNameGetCaselessSearchKey(m_handlers[t_mid] -> getname()));

		if (d < 0)
			t_high = t_mid;
		else if (d > 0)
			t_low = t_mid + 1;
		else
			return m_handlers[t_mid];
	}
	return NULL;
}

// This method searches the handler array to determine if a handler already
// exists. We use a linear search here since we can't guarantee the list is
// sorted (it is called during parsing).
bool MCHandlerArray::exists(MCNameRef p_name)
{
	for(uint4 i = 0; i < m_count; ++i)
		if (m_handlers[i] -> hasname(p_name))
			return true;

	return false;
}

int MCHandlerArray::compare_handler(const void *a, const void *b)
{
	MCHandler *ha, *hb;
	ha = *(MCHandler **)a;
	hb = *(MCHandler **)b;
    bool t_ha_is_engine_prop = ha->getprop() != P_UNDEFINED && ha->getprop() != P_CUSTOM;
    bool t_hb_is_engine_prop = hb->getprop() != P_UNDEFINED && hb->getprop() != P_CUSTOM;
    if (t_ha_is_engine_prop == t_hb_is_engine_prop)
       	return MCCompare(MCNameGetCaselessSearchKey(ha -> getname()), MCNameGetCaselessSearchKey(hb -> getname()));
    else
        return !t_ha_is_engine_prop && t_hb_is_engine_prop;
}

////

MCHandlerlist::MCHandlerlist()
{
	parent = NULL;
	vars = NULL;
	vinits = NULL;
	globals = NULL;
	cinfo = NULL;
	nglobals = 0;
	nconstants = 0;
	nvars = 0;
}

MCHandlerlist::~MCHandlerlist()
{
	reset();
}

void MCHandlerlist::reset(void)
{
	// MW-2012-09-05: [[ Bug ]] Make sure we reset the lists of before and after
	//   as well as the others.
	for(uint32_t i = 0; i < 6; ++i)
		handlers[i] . clear();

	MCVariable *vtmp;
	while (vars != NULL)
	{
		vtmp = vars;
		vars = vars->getnext();
		delete vtmp;
	}

	for(uint32_t i = 0; i < nvars; i++)
		MCNameDelete(vinits[i]);
	delete vinits;
	vinits = NULL;
	nvars = 0;

	delete globals;
	globals = NULL;
	nglobals = 0;

	for(uint32_t i = 0; i < nconstants; i++)
	{
		MCNameDelete(cinfo[i] . name);
		MCNameDelete(cinfo[i] . value);
	}
	delete cinfo;
	cinfo = NULL;
	nconstants = 0;
}

MCObject *MCHandlerlist::getparent()
{
	return parent;
}

// MW-2011-08-23: [[ UQL ]] If 'ignore_url' is true, then any vars that have the
//   UQL attribute will be ignore for the search.
Parse_stat MCHandlerlist::findvar(MCNameRef p_name, bool p_ignore_uql, MCVarref **dptr)
{
	MCVariable *tmp;

	uint32_t t_vindex;
	for (tmp = vars, t_vindex = 0 ; tmp != NULL ; tmp = tmp->getnext(), t_vindex += 1)
		if ((!tmp -> isuql() || !p_ignore_uql) && tmp->hasname(p_name))
		{
			*dptr = new MCVarref(tmp, t_vindex);
			return PS_NORMAL;
		}

	uint2 i;
	for (i = 0 ; i < nglobals ; i++)
	{
		if (globals[i]->hasname(p_name))
		{
			*dptr = globals[i] -> newvarref();
			return PS_NORMAL;
		}
	}

	if (MCNameIsEqualTo(p_name, MCN_msg, kMCCompareCaseless))
	{
		*dptr = new MCVarref(MCmb);
		return PS_NORMAL;
	}

	if (MCNameIsEqualTo(p_name, MCN_each, kMCCompareCaseless))
	{
		*dptr = new MCVarref(MCeach);
		return PS_NORMAL;
	}

	// In server mode, we need to resolve $ vars in the context of the global
	// scope. (This doesn't happen in non-server mode as there is never any
	// 'code' in 'global' scope).
	if (MCNameGetCharAtIndex(p_name, 0) == '$')
	{
		MCVariable *tmp;
		for (tmp = MCglobals ; tmp != NULL ; tmp = tmp->getnext())
			if (tmp->hasname(p_name))
			{
				*dptr = tmp -> newvarref();
				return PS_NORMAL;
			}
		MCVariable *t_global;
		/* UNCHECKED */ MCVariable::ensureglobal(p_name, t_global);
		*dptr = t_global -> newvarref();
		return PS_NORMAL;
	}

	if (MCdebugcontext != MAXUINT2)
	{
		MCExecPoint *epptr = MCexecutioncontexts[MCdebugcontext];
		if (epptr->gethlist() != this)
			if (epptr->gethandler()->findvar(p_name, dptr) != PS_NORMAL)
				return epptr->gethlist()->findvar(p_name, false, dptr);
			else
				return PS_NORMAL;
	}

	return PS_NO_MATCH;
}

Parse_stat MCHandlerlist::newvar(MCNameRef p_name, MCNameRef p_init, MCVarref **newptr, Boolean initialised)
{
	MCVariable *t_new_variable;

	if (!initialised && s_old_variables != NULL)
	{
		uint32_t t_index;
		for(t_index = 0, t_new_variable = NULL; t_index < s_old_variable_count; t_index += 1)
			if (s_old_variables[t_index] != NULL && s_old_variables[t_index] -> hasname(p_name))
			{
				t_new_variable = s_old_variables[t_index];
				break;
			}

		if (t_new_variable != NULL)
		{
			t_new_variable -> setnext(NULL);
			s_old_variables[t_index] = NULL;

			// MW-2008-10-28: [[ ParentScripts ]] If there is a variable map to fill in, do so.
			if (s_old_variable_map != NULL)
				s_old_variable_map[t_index] = nvars;
		}
	}
	else
		t_new_variable = NULL;

	if (t_new_variable == NULL)
		/* UNCHECKED */ MCVariable::createwithname(p_name, t_new_variable);

	MCVariable *lastvar = vars;
	if (vars == NULL)
		vars = lastvar = t_new_variable;
	else
	{
		while (lastvar->getnext() != NULL)
			lastvar = lastvar->getnext();
		lastvar->setnext(t_new_variable);
		lastvar = lastvar->getnext();
	}

	if (initialised)
	{
		if (p_init != nil)
			lastvar->setnameref_unsafe(p_init);
		else
		{
			// MW-2011-08-22: [[ Bug ]] We are initializing a UQL in server script
			//   scope - so set it as a UQL.
			lastvar->setnameref_unsafe(p_name);
			lastvar->setuql();
		}
	}

	// MW-2008-10-28: [[ ParentScripts ]] Make a varref for a script local variable.
	*newptr = new MCVarref(lastvar, nvars);

	// MW-2008-10-28: [[ ParentScripts ]] Extend the vinits array
	// MW-2011-08-22: [[ Bug ]] Don't clone the init when we are creating a UQL in
	//   global script scope (never used as parentscript so irrelevant).
	MCU_realloc((char **)&vinits, nvars, nvars + 1, sizeof(MCNameRef));
	if (p_init != nil)
		/* UNCHECKED */ MCNameClone(p_init, vinits[nvars]);
	else
		vinits[nvars] = p_init;

	// MW-2008-10-28: [[ ParentScripts ]] Increment the number of vars
	nvars += 1;

	return PS_NORMAL;
}

Parse_stat MCHandlerlist::findconstant(MCNameRef p_name, MCExpression **dptr)
{
	uint2 i;
	for (i = 0 ; i < nconstants ; i++)
		if (MCNameIsEqualTo(p_name, cinfo[i].name, kMCCompareCaseless))
		{
			*dptr = new MCLiteral(cinfo[i].value);
			return PS_NORMAL;
		}
	return PS_NO_MATCH;
}

Parse_stat MCHandlerlist::newconstant(MCNameRef p_name, MCNameRef p_value)
{
	MCU_realloc((char **)&cinfo, nconstants, nconstants + 1, sizeof(MCHandlerConstantInfo));
	/* UNCHECKED */ MCNameClone(p_name, cinfo[nconstants].name);
	/* UNCHECKED */ MCNameClone(p_value, cinfo[nconstants++].value);
	return PS_NORMAL;
}

void MCHandlerlist::appendlocalnames(MCExecPoint &ep)
{
	MCVariable *tmp;
	for (tmp = vars ; tmp != NULL ; tmp = tmp->getnext())
		ep.concatnameref(tmp->getname(), EC_COMMA, tmp == vars);
}

void MCHandlerlist::appendglobalnames(MCExecPoint &ep, bool first)
{
	uint2 i;
	for (i = 0 ; i < nglobals ; i++)
	{
		ep.concatnameref(globals[i]->getname(), EC_COMMA, first);
		first = false;
	}
}

// OK-2008-06-25: <Bug where the variableNames property would return duplicate global names>
uint2 MCHandlerlist::getnglobals(void)
{
	return nglobals;
}

MCVariable *MCHandlerlist::getglobal(uint2 p_index)
{
	return globals[p_index];
}



void MCHandlerlist::newglobal(MCNameRef p_name)
{
	// Check to see if the global is already listed
	for(unsigned int i = 0; i < nglobals; ++i)
		if (globals[i] -> hasname(p_name))
			return;
	
	// Ensure a global exists with the given name
	MCVariable *gptr;
	/* UNCHECKED */ MCVariable::ensureglobal(p_name, gptr);
	
	// Add the global to the list
	MCU_realloc((char **)&globals, nglobals, nglobals + 1, sizeof(MCVariable *));
	globals[nglobals++] = gptr;
}

Parse_stat MCHandlerlist::parse(MCObject *objptr, const char *script)
{
	Parse_stat status = PS_NORMAL;

	// MW-2008-01-30: [[ Bug 5781 ]] Make sure we clear the parse error stack before
	//   parsing. Otherwise we get bogus errors...
	if (!MCperror -> isempty())
		MCperror -> clear();

	MCScriptPoint sp(objptr, this, script);

	// MW-2008-11-02: Its possible for the objptr to be NULL if this is inert execution
	//   (for example 'getdefaultprinter()' on Linux) so don't indirect in this case.
	bool t_is_parent_script;
	if (objptr != NULL)
		t_is_parent_script = objptr -> getstate(CS_IS_PARENTSCRIPT) && objptr -> gettype() == CT_BUTTON;
	else
		t_is_parent_script = false;

	// MW-2009-11-24: [[ Bug 8448 ]] Make sure we don't do any variable preservation stuff
	//   if there are no variables to preserve.
	if (MCpreservevariables && nvars != 0)
	{
		// MW-2008-11-02: [[ ParentScripts ]] Rejig the old variables list to be a vector
		//   so we can preserve the ordering for later mapping.
		s_old_variables = new MCVariable *[nvars];
		s_old_variable_count = nvars;
		for(uint32_t i = 0; vars != NULL; vars = vars -> getnext(), i++)
			s_old_variables[i] = vars;
		
		// MW-2008-10-28: [[ ParentScripts ]] Allocate an array for the var remapping
		//   but only if this object is used as a parentscript.
		if (t_is_parent_script)
		{
			s_old_variable_map = new uint32_t[nvars];

			// We initialize the map to all 0xffffffff's - this indicates that the existing
			// value should not be brought forward.
			memset(s_old_variable_map, 255, sizeof(uint32_t) * nvars);
		}
	}
	else
		s_old_variables = NULL;

	reset();

	Bool finished = False;
	parent = objptr;
	while (status != PS_ERROR && !finished)
	{
		Symbol_type type;
		const LT *te;

		switch (sp.next(type))
		{
		case PS_NORMAL:
			if (sp.lookup(SP_HANDLER, te) != PS_NORMAL)
				sp.skip_eol();
			else
			{
				MCHandler *newhandler;
				switch (te->type)
				{
				case TT_HANDLER:
				{
					bool t_is_private;
					t_is_private = false;
					if (te -> which == HT_PRIVATE)
					{
						sp.next(type);
						if (sp.lookup(SP_HANDLER, te) != PS_NORMAL ||
								te -> which == HT_GETPROP || te -> which == HT_SETPROP ||
								te -> which == HT_BEFORE || te -> which == HT_AFTER)
						{
							MCperror -> add(PE_HANDLERLIST_BADHANDLER, sp);
							status = PS_ERROR;
							break;
						}

						t_is_private = true;
					}
					newhandler = new MCHandler((uint1)te->which, t_is_private);
					if (newhandler->parse(sp, te->which == HT_GETPROP || te->which == HT_SETPROP) != PS_NORMAL)
					{
						sp.sethandler(NULL);
						delete newhandler;
						MCperror->add(PE_HANDLERLIST_BADHANDLER, sp);
						status = PS_ERROR;
						break;
					}

					sp.sethandler(NULL);

					// MW-2008-07-21: [[ Bug 6779 ]] If a handler of the given type already exists
					//   with the same name then don't include it in the list. At some point we
					//   probably want this to cause a warning.
					if (!handlers[te -> which - 1] . exists(newhandler -> getname()))
						handlers[te -> which - 1] . append(newhandler);
					else
						delete newhandler;
				}
				break;
				case TT_VARIABLE:
					sp.sethandler(NULL);
					switch (te->which)
					{
					case S_GLOBAL:
						{
							MCGlobal *gptr = new MCGlobal;
							if (gptr->parse(sp) != PS_NORMAL)
							{
								MCperror->add(PE_HANDLER_BADVAR, sp);
								status = PS_ERROR;
							}
							delete gptr;
							break;
						}
					case S_LOCAL:
						{
							MCLocalVariable *lptr = new MCLocalVariable;
							if (lptr->parse(sp) != PS_NORMAL)
							{
								MCperror->add(PE_HANDLER_BADVAR, sp);
								status = PS_ERROR;
							}
							delete lptr;
							break;
						}
					case S_CONSTANT:
						{
							MCLocalConstant *cptr = new MCLocalConstant;
							if (cptr->parse(sp) != PS_NORMAL)
							{
								MCperror->add(PE_HANDLER_BADVAR, sp);
								status = PS_ERROR;
							}
							delete cptr;
							break;
						}
					default:
						break;
					}
					break;
				default:
					break;
				}
			}
			break;
		case PS_EOF:
			finished = True;
			break;
		case PS_EOL:
			if (sp.skip_eol() != PS_NORMAL)
			{
				MCperror->add(PE_HANDLERLIST_BADEOL, sp);
				status = PS_ERROR;
			}
			break;
		default:
			MCperror->add(PE_HANDLERLIST_BADCHAR, sp);
			status = PS_ERROR;
			break;
		}
	}

	if (s_old_variables != NULL)
	{
		for(uint32_t i = 0; i < s_old_variable_count; ++i)
			if (s_old_variables[i] != NULL)
				delete s_old_variables[i];
	}

	delete s_old_variables;
	s_old_variables = NULL;

	if (status != PS_ERROR)
	{
		// MW-2012-10-17: [[ Bug 10475 ]] Make sure we sort all the handler arrays!
		for(uint32_t i = 0; i < 6; i++)
			handlers[i] . sort();
	}

	if (t_is_parent_script)
	{
		if (s_old_variable_map != NULL && status != PS_ERROR)
			MCParentScript::PreserveVars(objptr, s_old_variable_map, vinits, nvars);
		else
			MCParentScript::ClearVars(objptr);

		delete s_old_variable_map;
		s_old_variable_map = NULL;
	}

	return status;
}

Exec_stat MCHandlerlist::findhandler(Handler_type type, MCNameRef name, MCHandler *&handret)
{
	assert(type > 0 && type <= 6);

	handret = handlers[type - 1] . find(name);
	if (handret != NULL)
		return ES_NORMAL;

	return ES_NOT_FOUND;
}

uint4 MCHandlerlist::linecount()
{
	uint4 count = 0;

	for(uint32_t i = 0; i < 6; ++i)
		for(uint32_t j = 0; j < handlers[i] . count(); ++j)
			count += handlers[i] . get()[j] -> linecount();

	return count;
}

bool MCHandlerlist::hashandler(Handler_type type, MCNameRef name)
{
	return handlers[type - 1] . exists(name);
}

void MCHandlerlist::addhandler(Handler_type type, MCHandler *handler)
{
	handlers[type - 1] . append(handler);
	handlers[type - 1] . sort();
}

static bool enumerate_handlers(MCExecPoint& ep, const char *p_type, MCHandlerArray& p_handlers, bool p_first = false, MCObject *p_object = NULL)
{
	for(uint32_t j = 0; j < p_handlers . count(); ++j)
	{
		MCHandler *t_handler;
		t_handler = p_handlers . get()[j];

		if (!p_first)
			ep.appendchar('\n');

		ep . appendstringf("%s%s %s %d %d", t_handler -> isprivate() ? "P" : "", p_type, t_handler -> getname_cstring(), t_handler -> getstartline(), t_handler -> getendline());

		// OK-2008-07-23 : Add the object long id to the first handler from each object. This will
		// allow the script editor to look up handlers faster.
		if (p_first && p_object != NULL)
		{
			MCExecPoint t_ep;
			p_object -> getprop(0, P_LONG_ID, t_ep, False);
			ep . concatmcstring(t_ep . getsvalue(), EC_SPACE, false);
		}	

		p_first = false;
	}
	
	return p_first;
}

bool MCHandlerlist::enumerate(MCExecPoint& ep, bool p_first)
{
	// OK-2008-07-23 : Added parent object reference for script editor.
	MCObject *t_object;
	t_object = getparent();

	p_first = enumerate_handlers(ep, "M", handlers[0], p_first, t_object);
	p_first = enumerate_handlers(ep, "F", handlers[1], p_first, t_object);
	p_first = enumerate_handlers(ep, "G", handlers[2], p_first, t_object);
	p_first = enumerate_handlers(ep, "S", handlers[3], p_first, t_object);
	
	// MW-2012-09-07: [[ BeforeAfter ]] Make sure before/after appear in the handlerlist.
	p_first = enumerate_handlers(ep, "B", handlers[4], p_first, t_object);
	p_first = enumerate_handlers(ep, "A", handlers[5], p_first, t_object);
	
	return p_first;
}

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

#include "scriptpt.h"

#include "hndlrlst.h"
#include "handler.h"
#include "keywords.h"
#include "literal.h"
#include "mcerror.h"
#include "object.h"
#include "util.h"
#include "debug.h"
#include "parentscript.h"
#include "variable.h"

#include "globals.h"

////////////////////////////////////////////////////////////////////////////////

MCVariable **MCHandlerlist::s_old_variables = NULL;
uint32_t MCHandlerlist::s_old_variable_count = 0;
uint32_t *MCHandlerlist::s_old_variable_map;

////////////////////////////////////////////////////////////////////////////////

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

	return MCCompare(MCNameGetCaselessSearchKey(ha -> getname()), MCNameGetCaselessSearchKey(hb -> getname()));
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
		MCValueRelease(vinits[i]);
	delete[] vinits; /* Allocated with new[] */
	vinits = NULL;
	nvars = 0;

	delete[] globals; /* Allocated with new[] */
	globals = NULL;
	nglobals = 0;

	for(uint32_t i = 0; i < nconstants; i++)
	{
		MCValueRelease(cinfo[i] . name);
		MCValueRelease(cinfo[i] . value);
	}
	delete[] cinfo; /* Allocated with new[] */
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
			*dptr = new (nothrow) MCVarref(tmp, t_vindex);
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

	if (MCNameIsEqualToCaseless(p_name, MCN_msg))
	{
		*dptr = new (nothrow) MCVarref(MCmb);
		return PS_NORMAL;
	}

	if (MCNameIsEqualToCaseless(p_name, MCN_each))
	{
		*dptr = new (nothrow) MCVarref(MCeach);
		return PS_NORMAL;
	}

	// In server mode, we need to resolve $ vars in the context of the global
	// scope. (This doesn't happen in non-server mode as there is never any
	// 'code' in 'global' scope).
	if (MCStringGetNativeCharAtIndex(MCNameGetString(p_name), 0) == '$')
	{
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
		MCExecContext *ctxtptr = MCexecutioncontexts[MCdebugcontext];
		if (ctxtptr->GetHandlerList() != this)
		{
			if (ctxtptr->GetHandler()->findvar(p_name, dptr) != PS_NORMAL)
				return ctxtptr->GetHandlerList()->findvar(p_name, false, dptr);
			else
				return PS_NORMAL;
		}
	}

	return PS_NO_MATCH;
}

Parse_stat MCHandlerlist::newvar(MCNameRef p_name, MCValueRef p_init, MCVarref **newptr, Boolean initialised)
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
			lastvar->setvalueref(p_init);
		else
		{
			// MW-2011-08-22: [[ Bug ]] We are initializing a UQL in server script
			//   scope - so set it as a UQL.
			lastvar->setvalueref(p_name);
			lastvar->setuql();
		}
	}

	// MW-2008-10-28: [[ ParentScripts ]] Make a varref for a script local variable.
	*newptr = new (nothrow) MCVarref(lastvar, nvars);

	// MW-2008-10-28: [[ ParentScripts ]] Extend the vinits array
	// MW-2011-08-22: [[ Bug ]] Don't clone the init when we are creating a UQL in
	//   global script scope (never used as parentscript so irrelevant).
	MCU_realloc((char **)&vinits, nvars, nvars + 1, sizeof(MCValueRef));
	if (p_init != nil)
		/* UNCHECKED */ vinits[nvars] = MCValueRetain(p_init);
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
		if (MCNameIsEqualToCaseless(p_name, cinfo[i].name))
		{
			*dptr = new (nothrow) MCLiteral(cinfo[i].value);
			return PS_NORMAL;
		}
	return PS_NO_MATCH;
}

Parse_stat MCHandlerlist::newconstant(MCNameRef p_name, MCValueRef p_value)
{
	MCU_realloc((char **)&cinfo, nconstants, nconstants + 1, sizeof(MCHandlerConstantInfo));
    cinfo[nconstants].name = MCValueRetain(p_name);
	cinfo[nconstants++].value = MCValueRetain(p_value);
	return PS_NORMAL;
}

bool MCHandlerlist::getlocalnames(MCListRef& r_list)
{
	MCAutoListRef t_list;
	if (!MCListCreateMutable(',', &t_list))
		return false;
	for (MCVariable *t_var = vars; t_var != nil; t_var = t_var->getnext())
		if (!MCListAppend(*t_list, t_var->getname()))
			return false;

	return MCListCopy(*t_list, r_list);
}

bool MCHandlerlist::getglobalnames(MCListRef& r_list)
{
	MCAutoListRef t_list;
	if (!MCListCreateMutable(',', &t_list))
		return false;
	for (uinteger_t i = 0 ; i < nglobals ; i++)
		if (!MCListAppend(*t_list, globals[i]->getname()))
			return false;

	return MCListCopy(*t_list, r_list);
}

bool MCHandlerlist::getconstantnames(MCListRef& r_list)
{
    MCAutoListRef t_list;
    if (!MCListCreateMutable(',', &t_list))
        return false;
    for (uinteger_t i = 0 ; i < nconstants ; i++)
        if (!MCListAppend(*t_list, cinfo[i].name))
            return false;
    
    return MCListCopy(*t_list, r_list);
}

void MCHandlerlist::appendglobalnames(MCStringRef& r_string, bool first)
{
	MCAutoListRef t_list;
	/* UNCHECKED */ getglobalnames(&t_list);
	/* UNCHECKED */ MCListCopyAsString(*t_list, r_string);

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

Parse_stat MCHandlerlist::parse(MCObject *objptr, MCDataRef script_utf8)
{
	Parse_stat status = PS_NORMAL;

	// MW-2008-01-30: [[ Bug 5781 ]] Make sure we clear the parse error stack before
	//   parsing. Otherwise we get bogus errors...
	if (!MCperror -> isempty())
		MCperror -> clear();

	MCScriptPoint sp(objptr, this, script_utf8);

	// MW-2008-11-02: Its possible for the objptr to be NULL if this is inert execution
	//   (for example 'getdefaultprinter()' on Linux) so don't indirect in this case.
	bool t_is_parent_script;
	if (objptr != NULL)
		t_is_parent_script = objptr -> getisparentscript();
	else
		t_is_parent_script = false;

	// MW-2009-11-24: [[ Bug 8448 ]] Make sure we don't do any variable preservation stuff
	//   if there are no variables to preserve.
	if (MCpreservevariables && nvars != 0)
	{
		// MW-2008-11-02: [[ ParentScripts ]] Rejig the old variables list to be a vector
		//   so we can preserve the ordering for later mapping.
		s_old_variables = new (nothrow) MCVariable *[nvars];
		s_old_variable_count = nvars;
		for(uint32_t i = 0; vars != NULL; vars = vars -> getnext(), i++)
			s_old_variables[i] = vars;
		
		// MW-2008-10-28: [[ ParentScripts ]] Allocate an array for the var remapping
		//   but only if this object is used as a parentscript.
		if (t_is_parent_script)
		{
			s_old_variable_map = new (nothrow) uint32_t[nvars];

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
					newhandler = new (nothrow) MCHandler((uint1)te->which, t_is_private);
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
							MCGlobal *gptr = new (nothrow) MCGlobal;
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
							MCLocalVariable *lptr = new (nothrow) MCLocalVariable;
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
							MCLocalConstant *cptr = new (nothrow) MCLocalConstant;
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

Parse_stat MCHandlerlist::parse(MCObject *objptr, MCStringRef p_script)
{
    MCAutoDataRef t_utf16_script;
    unichar_t *t_unicode_string;
    uint32_t t_length;
    /* UNCHECKED */ MCStringConvertToUnicode(p_script, t_unicode_string, t_length);
    /* UNCHECKED */ MCDataCreateWithBytesAndRelease((byte_t *)t_unicode_string, (t_length + 1) * 2, &t_utf16_script);
    return parse(objptr, *t_utf16_script);
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

static const char *s_handler_types[] =
{
    "M",
    "F",
    "G",
    "S",
	// MW-2012-09-07: [[ BeforeAfter ]] Make sure before/after appear in the handlerlist.
    "B",
    "A",
};

static bool enumerate_handlers(MCExecContext& ctxt, bool p_include_private, const char *p_type, MCHandlerArray& p_handlers, uindex_t& r_count, MCStringRef*& r_handlers, bool p_first = false, MCObject *p_object = nil)
{
    MCAutoArray<MCStringRef> t_handlers;
    MCAutoStringRef t_long_id;
	for(uint32_t j = 0; j < p_handlers . count(); ++j)
	{
		MCHandler *t_handler;
		t_handler = p_handlers . get()[j];
        
        if (t_handler->isprivate() && !p_include_private)
        {
            continue;
        }
        
		MCStringRef t_string;
        const char *t_format;
        
        // OK-2008-07-23 : Add the object long id to the first handler from each object. This will
		// allow the script editor to look up handlers faster.
		if (p_first && p_object != nil)
        {
            t_format = "%s%s %@ %d %d %@";
            p_object -> GetLongId(ctxt, 0, &t_long_id);
        }
        else
            t_format = "%s%s %@ %d %d";
        
        /* UNCHECKED */ MCStringFormat(t_string,
                                       t_format,
                                       t_handler->isprivate() ? "P" : "",
                                       p_type,
                                       t_handler->getname(),
                                       t_handler->getstartline(),
                                       t_handler->getendline(),
                                       *t_long_id);
		
		t_handlers . Push(t_string);
		p_first = false;
	}
	
    t_handlers . Take(r_handlers, r_count);
	return p_first;
}

bool MCHandlerlist::enumerate(MCExecContext& ctxt, bool p_include_private, bool p_first, uindex_t& r_count, MCStringRef*& r_handlers)
{
	// OK-2008-07-23 : Added parent object reference for script editor.
	MCObject *t_object;
	t_object = getparent();
    
    MCAutoArray<MCStringRef> t_handlers;
    
    for (uindex_t i = 0; i < 6; i++)
    {
        MCStringRef *t_handler_array;
        t_handler_array = nil;
        uindex_t t_count;
        
        p_first = enumerate_handlers(ctxt, p_include_private, s_handler_types[i], handlers[i], t_count, t_handler_array, p_first, t_object);
        for (uindex_t j = 0; j < t_count; j++)
            t_handlers . Push(t_handler_array[j]);
        
        MCMemoryDeleteArray(t_handler_array);
    }
    
    t_handlers . Take(r_handlers, r_count);
	return p_first;
}

bool MCHandlerlist::listconstants(MCHandlerlistListConstantsCallback p_callback, void *p_context)
{
	for(uint2 i = 0; i < nconstants; i++)
	{
		if (!p_callback(p_context,
						&cinfo[i]))
		{
			return false;
		}
	}
	
	return true;
}

bool MCHandlerlist::listvariables(MCHandlerlistListVariablesCallback p_callback, void *p_context)
{
    
    for (MCVariable *t_var = vars; t_var != nil; t_var = t_var->getnext())
    {
        if (!p_callback(p_context,
                        t_var))
        {
            return false;
        }
    }
    
    return true;
}

bool MCHandlerlist::listglobals(MCHandlerlistListVariablesCallback p_callback, void *p_context)
{
    
    for (uinteger_t i = 0 ; i < nglobals ; i++)
    {
        if (!p_callback(p_context,
                        globals[i]))
        {
            return false;
        }
    }
    
    return true;
}


bool MCHandlerlist::listhandlers(MCHandlerlistListHandlersCallback p_callback, void *p_context, bool p_include_all)
{
	for(int t_htype = HT_MIN; t_htype < HT_MAX; t_htype++)
	{
        int t_htype_index = static_cast<int>(t_htype - HT_MIN);
        
		for(uint2 i = 0; i < handlers[t_htype_index].count(); i++)
		{
			if (!p_callback(p_context,
							static_cast<Handler_type>(t_htype),
							handlers[t_htype_index].get()[i],
                            p_include_all))
			{
				return false;
			}
		}
	}
	
	return true;
}

////////////////////////////////////////////////////////////////////////////////

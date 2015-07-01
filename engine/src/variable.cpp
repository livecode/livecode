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
#include "parsedef.h"
#include "filedefs.h"
#include "objdefs.h"

#include "stack.h"
#include "card.h"
#include "field.h"
#include "handler.h"
#include "hndlrlst.h"
#include "execpt.h"
#include "scriptpt.h"
#include "mcerror.h"
#include "util.h"
#include "debug.h"
#include "globals.h"
#include "objectstream.h"
#include "parentscript.h"
#include "osspec.h"

////////////////////////////////////////////////////////////////////////////////

bool MCVariable::create(MCVariable*& r_var)
{
	MCVariable *self;
	self = new MCVariable;
	if (self == nil)
		return false;

	self -> next = nil;
	self -> name = nil;

	self -> is_msg = false;
	self -> is_env = false;
	self -> is_global = false;
	self -> is_deferred = false;
	self -> is_uql = false;

	r_var = self;
	
	return true;
}

bool MCVariable::createwithname(MCNameRef p_name, MCVariable*& r_var)
{
	MCVariable *self;
	if (!create(self))
		return false;

	if (!MCNameClone(p_name, self -> name))
	{
		delete self;	
		return false;
}

	r_var = self;

	return true;
}

bool MCVariable::createwithname_cstring(const char *p_name, MCVariable*& r_var)
{
	MCVariable *self;
	if (!create(self))
		return false;

	if (!MCNameCreateWithCString(p_name, self -> name))
	{
		delete self;
		return false;
	}

	r_var = self;

	return true;
}

// This is only called by MCObject to create copies of prop sets.
bool MCVariable::createcopy(MCVariable& p_var, MCVariable*& r_new_var)
{
	bool t_success;
	t_success = true;

	MCVariable *self;
	self = nil;
	if (t_success)
		t_success = create(self);

	if (t_success)
		t_success = MCNameClone(p_var . name, self -> name);

	if (t_success)
		t_success = self -> value . assign(p_var . value);

	if (t_success)
		r_new_var = self;
	else
		delete self;

	return t_success;
}

MCVariable::~MCVariable(void)
{
	MCNameDelete(name);
}

////////////////////////////////////////////////////////////////////////////////

void MCVariable::doclearuql(void)
{
	if (value . is_string() && value . get_string() . getstring() == MCNameGetOldString(name) . getstring())
		clear(True);
	is_uql = false;
}

////////////////////////////////////////////////////////////////////////////////

MCVariable *MCVariable::lookupglobal_cstring(const char *p_name)
{
	// If we can't find an existing name, then there can be no global with
	// name 'p_name'.
	MCNameRef t_name;
	t_name = MCNameLookupWithCString(p_name, kMCCompareCaseless);
	if (t_name == nil)
		return nil;

	// The name is in use, so check to see if there is a global using it.
	return lookupglobal(t_name);
}

MCVariable *MCVariable::lookupglobal(MCNameRef p_name)
{
	// See if the global already exists.
	for(MCVariable *t_var = MCglobals; t_var != nil; t_var = t_var -> next)
		if (t_var -> hasname(p_name))
			return t_var;

	return nil;
}

bool MCVariable::ensureglobal_cstring(const char *p_name, MCVariable*& r_var)
{
	bool t_success;
	t_success = true;

	MCNameRef t_name;
	t_name = nil;
	if (t_success)
		t_success = MCNameCreateWithCString(p_name, t_name);

	if (t_success)
		t_success = ensureglobal(t_name, r_var);

	MCNameDelete(t_name);
	
	return t_success;
}

bool MCVariable::ensureglobal(MCNameRef p_name, MCVariable*& r_var)
{
	// First check to see if the global variable already exists
	MCVariable *t_var;
	t_var = lookupglobal(p_name);
	if (t_var != nil)
	{
		r_var = t_var;
		return true;
	}

	// Didn't find one, so create a shiny new one!
	MCVariable *t_new_global;
	if (!createwithname(p_name, t_new_global))
		return false;

	if (MCNameGetCharAtIndex(p_name, 0) == '$')
		t_new_global -> is_env = true;

	t_new_global -> is_global = true;

	t_new_global -> next = MCglobals;
	MCglobals = t_new_global;

	r_var = t_new_global;

	return true;
}

////////////////////////////////////////////////////////////////////////////////

void MCVariable::synchronize(MCExecPoint& ep, Boolean notify)
{
	if (is_env)
	{
		if (!isdigit(MCNameGetCharAtIndex(name, 1)) && MCNameGetCharAtIndex(name, 1) != '#')
		{
			if (value . ensure_string(ep))
			{
				char *v = value . get_string() . clone();
				MCS_setenv(MCNameGetCString(name) + 1, v);
				delete v;
			}
			else
				MCS_setenv(MCNameGetCString(name) + 1, "");
		}
	}
	else if (is_msg)
	{
		fetch(ep);
		MCB_setmsg(ep);
	}

	if (notify && MCnwatchedvars)
	{
		uint2 i;
		for (i = 0 ; i < MCnwatchedvars ; i++)
		{
			if ((MCwatchedvars[i].object == NULL || MCwatchedvars[i].object == ep.getobj()) &&
				(MCwatchedvars[i].handlername == NULL || ep.gethandler()->hasname(MCwatchedvars[i].handlername)) &&
				hasname(MCwatchedvars[i].varname))
			{
				// If this is a global watch (object == handlername == nil) then
				// check that this var is a global - if not carry on the search.
				if (MCwatchedvars[i] . object == NULL &&
					MCwatchedvars[i] . handlername == NULL &&
					!is_global)
					continue;

				// Otherwise, trigger the setvar message.
				fetch(ep);
				if (*MCwatchedvars[i].expression)
				{
					MCExecPoint ep2(ep);
					ep2.setsvalue(MCwatchedvars[i].expression);
					Boolean d;
					if (ep . eval(ep2) == ES_NORMAL && MCU_stob(ep2.getsvalue(), d) && d)
						MCB_setvar(ep, name);
				}
				else
					MCB_setvar(ep, name);

				break;
			}
		}
	}
}

Exec_stat MCVariable::remove(MCExecPoint& ep, Boolean notify)
{
	value . clear();
	
	if (is_env)
	{
		if (!isdigit(MCNameGetCharAtIndex(name, 1)) && MCNameGetCharAtIndex(name, 1) != '#')
			MCS_unsetenv(MCNameGetCString(name) + 1);
	}

	return ES_NORMAL;
}

MCVarref *MCVariable::newvarref(void)
{
	if (!is_deferred)
		return new MCVarref(this);

	return new MCDeferredVarref(this);
}

////////////////////////////////////////////////////////////////////////////////

MCVarref::~MCVarref()
{
	if (dimensions <= 1)
		delete exp;
	else
	{
		for(uint4 i = 0; i < dimensions; ++i)
			delete exps[i];
		delete exps;
	}
}

MCVariable *MCVarref::fetchvar(MCExecPoint& ep)
{
	// MW-2009-01-28: [[ Inherited parentScripts ]]
	// If we are in parentScript context, then fetch the script local from there,
	// otherwise use the information stored in this.
	MCParentScriptUse *t_parentscript;
	t_parentscript = ep . getparentscript();
	if (!isscriptlocal || t_parentscript == NULL)
	{
		if (ref != NULL)
			return ref;

		return handler -> getvar(index, isparam);
	}
	
	return t_parentscript -> GetVariable(index);
}

MCVariable *MCVarref::evalvar(MCExecPoint& ep)
{
	if (dimensions != 0)
		return NULL;

	return fetchvar(ep);
}

Exec_stat MCVarref::eval(MCExecPoint& ep)
{
	if (dimensions == 0)
	{
		MCVariable *t_resolved_ref;
		
		t_resolved_ref = fetchvar(ep);

		return t_resolved_ref -> fetch(ep);
	}

	MCVariable *t_var;
	MCVariableValue *t_parent;
	MCHashentry *t_hash;

	if (resolve(ep, t_var, t_parent, t_hash, false) != ES_NORMAL)
		return ES_ERROR;

	if (t_hash != NULL)
		return t_hash -> value . fetch(ep);

	ep . clear();
	
	return ES_NORMAL;
}

Exec_stat MCVarref::evalcontainer(MCExecPoint& ep, MCVariable*& r_var, MCVariableValue*& r_ref)
{
	if (dimensions == 0)
	{
		r_var = fetchvar(ep);
		
		r_ref = &r_var -> getvalue();
		
		return ES_NORMAL;
	}

	MCVariableValue *t_parent;
	MCHashentry *t_hash;
	
	if (resolve(ep, r_var, t_parent, t_hash, true) != ES_NORMAL)
		return ES_ERROR;
		
	r_ref = &t_hash -> value;
	
	return ES_NORMAL;
}

MCVarref *MCVarref::getrootvarref(void)
{
	return this;
}


bool MCVarref::rootmatches(MCVarref *p_other) const
{
	if (this == p_other)
		return true;

	if (p_other -> ref != NULL)
		return ref == p_other -> ref;

	return handler == p_other -> handler && index == p_other -> index && isparam == p_other -> isparam;
}

Exec_stat MCVarref::set(MCExecPoint &ep, Boolean append)
{
	if (dimensions == 0)
	{
		MCVariable *t_resolved_ref;

		t_resolved_ref = fetchvar(ep);

		if (!append)
			return t_resolved_ref -> store(ep, True);

		return t_resolved_ref -> append(ep, True);
	}

	MCVariable *t_var;
	MCVariableValue *t_parent;
	MCHashentry *t_hash;
	if (resolve(ep, t_var, t_parent, t_hash, true) != ES_NORMAL)
		return ES_ERROR;
	
	if (!append)
	{
		if (t_hash -> value . store(ep) == ES_NORMAL)
		{
			t_var -> synchronize(ep, True);
			return ES_NORMAL;
		}
	}
	else
	{
		if (t_hash -> value . append(ep) == ES_NORMAL)
		{
			t_var -> synchronize(ep, True);
			return ES_NORMAL;
		}
	}
	
	return ES_ERROR;
}

Parse_stat MCVarref::parsearray(MCScriptPoint &sp)
{
	Symbol_type type;

	for(;;)
	{
		if (sp.next(type) != PS_NORMAL)
			return PS_NORMAL;

		if (type != ST_LB)
		{
			sp.backup();
			return PS_NORMAL;
		}

		MCExpression *t_new_dimension;
		if (dimensions == 255 || sp.parseexp(False, True, &t_new_dimension) != PS_NORMAL)
		{
			MCperror->add(PE_VARIABLE_BADINDEX, sp);
			return PS_ERROR;
		}

		if (sp.next(type) != PS_NORMAL || type != ST_RB)
		{
			MCperror->add(PE_VARIABLE_NORBRACE, sp);
			return PS_ERROR;
		}

		if (dimensions == 0)
		{
			exp = t_new_dimension;
			dimensions = 1;
		}
		else if (dimensions == 1)
		{
			MCExpression *t_current_exp;
			t_current_exp = exp;
			exps = (MCExpression **)malloc(sizeof(MCExpression *) * 2);
			if (exps != NULL)
			{
				exps[0] = t_current_exp;
				exps[1] = t_new_dimension;
				dimensions += 1;
			}
		}
		else
		{
			MCExpression **t_new_exps;
			t_new_exps = (MCExpression **)realloc(exps, sizeof(MCExpression *) * (dimensions + 1));
			if (t_new_exps != NULL)
			{
				t_new_exps[dimensions] = t_new_dimension;
				exps = t_new_exps;
				dimensions += 1;
			}
		}
	}

	return PS_NORMAL;
}

Exec_stat MCVarref::sets(const MCString &s)
{
	if (ref != NULL)
		return ref->sets(s);
	return handler->getvar(index, isparam)->sets(s);
}

void MCVarref::clear()
{
	if (ref != NULL)
		ref->clear(True);
	else
		handler->getvar(index, isparam)->clear(True);
}

void MCVarref::clearuql()
{
	if (ref != NULL)
		ref->clearuql();
	else
		handler->getvar(index, isparam)->clearuql();
}

// MW-2008-08-18: [[ Bug 6945 ]] Cannot delete a nested array key.
Exec_stat MCVarref::dofree(MCExecPoint &ep)
{
	if (dimensions == 0)
	{
		MCVariable *t_var;
		
		t_var = fetchvar(ep);
			
		return t_var -> remove(ep, True);
	}
	
	MCVariable *t_var;
	MCVariableValue *t_parent;
	MCHashentry *t_hash;
	if (resolve(ep, t_var, t_parent, t_hash, false) != ES_NORMAL)
		return ES_ERROR;
		
	if (t_hash != NULL)
	{
		t_parent -> remove_hash(t_hash);
		t_var -> synchronize(ep, True);
	}
		
	return ES_NORMAL;
}

//

// Resolve references to the appropriate element refered to by this Varref.
// On return:
//   r_var contains the variable containing the element
//   r_parent_hash contains the Hashentry containing the array containing the element
//   r_hash is the Hashentry being referred to.
//
// If p_add is true, then nested arrays will be created as neeeded.
// If p_dont_destroy is true, then no string to array conversion will be carried out in lookup_hash
// 
// If r_parent_hash and r_hash are NULL, the element is the variable's value
// If r_parent_hash is NULL and r_hash is not NULL, the element is a key of the variable's value
//
Exec_stat MCVarref::resolve(MCExecPoint& ep, MCVariable*& r_var, MCVariableValue*& r_parent, MCHashentry*& r_hash, bool p_add)
{
	MCVariable *t_var;
	t_var = fetchvar(ep);

	MCVariableValue *t_parent;
	t_parent = &(t_var -> getvalue());
	
	MCHashentry *t_hash;
	t_hash = NULL;
	
	if (dimensions != 0)
	{
		MCExpression **t_dimensions;
		if (dimensions == 1)
			t_dimensions = &exp;
		else
			t_dimensions = exps;
		
		MCExecPoint ep2(ep);
		for(uint4 i = 0; i < dimensions; ++i)
		{
			if (t_dimensions[i] -> eval(ep2) != ES_NORMAL)
			{
				MCeerror -> add(EE_VARIABLE_BADINDEX, line, pos);
				return ES_ERROR;
			}
			
			if (ep2 . getformat() == VF_ARRAY)
			{
				MCVariableArray *t_array;
				t_array = ep2 . getarray() -> get_array();
				if (!t_array -> issequence())
				{
					MCeerror -> add(EE_VARIABLE_BADINDEX, line, pos);
					return ES_ERROR;
				}
				
				MCExecPoint ep3(ep);
				for(uint32_t t_index = 1; t_index <= t_array -> getnfilled(); t_index += 1)
				{
					MCHashentry *t_entry;
					t_entry = t_array -> lookupindex(t_index, False);
					if (t_entry == NULL)
					{
						MCeerror -> add(EE_VARIABLE_BADINDEX, line, pos);
						return ES_ERROR;
					}
					
					t_entry -> value . fetch(ep3);
				
					if (t_hash != NULL)
						t_parent = &t_hash -> value;

					if (t_parent != NULL && t_parent -> lookup_hash(ep, ep3 . getsvalue(), p_add, t_hash) != ES_NORMAL)
						return ES_ERROR;
					
					// MW-2009-01-15: If we failed to find the hash entry, then make sure we
					//   set the parent to NULL to stop further searches. Failing to do this
					//   means tArray[a][b][c] returns tArray[a][c] if tArray[a][b] doesn't
					//   exist.
					if (t_hash == NULL)
						t_parent = NULL;
				}
			}
			else
			{
				if (t_hash != NULL)
					t_parent = &t_hash -> value;

				if (t_parent != NULL && t_parent -> lookup_hash(ep, ep2 . getsvalue(), p_add, t_hash) != ES_NORMAL)
					return ES_ERROR;
					
				// MW-2009-01-15: If we failed to find the hash entry, then make sure we
				//   set the parent to NULL to stop further searches. Failing to do this
				//   means tArray[a][b][c] returns tArray[a][c] if tArray[a][b] doesn't
				//   exist.
				if (t_hash == NULL)
					t_parent = NULL;
			}
		}
	}

	r_var = t_var;
	r_parent = t_parent;
	r_hash = t_hash;

	return ES_NORMAL;
}

////////////////////////////////////////////////////////////////////////////////

bool MCDeferredVariable::createwithname_cstring(const char *p_name, MCDeferredVariableComputeCallback p_callback, void *p_context, MCVariable*& r_var)
{
	MCDeferredVariable *self;
	self = new MCDeferredVariable;
	if (self == nil)
		return false;

	self -> next = nil;
	/* UNCHECKED */ MCNameCreateWithCString(p_name, self -> name);

	self -> is_msg = false;
	self -> is_env = false;
	self -> is_global = false;
	self -> is_deferred = true;
	self -> is_uql = false;

	self -> m_callback = p_callback;
	self -> m_context = p_context;

	r_var = self;

	return true;
}

Exec_stat MCDeferredVariable::compute(void)
{
	// Compute can only be called once. By setting deferred to false here, any
	// future access of the variable via an MCDeferredVarref will result in this
	// not being called.
	is_deferred = false;

	// Request the variable's value be computed.
	Exec_stat t_stat;
	t_stat = m_callback(m_context, this);
	if (t_stat != ES_NORMAL)
		return t_stat;

	// We are done.
	return ES_NORMAL;
}

Exec_stat MCDeferredVarref::eval(MCExecPoint& ep)
{
	Exec_stat t_stat;
	if (ref -> isdeferred())
		t_stat = static_cast<MCDeferredVariable *>(ref) -> compute();
	else
		t_stat = ES_NORMAL;

	if (t_stat == ES_NORMAL)
		t_stat = MCVarref::eval(ep);

	return t_stat;
}

Exec_stat MCDeferredVarref::evalcontainer(MCExecPoint& ep, MCVariable*& r_var, MCVariableValue*& r_ref)
{
	Exec_stat t_stat;
	if (ref -> isdeferred())
		t_stat = static_cast<MCDeferredVariable *>(ref) -> compute();
	else
		t_stat = ES_NORMAL;

	if (t_stat == ES_NORMAL)
		t_stat = MCVarref::evalcontainer(ep, r_var, r_ref);

	return t_stat;
}

MCVariable *MCDeferredVarref::evalvar(MCExecPoint& ep)
{
	if (ref -> isdeferred())
		static_cast<MCDeferredVariable *>(ref) -> compute();

	return MCVarref::evalvar(ep);
}

////////////////////////////////////////////////////////////////////////////////

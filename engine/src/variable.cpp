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
//#include "execpt.h"
#include "scriptpt.h"
#include "mcerror.h"
#include "util.h"
#include "debug.h"
#include "globals.h"
#include "objectstream.h"
#include "parentscript.h"
#include "osspec.h"
#include "syntax.h"
#include "variable.h"

////////////////////////////////////////////////////////////////////////////////

bool MCVariable::create(MCVariable*& r_var)
{
	MCVariable *self;
	self = new MCVariable;
	if (self == nil)
		return false;

	self -> next = nil;
	self -> name = nil;

	self -> value = MCValueRetain(kMCNull);

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
		t_success = MCValueCopy(p_var . value, self -> value);

	if (t_success)
		r_new_var = self;
	else
		delete self;

	return t_success;
}

MCVariable::~MCVariable(void)
{
	MCNameDelete(name);
	MCValueRelease(value);
}

////////////////////////////////////////////////////////////////////////////////

bool MCVariable::isuql(void) const
{
	return is_uql;
}

void MCVariable::clearuql(void)
{
	if (!is_uql)
		return;
	is_uql = false;
}

Boolean MCVariable::isclear(void) const
{
	return value == kMCNull;
}

Boolean MCVariable::isfree(void) const
{
	return value == kMCNull;
}

Boolean MCVariable::isarray(void) const
{
	return MCValueGetTypeCode(value) == kMCValueTypeCodeArray;
}

Boolean MCVariable::isempty(void) const
{
	if (value == kMCEmptyName)
		return True;
	return MCValueGetTypeCode(value) == kMCValueTypeCodeString && MCStringGetLength((MCStringRef)value) == 0;
}

void MCVariable::clear(Boolean p_delete_buffer)
{
	MCValueRelease(value);
	value = MCValueRetain(kMCNull);
}

Exec_stat MCVariable::sets(const MCString& p_string)
{
	copysvalue(p_string);
	return ES_NORMAL;
}

void MCVariable::copysvalue(const MCString& p_string)
{
	MCStringRef t_string;
	if (MCStringCreateWithNativeChars((const char_t *)p_string . getstring(), p_string . getlength(), t_string))
	{
		MCValueRelease(value);
		value = t_string;
		return;
	}
}

void MCVariable::grab(char *p_buffer, uint32_t p_length)
{
	copysvalue(MCString(p_buffer, p_length));
	free(p_buffer);
}

void MCVariable::setnvalue(real8 p_number)
{
	MCNumberRef t_number;
	if (!MCNumberCreateWithReal(p_number, t_number))
		return;

	MCValueRelease(value);
	value = t_number;
}

bool MCVariable::setvalueref(MCValueRef p_value)
{
	return setvalueref(nil, 0, false, p_value);
}

bool MCVariable::setvalueref(MCNameRef *p_path, uindex_t p_length, bool p_case_sensitive, MCValueRef p_value)
{
	if (p_length == 0)
	{
		MCValueRef t_new_value;
		if (!MCValueCopy(p_value, t_new_value))
			return false;
		MCValueRelease(value);
		value = t_new_value;
		return true;
	}

	if (!converttomutablearray())
		return false;

	MCValueRef t_copied_value;
	if (!MCValueCopy(p_value, t_copied_value))
		return false;

	if (MCArrayStoreValueOnPath((MCArrayRef)value, p_case_sensitive, p_path, p_length, t_copied_value))
		return true;

	return false;
}

MCValueRef MCVariable::getvalueref(void)
{
	if (!is_uql)
		return value;
	return name;
}

MCValueRef MCVariable::getvalueref(MCNameRef *p_path, uindex_t p_length, bool p_case_sensitive)
{
	if (p_length == 0)
		return getvalueref();

	MCValueRef t_value;
	if (MCValueGetTypeCode(value) == kMCValueTypeCodeArray &&
		MCArrayFetchValueOnPath((MCArrayRef)value, p_case_sensitive, p_path, p_length, t_value))
		return t_value;

	return kMCEmptyString;
}

bool MCVariable::copyasvalueref(MCValueRef& r_value)
{
    return copyasvalueref(nil, 0, false, r_value);
}

bool MCVariable::copyasvalueref(MCNameRef *p_path, uindex_t p_length, bool p_case_sensitive, MCValueRef& r_value)
{
    return MCValueCopy(getvalueref(p_path, p_length, p_case_sensitive), r_value);
}

#ifdef LEGACY_EXEC
Exec_stat MCVariable::eval(MCExecPoint& ep)
{
    return eval(ep, nil, 0);
}


Exec_stat MCVariable::eval(MCExecPoint& ep, MCNameRef *p_path, uindex_t p_length)
{
    MCExecContext ctxt(ep);

    MCAutoValueRef t_value;
    if (eval(ctxt, p_path, p_length, &t_value) &&
            ep . setvalueref(*t_value))
        return ES_NORMAL;

    return ES_ERROR;
}
#endif

bool MCVariable::eval(MCExecContext& ctxt, MCValueRef& r_value)
{
    return eval(ctxt, nil, 0, r_value);
}

bool MCVariable::eval(MCExecContext& ctxt, MCNameRef *p_path, uindex_t p_length, MCValueRef& r_value)
{
    return copyasvalueref(p_path, p_length, ctxt . GetCaseSensitive(), r_value);
}

#ifdef LEGACY_EXEC
Exec_stat MCVariable::set(MCExecPoint& ep)
{
    return set(ep, nil, 0);
}

Exec_stat MCVariable::set(MCExecPoint& ep, MCNameRef *p_path, uindex_t p_length)
{
    MCAutoValueRef t_value;
    if (!ep . copyasvalueref(&t_value))
        return ES_ERROR;

    MCExecContext ctxt(ep);
    if (set(ctxt, *t_value, p_path, p_length))
        return ES_NORMAL;

    return ES_ERROR;
}
#endif

bool MCVariable::set(MCExecContext& ctxt, MCValueRef p_value)
{
    return set(ctxt, p_value, nil, 0);
}

bool MCVariable::set(MCExecContext& ctxt, MCValueRef p_value, MCNameRef *p_path, uindex_t p_length)
{
    MCAutoValueRef t_value;
    MCValueCopy(p_value, &t_value);
    
    if (setvalueref(p_path, p_length, ctxt . GetCaseSensitive(), *t_value))
    {
        synchronize(ctxt, true);
        return true;
    }
    
    return false;
}

#ifdef LEGACY_EXEC
Exec_stat MCVariable::append(MCExecPoint& ep)
{
    return append(ep, nil, 0);
}

Exec_stat MCVariable::append(MCExecPoint& ep, MCNameRef *p_path, uindex_t p_length)
{
    MCAutoValueRef t_value;
    if (!ep . copyasvalueref(&t_value))
        return ES_ERROR;

    MCExecContext ctxt(ep);
    if (append(ctxt, *t_value, p_path, p_length))
        return ES_NORMAL;

    return ES_ERROR;
}
#endif

bool MCVariable::append(MCExecContext& ctxt, MCValueRef p_value)
{
	return append(ctxt, p_value, nil, 0);
}

bool MCVariable::append(MCExecContext& ctxt, MCValueRef p_value, MCNameRef *p_path, uindex_t p_length)
{
    MCAutoStringRef t_value;
    if (!ctxt . ConvertToString(p_value, &t_value))
        return false;
    
	if (p_length == 0)
	{
        if (!converttomutablestring(ctxt))
            return false;
        
		if (!MCStringAppend((MCStringRef)value, *t_value))
			return false;
        
        synchronize(ctxt, true);
        
		return true;
	}

	MCValueRef t_current_value;
	t_current_value = getvalueref(p_path, p_length, ctxt . GetCaseSensitive());
	
	MCStringRef t_current_value_as_string;
	t_current_value_as_string = nil;
	if (ctxt . ConvertToString(t_current_value, t_current_value_as_string) &&
		MCStringMutableCopyAndRelease(t_current_value_as_string, t_current_value_as_string) &&
		MCStringAppend(t_current_value_as_string, *t_value) &&
		setvalueref(p_path, p_length, ctxt . GetCaseSensitive(), t_current_value_as_string))
	{
		MCValueRelease(t_current_value_as_string);
        synchronize(ctxt, true);
		return true;
	}
    
	MCValueRelease(t_current_value_as_string);
	return false;
}

#ifdef LEGACY_EXEC
Exec_stat MCVariable::remove(MCExecPoint& ep)
{
    return remove(ep, nil, 0);
}

Exec_stat MCVariable::remove(MCExecPoint& ep, MCNameRef *p_path, uindex_t p_length)
{
    MCExecContext ctxt(ep);
    if (remove(ctxt, p_path, p_length))
        return ES_NORMAL;

    return ES_ERROR;
}
#endif

bool MCVariable::remove(MCExecContext& ctxt)
{
	return remove(ctxt, nil, 0);
}

bool MCVariable::remove(MCExecContext& ctxt, MCNameRef *p_path, uindex_t p_length)
{
	if (p_length == 0)
	{
		clear();
		
		if (is_env)
		{
			if (!isdigit(MCNameGetCharAtIndex(name, 1)) && MCNameGetCharAtIndex(name, 1) != '#')
			{
				MCAutoStringRef t_env;
				/* UNCHECKED */ MCStringCopySubstring(MCNameGetString(name), MCRangeMake(1, MCStringGetLength(MCNameGetString(name))), &t_env);
				MCS_unsetenv(*t_env);
			}
		}
	}
    
	if (MCValueGetTypeCode(value) != kMCValueTypeCodeArray)
		return true;
    
	if (!converttomutablearray())
		return false;
    
	MCArrayRemoveValueOnPath((MCArrayRef)value, ctxt . GetCaseSensitive(), p_path, p_length);
    
	return true;
    
}


bool MCVariable::converttomutablearray(void)
{
	if (MCValueGetTypeCode(value) == kMCValueTypeCodeArray)
	{
		MCArrayRef t_array;
		if (!MCArrayMutableCopyAndRelease((MCArrayRef)value, t_array))
			return false;
		value = t_array;
	}
	else
	{
		MCArrayRef t_array;
		if (!MCArrayCreateMutable(t_array))
			return false;
		MCValueRelease(value);
		value = t_array;
	}

	return true;
}

bool MCVariable::converttoarrayofstrings(MCExecContext& ctxt)
{
	return false;
}

#ifdef LEGACY_EXEC
bool MCVariable::converttoarrayofstrings(MCExecPoint& ep)
{
	return false;
}
#endif

#ifdef LEGACY_EXEC
bool MCVariable::converttomutablestring(MCExecPoint& ep)
{
	if (MCValueGetTypeCode(value) != kMCValueTypeCodeString)
	{
		MCStringRef t_string = nil;
		if (!ep.convertvaluereftostring(value, t_string))
		{
			MCStringRef t_mutable_string;
			if (MCStringCreateMutable(0, t_mutable_string))
			{
				MCValueRelease(value);
				value = t_mutable_string;
				return true;
			}

			return false;
		}

		MCValueRelease(value);
		value = t_string;
	}

	MCStringRef t_mutable_string;
	if (MCStringMutableCopyAndRelease((MCStringRef)value, t_mutable_string))
	{
		value = t_mutable_string;
		return true;
	}
	return false;
}
#endif

bool MCVariable::converttomutablestring(MCExecContext& ctxt)
{
	if (MCValueGetTypeCode(value) != kMCValueTypeCodeString)
	{
		MCStringRef t_string = nil;
		if (!ctxt . ConvertToString(value, t_string))
		{
			MCStringRef t_mutable_string;
			if (MCStringCreateMutable(0, t_mutable_string))
			{
				MCValueRelease(value);
				value = t_mutable_string;
				return true;
			}
            
			return false;
		}
        
		MCValueRelease(value);
		value = t_string;
	}
    
	MCStringRef t_mutable_string;
	if (MCStringMutableCopyAndRelease((MCStringRef)value, t_mutable_string))
	{
		value = t_mutable_string;
		return true;
	}
	return false;
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

#if 0
void MCVariable::synchronize(MCExecPoint& ep, Boolean notify)
{
	MCExecContext ctxt(ep);
	if (is_env)
	{
		if (!isdigit(MCNameGetCharAtIndex(name, 1)) && MCNameGetCharAtIndex(name, 1) != '#')
		{
			MCAutoStringRef t_string;
			if (ep . copyasstringref(&t_string))
			{
				MCAutoStringRef t_env;
				/* UNCHECKED */ MCStringCopySubstring(MCNameGetString(name), MCRangeMake(1, MCStringGetLength(MCNameGetString(name))), &t_env);
				MCS_setenv(*t_env, *t_string);
			}
		}
	}
	else if (is_msg)
	{
		eval(ep);
		MCAutoStringRef t_string;
		/* UNCHECPED */ ep.copyasstringref(&t_string);
		MCB_setmsg(ctxt, *t_string);
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
				eval(ep);
				MCAutoStringRef t_string;
				/* UNCHECKED */ ep.copyasstringref(&t_string);
				if (MCwatchedvars[i].expression != nil && !MCStringIsEmpty(MCwatchedvars[i].expression))
				{
					MCExecPoint ep2(ep);
					MCExecContext ctxt(ep2);
					MCAutoValueRef t_val;
					ctxt.GetHandler()->eval(ctxt, MCwatchedvars[i].expression, &t_val);
					
					MCAutoBooleanRef t_bool;
					if (!ctxt.HasError() && ctxt.ConvertToBoolean(*t_val, &t_bool) && *t_bool == kMCTrue)
						MCB_setvar(ctxt, *t_string, name);
				}
				else
					MCB_setvar(ctxt, *t_string, name);

				break;
			}
		}
	}
}
#endif

void MCVariable::synchronize(MCExecContext& ctxt, bool p_notify)
{
	if (is_env)
	{
		if (!isdigit(MCNameGetCharAtIndex(name, 1)) && MCNameGetCharAtIndex(name, 1) != '#')
		{
			MCAutoStringRef t_string;
            if (ctxt . ConvertToString(value, &t_string))
            {
                MCAutoStringRef t_env;
				/* UNCHECKED */ MCStringCopySubstring(MCNameGetString(name), MCRangeMake(1, MCStringGetLength(MCNameGetString(name))), &t_env);
				MCS_setenv(*t_env, *t_string);
            }
		}
	}
	else if (is_msg)
    {
        MCAutoStringRef t_msg;
        if (ctxt . ConvertToString(value, &t_msg))
            MCB_setmsg(ctxt, *t_msg);
	}
    
	if (p_notify && MCnwatchedvars)
	{
		uint2 i;
		for (i = 0 ; i < MCnwatchedvars ; i++)
		{
			if ((MCwatchedvars[i].object == NULL || MCwatchedvars[i].object == ctxt . GetObject()) &&
				(MCwatchedvars[i].handlername == NULL || ctxt . GetHandler() -> hasname(MCwatchedvars[i].handlername)) &&
				hasname(MCwatchedvars[i].varname))
			{
				// If this is a global watch (object == handlername == nil) then
				// check that this var is a global - if not carry on the search.
				if (MCwatchedvars[i] . object == NULL &&
					MCwatchedvars[i] . handlername == NULL &&
					!is_global)
					continue;

				if (MCwatchedvars[i].expression != nil && !MCStringIsEmpty(MCwatchedvars[i].expression))
				{
                    MCAutoValueRef t_val;
					ctxt.GetHandler() -> eval(ctxt, MCwatchedvars[i].expression, &t_val);
                    
					MCAutoBooleanRef t_bool;
					if (!ctxt.HasError() && ctxt.ConvertToBoolean(*t_val, &t_bool) && *t_bool == kMCTrue)
						MCB_setvar(ctxt, value, name);
				}
				else
                    MCB_setvar(ctxt, value, name);
                
				break;
			}
		}
	}
}

#if 0
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
#endif

MCVarref *MCVariable::newvarref(void)
{
	if (!is_deferred)
		return new MCVarref(this);

	return new MCDeferredVarref(this);
}

////////////////////////////////////////////////////////////////////////////////

MCContainer::~MCContainer(void)
{
	if (m_length == 0)
		return;

	for(uindex_t i = 0; i < m_length; i++)
		MCValueRelease(m_path[i]);
	MCMemoryDeleteArray(m_path);
}

#ifdef LEGACY_EXEC
Exec_stat MCContainer::eval(MCExecPoint& ep)
{
	return m_variable -> eval(ep, m_path, m_length);
}

Exec_stat MCContainer::set(MCExecPoint& ep)
{
	return m_variable -> set(ep, m_path, m_length);
}

Exec_stat MCContainer::append(MCExecPoint& ep)
{
	return m_variable -> append(ep, m_path, m_length);
}

Exec_stat MCContainer::remove(MCExecPoint& ep)
{
	return m_variable -> remove(ep, m_path, m_length);
}
#endif

bool MCContainer::eval(MCExecContext& ctxt, MCValueRef& r_value)
{
    return m_variable -> eval(ctxt, m_path, m_length, r_value);
}

bool MCContainer::set(MCExecContext& ctxt, MCValueRef p_value)
{
	return m_variable -> set(ctxt, p_value, m_path, m_length);
}

bool MCContainer::append(MCExecContext& ctxt, MCValueRef p_value)
{
	return m_variable -> append(ctxt, p_value, m_path, m_length);
}

bool MCContainer::remove(MCExecContext& ctxt)
{
	return m_variable -> remove(ctxt, m_path, m_length);
}


bool MCContainer::set_valueref(MCValueRef p_value)
{
	return m_variable -> setvalueref(m_path, m_length, m_case_sensitive, p_value);
}

bool MCContainer::clear(void)
{
	return set_valueref(kMCEmptyString);
}

bool MCContainer::set_oldstring(const MCString& p_string)
{
	MCAutoStringRef t_string;
	if (!MCStringCreateWithNativeChars((const char_t *)p_string . getstring(), p_string . getlength(), &t_string))
		return false;
	return set_valueref(*t_string);
}

bool MCContainer::set_cstring(const char *p_string)
{
	return set_oldstring(p_string);
}

bool MCContainer::set_real(double p_real)
{
	MCAutoNumberRef t_number;
	if (!MCNumberCreateWithReal(p_real, &t_number))
		return false;
	return set_valueref(*t_number);
}

bool MCContainer::createwithvariable(MCVariable *p_var, MCContainer*& r_container)
{
	r_container = new MCContainer;
	r_container -> m_variable = p_var;
	r_container -> m_length = 0;
	r_container -> m_case_sensitive = false;
	return true;
}

bool MCContainer::createwithpath(MCVariable *p_var, MCNameRef *p_path, uindex_t p_length, MCContainer*& r_container)
{
	r_container = new MCContainer;
	r_container -> m_variable = p_var;
	r_container -> m_path = p_path;
	r_container -> m_length = p_length;
	r_container -> m_case_sensitive = false;
	return true;
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

#ifdef LEGACY_EXEC
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
#endif

MCVariable *MCVarref::fetchvar(MCExecContext& ctxt)
{
	// MW-2009-01-28: [[ Inherited parentScripts ]]
	// If we are in parentScript context, then fetch the script local from there,
	// otherwise use the information stored in this.
	MCParentScriptUse *t_parentscript;
	t_parentscript = ctxt . GetParentScript();
	if (!isscriptlocal || t_parentscript == NULL)
	{
		if (ref != NULL)
			return ref;
        
		return handler -> getvar(index, isparam);
	}
	
	return t_parentscript -> GetVariable(index);
}

#ifdef LEGACY_EXEC
MCVariable *MCVarref::evalvar(MCExecPoint& ep)
{
	if (dimensions != 0)
		return NULL;

	return fetchvar(ep);
}
#endif

MCVariable *MCVarref::evalvar(MCExecContext& ctxt)
{
    if (dimensions != 0)
        return NULL;

    return fetchvar(ctxt);
}

#ifdef LEGACY_EXEC
Exec_stat MCVarref::eval(MCExecPoint& ep)
{
    MCExecContext ctxt(ep);
    MCAutoValueRef t_value;
	if (dimensions == 0)
	{
		MCVariable *t_resolved_ref;
		
		t_resolved_ref = fetchvar(ctxt);

		if (t_resolved_ref -> eval(ctxt, &t_value))
        {
            ep . setvalueref(*t_value);
            return ES_NORMAL;
        }
        return ES_ERROR;
	}

	MCAutoPointer<MCContainer> t_container;
	if (!resolve(ctxt, &t_container))
		return ES_ERROR;

    if (t_container -> eval(ctxt, &t_value))
    {
        ep . setvalueref(*t_value);
        return ES_NORMAL;
    }
	return ES_ERROR;
}
#endif

void MCVarref::eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value)
{
    MCValueRef t_value;
	if (dimensions == 0)
	{
        MCVariable *t_resolved_ref;
		
		t_resolved_ref = fetchvar(ctxt);
        
        if (!t_resolved_ref -> copyasvalueref(t_value))
            ctxt . Throw();
        else
            MCExecValueTraits<MCValueRef>::set(r_value, t_value);

        return;
	}
    else
    {
        MCAutoPointer<MCContainer> t_container;
        if (!resolve(ctxt, &t_container)
                || !t_container -> eval(ctxt, t_value))
            ctxt . Throw();
        else
            MCExecValueTraits<MCValueRef>::set(r_value, t_value);
    }
}

#ifdef LEGACY_EXEC
Exec_stat MCVarref::evalcontainer(MCExecPoint& ep, MCContainer*& r_container)
{
	if (dimensions == 0)
	{
		/* UNCHECKED */ MCContainer::createwithvariable(fetchvar(ep), r_container);
		return ES_NORMAL;
	}

	return resolve(ep, r_container);
}
#endif

bool MCVarref::evalcontainer(MCExecContext& ctxt, MCContainer*& r_container)
{
    if (dimensions == 0)
        return MCContainer::createwithvariable(fetchvar(ctxt), r_container);
    else
        return resolve(ctxt, r_container);
}

void MCVarref::compile(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginExpression(ctxt, line, pos);
	MCSyntaxFactoryEvalUnimplemented(ctxt);
	MCSyntaxFactoryEndExpression(ctxt);
}

void MCVarref::compile_in(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginExpression(ctxt, line, pos);
	MCSyntaxFactoryEvalUnimplemented(ctxt);
	MCSyntaxFactoryEndExpression(ctxt);
}

void MCVarref::compile_out(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginExpression(ctxt, line, pos);
	MCSyntaxFactoryEvalUnimplemented(ctxt);
	MCSyntaxFactoryEndExpression(ctxt);
}

void MCVarref::compile_inout(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginExpression(ctxt, line, pos);
	MCSyntaxFactoryEvalUnimplemented(ctxt);
	MCSyntaxFactoryEndExpression(ctxt);
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

#ifdef LEGACY_EXEC
Exec_stat MCVarref::set(MCExecPoint &ep, Boolean append)
{
	if (dimensions == 0)
	{
		MCVariable *t_resolved_ref;

		t_resolved_ref = fetchvar(ep);

		if (!append)
			return t_resolved_ref -> set(ep);

		return t_resolved_ref -> append(ep);
	}

	MCAutoPointer<MCContainer> t_container;
	if (resolve(ep, &t_container) != ES_NORMAL)
		return ES_ERROR;
	
	if (!append)
		return t_container -> set(ep);

	return t_container -> append(ep);
}
#endif

bool MCVarref::set(MCExecContext& ctxt, MCValueRef p_value, bool p_append)
{
	if (dimensions == 0)
	{
		MCVariable *t_resolved_ref;
        
		t_resolved_ref = fetchvar(ctxt);
        
		if (!p_append)
			return t_resolved_ref -> set(ctxt, p_value);
        
		return t_resolved_ref -> append(ctxt, p_value);
	}
    
	MCAutoPointer<MCContainer> t_container;
    if (!resolve(ctxt, &t_container))
		return false;
	
	if (!p_append)
		return t_container -> set(ctxt, p_value);
    
	return t_container -> append(ctxt, p_value);
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
#ifdef LEGACY_EXEC
Exec_stat MCVarref::dofree(MCExecPoint &ep)
{
	if (dimensions == 0)
	{
		MCVariable *t_var;
		
		t_var = fetchvar(ep);
			
        return t_var -> remove(ep);
	}
	
	MCAutoPointer<MCContainer> t_container;
	if (resolve(ep, &t_container) != ES_NORMAL)
		return ES_ERROR;

	return t_container -> remove(ep);
}
#endif

bool MCVarref::dofree(MCExecContext& ctxt)
{
	if (dimensions == 0)
	{
		MCVariable *t_var;
		
		t_var = fetchvar(ctxt);
        
		return t_var -> remove(ctxt);
	}
	
	MCAutoPointer<MCContainer> t_container;
    if (!resolve(ctxt, &t_container))
        return false;
    
	return t_container -> remove(ctxt);
}

//
#ifdef LEGACY_EXEC
Exec_stat MCVarref::resolve(MCExecPoint& ep, MCContainer*& r_container)
{
	MCVariable *t_var;
	t_var = fetchvar(ep);

	if (dimensions == 0)
	{
		/* UNCHECKED */ MCContainer::createwithvariable(t_var, r_container);
		return ES_NORMAL;
	}

	MCExpression **t_dimensions;
	if (dimensions == 1)
		t_dimensions = &exp;
	else
		t_dimensions = exps;

	uindex_t t_dimension_count;
	t_dimension_count = dimensions;

	uindex_t t_path_length;
	t_path_length = 0;

	MCNameRef *t_path;
	/* UNCHECKED */ MCMemoryNewArray(dimensions, t_path);

	Exec_stat t_stat;
	t_stat = ES_NORMAL;

	MCExecPoint ep2(ep);
	for(uindex_t i = 0; i < dimensions && t_stat == ES_NORMAL; i++)
	{
		t_stat = t_dimensions[i] -> eval(ep2);

		if (t_stat == ES_NORMAL)
		{
			if (ep2 . isarray() && !ep2 . isempty())
			{
				MCArrayRef t_array;
				t_array = ep2 . getarrayref();

				if (!MCArrayIsSequence(t_array))
				{
					MCeerror -> add(EE_VARIABLE_BADINDEX, line, pos);
					t_stat = ES_ERROR;
				}

				if (t_stat == ES_NORMAL)
				{
					uindex_t t_length;
					t_length = MCArrayGetCount(t_array);

					/* UNCHECKED */ MCMemoryResizeArray(t_dimension_count + t_length, t_path, t_dimension_count);

					MCExecPoint ep3(ep);
					for(uindex_t t_index = 1; t_index <= t_length; t_index += 1)
					{
						if (!ep3 . fetcharrayindex(t_array, t_index))
							ep3 . clear();
						
						if (!ep3 . copyasnameref(t_path[t_path_length++]))	
						{
							MCeerror -> add(EE_VARIABLE_BADINDEX, line, pos);
							t_stat = ES_ERROR;
							break;
						}
					}
				}
			}
			else if (!ep2 . copyasnameref(t_path[t_path_length++]))
			{
				MCeerror -> add(EE_VARIABLE_BADINDEX, line, pos);
				t_stat = ES_ERROR;
			}
		}
	}

	if (t_stat == ES_NORMAL)
		/* UNCHECKED */ MCContainer::createwithpath(t_var, t_path, t_path_length, r_container);
	else
	{
		for(uindex_t i = 0; i < t_path_length; i++)
			MCValueRelease(t_path[i]);
		MCMemoryDeleteArray(t_path);
	}

	return t_stat;
}
#endif

bool MCVarref::resolve(MCExecContext& ctxt, MCContainer*& r_container)
{
	MCVariable *t_var;
	t_var = fetchvar(ctxt);
    
	if (dimensions == 0)
	{
		/* UNCHECKED */ MCContainer::createwithvariable(t_var, r_container);
        return true;
	}
    
	MCExpression **t_dimensions;
	if (dimensions == 1)
		t_dimensions = &exp;
	else
		t_dimensions = exps;
    
	uindex_t t_dimension_count;
	t_dimension_count = dimensions;
    
	uindex_t t_path_length;
	t_path_length = 0;
    
	MCNameRef *t_path;
	/* UNCHECKED */ MCMemoryNewArray(dimensions, t_path);    

    for(uindex_t i = 0; i < dimensions && !ctxt . HasError(); i++)
	{
        MCAutoValueRef t_value;
        if (ctxt . EvalExprAsValueRef(t_dimensions[i], EE_VARIABLE_BADINDEX, &t_value))
        {
            MCAutoArrayRef t_array;

            if (ctxt . ConvertToArray(*t_value, &t_array)
                    && !MCArrayIsEmpty(*t_array))
            {
                if (!MCArrayIsSequence(*t_array))
                    ctxt . LegacyThrow(EE_VARIABLE_BADINDEX);
                else
				{
					uindex_t t_length;
                    t_length = MCArrayGetCount(*t_array);
                    
					/* UNCHECKED */ MCMemoryResizeArray(t_dimension_count + t_length, t_path, t_dimension_count);

					for(uindex_t t_index = 1; t_index <= t_length; t_index += 1)
                    {
                        MCAutoValueRef t_value_fetched;
                        /* UNCHECKED */ MCArrayFetchValueAtIndex(*t_array, t_index, &t_value_fetched);

                        if (!ctxt . ConvertToName(*t_value_fetched, t_path[t_path_length++]))
                        {
                            ctxt . LegacyThrow(EE_VARIABLE_BADINDEX);
							break;
						}
					}
				}
			}
            else if (!ctxt . ConvertToName(*t_value, t_path[t_path_length++]))
                ctxt . LegacyThrow(EE_VARIABLE_BADINDEX);
		}
	}

    if (!ctxt . HasError())
    {
        /* UNCHECKED */ MCContainer::createwithpath(t_var, t_path, t_path_length, r_container);
        return true;
    }
	else
	{
		for(uindex_t i = 0; i < t_path_length; i++)
			MCValueRelease(t_path[i]);
        MCMemoryDeleteArray(t_path);

        return false;
    }
}

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
#if 0
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
#endif

////////////////////////////////////////////////////////////////////////////////

bool MCDeferredVariable::createwithname(MCNameRef p_name, MCDeferredVariableComputeCallback p_callback, void *p_context, MCVariable*& r_var)
{
	MCDeferredVariable *self;
	self = new MCDeferredVariable;
	if (self == nil)
		return false;

	self -> next = nil;
	self -> name = MCValueRetain(p_name);

	self -> value = nil;
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

#ifdef LEGACY_EXEC
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
#endif

void MCDeferredVarref::eval_ctxt(MCExecContext &ctxt, MCExecValue &r_value)
{
    bool t_error;
    if (ref -> isdeferred())
        t_error = static_cast<MCDeferredVariable *>(ref) -> compute() != ES_NORMAL;
    else
        t_error = false;

    if (!t_error)
        MCVarref::eval_ctxt(ctxt, r_value);
    else
        ctxt . Throw();
}

#ifdef LEGACY_EXEC
Exec_stat MCDeferredVarref::evalcontainer(MCExecPoint& ep, MCContainer*& r_container)
{
	Exec_stat t_stat;
	if (ref -> isdeferred())
		t_stat = static_cast<MCDeferredVariable *>(ref) -> compute();
	else
		t_stat = ES_NORMAL;

	if (t_stat == ES_NORMAL)
		t_stat = MCVarref::evalcontainer(ep, r_container);

	return t_stat;
}
#endif

bool MCDeferredVarref::evalcontainer(MCExecContext &ctxt, MCContainer *&r_container)
{
    bool t_error = false;
    if (ref -> isdeferred())
        t_error = static_cast<MCDeferredVariable *>(ref) -> compute() != ES_NORMAL;

    if (!t_error)
        t_error = MCVarref::evalcontainer(ctxt, r_container);

    return t_error;
}

#ifdef LEGACY_EXEC
MCVariable *MCDeferredVarref::evalvar(MCExecPoint& ep)
{
	if (ref -> isdeferred())
		static_cast<MCDeferredVariable *>(ref) -> compute();

	return MCVarref::evalvar(ep);
}
#endif

MCVariable *MCDeferredVarref::evalvar(MCExecContext &ctxt)
{
    if (ref -> isdeferred())
        static_cast<MCDeferredVariable *>(ref) -> compute();

    return MCVarref::evalvar(ctxt);
}

////////////////////////////////////////////////////////////////////////////////

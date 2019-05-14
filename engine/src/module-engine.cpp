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

////////////////////////////////////////////////////////////////////////////////

#include "prefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "mcio.h"

#include "object.h"
#include "stack.h"
#include "exec.h"
#include "chunk.h"
#include "scriptpt.h"
#include "param.h"
#include "card.h"
#include "redraw.h"
#include "eventqueue.h"
#include "globals.h"
#include "hndlrlst.h"

#include "dispatch.h"
#include "notify.h"

#include "module-engine.h"
#include "widget.h"
#include "libscript/script.h"
#include "filepath.h"
#include "osspec.h"

////////////////////////////////////////////////////////////////////////////////

typedef struct __MCScriptObject *MCScriptObjectRef;

struct __MCScriptObjectImpl
{
    MCObjectHandle handle;
    uint32_t part_id;
};

////////////////////////////////////////////////////////////////////////////////

MC_DLLEXPORT_DEF MCTypeInfoRef kMCEngineScriptObjectTypeInfo;

extern "C" MC_DLLEXPORT_DEF MCTypeInfoRef MCEngineScriptObjectTypeInfo()
{ return kMCEngineScriptObjectTypeInfo; }

////////////////////////////////////////////////////////////////////////////////

static bool s_last_message_was_handled = false;

////////////////////////////////////////////////////////////////////////////////

bool MCEngineScriptObjectCreate(MCObject *p_object, uint32_t p_part_id, MCScriptObjectRef& r_script_object)
{
    MCScriptObjectRef t_script_object;
    if (!MCValueCreateCustom(kMCEngineScriptObjectTypeInfo, sizeof(__MCScriptObjectImpl), t_script_object))
        return false;
    
    __MCScriptObjectImpl *t_script_object_imp;
    t_script_object_imp = (__MCScriptObjectImpl *)MCValueGetExtraBytesPtr(t_script_object);
    t_script_object_imp -> handle = p_object != nil ? p_object -> GetHandle() : MCObjectHandle(nil);
    t_script_object_imp -> part_id = p_part_id;
    
    r_script_object = t_script_object;
    
    return true;
}

////////////////////////////////////////////////////////////////////////////////

bool MCEngineThrowScriptError(void)
{
    // TODO: Process MCeerror and such.
    MCErrorCreateAndThrow(kMCGenericErrorTypeInfo, "reason", MCSTR("script error"), nil);
    return false;
}

bool MCEngineThrowScripObjectDoesNotExistError(void)
{
	MCErrorCreateAndThrow(kMCEngineScriptObjectDoesNotExistErrorTypeInfo, nil);
	return false;
}

bool MCEngineThrowNoScriptContextError(void)
{
    MCErrorCreateAndThrow(kMCEngineScriptObjectNoContextErrorTypeInfo, nil);
    return false;
}

////////////////////////////////////////////////////////////////////////////////

static uint32_t s_script_object_access_lock_count;

bool MCEngineEnsureScriptObjectAccessIsAllowed(void)
{
    if (s_script_object_access_lock_count != 0)
    {
        return MCEngineThrowNoScriptContextError();
    }
    
    return true;
}

void MCEngineScriptObjectPreventAccess(void)
{
    s_script_object_access_lock_count += 1;
}

void MCEngineScriptObjectAllowAccess(void)
{
    s_script_object_access_lock_count -= 1;
}

////////////////////////////////////////////////////////////////////////////////

MCValueRef
MCEngineEvalScriptResult (MCExecContext& ctxt)
{
	if (MCresult->isclear())
	{
		return nil;
	}

	MCAutoValueRef t_result(MCresult->getvalueref());
	if (!MCExtensionConvertFromScriptType(ctxt, kMCAnyTypeInfo,
	                                      InOut(t_result)))
	{
		return nil;
	}
	return t_result.Take();
}

////////////////////////////////////////////////////////////////////////////////

extern "C" MC_DLLEXPORT_DEF MCScriptObjectRef MCEngineExecResolveScriptObject(MCStringRef p_object_id)
{
    if (!MCEngineEnsureScriptObjectAccessIsAllowed())
    {
        return nil;
    }
    
    MCExecContext ctxt(MCdefaultstackptr, nil, nil);
    
	// Create a script point with the value we are taking the id from.
    MCScriptPoint sp(p_object_id);
    
	// Create a new chunk object to parse the reference into.
    MCChunk *t_chunk;
    t_chunk = new (nothrow) MCChunk(False);
    if (t_chunk == nil)
    {
        MCErrorThrowOutOfMemory();
        return nil;
    }
    
	// Attempt to parse a chunk. We also check that there is no 'junk' at
	// the end of the string - if there is, its an error. Note the errorlock
	// here - it stops parse errors being pushed onto MCperror.
	// Then attempt to evaluate the object reference - this will only succeed
	// if the object exists.
    Symbol_type t_next_type;
    MCObject *t_object;
    uint32_t t_part_id;
	MCerrorlock++;
    if (t_chunk -> parse(sp, False) != PS_NORMAL ||
        sp.next(t_next_type) != PS_EOF ||
        t_chunk -> getobj(ctxt, t_object, t_part_id, False) != ES_NORMAL)
    {
        t_object = nil;
        t_part_id = 0;
    }
    MCerrorlock--;
    
    // We can get rid of the chunk now.
    delete t_chunk;
    
    // Now build our script object.
    MCScriptObjectRef t_script_object;
    if (!MCEngineScriptObjectCreate(t_object, t_part_id, t_script_object))
        return nil;
    
    return t_script_object;
}

extern "C" MC_DLLEXPORT_DEF void MCEngineEvalScriptObjectExists(MCScriptObjectRef p_object, bool& r_exists)
{
    // This method does't require any script interaction so it always allowed.
    
    __MCScriptObjectImpl *t_script_object_imp;
    t_script_object_imp = (__MCScriptObjectImpl *)MCValueGetExtraBytesPtr(p_object);
    
    r_exists = t_script_object_imp->handle.IsValid();
}

extern "C" MC_DLLEXPORT_DEF void MCEngineEvalScriptObjectDoesNotExist(MCScriptObjectRef p_object, bool& r_not_exists)
{
    // This method does't require any script interaction so it always allowed.
    
    bool t_exists;
    MCEngineEvalScriptObjectExists(p_object, t_exists);
    r_not_exists = !t_exists;
}

//////////

static Properties parse_property_name(MCStringRef p_name)
{
	MCScriptPoint t_sp(p_name);
	Symbol_type t_type;
	const LT *t_literal;
	if (t_sp . next(t_type) &&
		t_sp . lookup(SP_FACTOR, t_literal) == PS_NORMAL &&
		t_literal -> type == TT_PROPERTY)
    {
        Properties t_which = (Properties)t_literal -> which;
        
        // check for object property modifiers
        if (t_which == P_SHORT || t_which == P_LONG || t_which == P_ABBREVIATE)
        {
            if (t_sp . next(t_type) &&
                t_sp . lookup(SP_FACTOR, t_literal) == PS_NORMAL &&
                t_literal -> type == TT_PROPERTY)
            {
                if (t_literal->which == P_ID || t_literal->which == P_NAME || t_literal->which == P_OWNER)
                {
                    t_which = (Properties)(t_literal->which + t_which - P_SHORT + 1);
                }
            }
        }
        
        if (t_sp . next(t_type) == PS_EOF)
            return t_which;
    }
    
	return P_CUSTOM;
}

bool MCEngineEvalObjectOfScriptObject(MCScriptObjectRef p_object, MCObject *&r_object, uint32_t &r_part_id)
{
	__MCScriptObjectImpl *t_script_object_imp;
	t_script_object_imp = (__MCScriptObjectImpl *)MCValueGetExtraBytesPtr(p_object);
	if (!t_script_object_imp->handle.IsValid())
	{
		return MCEngineThrowScripObjectDoesNotExistError();
	}
	
	r_object = t_script_object_imp->handle;
	r_part_id = t_script_object_imp->part_id;
	return true;
}

MCValueRef MCEngineGetPropertyOfObject(MCExecContext &ctxt, MCStringRef p_property, MCObject *p_object, uint32_t p_part_id)
{
	Properties t_prop;
	t_prop = parse_property_name(p_property);
	
	MCExecValue t_value;
	
	if (t_prop == P_CUSTOM)
	{
		MCNewAutoNameRef t_propset_name, t_propset_key;
		t_propset_name = p_object -> getdefaultpropsetname();
		if (!MCNameCreate(p_property, &t_propset_key))
			return nil;
		p_object -> getcustomprop(ctxt, *t_propset_name, *t_propset_key, nil, t_value);
	}
	else
		p_object -> getprop(ctxt, p_part_id, t_prop, nil, False, t_value);
	
	if (ctxt . HasError())
	{
		MCEngineThrowScriptError();
		return nil;
	}
	
	// This function only relies on throws from libfoundation, so we don't have to do any higher-level
	// error prodding.
	MCValueRef t_value_ref;
	MCExecTypeConvertAndReleaseAlways(ctxt, t_value . type, &t_value, kMCExecValueTypeValueRef, &t_value_ref);
	if (ctxt . HasError())
		return nil;
	
	return t_value_ref;
}

extern "C" MC_DLLEXPORT_DEF void MCEngineExecGetPropertyOfScriptObject(MCStringRef p_property, MCScriptObjectRef p_object, MCValueRef& r_value)
{
    if (!MCEngineEnsureScriptObjectAccessIsAllowed())
    {
        return;
    }
    
	MCObject *t_object;
	uint32_t t_part_id;
	if (!MCEngineEvalObjectOfScriptObject(p_object, t_object, t_part_id))
    {
        return;
    }
	
    MCExecContext ctxt(MCdefaultstackptr, nil, nil);

    // AL-2015-07-24: [[ Bug 15630 ]] Syntax binding dictates value returned as out parameter rather than directly
	r_value = MCEngineGetPropertyOfObject(ctxt, p_property, t_object, t_part_id);
}

// IM-2015-02-23: [[ WidgetPopup ]] Factored-out function for setting the named property of an object to a value.
void MCEngineSetPropertyOfObject(MCExecContext &ctxt, MCStringRef p_property, MCObject *p_object, uint32_t p_part_id, MCValueRef p_value)
{
    MCValueRef t_value_copy;
    t_value_copy = MCValueRetain(p_value);
    
    if (!MCExtensionConvertToScriptType(ctxt, t_value_copy))
    {
        MCValueRelease(t_value_copy);
        return;
    }
    
	Properties t_prop;
	t_prop = parse_property_name(p_property);
    
    // It seems using 'setproperty' takes the value in the execvalue :S
	MCExecValue t_value;
    t_value . type = kMCExecValueTypeValueRef;
    t_value . valueref_value = t_value_copy;
    
    if (t_prop == P_CUSTOM)
    {
        MCNewAutoNameRef t_propset_name, t_propset_key;
		t_propset_name = p_object -> getdefaultpropsetname();
        if (!MCNameCreate(p_property, &t_propset_key))
        {
            MCValueRelease(t_value_copy);
            return;
        }
		p_object -> setcustomprop(ctxt, *t_propset_name, *t_propset_key, nil, t_value);
    }
    else
		p_object -> setprop(ctxt, p_part_id, t_prop, nil, False, t_value);
    
    if (ctxt . HasError())
    {
        MCEngineThrowScriptError();
        return;
    }
}

extern "C" MC_DLLEXPORT_DEF void MCEngineExecSetPropertyOfScriptObject(MCValueRef p_value, MCStringRef p_property, MCScriptObjectRef p_object)
{
    if (!MCEngineEnsureScriptObjectAccessIsAllowed())
    {
        return;
    }
    
	MCObject *t_object;
	uint32_t t_part_id;
	if (!MCEngineEvalObjectOfScriptObject(p_object, t_object, t_part_id))
		return;
	
    MCExecContext ctxt(MCdefaultstackptr, nil, nil);

	MCEngineSetPropertyOfObject(ctxt, p_property, t_object, t_part_id, p_value);
}

extern "C" MC_DLLEXPORT_DEF void MCEngineEvalOwnerOfScriptObject(MCScriptObjectRef p_object, MCScriptObjectRef &r_owner)
{
    // This method does't require any script interaction so it always allowed.
    
	MCObject *t_object;
	uint32_t t_part_id;
	if (!MCEngineEvalObjectOfScriptObject(p_object, t_object, t_part_id))
		return;
	
	MCObject *t_owner;
	if (t_object->gettype() == CT_STACK && MCdispatcher->ismainstack((MCStack*)t_object))
		t_owner = nil;
	else
		t_owner = t_object->getparent();
	
	MCEngineScriptObjectCreate(t_owner, t_part_id, r_owner);
}

struct MCScriptObjectChildControlsVisitor : public MCObjectVisitor
{
	MCScriptObjectChildControlsVisitor(MCProperListRef p_list)
	{
		m_list = p_list;
	}
	
	virtual bool OnObject(MCObject *p_object)
	{
		MCAutoValueRefBase<MCScriptObjectRef> t_object_ref;
		
		return MCEngineScriptObjectCreate(p_object, 0, &t_object_ref) && MCProperListPushElementOntoBack(m_list, *t_object_ref);
	}
	
	virtual bool OnStyledText(MCStyledText *p_text)
	{
		// don't include styled text objects
		return true;
	}
	
	MCProperListRef m_list;
};

extern "C" MC_DLLEXPORT_DEF void MCEngineEvalChildrenOfScriptObject(MCScriptObjectRef p_object, MCProperListRef &r_controls)
{
    // This method does't require any script interaction so it always allowed.
    
	MCObject *t_object;
	uint32_t t_part_id;
	if (!MCEngineEvalObjectOfScriptObject(p_object, t_object, t_part_id))
		return;
	
	MCAutoProperListRef t_list;
	if (!MCProperListCreateMutable(&t_list))
		return;
	
	MCScriptObjectChildControlsVisitor t_visitor(*t_list);
	if (!t_object->visit_children(kMCObjectVisitorHeirarchical, t_part_id, &t_visitor))
		return;
	
	MCProperListCopy(*t_list, r_controls);
}

/* TODO[C++11] This should be "static" but MSVC2010 doesn't support
 * using objects with internal linkage as non-type template
 * arguments. */
void MCEngineFreeScriptParameters(MCParameter* p_params)
{
	while(p_params != nil)
	{
		MCParameter *t_param;
		t_param = p_params;
		p_params = p_params -> getnext();
		delete t_param;
	}
}

bool MCEngineConvertToScriptParameters(MCExecContext& ctxt, MCProperListRef p_arguments, MCParameter*& r_script_params)
{
    MCAutoCustomPointer<MCParameter, MCEngineFreeScriptParameters> t_params;
    MCParameter *t_last_param = nullptr;
	for(uint32_t i = 0; i < MCProperListGetLength(p_arguments); i++)
	{
        MCValueRef t_value;
        t_value = MCValueRetain(MCProperListFetchElementAtIndex(p_arguments, i));
        
        if (!MCExtensionConvertToScriptType(ctxt, t_value))
        {
            MCValueRelease(t_value);
            return false;
        }
        
		MCParameter *t_param;
		t_param = new (nothrow) MCParameter;
        
        /* setvalueref_argument retains its argument so set then release the
         * value */
		t_param -> setvalueref_argument(t_value);
        MCValueRelease(t_value);
        
		if (t_last_param == nil)
			&t_params = t_param;
		else
			t_last_param -> setnext(t_param);
        
		t_last_param = t_param;
	}
    
    r_script_params = t_params.Release();
    return true;
}

MCValueRef MCEngineDoSendToObjectWithArguments(bool p_is_function, MCStringRef p_message, MCObject *p_object, MCProperListRef p_arguments)
{
    MCNewAutoNameRef t_message_as_name;
    if (!MCNameCreate(p_message, &t_message_as_name))
        return nil;
    
    MCExecContext ctxt(MCdefaultstackptr, nil, nil);
    MCAutoCustomPointer<MCParameter, MCEngineFreeScriptParameters> t_params;
    t_params = nil;
    
    if (!MCEngineConvertToScriptParameters(ctxt, p_arguments, &t_params))
        return nullptr;
    
	/* Clear any existing value from the result to enable testing
	 * whether dispatching generated a result. */
	MCresult->clear();

    Exec_stat t_stat;
    t_stat = p_object -> dispatch(!p_is_function ? HT_MESSAGE : HT_FUNCTION, *t_message_as_name, *t_params);
    if (t_stat == ES_ERROR)
    {
        MCEngineThrowScriptError();
        return nullptr;
    }
    
    if (t_stat == ES_NORMAL)
        s_last_message_was_handled = true;
    else
        s_last_message_was_handled = false;

    return MCEngineEvalScriptResult(ctxt);
}

extern "C" MC_DLLEXPORT_DEF MCValueRef MCEngineExecSendToScriptObjectWithArguments(bool p_is_function, MCStringRef p_message, MCScriptObjectRef p_object, MCProperListRef p_arguments)
{
    if (!MCEngineEnsureScriptObjectAccessIsAllowed())
        return nil;
    
	MCObject *t_object;
	uint32_t t_part_id;
	if (!MCEngineEvalObjectOfScriptObject(p_object, t_object, t_part_id))
		return nil;
	
    return MCEngineDoSendToObjectWithArguments(p_is_function, p_message, t_object, p_arguments);
}

extern "C" MC_DLLEXPORT_DEF MCValueRef MCEngineExecSendToScriptObject(bool p_is_function, MCStringRef p_message, MCScriptObjectRef p_object)
{
    return MCEngineExecSendToScriptObjectWithArguments(p_is_function, p_message, p_object, kMCEmptyProperList);
}

extern MCWidgetRef MCcurrentwidget;
extern void MCWidgetExecPostToParentWithArguments(MCStringRef p_message, MCProperListRef p_arguments);

MCObject* MCEngineCurrentContextObject(void)
{
    MCObject *t_object = nullptr;
    if (MCcurrentwidget)
    {
        t_object = MCWidgetGetHost(MCcurrentwidget);
    }
    else if (MCdefaultstackptr)
    {
        t_object = MCdefaultstackptr->getcurcard();
    }
    
    if (t_object == nullptr)
    {
        MCErrorCreateAndThrow(kMCGenericErrorTypeInfo, "reason", MCSTR("no default stack"), nil);
        return nullptr;
    }
    
    return t_object;
}

extern "C" MC_DLLEXPORT_DEF MCValueRef MCEngineExecSendWithArguments(bool p_is_function, MCStringRef p_message, MCProperListRef p_arguments)
{
    MCObject *t_target = MCEngineCurrentContextObject();
    
    if (t_target == nullptr)
    {
        return nullptr;
    }
    
    return MCEngineDoSendToObjectWithArguments(p_is_function, p_message, t_target, p_arguments);
}

extern "C" MC_DLLEXPORT_DEF MCValueRef MCEngineExecSend(bool p_is_function, MCStringRef p_message)
{
    return MCEngineExecSendWithArguments(p_is_function, p_message, kMCEmptyProperList);
}

void MCEngineDoPostToObjectWithArguments(MCStringRef p_message, MCObject *p_object, MCProperListRef p_arguments)
{
    MCNewAutoNameRef t_message_as_name;
    if (!MCNameCreate(p_message, &t_message_as_name))
        return;
    
    MCExecContext ctxt(MCdefaultstackptr, nil, nil);
    MCParameter *t_params;
    MCValueRef t_result;
    t_params = nil;
    t_result = nil;
    
    if (!MCEngineConvertToScriptParameters(ctxt, p_arguments, t_params))
        return;
    
    MCscreen -> addmessage(p_object, *t_message_as_name, 0.0f, t_params);
    
    MCEngineRunloopBreakWait();
}

extern "C" MC_DLLEXPORT_DEF void MCEngineExecPostToScriptObjectWithArguments(MCStringRef p_message, MCScriptObjectRef p_object, MCProperListRef p_arguments)
{
    if (!MCEngineEnsureScriptObjectAccessIsAllowed())
        return;
    
	MCObject *t_object;
	uint32_t t_part_id;
	if (!MCEngineEvalObjectOfScriptObject(p_object, t_object, t_part_id))
		return;
    
    MCEngineDoPostToObjectWithArguments(p_message, t_object, p_arguments);
}

extern "C" MC_DLLEXPORT_DEF void MCEngineExecPostToScriptObject(MCStringRef p_message, MCScriptObjectRef p_object)
{
    MCEngineExecPostToScriptObjectWithArguments(p_message, p_object, kMCEmptyProperList);
}

extern "C" MC_DLLEXPORT_DEF void MCEngineExecPostWithArguments(MCStringRef p_message, MCProperListRef p_arguments)
{
    if (MCcurrentwidget && !MCWidgetIsRoot(MCcurrentwidget))
    {
        MCWidgetExecPostToParentWithArguments(p_message, p_arguments);
        return;
    }
    
    MCObject *t_target = MCEngineCurrentContextObject();
    
    if (t_target != nullptr)
    {
        MCEngineDoPostToObjectWithArguments(p_message, t_target, p_arguments);
    }
}

extern "C" MC_DLLEXPORT_DEF void MCEngineExecPost(MCStringRef p_message)
{
    MCEngineExecPostWithArguments(p_message, kMCEmptyProperList);
}

extern "C" MC_DLLEXPORT_DEF void MCEngineEvalMessageWasHandled(bool& r_handled)
{
    r_handled = s_last_message_was_handled;
}

extern "C" MC_DLLEXPORT_DEF void MCEngineEvalMessageWasNotHandled(bool& r_not_handled)
{
    bool t_handled;
    MCEngineEvalMessageWasHandled(t_handled);
    r_not_handled = !t_handled;
}

extern MCExecContext *MCECptr;
extern "C" MC_DLLEXPORT_DEF void MCEngineEvalCaller(MCScriptObjectRef& r_script_object)
{
    if (!MCEngineScriptObjectCreate(MCECptr->GetObject(), 0, r_script_object))
        return;
}
////////////////////////////////////////////////////////////////////////////////

static MCValueRef
MCEngineDoExecuteScriptInObjectWithArguments(MCStringRef p_script, MCObject *p_object, MCProperListRef p_arguments)
{
	if (p_object == nil)
	{
        p_object = MCEngineCurrentContextObject();
        
        if (p_object == nullptr)
        {
        	return nullptr;
		}
	}
	
	MCExecContext ctxt(p_object, nil, nil);
	MCAutoCustomPointer<MCParameter, MCEngineFreeScriptParameters> t_params;
	
	if (!MCEngineConvertToScriptParameters(ctxt, p_arguments, &t_params))
        return nullptr;
	
	MCRedrawLockScreen();
	
	Exec_stat t_stat;
	t_stat = p_object -> domess(p_script,
								*t_params);
	
	MCRedrawUnlockScreen();
	
	if (t_stat == ES_ERROR)
	{
        MCEngineThrowScriptError();
        return nullptr;
	}
	
	return MCEngineEvalScriptResult(ctxt);
}

extern "C" MC_DLLEXPORT_DEF MCValueRef MCEngineExecExecuteScriptInScriptObjectWithArguments(MCStringRef p_script, MCScriptObjectRef p_object, MCProperListRef p_arguments)
{
	if (!MCEngineEnsureScriptObjectAccessIsAllowed())
		return nil;
	
	MCObject *t_object = nil;
	uint32_t t_part_id = 0;
	if (p_object != nil)
	{
		if (!MCEngineEvalObjectOfScriptObject(p_object, t_object, t_part_id))
			return nil;
	}

	return MCEngineDoExecuteScriptInObjectWithArguments(p_script, t_object, p_arguments);
}

extern "C" MC_DLLEXPORT_DEF MCValueRef MCEngineExecExecuteScriptInScriptObject(MCStringRef p_script, MCScriptObjectRef p_object)
{
	return MCEngineExecExecuteScriptInScriptObjectWithArguments(p_script, p_object, kMCEmptyProperList);
}

extern "C" MC_DLLEXPORT_DEF MCValueRef MCEngineExecExecuteScriptWithArguments(MCStringRef p_script, MCProperListRef p_arguments)
{
	return MCEngineExecExecuteScriptInScriptObjectWithArguments(p_script,
																nil,
																p_arguments);
}

extern "C" MC_DLLEXPORT_DEF MCValueRef MCEngineExecExecuteScript(MCStringRef p_script)
{
	return MCEngineExecExecuteScriptInScriptObjectWithArguments(p_script,
																nil,
																kMCEmptyProperList);
}

////////////////////////////////////////////////////////////////////////////////

static void __MCScriptObjectDestroy(MCValueRef p_value)
{
    __MCScriptObjectImpl *self;
    self = (__MCScriptObjectImpl *)MCValueGetExtraBytesPtr(p_value);
    
    self->~__MCScriptObjectImpl();
}

static bool __MCScriptObjectCopy(MCValueRef p_value, bool p_release, MCValueRef& r_copy)
{
    if (p_release)
        r_copy = p_value;
    else
        r_copy = MCValueRetain(p_value);
    
    return true;
}

static bool __MCScriptObjectEqual(MCValueRef p_left, MCValueRef p_right)
{
	__MCScriptObjectImpl *t_left, *t_right;
	t_left = (__MCScriptObjectImpl *)MCValueGetExtraBytesPtr(p_left);
	t_right = (__MCScriptObjectImpl *)MCValueGetExtraBytesPtr(p_right);
	
	return t_left->handle == t_right->handle && t_left->part_id == t_right->part_id;
}

static hash_t __MCScriptObjectHash(MCValueRef p_value)
{
	__MCScriptObjectImpl *self;
	self = (__MCScriptObjectImpl *)MCValueGetExtraBytesPtr(p_value);

	return MCHashPointer(self->handle) ^ MCHashInteger(self->part_id);
}

static bool __MCScriptObjectDescribe(MCValueRef p_value, MCStringRef& r_description)
{
    auto self =
            reinterpret_cast<__MCScriptObjectImpl *>(MCValueGetExtraBytesPtr(p_value));
    
    if (!self->handle.IsValid())
    {
        return MCStringCopy(MCSTR("<deleted script object>"),
                            r_description);
    }
    
    MCAutoValueRef t_object_name;
    if (!self->handle->names(P_LONG_NAME_NO_FILENAME,
                             &t_object_name))
    {
        return false;
    }
    
    return MCStringFormat(r_description,
                          "<script object %@>",
                          *t_object_name);
}

static MCValueCustomCallbacks kMCScriptObjectCustomValueCallbacks =
{
    false,
    __MCScriptObjectDestroy,
    __MCScriptObjectCopy,
    __MCScriptObjectEqual,
    __MCScriptObjectHash,
    __MCScriptObjectDescribe,
    nil,
    nil,
};

////////////////////////////////////////////////////////////////////////////////

static MCStringRef s_log_buffer = nil;
static bool s_log_update_pending = false;

class MCEngineLogChangedEvent: public MCCustomEvent
{
public:
    void Destroy(void)
    {
        delete this;
    }
    
    void Dispatch(void)
    {
        MCdefaultstackptr -> getcard() -> message_with_valueref_args(MCNAME("logChanged"), s_log_buffer);
        MCStringRemove(s_log_buffer, MCRangeMake(0, MCStringGetLength(s_log_buffer)));
        s_log_update_pending = false;
    }
};

extern "C" MC_DLLEXPORT_DEF void MCEngineExecLog(MCValueRef p_message)
{
	if (p_message == nil)
		p_message = kMCNull;

    MCAutoStringRef t_message_desc;
    if (!MCValueCopyDescription(p_message, &t_message_desc))
        return;

#ifdef _SERVER
	MCAutoStringRefAsSysString t_sys_string;
	t_sys_string . Lock(*t_message_desc);

	MCS_write(*t_sys_string, 1, t_sys_string . Size(), IO_stderr);
	MCS_write("\n", 1, 1, IO_stderr);

	MCS_flush(IO_stderr);

#else
    if (!MCStringIsEmpty(s_log_buffer))
    {
        if (!MCStringAppendChar(s_log_buffer, '\n'))
            return;
    }


    if (!MCStringAppend(s_log_buffer, *t_message_desc))
        return;
    
    if (s_log_update_pending)
        return;
    
    s_log_update_pending = true;
    MCEventQueuePostCustom(new MCEngineLogChangedEvent);
#endif
}

extern "C" MC_DLLEXPORT_DEF void MCEngineExecLogWithValues(MCStringRef p_message, MCProperListRef p_values)
{
    MCAutoStringRef t_formatted_message;
    if (!MCStringCreateMutable(0, &t_formatted_message))
        return;
    
    uindex_t t_value_index;
    t_value_index = 0;
    for(uindex_t i = 0; i < MCStringGetLength(p_message); i++)
    {
        if (MCStringGetCharAtIndex(p_message, i) == '%')
        {
            if (i + 1 < MCStringGetLength(p_message) &&
                MCStringGetCharAtIndex(p_message, i + 1) == '@')
            {
                i += 1;
                if (t_value_index < MCProperListGetLength(p_values))
                {
                    MCAutoStringRef t_value_as_string;
                    if (!MCValueCopyDescription(MCProperListFetchElementAtIndex(p_values, t_value_index), &t_value_as_string))
                    {
                        if (!MCStringAppendNativeChars(*t_formatted_message, (const char_t *)"<unknown>", strlen("<unknown>")))
                            return;
                    }
                    else
                    {
                        if (!MCStringAppend(*t_formatted_message, *t_value_as_string))
                            return;
                    }
                    
                    t_value_index += 1;
                }
                else
                {
                    if (!MCStringAppendNativeChars(*t_formatted_message, (const char_t *)"<overflow>", strlen("<overflow>")))
                        return;
                }
                
                continue;
            }
        }
        
        if (!MCStringAppendChar(*t_formatted_message, MCStringGetCharAtIndex(p_message, i)))
            return;
    }
    
    MCEngineExecLog(*t_formatted_message);
}

////////////////////////////////////////////////////////////////////////////////

extern "C" MC_DLLEXPORT_DEF bool MCEngineAddRunloopAction(MCRunloopActionCallback p_callback, void *p_context, MCRunloopActionRef &r_action)
{
	return MCscreen->AddRunloopAction(p_callback, p_context, r_action);
}

extern "C" MC_DLLEXPORT_DEF void MCEngineRemoveRunloopAction(MCRunloopActionRef p_action)
{
	MCscreen->RemoveRunloopAction(p_action);
}

extern "C" MC_DLLEXPORT_DEF bool MCEngineRunloopWait()
{
	MCscreen->wait(60.0, True, True);
	return true;
}

static void break_no_op(void*)
{
}

extern "C" MC_DLLEXPORT_DEF void MCEngineRunloopBreakWait()
{
#if defined(FEATURE_NOTIFY)
	// IM-2016-07-21: [[ Bug 17633 ]] Need to give notify dispatch something
	//    to do as just pinging the queue doesn't break out of the wait loop.
	MCNotifyPush(break_no_op, nil, false, true);
#endif
}

////////////////////////////////////////////////////////////////////////////////

extern MCExecContext *MCECptr;

extern "C" MC_DLLEXPORT_DEF void
MCEngineEvalTheColumnDelimiter(MCStringRef& r_del)
{
    MCStringRef t_del =
        MCECptr != nil ? MCECptr->GetColumnDelimiter() : MCSTR("\t");

    r_del = MCValueRetain(t_del);
}

extern "C" MC_DLLEXPORT_DEF void
MCEngineEvalTheRowDelimiter(MCStringRef& r_del)
{
    MCStringRef t_del =
        MCECptr != nil ? MCECptr->GetRowDelimiter() : MCSTR("\n");
    
    r_del = MCValueRetain(t_del);
}

extern "C" MC_DLLEXPORT_DEF void
MCEngineEvalTheLineDelimiter(MCStringRef& r_del)
{
    MCStringRef t_del =
            MCECptr != nil ? MCECptr->GetLineDelimiter() : MCSTR("\n");

    r_del = MCValueRetain(t_del);
}

extern "C" MC_DLLEXPORT_DEF void
MCEngineEvalTheItemDelimiter(MCStringRef& r_del)
{
    MCStringRef t_del =
            MCECptr != nil ? MCECptr->GetItemDelimiter() : MCSTR(",");
    
    r_del = MCValueRetain(t_del);
}

////////////////////////////////////////////////////////////////////////////////

extern bool MCEngineLookupResourcePathForModule(MCScriptModuleRef p_module, MCStringRef &r_resource_path);

extern "C" MC_DLLEXPORT_DEF void
MCEngineEvalMyResourcesFolder(MCStringRef& r_folder)
{
    MCScriptModuleRef t_module = MCScriptGetCurrentModule();
    if (t_module == nullptr)
    {
        r_folder = nullptr;
        return;
    }
    
    if (!MCEngineLookupResourcePathForModule(t_module,
                                             r_folder))
    {
        r_folder = nullptr;
        return;
    }
}

////////////////////////////////////////////////////////////////////////////////
    
static bool
__MCEngineDescribeScriptOfScriptObject_ConstantCallback(void *p_context,
                                                        MCHandlerConstantInfo *p_info)
{
    MCArrayRef t_array = (MCArrayRef)p_context;
    
    if (!MCArrayStoreValue(t_array,
                           false,
                           p_info->name,
                           p_info->value))
    {
        return false;
    }
    
    return true;
}

static bool
__MCEngineDescribeScriptOfScriptObject_VariableCallback(void *p_context,
                                                        MCVariable *p_variable)
{
    MCProperListRef t_list = (MCProperListRef)p_context;
    
    if (!MCProperListPushElementOntoBack(t_list,
                           p_variable->getname()))
    {
        return false;
    }
    
    return true;
}

static bool
__MCEngineDescribeScriptOfScriptObject_HandlerCallback(void *p_context,
                                                       Handler_type p_type,
                                                       MCHandler* p_handler,
                                                       bool p_include_all)
{
    
    if (!p_include_all && p_handler->isprivate())
    {
        // skip
        return true;
    }
    
    MCArrayRef t_array = (MCArrayRef)p_context;
    
    MCAutoArrayRef t_entry;
    if (!MCArrayCreateMutable(&t_entry))
    {
        return false;
    }
    
    MCValueRef t_value;
    switch(p_type)
    {
        case HT_MESSAGE:
            t_value = MCNameGetString(MCNAME("command"));
            break;
        case HT_FUNCTION:
            t_value = MCNameGetString(MCNAME("function"));
            break;
        case HT_GETPROP:
            t_value = MCNameGetString(MCNAME("getprop"));
            break;
        case HT_SETPROP:
            t_value = MCNameGetString(MCNAME("setprop"));
            break;
        case HT_BEFORE:
            t_value = MCNameGetString(MCNAME("before"));
            break;
        case HT_AFTER:
            t_value = MCNameGetString(MCNAME("after"));
            break;
        default:
            MCUnreachable();
            break;
    }
    
    if (!MCArrayStoreValue(*t_entry,
                           false,
                           MCNAME("type"),
                           t_value))
    {
        return false;
    }
    
    if (!MCArrayStoreValue(*t_entry,
                           false,
                           MCNAME("is_private"),
                           p_handler->isprivate() ? kMCTrue : kMCFalse))
    {
        return false;
    }
    
    MCAutoProperListRef t_param_names;
    if (!p_handler->getparamnames_as_properlist(&t_param_names))
    {
        return false;
    }
    
    if (!MCArrayStoreValue(*t_entry,
                           false,
                           MCNAME("parameters"),
                           *t_param_names))
    {
        return false;
    }
    
    MCAutoNumberRef t_start_line;
    if (!MCNumberCreateWithInteger(p_handler->getstartline(), &t_start_line))
    {
        return false;
    }
    
    if (!MCArrayStoreValue(*t_entry,
                           false,
                           MCNAME("start_line"),
                           *t_start_line))
    {
        return false;
    }
    
    MCAutoNumberRef t_end_line;
    if (!MCNumberCreateWithInteger(p_handler->getendline(), &t_end_line))
    {
        return false;
    }
    
    if (!MCArrayStoreValue(*t_entry,
                           false,
                           MCNAME("end_line"),
                           *t_end_line))
    {
        return false;
    }
    
    if (p_include_all)
    {
        MCAutoProperListRef t_global_names;
        if (!p_handler->getglobalnames_as_properlist(&t_global_names))
        {
            return false;
        }
        
        if (!MCArrayStoreValue(*t_entry,
                               false,
                               MCNAME("globals"),
                               *t_global_names))
        {
            return false;
        }
        
        MCAutoProperListRef t_variable_names;
        if (!p_handler->getvariablenames_as_properlist(&t_variable_names))
        {
            return false;
        }
        
        if (!MCArrayStoreValue(*t_entry,
                               false,
                               MCNAME("locals"),
                               *t_variable_names))
        {
            return false;
        }
        
        MCAutoProperListRef t_constant_names;
        if (!p_handler->getconstantnames_as_properlist(&t_constant_names))
        {
            return false;
        }
        
        if (!MCArrayStoreValue(*t_entry,
                               false,
                               MCNAME("constants"),
                               *t_constant_names))
        {
            return false;
        }
        
    }
    
    if (!MCArrayStoreValue(t_array,
                           false,
                           p_handler->getname(),
                           *t_entry))
    {
        return false;
    }
    
    return true;
}



extern "C" MC_DLLEXPORT_DEF MCArrayRef MCEngineExecDescribeScriptOfScriptObject(MCScriptObjectRef p_object, bool p_include_all)
{
    // This does not require script access.
    
    MCObject *t_object = nil;
    uint32_t t_part_id = 0;
    if (!MCEngineEvalObjectOfScriptObject(p_object, t_object, t_part_id))
    {
        return nil;
    }
    
    MCAutoArrayRef t_description;
    if (!MCArrayCreateMutable(&t_description))
    {
        return nil;
    }
    
    bool t_valid = t_object->parsescript(False);
    
    if (!MCArrayStoreValue(*t_description,
                           false,
                           MCNAME("valid"),
                           t_valid ? kMCTrue : kMCFalse))
    {
        return nil;
    }
    
    if (t_valid)
    {
        MCHandlerlist *t_hlist = t_object->gethandlers();
        
        if (p_include_all)
        {
            MCAutoArrayRef t_constants;
            if (!MCArrayCreateMutable(&t_constants) ||
                (t_hlist != nil &&
                 !t_hlist->listconstants(__MCEngineDescribeScriptOfScriptObject_ConstantCallback,
                                         *t_constants)) ||
                !t_constants.MakeImmutable() ||
                !MCArrayStoreValue(*t_description,
                                   false,
                                   MCNAME("constants"),
                                   *t_constants))
            {
                return nil;
            }
        
            MCAutoProperListRef t_variables;
            if (!MCProperListCreateMutable(&t_variables) ||
                (t_hlist != nil &&
                 !t_hlist->listvariables(__MCEngineDescribeScriptOfScriptObject_VariableCallback,
                                         *t_variables)) ||
                !t_variables.MakeImmutable() ||
                !MCArrayStoreValue(*t_description,
                                   false,
                                   MCNAME("locals"),
                                   *t_variables))
            {
                return nil;
            }

            MCAutoProperListRef t_globals;
            if (!MCProperListCreateMutable(&t_globals) ||
                (t_hlist != nil &&
                 !t_hlist->listglobals(__MCEngineDescribeScriptOfScriptObject_VariableCallback,
                                         *t_globals)) ||
                !t_globals.MakeImmutable() ||
                !MCArrayStoreValue(*t_description,
                                   false,
                                   MCNAME("globals"),
                                   *t_globals))
            {
                return nil;
            }
        }
        
        MCAutoArrayRef t_handlers;
        if (!MCArrayCreateMutable(&t_handlers) ||
            (t_hlist != nil &&
             !t_hlist->listhandlers(__MCEngineDescribeScriptOfScriptObject_HandlerCallback,
                                    *t_handlers, p_include_all)) ||
            !t_handlers.MakeImmutable() ||
            !MCArrayStoreValue(*t_description,
                               false,
                               MCNAME("handlers"),
                               *t_handlers))
        {
            return nil;
        }
    }
    
    if (!t_description.MakeImmutable())
    {
        return nil;
    }
    
    return t_description.Take();
}

////////////////////////////////////////////////////////////////////////////////

extern "C" MC_DLLEXPORT_DEF void
MCEngineEvalKeyIsDown(uint8_t p_key, bool p_event, bool& r_down)
{
    uint8_t t_modifier = 0;
    switch (p_key)
    {
        case 0:
            t_modifier = MS_SHIFT;
            break;
        case 1:
            t_modifier = MS_CONTROL;
            break;
        case 2:
            t_modifier = MS_MAC_CONTROL;
            break;
        case 3:
            t_modifier = MS_MOD1;
            break;
        case 4:
            t_modifier = MS_CAPS_LOCK;
            break;
        default:
            r_down = false;
            MCUnreachable();
            break;
    }
    
    if (p_event)
    {
        r_down = (MCmodifierstate & t_modifier) != 0;
    }
    else
    {
        r_down = (MCscreen->querymods() & t_modifier) != 0;
    }
}

////////////////////////////////////////////////////////////////////////////////


static MCStringRef
MCEngineDoResolveFilePathRelativeToStack(MCStringRef p_filepath, MCStack *p_stack)
{
    if (!MCPathIsAbsolute(p_filepath))
    {
        if (p_stack == nullptr)
        {
            MCObject *t_target = MCEngineCurrentContextObject();
            
            if (t_target == nullptr)
            {
                return nullptr;
            }
            
            p_stack = t_target->getstack();
        }
        
        // else try to resolve from stack file location
        MCAutoStringRef t_resolved;
        if (p_stack->resolve_relative_path(p_filepath, &t_resolved))
        {
            return t_resolved.Take();
        }
        
        // else try to resolve from current folder
        if (MCS_resolvepath(p_filepath, &t_resolved))
        {
            return t_resolved.Take();
        }
    }
            
    return MCValueRetain(p_filepath);;
}

extern "C" MC_DLLEXPORT_DEF MCStringRef MCEngineExecResolveFilePathRelativeToObject(MCStringRef p_filepath, MCScriptObjectRef p_object)
{
    if (!MCEngineEnsureScriptObjectAccessIsAllowed())
        return nullptr;
    
    MCStack *t_stack = nullptr;
    if (p_object != nullptr)
    {
        MCObject *t_object = nullptr;
        uint32_t t_part_id = 0;
        if (!MCEngineEvalObjectOfScriptObject(p_object, t_object, t_part_id))
            return nullptr;
        
        t_stack = t_object->getstack();
    }
    
    return MCEngineDoResolveFilePathRelativeToStack(p_filepath, t_stack);
}

extern "C" MC_DLLEXPORT_DEF MCStringRef MCEngineExecResolveFilePath(MCStringRef p_filepath)
{
    if (!MCEngineEnsureScriptObjectAccessIsAllowed())
        return nullptr;
    
    return MCEngineDoResolveFilePathRelativeToStack(p_filepath, nullptr);
}

////////////////////////////////////////////////////////////////////////////////

MC_DLLEXPORT_DEF MCTypeInfoRef kMCEngineScriptObjectDoesNotExistErrorTypeInfo = nil;
MC_DLLEXPORT_DEF MCTypeInfoRef kMCEngineScriptObjectNoContextErrorTypeInfo = nil;

////////////////////////////////////////////////////////////////////////////////

extern "C" bool com_livecode_engine_Initialize(void)
{
	if (!MCNamedErrorTypeInfoCreate(MCNAME("com.livecode.engine.ScriptObjectDoesNotExistError"), MCNAME("engine"), MCSTR("object does not exist"), kMCEngineScriptObjectDoesNotExistErrorTypeInfo))
		return false;
	
	if (!MCNamedErrorTypeInfoCreate(MCNAME("com.livecode.engine.ScriptObjectNoContextError"), MCNAME("engine"), MCSTR("script access not allowed"), kMCEngineScriptObjectNoContextErrorTypeInfo))
		return false;
	
	if (!MCNamedCustomTypeInfoCreate(MCNAME("com.livecode.engine.ScriptObject"), kMCNullTypeInfo, &kMCScriptObjectCustomValueCallbacks, kMCEngineScriptObjectTypeInfo))
		return false;
	
	if (!MCStringCreateMutable(0, s_log_buffer))
		return false;
    
    s_script_object_access_lock_count = 0;
    
    return true;
}

extern "C" void com_livecode_engine_Finalize(void)
{
    MCValueRelease(s_log_buffer);
    MCValueRelease(kMCEngineScriptObjectTypeInfo);
	MCValueRelease(kMCEngineScriptObjectDoesNotExistErrorTypeInfo);
}

////////////////////////////////////////////////////////////////////////////////

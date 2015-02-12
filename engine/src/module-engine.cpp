/* Copyright (C) 2003-2014 Runtime Revolution Ltd.
 
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

#include "dispatch.h"

#include "module-engine.h"

////////////////////////////////////////////////////////////////////////////////

typedef struct __MCScriptObject *MCScriptObjectRef;

struct __MCScriptObjectImpl
{
    MCObjectHandle *handle;
    uint32_t part_id;
};

////////////////////////////////////////////////////////////////////////////////

MCTypeInfoRef kMCEngineScriptObjectTypeInfo;

////////////////////////////////////////////////////////////////////////////////

static bool s_last_message_was_handled = false;

////////////////////////////////////////////////////////////////////////////////

bool MCScriptObjectCreate(MCObject *p_object, uint32_t p_part_id, MCScriptObjectRef& r_script_object)
{
    MCScriptObjectRef t_script_object;
    if (!MCValueCreateCustom(kMCEngineScriptObjectTypeInfo, sizeof(__MCScriptObjectImpl), t_script_object))
        return false;
    
    __MCScriptObjectImpl *t_script_object_imp;
    t_script_object_imp = (__MCScriptObjectImpl *)MCValueGetExtraBytesPtr(t_script_object);
    t_script_object_imp -> handle = p_object != nil ? p_object -> gethandle() : nil;
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

////////////////////////////////////////////////////////////////////////////////

extern "C" MC_DLLEXPORT MCScriptObjectRef MCEngineExecResolveScriptObject(MCStringRef p_object_id)
{
    MCExecContext ctxt(MCdefaultstackptr, nil, nil);
    
	// Create a script point with the value we are taking the id from.
    MCScriptPoint sp(p_object_id);
    
	// Create a new chunk object to parse the reference into.
    MCChunk *t_chunk;
    t_chunk = new MCChunk(False);
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
    if (!MCScriptObjectCreate(t_object, t_part_id, t_script_object))
        return nil;
    
    return t_script_object;
}

extern "C" MC_DLLEXPORT void MCEngineEvalScriptObjectExists(MCScriptObjectRef p_object, bool& r_exists)
{
    __MCScriptObjectImpl *t_script_object_imp;
    t_script_object_imp = (__MCScriptObjectImpl *)MCValueGetExtraBytesPtr(p_object);
    
    if (t_script_object_imp -> handle != nil)
        r_exists = t_script_object_imp -> handle -> Exists();
    else
        r_exists = false;
}

extern "C" MC_DLLEXPORT void MCEngineEvalScriptObjectDoesNotExist(MCScriptObjectRef p_object, bool& r_not_exists)
{
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
		t_literal -> type == TT_PROPERTY &&
		t_sp . next(t_type) == PS_EOF)
		return (Properties)t_literal -> which;
	
	return P_CUSTOM;
}

static inline bool MCEngineEvalObjectOfScriptObject(MCScriptObjectRef p_object, MCObject *&r_object, uint32_t &r_part_id)
{
	__MCScriptObjectImpl *t_script_object_imp;
	t_script_object_imp = (__MCScriptObjectImpl *)MCValueGetExtraBytesPtr(p_object);
	if (t_script_object_imp -> handle == nil ||
		!t_script_object_imp -> handle -> Exists())
	{
		// TODO: Throw script object doesn't exist error.
		MCErrorCreateAndThrow(kMCGenericErrorTypeInfo, "reason", MCSTR("object does not exist"), nil);
		return false;
	}
	
	r_object = t_script_object_imp->handle->Get();
	r_part_id = t_script_object_imp->part_id;
	return true;
}

extern "C" MC_DLLEXPORT MCValueRef MCEngineExecGetPropertyOfScriptObject(MCStringRef p_property, MCScriptObjectRef p_object)
{
	MCObject *t_object;
	uint32_t t_part_id;
	if (!MCEngineEvalObjectOfScriptObject(p_object, t_object, t_part_id))
		return nil;
	
	Properties t_prop;
	t_prop = parse_property_name(p_property);
    
    MCExecContext ctxt(MCdefaultstackptr, nil, nil);
	MCExecValue t_value;
    
    if (t_prop == P_CUSTOM)
    {
        MCNewAutoNameRef t_propset_name, t_propset_key;
        t_propset_name = t_object -> getdefaultpropsetname();
        if (!MCNameCreate(p_property, &t_propset_key))
            return nil;
        t_object -> getcustomprop(ctxt, *t_propset_name, *t_propset_key, t_value);
    }
    else
        t_object -> getprop(ctxt, t_part_id, t_prop, nil, False, t_value);

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

extern "C" MC_DLLEXPORT void MCEngineExecSetPropertyOfScriptObject(MCStringRef p_property, MCScriptObjectRef p_object, MCValueRef p_value)
{
	MCObject *t_object;
	uint32_t t_part_id;
	if (!MCEngineEvalObjectOfScriptObject(p_object, t_object, t_part_id))
		return;
	
    MCValueRef t_value_copy;
    t_value_copy = MCValueRetain(p_value);
    
    // Make sure the value is something the script world can understand.
    MCExecContext ctxt(MCdefaultstackptr, nil, nil);
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
        t_propset_name = t_object -> getdefaultpropsetname();
        if (!MCNameCreate(p_property, &t_propset_key))
        {
            MCValueRelease(t_value_copy);
            return;
        }
        t_object -> setcustomprop(ctxt, *t_propset_name, *t_propset_key, t_value);
    }
    else
        t_object -> setprop(ctxt, t_part_id, t_prop, nil, False, t_value);
    
    if (ctxt . HasError())
    {
        MCEngineThrowScriptError();
        return;
    }
}

extern "C" MC_DLLEXPORT void MCEngineEvalOwnerOfScriptObject(MCScriptObjectRef p_object, MCScriptObjectRef &r_owner)
{
	MCObject *t_object;
	uint32_t t_part_id;
	if (!MCEngineEvalObjectOfScriptObject(p_object, t_object, t_part_id))
		return;
	
	MCObject *t_owner;
	if (t_object->gettype() == CT_STACK && MCdispatcher->ismainstack((MCStack*)t_object))
		t_owner = nil;
	else
		t_owner = t_object->getparent();
	
	MCScriptObjectCreate(t_owner, t_part_id, r_owner);
}

struct MCScriptObjectChildControlsVisitor : public MCObjectVisitor
{
	virtual bool OnObject(MCObject *p_object)
	{
		MCAutoValueRefBase<MCScriptObjectRef> t_object_ref;
		
		return MCScriptObjectCreate(p_object, 0, &t_object_ref) && MCProperListPushElementOntoBack(m_list, *t_object_ref);
	}
	
	virtual bool OnStyledText(MCStyledText *p_text)
	{
		// don't include styled text objects
		return true;
	}
	
	virtual bool IsRecursive()
	{
		// We only want immediate children of the object
		return false;
	}
	
	virtual bool IsHeirarchical()
	{
		// stack children should be visited (cards, substacks, orphan shared groups, audioclips, videoclips) rather than all controls on stack
		return true;
	}
	
	MCProperListRef m_list;
	MCObject *m_object;
};

extern "C" MC_DLLEXPORT void MCEngineEvalChildrenOfScriptObject(MCScriptObjectRef p_object, MCProperListRef &r_controls)
{
	MCObject *t_object;
	uint32_t t_part_id;
	if (!MCEngineEvalObjectOfScriptObject(p_object, t_object, t_part_id))
		return;
	
	MCAutoProperListRef t_list;
	if (!MCProperListCreateMutable(&t_list))
		return;
	
	MCScriptObjectChildControlsVisitor t_visitor;
	t_visitor.m_list = *t_list;
	t_visitor.m_object = t_object;
	
	if (!t_object->visit_children(VISIT_STYLE_DEPTH_FIRST, t_part_id, &t_visitor))
		return;
	
	MCProperListCopy(*t_list, r_controls);
}

MCValueRef MCEngineDoDispatchToObjectWithArguments(bool p_is_function, MCStringRef p_message, MCObject *p_object, MCProperListRef p_arguments)
{
    MCNewAutoNameRef t_message_as_name;
    if (!MCNameCreate(p_message, &t_message_as_name))
        return nil;
    
    MCExecContext ctxt(MCdefaultstackptr, nil, nil);
    MCValueRef t_result;
    t_result = nil;
    
	MCParameter *t_params, *t_last_param;
	t_params = t_last_param = nil;
	for(uint32_t i = 0; i < MCProperListGetLength(p_arguments); i++)
	{
        MCValueRef t_value;
        t_value = MCValueRetain(MCProperListFetchElementAtIndex(p_arguments, i));
        
        if (!MCExtensionConvertToScriptType(ctxt, t_value))
        {
            MCValueRelease(t_value);
            goto cleanup;
        }
        
		MCParameter *t_param;
		t_param = new MCParameter;
		t_param -> setvalueref_argument(t_value);
        
		if (t_last_param == nil)
			t_params = t_param;
		else
			t_last_param -> setnext(t_param);
        
		t_last_param = t_param;
	}
    
    Exec_stat t_stat;
    t_stat = p_object -> dispatch(!p_is_function ? HT_MESSAGE : HT_FUNCTION, *t_message_as_name, t_params);
    if (t_stat == ES_ERROR)
    {
        MCEngineThrowScriptError();
        goto cleanup;
    }
    
    if (t_stat == ES_NORMAL)
        s_last_message_was_handled = true;
    else
        s_last_message_was_handled = false;
    
    t_result = MCValueRetain(MCresult -> getvalueref());
    
cleanup:
	while(t_params != nil)
	{
		MCParameter *t_param;
		t_param = t_params;
		t_params = t_params -> getnext();
		delete t_param;
	}
    
    return t_result;
}

extern "C" MC_DLLEXPORT MCValueRef MCEngineExecDispatchToScriptObjectWithArguments(bool p_is_function, MCStringRef p_message, MCScriptObjectRef p_object, MCProperListRef p_arguments)
{
	MCObject *t_object;
	uint32_t t_part_id;
	if (!MCEngineEvalObjectOfScriptObject(p_object, t_object, t_part_id))
		return nil;
	
    return MCEngineDoDispatchToObjectWithArguments(p_is_function, p_message, t_object, p_arguments);
}

extern "C" MC_DLLEXPORT MCValueRef MCEngineExecDispatchToScriptObject(bool p_is_function, MCStringRef p_message, MCScriptObjectRef p_object)
{
    return MCEngineExecDispatchToScriptObjectWithArguments(p_is_function, p_message, p_object, kMCEmptyProperList);
}

extern "C" MC_DLLEXPORT void MCEngineEvalMessageWasHandled(bool& r_handled)
{
    r_handled = s_last_message_was_handled;
}

extern "C" MC_DLLEXPORT void MCEngineEvalMessageWasNotHandled(bool& r_not_handled)
{
    bool t_handled;
    MCEngineEvalMessageWasHandled(t_handled);
    r_not_handled = !t_handled;
}

////////////////////////////////////////////////////////////////////////////////

extern "C" MC_DLLEXPORT MCValueRef MCEngineExecExecuteScript(MCStringRef p_script)
{
    MCStack *t_stack;
    t_stack = MCdefaultstackptr;
    if (t_stack == nil)
    {
        MCErrorCreateAndThrow(kMCGenericErrorTypeInfo, "reason", MCSTR("no default stack"), nil);
        return nil;
    }
    
    MCRedrawLockScreen();
    
    Exec_stat t_stat;
    t_stat = t_stack -> getcurcard() -> domess(p_script);
    
    MCRedrawUnlockScreen();
    
    if (t_stat == ES_ERROR)
    {
        MCEngineThrowScriptError();
        return nil;
    }
    
    return MCValueRetain(MCresult -> getvalueref());
}

////////////////////////////////////////////////////////////////////////////////

static void __MCScriptObjectDestroy(MCValueRef p_value)
{
    __MCScriptObjectImpl *self;
    self = (__MCScriptObjectImpl *)MCValueGetExtraBytesPtr(p_value);
    
    if (self -> handle != nil)
        self -> handle -> Release();
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
    return false;
}

static MCValueCustomCallbacks kMCScriptObjectCustomValueCallbacks =
{
    false,
    __MCScriptObjectDestroy,
    __MCScriptObjectCopy,
    __MCScriptObjectEqual,
    __MCScriptObjectHash,
    __MCScriptObjectDescribe,
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

extern "C" MC_DLLEXPORT void MCEngineExecLog(MCStringRef p_message)
{
    if (!MCStringIsEmpty(s_log_buffer))
    {
        if (!MCStringAppendChar(s_log_buffer, '\n'))
            return;
    }
    
    if (!MCStringAppend(s_log_buffer, p_message))
        return;
    
    if (s_log_update_pending)
        return;
    
    s_log_update_pending = true;
    MCEventQueuePostCustom(new MCEngineLogChangedEvent);
}

extern "C" MC_DLLEXPORT void MCEngineExecLogWithValues(MCStringRef p_message, MCProperListRef p_values)
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

static void __create_named_custom_typeinfo(MCTypeInfoRef p_base, const MCValueCustomCallbacks *p_callbacks, MCNameRef p_name, MCTypeInfoRef& r_typeinfo)
{
    MCAutoTypeInfoRef t_unnamed;
    /* UNCHECKED */ MCCustomTypeInfoCreate(p_base, p_callbacks, &t_unnamed);
    /* UNCHECKED */ MCNamedTypeInfoCreate(p_name, r_typeinfo);
    /* UNCHECKED */ MCNamedTypeInfoBind(r_typeinfo, *t_unnamed);
}

bool MCEngineModuleInitialize(void)
{
	/* UNCHECKED */ __create_named_custom_typeinfo(kMCNullTypeInfo, &kMCScriptObjectCustomValueCallbacks, MCNAME("com.livecode.engine.ScriptObject"), kMCEngineScriptObjectTypeInfo);
    
    /* UNCHECKED */ MCStringCreateMutable(0, s_log_buffer);
    
    return true;
}

void MCEngineModuleFinalize(void)
{
    MCValueRelease(s_log_buffer);
    MCValueRelease(kMCEngineScriptObjectTypeInfo);
}

////////////////////////////////////////////////////////////////////////////////

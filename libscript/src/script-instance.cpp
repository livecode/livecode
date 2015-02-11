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

#include "script.h"
#include "script-private.h"

#include "ffi.h"

#include "foundation-auto.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

////////////////////////////////////////////////////////////////////////////////

// This context struct, callbacks and associated functions provide an implementation
// of MCHandler for handlers coming from libscript. It enables an MCHandlerRef
// created from within LCB to be used 'out-of-frame' - i.e. not just from direct
// invocation within the LCB VM.

struct __MCScriptHandlerContext
{
    MCScriptInstanceRef instance;
    MCScriptCommonHandlerDefinition *definition;
};

extern MCHandlerCallbacks __kMCScriptHandlerCallbacks;
bool __MCScriptHandlerInvoke(void *context, MCValueRef *p_arguments, uindex_t p_argument_count, MCValueRef& r_value);
void __MCScriptHandlerRelease(void *context);
bool __MCScriptHandlerDescribe(void *context, MCStringRef &r_desc);

////////////////////////////////////////////////////////////////////////////////

// This is the module of the most recent LCB stack frame on the current thread's
// stack. It is set before and after a foreign handler call so that the native
// code can get some element of context.
static MCScriptModuleRef s_current_module = nil;

////////////////////////////////////////////////////////////////////////////////

bool MCScriptCreateInstanceOfModule(MCScriptModuleRef p_module, MCScriptInstanceRef& r_instance)
{
    bool t_success;
    t_success = true;
    
    MCScriptInstanceRef t_instance;
    t_instance = nil;
    
    // If the module is not usable, then we cannot create an instance.
    if (t_success)
        if (!p_module -> is_usable)
            return false;
    
    // If this is a module which shares a single instance, then return that if we have
    // one.
    if (p_module -> module_kind != kMCScriptModuleKindWidget &&
        p_module -> shared_instance != nil)
    {
        r_instance = MCScriptRetainInstance(p_module -> shared_instance);
        return true;
    }
    
    // Attempt to create a script object.
    if (t_success)
        t_success = MCScriptCreateObject(kMCScriptObjectKindInstance, sizeof(MCScriptInstance), (MCScriptObject*&)t_instance);

    // Now associate the script object with the module (so the 'slots' field make sense).
    if (t_success)
    {
        t_instance -> module = MCScriptRetainModule(p_module);
    }
    
    // Now allocate the slots field.
    if (t_success)
        t_success = MCMemoryNewArray(p_module -> slot_count, t_instance -> slots);
    
    if (t_success)
    {
        for(uindex_t i = 0; i < p_module -> slot_count; i++)
            t_instance -> slots[i] = MCValueRetain(kMCNull);

        // If this is a module which shares its instance, then add a link to it.
        // (Note this is weak reference - we don't retain, otherwise we would have
        //  a cycle!).
        if (p_module -> module_kind != kMCScriptModuleKindWidget)
            p_module -> shared_instance = t_instance;
        
        r_instance = t_instance;
    }
    else
        MCScriptDestroyObject(t_instance);
    
    return t_success;
}

void MCScriptDestroyInstance(MCScriptInstanceRef self)
{
    __MCScriptValidateObjectAndKind__(self, kMCScriptObjectKindInstance);
    
    // If the object had slots initialized, then free them.
    if (self -> slots != nil)
    {
        for(uindex_t i = 0; i < self -> module -> slot_count; i++)
            MCValueRelease(self -> slots[i]);
        MCMemoryDeleteArray(self -> slots);
    }
    
    // If the instance has a module, and this is the shared instance then set
    // the shared instance field to nil.
    if (self -> module != nil &&
        self -> module -> shared_instance == self)
        self -> module -> shared_instance = nil;
    
    // If the instance was associated with its module, then release it.
    if (self -> module != nil)
        MCScriptReleaseModule(self -> module);
    
    // The rest of the deallocation is handled by MCScriptDestroyObject.
}

////////////////////////////////////////////////////////////////////////////////

MCScriptInstanceRef MCScriptRetainInstance(MCScriptInstanceRef self)
{
    __MCScriptValidateObjectAndKind__(self, kMCScriptObjectKindInstance);
    
    return (MCScriptInstanceRef)MCScriptRetainObject(self);
}

void MCScriptReleaseInstance(MCScriptInstanceRef self)
{
	if (nil == self)
		return;

    __MCScriptValidateObjectAndKind__(self, kMCScriptObjectKindInstance);
    
    MCScriptReleaseObject(self);
}

////////////////////////////////////////////////////////////////////////////////

MCScriptModuleRef MCScriptGetModuleOfInstance(MCScriptInstanceRef self)
{
    return self -> module;
}

////////////////////////////////////////////////////////////////////////////////

bool MCHandlerTypeInfoConformsToPropertyGetter(MCTypeInfoRef typeinfo)
{
    return MCHandlerTypeInfoGetParameterCount(typeinfo) == 0 &&
            MCHandlerTypeInfoGetReturnType(typeinfo) != kMCNullTypeInfo;
}

bool MCHandlerTypeInfoConformsToPropertySetter(MCTypeInfoRef typeinfo)
{
    return MCHandlerTypeInfoGetParameterCount(typeinfo) == 1 &&
            MCHandlerTypeInfoGetParameterMode(typeinfo, 0) == kMCHandlerTypeFieldModeIn &&
            MCHandlerTypeInfoGetReturnType(typeinfo) == kMCNullTypeInfo;
}

///////////

bool MCScriptThrowAttemptToSetReadOnlyPropertyError(MCScriptModuleRef p_module, MCNameRef p_property)
{
    return MCErrorCreateAndThrow(kMCScriptCannotSetReadOnlyPropertyErrorTypeInfo, "module", p_module -> name, "property", p_property, nil);
}

bool MCScriptThrowInvalidValueForPropertyError(MCScriptModuleRef p_module, MCNameRef p_property, MCTypeInfoRef p_expected_type, MCValueRef p_value)
{
    return MCErrorCreateAndThrow(kMCScriptInvalidPropertyValueErrorTypeInfo, "module", p_module -> name, "property", p_property, "type", MCNamedTypeInfoGetName(p_expected_type), "value", p_value, nil);
}

bool MCScriptThrowInvalidValueForResultError(MCScriptModuleRef p_module, MCScriptDefinition *p_handler, MCTypeInfoRef p_expected_type, MCValueRef p_value)
{
    return MCErrorCreateAndThrow(kMCScriptInvalidReturnValueErrorTypeInfo, "module", p_module -> name, "handler", MCScriptGetNameOfDefinitionInModule(p_module, p_handler), "type", MCNamedTypeInfoGetName(p_expected_type), "value", p_value, nil);
}

bool MCScriptThrowInParameterNotDefinedError(MCScriptModuleRef p_module, MCScriptDefinition *p_handler, uindex_t p_index)
{
    return MCErrorCreateAndThrow(kMCScriptInParameterNotDefinedErrorTypeInfo, "module", p_module -> name, "handler", MCScriptGetNameOfDefinitionInModule(p_module, p_handler), "parameter", MCScriptGetNameOfParameterInModule(p_module, p_handler, p_index), nil);
}

bool MCScriptThrowOutParameterNotDefinedError(MCScriptModuleRef p_module, MCScriptDefinition *p_handler, uindex_t p_index)
{
    return MCErrorCreateAndThrow(kMCScriptOutParameterNotDefinedErrorTypeInfo, "module", p_module -> name, "handler", MCScriptGetNameOfDefinitionInModule(p_module, p_handler), "parameter", MCScriptGetNameOfParameterInModule(p_module, p_handler, p_index), nil);
}

bool MCScriptThrowLocalVariableUsedBeforeDefinedError(MCScriptModuleRef p_module, MCScriptDefinition *p_handler, uindex_t p_index)
{
    return MCErrorCreateAndThrow(kMCScriptVariableUsedBeforeDefinedErrorTypeInfo, "module", p_module -> name, "handler", MCScriptGetNameOfDefinitionInModule(p_module, p_handler), "variable", MCScriptGetNameOfLocalVariableInModule(p_module, p_handler, p_index), nil);
}

bool MCScriptThrowGlobalVariableUsedBeforeDefinedError(MCScriptModuleRef p_module, uindex_t p_index)
{
    return MCErrorCreateAndThrow(kMCScriptVariableUsedBeforeDefinedErrorTypeInfo, "module", p_module -> name, "variable", MCScriptGetNameOfGlobalVariableInModule(p_module, p_index), nil);
}

bool MCScriptThrowInvalidValueForLocalVariableError(MCScriptModuleRef p_module, MCScriptDefinition *p_handler, uindex_t p_index, MCTypeInfoRef p_expected_type, MCValueRef p_value)
{
    return MCErrorCreateAndThrow(kMCScriptInvalidVariableValueErrorTypeInfo, "module", p_module -> name, "handler", MCScriptGetNameOfDefinitionInModule(p_module, p_handler), "variable", MCScriptGetNameOfLocalVariableInModule(p_module, p_handler, p_index), "type", MCNamedTypeInfoGetName(p_expected_type), "value", p_value, nil);
}

bool MCScriptThrowInvalidValueForGlobalVariableError(MCScriptModuleRef p_module, uindex_t p_index, MCTypeInfoRef p_expected_type, MCValueRef p_value)
{
    return MCErrorCreateAndThrow(kMCScriptInvalidVariableValueErrorTypeInfo, "module", p_module -> name, "variable",  MCScriptGetNameOfGlobalVariableInModule(p_module, p_index), "type", MCNamedTypeInfoGetName(p_expected_type), "value", p_value, nil);
}

bool MCScriptThrowNotABooleanError(MCValueRef p_value)
{
    return MCErrorCreateAndThrow(kMCScriptNotABooleanValueErrorTypeInfo, "value", p_value, nil);
}

bool MCScriptThrowWrongNumberOfArgumentsForInvokeError(MCScriptModuleRef p_module, MCScriptDefinition *p_definition, uindex_t p_provided)
{
    // TODO: Encode provided / expected.
    return MCErrorCreateAndThrow(kMCScriptWrongNumberOfArgumentsErrorTypeInfo, "module", p_module -> name, "handler", MCScriptGetNameOfDefinitionInModule(p_module, p_definition), nil);
}

bool MCScriptThrowInvalidValueForArgumentError(MCScriptModuleRef p_module, MCScriptDefinition *p_definition, uindex_t p_arg_index, MCTypeInfoRef p_expected_type, MCValueRef p_value)
{
    return MCErrorCreateAndThrow(kMCScriptInvalidArgumentValueErrorTypeInfo, "module", p_module -> name, "handler", MCScriptGetNameOfDefinitionInModule(p_module, p_definition), "parameter", MCScriptGetNameOfParameterInModule(p_module, p_definition, p_arg_index), "type", MCNamedTypeInfoGetName(p_expected_type), "value", p_value, nil);
}

bool MCScriptThrowUnableToResolveForeignHandlerError(MCScriptModuleRef p_module, MCScriptDefinition *p_definition)
{
    return MCErrorCreateAndThrow(kMCScriptForeignHandlerBindingErrorTypeInfo, "module", p_module -> name, "handler", MCScriptGetNameOfDefinitionInModule(p_module, p_definition), nil);
}

bool MCScriptThrowUnableToResolveTypeError(MCTypeInfoRef p_type)
{
    return MCErrorCreateAndThrow(kMCScriptTypeBindingErrorTypeInfo, "type", MCNamedTypeInfoGetName(p_type), nil);
}

bool MCScriptThrowUnableToResolveMultiInvoke(MCScriptModuleRef p_module, MCScriptDefinition *p_definition, MCProperListRef p_arguments)
{
    MCAutoListRef t_handlers;
    if (!MCListCreateMutable(',', &t_handlers))
        return false;
    
    MCScriptDefinitionGroupDefinition *t_group;
    t_group = static_cast<MCScriptDefinitionGroupDefinition *>(p_definition);
    for(uindex_t i = 0; i < t_group -> handler_count; i++)
    {
        MCNameRef t_name;
        t_name = MCScriptGetNameOfDefinitionInModule(p_module, p_module -> definitions[t_group -> handlers[i]]);
        if (!MCListAppend(*t_handlers, t_name))
            return false;
    }
    
    MCAutoListRef t_types;
    if (!MCListCreateMutable(',', &t_types))
        return false;
    
    for(uindex_t i = 0; i < MCProperListGetLength(p_arguments); i++)
        if (!MCListAppend(*t_types, MCNamedTypeInfoGetName(MCValueGetTypeInfo(MCProperListFetchElementAtIndex(p_arguments, i)))))
            return false;
    
    MCAutoStringRef t_handler_list, t_type_list;
    if (!MCListCopyAsString(*t_handlers, &t_handler_list) ||
        !MCListCopyAsString(*t_types, &t_type_list))
        return false;
    
    return MCErrorCreateAndThrow(kMCScriptNoMatchingHandlerErrorTypeInfo, "handlers", *t_handler_list, "types", *t_type_list, nil);
}

bool MCScriptThrowNotAHandlerValueError(MCValueRef p_value)
{
    return MCErrorCreateAndThrow(kMCScriptNotAHandlerValueErrorTypeInfo, "value", p_value, nil);
}

///////////

MCScriptVariableDefinition *MCScriptDefinitionAsVariable(MCScriptDefinition *self)
{
    __MCScriptAssert__(self -> kind == kMCScriptDefinitionKindVariable,
                            "definition not a variable");
    return static_cast<MCScriptVariableDefinition *>(self);
}

MCScriptHandlerDefinition *MCScriptDefinitionAsHandler(MCScriptDefinition *self)
{
    __MCScriptAssert__(self -> kind == kMCScriptDefinitionKindHandler,
                       "definition not a handler");
    return static_cast<MCScriptHandlerDefinition *>(self);
}

MCScriptForeignHandlerDefinition *MCScriptDefinitionAsForeignHandler(MCScriptDefinition *self)
{
    __MCScriptAssert__(self -> kind == kMCScriptDefinitionKindForeignHandler,
                       "definition not a foreign handler");
    return static_cast<MCScriptForeignHandlerDefinition *>(self);
}

MCScriptDefinitionGroupDefinition *MCScriptDefinitionAsDefinitionGroup(MCScriptDefinition *self)
{
    __MCScriptAssert__(self -> kind == kMCScriptDefinitionKindDefinitionGroup,
                       "definition not a definition group");
    return static_cast<MCScriptDefinitionGroupDefinition *>(self);
}

///////////

bool MCScriptGetPropertyOfInstance(MCScriptInstanceRef self, MCNameRef p_property, MCValueRef& r_value)
{
    __MCScriptValidateObjectAndKind__(self, kMCScriptObjectKindInstance);
    
    // Lookup the definition (throws if not found).
    MCScriptPropertyDefinition *t_definition;
    if (!MCScriptLookupPropertyDefinitionInModule(self -> module, p_property, t_definition))
        return false;
    
    MCScriptDefinition *t_getter;
    t_getter = t_definition -> getter != 0 ? self -> module -> definitions[t_definition -> getter - 1] : nil;
    
    /* LOAD CHECK */ __MCScriptAssert__(t_getter != nil,
                                            "property has no getter");
    /* LOAD CHECK */ __MCScriptAssert__(t_getter -> kind == kMCScriptDefinitionKindVariable ||
                                            t_getter -> kind == kMCScriptDefinitionKindHandler,
                                            "property getter is not a variable or handler");
    
    if (t_getter -> kind == kMCScriptDefinitionKindVariable)
    {
        // The easy case - fetching a variable-based property.
        
        MCScriptVariableDefinition *t_variable_def;
        t_variable_def = MCScriptDefinitionAsVariable(t_getter);
        
        // Variables are backed by an slot in the instance.
        uindex_t t_slot_index;
        t_slot_index = t_variable_def -> slot_index;
        
        /* COMPUTE CHECK */ __MCScriptAssert__(t_slot_index < self -> module -> slot_count,
                                               "computed variable slot out of range");
        
        // Slot based properties are easy, we just copy the value out of the slot.
        r_value = MCValueRetain(self -> slots[t_slot_index]);
    }
    else if (t_getter -> kind == kMCScriptDefinitionKindHandler)
    {
        // The more difficult case - we have to execute a handler.
        
        MCScriptHandlerDefinition *t_handler_def;
        t_handler_def = MCScriptDefinitionAsHandler(t_getter);
        
        /* LOAD CHECK */ __MCScriptAssert__(MCHandlerTypeInfoConformsToPropertyGetter(self -> module -> types[t_handler_def -> type] -> typeinfo),
                                            "incorrect signature for property getter");
    
        if (!MCScriptCallHandlerOfInstanceInternal(self, t_handler_def, nil, 0, r_value))
            return false;
    }
    else
    {
        __MCScriptUnreachable__("inconsistency with definition kind in property fetching");
    }
    
    return true;
}

bool MCScriptSetPropertyOfInstance(MCScriptInstanceRef self, MCNameRef p_property, MCValueRef p_value)
{
    __MCScriptValidateObjectAndKind__(self, kMCScriptObjectKindInstance);
    
    // Lookup the definition (throws if not found).
    MCScriptPropertyDefinition *t_definition;
    if (!MCScriptLookupPropertyDefinitionInModule(self -> module, p_property, t_definition))
        return false;
    
    MCScriptDefinition *t_setter;
    t_setter = t_definition -> setter != 0 ? self -> module -> definitions[t_definition -> setter - 1] : nil;
    
    // If there is no setter for the property then this is an error.
    if (t_definition -> setter == nil)
        return MCScriptThrowAttemptToSetReadOnlyPropertyError(self -> module, p_property);
    
    /* LOAD CHECK */ __MCScriptAssert__(t_setter != nil,
                                        "property has no setter");
    /* LOAD CHECK */ __MCScriptAssert__(t_setter -> kind == kMCScriptDefinitionKindVariable ||
                                        t_setter -> kind == kMCScriptDefinitionKindHandler,
                                        "property setter is not a variable or handler");
    
    if (t_setter -> kind == kMCScriptDefinitionKindVariable)
    {
        // The easy case - storing a variable-based property.
        
        MCScriptVariableDefinition *t_variable_def;
        t_variable_def = MCScriptDefinitionAsVariable(t_setter);
        
        MCTypeInfoRef t_type;
        t_type = self -> module -> types[t_variable_def -> type] -> typeinfo;
        
        // Make sure the value is of the correct type - if not it is an error.
        // (The caller has to ensure things are converted as appropriate).
        if (!MCTypeInfoConforms(MCValueGetTypeInfo(p_value), t_type))
            return MCScriptThrowInvalidValueForPropertyError(self -> module, p_property, t_type, p_value);
        
        // Variables are backed by an slot in the instance.
        uindex_t t_slot_index;
        t_slot_index = t_variable_def -> slot_index;
        
        /* COMPUTE CHECK */ __MCScriptAssert__(t_slot_index < self -> module -> slot_count,
                                               "computed variable slot out of range");
        
        // If the value of the slot isn't changing, assign our new value.
        if (p_value != self -> slots[t_slot_index])
        {
            MCValueRelease(self -> slots[t_slot_index]);
            self -> slots[t_slot_index] = MCValueRetain(p_value);
        }
    }
    else if (t_setter -> kind == kMCScriptDefinitionKindHandler)
    {
        // The more difficult case - we have to execute a handler.
        
        MCScriptHandlerDefinition *t_handler_def;
        t_handler_def = MCScriptDefinitionAsHandler(t_setter);
        
        /* LOAD CHECK */ __MCScriptAssert__(MCHandlerTypeInfoConformsToPropertySetter(self -> module -> types[t_handler_def -> type] -> typeinfo),
                                            "incorrect signature for property setter");
        
        // Get the required type of the parameter.
        MCTypeInfoRef t_property_type;
        t_property_type = MCHandlerTypeInfoGetParameterType(self -> module -> types[t_handler_def -> type] -> typeinfo, 0);
        
        // Make sure the value if of the correct type - if not it is an error.
        // (The caller has to ensure things are converted as appropriate).
        if (!MCTypeInfoConforms(MCValueGetTypeInfo(p_value), t_property_type))
            return MCScriptThrowInvalidValueForPropertyError(self -> module, p_property, t_property_type, p_value);
        
        MCValueRef t_result;
        if (!MCScriptCallHandlerOfInstanceInternal(self, t_handler_def, &p_value, 1, t_result))
            return false;
        
        MCValueRelease(t_result);
    }
    else
    {
        __MCScriptUnreachable__("inconsistency with definition kind in property fetching");
    }
    
    
    return true;
}

static bool MCScriptCallHandlerOfInstanceDirect(MCScriptInstanceRef self, MCScriptHandlerDefinition *p_handler, MCValueRef *p_arguments, uindex_t p_argument_count, MCValueRef& r_value)
{
    // Get the signature of the handler.
    MCTypeInfoRef t_signature;
    t_signature = self -> module -> types[p_handler -> type] -> typeinfo;
    
    // Check the number of arguments.
    uindex_t t_required_param_count;
    t_required_param_count = MCHandlerTypeInfoGetParameterCount(t_signature);
    if (t_required_param_count != p_argument_count)
        return MCScriptThrowWrongNumberOfArgumentsForInvokeError(self -> module, p_handler, p_argument_count);
    
    // Check the types of the arguments.
    for(uindex_t i = 0; i < t_required_param_count; i++)
    {
        MCHandlerTypeFieldMode t_mode;
        t_mode = MCHandlerTypeInfoGetParameterMode(t_signature, i);
        
        if (t_mode != kMCHandlerTypeFieldModeOut)
        {
            if (p_arguments[i] == nil)
                return MCScriptThrowInParameterNotDefinedError(self -> module, p_handler, i);
            
            MCTypeInfoRef t_type;
            t_type = MCHandlerTypeInfoGetParameterType(t_signature, i);
            
            if (!MCTypeInfoConforms(MCValueGetTypeInfo(p_arguments[i]), t_type))
                return MCScriptThrowInvalidValueForArgumentError(self -> module, p_handler, i, t_type, p_arguments[i]);
        }
    }
    
    // Now the input argument array is appropriate, we can just call the handler.
    if (!MCScriptCallHandlerOfInstanceInternal(self, p_handler, p_arguments, p_argument_count, r_value))
        return false;
    
    return true;
}

bool MCScriptCallHandlerOfInstance(MCScriptInstanceRef self, MCNameRef p_handler, MCValueRef *p_arguments, uindex_t p_argument_count, MCValueRef& r_value)
{
    __MCScriptValidateObjectAndKind__(self, kMCScriptObjectKindInstance);
    
    // Lookup the definition (throws if not found).
    MCScriptHandlerDefinition *t_definition;
    if (!MCScriptLookupHandlerDefinitionInModule(self -> module, p_handler, t_definition))
        return MCErrorThrowGeneric(MCSTR("handler not found"));
    
    return MCScriptCallHandlerOfInstanceDirect(self, t_definition, p_arguments, p_argument_count, r_value);
}


bool MCScriptCallHandlerOfInstanceIfFound(MCScriptInstanceRef self, MCNameRef p_handler, MCValueRef *p_arguments, uindex_t p_argument_count, MCValueRef& r_value)
{
    __MCScriptValidateObjectAndKind__(self, kMCScriptObjectKindInstance);
    
    // Lookup the definition (throws if not found).
    MCScriptHandlerDefinition *t_definition;
    if (!MCScriptLookupHandlerDefinitionInModule(self -> module, p_handler, t_definition))
    {
        r_value = MCValueRetain(kMCNull);
        return true;
    }
    
    return MCScriptCallHandlerOfInstanceDirect(self, t_definition, p_arguments, p_argument_count, r_value);
}

////////////////////////////////////////////////////////////////////////////////

// This structure is a single frame on the execution stack.
struct MCScriptFrame
{
    // The calling frame.
    MCScriptFrame *caller;
    
    // The instance we are executing within.
    MCScriptInstanceRef instance;
    
    // The handler of the instance's module being run right now.
    MCScriptHandlerDefinition *handler;
    
    // The address (instruction pointer) into the instance's module bytecode.
    uindex_t address;
    
    // The slots for the current handler invocation. The slots are in this order:
    //   <parameters>
    //   <locals>
    //   <registers>
    MCValueRef *slots;
	
    // The result register in the caller.
    uindex_t result;
    
	// The mapping array - this lists the mapping from parameters to registers
	// in the callers frame, for handling inout/out parameters.
	uindex_t *mapping;
};

static inline bool MCScriptIsRegisterValidInFrame(MCScriptFrame *p_frame, int p_register)
{
    return p_register >= 0 && p_register < p_frame -> handler -> slot_count;
}

static inline bool MCScriptIsConstantValidInFrame(MCScriptFrame *p_frame, int p_constant)
{
    return p_constant >= 0 && p_constant < p_frame -> instance -> module -> value_count;
}

static bool MCScriptCreateFrame(MCScriptFrame *p_caller, MCScriptInstanceRef p_instance, MCScriptHandlerDefinition *p_handler, MCScriptFrame*& r_frame)
{
    MCScriptFrame *self;
    self = nil;
    if (!MCMemoryNew(self) ||
        !MCMemoryNewArray(p_handler -> slot_count, self -> slots))
    {
        MCMemoryDelete(self);
        return false;
    }
    
    for(uindex_t i = 0; i < p_handler -> slot_count; i++)
        self -> slots[i] = MCValueRetain(kMCNull);
    
    self -> caller = p_caller;
    self -> instance = MCScriptRetainInstance(p_instance);
    self -> handler = p_handler;
    self -> address = p_handler -> start_address;
    
    r_frame = self;
    
    return true;
}

static MCScriptFrame *MCScriptDestroyFrame(MCScriptFrame *self)
{
    MCScriptFrame *t_caller;
    t_caller = self -> caller;
    
    for(int i = 0; i < self -> handler -> slot_count; i++)
        MCValueRelease(self -> slots[i]);
    
    MCScriptReleaseInstance(self -> instance);
    MCMemoryDeleteArray(self -> slots);
    MCMemoryDeleteArray(self -> mapping);
    MCMemoryDelete(self);
    
    return t_caller;
}

static inline void MCScriptBytecodeDecodeOp(byte_t*& x_bytecode_ptr, MCScriptBytecodeOp& r_op, uindex_t& r_arity)
{
    byte_t t_op_byte;
	t_op_byte = *x_bytecode_ptr++;
	
	// The lower nibble is the bytecode operation.
	MCScriptBytecodeOp t_op;
	t_op = (MCScriptBytecodeOp)(t_op_byte & 0xf);
	
	// The upper nibble is the arity.
	uindex_t t_arity;
	t_arity = (t_op_byte >> 4);
	
	// If the arity is 15, then overflow to a subsequent byte.
	if (t_arity == 15)
		t_arity += *x_bytecode_ptr++;
    
    r_op = t_op;
    r_arity = t_arity;
}

// TODO: Make this better for negative numbers.
static inline uindex_t MCScriptBytecodeDecodeArgument(byte_t*& x_bytecode_ptr)
{
    uindex_t t_value;
    t_value = 0;
    int t_shift;
    t_shift = 0;
    for(;;)
    {
        byte_t t_next;
        t_next = *x_bytecode_ptr++;
        t_value |= (t_next & 0x7f) << t_shift;
        if ((t_next & 0x80) == 0)
            break;
        t_shift += 7;
    }
    return t_value;
}

static inline void MCScriptResolveDefinitionInFrame(MCScriptFrame *p_frame, uindex_t p_index, MCScriptInstanceRef& r_instance, MCScriptDefinition*& r_definition)
{
    MCScriptInstanceRef t_instance;
    t_instance = p_frame -> instance;
    
    MCScriptDefinition *t_definition;
    t_definition = t_instance -> module -> definitions[p_index];
    
    if (t_definition -> kind == kMCScriptDefinitionKindExternal)
    {
        MCScriptExternalDefinition *t_ext_def;
        t_ext_def = static_cast<MCScriptExternalDefinition *>(t_definition);
        
        MCScriptImportedDefinition *t_import_def;
        t_import_def = &t_instance -> module -> imported_definitions[t_ext_def -> index];
        
        t_instance = t_import_def -> resolved_module -> shared_instance;
        t_definition = t_import_def -> resolved_definition;
    }

    r_instance = t_instance;
    r_definition = t_definition;
}

static inline MCTypeInfoRef MCScriptGetRegisterTypeInFrame(MCScriptFrame *p_frame, int p_register)
{
    /* LOAD CHECK */ __MCScriptAssert__(MCScriptIsRegisterValidInFrame(p_frame, p_register),
                                        "register out of range on fetch register type");
    
    MCTypeInfoRef t_handler_type;
    t_handler_type = p_frame -> instance -> module -> types[p_frame -> handler -> type] -> typeinfo;
    
    uindex_t t_parameter_count;
    t_parameter_count = MCHandlerTypeInfoGetParameterCount(t_handler_type);
    
    if (p_register < t_parameter_count)
        return MCHandlerTypeInfoGetParameterType(t_handler_type, p_register);
    
    if (p_register < t_parameter_count + p_frame -> handler -> local_type_count)
        return p_frame -> instance -> module -> types[p_frame -> handler -> local_types[p_register - t_parameter_count]] -> typeinfo;
    
    return nil;
}

static bool MCScriptGetRegisterTypeIsOptionalInFrame(MCScriptFrame *p_frame, int p_register)
{
    /* LOAD CHECK */ __MCScriptAssert__(MCScriptIsRegisterValidInFrame(p_frame, p_register),
                                        "register out of range on fetch register type is optional");
    
    MCTypeInfoRef t_type;
    t_type = MCScriptGetRegisterTypeInFrame(p_frame, p_register);
    
    if (t_type == nil)
        return true;
    
    MCResolvedTypeInfo t_resolved_type;
    if (!MCTypeInfoResolve(t_type, t_resolved_type))
        return true; /* RESOLVE UNCHECKED */
    
    return t_resolved_type . is_optional;
}

static inline MCValueRef MCScriptFetchFromRegisterInFrame(MCScriptFrame *p_frame, int p_register)
{
    /* LOAD CHECK */ __MCScriptAssert__(MCScriptIsRegisterValidInFrame(p_frame, p_register),
                                        "register out of range on fetch");
    
    return p_frame -> slots[p_register];
}

static inline void MCScriptStoreToRegisterInFrameAndRelease(MCScriptFrame *p_frame, int p_register, MCValueRef p_value)
{
    /* LOAD CHECK */ __MCScriptAssert__(MCScriptIsRegisterValidInFrame(p_frame, p_register),
                                        "register out of range on store");
    
    if (p_frame -> slots[p_register] != p_value)
    {
        MCValueRelease(p_frame -> slots[p_register]);
        p_frame -> slots[p_register] = p_value;
    }
}

static inline void MCScriptStoreToRegisterInFrame(MCScriptFrame *p_frame, int p_register, MCValueRef p_value)
{
    /* LOAD CHECK */ __MCScriptAssert__(MCScriptIsRegisterValidInFrame(p_frame, p_register),
                       "register out of range on store");
    
    if (p_frame -> slots[p_register] != p_value)
    {
        MCValueRelease(p_frame -> slots[p_register]);
        p_frame -> slots[p_register] = MCValueRetain(p_value);
    }
}

static bool MCScriptCheckedFetchFromRegisterInFrame(MCScriptFrame *p_frame, int p_register, MCValueRef& r_value)
{
    MCValueRef t_value;
    t_value = MCScriptFetchFromRegisterInFrame(p_frame, p_register);
    
    if (t_value == kMCNull &&
        !MCScriptGetRegisterTypeIsOptionalInFrame(p_frame, p_register))
        return MCScriptThrowLocalVariableUsedBeforeDefinedError(p_frame -> instance -> module, p_frame -> handler, p_register);
    
    r_value = t_value;
    
    return true;
}

static bool MCScriptCheckedStoreToRegisterInFrame(MCScriptFrame *p_frame, int p_register, MCValueRef p_value)
{
    MCTypeInfoRef t_type;
    t_type = MCScriptGetRegisterTypeInFrame(p_frame, p_register);
    
    if (t_type != nil &&
        !MCTypeInfoConforms(MCValueGetTypeInfo(p_value), t_type))
        return MCScriptThrowInvalidValueForLocalVariableError(p_frame -> instance -> module, p_frame -> handler, p_register, t_type, p_value);
    
    MCScriptStoreToRegisterInFrame(p_frame, p_register, p_value);
    
    return true;
}

static inline MCValueRef MCScriptFetchConstantInFrame(MCScriptFrame *p_frame, int p_index)
{
    /* LOAD CHECK */ __MCScriptAssert__(MCScriptIsConstantValidInFrame(p_frame, p_index),
                       "constant out of range on fetch");
    return p_frame -> instance -> module -> values[p_index];
}

static bool MCScriptPerformScriptInvoke(MCScriptFrame*& x_frame, byte_t*& x_next_bytecode, MCScriptInstanceRef p_instance, MCScriptHandlerDefinition *p_handler, uindex_t *p_arguments, uindex_t p_arity)
{
    MCTypeInfoRef t_signature;
    t_signature = p_instance -> module -> types[p_handler -> type] -> typeinfo;
    
    // If the signature of the handler includes a return value, then the first
    // argument will be the register to place it in.
    uindex_t t_result_reg;
    t_result_reg = p_arguments[0];
    p_arity -= 1;
    p_arguments += 1;
    
    // If the number of remaining arguments is not the same as the parameter count then
    // it is an error.
    if (MCHandlerTypeInfoGetParameterCount(t_signature) != p_arity)
        return MCScriptThrowWrongNumberOfArgumentsForInvokeError(p_instance -> module, p_handler, p_arity);
    
    // Check that the arguments all conform to the required types and that we aren't
    // using any local variables before they have been assigned.
    for(uindex_t i = 0; i < p_arity; i++)
    {
        // Out parameters are initialized to undefined, so we don't care about
        // their values on input.
        if (MCHandlerTypeInfoGetParameterMode(t_signature, i) == kMCHandlerTypeFieldModeOut)
            continue;
        
        MCValueRef t_value;
        if (!MCScriptCheckedFetchFromRegisterInFrame(x_frame, p_arguments[i], t_value))
            return false;
        
        MCTypeInfoRef t_type;
        t_type = MCHandlerTypeInfoGetParameterType(t_signature, i);
        
        if (!MCTypeInfoConforms(MCValueGetTypeInfo(t_value), t_type))
            return MCScriptThrowInvalidValueForArgumentError(p_instance -> module, p_handler, i, t_type, t_value);
    }
    
    // Create a (stack) frame for the handler.
    MCScriptFrame *t_callee;
    if (!MCScriptCreateFrame(x_frame, p_instance, p_handler, t_callee))
        return false;
    
    // We need to record a mapping vector if we have any out parameters.
    bool t_needs_mapping;
    t_needs_mapping = false;

    // Fetch the parameter values and store them in the appropriate slots. The
    // parameters are always the first 'arity' slots in the frame.
    for(int i = 0; i < MCHandlerTypeInfoGetParameterCount(t_signature); i++)
    {
        MCHandlerTypeFieldMode t_mode;
        t_mode = MCHandlerTypeInfoGetParameterMode(t_signature, i);
        
        MCValueRef t_value;
        if (t_mode != kMCHandlerTypeFieldModeOut)
            t_value = MCScriptFetchFromRegisterInFrame(x_frame, p_arguments[i]);
        else
            t_value = kMCNull;
        
        if (t_mode != kMCHandlerTypeFieldModeIn)
            t_needs_mapping = true;
        
        MCValueAssign(t_callee -> slots[i], t_value);
    }
    
    if (t_needs_mapping)
    {
        if (!MCMemoryNewArray(p_arity, t_callee -> mapping))
		{
			MCScriptDestroyFrame (t_callee);
            return false;
		}
        
        MCMemoryCopy(t_callee -> mapping, p_arguments, sizeof(int) * p_arity);
    }
    
    t_callee -> result = t_result_reg;
    
    x_frame = t_callee;
    x_next_bytecode = x_frame -> instance -> module -> bytecode + x_frame -> address;
    
	return true;
}

// This method resolves the binding string in the foreign function. The format is:
//   [lang:][library@][class.]function[!calling]
//
// lang - one of c, cpp, objc or java. If not present, it is taken to be c.
// library - the library to load the symbol from. If not present, it is taken to be the
//   main executable.
// class - only valid for cpp, objc, or java. If not present, it is taken to be no class.
// function - the name of the function or method.
// calling - the calling convention, if not present it is taken to be the platform default.
//
// The calling conventions are:
//   default (sysv on 32-bit unix intel, unix64 on 64-bit unix intel, win64 on 64-bit windows, cdelc on 32-bit windows)
//   stdcall (windows 32-bit)
//   thiscall (windows 32-bit)
//   fastcall (windows 32-bit)
//   cdecl (windows 32-bit)
//   pascal (windows 32-bit)
//   register (windows 32-bit)
// If a windows calling convention is specified, it is taken to be 'default' on
// other platforms.
//

static bool __split_binding(MCStringRef& x_string, codepoint_t p_char, MCStringRef& r_field)
{
    MCStringRef t_head, t_tail;
    if (!MCStringDivideAtChar(x_string, p_char, kMCStringOptionCompareExact, t_head, t_tail))
        return false;
    
    if (!MCStringIsEmpty(t_tail))
    {
        r_field = t_head;
        MCValueRelease(x_string);
        x_string = t_tail;
    }
    else
    {
        MCValueRelease(x_string);
		MCValueRelease(t_tail);
        x_string = t_head;
    }
    
    return true;
}

static bool MCScriptResolveForeignFunctionBinding(MCScriptForeignHandlerDefinition *p_handler, ffi_abi& r_abi, bool p_throw, bool& r_bound)
{
    MCStringRef t_rest;
    t_rest = MCValueRetain(p_handler -> binding);
    
    MCAutoStringRef t_language;
    MCAutoStringRef t_library;
    MCAutoStringRef t_class;
    MCAutoStringRef t_function;
    MCAutoStringRef t_calling;
    if (!__split_binding(t_rest, ':', &t_language) ||
        !__split_binding(t_rest, '>', &t_library) ||
        !__split_binding(t_rest, '.', &t_class) ||
        !MCStringDivideAtChar(t_rest, '!', kMCStringOptionCompareExact, &t_function, &t_calling))
    {
        MCValueRelease(t_rest);
        return false;
    }
    
    MCValueRelease(t_rest);
    
    int t_cc;
    if (!MCStringIsEmpty(*t_calling))
    {
        static const char *s_callconvs[] = { "default", "sysv", "stdcall", "thiscall", "fastcall", "cdecl", "pascal", "register", nil};
        for(t_cc = 0; s_callconvs[t_cc] != nil; t_cc++)
            if (MCStringIsEqualToCString(*t_calling, s_callconvs[t_cc], kMCStringOptionCompareCaseless))
                break;
        if (s_callconvs[t_cc] == nil)
            return MCErrorCreateAndThrow(kMCGenericErrorTypeInfo, "reason", MCSTR("unknown calling convention"), nil);
    }
    else
        t_cc = 0;
    
    if (MCStringIsEmpty(*t_function))
        return MCErrorCreateAndThrow(kMCGenericErrorTypeInfo, "reason", MCSTR("no function specified in binding string"), nil);
    
    if (MCStringIsEmpty(*t_language) ||
        MCStringIsEqualToCString(*t_language, "c", kMCStringOptionCompareExact))
    {
        if (!MCStringIsEmpty(*t_class))
            return MCErrorCreateAndThrow(kMCGenericErrorTypeInfo, "reason", MCSTR("class not allowed in c binding string"), nil);
        
        
#ifdef _WIN32
        if (MCStringIsEmpty(*t_library))
        {
            p_handler -> function = GetProcAddress(GetModuleHandle(NULL), MCStringGetCString(*t_function));
        }
        else
        {
            HMODULE t_module;
			MCAutoStringRefAsWString t_library_wstr;
			if (!t_library_wstr.Lock(*t_library))
                return false;
            t_module = LoadLibraryW(*t_library_wstr);
            if (t_module == nil)
            {
                if (p_throw)
                    return MCErrorCreateAndThrow(kMCGenericErrorTypeInfo, "reason", MCSTR("unable to load foreign library"), nil);

                r_bound = false;
                return true;
            }
            p_handler -> function = GetProcAddress(t_module, MCStringGetCString(*t_function));
        }
#else
        if (MCStringIsEmpty(*t_library))
        {
            void* t_self;
#ifdef TARGET_SUBPLATFORM_ANDROID
            t_self = dlopen("librevandroid.so", 0);
            if (t_self == NULL)
            {
                return MCErrorCreateAndThrow(kMCGenericErrorTypeInfo, "reason", MCSTR("could not bind to engine"), nil);
            }
#else
            t_self = dlopen(NULL, 0);
#endif
            p_handler -> function = dlsym(t_self, MCStringGetCString(*t_function));
        }
        else
        {
            MCAutoStringRefAsUTF8String t_utf8_library;
            if (!t_utf8_library.Lock(*t_library))
                return false;
            void *t_module;
            t_module = dlopen(*t_utf8_library, RTLD_LAZY);
            if (t_module == nil)
            {
                if (p_throw)
                    return MCErrorCreateAndThrow(kMCGenericErrorTypeInfo, "reason", MCSTR("unable to load foreign library"), nil);
             
                r_bound = false;
                return true;
            }
            p_handler -> function = dlsym(t_module, MCStringGetCString(*t_function));
        }
#endif
    }
    else if (MCStringIsEqualToCString(*t_language, "cpp", kMCStringOptionCompareExact))
    {
        return MCErrorCreateAndThrow(kMCGenericErrorTypeInfo, "reason", MCSTR("c++ binding not implemented yet"), nil);
    }
    else if (MCStringIsEqualToCString(*t_language, "objc", kMCStringOptionCompareExact))
    {
#if !defined(_MACOSX) && !defined(TARGET_SUBPLATFORM_IPHONE)
        if (p_throw)
            return MCErrorCreateAndThrow(kMCGenericErrorTypeInfo, "reason", MCSTR("objc binding not supported on this platform"), nil);
        
        r_bound = false;
        return true;
#else
        return MCErrorCreateAndThrow(kMCGenericErrorTypeInfo, "reason", MCSTR("objc binding not implemented yet"), nil);
#endif
    }
    else if (MCStringIsEqualToCString(*t_language, "java", kMCStringOptionCompareExact))
    {
#if !defined(TARGET_SUBPLATFORM_ANDROID)
        if (p_throw)
            return MCErrorCreateAndThrow(kMCGenericErrorTypeInfo, "reason", MCSTR("java binding not supported on this platform"), nil);
        
        r_bound = false;
        return true;
#else
        return MCErrorCreateAndThrow(kMCGenericErrorTypeInfo, "reason", MCSTR("java binding not implemented yet"), nil);
#endif
    }
    
#ifdef _WIN32
    r_abi = t_cc == 0 ? FFI_DEFAULT_ABI : (ffi_abi)t_cc;
#else
    r_abi = FFI_DEFAULT_ABI;
#endif
    
    if (!p_throw)
        r_bound = true;
    
    return true;
}

// If p_throw is true, then this function throws an error if resolution fails.
// If p_throw is false, then this function does not throw errors related to being
// unable to bind. Instead it returns true, and indicates binding success in r_bound.
static bool MCScriptPrepareForeignFunction(MCScriptFrame *p_frame, MCScriptInstanceRef p_instance, MCScriptForeignHandlerDefinition *p_handler, bool p_throw, bool& r_bound)
{
    ffi_abi t_abi;
    if (!MCScriptResolveForeignFunctionBinding(p_handler, t_abi, p_throw, r_bound))
        return false;
    
    if (!p_throw && !r_bound)
        return true;
    
    if (p_handler -> function == nil)
    {
        if (p_throw)
            return MCScriptThrowUnableToResolveForeignHandlerError(p_instance -> module, p_handler);
        
        r_bound = false;
        
        return true;
    }
    
    MCTypeInfoRef t_signature;
    t_signature = p_instance -> module -> types[p_handler -> type] -> typeinfo;
    
    MCTypeInfoRef t_return_type;
    t_return_type = MCHandlerTypeInfoGetReturnType(t_signature);
    
    MCResolvedTypeInfo t_resolved_return_type;
    if (!MCTypeInfoResolve(t_return_type, t_resolved_return_type))
        return MCScriptThrowUnableToResolveTypeError(t_return_type);
    
    ffi_type *t_ffi_return_type;
    if (t_return_type != kMCNullTypeInfo)
    {
        if (MCTypeInfoIsForeign(t_resolved_return_type . type))
            t_ffi_return_type = (ffi_type *)MCForeignTypeInfoGetLayoutType(t_resolved_return_type . type);
        else
            t_ffi_return_type = &ffi_type_pointer;
    }
    else
        t_ffi_return_type = &ffi_type_void;
    
    uindex_t t_arity;
    t_arity = MCHandlerTypeInfoGetParameterCount(t_signature);
    
    ffi_type **t_ffi_arg_types;
    if (!MCMemoryNewArray(t_arity, t_ffi_arg_types))
        return false;
    
    ffi_cif *t_cif;
    if (!MCMemoryNew(t_cif))
    {
        MCMemoryDeleteArray(t_ffi_arg_types);
        return false;
    }
    
    bool t_success;
    t_success = true;
    for(uindex_t i = 0; t_success && i < t_arity; i++)
    {
        MCTypeInfoRef t_type;
        MCHandlerTypeFieldMode t_mode;
        t_type = MCHandlerTypeInfoGetParameterType(t_signature, i);
        t_mode = MCHandlerTypeInfoGetParameterMode(t_signature, i);
        
        MCResolvedTypeInfo t_resolved_type;
        if (!MCTypeInfoResolve(t_type, t_resolved_type))
        {
            t_success = false;
            break;
        }
    
        if (t_mode == kMCHandlerTypeFieldModeIn)
        {
            if (MCTypeInfoIsForeign(t_resolved_type . type))
                t_ffi_arg_types[i] = (ffi_type *)MCForeignTypeInfoGetLayoutType(t_resolved_type . type);
            else
                t_ffi_arg_types[i] = &ffi_type_pointer;
        }
        else
            t_ffi_arg_types[i] = &ffi_type_pointer;
    }
    
    if (!t_success ||
        ffi_prep_cif(t_cif, t_abi, t_arity, t_ffi_return_type, t_ffi_arg_types) != FFI_OK)
    {
        MCMemoryDeleteArray(t_ffi_arg_types);
        MCMemoryDelete(t_cif);
        return MCErrorThrowGeneric(nil);
    }
    
    p_handler -> function_argtypes = t_ffi_arg_types;
    p_handler -> function_cif = t_cif;
    
    if (!p_throw)
        r_bound = true;
    
    return true;
}

static bool MCScriptPerformForeignInvoke(MCScriptFrame*& x_frame, MCScriptInstanceRef p_instance, MCScriptForeignHandlerDefinition *p_handler, uindex_t *p_arguments, uindex_t p_arity)
{
    if (p_handler -> function == nil)
    {
        bool t_bound;
        if (!MCScriptPrepareForeignFunction(x_frame, p_instance, p_handler, true, t_bound))
            return false;
    }
    
    MCTypeInfoRef t_signature;
    t_signature = p_instance -> module -> types[p_handler -> type] -> typeinfo;
    
    uindex_t t_result_reg;
    t_result_reg = p_arguments[0];
    p_arity -= 1;
    p_arguments += 1;
    
    if (MCHandlerTypeInfoGetParameterCount(t_signature) != p_arity)
        return MCScriptThrowWrongNumberOfArgumentsForInvokeError(p_instance -> module, p_handler, p_arity);
    
    MCHandlerTypeFieldMode t_modes[16];
    MCResolvedTypeInfo t_types[16];
    void *t_args[16];
    ffi_type *t_arg_types[16];
    bool t_arg_new[16];
    uint8_t t_storage[256];
    uindex_t t_storage_index;
    t_storage_index = 0;
    
    uindex_t t_arg_index;
    t_arg_index = 0;
    for(t_arg_index = 0; t_arg_index < p_arity; t_arg_index++)
    {
        t_modes[t_arg_index] = MCHandlerTypeInfoGetParameterMode(t_signature, t_arg_index);
        
        MCTypeInfoRef t_type;
        t_type = MCHandlerTypeInfoGetParameterType(t_signature, t_arg_index);
        
        // We need the target typeinfo regardless of mode.
        if (!MCTypeInfoResolve(t_type, t_types[t_arg_index]))
            return MCScriptThrowUnableToResolveTypeError(t_type);
        
        // Target foreign descriptor.
        const MCForeignTypeDescriptor *t_descriptor;
        if (MCTypeInfoIsForeign(t_types[t_arg_index] . type))
            t_descriptor = MCForeignTypeInfoGetDescriptor(t_types[t_arg_index] . type);
        else
            t_descriptor = nil;
        
        void *t_argument;
        if (t_modes[t_arg_index] != kMCHandlerTypeFieldModeOut)
        {
            // Not an out mode, so we have an input value.
            MCValueRef t_value;
            if (!MCScriptCheckedFetchFromRegisterInFrame(x_frame, p_arguments[t_arg_index], t_value))
                return false;
            
            MCTypeInfoRef t_source_type;
            t_source_type = MCValueGetTypeInfo(t_value);
            
            MCResolvedTypeInfo t_source;
            if (!MCTypeInfoResolve(t_source_type, t_source))
                return MCScriptThrowUnableToResolveTypeError(t_source_type);
            
            // Check that the source type conforms to target.
            if (!MCResolvedTypeInfoConforms(t_source, t_types[t_arg_index]))
                return MCScriptThrowInvalidValueForArgumentError(p_instance -> module, p_handler, t_arg_index, t_type, t_value);
            
            if (MCTypeInfoIsForeign(t_types[t_arg_index] . type))
            {
                // The target type is foreign - t_argument will be a pointer to the
                // contents.
                
                if (MCTypeInfoIsForeign(t_source . type))
                {
                    // The source type is foreign - they must be contents compatible
                    // due to conformance.
                    if (t_modes[t_arg_index] == kMCHandlerTypeFieldModeIn)
                    {
                        // For in mode, we can use the contentsptr direct.
                        
                        t_argument = MCForeignValueGetContentsPtr(t_value);
                    
                        // Nothing to free.
                        t_arg_new[t_arg_index] = false;
                    }
                    else
                    {
                        // For inout mode, we need to copy it.
                        
                        if (!t_descriptor -> copy(MCForeignValueGetContentsPtr(t_value), t_storage + t_storage_index))
                            break;
                        
                        t_argument = t_storage + t_storage_index;
                        t_storage_index += t_descriptor -> size;
                        
                        // Need to finalize the storage.
                        t_arg_new[t_arg_index] = true;
                    }
                }
                else
                {
                    // The source type is not foreign - it must be the target type's
                    // bridge type or null. Thus we must export the value.
                    if (t_value == kMCNull)
                    {
                        if (!t_descriptor -> initialize(t_storage + t_storage_index))
                            break;
                    }
                    else
                    {
                        if (!t_descriptor -> doexport(t_value, false, t_storage + t_storage_index))
                            break;
                    }
                    
                    t_argument = t_storage + t_storage_index;
                    t_storage_index += t_descriptor -> size;
                    
                    // Need to finalize the storage.
                    t_arg_new[t_arg_index] = true;
                }
            }
            else
            {
                // The target type is not foreign - t_argument will be a valueref ptr.
                
                if (MCTypeInfoIsForeign(t_source . type))
                {
                    // The source type is foreign - so the target must be the bridge
                    // type. Thus we must import the value.
                    const MCForeignTypeDescriptor *t_src_descriptor;
                    t_src_descriptor = MCForeignTypeInfoGetDescriptor(t_source . type);
                    
                    if (!t_src_descriptor -> doimport(MCForeignValueGetContentsPtr(t_value), false, t_argument))
                        break;
                    
                    // Need to release the argument.
                    t_arg_new[t_arg_index] = true;
                }
                else
                {
                    // The source type is not foreign - so the source and target are
                    // compatible valuerefs.
                    
                    if (t_modes[t_arg_index] == kMCHandlerTypeFieldModeIn)
                    {
                        // For in mode, we just use it direct - but change to nil if
                        // kMCNull.
                        if (t_value != kMCNull)
                            t_argument = t_value;
                        else
                            t_argument = nil;
                    
                        // Nothing to free.
                        t_arg_new[t_arg_index] = false;
                    }
                    else
                    {
                        // For inout mode, we must copy.
                        if (t_value != kMCNull)
                            t_argument = MCValueRetain(t_value);
                        else
                            t_argument = nil;
                        
                        // Need to release the argument
                        t_arg_new[t_arg_index] = true;
                    }
                }
            }
        }
        else
        {
            // Out parameter, so prepare the target storage.
            if (t_descriptor != nil)
            {
                // Target is foreign - use the initialize method, if any.
                if (t_descriptor -> initialize != nil)
                {
                    if (!t_descriptor -> initialize(t_storage + t_storage_index))
                        break;
                    
                    // Need to finalize the storage.
                    t_arg_new[t_arg_index] = true;
                }
                else
                {
                    // Type has no initialize, so don't finalize if we have to
                    // cleanup.
                    t_arg_new[t_arg_index] = false;
                }
                
                t_argument = t_storage + t_storage_index;
                t_storage_index += t_descriptor -> size;
            }
            else
            {
                // Target is not foreign, so we initialize to nil.
                t_argument = nil;
                
                // Nothing to free.
            }
        }
        
        // We now have the argument in storage, so process according to mode.
        if (t_modes[t_arg_index] == kMCHandlerTypeFieldModeIn)
        {
            
            // In mode types are the map of the foreign type, or pointer if a
            // valueref.
            if (t_descriptor != nil)
            {
                // In mode arguments are the value themselves.
                t_args[t_arg_index] = t_argument;
                t_arg_types[t_arg_index] = (ffi_type *)MCForeignTypeInfoGetLayoutType(t_types[t_arg_index] . type);
            }
            else
            {
                // Allocate space for the storage pointer
                t_args[t_arg_index] = t_storage + t_storage_index;
                t_storage_index += sizeof(uintptr_t);
                *(void **)t_args[t_arg_index] = t_argument;
                t_arg_types[t_arg_index] = &ffi_type_pointer;
            }
        }
        else
        {
            // Out mode arguments are the contents for foreign values but
            // marked 'pointer' type. However, for valuerefs we must make storage
            // to put the valueref in.
            t_arg_types[t_arg_index] = &ffi_type_pointer;
            
            if (t_descriptor != nil)
            {
                // Allocate space for the storage pointer
                t_args[t_arg_index] = t_storage + t_storage_index;
                t_storage_index += sizeof(uintptr_t);
                
                *(void **)t_args[t_arg_index] = t_argument;
            }
            else
            {
                // Allocate space for the storage pointer
                t_args[t_arg_index] = t_storage + t_storage_index;
                t_storage_index += sizeof(uintptr_t);
                
                // The argument is the storage pointer.
                *(void **)t_args[t_arg_index] = t_storage + t_storage_index;
                t_storage_index += sizeof(uintptr_t);
                
                **(void ***)t_args[t_arg_index] = t_argument;
            }
            
        }
    }
    
    // Cleanup after ourselves.
    bool t_success;
    t_success = t_arg_index == p_arity;
    
    // If the arg index reached arity, then everything is setup.
    MCValueRef t_result_value;
    t_result_value = nil;
    if (t_success)
    {
        uint8_t t_result[64];
        ffi_call((ffi_cif *)p_handler -> function_cif, (void(*)())p_handler -> function, &t_result, t_args);
        
        // If no error is pending then do the copyback of the result, and then
        // arguments. Otherwise we just continue the throw.
        if (!MCErrorIsPending())
        {
            MCTypeInfoRef t_return_type;
            MCResolvedTypeInfo t_resolved_return_type;
            t_return_type = MCHandlerTypeInfoGetReturnType(t_signature);
            if (!MCTypeInfoResolve(t_return_type, t_resolved_return_type))
                t_success = false;
            
            if (t_success)
            {
                if (t_resolved_return_type . named_type != kMCNullTypeInfo)
                {
                    if (MCTypeInfoIsForeign(t_resolved_return_type . type))
                    {
                        const MCForeignTypeDescriptor *t_descriptor;
                        t_descriptor = MCForeignTypeInfoGetDescriptor(t_resolved_return_type . type);
                        
                        // If the foreign value has a bridge type, then import.
                        if (t_descriptor -> bridgetype != kMCNullTypeInfo)
                            t_success = t_descriptor -> doimport(t_result, true, t_result_value);
                        else if (!MCForeignValueCreateAndRelease(t_resolved_return_type . named_type, t_result, (MCForeignValueRef&)t_result_value))
                            t_success = false;
                    }
                    else
                    {
                        t_result_value = *(MCValueRef *)t_result;
                        
                        // If the return value is nil, then map to null.
                        if (t_result_value == nil)
                            t_result_value = MCValueRetain(kMCNull);
                    }
                }
                else
                    t_result_value = MCValueRetain(kMCNull);
            }
        }
        else
            t_success = false;
    }
    
    MCValueRef t_out_values[16];
    uindex_t t_out_arg_index;
    t_out_arg_index = 0;
    for(uindex_t i = 0; i < t_arg_index; i++)
    {
        // Target foreign descriptor.
        const MCForeignTypeDescriptor *t_descriptor;
        if (MCTypeInfoIsForeign(t_types[i] . type))
            t_descriptor = MCForeignTypeInfoGetDescriptor(t_types[i] . type);
        else
            t_descriptor = nil;
     
        if (t_modes[i] == kMCHandlerTypeFieldModeIn)
        {
            // In mode parameters we just free the values that need freed.
            if (t_descriptor != nil)
            {
                if (t_arg_new[i])
                    t_descriptor -> finalize(t_args[i]);
            }
            else
            {
                if (t_arg_new[i])
                    MCValueRelease((MCValueRef)t_args[i]);
            }
            
            t_out_values[i] = nil;
        }
        else if (t_success)
        {
            // Out mode parameters we must do something with the value - but only if
            // things succeeded (i.e. arity == arg_index).
            if (t_descriptor != nil)
            {
                // If the foreign value has a notion of defined, then we check that.
                // If the value isn't defined, we return null.
                if (t_descriptor -> defined != nil &&
                    !t_descriptor -> defined(t_args[i]))
                    t_out_values[i] = MCValueRetain(kMCNull);
                else
                {
                    // Otherwise, we build a foreign value out of it.
                    // Foreign out or in-out parameters are indirect...
                    
                    // If the foreign value has a bridge type, then import.
                    if (t_descriptor -> bridgetype != kMCNullTypeInfo)
                    {
                        if (!t_descriptor -> doimport(*(void**)t_args[i], true, t_out_values[i]))
                        {
                            // If that failed, finalize the contents.
                            t_descriptor -> finalize(t_args[i]);
                            t_success = false;
                            t_out_arg_index = i;
                        }
                    }
                    else
                    {
                        if (!MCForeignValueCreateAndRelease(t_types[i] . named_type, *(void**)t_args[i], (MCForeignValueRef&)t_out_values[i]))
                        {
                            // If that failed, finalize the contents.
                            t_descriptor -> finalize(t_args[i]);
                            t_success = false;
                            t_out_arg_index = i;
                        }
                    }
                }
            }
            else
            {
                // It's just a valueref - nice and easy to do something with.
                
                // If it is non-nil then just take the value; otherwise take kMCNull.
                if (*(MCValueRef *)t_args[i] != nil)
                    t_out_values[i] = **(MCValueRef **)t_args[i];
                else
                    t_out_values[i] = MCValueRetain(kMCNull);
            }
        }
        else
        {
            // Clean up out mode parameters - we get here if something failed along
            // the way, including during out mode processing.
            if (t_descriptor != nil)
            {
                if (t_arg_new[i])
                    t_descriptor -> finalize(t_args[i]);
            }
            else
            {
                if (t_arg_new[i] &&
                    *(MCValueRef *)t_args[i] != nil)
                    MCValueRelease(*(MCValueRef *)t_args[i]);
            }
        }
    }
    
    // Check that the out values conform to register types.
    if (t_success)
    {
        for(uindex_t i = 0; t_success && i < p_arity; i++)
            if (t_out_values[i] != nil)
            {
                MCTypeInfoRef t_type;
                t_type = MCScriptGetRegisterTypeInFrame(x_frame, p_arguments[i]);
                
                if (t_type != nil &&
                    !MCTypeInfoConforms(MCValueGetTypeInfo(t_out_values[i]), t_type))
                    t_success = MCScriptThrowInvalidValueForLocalVariableError(x_frame -> instance -> module, x_frame -> handler, p_arguments[i], t_type, t_out_values[i]);
            }
        
        if (t_result_reg != UINDEX_MAX)
        {
            MCTypeInfoRef t_type;
            t_type = MCScriptGetRegisterTypeInFrame(x_frame, t_result_reg);
            
            if (t_type != nil &&
                !MCTypeInfoConforms(MCValueGetTypeInfo(t_result_value), t_type))
                t_success = MCScriptThrowInvalidValueForResultError(x_frame -> instance -> module, x_frame -> handler, t_type, t_result_value);
        }
    }
    
    if (t_success)
    {
        // If we get here, then we can go through and assign things to registers.
        for(uindex_t i = 0; i < p_arity; i++)
            if (t_out_values[i] != nil)
                MCScriptStoreToRegisterInFrameAndRelease(x_frame, p_arguments[i], t_out_values[i]);
        
        // If there is a result, then store it.
        if (t_result_reg != UINDEX_MAX)
            MCScriptStoreToRegisterInFrameAndRelease(x_frame, t_result_reg, t_result_value);
    }
    else
    {
        // If we get here, then 't_out_arg_index' is the limit of out args that
        // need free due to failure.
        for(uindex_t i = 0; i < t_out_arg_index; i++)
            if (t_out_values[i] != nil)
                MCValueRelease(t_out_values[i]);
        
        // Free the result, if any.
        if (t_result_value != nil)
            MCValueRelease(t_result_value);
    }

    return t_success;
}

static bool MCScriptPerformInvoke(MCScriptFrame*& x_frame, byte_t*& x_next_bytecode, MCScriptInstanceRef p_instance, MCScriptDefinition *p_handler, uindex_t *p_arguments, uindex_t p_arity)
{
    x_frame -> address = x_next_bytecode - x_frame -> instance -> module -> bytecode;
    
    // MCLog("Invoke %@", MCScriptGetNameOfDefinitionInModule(p_instance -> module, p_handler));
    
	if (p_handler -> kind == kMCScriptDefinitionKindHandler)
	{
		MCScriptHandlerDefinition *t_handler;
		t_handler = MCScriptDefinitionAsHandler(p_handler);
        
		return MCScriptPerformScriptInvoke(x_frame, x_next_bytecode, p_instance, t_handler, p_arguments, p_arity);
	}
	else if (p_handler -> kind == kMCScriptDefinitionKindForeignHandler)
	{
		MCScriptForeignHandlerDefinition *t_foreign_handler;
		t_foreign_handler = MCScriptDefinitionAsForeignHandler(p_handler);
		
        MCScriptModuleRef t_previous_module;
        t_previous_module = s_current_module;
        s_current_module = x_frame -> instance -> module;
        
		bool t_success;
        t_success = MCScriptPerformForeignInvoke(x_frame, p_instance, t_foreign_handler, p_arguments, p_arity);
        
        s_current_module = t_previous_module;
        
        return t_success;
	}
	
	/* LOAD CHECK */ __MCScriptUnreachable__("non-handler definition passed to invoke");
    
    return false;
}

static bool MCScriptPerformMultiInvoke(MCScriptFrame*& x_frame, byte_t*& x_next_bytecode, MCScriptInstanceRef p_instance, MCScriptDefinition *p_handler, uindex_t *p_arguments, uindex_t p_arity)
{
    MCScriptDefinitionGroupDefinition *t_group;
    t_group = MCScriptDefinitionAsDefinitionGroup(p_handler);
    
    // We use a simple scoring mechanism to determine which method to use. If
    // input type for an argument is equal to expected type, then this is a
    // score of zero. If input type for an argument conforms to expected type, then
    // this is a score of 1. The method with the lowest score is chosen, with
    // priority given to the first in the list in the case of a tie.
    
    uindex_t t_min_score;
    MCScriptDefinition *t_min_score_def;
    MCScriptInstanceRef t_min_score_inst;
    t_min_score = UINDEX_MAX;
    t_min_score_def = nil;
    t_min_score_inst = nil;
    
    for(uindex_t i = 0; i < t_group -> handler_count; i++)
    {
        MCScriptInstanceRef t_instance;
        MCScriptDefinition *t_definition;
        MCScriptResolveDefinitionInFrame(x_frame, t_group -> handlers[i], t_instance, t_definition);
        
        uindex_t t_type_index;
        if (t_definition -> kind == kMCScriptDefinitionKindHandler)
            t_type_index = static_cast<MCScriptHandlerDefinition *>(t_definition) -> type;
        else if (t_definition -> kind == kMCScriptDefinitionKindForeignHandler)
            t_type_index = static_cast<MCScriptForeignHandlerDefinition *>(t_definition) -> type;
        else
        /* LOAD CHECK */ __MCScriptUnreachable__("non-handler definition in handler group");
        
        MCTypeInfoRef t_type;
        t_type = t_instance -> module -> types[t_type_index] -> typeinfo;
        
        if (MCHandlerTypeInfoGetParameterCount(t_type) != p_arity - 1)
            continue;
        
        uindex_t t_score;
        t_score = 0;
        for(uindex_t j = 0; j < p_arity - 1; j++)
        {
            if (MCHandlerTypeInfoGetParameterMode(t_type, j) == kMCHandlerTypeFieldModeOut)
                continue;
            
            MCValueRef t_value;
            t_value = MCScriptFetchFromRegisterInFrame(x_frame, p_arguments[j + 1]);
            
            MCTypeInfoRef t_value_type, t_param_type;
            t_value_type = MCValueGetTypeInfo(t_value);
            t_param_type = MCHandlerTypeInfoGetParameterType(t_type, j);
            
            // If the types are the same, shortcircuit to a score of 0.
            if (t_value_type == t_param_type)
                continue;
            
            MCResolvedTypeInfo t_resolved_value_type, t_resolved_param_type;
            if (!MCTypeInfoResolve(t_value_type, t_resolved_value_type))
                return MCScriptThrowUnableToResolveTypeError(t_value_type);
            if (!MCTypeInfoResolve(t_param_type, t_resolved_param_type))
                return MCScriptThrowUnableToResolveTypeError(t_param_type);
            
            // If the value is undefined, and the param type doesn't take an optional
            // argument - then this method is no good.
            if (t_resolved_value_type . named_type == kMCNullTypeInfo)
            {
                if (!t_resolved_param_type . is_optional)
                {
                    t_score = UINDEX_MAX;
                    break;
                }
                
                // The value is undefined and the parameter is optional.
                continue;
            }
             
            // If the resolved types are the same, then this is a score of 0 for
            // the argument.
            if (t_resolved_value_type . type == t_resolved_param_type . type)
                continue;
            
            // If the types don't conform, then this method is no good.
            if (!MCResolvedTypeInfoConforms(t_resolved_value_type, t_resolved_param_type))
            {
                t_score = UINDEX_MAX;
                break;
            }
            
            // Otherwise it is a score of 1.
            t_score += 1;
        }
        
        if (t_score < t_min_score)
        {
            t_min_score = t_score;
            t_min_score_def = t_definition;
            t_min_score_inst = t_instance;
        }
    }
    
    if (t_min_score_def != NULL)
        return MCScriptPerformInvoke(x_frame, x_next_bytecode, t_min_score_inst, t_min_score_def, p_arguments, p_arity);
    
    MCAutoProperListRef t_args;
    if (!MCProperListCreateMutable(&t_args))
        return false;
    
    for(uindex_t i = 0; i < p_arity - 1; i++)
        if (!MCProperListPushElementOntoBack(*t_args, MCScriptFetchFromRegisterInFrame(x_frame, p_arguments[i + 1])))
            return false;
        
    return MCScriptThrowUnableToResolveMultiInvoke(p_instance -> module, p_handler, *t_args);
}

bool MCScriptBytecodeIterate(byte_t*& x_bytecode, byte_t *p_bytecode_limit, MCScriptBytecodeOp& r_op, uindex_t& r_arity, uindex_t *r_arguments)
{
    MCScriptBytecodeDecodeOp(x_bytecode, r_op, r_arity);
    if (x_bytecode > p_bytecode_limit)
        return false;
    
    for(uindex_t i = 0; i < r_arity; i++)
    {
        r_arguments[i] = MCScriptBytecodeDecodeArgument(x_bytecode);
        if (x_bytecode > p_bytecode_limit)
            return false;
    }
    
    return true;
}

bool MCScriptCallHandlerOfInstanceInternal(MCScriptInstanceRef self, MCScriptHandlerDefinition *p_handler, MCValueRef *p_arguments, uindex_t p_argument_count, MCValueRef& r_value)
{
    // As this method is called internally, we can be sure that the arguments conform
    // to the signature so in theory don't need to check here.
    
    // TODO: Add static assertion for the above.
    
    MCScriptFrame *t_frame;
    if (!MCScriptCreateFrame(nil, self, p_handler, t_frame))
        return false;
    
    MCTypeInfoRef t_signature;
    t_signature = self -> module -> types[p_handler -> type] -> typeinfo;
    
    // Populate the parameter slots in the frame with the input arguments. These
    // are always the first <arg-count> slots.
    for(uindex_t i = 0; i < p_argument_count; i++)
    {
        MCValueRef t_value;
        if (MCHandlerTypeInfoGetParameterMode(t_signature, i) != kMCHandlerTypeFieldModeOut)
            t_value = MCValueRetain(p_arguments[i]);
        else
            t_value = MCValueRetain(kMCNull);
        
        t_frame -> slots[i] = t_value;
    }

    bool t_success;
    t_success = true;
    
    // This is used to build the mapping array for invokes.
	uindex_t t_arguments[256];
	
    byte_t *t_bytecode;
    t_bytecode = t_frame -> instance -> module -> bytecode + t_frame -> address;
    for(;;)
    {
        byte_t *t_next_bytecode;
        t_next_bytecode = t_bytecode;
        
        MCScriptBytecodeOp t_op;
		uindex_t t_arity;
        MCScriptBytecodeDecodeOp(t_next_bytecode, t_op, t_arity);
		
		for(int i = 0; i < t_arity; i++)
			t_arguments[i] = MCScriptBytecodeDecodeArgument(t_next_bytecode);
		
        switch(t_op)
        {
            case kMCScriptBytecodeOpJump:
            {
                // jump <offset>
                int t_offset;
                t_offset = (t_arguments[0] & 1) != 0 ? -(signed)(t_arguments[0] >> 1) : t_arguments[0] >> 1;
                
                // <offset> is relative to the start of this instruction.
                t_next_bytecode = t_bytecode + t_offset;
            }
            break;
            case kMCScriptBytecodeOpJumpIfTrue:
            {
                // jumpiftrue <register>, <offset>
                int t_register, t_offset;
                t_register = t_arguments[0];
                t_offset = (t_arguments[1] & 1) != 0 ? -(signed)(t_arguments[1] >> 1) : t_arguments[1] >> 1;
                
                // if the value in the register is true, then jump.
                MCValueRef t_value;
                t_value = MCScriptFetchFromRegisterInFrame(t_frame, t_register);
                
                if (MCValueGetTypeCode(t_value) == kMCValueTypeCodeBoolean)
                {
                    if (t_value == kMCTrue)
                        t_next_bytecode = t_bytecode + t_offset;
                }
                else if (MCValueGetTypeInfo(t_value) == kMCBoolTypeInfo)
                {
                    if (*(bool *)MCForeignValueGetContentsPtr(t_value) == 0)
                        t_next_bytecode = t_bytecode + t_offset;
                }
                else
                    t_success = MCScriptThrowNotABooleanError(t_value);
            }
            break;
            case kMCScriptBytecodeOpJumpIfFalse:
            {
                // jumpiffalse <register>, <offset>
                int t_register, t_offset;
                t_register = t_arguments[0];
                t_offset = (t_arguments[1] & 1) != 0 ? -(signed)(t_arguments[1] >> 1) : t_arguments[1] >> 1;
                
                // if the value in the register is true, then jump.
                MCValueRef t_value;
                t_value = MCScriptFetchFromRegisterInFrame(t_frame, t_register);
                
                if (MCValueGetTypeCode(t_value) == kMCValueTypeCodeBoolean)
                {
                    if (t_value == kMCFalse)
                        t_next_bytecode = t_bytecode + t_offset;
                }
                else if (MCValueGetTypeInfo(t_value) == kMCBoolTypeInfo)
                {
                    if (*(bool *)MCForeignValueGetContentsPtr(t_value) == 0)
                        t_next_bytecode = t_bytecode + t_offset;
                }
                else
                    t_success = MCScriptThrowNotABooleanError(t_value);
            }
            break;
            case kMCScriptBytecodeOpAssignConstant:
            {
                // assignconst <dst>, <index>
                int t_dst, t_constant_index;
                t_dst = t_arguments[0];
                t_constant_index = t_arguments[1];
                
                // Fetch the constant.
                MCValueRef t_value;
                t_value = MCScriptFetchConstantInFrame(t_frame, t_constant_index);
                
                // Do a (type-checked) store.
                t_success = MCScriptCheckedStoreToRegisterInFrame(t_frame, t_dst, t_value);
            }
            break;
            case kMCScriptBytecodeOpAssign:
            {
                // assign <dst>, <src>
                int t_dst, t_src;
                t_dst = t_arguments[0];
                t_src = t_arguments[1];
                
                // Do a (defined-checked) fetch.
                MCValueRef t_value;
                t_success = MCScriptCheckedFetchFromRegisterInFrame(t_frame, t_src, t_value);
                
                // Do a (type-checked) store.
                if (t_success)
                    t_success = MCScriptCheckedStoreToRegisterInFrame(t_frame, t_dst, t_value);
            }
            break;
            case kMCScriptBytecodeOpReturn:
            {
                // Fetch the value of the result.
                MCValueRef t_value;
                if (t_arity == 0)
                    t_value = kMCNull;
                else
                {
                    // return <reg>
                    int t_reg;
                    t_reg = t_arguments[0];
                    t_success = MCScriptCheckedFetchFromRegisterInFrame(t_frame, t_reg, t_value);
                }
                
                // Fetch the signature of the current handler.
                MCTypeInfoRef t_signature;
                MCTypeInfoRef t_output_type;
                MCTypeInfoRef t_input_type;
                t_signature = nil;
                t_output_type = nil;
                t_input_type = nil;
                if (t_success)
                {
                    t_signature = t_frame -> instance -> module -> types[t_frame -> handler -> type] -> typeinfo;
                    t_output_type = MCHandlerTypeInfoGetReturnType(t_signature);
                    t_input_type = MCValueGetTypeInfo(t_value);
                }
                
                MCResolvedTypeInfo t_resolved_input_type, t_resolved_output_type;
                if (t_success &&
                    !MCTypeInfoResolve(t_input_type, t_resolved_input_type))
                    t_success = MCScriptThrowUnableToResolveTypeError(t_input_type);
                if (t_success &&
                    !MCTypeInfoResolve(t_output_type, t_resolved_output_type))
                    t_success = MCScriptThrowUnableToResolveTypeError(t_output_type);
                if (t_success &&
                    !MCResolvedTypeInfoConforms(t_resolved_input_type, t_resolved_output_type))
                    t_success = MCScriptThrowInvalidValueForResultError(t_frame -> instance -> module, t_frame -> handler, t_output_type, t_value);
                
                // Check that all out variables which should be defined are.
                for(uindex_t i = 0; t_success && i < MCHandlerTypeInfoGetParameterCount(t_signature); i++)
                    if (MCHandlerTypeInfoGetParameterMode(t_signature, i) == kMCHandlerTypeFieldModeOut)
                    {
                        if (MCScriptFetchFromRegisterInFrame(t_frame, i) == kMCNull &&
                            !MCScriptGetRegisterTypeIsOptionalInFrame(t_frame, i))
                            t_success = MCScriptThrowOutParameterNotDefinedError(t_frame -> instance -> module, t_frame -> handler, i);
                    }
                
                // At this point we know that the result value conforms with the signature
                // and the out parameters conform to their types and definedness.
                
                if (t_success)
                {
                    if (t_frame -> caller == nil)
                    {
                        for(uindex_t i = 0; i < MCHandlerTypeInfoGetParameterCount(t_signature); i++)
                            if (MCHandlerTypeInfoGetParameterMode(t_signature, i) != kMCHandlerTypeFieldModeIn)
                            {
                                if (p_arguments[i] != nil)
                                    MCValueAssign(p_arguments[i], MCScriptFetchFromRegisterInFrame(t_frame, i));
                                else
                                    p_arguments[i] = MCValueRetain(MCScriptFetchFromRegisterInFrame(t_frame, i));
                            }
                        
                        // Set the result value argument.
                        r_value = MCValueRetain(t_value);
                    }
                    else
                    {
                        if (t_frame -> mapping != nil)
                            for(uindex_t i = 0; i < MCHandlerTypeInfoGetParameterCount(t_signature); i++)
                            {
                                if (MCHandlerTypeInfoGetParameterMode(t_signature, i) != kMCHandlerTypeFieldModeIn)
                                    t_success = MCScriptCheckedStoreToRegisterInFrame(t_frame -> caller, t_frame -> mapping[i], MCScriptFetchFromRegisterInFrame(t_frame, i));
                            }
                        
                        // Store the result in the appropriate reg in the caller.
                        if (t_success && t_frame -> result != UINDEX_MAX)
                            t_success = MCScriptCheckedStoreToRegisterInFrame(t_frame -> caller, t_frame -> result, t_value);
                        
                        // Update the bytecode pointer to that of the caller.
                        t_next_bytecode = t_frame -> caller -> instance -> module -> bytecode + t_frame -> caller -> address;
                    }
                }
                
                // Pop and destroy the top frame of the stack, but only if there
                // is no error.
                if (t_success)
                    t_frame = MCScriptDestroyFrame(t_frame);
            }
            break;
            case kMCScriptBytecodeOpInvoke:
            {
                // invoke <index>, <special>, <arg_1>, ..., <arg_n>
                int t_index;
                t_index = t_arguments[0];
				
                // The 'PerformInvoke' method expects a list of registers, the
                // first being the result register, followed by each argument
                // register.
                uindex_t t_result_then_arg_count;
                uindex_t *t_result_then_args;
                t_result_then_arg_count = t_arity - 1;
                t_result_then_args = &t_arguments[1];
                
				MCScriptInstanceRef t_instance;
                MCScriptDefinition *t_definition;
                MCScriptResolveDefinitionInFrame(t_frame, t_index, t_instance, t_definition);

                if (t_definition -> kind != kMCScriptDefinitionKindDefinitionGroup)
                    t_success = MCScriptPerformInvoke(t_frame, t_next_bytecode, t_instance, t_definition, t_result_then_args, t_result_then_arg_count);
                else
                    t_success = MCScriptPerformMultiInvoke(t_frame, t_next_bytecode, t_instance, t_definition, t_result_then_args, t_result_then_arg_count);
            }
            break;
            case kMCScriptBytecodeOpInvokeIndirect:
            {
                // invoke *<src>, <result>, <arg_1>, ..., <arg_n>
				int t_src;
				t_src = t_arguments[0];
				
				MCValueRef t_handler;
                t_handler = nil;
                if (t_success)
                    t_success = MCScriptCheckedFetchFromRegisterInFrame(t_frame, t_src, t_handler);
				
                if (t_success &&
                    MCValueGetTypeCode(t_handler) != kMCValueTypeCodeHandler)
                    t_success = MCScriptThrowNotAHandlerValueError(t_handler);
                
                // If the handler value is a 'script handler' then we can go direct.
                // Otherwise we have to indirect through a ValueRef array.
                if (t_success &&
                    MCHandlerGetCallbacks((MCHandlerRef)t_handler) == &__kMCScriptHandlerCallbacks)
                {
                    __MCScriptHandlerContext *t_context;
                    t_context = (__MCScriptHandlerContext *)MCHandlerGetContext((MCHandlerRef)t_handler);

                    // The 'PerformInvoke' method expects a list of registers, the
                    // first being the result register, followed by each argument
                    // register.
                    uindex_t t_result_then_arg_count;
                    uindex_t *t_result_then_args;
                    t_result_then_arg_count = t_arity - 1;
                    t_result_then_args = &t_arguments[1];
                    
                    t_success = MCScriptPerformInvoke(t_frame, t_next_bytecode, t_context -> instance, t_context -> definition, t_result_then_args, t_result_then_arg_count);
                }
                else if (t_success)
                {
                    // The argument at index 1 is the result register.
                    uindex_t t_result_reg;
                    t_result_reg = t_arguments[1];
                    
                    // The actual arguments start at index 2 onwards.
                    uindex_t t_arg_count;
                    uindex_t *t_arg_regs;
                    t_arg_count = t_arity - 2;
                    t_arg_regs = &t_arguments[2];
                    
                    // As the arguments to an invoke is a list of (non-contiguous)
                    // registers, we must build a contiguous array of their values
                    // to pass to HandlerInvoke.
                    MCValueRef *t_linear_args;
                    t_linear_args = nil;
                    if (t_success)
                        t_success = MCMemoryNewArray(t_arg_count, t_linear_args);
                
                    for(int i = 0; t_success && i < t_arg_count; i++)
                        t_success = MCScriptCheckedFetchFromRegisterInFrame(t_frame, t_arg_regs[i], t_linear_args[i]);
                
                    MCValueRef t_result;
                    t_result = nil;
                    if (t_success)
                        t_success = MCHandlerInvoke((MCHandlerRef)t_handler, t_linear_args, t_arg_count, t_result);
                        
                    // If the call succeeded, we must copy back all 'out' / 'inout' mode parameters
                    // to the register file, and also the result.
                    if (t_success)
                    {
                        MCTypeInfoRef t_signature;
                        t_signature = MCValueGetTypeInfo(t_handler);
                        
                        // This loop doesn't terminate on failure as we must free all out values
                        // (an error at this point would be a typecheck when storing - we shouldn't
                        // do any more storing, but must release the rest of the out's).
                        for(uindex_t i = 0; i < MCHandlerTypeInfoGetParameterCount(t_signature); i++)
                        {
                            if (MCHandlerTypeInfoGetParameterMode(t_signature, i) == kMCHandlerTypeFieldModeIn)
                                continue;
                            
                            if (t_success)
                                t_success = MCScriptCheckedStoreToRegisterInFrame(t_frame, t_arg_regs[i], t_linear_args[i]);
                            
                            MCValueRelease(t_linear_args[i]);
                        }

                        if (t_success)
                            t_success = MCScriptCheckedStoreToRegisterInFrame(t_frame, t_result_reg, t_result);
                        
                        MCValueRelease(t_result);
                    }
                    
                    MCMemoryDeleteArray(t_linear_args);
                }
            }
            break;
            case kMCScriptBytecodeOpFetch:
            {
                MCAssert(t_arity == 2);
                
                // fetch <dst>, <index>
                int t_dst, t_index;
                t_dst = t_arguments[0];
                t_index = t_arguments[1];
                
				MCScriptInstanceRef t_instance;
                MCScriptDefinition *t_definition;
                MCScriptResolveDefinitionInFrame(t_frame, t_index, t_instance, t_definition);
                
                // Fetch the value:
                //   - variables get fetched from the slot
                //   - constants from the constant pool
                //   - handlers have a value constructed
                if (t_definition -> kind == kMCScriptDefinitionKindVariable)
                {
                    MCScriptVariableDefinition *t_var_definition;
                    t_var_definition = static_cast<MCScriptVariableDefinition *>(t_definition);
                    
                    MCValueRef t_value;
                    t_value = t_instance -> slots[t_var_definition -> slot_index];
                
                    if (t_value == kMCNull &&
                        !MCTypeInfoIsOptional(t_instance -> module -> types[t_var_definition -> type] -> typeinfo))
                        t_success = MCScriptThrowGlobalVariableUsedBeforeDefinedError(t_frame -> instance -> module, t_index);
                    
                    if (t_success)
                        t_success = MCScriptCheckedStoreToRegisterInFrame(t_frame, t_dst, t_value);
                }
                else if (t_definition -> kind == kMCScriptDefinitionKindConstant)
                {
                    MCScriptConstantDefinition *t_constant_definition;
                    t_constant_definition = static_cast<MCScriptConstantDefinition *>(t_definition);
                    
                    MCValueRef t_value;
                    t_value = t_instance -> module -> values[t_constant_definition -> value];
                    
                    if (t_success)
                        t_success = MCScriptCheckedStoreToRegisterInFrame(t_frame, t_dst, t_value);
                    
                }
                else if (t_definition -> kind == kMCScriptDefinitionKindHandler)
                {
                    MCScriptHandlerDefinition *t_handler_definition;
                    t_handler_definition = static_cast<MCScriptHandlerDefinition *>(t_definition);
                    
                    MCTypeInfoRef t_signature;
                    t_signature = t_instance -> module -> types[t_handler_definition -> type] -> typeinfo;
                    
                    // The context struct is 'moved' into the handlerref.
                    __MCScriptHandlerContext t_context;
                    t_context . instance = MCScriptRetainInstance(t_instance);
                    t_context . definition = t_handler_definition;
                    
                    MCHandlerRef t_value;
                    t_success = MCHandlerCreate(t_signature, &__kMCScriptHandlerCallbacks, &t_context, t_value);
                    
                    if (t_success)
                    {
                        t_success = MCScriptCheckedStoreToRegisterInFrame(t_frame, t_dst, t_value);
                        MCValueRelease(t_value);
                    }
                }
                else if (t_definition -> kind == kMCScriptDefinitionKindForeignHandler)
                {
                    MCScriptForeignHandlerDefinition *t_handler_definition;
                    t_handler_definition = static_cast<MCScriptForeignHandlerDefinition *>(t_definition);
                    
                    MCTypeInfoRef t_signature;
                    t_signature = t_instance -> module -> types[t_handler_definition -> type] -> typeinfo;
                    
                    bool t_bound;
                    if (t_handler_definition -> function == nil)
                    {
                        if (!MCScriptPrepareForeignFunction(t_frame, t_instance, t_handler_definition, false, t_bound))
	                        t_success = false;
                    }
                    else
                        t_bound = true;
                    
                    if (t_success)
                    {
	                    if (t_bound)
	                    {
		                    // The context struct is 'moved' into the handlerref.
		                    __MCScriptHandlerContext t_context;
		                    t_context . instance = MCScriptRetainInstance(t_instance);
		                    t_context . definition = t_handler_definition;
                        
		                    MCHandlerRef t_value;
		                    t_success = MCHandlerCreate(t_signature, &__kMCScriptHandlerCallbacks, &t_context, t_value);
                        
		                    if (t_success)
		                    {
			                    t_success = MCScriptCheckedStoreToRegisterInFrame(t_frame, t_dst, t_value);
			                    MCValueRelease(t_value);
		                    }
	                    }
	                    else
	                    {
		                    t_success = MCScriptCheckedStoreToRegisterInFrame(t_frame, t_dst, kMCNull);
	                    }
                    }
                }
            }
            break;
            case kMCScriptBytecodeOpStore:
            {
                MCAssert(t_arity == 2);
                
                // store <src>, <index>
                int t_dst, t_index;
                t_dst = t_arguments[0];
                t_index = t_arguments[1];
                
				MCScriptInstanceRef t_instance;
                MCScriptDefinition *t_definition;
                MCScriptResolveDefinitionInFrame(t_frame, t_index, t_instance, t_definition);
                
                __MCScriptAssert__(t_definition -> kind == kMCScriptDefinitionKindVariable,
                                   "definition not a variable in store global");
                
                MCScriptVariableDefinition *t_var_definition;
                t_var_definition = static_cast<MCScriptVariableDefinition *>(t_definition);
                
                MCTypeInfoRef t_output_type;
                t_output_type = t_instance -> module -> types[t_var_definition -> type] -> typeinfo;
                
                MCValueRef t_value;
                t_success = MCScriptCheckedFetchFromRegisterInFrame(t_frame, t_dst, t_value);
                
                if (t_success &&
                    !MCTypeInfoConforms(MCValueGetTypeInfo(t_value), t_output_type))
                    t_success = MCScriptThrowInvalidValueForGlobalVariableError(t_frame -> instance -> module, t_index, t_output_type, t_value);
                
                if (t_success)
                {
                    if (t_instance -> slots[t_var_definition -> slot_index] != t_value)
                    {
                        MCValueRelease(t_instance -> slots[t_var_definition -> slot_index]);
                        t_instance -> slots[t_var_definition -> slot_index] = MCValueRetain(t_value);
                    }
                }
            }
            break;
            case kMCScriptBytecodeOpAssignList:
            {
                int t_dst;
                t_dst = t_arguments[0];
                
                MCValueRef *t_values;
                t_values = nil;
                if (!MCMemoryNewArray(t_arity - 1, t_values))
                    t_success = false;
                
                for(uindex_t i = 1; t_success && i < t_arity; i++)
                    t_success = MCScriptCheckedFetchFromRegisterInFrame(t_frame, t_arguments[i], t_values[i - 1]);
                
                MCProperListRef t_list;
                t_list = nil;
                if (t_success)
                    t_success = MCProperListCreate(t_values, t_arity - 1, t_list);
                
                if (t_success)
                {
                    t_success = MCScriptCheckedStoreToRegisterInFrame(t_frame, t_dst, t_list);
                    MCValueRelease(t_list);
                }
                
                if (t_values != nil)
                    MCMemoryDeleteArray(t_values);
            }
            break;
        }
        
        // If we failed, then make sure the frame address is up to date.
        if (!t_success)
        {
            t_frame -> address = t_bytecode - t_frame -> instance -> module -> bytecode;
            break;
        }
        
        if (t_frame == nil)
            break;
        
        // Move to the next instruction.
        t_bytecode = t_next_bytecode;
    }
    
    // If we failed, we must unwind the error.
    if (!t_success)
    {
        // Catch the error - this is so we can detect if an error is thrown whilst
        // unwinding.
        MCErrorRef t_error;
        if (!MCErrorCatch(t_error))
            __MCScriptUnreachable__("Error indicated without throwing!");
        
        while(t_frame != nil)
        {
            // If we have an error, and the module in the current frame has debug
            // info, then map.
            if (t_error != nil && t_frame -> instance -> module -> position_count > 0)
            {
                uindex_t t_index;
                for(t_index = 0; t_index < t_frame -> instance -> module -> position_count - 1; t_index++)
                    if (t_frame -> instance -> module -> positions[t_index + 1] . address > t_frame -> address)
                        break;
                
                MCScriptPosition *t_pos;
                t_pos = &t_frame -> instance -> module -> positions[t_index];
                
                // If unwinding fails (due to oom), then release the error.
                if (!MCErrorUnwind(t_error, t_frame -> instance -> module -> source_files[t_pos -> file], t_pos -> line, 1))
                {
                    MCValueRelease(t_error);
                    t_error = nil;
                }
            }
            
            t_frame = MCScriptDestroyFrame(t_frame);
        }
        
        // If we still have an error, throw it again.
        MCErrorThrow(t_error);
    }
    
    return t_success;
}

////////////////////////////////////////////////////////////////////////////////

MCScriptModuleRef MCScriptGetCurrentModule(void)
{
    return s_current_module;
}

////////////////////////////////////////////////////////////////////////////////

MCHandlerCallbacks __kMCScriptHandlerCallbacks =
{
    sizeof(__MCScriptHandlerContext),
    __MCScriptHandlerRelease,
    __MCScriptHandlerInvoke,
	__MCScriptHandlerDescribe,
};

bool __MCScriptHandlerInvoke(void *p_context, MCValueRef *p_arguments, uindex_t p_argument_count, MCValueRef& r_result)
{
    __MCScriptHandlerContext *context;
    context = (__MCScriptHandlerContext *)p_context;
    
    if (context -> definition -> kind != kMCScriptDefinitionKindHandler)
        return MCErrorThrowGeneric(MCSTR("out-of-frame indirect foreign handler calls not yet supported"));
    
    return MCScriptCallHandlerOfInstanceDirect(context -> instance, static_cast<MCScriptHandlerDefinition *>(context -> definition), p_arguments, p_argument_count, r_result);
}

void __MCScriptHandlerRelease(void *p_context)
{
    __MCScriptHandlerContext *context;
    context = (__MCScriptHandlerContext *)p_context;
    MCScriptReleaseInstance(context -> instance);
}

bool
__MCScriptHandlerDescribe (void *p_context,
                           MCStringRef & r_desc)
{
	__MCScriptHandlerContext *context;
	context = (__MCScriptHandlerContext *)p_context;

	MCScriptModuleRef t_module;
	t_module = MCScriptGetModuleOfInstance (context->instance);

	MCNameRef t_module_name;
	t_module_name = MCScriptGetNameOfModule (t_module);

	MCNameRef t_handler_name;
	t_handler_name = MCScriptGetNameOfDefinitionInModule(t_module,
	                                                     context->definition);

	return MCStringFormat(r_desc, "%@.%@()", t_module_name, t_handler_name);
}

////////////////////////////////////////////////////////////////////////////////

extern "C" bool MC_DLLEXPORT MCScriptBuiltinRepeatCounted(uinteger_t *x_count)
{
    if (*x_count == 0)
        return false;
    
    *x_count -= 1;
    return true;
}

extern "C" bool MC_DLLEXPORT MCScriptBuiltinRepeatUpToCondition(double p_counter, double p_limit)
{
    return p_counter <= p_limit;
}

extern "C" double MC_DLLEXPORT MCScriptBuiltinRepeatUpToIterate(double p_counter, double p_step)
{
    return p_counter + p_step;
}

extern "C" bool MC_DLLEXPORT MCScriptBuiltinRepeatDownToCondition(double p_counter, double p_limit)
{
    return p_counter >= p_limit;
}

extern "C" double MC_DLLEXPORT MCScriptBuiltinRepeatDownToIterate(double p_counter, double p_step)
{
    return p_counter + p_step;
}

extern "C" void MC_DLLEXPORT MCScriptBuiltinThrow(MCStringRef p_reason)
{
    MCErrorThrowGeneric(p_reason);
}

////////////////////////////////////////////////////////////////////////////////

#include "script.h"
#include "script-private.h"

#include <ffi/ffi.h>
#include <dlfcn.h>

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
    __MCScriptValidateObjectAndKind__(self, kMCScriptObjectKindInstance);
    
    MCScriptReleaseObject(self);
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
            MCHandlerTypeInfoGetParameterMode(typeinfo, 1) == kMCHandlerTypeFieldModeIn &&
            MCHandlerTypeInfoGetReturnType(typeinfo) == kMCNullTypeInfo;
}

///////////

bool MCScriptThrowAttemptToSetReadOnlyPropertyError(MCScriptModuleRef module, MCNameRef property)
{
    return MCErrorThrowGeneric();
}

bool MCScriptThrowInvalidValueForPropertyError(MCScriptModuleRef module, MCNameRef property, MCTypeInfoRef type, MCValueRef value)
{
    return MCErrorThrowGeneric();
}

bool MCScriptThrowWrongNumberOfArgumentsForHandlerError(MCScriptModuleRef module, MCNameRef handler, uindex_t expected, uindex_t provided)
{
    return MCErrorThrowGeneric();
}

bool MCScriptThrowNoValueProvidedForInParameterError(MCScriptModuleRef module, MCNameRef handler, MCNameRef parameter)
{
    return MCErrorThrowGeneric();
}

bool MCScriptThrowInvalidValueForParameterError(MCScriptModuleRef module, MCNameRef handler, MCNameRef parameter, MCTypeInfoRef type, MCValueRef value)
{
    return MCErrorThrowGeneric();
}

bool MCScriptThrowInvalidValueForResultError(MCScriptModuleRef module, uindex_t address, MCTypeInfoRef type, MCValueRef value)
{
    return MCErrorThrowGeneric();
}

bool MCScriptThrowOutParameterNotDefinedError(MCScriptModuleRef module, uindex_t address, uindex_t index)
{
    return MCErrorThrowGeneric();
}

bool MCScriptThrowLocalVariableUsedBeforeDefinedError(MCScriptModuleRef module, uindex_t address, uindex_t index)
{
    return MCErrorThrowGeneric();
}

bool MCScriptThrowGlobalVariableUsedBeforeDefinedError(MCScriptModuleRef module, uindex_t address, uindex_t index)
{
    return MCErrorThrowGeneric();
}

bool MCScriptThrowInvalidValueForLocalVariableError(MCScriptModuleRef module, uindex_t address, uindex_t index, MCValueRef value)
{
    return MCErrorThrowGeneric();
}

bool MCScriptThrowInvalidValueForGlobalVariableError(MCScriptModuleRef module, uindex_t address, uindex_t index, MCValueRef value)
{
    return MCErrorThrowGeneric();
}

bool MCScriptThrowDefcheckFailureError(MCScriptModuleRef module, uindex_t address)
{
    return MCErrorThrowGeneric();
}

bool MCScriptThrowTypecheckFailureError(MCScriptModuleRef module, uindex_t address, MCTypeInfoRef type, MCValueRef value)
{
    return MCErrorThrowGeneric();
}

bool MCScriptThrowOutOfMemoryError(MCScriptModuleRef module, uindex_t address)
{
    return MCErrorThrowOutOfMemory();
}

bool MCScriptThrowNotABooleanError(MCScriptModuleRef module, uindex_t address, MCValueRef value)
{
    return MCErrorThrowGeneric();
}

bool MCScriptThrowWrongNumberOfArgumentsForInvokeError(MCScriptModuleRef module, uindex_t address, MCScriptDefinition *def, uindex_t arity)
{
    return MCErrorThrowGeneric();
}

bool MCScriptThrowInvalidValueForArgumentError(MCScriptModuleRef module, uindex_t address, MCScriptDefinition *def, uindex_t index, MCValueRef value)
{
    return MCErrorThrowGeneric();
}

bool MCScriptThrowUnableToResolveForeignHandlerError(MCScriptModuleRef module, uindex_t address, MCScriptDefinition *def)
{
    return MCErrorThrowGeneric();
}

bool MCScriptThrowUnableToResolveTypeError(MCScriptModuleRef module, uindex_t address, MCTypeInfoRef type)
{
    return MCErrorThrowGeneric();
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
                       "definition not a forieng handler");
    return static_cast<MCScriptForeignHandlerDefinition *>(self);
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

bool MCScriptCallHandlerOfInstance(MCScriptInstanceRef self, MCNameRef p_handler, MCValueRef *p_arguments, uindex_t p_argument_count, MCValueRef& r_value)
{
    __MCScriptValidateObjectAndKind__(self, kMCScriptObjectKindInstance);
    
    // Lookup the definition (throws if not found).
    MCScriptHandlerDefinition *t_definition;
    if (!MCScriptLookupHandlerDefinitionInModule(self -> module, p_handler, t_definition))
        return false;
    
    // Get the signature of the handler.
    MCTypeInfoRef t_signature;
    t_signature = self -> module -> types[t_definition -> type] -> typeinfo;
    
    // Check the number of arguments.
    uindex_t t_required_param_count;
    t_required_param_count = MCHandlerTypeInfoGetParameterCount(t_signature);
    if (t_required_param_count != p_argument_count)
        return MCScriptThrowWrongNumberOfArgumentsForHandlerError(self -> module, p_handler, t_required_param_count, p_argument_count);
    
    // Check the types of the arguments.
    for(uindex_t i = 0; i < t_required_param_count; i++)
    {
        MCHandlerTypeFieldMode t_mode;
        t_mode = MCHandlerTypeInfoGetParameterMode(t_signature, i);
        
        if (t_mode != kMCHandlerTypeFieldModeOut)
        {
            if (p_arguments[i] == nil)
                return MCScriptThrowNoValueProvidedForInParameterError(self -> module, p_handler, nil);
            
            MCTypeInfoRef t_type;
            t_type = MCHandlerTypeInfoGetParameterType(t_signature, i);
            
            if (!MCTypeInfoConforms(MCValueGetTypeInfo(p_arguments[i]), t_type))
                return MCScriptThrowInvalidValueForParameterError(self -> module, p_handler, nil, t_type, p_arguments[i]);
        }
    }
    
    // Now the input argument array is appropriate, we can just call the handler.
    if (!MCScriptCallHandlerOfInstanceInternal(self, t_definition, p_arguments, p_argument_count, r_value))
        return false;
    
    return true;
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

static bool MCScriptCreateFrame(MCScriptFrame *p_caller, MCScriptInstanceRef p_instance, MCScriptHandlerDefinition *p_handler, MCScriptFrame*& r_frame)
{
    MCScriptFrame *self;
    self = nil;
    if (!MCMemoryNew(self) ||
        !MCMemoryNewArray(p_handler -> slot_count, self -> slots))
    {
        MCMemoryDelete(self);
        return p_caller != nil ? MCScriptThrowOutOfMemoryError(p_caller -> instance -> module, p_caller -> address) : MCScriptThrowOutOfMemoryError(nil, 0);
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
    for(;;)
    {
        byte_t t_next;
        t_next = *x_bytecode_ptr++;
        t_value = (t_value << 7) | (t_next & 0x7f);
        if ((t_next & 0x80) == 0)
            break;
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
        
        t_instance = t_instance -> module -> dependencies[t_import_def -> module] . instance;
        t_definition = t_import_def -> definition;
    }

    r_instance = t_instance;
    r_definition = t_definition;
}

static inline MCValueRef MCScriptFetchFromLocalInFrame(MCScriptFrame *p_frame, uindex_t p_local)
{
    /* LOAD CHECK */ __MCScriptAssert__(p_local >= 0 && p_local < p_frame -> handler -> local_count + MCHandlerTypeInfoGetParameterCount(p_frame -> instance -> module -> types[p_frame -> handler -> type] -> typeinfo),
                                        "local out of range on fetch");
    return p_frame -> slots[p_local];
}

static inline void MCScriptStoreToLocalInFrame(MCScriptFrame *p_frame, uindex_t p_local, MCValueRef p_value)
{
    /* LOAD CHECK */ __MCScriptAssert__(p_local >= 0 && p_local < p_frame -> handler -> local_count + MCHandlerTypeInfoGetParameterCount(p_frame -> instance -> module -> types[p_frame -> handler -> type] -> typeinfo),
                                        "local out of range on store");
    if (p_frame -> slots[p_local] != p_value)
    {
        MCValueRelease(p_frame -> slots[p_local]);
        p_frame -> slots[p_local] = MCValueRetain(p_value);
    }
}

static inline bool MCScriptIsLocalInFrameOptional(MCScriptFrame *p_frame, uindex_t p_local)
{
    /* LOAD CHECK */ __MCScriptAssert__(p_local >= 0 && p_local < p_frame -> handler -> local_count + MCHandlerTypeInfoGetParameterCount(p_frame -> instance -> module -> types[p_frame -> handler -> type] -> typeinfo),
                                        "local out of range on optional check");
    
    MCTypeInfoRef t_handler_type;
    t_handler_type = p_frame -> instance -> module -> types[p_frame -> handler -> type] -> typeinfo;
    
    MCTypeInfoRef t_type;
    uindex_t t_param_count;
    t_param_count = MCHandlerTypeInfoGetParameterCount(t_handler_type);
    if (p_local < t_param_count)
        t_type = MCHandlerTypeInfoGetParameterType(t_handler_type, p_local);
    else
        t_type = p_frame -> instance -> module -> types[p_frame -> handler -> locals[p_local - t_param_count]] -> typeinfo;
    
    MCResolvedTypeInfo t_resolved_type;
    /* RESOLVE UNCHECKED */ MCTypeInfoResolve(t_type, t_resolved_type);
    
    return t_resolved_type . is_optional;
}

static inline bool MCScriptCanLocalInFrameHoldValue(MCScriptFrame *p_frame, uindex_t p_local, MCValueRef p_value)
{
    /* LOAD CHECK */ __MCScriptAssert__(p_local >= 0 && p_local < p_frame -> handler -> local_count + MCHandlerTypeInfoGetParameterCount(p_frame -> instance -> module -> types[p_frame -> handler -> type] -> typeinfo),
                                        "local out of range on type check");

    MCTypeInfoRef t_handler_type;
    t_handler_type = p_frame -> instance -> module -> types[p_frame -> handler -> type] -> typeinfo;
    
    MCTypeInfoRef t_type;
    uindex_t t_param_count;
    t_param_count = MCHandlerTypeInfoGetParameterCount(t_handler_type);
    if (p_local < t_param_count)
        t_type = MCHandlerTypeInfoGetParameterType(t_handler_type, p_local);
    else
        t_type = p_frame -> instance -> module -> types[p_frame -> handler -> locals[p_local - t_param_count]] -> typeinfo;
    
    return MCTypeInfoConforms(MCValueGetTypeInfo(p_value), t_type);
}

static inline MCValueRef MCScriptFetchFromRegisterInFrame(MCScriptFrame *p_frame, int p_register)
{
    /* LOAD CHECK */ __MCScriptAssert__(p_register >= 0 && p_register < p_frame -> handler -> slot_count - p_frame -> handler -> register_offset,
                       "register out of range on fetch");
    return p_frame -> slots[p_frame -> handler -> register_offset + p_register];
}

static inline void MCScriptStoreToRegisterInFrame(MCScriptFrame *p_frame, int p_register, MCValueRef p_value)
{
    /* LOAD CHECK */ __MCScriptAssert__(p_register >= 0 && p_register < p_frame -> handler -> slot_count - p_frame -> handler -> register_offset,
                       "register out of range on store");
    if (p_frame -> slots[p_frame -> handler -> register_offset + p_register] != p_value)
    {
        MCValueRelease(p_frame -> slots[p_frame -> handler -> register_offset + p_register]);
        p_frame -> slots[p_frame -> handler -> register_offset + p_register] = MCValueRetain(p_value);
    }
}

static inline MCValueRef MCScriptFetchConstantInFrame(MCScriptFrame *p_frame, int p_index)
{
    /* LOAD CHECK */ __MCScriptAssert__(p_index >= 0 && p_index < p_frame -> instance -> module -> value_count,
                       "constant out of range on fetch");
    return p_frame -> instance -> module -> values[p_index];
}

static bool MCScriptPerformScriptInvoke(MCScriptFrame*& x_frame, byte_t*& x_next_bytecode, MCScriptInstanceRef p_instance, MCScriptHandlerDefinition *p_handler, uindex_t *p_arguments, uindex_t p_arity)
{
    uindex_t t_result_reg;
    t_result_reg = p_arguments[0];
    
    p_arity -= 1;
    p_arguments += 1;
    
    MCTypeInfoRef t_signature;
    t_signature = p_instance -> module -> types[p_handler -> type] -> typeinfo;
    
    if (MCHandlerTypeInfoGetParameterCount(t_signature) != p_arity)
        return MCScriptThrowWrongNumberOfArgumentsForInvokeError(x_frame -> instance -> module, x_frame -> address, p_handler, p_arity - 1);
    
    for(uindex_t i = 0; i < p_arity; i++)
    {
        if (MCHandlerTypeInfoGetParameterMode(t_signature, i) == kMCHandlerTypeFieldModeOut)
            continue;
        
        MCValueRef t_value;
        t_value = MCScriptFetchFromRegisterInFrame(x_frame, p_arguments[i]);
        
        if (!MCTypeInfoConforms(MCValueGetTypeInfo(t_value), MCHandlerTypeInfoGetParameterType(t_signature, i)))
            return MCScriptThrowInvalidValueForArgumentError(x_frame -> instance -> module, x_frame -> address, p_handler, i, t_value);
    }
        
    MCScriptFrame *t_callee;
    if (!MCScriptCreateFrame(x_frame, p_instance, p_handler, t_callee))
        return false;

    bool t_needs_mapping;
    t_needs_mapping = false;

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
        
        t_callee -> slots[i] = MCValueRetain(t_value);
    }
    
    if (t_needs_mapping)
    {
        if (!MCMemoryNewArray(p_arity, t_callee -> mapping))
            return MCScriptThrowOutOfMemoryError(x_frame -> instance -> module, x_frame -> address);
        
        MCMemoryCopy(t_callee -> mapping, p_arguments, sizeof(int) * p_arity);
    }
    
    t_callee -> result = t_result_reg;
    
    x_frame = t_callee;
    x_next_bytecode = x_frame -> instance -> module -> bytecode + x_frame -> address;
    
	return true;
}

static bool MCScriptPrepareForeignFunction(MCScriptFrame *p_frame, MCScriptInstanceRef p_instance, MCScriptForeignHandlerDefinition *p_handler)
{
    p_handler -> function = dlsym(RTLD_MAIN_ONLY, MCStringGetCString(p_handler -> binding));
    if (p_handler -> function == nil)
        return false;
    
    MCTypeInfoRef t_signature;
    t_signature = p_instance -> module -> types[p_handler -> type] -> typeinfo;
    
    MCTypeInfoRef t_return_type;
    t_return_type = MCHandlerTypeInfoGetReturnType(t_signature);
    
    MCResolvedTypeInfo t_resolved_return_type;
    if (!MCTypeInfoResolve(t_return_type, t_resolved_return_type))
        return false;
    
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
        ffi_prep_cif(t_cif, FFI_DEFAULT_ABI, t_arity, t_ffi_return_type, t_ffi_arg_types) != FFI_OK)
    {
        MCMemoryDeleteArray(t_ffi_arg_types);
        MCMemoryDelete(t_cif);
        return false;
    }
    
    p_handler -> function_argtypes = t_ffi_arg_types;
    p_handler -> function_cif = t_cif;
    
    return true;
}

static bool MCScriptPerformForeignInvoke(MCScriptFrame*& x_frame, MCScriptInstanceRef p_instance, MCScriptForeignHandlerDefinition *p_handler, uindex_t *p_arguments, uindex_t p_arity)
{
    if (p_handler -> function == nil)
    {
        if (!MCScriptPrepareForeignFunction(x_frame, p_instance, p_handler))
            return false;
        //    !__build_function_cif(p_handler -> signature, p_handler -> function_cif))
        //    return MCScriptThrowUnableToResolveForeignHandlerError(x_frame -> instance -> module, x_frame -> address,p_handler);
    }
    
    uindex_t t_result_reg;
    t_result_reg = p_arguments[0];
    
    p_arity -= 1;
    p_arguments += 1;
    
    MCTypeInfoRef t_signature;
    t_signature = p_instance -> module -> types[p_handler -> type] -> typeinfo;
    
    if (MCHandlerTypeInfoGetParameterCount(t_signature) != p_arity)
        return MCScriptThrowWrongNumberOfArgumentsForInvokeError(x_frame -> instance -> module, x_frame -> address, p_handler, p_arity - 1);
    
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
            return MCScriptThrowUnableToResolveTypeError(x_frame -> instance -> module, x_frame -> address, t_type);
        
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
            t_value = MCScriptFetchFromRegisterInFrame(x_frame, p_arguments[t_arg_index]);
            
            MCTypeInfoRef t_source_type;
            t_source_type = MCValueGetTypeInfo(t_value);
            
            MCResolvedTypeInfo t_source;
            if (!MCTypeInfoResolve(t_source_type, t_source))
                return MCScriptThrowUnableToResolveTypeError(x_frame -> instance -> module, x_frame -> address, t_source_type);
            
            // Check that the source type conforms to target.
            if (!MCResolvedTypeInfoConforms(t_source, t_types[t_arg_index]))
                return MCScriptThrowInvalidValueForArgumentError(x_frame -> instance -> module, x_frame -> address, p_handler, t_arg_index, t_value);
            
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
                    // bridge type. Thus we must export the value.
                    if (!t_descriptor -> doexport(t_value, false, t_storage + t_storage_index))
                        break;
                        
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
                    
                    if (!t_descriptor -> doimport(MCForeignValueGetContentsPtr(t_value), false, t_argument))
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
                        // For in mode, we just use it direct.
                        t_argument = t_value;
                    
                        // Nothing to free.
                        t_arg_new[t_arg_index] = false;
                    }
                    else
                    {
                        // For inout mode, we must copy.
                        t_argument = MCValueRetain(t_value);
                        
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
            // In mode arguments are the value themselves.
            t_args[t_arg_index] = t_argument;
            
            // In mode types are the map of the foreign type, or pointer if a
            // valueref.
            if (t_descriptor != nil)
                t_arg_types[t_arg_index] = (ffi_type *)MCForeignTypeInfoGetLayoutType(t_types[t_arg_index] . type);
            else
                t_arg_types[t_arg_index] = &ffi_type_pointer;
        }
        else
        {
            // Out mode arguments are the contents for foreign values but
            // marked 'pointer' type. However, for valuerefs we must make storage
            // to put the valueref in.
            t_arg_types[t_arg_index] = &ffi_type_pointer;
            
            if (t_descriptor != nil)
                t_args[t_arg_index] = t_argument;
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
    if (t_success)
    {
        uint8_t t_result[64];
        ffi_call((ffi_cif *)p_handler -> function_cif, (void(*)())p_handler -> function, &t_result, t_args);
        
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
                    if (!MCForeignValueCreateAndRelease(t_resolved_return_type . named_type, t_result, (MCForeignValueRef&)t_result_value))
                        t_success = false;
                }
                else
                    t_result_value = *(MCValueRef *)t_result;
            }
            else
                t_result_value = MCValueRetain(kMCNull);
        }
    }
    else
        t_result_value = nil;
    
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
                    if (!MCForeignValueCreateAndRelease(t_types[i] . type, t_args[i], (MCForeignValueRef&)t_out_values[i]))
                    {
                        // If that failed, finalize the contents.
                        t_descriptor -> finalize(t_args[i]);
                        t_success = false;
                        t_out_arg_index = i;
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
    
    if (t_success)
    {
        // If we get here, then we can go through and assign things to registers.
        for(uindex_t i = 0; i < p_arity; i++)
            if (t_out_values[i] != nil)
            {
                MCScriptStoreToRegisterInFrame(x_frame, p_arguments[i], t_out_values[i]);
                MCValueRelease(t_out_values[i]);
            }
        
        // If there is a result, then store it.
        MCScriptStoreToRegisterInFrame(x_frame, t_result_reg, t_result_value);
        MCValueRelease(t_result_value);
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
		
		return MCScriptPerformForeignInvoke(x_frame, p_instance, t_foreign_handler, p_arguments, p_arity);
	}
	
	/* LOAD CHECK */ __MCScriptUnreachable__("non-handler definition passed to invoke");
    
    return false;
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
                t_offset = (t_arguments[0] & 1) != 0 ? -(t_arguments[0] >> 1) : t_arguments[0] >> 1;
                
                // <offset> is relative to the start of this instruction.
                t_next_bytecode = t_bytecode + t_offset;
            }
            break;
            case kMCScriptBytecodeOpJumpIfTrue:
            {
                // jumpiftrue <register>, <offset>
                int t_register, t_offset;
                t_register = t_arguments[0];
                t_offset = (t_arguments[1] & 1) != 0 ? -(t_arguments[1] >> 1) : t_arguments[1] >> 1;
                
                // if the value in the register is true, then jump.
                MCValueRef t_value;
                t_value = MCScriptFetchFromRegisterInFrame(t_frame, t_register);
                
                if (MCValueGetTypeCode(t_value) == kMCValueTypeCodeBoolean)
                {
                    if (t_value == kMCTrue)
                        t_next_bytecode = t_bytecode + t_offset;
                }
                else
                    t_success = MCScriptThrowNotABooleanError(t_frame -> instance -> module, t_bytecode - t_frame -> instance -> module -> bytecode, t_value);
            }
            break;
            case kMCScriptBytecodeOpJumpIfFalse:
            {
                // jumpiffalse <register>, <offset>
                int t_register, t_offset;
                t_register = t_arguments[0];
                t_offset = (t_arguments[1] & 1) != 0 ? -(t_arguments[1] >> 1) : t_arguments[1] >> 1;
                
                // if the value in the register is true, then jump.
                MCValueRef t_value;
                t_value = MCScriptFetchFromRegisterInFrame(t_frame, t_register);
                
                if (MCValueGetTypeCode(t_value) == kMCValueTypeCodeBoolean)
                {
                    if (t_value == kMCFalse)
                        t_next_bytecode = t_bytecode + t_offset;
                }
                else
                    t_success = MCScriptThrowNotABooleanError(t_frame -> instance -> module, t_bytecode - t_frame -> instance -> module -> bytecode, t_value);
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
                
                // Store the constant in the frame.
                MCScriptStoreToRegisterInFrame(t_frame, t_dst, t_value);
            }
            break;
            case kMCScriptBytecodeOpAssign:
            {
                // assign <dst>, <src>
                int t_dst, t_src;
                t_dst = t_arguments[0];
                t_src = t_arguments[1];
                
                MCValueRef t_value;
                t_value = MCScriptFetchFromRegisterInFrame(t_frame, t_src);
                MCScriptStoreToRegisterInFrame(t_frame, t_dst, t_value);
            }
            break;
            case kMCScriptBytecodeOpReturn:
            {
                // return <reg>
                int t_reg;
                t_reg = t_arguments[0];
                
                // Fetch the value of the result.
                MCValueRef t_value;
                t_value = MCScriptFetchFromRegisterInFrame(t_frame, t_reg);
                
                // Fetch the signature of the current handler.
                MCTypeInfoRef t_signature;
                t_signature = self -> module -> types[t_frame -> handler -> type] -> typeinfo;
                
                // Check the return type is correct.
                if (!MCTypeInfoConforms(MCValueGetTypeInfo(t_value), MCHandlerTypeInfoGetReturnType(t_signature)))
                    t_success = MCScriptThrowInvalidValueForResultError(t_frame -> instance -> module, t_frame -> instance -> module -> bytecode - t_bytecode, MCHandlerTypeInfoGetReturnType(t_signature), t_value);
                
                // Check that out parameters are defined.
                for(uindex_t i = 0; t_success && i < MCHandlerTypeInfoGetParameterCount(t_signature); i++)
                    if (MCHandlerTypeInfoGetParameterMode(t_signature, i) == kMCHandlerTypeFieldModeOut)
                        if (MCScriptFetchFromLocalInFrame(t_frame, i) == kMCNull &&
                            !MCScriptIsLocalInFrameOptional(t_frame, i))
                            t_success = MCScriptThrowOutParameterNotDefinedError(t_frame -> instance -> module, t_frame -> instance -> module -> bytecode - t_bytecode, i);
                
                // If we get here then everything is conforming.
                if (t_success)
                {
                    if (t_frame -> caller == nil)
                    {
                        // Set the result value argument.
                        r_value = MCValueRetain(t_value);
                        
                        for(uindex_t i = 0; i < MCHandlerTypeInfoGetParameterCount(t_signature); i++)
                            if (MCHandlerTypeInfoGetParameterMode(t_signature, i) != kMCHandlerTypeFieldModeIn)
                                p_arguments[i] = MCValueRetain(MCScriptFetchFromLocalInFrame(t_frame, i));
                    }
                    else
                    {
                        // Store the result in the appropriate reg in the caller.
                        MCScriptStoreToRegisterInFrame(t_frame -> caller, t_frame -> result, t_value);
                        
                        if (t_frame -> mapping != nil)
                            for(uindex_t i = 0; i < MCHandlerTypeInfoGetParameterCount(t_signature); i++)
                                if (MCHandlerTypeInfoGetParameterMode(t_signature, i) != kMCHandlerTypeFieldModeIn)
                                    MCScriptStoreToRegisterInFrame(t_frame -> caller, t_frame -> mapping[i], MCScriptFetchFromLocalInFrame(t_frame, i));
                        
                        // Update the bytecode pointer to that of the caller.
                        t_next_bytecode = t_frame -> caller -> instance -> module -> bytecode + t_frame -> caller -> address;
                    }
                }
                
                // Pop and destroy the top frame of the stack.
                t_frame = MCScriptDestroyFrame(t_frame);
            }
            break;
            case kMCScriptBytecodeOpInvoke:
            case kMCScriptBytecodeOpInvokeEvaluate:
            case kMCScriptBytecodeOpInvokeAssign:
            {
                // invoke <index>, <result>, <arg_1>, ..., <arg_n>
                int t_index;
                t_index = t_arguments[0];
				
				MCScriptInstanceRef t_instance;
                MCScriptDefinition *t_definition;
                MCScriptResolveDefinitionInFrame(t_frame, t_index, t_instance, t_definition);

				t_success = MCScriptPerformInvoke(t_frame, t_next_bytecode, t_instance, t_definition, t_arguments + 1, t_arity - 1);
            }
            break;
            case kMCScriptBytecodeOpInvokeIndirect:
            {
                // invoke *<src>, <result>, <arg_1>, ..., <arg_n>
				int t_src;
				t_src = t_arguments[0];
				
				MCValueRef t_handler;
				t_handler = MCScriptFetchFromRegisterInFrame(t_frame, t_src);
				
				__MCScriptAssert__(MCValueGetTypeCode(t_handler) == kMCValueTypeCodeHandler,
									"handler argument to invoke not a handler");
				
				MCScriptInstanceRef t_instance;
				MCScriptDefinition *t_definition;
				t_instance = (MCScriptInstanceRef)MCHandlerGetInstance((MCHandlerRef)t_handler);
				t_definition = (MCScriptDefinition *)MCHandlerGetDefinition((MCHandlerRef)t_handler);
				
				t_success = MCScriptPerformInvoke(t_frame, t_next_bytecode, t_instance, t_definition, t_arguments + 1, t_arity - 1);
            }
            break;
            case kMCScriptBytecodeOpFetchLocal:
            {
                // fetch-local <dst>, <index>
                int t_dst, t_index;
                t_dst = t_arguments[0];
                t_index = t_arguments[1];
                
                MCValueRef t_value;
                t_value = MCScriptFetchFromLocalInFrame(t_frame, t_index);
                
                // Check that the local is defined if not optional
                if (t_value == kMCNull &&
                    !MCScriptIsLocalInFrameOptional(t_frame, t_index))
                    t_success = MCScriptThrowLocalVariableUsedBeforeDefinedError(t_frame -> instance -> module, t_bytecode - t_frame -> instance -> module -> bytecode, t_index);
                
                if (t_success)
                    MCScriptStoreToRegisterInFrame(t_frame, t_dst, t_value);
            }
            break;
            case kMCScriptBytecodeOpStoreLocal:
            {
                // store-local <src>, <index>
                int t_dst, t_index;
                t_dst = t_arguments[0];
                t_index = t_arguments[1];
                
                MCValueRef t_value;
                t_value = MCScriptFetchFromRegisterInFrame(t_frame, t_dst);
                
                if (!MCScriptCanLocalInFrameHoldValue(t_frame, t_index, t_value))
                    t_success = MCScriptThrowInvalidValueForLocalVariableError(t_frame -> instance -> module, t_bytecode - t_frame -> instance -> module -> bytecode, t_index, t_value);
                
                if (t_success)
                    MCScriptStoreToLocalInFrame(t_frame, t_index, t_value);
            }
            break;
            case kMCScriptBytecodeOpFetchGlobal:
            {
                // fetch-global <dst>, <index>
                int t_dst, t_index;
                t_dst = t_arguments[0];
                t_index = t_arguments[1];
                
				MCScriptInstanceRef t_instance;
                MCScriptDefinition *t_definition;
                MCScriptResolveDefinitionInFrame(t_frame, t_index, t_instance, t_definition);
                
                __MCScriptAssert__(t_definition -> kind == kMCScriptDefinitionKindVariable,
                                   "definition not a variable in fetch global");
                
                MCScriptVariableDefinition *t_var_definition;
                t_var_definition = static_cast<MCScriptVariableDefinition *>(t_definition);
                
                MCValueRef t_value;
                t_value = t_instance -> slots[t_var_definition -> slot_index];
                
                if (t_value == kMCNull &&
                    !MCTypeInfoIsOptional(t_instance -> module -> types[t_var_definition -> type] -> typeinfo))
                    t_success = MCScriptThrowGlobalVariableUsedBeforeDefinedError(t_frame -> instance -> module, t_bytecode - t_frame -> instance -> module -> bytecode, t_index);
                
                if (t_success)
                    MCScriptStoreToRegisterInFrame(t_frame, t_dst, t_value);
            }
            break;
            case kMCScriptBytecodeOpStoreGlobal:
            {
                // store-global <src>, <index>
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
                
                MCValueRef t_value;
                t_value = MCScriptFetchFromRegisterInFrame(t_frame, t_dst);
                
                if (!MCTypeInfoConforms(MCValueGetTypeInfo(t_value), t_instance -> module -> types[t_var_definition -> type] -> typeinfo))
                    t_success = MCScriptThrowInvalidValueForGlobalVariableError(t_frame -> instance -> module, t_bytecode - t_frame -> instance -> module -> bytecode, t_index, t_value);
                
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
        }
        
        if (!t_success)
            break;
        
        if (t_frame == nil)
            break;
        
        // Move to the next instruction.
        t_bytecode = t_next_bytecode;
    }
    
    // Copy return value (if any).
    if (!t_success)
        while(t_frame != nil)
            t_frame = MCScriptDestroyFrame(t_frame);
    
    return t_success;
}

////////////////////////////////////////////////////////////////////////////////

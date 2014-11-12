#include "script.h"
#include "script-private.h"

////////////////////////////////////////////////////////////////////////////////

bool MCScriptCreateInstanceOfModule(MCScriptModuleRef p_module, MCScriptInstanceRef& r_instance)
{
    bool t_success;
    t_success = true;
    
    MCScriptInstanceRef t_instance;
    t_instance = nil;
    
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

bool MCHandlerTypeInfoConformsToPropertyGetter(MCTypeInfoRef typeinfo);
bool MCHandlerTypeInfoConformsToPropertySetter(MCTypeInfoRef typeinfo);

bool MCScriptThrowAttemptToSetReadOnlyPropertyError(MCScriptModuleRef module, MCNameRef property);
bool MCScriptThrowInvalidValueForPropertyError(MCScriptModuleRef module, MCNameRef property, MCTypeInfoRef type, MCValueRef value);
bool MCScriptThrowWrongNumberOfArgumentsForHandlerError(MCScriptModuleRef module, MCNameRef handler, uindex_t expected, uindex_t provided);
bool MCScriptThrowValueProvidedForOutParameterError(MCScriptModuleRef module, MCNameRef handler, MCNameRef parameter);
bool MCScriptThrowNoValueProvidedForInParameterError(MCScriptModuleRef module, MCNameRef handler, MCNameRef parameter);
bool MCScriptThrowInvalidValueForParameterError(MCScriptModuleRef module, MCNameRef handler, MCNameRef parameter, MCTypeInfoRef type, MCValueRef value);
bool MCScriptThrowTypecheckFailureError(MCScriptModuleRef module, uindex_t address, MCTypeInfoRef type, MCValueRef value);
bool MCScriptThrowOutOfMemoryError(MCScriptModuleRef module, uindex_t address);

bool MCScriptGetPropertyOfInstance(MCScriptInstanceRef self, MCNameRef p_property, MCValueRef& r_value)
{
    __MCScriptValidateObjectAndKind__(self, kMCScriptObjectKindInstance);
    
    // Lookup the definition (throws if not found).
    MCScriptPropertyDefinition *t_definition;
    if (!MCScriptLookupPropertyDefinitionInModule(self -> module, p_property, t_definition))
        return false;
    
    MCScriptDefinition *t_getter;
    t_getter = t_definition -> getter != 0 ? self -> module -> definitions[t_definition -> getter] : nil;
    
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
        
        /* LOAD CHECK */ __MCScriptAssert__(MCHandlerTypeInfoConformsToPropertyGetter(t_handler_def -> signature),
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
    t_setter = t_definition -> setter != 0 ? self -> module -> definitions[t_definition -> setter] : nil;
    
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
        
        // Make sure the value is of the correct type - if not it is an error.
        // (The caller has to ensure things are converted as appropriate).
        if (!MCTypeInfoConforms(MCValueGetTypeInfo(p_value), t_variable_def -> type))
            return MCScriptThrowInvalidValueForPropertyError(self -> module, p_property, t_variable_def -> type, p_value);
        
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
        
        /* LOAD CHECK */ __MCScriptAssert__(MCHandlerTypeInfoConformsToPropertySetter(t_handler_def -> signature),
                                            "incorrect signature for property setter");
        
        // Get the required type of the parameter.
        MCTypeInfoRef t_property_type;
        t_property_type = MCHandlerTypeInfoGetParameterType(t_handler_def -> signature, 0);
        
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
    t_signature = t_definition -> signature;
    
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
                return MCScriptThrowNoValueProvidedForInParameterError(self -> module, p_handler, MCHandlerTypeInfoGetParameterName(t_signature, i));
            
            MCTypeInfoRef t_type;
            t_type = MCHandlerTypeInfoGetParameterType(t_signature, i);
            
            if (!MCTypeInfoConforms(MCValueGetTypeInfo(p_arguments[i]), t_type))
                return MCScriptThrowInvalidValueForParameterError(self -> module, p_handler, MCHandlerTypeInfoGetParameterName(t_signature, i), t_type, p_arguments[i]);
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
    //   <return parameter> (if returntype defined)
    //   <parameters>
    //   <variables>
    //   <temporaries>
    MCValueRef *slots;
	
	// The mapping array - this lists the mapping from parameters to registers
	// in the callers frame, for handling inout/out parameters.
	int *mapping;
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
    
    self -> caller = p_caller;
    self -> instance = MCScriptRetainInstance(p_instance);
    self -> handler = p_handler;
    self -> address = p_handler -> address;
    
    r_frame = self;
    
    return true;
}

static MCScriptFrame *MCScriptDestroyFrame(MCScriptFrame *self)
{
    MCScriptFrame *t_caller;
    t_caller = self -> caller;
    
    for(int i = 0; i < self -> handler -> slot_count; i++)
        if (self -> slots[i] != nil)
            MCValueRelease(self -> slots[i]);
    
    MCScriptReleaseInstance(self -> instance);
    MCMemoryDeleteArray(self -> slots);
    MCMemoryDeleteArray(self -> mapping);
    MCMemoryDelete(self);
    
    return t_caller;
}

static inline void MCScriptBytecodeDecodeOp(byte_t*& x_bytecode_ptr, MCScriptBytecodeOp& r_op, int& r_arity)
{
    byte_t t_op_byte;
	t_op_byte = *x_bytecode_ptr++;
	
	// The lower nibble is the bytecode operation.
	MCScriptBytecodeOp t_op;
	t_op = (MCScriptBytecodeOp)(t_op & 0xf);
	
	// The upper nibble is the arity.
	int t_arity;
	t_arity = (t_op_byte >> 4);
	
	// If the arity is 15, then overflow to a subsequent byte.
	if (t_arity == 15)
		t_arity += *x_bytecode_ptr++;
}

// TODO: Make this better for negative numbers.
static inline int MCScriptBytecodeDecodeArgument(byte_t*& x_bytecode_ptr)
{
    int t_value;
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

static inline MCValueRef MCScriptFetchFromRegisterInFrame(MCScriptFrame *p_frame, int p_register)
{
    __MCScriptAssert__(p_register >= 0 && p_register < p_frame -> handler -> slot_count,
                       "register out of range on fetch");
    return p_frame -> slots[p_register];
}

static inline void MCScriptStoreToRegisterInFrame(MCScriptFrame *p_frame, int p_register, MCValueRef p_value)
{
    __MCScriptAssert__(p_register >= 0 && p_register < p_frame -> handler -> slot_count,
                       "register out of range on store");
    if  (p_frame -> slots[p_register] != p_value)
    {
        MCValueRelease(p_frame -> slots[p_register]);
        p_frame -> slots[p_register] = MCValueRetain(p_value);
    }
}

static inline MCValueRef MCScriptFetchFromGlobalInFrame(MCScriptFrame *p_frame, int p_index)
{
    __MCScriptAssert__(p_index >= 0 && p_index < p_frame -> instance -> module -> slot_count,
                       "global out of range on fetch");
    return p_frame -> instance -> slots[p_index];
}

static inline void MCScriptStoreToGlobalInFrame(MCScriptFrame *p_frame, int p_index, MCValueRef p_value)
{
    __MCScriptAssert__(p_index >= 0 && p_index < p_frame -> instance -> module -> slot_count,
                       "global out of range on fetch");
    if  (p_frame -> instance -> slots[p_index] != p_value)
    {
        MCValueRelease(p_frame -> instance -> slots[p_index]);
        p_frame -> instance -> slots[p_index] = MCValueRetain(p_value);
    }
}

static inline MCValueRef MCScriptFetchConstantInFrame(MCScriptFrame *p_frame, int p_index)
{
    __MCScriptAssert__(p_index >= 0 && p_index < p_frame -> instance -> module -> value_count,
                       "constant out of range on fetch");
    return p_frame -> instance -> module -> values[p_index];
}

static inline MCTypeInfoRef MCScriptFetchTypeInFrame(MCScriptFrame *p_frame, int p_index)
{
    MCValueRef t_value;
    t_value = MCScriptFetchConstantInFrame(p_frame, p_index);
    __MCScriptAssert__(MCValueGetTypeCode(t_value) == kMCValueTypeCodeTypeInfo,
                       "incorrect type of constant when fetched from pool");
    return (MCTypeInfoRef)t_value;
}

static bool MCScriptPerformScriptInvoke(MCScriptFrame*& x_frame, byte_t*& x_next_bytecode, MCScriptInstanceRef p_instance, MCScriptHandlerDefinition *p_handler, int *p_arguments, int p_arity)
{
    __MCScriptAssert__(p_arity == MCHandlerTypeInfoGetParameterCount(p_handler -> signature),
                       "wrong number of parameters passed to script handler");
    
    MCScriptFrame *t_callee;
    if (!MCScriptCreateFrame(x_frame, p_instance, p_handler, t_callee))
        return false;

    bool t_needs_mapping;
    t_needs_mapping = false;
    for(int i = 0; i < MCHandlerTypeInfoGetParameterCount(p_handler -> signature); i++)
    {
        MCHandlerTypeFieldMode t_mode;
        t_mode = MCHandlerTypeInfoGetParameterMode(p_handler -> signature, i);
        
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
    
    x_frame = t_callee;
    x_next_bytecode = x_frame -> instance -> module -> bytecode + x_frame -> address;
    
	return true;
}

static bool MCScriptPerformForeignInvoke(MCScriptFrame*& x_frame, MCScriptInstanceRef p_instance, MCScriptForeignHandlerDefinition *p_handler, int *p_arguments, int p_arity)
{
	return false;
}

static bool MCScriptPerformInvoke(MCScriptFrame*& x_frame, byte_t*& x_next_bytecode, MCScriptInstanceRef p_instance, MCScriptDefinition *p_handler, int *p_arguments, int p_arity)
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
	
	__MCScriptUnreachable__("non-handler definition passed to invoke");
    
    return false;
}

bool MCScriptCallHandlerOfInstanceInternal(MCScriptInstanceRef self, MCScriptHandlerDefinition *p_handler, MCValueRef *p_arguments, uindex_t p_argument_count, MCValueRef& r_value)
{
    // As this method is called internally, we can be sure that the arguments conform
    // to the signature so in theory don't need to check here.
    
    // TODO: Add static assertion for the above.
    
    MCScriptFrame *t_frame;
    if (!MCScriptCreateFrame(nil, self, p_handler, t_frame))
        return false;

    // After we've created the frame we need to populate the parameter slots with the
    // arguments. Internally, all script-based handlers are adjusted so that if the
    // handler is declared as returning a value, this is the first register of the
    // frame.
    int t_first_parameter_slot;
    t_first_parameter_slot = 0;
    if (MCHandlerTypeInfoGetReturnType(p_handler -> signature) != nil)
    {
        t_frame -> slots[0] = MCValueRetain(kMCNull);
        t_first_parameter_slot = 1;
    }
    for(uindex_t i = 0; i < p_argument_count; i++)
    {
        MCValueRef t_value;
        if (MCHandlerTypeInfoGetParameterMode(p_handler -> signature, i) != kMCHandlerTypeFieldModeOut)
            t_value = MCValueRetain(p_arguments[i]);
        else
            t_value = MCValueRetain(kMCNull);

        t_frame -> slots[t_first_parameter_slot + i] = t_value;
    }
    
    bool t_success;
    t_success = true;
    
    // This is used to build the mapping array for invokes.
	int t_arguments[256];
	
    byte_t *t_bytecode;
    t_bytecode = t_frame -> instance -> module -> bytecode + t_frame -> address;
    for(;;)
    {
        byte_t *t_next_bytecode;
        t_next_bytecode = t_bytecode;
        
        MCScriptBytecodeOp t_op;
		int t_arity;
        MCScriptBytecodeDecodeOp(t_next_bytecode, t_op, t_arity);
		
		for(int i = 0; i < t_arity; i++)
			t_arguments[i] = MCScriptBytecodeDecodeArgument(t_next_bytecode);
		
        switch(t_op)
        {
            case kMCScriptBytecodeOpNone:
                // Surprisingly, this opcode does absolutely nothing.
                break;
            case kMCScriptBytecodeOpJump:
            {
                // jump <offset>
                int t_offset;
                t_offset = t_arguments[0];
                
                // <offset> is relative to the start of this instruction.
                t_next_bytecode = t_bytecode + t_offset;
            }
            break;
            case kMCScriptBytecodeOpJumpIfUndefined:
            {
                // jumpifundef <register>, <offset>
                int t_register, t_offset;
                t_register = t_arguments[0];
                t_offset = t_arguments[1];
                
                // if the value in the register is undefined, then jump.
                if (MCScriptFetchFromRegisterInFrame(t_frame, t_register) == kMCNull)
                    t_next_bytecode = t_bytecode + t_offset;
            }
            break;
            case kMCScriptBytecodeOpJumpIfDefined:
            {
                // jumpifdef <register>, <offset>
                int t_register, t_offset;
                t_register = t_arguments[0];
                t_offset = t_arguments[1];
                
                // if the value in the register is not undefined, then jump.
                if (MCScriptFetchFromRegisterInFrame(t_frame, t_register) != kMCNull)
                    t_next_bytecode = t_bytecode + t_offset;
            }
            break;
            case kMCScriptBytecodeOpJumpIfTrue:
            {
                // jumpiftrue <register>, <offset>
                int t_register, t_offset;
                t_register = t_arguments[0];
                t_offset = t_arguments[1];
                
                // if the value in the register is true, then jump.
                MCValueRef t_value;
                t_value = MCScriptFetchFromRegisterInFrame(t_frame, t_register);
                
                __MCScriptAssert__(MCValueGetTypeCode(t_value) == kMCValueTypeCodeBoolean,
                                   "jumpiftrue argument not a boolean");
                
                if (t_value == kMCTrue)
                    t_next_bytecode = t_bytecode + t_offset;
            }
            break;
            case kMCScriptBytecodeOpJumpIfFalse:
            {
                // jumpiffalse <register>, <offset>
                int t_register, t_offset;
                t_register = t_arguments[0];
                t_offset = t_arguments[1];
                
                // if the value in the register is true, then jump.
                MCValueRef t_value;
                t_value = MCScriptFetchFromRegisterInFrame(t_frame, t_register);
                
                __MCScriptAssert__(MCValueGetTypeCode(t_value) == kMCValueTypeCodeBoolean,
                                   "jumpiffalse argument not a boolean");
                
                if (t_value == kMCFalse)
                    t_next_bytecode = t_bytecode + t_offset;
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
                
                __MCScriptAssert__(t_dst == t_src,
                                   "src and dst registers same in assign");
                
                MCValueRef t_value;
                t_value = MCScriptFetchFromRegisterInFrame(t_frame, t_src);
                MCScriptStoreToRegisterInFrame(t_frame, t_dst, t_value);
            }
            break;
            case kMCScriptBytecodeOpTypecheck:
            {
                // typecheck <reg>, <typeinfo>
                int t_register, t_type_index;
                t_register = t_arguments[0];
                t_type_index = t_arguments[1];
                
                // Fetch the value from the frame
                MCValueRef t_value;
                t_value = MCScriptFetchFromRegisterInFrame(t_frame, t_register);
                
                // Fetch the typeinfo from the frame (constant pool)
                MCTypeInfoRef t_type;
                t_type = MCScriptFetchTypeInFrame(t_frame, t_type_index);
                
                // If the value's typeinfo doesn't conform, its an error.
                if (!MCTypeInfoConforms(MCValueGetTypeInfo(t_value), t_type))
                {
                    t_frame -> address = t_bytecode - t_frame -> instance -> module -> bytecode;
                    t_success = MCScriptThrowTypecheckFailureError(t_frame -> instance -> module, t_frame -> address, t_type, t_value);
                }
            }
            break;
            case kMCScriptBytecodeOpReturn:
            {
                // return
                
                // If there is no frame to return to, fetch the return value and copyback any
                // out parameters.
                if (t_frame -> caller == nil)
                {
                    if (t_first_parameter_slot != 0)
                    {
                        __MCScriptAssert__(MCTypeInfoConforms(MCValueGetTypeInfo(t_frame -> slots[0]), MCHandlerTypeInfoGetReturnType(t_frame -> handler -> signature)),
                                           "return value type mismatch");
                        
                        // Set the result value argument.
                        r_value = t_frame -> slots[0];
                        
                        // Mark the slot as free as the value has been taken.
                        t_frame -> slots[0] = nil;
                    }
                    
					int t_param_count;
					t_param_count = MCHandlerTypeInfoGetParameterCount(t_frame -> handler -> signature);
                    for(int i = 0; i < t_param_count; i++)
						if (MCHandlerTypeInfoGetParameterMode(t_frame -> handler -> signature, i) != kMCHandlerTypeFieldModeIn)
						{
                            __MCScriptAssert__(MCTypeInfoConforms(MCValueGetTypeInfo(t_frame -> slots[t_first_parameter_slot + 1]), MCHandlerTypeInfoGetParameterType(t_frame -> handler -> signature, i)),
                                               "out parameter value type mismatch");
                            
                            // Move the value from the slot to the arguments array.
                            p_arguments[i] = t_frame -> slots[t_first_parameter_slot + 1];
                            
                            // Mark the slot as nil as the value has been taken.
                            t_frame -> slots[t_first_parameter_slot + 1] = nil;
                        }
                }
				else
                {
                    // If there is a mapping array the do the copyback.
                    if (t_frame -> mapping != nil)
                    {
                        int t_param_count;
                        t_param_count = MCHandlerTypeInfoGetParameterCount(t_frame -> handler -> signature);
                        for(int i = 0; i < t_param_count; i++)
                            if (MCHandlerTypeInfoGetParameterMode(t_frame -> handler -> signature, i) != kMCHandlerTypeFieldModeIn)
                            {
                                // Assign the return value to the mapped register.
                                MCScriptStoreToRegisterInFrame(t_frame -> caller, t_frame -> mapping[i], t_frame -> slots[i]);
                                
                                // Mark the slot as free as the value has been taken.
                                t_frame -> slots[i] = nil;
                            }
                    }
                    
                    // Update the bytecode pointer to that of the caller.
                    t_next_bytecode = t_frame -> caller -> instance -> module -> bytecode + t_frame -> caller -> address;
                }
                
                // Pop and destroy the top frame of the stack.
                t_frame = MCScriptDestroyFrame(t_frame);
            }
            break;
            case kMCScriptBytecodeOpInvoke:
            {
                // invoke <index>, <arg_1>, ..., <arg_n>
                int t_index;
                t_index = t_arguments[0];
				
				MCScriptInstanceRef t_instance;
				MCScriptDefinition *t_definition;
				MCScriptResolveDefinitionInModule(t_frame -> instance -> module, t_index, t_instance, t_definition);
				
				t_success = MCScriptPerformInvoke(t_frame, t_next_bytecode, t_instance, t_definition, t_arguments + 1, t_arity - 1);
            }
            break;
            case kMCScriptBytecodeOpInvokeIndirect:
            {
                // invoke *<src>, <arg_1>, ..., <arg_n>
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
            case kMCScriptBytecodeOpFetchGlobal:
            {
                // fetch <dst>, <index>
                int t_dst, t_index;
                t_dst = t_arguments[0];
                t_index = t_arguments[1];
                
                MCValueRef t_value;
                t_value = MCScriptFetchFromGlobalInFrame(t_frame, t_index);
                MCScriptStoreToRegisterInFrame(t_frame, t_dst, t_value);
            }
            break;
            case kMCScriptBytecodeOpStoreGlobal:
            {
                // store <dst>, <index>
                int t_dst, t_index;
                t_dst = t_arguments[0];
                t_index = t_arguments[1];
                
                MCValueRef t_value;
                t_value = MCScriptFetchFromRegisterInFrame(t_frame, t_dst);
                MCScriptStoreToGlobalInFrame(t_frame, t_index, t_value);
            }
            break;
            default:
                __MCScriptUnreachable__("invalid bytecode op encountered");
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

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

bool MCScriptGetPropertyOfInstance(MCScriptInstanceRef self, MCNameRef p_property, MCValueRef& r_value)
{
    __MCScriptValidateObjectAndKind__(self, kMCScriptObjectKindInstance);
    
    // Lookup the definition (throws if not found).
    MCScriptPropertyDefinition *t_definition;
    if (!MCScriptLookupPropertyDefinitionInModule(self -> module, p_property, t_definition))
        return false;
    
    /* LOAD CHECK */ __MCScriptAssert__(t_definition -> getter != nil,
                                            "property has no getter");
    /* LOAD CHECK */ __MCScriptAssert__(t_definition -> getter -> kind == kMCScriptDefinitionKindVariable ||
                                            t_definition -> getter -> kind == kMCScriptDefinitionKindHandler,
                                            "property getter is not a variable or handler");
    
    if (t_definition -> getter -> kind == kMCScriptDefinitionKindVariable)
    {
        // The easy case - fetching a variable-based property.
        
        MCScriptVariableDefinition *t_variable_def;
        t_variable_def = MCScriptDefinitionAsVariable(t_definition -> getter);
        
        // Variables are backed by an slot in the instance.
        uindex_t t_slot_index;
        t_slot_index = t_variable_def -> slot_index;
        
        /* COMPUTE CHECK */ __MCScriptAssert__(t_slot_index < self -> module -> slot_count,
                                               "computed variable slot out of range");
        
        // Slot based properties are easy, we just copy the value out of the slot.
        r_value = MCValueRetain(self -> slots[t_slot_index]);
    }
    else if (t_definition -> getter -> kind == kMCScriptDefinitionKindHandler)
    {
        // The more difficult case - we have to execute a handler.
        
        MCScriptHandlerDefinition *t_handler_def;
        t_handler_def = MCScriptDefinitionAsHandler(t_definition -> getter);
        
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
    
    // If there is no setter for the property then this is an error.
    if (t_definition -> setter == nil)
        return MCScriptThrowAttemptToSetReadOnlyPropertyError(self -> module, p_property);
    
    /* LOAD CHECK */ __MCScriptAssert__(t_definition -> setter != nil,
                                        "property has no setter");
    /* LOAD CHECK */ __MCScriptAssert__(t_definition -> getter -> kind == kMCScriptDefinitionKindVariable ||
                                        t_definition -> getter -> kind == kMCScriptDefinitionKindHandler,
                                        "property setter is not a variable or handler");
    
    if (t_definition -> setter -> kind == kMCScriptDefinitionKindVariable)
    {
        // The easy case - storing a variable-based property.
        
        MCScriptVariableDefinition *t_variable_def;
        t_variable_def = MCScriptDefinitionAsVariable(t_definition -> setter);
        
        // Make sure the value is of the correct type - if not it is an error.
        // (The caller has to ensure things are converted as appropriate).
        if (!MCTypeInfoConformsTo(MCValueGetTypeInfo(p_value), t_variable_def -> type))
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
    else if (t_definition -> getter -> kind == kMCScriptDefinitionKindHandler)
    {
        // The more difficult case - we have to execute a handler.
        
        MCScriptHandlerDefinition *t_handler_def;
        t_handler_def = MCScriptDefinitionAsHandler(t_definition -> getter);
        
        /* LOAD CHECK */ __MCScriptAssert__(MCHandlerTypeInfoConformsToPropertySetter(t_handler_def -> signature),
                                            "incorrect signature for property setter");
        
        // Get the required type of the parameter.
        MCTypeInfoRef t_property_type;
        t_property_type = MCHandlerTypeInfoGetParameterType(t_handler_def -> signature, 0);
        
        // Make sure the value if of the correct type - if not it is an error.
        // (The caller has to ensure things are converted as appropriate).
        if (!MCTypeInfoConformsTo(MCValueGetTypeInfo(p_value), t_property_type))
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
        
        if (t_mode == kMCHandlerTypeFieldModeOut)
        {
            if (p_arguments[i] != nil)
                return MCScriptThrowValueProvidedForOutParameterError(self -> module, p_handler, MCHandlerTypeInfoGetParameterName(t_signature, i));
        }
        else
        {
            if (p_arguments[i] == nil)
                return MCScriptThrowNoValueProvidedForInParameterError(self -> module, p_handler, MCHandlerTypeInfoGetParameterName(t_signature, i));
            
            MCTypeInfoRef t_type;
            t_type = MCHandlerTypeInfoGetParameterType(t_signature, i);
            
            if (!MCTypeInfoConformsTo(MCValueGetTypeInfo(p_arguments[i]), t_type))
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
    
    // The slots for the current handler invocation. This is a list consisting
    // parameters, then local variables, then temporaries.
    MCValueRef *slots;
};

static bool MCScriptCreateFrame(MCScriptFrame *p_caller, MCScriptInstanceRef p_instance, MCScriptHandlerDefinition *p_handler, MCScriptFrame*& r_frame)
{
    return false;
}

static MCScriptFrame *MCScriptDestroyFrame(MCScriptFrame *p_current)
{
    return nil;
}

static inline MCScriptBytecodeOp MCScriptBytecodeDecodeOp(byte_t*& x_bytecode_ptr)
{
    MCScriptBytecodeOp t_op;
    t_op = (MCScriptBytecodeOp)*x_bytecode_ptr;
    x_bytecode_ptr += 1;
    return t_op;
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

static bool MCScriptPerformInvoke(MCScriptFrame*& x_frame, byte_t*& x_next_bytecode, MCScriptInstanceRef p_instance, int p_handler_index)
{
    return true;
}

bool MCScriptCallHandlerOfInstanceInternal(MCScriptInstanceRef self, MCScriptHandlerDefinition *p_handler, MCValueRef *p_arguments, uindex_t p_argument_count, MCValueRef& r_value)
{
    // As this method is called internally, we can be sure that the arguments conform
    // to the signature so don't need to check here.
    
    // TODO: Add static assertion for the above.
    
    MCScriptFrame *t_frame;
    if (!MCScriptCreateFrame(nil, self, p_handler, t_frame))
        return false;

    bool t_success;
    t_success = true;
    
    byte_t *t_bytecode;
    t_bytecode = t_frame -> instance -> module -> bytecode + t_frame -> address;
    for(;;)
    {
        byte_t *t_next_bytecode;
        t_next_bytecode = t_bytecode;
        
        MCScriptBytecodeOp t_op;
        t_op = MCScriptBytecodeDecodeOp(t_next_bytecode);
        switch(t_op)
        {
            case kMCScriptBytecodeOpNone:
                // Surprisingly, this opcode does absolutely nothing.
                break;
            case kMCScriptBytecodeOpJump:
            {
                // jump <offset>
                int t_offset;
                t_offset = MCScriptBytecodeDecodeArgument(t_next_bytecode);
                
                // <offset> is relative to the start of this instruction.
                t_next_bytecode = t_bytecode + t_offset;
            }
            break;
            case kMCScriptBytecodeOpJumpIfUndefined:
            {
                // jumpifdef <register>, <offset>
                int t_register, t_offset;
                t_register = MCScriptBytecodeDecodeArgument(t_next_bytecode);
                t_offset = MCScriptBytecodeDecodeArgument(t_next_bytecode);
                
                // if the value in the register is undefined, then jump.
                if (MCScriptFetchFromRegisterInFrame(t_frame, t_register) == kMCNull)
                    t_next_bytecode = t_bytecode + t_offset;
            }
            break;
            case kMCScriptBytecodeOpJumpIfDefined:
            {
                // jumpifdef <register>, <offset>
                int t_register, t_offset;
                t_register = MCScriptBytecodeDecodeArgument(t_next_bytecode);
                t_offset = MCScriptBytecodeDecodeArgument(t_next_bytecode);
                
                // if the value in the register is not undefined, then jump.
                if (MCScriptFetchFromRegisterInFrame(t_frame, t_register) != kMCNull)
                    t_next_bytecode = t_bytecode + t_offset;
            }
            break;
            case kMCScriptBytecodeOpJumpIfTrue:
            {
                // jumpiftrue <register>, <offset>
                int t_register, t_offset;
                t_register = MCScriptBytecodeDecodeArgument(t_next_bytecode);
                t_offset = MCScriptBytecodeDecodeArgument(t_next_bytecode);
                
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
                t_register = MCScriptBytecodeDecodeArgument(t_next_bytecode);
                t_offset = MCScriptBytecodeDecodeArgument(t_next_bytecode);
                
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
                t_dst = MCScriptBytecodeDecodeArgument(t_next_bytecode);
                t_constant_index = MCScriptBytecodeDecodeArgument(t_next_bytecode);
                
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
                t_dst = MCScriptBytecodeDecodeArgument(t_next_bytecode);
                t_src = MCScriptBytecodeDecodeArgument(t_next_bytecode);
                
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
                t_register = MCScriptBytecodeDecodeArgument(t_next_bytecode);
                t_type_index = MCScriptBytecodeDecodeArgument(t_next_bytecode);
                
                // Fetch the value from the frame
                MCValueRef t_value;
                t_value = MCScriptFetchFromRegisterInFrame(t_frame, t_register);
                
                // Fetch the typeinfo from the frame (constant pool)
                MCTypeInfoRef t_type;
                t_type = MCScriptFetchTypeInFrame(t_frame, t_type_index);
                
                // If the value's typeinfo doesn't conform, its an error.
                if (!MCTypeInfoConformsTo(MCValueGetTypeInfo(t_value), t_type))
                {
                    t_frame -> address = t_bytecode - t_frame -> instance -> module -> bytecode;
                    t_success = MCScriptThrowTypecheckFailureError(t_frame -> instance -> module, t_frame -> address, t_type, t_value);
                }
            }
            break;
            case kMCScriptBytecodeOpReturn:
            {
                // return
                
                // Pop and destroy the top frame of the stack.
                t_frame = MCScriptDestroyFrame(t_frame);
                
                // If there is still a frame, update the bytecode ptr.
                if (t_frame != nil)
                    t_next_bytecode = t_frame -> instance -> module -> bytecode + t_frame -> address;
            }
            break;
            case kMCScriptBytecodeOpInvoke:
            {
                // invoke <index>, <arg_1>, ..., <arg_n>
                int t_index;
                t_index = MCScriptBytecodeDecodeArgument(t_next_bytecode);
                t_success = MCScriptPerformInvoke(t_frame, t_next_bytecode, t_frame -> instance, t_index);
            }
            break;
            case kMCScriptBytecodeOpInvokeIndirect:
            {
            }
            break;
            case kMCScriptBytecodeOpFetchGlobal:
            {
                // fetch <dst>, <index>
                int t_dst, t_index;
                t_dst = MCScriptBytecodeDecodeArgument(t_next_bytecode);
                t_index = MCScriptBytecodeDecodeArgument(t_next_bytecode);
                
                MCValueRef t_value;
                t_value = MCScriptFetchFromGlobalInFrame(t_frame, t_index);
                MCScriptStoreToRegisterInFrame(t_frame, t_dst, t_value);
            }
            break;
            case kMCScriptBytecodeOpStoreGlobal:
            {
                // store <dst>, <index>
                int t_dst, t_index;
                t_dst = MCScriptBytecodeDecodeArgument(t_next_bytecode);
                t_index = MCScriptBytecodeDecodeArgument(t_next_bytecode);
                
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
    
    if (!t_success)
        while(t_frame != nil)
            t_frame = MCScriptDestroyFrame(t_frame);
    
    return t_success;
}

////////////////////////////////////////////////////////////////////////////////

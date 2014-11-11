#include "script.h"
#include "script-private.h"

#include <stddef.h>

////////////////////////////////////////////////////////////////////////////////

MC_PICKLE_BEGIN_RECORD(MCScriptExportedDefinition)
    MC_PICKLE_NAMEREF(name)
    MC_PICKLE_UINDEX(index)
MC_PICKLE_END_RECORD()

MC_PICKLE_BEGIN_RECORD(MCScriptImportedDefinition)
    MC_PICKLE_UINDEX(module)
    MC_PICKLE_INTENUM(MCScriptDefinitionKind, kind)
    MC_PICKLE_NAMEREF(name)
    MC_PICKLE_TYPEINFOREF(type)
MC_PICKLE_END_RECORD()

MC_PICKLE_BEGIN_RECORD(MCScriptExternalDefinition)
    MC_PICKLE_UINDEX(index)
MC_PICKLE_END_RECORD()

MC_PICKLE_BEGIN_RECORD(MCScriptTypeDefinition)
    MC_PICKLE_TYPEINFOREF(type)
MC_PICKLE_END_RECORD()

MC_PICKLE_BEGIN_RECORD(MCScriptConstantDefinition)
    MC_PICKLE_VALUEREF(value)
MC_PICKLE_END_RECORD()

MC_PICKLE_BEGIN_RECORD(MCScriptVariableDefinition)
    MC_PICKLE_TYPEINFOREF(type)
MC_PICKLE_END_RECORD()

MC_PICKLE_BEGIN_RECORD(MCScriptHandlerDefinition)
    MC_PICKLE_TYPEINFOREF(signature)
    MC_PICKLE_UINDEX(address)
MC_PICKLE_END_RECORD()

MC_PICKLE_BEGIN_RECORD(MCScriptForeignHandlerDefinition)
    MC_PICKLE_TYPEINFOREF(signature)
    MC_PICKLE_STRINGREF(binding)
MC_PICKLE_END_RECORD()

MC_PICKLE_BEGIN_RECORD(MCScriptPropertyDefinition)
    MC_PICKLE_UINDEX(getter)
    MC_PICKLE_UINDEX(setter)
MC_PICKLE_END_RECORD()

MC_PICKLE_BEGIN_RECORD(MCScriptEventDefinition)
    MC_PICKLE_TYPEINFOREF(signature)
MC_PICKLE_END_RECORD()

MC_PICKLE_BEGIN_VARIANT(MCScriptDefinition, kind)
    MC_PICKLE_VARIANT_CASE(kMCScriptDefinitionKindExternal, MCScriptExternalDefinition)
    MC_PICKLE_VARIANT_CASE(kMCScriptDefinitionKindType, MCScriptTypeDefinition)
    MC_PICKLE_VARIANT_CASE(kMCScriptDefinitionKindConstant, MCScriptConstantDefinition)
    MC_PICKLE_VARIANT_CASE(kMCScriptDefinitionKindVariable, MCScriptVariableDefinition)
    MC_PICKLE_VARIANT_CASE(kMCScriptDefinitionKindHandler, MCScriptHandlerDefinition)
    MC_PICKLE_VARIANT_CASE(kMCScriptDefinitionKindForeignHandler, MCScriptForeignHandlerDefinition)
    MC_PICKLE_VARIANT_CASE(kMCScriptDefinitionKindProperty, MCScriptPropertyDefinition)
    MC_PICKLE_VARIANT_CASE(kMCScriptDefinitionKindEvent, MCScriptEventDefinition)
MC_PICKLE_END_VARIANT()

MC_PICKLE_BEGIN_RECORD(MCScriptModule)
    MC_PICKLE_INTENUM(MCScriptModuleKind, kind)
    MC_PICKLE_NAMEREF(name)
    MC_PICKLE_ARRAY_OF_NAMEREF(dependencies, dependency_count)
    MC_PICKLE_ARRAY_OF_VALUEREF(values, value_count)
    MC_PICKLE_ARRAY_OF_RECORD(MCScriptImportedDefinition, imported_definitions, imported_definition_count)
    MC_PICKLE_ARRAY_OF_RECORD(MCScriptExportedDefinition, exported_definitions, exported_definition_count)
    MC_PICKLE_ARRAY_OF_VARIANT(MCScriptDefinition, definitions, definition_count)
    MC_PICKLE_ARRAY_OF_BYTE(bytecode, bytecode_count)
MC_PICKLE_END_RECORD()

////////////////////////////////////////////////////////////////////////////////

void MCScriptDestroyModule(MCScriptModuleRef self)
{
    __MCScriptValidateObjectAndKind__(self, kMCScriptObjectKindModule);
    
    // Delete the dependencies arrays (refs are part of value pool).
    MCMemoryDeleteArray(self -> dependencies);
    
    // Delete the exported definitions array (refs are part of value pool).
    MCMemoryDeleteArray(self -> exported_definitions);
    
    // Delete the imported definitions array (refs are part of value pool).
    MCMemoryDeleteArray(self -> imported_definitions);
    
    // Delete the definitions array (refs are part of the value pool).
    for(uindex_t i = 0; i < self -> definition_count; i++)
        MCMemoryDeallocate(self -> definitions[i]);
    MCMemoryDeleteArray(self -> definitions);
    
    // Delete the bytecode array.
    MCMemoryDeleteArray(self -> bytecode);
    
    // Delete the value pool.
    for(uindex_t i = 0; i < self -> value_count; i++)
        MCValueRelease(self -> values[i]);
    MCMemoryDeleteArray(self -> values);
}

////////////////////////////////////////////////////////////////////////////////

#if 0
static bool MCScriptReadModuleHeaderFromStream(MCScriptStreamRef stream, uint32_t& r_version, MCScriptModuleKind& r_kind)
{
    byte_t t_header[4];
    if (!MCScriptReadBytesFromStream(stream, 4, t_header))
        return MCScriptThrowLastError();
    
    if (t_header[0] != 'L' ||
        t_header[1] != 'C')
        return MCScriptThrowError(kMCScriptBadEncodedModuleError);
    
    if (t_header[2] != 0)
        return MCScriptThrowError(kMCScriptBadEncodedModuleError);

    if (t_header[3] >= kMCScriptModuleKind__Last)
        return MCScriptThrowError(kMCScriptBadEncodedModuleError);
    
    r_version = t_header[2];
    r_kind = (MCScriptModuleKind)t_header[3];
    
    return true;
}

static bool MCScriptReadIntegerValueFromStream(MCScriptStreamRef stream, MCValueRef& r_value)
{
    int32_t t_integer;
    if (!MCScriptReadMaxInt32FromStream(stream, t_integer))
        return MCScriptThrowLastError();
    
    if (!MCNumberCreateWithInteger(t_integer, (MCNumberRef&)r_value))
        return MCScriptThrowLastFoundationError();
    
    return true;
}

static bool MCScriptReadRealValueFromStream(MCScriptStreamRef stream, MCValueRef& r_value)
{
    byte_t t_double_bytes[8];
    if (!MCScriptReadBytesFromStream(stream, 8, t_double_bytes))
        return MCScriptThrowLastError();
    
    double t_double_value;
    MCMemoryCopy(&t_double_value, t_double_bytes, 8);
    
    if (!MCNumberCreateWithReal(t_double_value, (MCNumberRef&)r_value))
        return MCScriptThrowLastFoundationError();
    
    return true;
}

static bool MCScriptReadStringValueFromStream(MCScriptStreamRef stream, MCValueRef& r_value)
{
    bool t_success;
    t_success = true;
    
    uint32_t t_length;
    t_length = 0;
    if (t_success)
        t_success = MCScriptReadMaxUInt32FromStream(stream, t_length);
    
    byte_t *t_encoded_string;
    t_encoded_string = nil;
    if (t_success &&
        !MCMemoryNewArray(t_length, t_encoded_string))
        t_success = MCScriptThrowLastFoundationError();
    
    if (t_success)
        t_success = MCScriptReadBytesFromStream(stream, t_length, t_encoded_string);
    
    if (t_success &&
        !MCStringCreateWithBytesAndRelease(t_encoded_string, t_length, kMCStringEncodingUTF8, false, (MCStringRef&)r_value))
        t_success = MCScriptThrowLastFoundationError();
    
    if (!t_success)
    {
        MCMemoryDeleteArray(t_encoded_string);
        return MCScriptThrowLastError();
    }
    
    return true;
}

static bool MCScriptReadPooledValueFromStream(MCScriptStreamRef p_stream, MCValueRef *p_values, uindex_t p_value_count, MCValueRef& r_value)
{
    uint32_t t_index;
    if (!MCScriptReadMaxUInt32FromStream(stream, t_index))
        return MCScriptThrowLastError();
    
    if (t_index > p_value_count)
        return MCScriptThrowError(kMCScriptPooledValueOverflowError);
    
    if (p_values[t_index] == nil)
        return MCScriptThrowError(kMCScriptPooledValueUsedBeforeDefinedError);
    
    r_value = p_values[t_index];
    
    return true;
}

static bool MCScriptReadPooledStringValueFromStream(MCScriptStreamRef p_stream, MCValueRef *p_values, uindex_t p_value_count, MCStringRef& r_value)
{
    MCValueRef t_value;
    if (!MCScriptReadPooledValueFromStream(p_stream, p_values, p_value_count, t_value))
        return MCScriptThrowLastError();
    
    if (MCValueGetTypeCode(t_value) != kMCValueTypeCodeString)
        return MCScriptThrowError(kMCScriptPooledValueTypeMismatchError);
    
    r_value = (MCStringRef)t_value;
    
    return true;
}

static bool MCScriptReadPooledNameValueFromStream(MCScriptStreamRef p_stream, MCValueRef *p_values, uindex_t p_value_count, MCNameRef& r_value)
{
    MCValueRef t_value;
    if (!MCScriptReadPooledValueFromStream(p_stream, p_values, p_value_count, t_value))
        return MCScriptThrowLastError();
    
    if (MCValueGetTypeCode(t_value) != kMCValueTypeCodeName)
        return MCScriptThrowError(kMCScriptPooledValueTypeMismatchError);
    
    r_value = (MCNameRef)t_value;
    
    return true;
}

static bool MCScriptReadNameValueFromStream(MCScriptStreamRef stream, MCValueRef *p_values, uindex_t p_value_count, MCValueRef& r_value)
{
    MCStringRef t_string;
    if (!MCScriptReadPooledStringValueFromStream(stream, p_values, p_value_count, t_string))
        return MCScriptThrowLastError();
    
    if (!MCNameCreate(t_string, (MCNameRef&)r_value))
        return MCScriptThrowLastFoundationError();
    
    return true;
}

static bool MCScriptReadEnumTypeValueFromStream(MCScriptStreamRef stream, MCValueRef *p_values, uindex_t p_value_count, MCValueRef& r_value)
{
    bool t_success;
    t_success = true;
    
    uint32_t t_field_count;
    if (t_success)
        t_success = MCScriptReadMaxUInt32FromStream(stream, t_field_count);
    
    MCEnumTypeFieldInfo *t_fields;
    t_fields = nil;
    if (t_success &&
        !MCMemoryNewArray(t_field_count, t_fields))
        t_success = MCScriptThrowLastFoundationError();
    
    uindex_t t_read_field_count;
    for(t_read_field_count = 0; t_success && t_read_field_count < t_field_count; t_read_field_count++)
        t_success = MCScriptReadPooledNameValueFromStream(stream, p_values, p_value_count, t_fields[t_read_field_count] . name);
  
    if (t_success &&
        !MCEnumTypeCreate(t_fields, t_field_count, (MCTypeRef&)r_value))
        t_success = MCScriptThrowLastFoundationError();
    
    MCMemoryDeleteArray(t_fields);
    
    return t_success;
}

static bool MCScriptReadRecordTypeValueFromStream(MCScriptStreamRef stream, MCValueRef *p_values, uindex_t p_value_count, MCValueRef& r_value)
{
    return false;
}

static bool MCScriptReadHandlerTypeValueFromStream(MCScriptStreamRef stream, MCValueRef *p_values, uindex_t p_value_count, MCValueRef& r_value)
{
    return false;
}

static bool MCScriptReadValueFromStream(MCScriptStreamRef stream, MCValueRef *p_values, uindex_t p_value_count, MCValueRef& r_value)
{
    bool t_success;
    t_success = true;
    
    uint32_t t_value_kind;
    if (t_success)
        t_success = MCScriptReadMaxUInt32FromStream(stream, t_value_kind);
    
    MCValueRef t_value;
    switch(t_value_kind)
    {
        case kMCScriptEncodedValueKindNull:
            t_value = MCValueRetain(kMCNull);
            break;
        case kMCScriptEncodedValueKindTrue:
            t_value = MCValueRetain(kMCTrue);
            break;
        case kMCScriptEncodedValueKindFalse:
            t_value = MCValueRetain(kMCFalse);
            break;
        case kMCScriptEncodedValueKindZero:
            t_value = MCValueRetain(kMCZero);
            break;
        case kMCScriptEncodedValueKindOne:
            t_value = MCValueRetain(kMCOne);
            break;
        case kMCScriptEncodedValueKindMinusOne:
            t_value = MCValueRetain(kMCMinusOne);
            break;
        case kMCScriptEncodedValueKindInteger:
            t_success = MCScriptReadIntegerValueFromStream(stream, t_value);
            break;
        case kMCScriptEncodedValueKindReal:
            t_success = MCScriptReadRealValueFromStream(stream, t_value);
            break;
        case kMCScriptEncodedValueKindEmptyString:
            t_value = MCValueRetain(kMCEmptyString);
            break;
        case kMCScriptEncodedValueKindString:
            t_success = MCScriptReadStringValueFromStream(stream, t_value);
            break;
        case kMCScriptEncodedValueKindEmptyName:
            t_value = MCValueRetain(kMCEmptyName);
            break;
        case kMCScriptEncodedValueKindName:
            t_success = MCScriptReadNameValueFromStream(stream, p_values, p_value_count, t_value);
            break;
        case kMCScriptEncodedValueKindNullType:
            t_value = MCValueRetain(kMCNullType);
            break;
        case kMCScriptEncodedValueKindBooleanType:
            t_value = MCValueRetain(kMCBooleanType);
            break;
        case kMCScriptEncodedValueKindIntegerType:
            t_value = MCValueRetain(kMCIntegerType);
            break;
        case kMCScriptEncodedValueKindNumberType:
            t_value = MCValueRetain(kMCNumberType);
            break;
        case kMCScriptEncodedValueKindStringType:
            t_value = MCValueRetain(kMCStringType);
            break;
        case kMCScriptEncodedValueKindDataType:
            t_value = MCValueRetain(kMCDataType);
            break;
        case kMCScriptEncodedValueKindArrayType:
            t_value = MCValueRetain(kMCArrayType);
            break;
        case kMCScriptEncodedValueKindEnumType:
            t_success = MCScriptReadEnumTypeValueFromStream(stream, p_values, p_value_count, t_value);
            break;
        case kMCScriptEncodedValueKindRecordType:
            t_success = MCScriptReadRecordTypeValueFromStream(stream, p_values, p_value_count, t_value);
            break;
        case kMCScriptEncodedValueKindHandlerType:
            t_success = MCScriptReadHandlerTypeValueFromStream(stream, p_values, p_value_count, t_value);
            break;
    }
    
    if (t_success)
        return MCScriptThrowLastError();
    
    r_value = t_value;
    
    return true;
}

static bool MCScriptReadValueArrayFromStream(MCScriptStreamRef stream, MCValueRef*& r_values, uindex_t& r_value_count)
{
    bool t_success;
    t_success = true;
    
    uint32_t t_value_count;
    t_value_count = 0;
    if (t_success)
        t_success = MCScriptReadMaxUInt32FromStream(stream, t_value_count);
 
    MCValueRef *t_values;
    t_values = nil;
    if (t_success &&
        !MCMemoryNewArray(t_value_count, t_values))
        t_success = MCScriptThrowLastFoundationError();
    
    uindex_t t_read_value_count;
    for(t_read_value_count = 0; t_read_value_count < t_value_count && t_success; t_read_value_count++)
        t_success = MCScriptReadValueFromStream(stream, t_values, t_value_count, t_values[t_read_value_count]);
    
    if (!t_success)
    {
        for(uindex_t i = 0; i < t_read_value_count; i++)
            MCValueRelease(t_values[i]);
        MCMemoryDeleteArray(t_values);
    
        return MCScriptThrowLastError();
    }
    
    return t_success;
}

static bool MCScriptReadModuleDependenciesFromStream(MCScriptStreamRef stream, MCValueRef *p_values, uindex_t p_value_count, MCNameRef*& r_names, uindex_t& r_name_count)
{
    uint32_t t_count;
    if (!MCScriptReadMaxUInt32FromStream(stream, t_count))
        return MCScriptThrowLastError();
    
    if (!MCMemoryNewArray(t_count, ))
}

bool MCScriptCreateModuleFromStream(MCScriptStreamRef stream, MCScriptModuleRef& r_module)
{
    bool t_success;
    t_success = true;
    
    uint32_t t_version;
    MCScriptModuleKind t_kind;
    if (t_success)
        t_success = MCScriptReadModuleHeaderFromStream(stream, t_version, t_kind);
    
    MCValueRef *t_values;
    uindex_t t_value_count;
    t_values = nil;
    t_value_count = 0;
    if (t_success)
        t_success = MCScriptReadValueArrayFromStream(stream, t_values, t_value_count);
    
    MCNameRef t_module_name;
    if (t_success)
        t_success = MCScriptReadPooledNameValueFromStream(stream, t_values, t_value_count, t_module_name);
    
    MCNameRef *t_dependencies;
    uindex_t t_dependency_count;
    t_dependencies = nil;
    t_dependency_count = 0;
    if (t_success)
        t_success = MCScriptReadModuleDependenciesFromStream(stream, t_values, t_value_count, t_dependencies, t_dependency_count);
    
    MCScriptImportedDefinition *t_imp_defs;
    uindex_t t_imp_def_count;
    t_imp_defs = nil;
    t_imp_def_count = 0;
    if (t_success)
        t_success = MCScriptReadModuleImportedDefinitionsFromStream(stream, t_values, t_value_count, t_imp_defs, t_imp_def_count);
    
    MCScriptExportedDefinition *t_exp_defs;
    uindex_t t_exp_def_count;
    t_exp_defs = nil;
    t_exp_def_count = 0;
    if (t_success)
        t_success = MCScriptReadModuleExportedDefinitionsFromStream(stream, t_values, t_value_count, t_exp_defs, t_exp_def_count);
    
    MCScriptDefinition **t_defs;
    uindex_t t_def_count;
    t_defs = nil;
    t_def_count = 0;
    if (t_success)
        t_success = MCScriptReadModuleDefinitionsFromStream(stream, t_values, t_value_count, t_defs, t_def_count);
    
    byte_t *t_bytecode;
    uindex_t t_bytecode_count;
    t_bytecode = nil;
    t_bytecode_count = 0;
    if (t_success)
        t_success = MCScriptReadModuleBytecodeFromStream(stream, t_bytecode, t_bytecode_count);
    
    MCScriptModuleRef t_module;
    if (t_success)
        t_success = MCScriptCreateObject(kMCScriptObjectKindModule, sizeof(MCScriptModule), (MCScriptObject*&)t_module);
    
    if (!t_success)
    {
        MCMemoryDeleteArray(t_bytecode);
        for(uindex_t i = 0; i < t_def_count; i++)
            MCMemoryDeallocate(t_defs[i]);
        MCMemoryDeleteArray(t_defs);
        MCMemoryDeleteArray(t_exp_defs);
        MCMemoryDeleteArray(t_imp_defs);
        MCMemoryDeleteArray(t_dependencies);
        for(uindex_t i = 0; i < t_value_count; i++)
            MCValueRelease(t_values[i]);
        MCMemoryDeleteArray(t_values);
        
        return MCScriptThrowLastError();
    }
    
    t_module -> values = t_values;
    t_module -> value_count = t_value_count;
    t_module -> name = t_module_name;
    t_module -> dependencies = t_dependencies;
    t_module -> dependency_count = t_dependency_count;
    t_module -> imported_definitions = t_imp_defs;
    t_module -> imported_definition_count = t_imp_def_count;
    t_module -> exported_definition = t_exp_defs;
    t_module -> exported_definition_count = t_exp_def_count;
    t_module -> definitions = t_defs;
    t_module -> definition_count = t_def_count;
    t_module -> bytecode = t_bytecode;
    t_module -> bytecode_count = t_bytecode_count;
    
    r_module = t_module;
    
    return true;
}

////////////////////////////////////////////////////////////////////////////////
#endif

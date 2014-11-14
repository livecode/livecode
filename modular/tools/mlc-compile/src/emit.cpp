#include "foundation.h"
#include "foundation-auto.h"
#include "script.h"

#include "report.h"
#include "literal.h"
#include "position.h"

extern "C" void EmitBeginModule(NameRef name, long& r_index);
extern "C" void EmitEndModule(void);
extern "C" void EmitModuleDependency(NameRef name, long& r_index);
extern "C" void EmitImportedType(long module_index, NameRef name, long type_index, long& r_index);
extern "C" void EmitImportedConstant(long module_index, NameRef name, long type_index, long& r_index);
extern "C" void EmitImportedVariable(long module_index, NameRef name, long type_index, long& r_index);
extern "C" void EmitImportedHandler(long module_index, NameRef name, long type_index, long& r_index);
extern "C" void EmitExportedDefinition(long index);
extern "C" void EmitDefinitionIndex(long& r_index);
extern "C" void EmitTypeDefinition(long index, PositionRef position, NameRef name, long type_index);
extern "C" void EmitVariableDefinition(long index, PositionRef position, NameRef name, long type_index);
extern "C" void EmitBeginHandlerDefinition(long index, PositionRef position, NameRef name, long type_index);
extern "C" void EmitEndHandlerDefinition(void);
extern "C" void EmitForeignHandlerDefinition(long index, PositionRef position, NameRef name, long type_index, long binding);
extern "C" void EmitNamedType(long index, long& r_new_index);
extern "C" void EmitOptionalType(long index, long& r_new_index);
extern "C" void EmitPointerType(long& r_new_index);
extern "C" void EmitBoolType(long& r_new_index);
extern "C" void EmitIntType(long& r_new_index);
extern "C" void EmitUIntType(long& r_new_index);
extern "C" void EmitFloatType(long& r_new_index);
extern "C" void EmitDoubleType(long& r_new_index);
extern "C" void EmitAnyType(long& r_new_index);
extern "C" void EmitBooleanType(long& r_new_index);
extern "C" void EmitIntegerType(long& r_new_index);
extern "C" void EmitRealType(long& r_new_index);
extern "C" void EmitNumberType(long& r_new_index);
extern "C" void EmitStringType(long& r_new_index);
extern "C" void EmitDataType(long& r_new_index);
extern "C" void EmitArrayType(long& r_new_index);
extern "C" void EmitListType(long& r_new_index);
extern "C" void EmitUndefinedType(long& r_new_index);
extern "C" void EmitBeginRecordType(long base_type_index);
extern "C" void EmitRecordTypeField(NameRef name, long type_index);
extern "C" void EmitEndRecordType(long& r_type_index);
extern "C" void EmitBeginHandlerType(long return_type_index);
extern "C" void EmitHandlerTypeInParameter(NameRef name, long type_index);
extern "C" void EmitHandlerTypeOutParameter(NameRef name, long type_index);
extern "C" void EmitHandlerTypeInOutParameter(NameRef name, long type_index);
extern "C" void EmitEndHandlerType(long& r_index);
extern "C" void EmitHandlerParameter(NameRef name, long type_index, long& r_index);
extern "C" void EmitHandlerVariable(NameRef name, long type_index, long& r_index);
extern "C" void EmitDeferLabel(long& r_label);
extern "C" void EmitResolveLabel(long label);
extern "C" void EmitCreateRegister(long& r_regindex);
extern "C" void EmitDestroyRegister(long regindex);
extern "C" void EmitJump(long label);
extern "C" void EmitJumpIfTrue(long reg, long label);
extern "C" void EmitJumpIfFalse(long reg, long label);
extern "C" void EmitPushRepeatLabels(long next, long exit);
extern "C" void EmitPopRepeatLabels(void);
extern "C" void EmitCurrentRepeatLabels(long& r_next, long& r_exit);
extern "C" void EmitBeginInvoke(long index, long resultreg);
extern "C" void EmitBeginIndirectInvoke(long reg, long resultreg);
extern "C" void EmitBeginBuiltinInvoke(long name, long resultreg);
extern "C" void EmitContinueInvoke(long reg);
extern "C" void EmitEndInvoke(void);
extern "C" void EmitAssignUndefined(long reg);
extern "C" void EmitAssignTrue(long reg);
extern "C" void EmitAssignFalse(long reg);
extern "C" void EmitAssignInteger(long reg, long value);
extern "C" void EmitAssignReal(long reg, long value);
extern "C" void EmitAssignString(long reg, long value);
extern "C" void EmitFetchLocal(long reg, long var);
extern "C" void EmitStoreLocal(long reg, long var);
extern "C" void EmitFetchGlobal(long reg, long var);
extern "C" void EmitStoreGlobal(long reg, long var);
extern "C" void EmitReturn(long reg);
extern "C" void EmitReturnNothing(void);

//////////

static MCTypeInfoRef *s_typeinfos = nil;
static uindex_t s_typeinfo_count = 0;

static MCNameRef to_mcnameref(NameRef p_name)
{
    const char *t_cstring;
    GetStringOfNameLiteral(p_name, &t_cstring);
    
    MCAutoStringRef t_string;
    MCStringCreateWithCString(t_cstring, &t_string);
    
    MCNameRef t_name;
    MCNameCreate(*t_string, t_name);

    return t_name;
}

static MCStringRef to_mcstringref(long p_string)
{
    MCAutoStringRef t_string;
    MCStringCreateWithCString((const char *)p_string, &t_string);
    MCStringRef t_uniq_string;
    MCValueInter(*t_string, t_uniq_string);
    return t_uniq_string;
}

static MCTypeInfoRef to_mctypeinforef(long p_type_index)
{
    return s_typeinfos[p_type_index];
}

static bool define_typeinfo(MCTypeInfoRef p_typeinfo, long& r_index)
{
    for(uindex_t i = 0; i < s_typeinfo_count; i++)
        if (p_typeinfo == s_typeinfos[i])
        {
            r_index = i;
            return false;
        }
    
    MCMemoryResizeArray(s_typeinfo_count + 1, s_typeinfos, s_typeinfo_count);
    s_typeinfos[s_typeinfo_count - 1] = MCValueRetain(p_typeinfo);
    
    r_index = s_typeinfo_count - 1;
    
    return true;
}

//////////

static MCScriptModuleBuilderRef s_builder;

//////////

void EmitBeginModule(NameRef p_name, long& r_index)
{
    MCInitialize();
    
    MCLog("[Emit] BeginModule(%@) -> 0", to_mcnameref(p_name));
    
    MCScriptBeginModule(kMCScriptModuleKindLibrary, to_mcnameref(p_name), s_builder);
    r_index = 0;
}

void EmitEndModule(void)
{
    MCLog("[Emit] EndModule()", 0);
    
    MCStreamRef t_stream;
    MCMemoryOutputStreamCreate(t_stream);
    
    bool t_success;
    t_success = MCScriptEndModule(s_builder, t_stream);
    
    void *t_buffer;
    size_t t_size;
    MCMemoryOutputStreamFinish(t_stream, t_buffer, t_size);
    
    if (t_success)
        MCLog("Generated module file of size %ld\n", t_size);
    
    MCFinalize();
}

void EmitModuleDependency(NameRef p_name, long& r_index)
{
    uindex_t t_index;
    MCScriptAddDependencyToModule(s_builder, to_mcnameref(p_name), t_index);
    r_index = t_index;
    
    MCLog("[Emit] ModuleDependency(%@ -> 0)", to_mcnameref(p_name), t_index);
}

void EmitImportedType(long p_module_index, NameRef p_name, long p_type_index, long& r_index)
{
    uindex_t t_index;
    MCScriptAddImportToModule(s_builder, p_module_index, to_mcnameref(p_name), kMCScriptDefinitionKindType, to_mctypeinforef(p_type_index), t_index);
    r_index = t_index;
    
    MCLog("[Emit] ImportedType(%ld, %@, %ld -> %d)", p_module_index, to_mcnameref(p_name), p_type_index, t_index);
}

void EmitImportedConstant(long p_module_index, NameRef p_name, long p_type_index, long& r_index)
{
    uindex_t t_index;
    MCScriptAddImportToModule(s_builder, p_module_index, to_mcnameref(p_name), kMCScriptDefinitionKindConstant, to_mctypeinforef(p_type_index), t_index);
    r_index = t_index;
    
    MCLog("[Emit] ImportedType(%ld, %@, %ld -> %d)", p_module_index, to_mcnameref(p_name), p_type_index, t_index);
}

void EmitImportedVariable(long p_module_index, NameRef p_name, long p_type_index, long& r_index)
{
    uindex_t t_index;
    MCScriptAddImportToModule(s_builder, p_module_index, to_mcnameref(p_name), kMCScriptDefinitionKindVariable, to_mctypeinforef(p_type_index), t_index);
    r_index = t_index;
    
    MCLog("[Emit] ImportedType(%ld, %@, %ld -> %d)", p_module_index, to_mcnameref(p_name), p_type_index, t_index);
}

void EmitImportedHandler(long p_module_index, NameRef p_name, long p_type_index, long& r_index)
{
    uindex_t t_index;
    MCScriptAddImportToModule(s_builder, p_module_index, to_mcnameref(p_name), kMCScriptDefinitionKindHandler, to_mctypeinforef(p_type_index), t_index);
    r_index = t_index;
    
    MCLog("[Emit] ImportedType(%ld, %@, %ld -> %d)", p_module_index, to_mcnameref(p_name), p_type_index, t_index);
}

void EmitExportedDefinition(long p_index)
{
    MCScriptAddExportToModule(s_builder, p_index);
    
    MCLog("[Emit] ExportedDefinition(%ld)", p_index);
}

void EmitDefinitionIndex(long& r_index)
{
    uindex_t t_index;
    MCScriptAddDefinitionToModule(s_builder, t_index);
    r_index = t_index;
    
    MCLog("[Emit] DefinitionIndex(-> %u)", t_index);
}

void EmitTypeDefinition(long p_index, PositionRef p_position, NameRef p_name, long p_type_index)
{
    MCScriptAddTypeToModule(s_builder, to_mcnameref(p_name), to_mctypeinforef(p_type_index), p_index);
    
    MCLog("[Emit] TypeDefinition(%ld, %@, %ld)", p_index, to_mcnameref(p_name), p_type_index);
}

void EmitVariableDefinition(long p_index, PositionRef p_position, NameRef p_name, long p_type_index)
{
    MCScriptAddVariableToModule(s_builder, to_mcnameref(p_name), to_mctypeinforef(p_type_index), p_index);
    
    MCLog("[Emit] VariableDefinition(%ld, %@, %ld)", p_index, to_mcnameref(p_name), p_type_index);
}

void EmitForeignHandlerDefinition(long p_index, PositionRef p_position, NameRef p_name, long p_type_index, long p_binding)
{
    MCScriptAddForeignHandlerToModule(s_builder, to_mcnameref(p_name), to_mctypeinforef(p_type_index), to_mcstringref(p_binding), p_index);
    
    MCLog("[Emit] ForeignHandlerDefinition(%ld, %@, %ld, %@)", p_index, to_mcnameref(p_name), p_type_index, to_mcstringref(p_binding));
}

void EmitBeginHandlerDefinition(long p_index, PositionRef p_position, NameRef p_name, long p_type_index)
{
    MCScriptBeginHandlerInModule(s_builder, to_mcnameref(p_name), to_mctypeinforef(p_type_index), p_index);
    
    MCLog("[Emit] BeginHandlerDefinition(%ld, %@, %ld)", p_index, to_mcnameref(p_name), p_type_index);
}

void EmitEndHandlerDefinition(void)
{
    MCScriptEndHandlerInModule(s_builder);
    
    MCLog("[Emit] EndHandlerDefinition()", 0);
}

void EmitNamedType(long index, long& r_new_index)
{
    if (!define_typeinfo(kMCNullTypeInfo, r_new_index))
        return;
    
    MCLog("[Emit] NamedType(%ld -> %ld)", index, r_new_index);
}

void EmitOptionalType(long base_index, long& r_new_index)
{
    if (!define_typeinfo(kMCNullTypeInfo, r_new_index))
        return;
    
    MCLog("[Emit] OptionalType(%ld -> %ld)", base_index, r_new_index);
}

void EmitPointerType(long& r_new_index)
{
    if (!define_typeinfo(kMCNullTypeInfo, r_new_index))
        return;
    
    MCLog("[Emit] PointerType(-> %ld)", r_new_index);
}

void EmitBoolType(long& r_new_index)
{
    if (!define_typeinfo(kMCNullTypeInfo, r_new_index))
        return;
    
    MCLog("[Emit] BoolType(-> %ld)", r_new_index);
}

void EmitIntType(long& r_new_index)
{
    if (!define_typeinfo(kMCNullTypeInfo, r_new_index))
        return;
    
    MCLog("[Emit] IntType(-> %ld)", r_new_index);
}

void EmitUIntType(long& r_new_index)
{
    if (!define_typeinfo(kMCNullTypeInfo, r_new_index))
        return;
    
    MCLog("[Emit] UIntType(-> %ld)", r_new_index);
}

void EmitFloatType(long& r_new_index)
{
    if (!define_typeinfo(kMCNullTypeInfo, r_new_index))
        return;
    
    MCLog("[Emit] FloatType(-> %ld)", r_new_index);
}

void EmitDoubleType(long& r_new_index)
{
    if (!define_typeinfo(kMCNullTypeInfo, r_new_index))
        return;
    
    MCLog("[Emit] DoubleType(-> %ld)", r_new_index);
}

void EmitAnyType(long& r_new_index)
{
    if (!define_typeinfo(kMCNullTypeInfo, r_new_index))
        return;
    
    MCLog("[Emit] AnyType(-> %ld)", r_new_index);
}

void EmitBooleanType(long& r_new_index)
{
    if (!define_typeinfo(kMCBooleanTypeInfo, r_new_index))
        return;
    
    MCLog("[Emit] BooleanType(-> %ld)", r_new_index);
}

void EmitIntegerType(long& r_new_index)
{
    if (!define_typeinfo(kMCNumberTypeInfo, r_new_index))
        return;
    
    MCLog("[Emit] IntegerType(-> %ld)", r_new_index);
}

void EmitRealType(long& r_new_index)
{
    if (!define_typeinfo(kMCNumberTypeInfo, r_new_index))
        return;
    
    MCLog("[Emit] RealType(-> %ld)", r_new_index);
}

void EmitNumberType(long& r_new_index)
{
    if (!define_typeinfo(kMCNumberTypeInfo, r_new_index))
        return;
    
    MCLog("[Emit] NumberType(-> %ld)", r_new_index);
}

void EmitStringType(long& r_new_index)
{
    if (!define_typeinfo(kMCStringTypeInfo, r_new_index))
        return;
    
    MCLog("[Emit] StringType(-> %ld)", r_new_index);
}

void EmitDataType(long& r_new_index)
{
    if (!define_typeinfo(kMCDataTypeInfo, r_new_index))
        return;
    
    MCLog("[Emit] DataType(-> %ld)", r_new_index);
}

void EmitArrayType(long& r_new_index)
{
    if (!define_typeinfo(kMCArrayTypeInfo, r_new_index))
        return;
    
    MCLog("[Emit] ArrayType(-> %ld)", r_new_index);
}

void EmitListType(long& r_new_index)
{
    if (!define_typeinfo(kMCProperListTypeInfo, r_new_index))
        return;
    
    MCLog("[Emit] ListType(-> %ld)", r_new_index);
}

void EmitUndefinedType(long& r_new_index)
{
    if (!define_typeinfo(kMCNullTypeInfo, r_new_index))
        return;
    
    MCLog("[Emit] UndefinedType(-> %ld)", r_new_index);
}

//////////

static MCTypeInfoRef s_current_record_basetype = nil;
static MCRecordTypeFieldInfo *s_current_record_fields = nil;
static uindex_t s_current_record_field_count = 0;

void EmitBeginRecordType(long base_type_index)
{
    s_current_record_basetype = to_mctypeinforef(base_type_index);
    s_current_record_field_count = 0;
}

void EmitRecordTypeField(NameRef name, long type_index)
{
    MCMemoryResizeArray(s_current_record_field_count + 1, s_current_record_fields, s_current_record_field_count);
    s_current_record_fields[s_current_record_field_count - 1] . name = to_mcnameref(name);
    s_current_record_fields[s_current_record_field_count - 1] . type = to_mctypeinforef(type_index);
}

void EmitEndRecordType(long& r_type_index)
{
    MCAutoTypeInfoRef t_typeinfo;
    MCRecordTypeInfoCreate(s_current_record_fields, s_current_record_field_count, &t_typeinfo);
    if (!define_typeinfo(*t_typeinfo, r_type_index))
        return;
    
    MCLog("[Emit] BeginRecordType(%ld -> %ld)", s_current_record_basetype, r_type_index);
    for(uindex_t i = 0; i < s_current_record_field_count; i++)
    {
        long t_index;
        define_typeinfo(s_current_record_fields[i] . type, t_index);
        MCLog("[Emit] RecordTypeField(%@, %ld)", s_current_record_fields[i] . name, t_index);
    }
    MCLog("[Emit] EndRecordType()", 0);
}

//////////

static MCTypeInfoRef s_current_handler_returntype = nil;
static MCHandlerTypeFieldInfo *s_current_handler_fields = nil;
static uindex_t s_current_handler_field_count = 0;

void EmitBeginHandlerType(long return_type_index)
{
    s_current_handler_returntype = to_mctypeinforef(return_type_index);
    s_current_handler_field_count = 0;
}

static void EmitHandlerTypeParameter(MCHandlerTypeFieldMode mode, NameRef name, long type_index)
{
    MCMemoryResizeArray(s_current_handler_field_count + 1, s_current_handler_fields, s_current_handler_field_count);
    s_current_handler_fields[s_current_handler_field_count - 1] . mode = mode;
    s_current_handler_fields[s_current_handler_field_count - 1] . type = to_mctypeinforef(type_index);
}

void EmitHandlerTypeInParameter(NameRef name, long type_index)
{
    EmitHandlerTypeParameter(kMCHandlerTypeFieldModeIn, name, type_index);
}

void EmitHandlerTypeOutParameter(NameRef name, long type_index)
{
    EmitHandlerTypeParameter(kMCHandlerTypeFieldModeOut, name, type_index);
}

void EmitHandlerTypeInOutParameter(NameRef name, long type_index)
{
    EmitHandlerTypeParameter(kMCHandlerTypeFieldModeInOut, name, type_index);
}

void EmitEndHandlerType(long& r_type_index)
{
    MCAutoTypeInfoRef t_typeinfo;
    MCHandlerTypeInfoCreate(s_current_handler_fields, s_current_handler_field_count, s_current_handler_returntype, &t_typeinfo);
    if (!define_typeinfo(*t_typeinfo, r_type_index))
        return;
    
    MCLog("[Emit] BeginHandlerType(%ld -> %ld)", s_current_handler_returntype, r_type_index);
    for(uindex_t i = 0; i < s_current_handler_field_count; i++)
    {
        long t_index;
        define_typeinfo(s_current_handler_fields[i] . type, t_index);
        MCLog("[Emit] HandlerTypeField(%d, %ld)", s_current_handler_fields[i] . mode, t_index);
    }
    MCLog("[Emit] EndHandlerType()", 0);
}

///////////

void EmitHandlerParameter(NameRef name, long type_index, long& r_index)
{
    uindex_t t_index;
    MCScriptAddParameterToHandlerInModule(s_builder, to_mcnameref(name), to_mctypeinforef(type_index), t_index);
    r_index = t_index;
    
    MCLog("[Emit] HandlerParameter(%@, %ld -> %ld)", to_mcnameref(name), type_index, r_index);
}

void EmitHandlerVariable(NameRef name, long type_index, long& r_index)
{
    uindex_t t_index;
    MCScriptAddVariableToHandlerInModule(s_builder, to_mcnameref(name), to_mctypeinforef(type_index), t_index);
    r_index = t_index;
    
    MCLog("[Emit] HandlerVariable(%@, %ld -> %ld)", to_mcnameref(name), type_index, r_index);
}

void EmitDeferLabel(long& r_label)
{
    uindex_t t_index;
    MCScriptDeferLabelForBytecodeInModule(s_builder, t_index);
    r_label = t_index;
    
    MCLog("[Emit] DeferLabel(-> %ld)", r_label);
}

void EmitResolveLabel(long label)
{
    MCScriptResolveLabelForBytecodeInModule(s_builder, label);

    MCLog("[Emit] ResolveLabel(%ld)", label);
}

//////////

static uint8_t *s_registers = nil;
static uindex_t s_register_count = 0;

void EmitCreateRegister(long& r_regindex)
{
    long t_reg;
    t_reg = -1;
    for(uindex_t i = 0; i < s_register_count; i++)
        if (s_registers[i] == 0)
        {
            s_registers[i] = 1;
            t_reg = i;
            break;
        }

    if (t_reg == -1)
    {
        MCMemoryResizeArray(s_register_count + 1, s_registers, s_register_count);
        t_reg = s_register_count - 1;
    }
    
    r_regindex = t_reg;
    
    MCLog("[Emit] CreateRegister(-> %ld)", r_regindex);
}

void EmitDestroyRegister(long regindex)
{
    s_registers[regindex] = 0;
    
    MCLog("[Emit] DestroyRegister(%ld)", regindex);
}

//////////

void EmitJump(long label)
{
    MCScriptEmitJumpInModule(s_builder, label);
    MCLog("[Emit] Jump(%ld)", label);
}

void EmitJumpIfTrue(long reg, long label)
{
    MCScriptEmitJumpIfTrueInModule(s_builder, reg, label);
    MCLog("[Emit] JumpIfTrue(%ld, %ld)", label);
}

void EmitJumpIfFalse(long reg, long label)
{
    MCScriptEmitJumpIfFalseInModule(s_builder, reg, label);
    MCLog("[Emit] JumpIfFalse(%ld, %ld)", reg, label);
}

//////////

struct RepeatLabels
{
    RepeatLabels *next;
    long head;
    long tail;
};

static RepeatLabels *s_repeat_labels = nil;

void EmitPushRepeatLabels(long next, long exit)
{
    RepeatLabels *t_labels;
    MCMemoryNew(t_labels);
    t_labels -> head = next;
    t_labels -> tail = exit;
    t_labels -> next = s_repeat_labels;
    s_repeat_labels = t_labels;
}

void EmitPopRepeatLabels(void)
{
    RepeatLabels *t_labels;
    t_labels = s_repeat_labels;
    s_repeat_labels = t_labels -> next;
}

void EmitCurrentRepeatLabels(long& r_next, long& r_exit)
{
    r_next = s_repeat_labels -> head;
    r_exit = s_repeat_labels -> tail;
}

//////////

void EmitBeginInvoke(long index, long resultreg)
{
    MCScriptBeginInvokeInModule(s_builder, index, resultreg);
    MCLog("[Emit] BeginInvoke(%ld, %ld)", index, resultreg);
}

void EmitBeginIndirectInvoke(long reg, long resultreg)
{
    MCScriptBeginIndirectInvokeInModule(s_builder, reg, resultreg);
    MCLog("[Emit] BeginIndirectInvoke(%ld, %ld)", reg, resultreg);
}

void EmitBeginBuiltinInvoke(long name, long resultreg)
{
    // TODO
    MCScriptBeginInvokeInModule(s_builder, 0, resultreg);
    MCLog("[Emit] BeginBuiltinInvoke(%s, %ld)", (const char *)name, resultreg);
}

void EmitContinueInvoke(long reg)
{
    MCScriptContinueInvokeInModule(s_builder, reg);
    MCLog("[Emit] ContinueInvoke(%ld)", reg);
}

void EmitEndInvoke(void)
{
    MCScriptEndInvokeInModule(s_builder);
    MCLog("[Emit] EndInvoke()", 0);
}

//////////

void EmitAssignUndefined(long reg)
{
    MCScriptEmitAssignConstantInModule(s_builder, reg, kMCNull);
    MCLog("[Emit] AssignUndefined(%ld)", reg);
}

void EmitAssignTrue(long reg)
{
    MCScriptEmitAssignConstantInModule(s_builder, reg, kMCNull);
    MCLog("[Emit] AssignUndefined(%ld)", reg);
}

void EmitAssignFalse(long reg)
{
    MCScriptEmitAssignConstantInModule(s_builder, reg, kMCNull);
    MCLog("[Emit] AssignUndefined(%ld)", reg);
}

void EmitAssignInteger(long reg, long value)
{
    MCAutoNumberRef t_number;
    MCNumberCreateWithInteger(value, &t_number);
    MCScriptEmitAssignConstantInModule(s_builder, reg, *t_number);
    MCLog("[Emit] AssignInteger(%ld, %ld)", reg, value);
}

void EmitAssignReal(long reg, long value)
{
    MCAutoNumberRef t_number;
    MCNumberCreateWithReal(*(double *)value, &t_number);
    MCScriptEmitAssignConstantInModule(s_builder, reg, *t_number);
    MCLog("[Emit] AssignReal(%ld, %lf)", reg, *(double *)value);
}

void EmitAssignString(long reg, long value)
{
    MCAutoStringRef t_string;
    MCStringCreateWithCString((const char *)value, &t_string);
    MCScriptEmitAssignConstantInModule(s_builder, reg, *t_string);
    MCLog("[Emit] AssignString(%ld, \"%s\")", reg, (const char *)value);
}

/////////

void EmitFetchLocal(long reg, long var)
{
    MCScriptEmitFetchLocalInModule(s_builder, reg, var);
    MCLog("[Emit] FetchLocal(%ld, %ld)", reg, var);
}

void EmitStoreLocal(long reg, long var)
{
    MCScriptEmitStoreLocalInModule(s_builder, reg, var);
    MCLog("[Emit] StoreLocal(%ld, %ld)", reg, var);
}

void EmitFetchGlobal(long reg, long var)
{
    MCScriptEmitFetchGlobalInModule(s_builder, reg, var);
    MCLog("[Emit] FetchGlobal(%ld, %ld)", reg, var);
}

void EmitStoreGlobal(long reg, long var)
{
    MCScriptEmitStoreGlobalInModule(s_builder, reg, var);
    MCLog("[Emit] StoreGlobal(%ld, %ld)", reg, var);
}

void EmitReturn(long reg)
{
    MCScriptEmitReturnInModule(s_builder, reg);
    MCLog("[Emit] Return(%ld)", reg);
}

void EmitReturnNothing(void)
{
    long t_reg;
    EmitCreateRegister(t_reg);
    EmitAssignUndefined(t_reg);
    EmitReturn(t_reg);
    EmitDestroyRegister(t_reg);
}

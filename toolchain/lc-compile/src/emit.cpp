#include "foundation.h"
#include "foundation-auto.h"
#include "script.h"

#include "report.h"
#include "literal.h"
#include "position.h"

int OutputFileAsC = 0;

extern "C" void EmitBeginModule(NameRef name, long& r_index);
extern "C" void EmitBeginWidgetModule(NameRef name, long& r_index);
extern "C" void EmitBeginLibraryModule(NameRef name, long& r_index);
extern "C" void EmitEndModule(void);
extern "C" void EmitModuleDependency(NameRef name, long& r_index);
extern "C" void EmitImportedType(long module_index, NameRef name, long type_index, long& r_index);
extern "C" void EmitImportedConstant(long module_index, NameRef name, long type_index, long& r_index);
extern "C" void EmitImportedVariable(long module_index, NameRef name, long type_index, long& r_index);
extern "C" void EmitImportedHandler(long module_index, NameRef name, long type_index, long& r_index);
extern "C" void EmitImportedSyntax(long p_module_index, NameRef p_name, long p_type_index, long& r_index);
extern "C" void EmitExportedDefinition(long index);
extern "C" void EmitDefinitionIndex(long& r_index);
extern "C" void EmitTypeDefinition(long index, PositionRef position, NameRef name, long type_index);
extern "C" void EmitVariableDefinition(long index, PositionRef position, NameRef name, long type_index);
extern "C" void EmitBeginHandlerDefinition(long index, PositionRef position, NameRef name, long type_index);
extern "C" void EmitEndHandlerDefinition(void);
extern "C" void EmitForeignHandlerDefinition(long index, PositionRef position, NameRef name, long type_index, long binding);
extern "C" void EmitEventDefinition(long p_index, PositionRef p_position, NameRef p_name, long p_type_index);
extern "C" void EmitPropertyDefinition(long p_index, PositionRef p_position, NameRef p_name, long setter, long getter);

extern "C" void EmitBeginSyntaxDefinition(long p_index, PositionRef p_position, NameRef p_name);
extern "C" void EmitEndSyntaxDefinition(void);
extern "C" void EmitBeginSyntaxMethod(long p_handler_index);
extern "C" void EmitEndSyntaxMethod(void);
extern "C" void EmitUndefinedSyntaxMethodArgument(void);
extern "C" void EmitTrueSyntaxMethodArgument(void);
extern "C" void EmitFalseSyntaxMethodArgument(void);
extern "C" void EmitInputSyntaxMethodArgument(void);
extern "C" void EmitOutputSyntaxMethodArgument(void);
extern "C" void EmitContextSyntaxMethodArgument(void);
extern "C" void EmitIteratorSyntaxMethodArgument(void);
extern "C" void EmitContainerSyntaxMethodArgument(void);
extern "C" void EmitIntegerSyntaxMethodArgument(long p_int);
extern "C" void EmitRealSyntaxMethodArgument(long p_double);
extern "C" void EmitStringSyntaxMethodArgument(long p_string);
extern "C" void EmitVariableSyntaxMethodArgument(long p_index);
extern "C" void EmitIndexedVariableSyntaxMethodArgument(long p_var_index, long p_element_index);

extern "C" void EmitBeginDefinitionGroup(void);
extern "C" void EmitContinueDefinitionGroup(long index);
extern "C" void EmitEndDefinitionGroup(long *r_index);

extern "C" void EmitDefinedType(long index, long& r_type_index);
extern "C" void EmitForeignType(long binding, long& r_type_index);
extern "C" void EmitNamedType(NameRef module_name, NameRef name, long& r_new_index);
extern "C" void EmitAliasType(NameRef name, long typeindex, long& r_new_index);
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
extern "C" void EmitBeginCall(long index, long resultreg);
extern "C" void EmitBeginIndirectCall(long reg, long resultreg);
extern "C" void EmitContinueCall(long reg);
extern "C" void EmitEndCall(void);
extern "C" void EmitBeginBuiltinInvoke(long name, long resultreg);
extern "C" void EmitBeginExecuteInvoke(long index, long contextreg, long resultreg);
extern "C" void EmitBeginEvaluateInvoke(long index, long contextreg, long outputreg);
extern "C" void EmitBeginAssignInvoke(long index, long contextreg, long inputreg);
extern "C" void EmitBeginIterateInvoke(long index, long contextreg, long iteratorreg, long containerreg);
extern "C" void EmitContinueInvoke(long reg);
extern "C" void EmitEndInvoke(void);
extern "C" void EmitAssign(long dst, long src);
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
extern "C" void EmitAttachRegisterToExpression(long reg, long expr);
extern "C" void EmitDetachRegisterFromExpression(long expr);
extern "C" int EmitGetRegisterAttachedToExpression(long expr, long *reg);
extern "C" void EmitPosition(PositionRef position);

extern "C" void OutputBeginManifest(void);
extern "C" void OutputEnd(void);
extern "C" void OutputWrite(const char *msg);
extern "C" void OutputWriteI(const char *left, NameRef name, const char *right);
extern "C" void OutputWriteS(const char *left, const char *string, const char *right);

//////////

//static MCTypeInfoRef *s_typeinfos = nil;
//static uindex_t s_typeinfo_count = 0;

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

//////////

static MCScriptModuleBuilderRef s_builder;
static NameRef s_module_name;

//////////

void EmitBeginModule(NameRef p_name, long& r_index)
{
    MCInitialize();
    
    MCLog("[Emit] BeginModule(%@) -> 0", to_mcnameref(p_name));

    s_module_name = p_name;
    
    MCScriptBeginModule(kMCScriptModuleKindNone, to_mcnameref(p_name), s_builder);
    r_index = 0;
}

void EmitBeginWidgetModule(NameRef p_name, long& r_index)
{
    MCInitialize();
    
    MCLog("[Emit] BeginWidgetModule(%@) -> 0", to_mcnameref(p_name));
    
    s_module_name = p_name;
    
    MCScriptBeginModule(kMCScriptModuleKindWidget, to_mcnameref(p_name), s_builder);
    r_index = 0;
}

void EmitBeginLibraryModule(NameRef p_name, long& r_index)
{
    MCInitialize();
    
    MCLog("[Emit] BeginLibraryModule(%@) -> 0", to_mcnameref(p_name));
    
    s_module_name = p_name;
    
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
    MCValueRelease(t_stream);
    
    if (t_success)
    {
        MCLog("Generated module file of size %ld\n", t_size);
        FILE *t_output;
        t_output = OpenOutputFile();
        
        if (OutputFileAsC)
        {
            fprintf(t_output, "static unsigned char module_data[] = {");
            for(size_t i = 0; i < t_size; i++)
            {
                if ((i % 16) == 0)
                    fprintf(t_output, "\n");
                fprintf(t_output, "0x%02x, ", ((unsigned char *)t_buffer)[i]);
            }
            fprintf(t_output, "};\n");
            
            const char *t_name;
            GetStringOfNameLiteral(s_module_name, &t_name);
            fprintf(t_output, "__attribute__((used)) __attribute__((section(\"__MODULES,__modules\"))) static volatile struct { const char *name; unsigned char *data; unsigned long length; } module_info = { \"%s\", module_data, sizeof(module_data) };\n", t_name);
        }
        else if (t_output != NULL)
        {
            fwrite(t_buffer, 1, t_size, t_output);
            fclose(t_output);
        }
    }

    free(t_buffer);
    
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
    MCScriptAddImportToModule(s_builder, p_module_index, to_mcnameref(p_name), kMCScriptDefinitionKindType, p_type_index, t_index);
    r_index = t_index;
    
    MCLog("[Emit] ImportedType(%ld, %@, %ld -> %d)", p_module_index, to_mcnameref(p_name), p_type_index, t_index);
}

void EmitImportedConstant(long p_module_index, NameRef p_name, long p_type_index, long& r_index)
{
    uindex_t t_index;
    MCScriptAddImportToModule(s_builder, p_module_index, to_mcnameref(p_name), kMCScriptDefinitionKindConstant, p_type_index, t_index);
    r_index = t_index;
    
    MCLog("[Emit] ImportedConstant(%ld, %@, %ld -> %d)", p_module_index, to_mcnameref(p_name), p_type_index, t_index);
}

void EmitImportedVariable(long p_module_index, NameRef p_name, long p_type_index, long& r_index)
{
    uindex_t t_index;
    MCScriptAddImportToModule(s_builder, p_module_index, to_mcnameref(p_name), kMCScriptDefinitionKindVariable, p_type_index, t_index);
    r_index = t_index;
    
    MCLog("[Emit] ImportedVariable(%ld, %@, %ld -> %d)", p_module_index, to_mcnameref(p_name), p_type_index, t_index);
}

void EmitImportedHandler(long p_module_index, NameRef p_name, long p_type_index, long& r_index)
{
    uindex_t t_index;
    MCScriptAddImportToModule(s_builder, p_module_index, to_mcnameref(p_name), kMCScriptDefinitionKindHandler, p_type_index, t_index);
    r_index = t_index;
    
    MCLog("[Emit] ImportedHandler(%ld, %@, %ld -> %d)", p_module_index, to_mcnameref(p_name), p_type_index, t_index);
}

void EmitImportedSyntax(long p_module_index, NameRef p_name, long p_type_index, long& r_index)
{
    uindex_t t_index;
    MCScriptAddImportToModule(s_builder, p_module_index, to_mcnameref(p_name), kMCScriptDefinitionKindSyntax, p_type_index, t_index);
    r_index = t_index;
    
    MCLog("[Emit] ImportedSyntax(%ld, %@, %ld -> %d)", p_module_index, to_mcnameref(p_name), p_type_index, t_index);
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
    MCScriptAddTypeToModule(s_builder, to_mcnameref(p_name), p_type_index, p_index);
    
    MCLog("[Emit] TypeDefinition(%ld, %@, %ld)", p_index, to_mcnameref(p_name), p_type_index);
}

void EmitVariableDefinition(long p_index, PositionRef p_position, NameRef p_name, long p_type_index)
{
    MCScriptAddVariableToModule(s_builder, to_mcnameref(p_name), p_type_index, p_index);
    
    MCLog("[Emit] VariableDefinition(%ld, %@, %ld)", p_index, to_mcnameref(p_name), p_type_index);
}

void EmitForeignHandlerDefinition(long p_index, PositionRef p_position, NameRef p_name, long p_type_index, long p_binding)
{
    MCAutoStringRef t_binding_str;
    if (strcmp((const char *)p_binding, "<builtin>") == 0)
        t_binding_str = MCNameGetString(to_mcnameref(p_name));
    else
        t_binding_str = to_mcstringref(p_binding);
    
    MCScriptAddForeignHandlerToModule(s_builder, to_mcnameref(p_name), p_type_index, *t_binding_str, p_index);
    
    MCLog("[Emit] ForeignHandlerDefinition(%ld, %@, %ld, %@)", p_index, to_mcnameref(p_name), p_type_index, to_mcstringref(p_binding));
}

void EmitPropertyDefinition(long p_index, PositionRef p_position, NameRef p_name, long p_getter, long p_setter)
{
    MCScriptAddPropertyToModule(s_builder, to_mcnameref(p_name), p_getter, p_setter, p_index);
    
    MCLog("[Emit] PropertyDefinition(%ld, %@, %ld, %ld)", p_index, to_mcnameref(p_name), p_getter, p_setter);
}

void EmitEventDefinition(long p_index, PositionRef p_position, NameRef p_name, long p_type_index)
{
    MCScriptAddEventToModule(s_builder, to_mcnameref(p_name), p_type_index, p_index);
    
    MCLog("[Emit] EmitEvent(%ld, %@, %ld)", p_index, to_mcnameref(p_name), p_type_index);
}

void EmitBeginHandlerDefinition(long p_index, PositionRef p_position, NameRef p_name, long p_type_index)
{
    MCScriptBeginHandlerInModule(s_builder, to_mcnameref(p_name), p_type_index, p_index);
    
    MCLog("[Emit] BeginHandlerDefinition(%ld, %@, %ld)", p_index, to_mcnameref(p_name), p_type_index);
}

void EmitEndHandlerDefinition(void)
{
    MCScriptEndHandlerInModule(s_builder);
    
    MCLog("[Emit] EndHandlerDefinition()", 0);
}

void EmitBeginSyntaxDefinition(long p_index, PositionRef p_position, NameRef p_name)
{
    MCScriptBeginSyntaxInModule(s_builder, to_mcnameref(p_name), p_index);
    
    MCLog("[Emit] BeginSyntaxDefinition(%ld, %@)", p_index, to_mcnameref(p_name));
}

void EmitEndSyntaxDefinition(void)
{
    MCScriptEndSyntaxInModule(s_builder);

    MCLog("[Emit] EndSyntaxDefinition()", 0);
}

void EmitBeginSyntaxMethod(long p_handler_index)
{
    MCScriptBeginSyntaxMethodInModule(s_builder, p_handler_index);
    
    MCLog("[Emit] BeginSyntaxMethod(%ld)", p_handler_index);
}

void EmitEndSyntaxMethod(void)
{
    MCScriptEndSyntaxMethodInModule(s_builder);
    
    MCLog("[Emit] EmitEndSyntaxMethod()", 0);
}

void EmitInputSyntaxMethodArgument(void)
{
    MCScriptAddBuiltinArgumentToSyntaxMethodInModule(s_builder, 0);
    
    MCLog("[Emit] InputSyntaxMethodArgument()", 0);
}

void EmitOutputSyntaxMethodArgument(void)
{
    MCScriptAddBuiltinArgumentToSyntaxMethodInModule(s_builder, 1);
    
    MCLog("[Emit] OutputSyntaxMethodArgument()", 0);
}

void EmitContextSyntaxMethodArgument(void)
{
    // TODO: Sort out context
    // MCScriptAddBuiltinArgumentToSyntaxMethodInModule(s_builder, 2);
    
    MCLog("[Emit] ContextSyntaxMethodArgument()", 0);
}

void EmitIteratorSyntaxMethodArgument(void)
{
    // TODO: Sort out iterate
    MCScriptAddBuiltinArgumentToSyntaxMethodInModule(s_builder, 2);
    
    MCLog("[Emit] IteratorSyntaxMethodArgument()", 0);
}

void EmitContainerSyntaxMethodArgument(void)
{
    // TODO: Sort out iterate
    MCScriptAddBuiltinArgumentToSyntaxMethodInModule(s_builder, 3);
    
    MCLog("[Emit] ContainerSyntaxMethodArgument()", 0);
}

void EmitUndefinedSyntaxMethodArgument(void)
{
    MCScriptAddConstantArgumentToSyntaxMethodInModule(s_builder, kMCNull);
    
    MCLog("[Emit] UndefinedSyntaxMethodArgument()", 0);
}

void EmitTrueSyntaxMethodArgument(void)
{
    MCScriptAddConstantArgumentToSyntaxMethodInModule(s_builder, kMCTrue);
    
    MCLog("[Emit] TrueSyntaxMethodArgument()", 0);
}

void EmitFalseSyntaxMethodArgument(void)
{
    MCScriptAddConstantArgumentToSyntaxMethodInModule(s_builder, kMCFalse);
    
    MCLog("[Emit] FalseSyntaxMethodArgument()", 0);
}

void EmitIntegerSyntaxMethodArgument(long p_int)
{
    MCAutoNumberRef t_number;
    MCNumberCreateWithInteger(p_int, &t_number);
    MCScriptAddConstantArgumentToSyntaxMethodInModule(s_builder, *t_number);
    
    MCLog("[Emit] IntegerSyntaxMethodArgument(%ld)", p_int);
}

void EmitRealSyntaxMethodArgument(long p_double)
{
    MCAutoNumberRef t_number;
    MCNumberCreateWithInteger(p_double, &t_number);
    MCScriptAddConstantArgumentToSyntaxMethodInModule(s_builder, *t_number);
    
    MCLog("[Emit] RealSyntaxMethodArgument(%lf)", *(double *)p_double);
}

void EmitStringSyntaxMethodArgument(long p_string)
{
    MCAutoStringRef t_string;
    MCStringCreateWithCString((const char *)p_string, &t_string);
    MCScriptAddConstantArgumentToSyntaxMethodInModule(s_builder, *t_string);
    
    MCLog("[Emit] RealSyntaxMethodArgument(\"%s\")", (const char *)p_string);
}

void EmitVariableSyntaxMethodArgument(long p_index)
{
    MCScriptAddVariableArgumentToSyntaxMethodInModule(s_builder, p_index);
    
    MCLog("[Emit] VariableSyntaxMethodArgument(%ld)", p_index);
}

void EmitIndexedVariableSyntaxMethodArgument(long p_var_index, long p_element_index)
{
    MCScriptAddIndexedVariableArgumentToSyntaxMethodInModule(s_builder, p_var_index, p_element_index);
    
    MCLog("[Emit] IndexedVariableSyntaxMethodArgument(%ld, %ld)", p_var_index, p_element_index);
}

void EmitBeginDefinitionGroup(void)
{
    MCScriptBeginDefinitionGroupInModule(s_builder);

    MCLog("[Emit] BeginDefinitionGroup()", 0);
}

void EmitContinueDefinitionGroup(long p_index)
{
    MCScriptAddHandlerToDefinitionGroupInModule(s_builder, p_index);
    
    MCLog("[Emit] ContinueDefinitionGroup(%ld)", p_index);
}

void EmitEndDefinitionGroup(long *r_index)
{
    uindex_t t_index;
    MCScriptEndDefinitionGroupInModule(s_builder, t_index);
    *r_index = t_index;
    
    MCLog("[Emit] EndDefinitionGroup(-> %ld)", *r_index);
}

/////////

static bool define_builtin_typeinfo(const char *name, long& r_new_index)
{
    uindex_t t_mod_index;
    MCScriptAddDependencyToModule(s_builder, MCNAME("__builtin__"), t_mod_index);
    
    uindex_t t_index;
    MCScriptAddImportToModule(s_builder, t_mod_index, MCNAME(name), kMCScriptDefinitionKindType, 0, t_index);
    
    uindex_t t_typeindex;
    MCScriptAddDefinedTypeToModule(s_builder, t_index, t_typeindex);
    
    r_new_index = t_typeindex;
    
    return true;
}

#if 0
void EmitNamedType(NameRef module_name, NameRef name, long& r_new_index)
{
    MCAutoStringRef t_string;
    MCStringFormat(&t_string, "%@.%@", to_mcnameref(module_name), to_mcnameref(name));
    MCNewAutoNameRef t_name;
    MCNameCreate(*t_string, &t_name);
    MCAutoTypeInfoRef t_type;
    MCNamedTypeInfoCreate(*t_name, &t_type);
    if (!define_typeinfo(*t_type, r_new_index))
        return;
    
    MCLog("[Emit] NamedType(%@, %@ -> %ld)", to_mcnameref(module_name), to_mcnameref(name), r_new_index);
}

void EmitAliasType(NameRef name, long target_index, long& r_new_index)
{
    MCAutoTypeInfoRef t_type;
    MCAliasTypeInfoCreate(to_mcnameref(name), to_mctypeinforef(target_index), &t_type);
    if (!define_typeinfo(*t_type, r_new_index))
        return;
    
    MCLog("[Emit] AliasType(%@, %ld -> %ld)", to_mcnameref(name), target_index, r_new_index);
}
#endif

void EmitDefinedType(long index, long& r_type_index)
{
    uindex_t t_type_index;
    MCScriptAddDefinedTypeToModule(s_builder, index, t_type_index);
    r_type_index = t_type_index;
    
    MCLog("[Emit] DefinedType(%ld -> %ld)", index, r_type_index);
}

void EmitForeignType(long p_binding, long& r_type_index)
{
    uindex_t t_type_index;
    MCScriptAddForeignTypeToModule(s_builder, to_mcstringref(p_binding), t_type_index);
    r_type_index = t_type_index;
    
    MCLog("[Emit] ForeignType(%s -> %ld)", (const char *)p_binding, r_type_index);
}

void EmitOptionalType(long base_index, long& r_new_index)
{
    uindex_t t_index;
    MCScriptAddOptionalTypeToModule(s_builder, base_index, t_index);
    r_new_index = t_index;
    MCLog("[Emit] OptionalType(%ld -> %ld)", base_index, r_new_index);
}

void EmitPointerType(long& r_new_index)
{
    if (!define_builtin_typeinfo("pointer", r_new_index))
        return;
    
    MCLog("[Emit] PointerType(-> %ld)", r_new_index);
}

void EmitBoolType(long& r_new_index)
{
    if (!define_builtin_typeinfo("bool", r_new_index))
        return;
    
    MCLog("[Emit] BoolType(-> %ld)", r_new_index);
}

void EmitIntType(long& r_new_index)
{
    if (!define_builtin_typeinfo("int", r_new_index))
        return;
    
    MCLog("[Emit] IntType(-> %ld)", r_new_index);
}

void EmitUIntType(long& r_new_index)
{
    if (!define_builtin_typeinfo("uint", r_new_index))
        return;
    
    MCLog("[Emit] UIntType(-> %ld)", r_new_index);
}

void EmitFloatType(long& r_new_index)
{
    if (!define_builtin_typeinfo("float", r_new_index))
        return;
    
    MCLog("[Emit] FloatType(-> %ld)", r_new_index);
}

void EmitDoubleType(long& r_new_index)
{
    if (!define_builtin_typeinfo("double", r_new_index))
        return;
    
    MCLog("[Emit] DoubleType(-> %ld)", r_new_index);
}

void EmitAnyType(long& r_new_index)
{
    if (!define_builtin_typeinfo("any", r_new_index))
        return;
    
    MCLog("[Emit] AnyType(-> %ld)", r_new_index);
}

void EmitBooleanType(long& r_new_index)
{
    if (!define_builtin_typeinfo("boolean", r_new_index))
        return;
    
    MCLog("[Emit] BooleanType(-> %ld)", r_new_index);
}

void EmitIntegerType(long& r_new_index)
{
    if (!define_builtin_typeinfo("number", r_new_index))
        return;
    
    MCLog("[Emit] IntegerType(-> %ld)", r_new_index);
}

void EmitRealType(long& r_new_index)
{
    // TODO: Real / Integer types.
    if (!define_builtin_typeinfo("number", r_new_index))
        return;
    
    MCLog("[Emit] RealType(-> %ld)", r_new_index);
}

void EmitNumberType(long& r_new_index)
{
    if (!define_builtin_typeinfo("number", r_new_index))
        return;
    
    MCLog("[Emit] NumberType(-> %ld)", r_new_index);
}

void EmitStringType(long& r_new_index)
{
    if (!define_builtin_typeinfo("string", r_new_index))
        return;
    
    MCLog("[Emit] StringType(-> %ld)", r_new_index);
}

void EmitDataType(long& r_new_index)
{
    if (!define_builtin_typeinfo("data", r_new_index))
        return;
    
    MCLog("[Emit] DataType(-> %ld)", r_new_index);
}

void EmitArrayType(long& r_new_index)
{
    if (!define_builtin_typeinfo("array", r_new_index))
        return;
    
    MCLog("[Emit] ArrayType(-> %ld)", r_new_index);
}

void EmitListType(long& r_new_index)
{
    if (!define_builtin_typeinfo("list", r_new_index))
        return;
    
    MCLog("[Emit] ListType(-> %ld)", r_new_index);
}

void EmitUndefinedType(long& r_new_index)
{
    if (!define_builtin_typeinfo("undefined", r_new_index))
        return;
    
    MCLog("[Emit] UndefinedType(-> %ld)", r_new_index);
}

//////////

static MCTypeInfoRef s_current_record_basetype = nil;
static MCRecordTypeFieldInfo *s_current_record_fields = nil;
static uindex_t s_current_record_field_count = 0;

void EmitBeginRecordType(long p_base_type_index)
{
    MCScriptBeginRecordTypeInModule(s_builder, p_base_type_index);
    MCLog("[Emit] BeginRecordType(%ld)", p_base_type_index);
}

void EmitRecordTypeField(NameRef name, long type_index)
{
    MCScriptContinueRecordTypeInModule(s_builder, to_mcnameref(name), type_index);
    MCLog("[Emit] RecordTypeField(%@, %ld)", to_mcnameref(name), type_index);
}

void EmitEndRecordType(long& r_type_index)
{
    uindex_t t_index;
    MCScriptEndRecordTypeInModule(s_builder, t_index);
    r_type_index = t_index;
    
    MCLog("[Emit] EndRecordType(-> %ld)", r_type_index);
}

//////////

static MCTypeInfoRef s_current_handler_returntype = nil;
static MCHandlerTypeFieldInfo *s_current_handler_fields = nil;
static uindex_t s_current_handler_field_count = 0;

void EmitBeginHandlerType(long return_type_index)
{
    MCScriptBeginHandlerTypeInModule(s_builder, return_type_index);
    MCLog("[Emit] BeginRecordType(%ld)", return_type_index);
}

static void EmitHandlerTypeParameter(MCHandlerTypeFieldMode mode, NameRef name, long type_index)
{
    MCScriptContinueHandlerTypeInModule(s_builder, (MCScriptHandlerTypeParameterMode)mode, to_mcnameref(name), type_index);
    MCLog("[Emit] HandlerTypeParameter(%d, %@, %ld)", mode, to_mcnameref(name), type_index);
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
    uindex_t t_index;
    MCScriptEndHandlerTypeInModule(s_builder, t_index);
    r_type_index = t_index;
    MCLog("[Emit] EndHandlerType(-> %ld)", t_index);
}

///////////

void EmitHandlerParameter(NameRef name, long type_index, long& r_index)
{
    uindex_t t_index;
    MCScriptAddParameterToHandlerInModule(s_builder, to_mcnameref(name), type_index, t_index);
    r_index = t_index;
    
    MCLog("[Emit] HandlerParameter(%@, %ld -> %ld)", to_mcnameref(name), type_index, r_index);
}

void EmitHandlerVariable(NameRef name, long type_index, long& r_index)
{
    uindex_t t_index;
    MCScriptAddVariableToHandlerInModule(s_builder, to_mcnameref(name), type_index, t_index);
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
        s_registers[t_reg] = 1;
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

void EmitBeginBuiltinInvoke(long name, long resultreg)
{
    // TODO: Builtin invoke
    MCScriptBeginInvokeInModule(s_builder, 0, resultreg);
    MCLog("[Emit] BeginBuiltinInvoke(%s, %ld)", (const char *)name, resultreg);
}

void EmitBeginExecuteInvoke(long index, long contextreg, long resultreg)
{
    MCScriptBeginInvokeInModule(s_builder, index, resultreg);
    MCLog("[Emit] BeginExecuteInvoke(%ld, %ld, %ld)", index, contextreg, resultreg);
}

#if 0
void EmitBeginEvaluateInvoke(long index, long contextreg, long outputreg)
{
    MCScriptBeginInvokeEvaluateInModule(s_builder, index, outputreg);
    MCLog("[Emit] BeginEvaluateInvoke(%ld, %ld, %ld)", index, contextreg, outputreg);
}

void EmitBeginAssignInvoke(long index, long contextreg, long inputreg)
{
    MCScriptBeginInvokeAssignInModule(s_builder, index, inputreg);
    MCLog("[Emit] BeginAssignInvoke(%ld, %ld, %ld)", index, contextreg, inputreg);
}

void EmitBeginIterateInvoke(long index, long contextreg, long iteratorreg, long containerreg)
{
    // TODO: Iterate invoke
    MCLog("[Emit] BeginIterateInvoke(%ld, %ld, %ld)", index, contextreg, iteratorreg, containerreg);
}
#endif

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
    MCScriptEmitAssignConstantInModule(s_builder, reg, kMCTrue);
    MCLog("[Emit] AssignUndefined(%ld)", reg);
}

void EmitAssignFalse(long reg)
{
    MCScriptEmitAssignConstantInModule(s_builder, reg, kMCFalse);
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

void EmitAssign(long dst, long src)
{
    MCScriptEmitAssignInModule(s_builder, dst, src);
    MCLog("[Emit] Assign(%ld, %ld)", dst, src);
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

////////

struct AttachedReg
{
    AttachedReg *next;
    long expr;
    long reg;
};

static AttachedReg *s_attached_regs = nil;

static bool FindAttachedReg(long expr, AttachedReg*& r_attach)
{
    for(AttachedReg *t_reg = s_attached_regs; t_reg != nil; t_reg = t_reg -> next)
        if (t_reg-> expr == expr)
        {
            r_attach = t_reg;
            return true;
        }
    
    return false;
}

void EmitAttachRegisterToExpression(long reg, long expr)
{
    AttachedReg *t_attach;
    if (FindAttachedReg(expr, t_attach))
        Fatal_InternalInconsistency("Register attached to expression which is already attached");
    
    MCMemoryNew(t_attach);
    t_attach -> next = s_attached_regs;
    t_attach -> expr = expr;
    t_attach -> reg = reg;
    s_attached_regs = t_attach;
}

void EmitDetachRegisterFromExpression(long expr)
{
    if (s_attached_regs == nil)
        return;
    
    AttachedReg *t_remove;
    t_remove = nil;
    
    if (s_attached_regs -> expr == expr)
    {
        t_remove = s_attached_regs;
        s_attached_regs = s_attached_regs -> next;
    }
    else
    {
        for(AttachedReg *t_reg = s_attached_regs; t_reg -> next != nil; t_reg = t_reg -> next)
            if (t_reg -> next -> expr == expr)
            {
                t_remove = t_reg -> next;
                t_reg -> next = t_reg -> next -> next;
                break;
            }
    }
    
    MCMemoryDelete(t_remove);
}

int EmitIsRegisterAttachedToExpression(long expr)
{
    AttachedReg *t_attach;
    return FindAttachedReg(expr, t_attach);
}

int EmitGetRegisterAttachedToExpression(long expr, long *reg)
{
    AttachedReg *t_attach;
    if (!FindAttachedReg(expr, t_attach))
        return 0;
    
    *reg = t_attach -> reg;

    return 1;
}

void EmitPosition(PositionRef p_position)
{
    FileRef t_file;
    GetFileOfPosition(p_position, &t_file);
    const char *t_filename;
    GetFileName(t_file, &t_filename);
    NameRef t_filename_name;
    MakeNameLiteral(t_filename, &t_filename_name);
    long t_line;
    GetRowOfPosition(p_position, &t_line);
    MCScriptEmitPositionInModule(s_builder, to_mcnameref(t_filename_name), t_line);
}

//////////

static FILE *s_output = NULL;

void OutputBeginManifest(void)
{
    s_output = OpenManifestOutputFile();
}

void OutputWrite(const char *p_string)
{
    if (s_output == NULL)
        return;
    
    fprintf(s_output, "%s", p_string);
}

void OutputWriteS(const char *p_left, const char *p_string, const char *p_right)
{
    fprintf(s_output, "%s%s%s", p_left, p_string, p_right);
}

void OutputWriteI(const char *p_left, NameRef p_name, const char *p_right)
{
    const char *t_name_string;
    GetStringOfNameLiteral(p_name, &t_name_string);
    OutputWriteS(p_left, t_name_string, p_right);
}

void OutputEnd(void)
{
    fclose(s_output);
}

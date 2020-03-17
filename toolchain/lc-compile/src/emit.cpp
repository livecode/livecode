/* Copyright (C) 2003-2016 LiveCode Ltd.
 
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

#include "foundation.h"
#include "foundation-auto.h"
#include "libscript/script.h"
#include "libscript/script-auto.h"

#include <output.h>
#include <allocate.h>
#include <position.h>
#include <report.h>
#include <literal.h>
#include "outputfile.h"

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/stat.h>

#ifndef WIN32
#include <unistd.h>
#endif

extern "C" int OutputFileAsC;
int OutputFileAsC = 0;
extern "C" int OutputFileAsAuxC;
int OutputFileAsAuxC = 0;
extern "C" int OutputFileAsBytecode;
int OutputFileAsBytecode = 0;

extern "C" void EmitStart(void);
extern "C" void EmitFinish(void);

extern "C" void EmitBeginModule(NameRef name, intptr_t& r_index);
extern "C" void EmitBeginWidgetModule(NameRef name, intptr_t& r_index);
extern "C" void EmitBeginLibraryModule(NameRef name, intptr_t& r_index);
extern "C" void EmitEndModule(void);
extern "C" void EmitModuleDependency(NameRef name, intptr_t& r_index);
extern "C" void EmitImportedType(intptr_t module_index, NameRef name, intptr_t type_index, intptr_t& r_index);
extern "C" void EmitImportedConstant(intptr_t module_index, NameRef name, intptr_t type_index, intptr_t& r_index);
extern "C" void EmitImportedVariable(intptr_t module_index, NameRef name, intptr_t type_index, intptr_t& r_index);
extern "C" void EmitImportedHandler(intptr_t module_index, NameRef name, intptr_t type_index, intptr_t& r_index);
extern "C" void EmitImportedSyntax(intptr_t p_module_index, NameRef p_name, intptr_t p_type_index, intptr_t& r_index);
extern "C" void EmitExportedDefinition(intptr_t index);
extern "C" void EmitDefinitionIndex(const char *type, intptr_t& r_index);
extern "C" void EmitTypeDefinition(intptr_t index, PositionRef position, NameRef name, intptr_t type_index);
extern "C" void EmitConstantDefinition(intptr_t p_index, PositionRef p_position, NameRef p_name, intptr_t p_const_index);
extern "C" void EmitVariableDefinition(intptr_t index, PositionRef position, NameRef name, intptr_t type_index);
extern "C" void EmitBeginHandlerDefinition(intptr_t index, PositionRef position, NameRef name, intptr_t type_index);
extern "C" void EmitBeginUnsafeHandlerDefinition(intptr_t index, PositionRef position, NameRef name, intptr_t type_index);
extern "C" void EmitEndHandlerDefinition(void);
extern "C" void EmitForeignHandlerDefinition(intptr_t index, PositionRef position, NameRef name, intptr_t type_index, intptr_t binding);
extern "C" void EmitEventDefinition(intptr_t p_index, PositionRef p_position, NameRef p_name, intptr_t p_type_index);
extern "C" void EmitPropertyDefinition(intptr_t p_index, PositionRef p_position, NameRef p_name, intptr_t setter, intptr_t getter);

extern "C" void EmitBeginSyntaxDefinition(intptr_t p_index, PositionRef p_position, NameRef p_name);
extern "C" void EmitEndSyntaxDefinition(void);
extern "C" void EmitBeginSyntaxMethod(intptr_t p_handler_index);
extern "C" void EmitEndSyntaxMethod(void);
extern "C" void EmitUndefinedSyntaxMethodArgument(void);
extern "C" void EmitTrueSyntaxMethodArgument(void);
extern "C" void EmitFalseSyntaxMethodArgument(void);
extern "C" void EmitInputSyntaxMethodArgument(void);
extern "C" void EmitOutputSyntaxMethodArgument(void);
extern "C" void EmitContextSyntaxMethodArgument(void);
extern "C" void EmitIteratorSyntaxMethodArgument(void);
extern "C" void EmitContainerSyntaxMethodArgument(void);
extern "C" void EmitIntegerSyntaxMethodArgument(intptr_t p_int);
extern "C" void EmitRealSyntaxMethodArgument(intptr_t p_double);
extern "C" void EmitStringSyntaxMethodArgument(intptr_t p_string);
extern "C" void EmitVariableSyntaxMethodArgument(intptr_t p_index);
extern "C" void EmitIndexedVariableSyntaxMethodArgument(intptr_t p_var_index, intptr_t p_element_index);

extern "C" void EmitBeginDefinitionGroup(void);
extern "C" void EmitContinueDefinitionGroup(intptr_t index);
extern "C" void EmitEndDefinitionGroup(intptr_t *r_index);

extern "C" void EmitDefinedType(intptr_t index, intptr_t& r_type_index);
extern "C" void EmitForeignType(intptr_t binding, intptr_t& r_type_index);
extern "C" void EmitNamedType(NameRef module_name, NameRef name, intptr_t& r_new_index);
extern "C" void EmitAliasType(NameRef name, intptr_t typeindex, intptr_t& r_new_index);
extern "C" void EmitOptionalType(intptr_t index, intptr_t& r_new_index);
extern "C" void EmitAnyType(intptr_t& r_new_index);
extern "C" void EmitBooleanType(intptr_t& r_new_index);
extern "C" void EmitIntegerType(intptr_t& r_new_index);
extern "C" void EmitRealType(intptr_t& r_new_index);
extern "C" void EmitNumberType(intptr_t& r_new_index);
extern "C" void EmitStringType(intptr_t& r_new_index);
extern "C" void EmitDataType(intptr_t& r_new_index);
extern "C" void EmitArrayType(intptr_t& r_new_index);
extern "C" void EmitListType(intptr_t& r_new_index);
extern "C" void EmitUndefinedType(intptr_t& r_new_index);
extern "C" void EmitBeginRecordType(void);
extern "C" void EmitRecordTypeField(NameRef name, intptr_t type_index);
extern "C" void EmitEndRecordType(intptr_t& r_type_index);
extern "C" void EmitBeginHandlerType(intptr_t return_type_index);
extern "C" void EmitBeginForeignHandlerType(intptr_t return_type_index);
extern "C" void EmitHandlerTypeInParameter(NameRef name, intptr_t type_index);
extern "C" void EmitHandlerTypeOutParameter(NameRef name, intptr_t type_index);
extern "C" void EmitHandlerTypeInOutParameter(NameRef name, intptr_t type_index);
extern "C" void EmitHandlerTypeVariadicParameter(NameRef name);
extern "C" void EmitEndHandlerType(intptr_t& r_index);
extern "C" void EmitHandlerParameter(NameRef name, intptr_t type_index, intptr_t& r_index);
extern "C" void EmitHandlerVariable(NameRef name, intptr_t type_index, intptr_t& r_index);
extern "C" void EmitDeferLabel(intptr_t& r_label);
extern "C" void EmitResolveLabel(intptr_t label);
extern "C" void EmitCreateRegister(intptr_t& r_regindex);
extern "C" void EmitDestroyRegister(intptr_t regindex);
extern "C" void EmitBeginOpcode(intptr_t opcode);
extern "C" void EmitContinueOpcode(intptr_t argument);
extern "C" void EmitEndOpcode(void);
extern "C" void EmitJump(intptr_t label);
extern "C" void EmitJumpIfTrue(intptr_t reg, intptr_t label);
extern "C" void EmitJumpIfFalse(intptr_t reg, intptr_t label);
extern "C" void EmitPushRepeatLabels(intptr_t next, intptr_t exit);
extern "C" void EmitPopRepeatLabels(void);
extern "C" void EmitCurrentRepeatLabels(intptr_t& r_next, intptr_t& r_exit);
extern "C" void EmitBeginCall(intptr_t index, intptr_t resultreg);
extern "C" void EmitBeginIndirectCall(intptr_t reg, intptr_t resultreg);
extern "C" void EmitContinueCall(intptr_t reg);
extern "C" void EmitEndCall(void);
extern "C" void EmitBeginInvoke(intptr_t index, intptr_t contextreg, intptr_t resultreg);
extern "C" void EmitBeginIndirectInvoke(intptr_t handlerreg, intptr_t contextreg, intptr_t resultreg);
extern "C" void EmitContinueInvoke(intptr_t reg);
extern "C" void EmitEndInvoke(void);
extern "C" void EmitAssign(intptr_t dst, intptr_t src);
extern "C" void EmitAssignConstant(intptr_t dst, intptr_t constidx);
extern "C" void EmitUndefinedConstant(intptr_t *idx);
extern "C" void EmitTrueConstant(intptr_t *idx);
extern "C" void EmitFalseConstant(intptr_t *idx);
extern "C" void EmitIntegerConstant(intptr_t value, intptr_t *idx);
extern "C" void EmitUnsignedIntegerConstant(uintptr_t value, intptr_t *idx);
extern "C" void EmitRealConstant(intptr_t value, intptr_t *idx);
extern "C" void EmitStringConstant(intptr_t value, intptr_t *idx);
extern "C" void EmitBeginListConstant(void);
extern "C" void EmitContinueListConstant(intptr_t idx);
extern "C" void EmitEndListConstant(intptr_t *idx);
extern "C" void EmitBeginArrayConstant(void);
extern "C" void EmitContinueArrayConstant(intptr_t key_idx, intptr_t value_idx);
extern "C" void EmitEndArrayConstant(intptr_t *idx);
extern "C" void EmitBeginAssignList(intptr_t reg);
extern "C" void EmitContinueAssignList(intptr_t reg);
extern "C" void EmitEndAssignList(void);
extern "C" void EmitBeginAssignArray(intptr_t reg);
extern "C" void EmitContinueAssignArray(intptr_t reg);
extern "C" void EmitEndAssignArray(void);
extern "C" void EmitFetch(intptr_t reg, intptr_t var, intptr_t level);
extern "C" void EmitStore(intptr_t reg, intptr_t var, intptr_t level);
extern "C" void EmitReturn(intptr_t reg);
extern "C" void EmitReturnNothing(void);
extern "C" void EmitReset(intptr_t reg);
extern "C" void EmitAttachRegisterToExpression(intptr_t reg, intptr_t expr);
extern "C" void EmitDetachRegisterFromExpression(intptr_t expr);
extern "C" int EmitGetRegisterAttachedToExpression(intptr_t expr, intptr_t *reg);
extern "C" void EmitPosition(PositionRef position);

extern "C" void OutputBeginManifest(void);

extern "C" int IsBootstrapCompile(void);
extern "C" int IsNotBytecodeOutput(void);

extern "C" int ForceCBindingsAsBuiltins(void);

extern "C" void DependStart(void);
extern "C" void DependFinish(void);
extern "C" void DependDefineMapping(NameRef module_name, const char *source_file);
extern "C" void DependDefineDependency(NameRef module_name, NameRef dependency_name);

extern "C" int BytecodeEnumerate(intptr_t index, intptr_t *r_name);
extern "C" int BytecodeLookup(intptr_t name, intptr_t *r_opcode);
extern "C" void BytecodeDescribe(intptr_t opcode, intptr_t *r_name);
extern "C" int BytecodeIsValidArgumentCount(intptr_t opcode, intptr_t count);
extern "C" void BytecodeDescribeParameter(intptr_t opcode, intptr_t index, intptr_t *r_type);

//////////

struct AttachedReg
{
    AttachedReg *next;
    intptr_t expr;
    intptr_t reg;
};

static AttachedReg *s_attached_regs = nil;

void EmitCheckNoRegisters(void);

//////////

//static MCTypeInfoRef *s_typeinfos = nil;
//static uindex_t s_typeinfo_count = 0;

static MCNameRef to_mcnameref(NameRef p_name)
{
    const char *t_cstring;
    GetStringOfNameLiteral(p_name, &t_cstring);

    /* Names _may_ contain non-ASCII characters.  Therefore interpret
     * them as UTF-8.  This is particularly important for properties,
     * for example, which generate a name but via a string literal. */
    MCAutoStringRef t_string;
    MCStringCreateWithBytes(reinterpret_cast<const byte_t *>(t_cstring),
                            (uindex_t)strlen(t_cstring), kMCStringEncodingUTF8,
                            false, &t_string);
    
    MCNameRef t_name;
    MCNameCreate(*t_string, t_name);

    return t_name;
}

static const char *
cstring_from_nameref(NameRef p_name)
{
	const char *t_cstring;
	GetStringOfNameLiteral(p_name, &t_cstring);
	return t_cstring;
}

static NameRef
nameref_from_cstring(const char *p_cstring)
{
    NameRef t_name;
    MakeNameLiteral(p_cstring, &t_name);
    return t_name;
}

static MCStringRef to_mcstringref(intptr_t p_string)
{
    MCAutoStringRef t_string;
    MCStringCreateWithBytes(reinterpret_cast<const byte_t *>(p_string),
                            (uindex_t)strlen(reinterpret_cast<const char *>(p_string)),
                            kMCStringEncodingUTF8, false, &t_string);
    MCAutoStringRef t_uniq_string;
    MCValueInter(*t_string, &t_uniq_string);
    return t_uniq_string.Take();
}

static NameRef nameref_from_mcstringref(MCStringRef p_string)
{
    MCAutoPointer<char> t_utf8_string;
    MCStringConvertToUTF8String(p_string, &t_utf8_string);
    
    NameRef t_name;
    MakeNameLiteral(*t_utf8_string, &t_name);
    
    return t_name;
}

//////////

static MCScriptModuleBuilderRef s_builder;
static NameRef s_module_name;

static NameRef *s_ordered_modules = NULL;
static unsigned int s_ordered_module_count;

static FILE *s_output_code_file = NULL;
static const char *s_output_code_filename = NULL;

//////////

struct CompiledModule
{
    CompiledModule *next;
    NameRef name;
    byte_t *bytecode;
    size_t bytecode_len;
};

static CompiledModule *s_compiled_modules = NULL;

//////////

struct EmittedModule
{
    EmittedModule *next;
    NameRef name;
    char *modified_name;
    bool has_foreign : 1;
};
static EmittedModule *s_emitted_modules = NULL;

static void EmittedModuleAdd(NameRef p_module_name, char *p_modified_name)
{
    EmittedModule *t_mod;
    t_mod = (EmittedModule *)Allocate(sizeof(EmittedModule));
    t_mod -> next = s_emitted_modules;
    t_mod -> name = p_module_name;
    t_mod -> modified_name = p_modified_name;
    s_emitted_modules = t_mod;
}

static bool EmitEmittedModules(void)
{
    if (!OutputFileAsC)
    {
        return true;
    }

	FILE *t_file = s_output_code_file;
	if (nullptr == t_file)
    {
		return false;
    }
    
    for(EmittedModule *t_module = s_emitted_modules; t_module != nullptr; t_module = t_module->next)
    {
        const char *t_modified_name = t_module->modified_name;
        
        if (0 > fprintf(t_file,
                        "static __builtin_module_info __%s_module_info =\n{\n    0,\n    0,\n    %s_module_data,\n    sizeof(%s_module_data),\n",
                        t_modified_name,
                        t_modified_name,
                        t_modified_name))
            return false;
        
        if (!ForceCBindingsAsBuiltins())
        {
            if (0 > fprintf(t_file,
                            "    %s_Initialize,\n    %s_Finalize,\n",
                            t_modified_name,
                            t_modified_name))
                return false;
        }
        else
        {
            if (0 > fprintf(t_file,
                            "    nullptr,\n    nullptr,\n"))
                return false;
        }
    
        if (0 > fprintf(t_file,
                        "    __builtin_ordinal_map\n};\n__builtin_module_loader __%s_loader(&__%s_module_info);\n\n",
                        t_modified_name,
                        t_modified_name))
            return false;
    }
    
    return true;
}

//////////

struct EmittedBuiltin
{
    EmittedBuiltin *next;
    NameRef name;
    uindex_t ordinal;
    MCStringRef ordinal_stringref;
    
    MCScriptForeignPrimitiveType return_type;
    uindex_t parameter_count;
    MCScriptForeignPrimitiveType *parameter_types;
};

static EmittedBuiltin *s_emitted_builtins = nullptr;
static uindex_t s_emitted_builtin_count = 0;

static MCStringRef EmittedBuiltinAdd(NameRef p_symbol_name, uindex_t p_type_index, bool p_force)
{
    if (!p_force && (!OutputFileAsC || OutputFileAsAuxC))
    {
        return MCNameGetString(to_mcnameref(p_symbol_name));
    }

    /* If this is a parameterized foriegn type, then don't shim it */
    if (p_type_index == UINDEX_MAX &&
        strchr(cstring_from_nameref(p_symbol_name), ':') != NULL)
    {
        return MCNameGetString(to_mcnameref(p_symbol_name));
    }
    
    for(EmittedBuiltin *t_builtin = s_emitted_builtins; t_builtin != nullptr; t_builtin = t_builtin->next)
    {
        if (t_builtin->name == p_symbol_name)
        {
            return t_builtin->ordinal_stringref;
        }
    }
    
    EmittedBuiltin *t_builtin;
    t_builtin = (EmittedBuiltin *)Allocate(sizeof(EmittedBuiltin));
    t_builtin -> next = s_emitted_builtins;
    t_builtin -> name = p_symbol_name;
    t_builtin -> ordinal = s_emitted_builtin_count;
    
    if (p_type_index != UINDEX_MAX)
    {
        t_builtin->return_type = MCScriptQueryForeignHandlerReturnTypeInModule(s_builder, p_type_index);
        t_builtin->parameter_count = MCScriptQueryForeignHandlerParameterCountInModule(s_builder, p_type_index);
        t_builtin->parameter_types = new MCScriptForeignPrimitiveType[t_builtin->parameter_count];
        for(uindex_t i = 0; i < t_builtin->parameter_count; i++)
        {
            t_builtin->parameter_types[i] = MCScriptQueryForeignHandlerParameterTypeInModule(s_builder, p_type_index, i);
        }
    }
    else
    {
        t_builtin->return_type = kMCScriptForeignPrimitiveTypePointer;
        t_builtin->parameter_count = 0;
        t_builtin->parameter_types = nullptr;
    }
    
    char t_ordinal_str[32];
    sprintf(t_ordinal_str, "%u", t_builtin->ordinal);
    t_builtin->ordinal_stringref = to_mcstringref((intptr_t)t_ordinal_str);
    
    s_emitted_builtins = t_builtin;
    s_emitted_builtin_count++;
    
    return t_builtin->ordinal_stringref;
};

#define DEFINE_PRIMITIVE_TYPE_MAPPING(PType, CType) \
    { kMCScriptForeignPrimitiveType##PType, #CType },

static struct { MCScriptForeignPrimitiveType type; const char *ctype; } s_primitive_type_map[] =
{
    DEFINE_PRIMITIVE_TYPE_MAPPING(Unknown, void)
    DEFINE_PRIMITIVE_TYPE_MAPPING(Void, void)
    DEFINE_PRIMITIVE_TYPE_MAPPING(Pointer, void*)
    DEFINE_PRIMITIVE_TYPE_MAPPING(SInt8, int8_t)
    DEFINE_PRIMITIVE_TYPE_MAPPING(UInt8, uint8_t)
    DEFINE_PRIMITIVE_TYPE_MAPPING(SInt16, int16_t)
    DEFINE_PRIMITIVE_TYPE_MAPPING(UInt16, uint16_t)
    DEFINE_PRIMITIVE_TYPE_MAPPING(SInt32, int32_t)
    DEFINE_PRIMITIVE_TYPE_MAPPING(UInt32, uint32_t)
    DEFINE_PRIMITIVE_TYPE_MAPPING(SInt64, int64_t)
    DEFINE_PRIMITIVE_TYPE_MAPPING(UInt64, uint64_t)
    DEFINE_PRIMITIVE_TYPE_MAPPING(SIntSize, ssize_t)
    DEFINE_PRIMITIVE_TYPE_MAPPING(UIntSize, size_t)
    DEFINE_PRIMITIVE_TYPE_MAPPING(SIntPtr, intptr_t)
    DEFINE_PRIMITIVE_TYPE_MAPPING(UIntPtr, uintptr_t)
    DEFINE_PRIMITIVE_TYPE_MAPPING(Float32, float)
    DEFINE_PRIMITIVE_TYPE_MAPPING(Float64, double)
    DEFINE_PRIMITIVE_TYPE_MAPPING(CBool, bool)
    DEFINE_PRIMITIVE_TYPE_MAPPING(CChar, char)
    DEFINE_PRIMITIVE_TYPE_MAPPING(CSChar, signed char)
    DEFINE_PRIMITIVE_TYPE_MAPPING(CUChar, unsigned char)
    DEFINE_PRIMITIVE_TYPE_MAPPING(CSShort, signed short)
    DEFINE_PRIMITIVE_TYPE_MAPPING(CUShort, unsigned short)
    DEFINE_PRIMITIVE_TYPE_MAPPING(CSInt, signed int)
    DEFINE_PRIMITIVE_TYPE_MAPPING(CUInt, unsigned int)
    DEFINE_PRIMITIVE_TYPE_MAPPING(CSLong, signed long)
    DEFINE_PRIMITIVE_TYPE_MAPPING(CULong, unsigned long)
    DEFINE_PRIMITIVE_TYPE_MAPPING(CSLongLong, signed long long)
    DEFINE_PRIMITIVE_TYPE_MAPPING(CULongLong, unsigned long long)
    DEFINE_PRIMITIVE_TYPE_MAPPING(CFloat, float)
    DEFINE_PRIMITIVE_TYPE_MAPPING(CDouble, double)
    DEFINE_PRIMITIVE_TYPE_MAPPING(SInt, int32_t)
    DEFINE_PRIMITIVE_TYPE_MAPPING(UInt, uint32_t)
    DEFINE_PRIMITIVE_TYPE_MAPPING(NaturalUInt, natural_uint_t)
    DEFINE_PRIMITIVE_TYPE_MAPPING(NaturalSInt, natural_sint_t)
    DEFINE_PRIMITIVE_TYPE_MAPPING(NaturalFloat, natural_float_t)
};

#undef DEFINE_PRIMITIVE_TYPE_MAPPING

static const char *CTypeFromPrimitiveType(MCScriptForeignPrimitiveType p_type)
{
    if (p_type == kMCScriptForeignPrimitiveTypeUnknown)
    {
        Fatal_InternalInconsistency("type bound in foreign handler unknown");
        return nullptr;
    }

    if (s_primitive_type_map[p_type].type != p_type)
    {
        Fatal_InternalInconsistency("s_primitive_type_map out of order");
        return nullptr;
    }
    
    return s_primitive_type_map[p_type].ctype;
}

static bool EmitEmittedBuiltins(void)
{
    if (!OutputFileAsC)
    {
        return true;
    }
    
	FILE *t_file = s_output_code_file;
	if (nullptr == t_file)
    {
		return false;
    }
    
    MCAutoArray<EmittedBuiltin*> t_builtins;
    if (!t_builtins.Resize(s_emitted_builtin_count))
    {
        return false;
    }
    
    for(EmittedBuiltin *t_builtin = s_emitted_builtins; t_builtin != nullptr; t_builtin = t_builtin->next)
    {
        t_builtins[t_builtin->ordinal] = t_builtin;
    }
    
    // Emit builtin symbol external references
    if (0 > fprintf(t_file, "\n"))
    {
        return false;
    }
    
    for(uindex_t i = 0; i < s_emitted_builtin_count; i++)
    {
        /* Generate extern for original function */
        if (0 > fprintf(t_file,
                        "extern \"C\" %s %s(",
                        CTypeFromPrimitiveType(t_builtins[i]->return_type),
                        cstring_from_nameref(t_builtins[i]->name)))
        {
            return false;
        }
        for(uindex_t t_arg_index = 0; t_arg_index < t_builtins[i]->parameter_count; t_arg_index++)
        {
            if (0 > fprintf(t_file,
                            "%s%s",
                            t_arg_index > 0 ? ", " : "",
                            CTypeFromPrimitiveType(t_builtins[i]->parameter_types[t_arg_index])))
            {
                return false;
            }
        }
        if (0 > fprintf(t_file,
                        ");\n"))
        {
            return false;
        }
        
        /* Generate shim around original function */
        if (0 > fprintf(t_file,
                        "static void shim__%s(void *rv, void **av)\n{\n",
                        cstring_from_nameref(t_builtins[i]->name)))
        {
            return false;
        }
        
        if (t_builtins[i]->return_type != kMCScriptForeignPrimitiveTypeVoid)
        {
            if (0 > fprintf(t_file,
                            "\t*(%s *)rv =",
                            CTypeFromPrimitiveType(t_builtins[i]->return_type)))
            {
                return false;
            }
        }
            
        if (0 > fprintf(t_file,
                        "%s(",
                        cstring_from_nameref(t_builtins[i]->name)))
        {
            return false;
        }
        for(uindex_t t_arg_index = 0; t_arg_index < t_builtins[i]->parameter_count; t_arg_index++)
        {
            if (0 > fprintf(t_file,
                            "%s*(%s *)av[%d]",
                            t_arg_index > 0 ? ", " : "",
                            CTypeFromPrimitiveType(t_builtins[i]->parameter_types[t_arg_index]),
                            t_arg_index))
            {
                return false;
            }
        }
        if (0 > fprintf(t_file,
                        ");\n}\n\n"))
        {
            return false;
        }
    }
    
    // Emit builtin ordinal mapping array
    if (0 > fprintf(t_file, "static __builtin_shim_type __builtin_ordinal_map[] =\n{\n"))
    {
        return false;
    }
    for(uindex_t i = 0; i < s_emitted_builtin_count; i++)
    {
        if (0 > fprintf(t_file, "\tshim__%s,\n", cstring_from_nameref(t_builtins[i]->name)))
        {
            return false;
        }
    }
    if (0 > fprintf(t_file, "\tnullptr\n};\n\n"))
    {
        return false;
    }

    return true;
}

//////////

static const char *kOutputCDefinitions =
    "#include <stdint.h>\n"
    "#include <stddef.h>\n\n"
    "#if UINTPTR_MAX == 0xffffffff\n"
    "typedef uint32_t natural_uint_t;\n"
    "typedef int32_t natural_sint_t;\n"
    "typedef float natural_float_t;\n"
    "#elif UINTPTR_MAX == 0xffffffffffffffff\n"
    "typedef uint64_t natural_uint_t;\n"
    "typedef int64_t natural_sint_t;\n"
    "typedef double natural_float_t;\n"
    "#endif\n\n"
    "typedef void (*__builtin_shim_type)(void*, void**);\n"
    "struct __builtin_module_info\n"
    "{\n"
    "    void *next;\n"
    "    void *handle;\n"
    "    unsigned char *data;\n"
    "    unsigned long length;\n"
    "    bool (*initializer)();\n"
    "    void (*finalizer)();\n"
    "    __builtin_shim_type *builtins;\n"
    "};\n"
    "extern \"C\" void MCScriptRegisterBuiltinModule(__builtin_module_info *desc);\n"
    "class __builtin_module_loader\n"
    "{\n"
    "public:\n"
    "    __builtin_module_loader(__builtin_module_info *p_desc)\n"
    "    {\n"
    "        MCScriptRegisterBuiltinModule(p_desc);\n"
    "    }\n"
    "};\n\n";

static void BuildBuiltinModule(void);

void EmitStart(void)
{
    MCInitialize();

    s_output_code_file = OpenOutputCodeFile(&s_output_code_filename);
    s_emitted_modules = NULL;
    
    if (s_output_code_file != NULL)
    {
        if (fprintf(s_output_code_file, "%s", kOutputCDefinitions) < 0)
            goto error_cleanup;
        
        if (OutputFileAsC &&
            !OutputFileAsAuxC)
            BuildBuiltinModule();
    }
    
    return;

error_cleanup:
	if (nil != s_output_code_file)
    {
		fclose (s_output_code_file);
        s_output_code_file = NULL;
    }
    Error_CouldNotWriteOutputFile (s_output_code_filename);
}

static void __EmitModuleOrder(NameRef p_name)
{
    for(unsigned int i = 0; i < s_ordered_module_count; i++)
        if (IsNameEqualToName(p_name, s_ordered_modules[i]))
            return;
    
    s_ordered_modules = (NameRef *)Reallocate(s_ordered_modules, sizeof(NameRef) * (s_ordered_module_count + 1));
    s_ordered_modules[s_ordered_module_count++] = p_name;
}

static bool
EmitCompiledModules (void)
{
    const char *t_filename = nil;
    FILE *t_file = OpenOutputBytecodeFile (&t_filename);
    
    if (nil == t_file)
        goto error_cleanup;
    
    while(s_compiled_modules != nullptr)
    {
        size_t t_written;
        t_written = fwrite (s_compiled_modules->bytecode, sizeof(byte_t), s_compiled_modules->bytecode_len, t_file);
    
        if (t_written != s_compiled_modules->bytecode_len)
            goto error_cleanup;
        
        s_compiled_modules = s_compiled_modules->next;
    }
    
    fflush (t_file);
    fclose (t_file);
    
    return true;
    
error_cleanup:
    if (nil != t_file)
        fclose (t_file);
    Error_CouldNotWriteOutputFile (t_filename);
    return false;
}

void EmitFinish(void)
{
    if (s_compiled_modules != NULL &&
        !EmitCompiledModules())
    {
        goto error_cleanup;
    }
    
    if (!EmitEmittedBuiltins())
    {
        goto error_cleanup;
    }

    if (!EmitEmittedModules())
    {
        goto error_cleanup;
    }

	if (nil != s_output_code_file)
    {
		fclose (s_output_code_file);
        s_output_code_file = NULL;
    }
    
    return;
    
error_cleanup:
	if (nil != s_output_code_file)
    {
		fclose (s_output_code_file);
        s_output_code_file = NULL;
    }
    Error_CouldNotWriteOutputFile (s_output_code_filename);
}

void EmitBeginModule(NameRef p_name, intptr_t& r_index)
{
    MCInitialize();

    Debug_Emit("BeginModule(%s) -> 0", cstring_from_nameref(p_name));

    s_module_name = p_name;
    
    MCScriptBeginModule(kMCScriptModuleKindNone, to_mcnameref(p_name), s_builder);
    r_index = 0;
}

void EmitBeginWidgetModule(NameRef p_name, intptr_t& r_index)
{
    MCInitialize();

    Debug_Emit("BeginWidgetModule(%s) -> 0", cstring_from_nameref(p_name));

    s_module_name = p_name;
    
    MCScriptBeginModule(kMCScriptModuleKindWidget, to_mcnameref(p_name), s_builder);
    r_index = 0;
}

void EmitBeginLibraryModule(NameRef p_name, intptr_t& r_index)
{
    MCInitialize();

    Debug_Emit("BeginLibraryModule(%s) -> 0", cstring_from_nameref(p_name));

    s_module_name = p_name;
    
    MCScriptBeginModule(kMCScriptModuleKindLibrary, to_mcnameref(p_name), s_builder);
    r_index = 0;
}

static bool
EmitEndModuleGetByteCodeBuffer (byte_t*& r_bytecode, size_t& r_bytecode_len)
{
	MCAutoValueRefBase<MCStreamRef> t_stream;
	MCMemoryOutputStreamCreate (&t_stream);

	if (!MCScriptEndModule (s_builder, *t_stream))
		goto error_cleanup;

	void *t_bytecode;
	size_t t_bytecode_len;
	MCMemoryOutputStreamFinish (*t_stream,
	                            t_bytecode,
	                            t_bytecode_len);

	MCAssert (t_bytecode_len <= UINDEX_MAX);
    r_bytecode = (byte_t*)t_bytecode;
    r_bytecode_len = t_bytecode_len;

	return true;

 error_cleanup:
	Error_CouldNotGenerateBytecode();
	return false;
}

static bool
EmitEndModuleOutputC (NameRef p_module_name,
                      const char *p_module_name_string,
                      const byte_t *p_bytecode,
                      size_t p_bytecode_len)
{
	char *t_modified_name = nil;
	FILE *t_file = nil;

	t_file = s_output_code_file;
	if (nil == t_file)
		goto error_cleanup;

	t_modified_name = strdup(p_module_name_string);
	if (nil == t_modified_name)
		goto error_cleanup;

	for(int i = 0; t_modified_name[i] != '\0'; i++)
		if (t_modified_name[i] == '.')
			t_modified_name[i] = '_';
    
    if (!ForceCBindingsAsBuiltins())
    {
        // Emit the initializer reference.
        if (0 > fprintf(s_output_code_file,
                        "extern \"C\" bool %s_Initialize(void);\n",
                        t_modified_name))
            goto error_cleanup;
        
        // Emit the finalizer reference.
        if (0 > fprintf(s_output_code_file,
                        "extern \"C\" void %s_Finalize(void);\n",
                        t_modified_name))
            goto error_cleanup;
    }
    
	if (0 > fprintf(t_file, "static unsigned char %s_module_data[] = {", t_modified_name))
		goto error_cleanup;

	for(size_t i = 0; i < p_bytecode_len; i++)
	{
		if ((i % 16) == 0)
			if (0 > fprintf(t_file, "\n"))
				goto error_cleanup;

		if (0 > fprintf(t_file, "0x%02x, ", ((unsigned char *)p_bytecode)[i]))
			goto error_cleanup;
	}
	if (0 > fprintf(t_file, "};\n"))
        goto error_cleanup;

    EmittedModuleAdd(p_module_name, t_modified_name);
    
	fflush (t_file);
	return true;

 error_cleanup:
	free (t_modified_name);
	if (nil != s_output_code_file)
    {
		fclose (s_output_code_file);
        s_output_code_file = NULL;
    }
    Error_CouldNotWriteOutputFile (s_output_code_filename);
	return false;
}

static bool
EmitEndModuleGetInterfaceBuffer (const byte_t *p_bytecode,
                                 const size_t p_bytecode_len,
                                 MCAutoByteArray & r_interface)
{
	MCAutoValueRefBase<MCStreamRef> t_stream;
	MCAutoScriptModuleRef t_module;
	MCAutoValueRefBase<MCStreamRef> t_output_stream;

	void *t_interface = nil;
	size_t t_interface_len = 0;

	if (!MCMemoryInputStreamCreate(p_bytecode, p_bytecode_len, &t_stream))
		goto error_cleanup;

	if (!MCScriptCreateModuleFromStream(*t_stream, &t_module))
		goto error_cleanup;

	if (!MCMemoryOutputStreamCreate(&t_output_stream))
		goto error_cleanup;

	if (!MCScriptWriteInterfaceOfModule(*t_module, *t_output_stream))
		goto error_cleanup;

	if (!MCMemoryOutputStreamFinish(*t_output_stream,
	                                t_interface, t_interface_len))
		goto error_cleanup;

	MCAssert (t_interface_len <= UINDEX_MAX);
	r_interface.Give ((byte_t *) t_interface, (uindex_t)t_interface_len);

	return true;

 error_cleanup:
	Error_CouldNotGenerateInterface();
	return false;
}

static bool
EmitEndModuleOutputInterface (const char *p_module_name,
                              const byte_t *p_interface,
                              size_t p_interface_len)
{
	char *t_filename = nil;
	FILE *t_import = nil;
	t_import = OpenImportedModuleFile(p_module_name, &t_filename);
	if (nil == t_import)
		goto error_cleanup;

	size_t t_written;
	t_written = fwrite (p_interface, sizeof(byte_t), p_interface_len, t_import);
	if (t_written != p_interface_len)
		goto error_cleanup;

	fflush (t_import);
	fclose (t_import);
	free (t_filename);

	return true;

 error_cleanup:
	if (nil != t_import)
		fclose (t_import);
	Error_CouldNotWriteInterfaceFile(t_filename);
	free (t_filename);
	return false;
}

void
EmitEndModule (void)
{
	const char *t_module_string = nil;

    byte_t *t_bytecode_buf = nil;
	size_t t_bytecode_len = 0;

	MCAutoByteArray t_interface;
	const byte_t *t_interface_buf = nil;
	size_t t_interface_len = 0;

	Debug_Emit("EndModule()");

    __EmitModuleOrder(s_module_name);
    
	GetStringOfNameLiteral(s_module_name, &t_module_string);
	MCAssert (nil != t_module_string);

	/* ---------- 1. Get bytecode */

	if (!EmitEndModuleGetByteCodeBuffer (t_bytecode_buf, t_bytecode_len))
		goto cleanup;

	/* ---------- 2. Output module contents */
	if (OutputFileAsC)
	{
		if (!EmitEndModuleOutputC (s_module_name, t_module_string,
		                           t_bytecode_buf, t_bytecode_len))
			goto cleanup;
	}
	else if (OutputFileAsBytecode)
	{
        CompiledModule *t_cmodule = (CompiledModule*)Allocate(sizeof(CompiledModule));
        t_cmodule->next = s_compiled_modules;
        t_cmodule->name = s_module_name;
        t_cmodule->bytecode = t_bytecode_buf;
        t_cmodule->bytecode_len = t_bytecode_len;
        s_compiled_modules = t_cmodule;
    }

	/* ---------- 3. Output module interface */
	if (!EmitEndModuleGetInterfaceBuffer (t_bytecode_buf, t_bytecode_len,
	                                      t_interface))
		goto cleanup;

	t_interface_buf = t_interface.Bytes();
	t_interface_len = t_interface.ByteCount();

	if (!EmitEndModuleOutputInterface (t_module_string,
	                                   t_interface_buf, t_interface_len))
		goto cleanup;

cleanup:
    return;
}

void EmitModuleDependency(NameRef p_name, intptr_t& r_index)
{
    __EmitModuleOrder(p_name);
    
    uindex_t t_index;
    MCScriptAddDependencyToModule(s_builder, to_mcnameref(p_name), t_index);
    r_index = t_index + 1;

    Debug_Emit("ModuleDependency(%s -> %d)", cstring_from_nameref(p_name), t_index + 1);
}

void EmitImportedType(intptr_t p_module_index, NameRef p_name, intptr_t p_type_index, intptr_t& r_index)
{
    uindex_t t_index;
    MCScriptAddImportToModule(s_builder, (uindex_t)p_module_index - 1, to_mcnameref(p_name), kMCScriptDefinitionKindType, (uindex_t)p_type_index, t_index);
    r_index = t_index;

    Debug_Emit("ImportedType(%ld, %s, %ld -> %d)", p_module_index,
               cstring_from_nameref(p_name), p_type_index, t_index);
}

void EmitImportedConstant(intptr_t p_module_index, NameRef p_name, intptr_t p_type_index, intptr_t& r_index)
{
    uindex_t t_index;
    MCScriptAddImportToModule(s_builder, (uindex_t)p_module_index - 1, to_mcnameref(p_name), kMCScriptDefinitionKindConstant, (uindex_t)p_type_index, t_index);
    r_index = t_index;

    Debug_Emit("ImportedConstant(%ld, %s, %ld -> %d)", p_module_index,
               cstring_from_nameref(p_name), p_type_index, t_index);
}

void EmitImportedVariable(intptr_t p_module_index, NameRef p_name, intptr_t p_type_index, intptr_t& r_index)
{
    uindex_t t_index;
    MCScriptAddImportToModule(s_builder, (uindex_t)p_module_index - 1, to_mcnameref(p_name), kMCScriptDefinitionKindVariable, (uindex_t)p_type_index, t_index);
    r_index = t_index;

    Debug_Emit("ImportedVariable(%ld, %s, %ld -> %d)", p_module_index,
               cstring_from_nameref(p_name), p_type_index, t_index);
}

void EmitImportedHandler(intptr_t p_module_index, NameRef p_name, intptr_t p_type_index, intptr_t& r_index)
{
    uindex_t t_index;
    MCScriptAddImportToModule(s_builder, (uindex_t)p_module_index - 1, to_mcnameref(p_name), kMCScriptDefinitionKindHandler, (uindex_t)p_type_index, t_index);
    r_index = t_index;

    Debug_Emit("ImportedHandler(%ld, %s, %ld -> %d)", p_module_index,
               cstring_from_nameref(p_name), p_type_index, t_index);
}

void EmitImportedSyntax(intptr_t p_module_index, NameRef p_name, intptr_t p_type_index, intptr_t& r_index)
{
    uindex_t t_index;
    MCScriptAddImportToModule(s_builder, (uindex_t)p_module_index - 1, to_mcnameref(p_name), kMCScriptDefinitionKindSyntax, (uindex_t)p_type_index, t_index);
    r_index = t_index;

    Debug_Emit("ImportedSyntax(%ld, %s, %ld -> %d)", p_module_index,
               cstring_from_nameref(p_name), p_type_index, t_index);
}

void EmitExportedDefinition(intptr_t p_index)
{
    MCScriptAddExportToModule(s_builder, (uindex_t)p_index);

    Debug_Emit("ExportedDefinition(%ld)", p_index);
}

void EmitDefinitionIndex(const char *p_type, intptr_t& r_index)
{
    static const struct { const char *name; MCScriptDefinitionKind kind; } kTypeMap[] =
    {
        { "external", kMCScriptDefinitionKindExternal },
        { "type", kMCScriptDefinitionKindType },
        { "constant", kMCScriptDefinitionKindConstant },
        { "variable", kMCScriptDefinitionKindVariable },
        { "handler", kMCScriptDefinitionKindHandler },
        { "foreignhandler", kMCScriptDefinitionKindForeignHandler },
        { "property", kMCScriptDefinitionKindProperty },
        { "event", kMCScriptDefinitionKindEvent },
        { "syntax", kMCScriptDefinitionKindSyntax },
        { "definitiongroup", kMCScriptDefinitionKindDefinitionGroup },
    };
    static const int kTypeMapLength = sizeof(kTypeMap) / sizeof(kTypeMap[0]);
    
    int t_kind_index = -1;
    for(int i = 0; i < kTypeMapLength; i++)
    {
        if (strcmp(kTypeMap[i].name, p_type) == 0)
        {
            t_kind_index = i;
            break;
        }
    }
    
    if (t_kind_index == -1)
        Fatal_InternalInconsistency("unknown definition kind");
    
    uindex_t t_index;
    MCScriptAddDefinitionToModule(s_builder, kTypeMap[t_kind_index] . kind, t_index);
    r_index = t_index;

    Debug_Emit("DefinitionIndex(%s -> %u)", p_type, t_index);
}

void EmitTypeDefinition(intptr_t p_index, PositionRef p_position, NameRef p_name, intptr_t p_type_index)
{
    MCScriptAddTypeToModule(s_builder, to_mcnameref(p_name), (uindex_t)p_type_index, (uindex_t)p_index);

    Debug_Emit("TypeDefinition(%ld, %s, %ld)", p_index,
               cstring_from_nameref(p_name), p_type_index);
}

void EmitConstantDefinition(intptr_t p_index, PositionRef p_position, NameRef p_name, intptr_t p_const_index)
{
    MCScriptAddConstantToModule(s_builder, to_mcnameref(p_name), (uindex_t)p_const_index, (uindex_t)p_index);

    Debug_Emit("ConstantDefinition(%ld, %s, %ld)", p_index,
               cstring_from_nameref(p_name), p_const_index);
}

void EmitVariableDefinition(intptr_t p_index, PositionRef p_position, NameRef p_name, intptr_t p_type_index)
{
    MCScriptAddVariableToModule(s_builder, to_mcnameref(p_name), (uindex_t)p_type_index, (uindex_t)p_index);

    Debug_Emit("VariableDefinition(%ld, %s, %ld)", p_index,
               cstring_from_nameref(p_name), p_type_index);
}

static void EmitForeignHandlerDefinition(intptr_t p_index, PositionRef p_position, NameRef p_name, intptr_t p_type_index, intptr_t p_binding, bool p_force_builtin)
{
    MCAutoStringRef t_binding(to_mcstringref(p_binding));
    MCAutoCustomPointer<struct MCScriptForeignHandlerInfo, MCScriptForeignHandlerInfoRelease> t_info;
    if (!MCScriptForeignHandlerInfoParse(*t_binding, &t_info))
    {
        return;
    }
    
    bool t_force_c_builtin = t_info->language == kMCScriptForeignHandlerLanguageC &&
                            ForceCBindingsAsBuiltins();
    
    MCAutoStringRef t_binding_str;
    if (t_info->language == kMCScriptForeignHandlerLanguageBuiltinC ||
        p_force_builtin ||
        t_force_c_builtin)
    {
        NameRef t_symbol_name;
        if (t_info->language == kMCScriptForeignHandlerLanguageBuiltinC)
        {
            t_symbol_name = p_name;
        }
        else if (p_force_builtin)
        {
            t_symbol_name = nameref_from_cstring((const char *)p_binding);
        }
        else
        {
            MCAutoStringRefAsCString t_symbol_c_string;
            if (!t_symbol_c_string . Lock(t_info->c.function))
            {
                return;
            }
            
            t_symbol_name = nameref_from_cstring(*t_symbol_c_string);
        }
        
        t_binding_str = EmittedBuiltinAdd(t_symbol_name, p_type_index, ForceCBindingsAsBuiltins());
    }
    else
        t_binding_str = to_mcstringref(p_binding);
    
    if (!MCStringBeginsWithCString(*t_binding_str, (const char_t *)"lcb:", kMCStringOptionCompareExact))
        MCScriptAddForeignHandlerToModule(s_builder, to_mcnameref(p_name), (uindex_t)p_type_index, *t_binding_str, (uindex_t)p_index);
    else
    {
        // The string should be of the form:
        //   lcb:<module>.<handler>
        
        MCAutoStringRef t_module_dot_handler;
        MCStringCopySubstring(*t_binding_str, MCRangeMake(4, UINDEX_MAX), &t_module_dot_handler);
        
        MCAutoStringRef t_module, t_handler;
        uindex_t t_offset;
        if (MCStringLastIndexOfChar(*t_module_dot_handler, '.', UINDEX_MAX, kMCStringOptionCompareExact, t_offset))
            MCStringDivideAtIndex(*t_module_dot_handler, t_offset, &t_module, &t_handler);
        else
            t_handler = *t_module_dot_handler;

        intptr_t t_module_dep;
        if (*t_module == nil)
            EmitModuleDependency(s_module_name, t_module_dep);
        else
            EmitModuleDependency(nameref_from_mcstringref(*t_module), t_module_dep);
        
        MCNewAutoNameRef t_hand_name;
        MCNameCreate(*t_handler, &t_hand_name);
        
        MCScriptAddImportToModuleWithIndex(s_builder, (uindex_t)t_module_dep - 1, *t_hand_name, kMCScriptDefinitionKindHandler, (uindex_t)p_type_index, (uindex_t)p_index);
    }

    Debug_Emit("ForeignHandlerDefinition(%ld, %s, %ld, %s)", p_index,
               cstring_from_nameref(p_name), p_type_index,
               (const char *) p_binding);
}

void EmitForeignHandlerDefinition(intptr_t p_index, PositionRef p_position, NameRef p_name, intptr_t p_type_index, intptr_t p_binding)
{
    EmitForeignHandlerDefinition(p_index, p_position, p_name, p_type_index, p_binding, false);
}

void EmitPropertyDefinition(intptr_t p_index, PositionRef p_position, NameRef p_name, intptr_t p_getter, intptr_t p_setter)
{
    MCScriptAddPropertyToModule(s_builder, to_mcnameref(p_name), (uindex_t)p_getter, (uindex_t)p_setter, (uindex_t)p_index);

    Debug_Emit("PropertyDefinition(%ld, %s, %ld, %ld)", p_index,
               cstring_from_nameref(p_name), p_getter, p_setter);
}

void EmitEventDefinition(intptr_t p_index, PositionRef p_position, NameRef p_name, intptr_t p_type_index)
{
    MCScriptAddEventToModule(s_builder, to_mcnameref(p_name), (uindex_t)p_type_index, (uindex_t)p_index);

    Debug_Emit("EmitEvent(%ld, %s, %ld)", p_index, cstring_from_nameref(p_name),
               p_type_index);
}

void EmitBeginHandlerDefinition(intptr_t p_index, PositionRef p_position, NameRef p_name, intptr_t p_type_index)
{
    MCScriptBeginHandlerInModule(s_builder, to_mcnameref(p_name), (uindex_t)p_type_index, kMCScriptHandlerAttributeSafe, (uindex_t)p_index);

    Debug_Emit("BeginHandlerDefinition(%ld, %s, %ld)", p_index,
               cstring_from_nameref(p_name), p_type_index);
}

void EmitBeginUnsafeHandlerDefinition(intptr_t p_index, PositionRef p_position, NameRef p_name, intptr_t p_type_index)
{
    MCScriptBeginHandlerInModule(s_builder, to_mcnameref(p_name), (uindex_t)p_type_index, kMCScriptHandlerAttributeUnsafe, (uindex_t)p_index);
    
    Debug_Emit("BeginUnsafeHandlerDefinition(%ld, %s, %ld)", p_index,
               cstring_from_nameref(p_name), p_type_index);
}

void EmitEndHandlerDefinition(void)
{
    /* Check that registers have all been destroyed */
    EmitCheckNoRegisters();
    
    if (s_attached_regs != nil)
    {
        for(AttachedReg *t_reg = s_attached_regs; t_reg != NULL; t_reg = t_reg -> next)
            Debug_Emit("Dangling register %d attached to %p", t_reg -> reg, t_reg -> expr);
        Fatal_InternalInconsistency("Handler code generated with dangling register attachments");
    }
    
    MCScriptEndHandlerInModule(s_builder);

    Debug_Emit("EndHandlerDefinition()");
}

void EmitBeginSyntaxDefinition(intptr_t p_index, PositionRef p_position, NameRef p_name)
{
    MCScriptBeginSyntaxInModule(s_builder, to_mcnameref(p_name), (uindex_t)p_index);

    Debug_Emit("BeginSyntaxDefinition(%ld, %s)", p_index,
               cstring_from_nameref(p_name));
}

void EmitEndSyntaxDefinition(void)
{
    MCScriptEndSyntaxInModule(s_builder);

    Debug_Emit("EndSyntaxDefinition()");
}

void EmitBeginSyntaxMethod(intptr_t p_handler_index)
{
    MCScriptBeginSyntaxMethodInModule(s_builder, (uindex_t)p_handler_index);

    Debug_Emit("BeginSyntaxMethod(%ld)", p_handler_index);
}

void EmitEndSyntaxMethod(void)
{
    MCScriptEndSyntaxMethodInModule(s_builder);

    Debug_Emit("EmitEndSyntaxMethod()");
}

void EmitInputSyntaxMethodArgument(void)
{
    MCScriptAddBuiltinArgumentToSyntaxMethodInModule(s_builder, 0);

    Debug_Emit("InputSyntaxMethodArgument()");
}

void EmitOutputSyntaxMethodArgument(void)
{
    MCScriptAddBuiltinArgumentToSyntaxMethodInModule(s_builder, 1);

    Debug_Emit("OutputSyntaxMethodArgument()");
}

void EmitContextSyntaxMethodArgument(void)
{
    // TODO: Sort out context
    // MCScriptAddBuiltinArgumentToSyntaxMethodInModule(s_builder, 2);

    Debug_Emit("ContextSyntaxMethodArgument()");
}

void EmitIteratorSyntaxMethodArgument(void)
{
    // TODO: Sort out iterate
    MCScriptAddBuiltinArgumentToSyntaxMethodInModule(s_builder, 2);

    Debug_Emit("IteratorSyntaxMethodArgument()");
}

void EmitContainerSyntaxMethodArgument(void)
{
    // TODO: Sort out iterate
    MCScriptAddBuiltinArgumentToSyntaxMethodInModule(s_builder, 3);

    Debug_Emit("ContainerSyntaxMethodArgument()");
}

void EmitUndefinedSyntaxMethodArgument(void)
{
    MCScriptAddConstantArgumentToSyntaxMethodInModule(s_builder, kMCNull);

    Debug_Emit("UndefinedSyntaxMethodArgument()");
}

void EmitTrueSyntaxMethodArgument(void)
{
    MCScriptAddConstantArgumentToSyntaxMethodInModule(s_builder, kMCTrue);

    Debug_Emit("TrueSyntaxMethodArgument()");
}

void EmitFalseSyntaxMethodArgument(void)
{
    MCScriptAddConstantArgumentToSyntaxMethodInModule(s_builder, kMCFalse);

    Debug_Emit("FalseSyntaxMethodArgument()");
}

void EmitIntegerSyntaxMethodArgument(intptr_t p_int)
{
    MCAutoNumberRef t_number;
    MCNumberCreateWithInteger((uindex_t)p_int, &t_number);
    MCScriptAddConstantArgumentToSyntaxMethodInModule(s_builder, *t_number);

    Debug_Emit("IntegerSyntaxMethodArgument(%ld)", p_int);
}

void EmitRealSyntaxMethodArgument(intptr_t p_double)
{
    MCAutoNumberRef t_number;
    MCNumberCreateWithInteger((uindex_t)p_double, &t_number);
    MCScriptAddConstantArgumentToSyntaxMethodInModule(s_builder, *t_number);

    Debug_Emit("RealSyntaxMethodArgument(%lf)", *(double *)p_double);
}

void EmitStringSyntaxMethodArgument(intptr_t p_string)
{
	MCAutoStringRef t_string(to_mcstringref(p_string));
    MCScriptAddConstantArgumentToSyntaxMethodInModule(s_builder, *t_string);

    Debug_Emit("RealSyntaxMethodArgument(\"%s\")", (const char *)p_string);
}

void EmitVariableSyntaxMethodArgument(intptr_t p_index)
{
    MCScriptAddVariableArgumentToSyntaxMethodInModule(s_builder, (uindex_t)p_index);

    Debug_Emit("VariableSyntaxMethodArgument(%ld)", p_index);
}

void EmitIndexedVariableSyntaxMethodArgument(intptr_t p_var_index, intptr_t p_element_index)
{
    MCScriptAddIndexedVariableArgumentToSyntaxMethodInModule(s_builder, (uindex_t)p_var_index, (uindex_t)p_element_index);

    Debug_Emit("IndexedVariableSyntaxMethodArgument(%ld, %ld)", p_var_index,
               p_element_index);
}

void EmitBeginDefinitionGroup(void)
{
    MCScriptBeginDefinitionGroupInModule(s_builder);

    Debug_Emit("BeginDefinitionGroup()");
}

void EmitContinueDefinitionGroup(intptr_t p_index)
{
    MCScriptAddHandlerToDefinitionGroupInModule(s_builder, (uindex_t)p_index);

    Debug_Emit("ContinueDefinitionGroup(%ld)", p_index);
}

void EmitEndDefinitionGroup(intptr_t *r_index)
{
    uindex_t t_index;
    MCScriptEndDefinitionGroupInModule(s_builder, t_index);
    *r_index = t_index;

    Debug_Emit("EndDefinitionGroup(-> %ld)", *r_index);
}

/////////

static bool define_builtin_typeinfo(const char *name, intptr_t& r_new_index)
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

void EmitDefinedType(intptr_t index, intptr_t& r_type_index)
{
    uindex_t t_type_index;
    MCScriptAddDefinedTypeToModule(s_builder, (uindex_t)index, t_type_index);
    r_type_index = t_type_index;

    Debug_Emit("DefinedType(%ld -> %ld)", index, r_type_index);
}

void EmitForeignType(intptr_t p_binding, intptr_t& r_type_index)
{
    uindex_t t_type_index;
    MCStringRef t_binding_str = 
            EmittedBuiltinAdd(nameref_from_mcstringref(to_mcstringref(p_binding)), UINDEX_MAX, false);
    
    MCScriptAddForeignTypeToModule(s_builder, t_binding_str, t_type_index);
    r_type_index = t_type_index;


    Debug_Emit("ForeignType(%s -> %ld)", (const char *)p_binding, r_type_index);
}

void EmitOptionalType(intptr_t base_index, intptr_t& r_new_index)
{
    uindex_t t_index;
    MCScriptAddOptionalTypeToModule(s_builder, (uindex_t)base_index, t_index);
    r_new_index = t_index;

    Debug_Emit("OptionalType(%ld -> %ld)", base_index, r_new_index);
}

void EmitAnyType(intptr_t& r_new_index)
{
    if (!define_builtin_typeinfo("any", r_new_index))
        return;

    Debug_Emit("AnyType(-> %ld)", r_new_index);
}

void EmitBooleanType(intptr_t& r_new_index)
{
    if (!define_builtin_typeinfo("Boolean", r_new_index))
        return;

    Debug_Emit("BooleanType(-> %ld)", r_new_index);
}

void EmitIntegerType(intptr_t& r_new_index)
{
    if (!define_builtin_typeinfo("Number", r_new_index))
        return;

    Debug_Emit("IntegerType(-> %ld)", r_new_index);
}

void EmitRealType(intptr_t& r_new_index)
{
    // TODO: Real / Integer types.
    if (!define_builtin_typeinfo("Number", r_new_index))
        return;

    Debug_Emit("RealType(-> %ld)", r_new_index);
}

void EmitNumberType(intptr_t& r_new_index)
{
    if (!define_builtin_typeinfo("Number", r_new_index))
        return;

    Debug_Emit("NumberType(-> %ld)", r_new_index);
}

void EmitStringType(intptr_t& r_new_index)
{
    if (!define_builtin_typeinfo("String", r_new_index))
        return;

    Debug_Emit("StringType(-> %ld)", r_new_index);
}

void EmitDataType(intptr_t& r_new_index)
{
    if (!define_builtin_typeinfo("Data", r_new_index))
        return;

    Debug_Emit("DataType(-> %ld)", r_new_index);
}

void EmitArrayType(intptr_t& r_new_index)
{
    if (!define_builtin_typeinfo("Array", r_new_index))
        return;

    Debug_Emit("ArrayType(-> %ld)", r_new_index);
}

void EmitListType(intptr_t& r_new_index)
{
    if (!define_builtin_typeinfo("List", r_new_index))
        return;

    Debug_Emit("ListType(-> %ld)", r_new_index);
}

void EmitUndefinedType(intptr_t& r_new_index)
{
    if (!define_builtin_typeinfo("undefined", r_new_index))
        return;

    Debug_Emit("UndefinedType(-> %ld)", r_new_index);
}

//////////

void EmitBeginRecordType()
{
    MCScriptBeginRecordTypeInModule(s_builder);

    Debug_Emit("BeginRecordType()");
}

void EmitRecordTypeField(NameRef name, intptr_t type_index)
{
    MCScriptContinueRecordTypeInModule(s_builder, to_mcnameref(name), (uindex_t)type_index);
    Debug_Emit("RecordTypeField(%s, %ld)", cstring_from_nameref(name),
               type_index);
}

void EmitEndRecordType(intptr_t& r_type_index)
{
    uindex_t t_index;
    MCScriptEndRecordTypeInModule(s_builder, t_index);
    r_type_index = t_index;

    Debug_Emit("EndRecordType(-> %ld)", r_type_index);
}

//////////

void EmitBeginHandlerType(intptr_t return_type_index)
{
    MCScriptBeginHandlerTypeInModule(s_builder, (uindex_t)return_type_index);

    Debug_Emit("BeginHandlerType(%ld)", return_type_index);
}

void EmitBeginForeignHandlerType(intptr_t return_type_index)
{
    MCScriptBeginForeignHandlerTypeInModule(s_builder, (uindex_t)return_type_index);
    
    Debug_Emit("BeginForeignHandlerType(%ld)", return_type_index);
}

static void EmitHandlerTypeParameter(MCHandlerTypeFieldMode mode, NameRef name, intptr_t type_index)
{
    MCScriptContinueHandlerTypeInModule(s_builder, (MCScriptHandlerTypeParameterMode)mode, to_mcnameref(name), (uindex_t)type_index);

    Debug_Emit("HandlerTypeParameter(%d, %s, %ld)", mode,
               cstring_from_nameref(name), type_index);
}

void EmitHandlerTypeInParameter(NameRef name, intptr_t type_index)
{
    EmitHandlerTypeParameter(kMCHandlerTypeFieldModeIn, name, type_index);
}

void EmitHandlerTypeOutParameter(NameRef name, intptr_t type_index)
{
    EmitHandlerTypeParameter(kMCHandlerTypeFieldModeOut, name, type_index);
}

void EmitHandlerTypeInOutParameter(NameRef name, intptr_t type_index)
{
    EmitHandlerTypeParameter(kMCHandlerTypeFieldModeInOut, name, type_index);
}

void EmitHandlerTypeVariadicParameter(NameRef name)
{
    intptr_t type_index;
    EmitListType(type_index);
    EmitHandlerTypeParameter(kMCHandlerTypeFieldModeVariadic, name, type_index);
}

void EmitEndHandlerType(intptr_t& r_type_index)
{
    uindex_t t_index;
    MCScriptEndHandlerTypeInModule(s_builder, t_index);
    r_type_index = t_index;

    Debug_Emit("EndHandlerType(-> %ld)", t_index);
}

///////////

void EmitHandlerParameter(NameRef name, intptr_t type_index, intptr_t& r_index)
{
    uindex_t t_index;
    MCScriptAddParameterToHandlerInModule(s_builder, to_mcnameref(name), (uindex_t)type_index, t_index);
    r_index = t_index;

    Debug_Emit("HandlerParameter(%s, %ld -> %ld)", cstring_from_nameref(name),
               type_index, r_index);
}

void EmitHandlerVariable(NameRef name, intptr_t type_index, intptr_t& r_index)
{
    uindex_t t_index;
    MCScriptAddVariableToHandlerInModule(s_builder, to_mcnameref(name), (uindex_t)type_index, t_index);
    r_index = t_index;

    Debug_Emit("HandlerVariable(%s, %ld -> %ld)", cstring_from_nameref(name),
               type_index, r_index);
}

void EmitDeferLabel(intptr_t& r_label)
{
    uindex_t t_index;
    MCScriptDeferLabelForBytecodeInModule(s_builder, t_index);
    r_label = t_index;

    Debug_Emit("DeferLabel(-> %ld)", r_label);
}

void EmitResolveLabel(intptr_t label)
{
    MCScriptResolveLabelForBytecodeInModule(s_builder, (uindex_t)label);

    Debug_Emit("ResolveLabel(%ld)", label);
}

//////////

static uint8_t *s_registers = nil;
static uindex_t s_register_count = 0;

void EmitCreateRegister(intptr_t& r_regindex)
{
    intptr_t t_reg;
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

    Debug_Emit("CreateRegister(-> %ld)", r_regindex);
}

void EmitDestroyRegister(intptr_t regindex)
{
    s_registers[regindex] = 0;

    Debug_Emit("DestroyRegister(%ld)", regindex);
}

void EmitCheckNoRegisters(void)
{
    bool t_all_destroyed = true;
    if (s_register_count > 0)
    {
        for(uindex_t i = 0; i < s_register_count; i++)
            if (s_registers[i] != 0)
            {
                t_all_destroyed = false;
                Debug_Emit("Register %d not destroyed", i);
            }
    }
    
    if (!t_all_destroyed)
    {
        Fatal_InternalInconsistency("handler generation finished without destroying all registers");
    }
}

//////////

struct RepeatLabels
{
    RepeatLabels *next;
    intptr_t head;
    intptr_t tail;
};

static RepeatLabels *s_repeat_labels = nil;

void EmitPushRepeatLabels(intptr_t next, intptr_t exit)
{
    RepeatLabels *t_labels = nullptr;
    /* UNCHECKED */ MCMemoryNew(t_labels);
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

void EmitCurrentRepeatLabels(intptr_t& r_next, intptr_t& r_exit)
{
    r_next = s_repeat_labels -> head;
    r_exit = s_repeat_labels -> tail;
}

//////////

void EmitUndefinedConstant(intptr_t *idx)
{
    uindex_t t_index;
    MCScriptAddValueToModule(s_builder, kMCNull, t_index);
    *idx = t_index;
    
    Debug_Emit("UndefinedConstant(-> %ld)", *idx);
}

void EmitTrueConstant(intptr_t *idx)
{
    uindex_t t_index;
    MCScriptAddValueToModule(s_builder, kMCTrue, t_index);
    *idx = t_index;
    
    Debug_Emit("TrueConstant(-> %ld)", *idx);
}

void EmitFalseConstant(intptr_t *idx)
{
    uindex_t t_index;
    MCScriptAddValueToModule(s_builder, kMCFalse, t_index);
    *idx = t_index;
    
    Debug_Emit("FalseConstant(%ld)", *idx);
}

void EmitIntegerConstant(intptr_t value, intptr_t *idx)
{
    MCAutoNumberRef t_number;
    uindex_t t_index;
    MCNumberCreateWithInteger((integer_t)value, &t_number);
    MCScriptAddValueToModule(s_builder, *t_number, t_index);
    *idx = t_index;
    
    Debug_Emit("IntegerConstant(%ld -> %ld)", value, *idx);
}

void EmitUnsignedIntegerConstant(uintptr_t value, intptr_t *idx)
{
    MCAutoNumberRef t_number;
    uindex_t t_index;
    MCNumberCreateWithUnsignedInteger((uinteger_t)value, &t_number);
    MCScriptAddValueToModule(s_builder, *t_number, t_index);
    *idx = t_index;
    
    Debug_Emit("UnsignedIntegerConstant(%lu -> %ld)", value, *idx);
}

void EmitRealConstant(intptr_t value, intptr_t *idx)
{
    MCAutoNumberRef t_number;
    uindex_t t_index;
    MCNumberCreateWithReal(*(double *)value, &t_number);
    MCScriptAddValueToModule(s_builder, *t_number, t_index);
    *idx = t_index;
    
    Debug_Emit("RealConstant(%lf -> %ld)", *(double *)value, *idx);
}

void EmitStringConstant(intptr_t value, intptr_t *idx)
{
    MCAutoStringRef t_string;
    uindex_t t_index;
    MCStringCreateWithBytes((const byte_t *)value, (uindex_t)strlen((const char *)value), kMCStringEncodingUTF8, false, &t_string);
    MCScriptAddValueToModule(s_builder, *t_string, t_index);
    *idx = t_index;
    
    Debug_Emit("StringConstant(\"%s\" -> %ld)", (const char *)value, *idx);
}

void EmitBeginListConstant(void)
{
    MCScriptBeginListValueInModule(s_builder);
    
    Debug_Emit("BeginListConstant()", 0);
}

void EmitContinueListConstant(intptr_t idx)
{
    MCScriptContinueListValueInModule(s_builder, (uindex_t)idx);
    
    Debug_Emit("ContinueListConstant(%ld)", idx);
}

void EmitEndListConstant(intptr_t *idx)
{
    MCScriptEndListValueInModule(s_builder, (uindex_t&)*idx);
    
    Debug_Emit("EndListConstant(-> %ld)", *idx);
}

void EmitBeginArrayConstant(void)
{
    MCScriptBeginArrayValueInModule(s_builder);
    
    Debug_Emit("BeginArrayConstant()", 0);
}

void EmitContinueArrayConstant(intptr_t key_idx, intptr_t value_idx)
{
    MCScriptContinueArrayValueInModule(s_builder, (uindex_t)key_idx, (uindex_t)value_idx);
    
    Debug_Emit("ContinueArrayConstant(%ld, %ld)", key_idx, value_idx);
}

void EmitEndArrayConstant(intptr_t *idx)
{
    uindex_t t_index;
    MCScriptEndArrayValueInModule(s_builder, t_index);
    *idx = t_index;
    
    Debug_Emit("EndArrayConstant(-> %ld)", *idx);
}

//////////

class __opcode_index
{
public:
    __opcode_index(const char *p_name) : m_index(0)
    {
        if (!MCScriptLookupBytecode(p_name, m_index))
        {
            Fatal_InternalInconsistency(p_name /*"unknown bytecode op"*/);
            return;
        }
    }
    
    operator uindex_t (void) const
    {
        return m_index;
    }
    
private:
    uindex_t m_index;
};

static uindex_t s_opcode;
static uindex_t *s_arguments = nil;
static uindex_t s_argument_count = 0;

static void push_argument(intptr_t arg)
{
    s_arguments = (uindex_t *)Reallocate(s_arguments, sizeof(uindex_t) * (s_argument_count + 1));
    s_arguments[s_argument_count] = (uindex_t)arg;
    s_argument_count += 1;
}

static void log_instruction(void)
{
    MCAutoStringRef t_string;
    MCStringCreateMutable(0, &t_string);
    MCStringAppendFormat(*t_string, "%s(", MCScriptDescribeBytecode(s_opcode));
    for(uindex_t i = 0; i < s_argument_count; i++)
    {
        if (i != 0)
            MCStringAppendFormat(*t_string, ", ");
        MCStringAppendFormat(*t_string, "%ld", s_arguments[i]);
    }
    MCStringAppendFormat(*t_string, ")");
    
    MCAutoStringRefAsCString t_cstring;
    t_cstring.Lock(*t_string);
    Debug_Emit("%s", *t_cstring);
}

void EmitBeginOpcode(intptr_t name)
{
    __opcode_index t_opcode((const char *)name);
    s_opcode = t_opcode;
}

void EmitContinueOpcode(intptr_t param)
{
    push_argument(param);
}

void EmitEndOpcode(void)
{
    MCScriptEmitBytecodeInModuleA(s_builder, s_opcode, s_arguments, s_argument_count);
    
    log_instruction();
    
    s_argument_count = 0;
}

void EmitJump(intptr_t label)
{
    static const __opcode_index kJumpOpcodeIndex("jump");
    MCScriptEmitBytecodeInModule(s_builder, kJumpOpcodeIndex, label, UINDEX_MAX);
    
    Debug_Emit("Jump(%ld)", label);
}

void EmitJumpIfTrue(intptr_t reg, intptr_t label)
{
    static const __opcode_index kJumpIfTrueOpcodeIndex("jump_if_true");
    MCScriptEmitBytecodeInModule(s_builder, kJumpIfTrueOpcodeIndex, reg, label, UINDEX_MAX);
    
    Debug_Emit("JumpIfTrue(%ld, %ld)", label);
}

void EmitJumpIfFalse(intptr_t reg, intptr_t label)
{
    static const __opcode_index kJumpIfFalseOpcodeIndex("jump_if_false");
    MCScriptEmitBytecodeInModule(s_builder, kJumpIfFalseOpcodeIndex, reg, label, UINDEX_MAX);
    
    Debug_Emit("JumpIfFalse(%ld, %ld)", reg, label);
}

void EmitBeginInvoke(intptr_t index, intptr_t contextreg, intptr_t resultreg)
{
    static const __opcode_index kInvokeOpcodeIndex("invoke");
    
    s_opcode = kInvokeOpcodeIndex;
    push_argument(index);
    push_argument(resultreg);
}

void EmitBeginIndirectInvoke(intptr_t handlerreg, intptr_t contextreg, intptr_t resultreg)
{
    static const __opcode_index kInvokeIndirectOpcodeIndex("invoke_indirect");
    
    s_opcode = kInvokeIndirectOpcodeIndex;
    push_argument(handlerreg);
    push_argument(resultreg);
}

void EmitContinueInvoke(intptr_t reg)
{
    push_argument(reg);
}

void EmitEndInvoke(void)
{
    MCScriptEmitBytecodeInModuleA(s_builder, s_opcode, s_arguments, s_argument_count);

    log_instruction();
    
    s_argument_count = 0;
}

//////////

void EmitBeginAssignList(intptr_t reg)
{
    static const __opcode_index kAssignListOpcodeIndex("assign_list");
    
    s_opcode = kAssignListOpcodeIndex;
    push_argument(reg);
}

void EmitContinueAssignList(intptr_t reg)
{
    push_argument(reg);
}

void EmitEndAssignList(void)
{
    MCScriptEmitBytecodeInModuleA(s_builder, s_opcode, s_arguments, s_argument_count);
    
    log_instruction();
    
    s_argument_count = 0;
}

void EmitBeginAssignArray(intptr_t reg)
{
    static const __opcode_index kAssignArrayOpcodeIndex("assign_array");
    
    s_opcode = kAssignArrayOpcodeIndex;
    push_argument(reg);
}

void EmitContinueAssignArray(intptr_t reg)
{
    push_argument(reg);
}

void EmitEndAssignArray(void)
{
    MCScriptEmitBytecodeInModuleA(s_builder, s_opcode, s_arguments, s_argument_count);
    
    log_instruction();
    
    s_argument_count = 0;
}

void EmitAssign(intptr_t dst, intptr_t src)
{
    static const __opcode_index kAssignOpcodeIndex("assign");
    MCScriptEmitBytecodeInModule(s_builder, kAssignOpcodeIndex, dst, src, UINDEX_MAX);
    
    Debug_Emit("Assign(%ld, %ld)", dst, src);
}

void EmitAssignConstant(intptr_t dst, intptr_t idx)
{
    static const __opcode_index kAssignConstantOpcodeIndex("assign_constant");
    MCScriptEmitBytecodeInModule(s_builder, kAssignConstantOpcodeIndex, dst, idx, UINDEX_MAX);

    Debug_Emit("AssignConstant(%ld, %ld)", dst, idx);
}

/////////

void EmitFetch(intptr_t reg, intptr_t var, intptr_t level)
{
    static const __opcode_index kFetchOpcodeIndex("fetch");
    MCScriptEmitBytecodeInModule(s_builder, kFetchOpcodeIndex, reg, var, UINDEX_MAX);

    Debug_Emit("Fetch(%ld, %ld)", reg, var);
}

void EmitStore(intptr_t reg, intptr_t var, intptr_t level)
{
    static const __opcode_index kStoreOpcodeIndex("store");
    MCScriptEmitBytecodeInModule(s_builder, kStoreOpcodeIndex, reg, var, UINDEX_MAX);

    Debug_Emit("Store(%ld, %ld)", reg, var);
}

void EmitReturn(intptr_t reg)
{
    static const __opcode_index kReturnOpcodeIndex("return");
    MCScriptEmitBytecodeInModule(s_builder, kReturnOpcodeIndex, reg, UINDEX_MAX);

    Debug_Emit("Return(%ld)", reg);
}

void EmitReturnNothing(void)
{
    static const __opcode_index kReturnOpcodeIndex("return");
    MCScriptEmitBytecodeInModule(s_builder, kReturnOpcodeIndex, UINDEX_MAX);

    Debug_Emit("ReturnUndefined()", 0);
}

void EmitReset(intptr_t reg)
{
    static const __opcode_index kResetOpcodeIndex("reset");
    MCScriptEmitBytecodeInModule(s_builder, kResetOpcodeIndex, reg, UINDEX_MAX);
    
    Debug_Emit("Reset(%ld)", reg);
}

////////

static bool FindAttachedReg(intptr_t expr, AttachedReg*& r_attach)
{
    for(AttachedReg *t_reg = s_attached_regs; t_reg != nil; t_reg = t_reg -> next)
        if (t_reg-> expr == expr)
        {
            r_attach = t_reg;
            return true;
        }
    
    return false;
}

void EmitAttachRegisterToExpression(intptr_t reg, intptr_t expr)
{
    AttachedReg *t_attach = nullptr;
    if (FindAttachedReg(expr, t_attach))
        Fatal_InternalInconsistency("Register attached to expression which is already attached");
    
    /* UNCHECKED */ MCMemoryNew(t_attach);
    t_attach -> next = s_attached_regs;
    t_attach -> expr = expr;
    t_attach -> reg = reg;
    s_attached_regs = t_attach;
    
    Debug_Emit("AttachRegister(%d, %p)", reg, expr);
}

void EmitDetachRegisterFromExpression(intptr_t expr)
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
    
    if (t_remove != nil)
        Debug_Emit("DetachRegister(%d, %p)", t_remove -> reg, t_remove -> expr);
    
    MCMemoryDelete(t_remove);
}

int EmitIsRegisterAttachedToExpression(intptr_t expr)
{
    AttachedReg *t_attach;
    return FindAttachedReg(expr, t_attach);
}

int EmitGetRegisterAttachedToExpression(intptr_t expr, intptr_t *reg)
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
    intptr_t t_line;
    GetRowOfPosition(p_position, &t_line);
    MCScriptEmitPositionForBytecodeInModule(s_builder, to_mcnameref(t_filename_name), (uindex_t)t_line);

    Debug_Emit("Position('%s', %ld)", t_filename, t_line);
}

//////////

void OutputBeginManifest(void)
{
	OutputFileBegin(OpenManifestOutputFile());
}

//////////

struct DependMapping
{
    NameRef module;
    const char *source;
    char *interface;
    time_t source_time;
    time_t interface_time;
    bool is_interface;
    bool processed;
    bool changed;
};

struct DependDependency
{
    NameRef module;
    NameRef dependency;
};

extern "C"
{
    DependencyModeType DependencyMode = kDependencyModeNone;
}

static DependMapping *s_depend_mappings;
static int s_depend_mapping_count;

static DependDependency *s_depend_deps;
static int s_depend_dep_count;

static bool DependFindMapping(NameRef p_module, DependMapping **r_mapping)
{
    for(int i = 0; i < s_depend_mapping_count; i++)
        if (p_module == s_depend_mappings[i] . module)
        {
            if (r_mapping != NULL)
                *r_mapping = &s_depend_mappings[i];
            return true;
        }
    
    return false;
}

static bool DependProcess(NameRef p_module)
{
    DependMapping *t_mapping;
    if (!DependFindMapping(p_module, &t_mapping))
        return false;
    
    const char *t_module_name;
    GetStringOfNameLiteral(p_module, &t_module_name);
    
    if (t_mapping -> processed)
        return t_mapping -> changed;
    
    // Mark this module as being processed.
    t_mapping -> processed = true;
    
    // If this module is an interface, then we can't do anything with the info
    // we've got so skip.
    if (t_mapping -> is_interface)
        return false;
    
    // If our source is more recent than our interface then we must recompile.
    bool t_changed;
    t_changed = t_mapping -> source_time > t_mapping -> interface_time;
    if (t_changed)
        Debug_Depend("Recompiling '%s' as source newer than interface", t_module_name);
    
    // If any dependent modules need recompiling, we must recompile after.
    for(int i = 0; i < s_depend_dep_count; i++)
    {
        if (s_depend_deps[i] . module == p_module)
        {
            // If a dependency has changed, then we must be recompiled.
            if (DependProcess(s_depend_deps[i] . dependency))
            {
                const char *t_depend_name;
                GetStringOfNameLiteral(s_depend_deps[i] . dependency, &t_depend_name);
                Debug_Depend("Recompiling '%s' as dependency '%s' changed", t_module_name, t_depend_name);
                t_changed = true;
            }
        }
    }
    
    // If any dependent module interfaces are more recent than our interface then
    // we must recompile. However, if we already know this module needs recompiled,
    // then we don't need to check.
    if (!t_changed)
    {
        for(int i = 0; i < s_depend_dep_count; i++)
        {
            if (s_depend_deps[i] . module == p_module)
            {
                DependMapping *t_depend_mapping;
                if (DependFindMapping(s_depend_deps[i] . dependency, &t_depend_mapping))
                    if (t_depend_mapping -> interface_time > t_mapping -> interface_time)
                    {
                        const char *t_depend_name;
                        GetStringOfNameLiteral(s_depend_deps[i] . dependency, &t_depend_name);
                        Debug_Depend("Recompiling '%s' as dependency '%s' interface newer", t_module_name, t_depend_name);
                        t_changed = true;
                    }
            }
        }
    }
    
    if (DependencyMode == kDependencyModeOrder)
        t_changed = true;
    
    // If we have changed, then emit the source file.
    if (t_changed)
        fprintf(stdout, "%s\n", t_mapping -> source);
    
    t_mapping -> changed = t_changed;
    
    return t_changed;
}

void DependStart(void)
{
    s_depend_mappings = NULL;
    s_depend_mapping_count = 0;
    s_depend_deps = NULL;
    s_depend_dep_count = 0;
}

void DependFinish(void)
{
    if (DependencyMode == kDependencyModeOrder ||
        DependencyMode == kDependencyModeChangedOrder)
    {
        for(int i = 0; i < s_depend_mapping_count; i++)
            DependProcess(s_depend_mappings[i] . module);
    }
    else if (DependencyMode == kDependencyModeMake)
    {
        const char *t_output_file;
        GetOutputFile(&t_output_file);
        if (t_output_file == NULL)
        {
            for(int i = 0; i < s_depend_mapping_count; i++)
            {
                DependMapping *t_module;
                t_module = &s_depend_mappings[i];
                
                fprintf(stdout, "%s:", t_module -> interface);
                for(int j = 0; j < s_depend_dep_count; j++)
                {
                    if (s_depend_deps[j] . module != t_module -> module)
                        continue;
                    
                    DependMapping *t_dependency;
                    if (!DependFindMapping(s_depend_deps[j] . dependency, &t_dependency))
                        continue;
                    
                    fprintf(stdout, " %s", t_dependency -> interface);
                }
                
                if (!t_module -> is_interface)
                    fprintf(stdout, " %s", t_module -> source);
                
                fprintf(stdout, "\n");
            }
        }
        else
        {
            // If we have an output code file we just process the first module
            // and generate an appropriate make-style depedency list.
            
            DependMapping *t_module;
            t_module = &s_depend_mappings[0];
            
            fprintf(stdout, "%s: %s\n", t_module -> interface, t_output_file);
            for(int j = 0; j < s_depend_dep_count; j++)
            {
                if (s_depend_deps[j] . module != t_module -> module)
                    continue;
                
                DependMapping *t_dependency;
                if (!DependFindMapping(s_depend_deps[j] . dependency, &t_dependency))
                    continue;
                
                fprintf(stdout, "%s: %s\n", t_output_file, t_dependency -> interface);
            }
        }
    }
}

static time_t time_of_file(const char *p_filename)
{
    struct stat t_stat;
    if (stat(p_filename, &t_stat) == -1)
        return 0;
    return t_stat . st_mtime;
}

void DependDefineMapping(NameRef p_module_name, const char *p_source_file)
{
    // Don't add a mapping if already there.
    if (DependFindMapping(p_module_name, NULL))
        return;
    
    s_depend_mapping_count += 1;
    s_depend_mappings = (DependMapping *)Reallocate(s_depend_mappings, s_depend_mapping_count * sizeof(DependMapping));
    
    const char *t_module_name_string;
    GetStringOfNameLiteral(p_module_name, &t_module_name_string);
    
    char *t_module_interface;
    FindImportedModuleFile(t_module_name_string, &t_module_interface);
    
    time_t t_source_time, t_interface_time;
    t_source_time = time_of_file(p_source_file);
    t_interface_time = time_of_file(t_module_interface);
    
    s_depend_mappings[s_depend_mapping_count - 1] . module = p_module_name;
    s_depend_mappings[s_depend_mapping_count - 1] . source = p_source_file;
    s_depend_mappings[s_depend_mapping_count - 1] . interface = t_module_interface;
    s_depend_mappings[s_depend_mapping_count - 1] . source_time = t_source_time;
    s_depend_mappings[s_depend_mapping_count - 1] . interface_time = t_interface_time;
    s_depend_mappings[s_depend_mapping_count - 1] . is_interface = strcmp(t_module_interface, p_source_file) == 0;
    s_depend_mappings[s_depend_mapping_count - 1] . processed = false;
    s_depend_mappings[s_depend_mapping_count - 1] . changed = false;
}

void DependDefineDependency(NameRef p_module_name, NameRef p_dependency_name)
{
    s_depend_dep_count += 1;
    s_depend_deps = (DependDependency *)Reallocate(s_depend_deps, s_depend_dep_count * sizeof(DependDependency));
    
    s_depend_deps[s_depend_dep_count - 1] . module = p_module_name;
    s_depend_deps[s_depend_dep_count - 1] . dependency = p_dependency_name;
}

//////////

static MCProperListRef s_bytecode_names = nil;

int BytecodeEnumerate(intptr_t index, intptr_t *r_name)
{
    if (s_bytecode_names == nil)
        MCScriptCopyBytecodeNames(s_bytecode_names);
    
    if (index >= MCProperListGetLength(s_bytecode_names))
    {
        MCValueRelease(s_bytecode_names);
        s_bytecode_names = nil;
        return 0;
    }
    
    MCStringRef t_name;
    t_name = (MCStringRef)MCProperListFetchElementAtIndex(s_bytecode_names, (uindex_t)index);
    *r_name = (intptr_t)nameref_from_mcstringref(t_name);
    
    return 1;
}

int BytecodeLookup(intptr_t name, intptr_t *r_opcode)
{
    uindex_t t_opcode;
    if (!MCScriptLookupBytecode((const char *)name, t_opcode))
        return 0;
    
    *r_opcode = t_opcode;
    return 1;
}

void BytecodeDescribe(intptr_t opcode, intptr_t *r_name)
{
    *r_name = (intptr_t)MCScriptDescribeBytecode((uindex_t)opcode);
}

int BytecodeIsValidArgumentCount(intptr_t opcode, intptr_t count)
{
    return MCScriptCheckBytecodeParameterCount((uindex_t)opcode, (uindex_t)count) ? 1 : 0;
}

void BytecodeDescribeParameter(intptr_t opcode, intptr_t index, intptr_t *r_type)
{
    *r_type = (intptr_t)MCScriptDescribeBytecodeParameter((uindex_t)opcode, (uindex_t)index);
}

//////////

static void BuildBuiltinModule_ForeignType(const char *p_name, const char *p_foreign_name, intptr_t* r_type_index = nullptr)
{
    intptr_t t_def_index = -1;
    EmitDefinitionIndex("type", t_def_index);
    intptr_t t_type_index = -1;
    EmitForeignType((intptr_t)p_foreign_name, t_type_index);
    EmitTypeDefinition(t_def_index, 0, nameref_from_cstring(p_name), t_type_index);
    EmitExportedDefinition(t_def_index);
    if (r_type_index != nullptr)
        EmitDefinedType(t_def_index, *r_type_index);
}

static void BuildBuiltinModule_ForeignHandler1(intptr_t p_return_type, MCHandlerTypeFieldMode p_arg_mode_0, const char *p_arg_name_0, intptr_t p_arg_type_0, const char *p_name, const char *p_foreign_name)
{
    intptr_t t_handler_type_index = -1;
    EmitBeginForeignHandlerType(p_return_type);
    EmitHandlerTypeParameter(p_arg_mode_0, nameref_from_cstring(p_arg_name_0), p_arg_type_0);
    EmitEndHandlerType(t_handler_type_index);
    
    intptr_t t_handler_def = -1;
    EmitDefinitionIndex("foreignhandler", t_handler_def);
    EmitForeignHandlerDefinition(t_handler_def, 0, nameref_from_cstring(p_name), t_handler_type_index, (intptr_t)p_foreign_name, true);
    EmitExportedDefinition(t_handler_def);
}

static void BuildBuiltinModule_ForeignHandler2(intptr_t p_return_type,
                                               MCHandlerTypeFieldMode p_arg_mode_0, const char *p_arg_name_0, intptr_t p_arg_type_0,
                                               MCHandlerTypeFieldMode p_arg_mode_1, const char *p_arg_name_1, intptr_t p_arg_type_1,
                                               const char *p_name, const char *p_foreign_name)
{
    intptr_t t_handler_type_index = -1;
    EmitBeginForeignHandlerType(p_return_type);
    EmitHandlerTypeParameter(p_arg_mode_0, nameref_from_cstring(p_arg_name_0), p_arg_type_0);
    EmitHandlerTypeParameter(p_arg_mode_1, nameref_from_cstring(p_arg_name_1), p_arg_type_1);
    EmitEndHandlerType(t_handler_type_index);
    
    intptr_t t_handler_def = -1;
    EmitDefinitionIndex("foreignhandler", t_handler_def);
    EmitForeignHandlerDefinition(t_handler_def, 0, nameref_from_cstring(p_name), t_handler_type_index, (intptr_t)p_foreign_name, true);
    EmitExportedDefinition(t_handler_def);
}

void BuildBuiltinModule(void)
{
    intptr_t t_mod_index;
    EmitBeginModule(nameref_from_cstring("__builtin__"), t_mod_index);
    
    intptr_t t_null_type_def, t_bool_type_def, t_double_type_def, t_uint_type_def, t_string_type_def;
    BuildBuiltinModule_ForeignType("any", "MCAnyTypeInfo");
    BuildBuiltinModule_ForeignType("undefined", "MCNullTypeInfo", &t_null_type_def);
    BuildBuiltinModule_ForeignType("boolean", "MCBooleanTypeInfo");
    BuildBuiltinModule_ForeignType("number", "MCNumberTypeInfo");
    BuildBuiltinModule_ForeignType("string", "MCStringTypeInfo", &t_string_type_def);
    BuildBuiltinModule_ForeignType("data", "MCDataTypeInfo");
    BuildBuiltinModule_ForeignType("array", "MCArrayTypeInfo");
    BuildBuiltinModule_ForeignType("list", "MCProperListTypeInfo");
    BuildBuiltinModule_ForeignType("bool", "MCForeignBoolTypeInfo", &t_bool_type_def);
    BuildBuiltinModule_ForeignType("sint", "MCForeignSIntTypeInfo");
    BuildBuiltinModule_ForeignType("uint", "MCForeignUIntTypeInfo", &t_uint_type_def);
    BuildBuiltinModule_ForeignType("float", "MCForeignFloatTypeInfo");
    BuildBuiltinModule_ForeignType("double", "MCForeignDoubleTypeInfo", &t_double_type_def);
    BuildBuiltinModule_ForeignType("pointer", "MCForeignPointerTypeInfo");
    
    BuildBuiltinModule_ForeignHandler1(t_bool_type_def,
                                       kMCHandlerTypeFieldModeInOut,
                                       "count",
                                       t_uint_type_def,
                                       "RepeatCounted",
                                       "MCScriptBuiltinRepeatCounted");
    
    BuildBuiltinModule_ForeignHandler2(t_bool_type_def,
                                       kMCHandlerTypeFieldModeIn,
                                       "counter",
                                       t_double_type_def,
                                       kMCHandlerTypeFieldModeIn,
                                       "limit",
                                       t_double_type_def,
                                       "RepeatUpToCondition",
                                       "MCScriptBuiltinRepeatUpToCondition");
                                       
    BuildBuiltinModule_ForeignHandler2(t_double_type_def,
                                       kMCHandlerTypeFieldModeIn,
                                       "counter",
                                       t_double_type_def,
                                       kMCHandlerTypeFieldModeIn,
                                       "step",
                                       t_double_type_def,
                                       "RepeatUpToIterate",
                                       "MCScriptBuiltinRepeatUpToIterate");
                                       
    BuildBuiltinModule_ForeignHandler2(t_bool_type_def,
                                       kMCHandlerTypeFieldModeIn,
                                       "counter",
                                       t_double_type_def,
                                       kMCHandlerTypeFieldModeIn,
                                       "step",
                                       t_double_type_def,
                                       "RepeatDownToCondition",
                                       "MCScriptBuiltinRepeatDownToCondition");
    
    BuildBuiltinModule_ForeignHandler2(t_double_type_def,
                                       kMCHandlerTypeFieldModeIn,
                                       "counter",
                                       t_double_type_def,
                                       kMCHandlerTypeFieldModeIn,
                                       "step",
                                       t_double_type_def,
                                       "RepeatDownToIterate",
                                       "MCScriptBuiltinRepeatDownToIterate");
                                       
    BuildBuiltinModule_ForeignHandler1(t_null_type_def,
                                       kMCHandlerTypeFieldModeIn,
                                       "reason",
                                       t_string_type_def,
                                       "Throw",
                                       "MCScriptBuiltinThrow");
    
    EmitEndModule();
}

//////////

extern "C" void InitializeFoundation(void);
void InitializeFoundation(void)
{
    if (!MCInitialize())
        Fatal_InternalInconsistency("unable to initialize foundation");
}

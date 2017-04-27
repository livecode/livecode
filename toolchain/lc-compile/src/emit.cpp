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
#include <stdio.h>
#include <sys/stat.h>

#ifndef WIN32
#include <unistd.h>
#endif

extern "C" int OutputFileAsC;
int OutputFileAsC = 0;
extern "C" int OutputFileAsBytecode;
int OutputFileAsBytecode = 0;

extern "C" void EmitStart(void);
extern "C" void EmitFinish(void);

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
extern "C" void EmitDefinitionIndex(const char *type, long& r_index);
extern "C" void EmitTypeDefinition(long index, PositionRef position, NameRef name, long type_index);
extern "C" void EmitConstantDefinition(long p_index, PositionRef p_position, NameRef p_name, long p_const_index);
extern "C" void EmitVariableDefinition(long index, PositionRef position, NameRef name, long type_index);
extern "C" void EmitBeginHandlerDefinition(long index, PositionRef position, NameRef name, long type_index);
extern "C" void EmitBeginUnsafeHandlerDefinition(long index, PositionRef position, NameRef name, long type_index);
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
extern "C" void EmitOptionalType(long index, long& r_new_index);extern "C" void EmitAnyType(long& r_new_index);
extern "C" void EmitBooleanType(long& r_new_index);
extern "C" void EmitIntegerType(long& r_new_index);
extern "C" void EmitRealType(long& r_new_index);
extern "C" void EmitNumberType(long& r_new_index);
extern "C" void EmitStringType(long& r_new_index);
extern "C" void EmitDataType(long& r_new_index);
extern "C" void EmitArrayType(long& r_new_index);
extern "C" void EmitListType(long& r_new_index);
extern "C" void EmitUndefinedType(long& r_new_index);
extern "C" void EmitBeginRecordType(void);
extern "C" void EmitRecordTypeField(NameRef name, long type_index);
extern "C" void EmitEndRecordType(long& r_type_index);
extern "C" void EmitBeginHandlerType(long return_type_index);
extern "C" void EmitBeginForeignHandlerType(long return_type_index);
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
extern "C" void EmitBeginOpcode(long opcode);
extern "C" void EmitContinueOpcode(long argument);
extern "C" void EmitEndOpcode(void);
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
extern "C" void EmitBeginInvoke(long index, long contextreg, long resultreg);
extern "C" void EmitBeginIndirectInvoke(long handlerreg, long contextreg, long resultreg);
extern "C" void EmitContinueInvoke(long reg);
extern "C" void EmitEndInvoke(void);
extern "C" void EmitAssign(long dst, long src);
extern "C" void EmitAssignConstant(long dst, long constidx);
extern "C" void EmitUndefinedConstant(long *idx);
extern "C" void EmitTrueConstant(long *idx);
extern "C" void EmitFalseConstant(long *idx);
extern "C" void EmitIntegerConstant(long value, long *idx);
extern "C" void EmitUnsignedIntegerConstant(unsigned long value, long *idx);
extern "C" void EmitRealConstant(long value, long *idx);
extern "C" void EmitStringConstant(long value, long *idx);
extern "C" void EmitBeginListConstant(void);
extern "C" void EmitContinueListConstant(long idx);
extern "C" void EmitEndListConstant(long *idx);
extern "C" void EmitBeginArrayConstant(void);
extern "C" void EmitContinueArrayConstant(long key_idx, long value_idx);
extern "C" void EmitEndArrayConstant(long *idx);
extern "C" void EmitBeginAssignList(long reg);
extern "C" void EmitContinueAssignList(long reg);
extern "C" void EmitEndAssignList(void);
extern "C" void EmitBeginAssignArray(long reg);
extern "C" void EmitContinueAssignArray(long reg);
extern "C" void EmitEndAssignArray(void);
extern "C" void EmitFetch(long reg, long var, long level);
extern "C" void EmitStore(long reg, long var, long level);
extern "C" void EmitReturn(long reg);
extern "C" void EmitReturnNothing(void);
extern "C" void EmitReset(long reg);
extern "C" void EmitAttachRegisterToExpression(long reg, long expr);
extern "C" void EmitDetachRegisterFromExpression(long expr);
extern "C" int EmitGetRegisterAttachedToExpression(long expr, long *reg);
extern "C" void EmitPosition(PositionRef position);
extern "C" void EmitCast(long type_idx, long out_reg, long in_reg);

extern "C" void OutputBeginManifest(void);

extern "C" int IsBootstrapCompile(void);

extern "C" void DependStart(void);
extern "C" void DependFinish(void);
extern "C" void DependDefineMapping(NameRef module_name, const char *source_file);
extern "C" void DependDefineDependency(NameRef module_name, NameRef dependency_name);

extern "C" int BytecodeEnumerate(long index, long *r_name);
extern "C" int BytecodeLookup(long name, long *r_opcode);
extern "C" void BytecodeDescribe(long opcode, long *r_name);
extern "C" int BytecodeIsValidArgumentCount(long opcode, long count);
extern "C" void BytecodeDescribeParameter(long opcode, long index, long *r_type);

//////////

struct AttachedReg
{
    AttachedReg *next;
    long expr;
    long reg;
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

static MCStringRef to_mcstringref(long p_string)
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

struct EmittedModule
{
    EmittedModule *next;
    NameRef name;
    char *modified_name;
    bool has_foreign : 1;
};
static EmittedModule *s_emitted_modules = NULL;

//////////

static const char *kOutputCDefinitions =
    "struct __builtin_module_info\n"
    "{\n"
    "    void *next;\n"
    "    void *handle;\n"
    "    unsigned char *data;\n"
    "    unsigned long length;\n"
    "    bool (*initializer)();\n"
    "    void (*finalizer)();\n"
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

void EmitStart(void)
{
    MCInitialize();

    s_output_code_file = OpenOutputCodeFile(&s_output_code_filename);
    s_emitted_modules = NULL;
    
    if (s_output_code_file != NULL)
    {
        if (fprintf(s_output_code_file, "%s", kOutputCDefinitions) < 0)
            goto error_cleanup;
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

void EmitFinish(void)
{
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

void EmitBeginModule(NameRef p_name, long& r_index)
{
    MCInitialize();

    Debug_Emit("BeginModule(%s) -> 0", cstring_from_nameref(p_name));

    s_module_name = p_name;
    
    MCScriptBeginModule(kMCScriptModuleKindNone, to_mcnameref(p_name), s_builder);
    r_index = 0;
}

void EmitBeginWidgetModule(NameRef p_name, long& r_index)
{
    MCInitialize();

    Debug_Emit("BeginWidgetModule(%s) -> 0", cstring_from_nameref(p_name));

    s_module_name = p_name;
    
    MCScriptBeginModule(kMCScriptModuleKindWidget, to_mcnameref(p_name), s_builder);
    r_index = 0;
}

void EmitBeginLibraryModule(NameRef p_name, long& r_index)
{
    MCInitialize();

    Debug_Emit("BeginLibraryModule(%s) -> 0", cstring_from_nameref(p_name));

    s_module_name = p_name;
    
    MCScriptBeginModule(kMCScriptModuleKindLibrary, to_mcnameref(p_name), s_builder);
    r_index = 0;
}

static bool
EmitEndModuleGetByteCodeBuffer (MCAutoByteArray & r_bytecode)
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
	r_bytecode.Give ((byte_t *) t_bytecode, (uindex_t)t_bytecode_len);

	return true;

 error_cleanup:
	Error_CouldNotGenerateBytecode();
	return false;
}

static bool
EmitEndModuleOutputBytecode (const byte_t *p_bytecode,
                             size_t p_bytecode_len)
{
	const char *t_filename = nil;
	FILE *t_file = OpenOutputBytecodeFile (&t_filename);

	if (nil == t_file)
		goto error_cleanup;

	size_t t_written;
	t_written = fwrite (p_bytecode, sizeof(byte_t), p_bytecode_len, t_file);

	if (t_written != p_bytecode_len)
		goto error_cleanup;

	fflush (t_file);
	fclose (t_file);

	return true;

 error_cleanup:
	if (nil != t_file)
		fclose (t_file);
	Error_CouldNotWriteOutputFile (t_filename);
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

	if (0 > fprintf(t_file, "static __builtin_module_info __%s_module_info = {\n    0,\n    0,\n    %s_module_data,\n    sizeof(%s_module_data),\n    %s_Initialize,\n    %s_Finalize };\n", t_modified_name, t_modified_name, t_modified_name, t_modified_name, t_modified_name))
		goto error_cleanup;
    
    if (0 > fprintf(t_file, "static __builtin_module_loader __%s_loader(&__%s_module_info);\n\n", t_modified_name, t_modified_name))
        goto error_cleanup;
    
    EmittedModule *t_mod;
    t_mod = (EmittedModule *)Allocate(sizeof(EmittedModule));
    t_mod -> next = s_emitted_modules;
    t_mod -> name = p_module_name;
    t_mod -> modified_name = t_modified_name;
    s_emitted_modules = t_mod;
    
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

	MCAutoByteArray t_bytecode;
	const byte_t *t_bytecode_buf = nil;
	size_t t_bytecode_len = 0;

	MCAutoByteArray t_interface;
	const byte_t *t_interface_buf = nil;
	size_t t_interface_len = 0;

	Debug_Emit("EndModule()");

    __EmitModuleOrder(s_module_name);
    
	GetStringOfNameLiteral(s_module_name, &t_module_string);
	MCAssert (nil != t_module_string);

	/* ---------- 1. Get bytecode */

	if (!EmitEndModuleGetByteCodeBuffer (t_bytecode))
		goto cleanup;

	t_bytecode_buf = t_bytecode.Bytes();
	t_bytecode_len = t_bytecode.ByteCount();

	/* ---------- 2. Output module contents */
	if (OutputFileAsC)
	{
		if (!EmitEndModuleOutputC (s_module_name, t_module_string,
		                           t_bytecode_buf, t_bytecode_len))
			goto cleanup;
	}
	else if (OutputFileAsBytecode)
	{
		if (!EmitEndModuleOutputBytecode (t_bytecode_buf, t_bytecode_len))
			goto cleanup;
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

void EmitModuleDependency(NameRef p_name, long& r_index)
{
    __EmitModuleOrder(p_name);
    
    uindex_t t_index;
    MCScriptAddDependencyToModule(s_builder, to_mcnameref(p_name), t_index);
    r_index = t_index + 1;

    Debug_Emit("ModuleDependency(%s -> %d)", cstring_from_nameref(p_name), t_index + 1);
}

void EmitImportedType(long p_module_index, NameRef p_name, long p_type_index, long& r_index)
{
    uindex_t t_index;
    MCScriptAddImportToModule(s_builder, (uindex_t)p_module_index - 1, to_mcnameref(p_name), kMCScriptDefinitionKindType, (uindex_t)p_type_index, t_index);
    r_index = t_index;

    Debug_Emit("ImportedType(%ld, %s, %ld -> %d)", p_module_index,
               cstring_from_nameref(p_name), p_type_index, t_index);
}

void EmitImportedConstant(long p_module_index, NameRef p_name, long p_type_index, long& r_index)
{
    uindex_t t_index;
    MCScriptAddImportToModule(s_builder, (uindex_t)p_module_index - 1, to_mcnameref(p_name), kMCScriptDefinitionKindConstant, (uindex_t)p_type_index, t_index);
    r_index = t_index;

    Debug_Emit("ImportedConstant(%ld, %s, %ld -> %d)", p_module_index,
               cstring_from_nameref(p_name), p_type_index, t_index);
}

void EmitImportedVariable(long p_module_index, NameRef p_name, long p_type_index, long& r_index)
{
    uindex_t t_index;
    MCScriptAddImportToModule(s_builder, (uindex_t)p_module_index - 1, to_mcnameref(p_name), kMCScriptDefinitionKindVariable, (uindex_t)p_type_index, t_index);
    r_index = t_index;

    Debug_Emit("ImportedVariable(%ld, %s, %ld -> %d)", p_module_index,
               cstring_from_nameref(p_name), p_type_index, t_index);
}

void EmitImportedHandler(long p_module_index, NameRef p_name, long p_type_index, long& r_index)
{
    uindex_t t_index;
    MCScriptAddImportToModule(s_builder, (uindex_t)p_module_index - 1, to_mcnameref(p_name), kMCScriptDefinitionKindHandler, (uindex_t)p_type_index, t_index);
    r_index = t_index;

    Debug_Emit("ImportedHandler(%ld, %s, %ld -> %d)", p_module_index,
               cstring_from_nameref(p_name), p_type_index, t_index);
}

void EmitImportedSyntax(long p_module_index, NameRef p_name, long p_type_index, long& r_index)
{
    uindex_t t_index;
    MCScriptAddImportToModule(s_builder, (uindex_t)p_module_index - 1, to_mcnameref(p_name), kMCScriptDefinitionKindSyntax, (uindex_t)p_type_index, t_index);
    r_index = t_index;

    Debug_Emit("ImportedSyntax(%ld, %s, %ld -> %d)", p_module_index,
               cstring_from_nameref(p_name), p_type_index, t_index);
}

void EmitExportedDefinition(long p_index)
{
    MCScriptAddExportToModule(s_builder, (uindex_t)p_index);

    Debug_Emit("ExportedDefinition(%ld)", p_index);
}

void EmitDefinitionIndex(const char *p_type, long& r_index)
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

void EmitTypeDefinition(long p_index, PositionRef p_position, NameRef p_name, long p_type_index)
{
    MCScriptAddTypeToModule(s_builder, to_mcnameref(p_name), (uindex_t)p_type_index, (uindex_t)p_index);

    Debug_Emit("TypeDefinition(%ld, %s, %ld)", p_index,
               cstring_from_nameref(p_name), p_type_index);
}

void EmitConstantDefinition(long p_index, PositionRef p_position, NameRef p_name, long p_const_index)
{
    MCScriptAddConstantToModule(s_builder, to_mcnameref(p_name), (uindex_t)p_const_index, (uindex_t)p_index);

    Debug_Emit("ConstantDefinition(%ld, %s, %ld)", p_index,
               cstring_from_nameref(p_name), p_const_index);
}

void EmitVariableDefinition(long p_index, PositionRef p_position, NameRef p_name, long p_type_index)
{
    MCScriptAddVariableToModule(s_builder, to_mcnameref(p_name), (uindex_t)p_type_index, (uindex_t)p_index);

    Debug_Emit("VariableDefinition(%ld, %s, %ld)", p_index,
               cstring_from_nameref(p_name), p_type_index);
}

void EmitForeignHandlerDefinition(long p_index, PositionRef p_position, NameRef p_name, long p_type_index, long p_binding)
{
    MCAutoStringRef t_binding_str;
    if (strcmp((const char *)p_binding, "<builtin>") == 0)
        t_binding_str = MCNameGetString(to_mcnameref(p_name));
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

        long t_module_dep;
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

void EmitPropertyDefinition(long p_index, PositionRef p_position, NameRef p_name, long p_getter, long p_setter)
{
    MCScriptAddPropertyToModule(s_builder, to_mcnameref(p_name), (uindex_t)p_getter, (uindex_t)p_setter, (uindex_t)p_index);

    Debug_Emit("PropertyDefinition(%ld, %s, %ld, %ld)", p_index,
               cstring_from_nameref(p_name), p_getter, p_setter);
}

void EmitEventDefinition(long p_index, PositionRef p_position, NameRef p_name, long p_type_index)
{
    MCScriptAddEventToModule(s_builder, to_mcnameref(p_name), (uindex_t)p_type_index, (uindex_t)p_index);

    Debug_Emit("EmitEvent(%ld, %s, %ld)", p_index, cstring_from_nameref(p_name),
               p_type_index);
}

void EmitBeginHandlerDefinition(long p_index, PositionRef p_position, NameRef p_name, long p_type_index)
{
    MCScriptBeginHandlerInModule(s_builder, to_mcnameref(p_name), (uindex_t)p_type_index, kMCScriptHandlerAttributeSafe, (uindex_t)p_index);

    Debug_Emit("BeginHandlerDefinition(%ld, %s, %ld)", p_index,
               cstring_from_nameref(p_name), p_type_index);
}

void EmitBeginUnsafeHandlerDefinition(long p_index, PositionRef p_position, NameRef p_name, long p_type_index)
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

void EmitBeginSyntaxDefinition(long p_index, PositionRef p_position, NameRef p_name)
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

void EmitBeginSyntaxMethod(long p_handler_index)
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

void EmitIntegerSyntaxMethodArgument(long p_int)
{
    MCAutoNumberRef t_number;
    MCNumberCreateWithInteger((uindex_t)p_int, &t_number);
    MCScriptAddConstantArgumentToSyntaxMethodInModule(s_builder, *t_number);

    Debug_Emit("IntegerSyntaxMethodArgument(%ld)", p_int);
}

void EmitRealSyntaxMethodArgument(long p_double)
{
    MCAutoNumberRef t_number;
    MCNumberCreateWithInteger((uindex_t)p_double, &t_number);
    MCScriptAddConstantArgumentToSyntaxMethodInModule(s_builder, *t_number);

    Debug_Emit("RealSyntaxMethodArgument(%lf)", *(double *)p_double);
}

void EmitStringSyntaxMethodArgument(long p_string)
{
	MCAutoStringRef t_string(to_mcstringref(p_string));
    MCScriptAddConstantArgumentToSyntaxMethodInModule(s_builder, *t_string);

    Debug_Emit("RealSyntaxMethodArgument(\"%s\")", (const char *)p_string);
}

void EmitVariableSyntaxMethodArgument(long p_index)
{
    MCScriptAddVariableArgumentToSyntaxMethodInModule(s_builder, (uindex_t)p_index);

    Debug_Emit("VariableSyntaxMethodArgument(%ld)", p_index);
}

void EmitIndexedVariableSyntaxMethodArgument(long p_var_index, long p_element_index)
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

void EmitContinueDefinitionGroup(long p_index)
{
    MCScriptAddHandlerToDefinitionGroupInModule(s_builder, (uindex_t)p_index);

    Debug_Emit("ContinueDefinitionGroup(%ld)", p_index);
}

void EmitEndDefinitionGroup(long *r_index)
{
    uindex_t t_index;
    MCScriptEndDefinitionGroupInModule(s_builder, t_index);
    *r_index = t_index;

    Debug_Emit("EndDefinitionGroup(-> %ld)", *r_index);
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

    Debug_Emit("NamedType(%s, %s -> %ld)", cstring_from_nameref(module_name),
               cstring_from_nameref(name), r_new_index);
}

void EmitAliasType(NameRef name, long target_index, long& r_new_index)
{
    MCAutoTypeInfoRef t_type;
    MCAliasTypeInfoCreate(to_mcnameref(name), to_mctypeinforef(target_index), &t_type);
    if (!define_typeinfo(*t_type, r_new_index))
        return;

    Debug_Emit("AliasType(%s, %ld -> %ld)", to_mcnameref(name), target_index, r_new_index);
}
#endif

void EmitDefinedType(long index, long& r_type_index)
{
    uindex_t t_type_index;
    MCScriptAddDefinedTypeToModule(s_builder, (uindex_t)index, t_type_index);
    r_type_index = t_type_index;

    Debug_Emit("DefinedType(%ld -> %ld)", index, r_type_index);
}

void EmitForeignType(long p_binding, long& r_type_index)
{
    uindex_t t_type_index;
    MCScriptAddForeignTypeToModule(s_builder, to_mcstringref(p_binding), t_type_index);
    r_type_index = t_type_index;

    Debug_Emit("ForeignType(%s -> %ld)", (const char *)p_binding, r_type_index);
}

void EmitOptionalType(long base_index, long& r_new_index)
{
    uindex_t t_index;
    MCScriptAddOptionalTypeToModule(s_builder, (uindex_t)base_index, t_index);
    r_new_index = t_index;

    Debug_Emit("OptionalType(%ld -> %ld)", base_index, r_new_index);
}

void EmitAnyType(long& r_new_index)
{
    if (!define_builtin_typeinfo("any", r_new_index))
        return;

    Debug_Emit("AnyType(-> %ld)", r_new_index);
}

void EmitBooleanType(long& r_new_index)
{
    if (!define_builtin_typeinfo("Boolean", r_new_index))
        return;

    Debug_Emit("BooleanType(-> %ld)", r_new_index);
}

void EmitIntegerType(long& r_new_index)
{
    if (!define_builtin_typeinfo("Number", r_new_index))
        return;

    Debug_Emit("IntegerType(-> %ld)", r_new_index);
}

void EmitRealType(long& r_new_index)
{
    // TODO: Real / Integer types.
    if (!define_builtin_typeinfo("Number", r_new_index))
        return;

    Debug_Emit("RealType(-> %ld)", r_new_index);
}

void EmitNumberType(long& r_new_index)
{
    if (!define_builtin_typeinfo("Number", r_new_index))
        return;

    Debug_Emit("NumberType(-> %ld)", r_new_index);
}

void EmitStringType(long& r_new_index)
{
    if (!define_builtin_typeinfo("String", r_new_index))
        return;

    Debug_Emit("StringType(-> %ld)", r_new_index);
}

void EmitDataType(long& r_new_index)
{
    if (!define_builtin_typeinfo("Data", r_new_index))
        return;

    Debug_Emit("DataType(-> %ld)", r_new_index);
}

void EmitArrayType(long& r_new_index)
{
    if (!define_builtin_typeinfo("Array", r_new_index))
        return;

    Debug_Emit("ArrayType(-> %ld)", r_new_index);
}

void EmitListType(long& r_new_index)
{
    if (!define_builtin_typeinfo("List", r_new_index))
        return;

    Debug_Emit("ListType(-> %ld)", r_new_index);
}

void EmitUndefinedType(long& r_new_index)
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

void EmitRecordTypeField(NameRef name, long type_index)
{
    MCScriptContinueRecordTypeInModule(s_builder, to_mcnameref(name), (uindex_t)type_index);
    Debug_Emit("RecordTypeField(%s, %ld)", cstring_from_nameref(name),
               type_index);
}

void EmitEndRecordType(long& r_type_index)
{
    uindex_t t_index;
    MCScriptEndRecordTypeInModule(s_builder, t_index);
    r_type_index = t_index;

    Debug_Emit("EndRecordType(-> %ld)", r_type_index);
}

//////////

void EmitBeginHandlerType(long return_type_index)
{
    MCScriptBeginHandlerTypeInModule(s_builder, (uindex_t)return_type_index);

    Debug_Emit("BeginHandlerType(%ld)", return_type_index);
}

void EmitBeginForeignHandlerType(long return_type_index)
{
    MCScriptBeginForeignHandlerTypeInModule(s_builder, (uindex_t)return_type_index);
    
    Debug_Emit("BeginForeignHandlerType(%ld)", return_type_index);
}

static void EmitHandlerTypeParameter(MCHandlerTypeFieldMode mode, NameRef name, long type_index)
{
    MCScriptContinueHandlerTypeInModule(s_builder, (MCScriptHandlerTypeParameterMode)mode, to_mcnameref(name), (uindex_t)type_index);

    Debug_Emit("HandlerTypeParameter(%d, %s, %ld)", mode,
               cstring_from_nameref(name), type_index);
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

    Debug_Emit("EndHandlerType(-> %ld)", t_index);
}

///////////

void EmitHandlerParameter(NameRef name, long type_index, long& r_index)
{
    uindex_t t_index;
    MCScriptAddParameterToHandlerInModule(s_builder, to_mcnameref(name), (uindex_t)type_index, t_index);
    r_index = t_index;

    Debug_Emit("HandlerParameter(%s, %ld -> %ld)", cstring_from_nameref(name),
               type_index, r_index);
}

void EmitHandlerVariable(NameRef name, long type_index, long& r_index)
{
    uindex_t t_index;
    MCScriptAddVariableToHandlerInModule(s_builder, to_mcnameref(name), (uindex_t)type_index, t_index);
    r_index = t_index;

    Debug_Emit("HandlerVariable(%s, %ld -> %ld)", cstring_from_nameref(name),
               type_index, r_index);
}

void EmitDeferLabel(long& r_label)
{
    uindex_t t_index;
    MCScriptDeferLabelForBytecodeInModule(s_builder, t_index);
    r_label = t_index;

    Debug_Emit("DeferLabel(-> %ld)", r_label);
}

void EmitResolveLabel(long label)
{
    MCScriptResolveLabelForBytecodeInModule(s_builder, (uindex_t)label);

    Debug_Emit("ResolveLabel(%ld)", label);
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

    Debug_Emit("CreateRegister(-> %ld)", r_regindex);
}

void EmitDestroyRegister(long regindex)
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
    long head;
    long tail;
};

static RepeatLabels *s_repeat_labels = nil;

void EmitPushRepeatLabels(long next, long exit)
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

void EmitCurrentRepeatLabels(long& r_next, long& r_exit)
{
    r_next = s_repeat_labels -> head;
    r_exit = s_repeat_labels -> tail;
}

//////////

void EmitUndefinedConstant(long *idx)
{
    uindex_t t_index;
    MCScriptAddValueToModule(s_builder, kMCNull, t_index);
    *idx = t_index;
    
    Debug_Emit("UndefinedConstant(-> %ld)", *idx);
}

void EmitTrueConstant(long *idx)
{
    uindex_t t_index;
    MCScriptAddValueToModule(s_builder, kMCTrue, t_index);
    *idx = t_index;
    
    Debug_Emit("TrueConstant(-> %ld)", *idx);
}

void EmitFalseConstant(long *idx)
{
    uindex_t t_index;
    MCScriptAddValueToModule(s_builder, kMCFalse, t_index);
    *idx = t_index;
    
    Debug_Emit("FalseConstant(%ld)", *idx);
}

void EmitIntegerConstant(long value, long *idx)
{
    MCAutoNumberRef t_number;
    uindex_t t_index;
    MCNumberCreateWithInteger((integer_t)value, &t_number);
    MCScriptAddValueToModule(s_builder, *t_number, t_index);
    *idx = t_index;
    
    Debug_Emit("IntegerConstant(%ld -> %ld)", value, *idx);
}

void EmitUnsignedIntegerConstant(unsigned long value, long *idx)
{
    MCAutoNumberRef t_number;
    uindex_t t_index;
    MCNumberCreateWithUnsignedInteger((uinteger_t)value, &t_number);
    MCScriptAddValueToModule(s_builder, *t_number, t_index);
    *idx = t_index;
    
    Debug_Emit("UnsignedIntegerConstant(%lu -> %ld)", value, *idx);
}

void EmitRealConstant(long value, long *idx)
{
    MCAutoNumberRef t_number;
    uindex_t t_index;
    MCNumberCreateWithReal(*(double *)value, &t_number);
    MCScriptAddValueToModule(s_builder, *t_number, t_index);
    *idx = t_index;
    
    Debug_Emit("RealConstant(%lf -> %ld)", *(double *)value, *idx);
}

void EmitStringConstant(long value, long *idx)
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

void EmitContinueListConstant(long idx)
{
    MCScriptContinueListValueInModule(s_builder, (uindex_t)idx);
    
    Debug_Emit("ContinueListConstant(%ld)", idx);
}

void EmitEndListConstant(long *idx)
{
    MCScriptEndListValueInModule(s_builder, (uindex_t&)*idx);
    
    Debug_Emit("EndListConstant(-> %ld)", *idx);
}

void EmitBeginArrayConstant(void)
{
    MCScriptBeginArrayValueInModule(s_builder);
    
    Debug_Emit("BeginArrayConstant()", 0);
}

void EmitContinueArrayConstant(long key_idx, long value_idx)
{
    MCScriptContinueArrayValueInModule(s_builder, (uindex_t)key_idx, (uindex_t)value_idx);
    
    Debug_Emit("ContinueArrayConstant(%ld, %ld)", key_idx, value_idx);
}

void EmitEndArrayConstant(long *idx)
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

static void push_argument(long arg)
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

void EmitBeginOpcode(long name)
{
    __opcode_index t_opcode((const char *)name);
    s_opcode = t_opcode;
}

void EmitContinueOpcode(long param)
{
    push_argument(param);
}

void EmitEndOpcode(void)
{
    MCScriptEmitBytecodeInModuleA(s_builder, s_opcode, s_arguments, s_argument_count);
    
    log_instruction();
    
    s_argument_count = 0;
}

void EmitJump(long label)
{
    static const __opcode_index kJumpOpcodeIndex("jump");
    MCScriptEmitBytecodeInModule(s_builder, kJumpOpcodeIndex, label, UINDEX_MAX);
    
    Debug_Emit("Jump(%ld)", label);
}

void EmitJumpIfTrue(long reg, long label)
{
    static const __opcode_index kJumpIfTrueOpcodeIndex("jump_if_true");
    MCScriptEmitBytecodeInModule(s_builder, kJumpIfTrueOpcodeIndex, reg, label, UINDEX_MAX);
    
    Debug_Emit("JumpIfTrue(%ld, %ld)", label);
}

void EmitJumpIfFalse(long reg, long label)
{
    static const __opcode_index kJumpIfFalseOpcodeIndex("jump_if_false");
    MCScriptEmitBytecodeInModule(s_builder, kJumpIfFalseOpcodeIndex, reg, label, UINDEX_MAX);
    
    Debug_Emit("JumpIfFalse(%ld, %ld)", reg, label);
}

void EmitBeginInvoke(long index, long contextreg, long resultreg)
{
    static const __opcode_index kInvokeOpcodeIndex("invoke");
    
    s_opcode = kInvokeOpcodeIndex;
    push_argument(index);
    push_argument(resultreg);
}

void EmitBeginIndirectInvoke(long handlerreg, long contextreg, long resultreg)
{
    static const __opcode_index kInvokeIndirectOpcodeIndex("invoke_indirect");
    
    s_opcode = kInvokeIndirectOpcodeIndex;
    push_argument(handlerreg);
    push_argument(resultreg);
}

void EmitContinueInvoke(long reg)
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

void EmitBeginAssignList(long reg)
{
    static const __opcode_index kAssignListOpcodeIndex("assign_list");
    
    s_opcode = kAssignListOpcodeIndex;
    push_argument(reg);
}

void EmitContinueAssignList(long reg)
{
    push_argument(reg);
}

void EmitEndAssignList(void)
{
    MCScriptEmitBytecodeInModuleA(s_builder, s_opcode, s_arguments, s_argument_count);
    
    log_instruction();
    
    s_argument_count = 0;
}

void EmitBeginAssignArray(long reg)
{
    static const __opcode_index kAssignArrayOpcodeIndex("assign_array");
    
    s_opcode = kAssignArrayOpcodeIndex;
    push_argument(reg);
}

void EmitContinueAssignArray(long reg)
{
    push_argument(reg);
}

void EmitEndAssignArray(void)
{
    MCScriptEmitBytecodeInModuleA(s_builder, s_opcode, s_arguments, s_argument_count);
    
    log_instruction();
    
    s_argument_count = 0;
}

void EmitAssign(long dst, long src)
{
    static const __opcode_index kAssignOpcodeIndex("assign");
    MCScriptEmitBytecodeInModule(s_builder, kAssignOpcodeIndex, dst, src, UINDEX_MAX);
    
    Debug_Emit("Assign(%ld, %ld)", dst, src);
}

void EmitAssignConstant(long dst, long idx)
{
    static const __opcode_index kAssignConstantOpcodeIndex("assign_constant");
    MCScriptEmitBytecodeInModule(s_builder, kAssignConstantOpcodeIndex, dst, idx, UINDEX_MAX);

    Debug_Emit("AssignConstant(%ld, %ld)", dst, idx);
}

/////////

void EmitFetch(long reg, long var, long level)
{
    static const __opcode_index kFetchOpcodeIndex("fetch");
    MCScriptEmitBytecodeInModule(s_builder, kFetchOpcodeIndex, reg, var, UINDEX_MAX);

    Debug_Emit("Fetch(%ld, %ld)", reg, var);
}

void EmitStore(long reg, long var, long level)
{
    static const __opcode_index kStoreOpcodeIndex("store");
    MCScriptEmitBytecodeInModule(s_builder, kStoreOpcodeIndex, reg, var, UINDEX_MAX);

    Debug_Emit("Store(%ld, %ld)", reg, var);
}

void EmitReturn(long reg)
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

void EmitReset(long reg)
{
    static const __opcode_index kResetOpcodeIndex("reset");
    MCScriptEmitBytecodeInModule(s_builder, kResetOpcodeIndex, reg, UINDEX_MAX);
    
    Debug_Emit("Reset(%ld)", reg);
}

////////

void EmitCast(long type_idx, long out_reg, long in_reg)
{
    static const __opcode_index kCastOpcodeIndex("cast");
    MCScriptEmitBytecodeInModule(s_builder, kCastOpcodeIndex, type_idx, out_reg, in_reg, UINDEX_MAX);
    
    Debug_Emit("Cast(%ld, %ld, %ld)", type_idx, out_reg, in_reg);
}

////////

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
    
    if (t_remove != nil)
        Debug_Emit("DetachRegister(%d, %p)", t_remove -> reg, t_remove -> expr);
    
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

int BytecodeEnumerate(long index, long *r_name)
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
    *r_name = (long)nameref_from_mcstringref(t_name);
    
    return 1;
}

int BytecodeLookup(long name, long *r_opcode)
{
    uindex_t t_opcode;
    if (!MCScriptLookupBytecode((const char *)name, t_opcode))
        return 0;
    
    *r_opcode = t_opcode;
    return 1;
}

void BytecodeDescribe(long opcode, long *r_name)
{
    *r_name = (long)MCScriptDescribeBytecode((uindex_t)opcode);
}

int BytecodeIsValidArgumentCount(long opcode, long count)
{
    return MCScriptCheckBytecodeParameterCount((uindex_t)opcode, (uindex_t)count) ? 1 : 0;
}

void BytecodeDescribeParameter(long opcode, long index, long *r_type)
{
    *r_type = (long)MCScriptDescribeBytecodeParameter((uindex_t)opcode, (uindex_t)index);
}

//////////

extern "C" void InitializeFoundation(void);
void InitializeFoundation(void)
{
    if (!MCInitialize())
        Fatal_InternalInconsistency("unable to initialize foundation");
}

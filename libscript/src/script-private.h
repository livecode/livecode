/* Copyright (C) 2003-2015 LiveCode Ltd.
 
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

#ifndef __MC_SCRIPT_PRIVATE__
#define __MC_SCRIPT_PRIVATE__

#include <stdlib.h>

// Win32 doesn't have the "__func__" macro
#ifdef _WIN32
#define __func__ __FUNCTION__
#endif

////////////////////////////////////////////////////////////////////////////////

extern MCTypeInfoRef kMCScriptOutParameterNotDefinedErrorTypeInfo;
extern MCTypeInfoRef kMCScriptInParameterNotDefinedErrorTypeInfo;
extern MCTypeInfoRef kMCScriptVariableUsedBeforeDefinedErrorTypeInfo;
extern MCTypeInfoRef kMCScriptInvalidReturnValueErrorTypeInfo;
extern MCTypeInfoRef kMCScriptInvalidVariableValueErrorTypeInfo;
extern MCTypeInfoRef kMCScriptInvalidArgumentValueErrorTypeInfo;
extern MCTypeInfoRef kMCScriptNotABooleanValueErrorTypeInfo;
extern MCTypeInfoRef kMCScriptNotAStringValueErrorTypeInfo;
extern MCTypeInfoRef kMCScriptWrongNumberOfArgumentsErrorTypeInfo;
extern MCTypeInfoRef kMCScriptForeignHandlerBindingErrorTypeInfo;
extern MCTypeInfoRef kMCScriptMultiInvokeBindingErrorTypeInfo;
extern MCTypeInfoRef kMCScriptNoMatchingHandlerErrorTypeInfo;
extern MCTypeInfoRef kMCScriptCannotSetReadOnlyPropertyErrorTypeInfo;
extern MCTypeInfoRef kMCScriptInvalidPropertyValueErrorTypeInfo;
extern MCTypeInfoRef kMCScriptNotAHandlerValueErrorTypeInfo;
extern MCTypeInfoRef kMCScriptCannotCallContextHandlerErrorTypeInfo;
extern MCTypeInfoRef kMCScriptPropertyNotFoundErrorTypeInfo;
extern MCTypeInfoRef kMCScriptHandlerNotFoundErrorTypeInfo;

////////////////////////////////////////////////////////////////////////////////

enum MCScriptObjectKind
{
    kMCScriptObjectKindNone,
    kMCScriptObjectKindPackage,
    kMCScriptObjectKindModule,
    kMCScriptObjectKindInstance,
};

struct MCScriptObject
{
#ifndef NDEBUG
    uint32_t __object_marker__;
#endif
    uint32_t references;
    MCScriptObjectKind kind;
};

bool MCScriptCreateObject(MCScriptObjectKind kind, size_t size, MCScriptObject*& r_object);
void MCScriptDestroyObject(MCScriptObject *object);

MCScriptObject *MCScriptRetainObject(MCScriptObject *object);
void MCScriptReleaseObject(MCScriptObject *object);

void MCScriptReleaseObjectArray(MCScriptObject **elements, uindex_t count);

template<typename T> inline void MCScriptReleaseArray(T **elements, uindex_t count)
{
    MCScriptReleaseObjectArray((MCScriptObject **)elements, count);
}

#ifndef NDEBUG

#define __MCSCRIPTOBJECT_MARKER__ 0xFEDEFECE

extern void __MCScriptValidateObjectFailed__(MCScriptObject *object, const char *function, const char *file, int line);
extern void __MCScriptValidateObjectAndKindFailed__(MCScriptObject *object, MCScriptObjectKind kind, const char *function, const char *file, int line);
extern void __MCScriptAssertFailed__(const char *label, const char *expr, const char *function, const char *file, int line);

#define __MCScriptValidateObject__(obj) (((obj) == nil || (obj) -> __object_marker__ != __MCSCRIPTOBJECT_MARKER__) ? __MCScriptValidateObjectFailed__((obj), __func__, __FILE__, __LINE__) : (void)0)
#define __MCScriptValidateObjectAndKind__(obj, m_kind) (((obj) == nil || (obj) -> __object_marker__ != __MCSCRIPTOBJECT_MARKER__) || (obj) -> kind != (m_kind) ? __MCScriptValidateObjectAndKindFailed__((obj), m_kind, __func__, __FILE__, __LINE__) : (void)0)
#define __MCScriptAssert__(expr, label) ((!(expr)) ? __MCScriptAssertFailed__(label, #expr, __func__, __FILE__, __LINE__) : (void)0)
#define __MCScriptUnreachable__(label) (__MCScriptAssert__(false, (label)))

#else

#define __MCScriptValidateObject__(obj) do { } while (false)
#define __MCScriptValidateObjectAndKind__(obj, kind) do { } while (false)
#define __MCScriptAssert__(expr, label) do { } while (false)
#define __MCScriptUnreachable__(label) do { } while (false)

#endif

////////////////////////////////////////////////////////////////////////////////

struct MCScriptPackage: public MCScriptObject
{
    // The filename of the package - this file has to remain around whilst the package
    // is loaded otherwise bad things happen (the user of the library has to guarantee this).
    MCStringRef filename;

    // The name of the package.
    MCNameRef name;
    
    // The list of modules that make up this package.
    MCScriptModuleRef *modules;
    uindex_t module_count;
};

void MCScriptDestroyPackage(MCScriptPackageRef package);

////////////////////////////////////////////////////////////////////////////////

enum MCScriptTypeKind
{
    kMCScriptTypeKindDefined,
    kMCScriptTypeKindForeign,
    kMCScriptTypeKindOptional,
    kMCScriptTypeKindHandler,
    kMCScriptTypeKindRecord,
    kMCScriptTypeKindForeignHandler,
};

struct MCScriptType
{
    MCScriptTypeKind kind;
    
    // (computed)
    MCTypeInfoRef typeinfo;
};

struct MCScriptDefinedType: public MCScriptType
{
    uindex_t index;
};

struct MCScriptForeignType: public MCScriptType
{
    MCStringRef binding;
};

struct MCScriptOptionalType: public MCScriptType
{
    uindex_t type;
};

struct MCScriptHandlerTypeParameter
{
    MCScriptHandlerTypeParameterMode mode;
    uindex_t type;
};

struct MCScriptHandlerType: public MCScriptType
{
    MCScriptHandlerTypeParameter *parameters;
    uindex_t parameter_count;
    uindex_t return_type;
    
    // This information can be stripped.
    MCNameRef *parameter_names;
    uindex_t parameter_name_count;
};

struct MCScriptRecordTypeField
{
    MCNameRef name;
    uindex_t type;
};

struct MCScriptRecordType: public MCScriptType
{
    uindex_t base_type;
    MCScriptRecordTypeField *fields;
    uindex_t field_count;
};

////////////////////////////////////////////////////////////////////////////////

struct MCScriptDefinition;

struct MCScriptExportedDefinition
{
    MCNameRef name;
    uindex_t index;
};

struct MCScriptDependency
{
    MCNameRef name;
    uindex_t version;
    
    // The resolved instance - not pickled
    MCScriptInstanceRef instance;
};

struct MCScriptSyntaxMethod
{
    uindex_t handler;
    uindex_t *arguments;
    uindex_t argument_count;
};

struct MCScriptImportedDefinition
{
    uindex_t module;
    MCScriptDefinitionKind kind;
    MCNameRef name;
    
    // The module the definition resides in.
    MCScriptModuleRef resolved_module;
    // The resolved definition - not pickled
    MCScriptDefinition *resolved_definition;
};

struct MCScriptPosition
{
    uindex_t address;
    uindex_t file;
    uindex_t line;
};

struct MCScriptDefinition
{
    MCScriptDefinitionKind kind;
};

struct MCScriptExternalDefinition: public MCScriptDefinition
{
    uindex_t index;
};

struct MCScriptTypeDefinition: public MCScriptDefinition
{
    uindex_t type;
};

struct MCScriptConstantDefinition: public MCScriptDefinition
{
	uindex_t value;
};

struct MCScriptVariableDefinition: public MCScriptDefinition
{
	uindex_t type;
    
    // (computed) The index of the variable in an instance's slot table - not pickled
	uindex_t slot_index;
};

struct MCScriptContextVariableDefinition: public MCScriptDefinition
{
    uindex_t type;
    uindex_t default_value;
    
    // (computed) The index of the variable in the context slot table - not pickled
    uindex_t slot_index;
};

struct MCScriptCommonHandlerDefinition: public MCScriptDefinition
{
    uindex_t type;
};

struct MCScriptHandlerDefinition: public MCScriptCommonHandlerDefinition
{
    uindex_t *local_types;
    uindex_t local_type_count;
    
    MCNameRef *local_names;
    uindex_t local_name_count;
    
	uindex_t start_address;
	uindex_t finish_address;
    
    MCScriptHandlerScope scope;
    
    // The number of slots required in a frame in order to execute this handler - computed.
    uindex_t slot_count;
};

struct MCScriptDefinitionGroupDefinition: public MCScriptDefinition
{
    uindex_t *handlers;
    uindex_t handler_count;
};

struct MCScriptSyntaxDefinition: public MCScriptDefinition
{
    // The number of inputs.
    uindex_t variable_count;
    
    // The list of methods.
    MCScriptSyntaxMethod *methods;
    uindex_t method_count;
};

struct MCScriptForeignHandlerDefinition: public MCScriptCommonHandlerDefinition
{
    MCStringRef binding;
    
    // Bound function information - not pickled.
    void *function;
    void *function_argtypes;
    void *function_cif;
};

struct MCScriptPropertyDefinition: public MCScriptDefinition
{
	uindex_t getter;
	uindex_t setter;
};

struct MCScriptEventDefinition: public MCScriptDefinition
{
	uindex_t type;
};

MCScriptVariableDefinition *MCScriptDefinitionAsVariable(MCScriptDefinition *definition);
MCScriptHandlerDefinition *MCScriptDefinitionAsHandler(MCScriptDefinition *definition);
MCScriptForeignHandlerDefinition *MCScriptDefinitionAsForeignHandler(MCScriptDefinition *definition);

////////////////////////////////////////////////////////////////////////////////

struct MCScriptModule: public MCScriptObject
{
    // The owning package - not pickled
    MCScriptPackageRef package;
    
    // The type of module.
    MCScriptModuleKind module_kind;
    
    // The name of the module (value_pool)
    MCNameRef name;
    
    // The list of dependencies (value_pool)
    MCScriptDependency *dependencies;
    uindex_t dependency_count;
    
    // The value pool for the module - this holds references to all the constants
    // used in the module structure.
    MCValueRef *values;
    uindex_t value_count;
    
    // The type pool for the module - this abstracts typeinfo so that
    // they can refer to definitions.
    MCScriptType **types;
    uindex_t type_count;
    
    // The exported definitions. This is a list of all public definitions mapping name to
    // index.
    MCScriptExportedDefinition *exported_definitions;
    uindex_t exported_definition_count;
    
    // The imported definitions. This is a list of all public definitions imported from
    // another module.
    MCScriptImportedDefinition *imported_definitions;
    uindex_t imported_definition_count;
    
    // The definition list. This is a table of all definitions in the module that
    // are referenced whether through export, or internally.
    MCScriptDefinition **definitions;
    uindex_t definition_count;
    
    // The bytecode used by the module.
    uint8_t *bytecode;
    uindex_t bytecode_count;
    
    // The following information may not be present if debugging info has been
    // stripped.
    MCNameRef *definition_names;
    uindex_t definition_name_count;
    MCNameRef *source_files;
    uindex_t source_file_count;
    MCScriptPosition *positions;
    uindex_t position_count;
    
    // After a module has been validated and had its dependencies resolved, this
    // var is true - not pickled
    bool is_usable : 1;
    // During the check for usability, this var is true - not pickled
    bool is_in_usable_check : 1;
    
    // (computed) The number of slots needed by an instance - not pickled
    uindex_t slot_count;
    
    // (computed) The number of slots needed by this modules context - not pickled
    uindex_t context_slot_count;
    
    // (computed) The index of this module's context info in a frame's context vector - not pickled
    uindex_t context_index;
    
    // If this is a non-widget module, then it only has one instance - not pickled
    MCScriptInstanceRef shared_instance;
    
    // This is the module-chain link. We keep a linked list of all modules in memory
    // with unique names -- not pickled.
    MCScriptModule *next_module;
};

bool MCScriptWriteRawModule(MCStreamRef stream, MCScriptModule *module);
bool MCScriptReadRawModule(MCStreamRef stream, MCScriptModule *module);
void MCScriptReleaseRawModule(MCScriptModule *module);

void MCScriptDestroyModule(MCScriptModuleRef module);

bool MCScriptLookupPropertyDefinitionInModule(MCScriptModuleRef module, MCNameRef property, MCScriptPropertyDefinition*& r_definition);
bool MCScriptLookupEventDefinitionInModule(MCScriptModuleRef module, MCNameRef property, MCScriptEventDefinition*& r_definition);
bool MCScriptLookupHandlerDefinitionInModule(MCScriptModuleRef module, MCNameRef handler, MCScriptHandlerDefinition*& r_definition);
bool MCScriptLookupDefinitionInModule(MCScriptModuleRef self, MCNameRef p_name, MCScriptDefinition*& r_definition);

MCNameRef MCScriptGetNameOfDefinitionInModule(MCScriptModuleRef module, MCScriptDefinition *definition);
MCNameRef MCScriptGetNameOfParameterInModule(MCScriptModuleRef module, MCScriptDefinition *definition, uindex_t index);
MCNameRef MCScriptGetNameOfLocalVariableInModule(MCScriptModuleRef module, MCScriptDefinition *definition, uindex_t index);
MCNameRef MCScriptGetNameOfGlobalVariableInModule(MCScriptModuleRef module, uindex_t index);
MCNameRef MCScriptGetNameOfContextVariableInModule(MCScriptModuleRef module, uindex_t index);

////////////////////////////////////////////////////////////////////////////////

struct MCScriptHandlerValue
{
    MCScriptCommonHandlerDefinition *definition;
    MCHandlerRef value;
};

struct MCScriptInstance: public MCScriptObject
{
    // The module defining the instance.
    MCScriptModuleRef module;
    
    // The module's array of slots (module -> slot_count in length).
    MCValueRef *slots;
    
    // The instance's handler refs. This is a mapping from handler def to
    // MCHandlerRef. The MCHandlerRefs are freed when the instance is freed.
    MCScriptHandlerValue *handlers;
    uindex_t handler_count;
};

void MCScriptDestroyInstance(MCScriptInstanceRef instance);

bool MCScriptCallHandlerOfInstanceInternal(MCScriptInstanceRef instance, MCScriptHandlerDefinition *handler, MCValueRef *arguments, uindex_t argument_count, MCValueRef& r_result);

// Evaluate the value of the given handler definition. This will create an
// appropriate MCHandlerRef which can then be called. The instance retains the
// handler ref and releases when the instance goes away. This implicitly means
// that any C function ptrs generated from MCHandlerRefs have lifetime equivalent
// to that of the instance.
//
// If the definition is a foreign function and it cannot be bound, then nil is
// returned for the handler (i.e. it is not an error).
//
// The function returns false if there is a memory error.
bool MCScriptEvaluateHandlerOfInstanceInternal(MCScriptInstanceRef instance, MCScriptCommonHandlerDefinition *definition, MCHandlerRef& r_handler);

////////////////////////////////////////////////////////////////////////////////

// The bytecode represents a simple register machine. It is designed to be very
// high-level and (easily) verifiably safe.
//
// It has a stack of activation frames, with each frame contaning an array of
// variables (parameters and locals) and an array of registers.
//
// Each variable and register can hold a single (boxed) value.
//
// Variables are strongly typed, registers can hold any value.
//
// Variables are accessed via fetch-local and store-local. Fetch-local requires
// that non-optionally typed variables are defined; store-local requires that
// the value being stored conforms type-wise with the variable.
//
// Global variables are accessed in an identical fashion, except they use
// fetch-global and store-global.
//
// Each instruction is represented by a single byte, with arguments being encoded
// sequentially as multi-byte (signed) integers.
//
// The top nibble of each op indicates how many subsequent parameters there are.
// If the top nibble is 15 then, an extension byte follows. If the extension byte is
// present then the instruction requires 15 + extension byte arguments up to a
// maximum of 256.

enum MCScriptBytecodeOp
{
	// Unconditional jump:
	//  X: jump <Y-X>
	// Location is encoded as relative position to jump instruction.
	kMCScriptBytecodeOpJump,
	
	// Conditional jumps:
	//  X: jump* <register>, <Y - X>
	// Location is encoded as relative position to jump instruction.
	// Register is used for test.
    //
    // It is a runtime error if <register> does not contain a boolean.
	//
    kMCScriptBytecodeOpJumpIfFalse,
	kMCScriptBytecodeOpJumpIfTrue,
	
	// Constant register assignment:
	//   assign-constant <dst>, <index>
	// Dst is a register and index is a constant pool index. The value in dst is
    // freed, and the constant value at the specified index is assigned to it.
	kMCScriptBytecodeOpAssignConstant,
    
	// Register assignment:
	//   assign <dst>, <src>
	// Dst and Src are registers. The value in dst is freed, and src copied
	// into it.
	kMCScriptBytecodeOpAssign,
    
	// Return control to caller with value:
	//   return [<reg>]
    // Return from a call. If <reg> is not specified, then the return value is taken
    // to be undefined.
    //
    // It is an error if <reg> does not contain a value conforming to the return
    // type of the executing handler.
    //
	kMCScriptBytecodeOpReturn,
    
	// Direct handler invocation:
	//   invoke <index>, [<result>], <arg_1>, ..., <arg_n>
	// Handler with index <index> is invoked with the given registers as arguments.
    // If the
    //
    // It is a runtime error if the number of arguments is different from the
    // signature of <index>.
    // It is a runtime error if for a non-out parameter, the contents of <arg_i>
    // does not conform to the type of the parameter required by the signature.
    // It is a runtime error if, on-exit, any out parameters which have a non-optional
    // type are undefined.
    //
	kMCScriptBytecodeOpInvoke,
	
	// Indirect handler invocation:
	//   invoke *<handler>, <result>, <arg_1>, ..., <arg_n>
	// The handler reference in register <handler> is invoked with the given registers
	// as arguments.
    //
    // Conformance rules are the same as for normal invoke, except the signature of
    // the handler is potentially dynamic.
    //
	kMCScriptBytecodeOpInvokeIndirect,
    
	// Fetch:
	//   fetch <dst>, <index>, [<level>]
	// Evaluates definition index <index> in level <level> and assigns the result to
    // register <dst>. If <level> is not present then it is taken to be 0.
    //
    // The <level> indicates where the definition should be looked up with 0 being
    // the enclosing scope, 1 the next scope and so on.
    //
    //
	kMCScriptBytecodeOpFetch,
    
	// Store:
	//   store <src>, <index>, [<level>]
	// Stores the value currently held in <src> into definition index <index> at level
    // <level>. If <level> is not present then it is taken to be 0.
    //
    // The <level> indicates where the definition should be looked up with 0 being
    // the enclosing scope, 1 the next scope and so on.
    //
	kMCScriptBytecodeOpStore,
    
    // List creation assignment.
    //   assign-list <dst>, <arg_1>, ..., <arg_n>
    // Dst is a register. The remaining arguments are registers and are used to
    // build a list. (This will be replaced by an invoke when variadic bindings are
    // implemented).
    kMCScriptBytecodeOpAssignList,

	// Array creation assignment.
	//   assign-array <dst>, <key_1>, <value_1>, ..., <key_n>, <value_n>
	// Dst is a register.  The remaining arguments are registers and
	// are used, pair-wise, to build an array. (This will be replaced by an invoke
	// when variadic bindings are implemented).
	kMCScriptBytecodeOpAssignArray,
};

bool MCScriptBytecodeIterate(byte_t*& x_bytecode, byte_t *p_bytecode_limit, MCScriptBytecodeOp& r_op, uindex_t& r_arity, uindex_t *r_arguments);

////////////////////////////////////////////////////////////////////////////////

// Compiled modules are serialized to disk in the following format:
//
//   byte       magic[0] = 'L'
//   byte       magic[1] = 'C'
//   byte    	version[0]
//   byte       version[1]
//   <pickle of module struct>

// The module version will be incremented for every public release of the libscript
// which occurs in which the module binary format changes.
//
// We will only aim to support module formats which have been released as final and
// stable.
//
// The following constants keep track of, and should be updated to reflect the meaning
// of each module version.

#define kMCScriptModuleVersion_8_0_0_DP_1 0
#define kMCScriptCurrentModuleVersion kMCScriptModuleVersion_8_0_0_DP_1

////////////////////////////////////////////////////////////////////////////////

#endif

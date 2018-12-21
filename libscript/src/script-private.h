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
#include "libscript/script.h"

// Win32 doesn't have the "__func__" macro
#ifdef _WIN32
#define __func__ __FUNCTION__
#endif

////////////////////////////////////////////////////////////////////////////////

extern MCTypeInfoRef kMCScriptVariableUsedBeforeAssignedErrorTypeInfo;
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
extern MCTypeInfoRef kMCScriptPropertyUsedBeforeAssignedErrorTypeInfo;
extern MCTypeInfoRef kMCScriptInvalidPropertyValueErrorTypeInfo;
extern MCTypeInfoRef kMCScriptNotAHandlerValueErrorTypeInfo;
extern MCTypeInfoRef kMCScriptPropertyNotFoundErrorTypeInfo;
extern MCTypeInfoRef kMCScriptHandlerNotFoundErrorTypeInfo;

////////////////////////////////////////////////////////////////////////////////

MCSLibraryRef MCScriptGetLibrary(void);
bool MCScriptLoadLibrary(MCScriptModuleRef module, MCStringRef name, MCSLibraryRef& r_path);

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

template <typename ScriptObjectType>
bool MCScriptCreateObject(MCScriptObjectKind kind, ScriptObjectType*& r_object)
{
    MCScriptObject* t_new = nullptr;
    if (!MCScriptCreateObject(kind, sizeof(ScriptObjectType), t_new))
        return false;
    r_object = reinterpret_cast<ScriptObjectType*>(t_new);
    return true;
}

MCScriptObject *MCScriptRetainObject(MCScriptObject *object);
void MCScriptReleaseObject(MCScriptObject *object);
uint32_t MCScriptGetRetainCountOfObject(MCScriptObject *object);

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

	/* Translate the mode of this parameter from a
	 * MCScriptHandlerTypeParameterMode to an
	 * MCHandlerTypeFieldMode. */
	inline MCHandlerTypeFieldMode GetFieldMode() const
	{
		switch (mode)
		{
		case kMCScriptHandlerTypeParameterModeIn:
			return kMCHandlerTypeFieldModeIn;
		case kMCScriptHandlerTypeParameterModeOut:
			return kMCHandlerTypeFieldModeOut;
		case kMCScriptHandlerTypeParameterModeInOut:
			return kMCHandlerTypeFieldModeInOut;
		case kMCScriptHandlerTypeParameterModeVariadic:
			return kMCHandlerTypeFieldModeVariadic;
		default:
			MCUnreachableReturn(kMCHandlerTypeFieldModeIn);
		}
	}
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
    
    MCScriptHandlerAttributes attributes;
    
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
    MCScriptForeignHandlerLanguage language : 8;
    MCScriptThreadAffinity thread_affinity : 8;
    union
    {
        struct
        {
            void *function;
            void *function_cif;
        } c;
        struct
        {
            void *function;
        } builtin_c;
        struct
        {
            MCScriptForeignHandlerObjcCallType call_type : 8;
            void *objc_class;
            void *objc_selector;
            void *function_cif;
        } objc;
        struct
        {
            MCNameRef class_name;
            void *method_id;
            int call_type : 8;
        } java;
    };
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
    // The module is licensed for use - not pickled
    bool licensed : 1;

    // (computed) The number of slots needed by an instance - not pickled
    uindex_t slot_count;
    
    // If this is a non-widget module, then it only has one instance - not pickled
    MCScriptInstanceRef shared_instance;
    
    // This is the module-chain link. We keep a linked list of all modules in memory
    // with unique names -- not pickled.
    MCScriptModule *next_module;
    
    // These are the native code initializer/finalizer (if any) -- not pickled
    bool (*initializer)(void);
    void (*finalizer)(void);
    
    // This is the ordinal mapping array (if any) -- not pickled
    void **builtins;
    
    // This is a map from library name used in the module to MCSLibraryRef
    // all foreign handler definitions which have the same library string share
    // a single MCSLibraryRef -- not pickled
    MCArrayRef libraries;
};

bool MCScriptWriteRawModule(MCStreamRef stream, MCScriptModule *module);
bool MCScriptReadRawModule(MCStreamRef stream, MCScriptModule *module);
void MCScriptReleaseRawModule(MCScriptModule *module);

void MCScriptDestroyModule(MCScriptModuleRef module);

bool MCScriptLookupConstantDefinitionInModule(MCScriptModuleRef module, MCNameRef property, MCScriptConstantDefinition*& r_definition);
bool MCScriptLookupPropertyDefinitionInModule(MCScriptModuleRef module, MCNameRef property, MCScriptPropertyDefinition*& r_definition);
bool MCScriptLookupEventDefinitionInModule(MCScriptModuleRef module, MCNameRef property, MCScriptEventDefinition*& r_definition);
bool MCScriptLookupHandlerDefinitionInModule(MCScriptModuleRef module, MCNameRef handler, MCScriptHandlerDefinition*& r_definition);
bool MCScriptLookupDefinitionInModule(MCScriptModuleRef self, MCNameRef p_name, MCScriptDefinition*& r_definition);

MCNameRef MCScriptGetNameOfDefinitionInModule(MCScriptModuleRef module, MCScriptDefinition *definition);
MCNameRef MCScriptGetNameOfLocalVariableInModule(MCScriptModuleRef module, MCScriptHandlerDefinition *definition, uindex_t index);
MCNameRef MCScriptGetNameOfGlobalVariableInModule(MCScriptModuleRef module, MCScriptVariableDefinition *definition);
MCNameRef MCScriptGetNameOfParameterInModule(MCScriptModuleRef module, MCScriptCommonHandlerDefinition *definition, uindex_t index);

MCTypeInfoRef MCScriptGetTypeOfLocalVariableInModule(MCScriptModuleRef module, MCScriptHandlerDefinition *definition, uindex_t index);
MCTypeInfoRef MCScriptGetTypeOfGlobalVariableInModule(MCScriptModuleRef module, MCScriptVariableDefinition *definition);
MCTypeInfoRef MCScriptGetTypeOfParameterInModule(MCScriptModuleRef module, MCScriptCommonHandlerDefinition *definition, uindex_t index);
MCTypeInfoRef MCScriptGetTypeOfReturnValueInModule(MCScriptModuleRef module, MCScriptCommonHandlerDefinition *definition);

MCNameRef MCScriptGetNameOfPropertyTypeInModule(MCScriptModuleRef module, MCScriptPropertyDefinition *definition);
MCNameRef MCScriptGetNameOfLocalVariableTypeInModule(MCScriptModuleRef module, MCScriptHandlerDefinition *definition, uindex_t index);
MCNameRef MCScriptGetNameOfReturnValueTypeInModule(MCScriptModuleRef module, MCScriptCommonHandlerDefinition *definition);
MCNameRef MCScriptGetNameOfParameterTypeInModule(MCScriptModuleRef module, MCScriptCommonHandlerDefinition *definition, uindex_t index);
MCNameRef MCScriptGetNameOfGlobalVariableTypeInModule(MCScriptModuleRef module, MCScriptVariableDefinition *definition);

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
    
    // The private host ptr, ignored by libscript but can be used by the host.
    void *host_ptr;
};

MCScriptModuleRef MCScriptSetCurrentModule(MCScriptModuleRef module);

void
MCScriptDestroyInstance(MCScriptInstanceRef instance);

bool
MCScriptCallHandlerInInstanceInternal(MCScriptInstanceRef instance,
									  MCScriptHandlerDefinition *handler,
									  MCValueRef *arguments,
									  uindex_t argument_count,
									  MCValueRef& r_result);

bool
MCScriptEvaluateHandlerInInstanceInternal(MCScriptInstanceRef instance,
										  MCScriptCommonHandlerDefinition *handler,
										  MCHandlerRef& r_handler);

bool
MCScriptTryToBindForeignHandlerInInstanceInternal(MCScriptInstanceRef instance,
												  MCScriptForeignHandlerDefinition *handler,
												  bool& r_bound);

bool
MCScriptBindForeignHandlerInInstanceInternal(MCScriptInstanceRef instance,
											 MCScriptForeignHandlerDefinition *handler);

bool
MCScriptHandlerIsInternal(MCHandlerRef handler);

void
MCScriptInternalHandlerQuery(MCHandlerRef handler,
							 MCScriptInstanceRef& r_instance,
							 MCScriptCommonHandlerDefinition*& r_definition);

////////////////////////////////////////////////////////////////////////////////

bool
MCScriptThrowPropertyNotFoundError(MCScriptInstanceRef instance,
								   MCNameRef property);

bool
MCScriptThrowCannotSetReadOnlyPropertyError(MCScriptInstanceRef instance,
                                   MCNameRef property);

bool
MCScriptThrowHandlerNotFoundError(MCScriptInstanceRef instance,
								  MCNameRef handler);
bool
MCScriptThrowPropertyUsedBeforeAssignedError(MCScriptInstanceRef instance,
											 MCScriptPropertyDefinition *property_def);
bool
MCScriptThrowInvalidValueForPropertyError(MCScriptInstanceRef instance,
										  MCScriptPropertyDefinition *property_def,
                                          MCTypeInfoRef property_type,
										  MCValueRef provided_value);

bool
MCScriptThrowWrongNumberOfArgumentsError(MCScriptInstanceRef instance,
										 MCScriptCommonHandlerDefinition *handler_def,
										 uindex_t provided_argument_count);

bool
MCScriptThrowInvalidValueForArgumentError(MCScriptInstanceRef instance,
										  MCScriptCommonHandlerDefinition *handler_def,
										  uindex_t argument_index,
										  MCValueRef provided_value);

bool
MCScriptThrowInvalidValueForReturnValueError(MCScriptInstanceRef instance,
											 MCScriptCommonHandlerDefinition *handler_def,
											 MCValueRef provided_value);

bool
MCScriptThrowNotAHandlerValueError(MCValueRef actual_value);

bool
MCScriptThrowNotAStringValueError(MCValueRef actual_value);

bool
MCScriptThrowNotABooleanOrBoolValueError(MCValueRef actual_value);

bool
MCScriptThrowGlobalVariableUsedBeforeAssignedError(MCScriptInstanceRef instance,
												   MCScriptVariableDefinition *variable_def);

bool
MCScriptThrowInvalidValueForGlobalVariableError(MCScriptInstanceRef instance,
												MCScriptVariableDefinition *variable_def,
												MCValueRef provided_value);

bool
MCScriptThrowLocalVariableUsedBeforeAssignedError(MCScriptInstanceRef instance,
												  MCScriptHandlerDefinition *handler,
												  uindex_t index);

bool
MCScriptThrowInvalidValueForLocalVariableError(MCScriptInstanceRef instance,
											   MCScriptHandlerDefinition *handler,
											   uindex_t index,
											   MCValueRef provided_value);

bool
MCScriptThrowUnableToResolveMultiInvokeError(MCScriptInstanceRef instance,
											 MCScriptDefinitionGroupDefinition *group,
											 MCProperListRef argument_values);

bool
MCScriptThrowUnableToResolveForeignHandlerError(MCScriptInstanceRef instance,
												MCScriptForeignHandlerDefinition *handler);

bool
MCScriptThrowUnknownForeignLanguageError(void);

bool
MCScriptThrowUnknownForeignCallingConventionError(void);

bool
MCScriptThrowMissingFunctionInForeignBindingError(void);

bool
MCScriptThrowUnableToLoadForiegnLibraryError(void);

bool
MCScriptThrowForeignExceptionError(MCStringRef p_reason);

bool
MCScriptThrowObjCBindingNotSupported(void);

bool
MCScriptThrowJavaBindingNotSupported(void);

bool
MCScriptThrowUnknownThreadAffinityError(void);

bool
MCScriptCreateErrorExpectedError(MCErrorRef& r_error);

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

enum
{
    kMCScriptBytecodeOpCodeMask = 0x0f,
    kMCScriptBytecodeOpCodeShift = 0,
    kMCScriptBytecodeOpCodeMax = kMCScriptBytecodeOpCodeMask >> kMCScriptBytecodeOpCodeShift,
    
    kMCScriptBytecodeOpArityMask = 0xf0,
    kMCScriptBytecodeOpArityShift = 4,
    kMCScriptBytecodeOpArityMax = kMCScriptBytecodeOpArityMask >> kMCScriptBytecodeOpArityShift,
};

enum MCScriptBytecodeOp
{
	kMCScriptBytecodeOp__First,
	
	// Unconditional jump:
	//  X: jump <Y-X>
	// Location is encoded as relative position to jump instruction.
    //
	kMCScriptBytecodeOpJump = kMCScriptBytecodeOp__First,
	
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
    //
	kMCScriptBytecodeOpAssignConstant,
    
	// Register assignment:
	//   assign <dst>, <src>
	// Dst and Src are registers. The value in dst is freed, and src copied
	// into it.
    //
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
    //
    kMCScriptBytecodeOpAssignList,

	// Array creation assignment.
	//   assign-array <dst>, <key_1>, <value_1>, ..., <key_n>, <value_n>
	// Dst is a register.  The remaining arguments are registers and
	// are used, pair-wise, to build an array. (This will be replaced by an invoke
    // when variadic bindings are implemented).
    //
	kMCScriptBytecodeOpAssignArray,
    
    // Slot resetting
    //   reset <reg_1>, ..., <reg_n>
    // Initializes the given slots to default values, if the type of the slot
    // has a default, otherwise makes the slot unassigned.
    kMCScriptBytecodeOpReset,
	
	kMCScriptBytecodeOp__Last = kMCScriptBytecodeOpReset
};

inline void
MCScriptBytecodeDecodeOp(const byte_t*& x_bytecode_ptr,
						 MCScriptBytecodeOp& r_op,
						 uindex_t& r_arity)
{
	byte_t t_op_byte;
	t_op_byte = *x_bytecode_ptr++;
	
	// The lower nibble is the bytecode operation.
	MCScriptBytecodeOp t_op;
	t_op = (MCScriptBytecodeOp)((t_op_byte & kMCScriptBytecodeOpCodeMask) >> kMCScriptBytecodeOpCodeShift);
	
	// The upper nibble is the arity.
	uindex_t t_arity;
	t_arity = (t_op_byte & kMCScriptBytecodeOpArityMask) >> kMCScriptBytecodeOpArityShift;
	
	// If the arity is 15, then overflow to a subsequent byte.
	if (t_arity == kMCScriptBytecodeOpArityMax)
		t_arity += *x_bytecode_ptr++;
	
	r_op = t_op;
	r_arity = t_arity;
}

// TODO: Make this better for negative numbers.
inline uindex_t
MCScriptBytecodeDecodeArgument(const byte_t*& x_bytecode_ptr)
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

inline bool
MCScriptBytecodeIterate(const byte_t*& x_bytecode,
						const byte_t *p_bytecode_limit,
						MCScriptBytecodeOp& r_op,
						uindex_t& r_arity,
						uindex_t *r_arguments)
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

inline void
MCScriptBytecodeDecode(const byte_t*& x_bytecode_ptr,
					   MCScriptBytecodeOp& r_operation,
					   uindex_t* r_arguments,
					   uindex_t& r_argument_count)
{
	MCScriptBytecodeDecodeOp(x_bytecode_ptr,
							 r_operation,
							 r_argument_count);
	for(uindex_t i = 0; i < r_argument_count; i++)
	{
		r_arguments[i] = MCScriptBytecodeDecodeArgument(x_bytecode_ptr);
	}
}

inline index_t
MCScriptBytecodeDecodeSignedArgument(uindex_t p_original_value)
{
	index_t t_value;
	if ((p_original_value & 1) == 0)
		t_value = (signed)(p_original_value >> 1);
	else
		t_value = -(signed)(p_original_value >> 1);
	return t_value;
}

////////////////////////////////////////////////////////////////////////////////

#endif

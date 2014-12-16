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
extern MCTypeInfoRef kMCScriptWrongNumberOfArgumentsErrorTypeInfo;
extern MCTypeInfoRef kMCScriptForeignHandlerBindingErrorTypeInfo;
extern MCTypeInfoRef kMCScriptMultiInvokeBindingErrorTypeInfo;
extern MCTypeInfoRef kMCScriptTypeBindingErrorTypeInfo;

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

#define __MCScriptValidateObject__(obj)
#define __MCScriptValidateObjectAndKind__(obj, kind)
#define __MCScriptAssert__(expr, label)
#define __MCScriptUnreachable__(label)

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
    //MCTypeInfoRef type;
    
    // The resolved definition - not pickled
    MCScriptDefinition *definition;
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
	MCValueRef value;
};

struct MCScriptVariableDefinition: public MCScriptDefinition
{
	uindex_t type;
    
    // (computed) The index of the variable in an instance's slot table - not pickled
	uindex_t slot_index;
};

struct MCScriptHandlerDefinition: public MCScriptDefinition
{
	uindex_t type;
    
    uindex_t *locals;
    uindex_t local_count;
    
	uindex_t start_address;
	uindex_t finish_address;
    
    // The names of the locals - this information can be stripped.
    MCNameRef *local_names;
    uindex_t local_name_count;
    
    // The number of slots required in a frame in order to execute this handler.
    // This is the sum of parameter count, local count and temporary count - not pickled
    uindex_t slot_count;
    
    // The start of the registers.
    uindex_t register_offset;
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

struct MCScriptForeignHandlerDefinition: public MCScriptDefinition
{
    uindex_t type;
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
    
    // (computed) The number of slots needed by an instance - not pickled
    uindex_t slot_count;
    
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

////////////////////////////////////////////////////////////////////////////////

struct MCScriptInstance: public MCScriptObject
{
    // The module defining the instance.
    MCScriptModuleRef module;
    
    // The module's array of slots (module -> slot_count in length).
    MCValueRef *slots;
};

void MCScriptDestroyInstance(MCScriptInstanceRef instance);

bool MCScriptCallHandlerOfInstanceInternal(MCScriptInstanceRef instance, MCScriptHandlerDefinition *handler, MCValueRef *arguments, uindex_t argument_count, MCValueRef& r_result);

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
	//   return <reg>
    // Return from a call.
    //
    // It is an error if <reg> does not contain a value conforming to the return
    // type of the executing handler.
    //
	kMCScriptBytecodeOpReturn,
    
	// Direct handler invocation:
	//   invoke <index>, <result>, <arg_1>, ..., <arg_n>
	// Handler with index <index> is invoked with the given registers as arguments.
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
    
	// Local fetch:
	//   fetch-local <dst>, <local-index>
	// Assigns the current value of <glob-index> to register <dst>.
    //
    // It is a runtime error if the type of the variable is non-optional and it
    // has an undefined value.
    //
	kMCScriptBytecodeOpFetchLocal,
	// Local store:
	//   store-local <src>, <local-index>
	// Assigns the current value of register <src> to <glob-index>.
    //
    // It is a runtime error if the type of the value in <src> does not conform
    // to the type of the target local variable.
    //
	kMCScriptBytecodeOpStoreLocal,
    
	// Global fetch:
	//   fetch-global <dst>, <glob-index>
	// Assigns the current value of <glob-index> to register <dst>.
    //
    // It is a runtime error if the type of the variable is non-optional and it
    // has an undefined value.
    //
	kMCScriptBytecodeOpFetchGlobal,
	// Global store:
	//   store-global: <src>, <glob-index>
	// Assigns the current value of register <src> to <glob-index>.
    //
    // It is a runtime error if the type of the value in <src> does not conform
    // to the type of the target global variable.
    //
	kMCScriptBytecodeOpStoreGlobal,
    
    // List creation assignment.
    //   assign-list <dst>, <arg_1>, ..., <arg_n>
    // Dst is a register. The remaining arguments are registers and are used to
    // build a list. (This will be replaced by an invoke when variadic bindings are
    // implemented).
    kMCScriptBytecodeOpAssignList,
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

////////////////////////////////////////////////////////////////////////////////

#endif

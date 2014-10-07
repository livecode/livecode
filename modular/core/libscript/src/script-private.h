#ifndef __MC_SCRIPT_PRIVATE__
#define __MC_SCRIPT_PRIVATE__

////////////////////////////////////////////////////////////////////////////////

typedef struct MCScriptStream *MCScriptStreamRef;

bool MCScriptReadByteFromStream(MCScriptStreamRef stream, byte_t& r_byte);
bool MCScriptReadBytesFromStream(MCScriptStreamRef stream, size_t count, byte_t* r_bytes);
bool MCScriptReadMaxInt32FromStream(MCScriptStreamRef stream, int32_t& r_int);
bool MCScriptReadMaxUInt32FromStream(MCScriptStreamRef stream, uint32_t& r_int);
bool MCScriptReadMaxInt64FromStream(MCScriptStreamRef stream, int32_t& r_int);
bool MCScriptReadMaxUInt64FromStream(MCScriptStreamRef stream, uint32_t& r_int);

////////////////////////////////////////////////////////////////////////////////

enum MCScriptObjectKind
{
    kMCScriptObjectKindNone,
    kMCScriptObjectKindError,
    kMCScriptObjectKindPackage,
    kMCScriptObjectKindModule,
    kMCScriptObjectKindInstance,
};

struct MCScriptObject
{
#ifdef _DEBUG
    uint32_t __object_marker__
#endif
    uint32_t references;
    MCScriptObjectKind kind;
};

bool MCScriptCreateObject(MCScriptObjectKind kind, size_t size, MCScriptObject*& r_object);
void MCScriptDestroyObject(MCScriptObject *object);

void MCScriptRetainObject(MCScriptObject *object);
void MCScriptReleaseObject(MCScriptObject *object);

void MCScriptReleaseObjectArray(MCScriptObject **elements, uindex_t count);

template<typename T> inline void MCScriptReleaseArray(T **elements, uindex_t count)
{
    MCScriptReleaseObjectArray((MCScriptObject **)elements, count);
}

#define __MCScriptValidateObject__(obj)
#define __MCScriptValidateObjectAndKind__(obj, kind)
#define __MCScriptAssert__(expr, label)

////////////////////////////////////////////////////////////////////////////////

struct MCScriptError: public MCScriptObject
{
    MCScriptErrorCode code;
    MCStringRef description;
};

bool MCScriptCreateError(MCScriptErrorCode code, MCStringRef description, MCScriptErrorRef& r_result);
void MCScriptDestroyError(MCScriptErrorRef error);

bool MCScriptThrowError(MCScriptErrorRef error);

bool MCScriptThrowLastError(void);

bool MCScriptThrowLastFoundationError(void);

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

enum MCScriptModuleKind
{
    kMCScriptModuleKindNone,
    kMCScriptModuleKindApplication,
    kMCScriptModuleKindLibrary,
    kMCScriptModuleKindWidget,
    
    kMCScriptModuleKind__Last,
};

struct MCScriptExportedDefinition
{
    MCNameRef name;
    uindex_t index;
};

struct MCScriptDependency
{
    MCNameRef name;
    uindex_t version;
};

enum MCScriptDefinitionKind
{
	kMCScriptDefinitionKindNone,
    kMCScriptDefinitionKindExternal,
	kMCScriptDefinitionKindType,
	kMCScriptDefinitionKindConstant,
	kMCScriptDefinitionKindVariable,
	kMCScriptDefinitionKindHandler,
	kMCScriptDefinitionKindForeignHandler,
	kMCScriptDefinitionKindProperty,
	kMCScriptDefinitionKindEvent,
    
	kMCScriptDefinitionKind__Last,
};

struct MCScriptImportedDefinition
{
    uindex_t module;
    MCScriptDefinitionKind kind;
    MCNameRef name;
    MCTypeRef type;
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
	MCTypeRef type;
};

struct MCScriptConstantDefinition: public MCScriptDefinition
{
	MCValueRef value;
};

struct MCScriptVariableDefinition: public MCScriptDefinition
{
	MCTypeRef type;
	uindex_t slot;
};

struct MCScriptHandlerDefinition: public MCScriptDefinition
{
	MCTypeRef signature;
	uindex_t address;
};

struct MCScriptForeignHandlerDefinition: public MCScriptDefinition
{
    MCTypeRef signature;
    MCStringRef binding;
};

struct MCScriptPropertyDefinition: public MCScriptDefinition
{
	MCScriptDefinition *getter;
	MCScriptDefinition *setter;
};

struct MCScriptEventDefinition: public MCScriptDefinition
{
	MCTypeRef signature;
};

struct MCScriptModule: public MCScriptObject
{
    // The owning package.
    MCScriptPackageRef package;
    
    // The type of module.
    MCScriptModuleKind kind;
    
    // The name of the module (value_pool)
    MCNameRef name;
    
    // The list of dependencies (value_pool)
    MCScriptDependency *dependencies;
    uindex_t dependency_count;
    
    // The value pool for the module - this holds references to all the constants
    // used in the module structure.
    MCValueRef *values;
    uindex_t value_count;
    
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
};

void MCScriptDestroyModule(MCScriptModuleRef module);

////////////////////////////////////////////////////////////////////////////////

struct MCScriptInstance: public MCScriptObject
{
};

void MCScriptDestroyInstance(MCScriptInstanceRef instance);

////////////////////////////////////////////////////////////////////////////////

// The bytecode represents a simple register machine. It has a stack of activation
// frames, with each frame containing an array of registers. Each register can hold
// a single boxed value.
//
// The array of registers is referenced directly in instructions with a 0-based
// index.
//
// Parameters and globals are accessed indirectly - they must be copied into a
// register before use; and copied back upon update. Both parameters and globals
// are referenced with a 0-based index.
//
// Each instruction is represented by a single byte, with arguments being encoded
// sequentially as multi-byte (signed) integers.
//

enum MCScriptBytecodeOp
{
    kMCScriptBytecodeOpNone,
	
	// Unconditional jump:
	//  X: jump <Y-X>
	// Location is encoded as relative position to jump instruction.
	kMCScriptBytecodeOpJump,
	
	// Conditional jumps:
	//  X: jump* <register>, <Y - X>
	// Location is encoded as relative position to jump instruction.
	// Register is used for test.
	kMCScriptBytecodeOpJumpIfUndefined,
	kMCScriptBytecodeOpJumpIfDefined,
	kMCScriptBytecodeOpJumpIfTrue,
	kMCScriptBytecodeOpJumpIfFalse,
	
	// Register assignment:
	//   assign <dst>, <src>
	// Dst and Src are registers. The value in dst is freed, and src copied
	// into it.
	kMCScriptBytecodeOpAssign,
	
	// Enter a handler:
	//   enter <regcount>
	// Begin a call, and <regcount> is the number of registers required for it.
	kMCScriptBytecodeEnter,
	// Leave a handler
	//   leave
	// End a call, returning control to the next instruction after the invoke.
	kMCScriptBytecodeLeave,
	
	// Direct handler invocation:
	//   invoke <index>, <arg_1>, ..., <arg_n>
	// Handler with index <index> is invoked with the given registers as arguments.
	kMCScriptBytecodeOpInvoke,
	// Indirect handler invocation:
	//   invoke *<handler>, <arg_1>, ..., <arg_n>
	// The handler reference in register <handler> is invoked with the given registers
	// as arguments.
	kMCScriptBytecodeOpInvokeIndirect,
	
	// Global fetch:
	//   fetch-global <dst>, <glob-index>
	// Assigns the current value of <glob-index> to register <dst>.
	kMCScriptBytecodeOpFetchGlobal,
	// Global store:
	//   store-global: <src>, <glob-index>
	// Assigns the current value of register <src> to <glob-index>.
	kMCScriptBytecodeOpStoreGlobal,
	
	// Parameter fetch:
	//   fetch-param <dst>, <param-index>
	// Assigns the current value of <param-index> to register <dst>.
	kMCScriptBytecodeOpFetchParameter,
	// Parameter store:
	//   store-param <src>, <param-index>
	// Assigns the current value of register <src> to <param-index>
	kMCScriptBytecodeOpStoreParameter,
};

////////////////////////////////////////////////////////////////////////////////

// Compiled modules are serialized to disk in the following format:
//
//   byte       magic[0] = 'L'
//   byte       magic[1] = 'C'
//   byte    	version
//   byte   	kind (module_kind)
//   uint        value_count
//   value      values[value_count]
//   uint        name
//   uint        dependency_count
//   uint        dependencies[dependency_count]
//   uint        imported_definition_count
//   imp_def    imported_definitions[imported_definition_count]
//   uint        exported_definition_count
//   exp_def    exported_definitions[exported_definition_count]
//   uint        definition_count
//   definition definitions[definition_count]
//   uint        bytecode_count
//   byte       bytecode[bytecode_count]
//
//  value_kind:
//   null true false zero one minus_one int real string name
//   null_type, boolean_type, integer_type, number_type, string_type, data_type,
//   array_type, enum_type, record_type, handler_type
//
//  value:
//   int       kind (value_kind)
//   if kind == int then
//     int value
//   if kind == real then
//     byte value[8] (host-byteorder IEEE764 double)
//   if kind == string or kind == name then
//     int length
//     byte value[length] (utf-8 encoded string)
//   if kind == enum_type then
//     int field_count
//     { int field_name; int field_value; } fields[field_count]
//   if kind == record_type then
//     int field_count
//     { int field_name; int field_type; int width; } fields[field_count]
//   if kind == handler_type then
//     int field_count
//     { int field_name; int field_type; int field_mode; } fields[field_count]
//
//  imp_def:
//   int        module
//   int        kind
//   int        name
//   int        type
//
//  exp_def:
//   int        name
//   int        index
//
//  definition:
//   int        kind (def_kind)
//   if kind == imported then
//     int index
//   if kind == type then
//     int type
//   if kind == constant then
//     int value
//   if kind == variable then
//     int type
//   if kind == handler then
//     int type
//     int address
//   if kind == property then
//     int getter
//     int setter (+1 index, 0 == none)
//   if kind == event then
//     int signature

enum MCScriptEncodedValueKind
{
    kMCScriptEncodedValueKindNull,
    kMCScriptEncodedValueKindTrue,
    kMCScriptEncodedValueKindFalse,
    kMCScriptEncodedValueKindZero,
    kMCScriptEncodedValueKindOne,
    kMCScriptEncodedValueKindMinusOne,
    kMCScriptEncodedValueKindInteger,
    kMCScriptEncodedValueKindReal,
    kMCScriptEncodedValueKindEmptyString,
    kMCScriptEncodedValueKindString,
    kMCScriptEncodedValueKindEmptyName,
    kMCScriptEncodedValueKindName,
    kMCScriptEncodedValueKindNullType,
    kMCScriptEncodedValueKindBooleanType,
    kMCScriptEncodedValueKindIntegerType,
    kMCScriptEncodedValueKindNumberType,
    kMCScriptEncodedValueKindStringType,
    kMCScriptEncodedValueKindDataType,
    kMCScriptEncodedValueKindArrayType,
    kMCScriptEncodedValueKindEnumType,
    kMCScriptEncodedValueKindRecordType,
    kMCScriptEncodedValueKindHandlerType,
};

////////////////////////////////////////////////////////////////////////////////

#endif

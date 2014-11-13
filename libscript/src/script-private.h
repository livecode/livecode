#ifndef __MC_SCRIPT_PRIVATE__
#define __MC_SCRIPT_PRIVATE__

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

#endif

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

struct MCScriptImportedDefinition
{
    uindex_t module;
    MCScriptDefinitionKind kind;
    MCNameRef name;
    MCTypeInfoRef type;
    
    // The resolved definition - not pickled
    MCScriptDefinition *definition;
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
	MCTypeInfoRef type;
};

struct MCScriptConstantDefinition: public MCScriptDefinition
{
	MCValueRef value;
};

struct MCScriptVariableDefinition: public MCScriptDefinition
{
	MCTypeInfoRef type;
    
    // (computed) The index of the variable in an instance's slot table - not pickled
	uindex_t slot_index;
};

struct MCScriptHandlerDefinition: public MCScriptDefinition
{
	MCTypeInfoRef signature;
	uindex_t start_address;
	uindex_t finish_address;
    
    // The number of slots required in a frame in order to execute this handler.
    // This is the sum of parameter count, local count and temporary count - not pickled
    uindex_t slot_count;
};

struct MCScriptForeignHandlerDefinition: public MCScriptDefinition
{
    MCTypeInfoRef signature;
    MCStringRef binding;
};

struct MCScriptPropertyDefinition: public MCScriptDefinition
{
	uindex_t getter;
	uindex_t setter;
};

struct MCScriptEventDefinition: public MCScriptDefinition
{
	MCTypeInfoRef signature;
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

bool MCScriptCreateModuleFromStream(MCStreamRef stream, MCScriptModuleRef& r_module);

bool MCScriptWriteRawModule(MCStreamRef stream, MCScriptModule *module);
bool MCScriptReadRawModule(MCStreamRef stream, MCScriptModule *module);
void MCScriptReleaseRawModule(MCScriptModule *module);

void MCScriptDestroyModule(MCScriptModuleRef module);

bool MCScriptLookupPropertyDefinitionInModule(MCScriptModuleRef module, MCNameRef property, MCScriptPropertyDefinition*& r_definition);
bool MCScriptLookupHandlerDefinitionInModule(MCScriptModuleRef module, MCNameRef handler, MCScriptHandlerDefinition*& r_definition);
bool MCScriptLookupDefinitionInModule(MCScriptModuleRef self, MCNameRef p_name, MCScriptDefinition*& r_definition);

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

// The bytecode represents a simple register machine. It has a stack of activation
// frames, with each frame containing an array of registers. Each register can hold
// a single boxed value.
//
// The array of registers is referenced directly in instructions with a 0-based
// index.
//
// The register file consists of the following in order:
//   result register (if returns a value)
//   parameters
//   local variables
//   temporaries
//
// Globals are accessed indirectly - they must be copied into a register before use;
// and copied back upon update.
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
	kMCScriptBytecodeOpJumpIfFalse,
	kMCScriptBytecodeOpJumpIfTrue,
	
	// Constant register assignment:
	//   assign <dst>, <index>
	// Dst is a register and index is a constant pool index. The value in dst is
    // freed, and the constant value at the specified index is assigned to it.
	kMCScriptBytecodeOpAssignConstant,
    
	// Register assignment:
	//   assign <dst>, <src>
	// Dst and Src are registers. The value in dst is freed, and src copied
	// into it.
	kMCScriptBytecodeOpAssign,
	
    // Defined check:
    //   defcheck <reg>
    // Checks that the value in <reg> is defined. Throws an error if it is not.
    kMCScriptBytecodeOpDefcheck,
    
    // Type conversion:
    //   typecheck <reg>, <typeinfo>
    // Reg is a register, and index is a definition. Throws an error if the value
    // in <reg> does not conform to the given type.
    kMCScriptBytecodeOpTypecheck,
    
	// Return control to caller:
	//   return
    // Return from a call.
	kMCScriptBytecodeOpReturn,
    
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

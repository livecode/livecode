#ifndef __MC_SCRIPT__
#define __MC_SCRIPT__

////////////////////////////////////////////////////////////////////////////////

typedef MCScriptPackageRef *MCScriptPackageRef;
typedef MCScriptModule *MCScriptModuleRef;
typedef MCScriptInstance *MCScriptInstanceRef;
typedef MCScriptError *MCScriptErrorRef;

////////////////////////////////////////////////////////////////////////////////

enum MCScriptErrorCode
{
    kMCScriptErrorCodeOutOfMemory,
};

// Copy the last error that has occurred on this thread from the script functions.
// The function returns false if there is no error.
bool MCScriptCopyLastError(MCScriptErrorRef& r_error);

// Release a script error.
void MCScriptReleaseError(MCScriptErrorRef error);

// Retain a script error.
void MCScriptRetainError(MCScriptErrorRef error);

// Get the code of a script error.
MCScriptErrorCode MCScriptGetErrorCode(MCScriptErrorRef error);

// Get a description of a script error.
MCStringRef MCScriptGetErrorDescription(MCScriptErrorRef error);

////////////////////////////////////////////////////////////////////////////////

// Packages are a collection of modules which share a common set of foreign
// code and resources.
//
// When a package is loaded into memory, all its modules are loaded also making
// them available in the module namespace and accessible through the module
// functions.
//
// A package loaded from a particular file, or with a particular name can only
// be loaded once - if an attempt is made to load a package from a file and that
// package has the same name as a loaded package it is an error. If an attempt
// is made to load a package by name and such a package is already loaded it just
// returns that package.
//
// Ensuring modules are usable is done lazily - either by an attempt to create
// a module instance, or by explicitly ensuring via the appropriate API.
//
// Packages can be unloaded from memory, however they are not completely removed
// until it is possible to do so - i.e. until there are no instances of any module
// within the package in use.
//
// Packages are a zip archive with the following structure:
//   <root>/
//     manifest.xml
//     modules/
//       <compiled module files>
//     symbols/
//       <compiled module debug info>
//     resources/
//       <shared resources>
//     code/
//       <compiled foriegn code>
//     source/
//       <original source code>
//     docs/
//       <docs tree>
//
// The manifest file describes the contents of the package along with any other
// metadata. Most of the information is inferable from the rest of the archive,
// however it is repeated in the manifest to make it easier for simple introspection.
//
// The module files are compiled bytecode for both the principal and child modules
// within the package. These files contain no debug information, instead all debug
// infomation is present in separate files in a separate folder of the archive. This
// makes it easy to strip such information out of the packages.
//
// The resources folder is the tree of file-based resources described in the package.
// It is constructed directly from the referenced files in the structure specified
// at the time of creation.
//
// The code folder contains the collection of foreign code resources required by the
// modules within the package.
//
// The source folder contains the original source for the compiled module. This is
// removed when a commercial module is built, but is required for an open-source
// module.
//
// The docs folder contains a tree of documentation resources.

// PONDERINGS:
//   Q: packages are zip files containing resources, if they are changed or 'go away'
//      behind the back of the IDE then it could cause horrendous problems. Therefore
//      should loading from file cause some kind of lock?
//   A: No - this wouldn't really help. A standalone will have everything embedded
//      so there's no problem there. The IDE will have to copy a package to a place
//      it manages and then index it. It can then load and unload packages on demand.

// Loads a package into memory from a file - this might result in multiple modules
// being loaded if the package is an umbrella. The caller must release the package
// ref when finished with it.
bool MCScriptLoadPackageFromFile(MCStringRef filename, MCScriptPackageRef& r_package);

// Loads a package into memory by name - this uses the standard search path. The
// caller must release the package ref when finished with it.
bool MCScriptLoadPackageWithName(MCNameRef name, MCScriptPackageRef& r_package);

// Marks a package so it unloads from memory as soon as it can. This occurs when
// there are no more (external) references to the package object, any of its
// modules, or any instances of those modules.
bool MCScriptUnloadPackage(MCScriptPackageRef package);

////////////////////////////////////////////////////////////////////////////////

// Lookup the module with the given name. Returns false if no such module exists.
bool MCScriptLookupModule(MCNameRef name, MCScriptModuleRef& r_module);

// Ensure all of a module's dependencies are loaded. An error is returned if
// a dependency cannot be satisfied.
bool MCScriptEnsureModuleIsUsable(MCScriptModuleRef module);

// List the module's direct dependencies.
bool MCScriptGetDependenciesOfModule(MCScriptModuleRef module, const MCNameRef*& r_dependencies, uindex_t& r_count);

// Create an instance of the given module. If the module is single-instance it
// returns that instance. Otherwise it returns a new instance. If the method
// fails, false is returned. In the case of success, the caller must release the
// instance.
bool MCScriptCreateInstanceOfModule(MCScriptModuleRef module, MCScriptInstanceRef& r_instance);

// Retain a module.
void MCScriptRetainModule(MCScriptModuleRef module);
// Release a module.
void MCScriptReleaseModule(MCScriptModuleRef module);

// Retain a instance.
void MCScriptRetainInstance(MCScriptInstanceRef instance);
// Release a instance.
void MCScriptReleaseInstance(MCScriptInstanceRef instance);

// Get a property of an instance.
bool MCScriptGetPropertyOfInstance(MCScriptInstanceRef instance, MCNameRef property, MCValueRef& r_value);
// Set a property of an instance.
bool MCScriptSetPropertyOfInstance(MCScriptInstanceRef instance, MCNameRef property, MCValueRef value);
// Call a handler of an instance.
bool MCScriptCallHandlerOfInstance(MCScriptInstanceRef instance, MCNameRef handler, MCValueRef *arguments, uindex_t argument_count, MCValueRef& r_value);

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
//   char       magic[0] = 'L'
//   char       magic[1] = 'C'
//   uint8_t	version
//   uint8_t	kind
//   int        value_count
//   value      values[value_count]
//   int        name
//   int        dependency_count
//   int        dependencies[dependency_count]
//   int        definition_count
//   definition definitions[definition_count]
//   int        symbol_count
//   symbol     symbols[symbol_count]
//   int        bytecode_count
//   uint8_t    bytecode
//   
//  value:
//
//  definition:
//

////////////////////////////////////////////////////////////////////////////////

#endif

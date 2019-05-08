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

#ifndef __MC_SCRIPT__
#define __MC_SCRIPT__

////////////////////////////////////////////////////////////////////////////////

#ifndef __MC_FOUNDATION__
#include <foundation.h>
#endif

#ifndef __MC_FOUNDATION_SYSTEM__
#include <foundation-system.h>
#endif

////////////////////////////////////////////////////////////////////////////////

struct MCScriptPackage;
struct MCScriptModule;
struct MCScriptInstance;

typedef MCScriptPackage*MCScriptPackageRef;
typedef MCScriptModule *MCScriptModuleRef;
typedef MCScriptInstance *MCScriptInstanceRef;

////////////////////////////////////////////////////////////////////////////////

typedef bool (*MCScriptForEachBuiltinModuleCallback)(void *p_context, MCScriptModuleRef p_module);

typedef bool (*MCScriptLoadLibraryCallback)(MCScriptModuleRef module, MCStringRef name, MCSLibraryRef& r_library);

typedef void *(*MCScriptWidgetEnterCallback)(MCScriptInstanceRef instance, void *host_ptr);
typedef void (*MCScriptWidgetLeaveCallback)(MCScriptInstanceRef instance, void *host_ptr, void* p_cookie);

bool MCScriptInitialize(void);
void MCScriptFinalize(void);

bool MCScriptForEachBuiltinModule(MCScriptForEachBuiltinModuleCallback p_callback, void *p_context);

void MCScriptSetLoadLibraryCallback(MCScriptLoadLibraryCallback callback);

MCSLibraryRef MCScriptGetLibrary(void);

void MCScriptSetWidgetBarrierCallbacks(MCScriptWidgetEnterCallback entry_callback, MCScriptWidgetLeaveCallback leave_callback);

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
//     module.lcm
//     support/
//       <files for the IDE / store etc.>
//     modules/
//       <compiled submodule files>
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
// The module file is the top-level compiled module for the package. Its contents is
// described by the manifest file.
//
// The manifest.xml file has the following schema:
//   <package version="1.0">
//     <label>Human Readable Foo</label>
//     <author>Mr Magoo</author>
//     <description>Foo is a super amazing widget that will do everything for you.</description>
//     <license>commercial|dual|community</license>
//     <name>com.livecode.foo</name>
//     <version>X.Y.Z</version>
//     <type>widget|library</type>
//     <requires name="com.livecode.bar" version="X.Y.Z" />
//     <requires name="com.livecode.baz" version="X.Y.Z" />
//     <property name="foo" get="optional(integer)" set="optional(integer)" />
//     <property name="bar" get="string" />
//     <event name="click" parameters="in(integer),out(real)" return="optional(any)" />
//     <handler name="magic" parameters="in(integer),inout(string)" return="undefined" />
//   </package>
// Here the 'version' field in the package tag is the version of the package manifest
// XML.
//
// There is either a widget or a library node. Widgets can have properties and events,
// libraries only handlers.
//
// The support folder is for resources required by things like the IDE and Marketplace.
// For example, icons for the tools palette and such.
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

// Load a module from a stream.
bool MCScriptCreateModuleFromStream(MCStreamRef stream, MCScriptModuleRef& r_module);

// Load a module from a blob
MC_DLLEXPORT bool MCScriptCreateModuleFromData(MCDataRef data, MCScriptModuleRef & r_module);

// Set initializer / finalizer / builtins
void MCScriptConfigureBuiltinModule(MCScriptModuleRef module, bool (*initializer)(void), void (*finalizer)(void), void **builtins);

// Lookup the module with the given name. Returns false if no such module exists.
bool MCScriptLookupModule(MCNameRef name, MCScriptModuleRef& r_module);

// Ensure that the module is valid and has resolved all its dependencies.
bool MCScriptEnsureModuleIsUsable(MCScriptModuleRef module);

// Get the name of the module.
MCNameRef MCScriptGetNameOfModule(MCScriptModuleRef module);

// Returns true if the module is a library.
bool MCScriptIsModuleALibrary(MCScriptModuleRef module);

// Returns true if the module is a widget.
bool MCScriptIsModuleAWidget(MCScriptModuleRef module);

// List the module's direct dependencies.
bool MCScriptListDependencyNamesOfModule(MCScriptModuleRef module, /* copy */ MCProperListRef& r_module_names);

// Returns a list of the constants defined by the module.
bool MCScriptListConstantNamesOfModule(MCScriptModuleRef module, /* copy */ MCProperListRef& r_constant_names);

// Queries the value of the given constant. If the constant doesn't exist, or
// module is not usable, false is returned.
bool MCScriptQueryConstantOfModule(MCScriptModuleRef module, MCNameRef name, /* get */ MCValueRef& r_constant_value);

// Returns a list of properties implemented by the module.
bool MCScriptListPropertyNamesOfModule(MCScriptModuleRef module, /* copy */ MCProperListRef& r_property_names);

// Queries the type of the given property. If the setting type is nil, then the
// property is read-only. If the property doesn't exist, or  module is not
// usable, false is returned.
bool MCScriptQueryPropertyOfModule(MCScriptModuleRef module, MCNameRef property, /* get */ MCTypeInfoRef& r_getter, /* get */ MCTypeInfoRef& r_setter);

// Returns a list of the events declared by the module.
bool MCScriptListEventNamesOfModule(MCScriptModuleRef module, /* copy */ MCProperListRef& r_event_names);

// Query the signature of the given event. If the event doesn't exist, or
// module is not usable, false is returned.
bool MCScriptQueryEventOfModule(MCScriptModuleRef module, MCNameRef event, /* get */ MCTypeInfoRef& r_signature);

// Returns a list of the handlers declared by the module.
bool MCScriptListHandlerNamesOfModule(MCScriptModuleRef module, /* copy */ MCProperListRef& r_handler_names);

// Query the signature of the given handler. If the handler doesn't exist, or
// module is not usable, false is returned.
bool MCScriptQueryHandlerSignatureOfModule(MCScriptModuleRef module, MCNameRef handler, /* get */ MCTypeInfoRef& r_signature);

// Copy the names of the parameters in the signature of the given handler.
// Note: If the module has had debugging info stripped, the list will be all
// empty names. If the handler doesn't exist, or module is not usable, false is
// returned.
bool MCScriptListHandlerParameterNamesOfModule(MCScriptModuleRef module, MCNameRef handler, /* copy */ MCProperListRef& r_names);

// Emit an interface definition for the module.
bool MCScriptWriteInterfaceOfModule(MCScriptModuleRef module, MCStreamRef stream);

// Retain a module.
MCScriptModuleRef MCScriptRetainModule(MCScriptModuleRef module);

// Release a module.
void MCScriptReleaseModule(MCScriptModuleRef module);

// Return the reference count of the given module.
uint32_t MCScriptGetRetainCountOfModule(MCScriptModuleRef module);

// Gets the module ptr for the most recent LCB stack frame on the current thread's stack.
MCScriptModuleRef MCScriptGetCurrentModule(void);

// Sets the licensed state of a module
void MCScriptSetModuleLicensed(MCScriptModuleRef self, bool p_licensed);

// Gets the licensed state of a module
bool MCScriptIsModuleLicensed(MCScriptModuleRef self);

// Attempt to load the named library using any context provided by the module
bool MCScriptLoadModuleLibrary(MCScriptModuleRef self, MCStringRef p_library, MCSLibraryRef& r_library);

////////////////////////////////////////////////////////////////////////////////

// Create an instance of the given module. If the module is single-instance it
// returns that instance. Otherwise it returns a new instance. If the method
// fails, false is returned. In the case of success, the caller must release the
// instance.
bool MCScriptCreateInstanceOfModule(MCScriptModuleRef module, MCScriptInstanceRef& r_instance);

// Retain a instance.
MCScriptInstanceRef MCScriptRetainInstance(MCScriptInstanceRef instance);

// Release a instance.
void MCScriptReleaseInstance(MCScriptInstanceRef instance);

// Sets a private pointer unused by libscript, but passed to the entry/exit
// callbacks.
void MCScriptSetInstanceHostPtr(MCScriptInstanceRef instance, void *state_ptr);

// Gets the host ptr.
void *MCScriptGetInstanceHostPtr(MCScriptInstanceRef instance);

// Get the module of an instance.
MCScriptModuleRef MCScriptGetModuleOfInstance(MCScriptInstanceRef instance);

// Get a property of an instance.
bool MCScriptGetPropertyInInstance(MCScriptInstanceRef instance, MCNameRef property, MCValueRef& r_value);
// Set a property of an instance.
bool MCScriptSetPropertyInInstance(MCScriptInstanceRef instance, MCNameRef property, MCValueRef value);

// Call a handler of an instance.
bool MCScriptCallHandlerInInstance(MCScriptInstanceRef instance, MCNameRef handler, MCValueRef *arguments, uindex_t argument_count, MCValueRef& r_value);

// Call a handler of an instance if found, it doesn't throw an error if not.
bool MCScriptCallHandlerInInstanceIfFound(MCScriptInstanceRef instance, MCNameRef handler, MCValueRef *arguments, uindex_t argument_count, MCValueRef& r_value);

// Create a handler-ref for the given handler in the instance.
bool MCScriptEvaluateHandlerBindingInInstance(MCScriptInstanceRef instance, MCNameRef handler, /* copy */ MCHandlerRef& r_handler);

////////////////////////////////////////////////////////////////////////////////

enum MCScriptBytecodeParameterType
{
    // Invalid index was passed to describe
    kMCScriptBytecodeParameterTypeUnknown,
    // The parameter should be a label index
    kMCScriptBytecodeParameterTypeLabel,
    // The parameter should be a register
    kMCScriptBytecodeParameterTypeRegister,
    // The parameter should be a constant pool index
    kMCScriptBytecodeParameterTypeConstant,
    // The parameter should be a fetchable definition (variable, constant, handler)
    kMCScriptBytecodeParameterTypeDefinition,
    // The parameter should be a variable definition
    kMCScriptBytecodeParameterTypeVariable,
    // The parameter should be a handler definition
    kMCScriptBytecodeParameterTypeHandler,
};

bool MCScriptCopyBytecodeNames(MCProperListRef& r_proper_list);
bool MCScriptLookupBytecode(const char *opname, uindex_t& r_opcode);
const char *MCScriptDescribeBytecode(uindex_t opcode);
MCScriptBytecodeParameterType MCScriptDescribeBytecodeParameter(uindex_t opcode, uindex_t index);
bool MCScriptCheckBytecodeParameterCount(uindex_t opcode, uindex_t proposed_count);

////////////////////////////////////////////////////////////////////////////////

typedef struct MCScriptModuleBuilder *MCScriptModuleBuilderRef;

enum MCScriptModuleKind
{
    kMCScriptModuleKindNone,
    kMCScriptModuleKindApplication,
    kMCScriptModuleKindLibrary,
    kMCScriptModuleKindWidget,
    
    kMCScriptModuleKind__Last,
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
    kMCScriptDefinitionKindSyntax,
    kMCScriptDefinitionKindDefinitionGroup,
    
	kMCScriptDefinitionKind__Last,
};

enum MCScriptHandlerTypeParameterMode
{
    kMCScriptHandlerTypeParameterModeIn,
    kMCScriptHandlerTypeParameterModeOut,
    kMCScriptHandlerTypeParameterModeInOut,
    kMCScriptHandlerTypeParameterModeVariadic,
    
    kMCScriptHandlerTypeParameterMode__Last
};

enum MCScriptHandlerAttributes
{
    kMCScriptHandlerAttributeSafe = 0 << 0,
    kMCScriptHandlerAttributeUnsafe = 1 << 0,
};
    
void MCScriptBeginModule(MCScriptModuleKind kind, MCNameRef name, MCScriptModuleBuilderRef& r_builder);
bool MCScriptEndModule(MCScriptModuleBuilderRef builder, MCStreamRef stream);

void MCScriptAddDependencyToModule(MCScriptModuleBuilderRef builder, MCNameRef dependency, uindex_t& r_index);

void MCScriptAddExportToModule(MCScriptModuleBuilderRef builder, uindex_t index);
void MCScriptAddImportToModule(MCScriptModuleBuilderRef builder, uindex_t module_index, MCNameRef definition, MCScriptDefinitionKind kind, uindex_t type, uindex_t& r_index);
void MCScriptAddImportToModuleWithIndex(MCScriptModuleBuilderRef builder, uindex_t module_index, MCNameRef definition, MCScriptDefinitionKind kind, uindex_t type, uindex_t p_index);

void MCScriptAddValueToModule(MCScriptModuleBuilderRef builder, MCValueRef value, uindex_t& r_index);
void MCScriptBeginListValueInModule(MCScriptModuleBuilderRef builder);
void MCScriptContinueListValueInModule(MCScriptModuleBuilderRef builder, uindex_t index);
void MCScriptEndListValueInModule(MCScriptModuleBuilderRef builder, uindex_t& r_index);
void MCScriptBeginArrayValueInModule(MCScriptModuleBuilderRef builder);
void MCScriptContinueArrayValueInModule(MCScriptModuleBuilderRef builder, uindex_t key_index, uindex_t value_index);
void MCScriptEndArrayValueInModule(MCScriptModuleBuilderRef builder, uindex_t& r_index);

void MCScriptAddDefinedTypeToModule(MCScriptModuleBuilderRef builder, uindex_t index, uindex_t& r_type);
void MCScriptAddForeignTypeToModule(MCScriptModuleBuilderRef builder, MCStringRef p_binding, uindex_t& r_type);
void MCScriptAddOptionalTypeToModule(MCScriptModuleBuilderRef builder, uindex_t type, uindex_t& r_new_type);
void MCScriptBeginHandlerTypeInModule(MCScriptModuleBuilderRef builder, uindex_t return_type);
void MCScriptBeginForeignHandlerTypeInModule(MCScriptModuleBuilderRef builder, uindex_t return_type);
void MCScriptContinueHandlerTypeInModule(MCScriptModuleBuilderRef builder, MCScriptHandlerTypeParameterMode mode, MCNameRef name, uindex_t type);
void MCScriptEndHandlerTypeInModule(MCScriptModuleBuilderRef builder, uindex_t& r_new_type);
void MCScriptBeginRecordTypeInModule(MCScriptModuleBuilderRef builder);
void MCScriptContinueRecordTypeInModule(MCScriptModuleBuilderRef builder, MCNameRef name, uindex_t type);
void MCScriptEndRecordTypeInModule(MCScriptModuleBuilderRef builder, uindex_t& r_new_type);

void MCScriptAddDefinitionToModule(MCScriptModuleBuilderRef builder, MCScriptDefinitionKind kind, uindex_t& r_index);

void MCScriptAddTypeToModule(MCScriptModuleBuilderRef builder, MCNameRef name, uindex_t type, uindex_t index);
void MCScriptAddConstantToModule(MCScriptModuleBuilderRef builder, MCNameRef name, uindex_t const_idx, uindex_t index);
void MCScriptAddVariableToModule(MCScriptModuleBuilderRef builder, MCNameRef name, uindex_t type, uindex_t index);

void MCScriptBeginHandlerInModule(MCScriptModuleBuilderRef builder, MCNameRef name, uindex_t signature, MCScriptHandlerAttributes attributes, uindex_t index);
void MCScriptAddParameterToHandlerInModule(MCScriptModuleBuilderRef builder, MCNameRef name, uindex_t type, uindex_t& r_index);
void MCScriptAddVariableToHandlerInModule(MCScriptModuleBuilderRef builder, MCNameRef name, uindex_t type, uindex_t& r_index);
void MCScriptEndHandlerInModule(MCScriptModuleBuilderRef builder);

void MCScriptBeginSyntaxInModule(MCScriptModuleBuilderRef builder, MCNameRef name, uindex_t index);
void MCScriptBeginSyntaxMethodInModule(MCScriptModuleBuilderRef builder, uindex_t handler);
void MCScriptAddBuiltinArgumentToSyntaxMethodInModule(MCScriptModuleBuilderRef builder, uindex_t index);
void MCScriptAddConstantArgumentToSyntaxMethodInModule(MCScriptModuleBuilderRef builder, MCValueRef value);
void MCScriptAddVariableArgumentToSyntaxMethodInModule(MCScriptModuleBuilderRef builder, uindex_t index);
void MCScriptAddIndexedVariableArgumentToSyntaxMethodInModule(MCScriptModuleBuilderRef builder, uindex_t var_index, uindex_t element_index);
void MCScriptEndSyntaxMethodInModule(MCScriptModuleBuilderRef builder);
void MCScriptEndSyntaxInModule(MCScriptModuleBuilderRef builder);

void MCScriptBeginDefinitionGroupInModule(MCScriptModuleBuilderRef builder);
void MCScriptAddHandlerToDefinitionGroupInModule(MCScriptModuleBuilderRef builder, uindex_t index);
void MCScriptEndDefinitionGroupInModule(MCScriptModuleBuilderRef builder, uindex_t& r_index);

void MCScriptAddForeignHandlerToModule(MCScriptModuleBuilderRef builder, MCNameRef name, uindex_t signature, MCStringRef binding, uindex_t index);

void MCScriptAddPropertyToModule(MCScriptModuleBuilderRef builder, MCNameRef name, uindex_t getter, uindex_t setter, uindex_t index);

void MCScriptAddEventToModule(MCScriptModuleBuilderRef builder, MCNameRef name, uindex_t signature, uindex_t index);

void MCScriptDeferLabelForBytecodeInModule(MCScriptModuleBuilderRef builder, uindex_t& r_label);
void MCScriptResolveLabelForBytecodeInModule(MCScriptModuleBuilderRef builder, uindex_t label);

void MCScriptEmitBytecodeInModule(MCScriptModuleBuilderRef builder, uindex_t opcode, ...);
void MCScriptEmitBytecodeInModuleV(MCScriptModuleBuilderRef builder, uindex_t opcode, va_list args);
void MCScriptEmitBytecodeInModuleA(MCScriptModuleBuilderRef builder, uindex_t opcode, uindex_t *arguments, uindex_t argument_count);

void MCScriptEmitPositionForBytecodeInModule(MCScriptModuleBuilderRef builder, MCNameRef file, uindex_t line);

/* These methods are used to build shims for builtin foreign handlers. They
   allow lc-compiles 'emit' module to get the details of a handler type. The
   types returned are the raw types of the arguments after taking into account
   mode which means we only (currently) need void, pointer, bool and the number
   types (both fixed and C). We need to enumerate them all separately, as some
   int types change depending on the architecture. */

enum MCScriptForeignPrimitiveType
{
    kMCScriptForeignPrimitiveTypeUnknown,
    kMCScriptForeignPrimitiveTypeVoid,
    kMCScriptForeignPrimitiveTypePointer,
    kMCScriptForeignPrimitiveTypeSInt8,
    kMCScriptForeignPrimitiveTypeUInt8,
    kMCScriptForeignPrimitiveTypeSInt16,
    kMCScriptForeignPrimitiveTypeUInt16,
    kMCScriptForeignPrimitiveTypeSInt32,
    kMCScriptForeignPrimitiveTypeUInt32,
    kMCScriptForeignPrimitiveTypeSInt64,
    kMCScriptForeignPrimitiveTypeUInt64,
    kMCScriptForeignPrimitiveTypeSIntSize,
    kMCScriptForeignPrimitiveTypeUIntSize,
    kMCScriptForeignPrimitiveTypeSIntPtr,
    kMCScriptForeignPrimitiveTypeUIntPtr,
    kMCScriptForeignPrimitiveTypeFloat32,
    kMCScriptForeignPrimitiveTypeFloat64,
    kMCScriptForeignPrimitiveTypeCBool,
    kMCScriptForeignPrimitiveTypeCChar,
    kMCScriptForeignPrimitiveTypeCSChar,
    kMCScriptForeignPrimitiveTypeCUChar,
    kMCScriptForeignPrimitiveTypeCSShort,
    kMCScriptForeignPrimitiveTypeCUShort,
    kMCScriptForeignPrimitiveTypeCSInt,
    kMCScriptForeignPrimitiveTypeCUInt,
    kMCScriptForeignPrimitiveTypeCSLong,
    kMCScriptForeignPrimitiveTypeCULong,
    kMCScriptForeignPrimitiveTypeCSLongLong,
    kMCScriptForeignPrimitiveTypeCULongLong,
    kMCScriptForeignPrimitiveTypeCFloat,
    kMCScriptForeignPrimitiveTypeCDouble,
    kMCScriptForeignPrimitiveTypeSInt,
    kMCScriptForeignPrimitiveTypeUInt,
    kMCScriptForeignPrimitiveTypeNaturalUInt,
    kMCScriptForeignPrimitiveTypeNaturalSInt,
    kMCScriptForeignPrimitiveTypeNaturalFloat,
};

MCScriptForeignPrimitiveType MCScriptQueryForeignHandlerReturnTypeInModule(MCScriptModuleBuilderRef build, uindex_t type_index);
uindex_t MCScriptQueryForeignHandlerParameterCountInModule(MCScriptModuleBuilderRef build, uindex_t type_index);
MCScriptForeignPrimitiveType MCScriptQueryForeignHandlerParameterTypeInModule(MCScriptModuleBuilderRef build, uindex_t type_index, uindex_t arg_index);

////////////////////////////////////////////////////////////////////////////////

enum MCJavaCallType {
    MCJavaCallTypeInstance,
    MCJavaCallTypeStatic,
    MCJavaCallTypeNonVirtual,
    MCJavaCallTypeConstructor,
    MCJavaCallTypeInterfaceProxy,
    MCJavaCallTypeGetter,
    MCJavaCallTypeSetter,
    MCJavaCallTypeStaticGetter,
    MCJavaCallTypeStaticSetter,
    
    /* This value is used to indicate that the call type was not known - it is
     * only used internally in libscript. */
    MCJavaCallTypeUnknown = -1,
};

/* MCScriptForeignHandlerLanguage describes the type of foreign handler which
 * has been bound - based on language. */
enum MCScriptForeignHandlerLanguage
{
    /* The handler has not yet been bound, or failed to bind */
    kMCScriptForeignHandlerLanguageUnknown,
    
    /* The handler should be called using libffi */
    kMCScriptForeignHandlerLanguageC,
    
    /* The handler has a lc-compile generated shim, so can be called directly */
    kMCScriptForeignHandlerLanguageBuiltinC,
    
    /* The handler should be called using objc_msgSend */
    kMCScriptForeignHandlerLanguageObjC,
    
    /* The handler should be called using the JNI */
    kMCScriptForeignHandlerLanguageJava,
};

/* MCScriptThreadAffinity describes which thread a foreign handler should be
 * executed on. This applies to Android and iOS, where a handler can either be
 * run on the default (engine) thread, or the UI (main) thread. */
enum MCScriptThreadAffinity
{
    kMCScriptThreadAffinityDefault,
    kMCScriptThreadAffinityUI,
};

/* MCScriptForeignHandlerObjcCallType describes how to call the objective-c
 * method. */
enum MCScriptForeignHandlerObjcCallType
{
    /* Call the method using method_invoke on the instance (on the default
     * thread) */
    kMCScriptForeignHandlerObjcCallTypeInstanceMethod,
    
    /* Call the method using method_invoke on the class instance (on the default
     * thread) */
    kMCScriptForeignHandlerObjcCallTypeClassMethod,
};

struct MCScriptForeignHandlerInfo
{
    MCScriptForeignHandlerLanguage language : 8;
    MCScriptThreadAffinity thread_affinity : 8;
    union
    {
        struct
        {
            int call_type;
            MCStringRef library;
            MCStringRef function;
        } c;
        struct
        {
            MCScriptForeignHandlerObjcCallType call_type : 8;
            MCStringRef library;
            MCStringRef class_name;
            MCStringRef method_name;
        } objc;
        struct
        {
            MCJavaCallType call_type : 8;
            MCStringRef class_name;
            MCStringRef method_name;
            MCStringRef arguments;
            MCStringRef return_type;
        } java;
    };
};

typedef struct MCScriptForeignHandlerInfo *MCScriptForeignHandlerInfoRef;

bool MCScriptForeignHandlerInfoParse(MCStringRef p_binding, MCScriptForeignHandlerInfoRef& r_info);
void MCScriptForeignHandlerInfoRelease(MCScriptForeignHandlerInfoRef p_info);

////////////////////////////////////////////////////////////////////////////////
// Compiled modules are serialized to disk in the following format:
//
//   byte       magic[0] = 'L'
//   byte       magic[1] = 'C'
//   byte        version[0]
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
#define kMCScriptModuleVersion_8_1_0_DP_2 1
#define kMCScriptModuleVersion_9_0_0_DP_4 2
#define kMCScriptCurrentModuleVersion kMCScriptModuleVersion_9_0_0_DP_4

////////////////////////////////////////////////////////////////////////////////

#endif

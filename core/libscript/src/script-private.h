#ifndef __MC_SCRIPT_PRIVATE__
#define __MC_SCRIPT_PRIVATE__

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
    uint32_t references;
    MCScriptObjectKind kind;
};

struct MCScriptError: public MCScriptObject
{
    MCScriptErrorCode code;
    MCStringRef description;
};

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

enum MCScriptModuleKind
{
    kMCScriptModuleKindNone,
    kMCScriptModuleKindApplication,
    kMCScriptModuleKindLibrary,
    kMCScriptModuleKindWidget,
};

struct MCScriptSymbol
{
    MCNameRef name;
    uindex_t index;
};

enum MCScriptDefinitionKind
{
};

struct MCScriptDefinition
{
    MCScriptDefinitionKind kind;
};

struct MCScriptModule: public MCScriptModule
{
    // The owning package.
    MCScriptPackageRef package;
    
    // The type of module.
    MCScriptModuleKind kind;
    
    // The name of the module.
    MCNameRef name;
    
    // The list of dependencies.
    MCNameRef *dependencies;
    uindex_t dependency_count;
    
    // The value pool for the module - this holds references to all the constants
    // used in the module structure.
    MCValueRef *values;
    uindex_t value_count;
    
    // The symbol list. This is a list of all public definitions mapping name to
    // index.
    MCScriptSymbol *symbols;
    uindex_t symbol_count;
    
    // The definition list. This is a table of all definitions in the module that
    // are referenced whether through export, or internally.
    MCScriptDefinition *definitions;
    uindex_t definition_count;
    
    // The bytecode used by the module.
    uint8_t *bytecode;
    uindex_t bytecode_count;
    
    // The number of slots needed by an instance of the module.
    uindex_t slot_count;
};

#endif

/* Copyright (C) 2016 LiveCode Ltd.
 
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

'module' output

'use'
    types
    support

'export'
    OutputPackages

--------------------------------------------------------------------------------

'var' OutputPackageIndex : INT

'action' OutputPackages(PACKAGELIST)

    'rule' OutputPackages(List):
        OutputBegin()
        OutputPackageIndex <- 1
        OutputForEachPackage(List)
        OutputEnd()
    
'action' OutputForEachPackage(PACKAGELIST)

    'rule' OutputForEachPackage(packagelist(Head, Rest)):
        OutputSinglePackage(Head)
        OutputPackageIndex -> CurrentIndex
        OutputPackageIndex <- CurrentIndex + 1
        OutputForEachPackage(Rest)
        
    'rule' OutputForEachPackage(nil):
        -- do nothing

'action' OutputSinglePackage(PACKAGE)

    'rule' OutputSinglePackage(Package:package(_, Id, Definitions)):
        ResolveIdName(Id -> Name)
        OutputWriteI("foreign package ", Name, "\n")

        OutputImportedDefinitions(Definitions)
        OutputDefinitions(Definitions)
        OutputWrite("end package\n\n")

----------

'action' OutputImportedDefinitions(DEFINITION)

    'rule' OutputImportedDefinitions(nil):
        -- done

    'rule' OutputImportedDefinitions(sequence(Head, Tail)):
        OutputImportedDefinition(Head)
        OutputImportedDefinitions(Tail)

'action' OutputImportedDefinition(DEFINITION)

    'rule' OutputImportedDefinition(use(_, Id))
        QueryId(Id -> Meaning)
        QuerySymbolId(Id -> Info)
        Info'Type -> Type

        -- Get the module in which the definition is defined
        QueryPackageOfId(Id -> PackageId)

        -- Ensure we have a dependency for the module
        ResolveIdName(PackageId -> Name)

        -- Fetch the info about the symbol.
        QuerySymbolId(Id -> SymbolInfo)
        SymbolInfo'Kind -> SymbolKind
        SymbolInfo'Type -> SymbolType

        ResolveIdName(Id -> SymbolName)
        OutputWriteI("use ", Name, ".")
        OutputWriteI("", SymbolName, "")
        (|
            where(SymbolKind -> class)
            OutputWrite(" -- class\n")
        ||
            where(SymbolKind -> interface)
            OutputWrite(" -- interface\n")
        |)

        OutputPackageIndex -> Generator
        SymbolInfo'Generator <- Generator
        
    'rule' OutputImportedDefinition(Id):
        -- If we get here then either the id isn't imported, or we have previously
        -- generated it.

----

'action' OutputDefinitions(DEFINITION)

    'rule' OutputDefinitions(sequence(Left, Right)):
        OutputDefinitions(Left)
        OutputDefinitions(Right)

    'rule' OutputDefinitions(constant(_, Id, Type, Value)):
        QuerySymbolId(Id -> SymbolInfo)
        ResolveIdName(Id -> SymbolName)
        OutputWriteI("   constant ", SymbolName, "")
        OutputWrite(" as ")
        OutputType(Type)
        (|
            where(Value -> nil)
        ||
            OutputWrite(" is ")
            OutputValue(Value)
        |)
        OutputWrite("\n")

    'rule' OutputDefinitions(variable(_, Modifiers, Id, Type)):
        QuerySymbolId(Id -> SymbolInfo)
        ResolveIdName(Id -> SymbolName)
        OutputWrite("   ")
        OutputModifiers(Modifiers)
        OutputWriteI("variable ", SymbolName, "")
        OutputWrite(" as ")
        OutputType(Type)
        OutputWrite("\n")

    'rule' OutputDefinitions(method(_, Modifiers, Id, signature(Parameters, Returns), Alias, Throws)):
        (|
            where(Alias -> id(AliasName))
            QuerySymbolId(AliasName -> SymbolInfo)
        ||
            QuerySymbolId(Id -> SymbolInfo)
        |)
        ResolveIdName(Id -> Name)
        OutputWrite("   ")
        OutputModifiers(Modifiers)
        OutputWriteI("method ", Name, "")
        OutputSignature(Parameters)
        OutputOptionalAlias(Alias)
        OutputOptionalThrows(Throws)
        OutputReturns(Returns)

        OutputWrite("\n")

    'rule' OutputDefinitions(constructor(_, Modifiers, Id, signature(Params, _), Alias)):
        (|
            where(Alias -> id(AliasName))
            QuerySymbolId(AliasName -> SymbolInfo)
        ||
            QuerySymbolId(Id -> SymbolInfo)
        |)
        ResolveIdName(Id -> Name)
        OutputWrite("   ")
        OutputModifiers(Modifiers)
        OutputWriteI("constructor ", Name, "")
        OutputSignature(Params)
        OutputOptionalAlias(Alias)

        OutputWrite("\n")
        
    'rule' OutputDefinitions(class(_, Modifiers, Type, Definitions, Inherits, Implements)):
        OutputWrite("class ")
        OutputType(Type)
        OutputInherits(Inherits)
        OutputImplements(Implements)
        OutputWrite("\n")
        OutputDefinitions(Definitions)
        OutputWrite("end class\n\n")
        
    'rule' OutputDefinitions(interface(_, Type, Definitions, Inherits)):
        OutputWrite("interface ")
        OutputType(Type)
        OutputInherits(Inherits)
        OutputWrite("\n")
        OutputDefinitions(Definitions)
        OutputWrite("end interface\n\n")

    'rule' OutputDefinitions(use(_,_)):
        -- do nothing

    'rule' OutputDefinitions(nil):
        -- do nothing

'action' OutputInherits(TYPELIST)

    'rule' OutputInherits(nil):

    'rule' OutputInherits(Typelist)
        OutputWrite(" inherits ")
        OutputTypelist(Typelist)

'action' OutputImplements(TYPELIST)

    'rule' OutputImplements(nil):

    'rule' OutputImplements(Typelist)
        OutputWrite(" implements ")
        OutputTypelist(Typelist)

'action' OutputTypelist(TYPELIST)

    'rule' OutputTypelist(typelist(Head, Tail)):
        OutputType(Head)
        (|
            where(Tail -> nil)
        ||
            OutputWrite(", ")
            OutputTypelist(Tail)
        |)

'action' OutputOptionalAlias(OPTIONALID)

    'rule' OutputOptionalAlias(nil):
        -- do nothing

    'rule' OutputOptionalAlias(id(Id)):
        ResolveIdName(Id -> AliasName)
        OutputWriteI(" named ", AliasName, "")

'action' OutputOptionalThrows(OPTIONALID)

    'rule' OutputOptionalThrows(nil):
        -- do nothing

    'rule' OutputOptionalThrows(id(Id)):
        ResolveIdName(Id -> ThrowsName)
        OutputWriteI(" throws ", ThrowsName, "")

'action' OutputReturns(TYPE)

    'rule' OutputReturns(Type):
        OutputWrite(" returns ")
        OutputType(Type)

'action' OutputType(TYPE)

    'rule' OutputType(byte):
        OutputWrite("byte")

    'rule' OutputType(short):
        OutputWrite("short")

    'rule' OutputType(int):
        OutputWrite("int")

    'rule' OutputType(long):
        OutputWrite("long")

    'rule' OutputType(float):
        OutputWrite("float")

    'rule' OutputType(double):
        OutputWrite("double")

    'rule' OutputType(boolean):
        OutputWrite("boolean")

    'rule' OutputType(char):
        OutputWrite("char")

    'rule' OutputType(string):
        OutputWrite("String")
        
    'rule' OutputType(nil):
        OutputWrite("nothing")

    'rule' OutputType(named(_, Id, Parameters)):
        ResolveIdName(Id -> SymbolName)
        OutputWriteI("", SymbolName, "")
        (|
            where(Parameters -> nil)
        ||
            OutputWrite("<")
            OutputTypeList(Parameters)
            OutputWrite(">")
        |)

    'rule' OutputType(template(_, Id, Parameters)):
        ResolveIdName(Id -> SymbolName)
        OutputWriteI("", SymbolName, "")
        (|
            where(Parameters -> nil)
        ||
            OutputWrite("<")
            OutputTypeList(Parameters)
            OutputWrite(">")
        |)

    'rule' OutputType(placeholder(_, Id)):
        QuerySymbolId(Id -> SymbolInfo)
        ResolveIdName(Id -> SymbolName)
        OutputWriteI("", SymbolName, "")

    -- output java array
    'rule' OutputType(jarray(_, Type, Dimension)):
        OutputJArray(Type, Dimension)

    'rule' OutputType(wildcard(_, Bounds)):
        OutputWrite("?")
        OutputBounds(Bounds)

'action' OutputBounds(BOUNDS)

    'rule' OutputBounds(upper(Type)):
        OutputWrite(" extends")
        OutputType(Type)

    'rule' OutputBounds(lower(Type)):
        OutputWrite(" super")
        OutputType(Type)

    'rule' OutputBounds(unbounded):

'action' OutputTypeList(TYPELIST)

    'rule' OutputTypeList(typelist(Head, Tail)):
        OutputType(Head)
        (|
            where(Tail -> nil)
        ||
            OutputWrite(", ")
            OutputTypeList(Tail)
        |)

'action' OutputJArray(TYPE, INT)

    'rule' OutputJArray(Type, Dim):
        (|
            ne(Dim, 1)
            OutputJArray(Type, Dim - 1)
        ||
            OutputType(Type)
        |)
        OutputWrite("[]")

'action' OutputValue(EXPRESSION)

    'rule' OutputValue(true(_)):
        OutputWrite("true")

    'rule' OutputValue(false(_)):
        OutputWrite("false")

    'rule' OutputValue(real(_, Value)):
        OutputWriteD("", Value, "")

    'rule' OutputValue(integer(_, Value)):
        OutputWriteN("", Value, "")

    'rule' OutputValue(string(_, Value)):
        OutputWriteS("\"", Value, "\"")


'action' OutputModifiers(MODIFIER)

    'rule' OutputModifiers(classmodifiers(Access, StrictFP, Inherit, Modify, Instance)):
        OutputModifiers(Access)
        OutputModifiers(StrictFP)
        OutputModifiers(Inherit)
        OutputModifiers(Modify)
        OutputModifiers(Instance)

    'rule' OutputModifiers(interfacemethodmodifiers(Modifier)):
        OutputModifiers(Modifier)

    'rule' OutputModifiers(variablemodifiers(Access, Transient, Modify, Instance)):
        OutputModifiers(Access)
        OutputModifiers(Transient)
        OutputModifiers(Modify)
        OutputModifiers(Instance)

    'rule' OutputModifiers(methodmodifiers(Access, Sync, Native, StrictFP, Inherit, Instance)):
        OutputModifiers(Access)
        OutputModifiers(Sync)
        OutputModifiers(Native)
        OutputModifiers(StrictFP)
        OutputModifiers(Inherit)
        OutputModifiers(Instance)

    'rule' OutputModifiers(constructormodifiers(Access)):
        OutputModifiers(Access)

    'rule' OutputModifiers(protected):
        OutputWrite("protected ")

    'rule' OutputModifiers(synchronized):
        OutputWrite("synchronized ")

    'rule' OutputModifiers(native):
        OutputWrite("native ")

    'rule' OutputModifiers(strictfp):
        OutputWrite("strictfp ")

    'rule' OutputModifiers(abstract):
        OutputWrite("abstract ")

    'rule' OutputModifiers(final):
        OutputWrite("final ")

    'rule' OutputModifiers(class):
        OutputWrite("class ")

    'rule' OutputModifiers(volatile):
        OutputWrite("volatile ")

    'rule' OutputModifiers(transient):
        OutputWrite("transient ")

    'rule' OutputModifiers(default):
        OutputWrite("default ")

    'rule' OutputModifiers(public):

    'rule' OutputModifiers(inferred):

'action' OutputSignature(PARAMETERLIST)

    'rule' OutputSignature(Params):
        OutputWrite("(")
        OutputParams(Params)
        OutputWrite(")")

'action' OutputParams(PARAMETERLIST)

    'rule' OutputParams(nil):

    'rule' OutputParams(parameterlist(Head, nil)):
        OutputParam(Head)

    'rule' OutputParams(parameterlist(Head, Tail)):
        OutputParam(Head)
        OutputWrite(", ")
        OutputParams(Tail)

'action' OutputParam(PARAMETER)

    'rule' OutputParam(parameter(_, Id, Type)):
        QuerySymbolId(Id -> SymbolInfo)
        ResolveIdName(Id -> SymbolName)
        OutputWriteI("", SymbolName, "")
        OutputWrite(" as ")
        OutputType(Type)

    'rule' OutputParam(variadic(_)):
        OutputWrite("...")

--------------------------------------------------------------------------------

-- Defined in check.g
'action' QueryId(ID -> MEANING)
'action' QueryPackageOfId(ID -> ID)

'condition' QuerySymbolId(ID -> SYMBOLINFO)
'condition' QueryPackageId(ID -> PACKAGEINFO)

-- Defined in bind.g
'action' ResolveIdName(ID -> NAME)
--------------------------------------------------------------------------------

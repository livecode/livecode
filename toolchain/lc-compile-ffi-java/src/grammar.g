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

'module' grammar

'use'
    bind
    check
    generate
    output
    types
    support

--------------------------------------------------------------------------------

'root'
	Parse(-> PackageList)
    (|
        ErrorsDidOccur()
    ||
        Compile(PackageList)
    |)

'nonterm' Parse(-> PACKAGELIST)

    'rule' Parse(-> Packages):
        PackageList(-> Packages)

'action' Compile(PACKAGELIST)

    'rule' Compile(Packages):
        BindPackages(Packages, Packages)
        (|
            ErrorsDidOccur()
        ||
            CheckPackages(Packages)
            (|
                ErrorsDidOccur()
            ||
                OutputPackages(Packages)
                GeneratePackages(Packages)
            |)
        |)

'action' BindPackages(PACKAGELIST, PACKAGELIST)

    'rule' BindPackages(packagelist(Head, Tail), Imports):
        Bind(Head, Imports)
        BindPackages(Tail, Imports)
        
    'rule' BindPackages(nil, _):
        -- empty

'action' CheckPackages(PACKAGELIST)

    'rule' CheckPackages(packagelist(Head, Tail)):
        Check(Head)
        CheckPackages(Tail)
        
    'rule' CheckPackages(nil):
        -- empty

--------------------------------------------------------------------------------
-- Package
--------------------------------------------------------------------------------

'nonterm' PackageList(-> PACKAGELIST)

    'rule' PackageList(-> packagelist(Head, Tail)):
        Package(-> Head) Separator
        PackageList(-> Tail)

    'rule' PackageList(-> packagelist(Head, Tail)):
        Package(-> Head)
        NEXT_UNIT
        PackageList(-> Tail)
        
    'rule' PackageList(-> packagelist(Head, nil)):
        Package(-> Head)

'nonterm' Package(-> PACKAGE)

	'rule' Package(-> package(Position, Name, Definitions)):
        OptionalSeparator
		"foreign" "package" @(-> Position) PackageId(-> Name) Separator
		Definitions(-> Definitions)
		"end" "package" OptionalSeparator
        END_OF_UNIT

	'rule' Package(-> package(Position, Name, Definitions)):
        OptionalSeparator
		"foreign" "package" @(-> Position) PackageId(-> Name) Separator
		Definitions(-> Definitions)
		"end" "package" OptionalSeparator
		
'nonterm' Definitions(-> DEFINITION)

	'rule' Definitions(-> sequence(Head, Tail)):
		Definition(-> Head) Separator 
		Definitions(-> Tail)
	
	'rule' Definitions(-> nil):
		-- finished

-- The three types of definition in a package are
-- use-clauses, class definitions and interface definitions
'nonterm' Definition(-> DEFINITION)
	
    'rule' Definition(-> Use):
        Use (-> Use)

    'rule' Definition(-> Class):
        Class(-> Class)
        
     'rule' Definition(-> Interface):
        Interface(-> Interface)

--------------------------------------------------------------------------------
-- Use
--------------------------------------------------------------------------------

'nonterm' Use(-> DEFINITION)

    'rule' Use(-> use(Position, QualifiedId)):
        OptionalSeparator
		"use" @(-> Position) QualifiedId(-> QualifiedId)
        OptionalSeparator

--------------------------------------------------------------------------------
-- Class and Interface
--------------------------------------------------------------------------------

'nonterm' Class(-> DEFINITION)
	
	'rule' Class(-> class(Position, Modifiers, Type, Definitions, Inherits, Implements)):
		OptionalSeparator
		ClassModifiers(-> Modifiers) "class" @(-> Position) TemplateType(-> Type) InheritsClause(-> Inherits) ImplementsClause(-> Implements) Separator
		ClassDefs(-> Definitions)
		"end" "class" OptionalSeparator

'nonterm' Interface(-> DEFINITION)

	'rule' Interface(-> interface(Position, Type, Definitions, Inherits)):
		OptionalSeparator
		"interface" @(-> Position) TemplateType(-> Type) InterfaceInheritsClause(-> Inherits) Separator
		InterfaceDefs(-> Definitions)
		"end" "interface" OptionalSeparator

--------------------------------------------------------------------------------
-- Interface-specific rules
--------------------------------------------------------------------------------

-- Interfaces can inherit from multiple interfaces
'nonterm' InterfaceInheritsClause(-> TYPELIST)
	
	'rule' InterfaceInheritsClause(-> InterfaceList):
		"inherits" InterfaceList(-> InterfaceList)

	'rule' InterfaceInheritsClause(-> nil)
		-- no inherits clause

'nonterm' InterfaceDefs(-> DEFINITION)

    'rule' InterfaceDefs(-> sequence(Left, Right)):
        InterfaceDef(-> Left) Separator
        InterfaceDefs(-> Right)
        
    'rule' InterfaceDefs(-> nil):
        -- no more definitions

'nonterm' InterfaceDef(-> DEFINITION)
	
	'rule' InterfaceDef(-> InterfaceDef):
		InterfaceMethodDef(-> InterfaceDef)
	
	'rule' InterfaceDef(->InterfaceDef):
		ConstantDef(-> InterfaceDef)

-- Interface methods have different modifiers from class methods
'nonterm' InterfaceMethodDef(-> DEFINITION)

	'rule' InterfaceMethodDef(-> method(Position, interfacemethodmodifiers(Modifier), Name, signature(Parameters, Returns), Alias, Throws)):
		InterfaceMethodModifier(-> Modifier) "method" @(-> Position) UnqualifiedId(-> Name) Parameters(-> Parameters) OptionalUnqualifiedAliasClause(-> Alias) OptionalThrowsClause(-> Throws) ReturnsClause(-> Returns)
	
'nonterm' InterfaceMethodModifier(-> MODIFIER)

	'rule' InterfaceMethodModifier(-> default):
		"default"

	'rule' InterfaceMethodModifier(-> class):
		"class"

	'rule' InterfaceMethodModifier(-> inferred):
		-- default

--------------------------------------------------------------------------------
-- Class-specific rules
--------------------------------------------------------------------------------

'nonterm' ClassModifiers(-> MODIFIER)

	'rule' ClassModifiers(-> classmodifiers(AccessModifier, StrictFPModifier, InheritanceModifier, ModifyModifier, InstanceModifier)):
		AccessModifier(-> AccessModifier) StrictFPModifier(-> StrictFPModifier) InheritanceModifier(-> InheritanceModifier) ModifyModifier(-> ModifyModifier) InstanceModifier(-> InstanceModifier)

'nonterm' ClassDefs(-> DEFINITION)

    'rule' ClassDefs(-> sequence(Left, Right)):
        ClassDef(-> Left) Separator
        ClassDefs(-> Right)
        
    'rule' ClassDefs(-> nil):
        -- no more definitions

-- definitions in classes are one of four types: constructor, method,
-- constant and variable
'nonterm' ClassDef(-> DEFINITION)

	'rule' ClassDef(-> Constructor):
		ConstructorDef(-> Constructor)
		
	'rule' ClassDef(-> Constant):
		ConstantDef(-> Constant)

	'rule' ClassDef(-> Method):
		MethodDef(-> Method)

	'rule' ClassDef(-> Variable):
		VariableDef(-> Variable)

--------------------------------------------------------------------------------

'nonterm' InheritsClause(-> TYPELIST)

	'rule' InheritsClause(-> typelist(Type, nil)):
		"inherits" ClassName(-> Type)

	'rule' InheritsClause(-> nil):
		-- optional

'nonterm' ImplementsClause(-> TYPELIST)

	'rule' ImplementsClause(-> Interfaces):
		"implements" InterfaceList(-> Interfaces)
		
	'rule' ImplementsClause(-> nil):
		-- optional

'nonterm' InterfaceList(-> TYPELIST)

	'rule' InterfaceList(-> typelist(Head, Tail)):
		ClassName(-> Head) "," InterfaceList(-> Tail)
	
	'rule' InterfaceList(-> typelist(Interface, nil)):
		ClassName(-> Interface)

'nonterm' ClassName(-> TYPE)

	'rule' ClassName(-> Type):
		NamedType(-> Type)

------------ Constructor

'nonterm' ConstructorDef(-> DEFINITION)

	'rule' ConstructorDef(-> constructor(Position, Modifiers, Name, signature(Parameters, nil), Alias)):
		ConstructorModifiers(-> Modifiers) "constructor" @(-> Position) UnqualifiedId(-> Name) Parameters(-> Parameters) OptionalUnqualifiedAliasClause(-> Alias)

	
'nonterm' ConstructorModifiers(-> MODIFIER)

	'rule' ConstructorModifiers(-> constructormodifiers(AccessModifier)):
		AccessModifier(-> AccessModifier)

------------ Method

'nonterm' MethodDef(-> DEFINITION)
	
	'rule' MethodDef(-> method(Position, Modifiers, Name, signature(Parameters, Returns), Alias, Throws)):
		MethodModifiers(-> Modifiers) "method" @(-> Position) UnqualifiedId(-> Name) Parameters(-> Parameters) OptionalUnqualifiedAliasClause(-> Alias) OptionalThrowsClause(-> Throws) ReturnsClause(-> Returns)
	
'nonterm' MethodModifiers(-> MODIFIER)

	'rule' MethodModifiers(-> methodmodifiers(AccessModifier, SyncModifier, NativeModifier, StrictFPModifier, InheritanceModifier, InstanceModifier)):
		AccessModifier(-> AccessModifier) SyncModifier(-> SyncModifier) NativeModifier(-> NativeModifier) StrictFPModifier(-> StrictFPModifier) InheritanceModifier(-> InheritanceModifier) InstanceModifier(-> InstanceModifier)

------------ Variable

'nonterm' VariableDef(-> DEFINITION)
	
	'rule' VariableDef(-> variable(Position, Modifiers, Name, Type)):
		VariableModifiers(-> Modifiers) "variable" @(-> Position) UnqualifiedId(-> Name) TypeClause(-> Type)
	
'nonterm' VariableModifiers(-> MODIFIER)

	'rule' VariableModifiers(-> variablemodifiers(AccessModifier, TransientModifier, ModifyModifier, InstanceModifier)):
		AccessModifier(-> AccessModifier) TransientModifier(-> TransientModifier) ModifyModifier(-> ModifyModifier) InstanceModifier(-> InstanceModifier)

------------ Constant

'nonterm' ConstantDef(-> DEFINITION)
	
	'rule' ConstantDef(-> constant(Position, Name, Type, Value)):
		"constant" @(-> Position) UnqualifiedId(-> Name) TypeClause(-> Type) OptionalLiteralClause(-> Value)

--------------------------------------------------------------------------------
-- Definition modifiers
--------------------------------------------------------------------------------

'nonterm' AccessModifier(-> MODIFIER)

	'rule' AccessModifier(-> public):
		"public"

	'rule' AccessModifier(-> protected):
		"protected"
		
	'rule' AccessModifier(-> inferred):
		-- default

'nonterm' SyncModifier(-> MODIFIER)

	'rule' SyncModifier(-> synchronized):
		"synchronized"

	'rule' SyncModifier(-> inferred):
		-- default

'nonterm' NativeModifier(-> MODIFIER)

	'rule' NativeModifier(-> native):
		"native"

	'rule' NativeModifier(-> inferred):
		-- default

'nonterm' StrictFPModifier(-> MODIFIER)

	'rule' StrictFPModifier(-> strictfp):
		"strictfp"

	'rule' StrictFPModifier(-> inferred):
		-- default
		
'nonterm' InheritanceModifier(-> MODIFIER)

	'rule' InheritanceModifier(-> abstract):
		"abstract"

	'rule' InheritanceModifier(-> final):
		"final"

	'rule' InheritanceModifier(-> inferred):
		-- default

'nonterm' InstanceModifier(-> MODIFIER)

	'rule' InstanceModifier(-> class):
		"class"

	'rule' InstanceModifier(-> inferred):
		-- default

'nonterm' ModifyModifier(-> MODIFIER)
	
	'rule' ModifyModifier(-> final):
		"final"
	
	'rule' ModifyModifier(-> volatile):
		"volatile"
	
	'rule' ModifyModifier(-> inferred):
		-- default

'nonterm' TransientModifier(-> MODIFIER)

	'rule' TransientModifier(-> transient):
		"transient"

	'rule' TransientModifier(-> inferred):
		-- default

------------ Clauses

'nonterm' TypeClause(-> TYPE)
	
	'rule' TypeClause(-> Type):
		"as" Type(-> Type)

'nonterm' OptionalTypeClause(-> TYPE)

    'rule' OptionalTypeClause(-> Type):
        TypeClause(-> Type)
        
    'rule' OptionalTypeClause(-> nil):
        @(-> Position)

'nonterm' OptionalLiteralClause(-> EXPRESSION)
	
	'rule' OptionalLiteralClause(-> Value):
		"is" ConstantTermExpression(-> Value)

	'rule' OptionalLiteralClause(-> nil):
		-- no literal

'nonterm' OptionalUnqualifiedAliasClause(-> OPTIONALID)
	
	'rule' OptionalUnqualifiedAliasClause(-> id(Alias)):
		"named" UnqualifiedId(-> Alias)

    'rule' OptionalUnqualifiedAliasClause(-> nil):
        -- no alias
        
'nonterm' QualifiedAliasClause(-> ID)
	
	'rule' QualifiedAliasClause(-> Alias):
		"named" PackageId(-> Alias)

'nonterm' Parameters(-> PARAMETERLIST)

    'rule' Parameters(-> Parameters):
        "(" OptionalParameterList(-> Parameters) ")"

'nonterm' ReturnsClause(-> TYPE)

    'rule' ReturnsClause(-> Type):
        "returns" @(-> Position) Type(-> Type)

'nonterm' OptionalThrowsClause(-> OPTIONALID)

    'rule' OptionalThrowsClause(-> id(Name)):
        "throws" @(-> Position) UnqualifiedId(-> Name)

    'rule' OptionalThrowsClause(-> nil):
        -- throws is optional

'nonterm' OptionalReturnsClause(-> TYPE)

    'rule' OptionalReturnsClause(-> Type):
        "returns" @(-> Position) Type(-> Type)

    'rule' OptionalReturnsClause(-> nil):
        @(-> Position)

'nonterm' OptionalParameterList(-> PARAMETERLIST)

    'rule' OptionalParameterList(-> List):
        ParameterList(-> List)
        
    'rule' OptionalParameterList(-> nil):
        -- empty

'nonterm' ParameterList(-> PARAMETERLIST)

    'rule' ParameterList(-> parameterlist(Head, Tail)):
        Parameter(-> Head) "," ParameterList(-> Tail)
        
    'rule' ParameterList(-> parameterlist(Head, nil)):
        Parameter(-> Head)

'nonterm' Parameter(-> PARAMETER)

    'rule' Parameter(-> parameter(Position, Name, Type)):
        UnqualifiedId(-> Name) @(-> Position) TypeClause(-> Type)

    'rule' Parameter(-> variadic(Position)):
        "..." @(-> Position)

--------------------------------------------------------------------------------
-- Type Syntax
--------------------------------------------------------------------------------

'nonterm' TemplateType(-> TYPE)

    -- simple declaration of a class or interface
    'rule' TemplateType(-> template(Position, Name, nil)):
        UnqualifiedId(-> Name) @(-> Position)

    'rule' TemplateType(-> Type):
        GenericType(-> Type)

'nonterm' GenericType(-> TYPE)

    'rule' GenericType(-> template(Position, Name, Params)):
        @(-> Position) UnqualifiedId(-> Name) "<" GenericTypeList(-> Params) ">"

'nonterm' GenericTypeList(-> TYPELIST)

    'rule' GenericTypeList(-> typelist(Head, Tail)):
         GenericTypeListElement(-> Head) "," GenericTypeList(-> Tail)

    'rule' GenericTypeList(-> typelist(Head, nil)):
        GenericTypeListElement(-> Head)

'nonterm' GenericTypeListElement(-> TYPE)

    'rule' GenericTypeListElement(-> placeholder(Position, Name)):
        @(-> Position) UnqualifiedId(-> Name)

    'rule' GenericTypeListElement(-> wildcard(Position, Bounds)):
        @(-> Position) "?" OptionalBounds(-> Bounds)

    'rule' GenericTypeListElement(-> Type):
        GenericType(-> Type)

'nonterm' OptionalBounds(-> BOUNDS)

    'rule' OptionalBounds(-> upper(Type)):
        "extends" NamedType(-> Type)

    'rule' OptionalBounds(-> lower(Type)):
        "super" NamedType(-> Type)

    'rule' OptionalBounds(-> unbounded):

'nonterm' Type(-> TYPE)

    'rule' Type(-> Type):
        JavaArray(-> Type)

	'rule' Type(-> Type):
		PrimitiveType(-> Type)

	'rule' Type(-> Type):
		NamedType(-> Type)

'nonterm' PrimitiveType(-> TYPE)

	'rule' PrimitiveType(-> byte(Position)):
        "byte" @(-> Position)

    'rule' PrimitiveType(-> short(Position)):
        "short" @(-> Position)

    'rule' PrimitiveType(-> int(Position)):
        "int" @(-> Position)

    'rule' PrimitiveType(-> long(Position)):
        "long" @(-> Position)

    'rule' PrimitiveType(-> float(Position)):
        "float" @(-> Position)

    'rule' PrimitiveType(-> double(Position)):
        "double" @(-> Position)

    'rule' PrimitiveType(-> boolean(Position)):
        "boolean" @(-> Position)

    'rule' PrimitiveType(-> char(Position)):
        "char" @(-> Position)

    'rule' PrimitiveType(-> string(Position)):
        "String" @(-> Position)
    
    'rule' PrimitiveType(-> nil)
        "nothing" @(-> Position)

'nonterm' NamedType(-> TYPE)

    'rule' NamedType(-> named(Position, Name, nil)):
        UnqualifiedId(-> Name) @(-> Position)

	'rule' NamedType(-> named(Position, Name, Parameter)):
		UnqualifiedId(-> Name) @(-> Position) "<" NamedTypeList(-> Parameter) ">"

'nonterm' NamedTypeList(-> TYPELIST)

    'rule' NamedTypeList(-> typelist(Named, Tail)):
        NamedTypeListElement(-> Named) "," NamedTypeList(-> Tail)
        
    'rule' NamedTypeList(-> typelist(Named, nil)):
        NamedTypeListElement(-> Named)

'nonterm' NamedTypeListElement(-> TYPE)

    'rule' NamedTypeListElement(-> wildcard(Position, Bounds)):
        @(-> Position) "?" OptionalBounds(-> Bounds)

    'rule' NamedTypeListElement(-> Type):
        NamedType(-> Type)

'nonterm' JavaArray(-> TYPE)

    'rule' JavaArray(-> jarray(Position, Type, 1)):
        PrimitiveType(-> Type) @(-> Position) "[]"

    'rule' JavaArray(-> jarray(Position, Type, 1)):
        NamedType(-> Type) @(-> Position) "[]"

    'rule' JavaArray(-> jarray(Position, Type, Dimension + 1)):
        JavaArray(-> jarray(Position, Type, Dimension)) "[]"

'nonterm' ConstantTermExpression(-> EXPRESSION)

    'rule' ConstantTermExpression(-> true(Position)):
        "true" @(-> Position)

    'rule' ConstantTermExpression(-> false(Position)):
        "false" @(-> Position)

    'rule' ConstantTermExpression(-> real(Position, Value)):
        DOUBLE_LITERAL(-> Value) @(-> Position)

    'rule' ConstantTermExpression(-> integer(Position, Value)):
        INTEGER_LITERAL(-> Value) @(-> Position)

    'rule' ConstantTermExpression(-> string(Position, Value)):
        StringLiteral(-> Value) @(-> Position)

--------------------------------------------------------------------------------
-- Identifier Syntax
--------------------------------------------------------------------------------

'nonterm' PackageId(-> ID)

    'rule' PackageId(-> Id):
        @(-> Position) IdentifierSequence(-> Name)
        Id::ID
        Id'Position <- Position
        Id'Name <- Name
        Id'Meaning <- nil

'nonterm' QualifiedId(-> ID)

    'rule' QualifiedId(-> Id):
        @(-> Position) NAME_LITERAL(-> Head) "." IdentifierSequence(-> Tail) "." NAME_LITERAL(-> Name)
        Id::ID
        Id'Position <- Position
        Id'Name <- qualified(Name, package(Head, Tail))
        Id'Meaning <- nil

'nonterm' UnqualifiedId(-> ID)

    'rule' UnqualifiedId(-> Id):
        @(-> Position) NAME_LITERAL(-> Name)
        Id::ID
        Id'Position <- Position
        Id'Name <- unqualified(Name)
        Id'Meaning <- nil

'nonterm' IdentifierSequence(-> QUALIFIEDNAME)

    'rule' IdentifierSequence(-> package(Head, Tail)):
        NAME_LITERAL(-> Head) "." IdentifierSequence(-> Tail)

    'rule' IdentifierSequence(-> unqualified(Name)):
        NAME_LITERAL(-> Name)

--------------------------------------------------------------------------------
-- Separator
--------------------------------------------------------------------------------

'nonterm' OptionalSeparator

    'rule' OptionalSeparator:
        Separator
        
    'rule' OptionalSeparator:
        -- empty
        
'nonterm' Separator

    'rule' Separator:
        SEPARATOR Separator
        
    'rule' Separator:
        SEPARATOR

--------------------------------------------------------------------------------
-- Tokens
--------------------------------------------------------------------------------

'nonterm' StringLiteral(-> STRING)

    'rule' StringLiteral(-> Value):
        STRING_LITERAL(-> EscapedValue) @(-> Position)
        (|
            UnescapeStringLiteral(Position, EscapedValue -> Value)
        ||
            Error_MalformedEscapedString(Position, EscapedValue)
            where(EscapedValue -> Value)
        |)

'nonterm' StringOrNameLiteral(-> STRING)

    'rule' StringOrNameLiteral(-> String):
        StringLiteral(-> String)
        
    'rule' StringOrNameLiteral(-> String):
        NAME_LITERAL(-> Name)
        GetStringOfNameLiteral(Name -> String)

'token' NAME_LITERAL (-> NAME)
'token' INTEGER_LITERAL (-> INT)
'token' DOUBLE_LITERAL (-> DOUBLE)
'token' STRING_LITERAL (-> STRING)

'token' SEPARATOR

'token' END_OF_UNIT
'token' NEXT_UNIT

--*--*--*--*--*--*--*--

'action' InitializeCustomInvokeLists()
    'rule' InitializeCustomInvokeLists():
        -- nothing
'nonterm' CustomPostfixOperators(-> INT)
    'rule' CustomPostfixOperators(-> 10000):
        "THISCANNEVERHAPPEN"
'nonterm' CustomPrefixOperators(-> INT)
    'rule' CustomPrefixOperators(-> 10000):
        "THISCANNEVERHAPPEN"
'nonterm' CustomBinaryOperators(-> INT)
    'rule' CustomBinaryOperators(-> 10000):
        "THISCANNEVERHAPPEN"
'nonterm' CustomTerms(-> EXPRESSION)
    'rule' CustomTerms(-> nil):
        "THISCANNEVERHAPPEN"
'nonterm' CustomIterators(-> EXPRESSION)
    'rule' CustomIterators(-> nil):
        "THISCANNEVERHAPPEN"
'nonterm' CustomKeywords(-> STRING)
    'rule' CustomKeywords(-> String):
        "THISCANNEVERHAPPEN"
        where("THISCANNEVERHAPPEN" -> String)



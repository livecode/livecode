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

'module' grammar

'use'
    support
    types
    bind
    check
    generate
    syntax

--------------------------------------------------------------------------------

'root'
    Parse(-> Modules)
    (|
        ErrorsDidOccur()
    ||
        IsDependencyCompile()
        
        DependStart()

        -- First generate the mapping between module name and source-file(s)
        Depend_GenerateMapping(Modules)
        
        -- Now generate the direct dependencies for each module
        Depend_GenerateDependencies(Modules)
        
        DependFinish()
    ||
        Compile(Modules)
    |)

---------

'action' Depend_GenerateMapping(MODULELIST)

    'rule' Depend_GenerateMapping(modulelist(module(Position, Kind, Name, _), Rest)):
    	GetQualifiedName(Name -> ModuleName)
        GetFilenameOfPosition(Position -> Filename)
        DependDefineMapping(ModuleName, Filename)
        Depend_GenerateMapping(Rest)
        
    'rule' Depend_GenerateMapping(nil):
        -- do nothing
        
'action' Depend_GenerateDependencies(MODULELIST)

    'rule' Depend_GenerateDependencies(modulelist(module(_, _, Name, Definitions), Rest)):
    	GetQualifiedName(Name -> ModuleName)
        Depend_GenerateDependenciesForModule(ModuleName, Definitions)
        Depend_GenerateDependencies(Rest)

    'rule' Depend_GenerateDependencies(nil):
        -- do nothing

'action' Depend_GenerateDependenciesForModule(NAME, DEFINITION)

    'rule' Depend_GenerateDependenciesForModule(ModuleName, sequence(Left, Right)):
        Depend_GenerateDependenciesForModule(ModuleName, Left)
        Depend_GenerateDependenciesForModule(ModuleName, Right)

    'rule' Depend_GenerateDependenciesForModule(ModuleName, import(_, Name)):
        GetQualifiedName(Name -> DependencyName)
        DependDefineDependency(ModuleName, DependencyName)

    'rule' Depend_GenerateDependenciesForModule(ModuleName, _):
        -- do nothing

---------

'action' Compile(MODULELIST)

    'rule' Compile(Modules):
        InitializeBind
        BindModules(Modules, Modules)
        (|
            ErrorsDidOccur()
        ||
            CheckModules(Modules)
            (|
                ErrorsDidOccur()
            ||
                (|
                    IsBootstrapCompile()
                    InitializeSyntax
                    GenerateSyntaxForModules(Modules)
                    (|
                        ErrorsDidOccur()
                    ||
                        GenerateSyntaxRules()
                        GenerateModules(Modules)
                    |)
                ||
					GenerateModules(Modules)
                |)
            |)
        |)

'action' BindModules(MODULELIST, MODULELIST)

    'rule' BindModules(modulelist(Head, Tail), Imports):
        (|
            Head'Kind -> import
        ||
            Bind(Head, Imports)
        |)
        BindModules(Tail, Imports)
        
    'rule' BindModules(nil, _):
        -- empty

'action' CheckModules(MODULELIST)

    'rule' CheckModules(modulelist(Head, Tail)):
        (|
            Head'Kind -> import
        ||
            Check(Head)
        |)
        CheckModules(Tail)
        
    'rule' CheckModules(nil):
        -- empty
        
'action' GenerateSyntaxForModules(MODULELIST)

    'rule' GenerateSyntaxForModules(modulelist(Head, Tail)):
        (|
            Head'Kind -> import
        ||
            GenerateSyntax(Head)
        |)
        GenerateSyntaxForModules(Tail)

    'rule' GenerateSyntaxForModules(nil):
        -- empty

--------------------------------------------------------------------------------

'nonterm' Parse(-> MODULELIST)

    'rule' Parse(-> Modules):
        ModuleList(-> Modules)

--------------------------------------------------------------------------------
-- Module Syntax
--------------------------------------------------------------------------------

'nonterm' ModuleList(-> MODULELIST)

    'rule' ModuleList(-> modulelist(Head, Tail)):
        Module(-> Head)
        NEXT_UNIT
        ModuleList(-> Tail)
        
    'rule' ModuleList(-> modulelist(Head, nil)):
        Module(-> Head)

'nonterm' Module(-> MODULE)

    'rule' Module(-> module(Position, module, Name, Definitions)):
        OptionalSeparator
        "module" @(-> Position) QualifiedId(-> Name) Separator
        Definitions(-> Definitions)
        "end" "module" OptionalSeparator
        END_OF_UNIT
		GetQualifiedName(Name -> NameName)
        GetStringOfNameLiteral(NameName -> NameString)
		AddImportedModuleName(NameString)

    'rule' Module(-> module(Position, widget, Name, Definitions)):
        OptionalSeparator
        "widget" @(-> Position) QualifiedId(-> Name) Separator
        Definitions(-> Definitions)
        "end" "widget" OptionalSeparator
        END_OF_UNIT

    'rule' Module(-> module(Position, library, Name, Definitions)):
        OptionalSeparator
        "library" @(-> Position) QualifiedId(-> Name) Separator
        Definitions(-> Definitions)
        "end" "library" OptionalSeparator
        END_OF_UNIT

    'rule' Module(-> module(Position, import, Name, Definitions)):
        OptionalSeparator
        "import" "module" @(-> Position) QualifiedId(-> Name) Separator
        ImportDefinitions(-> Definitions)
        "end" "module" OptionalSeparator
        END_OF_UNIT

'nonterm' ImportDefinitions(-> DEFINITION)

    'rule' ImportDefinitions(-> sequence(Left, Right)):
        ImportDefinition(-> Left) Separator
        ImportDefinitions(-> Right)
        
    'rule' ImportDefinitions(-> nil):
        -- done!
        
'nonterm' ImportDefinition(-> DEFINITION)

    'rule' ImportDefinition(-> Import):
        Import(-> Import)

    'rule' ImportDefinition(-> type(Position, public, Id, foreign(Position, ""))):
        "foreign" @(-> Position) "type" Identifier(-> Id)
    
    'rule' ImportDefinition(-> type(Position, public, Id, handler(Position, normal, Signature))):
        "handler" @(-> Position) "type" Identifier(-> Id) Signature(-> Signature)

    'rule' ImportDefinition(-> type(Position, public, Id, handler(Position, foreign, Signature))):
        "foreign" @(-> Position) "handler" "type" Identifier(-> Id) Signature(-> Signature)

    'rule' ImportDefinition(-> type(Position, public, Id, record(Position, Fields))):
        "record" @(-> Position) "type" Identifier(-> Id) Separator
            RecordFields(-> Fields)
        "end" "type"

    'rule' ImportDefinition(-> type(Position, public, Id, Type)):
        "type" @(-> Position) Identifier(-> Id) "is" Type(-> Type)

    'rule' ImportDefinition(-> constant(Position, public, Id, nil)):
        "constant" @(-> Position) Identifier(-> Id)

    'rule' ImportDefinition(-> variable(Position, public, Id, Type)):
        "variable" @(-> Position) Identifier(-> Id) "as" Type(-> Type)

    'rule' ImportDefinition(-> handler(Position, public, Id, Signature, nil, nil)):
        "handler" @(-> Position) Identifier(-> Id) Signature(-> Signature)

    'rule' ImportDefinition(-> unsafe(Position, handler(Position, public, Id, Signature, nil, nil))):
        "unsafe" "handler" @(-> Position) Identifier(-> Id) Signature(-> Signature)

    'rule' ImportDefinition(-> foreignhandler(Position, public, Id, Signature, "")):
        "foreign" "handler" @(-> Position) Identifier(-> Id) Signature(-> Signature)

--------------------------------------------------------------------------------
-- Metadata Syntax
--------------------------------------------------------------------------------

'nonterm' Metadata(-> DEFINITION)

    'rule' Metadata(-> metadata(Position, Key, Value)):
        "metadata" @(-> Position) StringOrNameLiteral(-> Key) "is" StringLiteral(-> Value)
        
--------------------------------------------------------------------------------
-- Import Syntax
--------------------------------------------------------------------------------

'nonterm' Import(-> DEFINITION)
    'rule' Import(-> ImportList):
        "use" @(-> Position) IdentifierList(-> Identifiers)
        ExpandImports(Position, Identifiers -> ImportList)

'action' ExpandImports(POS, IDLIST -> DEFINITION)

    'rule' ExpandImports(Position, idlist(Id, nil) -> import(Position, Id)):
    	GetQualifiedName(Id -> Name)
        GetStringOfNameLiteral(Name -> NameString)
        (|
            (|
                -- In bootstrap mode, all modules have to be listed on command line.
                IsBootstrapCompile()
			||
                AddImportedModuleFile(NameString)
            |)
        ||
            (|
                IsDependencyCompile()
            ||
                Error_UnableToFindImportedModule(Position, Name)
            |)
        |)

    'rule' ExpandImports(Position, idlist(Id, Tail) -> sequence(import(Position, Id), ExpandedTail)):
        ExpandImports(Position, Tail -> ExpandedTail)

--------------------------------------------------------------------------------
-- Definitions Syntax
--------------------------------------------------------------------------------

'nonterm' Definitions(-> DEFINITION)

    'rule' Definitions(-> sequence(Head, Tail)):
        Definition(-> Head) Separator
        Definitions(-> Tail)
        
    'rule' Definitions(-> nil):
        -- empty
        
'nonterm' Definition(-> DEFINITION)

    'rule' Definition(-> Metadata):
        Metadata(-> Metadata)

    'rule' Definition(-> Import):
        Import(-> Import)
        
    'rule' Definition(-> Constant):
        ConstantDefinition(-> Constant)
        
    'rule' Definition(-> Type):
        TypeDefinition(-> Type)

    'rule' Definition(-> Variable):
        VariableDefinition(-> Variable)

    'rule' Definition(-> Handler):
        HandlerDefinition(-> Handler)

    'rule' Definition(-> Property):
        PropertyDefinition(-> Property)

    'rule' Definition(-> Event):
        EventDefinition(-> Event)
        
    'rule' Definition(-> Syntax)
        SyntaxDefinition(-> Syntax)

---------- Constant

'nonterm' ConstantDefinition(-> DEFINITION)

    'rule' ConstantDefinition(-> constant(Position, Access, Name, Value)):
        Access(-> Access) "constant" @(-> Position) Identifier(-> Name) "is" Expression(-> Value)

'nonterm' Access(-> ACCESS)

    'rule' Access(-> inferred):
        -- empty

    'rule' Access(-> public):
        "public"
        
    'rule' Access(-> protected):
        "protected"
        
    'rule' Access(-> private):
        "private"

---------- Variable

'nonterm' VariableDefinition(-> DEFINITION)

    'rule' VariableDefinition(-> variable(Position, Access, Name, Type)):
        Access(-> Access) "variable" @(-> Position) Identifier(-> Name) OptionalTypeClause(-> Type)
        
'nonterm' OptionalTypeClause(-> TYPE)

    'rule' OptionalTypeClause(-> Type):
        "as" Type(-> Type)
        
    'rule' OptionalTypeClause(-> unspecified):
        @(-> Position)

---------- Type

'nonterm' TypeDefinition(-> DEFINITION)

    'rule' TypeDefinition(-> type(Position, Access, Name, Type)):
        Access(-> Access) "type" @(-> Position) Identifier(-> Name) "is" Type(-> Type)
    
    'rule' TypeDefinition(-> type(Position, Access, Name, foreign(Position, Binding))):
        Access(-> Access) "foreign" @(-> Position) "type" Identifier(-> Name) "binds" "to" StringLiteral(-> Binding)
        
    'rule' TypeDefinition(-> type(Position, Access, Name, record(Position, Fields))):
        Access(-> Access) "record" @(-> Position) "type" Identifier(-> Name) Separator
            RecordFields(-> Fields)
        "end" "type"
        
    'rule' TypeDefinition(-> type(Position, Access, Name, enum(Position, Base, Fields))):
        Access(-> Access) "enum" @(-> Position) "type" Identifier(-> Name) OptionalBaseType(-> Base) Separator
            EnumFields(-> Fields)
        "end" "type"
        
    'rule' TypeDefinition(-> type(Position, Access, Name, handler(Position, normal, Signature))):
        Access(-> Access) "handler" @(-> Position) "type" Identifier(-> Name) Signature(-> Signature)

    'rule' TypeDefinition(-> type(Position, Access, Name, handler(Position, foreign, Signature))):
        Access(-> Access) "foreign" @(-> Position) "handler" "type" Identifier(-> Name) Signature(-> Signature)

--

'nonterm' OptionalBaseType(-> TYPE)

    'rule' OptionalBaseType(-> BaseType):
        "based" "on" Type(-> BaseType)
        
    'rule' OptionalBaseType(-> undefined(Position)):
        @(-> Position)

--

'nonterm' TypeFields(-> FIELDLIST)

    'rule' TypeFields(-> fieldlist(Head, Rest)):
        TypeField(-> Head) Separator
        TypeFields(-> Rest)
        
    'rule' TypeFields(-> nil):
        -- nothing

'nonterm' TypeField(-> FIELD)

    'rule' TypeField(-> action(Position, Name, Handler)):
        Identifier(-> Name) @(-> Position) "is" Identifier(-> Handler)

--

'nonterm' RecordFields(-> FIELDLIST)

    'rule' RecordFields(-> fieldlist(Head, Rest)):
        RecordField(-> Head) Separator
        RecordFields(-> Rest)
        
    'rule' RecordFields(-> nil):
        -- nothing

'nonterm' RecordField(-> FIELD)

    'rule' RecordField(-> slot(Position, Name, Type)):
        Identifier(-> Name) @(-> Position) OptionalTypeClause(-> Type)
        
    'rule' RecordField(-> slot(Position, Name, Type)):
        StringyIdentifier(-> Name) @(-> Position) OptionalTypeClause(-> Type)

--

'nonterm' EnumFields(-> FIELDLIST)

    'rule' EnumFields(-> fieldlist(Head, Rest)):
        EnumField(-> Head) Separator
        EnumFields(-> Rest)
        
    'rule' EnumFields(-> nil):
        -- nothing

'nonterm' EnumField(-> FIELD)

    'rule' EnumField(-> element(Position, Name)):
        Identifier(-> Name) @(-> Position)

---------- Handler

'nonterm' HandlerDefinition(-> DEFINITION)

    'rule' HandlerDefinition(-> handler(Position, Access, Name, Signature, nil, Body)):
        Access(-> Access) "handler" @(-> Position) Identifier(-> Name) Signature(-> Signature) Separator
            Statements(-> Body)
        "end" "handler"

    'rule' HandlerDefinition(-> unsafe(Position, handler(Position, Access, Name, Signature, nil, Body))):
        Access(-> Access) "unsafe" "handler" @(-> Position) Identifier(-> Name) Signature(-> Signature) Separator
            Statements(-> Body)
        "end" "handler"

	'rule' HandlerDefinition(-> foreignhandler(Position, Access, Name, Signature, Binding)):
		Access(-> Access) "__safe" "foreign" "handler" @(-> Position) Identifier(-> Name) Signature(-> Signature) "binds" "to" StringLiteral(-> Binding)

    'rule' HandlerDefinition(-> unsafe(Position, foreignhandler(Position, Access, Name, Signature, Binding))):
        Access(-> Access) "foreign" "handler" @(-> Position) Identifier(-> Name) Signature(-> Signature) "binds" "to" StringLiteral(-> Binding)

'nonterm' Signature(-> SIGNATURE)

    'rule' Signature(-> signature(Parameters, Result)):
        "(" OptionalParameterList(-> Parameters) ")" OptionalReturnsClause(-> Result)

'nonterm' OptionalReturnsClause(-> TYPE)

    'rule' OptionalReturnsClause(-> Type)
        "returns" @(-> Position) Type(-> Type)

    'rule' OptionalReturnsClause(-> unspecified)
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

    'rule' Parameter(-> parameter(Position, Mode, Name, Type)):
        Mode(-> Mode) @(-> Position) Identifier(-> Name) OptionalTypeClause(-> Type)

    'rule' Parameter(-> parameter(Position, in, Name, Type)):
        Identifier(-> Name) @(-> Position) OptionalTypeClause(-> Type)

    'rule' Parameter(-> parameter(Position, variadic, Name, unspecified)):
        "..." @(-> Position)
        MakeNameLiteral("" -> Identifier)
		AssignId(Position, Identifier, nil -> Name)
        
'nonterm' Mode(-> MODE)

    'rule' Mode(-> in):
        "in"

    'rule' Mode(-> out):
        "out"

    'rule' Mode(-> inout):
        "inout"

---------- Property

'nonterm' PropertyDefinition(-> DEFINITION)

    'rule' PropertyDefinition(-> property(Position, public, Name, Getter, Setter)):
        "property" @(-> Position) Identifier(-> Name) "get" Identifier(-> Getter) OptionalSetClause(-> Setter)

    'rule' PropertyDefinition(-> property(Position, public, Name, Getter, Setter)):
        "property" @(-> Position) StringyIdentifier(-> Name) "get" Identifier(-> Getter) OptionalSetClause(-> Setter)

'nonterm' OptionalSetClause(-> OPTIONALID)

    'rule' OptionalSetClause(-> id(Setter)):
        "set" Identifier(-> Setter)
        
    'rule' OptionalSetClause(-> nil):
        -- nothing

---------- Event

'nonterm' EventDefinition(-> DEFINITION)

    'rule' EventDefinition(-> event(Position, public, Name, Signature)):
        "event" @(-> Position) Identifier(-> Name) Signature(-> Signature)

---------- Syntax

'nonterm' SyntaxDefinition(-> DEFINITION)

    'rule' SyntaxDefinition(-> syntax(Position, public, Name, Class, Warnings, Syntax, Methods)):
        "syntax" @(-> Position) Identifier(-> Name) SyntaxClass(-> Class) Separator
            SyntaxWarnings(-> Warnings)
            Syntax(-> Syntax) Separator
        "begin" Separator
            SyntaxMethods(-> Methods)
        "end" "syntax"
    
'nonterm' SyntaxWarnings(-> SYNTAXWARNING)

    'rule' SyntaxWarnings(-> deprecated(Message)):
        "deprecate" "with" "message" StringLiteral(-> Message) Separator
        
    'rule' SyntaxWarnings(-> nil):
        -- nothing

'nonterm' SyntaxClass(-> SYNTAXCLASS)

    'rule' SyntaxClass(-> statement):
        "is" "statement"

    'rule' SyntaxClass(-> expression):
        "is" "expression"

    'rule' SyntaxClass(-> iterator):
        "is" "iterator"

    'rule' SyntaxClass(-> prefix(Precedence)):
        "is" "prefix" "operator" "with" SyntaxPrecedence(-> Precedence) "precedence"

    'rule' SyntaxClass(-> postfix(Precedence)):
        "is" "postfix" "operator" "with" SyntaxPrecedence(-> Precedence) "precedence"

    'rule' SyntaxClass(-> binary(Assoc, Precedence)):
        "is" SyntaxAssoc(-> Assoc) "binary" "operator" "with" SyntaxPrecedence(-> Precedence) "precedence"

    'rule' SyntaxClass(-> phrase):
        "is" "phrase"

'nonterm' SyntaxAssoc(-> SYNTAXASSOC)

    'rule' SyntaxAssoc(-> neutral):
        "neutral"
        
    'rule' SyntaxAssoc(-> left):
        "left"
        
    'rule' SyntaxAssoc(-> right):
        "right"

'nonterm' SyntaxPrecedence(-> SYNTAXPRECEDENCE)

    'rule' SyntaxPrecedence(-> scoperesolution):
        "scope" "resolution"

    'rule' SyntaxPrecedence(-> functioncall):
        "function" "call"

    'rule' SyntaxPrecedence(-> subscript):
        "subscript"

    'rule' SyntaxPrecedence(-> property):
        "property"

    'rule' SyntaxPrecedence(-> subscriptchunk):
        "subscript chunk"

    'rule' SyntaxPrecedence(-> functionchunk):
        "function chunk"

    'rule' SyntaxPrecedence(-> constructor):
        "constructor"

    'rule' SyntaxPrecedence(-> conversion):
        "conversion"

    'rule' SyntaxPrecedence(-> exponentiation):
        "exponentiation"

    'rule' SyntaxPrecedence(-> modifier):
        "modifier"

    'rule' SyntaxPrecedence(-> multiplication):
        "multiplication"

    'rule' SyntaxPrecedence(-> addition):
        "addition"

    'rule' SyntaxPrecedence(-> bitwiseshift):
        "bitwise shift"

    'rule' SyntaxPrecedence(-> concatenation):
        "concatenation"

    'rule' SyntaxPrecedence(-> comparison):
        "comparison"

    'rule' SyntaxPrecedence(-> classification):
        "classification"

    'rule' SyntaxPrecedence(-> bitwiseand):
        "bitwise and"

    'rule' SyntaxPrecedence(-> bitwisexor):
        "bitwise xor"

    'rule' SyntaxPrecedence(-> bitwiseor):
        "bitwise or"

    'rule' SyntaxPrecedence(-> logicalnot):
        "logical not"

    'rule' SyntaxPrecedence(-> logicaland):
        "logical and"

    'rule' SyntaxPrecedence(-> logicalor):
        "logical or"

    'rule' SyntaxPrecedence(-> sequence):
        "sequence"

'nonterm' SyntaxMethods(-> SYNTAXMETHODLIST)

    'rule' SyntaxMethods(-> methodlist(Head, Tail)):
        SyntaxMethod(-> Head) Separator
        SyntaxMethods(-> Tail)
        
    'rule' SyntaxMethods(-> nil):
        -- empty

'nonterm' SyntaxMethod(-> SYNTAXMETHOD)

    'rule' SyntaxMethod(-> method(Position, Name, Arguments)):
        Identifier(-> Name) @(-> Position) "(" OptionalConstantList(-> Arguments) ")"

--------------------------------------------------------------------------------
-- Type Syntax
--------------------------------------------------------------------------------

'nonterm' Type(-> TYPE)

    'rule' Type(-> named(Position, Name)):
        QualifiedId(-> Name) @(-> Position)
        
    'rule' Type(-> optional(Position, Base)):
        "optional" @(-> Position) Type(-> Base)

    'rule' Type(-> any(Position)):
        "any" @(-> Position)

    'rule' Type(-> boolean(Position)):
        "Boolean" @(-> Position)

    'rule' Type(-> integer(Position)):
        "Integer" @(-> Position)

    'rule' Type(-> real(Position)):
        "Real" @(-> Position)

    'rule' Type(-> number(Position)):
        "Number" @(-> Position)

    'rule' Type(-> string(Position)):
        "String" @(-> Position)

    'rule' Type(-> data(Position)):
        "Data" @(-> Position)

    'rule' Type(-> array(Position)):
        "Array" @(-> Position)

    'rule' Type(-> list(Position, ElementType)):
        "List" @(-> Position) OptionalElementType(-> ElementType)

    'rule' Type(-> undefined(Position)):
        "nothing" @(-> Position)

'nonterm' OptionalElementType(-> TYPE)

    'rule' OptionalElementType(-> Type)
        "of" Type(-> Type)
        
    'rule' OptionalElementType(-> unspecified):
        @(-> Position)

--------------------------------------------------------------------------------
-- Statement Syntax
--------------------------------------------------------------------------------

'nonterm' Statements(-> STATEMENT)

    'rule' Statements(-> sequence(Left, Right)):
        Statement(-> Left) Separator
        Statements(-> Right)
        
    'rule' Statements(-> nil):
        -- empty

'nonterm' Statement(-> STATEMENT)

    'rule' Statement(-> variable(Position, Name, Type)):
        "variable" @(-> Position) Identifier(-> Name) OptionalTypeClause(-> Type)

    'rule' Statement(-> if(Position, Condition, Consequent, Alternate)):
        "if" @(-> Position) Expression(-> Condition) "then" Separator
            Statements(-> Consequent)
        IfStatementElseIfs(-> Alternate)
        "end" "if"
        
    'rule' Statement(-> repeatforever(Position, Body)):
        "repeat" @(-> Position) "forever" Separator
            Statements(-> Body)
        "end" "repeat"

    'rule' Statement(-> repeatcounted(Position, Count, Body)):
        "repeat" @(-> Position) Expression(-> Count) "times" Separator
            Statements(-> Body)
        "end" "repeat"

    'rule' Statement(-> repeatwhile(Position, Condition, Body)):
        "repeat" @(-> Position) "while" Expression(-> Condition) Separator
            Statements(-> Body)
        "end" "repeat"

    'rule' Statement(-> repeatuntil(Position, Condition, Body)):
        "repeat" @(-> Position) "until" Expression(-> Condition) Separator
            Statements(-> Body)
        "end" "repeat"

    'rule' Statement(-> repeatupto(Position, Slot, Start, Finish, Step, Body)):
        "repeat" @(-> Position) "with" Identifier(-> Slot) "from" Expression(-> Start) "up" "to" Expression(-> Finish) RepeatStatementOptionalBy(-> Step) Separator
            Statements(-> Body)
        "end" "repeat"

    'rule' Statement(-> repeatdownto(Position, Slot, Start, Finish, Step, Body)):
        "repeat" @(-> Position) "with" Identifier(-> Slot) "from" Expression(-> Start) "down" "to" Expression(-> Finish) RepeatStatementOptionalBy(-> Step) Separator
            Statements(-> Body)
        "end" "repeat"
        
    'rule' Statement(-> repeatforeach(Position, Iterator, Container, Body)):
        "repeat" @(-> Position) "for" "each" CustomIterators(-> Iterator) "in" Expression(-> Container) Separator
            Statements(-> Body)
        "end" "repeat"

    /*'rule' Statement(-> try(Position, Body, Catches, Finally)):
        "try" @(-> Position) Separator
            Statements(-> Body)
        TryStatementCatches(-> Catches)
        TryStatementFinally(-> Finally)
        "end" "try"*/
        
    'rule' Statement(-> throw(Position, Value)):
        "throw" @(-> Position) Expression(-> Value)

    'rule' Statement(-> nextrepeat(Position)):
        "next" @(-> Position) "repeat"

    'rule' Statement(-> exitrepeat(Position)):
        "exit" @(-> Position) "repeat"
        
    'rule' Statement(-> return(Position, nil)):
        "return" @(-> Position)
        
    'rule' Statement(-> return(Position, Value)):
        "return" @(-> Position) Expression(-> Value)
        
    'rule' Statement(-> put(Position, Source, Target)):
        "put" @(-> Position) Expression(-> Source) "into" Expression(-> Target)

    'rule' Statement(-> put(Position, Source, Target)):
        "set" @(-> Position) Expression(-> Target) "to" Expression(-> Source)
        
    'rule' Statement(-> get(Position, Value)):
        "get" @(-> Position) Expression(-> Value)

    'rule' Statement(-> call(Position, Handler, Arguments)):
        QualifiedId(-> Handler) @(-> Position) "(" OptionalExpressionList(-> Arguments) ")"

    'rule' Statement(-> bytecode(Position, Opcodes)):
        "bytecode" @(-> Position) Separator
            Bytecodes(-> Opcodes)
        "end" "bytecode"

    'rule' Statement(-> unsafe(Position, Body)):
        "unsafe" @(-> Position) Separator
            Statements(-> Body)
        "end" "unsafe"

    'rule' Statement(-> postfixinto(Position, Statement, Target)):
        CustomStatements(-> Statement) "into" @(-> Position) Expression(-> Target)

    'rule' Statement(-> Statement):
        CustomStatements(-> Statement)
        
'nonterm' IfStatementElseIfs(-> STATEMENT)

    'rule' IfStatementElseIfs(-> if(Position, Condition, Consequent, Alternate)):
        "else" @(-> Position) "if" Expression(-> Condition) "then" Separator
            Statements(-> Consequent)
        IfStatementElseIfs(-> Alternate)
        
    'rule' IfStatementElseIfs(-> Else):
        "else" @(-> Position) Separator
            Statements(-> Else)
            
    'rule' IfStatementElseIfs(-> nil):
        -- nothing

'nonterm' RepeatStatementOptionalBy(-> EXPRESSION)

    'rule' RepeatStatementOptionalBy(-> By):
        "by" Expression(-> By)
        
    'rule' RepeatStatementOptionalBy(-> nil):
        -- nothing

/*'nonterm' TryStatementCatches(-> STATEMENT)

    'rule' TryStatementCatches(-> catch(Position, Type, Body)):
        "catch" Type(-> Type) Separator
            Statements(-> Body)*/

--------------------------------------------------------------------------------
-- Bytecode Syntax
--------------------------------------------------------------------------------

'nonterm' Bytecodes(-> BYTECODE)

    'rule' Bytecodes(-> sequence(Left, Right)):
        Bytecode(-> Left) Separator
        Bytecodes(-> Right)

    'rule' Bytecodes(-> nil):
        -- empty

'nonterm' Bytecode(-> BYTECODE)

    'rule' Bytecode(-> label(Position, Name)):
        Identifier(-> Name) @(-> Position) ":"

    'rule' Bytecode(-> register(Position, Name, Type)):
        "register" @(-> Position) Identifier(-> Name) OptionalTypeClause(-> Type)

    'rule' Bytecode(-> opcode(Position, Opcode, Arguments)):
        NAME_LITERAL(-> Opcode) @(-> Position) OptionalExpressionList(-> Arguments)

    'rule' Bytecode(-> opcode(Position, Opcode, Arguments)):
        CustomKeywords(-> OpcodeString) @(-> Position) OptionalExpressionList(-> Arguments)
        MakeNameLiteral(OpcodeString -> Opcode)

--------------------------------------------------------------------------------
-- Expression Syntax
--------------------------------------------------------------------------------

'action' ProcessOperatorExpression(-> EXPRESSION)

    'rule' ProcessOperatorExpression(-> invoke(Position, Method, Arguments)):
        PopOperatorExpression(-> Position, MethodIndex, Arity)
        ProcessOperatorExpressionChildren(Arity -> Arguments)
        CustomInvokeLists(MethodIndex -> Method)

    'rule' ProcessOperatorExpression(-> Expr):
        PopOperatorExpressionArgument(-> Expr)
        
'action' ProcessOperatorExpressionChildren(INT -> EXPRESSIONLIST)

    'rule' ProcessOperatorExpressionChildren(Arity -> nil):
        eq(Arity, 0)

    'rule' ProcessOperatorExpressionChildren(Arity -> expressionlist(Head, Tail)):
        ProcessOperatorExpression(-> Head)
        ProcessOperatorExpressionChildren(Arity - 1 -> Tail)

'nonterm' Expression(-> EXPRESSION)

    'rule' Expression(-> logicalor(Position, Left, Right)):
        Expression(-> Left) "or" @(-> Position) AndExpression(-> Right)
        
    'rule' Expression(-> Expr)
        AndExpression(-> Expr)
        
'nonterm' AndExpression(-> EXPRESSION)

    'rule' AndExpression(-> logicaland(Position, Left, Right)):
        AndExpression(-> Left) "and" @(-> Position) NormalExpression(-> Right)
    
    'rule' AndExpression(-> Result):
        NormalExpression(-> Result)
        
'nonterm' NormalExpression(-> EXPRESSION)

    'rule' NormalExpression(-> Result):
        FlatExpression(-> Sentinal)
        ReorderOperatorExpression(Sentinal)
        ProcessOperatorExpression(-> Result)

----------

'action' min(INT, INT -> INT)
    'rule' min(A, B -> A):
        le(A, B)
    'rule' min(A, B -> B):

'nonterm' FlatExpression(-> INT)

    'rule' FlatExpression(-> Sentinal):
        FlatExpressionTerm(-> Sentinal) FlatExpressionBinaryOperator(-> _) FlatExpression(-> _)
        
    'rule' FlatExpression(-> Sentinal):
        FlatExpressionTerm(-> Sentinal)
        
'nonterm' FlatExpressionTerm(-> INT)

    'rule' FlatExpressionTerm(-> Sentinal):
        FlatExpressionPrefixOperators(-> Sentinal1) FlatExpressionOperand(-> Sentinal2) FlatExpressionPostfixOperators(-> _)
        min(Sentinal1, Sentinal2 -> Sentinal)
        
'nonterm' FlatExpressionPrefixOperators(-> INT)

    'rule' FlatExpressionPrefixOperators(-> Sentinal):
        FlatExpressionPrefixOperator(-> Sentinal) FlatExpressionPrefixOperators(-> _)
        
    'rule' FlatExpressionPrefixOperators(-> 10000):
        -- nothing

'nonterm' FlatExpressionPostfixOperators(-> INT)

    'rule' FlatExpressionPostfixOperators(-> Sentinal):
        FlatExpressionPostfixOperator(-> Sentinal) FlatExpressionPostfixOperators(-> _)
        
    'rule' FlatExpressionPostfixOperators(-> 10000):
        -- nothing
        
'nonterm' FlatExpressionPrefixOperator(-> INT)
        
    'rule' FlatExpressionPrefixOperator(-> Sentinal):
        CustomPrefixOperators(-> Sentinal)

'nonterm' FlatExpressionPostfixOperator(-> INT)

    'rule' FlatExpressionPostfixOperator(-> Sentinal):
        CustomPostfixOperators(-> Sentinal)

'nonterm' FlatExpressionBinaryOperator(-> INT)
        
    'rule' FlatExpressionBinaryOperator(-> Sentinal):
        CustomBinaryOperators(-> Sentinal)

'nonterm' FlatExpressionOperand(-> INT)
        
    'rule' FlatExpressionOperand(-> Sentinal)
        TermExpression(-> Term)
        PushOperatorExpressionOperand(Term -> Sentinal)
        
    'rule' FlatExpressionOperand(-> Sentinal):
        CustomTerms(-> Term)
        PushOperatorExpressionOperand(Term -> Sentinal)

----------

'nonterm' TermExpression(-> EXPRESSION)

    'rule' TermExpression(-> Constant):
        ConstantTermExpression(-> Constant)

    'rule' TermExpression(-> slot(Position, Name)):
        QualifiedId(-> Name) @(-> Position)
        
    'rule' TermExpression(-> result(Position)):
        "the" @(-> Position) "result"

    --'rule' TermExpression(-> as(Position, Value, Type)):
    --    TermExpression(-> Value) "as" @(-> Position) Type(-> Type)

    'rule' TermExpression(-> call(Position, Handler, Arguments)):
        QualifiedId(-> Handler) @(-> Position) "(" OptionalExpressionList(-> Arguments) ")"

    'rule' TermExpression(-> list(Position, List)):
        "[" @(-> Position) OptionalExpressionList(-> List) "]"

    'rule' TermExpression(-> array(Position, Pairs)):
        "{" @(-> Position) OptionalExpressionArray(-> Pairs) "}"

    'rule' TermExpression(-> Expression):
        "(" Expression(-> Expression) ")"

/*'nonterm' ConstantTermExpressionList(-> EXPRESSIONLIST)

    'rule' ConstantTermExpressionList(-> expressionlist(Head, Tail)):
        ConstantTermExpression(-> Head) "," ConstantTermExpressionList(-> Tail)
        
    'rule' ConstantTermExpressionList(-> expressionlist(Tail, nil)):
        ConstantTermExpression(-> Tail)*/

'nonterm' ConstantTermExpression(-> EXPRESSION)

    'rule' ConstantTermExpression(-> undefined(Position)):
        "nothing" @(-> Position)

    'rule' ConstantTermExpression(-> true(Position)):
        "true" @(-> Position)

    'rule' ConstantTermExpression(-> false(Position)):
        "false" @(-> Position)

    'rule' ConstantTermExpression(-> unsignedinteger(Position, Value)):
        INTEGER_LITERAL(-> Value) @(-> Position)

    'rule' ConstantTermExpression(-> real(Position, Value)):
        DOUBLE_LITERAL(-> Value) @(-> Position)

    'rule' ConstantTermExpression(-> string(Position, Value)):
        StringLiteral(-> Value) @(-> Position)
        
/*    'rule' ConstantTermExpression(-> list(Position, Value)):
        "[" @(-> Position) ConstantTermExpressionList(-> Value) "]"
        
    'rule' ConstantTermExpression(-> list(Position, nil)):
        "[" @(-> Position) "]"*/

----------

'nonterm' OptionalExpressionList(-> EXPRESSIONLIST)

    'rule' OptionalExpressionList(-> List):
        ExpressionList(-> List)
    
    'rule' OptionalExpressionList(-> nil)
        -- empty

'nonterm' ExpressionList(-> EXPRESSIONLIST)

    'rule' ExpressionList(-> expressionlist(Head, Tail)):
        Expression(-> Head) "," ExpressionList(-> Tail)
        
    'rule' ExpressionList(-> expressionlist(Expression, nil)):
        Expression(-> Expression)

'nonterm' ExpressionListAsExpression(-> EXPRESSION)

    'rule' ExpressionListAsExpression(-> list(Position, List)):
        ExpressionList(-> List) @(-> Position)

----------

'nonterm' OptionalExpressionArray(-> EXPRESSIONLIST)

    'rule' OptionalExpressionArray(-> Pairs):
        ExpressionArray(-> Pairs)

    'rule' OptionalExpressionArray(-> nil):
        -- empty

'nonterm' ExpressionArray(-> EXPRESSIONLIST)

    'rule' ExpressionArray(-> expressionlist(Head, Tail)):
        ExpressionArrayEntry(-> Head) "," ExpressionArray(-> Tail)

    'rule' ExpressionArray(-> expressionlist(Head, nil)):
        ExpressionArrayEntry(-> Head)

'nonterm' ExpressionArrayEntry(-> EXPRESSION)

    'rule' ExpressionArrayEntry(-> pair(Position, Key, Value)):
        Expression(-> Key) ":" @(-> Position) Expression(-> Value)

--------------------------------------------------------------------------------
-- Syntax Syntax
--------------------------------------------------------------------------------

'nonterm' Syntax(-> SYNTAX)

    'rule' Syntax(-> Syntax):
        AlternateSyntax(-> Syntax)

'nonterm' AlternateSyntax(-> SYNTAX)

    'rule' AlternateSyntax(-> alternate(Position, Left, Right)):
        AlternateSyntax(-> Left) "|" @(-> Position) ConcatenateSyntax(-> Right)

    'rule' AlternateSyntax(-> Syntax)
        ConcatenateSyntax(-> Syntax)
        
'nonterm' ConcatenateSyntax(-> SYNTAX)

    'rule' ConcatenateSyntax(-> concatenate(Position, Left, Right)):
        ConcatenateSyntax(-> Left) @(-> Position) AtomicSyntax(-> Right)

    'rule' ConcatenateSyntax(-> Syntax)
        AtomicSyntax(-> Syntax)
        
'nonterm' AtomicSyntax(-> SYNTAX)

    'rule' AtomicSyntax(-> repeat(Position, Element)):
        "{" @(-> Position) Syntax(-> Element) "}"

    'rule' AtomicSyntax(-> list(Position, Element, Delimiter)):
        "{" @(-> Position) Syntax(-> Element) "," Syntax(-> Delimiter) "}"
        
    'rule' AtomicSyntax(-> optional(Position, Operand)):
        "[" @(-> Position) Syntax(-> Operand) "]"
        
    'rule' AtomicSyntax(-> keyword(Position, Value)):
        StringLiteral(-> Value) @(-> Position)

    'rule' AtomicSyntax(-> unreservedkeyword(Position, Value)):
        StringLiteral(-> Value) @(-> Position) "!"
        
    'rule' AtomicSyntax(-> rule(Position, Name)):
        "<" @(-> Position) Identifier(-> Name) ">"
        
    'rule' AtomicSyntax(-> markedrule(Position, Variable, Name)):
        "<" @(-> Position) Identifier(-> Variable) ":" Identifier(-> Name) ">"
        
    'rule' AtomicSyntax(-> mark(Position, Variable, Value)):
        "<" @(-> Position) Identifier(-> Variable) "=" Constant(-> Value) ">"
        
    'rule' AtomicSyntax(-> Syntax):
        "(" Syntax(-> Syntax) ")"

'nonterm' OptionalConstantList(-> SYNTAXCONSTANTLIST)

    'rule' OptionalConstantList(-> List):
        ConstantList(-> List)
        
    'rule' OptionalConstantList(-> nil):
        -- empty

'nonterm' ConstantList(-> SYNTAXCONSTANTLIST)

    'rule' ConstantList(-> constantlist(Head, Tail)):
        Constant(-> Head) "," ConstantList(-> Tail)

    'rule' ConstantList(-> constantlist(Head, nil)):
        Constant(-> Head)

'nonterm' Constant(-> SYNTAXCONSTANT)

    'rule' Constant(-> undefined(Position)):
        "nothing" @(-> Position)
        
    'rule' Constant(-> true(Position)):
        "true" @(-> Position)
        
    'rule' Constant(-> false(Position)):
        "false" @(-> Position)

    'rule' Constant(-> integer(Position, Value)):
        INTEGER_LITERAL(-> Value) @(-> Position)

    'rule' Constant(-> integer(Position, Value)):
        "-" INTEGER_LITERAL(-> PositiveValue) @(-> Position)
        where(-PositiveValue -> Value)

    'rule' Constant(-> integer(Position, Value)):
        "+" INTEGER_LITERAL(-> Value) @(-> Position)

    'rule' Constant(-> real(Position, Value)):
        DOUBLE_LITERAL(-> Value) @(-> Position)

    'rule' Constant(-> real(Position, PosValue)):
        "-" DOUBLE_LITERAL(-> Value) @(-> Position)
        NegateReal(Value -> PosValue)

    'rule' Constant(-> real(Position, Value)):
        "+" DOUBLE_LITERAL(-> Value) @(-> Position)

    'rule' Constant(-> string(Position, Value)):
        StringLiteral(-> Value) @(-> Position)

    'rule' Constant(-> variable(Position, Value)):
        Identifier(-> Value) @(-> Position)

    'rule' Constant(-> indexedvariable(Position, Value, Index)):
        Identifier(-> Value) @(-> Position) "[" INTEGER_LITERAL(-> Index) "]"
        
    'rule' Constant(-> variable(Position, Value)):
        "output" @(-> Position)
        MakeNameLiteral("output" -> Identifier)
		AssignId(Position, Identifier, nil -> Value)

    'rule' Constant(-> variable(Position, Value)):
        "input" @(-> Position)
        MakeNameLiteral("input" -> Identifier)
        AssignId(Position, Identifier, nil -> Value)

--------------------------------------------------------------------------------
-- Identifier Syntax
--------------------------------------------------------------------------------

'nonterm' Identifier(-> ID)

    'rule' Identifier(-> Id):
        NAME_LITERAL(-> Identifier) @(-> Position)
		AssignId(Position, Identifier, nil -> Id)

    'rule' Identifier(-> Id):
        "iterator" @(-> Position)
        MakeNameLiteral("iterator" -> Identifier)
		AssignId(Position, Identifier, nil -> Id)
        
    'rule' Identifier(-> Id):
        CustomKeywords(-> String) @(-> Position)
        MakeNameLiteral(String -> Identifier)
		AssignId(Position, Identifier, nil -> Id)

'nonterm' StringyIdentifier(-> ID)

    'rule' StringyIdentifier(-> Id):
        StringLiteral(-> String) @(-> Position)
        MakeNameLiteral(String -> Identifier)
		AssignId(Position, Identifier, nil -> Id)

'nonterm' IdentifierList(-> IDLIST)

    'rule' IdentifierList(-> idlist(Head, Tail)):
        QualifiedId(-> Head) "," IdentifierList(-> Tail)
        
    'rule' IdentifierList(-> idlist(Id, nil)):
        QualifiedId(-> Id)

'nonterm' QualifiedId(-> ID)

	'rule' QualifiedId(-> Id):
		NAME_LITERAL(-> Identifier) @(-> Position)
		QualifiedNameToId(Position, Identifier -> Id)
		
'action' QualifiedNameToId(POS, NAME -> ID)

	'rule' QualifiedNameToId(Position, Name -> Id):		
		ContainsNamespaceOperator(Name)
		SplitNamespace(Name -> Namespace, Identifier)
		QualifiedNameToOptionalId(Position, Namespace -> NamespaceId)
		AssignId(Position, Identifier, NamespaceId -> Id)

	'rule' QualifiedNameToId(Position, Name -> Id)
		AssignId(Position, Name, nil -> Id)

'action' QualifiedNameToOptionalId(POS, NAME -> OPTIONALID)

	'rule' QualifiedNameToOptionalId(Position, Name -> id(Id))
		QualifiedNameToId(Position, Name -> Id)

'action' AssignId(POS, NAME, OPTIONALID -> ID)
	
	'rule' AssignId(Position, Identifier, Namespace -> Id)
		Id::ID
        Id'Position <- Position
        Id'Name <- Identifier
        Id'Meaning <- nil
        Id'Namespace <- Namespace
        (|
        	ContainsNamespaceOperator(Identifier)
        	Error_IllegalNamespaceOperator(Position)
		||
        |)
        
'action' GetQualifiedName(ID -> NAME)

	'rule' GetQualifiedName(Id -> Name):
		Id'Namespace -> Namespace
		ResolveIdInNamespace(Namespace, Id -> Name)

'action' ResolveIdInNamespace(OPTIONALID, ID -> NAME)

	'rule' ResolveIdInNamespace(id(NamespaceId), Id -> Name):
		Id'Name -> UnqualifiedName
		GetQualifiedName(NamespaceId -> Namespace)
		ConcatenateNameParts(Namespace, UnqualifiedName -> Name)

	'rule' ResolveIdInNamespace(nil, Id -> Name):
		Id'Name -> Name

'condition' ResolveNamespace(OPTIONALID -> NAME)

	'rule' ResolveNamespace(id(NamespaceId) -> Name):
		GetQualifiedName(NamespaceId -> Name)

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

'action' DefineCustomInvokeList(INT, INVOKELIST)
'action' LookupCustomInvokeList(INT -> INVOKELIST)

'action' CustomInvokeLists(INT -> INVOKELIST)
    'rule' CustomInvokeLists(Index -> List):
        LookupCustomInvokeList(Index -> List)

'action' MakeCustomInvokeMethodArgs1(MODE, INT -> INVOKESIGNATURE)
    'rule' MakeCustomInvokeMethodArgs1(Mode, Index -> invokesignature(Mode, Index, nil)):
    
'action' MakeCustomInvokeMethodArgs2(MODE, INT, MODE, INT -> INVOKESIGNATURE)
    'rule' MakeCustomInvokeMethodArgs2(Mode1, Index1, Mode2, Index2 -> invokesignature(Mode1, Index1, invokesignature(Mode2, Index2, nil))):

'action' MakeCustomInvokeMethodArgs3(MODE, INT, MODE, INT, MODE, INT, -> INVOKESIGNATURE)
    'rule' MakeCustomInvokeMethodArgs3(Mode1, Index1, Mode2, Index2, Mode3, Index3 -> invokesignature(Mode1, Index1, invokesignature(Mode2, Index2, invokesignature(Mode3, Index3, nil)))):

'action' MakeCustomInvokeMethodList(STRING, INVOKEMETHODTYPE, INVOKESIGNATURE, INVOKEMETHODLIST -> INVOKEMETHODLIST)
    'rule' MakeCustomInvokeMethodList(Name, Type, Signature, Previous -> methodlist(Name, Type, Signature, Previous))

'action' MakeCustomInvokeList(STRING, STRING, INVOKEMETHODLIST, INVOKELIST -> INVOKELIST)
    'rule' MakeCustomInvokeList(Name, ModuleName, Methods, Previous -> List)
        Info::INVOKEINFO
        Info'Index <- -1
        Info'ModuleIndex <- -1
        Info'Name <- Name
        Info'ModuleName <- ModuleName
        Info'Methods <- Methods
        where(invokelist(Info, Previous) -> List)

--*--*--*--*--*--*--*--

'action' InitializeCustomInvokeLists()
    'rule' InitializeCustomInvokeLists():
        -- nothing
'nonterm' CustomStatements(-> STATEMENT)
    'rule' CustomStatements(-> nil):
        "THISCANNEVERHAPPEN"
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





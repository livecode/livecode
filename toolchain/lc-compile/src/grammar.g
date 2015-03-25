/* Copyright (C) 2003-2013 Runtime Revolution Ltd.
 
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
        (|
            IsBootstrapCompile()
            BootstrapCompile(Modules)
        ||
            Compile(Modules)
        |)
    |)

---------

'action' Compile(MODULELIST)

    'rule' Compile(Modules):
        InitializeBind
        BindModules(Modules, Modules)
        CheckModules(Modules)
        (|
            ErrorsDidOccur()
        ||
            where(Modules -> modulelist(Head, _))
            Generate(Head)
        |)

'action' BootstrapCompile(MODULELIST)

    'rule' BootstrapCompile(Modules):
        BindModules(Modules, Modules)
        CheckModules(Modules)
        (|
            ErrorsDidOccur()
        ||
            GenerateSyntaxForModules(Modules)
            (|
                ErrorsDidOccur()
            ||
                GenerateSyntaxRules()
            |)
        |)

'action' BindModules(MODULELIST, MODULELIST)

    'rule' BindModules(modulelist(Head, Tail), Imports):
        InitializeBind
        
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
        InitializeSyntax
        
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

    'rule' Module(-> module(Position, module, Name, Imports, Definitions)):
        OptionalSeparator
        "module" @(-> Position) Identifier(-> Name) Separator
        Imports(-> Imports)
        Definitions(-> Definitions)
        "end" "module" OptionalSeparator
        END_OF_UNIT

    'rule' Module(-> module(Position, widget, Name, Imports, sequence(PreImportDefs, Definitions))):
        OptionalSeparator
        "widget" @(-> Position) Identifier(-> Name) Separator
        PreImportMetadata(-> PreImportDefs)
        Imports(-> Imports)
        Definitions(-> Definitions)
        "end" "widget" OptionalSeparator
        END_OF_UNIT

    'rule' Module(-> module(Position, library, Name, Imports, sequence(PreImportDefs, Definitions))):
        OptionalSeparator
        "library" @(-> Position) Identifier(-> Name) Separator
        PreImportMetadata(-> PreImportDefs)
        Imports(-> Imports)
        Definitions(-> Definitions)
        "end" "library" OptionalSeparator
        END_OF_UNIT

    'rule' Module(-> module(Position, import, Name, Imports, Definitions)):
        OptionalSeparator
        "import" "module" @(-> Position) Identifier(-> Name) Separator
        Imports(-> Imports)
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

    'rule' ImportDefinition(-> type(Position, public, Id, foreign(Position, ""))):
        "foreign" @(-> Position) "type" Identifier(-> Id)
    
    'rule' ImportDefinition(-> type(Position, public, Id, handler(Position, Signature))):
        "handler" @(-> Position) "type" Identifier(-> Id) Signature(-> Signature)
        
    'rule' ImportDefinition(-> type(Position, public, Id, Type)):
        "type" @(-> Position) Identifier(-> Id) "is" Type(-> Type)

    'rule' ImportDefinition(-> constant(Position, public, Id, nil)):
        "constant" @(-> Position) Identifier(-> Id)

    'rule' ImportDefinition(-> variable(Position, public, Id, Type)):
        "variable" @(-> Position) Identifier(-> Id) "as" Type(-> Type)

    'rule' ImportDefinition(-> handler(Position, public, Id, normal, Signature, nil, nil)):
        "handler" @(-> Position) Identifier(-> Id) Signature(-> Signature)

    'rule' ImportDefinition(-> foreignhandler(Position, public, Id, Signature, "")):
        "foreign" "handler" @(-> Position) Identifier(-> Id) Signature(-> Signature)

--------------------------------------------------------------------------------
-- Metadata Syntax
--------------------------------------------------------------------------------

'nonterm' PreImportMetadata(-> DEFINITION)

    'rule' PreImportMetadata(-> sequence(Left, Right)):
        Metadata(-> Left) Separator
        PreImportMetadata(-> Right)
        where(Left -> metadata(Position, _, _))
        Warning_MetadataClausesShouldComeAfterUseClauses(Position)

    'rule' PreImportMetadata(-> nil):
        -- do nothing

'nonterm' Metadata(-> DEFINITION)

    'rule' Metadata(-> metadata(Position, Key, Value)):
        "metadata" @(-> Position) StringOrNameLiteral(-> Key) "is" STRING_LITERAL(-> Value)
        
--------------------------------------------------------------------------------
-- Import Syntax
--------------------------------------------------------------------------------

'nonterm' Imports(-> IMPORT)

    'rule' Imports(-> sequence(Head, Tail)):
        Import(-> Head) Separator
        Imports(-> Tail)
        
    'rule' Imports(-> nil):
        -- empty
        
'nonterm' Import(-> IMPORT)

    'rule' Import(-> ImportList):
        "use" @(-> Position) IdentifierList(-> Identifiers)
        ExpandImports(Position, Identifiers -> ImportList)

'action' ExpandImports(POS, IDLIST -> IMPORT)

    'rule' ExpandImports(Position, idlist(Id, nil) -> import(Position, Id)):
        Id'Name -> Name
        GetStringOfNameLiteral(Name -> NameString)
        (|
            AddImportedModuleFile(NameString)
        ||
            Error_UnableToFindImportedModule(Position, Name)
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

    'rule' VariableDefinition(-> contextvariable(Position, Access, Name, Type, Default)):
        Access(-> Access) "context" @(-> Position) "variable" Identifier(-> Name) OptionalTypeClause(-> Type) "default" Expression(-> Default)
        
'nonterm' OptionalTypeClause(-> TYPE)

    'rule' OptionalTypeClause(-> Type):
        "as" Type(-> Type)
        
    'rule' OptionalTypeClause(-> optional(Position, any(Position))):
        @(-> Position)

---------- Type

'nonterm' TypeDefinition(-> DEFINITION)

    'rule' TypeDefinition(-> type(Position, Access, Name, Type)):
        Access(-> Access) "type" @(-> Position) Identifier(-> Name) "is" Type(-> Type)
    
    'rule' TypeDefinition(-> type(Position, Access, Name, foreign(Position, Binding))):
        Access(-> Access) "foreign" @(-> Position) "type" Identifier(-> Name) "binds" "to" STRING_LITERAL(-> Binding)

        
    'rule' TypeDefinition(-> type(Position, Access, Name, record(Position, Base, Fields))):
        Access(-> Access) "record" @(-> Position) "type" Identifier(-> Name) OptionalBaseType(-> Base) Separator
            RecordFields(-> Fields)
        "end" "type"
        
    'rule' TypeDefinition(-> type(Position, Access, Name, enum(Position, Base, Fields))):
        Access(-> Access) "enum" @(-> Position) "type" Identifier(-> Name) OptionalBaseType(-> Base) Separator
            EnumFields(-> Fields)
        "end" "type"
        
    'rule' TypeDefinition(-> type(Position, Access, Name, handler(Position, Signature))):
        Access(-> Access) "handler" @(-> Position) "type" Identifier(-> Name) Signature(-> Signature)

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

    'rule' HandlerDefinition(-> handler(Position, Access, Name, normal, Signature, nil, Body)):
        Access(-> Access) "handler" @(-> Position) Identifier(-> Name) Signature(-> Signature) Separator
            Statements(-> Body)
        "end" "handler"
        
    'rule' HandlerDefinition(-> handler(Position, Access, Name, context, Signature, nil, Body)):
        Access(-> Access) "context" @(-> Position) "handler" Identifier(-> Name) Signature(-> Signature) Separator
            Statements(-> Body)
        "end" "handler"
            --'rule' HandlerDefinition(-> handler(Position, Access, Name, Signature, nil, Body)):
    --    Access(-> Access) "handler" @(-> Position) Identifier(-> Name) Signature(-> Signature) Separator
    --        Definitions(-> Definitions)
    --    "begin"
    --        Statements(-> Body)
    --    "end" "handler"
        
    'rule' HandlerDefinition(-> foreignhandler(Position, Access, Name, Signature, Binding)):
        Access(-> Access) "foreign" "handler" @(-> Position) Identifier(-> Name) Signature(-> Signature) "binds" "to" STRING_LITERAL(-> Binding)

'nonterm' Signature(-> SIGNATURE)

    'rule' Signature(-> signature(Parameters, Result)):
        "(" OptionalParameterList(-> Parameters) ")" OptionalReturnsClause(-> Result)

'nonterm' OptionalReturnsClause(-> TYPE)

    'rule' OptionalReturnsClause(-> Type):
        "as" @(-> Position) TypeNoUndefined(-> Type)
        Warning_UsingAsForHandlerReturnTypeDeprecated(Position)
        
    'rule' OptionalReturnsClause(-> undefined(Position)):
        "as" "undefined" @(-> Position)
        Warning_UsingAsUndefinedForVoidHandlerReturnTypeDeprecated(Position)

    'rule' OptionalReturnsClause(-> Type)
        "returns" @(-> Position) TypeNoUndefined(-> Type)
        
    'rule' OptionalReturnsClause(-> undefined(Position))
        "returns" "nothing" @(-> Position)
    
    'rule' OptionalReturnsClause(-> optional(Position, any(Position)))
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

    'rule' SyntaxDefinition(-> syntax(Position, public, Name, Class, Syntax, Methods)):
        "syntax" @(-> Position) Identifier(-> Name) SyntaxClass(-> Class) Separator
            Syntax(-> Syntax) Separator
        "begin" Separator
            SyntaxMethods(-> Methods)
        "end" "syntax"
        
'nonterm' SyntaxClass(-> SYNTAXCLASS)

    'rule' SyntaxClass(-> statement):
        "is" "statement"

    'rule' SyntaxClass(-> expression):
        "is" "expression"

    'rule' SyntaxClass(-> iterator):
        "is" "iterator"

    'rule' SyntaxClass(-> prefix(Precedence)):
        "is" "prefix" "operator" "with" "precedence" INTEGER_LITERAL(-> Precedence)

    'rule' SyntaxClass(-> postfix(Precedence)):
        "is" "postfix" "operator" "with" "precedence" INTEGER_LITERAL(-> Precedence)

    'rule' SyntaxClass(-> binary(Assoc, Precedence)):
        "is" SyntaxAssoc(-> Assoc) "binary" "operator" "with" "precedence" INTEGER_LITERAL(-> Precedence)

    'rule' SyntaxClass(-> phrase):
        "is" "phrase"

'nonterm' SyntaxAssoc(-> SYNTAXASSOC)

    'rule' SyntaxAssoc(-> neutral):
        "neutral"
        
    'rule' SyntaxAssoc(-> left):
        "left"
        
    'rule' SyntaxAssoc(-> right):
        "right"

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

'nonterm' TypeNoUndefined(-> TYPE)

    'rule' TypeNoUndefined(-> named(Position, Name)):
        Identifier(-> Name) @(-> Position)
        
    'rule' TypeNoUndefined(-> optional(Position, Base)):
        "optional" @(-> Position) Type(-> Base)

    'rule' TypeNoUndefined(-> any(Position)):
        "any" @(-> Position)

    'rule' TypeNoUndefined(-> boolean(Position)):
        "Boolean" @(-> Position)
    'rule' TypeNoUndefined(-> boolean(Position)):
        "boolean" @(-> Position)
        Warning_DeprecatedTypeName(Position, "Boolean")

    'rule' TypeNoUndefined(-> integer(Position)):
        "Integer" @(-> Position)
    'rule' TypeNoUndefined(-> integer(Position)):
        "integer" @(-> Position)
        Warning_DeprecatedTypeName(Position, "Integer")

    'rule' TypeNoUndefined(-> real(Position)):
        "Real" @(-> Position)
    'rule' TypeNoUndefined(-> real(Position)):
        "real" @(-> Position)
        Warning_DeprecatedTypeName(Position, "Real")

    'rule' TypeNoUndefined(-> number(Position)):
        "Number" @(-> Position)
    'rule' TypeNoUndefined(-> number(Position)):
        "number" @(-> Position)
        Warning_DeprecatedTypeName(Position, "Number")

    'rule' TypeNoUndefined(-> string(Position)):
        "String" @(-> Position)
    'rule' TypeNoUndefined(-> string(Position)):
        "string" @(-> Position)
        Warning_DeprecatedTypeName(Position, "String")

    'rule' TypeNoUndefined(-> data(Position)):
        "Data" @(-> Position)
    'rule' TypeNoUndefined(-> data(Position)):
        "data" @(-> Position)
        Warning_DeprecatedTypeName(Position, "Data")

    'rule' TypeNoUndefined(-> array(Position)):
        "Array" @(-> Position)
    'rule' TypeNoUndefined(-> array(Position)):
        "array" @(-> Position)
        Warning_DeprecatedTypeName(Position, "Array")

    'rule' TypeNoUndefined(-> list(Position, ElementType)):
        "List" @(-> Position) OptionalElementType(-> ElementType)
    'rule' TypeNoUndefined(-> list(Position, ElementType)):
        "list" @(-> Position) OptionalElementType(-> ElementType)
        Warning_DeprecatedTypeName(Position, "List")

'nonterm' Type(-> TYPE)

    'rule' Type(-> Type):
        TypeNoUndefined(-> Type)

    'rule' Type(-> undefined(Position)):
        "undefined" @(-> Position)
        Warning_UndefinedTypeDeprecated(Position)

'nonterm' OptionalElementType(-> TYPE)

    'rule' OptionalElementType(-> Type)
        "of" Type(-> Type)
        
    'rule' OptionalElementType(-> optional(Position, any(Position))):
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
        Identifier(-> Handler) @(-> Position) "(" OptionalExpressionList(-> Arguments) ")"

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
        Identifier(-> Name) @(-> Position)
        
    'rule' TermExpression(-> result(Position)):
        "the" @(-> Position) "result"

    --'rule' TermExpression(-> as(Position, Value, Type)):
    --    TermExpression(-> Value) "as" @(-> Position) Type(-> Type)

    'rule' TermExpression(-> call(Position, Handler, Arguments)):
        Identifier(-> Handler) @(-> Position) "(" OptionalExpressionList(-> Arguments) ")"

    'rule' TermExpression(-> list(Position, List)):
        "[" @(-> Position) OptionalExpressionList(-> List) "]"

    'rule' TermExpression(-> Expression):
        "(" Expression(-> Expression) ")"

/*'nonterm' ConstantTermExpressionList(-> EXPRESSIONLIST)

    'rule' ConstantTermExpressionList(-> expressionlist(Head, Tail)):
        ConstantTermExpression(-> Head) "," ConstantTermExpressionList(-> Tail)
        
    'rule' ConstantTermExpressionList(-> expressionlist(Tail, nil)):
        ConstantTermExpression(-> Tail)*/

'nonterm' ConstantTermExpression(-> EXPRESSION)

    'rule' ConstantTermExpression(-> undefined(Position)):
        "undefined" @(-> Position)

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
        STRING_LITERAL(-> Value) @(-> Position)

    'rule' AtomicSyntax(-> unreservedkeyword(Position, Value)):
        STRING_LITERAL(-> Value) @(-> Position) "!"
        
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
        "undefined" @(-> Position)
        
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
        Value::ID
        Value'Position <- Position
        Value'Name <- Identifier
        Value'Meaning <- nil

    'rule' Constant(-> variable(Position, Value)):
        "input" @(-> Position)
        MakeNameLiteral("input" -> Identifier)
        Value::ID
        Value'Position <- Position
        Value'Name <- Identifier
        Value'Meaning <- nil

--------------------------------------------------------------------------------
-- Identifier Syntax
--------------------------------------------------------------------------------

'nonterm' Identifier(-> ID)

    'rule' Identifier(-> Id):
        NAME_LITERAL(-> Identifier) @(-> Position)
        Id::ID
        Id'Position <- Position
        Id'Name <- Identifier
        Id'Meaning <- nil
        
    'rule' Identifier(-> Id):
        "iterator" @(-> Position)
        MakeNameLiteral("iterator" -> Identifier)
        Id::ID
        Id'Position <- Position
        Id'Name <- Identifier
        
    'rule' Identifier(-> Id):
        CustomKeywords(-> String) @(-> Position)
        MakeNameLiteral(String -> Identifier)
        Id::ID
        Id'Position <- Position
        Id'Name <- Identifier

'nonterm' StringyIdentifier(-> ID)

    'rule' StringyIdentifier(-> Id):
        StringLiteral(-> String) @(-> Position)
        MakeNameLiteral(String -> Identifier)
        Id::ID
        Id'Position <- Position
        Id'Name <- Identifier
        Id'Meaning <- nil

'nonterm' IdentifierList(-> IDLIST)

    'rule' IdentifierList(-> idlist(Head, Tail)):
        Identifier(-> Head) "," IdentifierList(-> Tail)
        
    'rule' IdentifierList(-> idlist(Id, nil)):
        Identifier(-> Id)

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
'action' CustomInvokeLists(INT -> INVOKELIST)
    'rule' CustomInvokeLists(_ -> nil):
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





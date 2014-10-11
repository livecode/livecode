'module' grammar

'use'
    support
    types
    bind
    check
    syntax

--------------------------------------------------------------------------------

'root'
    Parse(-> Modules)
    (|
        IsBootstrapCompile()
        BootstrapCompile(Modules)
    ||
        -- Compile(Modules)
    |)

---------

'action' BootstrapCompile(MODULELIST)

    'rule' BootstrapCompile(Modules):
        BindModules(Modules)
        CheckModules(Modules)
        (|
            ErrorsDidOccur()
        ||
            GenerateSyntaxForModules(Modules)
            GenerateSyntaxRules()
        |)

'action' BindModules(MODULELIST)

    'rule' BindModules(modulelist(Head, Tail)):
        InitializeBind
        
        Bind(Head)
        BindModules(Tail)
        
    'rule' BindModules(nil):
        -- empty

'action' CheckModules(MODULELIST)

    'rule' CheckModules(modulelist(Head, Tail)):
        Check(Head)
        CheckModules(Tail)
        
    'rule' CheckModules(nil):
        -- empty
        
'action' GenerateSyntaxForModules(MODULELIST)

    'rule' GenerateSyntaxForModules(modulelist(Head, Tail)):
        InitializeSyntax
        
        GenerateSyntax(Head)
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

    'rule' Module(-> module(Position, Name, Imports, Definitions)):
        OptionalSeparator
        "module" @(-> Position) Identifier(-> Name) Separator
        Imports(-> Imports)
        Definitions(-> Definitions)
        "end" "module"
        END_OF_UNIT

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
        -- empty
        
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
        
    'rule' OptionalTypeClause(-> nil):
        -- empty

---------- Type

'nonterm' TypeDefinition(-> DEFINITION)

    'rule' TypeDefinition(-> type(Position, Access, Name, Type)):
        Access(-> Access) "type" @(-> Position) Identifier(-> Name) "is" Type(-> Type)

---------- Handler

'nonterm' HandlerDefinition(-> DEFINITION)

    'rule' HandlerDefinition(-> handler(Position, Access, Name, Signature, nil, Body)):
        Access(-> Access) "handler" @(-> Position) Identifier(-> Name) Signature(-> Signature) Separator
            Statements(-> Body)
        "end" "handler"
        
    'rule' HandlerDefinition(-> handler(Position, Access, Name, Signature, nil, Body)):
        Access(-> Access) "handler" @(-> Position) Identifier(-> Name) Signature(-> Signature) Separator
            Definitions(-> Definitions)
        "begin"
            Statements(-> Body)
        "end" "handler"    
        
    'rule' HandlerDefinition(-> foreignhandler(Position, Access, Name, Signature, Binding)):
        Access(-> Access) "foreign" "handler" @(-> Position) Identifier(-> Name) Signature(-> Signature) "binds" "to" STRING_LITERAL(-> Binding)

'nonterm' Signature(-> SIGNATURE)

    'rule' Signature(-> signature(Parameters, Result)):
        "(" OptionalParameterList(-> Parameters) ")" OptionalTypeClause(-> Result)

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

    'rule' PropertyDefinition(-> property(Position, public, Name)):
        "property" @(-> Position) Identifier(-> Name)
        
---------- Event

'nonterm' EventDefinition(-> DEFINITION)

    'rule' EventDefinition(-> event(Position, public, Name)):
        "event" @(-> Position) Identifier(-> Name)

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

    'rule' SyntaxClass(-> prefix(Precedence)):
        "is" "prefix" "operator" "with" "precedence" INTEGER_LITERAL(-> Precedence)

    'rule' SyntaxClass(-> postfix(Precedence)):
        "is" "postfix" "operator" "with" "precedence" INTEGER_LITERAL(-> Precedence)

    'rule' SyntaxClass(-> binary(Assoc, Precedence)):
        "is" SyntaxAssoc(-> Assoc) "binary" "operator" "with" "precedence" INTEGER_LITERAL(-> Precedence)

    'rule' SyntaxClass(-> phrase):
        -- empty

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

'nonterm' Type(-> TYPE)

    'rule' Type(-> named(Position, Name)):
        Identifier(-> Name) @(-> Position)
        
    'rule' Type(-> bool(Position)):
        "bool" @(-> Position)

    'rule' Type(-> int(Position)):
        "int" @(-> Position)
    
    'rule' Type(-> uint(Position)):
        "uint" @(-> Position)

    'rule' Type(-> int(Position)):
        "index" @(-> Position)
    
    'rule' Type(-> uint(Position)):
        "uindex" @(-> Position)

    'rule' Type(-> double(Position)):
        "float" @(-> Position)

    'rule' Type(-> double(Position)):
        "double" @(-> Position)

    --

    'rule' Type(-> any(Position)):
        "any" @(-> Position)

    'rule' Type(-> boolean(Position)):
        "boolean" @(-> Position)

    'rule' Type(-> integer(Position)):
        "integer" @(-> Position)

    'rule' Type(-> real(Position)):
        "real" @(-> Position)

    'rule' Type(-> number(Position)):
        "number" @(-> Position)

    'rule' Type(-> string(Position)):
        "string" @(-> Position)

    'rule' Type(-> data(Position)):
        "data" @(-> Position)

    'rule' Type(-> array(Position)):
        "array" @(-> Position)

    'rule' Type(-> undefined(Position)):
        "undefined" @(-> Position)

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

    'rule' Statement(-> call(Position, Handler, Arguments)):
        Identifier(-> Handler) @(-> Position) ExpressionList(-> Arguments)
        
    'rule' Statement(-> Statement):
        CustomStatements(-> Statement)

--------------------------------------------------------------------------------
-- Expression Syntax
--------------------------------------------------------------------------------

'action' ProcessOperatorExpression(-> EXPRESSION)

    'rule' ProcessOperatorExpression(-> invoke(Position, Method, Arguments)):
        PopOperatorExpression(-> Position, Method, Arity)
        ProcessOperatorExpressionChildren(Arity -> Arguments)

    'rule' ProcessOperatorExpression(-> Expr):
        PopOperatorExpressionArgument(-> Expr)
        
'action' ProcessOperatorExpressionChildren(INT -> EXPRESSIONLIST)

    'rule' ProcessOperatorExpressionChildren(Arity -> nil):
        eq(Arity, 0)

    'rule' ProcessOperatorExpressionChildren(Arity -> expressionlist(Head, Tail)):
        ProcessOperatorExpression(-> Head)
        ProcessOperatorExpressionChildren(Arity - 1 -> Tail)

'nonterm' Expression(-> EXPRESSION)

    'rule' Expression(-> Result):
        FlatExpression
        ReorderOperatorExpression
        ProcessOperatorExpression(-> Result)

----------

'nonterm' FlatExpression

    'rule' FlatExpression:
        FlatExpressionTerm FlatExpressionBinaryOperator FlatExpression
        
    'rule' FlatExpression:
        FlatExpressionTerm
        
'nonterm' FlatExpressionTerm

    'rule' FlatExpressionTerm:
        FlatExpressionPrefixOperators FlatExpressionOperand FlatExpressionPostfixOperators
        
'nonterm' FlatExpressionPrefixOperators

    'rule' FlatExpressionPrefixOperators:
        FlatExpressionPrefixOperator FlatExpressionPrefixOperators
        
    'rule' FlatExpressionPrefixOperators:
        -- nothing

'nonterm' FlatExpressionPostfixOperators

    'rule' FlatExpressionPostfixOperators:
        FlatExpressionPostfixOperator FlatExpressionPostfixOperators
        
    'rule' FlatExpressionPostfixOperators:
        -- nothing
        
'nonterm' FlatExpressionPrefixOperator

    'rule' FlatExpressionPrefixOperator:
        "+" @(-> Position)
        PushOperatorExpressionPrefix(Position, 1, 1)
        
    'rule' FlatExpressionPrefixOperator:
        CustomPrefixOperators

'nonterm' FlatExpressionPostfixOperator

    'rule' FlatExpressionPostfixOperator:
        "is" @(-> Position) "foo"
        PushOperatorExpressionPrefix(Position, 1, 2)

    'rule' FlatExpressionPostfixOperator:
        CustomPostfixOperators

'nonterm' FlatExpressionBinaryOperator

    'rule' FlatExpressionBinaryOperator:
        "*" @(-> Position)
        PushOperatorExpressionLeftBinary(Position, 1, 3)
        
    'rule' FlatExpressionBinaryOperator:
        CustomBinaryOperators

'nonterm' FlatExpressionOperand

    'rule' FlatExpressionOperand:
        INTEGER_LITERAL(-> Value) @(-> Position)
        PushOperatorExpressionOperand(integer(Position, Value))
        
    'rule' FlatExpressionOperand:
        CustomOperands

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

    'rule' Constant(-> string(Position, Value)):
        STRING_LITERAL(-> Value) @(-> Position)

    'rule' Constant(-> name(Position, Value)):
        Identifier(-> Value) @(-> Position)

--------------------------------------------------------------------------------
-- Identifier Syntax
--------------------------------------------------------------------------------

'nonterm' Identifier(-> ID)

    'rule' Identifier(-> Id):
        NAME_LITERAL(-> Identifier) @(-> Position)
        Id::ID
        Id'Position <- Position
        Id'Name <- Identifier

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

'token' NAME_LITERAL (-> NAME)
'token' INTEGER_LITERAL (-> INT)
'token' DOUBLE_LITERAL (-> DOUBLE)
'token' STRING_LITERAL (-> STRING)

'token' SEPARATOR

'token' END_OF_UNIT
'token' NEXT_UNIT

--*--*--*--*--*--*--*--

'nonterm' CustomStatements(-> STATEMENT)
    'rule' CustomStatements(-> nil):
        "THISCANNEVERHAPPEN"
'nonterm' CustomPostfixOperators
    'rule' CustomPostfixOperators:
        "THISCANNEVERHAPPEN"
'nonterm' CustomPrefixOperators
    'rule' CustomPrefixOperators:
        "THISCANNEVERHAPPEN"
'nonterm' CustomBinaryOperators
    'rule' CustomBinaryOperators:
        "THISCANNEVERHAPPEN"
'nonterm' CustomOperands
    'rule' CustomOperands:
        "THISCANNEVERHAPPEN"



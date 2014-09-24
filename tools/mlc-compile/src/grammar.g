'module' grammar

'use'
    types

--------------------------------------------------------------------------------

'root'
    Parse(-> Module)

--------------------------------------------------------------------------------

'nonterm' Parse(-> MODULE)

    'rule' Parse(-> Module):
        Module(-> Module)

--------------------------------------------------------------------------------
-- Module Syntax
--------------------------------------------------------------------------------

'nonterm' Module(-> MODULE)

    'rule' Module(-> module(Position, Name, Imports, Definitions)):
        "module" @(-> Position) Identifier(-> Name) SEPARATOR
        Imports(-> Imports)
        Definitions(-> Definitions)
        "end" "module"

--------------------------------------------------------------------------------
-- Import Syntax
--------------------------------------------------------------------------------

'nonterm' Imports(-> IMPORT)

    'rule' Imports(-> sequence(Head, Tail)):
        Import(-> Head) SEPARATOR
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
        Definition(-> Head) SEPARATOR
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
        Access(-> Access) "handler" @(-> Position) Identifier(-> Name) Signature(-> Signature) SEPARATOR
            Statements(-> Body)
        "end" "handler"
        
    'rule' HandlerDefinition(-> handler(Position, Access, Name, Signature, nil, Body)):
        Access(-> Access) "handler" @(-> Position) Identifier(-> Name) Signature(-> Signature) SEPARATOR
            Definitions(-> Definitions)
        "begin"
            Statements(-> Body)
        "end" "handler"    
        
    'rule' HandlerDefinition(-> foreignhandler(Position, Access, Name, Signature, Binding)):
        Access(-> Access) "foreign" "handler" @(-> Position) Identifier(-> Name) Signature(-> Signature) "binds" "to" STRING_LITERAL(-> Binding)

'nonterm' Signature(-> SIGNATURE)

    'rule' Signature(-> signature(Parameters, Result)):
        "(" OptionalParameterList(-> Parameters) ")" "returns" Type(-> Result)

    'rule' Signature(-> signature(Parameters, void(Position))):
        "(" OptionalParameterList(-> Parameters) ")" "returns" "nothing" @(-> Position)
        
    'rule' Signature(-> signature(Parameters, nil)):
        "(" OptionalParameterList(-> Parameters) ")"

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

    'rule' PropertyDefinition(-> property(Position, public)):
        "property" @(-> Position)
        
---------- Event

'nonterm' EventDefinition(-> DEFINITION)

    'rule' EventDefinition(-> event(Position, public)):
        "event" @(-> Position)

---------- Syntax

'nonterm' SyntaxDefinition(-> DEFINITION)

    'rule' SyntaxDefinition(-> syntax(Position, public, Name, Class, Syntax, Methods)):
        "syntax" @(-> Position) Identifier(-> Name) SyntaxClass(-> Class) SEPARATOR
            Syntax(-> Syntax) SEPARATOR
        "begin" SEPARATOR
            SyntaxMethods(-> Methods)
        "end" "syntax"
        
'nonterm' SyntaxClass(-> SYNTAXCLASS)

    'rule' SyntaxClass(-> statement):
        "is" "statement"

    'rule' SyntaxClass(-> expression):
        "is" "expression"

    'rule' SyntaxClass(-> unary(Assoc, Precedence)):
        "is" SyntaxAssoc(-> Assoc) "unary" "operator" "with" "precedence" INTEGER_LITERAL(-> Precedence)

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

'nonterm' SyntaxMethods(-> METHODLIST)

    'rule' SyntaxMethods(-> methodlist(Head, Tail)):
        SyntaxMethod(-> Head) SEPARATOR
        SyntaxMethods(-> Tail)
        
    'rule' SyntaxMethods(-> nil):
        -- empty

'nonterm' SyntaxMethod(-> METHOD)

    'rule' SyntaxMethod(-> method(Position, Name, Arguments)):
        Identifier(-> Name) @(-> Position) "(" OptionalConstantList(-> Arguments) ")"

--------------------------------------------------------------------------------
-- Type Syntax
--------------------------------------------------------------------------------

'nonterm' Type(-> TYPE)

    'rule' Type(-> named(Position, Name)):
        Identifier(-> Name) @(-> Position)

--------------------------------------------------------------------------------
-- Statement Syntax
--------------------------------------------------------------------------------

'nonterm' Statements(-> STATEMENT)

    'rule' Statements(-> sequence(Left, Right)):
        Statement(-> Left) SEPARATOR
        Statements(-> Right)
        
    'rule' Statements(-> nil):
        -- empty
        
'nonterm' Statement(-> STATEMENT)

    'rule' Statement(-> call(Position, Handler, Arguments)):
        Identifier(-> Handler) @(-> Position) ExpressionList(-> Arguments)

--------------------------------------------------------------------------------
-- Expression Syntax
--------------------------------------------------------------------------------

'nonterm' Expression(-> EXPRESSION)

    'rule' Expression(-> integer(Position, Value)):
        INTEGER_LITERAL(-> Value) @(-> Position)

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

'nonterm' OptionalConstantList(-> CONSTANTLIST)

    'rule' OptionalConstantList(-> List):
        ConstantList(-> List)
        
    'rule' OptionalConstantList(-> nil):
        -- empty

'nonterm' ConstantList(-> CONSTANTLIST)

    'rule' ConstantList(-> constantlist(Head, Tail)):
        Constant(-> Head) "," ConstantList(-> Tail)

    'rule' ConstantList(-> constantlist(Head, nil)):
        Constant(-> Head)

'nonterm' Constant(-> CONSTANT)

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
-- Tokens
--------------------------------------------------------------------------------

'token' NAME_LITERAL (-> NAME)
'token' INTEGER_LITERAL (-> INT)
'token' DOUBLE_LITERAL (-> DOUBLE)
'token' STRING_LITERAL (-> STRING)

'token' SEPARATOR


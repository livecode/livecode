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

---------- Constant

'nonterm' ConstantDefinition(-> DEFINITION)

    'rule' ConstantDefinition(-> constant(Position, Name, Value)):
        "constant" @(-> Position) Identifier(-> Name) "is" Expression(-> Value)
        
---------- Variable

'nonterm' VariableDefinition(-> DEFINITION)

    'rule' VariableDefinition(-> variable(Position, Name, Type)):
        "variable" @(-> Position) Identifier(-> Name) OptionalTypeClause(-> Type)
        
'nonterm' OptionalTypeClause(-> TYPE)

    'rule' OptionalTypeClause(-> Type):
        "as" Type(-> Type)
        
    'rule' OptionalTypeClause(-> nil):
        -- empty

---------- Type

'nonterm' TypeDefinition(-> DEFINITION)

    'rule' TypeDefinition(-> type(Position, Name, Type)):
        "type" @(-> Position) Identifier(-> Name) "is" Type(-> Type)

---------- Handler

'nonterm' HandlerDefinition(-> DEFINITION)

    'rule' HandlerDefinition(-> handler(Position, Name, Signature, Body)):
        "handler" @(-> Position) Identifier(-> Name) Signature(-> Signature) SEPARATOR
            Statements(-> Body)
        "end" "handler"
        
    'rule' HandlerDefinition(-> foreignhandler(Position, Name, Signature, Binding)):
        "foreign" "handler" @(-> Position) Identifier(-> Name) Signature(-> Signature) "binds" "to" STRING_LITERAL(-> Binding)

'nonterm' Signature(-> SIGNATURE)

    'rule' Signature(-> signature(Parameters, Result)):
        OptionalParameterList(-> Parameters) "returns" Type(-> Result)

    'rule' Signature(-> signature(Parameters, void(Position))):
        OptionalParameterList(-> Parameters) "returns" "nothing" @(-> Position)
        
    'rule' Signature(-> signature(Parameters, nil)):
        OptionalParameterList(-> Parameters)

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

    'rule' PropertyDefinition(-> property(Position)):
        "property" @(-> Position)
        
---------- Event

'nonterm' EventDefinition(-> DEFINITION)

    'rule' EventDefinition(-> event(Position)):
        "event" @(-> Position)

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
        Statement(-> Left)
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


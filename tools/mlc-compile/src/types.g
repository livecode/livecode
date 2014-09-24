'module' types

'use'

'export'
    MODULE
    IMPORT
    DEFINITION SIGNATURE ACCESS
    TYPE
    PARAMETER MODE PARAMETERLIST
    STATEMENT
    EXPRESSION EXPRESSIONLIST
    SYNTAX SYNTAXCLASS SYNTAXASSOC CONSTANT CONSTANTLIST METHOD METHODLIST
    ID IDLIST
    NAME DOUBLE

--------------------------------------------------------------------------------

'type' MODULE
    module(Position: POS, Name: ID, Imports: IMPORT, Definitions: DEFINITION)

'type' IMPORT
    sequence(Left: IMPORT, Right: IMPORT)
    import(Position: POS, Name: ID)
    nil
    
'type' DEFINITION
    sequence(Left: DEFINITION, Right: DEFINITION)
    type(Position: POS, Access: ACCESS, Name: ID, Type: TYPE)
    constant(Position: POS, Access: ACCESS, Name: ID, Value: EXPRESSION)
    variable(Position: POS, Access: ACCESS, Name: ID, Type: TYPE)
    handler(Position: POS, Access: ACCESS, Name: ID, Signature: SIGNATURE, Definitions: DEFINITION, Body: STATEMENT)
    foreignhandler(Position: POS, Access: ACCESS, Name: ID, Signature: SIGNATURE, Binding: STRING)
    property(Position: POS, Access: ACCESS)
    event(Position: POS, Access: ACCESS)
    syntax(Position: POS, Access: ACCESS, Name: ID, Class: SYNTAXCLASS, Syntax: SYNTAX, Methods: METHODLIST)
    nil

'type' SIGNATURE
    signature(Parameters: PARAMETERLIST, ReturnType: TYPE)

'type' ACCESS
    inferred
    public
    protected
    private

'type' TYPE
    named(Position: POS, Name: ID)
    void(Position: POS)
    nil

'type' PARAMETERLIST
    parameterlist(Head: PARAMETER, Tail: PARAMETERLIST)
    nil
    
'type' PARAMETER
    parameter(Position: POS, Mode: MODE, Name: ID, Type: TYPE)
    
'type' MODE
    in
    out
    inout

'type' STATEMENT
    sequence(Left: STATEMENT, Right: STATEMENT)
    call(Position: POS, Handler: ID, Arguments: EXPRESSIONLIST)
    nil
    
'type' EXPRESSIONLIST
    expressionlist(Head: EXPRESSION, Tail: EXPRESSIONLIST)
    nil
    
'type' EXPRESSION
    integer(Position: POS, Value: INT)
    nil

'type' METHODLIST
    methodlist(Head: METHOD, Tail: METHODLIST)
    nil
    
'type' METHOD
    method(Position: POS, Name: ID, Arguments: CONSTANTLIST)

'type' SYNTAX
    concatenate(Position: POS, Left: SYNTAX, Right: SYNTAX)
    alternate(Position: POS, Left: SYNTAX, Right: SYNTAX)
    repeat(Position: POS, Element: SYNTAX)
    list(Position: POS, Element: SYNTAX, Delimiter: SYNTAX)
    optional(Position: POS, Operand: SYNTAX)
    keyword(Position: POS, Value: STRING)
    markedrule(Position: POS, Variable: ID, Name: ID)
    rule(Position: POS, Name: ID)
    mark(Position: POS, Variable: ID, Value: CONSTANT)

'type' SYNTAXCLASS
    phrase
    statement
    expression
    unary(Assoc: SYNTAXASSOC, Precedence: INT)
    binary(Assoc: SYNTAXASSOC, Precedence: INT)

'type' SYNTAXASSOC
    left
    right
    neutral

'type' CONSTANTLIST
    constantlist(Head: CONSTANT, Tail: CONSTANTLIST)
    nil

'type' CONSTANT
    undefined(Position: POS)
    true(Position: POS)
    false(Position: POS)
    integer(Position: POS, Value: INT)
    string(Position: POS, Value: STRING)
    name(Position: POS, Value: ID)

'type' IDLIST
    idlist(Head: ID, Tail: IDLIST)
    nil

'table' ID(Position: POS, Name: NAME)

'type' NAME
'type' DOUBLE

--------------------------------------------------------------------------------

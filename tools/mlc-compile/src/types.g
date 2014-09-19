'module' types

'use'

'export'
    MODULE
    IMPORT
    DEFINITION SIGNATURE
    TYPE
    PARAMETER MODE PARAMETERLIST
    STATEMENT
    EXPRESSION EXPRESSIONLIST
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
    type(Position: POS, Name: ID, Type: TYPE)
    constant(Position: POS, Name: ID, Value: EXPRESSION)
    variable(Position: POS, Name: ID, Type: TYPE)
    handler(Position: POS, Name: ID, Signature: SIGNATURE, Body: STATEMENT)
    foreignhandler(Position: POS, Name: ID, Signature: SIGNATURE, Binding: STRING)
    property(Position: POS)
    event(Position: POS)
    nil

'type' SIGNATURE
    signature(Parameters: PARAMETERLIST, ReturnType: TYPE)

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

'type' IDLIST
    idlist(Head: ID, Tail: IDLIST)
    nil

'table' ID(Position: POS, Name: NAME)

'type' NAME
'type' DOUBLE

--------------------------------------------------------------------------------

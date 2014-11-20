'module' types

'use'
    support

'export'
    MODULE MODULELIST
    IMPORT
    DEFINITION SIGNATURE ACCESS
    TYPE FIELD FIELDLIST
    PARAMETER MODE PARAMETERLIST
    STATEMENT
    EXPRESSION EXPRESSIONLIST
    INVOKELIST INVOKEINFO INVOKESIGNATURE
    SYNTAX SYNTAXCLASS SYNTAXASSOC SYNTAXCONSTANT SYNTAXCONSTANTLIST SYNTAXMETHOD SYNTAXMETHODLIST SYNTAXTERM
    ID IDLIST
    INTLIST
    MEANING
    MODULEINFO
    SYMBOLINFO SYMBOLKIND
    SYNTAXINFO
    SYNTAXMARKINFO SYNTAXMARKTYPE
    NAME DOUBLE

--------------------------------------------------------------------------------

'type' MODULELIST
    modulelist(Head: MODULE, Tail: MODULELIST)
    nil

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
    property(Position: POS, Access: ACCESS, Name: ID)
    event(Position: POS, Access: ACCESS, Name: ID)
    syntax(Position: POS, Access: ACCESS, Name: ID, Class: SYNTAXCLASS, Syntax: SYNTAX, Methods: SYNTAXMETHODLIST)
    nil

'type' SIGNATURE
    signature(Parameters: PARAMETERLIST, ReturnType: TYPE)

'type' ACCESS
    inferred
    public
    protected
    private

'type' TYPE
    any(Position: POS)
    undefined(Position: POS)
    named(Position: POS, Name: ID)
    optional(Position: POS, Type: TYPE)
    opaque(Position: POS, Base: TYPE, Fields: FIELDLIST)
    record(Position: POS, Base: TYPE, Fields: FIELDLIST)
    enum(Position: POS, Base: TYPE, Fields: FIELDLIST)
    handler(Position: POS, Signature: SIGNATURE)
    pointer(Position: POS)
    bool(Position: POS)
    int(Position: POS)
    uint(Position: POS)
    float(Position: POS)
    double(Position: POS)
    boolean(Position: POS)
    integer(Position: POS)
    real(Position: POS)
    number(Position: POS)
    string(Position: POS)
    data(Position: POS)
    array(Position: POS)
    list(Position: POS, Element: TYPE)
    nil

'type' FIELDLIST
    fieldlist(Head: FIELD, Tail: FIELDLIST)
    nil
    
'type' FIELD
    action(Position: POS, Name: ID, Handler: ID)
    slot(Position: POS, Name: ID, Type: TYPE)
    element(Position: POS, Name: ID)
    nil

'type' PARAMETERLIST
    parameterlist(Head: PARAMETER, Tail: PARAMETERLIST)
    nil
    
'type' PARAMETER
    parameter(Position: POS, Mode: MODE, Name: ID, Type: TYPE)
    
'type' MODE
    uncomputed
    in
    out
    inout

'type' STATEMENT
    sequence(Left: STATEMENT, Right: STATEMENT)
    variable(Position: POS, Name: ID, Type: TYPE)
    if(Position: POS, Condition: EXPRESSION, Consequent: STATEMENT, Alternate: STATEMENT)
    repeatforever(Position: POS, Body: STATEMENT)
    repeatcounted(Position: POS, Count: EXPRESSION, Body: STATEMENT)
    repeatwhile(Position: POS, Condition: EXPRESSION, Body: STATEMENT)
    repeatuntil(Position: POS, Condition: EXPRESSION, Body: STATEMENT)
    repeatupto(Position: POS, Slot: ID, Start: EXPRESSION, Finish: EXPRESSION, Step: EXPRESSION, Body: STATEMENT)
    repeatdownto(Position: POS, Slot: ID, Start: EXPRESSION, Finish: EXPRESSION, Step: EXPRESSION, Body: STATEMENT)
    repeatforeach(Position: POS, Iterator: EXPRESSION, Slot: ID, Container: EXPRESSION, Body: STATEMENT)
    nextrepeat(Position: POS)
    exitrepeat(Position: POS)
    return(Position: POS, Value: EXPRESSION)
    call(Position: POS, Handler: ID, Arguments: EXPRESSIONLIST)
    invoke(Position: POS, Info: INVOKELIST, Arguments: EXPRESSIONLIST)
    nil
    
'type' EXPRESSIONLIST
    expressionlist(Head: EXPRESSION, Tail: EXPRESSIONLIST)
    nil
    
'type' EXPRESSION
    undefined(Position: POS)
    true(Position: POS)
    false(Position: POS)
    integer(Position: POS, Value: INT)
    real(Position: POS, Value: DOUBLE)
    string(Position: POS, Value: STRING)
    slot(Position: POS, Name: ID)
    as(Position: POS, Value: EXPRESSION, Type: TYPE)
    list(Position: POS, List: EXPRESSIONLIST)
    call(Position: POS, Handler: ID, Arguments: EXPRESSIONLIST)
    invoke(Position: POS, Info: INVOKELIST, Arguments: EXPRESSIONLIST)
    nil

'type' INVOKELIST
    invokelist(Info: INVOKEINFO, Rest: INVOKELIST)
    nil

'type' INVOKESIGNATURE
    invokesignature(Mode: MODE, Tail: INVOKESIGNATURE)
    nil

'type' SYNTAX
    concatenate(Position: POS, Left: SYNTAX, Right: SYNTAX)
    alternate(Position: POS, Left: SYNTAX, Right: SYNTAX)
    repeat(Position: POS, Element: SYNTAX)
    list(Position: POS, Element: SYNTAX, Delimiter: SYNTAX)
    optional(Position: POS, Operand: SYNTAX)
    keyword(Position: POS, Value: STRING)
    markedrule(Position: POS, Variable: ID, Name: ID)
    rule(Position: POS, Name: ID)
    mark(Position: POS, Variable: ID, Value: SYNTAXCONSTANT)

'type' SYNTAXCLASS
    phrase
    statement
    iterator
    expression
    prefix(Precedence: INT)
    postfix(Precedence: INT)
    binary(Assoc: SYNTAXASSOC, Precedence: INT)
    expressionphrase
    expressionlistphrase

'type' SYNTAXASSOC
    left
    right
    neutral

'type' SYNTAXCONSTANTLIST
    constantlist(Head: SYNTAXCONSTANT, Tail: SYNTAXCONSTANTLIST)
    nil

'type' SYNTAXCONSTANT
    undefined(Position: POS)
    true(Position: POS)
    false(Position: POS)
    integer(Position: POS, Value: INT)
    real(Position: POS, Value: DOUBLE)
    string(Position: POS, Value: STRING)
    variable(Position: POS, Name: ID)
    indexedvariable(Position: POS, Name: ID, Index: INT)

'type' SYNTAXMETHODLIST
    methodlist(Head: SYNTAXMETHOD, Tail: SYNTAXMETHODLIST)
    nil
    
'type' SYNTAXMETHOD
    method(Position: POS, Name: ID, Arguments: SYNTAXCONSTANTLIST)

'type' SYNTAXTERM
    undefined
    error
    mark
    expression
    keyword
    mixed

'type' IDLIST
    idlist(Head: ID, Tail: IDLIST)
    nil

'type' MEANING
    definingid(Id: ID)
    module(Info: MODULEINFO)
    symbol(Info: SYMBOLINFO)
    syntax(Info: SYNTAXINFO)
    syntaxmark(Info: SYNTAXMARKINFO)
    error
    nil

'type' SYNTAXMARKTYPE
    uncomputed
    undefined
    error
    boolean
    integer
    real
    string
    phrase
    expression
    input
    output
    context
    iterator
    container
    
'type' SYMBOLKIND
    module
    type
    constant
    variable
    handler
    property
    event
    parameter
    local

'type' INTLIST
    intlist(Head: INT, Tail: INTLIST)
    nil

'table' ID(Position: POS, Name: NAME, Meaning: MEANING)

'table' MODULEINFO(Index: INT)
'table' SYMBOLINFO(Index: INT, Parent: ID, Access: ACCESS, Kind: SYMBOLKIND, Type: TYPE)
'table' SYNTAXINFO(Index: INT, Parent: ID, Class: SYNTAXCLASS, Syntax: SYNTAX, Methods: SYNTAXMETHODLIST, Prefix: SYNTAXTERM, Suffix: SYNTAXTERM)
'table' SYNTAXMARKINFO(Index: INT, RMode: MODE, LMode: MODE, Type: SYNTAXMARKTYPE)
'table' INVOKEINFO(Index: INT, ModuleIndex: INT, Name: STRING, ModuleName: STRING, RSignature: INVOKESIGNATURE, LSignature: INVOKESIGNATURE)

'table' TYPEINFO(Position: POS)

--------------------------------------------------------------------------------

'type' NAME
'type' DOUBLE

--------------------------------------------------------------------------------

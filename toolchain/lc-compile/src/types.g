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

'module' types

'use'
    support

'export'
    MODULE MODULELIST MODULEKIND
    DEFINITION SIGNATURE ACCESS
    TYPE FIELD FIELDLIST LANGUAGE
    PARAMETER MODE PARAMETERLIST
    STATEMENT
    EXPRESSION EXPRESSIONLIST
    INVOKELIST INVOKEINFO INVOKESIGNATURE INVOKEMETHODTYPE INVOKEMETHODLIST
    SYNTAX SYNTAXCLASS SYNTAXASSOC SYNTAXCONSTANT SYNTAXCONSTANTLIST SYNTAXMETHOD SYNTAXMETHODLIST SYNTAXTERM SYNTAXWARNING
    ID IDLIST OPTIONALID
    INTLIST
    NAMELIST
    MEANING
    MODULEINFO
    SYMBOLINFO SYMBOLKIND SYMBOLSAFETY
    SYNTAXINFO
    SYNTAXMARKINFO SYNTAXMARKTYPE
    NAME DOUBLE
    SYNTAXPRECEDENCE
    BYTECODE
    QueryExpressionListLength

--------------------------------------------------------------------------------

'type' MODULELIST
    modulelist(Head: MODULE, Tail: MODULELIST)
    nil

'type' MODULEKIND
    import
    module
    library
    widget
    application

'type' MODULE
    module(Position: POS, Kind: MODULEKIND, Name: ID, Definitions: DEFINITION)

'type' DEFINITION
    sequence(Left: DEFINITION, Right: DEFINITION)
    metadata(Position: POS, Key: STRING, Value: STRING)
    import(Position: POS, Name: ID)
    type(Position: POS, Access: ACCESS, Name: ID, Type: TYPE)
    constant(Position: POS, Access: ACCESS, Name: ID, Value: EXPRESSION)
    variable(Position: POS, Access: ACCESS, Name: ID, Type: TYPE)
    handler(Position: POS, Access: ACCESS, Name: ID, Signature: SIGNATURE, Definitions: DEFINITION, Body: STATEMENT)
    foreignhandler(Position: POS, Access: ACCESS, Name: ID, Signature: SIGNATURE, Binding: STRING)
    property(Position: POS, Access: ACCESS, Name: ID, Getter: ID, Setter: OPTIONALID)
    event(Position: POS, Access: ACCESS, Name: ID, Signature: SIGNATURE)
    syntax(Position: POS, Access: ACCESS, Name: ID, Class: SYNTAXCLASS, Warnings: SYNTAXWARNING, Syntax: SYNTAX, Methods: SYNTAXMETHODLIST)
    unsafe(Position: POS, Definition: DEFINITION)
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
    foreign(Position: POS, Binding: STRING)
    optional(Position: POS, Type: TYPE)
    record(Position: POS, Fields: FIELDLIST)
    enum(Position: POS, Base: TYPE, Fields: FIELDLIST)
    handler(Position: POS, Language: LANGUAGE, Signature: SIGNATURE)
    boolean(Position: POS)
    integer(Position: POS)
    real(Position: POS)
    number(Position: POS)
    string(Position: POS)
    data(Position: POS)
    array(Position: POS)
    list(Position: POS, Element: TYPE)
    unspecified
    nil

'type' LANGUAGE
    normal
    foreign

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
    variadic

'type' BYTECODE
    sequence(Left: BYTECODE, Right: BYTECODE)
    label(Position: POS, Name: ID)
    register(Position: POS, Name: ID, Type: TYPE)
    opcode(Position: POS, Opcode: NAME, Arguments: EXPRESSIONLIST)
    nil

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
    repeatforeach(Position: POS, Iterator: EXPRESSION, Container: EXPRESSION, Body: STATEMENT)
    nextrepeat(Position: POS)
    exitrepeat(Position: POS)
    return(Position: POS, Value: EXPRESSION)
    put(Position: POS, Source: EXPRESSION, Target: EXPRESSION)
    get(Position: POS, Value: EXPRESSION)
    call(Position: POS, Handler: ID, Arguments: EXPRESSIONLIST)
    invoke(Position: POS, Info: INVOKELIST, Arguments: EXPRESSIONLIST)
    throw(Position: POS, Error: EXPRESSION)
    postfixinto(Position: POS, Command: STATEMENT, Target: EXPRESSION)
    bytecode(Position: POS, Block: BYTECODE)
    unsafe(Position: POS, Block: STATEMENT)
    nil
    
'type' EXPRESSIONLIST
    expressionlist(Head: EXPRESSION, Tail: EXPRESSIONLIST)
    nil
    
'type' EXPRESSION
    undefined(Position: POS)
    true(Position: POS)
    false(Position: POS)
    integer(Position: POS, Value: INT)
    unsignedinteger(Position: POS, Value: INT)
    real(Position: POS, Value: DOUBLE)
    string(Position: POS, Value: STRING)
    slot(Position: POS, Name: ID)
    result(Position: POS)
    logicaland(Position: POS, Left: EXPRESSION, Right: EXPRESSION)
    logicalor(Position: POS, Left: EXPRESSION, Right: EXPRESSION)
    as(Position: POS, Value: EXPRESSION, Type: TYPE)
    list(Position: POS, List: EXPRESSIONLIST)
    call(Position: POS, Handler: ID, Arguments: EXPRESSIONLIST)
    invoke(Position: POS, Info: INVOKELIST, Arguments: EXPRESSIONLIST)
    array(Position: POS, Pairs: EXPRESSIONLIST)
    pair(Position: POS, Key: EXPRESSION, Value: EXPRESSION)
    nil

'type' INVOKELIST
    invokelist(Info: INVOKEINFO, Rest: INVOKELIST)
    nil

'type' INVOKESIGNATURE
    invokesignature(Mode: MODE, Index: INT, Tail: INVOKESIGNATURE)
    nil

'type' SYNTAX
    concatenate(Position: POS, Left: SYNTAX, Right: SYNTAX)
    alternate(Position: POS, Left: SYNTAX, Right: SYNTAX)
    repeat(Position: POS, Element: SYNTAX)
    list(Position: POS, Element: SYNTAX, Delimiter: SYNTAX)
    optional(Position: POS, Operand: SYNTAX)
    keyword(Position: POS, Value: STRING)
    unreservedkeyword(Position: POS, Value: STRING)
    markedrule(Position: POS, Variable: ID, Name: ID)
    rule(Position: POS, Name: ID)
    mark(Position: POS, Variable: ID, Value: SYNTAXCONSTANT)

'type' SYNTAXWARNING
    deprecated(Message: STRING)
    nil

'type' SYNTAXCLASS
    phrase
    statement
    iterator
    expression
    prefix(Precedence: SYNTAXPRECEDENCE)
    postfix(Precedence: SYNTAXPRECEDENCE)
    binary(Assoc: SYNTAXASSOC, Precedence: SYNTAXPRECEDENCE)
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
    handler
    property
    event
    variable
    parameter
    local
    context
    label

'type' SYMBOLSAFETY
    safe
    unsafe

'type' INTLIST
    intlist(Head: INT, Tail: INTLIST)
    nil

'type' INVOKEMETHODTYPE
    execute
    evaluate
    assign
    iterate

'type' INVOKEMETHODLIST
    methodlist(Name: STRING, Type: INVOKEMETHODTYPE, Signature: INVOKESIGNATURE, Tail: INVOKEMETHODLIST)
    nil

'type' NAMELIST
    namelist(Name: NAME, Rest: NAMELIST)
    nil

'type' OPTIONALID
    id(Id: ID)
    nil

'table' ID(Position: POS, Name: NAME, Meaning: MEANING, Namespace: OPTIONALID)

'table' MODULEINFO(Index: INT, Generator: INT)
'table' SYMBOLINFO(Index: INT, Generator: INT, Parent: ID, Access: ACCESS, Safety: SYMBOLSAFETY, Kind: SYMBOLKIND, Type: TYPE)
'table' SYNTAXINFO(Index: INT, Parent: ID, Class: SYNTAXCLASS, Syntax: SYNTAX, Methods: SYNTAXMETHODLIST, Prefix: SYNTAXTERM, Suffix: SYNTAXTERM)
'table' SYNTAXMARKINFO(Index: INT, RMode: MODE, LMode: MODE, Type: SYNTAXMARKTYPE)
'table' INVOKEINFO(Index: INT, ModuleIndex: INT, Name: STRING, ModuleName: STRING, Methods: INVOKEMETHODLIST)

'table' TYPEINFO(Position: POS)

--------------------------------------------------------------------------------

-- All operators are classified as particular types representing their syntactic
-- structure. These patterns have fixed precedence when used in expressions to
-- ensure consistent interpretation.
--
-- These classifications easy syntactic description and ensure consistency. The
-- parser maps them to concrete operator type and precedence level before
-- building the AST.
--
-- Some classifications have the same underlying syntactic pattern but distinction
-- is made to make it clear what the intent of the syntax is. Generally, if two
-- classifications share a syntax pattern, then they must share the same precedence
-- and operator type to ensure the principal of least surprise is enforced.
--
-- See also /docs/specs/lcb-precedence.md
--
'type' SYNTAXPRECEDENCE
    scoperesolution,  -- com.livecode.module.Identifier
    functioncall,     -- Handler()
    subscript,        -- tList[5]
    property,         -- the paint of tCanvas
    subscriptchunk,   -- char 5 of tString
    conversion,       -- tString parsed as number
    functionchunk,    -- the length of tList
    modifier,         -- -tNumber, bitwise not tNumber
    exponentiation,   -- 2 ^ 10
    multiplication,   -- /, *, div, mod
    addition,         -- +, -
    concatenation,    -- &, &&
    bitwiseshift,     -- 1 shifted left by 3 bitwise
    bitwiseand,       -- 5 bitwise and 1
    bitwisexor,       -- 5 bitwise xor 1
    bitwiseor,        -- 5 bitwise or 1
    constructor,      -- rectangle tList
    comparison,       -- <=, <, is, =
    classification,   -- 5 is a number
    logicalnot,       -- not tBoolean
    logicaland,       -- tLeft and tRight
    logicalor,        -- tLeft or tRight
    sequence          -- ,

--------------------------------------------------------------------------------

'type' NAME
'type' DOUBLE

--------------------------------------------------------------------------------

'action' QueryExpressionListLength(EXPRESSIONLIST -> INT)

    'rule' QueryExpressionListLength(expressionlist(_, Tail) -> TailCount + 1)
        QueryExpressionListLength(Tail -> TailCount)

    'rule' QueryExpressionListLength(nil -> 0)
        -- nothing

--------------------------------------------------------------------------------

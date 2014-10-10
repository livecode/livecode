'module' check

'export'
    Check

'use'
    support
    types

--------------------------------------------------------------------------------

'action' Check(MODULE)

    'rule' Check(Module):
        --

--------------------------------------------------------------------------------

-- The syntax clauses have the following rules:
--   1) Mark variables can be used only once on any concrete path through the
--      syntax.
--   2) Mark variables cannot have the name 'output', 'input', 'context' or
--      'error'.
--   4) The methods referenced in the mapping section must exist.
--   5) A signature of a method referenced must match the derived signature
--      of the parameters specified for it.
--   6) A binary operator must start with an Expression and end with an
--      Expression.
--   7) A left unary operator

'sweep' CheckSyntax(ANY)

    'rule' CheckSyntax(DEFINITION'syntax(_, _, _, _, Syntax, Methods)):
        -- Now we've bound all mark variable ids to an index, we can check to
        -- see if there are any cases where they are defined twice.
        PushEmptySet()
        CheckSyntaxMarkDefinitions(Syntax)
        
-- Mark variables can only be defined once for each possible path through a
-- syntax rule. For example:
--   <Foo: Expression> <Foo: Expression> is not allowed
--   ( <Foo: Expression> | <Foo: Expression> ) is allowed.
--
'action' CheckSyntaxMarkDefinitions(SYNTAX)

    'rule' CheckSyntaxMarkDefinitions(concatenate(_, Left, Right)):
        CheckSyntaxMarkDefinitions(Left)
        CheckSyntaxMarkDefinitions(Right)
        
    'rule' CheckSyntaxMarkDefinitions(alternate(_, Left, Right)):
        DuplicateSet()
        CheckSyntaxMarkDefinitions(Left)
        ExchangeSet()
        CheckSyntaxMarkDefinitions(Right)
        UnionSet()

    'rule' CheckSyntaxMarkDefinitions(repeat(_, Element)):
        CheckSyntaxMarkDefinitions(Element)
        
    'rule' CheckSyntaxMarkDefinitions(list(_, Element, Delimiter)):
        CheckSyntaxMarkDefinitions(Element)
        CheckNoSyntaxMarkDefinitionsInDelimiter(Delimiter)
        
    'rule' CheckSyntaxMarkDefinitions(optional(_, Operand)):
        CheckSyntaxMarkDefinitions(Operand)

    'rule' CheckSyntaxMarkDefinitions(markedrule(_, Variable, _)):
        CheckSyntaxMarkVariableNotDefined(Variable)

    'rule' CheckSyntaxMarkDefinitions(mark(_, Variable, _)):
        CheckSyntaxMarkVariableNotDefined(Variable)

    'rule' CheckSyntaxMarkDefinitions(_):
        -- do nothing

'action' CheckSyntaxMarkVariableNotDefined(ID)

    'rule' CheckSyntaxMarkVariableNotDefined(Variable):
        Variable'Meaning -> syntaxmark(Index)
        (|
            IsIndexInSet(Index)
            Variable'Position -> Position
            Variable'Name -> Name
            Error_SyntaxMarkVariableAlreadyDefined(Position, Name)
        ||
            IncludeIndexInSet(Index)
        |)

'sweep' CheckNoSyntaxMarkDefinitionsInDelimiter(ANY)

    'rule' CheckNoSyntaxMarkDefinitionsInDelimiter(markedrule(_, Variable, _)):
        Variable'Position -> Position
        Error_SyntaxMarkVariableNotAllowedInDelimiter(Position)

    'rule' CheckNoSyntaxMarkDefinitionsInDelimiter(mark(_, Variable, _)):
        Variable'Position -> Position
        Error_SyntaxMarkVariableNotAllowedInDelimiter(Position)

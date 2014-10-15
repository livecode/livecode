'module' check

'export'
    Check

'use'
    support
    types

--------------------------------------------------------------------------------

'action' Check(MODULE)

    'rule' Check(Module):
        -- Check that all the ids bind to the correct type of thing. Any ids
        -- which aren't bound correctly get assigned a meaning of 'error'.
        CheckBindings(Module)
        
        -- Check the syntax definitions are all correct.
        CheckSyntaxDefinitions(Module)

--------------------------------------------------------------------------------

-- At this point all identifiers either have a defined meaning, or are defined
-- to be a pointer to the definingid. The next step is to check that bindings
-- are appropriate to each id:
--   B1) The id in TYPE'named() must refer to a type id.
--   B2) The id's in DEFINITION'constant(Value) must all refer to constants (non-recursively)
--   B3) The id's in SYNTAX'rule(Name) and SYNTAX'markedrule(Name) must either be Expression or a phrase definition.
--   B4) The id's in SYNTAX'mark(Variable) and SYNTAX'markedrule(Variable) must be a syntaxmark
--   B5) The id in SYNTAXMETHOD'method(name) must be a handler
--   B6) The id's in SYNTAXMETHOD'method(Arguments) must be a syntaxmark, syntaxoutputmark, syntaxinputmark or syntaxcontextmark.

'sweep' CheckBindings(ANY)

    'rule' CheckBindings(DEFINITION'constant(Position, Name, _, Value)):
        /* B2 */ CheckBindingOfConstantExpressionIds(Value)

    'rule' CheckBindings(TYPE'named(Position, Name)):
        /* B1 */ CheckBindingOfNamedTypeId(Name)

    'rule' CheckBindings(SYNTAX'rule(_, Name)):
        /* B3 */ CheckBindingOfSyntaxRule(Name)

    'rule' CheckBindings(SYNTAX'markedrule(_, Variable, Name)):
        /* B4 */ CheckBindingOfSyntaxMark(Variable)
        /* B3 */ CheckBindingOfSyntaxRule(Name)
        
    'rule' CheckBindings(SYNTAX'mark(_, Variable, Value)):
        /* B4 */ CheckBindingOfSyntaxMark(Variable)
    
    'rule' CheckBindings(SYNTAXMETHOD'method(_, Name, Arguments)):
        /* B5 */ CheckBindingOfSyntaxMethod(Name)
        /* B6 */ CheckBindings(Arguments)
        
'sweep' CheckBindingOfConstantExpressionIds(ANY)

    'rule' CheckBindingOfConstantExpressionIds(EXPRESSION'nil):
        -- TODO
        
'action' CheckBindingOfNamedTypeId(ID)

    'rule' CheckBindingOfNamedTypeId(Id):
        -- Do nothing if the meaning is error.
        QueryId(Id -> error)

    'rule' CheckBindingOfNamedTypeId(Id):
        QueryId(Id -> type)
        
    'rule' CheckBindingOfNamedTypeId(Id):
        Id'Name -> Name
        Id'Position -> Position
        Error_NotBoundToAType(Position, Name)
        -- Mark this id as being in error.
        Id'Meaning <- error
        
'action' CheckBindingOfSyntaxRule(ID)

    'rule' CheckBindingOfSyntaxRule(Id):
        -- Do nothing if the meaning is error.
        QueryId(Id -> error)

    'rule' CheckBindingOfSyntaxRule(Id):
        QueryId(Id -> syntaxexpressionrule)

    'rule' CheckBindingOfSyntaxRule(Id):
        QueryId(Id -> syntaxexpressionlistrule)
        
    'rule' CheckBindingOfSyntaxRule(Id):
        QueryId(Id -> syntaxrule(phrase, _))
        
    'rule' CheckBindingOfSyntaxRule(Id):
        QueryId(Id -> syntaxrule(_, _))
        Id'Name -> Name
        Id'Position -> Position
        Error_NotBoundToAPhrase(Position, Name)
        -- Mark this id as being in error.
        Id'Meaning <- error
        
    'rule' CheckBindingOfSyntaxRule(Id):
        Id'Name -> Name
        Id'Position -> Position
        Error_NotBoundToASyntaxRule(Position, Name)
        -- Mark this id as being in error.
        Id'Meaning <- error

'action' CheckBindingOfSyntaxMark(ID)

    'rule' CheckBindingOfSyntaxMark(Id):
        QueryId(Id -> error)

    'rule' CheckBindingOfSyntaxMark(Id):
        QueryId(Id -> syntaxmark(_))
        
    'rule' CheckBindingOfSyntaxMark(Id):
        Id'Name -> Name
        Id'Position -> Position
        Error_NotBoundToASyntaxMark(Position, Name)
        -- Mark this id as being in error.
        Id'Meaning <- error

'action' CheckBindingOfSyntaxMarkUse(ID)

    'rule' CheckBindingOfSyntaxMarkUse(Id):
        QueryId(Id -> error)

    'rule' CheckBindingOfSyntaxMarkUse(Id):
        QueryId(Id -> syntaxmark(_))

    'rule' CheckBindingOfSyntaxMarkUse(Id):
        QueryId(Id -> syntaxoutputmark)

    'rule' CheckBindingOfSyntaxMarkUse(Id):
        QueryId(Id -> syntaxinputmark)

    'rule' CheckBindingOfSyntaxMarkUse(Id):
        QueryId(Id -> syntaxcontextmark)
        
    'rule' CheckBindingOfSyntaxMarkUse(Id):
        Id'Name -> Name
        Id'Position -> Position
        Error_NotBoundToASyntaxMark(Position, Name)
        -- Mark this id as being in error.
        Id'Meaning <- error
        
'action' CheckBindingOfSyntaxMethod(ID)

    'rule' CheckBindingOfSyntaxMethod(Id):
        QueryId(Id -> error)

    'rule' CheckBindingOfSyntaxMethod(Id):
        QueryId(Id -> handler)
        
    'rule' CheckBindingOfSyntaxMethod(Id):
        Id'Name -> Name
        Id'Position -> Position
        Error_NotBoundToAHandler(Position, Name)
        -- Mark this id as being in error.
        Id'Meaning <- error

--------------------------------------------------------------------------------

-- The syntax clauses have the following rules:
--   S1) Mark variables can be used only once on any concrete path through the
--      syntax. (DONE)
--   S2) Mark variables cannot have the name 'output', 'input', 'context' or
--      'error'. (DONE - in Bind)
--   S3) A signature of a method referenced must match the derived signature
--      of the parameters specified for it.
--   S4) A binary operator must start with an Expression and end with an
--      Expression. (DONE)
--   S5) A prefix operator must end with an Expression. (DONE)
--   S6) A postfix operator must start with an Expression. (DONE)
--   S7) Expression syntax methods must either use 'output' or 'input' as one
--      parameter, but not both.
--   S8) Only terminals are allowed in the delimiter section of a repetition. (DONE)
--   S9) The element section of a repetition must not be nullable. (DONE)
--   S10) Rules must either be 'Expression' or the name of a phrase definition. (DONE - CheckBindings)
--   S11) Values of marks must be constant. (DONE)
--   S12) Optional elements must contain a terminal (DONE)

'sweep' CheckSyntaxDefinitions(ANY)

    'rule' CheckSyntaxDefinitions(DEFINITION'syntax(Position, _, _, Class, Syntax, Methods)):
        -- Check that mark variables are only defined once on each path.
        PushEmptySet()
        /* S1 */ CheckSyntaxMarkDefinitions(Syntax)
        
        -- Check that operators have the appropriate form.
        /* S4, S5, S6 */ CheckSyntaxStructure(Position, Class, Syntax)
        
        -- Check that the repeat clauses have supported structure.
        /* S8, S9 */ CheckSyntaxRepetitions(Syntax)
        
        -- Check that optional elements contain a terminal
        /* S12 */ CheckSyntaxOptionals(Syntax)
        
        -- Check that values of marks are constant
        /* S11 */ CheckSyntaxMarks(Syntax)
        
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
        
    'rule' CheckSyntaxMarkVariableNotDefined(Variable):
        Variable'Meaning -> error

--

'action' CheckSyntaxStructure(POS, SYNTAXCLASS, SYNTAX)

    'rule' CheckSyntaxStructure(Position, Class, Syntax):
        ComputeSyntaxPrefixAndSuffix(Syntax -> Prefix, Suffix)
        CheckPrefixOfSyntaxStructure(Position, Class, Prefix)
        CheckSuffixOfSyntaxStructure(Position, Class, Suffix)

'action' CheckPrefixOfSyntaxStructure(POS, SYNTAXCLASS, SYNTAXTERM)

    'rule' CheckPrefixOfSyntaxStructure(Position, _, error):
        -- Do nothing if term is an error.

    'rule' CheckPrefixOfSyntaxStructure(Position, expression, Term):
        [|
            where(Term -> expression)
            Error_ExpressionSyntaxCannotStartWithExpression(Position)
        |]

    'rule' CheckPrefixOfSyntaxStructure(Position, prefix, Term):
        [|
            where(Term -> expression)
            Error_PrefixSyntaxCannotStartWithExpression(Position)
        |]
        
    'rule' CheckPrefixOfSyntaxStructure(Position, postfix, Term):
        (|
            where(Term -> expression)
        ||
            Error_PostfixSyntaxMustStartWithExpression(Position)
        |)
        
    'rule' CheckPrefixOfSyntaxStructure(Position, binary, Term):
        (|
            where(Term -> expression)
        ||
            Error_BinarySyntaxMustStartWithExpression(Position)
        |)
        
    'rule' CheckPrefixOfSyntaxStructure(_, _, _):
        -- Do nothing

'action' CheckSuffixOfSyntaxStructure(POS, SYNTAXCLASS, SYNTAXTERM)

    'rule' CheckSuffixOfSyntaxStructure(Position, _, error):
        -- Do nothing if term is an error.

    'rule' CheckSuffixOfSyntaxStructure(Position, expression, Term):
        [|
            where(Term -> expression)
            Error_ExpressionSyntaxCannotFinishWithExpression(Position)
        |]

    'rule' CheckSuffixOfSyntaxStructure(Position, prefix, Term):
        (|
            where(Term -> expression)
        ||
            Error_PrefixSyntaxMustFinishWithExpression(Position)
        |)
        
    'rule' CheckSuffixOfSyntaxStructure(Position, postfix, Term):
        [|
            where(Term -> expression)
            Error_PostfixSyntaxCannotFinishWithExpression(Position)
        |]
        
    'rule' CheckSuffixOfSyntaxStructure(Position, binary, Term):
        (|
            where(Term -> expression)
        ||
            Error_BinarySyntaxMustFinishWithExpression(Position)
        |)

    'rule' CheckSuffixOfSyntaxStructure(_, _, _):
        -- Do nothing

--

'action' CheckSyntaxRepetitions(SYNTAX)

    'rule' CheckSyntaxRepetitions(concatenate(_, Left, Right)):
        CheckSyntaxRepetitions(Left)
        CheckSyntaxRepetitions(Right)
        
    'rule' CheckSyntaxRepetitions(alternate(_, Left, Right)):
        CheckSyntaxRepetitions(Left)
        CheckSyntaxRepetitions(Right)

    'rule' CheckSyntaxRepetitions(repeat(_, Element)):
        CheckSyntaxRepetitions(Element)
        
    'rule' CheckSyntaxRepetitions(list(Position, Element, Delimiter)):
        CheckSyntaxRepetitions(Element)
        CheckSyntaxOnlyKeywordsInDelimiter(Delimiter)
        [|
            SyntaxIsNullable(Element)
            Error_ElementSyntaxCannotBeNullable(Position)
        |]
        
    'rule' CheckSyntaxRepetitions(optional(_, Operand)):
        CheckSyntaxRepetitions(Operand)

    'rule' CheckSyntaxRepetitions(_):
        -- do nothing
        
'action' CheckSyntaxOnlyKeywordsInDelimiter(SYNTAX)

    'rule' CheckSyntaxOnlyKeywordsInDelimiter(concatenate(_, Left, Right)):
        CheckSyntaxOnlyKeywordsInDelimiter(Left)
        CheckSyntaxOnlyKeywordsInDelimiter(Right)
        
    'rule' CheckSyntaxOnlyKeywordsInDelimiter(alternate(_, Left, Right)):
        CheckSyntaxOnlyKeywordsInDelimiter(Left)
        CheckSyntaxOnlyKeywordsInDelimiter(Right)

    'rule' CheckSyntaxOnlyKeywordsInDelimiter(optional(_, Operand)):
        CheckSyntaxOnlyKeywordsInDelimiter(Operand)

    'rule' CheckSyntaxOnlyKeywordsInDelimiter(Syntax):
        Syntax'Position -> Position
        Error_OnlyKeywordsAllowedInDelimiterSyntax(Position)

----------

'action' CheckSyntaxOptionals(SYNTAX)

    'rule' CheckSyntaxOptionals(concatenate(_, Left, Right)):
        CheckSyntaxOptionals(Left)
        CheckSyntaxOptionals(Right)
        
    'rule' CheckSyntaxOptionals(alternate(_, Left, Right)):
        CheckSyntaxOptionals(Left)
        CheckSyntaxOptionals(Right)

    'rule' CheckSyntaxOptionals(repeat(_, Element)):
        CheckSyntaxOptionals(Element)
        
    'rule' CheckSyntaxOptionals(list(Position, Element, Delimiter)):
        CheckSyntaxOptionals(Element)
        CheckSyntaxOptionals(Delimiter)
        
    'rule' CheckSyntaxOptionals(optional(Position, Operand)):
        ComputeSyntaxPrefixAndSuffix(Operand -> Prefix, Suffix)
        [|
            eq(Prefix, Suffix)
            eq(Prefix, mark)
            Error_OptionalSyntaxCannotContainOnlyMarks(Position)
        |]

    'rule' CheckSyntaxOptionals(_):
        -- do nothing

----------

'action' CheckSyntaxMarks(SYNTAX)

    'rule' CheckSyntaxMarks(concatenate(_, Left, Right)):
        CheckSyntaxMarks(Left)
        CheckSyntaxMarks(Right)
        
    'rule' CheckSyntaxMarks(alternate(_, Left, Right)):
        CheckSyntaxMarks(Left)
        CheckSyntaxMarks(Right)

    'rule' CheckSyntaxMarks(repeat(_, Element)):
        CheckSyntaxMarks(Element)
        
    'rule' CheckSyntaxMarks(list(Position, Element, Delimiter)):
        CheckSyntaxMarks(Element)
        CheckSyntaxMarks(Delimiter)
        
    'rule' CheckSyntaxMarks(optional(_, Operand)):
        CheckSyntaxMarks(Operand)

    'rule' CheckSyntaxMarks(mark(Position, Variable, Value)):
        [|
            where(Value -> name(_, _))
            Error_SyntaxMarksMustBeConstant(Position)
        |]
        
    'rule' CheckSyntaxMarks(_):
        -- do nothing

----------

'action' ComputeSyntaxPrefixAndSuffix(SYNTAX -> SYNTAXTERM, SYNTAXTERM)

    'rule' ComputeSyntaxPrefixAndSuffix(concatenate(_, Left, Right) -> Prefix, Suffix):
        ComputeSyntaxPrefixAndSuffix(Left -> LeftPrefix, LeftSuffix)
        ComputeSyntaxPrefixAndSuffix(Right -> RightPrefix, RightSuffix)
        (|
            SyntaxIsNullable(Left)
            UnionSyntaxTerm(LeftPrefix, RightPrefix -> Prefix)
        ||
            where(LeftPrefix -> Prefix)
        |)
        (|
            SyntaxIsNullable(Right)
            UnionSyntaxTerm(LeftSuffix, RightSuffix -> Suffix)
        ||
            where(RightSuffix -> Suffix)
        |)

    'rule' ComputeSyntaxPrefixAndSuffix(alternate(_, Left, Right) -> Prefix, Suffix):
        ComputeSyntaxPrefixAndSuffix(Left -> LeftPrefix, LeftSuffix)
        ComputeSyntaxPrefixAndSuffix(Right -> RightPrefix, RightSuffix)
        UnionSyntaxTerm(LeftPrefix, RightPrefix -> Prefix)
        UnionSyntaxTerm(LeftSuffix, RightSuffix -> Suffix)
        
    'rule' ComputeSyntaxPrefixAndSuffix(optional(_, Element) -> Prefix, Suffix):
        ComputeSyntaxPrefixAndSuffix(Element -> Prefix, Suffix)

    'rule' ComputeSyntaxPrefixAndSuffix(repeat(_, Element) -> Prefix, Suffix):
        ComputeSyntaxPrefixAndSuffix(Element -> Prefix, Suffix)

    'rule' ComputeSyntaxPrefixAndSuffix(list(_, Element, _) -> Prefix, Suffix):
        ComputeSyntaxPrefixAndSuffix(Element -> Prefix, Suffix)

    'rule' ComputeSyntaxPrefixAndSuffix(rule(_, Name) -> Prefix, Suffix):
        ComputeSyntaxPrefixAndSuffixOfRule(Name -> Prefix, Suffix)

    'rule' ComputeSyntaxPrefixAndSuffix(markedrule(_, _, Name) -> Prefix, Suffix):
        ComputeSyntaxPrefixAndSuffixOfRule(Name -> Prefix, Suffix)
        
    'rule' ComputeSyntaxPrefixAndSuffix(mark(_, _, _) -> mark, mark):
        -- do nothing

    'rule' ComputeSyntaxPrefixAndSuffix(_ -> keyword, keyword):
        -- do nothing

'action' ComputeSyntaxPrefixAndSuffixOfRule(ID -> SYNTAXTERM, SYNTAXTERM)

    'rule' ComputeSyntaxPrefixAndSuffixOfRule(Name -> Prefix, Suffix)
        QueryId(Name -> Meaning)
        (|
            (|
                where(Meaning -> syntaxexpressionrule)
            ||
                where(Meaning -> syntaxexpressionlistrule)
            |)
            where(SYNTAXTERM'expression -> Prefix)
            where(SYNTAXTERM'expression -> Suffix)
        ||
            where(Meaning -> syntaxrule(_, Syntax))
            ComputeSyntaxPrefixAndSuffix(Syntax -> Prefix, Suffix)
        ||
            where(SYNTAXTERM'error -> Prefix)
            where(SYNTAXTERM'error -> Suffix)
        |)

'action' UnionSyntaxTerm(SYNTAXTERM, SYNTAXTERM -> SYNTAXTERM)

    'rule' UnionSyntaxTerm(error, _ -> error):
    'rule' UnionSyntaxTerm(_, error -> error):
    'rule' UnionSyntaxTerm(mark, Right -> Right):
    'rule' UnionSyntaxTerm(Left, mark -> Left):
    'rule' UnionSyntaxTerm(Left, Right -> Left):
        eq(Left, Right)
    'rule' UnionSyntaxTerm(_, _ -> mixed):
        --

'condition' SyntaxIsNullable(SYNTAX)

    'rule' SyntaxIsNullable(concatenate(_, Left, Right)):
        SyntaxIsNullable(Left)
        SyntaxIsNullable(Right)
        
    'rule' SyntaxIsNullable(alternate(_, Left, Right)):
        (|
            SyntaxIsNullable(Left)
        ||
            SyntaxIsNullable(Right)
        |)

    'rule' SyntaxIsNullable(optional(_, Operand)):
        -- optionals are nullable

    'rule' SyntaxIsNullable(rule(_, Name)):
        QueryId(Name -> syntaxrule(_, Syntax))
        SyntaxIsNullable(Syntax)
        
    'rule' SyntaxIsNullable(markedrule(_, _, Name)):
        QueryId(Name -> syntaxrule(_, Syntax))
        SyntaxIsNullable(Syntax)

    'rule' SyntaxIsNullable(mark(_, _, _)):
        -- marks are nullable

--------------------------------------------------------------------------------

'action' QueryId(ID -> MEANING)

    'rule' QueryId(Id -> Meaning):
        Id'Meaning -> definingid(DefId)
        DefId'Meaning -> Meaning
        
    'rule' QueryId(Id -> Meaning):
        Id'Meaning -> Meaning

--------------------------------------------------------------------------------
